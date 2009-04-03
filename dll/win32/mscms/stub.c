/*
 * MSCMS - Color Management System for Wine
 *
 * Copyright 2004, 2005 Hans Leidekker
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

#include "config.h"
#include "wine/debug.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "icm.h"

#include "mscms_priv.h"

WINE_DEFAULT_DEBUG_CHANNEL(mscms);

BOOL WINAPI CheckBitmapBits( HTRANSFORM transform, PVOID srcbits, BMFORMAT format, DWORD width,
                             DWORD height, DWORD stride, PBYTE result, PBMCALLBACKFN callback,
                             LPARAM data )
{
    FIXME( "( %p, %p, 0x%08x, 0x%08x, 0x%08x, 0x%08x, %p, %p, 0x%08lx ) stub\n",
           transform, srcbits, format, width, height, stride, result, callback, data );

    return FALSE;
}

BOOL WINAPI CheckColors( HTRANSFORM transform, PCOLOR colors, DWORD number, COLORTYPE type,
                         PBYTE result )
{
    FIXME( "( %p, %p, 0x%08x, 0x%08x, %p ) stub\n", transform, colors, number, type, result );

    return FALSE;
}

BOOL WINAPI ConvertColorNameToIndex( HPROFILE profile, PCOLOR_NAME name, PDWORD index, DWORD count )
{
    FIXME( "( %p, %p, %p, 0x%08x ) stub\n", profile, name, index, count );

    return FALSE;
}

BOOL WINAPI ConvertIndexToColorName( HPROFILE profile, PDWORD index, PCOLOR_NAME name, DWORD count )
{
    FIXME( "( %p, %p, %p, 0x%08x ) stub\n", profile, index, name, count );

    return FALSE;
}

BOOL WINAPI CreateDeviceLinkProfile( PHPROFILE profiles, DWORD nprofiles, PDWORD intents,
                                     DWORD nintents, DWORD flags, PBYTE *data, DWORD index )
{
    FIXME( "( %p, 0x%08x, %p, 0x%08x, 0x%08x, %p, 0x%08x ) stub\n",
           profiles, nprofiles, intents, nintents, flags, data, index );

    return FALSE;
}

BOOL WINAPI CreateProfileFromLogColorSpaceA( LPLOGCOLORSPACEA space, PBYTE *buffer )
{
    FIXME( "( %p, %p ) stub\n", space, buffer );

    return FALSE;
}

BOOL WINAPI CreateProfileFromLogColorSpaceW( LPLOGCOLORSPACEW space, PBYTE *buffer )
{
    FIXME( "( %p, %p ) stub\n", space, buffer );

    return FALSE;
}

DWORD WINAPI GenerateCopyFilePaths( LPCWSTR printer, LPCWSTR directory, LPBYTE clientinfo,
                                    DWORD level, LPWSTR sourcedir, LPDWORD sourcedirsize,
                                    LPWSTR targetdir, LPDWORD targetdirsize, DWORD flags )
{
    FIXME( "( %s, %s, %p, 0x%08x, %p, %p, %p, %p, 0x%08x ) stub\n",
           debugstr_w(printer), debugstr_w(directory), clientinfo, level, sourcedir,
           sourcedirsize, targetdir, targetdirsize, flags );
    return ERROR_SUCCESS;
}

DWORD WINAPI GetCMMInfo( HTRANSFORM transform, DWORD info )
{
    FIXME( "( %p, 0x%08x ) stub\n", transform, info );

    return 0;
}

BOOL WINAPI GetNamedProfileInfo( HPROFILE profile, PNAMED_PROFILE_INFO info )
{
    FIXME( "( %p, %p ) stub\n", profile, info );

    return FALSE;
}

BOOL WINAPI GetPS2ColorRenderingDictionary( HPROFILE profile, DWORD intent, PBYTE buffer,
                                            PDWORD size, PBOOL binary )
{
    FIXME( "( %p, 0x%08x, %p, %p, %p ) stub\n", profile, intent, buffer, size, binary );

    return FALSE;
}

BOOL WINAPI GetPS2ColorRenderingIntent( HPROFILE profile, DWORD intent, PBYTE buffer, PDWORD size )
{
    FIXME( "( %p, 0x%08x, %p, %p ) stub\n", profile, intent, buffer, size );

    return FALSE;
}

BOOL WINAPI GetPS2ColorSpaceArray( HPROFILE profile, DWORD intent, DWORD type, PBYTE buffer,
                                   PDWORD size, PBOOL binary )
{
    FIXME( "( %p, 0x%08x, 0x%08x, %p, %p, %p ) stub\n", profile, intent, type, buffer, size, binary );

    return FALSE;
}

BOOL WINAPI RegisterCMMA( PCSTR machine, DWORD id, PCSTR dll )
{
    FIXME( "( %p, 0x%08x, %p ) stub\n", machine, id, dll );

    return TRUE;
}

BOOL WINAPI RegisterCMMW( PCWSTR machine, DWORD id, PCWSTR dll )
{
    FIXME( "( %p, 0x%08x, %p ) stub\n", machine, id, dll );

    return TRUE;
}

BOOL WINAPI SelectCMM( DWORD id )
{
    FIXME( "(%x) stub\n", id );

    return TRUE;
}

BOOL WINAPI SetColorProfileElementReference( HPROFILE profile, TAGTYPE type, TAGTYPE ref )
{
    FIXME( "( %p, 0x%08x, 0x%08x ) stub\n", profile, type, ref );

    return TRUE;
}

BOOL WINAPI SetColorProfileElementSize( HPROFILE profile, TAGTYPE type, DWORD size )
{
    FIXME( "( %p, 0x%08x, 0x%08x ) stub\n", profile, type, size );

    return FALSE;
}

BOOL WINAPI SetStandardColorSpaceProfileA( PCSTR machine, DWORD id, PSTR profile )
{
    FIXME( "( 0x%08x, %p ) stub\n", id, profile );
    return TRUE;
}

BOOL WINAPI SetStandardColorSpaceProfileW( PCWSTR machine, DWORD id, PWSTR profile )
{
    FIXME( "( 0x%08x, %p ) stub\n", id, profile );
    return TRUE;
}

BOOL WINAPI SpoolerCopyFileEvent( LPWSTR printer, LPWSTR key, DWORD event )
{
    FIXME( "( %s, %s, 0x%08x ) stub\n", debugstr_w(printer), debugstr_w(key), event );
    return TRUE;
}

BOOL WINAPI UnregisterCMMA( PCSTR machine, DWORD id )
{
    FIXME( "( %p, 0x%08x ) stub\n", machine, id );

    return TRUE;
}

BOOL WINAPI UnregisterCMMW( PCWSTR machine, DWORD id )
{
    FIXME( "( %p, 0x%08x ) stub\n", machine, id );

    return TRUE;
}
