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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "commctrl.h"
#include "comctl32.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(nativefont);

typedef struct
{
    DWORD  dwDummy;   /* just to keep the compiler happy ;-) */
} NATIVEFONT_INFO;

#define NATIVEFONT_GetInfoPtr(hwnd) ((NATIVEFONT_INFO *)GetWindowLongPtrW (hwnd, 0))


static LRESULT
NATIVEFONT_Create (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    NATIVEFONT_INFO *infoPtr;

    /* allocate memory for info structure */
    infoPtr = (NATIVEFONT_INFO *)Alloc (sizeof(NATIVEFONT_INFO));
    SetWindowLongPtrW (hwnd, 0, (DWORD_PTR)infoPtr);


    /* initialize info structure */


    return 0;
}


static LRESULT
NATIVEFONT_Destroy (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    NATIVEFONT_INFO *infoPtr = NATIVEFONT_GetInfoPtr (hwnd);




    /* free comboex info data */
    Free (infoPtr);
    SetWindowLongPtrW( hwnd, 0, 0 );

    return 0;
}



static LRESULT WINAPI
NATIVEFONT_WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (!NATIVEFONT_GetInfoPtr(hwnd) && (uMsg != WM_CREATE))
	return DefWindowProcA( hwnd, uMsg, wParam, lParam );

    switch (uMsg)
    {

	case WM_CREATE:
	    return NATIVEFONT_Create (hwnd, wParam, lParam);

	case WM_DESTROY:
	    return NATIVEFONT_Destroy (hwnd, wParam, lParam);

        case WM_MOVE:
        case WM_SIZE:
        case WM_SHOWWINDOW:
        case WM_WINDOWPOSCHANGING:
        case WM_WINDOWPOSCHANGED:
        case WM_SETFONT:
        case WM_GETDLGCODE:
	    /* FIXME("message %04x seen but stubbed\n", uMsg); */
	    return DefWindowProcA (hwnd, uMsg, wParam, lParam);

	default:
	    ERR("unknown msg %04x wp=%08x lp=%08lx\n",
		     uMsg, wParam, lParam);
	    return DefWindowProcA (hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


VOID
NATIVEFONT_Register (void)
{
    WNDCLASSA wndClass;

    ZeroMemory (&wndClass, sizeof(WNDCLASSA));
    wndClass.style         = CS_GLOBALCLASS;
    wndClass.lpfnWndProc   = (WNDPROC)NATIVEFONT_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(NATIVEFONT_INFO *);
    wndClass.hCursor       = LoadCursorA (0, (LPSTR)IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wndClass.lpszClassName = WC_NATIVEFONTCTLA;

    RegisterClassA (&wndClass);
}


VOID
NATIVEFONT_Unregister (void)
{
    UnregisterClassA (WC_NATIVEFONTCTLA, NULL);
}
