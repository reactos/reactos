/*
 * Interface code to StatusWindow widget/control
 *
 * Copyright 1996 Bruce Milner
 * Copyright 1998, 1999 Eric Kohl
 * Copyright 2002 Dimitrie O. Paun
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
 * TODO:
 * 	-- CCS_BOTTOM (default)
 * 	-- CCS_LEFT
 * 	-- CCS_NODIVIDER
 * 	-- CCS_NOMOVEX
 * 	-- CCS_NOMOVEY
 * 	-- CCS_NOPARENTALIGN
 * 	-- CCS_RIGHT
 * 	-- CCS_TOP
 * 	-- CCS_VERT (defaults to RIGHT)
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
#include "uxtheme.h"
#include "vssym32.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(statusbar);

typedef struct
{
    INT 	x;
    INT 	style;
    RECT	bound;
    LPWSTR	text;
    HICON       hIcon;
} STATUSWINDOWPART;

typedef struct
{
    HWND              Self;
    HWND              Notify;
    WORD              numParts;
    UINT              height;
    UINT              minHeight;        /* at least MIN_PANE_HEIGHT, can be increased by SB_SETMINHEIGHT */
    BOOL              simple;
    HWND              hwndToolTip;
    HFONT             hFont;
    HFONT             hDefaultFont;
    COLORREF          clrBk;		/* background color */
    BOOL              bUnicode;         /* notify format. TRUE if notifies in Unicode */
    STATUSWINDOWPART  part0;		/* simple window */
    STATUSWINDOWPART* parts;
    INT               horizontalBorder;
    INT               verticalBorder;
    INT               horizontalGap;
} STATUS_INFO;

/*
 * Run tests using Waite Group Windows95 API Bible Vol. 1&2
 * The second cdrom contains executables drawstat.exe, gettext.exe,
 * simple.exe, getparts.exe, setparts.exe, statwnd.exe
 */

#define HORZ_BORDER 0
#define VERT_BORDER 2
#define HORZ_GAP    2

static const WCHAR themeClass[] = { 'S','t','a','t','u','s',0 };

/* prototype */
static void
STATUSBAR_SetPartBounds (STATUS_INFO *infoPtr);
static LRESULT
STATUSBAR_NotifyFormat (STATUS_INFO *infoPtr, HWND from, INT cmd);

static inline LPCSTR debugstr_t(LPCWSTR text, BOOL isW)
{
  return isW ? debugstr_w(text) : debugstr_a((LPCSTR)text);
}

static UINT
STATUSBAR_ComputeHeight(STATUS_INFO *infoPtr)
{
    HTHEME theme;
    UINT height;
    TEXTMETRICW tm;
    int margin;

    COMCTL32_GetFontMetrics(infoPtr->hFont ? infoPtr->hFont : infoPtr->hDefaultFont, &tm);
    margin = (tm.tmInternalLeading ? tm.tmInternalLeading : 2);
    height = max(tm.tmHeight + margin + 2*GetSystemMetrics(SM_CYBORDER), infoPtr->minHeight) + infoPtr->verticalBorder;

    if ((theme = GetWindowTheme(infoPtr->Self)))
    {
        /* Determine bar height from theme such that the content area is
         * textHeight pixels large */
        HDC hdc = GetDC(infoPtr->Self);
        RECT r;

        SetRect(&r, 0, 0, 0, max(infoPtr->minHeight, tm.tmHeight));
        if (SUCCEEDED(GetThemeBackgroundExtent(theme, hdc, SP_PANE, 0, &r, &r)))
        {
            height = r.bottom - r.top;
        }
        ReleaseDC(infoPtr->Self, hdc);
    }

    TRACE("    textHeight=%d+%d, final height=%d\n", tm.tmHeight, tm.tmInternalLeading, height);
    return height;
}

static void
STATUSBAR_DrawSizeGrip (HTHEME theme, HDC hdc, LPRECT lpRect)
{
    RECT rc = *lpRect;

    TRACE("draw size grip %s\n", wine_dbgstr_rect(lpRect));

    if (theme)
    {
        SIZE gripperSize;
        if (SUCCEEDED (GetThemePartSize (theme, hdc, SP_GRIPPER, 0, lpRect, 
            TS_DRAW, &gripperSize)))
        {
            rc.left = rc.right - gripperSize.cx;
            rc.top = rc.bottom - gripperSize.cy;
            if (SUCCEEDED (DrawThemeBackground(theme, hdc, SP_GRIPPER, 0, &rc, NULL)))
                return;
        }
    }

    rc.left = max( rc.left, rc.right - GetSystemMetrics(SM_CXVSCROLL) - 1 );
    rc.top  = max( rc.top, rc.bottom - GetSystemMetrics(SM_CYHSCROLL) - 1 );
    DrawFrameControl( hdc, &rc, DFC_SCROLL, DFCS_SCROLLSIZEGRIP );
}


static void
STATUSBAR_DrawPart (const STATUS_INFO *infoPtr, HDC hdc, const STATUSWINDOWPART *part, int itemID)
{
    RECT r = part->bound;
    UINT border = BDR_SUNKENOUTER;
    HTHEME theme = GetWindowTheme (infoPtr->Self);
    int themePart = SP_PANE;
    int x = 0;

    TRACE("part bound %s\n", wine_dbgstr_rect(&r));
    if (part->style & SBT_POPOUT)
        border = BDR_RAISEDOUTER;
    else if (part->style & SBT_NOBORDERS)
        border = 0;

    if (theme)
    {
        if ((GetWindowLongW (infoPtr->Self, GWL_STYLE) & SBARS_SIZEGRIP)
            && (infoPtr->simple || (itemID == (infoPtr->numParts-1))))
            themePart = SP_GRIPPERPANE;
        DrawThemeBackground(theme, hdc, themePart, 0, &r, NULL);
    }
    else
        DrawEdge(hdc, &r, border, BF_RECT|BF_ADJUST);

    if (part->hIcon) {
        INT cy = r.bottom - r.top;
        DrawIconEx (hdc, r.left + 2, r.top, part->hIcon, cy, cy, 0, 0, DI_NORMAL);
        x = 2 + cy;
    }

    if (part->style & SBT_OWNERDRAW) {
	DRAWITEMSTRUCT dis;

	dis.CtlID = GetWindowLongPtrW (infoPtr->Self, GWLP_ID);
	dis.itemID = itemID;
	dis.hwndItem = infoPtr->Self;
	dis.hDC = hdc;
	dis.rcItem = r;
	dis.itemData = (ULONG_PTR)part->text;
        SendMessageW (infoPtr->Notify, WM_DRAWITEM, dis.CtlID, (LPARAM)&dis);
    } else {
        r.left += x;
#ifdef __REACTOS__
        if (!theme)
        {
            r.left -= 2;
            DrawStatusTextW (hdc, &r, part->text, SBT_NOBORDERS);
        }
        else
        {
            r.left += 2;
            r.right -= 2;
            DrawThemeText(theme, hdc, SP_PANE, 0, part->text, -1, DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX, 0, &r);
        }
#else
        DrawStatusTextW (hdc, &r, part->text, SBT_NOBORDERS);
#endif
    }
}


static void
STATUSBAR_RefreshPart (const STATUS_INFO *infoPtr, HDC hdc, const STATUSWINDOWPART *part, int itemID)
{
    HBRUSH hbrBk;
    HTHEME theme;

    TRACE("item %d\n", itemID);

    if (part->bound.right < part->bound.left) return;

    if (!RectVisible(hdc, &part->bound))
        return;

    if ((theme = GetWindowTheme (infoPtr->Self)))
    {
        RECT cr;
        GetClientRect (infoPtr->Self, &cr);
        DrawThemeBackground(theme, hdc, 0, 0, &cr, &part->bound);
    }
    else
    {
        if (infoPtr->clrBk != CLR_DEFAULT)
                hbrBk = CreateSolidBrush (infoPtr->clrBk);
        else
                hbrBk = GetSysColorBrush (COLOR_3DFACE);
        FillRect(hdc, &part->bound, hbrBk);
        if (infoPtr->clrBk != CLR_DEFAULT)
                DeleteObject (hbrBk);
    }

    STATUSBAR_DrawPart (infoPtr, hdc, part, itemID);
}


static LRESULT
STATUSBAR_Refresh (STATUS_INFO *infoPtr, HDC hdc)
{
    RECT   rect;
    HBRUSH hbrBk;
    HFONT  hOldFont;
    HTHEME theme;

    TRACE("\n");
    if (!IsWindowVisible(infoPtr->Self))
        return 0;

    STATUSBAR_SetPartBounds(infoPtr);

    GetClientRect (infoPtr->Self, &rect);

    if ((theme = GetWindowTheme (infoPtr->Self)))
    {
        DrawThemeBackground(theme, hdc, 0, 0, &rect, NULL);
    }
    else
    {
        if (infoPtr->clrBk != CLR_DEFAULT)
            hbrBk = CreateSolidBrush (infoPtr->clrBk);
        else
            hbrBk = GetSysColorBrush (COLOR_3DFACE);
        FillRect(hdc, &rect, hbrBk);
        if (infoPtr->clrBk != CLR_DEFAULT)
            DeleteObject (hbrBk);
    }

    hOldFont = SelectObject (hdc, infoPtr->hFont ? infoPtr->hFont : infoPtr->hDefaultFont);

    if (infoPtr->simple) {
	STATUSBAR_RefreshPart (infoPtr, hdc, &infoPtr->part0, 0);
    } else {
        unsigned int i;

	for (i = 0; i < infoPtr->numParts; i++) {
	    STATUSBAR_RefreshPart (infoPtr, hdc, &infoPtr->parts[i], i);
	}
    }

    SelectObject (hdc, hOldFont);

    if ((GetWindowLongW (infoPtr->Self, GWL_STYLE) & SBARS_SIZEGRIP)
            && !(GetWindowLongW (infoPtr->Notify, GWL_STYLE) & WS_MAXIMIZE))
	    STATUSBAR_DrawSizeGrip (theme, hdc, &rect);

    return 0;
}


static int
STATUSBAR_InternalHitTest(const STATUS_INFO *infoPtr, const POINT *pt)
{
    unsigned int i;

    if (infoPtr->simple)
        return 255;

    for (i = 0; i < infoPtr->numParts; i++)
        if (pt->x >= infoPtr->parts[i].bound.left && pt->x <= infoPtr->parts[i].bound.right)
            return i;
    return -2;
}


static void
STATUSBAR_SetPartBounds (STATUS_INFO *infoPtr)
{
    STATUSWINDOWPART *part;
    RECT rect, *r;
    UINT i;

    /* get our window size */
    GetClientRect (infoPtr->Self, &rect);
    TRACE("client wnd size is %s\n", wine_dbgstr_rect(&rect));

    rect.left += infoPtr->horizontalBorder;
    rect.top += infoPtr->verticalBorder;

    /* set bounds for simple rectangle */
    infoPtr->part0.bound = rect;

    /* set bounds for non-simple rectangles */
    for (i = 0; i < infoPtr->numParts; i++) {
	part = &infoPtr->parts[i];
	r = &infoPtr->parts[i].bound;
	r->top = rect.top;
	r->bottom = rect.bottom;
	if (i == 0)
	    r->left = 0;
	else
	    r->left = infoPtr->parts[i-1].bound.right + infoPtr->horizontalGap;
	if (part->x == -1)
	    r->right = rect.right;
	else
	    r->right = part->x;

	if (infoPtr->hwndToolTip) {
	    TTTOOLINFOW ti;

	    ti.cbSize = sizeof(TTTOOLINFOW);
	    ti.hwnd = infoPtr->Self;
	    ti.uId = i;
	    ti.rect = *r;
	    SendMessageW (infoPtr->hwndToolTip, TTM_NEWTOOLRECTW,
			    0, (LPARAM)&ti);
	}
    }
}


static LRESULT
STATUSBAR_Relay2Tip (const STATUS_INFO *infoPtr, UINT uMsg,
		     WPARAM wParam, LPARAM lParam)
{
    MSG msg;

    msg.hwnd = infoPtr->Self;
    msg.message = uMsg;
    msg.wParam = wParam;
    msg.lParam = lParam;
    msg.time = GetMessageTime ();
    msg.pt.x = (short)LOWORD(GetMessagePos ());
    msg.pt.y = (short)HIWORD(GetMessagePos ());

    return SendMessageW (infoPtr->hwndToolTip, TTM_RELAYEVENT, 0, (LPARAM)&msg);
}


static BOOL
STATUSBAR_GetBorders (const STATUS_INFO *infoPtr, INT out[])
{
    TRACE("\n");
    out[0] = infoPtr->horizontalBorder;
    out[1] = infoPtr->verticalBorder;
    out[2] = infoPtr->horizontalGap;

    return TRUE;
}


static BOOL
STATUSBAR_SetBorders (STATUS_INFO *infoPtr, const INT in[])
{
    TRACE("\n");
    infoPtr->horizontalBorder = in[0];
    infoPtr->verticalBorder = in[1];
    infoPtr->horizontalGap = in[2];
    InvalidateRect(infoPtr->Self, NULL, FALSE);

    return TRUE;
}


static HICON
STATUSBAR_GetIcon (const STATUS_INFO *infoPtr, INT nPart)
{
    TRACE("%d\n", nPart);
    /* MSDN says: "simple parts are indexed with -1" */
    if ((nPart < -1) || (nPart >= infoPtr->numParts))
	return 0;

    if (nPart == -1)
        return (infoPtr->part0.hIcon);
    else
        return (infoPtr->parts[nPart].hIcon);
}


static INT
STATUSBAR_GetParts (const STATUS_INFO *infoPtr, INT num_parts, INT parts[])
{
    INT   i;

    TRACE("(%d)\n", num_parts);
    if (parts) {
#ifdef __REACTOS__
        if (num_parts > infoPtr->numParts)
            num_parts = infoPtr->numParts;
#endif
	for (i = 0; i < num_parts; i++) {
	    parts[i] = infoPtr->parts[i].x;
	}
    }
    return infoPtr->numParts;
}


static BOOL
STATUSBAR_GetRect (const STATUS_INFO *infoPtr, INT nPart, LPRECT rect)
{
    TRACE("part %d\n", nPart);
    if(nPart >= infoPtr->numParts || nPart < 0)
      return FALSE;
    if (infoPtr->simple)
	*rect = infoPtr->part0.bound;
    else
	*rect = infoPtr->parts[nPart].bound;
    return TRUE;
}


static LRESULT
STATUSBAR_GetTextA (STATUS_INFO *infoPtr, INT nPart, LPSTR buf)
{
    STATUSWINDOWPART *part;
    LRESULT result;

    TRACE("part %d\n", nPart);

    /* MSDN says: "simple parts use index of 0", so this check is ok. */
    if (nPart < 0 || nPart >= infoPtr->numParts) return 0;

    if (infoPtr->simple)
	part = &infoPtr->part0;
    else
	part = &infoPtr->parts[nPart];

    if (part->style & SBT_OWNERDRAW)
	result = (LRESULT)part->text;
    else {
        DWORD len = part->text ? WideCharToMultiByte( CP_ACP, 0, part->text, -1,
                                                      NULL, 0, NULL, NULL ) - 1 : 0;
        result = MAKELONG( len, part->style );
        if (part->text && buf)
            WideCharToMultiByte( CP_ACP, 0, part->text, -1, buf, len+1, NULL, NULL );
    }
    return result;
}


static LRESULT
STATUSBAR_GetTextW (STATUS_INFO *infoPtr, INT nPart, LPWSTR buf)
{
    STATUSWINDOWPART *part;
    LRESULT result;

    TRACE("part %d\n", nPart);
    if (nPart < 0 || nPart >= infoPtr->numParts) return 0;

    if (infoPtr->simple)
	part = &infoPtr->part0;
    else
	part = &infoPtr->parts[nPart];

    if (part->style & SBT_OWNERDRAW)
	result = (LRESULT)part->text;
    else {
	result = part->text ? lstrlenW (part->text) : 0;
	result |= (part->style << 16);
	if (part->text && buf)
	    lstrcpyW (buf, part->text);
    }
    return result;
}


static LRESULT
STATUSBAR_GetTextLength (STATUS_INFO *infoPtr, INT nPart)
{
    STATUSWINDOWPART *part;
    DWORD result;

    TRACE("part %d\n", nPart);

    /* MSDN says: "simple parts use index of 0", so this check is ok. */
    if (nPart < 0 || nPart >= infoPtr->numParts) return 0;

    if (infoPtr->simple)
	part = &infoPtr->part0;
    else
	part = &infoPtr->parts[nPart];

    if ((~part->style & SBT_OWNERDRAW) && part->text)
	result = lstrlenW(part->text);
    else
	result = 0;

    result |= (part->style << 16);
    return result;
}

static LRESULT
STATUSBAR_GetTipTextA (const STATUS_INFO *infoPtr, INT id, LPSTR tip, INT size)
{
    TRACE("\n");
    if (tip) {
        CHAR buf[INFOTIPSIZE];
        buf[0]='\0';

        if (infoPtr->hwndToolTip) {
            TTTOOLINFOA ti;
            ti.cbSize = sizeof(TTTOOLINFOA);
            ti.hwnd = infoPtr->Self;
            ti.uId = id;
            ti.lpszText = buf;
            SendMessageA (infoPtr->hwndToolTip, TTM_GETTEXTA, 0, (LPARAM)&ti);
        }
        lstrcpynA (tip, buf, size);
    }
    return 0;
}


static LRESULT
STATUSBAR_GetTipTextW (const STATUS_INFO *infoPtr, INT id, LPWSTR tip, INT size)
{
    TRACE("\n");
    if (tip) {
        WCHAR buf[INFOTIPSIZE];
        buf[0]=0;

	if (infoPtr->hwndToolTip) {
	    TTTOOLINFOW ti;
	    ti.cbSize = sizeof(TTTOOLINFOW);
	    ti.hwnd = infoPtr->Self;
	    ti.uId = id;
            ti.lpszText = buf;
	    SendMessageW(infoPtr->hwndToolTip, TTM_GETTEXTW, 0, (LPARAM)&ti);
	}
	lstrcpynW(tip, buf, size);
    }

    return 0;
}


static COLORREF
STATUSBAR_SetBkColor (STATUS_INFO *infoPtr, COLORREF color)
{
    COLORREF oldBkColor;

    oldBkColor = infoPtr->clrBk;
    infoPtr->clrBk = color;
    InvalidateRect(infoPtr->Self, NULL, FALSE);

    TRACE("CREF: %08x -> %08x\n", oldBkColor, infoPtr->clrBk);
    return oldBkColor;
}


static BOOL
STATUSBAR_SetIcon (STATUS_INFO *infoPtr, INT nPart, HICON hIcon)
{
    if ((nPart < -1) || (nPart >= infoPtr->numParts))
	return FALSE;

    TRACE("setting part %d\n", nPart);

    /* FIXME: MSDN says "if nPart is -1, the status bar is assumed simple" */
    if (nPart == -1) {
	if (infoPtr->part0.hIcon == hIcon) /* same as - no redraw */
	    return TRUE;
	infoPtr->part0.hIcon = hIcon;
	if (infoPtr->simple)
            InvalidateRect(infoPtr->Self, &infoPtr->part0.bound, FALSE);
    } else {
	if (infoPtr->parts[nPart].hIcon == hIcon) /* same as - no redraw */
	    return TRUE;

	infoPtr->parts[nPart].hIcon = hIcon;
	if (!(infoPtr->simple))
            InvalidateRect(infoPtr->Self, &infoPtr->parts[nPart].bound, FALSE);
    }
    return TRUE;
}


static BOOL
STATUSBAR_SetMinHeight (STATUS_INFO *infoPtr, INT height)
{
    DWORD ysize = GetSystemMetrics(SM_CYSIZE);
    if (ysize & 1) ysize--;
    infoPtr->minHeight = max(height, ysize);
    infoPtr->height = STATUSBAR_ComputeHeight(infoPtr);
    /* like native, don't resize the control */
    return TRUE;
}


static BOOL
STATUSBAR_SetParts (STATUS_INFO *infoPtr, INT count, LPINT parts)
{
    STATUSWINDOWPART *tmp;
    INT i, oldNumParts;

    TRACE("(%d,%p)\n", count, parts);

    if(!count) return FALSE;

    oldNumParts = infoPtr->numParts;
    infoPtr->numParts = count;
    if (oldNumParts > infoPtr->numParts) {
	for (i = infoPtr->numParts ; i < oldNumParts; i++) {
	    if (!(infoPtr->parts[i].style & SBT_OWNERDRAW))
		Free (infoPtr->parts[i].text);
	}
    } else if (oldNumParts < infoPtr->numParts) {
	tmp = Alloc (sizeof(STATUSWINDOWPART) * infoPtr->numParts);
	if (!tmp) return FALSE;
	for (i = 0; i < oldNumParts; i++) {
	    tmp[i] = infoPtr->parts[i];
	}
        Free (infoPtr->parts);
	infoPtr->parts = tmp;
    }
    if (oldNumParts == infoPtr->numParts) {
	for (i=0; i < oldNumParts; i++)
	    if (infoPtr->parts[i].x != parts[i])
		break;
	if (i==oldNumParts) /* Unchanged? no need to redraw! */
	    return TRUE;
    }

    for (i = 0; i < infoPtr->numParts; i++)
	infoPtr->parts[i].x = parts[i];

    if (infoPtr->hwndToolTip) {
	INT nTipCount;
	TTTOOLINFOW ti;
	WCHAR wEmpty = 0;

	ZeroMemory (&ti, sizeof(TTTOOLINFOW));
	ti.cbSize = sizeof(TTTOOLINFOW);
	ti.hwnd = infoPtr->Self;
	ti.lpszText = &wEmpty;

	nTipCount = SendMessageW (infoPtr->hwndToolTip, TTM_GETTOOLCOUNT, 0, 0);
	if (nTipCount < infoPtr->numParts) {
	    /* add tools */
	    for (i = nTipCount; i < infoPtr->numParts; i++) {
		TRACE("add tool %d\n", i);
		ti.uId = i;
		SendMessageW (infoPtr->hwndToolTip, TTM_ADDTOOLW,
				0, (LPARAM)&ti);
	    }
	}
	else if (nTipCount > infoPtr->numParts) {
	    /* delete tools */
	    for (i = nTipCount - 1; i >= infoPtr->numParts; i--) {
		TRACE("delete tool %d\n", i);
		ti.uId = i;
		SendMessageW (infoPtr->hwndToolTip, TTM_DELTOOLW,
				0, (LPARAM)&ti);
	    }
	}
    }
    STATUSBAR_SetPartBounds (infoPtr);
    InvalidateRect(infoPtr->Self, NULL, FALSE);
    return TRUE;
}


static BOOL
STATUSBAR_SetTextT (STATUS_INFO *infoPtr, INT nPart, WORD style,
		    LPWSTR text, BOOL isW)
{
    STATUSWINDOWPART *part=NULL;
    BOOL changed = FALSE;
    INT  oldStyle;

    if (style & SBT_OWNERDRAW) {
         TRACE("part %d, text %p\n",nPart,text);
    }
    else TRACE("part %d, text %s\n", nPart, debugstr_t(text, isW));

    /* MSDN says: "If the parameter is set to SB_SIMPLEID (255), the status
     * window is assumed to be a simple window */

    if (nPart == 0x00ff) {
	part = &infoPtr->part0;
    } else {
	if (infoPtr->parts && nPart >= 0 && nPart < infoPtr->numParts) {
	    part = &infoPtr->parts[nPart];
	}
    }
    if (!part) return FALSE;

    if (part->style != style)
	changed = TRUE;

    oldStyle = part->style;
    part->style = style;
    if (style & SBT_OWNERDRAW) {
        if (!(oldStyle & SBT_OWNERDRAW))
            Free (part->text);
        part->text = text;
    } else {
	LPWSTR ntext;
	WCHAR  *idx;

	if (text && !isW) {
	    LPCSTR atxt = (LPCSTR)text;
            DWORD len = MultiByteToWideChar( CP_ACP, 0, atxt, -1, NULL, 0 );
	    ntext = Alloc( (len + 1)*sizeof(WCHAR) );
	    if (!ntext) return FALSE;
            MultiByteToWideChar( CP_ACP, 0, atxt, -1, ntext, len );
	} else if (text) {
	    ntext = Alloc( (lstrlenW(text) + 1)*sizeof(WCHAR) );
	    if (!ntext) return FALSE;
	    lstrcpyW (ntext, text);
	} else ntext = 0;

	/* replace nonprintable characters with spaces */
	if (ntext) {
	    idx = ntext;
	    while (*idx) {
	        if(!iswprint(*idx))
	            *idx = ' ';
	        idx++;
	    }
	}

	/* check if text is unchanged -> no need to redraw */
	if (text) {
	    if (!changed && part->text && !lstrcmpW(ntext, part->text)) {
		Free(ntext);
		return TRUE;
	    }
	} else {
	    if (!changed && !part->text)
		return TRUE;
	}

	if (!(oldStyle & SBT_OWNERDRAW))
	    Free (part->text);
	part->text = ntext;
    }
    InvalidateRect(infoPtr->Self, &part->bound, FALSE);
    UpdateWindow(infoPtr->Self);

    return TRUE;
}


static LRESULT
STATUSBAR_SetTipTextA (const STATUS_INFO *infoPtr, INT id, LPSTR text)
{
    TRACE("part %d: \"%s\"\n", id, text);
    if (infoPtr->hwndToolTip) {
	TTTOOLINFOA ti;
	ti.cbSize = sizeof(TTTOOLINFOA);
	ti.hwnd = infoPtr->Self;
	ti.uId = id;
	ti.hinst = 0;
	ti.lpszText = text;
	SendMessageA (infoPtr->hwndToolTip, TTM_UPDATETIPTEXTA, 0, (LPARAM)&ti);
    }

    return 0;
}


static LRESULT
STATUSBAR_SetTipTextW (const STATUS_INFO *infoPtr, INT id, LPWSTR text)
{
    TRACE("part %d: \"%s\"\n", id, debugstr_w(text));
    if (infoPtr->hwndToolTip) {
	TTTOOLINFOW ti;
	ti.cbSize = sizeof(TTTOOLINFOW);
	ti.hwnd = infoPtr->Self;
	ti.uId = id;
	ti.hinst = 0;
	ti.lpszText = text;
	SendMessageW (infoPtr->hwndToolTip, TTM_UPDATETIPTEXTW, 0, (LPARAM)&ti);
    }

    return 0;
}


static inline LRESULT
STATUSBAR_SetUnicodeFormat (STATUS_INFO *infoPtr, BOOL bUnicode)
{
    BOOL bOld = infoPtr->bUnicode;

    TRACE("(0x%x)\n", bUnicode);
    infoPtr->bUnicode = bUnicode;

    return bOld;
}


static BOOL
STATUSBAR_Simple (STATUS_INFO *infoPtr, BOOL simple)
{
    NMHDR  nmhdr;

    TRACE("(simple=%d)\n", simple);
    if (infoPtr->simple == simple) /* no need to change */
	return TRUE;

    infoPtr->simple = simple;

    /* send notification */
    nmhdr.hwndFrom = infoPtr->Self;
    nmhdr.idFrom = GetWindowLongPtrW (infoPtr->Self, GWLP_ID);
    nmhdr.code = SBN_SIMPLEMODECHANGE;
    SendMessageW (infoPtr->Notify, WM_NOTIFY, 0, (LPARAM)&nmhdr);
    InvalidateRect(infoPtr->Self, NULL, FALSE);
    return TRUE;
}


static LRESULT
STATUSBAR_WMDestroy (STATUS_INFO *infoPtr)
{
    unsigned int i;

    TRACE("\n");
    for (i = 0; i < infoPtr->numParts; i++) {
	if (!(infoPtr->parts[i].style & SBT_OWNERDRAW))
	    Free (infoPtr->parts[i].text);
    }
    if (!(infoPtr->part0.style & SBT_OWNERDRAW))
	Free (infoPtr->part0.text);
    Free (infoPtr->parts);

    /* delete default font */
    if (infoPtr->hDefaultFont)
	DeleteObject (infoPtr->hDefaultFont);

    /* delete tool tip control */
    if (infoPtr->hwndToolTip)
	DestroyWindow (infoPtr->hwndToolTip);

    CloseThemeData (GetWindowTheme (infoPtr->Self));

    SetWindowLongPtrW(infoPtr->Self, 0, 0);
    Free (infoPtr);
    return 0;
}


static LRESULT
STATUSBAR_WMCreate (HWND hwnd, const CREATESTRUCTA *lpCreate)
{
    STATUS_INFO *infoPtr;
    NONCLIENTMETRICSW nclm;
    DWORD dwStyle;
    RECT rect;
    int	len;

    TRACE("\n");
    infoPtr = Alloc (sizeof(STATUS_INFO));
    if (!infoPtr) goto create_fail;
    SetWindowLongPtrW (hwnd, 0, (DWORD_PTR)infoPtr);

    infoPtr->Self = hwnd;
    infoPtr->Notify = lpCreate->hwndParent;
    infoPtr->numParts = 1;
    infoPtr->parts = 0;
    infoPtr->simple = FALSE;
    infoPtr->clrBk = CLR_DEFAULT;
    infoPtr->hFont = 0;
    infoPtr->horizontalBorder = HORZ_BORDER;
    infoPtr->verticalBorder = VERT_BORDER;
    infoPtr->horizontalGap = HORZ_GAP;
    infoPtr->minHeight = GetSystemMetrics(SM_CYSIZE);
    if (infoPtr->minHeight & 1) infoPtr->minHeight--;

    STATUSBAR_NotifyFormat(infoPtr, infoPtr->Notify, NF_REQUERY);

    ZeroMemory (&nclm, sizeof(nclm));
    nclm.cbSize = sizeof(nclm);
    SystemParametersInfoW (SPI_GETNONCLIENTMETRICS, nclm.cbSize, &nclm, 0);
    infoPtr->hDefaultFont = CreateFontIndirectW (&nclm.lfStatusFont);

    GetClientRect (hwnd, &rect);

    /* initialize simple case */
    infoPtr->part0.bound = rect;
    infoPtr->part0.text = 0;
    infoPtr->part0.x = 0;
    infoPtr->part0.style = 0;
    infoPtr->part0.hIcon = 0;

    /* initialize first part */
    infoPtr->parts = Alloc (sizeof(STATUSWINDOWPART));
    if (!infoPtr->parts) goto create_fail;
    infoPtr->parts[0].bound = rect;
    infoPtr->parts[0].text = 0;
    infoPtr->parts[0].x = -1;
    infoPtr->parts[0].style = 0;
    infoPtr->parts[0].hIcon = 0;
    
    OpenThemeData (hwnd, themeClass);

    if (lpCreate->lpszName && (len = lstrlenW ((LPCWSTR)lpCreate->lpszName)))
    {
        infoPtr->parts[0].text = Alloc ((len + 1)*sizeof(WCHAR));
        if (!infoPtr->parts[0].text) goto create_fail;
        lstrcpyW (infoPtr->parts[0].text, (LPCWSTR)lpCreate->lpszName);
    }

    dwStyle = GetWindowLongW (hwnd, GWL_STYLE);
    /* native seems to clear WS_BORDER, too */
    dwStyle &= ~WS_BORDER;
    SetWindowLongW (hwnd, GWL_STYLE, dwStyle);

    infoPtr->height = STATUSBAR_ComputeHeight(infoPtr);

    if (dwStyle & SBT_TOOLTIPS) {
	infoPtr->hwndToolTip =
	    CreateWindowExW (0, TOOLTIPS_CLASSW, NULL, WS_POPUP | TTS_ALWAYSTIP,
			     CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			     CW_USEDEFAULT, hwnd, 0,
			     (HINSTANCE)GetWindowLongPtrW(hwnd, GWLP_HINSTANCE), NULL);

	if (infoPtr->hwndToolTip) {
	    NMTOOLTIPSCREATED nmttc;

	    nmttc.hdr.hwndFrom = hwnd;
	    nmttc.hdr.idFrom = GetWindowLongPtrW (hwnd, GWLP_ID);
	    nmttc.hdr.code = NM_TOOLTIPSCREATED;
	    nmttc.hwndToolTips = infoPtr->hwndToolTip;

	    SendMessageW (lpCreate->hwndParent, WM_NOTIFY, nmttc.hdr.idFrom, (LPARAM)&nmttc);
	}
    }

    return 0;

create_fail:
    TRACE("    failed!\n");
    if (infoPtr) STATUSBAR_WMDestroy(infoPtr);
    return -1;
}


/* in contrast to SB_GETTEXT*, WM_GETTEXT handles the text
 * of the first part only (usual behaviour) */
static INT
STATUSBAR_WMGetText (const STATUS_INFO *infoPtr, INT size, LPWSTR buf)
{
    INT len;

    TRACE("\n");
    if (!(infoPtr->parts[0].text))
        return 0;

    len = lstrlenW (infoPtr->parts[0].text);

    if (!size)
        return len;
    else if (size > len) {
        lstrcpyW (buf, infoPtr->parts[0].text);
	return len;
    }
    else {
        memcpy (buf, infoPtr->parts[0].text, (size - 1) * sizeof(WCHAR));
        buf[size - 1] = 0;
        return size - 1;
    }
}


static BOOL
STATUSBAR_WMNCHitTest (const STATUS_INFO *infoPtr, INT x, INT y)
{
    if ((GetWindowLongW (infoPtr->Self, GWL_STYLE) & SBARS_SIZEGRIP)
            && !(GetWindowLongW (infoPtr->Notify, GWL_STYLE) & WS_MAXIMIZE)) {
	RECT  rect;
	POINT pt;

	GetClientRect (infoPtr->Self, &rect);

	pt.x = x;
	pt.y = y;
	ScreenToClient (infoPtr->Self, &pt);

	if (pt.x >= rect.right - GetSystemMetrics(SM_CXVSCROLL))
        {
            if (GetWindowLongW( infoPtr->Self, GWL_EXSTYLE ) & WS_EX_LAYOUTRTL) return HTBOTTOMLEFT;
	    else return HTBOTTOMRIGHT;
        }
    }

    return HTERROR;
}


static LRESULT
STATUSBAR_WMPaint (STATUS_INFO *infoPtr, HDC hdc)
{
    PAINTSTRUCT ps;

    TRACE("\n");
    if (hdc) return STATUSBAR_Refresh (infoPtr, hdc);
    hdc = BeginPaint (infoPtr->Self, &ps);
    STATUSBAR_Refresh (infoPtr, hdc);
    EndPaint (infoPtr->Self, &ps);

    return 0;
}


static LRESULT
STATUSBAR_WMSetFont (STATUS_INFO *infoPtr, HFONT font, BOOL redraw)
{
    infoPtr->hFont = font;
    TRACE("%p\n", infoPtr->hFont);

    infoPtr->height = STATUSBAR_ComputeHeight(infoPtr);
    SendMessageW(infoPtr->Self, WM_SIZE, 0, 0);  /* update size */
    if (redraw)
        InvalidateRect(infoPtr->Self, NULL, FALSE);

    return 0;
}


static BOOL
STATUSBAR_WMSetText (const STATUS_INFO *infoPtr, LPCSTR text)
{
    STATUSWINDOWPART *part;
    int len;

    TRACE("\n");
    if (infoPtr->numParts == 0)
	return FALSE;

    part = &infoPtr->parts[0];
    /* duplicate string */
    Free (part->text);
    part->text = 0;

    if (text && (len = lstrlenW((LPCWSTR)text))) {
        part->text = Alloc ((len+1)*sizeof(WCHAR));
        if (!part->text) return FALSE;
        lstrcpyW (part->text, (LPCWSTR)text);
    }

    InvalidateRect(infoPtr->Self, &part->bound, FALSE);

    return TRUE;
}


static BOOL
STATUSBAR_WMSize (STATUS_INFO *infoPtr, WORD flags)
{
    INT  width, x, y;
    RECT parent_rect;

    /* Need to resize width to match parent */
    TRACE("flags %04x\n", flags);

    if (flags != SIZE_RESTORED && flags != SIZE_MAXIMIZED) {
	WARN("flags MUST be SIZE_RESTORED or SIZE_MAXIMIZED\n");
	return FALSE;
    }

    if (GetWindowLongW(infoPtr->Self, GWL_STYLE) & CCS_NORESIZE) return FALSE;

    /* width and height don't apply */
    if (!GetClientRect (infoPtr->Notify, &parent_rect))
        return FALSE;

    width = parent_rect.right - parent_rect.left;
    x = parent_rect.left;
    y = parent_rect.bottom - infoPtr->height;
    MoveWindow (infoPtr->Self, x, y, width, infoPtr->height, TRUE);
    STATUSBAR_SetPartBounds (infoPtr);
#ifdef __REACTOS__
    parent_rect = infoPtr->parts[infoPtr->numParts - 1].bound;
    InvalidateRect(infoPtr->Self, &parent_rect, TRUE);
#endif
    return TRUE;
}


/* update theme after a WM_THEMECHANGED message */
static LRESULT theme_changed (const STATUS_INFO* infoPtr)
{
    HTHEME theme = GetWindowTheme (infoPtr->Self);
    CloseThemeData (theme);
    OpenThemeData (infoPtr->Self, themeClass);
    return 0;
}


static LRESULT
STATUSBAR_NotifyFormat (STATUS_INFO *infoPtr, HWND from, INT cmd)
{
    if (cmd == NF_REQUERY) {
	INT i = SendMessageW(from, WM_NOTIFYFORMAT, (WPARAM)infoPtr->Self, NF_QUERY);
	infoPtr->bUnicode = (i == NFR_UNICODE);
    }
    return infoPtr->bUnicode ? NFR_UNICODE : NFR_ANSI;
}


static LRESULT
STATUSBAR_SendMouseNotify(const STATUS_INFO *infoPtr, UINT code, UINT msg, WPARAM wParam, LPARAM lParam)
{
    NMMOUSE  nm;

    TRACE("code %04x, lParam=%lx\n", code, lParam);
    nm.hdr.hwndFrom = infoPtr->Self;
    nm.hdr.idFrom = GetWindowLongPtrW(infoPtr->Self, GWLP_ID);
    nm.hdr.code = code;
    nm.pt.x = (short)LOWORD(lParam);
    nm.pt.y = (short)HIWORD(lParam);
    nm.dwItemSpec = STATUSBAR_InternalHitTest(infoPtr, &nm.pt);
    nm.dwItemData = 0;
    nm.dwHitInfo = 0x30000;     /* seems constant */

    /* Do default processing if WM_NOTIFY returns zero */
    if(!SendMessageW(infoPtr->Notify, WM_NOTIFY, nm.hdr.idFrom, (LPARAM)&nm))
    {
      return DefWindowProcW(infoPtr->Self, msg, wParam, lParam);
    }
    return 0;
}



static LRESULT WINAPI
StatusWindowProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    STATUS_INFO *infoPtr = (STATUS_INFO *)GetWindowLongPtrW (hwnd, 0);
    INT nPart = ((INT) wParam) & 0x00ff;
    LRESULT res;

    TRACE("hwnd=%p msg=%x wparam=%lx lparam=%lx\n", hwnd, msg, wParam, lParam);
    if (!infoPtr && msg != WM_CREATE)
        return DefWindowProcW (hwnd, msg, wParam, lParam);

    switch (msg) {
	case SB_GETBORDERS:
	    return STATUSBAR_GetBorders (infoPtr, (INT *)lParam);

	case SB_GETICON:
	    return (LRESULT)STATUSBAR_GetIcon (infoPtr, nPart);

	case SB_GETPARTS:
	    return STATUSBAR_GetParts (infoPtr, (INT)wParam, (INT *)lParam);

	case SB_GETRECT:
	    return STATUSBAR_GetRect (infoPtr, nPart, (LPRECT)lParam);

	case SB_GETTEXTA:
	    return STATUSBAR_GetTextA (infoPtr, nPart, (LPSTR)lParam);

	case SB_GETTEXTW:
	    return STATUSBAR_GetTextW (infoPtr, nPart, (LPWSTR)lParam);

	case SB_GETTEXTLENGTHA:
	case SB_GETTEXTLENGTHW:
	    return STATUSBAR_GetTextLength (infoPtr, nPart);

	case SB_GETTIPTEXTA:
	    return STATUSBAR_GetTipTextA (infoPtr,  LOWORD(wParam), (LPSTR)lParam,  HIWORD(wParam));

	case SB_GETTIPTEXTW:
	    return STATUSBAR_GetTipTextW (infoPtr,  LOWORD(wParam), (LPWSTR)lParam,  HIWORD(wParam));

	case SB_GETUNICODEFORMAT:
	    return infoPtr->bUnicode;

	case SB_ISSIMPLE:
	    return infoPtr->simple;

	case SB_SETBORDERS:
	    return STATUSBAR_SetBorders (infoPtr, (INT *)lParam);

	case SB_SETBKCOLOR:
	    return STATUSBAR_SetBkColor (infoPtr, (COLORREF)lParam);

	case SB_SETICON:
	    return STATUSBAR_SetIcon (infoPtr, nPart, (HICON)lParam);

	case SB_SETMINHEIGHT:
	    return STATUSBAR_SetMinHeight (infoPtr, (INT)wParam);

	case SB_SETPARTS:
	    return STATUSBAR_SetParts (infoPtr, (INT)wParam, (LPINT)lParam);

	case SB_SETTEXTA:
	    return STATUSBAR_SetTextT (infoPtr, nPart, wParam & 0xff00, (LPWSTR)lParam, FALSE);

	case SB_SETTEXTW:
	    return STATUSBAR_SetTextT (infoPtr, nPart, wParam & 0xff00, (LPWSTR)lParam, TRUE);

	case SB_SETTIPTEXTA:
	    return STATUSBAR_SetTipTextA (infoPtr, (INT)wParam, (LPSTR)lParam);

	case SB_SETTIPTEXTW:
	    return STATUSBAR_SetTipTextW (infoPtr, (INT)wParam, (LPWSTR)lParam);

	case SB_SETUNICODEFORMAT:
	    return STATUSBAR_SetUnicodeFormat (infoPtr, (BOOL)wParam);

	case SB_SIMPLE:
	    return STATUSBAR_Simple (infoPtr, (BOOL)wParam);

	case WM_CREATE:
	    return STATUSBAR_WMCreate (hwnd, (LPCREATESTRUCTA)lParam);

	case WM_DESTROY:
	    return STATUSBAR_WMDestroy (infoPtr);

	case WM_GETFONT:
	    return (LRESULT)(infoPtr->hFont? infoPtr->hFont : infoPtr->hDefaultFont);

	case WM_GETTEXT:
            return STATUSBAR_WMGetText (infoPtr, (INT)wParam, (LPWSTR)lParam);

	case WM_GETTEXTLENGTH:
	    return LOWORD(STATUSBAR_GetTextLength (infoPtr, 0));

	case WM_LBUTTONDBLCLK:
            return STATUSBAR_SendMouseNotify(infoPtr, NM_DBLCLK, msg, wParam, lParam);

	case WM_LBUTTONUP:
	    return STATUSBAR_SendMouseNotify(infoPtr, NM_CLICK, msg, wParam, lParam);

	case WM_MOUSEMOVE:
	    return STATUSBAR_Relay2Tip (infoPtr, msg, wParam, lParam);

	case WM_NCHITTEST:
	    res = STATUSBAR_WMNCHitTest(infoPtr, (short)LOWORD(lParam),
                                        (short)HIWORD(lParam));
	    if (res != HTERROR) return res;
	    return DefWindowProcW (hwnd, msg, wParam, lParam);

	case WM_NCLBUTTONUP:
	case WM_NCLBUTTONDOWN:
    	    PostMessageW (infoPtr->Notify, msg, wParam, lParam);
	    return 0;

	case WM_NOTIFYFORMAT:
	    return STATUSBAR_NotifyFormat(infoPtr, (HWND)wParam, (INT)lParam);

	case WM_PRINTCLIENT:
	case WM_PAINT:
	    return STATUSBAR_WMPaint (infoPtr, (HDC)wParam);

	case WM_RBUTTONDBLCLK:
	    return STATUSBAR_SendMouseNotify(infoPtr, NM_RDBLCLK, msg, wParam, lParam);

	case WM_RBUTTONUP:
	    return STATUSBAR_SendMouseNotify(infoPtr, NM_RCLICK, msg, wParam, lParam);

	case WM_SETFONT:
	    return STATUSBAR_WMSetFont (infoPtr, (HFONT)wParam, LOWORD(lParam));

	case WM_SETTEXT:
	    return STATUSBAR_WMSetText (infoPtr, (LPCSTR)lParam);

	case WM_SIZE:
	    if (STATUSBAR_WMSize (infoPtr, (WORD)wParam)) return 0;
            return DefWindowProcW (hwnd, msg, wParam, lParam);

        case WM_SYSCOLORCHANGE:
            COMCTL32_RefreshSysColors();
            return 0;

        case WM_THEMECHANGED:
            return theme_changed (infoPtr);

	default:
	    if ((msg >= WM_USER) && (msg < WM_APP) && !COMCTL32_IsReflectedMessage(msg))
		ERR("unknown msg %04x wp=%04lx lp=%08lx\n",
		     msg, wParam, lParam);
	    return DefWindowProcW (hwnd, msg, wParam, lParam);
    }
}


/***********************************************************************
 * STATUS_Register [Internal]
 *
 * Registers the status window class.
 */

void
STATUS_Register (void)
{
    WNDCLASSW wndClass;

    ZeroMemory (&wndClass, sizeof(WNDCLASSW));
    wndClass.style         = CS_GLOBALCLASS | CS_DBLCLKS | CS_VREDRAW;
    wndClass.lpfnWndProc   = StatusWindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(STATUS_INFO *);
    wndClass.hCursor       = LoadCursorW (0, (LPWSTR)IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wndClass.lpszClassName = STATUSCLASSNAMEW;

    RegisterClassW (&wndClass);
}


/***********************************************************************
 * STATUS_Unregister [Internal]
 *
 * Unregisters the status window class.
 */

void
STATUS_Unregister (void)
{
    UnregisterClassW (STATUSCLASSNAMEW, NULL);
}
