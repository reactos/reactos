/*
 * IP Address control
 *
 * Copyright 2002 Dimitrie O. Paun
 * Copyright 1999 Chris Morgan<cmorgan@wpi.edu>
 * Copyright 1999 James Abbatiello<abbeyj@wpi.edu>
 * Copyright 1998, 1999 Eric Kohl
 * Copyright 1998 Alex Priem <alexp@sci.kun.nl>
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
 * NOTE
 * 
 * This code was audited for completeness against the documented features
 * of Comctl32.dll version 6.0 on Sep. 9, 2002, by Dimitrie O. Paun.
 * 
 * Unless otherwise noted, we believe this code to be complete, as per
 * the specification mentioned above.
 * If you discover missing features, or bugs, please note them below.
 * 
 */

#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "commctrl.h"
#include "comctl32.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ipaddress);

typedef struct
{
    HWND     EditHwnd;
    INT      LowerLimit;
    INT      UpperLimit;
    WNDPROC  OrigProc;
} IPPART_INFO;

typedef struct
{
    HWND	Self;
    HWND	Notify;
    BOOL	Enabled;
    IPPART_INFO	Part[4];
} IPADDRESS_INFO;

static const WCHAR IP_SUBCLASS_PROP[] = 
    { 'C', 'C', 'I', 'P', '3', '2', 'S', 'u', 'b', 'c', 'l', 'a', 's', 's', 'I', 'n', 'f', 'o', 0 };

#define POS_DEFAULT	0
#define POS_LEFT	1
#define POS_RIGHT	2
#define POS_SELALL	3

static LRESULT CALLBACK
IPADDRESS_SubclassProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static LRESULT IPADDRESS_Notify (const IPADDRESS_INFO *infoPtr, UINT command)
{
    HWND hwnd = infoPtr->Self;

    TRACE("(command=%x)\n", command);

    return SendMessageW (infoPtr->Notify, WM_COMMAND,
             MAKEWPARAM (GetWindowLongPtrW (hwnd, GWLP_ID), command), (LPARAM)hwnd);
}

static INT IPADDRESS_IPNotify (const IPADDRESS_INFO *infoPtr, INT field, INT value)
{
    NMIPADDRESS nmip;

    TRACE("(field=%x, value=%d)\n", field, value);

    nmip.hdr.hwndFrom = infoPtr->Self;
    nmip.hdr.idFrom   = GetWindowLongPtrW (infoPtr->Self, GWLP_ID);
    nmip.hdr.code     = IPN_FIELDCHANGED;

    nmip.iField = field;
    nmip.iValue = value;

    SendMessageW (infoPtr->Notify, WM_NOTIFY,
                  (WPARAM)nmip.hdr.idFrom, (LPARAM)&nmip);

    TRACE("<-- %d\n", nmip.iValue);

    return nmip.iValue;
}


static int IPADDRESS_GetPartIndex(const IPADDRESS_INFO *infoPtr, HWND hwnd)
{
    int i;

    TRACE("(hwnd=%p)\n", hwnd);

    for (i = 0; i < 4; i++)
        if (infoPtr->Part[i].EditHwnd == hwnd) return i;

    ERR("We subclassed the wrong window! (hwnd=%p)\n", hwnd);
    return -1;
}


static LRESULT IPADDRESS_Draw (const IPADDRESS_INFO *infoPtr, HDC hdc)
{
    static const WCHAR dotW[] = { '.', 0 };
    RECT rect, rcPart;
    POINT pt;
    COLORREF bgCol, fgCol;
    int i;

    TRACE("\n");

    GetClientRect (infoPtr->Self, &rect);

    if (infoPtr->Enabled) {
        bgCol = COLOR_WINDOW;
        fgCol = COLOR_WINDOWTEXT;
    } else {
        bgCol = COLOR_3DFACE;
        fgCol = COLOR_GRAYTEXT;
    }
    
    FillRect (hdc, &rect, (HBRUSH)(DWORD_PTR)(bgCol+1));
    DrawEdge (hdc, &rect, EDGE_SUNKEN, BF_RECT | BF_ADJUST);
    
    SetBkColor  (hdc, GetSysColor(bgCol));
    SetTextColor(hdc, GetSysColor(fgCol));

    for (i = 0; i < 3; i++) {
        GetWindowRect (infoPtr->Part[i].EditHwnd, &rcPart);
	pt.x = rcPart.right;
	ScreenToClient(infoPtr->Self, &pt);
	rect.left = pt.x;
	GetWindowRect (infoPtr->Part[i+1].EditHwnd, &rcPart);
	pt.x = rcPart.left;
	ScreenToClient(infoPtr->Self, &pt);
	rect.right = pt.x;
	DrawTextW(hdc, dotW, 1, &rect, DT_SINGLELINE | DT_CENTER | DT_BOTTOM);
    }

    return 0;
}


static LRESULT IPADDRESS_Create (HWND hwnd, const CREATESTRUCTA *lpCreate)
{
    static const WCHAR EDIT[] = { 'E', 'd', 'i', 't', 0 };
    IPADDRESS_INFO *infoPtr;
    RECT rcClient, edit;
    int i, fieldsize;

    TRACE("\n");

    SetWindowLongW (hwnd, GWL_STYLE,
		    GetWindowLongW(hwnd, GWL_STYLE) & ~WS_BORDER);

    infoPtr = (IPADDRESS_INFO *)Alloc (sizeof(IPADDRESS_INFO));
    if (!infoPtr) return -1;
    SetWindowLongPtrW (hwnd, 0, (DWORD_PTR)infoPtr);

    GetClientRect (hwnd, &rcClient);

    fieldsize = (rcClient.right - rcClient.left) / 4;

    edit.top    = rcClient.top + 2;
    edit.bottom = rcClient.bottom - 2;

    infoPtr->Self = hwnd;
    infoPtr->Enabled = TRUE;
    infoPtr->Notify = lpCreate->hwndParent;

    for (i = 0; i < 4; i++) {
	IPPART_INFO* part = &infoPtr->Part[i];

	part->LowerLimit = 0;
	part->UpperLimit = 255;
        edit.left = rcClient.left + i*fieldsize + 6;
        edit.right = rcClient.left + (i+1)*fieldsize - 2;
        part->EditHwnd =
		CreateWindowW (EDIT, NULL, WS_CHILD | WS_VISIBLE | ES_CENTER,
                               edit.left, edit.top, edit.right - edit.left,
			       edit.bottom - edit.top, hwnd, (HMENU) 1,
			       (HINSTANCE)GetWindowLongPtrW(hwnd, GWLP_HINSTANCE), NULL);
	SetPropW(part->EditHwnd, IP_SUBCLASS_PROP, hwnd);
        part->OrigProc = (WNDPROC)
		SetWindowLongPtrW (part->EditHwnd, GWLP_WNDPROC,
				(DWORD_PTR)IPADDRESS_SubclassProc);
        EnableWindow(part->EditHwnd, infoPtr->Enabled);
    }

    return 0;
}


static LRESULT IPADDRESS_Destroy (IPADDRESS_INFO *infoPtr)
{
    int i;

    TRACE("\n");

    for (i = 0; i < 4; i++) {
	IPPART_INFO* part = &infoPtr->Part[i];
        SetWindowLongPtrW (part->EditHwnd, GWLP_WNDPROC, (DWORD_PTR)part->OrigProc);
    }

    SetWindowLongPtrW (infoPtr->Self, 0, 0);
    Free (infoPtr);
    return 0;
}


static LRESULT IPADDRESS_Enable (IPADDRESS_INFO *infoPtr, BOOL enabled)
{
    int i;

    infoPtr->Enabled = enabled;

    for (i = 0; i < 4; i++)
        EnableWindow(infoPtr->Part[i].EditHwnd, enabled);

    InvalidateRgn(infoPtr->Self, NULL, FALSE);
    return 0;
}


static LRESULT IPADDRESS_Paint (const IPADDRESS_INFO *infoPtr, HDC hdc)
{
    PAINTSTRUCT ps;

    TRACE("\n");

    if (hdc) return IPADDRESS_Draw (infoPtr, hdc);

    hdc = BeginPaint (infoPtr->Self, &ps);
    IPADDRESS_Draw (infoPtr, hdc);
    EndPaint (infoPtr->Self, &ps);
    return 0;
}


static BOOL IPADDRESS_IsBlank (const IPADDRESS_INFO *infoPtr)
{
    int i;

    TRACE("\n");

    for (i = 0; i < 4; i++)
        if (GetWindowTextLengthW (infoPtr->Part[i].EditHwnd)) return FALSE;

    return TRUE;
}


static int IPADDRESS_GetAddress (const IPADDRESS_INFO *infoPtr, LPDWORD ip_address)
{
    WCHAR field[5];
    int i, invalid = 0;
    DWORD ip_addr = 0;

    TRACE("\n");

    for (i = 0; i < 4; i++) {
        ip_addr *= 256;
        if (GetWindowTextW (infoPtr->Part[i].EditHwnd, field, 4))
  	    ip_addr += atolW(field);
	else
	    invalid++;
    }
    *ip_address = ip_addr;

    return 4 - invalid;
}


static BOOL IPADDRESS_SetRange (IPADDRESS_INFO *infoPtr, int index, WORD range)
{
    TRACE("\n");

    if ( (index < 0) || (index > 3) ) return FALSE;

    infoPtr->Part[index].LowerLimit = range & 0xFF;
    infoPtr->Part[index].UpperLimit = (range >> 8)  & 0xFF;

    return TRUE;
}


static void IPADDRESS_ClearAddress (const IPADDRESS_INFO *infoPtr)
{
    WCHAR nil[1] = { 0 };
    int i;

    TRACE("\n");

    for (i = 0; i < 4; i++)
        SetWindowTextW (infoPtr->Part[i].EditHwnd, nil);
}


static LRESULT IPADDRESS_SetAddress (const IPADDRESS_INFO *infoPtr, DWORD ip_address)
{
    WCHAR buf[20];
    static const WCHAR fmt[] = { '%', 'd', 0 };
    int i;

    TRACE("\n");

    for (i = 3; i >= 0; i--) {
	const IPPART_INFO* part = &infoPtr->Part[i];
        int value = ip_address & 0xff;
	if ( (value >= part->LowerLimit) && (value <= part->UpperLimit) ) {
	    wsprintfW (buf, fmt, value);
	    SetWindowTextW (part->EditHwnd, buf);
	    IPADDRESS_Notify (infoPtr, EN_CHANGE);
        }
        ip_address >>= 8;
    }

    return TRUE;
}


static void IPADDRESS_SetFocusToField (const IPADDRESS_INFO *infoPtr, INT index)
{
    TRACE("(index=%d)\n", index);

    if (index > 3 || index < 0) index=0;

    SetFocus (infoPtr->Part[index].EditHwnd);
}


static BOOL IPADDRESS_ConstrainField (const IPADDRESS_INFO *infoPtr, int currentfield)
{
    const IPPART_INFO *part = &infoPtr->Part[currentfield];
    WCHAR field[10];
    static const WCHAR fmt[] = { '%', 'd', 0 };
    int curValue, newValue;

    TRACE("(currentfield=%d)\n", currentfield);

    if (currentfield < 0 || currentfield > 3) return FALSE;

    if (!GetWindowTextW (part->EditHwnd, field, 4)) return FALSE;

    curValue = atoiW(field);
    TRACE("  curValue=%d\n", curValue);

    newValue = IPADDRESS_IPNotify(infoPtr, currentfield, curValue);
    TRACE("  newValue=%d\n", newValue);

    if (newValue < part->LowerLimit) newValue = part->LowerLimit;
    if (newValue > part->UpperLimit) newValue = part->UpperLimit;

    if (newValue == curValue) return FALSE;

    wsprintfW (field, fmt, newValue);
    TRACE("  field=%s\n", debugstr_w(field));
    return SetWindowTextW (part->EditHwnd, field);
}


static BOOL IPADDRESS_GotoNextField (const IPADDRESS_INFO *infoPtr, int cur, int sel)
{
    TRACE("\n");

    if(cur >= -1 && cur < 4) {
	IPADDRESS_ConstrainField(infoPtr, cur);

	if(cur < 3) {
	    const IPPART_INFO *next = &infoPtr->Part[cur + 1];
	    int start = 0, end = 0;
            SetFocus (next->EditHwnd);
	    if (sel != POS_DEFAULT) {
		if (sel == POS_RIGHT)
		    start = end = GetWindowTextLengthW(next->EditHwnd);
		else if (sel == POS_SELALL)
		    end = -1;
	        SendMessageW(next->EditHwnd, EM_SETSEL, start, end);
	    }
	    return TRUE;
	}

    }
    return FALSE;
}


/*
 * period: move and select the text in the next field to the right if
 *         the current field is not empty(l!=0), we are not in the
 *         left most position, and nothing is selected(startsel==endsel)
 *
 * spacebar: same behavior as period
 *
 * alpha characters: completely ignored
 *
 * digits: accepted when field text length < 2 ignored otherwise.
 *         when 3 numbers have been entered into the field the value
 *         of the field is checked, if the field value exceeds the
 *         maximum value and is changed the field remains the current
 *         field, otherwise focus moves to the field to the right
 *
 * tab: change focus from the current ipaddress control to the next
 *      control in the tab order
 *
 * right arrow: move to the field on the right to the left most
 *              position in that field if no text is selected,
 *              we are in the right most position in the field,
 *              we are not in the right most field
 *
 * left arrow: move to the field on the left to the right most
 *             position in that field if no text is selected,
 *             we are in the left most position in the current field
 *             and we are not in the left most field
 *
 * backspace: delete the character to the left of the cursor position,
 *            if none are present move to the field on the left if
 *            we are not in the left most field and delete the right
 *            most digit in that field while keeping the cursor
 *            on the right side of the field
 */
LRESULT CALLBACK
IPADDRESS_SubclassProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HWND Self = (HWND)GetPropW (hwnd, IP_SUBCLASS_PROP);
    IPADDRESS_INFO *infoPtr = (IPADDRESS_INFO *)GetWindowLongPtrW (Self, 0);
    CHAR c = (CHAR)wParam;
    INT index, len = 0, startsel, endsel;
    IPPART_INFO *part;

    TRACE("(hwnd=%p msg=0x%x wparam=0x%lx lparam=0x%lx)\n", hwnd, uMsg, wParam, lParam);

    if ( (index = IPADDRESS_GetPartIndex(infoPtr, hwnd)) < 0) return 0;
    part = &infoPtr->Part[index];

    if (uMsg == WM_CHAR || uMsg == WM_KEYDOWN) {
	len = GetWindowTextLengthW (hwnd);
	SendMessageW(hwnd, EM_GETSEL, (WPARAM)&startsel, (LPARAM)&endsel);
    }
    switch (uMsg) {
 	case WM_CHAR:
 	    if(isdigit(c)) {
		if(len == 2 && startsel==endsel && endsel==len) {
		    /* process the digit press before we check the field */
		    int return_val = CallWindowProcW (part->OrigProc, hwnd, uMsg, wParam, lParam);

		    /* if the field value was changed stay at the current field */
		    if(!IPADDRESS_ConstrainField(infoPtr, index))
			IPADDRESS_GotoNextField (infoPtr, index, POS_DEFAULT);

		    return return_val;
		} else if (len == 3 && startsel==endsel && endsel==len)
		    IPADDRESS_GotoNextField (infoPtr, index, POS_SELALL);
		else if (len < 3) break;
	    } else if(c == '.' || c == ' ') {
		if(len && startsel==endsel && startsel != 0) {
		    IPADDRESS_GotoNextField(infoPtr, index, POS_SELALL);
		}
 	    } else if (c == VK_BACK) break;
	    return 0;

	case WM_KEYDOWN:
	    switch(c) {
		case VK_RIGHT:
		    if(startsel==endsel && startsel==len) {
			IPADDRESS_GotoNextField(infoPtr, index, POS_LEFT);
			return 0;
		    }
		    break;
		case VK_LEFT:
		    if(startsel==0 && startsel==endsel && index > 0) {
			IPADDRESS_GotoNextField(infoPtr, index - 2, POS_RIGHT);
			return 0;
		    }
		    break;
		case VK_BACK:
		    if(startsel==endsel && startsel==0 && index > 0) {
			IPPART_INFO *prev = &infoPtr->Part[index-1];
			WCHAR val[10];

			if(GetWindowTextW(prev->EditHwnd, val, 5)) {
			    val[lstrlenW(val) - 1] = 0;
			    SetWindowTextW(prev->EditHwnd, val);
			}

			IPADDRESS_GotoNextField(infoPtr, index - 2, POS_RIGHT);
			return 0;
		    }
		    break;
	    }
	    break;
	case WM_KILLFOCUS:
	    if (IPADDRESS_GetPartIndex(infoPtr, (HWND)wParam) < 0)
		IPADDRESS_Notify(infoPtr, EN_KILLFOCUS);
	    break;
	case WM_SETFOCUS:
	    if (IPADDRESS_GetPartIndex(infoPtr, (HWND)wParam) < 0)
		IPADDRESS_Notify(infoPtr, EN_SETFOCUS);
	    break;
    }
    return CallWindowProcW (part->OrigProc, hwnd, uMsg, wParam, lParam);
}


static LRESULT WINAPI
IPADDRESS_WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    IPADDRESS_INFO *infoPtr = (IPADDRESS_INFO *)GetWindowLongPtrW (hwnd, 0);

    TRACE("(hwnd=%p msg=0x%x wparam=0x%lx lparam=0x%lx)\n", hwnd, uMsg, wParam, lParam);

    if (!infoPtr && (uMsg != WM_CREATE))
        return DefWindowProcW (hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
	case WM_CREATE:
	    return IPADDRESS_Create (hwnd, (LPCREATESTRUCTA)lParam);

	case WM_DESTROY:
	    return IPADDRESS_Destroy (infoPtr);

	case WM_ENABLE:
	    return IPADDRESS_Enable (infoPtr, (BOOL)wParam);

	case WM_PAINT:
	    return IPADDRESS_Paint (infoPtr, (HDC)wParam);

	case WM_COMMAND:
	    switch(wParam >> 16) {
		case EN_CHANGE:
		    IPADDRESS_Notify(infoPtr, EN_CHANGE);
		    break;
		case EN_KILLFOCUS:
		    IPADDRESS_ConstrainField(infoPtr, IPADDRESS_GetPartIndex(infoPtr, (HWND)lParam));
		    break;
	    }
	    break;

        case IPM_CLEARADDRESS:
            IPADDRESS_ClearAddress (infoPtr);
	    break;

        case IPM_SETADDRESS:
            return IPADDRESS_SetAddress (infoPtr, (DWORD)lParam);

        case IPM_GETADDRESS:
 	    return IPADDRESS_GetAddress (infoPtr, (LPDWORD)lParam);

	case IPM_SETRANGE:
	    return IPADDRESS_SetRange (infoPtr, (int)wParam, (WORD)lParam);

	case IPM_SETFOCUS:
	    IPADDRESS_SetFocusToField (infoPtr, (int)wParam);
	    break;

	case IPM_ISBLANK:
	    return IPADDRESS_IsBlank (infoPtr);

	default:
	    if ((uMsg >= WM_USER) && (uMsg < WM_APP))
		ERR("unknown msg %04x wp=%08lx lp=%08lx\n", uMsg, wParam, lParam);
	    return DefWindowProcW (hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


void IPADDRESS_Register (void)
{
    WNDCLASSW wndClass;

    ZeroMemory (&wndClass, sizeof(WNDCLASSW));
    wndClass.style         = CS_GLOBALCLASS | CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc   = IPADDRESS_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(IPADDRESS_INFO *);
    wndClass.hCursor       = LoadCursorW (0, (LPWSTR)IDC_IBEAM);
    wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wndClass.lpszClassName = WC_IPADDRESSW;

    RegisterClassW (&wndClass);
}


void IPADDRESS_Unregister (void)
{
    UnregisterClassW (WC_IPADDRESSW, NULL);
}
