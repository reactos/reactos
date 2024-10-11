/*
 * Hotkey control
 *
 * Copyright 1998, 1999 Eric Kohl
 * Copyright 2002 Gyorgy 'Nog' Jeney
 * Copyright 2004 Robert Shearman
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
 * This code was audited for completeness against the documented features
 * of Comctl32.dll version 6.0 on Sep. 21, 2004, by Robert Shearman.
 * 
 * Unless otherwise noted, we believe this code to be complete, as per
 * the specification mentioned above.
 * If you discover missing features or bugs please note them below.
 *
 */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "commctrl.h"
#include "comctl32.h"
#include "uxtheme.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(hotkey);

typedef struct tagHOTKEY_INFO
{
    HWND  hwndSelf;
    HWND  hwndNotify;
    HFONT hFont;
    BOOL  bFocus;
    INT   nHeight;
    WORD  HotKey;
    WORD  InvComb;
    WORD  InvMod;
    BYTE  CurrMod;
    INT   CaretPos;
    DWORD ScanCode;
    WCHAR strNone[15]; /* hope it's long enough ... */
} HOTKEY_INFO;

static const WCHAR HOTKEY_plussep[] = { ' ', '+', ' ' };
static LRESULT HOTKEY_SetFont (HOTKEY_INFO *infoPtr, HFONT hFont, BOOL redraw);

#define IsOnlySet(flags) (infoPtr->CurrMod == (flags))

static BOOL
HOTKEY_IsCombInv(const HOTKEY_INFO *infoPtr)
{
    TRACE("(infoPtr=%p)\n", infoPtr);
    if((infoPtr->InvComb & HKCOMB_NONE) && !infoPtr->CurrMod)
	return TRUE;
    if((infoPtr->InvComb & HKCOMB_S) && IsOnlySet(HOTKEYF_SHIFT))
	return TRUE;
    if((infoPtr->InvComb & HKCOMB_C) && IsOnlySet(HOTKEYF_CONTROL))
	return TRUE;
    if((infoPtr->InvComb & HKCOMB_A) && IsOnlySet(HOTKEYF_ALT))
	return TRUE;
    if((infoPtr->InvComb & HKCOMB_SC) && 
       IsOnlySet(HOTKEYF_SHIFT | HOTKEYF_CONTROL))
	return TRUE;
    if((infoPtr->InvComb & HKCOMB_SA) && IsOnlySet(HOTKEYF_SHIFT | HOTKEYF_ALT))
	return TRUE;
    if((infoPtr->InvComb & HKCOMB_CA) && 
       IsOnlySet(HOTKEYF_CONTROL | HOTKEYF_ALT))
	return TRUE;
    if((infoPtr->InvComb & HKCOMB_SCA) && 
       IsOnlySet(HOTKEYF_SHIFT | HOTKEYF_CONTROL | HOTKEYF_ALT))
	return TRUE;

    TRACE("() Modifiers are valid\n");
    return FALSE;
}
#undef IsOnlySet

static void
HOTKEY_DrawHotKey(HOTKEY_INFO *infoPtr, HDC hdc, LPCWSTR KeyName, WORD NameLen)
{
    SIZE TextSize;
    INT nXStart, nYStart;
    COLORREF clrOldText, clrOldBk;
    HFONT hFontOld;

    /* Make a gap from the frame */
    nXStart = GetSystemMetrics(SM_CXBORDER);
    nYStart = GetSystemMetrics(SM_CYBORDER);

    hFontOld = SelectObject(hdc, infoPtr->hFont);
    if (GetWindowLongW(infoPtr->hwndSelf, GWL_STYLE) & WS_DISABLED)
    {
        clrOldText = SetTextColor(hdc, comctl32_color.clrGrayText);
        clrOldBk = SetBkColor(hdc, comctl32_color.clrBtnFace);
    }
    else
    {
        clrOldText = SetTextColor(hdc, comctl32_color.clrWindowText);
        clrOldBk = SetBkColor(hdc, comctl32_color.clrWindow);
    }

    TextOutW(hdc, nXStart, nYStart, KeyName, NameLen);

    /* Get the text width for the caret */
    GetTextExtentPoint32W(hdc, KeyName, NameLen, &TextSize);
    infoPtr->CaretPos = nXStart + TextSize.cx;

    SetBkColor(hdc, clrOldBk);
    SetTextColor(hdc, clrOldText);
    SelectObject(hdc, hFontOld);

    /* position the caret */
    SetCaretPos(infoPtr->CaretPos, nYStart);
}

/* Draw the names of the keys in the control */
static void 
HOTKEY_Refresh(HOTKEY_INFO *infoPtr, HDC hdc)
{
    WCHAR KeyName[64];
    WORD NameLen = 0;
    BYTE Modifier;

    TRACE("(infoPtr=%p hdc=%p)\n", infoPtr, hdc);

    if(!infoPtr->CurrMod && !infoPtr->HotKey) {
	HOTKEY_DrawHotKey (infoPtr, hdc, infoPtr->strNone, lstrlenW(infoPtr->strNone));
	return;
    }
	
    if(infoPtr->HotKey)
	Modifier = HIBYTE(infoPtr->HotKey);
    else if(HOTKEY_IsCombInv(infoPtr)) 
	Modifier = infoPtr->InvMod;
    else
	Modifier = infoPtr->CurrMod;

    if(Modifier & HOTKEYF_CONTROL) {
	GetKeyNameTextW(MAKELPARAM(0, MapVirtualKeyW(VK_CONTROL, 0)),
	                          KeyName, 64);
        NameLen = lstrlenW(KeyName);
	memcpy(&KeyName[NameLen], HOTKEY_plussep, sizeof(HOTKEY_plussep));
	NameLen += 3;
    }
    if(Modifier & HOTKEYF_SHIFT) {
	GetKeyNameTextW(MAKELPARAM(0, MapVirtualKeyW(VK_SHIFT, 0)),
	                           &KeyName[NameLen], 64 - NameLen);
	NameLen = lstrlenW(KeyName);
	memcpy(&KeyName[NameLen], HOTKEY_plussep, sizeof(HOTKEY_plussep));
	NameLen += 3;
    }
    if(Modifier & HOTKEYF_ALT) {
	GetKeyNameTextW(MAKELPARAM(0, MapVirtualKeyW(VK_MENU, 0)),
	                           &KeyName[NameLen], 64 - NameLen);
	NameLen = lstrlenW(KeyName);
	memcpy(&KeyName[NameLen], HOTKEY_plussep, sizeof(HOTKEY_plussep));
	NameLen += 3;
    }

    if(infoPtr->HotKey) {
	GetKeyNameTextW(infoPtr->ScanCode, &KeyName[NameLen], 64 - NameLen);
	NameLen = lstrlenW(KeyName);
    }
    else
	KeyName[NameLen] = 0;

    HOTKEY_DrawHotKey (infoPtr, hdc, KeyName, NameLen);
}

static void
HOTKEY_Paint(HOTKEY_INFO *infoPtr, HDC hdc)
{
    if (hdc)
	HOTKEY_Refresh(infoPtr, hdc);
    else {
	PAINTSTRUCT ps;
	hdc = BeginPaint (infoPtr->hwndSelf, &ps);
	HOTKEY_Refresh (infoPtr, hdc);
	EndPaint (infoPtr->hwndSelf, &ps);
    }
}

static LRESULT
HOTKEY_GetHotKey(const HOTKEY_INFO *infoPtr)
{
    TRACE("(infoPtr=%p) Modifiers: 0x%x, Virtual Key: %d\n", infoPtr, 
          HIBYTE(infoPtr->HotKey), LOBYTE(infoPtr->HotKey));
    return (LRESULT)infoPtr->HotKey;
}

static void
HOTKEY_SetHotKey(HOTKEY_INFO *infoPtr, WORD hotKey)
{
    infoPtr->HotKey = hotKey;
    infoPtr->ScanCode = 
        MAKELPARAM(0, MapVirtualKeyW(LOBYTE(infoPtr->HotKey), 0));
    TRACE("(infoPtr=%p hotKey=%x) Modifiers: 0x%x, Virtual Key: %d\n", infoPtr,
          hotKey, HIBYTE(infoPtr->HotKey), LOBYTE(infoPtr->HotKey));
    InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);
}

static void 
HOTKEY_SetRules(HOTKEY_INFO *infoPtr, WORD invComb, WORD invMod)
{
    infoPtr->InvComb = invComb;
    infoPtr->InvMod = invMod;
    TRACE("(infoPtr=%p) Invalid Modifiers: 0x%x, If Invalid: 0x%x\n", infoPtr,
          infoPtr->InvComb, infoPtr->InvMod);
}


static LRESULT
HOTKEY_Create (HOTKEY_INFO *infoPtr, const CREATESTRUCTW *lpcs)
{
    infoPtr->hwndNotify = lpcs->hwndParent;

    HOTKEY_SetFont(infoPtr, GetStockObject(SYSTEM_FONT), 0);

    return 0;
}


static LRESULT
HOTKEY_Destroy (HOTKEY_INFO *infoPtr)
{
    /* Free hotkey info data */
    SetWindowLongPtrW (infoPtr->hwndSelf, 0, 0);
    Free (infoPtr);
    return 0;
}


static LRESULT
HOTKEY_EraseBackground (const HOTKEY_INFO *infoPtr, HDC hdc)
{
    HBRUSH hBrush, hSolidBrush = NULL;
    RECT   rc;

    if (GetWindowLongW(infoPtr->hwndSelf, GWL_STYLE) & WS_DISABLED)
        hBrush = hSolidBrush = CreateSolidBrush(comctl32_color.clrBtnFace);
    else
    {
        hBrush = (HBRUSH)SendMessageW(infoPtr->hwndNotify, WM_CTLCOLOREDIT,
                                      (WPARAM)hdc, (LPARAM)infoPtr->hwndSelf);
        if (!hBrush)
            hBrush = hSolidBrush = CreateSolidBrush(comctl32_color.clrWindow);
    }

    GetClientRect (infoPtr->hwndSelf, &rc);

    FillRect (hdc, &rc, hBrush);

    if (hSolidBrush)
        DeleteObject(hSolidBrush);

    return -1;
}


static inline LRESULT
HOTKEY_GetFont (const HOTKEY_INFO *infoPtr)
{
    return (LRESULT)infoPtr->hFont;
}

static LRESULT
HOTKEY_KeyDown (HOTKEY_INFO *infoPtr, DWORD key, DWORD flags)
{
    WORD wOldHotKey;
    BYTE bOldMod;

    if (GetWindowLongW(infoPtr->hwndSelf, GWL_STYLE) & WS_DISABLED)
        return 0;

    TRACE("() Key: %ld\n", key);

    wOldHotKey = infoPtr->HotKey;
    bOldMod = infoPtr->CurrMod;

    /* If any key is Pressed, we have to reset the hotkey in the control */
    infoPtr->HotKey = 0;

    switch (key)
    {
	case VK_RETURN:
	case VK_TAB:
	case VK_SPACE:
	case VK_DELETE:
	case VK_ESCAPE:
	case VK_BACK:
	    InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);
	    return DefWindowProcW (infoPtr->hwndSelf, WM_KEYDOWN, key, flags);

	case VK_SHIFT:
	    infoPtr->CurrMod |= HOTKEYF_SHIFT;
	    break;
	case VK_CONTROL:
	    infoPtr->CurrMod |= HOTKEYF_CONTROL;
	    break;
	case VK_MENU:
	    infoPtr->CurrMod |= HOTKEYF_ALT;
	    break;

	default:
	    if(HOTKEY_IsCombInv(infoPtr))
	        infoPtr->HotKey = MAKEWORD(key, infoPtr->InvMod);
	    else
	        infoPtr->HotKey = MAKEWORD(key, infoPtr->CurrMod);
	    infoPtr->ScanCode = flags;
	    break;
    }

    if ((wOldHotKey != infoPtr->HotKey) || (bOldMod != infoPtr->CurrMod))
    {
        InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);

        /* send EN_CHANGE notification */
        SendMessageW(infoPtr->hwndNotify, WM_COMMAND,
            MAKEWPARAM(GetDlgCtrlID(infoPtr->hwndSelf), EN_CHANGE),
            (LPARAM)infoPtr->hwndSelf);
    }

    return 0;
}


static LRESULT
HOTKEY_KeyUp (HOTKEY_INFO *infoPtr, DWORD key)
{
    BYTE bOldMod;

    if (GetWindowLongW(infoPtr->hwndSelf, GWL_STYLE) & WS_DISABLED)
        return 0;

    TRACE("() Key: %ld\n", key);

    bOldMod = infoPtr->CurrMod;

    switch (key)
    {
	case VK_SHIFT:
	    infoPtr->CurrMod &= ~HOTKEYF_SHIFT;
	    break;
	case VK_CONTROL:
	    infoPtr->CurrMod &= ~HOTKEYF_CONTROL;
	    break;
	case VK_MENU:
	    infoPtr->CurrMod &= ~HOTKEYF_ALT;
	    break;
	default:
	    return 1;
    }

    if (bOldMod != infoPtr->CurrMod)
    {
        InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);

        /* send EN_CHANGE notification */
        SendMessageW(infoPtr->hwndNotify, WM_COMMAND,
            MAKEWPARAM(GetDlgCtrlID(infoPtr->hwndSelf), EN_CHANGE),
            (LPARAM)infoPtr->hwndSelf);
    }

    return 0;
}


static LRESULT
HOTKEY_KillFocus (HOTKEY_INFO *infoPtr)
{
    infoPtr->bFocus = FALSE;
    DestroyCaret ();

    return 0;
}


static LRESULT
HOTKEY_LButtonDown (const HOTKEY_INFO *infoPtr)
{
    if (!(GetWindowLongW(infoPtr->hwndSelf, GWL_STYLE) & WS_DISABLED))
        SetFocus (infoPtr->hwndSelf);

    return 0;
}


static inline LRESULT
HOTKEY_NCCreate (HWND hwnd, const CREATESTRUCTW *lpcs)
{
    HOTKEY_INFO *infoPtr;
    DWORD dwExStyle = GetWindowLongW (hwnd, GWL_EXSTYLE);
    SetWindowLongW (hwnd, GWL_EXSTYLE, 
                    dwExStyle | WS_EX_CLIENTEDGE);

    /* allocate memory for info structure */
    infoPtr = Alloc(sizeof(*infoPtr));
    SetWindowLongPtrW(hwnd, 0, (DWORD_PTR)infoPtr);

    /* initialize info structure */
    infoPtr->HotKey = infoPtr->InvComb = infoPtr->InvMod = infoPtr->CurrMod = 0;
    infoPtr->CaretPos = GetSystemMetrics(SM_CXBORDER);
    infoPtr->hwndSelf = hwnd;
    LoadStringW(COMCTL32_hModule, HKY_NONE, infoPtr->strNone, 15);

    return DefWindowProcW (infoPtr->hwndSelf, WM_NCCREATE, 0, (LPARAM)lpcs);
}

static LRESULT
HOTKEY_NCPaint (HWND hwnd, HRGN region)
{
    INT cxEdge, cyEdge;
    HRGN clipRgn;
    HTHEME theme;
    LONG exStyle;
    RECT r;
    HDC dc;

    theme = OpenThemeDataForDpi(NULL, WC_EDITW, GetDpiForWindow(hwnd));
    if (!theme)
        return DefWindowProcW(hwnd, WM_NCPAINT, (WPARAM)region, 0);

    exStyle = GetWindowLongW(hwnd, GWL_EXSTYLE);
    if (!(exStyle & WS_EX_CLIENTEDGE))
    {
        CloseThemeData(theme);
        return DefWindowProcW(hwnd, WM_NCPAINT, (WPARAM)region, 0);
    }

    cxEdge = GetSystemMetrics(SM_CXEDGE);
    cyEdge = GetSystemMetrics(SM_CYEDGE);
    GetWindowRect(hwnd, &r);

    /* New clipping region passed to default proc to exclude border */
    clipRgn = CreateRectRgn(r.left + cxEdge, r.top + cyEdge, r.right - cxEdge, r.bottom - cyEdge);
    if (region != (HRGN)1)
        CombineRgn(clipRgn, clipRgn, region, RGN_AND);
    OffsetRect(&r, -r.left, -r.top);

    dc = GetDCEx(hwnd, region, DCX_WINDOW | DCX_INTERSECTRGN);
    if (IsThemeBackgroundPartiallyTransparent(theme, 0, 0))
        DrawThemeParentBackground(hwnd, dc, &r);
    DrawThemeBackground(theme, dc, 0, 0, &r, 0);
    ReleaseDC(hwnd, dc);
    CloseThemeData(theme);

    /* Call default proc to get the scrollbars etc. also painted */
    DefWindowProcW(hwnd, WM_NCPAINT, (WPARAM)clipRgn, 0);
    DeleteObject(clipRgn);
    return 0;
}

static LRESULT
HOTKEY_SetFocus (HOTKEY_INFO *infoPtr)
{
    infoPtr->bFocus = TRUE;

    CreateCaret (infoPtr->hwndSelf, NULL, 1, infoPtr->nHeight);
    SetCaretPos (infoPtr->CaretPos, GetSystemMetrics(SM_CYBORDER));
    ShowCaret (infoPtr->hwndSelf);

    return 0;
}


static LRESULT
HOTKEY_SetFont (HOTKEY_INFO *infoPtr, HFONT hFont, BOOL redraw)
{
    TEXTMETRICW tm;
    HDC hdc;
    HFONT hOldFont = 0;

    infoPtr->hFont = hFont;

    hdc = GetDC (infoPtr->hwndSelf);
    if (infoPtr->hFont)
	hOldFont = SelectObject (hdc, infoPtr->hFont);

    GetTextMetricsW (hdc, &tm);
    infoPtr->nHeight = tm.tmHeight;

    if (infoPtr->hFont)
	SelectObject (hdc, hOldFont);
    ReleaseDC (infoPtr->hwndSelf, hdc);

    if (redraw)
	InvalidateRect (infoPtr->hwndSelf, NULL, TRUE);

    return 0;
}

static LRESULT WINAPI
HOTKEY_WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HOTKEY_INFO *infoPtr = (HOTKEY_INFO *)GetWindowLongPtrW (hwnd, 0);

    TRACE("hwnd %p, msg %x, wparam %Ix, lparam %Ix\n", hwnd, uMsg, wParam, lParam);

    if (!infoPtr && (uMsg != WM_NCCREATE))
        return DefWindowProcW (hwnd, uMsg, wParam, lParam);
    switch (uMsg)
    {
	case HKM_GETHOTKEY:
	    return HOTKEY_GetHotKey (infoPtr);
	case HKM_SETHOTKEY:
	    HOTKEY_SetHotKey (infoPtr, (WORD)wParam);
	    break;
	case HKM_SETRULES:
            HOTKEY_SetRules (infoPtr, (WORD)wParam, (WORD)lParam);
	    break;

	case WM_CHAR:
	case WM_SYSCHAR:
	    return HOTKEY_KeyDown (infoPtr, MapVirtualKeyW(LOBYTE(HIWORD(lParam)), 1), lParam);

	case WM_CREATE:
	    return HOTKEY_Create (infoPtr, (LPCREATESTRUCTW)lParam);

	case WM_DESTROY:
	    return HOTKEY_Destroy (infoPtr);

	case WM_ERASEBKGND:
	    return HOTKEY_EraseBackground (infoPtr, (HDC)wParam);

	case WM_GETDLGCODE:
	    return DLGC_WANTCHARS | DLGC_WANTARROWS;

	case WM_GETFONT:
	    return HOTKEY_GetFont (infoPtr);

	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	    return HOTKEY_KeyDown (infoPtr, wParam, lParam);

	case WM_KEYUP:
	case WM_SYSKEYUP:
	    return HOTKEY_KeyUp (infoPtr, wParam);

	case WM_KILLFOCUS:
	    return HOTKEY_KillFocus (infoPtr);

	case WM_LBUTTONDOWN:
	    return HOTKEY_LButtonDown (infoPtr);

	case WM_NCCREATE:
	    return HOTKEY_NCCreate (hwnd, (LPCREATESTRUCTW)lParam);

        case WM_NCPAINT:
            return HOTKEY_NCPaint (hwnd, (HRGN)wParam);

	case WM_PRINTCLIENT:
	case WM_PAINT:
	    HOTKEY_Paint(infoPtr, (HDC)wParam);
	    return 0;

	case WM_SETFOCUS:
	    return HOTKEY_SetFocus (infoPtr);

	case WM_SETFONT:
	    return HOTKEY_SetFont (infoPtr, (HFONT)wParam, LOWORD(lParam));

	default:
	    if ((uMsg >= WM_USER) && (uMsg < WM_APP) && !COMCTL32_IsReflectedMessage(uMsg))
		ERR("unknown msg %04x, wp %Ix, lp %Ix\n", uMsg, wParam, lParam);
	    return DefWindowProcW (hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


void
HOTKEY_Register (void)
{
    WNDCLASSW wndClass;

    ZeroMemory (&wndClass, sizeof(WNDCLASSW));
    wndClass.style         = CS_GLOBALCLASS;
    wndClass.lpfnWndProc   = HOTKEY_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(HOTKEY_INFO *);
    wndClass.hCursor       = 0;
    wndClass.hbrBackground = 0;
    wndClass.lpszClassName = HOTKEY_CLASSW;

    RegisterClassW (&wndClass);
}


void
HOTKEY_Unregister (void)
{
    UnregisterClassW (HOTKEY_CLASSW, NULL);
}
