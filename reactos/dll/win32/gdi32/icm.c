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


/***********************************************************************
 *           EnumICMProfilesA    (GDI32.@)
 */
INT WINAPI EnumICMProfilesA(HDC hdc, ICMENUMPROCA func, LPARAM lparam)
{
	FIXME("%p, %p, 0x%08lx stub\n", hdc, func, lparam);
	return -1;
}

/***********************************************************************
 *           EnumICMProfilesW    (GDI32.@)
 */
INT WINAPI EnumICMProfilesW(HDC hdc, ICMENUMPROCW func, LPARAM lparam)
{
	FIXME("%p, %p, 0x%08lx stub\n", hdc, func, lparam);
	return -1;
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
    return TRUE;
}

/**********************************************************************
 *           SetICMProfileW         (GDI32.@)
 */
BOOL WINAPI SetICMProfileW(HDC hdc, LPWSTR filename)
{
    FIXME("%p %s stub\n", hdc, debugstr_w(filename));
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
