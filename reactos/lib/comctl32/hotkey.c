/*
 * Hotkey control
 *
 * Copyright 1998, 1999 Eric Kohl
 * Copyright 2002 Gyorgy 'Nog' Jeney
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
 * TODO:
 *   - What are we meant to do with the WM_CHAR message?
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

WINE_DEFAULT_DEBUG_CHANNEL(hotkey);

typedef struct tagHOTKEY_INFO
{
    HWND  hwndSelf;
    HFONT hFont;
    BOOL  bFocus;
    INT   nHeight;
    WORD  HotKey;
    WORD  InvComb;
    WORD  InvMod;
    BYTE  CurrMod;
    INT   CaretPos;
    DWORD ScanCode;
    WCHAR strNone[15]; /* hope its long enough ... */
} HOTKEY_INFO;

#define HOTKEY_GetInfoPtr(hwnd) ((HOTKEY_INFO *)GetWindowLongA (hwnd, 0))

static const WCHAR HOTKEY_plussep[] = { ' ', '+', ' ' };

#define IsOnlySet(flags) (infoPtr->CurrMod == (flags))

static BOOL
HOTKEY_IsCombInv(HOTKEY_INFO *infoPtr)
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
HOTKEY_DrawHotKey(HOTKEY_INFO *infoPtr, LPCWSTR KeyName, WORD NameLen, 
                  LPRECT rc, HDC hdc)
{
    SIZE TextSize;
    DWORD dwExStyle = GetWindowLongW (infoPtr->hwndSelf, GWL_EXSTYLE);

    /* We have to allow some space for the frame to be drawn */
    rc->left += 2;
    rc->top++;
    DrawTextW(hdc, KeyName, NameLen, rc, DT_LEFT | DT_VCENTER);
    rc->left -= 2;
    rc->top--;
    if(dwExStyle & WS_EX_CLIENTEDGE)
	DrawEdge(hdc, rc, EDGE_SUNKEN, BF_RECT | BF_ADJUST);

    /* Get the text size and position the caret accordingly */
    GetTextExtentPoint32W (hdc, KeyName, NameLen, &TextSize);
    infoPtr->CaretPos = TextSize.cx + 2;
    SetCaretPos(infoPtr->CaretPos, 3);
}

/* Draw the names of the keys in the control */
static void 
HOTKEY_Refresh(HOTKEY_INFO *infoPtr, HDC hdc)
{
    WCHAR KeyName[sizeof(WCHAR) * 64];
    WORD NameLen = 0;
    BYTE Modifier;
    RECT rc;

    GetClientRect(infoPtr->hwndSelf, &rc);

    TRACE("(infoPtr=%p hdc=%p)\n", infoPtr, hdc);

    if(!infoPtr->CurrMod && !infoPtr->HotKey) {
	HOTKEY_DrawHotKey (infoPtr, infoPtr->strNone, 4, &rc, hdc);
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

    HOTKEY_DrawHotKey (infoPtr, KeyName, NameLen, &rc, hdc);
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
HOTKEY_GetHotKey(HOTKEY_INFO *infoPtr)
{
    TRACE("(infoPtr=%p) Modifiers: 0x%x, Virtual Key: %d\n", infoPtr, 
          HIBYTE(infoPtr->HotKey), LOBYTE(infoPtr->HotKey));
    return (LRESULT)infoPtr->HotKey;
}

static void
HOTKEY_SetHotKey(HOTKEY_INFO *infoPtr, WPARAM wParam)
{
    infoPtr->HotKey = (WORD)wParam;
    infoPtr->ScanCode = 
        MAKELPARAM(0, MapVirtualKeyW(LOBYTE(infoPtr->HotKey), 0));
    TRACE("(infoPtr=%p wParam=%x) Modifiers: 0x%x, Virtual Key: %d\n", infoPtr,
          wParam, HIBYTE(infoPtr->HotKey), LOBYTE(infoPtr->HotKey));
    InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);
}

static void 
HOTKEY_SetRules(HOTKEY_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    infoPtr->InvComb = (WORD)wParam;
    infoPtr->InvMod = (WORD)lParam;
    TRACE("(infoPtr=%p) Invalid Modifers: 0x%x, If Invalid: 0x%x\n", infoPtr,
          infoPtr->InvComb, infoPtr->InvMod);
}

/* << HOTKEY_Char >> */

static LRESULT
HOTKEY_Create (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HOTKEY_INFO *infoPtr;
    TEXTMETRICW tm;
    HDC hdc;

    /* allocate memory for info structure */
    infoPtr = (HOTKEY_INFO *)Alloc (sizeof(HOTKEY_INFO));
    SetWindowLongW (hwnd, 0, (DWORD)infoPtr);

    /* initialize info structure */
    infoPtr->HotKey = infoPtr->InvComb = infoPtr->InvMod = infoPtr->CurrMod = 0;
    infoPtr->CaretPos = 2;
    infoPtr->hwndSelf = hwnd;
    LoadStringW(COMCTL32_hModule, HKY_NONE, infoPtr->strNone, 15);

    /* get default font height */
    hdc = GetDC (hwnd);
    GetTextMetricsW (hdc, &tm);
    infoPtr->nHeight = tm.tmHeight;
    ReleaseDC (hwnd, hdc);

    return 0;
}


static LRESULT
HOTKEY_Destroy (HOTKEY_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    HWND hwnd = infoPtr->hwndSelf;
    /* free hotkey info data */
    Free (infoPtr);
    SetWindowLongW (hwnd, 0, 0);
    return 0;
}


static LRESULT
HOTKEY_EraseBackground (HOTKEY_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    HBRUSH hBrush;
    RECT   rc;

    hBrush =
	(HBRUSH)SendMessageW (GetParent (infoPtr->hwndSelf), WM_CTLCOLOREDIT,
				wParam, (LPARAM)infoPtr->hwndSelf);
    if (hBrush)
	hBrush = (HBRUSH)GetStockObject (WHITE_BRUSH);
    GetClientRect (infoPtr->hwndSelf, &rc);

    FillRect ((HDC)wParam, &rc, hBrush);

    return -1;
}


inline static LRESULT
HOTKEY_GetFont (HOTKEY_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    return (LRESULT)infoPtr->hFont;
}

static LRESULT
HOTKEY_KeyDown (HOTKEY_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    TRACE("() Key: %d\n", wParam);
    /* If any key is Pressed, we have to reset the hotkey in the control */
    infoPtr->HotKey = 0;

    switch (wParam) {
	case VK_RETURN:
	case VK_TAB:
	case VK_SPACE:
	case VK_DELETE:
	case VK_ESCAPE:
	case VK_BACK:
	    InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);
	    return DefWindowProcW (infoPtr->hwndSelf, WM_KEYDOWN, wParam, 
	                           lParam);

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
	        infoPtr->HotKey = MAKEWORD(wParam, infoPtr->InvMod);
	    else
	        infoPtr->HotKey = MAKEWORD(wParam, infoPtr->CurrMod);
	    infoPtr->ScanCode = lParam;
	    break;
    }

    InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);
    return 0;
}


static LRESULT
HOTKEY_KeyUp (HOTKEY_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    TRACE("() Key: %d\n", wParam);
    switch (wParam) {
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

    InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);

    return 0;
}


static LRESULT
HOTKEY_KillFocus (HOTKEY_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    infoPtr->bFocus = FALSE;
    DestroyCaret ();

    return 0;
}


static LRESULT
HOTKEY_LButtonDown (HOTKEY_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    SetFocus (infoPtr->hwndSelf);

    return 0;
}


inline static LRESULT
HOTKEY_NCCreate (HOTKEY_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    DWORD dwExStyle = GetWindowLongW (infoPtr->hwndSelf, GWL_EXSTYLE);
    SetWindowLongW (infoPtr->hwndSelf, GWL_EXSTYLE, 
                    dwExStyle | WS_EX_CLIENTEDGE);
    return DefWindowProcW (infoPtr->hwndSelf, WM_NCCREATE, wParam, lParam);
}

static LRESULT
HOTKEY_SetFocus (HOTKEY_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    infoPtr->bFocus = TRUE;


    CreateCaret (infoPtr->hwndSelf, NULL, 1, infoPtr->nHeight - 2);

    SetCaretPos (infoPtr->CaretPos, 3);

    ShowCaret (infoPtr->hwndSelf);


    return 0;
}


inline static LRESULT
HOTKEY_SetFont (HOTKEY_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    TEXTMETRICW tm;
    HDC hdc;
    HFONT hOldFont = 0;

    infoPtr->hFont = (HFONT)wParam;

    hdc = GetDC (infoPtr->hwndSelf);
    if (infoPtr->hFont)
	hOldFont = SelectObject (hdc, infoPtr->hFont);

    GetTextMetricsW (hdc, &tm);
    infoPtr->nHeight = tm.tmHeight;

    if (infoPtr->hFont)
	SelectObject (hdc, hOldFont);
    ReleaseDC (infoPtr->hwndSelf, hdc);

    if (LOWORD(lParam))
	InvalidateRect (infoPtr->hwndSelf, NULL, TRUE);

    return 0;
}

static LRESULT WINAPI
HOTKEY_WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HOTKEY_INFO *infoPtr = HOTKEY_GetInfoPtr (hwnd);
    TRACE("hwnd=%p msg=%x wparam=%x lparam=%lx\n", hwnd, uMsg, wParam, lParam);
    if (!infoPtr && (uMsg != WM_CREATE))
        return DefWindowProcW (hwnd, uMsg, wParam, lParam);
    switch (uMsg)
    {
	case HKM_GETHOTKEY:
	    return HOTKEY_GetHotKey (infoPtr);
	case HKM_SETHOTKEY:
	    HOTKEY_SetHotKey (infoPtr, wParam);
	    break;
	case HKM_SETRULES:
            HOTKEY_SetRules (infoPtr, wParam, lParam);
	    break;

/*	case WM_CHAR: */

	case WM_CREATE:
	    return HOTKEY_Create (hwnd, wParam, lParam);

	case WM_DESTROY:
	    return HOTKEY_Destroy (infoPtr, wParam, lParam);

	case WM_ERASEBKGND:
	    return HOTKEY_EraseBackground (infoPtr, wParam, lParam);

	case WM_GETDLGCODE:
	    return DLGC_WANTCHARS | DLGC_WANTARROWS;

	case WM_GETFONT:
	    return HOTKEY_GetFont (infoPtr, wParam, lParam);

	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	    return HOTKEY_KeyDown (infoPtr, wParam, lParam);

	case WM_KEYUP:
	case WM_SYSKEYUP:
	    return HOTKEY_KeyUp (infoPtr, wParam, lParam);

	case WM_KILLFOCUS:
	    return HOTKEY_KillFocus (infoPtr, wParam, lParam);

	case WM_LBUTTONDOWN:
	    return HOTKEY_LButtonDown (infoPtr, wParam, lParam);

	case WM_NCCREATE:
	    return HOTKEY_NCCreate (infoPtr, wParam, lParam);

	case WM_PAINT:
	    HOTKEY_Paint(infoPtr, (HDC)wParam);
	    return 0;

	case WM_SETFOCUS:
	    return HOTKEY_SetFocus (infoPtr, wParam, lParam);

	case WM_SETFONT:
	    return HOTKEY_SetFont (infoPtr, wParam, lParam);

/*	case WM_SYSCHAR: */

	default:
	    if ((uMsg >= WM_USER) && (uMsg < WM_APP))
		ERR("unknown msg %04x wp=%08x lp=%08lx\n",
		     uMsg, wParam, lParam);
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
    wndClass.lpfnWndProc   = (WNDPROC)HOTKEY_WindowProc;
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
