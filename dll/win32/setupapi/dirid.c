/*
 * Directory id handling
 *
 * Copyright 2002 Alexandre Julliard for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "setupapi_private.h"

#define MAX_SYSTEM_DIRID DIRID_PRINTPROCESSOR
#define MIN_CSIDL_DIRID 0x4000
#define MAX_CSIDL_DIRID 0x403f

struct user_dirid
{
    int    id;
    WCHAR *str;
};

#ifdef __i386__
static const WCHAR printer_env[] = L"w32x86";
#elif defined __x86_64__
static const WCHAR printer_env[] = L"x64";
#elif defined __arm__
static const WCHAR printer_env[] = L"arm";
#elif defined __aarch64__
static const WCHAR printer_env[] = L"arm64";
#else
#error not defined for this cpu
#endif

static int nb_user_dirids;     /* number of user dirids in use */
static int alloc_user_dirids;  /* number of allocated user dirids */
static struct user_dirid *user_dirids;
static const WCHAR *printer_dirids[5];  /* 66000..66004 */
static const WCHAR *system_dirids[MAX_SYSTEM_DIRID+1];
static const WCHAR *csidl_dirids[MAX_CSIDL_DIRID-MIN_CSIDL_DIRID+1];

/* retrieve the string for unknown dirids */
static const WCHAR *get_unknown_dirid(void)
{
    static WCHAR *unknown_dirid;

    if (!unknown_dirid)
    {
        UINT len = GetSystemDirectoryW( NULL, 0 ) + lstrlenW(L"\\unknown");
        if (!(unknown_dirid = malloc( len * sizeof(WCHAR) ))) return NULL;
        GetSystemDirectoryW( unknown_dirid, len );
        lstrcatW( unknown_dirid, L"\\unknown" );
    }
    return unknown_dirid;
}

static const WCHAR *get_csidl_dir(DWORD csidl);

/* create the string for a system dirid */
static const WCHAR *create_system_dirid( int dirid )
{
    WCHAR buffer[MAX_PATH+32], *str;
    int len;

    switch(dirid)
    {
    case DIRID_NULL:
        return L"";
    case DIRID_WINDOWS:
        GetWindowsDirectoryW( buffer, MAX_PATH );
        break;
    case DIRID_SYSTEM:
        GetSystemDirectoryW( buffer, MAX_PATH );
        break;
    case DIRID_DRIVERS:
        GetSystemDirectoryW( buffer, MAX_PATH );
        lstrcatW( buffer, L"\\drivers" );
        break;
    case DIRID_INF:
        GetWindowsDirectoryW( buffer, MAX_PATH );
        lstrcatW( buffer, L"\\inf" );
        break;
    case DIRID_HELP:
        GetWindowsDirectoryW( buffer, MAX_PATH );
        lstrcatW( buffer, L"\\help" );
        break;
    case DIRID_FONTS:
        GetWindowsDirectoryW( buffer, MAX_PATH );
        lstrcatW( buffer, L"\\fonts" );
        break;
    case DIRID_VIEWERS:
        GetSystemDirectoryW( buffer, MAX_PATH );
        lstrcatW( buffer, L"\\viewers" );
        break;
    case DIRID_COLOR:
        GetSystemDirectoryW( buffer, MAX_PATH );
        lstrcatW( buffer, L"\\spool\\drivers\\color" );
        break;
    case DIRID_APPS:
        return L"C:\\";  /* FIXME */
    case DIRID_SHARED:
        GetWindowsDirectoryW( buffer, MAX_PATH );
        break;
    case DIRID_BOOT:
        return L"C:\\";  /* FIXME */
    case DIRID_SYSTEM16:
        GetWindowsDirectoryW( buffer, MAX_PATH );
        lstrcatW( buffer, L"\\system" );
        break;
    case DIRID_SPOOL:
        GetSystemDirectoryW( buffer, MAX_PATH );
        lstrcatW( buffer, L"\\spool" );
        break;
    case DIRID_SPOOLDRIVERS:
        GetSystemDirectoryW( buffer, MAX_PATH );
        lstrcatW( buffer, L"\\spool\\drivers" );
        break;
    case DIRID_USERPROFILE:
        if (GetEnvironmentVariableW( L"USERPROFILE", buffer, MAX_PATH )) break;
        return get_csidl_dir(CSIDL_PROFILE);
    case DIRID_LOADER:
        return L"C:\\";  /* FIXME */
    case DIRID_PRINTPROCESSOR:
        GetSystemDirectoryW( buffer, MAX_PATH );
        lstrcatW( buffer, L"\\spool\\prtprocs\\" );
        lstrcatW( buffer, printer_env );
        break;
    default:
        FIXME( "unknown dirid %d\n", dirid );
        return get_unknown_dirid();
    }
    len = (lstrlenW(buffer) + 1) * sizeof(WCHAR);
    if ((str = malloc( len ))) memcpy( str, buffer, len );
    return str;
}

static const WCHAR *create_printer_dirid( DWORD dirid )
{
    WCHAR buffer[MAX_PATH+32], *str;
    int len;

    switch (dirid)
    {
    case 66000:  /* printer driver */
        GetSystemDirectoryW( buffer, MAX_PATH );
        lstrcatW( buffer, L"\\spool\\drivers\\" );
        lstrcatW( buffer, printer_env );
        break;
    case 66001:  /* print processor */
        return create_system_dirid( DIRID_PRINTPROCESSOR );
    case 66002:  /* system directory (FIXME: only for native architecture) */
        GetSystemDirectoryW( buffer, MAX_PATH );
        break;
    case 66003:  /* color directory */
        return create_system_dirid( DIRID_COLOR );
    case 66004:  /* asp files directory */
    default:
        FIXME( "unsupported dirid %ld\n", dirid );
        return get_unknown_dirid();
    }
    len = (lstrlenW(buffer) + 1) * sizeof(WCHAR);
    if ((str = malloc( len ))) memcpy( str, buffer, len );
    return str;
}

static const WCHAR *get_csidl_dir( DWORD csidl )
{
    WCHAR buffer[MAX_PATH], *str;
    int len;

    if (!SHGetSpecialFolderPathW( NULL, buffer, csidl, TRUE ))
    {
        FIXME( "CSIDL %lx not found\n", csidl );
        return get_unknown_dirid();
    }
    len = (lstrlenW(buffer) + 1) * sizeof(WCHAR);
    if ((str = malloc( len ))) memcpy( str, buffer, len );
    return str;
}

/* retrieve the string corresponding to a dirid, or NULL if none */
const WCHAR *DIRID_get_string( int dirid )
{
    int i;

    if (dirid == DIRID_ABSOLUTE || dirid == DIRID_ABSOLUTE_16BIT) dirid = DIRID_NULL;

    if (dirid >= 66000 && dirid <= 66004)
    {
        if (!printer_dirids[dirid - 66000]) printer_dirids[dirid - 66000] = create_printer_dirid( dirid );
        return printer_dirids[dirid - 66000];
    }
    else if (dirid >= DIRID_USER)
    {
        for (i = 0; i < nb_user_dirids; i++)
            if (user_dirids[i].id == dirid) return user_dirids[i].str;
        WARN("user id %d not found\n", dirid );
        return NULL;
    }
    else if (dirid >= MIN_CSIDL_DIRID)
    {
        if (dirid > MAX_CSIDL_DIRID) return get_unknown_dirid();
        dirid -= MIN_CSIDL_DIRID;
        if (!csidl_dirids[dirid]) csidl_dirids[dirid] = get_csidl_dir( dirid );
        return csidl_dirids[dirid];
    }
    else
    {
        if (dirid > MAX_SYSTEM_DIRID) return get_unknown_dirid();
        if (!system_dirids[dirid]) system_dirids[dirid] = create_system_dirid( dirid );
        return system_dirids[dirid];
    }
}

/* store a user dirid string */
static BOOL store_user_dirid( HINF hinf, int id, WCHAR *str )
{
    int i;

    for (i = 0; i < nb_user_dirids; i++) if (user_dirids[i].id == id) break;

    if (i < nb_user_dirids) free( user_dirids[i].str );
    else
    {
        if (nb_user_dirids >= alloc_user_dirids)
        {
            int new_size = max( 32, alloc_user_dirids * 2 );
            struct user_dirid *new = realloc( user_dirids, new_size * sizeof(*new) );
            if (!new) return FALSE;
            user_dirids = new;
            alloc_user_dirids = new_size;
        }
        nb_user_dirids++;
    }
    user_dirids[i].id  = id;
    user_dirids[i].str = str;
    TRACE("id %d -> %s\n", id, debugstr_w(str) );
    return TRUE;
}


/***********************************************************************
 *		SetupSetDirectoryIdA    (SETUPAPI.@)
 */
BOOL WINAPI SetupSetDirectoryIdA( HINF hinf, DWORD id, PCSTR dir )
{
    UNICODE_STRING dirW;
    int i;

    if (!id)  /* clear everything */
    {
        for (i = 0; i < nb_user_dirids; i++) free( user_dirids[i].str );
        nb_user_dirids = 0;
        return TRUE;
    }
    if (id < DIRID_USER)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    /* duplicate the string */
    if (!RtlCreateUnicodeStringFromAsciiz( &dirW, dir ))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }
    return store_user_dirid( hinf, id, dirW.Buffer );
}


/***********************************************************************
 *		SetupSetDirectoryIdW    (SETUPAPI.@)
 */
BOOL WINAPI SetupSetDirectoryIdW( HINF hinf, DWORD id, PCWSTR dir )
{
    int i, len;
    WCHAR *str;

    if (!id)  /* clear everything */
    {
        for (i = 0; i < nb_user_dirids; i++) free( user_dirids[i].str );
        nb_user_dirids = 0;
        return TRUE;
    }
    if (id < DIRID_USER)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    /* duplicate the string */
    len = (lstrlenW(dir)+1) * sizeof(WCHAR);
    if (!(str = malloc( len ))) return FALSE;
    memcpy( str, dir, len );
    return store_user_dirid( hinf, id, str );
}
