/*
 * Native Font control
 *
 * Copyright 1998, 1999 Eric Kohl
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
 * NOTES
 *   This is just a dummy control. An author is needed! Any volunteers?
 *   I will only improve this control once in a while.
 *     Eric <ekohl@abo.rhein-zeitung.de>
 *
 * TODO:
 *   - All messages.
 *   - All notifications.
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "commctrl.h"
#include "comctl32.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(nativefont);

typedef struct
{
    HWND     hwndSelf;        /* my own handle */
} NATIVEFONT_INFO;

#define NATIVEFONT_GetInfoPtr(hwnd) ((NATIVEFONT_INFO *)GetWindowLongPtrW (hwnd, 0))

static LRESULT
NATIVEFONT_Create (HWND hwnd)
{
    NATIVEFONT_INFO *infoPtr;

    /* allocate memory for info structure */
    infoPtr = Alloc (sizeof(NATIVEFONT_INFO));
    SetWindowLongPtrW (hwnd, 0, (DWORD_PTR)infoPtr);

    /* initialize info structure */
    infoPtr->hwndSelf = hwnd;

    return 0;
}

static LRESULT
NATIVEFONT_Destroy (NATIVEFONT_INFO *infoPtr)
{
    /* free control info data */
    SetWindowLongPtrW( infoPtr->hwndSelf, 0, 0 );
    Free (infoPtr);

    return 0;
}

static LRESULT WINAPI
NATIVEFONT_WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    NATIVEFONT_INFO *infoPtr = NATIVEFONT_GetInfoPtr(hwnd);

    TRACE("hwnd %p, msg %04x, wparam %Ix, lparam %Ix\n", hwnd, uMsg, wParam, lParam);

    if (!infoPtr && (uMsg != WM_CREATE))
	return DefWindowProcW( hwnd, uMsg, wParam, lParam );

    switch (uMsg)
    {
	case WM_CREATE:
	    return NATIVEFONT_Create (hwnd);

	case WM_DESTROY:
	    return NATIVEFONT_Destroy (infoPtr);

        case WM_MOVE:
        case WM_SIZE:
        case WM_SHOWWINDOW:
        case WM_WINDOWPOSCHANGING:
        case WM_WINDOWPOSCHANGED:
        case WM_SETFONT:
        case WM_GETDLGCODE:
	    /* FIXME("message %04x seen but stubbed\n", uMsg); */
	    return DefWindowProcW (hwnd, uMsg, wParam, lParam);

	default:
	    if ((uMsg >= WM_USER) && (uMsg < WM_APP) && !COMCTL32_IsReflectedMessage(uMsg))
		ERR("unknown msg %04x, wp %Ix, lp %Ix\n", uMsg, wParam, lParam);
	    return DefWindowProcW (hwnd, uMsg, wParam, lParam);
    }
}


VOID
NATIVEFONT_Register (void)
{
    WNDCLASSW wndClass;

    ZeroMemory (&wndClass, sizeof(WNDCLASSW));
    wndClass.style         = CS_GLOBALCLASS;
    wndClass.lpfnWndProc   = NATIVEFONT_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(NATIVEFONT_INFO *);
    wndClass.hCursor       = LoadCursorW (0, (LPWSTR)IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wndClass.lpszClassName = WC_NATIVEFONTCTLW;

    RegisterClassW (&wndClass);
}


VOID
NATIVEFONT_Unregister (void)
{
    UnregisterClassW (WC_NATIVEFONTCTLW, NULL);
}
