/*
 *  Header control
 *
 *  Copyright 1998 Eric Kohl
 *  Copyright 2000 Eric Kohl for CodeWeavers
 *  Copyright 2003 Maxime Bellenge
 *  Copyright 2006 Mikolaj Zalewski
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
 *  TODO:
 *   - Imagelist support (completed?)
 *   - Hottrack support (completed?)
 *   - Filters support (HDS_FILTER, HDI_FILTER, HDM_*FILTER*, HDN_*FILTER*)
 *   - New Windows Vista features
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
#include "vssym32.h"
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
    DWORD   callbackMask;       /* HDI_* flags for items that are callback */
} HEADER_ITEM;


typedef struct
{
    HWND      hwndSelf;		/* Control window */
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
    BOOL      bDragging;        /* Are we dragging an item? */
    BOOL      bTracking;	/* Is in tracking mode? */
    POINT     ptLButtonDown;    /* The point where the left button was pressed */
    DWORD     dwStyle;		/* the cached window GWL_STYLE */
    INT       iMoveItem;	/* index of tracked item. (Tracking mode) */
    INT       xTrackOffset;	/* distance between the right side of the tracked item and the cursor */
    INT       xOldTrack;	/* track offset (see above) after the last WM_MOUSEMOVE */
    INT       iHotItem;		/* index of hot item (cursor is over this item) */
    INT       iHotDivider;      /* index of the hot divider (used while dragging an item or by HDM_SETHOTDIVIDER) */
    INT       iMargin;          /* width of the margin that surrounds a bitmap */
    INT       filter_change_timeout; /* change timeout set with HDM_SETFILTERCHANGETIMEOUT */

    HIMAGELIST  himl;		/* handle to an image list (may be 0) */
    HEADER_ITEM *items;		/* pointer to array of HEADER_ITEM's */
    INT         *order;         /* array of item IDs indexed by order */
    BOOL	bRectsValid;	/* validity flag for bounding rectangles */
} HEADER_INFO;


#define VERT_BORDER     4
#define DIVIDER_WIDTH  10
#define HOT_DIVIDER_WIDTH 2
#define MAX_HEADER_TEXT_LEN 260
#define HDN_UNICODE_OFFSET 20
#define HDN_FIRST_UNICODE (HDN_FIRST-HDN_UNICODE_OFFSET)

#define HDI_SUPPORTED_FIELDS (HDI_WIDTH|HDI_TEXT|HDI_FORMAT|HDI_LPARAM|HDI_BITMAP|HDI_IMAGE|HDI_ORDER)
#define HDI_UNSUPPORTED_FIELDS (HDI_FILTER)
#define HDI_UNKNOWN_FIELDS (~(HDI_SUPPORTED_FIELDS|HDI_UNSUPPORTED_FIELDS|HDI_DI_SETITEM))
#define HDI_COMCTL32_4_0_FIELDS (HDI_WIDTH|HDI_TEXT|HDI_FORMAT|HDI_LPARAM|HDI_BITMAP)


static BOOL HEADER_PrepareCallbackItems(const HEADER_INFO *infoPtr, INT iItem, INT reqMask);
static void HEADER_FreeCallbackItems(HEADER_ITEM *lpItem);
static LRESULT HEADER_SendNotify(const HEADER_INFO *infoPtr, UINT code, NMHDR *hdr);
static LRESULT HEADER_SendCtrlCustomDraw(const HEADER_INFO *infoPtr, DWORD dwDrawStage, HDC hdc, const RECT *rect);

static const WCHAR themeClass[] = L"Header";

static void HEADER_StoreHDItemInHeader(HEADER_ITEM *lpItem, UINT mask, const HDITEMW *phdi, BOOL fUnicode)
{
    if (mask & HDI_UNSUPPORTED_FIELDS)
        FIXME("unsupported header fields %x\n", (mask & HDI_UNSUPPORTED_FIELDS));
    
    if (mask & HDI_BITMAP)
        lpItem->hbm = phdi->hbm;

    if (mask & HDI_FORMAT)
        lpItem->fmt = phdi->fmt;

    if (mask & HDI_LPARAM)
        lpItem->lParam = phdi->lParam;

    if (mask & HDI_WIDTH)
        lpItem->cxy = phdi->cxy;

    if (mask & HDI_IMAGE) 
    {
        lpItem->iImage = phdi->iImage;
        if (phdi->iImage == I_IMAGECALLBACK)
            lpItem->callbackMask |= HDI_IMAGE;
        else
            lpItem->callbackMask &= ~HDI_IMAGE;
    }

    if (mask & HDI_TEXT)
    {
        Free(lpItem->pszText);
        lpItem->pszText = NULL;

        if (phdi->pszText != LPSTR_TEXTCALLBACKW) /* covers != TEXTCALLBACKA too */
        {
            const WCHAR *pszText = phdi->pszText != NULL ? phdi->pszText : L"";
            if (fUnicode)
                Str_SetPtrW(&lpItem->pszText, pszText);
            else
                Str_SetPtrAtoW(&lpItem->pszText, (LPCSTR)pszText);
            lpItem->callbackMask &= ~HDI_TEXT;
        }
        else
        {
            lpItem->pszText = NULL;
            lpItem->callbackMask |= HDI_TEXT;
        }  
    }
}

static inline LRESULT
HEADER_IndexToOrder (const HEADER_INFO *infoPtr, INT iItem)
{
    HEADER_ITEM *lpItem = &infoPtr->items[iItem];
    return lpItem->iOrder;
}


static INT
HEADER_OrderToIndex(const HEADER_INFO *infoPtr, INT iorder)
{
    if ((iorder <0) || iorder >= infoPtr->uNumItem)
      return iorder;
    return infoPtr->order[iorder];
}

static void
HEADER_ChangeItemOrder(const HEADER_INFO *infoPtr, INT iItem, INT iNewOrder)
{
    HEADER_ITEM *lpItem = &infoPtr->items[iItem];
    INT i, nMin, nMax;

    TRACE("%d: %d->%d\n", iItem, lpItem->iOrder, iNewOrder);
    if (lpItem->iOrder < iNewOrder)
    {
        memmove(&infoPtr->order[lpItem->iOrder],
               &infoPtr->order[lpItem->iOrder + 1],
               (iNewOrder - lpItem->iOrder) * sizeof(INT));
    }
    if (iNewOrder < lpItem->iOrder)
    {
        memmove(&infoPtr->order[iNewOrder + 1],
                &infoPtr->order[iNewOrder],
                (lpItem->iOrder - iNewOrder) * sizeof(INT));
    }
    infoPtr->order[iNewOrder] = iItem;
    nMin = min(lpItem->iOrder, iNewOrder);
    nMax = max(lpItem->iOrder, iNewOrder);
    for (i = nMin; i <= nMax; i++)
        infoPtr->items[infoPtr->order[i]].iOrder = i;
}

/* Note: if iItem is the last item then this function returns infoPtr->uNumItem */
static INT
HEADER_NextItem(const HEADER_INFO *infoPtr, INT iItem)
{
    return HEADER_OrderToIndex(infoPtr, HEADER_IndexToOrder(infoPtr, iItem)+1);
}

static INT
HEADER_PrevItem(const HEADER_INFO *infoPtr, INT iItem)
{
    return HEADER_OrderToIndex(infoPtr, HEADER_IndexToOrder(infoPtr, iItem)-1);
}

/* TRUE when item is not resizable with dividers,
   note that valid index should be supplied */
static inline BOOL
HEADER_IsItemFixed(const HEADER_INFO *infoPtr, INT iItem)
{
    return (infoPtr->dwStyle & HDS_NOSIZING) || (infoPtr->items[iItem].fmt & HDF_FIXEDWIDTH);
}

static void
HEADER_SetItemBounds (HEADER_INFO *infoPtr)
{
    HEADER_ITEM *phdi;
    RECT rect;
    unsigned int i;
    int x;

    infoPtr->bRectsValid = TRUE;

    if (infoPtr->uNumItem == 0)
        return;

    GetClientRect (infoPtr->hwndSelf, &rect);

    x = rect.left;
    for (i = 0; i < infoPtr->uNumItem; i++) {
        phdi = &infoPtr->items[HEADER_OrderToIndex(infoPtr,i)];
        phdi->rect.top = rect.top;
        phdi->rect.bottom = rect.bottom;
        phdi->rect.left = x;
        phdi->rect.right = phdi->rect.left + ((phdi->cxy>0)?phdi->cxy:0);
        x = phdi->rect.right;
    }
}

static LRESULT
HEADER_Size (HEADER_INFO *infoPtr)
{
    HEADER_SetItemBounds(infoPtr);
    return 0;
}

static void HEADER_GetHotDividerRect(const HEADER_INFO *infoPtr, RECT *r)
{
    INT iDivider = infoPtr->iHotDivider;
    if (infoPtr->uNumItem > 0)
    {
        HEADER_ITEM *lpItem;
        
        if (iDivider < infoPtr->uNumItem)
        {
            lpItem = &infoPtr->items[iDivider];
            r->left  = lpItem->rect.left - HOT_DIVIDER_WIDTH/2;
            r->right = lpItem->rect.left + HOT_DIVIDER_WIDTH/2;
        }
        else
        {
            lpItem = &infoPtr->items[HEADER_OrderToIndex(infoPtr, infoPtr->uNumItem-1)];
            r->left  = lpItem->rect.right - HOT_DIVIDER_WIDTH/2;
            r->right = lpItem->rect.right + HOT_DIVIDER_WIDTH/2;
        }
        r->top    = lpItem->rect.top;
        r->bottom = lpItem->rect.bottom;
    }
    else
    {
        RECT clientRect;
        GetClientRect(infoPtr->hwndSelf, &clientRect);
        *r = clientRect;
        r->right = r->left + HOT_DIVIDER_WIDTH/2;
    }
}

static void
HEADER_FillItemFrame(HEADER_INFO *infoPtr, HDC hdc, RECT *r, const HEADER_ITEM *item, BOOL hottrack)
{
    HTHEME theme = GetWindowTheme (infoPtr->hwndSelf);

    if (theme) {
        int state = (item->bDown) ? HIS_PRESSED : (hottrack ? HIS_HOT : HIS_NORMAL);
        DrawThemeBackground (theme, hdc, HP_HEADERITEM, state, r, NULL);
        GetThemeBackgroundContentRect (theme, hdc, HP_HEADERITEM, state, r, r);
    }
    else
    {
        HBRUSH hbr = CreateSolidBrush(GetBkColor(hdc));
        FillRect(hdc, r, hbr);
        DeleteObject(hbr);
    }
}

static void
HEADER_DrawItemFrame(HEADER_INFO *infoPtr, HDC hdc, RECT *r, const HEADER_ITEM *item)
{
    if (GetWindowTheme(infoPtr->hwndSelf)) return;

    if (!(infoPtr->dwStyle & HDS_FLAT))
    {
        if (infoPtr->dwStyle & HDS_BUTTONS) {
            if (item->bDown)
                DrawEdge (hdc, r, BDR_RAISEDOUTER, BF_RECT | BF_FLAT | BF_ADJUST);
            else
                DrawEdge (hdc, r, EDGE_RAISED, BF_RECT | BF_SOFT | BF_ADJUST);
        }
        else
            DrawEdge (hdc, r, EDGE_ETCHED, BF_BOTTOM | BF_RIGHT | BF_ADJUST);
    }
}

/* Create a region for the sort arrow with its bounding rect's top-left
   co-ord x,y and its height h. */
static HRGN create_sort_arrow( INT x, INT y, INT h, BOOL is_up )
{
    char buffer[256];
    RGNDATA *data = (RGNDATA *)buffer;
    DWORD size = FIELD_OFFSET(RGNDATA, Buffer[h * sizeof(RECT)]);
    INT i, yinc = 1;
    HRGN rgn;

    if (size > sizeof(buffer))
    {
        data = Alloc( size );
        if (!data) return NULL;
    }
    data->rdh.dwSize = sizeof(data->rdh);
    data->rdh.iType = RDH_RECTANGLES;
    data->rdh.nCount = 0;
    data->rdh.nRgnSize = h * sizeof(RECT);

    if (!is_up)
    {
        y += h - 1;
        yinc = -1;
    }

    x += h - 1; /* set x to the centre */

    for (i = 0; i < h; i++, y += yinc)
    {
        RECT *rect = (RECT *)data->Buffer + data->rdh.nCount;
        rect->left   = x - i;
        rect->top    = y;
        rect->right  = x + i + 1;
        rect->bottom = y + 1;
        data->rdh.nCount++;
    }
    rgn = ExtCreateRegion( NULL, size, data );
    if (data != (RGNDATA *)buffer) Free( data );
    return rgn;
}

static INT
HEADER_DrawItem (HEADER_INFO *infoPtr, HDC hdc, INT iItem, BOOL bHotTrack, LRESULT lCDFlags)
{
    HEADER_ITEM *phdi = &infoPtr->items[iItem];
    RECT r;
    INT  oldBkMode;
    HTHEME theme = GetWindowTheme (infoPtr->hwndSelf);
    NMCUSTOMDRAW nmcd;
    int state = 0;

    TRACE("DrawItem(iItem %d bHotTrack %d unicode flag %d)\n", iItem, bHotTrack, (infoPtr->nNotifyFormat == NFR_UNICODE));

    r = phdi->rect;
    if (r.right - r.left == 0)
	return phdi->rect.right;

    if (theme)
        state = (phdi->bDown) ? HIS_PRESSED : (bHotTrack ? HIS_HOT : HIS_NORMAL);

    /* Set the colors before sending NM_CUSTOMDRAW so that it can change them */
    SetTextColor(hdc, (bHotTrack && !theme) ? comctl32_color.clrHighlight : comctl32_color.clrBtnText);
    SetBkColor(hdc, comctl32_color.clr3dFace);

    if (lCDFlags & CDRF_NOTIFYITEMDRAW && !(phdi->fmt & HDF_OWNERDRAW))
    {
        LRESULT lCDItemFlags;

        nmcd.dwDrawStage  = CDDS_PREPAINT | CDDS_ITEM;
        nmcd.hdc          = hdc;
        nmcd.dwItemSpec   = iItem;
        nmcd.rc           = r;
        nmcd.uItemState   = phdi->bDown ? CDIS_SELECTED : 0;
        nmcd.lItemlParam  = phdi->lParam;

        lCDItemFlags = HEADER_SendNotify(infoPtr, NM_CUSTOMDRAW, (NMHDR *)&nmcd);
        if (lCDItemFlags & CDRF_SKIPDEFAULT)
            return phdi->rect.right;
    }

    /* Fill background, owner could draw over it. */
    HEADER_FillItemFrame(infoPtr, hdc, &r, phdi, bHotTrack);

    if (phdi->fmt & HDF_OWNERDRAW)
    {
	DRAWITEMSTRUCT dis;
        BOOL ret;

	dis.CtlType    = ODT_HEADER;
	dis.CtlID      = GetWindowLongPtrW (infoPtr->hwndSelf, GWLP_ID);
	dis.itemID     = iItem;
	dis.itemAction = ODA_DRAWENTIRE;
	dis.itemState  = phdi->bDown ? ODS_SELECTED : 0;
	dis.hwndItem   = infoPtr->hwndSelf;
	dis.hDC        = hdc;
	dis.rcItem     = phdi->rect;
	dis.itemData   = phdi->lParam;
        oldBkMode = SetBkMode(hdc, TRANSPARENT);
        ret = SendMessageW (infoPtr->hwndNotify, WM_DRAWITEM, dis.CtlID, (LPARAM)&dis);
        if (oldBkMode != TRANSPARENT)
            SetBkMode(hdc, oldBkMode);

        if (!ret)
            HEADER_FillItemFrame(infoPtr, hdc, &r, phdi, bHotTrack);

        /* Edges are always drawn if we don't have attached theme. */
        HEADER_DrawItemFrame(infoPtr, hdc, &r, phdi);
        /* If application processed WM_DRAWITEM we should skip label painting,
           edges are drawn no matter what. */
        if (ret) return phdi->rect.right;
    }
    else
        HEADER_DrawItemFrame(infoPtr, hdc, &r, phdi);

    if (phdi->bDown) {
        r.left += 2;
        r.top  += 2;
    }

    /* Now text and image */
    {
	INT rw, rh; /* width and height of r */
        INT *x = NULL; /* x and ... */
        UINT *w = NULL; /* ...  width of the pic (bmp or img) which is part of cnt */
	  /* cnt,txt,img,bmp */
        INT  cx, tx, ix, bx;
	UINT cw, tw, iw, bw;
        INT img_cx, img_cy;
        INT sort_w, sort_x, sort_h;
	BITMAP bmp;

        HEADER_PrepareCallbackItems(infoPtr, iItem, HDI_TEXT|HDI_IMAGE);
        cw = iw = bw = sort_w = sort_h = 0;
	rw = r.right - r.left;
	rh = r.bottom - r.top;

	if (phdi->fmt & HDF_STRING) {
	    RECT textRect;

            SetRectEmpty(&textRect);

	    if (theme) {
		GetThemeTextExtent(theme, hdc, HP_HEADERITEM, state, phdi->pszText, -1,
		    DT_LEFT|DT_VCENTER|DT_SINGLELINE, NULL, &textRect);
	    } else {
		DrawTextW (hdc, phdi->pszText, -1,
			&textRect, DT_LEFT|DT_VCENTER|DT_SINGLELINE|DT_CALCRECT);
	    }
	    cw = textRect.right - textRect.left + 2 * infoPtr->iMargin;
	}

        if (phdi->fmt & (HDF_SORTUP | HDF_SORTDOWN)) {
            sort_h = MulDiv( infoPtr->nHeight - VERT_BORDER, 4, 13 );
            sort_w = 2 * sort_h - 1 + infoPtr->iMargin * 2;
            cw += sort_w;
        } else { /* sort arrows take precedent over images/bitmaps */
            if ((phdi->fmt & HDF_IMAGE) && ImageList_GetIconSize( infoPtr->himl, &img_cx, &img_cy )) {
                iw = img_cx + 2 * infoPtr->iMargin;
                x = &ix;
                w = &iw;
            }

            if ((phdi->fmt & HDF_BITMAP) && (phdi->hbm)) {
                GetObjectW (phdi->hbm, sizeof(BITMAP), &bmp);
                bw = bmp.bmWidth + 2 * infoPtr->iMargin;
                if (!iw) {
                    x = &bx;
                    w = &bw;
                }
            }
            if (bw || iw)
                cw += *w;
        }

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

        tw -= sort_w;
        sort_x = cx + tw + infoPtr->iMargin * 3;

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

	if (sort_w || iw || bw) {
	    HDC hClipDC = GetDC(infoPtr->hwndSelf);
	    HRGN hClipRgn = CreateRectRgn(r.left, r.top, r.right, r.bottom);
	    SelectClipRgn(hClipDC, hClipRgn);
	    
            if (sort_w) {
                HRGN arrow = create_sort_arrow( sort_x, r.top + (rh - sort_h) / 2,
                                                sort_h, phdi->fmt & HDF_SORTUP );
                if (arrow) {
                    FillRgn( hClipDC, arrow, GetSysColorBrush( COLOR_GRAYTEXT ) );
                    DeleteObject( arrow );
                }
            }

	    if (bw) {
	        HDC hdcBitmap = CreateCompatibleDC (hClipDC);
	        SelectObject (hdcBitmap, phdi->hbm);
	        BitBlt (hClipDC, bx, r.top + (rh - bmp.bmHeight) / 2,
		        bmp.bmWidth, bmp.bmHeight, hdcBitmap, 0, 0, SRCCOPY);
	        DeleteDC (hdcBitmap);
	    }

	    if (iw) {
	        ImageList_DrawEx (infoPtr->himl, phdi->iImage, hClipDC, 
	                          ix, r.top + (rh - img_cy) / 2,
	                          img_cx, img_cy, CLR_DEFAULT, CLR_DEFAULT, 0);
	    }

	    DeleteObject(hClipRgn);
	    ReleaseDC(infoPtr->hwndSelf, hClipDC);
	}
        
	if (((phdi->fmt & HDF_STRING)
		|| (!(phdi->fmt & (HDF_OWNERDRAW|HDF_STRING|HDF_BITMAP|
				   HDF_BITMAP_ON_RIGHT|HDF_IMAGE)))) /* no explicit format specified? */
	    && (phdi->pszText)) {
	    oldBkMode = SetBkMode(hdc, TRANSPARENT);
	    r.left  = tx;
	    r.right = tx + tw;
	    if (theme) {
		DrawThemeText(theme, hdc, HP_HEADERITEM, state, phdi->pszText,
			    -1, DT_LEFT|DT_END_ELLIPSIS|DT_VCENTER|DT_SINGLELINE,
			    0, &r);
	    } else {
		DrawTextW (hdc, phdi->pszText, -1,
			&r, DT_LEFT|DT_END_ELLIPSIS|DT_VCENTER|DT_SINGLELINE);
	    }
	    if (oldBkMode != TRANSPARENT)
	        SetBkMode(hdc, oldBkMode);
        }
        HEADER_FreeCallbackItems(phdi);
    }

    return phdi->rect.right;
}

static void
HEADER_DrawHotDivider(const HEADER_INFO *infoPtr, HDC hdc)
{
    HBRUSH brush;
    RECT r;
    
    HEADER_GetHotDividerRect(infoPtr, &r);
    brush = CreateSolidBrush(comctl32_color.clrHighlight);
    FillRect(hdc, &r, brush);
    DeleteObject(brush);
}

static void
HEADER_Refresh (HEADER_INFO *infoPtr, HDC hdc)
{
    HFONT hFont, hOldFont;
    RECT rect, rcRest;
    HBRUSH hbrBk;
    UINT i;
    INT x;
    LRESULT lCDFlags;
    HTHEME theme = GetWindowTheme (infoPtr->hwndSelf);

    if (!infoPtr->bRectsValid)
        HEADER_SetItemBounds(infoPtr);

    /* get rect for the bar, adjusted for the border */
    GetClientRect (infoPtr->hwndSelf, &rect);
    lCDFlags = HEADER_SendCtrlCustomDraw(infoPtr, CDDS_PREPAINT, hdc, &rect);
    
    if (infoPtr->bDragging)
	ImageList_DragShowNolock(FALSE);

    hFont = infoPtr->hFont ? infoPtr->hFont : GetStockObject (SYSTEM_FONT);
    hOldFont = SelectObject (hdc, hFont);

    /* draw Background */
    if (infoPtr->uNumItem == 0 && theme == NULL) {
        hbrBk = GetSysColorBrush(COLOR_3DFACE);
        FillRect(hdc, &rect, hbrBk);
    }

    x = rect.left;
    for (i = 0; x <= rect.right && i < infoPtr->uNumItem; i++) {
        int idx = HEADER_OrderToIndex(infoPtr,i);
        if (RectVisible(hdc, &infoPtr->items[idx].rect))
            HEADER_DrawItem(infoPtr, hdc, idx, infoPtr->iHotItem == idx, lCDFlags);
        x = infoPtr->items[idx].rect.right;
    }

    rcRest = rect;
    rcRest.left = x;
    if ((x <= rect.right) && RectVisible(hdc, &rcRest) && (infoPtr->uNumItem > 0)) {
        if (theme != NULL) {
            DrawThemeBackground(theme, hdc, HP_HEADERITEM, HIS_NORMAL, &rcRest, NULL);
        }
        else if (infoPtr->dwStyle & HDS_FLAT) {
            hbrBk = GetSysColorBrush(COLOR_3DFACE);
            FillRect(hdc, &rcRest, hbrBk);
        }
        else
        {
            if (infoPtr->dwStyle & HDS_BUTTONS)
                DrawEdge (hdc, &rcRest, EDGE_RAISED, BF_TOP|BF_LEFT|BF_BOTTOM|BF_SOFT|BF_MIDDLE);
            else
                DrawEdge (hdc, &rcRest, EDGE_ETCHED, BF_BOTTOM|BF_MIDDLE);
        }
    }

    if (infoPtr->iHotDivider != -1)
        HEADER_DrawHotDivider(infoPtr, hdc);

    if (infoPtr->bDragging)
	ImageList_DragShowNolock(TRUE);
    SelectObject (hdc, hOldFont);
    
    if (lCDFlags & CDRF_NOTIFYPOSTPAINT)
        HEADER_SendCtrlCustomDraw(infoPtr, CDDS_POSTPAINT, hdc, &rect);
}


static void
HEADER_RefreshItem (HEADER_INFO *infoPtr, INT iItem)
{
    if (!infoPtr->bRectsValid)
        HEADER_SetItemBounds(infoPtr);

    InvalidateRect(infoPtr->hwndSelf, &infoPtr->items[iItem].rect, FALSE);
}


static void
HEADER_InternalHitTest (const HEADER_INFO *infoPtr, const POINT *lpPt, UINT *pFlags, INT *pItem)
{
    RECT rect, rcTest;
    UINT iCount;
    INT width;
    BOOL bNoWidth;

    GetClientRect (infoPtr->hwndSelf, &rect);

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
                    if (HEADER_IndexToOrder(infoPtr, iCount) > 0) {
			rcTest = rect;
			rcTest.right = rcTest.left + DIVIDER_WIDTH;
			if (PtInRect (&rcTest, *lpPt)) {
			    if (HEADER_IsItemFixed(infoPtr, HEADER_PrevItem(infoPtr, iCount)))
			    {
				*pFlags |= HHT_ONHEADER;
                                *pItem = iCount;
				TRACE("ON HEADER %d\n", *pItem);
				return;
			    }
			    if (bNoWidth) {
				*pFlags |= HHT_ONDIVOPEN;
                                *pItem = HEADER_PrevItem(infoPtr, iCount);
				TRACE("ON DIVOPEN %d\n", *pItem);
				return;
			    }
			    else {
				*pFlags |= HHT_ONDIVIDER;
                                *pItem = HEADER_PrevItem(infoPtr, iCount);
				TRACE("ON DIVIDER %d\n", *pItem);
				return;
			    }
			}
		    }
		    rcTest = rect;
		    rcTest.left = rcTest.right - DIVIDER_WIDTH;
		    if (!HEADER_IsItemFixed(infoPtr, iCount) && PtInRect (&rcTest, *lpPt))
		    {
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
	    if (!HEADER_IsItemFixed(infoPtr, infoPtr->uNumItem - 1))
	    {
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
			*pItem = infoPtr->uNumItem - 1;
			TRACE("ON DIVIDER %d\n", *pItem);
			return;
		    }
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
HEADER_DrawTrackLine (const HEADER_INFO *infoPtr, HDC hdc, INT x)
{
    RECT rect;

    GetClientRect (infoPtr->hwndSelf, &rect);
    PatBlt( hdc, x, rect.top, 1, rect.bottom - rect.top, DSTINVERT );
}

/***
 * DESCRIPTION:
 * Convert a HDITEM into the correct format (ANSI/Unicode) to send it in a notify
 *
 * PARAMETER(S):
 * [I] infoPtr : the header that wants to send the notify
 * [O] dest : The buffer to store the HDITEM for notify. It may be set to a HDITEMA of HDITEMW
 * [I] src  : The source HDITEM. It may be a HDITEMA or HDITEMW
 * [I] fSourceUnicode : is src a HDITEMW or HDITEMA
 * [O] ppvScratch : a pointer to a scratch buffer that needs to be freed after
 *                  the HDITEM is no longer in use or NULL if none was needed
 * 
 * NOTE: We depend on HDITEMA and HDITEMW having the same structure
 */
static void HEADER_CopyHDItemForNotify(const HEADER_INFO *infoPtr, HDITEMW *dest,
    const HDITEMW *src, BOOL fSourceUnicode, LPVOID *ppvScratch)
{
    *ppvScratch = NULL;
    *dest = *src;
    
    if (src->mask & HDI_TEXT && src->pszText != LPSTR_TEXTCALLBACKW) /* covers TEXTCALLBACKA as well */
    {
        if (fSourceUnicode && infoPtr->nNotifyFormat != NFR_UNICODE)
        {
            dest->pszText = NULL;
            Str_SetPtrWtoA((LPSTR *)&dest->pszText, src->pszText);
            *ppvScratch = dest->pszText;
        }
        
        if (!fSourceUnicode && infoPtr->nNotifyFormat == NFR_UNICODE)
        {
            dest->pszText = NULL;
            Str_SetPtrAtoW(&dest->pszText, (LPSTR)src->pszText);
            *ppvScratch = dest->pszText;
        }
    }
}

static UINT HEADER_NotifyCodeWtoA(UINT code)
{
    /* we use the fact that all the unicode messages are in HDN_FIRST_UNICODE..HDN_LAST*/
    if (code >= HDN_LAST && code <= HDN_FIRST_UNICODE)
        return code + HDN_UNICODE_OFFSET;
    else
        return code;
}

static LRESULT
HEADER_SendNotify(const HEADER_INFO *infoPtr, UINT code, NMHDR *nmhdr)
{
    nmhdr->hwndFrom = infoPtr->hwndSelf;
    nmhdr->idFrom   = GetWindowLongPtrW (infoPtr->hwndSelf, GWLP_ID);
    nmhdr->code     = code;

    return SendMessageW(infoPtr->hwndNotify, WM_NOTIFY,
				   nmhdr->idFrom, (LPARAM)nmhdr);
}

static BOOL
HEADER_SendSimpleNotify (const HEADER_INFO *infoPtr, UINT code)
{
    NMHDR nmhdr;
    return (BOOL)HEADER_SendNotify(infoPtr, code, &nmhdr);
}

static LRESULT
HEADER_SendCtrlCustomDraw(const HEADER_INFO *infoPtr, DWORD dwDrawStage, HDC hdc, const RECT *rect)
{
    NMCUSTOMDRAW nm;
    nm.dwDrawStage = dwDrawStage;
    nm.hdc = hdc;
    nm.rc = *rect;
    nm.dwItemSpec = 0;
    nm.uItemState = 0;
    nm.lItemlParam = 0;

    return HEADER_SendNotify(infoPtr, NM_CUSTOMDRAW, (NMHDR *)&nm);
}

static BOOL
HEADER_SendNotifyWithHDItemT(const HEADER_INFO *infoPtr, UINT code, INT iItem, HDITEMW *lpItem)
{
    NMHEADERW nmhdr;
    
    if (infoPtr->nNotifyFormat != NFR_UNICODE)
        code = HEADER_NotifyCodeWtoA(code);
    nmhdr.iItem = iItem;
    nmhdr.iButton = 0;
    nmhdr.pitem = lpItem;

    return (BOOL)HEADER_SendNotify(infoPtr, code, (NMHDR *)&nmhdr);
}

static BOOL
HEADER_SendNotifyWithIntFieldT(const HEADER_INFO *infoPtr, UINT code, INT iItem, INT mask, INT iValue)
{
    HDITEMW nmitem;

    /* copying only the iValue should be ok but to make the code more robust we copy everything */
    nmitem.cxy = infoPtr->items[iItem].cxy;
    nmitem.hbm = infoPtr->items[iItem].hbm;
    nmitem.pszText = NULL;
    nmitem.cchTextMax = 0;
    nmitem.fmt = infoPtr->items[iItem].fmt;
    nmitem.lParam = infoPtr->items[iItem].lParam;
    nmitem.iOrder = infoPtr->items[iItem].iOrder;
    nmitem.iImage = infoPtr->items[iItem].iImage;

    nmitem.mask = mask;
    switch (mask)
    {
	case HDI_WIDTH:
	    nmitem.cxy = iValue;
	    break;
	case HDI_ORDER:
	    nmitem.iOrder = iValue;
	    break;
	default:
	    ERR("invalid mask value 0x%x\n", iValue);
    }

    return HEADER_SendNotifyWithHDItemT(infoPtr, code, iItem, &nmitem);
}

/**
 * Prepare callback items
 *   depends on NMHDDISPINFOW having same structure as NMHDDISPINFOA 
 *   (so we handle the two cases only doing a specific cast for pszText).
 * Checks if any of the required fields is a callback. If this is the case sends a
 * NMHDISPINFO notify to retrieve these items. The items are stored in the
 * HEADER_ITEM pszText and iImage fields. They should be Freed with
 * HEADER_FreeCallbackItems.
 *
 * @param hwnd : hwnd header container handler
 * @param iItem : the header item id
 * @param reqMask : required fields. If any of them is callback this function will fetch it
 *
 * @return TRUE on success, else FALSE
 */
static BOOL
HEADER_PrepareCallbackItems(const HEADER_INFO *infoPtr, INT iItem, INT reqMask)
{
    HEADER_ITEM *lpItem = &infoPtr->items[iItem];
    DWORD mask = reqMask & lpItem->callbackMask;
    NMHDDISPINFOW dispInfo;
    void *pvBuffer = NULL;

    if (mask == 0)
        return TRUE;
    if (mask&HDI_TEXT && lpItem->pszText != NULL)
    {
        ERR("(): function called without a call to FreeCallbackItems\n");
        Free(lpItem->pszText);
        lpItem->pszText = NULL;
    }
    
    memset(&dispInfo, 0, sizeof(NMHDDISPINFOW));
    dispInfo.hdr.hwndFrom = infoPtr->hwndSelf;
    dispInfo.hdr.idFrom   = GetWindowLongPtrW (infoPtr->hwndSelf, GWLP_ID);
    if (infoPtr->nNotifyFormat == NFR_UNICODE)
    {
        dispInfo.hdr.code = HDN_GETDISPINFOW;
        if (mask & HDI_TEXT)
            pvBuffer = Alloc(MAX_HEADER_TEXT_LEN * sizeof(WCHAR));
    }
    else
    {
        dispInfo.hdr.code = HDN_GETDISPINFOA;
        if (mask & HDI_TEXT)
            pvBuffer = Alloc(MAX_HEADER_TEXT_LEN * sizeof(CHAR));
    }
    dispInfo.pszText      = pvBuffer;
    dispInfo.cchTextMax   = (pvBuffer!=NULL?MAX_HEADER_TEXT_LEN:0);
    dispInfo.iItem        = iItem;
    dispInfo.mask         = mask;
    dispInfo.lParam       = lpItem->lParam;
    
    TRACE("Sending HDN_GETDISPINFO%c\n", infoPtr->nNotifyFormat == NFR_UNICODE?'W':'A');
    SendMessageW(infoPtr->hwndNotify, WM_NOTIFY, dispInfo.hdr.idFrom, (LPARAM)&dispInfo);

    TRACE("SendMessage returns(mask:0x%x,str:%s,lParam:%p)\n", 
          dispInfo.mask,
          (infoPtr->nNotifyFormat == NFR_UNICODE ? debugstr_w(dispInfo.pszText) : (LPSTR) dispInfo.pszText),
          (void*) dispInfo.lParam);
          
    if (mask & HDI_IMAGE)
        lpItem->iImage = dispInfo.iImage;
    if (mask & HDI_TEXT)
    {
        if (infoPtr->nNotifyFormat == NFR_UNICODE)
        {
            lpItem->pszText = pvBuffer;

            /* the user might have used his own buffer */
            if (dispInfo.pszText != lpItem->pszText)
                Str_GetPtrW(dispInfo.pszText, lpItem->pszText, MAX_HEADER_TEXT_LEN);
        }
        else
        {
            Str_SetPtrAtoW(&lpItem->pszText, (LPSTR)dispInfo.pszText);
            Free(pvBuffer);
        }
    }
        
    if (dispInfo.mask & HDI_DI_SETITEM) 
    {
        /* make the items permanent */
        lpItem->callbackMask &= ~dispInfo.mask;
    }
    
    return TRUE;
}

/***
 * DESCRIPTION:
 * Free the items that might be allocated with HEADER_PrepareCallbackItems
 *
 * PARAMETER(S):
 * [I] lpItem : the item to free the data
 *
 */
static void
HEADER_FreeCallbackItems(HEADER_ITEM *lpItem)
{
    if (lpItem->callbackMask&HDI_TEXT)
    {
        Free(lpItem->pszText);
        lpItem->pszText = NULL;
    }

    if (lpItem->callbackMask&HDI_IMAGE)
        lpItem->iImage = I_IMAGECALLBACK;
}

static HIMAGELIST
HEADER_CreateDragImage (HEADER_INFO *infoPtr, INT iItem)
{
    HEADER_ITEM *lpItem;
    HIMAGELIST himl;
    HBITMAP hMemory, hOldBitmap;
    LRESULT lCDFlags;
    RECT rc;
    HDC hMemoryDC;
    HDC hDeviceDC;
    int height, width;
    HFONT hFont;
    
    if (iItem >= infoPtr->uNumItem)
        return NULL;

    if (!infoPtr->bRectsValid)
        HEADER_SetItemBounds(infoPtr);

    lpItem = &infoPtr->items[iItem];
    width = lpItem->rect.right - lpItem->rect.left;
    height = lpItem->rect.bottom - lpItem->rect.top;
    
    hDeviceDC = GetDC(NULL);
    hMemoryDC = CreateCompatibleDC(hDeviceDC);
    hMemory = CreateCompatibleBitmap(hDeviceDC, width, height);
    ReleaseDC(NULL, hDeviceDC);
    hOldBitmap = SelectObject(hMemoryDC, hMemory);
    SetViewportOrgEx(hMemoryDC, -lpItem->rect.left, -lpItem->rect.top, NULL);
    hFont = infoPtr->hFont ? infoPtr->hFont : GetStockObject(SYSTEM_FONT);
    SelectObject(hMemoryDC, hFont);

    GetClientRect(infoPtr->hwndSelf, &rc);
    lCDFlags = HEADER_SendCtrlCustomDraw(infoPtr, CDDS_PREPAINT, hMemoryDC, &rc);
    HEADER_DrawItem(infoPtr, hMemoryDC, iItem, FALSE, lCDFlags);
    if (lCDFlags & CDRF_NOTIFYPOSTPAINT)
        HEADER_SendCtrlCustomDraw(infoPtr, CDDS_POSTPAINT, hMemoryDC, &rc);
    
    hMemory = SelectObject(hMemoryDC, hOldBitmap);
    DeleteDC(hMemoryDC);
    
    if (hMemory == NULL)    /* if anything failed */
        return NULL;

    himl = ImageList_Create(width, height, ILC_COLORDDB, 1, 1);
    ImageList_Add(himl, hMemory, NULL);
    DeleteObject(hMemory);
    return himl;
}

static LRESULT
HEADER_SetHotDivider(HEADER_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    INT iDivider;
    RECT r;
    
    if (wParam)
    {
        POINT pt;
        UINT flags;
        pt.x = (INT)(SHORT)LOWORD(lParam);
        pt.y = 0;
        HEADER_InternalHitTest (infoPtr, &pt, &flags, &iDivider);
        
        if (flags & HHT_TOLEFT)
            iDivider = 0;
        else if (flags & HHT_NOWHERE || flags & HHT_TORIGHT)
            iDivider = infoPtr->uNumItem;
        else
        {
            HEADER_ITEM *lpItem = &infoPtr->items[iDivider];
            if (pt.x > (lpItem->rect.left+lpItem->rect.right)/2)
                iDivider = HEADER_NextItem(infoPtr, iDivider);
        }
    }
    else
        iDivider = (INT)lParam;
        
    /* Note; wParam==FALSE, lParam==-1 is valid and is used to clear the hot divider */
    if (iDivider<-1 || iDivider>(int)infoPtr->uNumItem)
        return iDivider;

    if (iDivider != infoPtr->iHotDivider)
    {
        if (infoPtr->iHotDivider != -1)
        {
            HEADER_GetHotDividerRect(infoPtr, &r);
            InvalidateRect(infoPtr->hwndSelf, &r, FALSE);
        }
        infoPtr->iHotDivider = iDivider;
        if (iDivider != -1)
        {
            HEADER_GetHotDividerRect(infoPtr, &r);
            InvalidateRect(infoPtr->hwndSelf, &r, FALSE);
        }
    }
    return iDivider;
}

static LRESULT
HEADER_DeleteItem (HEADER_INFO *infoPtr, INT iItem)
{
    INT iOrder;
    UINT i;

    TRACE("[iItem=%d]\n", iItem);

    if ((iItem < 0) || (iItem >= (INT)infoPtr->uNumItem))
        return FALSE;

    for (i = 0; i < infoPtr->uNumItem; i++)
       TRACE("%d: order=%d, iOrder=%d, ->iOrder=%d\n", i, infoPtr->order[i], infoPtr->items[i].iOrder, infoPtr->items[infoPtr->order[i]].iOrder);

    iOrder = infoPtr->items[iItem].iOrder;
    Free(infoPtr->items[iItem].pszText);

    infoPtr->uNumItem--;
    memmove(&infoPtr->items[iItem], &infoPtr->items[iItem + 1],
            (infoPtr->uNumItem - iItem) * sizeof(HEADER_ITEM));
    memmove(&infoPtr->order[iOrder], &infoPtr->order[iOrder + 1],
            (infoPtr->uNumItem - iOrder) * sizeof(INT));
    infoPtr->items = ReAlloc(infoPtr->items, sizeof(HEADER_ITEM) * infoPtr->uNumItem);
    infoPtr->order = ReAlloc(infoPtr->order, sizeof(INT) * infoPtr->uNumItem);
        
    /* Correct the orders */
    for (i = 0; i < infoPtr->uNumItem; i++)
    {
        if (infoPtr->order[i] > iItem)
            infoPtr->order[i]--;
        if (i >= iOrder)
            infoPtr->items[infoPtr->order[i]].iOrder = i;
    }
    for (i = 0; i < infoPtr->uNumItem; i++)
       TRACE("%d: order=%d, iOrder=%d, ->iOrder=%d\n", i, infoPtr->order[i], infoPtr->items[i].iOrder, infoPtr->items[infoPtr->order[i]].iOrder);

    HEADER_SetItemBounds (infoPtr);
    InvalidateRect(infoPtr->hwndSelf, NULL, FALSE);

    return TRUE;
}


static LRESULT
HEADER_GetImageList (const HEADER_INFO *infoPtr)
{
    return (LRESULT)infoPtr->himl;
}


static LRESULT
HEADER_GetItemT (const HEADER_INFO *infoPtr, INT nItem, LPHDITEMW phdi, BOOL bUnicode)
{
    HEADER_ITEM *lpItem;
    UINT mask;

    if (!phdi)
	return FALSE;

    TRACE("[nItem=%d]\n", nItem);

    mask = phdi->mask;
    if (mask == 0)
	return TRUE;

    if ((nItem < 0) || (nItem >= (INT)infoPtr->uNumItem))
        return FALSE;

    if (mask & HDI_UNKNOWN_FIELDS)
    {
        TRACE("mask %x contains unknown fields. Using only comctl32 4.0 fields\n", mask);
        mask &= HDI_COMCTL32_4_0_FIELDS;
    }
    
    lpItem = &infoPtr->items[nItem];
    HEADER_PrepareCallbackItems(infoPtr, nItem, mask);

    if (mask & HDI_BITMAP)
        phdi->hbm = lpItem->hbm;

    if (mask & HDI_FORMAT)
        phdi->fmt = lpItem->fmt;

    if (mask & HDI_WIDTH)
        phdi->cxy = lpItem->cxy;

    if (mask & HDI_LPARAM)
        phdi->lParam = lpItem->lParam;

    if (mask & HDI_IMAGE) 
        phdi->iImage = lpItem->iImage;

    if (mask & HDI_ORDER)
        phdi->iOrder = lpItem->iOrder;

    if (mask & HDI_TEXT)
    {
        if (bUnicode)
            Str_GetPtrW (lpItem->pszText, phdi->pszText, phdi->cchTextMax);
        else
            Str_GetPtrWtoA (lpItem->pszText, (LPSTR)phdi->pszText, phdi->cchTextMax);
    }

    HEADER_FreeCallbackItems(lpItem);
    return TRUE;
}


static inline LRESULT
HEADER_GetItemCount (const HEADER_INFO *infoPtr)
{
    return infoPtr->uNumItem;
}


static LRESULT
HEADER_GetItemRect (const HEADER_INFO *infoPtr, INT iItem, LPRECT lpRect)
{
    if ((iItem < 0) || (iItem >= (INT)infoPtr->uNumItem))
        return FALSE;

    lpRect->left   = infoPtr->items[iItem].rect.left;
    lpRect->right  = infoPtr->items[iItem].rect.right;
    lpRect->top    = infoPtr->items[iItem].rect.top;
    lpRect->bottom = infoPtr->items[iItem].rect.bottom;

    return TRUE;
}


static LRESULT
HEADER_GetOrderArray(const HEADER_INFO *infoPtr, INT size, LPINT order)
{
    if ((UINT)size <infoPtr->uNumItem)
      return FALSE;

    memcpy(order, infoPtr->order, infoPtr->uNumItem * sizeof(INT));
    return TRUE;
}

/* Returns index of first duplicate 'value' from [0,to) range,
   or -1 if there isn't any */
static INT has_duplicate(const INT *array, INT to, INT value)
{
    INT i;
    for(i = 0; i < to; i++)
        if (array[i] == value) return i;
    return -1;
}

/* returns next available value from [0,max] not to duplicate in [0,to) */
static INT get_nextvalue(const INT *array, INT to, INT max)
{
    INT i;
    for(i = 0; i < max; i++)
        if (has_duplicate(array, to, i) == -1) return i;
    return 0;
}

static LRESULT
HEADER_SetOrderArray(HEADER_INFO *infoPtr, INT size, const INT *order)
{
    HEADER_ITEM *lpItem;
    INT i;

    if ((UINT)size != infoPtr->uNumItem)
      return FALSE;

    if (TRACE_ON(header))
    {
        TRACE("count=%d, order array={", size);
        for (i = 0; i < size; i++)
            TRACE("%d%c", order[i], i != size-1 ? ',' : '}');
        TRACE("\n");
    }

    for (i=0; i<size; i++)
    {
        if (order[i] >= size || order[i] < 0)
           /* on invalid index get next available */
           /* FIXME: if i==0 array item is out of range behaviour is
                     different, see tests */
           infoPtr->order[i] = get_nextvalue(infoPtr->order, i, size);
        else
        {
           INT j, dup;

           infoPtr->order[i] = order[i];
           j = i;
           /* remove duplicates */
           while ((dup = has_duplicate(infoPtr->order, j, order[j])) != -1)
           {
               INT next;

               next = get_nextvalue(infoPtr->order, j, size);
               infoPtr->order[dup] = next;
               j--;
           }
        }
    }
    /* sync with item data */
    for (i=0; i<size; i++)
    {
        lpItem = &infoPtr->items[infoPtr->order[i]];
        lpItem->iOrder = i;
    }
    HEADER_SetItemBounds(infoPtr);
    InvalidateRect(infoPtr->hwndSelf, NULL, FALSE);
    return TRUE;
}

static inline LRESULT
HEADER_GetUnicodeFormat (const HEADER_INFO *infoPtr)
{
    return (infoPtr->nNotifyFormat == NFR_UNICODE);
}


static LRESULT
HEADER_HitTest (const HEADER_INFO *infoPtr, LPHDHITTESTINFO phti)
{
    UINT outside = HHT_NOWHERE | HHT_ABOVE | HHT_BELOW | HHT_TOLEFT | HHT_TORIGHT;

    HEADER_InternalHitTest (infoPtr, &phti->pt, &phti->flags, &phti->iItem);

    if (phti->flags & outside)
	return phti->iItem = -1;
    else
        return phti->iItem;
}


static LRESULT
HEADER_InsertItemT (HEADER_INFO *infoPtr, INT nItem, const HDITEMW *phdi, BOOL bUnicode)
{
    HEADER_ITEM *lpItem;
    INT       iOrder;
    UINT      i;
    UINT      copyMask;

    if ((phdi == NULL) || (nItem < 0) || (phdi->mask == 0))
	return -1;

    if (nItem > infoPtr->uNumItem)
        nItem = infoPtr->uNumItem;

    iOrder = (phdi->mask & HDI_ORDER) ? phdi->iOrder : nItem;
    if (iOrder < 0)
        iOrder = 0;
    else if (infoPtr->uNumItem < iOrder)
        iOrder = infoPtr->uNumItem;

    infoPtr->uNumItem++;
    infoPtr->items = ReAlloc(infoPtr->items, sizeof(HEADER_ITEM) * infoPtr->uNumItem);
    infoPtr->order = ReAlloc(infoPtr->order, sizeof(INT) * infoPtr->uNumItem);
    
    /* make space for the new item */
    memmove(&infoPtr->items[nItem + 1], &infoPtr->items[nItem],
            (infoPtr->uNumItem - nItem - 1) * sizeof(HEADER_ITEM));
    memmove(&infoPtr->order[iOrder + 1], &infoPtr->order[iOrder],
           (infoPtr->uNumItem - iOrder - 1) * sizeof(INT));

    /* update the order array */
    infoPtr->order[iOrder] = nItem;
    for (i = 0; i < infoPtr->uNumItem; i++)
    {
        if (i != iOrder && infoPtr->order[i] >= nItem)
            infoPtr->order[i]++;
        infoPtr->items[infoPtr->order[i]].iOrder = i;
    }

    lpItem = &infoPtr->items[nItem];
    ZeroMemory(lpItem, sizeof(HEADER_ITEM));
    /* cxy, fmt and lParam are copied even if not in the HDITEM mask */
    copyMask = phdi->mask | HDI_WIDTH | HDI_FORMAT | HDI_LPARAM;
    HEADER_StoreHDItemInHeader(lpItem, copyMask, phdi, bUnicode);
    lpItem->iOrder = iOrder;

    /* set automatically some format bits */
    if (phdi->mask & HDI_TEXT)
        lpItem->fmt |= HDF_STRING;
    else
        lpItem->fmt &= ~HDF_STRING;

    if (lpItem->hbm != NULL)
        lpItem->fmt |= HDF_BITMAP;
    else
        lpItem->fmt &= ~HDF_BITMAP;

    if (phdi->mask & HDI_IMAGE)
        lpItem->fmt |= HDF_IMAGE;

    HEADER_SetItemBounds (infoPtr);
    InvalidateRect(infoPtr->hwndSelf, NULL, FALSE);

    return nItem;
}


static LRESULT
HEADER_Layout (HEADER_INFO *infoPtr, LPHDLAYOUT lpLayout)
{
    lpLayout->pwpos->hwndInsertAfter = 0;
    lpLayout->pwpos->x = lpLayout->prc->left;
    lpLayout->pwpos->y = lpLayout->prc->top;
    lpLayout->pwpos->cx = lpLayout->prc->right - lpLayout->prc->left;
    if (infoPtr->dwStyle & HDS_HIDDEN)
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
HEADER_SetImageList (HEADER_INFO *infoPtr, HIMAGELIST himl)
{
    HIMAGELIST himlOld;

    TRACE("(himl %p)\n", himl);
    himlOld = infoPtr->himl;
    infoPtr->himl = himl;

    /* FIXME: Refresh needed??? */

    return (LRESULT)himlOld;
}


static LRESULT
HEADER_GetBitmapMargin(const HEADER_INFO *infoPtr)
{
    return infoPtr->iMargin;
}

static LRESULT
HEADER_SetBitmapMargin(HEADER_INFO *infoPtr, INT iMargin)
{
    INT oldMargin = infoPtr->iMargin;

    infoPtr->iMargin = iMargin;

    return oldMargin;
}

static LRESULT
HEADER_SetItemT (HEADER_INFO *infoPtr, INT nItem, const HDITEMW *phdi, BOOL bUnicode)
{
    HEADER_ITEM *lpItem;
    HDITEMW hdNotify;
    void *pvScratch;

    if (phdi == NULL)
	return FALSE;
    if ((nItem < 0) || (nItem >= (INT)infoPtr->uNumItem))
        return FALSE;

    TRACE("[nItem=%d]\n", nItem);

    HEADER_CopyHDItemForNotify(infoPtr, &hdNotify, phdi, bUnicode, &pvScratch);
    if (HEADER_SendNotifyWithHDItemT(infoPtr, HDN_ITEMCHANGINGW, nItem, &hdNotify))
    {
        Free(pvScratch);
        return FALSE;
    }

    lpItem = &infoPtr->items[nItem];
    HEADER_StoreHDItemInHeader(lpItem, phdi->mask, phdi, bUnicode);

    if (phdi->mask & HDI_ORDER)
        if (phdi->iOrder >= 0 && phdi->iOrder < infoPtr->uNumItem)
            HEADER_ChangeItemOrder(infoPtr, nItem, phdi->iOrder);

    HEADER_SendNotifyWithHDItemT(infoPtr, HDN_ITEMCHANGEDW, nItem, &hdNotify);

    HEADER_SetItemBounds (infoPtr);

    InvalidateRect(infoPtr->hwndSelf, NULL, FALSE);

    Free(pvScratch);
    return TRUE;
}

static inline LRESULT
HEADER_SetUnicodeFormat (HEADER_INFO *infoPtr, WPARAM wParam)
{
    BOOL bTemp = (infoPtr->nNotifyFormat == NFR_UNICODE);

    infoPtr->nNotifyFormat = ((BOOL)wParam ? NFR_UNICODE : NFR_ANSI);

    return bTemp;
}


static LRESULT
HEADER_Create (HWND hwnd, const CREATESTRUCTW *lpcs)
{
    HEADER_INFO *infoPtr;
    TEXTMETRICW tm;
    HFONT hOldFont;
    HDC   hdc;

    infoPtr = Alloc(sizeof(*infoPtr));
    SetWindowLongPtrW (hwnd, 0, (DWORD_PTR)infoPtr);

    infoPtr->hwndSelf = hwnd;
    infoPtr->hwndNotify = lpcs->hwndParent;
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
    infoPtr->dwStyle = lpcs->style;
    infoPtr->iMoveItem = 0;
    infoPtr->himl = 0;
    infoPtr->iHotItem = -1;
    infoPtr->iHotDivider = -1;
    infoPtr->iMargin = 3*GetSystemMetrics(SM_CXEDGE);
    infoPtr->nNotifyFormat =
	SendMessageW (infoPtr->hwndNotify, WM_NOTIFYFORMAT, (WPARAM)hwnd, NF_QUERY);
    infoPtr->filter_change_timeout = 1000;

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
HEADER_Destroy (HEADER_INFO *infoPtr)
{
    HTHEME theme = GetWindowTheme(infoPtr->hwndSelf);
    CloseThemeData(theme);
    return 0;
}

static LRESULT
HEADER_NCDestroy (HEADER_INFO *infoPtr)
{
    HEADER_ITEM *lpItem;
    INT nItem;

    if (infoPtr->items) {
        lpItem = infoPtr->items;
        for (nItem = 0; nItem < infoPtr->uNumItem; nItem++, lpItem++)
            Free(lpItem->pszText);
        Free(infoPtr->items);
    }

    Free(infoPtr->order);

    SetWindowLongPtrW (infoPtr->hwndSelf, 0, 0);
    Free(infoPtr);

    return 0;
}


static inline LRESULT
HEADER_GetFont (const HEADER_INFO *infoPtr)
{
    return (LRESULT)infoPtr->hFont;
}


static BOOL
HEADER_IsDragDistance(const HEADER_INFO *infoPtr, const POINT *pt)
{
    /* Windows allows for a mouse movement before starting the drag. We use the
     * SM_CXDOUBLECLICK/SM_CYDOUBLECLICK as that distance.
     */
    return (abs(infoPtr->ptLButtonDown.x - pt->x)>GetSystemMetrics(SM_CXDOUBLECLK) ||
            abs(infoPtr->ptLButtonDown.y - pt->y)>GetSystemMetrics(SM_CYDOUBLECLK));
}

static LRESULT
HEADER_LButtonDblClk (const HEADER_INFO *infoPtr, INT x, INT y)
{
    POINT pt;
    UINT  flags;
    INT   nItem;

    pt.x = x;
    pt.y = y;
    HEADER_InternalHitTest (infoPtr, &pt, &flags, &nItem);

    if ((infoPtr->dwStyle & HDS_BUTTONS) && (flags == HHT_ONHEADER))
        HEADER_SendNotifyWithHDItemT(infoPtr, HDN_ITEMDBLCLICKW, nItem, NULL);
    else if ((flags == HHT_ONDIVIDER) || (flags == HHT_ONDIVOPEN))
        HEADER_SendNotifyWithHDItemT(infoPtr, HDN_DIVIDERDBLCLICKW, nItem, NULL);

    return 0;
}


static LRESULT
HEADER_LButtonDown (HEADER_INFO *infoPtr, INT x, INT y)
{
    POINT pt;
    UINT  flags;
    INT   nItem;
    HDC   hdc;

    pt.x = x;
    pt.y = y;
    HEADER_InternalHitTest (infoPtr, &pt, &flags, &nItem);

    if ((infoPtr->dwStyle & HDS_BUTTONS) && (flags == HHT_ONHEADER)) {
	SetCapture (infoPtr->hwndSelf);
	infoPtr->bCaptured = TRUE;
	infoPtr->bPressed  = TRUE;
	infoPtr->bDragging = FALSE;
	infoPtr->iMoveItem = nItem;
	infoPtr->ptLButtonDown = pt;

	infoPtr->items[nItem].bDown = TRUE;

	/* Send WM_CUSTOMDRAW */
	hdc = GetDC (infoPtr->hwndSelf);
	HEADER_RefreshItem (infoPtr, nItem);
	ReleaseDC (infoPtr->hwndSelf, hdc);

	TRACE("Pressed item %d.\n", nItem);
    }
    else if ((flags == HHT_ONDIVIDER) || (flags == HHT_ONDIVOPEN)) {
        INT iCurrWidth = infoPtr->items[nItem].cxy;
        if (!HEADER_SendNotifyWithIntFieldT(infoPtr, HDN_BEGINTRACKW, nItem, HDI_WIDTH, iCurrWidth))
        {
	    SetCapture (infoPtr->hwndSelf);
	    infoPtr->bCaptured = TRUE;
	    infoPtr->bTracking = TRUE;
	    infoPtr->iMoveItem = nItem;
	    infoPtr->xTrackOffset = infoPtr->items[nItem].rect.right - pt.x;

	    if (!(infoPtr->dwStyle & HDS_FULLDRAG)) {
		infoPtr->xOldTrack = infoPtr->items[nItem].rect.right;
		hdc = GetDC (infoPtr->hwndSelf);
		HEADER_DrawTrackLine (infoPtr, hdc, infoPtr->xOldTrack);
		ReleaseDC (infoPtr->hwndSelf, hdc);
	    }

	    TRACE("Begin tracking item %d.\n", nItem);
	}
    }

    return 0;
}


static LRESULT
HEADER_LButtonUp (HEADER_INFO *infoPtr, INT x, INT y)
{
    POINT pt;
    UINT  flags;
    INT   nItem;
    HDC   hdc;

    pt.x = x;
    pt.y = y;
    HEADER_InternalHitTest (infoPtr, &pt, &flags, &nItem);

    if (infoPtr->bPressed) {

	infoPtr->items[infoPtr->iMoveItem].bDown = FALSE;

	if (infoPtr->bDragging)
	{
            HEADER_ITEM *lpItem = &infoPtr->items[infoPtr->iMoveItem];
            INT iNewOrder;
            
	    ImageList_DragShowNolock(FALSE);
	    ImageList_EndDrag();

            if (infoPtr->iHotDivider == -1)
                iNewOrder = -1;
            else if (infoPtr->iHotDivider == infoPtr->uNumItem)
                iNewOrder = infoPtr->uNumItem-1;
            else
	    {
                iNewOrder = HEADER_IndexToOrder(infoPtr, infoPtr->iHotDivider);
                if (iNewOrder > lpItem->iOrder)
                    iNewOrder--;
            }

            if (iNewOrder != -1 &&
                !HEADER_SendNotifyWithIntFieldT(infoPtr, HDN_ENDDRAG, infoPtr->iMoveItem, HDI_ORDER, iNewOrder))
            {
                HEADER_ChangeItemOrder(infoPtr, infoPtr->iMoveItem, iNewOrder);
		infoPtr->bRectsValid = FALSE;
		InvalidateRect(infoPtr->hwndSelf, NULL, FALSE);
	    }
	    else
		InvalidateRect(infoPtr->hwndSelf, &infoPtr->items[infoPtr->iMoveItem].rect, FALSE);

            infoPtr->bDragging = FALSE;
            HEADER_SetHotDivider(infoPtr, FALSE, -1);
	}
	else
	{
	    hdc = GetDC (infoPtr->hwndSelf);
	    HEADER_RefreshItem (infoPtr, infoPtr->iMoveItem);
	    ReleaseDC (infoPtr->hwndSelf, hdc);

	    if (!(infoPtr->dwStyle & HDS_DRAGDROP) || !HEADER_IsDragDistance(infoPtr, &pt))
		HEADER_SendNotifyWithHDItemT(infoPtr, HDN_ITEMCLICKW, infoPtr->iMoveItem, NULL);
	}

	TRACE("Released item %d.\n", infoPtr->iMoveItem);
	infoPtr->bPressed = FALSE;
    }
    else if (infoPtr->bTracking) {
        INT iNewWidth = pt.x - infoPtr->items[infoPtr->iMoveItem].rect.left + infoPtr->xTrackOffset;
        if (iNewWidth < 0)
	    iNewWidth = 0;
	TRACE("End tracking item %d.\n", infoPtr->iMoveItem);
	infoPtr->bTracking = FALSE;

        HEADER_SendNotifyWithIntFieldT(infoPtr, HDN_ENDTRACKW, infoPtr->iMoveItem, HDI_WIDTH, iNewWidth);

        if (!(infoPtr->dwStyle & HDS_FULLDRAG)) {
	    hdc = GetDC (infoPtr->hwndSelf);
	    HEADER_DrawTrackLine (infoPtr, hdc, infoPtr->xOldTrack);
            ReleaseDC (infoPtr->hwndSelf, hdc);
        }
          
        if (!HEADER_SendNotifyWithIntFieldT(infoPtr, HDN_ITEMCHANGINGW, infoPtr->iMoveItem, HDI_WIDTH, iNewWidth))
        {
            infoPtr->items[infoPtr->iMoveItem].cxy = iNewWidth;
            HEADER_SendNotifyWithIntFieldT(infoPtr, HDN_ITEMCHANGEDW, infoPtr->iMoveItem, HDI_WIDTH, iNewWidth);
        }

	HEADER_SetItemBounds (infoPtr);
	InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);
    }

    if (infoPtr->bCaptured) {
	infoPtr->bCaptured = FALSE;
	ReleaseCapture ();
	HEADER_SendSimpleNotify (infoPtr, NM_RELEASEDCAPTURE);
    }

    return 0;
}


static LRESULT
HEADER_NotifyFormat (HEADER_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    switch (lParam)
    {
	case NF_QUERY:
	    return infoPtr->nNotifyFormat;

	case NF_REQUERY:
	    infoPtr->nNotifyFormat =
		SendMessageW ((HWND)wParam, WM_NOTIFYFORMAT,
                             (WPARAM)infoPtr->hwndSelf, NF_QUERY);
	    return infoPtr->nNotifyFormat;
    }

    return 0;
}

static LRESULT
HEADER_MouseLeave (HEADER_INFO *infoPtr)
{
    /* Reset hot-tracked item when mouse leaves control. */
    INT oldHotItem = infoPtr->iHotItem;
    HDC hdc = GetDC (infoPtr->hwndSelf);

    infoPtr->iHotItem = -1;
    if (oldHotItem != -1) HEADER_RefreshItem (infoPtr, oldHotItem);
    ReleaseDC (infoPtr->hwndSelf, hdc);

    return 0;
}


static LRESULT
HEADER_MouseMove (HEADER_INFO *infoPtr, LPARAM lParam)
{
    POINT pt;
    UINT  flags;
    INT   nItem, nWidth;
    HDC   hdc;
    /* With theming, hottracking is always enabled */
    BOOL  hotTrackEnabled =
        ((infoPtr->dwStyle & HDS_BUTTONS) && (infoPtr->dwStyle & HDS_HOTTRACK))
        || (GetWindowTheme (infoPtr->hwndSelf) != NULL);
    INT oldHotItem = infoPtr->iHotItem;

    pt.x = (INT)(SHORT)LOWORD(lParam);
    pt.y = (INT)(SHORT)HIWORD(lParam);
    HEADER_InternalHitTest (infoPtr, &pt, &flags, &nItem);

    if (hotTrackEnabled) {
	if (flags & (HHT_ONHEADER | HHT_ONDIVIDER | HHT_ONDIVOPEN))
	    infoPtr->iHotItem = nItem;
	else
	    infoPtr->iHotItem = -1;
    }

    if (infoPtr->bCaptured) {
        /* check if we should drag the header */
	if (infoPtr->bPressed && !infoPtr->bDragging && (infoPtr->dwStyle & HDS_DRAGDROP)
	    && HEADER_IsDragDistance(infoPtr, &pt))
	{
            if (!HEADER_SendNotifyWithHDItemT(infoPtr, HDN_BEGINDRAG, infoPtr->iMoveItem, NULL))
	    {
		HIMAGELIST hDragItem = HEADER_CreateDragImage(infoPtr, infoPtr->iMoveItem);
		if (hDragItem != NULL)
		{
		    HEADER_ITEM *lpItem = &infoPtr->items[infoPtr->iMoveItem];
		    TRACE("Starting item drag\n");
		    ImageList_BeginDrag(hDragItem, 0, pt.x - lpItem->rect.left, 0);
		    ImageList_DragShowNolock(TRUE);
		    ImageList_Destroy(hDragItem);
		    infoPtr->bDragging = TRUE;
		}
	    }
	}
	
	if (infoPtr->bDragging)
	{
	    POINT drag;
	    drag.x = pt.x;
	    drag.y = 0;
	    ClientToScreen(infoPtr->hwndSelf, &drag);
	    ImageList_DragMove(drag.x, drag.y);
	    HEADER_SetHotDivider(infoPtr, TRUE, lParam);
	}
	
	if (infoPtr->bPressed && !infoPtr->bDragging) {
            BOOL oldState = infoPtr->items[infoPtr->iMoveItem].bDown;
	    if ((nItem == infoPtr->iMoveItem) && (flags == HHT_ONHEADER))
		infoPtr->items[infoPtr->iMoveItem].bDown = TRUE;
	    else
		infoPtr->items[infoPtr->iMoveItem].bDown = FALSE;
            if (oldState != infoPtr->items[infoPtr->iMoveItem].bDown) {
                hdc = GetDC (infoPtr->hwndSelf);
	        HEADER_RefreshItem (infoPtr, infoPtr->iMoveItem);
	        ReleaseDC (infoPtr->hwndSelf, hdc);
            }

	    TRACE("Moving pressed item %d.\n", infoPtr->iMoveItem);
	}
	else if (infoPtr->bTracking) {
	    if (infoPtr->dwStyle & HDS_FULLDRAG) {
                HEADER_ITEM *lpItem = &infoPtr->items[infoPtr->iMoveItem];
                nWidth = pt.x - lpItem->rect.left + infoPtr->xTrackOffset;
                if (!HEADER_SendNotifyWithIntFieldT(infoPtr, HDN_ITEMCHANGINGW, infoPtr->iMoveItem, HDI_WIDTH, nWidth))
		{
                    INT nOldWidth = lpItem->rect.right - lpItem->rect.left;
                    RECT rcClient;
                    RECT rcScroll;
                    
                    if (nWidth < 0) nWidth = 0;
                    infoPtr->items[infoPtr->iMoveItem].cxy = nWidth;
                    HEADER_SetItemBounds(infoPtr);
                    
                    GetClientRect(infoPtr->hwndSelf, &rcClient);
                    rcScroll = rcClient;
                    rcScroll.left = lpItem->rect.left + nOldWidth;
                    ScrollWindowEx(infoPtr->hwndSelf, nWidth - nOldWidth, 0, &rcScroll, &rcClient, NULL, NULL, 0);
                    InvalidateRect(infoPtr->hwndSelf, &lpItem->rect, FALSE);
                    UpdateWindow(infoPtr->hwndSelf);
                    
                    HEADER_SendNotifyWithIntFieldT(infoPtr, HDN_ITEMCHANGEDW, infoPtr->iMoveItem, HDI_WIDTH, nWidth);
		}
	    }
	    else {
                INT iTrackWidth;
		hdc = GetDC (infoPtr->hwndSelf);
		HEADER_DrawTrackLine (infoPtr, hdc, infoPtr->xOldTrack);
		infoPtr->xOldTrack = pt.x + infoPtr->xTrackOffset;
		if (infoPtr->xOldTrack < infoPtr->items[infoPtr->iMoveItem].rect.left)
		    infoPtr->xOldTrack = infoPtr->items[infoPtr->iMoveItem].rect.left;
		HEADER_DrawTrackLine (infoPtr, hdc, infoPtr->xOldTrack);
		ReleaseDC (infoPtr->hwndSelf, hdc);
                iTrackWidth = infoPtr->xOldTrack - infoPtr->items[infoPtr->iMoveItem].rect.left;
                /* FIXME: should stop tracking if HDN_TRACK returns TRUE */
                HEADER_SendNotifyWithIntFieldT(infoPtr, HDN_TRACKW, infoPtr->iMoveItem, HDI_WIDTH, iTrackWidth);
	    }

	    TRACE("Tracking item %d.\n", infoPtr->iMoveItem);
	}
    }

    if (hotTrackEnabled) {
        TRACKMOUSEEVENT tme;
        if (oldHotItem != infoPtr->iHotItem && !infoPtr->bDragging) {
	    hdc = GetDC (infoPtr->hwndSelf);
	    if (oldHotItem != -1) HEADER_RefreshItem (infoPtr, oldHotItem);
	    if (infoPtr->iHotItem != -1) HEADER_RefreshItem (infoPtr, infoPtr->iHotItem);
	    ReleaseDC (infoPtr->hwndSelf, hdc);
        }
        tme.cbSize = sizeof( tme );
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = infoPtr->hwndSelf;
        TrackMouseEvent( &tme );
    }

    return 0;
}


static LRESULT
HEADER_Paint (HEADER_INFO *infoPtr, HDC hdcParam)
{
    HDC hdc;
    PAINTSTRUCT ps;

    hdc = hdcParam==0 ? BeginPaint (infoPtr->hwndSelf, &ps) : hdcParam;
    HEADER_Refresh (infoPtr, hdc);
    if(!hdcParam)
	EndPaint (infoPtr->hwndSelf, &ps);
    return 0;
}


static LRESULT
HEADER_RButtonUp (HEADER_INFO *infoPtr, INT x, INT y)
{
    BOOL bRet;
    POINT pt;

    pt.x = x;
    pt.y = y;

    /* Send a Notify message */
    bRet = HEADER_SendSimpleNotify (infoPtr, NM_RCLICK);

    /* Change to screen coordinate for WM_CONTEXTMENU */
    ClientToScreen(infoPtr->hwndSelf, &pt);

    /* Send a WM_CONTEXTMENU message in response to the RBUTTONUP */
    SendMessageW( infoPtr->hwndSelf, WM_CONTEXTMENU, (WPARAM) infoPtr->hwndSelf, MAKELPARAM(pt.x, pt.y));

    return bRet;
}


static LRESULT
HEADER_SetCursor (HEADER_INFO *infoPtr, LPARAM lParam)
{
    POINT pt;
    UINT  flags;
    INT   nItem;

    TRACE("code=0x%X  id=0x%X\n", LOWORD(lParam), HIWORD(lParam));

    GetCursorPos (&pt);
    ScreenToClient (infoPtr->hwndSelf, &pt);

    HEADER_InternalHitTest (infoPtr, &pt, &flags, &nItem);

    if (flags == HHT_ONDIVIDER)
        SetCursor (infoPtr->hcurDivider);
    else if (flags == HHT_ONDIVOPEN)
        SetCursor (infoPtr->hcurDivopen);
    else
        SetCursor (infoPtr->hcurArrow);

    return 0;
}


static LRESULT
HEADER_SetFont (HEADER_INFO *infoPtr, HFONT hFont, WORD Redraw)
{
    TEXTMETRICW tm;
    HFONT hOldFont;
    HDC hdc;

    infoPtr->hFont = hFont;

    hdc = GetDC (0);
    hOldFont = SelectObject (hdc, infoPtr->hFont ? infoPtr->hFont : GetStockObject (SYSTEM_FONT));
    GetTextMetricsW (hdc, &tm);
    infoPtr->nHeight = tm.tmHeight + VERT_BORDER;
    SelectObject (hdc, hOldFont);
    ReleaseDC (0, hdc);

    infoPtr->bRectsValid = FALSE;

    if (Redraw) {
        InvalidateRect(infoPtr->hwndSelf, NULL, FALSE);
    }

    return 0;
}

static LRESULT HEADER_SetRedraw(HEADER_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    /* ignoring the InvalidateRect calls is handled by user32. But some apps expect
     * that we invalidate the header and this has to be done manually  */
    LRESULT ret;

    ret = DefWindowProcW(infoPtr->hwndSelf, WM_SETREDRAW, wParam, lParam);
    if (wParam)
        InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);
    return ret;
}

static INT HEADER_StyleChanged(HEADER_INFO *infoPtr, WPARAM wStyleType,
                               const STYLESTRUCT *lpss)
{
    TRACE("styletype %Ix, styleOld %#lx, styleNew %#lx\n", wStyleType, lpss->styleOld, lpss->styleNew);

    if (wStyleType != GWL_STYLE) return 0;

    infoPtr->dwStyle = lpss->styleNew;

    return 0;
}

/* Update the theme handle after a theme change */
static LRESULT HEADER_ThemeChanged(const HEADER_INFO *infoPtr)
{
    HTHEME theme = GetWindowTheme(infoPtr->hwndSelf);
    CloseThemeData(theme);
    OpenThemeData(infoPtr->hwndSelf, themeClass);
    InvalidateRect(infoPtr->hwndSelf, NULL, TRUE);
    return 0;
}

static INT HEADER_SetFilterChangeTimeout(HEADER_INFO *infoPtr, INT timeout)
{
    INT old_timeout = infoPtr->filter_change_timeout;

    if (timeout != 0)
        infoPtr->filter_change_timeout = timeout;
    return old_timeout;
}

static LRESULT WINAPI
HEADER_WindowProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HEADER_INFO *infoPtr = (HEADER_INFO *)GetWindowLongPtrW(hwnd, 0);

    TRACE("hwnd %p, msg %x, wparam %Ix, lParam %Ix\n", hwnd, msg, wParam, lParam);

    if (!infoPtr && (msg != WM_CREATE))
	return DefWindowProcW (hwnd, msg, wParam, lParam);
    switch (msg) {
/*	case HDM_CLEARFILTER: */

	case HDM_CREATEDRAGIMAGE:
	    return (LRESULT)HEADER_CreateDragImage (infoPtr, (INT)wParam);

	case HDM_DELETEITEM:
	    return HEADER_DeleteItem (infoPtr, (INT)wParam);

/*	case HDM_EDITFILTER: */

	case HDM_GETBITMAPMARGIN:
	    return HEADER_GetBitmapMargin(infoPtr);

	case HDM_GETIMAGELIST:
	    return HEADER_GetImageList (infoPtr);

	case HDM_GETITEMA:
	case HDM_GETITEMW:
	    return HEADER_GetItemT (infoPtr, (INT)wParam, (LPHDITEMW)lParam, msg == HDM_GETITEMW);

	case HDM_GETITEMCOUNT:
	    return HEADER_GetItemCount (infoPtr);

	case HDM_GETITEMRECT:
	    return HEADER_GetItemRect (infoPtr, (INT)wParam, (LPRECT)lParam);

	case HDM_GETORDERARRAY:
	    return HEADER_GetOrderArray(infoPtr, (INT)wParam, (LPINT)lParam);

	case HDM_GETUNICODEFORMAT:
	    return HEADER_GetUnicodeFormat (infoPtr);

	case HDM_HITTEST:
	    return HEADER_HitTest (infoPtr, (LPHDHITTESTINFO)lParam);

	case HDM_INSERTITEMA:
	case HDM_INSERTITEMW:
	    return HEADER_InsertItemT (infoPtr, (INT)wParam, (LPHDITEMW)lParam, msg == HDM_INSERTITEMW);

	case HDM_LAYOUT:
	    return HEADER_Layout (infoPtr, (LPHDLAYOUT)lParam);

	case HDM_ORDERTOINDEX:
	    return HEADER_OrderToIndex(infoPtr, (INT)wParam);

	case HDM_SETBITMAPMARGIN:
	    return HEADER_SetBitmapMargin(infoPtr, (INT)wParam);

        case HDM_SETFILTERCHANGETIMEOUT:
            return HEADER_SetFilterChangeTimeout(infoPtr, (INT)lParam);

        case HDM_SETHOTDIVIDER:
            return HEADER_SetHotDivider(infoPtr, wParam, lParam);

	case HDM_SETIMAGELIST:
	    return HEADER_SetImageList (infoPtr, (HIMAGELIST)lParam);

	case HDM_SETITEMA:
	case HDM_SETITEMW:
	    return HEADER_SetItemT (infoPtr, (INT)wParam, (LPHDITEMW)lParam, msg == HDM_SETITEMW);

	case HDM_SETORDERARRAY:
	    return HEADER_SetOrderArray(infoPtr, (INT)wParam, (LPINT)lParam);

	case HDM_SETUNICODEFORMAT:
	    return HEADER_SetUnicodeFormat (infoPtr, wParam);

        case WM_CREATE:
            return HEADER_Create (hwnd, (LPCREATESTRUCTW)lParam);

        case WM_DESTROY:
            return HEADER_Destroy (infoPtr);

        case WM_NCDESTROY:
            return HEADER_NCDestroy (infoPtr);

        case WM_ERASEBKGND:
            return 1;

        case WM_GETDLGCODE:
            return DLGC_WANTTAB | DLGC_WANTARROWS;

        case WM_GETFONT:
            return HEADER_GetFont (infoPtr);

        case WM_LBUTTONDBLCLK:
            return HEADER_LButtonDblClk (infoPtr, (SHORT)LOWORD(lParam), (SHORT)HIWORD(lParam));

        case WM_LBUTTONDOWN:
            return HEADER_LButtonDown (infoPtr, (SHORT)LOWORD(lParam), (SHORT)HIWORD(lParam));

        case WM_LBUTTONUP:
            return HEADER_LButtonUp (infoPtr, (SHORT)LOWORD(lParam), (SHORT)HIWORD(lParam));

        case WM_MOUSELEAVE:
            return HEADER_MouseLeave (infoPtr);

        case WM_MOUSEMOVE:
            return HEADER_MouseMove (infoPtr, lParam);

	case WM_NOTIFYFORMAT:
            return HEADER_NotifyFormat (infoPtr, wParam, lParam);

	case WM_SIZE:
	    return HEADER_Size (infoPtr);

        case WM_THEMECHANGED:
            return HEADER_ThemeChanged (infoPtr);

        case WM_PRINTCLIENT:
        case WM_PAINT:
            return HEADER_Paint (infoPtr, (HDC)wParam);

        case WM_RBUTTONUP:
            return HEADER_RButtonUp (infoPtr, (SHORT)LOWORD(lParam), (SHORT)HIWORD(lParam));

        case WM_SETCURSOR:
            return HEADER_SetCursor (infoPtr, lParam);

        case WM_SETFONT:
            return HEADER_SetFont (infoPtr, (HFONT)wParam, (WORD)lParam);

        case WM_SETREDRAW:
            return HEADER_SetRedraw(infoPtr, wParam, lParam);

        case WM_STYLECHANGED:
            return HEADER_StyleChanged(infoPtr, wParam, (LPSTYLESTRUCT)lParam);

        case WM_SYSCOLORCHANGE:
            COMCTL32_RefreshSysColors();
            return 0;

        default:
            if ((msg >= WM_USER) && (msg < WM_APP) && !COMCTL32_IsReflectedMessage(msg))
                ERR("unknown msg %04x, wp %#Ix, lp %#Ix\n", msg, wParam, lParam );
	    return DefWindowProcW(hwnd, msg, wParam, lParam);
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
