/*
 * Comctl32 Icon functions
 *
 * Copyright 2014 Michael Müller
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

#include "comctl32.h"

WINE_DEFAULT_DEBUG_CHANNEL(commctrl);

HRESULT WINAPI
LoadIconMetric (HINSTANCE hinst, PCWSTR name, INT size, HICON *icon)
{
    INT width, height;

    TRACE("(%p %s %d %p)\n", hinst, debugstr_w(name), size, icon);

    if (!icon)
        return E_INVALIDARG;

    /* windows sets it to zero in a case of failure */
    *icon = NULL;

    if (!name)
        return E_INVALIDARG;

    if (size == LIM_SMALL)
    {
        width  = GetSystemMetrics( SM_CXSMICON );
        height = GetSystemMetrics( SM_CYSMICON );
    }
    else if (size == LIM_LARGE)
    {
        width  = GetSystemMetrics( SM_CXICON );
        height = GetSystemMetrics( SM_CYICON );
    }
    else
        return E_INVALIDARG;

    *icon = LoadImageW( hinst, name, IMAGE_ICON, width, height, LR_SHARED );
    if (*icon)
        return S_OK;

    return HRESULT_FROM_WIN32(GetLastError());
}
