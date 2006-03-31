/*
 *  Header control
 *
 *  Copyright 1998 Eric Kohl
 *  Copyright 2000 Eric Kohl for CodeWeavers
 *  Copyright 2003 Maxime Bellenge
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
 *  TODO:
 *   - Imagelist support (partially).
 *   - Callback items (under construction).
 *   - Hottrack support (partially).
 *   - Custom draw support (including Notifications).
 *   - Drag and Drop support (including Notifications).
 *   - New messages.
 *   - Use notification format
 *   - Correct the order maintenance code to preserve valid order
 *
 */

#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wine/unicode.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "commctrl.h"
#include "comctl32.h"
#include "imagelist.h"
#include "tmschema.h"
#include "uxtheme.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(header);

typedef struct
{
    INT     cxy;
    HBITMAP hbm;
    LPWSTR    pszText;
    INT     fmt;
    LPARAM    lParam;
    INT     iImage;
    INT     iOrder;		/* see documentation of HD_ITEM */

    BOOL    bDown;		/* is item pressed? (used for drawing) */
    RECT    rect;		/* bounding rectangle of the item */
} HEADER_ITEM;


typedef struct
{
    HWND      hwndNotify;	/* Owner window to send notifications to */
    INT       nNotifyFormat;	/* format used for WM_NOTIFY messages */
    UINT      uNumItem;		/* number of items (columns) */
    INT       nHeight;		/* height of the header (pixels) */
    HFONT     hFont;		/* handle to the current font */
    HCURSOR   hcurArrow;	/* handle to the arrow cursor */
    HCURSOR   hcurDivider;	/* handle to a cursor (used over dividers) <-|-> */
    HCURSOR   hcurDivopen;	/* handle to a cursor (used over dividers) <-||-> */
    BOOL      bCaptured;	/* Is the mouse captured? */
    BOOL      bPressed;		/* Is a header item pressed (down)? */
    BOOL      bTracking;	/* Is in tracking mode? */
    BOOL      bUnicode;		/* Unicode flag */
    INT       iMoveItem;	/* index of tracked item. (Tracking mode) */
    INT       xTrackOffset;	/* distance between the right side of the tracked item and the cursor */
    INT       xOldTrack;	/* track offset (see above) after the last WM_MOUSEMOVE */
    INT       nOldWidth;	/* width of a sizing item after the last WM_MOUSEMOVE */
    INT       iHotItem;		/* index of hot item (cursor is over this item) */
    INT       iMargin;          /* width of the margin that surrounds a bitmap */

    HIMAGELIST  himl;		/* handle to an image list (may be 0) */
    HEADER_ITEM *items;		/* pointer to array of HEADER_ITEM's */
    INT         *order;         /* array of item IDs indexed by order */
    BOOL	bRectsValid;	/* validity flag for bounding rectangles */
} HEADER_INFO;


#define VERT_BORDER     3
#define DIVIDER_WIDTH  10

#define HEADER_GetInfoPtr(hwnd) ((HEADER_INFO *)GetWindowLongPtrW(hwnd,0))

static const WCHAR themeClass[] = {'H','e','a','d','e','r',0};


inline static LRESULT
HEADER_IndexToOrder (HWND hwnd, INT iItem)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    HEADER_ITEM *lpItem = &infoPtr->items[iItem];
    return lpItem->iOrder;
}


static INT
HEADER_OrderToIndex(HWND hwnd, WPARAM wParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    INT iorder = (INT)wParam;

    if ((iorder <0) || iorder >= infoPtr->uNumItem)
      return iorder;
    return infoPtr->order[iorder];
}

static void
HEADER_SetItemBounds (HWND hwnd)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    HEADER_ITEM *phdi;
    RECT rect;
    unsigned int i;
    int x;

    infoPtr->bRectsValid = TRUE;

    if (infoPtr->uNumItem == 0)
        return;

    GetClientRect (hwnd, &rect);

    x = rect.left;
    for (i = 0; i < infoPtr->uNumItem; i++) {
        phdi = &infoPtr->items[HEADER_OrderToIndex(hwnd,i)];
        phdi->rect.top = rect.top;
        phdi->rect.bottom = rect.bottom;
        phdi->rect.left = x;
        phdi->rect.right = phdi->rect.left + ((phdi->cxy>0)?phdi->cxy:0);
        x = phdi->rect.right;
    }
}

static LRESULT
HEADER_Size (HWND hwnd, WPARAM wParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);

    infoPtr->bRectsValid = FALSE;

    return 0;
}


static INT
HEADER_DrawItem (HWND hwnd, HDC hdc, INT iItem, BOOL bHotTrack)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    HEADER_ITEM *phdi = &infoPtr->items[iItem];
    RECT r;
    INT  oldBkMode, cxEdge = GetSystemMetrics(SM_CXEDGE);
    HTHEME theme = GetWindowTheme (hwnd);
    NMCUSTOMDRAW nmcd;

    TRACE("DrawItem(iItem %d bHotTrack %d unicode flag %d)\n", iItem, bHotTrack, infoPtr->bUnicode);

    if (!infoPtr->bRectsValid)
    	HEADER_SetItemBounds(hwnd);

    r = phdi->rect;
    if (r.right - r.left == 0)
	return phdi->rect.right;

    if (theme != NULL) {
        int state = (phdi->bDown) ? HIS_PRESSED :
            (bHotTrack ? HIS_HOT : HIS_NORMAL);
        DrawThemeBackground (theme, hdc, HP_HEADERITEM, state,
            &r, NULL);
        GetThemeBackgroundContentRect (theme, hdc, HP_HEADERITEM, state,
            &r, &r);
    }
    else {
        if (GetWindowLongW (hwnd, GWL_STYLE) & HDS_BUTTONS) {
            if (phdi->bDown) {
                DrawEdge (hdc, &r, BDR_RAISEDOUTER,
                            BF_RECT | BF_FLAT | BF_MIDDLE | BF_ADJUST);
            }
            else
                DrawEdge (hdc, &r, EDGE_RAISED,
                            BF_RECT | BF_SOFT | BF_MIDDLE | BF_ADJUST);
        }
        else
            DrawEdge (hdc, &r, EDGE_ETCHED, BF_BOTTOM | BF_RIGHT | BF_ADJUST);
    }
    if (phdi->bDown) {
        r.left += 2;
        r.top  += 2;
    }

    r.left  -= cxEdge;
    r.right += cxEdge;

    /* Set the colors before sending NM_CUSTOMDRAW so that it can change them */
    SetTextColor (hdc, (bHotTrack && !theme) ? COLOR_HIGHLIGHT : COLOR_BTNTEXT);
    SetBkColor(hdc, GetSysColor(COLOR_3DFACE));

    nmcd.hdr.hwndFrom = hwnd;
    nmcd.hdr.idFrom   = GetWindowLongPtrW (hwnd, GWLP_ID);
    nmcd.hdr.code     = NM_CUSTOMDRAW;
    nmcd.dwDrawStage  = CDDS_PREPAINT | CDDS_ITEM | CDDS_ITEMPOSTERASE;
    nmcd.hdc          = hdc;
    nmcd.dwItemSpec   = iItem;
    nmcd.rc           = r;
    nmcd.uItemState   = phdi->bDown ? CDIS_SELECTED : 0;
    nmcd.lItemlParam  = phdi->lParam;

    SendMessageW (infoPtr->hwndNotify, WM_NOTIFY, nmcd.hdr.idFrom, (LPARAM)&nmcd);

    if (phdi->fmt & HDF_OWNERDRAW) {
	DRAWITEMSTRUCT dis;

	dis.CtlType    = ODT_HEADER;
	dis.CtlID      = GetWindowLongPtrW (hwnd, GWLP_ID);
	dis.itemID     = iItem;
	dis.itemAction = ODA_DRAWENTIRE;
	dis.itemState  = phdi->bDown ? ODS_SELECTED : 0;
	dis.hwndItem   = hwnd;
	dis.hDC        = hdc;
	dis.rcItem     = r;
	dis.itemData   = phdi->lParam;
        oldBkMode = SetBkMode(hdc, TRANSPARENT);
	SendMessageW (infoPtr->hwndNotify, WM_DRAWITEM,
			(WPARAM)dis.CtlID, (LPARAM)&dis);
        if (oldBkMode != TRANSPARENT)
            SetBkMode(hdc, oldBkMode);
    }
    else {
	UINT rw, rh, /* width and height of r */
	     *x = NULL, *w = NULL; /* x and width of the pic (bmp or img) which is part of cnt */
	  /* cnt,txt,img,bmp */
	UINT cx, tx, ix, bx,
	     cw, tw, iw, bw;
	BITMAP bmp;

	cw = tw = iw = bw = 0;
	rw = r.right - r.left;
	rh = r.bottom - r.top;

        if (theme == NULL) {
            HBRUSH hbr = CreateSolidBrush(GetBkColor(hdc));
            RECT rcBackground = r;

            rcBackground.right -= cxEdge;
            FillRect(hdc, &rcBackground, hbr);
            DeleteObject(hbr);
        }
	if (phdi->fmt & HDF_STRING) {
	    RECT textRect;

	    DrawTextW (hdc, phdi->pszText, -1,
	               &textRect, DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_CALCRECT);
	    cw = textRect.right - textRect.left + 2 * infoPtr->iMargin;
	}

	if ((phdi->fmt & HDF_IMAGE) && (infoPtr->himl)) {
	    iw = infoPtr->himl->cx + 2 * infoPtr->iMargin;
	    x = &ix;
	    w = &iw;
	}

	if ((phdi->fmt & HDF_BITMAP) && (phdi->hbm)) {
	    GetObjectW (phdi->hbm, sizeof(BITMAP), (LPVOID)&bmp);
	    bw = bmp.bmWidth + 2 * infoPtr->iMargin;
	    if (!iw) {
		x = &bx;
		w = &bw;
	    }
	}

	if (bw || iw)
	    cw += *w; 

	/* align cx using the unclipped cw */
	if ((phdi->fmt & HDF_JUSTIFYMASK) == HDF_LEFT)
	    cx = r.left;
	else if ((phdi->fmt & HDF_JUSTIFYMASK) == HDF_CENTER)
	    cx = r.left + rw / 2 - cw / 2;
	else /* HDF_RIGHT */
	    cx = r.right - cw;
        
	/* clip cx & cw */
	if (cx < r.left)
	    cx = r.left;
	if (cx + cw > r.right)
	    cw = r.right - cx;
	
	tx = cx + infoPtr->iMargin;
	/* since cw might have changed we have to recalculate tw */
	tw = cw - infoPtr->iMargin * 2;
			
	if (iw || bw) {
	    tw -= *w;
	    if (phdi->fmt & HDF_BITMAP_ON_RIGHT) {
		/* put pic behind text */
		*x = cx + tw + infoPtr->iMargin * 3;
	    } else {
		*x = cx + infoPtr->iMargin;
		/* move text behind pic */
		tx += *w;
	    }
	}

	if (iw && bw) {
	    /* since we're done with the layout we can
	       now calculate the position of bmp which
	       has no influence on alignment and layout
	       because of img */
	    if ((phdi->fmt & HDF_JUSTIFYMASK) == HDF_RIGHT)
	        bx = cx - bw + infoPtr->iMargin;
	    else
	        bx = cx + cw + infoPtr->iMargin;
	}

	if (iw || bw) {
	    HDC hClipDC = GetDC(hwnd);
	    HRGN hClipRgn = CreateRectRgn(r.left, r.top, r.right, r.bottom);
	    SelectClipRgn(hClipDC, hClipRgn);
	    
	    if (bw) {
	        HDC hdcBitmap = CreateCompatibleDC (hClipDC);
	        SelectObject (hdcBitmap, phdi->hbm);
	        BitBlt (hClipDC, bx, r.top + ((INT)rh - bmp.bmHeight) / 2, 
		        bmp.bmWidth, bmp.bmHeight, hdcBitmap, 0, 0, SRCCOPY);
	        DeleteDC (hdcBitmap);
	    }

	    if (iw) {
	        ImageList_DrawEx (infoPtr->himl, phdi->iImage, hClipDC, 
	                          ix, r.top + ((INT)rh - infoPtr->himl->cy) / 2,
	                          infoPtr->himl->cx, infoPtr->himl->cy, CLR_DEFAULT, CLR_DEFAULT, 0);
	    }

	    DeleteObject(hClipRgn);
	    ReleaseDC(hwnd, hClipDC);
	}
        
	if (((phdi->fmt & HDF_STRING)
		|| (!(phdi->fmt & (HDF_OWNERDRAW|HDF_STRING|HDF_BITMAP|
				   HDF_BITMAP_ON_RIGHT|HDF_IMAGE)))) /* no explicit format specified? */
	    && (phdi->pszText)) {
	    oldBkMode = SetBkMode(hdc, TRANSPARENT);
	    r.left  = tx;
	    r.right = tx + tw;
	    DrawTextW (hdc, phdi->pszText, -1,
	               &r, DT_LEFT|DT_END_ELLIPSIS|DT_VCENTER|DT_SINGLELINE);
	    if (oldBkMode != TRANSPARENT)
	        SetBkMode(hdc, oldBkMode);
        }
    }/*Ownerdrawn*/

    return phdi->rect.right;
}


static void
HEADER_Refresh (HWND hwnd, HDC hdc)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    HFONT hFont, hOldFont;
    RECT rect;
    HBRUSH hbrBk;
    UINT i;
    INT x;
    HTHEME theme = GetWindowTheme (hwnd);

    /* get rect for the bar, adjusted for the border */
    GetClientRect (hwnd, &rect);

    hFont = infoPtr->hFont ? infoPtr->hFont : GetStockObject (SYSTEM_FONT);
    hOldFont = SelectObject (hdc, hFont);

    /* draw Background */
    if (theme == NULL) {
        hbrBk = GetSysColorBrush(COLOR_3DFACE);
        FillRect(hdc, &rect, hbrBk);
    }

    x = rect.left;
    for (i = 0; x <= rect.right && i < infoPtr->uNumItem; i++) {
        x = HEADER_DrawItem (hwnd, hdc, HEADER_OrderToIndex(hwnd,i), 
            infoPtr->iHotItem == i);
    }

    if ((x <= rect.right) && (infoPtr->uNumItem > 0)) {
        rect.left = x;
        if (theme != NULL) {
            DrawThemeBackground (theme, hdc, HP_HEADERITEM, HIS_NORMAL, &rect,
                NULL);
        }
        else {
            if (GetWindowLongW (hwnd, GWL_STYLE) & HDS_BUTTONS)
                DrawEdge (hdc, &rect, EDGE_RAISED, BF_TOP|BF_LEFT|BF_BOTTOM|BF_SOFT);
            else
                DrawEdge (hdc, &rect, EDGE_ETCHED, BF_BOTTOM);
        }
    }

    SelectObject (hdc, hOldFont);
}


static void
HEADER_RefreshItem (HWND hwnd, HDC hdc, INT iItem)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    HFONT hFont, hOldFont;

    hFont = infoPtr->hFont ? infoPtr->hFont : GetStockObject (SYSTEM_FONT);
    hOldFont = SelectObject (hdc, hFont);
    HEADER_DrawItem (hwnd, hdc, iItem, infoPtr->iHotItem == iItem);
    SelectObject (hdc, hOldFont);
}


static void
HEADER_InternalHitTest (HWND hwnd, LPPOINT lpPt, UINT *pFlags, INT *pItem)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    RECT rect, rcTest;
    UINT iCount;
    INT width;
    BOOL bNoWidth;

    GetClientRect (hwnd, &rect);

    *pFlags = 0;
    bNoWidth = FALSE;
    if (PtInRect (&rect, *lpPt))
    {
	if (infoPtr->uNumItem == 0) {
	    *pFlags |= HHT_NOWHERE;
	    *pItem = 1;
	    TRACE("NOWHERE\n");
	    return;
	}
	else {
	    /* somewhere inside */
	    for (iCount = 0; iCount < infoPtr->uNumItem; iCount++) {
		rect = infoPtr->items[iCount].rect;
		width = rect.right - rect.left;
		if (width == 0) {
		    bNoWidth = TRUE;
		    continue;
		}
		if (PtInRect (&rect, *lpPt)) {
		    if (width <= 2 * DIVIDER_WIDTH) {
			*pFlags |= HHT_ONHEADER;
			*pItem = iCount;
			TRACE("ON HEADER %d\n", iCount);
			return;
		    }
		    if (iCount > 0) {
			rcTest = rect;
			rcTest.right = rcTest.left + DIVIDER_WIDTH;
			if (PtInRect (&rcTest, *lpPt)) {
			    if (bNoWidth) {
				*pFlags |= HHT_ONDIVOPEN;
				*pItem = iCount - 1;
				TRACE("ON DIVOPEN %d\n", *pItem);
				return;
			    }
			    else {
				*pFlags |= HHT_ONDIVIDER;
				*pItem = iCount - 1;
				TRACE("ON DIVIDER %d\n", *pItem);
				return;
			    }
			}
		    }
		    rcTest = rect;
		    rcTest.left = rcTest.right - DIVIDER_WIDTH;
		    if (PtInRect (&rcTest, *lpPt)) {
			*pFlags |= HHT_ONDIVIDER;
			*pItem = iCount;
			TRACE("ON DIVIDER %d\n", *pItem);
			return;
		    }

		    *pFlags |= HHT_ONHEADER;
		    *pItem = iCount;
		    TRACE("ON HEADER %d\n", iCount);
		    return;
		}
	    }

	    /* check for last divider part (on nowhere) */
	    rect = infoPtr->items[infoPtr->uNumItem-1].rect;
	    rect.left = rect.right;
	    rect.right += DIVIDER_WIDTH;
	    if (PtInRect (&rect, *lpPt)) {
		if (bNoWidth) {
		    *pFlags |= HHT_ONDIVOPEN;
		    *pItem = infoPtr->uNumItem - 1;
		    TRACE("ON DIVOPEN %d\n", *pItem);
		    return;
		}
		else {
		    *pFlags |= HHT_ONDIVIDER;
		    *pItem = infoPtr->uNumItem-1;
		    TRACE("ON DIVIDER %d\n", *pItem);
		    return;
		}
	    }

	    *pFlags |= HHT_NOWHERE;
	    *pItem = 1;
	    TRACE("NOWHERE\n");
	    return;
	}
    }
    else {
	if (lpPt->x < rect.left) {
	   TRACE("TO LEFT\n");
	   *pFlags |= HHT_TOLEFT;
	}
	else if (lpPt->x > rect.right) {
	    TRACE("TO RIGHT\n");
	    *pFlags |= HHT_TORIGHT;
	}

	if (lpPt->y < rect.top) {
	    TRACE("ABOVE\n");
	    *pFlags |= HHT_ABOVE;
	}
	else if (lpPt->y > rect.bottom) {
	    TRACE("BELOW\n");
	    *pFlags |= HHT_BELOW;
	}
    }

    *pItem = 1;
    TRACE("flags=0x%X\n", *pFlags);
    return;
}


static void
HEADER_DrawTrackLine (HWND hwnd, HDC hdc, INT x)
{
    RECT rect;
    HPEN hOldPen;
    INT  oldRop;

    GetClientRect (hwnd, &rect);

    hOldPen = SelectObject (hdc, GetStockObject (BLACK_PEN));
    oldRop = SetROP2 (hdc, R2_XORPEN);
    MoveToEx (hdc, x, rect.top, NULL);
    LineTo (hdc, x, rect.bottom);
    SetROP2 (hdc, oldRop);
    SelectObject (hdc, hOldPen);
}


static BOOL
HEADER_SendSimpleNotify (HWND hwnd, UINT code)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    NMHDR nmhdr;

    nmhdr.hwndFrom = hwnd;
    nmhdr.idFrom   = GetWindowLongPtrW (hwnd, GWLP_ID);
    nmhdr.code     = code;

    return (BOOL)SendMessageW (infoPtr->hwndNotify, WM_NOTIFY,
				   (WPARAM)nmhdr.idFrom, (LPARAM)&nmhdr);
}

static BOOL
HEADER_SendHeaderNotify (HWND hwnd, UINT code, INT iItem, INT mask)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    NMHEADERA nmhdr;
    HDITEMA nmitem;

    nmhdr.hdr.hwndFrom = hwnd;
    nmhdr.hdr.idFrom   = GetWindowLongPtrW (hwnd, GWLP_ID);
    nmhdr.hdr.code = code;
    nmhdr.iItem = iItem;
    nmhdr.iButton = 0;
    nmhdr.pitem = &nmitem;
    nmitem.mask = mask;
    nmitem.cxy = infoPtr->items[iItem].cxy;
    nmitem.hbm = infoPtr->items[iItem].hbm;
    nmitem.pszText = NULL;
    nmitem.cchTextMax = 0;
/*    nmitem.pszText = infoPtr->items[iItem].pszText; */
/*    nmitem.cchTextMax = infoPtr->items[iItem].cchTextMax; */
    nmitem.fmt = infoPtr->items[iItem].fmt;
    nmitem.lParam = infoPtr->items[iItem].lParam;
    nmitem.iOrder = infoPtr->items[iItem].iOrder;
    nmitem.iImage = infoPtr->items[iItem].iImage;

    return (BOOL)SendMessageW (infoPtr->hwndNotify, WM_NOTIFY,
                               (WPARAM)nmhdr.hdr.idFrom, (LPARAM)&nmhdr);
}

/**
 * Send Disp Info notification. 
 *   depends on NMHDDISPINFOW having same structure as NMHDDISPINFOA 
 *   (so we handle the two cases only doing a specific cast for pszText).
 *
 * @param hwnd : hwnd header container handler
 * @param mask : notification mask (usually HDI_TEXT or HDI_IMAGE)
 * @param pDispInfo : NMHDDISPINFO structure (can be unicode or ansi)
 * @param isW : TRUE if dispinfo is Unicode
 */
static BOOL
HEADER_SendHeaderDispInfoNotify(HWND hwnd, INT iItem, INT mask, LPHDITEMW phdi, HEADER_ITEM* lpItem, BOOL isW)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    BOOL ret;
    BOOL convertToAnsi = FALSE;
    BOOL convertToUnicode = FALSE;
    BOOL isUnicodeNotify = FALSE;
    NMHDDISPINFOW dispInfo;

    if (mask & HDI_TEXT)
    {
        convertToAnsi = (isW && infoPtr->nNotifyFormat == NFR_ANSI);
        convertToUnicode = (!isW && infoPtr->nNotifyFormat == NFR_UNICODE);
    }
    isUnicodeNotify = (isW && !convertToAnsi);
    
    memset(&dispInfo, 0, sizeof(NMHDDISPINFOW));
    dispInfo.hdr.hwndFrom = hwnd;
    dispInfo.hdr.idFrom   = GetWindowLongPtrW (hwnd, GWLP_ID);
    if (isUnicodeNotify || convertToUnicode) 
    {
        dispInfo.hdr.code = HDN_GETDISPINFOW;
    }
    else
    {
        dispInfo.hdr.code = HDN_GETDISPINFOA;
    }
    dispInfo.iItem        = iItem;
    dispInfo.mask         = mask;
    /*
    dispInfo.pszText      = Alloc(sizeof(WCHAR) * 260);
    dispInfo.cchTextMax   = 260;
    */
    ret = (BOOL) SendMessageW(infoPtr->hwndNotify, WM_NOTIFY, 
                              (WPARAM) dispInfo.hdr.idFrom, 
                              (LPARAM) &dispInfo);

    TRACE("SendMessage returns(mask:0x%x,str:%s,lParam:%p)\n", 
          dispInfo.mask,
          (isUnicodeNotify ? debugstr_w(dispInfo.pszText) : (LPSTR) dispInfo.pszText),
          (void*) dispInfo.lParam);
	  
    if (dispInfo.mask & HDI_DI_SETITEM) 
    {
        if (dispInfo.mask & HDI_IMAGE) 
        {
            lpItem->iImage = dispInfo.iImage;
        }
        if (dispInfo.mask & HDI_TEXT) 
        {
            if (isUnicodeNotify || convertToUnicode)
                Str_SetPtrW(&lpItem->pszText, (LPCWSTR)dispInfo.pszText);
            else /*if (convertToAnsi || !isW)*/
                Str_SetPtrAtoW(&lpItem->pszText, (LPCSTR)dispInfo.pszText);
        }
        
        FIXME("NMHDDISPINFO returns with flags HDI_DI_SETITEM\n");
    }
    
    if (NULL != phdi)
    {
        if ((phdi->mask & mask) & HDI_IMAGE) 
        {
            phdi->iImage = dispInfo.iImage;
        }
        if ((phdi->mask & mask) & HDI_TEXT) 
        {
            if (isUnicodeNotify)
                Str_GetPtrW ((LPCWSTR)dispInfo.pszText, phdi->pszText, phdi->cchTextMax);
            else if (convertToUnicode)
                Str_GetPtrWtoA ((LPCWSTR)dispInfo.pszText, (LPSTR)phdi->pszText, phdi->cchTextMax);
            else /*if (!isW) */
                Str_GetPtrA ((LPCSTR)dispInfo.pszText, (LPSTR)phdi->pszText, phdi->cchTextMax);
        }
    }
    return ret;
}


static BOOL
HEADER_SendClickNotify (HWND hwnd, UINT code, INT iItem)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    NMHEADERA nmhdr;

    nmhdr.hdr.hwndFrom = hwnd;
    nmhdr.hdr.idFrom   = GetWindowLongPtrW (hwnd, GWLP_ID);
    nmhdr.hdr.code = code;
    nmhdr.iItem = iItem;
    nmhdr.iButton = 0;
    nmhdr.pitem = NULL;

    return (BOOL)SendMessageA (infoPtr->hwndNotify, WM_NOTIFY,
			       (WPARAM)nmhdr.hdr.idFrom, (LPARAM)&nmhdr);
}


static LRESULT
HEADER_CreateDragImage (HWND hwnd, WPARAM wParam)
{
    FIXME("empty stub!\n");
    return 0;
}


static LRESULT
HEADER_DeleteItem (HWND hwnd, WPARAM wParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr(hwnd);
    INT iItem = (INT)wParam;

    TRACE("[iItem=%d]\n", iItem);

    if ((iItem < 0) || (iItem >= (INT)infoPtr->uNumItem))
        return FALSE;

    if (infoPtr->uNumItem == 1) {
        TRACE("Simple delete!\n");
        if (infoPtr->items[0].pszText)
            Free (infoPtr->items[0].pszText);
        Free (infoPtr->items);
        Free(infoPtr->order);
        infoPtr->items = 0;
        infoPtr->order = 0;
        infoPtr->uNumItem = 0;
    }
    else {
        HEADER_ITEM *oldItems = infoPtr->items;
        INT i;
        INT iOrder;
        TRACE("Complex delete! [iItem=%d]\n", iItem);

        for (i = 0; i < infoPtr->uNumItem; i++)
           TRACE("%d: order=%d, iOrder=%d, ->iOrder=%d\n", i, infoPtr->order[i], infoPtr->items[i].iOrder, infoPtr->items[infoPtr->order[i]].iOrder);
        if (infoPtr->items[iItem].pszText)
            Free (infoPtr->items[iItem].pszText);
        iOrder = infoPtr->items[iItem].iOrder;

        infoPtr->uNumItem--;
        infoPtr->items = Alloc (sizeof (HEADER_ITEM) * infoPtr->uNumItem);
        /* pre delete copy */
        if (iItem > 0) {
            memcpy (&infoPtr->items[0], &oldItems[0],
                    iItem * sizeof(HEADER_ITEM));
        }

        /* post delete copy */
        if (iItem < infoPtr->uNumItem) {
            memcpy (&infoPtr->items[iItem], &oldItems[iItem+1],
                    (infoPtr->uNumItem - iItem) * sizeof(HEADER_ITEM));
        }

        /* Correct the orders */
        if (iOrder < infoPtr->uNumItem)
        {
            memmove(&infoPtr->order[iOrder], &infoPtr->order[iOrder + 1],
                   (infoPtr->uNumItem - iOrder) * sizeof(INT));
            for (i = 0; i < infoPtr->uNumItem; i++)
            {
                if (infoPtr->order[i] > iItem)
                    infoPtr->order[i]--;
                if (i >= iOrder)
                    infoPtr->items[infoPtr->order[i]].iOrder = infoPtr->order[i];
            }
        }

        for (i = 0; i < infoPtr->uNumItem; i++)
           TRACE("%d: order=%d, iOrder=%d, ->iOrder=%d\n", i, infoPtr->order[i], infoPtr->items[i].iOrder, infoPtr->items[infoPtr->order[i]].iOrder);
        Free (oldItems);
    }

    HEADER_SetItemBounds (hwnd);

    InvalidateRect(hwnd, NULL, FALSE);

    return TRUE;
}


static LRESULT
HEADER_GetImageList (HWND hwnd)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);

    return (LRESULT)infoPtr->himl;
}


static LRESULT
HEADER_GetItemT (HWND hwnd, INT nItem, LPHDITEMW phdi, BOOL bUnicode)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    HEADER_ITEM *lpItem;

    if (!phdi)
	return FALSE;

    TRACE("[nItem=%d]\n", nItem);

    if (phdi->mask == 0)
	return TRUE;
    if ((nItem < 0) || (nItem >= (INT)infoPtr->uNumItem))
        return FALSE;

    lpItem = &infoPtr->items[nItem];

    if (phdi->mask & HDI_BITMAP)
	phdi->hbm = (lpItem != NULL) ? lpItem->hbm : 0;

    if (phdi->mask & HDI_FORMAT)
	phdi->fmt = (lpItem != NULL) ? lpItem->fmt : 0;

    if (phdi->mask & HDI_WIDTH)
	phdi->cxy = (lpItem != NULL) ? lpItem->cxy : 0;

    if (phdi->mask & HDI_LPARAM)
	phdi->lParam = (lpItem != NULL) ? lpItem->lParam : 0;

    if (phdi->mask & HDI_IMAGE) 
    {
        phdi->iImage = (lpItem != NULL) ? lpItem->iImage : 0;
        if (lpItem->iImage == I_IMAGECALLBACK) 
        {
            HEADER_SendHeaderDispInfoNotify(hwnd, nItem, HDI_IMAGE, phdi, lpItem, bUnicode);
        }
    }

    if (phdi->mask & HDI_ORDER)
	phdi->iOrder = (lpItem != NULL) ? lpItem->iOrder : 0;

    if (phdi->mask & HDI_TEXT)
    {
        if (lpItem == NULL) *phdi->pszText = 0;  /* null pointer check */
        else if (lpItem->pszText == LPSTR_TEXTCALLBACKW) /* covers == TEXTCALLBACKA too */
        {
            HEADER_SendHeaderDispInfoNotify(hwnd, nItem, HDI_TEXT, phdi, lpItem, bUnicode);
        }
        else
        {
            if (bUnicode)
                Str_GetPtrW (lpItem->pszText, phdi->pszText, phdi->cchTextMax);
            else
                Str_GetPtrWtoA (lpItem->pszText, (LPSTR)phdi->pszText, phdi->cchTextMax);
        }
    }

    return TRUE;
}


inline static LRESULT
HEADER_GetItemCount (HWND hwnd)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    return infoPtr->uNumItem;
}


static LRESULT
HEADER_GetItemRect (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    INT iItem = (INT)wParam;
    LPRECT lpRect = (LPRECT)lParam;

    if ((iItem < 0) || (iItem >= (INT)infoPtr->uNumItem))
        return FALSE;

    lpRect->left   = infoPtr->items[iItem].rect.left;
    lpRect->right  = infoPtr->items[iItem].rect.right;
    lpRect->top    = infoPtr->items[iItem].rect.top;
    lpRect->bottom = infoPtr->items[iItem].rect.bottom;

    return TRUE;
}


static LRESULT
HEADER_GetOrderArray(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    LPINT order = (LPINT) lParam;
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);

    if ((unsigned int)wParam <infoPtr->uNumItem)
      return FALSE;

    memcpy(order, infoPtr->order, infoPtr->uNumItem * sizeof(INT));
    return TRUE;
}

static LRESULT
HEADER_SetOrderArray(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    int i;
    LPINT order = (LPINT) lParam;
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    HEADER_ITEM *lpItem;

    if ((unsigned int)wParam <infoPtr->uNumItem)
      return FALSE;
    memcpy(infoPtr->order, order, infoPtr->uNumItem * sizeof(INT));
    for (i=0; i<(int)wParam; i++)
      {
        lpItem = &infoPtr->items[*order++];
	lpItem->iOrder=i;
      }
    infoPtr->bRectsValid=0;
    InvalidateRect(hwnd, NULL, FALSE);
    return TRUE;
}

inline static LRESULT
HEADER_GetUnicodeFormat (HWND hwnd)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    return infoPtr->bUnicode;
}


static LRESULT
HEADER_HitTest (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    LPHDHITTESTINFO phti = (LPHDHITTESTINFO)lParam;

    HEADER_InternalHitTest (hwnd, &phti->pt, &phti->flags, &phti->iItem);

    if (phti->flags == HHT_NOWHERE)
        return -1;
    else
        return phti->iItem;
}


static LRESULT
HEADER_InsertItemT (HWND hwnd, INT nItem, LPHDITEMW phdi, BOOL bUnicode)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    HEADER_ITEM *lpItem;
    INT       iOrder;
    UINT      i;

    if ((phdi == NULL) || (nItem < 0))
	return -1;

    if (nItem > infoPtr->uNumItem)
        nItem = infoPtr->uNumItem;

    iOrder = (phdi->mask & HDI_ORDER) ? phdi->iOrder : nItem;
    if (iOrder < 0)
        iOrder = 0;
    else if (infoPtr->uNumItem < iOrder)
        iOrder = infoPtr->uNumItem;

    if (infoPtr->uNumItem == 0) {
        infoPtr->items = Alloc (sizeof (HEADER_ITEM));
        infoPtr->order = Alloc(sizeof(INT));
        infoPtr->uNumItem++;
    }
    else {
        HEADER_ITEM *oldItems = infoPtr->items;
        INT *oldOrder = infoPtr->order;

        infoPtr->uNumItem++;
        infoPtr->items = Alloc (sizeof (HEADER_ITEM) * infoPtr->uNumItem);
        if (nItem == 0) {
            memcpy (&infoPtr->items[1], &oldItems[0],
                    (infoPtr->uNumItem-1) * sizeof(HEADER_ITEM));
        }
        else
        {
              /* pre insert copy */
            if (nItem > 0) {
                 memcpy (&infoPtr->items[0], &oldItems[0],
                         nItem * sizeof(HEADER_ITEM));
            }

            /* post insert copy */
            if (nItem < infoPtr->uNumItem - 1) {
                memcpy (&infoPtr->items[nItem+1], &oldItems[nItem],
                        (infoPtr->uNumItem - nItem - 1) * sizeof(HEADER_ITEM));
            }
        }

        infoPtr->order = Alloc(sizeof(INT) * infoPtr->uNumItem);
        memcpy(infoPtr->order, oldOrder, iOrder * sizeof(INT));
        infoPtr->order[iOrder] = nItem;
        memcpy(&infoPtr->order[iOrder + 1], &oldOrder[iOrder],
               (infoPtr->uNumItem - iOrder - 1) * sizeof(INT));

        Free (oldItems);
        Free(oldOrder);
    }

    for (i = 0; i < infoPtr->uNumItem; i++)
    {
        if (i != iOrder && infoPtr->order[i] >= nItem)
            infoPtr->order[i]++;
        infoPtr->items[infoPtr->order[i]].iOrder = infoPtr->order[i];
    }

    lpItem = &infoPtr->items[nItem];
    lpItem->bDown = FALSE;

    if (phdi->mask & HDI_WIDTH)
	lpItem->cxy = phdi->cxy;

    if (phdi->mask & HDI_FORMAT)
	lpItem->fmt = phdi->fmt;

    if (lpItem->fmt == 0)
	lpItem->fmt = HDF_LEFT;

    if (phdi->mask & HDI_BITMAP)
        lpItem->hbm = phdi->hbm;

    if (phdi->mask & HDI_LPARAM)
        lpItem->lParam = phdi->lParam;

    if (phdi->mask & HDI_IMAGE) 
    {
        if (phdi->iImage != I_IMAGECALLBACK) 
        {
            lpItem->iImage = phdi->iImage;
        } 
        else 
        {
            lpItem->iImage = phdi->iImage;
            HEADER_SendHeaderDispInfoNotify(hwnd, nItem, HDI_IMAGE, NULL, lpItem, bUnicode);
        }
    }

    if (phdi->mask & HDI_TEXT)
    {
	if (!phdi->pszText) phdi->pszText = '\0'; /* null pointer check */
        if (phdi->pszText != LPSTR_TEXTCALLBACKW) /* covers != TEXTCALLBACKA too */
        {
            if (bUnicode)
                Str_SetPtrW(&lpItem->pszText, phdi->pszText);
            else
                Str_SetPtrAtoW(&lpItem->pszText, (LPSTR)phdi->pszText);
        }
        else 
        {
            lpItem->pszText = phdi->pszText;
            HEADER_SendHeaderDispInfoNotify(hwnd, nItem, HDI_TEXT, NULL, lpItem, bUnicode);
        }
        lpItem->fmt |= HDF_STRING;
    }

    lpItem->iOrder = iOrder;

    HEADER_SetItemBounds (hwnd);

    InvalidateRect(hwnd, NULL, FALSE);

    return nItem;
}


static LRESULT
HEADER_Layout (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    LPHDLAYOUT lpLayout = (LPHDLAYOUT)lParam;

    lpLayout->pwpos->hwnd = hwnd;
    lpLayout->pwpos->hwndInsertAfter = 0;
    lpLayout->pwpos->x = lpLayout->prc->left;
    lpLayout->pwpos->y = lpLayout->prc->top;
    lpLayout->pwpos->cx = lpLayout->prc->right - lpLayout->prc->left;
    if (GetWindowLongW (hwnd, GWL_STYLE) & HDS_HIDDEN)
        lpLayout->pwpos->cy = 0;
    else {
        lpLayout->pwpos->cy = infoPtr->nHeight;
        lpLayout->prc->top += infoPtr->nHeight;
    }
    lpLayout->pwpos->flags = SWP_NOZORDER;

    TRACE("Layout x=%d y=%d cx=%d cy=%d\n",
           lpLayout->pwpos->x, lpLayout->pwpos->y,
           lpLayout->pwpos->cx, lpLayout->pwpos->cy);

    infoPtr->bRectsValid = FALSE;

    return TRUE;
}


static LRESULT
HEADER_SetImageList (HWND hwnd, HIMAGELIST himl)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    HIMAGELIST himlOld;

    TRACE("(himl %p)\n", himl);
    himlOld = infoPtr->himl;
    infoPtr->himl = himl;

    /* FIXME: Refresh needed??? */

    return (LRESULT)himlOld;
}


static LRESULT
HEADER_GetBitmapMargin(HWND hwnd)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr(hwnd);
    
    return infoPtr->iMargin;
}

static LRESULT
HEADER_SetBitmapMargin(HWND hwnd, WPARAM wParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    INT oldMargin = infoPtr->iMargin;

    infoPtr->iMargin = (INT)wParam;

    return oldMargin;
}

static LRESULT
HEADER_SetItemT (HWND hwnd, INT nItem, LPHDITEMW phdi, BOOL bUnicode)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    HEADER_ITEM *lpItem;

    if (phdi == NULL)
	return FALSE;
    if ((nItem < 0) || (nItem >= (INT)infoPtr->uNumItem))
        return FALSE;

    TRACE("[nItem=%d]\n", nItem);

    if (HEADER_SendHeaderNotify (hwnd, bUnicode ? HDN_ITEMCHANGINGW : HDN_ITEMCHANGINGA,
                                 nItem, phdi->mask))
	return FALSE;

    lpItem = &infoPtr->items[nItem];
    if (phdi->mask & HDI_BITMAP)
	lpItem->hbm = phdi->hbm;

    if (phdi->mask & HDI_FORMAT)
	lpItem->fmt = phdi->fmt;

    if (phdi->mask & HDI_LPARAM)
	lpItem->lParam = phdi->lParam;

    if (phdi->mask & HDI_WIDTH)
	lpItem->cxy = phdi->cxy;

    if (phdi->mask & HDI_IMAGE) 
    {
        if (phdi->iImage != I_IMAGECALLBACK) 
        {
            lpItem->iImage = phdi->iImage;
        } 
        else 
        {
            lpItem->iImage = phdi->iImage;
            HEADER_SendHeaderDispInfoNotify(hwnd, nItem, HDI_IMAGE, NULL, lpItem, bUnicode);
        }
    }

    if (phdi->mask & HDI_TEXT)
    {
        if (phdi->pszText != LPSTR_TEXTCALLBACKW) /* covers != TEXTCALLBACKA too */
        {
            if (lpItem->pszText)
            {
                Free(lpItem->pszText);
                lpItem->pszText = NULL;
            }
            if (phdi->pszText)
            {
                if (bUnicode)
                    Str_SetPtrW(&lpItem->pszText, phdi->pszText);
                else
                    Str_SetPtrAtoW(&lpItem->pszText, (LPSTR)phdi->pszText);
            }
	}
	else 
	{
            lpItem->pszText = phdi->pszText;
            HEADER_SendHeaderDispInfoNotify(hwnd, nItem, HDI_TEXT, NULL, lpItem, bUnicode);
        }  
    }

    if (phdi->mask & HDI_ORDER)
      {
        INT i, nMin, nMax;
        
        if (lpItem->iOrder < phdi->iOrder)
        {
            memmove(&infoPtr->order[lpItem->iOrder],
                   &infoPtr->order[lpItem->iOrder + 1],
                   (phdi->iOrder - lpItem->iOrder) * sizeof(INT));
        }
        if (phdi->iOrder < lpItem->iOrder)
        {
            memmove(&infoPtr->order[phdi->iOrder + 1],
                    &infoPtr->order[phdi->iOrder],
                    (lpItem->iOrder - phdi->iOrder) * sizeof(INT));
        }
        infoPtr->order[phdi->iOrder] = nItem;
        nMin = min(lpItem->iOrder, phdi->iOrder);
        nMax = max(lpItem->iOrder, phdi->iOrder);
        for (i = nMin; i <= nMax; i++)
        {
            infoPtr->items[infoPtr->order[i]].iOrder = infoPtr->order[i];
        }
      }

    HEADER_SendHeaderNotify (hwnd, bUnicode ? HDN_ITEMCHANGEDW : HDN_ITEMCHANGEDA,
                             nItem, phdi->mask);

    HEADER_SetItemBounds (hwnd);

    InvalidateRect(hwnd, NULL, FALSE);

    return TRUE;
}

inline static LRESULT
HEADER_SetUnicodeFormat (HWND hwnd, WPARAM wParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    BOOL bTemp = infoPtr->bUnicode;

    infoPtr->bUnicode = (BOOL)wParam;

    return bTemp;
}


static LRESULT
HEADER_Create (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HEADER_INFO *infoPtr;
    TEXTMETRICW tm;
    HFONT hOldFont;
    HDC   hdc;

    infoPtr = (HEADER_INFO *)Alloc (sizeof(HEADER_INFO));
    SetWindowLongPtrW (hwnd, 0, (DWORD_PTR)infoPtr);

    infoPtr->hwndNotify = ((LPCREATESTRUCTA)lParam)->hwndParent;
    infoPtr->uNumItem = 0;
    infoPtr->hFont = 0;
    infoPtr->items = 0;
    infoPtr->order = 0;
    infoPtr->bRectsValid = FALSE;
    infoPtr->hcurArrow = LoadCursorW (0, (LPWSTR)IDC_ARROW);
    infoPtr->hcurDivider = LoadCursorW (COMCTL32_hModule, MAKEINTRESOURCEW(IDC_DIVIDER));
    infoPtr->hcurDivopen = LoadCursorW (COMCTL32_hModule, MAKEINTRESOURCEW(IDC_DIVIDEROPEN));
    infoPtr->bPressed  = FALSE;
    infoPtr->bTracking = FALSE;
    infoPtr->iMoveItem = 0;
    infoPtr->himl = 0;
    infoPtr->iHotItem = -1;
    infoPtr->bUnicode = IsWindowUnicode (hwnd);
    infoPtr->iMargin = 3*GetSystemMetrics(SM_CXEDGE);
    infoPtr->nNotifyFormat =
	SendMessageW (infoPtr->hwndNotify, WM_NOTIFYFORMAT, (WPARAM)hwnd, NF_QUERY);

    hdc = GetDC (0);
    hOldFont = SelectObject (hdc, GetStockObject (SYSTEM_FONT));
    GetTextMetricsW (hdc, &tm);
    infoPtr->nHeight = tm.tmHeight + VERT_BORDER;
    SelectObject (hdc, hOldFont);
    ReleaseDC (0, hdc);

    OpenThemeData(hwnd, themeClass);

    return 0;
}


static LRESULT
HEADER_Destroy (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    HEADER_ITEM *lpItem;
    INT nItem;
    HTHEME theme;

    if (infoPtr->items) {
        lpItem = infoPtr->items;
        for (nItem = 0; nItem < infoPtr->uNumItem; nItem++, lpItem++) {
	    if ((lpItem->pszText) && (lpItem->pszText != LPSTR_TEXTCALLBACKW))
		Free (lpItem->pszText);
        }
        Free (infoPtr->items);
    }

    if (infoPtr->order)
        Free(infoPtr->order);

    if (infoPtr->himl)
	ImageList_Destroy (infoPtr->himl);

    SetWindowLongPtrW (hwnd, 0, 0);
    Free (infoPtr);

    theme = GetWindowTheme(hwnd);
    CloseThemeData(theme);
    return 0;
}


static inline LRESULT
HEADER_GetFont (HWND hwnd)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);

    return (LRESULT)infoPtr->hFont;
}


static LRESULT
HEADER_LButtonDblClk (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    POINT pt;
    UINT  flags;
    INT   nItem;

    pt.x = (INT)LOWORD(lParam);
    pt.y = (INT)HIWORD(lParam);
    HEADER_InternalHitTest (hwnd, &pt, &flags, &nItem);

    if ((GetWindowLongW (hwnd, GWL_STYLE) & HDS_BUTTONS) && (flags == HHT_ONHEADER))
	HEADER_SendHeaderNotify (hwnd, HDN_ITEMDBLCLICKA, nItem,0);
    else if ((flags == HHT_ONDIVIDER) || (flags == HHT_ONDIVOPEN))
	HEADER_SendHeaderNotify (hwnd, HDN_DIVIDERDBLCLICKA, nItem,0);

    return 0;
}


static LRESULT
HEADER_LButtonDown (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    DWORD dwStyle = GetWindowLongW (hwnd, GWL_STYLE);
    POINT pt;
    UINT  flags;
    INT   nItem;
    HDC   hdc;

    pt.x = (INT)LOWORD(lParam);
    pt.y = (INT)HIWORD(lParam);
    HEADER_InternalHitTest (hwnd, &pt, &flags, &nItem);

    if ((dwStyle & HDS_BUTTONS) && (flags == HHT_ONHEADER)) {
	SetCapture (hwnd);
	infoPtr->bCaptured = TRUE;
	infoPtr->bPressed  = TRUE;
	infoPtr->iMoveItem = nItem;

	infoPtr->items[nItem].bDown = TRUE;

	/* Send WM_CUSTOMDRAW */
	hdc = GetDC (hwnd);
	HEADER_RefreshItem (hwnd, hdc, nItem);
	ReleaseDC (hwnd, hdc);

	TRACE("Pressed item %d!\n", nItem);
    }
    else if ((flags == HHT_ONDIVIDER) || (flags == HHT_ONDIVOPEN)) {
	if (!(HEADER_SendHeaderNotify (hwnd, HDN_BEGINTRACKA, nItem,0))) {
	    SetCapture (hwnd);
	    infoPtr->bCaptured = TRUE;
	    infoPtr->bTracking = TRUE;
	    infoPtr->iMoveItem = nItem;
	    infoPtr->nOldWidth = infoPtr->items[nItem].cxy;
	    infoPtr->xTrackOffset = infoPtr->items[nItem].rect.right - pt.x;

	    if (!(dwStyle & HDS_FULLDRAG)) {
		infoPtr->xOldTrack = infoPtr->items[nItem].rect.right;
		hdc = GetDC (hwnd);
		HEADER_DrawTrackLine (hwnd, hdc, infoPtr->xOldTrack);
		ReleaseDC (hwnd, hdc);
	    }

	    TRACE("Begin tracking item %d!\n", nItem);
	}
    }

    return 0;
}


static LRESULT
HEADER_LButtonUp (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    /*
     *DWORD dwStyle = GetWindowLongW (hwnd, GWL_STYLE);
     */
    POINT pt;
    UINT  flags;
    INT   nItem, nWidth;
    HDC   hdc;

    pt.x = (INT)(SHORT)LOWORD(lParam);
    pt.y = (INT)(SHORT)HIWORD(lParam);
    HEADER_InternalHitTest (hwnd, &pt, &flags, &nItem);

    if (infoPtr->bPressed) {
	if ((nItem == infoPtr->iMoveItem) && (flags == HHT_ONHEADER)) {
	    infoPtr->items[infoPtr->iMoveItem].bDown = FALSE;
	    hdc = GetDC (hwnd);
	    HEADER_RefreshItem (hwnd, hdc, infoPtr->iMoveItem);
	    ReleaseDC (hwnd, hdc);

	    HEADER_SendClickNotify (hwnd, HDN_ITEMCLICKA, infoPtr->iMoveItem);
	}
	else if (flags == HHT_ONHEADER)
	  {
	    HEADER_ITEM *lpItem;
	    INT newindex = HEADER_IndexToOrder(hwnd,nItem);
	    INT oldindex = HEADER_IndexToOrder(hwnd,infoPtr->iMoveItem);

	    TRACE("Exchanging [index:order] [%d:%d] [%d:%d]\n",
		  infoPtr->iMoveItem,oldindex,nItem,newindex);
            lpItem= &infoPtr->items[nItem];
	    lpItem->iOrder=oldindex;

            lpItem= &infoPtr->items[infoPtr->iMoveItem];
	    lpItem->iOrder = newindex;

            infoPtr->order[oldindex] = nItem;
            infoPtr->order[newindex] = infoPtr->iMoveItem;

	    infoPtr->bRectsValid = FALSE;
	    InvalidateRect(hwnd, NULL, FALSE);
	    /* FIXME: Should some WM_NOTIFY be sent */
	  }

	TRACE("Released item %d!\n", infoPtr->iMoveItem);
	infoPtr->bPressed = FALSE;
    }
    else if (infoPtr->bTracking) {
	TRACE("End tracking item %d!\n", infoPtr->iMoveItem);
	infoPtr->bTracking = FALSE;

	HEADER_SendHeaderNotify (hwnd, HDN_ENDTRACKA, infoPtr->iMoveItem,HDI_WIDTH);

         /*
          * we want to do this even for HDS_FULLDRAG because this is where
          * we send the HDN_ITEMCHANGING and HDN_ITEMCHANGED notifications
          *
          * if (!(dwStyle & HDS_FULLDRAG)) {
          */

	    hdc = GetDC (hwnd);
	    HEADER_DrawTrackLine (hwnd, hdc, infoPtr->xOldTrack);
            ReleaseDC (hwnd, hdc);
			if (HEADER_SendHeaderNotify(hwnd, HDN_ITEMCHANGINGA, infoPtr->iMoveItem, HDI_WIDTH))
	    {
		infoPtr->items[infoPtr->iMoveItem].cxy = infoPtr->nOldWidth;
	    }
	    else {
		nWidth = pt.x - infoPtr->items[infoPtr->iMoveItem].rect.left + infoPtr->xTrackOffset;
		if (nWidth < 0)
		    nWidth = 0;
		infoPtr->items[infoPtr->iMoveItem].cxy = nWidth;
            }

	    HEADER_SetItemBounds (hwnd);
	    InvalidateRect(hwnd, NULL, TRUE);
	    HEADER_SendHeaderNotify(hwnd, HDN_ITEMCHANGEDA, infoPtr->iMoveItem, HDI_WIDTH);
       /*
	* }
        */
    }

    if (infoPtr->bCaptured) {
	infoPtr->bCaptured = FALSE;
	ReleaseCapture ();
	HEADER_SendSimpleNotify (hwnd, NM_RELEASEDCAPTURE);
    }

    return 0;
}


static LRESULT
HEADER_NotifyFormat (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);

    switch (lParam)
    {
	case NF_QUERY:
	    return infoPtr->nNotifyFormat;

	case NF_REQUERY:
	    infoPtr->nNotifyFormat =
		SendMessageW ((HWND)wParam, WM_NOTIFYFORMAT,
			      (WPARAM)hwnd, (LPARAM)NF_QUERY);
	    return infoPtr->nNotifyFormat;
    }

    return 0;
}


static LRESULT
HEADER_MouseLeave (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    /* Reset hot-tracked item when mouse leaves control. */
    INT oldHotItem = infoPtr->iHotItem;
    HDC hdc = GetDC (hwnd);

    infoPtr->iHotItem = -1;
    if (oldHotItem != -1) HEADER_RefreshItem (hwnd, hdc, oldHotItem);
    ReleaseDC (hwnd, hdc);

    return 0;
}


static LRESULT
HEADER_MouseMove (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    DWORD dwStyle = GetWindowLongW (hwnd, GWL_STYLE);
    POINT pt;
    UINT  flags;
    INT   nItem, nWidth;
    HDC   hdc;
    /* With theming, hottracking is always enabled */
    BOOL  hotTrackEnabled =
        ((dwStyle & HDS_BUTTONS) && (dwStyle & HDS_HOTTRACK))
        || (GetWindowTheme (hwnd) != NULL);
    INT oldHotItem = infoPtr->iHotItem;

    pt.x = (INT)(SHORT)LOWORD(lParam);
    pt.y = (INT)(SHORT)HIWORD(lParam);
    HEADER_InternalHitTest (hwnd, &pt, &flags, &nItem);

    if (hotTrackEnabled) {
	if (flags & (HHT_ONHEADER | HHT_ONDIVIDER | HHT_ONDIVOPEN))
	    infoPtr->iHotItem = nItem;
	else
	    infoPtr->iHotItem = -1;
    }

    if (infoPtr->bCaptured) {
	if (infoPtr->bPressed) {
            BOOL oldState = infoPtr->items[infoPtr->iMoveItem].bDown;
	    if ((nItem == infoPtr->iMoveItem) && (flags == HHT_ONHEADER))
		infoPtr->items[infoPtr->iMoveItem].bDown = TRUE;
	    else
		infoPtr->items[infoPtr->iMoveItem].bDown = FALSE;
            if (oldState != infoPtr->items[infoPtr->iMoveItem].bDown) {
                hdc = GetDC (hwnd);
	        HEADER_RefreshItem (hwnd, hdc, infoPtr->iMoveItem);
	        ReleaseDC (hwnd, hdc);
            }

	    TRACE("Moving pressed item %d!\n", infoPtr->iMoveItem);
	}
	else if (infoPtr->bTracking) {
	    if (dwStyle & HDS_FULLDRAG) {
		if (HEADER_SendHeaderNotify (hwnd, HDN_TRACKA, infoPtr->iMoveItem, HDI_WIDTH))
		{
		nWidth = pt.x - infoPtr->items[infoPtr->iMoveItem].rect.left + infoPtr->xTrackOffset;
		if (nWidth < 0)
		  nWidth = 0;
		infoPtr->items[infoPtr->iMoveItem].cxy = nWidth;
			HEADER_SendHeaderNotify(hwnd, HDN_ITEMCHANGEDA, infoPtr->iMoveItem, HDI_WIDTH);
		}
		HEADER_SetItemBounds (hwnd);
	    }
	    else {
		hdc = GetDC (hwnd);
		HEADER_DrawTrackLine (hwnd, hdc, infoPtr->xOldTrack);
		infoPtr->xOldTrack = pt.x + infoPtr->xTrackOffset;
		if (infoPtr->xOldTrack < infoPtr->items[infoPtr->iMoveItem].rect.left)
		    infoPtr->xOldTrack = infoPtr->items[infoPtr->iMoveItem].rect.left;
		infoPtr->items[infoPtr->iMoveItem].cxy =
		    infoPtr->xOldTrack - infoPtr->items[infoPtr->iMoveItem].rect.left;
		HEADER_DrawTrackLine (hwnd, hdc, infoPtr->xOldTrack);
		ReleaseDC (hwnd, hdc);
	    HEADER_SendHeaderNotify (hwnd, HDN_TRACKA, infoPtr->iMoveItem, HDI_WIDTH);
	    }

	    TRACE("Tracking item %d!\n", infoPtr->iMoveItem);
	}
    }

    if (hotTrackEnabled) {
        TRACKMOUSEEVENT tme;
        if (oldHotItem != infoPtr->iHotItem) {
	    hdc = GetDC (hwnd);
	    if (oldHotItem != -1) HEADER_RefreshItem (hwnd, hdc, oldHotItem);
	    if (infoPtr->iHotItem != -1) HEADER_RefreshItem (hwnd, hdc, infoPtr->iHotItem);
	    ReleaseDC (hwnd, hdc);
        }
        tme.cbSize = sizeof( tme );
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = hwnd;
        TrackMouseEvent( &tme );
    }

    return 0;
}


static LRESULT
HEADER_Paint (HWND hwnd, WPARAM wParam)
{
    HDC hdc;
    PAINTSTRUCT ps;

    hdc = wParam==0 ? BeginPaint (hwnd, &ps) : (HDC)wParam;
    HEADER_Refresh (hwnd, hdc);
    if(!wParam)
	EndPaint (hwnd, &ps);
    return 0;
}


static LRESULT
HEADER_RButtonUp (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    BOOL bRet;
    POINT pt;

    pt.x = LOWORD(lParam);
    pt.y = HIWORD(lParam);

    /* Send a Notify message */
    bRet = HEADER_SendSimpleNotify (hwnd, NM_RCLICK);

    /* Change to screen coordinate for WM_CONTEXTMENU */
    ClientToScreen(hwnd, &pt);

    /* Send a WM_CONTEXTMENU message in response to the RBUTTONUP */
    SendMessageW( hwnd, WM_CONTEXTMENU, (WPARAM) hwnd, MAKELPARAM(pt.x, pt.y));

    return bRet;
}


static LRESULT
HEADER_SetCursor (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    POINT pt;
    UINT  flags;
    INT   nItem;

    TRACE("code=0x%X  id=0x%X\n", LOWORD(lParam), HIWORD(lParam));

    GetCursorPos (&pt);
    ScreenToClient (hwnd, &pt);

    HEADER_InternalHitTest (hwnd, &pt, &flags, &nItem);

    if (flags == HHT_ONDIVIDER)
        SetCursor (infoPtr->hcurDivider);
    else if (flags == HHT_ONDIVOPEN)
        SetCursor (infoPtr->hcurDivopen);
    else
        SetCursor (infoPtr->hcurArrow);

    return 0;
}


static LRESULT
HEADER_SetFont (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    HEADER_INFO *infoPtr = HEADER_GetInfoPtr (hwnd);
    TEXTMETRICW tm;
    HFONT hFont, hOldFont;
    HDC hdc;

    infoPtr->hFont = (HFONT)wParam;

    hFont = infoPtr->hFont ? infoPtr->hFont : GetStockObject (SYSTEM_FONT);

    hdc = GetDC (0);
    hOldFont = SelectObject (hdc, hFont);
    GetTextMetricsW (hdc, &tm);
    infoPtr->nHeight = tm.tmHeight + VERT_BORDER;
    SelectObject (hdc, hOldFont);
    ReleaseDC (0, hdc);

    infoPtr->bRectsValid = FALSE;

    if (lParam) {
        InvalidateRect(hwnd, NULL, FALSE);
    }

    return 0;
}

/* Update the theme handle after a theme change */
static LRESULT HEADER_ThemeChanged(HWND hwnd)
{
    HTHEME theme = GetWindowTheme(hwnd);
    CloseThemeData(theme);
    OpenThemeData(hwnd, themeClass);
    InvalidateRect(hwnd, NULL, FALSE);
    return 0;
}


static LRESULT WINAPI
HEADER_WindowProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    TRACE("hwnd=%p msg=%x wparam=%x lParam=%lx\n", hwnd, msg, wParam, lParam);
    if (!HEADER_GetInfoPtr (hwnd) && (msg != WM_CREATE))
	return DefWindowProcW (hwnd, msg, wParam, lParam);
    switch (msg) {
/*	case HDM_CLEARFILTER: */

	case HDM_CREATEDRAGIMAGE:
	    return HEADER_CreateDragImage (hwnd, wParam);

	case HDM_DELETEITEM:
	    return HEADER_DeleteItem (hwnd, wParam);

/*	case HDM_EDITFILTER: */

	case HDM_GETBITMAPMARGIN:
	    return HEADER_GetBitmapMargin(hwnd);

	case HDM_GETIMAGELIST:
	    return HEADER_GetImageList (hwnd);

	case HDM_GETITEMA:
	case HDM_GETITEMW:
	    return HEADER_GetItemT (hwnd, (INT)wParam, (LPHDITEMW)lParam, msg == HDM_GETITEMW);

	case HDM_GETITEMCOUNT:
	    return HEADER_GetItemCount (hwnd);

	case HDM_GETITEMRECT:
	    return HEADER_GetItemRect (hwnd, wParam, lParam);

	case HDM_GETORDERARRAY:
	    return HEADER_GetOrderArray(hwnd, wParam, lParam);

	case HDM_GETUNICODEFORMAT:
	    return HEADER_GetUnicodeFormat (hwnd);

	case HDM_HITTEST:
	    return HEADER_HitTest (hwnd, wParam, lParam);

	case HDM_INSERTITEMA:
	case HDM_INSERTITEMW:
	    return HEADER_InsertItemT (hwnd, (INT)wParam, (LPHDITEMW)lParam, msg == HDM_INSERTITEMW);

	case HDM_LAYOUT:
	    return HEADER_Layout (hwnd, wParam, lParam);

	case HDM_ORDERTOINDEX:
	    return HEADER_OrderToIndex(hwnd, wParam);

	case HDM_SETBITMAPMARGIN:
	    return HEADER_SetBitmapMargin(hwnd, wParam);

/*	case HDM_SETFILTERCHANGETIMEOUT: */

/*	case HDM_SETHOTDIVIDER: */

	case HDM_SETIMAGELIST:
	    return HEADER_SetImageList (hwnd, (HIMAGELIST)lParam);

	case HDM_SETITEMA:
	case HDM_SETITEMW:
	    return HEADER_SetItemT (hwnd, (INT)wParam, (LPHDITEMW)lParam, msg == HDM_SETITEMW);

	case HDM_SETORDERARRAY:
	    return HEADER_SetOrderArray(hwnd, wParam, lParam);

	case HDM_SETUNICODEFORMAT:
	    return HEADER_SetUnicodeFormat (hwnd, wParam);

        case WM_CREATE:
            return HEADER_Create (hwnd, wParam, lParam);

        case WM_DESTROY:
            return HEADER_Destroy (hwnd, wParam, lParam);

        case WM_ERASEBKGND:
            return 1;

        case WM_GETDLGCODE:
            return DLGC_WANTTAB | DLGC_WANTARROWS;

        case WM_GETFONT:
            return HEADER_GetFont (hwnd);

        case WM_LBUTTONDBLCLK:
            return HEADER_LButtonDblClk (hwnd, wParam, lParam);

        case WM_LBUTTONDOWN:
            return HEADER_LButtonDown (hwnd, wParam, lParam);

        case WM_LBUTTONUP:
            return HEADER_LButtonUp (hwnd, wParam, lParam);

        case WM_MOUSELEAVE:
            return HEADER_MouseLeave (hwnd, wParam, lParam);

        case WM_MOUSEMOVE:
            return HEADER_MouseMove (hwnd, wParam, lParam);

	case WM_NOTIFYFORMAT:
            return HEADER_NotifyFormat (hwnd, wParam, lParam);

	case WM_SIZE:
	    return HEADER_Size (hwnd, wParam);

        case WM_THEMECHANGED:
            return HEADER_ThemeChanged (hwnd);

        case WM_PRINTCLIENT:
        case WM_PAINT:
            return HEADER_Paint (hwnd, wParam);

        case WM_RBUTTONUP:
            return HEADER_RButtonUp (hwnd, wParam, lParam);

        case WM_SETCURSOR:
            return HEADER_SetCursor (hwnd, wParam, lParam);

        case WM_SETFONT:
            return HEADER_SetFont (hwnd, wParam, lParam);

        default:
            if ((msg >= WM_USER) && (msg < WM_APP))
		ERR("unknown msg %04x wp=%04x lp=%08lx\n",
		     msg, wParam, lParam );
	    return DefWindowProcA (hwnd, msg, wParam, lParam);
    }
}


VOID
HEADER_Register (void)
{
    WNDCLASSW wndClass;

    ZeroMemory (&wndClass, sizeof(WNDCLASSW));
    wndClass.style         = CS_GLOBALCLASS | CS_DBLCLKS;
    wndClass.lpfnWndProc   = HEADER_WindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof(HEADER_INFO *);
    wndClass.hCursor       = LoadCursorW (0, (LPWSTR)IDC_ARROW);
    wndClass.lpszClassName = WC_HEADERW;

    RegisterClassW (&wndClass);
}


VOID
HEADER_Unregister (void)
{
    UnregisterClassW (WC_HEADERW, NULL);
}
