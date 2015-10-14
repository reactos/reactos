/*
 * USER resource functions
 *
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1995, 2009 Alexandre Julliard
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wine/winternl.h"
#include "winnls.h"
#include "wine/debug.h"
#include "user_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(resource);
WINE_DECLARE_DEBUG_CHANNEL(accel);

/* this is the 8 byte accel struct used in Win32 resources (internal only) */
typedef struct
{
    WORD   fVirt;
    WORD   key;
    WORD   cmd;
    WORD   pad;
} PE_ACCEL;

/* the accelerator user object */
struct accelerator
{
    struct user_object obj;
    unsigned int       count;
    PE_ACCEL           table[1];
};

/**********************************************************************
 *			LoadAcceleratorsW	(USER32.@)
 */
HACCEL WINAPI LoadAcceleratorsW(HINSTANCE instance, LPCWSTR name)
{
    struct accelerator *accel;
    const PE_ACCEL *table;
    HRSRC rsrc;
    HACCEL handle;
    DWORD count;

    if (!(rsrc = FindResourceW( instance, name, (LPWSTR)RT_ACCELERATOR ))) return 0;
    table = LoadResource( instance, rsrc );
    count = SizeofResource( instance, rsrc ) / sizeof(*table);
    if (!count) return 0;
    accel = HeapAlloc( GetProcessHeap(), 0, FIELD_OFFSET( struct accelerator, table[count] ));
    if (!accel) return 0;
    accel->count = count;
    memcpy( accel->table, table, count * sizeof(*table) );
    if (!(handle = alloc_user_handle( &accel->obj, USER_ACCEL )))
        HeapFree( GetProcessHeap(), 0, accel );
    TRACE_(accel)("%p %s returning %p\n", instance, debugstr_w(name), handle );
    return handle;
}

/***********************************************************************
 *		LoadAcceleratorsA   (USER32.@)
 */
HACCEL WINAPI LoadAcceleratorsA(HINSTANCE instance,LPCSTR lpTableName)
{
    INT len;
    LPWSTR uni;
    HACCEL result = 0;

    if (IS_INTRESOURCE(lpTableName)) return LoadAcceleratorsW( instance, (LPCWSTR)lpTableName );

    len = MultiByteToWideChar( CP_ACP, 0, lpTableName, -1, NULL, 0 );
    if ((uni = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
    {
        MultiByteToWideChar( CP_ACP, 0, lpTableName, -1, uni, len );
        result = LoadAcceleratorsW(instance,uni);
        HeapFree( GetProcessHeap(), 0, uni);
    }
    return result;
}

/**********************************************************************
 *             CopyAcceleratorTableA   (USER32.@)
 */
INT WINAPI CopyAcceleratorTableA(HACCEL src, LPACCEL dst, INT count)
{
    char ch;
    int i, ret = CopyAcceleratorTableW( src, dst, count );

    if (ret && dst)
    {
        for (i = 0; i < ret; i++)
        {
            if (dst[i].fVirt & FVIRTKEY) continue;
            WideCharToMultiByte( CP_ACP, 0, &dst[i].key, 1, &ch, 1, NULL, NULL );
            dst[i].key = ch;
        }
    }
    return ret;
}

/**********************************************************************
 *             CopyAcceleratorTableW   (USER32.@)
 */
INT WINAPI CopyAcceleratorTableW(HACCEL src, LPACCEL dst, INT count)
{
    struct accelerator *accel;
    int i;

    if (!(accel = get_user_handle_ptr( src, USER_ACCEL ))) return 0;
    if (accel == OBJ_OTHER_PROCESS)
    {
        FIXME( "other process handle %p?\n", src );
        return 0;
    }
    if (dst)
    {
        if (count > accel->count) count = accel->count;
        for (i = 0; i < count; i++)
        {
            dst[i].fVirt = accel->table[i].fVirt & 0x7f;
            dst[i].key   = accel->table[i].key;
            dst[i].cmd   = accel->table[i].cmd;
        }
    }
    else count = accel->count;
    release_user_handle_ptr( accel );
    return count;
}

/*********************************************************************
 *                    CreateAcceleratorTableA   (USER32.@)
 */
HACCEL WINAPI CreateAcceleratorTableA(LPACCEL lpaccel, INT count)
{
    struct accelerator *accel;
    HACCEL handle;
    int i;

    if (count < 1)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    accel = HeapAlloc( GetProcessHeap(), 0, FIELD_OFFSET( struct accelerator, table[count] ));
    if (!accel) return 0;
    accel->count = count;
    for (i = 0; i < count; i++)
    {
        accel->table[i].fVirt = lpaccel[i].fVirt;
        accel->table[i].cmd   = lpaccel[i].cmd;
        if (!(lpaccel[i].fVirt & FVIRTKEY))
        {
            char ch = lpaccel[i].key;
            MultiByteToWideChar( CP_ACP, 0, &ch, 1, &accel->table[i].key, 1 );
        }
        else accel->table[i].key = lpaccel[i].key;
    }
    if (!(handle = alloc_user_handle( &accel->obj, USER_ACCEL )))
        HeapFree( GetProcessHeap(), 0, accel );
    TRACE_(accel)("returning %p\n", handle );
    return handle;
}

/*********************************************************************
 *                    CreateAcceleratorTableW   (USER32.@)
 */
HACCEL WINAPI CreateAcceleratorTableW(LPACCEL lpaccel, INT count)
{
    struct accelerator *accel;
    HACCEL handle;
    int i;

    if (count < 1)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    accel = HeapAlloc( GetProcessHeap(), 0, FIELD_OFFSET( struct accelerator, table[count] ));
    if (!accel) return 0;
    accel->count = count;
    for (i = 0; i < count; i++)
    {
        accel->table[i].fVirt = lpaccel[i].fVirt;
        accel->table[i].key   = lpaccel[i].key;
        accel->table[i].cmd   = lpaccel[i].cmd;
    }
    if (!(handle = alloc_user_handle( &accel->obj, USER_ACCEL )))
        HeapFree( GetProcessHeap(), 0, accel );
    TRACE_(accel)("returning %p\n", handle );
    return handle;
}

/******************************************************************************
 * DestroyAcceleratorTable [USER32.@]
 * Destroys an accelerator table
 *
 * PARAMS
 *    handle [I] Handle to accelerator table
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI DestroyAcceleratorTable( HACCEL handle )
{
    struct accelerator *accel;

    if (!(accel = free_user_handle( handle, USER_ACCEL ))) return FALSE;
    if (accel == OBJ_OTHER_PROCESS)
    {
        FIXME( "other process handle %p?\n", accel );
        return FALSE;
    }
    return HeapFree( GetProcessHeap(), 0, accel );
}

/**********************************************************************
 *	LoadStringW		(USER32.@)
 */
INT WINAPI LoadStringW( HINSTANCE instance, UINT resource_id,
                            LPWSTR buffer, INT buflen )
{
    HGLOBAL hmem;
    HRSRC hrsrc;
    WCHAR *p;
    int string_num;
    int i;

    TRACE("instance = %p, id = %04x, buffer = %p, length = %d\n",
          instance, resource_id, buffer, buflen);

    if(buffer == NULL)
        return 0;

    /* Use loword (incremented by 1) as resourceid */
    hrsrc = FindResourceW( instance, MAKEINTRESOURCEW((LOWORD(resource_id) >> 4) + 1),
                           (LPWSTR)RT_STRING );
    if (!hrsrc) return 0;
    hmem = LoadResource( instance, hrsrc );
    if (!hmem) return 0;

    p = LockResource(hmem);
    string_num = resource_id & 0x000f;
    for (i = 0; i < string_num; i++)
	p += *p + 1;

    TRACE("strlen = %d\n", (int)*p );

    /*if buflen == 0, then return a read-only pointer to the resource itself in buffer
    it is assumed that buffer is actually a (LPWSTR *) */
    if(buflen == 0)
    {
        *((LPWSTR *)buffer) = p + 1;
        return *p;
    }

    i = min(buflen - 1, *p);
    if (i > 0) {
	memcpy(buffer, p + 1, i * sizeof (WCHAR));
        buffer[i] = 0;
    } else {
	if (buflen > 1) {
            buffer[0] = 0;
	    return 0;
	}
    }

    TRACE("%s loaded !\n", debugstr_w(buffer));
    return i;
}

/**********************************************************************
 *	LoadStringA	(USER32.@)
 */
INT WINAPI LoadStringA( HINSTANCE instance, UINT resource_id, LPSTR buffer, INT buflen )
{
    HGLOBAL hmem;
    HRSRC hrsrc;
    DWORD retval = 0;

    TRACE("instance = %p, id = %04x, buffer = %p, length = %d\n",
          instance, resource_id, buffer, buflen);

    if (!buflen) return -1;

    /* Use loword (incremented by 1) as resourceid */
    if ((hrsrc = FindResourceW( instance, MAKEINTRESOURCEW((LOWORD(resource_id) >> 4) + 1),
                                (LPWSTR)RT_STRING )) &&
        (hmem = LoadResource( instance, hrsrc )))
    {
        const WCHAR *p = LockResource(hmem);
        unsigned int id = resource_id & 0x000f;

        while (id--) p += *p + 1;

        RtlUnicodeToMultiByteN( buffer, buflen - 1, &retval, p + 1, *p * sizeof(WCHAR) );
    }
    buffer[retval] = 0;
    TRACE("returning %s\n", debugstr_a(buffer));
    return retval;
}

/**********************************************************************
 *	GetGuiResources	(USER32.@)
 */
DWORD WINAPI GetGuiResources( HANDLE hProcess, DWORD uiFlags )
{
    static BOOL warn = TRUE;

    if (warn) {
        FIXME("(%p,%x): stub\n",hProcess,uiFlags);
       warn = FALSE;
    }

    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return 0;
}
