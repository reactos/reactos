/* Copyright 1993 Erik Bos
 * Copyright 1996, 2004 Alexandre Julliard
 * Copyright 2003 Eric Pouech
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
 *
 */

#include <windef.h>
#include <winbase.h>
#include <winnls.h>

#include <wine/winternl.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(path);

static inline BOOL set_ntstatus( NTSTATUS status )
{
    if (status) SetLastError( RtlNtStatusToDosError( status ));
    return !status;
}

/***********************************************************************
 *           wine_get_dos_file_name (KERNEL32.@) Not a Windows API
 *
 * Return the full DOS file name for a given Unix path.
 * Returned buffer must be freed by caller.
 */
WCHAR * CDECL wine_get_dos_file_name( LPCSTR str )
{
    UNICODE_STRING nt_name;
    NTSTATUS status;
    WCHAR *buffer;
    SIZE_T len = strlen(str) + 1;

    if (str[0] != '/')  /* relative path name */
    {
        if (!(buffer = RtlAllocateHeap( GetProcessHeap(), 0, len * sizeof(WCHAR) ))) return NULL;
        MultiByteToWideChar( CP_UNIXCP, 0, str, len, buffer, len );
        status = RtlDosPathNameToNtPathName_U_WithStatus( buffer, &nt_name, NULL, NULL );
        RtlFreeHeap( GetProcessHeap(), 0, buffer );
        if (!set_ntstatus( status )) return NULL;
        buffer = nt_name.Buffer;
        len = nt_name.Length / sizeof(WCHAR) + 1;
    }
    else
    {
#ifdef __REACTOS__
        ERR("Got absolute UNIX path name in function wine_get_dos_file_name. This is not UNIX. Please fix the caller!\n");
        ERR("File name: %s\n", str);
#else
        len += 8;  /* \??\unix prefix */
        if (!(buffer = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) ))) return NULL;
        if (!set_ntstatus( wine_unix_to_nt_file_name( str, buffer, &len )))
        {
            HeapFree( GetProcessHeap(), 0, buffer );
            return NULL;
        }
#endif
    }
    if (buffer[5] == ':')
    {
        /* get rid of the \??\ prefix */
        /* FIXME: should implement RtlNtPathNameToDosPathName and use that instead */
        memmove( buffer, buffer + 4, (len - 4) * sizeof(WCHAR) );
    }
    else buffer[1] = '\\';
    return buffer;
}
