/*
 * SetupX .inf file parsing functions
 *
 * Copyright 2000 Andreas Mohr for CodeWeavers
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * FIXME:
 * - return values ???
 * - this should be reimplemented at some point to have its own
 *   file parsing instead of using profile functions,
 *   as some SETUPX exports probably demand that
 *   (IpSaveRestorePosition, IpFindNextMatchLine, ...).
 */

#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winternl.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "setupapi.h"
#include "setupx16.h"
#include "setupapi_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(setupapi);

#define MAX_HANDLES 16384
#define FIRST_HANDLE 32

static HINF handles[MAX_HANDLES];


static RETERR16 alloc_hinf16( HINF hinf, HINF16 *hinf16 )
{
    int i;
    for (i = 0; i < MAX_HANDLES; i++)
    {
        if (!handles[i])
        {
            handles[i] = hinf;
            *hinf16 = i + FIRST_HANDLE;
            return OK;
        }
    }
    return ERR_IP_OUT_OF_HANDLES;
}

static HINF get_hinf( HINF16 hinf16 )
{
    int idx = hinf16 - FIRST_HANDLE;
    if (idx < 0 || idx >= MAX_HANDLES) return 0;
    return handles[idx];
}


static HINF free_hinf16( HINF16 hinf16 )
{
    HINF ret;
    int idx = hinf16 - FIRST_HANDLE;

    if (idx < 0 || idx >= MAX_HANDLES) return 0;
    ret = handles[idx];
    handles[idx] = 0;
    return ret;
}

/* convert last error code to a RETERR16 value */
static RETERR16 get_last_error(void)
{
    switch(GetLastError())
    {
    case ERROR_EXPECTED_SECTION_NAME:
    case ERROR_BAD_SECTION_NAME_LINE:
    case ERROR_SECTION_NAME_TOO_LONG: return ERR_IP_INVALID_SECT_NAME;
    case ERROR_SECTION_NOT_FOUND: return ERR_IP_SECT_NOT_FOUND;
    case ERROR_LINE_NOT_FOUND: return ERR_IP_LINE_NOT_FOUND;
    default: return IP_ERROR;  /* FIXME */
    }
}


/***********************************************************************
 *		IpOpen (SETUPX.2)
 *
 */
RETERR16 WINAPI IpOpen16( LPCSTR filename, HINF16 *hinf16 )
{
    HINF hinf = SetupOpenInfFileA( filename, NULL, INF_STYLE_WIN4, NULL );
    if (hinf == (HINF)INVALID_HANDLE_VALUE) return get_last_error();
    return alloc_hinf16( hinf, hinf16 );
}


/***********************************************************************
 *		IpClose (SETUPX.4)
 */
RETERR16 WINAPI IpClose16( HINF16 hinf16 )
{
    HINF hinf = free_hinf16( hinf16 );
    if (!hinf) return ERR_IP_INVALID_HINF;
    SetupCloseInfFile( hinf );
    return OK;
}


/***********************************************************************
 *		IpGetProfileString (SETUPX.210)
 */
RETERR16 WINAPI IpGetProfileString16( HINF16 hinf16, LPCSTR section, LPCSTR entry,
                                      LPSTR buffer, WORD buflen )
{
    DWORD required_size;
    HINF hinf = get_hinf( hinf16 );

    if (!hinf) return ERR_IP_INVALID_HINF;
    if (!SetupGetLineTextA( NULL, hinf, section, entry, buffer, buflen, &required_size ))
        return get_last_error();
    TRACE("%p: section %s entry %s ret %s\n",
          hinf, debugstr_a(section), debugstr_a(entry), debugstr_a(buffer) );
    return OK;
}


/***********************************************************************
 *		GenFormStrWithoutPlaceHolders (SETUPX.103)
 *
 * ought to be pretty much implemented, I guess...
 */
void WINAPI GenFormStrWithoutPlaceHolders16( LPSTR dst, LPCSTR src, HINF16 hinf16 )
{
    UNICODE_STRING srcW;
    HINF hinf = get_hinf( hinf16 );

    if (!hinf) return;

    if (!RtlCreateUnicodeStringFromAsciiz( &srcW, src )) return;
    PARSER_string_substA( hinf, srcW.Buffer, dst, MAX_INF_STRING_LENGTH );
    RtlFreeUnicodeString( &srcW );
    TRACE( "%s -> %s\n", debugstr_a(src), debugstr_a(dst) );
}

/***********************************************************************
 *		GenInstall (SETUPX.101)
 *
 * generic installer function for .INF file sections
 *
 * This is not perfect - patch whenever you can !
 *
 * wFlags == GENINSTALL_DO_xxx
 * e.g. NetMeeting:
 * first call GENINSTALL_DO_REGSRCPATH | GENINSTALL_DO_FILES,
 * second call GENINSTALL_DO_LOGCONFIG | CFGAUTO | INI2REG | REG | INI
 */
RETERR16 WINAPI GenInstall16( HINF16 hinf16, LPCSTR section, WORD genflags )
{
    UINT flags = 0;
    HINF hinf = get_hinf( hinf16 );
    RETERR16 ret = OK;
    void *context;

    if (!hinf) return ERR_IP_INVALID_HINF;

    if (genflags & GENINSTALL_DO_FILES) flags |= SPINST_FILES;
    if (genflags & GENINSTALL_DO_INI) flags |= SPINST_INIFILES;
    if (genflags & GENINSTALL_DO_REG) flags |= SPINST_REGISTRY;
    if (genflags & GENINSTALL_DO_INI2REG) flags |= SPINST_INI2REG;
    if (genflags & GENINSTALL_DO_LOGCONFIG) flags |= SPINST_LOGCONFIG;
    if (genflags & GENINSTALL_DO_REGSRCPATH) FIXME( "unsupported flag: GENINSTALL_DO_REGSRCPATH\n" );
    if (genflags & GENINSTALL_DO_CFGAUTO) FIXME( "unsupported flag: GENINSTALL_DO_CFGAUTO\n" );
    if (genflags & GENINSTALL_DO_PERUSER) FIXME( "unsupported flag: GENINSTALL_DO_PERUSER\n" );

    context = SetupInitDefaultQueueCallback( 0 );
    if (!SetupInstallFromInfSectionA( 0, hinf, section, flags, 0, NULL,
                                      SP_COPY_NEWER_OR_SAME, SetupDefaultQueueCallbackA,
                                      context, 0, 0 ))
        ret = get_last_error();

    SetupTermDefaultQueueCallback( context );
    return ret;
}
