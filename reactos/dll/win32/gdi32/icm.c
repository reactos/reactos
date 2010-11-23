/*
 * Image Color Management
 *
 * Copyright 2004 Marcus Meissner
 * Copyright 2008 Hans Leidekker
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

#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winnls.h"
#include "winreg.h"

#include "gdi_private.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(icm);


struct enum_profiles
{
    BOOL unicode;
    union
    {
        ICMENUMPROCA funcA;
        ICMENUMPROCW funcW;
    } callback;
    LPARAM data;
};

INT CALLBACK enum_profiles_callback( LPWSTR filename, LPARAM lparam )
{
    int len, ret = -1;
    struct enum_profiles *ep = (struct enum_profiles *)lparam;
    char *filenameA;

    if (ep->unicode)
        return ep->callback.funcW( filename, ep->data );

    len = WideCharToMultiByte( CP_ACP, 0, filename, -1, NULL, 0, NULL, NULL );
    filenameA = HeapAlloc( GetProcessHeap(), 0, len );
    if (filenameA)
    {
        WideCharToMultiByte( CP_ACP, 0, filename, -1, filenameA, len, NULL, NULL );
        ret = ep->callback.funcA( filenameA, ep->data );
        HeapFree( GetProcessHeap(), 0, filenameA );
    }
    return ret;
}

/***********************************************************************
 *           EnumICMProfilesA    (GDI32.@)
 */
INT WINAPI EnumICMProfilesA(HDC hdc, ICMENUMPROCA func, LPARAM lparam)
{
    DC *dc;
    INT ret = -1;

    TRACE("%p, %p, 0x%08lx\n", hdc, func, lparam);

    if (!func) return -1;
    if ((dc = get_dc_ptr(hdc)))
    {
        if (dc->funcs->pEnumICMProfiles)
        {
            struct enum_profiles ep;

            ep.unicode        = FALSE;
            ep.callback.funcA = func;
            ep.data           = lparam;
            ret = dc->funcs->pEnumICMProfiles(dc->physDev, enum_profiles_callback, (LPARAM)&ep);
        }
        release_dc_ptr(dc);
    }
    return ret;
}

/***********************************************************************
 *           EnumICMProfilesW    (GDI32.@)
 */
INT WINAPI EnumICMProfilesW(HDC hdc, ICMENUMPROCW func, LPARAM lparam)
{
    DC *dc;
    INT ret = -1;

    TRACE("%p, %p, 0x%08lx\n", hdc, func, lparam);

    if (!func) return -1;
    if ((dc = get_dc_ptr(hdc)))
    {
        if (dc->funcs->pEnumICMProfiles)
        {
            struct enum_profiles ep;

            ep.unicode        = TRUE;
            ep.callback.funcW = func;
            ep.data           = lparam;
            ret = dc->funcs->pEnumICMProfiles(dc->physDev, enum_profiles_callback, (LPARAM)&ep);
        }
        release_dc_ptr(dc);
    }
    return ret;
}

/**********************************************************************
 *           GetICMProfileA   (GDI32.@)
 *
 * Returns the filename of the specified device context's color
 * management profile, even if color management is not enabled
 * for that DC.
 *
 * RETURNS
 *    TRUE if filename is copied successfully.
 *    FALSE if the buffer length pointed to by size is too small.
 *
 * FIXME
 *    How does Windows assign these? Some registry key?
 */
BOOL WINAPI GetICMProfileA(HDC hdc, LPDWORD size, LPSTR filename)
{
    WCHAR filenameW[MAX_PATH];
    DWORD buflen = MAX_PATH;
    BOOL ret = FALSE;

    TRACE("%p, %p, %p\n", hdc, size, filename);

    if (!hdc || !size || !filename) return FALSE;

    if (GetICMProfileW(hdc, &buflen, filenameW))
    {
        int len = WideCharToMultiByte(CP_ACP, 0, filenameW, -1, NULL, 0, NULL, NULL);
        if (*size >= len)
        {
            WideCharToMultiByte(CP_ACP, 0, filenameW, -1, filename, *size, NULL, NULL);
            ret = TRUE;
        }
        else SetLastError(ERROR_INSUFFICIENT_BUFFER);
        *size = len;
    }
    return ret;
}

/**********************************************************************
 *           GetICMProfileW     (GDI32.@)
 */
BOOL WINAPI GetICMProfileW(HDC hdc, LPDWORD size, LPWSTR filename)
{
    BOOL ret = FALSE;
    DC *dc = get_dc_ptr(hdc);

    TRACE("%p, %p, %p\n", hdc, size, filename);

    if (dc)
    {
        if (dc->funcs->pGetICMProfile)
            ret = dc->funcs->pGetICMProfile(dc->physDev, size, filename);
        release_dc_ptr(dc);
    }
    return ret;
}

/**********************************************************************
 *           GetLogColorSpaceA     (GDI32.@)
 */
BOOL WINAPI GetLogColorSpaceA(HCOLORSPACE colorspace, LPLOGCOLORSPACEA buffer, DWORD size)
{
    FIXME("%p %p 0x%08x stub\n", colorspace, buffer, size);
    return FALSE;
}

/**********************************************************************
 *           GetLogColorSpaceW      (GDI32.@)
 */
BOOL WINAPI GetLogColorSpaceW(HCOLORSPACE colorspace, LPLOGCOLORSPACEW buffer, DWORD size)
{
    FIXME("%p %p 0x%08x stub\n", colorspace, buffer, size);
    return FALSE;
}

/**********************************************************************
 *           SetICMProfileA         (GDI32.@)
 */
BOOL WINAPI SetICMProfileA(HDC hdc, LPSTR filename)
{
    FIXME("%p %s stub\n", hdc, debugstr_a(filename));

    if (!filename)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (!hdc)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    return TRUE;
}

/**********************************************************************
 *           SetICMProfileW         (GDI32.@)
 */
BOOL WINAPI SetICMProfileW(HDC hdc, LPWSTR filename)
{
    FIXME("%p %s stub\n", hdc, debugstr_w(filename));

    if (!filename)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (!hdc)
    {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    return TRUE;
}

/**********************************************************************
 *           UpdateICMRegKeyA       (GDI32.@)
 */
BOOL WINAPI UpdateICMRegKeyA(DWORD reserved, LPSTR cmid, LPSTR filename, UINT command)
{
    FIXME("0x%08x, %s, %s, 0x%08x stub\n", reserved, debugstr_a(cmid), debugstr_a(filename), command);
    return TRUE;
}

/**********************************************************************
 *           UpdateICMRegKeyW       (GDI32.@)
 */
BOOL WINAPI UpdateICMRegKeyW(DWORD reserved, LPWSTR cmid, LPWSTR filename, UINT command)
{
    FIXME("0x%08x, %s, %s, 0x%08x stub\n", reserved, debugstr_w(cmid), debugstr_w(filename), command);
    return TRUE;
}
