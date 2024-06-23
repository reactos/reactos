/* Treeview control
 *
 * Copyright 1998 Eric Kohl <ekohl@abo.rhein-zeitung.de>
 * Copyright 1998,1999 Alex Priem <alexp@sci.kun.nl>
 * Copyright 1999 Sylvain St-Germain
 * Copyright 2002 CodeWeavers, Aric Stewart
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
 *
 * Note that TREEVIEW_INFO * and HTREEITEM are the same thing.
 *
 * Note2: If item's text == LPSTR_TEXTCALLBACKA we allocate buffer
 *      of size TEXT_CALLBACK_SIZE in DoSetItem.
 *      We use callbackMask to keep track of fields to be updated.
 *
 * TODO:
 *   missing notifications: TVN_GETINFOTIP, TVN_KEYDOWN,
 *      TVN_SETDISPINFO
 *
 *   missing styles: TVS_INFOTIP, TVS_RTLREADING,
 *
 *   missing item styles: TVIS_EXPANDPARTIAL, TVIS_EX_FLAT,
 *      TVIS_EX_DISABLED
 *
 *   Make the insertion mark look right.
 *   Scroll (instead of repaint) as much as possible.
 */

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>

#define NONAMELESSUNION

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
#include "wine/exception.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(treeview);

/* internal structures */
typedef struct tagTREEVIEW_INFO
{
  HWND          hwnd;
  HWND          hwndNotify;     /* Owner window to send notifications to */
  DWORD         dwStyle;
  HTREEITEM     root;
  UINT          uInternalStatus;
  INT           Timer;
  UINT          uNumItems;      /* number of valid TREEVIEW_ITEMs */
  INT           cdmode;         /* last custom draw setting */
  UINT          uScrollTime;	/* max. time for scrolling in milliseconds */
  BOOL          bRedraw;        /* if FALSE we validate but don't redraw in TREEVIEW_Paint() */

  UINT          uItemHeight;    /* item height */
  BOOL          bHeightSet;

  LONG          clientWidth;    /* width of control window */
  LONG          clientHeight;   /* height of control window */

  LONG          treeWidth;      /* width of visible tree items */
  LONG          treeHeight;     /* height of visible tree items */

  UINT          uIndent;        /* indentation in pixels */
  HTREEITEM     selectedItem;   /* handle to selected item or 0 if none */
  HTREEITEM     hotItem;        /* handle currently under cursor, 0 if none */
  HTREEITEM	focusedItem;    /* item that was under the cursor when WM_LBUTTONDOWN was received */
  HTREEITEM     editItem;       /* item being edited with builtin edit box */

  HTREEITEM     firstVisible;   /* handle to item whose top edge is at y = 0 */
  LONG          maxVisibleOrder;
  HTREEITEM     dropItem;       /* handle to item selected by drag cursor */
  HTREEITEM     insertMarkItem; /* item after which insertion mark is placed */
  BOOL          insertBeforeorAfter; /* flag used by TVM_SETINSERTMARK */
  HIMAGELIST    dragList;       /* Bitmap of dragged item */
  LONG          scrollX;
  INT           wheelRemainder;
  COLORREF      clrBk;
  COLORREF      clrText;
  COLORREF      clrLine;
  COLORREF      clrInsertMark;
  HFONT         hFont;
  HFONT         hDefaultFont;
  HFONT         hBoldFont;
  HFONT         hUnderlineFont;
  HFONT         hBoldUnderlineFont;
  HCURSOR       hcurHand;
  HWND          hwndToolTip;

  HWND          hwndEdit;
  WNDPROC       wpEditOrig;     /* orig window proc for subclassing edit */
  BOOL          bIgnoreEditKillFocus;
  BOOL          bLabelChanged;

  BOOL          bNtfUnicode;    /* TRUE if should send NOTIFY with W */
  HIMAGELIST    himlNormal;
  int           normalImageHeight;
  int           normalImageWidth;
  HIMAGELIST    himlState;
  int           stateImageHeight;
  int           stateImageWidth;
  HDPA          items;

  DWORD lastKeyPressTimestamp;
  WPARAM charCode;
  INT nSearchParamLength;
  WCHAR szSearchParam[ MAX_PATH ];
} TREEVIEW_INFO;

typedef struct _TREEITEM    /* HTREEITEM is a _TREEINFO *. */
{
  HTREEITEM parent;         /* handle to parent or 0 if at root */
  HTREEITEM nextSibling;    /* handle to next item in list, 0 if last */
  HTREEITEM firstChild;     /* handle to first child or 0 if no child */

  UINT      callbackMask;
  UINT      state;
  UINT      stateMask;
  LPWSTR    pszText;
  int       cchTextMax;
  int       iImage;
  int       iSelectedImage;
  int       iExpandedImage;
  int       cChildren;
  LPARAM    lParam;
  int       iIntegral;      /* item height multiplier (1 is normal) */
  int       iLevel;         /* indentation level:0=root level */
  HTREEITEM lastChild;
  HTREEITEM prevSibling;    /* handle to prev item in list, 0 if first */
  RECT      rect;
  LONG      linesOffset;
  LONG      stateOffset;
  LONG      imageOffset;
  LONG      textOffset;
  LONG      textWidth;      /* horizontal text extent for pszText */
  LONG      visibleOrder;   /* Depth-first numbering of the items whose ancestors are all expanded,
                               corresponding to a top-to-bottom ordering in the tree view.
                               Each item takes up "item.iIntegral" spots in the visible order.
                               0 is the root's first child. */
  const TREEVIEW_INFO *infoPtr; /* tree data this item belongs to */
} TREEVIEW_ITEM;

/******** Defines that TREEVIEW_ProcessLetterKeys uses ****************/
#define KEY_DELAY       450

/* bitflags for infoPtr->uInternalStatus */

#define TV_HSCROLL 	0x01    /* treeview too large to fit in window */
#define TV_VSCROLL 	0x02	/* (horizontal/vertical) */
#define TV_LDRAG		0x04	/* Lbutton pushed to start drag */
#define TV_LDRAGGING	0x08	/* Lbutton pushed, mouse moved. */
#define TV_RDRAG		0x10	/* ditto Rbutton */
#define TV_RDRAGGING	0x20

/* bitflags for infoPtr->timer */

#define TV_EDIT_TIMER    2
#define TV_EDIT_TIMER_SET 2

#define TEXT_CALLBACK_SIZE 260

#define TREEVIEW_LEFT_MARGIN 8

#define MINIMUM_INDENT 19

#define CALLBACK_MASK_ALL (TVIF_TEXT|TVIF_CHILDREN|TVIF_IMAGE|TVIF_SELECTEDIMAGE)

#define STATEIMAGEINDEX(x) (((x) >> 12) & 0x0f)
#define OVERLAYIMAGEINDEX(x) (((x) >> 8) & 0x0f)
#define ISVISIBLE(x)         ((x)->visibleOrder >= 0)

#define GETLINECOLOR(x) ((x) == CLR_DEFAULT ? comctl32_color.clrGrayText   : (x))
#define GETBKCOLOR(x)   ((x) == CLR_NONE    ? comctl32_color.clrWindow     : (x))
#define GETTXTCOLOR(x)  ((x) == CLR_NONE    ? comctl32_color.clrWindowText : (x))
#define GETINSCOLOR(x)  ((x) == CLR_DEFAULT ? comctl32_color.clrBtnText    : (x))

static const WCHAR themeClass[] = { 'T','r','e','e','v','i','e','w',0 };


typedef VOID (*TREEVIEW_ItemEnumFunc)(TREEVIEW_INFO *, TREEVIEW_ITEM *,LPVOID);


static VOID TREEVIEW_Invalidate(const TREEVIEW_INFO *, const TREEVIEW_ITEM *);

static LRESULT TREEVIEW_DoSelectItem(TREEVIEW_INFO *, INT, HTREEITEM, INT);
static VOID TREEVIEW_SetFirstVisible(TREEVIEW_INFO *, TREEVIEW_ITEM *, BOOL);
static LRESULT TREEVIEW_EnsureVisible(TREEVIEW_INFO *, HTREEITEM, BOOL);
static LRESULT TREEVIEW_EndEditLabelNow(TREEVIEW_INFO *infoPtr, BOOL bCancel);
static VOID TREEVIEW_UpdateScrollBars(TREEVIEW_INFO *infoPtr);
static LRESULT TREEVIEW_HScroll(TREEVIEW_INFO *, WPARAM);

/* Random Utilities *****************************************************/
static void TREEVIEW_VerifyTree(TREEVIEW_INFO *infoPtr);

/* Returns the treeview private data if hwnd is a treeview.
 * Otherwise returns an undefined value. */
static inline TREEVIEW_INFO *
TREEVIEW_GetInfoPtr(HWND hwnd)
{
    return (TREEVIEW_INFO *)GetWindowLongPtrW(hwnd, 0);
}

/* Don't call this. Nothing wants an item index. */
static inline int
TREEVIEW_GetItemIndex(const TREEVIEW_INFO *infoPtr, HTREEITEM handle)
{
    return DPA_GetPtrIndex(infoPtr->items, handle);
}

/* Checks if item has changed and needs to be redrawn */
static inline BOOL item_changed (const TREEVIEW_ITEM *tiOld, const TREEVIEW_ITEM *tiNew,
                                 const TVITEMEXW *tvChange)
{
    /* Number of children has changed */
    if ((tvChange->mask & TVIF_CHILDREN) && (tiOld->cChildren != tiNew->cChildren))
	return TRUE;

    /* Image has changed and it's not a callback */
    if ((tvChange->mask & TVIF_IMAGE) && (tiOld->iImage != tiNew->iImage) &&
	tiNew->iImage != I_IMAGECALLBACK)
	return TRUE;

    /* Selected image has changed and it's not a callback */
    if ((tvChange->mask & TVIF_SELECTEDIMAGE) && (tiOld->iSelectedImage != tiNew->iSelectedImage) &&
	tiNew->iSelectedImage != I_IMAGECALLBACK)
	return TRUE;

    if ((tvChange->mask & TVIF_EXPANDEDIMAGE) && (tiOld->iExpandedImage != tiNew->iExpandedImage) &&
	tiNew->iExpandedImage != I_IMAGECALLBACK)
	return TRUE;

    /* Text has changed and it's not a callback */
    if ((tvChange->mask & TVIF_TEXT) && (tiOld->pszText != tiNew->pszText) &&
	tiNew->pszText != LPSTR_TEXTCALLBACKW)
	return TRUE;

    /* Indent has changed */
    if ((tvChange->mask & TVIF_INTEGRAL) && (tiOld->iIntegral != tiNew->iIntegral))
	return TRUE;

    /* Item state has changed */
    if ((tvChange->mask & TVIF_STATE) && ((tiOld->state ^ tiNew->state) & tvChange->stateMask ))
	return TRUE;

    return FALSE;
}

/***************************************************************************
 * This method checks that handle is an item for this tree.
 */
static BOOL
TREEVIEW_ValidItem(const TREEVIEW_INFO *infoPtr, HTREEITEM handle)
{
    if (TREEVIEW_GetItemIndex(infoPtr, handle) == -1)
    {
	TRACE("invalid item %p\n", handle);
	return FALSE;
    }
    else
	return TRUE;
}

static HFONT
TREEVIEW_CreateBoldFont(HFONT hOrigFont)
{
    LOGFONTW font;

    GetObjectW(hOrigFont, sizeof(font), &font);
    font.lfWeight = FW_BOLD;
    return CreateFontIndirectW(&font);
}

static HFONT
TREEVIEW_CreateUnderlineFont(HFONT hOrigFont)
{
    LOGFONTW font;

    GetObjectW(hOrigFont, sizeof(font), &font);
    font.lfUnderline = TRUE;
    return CreateFontIndirectW(&font);
}

static HFONT
TREEVIEW_CreateBoldUnderlineFont(HFONT hfont)
{
    LOGFONTW font;

    GetObjectW(hfont, sizeof(font), &font);
    font.lfWeight = FW_BOLD;
    font.lfUnderline = TRUE;
    return CreateFontIndirectW(&font);
}

static inline HFONT
TREEVIEW_FontForItem(const TREEVIEW_INFO *infoPtr, const TREEVIEW_ITEM *item)
{
    if ((infoPtr->dwStyle & TVS_TRACKSELECT) && (item == infoPtr->hotItem))
        return item->state & TVIS_BOLD ? infoPtr->hBoldUnderlineFont : infoPtr->hUnderlineFont;
    if (item->state & TVIS_BOLD)
        return infoPtr->hBoldFont;
    return infoPtr->hFont;
}

/* for trace/debugging purposes only */
static const char *
TREEVIEW_ItemName(const TREEVIEW_ITEM *item)
{
    if (item == NULL) return "<null item>";
    if (item->pszText == LPSTR_TEXTCALLBACKW) return "<callback>";
    if (item->pszText == NULL) return "<null>";
    return debugstr_w(item->pszText);
}

/* An item is not a child of itself. */
static BOOL
TREEVIEW_IsChildOf(const TREEVIEW_ITEM *parent, const TREEVIEW_ITEM *child)
{
    do
    {
	child = child->parent;
	if (child == parent) return TRUE;
    } while (child != NULL);

    return FALSE;
}

static BOOL
TREEVIEW_IsFullRowSelect(const TREEVIEW_INFO *infoPtr)
{
    return !(infoPtr->dwStyle & TVS_HASLINES) && (infoPtr->dwStyle & TVS_FULLROWSELECT);
}

static BOOL
TREEVIEW_IsItemHit(const TREEVIEW_INFO *infoPtr, const TVHITTESTINFO *ht)
{
    if (TREEVIEW_IsFullRowSelect(infoPtr))
        return ht->flags & (TVHT_ONITEMINDENT | TVHT_ONITEMBUTTON | TVHT_ONITEM | TVHT_ONITEMRIGHT);
    else
        return ht->flags & TVHT_ONITEM;
}

/* Tree Traversal *******************************************************/

/***************************************************************************
 * This method returns the last expanded sibling or child child item
 * of a tree node
 */
static TREEVIEW_ITEM *
TREEVIEW_GetLastListItem(const TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *item)
{
    if (!item) return NULL;

    while (item->lastChild)
    {
       if (item->state & TVIS_EXPANDED)
          item = item->lastChild;
       else
          break;
    }

    if (item == infoPtr->root)
        return NULL;

    return item;
}

/***************************************************************************
 * This method returns the previous non-hidden item in the list not
 * considering the tree hierarchy.
 */
static TREEVIEW_ITEM *
TREEVIEW_GetPrevListItem(const TREEVIEW_INFO *infoPtr, const TREEVIEW_ITEM *tvItem)
{
    if (tvItem->prevSibling)
    {
	/* This item has a prevSibling, get the last item in the sibling's tree. */
	TREEVIEW_ITEM *upItem = tvItem->prevSibling;

	if ((upItem->state & TVIS_EXPANDED) && upItem->lastChild != NULL)
	    return TREEVIEW_GetLastListItem(infoPtr, upItem->lastChild);
	else
	    return upItem;
    }
    else
    {
	/* this item does not have a prevSibling, get the parent */
	return (tvItem->parent != infoPtr->root) ? tvItem->parent : NULL;
    }
}


/***************************************************************************
 * This method returns the next physical item in the treeview not
 * considering the tree hierarchy.
 */
static TREEVIEW_ITEM *
TREEVIEW_GetNextListItem(const TREEVIEW_INFO *infoPtr, const TREEVIEW_ITEM *tvItem)
{
    /*
     * If this item has children and is expanded, return the first child
     */
    if ((tvItem->state & TVIS_EXPANDED) && tvItem->firstChild != NULL)
    {
	return tvItem->firstChild;
    }


    /*
     * try to get the sibling
     */
    if (tvItem->nextSibling)
	return tvItem->nextSibling;

    /*
     * Otherwise, get the parent's sibling.
     */
    while (tvItem->parent)
    {
	tvItem = tvItem->parent;

	if (tvItem->nextSibling)
	    return tvItem->nextSibling;
    }

    return NULL;
}

/***************************************************************************
 * This method returns the nth item starting at the given item.  It returns
 * the last item (or first) we we run out of items.
 *
 * Will scroll backward if count is <0.
 *             forward if count is >0.
 */
static TREEVIEW_ITEM *
TREEVIEW_GetListItem(const TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *item,
		     LONG count)
{
    TREEVIEW_ITEM *(*next_item)(const TREEVIEW_INFO *, const TREEVIEW_ITEM *);
    TREEVIEW_ITEM *previousItem;

    assert(item != NULL);

    if (count > 0)
    {
	next_item = TREEVIEW_GetNextListItem;
    }
    else if (count < 0)
    {
	count = -count;
	next_item = TREEVIEW_GetPrevListItem;
    }
    else
	return item;

    do
    {
	previousItem = item;
	item = next_item(infoPtr, item);

    } while (--count && item != NULL);


    return item ? item : previousItem;
}

/* Notifications ************************************************************/

static INT get_notifycode(const TREEVIEW_INFO *infoPtr, INT code)
{
    if (!infoPtr->bNtfUnicode) {
	switch (code) {
	case TVN_SELCHANGINGW:	  return TVN_SELCHANGINGA;
	case TVN_SELCHANGEDW:	  return TVN_SELCHANGEDA;
	case TVN_GETDISPINFOW:	  return TVN_GETDISPINFOA;
	case TVN_SETDISPINFOW:	  return TVN_SETDISPINFOA;
	case TVN_ITEMEXPANDINGW:  return TVN_ITEMEXPANDINGA;
	case TVN_ITEMEXPANDEDW:	  return TVN_ITEMEXPANDEDA;
	case TVN_BEGINDRAGW:	  return TVN_BEGINDRAGA;
	case TVN_BEGINRDRAGW:	  return TVN_BEGINRDRAGA;
	case TVN_DELETEITEMW:	  return TVN_DELETEITEMA;
	case TVN_BEGINLABELEDITW: return TVN_BEGINLABELEDITA;
	case TVN_ENDLABELEDITW:	  return TVN_ENDLABELEDITA;
	case TVN_GETINFOTIPW:	  return TVN_GETINFOTIPA;
	}
    }
    return code;
}

static inline BOOL
TREEVIEW_SendRealNotify(const TREEVIEW_INFO *infoPtr, UINT code, NMHDR *hdr)
{
    TRACE("code=%d, hdr=%p\n", code, hdr);

    hdr->hwndFrom = infoPtr->hwnd;
    hdr->idFrom = GetWindowLongPtrW(infoPtr->hwnd, GWLP_ID);
    hdr->code = get_notifycode(infoPtr, code);

    return SendMessageW(infoPtr->hwndNotify, WM_NOTIFY, hdr->idFrom, (LPARAM)hdr);
}

static BOOL
TREEVIEW_SendSimpleNotify(const TREEVIEW_INFO *infoPtr, UINT code)
{
    NMHDR hdr;
    return TREEVIEW_SendRealNotify(infoPtr, code, &hdr);
}

static VOID
TREEVIEW_TVItemFromItem(const TREEVIEW_INFO *infoPtr, UINT mask, TVITEMW *tvItem, TREEVIEW_ITEM *item)
{
    tvItem->mask = mask;
    tvItem->hItem = item;
    tvItem->state = item->state;
    tvItem->stateMask = 0;
    tvItem->iImage = item->iImage;
    tvItem->iSelectedImage = item->iSelectedImage;
    tvItem->cChildren = item->cChildren;
    tvItem->lParam = item->lParam;

    if(mask & TVIF_TEXT)
    {
        if (!infoPtr->bNtfUnicode)
        {
            tvItem->cchTextMax = WideCharToMultiByte( CP_ACP, 0, item->pszText, -1, NULL, 0, NULL, NULL );
            tvItem->pszText = heap_alloc (tvItem->cchTextMax);
            WideCharToMultiByte( CP_ACP, 0, item->pszText, -1, (LPSTR)tvItem->pszText, tvItem->cchTextMax, 0, 0 );
	}
        else
        {
            tvItem->cchTextMax = item->cchTextMax;
            tvItem->pszText = item->pszText;
        }
    }
    else
    {
        tvItem->cchTextMax = 0;
        tvItem->pszText = NULL;
    }
}

static BOOL
TREEVIEW_SendTreeviewNotify(const TREEVIEW_INFO *infoPtr, UINT code, UINT action,
			    UINT mask, HTREEITEM oldItem, HTREEITEM newItem)
{
    NMTREEVIEWW nmhdr;
    BOOL ret;

    TRACE("code:%d action:0x%x olditem:%p newitem:%p\n",
	  code, action, oldItem, newItem);

    memset(&nmhdr, 0, sizeof(NMTREEVIEWW));
    nmhdr.action = action;

    if (oldItem)
	TREEVIEW_TVItemFromItem(infoPtr, mask, &nmhdr.itemOld, oldItem);

    if (newItem)
	TREEVIEW_TVItemFromItem(infoPtr, mask, &nmhdr.itemNew, newItem);

    nmhdr.ptDrag.x = 0;
    nmhdr.ptDrag.y = 0;

    ret = TREEVIEW_SendRealNotify(infoPtr, code, &nmhdr.hdr);
    if (!infoPtr->bNtfUnicode)
    {
        heap_free(nmhdr.itemOld.pszText);
        heap_free(nmhdr.itemNew.pszText);
    }
    return ret;
}

static BOOL
TREEVIEW_SendTreeviewDnDNotify(const TREEVIEW_INFO *infoPtr, UINT code,
			       HTREEITEM dragItem, POINT pt)
{
    NMTREEVIEWW nmhdr;

    TRACE("code:%d dragitem:%p\n", code, dragItem);

    nmhdr.action = 0;
    nmhdr.itemNew.mask = TVIF_STATE | TVIF_PARAM | TVIF_HANDLE;
    nmhdr.itemNew.hItem = dragItem;
    nmhdr.itemNew.state = dragItem->state;
    nmhdr.itemNew.lParam = dragItem->lParam;

    nmhdr.ptDrag.x = pt.x;
    nmhdr.ptDrag.y = pt.y;

    return TREEVIEW_SendRealNotify(infoPtr, code, &nmhdr.hdr);
}


static BOOL
TREEVIEW_SendCustomDrawNotify(const TREEVIEW_INFO *infoPtr, DWORD dwDrawStage,
			      HDC hdc, RECT rc)
{
    NMTVCUSTOMDRAW nmcdhdr;
    NMCUSTOMDRAW *nmcd;

    TRACE("drawstage:0x%x hdc:%p\n", dwDrawStage, hdc);

    nmcd = &nmcdhdr.nmcd;
    nmcd->dwDrawStage = dwDrawStage;
    nmcd->hdc = hdc;
    nmcd->rc = rc;
    nmcd->dwItemSpec = 0;
    nmcd->uItemState = 0;
    nmcd->lItemlParam = 0;
    nmcdhdr.clrText = infoPtr->clrText;
    nmcdhdr.clrTextBk = infoPtr->clrBk;
    nmcdhdr.iLevel = 0;

    return TREEVIEW_SendRealNotify(infoPtr, NM_CUSTOMDRAW, &nmcdhdr.nmcd.hdr);
}

/* FIXME: need to find out when the flags in uItemState need to be set */

static BOOL
TREEVIEW_SendCustomDrawItemNotify(const TREEVIEW_INFO *infoPtr, HDC hdc,
				  TREEVIEW_ITEM *item, UINT uItemDrawState,
				  NMTVCUSTOMDRAW *nmcdhdr)
{
    NMCUSTOMDRAW *nmcd;
    DWORD dwDrawStage;
    DWORD_PTR dwItemSpec;
    UINT uItemState;

    dwDrawStage = CDDS_ITEM | uItemDrawState;
    dwItemSpec = (DWORD_PTR)item;
    uItemState = 0;
    if (item->state & TVIS_SELECTED)
	uItemState |= CDIS_SELECTED;
    if (item == infoPtr->selectedItem)
	uItemState |= CDIS_FOCUS;
    if (item == infoPtr->hotItem)
	uItemState |= CDIS_HOT;

    nmcd = &nmcdhdr->nmcd;
    nmcd->dwDrawStage = dwDrawStage;
    nmcd->hdc = hdc;
    nmcd->rc = item->rect;
    nmcd->dwItemSpec = dwItemSpec;
    nmcd->uItemState = uItemState;
    nmcd->lItemlParam = item->lParam;
    nmcdhdr->iLevel = item->iLevel;

    TRACE("drawstage:0x%x hdc:%p item:%lx, itemstate:0x%x, lItemlParam:0x%lx\n",
	  nmcd->dwDrawStage, nmcd->hdc, nmcd->dwItemSpec,
	  nmcd->uItemState, nmcd->lItemlParam);

    return TREEVIEW_SendRealNotify(infoPtr, NM_CUSTOMDRAW, &nmcdhdr->nmcd.hdr);
}

static BOOL
TREEVIEW_BeginLabelEditNotify(const TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *editItem)
{
    NMTVDISPINFOW tvdi;
    BOOL ret;

    TREEVIEW_TVItemFromItem(infoPtr, TVIF_HANDLE | TVIF_STATE | TVIF_PARAM | TVIF_TEXT,
                            &tvdi.item, editItem);

    ret = TREEVIEW_SendRealNotify(infoPtr, TVN_BEGINLABELEDITW, &tvdi.hdr);

    if (!infoPtr->bNtfUnicode)
        heap_free(tvdi.item.pszText);

    return ret;
}

static void
TREEVIEW_UpdateDispInfo(const TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *item,
			UINT mask)
{
    NMTVDISPINFOEXW callback;

    TRACE("mask=0x%x, callbackmask=0x%x\n", mask, item->callbackMask);
    mask &= item->callbackMask;

    if (mask == 0) return;

    /* 'state' always contains valid value, as well as 'lParam'.
     * All other parameters are uninitialized.
     */
    callback.item.pszText         = item->pszText;
    callback.item.cchTextMax      = item->cchTextMax;
    callback.item.mask            = mask;
    callback.item.hItem           = item;
    callback.item.state           = item->state;
    callback.item.lParam          = item->lParam;

    /* If text is changed we need to recalculate textWidth */
    if (mask & TVIF_TEXT)
       item->textWidth = 0;

    TREEVIEW_SendRealNotify(infoPtr, TVN_GETDISPINFOW, &callback.hdr);
    TRACE("resulting code 0x%08x\n", callback.hdr.code);

    /* It may have changed due to a call to SetItem. */
    mask &= item->callbackMask;

    if ((mask & TVIF_TEXT) && callback.item.pszText != item->pszText)
    {
	/* Instead of copying text into our buffer user specified his own */
	if (!infoPtr->bNtfUnicode && (callback.hdr.code == TVN_GETDISPINFOA)) {
	    LPWSTR newText;
	    int buflen;
            int len = MultiByteToWideChar( CP_ACP, 0,
					   (LPSTR)callback.item.pszText, -1,
                                           NULL, 0);
	    buflen = max((len)*sizeof(WCHAR), TEXT_CALLBACK_SIZE);
            newText = heap_realloc(item->pszText, buflen);

	    TRACE("returned str %s, len=%d, buflen=%d\n",
		  debugstr_a((LPSTR)callback.item.pszText), len, buflen);

	    if (newText)
	    {
		item->pszText = newText;
		MultiByteToWideChar( CP_ACP, 0,
				     (LPSTR)callback.item.pszText, -1,
				     item->pszText, buflen/sizeof(WCHAR));
		item->cchTextMax = buflen/sizeof(WCHAR);
	    }
	    /* If realloc fails we have nothing to do, but keep original text */
	}
	else {
	    int len = max(lstrlenW(callback.item.pszText) + 1,
			  TEXT_CALLBACK_SIZE);
	    LPWSTR newText = heap_realloc(item->pszText, len*sizeof(WCHAR));

	    TRACE("returned wstr %s, len=%d\n",
		  debugstr_w(callback.item.pszText), len);

	    if (newText)
	    {
		item->pszText = newText;
		lstrcpyW(item->pszText, callback.item.pszText);
		item->cchTextMax = len;
	    }
	    /* If realloc fails we have nothing to do, but keep original text */
	}
    }
    else if (mask & TVIF_TEXT) {
	/* User put text into our buffer, that is ok unless A string */
	if (!infoPtr->bNtfUnicode && (callback.hdr.code == TVN_GETDISPINFOA)) {
	    LPWSTR newText;
	    int buflen;
            int len = MultiByteToWideChar( CP_ACP, 0,
					  (LPSTR)callback.item.pszText, -1,
                                           NULL, 0);
	    buflen = max((len)*sizeof(WCHAR), TEXT_CALLBACK_SIZE);
            newText = heap_alloc(buflen);

	    TRACE("same buffer str %s, len=%d, buflen=%d\n",
		  debugstr_a((LPSTR)callback.item.pszText), len, buflen);

	    if (newText)
	    {
		LPWSTR oldText = item->pszText;
		item->pszText = newText;
		MultiByteToWideChar( CP_ACP, 0,
				     (LPSTR)callback.item.pszText, -1,
				     item->pszText, buflen/sizeof(WCHAR));
		item->cchTextMax = buflen/sizeof(WCHAR);
		heap_free(oldText);
	    }
	}
    }

    if (mask & TVIF_IMAGE)
	item->iImage = callback.item.iImage;

    if (mask & TVIF_SELECTEDIMAGE)
	item->iSelectedImage = callback.item.iSelectedImage;

    if (mask & TVIF_EXPANDEDIMAGE)
	item->iExpandedImage = callback.item.iExpandedImage;

    if (mask & TVIF_CHILDREN)
	item->cChildren = callback.item.cChildren;

    if (callback.item.mask & TVIF_STATE)
    {
        item->state &= ~callback.item.stateMask;
        item->state |= (callback.item.state & callback.item.stateMask);
    }

    /* These members are now permanently set. */
    if (callback.item.mask & TVIF_DI_SETITEM)
	item->callbackMask &= ~callback.item.mask;
}

/***************************************************************************
 * This function uses cChildren field to decide whether the item has
 * children or not.
 * Note: if this returns TRUE, the child items may not actually exist,
 * they could be virtual.
 *
 * Just use item->firstChild to check for physical children.
 */
static BOOL
TREEVIEW_HasChildren(const TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *item)
{
    TREEVIEW_UpdateDispInfo(infoPtr, item, TVIF_CHILDREN);
    /* Protect for a case when callback field is not changed by a host,
       otherwise negative values trigger normal notifications. */
    return item->cChildren != 0 && item->cChildren != I_CHILDRENCALLBACK;
}

static INT TREEVIEW_NotifyFormat (TREEVIEW_INFO *infoPtr, HWND hwndFrom, UINT nCommand)
{
    INT format;

    TRACE("(hwndFrom=%p, nCommand=%d)\n", hwndFrom, nCommand);

    if (nCommand != NF_REQUERY) return 0;

    format = SendMessageW(hwndFrom, WM_NOTIFYFORMAT, (WPARAM)infoPtr->hwnd, NF_QUERY);
    TRACE("format=%d\n", format);

    /* Invalid format returned by NF_QUERY defaults to ANSI*/
    if (format != NFR_ANSI && format != NFR_UNICODE)
        format = NFR_ANSI;

    infoPtr->bNtfUnicode = (format == NFR_UNICODE);

    return format;
}

/* Item Position ********************************************************/

/* Compute linesOffset, stateOffset, imageOffset, textOffset of an item. */
static VOID
TREEVIEW_ComputeItemInternalMetrics(const TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *item)
{
    /* has TVS_LINESATROOT and (TVS_HASLINES|TVS_HASBUTTONS) */
    BOOL lar = ((infoPtr->dwStyle & (TVS_LINESATROOT|TVS_HASLINES|TVS_HASBUTTONS))
		> TVS_LINESATROOT);

    item->linesOffset = infoPtr->uIndent * (lar ? item->iLevel : item->iLevel - 1)
	- infoPtr->scrollX;
    item->stateOffset = item->linesOffset + infoPtr->uIndent;
    item->imageOffset = item->stateOffset
	+ (STATEIMAGEINDEX(item->state) ? infoPtr->stateImageWidth : 0);
    item->textOffset  = item->imageOffset + infoPtr->normalImageWidth;
}

static VOID
TREEVIEW_ComputeTextWidth(const TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *item, HDC hDC)
{
    HDC hdc;
    HFONT hOldFont=0;
    SIZE sz;

    /* DRAW's OM docker creates items like this */
    if (item->pszText == NULL)
    {
	item->textWidth = 0;
	return;
    }

    if (hDC != 0)
    {
	hdc = hDC;
    }
    else
    {
	hdc = GetDC(infoPtr->hwnd);
	hOldFont = SelectObject(hdc, TREEVIEW_FontForItem(infoPtr, item));
    }

    GetTextExtentPoint32W(hdc, item->pszText, lstrlenW(item->pszText), &sz);
    item->textWidth = sz.cx;

    if (hDC == 0)
    {
	SelectObject(hdc, hOldFont);
	ReleaseDC(0, hdc);
    }
}

static VOID
TREEVIEW_ComputeItemRect(const TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *item)
{
    item->rect.top = infoPtr->uItemHeight *
	(item->visibleOrder - infoPtr->firstVisible->visibleOrder);

    item->rect.bottom = item->rect.top
	+ infoPtr->uItemHeight * item->iIntegral - 1;

    item->rect.left = 0;
    item->rect.right = infoPtr->clientWidth;
}

/* We know that only items after start need their order updated. */
static void
TREEVIEW_RecalculateVisibleOrder(TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *start)
{
    TREEVIEW_ITEM *item;
    int order;

    if (!start)
    {
	start = infoPtr->root->firstChild;
	order = 0;
    }
    else
	order = start->visibleOrder;

    for (item = start; item != NULL;
         item = TREEVIEW_GetNextListItem(infoPtr, item))
    {
	if (!ISVISIBLE(item) && order > 0)
		TREEVIEW_ComputeItemInternalMetrics(infoPtr, item);
	item->visibleOrder = order;
	order += item->iIntegral;
    }

    infoPtr->maxVisibleOrder = order;

    for (item = infoPtr->root->firstChild; item != NULL;
	 item = TREEVIEW_GetNextListItem(infoPtr, item))
    {
	TREEVIEW_ComputeItemRect(infoPtr, item);
    }
}


/* Update metrics of all items in selected subtree.
 * root must be expanded
 */
static VOID
TREEVIEW_UpdateSubTree(const TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *root)
{
   TREEVIEW_ITEM *sibling;
   HDC hdc;
   HFONT hOldFont;

   if (!root->firstChild || !(root->state & TVIS_EXPANDED))
      return;

   root->state &= ~TVIS_EXPANDED;
   sibling = TREEVIEW_GetNextListItem(infoPtr, root);
   root->state |= TVIS_EXPANDED;

   hdc = GetDC(infoPtr->hwnd);
   hOldFont = SelectObject(hdc, infoPtr->hFont);

   for (; root != sibling;
        root = TREEVIEW_GetNextListItem(infoPtr, root))
   {
      TREEVIEW_ComputeItemInternalMetrics(infoPtr, root);

      if (root->callbackMask & TVIF_TEXT)
         TREEVIEW_UpdateDispInfo(infoPtr, root, TVIF_TEXT);

      if (root->textWidth == 0)
      {
         SelectObject(hdc, TREEVIEW_FontForItem(infoPtr, root));
         TREEVIEW_ComputeTextWidth(infoPtr, root, hdc);
      }
   }

   SelectObject(hdc, hOldFont);
   ReleaseDC(infoPtr->hwnd, hdc);
}

/* Item Allocation **********************************************************/

static TREEVIEW_ITEM *
TREEVIEW_AllocateItem(const TREEVIEW_INFO *infoPtr)
{
    TREEVIEW_ITEM *newItem = heap_alloc_zero(sizeof(*newItem));

    if (!newItem)
	return NULL;

    /* I_IMAGENONE would make more sense but this is neither what is
     * documented (MSDN doesn't specify) nor what Windows actually does
     * (it sets it to zero)... and I can so imagine an application using
     * inc/dec to toggle the images. */
    newItem->iImage = 0;
    newItem->iSelectedImage = 0;
    newItem->iExpandedImage = (WORD)I_IMAGENONE;
    newItem->infoPtr = infoPtr;

    if (DPA_InsertPtr(infoPtr->items, INT_MAX, newItem) == -1)
    {
        heap_free(newItem);
        return NULL;
    }

    return newItem;
}

/* Exact opposite of TREEVIEW_AllocateItem. In particular, it does not
 * free item->pszText. */
static void
TREEVIEW_FreeItem(TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *item)
{
    DPA_DeletePtr(infoPtr->items, DPA_GetPtrIndex(infoPtr->items, item));
    if (infoPtr->selectedItem == item)
        infoPtr->selectedItem = NULL;
    if (infoPtr->hotItem == item)
        infoPtr->hotItem = NULL;
    if (infoPtr->focusedItem == item)
        infoPtr->focusedItem = NULL;
    if (infoPtr->firstVisible == item)
        infoPtr->firstVisible = NULL;
    if (infoPtr->dropItem == item)
        infoPtr->dropItem = NULL;
    if (infoPtr->insertMarkItem == item)
        infoPtr->insertMarkItem = NULL;
    heap_free(item);
}


/* Item Insertion *******************************************************/

/***************************************************************************
 * This method inserts newItem before sibling as a child of parent.
 * sibling can be NULL, but only if parent has no children.
 */
static void
TREEVIEW_InsertBefore(TREEVIEW_ITEM *newItem, TREEVIEW_ITEM *sibling,
		      TREEVIEW_ITEM *parent)
{
    assert(parent != NULL);

    if (sibling != NULL)
    {
	assert(sibling->parent == parent);

	if (sibling->prevSibling != NULL)
	    sibling->prevSibling->nextSibling = newItem;

	newItem->prevSibling = sibling->prevSibling;
	sibling->prevSibling = newItem;
    }
    else
       newItem->prevSibling = NULL;

    newItem->nextSibling = sibling;

    if (parent->firstChild == sibling)
	parent->firstChild = newItem;

    if (parent->lastChild == NULL)
	parent->lastChild = newItem;
}

/***************************************************************************
 * This method inserts newItem after sibling as a child of parent.
 * sibling can be NULL, but only if parent has no children.
 */
static void
TREEVIEW_InsertAfter(TREEVIEW_ITEM *newItem, TREEVIEW_ITEM *sibling,
		     TREEVIEW_ITEM *parent)
{
    assert(parent != NULL);

    if (sibling != NULL)
    {
	assert(sibling->parent == parent);

	if (sibling->nextSibling != NULL)
	    sibling->nextSibling->prevSibling = newItem;

	newItem->nextSibling = sibling->nextSibling;
	sibling->nextSibling = newItem;
    }
    else
       newItem->nextSibling = NULL;

    newItem->prevSibling = sibling;

    if (parent->lastChild == sibling)
	parent->lastChild = newItem;

    if (parent->firstChild == NULL)
	parent->firstChild = newItem;
}

static BOOL
TREEVIEW_DoSetItemT(const TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *item,
		   const TVITEMEXW *tvItem, BOOL isW)
{
    UINT callbackClear = 0;
    UINT callbackSet = 0;

    TRACE("item %p\n", item);
    /* Do this first in case it fails. */
    if (tvItem->mask & TVIF_TEXT)
    {
        item->textWidth = 0; /* force width recalculation */

        /* Covers != TEXTCALLBACKA too, and undocumented: pszText of NULL also means TEXTCALLBACK */
        if (tvItem->pszText != LPSTR_TEXTCALLBACKW && tvItem->pszText != NULL)
        {
            int len;
            LPWSTR newText;
            if (isW)
                len = lstrlenW(tvItem->pszText) + 1;
            else
                len = MultiByteToWideChar(CP_ACP, 0, (LPSTR)tvItem->pszText, -1, NULL, 0);

            /* Allocate new block to make pointer comparison in item_changed() work. */
            newText = heap_alloc(len * sizeof(WCHAR));

            if (newText == NULL) return FALSE;

            callbackClear |= TVIF_TEXT;

            heap_free(item->pszText);
            item->pszText = newText;
            item->cchTextMax = len;
            if (isW)
                lstrcpynW(item->pszText, tvItem->pszText, len);
            else
                MultiByteToWideChar(CP_ACP, 0, (LPSTR)tvItem->pszText, -1,
                                    item->pszText, len);

            TRACE("setting text %s, item %p\n", debugstr_w(item->pszText), item);
        }
	else
	{
            callbackSet |= TVIF_TEXT;
            item->pszText = heap_realloc(item->pszText, TEXT_CALLBACK_SIZE * sizeof(WCHAR));
	    item->cchTextMax = TEXT_CALLBACK_SIZE;
	    TRACE("setting callback, item %p\n", item);
	}
    }

    if (tvItem->mask & TVIF_CHILDREN)
    {
	item->cChildren = tvItem->cChildren;

	if (item->cChildren == I_CHILDRENCALLBACK)
	    callbackSet |= TVIF_CHILDREN;
	else
	    callbackClear |= TVIF_CHILDREN;
    }

    if (tvItem->mask & TVIF_IMAGE)
    {
	item->iImage = tvItem->iImage;

	if (item->iImage == I_IMAGECALLBACK)
	    callbackSet |= TVIF_IMAGE;
	else
	    callbackClear |= TVIF_IMAGE;
    }

    if (tvItem->mask & TVIF_SELECTEDIMAGE)
    {
	item->iSelectedImage = tvItem->iSelectedImage;

	if (item->iSelectedImage == I_IMAGECALLBACK)
	    callbackSet |= TVIF_SELECTEDIMAGE;
	else
	    callbackClear |= TVIF_SELECTEDIMAGE;
    }

    if (tvItem->mask & TVIF_EXPANDEDIMAGE)
    {
	item->iExpandedImage = tvItem->iExpandedImage;

	if (item->iExpandedImage == I_IMAGECALLBACK)
	    callbackSet |= TVIF_EXPANDEDIMAGE;
	else
	    callbackClear |= TVIF_EXPANDEDIMAGE;
    }

    if (tvItem->mask & TVIF_PARAM)
	item->lParam = tvItem->lParam;

    /* If the application sets TVIF_INTEGRAL without
     * supplying a TVITEMEX structure, it's toast. */
    if (tvItem->mask & TVIF_INTEGRAL)
	item->iIntegral = tvItem->iIntegral;

    if (tvItem->mask & TVIF_STATE)
    {
	TRACE("prevstate 0x%x, state 0x%x, mask 0x%x\n", item->state, tvItem->state,
	      tvItem->stateMask);
	item->state &= ~tvItem->stateMask;
	item->state |= (tvItem->state & tvItem->stateMask);
    }

    if (tvItem->mask & TVIF_STATEEX)
    {
        FIXME("New extended state: 0x%x\n", tvItem->uStateEx);
    }

    item->callbackMask |= callbackSet;
    item->callbackMask &= ~callbackClear;

    return TRUE;
}

/* Note that the new item is pre-zeroed. */
static LRESULT
TREEVIEW_InsertItemT(TREEVIEW_INFO *infoPtr, const TVINSERTSTRUCTW *ptdi, BOOL isW)
{
    const TVITEMEXW *tvItem = &ptdi->u.itemex;
    HTREEITEM insertAfter;
    TREEVIEW_ITEM *newItem, *parentItem;
    BOOL bTextUpdated = FALSE;

    if (ptdi->hParent == TVI_ROOT || ptdi->hParent == 0)
    {
	parentItem = infoPtr->root;
    }
    else
    {
	parentItem = ptdi->hParent;

	if (!TREEVIEW_ValidItem(infoPtr, parentItem))
	{
	    WARN("invalid parent %p\n", parentItem);
            return 0;
	}
    }

    insertAfter = ptdi->hInsertAfter;

    /* Validate this now for convenience. */
    switch ((DWORD_PTR)insertAfter)
    {
    case (DWORD_PTR)TVI_FIRST:
    case (DWORD_PTR)TVI_LAST:
    case (DWORD_PTR)TVI_SORT:
	break;

    default:
	if (!TREEVIEW_ValidItem(infoPtr, insertAfter) ||
            insertAfter->parent != parentItem)
	{
	    WARN("invalid insert after %p\n", insertAfter);
	    insertAfter = TVI_LAST;
	}
    }

    TRACE("parent %p position %p: %s\n", parentItem, insertAfter,
	  (tvItem->mask & TVIF_TEXT)
	  ? ((tvItem->pszText == LPSTR_TEXTCALLBACKW) ? "<callback>"
	     : (isW ? debugstr_w(tvItem->pszText) : debugstr_a((LPSTR)tvItem->pszText)))
	  : "<no label>");

    newItem = TREEVIEW_AllocateItem(infoPtr);
    if (newItem == NULL)
        return 0;

    newItem->parent = parentItem;
    newItem->iIntegral = 1;
    newItem->visibleOrder = -1;

    if (!TREEVIEW_DoSetItemT(infoPtr, newItem, tvItem, isW))
        return 0;

    /* After this point, nothing can fail. (Except for TVI_SORT.) */

    infoPtr->uNumItems++;

    switch ((DWORD_PTR)insertAfter)
    {
    case (DWORD_PTR)TVI_FIRST:
        {
           TREEVIEW_ITEM *originalFirst = parentItem->firstChild;
           TREEVIEW_InsertBefore(newItem, parentItem->firstChild, parentItem);
           if (infoPtr->firstVisible == originalFirst)
              TREEVIEW_SetFirstVisible(infoPtr, newItem, TRUE);
        }
	break;

    case (DWORD_PTR)TVI_LAST:
	TREEVIEW_InsertAfter(newItem, parentItem->lastChild, parentItem);
	break;

	/* hInsertAfter names a specific item we want to insert after */
    default:
	TREEVIEW_InsertAfter(newItem, insertAfter, insertAfter->parent);
	break;

    case (DWORD_PTR)TVI_SORT:
	{
	    TREEVIEW_ITEM *aChild;
	    TREEVIEW_ITEM *previousChild = NULL;
            TREEVIEW_ITEM *originalFirst = parentItem->firstChild;
	    BOOL bItemInserted = FALSE;

	    aChild = parentItem->firstChild;

	    bTextUpdated = TRUE;
	    TREEVIEW_UpdateDispInfo(infoPtr, newItem, TVIF_TEXT);

	    /* Iterate the parent children to see where we fit in */
	    while (aChild != NULL)
	    {
		INT comp;

		TREEVIEW_UpdateDispInfo(infoPtr, aChild, TVIF_TEXT);
		comp = lstrcmpW(newItem->pszText, aChild->pszText);

		if (comp < 0)	/* we are smaller than the current one */
		{
		    TREEVIEW_InsertBefore(newItem, aChild, parentItem);
                    if (infoPtr->firstVisible == originalFirst &&
                        aChild == originalFirst)
                        TREEVIEW_SetFirstVisible(infoPtr, newItem, TRUE);
		    bItemInserted = TRUE;
		    break;
		}
		else if (comp > 0)	/* we are bigger than the current one */
		{
		    previousChild = aChild;

		    /* This will help us to exit if there is no more sibling */
		    aChild = (aChild->nextSibling == 0)
			? NULL
			: aChild->nextSibling;

		    /* Look at the next item */
		    continue;
		}
		else if (comp == 0)
		{
		    /*
		     * An item with this name is already existing, therefore,
		     * we add after the one we found
		     */
		    TREEVIEW_InsertAfter(newItem, aChild, parentItem);
		    bItemInserted = TRUE;
		    break;
		}
	    }

	    /*
	     * we reach the end of the child list and the item has not
	     * yet been inserted, therefore, insert it after the last child.
	     */
	    if ((!bItemInserted) && (aChild == NULL))
		TREEVIEW_InsertAfter(newItem, previousChild, parentItem);

	    break;
	}
    }


    TRACE("new item %p; parent %p, mask 0x%x\n", newItem,
	  newItem->parent, tvItem->mask);

    newItem->iLevel = newItem->parent->iLevel + 1;

    if (newItem->parent->cChildren == 0)
	newItem->parent->cChildren = 1;

    if (infoPtr->dwStyle & TVS_CHECKBOXES)
    {
	if (STATEIMAGEINDEX(newItem->state) == 0)
	    newItem->state |= INDEXTOSTATEIMAGEMASK(1);
    }

    if (infoPtr->firstVisible == NULL)
	infoPtr->firstVisible = newItem;

    TREEVIEW_VerifyTree(infoPtr);

    if (!infoPtr->bRedraw) return (LRESULT)newItem;

    if (parentItem == infoPtr->root ||
        (ISVISIBLE(parentItem) && parentItem->state & TVIS_EXPANDED))
    {
       TREEVIEW_ITEM *item;
       TREEVIEW_ITEM *prev = TREEVIEW_GetPrevListItem(infoPtr, newItem);

       TREEVIEW_RecalculateVisibleOrder(infoPtr, prev);
       TREEVIEW_ComputeItemInternalMetrics(infoPtr, newItem);

       if (!bTextUpdated)
          TREEVIEW_UpdateDispInfo(infoPtr, newItem, TVIF_TEXT);

       TREEVIEW_ComputeTextWidth(infoPtr, newItem, 0);
       TREEVIEW_UpdateScrollBars(infoPtr);
    /*
     * if the item was inserted in a visible part of the tree,
     * invalidate it, as well as those after it
     */
       for (item = newItem;
            item != NULL;
	    item = TREEVIEW_GetNextListItem(infoPtr, item))
          TREEVIEW_Invalidate(infoPtr, item);
    }
    else
    {
       /* refresh treeview if newItem is the first item inserted under parentItem */
       if (ISVISIBLE(parentItem) && newItem->prevSibling == newItem->nextSibling)
       {
          /* parent got '+' - update it */
          TREEVIEW_Invalidate(infoPtr, parentItem);
       }
    }

    return (LRESULT)newItem;
}

/* Item Deletion ************************************************************/
static void
TREEVIEW_RemoveItem(TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *item);

static void
TREEVIEW_RemoveAllChildren(TREEVIEW_INFO *infoPtr, const TREEVIEW_ITEM *parentItem)
{
    TREEVIEW_ITEM *kill = parentItem->firstChild;

    while (kill != NULL)
    {
	TREEVIEW_ITEM *next = kill->nextSibling;

	TREEVIEW_RemoveItem(infoPtr, kill);

	kill = next;
    }

    assert(parentItem->cChildren <= 0); /* I_CHILDRENCALLBACK or 0 */
    assert(parentItem->firstChild == NULL);
    assert(parentItem->lastChild == NULL);
}

static void
TREEVIEW_UnlinkItem(const TREEVIEW_ITEM *item)
{
    TREEVIEW_ITEM *parentItem;

    assert(item != NULL);
    assert(item->parent != NULL); /* i.e. it must not be the root */

    parentItem = item->parent;

    if (parentItem->firstChild == item)
	parentItem->firstChild = item->nextSibling;

    if (parentItem->lastChild == item)
	parentItem->lastChild = item->prevSibling;

    if (parentItem->firstChild == NULL && parentItem->lastChild == NULL
	&& parentItem->cChildren > 0)
	parentItem->cChildren = 0;

    if (item->prevSibling)
	item->prevSibling->nextSibling = item->nextSibling;

    if (item->nextSibling)
	item->nextSibling->prevSibling = item->prevSibling;
}

static void
TREEVIEW_RemoveItem(TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *item)
{
    TRACE("%p, (%s)\n", item, TREEVIEW_ItemName(item));

    if (item->firstChild)
	TREEVIEW_RemoveAllChildren(infoPtr, item);

    TREEVIEW_SendTreeviewNotify(infoPtr, TVN_DELETEITEMW, TVC_UNKNOWN,
				TVIF_HANDLE | TVIF_PARAM, item, 0);

    TREEVIEW_UnlinkItem(item);

    infoPtr->uNumItems--;

    if (item->pszText != LPSTR_TEXTCALLBACKW)
        heap_free(item->pszText);

    TREEVIEW_FreeItem(infoPtr, item);
}


/* Empty out the tree. */
static void
TREEVIEW_RemoveTree(TREEVIEW_INFO *infoPtr)
{
    TREEVIEW_RemoveAllChildren(infoPtr, infoPtr->root);

    assert(infoPtr->uNumItems == 0);	/* root isn't counted in uNumItems */
}

static LRESULT
TREEVIEW_DeleteItem(TREEVIEW_INFO *infoPtr, HTREEITEM item)
{
    TREEVIEW_ITEM *newSelection = NULL;
    TREEVIEW_ITEM *newFirstVisible = NULL;
    TREEVIEW_ITEM *parent, *prev = NULL;
    BOOL visible = FALSE;

    if (item == TVI_ROOT || !item)
    {
	TRACE("TVI_ROOT\n");
	parent = infoPtr->root;
	newSelection = NULL;
	visible = TRUE;
	TREEVIEW_RemoveTree(infoPtr);
    }
    else
    {
	if (!TREEVIEW_ValidItem(infoPtr, item))
	    return FALSE;

	TRACE("%p (%s)\n", item, TREEVIEW_ItemName(item));
	parent = item->parent;

        if (ISVISIBLE(item))
        {
            prev = TREEVIEW_GetPrevListItem(infoPtr, item);
            visible = TRUE;
        }

	if (infoPtr->selectedItem != NULL
	    && (item == infoPtr->selectedItem
		|| TREEVIEW_IsChildOf(item, infoPtr->selectedItem)))
	{
	    if (item->nextSibling)
		newSelection = item->nextSibling;
	    else if (item->parent != infoPtr->root)
		newSelection = item->parent;
            else
                newSelection = item->prevSibling;
            TRACE("newSelection = %p\n", newSelection);
	}

	if (infoPtr->firstVisible == item)
	{
	    visible = TRUE;
	    if (item->nextSibling)
	       newFirstVisible = item->nextSibling;
	    else if (item->prevSibling)
	       newFirstVisible = item->prevSibling;
	    else if (item->parent != infoPtr->root)
	       newFirstVisible = item->parent;
	    TREEVIEW_SetFirstVisible(infoPtr, NULL, TRUE);
	}
	else
	    newFirstVisible = infoPtr->firstVisible;

	TREEVIEW_RemoveItem(infoPtr, item);
    }

    /* Don't change if somebody else already has (infoPtr->selectedItem is cleared by FreeItem). */
    if (!infoPtr->selectedItem && newSelection)
    {
	if (TREEVIEW_ValidItem(infoPtr, newSelection))
	    TREEVIEW_DoSelectItem(infoPtr, TVGN_CARET, newSelection, TVC_UNKNOWN);
    }

    /* Validate insertMark dropItem.
     * hotItem ??? - used for comparison only.
     */
    if (!TREEVIEW_ValidItem(infoPtr, infoPtr->insertMarkItem))
	infoPtr->insertMarkItem = 0;

    if (!TREEVIEW_ValidItem(infoPtr, infoPtr->dropItem))
	infoPtr->dropItem = 0;

    if (!TREEVIEW_ValidItem(infoPtr, newFirstVisible))
        newFirstVisible = infoPtr->root->firstChild;

    TREEVIEW_VerifyTree(infoPtr);

    if (visible)
        TREEVIEW_SetFirstVisible(infoPtr, newFirstVisible, TRUE);

    if (!infoPtr->bRedraw) return TRUE;

    if (visible)
    {
       TREEVIEW_RecalculateVisibleOrder(infoPtr, prev);
       TREEVIEW_UpdateScrollBars(infoPtr);
       TREEVIEW_Invalidate(infoPtr, NULL);
    }
    else if (ISVISIBLE(parent) && !TREEVIEW_HasChildren(infoPtr, parent))
    {
       /* parent lost '+/-' - update it */
       TREEVIEW_Invalidate(infoPtr, parent);
    }

    return TRUE;
}


/* Get/Set Messages *********************************************************/
static LRESULT
TREEVIEW_SetRedraw(TREEVIEW_INFO* infoPtr, WPARAM wParam)
{
    infoPtr->bRedraw = wParam != 0;

    if (infoPtr->bRedraw)
    {
        TREEVIEW_UpdateSubTree(infoPtr, infoPtr->root);
        TREEVIEW_RecalculateVisibleOrder(infoPtr, NULL);
        TREEVIEW_UpdateScrollBars(infoPtr);
        TREEVIEW_Invalidate(infoPtr, NULL);
    }
    return 0;
}

static LRESULT
TREEVIEW_GetIndent(const TREEVIEW_INFO *infoPtr)
{
    TRACE("\n");
    return infoPtr->uIndent;
}

static LRESULT
TREEVIEW_SetIndent(TREEVIEW_INFO *infoPtr, UINT newIndent)
{
    TRACE("\n");

    if (newIndent < MINIMUM_INDENT)
	newIndent = MINIMUM_INDENT;

    if (infoPtr->uIndent != newIndent)
    {
	infoPtr->uIndent = newIndent;
	TREEVIEW_UpdateSubTree(infoPtr, infoPtr->root);
	TREEVIEW_UpdateScrollBars(infoPtr);
	TREEVIEW_Invalidate(infoPtr, NULL);
    }

    return 0;
}


static LRESULT
TREEVIEW_GetToolTips(const TREEVIEW_INFO *infoPtr)
{
    TRACE("\n");
    return (LRESULT)infoPtr->hwndToolTip;
}

static LRESULT
TREEVIEW_SetToolTips(TREEVIEW_INFO *infoPtr, HWND hwndTT)
{
    HWND prevToolTip;

    TRACE("\n");
    prevToolTip = infoPtr->hwndToolTip;
    infoPtr->hwndToolTip = hwndTT;

    return (LRESULT)prevToolTip;
}

static LRESULT
TREEVIEW_SetUnicodeFormat(TREEVIEW_INFO *infoPtr, BOOL fUnicode)
{
    BOOL rc = infoPtr->bNtfUnicode;
    infoPtr->bNtfUnicode = fUnicode;
    return rc;
}

static LRESULT
TREEVIEW_GetUnicodeFormat(const TREEVIEW_INFO *infoPtr)
{
     return infoPtr->bNtfUnicode;
}

static LRESULT
TREEVIEW_GetScrollTime(const TREEVIEW_INFO *infoPtr)
{
    return infoPtr->uScrollTime;
}

static LRESULT
TREEVIEW_SetScrollTime(TREEVIEW_INFO *infoPtr, UINT uScrollTime)
{
    UINT uOldScrollTime = infoPtr->uScrollTime;

    infoPtr->uScrollTime = min(uScrollTime, 100);

    return uOldScrollTime;
}


static LRESULT
TREEVIEW_GetImageList(const TREEVIEW_INFO *infoPtr, WPARAM wParam)
{
    TRACE("\n");

    switch (wParam)
    {
    case TVSIL_NORMAL:
	return (LRESULT)infoPtr->himlNormal;

    case TVSIL_STATE:
	return (LRESULT)infoPtr->himlState;

    default:
	return 0;
    }
}

#define TVHEIGHT_MIN         16
#define TVHEIGHT_FONT_ADJUST 3 /* 2 for focus border + 1 for margin some apps assume */

/* Compute the natural height for items. */
static UINT
TREEVIEW_NaturalHeight(const TREEVIEW_INFO *infoPtr)
{
    TEXTMETRICW tm;
    HDC hdc = GetDC(0);
    HFONT hOldFont = SelectObject(hdc, infoPtr->hFont);
    UINT height;

    /* Height is the maximum of:
     * 16 (a hack because our fonts are tiny), and
     * The text height + border & margin, and
     * The size of the normal image list
     */
    GetTextMetricsW(hdc, &tm);
    SelectObject(hdc, hOldFont);
    ReleaseDC(0, hdc);

    height = TVHEIGHT_MIN;
    if (height < tm.tmHeight + tm.tmExternalLeading + TVHEIGHT_FONT_ADJUST)
        height = tm.tmHeight + tm.tmExternalLeading + TVHEIGHT_FONT_ADJUST;
    if (height < infoPtr->normalImageHeight)
        height = infoPtr->normalImageHeight;

    /* Round down, unless we support odd ("non even") heights. */
    if (!(infoPtr->dwStyle & TVS_NONEVENHEIGHT))
        height &= ~1;

    return height;
}

static LRESULT
TREEVIEW_SetImageList(TREEVIEW_INFO *infoPtr, UINT type, HIMAGELIST himlNew)
{
    HIMAGELIST himlOld = 0;
    int oldWidth  = infoPtr->normalImageWidth;
    int oldHeight = infoPtr->normalImageHeight;

    TRACE("%u,%p\n", type, himlNew);

    switch (type)
    {
    case TVSIL_NORMAL:
	himlOld = infoPtr->himlNormal;
	infoPtr->himlNormal = himlNew;

	if (himlNew)
	    ImageList_GetIconSize(himlNew, &infoPtr->normalImageWidth,
				  &infoPtr->normalImageHeight);
	else
	{
	    infoPtr->normalImageWidth = 0;
	    infoPtr->normalImageHeight = 0;
	}

	break;

    case TVSIL_STATE:
	himlOld = infoPtr->himlState;
	infoPtr->himlState = himlNew;

	if (himlNew)
	    ImageList_GetIconSize(himlNew, &infoPtr->stateImageWidth,
				  &infoPtr->stateImageHeight);
	else
	{
	    infoPtr->stateImageWidth = 0;
	    infoPtr->stateImageHeight = 0;
	}

	break;

    default:
        ERR("unknown imagelist type %u\n", type);
    }

    if (oldWidth != infoPtr->normalImageWidth ||
        oldHeight != infoPtr->normalImageHeight)
    {
        BOOL bRecalcVisible = FALSE;

        if (oldHeight != infoPtr->normalImageHeight &&
            !infoPtr->bHeightSet)
        {
            infoPtr->uItemHeight = TREEVIEW_NaturalHeight(infoPtr);
            bRecalcVisible = TRUE;
        }

        if (infoPtr->normalImageWidth > MINIMUM_INDENT &&
            infoPtr->normalImageWidth != infoPtr->uIndent)
        {
            infoPtr->uIndent = infoPtr->normalImageWidth;
            bRecalcVisible = TRUE;
        }

        if (bRecalcVisible)
            TREEVIEW_RecalculateVisibleOrder(infoPtr, NULL);

       TREEVIEW_UpdateSubTree(infoPtr, infoPtr->root);
       TREEVIEW_UpdateScrollBars(infoPtr);
    }

    TREEVIEW_Invalidate(infoPtr, NULL);

    return (LRESULT)himlOld;
}

static LRESULT
TREEVIEW_SetItemHeight(TREEVIEW_INFO *infoPtr, INT newHeight)
{
    INT prevHeight = infoPtr->uItemHeight;

    TRACE("new=%d, old=%d\n", newHeight, prevHeight);
    if (newHeight == -1)
    {
	infoPtr->uItemHeight = TREEVIEW_NaturalHeight(infoPtr);
	infoPtr->bHeightSet = FALSE;
    }
    else
    {
        if (newHeight == 0) newHeight = 1;
        infoPtr->uItemHeight = newHeight;
        infoPtr->bHeightSet = TRUE;
    }

    /* Round down, unless we support odd ("non even") heights. */
    if (!(infoPtr->dwStyle & TVS_NONEVENHEIGHT) && infoPtr->uItemHeight != 1)
    {
        infoPtr->uItemHeight &= ~1;
        TRACE("after rounding=%d\n", infoPtr->uItemHeight);
    }

    if (infoPtr->uItemHeight != prevHeight)
    {
	TREEVIEW_RecalculateVisibleOrder(infoPtr, NULL);
	TREEVIEW_UpdateScrollBars(infoPtr);
	TREEVIEW_Invalidate(infoPtr, NULL);
    }

    return prevHeight;
}

static LRESULT
TREEVIEW_GetItemHeight(const TREEVIEW_INFO *infoPtr)
{
    TRACE("\n");
    return infoPtr->uItemHeight;
}


static LRESULT
TREEVIEW_GetFont(const TREEVIEW_INFO *infoPtr)
{
    TRACE("%p\n", infoPtr->hFont);
    return (LRESULT)infoPtr->hFont;
}


static INT CALLBACK
TREEVIEW_ResetTextWidth(LPVOID pItem, LPVOID unused)
{
    (void)unused;

    ((TREEVIEW_ITEM *)pItem)->textWidth = 0;

    return 1;
}

static LRESULT
TREEVIEW_SetFont(TREEVIEW_INFO *infoPtr, HFONT hFont, BOOL bRedraw)
{
    UINT uHeight = infoPtr->uItemHeight;

    TRACE("%p %i\n", hFont, bRedraw);

    infoPtr->hFont = hFont ? hFont : infoPtr->hDefaultFont;

    DeleteObject(infoPtr->hBoldFont);
    DeleteObject(infoPtr->hUnderlineFont);
    DeleteObject(infoPtr->hBoldUnderlineFont);
    infoPtr->hBoldFont = TREEVIEW_CreateBoldFont(infoPtr->hFont);
    infoPtr->hUnderlineFont = TREEVIEW_CreateUnderlineFont(infoPtr->hFont);
    infoPtr->hBoldUnderlineFont = TREEVIEW_CreateBoldUnderlineFont(infoPtr->hFont);

    if (!infoPtr->bHeightSet)
	infoPtr->uItemHeight = TREEVIEW_NaturalHeight(infoPtr);

    if (uHeight != infoPtr->uItemHeight)
       TREEVIEW_RecalculateVisibleOrder(infoPtr, NULL);

    DPA_EnumCallback(infoPtr->items, TREEVIEW_ResetTextWidth, 0);

    TREEVIEW_UpdateSubTree(infoPtr, infoPtr->root);
    TREEVIEW_UpdateScrollBars(infoPtr);

    if (bRedraw)
	TREEVIEW_Invalidate(infoPtr, NULL);

    return 0;
}


static LRESULT
TREEVIEW_GetLineColor(const TREEVIEW_INFO *infoPtr)
{
    TRACE("\n");
    return (LRESULT)infoPtr->clrLine;
}

static LRESULT
TREEVIEW_SetLineColor(TREEVIEW_INFO *infoPtr, COLORREF color)
{
    COLORREF prevColor = infoPtr->clrLine;

    TRACE("\n");
    infoPtr->clrLine = color;
    return (LRESULT)prevColor;
}


static LRESULT
TREEVIEW_GetTextColor(const TREEVIEW_INFO *infoPtr)
{
    TRACE("\n");
    return (LRESULT)infoPtr->clrText;
}

static LRESULT
TREEVIEW_SetTextColor(TREEVIEW_INFO *infoPtr, COLORREF color)
{
    COLORREF prevColor = infoPtr->clrText;

    TRACE("\n");
    infoPtr->clrText = color;

    if (infoPtr->clrText != prevColor)
	TREEVIEW_Invalidate(infoPtr, NULL);

    return (LRESULT)prevColor;
}


static LRESULT
TREEVIEW_GetBkColor(const TREEVIEW_INFO *infoPtr)
{
    TRACE("\n");
    return (LRESULT)infoPtr->clrBk;
}

static LRESULT
TREEVIEW_SetBkColor(TREEVIEW_INFO *infoPtr, COLORREF newColor)
{
    COLORREF prevColor = infoPtr->clrBk;

    TRACE("\n");
    infoPtr->clrBk = newColor;

    if (newColor != prevColor)
	TREEVIEW_Invalidate(infoPtr, NULL);

    return (LRESULT)prevColor;
}


static LRESULT
TREEVIEW_GetInsertMarkColor(const TREEVIEW_INFO *infoPtr)
{
    TRACE("\n");
    return (LRESULT)infoPtr->clrInsertMark;
}

static LRESULT
TREEVIEW_SetInsertMarkColor(TREEVIEW_INFO *infoPtr, COLORREF color)
{
    COLORREF prevColor = infoPtr->clrInsertMark;

    TRACE("0x%08x\n", color);
    infoPtr->clrInsertMark = color;

    return (LRESULT)prevColor;
}


static LRESULT
TREEVIEW_SetInsertMark(TREEVIEW_INFO *infoPtr, BOOL wParam, HTREEITEM item)
{
    TRACE("%d %p\n", wParam, item);

    if (!TREEVIEW_ValidItem(infoPtr, item))
	return 0;

    infoPtr->insertBeforeorAfter = wParam;
    infoPtr->insertMarkItem = item;

    TREEVIEW_Invalidate(infoPtr, NULL);

    return 1;
}


/************************************************************************
 * Some serious braindamage here. lParam is a pointer to both the
 * input HTREEITEM and the output RECT.
 */
static LRESULT
TREEVIEW_GetItemRect(const TREEVIEW_INFO *infoPtr, BOOL fTextRect, LPRECT lpRect)
{
    TREEVIEW_ITEM *item;
    const HTREEITEM *pItem = (HTREEITEM *)lpRect;

    TRACE("\n");

    if (pItem == NULL)
	return FALSE;

    item = *pItem;
    if (!TREEVIEW_ValidItem(infoPtr, item) || !ISVISIBLE(item))
	return FALSE;

    /*
     * If wParam is TRUE return the text size otherwise return
     * the whole item size
     */
    if (fTextRect)
    {
	/* Windows does not send TVN_GETDISPINFO here. */

	lpRect->top = item->rect.top;
	lpRect->bottom = item->rect.bottom;

	lpRect->left = item->textOffset;
	if (!item->textWidth)
		TREEVIEW_ComputeTextWidth(infoPtr, item, 0);

	lpRect->right = item->textOffset + item->textWidth + 4;
    }
    else
    {
	*lpRect = item->rect;
    }

    TRACE("%s [%s]\n", fTextRect ? "text" : "item", wine_dbgstr_rect(lpRect));

    return TRUE;
}

static inline LRESULT
TREEVIEW_GetVisibleCount(const TREEVIEW_INFO *infoPtr)
{
    /* Surprise! This does not take integral height into account. */
    TRACE("client=%d, item=%d\n", infoPtr->clientHeight, infoPtr->uItemHeight);
    return infoPtr->clientHeight / infoPtr->uItemHeight;
}


static LRESULT
TREEVIEW_GetItemT(const TREEVIEW_INFO *infoPtr, LPTVITEMEXW tvItem, BOOL isW)
{
    TREEVIEW_ITEM *item = tvItem->hItem;

    if (!TREEVIEW_ValidItem(infoPtr, item))
    {
        BOOL valid_item = FALSE;
        if (!item) return FALSE;

        __TRY
        {
            infoPtr = item->infoPtr;
            TRACE("got item from different tree %p, called from %p\n", item->infoPtr, infoPtr);
            valid_item = TREEVIEW_ValidItem(infoPtr, item);
        }
        __EXCEPT_PAGE_FAULT
        {
        }
        __ENDTRY
        if (!valid_item) return FALSE;
    }

    TREEVIEW_UpdateDispInfo(infoPtr, item, tvItem->mask);

    if (tvItem->mask & TVIF_CHILDREN)
    {
        if (item->cChildren==I_CHILDRENCALLBACK)
            FIXME("I_CHILDRENCALLBACK not supported\n");
	tvItem->cChildren = item->cChildren;
    }

    if (tvItem->mask & TVIF_HANDLE)
	tvItem->hItem = item;

    if (tvItem->mask & TVIF_IMAGE)
	tvItem->iImage = item->iImage;

    if (tvItem->mask & TVIF_INTEGRAL)
	tvItem->iIntegral = item->iIntegral;

    /* undocumented: (mask & TVIF_PARAM) ignored and lParam is always set */
    tvItem->lParam = item->lParam;

    if (tvItem->mask & TVIF_SELECTEDIMAGE)
	tvItem->iSelectedImage = item->iSelectedImage;

    if (tvItem->mask & TVIF_EXPANDEDIMAGE)
	tvItem->iExpandedImage = item->iExpandedImage;

    /* undocumented: stateMask and (state & TVIF_STATE) ignored, so state is always set */
    tvItem->state = item->state;

    if (tvItem->mask & TVIF_TEXT)
    {
        if (item->pszText == NULL)
        {
            if (tvItem->cchTextMax > 0)
                tvItem->pszText[0] = '\0';
        }
        else if (isW)
        {
            if (item->pszText == LPSTR_TEXTCALLBACKW)
            {
                tvItem->pszText = LPSTR_TEXTCALLBACKW;
                FIXME(" GetItem called with LPSTR_TEXTCALLBACK\n");
            }
            else
            {
                lstrcpynW(tvItem->pszText, item->pszText, tvItem->cchTextMax);
            }
        }
        else
        {
            if (item->pszText == LPSTR_TEXTCALLBACKW)
            {
                tvItem->pszText = (LPWSTR)LPSTR_TEXTCALLBACKA;
                FIXME(" GetItem called with LPSTR_TEXTCALLBACK\n");
            }
            else
            {
                WideCharToMultiByte(CP_ACP, 0, item->pszText, -1,
                                    (LPSTR)tvItem->pszText, tvItem->cchTextMax, NULL, NULL);
            }
        }
    }

    if (tvItem->mask & TVIF_STATEEX)
    {
        FIXME("Extended item state not supported, returning 0.\n");
        tvItem->uStateEx = 0;
    }

    TRACE("item <%p>, txt %p, img %d, mask 0x%x\n",
	  item, tvItem->pszText, tvItem->iImage, tvItem->mask);

    return TRUE;
}

/* Beware MSDN Library Visual Studio 6.0. It says -1 on failure, 0 on success,
 * which is wrong. */
static LRESULT
TREEVIEW_SetItemT(TREEVIEW_INFO *infoPtr, const TVITEMEXW *tvItem, BOOL isW)
{
    TREEVIEW_ITEM *item;
    TREEVIEW_ITEM originalItem;

    item = tvItem->hItem;

    TRACE("item %d, mask 0x%x\n", TREEVIEW_GetItemIndex(infoPtr, item),
	  tvItem->mask);

    if (!TREEVIEW_ValidItem(infoPtr, item))
	return FALSE;

    /* Store the original item values. Text buffer will be freed in TREEVIEW_DoSetItemT() below. */
    originalItem = *item;

    if (!TREEVIEW_DoSetItemT(infoPtr, item, tvItem, isW))
	return FALSE;

    /* If the text or TVIS_BOLD was changed, and it is visible, recalculate. */
    if ((tvItem->mask & TVIF_TEXT
	 || (tvItem->mask & TVIF_STATE && tvItem->stateMask & TVIS_BOLD))
	&& ISVISIBLE(item))
    {
	TREEVIEW_UpdateDispInfo(infoPtr, item, TVIF_TEXT);
	TREEVIEW_ComputeTextWidth(infoPtr, item, 0);
    }

    if (tvItem->mask != 0 && ISVISIBLE(item))
    {
	/* The refresh updates everything, but we can't wait until then. */
	TREEVIEW_ComputeItemInternalMetrics(infoPtr, item);

        /* if any of the item's values changed and it's not a callback, redraw the item */
        if (item_changed(&originalItem, item, tvItem))
        {
            if (tvItem->mask & TVIF_INTEGRAL)
	    {
	        TREEVIEW_RecalculateVisibleOrder(infoPtr, item);
	        TREEVIEW_UpdateScrollBars(infoPtr);

	        TREEVIEW_Invalidate(infoPtr, NULL);
	    }
	    else
	    {
	        TREEVIEW_UpdateScrollBars(infoPtr);
	        TREEVIEW_Invalidate(infoPtr, item);
	    }
        }
    }

    return TRUE;
}

static LRESULT
TREEVIEW_GetItemState(const TREEVIEW_INFO *infoPtr, HTREEITEM item, UINT mask)
{
    TRACE("\n");

    if (!item || !TREEVIEW_ValidItem(infoPtr, item))
	return 0;

    return (item->state & mask);
}

static LRESULT
TREEVIEW_GetNextItem(const TREEVIEW_INFO *infoPtr, UINT which, HTREEITEM item)
{
    TREEVIEW_ITEM *retval;

    retval = 0;

    /* handle all the global data here */
    switch (which)
    {
    case TVGN_CHILD:		/* Special case: child of 0 is root */
	if (item)
	    break;
	/* fall through */
    case TVGN_ROOT:
	retval = infoPtr->root->firstChild;
	break;

    case TVGN_CARET:
	retval = infoPtr->selectedItem;
	break;

    case TVGN_FIRSTVISIBLE:
	retval = infoPtr->firstVisible;
	break;

    case TVGN_DROPHILITE:
	retval = infoPtr->dropItem;
	break;

    case TVGN_LASTVISIBLE:
	retval = TREEVIEW_GetLastListItem(infoPtr, infoPtr->root);
	break;
    }

    if (retval)
    {
	TRACE("flags:0x%x, returns %p\n", which, retval);
	return (LRESULT)retval;
    }

    if (item == TVI_ROOT) item = infoPtr->root;

    if (!TREEVIEW_ValidItem(infoPtr, item))
	return FALSE;

    switch (which)
    {
    case TVGN_NEXT:
	retval = item->nextSibling;
	break;
    case TVGN_PREVIOUS:
	retval = item->prevSibling;
	break;
    case TVGN_PARENT:
	retval = (item->parent != infoPtr->root) ? item->parent : NULL;
	break;
    case TVGN_CHILD:
	retval = item->firstChild;
	break;
    case TVGN_NEXTVISIBLE:
	retval = TREEVIEW_GetNextListItem(infoPtr, item);
	break;
    case TVGN_PREVIOUSVISIBLE:
	retval = TREEVIEW_GetPrevListItem(infoPtr, item);
	break;
    default:
	TRACE("Unknown msg 0x%x, item %p\n", which, item);
	break;
    }

    TRACE("flags: 0x%x, item %p;returns %p\n", which, item, retval);
    return (LRESULT)retval;
}


static LRESULT
TREEVIEW_GetCount(const TREEVIEW_INFO *infoPtr)
{
    TRACE(" %d\n", infoPtr->uNumItems);
    return (LRESULT)infoPtr->uNumItems;
}

#ifdef __REACTOS__
static LRESULT
TREEVIEW_SelectItem(TREEVIEW_INFO *infoPtr, INT wParam, HTREEITEM item);
#endif

#ifdef __REACTOS__
static VOID
TREEVIEW_ToggleItemState(TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *item)
#else
static VOID
TREEVIEW_ToggleItemState(const TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *item)
#endif
{
    if (infoPtr->dwStyle & TVS_CHECKBOXES)
    {
	static const unsigned int state_table[] = { 0, 2, 1 };

	unsigned int state;

	state = STATEIMAGEINDEX(item->state);
	TRACE("state: 0x%x\n", state);
	item->state &= ~TVIS_STATEIMAGEMASK;

	if (state < 3)
	    state = state_table[state];

	item->state |= INDEXTOSTATEIMAGEMASK(state);

	TRACE("state: 0x%x\n", state);
#ifdef __REACTOS__
	TREEVIEW_SelectItem(infoPtr, TVGN_CARET, item);
#endif
	TREEVIEW_Invalidate(infoPtr, item);
    }
}


/* Painting *************************************************************/

/* Draw the lines and expand button for an item. Also draws one section
 * of the line from item's parent to item's parent's next sibling. */
static void
TREEVIEW_DrawItemLines(const TREEVIEW_INFO *infoPtr, HDC hdc, const TREEVIEW_ITEM *item)
{
    LONG centerx, centery;
    BOOL lar = ((infoPtr->dwStyle
		 & (TVS_LINESATROOT|TVS_HASLINES|TVS_HASBUTTONS))
		> TVS_LINESATROOT);
    HBRUSH hbr, hbrOld;
    COLORREF clrBk = GETBKCOLOR(infoPtr->clrBk);

    if (!lar && item->iLevel == 0)
	return;

    hbr    = CreateSolidBrush(clrBk);
    hbrOld = SelectObject(hdc, hbr);

    centerx = (item->linesOffset + item->stateOffset) / 2;
    centery = (item->rect.top + item->rect.bottom) / 2;

    if (infoPtr->dwStyle & TVS_HASLINES)
    {
	HPEN hOldPen, hNewPen;
	HTREEITEM parent;
        LOGBRUSH lb;

	/* Get a dotted grey pen */
        lb.lbStyle = BS_SOLID;
        lb.lbColor = GETLINECOLOR(infoPtr->clrLine);
        hNewPen = ExtCreatePen(PS_COSMETIC|PS_ALTERNATE, 1, &lb, 0, NULL);
	hOldPen = SelectObject(hdc, hNewPen);

        /* Make sure the center is on a dot (using +2 instead
         * of +1 gives us pixel-by-pixel compat with native) */
        centery = (centery + 2) & ~1;

	MoveToEx(hdc, item->stateOffset, centery, NULL);
	LineTo(hdc, centerx - 1, centery);

	if (item->prevSibling || item->parent != infoPtr->root)
	{
	    MoveToEx(hdc, centerx, item->rect.top, NULL);
	    LineTo(hdc, centerx, centery);
	}

	if (item->nextSibling)
	{
	    MoveToEx(hdc, centerx, centery, NULL);
	    LineTo(hdc, centerx, item->rect.bottom + 1);
	}

	/* Draw the line from our parent to its next sibling. */
	parent = item->parent;
	while (parent != infoPtr->root)
	{
	    int pcenterx = (parent->linesOffset + parent->stateOffset) / 2;

	    if (parent->nextSibling
		/* skip top-levels unless TVS_LINESATROOT */
		&& parent->stateOffset > parent->linesOffset)
	    {
		MoveToEx(hdc, pcenterx, item->rect.top, NULL);
		LineTo(hdc, pcenterx, item->rect.bottom + 1);
	    }

	    parent = parent->parent;
	}

	SelectObject(hdc, hOldPen);
	DeleteObject(hNewPen);
    }

    /*
     * Display the (+/-) signs
     */

    if (infoPtr->dwStyle & TVS_HASBUTTONS)
    {
	if (item->cChildren)
	{
            HTHEME theme = GetWindowTheme(infoPtr->hwnd);
            if (theme)
            {
                RECT glyphRect = item->rect;
                glyphRect.left = item->linesOffset;
                glyphRect.right = item->stateOffset;
                DrawThemeBackground (theme, hdc, TVP_GLYPH,
                    (item->state & TVIS_EXPANDED) ? GLPS_OPENED : GLPS_CLOSED,
                    &glyphRect, NULL);
            }
            else
            {
                LONG height = item->rect.bottom - item->rect.top;
                LONG width  = item->stateOffset - item->linesOffset;
                LONG rectsize = min(height, width) / 4;
                /* plussize = ceil(rectsize * 3/4) */
                LONG plussize = (rectsize + 1) * 3 / 4;

                HPEN new_pen  = CreatePen(PS_SOLID, 0, GETLINECOLOR(infoPtr->clrLine));
                HPEN old_pen  = SelectObject(hdc, new_pen);

                Rectangle(hdc, centerx - rectsize - 1, centery - rectsize - 1,
                          centerx + rectsize + 2, centery + rectsize + 2);

                SelectObject(hdc, old_pen);
                DeleteObject(new_pen);

                /* draw +/- signs with current text color */
                new_pen = CreatePen(PS_SOLID, 0, GETTXTCOLOR(infoPtr->clrText));
                old_pen = SelectObject(hdc, new_pen);

                if (height < 18 || width < 18)
                {
                    MoveToEx(hdc, centerx - plussize + 1, centery, NULL);
                    LineTo(hdc, centerx + plussize, centery);
    
                    if (!(item->state & TVIS_EXPANDED) ||
                         (item->state & TVIS_EXPANDPARTIAL))
                    {
                        MoveToEx(hdc, centerx, centery - plussize + 1, NULL);
                        LineTo(hdc, centerx, centery + plussize);
                    }
                }
                else
                {
                    Rectangle(hdc, centerx - plussize + 1, centery - 1,
                    centerx + plussize, centery + 2);

                    if (!(item->state & TVIS_EXPANDED) ||
                         (item->state & TVIS_EXPANDPARTIAL))
                    {
                        Rectangle(hdc, centerx - 1, centery - plussize + 1,
                        centerx + 2, centery + plussize);
                        SetPixel(hdc, centerx - 1, centery, clrBk);
                        SetPixel(hdc, centerx + 1, centery, clrBk);
                    }
                }

                SelectObject(hdc, old_pen);
                DeleteObject(new_pen);
            }
	}
    }
    SelectObject(hdc, hbrOld);
    DeleteObject(hbr);
}

static void
TREEVIEW_DrawItem(const TREEVIEW_INFO *infoPtr, HDC hdc, TREEVIEW_ITEM *item)
{
    INT cditem;
    HFONT hOldFont;
    COLORREF oldTextColor, oldTextBkColor;
    int centery;
    BOOL inFocus = (GetFocus() == infoPtr->hwnd);
    NMTVCUSTOMDRAW nmcdhdr;

    TREEVIEW_UpdateDispInfo(infoPtr, item, CALLBACK_MASK_ALL);

    /* - If item is drop target or it is selected and window is in focus -
     * use blue background (COLOR_HIGHLIGHT).
     * - If item is selected, window is not in focus, but it has style
     * TVS_SHOWSELALWAYS - use grey background (COLOR_BTNFACE)
     * - Otherwise - use background color
     */
    if ((item->state & TVIS_DROPHILITED) || ((item == infoPtr->focusedItem) && !(item->state & TVIS_SELECTED)) ||
	((item->state & TVIS_SELECTED) && (!infoPtr->focusedItem || item == infoPtr->focusedItem) &&
	 (inFocus || (infoPtr->dwStyle & TVS_SHOWSELALWAYS))))
    {
	if ((item->state & TVIS_DROPHILITED) || inFocus)
	{
	    nmcdhdr.clrTextBk = comctl32_color.clrHighlight;
	    nmcdhdr.clrText   = comctl32_color.clrHighlightText;
	}
	else
	{
	    nmcdhdr.clrTextBk = comctl32_color.clrBtnFace;
	    nmcdhdr.clrText   = GETTXTCOLOR(infoPtr->clrText);
	}
    }
    else
    {
	nmcdhdr.clrTextBk = GETBKCOLOR(infoPtr->clrBk);
	if ((infoPtr->dwStyle & TVS_TRACKSELECT) && (item == infoPtr->hotItem))
	    nmcdhdr.clrText = comctl32_color.clrHighlight;
	else
	    nmcdhdr.clrText = GETTXTCOLOR(infoPtr->clrText);
    }

    hOldFont = SelectObject(hdc, TREEVIEW_FontForItem(infoPtr, item));
    oldTextColor = SetTextColor(hdc, nmcdhdr.clrText);
    oldTextBkColor = SetBkColor(hdc, nmcdhdr.clrTextBk);

    /* The custom draw handler can query the text rectangle,
     * so get ready. */
    /* should already be known, set to 0 when changed */
    if (!item->textWidth)
        TREEVIEW_ComputeTextWidth(infoPtr, item, hdc);

    cditem = 0;

    if (infoPtr->cdmode & CDRF_NOTIFYITEMDRAW)
    {
	cditem = TREEVIEW_SendCustomDrawItemNotify
	    (infoPtr, hdc, item, CDDS_ITEMPREPAINT, &nmcdhdr);
	TRACE("prepaint:cditem-app returns 0x%x\n", cditem);

	if (cditem & CDRF_SKIPDEFAULT)
	{
	    SelectObject(hdc, hOldFont);
	    return;
	}
    }

    if (cditem & CDRF_NEWFONT)
	TREEVIEW_ComputeTextWidth(infoPtr, item, hdc);

    if (TREEVIEW_IsFullRowSelect(infoPtr))
    {
        HBRUSH brush = CreateSolidBrush(nmcdhdr.clrTextBk);
        FillRect(hdc, &item->rect, brush);
        DeleteObject(brush);
    }

    TREEVIEW_DrawItemLines(infoPtr, hdc, item);

    /* reset colors. Custom draw handler can change them */
    SetTextColor(hdc, nmcdhdr.clrText);
    SetBkColor(hdc, nmcdhdr.clrTextBk);

    centery = (item->rect.top + item->rect.bottom) / 2;

    /*
     * Display the images associated with this item
     */
    {
	INT imageIndex;

	/* State images are displayed to the left of the Normal image
	 * image number is in state; zero should be `display no image'.
	 */
	imageIndex = STATEIMAGEINDEX(item->state);

	if (infoPtr->himlState && imageIndex)
	{
	    ImageList_Draw(infoPtr->himlState, imageIndex, hdc,
			   item->stateOffset,
			   centery - infoPtr->stateImageHeight / 2,
			   ILD_NORMAL);
	}

	/* Now, draw the normal image; can be either selected,
	 * non-selected or expanded image.
	 */

	if ((item->state & TVIS_SELECTED) && (item->iSelectedImage >= 0))
	{
	    /* The item is currently selected */
	    imageIndex = item->iSelectedImage;
	}
	else if ((item->state & TVIS_EXPANDED) && (item->iExpandedImage != (WORD)I_IMAGENONE))
	{
	    /* The item is currently not selected but expanded */
	    imageIndex = item->iExpandedImage;
	}
	else
	{
	    /* The item is not selected and not expanded */
	    imageIndex = item->iImage;
	}

	if (infoPtr->himlNormal)
	{
            UINT style = item->state & TVIS_CUT ? ILD_SELECTED : ILD_NORMAL;

            style |= item->state & TVIS_OVERLAYMASK;

            ImageList_DrawEx(infoPtr->himlNormal, imageIndex, hdc,
                         item->imageOffset, centery - infoPtr->normalImageHeight / 2,
                         0, 0,
                         TREEVIEW_IsFullRowSelect(infoPtr) ? nmcdhdr.clrTextBk : infoPtr->clrBk,
                         item->state & TVIS_CUT ? GETBKCOLOR(infoPtr->clrBk) : CLR_DEFAULT,
                         style);
	}
    }


    /*
     * Display the text associated with this item
     */

    /* Don't paint item's text if it's being edited */
    if (!infoPtr->hwndEdit || (infoPtr->selectedItem != item))
    {
	if (item->pszText)
	{
	    RECT rcText;
	    UINT align;
	    SIZE sz;

	    rcText.top = item->rect.top;
	    rcText.bottom = item->rect.bottom;
	    rcText.left = item->textOffset;
	    rcText.right = rcText.left + item->textWidth + 4;

            TRACE("drawing text %s at (%s)\n",
                  debugstr_w(item->pszText), wine_dbgstr_rect(&rcText));

	    /* Draw it */
	    GetTextExtentPoint32W(hdc, item->pszText, lstrlenW(item->pszText), &sz);

	    align = SetTextAlign(hdc, TA_LEFT | TA_TOP);
	    ExtTextOutW(hdc, rcText.left + 2, (rcText.top + rcText.bottom - sz.cy) / 2,
		        ETO_CLIPPED | ETO_OPAQUE,
			&rcText,
		        item->pszText,
		        lstrlenW(item->pszText),
			NULL);
	    SetTextAlign(hdc, align);

	    /* Draw focus box around the selected item */
	    if ((item == infoPtr->selectedItem) && inFocus)
	    {
		DrawFocusRect(hdc,&rcText);
	    }
	}
    }

    /* Draw insertion mark if necessary */

    if (infoPtr->insertMarkItem)
	TRACE("item:%d,mark:%p\n",
	      TREEVIEW_GetItemIndex(infoPtr, item),
	      infoPtr->insertMarkItem);

    if (item == infoPtr->insertMarkItem)
    {
	HPEN hNewPen, hOldPen;
	int offset;
	int left, right;

	hNewPen = CreatePen(PS_SOLID, 2, GETINSCOLOR(infoPtr->clrInsertMark));
	hOldPen = SelectObject(hdc, hNewPen);

	if (infoPtr->insertBeforeorAfter)
	    offset = item->rect.bottom - 1;
	else
	    offset = item->rect.top + 1;

	left = item->textOffset - 2;
	right = item->textOffset + item->textWidth + 2;

	MoveToEx(hdc, left, offset - 3, NULL);
	LineTo(hdc, left, offset + 4);

	MoveToEx(hdc, left, offset, NULL);
	LineTo(hdc, right + 1, offset);

	MoveToEx(hdc, right, offset + 3, NULL);
	LineTo(hdc, right, offset - 4);

	SelectObject(hdc, hOldPen);
	DeleteObject(hNewPen);
    }

    /* Restore the hdc state */
    SetTextColor(hdc, oldTextColor);
    SetBkColor(hdc, oldTextBkColor);
    SelectObject(hdc, hOldFont);

    if (cditem & CDRF_NOTIFYPOSTPAINT)
    {
	cditem = TREEVIEW_SendCustomDrawItemNotify
	    (infoPtr, hdc, item, CDDS_ITEMPOSTPAINT, &nmcdhdr);
	TRACE("postpaint:cditem-app returns 0x%x\n", cditem);
    }
}

/* Computes treeHeight and treeWidth and updates the scroll bars.
 */
static void
TREEVIEW_UpdateScrollBars(TREEVIEW_INFO *infoPtr)
{
    TREEVIEW_ITEM *item;
    HWND hwnd = infoPtr->hwnd;
    BOOL vert = FALSE;
    BOOL horz = FALSE;
    SCROLLINFO si;
    LONG scrollX = infoPtr->scrollX;

    infoPtr->treeWidth = 0;
    infoPtr->treeHeight = 0;

    /* We iterate through all visible items in order to get the tree height
     * and width */
    item = infoPtr->root->firstChild;

    while (item != NULL)
    {
	if (ISVISIBLE(item))
	{
            /* actually we draw text at textOffset + 2 */
	    if (2+item->textOffset+item->textWidth > infoPtr->treeWidth)
		infoPtr->treeWidth = item->textOffset+item->textWidth+2;

	    /* This is scroll-adjusted, but we fix this below. */
	    infoPtr->treeHeight = item->rect.bottom;
	}

	item = TREEVIEW_GetNextListItem(infoPtr, item);
    }

    /* Fix the scroll adjusted treeHeight and treeWidth. */
    if (infoPtr->root->firstChild)
	infoPtr->treeHeight -= infoPtr->root->firstChild->rect.top;

    infoPtr->treeWidth += infoPtr->scrollX;

    if (infoPtr->dwStyle & TVS_NOSCROLL) return;

    /* Adding one scroll bar may take up enough space that it forces us
     * to add the other as well. */
    if (infoPtr->treeHeight > infoPtr->clientHeight)
    {
	vert = TRUE;

	if (infoPtr->treeWidth
	    > infoPtr->clientWidth - GetSystemMetrics(SM_CXVSCROLL))
	    horz = TRUE;
    }
    else if (infoPtr->treeWidth > infoPtr->clientWidth || infoPtr->scrollX > 0)
	horz = TRUE;

    if (!vert && horz && infoPtr->treeHeight
	> infoPtr->clientHeight - GetSystemMetrics(SM_CYVSCROLL))
	vert = TRUE;

    if (horz && (infoPtr->dwStyle & TVS_NOHSCROLL)) horz = FALSE;

    si.cbSize = sizeof(SCROLLINFO);
    si.fMask  = SIF_POS|SIF_RANGE|SIF_PAGE;
    si.nMin   = 0;

    if (vert)
    {
	si.nPage = TREEVIEW_GetVisibleCount(infoPtr);
       if ( si.nPage && NULL != infoPtr->firstVisible)
       {
           si.nPos  = infoPtr->firstVisible->visibleOrder;
           si.nMax  = infoPtr->maxVisibleOrder - 1;

           SetScrollInfo(hwnd, SB_VERT, &si, TRUE);

           if (!(infoPtr->uInternalStatus & TV_VSCROLL))
               ShowScrollBar(hwnd, SB_VERT, TRUE);
           infoPtr->uInternalStatus |= TV_VSCROLL;
       }
       else
       {
           if (infoPtr->uInternalStatus & TV_VSCROLL)
               ShowScrollBar(hwnd, SB_VERT, FALSE);
           infoPtr->uInternalStatus &= ~TV_VSCROLL;
       }
    }
    else
    {
	if (infoPtr->uInternalStatus & TV_VSCROLL)
	    ShowScrollBar(hwnd, SB_VERT, FALSE);
	infoPtr->uInternalStatus &= ~TV_VSCROLL;
    }

    if (horz)
    {
	si.nPage = infoPtr->clientWidth;
	si.nPos  = infoPtr->scrollX;
	si.nMax  = infoPtr->treeWidth - 1;

	if (si.nPos > si.nMax - max( si.nPage-1, 0 ))
        {
           si.nPos = si.nMax - max( si.nPage-1, 0 );
           scrollX = si.nPos;
        }

	if (!(infoPtr->uInternalStatus & TV_HSCROLL))
	    ShowScrollBar(hwnd, SB_HORZ, TRUE);
	infoPtr->uInternalStatus |= TV_HSCROLL;

	SetScrollInfo(hwnd, SB_HORZ, &si, TRUE);
	TREEVIEW_HScroll(infoPtr,
	                MAKEWPARAM(SB_THUMBPOSITION, scrollX));
    }
    else
    {
	if (infoPtr->uInternalStatus & TV_HSCROLL)
	    ShowScrollBar(hwnd, SB_HORZ, FALSE);
	infoPtr->uInternalStatus &= ~TV_HSCROLL;

	scrollX = 0;
        if (infoPtr->scrollX != 0)
        {
	    TREEVIEW_HScroll(infoPtr,
	                    MAKEWPARAM(SB_THUMBPOSITION, scrollX));
        }
    }

    if (!horz)
	infoPtr->uInternalStatus &= ~TV_HSCROLL;
}

static void
TREEVIEW_FillBkgnd(const TREEVIEW_INFO *infoPtr, HDC hdc, const RECT *rc)
{
    HBRUSH hBrush;
    COLORREF clrBk = GETBKCOLOR(infoPtr->clrBk);

    hBrush =  CreateSolidBrush(clrBk);
    FillRect(hdc, rc, hBrush);
    DeleteObject(hBrush);
}

/* CtrlSpy doesn't mention this, but CorelDRAW's object manager needs it. */
static LRESULT
TREEVIEW_EraseBackground(const TREEVIEW_INFO *infoPtr, HDC hdc)
{
    RECT rect;

    TRACE("%p\n", infoPtr);

    GetClientRect(infoPtr->hwnd, &rect);
    TREEVIEW_FillBkgnd(infoPtr, hdc, &rect);

    return 1;
}

static void
TREEVIEW_Refresh(TREEVIEW_INFO *infoPtr, HDC hdc, const RECT *rc)
{
    HWND hwnd = infoPtr->hwnd;
    RECT rect = *rc;
    TREEVIEW_ITEM *item;

    if (infoPtr->clientHeight == 0 || infoPtr->clientWidth == 0)
    {
	TRACE("empty window\n");
	return;
    }

    infoPtr->cdmode = TREEVIEW_SendCustomDrawNotify(infoPtr, CDDS_PREPAINT,
						    hdc, rect);

    if (infoPtr->cdmode == CDRF_SKIPDEFAULT)
    {
	ReleaseDC(hwnd, hdc);
	return;
    }

    for (item = infoPtr->root->firstChild;
         item != NULL;
         item = TREEVIEW_GetNextListItem(infoPtr, item))
    {
	if (ISVISIBLE(item))
	{
            /* Avoid unneeded calculations */
            if (item->rect.top > rect.bottom)
                break;
            if (item->rect.bottom < rect.top)
                continue;

	    TREEVIEW_DrawItem(infoPtr, hdc, item);
	}
    }

    //
    // FIXME: This is correct, but is causes and infinite loop of WM_PAINT
    // messages, resulting in continuous painting of the scroll bar in reactos.
    // Comment out until the real bug is found. CORE-4912
    //
#ifndef __REACTOS__
    TREEVIEW_UpdateScrollBars(infoPtr);
#endif

    if (infoPtr->cdmode & CDRF_NOTIFYPOSTPAINT)
	infoPtr->cdmode =
	    TREEVIEW_SendCustomDrawNotify(infoPtr, CDDS_POSTPAINT, hdc, rect);
}

static inline void
TREEVIEW_InvalidateItem(const TREEVIEW_INFO *infoPtr, const TREEVIEW_ITEM *item)
{
    if (item) InvalidateRect(infoPtr->hwnd, &item->rect, TRUE);
}

static void
TREEVIEW_Invalidate(const TREEVIEW_INFO *infoPtr, const TREEVIEW_ITEM *item)
{
    if (item)
	InvalidateRect(infoPtr->hwnd, &item->rect, TRUE);
    else
        InvalidateRect(infoPtr->hwnd, NULL, TRUE);
}

static void
TREEVIEW_InitCheckboxes(TREEVIEW_INFO *infoPtr)
{
    RECT rc;
    HBITMAP hbm, hbmOld;
    HDC hdc, hdcScreen;
    int nIndex;

    infoPtr->himlState = ImageList_Create(16, 16, ILC_COLOR | ILC_MASK, 3, 0);

    hdcScreen = GetDC(0);

    hdc = CreateCompatibleDC(hdcScreen);
    hbm = CreateCompatibleBitmap(hdcScreen, 48, 16);
    hbmOld = SelectObject(hdc, hbm);

    SetRect(&rc, 0, 0, 48, 16);
    FillRect(hdc, &rc, (HBRUSH)(COLOR_WINDOW+1));

    SetRect(&rc, 18, 2, 30, 14);
    DrawFrameControl(hdc, &rc, DFC_BUTTON,
                     DFCS_BUTTONCHECK|DFCS_FLAT);

    SetRect(&rc, 34, 2, 46, 14);
    DrawFrameControl(hdc, &rc, DFC_BUTTON,
                     DFCS_BUTTONCHECK|DFCS_FLAT|DFCS_CHECKED);

    SelectObject(hdc, hbmOld);
    nIndex = ImageList_AddMasked(infoPtr->himlState, hbm,
                                 comctl32_color.clrWindow);
    TRACE("checkbox index %d\n", nIndex);

    DeleteObject(hbm);
    DeleteDC(hdc);
    ReleaseDC(0, hdcScreen);

    infoPtr->stateImageWidth = 16;
    infoPtr->stateImageHeight = 16;
}

static void
TREEVIEW_ResetImageStateIndex(TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *item)
{
    TREEVIEW_ITEM *child = item->firstChild;

    item->state &= ~TVIS_STATEIMAGEMASK;
    item->state |= INDEXTOSTATEIMAGEMASK(1);

    while (child)
    {
        TREEVIEW_ITEM *next = child->nextSibling;
        TREEVIEW_ResetImageStateIndex(infoPtr, child);
        child = next;
    }
}

static LRESULT
TREEVIEW_Paint(TREEVIEW_INFO *infoPtr, HDC hdc_ref)
{
    HDC hdc;
    PAINTSTRUCT ps;
    RECT rc;

    TRACE("(%p %p)\n", infoPtr, hdc_ref);

    if ((infoPtr->dwStyle & TVS_CHECKBOXES) && !infoPtr->himlState)
    {
        TREEVIEW_InitCheckboxes(infoPtr);
        TREEVIEW_ResetImageStateIndex(infoPtr, infoPtr->root);

        TREEVIEW_EndEditLabelNow(infoPtr, TRUE);
        TREEVIEW_UpdateSubTree(infoPtr, infoPtr->root);
        TREEVIEW_UpdateScrollBars(infoPtr);
        TREEVIEW_Invalidate(infoPtr, NULL);
    }

    if (hdc_ref)
    {
        hdc = hdc_ref;
        GetClientRect(infoPtr->hwnd, &rc);
        TREEVIEW_FillBkgnd(infoPtr, hdc, &rc);
    }
    else
    {
        hdc = BeginPaint(infoPtr->hwnd, &ps);
        rc  = ps.rcPaint;
        if(ps.fErase)
            TREEVIEW_FillBkgnd(infoPtr, hdc, &rc);
    }

    if(infoPtr->bRedraw) /* WM_SETREDRAW sets bRedraw */
        TREEVIEW_Refresh(infoPtr, hdc, &rc);

    if (!hdc_ref)
	EndPaint(infoPtr->hwnd, &ps);

    return 0;
}

static LRESULT
TREEVIEW_PrintClient(TREEVIEW_INFO *infoPtr, HDC hdc, DWORD options)
{
    FIXME("Partial Stub: (hdc=%p options=0x%08x)\n", hdc, options);

    if ((options & PRF_CHECKVISIBLE) && !IsWindowVisible(infoPtr->hwnd))
        return 0;

    if (options & PRF_ERASEBKGND)
        TREEVIEW_EraseBackground(infoPtr, hdc);

    if (options & PRF_CLIENT)
    {
        RECT rc;
        GetClientRect(infoPtr->hwnd, &rc);
        TREEVIEW_Refresh(infoPtr, hdc, &rc);
    }

    return 0;
}

/* Sorting **************************************************************/

/***************************************************************************
 * Forward the DPA local callback to the treeview owner callback
 */
static INT WINAPI
TREEVIEW_CallBackCompare(const TREEVIEW_ITEM *first, const TREEVIEW_ITEM *second,
                         const TVSORTCB *pCallBackSort)
{
    /* Forward the call to the client-defined callback */
    return pCallBackSort->lpfnCompare(first->lParam,
				      second->lParam,
				      pCallBackSort->lParam);
}

/***************************************************************************
 * Treeview native sort routine: sort on item text.
 */
static INT WINAPI
TREEVIEW_SortOnName(TREEVIEW_ITEM *first, TREEVIEW_ITEM *second,
                    const TREEVIEW_INFO *infoPtr)
{
    TREEVIEW_UpdateDispInfo(infoPtr, first, TVIF_TEXT);
    TREEVIEW_UpdateDispInfo(infoPtr, second, TVIF_TEXT);

    if(first->pszText && second->pszText)
        return lstrcmpiW(first->pszText, second->pszText);
    else if(first->pszText)
        return -1;
    else if(second->pszText)
        return 1;
    else
        return 0;
}

/* Returns the number of physical children belonging to item. */
static INT
TREEVIEW_CountChildren(const TREEVIEW_ITEM *item)
{
    INT cChildren = 0;
    HTREEITEM hti;

    for (hti = item->firstChild; hti != NULL; hti = hti->nextSibling)
	cChildren++;

    return cChildren;
}

/* Returns a DPA containing a pointer to each physical child of item in
 * sibling order. If item has no children, an empty DPA is returned. */
static HDPA
TREEVIEW_BuildChildDPA(const TREEVIEW_ITEM *item)
{
    HTREEITEM child;

    HDPA list = DPA_Create(8);
    if (list == 0) return NULL;

    for (child = item->firstChild; child != NULL; child = child->nextSibling)
    {
	if (DPA_InsertPtr(list, INT_MAX, child) == -1)
	{
	    DPA_Destroy(list);
	    return NULL;
	}
    }

    return list;
}

/***************************************************************************
 * Setup the treeview structure with regards of the sort method
 * and sort the children of the TV item specified in lParam
 * fRecurse: currently unused. Should be zero.
 * parent: if pSort!=NULL, should equal pSort->hParent.
 *         otherwise, item which child items are to be sorted.
 * pSort:  sort method info. if NULL, sort on item text.
 *         if non-NULL, sort on item's lParam content, and let the
 *         application decide what that means. See also TVM_SORTCHILDRENCB.
 */

static LRESULT
TREEVIEW_Sort(TREEVIEW_INFO *infoPtr, HTREEITEM parent,
	      LPTVSORTCB pSort)
{
    INT cChildren;
    PFNDPACOMPARE pfnCompare;
    LPARAM lpCompare;

    /* undocumented feature: TVI_ROOT or NULL means `sort the whole tree' */
    if (parent == TVI_ROOT || parent == NULL)
	parent = infoPtr->root;

    /* Check for a valid handle to the parent item */
    if (!TREEVIEW_ValidItem(infoPtr, parent))
    {
	ERR("invalid item hParent=%p\n", parent);
	return FALSE;
    }

    if (pSort)
    {
	pfnCompare = (PFNDPACOMPARE)TREEVIEW_CallBackCompare;
	lpCompare = (LPARAM)pSort;
    }
    else
    {
	pfnCompare = (PFNDPACOMPARE)TREEVIEW_SortOnName;
	lpCompare = (LPARAM)infoPtr;
    }

    cChildren = TREEVIEW_CountChildren(parent);

    /* Make sure there is something to sort */
    if (cChildren > 1)
    {
	/* TREEVIEW_ITEM rechaining */
	INT count = 0;
	HTREEITEM item = 0;
	HTREEITEM nextItem = 0;
	HTREEITEM prevItem = 0;

	HDPA sortList = TREEVIEW_BuildChildDPA(parent);

	if (sortList == NULL)
	    return FALSE;

	/* let DPA sort the list */
	DPA_Sort(sortList, pfnCompare, lpCompare);

	/* The order of DPA entries has been changed, so fixup the
	 * nextSibling and prevSibling pointers. */

        item = DPA_GetPtr(sortList, count++);
        while ((nextItem = DPA_GetPtr(sortList, count++)) != NULL)
	{
	    /* link the two current item together */
	    item->nextSibling = nextItem;
	    nextItem->prevSibling = item;

	    if (prevItem == NULL)
	    {
		/* this is the first item, update the parent */
		parent->firstChild = item;
		item->prevSibling = NULL;
	    }
	    else
	    {
		/* fix the back chaining */
		item->prevSibling = prevItem;
	    }

	    /* get ready for the next one */
	    prevItem = item;
	    item = nextItem;
	}

	/* the last item is pointed to by item and never has a sibling */
	item->nextSibling = NULL;
	parent->lastChild = item;

	DPA_Destroy(sortList);

	TREEVIEW_VerifyTree(infoPtr);

	if (parent->state & TVIS_EXPANDED)
	{
	    int visOrder = infoPtr->firstVisible->visibleOrder;

	    if (parent == infoPtr->root)
	        TREEVIEW_RecalculateVisibleOrder(infoPtr, NULL);
	    else
	        TREEVIEW_RecalculateVisibleOrder(infoPtr, parent);

	    if (TREEVIEW_IsChildOf(parent, infoPtr->firstVisible))
	    {
	        TREEVIEW_ITEM *item;

	        for (item = infoPtr->root->firstChild; item != NULL;
	             item = TREEVIEW_GetNextListItem(infoPtr, item))
	        {
	            if (item->visibleOrder == visOrder)
	                break;
	        }

                if (!item) item = parent->firstChild;
                TREEVIEW_SetFirstVisible(infoPtr, item, FALSE);
	    }

	    TREEVIEW_Invalidate(infoPtr, NULL);
	}

	return TRUE;
    }
    return FALSE;
}


/***************************************************************************
 * Setup the treeview structure with regards of the sort method
 * and sort the children of the TV item specified in lParam
 */
static LRESULT
TREEVIEW_SortChildrenCB(TREEVIEW_INFO *infoPtr, LPTVSORTCB pSort)
{
    return TREEVIEW_Sort(infoPtr, pSort->hParent, pSort);
}


/***************************************************************************
 * Sort the children of the TV item specified in lParam.
 */
static LRESULT
TREEVIEW_SortChildren(TREEVIEW_INFO *infoPtr, LPARAM lParam)
{
    return TREEVIEW_Sort(infoPtr, (HTREEITEM)lParam, NULL);
}


/* Expansion/Collapse ***************************************************/

static BOOL
TREEVIEW_SendExpanding(const TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *item,
		       UINT action)
{
    return !TREEVIEW_SendTreeviewNotify(infoPtr, TVN_ITEMEXPANDINGW, action,
					TVIF_HANDLE | TVIF_STATE | TVIF_PARAM
					| TVIF_IMAGE | TVIF_SELECTEDIMAGE,
					0, item);
}

static VOID
TREEVIEW_SendExpanded(const TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *item,
		      UINT action)
{
    TREEVIEW_SendTreeviewNotify(infoPtr, TVN_ITEMEXPANDEDW, action,
				TVIF_HANDLE | TVIF_STATE | TVIF_PARAM
				| TVIF_IMAGE | TVIF_SELECTEDIMAGE,
				0, item);
}


/* This corresponds to TVM_EXPAND with TVE_COLLAPSE.
 * bRemoveChildren corresponds to TVE_COLLAPSERESET. */
static BOOL
TREEVIEW_Collapse(TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *item,
		  BOOL bRemoveChildren, BOOL bUser)
{
    UINT action = TVE_COLLAPSE | (bRemoveChildren ? TVE_COLLAPSERESET : 0);
    BOOL bSetSelection, bSetFirstVisible;
    RECT scrollRect;
    LONG scrollDist = 0;
    TREEVIEW_ITEM *nextItem = NULL, *tmpItem;
    BOOL wasExpanded;

    TRACE("TVE_COLLAPSE %p %s\n", item, TREEVIEW_ItemName(item));

    if (!TREEVIEW_HasChildren(infoPtr, item))
	return FALSE;

    if (bUser)
	TREEVIEW_SendExpanding(infoPtr, item, action);

    if (item->firstChild == NULL)
	return FALSE;

    wasExpanded = (item->state & TVIS_EXPANDED) != 0;
    item->state &= ~TVIS_EXPANDED;

    if (wasExpanded && bUser)
	TREEVIEW_SendExpanded(infoPtr, item, action);

    bSetSelection = (infoPtr->selectedItem != NULL
		     && TREEVIEW_IsChildOf(item, infoPtr->selectedItem));

    bSetFirstVisible = (infoPtr->firstVisible != NULL
                        && TREEVIEW_IsChildOf(item, infoPtr->firstVisible));

    tmpItem = item;
    while (tmpItem)
    {
        if (tmpItem->nextSibling)
        {
            nextItem = tmpItem->nextSibling;
            break;
        }
        tmpItem = tmpItem->parent;
    }

    if (nextItem)
        scrollDist = nextItem->rect.top;

    if (bRemoveChildren)
    {
        INT old_cChildren = item->cChildren;
	TRACE("TVE_COLLAPSERESET\n");
	item->state &= ~TVIS_EXPANDEDONCE;
	TREEVIEW_RemoveAllChildren(infoPtr, item);
        item->cChildren = old_cChildren;
    }
    if (!wasExpanded)
        return FALSE;

    if (item->firstChild)
    {
        TREEVIEW_ITEM *i, *sibling;

	sibling = TREEVIEW_GetNextListItem(infoPtr, item);

	for (i = item->firstChild; i != sibling;
	     i = TREEVIEW_GetNextListItem(infoPtr, i))
	{
	    i->visibleOrder = -1;
	}
    }

    TREEVIEW_RecalculateVisibleOrder(infoPtr, item);

    if (nextItem)
        scrollDist = -(scrollDist - nextItem->rect.top);

    if (bSetSelection)
    {
	/* Don't call DoSelectItem, it sends notifications. */
	if (TREEVIEW_ValidItem(infoPtr, infoPtr->selectedItem))
	    infoPtr->selectedItem->state &= ~TVIS_SELECTED;
	item->state |= TVIS_SELECTED;
	infoPtr->selectedItem = item;
    }

    TREEVIEW_UpdateScrollBars(infoPtr);

    scrollRect.left = 0;
    scrollRect.right = infoPtr->clientWidth;
    scrollRect.bottom = infoPtr->clientHeight;

    if (nextItem)
    {
        scrollRect.top = nextItem->rect.top;

        ScrollWindowEx (infoPtr->hwnd, 0, scrollDist, &scrollRect, &scrollRect,
                       NULL, NULL, SW_ERASE | SW_INVALIDATE);
        TREEVIEW_Invalidate(infoPtr, item);
    } else {
        scrollRect.top = item->rect.top;
        InvalidateRect(infoPtr->hwnd, &scrollRect, TRUE);
    }

    TREEVIEW_SetFirstVisible(infoPtr,
                             bSetFirstVisible ? item : infoPtr->firstVisible,
                             TRUE);

    return wasExpanded;
}

static BOOL
TREEVIEW_Expand(TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *item,
		BOOL partial, BOOL user)
{
    LONG scrollDist;
    LONG orgNextTop = 0;
    RECT scrollRect;
    TREEVIEW_ITEM *nextItem, *tmpItem;
    BOOL sendsNotifications;

    TRACE("(%p, %p, partial=%d, %d)\n", infoPtr, item, partial, user);

    if (!TREEVIEW_HasChildren(infoPtr, item))
	return FALSE;

    tmpItem = item; nextItem = NULL;
    while (tmpItem)
    {
        if (tmpItem->nextSibling)
        {
            nextItem = tmpItem->nextSibling;
            break;
        }
        tmpItem = tmpItem->parent;
    }

    if (nextItem)
        orgNextTop = nextItem->rect.top;

    TRACE("TVE_EXPAND %p %s\n", item, TREEVIEW_ItemName(item));

    sendsNotifications = user || ((item->cChildren != 0) &&
                                    !(item->state & TVIS_EXPANDEDONCE));
    if (sendsNotifications)
    {
	if (!TREEVIEW_SendExpanding(infoPtr, item, TVE_EXPAND))
	{
	    TRACE("  TVN_ITEMEXPANDING returned TRUE, exiting...\n");
	    return FALSE;
	}
    }
    if (!item->firstChild)
        return FALSE;

    item->state |= TVIS_EXPANDED;

    if (partial)
	FIXME("TVE_EXPANDPARTIAL not implemented\n");

    if (ISVISIBLE(item))
    {
        TREEVIEW_RecalculateVisibleOrder(infoPtr, item);
        TREEVIEW_UpdateSubTree(infoPtr, item);
        TREEVIEW_UpdateScrollBars(infoPtr);

        scrollRect.left = 0;
        scrollRect.bottom = infoPtr->treeHeight;
        scrollRect.right = infoPtr->clientWidth;
        if (nextItem)
        {
            scrollDist = nextItem->rect.top - orgNextTop;
            scrollRect.top = orgNextTop;

            ScrollWindowEx (infoPtr->hwnd, 0, scrollDist, &scrollRect, NULL,
                        NULL, NULL, SW_ERASE | SW_INVALIDATE);
            TREEVIEW_Invalidate (infoPtr, item);
        } else {
            scrollRect.top = item->rect.top;
            InvalidateRect(infoPtr->hwnd, &scrollRect, FALSE);
        }

        /* Scroll up so that as many children as possible are visible.
        * This fails when expanding causes an HScroll bar to appear, but we
        * don't know that yet, so the last item is obscured. */
        if (item->firstChild != NULL)
        {
            int nChildren = item->lastChild->visibleOrder
                - item->firstChild->visibleOrder + 1;

            int visible_pos = item->visibleOrder
                - infoPtr->firstVisible->visibleOrder;

            int rows_below = TREEVIEW_GetVisibleCount(infoPtr) - visible_pos - 1;

            if (visible_pos > 0 && nChildren > rows_below)
            {
                int scroll = nChildren - rows_below;

                if (scroll > visible_pos)
                    scroll = visible_pos;

                if (scroll > 0)
                {
                    TREEVIEW_ITEM *newFirstVisible
                        = TREEVIEW_GetListItem(infoPtr, infoPtr->firstVisible,
                                            scroll);


                    TREEVIEW_SetFirstVisible(infoPtr, newFirstVisible, TRUE);
                }
            }
        }
    }

    if (sendsNotifications) {
        TREEVIEW_SendExpanded(infoPtr, item, TVE_EXPAND);
        item->state |= TVIS_EXPANDEDONCE;
    }

    return TRUE;
}

/* Handler for TVS_SINGLEEXPAND behaviour. Used on response
   to mouse messages and TVM_SELECTITEM.

   selection - previously selected item, used to collapse a part of a tree
   item - new selected item
*/
static void TREEVIEW_SingleExpand(TREEVIEW_INFO *infoPtr,
    HTREEITEM selection, HTREEITEM item)
{
    TREEVIEW_ITEM *prev, *curr;

    if ((infoPtr->dwStyle & TVS_SINGLEEXPAND) == 0 || infoPtr->hwndEdit || !item) return;

    TREEVIEW_SendTreeviewNotify(infoPtr, TVN_SINGLEEXPAND, TVC_UNKNOWN, TVIF_HANDLE | TVIF_PARAM, item, 0);

    /*
     * Close the previous item and its ancestors as long as they are not
     * ancestors of the current item
     */
    for (prev = selection; prev && TREEVIEW_ValidItem(infoPtr, prev); prev = prev->parent)
    {
        for (curr = item; curr && TREEVIEW_ValidItem(infoPtr, curr); curr = curr->parent)
        {
            if (curr == prev)
                goto finish;
        }
        TREEVIEW_Collapse(infoPtr, prev, FALSE, TRUE);
    }

finish:
    /*
     * Expand the current item
     */
    TREEVIEW_Expand(infoPtr, item, FALSE, TRUE);
}

static BOOL
TREEVIEW_Toggle(TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *item, BOOL user)
{
    TRACE("item=%p, user=%d\n", item, user);

    if (item->state & TVIS_EXPANDED)
	return TREEVIEW_Collapse(infoPtr, item, FALSE, user);
    else
	return TREEVIEW_Expand(infoPtr, item, FALSE, user);
}

static VOID
TREEVIEW_ExpandAll(TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *item)
{
    TREEVIEW_Expand(infoPtr, item, FALSE, TRUE);

    for (item = item->firstChild; item != NULL; item = item->nextSibling)
    {
	if (TREEVIEW_HasChildren(infoPtr, item))
	    TREEVIEW_ExpandAll(infoPtr, item);
    }
}

/* Note:If the specified item is the child of a collapsed parent item,
   the parent's list of child items is (recursively) expanded to reveal the
   specified item. This is mentioned for TREEVIEW_SelectItem; don't
   know if it also applies here.
*/

static LRESULT
TREEVIEW_ExpandMsg(TREEVIEW_INFO *infoPtr, UINT flag, HTREEITEM item)
{
    if (!TREEVIEW_ValidItem(infoPtr, item))
	return 0;

    TRACE("For (%s) item:%d, flags 0x%x, state:%d\n",
	      TREEVIEW_ItemName(item), TREEVIEW_GetItemIndex(infoPtr, item),
              flag, item->state);

    switch (flag & TVE_TOGGLE)
    {
    case TVE_COLLAPSE:
	return TREEVIEW_Collapse(infoPtr, item, flag & TVE_COLLAPSERESET,
				 FALSE);

    case TVE_EXPAND:
	return TREEVIEW_Expand(infoPtr, item, flag & TVE_EXPANDPARTIAL,
			       FALSE);

    case TVE_TOGGLE:
	return TREEVIEW_Toggle(infoPtr, item, FALSE);

    default:
	return 0;
    }
}

/* Hit-Testing **********************************************************/

static TREEVIEW_ITEM *
TREEVIEW_HitTestPoint(const TREEVIEW_INFO *infoPtr, POINT pt)
{
    TREEVIEW_ITEM *item;
    LONG row;

    if (!infoPtr->firstVisible)
	return NULL;

    row = pt.y / infoPtr->uItemHeight + infoPtr->firstVisible->visibleOrder;

    for (item = infoPtr->firstVisible; item != NULL;
	 item = TREEVIEW_GetNextListItem(infoPtr, item))
    {
	if (row >= item->visibleOrder
	    && row < item->visibleOrder + item->iIntegral)
	    break;
    }

    return item;
}

static TREEVIEW_ITEM *
TREEVIEW_HitTest(const TREEVIEW_INFO *infoPtr, LPTVHITTESTINFO lpht)
{
    TREEVIEW_ITEM *item;
    RECT rect;
    UINT status;
    LONG x, y;

    lpht->hItem = 0;
    GetClientRect(infoPtr->hwnd, &rect);
    status = 0;
    x = lpht->pt.x;
    y = lpht->pt.y;

    if (x < rect.left)
    {
	status |= TVHT_TOLEFT;
    }
    else if (x > rect.right)
    {
	status |= TVHT_TORIGHT;
    }

    if (y < rect.top)
    {
	status |= TVHT_ABOVE;
    }
    else if (y > rect.bottom)
    {
	status |= TVHT_BELOW;
    }

    if (status)
    {
	lpht->flags = status;
        return NULL;
    }

    item = TREEVIEW_HitTestPoint(infoPtr, lpht->pt);
    if (!item)
    {
	lpht->flags = TVHT_NOWHERE;
        return NULL;
    }

    if (!item->textWidth)
        TREEVIEW_ComputeTextWidth(infoPtr, item, 0);

    if (x >= item->textOffset + item->textWidth)
    {
	lpht->flags = TVHT_ONITEMRIGHT;
    }
    else if (x >= item->textOffset)
    {
	lpht->flags = TVHT_ONITEMLABEL;
    }
    else if (x >= item->imageOffset)
    {
	lpht->flags = TVHT_ONITEMICON;
    }
    else if (x >= item->stateOffset)
    {
	lpht->flags = TVHT_ONITEMSTATEICON;
    }
    else if (x >= item->linesOffset && infoPtr->dwStyle & TVS_HASBUTTONS)
    {
	lpht->flags = TVHT_ONITEMBUTTON;
    }
    else
    {
	lpht->flags = TVHT_ONITEMINDENT;
    }

    lpht->hItem = item;
    TRACE("(%d,%d):result 0x%x\n", lpht->pt.x, lpht->pt.y, lpht->flags);

    return item;
}

/* Item Label Editing ***************************************************/

static LRESULT
TREEVIEW_GetEditControl(const TREEVIEW_INFO *infoPtr)
{
    return (LRESULT)infoPtr->hwndEdit;
}

static LRESULT CALLBACK
TREEVIEW_Edit_SubclassProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(GetParent(hwnd));
    BOOL bCancel = FALSE;
    LRESULT rc;

    switch (uMsg)
    {
    case WM_PAINT:
	 TRACE("WM_PAINT start\n");
	 rc = CallWindowProcW(infoPtr->wpEditOrig, hwnd, uMsg, wParam,
				 lParam);
	 TRACE("WM_PAINT done\n");
	 return rc;

    case WM_KILLFOCUS:
	if (infoPtr->bIgnoreEditKillFocus)
	    return TRUE;
	break;

    case WM_DESTROY:
    {
	WNDPROC editProc = infoPtr->wpEditOrig;
	infoPtr->wpEditOrig = 0;
	SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (DWORD_PTR)editProc);
	return CallWindowProcW(editProc, hwnd, uMsg, wParam, lParam);
    }

    case WM_GETDLGCODE:
	return DLGC_WANTARROWS | DLGC_WANTALLKEYS;

    case WM_KEYDOWN:
       if (wParam == VK_ESCAPE)
	{
	    bCancel = TRUE;
	    break;
	}
       else if (wParam == VK_RETURN)
	{
	    break;
	}

	/* fall through */
    default:
	return CallWindowProcW(infoPtr->wpEditOrig, hwnd, uMsg, wParam, lParam);
    }

    /* Processing TVN_ENDLABELEDIT message could kill the focus       */
    /* eg. Using a messagebox                                         */

    infoPtr->bIgnoreEditKillFocus = TRUE;
    TREEVIEW_EndEditLabelNow(infoPtr, bCancel || !infoPtr->bLabelChanged);
    infoPtr->bIgnoreEditKillFocus = FALSE;

    return 0;
}


/* should handle edit control messages here */

static LRESULT
TREEVIEW_Command(TREEVIEW_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    TRACE("code=0x%x, id=0x%x, handle=0x%lx\n", HIWORD(wParam), LOWORD(wParam), lParam);

    switch (HIWORD(wParam))
    {
    case EN_UPDATE:
	{
	    /*
	     * Adjust the edit window size
	     */
	    WCHAR buffer[1024];
	    TREEVIEW_ITEM *editItem = infoPtr->editItem;
	    HDC hdc = GetDC(infoPtr->hwndEdit);
	    SIZE sz;
	    HFONT hFont, hOldFont = 0;

	    TRACE("edit=%p\n", infoPtr->hwndEdit);

	    if (!IsWindow(infoPtr->hwndEdit) || !hdc) return FALSE;

	    infoPtr->bLabelChanged = TRUE;

	    GetWindowTextW(infoPtr->hwndEdit, buffer, ARRAY_SIZE(buffer));

	    /* Select font to get the right dimension of the string */
	    hFont = (HFONT)SendMessageW(infoPtr->hwndEdit, WM_GETFONT, 0, 0);

	    if (hFont != 0)
	    {
		hOldFont = SelectObject(hdc, hFont);
	    }

	    if (GetTextExtentPoint32W(hdc, buffer, lstrlenW(buffer), &sz))
	    {
		TEXTMETRICW textMetric;

		/* Add Extra spacing for the next character */
		GetTextMetricsW(hdc, &textMetric);
		sz.cx += (textMetric.tmMaxCharWidth * 2);

		sz.cx = max(sz.cx, textMetric.tmMaxCharWidth * 3);
		sz.cx = min(sz.cx,
			    infoPtr->clientWidth - editItem->textOffset + 2);

		SetWindowPos(infoPtr->hwndEdit,
			     HWND_TOP,
			     0,
			     0,
			     sz.cx,
			     editItem->rect.bottom - editItem->rect.top + 3,
			     SWP_NOMOVE | SWP_DRAWFRAME);
	    }

	    if (hFont != 0)
	    {
		SelectObject(hdc, hOldFont);
	    }

	    ReleaseDC(infoPtr->hwnd, hdc);
	    break;
	}
    case EN_KILLFOCUS:
	/* apparently we should respect passed handle value */
	if (infoPtr->hwndEdit != (HWND)lParam) return FALSE;

	TREEVIEW_EndEditLabelNow(infoPtr, FALSE);
	break;

    default:
	return SendMessageW(infoPtr->hwndNotify, WM_COMMAND, wParam, lParam);
    }

    return 0;
}

static HWND
TREEVIEW_EditLabel(TREEVIEW_INFO *infoPtr, HTREEITEM hItem)
{
    HWND hwnd = infoPtr->hwnd;
    HWND hwndEdit;
    SIZE sz;
    HINSTANCE hinst = (HINSTANCE)GetWindowLongPtrW(hwnd, GWLP_HINSTANCE);
    HDC hdc;
    HFONT hOldFont=0;
    TEXTMETRICW textMetric;

    TRACE("%p %p\n", hwnd, hItem);
    if (!(infoPtr->dwStyle & TVS_EDITLABELS))
        return NULL;

    if (!TREEVIEW_ValidItem(infoPtr, hItem))
	return NULL;

    if (infoPtr->hwndEdit)
	return infoPtr->hwndEdit;

    infoPtr->bLabelChanged = FALSE;

    /* make edit item visible */
    TREEVIEW_EnsureVisible(infoPtr, hItem, TRUE);

    TREEVIEW_UpdateDispInfo(infoPtr, hItem, TVIF_TEXT);

    hdc = GetDC(hwnd);
    /* Select the font to get appropriate metric dimensions */
    if (infoPtr->hFont != 0)
    {
	hOldFont = SelectObject(hdc, infoPtr->hFont);
    }

    /* Get string length in pixels */
    if (hItem->pszText)
        GetTextExtentPoint32W(hdc, hItem->pszText, lstrlenW(hItem->pszText),
                        &sz);
    else
        GetTextExtentPoint32A(hdc, "", 0, &sz);

    /* Add Extra spacing for the next character */
    GetTextMetricsW(hdc, &textMetric);
    sz.cx += (textMetric.tmMaxCharWidth * 2);

    sz.cx = max(sz.cx, textMetric.tmMaxCharWidth * 3);
    sz.cx = min(sz.cx, infoPtr->clientWidth - hItem->textOffset + 2);

    if (infoPtr->hFont != 0)
    {
	SelectObject(hdc, hOldFont);
    }

    ReleaseDC(hwnd, hdc);

    infoPtr->editItem = hItem;

    hwndEdit = CreateWindowExW(WS_EX_LEFT,
			       WC_EDITW,
			       0,
			       WS_CHILD | WS_BORDER | ES_AUTOHSCROLL |
			       WS_CLIPSIBLINGS | ES_WANTRETURN |
			       ES_LEFT, hItem->textOffset - 2,
			       hItem->rect.top - 1, sz.cx + 3,
			       hItem->rect.bottom -
			       hItem->rect.top + 3, hwnd, 0, hinst, 0);
/* FIXME: (HMENU)IDTVEDIT,pcs->hInstance,0); */

    infoPtr->hwndEdit = hwndEdit;

    /* Get a 2D border. */
    SetWindowLongW(hwndEdit, GWL_EXSTYLE,
		   GetWindowLongW(hwndEdit, GWL_EXSTYLE) & ~WS_EX_CLIENTEDGE);
    SetWindowLongW(hwndEdit, GWL_STYLE,
		   GetWindowLongW(hwndEdit, GWL_STYLE) | WS_BORDER);

    SendMessageW(hwndEdit, WM_SETFONT,
		 (WPARAM)TREEVIEW_FontForItem(infoPtr, hItem), FALSE);

    infoPtr->wpEditOrig = (WNDPROC)SetWindowLongPtrW(hwndEdit, GWLP_WNDPROC,
						  (DWORD_PTR)
						  TREEVIEW_Edit_SubclassProc);
    SendMessageW(hwndEdit, EM_SETLIMITTEXT, MAX_PATH - 1, 0);
    if (hItem->pszText)
        SetWindowTextW(hwndEdit, hItem->pszText);

    if (TREEVIEW_BeginLabelEditNotify(infoPtr, hItem))
    {
	DestroyWindow(hwndEdit);
	infoPtr->hwndEdit = 0;
	infoPtr->editItem = NULL;
	return NULL;
    }

    SetFocus(hwndEdit);
    SendMessageW(hwndEdit, EM_SETSEL, 0, -1);
    ShowWindow(hwndEdit, SW_SHOW);

    return hwndEdit;
}


static LRESULT
TREEVIEW_EndEditLabelNow(TREEVIEW_INFO *infoPtr, BOOL bCancel)
{
    TREEVIEW_ITEM *editedItem = infoPtr->editItem;
    NMTVDISPINFOW tvdi;
    BOOL bCommit;
    WCHAR tmpText[MAX_PATH] = { '\0' };
    WCHAR *newText;
    int iLength = 0;

    if (!IsWindow(infoPtr->hwndEdit)) return FALSE;

    tvdi.item.mask = 0;
    tvdi.item.hItem = editedItem;
    tvdi.item.state = editedItem->state;
    tvdi.item.lParam = editedItem->lParam;

    if (!bCancel)
    {
        if (!infoPtr->bNtfUnicode)
            iLength = GetWindowTextA(infoPtr->hwndEdit, (LPSTR)tmpText, ARRAY_SIZE(tmpText));
        else
            iLength = GetWindowTextW(infoPtr->hwndEdit, tmpText, ARRAY_SIZE(tmpText));

        tvdi.item.mask = TVIF_TEXT;
	tvdi.item.pszText = tmpText;
	tvdi.item.cchTextMax = ARRAY_SIZE(tmpText);
    }
    else
    {
	tvdi.item.pszText = NULL;
	tvdi.item.cchTextMax = 0;
    }

    bCommit = TREEVIEW_SendRealNotify(infoPtr, TVN_ENDLABELEDITW, &tvdi.hdr);

    if (!bCancel && bCommit)	/* Apply the changes */
    {
        if (!infoPtr->bNtfUnicode)
        {
            DWORD len = MultiByteToWideChar( CP_ACP, 0, (LPSTR)tvdi.item.pszText, -1, NULL, 0 );
            newText = heap_alloc(len * sizeof(WCHAR));
            MultiByteToWideChar( CP_ACP, 0, (LPSTR)tvdi.item.pszText, -1, newText, len );
            iLength = len - 1;
        }
        else
            newText = tvdi.item.pszText;

        if (lstrcmpW(newText, editedItem->pszText) != 0)
        {
            WCHAR *ptr = heap_realloc(editedItem->pszText, sizeof(WCHAR)*(iLength + 1));
            if (ptr == NULL)
            {
                ERR("OutOfMemory, cannot allocate space for label\n");
                if (newText != tmpText) heap_free(newText);
                DestroyWindow(infoPtr->hwndEdit);
                infoPtr->hwndEdit = 0;
                infoPtr->editItem = NULL;
                return FALSE;
            }
            else
            {
                editedItem->pszText = ptr;
                editedItem->cchTextMax = iLength + 1;
                lstrcpyW(editedItem->pszText, newText);
                TREEVIEW_ComputeTextWidth(infoPtr, editedItem, 0);
            }
        }
        if (newText != tmpText) heap_free(newText);
    }

    ShowWindow(infoPtr->hwndEdit, SW_HIDE);
    DestroyWindow(infoPtr->hwndEdit);
    infoPtr->hwndEdit = 0;
    infoPtr->editItem = NULL;
    return TRUE;
}

static LRESULT
TREEVIEW_HandleTimer(TREEVIEW_INFO *infoPtr, WPARAM wParam)
{
    if (wParam != TV_EDIT_TIMER)
    {
	ERR("got unknown timer\n");
	return 1;
    }

    KillTimer(infoPtr->hwnd, TV_EDIT_TIMER);
    infoPtr->Timer &= ~TV_EDIT_TIMER_SET;

    TREEVIEW_EditLabel(infoPtr, infoPtr->selectedItem);

    return 0;
}


/* Mouse Tracking/Drag **************************************************/

/***************************************************************************
 * This is quite unusual piece of code, but that's how it's implemented in
 * Windows.
 */
static LRESULT
TREEVIEW_TrackMouse(const TREEVIEW_INFO *infoPtr, POINT pt)
{
    INT cxDrag = GetSystemMetrics(SM_CXDRAG);
    INT cyDrag = GetSystemMetrics(SM_CYDRAG);
    RECT r;
    MSG msg;

    r.top = pt.y - cyDrag;
    r.left = pt.x - cxDrag;
    r.bottom = pt.y + cyDrag;
    r.right = pt.x + cxDrag;

    SetCapture(infoPtr->hwnd);

    while (1)
    {
	if (PeekMessageW(&msg, 0, 0, 0, PM_REMOVE | PM_NOYIELD))
	{
	    if (msg.message == WM_MOUSEMOVE)
	    {
		pt.x = (short)LOWORD(msg.lParam);
		pt.y = (short)HIWORD(msg.lParam);
		if (PtInRect(&r, pt))
		    continue;
		else
		{
		    ReleaseCapture();
		    return 1;
		}
	    }
	    else if (msg.message >= WM_LBUTTONDOWN &&
		     msg.message <= WM_RBUTTONDBLCLK)
	    {
		break;
	    }

	    DispatchMessageW(&msg);
	}

	if (GetCapture() != infoPtr->hwnd)
	    return 0;
    }

    ReleaseCapture();
    return 0;
}


static LRESULT
TREEVIEW_LButtonDoubleClick(TREEVIEW_INFO *infoPtr, LPARAM lParam)
{
    TREEVIEW_ITEM *item;
    TVHITTESTINFO hit;

    TRACE("\n");
    SetFocus(infoPtr->hwnd);

    if (infoPtr->Timer & TV_EDIT_TIMER_SET)
    {
	/* If there is pending 'edit label' event - kill it now */
	KillTimer(infoPtr->hwnd, TV_EDIT_TIMER);
    }

    hit.pt.x = (short)LOWORD(lParam);
    hit.pt.y = (short)HIWORD(lParam);

    item = TREEVIEW_HitTest(infoPtr, &hit);
    if (!item)
	return 0;
    TRACE("item %d\n", TREEVIEW_GetItemIndex(infoPtr, item));

    if (TREEVIEW_SendSimpleNotify(infoPtr, NM_DBLCLK) == FALSE)
    {				/* FIXME! */
	switch (hit.flags)
	{
	case TVHT_ONITEMRIGHT:
	    /* FIXME: we should not have sent NM_DBLCLK in this case. */
	    break;

	case TVHT_ONITEMINDENT:
	    if (!(infoPtr->dwStyle & TVS_HASLINES))
	    {
		break;
	    }
	    else
	    {
		int level = hit.pt.x / infoPtr->uIndent;
		if (!(infoPtr->dwStyle & TVS_LINESATROOT)) level++;

		while (item->iLevel > level)
		{
		    item = item->parent;
		}

		/* fall through */
	    }

	case TVHT_ONITEMLABEL:
	case TVHT_ONITEMICON:
	case TVHT_ONITEMBUTTON:
	    TREEVIEW_Toggle(infoPtr, item, TRUE);
	    break;

	case TVHT_ONITEMSTATEICON:
	   if (infoPtr->dwStyle & TVS_CHECKBOXES)
	       TREEVIEW_ToggleItemState(infoPtr, item);
	   else
	       TREEVIEW_Toggle(infoPtr, item, TRUE);
	   break;
	}
    }
    return TRUE;
}


static LRESULT
TREEVIEW_LButtonDown(TREEVIEW_INFO *infoPtr, LPARAM lParam)
{
    BOOL do_track, do_select, bDoLabelEdit;
    HWND hwnd = infoPtr->hwnd;
    TVHITTESTINFO ht;

    /* If Edit control is active - kill it and return.
     * The best way to do it is to set focus to itself.
     * Edit control subclassed procedure will automatically call
     * EndEditLabelNow.
     */
    if (infoPtr->hwndEdit)
    {
	SetFocus(hwnd);
	return 0;
    }

    ht.pt.x = (short)LOWORD(lParam);
    ht.pt.y = (short)HIWORD(lParam);

    TREEVIEW_HitTest(infoPtr, &ht);
    TRACE("item %d\n", TREEVIEW_GetItemIndex(infoPtr, ht.hItem));

    /* update focusedItem and redraw both items */
    if (ht.hItem)
    {
        BOOL do_focus;

        if (TREEVIEW_IsFullRowSelect(infoPtr))
            do_focus = ht.flags & (TVHT_ONITEMINDENT | TVHT_ONITEM | TVHT_ONITEMRIGHT);
        else
            do_focus = ht.flags & TVHT_ONITEM;

        if (do_focus)
        {
            infoPtr->focusedItem = ht.hItem;
            TREEVIEW_InvalidateItem(infoPtr, infoPtr->focusedItem);
            TREEVIEW_InvalidateItem(infoPtr, infoPtr->selectedItem);
        }
    }

    if (!(infoPtr->dwStyle & TVS_DISABLEDRAGDROP))
    {
        if (TREEVIEW_IsFullRowSelect(infoPtr))
            do_track = ht.flags & (TVHT_ONITEMINDENT | TVHT_ONITEM | TVHT_ONITEMRIGHT);
        else
            do_track = ht.flags & TVHT_ONITEM;
    }
    else
        do_track = FALSE;

    /*
     * If the style allows editing and the node is already selected
     * and the click occurred on the item label...
     */
    bDoLabelEdit = (infoPtr->dwStyle & TVS_EDITLABELS) &&
        (ht.flags & TVHT_ONITEMLABEL) && (infoPtr->selectedItem == ht.hItem);

    /* Send NM_CLICK right away */
    if (!do_track && TREEVIEW_SendSimpleNotify(infoPtr, NM_CLICK))
        goto setfocus;

    if (ht.flags & TVHT_ONITEMBUTTON)
    {
	TREEVIEW_Toggle(infoPtr, ht.hItem, TRUE);
	goto setfocus;
    }
    else if (do_track)
    {   /* if TREEVIEW_TrackMouse == 1 dragging occurred and the cursor left the dragged item's rectangle */
	if (TREEVIEW_TrackMouse(infoPtr, ht.pt))
	{
	    TREEVIEW_SendTreeviewDnDNotify(infoPtr, TVN_BEGINDRAGW, ht.hItem, ht.pt);
	    infoPtr->dropItem = ht.hItem;

            /* clean up focusedItem as we dragged and won't select this item */
            if(infoPtr->focusedItem)
            {
                /* refresh the item that was focused */
                TREEVIEW_InvalidateItem(infoPtr, infoPtr->focusedItem);
                infoPtr->focusedItem = NULL;

                /* refresh the selected item to return the filled background */
                TREEVIEW_InvalidateItem(infoPtr, infoPtr->selectedItem);
            }

	    return 0;
        }
    }

    if (do_track && TREEVIEW_SendSimpleNotify(infoPtr, NM_CLICK))
        goto setfocus;

    if (TREEVIEW_IsFullRowSelect(infoPtr))
        do_select = ht.flags & (TVHT_ONITEMINDENT | TVHT_ONITEMICON | TVHT_ONITEMLABEL | TVHT_ONITEMRIGHT);
    else
        do_select = ht.flags & (TVHT_ONITEMICON | TVHT_ONITEMLABEL);

    if (bDoLabelEdit)
    {
	if (infoPtr->Timer & TV_EDIT_TIMER_SET)
	    KillTimer(hwnd, TV_EDIT_TIMER);

	SetTimer(hwnd, TV_EDIT_TIMER, GetDoubleClickTime(), 0);
	infoPtr->Timer |= TV_EDIT_TIMER_SET;
    }
    else if (do_select)
    {
        TREEVIEW_ITEM *selection = infoPtr->selectedItem;

        /* Select the current item */
        TREEVIEW_DoSelectItem(infoPtr, TVGN_CARET, ht.hItem, TVC_BYMOUSE);
        TREEVIEW_SingleExpand(infoPtr, selection, ht.hItem);
    }
    else if (ht.flags & TVHT_ONITEMSTATEICON)
    {
	/* TVS_CHECKBOXES requires us to toggle the current state */
	if (infoPtr->dwStyle & TVS_CHECKBOXES)
	    TREEVIEW_ToggleItemState(infoPtr, ht.hItem);
    }

  setfocus:
    SetFocus(hwnd);
    return 0;
}


static LRESULT
TREEVIEW_RButtonDown(TREEVIEW_INFO *infoPtr, LPARAM lParam)
{
    TVHITTESTINFO ht;

    if (infoPtr->hwndEdit)
    {
	SetFocus(infoPtr->hwnd);
	return 0;
    }

    ht.pt.x = (short)LOWORD(lParam);
    ht.pt.y = (short)HIWORD(lParam);

    if (TREEVIEW_HitTest(infoPtr, &ht))
    {
        infoPtr->focusedItem = ht.hItem;
        TREEVIEW_InvalidateItem(infoPtr, infoPtr->focusedItem);
        TREEVIEW_InvalidateItem(infoPtr, infoPtr->selectedItem);
    }

    if (TREEVIEW_TrackMouse(infoPtr, ht.pt))
    {
	if (ht.hItem)
	{
	    TREEVIEW_SendTreeviewDnDNotify(infoPtr, TVN_BEGINRDRAGW, ht.hItem, ht.pt);
	    infoPtr->dropItem = ht.hItem;
	}
    }
    else
    {
	SetFocus(infoPtr->hwnd);
	if(!TREEVIEW_SendSimpleNotify(infoPtr, NM_RCLICK))
	{
	    /* Send a WM_CONTEXTMENU message in response to the RBUTTONUP */
	    SendMessageW(infoPtr->hwndNotify, WM_CONTEXTMENU,
		(WPARAM)infoPtr->hwnd, (LPARAM)GetMessagePos());
	}
    }

    if (ht.hItem)
    {
        TREEVIEW_InvalidateItem(infoPtr, infoPtr->focusedItem);
        infoPtr->focusedItem = infoPtr->selectedItem;
        TREEVIEW_InvalidateItem(infoPtr, infoPtr->focusedItem);
    }

    return 0;
}

static LRESULT
TREEVIEW_CreateDragImage(TREEVIEW_INFO *infoPtr, LPARAM lParam)
{
    TREEVIEW_ITEM *dragItem = (HTREEITEM)lParam;
    INT cx, cy;
    HDC hdc, htopdc;
    HWND hwtop;
    HBITMAP hbmp, hOldbmp;
    SIZE size;
    RECT rc;
    HFONT hOldFont;

    TRACE("\n");

    if (!(infoPtr->himlNormal))
	return 0;

    if (!dragItem || !TREEVIEW_ValidItem(infoPtr, dragItem))
	return 0;

    TREEVIEW_UpdateDispInfo(infoPtr, dragItem, TVIF_TEXT);

    hwtop = GetDesktopWindow();
    htopdc = GetDC(hwtop);
    hdc = CreateCompatibleDC(htopdc);

    hOldFont = SelectObject(hdc, infoPtr->hFont);

    if (dragItem->pszText)
        GetTextExtentPoint32W(hdc, dragItem->pszText, lstrlenW(dragItem->pszText),
			  &size);
    else
        GetTextExtentPoint32A(hdc, "", 0, &size);

    TRACE("%d %d %s\n", size.cx, size.cy, debugstr_w(dragItem->pszText));
    hbmp = CreateCompatibleBitmap(htopdc, size.cx, size.cy);
    hOldbmp = SelectObject(hdc, hbmp);

    ImageList_GetIconSize(infoPtr->himlNormal, &cx, &cy);
    size.cx += cx;
    if (cy > size.cy)
	size.cy = cy;

    infoPtr->dragList = ImageList_Create(size.cx, size.cy, ILC_COLOR, 10, 10);
    ImageList_Draw(infoPtr->himlNormal, dragItem->iImage, hdc, 0, 0,
		   ILD_NORMAL);

/*
 ImageList_GetImageInfo (infoPtr->himlNormal, dragItem->hItem, &iminfo);
 ImageList_AddMasked (infoPtr->dragList, iminfo.hbmImage, CLR_DEFAULT);
*/

/* draw item text */

    SetRect(&rc, cx, 0, size.cx, size.cy);

    if (dragItem->pszText)
        DrawTextW(hdc, dragItem->pszText, lstrlenW(dragItem->pszText), &rc,
                  DT_LEFT);

    SelectObject(hdc, hOldFont);
    SelectObject(hdc, hOldbmp);

    ImageList_Add(infoPtr->dragList, hbmp, 0);

    DeleteDC(hdc);
    DeleteObject(hbmp);
    ReleaseDC(hwtop, htopdc);

    return (LRESULT)infoPtr->dragList;
}

/* Selection ************************************************************/

static LRESULT
TREEVIEW_DoSelectItem(TREEVIEW_INFO *infoPtr, INT action, HTREEITEM newSelect,
		      INT cause)
{
    TREEVIEW_ITEM *prevSelect;

    assert(newSelect == NULL || TREEVIEW_ValidItem(infoPtr, newSelect));

    TRACE("Entering item %p (%s), flag 0x%x, cause 0x%x, state 0x%x\n",
	  newSelect, TREEVIEW_ItemName(newSelect), action, cause,
	  newSelect ? newSelect->state : 0);

    /* reset and redraw focusedItem if focusedItem was set so we don't */
    /* have to worry about the previously focused item when we set a new one */
    TREEVIEW_InvalidateItem(infoPtr, infoPtr->focusedItem);
    infoPtr->focusedItem = NULL;

    switch (action)
    {
    case TVGN_CARET|TVSI_NOSINGLEEXPAND:
        FIXME("TVSI_NOSINGLEEXPAND specified.\n");
        /* Fall through */
    case TVGN_CARET:
	prevSelect = infoPtr->selectedItem;

	if (prevSelect == newSelect) {
	    TREEVIEW_EnsureVisible(infoPtr, infoPtr->selectedItem, FALSE);
	    break;
	}

	if (TREEVIEW_SendTreeviewNotify(infoPtr,
					TVN_SELCHANGINGW,
					cause,
					TVIF_TEXT | TVIF_HANDLE | TVIF_STATE | TVIF_PARAM,
					prevSelect,
					newSelect))
	    return FALSE;

	if (prevSelect)
	    prevSelect->state &= ~TVIS_SELECTED;
	if (newSelect)
	    newSelect->state |= TVIS_SELECTED;

	infoPtr->selectedItem = newSelect;

	TREEVIEW_EnsureVisible(infoPtr, infoPtr->selectedItem, FALSE);

        TREEVIEW_InvalidateItem(infoPtr, prevSelect);
        TREEVIEW_InvalidateItem(infoPtr, newSelect);

	TREEVIEW_SendTreeviewNotify(infoPtr,
				    TVN_SELCHANGEDW,
				    cause,
				    TVIF_TEXT | TVIF_HANDLE | TVIF_STATE | TVIF_PARAM,
				    prevSelect,
				    newSelect);
	break;

    case TVGN_DROPHILITE:
	prevSelect = infoPtr->dropItem;

	if (prevSelect)
	    prevSelect->state &= ~TVIS_DROPHILITED;

	infoPtr->dropItem = newSelect;

	if (newSelect)
	    newSelect->state |= TVIS_DROPHILITED;

	TREEVIEW_Invalidate(infoPtr, prevSelect);
	TREEVIEW_Invalidate(infoPtr, newSelect);
	break;

    case TVGN_FIRSTVISIBLE:
	if (newSelect != NULL)
	{
	    TREEVIEW_EnsureVisible(infoPtr, newSelect, FALSE);
	    TREEVIEW_SetFirstVisible(infoPtr, newSelect, TRUE);
	    TREEVIEW_Invalidate(infoPtr, NULL);
	}
	break;
    }

    TRACE("Leaving state 0x%x\n", newSelect ? newSelect->state : 0);
    return TRUE;
}

/* FIXME: handle NM_KILLFOCUS etc */
static LRESULT
TREEVIEW_SelectItem(TREEVIEW_INFO *infoPtr, INT wParam, HTREEITEM item)
{
    TREEVIEW_ITEM *selection = infoPtr->selectedItem;

    if (item && !TREEVIEW_ValidItem(infoPtr, item))
	return FALSE;

    if (item == infoPtr->selectedItem)
	return TRUE;

    TRACE("%p (%s) %d\n", item, TREEVIEW_ItemName(item), wParam);

    if (!TREEVIEW_DoSelectItem(infoPtr, wParam, item, TVC_UNKNOWN))
	return FALSE;

    TREEVIEW_SingleExpand(infoPtr, selection, item);

    return TRUE;
}

/*************************************************************************
 *		TREEVIEW_ProcessLetterKeys
 *
 *  Processes keyboard messages generated by pressing the letter keys
 *  on the keyboard.
 *  What this does is perform a case insensitive search from the
 *  current position with the following quirks:
 *  - If two chars or more are pressed in quick succession we search
 *    for the corresponding string (e.g. 'abc').
 *  - If there is a delay we wipe away the current search string and
 *    restart with just that char.
 *  - If the user keeps pressing the same character, whether slowly or
 *    fast, so that the search string is entirely composed of this
 *    character ('aaaaa' for instance), then we search for first item
 *    that starting with that character.
 *  - If the user types the above character in quick succession, then
 *    we must also search for the corresponding string ('aaaaa'), and
 *    go to that string if there is a match.
 *
 * RETURNS
 *
 *  Zero.
 *
 * BUGS
 *
 *  - The current implementation has a list of characters it will
 *    accept and it ignores everything else. In particular it will
 *    ignore accentuated characters which seems to match what
 *    Windows does. But I'm not sure it makes sense to follow
 *    Windows there.
 *  - We don't sound a beep when the search fails.
 *  - The search should start from the focused item, not from the selected
 *    item. One reason for this is to allow for multiple selections in trees.
 *    But currently infoPtr->focusedItem does not seem very usable.
 *
 * SEE ALSO
 *
 *  TREEVIEW_ProcessLetterKeys
 */
static INT TREEVIEW_ProcessLetterKeys(TREEVIEW_INFO *infoPtr, WPARAM charCode, LPARAM keyData)
{
    HTREEITEM nItem;
    HTREEITEM endidx,idx;
    TVITEMEXW item;
    WCHAR buffer[MAX_PATH];
    DWORD timestamp,elapsed;

    /* simple parameter checking */
    if (!charCode || !keyData) return 0;

    /* only allow the valid WM_CHARs through */
    if (!isalnum(charCode) &&
        charCode != '.' && charCode != '`' && charCode != '!' &&
        charCode != '@' && charCode != '#' && charCode != '$' &&
        charCode != '%' && charCode != '^' && charCode != '&' &&
        charCode != '*' && charCode != '(' && charCode != ')' &&
        charCode != '-' && charCode != '_' && charCode != '+' &&
        charCode != '=' && charCode != '\\'&& charCode != ']' &&
        charCode != '}' && charCode != '[' && charCode != '{' &&
        charCode != '/' && charCode != '?' && charCode != '>' &&
        charCode != '<' && charCode != ',' && charCode != '~')
        return 0;

    /* compute how much time elapsed since last keypress */
    timestamp = GetTickCount();
    if (timestamp > infoPtr->lastKeyPressTimestamp) {
        elapsed=timestamp-infoPtr->lastKeyPressTimestamp;
    } else {
        elapsed=infoPtr->lastKeyPressTimestamp-timestamp;
    }

    /* update the search parameters */
    infoPtr->lastKeyPressTimestamp=timestamp;
    if (elapsed < KEY_DELAY) {
        if (infoPtr->nSearchParamLength < ARRAY_SIZE(infoPtr->szSearchParam)) {
            infoPtr->szSearchParam[infoPtr->nSearchParamLength++]=charCode;
        }
        if (infoPtr->charCode != charCode) {
            infoPtr->charCode=charCode=0;
        }
    } else {
        infoPtr->charCode=charCode;
        infoPtr->szSearchParam[0]=charCode;
        infoPtr->nSearchParamLength=1;
        /* Redundant with the 1 char string */
        charCode=0;
    }

    /* and search from the current position */
    nItem=NULL;
    if (infoPtr->selectedItem != NULL) {
        endidx=infoPtr->selectedItem;
        /* if looking for single character match,
         * then we must always move forward
         */
        if (infoPtr->nSearchParamLength == 1)
            idx=TREEVIEW_GetNextListItem(infoPtr,endidx);
        else
            idx=endidx;
    } else {
        endidx=NULL;
        idx=infoPtr->root->firstChild;
    }
    do {
        /* At the end point, sort out wrapping */
        if (idx == NULL) {

            /* If endidx is null, stop at the last item (ie top to bottom) */
            if (endidx == NULL)
                break;

            /* Otherwise, start again at the very beginning */
            idx=infoPtr->root->firstChild;

            /* But if we are stopping on the first child, end now! */
            if (idx == endidx) break;
        }

        /* get item */
        ZeroMemory(&item, sizeof(item));
        item.mask = TVIF_TEXT;
        item.hItem = idx;
        item.pszText = buffer;
        item.cchTextMax = ARRAY_SIZE(buffer);
        TREEVIEW_GetItemT( infoPtr, &item, TRUE );

        /* check for a match */
        if (wcsnicmp(item.pszText,infoPtr->szSearchParam,infoPtr->nSearchParamLength) == 0) {
            nItem=idx;
            break;
        } else if ( (charCode != 0) && (nItem == NULL) &&
                    (nItem != infoPtr->selectedItem) &&
                    (wcsnicmp(item.pszText,infoPtr->szSearchParam,1) == 0) ) {
            /* This would work but we must keep looking for a longer match */
            nItem=idx;
        }
        idx=TREEVIEW_GetNextListItem(infoPtr,idx);
    } while (idx != endidx);

    if (nItem != NULL) {
        if (TREEVIEW_DoSelectItem(infoPtr, TVGN_CARET, nItem, TVC_BYKEYBOARD)) {
            TREEVIEW_EnsureVisible(infoPtr, nItem, FALSE);
        }
    }

    return 0;
}

/* Scrolling ************************************************************/

static LRESULT
TREEVIEW_EnsureVisible(TREEVIEW_INFO *infoPtr, HTREEITEM item, BOOL bHScroll)
{
    int viscount;
    BOOL hasFirstVisible = infoPtr->firstVisible != NULL;
    HTREEITEM newFirstVisible = NULL;
    int visible_pos = -1;

    if (!TREEVIEW_ValidItem(infoPtr, item))
	return FALSE;

    if (!ISVISIBLE(item))
    {
	/* Expand parents as necessary. */
	HTREEITEM parent;

        /* see if we are trying to ensure that root is visible */
        if((item != infoPtr->root) && TREEVIEW_ValidItem(infoPtr, item))
          parent = item->parent;
        else
          parent = item; /* this item is the topmost item */

	while (parent != infoPtr->root)
	{
	    if (!(parent->state & TVIS_EXPANDED))
		TREEVIEW_Expand(infoPtr, parent, FALSE, TRUE);

	    parent = parent->parent;
	}
    }

    viscount = TREEVIEW_GetVisibleCount(infoPtr);

    TRACE("%p (%s) %d - %d viscount(%d)\n", item, TREEVIEW_ItemName(item), item->visibleOrder,
        hasFirstVisible ? infoPtr->firstVisible->visibleOrder : -1, viscount);

    if (hasFirstVisible)
        visible_pos = item->visibleOrder - infoPtr->firstVisible->visibleOrder;

    if (visible_pos < 0)
    {
	/* item is before the start of the list: put it at the top. */
	newFirstVisible = item;
    }
    else if (visible_pos >= viscount
	     /* Sometimes, before we are displayed, GVC is 0, causing us to
	      * spuriously scroll up. */
	     && visible_pos > 0 && !(infoPtr->dwStyle & TVS_NOSCROLL) )
    {
	/* item is past the end of the list. */
	int scroll = visible_pos - viscount;

	newFirstVisible = TREEVIEW_GetListItem(infoPtr, infoPtr->firstVisible,
					       scroll + 1);
    }

    if (bHScroll)
    {
        /* Scroll window so item's text is visible as much as possible */
        /* Calculation of amount of extra space is taken from EditLabel code */
        INT pos, x;
        TEXTMETRICW textMetric;
        HDC hdc = GetWindowDC(infoPtr->hwnd);

        x = item->textWidth;

        GetTextMetricsW(hdc, &textMetric);
        ReleaseDC(infoPtr->hwnd, hdc);

        x += (textMetric.tmMaxCharWidth * 2);
        x = max(x, textMetric.tmMaxCharWidth * 3);

	if (item->textOffset < 0)
	   pos = item->textOffset;
	else if (item->textOffset + x > infoPtr->clientWidth)
        {
           if (x > infoPtr->clientWidth)
              pos = item->textOffset;
           else
              pos = item->textOffset + x - infoPtr->clientWidth;
        }
        else
           pos = 0;

	TREEVIEW_HScroll(infoPtr, MAKEWPARAM(SB_THUMBPOSITION, infoPtr->scrollX + pos));
    }

    if (newFirstVisible != NULL && newFirstVisible != infoPtr->firstVisible)
    {
	TREEVIEW_SetFirstVisible(infoPtr, newFirstVisible, TRUE);

	return TRUE;
    }

    return FALSE;
}

static VOID
TREEVIEW_SetFirstVisible(TREEVIEW_INFO *infoPtr,
                         TREEVIEW_ITEM *newFirstVisible,
                         BOOL bUpdateScrollPos)
{
    int gap_size;

    TRACE("%p: %s\n", newFirstVisible, TREEVIEW_ItemName(newFirstVisible));

    if (newFirstVisible != NULL)
    {
	/* Prevent an empty gap from appearing at the bottom... */
	gap_size = TREEVIEW_GetVisibleCount(infoPtr)
	    - infoPtr->maxVisibleOrder + newFirstVisible->visibleOrder;

	if (gap_size > 0)
	{
	    newFirstVisible = TREEVIEW_GetListItem(infoPtr, newFirstVisible,
						   -gap_size);

	    /* ... unless we just don't have enough items. */
	    if (newFirstVisible == NULL)
		newFirstVisible = infoPtr->root->firstChild;
	}
    }

    if (infoPtr->firstVisible != newFirstVisible)
    {
	if (infoPtr->firstVisible == NULL || newFirstVisible == NULL)
	{
	    infoPtr->firstVisible = newFirstVisible;
	    TREEVIEW_Invalidate(infoPtr, NULL);
	}
	else
	{
	    TREEVIEW_ITEM *item;
	    int scroll = infoPtr->uItemHeight *
	                 (infoPtr->firstVisible->visibleOrder
	                  - newFirstVisible->visibleOrder);

	    infoPtr->firstVisible = newFirstVisible;

	    for (item = infoPtr->root->firstChild; item != NULL;
	         item = TREEVIEW_GetNextListItem(infoPtr, item))
	    {
	       item->rect.top += scroll;
	       item->rect.bottom += scroll;
	    }

	    if (bUpdateScrollPos)
		SetScrollPos(infoPtr->hwnd, SB_VERT,
		              newFirstVisible->visibleOrder, TRUE);

	    ScrollWindowEx(infoPtr->hwnd, 0, scroll, NULL, NULL, NULL, NULL, SW_ERASE | SW_INVALIDATE);
	}
    }
}

/************************************************************************
 * VScroll is always in units of visible items. i.e. we always have a
 * visible item aligned to the top of the control. (Unless we have no
 * items at all.)
 */
static LRESULT
TREEVIEW_VScroll(TREEVIEW_INFO *infoPtr, WPARAM wParam)
{
    TREEVIEW_ITEM *oldFirstVisible = infoPtr->firstVisible;
    TREEVIEW_ITEM *newFirstVisible = NULL;

    int nScrollCode = LOWORD(wParam);

    TRACE("wp %lx\n", wParam);

    if (!(infoPtr->uInternalStatus & TV_VSCROLL))
	return 0;

    if (!oldFirstVisible)
    {
	assert(infoPtr->root->firstChild == NULL);
	return 0;
    }

    switch (nScrollCode)
    {
    case SB_TOP:
	newFirstVisible = infoPtr->root->firstChild;
	break;

    case SB_BOTTOM:
	newFirstVisible = TREEVIEW_GetLastListItem(infoPtr, infoPtr->root);
	break;

    case SB_LINEUP:
	newFirstVisible = TREEVIEW_GetPrevListItem(infoPtr, oldFirstVisible);
	break;

    case SB_LINEDOWN:
	newFirstVisible = TREEVIEW_GetNextListItem(infoPtr, oldFirstVisible);
	break;

    case SB_PAGEUP:
	newFirstVisible = TREEVIEW_GetListItem(infoPtr, oldFirstVisible,
					       -max(1, TREEVIEW_GetVisibleCount(infoPtr)));
	break;

    case SB_PAGEDOWN:
	newFirstVisible = TREEVIEW_GetListItem(infoPtr, oldFirstVisible,
					       max(1, TREEVIEW_GetVisibleCount(infoPtr)));
	break;

    case SB_THUMBTRACK:
    case SB_THUMBPOSITION:
	newFirstVisible = TREEVIEW_GetListItem(infoPtr,
					       infoPtr->root->firstChild,
					       (LONG)(SHORT)HIWORD(wParam));
	break;

    case SB_ENDSCROLL:
	return 0;
    }

    if (newFirstVisible != NULL)
    {
	if (newFirstVisible != oldFirstVisible)
	    TREEVIEW_SetFirstVisible(infoPtr, newFirstVisible,
	                          nScrollCode != SB_THUMBTRACK);
	else if (nScrollCode == SB_THUMBPOSITION)
	    SetScrollPos(infoPtr->hwnd, SB_VERT,
	                 newFirstVisible->visibleOrder, TRUE);
    }

    return 0;
}

static LRESULT
TREEVIEW_HScroll(TREEVIEW_INFO *infoPtr, WPARAM wParam)
{
    int maxWidth;
    int scrollX = infoPtr->scrollX;
    int nScrollCode = LOWORD(wParam);

    TRACE("wp %lx\n", wParam);

    if (!(infoPtr->uInternalStatus & TV_HSCROLL))
	return FALSE;

    maxWidth = infoPtr->treeWidth - infoPtr->clientWidth;
    /* shall never occur */
    if (maxWidth <= 0)
    {
       scrollX = 0;
       goto scroll;
    }

    switch (nScrollCode)
    {
    case SB_LINELEFT:
	scrollX -= infoPtr->uItemHeight;
	break;
    case SB_LINERIGHT:
	scrollX += infoPtr->uItemHeight;
	break;
    case SB_PAGELEFT:
	scrollX -= infoPtr->clientWidth;
	break;
    case SB_PAGERIGHT:
	scrollX += infoPtr->clientWidth;
	break;

    case SB_THUMBTRACK:
    case SB_THUMBPOSITION:
	scrollX = (int)(SHORT)HIWORD(wParam);
	break;

    case SB_ENDSCROLL:
       return 0;
    }

    if (scrollX > maxWidth)
        scrollX = maxWidth;
    else if (scrollX < 0)
        scrollX = 0;

scroll:
    if (scrollX != infoPtr->scrollX)
    {
        TREEVIEW_ITEM *item;
        LONG scroll_pixels = infoPtr->scrollX - scrollX;

        for (item = infoPtr->root->firstChild; item != NULL;
             item = TREEVIEW_GetNextListItem(infoPtr, item))
        {
           item->linesOffset += scroll_pixels;
           item->stateOffset += scroll_pixels;
           item->imageOffset += scroll_pixels;
           item->textOffset  += scroll_pixels;
        }

	ScrollWindow(infoPtr->hwnd, scroll_pixels, 0, NULL, NULL);
	infoPtr->scrollX = scrollX;
	UpdateWindow(infoPtr->hwnd);
    }

    if (nScrollCode != SB_THUMBTRACK)
       SetScrollPos(infoPtr->hwnd, SB_HORZ, scrollX, TRUE);

    return 0;
}

static LRESULT
TREEVIEW_MouseWheel(TREEVIEW_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    short wheelDelta;
    INT pulScrollLines = 3;

    if (wParam & (MK_SHIFT | MK_CONTROL))
        return DefWindowProcW(infoPtr->hwnd, WM_MOUSEWHEEL, wParam, lParam);

    if (infoPtr->firstVisible == NULL)
	return TRUE;

    SystemParametersInfoW(SPI_GETWHEELSCROLLLINES, 0, &pulScrollLines, 0);

    wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam);
    /* if scrolling changes direction, ignore left overs */
    if ((wheelDelta < 0 && infoPtr->wheelRemainder < 0) ||
        (wheelDelta > 0 && infoPtr->wheelRemainder > 0))
        infoPtr->wheelRemainder += wheelDelta;
    else
        infoPtr->wheelRemainder = wheelDelta;

    if (infoPtr->wheelRemainder && pulScrollLines)
    {
	int newDy;
	int maxDy;
	int lineScroll;

	lineScroll = pulScrollLines * infoPtr->wheelRemainder / WHEEL_DELTA;
	infoPtr->wheelRemainder -= WHEEL_DELTA * lineScroll / pulScrollLines;

	newDy = infoPtr->firstVisible->visibleOrder - lineScroll;
	maxDy = infoPtr->maxVisibleOrder;

	if (newDy > maxDy)
	    newDy = maxDy;

	if (newDy < 0)
	    newDy = 0;

	TREEVIEW_VScroll(infoPtr, MAKEWPARAM(SB_THUMBPOSITION, newDy));
    }
    return TRUE;
}

/* Create/Destroy *******************************************************/

static LRESULT
TREEVIEW_Create(HWND hwnd, const CREATESTRUCTW *lpcs)
{
    RECT rcClient;
    TREEVIEW_INFO *infoPtr;
    LOGFONTW lf;

    TRACE("wnd %p, style 0x%x\n", hwnd, GetWindowLongW(hwnd, GWL_STYLE));

    infoPtr = heap_alloc_zero(sizeof(TREEVIEW_INFO));

    if (infoPtr == NULL)
    {
	ERR("could not allocate info memory!\n");
	return 0;
    }

    SetWindowLongPtrW(hwnd, 0, (DWORD_PTR)infoPtr);

    infoPtr->hwnd = hwnd;
    infoPtr->dwStyle = GetWindowLongW(hwnd, GWL_STYLE);
    infoPtr->Timer = 0;
    infoPtr->uNumItems = 0;
    infoPtr->cdmode = 0;
    infoPtr->uScrollTime = 300;	/* milliseconds */
    infoPtr->bRedraw = TRUE;

    GetClientRect(hwnd, &rcClient);

    /* No scroll bars yet. */
    infoPtr->clientWidth = rcClient.right;
    infoPtr->clientHeight = rcClient.bottom;
    infoPtr->uInternalStatus = 0;

    infoPtr->treeWidth = 0;
    infoPtr->treeHeight = 0;

    infoPtr->uIndent = MINIMUM_INDENT;
    infoPtr->selectedItem = NULL;
    infoPtr->focusedItem = NULL;
    infoPtr->hotItem = NULL;
    infoPtr->editItem = NULL;
    infoPtr->firstVisible = NULL;
    infoPtr->maxVisibleOrder = 0;
    infoPtr->dropItem = NULL;
    infoPtr->insertMarkItem = NULL;
    infoPtr->insertBeforeorAfter = 0;
    /* dragList */

    infoPtr->scrollX = 0;
    infoPtr->wheelRemainder = 0;

    infoPtr->clrBk   = CLR_NONE; /* use system color */
    infoPtr->clrText = CLR_NONE; /* use system color */
    infoPtr->clrLine = CLR_DEFAULT;
    infoPtr->clrInsertMark = CLR_DEFAULT;

    /* hwndToolTip */

    infoPtr->hwndEdit = NULL;
    infoPtr->wpEditOrig = NULL;
    infoPtr->bIgnoreEditKillFocus = FALSE;
    infoPtr->bLabelChanged = FALSE;

    infoPtr->himlNormal = NULL;
    infoPtr->himlState = NULL;
    infoPtr->normalImageWidth = 0;
    infoPtr->normalImageHeight = 0;
    infoPtr->stateImageWidth = 0;
    infoPtr->stateImageHeight = 0;

    infoPtr->items = DPA_Create(16);

    SystemParametersInfoW(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, 0);
    infoPtr->hFont = infoPtr->hDefaultFont = CreateFontIndirectW(&lf);
    infoPtr->hBoldFont = TREEVIEW_CreateBoldFont(infoPtr->hFont);
    infoPtr->hUnderlineFont = TREEVIEW_CreateUnderlineFont(infoPtr->hFont);
    infoPtr->hBoldUnderlineFont = TREEVIEW_CreateBoldUnderlineFont(infoPtr->hFont);
    infoPtr->hcurHand = LoadCursorW(NULL, (LPWSTR)IDC_HAND);

    infoPtr->uItemHeight = TREEVIEW_NaturalHeight(infoPtr);

    infoPtr->root = TREEVIEW_AllocateItem(infoPtr);
    infoPtr->root->state = TVIS_EXPANDED;
    infoPtr->root->iLevel = -1;
    infoPtr->root->visibleOrder = -1;

    infoPtr->hwndNotify = lpcs->hwndParent;
    infoPtr->hwndToolTip = 0;

    /* Determine what type of notify should be issued (sets infoPtr->bNtfUnicode) */
    TREEVIEW_NotifyFormat(infoPtr, infoPtr->hwndNotify, NF_REQUERY);

    if (!(infoPtr->dwStyle & TVS_NOTOOLTIPS))
        infoPtr->hwndToolTip = CreateWindowExW(0, TOOLTIPS_CLASSW, NULL, WS_POPUP,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            hwnd, 0, 0, 0);

    /* Make sure actual scrollbar state is consistent with uInternalStatus */
    ShowScrollBar(hwnd, SB_VERT, FALSE);
    ShowScrollBar(hwnd, SB_HORZ, FALSE);
    
    OpenThemeData (hwnd, themeClass);

    return 0;
}


static LRESULT
TREEVIEW_Destroy(TREEVIEW_INFO *infoPtr)
{
    TRACE("\n");

    /* free item data */
    TREEVIEW_RemoveTree(infoPtr);
    /* root isn't freed with other items */
    TREEVIEW_FreeItem(infoPtr, infoPtr->root);
    DPA_Destroy(infoPtr->items);

    /* Restore original wndproc */
    if (infoPtr->hwndEdit)
	SetWindowLongPtrW(infoPtr->hwndEdit, GWLP_WNDPROC,
		       (DWORD_PTR)infoPtr->wpEditOrig);

    CloseThemeData (GetWindowTheme (infoPtr->hwnd));

    /* Deassociate treeview from the window before doing anything drastic. */
    SetWindowLongPtrW(infoPtr->hwnd, 0, 0);

    DeleteObject(infoPtr->hDefaultFont);
    DeleteObject(infoPtr->hBoldFont);
    DeleteObject(infoPtr->hUnderlineFont);
    DeleteObject(infoPtr->hBoldUnderlineFont);
    DestroyWindow(infoPtr->hwndToolTip);
    heap_free(infoPtr);

    return 0;
}

/* Miscellaneous Messages ***********************************************/

static LRESULT
TREEVIEW_ScrollKeyDown(TREEVIEW_INFO *infoPtr, WPARAM key)
{
    static const struct
    {
	unsigned char code;
    }
    scroll[] =
    {
#define SCROLL_ENTRY(dir, code) { ((dir) << 7) | (code) }
	SCROLL_ENTRY(SB_VERT, SB_PAGEUP),	/* VK_PRIOR */
	SCROLL_ENTRY(SB_VERT, SB_PAGEDOWN),	/* VK_NEXT */
	SCROLL_ENTRY(SB_VERT, SB_BOTTOM),	/* VK_END */
	SCROLL_ENTRY(SB_VERT, SB_TOP),		/* VK_HOME */
	SCROLL_ENTRY(SB_HORZ, SB_LINEUP),	/* VK_LEFT */
	SCROLL_ENTRY(SB_VERT, SB_LINEUP),	/* VK_UP */
	SCROLL_ENTRY(SB_HORZ, SB_LINEDOWN),	/* VK_RIGHT */
	SCROLL_ENTRY(SB_VERT, SB_LINEDOWN)	/* VK_DOWN */
#undef SCROLL_ENTRY
    };

    if (key >= VK_PRIOR && key <= VK_DOWN)
    {
	unsigned char code = scroll[key - VK_PRIOR].code;

	(((code & (1 << 7)) == (SB_HORZ << 7))
	 ? TREEVIEW_HScroll
	 : TREEVIEW_VScroll)(infoPtr, code & 0x7F);
    }

    return 0;
}

/************************************************************************
 *        TREEVIEW_KeyDown
 *
 * VK_UP       Move selection to the previous non-hidden item.
 * VK_DOWN     Move selection to the next non-hidden item.
 * VK_HOME     Move selection to the first item.
 * VK_END      Move selection to the last item.
 * VK_LEFT     If expanded then collapse, otherwise move to parent.
 * VK_RIGHT    If collapsed then expand, otherwise move to first child.
 * VK_ADD      Expand.
 * VK_SUBTRACT Collapse.
 * VK_MULTIPLY Expand all.
 * VK_PRIOR    Move up GetVisibleCount items.
 * VK_NEXT     Move down GetVisibleCount items.
 * VK_BACK     Move to parent.
 * CTRL-Left,Right,Up,Down,PgUp,PgDown,Home,End: Scroll without changing selection
 */
static LRESULT
TREEVIEW_KeyDown(TREEVIEW_INFO *infoPtr, WPARAM wParam)
{
    /* If it is non-NULL and different, it will be selected and visible. */
    TREEVIEW_ITEM *newSelection = NULL;
    TREEVIEW_ITEM *prevItem = infoPtr->selectedItem;
    NMTVKEYDOWN nmkeydown;

    TRACE("%lx\n", wParam);

    nmkeydown.wVKey = wParam;
    nmkeydown.flags = 0;
    TREEVIEW_SendRealNotify(infoPtr, TVN_KEYDOWN, &nmkeydown.hdr);

    if (prevItem == NULL)
	return FALSE;

    if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
	return TREEVIEW_ScrollKeyDown(infoPtr, wParam);

    switch (wParam)
    {
    case VK_UP:
	newSelection = TREEVIEW_GetPrevListItem(infoPtr, prevItem);
	if (!newSelection)
	    newSelection = infoPtr->root->firstChild;
	break;

    case VK_DOWN:
	newSelection = TREEVIEW_GetNextListItem(infoPtr, prevItem);
	break;

    case VK_RETURN:
        TREEVIEW_SendSimpleNotify(infoPtr, NM_RETURN);
        break;

    case VK_HOME:
	newSelection = infoPtr->root->firstChild;
	break;

    case VK_END:
	newSelection = TREEVIEW_GetLastListItem(infoPtr, infoPtr->root);
	break;

    case VK_LEFT:
	if (prevItem->state & TVIS_EXPANDED)
	{
	    TREEVIEW_Collapse(infoPtr, prevItem, FALSE, TRUE);
	}
	else if (prevItem->parent != infoPtr->root)
	{
	    newSelection = prevItem->parent;
	}
	break;

    case VK_RIGHT:
	if (TREEVIEW_HasChildren(infoPtr, prevItem))
	{
	    if (!(prevItem->state & TVIS_EXPANDED))
		TREEVIEW_Expand(infoPtr, prevItem, FALSE, TRUE);
	    else
	    {
		newSelection = prevItem->firstChild;
	    }
	}

	break;

    case VK_MULTIPLY:
	TREEVIEW_ExpandAll(infoPtr, prevItem);
	break;

    case VK_ADD:
	TREEVIEW_Expand(infoPtr, prevItem, FALSE, TRUE);
	break;

    case VK_SUBTRACT:
	TREEVIEW_Collapse(infoPtr, prevItem, FALSE, TRUE);
	break;

    case VK_PRIOR:
	newSelection
	    = TREEVIEW_GetListItem(infoPtr, prevItem,
				   -TREEVIEW_GetVisibleCount(infoPtr));
	break;

    case VK_NEXT:
	newSelection
	    = TREEVIEW_GetListItem(infoPtr, prevItem,
				   TREEVIEW_GetVisibleCount(infoPtr));
	break;

    case VK_BACK:
	newSelection = prevItem->parent;
	if (newSelection == infoPtr->root)
	    newSelection = NULL;
	break;

    case VK_SPACE:
	if (infoPtr->dwStyle & TVS_CHECKBOXES)
	    TREEVIEW_ToggleItemState(infoPtr, prevItem);
	break;
    }

    if (newSelection && newSelection != prevItem)
    {
	if (TREEVIEW_DoSelectItem(infoPtr, TVGN_CARET, newSelection,
				  TVC_BYKEYBOARD))
	{
	    TREEVIEW_EnsureVisible(infoPtr, newSelection, FALSE);
	}
    }

    return FALSE;
}

static LRESULT
TREEVIEW_MouseLeave (TREEVIEW_INFO * infoPtr)
{
    /* remove hot effect from item */
    TREEVIEW_InvalidateItem(infoPtr, infoPtr->hotItem);
    infoPtr->hotItem = NULL;

    return 0;
}

static LRESULT
TREEVIEW_MouseMove (TREEVIEW_INFO * infoPtr, LPARAM lParam)
{
    TRACKMOUSEEVENT trackinfo;
    TREEVIEW_ITEM * item;
    TVHITTESTINFO ht;
    BOOL item_hit;

    if (!(infoPtr->dwStyle & TVS_TRACKSELECT)) return 0;

    /* fill in the TRACKMOUSEEVENT struct */
    trackinfo.cbSize = sizeof(TRACKMOUSEEVENT);
    trackinfo.dwFlags = TME_QUERY;
    trackinfo.hwndTrack = infoPtr->hwnd;

    /* call _TrackMouseEvent to see if we are currently tracking for this hwnd */
    _TrackMouseEvent(&trackinfo);

    /* Make sure tracking is enabled so we receive a WM_MOUSELEAVE message */
    if(!(trackinfo.dwFlags & TME_LEAVE))
    {
        trackinfo.dwFlags = TME_LEAVE; /* notify upon leaving */
        trackinfo.hwndTrack = infoPtr->hwnd;
        /* do it as fast as possible, minimal systimer latency will be used */
        trackinfo.dwHoverTime = 1;

        /* call TRACKMOUSEEVENT so we receive a WM_MOUSELEAVE message */
        /* and can properly deactivate the hot item */
        _TrackMouseEvent(&trackinfo);
    }

    ht.pt.x = (short)LOWORD(lParam);
    ht.pt.y = (short)HIWORD(lParam);

    item = TREEVIEW_HitTest(infoPtr, &ht);
    item_hit = TREEVIEW_IsItemHit(infoPtr, &ht);
    if ((item != infoPtr->hotItem) || !item_hit)
    {
        /* redraw old hot item */
        TREEVIEW_InvalidateItem(infoPtr, infoPtr->hotItem);
        infoPtr->hotItem = NULL;
        if (item && item_hit)
        {
            infoPtr->hotItem = item;
            /* redraw new hot item */
            TREEVIEW_InvalidateItem(infoPtr, infoPtr->hotItem);
        }
    }

    return 0;
}

/* Draw themed border */
static BOOL TREEVIEW_NCPaint (const TREEVIEW_INFO *infoPtr, HRGN region, LPARAM lParam)
{
    HTHEME theme = GetWindowTheme (infoPtr->hwnd);
    HDC dc;
    RECT r;
    HRGN cliprgn;
    int cxEdge = GetSystemMetrics (SM_CXEDGE),
        cyEdge = GetSystemMetrics (SM_CYEDGE);

    if (!theme)
        return DefWindowProcW (infoPtr->hwnd, WM_NCPAINT, (WPARAM)region, lParam);

    GetWindowRect(infoPtr->hwnd, &r);

    cliprgn = CreateRectRgn (r.left + cxEdge, r.top + cyEdge,
        r.right - cxEdge, r.bottom - cyEdge);
    if (region != (HRGN)1)
        CombineRgn (cliprgn, cliprgn, region, RGN_AND);
    OffsetRect(&r, -r.left, -r.top);

#ifdef __REACTOS__ /* r73789 */
    dc = GetWindowDC(infoPtr->hwnd);
    /* Exclude client part */
    ExcludeClipRect(dc, r.left + cxEdge, r.top + cyEdge,
        r.right - cxEdge, r.bottom -cyEdge);
#else
    dc = GetDCEx(infoPtr->hwnd, region, DCX_WINDOW|DCX_INTERSECTRGN);
    OffsetRect(&r, -r.left, -r.top);
#endif

    if (IsThemeBackgroundPartiallyTransparent (theme, 0, 0))
        DrawThemeParentBackground(infoPtr->hwnd, dc, &r);
    DrawThemeBackground (theme, dc, 0, 0, &r, 0);
    ReleaseDC(infoPtr->hwnd, dc);

    /* Call default proc to get the scrollbars etc. painted */
    DefWindowProcW (infoPtr->hwnd, WM_NCPAINT, (WPARAM)cliprgn, 0);
    DeleteObject(cliprgn);

    return TRUE;
}

static LRESULT
TREEVIEW_Notify(const TREEVIEW_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    LPNMHDR lpnmh = (LPNMHDR)lParam;

    if (lpnmh->code == PGN_CALCSIZE) {
	LPNMPGCALCSIZE lppgc = (LPNMPGCALCSIZE)lParam;

	if (lppgc->dwFlag == PGF_CALCWIDTH) {
	    lppgc->iWidth = infoPtr->treeWidth;
            TRACE("got PGN_CALCSIZE, returning horz size = %d, client=%d\n",
		  infoPtr->treeWidth, infoPtr->clientWidth);
	}
	else {
	    lppgc->iHeight = infoPtr->treeHeight;
            TRACE("got PGN_CALCSIZE, returning vert size = %d, client=%d\n",
		  infoPtr->treeHeight, infoPtr->clientHeight);
	}
	return 0;
    }
    return DefWindowProcW(infoPtr->hwnd, WM_NOTIFY, wParam, lParam);
}

static LRESULT
TREEVIEW_Size(TREEVIEW_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    if (wParam == SIZE_RESTORED)
    {
	infoPtr->clientWidth  = (short)LOWORD(lParam);
	infoPtr->clientHeight = (short)HIWORD(lParam);

	TREEVIEW_RecalculateVisibleOrder(infoPtr, NULL);
	TREEVIEW_SetFirstVisible(infoPtr, infoPtr->firstVisible, TRUE);
	TREEVIEW_UpdateScrollBars(infoPtr);
    }
    else
    {
	FIXME("WM_SIZE flag %lx %lx not handled\n", wParam, lParam);
    }

    TREEVIEW_Invalidate(infoPtr, NULL);
    return 0;
}

static LRESULT
TREEVIEW_StyleChanged(TREEVIEW_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    TRACE("(%lx %lx)\n", wParam, lParam);

    if (wParam == GWL_STYLE)
    {
        DWORD dwNewStyle = ((LPSTYLESTRUCT)lParam)->styleNew;

        if ((infoPtr->dwStyle ^ dwNewStyle) & TVS_CHECKBOXES)
        {
            if (dwNewStyle & TVS_CHECKBOXES)
            {
                TREEVIEW_InitCheckboxes(infoPtr);
                TRACE("checkboxes enabled\n");

                /* set all items to state image index 1 */
                TREEVIEW_ResetImageStateIndex(infoPtr, infoPtr->root);
            }
            else
            {
                FIXME("tried to disable checkboxes\n");
            }
        }

        if ((infoPtr->dwStyle ^ dwNewStyle) & TVS_NOTOOLTIPS)
        {
            if (infoPtr->dwStyle & TVS_NOTOOLTIPS)
            {
                infoPtr->hwndToolTip = COMCTL32_CreateToolTip(infoPtr->hwnd);
                TRACE("tooltips enabled\n");
            }
            else
            {
                DestroyWindow(infoPtr->hwndToolTip);
                infoPtr->hwndToolTip = 0;
                TRACE("tooltips disabled\n");
            }
        }

        infoPtr->dwStyle = dwNewStyle;
    }

    TREEVIEW_EndEditLabelNow(infoPtr, TRUE);
    TREEVIEW_UpdateSubTree(infoPtr, infoPtr->root);
    TREEVIEW_UpdateScrollBars(infoPtr);
    TREEVIEW_Invalidate(infoPtr, NULL);

    return 0;
}

static LRESULT
TREEVIEW_SetCursor(const TREEVIEW_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    TREEVIEW_ITEM * item;
    TVHITTESTINFO ht;
    NMMOUSE nmmouse;

    GetCursorPos(&ht.pt);
    ScreenToClient(infoPtr->hwnd, &ht.pt);

    item = TREEVIEW_HitTest(infoPtr, &ht);

    memset(&nmmouse, 0, sizeof(nmmouse));
    if (item)
    {
        nmmouse.dwItemSpec = (DWORD_PTR)item;
        nmmouse.dwItemData = item->lParam;
    }
    nmmouse.pt.x = 0;
    nmmouse.pt.y = 0;
    nmmouse.dwHitInfo = lParam;
    if (TREEVIEW_SendRealNotify(infoPtr, NM_SETCURSOR, &nmmouse.hdr))
        return 0;

    if (item && (infoPtr->dwStyle & TVS_TRACKSELECT) && TREEVIEW_IsItemHit(infoPtr, &ht))
    {
        SetCursor(infoPtr->hcurHand);
        return 0;
    }
    else
        return DefWindowProcW(infoPtr->hwnd, WM_SETCURSOR, wParam, lParam);
}

static LRESULT
TREEVIEW_SetFocus(TREEVIEW_INFO *infoPtr)
{
    TRACE("\n");

    if (!infoPtr->selectedItem)
    {
	TREEVIEW_DoSelectItem(infoPtr, TVGN_CARET, infoPtr->firstVisible,
			      TVC_UNKNOWN);
    }

    TREEVIEW_Invalidate(infoPtr, infoPtr->selectedItem);
    TREEVIEW_SendSimpleNotify(infoPtr, NM_SETFOCUS);
    return 0;
}

static LRESULT
TREEVIEW_KillFocus(const TREEVIEW_INFO *infoPtr)
{
    TRACE("\n");

    TREEVIEW_Invalidate(infoPtr, infoPtr->selectedItem);
    UpdateWindow(infoPtr->hwnd);
    TREEVIEW_SendSimpleNotify(infoPtr, NM_KILLFOCUS);
    return 0;
}

/* update theme after a WM_THEMECHANGED message */
static LRESULT TREEVIEW_ThemeChanged(const TREEVIEW_INFO *infoPtr)
{
    HTHEME theme = GetWindowTheme (infoPtr->hwnd);
    CloseThemeData (theme);
    OpenThemeData (infoPtr->hwnd, themeClass);
    return 0;
}


static LRESULT WINAPI
TREEVIEW_WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);

    TRACE("hwnd %p msg %04x wp=%08lx lp=%08lx\n", hwnd, uMsg, wParam, lParam);

    if (infoPtr) TREEVIEW_VerifyTree(infoPtr);
    else
    {
	if (uMsg == WM_CREATE)
	    TREEVIEW_Create(hwnd, (LPCREATESTRUCTW)lParam);
	else
	    goto def;
    }

    switch (uMsg)
    {
    case TVM_CREATEDRAGIMAGE:
	return TREEVIEW_CreateDragImage(infoPtr, lParam);

    case TVM_DELETEITEM:
	return TREEVIEW_DeleteItem(infoPtr, (HTREEITEM)lParam);

    case TVM_EDITLABELA:
    case TVM_EDITLABELW:
	return (LRESULT)TREEVIEW_EditLabel(infoPtr, (HTREEITEM)lParam);

    case TVM_ENDEDITLABELNOW:
	return TREEVIEW_EndEditLabelNow(infoPtr, (BOOL)wParam);

    case TVM_ENSUREVISIBLE:
	return TREEVIEW_EnsureVisible(infoPtr, (HTREEITEM)lParam, TRUE);

    case TVM_EXPAND:
	return TREEVIEW_ExpandMsg(infoPtr, (UINT)wParam, (HTREEITEM)lParam);

    case TVM_GETBKCOLOR:
	return TREEVIEW_GetBkColor(infoPtr);

    case TVM_GETCOUNT:
	return TREEVIEW_GetCount(infoPtr);

    case TVM_GETEDITCONTROL:
	return TREEVIEW_GetEditControl(infoPtr);

    case TVM_GETIMAGELIST:
	return TREEVIEW_GetImageList(infoPtr, wParam);

    case TVM_GETINDENT:
	return TREEVIEW_GetIndent(infoPtr);

    case TVM_GETINSERTMARKCOLOR:
	return TREEVIEW_GetInsertMarkColor(infoPtr);

    case TVM_GETISEARCHSTRINGA:
	FIXME("Unimplemented msg TVM_GETISEARCHSTRINGA\n");
	return 0;

    case TVM_GETISEARCHSTRINGW:
	FIXME("Unimplemented msg TVM_GETISEARCHSTRINGW\n");
	return 0;

    case TVM_GETITEMA:
    case TVM_GETITEMW:
	return TREEVIEW_GetItemT(infoPtr, (LPTVITEMEXW)lParam,
	                         uMsg == TVM_GETITEMW);
    case TVM_GETITEMHEIGHT:
	return TREEVIEW_GetItemHeight(infoPtr);

    case TVM_GETITEMRECT:
	return TREEVIEW_GetItemRect(infoPtr, (BOOL)wParam, (LPRECT)lParam);

    case TVM_GETITEMSTATE:
	return TREEVIEW_GetItemState(infoPtr, (HTREEITEM)wParam, (UINT)lParam);

    case TVM_GETLINECOLOR:
	return TREEVIEW_GetLineColor(infoPtr);

    case TVM_GETNEXTITEM:
	return TREEVIEW_GetNextItem(infoPtr, (UINT)wParam, (HTREEITEM)lParam);

    case TVM_GETSCROLLTIME:
	return TREEVIEW_GetScrollTime(infoPtr);

    case TVM_GETTEXTCOLOR:
	return TREEVIEW_GetTextColor(infoPtr);

    case TVM_GETTOOLTIPS:
	return TREEVIEW_GetToolTips(infoPtr);

    case TVM_GETUNICODEFORMAT:
        return TREEVIEW_GetUnicodeFormat(infoPtr);

    case TVM_GETVISIBLECOUNT:
	return TREEVIEW_GetVisibleCount(infoPtr);

    case TVM_HITTEST:
	return (LRESULT)TREEVIEW_HitTest(infoPtr, (TVHITTESTINFO*)lParam);

    case TVM_INSERTITEMA:
    case TVM_INSERTITEMW:
	return TREEVIEW_InsertItemT(infoPtr, (LPTVINSERTSTRUCTW)lParam,
	                            uMsg == TVM_INSERTITEMW);
    case TVM_SELECTITEM:
	return TREEVIEW_SelectItem(infoPtr, (INT)wParam, (HTREEITEM)lParam);

    case TVM_SETBKCOLOR:
	return TREEVIEW_SetBkColor(infoPtr, (COLORREF)lParam);

    case TVM_SETIMAGELIST:
	return TREEVIEW_SetImageList(infoPtr, wParam, (HIMAGELIST)lParam);

    case TVM_SETINDENT:
	return TREEVIEW_SetIndent(infoPtr, (UINT)wParam);

    case TVM_SETINSERTMARK:
	return TREEVIEW_SetInsertMark(infoPtr, (BOOL)wParam, (HTREEITEM)lParam);

    case TVM_SETINSERTMARKCOLOR:
	return TREEVIEW_SetInsertMarkColor(infoPtr, (COLORREF)lParam);

    case TVM_SETITEMA:
    case TVM_SETITEMW:
        return TREEVIEW_SetItemT(infoPtr, (LPTVITEMEXW)lParam,
	                         uMsg == TVM_SETITEMW);
    case TVM_SETLINECOLOR:
	return TREEVIEW_SetLineColor(infoPtr, (COLORREF)lParam);

    case TVM_SETITEMHEIGHT:
	return TREEVIEW_SetItemHeight(infoPtr, (INT)(SHORT)wParam);

    case TVM_SETSCROLLTIME:
	return TREEVIEW_SetScrollTime(infoPtr, (UINT)wParam);

    case TVM_SETTEXTCOLOR:
	return TREEVIEW_SetTextColor(infoPtr, (COLORREF)lParam);

    case TVM_SETTOOLTIPS:
	return TREEVIEW_SetToolTips(infoPtr, (HWND)wParam);

    case TVM_SETUNICODEFORMAT:
        return TREEVIEW_SetUnicodeFormat(infoPtr, (BOOL)wParam);

    case TVM_SORTCHILDREN:
	return TREEVIEW_SortChildren(infoPtr, lParam);

    case TVM_SORTCHILDRENCB:
	return TREEVIEW_SortChildrenCB(infoPtr, (LPTVSORTCB)lParam);

    case WM_CHAR:
        return TREEVIEW_ProcessLetterKeys(infoPtr, wParam, lParam);

    case WM_COMMAND:
	return TREEVIEW_Command(infoPtr, wParam, lParam);

    case WM_DESTROY:
	return TREEVIEW_Destroy(infoPtr);

	/* WM_ENABLE */

    case WM_ERASEBKGND:
	return TREEVIEW_EraseBackground(infoPtr, (HDC)wParam);

    case WM_GETDLGCODE:
	return DLGC_WANTARROWS | DLGC_WANTCHARS;

    case WM_GETFONT:
	return TREEVIEW_GetFont(infoPtr);

    case WM_HSCROLL:
	return TREEVIEW_HScroll(infoPtr, wParam);

    case WM_KEYDOWN:
#ifndef __REACTOS__
    case WM_SYSKEYDOWN:
#endif
	return TREEVIEW_KeyDown(infoPtr, wParam);

    case WM_KILLFOCUS:
	return TREEVIEW_KillFocus(infoPtr);

    case WM_LBUTTONDBLCLK:
	return TREEVIEW_LButtonDoubleClick(infoPtr, lParam);

    case WM_LBUTTONDOWN:
	return TREEVIEW_LButtonDown(infoPtr, lParam);

	/* WM_MBUTTONDOWN */

    case WM_MOUSELEAVE:
	return TREEVIEW_MouseLeave(infoPtr);

    case WM_MOUSEMOVE:
	return TREEVIEW_MouseMove(infoPtr, lParam);

    case WM_NCLBUTTONDOWN:
        if (infoPtr->hwndEdit)
            SetFocus(infoPtr->hwnd);
        goto def;

    case WM_NCPAINT:
        return TREEVIEW_NCPaint (infoPtr, (HRGN)wParam, lParam);

    case WM_NOTIFY:
	return TREEVIEW_Notify(infoPtr, wParam, lParam);

    case WM_NOTIFYFORMAT:
	return TREEVIEW_NotifyFormat(infoPtr, (HWND)wParam, (UINT)lParam);

    case WM_PRINTCLIENT:
        return TREEVIEW_PrintClient(infoPtr, (HDC)wParam, lParam);

    case WM_PAINT:
	return TREEVIEW_Paint(infoPtr, (HDC)wParam);

    case WM_RBUTTONDOWN:
	return TREEVIEW_RButtonDown(infoPtr, lParam);

    case WM_SETCURSOR:
	return TREEVIEW_SetCursor(infoPtr, wParam, lParam);

    case WM_SETFOCUS:
	return TREEVIEW_SetFocus(infoPtr);

    case WM_SETFONT:
	return TREEVIEW_SetFont(infoPtr, (HFONT)wParam, (BOOL)lParam);

    case WM_SETREDRAW:
        return TREEVIEW_SetRedraw(infoPtr, wParam);

    case WM_SIZE:
	return TREEVIEW_Size(infoPtr, wParam, lParam);

    case WM_STYLECHANGED:
	return TREEVIEW_StyleChanged(infoPtr, wParam, lParam);

    case WM_SYSCOLORCHANGE:
        COMCTL32_RefreshSysColors();
        return 0;

    case WM_TIMER:
	return TREEVIEW_HandleTimer(infoPtr, wParam);

    case WM_THEMECHANGED:
        return TREEVIEW_ThemeChanged (infoPtr);

    case WM_VSCROLL:
	return TREEVIEW_VScroll(infoPtr, wParam);

	/* WM_WININICHANGE */

    case WM_MOUSEWHEEL:
	return TREEVIEW_MouseWheel(infoPtr, wParam, lParam);

    case WM_DRAWITEM:
	TRACE("drawItem\n");
	goto def;

    default:
	/* This mostly catches MFC and Delphi messages. :( */
	if ((uMsg >= WM_USER) && (uMsg < WM_APP) && !COMCTL32_IsReflectedMessage(uMsg))
	    TRACE("Unknown msg %04x wp=%08lx lp=%08lx\n", uMsg, wParam, lParam);
def:
	return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
}


/* Class Registration ***************************************************/

VOID
TREEVIEW_Register(void)
{
    WNDCLASSW wndClass;

    TRACE("\n");

    ZeroMemory(&wndClass, sizeof(WNDCLASSW));
    wndClass.style = CS_GLOBALCLASS | CS_DBLCLKS;
    wndClass.lpfnWndProc = TREEVIEW_WindowProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = sizeof(TREEVIEW_INFO *);

    wndClass.hCursor = LoadCursorW(0, (LPWSTR)IDC_ARROW);
    wndClass.hbrBackground = 0;
    wndClass.lpszClassName = WC_TREEVIEWW;

    RegisterClassW(&wndClass);
}


VOID
TREEVIEW_Unregister(void)
{
    UnregisterClassW(WC_TREEVIEWW, NULL);
}


/* Tree Verification ****************************************************/

static inline void
TREEVIEW_VerifyChildren(TREEVIEW_INFO *infoPtr, const TREEVIEW_ITEM *item);

static inline void TREEVIEW_VerifyItemCommon(TREEVIEW_INFO *infoPtr,
					     const TREEVIEW_ITEM *item)
{
    assert(infoPtr != NULL);
    assert(item != NULL);

    /* both NULL, or both non-null */
    assert((item->firstChild == NULL) == (item->lastChild == NULL));

    assert(item->firstChild != item);
    assert(item->lastChild != item);

    if (item->firstChild)
    {
	assert(item->firstChild->parent == item);
	assert(item->firstChild->prevSibling == NULL);
    }

    if (item->lastChild)
    {
	assert(item->lastChild->parent == item);
	assert(item->lastChild->nextSibling == NULL);
    }

    assert(item->nextSibling != item);
    if (item->nextSibling)
    {
	assert(item->nextSibling->parent == item->parent);
	assert(item->nextSibling->prevSibling == item);
    }

    assert(item->prevSibling != item);
    if (item->prevSibling)
    {
	assert(item->prevSibling->parent == item->parent);
	assert(item->prevSibling->nextSibling == item);
    }
}

static inline void
TREEVIEW_VerifyItem(TREEVIEW_INFO *infoPtr, const TREEVIEW_ITEM *item)
{
    assert(item != NULL);

    assert(item->parent != NULL);
    assert(item->parent != item);
    assert(item->iLevel == item->parent->iLevel + 1);

    assert(DPA_GetPtrIndex(infoPtr->items, item) != -1);

    TREEVIEW_VerifyItemCommon(infoPtr, item);

    TREEVIEW_VerifyChildren(infoPtr, item);
}

static inline void
TREEVIEW_VerifyChildren(TREEVIEW_INFO *infoPtr, const TREEVIEW_ITEM *item)
{
    const TREEVIEW_ITEM *child;
    assert(item != NULL);

    for (child = item->firstChild; child != NULL; child = child->nextSibling)
	TREEVIEW_VerifyItem(infoPtr, child);
}

static inline void
TREEVIEW_VerifyRoot(TREEVIEW_INFO *infoPtr)
{
    TREEVIEW_ITEM *root = infoPtr->root;

    assert(root != NULL);
    assert(root->iLevel == -1);
    assert(root->parent == NULL);
    assert(root->prevSibling == NULL);

    TREEVIEW_VerifyItemCommon(infoPtr, root);

    TREEVIEW_VerifyChildren(infoPtr, root);
}

static void
TREEVIEW_VerifyTree(TREEVIEW_INFO *infoPtr)
{
    if (!TRACE_ON(treeview)) return;

    assert(infoPtr != NULL);
    TREEVIEW_VerifyRoot(infoPtr);
}
