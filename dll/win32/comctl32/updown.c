/*
 * Updown control
 *
 * Copyright 1997, 2002 Dimitrie O. Paun
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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "commctrl.h"
#include "comctl32.h"
#include "uxtheme.h"
#include "vssym32.h"
#include "wine/heap.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(updown);

typedef struct
{
    HWND      Self;            /* Handle to this up-down control */
    HWND      Notify;          /* Handle to the parent window */
    DWORD     dwStyle;         /* The GWL_STYLE for this window */
    UINT      AccelCount;      /* Number of elements in AccelVect */
    UDACCEL*  AccelVect;       /* Vector containing AccelCount elements */
    INT       AccelIndex;      /* Current accel index, -1 if not accel'ing */
    INT       Base;            /* Base to display nr in the buddy window */
    INT       CurVal;          /* Current up-down value */
    INT       MinVal;          /* Minimum up-down value */
    INT       MaxVal;          /* Maximum up-down value */
    HWND      Buddy;           /* Handle to the buddy window */
    INT       BuddyType;       /* Remembers the buddy type BUDDY_TYPE_* */
    INT       Flags;           /* Internal Flags FLAG_* */
    BOOL      UnicodeFormat;   /* Marks the use of Unicode internally */
} UPDOWN_INFO;

/* Control configuration constants */

#define INITIAL_DELAY	500    /* initial timer until auto-inc kicks in */
#define AUTOPRESS_DELAY	250    /* time to keep arrow pressed on KEY_DOWN */
#define REPEAT_DELAY	50     /* delay between auto-increments */

#define DEFAULT_WIDTH	    16 /* default width of the ctrl */
#define DEFAULT_XSEP         0 /* default separation between buddy and ctrl */
#define DEFAULT_ADDTOP       0 /* amount to extend above the buddy window */
#define DEFAULT_ADDBOT       0 /* amount to extend below the buddy window */
#define DEFAULT_BUDDYBORDER  2 /* Width/height of the buddy border */
#define DEFAULT_BUDDYSPACER  2 /* Spacer between the buddy and the ctrl */
#define DEFAULT_BUDDYBORDER_THEMED  1 /* buddy border when theming is enabled */
#define DEFAULT_BUDDYSPACER_THEMED  0 /* buddy spacer when theming is enabled */

/* Work constants */

#define FLAG_INCR	0x01
#define FLAG_DECR	0x02
#define FLAG_MOUSEIN	0x04
#define FLAG_PRESSED	0x08
#define FLAG_BUDDYINT	0x10 /* UDS_SETBUDDYINT was set on creation */
#define FLAG_ARROW	(FLAG_INCR | FLAG_DECR)

#define BUDDY_TYPE_UNKNOWN 0
#define BUDDY_TYPE_LISTBOX 1
#define BUDDY_TYPE_EDIT    2

#define TIMER_AUTOREPEAT   1
#define TIMER_ACCEL        2
#define TIMER_AUTOPRESS    3

#define UPDOWN_GetInfoPtr(hwnd) ((UPDOWN_INFO *)GetWindowLongPtrW (hwnd,0))

/* id used for SetWindowSubclass */
#define BUDDY_SUBCLASSID   1

static void UPDOWN_DoAction (UPDOWN_INFO *infoPtr, int delta, int action);

/***********************************************************************
 *           UPDOWN_IsBuddyEdit
 * Tests if our buddy is an edit control.
 */
static inline BOOL UPDOWN_IsBuddyEdit(const UPDOWN_INFO *infoPtr)
{
    return infoPtr->BuddyType == BUDDY_TYPE_EDIT;
}

/***********************************************************************
 *           UPDOWN_IsBuddyListbox
 * Tests if our buddy is a listbox control.
 */
static inline BOOL UPDOWN_IsBuddyListbox(const UPDOWN_INFO *infoPtr)
{
    return infoPtr->BuddyType == BUDDY_TYPE_LISTBOX;
}

/***********************************************************************
 *           UPDOWN_InBounds
 * Tests if a given value 'val' is between the Min&Max limits
 */
static BOOL UPDOWN_InBounds(const UPDOWN_INFO *infoPtr, int val)
{
    if(infoPtr->MaxVal > infoPtr->MinVal)
        return (infoPtr->MinVal <= val) && (val <= infoPtr->MaxVal);
    else
        return (infoPtr->MaxVal <= val) && (val <= infoPtr->MinVal);
}

/***********************************************************************
 *           UPDOWN_OffsetVal
 * Change the current value by delta.
 * It returns TRUE is the value was changed successfully, or FALSE
 * if the value was not changed, as it would go out of bounds.
 */
static BOOL UPDOWN_OffsetVal(UPDOWN_INFO *infoPtr, int delta)
{
    /* check if we can do the modification first */
    if(!UPDOWN_InBounds (infoPtr, infoPtr->CurVal+delta)) {
        if (infoPtr->dwStyle & UDS_WRAP) {
            delta += (delta < 0 ? -1 : 1) *
		     (infoPtr->MaxVal < infoPtr->MinVal ? -1 : 1) *
		     (infoPtr->MinVal - infoPtr->MaxVal) +
		     (delta < 0 ? 1 : -1);
        } else if ((infoPtr->MaxVal > infoPtr->MinVal && infoPtr->CurVal+delta > infoPtr->MaxVal)
                || (infoPtr->MaxVal < infoPtr->MinVal && infoPtr->CurVal+delta < infoPtr->MaxVal)) {
            delta = infoPtr->MaxVal - infoPtr->CurVal;
        } else {
            delta = infoPtr->MinVal - infoPtr->CurVal;
        }
    }

    infoPtr->CurVal += delta;
    return delta != 0;
}

/***********************************************************************
 * UPDOWN_HasBuddyBorder
 *
 * When we have a buddy set and that we are aligned on our buddy, we
 * want to draw a sunken edge to make like we are part of that control.
 */
static BOOL UPDOWN_HasBuddyBorder(const UPDOWN_INFO *infoPtr)
{
    return  ( ((infoPtr->dwStyle & (UDS_ALIGNLEFT | UDS_ALIGNRIGHT)) != 0) &&
	      UPDOWN_IsBuddyEdit(infoPtr) );
}

/***********************************************************************
 *           UPDOWN_GetArrowRect
 * wndPtr   - pointer to the up-down wnd
 * rect     - will hold the rectangle
 * arrow    - FLAG_INCR to get the "increment" rect (up or right)
 *            FLAG_DECR to get the "decrement" rect (down or left)
 */
static void UPDOWN_GetArrowRect (const UPDOWN_INFO* infoPtr, RECT *rect, unsigned int arrow)
{
    HTHEME theme = GetWindowTheme (infoPtr->Self);
    const int border = theme ? DEFAULT_BUDDYBORDER_THEMED : DEFAULT_BUDDYBORDER;
    const int spacer = theme ? DEFAULT_BUDDYSPACER_THEMED : DEFAULT_BUDDYSPACER;
    int size;

    assert(arrow && (arrow & (FLAG_INCR | FLAG_DECR)) != (FLAG_INCR | FLAG_DECR));

    GetClientRect (infoPtr->Self, rect);

    /*
     * Make sure we calculate the rectangle to fit even if we draw the
     * border.
     */
    if (UPDOWN_HasBuddyBorder(infoPtr)) {
        if (infoPtr->dwStyle & UDS_ALIGNLEFT)
            rect->left += border;
        else
            rect->right -= border;

        InflateRect(rect, 0, -border);
    }

    /* now figure out if we need a space away from the buddy */
    if (IsWindow(infoPtr->Buddy) ) {
	if (infoPtr->dwStyle & UDS_ALIGNLEFT) rect->right -= spacer;
	else if (infoPtr->dwStyle & UDS_ALIGNRIGHT) rect->left += spacer;
    }

    /*
     * We're calculating the midpoint to figure-out where the
     * separation between the buttons will lay.
     */
    if (infoPtr->dwStyle & UDS_HORZ) {
        size = (rect->right - rect->left) / 2;
        if (arrow & FLAG_INCR)
            rect->left = rect->right - size;
        else if (arrow & FLAG_DECR)
            rect->right = rect->left + size;
    } else {
        size = (rect->bottom - rect->top) / 2;
        if (arrow & FLAG_INCR)
            rect->bottom = rect->top + size;
        else if (arrow & FLAG_DECR)
            rect->top = rect->bottom - size;
    }
}

/***********************************************************************
 *           UPDOWN_GetArrowFromPoint
 * Returns the rectagle (for the up or down arrow) that contains pt.
 * If it returns the up rect, it returns FLAG_INCR.
 * If it returns the down rect, it returns FLAG_DECR.
 */
static INT UPDOWN_GetArrowFromPoint (const UPDOWN_INFO *infoPtr, RECT *rect, POINT pt)
{
    UPDOWN_GetArrowRect (infoPtr, rect, FLAG_INCR);
    if(PtInRect(rect, pt)) return FLAG_INCR;

    UPDOWN_GetArrowRect (infoPtr, rect, FLAG_DECR);
    if(PtInRect(rect, pt)) return FLAG_DECR;

    return 0;
}


/***********************************************************************
 *           UPDOWN_GetThousandSep
 * Returns the thousand sep. If an error occurs, it returns ','.
 */
static WCHAR UPDOWN_GetThousandSep(void)
{
    WCHAR sep[2];

    if(GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, sep, 2) != 1)
        sep[0] = ',';

    return sep[0];
}

/***********************************************************************
 *           UPDOWN_GetBuddyInt
 * Tries to read the pos from the buddy window and if it succeeds,
 * it stores it in the control's CurVal
 * returns:
 *   TRUE  - if it read the integer from the buddy successfully
 *   FALSE - if an error occurred
 */
static BOOL UPDOWN_GetBuddyInt (UPDOWN_INFO *infoPtr)
{
    WCHAR txt[20], sep, *src, *dst;
    int newVal;

    if (!((infoPtr->Flags & FLAG_BUDDYINT) && IsWindow(infoPtr->Buddy)))
        return FALSE;

    /*if the buddy is a list window, we must set curr index */
    if (UPDOWN_IsBuddyListbox(infoPtr)) {
        newVal = SendMessageW(infoPtr->Buddy, LB_GETCARETINDEX, 0, 0);
        if(newVal < 0) return FALSE;
    } else {
        /* we have a regular window, so will get the text */
        /* note that a zero-length string is a legitimate value for 'txt',
         * and ought to result in a successful conversion to '0'. */
        if (GetWindowTextW(infoPtr->Buddy, txt, ARRAY_SIZE(txt)) < 0)
            return FALSE;

        sep = UPDOWN_GetThousandSep();

        /* now get rid of the separators */
        for(src = dst = txt; *src; src++)
            if(*src != sep) *dst++ = *src;
        *dst = 0;

        /* try to convert the number and validate it */
        newVal = wcstol(txt, &src, infoPtr->Base);
        if(*src || !UPDOWN_InBounds (infoPtr, newVal)) return FALSE;
    }

    TRACE("new value(%d) from buddy (old=%d)\n", newVal, infoPtr->CurVal);
    infoPtr->CurVal = newVal;
    return TRUE;
}


/***********************************************************************
 *           UPDOWN_SetBuddyInt
 * Tries to set the pos to the buddy window based on current pos
 * returns:
 *   TRUE  - if it set the caption of the  buddy successfully
 *   FALSE - if an error occurred
 */
static BOOL UPDOWN_SetBuddyInt (const UPDOWN_INFO *infoPtr)
{
    static const WCHAR fmt_hex[] = { '0', 'x', '%', '0', '4', 'X', 0 };
    static const WCHAR fmt_dec_oct[] = { '%', 'd', '\0' };
    const WCHAR *fmt;
    WCHAR txt[20], txt_old[20] = { 0 };
    int len;

    if (!((infoPtr->Flags & FLAG_BUDDYINT) && IsWindow(infoPtr->Buddy)))
        return FALSE;

    TRACE("set new value(%d) to buddy.\n", infoPtr->CurVal);

    /*if the buddy is a list window, we must set curr index */
    if (UPDOWN_IsBuddyListbox(infoPtr)) {
        return SendMessageW(infoPtr->Buddy, LB_SETCURSEL, infoPtr->CurVal, 0) != LB_ERR;
    }

    /* Regular window, so set caption to the number */
    fmt = (infoPtr->Base == 16) ? fmt_hex : fmt_dec_oct;
    len = wsprintfW(txt, fmt, infoPtr->CurVal);


    /* Do thousands separation if necessary */
    if ((infoPtr->Base == 10) && !(infoPtr->dwStyle & UDS_NOTHOUSANDS) && (len > 3)) {
        WCHAR tmp[ARRAY_SIZE(txt)], *src = tmp, *dst = txt;
        WCHAR sep = UPDOWN_GetThousandSep();
	int start = len % 3;

	memcpy(tmp, txt, sizeof(txt));
	if (start == 0) start = 3;
	dst += start;
	src += start;
        for (len=0; *src; len++) {
	    if (len % 3 == 0) *dst++ = sep;
	    *dst++ = *src++;
        }
        *dst = 0;
    }

    /* if nothing changed exit earlier */
    GetWindowTextW(infoPtr->Buddy, txt_old, ARRAY_SIZE(txt_old));
    if (lstrcmpiW(txt_old, txt) == 0) return FALSE;

    return SetWindowTextW(infoPtr->Buddy, txt);
}

/***********************************************************************
 * UPDOWN_DrawBuddyBackground
 *
 * Draw buddy background for visual integration.
 */
static BOOL UPDOWN_DrawBuddyBackground (const UPDOWN_INFO *infoPtr, HDC hdc)
{
    RECT br, r;
    HTHEME buddyTheme = GetWindowTheme (infoPtr->Buddy);
    if (!buddyTheme) return FALSE;

    GetWindowRect (infoPtr->Buddy, &br);
    MapWindowPoints (NULL, infoPtr->Self, (POINT*)&br, 2);
    GetClientRect (infoPtr->Self, &r);

    if (infoPtr->dwStyle & UDS_ALIGNLEFT)
        br.left = r.left;
    else if (infoPtr->dwStyle & UDS_ALIGNRIGHT)
        br.right = r.right;
    /* FIXME: take disabled etc. into account */
    DrawThemeBackground (buddyTheme, hdc, 0, 0, &br, NULL);
    return TRUE;
}

/***********************************************************************
 * UPDOWN_Draw
 *
 * Draw the arrows. The background need not be erased.
 */
static LRESULT UPDOWN_Draw (const UPDOWN_INFO *infoPtr, HDC hdc)
{
    BOOL uPressed, uHot, dPressed, dHot;
    RECT rect;
    HTHEME theme = GetWindowTheme (infoPtr->Self);
    int uPart = 0, uState = 0, dPart = 0, dState = 0;
    BOOL needBuddyBg = FALSE;

    uPressed = (infoPtr->Flags & FLAG_PRESSED) && (infoPtr->Flags & FLAG_INCR);
    uHot = (infoPtr->Flags & FLAG_INCR) && (infoPtr->Flags & FLAG_MOUSEIN);
    dPressed = (infoPtr->Flags & FLAG_PRESSED) && (infoPtr->Flags & FLAG_DECR);
    dHot = (infoPtr->Flags & FLAG_DECR) && (infoPtr->Flags & FLAG_MOUSEIN);
    if (theme) {
        uPart = (infoPtr->dwStyle & UDS_HORZ) ? SPNP_UPHORZ : SPNP_UP;
        uState = (infoPtr->dwStyle & WS_DISABLED) ? DNS_DISABLED 
            : (uPressed ? DNS_PRESSED : (uHot ? DNS_HOT : DNS_NORMAL));
        dPart = (infoPtr->dwStyle & UDS_HORZ) ? SPNP_DOWNHORZ : SPNP_DOWN;
        dState = (infoPtr->dwStyle & WS_DISABLED) ? DNS_DISABLED 
            : (dPressed ? DNS_PRESSED : (dHot ? DNS_HOT : DNS_NORMAL));
        needBuddyBg = IsWindow (infoPtr->Buddy)
            && (IsThemeBackgroundPartiallyTransparent (theme, uPart, uState)
              || IsThemeBackgroundPartiallyTransparent (theme, dPart, dState));
    }

    /* Draw the common border between ourselves and our buddy */
    if (UPDOWN_HasBuddyBorder(infoPtr) || needBuddyBg) {
        if (!theme || !UPDOWN_DrawBuddyBackground (infoPtr, hdc)) {
            GetClientRect(infoPtr->Self, &rect);
	    DrawEdge(hdc, &rect, EDGE_SUNKEN,
		     BF_BOTTOM | BF_TOP |
		     (infoPtr->dwStyle & UDS_ALIGNLEFT ? BF_LEFT : BF_RIGHT));
        }
    }

    /* Draw the incr button */
    UPDOWN_GetArrowRect (infoPtr, &rect, FLAG_INCR);
    if (theme) {
        DrawThemeBackground(theme, hdc, uPart, uState, &rect, NULL);
    } else {
        DrawFrameControl(hdc, &rect, DFC_SCROLL,
            (infoPtr->dwStyle & UDS_HORZ ? DFCS_SCROLLRIGHT : DFCS_SCROLLUP) |
            ((infoPtr->dwStyle & UDS_HOTTRACK) && uHot ? DFCS_HOT : 0) |
            (uPressed ? DFCS_PUSHED : 0) |
            (infoPtr->dwStyle & WS_DISABLED ? DFCS_INACTIVE : 0) );
    }

    /* Draw the decr button */
    UPDOWN_GetArrowRect(infoPtr, &rect, FLAG_DECR);
    if (theme) {
        DrawThemeBackground(theme, hdc, dPart, dState, &rect, NULL);
    } else {
        DrawFrameControl(hdc, &rect, DFC_SCROLL,
            (infoPtr->dwStyle & UDS_HORZ ? DFCS_SCROLLLEFT : DFCS_SCROLLDOWN) |
            ((infoPtr->dwStyle & UDS_HOTTRACK) && dHot ? DFCS_HOT : 0) |
            (dPressed ? DFCS_PUSHED : 0) |
            (infoPtr->dwStyle & WS_DISABLED ? DFCS_INACTIVE : 0) );
    }

    return 0;
}

/***********************************************************************
 * UPDOWN_Paint
 *
 * Asynchronous drawing (must ONLY be used in WM_PAINT).
 * Calls UPDOWN_Draw.
 */
static LRESULT UPDOWN_Paint (const UPDOWN_INFO *infoPtr, HDC hdc)
{
    PAINTSTRUCT ps;
    if (hdc) return UPDOWN_Draw (infoPtr, hdc);
    hdc = BeginPaint (infoPtr->Self, &ps);
    UPDOWN_Draw (infoPtr, hdc);
    EndPaint (infoPtr->Self, &ps);
    return 0;
}

/***********************************************************************
 * UPDOWN_KeyPressed
 *
 * Handle key presses (up & down) when we have to do so
 */
static LRESULT UPDOWN_KeyPressed(UPDOWN_INFO *infoPtr, int key)
{
    int arrow, accel;

    if (key == VK_UP) arrow = FLAG_INCR;
    else if (key == VK_DOWN) arrow = FLAG_DECR;
    else return 1;

    UPDOWN_GetBuddyInt (infoPtr);
    infoPtr->Flags &= ~FLAG_ARROW;
    infoPtr->Flags |= FLAG_PRESSED | arrow;
    InvalidateRect (infoPtr->Self, NULL, FALSE);
    SetTimer(infoPtr->Self, TIMER_AUTOPRESS, AUTOPRESS_DELAY, 0);
    accel = (infoPtr->AccelCount && infoPtr->AccelVect) ? infoPtr->AccelVect[0].nInc : 1;
    UPDOWN_DoAction (infoPtr, accel, arrow);
    return 0;
}

static int UPDOWN_GetPos(UPDOWN_INFO *infoPtr, BOOL *err)
{
    BOOL succ = UPDOWN_GetBuddyInt(infoPtr);
    int val = infoPtr->CurVal;

    if(!UPDOWN_InBounds(infoPtr, val)) {
        if((infoPtr->MinVal < infoPtr->MaxVal && val < infoPtr->MinVal)
                || (infoPtr->MinVal > infoPtr->MaxVal && val > infoPtr->MinVal))
            val = infoPtr->MinVal;
        else
            val = infoPtr->MaxVal;

        succ = FALSE;
    }

    if(err) *err = !succ;
    return val;
}

static int UPDOWN_SetPos(UPDOWN_INFO *infoPtr, int pos)
{
    int ret = infoPtr->CurVal;

    if(!UPDOWN_InBounds(infoPtr, pos)) {
        if((infoPtr->MinVal < infoPtr->MaxVal && pos < infoPtr->MinVal)
                || (infoPtr->MinVal > infoPtr->MaxVal && pos > infoPtr->MinVal))
            pos = infoPtr->MinVal;
        else
            pos = infoPtr->MaxVal;
    }

    infoPtr->CurVal = pos;
    UPDOWN_SetBuddyInt(infoPtr);

    if(!UPDOWN_InBounds(infoPtr, ret)) {
        if((infoPtr->MinVal < infoPtr->MaxVal && ret < infoPtr->MinVal)
                || (infoPtr->MinVal > infoPtr->MaxVal && ret > infoPtr->MinVal))
            ret = infoPtr->MinVal;
        else
            ret = infoPtr->MaxVal;
    }
    return ret;
}


/***********************************************************************
 * UPDOWN_SetRange
 *
 * Handle UDM_SETRANGE, UDM_SETRANGE32
 *
 * FIXME: handle Max == Min properly:
 *        - arrows should be disabled (without WS_DISABLED set),
 *          visually they can't be pressed and don't respond;
 *        - all input messages should still pass in.
 */
static LRESULT UPDOWN_SetRange(UPDOWN_INFO *infoPtr, INT Max, INT Min)
{
    infoPtr->MaxVal = Max;
    infoPtr->MinVal = Min;

    TRACE("UpDown Ctrl new range(%d to %d), hwnd=%p\n",
           infoPtr->MinVal, infoPtr->MaxVal, infoPtr->Self);

    return 0;
}

/***********************************************************************
 * UPDOWN_MouseWheel
 *
 * Handle mouse wheel scrolling
 */
static LRESULT UPDOWN_MouseWheel(UPDOWN_INFO *infoPtr, WPARAM wParam)
{
    int iWheelDelta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;

    if (wParam & (MK_SHIFT | MK_CONTROL))
        return 0;

    if (iWheelDelta != 0)
    {
        UPDOWN_GetBuddyInt(infoPtr);
        UPDOWN_DoAction(infoPtr, abs(iWheelDelta), iWheelDelta > 0 ? FLAG_INCR : FLAG_DECR);
    }

    return 1;
}


/***********************************************************************
 * UPDOWN_Buddy_SubclassProc used to handle messages sent to the buddy
 *                           control.
 */
static LRESULT CALLBACK
UPDOWN_Buddy_SubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
                          UINT_PTR uId, DWORD_PTR ref_data)
{
    UPDOWN_INFO *infoPtr = UPDOWN_GetInfoPtr((HWND)ref_data);

    TRACE("hwnd=%p, uMsg=%04x, wParam=%08lx, lParam=%08lx\n",
          hwnd, uMsg, wParam, lParam);

    switch(uMsg)
    {
    case WM_KEYDOWN:
        if (infoPtr)
        {
            UPDOWN_KeyPressed(infoPtr, (int)wParam);
            if (wParam == VK_UP || wParam == VK_DOWN)
                return 0;
        }
        break;

    case WM_MOUSEWHEEL:
        if (infoPtr)
            UPDOWN_MouseWheel(infoPtr, (int)wParam);
        break;

    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, UPDOWN_Buddy_SubclassProc, BUDDY_SUBCLASSID);
        break;
    default:
	break;
    }

    return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

static void UPDOWN_ResetSubclass (UPDOWN_INFO *infoPtr)
{
    SetWindowSubclass(infoPtr->Buddy, UPDOWN_Buddy_SubclassProc, BUDDY_SUBCLASSID, 0);
}

/***********************************************************************
 *           UPDOWN_SetBuddy
 *
 * Sets bud as a new Buddy.
 * Then, it should subclass the buddy
 * If window has the UDS_ARROWKEYS, it subclasses the buddy window to
 * process the UP/DOWN arrow keys.
 * If window has the UDS_ALIGNLEFT or UDS_ALIGNRIGHT style
 * the size/pos of the buddy and the control are adjusted accordingly.
 */
static HWND UPDOWN_SetBuddy (UPDOWN_INFO* infoPtr, HWND bud)
{
    RECT  budRect;  /* new coord for the buddy */
    int   x, width;  /* new x position and width for the up-down */
    WCHAR buddyClass[40];
    HWND old_buddy;

    TRACE("(hwnd=%p, bud=%p)\n", infoPtr->Self, bud);

    old_buddy = infoPtr->Buddy;

    UPDOWN_ResetSubclass (infoPtr);

    if (!IsWindow(bud)) bud = NULL;

    /* Store buddy window handle */
    infoPtr->Buddy = bud;

    if(bud) {
        /* Store buddy window class type */
        infoPtr->BuddyType = BUDDY_TYPE_UNKNOWN;
        if (GetClassNameW(bud, buddyClass, ARRAY_SIZE(buddyClass))) {
            if (lstrcmpiW(buddyClass, WC_EDITW) == 0)
                infoPtr->BuddyType = BUDDY_TYPE_EDIT;
            else if (lstrcmpiW(buddyClass, WC_LISTBOXW) == 0)
                infoPtr->BuddyType = BUDDY_TYPE_LISTBOX;
        }

        if (infoPtr->dwStyle & UDS_ARROWKEYS)
            SetWindowSubclass(bud, UPDOWN_Buddy_SubclassProc, BUDDY_SUBCLASSID,
                              (DWORD_PTR)infoPtr->Self);

        /* Get the rect of the buddy relative to its parent */
        GetWindowRect(infoPtr->Buddy, &budRect);
        MapWindowPoints(HWND_DESKTOP, GetParent(infoPtr->Buddy), (POINT *)(&budRect.left), 2);

        /* now do the positioning */
        if  (infoPtr->dwStyle & UDS_ALIGNLEFT) {
            x  = budRect.left;
            budRect.left += DEFAULT_WIDTH + DEFAULT_XSEP;
        } else if (infoPtr->dwStyle & UDS_ALIGNRIGHT) {
            budRect.right -= DEFAULT_WIDTH + DEFAULT_XSEP;
            x  = budRect.right+DEFAULT_XSEP;
        } else {
            /* nothing to do */
            return old_buddy;
        }

        /* first adjust the buddy to accommodate the up/down */
        SetWindowPos(infoPtr->Buddy, 0, budRect.left, budRect.top,
                     budRect.right  - budRect.left, budRect.bottom - budRect.top,
                     SWP_NOACTIVATE|SWP_NOZORDER);

        /* now position the up/down */
        /* Since the UDS_ALIGN* flags were used, */
        /* we will pick the position and size of the window. */
        width = DEFAULT_WIDTH;

        /*
         * If the updown has a buddy border, it has to overlap with the buddy
         * to look as if it is integrated with the buddy control.
         * We nudge the control or change its size to overlap.
         */
        if (UPDOWN_HasBuddyBorder(infoPtr)) {
            if(infoPtr->dwStyle & UDS_ALIGNLEFT)
                width += DEFAULT_BUDDYBORDER;
            else
                x -= DEFAULT_BUDDYBORDER;
        }

        SetWindowPos(infoPtr->Self, 0, x,
                     budRect.top - DEFAULT_ADDTOP, width,
                     budRect.bottom - budRect.top + DEFAULT_ADDTOP + DEFAULT_ADDBOT,
                     SWP_NOACTIVATE|SWP_FRAMECHANGED|SWP_NOZORDER);
    } else if (!(infoPtr->dwStyle & UDS_HORZ) && old_buddy != NULL) {
        RECT rect;
        GetWindowRect(infoPtr->Self, &rect);
        MapWindowPoints(HWND_DESKTOP, GetParent(infoPtr->Self), (POINT *)&rect, 2);
        SetWindowPos(infoPtr->Self, 0, rect.left, rect.top, DEFAULT_WIDTH, rect.bottom - rect.top,
                     SWP_NOACTIVATE|SWP_FRAMECHANGED|SWP_NOZORDER);
    }

    return old_buddy;
}

/***********************************************************************
 *           UPDOWN_DoAction
 *
 * This function increments/decrements the CurVal by the
 * 'delta' amount according to the 'action' flag which can be a
 * combination of FLAG_INCR and FLAG_DECR
 * It notifies the parent as required.
 * It handles wrapping and non-wrapping correctly.
 * It is assumed that delta>0
 */
static void UPDOWN_DoAction (UPDOWN_INFO *infoPtr, int delta, int action)
{
    NM_UPDOWN ni;

    TRACE("%d by %d\n", action, delta);

    /* check if we can do the modification first */
    delta *= (action & FLAG_INCR ? 1 : -1) * (infoPtr->MaxVal < infoPtr->MinVal ? -1 : 1);
    if ( (action & FLAG_INCR) && (action & FLAG_DECR) ) delta = 0;

    TRACE("current %d, delta: %d\n", infoPtr->CurVal, delta);

    /* We must notify parent now to obtain permission */
    ni.iPos = infoPtr->CurVal;
    ni.iDelta = delta;
    ni.hdr.hwndFrom = infoPtr->Self;
    ni.hdr.idFrom   = GetWindowLongPtrW (infoPtr->Self, GWLP_ID);
    ni.hdr.code = UDN_DELTAPOS;
    if (!SendMessageW(infoPtr->Notify, WM_NOTIFY, ni.hdr.idFrom, (LPARAM)&ni)) {
        /* Parent said: OK to adjust */

        /* Now adjust value with (maybe new) delta */
        if (UPDOWN_OffsetVal (infoPtr, ni.iDelta)) {
            TRACE("new %d, delta: %d\n", infoPtr->CurVal, ni.iDelta);

            /* Now take care about our buddy */
            UPDOWN_SetBuddyInt (infoPtr);
        }
    }

    /* Also, notify it. This message is sent in any case. */
    SendMessageW( infoPtr->Notify, (infoPtr->dwStyle & UDS_HORZ) ? WM_HSCROLL : WM_VSCROLL,
		  MAKELONG(SB_THUMBPOSITION, infoPtr->CurVal), (LPARAM)infoPtr->Self);
}

/***********************************************************************
 *           UPDOWN_IsEnabled
 *
 * Returns TRUE if it is enabled as well as its buddy (if any)
 *         FALSE otherwise
 */
static BOOL UPDOWN_IsEnabled (const UPDOWN_INFO *infoPtr)
{
    if (!IsWindowEnabled(infoPtr->Self))
        return FALSE;
    if(infoPtr->Buddy)
        return IsWindowEnabled(infoPtr->Buddy);
    return TRUE;
}

/***********************************************************************
 *           UPDOWN_CancelMode
 *
 * Deletes any timers, releases the mouse and does  redraw if necessary.
 * If the control is not in "capture" mode, it does nothing.
 * If the control was not in cancel mode, it returns FALSE.
 * If the control was in cancel mode, it returns TRUE.
 */
static BOOL UPDOWN_CancelMode (UPDOWN_INFO *infoPtr)
{
    if (!(infoPtr->Flags & FLAG_PRESSED)) return FALSE;

    KillTimer (infoPtr->Self, TIMER_AUTOREPEAT);
    KillTimer (infoPtr->Self, TIMER_ACCEL);
    KillTimer (infoPtr->Self, TIMER_AUTOPRESS);

    if (GetCapture() == infoPtr->Self)
        ReleaseCapture();

    infoPtr->Flags &= ~FLAG_PRESSED;
    InvalidateRect (infoPtr->Self, NULL, FALSE);

    return TRUE;
}

/***********************************************************************
 *           UPDOWN_HandleMouseEvent
 *
 * Handle a mouse event for the updown.
 * 'pt' is the location of the mouse event in client or
 * windows coordinates.
 */
static void UPDOWN_HandleMouseEvent (UPDOWN_INFO *infoPtr, UINT msg, INT x, INT y)
{
    POINT pt = { x, y };
    RECT rect;
    int temp, arrow;
    TRACKMOUSEEVENT tme;

    TRACE("msg %04x point %s\n", msg, wine_dbgstr_point(&pt));

    switch(msg)
    {
        case WM_LBUTTONDOWN:  /* Initialise mouse tracking */

            /* If the buddy is an edit, will set focus to it */
	    if (UPDOWN_IsBuddyEdit(infoPtr)) SetFocus(infoPtr->Buddy);

            /* Now see which one is the 'active' arrow */
            arrow = UPDOWN_GetArrowFromPoint (infoPtr, &rect, pt);

            /* Update the flags if we are in/out */
            infoPtr->Flags &= ~(FLAG_MOUSEIN | FLAG_ARROW);
            if (arrow)
                infoPtr->Flags |= FLAG_MOUSEIN | arrow;
            else
                if (infoPtr->AccelIndex != -1) infoPtr->AccelIndex = 0;

	    if (infoPtr->Flags & FLAG_ARROW) {

            	/* Update the CurVal if necessary */
            	UPDOWN_GetBuddyInt (infoPtr);

            	/* Set up the correct flags */
            	infoPtr->Flags |= FLAG_PRESSED;

            	/* repaint the control */
	    	InvalidateRect (infoPtr->Self, NULL, FALSE);

            	/* process the click */
		temp = (infoPtr->AccelCount && infoPtr->AccelVect) ? infoPtr->AccelVect[0].nInc : 1;
            	UPDOWN_DoAction (infoPtr, temp, infoPtr->Flags & FLAG_ARROW);

            	/* now capture all mouse messages */
            	SetCapture (infoPtr->Self);

            	/* and startup the first timer */
            	SetTimer(infoPtr->Self, TIMER_AUTOREPEAT, INITIAL_DELAY, 0);
	    }
            break;

	case WM_MOUSEMOVE:
            /* save the flags to see if any got modified */
            temp = infoPtr->Flags;

            /* Now see which one is the 'active' arrow */
            arrow = UPDOWN_GetArrowFromPoint (infoPtr, &rect, pt);

            /* Update the flags if we are in/out */
	    infoPtr->Flags &= ~(FLAG_MOUSEIN | FLAG_ARROW);
            if(arrow) {
	        infoPtr->Flags |=  FLAG_MOUSEIN | arrow;
            } else {
	        if(infoPtr->AccelIndex != -1) infoPtr->AccelIndex = 0;
            }

            /* If state changed, redraw the control */
            if(temp != infoPtr->Flags)
		 InvalidateRect (infoPtr->Self, NULL, FALSE);

            /* Set up tracking so the mousein flags can be reset when the 
             * mouse leaves the control */
            tme.cbSize = sizeof( tme );
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = infoPtr->Self;
            TrackMouseEvent (&tme);

            break;
        case WM_MOUSELEAVE:
	    infoPtr->Flags &= ~(FLAG_MOUSEIN | FLAG_ARROW);
            InvalidateRect (infoPtr->Self, NULL, FALSE);
            break;

	default:
	    ERR("Impossible case (msg=%x)!\n", msg);
    }

}

/***********************************************************************
 *           UpDownWndProc
 */
static LRESULT WINAPI UpDownWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    UPDOWN_INFO *infoPtr = UPDOWN_GetInfoPtr (hwnd);
    static const WCHAR themeClass[] = {'S','p','i','n',0};
    HTHEME theme;

    TRACE("hwnd=%p msg=%04x wparam=%08lx lparam=%08lx\n", hwnd, message, wParam, lParam);

    if (!infoPtr && (message != WM_CREATE))
        return DefWindowProcW (hwnd, message, wParam, lParam);

    switch(message)
    {
        case WM_CREATE:
	    {
	    CREATESTRUCTW *pcs = (CREATESTRUCTW*)lParam;

            infoPtr = heap_alloc_zero(sizeof(*infoPtr));
	    SetWindowLongPtrW (hwnd, 0, (DWORD_PTR)infoPtr);

	    /* initialize the info struct */
	    infoPtr->Self = hwnd;
	    infoPtr->Notify  = pcs->hwndParent;
	    infoPtr->dwStyle = pcs->style;
	    infoPtr->AccelCount = 0;
	    infoPtr->AccelVect = 0;
	    infoPtr->AccelIndex = -1;
	    infoPtr->CurVal = 0;
	    infoPtr->MinVal = 100;
	    infoPtr->MaxVal = 0;
	    infoPtr->Base  = 10; /* Default to base 10  */
	    infoPtr->Buddy = 0;  /* No buddy window yet */
	    infoPtr->Flags = (infoPtr->dwStyle & UDS_SETBUDDYINT) ? FLAG_BUDDYINT : 0;

            SetWindowLongW (hwnd, GWL_STYLE, infoPtr->dwStyle & ~WS_BORDER);
	    if (!(infoPtr->dwStyle & UDS_HORZ))
	        SetWindowPos (hwnd, NULL, 0, 0, DEFAULT_WIDTH, pcs->cy,
	                      SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOMOVE);

            /* Do we pick the buddy win ourselves? */
	    if (infoPtr->dwStyle & UDS_AUTOBUDDY)
		UPDOWN_SetBuddy (infoPtr, GetWindow (hwnd, GW_HWNDPREV));

	    OpenThemeData (hwnd, themeClass);

	    TRACE("UpDown Ctrl creation, hwnd=%p\n", hwnd);
	    }
	    break;

	case WM_DESTROY:
	    heap_free (infoPtr->AccelVect);
            UPDOWN_ResetSubclass (infoPtr);
	    heap_free (infoPtr);
	    SetWindowLongPtrW (hwnd, 0, 0);
            theme = GetWindowTheme (hwnd);
            CloseThemeData (theme);
	    TRACE("UpDown Ctrl destruction, hwnd=%p\n", hwnd);
	    break;

	case WM_ENABLE:
	    if (wParam) {
		infoPtr->dwStyle &= ~WS_DISABLED;
	    } else {
		infoPtr->dwStyle |= WS_DISABLED;
	    	UPDOWN_CancelMode (infoPtr);
	    }
	    InvalidateRect (infoPtr->Self, NULL, FALSE);
	    break;

        case WM_STYLECHANGED:
            if (wParam == GWL_STYLE) {
                infoPtr->dwStyle = ((LPSTYLESTRUCT)lParam)->styleNew;
	        InvalidateRect (infoPtr->Self, NULL, FALSE);
            }
            break;

        case WM_THEMECHANGED:
            theme = GetWindowTheme (hwnd);
            CloseThemeData (theme);
            OpenThemeData (hwnd, themeClass);
            InvalidateRect (hwnd, NULL, FALSE);
            break;

	case WM_TIMER:
	   /* is this the auto-press timer? */
	   if(wParam == TIMER_AUTOPRESS) {
		KillTimer(hwnd, TIMER_AUTOPRESS);
		infoPtr->Flags &= ~(FLAG_PRESSED | FLAG_ARROW);
		InvalidateRect(infoPtr->Self, NULL, FALSE);
	   }

	   /* if initial timer, kill it and start the repeat timer */
  	   if(wParam == TIMER_AUTOREPEAT) {
		INT delay;

		KillTimer(hwnd, TIMER_AUTOREPEAT);
		/* if no accel info given, used default timer */
		if(infoPtr->AccelCount==0 || infoPtr->AccelVect==0) {
		    infoPtr->AccelIndex = -1;
		    delay = REPEAT_DELAY;
		} else {
		    infoPtr->AccelIndex = 0; /* otherwise, use it */
		    delay = infoPtr->AccelVect[infoPtr->AccelIndex].nSec * 1000 + 1;
		}
		SetTimer(hwnd, TIMER_ACCEL, delay, 0);
      	    }

	    /* now, if the mouse is above us, do the thing...*/
	    if(infoPtr->Flags & FLAG_MOUSEIN) {
		int temp;

		temp = infoPtr->AccelIndex == -1 ? 1 : infoPtr->AccelVect[infoPtr->AccelIndex].nInc;
		UPDOWN_DoAction(infoPtr, temp, infoPtr->Flags & FLAG_ARROW);

		if(infoPtr->AccelIndex != -1 && infoPtr->AccelIndex < infoPtr->AccelCount-1) {
		    KillTimer(hwnd, TIMER_ACCEL);
		    infoPtr->AccelIndex++; /* move to the next accel info */
		    temp = infoPtr->AccelVect[infoPtr->AccelIndex].nSec * 1000 + 1;
	  	    /* make sure we have at least 1ms intervals */
		    SetTimer(hwnd, TIMER_ACCEL, temp, 0);
		}
	    }
	    break;

	case WM_CANCELMODE:
	  return UPDOWN_CancelMode (infoPtr);

	case WM_LBUTTONUP:
	    if (GetCapture() != infoPtr->Self) break;

	    if ( (infoPtr->Flags & FLAG_MOUSEIN) &&
		 (infoPtr->Flags & FLAG_ARROW) ) {

	    	SendMessageW( infoPtr->Notify,
			      (infoPtr->dwStyle & UDS_HORZ) ? WM_HSCROLL : WM_VSCROLL,
                  	      MAKELONG(SB_ENDSCROLL, infoPtr->CurVal),
			      (LPARAM)hwnd);
		if (UPDOWN_IsBuddyEdit(infoPtr))
		    SendMessageW(infoPtr->Buddy, EM_SETSEL, 0, MAKELONG(0, -1));
	    }
	    UPDOWN_CancelMode(infoPtr);
	    break;

	case WM_LBUTTONDOWN:
	case WM_MOUSEMOVE:
        case WM_MOUSELEAVE:
	    if(UPDOWN_IsEnabled(infoPtr))
		UPDOWN_HandleMouseEvent (infoPtr, message, (SHORT)LOWORD(lParam), (SHORT)HIWORD(lParam));
	    break;

        case WM_MOUSEWHEEL:
            UPDOWN_MouseWheel(infoPtr, wParam);
            break;

	case WM_KEYDOWN:
	    if((infoPtr->dwStyle & UDS_ARROWKEYS) && UPDOWN_IsEnabled(infoPtr))
		return UPDOWN_KeyPressed(infoPtr, (int)wParam);
	    break;

	case WM_PRINTCLIENT:
	case WM_PAINT:
	    return UPDOWN_Paint (infoPtr, (HDC)wParam);

	case UDM_GETACCEL:
	    if (wParam==0 && lParam==0) return infoPtr->AccelCount;
	    if (wParam && lParam) {
		int temp = min(infoPtr->AccelCount, wParam);
	        memcpy((void *)lParam, infoPtr->AccelVect, temp*sizeof(UDACCEL));
	        return temp;
      	    }
	    return 0;

	case UDM_SETACCEL:
	{
	    TRACE("UDM_SETACCEL\n");

	    if(infoPtr->AccelVect) {
		heap_free (infoPtr->AccelVect);
		infoPtr->AccelCount = 0;
		infoPtr->AccelVect  = 0;
      	    }
	    if(wParam==0) return TRUE;
	    infoPtr->AccelVect = heap_alloc(wParam*sizeof(UDACCEL));
	    if(!infoPtr->AccelVect) return FALSE;
	    memcpy(infoPtr->AccelVect, (void*)lParam, wParam*sizeof(UDACCEL));
            infoPtr->AccelCount = wParam;

            if (TRACE_ON(updown))
            {
                UINT i;

                for (i = 0; i < wParam; i++)
                    TRACE("%u: nSec %u nInc %u\n", i,
                        infoPtr->AccelVect[i].nSec, infoPtr->AccelVect[i].nInc);
            }

    	    return TRUE;
	}
	case UDM_GETBASE:
	    return infoPtr->Base;

	case UDM_SETBASE:
	    TRACE("UpDown Ctrl new base(%ld), hwnd=%p\n", wParam, hwnd);
	    if (wParam==10 || wParam==16) {
		WPARAM old_base = infoPtr->Base;
		infoPtr->Base = wParam;

		if (old_base != infoPtr->Base)
		    UPDOWN_SetBuddyInt(infoPtr);

		return old_base;
	    }
	    break;

	case UDM_GETBUDDY:
	    return (LRESULT)infoPtr->Buddy;

	case UDM_SETBUDDY:
	    return (LRESULT)UPDOWN_SetBuddy (infoPtr, (HWND)wParam);

	case UDM_GETPOS:
	{
            BOOL err;
            int pos;

            pos = UPDOWN_GetPos(infoPtr, &err);
            return MAKELONG(pos, err);
	}
	case UDM_SETPOS:
	{
            return UPDOWN_SetPos(infoPtr, (short)LOWORD(lParam));
	}
	case UDM_GETRANGE:
	    return MAKELONG(infoPtr->MaxVal, infoPtr->MinVal);

	case UDM_SETRANGE:
	    /* we must have:
	    UD_MINVAL <= Max <= UD_MAXVAL
	    UD_MINVAL <= Min <= UD_MAXVAL
	    |Max-Min| <= UD_MAXVAL */
	    UPDOWN_SetRange(infoPtr, (short)lParam, (short)HIWORD(lParam));
	    break;

	case UDM_GETRANGE32:
	    if (wParam) *(LPINT)wParam = infoPtr->MinVal;
	    if (lParam) *(LPINT)lParam = infoPtr->MaxVal;
	    break;

	case UDM_SETRANGE32:
	    UPDOWN_SetRange(infoPtr, (INT)lParam, (INT)wParam);
	    break;

	case UDM_GETPOS32:
	{
            return UPDOWN_GetPos(infoPtr, (BOOL*)lParam);
	}
	case UDM_SETPOS32:
	{
            return UPDOWN_SetPos(infoPtr, (int)lParam);
	}
	case UDM_GETUNICODEFORMAT:
	    /* we lie a bit here, we're always using Unicode internally */
	    return infoPtr->UnicodeFormat;

	case UDM_SETUNICODEFORMAT:
	{
	    /* do we really need to honour this flag? */
	    int temp = infoPtr->UnicodeFormat;
	    infoPtr->UnicodeFormat = (BOOL)wParam;
	    return temp;
	}
	default:
	    if ((message >= WM_USER) && (message < WM_APP) && !COMCTL32_IsReflectedMessage(message))
		ERR("unknown msg %04x wp=%04lx lp=%08lx\n", message, wParam, lParam);
	    return DefWindowProcW (hwnd, message, wParam, lParam);
    }

    return 0;
}

/***********************************************************************
 *		UPDOWN_Register	[Internal]
 *
 * Registers the updown window class.
 */
void UPDOWN_Register(void)
{
    WNDCLASSW wndClass;

    ZeroMemory( &wndClass, sizeof( WNDCLASSW ) );
    wndClass.style         = CS_GLOBALCLASS | CS_VREDRAW | CS_HREDRAW;
    wndClass.lpfnWndProc   = UpDownWindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(UPDOWN_INFO*);
    wndClass.hCursor       = LoadCursorW( 0, (LPWSTR)IDC_ARROW );
    wndClass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wndClass.lpszClassName = UPDOWN_CLASSW;

    RegisterClassW( &wndClass );
}


/***********************************************************************
 *		UPDOWN_Unregister	[Internal]
 *
 * Unregisters the updown window class.
 */
void UPDOWN_Unregister (void)
{
    UnregisterClassW (UPDOWN_CLASSW, NULL);
}
