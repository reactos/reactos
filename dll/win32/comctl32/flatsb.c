/*
 * Flat Scrollbar control
 *
 * Copyright 1998, 1999 Eric Kohl
 * Copyright 1998 Alex Priem
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
 *
 */

#include <stdarg.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winuser.h"
#include "commctrl.h"
#include "comctl32.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(commctrl);

typedef struct
{
    DWORD dwDummy;  /* just to keep the compiler happy ;-) */
} FLATSB_INFO, *LPFLATSB_INFO;


/***********************************************************************
 *		InitializeFlatSB (COMCTL32.@)
 *
 * Initializes flat scroll bars for the specified window.
 *
 * RETURNS
 *     Success: Non-zero
 *     Failure: Zero
 *
 * NOTES
 *     Subclasses specified window so that flat scroll bars may be drawn
 *     and used.
 */
BOOL WINAPI InitializeFlatSB(HWND hwnd)
{
    TRACE("[%p]\n", hwnd);
    return TRUE;
}

/***********************************************************************
 *		UninitializeFlatSB (COMCTL32.@)
 *
 * Uninitializes flat scroll bars for the specified window.
 *
 * RETURNS
 *	E_FAIL		if one of the scroll bars is currently in use
 *	S_FALSE		if InitializeFlatSB() was never called on this hwnd
 *	S_OK		otherwise
 *
 * NOTES
 *     Removes any subclassing on the specified window so that regular
 *     scroll bars are drawn and used.
 */
HRESULT WINAPI UninitializeFlatSB(HWND hwnd)
{
    TRACE("[%p]\n", hwnd);
    return S_FALSE;
}

/***********************************************************************
 *		FlatSB_GetScrollProp (COMCTL32.@)
 *
 * Retrieves flat-scroll-bar-specific properties for the specified window.
 *
 * RETURNS
 *     nonzero if successful, or zero otherwise. If index is WSB_PROP_HSTYLE,
 *     the return is nonzero if InitializeFlatSB has been called for this window, or
 *     zero otherwise.
 */
BOOL WINAPI
FlatSB_GetScrollProp(HWND hwnd, INT propIndex, LPINT prop)
{
    TRACE("[%p] propIndex=%d\n", hwnd, propIndex);
    return FALSE;
}

/***********************************************************************
 *		FlatSB_SetScrollProp (COMCTL32.@)
 *
 * Sets flat-scroll-bar-specific properties for the specified window.
 *
 * RETURNS
 *     Success: Non-zero
 *     Failure: Zero
 */
BOOL WINAPI
FlatSB_SetScrollProp(HWND hwnd, UINT index, INT newValue, BOOL flag)
{
    TRACE("[%p] index=%u newValue=%d flag=%d\n", hwnd, index, newValue, flag);
    return FALSE;
}

/***********************************************************************
 * 	From the Microsoft docs:
 *	"If flat scroll bars haven't been initialized for the
 *	window, the flat scroll bar APIs will defer to the corresponding
 *	standard APIs.  This allows the developer to turn flat scroll
 *	bars on and off without having to write conditional code."
 *
 *	So, if we just call the standard functions until we implement
 *	the flat scroll bar functions, flat scroll bars will show up and
 *	behave properly, as though they had simply not been setup to
 *	have flat properties.
 *
 *	Susan <sfarley@codeweavers.com>
 *
 */

/***********************************************************************
 *		FlatSB_EnableScrollBar (COMCTL32.@)
 *
 * See EnableScrollBar.
 */
BOOL WINAPI
FlatSB_EnableScrollBar(HWND hwnd, int nBar, UINT flags)
{
    return EnableScrollBar(hwnd, nBar, flags);
}

/***********************************************************************
 *		FlatSB_ShowScrollBar (COMCTL32.@)
 *
 * See ShowScrollBar.
 */
BOOL WINAPI
FlatSB_ShowScrollBar(HWND hwnd, int nBar, BOOL fShow)
{
    return ShowScrollBar(hwnd, nBar, fShow);
}

/***********************************************************************
 *		FlatSB_GetScrollRange (COMCTL32.@)
 *
 * See GetScrollRange.
 */
BOOL WINAPI
FlatSB_GetScrollRange(HWND hwnd, int nBar, LPINT min, LPINT max)
{
    return GetScrollRange(hwnd, nBar, min, max);
}

/***********************************************************************
 *		FlatSB_GetScrollInfo (COMCTL32.@)
 *
 * See GetScrollInfo.
 */
BOOL WINAPI
FlatSB_GetScrollInfo(HWND hwnd, int nBar, LPSCROLLINFO info)
{
    return GetScrollInfo(hwnd, nBar, info);
}

/***********************************************************************
 *		FlatSB_GetScrollPos (COMCTL32.@)
 *
 * See GetScrollPos.
 */
INT WINAPI
FlatSB_GetScrollPos(HWND hwnd, int nBar)
{
    return GetScrollPos(hwnd, nBar);
}

/***********************************************************************
 *		FlatSB_SetScrollPos (COMCTL32.@)
 *
 * See SetScrollPos.
 */
INT WINAPI
FlatSB_SetScrollPos(HWND hwnd, int nBar, INT pos, BOOL bRedraw)
{
    return SetScrollPos(hwnd, nBar, pos, bRedraw);
}

/***********************************************************************
 *		FlatSB_SetScrollInfo (COMCTL32.@)
 *
 * See SetScrollInfo.
 */
INT WINAPI
FlatSB_SetScrollInfo(HWND hwnd, int nBar, LPSCROLLINFO info, BOOL bRedraw)
{
    return SetScrollInfo(hwnd, nBar, info, bRedraw);
}

/***********************************************************************
 *		FlatSB_SetScrollRange (COMCTL32.@)
 *
 * See SetScrollRange.
 */
INT WINAPI
FlatSB_SetScrollRange(HWND hwnd, int nBar, INT min, INT max, BOOL bRedraw)
{
    return SetScrollRange(hwnd, nBar, min, max, bRedraw);
}


static LRESULT
FlatSB_Create (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACE("[%p] wParam %Ix, lParam %Ix\n", hwnd, wParam, lParam);
    return 0;
}


static LRESULT
FlatSB_Destroy (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    TRACE("[%p] wParam %Ix, lParam %Ix\n", hwnd, wParam, lParam);
    return 0;
}


static LRESULT WINAPI
FlatSB_WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (!GetWindowLongPtrW(hwnd, 0) && (uMsg != WM_CREATE))
	return DefWindowProcW( hwnd, uMsg, wParam, lParam );

    switch (uMsg)
    {
	case WM_CREATE:
	    return FlatSB_Create (hwnd, wParam, lParam);

	case WM_DESTROY:
	    return FlatSB_Destroy (hwnd, wParam, lParam);

	default:
	    if ((uMsg >= WM_USER) && (uMsg < WM_APP) && !COMCTL32_IsReflectedMessage(uMsg))
		ERR("unknown msg %04x, wp %#Ix, lp %#Ix\n", uMsg, wParam, lParam);
	    return DefWindowProcW (hwnd, uMsg, wParam, lParam);
    }
}


VOID
FLATSB_Register (void)
{
    WNDCLASSW wndClass;

    ZeroMemory (&wndClass, sizeof(WNDCLASSW));
    wndClass.style         = CS_GLOBALCLASS;
    wndClass.lpfnWndProc   = FlatSB_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(FLATSB_INFO *);
    wndClass.hCursor       = LoadCursorW (0, (LPWSTR)IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wndClass.lpszClassName = FLATSB_CLASSW;

    RegisterClassW (&wndClass);
}


VOID
FLATSB_Unregister (void)
{
    UnregisterClassW (FLATSB_CLASSW, NULL);
}
