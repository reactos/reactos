/*
 * Theming - Dialogs
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

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "uxtheme.h"
#include "tmschema.h"
#include "comctl32.h"
#include "wine/debug.h"

/**********************************************************************
 * The dialog subclass window proc.
 */
LRESULT CALLBACK THEMING_DialogSubclassProc (HWND hWnd, UINT msg, 
                                             WPARAM wParam, LPARAM lParam, 
                                             ULONG_PTR dwRefData)
{
    HTHEME theme = GetWindowTheme ( hWnd );
    static const WCHAR themeClass[] = { 'W','i','n','d','o','w',0 };
    BOOL themingActive = IsThemeDialogTextureEnabled (hWnd);
    BOOL doTheming = themingActive && (theme != NULL);
    LRESULT result;
      
    switch (msg)
    {
    case WM_CREATE:
	result = THEMING_CallOriginalClass (hWnd, msg, wParam, lParam);
	theme = OpenThemeData( hWnd, themeClass );
	return result;
    
    case WM_DESTROY:
        CloseThemeData ( theme );
	return THEMING_CallOriginalClass (hWnd, msg, wParam, lParam);

    case WM_THEMECHANGED:
        CloseThemeData ( theme );
	OpenThemeData( hWnd, themeClass );
	InvalidateRect( hWnd, NULL, TRUE );
	return 0;
	
    case WM_SYSCOLORCHANGE:
	if (!doTheming) return THEMING_CallOriginalClass (hWnd, msg, wParam, lParam);
        /* Do nothing. When themed, a WM_THEMECHANGED will be received, too,
   	 * which will do the repaint. */
        break;
        
    case WM_ERASEBKGND:
	if (!doTheming) return THEMING_CallOriginalClass (hWnd, msg, wParam, lParam);
        {
            RECT rc;
            WNDPROC dlgp = (WNDPROC)GetWindowLongPtrW (hWnd, DWLP_DLGPROC);
            if (!CallWindowProcW(dlgp, hWnd, msg, wParam, lParam))
            {
                /* Draw background*/
                GetClientRect (hWnd, &rc);
                if (IsThemePartDefined (theme, WP_DIALOG, 0))
                    /* Although there is a theme for the WINDOW class/DIALOG part, 
                     * but I[res] haven't seen Windows using it yet... Even when
                     * dialog theming is activated, the good ol' BTNFACE 
                     * background seems to be used. */
#if 0
                    DrawThemeBackground (theme, (HDC)wParam, WP_DIALOG, 0, &rc, 
                        NULL);
#endif
                    return THEMING_CallOriginalClass (hWnd, msg, wParam, lParam);
                else 
                /* We might have gotten a TAB theme class, so check if we can 
                 * draw as a tab page. */
                if (IsThemePartDefined (theme, TABP_BODY, 0))
                    DrawThemeBackground (theme, (HDC)wParam, TABP_BODY, 0, &rc, 
                        NULL);
                else
                    return THEMING_CallOriginalClass (hWnd, msg, wParam, lParam);
            }
            return 1;
        }

    case WM_CTLCOLORSTATIC:
        if (!doTheming) return THEMING_CallOriginalClass (hWnd, msg, wParam, lParam);
        {
            WNDPROC dlgp = (WNDPROC)GetWindowLongPtrW (hWnd, DWLP_DLGPROC);
            LRESULT result = CallWindowProcW(dlgp, hWnd, msg, wParam, lParam);
            if (!result)
            {
                /* Override defaults with more suitable values when themed */
                HDC controlDC = (HDC)wParam;
                HWND controlWnd = (HWND)lParam;
                WCHAR controlClass[32];
                RECT rc;

                GetClassNameW (controlWnd, controlClass, 
                    sizeof(controlClass) / sizeof(controlClass[0]));
                if (lstrcmpiW (controlClass, WC_STATICW) == 0)
                {
                    /* Static control - draw parent background and set text to 
                     * transparent, so it looks right on tab pages. */
                    GetClientRect (controlWnd, &rc);
                    DrawThemeParentBackground (controlWnd, controlDC, &rc);
                    SetBkMode (controlDC, TRANSPARENT);

                    /* Return NULL brush since we painted the BG already */
                    return (LRESULT)GetStockObject (NULL_BRUSH);
                }
                else
                    return THEMING_CallOriginalClass (hWnd, msg, wParam, lParam);

            }
            return result;
        }

    default: 
	/* Call old proc */
	return THEMING_CallOriginalClass (hWnd, msg, wParam, lParam);
    }
    return 0;
}
