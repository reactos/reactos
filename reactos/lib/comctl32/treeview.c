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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Note that TREEVIEW_INFO * and HTREEITEM are the same thing.
 *
 * Note2: All items always! have valid (allocated) pszText field.
 *      If item's text == LPSTR_TEXTCALLBACKA we allocate buffer
 *      of size TEXT_CALLBACK_SIZE in DoSetItem.
 *      We use callbackMask to keep track of fields to be updated.
 *
 * TODO:
 *   missing notifications: NM_SETCURSOR, TVN_GETINFOTIP, TVN_KEYDOWN,
 *      TVN_SETDISPINFO, TVN_SINGLEEXPAND
 *
 *   missing styles: TVS_FULLROWSELECT, TVS_INFOTIP, TVS_RTLREADING,
 *      TVS_TRACKSELECT
 *
 *   missing item styles: TVIS_CUT, TVIS_EXPANDPARTIAL
 *
 *   Make the insertion mark look right.
 *   Scroll (instead of repaint) as much as possible.
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "commctrl.h"
#include "comctl32.h"
#include "wine/unicode.h"
#include "wine/debug.h"

/* internal structures */

typedef struct _TREEITEM    /* HTREEITEM is a _TREEINFO *. */
{
  UINT      callbackMask;
  UINT      state;
  UINT      stateMask;
  LPWSTR    pszText;
  int       cchTextMax;
  int       iImage;
  int       iSelectedImage;
  int       cChildren;
  LPARAM    lParam;
  int       iIntegral;      /* item height multiplier (1 is normal) */
  int       iLevel;         /* indentation level:0=root level */
  HTREEITEM parent;         /* handle to parent or 0 if at root */
  HTREEITEM firstChild;     /* handle to first child or 0 if no child */
  HTREEITEM lastChild;
  HTREEITEM prevSibling;    /* handle to prev item in list, 0 if first */
  HTREEITEM nextSibling;    /* handle to next item in list, 0 if last */
  RECT      rect;
  LONG      linesOffset;
  LONG      stateOffset;
  LONG      imageOffset;
  LONG      textOffset;
  LONG      textWidth;      /* horizontal text extent for pszText */
  LONG      visibleOrder;   /* visible ordering, 0 is first visible item */
} TREEVIEW_ITEM;


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
  BOOL		bRedraw;	/* if FALSE we validate but don't redraw in TREEVIEW_Paint() */

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

  HTREEITEM     firstVisible;   /* handle to first visible item */
  LONG          maxVisibleOrder;
  HTREEITEM     dropItem;       /* handle to item selected by drag cursor */
  HTREEITEM     insertMarkItem; /* item after which insertion mark is placed */
  BOOL          insertBeforeorAfter; /* flag used by TVM_SETINSERTMARK */
  HIMAGELIST    dragList;       /* Bitmap of dragged item */
  LONG          scrollX;
  COLORREF      clrBk;
  COLORREF      clrText;
  COLORREF      clrLine;
  COLORREF      clrInsertMark;
  HFONT         hFont;
  HFONT         hBoldFont;
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

  DWORD lastKeyPressTimestamp; /* Added */
  WPARAM charCode; /* Added */
  INT nSearchParamLength; /* Added */
  WCHAR szSearchParam[ MAX_PATH ]; /* Added */
} TREEVIEW_INFO;


/******** Defines that TREEVIEW_ProcessLetterKeys uses ****************/
#define KEY_DELAY       450

/* bitflags for infoPtr->uInternalStatus */

#define TV_HSCROLL 	0x01    /* treeview too large to fit in window */
#define TV_VSCROLL 	0x02	/* (horizontal/vertical) */
#define TV_LDRAG		0x04	/* Lbutton pushed to start drag */
#define TV_LDRAGGING	0x08	/* Lbutton pushed, mouse moved. */
#define TV_RDRAG		0x10	/* dito Rbutton */
#define TV_RDRAGGING	0x20

/* bitflags for infoPtr->timer */

#define TV_EDIT_TIMER    2
#define TV_EDIT_TIMER_SET 2


VOID TREEVIEW_Register (VOID);
VOID TREEVIEW_Unregister (VOID);


WINE_DEFAULT_DEBUG_CHANNEL(treeview);


#define TEXT_CALLBACK_SIZE 260

#define TREEVIEW_LEFT_MARGIN 8

#define MINIMUM_INDENT 19

#define CALLBACK_MASK_ALL (TVIF_TEXT|TVIF_CHILDREN|TVIF_IMAGE|TVIF_SELECTEDIMAGE)

#define STATEIMAGEINDEX(x) (((x) >> 12) & 0x0f)
#define OVERLAYIMAGEINDEX(x) (((x) >> 8) & 0x0f)
#define ISVISIBLE(x)         ((x)->visibleOrder >= 0)


typedef VOID (*TREEVIEW_ItemEnumFunc)(TREEVIEW_INFO *, TREEVIEW_ITEM *,LPVOID);


static VOID TREEVIEW_Invalidate(TREEVIEW_INFO *, TREEVIEW_ITEM *);

static LRESULT TREEVIEW_DoSelectItem(TREEVIEW_INFO *, INT, HTREEITEM, INT);
static VOID TREEVIEW_SetFirstVisible(TREEVIEW_INFO *, TREEVIEW_ITEM *, BOOL);
static LRESULT TREEVIEW_EnsureVisible(TREEVIEW_INFO *, HTREEITEM, BOOL);
static LRESULT TREEVIEW_RButtonUp(TREEVIEW_INFO *, LPPOINT);
static LRESULT TREEVIEW_EndEditLabelNow(TREEVIEW_INFO *infoPtr, BOOL bCancel);
static VOID TREEVIEW_UpdateScrollBars(TREEVIEW_INFO *infoPtr);
static LRESULT TREEVIEW_HScroll(TREEVIEW_INFO *, WPARAM);
static INT TREEVIEW_NotifyFormat (TREEVIEW_INFO *infoPtr, HWND wParam, UINT lParam);


/* Random Utilities *****************************************************/

#ifndef NDEBUG
static inline void
TREEVIEW_VerifyTree(TREEVIEW_INFO *infoPtr)
{
    (void)infoPtr;
}
#else
/* The definition is at the end of the file. */
static void TREEVIEW_VerifyTree(TREEVIEW_INFO *infoPtr);
#endif

/* Returns the treeview private data if hwnd is a treeview.
 * Otherwise returns an undefined value. */
static TREEVIEW_INFO *
TREEVIEW_GetInfoPtr(HWND hwnd)
{
    return (TREEVIEW_INFO *)GetWindowLongW(hwnd, 0);
}

/* Don't call this. Nothing wants an item index. */
static inline int
TREEVIEW_GetItemIndex(TREEVIEW_INFO *infoPtr, HTREEITEM handle)
{
    assert(infoPtr != NULL);

    return DPA_GetPtrIndex(infoPtr->items, handle);
}

/***************************************************************************
 * This method checks that handle is an item for this tree.
 */
static BOOL
TREEVIEW_ValidItem(TREEVIEW_INFO *infoPtr, HTREEITEM handle)
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
    LOGFONTA font;

    GetObjectA(hOrigFont, sizeof(font), &font);
    font.lfWeight = FW_BOLD;
    return CreateFontIndirectA(&font);
}

static inline HFONT
TREEVIEW_FontForItem(TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *item)
{
    return (item->state & TVIS_BOLD) ? infoPtr->hBoldFont : infoPtr->hFont;
}

/* for trace/debugging purposes only */
static const char *
TREEVIEW_ItemName(TREEVIEW_ITEM *item)
{
    if (item == NULL) return "<null item>";
    if (item->pszText == LPSTR_TEXTCALLBACKW) return "<callback>";
    if (item->pszText == NULL) return "<null>";
    return debugstr_w(item->pszText);
}

/* An item is not a child of itself. */
static BOOL
TREEVIEW_IsChildOf(TREEVIEW_ITEM *parent, TREEVIEW_ITEM *child)
{
    do
    {
	child = child->parent;
	if (child == parent) return TRUE;
    } while (child != NULL);

    return FALSE;
}


/* Tree Traversal *******************************************************/

/***************************************************************************
 * This method returns the last expanded sibling or child child item
 * of a tree node
 */
static TREEVIEW_ITEM *
TREEVIEW_GetLastListItem(TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *wineItem)
{
    if (!wineItem)
       return NULL;

    while (wineItem->lastChild)
    {
       if (wineItem->state & TVIS_EXPANDED)
          wineItem = wineItem->lastChild;
       else
          break;
    }

    if (wineItem == infoPtr->root)
        return NULL;

    return wineItem;
}

/***************************************************************************
 * This method returns the previous non-hidden item in the list not
 * considering the tree hierarchy.
 */
static TREEVIEW_ITEM *
TREEVIEW_GetPrevListItem(TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *tvItem)
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
TREEVIEW_GetNextListItem(TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *tvItem)
{
    assert(tvItem != NULL);

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
TREEVIEW_GetListItem(TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *wineItem,
		     LONG count)
{
    TREEVIEW_ITEM *(*next_item)(TREEVIEW_INFO *, TREEVIEW_ITEM *);
    TREEVIEW_ITEM *previousItem;

    assert(wineItem != NULL);

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
	return wineItem;

    do
    {
	previousItem = wineItem;
	wineItem = next_item(infoPtr, wineItem);

    } while (--count && wineItem != NULL);


    return wineItem ? wineItem : previousItem;
}

/* Notifications ************************************************************/

static INT get_notifycode(TREEVIEW_INFO *infoPtr, INT code)
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

static LRESULT
TREEVIEW_SendRealNotify(TREEVIEW_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    TRACE("wParam=%d, lParam=%ld\n", wParam, lParam);
    return SendMessageW(infoPtr->hwndNotify, WM_NOTIFY, wParam, lParam);
}

static BOOL
TREEVIEW_SendSimpleNotify(TREEVIEW_INFO *infoPtr, UINT code)
{
    NMHDR nmhdr;
    HWND hwnd = infoPtr->hwnd;

    TRACE("%d\n", code);
    nmhdr.hwndFrom = hwnd;
    nmhdr.idFrom = GetWindowLongW(hwnd, GWL_ID);
    nmhdr.code = get_notifycode(infoPtr, code);

    return (BOOL)TREEVIEW_SendRealNotify(infoPtr,
				  (WPARAM)nmhdr.idFrom, (LPARAM)&nmhdr);
}

static VOID
TREEVIEW_TVItemFromItem(TREEVIEW_INFO *infoPtr, UINT mask, TVITEMW *tvItem, TREEVIEW_ITEM *item)
{
    tvItem->mask = mask;
    tvItem->hItem = item;
    tvItem->state = item->state;
    tvItem->stateMask = 0;
    tvItem->iImage = item->iImage;
    tvItem->iImage = item->iImage;
    tvItem->iSelectedImage = item->iSelectedImage;
    tvItem->cChildren = item->cChildren;
    tvItem->lParam = item->lParam;

    if(mask & TVIF_TEXT)
    {
        if (!infoPtr->bNtfUnicode)
        {
            tvItem->cchTextMax = WideCharToMultiByte( CP_ACP, 0, item->pszText, -1, NULL, 0, NULL, NULL );
            tvItem->pszText = Alloc (tvItem->cchTextMax);
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
TREEVIEW_SendTreeviewNotify(TREEVIEW_INFO *infoPtr, UINT code, UINT action,
			    UINT mask, HTREEITEM oldItem, HTREEITEM newItem)
{
    HWND hwnd = infoPtr->hwnd;
    NMTREEVIEWW nmhdr;
    BOOL ret;

    TRACE("code:%d action:%x olditem:%p newitem:%p\n",
	  code, action, oldItem, newItem);

    ZeroMemory(&nmhdr, sizeof(NMTREEVIEWA));

    nmhdr.hdr.hwndFrom = hwnd;
    nmhdr.hdr.idFrom = GetWindowLongW(hwnd, GWL_ID);
    nmhdr.hdr.code = get_notifycode(infoPtr, code);
    nmhdr.action = action;

    if (oldItem)
	TREEVIEW_TVItemFromItem(infoPtr, mask, &nmhdr.itemOld, oldItem);

    if (newItem)
	TREEVIEW_TVItemFromItem(infoPtr, mask, &nmhdr.itemNew, newItem);

    nmhdr.ptDrag.x = 0;
    nmhdr.ptDrag.y = 0;

    ret = (BOOL)TREEVIEW_SendRealNotify(infoPtr,
                              (WPARAM)nmhdr.hdr.idFrom,
			      (LPARAM)&nmhdr);
    if (!infoPtr->bNtfUnicode)
    {
	Free(nmhdr.itemOld.pszText);
	Free(nmhdr.itemNew.pszText);
    }
    return ret;
}

static BOOL
TREEVIEW_SendTreeviewDnDNotify(TREEVIEW_INFO *infoPtr, UINT code,
			       HTREEITEM dragItem, POINT pt)
{
    HWND hwnd = infoPtr->hwnd;
    NMTREEVIEWW nmhdr;

    TRACE("code:%d dragitem:%p\n", code, dragItem);

    nmhdr.hdr.hwndFrom = hwnd;
    nmhdr.hdr.idFrom = GetWindowLongW(hwnd, GWL_ID);
    nmhdr.hdr.code = get_notifycode(infoPtr, code);
    nmhdr.action = 0;
    nmhdr.itemNew.mask = TVIF_STATE | TVIF_PARAM | TVIF_HANDLE;
    nmhdr.itemNew.hItem = dragItem;
    nmhdr.itemNew.state = dragItem->state;
    nmhdr.itemNew.lParam = dragItem->lParam;

    nmhdr.ptDrag.x = pt.x;
    nmhdr.ptDrag.y = pt.y;

    return (BOOL)TREEVIEW_SendRealNotify(infoPtr,
			      (WPARAM)nmhdr.hdr.idFrom,
			      (LPARAM)&nmhdr);
}


static BOOL
TREEVIEW_SendCustomDrawNotify(TREEVIEW_INFO *infoPtr, DWORD dwDrawStage,
			      HDC hdc, RECT rc)
{
    HWND hwnd = infoPtr->hwnd;
    NMTVCUSTOMDRAW nmcdhdr;
    LPNMCUSTOMDRAW nmcd;

    TRACE("drawstage:%lx hdc:%p\n", dwDrawStage, hdc);

    nmcd = &nmcdhdr.nmcd;
    nmcd->hdr.hwndFrom = hwnd;
    nmcd->hdr.idFrom = GetWindowLongW(hwnd, GWL_ID);
    nmcd->hdr.code = NM_CUSTOMDRAW;
    nmcd->dwDrawStage = dwDrawStage;
    nmcd->hdc = hdc;
    nmcd->rc = rc;
    nmcd->dwItemSpec = 0;
    nmcd->uItemState = 0;
    nmcd->lItemlParam = 0;
    nmcdhdr.clrText = infoPtr->clrText;
    nmcdhdr.clrTextBk = infoPtr->clrBk;
    nmcdhdr.iLevel = 0;

    return (BOOL)TREEVIEW_SendRealNotify(infoPtr,
			      (WPARAM)nmcd->hdr.idFrom,
			      (LPARAM)&nmcdhdr);
}



/* FIXME: need to find out when the flags in uItemState need to be set */

static BOOL
TREEVIEW_SendCustomDrawItemNotify(TREEVIEW_INFO *infoPtr, HDC hdc,
				  TREEVIEW_ITEM *wineItem, UINT uItemDrawState)
{
    HWND hwnd = infoPtr->hwnd;
    NMTVCUSTOMDRAW nmcdhdr;
    LPNMCUSTOMDRAW nmcd;
    DWORD dwDrawStage, dwItemSpec;
    UINT uItemState;
    INT retval;

    dwDrawStage = CDDS_ITEM | uItemDrawState;
    dwItemSpec = (DWORD)wineItem;
    uItemState = 0;
    if (wineItem->state & TVIS_SELECTED)
	uItemState |= CDIS_SELECTED;
    if (wineItem == infoPtr->selectedItem)
	uItemState |= CDIS_FOCUS;
    if (wineItem == infoPtr->hotItem)
	uItemState |= CDIS_HOT;

    nmcd = &nmcdhdr.nmcd;
    nmcd->hdr.hwndFrom = hwnd;
    nmcd->hdr.idFrom = GetWindowLongW(hwnd, GWL_ID);
    nmcd->hdr.code = NM_CUSTOMDRAW;
    nmcd->dwDrawStage = dwDrawStage;
    nmcd->hdc = hdc;
    nmcd->rc = wineItem->rect;
    nmcd->dwItemSpec = dwItemSpec;
    nmcd->uItemState = uItemState;
    nmcd->lItemlParam = wineItem->lParam;
    nmcdhdr.clrText = infoPtr->clrText;
    nmcdhdr.clrTextBk = infoPtr->clrBk;
    nmcdhdr.iLevel = wineItem->iLevel;

    TRACE("drawstage:%lx hdc:%p item:%lx, itemstate:%x, lItemlParam:%lx\n",
	  nmcd->dwDrawStage, nmcd->hdc, nmcd->dwItemSpec,
	  nmcd->uItemState, nmcd->lItemlParam);

    retval = TREEVIEW_SendRealNotify(infoPtr,
                          (WPARAM)nmcd->hdr.idFrom,
			  (LPARAM)&nmcdhdr);

    infoPtr->clrText = nmcdhdr.clrText;
    infoPtr->clrBk = nmcdhdr.clrTextBk;
    return (BOOL)retval;
}

static BOOL
TREEVIEW_BeginLabelEditNotify(TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *editItem)
{
    HWND hwnd = infoPtr->hwnd;
    NMTVDISPINFOW tvdi;
    BOOL ret;

    tvdi.hdr.hwndFrom = hwnd;
    tvdi.hdr.idFrom = GetWindowLongW(hwnd, GWL_ID);
    tvdi.hdr.code = get_notifycode(infoPtr, TVN_BEGINLABELEDITW);

    TREEVIEW_TVItemFromItem(infoPtr, TVIF_HANDLE | TVIF_STATE | TVIF_PARAM | TVIF_TEXT,
                            &tvdi.item, editItem);

    ret = (BOOL)TREEVIEW_SendRealNotify(infoPtr, tvdi.hdr.idFrom, (LPARAM)&tvdi);

    if (!infoPtr->bNtfUnicode)
	Free(tvdi.item.pszText);

    return ret;
}

static void
TREEVIEW_UpdateDispInfo(TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *wineItem,
			UINT mask)
{
    NMTVDISPINFOW callback;
    HWND hwnd = infoPtr->hwnd;

    TRACE("mask %x callbackMask %x\n", mask, wineItem->callbackMask);
    mask &= wineItem->callbackMask;

    if (mask == 0) return;

    callback.hdr.hwndFrom         = hwnd;
    callback.hdr.idFrom           = GetWindowLongW(hwnd, GWL_ID);
    callback.hdr.code             = get_notifycode(infoPtr, TVN_GETDISPINFOW);

    /* 'state' always contains valid value, as well as 'lParam'.
     * All other parameters are uninitialized.
     */
    callback.item.pszText         = wineItem->pszText;
    callback.item.cchTextMax      = wineItem->cchTextMax;
    callback.item.mask            = mask;
    callback.item.hItem           = wineItem;
    callback.item.state           = wineItem->state;
    callback.item.lParam          = wineItem->lParam;

    /* If text is changed we need to recalculate textWidth */
    if (mask & TVIF_TEXT)
       wineItem->textWidth = 0;

    TREEVIEW_SendRealNotify(infoPtr,
                            (WPARAM)callback.hdr.idFrom, (LPARAM)&callback);

    /* It may have changed due to a call to SetItem. */
    mask &= wineItem->callbackMask;

    if ((mask & TVIF_TEXT) && callback.item.pszText != wineItem->pszText)
    {
	/* Instead of copying text into our buffer user specified its own */
	if (!infoPtr->bNtfUnicode) {
	    LPWSTR newText;
	    int buflen;
            int len = MultiByteToWideChar( CP_ACP, 0,
					   (LPSTR)callback.item.pszText, -1,
                                           NULL, 0);
	    buflen = max((len)*sizeof(WCHAR), TEXT_CALLBACK_SIZE);
	    newText = (LPWSTR)ReAlloc(wineItem->pszText, buflen);

	    TRACE("returned str %s, len=%d, buflen=%d\n",
		  debugstr_a((LPSTR)callback.item.pszText), len, buflen);

	    if (newText)
	    {
		wineItem->pszText = newText;
		MultiByteToWideChar( CP_ACP, 0,
				     (LPSTR)callback.item.pszText, -1,
				     wineItem->pszText, buflen);
		wineItem->cchTextMax = buflen;
	    }
	    /* If ReAlloc fails we have nothing to do, but keep original text */
	}
	else {
	    int len = max(lstrlenW(callback.item.pszText) + 1,
			  TEXT_CALLBACK_SIZE);
	    LPWSTR newText = ReAlloc(wineItem->pszText, len);

	    TRACE("returned wstr %s, len=%d\n",
		  debugstr_w(callback.item.pszText), len);

	    if (newText)
	    {
		wineItem->pszText = newText;
		strcpyW(wineItem->pszText, callback.item.pszText);
		wineItem->cchTextMax = len;
	    }
	    /* If ReAlloc fails we have nothing to do, but keep original text */
	}
    }
    else if (mask & TVIF_TEXT) {
	/* User put text into our buffer, that is ok unless A string */
	if (!infoPtr->bNtfUnicode) {
	    LPWSTR newText;
	    LPWSTR oldText = NULL;
	    int buflen;
            int len = MultiByteToWideChar( CP_ACP, 0,
					  (LPSTR)callback.item.pszText, -1,
                                           NULL, 0);
	    buflen = max((len)*sizeof(WCHAR), TEXT_CALLBACK_SIZE);
	    newText = (LPWSTR)Alloc(buflen);

	    TRACE("same buffer str %s, len=%d, buflen=%d\n",
		  debugstr_a((LPSTR)callback.item.pszText), len, buflen);

	    if (newText)
	    {
		oldText = wineItem->pszText;
		wineItem->pszText = newText;
		MultiByteToWideChar( CP_ACP, 0,
				     (LPSTR)callback.item.pszText, -1,
				     wineItem->pszText, buflen);
		wineItem->cchTextMax = buflen;
		if (oldText)
		    Free(oldText);
	    }
	}
    }

    if (mask & TVIF_IMAGE)
	wineItem->iImage = callback.item.iImage;

    if (mask & TVIF_SELECTEDIMAGE)
	wineItem->iSelectedImage = callback.item.iSelectedImage;

    if (mask & TVIF_CHILDREN)
	wineItem->cChildren = callback.item.cChildren;

    /* These members are now permanently set. */
    if (callback.item.mask & TVIF_DI_SETITEM)
	wineItem->callbackMask &= ~callback.item.mask;
}

/***************************************************************************
 * This function uses cChildren field to decide whether the item has
 * children or not.
 * Note: if this returns TRUE, the child items may not actually exist,
 * they could be virtual.
 *
 * Just use wineItem->firstChild to check for physical children.
 */
static BOOL
TREEVIEW_HasChildren(TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *wineItem)
{
    TREEVIEW_UpdateDispInfo(infoPtr, wineItem, TVIF_CHILDREN);

    return wineItem->cChildren > 0;
}


/* Item Position ********************************************************/

/* Compute linesOffset, stateOffset, imageOffset, textOffset of an item. */
static VOID
TREEVIEW_ComputeItemInternalMetrics(TREEVIEW_INFO *infoPtr,
				    TREEVIEW_ITEM *item)
{
    /* Same effect, different optimisation. */
#if 0
    BOOL lar = ((infoPtr->dwStyle & TVS_LINESATROOT)
		&& (infoPtr->dwStyle & (TVS_HASLINES|TVS_HASBUTTONS)));
#else
    BOOL lar = ((infoPtr->dwStyle
		 & (TVS_LINESATROOT|TVS_HASLINES|TVS_HASBUTTONS))
		> TVS_LINESATROOT);
#endif

    item->linesOffset = infoPtr->uIndent * (item->iLevel + lar - 1)
	- infoPtr->scrollX;
    item->stateOffset = item->linesOffset + infoPtr->uIndent;
    item->imageOffset = item->stateOffset
	+ (STATEIMAGEINDEX(item->state) ? infoPtr->stateImageWidth : 0);
    item->textOffset  = item->imageOffset + infoPtr->normalImageWidth;
}

static VOID
TREEVIEW_ComputeTextWidth(TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *item, HDC hDC)
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

    GetTextExtentPoint32W(hdc, item->pszText, strlenW(item->pszText), &sz);
    item->textWidth = sz.cx;

    if (hDC == 0)
    {
	SelectObject(hdc, hOldFont);
	ReleaseDC(0, hdc);
    }
}

static VOID
TREEVIEW_ComputeItemRect(TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *item)
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
	item->visibleOrder = order;
	order += item->iIntegral;
    }

    infoPtr->maxVisibleOrder = order;

    for (item = start; item != NULL;
	 item = TREEVIEW_GetNextListItem(infoPtr, item))
    {
	TREEVIEW_ComputeItemRect(infoPtr, item);
    }
}


/* Update metrics of all items in selected subtree.
 * root must be expanded
 */
static VOID
TREEVIEW_UpdateSubTree(TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *root)
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
TREEVIEW_AllocateItem(TREEVIEW_INFO *infoPtr)
{
    TREEVIEW_ITEM *newItem = Alloc(sizeof(TREEVIEW_ITEM));

    if (!newItem)
	return NULL;

    if (DPA_InsertPtr(infoPtr->items, INT_MAX, newItem) == -1)
    {
	Free(newItem);
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
    Free(item);
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
    assert(newItem != NULL);
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
    assert(newItem != NULL);
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
TREEVIEW_DoSetItemT(TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *wineItem,
		   const TVITEMEXW *tvItem, BOOL isW)
{
    UINT callbackClear = 0;
    UINT callbackSet = 0;

    TRACE("item %p\n", wineItem);
    /* Do this first in case it fails. */
    if (tvItem->mask & TVIF_TEXT)
    {
        wineItem->textWidth = 0; /* force width recalculation */
	if (tvItem->pszText != LPSTR_TEXTCALLBACKW) /* covers != TEXTCALLBACKA too */
	{
            int len;
            LPWSTR newText;
            if (isW)
                len = lstrlenW(tvItem->pszText) + 1;
            else
                len = MultiByteToWideChar(CP_ACP, 0, (LPSTR)tvItem->pszText, -1, NULL, 0);
            
            newText  = ReAlloc(wineItem->pszText, len * sizeof(WCHAR));

            if (newText == NULL) return FALSE;

            callbackClear |= TVIF_TEXT;

            wineItem->pszText = newText;
            wineItem->cchTextMax = len;
            if (isW)
                lstrcpynW(wineItem->pszText, tvItem->pszText, len);
            else
                MultiByteToWideChar(CP_ACP, 0, (LPSTR)tvItem->pszText, -1,
                                    wineItem->pszText, len);

            TRACE("setting text %s, item %p\n", debugstr_w(wineItem->pszText), wineItem);
        }
	else
	{
	    callbackSet |= TVIF_TEXT;

	    wineItem->pszText = ReAlloc(wineItem->pszText,
                                        TEXT_CALLBACK_SIZE * sizeof(WCHAR));
	    wineItem->cchTextMax = TEXT_CALLBACK_SIZE;
	    TRACE("setting callback, item %p\n", wineItem);
	}
    }

    if (tvItem->mask & TVIF_CHILDREN)
    {
	wineItem->cChildren = tvItem->cChildren;

	if (wineItem->cChildren == I_CHILDRENCALLBACK)
	    callbackSet |= TVIF_CHILDREN;
	else
	    callbackClear |= TVIF_CHILDREN;
    }

    if (tvItem->mask & TVIF_IMAGE)
    {
	wineItem->iImage = tvItem->iImage;

	if (wineItem->iImage == I_IMAGECALLBACK)
	    callbackSet |= TVIF_IMAGE;
	else
	    callbackClear |= TVIF_IMAGE;
    }

    if (tvItem->mask & TVIF_SELECTEDIMAGE)
    {
	wineItem->iSelectedImage = tvItem->iSelectedImage;

	if (wineItem->iSelectedImage == I_IMAGECALLBACK)
	    callbackSet |= TVIF_SELECTEDIMAGE;
	else
	    callbackClear |= TVIF_SELECTEDIMAGE;
    }

    if (tvItem->mask & TVIF_PARAM)
	wineItem->lParam = tvItem->lParam;

    /* If the application sets TVIF_INTEGRAL without
     * supplying a TVITEMEX structure, it's toast. */
    if (tvItem->mask & TVIF_INTEGRAL)
	wineItem->iIntegral = tvItem->iIntegral;

    if (tvItem->mask & TVIF_STATE)
    {
	TRACE("prevstate,state,mask:%x,%x,%x\n", wineItem->state, tvItem->state,
	      tvItem->stateMask);
	wineItem->state &= ~tvItem->stateMask;
	wineItem->state |= (tvItem->state & tvItem->stateMask);
    }

    wineItem->callbackMask |= callbackSet;
    wineItem->callbackMask &= ~callbackClear;

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
	    return (LRESULT)(HTREEITEM)NULL;
	}
    }

    insertAfter = ptdi->hInsertAfter;

    /* Validate this now for convenience. */
    switch ((DWORD)insertAfter)
    {
    case (DWORD)TVI_FIRST:
    case (DWORD)TVI_LAST:
    case (DWORD)TVI_SORT:
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
	return (LRESULT)(HTREEITEM)NULL;

    newItem->parent = parentItem;
    newItem->iIntegral = 1;

    if (!TREEVIEW_DoSetItemT(infoPtr, newItem, tvItem, isW))
	return (LRESULT)(HTREEITEM)NULL;

    /* After this point, nothing can fail. (Except for TVI_SORT.) */

    infoPtr->uNumItems++;

    switch ((DWORD)insertAfter)
    {
    case (DWORD)TVI_FIRST:
        {
           TREEVIEW_ITEM *originalFirst = parentItem->firstChild;
           TREEVIEW_InsertBefore(newItem, parentItem->firstChild, parentItem);
           if (infoPtr->firstVisible == originalFirst)
              TREEVIEW_SetFirstVisible(infoPtr, newItem, TRUE);
        }
	break;

    case (DWORD)TVI_LAST:
	TREEVIEW_InsertAfter(newItem, parentItem->lastChild, parentItem);
	break;

	/* hInsertAfter names a specific item we want to insert after */
    default:
	TREEVIEW_InsertAfter(newItem, insertAfter, insertAfter->parent);
	break;

    case (DWORD)TVI_SORT:
	{
	    TREEVIEW_ITEM *aChild;
	    TREEVIEW_ITEM *previousChild = NULL;
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


    TRACE("new item %p; parent %p, mask %x\n", newItem,
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
       newItem->visibleOrder = -1;

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
TREEVIEW_RemoveItem(TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *wineItem);

static void
TREEVIEW_RemoveAllChildren(TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *parentItem)
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
TREEVIEW_UnlinkItem(TREEVIEW_ITEM *item)
{
    TREEVIEW_ITEM *parentItem = item->parent;

    assert(item != NULL);
    assert(item->parent != NULL); /* i.e. it must not be the root */

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
TREEVIEW_RemoveItem(TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *wineItem)
{
    TRACE("%p, (%s)\n", wineItem, TREEVIEW_ItemName(wineItem));

    TREEVIEW_SendTreeviewNotify(infoPtr, TVN_DELETEITEMW, TVC_UNKNOWN,
				TVIF_HANDLE | TVIF_PARAM, wineItem, 0);

    if (wineItem->firstChild)
	TREEVIEW_RemoveAllChildren(infoPtr, wineItem);

    TREEVIEW_UnlinkItem(wineItem);

    infoPtr->uNumItems--;

    if (wineItem->pszText && wineItem->pszText != LPSTR_TEXTCALLBACKW)
	Free(wineItem->pszText);

    TREEVIEW_FreeItem(infoPtr, wineItem);
}


/* Empty out the tree. */
static void
TREEVIEW_RemoveTree(TREEVIEW_INFO *infoPtr)
{
    TREEVIEW_RemoveAllChildren(infoPtr, infoPtr->root);

    assert(infoPtr->uNumItems == 0);	/* root isn't counted in uNumItems */
}

static LRESULT
TREEVIEW_DeleteItem(TREEVIEW_INFO *infoPtr, HTREEITEM wineItem)
{
    TREEVIEW_ITEM *newSelection = NULL;
    TREEVIEW_ITEM *newFirstVisible = NULL;
    TREEVIEW_ITEM *parent, *prev = NULL;
    BOOL visible = FALSE;

    if (wineItem == TVI_ROOT)
    {
	TRACE("TVI_ROOT\n");
	parent = infoPtr->root;
	newSelection = NULL;
	visible = TRUE;
	TREEVIEW_RemoveTree(infoPtr);
    }
    else
    {
	if (!TREEVIEW_ValidItem(infoPtr, wineItem))
	    return FALSE;

	TRACE("%p (%s)\n", wineItem, TREEVIEW_ItemName(wineItem));
	parent = wineItem->parent;

        if (ISVISIBLE(wineItem))
        {
            prev = TREEVIEW_GetPrevListItem(infoPtr, wineItem);
            visible = TRUE;
        }

	if (infoPtr->selectedItem != NULL
	    && (wineItem == infoPtr->selectedItem
		|| TREEVIEW_IsChildOf(wineItem, infoPtr->selectedItem)))
	{
	    if (wineItem->nextSibling)
		newSelection = wineItem->nextSibling;
	    else if (wineItem->parent != infoPtr->root)
		newSelection = wineItem->parent;
            else
                newSelection = wineItem->prevSibling;
            TRACE("newSelection = %p\n", newSelection);
	}

	if (infoPtr->firstVisible == wineItem)
	{
	    if (wineItem->nextSibling)
	       newFirstVisible = wineItem->nextSibling;
	    else if (wineItem->prevSibling)
	       newFirstVisible = wineItem->prevSibling;
	    else if (wineItem->parent != infoPtr->root)
	       newFirstVisible = wineItem->parent;
	       TREEVIEW_SetFirstVisible(infoPtr, NULL, TRUE);
	}
	else
	    newFirstVisible = infoPtr->firstVisible;

	TREEVIEW_RemoveItem(infoPtr, wineItem);
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
    {
       TREEVIEW_SetFirstVisible(infoPtr, newFirstVisible, TRUE);
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
TREEVIEW_SetRedraw(TREEVIEW_INFO* infoPtr, WPARAM wParam, LPARAM lParam)
{
  if(wParam)
    infoPtr->bRedraw = TRUE;
  else
    infoPtr->bRedraw = FALSE;

  return 0;
}

static LRESULT
TREEVIEW_GetIndent(TREEVIEW_INFO *infoPtr)
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
TREEVIEW_GetToolTips(TREEVIEW_INFO *infoPtr)
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
TREEVIEW_GetUnicodeFormat(TREEVIEW_INFO *infoPtr)
{
     return infoPtr->bNtfUnicode;
}

static LRESULT
TREEVIEW_GetScrollTime(TREEVIEW_INFO *infoPtr)
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
TREEVIEW_GetImageList(TREEVIEW_INFO *infoPtr, WPARAM wParam)
{
    TRACE("\n");

    switch (wParam)
    {
    case (WPARAM)TVSIL_NORMAL:
	return (LRESULT)infoPtr->himlNormal;

    case (WPARAM)TVSIL_STATE:
	return (LRESULT)infoPtr->himlState;

    default:
	return 0;
    }
}

static LRESULT
TREEVIEW_SetImageList(TREEVIEW_INFO *infoPtr, WPARAM wParam, HIMAGELIST himlNew)
{
    HIMAGELIST himlOld = 0;
    int oldWidth  = infoPtr->normalImageWidth;
    int oldHeight = infoPtr->normalImageHeight;


    TRACE("%x,%p\n", wParam, himlNew);

    switch (wParam)
    {
    case (WPARAM)TVSIL_NORMAL:
	himlOld = infoPtr->himlNormal;
	infoPtr->himlNormal = himlNew;

	if (himlNew != NULL)
	    ImageList_GetIconSize(himlNew, &infoPtr->normalImageWidth,
				  &infoPtr->normalImageHeight);
	else
	{
	    infoPtr->normalImageWidth = 0;
	    infoPtr->normalImageHeight = 0;
	}

	break;

    case (WPARAM)TVSIL_STATE:
	himlOld = infoPtr->himlState;
	infoPtr->himlState = himlNew;

	if (himlNew != NULL)
	    ImageList_GetIconSize(himlNew, &infoPtr->stateImageWidth,
				  &infoPtr->stateImageHeight);
	else
	{
	    infoPtr->stateImageWidth = 0;
	    infoPtr->stateImageHeight = 0;
	}

	break;
    }

    if (oldWidth != infoPtr->normalImageWidth ||
        oldHeight != infoPtr->normalImageHeight)
    {
       TREEVIEW_UpdateSubTree(infoPtr, infoPtr->root);
       TREEVIEW_UpdateScrollBars(infoPtr);
    }

    TREEVIEW_Invalidate(infoPtr, NULL);

    return (LRESULT)himlOld;
}

/* Compute the natural height (based on the font size) for items. */
static UINT
TREEVIEW_NaturalHeight(TREEVIEW_INFO *infoPtr)
{
    TEXTMETRICW tm;
    HDC hdc = GetDC(0);
    HFONT hOldFont = SelectObject(hdc, infoPtr->hFont);

    GetTextMetricsW(hdc, &tm);

    SelectObject(hdc, hOldFont);
    ReleaseDC(0, hdc);

    /* The 16 is a hack because our fonts are tiny. */
    /* add 2 for the focus border and 1 more for margin some apps assume */
    return max(16, tm.tmHeight + tm.tmExternalLeading + 3);
}

static LRESULT
TREEVIEW_SetItemHeight(TREEVIEW_INFO *infoPtr, INT newHeight)
{
    INT prevHeight = infoPtr->uItemHeight;

    TRACE("%d \n", newHeight);
    if (newHeight == -1)
    {
	infoPtr->uItemHeight = TREEVIEW_NaturalHeight(infoPtr);
	infoPtr->bHeightSet = FALSE;
    }
    else
    {
	infoPtr->uItemHeight = newHeight;
	infoPtr->bHeightSet = TRUE;
    }

    /* Round down, unless we support odd ("non even") heights. */
    if (!(infoPtr->dwStyle) & TVS_NONEVENHEIGHT)
	infoPtr->uItemHeight &= ~1;

    if (infoPtr->uItemHeight != prevHeight)
    {
	TREEVIEW_RecalculateVisibleOrder(infoPtr, NULL);
	TREEVIEW_UpdateScrollBars(infoPtr);
	TREEVIEW_Invalidate(infoPtr, NULL);
    }

    return prevHeight;
}

static LRESULT
TREEVIEW_GetItemHeight(TREEVIEW_INFO *infoPtr)
{
    TRACE("\n");
    return infoPtr->uItemHeight;
}


static LRESULT
TREEVIEW_GetFont(TREEVIEW_INFO *infoPtr)
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

    infoPtr->hFont = hFont ? hFont : GetStockObject(SYSTEM_FONT);

    DeleteObject(infoPtr->hBoldFont);
    infoPtr->hBoldFont = TREEVIEW_CreateBoldFont(infoPtr->hFont);

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
TREEVIEW_GetLineColor(TREEVIEW_INFO *infoPtr)
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
TREEVIEW_GetTextColor(TREEVIEW_INFO *infoPtr)
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
TREEVIEW_GetBkColor(TREEVIEW_INFO *infoPtr)
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
TREEVIEW_GetInsertMarkColor(TREEVIEW_INFO *infoPtr)
{
    TRACE("\n");
    return (LRESULT)infoPtr->clrInsertMark;
}

static LRESULT
TREEVIEW_SetInsertMarkColor(TREEVIEW_INFO *infoPtr, COLORREF color)
{
    COLORREF prevColor = infoPtr->clrInsertMark;

    TRACE("%lx\n", color);
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
TREEVIEW_GetItemRect(TREEVIEW_INFO *infoPtr, BOOL fTextRect, LPRECT lpRect)
{
    TREEVIEW_ITEM *wineItem;
    const HTREEITEM *pItem = (HTREEITEM *)lpRect;

    TRACE("\n");
    /*
     * validate parameters
     */
    if (pItem == NULL)
	return FALSE;

    wineItem = *pItem;
    if (!TREEVIEW_ValidItem(infoPtr, wineItem) || !ISVISIBLE(wineItem))
	return FALSE;

    /*
     * If wParam is TRUE return the text size otherwise return
     * the whole item size
     */
    if (fTextRect)
    {
	/* Windows does not send TVN_GETDISPINFO here. */

	lpRect->top = wineItem->rect.top;
	lpRect->bottom = wineItem->rect.bottom;

	lpRect->left = wineItem->textOffset;
	lpRect->right = wineItem->textOffset + wineItem->textWidth;
    }
    else
    {
	*lpRect = wineItem->rect;
    }

    TRACE("%s [L:%ld R:%ld T:%ld B:%ld]\n", fTextRect ? "text" : "item",
	  lpRect->left, lpRect->right, lpRect->top, lpRect->bottom);

    return TRUE;
}

static inline LRESULT
TREEVIEW_GetVisibleCount(TREEVIEW_INFO *infoPtr)
{
    /* Suprise! This does not take integral height into account. */
    return infoPtr->clientHeight / infoPtr->uItemHeight;
}


static LRESULT
TREEVIEW_GetItemT(TREEVIEW_INFO *infoPtr, LPTVITEMEXW tvItem, BOOL isW)
{
    TREEVIEW_ITEM *wineItem;

    wineItem = tvItem->hItem;
    if (!TREEVIEW_ValidItem(infoPtr, wineItem))
	return FALSE;

    TREEVIEW_UpdateDispInfo(infoPtr, wineItem, tvItem->mask);

    if (tvItem->mask & TVIF_CHILDREN)
    {
        if (TVIF_CHILDREN==I_CHILDRENCALLBACK)
            FIXME("I_CHILDRENCALLBACK not supported\n");
	tvItem->cChildren = wineItem->cChildren;
    }

    if (tvItem->mask & TVIF_HANDLE)
	tvItem->hItem = wineItem;

    if (tvItem->mask & TVIF_IMAGE)
	tvItem->iImage = wineItem->iImage;

    if (tvItem->mask & TVIF_INTEGRAL)
	tvItem->iIntegral = wineItem->iIntegral;

    /* undocumented: windows ignores TVIF_PARAM and
     * * always sets lParam
     */
    tvItem->lParam = wineItem->lParam;

    if (tvItem->mask & TVIF_SELECTEDIMAGE)
	tvItem->iSelectedImage = wineItem->iSelectedImage;

    if (tvItem->mask & TVIF_STATE)
        /* Careful here - Windows ignores the stateMask when you get the state
 	    That contradicts the documentation, but makes more common sense, masking
	    retrieval in this way seems overkill */
        tvItem->state = wineItem->state;

    if (tvItem->mask & TVIF_TEXT)
    {
        if (isW)
        {
            if (wineItem->pszText == LPSTR_TEXTCALLBACKW)
            {
                tvItem->pszText = LPSTR_TEXTCALLBACKW;
                FIXME(" GetItem called with LPSTR_TEXTCALLBACK\n");
            }
            else
            {
                lstrcpynW(tvItem->pszText, wineItem->pszText, tvItem->cchTextMax);
            }
        }
        else
        {
            if (wineItem->pszText == LPSTR_TEXTCALLBACKW)
            {
                tvItem->pszText = (LPWSTR)LPSTR_TEXTCALLBACKA;
                FIXME(" GetItem called with LPSTR_TEXTCALLBACK\n");
            }
            else
            {
                WideCharToMultiByte(CP_ACP, 0, wineItem->pszText, -1,
                                    (LPSTR)tvItem->pszText, tvItem->cchTextMax, NULL, NULL);
            }
        }
    }
    TRACE("item <%p>, txt %p, img %p, mask %x\n",
	  wineItem, tvItem->pszText, &tvItem->iImage, tvItem->mask);

    return TRUE;
}

/* Beware MSDN Library Visual Studio 6.0. It says -1 on failure, 0 on success,
 * which is wrong. */
static LRESULT
TREEVIEW_SetItemT(TREEVIEW_INFO *infoPtr, LPTVITEMEXW tvItem, BOOL isW)
{
    TREEVIEW_ITEM *wineItem;
    TREEVIEW_ITEM originalItem;

    wineItem = tvItem->hItem;

    TRACE("item %d,mask %x\n", TREEVIEW_GetItemIndex(infoPtr, wineItem),
	  tvItem->mask);

    if (!TREEVIEW_ValidItem(infoPtr, wineItem))
	return FALSE;
    
    /* store the orignal item values */
    originalItem = *wineItem;

    if (!TREEVIEW_DoSetItemT(infoPtr, wineItem, tvItem, isW))
	return FALSE;

    /* If the text or TVIS_BOLD was changed, and it is visible, recalculate. */
    if ((tvItem->mask & TVIF_TEXT
	 || (tvItem->mask & TVIF_STATE && tvItem->stateMask & TVIS_BOLD))
	&& ISVISIBLE(wineItem))
    {
	TREEVIEW_UpdateDispInfo(infoPtr, wineItem, TVIF_TEXT);
	TREEVIEW_ComputeTextWidth(infoPtr, wineItem, 0);
    }

    if (tvItem->mask != 0 && ISVISIBLE(wineItem))
    {
	/* The refresh updates everything, but we can't wait until then. */
	TREEVIEW_ComputeItemInternalMetrics(infoPtr, wineItem);

        /* if any of the items values changed, redraw the item */
        if(memcmp(&originalItem, wineItem, sizeof(TREEVIEW_ITEM)) ||
           (tvItem->stateMask & TVIS_BOLD))
        {
            if (tvItem->mask & TVIF_INTEGRAL)
	    {
	        TREEVIEW_RecalculateVisibleOrder(infoPtr, wineItem);
	        TREEVIEW_UpdateScrollBars(infoPtr);

	        TREEVIEW_Invalidate(infoPtr, NULL);
	    }
	    else
	    {
	        TREEVIEW_UpdateScrollBars(infoPtr);
	        TREEVIEW_Invalidate(infoPtr, wineItem);
	    }
        }
    }

    return TRUE;
}

static LRESULT
TREEVIEW_GetItemState(TREEVIEW_INFO *infoPtr, HTREEITEM wineItem, UINT mask)
{
    TRACE("\n");

    if (!wineItem || !TREEVIEW_ValidItem(infoPtr, wineItem))
	return 0;

    return (wineItem->state & mask);
}

static LRESULT
TREEVIEW_GetNextItem(TREEVIEW_INFO *infoPtr, UINT which, HTREEITEM wineItem)
{
    TREEVIEW_ITEM *retval;

    retval = 0;

    /* handle all the global data here */
    switch (which)
    {
    case TVGN_CHILD:		/* Special case: child of 0 is root */
	if (wineItem)
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
	TRACE("flags:%x, returns %p\n", which, retval);
	return (LRESULT)retval;
    }

    if (wineItem == TVI_ROOT) wineItem = infoPtr->root;

    if (!TREEVIEW_ValidItem(infoPtr, wineItem))
	return FALSE;

    switch (which)
    {
    case TVGN_NEXT:
	retval = wineItem->nextSibling;
	break;
    case TVGN_PREVIOUS:
	retval = wineItem->prevSibling;
	break;
    case TVGN_PARENT:
	retval = (wineItem->parent != infoPtr->root) ? wineItem->parent : NULL;
	break;
    case TVGN_CHILD:
	retval = wineItem->firstChild;
	break;
    case TVGN_NEXTVISIBLE:
	retval = TREEVIEW_GetNextListItem(infoPtr, wineItem);
	break;
    case TVGN_PREVIOUSVISIBLE:
	retval = TREEVIEW_GetPrevListItem(infoPtr, wineItem);
	break;
    default:
	TRACE("Unknown msg %x,item %p\n", which, wineItem);
	break;
    }

    TRACE("flags:%x, item %p;returns %p\n", which, wineItem, retval);
    return (LRESULT)retval;
}


static LRESULT
TREEVIEW_GetCount(TREEVIEW_INFO *infoPtr)
{
    TRACE(" %d\n", infoPtr->uNumItems);
    return (LRESULT)infoPtr->uNumItems;
}

static VOID
TREEVIEW_ToggleItemState(TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *item)
{
    if (infoPtr->dwStyle & TVS_CHECKBOXES)
    {
	static const unsigned int state_table[] = { 0, 2, 1 };

	unsigned int state;

	state = STATEIMAGEINDEX(item->state);
	TRACE("state:%x\n", state);
	item->state &= ~TVIS_STATEIMAGEMASK;

	if (state < 3)
	    state = state_table[state];

	item->state |= INDEXTOSTATEIMAGEMASK(state);

	TRACE("state:%x\n", state);
	TREEVIEW_Invalidate(infoPtr, item);
    }
}


/* Painting *************************************************************/

/* Draw the lines and expand button for an item. Also draws one section
 * of the line from item's parent to item's parent's next sibling. */
static void
TREEVIEW_DrawItemLines(TREEVIEW_INFO *infoPtr, HDC hdc, TREEVIEW_ITEM *item)
{
    LONG centerx, centery;
    BOOL lar = ((infoPtr->dwStyle
		 & (TVS_LINESATROOT|TVS_HASLINES|TVS_HASBUTTONS))
		> TVS_LINESATROOT);

    if (!lar && item->iLevel == 0)
	return;

    centerx = (item->linesOffset + item->stateOffset) / 2;
    centery = (item->rect.top + item->rect.bottom) / 2;

    if (infoPtr->dwStyle & TVS_HASLINES)
    {
	HPEN hOldPen, hNewPen;
	HTREEITEM parent;

	/*
	 * Get a dotted grey pen
	 */
	hNewPen = CreatePen(PS_ALTERNATE, 0, infoPtr->clrLine);
	hOldPen = SelectObject(hdc, hNewPen);

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
	    LONG height = item->rect.bottom - item->rect.top;
	    LONG width  = item->stateOffset - item->linesOffset;
	    LONG rectsize = min(height, width) / 4;
	    /* plussize = ceil(rectsize * 3/4) */
	    LONG plussize = (rectsize + 1) * 3 / 4;

	    HPEN hNewPen  = CreatePen(PS_SOLID, 0, infoPtr->clrLine);
	    HPEN hOldPen  = SelectObject(hdc, hNewPen);
	    HBRUSH hbr    = CreateSolidBrush(infoPtr->clrBk);
	    HBRUSH hbrOld = SelectObject(hdc, hbr);

           Rectangle(hdc, centerx - rectsize - 1, centery - rectsize - 1,
                     centerx + rectsize + 2, centery + rectsize + 2);

	    SelectObject(hdc, hbrOld);
	    DeleteObject(hbr);

	    SelectObject(hdc, hOldPen);
	    DeleteObject(hNewPen);

	    MoveToEx(hdc, centerx - plussize + 1, centery, NULL);
	    LineTo(hdc, centerx + plussize, centery);

	    if (!(item->state & TVIS_EXPANDED))
	    {
		MoveToEx(hdc, centerx, centery - plussize + 1, NULL);
		LineTo(hdc, centerx, centery + plussize);
	    }
	}
    }
}

static void
TREEVIEW_DrawItem(TREEVIEW_INFO *infoPtr, HDC hdc, TREEVIEW_ITEM *wineItem)
{
    INT cditem;
    HFONT hOldFont;
    int centery;

    hOldFont = SelectObject(hdc, TREEVIEW_FontForItem(infoPtr, wineItem));

    TREEVIEW_UpdateDispInfo(infoPtr, wineItem, CALLBACK_MASK_ALL);

    /* The custom draw handler can query the text rectangle,
     * so get ready. */
    TREEVIEW_ComputeTextWidth(infoPtr, wineItem, hdc);

    cditem = 0;

    if (infoPtr->cdmode & CDRF_NOTIFYITEMDRAW)
    {
	cditem = TREEVIEW_SendCustomDrawItemNotify
	    (infoPtr, hdc, wineItem, CDDS_ITEMPREPAINT);
	TRACE("prepaint:cditem-app returns 0x%x\n", cditem);

	if (cditem & CDRF_SKIPDEFAULT)
	{
	    SelectObject(hdc, hOldFont);
	    return;
	}
    }

    if (cditem & CDRF_NEWFONT)
	TREEVIEW_ComputeTextWidth(infoPtr, wineItem, hdc);

    TREEVIEW_DrawItemLines(infoPtr, hdc, wineItem);

    centery = (wineItem->rect.top + wineItem->rect.bottom) / 2;

    /*
     * Display the images associated with this item
     */
    {
	INT imageIndex;

	/* State images are displayed to the left of the Normal image
	 * image number is in state; zero should be `display no image'.
	 */
	imageIndex = STATEIMAGEINDEX(wineItem->state);

	if (infoPtr->himlState && imageIndex)
	{
	    ImageList_Draw(infoPtr->himlState, imageIndex, hdc,
			   wineItem->stateOffset,
			   centery - infoPtr->stateImageHeight / 2,
			   ILD_NORMAL);
	}

	/* Now, draw the normal image; can be either selected or
	 * non-selected image.
	 */

	if ((wineItem->state & TVIS_SELECTED) && (wineItem->iSelectedImage))
	{
	    /* The item is currently selected */
	    imageIndex = wineItem->iSelectedImage;
	}
	else
	{
	    /* The item is not selected */
	    imageIndex = wineItem->iImage;
	}

	if (infoPtr->himlNormal)
	{
	    int ovlIdx = wineItem->state & TVIS_OVERLAYMASK;

	    ImageList_Draw(infoPtr->himlNormal, imageIndex, hdc,
			   wineItem->imageOffset,
			   centery - infoPtr->normalImageHeight / 2,
			   ILD_NORMAL | ovlIdx);
	}
    }


    /*
     * Display the text associated with this item
     */

    /* Don't paint item's text if it's being edited */
    if (!infoPtr->hwndEdit || (infoPtr->selectedItem != wineItem))
    {
	if (wineItem->pszText)
	{
	    COLORREF oldTextColor = 0;
	    INT oldBkMode;
	    HBRUSH hbrBk = 0;
	    BOOL inFocus = (GetFocus() == infoPtr->hwnd);
	    RECT rcText;

	    oldBkMode = SetBkMode(hdc, TRANSPARENT);

	    /* - If item is drop target or it is selected and window is in focus -
	     * use blue background (COLOR_HIGHLIGHT).
	     * - If item is selected, window is not in focus, but it has style
	     * TVS_SHOWSELALWAYS - use grey background (COLOR_BTNFACE)
	     * - Otherwise - don't fill background
	     */
	    if ((wineItem->state & TVIS_DROPHILITED) || ((wineItem == infoPtr->focusedItem) && !(wineItem->state & TVIS_SELECTED)) ||
		((wineItem->state & TVIS_SELECTED) && (!infoPtr->focusedItem) &&
		 (inFocus || (infoPtr->dwStyle & TVS_SHOWSELALWAYS))))
	    {
		if ((wineItem->state & TVIS_DROPHILITED) || inFocus)
		{
		    hbrBk = CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT));
		    oldTextColor =
			SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
		}
		else
		{
		    hbrBk = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));

		    if (infoPtr->clrText == -1)
			oldTextColor =
			    SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
		    else
			oldTextColor = SetTextColor(hdc, infoPtr->clrText);
		}
	    }
	    else
	    {
		if (infoPtr->clrText == -1)
		    oldTextColor =
			SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
		else
		    oldTextColor = SetTextColor(hdc, infoPtr->clrText);
	    }

	    rcText.top = wineItem->rect.top;
	    rcText.bottom = wineItem->rect.bottom;
	    rcText.left = wineItem->textOffset;
	    rcText.right = rcText.left + wineItem->textWidth + 4;

	    if (hbrBk)
	    {
		FillRect(hdc, &rcText, hbrBk);
		DeleteObject(hbrBk);
	    }

	    /* Draw the box around the selected item */
	    if ((wineItem == infoPtr->selectedItem) && inFocus)
	    {
		DrawFocusRect(hdc,&rcText);
	    }

	    InflateRect(&rcText, -2, -1); /* allow for the focus rect */

	    TRACE("drawing text %s at (%ld,%ld)-(%ld,%ld)\n",
		  debugstr_w(wineItem->pszText),
		  rcText.left, rcText.top, rcText.right, rcText.bottom);

	    /* Draw it */
	    DrawTextW(hdc,
		      wineItem->pszText,
		      lstrlenW(wineItem->pszText),
		      &rcText,
		      DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

	    /* Restore the hdc state */
	    SetTextColor(hdc, oldTextColor);

	    if (oldBkMode != TRANSPARENT)
		SetBkMode(hdc, oldBkMode);
	}
    }

    /* Draw insertion mark if necessary */

    if (infoPtr->insertMarkItem)
	TRACE("item:%d,mark:%d\n",
	      TREEVIEW_GetItemIndex(infoPtr, wineItem),
	      (int)infoPtr->insertMarkItem);

    if (wineItem == infoPtr->insertMarkItem)
    {
	HPEN hNewPen, hOldPen;
	int offset;
	int left, right;

	hNewPen = CreatePen(PS_SOLID, 2, infoPtr->clrInsertMark);
	hOldPen = SelectObject(hdc, hNewPen);

	if (infoPtr->insertBeforeorAfter)
	    offset = wineItem->rect.bottom - 1;
	else
	    offset = wineItem->rect.top + 1;

	left = wineItem->textOffset - 2;
	right = wineItem->textOffset + wineItem->textWidth + 2;

	MoveToEx(hdc, left, offset - 3, NULL);
	LineTo(hdc, left, offset + 4);

	MoveToEx(hdc, left, offset, NULL);
	LineTo(hdc, right + 1, offset);

	MoveToEx(hdc, right, offset + 3, NULL);
	LineTo(hdc, right, offset - 4);

	SelectObject(hdc, hOldPen);
	DeleteObject(hNewPen);
    }

    if (cditem & CDRF_NOTIFYPOSTPAINT)
    {
	cditem = TREEVIEW_SendCustomDrawItemNotify
	    (infoPtr, hdc, wineItem, CDDS_ITEMPOSTPAINT);
	TRACE("postpaint:cditem-app returns 0x%x\n", cditem);
    }

    SelectObject(hdc, hOldFont);
}

/* Computes treeHeight and treeWidth and updates the scroll bars.
 */
static void
TREEVIEW_UpdateScrollBars(TREEVIEW_INFO *infoPtr)
{
    TREEVIEW_ITEM *wineItem;
    HWND hwnd = infoPtr->hwnd;
    BOOL vert = FALSE;
    BOOL horz = FALSE;
    SCROLLINFO si;
    LONG scrollX = infoPtr->scrollX;

    infoPtr->treeWidth = 0;
    infoPtr->treeHeight = 0;

    /* We iterate through all visible items in order to get the tree height
     * and width */
    wineItem = infoPtr->root->firstChild;

    while (wineItem != NULL)
    {
	if (ISVISIBLE(wineItem))
	{
            /* actually we draw text at textOffset + 2 */
	    if (2+wineItem->textOffset+wineItem->textWidth > infoPtr->treeWidth)
		infoPtr->treeWidth = wineItem->textOffset+wineItem->textWidth+2;

	    /* This is scroll-adjusted, but we fix this below. */
	    infoPtr->treeHeight = wineItem->rect.bottom;
	}

	wineItem = TREEVIEW_GetNextListItem(infoPtr, wineItem);
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
    else if (infoPtr->treeWidth > infoPtr->clientWidth)
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
    }
    else
    {
	if (infoPtr->uInternalStatus & TV_HSCROLL)
	    ShowScrollBar(hwnd, SB_HORZ, FALSE);
	infoPtr->uInternalStatus &= ~TV_HSCROLL;

	scrollX = 0;
    }

    if (infoPtr->scrollX != scrollX)
    {
	TREEVIEW_HScroll(infoPtr,
	                 MAKEWPARAM(SB_THUMBPOSITION, scrollX));
    }

    if (!horz)
	infoPtr->uInternalStatus &= ~TV_HSCROLL;
}

/* CtrlSpy doesn't mention this, but CorelDRAW's object manager needs it. */
static LRESULT
TREEVIEW_EraseBackground(TREEVIEW_INFO *infoPtr, HDC hDC)
{
    HBRUSH hBrush = CreateSolidBrush(infoPtr->clrBk);
    RECT rect;

    GetClientRect(infoPtr->hwnd, &rect);
    FillRect(hDC, &rect, hBrush);
    DeleteObject(hBrush);

    return 1;
}

static void
TREEVIEW_Refresh(TREEVIEW_INFO *infoPtr, HDC hdc, RECT *rc)
{
    HWND hwnd = infoPtr->hwnd;
    RECT rect = *rc;
    TREEVIEW_ITEM *wineItem;

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

    for (wineItem = infoPtr->root->firstChild;
         wineItem != NULL;
         wineItem = TREEVIEW_GetNextListItem(infoPtr, wineItem))
    {
	if (ISVISIBLE(wineItem))
	{
            /* Avoid unneeded calculations */
            if (wineItem->rect.top > rect.bottom)
                break;
            if (wineItem->rect.bottom < rect.top)
                continue;

	    TREEVIEW_DrawItem(infoPtr, hdc, wineItem);
	}
    }

    TREEVIEW_UpdateScrollBars(infoPtr);

    if (infoPtr->cdmode & CDRF_NOTIFYPOSTPAINT)
	infoPtr->cdmode =
	    TREEVIEW_SendCustomDrawNotify(infoPtr, CDDS_POSTPAINT, hdc, rect);
}

static void
TREEVIEW_Invalidate(TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *item)
{
    if (item != NULL)
	InvalidateRect(infoPtr->hwnd, &item->rect, TRUE);
    else
        InvalidateRect(infoPtr->hwnd, NULL, TRUE);
}

static LRESULT
TREEVIEW_Paint(TREEVIEW_INFO *infoPtr, WPARAM wParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    RECT rc;

    TRACE("\n");

    if (wParam)
    {
        hdc = (HDC)wParam;
        if (!GetUpdateRect(infoPtr->hwnd, &rc, TRUE))
        {
            HBITMAP hbitmap;
            BITMAP bitmap;
            hbitmap = GetCurrentObject(hdc, OBJ_BITMAP);
            if (!hbitmap) return 0;
            GetObjectA(hbitmap, sizeof(BITMAP), &bitmap);
            rc.left = 0; rc.top = 0;
            rc.right = bitmap.bmWidth;
            rc.bottom = bitmap.bmHeight;
            TREEVIEW_EraseBackground(infoPtr, (HDC)wParam);
        }
    }
    else
    {
        hdc = BeginPaint(infoPtr->hwnd, &ps);
        rc = ps.rcPaint;
    }

    if(infoPtr->bRedraw) /* WM_SETREDRAW sets bRedraw */
        TREEVIEW_Refresh(infoPtr, hdc, &rc);

    if (!wParam)
	EndPaint(infoPtr->hwnd, &ps);

    return 0;
}


/* Sorting **************************************************************/

/***************************************************************************
 * Forward the DPA local callback to the treeview owner callback
 */
static INT WINAPI
TREEVIEW_CallBackCompare(TREEVIEW_ITEM *first, TREEVIEW_ITEM *second, LPTVSORTCB pCallBackSort)
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
                     TREEVIEW_INFO *infoPtr)
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
TREEVIEW_CountChildren(TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *item)
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
TREEVIEW_BuildChildDPA(TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *item)
{
    HTREEITEM child = item->firstChild;

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
TREEVIEW_Sort(TREEVIEW_INFO *infoPtr, BOOL fRecurse, HTREEITEM parent,
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
	ERR("invalid item hParent=%x\n", (INT)parent);
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

    cChildren = TREEVIEW_CountChildren(infoPtr, parent);

    /* Make sure there is something to sort */
    if (cChildren > 1)
    {
	/* TREEVIEW_ITEM rechaining */
	INT count = 0;
	HTREEITEM item = 0;
	HTREEITEM nextItem = 0;
	HTREEITEM prevItem = 0;

	HDPA sortList = TREEVIEW_BuildChildDPA(infoPtr, parent);

	if (sortList == NULL)
	    return FALSE;

	/* let DPA sort the list */
	DPA_Sort(sortList, pfnCompare, lpCompare);

	/* The order of DPA entries has been changed, so fixup the
	 * nextSibling and prevSibling pointers. */

	item = (HTREEITEM)DPA_GetPtr(sortList, count++);
	while ((nextItem = (HTREEITEM)DPA_GetPtr(sortList, count++)) != NULL)
	{
	    /* link the two current item toghether */
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
TREEVIEW_SortChildrenCB(TREEVIEW_INFO *infoPtr, WPARAM wParam, LPTVSORTCB pSort)
{
    return TREEVIEW_Sort(infoPtr, wParam, pSort->hParent, pSort);
}


/***************************************************************************
 * Sort the children of the TV item specified in lParam.
 */
static LRESULT
TREEVIEW_SortChildren(TREEVIEW_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    return TREEVIEW_Sort(infoPtr, (BOOL)wParam, (HTREEITEM)lParam, NULL);
}


/* Expansion/Collapse ***************************************************/

static BOOL
TREEVIEW_SendExpanding(TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *wineItem,
		       UINT action)
{
    return !TREEVIEW_SendTreeviewNotify(infoPtr, TVN_ITEMEXPANDINGW, action,
					TVIF_HANDLE | TVIF_STATE | TVIF_PARAM
					| TVIF_IMAGE | TVIF_SELECTEDIMAGE,
					0, wineItem);
}

static VOID
TREEVIEW_SendExpanded(TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *wineItem,
		      UINT action)
{
    TREEVIEW_SendTreeviewNotify(infoPtr, TVN_ITEMEXPANDEDW, action,
				TVIF_HANDLE | TVIF_STATE | TVIF_PARAM
				| TVIF_IMAGE | TVIF_SELECTEDIMAGE,
				0, wineItem);
}


/* This corresponds to TVM_EXPAND with TVE_COLLAPSE.
 * bRemoveChildren corresponds to TVE_COLLAPSERESET. */
static BOOL
TREEVIEW_Collapse(TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *wineItem,
		  BOOL bRemoveChildren, BOOL bUser)
{
    UINT action = TVE_COLLAPSE | (bRemoveChildren ? TVE_COLLAPSERESET : 0);
    BOOL bSetSelection, bSetFirstVisible;

    TRACE("TVE_COLLAPSE %p %s\n", wineItem, TREEVIEW_ItemName(wineItem));

    if (!(wineItem->state & TVIS_EXPANDED))
	return FALSE;

    if (bUser || !(wineItem->state & TVIS_EXPANDEDONCE))
	TREEVIEW_SendExpanding(infoPtr, wineItem, action);

    if (wineItem->firstChild == NULL)
	return FALSE;

    wineItem->state &= ~TVIS_EXPANDED;

    if (bUser || !(wineItem->state & TVIS_EXPANDEDONCE))
	TREEVIEW_SendExpanded(infoPtr, wineItem, action);

    bSetSelection = (infoPtr->selectedItem != NULL
		     && TREEVIEW_IsChildOf(wineItem, infoPtr->selectedItem));

    bSetFirstVisible = (infoPtr->firstVisible != NULL
                        && TREEVIEW_IsChildOf(wineItem, infoPtr->firstVisible));

    if (bRemoveChildren)
    {
        INT old_cChildren = wineItem->cChildren;
	TRACE("TVE_COLLAPSERESET\n");
	wineItem->state &= ~TVIS_EXPANDEDONCE;
	TREEVIEW_RemoveAllChildren(infoPtr, wineItem);
        wineItem->cChildren = old_cChildren;
    }

    if (wineItem->firstChild)
    {
        TREEVIEW_ITEM *item, *sibling;

	sibling = TREEVIEW_GetNextListItem(infoPtr, wineItem);

	for (item = wineItem->firstChild; item != sibling;
	     item = TREEVIEW_GetNextListItem(infoPtr, item))
	{
	    item->visibleOrder = -1;
	}
    }

    TREEVIEW_RecalculateVisibleOrder(infoPtr, wineItem);

    TREEVIEW_SetFirstVisible(infoPtr, bSetFirstVisible ? wineItem
			     : infoPtr->firstVisible, TRUE);

    if (bSetSelection)
    {
	/* Don't call DoSelectItem, it sends notifications. */
	if (TREEVIEW_ValidItem(infoPtr, infoPtr->selectedItem))
	    infoPtr->selectedItem->state &= ~TVIS_SELECTED;
	wineItem->state |= TVIS_SELECTED;
	infoPtr->selectedItem = wineItem;

	TREEVIEW_EnsureVisible(infoPtr, wineItem, FALSE);
    }

    TREEVIEW_UpdateScrollBars(infoPtr);
    TREEVIEW_Invalidate(infoPtr, NULL);

    return TRUE;
}

static BOOL
TREEVIEW_Expand(TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *wineItem,
		BOOL bExpandPartial, BOOL bUser)
{
    TRACE("\n");

    if (wineItem->state & TVIS_EXPANDED)
       return TRUE;

    TRACE("TVE_EXPAND %p %s\n", wineItem, TREEVIEW_ItemName(wineItem));

    if (bUser || ((wineItem->cChildren != 0) &&
                  !(wineItem->state & TVIS_EXPANDEDONCE)))
    {
	if (!TREEVIEW_SendExpanding(infoPtr, wineItem, TVE_EXPAND))
	{
	    TRACE("  TVN_ITEMEXPANDING returned TRUE, exiting...\n");
	    return FALSE;
	}

        if (!wineItem->firstChild)
            return FALSE;

	wineItem->state |= TVIS_EXPANDED;
	TREEVIEW_SendExpanded(infoPtr, wineItem, TVE_EXPAND);
	wineItem->state |= TVIS_EXPANDEDONCE;
    }
    else
    {
        if (!wineItem->firstChild)
            return FALSE;

	/* this item has already been expanded */
	wineItem->state |= TVIS_EXPANDED;
    }

    if (bExpandPartial)
	FIXME("TVE_EXPANDPARTIAL not implemented\n");

    TREEVIEW_RecalculateVisibleOrder(infoPtr, wineItem);
    TREEVIEW_UpdateSubTree(infoPtr, wineItem);
    TREEVIEW_UpdateScrollBars(infoPtr);

    /* Scroll up so that as many children as possible are visible.
     * This looses when expanding causes an HScroll bar to appear, but we
     * don't know that yet, so the last item is obscured. */
    if (wineItem->firstChild != NULL)
    {
	int nChildren = wineItem->lastChild->visibleOrder
	    - wineItem->firstChild->visibleOrder + 1;

	int visible_pos = wineItem->visibleOrder
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

    TREEVIEW_Invalidate(infoPtr, NULL);

    return TRUE;
}

static BOOL
TREEVIEW_Toggle(TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *wineItem, BOOL bUser)
{
    TRACE("\n");

    if (wineItem->state & TVIS_EXPANDED)
	return TREEVIEW_Collapse(infoPtr, wineItem, FALSE, bUser);
    else
	return TREEVIEW_Expand(infoPtr, wineItem, FALSE, bUser);
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
TREEVIEW_ExpandMsg(TREEVIEW_INFO *infoPtr, UINT flag, HTREEITEM wineItem)
{
    if (!TREEVIEW_ValidItem(infoPtr, wineItem))
	return 0;

    TRACE("For (%s) item:%d, flags %x, state:%d\n",
	      TREEVIEW_ItemName(wineItem), flag,
	      TREEVIEW_GetItemIndex(infoPtr, wineItem), wineItem->state);

    switch (flag & TVE_TOGGLE)
    {
    case TVE_COLLAPSE:
	return TREEVIEW_Collapse(infoPtr, wineItem, flag & TVE_COLLAPSERESET,
				 FALSE);

    case TVE_EXPAND:
	return TREEVIEW_Expand(infoPtr, wineItem, flag & TVE_EXPANDPARTIAL,
			       FALSE);

    case TVE_TOGGLE:
	return TREEVIEW_Toggle(infoPtr, wineItem, TRUE);

    default:
	return 0;
    }

#if 0
    TRACE("Exiting, Item %p state is now %d...\n", wineItem, wineItem->state);
#endif
}

/* Hit-Testing **********************************************************/

static TREEVIEW_ITEM *
TREEVIEW_HitTestPoint(TREEVIEW_INFO *infoPtr, POINT pt)
{
    TREEVIEW_ITEM *wineItem;
    LONG row;

    if (!infoPtr->firstVisible)
	return NULL;

    row = pt.y / infoPtr->uItemHeight + infoPtr->firstVisible->visibleOrder;

    for (wineItem = infoPtr->firstVisible; wineItem != NULL;
	 wineItem = TREEVIEW_GetNextListItem(infoPtr, wineItem))
    {
	if (row >= wineItem->visibleOrder
	    && row < wineItem->visibleOrder + wineItem->iIntegral)
	    break;
    }

    return wineItem;
}

static LRESULT
TREEVIEW_HitTest(TREEVIEW_INFO *infoPtr, LPTVHITTESTINFO lpht)
{
    TREEVIEW_ITEM *wineItem;
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
	return (LRESULT)(HTREEITEM)NULL;
    }

    wineItem = TREEVIEW_HitTestPoint(infoPtr, lpht->pt);
    if (!wineItem)
    {
	lpht->flags = TVHT_NOWHERE;
	return (LRESULT)(HTREEITEM)NULL;
    }

    if (x >= wineItem->textOffset + wineItem->textWidth)
    {
	lpht->flags = TVHT_ONITEMRIGHT;
    }
    else if (x >= wineItem->textOffset)
    {
	lpht->flags = TVHT_ONITEMLABEL;
    }
    else if (x >= wineItem->imageOffset)
    {
	lpht->flags = TVHT_ONITEMICON;
    }
    else if (x >= wineItem->stateOffset)
    {
	lpht->flags = TVHT_ONITEMSTATEICON;
    }
    else if (x >= wineItem->linesOffset && infoPtr->dwStyle & TVS_HASBUTTONS)
    {
	lpht->flags = TVHT_ONITEMBUTTON;
    }
    else
    {
	lpht->flags = TVHT_ONITEMINDENT;
    }

    lpht->hItem = wineItem;
    TRACE("(%ld,%ld):result %x\n", lpht->pt.x, lpht->pt.y, lpht->flags);

    return (LRESULT)wineItem;
}

/* Item Label Editing ***************************************************/

static LRESULT
TREEVIEW_GetEditControl(TREEVIEW_INFO *infoPtr)
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

    case WM_GETDLGCODE:
	return DLGC_WANTARROWS | DLGC_WANTALLKEYS;

    case WM_KEYDOWN:
	if (wParam == (WPARAM)VK_ESCAPE)
	{
	    bCancel = TRUE;
	    break;
	}
	else if (wParam == (WPARAM)VK_RETURN)
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
    TRACE("%x %ld\n", wParam, lParam);

    switch (HIWORD(wParam))
    {
    case EN_UPDATE:
	{
	    /*
	     * Adjust the edit window size
	     */
	    WCHAR buffer[1024];
	    TREEVIEW_ITEM *editItem = infoPtr->selectedItem;
	    HDC hdc = GetDC(infoPtr->hwndEdit);
	    SIZE sz;
	    int len;
	    HFONT hFont, hOldFont = 0;

	    infoPtr->bLabelChanged = TRUE;

	    len = GetWindowTextW(infoPtr->hwndEdit, buffer, sizeof(buffer));

	    /* Select font to get the right dimension of the string */
	    hFont = (HFONT)SendMessageW(infoPtr->hwndEdit, WM_GETFONT, 0, 0);

	    if (hFont != 0)
	    {
		hOldFont = SelectObject(hdc, hFont);
	    }

	    if (GetTextExtentPoint32W(hdc, buffer, strlenW(buffer), &sz))
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
    TREEVIEW_ITEM *editItem = hItem;
    HINSTANCE hinst = (HINSTANCE)GetWindowLongW(hwnd, GWL_HINSTANCE);
    HDC hdc;
    HFONT hOldFont=0;
    TEXTMETRICW textMetric;
    static const WCHAR EditW[] = {'E','d','i','t',0};

    TRACE("%x %p\n", (unsigned)hwnd, hItem);
    if (!TREEVIEW_ValidItem(infoPtr, editItem))
	return NULL;

    if (infoPtr->hwndEdit)
	return infoPtr->hwndEdit;

    infoPtr->bLabelChanged = FALSE;

    /* Make sure that edit item is selected */
    TREEVIEW_DoSelectItem(infoPtr, TVGN_CARET, hItem, TVC_UNKNOWN);
    TREEVIEW_EnsureVisible(infoPtr, hItem, TRUE);

    TREEVIEW_UpdateDispInfo(infoPtr, editItem, TVIF_TEXT);

    hdc = GetDC(hwnd);
    /* Select the font to get appropriate metric dimensions */
    if (infoPtr->hFont != 0)
    {
	hOldFont = SelectObject(hdc, infoPtr->hFont);
    }

    /* Get string length in pixels */
    GetTextExtentPoint32W(hdc, editItem->pszText, strlenW(editItem->pszText),
			  &sz);

    /* Add Extra spacing for the next character */
    GetTextMetricsW(hdc, &textMetric);
    sz.cx += (textMetric.tmMaxCharWidth * 2);

    sz.cx = max(sz.cx, textMetric.tmMaxCharWidth * 3);
    sz.cx = min(sz.cx, infoPtr->clientWidth - editItem->textOffset + 2);

    if (infoPtr->hFont != 0)
    {
	SelectObject(hdc, hOldFont);
    }

    ReleaseDC(hwnd, hdc);
    hwndEdit = CreateWindowExW(WS_EX_LEFT,
			       EditW,
			       0,
			       WS_CHILD | WS_BORDER | ES_AUTOHSCROLL |
			       WS_CLIPSIBLINGS | ES_WANTRETURN |
			       ES_LEFT, editItem->textOffset - 2,
			       editItem->rect.top - 1, sz.cx + 3,
			       editItem->rect.bottom -
			       editItem->rect.top + 3, hwnd, 0, hinst, 0);
/* FIXME: (HMENU)IDTVEDIT,pcs->hInstance,0); */

    infoPtr->hwndEdit = hwndEdit;

    /* Get a 2D border. */
    SetWindowLongW(hwndEdit, GWL_EXSTYLE,
		   GetWindowLongW(hwndEdit, GWL_EXSTYLE) & ~WS_EX_CLIENTEDGE);
    SetWindowLongW(hwndEdit, GWL_STYLE,
		   GetWindowLongW(hwndEdit, GWL_STYLE) | WS_BORDER);

    SendMessageW(hwndEdit, WM_SETFONT,
		 (WPARAM)TREEVIEW_FontForItem(infoPtr, editItem), FALSE);

    infoPtr->wpEditOrig = (WNDPROC)SetWindowLongW(hwndEdit, GWL_WNDPROC,
						  (DWORD)
						  TREEVIEW_Edit_SubclassProc);

    if (TREEVIEW_BeginLabelEditNotify(infoPtr, editItem))
    {
	DestroyWindow(hwndEdit);
	infoPtr->hwndEdit = 0;
	return NULL;
    }

    infoPtr->selectedItem = hItem;
    SetWindowTextW(hwndEdit, editItem->pszText);
    SetFocus(hwndEdit);
    SendMessageW(hwndEdit, EM_SETSEL, 0, -1);
    ShowWindow(hwndEdit, SW_SHOW);

    return hwndEdit;
}


static LRESULT
TREEVIEW_EndEditLabelNow(TREEVIEW_INFO *infoPtr, BOOL bCancel)
{
    HWND hwnd = infoPtr->hwnd;
    TREEVIEW_ITEM *editedItem = infoPtr->selectedItem;
    NMTVDISPINFOW tvdi;
    BOOL bCommit;
    WCHAR tmpText[1024] = { '\0' };
    WCHAR *newText = tmpText;
    int iLength = 0;

    if (!infoPtr->hwndEdit)
	return FALSE;

    tvdi.hdr.hwndFrom = hwnd;
    tvdi.hdr.idFrom = GetWindowLongW(hwnd, GWL_ID);
    tvdi.hdr.code = get_notifycode(infoPtr, TVN_ENDLABELEDITW);
    tvdi.item.mask = 0;
    tvdi.item.hItem = editedItem;
    tvdi.item.state = editedItem->state;
    tvdi.item.lParam = editedItem->lParam;

    if (!bCancel)
    {
        if (!infoPtr->bNtfUnicode)
            iLength = GetWindowTextA(infoPtr->hwndEdit, (LPSTR)tmpText, 1023);
        else
            iLength = GetWindowTextW(infoPtr->hwndEdit, tmpText, 1023);

	if (iLength >= 1023)
	{
	    ERR("Insufficient space to retrieve new item label\n");
	}

        tvdi.item.mask = TVIF_TEXT;
	tvdi.item.pszText = tmpText;
	tvdi.item.cchTextMax = iLength + 1;
    }
    else
    {
	tvdi.item.pszText = NULL;
	tvdi.item.cchTextMax = 0;
    }

    bCommit = (BOOL)TREEVIEW_SendRealNotify(infoPtr,
				 (WPARAM)tvdi.hdr.idFrom, (LPARAM)&tvdi);

    if (!bCancel && bCommit)	/* Apply the changes */
    {
        if (!infoPtr->bNtfUnicode)
        {
            DWORD len = MultiByteToWideChar( CP_ACP, 0, (LPSTR)tmpText, -1, NULL, 0 );
            newText = Alloc(len * sizeof(WCHAR));
            MultiByteToWideChar( CP_ACP, 0, (LPSTR)tmpText, -1, newText, len );
            iLength = len - 1;
        }

        if (strcmpW(newText, editedItem->pszText) != 0)
        {
            if (NULL == ReAlloc(editedItem->pszText, iLength + 1))
            {
                ERR("OutOfMemory, cannot allocate space for label\n");
                DestroyWindow(infoPtr->hwndEdit);
                infoPtr->hwndEdit = 0;
                return FALSE;
            }
            else
            {
                editedItem->cchTextMax = iLength + 1;
                strcpyW(editedItem->pszText, newText);
            }
        }
        if(newText != tmpText) Free(newText);
    }

    ShowWindow(infoPtr->hwndEdit, SW_HIDE);
    DestroyWindow(infoPtr->hwndEdit);
    infoPtr->hwndEdit = 0;
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
TREEVIEW_TrackMouse(TREEVIEW_INFO *infoPtr, POINT pt)
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
	if (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE | PM_NOYIELD))
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
		if (msg.message == WM_RBUTTONUP)
		    TREEVIEW_RButtonUp(infoPtr, &pt);
		break;
	    }

	    DispatchMessageA(&msg);
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
    TREEVIEW_ITEM *wineItem;
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

    wineItem = (TREEVIEW_ITEM *)TREEVIEW_HitTest(infoPtr, &hit);
    if (!wineItem)
	return 0;
    TRACE("item %d\n", TREEVIEW_GetItemIndex(infoPtr, wineItem));

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

		while (wineItem->iLevel > level)
		{
		    wineItem = wineItem->parent;
		}

		/* fall through */
	    }

	case TVHT_ONITEMLABEL:
	case TVHT_ONITEMICON:
	case TVHT_ONITEMBUTTON:
	    TREEVIEW_Toggle(infoPtr, wineItem, TRUE);
	    break;

	case TVHT_ONITEMSTATEICON:
	   if (infoPtr->dwStyle & TVS_CHECKBOXES)
	       TREEVIEW_ToggleItemState(infoPtr, wineItem);
	   else
	       TREEVIEW_Toggle(infoPtr, wineItem, TRUE);
	   break;
	}
    }
    return TRUE;
}


static LRESULT
TREEVIEW_LButtonDown(TREEVIEW_INFO *infoPtr, LPARAM lParam)
{
    HWND hwnd = infoPtr->hwnd;
    TVHITTESTINFO ht;
    BOOL bTrack;
    HTREEITEM tempItem;

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
    if(ht.hItem && (ht.flags & TVHT_ONITEM))
    {
        infoPtr->focusedItem = ht.hItem;
        InvalidateRect(hwnd, &(((HTREEITEM)(ht.hItem))->rect), TRUE);

        if(infoPtr->selectedItem)
            InvalidateRect(hwnd, &(infoPtr->selectedItem->rect), TRUE);
    }

    bTrack = (ht.flags & TVHT_ONITEM)
	&& !(infoPtr->dwStyle & TVS_DISABLEDRAGDROP);

    /* Send NM_CLICK right away */
    if (!bTrack)
	if (TREEVIEW_SendSimpleNotify(infoPtr, NM_CLICK))
	    goto setfocus;

    if (ht.flags & TVHT_ONITEMBUTTON)
    {
	TREEVIEW_Toggle(infoPtr, ht.hItem, TRUE);
	goto setfocus;
    }
    else if (bTrack)
    {   /* if TREEVIEW_TrackMouse == 1 dragging occurred and the cursor left the dragged item's rectangle */
	if (TREEVIEW_TrackMouse(infoPtr, ht.pt))
	{
	    TREEVIEW_SendTreeviewDnDNotify(infoPtr, TVN_BEGINDRAGW, ht.hItem, ht.pt);
	    infoPtr->dropItem = ht.hItem;

            /* clean up focusedItem as we dragged and won't select this item */
            if(infoPtr->focusedItem)
            {
                /* refresh the item that was focused */
                tempItem = infoPtr->focusedItem;
                infoPtr->focusedItem = 0;
                InvalidateRect(infoPtr->hwnd, &tempItem->rect, TRUE);

                /* refresh the selected item to return the filled background */
                InvalidateRect(infoPtr->hwnd, &(infoPtr->selectedItem->rect), TRUE);
            }

	    return 0;
        }
    }

    if (bTrack && TREEVIEW_SendSimpleNotify(infoPtr, NM_CLICK))
        goto setfocus;

    /*
     * If the style allows editing and the node is already selected
     * and the click occurred on the item label...
     */
    if ((infoPtr->dwStyle & TVS_EDITLABELS) &&
	        (ht.flags & TVHT_ONITEMLABEL) && (infoPtr->selectedItem == ht.hItem))
    {
	if (infoPtr->Timer & TV_EDIT_TIMER_SET)
	    KillTimer(hwnd, TV_EDIT_TIMER);

	SetTimer(hwnd, TV_EDIT_TIMER, GetDoubleClickTime(), 0);
	infoPtr->Timer |= TV_EDIT_TIMER_SET;
    }
    else if (ht.flags & (TVHT_ONITEMICON|TVHT_ONITEMLABEL)) /* select the item if the hit was inside of the icon or text */
    {
        /*
         * if we are TVS_SINGLEEXPAND then we want this single click to
         * do a bunch of things.
         */
        if((infoPtr->dwStyle & TVS_SINGLEEXPAND) &&
          (infoPtr->hwndEdit == 0))
        {
            TREEVIEW_ITEM *SelItem;

            /*
             * Send the notification
             */
            TREEVIEW_SendTreeviewNotify(infoPtr, TVN_SINGLEEXPAND, TVC_UNKNOWN, TVIF_HANDLE | TVIF_PARAM, ht.hItem, 0);

            /*
             * Close the previous selection all the way to the root
             * as long as the new selection is not a child
             */
            if((infoPtr->selectedItem)
                && (infoPtr->selectedItem != ht.hItem))
            {
                BOOL closeit = TRUE;
                SelItem = ht.hItem;

                /* determine if the hitItem is a child of the currently selected item */
                while(closeit && SelItem && TREEVIEW_ValidItem(infoPtr, SelItem) && (SelItem != infoPtr->root))
                {
                    closeit = (SelItem != infoPtr->selectedItem);
                    SelItem = SelItem->parent;
                }

                if(closeit)
                {
                    if(TREEVIEW_ValidItem(infoPtr, infoPtr->selectedItem))
                        SelItem = infoPtr->selectedItem;

                    while(SelItem && (SelItem != ht.hItem) && TREEVIEW_ValidItem(infoPtr, SelItem) && (SelItem != infoPtr->root))
                    {
                        TREEVIEW_Collapse(infoPtr, SelItem, FALSE, FALSE);
                        SelItem = SelItem->parent;
                    }
                }
            }

            /*
             * Expand the current item
             */
            TREEVIEW_Expand(infoPtr, ht.hItem, TVE_TOGGLE, FALSE);
        }

        /* Select the current item */
        TREEVIEW_DoSelectItem(infoPtr, TVGN_CARET, ht.hItem, TVC_BYMOUSE);
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

    TREEVIEW_HitTest(infoPtr, &ht);

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
	TREEVIEW_SendSimpleNotify(infoPtr, NM_RCLICK);
    }

    return 0;
}

static LRESULT
TREEVIEW_RButtonUp(TREEVIEW_INFO *infoPtr, LPPOINT pPt)
{
    return 0;
}


static LRESULT
TREEVIEW_CreateDragImage(TREEVIEW_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
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
    GetTextExtentPoint32W(hdc, dragItem->pszText, strlenW(dragItem->pszText),
			  &size);
    TRACE("%ld %ld %s %d\n", size.cx, size.cy, debugstr_w(dragItem->pszText),
	  strlenW(dragItem->pszText));
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
    DrawTextW(hdc, dragItem->pszText, strlenW(dragItem->pszText), &rc,
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
    RECT rcFocused;

    assert(newSelect == NULL || TREEVIEW_ValidItem(infoPtr, newSelect));

    TRACE("Entering item %p (%s), flag %x, cause %x, state %d\n",
	  newSelect, TREEVIEW_ItemName(newSelect), action, cause,
	  newSelect ? newSelect->state : 0);

    /* reset and redraw focusedItem if focusedItem was set so we don't */
    /* have to worry about the previously focused item when we set a new one */
    if(infoPtr->focusedItem)
    {
        rcFocused = (infoPtr->focusedItem)->rect;
        infoPtr->focusedItem = 0;
        InvalidateRect(infoPtr->hwnd, &rcFocused, TRUE);
    }

    switch (action)
    {
    case TVGN_CARET:
	prevSelect = infoPtr->selectedItem;

	if (prevSelect == newSelect)
	    return FALSE;

	if (TREEVIEW_SendTreeviewNotify(infoPtr,
					TVN_SELCHANGINGW,
					cause,
					TVIF_HANDLE | TVIF_STATE | TVIF_PARAM,
					prevSelect,
					newSelect))
	    return FALSE;

	if (prevSelect)
	    prevSelect->state &= ~TVIS_SELECTED;
	if (newSelect)
	    newSelect->state |= TVIS_SELECTED;

	infoPtr->selectedItem = newSelect;

	TREEVIEW_EnsureVisible(infoPtr, infoPtr->selectedItem, FALSE);

	TREEVIEW_SendTreeviewNotify(infoPtr,
				    TVN_SELCHANGEDW,
				    cause,
				    TVIF_HANDLE | TVIF_STATE | TVIF_PARAM,
				    prevSelect,
				    newSelect);
	TREEVIEW_Invalidate(infoPtr, prevSelect);
	TREEVIEW_Invalidate(infoPtr, newSelect);
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
	TREEVIEW_EnsureVisible(infoPtr, newSelect, FALSE);
	TREEVIEW_SetFirstVisible(infoPtr, newSelect, TRUE);
	TREEVIEW_Invalidate(infoPtr, NULL);
	break;
    }

    TRACE("Leaving state %d\n", newSelect ? newSelect->state : 0);
    return TRUE;
}

/* FIXME: handle NM_KILLFOCUS etc */
static LRESULT
TREEVIEW_SelectItem(TREEVIEW_INFO *infoPtr, INT wParam, HTREEITEM item)
{
    if (item != NULL && !TREEVIEW_ValidItem(infoPtr, item))
	return FALSE;

    TRACE("%p (%s) %d\n", item, TREEVIEW_ItemName(item), wParam);

    if (!TREEVIEW_DoSelectItem(infoPtr, wParam, item, TVC_UNKNOWN))
	return FALSE;

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
 *    accept and it ignores averything else. In particular it will
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
static INT TREEVIEW_ProcessLetterKeys(
    HWND hwnd, /* handle to the window */
    WPARAM charCode, /* the character code, the actual character */
    LPARAM keyData /* key data */
    )
{
    TREEVIEW_INFO *infoPtr;
    HTREEITEM nItem;
    HTREEITEM endidx,idx;
    TVITEMEXW item;
    WCHAR buffer[MAX_PATH];
    DWORD timestamp,elapsed;

    /* simple parameter checking */
    if (!hwnd || !charCode || !keyData)
        return 0;

    infoPtr=(TREEVIEW_INFO*)GetWindowLongW(hwnd, 0);
    if (!infoPtr)
        return 0;

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
        if (infoPtr->nSearchParamLength < sizeof(infoPtr->szSearchParam) / sizeof(WCHAR)) {
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
        if (idx == NULL) {
            if (endidx == NULL)
                break;
            idx=infoPtr->root->firstChild;
        }

        /* get item */
        ZeroMemory(&item, sizeof(item));
        item.mask = TVIF_TEXT;
        item.hItem = idx;
        item.pszText = buffer;
        item.cchTextMax = sizeof(buffer);
        TREEVIEW_GetItemT( infoPtr, &item, TRUE );

        /* check for a match */
        if (strncmpiW(item.pszText,infoPtr->szSearchParam,infoPtr->nSearchParamLength) == 0) {
            nItem=idx;
            break;
        } else if ( (charCode != 0) && (nItem == NULL) &&
                    (nItem != infoPtr->selectedItem) &&
                    (strncmpiW(item.pszText,infoPtr->szSearchParam,1) == 0) ) {
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

        /* see if we are trying to ensure that root is vislble */
        if((item != infoPtr->root) && TREEVIEW_ValidItem(infoPtr, item))
          parent = item->parent;
        else
          parent = item; /* this item is the topmost item */

	while (parent != infoPtr->root)
	{
	    if (!(parent->state & TVIS_EXPANDED))
		TREEVIEW_Expand(infoPtr, parent, FALSE, FALSE);

	    parent = parent->parent;
	}
    }

    viscount = TREEVIEW_GetVisibleCount(infoPtr);

    TRACE("%p (%s) %ld - %ld viscount(%d)\n", item, TREEVIEW_ItemName(item), item->visibleOrder, 
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

	    ScrollWindow(infoPtr->hwnd, 0, scroll, NULL, NULL);
	    UpdateWindow(infoPtr->hwnd);
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

    TRACE("wp %x\n", wParam);

    if (!(infoPtr->uInternalStatus & TV_VSCROLL))
	return 0;

    if (infoPtr->hwndEdit)
	SetFocus(infoPtr->hwnd);

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

    TRACE("wp %x\n", wParam);

    if (!(infoPtr->uInternalStatus & TV_HSCROLL))
	return FALSE;

    if (infoPtr->hwndEdit)
	SetFocus(infoPtr->hwnd);

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
TREEVIEW_MouseWheel(TREEVIEW_INFO *infoPtr, WPARAM wParam)
{
    short gcWheelDelta;
    UINT pulScrollLines = 3;

    if (infoPtr->firstVisible == NULL)
	return TRUE;

    SystemParametersInfoW(SPI_GETWHEELSCROLLLINES, 0, &pulScrollLines, 0);

    gcWheelDelta = -(short)HIWORD(wParam);
    pulScrollLines *= (gcWheelDelta / WHEEL_DELTA);

    if (abs(gcWheelDelta) >= WHEEL_DELTA && pulScrollLines)
    {
	int newDy = infoPtr->firstVisible->visibleOrder + pulScrollLines;
	int maxDy = infoPtr->maxVisibleOrder;

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

    TRACE("wnd %p, style %lx\n", hwnd, GetWindowLongW(hwnd, GWL_STYLE));

    infoPtr = (TREEVIEW_INFO *)Alloc(sizeof(TREEVIEW_INFO));

    if (infoPtr == NULL)
    {
	ERR("could not allocate info memory!\n");
	return 0;
    }

    SetWindowLongW(hwnd, 0, (DWORD)infoPtr);

    infoPtr->hwnd = hwnd;
    infoPtr->dwStyle = GetWindowLongW(hwnd, GWL_STYLE);
    infoPtr->uInternalStatus = 0;
    infoPtr->Timer = 0;
    infoPtr->uNumItems = 0;
    infoPtr->cdmode = 0;
    infoPtr->uScrollTime = 300;	/* milliseconds */
    infoPtr->bRedraw = TRUE;

    GetClientRect(hwnd, &rcClient);

    /* No scroll bars yet. */
    infoPtr->clientWidth = rcClient.right;
    infoPtr->clientHeight = rcClient.bottom;

    infoPtr->treeWidth = 0;
    infoPtr->treeHeight = 0;

    infoPtr->uIndent = 19;
    infoPtr->selectedItem = 0;
    infoPtr->focusedItem = 0;
    /* hotItem? */
    infoPtr->firstVisible = 0;
    infoPtr->maxVisibleOrder = 0;
    infoPtr->dropItem = 0;
    infoPtr->insertMarkItem = 0;
    infoPtr->insertBeforeorAfter = 0;
    /* dragList */

    infoPtr->scrollX = 0;

    infoPtr->clrBk = GetSysColor(COLOR_WINDOW);
    infoPtr->clrText = -1;	/* use system color */
    infoPtr->clrLine = RGB(128, 128, 128);
    infoPtr->clrInsertMark = GetSysColor(COLOR_BTNTEXT);

    /* hwndToolTip */

    infoPtr->hwndEdit = 0;
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

    infoPtr->hFont = GetStockObject(DEFAULT_GUI_FONT);
    infoPtr->hBoldFont = TREEVIEW_CreateBoldFont(infoPtr->hFont);

    infoPtr->uItemHeight = TREEVIEW_NaturalHeight(infoPtr);

    infoPtr->root = TREEVIEW_AllocateItem(infoPtr);
    infoPtr->root->state = TVIS_EXPANDED;
    infoPtr->root->iLevel = -1;
    infoPtr->root->visibleOrder = -1;

    infoPtr->hwndNotify = lpcs->hwndParent;
#if 0
    infoPtr->bTransparent = ( GetWindowLongW( hwnd, GWL_STYLE) & TBSTYLE_FLAT);
#endif

    infoPtr->hwndToolTip = 0;

    infoPtr->bNtfUnicode = IsWindowUnicode (hwnd);

    /* Determine what type of notify should be issued */
    /* sets infoPtr->bNtfUnicode */
    TREEVIEW_NotifyFormat(infoPtr, infoPtr->hwndNotify, NF_REQUERY);

    if (!(infoPtr->dwStyle & TVS_NOTOOLTIPS))
	infoPtr->hwndToolTip = COMCTL32_CreateToolTip(hwnd);

    if (infoPtr->dwStyle & TVS_CHECKBOXES)
    {
	RECT rc;
	HBITMAP hbm, hbmOld;
	HDC hdc,hdcScreen;
	int nIndex;

	infoPtr->himlState =
	    ImageList_Create(16, 16, ILC_COLOR | ILC_MASK, 3, 0);

	hdcScreen = CreateDCA("DISPLAY", NULL, NULL, NULL);

	/* Create a coloured bitmap compatible with the screen depth
	   because checkboxes are not black&white */
	hdc = CreateCompatibleDC(hdcScreen);
	hbm = CreateCompatibleBitmap(hdcScreen, 48, 16);
	hbmOld = SelectObject(hdc, hbm);

	rc.left  = 0;   rc.top    = 0;
	rc.right = 48;  rc.bottom = 16;
	FillRect(hdc, &rc, (HBRUSH)(COLOR_WINDOW+1));

	rc.left  = 18;   rc.top    = 2;
	rc.right = 30;   rc.bottom = 14;
	DrawFrameControl(hdc, &rc, DFC_BUTTON,
	                  DFCS_BUTTONCHECK|DFCS_FLAT);

	rc.left  = 34;   rc.right  = 46;
	DrawFrameControl(hdc, &rc, DFC_BUTTON,
	                  DFCS_BUTTONCHECK|DFCS_FLAT|DFCS_CHECKED);

	SelectObject(hdc, hbmOld);
	nIndex = ImageList_AddMasked(infoPtr->himlState, hbm,
	                              GetSysColor(COLOR_WINDOW));
	TRACE("checkbox index %d\n", nIndex);

	DeleteObject(hbm);
	DeleteDC(hdc);
	DeleteDC(hdcScreen);

	infoPtr->stateImageWidth = 16;
	infoPtr->stateImageHeight = 16;
    }
    return 0;
}


static LRESULT
TREEVIEW_Destroy(TREEVIEW_INFO *infoPtr)
{
    TRACE("\n");

    TREEVIEW_RemoveTree(infoPtr);

    /* tool tip is automatically destroyed: we are its owner */

    /* Restore original wndproc */
    if (infoPtr->hwndEdit)
	SetWindowLongW(infoPtr->hwndEdit, GWL_WNDPROC,
		       (LONG)infoPtr->wpEditOrig);

    /* Deassociate treeview from the window before doing anything drastic. */
    SetWindowLongW(infoPtr->hwnd, 0, (LONG)NULL);

    DeleteObject(infoPtr->hBoldFont);
    Free(infoPtr);

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

    TRACE("%x\n", wParam);

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
	if (!(prevItem->state & TVIS_EXPANDED))
	    TREEVIEW_Expand(infoPtr, prevItem, FALSE, TRUE);
	break;

    case VK_SUBTRACT:
	if (prevItem->state & TVIS_EXPANDED)
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
TREEVIEW_Notify(TREEVIEW_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    LPNMHDR lpnmh = (LPNMHDR)lParam;

    if (lpnmh->code == PGN_CALCSIZE) {
	LPNMPGCALCSIZE lppgc = (LPNMPGCALCSIZE)lParam;

	if (lppgc->dwFlag == PGF_CALCWIDTH) {
	    lppgc->iWidth = infoPtr->treeWidth;
	    TRACE("got PGN_CALCSIZE, returning horz size = %ld, client=%ld\n",
		  infoPtr->treeWidth, infoPtr->clientWidth);
	}
	else {
	    lppgc->iHeight = infoPtr->treeHeight;
	    TRACE("got PGN_CALCSIZE, returning vert size = %ld, client=%ld\n",
		  infoPtr->treeHeight, infoPtr->clientHeight);
	}
	return 0;
    }
    return DefWindowProcA(infoPtr->hwnd, WM_NOTIFY, wParam, lParam);
}

static INT TREEVIEW_NotifyFormat (TREEVIEW_INFO *infoPtr, HWND hwndFrom, UINT nCommand)
{
    INT format;
    
    TRACE("(hwndFrom=%p, nCommand=%d)\n", hwndFrom, nCommand);

    if (nCommand != NF_REQUERY) return 0;
    
    format = SendMessageW(hwndFrom, WM_NOTIFYFORMAT, (WPARAM)infoPtr->hwnd, NF_QUERY);
    TRACE("format=%d\n", format);

    if (format != NFR_ANSI && format != NFR_UNICODE) return 0;
    
    infoPtr->bNtfUnicode = (format == NFR_UNICODE);
    
    return format;
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
	FIXME("WM_SIZE flag %x %lx not handled\n", wParam, lParam);
    }

    TREEVIEW_Invalidate(infoPtr, NULL);
    return 0;
}

static LRESULT
TREEVIEW_StyleChanged(TREEVIEW_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
    TRACE("(%x %lx)\n", wParam, lParam);

    if (wParam == GWL_STYLE)
    {
       DWORD dwNewStyle = ((LPSTYLESTRUCT)lParam)->styleNew;

       /* we have to take special care about tooltips */
       if ((infoPtr->dwStyle ^ dwNewStyle) & TVS_NOTOOLTIPS)
       {
          if (infoPtr->dwStyle & TVS_NOTOOLTIPS)
          {
              infoPtr->hwndToolTip = COMCTL32_CreateToolTip(infoPtr->hwnd);
              TRACE("\n");
          }
          else
          {
             DestroyWindow(infoPtr->hwndToolTip);
             infoPtr->hwndToolTip = 0;
          }
       }

       infoPtr->dwStyle = dwNewStyle;
    }

    TREEVIEW_UpdateSubTree(infoPtr, infoPtr->root);
    TREEVIEW_UpdateScrollBars(infoPtr);
    TREEVIEW_Invalidate(infoPtr, NULL);

    return 0;
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

    TREEVIEW_SendSimpleNotify(infoPtr, NM_SETFOCUS);
    TREEVIEW_Invalidate(infoPtr, infoPtr->selectedItem);
    return 0;
}

static LRESULT
TREEVIEW_KillFocus(TREEVIEW_INFO *infoPtr)
{
    TRACE("\n");

    TREEVIEW_SendSimpleNotify(infoPtr, NM_KILLFOCUS);
    TREEVIEW_Invalidate(infoPtr, infoPtr->selectedItem);
    return 0;
}


static LRESULT WINAPI
TREEVIEW_WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TREEVIEW_INFO *infoPtr = TREEVIEW_GetInfoPtr(hwnd);

    TRACE("hwnd %p msg %04x wp=%08x lp=%08lx\n", hwnd, uMsg, wParam, lParam);

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
	return TREEVIEW_CreateDragImage(infoPtr, wParam, lParam);

    case TVM_DELETEITEM:
	return TREEVIEW_DeleteItem(infoPtr, (HTREEITEM)lParam);

    case TVM_EDITLABELA:
	return (LRESULT)TREEVIEW_EditLabel(infoPtr, (HTREEITEM)lParam);

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
	return TREEVIEW_GetItemT(infoPtr, (LPTVITEMEXW)lParam, FALSE);

    case TVM_GETITEMW:
	return TREEVIEW_GetItemT(infoPtr, (LPTVITEMEXW)lParam, TRUE);

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
	return TREEVIEW_HitTest(infoPtr, (LPTVHITTESTINFO)lParam);

    case TVM_INSERTITEMA:
	return TREEVIEW_InsertItemT(infoPtr, (LPTVINSERTSTRUCTW)lParam, FALSE);

    case TVM_INSERTITEMW:
	return TREEVIEW_InsertItemT(infoPtr, (LPTVINSERTSTRUCTW)lParam, TRUE);

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
	return TREEVIEW_SetItemT(infoPtr, (LPTVITEMEXW)lParam, FALSE);

    case TVM_SETITEMW:
        return TREEVIEW_SetItemT(infoPtr, (LPTVITEMEXW)lParam, TRUE);

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
	return TREEVIEW_SortChildren(infoPtr, wParam, lParam);

    case TVM_SORTCHILDRENCB:
	return TREEVIEW_SortChildrenCB(infoPtr, wParam, (LPTVSORTCB)lParam);

    case WM_CHAR:
        return TREEVIEW_ProcessLetterKeys( hwnd, wParam, lParam );

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
	return TREEVIEW_KeyDown(infoPtr, wParam);

    case WM_KILLFOCUS:
	return TREEVIEW_KillFocus(infoPtr);

    case WM_LBUTTONDBLCLK:
	return TREEVIEW_LButtonDoubleClick(infoPtr, lParam);

    case WM_LBUTTONDOWN:
	return TREEVIEW_LButtonDown(infoPtr, lParam);

	/* WM_MBUTTONDOWN */

	/* WM_MOUSEMOVE */

    case WM_NOTIFY:
	return TREEVIEW_Notify(infoPtr, wParam, lParam);

    case WM_NOTIFYFORMAT:
	return TREEVIEW_NotifyFormat(infoPtr, (HWND)wParam, (UINT)lParam);

    case WM_PAINT:
	return TREEVIEW_Paint(infoPtr, wParam);

	/* WM_PRINTCLIENT */

    case WM_RBUTTONDOWN:
	return TREEVIEW_RButtonDown(infoPtr, lParam);

    case WM_SETFOCUS:
	return TREEVIEW_SetFocus(infoPtr);

    case WM_SETFONT:
	return TREEVIEW_SetFont(infoPtr, (HFONT)wParam, (BOOL)lParam);

    case WM_SETREDRAW:
        return TREEVIEW_SetRedraw(infoPtr, wParam, lParam);

    case WM_SIZE:
	return TREEVIEW_Size(infoPtr, wParam, lParam);

    case WM_STYLECHANGED:
	return TREEVIEW_StyleChanged(infoPtr, wParam, lParam);

	/* WM_SYSCOLORCHANGE */

	/* WM_SYSKEYDOWN */

    case WM_TIMER:
	return TREEVIEW_HandleTimer(infoPtr, wParam);

    case WM_VSCROLL:
	return TREEVIEW_VScroll(infoPtr, wParam);

	/* WM_WININICHANGE */

    case WM_MOUSEWHEEL:
	if (wParam & (MK_SHIFT | MK_CONTROL))
	    goto def;
	return TREEVIEW_MouseWheel(infoPtr, wParam);

    case WM_DRAWITEM:
	TRACE("drawItem\n");
	goto def;

    default:
	/* This mostly catches MFC and Delphi messages. :( */
	if ((uMsg >= WM_USER) && (uMsg < WM_APP))
	    TRACE("Unknown msg %04x wp=%08x lp=%08lx\n", uMsg, wParam, lParam);
def:
	return DefWindowProcA(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}


/* Class Registration ***************************************************/

VOID
TREEVIEW_Register(void)
{
    WNDCLASSA wndClass;

    TRACE("\n");

    ZeroMemory(&wndClass, sizeof(WNDCLASSA));
    wndClass.style = CS_GLOBALCLASS | CS_DBLCLKS;
    wndClass.lpfnWndProc = (WNDPROC)TREEVIEW_WindowProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = sizeof(TREEVIEW_INFO *);

    wndClass.hCursor = LoadCursorA(0, (LPSTR)IDC_ARROW);
    wndClass.hbrBackground = 0;
    wndClass.lpszClassName = WC_TREEVIEWA;

    RegisterClassA(&wndClass);
}


VOID
TREEVIEW_Unregister(void)
{
    UnregisterClassA(WC_TREEVIEWA, NULL);
}


/* Tree Verification ****************************************************/

#ifdef NDEBUG
static inline void
TREEVIEW_VerifyChildren(TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *item);

static inline void TREEVIEW_VerifyItemCommon(TREEVIEW_INFO *infoPtr,
					     TREEVIEW_ITEM *item)
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
TREEVIEW_VerifyItem(TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *item)
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
TREEVIEW_VerifyChildren(TREEVIEW_INFO *infoPtr, TREEVIEW_ITEM *item)
{
    TREEVIEW_ITEM *child;
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
    assert(infoPtr != NULL);

    TREEVIEW_VerifyRoot(infoPtr);
}
#endif
