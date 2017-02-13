/*
 * Theming - List box control
 *
 * Copyright (c) 2005 by Frank Richter
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

#include "comctl32.h"

/* Draw themed border */
static void nc_paint (HTHEME theme, HWND hwnd, HRGN region)
{
    HRGN cliprgn = region;
    DWORD exStyle = GetWindowLongW (hwnd, GWL_EXSTYLE);
    if (exStyle & WS_EX_CLIENTEDGE)
    {
        HDC dc;
        RECT r;
        int cxEdge = GetSystemMetrics (SM_CXEDGE),
            cyEdge = GetSystemMetrics (SM_CYEDGE);
    
        GetWindowRect(hwnd, &r);
    
        /* New clipping region passed to default proc to exclude border */
        cliprgn = CreateRectRgn (r.left + cxEdge, r.top + cyEdge,
            r.right - cxEdge, r.bottom - cyEdge);
        if (region != (HRGN)1)
            CombineRgn (cliprgn, cliprgn, region, RGN_AND);
        OffsetRect(&r, -r.left, -r.top);
    
        dc = GetWindowDC(hwnd);
        /* Exclude client part */
        ExcludeClipRect(dc, r.left + cxEdge, r.top + cyEdge,
            r.right - cxEdge, r.bottom -cyEdge);

        if (IsThemeBackgroundPartiallyTransparent (theme, 0, 0))
            DrawThemeParentBackground(hwnd, dc, &r);
        DrawThemeBackground (theme, dc, 0, 0, &r, 0);
        ReleaseDC(hwnd, dc);
    }

    /* Call default proc to get the scrollbars etc. painted */
    DefWindowProcW (hwnd, WM_NCPAINT, (WPARAM)cliprgn, 0);
}

/**********************************************************************
 * The list control subclass window proc.
 */
LRESULT CALLBACK THEMING_ListBoxSubclassProc (HWND hwnd, UINT msg, 
                                              WPARAM wParam, LPARAM lParam, 
                                              ULONG_PTR dwRefData)
{
    const WCHAR* themeClass = WC_LISTBOXW;
    HTHEME theme;
    LRESULT result;
      
    switch (msg)
    {
    case WM_CREATE:
        result = THEMING_CallOriginalClass  (hwnd, msg, wParam, lParam);
        OpenThemeData( hwnd, themeClass );
        return result;
    
    case WM_DESTROY:
        theme = GetWindowTheme( hwnd );
        CloseThemeData ( theme );
        return THEMING_CallOriginalClass (hwnd, msg, wParam, lParam);

    case WM_THEMECHANGED:
        theme = GetWindowTheme( hwnd );
        CloseThemeData ( theme );
        OpenThemeData( hwnd, themeClass );
        break;
        
    case WM_SYSCOLORCHANGE:
        theme = GetWindowTheme( hwnd );
	if (!theme) return THEMING_CallOriginalClass (hwnd, msg, wParam, lParam);
        /* Do nothing. When themed, a WM_THEMECHANGED will be received, too,
   	 * which will do the repaint. */
        break;
        
    case WM_NCPAINT:
        theme = GetWindowTheme( hwnd );
        if (!theme) return THEMING_CallOriginalClass (hwnd, msg, wParam, lParam);
        nc_paint (theme, hwnd, (HRGN)wParam);
        break;

    default: 
	/* Call old proc */
	return THEMING_CallOriginalClass (hwnd, msg, wParam, lParam);
    }
    return 0;
}
