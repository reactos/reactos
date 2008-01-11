/*
 * Trackbar control
 *
 * Copyright 1998, 1999 Eric Kohl
 * Copyright 1998, 1999 Alex Priem
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
 * NOTE
 * 
 * This code was audited for completeness against the documented features
 * of Comctl32.dll version 6.0 on Sep. 12, 2002, by Dimitrie O. Paun.
 * 
 * Unless otherwise noted, we believe this code to be complete, as per
 * the specification mentioned above.
 * If you discover missing features, or bugs, please note them below.
 * 
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "commctrl.h"
#include "uxtheme.h"
#include "tmschema.h"
#include "wine/debug.h"

#include "comctl32.h"

WINE_DEFAULT_DEBUG_CHANNEL(trackbar);

typedef struct
{
    HWND hwndSelf;
    LONG lRangeMin;
    LONG lRangeMax;
    LONG lLineSize;
    LONG lPageSize;
    LONG lSelMin;
    LONG lSelMax;
    LONG lPos;
    UINT uThumbLen;
    UINT uNumTics;
    UINT uTicFreq;
    HWND hwndNotify;
    HWND hwndToolTip;
    HWND hwndBuddyLA;
    HWND hwndBuddyRB;
    INT  fLocation;
    INT  flags;
    BOOL bUnicode;
    BOOL bFocussed;
    RECT rcChannel;
    RECT rcSelection;
    RECT rcThumb;
    LPLONG tics;
} TRACKBAR_INFO;

#define TB_REFRESH_TIMER	1
#define TB_REFRESH_DELAY	500

#define TOOLTIP_OFFSET		2     /* distance from ctrl edge to tooltip */

/* Used by TRACKBAR_Refresh to find out which parts of the control
   need to be recalculated */

#define TB_THUMBPOSCHANGED      1
#define TB_THUMBSIZECHANGED     2
#define TB_THUMBCHANGED 	(TB_THUMBPOSCHANGED | TB_THUMBSIZECHANGED)
#define TB_SELECTIONCHANGED     4
#define TB_DRAG_MODE            8     /* we're dragging the slider */
#define TB_AUTO_PAGE_LEFT	16
#define TB_AUTO_PAGE_RIGHT	32
#define TB_AUTO_PAGE		(TB_AUTO_PAGE_LEFT | TB_AUTO_PAGE_RIGHT)
#define TB_THUMB_HOT            64    /* mouse hovers above thumb */

/* helper defines for TRACKBAR_DrawTic */
#define TIC_EDGE                0x20
#define TIC_SELECTIONMARKMAX    0x80
#define TIC_SELECTIONMARKMIN    0x100
#define TIC_SELECTIONMARK       (TIC_SELECTIONMARKMAX | TIC_SELECTIONMARKMIN)

static const WCHAR themeClass[] = { 'T','r','a','c','k','b','a','r',0 };

static inline int 
notify_customdraw (const TRACKBAR_INFO *infoPtr, NMCUSTOMDRAW *pnmcd, int stage)
{
    pnmcd->dwDrawStage = stage;
    return SendMessageW (infoPtr->hwndNotify, WM_NOTIFY, 
		         pnmcd->hdr.idFrom, (LPARAM)pnmcd);
}

static LRESULT notify_hdr (const TRACKBAR_INFO *infoPtr, INT code, LPNMHDR pnmh)
{
    LRESULT result;
    
    TRACE("(code=%d)\n", code);

    pnmh->hwndFrom = infoPtr->hwndSelf;
    pnmh->idFrom = GetWindowLongPtrW(infoPtr->hwndSelf, GWLP_ID);
    pnmh->code = code;
    result = SendMessageW(infoPtr->hwndNotify, WM_NOTIFY,
			  (WPARAM)pnmh->idFrom, (LPARAM)pnmh);

    TRACE("  <= %ld\n", result);

    return result;
}

static inline int notify (const TRACKBAR_INFO *infoPtr, INT code)
{
    NMHDR nmh;
    return notify_hdr(infoPtr, code, &nmh);
}

static BOOL
notify_with_scroll (const TRACKBAR_INFO *infoPtr, UINT code)
{
    BOOL bVert = GetWindowLongW (infoPtr->hwndSelf, GWL_STYLE) & TBS_VERT;

    TRACE("%x\n", code);

    return (BOOL) SendMessageW (infoPtr->hwndNotify,
                                bVert ? WM_VSCROLL : WM_HSCROLL,
				(WPARAM)code, (LPARAM)infoPtr->hwndSelf);
}
    
static void TRACKBAR_RecalculateTics (TRACKBAR_INFO *infoPtr)
{
    int i, tic, nrTics;

    if (infoPtr->uTicFreq && infoPtr->lRangeMax >= infoPtr->lRangeMin)
    	nrTics=(infoPtr->lRangeMax - infoPtr->lRangeMin)/infoPtr->uTicFreq;
    else {
        nrTics = 0;
        Free (infoPtr->tics);
        infoPtr->tics = NULL;
        infoPtr->uNumTics = 0;
        return;
    }

    if (nrTics != infoPtr->uNumTics) {
    	infoPtr->tics=ReAlloc (infoPtr->tics,
                                        (nrTics+1)*sizeof (DWORD));
	if (!infoPtr->tics) {
	    infoPtr->uNumTics = 0;
	    notify(infoPtr, NM_OUTOFMEMORY);
	    return;
	}
    	infoPtr->uNumTics = nrTics;
    }

    tic = infoPtr->lRangeMin + infoPtr->uTicFreq;
    for (i = 0; i < nrTics; i++, tic += infoPtr->uTicFreq)
        infoPtr->tics[i] = tic;
}

/* converts from physical (mouse) position to logical position
   (in range of trackbar) */

static inline LONG
TRACKBAR_ConvertPlaceToPosition (const TRACKBAR_INFO *infoPtr, int place, int vertical)
{
    double range, width, pos, offsetthumb;

    range = infoPtr->lRangeMax - infoPtr->lRangeMin;
    if (vertical) {
        offsetthumb = (infoPtr->rcThumb.bottom - infoPtr->rcThumb.top)/2;
        width = infoPtr->rcChannel.bottom - infoPtr->rcChannel.top - (offsetthumb * 2) - 1;
        pos = (range*(place - infoPtr->rcChannel.top - offsetthumb)) / width;
    } else {
        offsetthumb = (infoPtr->rcThumb.right - infoPtr->rcThumb.left)/2;
        width = infoPtr->rcChannel.right - infoPtr->rcChannel.left - (offsetthumb * 2) - 1;
        pos = (range*(place - infoPtr->rcChannel.left - offsetthumb)) / width;
    }
    pos += infoPtr->lRangeMin;
    if (pos > infoPtr->lRangeMax)
        pos = infoPtr->lRangeMax;
    else if (pos < infoPtr->lRangeMin)
        pos = infoPtr->lRangeMin;

    TRACE("%.2f\n", pos);
    return (LONG)(pos + 0.5);
}


/* return: 0> prev, 0 none, >0 next */
static LONG
TRACKBAR_GetAutoPageDirection (const TRACKBAR_INFO *infoPtr, POINT clickPoint)
{
    DWORD dwStyle = GetWindowLongW (infoPtr->hwndSelf, GWL_STYLE);
    RECT pageRect;

    if (dwStyle & TBS_VERT) {
	pageRect.top = infoPtr->rcChannel.top;
	pageRect.bottom = infoPtr->rcChannel.bottom;
	pageRect.left = infoPtr->rcThumb.left;
	pageRect.right = infoPtr->rcThumb.right;
    } else {
	pageRect.top = infoPtr->rcThumb.top;
	pageRect.bottom = infoPtr->rcThumb.bottom;
	pageRect.left = infoPtr->rcChannel.left;
	pageRect.right = infoPtr->rcChannel.right;
    }


    if (PtInRect(&pageRect, clickPoint))
    {
	int clickPlace = (dwStyle & TBS_VERT) ? clickPoint.y : clickPoint.x;

        LONG clickPos = TRACKBAR_ConvertPlaceToPosition(infoPtr, clickPlace,
                                                        dwStyle & TBS_VERT);
	return clickPos - infoPtr->lPos;
    }

    return 0;
}

static inline void
TRACKBAR_PageDown (TRACKBAR_INFO *infoPtr)
{
    if (infoPtr->lPos == infoPtr->lRangeMax) return;

    infoPtr->lPos += infoPtr->lPageSize;
    if (infoPtr->lPos > infoPtr->lRangeMax)
	infoPtr->lPos = infoPtr->lRangeMax;
    notify_with_scroll (infoPtr, TB_PAGEDOWN);
}


static inline void
TRACKBAR_PageUp (TRACKBAR_INFO *infoPtr)
{
    if (infoPtr->lPos == infoPtr->lRangeMin) return;

    infoPtr->lPos -= infoPtr->lPageSize;
    if (infoPtr->lPos < infoPtr->lRangeMin)
        infoPtr->lPos = infoPtr->lRangeMin;
    notify_with_scroll (infoPtr, TB_PAGEUP);
}

static inline void TRACKBAR_LineUp(TRACKBAR_INFO *infoPtr)
{
    if (infoPtr->lPos == infoPtr->lRangeMin) return;
    infoPtr->lPos -= infoPtr->lLineSize;
    if (infoPtr->lPos < infoPtr->lRangeMin)
        infoPtr->lPos = infoPtr->lRangeMin;
    notify_with_scroll (infoPtr, TB_LINEUP);
}

static inline void TRACKBAR_LineDown(TRACKBAR_INFO *infoPtr)
{
    if (infoPtr->lPos == infoPtr->lRangeMax) return;
    infoPtr->lPos += infoPtr->lLineSize;
    if (infoPtr->lPos > infoPtr->lRangeMax)
        infoPtr->lPos = infoPtr->lRangeMax;
    notify_with_scroll (infoPtr, TB_LINEDOWN);
}

static void
TRACKBAR_CalcChannel (TRACKBAR_INFO *infoPtr)
{
    DWORD dwStyle = GetWindowLongW (infoPtr->hwndSelf, GWL_STYLE);
    INT cyChannel, offsetthumb, offsetedge;
    RECT lpRect, *channel = & infoPtr->rcChannel;

    GetClientRect (infoPtr->hwndSelf, &lpRect);

    offsetthumb = infoPtr->uThumbLen / 4;
    offsetedge  = offsetthumb + 3;
    cyChannel   = (dwStyle & TBS_ENABLESELRANGE) ? offsetthumb*3 : 4;
    if (dwStyle & TBS_VERT) {
        channel->top    = lpRect.top + offsetedge;
        channel->bottom = lpRect.bottom - offsetedge;
        if (dwStyle & TBS_ENABLESELRANGE)
            channel->left = lpRect.left + ((infoPtr->uThumbLen - cyChannel + 2) / 2);
        else
            channel->left = lpRect.left + (infoPtr->uThumbLen / 2) - 1;
        if (dwStyle & TBS_BOTH) {
            if (dwStyle & TBS_NOTICKS)
                channel->left += 1;
            else
                channel->left += 9;
        }
        else if (dwStyle & TBS_TOP) {
            if (dwStyle & TBS_NOTICKS)
                channel->left += 2;
            else
                channel->left += 10;
        }
        channel->right = channel->left + cyChannel;
    } else {
        channel->left = lpRect.left + offsetedge;
        channel->right = lpRect.right - offsetedge;
        if (dwStyle & TBS_ENABLESELRANGE)
            channel->top = lpRect.top + ((infoPtr->uThumbLen - cyChannel + 2) / 2);
        else
            channel->top = lpRect.top + (infoPtr->uThumbLen / 2) - 1;
        if (dwStyle & TBS_BOTH) {
            if (dwStyle & TBS_NOTICKS)
                channel->top += 1;
            else
                channel->top += 9;
        }
        else if (dwStyle & TBS_TOP) {
            if (dwStyle & TBS_NOTICKS)
                channel->top += 2;
            else
                channel->top += 10;
        }
        channel->bottom   = channel->top + cyChannel;
    }
}

static void
TRACKBAR_CalcThumb (const TRACKBAR_INFO *infoPtr, LONG lPos, RECT *thumb)
{
    int range, width, height, thumbwidth;
    DWORD dwStyle = GetWindowLongW (infoPtr->hwndSelf, GWL_STYLE);
    RECT lpRect;

    range = infoPtr->lRangeMax - infoPtr->lRangeMin;
    thumbwidth = (infoPtr->uThumbLen / 2) | 1;

    if (!range) range = 1;

    GetClientRect(infoPtr->hwndSelf, &lpRect);
    if (dwStyle & TBS_VERT)
    {
    	height = infoPtr->rcChannel.bottom - infoPtr->rcChannel.top - thumbwidth;

        if ((dwStyle & (TBS_BOTH | TBS_LEFT)) && !(dwStyle & TBS_NOTICKS))
            thumb->left = 10;
        else
            thumb->left = 2;
        thumb->right = thumb->left + infoPtr->uThumbLen;
        thumb->top = infoPtr->rcChannel.top +
                     (height*(lPos - infoPtr->lRangeMin))/range;
        thumb->bottom = thumb->top + thumbwidth;
    }
    else
    {
    	width = infoPtr->rcChannel.right - infoPtr->rcChannel.left - thumbwidth;

        thumb->left = infoPtr->rcChannel.left +
                      (width*(lPos - infoPtr->lRangeMin))/range;
        thumb->right = thumb->left + thumbwidth;
        if ((dwStyle & (TBS_BOTH | TBS_TOP)) && !(dwStyle & TBS_NOTICKS))
            thumb->top = 10;
        else
            thumb->top = 2;
        thumb->bottom = thumb->top + infoPtr->uThumbLen;
    }
}

static inline void
TRACKBAR_UpdateThumb (TRACKBAR_INFO *infoPtr)
{
    TRACKBAR_CalcThumb(infoPtr, infoPtr->lPos, &infoPtr->rcThumb);
}

static inline void
TRACKBAR_InvalidateAll (const TRACKBAR_INFO *infoPtr)
{
    InvalidateRect(infoPtr->hwndSelf, NULL, FALSE);
}

static void
TRACKBAR_InvalidateThumb (const TRACKBAR_INFO *infoPtr, LONG thumbPos)
{
    RECT rcThumb;

    TRACKBAR_CalcThumb(infoPtr, thumbPos, &rcThumb);
    InflateRect(&rcThumb, 1, 1);
    InvalidateRect(infoPtr->hwndSelf, &rcThumb, FALSE);
}

static inline void
TRACKBAR_InvalidateThumbMove (const TRACKBAR_INFO *infoPtr, LONG oldPos, LONG newPos)
{
    TRACKBAR_InvalidateThumb (infoPtr, oldPos);
    if (newPos != oldPos)
        TRACKBAR_InvalidateThumb (infoPtr, newPos);
}

static inline BOOL
TRACKBAR_HasSelection (const TRACKBAR_INFO *infoPtr)
{
    return infoPtr->lSelMin != infoPtr->lSelMax;
}

static void
TRACKBAR_CalcSelection (TRACKBAR_INFO *infoPtr)
{
    RECT *selection = &infoPtr->rcSelection;
    int range = infoPtr->lRangeMax - infoPtr->lRangeMin;
    int offsetthumb, height, width;

    if (range <= 0) {
        SetRectEmpty (selection);
    } else {
        if (GetWindowLongW (infoPtr->hwndSelf, GWL_STYLE) & TBS_VERT) {
            offsetthumb = (infoPtr->rcThumb.bottom - infoPtr->rcThumb.top)/2;
            height = infoPtr->rcChannel.bottom - infoPtr->rcChannel.top - offsetthumb*2;
            selection->top    = infoPtr->rcChannel.top + offsetthumb +
                (height*infoPtr->lSelMin)/range;
            selection->bottom = infoPtr->rcChannel.top + offsetthumb +
                (height*infoPtr->lSelMax)/range;
            selection->left   = infoPtr->rcChannel.left + 3;
            selection->right  = infoPtr->rcChannel.right - 3;
        } else {
            offsetthumb = (infoPtr->rcThumb.right - infoPtr->rcThumb.left)/2;
            width = infoPtr->rcChannel.right - infoPtr->rcChannel.left - offsetthumb*2;
            selection->left   = infoPtr->rcChannel.left + offsetthumb +
                (width*infoPtr->lSelMin)/range;
            selection->right  = infoPtr->rcChannel.left + offsetthumb +
                (width*infoPtr->lSelMax)/range;
            selection->top    = infoPtr->rcChannel.top + 3;
            selection->bottom = infoPtr->rcChannel.bottom - 3;
        }
    }

    TRACE("selection[left=%d, top=%d, right=%d, bottom=%d]\n",
	   selection->left, selection->top, selection->right, selection->bottom);
}

static BOOL
TRACKBAR_AutoPage (TRACKBAR_INFO *infoPtr, POINT clickPoint)
{
    LONG dir = TRACKBAR_GetAutoPageDirection(infoPtr, clickPoint);
    LONG prevPos = infoPtr->lPos;

    TRACE("x=%d, y=%d, dir=%d\n", clickPoint.x, clickPoint.y, dir);

    if (dir > 0 && (infoPtr->flags & TB_AUTO_PAGE_RIGHT))
	TRACKBAR_PageDown(infoPtr);
    else if (dir < 0 && (infoPtr->flags & TB_AUTO_PAGE_LEFT))
	TRACKBAR_PageUp(infoPtr);
    else return FALSE;

    infoPtr->flags |= TB_THUMBPOSCHANGED;
    TRACKBAR_InvalidateThumbMove (infoPtr, prevPos, infoPtr->lPos);

    return TRUE;
}

/* Trackbar drawing code. I like my spaghetti done milanese.  */

static void
TRACKBAR_DrawChannel (const TRACKBAR_INFO *infoPtr, HDC hdc, DWORD dwStyle)
{
    RECT rcChannel = infoPtr->rcChannel;
    HTHEME theme = GetWindowTheme (infoPtr->hwndSelf);

    if (theme)
    {
        DrawThemeBackground (theme, hdc, 
            (GetWindowLongW (infoPtr->hwndSelf, GWL_STYLE) & TBS_VERT) ? 
                TKP_TRACKVERT : TKP_TRACK, TKS_NORMAL, &rcChannel, 0);
    }
    else
    {
        DrawEdge (hdc, &rcChannel, EDGE_SUNKEN, BF_RECT | BF_ADJUST);
        if (dwStyle & TBS_ENABLESELRANGE) {		 /* fill the channel */
            FillRect (hdc, &rcChannel, GetStockObject(WHITE_BRUSH));
            if (TRACKBAR_HasSelection(infoPtr))
                FillRect (hdc, &infoPtr->rcSelection, GetSysColorBrush(COLOR_HIGHLIGHT));
        }
    }
}

static void
TRACKBAR_DrawOneTic (const TRACKBAR_INFO *infoPtr, HDC hdc, LONG ticPos, int flags)
{
    int x, y, ox, oy, range, side, indent = 0, len = 3;
    int offsetthumb;
    RECT rcTics;

    if (flags & TBS_VERT) {
        offsetthumb = (infoPtr->rcThumb.bottom - infoPtr->rcThumb.top)/2;
	rcTics.left = infoPtr->rcThumb.left - 2;
	rcTics.right = infoPtr->rcThumb.right + 2;
	rcTics.top    = infoPtr->rcChannel.top + offsetthumb + 1;
	rcTics.bottom = infoPtr->rcChannel.bottom - offsetthumb;
    } else {
        offsetthumb = (infoPtr->rcThumb.right - infoPtr->rcThumb.left)/2;
	rcTics.left   = infoPtr->rcChannel.left + offsetthumb + 1;
	rcTics.right  = infoPtr->rcChannel.right - offsetthumb;
	rcTics.top = infoPtr->rcThumb.top - 2;
	rcTics.bottom = infoPtr->rcThumb.bottom + 2;
    }

    if (flags & (TBS_TOP | TBS_LEFT)) {
	x = rcTics.left;
	y = rcTics.top;
	side = -1;
    } else {
  	x = rcTics.right;
  	y = rcTics.bottom;
	side = 1;
    }

    range = infoPtr->lRangeMax - infoPtr->lRangeMin;
    if (range <= 0)
      range = 1; /* to avoid division by zero */

    if (flags & TIC_SELECTIONMARK) {
  	indent = (flags & TIC_SELECTIONMARKMIN) ? -1 : 1;
    } else if (flags & TIC_EDGE) {
	len++;
    }

    if (flags & TBS_VERT) {
	int height = rcTics.bottom - rcTics.top;
	y = rcTics.top + (height*(ticPos - infoPtr->lRangeMin))/range;
    } else {
        int width = rcTics.right - rcTics.left;
        x = rcTics.left + (width*(ticPos - infoPtr->lRangeMin))/range;
    }

    ox = x;
    oy = y;
    MoveToEx(hdc, x, y, 0);
    if (flags & TBS_VERT) x += len * side;
    else y += len * side;
    LineTo(hdc, x, y);
	    
    if (flags & TIC_SELECTIONMARK) {
	if (flags & TBS_VERT) {
	    x -= side;
	} else {
	    y -= side;
	}
	MoveToEx(hdc, x, y, 0);
	if (flags & TBS_VERT) {
	    y += 2 * indent;
	} else {
	    x += 2 * indent;
	}
	
	LineTo(hdc, x, y);
	LineTo(hdc, ox, oy);
    }
}


static inline void
TRACKBAR_DrawTic (const TRACKBAR_INFO *infoPtr, HDC hdc, LONG ticPos, int flags)
{
    if ((flags & (TBS_LEFT | TBS_TOP)) || (flags & TBS_BOTH))
        TRACKBAR_DrawOneTic (infoPtr, hdc, ticPos, flags | TBS_LEFT);

    if (!(flags & (TBS_LEFT | TBS_TOP)) || (flags & TBS_BOTH))
        TRACKBAR_DrawOneTic (infoPtr, hdc, ticPos, flags & ~TBS_LEFT);
}

static void
TRACKBAR_DrawTics (const TRACKBAR_INFO *infoPtr, HDC hdc, DWORD dwStyle)
{
    unsigned int i;
    int ticFlags = dwStyle & 0x0f;
    LOGPEN ticPen = { PS_SOLID, {1, 0}, GetSysColor (COLOR_3DDKSHADOW) };
    HPEN hOldPen, hTicPen;
    HTHEME theme = GetWindowTheme (infoPtr->hwndSelf);
    
    if (theme)
    {
        int part = (dwStyle & TBS_VERT) ? TKP_TICSVERT : TKP_TICS;
        GetThemeColor (theme, part, TSS_NORMAL, TMT_COLOR, &ticPen.lopnColor);
    }
    /* create the pen to draw the tics with */
    hTicPen = CreatePenIndirect(&ticPen);
    hOldPen = hTicPen ? SelectObject(hdc, hTicPen) : 0;

    /* actually draw the tics */
    for (i=0; i<infoPtr->uNumTics; i++)
        TRACKBAR_DrawTic (infoPtr, hdc, infoPtr->tics[i], ticFlags);

    TRACKBAR_DrawTic (infoPtr, hdc, infoPtr->lRangeMin, ticFlags | TIC_EDGE);
    TRACKBAR_DrawTic (infoPtr, hdc, infoPtr->lRangeMax, ticFlags | TIC_EDGE);

    if ((dwStyle & TBS_ENABLESELRANGE) && TRACKBAR_HasSelection(infoPtr)) {
        TRACKBAR_DrawTic (infoPtr, hdc, infoPtr->lSelMin,
                          ticFlags | TIC_SELECTIONMARKMIN);
        TRACKBAR_DrawTic (infoPtr, hdc, infoPtr->lSelMax,
                          ticFlags | TIC_SELECTIONMARKMAX);
    }
    
    /* clean up the pen, if we created one */
    if (hTicPen) {
	SelectObject(hdc, hOldPen);
	DeleteObject(hTicPen);
    }
}

static void
TRACKBAR_DrawThumb (const TRACKBAR_INFO *infoPtr, HDC hdc, DWORD dwStyle)
{
    HBRUSH oldbr;
    HPEN  oldpen;
    RECT thumb = infoPtr->rcThumb;
    int BlackUntil = 3;
    int PointCount = 6;
    POINT points[6];
    int fillClr;
    int PointDepth;
    HTHEME theme = GetWindowTheme (infoPtr->hwndSelf);
    
    if (theme)
    {
        int partId;
        int stateId;
        if (dwStyle & TBS_BOTH)
            partId = (dwStyle & TBS_VERT) ? TKP_THUMBVERT : TKP_THUMB;
        else if (dwStyle & TBS_LEFT)
            partId = (dwStyle & TBS_VERT) ? TKP_THUMBLEFT : TKP_THUMBTOP;
        else
            partId = (dwStyle & TBS_VERT) ? TKP_THUMBRIGHT : TKP_THUMBBOTTOM;
            
        if (dwStyle & WS_DISABLED)
            stateId = TUS_DISABLED;
        else if (infoPtr->flags & TB_DRAG_MODE)
            stateId = TUS_PRESSED;
        else if (infoPtr->flags & TB_THUMB_HOT)
            stateId = TUS_HOT;
        else
            stateId = TUS_NORMAL;
        
        DrawThemeBackground (theme, hdc, partId, stateId, &thumb, 0);
        
        return;
    }

    fillClr = infoPtr->flags & TB_DRAG_MODE ? COLOR_BTNHILIGHT : COLOR_BTNFACE;
    oldbr = SelectObject (hdc, GetSysColorBrush(fillClr));
    SetPolyFillMode (hdc, WINDING);

    if (dwStyle & TBS_BOTH)
    {
       points[0].x=thumb.right;
       points[0].y=thumb.top;
       points[1].x=thumb.right;
       points[1].y=thumb.bottom;
       points[2].x=thumb.left;
       points[2].y=thumb.bottom;
       points[3].x=thumb.left;
       points[3].y=thumb.top;
       points[4].x=points[0].x;
       points[4].y=points[0].y;
       PointCount = 5;
       BlackUntil = 3;
    }
    else
    {
        if (dwStyle & TBS_VERT)
        {
          PointDepth = (thumb.bottom - thumb.top) / 2;
          if (dwStyle & TBS_LEFT)
          {
            points[0].x=thumb.right;
            points[0].y=thumb.top;
            points[1].x=thumb.right;
            points[1].y=thumb.bottom;
            points[2].x=thumb.left + PointDepth;
            points[2].y=thumb.bottom;
            points[3].x=thumb.left;
            points[3].y=(thumb.bottom - thumb.top) / 2 + thumb.top + 1;
            points[4].x=thumb.left + PointDepth;
            points[4].y=thumb.top;
            points[5].x=points[0].x;
            points[5].y=points[0].y;
            BlackUntil = 4;
          }
          else
          {
            points[0].x=thumb.right;
            points[0].y=(thumb.bottom - thumb.top) / 2 + thumb.top + 1;
            points[1].x=thumb.right - PointDepth;
            points[1].y=thumb.bottom;
            points[2].x=thumb.left;
            points[2].y=thumb.bottom;
            points[3].x=thumb.left;
            points[3].y=thumb.top;
            points[4].x=thumb.right - PointDepth;
            points[4].y=thumb.top;
            points[5].x=points[0].x;
            points[5].y=points[0].y;
          }
        }
        else
        {
          PointDepth = (thumb.right - thumb.left) / 2;
          if (dwStyle & TBS_TOP)
          {
            points[0].x=(thumb.right - thumb.left) / 2 + thumb.left + 1;
            points[0].y=thumb.top;
            points[1].x=thumb.right;
            points[1].y=thumb.top + PointDepth;
            points[2].x=thumb.right;
            points[2].y=thumb.bottom;
            points[3].x=thumb.left;
            points[3].y=thumb.bottom;
            points[4].x=thumb.left;
            points[4].y=thumb.top + PointDepth;
            points[5].x=points[0].x;
            points[5].y=points[0].y;
            BlackUntil = 4;
          }
          else
          {
            points[0].x=thumb.right;
            points[0].y=thumb.top;
            points[1].x=thumb.right;
            points[1].y=thumb.bottom - PointDepth;
            points[2].x=(thumb.right - thumb.left) / 2 + thumb.left + 1;
            points[2].y=thumb.bottom;
            points[3].x=thumb.left;
            points[3].y=thumb.bottom - PointDepth;
            points[4].x=thumb.left;
            points[4].y=thumb.top;
            points[5].x=points[0].x;
            points[5].y=points[0].y;
          }
        }

    }

    /* Draw the thumb now */
    Polygon (hdc, points, PointCount);
    oldpen = SelectObject(hdc, GetStockObject(BLACK_PEN));
    Polyline(hdc,points, BlackUntil);
    SelectObject(hdc, GetStockObject(WHITE_PEN));
    Polyline(hdc, &points[BlackUntil-1], PointCount+1-BlackUntil);
    SelectObject(hdc, oldpen);
    SelectObject(hdc, oldbr);
}


static inline void
TRACKBAR_ActivateToolTip (const TRACKBAR_INFO *infoPtr, BOOL fShow)
{
    TTTOOLINFOW ti;

    if (!infoPtr->hwndToolTip) return;

    ZeroMemory(&ti, sizeof(ti));
    ti.cbSize = sizeof(ti);
    ti.hwnd   = infoPtr->hwndSelf;

    SendMessageW (infoPtr->hwndToolTip, TTM_TRACKACTIVATE, fShow, (LPARAM)&ti);
}


static void
TRACKBAR_UpdateToolTip (const TRACKBAR_INFO *infoPtr)
{
    DWORD dwStyle = GetWindowLongW (infoPtr->hwndSelf, GWL_STYLE);
    WCHAR buf[80];
    static const WCHAR fmt[] = { '%', 'l', 'd', 0 };
    TTTOOLINFOW ti;
    POINT pt;
    RECT rcClient;
    LRESULT size;

    if (!infoPtr->hwndToolTip) return;

    ZeroMemory(&ti, sizeof(ti));
    ti.cbSize = sizeof(ti);
    ti.hwnd   = infoPtr->hwndSelf;
    ti.uFlags = TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE;

    wsprintfW (buf, fmt, infoPtr->lPos);
    ti.lpszText = buf;
    SendMessageW (infoPtr->hwndToolTip, TTM_UPDATETIPTEXTW, 0, (LPARAM)&ti);

    GetClientRect (infoPtr->hwndSelf, &rcClient);
    size = SendMessageW (infoPtr->hwndToolTip, TTM_GETBUBBLESIZE, 0, (LPARAM)&ti);
    if (dwStyle & TBS_VERT) {
	if (infoPtr->fLocation == TBTS_LEFT)
	    pt.x = 0 - LOWORD(size) - TOOLTIP_OFFSET;
	else
	    pt.x = rcClient.right + TOOLTIP_OFFSET;
    	pt.y = (infoPtr->rcThumb.top + infoPtr->rcThumb.bottom - HIWORD(size))/2;
    } else {
	if (infoPtr->fLocation == TBTS_TOP)
	    pt.y = 0 - HIWORD(size) - TOOLTIP_OFFSET;
	else
            pt.y = rcClient.bottom + TOOLTIP_OFFSET;
        pt.x = (infoPtr->rcThumb.left + infoPtr->rcThumb.right - LOWORD(size))/2;
    }
    ClientToScreen(infoPtr->hwndSelf, &pt);

    SendMessageW (infoPtr->hwndToolTip, TTM_TRACKPOSITION,
                  0, MAKELPARAM(pt.x, pt.y));
}


static void
TRACKBAR_Refresh (TRACKBAR_INFO *infoPtr, HDC hdcDst)
{
    DWORD dwStyle = GetWindowLongW (infoPtr->hwndSelf, GWL_STYLE);
    RECT rcClient;
    HDC hdc;
    HBITMAP hOldBmp = 0, hOffScreenBmp = 0;
    NMCUSTOMDRAW nmcd;
    int gcdrf, icdrf;
    HTHEME theme;

    if (infoPtr->flags & TB_THUMBCHANGED) {
        TRACKBAR_UpdateThumb (infoPtr);
        if (infoPtr->flags & TB_THUMBSIZECHANGED)
            TRACKBAR_CalcChannel (infoPtr);
    }
    if (infoPtr->flags & TB_SELECTIONCHANGED)
        TRACKBAR_CalcSelection (infoPtr);

    if (infoPtr->flags & TB_DRAG_MODE)
        TRACKBAR_UpdateToolTip (infoPtr);

    infoPtr->flags &= ~ (TB_THUMBCHANGED | TB_SELECTIONCHANGED);

    GetClientRect (infoPtr->hwndSelf, &rcClient);
    
    /* try to render offscreen, if we fail, carrry onscreen */
    hdc = CreateCompatibleDC(hdcDst);
    if (hdc) {
        hOffScreenBmp = CreateCompatibleBitmap(hdcDst, rcClient.right, rcClient.bottom);
        if (hOffScreenBmp) {
	    hOldBmp = SelectObject(hdc, hOffScreenBmp);
	} else {
	    DeleteObject(hdc);
	    hdc = hdcDst;
	}
    } else {
	hdc = hdcDst;
    }

    ZeroMemory(&nmcd, sizeof(nmcd));
    nmcd.hdr.hwndFrom = infoPtr->hwndSelf;
    nmcd.hdr.idFrom = GetWindowLongPtrW (infoPtr->hwndSelf, GWLP_ID);
    nmcd.hdr.code = NM_CUSTOMDRAW;
    nmcd.hdc = hdc;

    /* start the paint cycle */
    nmcd.rc = rcClient;
    gcdrf = notify_customdraw(infoPtr, &nmcd, CDDS_PREPAINT);
    if (gcdrf & CDRF_SKIPDEFAULT) goto cleanup;
    
    /* Erase backbround */
    if (gcdrf == CDRF_DODEFAULT ||
        notify_customdraw(infoPtr, &nmcd, CDDS_PREERASE) != CDRF_SKIPDEFAULT) {
        if ((theme = GetWindowTheme (infoPtr->hwndSelf))) {
            DrawThemeBackground (theme, hdc,
                (GetWindowLongW (infoPtr->hwndSelf, GWL_STYLE) & TBS_VERT) ?
                    TKP_TRACKVERT : TKP_TRACK, TKS_NORMAL, &rcClient, 0);
            DrawThemeParentBackground (infoPtr->hwndSelf, hdc, &rcClient);
        }
        else
	    FillRect (hdc, &rcClient, GetSysColorBrush(COLOR_BTNFACE));
        if (gcdrf != CDRF_DODEFAULT)
	    notify_customdraw(infoPtr, &nmcd, CDDS_POSTERASE);
    }
    
    /* draw channel */
    if (gcdrf & CDRF_NOTIFYITEMDRAW) {
        nmcd.dwItemSpec = TBCD_CHANNEL;
	nmcd.uItemState = CDIS_DEFAULT;
	nmcd.rc = infoPtr->rcChannel;
	icdrf = notify_customdraw(infoPtr, &nmcd, CDDS_ITEMPREPAINT);
    } else icdrf = CDRF_DODEFAULT;
    if ( !(icdrf & CDRF_SKIPDEFAULT) ) {
	TRACKBAR_DrawChannel (infoPtr, hdc, dwStyle);
	if (icdrf & CDRF_NOTIFYPOSTPAINT)
	    notify_customdraw(infoPtr, &nmcd, CDDS_ITEMPOSTPAINT);
    }


    /* draw tics */
    if (!(dwStyle & TBS_NOTICKS)) {
    	if (gcdrf & CDRF_NOTIFYITEMDRAW) {
            nmcd.dwItemSpec = TBCD_TICS;
	    nmcd.uItemState = CDIS_DEFAULT;
	    nmcd.rc = rcClient;
	    icdrf = notify_customdraw(infoPtr, &nmcd, CDDS_ITEMPREPAINT);
        } else icdrf = CDRF_DODEFAULT;
	if ( !(icdrf & CDRF_SKIPDEFAULT) ) {
	    TRACKBAR_DrawTics (infoPtr, hdc, dwStyle);
	    if (icdrf & CDRF_NOTIFYPOSTPAINT)
		notify_customdraw(infoPtr, &nmcd, CDDS_ITEMPOSTPAINT);
	}
    }
    
    /* draw thumb */
    if (!(dwStyle & TBS_NOTHUMB)) {
	if (gcdrf & CDRF_NOTIFYITEMDRAW) {
	    nmcd.dwItemSpec = TBCD_THUMB;
	    nmcd.uItemState = infoPtr->flags & TB_DRAG_MODE ? CDIS_HOT : CDIS_DEFAULT;
	    nmcd.rc = infoPtr->rcThumb;
	    icdrf = notify_customdraw(infoPtr, &nmcd, CDDS_ITEMPREPAINT);
	} else icdrf = CDRF_DODEFAULT;
	if ( !(icdrf & CDRF_SKIPDEFAULT) ) {
            TRACKBAR_DrawThumb(infoPtr, hdc, dwStyle);
	    if (icdrf & CDRF_NOTIFYPOSTPAINT)
		notify_customdraw(infoPtr, &nmcd, CDDS_ITEMPOSTPAINT);
	}
    }

    /* draw focus rectangle */
    if (infoPtr->bFocussed) {
	DrawFocusRect(hdc, &rcClient);
    }

    /* finish up the painting */
    if (gcdrf & CDRF_NOTIFYPOSTPAINT)
	notify_customdraw(infoPtr, &nmcd, CDDS_POSTPAINT);
    
cleanup:
    /* cleanup, if we rendered offscreen */
    if (hdc != hdcDst) {
	BitBlt(hdcDst, 0, 0, rcClient.right, rcClient.bottom, hdc, 0, 0, SRCCOPY);
	SelectObject(hdc, hOldBmp);
	DeleteObject(hOffScreenBmp);
	DeleteObject(hdc);
    }
}


static void
TRACKBAR_AlignBuddies (const TRACKBAR_INFO *infoPtr)
{
    DWORD dwStyle = GetWindowLongW (infoPtr->hwndSelf, GWL_STYLE);
    HWND hwndParent = GetParent (infoPtr->hwndSelf);
    RECT rcSelf, rcBuddy;
    INT x, y;

    GetWindowRect (infoPtr->hwndSelf, &rcSelf);
    MapWindowPoints (HWND_DESKTOP, hwndParent, (LPPOINT)&rcSelf, 2);

    /* align buddy left or above */
    if (infoPtr->hwndBuddyLA) {
	GetWindowRect (infoPtr->hwndBuddyLA, &rcBuddy);
	MapWindowPoints (HWND_DESKTOP, hwndParent, (LPPOINT)&rcBuddy, 2);

	if (dwStyle & TBS_VERT) {
	    x = (infoPtr->rcChannel.right + infoPtr->rcChannel.left) / 2 -
		(rcBuddy.right - rcBuddy.left) / 2 + rcSelf.left;
	    y = rcSelf.top - (rcBuddy.bottom - rcBuddy.top);
	}
	else {
	    x = rcSelf.left - (rcBuddy.right - rcBuddy.left);
	    y = (infoPtr->rcChannel.bottom + infoPtr->rcChannel.top) / 2 -
		(rcBuddy.bottom - rcBuddy.top) / 2 + rcSelf.top;
	}

	SetWindowPos (infoPtr->hwndBuddyLA, 0, x, y, 0, 0,
                      SWP_NOZORDER | SWP_NOSIZE);
    }


    /* align buddy right or below */
    if (infoPtr->hwndBuddyRB) {
	GetWindowRect (infoPtr->hwndBuddyRB, &rcBuddy);
	MapWindowPoints (HWND_DESKTOP, hwndParent, (LPPOINT)&rcBuddy, 2);

	if (dwStyle & TBS_VERT) {
	    x = (infoPtr->rcChannel.right + infoPtr->rcChannel.left) / 2 -
		(rcBuddy.right - rcBuddy.left) / 2 + rcSelf.left;
	    y = rcSelf.bottom;
	}
	else {
	    x = rcSelf.right;
	    y = (infoPtr->rcChannel.bottom + infoPtr->rcChannel.top) / 2 -
		(rcBuddy.bottom - rcBuddy.top) / 2 + rcSelf.top;
	}
	SetWindowPos (infoPtr->hwndBuddyRB, 0, x, y, 0, 0,
                      SWP_NOZORDER | SWP_NOSIZE);
    }
}


static LRESULT
TRACKBAR_ClearSel (TRACKBAR_INFO *infoPtr, BOOL fRedraw)
{
    infoPtr->lSelMin = 0;
    infoPtr->lSelMax = 0;
    infoPtr->flags |= TB_SELECTIONCHANGED;

    if (fRedraw) TRACKBAR_InvalidateAll(infoPtr);

    return 0;
}


static LRESULT
TRACKBAR_ClearTics (TRACKBAR_INFO *infoPtr, BOOL fRedraw)
{
    if (infoPtr->tics) {
        Free (infoPtr->tics);
        infoPtr->tics = NULL;
        infoPtr->uNumTics = 0;
    }

    if (fRedraw) TRACKBAR_InvalidateAll(infoPtr);

    return 0;
}


static inline LRESULT
TRACKBAR_GetChannelRect (const TRACKBAR_INFO *infoPtr, LPRECT lprc)
{
    if (lprc == NULL) return 0;

    lprc->left   = infoPtr->rcChannel.left;
    lprc->right  = infoPtr->rcChannel.right;
    lprc->bottom = infoPtr->rcChannel.bottom;
    lprc->top    = infoPtr->rcChannel.top;

    return 0;
}


static inline LONG
TRACKBAR_GetNumTics (const TRACKBAR_INFO *infoPtr)
{
    if (GetWindowLongW (infoPtr->hwndSelf, GWL_STYLE) & TBS_NOTICKS)
        return 0;

    if(infoPtr->uNumTics == 0)
        return 2;
    else
        return infoPtr->uNumTics + 1;
}


static int comp_tics (const void *ap, const void *bp)
{
    const DWORD a = *(const DWORD *)ap;
    const DWORD b = *(const DWORD *)bp;

    TRACE("(a=%d, b=%d)\n", a, b);
    if (a < b) return -1;
    if (a > b) return 1;
    return 0;
}


static inline LONG
TRACKBAR_GetTic (const TRACKBAR_INFO *infoPtr, INT iTic)
{
    if ((iTic < 0) || (iTic >= infoPtr->uNumTics) || !infoPtr->tics)
	return -1;

    qsort(infoPtr->tics, infoPtr->uNumTics, sizeof(DWORD), comp_tics);
    return infoPtr->tics[iTic];
}


static inline LONG
TRACKBAR_GetTicPos (const TRACKBAR_INFO *infoPtr, INT iTic)
{
    LONG range, width, pos, tic;
    int offsetthumb;

    if ((iTic < 0) || (iTic >= infoPtr->uNumTics) || !infoPtr->tics)
	return -1;

    tic   = TRACKBAR_GetTic (infoPtr, iTic);
    range = infoPtr->lRangeMax - infoPtr->lRangeMin;
    if (range <= 0) range = 1;
    offsetthumb = (infoPtr->rcThumb.right - infoPtr->rcThumb.left)/2;
    width = infoPtr->rcChannel.right - infoPtr->rcChannel.left - offsetthumb*2;
    pos   = infoPtr->rcChannel.left + offsetthumb + (width * tic) / range;

    return pos;
}


static HWND
TRACKBAR_SetBuddy (TRACKBAR_INFO *infoPtr, BOOL fLocation, HWND hwndBuddy)
{
    HWND hwndTemp;

    if (fLocation) {
	/* buddy is left or above */
	hwndTemp = infoPtr->hwndBuddyLA;
	infoPtr->hwndBuddyLA = hwndBuddy;
    }
    else {
        /* buddy is right or below */
        hwndTemp = infoPtr->hwndBuddyRB;
        infoPtr->hwndBuddyRB = hwndBuddy;
    }

    TRACKBAR_AlignBuddies (infoPtr);

    return hwndTemp;
}


static inline LONG
TRACKBAR_SetLineSize (TRACKBAR_INFO *infoPtr, LONG lLineSize)
{
    LONG lTemp = infoPtr->lLineSize;

    infoPtr->lLineSize = lLineSize;

    return lTemp;
}


static inline LONG
TRACKBAR_SetPageSize (TRACKBAR_INFO *infoPtr, LONG lPageSize)
{
    LONG lTemp = infoPtr->lPageSize;

    infoPtr->lPageSize = lPageSize;

    return lTemp;
}


static inline LRESULT
TRACKBAR_SetPos (TRACKBAR_INFO *infoPtr, BOOL fPosition, LONG lPosition)
{
    LONG oldPos = infoPtr->lPos;
    infoPtr->lPos = lPosition;

    if (infoPtr->lPos < infoPtr->lRangeMin)
	infoPtr->lPos = infoPtr->lRangeMin;

    if (infoPtr->lPos > infoPtr->lRangeMax)
	infoPtr->lPos = infoPtr->lRangeMax;
    infoPtr->flags |= TB_THUMBPOSCHANGED;

    if (fPosition) TRACKBAR_InvalidateThumbMove(infoPtr, oldPos, lPosition);

    return 0;
}


static inline LRESULT
TRACKBAR_SetRange (TRACKBAR_INFO *infoPtr, BOOL fRedraw, LONG lRange)
{
    infoPtr->lRangeMin = (SHORT)LOWORD(lRange);
    infoPtr->lRangeMax = (SHORT)HIWORD(lRange);

    if (infoPtr->lPos < infoPtr->lRangeMin) {
        infoPtr->lPos = infoPtr->lRangeMin;
        infoPtr->flags |= TB_THUMBPOSCHANGED;
    }

    if (infoPtr->lPos > infoPtr->lRangeMax) {
        infoPtr->lPos = infoPtr->lRangeMax;
        infoPtr->flags |= TB_THUMBPOSCHANGED;
    }

    infoPtr->lPageSize = (infoPtr->lRangeMax - infoPtr->lRangeMin) / 5;
    if (infoPtr->lPageSize == 0) infoPtr->lPageSize = 1;

    if (fRedraw) TRACKBAR_InvalidateAll(infoPtr);

    return 0;
}


static inline LRESULT
TRACKBAR_SetRangeMax (TRACKBAR_INFO *infoPtr, BOOL fRedraw, LONG lMax)
{
    infoPtr->lRangeMax = lMax;
    if (infoPtr->lPos > infoPtr->lRangeMax) {
        infoPtr->lPos = infoPtr->lRangeMax;
        infoPtr->flags |= TB_THUMBPOSCHANGED;
    }

    infoPtr->lPageSize = (infoPtr->lRangeMax - infoPtr->lRangeMin) / 5;
    if (infoPtr->lPageSize == 0) infoPtr->lPageSize = 1;

    if (fRedraw) TRACKBAR_InvalidateAll(infoPtr);

    return 0;
}


static inline LRESULT
TRACKBAR_SetRangeMin (TRACKBAR_INFO *infoPtr, BOOL fRedraw, LONG lMin)
{
    infoPtr->lRangeMin = lMin;
    if (infoPtr->lPos < infoPtr->lRangeMin) {
        infoPtr->lPos = infoPtr->lRangeMin;
        infoPtr->flags |= TB_THUMBPOSCHANGED;
    }

    infoPtr->lPageSize = (infoPtr->lRangeMax - infoPtr->lRangeMin) / 5;
    if (infoPtr->lPageSize == 0) infoPtr->lPageSize = 1;

    if (fRedraw) TRACKBAR_InvalidateAll(infoPtr);

    return 0;
}


static inline LRESULT
TRACKBAR_SetSel (TRACKBAR_INFO *infoPtr, BOOL fRedraw, LONG lSel)
{
    if (!(GetWindowLongW (infoPtr->hwndSelf, GWL_STYLE) & TBS_ENABLESELRANGE)){
        infoPtr->lSelMin = 0;
        infoPtr->lSelMax = 0;
        return 0;
    }

    infoPtr->lSelMin = (SHORT)LOWORD(lSel);
    infoPtr->lSelMax = (SHORT)HIWORD(lSel);
    infoPtr->flags |= TB_SELECTIONCHANGED;

    if (infoPtr->lSelMin < infoPtr->lRangeMin)
        infoPtr->lSelMin = infoPtr->lRangeMin;
    if (infoPtr->lSelMax > infoPtr->lRangeMax)
        infoPtr->lSelMax = infoPtr->lRangeMax;

    if (fRedraw) TRACKBAR_InvalidateAll(infoPtr);

    return 0;
}


static inline LRESULT
TRACKBAR_SetSelEnd (TRACKBAR_INFO *infoPtr, BOOL fRedraw, LONG lEnd)
{
    if (!(GetWindowLongW (infoPtr->hwndSelf, GWL_STYLE) & TBS_ENABLESELRANGE)){
        infoPtr->lSelMax = 0;
	return 0;
    }

    infoPtr->lSelMax = lEnd;
    infoPtr->flags |= TB_SELECTIONCHANGED;

    if (infoPtr->lSelMax > infoPtr->lRangeMax)
        infoPtr->lSelMax = infoPtr->lRangeMax;

    if (fRedraw) TRACKBAR_InvalidateAll(infoPtr);

    return 0;
}


static inline LRESULT
TRACKBAR_SetSelStart (TRACKBAR_INFO *infoPtr, BOOL fRedraw, LONG lStart)
{
    if (!(GetWindowLongW (infoPtr->hwndSelf, GWL_STYLE) & TBS_ENABLESELRANGE)){
        infoPtr->lSelMin = 0;
	return 0;
    }

    infoPtr->lSelMin = lStart;
    infoPtr->flags  |=TB_SELECTIONCHANGED;

    if (infoPtr->lSelMin < infoPtr->lRangeMin)
        infoPtr->lSelMin = infoPtr->lRangeMin;

    if (fRedraw) TRACKBAR_InvalidateAll(infoPtr);

    return 0;
}


static inline LRESULT
TRACKBAR_SetThumbLength (TRACKBAR_INFO *infoPtr, UINT iLength)
{
    if (GetWindowLongW (infoPtr->hwndSelf, GWL_STYLE) & TBS_FIXEDLENGTH) {
        infoPtr->uThumbLen = iLength;
	infoPtr->flags |= TB_THUMBSIZECHANGED;
	InvalidateRect (infoPtr->hwndSelf, &infoPtr->rcThumb, FALSE);
    }

    return 0;
}


static inline LRESULT
TRACKBAR_SetTic (TRACKBAR_INFO *infoPtr, LONG lPos)
{
    if (GetWindowLongW (infoPtr->hwndSelf, GWL_STYLE) & TBS_AUTOTICKS)
	return FALSE;

    if ((lPos < infoPtr->lRangeMin) || (lPos> infoPtr->lRangeMax))
        return FALSE;

    TRACE("lPos=%d\n", lPos);

    infoPtr->uNumTics++;
    infoPtr->tics=ReAlloc( infoPtr->tics,
                                    (infoPtr->uNumTics)*sizeof (DWORD));
    if (!infoPtr->tics) {
	infoPtr->uNumTics = 0;
	notify(infoPtr, NM_OUTOFMEMORY);
	return FALSE;
    }
    infoPtr->tics[infoPtr->uNumTics-1] = lPos;

    TRACKBAR_InvalidateAll(infoPtr);

    return TRUE;
}


static inline LRESULT
TRACKBAR_SetTicFreq (TRACKBAR_INFO *infoPtr, WORD wFreq)
{
    if (GetWindowLongW (infoPtr->hwndSelf, GWL_STYLE) & TBS_AUTOTICKS) {
        infoPtr->uTicFreq = wFreq;
	TRACKBAR_RecalculateTics (infoPtr);
	TRACKBAR_InvalidateAll(infoPtr);
    }

    return 0;
}


static inline INT
TRACKBAR_SetTipSide (TRACKBAR_INFO *infoPtr, INT fLocation)
{
    INT fTemp = infoPtr->fLocation;

    infoPtr->fLocation = fLocation;

    return fTemp;
}


static inline LRESULT
TRACKBAR_SetToolTips (TRACKBAR_INFO *infoPtr, HWND hwndTT)
{
    infoPtr->hwndToolTip = hwndTT;

    return 0;
}


static inline BOOL
TRACKBAR_SetUnicodeFormat (TRACKBAR_INFO *infoPtr, BOOL fUnicode)
{
    BOOL bTemp = infoPtr->bUnicode;

    infoPtr->bUnicode = fUnicode;

    return bTemp;
}


static LRESULT
TRACKBAR_InitializeThumb (TRACKBAR_INFO *infoPtr)
{
    DWORD dwStyle = GetWindowLongW (infoPtr->hwndSelf, GWL_STYLE);
    RECT rect;
    int clientWidth, clientMetric;

    /* initial thumb length */
    clientMetric = (dwStyle & TBS_ENABLESELRANGE) ? 23 : 21;
    GetClientRect(infoPtr->hwndSelf,&rect);
    if (dwStyle & TBS_VERT) {
	clientWidth = rect.right - rect.left;
    } else {
	clientWidth = rect.bottom - rect.top;
    }
    if (clientWidth >= clientMetric)
        infoPtr->uThumbLen = clientMetric;
    else
        infoPtr->uThumbLen = clientWidth > 9 ? clientWidth - 6 : 4;

    TRACKBAR_CalcChannel (infoPtr);
    TRACKBAR_UpdateThumb (infoPtr);
    infoPtr->flags &= ~TB_SELECTIONCHANGED;

    return 0;
}


static LRESULT
TRACKBAR_Create (HWND hwnd, const CREATESTRUCTW *lpcs)
{
    TRACKBAR_INFO *infoPtr;
    DWORD dwStyle;

    infoPtr = (TRACKBAR_INFO *)Alloc (sizeof(TRACKBAR_INFO));
    if (!infoPtr) return -1;
    SetWindowLongPtrW (hwnd, 0, (DWORD_PTR)infoPtr);

    /* set default values */
    infoPtr->hwndSelf  = hwnd;
    infoPtr->lRangeMin = 0;
    infoPtr->lRangeMax = 100;
    infoPtr->lLineSize = 1;
    infoPtr->lPageSize = 20;
    infoPtr->lSelMin   = 0;
    infoPtr->lSelMax   = 0;
    infoPtr->lPos      = 0;
    infoPtr->fLocation = -1;
    infoPtr->uNumTics  = 0;    /* start and end tic are not included in count*/
    infoPtr->uTicFreq  = 1;
    infoPtr->tics      = NULL;
    infoPtr->hwndNotify= lpcs->hwndParent;

    TRACKBAR_InitializeThumb (infoPtr);

    dwStyle = GetWindowLongW (hwnd, GWL_STYLE);

    /* Create tooltip control */
    if (dwStyle & TBS_TOOLTIPS) {

    	infoPtr->hwndToolTip =
            CreateWindowExW (0, TOOLTIPS_CLASSW, NULL, WS_POPUP,
                             CW_USEDEFAULT, CW_USEDEFAULT,
                             CW_USEDEFAULT, CW_USEDEFAULT,
                             hwnd, 0, 0, 0);

    	if (infoPtr->hwndToolTip) {
            TTTOOLINFOW ti;	    
            ZeroMemory (&ti, sizeof(ti));
            ti.cbSize   = sizeof(ti);
     	    ti.uFlags   = TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE;
	    ti.hwnd     = hwnd;

            SendMessageW (infoPtr->hwndToolTip, TTM_ADDTOOLW, 0, (LPARAM)&ti);
	 }
    }
    
    OpenThemeData (hwnd, themeClass);

    return 0;
}


static LRESULT
TRACKBAR_Destroy (TRACKBAR_INFO *infoPtr)
{
    /* delete tooltip control */
    if (infoPtr->hwndToolTip)
    	DestroyWindow (infoPtr->hwndToolTip);

    Free (infoPtr);
    SetWindowLongPtrW (infoPtr->hwndSelf, 0, 0);
    CloseThemeData (GetWindowTheme (infoPtr->hwndSelf));
    return 0;
}


static LRESULT
TRACKBAR_KillFocus (TRACKBAR_INFO *infoPtr, HWND hwndGetFocus)
{
    TRACE("\n");
    infoPtr->bFocussed = FALSE;
    TRACKBAR_InvalidateAll(infoPtr);

    return 0;
}

static LRESULT
TRACKBAR_LButtonDown (TRACKBAR_INFO *infoPtr, DWORD fwKeys, INT x, INT y)
{
    POINT clickPoint;

    clickPoint.x = x;
    clickPoint.y = y;

    SetFocus(infoPtr->hwndSelf);

    if (PtInRect(&infoPtr->rcThumb, clickPoint)) {
        infoPtr->flags |= TB_DRAG_MODE;
        SetCapture (infoPtr->hwndSelf);
	TRACKBAR_UpdateToolTip (infoPtr);
	TRACKBAR_ActivateToolTip (infoPtr, TRUE);
	TRACKBAR_InvalidateThumb(infoPtr, infoPtr->lPos);
    } else {
	LONG dir = TRACKBAR_GetAutoPageDirection(infoPtr, clickPoint);
	if (dir == 0) return 0;
	infoPtr->flags |= (dir < 0) ? TB_AUTO_PAGE_LEFT : TB_AUTO_PAGE_RIGHT;
	TRACKBAR_AutoPage (infoPtr, clickPoint);
        SetCapture (infoPtr->hwndSelf);
        SetTimer(infoPtr->hwndSelf, TB_REFRESH_TIMER, TB_REFRESH_DELAY, 0);
    }

    return 0;
}


static LRESULT
TRACKBAR_LButtonUp (TRACKBAR_INFO *infoPtr, DWORD fwKeys, INT x, INT y)
{
    if (infoPtr->flags & TB_DRAG_MODE) {
        notify_with_scroll (infoPtr, TB_THUMBPOSITION | (infoPtr->lPos<<16));
        notify_with_scroll (infoPtr, TB_ENDTRACK);
        infoPtr->flags &= ~TB_DRAG_MODE;
        ReleaseCapture ();
	notify(infoPtr, NM_RELEASEDCAPTURE);
        TRACKBAR_ActivateToolTip(infoPtr, FALSE);
	TRACKBAR_InvalidateThumb(infoPtr, infoPtr->lPos);
    }
    if (infoPtr->flags & TB_AUTO_PAGE) {
	KillTimer (infoPtr->hwndSelf, TB_REFRESH_TIMER);
        infoPtr->flags &= ~TB_AUTO_PAGE;
        notify_with_scroll (infoPtr, TB_ENDTRACK);
        ReleaseCapture ();
	notify(infoPtr, NM_RELEASEDCAPTURE);
    }

    return 0;
}


static LRESULT
TRACKBAR_CaptureChanged (const TRACKBAR_INFO *infoPtr)
{
    notify_with_scroll (infoPtr, TB_ENDTRACK);
    return 0;
}


static LRESULT
TRACKBAR_Paint (TRACKBAR_INFO *infoPtr, HDC hdc)
{
    if (hdc) {
	TRACKBAR_Refresh(infoPtr, hdc);
    } else {
	PAINTSTRUCT ps;
    	hdc = BeginPaint (infoPtr->hwndSelf, &ps);
    	TRACKBAR_Refresh (infoPtr, hdc);
    	EndPaint (infoPtr->hwndSelf, &ps);
    }

    return 0;
}


static LRESULT
TRACKBAR_SetFocus (TRACKBAR_INFO *infoPtr, HWND hwndLoseFocus)
{
    TRACE("\n");
    infoPtr->bFocussed = TRUE;
    TRACKBAR_InvalidateAll(infoPtr);

    return 0;
}


static LRESULT
TRACKBAR_Size (TRACKBAR_INFO *infoPtr, DWORD fwSizeType, INT nWidth, INT nHeight)
{
    TRACKBAR_InitializeThumb (infoPtr);
    TRACKBAR_AlignBuddies (infoPtr);

    return 0;
}


static LRESULT
TRACKBAR_Timer (TRACKBAR_INFO *infoPtr, INT wTimerID, const TIMERPROC *tmrpc)
{
    if (infoPtr->flags & TB_AUTO_PAGE) {
	POINT pt;
	if (GetCursorPos(&pt))
	    if (ScreenToClient(infoPtr->hwndSelf, &pt))
		TRACKBAR_AutoPage(infoPtr, pt);
    }
    return 0;
}


/* update theme after a WM_THEMECHANGED message */
static LRESULT theme_changed (const TRACKBAR_INFO* infoPtr)
{
    HTHEME theme = GetWindowTheme (infoPtr->hwndSelf);
    CloseThemeData (theme);
    theme = OpenThemeData (infoPtr->hwndSelf, themeClass);
    return 0;
}


static LRESULT
TRACKBAR_MouseMove (TRACKBAR_INFO *infoPtr, DWORD fwKeys, INT x, INT y)
{
    DWORD dwStyle = GetWindowLongW (infoPtr->hwndSelf, GWL_STYLE);
    INT clickPlace = (dwStyle & TBS_VERT) ? y : x;
    LONG dragPos, oldPos = infoPtr->lPos;

    TRACE("(x=%d. y=%d)\n", x, y);

    if (infoPtr->flags & TB_AUTO_PAGE) {
	POINT pt;
	pt.x = x;
	pt.y = y;
	TRACKBAR_AutoPage (infoPtr, pt);
	return TRUE;
    }

    if (!(infoPtr->flags & TB_DRAG_MODE)) 
    {
        if (GetWindowTheme (infoPtr->hwndSelf))
        {
            DWORD oldFlags = infoPtr->flags;
            POINT pt;
            pt.x = x;
            pt.y = y;
            if (PtInRect (&infoPtr->rcThumb, pt))
            {
                TRACKMOUSEEVENT tme;
                tme.cbSize = sizeof( tme );
                tme.dwFlags = TME_LEAVE;
                tme.hwndTrack = infoPtr->hwndSelf;
                TrackMouseEvent( &tme );
                infoPtr->flags |= TB_THUMB_HOT;
            }
            else
            {
                TRACKMOUSEEVENT tme;
                tme.cbSize = sizeof( tme );
                tme.dwFlags = TME_CANCEL;
                tme.hwndTrack = infoPtr->hwndSelf;
                TrackMouseEvent( &tme );
                infoPtr->flags &= ~TB_THUMB_HOT; 
            }
            if (oldFlags != infoPtr->flags) InvalidateRect (infoPtr->hwndSelf, &infoPtr->rcThumb, FALSE);
        }
        return TRUE;
    }

    dragPos = TRACKBAR_ConvertPlaceToPosition (infoPtr, clickPlace,
                                               dwStyle & TBS_VERT);
    if (dragPos == oldPos) return TRUE;

    infoPtr->lPos = dragPos;

    infoPtr->flags |= TB_THUMBPOSCHANGED;
    notify_with_scroll (infoPtr, TB_THUMBTRACK | (infoPtr->lPos<<16));


    TRACKBAR_InvalidateThumbMove(infoPtr, oldPos, dragPos);
    UpdateWindow (infoPtr->hwndSelf);

    return TRUE;
}

static BOOL
TRACKBAR_KeyDown (TRACKBAR_INFO *infoPtr, INT nVirtKey, DWORD lKeyData)
{
    DWORD style = GetWindowLongW (infoPtr->hwndSelf, GWL_STYLE);
    BOOL downIsLeft = style & TBS_DOWNISLEFT;
    BOOL vert = style & TBS_VERT;
    LONG pos = infoPtr->lPos;

    TRACE("%x\n", nVirtKey);

    switch (nVirtKey) {
    case VK_UP:
	if (!vert && downIsLeft) TRACKBAR_LineDown(infoPtr);
        else TRACKBAR_LineUp(infoPtr);
        break;
    case VK_LEFT:
        if (vert && downIsLeft) TRACKBAR_LineDown(infoPtr);
        else TRACKBAR_LineUp(infoPtr);
        break;
    case VK_DOWN:
	if (!vert && downIsLeft) TRACKBAR_LineUp(infoPtr);
        else TRACKBAR_LineDown(infoPtr);
        break;
    case VK_RIGHT:
	if (vert && downIsLeft) TRACKBAR_LineUp(infoPtr);
        else TRACKBAR_LineDown(infoPtr);
        break;
    case VK_NEXT:
	if (!vert && downIsLeft) TRACKBAR_PageUp(infoPtr);
        else TRACKBAR_PageDown(infoPtr);
        break;
    case VK_PRIOR:
	if (!vert && downIsLeft) TRACKBAR_PageDown(infoPtr);
        else TRACKBAR_PageUp(infoPtr);
        break;
    case VK_HOME:
        if (infoPtr->lPos == infoPtr->lRangeMin) return FALSE;
        infoPtr->lPos = infoPtr->lRangeMin;
        notify_with_scroll (infoPtr, TB_TOP);
        break;
    case VK_END:
        if (infoPtr->lPos == infoPtr->lRangeMax) return FALSE;
        infoPtr->lPos = infoPtr->lRangeMax;
        notify_with_scroll (infoPtr, TB_BOTTOM);
        break;
    }

    if (pos != infoPtr->lPos) {
	infoPtr->flags |=TB_THUMBPOSCHANGED;
	TRACKBAR_InvalidateThumbMove (infoPtr, pos, infoPtr->lPos);
    }

    return TRUE;
}


static inline BOOL
TRACKBAR_KeyUp (const TRACKBAR_INFO *infoPtr, INT nVirtKey, DWORD lKeyData)
{
    switch (nVirtKey) {
    case VK_LEFT:
    case VK_UP:
    case VK_RIGHT:
    case VK_DOWN:
    case VK_NEXT:
    case VK_PRIOR:
    case VK_HOME:
    case VK_END:
        notify_with_scroll (infoPtr, TB_ENDTRACK);
    }
    return TRUE;
}


static LRESULT WINAPI
TRACKBAR_WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TRACKBAR_INFO *infoPtr = (TRACKBAR_INFO *)GetWindowLongPtrW (hwnd, 0);

    TRACE("hwnd=%p msg=%x wparam=%lx lparam=%lx\n", hwnd, uMsg, wParam, lParam);

    if (!infoPtr && (uMsg != WM_CREATE))
        return DefWindowProcW (hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case TBM_CLEARSEL:
        return TRACKBAR_ClearSel (infoPtr, (BOOL)wParam);

    case TBM_CLEARTICS:
        return TRACKBAR_ClearTics (infoPtr, (BOOL)wParam);

    case TBM_GETBUDDY:
        return (LRESULT)(wParam ? infoPtr->hwndBuddyLA : infoPtr->hwndBuddyRB);

    case TBM_GETCHANNELRECT:
        return TRACKBAR_GetChannelRect (infoPtr, (LPRECT)lParam);

    case TBM_GETLINESIZE:
        return infoPtr->lLineSize;

    case TBM_GETNUMTICS:
        return TRACKBAR_GetNumTics (infoPtr);

    case TBM_GETPAGESIZE:
        return infoPtr->lPageSize;

    case TBM_GETPOS:
        return infoPtr->lPos;

    case TBM_GETPTICS:
        return (LRESULT)infoPtr->tics;

    case TBM_GETRANGEMAX:
        return infoPtr->lRangeMax;

    case TBM_GETRANGEMIN:
        return infoPtr->lRangeMin;

    case TBM_GETSELEND:
        return infoPtr->lSelMax;

    case TBM_GETSELSTART:
        return infoPtr->lSelMin;

    case TBM_GETTHUMBLENGTH:
        return infoPtr->uThumbLen;

    case TBM_GETTHUMBRECT:
	return CopyRect((LPRECT)lParam, &infoPtr->rcThumb);

    case TBM_GETTIC:
        return TRACKBAR_GetTic (infoPtr, (INT)wParam);

    case TBM_GETTICPOS:
        return TRACKBAR_GetTicPos (infoPtr, (INT)wParam);

    case TBM_GETTOOLTIPS:
        return (LRESULT)infoPtr->hwndToolTip;

    case TBM_GETUNICODEFORMAT:
        return infoPtr->bUnicode;

    case TBM_SETBUDDY:
        return (LRESULT) TRACKBAR_SetBuddy(infoPtr, (BOOL)wParam, (HWND)lParam);

    case TBM_SETLINESIZE:
        return TRACKBAR_SetLineSize (infoPtr, (LONG)lParam);

    case TBM_SETPAGESIZE:
        return TRACKBAR_SetPageSize (infoPtr, (LONG)lParam);

    case TBM_SETPOS:
        return TRACKBAR_SetPos (infoPtr, (BOOL)wParam, (LONG)lParam);

    case TBM_SETRANGE:
        return TRACKBAR_SetRange (infoPtr, (BOOL)wParam, (LONG)lParam);

    case TBM_SETRANGEMAX:
        return TRACKBAR_SetRangeMax (infoPtr, (BOOL)wParam, (LONG)lParam);

    case TBM_SETRANGEMIN:
        return TRACKBAR_SetRangeMin (infoPtr, (BOOL)wParam, (LONG)lParam);

    case TBM_SETSEL:
        return TRACKBAR_SetSel (infoPtr, (BOOL)wParam, (LONG)lParam);

    case TBM_SETSELEND:
        return TRACKBAR_SetSelEnd (infoPtr, (BOOL)wParam, (LONG)lParam);

    case TBM_SETSELSTART:
        return TRACKBAR_SetSelStart (infoPtr, (BOOL)wParam, (LONG)lParam);

    case TBM_SETTHUMBLENGTH:
        return TRACKBAR_SetThumbLength (infoPtr, (UINT)wParam);

    case TBM_SETTIC:
        return TRACKBAR_SetTic (infoPtr, (LONG)lParam);

    case TBM_SETTICFREQ:
        return TRACKBAR_SetTicFreq (infoPtr, (WORD)wParam);

    case TBM_SETTIPSIDE:
        return TRACKBAR_SetTipSide (infoPtr, (INT)wParam);

    case TBM_SETTOOLTIPS:
        return TRACKBAR_SetToolTips (infoPtr, (HWND)wParam);

    case TBM_SETUNICODEFORMAT:
	return TRACKBAR_SetUnicodeFormat (infoPtr, (BOOL)wParam);


    case WM_CAPTURECHANGED:
        return TRACKBAR_CaptureChanged (infoPtr);

    case WM_CREATE:
        return TRACKBAR_Create (hwnd, (LPCREATESTRUCTW)lParam);

    case WM_DESTROY:
        return TRACKBAR_Destroy (infoPtr);

/*	case WM_ENABLE: */

    case WM_ERASEBKGND:
	return 0;

    case WM_GETDLGCODE:
        return DLGC_WANTARROWS;

    case WM_KEYDOWN:
        return TRACKBAR_KeyDown (infoPtr, (INT)wParam, (DWORD)lParam);

    case WM_KEYUP:
        return TRACKBAR_KeyUp (infoPtr, (INT)wParam, (DWORD)lParam);

    case WM_KILLFOCUS:
        return TRACKBAR_KillFocus (infoPtr, (HWND)wParam);

    case WM_LBUTTONDOWN:
        return TRACKBAR_LButtonDown (infoPtr, wParam, (SHORT)LOWORD(lParam), (SHORT)HIWORD(lParam));

    case WM_LBUTTONUP:
        return TRACKBAR_LButtonUp (infoPtr, wParam, (SHORT)LOWORD(lParam), (SHORT)HIWORD(lParam));

    case WM_MOUSELEAVE:
        infoPtr->flags &= ~TB_THUMB_HOT; 
        InvalidateRect (infoPtr->hwndSelf, &infoPtr->rcThumb, FALSE);
        return 0;
    
    case WM_MOUSEMOVE:
        return TRACKBAR_MouseMove (infoPtr, wParam, (SHORT)LOWORD(lParam), (SHORT)HIWORD(lParam));

    case WM_PRINTCLIENT:
    case WM_PAINT:
        return TRACKBAR_Paint (infoPtr, (HDC)wParam);

    case WM_SETFOCUS:
        return TRACKBAR_SetFocus (infoPtr, (HWND)wParam);

    case WM_SIZE:
        return TRACKBAR_Size (infoPtr, wParam, LOWORD(lParam), HIWORD(lParam));

    case WM_THEMECHANGED:
        return theme_changed (infoPtr);

    case WM_TIMER:
	return TRACKBAR_Timer (infoPtr, (INT)wParam, (TIMERPROC *)lParam);

    case WM_WININICHANGE:
        return TRACKBAR_InitializeThumb (infoPtr);

    default:
        if ((uMsg >= WM_USER) && (uMsg < WM_APP))
            ERR("unknown msg %04x wp=%08lx lp=%08lx\n", uMsg, wParam, lParam);
        return DefWindowProcW (hwnd, uMsg, wParam, lParam);
    }
}


void TRACKBAR_Register (void)
{
    WNDCLASSW wndClass;

    ZeroMemory (&wndClass, sizeof(WNDCLASSW));
    wndClass.style         = CS_GLOBALCLASS;
    wndClass.lpfnWndProc   = TRACKBAR_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(TRACKBAR_INFO *);
    wndClass.hCursor       = LoadCursorW (0, (LPWSTR)IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wndClass.lpszClassName = TRACKBAR_CLASSW;

    RegisterClassW (&wndClass);
}


void TRACKBAR_Unregister (void)
{
    UnregisterClassW (TRACKBAR_CLASSW, NULL);
}
