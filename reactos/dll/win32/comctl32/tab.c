/*
 * Tab control
 *
 * Copyright 1998 Anders Carlsson
 * Copyright 1999 Alex Priem <alexp@sci.kun.nl>
 * Copyright 1999 Francis Beaudet
 * Copyright 2003 Vitaliy Margolen
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
 * This code was audited for completeness against the documented features
 * of Comctl32.dll version 6.0 on May. 20, 2005, by James Hawkins.
 *
 * Unless otherwise noted, we believe this code to be complete, as per
 * the specification mentioned above.
 * If you discover missing features, or bugs, please note them below.
 *
 * TODO:
 *
 *  Styles:
 *   TCS_MULTISELECT
 *   TCS_RIGHT
 *   TCS_RIGHTJUSTIFY
 *   TCS_SCROLLOPPOSITE
 *   TCS_SINGLELINE
 *   TCIF_RTLREADING
 *
 *  Extended Styles:
 *   TCS_EX_FLATSEPARATORS
 *   TCS_EX_REGISTERDROP
 *
 *  States:
 *   TCIS_BUTTONPRESSED
 *
 *  Notifications:
 *   NM_RELEASEDCAPTURE
 *   TCN_FOCUSCHANGE
 *   TCN_GETOBJECT
 *   TCN_KEYDOWN
 *
 *  Messages:
 *   TCM_REMOVEIMAGE
 *   TCM_DESELECTALL
 *   TCM_GETEXTENDEDSTYLE
 *   TCM_SETEXTENDEDSTYLE
 *
 *  Macros:
 *   TabCtrl_AdjustRect
 *
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
#include "tmschema.h"
#include "wine/debug.h"
#include <math.h>

WINE_DEFAULT_DEBUG_CHANNEL(tab);

typedef struct
{
  DWORD  dwState;
  LPWSTR pszText;
  INT    iImage;
  RECT   rect;      /* bounding rectangle of the item relative to the
                     * leftmost item (the leftmost item, 0, would have a
                     * "left" member of 0 in this rectangle)
                     *
                     * additionally the top member holds the row number
                     * and bottom is unused and should be 0 */
  BYTE   extra[1];  /* Space for caller supplied info, variable size */
} TAB_ITEM;

/* The size of a tab item depends on how much extra data is requested */
#define TAB_ITEM_SIZE(infoPtr) (FIELD_OFFSET(TAB_ITEM, extra[(infoPtr)->cbInfo]))

typedef struct
{
  HWND       hwnd;            /* Tab control window */
  HWND       hwndNotify;      /* notification window (parent) */
  UINT       uNumItem;        /* number of tab items */
  UINT       uNumRows;	      /* number of tab rows */
  INT        tabHeight;       /* height of the tab row */
  INT        tabWidth;        /* width of tabs */
  INT        tabMinWidth;     /* minimum width of items */
  USHORT     uHItemPadding;   /* amount of horizontal padding, in pixels */
  USHORT     uVItemPadding;   /* amount of vertical padding, in pixels */
  USHORT     uHItemPadding_s; /* Set amount of horizontal padding, in pixels */
  USHORT     uVItemPadding_s; /* Set amount of vertical padding, in pixels */
  HFONT      hFont;           /* handle to the current font */
  HCURSOR    hcurArrow;       /* handle to the current cursor */
  HIMAGELIST himl;            /* handle to an image list (may be 0) */
  HWND       hwndToolTip;     /* handle to tab's tooltip */
  INT        leftmostVisible; /* Used for scrolling, this member contains
                               * the index of the first visible item */
  INT        iSelected;       /* the currently selected item */
  INT        iHotTracked;     /* the highlighted item under the mouse */
  INT        uFocus;          /* item which has the focus */
  TAB_ITEM*  items;           /* pointer to an array of TAB_ITEM's */
  BOOL       DoRedraw;        /* flag for redrawing when tab contents is changed*/
  BOOL       needsScrolling;  /* TRUE if the size of the tabs is greater than
                               * the size of the control */
  BOOL       fHeightSet;      /* was the height of the tabs explicitly set? */
  BOOL       bUnicode;        /* Unicode control? */
  HWND       hwndUpDown;      /* Updown control used for scrolling */
  INT        cbInfo;          /* Number of bytes of caller supplied info per tab */
} TAB_INFO;

/******************************************************************************
 * Positioning constants
 */
#define SELECTED_TAB_OFFSET     2
#define ROUND_CORNER_SIZE       2
#define DISPLAY_AREA_PADDINGX   2
#define DISPLAY_AREA_PADDINGY   2
#define CONTROL_BORDER_SIZEX    2
#define CONTROL_BORDER_SIZEY    2
#define BUTTON_SPACINGX         3
#define BUTTON_SPACINGY         3
#define FLAT_BTN_SPACINGX       8
#define DEFAULT_MIN_TAB_WIDTH   54
#define DEFAULT_PADDING_X       6
#define EXTRA_ICON_PADDING      3

#define TAB_GetInfoPtr(hwnd) ((TAB_INFO *)GetWindowLongPtrW(hwnd,0))
/* Since items are variable sized, cannot directly access them */
#define TAB_GetItem(info,i) \
  ((TAB_ITEM*)((LPBYTE)info->items + (i) * TAB_ITEM_SIZE(info)))

#define GET_DEFAULT_MIN_TAB_WIDTH(infoPtr) (DEFAULT_MIN_TAB_WIDTH - (DEFAULT_PADDING_X - (infoPtr)->uHItemPadding) * 2)

/******************************************************************************
 * Hot-tracking timer constants
 */
#define TAB_HOTTRACK_TIMER            1
#define TAB_HOTTRACK_TIMER_INTERVAL   100   /* milliseconds */

static const WCHAR themeClass[] = { 'T','a','b',0 };

/******************************************************************************
 * Prototypes
 */
static void TAB_InvalidateTabArea(const TAB_INFO *);
static void TAB_EnsureSelectionVisible(TAB_INFO *);
static void TAB_DrawItemInterior(const TAB_INFO *, HDC, INT, RECT*);

static BOOL
TAB_SendSimpleNotify (const TAB_INFO *infoPtr, UINT code)
{
    NMHDR nmhdr;

    nmhdr.hwndFrom = infoPtr->hwnd;
    nmhdr.idFrom = GetWindowLongPtrW(infoPtr->hwnd, GWLP_ID);
    nmhdr.code = code;

    return (BOOL) SendMessageW (infoPtr->hwndNotify, WM_NOTIFY,
            nmhdr.idFrom, (LPARAM) &nmhdr);
}

static void
TAB_RelayEvent (HWND hwndTip, HWND hwndMsg, UINT uMsg,
            WPARAM wParam, LPARAM lParam)
{
    MSG msg;

    msg.hwnd = hwndMsg;
    msg.message = uMsg;
    msg.wParam = wParam;
    msg.lParam = lParam;
    msg.time = GetMessageTime ();
    msg.pt.x = (short)LOWORD(GetMessagePos ());
    msg.pt.y = (short)HIWORD(GetMessagePos ());

    SendMessageW (hwndTip, TTM_RELAYEVENT, 0, (LPARAM)&msg);
}

static void
TAB_DumpItemExternalT(const TCITEMW *pti, UINT iItem, BOOL isW)
{
    if (TRACE_ON(tab)) {
	TRACE("external tab %d, mask=0x%08x, dwState=0x%08x, dwStateMask=0x%08x, cchTextMax=0x%08x\n",
	      iItem, pti->mask, pti->dwState, pti->dwStateMask, pti->cchTextMax);
	TRACE("external tab %d,   iImage=%d, lParam=0x%08lx, pszTextW=%s\n",
	      iItem, pti->iImage, pti->lParam, isW ? debugstr_w(pti->pszText) : debugstr_a((LPSTR)pti->pszText));
    }
}

static void
TAB_DumpItemInternal(const TAB_INFO *infoPtr, UINT iItem)
{
    if (TRACE_ON(tab)) {
	TAB_ITEM *ti;

	ti = TAB_GetItem(infoPtr, iItem);
	TRACE("tab %d, dwState=0x%08x, pszText=%s, iImage=%d\n",
	      iItem, ti->dwState, debugstr_w(ti->pszText), ti->iImage);
	TRACE("tab %d, rect.left=%d, rect.top(row)=%d\n",
	      iItem, ti->rect.left, ti->rect.top);
    }
}

/* RETURNS
 *   the index of the selected tab, or -1 if no tab is selected. */
static inline LRESULT TAB_GetCurSel (const TAB_INFO *infoPtr)
{
    return infoPtr->iSelected;
}

/* RETURNS
 *   the index of the tab item that has the focus. */
static inline LRESULT
TAB_GetCurFocus (const TAB_INFO *infoPtr)
{
    return infoPtr->uFocus;
}

static inline LRESULT TAB_GetToolTips (const TAB_INFO *infoPtr)
{
    if (infoPtr == NULL) return 0;
    return (LRESULT)infoPtr->hwndToolTip;
}

static inline LRESULT TAB_SetCurSel (TAB_INFO *infoPtr, INT iItem)
{
  INT prevItem = infoPtr->iSelected;

  if (iItem < 0)
      infoPtr->iSelected=-1;
  else if (iItem >= infoPtr->uNumItem)
      return -1;
  else {
      if (infoPtr->iSelected != iItem) {
          infoPtr->iSelected=iItem;
          infoPtr->uFocus=iItem;
          TAB_EnsureSelectionVisible(infoPtr);
          TAB_InvalidateTabArea(infoPtr);
      }
  }
  return prevItem;
}

static LRESULT TAB_SetCurFocus (TAB_INFO *infoPtr, INT iItem)
{
  if (iItem < 0)
      infoPtr->uFocus = -1;
  else if (iItem < infoPtr->uNumItem) {
    if (GetWindowLongW(infoPtr->hwnd, GWL_STYLE) & TCS_BUTTONS) {
      FIXME("Should set input focus\n");
    } else {
      int oldFocus = infoPtr->uFocus;
      if (infoPtr->iSelected != iItem || oldFocus == -1 ) {
        infoPtr->uFocus = iItem;
        if (oldFocus != -1) {
          if (!TAB_SendSimpleNotify(infoPtr, TCN_SELCHANGING))  {
            infoPtr->iSelected = iItem;
            TAB_SendSimpleNotify(infoPtr, TCN_SELCHANGE);
          }
          else
            infoPtr->iSelected = iItem;
          TAB_EnsureSelectionVisible(infoPtr);
          TAB_InvalidateTabArea(infoPtr);
        }
      }
    }
  }
  return 0;
}

static inline LRESULT
TAB_SetToolTips (TAB_INFO *infoPtr, HWND hwndToolTip)
{
    if (infoPtr)
        infoPtr->hwndToolTip = hwndToolTip;
    return 0;
}

static inline LRESULT
TAB_SetPadding (TAB_INFO *infoPtr, LPARAM lParam)
{
    if (infoPtr)
    {
        infoPtr->uHItemPadding_s=LOWORD(lParam);
        infoPtr->uVItemPadding_s=HIWORD(lParam);
    }
    return 0;
}

/******************************************************************************
 * TAB_InternalGetItemRect
 *
 * This method will calculate the rectangle representing a given tab item in
 * client coordinates. This method takes scrolling into account.
 *
 * This method returns TRUE if the item is visible in the window and FALSE
 * if it is completely outside the client area.
 */
static BOOL TAB_InternalGetItemRect(
  const TAB_INFO* infoPtr,
  INT         itemIndex,
  RECT*       itemRect,
  RECT*       selectedRect)
{
  RECT tmpItemRect,clientRect;
  LONG lStyle = GetWindowLongW(infoPtr->hwnd, GWL_STYLE);

  /* Perform a sanity check and a trivial visibility check. */
  if ( (infoPtr->uNumItem <= 0) ||
       (itemIndex >= infoPtr->uNumItem) ||
       (!((lStyle & TCS_MULTILINE) || (lStyle & TCS_VERTICAL)) && (itemIndex < infoPtr->leftmostVisible)) )
    {
        TRACE("Not Visible\n");
        /* need to initialize these to empty rects */
        if (itemRect)
        {
            memset(itemRect,0,sizeof(RECT));
            itemRect->bottom = infoPtr->tabHeight;
        }
        if (selectedRect)
            memset(selectedRect,0,sizeof(RECT));
        return FALSE;
    }

  /*
   * Avoid special cases in this procedure by assigning the "out"
   * parameters if the caller didn't supply them
   */
  if (itemRect == NULL)
    itemRect = &tmpItemRect;

  /* Retrieve the unmodified item rect. */
  *itemRect = TAB_GetItem(infoPtr,itemIndex)->rect;

  /* calculate the times bottom and top based on the row */
  GetClientRect(infoPtr->hwnd, &clientRect);

  if ((lStyle & TCS_BOTTOM) && (lStyle & TCS_VERTICAL))
  {
    itemRect->right  = clientRect.right - SELECTED_TAB_OFFSET - itemRect->left * infoPtr->tabHeight -
                       ((lStyle & TCS_BUTTONS) ? itemRect->left * BUTTON_SPACINGX : 0);
    itemRect->left   = itemRect->right - infoPtr->tabHeight;
  }
  else if (lStyle & TCS_VERTICAL)
  {
    itemRect->left   = clientRect.left + SELECTED_TAB_OFFSET + itemRect->left * infoPtr->tabHeight +
                       ((lStyle & TCS_BUTTONS) ? itemRect->left * BUTTON_SPACINGX : 0);
    itemRect->right  = itemRect->left + infoPtr->tabHeight;
  }
  else if (lStyle & TCS_BOTTOM)
  {
    itemRect->bottom = clientRect.bottom - itemRect->top * infoPtr->tabHeight -
                       ((lStyle & TCS_BUTTONS) ? itemRect->top * BUTTON_SPACINGY : SELECTED_TAB_OFFSET);
    itemRect->top    = itemRect->bottom - infoPtr->tabHeight;
  }
  else /* not TCS_BOTTOM and not TCS_VERTICAL */
  {
    itemRect->top    = clientRect.top + itemRect->top * infoPtr->tabHeight +
                       ((lStyle & TCS_BUTTONS) ? itemRect->top * BUTTON_SPACINGY : SELECTED_TAB_OFFSET);
    itemRect->bottom = itemRect->top + infoPtr->tabHeight;
 }

  /*
   * "scroll" it to make sure the item at the very left of the
   * tab control is the leftmost visible tab.
   */
  if(lStyle & TCS_VERTICAL)
  {
    OffsetRect(itemRect,
	     0,
	     -TAB_GetItem(infoPtr, infoPtr->leftmostVisible)->rect.top);

    /*
     * Move the rectangle so the first item is slightly offset from
     * the bottom of the tab control.
     */
    OffsetRect(itemRect,
	     0,
	     SELECTED_TAB_OFFSET);

  } else
  {
    OffsetRect(itemRect,
	     -TAB_GetItem(infoPtr, infoPtr->leftmostVisible)->rect.left,
	     0);

    /*
     * Move the rectangle so the first item is slightly offset from
     * the left of the tab control.
     */
    OffsetRect(itemRect,
	     SELECTED_TAB_OFFSET,
	     0);
  }
  TRACE("item %d tab h=%d, rect=(%s)\n",
        itemIndex, infoPtr->tabHeight, wine_dbgstr_rect(itemRect));

  /* Now, calculate the position of the item as if it were selected. */
  if (selectedRect!=NULL)
  {
    CopyRect(selectedRect, itemRect);

    /* The rectangle of a selected item is a bit wider. */
    if(lStyle & TCS_VERTICAL)
      InflateRect(selectedRect, 0, SELECTED_TAB_OFFSET);
    else
      InflateRect(selectedRect, SELECTED_TAB_OFFSET, 0);

    /* If it also a bit higher. */
    if ((lStyle & TCS_BOTTOM) && (lStyle & TCS_VERTICAL))
    {
      selectedRect->left   -= 2; /* the border is thicker on the right */
      selectedRect->right  += SELECTED_TAB_OFFSET;
    }
    else if (lStyle & TCS_VERTICAL)
    {
      selectedRect->left   -= SELECTED_TAB_OFFSET;
      selectedRect->right  += 1;
    }
    else if (lStyle & TCS_BOTTOM)
    {
      selectedRect->bottom += SELECTED_TAB_OFFSET;
    }
    else /* not TCS_BOTTOM and not TCS_VERTICAL */
    {
      selectedRect->top    -= SELECTED_TAB_OFFSET;
      selectedRect->bottom -= 1;
    }
  }

  /* Check for visibility */
  if (lStyle & TCS_VERTICAL)
    return (itemRect->top < clientRect.bottom) && (itemRect->bottom > clientRect.top);
  else
    return (itemRect->left < clientRect.right) && (itemRect->right > clientRect.left);
}

static inline BOOL
TAB_GetItemRect(const TAB_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
  return TAB_InternalGetItemRect(infoPtr, wParam, (LPRECT)lParam, NULL);
}

/******************************************************************************
 * TAB_KeyUp
 *
 * This method is called to handle keyboard input
 */
static LRESULT TAB_KeyUp(TAB_INFO* infoPtr, WPARAM keyCode)
{
  int       newItem = -1;

  switch (keyCode)
  {
    case VK_LEFT:
      newItem = infoPtr->uFocus - 1;
      break;
    case VK_RIGHT:
      newItem = infoPtr->uFocus + 1;
      break;
  }

  /*
   * If we changed to a valid item, change the selection
   */
  if (newItem >= 0 &&
      newItem < infoPtr->uNumItem &&
      infoPtr->uFocus != newItem)
  {
    if (!TAB_SendSimpleNotify(infoPtr, TCN_SELCHANGING))
    {
      infoPtr->iSelected = newItem;
      infoPtr->uFocus    = newItem;
      TAB_SendSimpleNotify(infoPtr, TCN_SELCHANGE);

      TAB_EnsureSelectionVisible(infoPtr);
      TAB_InvalidateTabArea(infoPtr);
    }
  }

  return 0;
}

/******************************************************************************
 * TAB_FocusChanging
 *
 * This method is called whenever the focus goes in or out of this control
 * it is used to update the visual state of the control.
 */
static void TAB_FocusChanging(const TAB_INFO *infoPtr)
{
  RECT      selectedRect;
  BOOL      isVisible;

  /*
   * Get the rectangle for the item.
   */
  isVisible = TAB_InternalGetItemRect(infoPtr,
				      infoPtr->uFocus,
				      NULL,
				      &selectedRect);

  /*
   * If the rectangle is not completely invisible, invalidate that
   * portion of the window.
   */
  if (isVisible)
  {
    TRACE("invalidate (%s)\n", wine_dbgstr_rect(&selectedRect));
    InvalidateRect(infoPtr->hwnd, &selectedRect, TRUE);
  }
}

static INT TAB_InternalHitTest (const TAB_INFO *infoPtr, POINT pt, UINT *flags)
{
  RECT rect;
  INT iCount;

  for (iCount = 0; iCount < infoPtr->uNumItem; iCount++)
  {
    TAB_InternalGetItemRect(infoPtr, iCount, &rect, NULL);

    if (PtInRect(&rect, pt))
    {
      *flags = TCHT_ONITEM;
      return iCount;
    }
  }

  *flags = TCHT_NOWHERE;
  return -1;
}

static inline LRESULT
TAB_HitTest (const TAB_INFO *infoPtr, LPTCHITTESTINFO lptest)
{
  return TAB_InternalHitTest (infoPtr, lptest->pt, &lptest->flags);
}

/******************************************************************************
 * TAB_NCHitTest
 *
 * Napster v2b5 has a tab control for its main navigation which has a client
 * area that covers the whole area of the dialog pages.
 * That's why it receives all msgs for that area and the underlying dialog ctrls
 * are dead.
 * So I decided that we should handle WM_NCHITTEST here and return
 * HTTRANSPARENT if we don't hit the tab control buttons.
 * FIXME: WM_NCHITTEST handling correct ? Fix it if you know that Windows
 * doesn't do it that way. Maybe depends on tab control styles ?
 */
static inline LRESULT
TAB_NCHitTest (const TAB_INFO *infoPtr, LPARAM lParam)
{
  POINT pt;
  UINT dummyflag;

  pt.x = (short)LOWORD(lParam);
  pt.y = (short)HIWORD(lParam);
  ScreenToClient(infoPtr->hwnd, &pt);

  if (TAB_InternalHitTest(infoPtr, pt, &dummyflag) == -1)
    return HTTRANSPARENT;
  else
    return HTCLIENT;
}

static LRESULT
TAB_LButtonDown (TAB_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
  POINT pt;
  INT newItem;
  UINT dummy;

  if (infoPtr->hwndToolTip)
    TAB_RelayEvent (infoPtr->hwndToolTip, infoPtr->hwnd,
		    WM_LBUTTONDOWN, wParam, lParam);

  if (GetWindowLongW(infoPtr->hwnd, GWL_STYLE) & TCS_FOCUSONBUTTONDOWN ) {
    SetFocus (infoPtr->hwnd);
  }

  if (infoPtr->hwndToolTip)
    TAB_RelayEvent (infoPtr->hwndToolTip, infoPtr->hwnd,
		    WM_LBUTTONDOWN, wParam, lParam);

  pt.x = (short)LOWORD(lParam);
  pt.y = (short)HIWORD(lParam);

  newItem = TAB_InternalHitTest (infoPtr, pt, &dummy);

  TRACE("On Tab, item %d\n", newItem);

  if (newItem != -1 && infoPtr->iSelected != newItem)
  {
    if (!TAB_SendSimpleNotify(infoPtr, TCN_SELCHANGING))
    {
      infoPtr->iSelected = newItem;
      infoPtr->uFocus    = newItem;
      TAB_SendSimpleNotify(infoPtr, TCN_SELCHANGE);

      TAB_EnsureSelectionVisible(infoPtr);

      TAB_InvalidateTabArea(infoPtr);
    }
  }
  return 0;
}

static inline LRESULT
TAB_LButtonUp (const TAB_INFO *infoPtr)
{
  TAB_SendSimpleNotify(infoPtr, NM_CLICK);

  return 0;
}

static inline LRESULT
TAB_RButtonDown (const TAB_INFO *infoPtr)
{
  TAB_SendSimpleNotify(infoPtr, NM_RCLICK);
  return 0;
}

/******************************************************************************
 * TAB_DrawLoneItemInterior
 *
 * This calls TAB_DrawItemInterior.  However, TAB_DrawItemInterior is normally
 * called by TAB_DrawItem which is normally called by TAB_Refresh which sets
 * up the device context and font.  This routine does the same setup but
 * only calls TAB_DrawItemInterior for the single specified item.
 */
static void
TAB_DrawLoneItemInterior(const TAB_INFO* infoPtr, int iItem)
{
  HDC hdc = GetDC(infoPtr->hwnd);
  RECT r, rC;

  /* Clip UpDown control to not draw over it */
  if (infoPtr->needsScrolling)
  {
    GetWindowRect(infoPtr->hwnd, &rC);
    GetWindowRect(infoPtr->hwndUpDown, &r);
    ExcludeClipRect(hdc, r.left - rC.left, r.top - rC.top, r.right - rC.left, r.bottom - rC.top);
  }
  TAB_DrawItemInterior(infoPtr, hdc, iItem, NULL);
  ReleaseDC(infoPtr->hwnd, hdc);
}

/* update a tab after hottracking - invalidate it or just redraw the interior,
 * based on whether theming is used or not */
static inline void hottrack_refresh(const TAB_INFO *infoPtr, int tabIndex)
{
    if (tabIndex == -1) return;

    if (GetWindowTheme (infoPtr->hwnd))
    {
        RECT rect;
        TAB_InternalGetItemRect(infoPtr, tabIndex, &rect, NULL);
        InvalidateRect (infoPtr->hwnd, &rect, FALSE);
    }
    else
        TAB_DrawLoneItemInterior(infoPtr, tabIndex);
}

/******************************************************************************
 * TAB_HotTrackTimerProc
 *
 * When a mouse-move event causes a tab to be highlighted (hot-tracking), a
 * timer is setup so we can check if the mouse is moved out of our window.
 * (We don't get an event when the mouse leaves, the mouse-move events just
 * stop being delivered to our window and just start being delivered to
 * another window.)  This function is called when the timer triggers so
 * we can check if the mouse has left our window.  If so, we un-highlight
 * the hot-tracked tab.
 */
static void CALLBACK
TAB_HotTrackTimerProc
  (
  HWND hwnd,    /* handle of window for timer messages */
  UINT uMsg,    /* WM_TIMER message */
  UINT_PTR idEvent, /* timer identifier */
  DWORD dwTime  /* current system time */
  )
{
  TAB_INFO* infoPtr = TAB_GetInfoPtr(hwnd);

  if (infoPtr != NULL && infoPtr->iHotTracked >= 0)
  {
    POINT pt;

    /*
    ** If we can't get the cursor position, or if the cursor is outside our
    ** window, we un-highlight the hot-tracked tab.  Note that the cursor is
    ** "outside" even if it is within our bounding rect if another window
    ** overlaps.  Note also that the case where the cursor stayed within our
    ** window but has moved off the hot-tracked tab will be handled by the
    ** WM_MOUSEMOVE event.
    */
    if (!GetCursorPos(&pt) || WindowFromPoint(pt) != hwnd)
    {
      /* Redraw iHotTracked to look normal */
      INT iRedraw = infoPtr->iHotTracked;
      infoPtr->iHotTracked = -1;
      hottrack_refresh (infoPtr, iRedraw);

      /* Kill this timer */
      KillTimer(hwnd, TAB_HOTTRACK_TIMER);
    }
  }
}

/******************************************************************************
 * TAB_RecalcHotTrack
 *
 * If a tab control has the TCS_HOTTRACK style, then the tab under the mouse
 * should be highlighted.  This function determines which tab in a tab control,
 * if any, is under the mouse and records that information.  The caller may
 * supply output parameters to receive the item number of the tab item which
 * was highlighted but isn't any longer and of the tab item which is now
 * highlighted but wasn't previously.  The caller can use this information to
 * selectively redraw those tab items.
 *
 * If the caller has a mouse position, it can supply it through the pos
 * parameter.  For example, TAB_MouseMove does this.  Otherwise, the caller
 * supplies NULL and this function determines the current mouse position
 * itself.
 */
static void
TAB_RecalcHotTrack
  (
  TAB_INFO*       infoPtr,
  const LPARAM*   pos,
  int*            out_redrawLeave,
  int*            out_redrawEnter
  )
{
  int item = -1;


  if (out_redrawLeave != NULL)
    *out_redrawLeave = -1;
  if (out_redrawEnter != NULL)
    *out_redrawEnter = -1;

  if ((GetWindowLongW(infoPtr->hwnd, GWL_STYLE) & TCS_HOTTRACK)
      || GetWindowTheme (infoPtr->hwnd))
  {
    POINT pt;
    UINT  flags;

    if (pos == NULL)
    {
      GetCursorPos(&pt);
      ScreenToClient(infoPtr->hwnd, &pt);
    }
    else
    {
      pt.x = (short)LOWORD(*pos);
      pt.y = (short)HIWORD(*pos);
    }

    item = TAB_InternalHitTest(infoPtr, pt, &flags);
  }

  if (item != infoPtr->iHotTracked)
  {
    if (infoPtr->iHotTracked >= 0)
    {
      /* Mark currently hot-tracked to be redrawn to look normal */
      if (out_redrawLeave != NULL)
        *out_redrawLeave = infoPtr->iHotTracked;

      if (item < 0)
      {
        /* Kill timer which forces recheck of mouse pos */
        KillTimer(infoPtr->hwnd, TAB_HOTTRACK_TIMER);
      }
    }
    else
    {
      /* Start timer so we recheck mouse pos */
      UINT timerID = SetTimer
        (
        infoPtr->hwnd,
        TAB_HOTTRACK_TIMER,
        TAB_HOTTRACK_TIMER_INTERVAL,
        TAB_HotTrackTimerProc
        );

      if (timerID == 0)
        return; /* Hot tracking not available */
    }

    infoPtr->iHotTracked = item;

    if (item >= 0)
    {
	/* Mark new hot-tracked to be redrawn to look highlighted */
      if (out_redrawEnter != NULL)
        *out_redrawEnter = item;
    }
  }
}

/******************************************************************************
 * TAB_MouseMove
 *
 * Handles the mouse-move event.  Updates tooltips.  Updates hot-tracking.
 */
static LRESULT
TAB_MouseMove (TAB_INFO *infoPtr, WPARAM wParam, LPARAM lParam)
{
  int redrawLeave;
  int redrawEnter;

  if (infoPtr->hwndToolTip)
    TAB_RelayEvent (infoPtr->hwndToolTip, infoPtr->hwnd,
		    WM_LBUTTONDOWN, wParam, lParam);

  /* Determine which tab to highlight.  Redraw tabs which change highlight
  ** status. */
  TAB_RecalcHotTrack(infoPtr, &lParam, &redrawLeave, &redrawEnter);

  hottrack_refresh (infoPtr, redrawLeave);
  hottrack_refresh (infoPtr, redrawEnter);

  return 0;
}

/******************************************************************************
 * TAB_AdjustRect
 *
 * Calculates the tab control's display area given the window rectangle or
 * the window rectangle given the requested display rectangle.
 */
static LRESULT TAB_AdjustRect(const TAB_INFO *infoPtr, WPARAM fLarger, LPRECT prc)
{
    DWORD lStyle = GetWindowLongW(infoPtr->hwnd, GWL_STYLE);
    LONG *iRightBottom, *iLeftTop;

    TRACE ("hwnd=%p fLarger=%ld (%s)\n", infoPtr->hwnd, fLarger,
           wine_dbgstr_rect(prc));

    if (!prc) return -1;

    if(lStyle & TCS_VERTICAL)
    {
	iRightBottom = &(prc->right);
	iLeftTop     = &(prc->left);
    }
    else
    {
	iRightBottom = &(prc->bottom);
	iLeftTop     = &(prc->top);
    }

    if (fLarger) /* Go from display rectangle */
    {
        /* Add the height of the tabs. */
	if (lStyle & TCS_BOTTOM)
	    *iRightBottom += infoPtr->tabHeight * infoPtr->uNumRows;
	else
	    *iLeftTop -= infoPtr->tabHeight * infoPtr->uNumRows +
			 ((lStyle & TCS_BUTTONS)? 3 * (infoPtr->uNumRows - 1) : 0);

	/* Inflate the rectangle for the padding */
	InflateRect(prc, DISPLAY_AREA_PADDINGX, DISPLAY_AREA_PADDINGY); 

	/* Inflate for the border */
	InflateRect(prc, CONTROL_BORDER_SIZEX, CONTROL_BORDER_SIZEY);
    }
    else /* Go from window rectangle. */
    {
	/* Deflate the rectangle for the border */
	InflateRect(prc, -CONTROL_BORDER_SIZEX, -CONTROL_BORDER_SIZEY);

	/* Deflate the rectangle for the padding */
	InflateRect(prc, -DISPLAY_AREA_PADDINGX, -DISPLAY_AREA_PADDINGY);

	/* Remove the height of the tabs. */
	if (lStyle & TCS_BOTTOM)
	    *iRightBottom -= infoPtr->tabHeight * infoPtr->uNumRows;
	else
	    *iLeftTop += (infoPtr->tabHeight) * infoPtr->uNumRows +
			 ((lStyle & TCS_BUTTONS)? 3 * (infoPtr->uNumRows - 1) : 0);
    }

  return 0;
}

/******************************************************************************
 * TAB_OnHScroll
 *
 * This method will handle the notification from the scroll control and
 * perform the scrolling operation on the tab control.
 */
static LRESULT TAB_OnHScroll(TAB_INFO *infoPtr, int nScrollCode, int nPos)
{
  if(nScrollCode == SB_THUMBPOSITION && nPos != infoPtr->leftmostVisible)
  {
     if(nPos < infoPtr->leftmostVisible)
        infoPtr->leftmostVisible--;
     else
        infoPtr->leftmostVisible++;

     TAB_RecalcHotTrack(infoPtr, NULL, NULL, NULL);
     TAB_InvalidateTabArea(infoPtr);
     SendMessageW(infoPtr->hwndUpDown, UDM_SETPOS, 0,
                   MAKELONG(infoPtr->leftmostVisible, 0));
   }

   return 0;
}

/******************************************************************************
 * TAB_SetupScrolling
 *
 * This method will check the current scrolling state and make sure the
 * scrolling control is displayed (or not).
 */
static void TAB_SetupScrolling(
  HWND        hwnd,
  TAB_INFO*   infoPtr,
  const RECT* clientRect)
{
  static const WCHAR msctls_updown32W[] = { 'm','s','c','t','l','s','_','u','p','d','o','w','n','3','2',0 };
  static const WCHAR emptyW[] = { 0 };
  INT maxRange = 0;
  DWORD lStyle = GetWindowLongW(hwnd, GWL_STYLE);

  if (infoPtr->needsScrolling)
  {
    RECT controlPos;
    INT vsize, tabwidth;

    /*
     * Calculate the position of the scroll control.
     */
    if(lStyle & TCS_VERTICAL)
    {
      controlPos.right = clientRect->right;
      controlPos.left  = controlPos.right - 2 * GetSystemMetrics(SM_CXHSCROLL);

      if (lStyle & TCS_BOTTOM)
      {
        controlPos.top    = clientRect->bottom - infoPtr->tabHeight;
        controlPos.bottom = controlPos.top + GetSystemMetrics(SM_CYHSCROLL);
      }
      else
      {
        controlPos.bottom = clientRect->top + infoPtr->tabHeight;
        controlPos.top    = controlPos.bottom - GetSystemMetrics(SM_CYHSCROLL);
      }
    }
    else
    {
      controlPos.right = clientRect->right;
      controlPos.left  = controlPos.right - 2 * GetSystemMetrics(SM_CXHSCROLL);

      if (lStyle & TCS_BOTTOM)
      {
        controlPos.top    = clientRect->bottom - infoPtr->tabHeight;
        controlPos.bottom = controlPos.top + GetSystemMetrics(SM_CYHSCROLL);
      }
      else
      {
        controlPos.bottom = clientRect->top + infoPtr->tabHeight;
        controlPos.top    = controlPos.bottom - GetSystemMetrics(SM_CYHSCROLL);
      }
    }

    /*
     * If we don't have a scroll control yet, we want to create one.
     * If we have one, we want to make sure it's positioned properly.
     */
    if (infoPtr->hwndUpDown==0)
    {
      infoPtr->hwndUpDown = CreateWindowW(msctls_updown32W, emptyW,
					  WS_VISIBLE | WS_CHILD | UDS_HORZ,
					  controlPos.left, controlPos.top,
					  controlPos.right - controlPos.left,
					  controlPos.bottom - controlPos.top,
					  hwnd, NULL, NULL, NULL);
    }
    else
    {
      SetWindowPos(infoPtr->hwndUpDown,
		   NULL,
		   controlPos.left, controlPos.top,
		   controlPos.right - controlPos.left,
		   controlPos.bottom - controlPos.top,
		   SWP_SHOWWINDOW | SWP_NOZORDER);
    }

    /* Now calculate upper limit of the updown control range.
     * We do this by calculating how many tabs will be offscreen when the
     * last tab is visible.
     */
    if(infoPtr->uNumItem)
    {
       vsize = clientRect->right - (controlPos.right - controlPos.left + 1);
       maxRange = infoPtr->uNumItem;
       tabwidth = TAB_GetItem(infoPtr, infoPtr->uNumItem - 1)->rect.right;

       for(; maxRange > 0; maxRange--)
       {
          if(tabwidth - TAB_GetItem(infoPtr,maxRange - 1)->rect.left > vsize)
             break;
       }

       if(maxRange == infoPtr->uNumItem)
          maxRange--;
    }
  }
  else
  {
    /* If we once had a scroll control... hide it */
    if (infoPtr->hwndUpDown!=0)
      ShowWindow(infoPtr->hwndUpDown, SW_HIDE);
  }
  if (infoPtr->hwndUpDown)
     SendMessageW(infoPtr->hwndUpDown, UDM_SETRANGE32, 0, maxRange);
}

/******************************************************************************
 * TAB_SetItemBounds
 *
 * This method will calculate the position rectangles of all the items in the
 * control. The rectangle calculated starts at 0 for the first item in the
 * list and ignores scrolling and selection.
 * It also uses the current font to determine the height of the tab row and
 * it checks if all the tabs fit in the client area of the window. If they
 * don't, a scrolling control is added.
 */
static void TAB_SetItemBounds (TAB_INFO *infoPtr)
{
  LONG        lStyle = GetWindowLongW(infoPtr->hwnd, GWL_STYLE);
  TEXTMETRICW fontMetrics;
  UINT        curItem;
  INT         curItemLeftPos;
  INT         curItemRowCount;
  HFONT       hFont, hOldFont;
  HDC         hdc;
  RECT        clientRect;
  INT         iTemp;
  RECT*       rcItem;
  INT         iIndex;
  INT         icon_width = 0;

  /*
   * We need to get text information so we need a DC and we need to select
   * a font.
   */
  hdc = GetDC(infoPtr->hwnd);

  hFont = infoPtr->hFont ? infoPtr->hFont : GetStockObject (SYSTEM_FONT);
  hOldFont = SelectObject (hdc, hFont);

  /*
   * We will base the rectangle calculations on the client rectangle
   * of the control.
   */
  GetClientRect(infoPtr->hwnd, &clientRect);

  /* if TCS_VERTICAL then swap the height and width so this code places the
     tabs along the top of the rectangle and we can just rotate them after
     rather than duplicate all of the below code */
  if(lStyle & TCS_VERTICAL)
  {
     iTemp = clientRect.bottom;
     clientRect.bottom = clientRect.right;
     clientRect.right = iTemp;
  }

  /* Now use hPadding and vPadding */
  infoPtr->uHItemPadding = infoPtr->uHItemPadding_s;
  infoPtr->uVItemPadding = infoPtr->uVItemPadding_s;
  
  /* The leftmost item will be "0" aligned */
  curItemLeftPos = 0;
  curItemRowCount = infoPtr->uNumItem ? 1 : 0;

  if (!(infoPtr->fHeightSet))
  {
    int item_height;
    int icon_height = 0;

    /* Use the current font to determine the height of a tab. */
    GetTextMetricsW(hdc, &fontMetrics);

    /* Get the icon height */
    if (infoPtr->himl)
      ImageList_GetIconSize(infoPtr->himl, 0, &icon_height);

    /* Take the highest between font or icon */
    if (fontMetrics.tmHeight > icon_height)
      item_height = fontMetrics.tmHeight + 2;
    else
      item_height = icon_height;

    /*
     * Make sure there is enough space for the letters + icon + growing the
     * selected item + extra space for the selected item.
     */
    infoPtr->tabHeight = item_height + 
	                 ((lStyle & TCS_BUTTONS) ? 2 : 1) *
                          infoPtr->uVItemPadding;

    TRACE("tabH=%d, tmH=%d, iconh=%d\n",
	  infoPtr->tabHeight, fontMetrics.tmHeight, icon_height);
  }

  TRACE("client right=%d\n", clientRect.right);

  /* Get the icon width */
  if (infoPtr->himl)
  {
    ImageList_GetIconSize(infoPtr->himl, &icon_width, 0);

    if (lStyle & TCS_FIXEDWIDTH)
      icon_width += 4;
    else
      /* Add padding if icon is present */
      icon_width += infoPtr->uHItemPadding;
  }

  for (curItem = 0; curItem < infoPtr->uNumItem; curItem++)
  {
    TAB_ITEM *curr = TAB_GetItem(infoPtr, curItem);
	
    /* Set the leftmost position of the tab. */
    curr->rect.left = curItemLeftPos;

    if (lStyle & TCS_FIXEDWIDTH)
    {
      curr->rect.right = curr->rect.left +
        max(infoPtr->tabWidth, icon_width);
    }
    else if (!curr->pszText)
    {
      /* If no text use minimum tab width including padding. */
      if (infoPtr->tabMinWidth < 0)
        curr->rect.right = curr->rect.left + GET_DEFAULT_MIN_TAB_WIDTH(infoPtr);
      else
      {
        curr->rect.right = curr->rect.left + infoPtr->tabMinWidth;

        /* Add extra padding if icon is present */
        if (infoPtr->himl && infoPtr->tabMinWidth > 0 && infoPtr->tabMinWidth < DEFAULT_MIN_TAB_WIDTH
            && infoPtr->uHItemPadding > 1)
          curr->rect.right += EXTRA_ICON_PADDING * (infoPtr->uHItemPadding-1);
      }
    }
    else
    {
      int tabwidth;
      SIZE size;
      /* Calculate how wide the tab is depending on the text it contains */
      GetTextExtentPoint32W(hdc, curr->pszText,
                            lstrlenW(curr->pszText), &size);

      tabwidth = size.cx + icon_width + 2 * infoPtr->uHItemPadding;

      if (infoPtr->tabMinWidth < 0)
        tabwidth = max(tabwidth, GET_DEFAULT_MIN_TAB_WIDTH(infoPtr));
      else
        tabwidth = max(tabwidth, infoPtr->tabMinWidth);

      curr->rect.right = curr->rect.left + tabwidth;
      TRACE("for <%s>, l,r=%d,%d\n",
	  debugstr_w(curr->pszText), curr->rect.left, curr->rect.right);
    }

    /*
     * Check if this is a multiline tab control and if so
     * check to see if we should wrap the tabs
     *
     * Wrap all these tabs. We will arrange them evenly later.
     *
     */

    if (((lStyle & TCS_MULTILINE) || (lStyle & TCS_VERTICAL)) &&
        (curr->rect.right > 
	(clientRect.right - CONTROL_BORDER_SIZEX - DISPLAY_AREA_PADDINGX)))
    {
        curr->rect.right -= curr->rect.left;

	curr->rect.left = 0;
        curItemRowCount++;
	TRACE("wrapping <%s>, l,r=%d,%d\n", debugstr_w(curr->pszText),
	    curr->rect.left, curr->rect.right);
    }

    curr->rect.bottom = 0;
    curr->rect.top = curItemRowCount - 1;

    TRACE("Rect: %s\n", wine_dbgstr_rect(&curr->rect));

    /*
     * The leftmost position of the next item is the rightmost position
     * of this one.
     */
    if (lStyle & TCS_BUTTONS)
    {
      curItemLeftPos = curr->rect.right + BUTTON_SPACINGX;
      if (lStyle & TCS_FLATBUTTONS)
        curItemLeftPos += FLAT_BTN_SPACINGX;
    }
    else
      curItemLeftPos = curr->rect.right;
  }

  if (!((lStyle & TCS_MULTILINE) || (lStyle & TCS_VERTICAL)))
  {
    /*
     * Check if we need a scrolling control.
     */
    infoPtr->needsScrolling = (curItemLeftPos + (2 * SELECTED_TAB_OFFSET) >
                               clientRect.right);

    /* Don't need scrolling, then update infoPtr->leftmostVisible */
    if(!infoPtr->needsScrolling)
      infoPtr->leftmostVisible = 0;
  }
  else
  {
    /*
     * No scrolling in Multiline or Vertical styles.
     */
    infoPtr->needsScrolling = FALSE;
    infoPtr->leftmostVisible = 0;
  }
  TAB_SetupScrolling(infoPtr->hwnd, infoPtr, &clientRect);

  /* Set the number of rows */
  infoPtr->uNumRows = curItemRowCount;

  /* Arrange all tabs evenly if style says so */
   if (!(lStyle & TCS_RAGGEDRIGHT) &&
       ((lStyle & TCS_MULTILINE) || (lStyle & TCS_VERTICAL)) &&
       (infoPtr->uNumItem > 0) &&
       (infoPtr->uNumRows > 1))
   {
      INT tabPerRow,remTab,iRow;
      UINT iItm;
      INT iCount=0;

      /*
       * Ok windows tries to even out the rows. place the same
       * number of tabs in each row. So lets give that a shot
       */

      tabPerRow = infoPtr->uNumItem / (infoPtr->uNumRows);
      remTab = infoPtr->uNumItem % (infoPtr->uNumRows);

      for (iItm=0,iRow=0,iCount=0,curItemLeftPos=0;
           iItm<infoPtr->uNumItem;
           iItm++,iCount++)
      {
          /* normalize the current rect */
          TAB_ITEM *curr = TAB_GetItem(infoPtr, iItm);
 
          /* shift the item to the left side of the clientRect */
          curr->rect.right -= curr->rect.left;
          curr->rect.left = 0;

          TRACE("r=%d, cl=%d, cl.r=%d, iCount=%d, iRow=%d, uNumRows=%d, remTab=%d, tabPerRow=%d\n",
	      curr->rect.right, curItemLeftPos, clientRect.right,
	      iCount, iRow, infoPtr->uNumRows, remTab, tabPerRow);

          /* if we have reached the maximum number of tabs on this row */
          /* move to the next row, reset our current item left position and */
          /* the count of items on this row */

	  if (lStyle & TCS_VERTICAL) {
	      /* Vert: Add the remaining tabs in the *last* remainder rows */
	      if (iCount >= ((iRow>=(INT)infoPtr->uNumRows - remTab)?tabPerRow + 1:tabPerRow)) {
		  iRow++;
		  curItemLeftPos = 0;
		  iCount = 0;
	      }
	  } else {
	      /* Horz: Add the remaining tabs in the *first* remainder rows */
	      if (iCount >= ((iRow<remTab)?tabPerRow + 1:tabPerRow)) {
		  iRow++;
		  curItemLeftPos = 0;
		  iCount = 0;
	      }
	  }

          /* shift the item to the right to place it as the next item in this row */
          curr->rect.left += curItemLeftPos;
          curr->rect.right += curItemLeftPos;
          curr->rect.top = iRow;
          if (lStyle & TCS_BUTTONS)
	  {
            curItemLeftPos = curr->rect.right + 1;
            if (lStyle & TCS_FLATBUTTONS)
	      curItemLeftPos += FLAT_BTN_SPACINGX;
	  }
          else
            curItemLeftPos = curr->rect.right;

          TRACE("arranging <%s>, l,r=%d,%d, row=%d\n",
	      debugstr_w(curr->pszText), curr->rect.left,
	      curr->rect.right, curr->rect.top);
      }

      /*
       * Justify the rows
       */
      {
	INT widthDiff, iIndexStart=0, iIndexEnd=0;
	INT remainder;
	INT iCount=0;

        while(iIndexStart < infoPtr->uNumItem)
        {
          TAB_ITEM *start = TAB_GetItem(infoPtr, iIndexStart);

          /*
           * find the index of the row
           */
          /* find the first item on the next row */
          for (iIndexEnd=iIndexStart;
              (iIndexEnd < infoPtr->uNumItem) &&
 	      (TAB_GetItem(infoPtr, iIndexEnd)->rect.top ==
                start->rect.top) ;
              iIndexEnd++)
          /* intentionally blank */;

          /*
           * we need to justify these tabs so they fill the whole given
           * client area
           *
           */
          /* find the amount of space remaining on this row */
          widthDiff = clientRect.right - (2 * SELECTED_TAB_OFFSET) -
			TAB_GetItem(infoPtr, iIndexEnd - 1)->rect.right;

	  /* iCount is the number of tab items on this row */
	  iCount = iIndexEnd - iIndexStart;

	  if (iCount > 1)
	  {
	    remainder = widthDiff % iCount;
	    widthDiff = widthDiff / iCount;
	    /* add widthDiff/iCount, or extra space/items on row, to each item on this row */
	    for (iIndex=iIndexStart, iCount=0; iIndex < iIndexEnd; iIndex++, iCount++)
	    {
              TAB_ITEM *item = TAB_GetItem(infoPtr, iIndex);

	      item->rect.left += iCount * widthDiff;
	      item->rect.right += (iCount + 1) * widthDiff;

              TRACE("adjusting 1 <%s>, l,r=%d,%d\n",
		  debugstr_w(item->pszText),
		  item->rect.left, item->rect.right);

	    }
	    TAB_GetItem(infoPtr, iIndex - 1)->rect.right += remainder;
	  }
	  else /* we have only one item on this row, make it take up the entire row */
	  {
	    start->rect.left = clientRect.left;
	    start->rect.right = clientRect.right - 4;

            TRACE("adjusting 2 <%s>, l,r=%d,%d\n",
		debugstr_w(start->pszText),
		start->rect.left, start->rect.right);

	  }


	  iIndexStart = iIndexEnd;
	}
      }
  }

  /* if TCS_VERTICAL rotate the tabs so they are along the side of the clientRect */
  if(lStyle & TCS_VERTICAL)
  {
    RECT rcOriginal;
    for(iIndex = 0; iIndex < infoPtr->uNumItem; iIndex++)
    {
      rcItem = &TAB_GetItem(infoPtr, iIndex)->rect;

      rcOriginal = *rcItem;

      /* this is rotating the items by 90 degrees clockwise around the center of the control */
      rcItem->top = (rcOriginal.left - clientRect.left);
      rcItem->bottom = rcItem->top + (rcOriginal.right - rcOriginal.left);
      rcItem->left = rcOriginal.top;
      rcItem->right = rcOriginal.bottom;
    }
  }

  TAB_EnsureSelectionVisible(infoPtr);
  TAB_RecalcHotTrack(infoPtr, NULL, NULL, NULL);

  /* Cleanup */
  SelectObject (hdc, hOldFont);
  ReleaseDC (infoPtr->hwnd, hdc);
}


static void
TAB_EraseTabInterior(const TAB_INFO *infoPtr, HDC hdc, INT iItem, RECT *drawRect)
{
    LONG     lStyle  = GetWindowLongW(infoPtr->hwnd, GWL_STYLE);
    HBRUSH   hbr = CreateSolidBrush (comctl32_color.clrBtnFace);
    BOOL     deleteBrush = TRUE;
    RECT     rTemp = *drawRect;

    InflateRect(&rTemp, -2, -2);
    if (lStyle & TCS_BUTTONS)
    {
	if (iItem == infoPtr->iSelected)
	{
	    /* Background color */
	    if (!(lStyle & TCS_OWNERDRAWFIXED))
	    {
		DeleteObject(hbr);
		hbr = GetSysColorBrush(COLOR_SCROLLBAR);

		SetTextColor(hdc, comctl32_color.clr3dFace);
		SetBkColor(hdc, comctl32_color.clr3dHilight);

		/* if COLOR_WINDOW happens to be the same as COLOR_3DHILIGHT
		* we better use 0x55aa bitmap brush to make scrollbar's background
		* look different from the window background.
		*/
		if (comctl32_color.clr3dHilight == comctl32_color.clrWindow)
		    hbr = COMCTL32_hPattern55AABrush;

		deleteBrush = FALSE;
	    }
	    FillRect(hdc, &rTemp, hbr);
	}
	else  /* ! selected */
	{
	    if (lStyle & TCS_FLATBUTTONS)
	    {
		FillRect(hdc, drawRect, hbr);
		if (iItem == infoPtr->iHotTracked)
		    DrawEdge(hdc, drawRect, EDGE_RAISED, BF_SOFT|BF_RECT);
	    }
	    else
		FillRect(hdc, &rTemp, hbr);
	}

    }
    else /* !TCS_BUTTONS */
    {
        if (!GetWindowTheme (infoPtr->hwnd))
	    FillRect(hdc, &rTemp, hbr);
    }

    /* Cleanup */
    if (deleteBrush) DeleteObject(hbr);
}

/******************************************************************************
 * TAB_DrawItemInterior
 *
 * This method is used to draw the interior (text and icon) of a single tab
 * into the tab control.
 */
static void
TAB_DrawItemInterior(const TAB_INFO *infoPtr, HDC hdc, INT iItem, RECT *drawRect)
{
  LONG      lStyle  = GetWindowLongW(infoPtr->hwnd, GWL_STYLE);

  RECT localRect;

  HPEN   htextPen;
  HPEN   holdPen;
  INT    oldBkMode;
  HFONT  hOldFont;
  
/*  if (drawRect == NULL) */
  {
    BOOL isVisible;
    RECT itemRect;
    RECT selectedRect;

    /*
     * Get the rectangle for the item.
     */
    isVisible = TAB_InternalGetItemRect(infoPtr, iItem, &itemRect, &selectedRect);
    if (!isVisible)
      return;

    /*
     * Make sure drawRect points to something valid; simplifies code.
     */
    drawRect = &localRect;

    /*
     * This logic copied from the part of TAB_DrawItem which draws
     * the tab background.  It's important to keep it in sync.  I
     * would have liked to avoid code duplication, but couldn't figure
     * out how without making spaghetti of TAB_DrawItem.
     */
    if (iItem == infoPtr->iSelected)
      *drawRect = selectedRect;
    else
      *drawRect = itemRect;
        
    if (lStyle & TCS_BUTTONS)
    {
      if (iItem == infoPtr->iSelected)
      {
	drawRect->left   += 4;
	drawRect->top    += 4;
	drawRect->right  -= 4;
	drawRect->bottom -= 1;
      }
      else
      {
	drawRect->left   += 2;
	drawRect->top    += 2;
	drawRect->right  -= 2;
	drawRect->bottom -= 2;
      }
    }
    else
    {
      if ((lStyle & TCS_VERTICAL) && (lStyle & TCS_BOTTOM))
      {
        if (iItem != infoPtr->iSelected)
	{
	  drawRect->left   += 2;
	  drawRect->top    += 2;
	  drawRect->bottom -= 2;
	}
      }
      else if (lStyle & TCS_VERTICAL)
      {
        if (iItem == infoPtr->iSelected)
	{
	  drawRect->right  += 1;
	}
	else
	{
	  drawRect->top    += 2;
	  drawRect->right  -= 2;
	  drawRect->bottom -= 2;
	}
      }
      else if (lStyle & TCS_BOTTOM)
      {
        if (iItem == infoPtr->iSelected)
	{
	  drawRect->top    -= 2;
	}
	else
	{
	  InflateRect(drawRect, -2, -2);
          drawRect->bottom += 2;
	}
      }
      else
      {
        if (iItem == infoPtr->iSelected)
	{
	  drawRect->bottom += 3;
	}
	else
	{
	  drawRect->bottom -= 2;
	  InflateRect(drawRect, -2, 0);
	}
      }
    }
  }
  TRACE("drawRect=(%s)\n", wine_dbgstr_rect(drawRect));

  /* Clear interior */
  TAB_EraseTabInterior (infoPtr, hdc, iItem, drawRect);

  /* Draw the focus rectangle */
  if (!(lStyle & TCS_FOCUSNEVER) &&
      (GetFocus() == infoPtr->hwnd) &&
      (iItem == infoPtr->uFocus) )
  {
    RECT rFocus = *drawRect;
    InflateRect(&rFocus, -3, -3);
    if (lStyle & TCS_BOTTOM && !(lStyle & TCS_VERTICAL))
      rFocus.top -= 3;
    if (lStyle & TCS_BUTTONS)
    {
      rFocus.left -= 3;
      rFocus.top -= 3;
    }

    DrawFocusRect(hdc, &rFocus);
  }

  /*
   * Text pen
   */
  htextPen = CreatePen( PS_SOLID, 1, GetSysColor(COLOR_BTNTEXT) );
  holdPen  = SelectObject(hdc, htextPen);
  hOldFont = SelectObject(hdc, infoPtr->hFont);

  /*
   * Setup for text output
  */
  oldBkMode = SetBkMode(hdc, TRANSPARENT);
  if (!GetWindowTheme (infoPtr->hwnd) || (lStyle & TCS_BUTTONS))
      SetTextColor(hdc, (((lStyle & TCS_HOTTRACK) && (iItem == infoPtr->iHotTracked) 
                          && !(lStyle & TCS_FLATBUTTONS)) 
                        | (TAB_GetItem(infoPtr, iItem)->dwState & TCIS_HIGHLIGHTED)) ?
                        comctl32_color.clrHighlight : comctl32_color.clrBtnText);

  /*
   * if owner draw, tell the owner to draw
   */
  if ((lStyle & TCS_OWNERDRAWFIXED) && GetParent(infoPtr->hwnd))
  {
    DRAWITEMSTRUCT dis;
    UINT id;

    drawRect->top += 2;
    drawRect->right -= 1;
    if ( iItem == infoPtr->iSelected )
    {
        drawRect->right -= 1;
        drawRect->left += 1;
    }

    /*
     * get the control id
     */
    id = (UINT)GetWindowLongPtrW( infoPtr->hwnd, GWLP_ID );

    /*
     * put together the DRAWITEMSTRUCT
     */
    dis.CtlType    = ODT_TAB;
    dis.CtlID      = id;
    dis.itemID     = iItem;
    dis.itemAction = ODA_DRAWENTIRE;
    dis.itemState = 0;
    if ( iItem == infoPtr->iSelected )
      dis.itemState |= ODS_SELECTED;
    if (infoPtr->uFocus == iItem) 
      dis.itemState |= ODS_FOCUS;
    dis.hwndItem = infoPtr->hwnd;
    dis.hDC      = hdc;
    CopyRect(&dis.rcItem,drawRect);
    dis.itemData = (ULONG_PTR)TAB_GetItem(infoPtr, iItem)->extra;

    /*
     * send the draw message
     */
    SendMessageW( infoPtr->hwndNotify, WM_DRAWITEM, (WPARAM)id, (LPARAM)&dis );
  }
  else
  {
    TAB_ITEM *item = TAB_GetItem(infoPtr, iItem);
    RECT rcTemp;
    RECT rcImage;

    /* used to center the icon and text in the tab */
    RECT rcText;
    INT center_offset_h, center_offset_v;

    /* set rcImage to drawRect, we will use top & left in our ImageList_Draw call */
    rcImage = *drawRect;

    rcTemp = *drawRect;

    rcText.left = rcText.top = rcText.right = rcText.bottom = 0;

    /* get the rectangle that the text fits in */
    if (item->pszText)
    {
      DrawTextW(hdc, item->pszText, -1, &rcText, DT_CALCRECT);
    }
    /*
     * If not owner draw, then do the drawing ourselves.
     *
     * Draw the icon.
     */
    if (infoPtr->himl && item->iImage != -1)
    {
      INT cx;
      INT cy;
      
      ImageList_GetIconSize(infoPtr->himl, &cx, &cy);

      if(lStyle & TCS_VERTICAL)
      {
        center_offset_h = ((drawRect->bottom - drawRect->top) - (cy + infoPtr->uHItemPadding + (rcText.right  - rcText.left))) / 2;
        center_offset_v = ((drawRect->right - drawRect->left) - cx) / 2;
      }
      else
      {
        center_offset_h = ((drawRect->right - drawRect->left) - (cx + infoPtr->uHItemPadding + (rcText.right  - rcText.left))) / 2;
        center_offset_v = ((drawRect->bottom - drawRect->top) - cy) / 2;
      }

      /* if an item is selected, the icon is shifted up instead of down */
      if (iItem == infoPtr->iSelected)
        center_offset_v -= infoPtr->uVItemPadding / 2;
      else
        center_offset_v += infoPtr->uVItemPadding / 2;

      if (lStyle & TCS_FIXEDWIDTH && lStyle & (TCS_FORCELABELLEFT | TCS_FORCEICONLEFT))
	center_offset_h = infoPtr->uHItemPadding;

      if (center_offset_h < 2)
        center_offset_h = 2;
	
      if (center_offset_v < 0)
        center_offset_v = 0;
	
      TRACE("for <%s>, c_o_h=%d, c_o_v=%d, draw=(%s), textlen=%d\n",
	  debugstr_w(item->pszText), center_offset_h, center_offset_v,
          wine_dbgstr_rect(drawRect), (rcText.right-rcText.left));

      if((lStyle & TCS_VERTICAL) && (lStyle & TCS_BOTTOM))
      {
        rcImage.top = drawRect->top + center_offset_h;
	/* if tab is TCS_VERTICAL and TCS_BOTTOM, the text is drawn from the */
	/* right side of the tab, but the image still uses the left as its x position */
	/* this keeps the image always drawn off of the same side of the tab */
        rcImage.left = drawRect->right - cx - center_offset_v;
        drawRect->top += cy + infoPtr->uHItemPadding;
      }
      else if(lStyle & TCS_VERTICAL)
      {
        rcImage.top  = drawRect->bottom - cy - center_offset_h;
	rcImage.left = drawRect->left + center_offset_v;
        drawRect->bottom -= cy + infoPtr->uHItemPadding;
      }
      else /* normal style, whether TCS_BOTTOM or not */
      {
        rcImage.left = drawRect->left + center_offset_h;
	rcImage.top = drawRect->top + center_offset_v;
        drawRect->left += cx + infoPtr->uHItemPadding;
      }

      TRACE("drawing image=%d, left=%d, top=%d\n",
	    item->iImage, rcImage.left, rcImage.top-1);
      ImageList_Draw
        (
        infoPtr->himl,
        item->iImage,
        hdc,
        rcImage.left,
        rcImage.top,
        ILD_NORMAL
        );
    }

    /* Now position text */
    if (lStyle & TCS_FIXEDWIDTH && lStyle & TCS_FORCELABELLEFT)
      center_offset_h = infoPtr->uHItemPadding;
    else
      if(lStyle & TCS_VERTICAL)
        center_offset_h = ((drawRect->bottom - drawRect->top) - (rcText.right - rcText.left)) / 2;
      else
        center_offset_h = ((drawRect->right - drawRect->left) - (rcText.right - rcText.left)) / 2;

    if(lStyle & TCS_VERTICAL)
    {
      if(lStyle & TCS_BOTTOM)
        drawRect->top+=center_offset_h;
      else
        drawRect->bottom-=center_offset_h;

      center_offset_v = ((drawRect->right - drawRect->left) - (rcText.bottom - rcText.top)) / 2;
    }
    else
    {
      drawRect->left += center_offset_h;
      center_offset_v = ((drawRect->bottom - drawRect->top) - (rcText.bottom - rcText.top)) / 2;
    }

    /* if an item is selected, the text is shifted up instead of down */
    if (iItem == infoPtr->iSelected)
        center_offset_v -= infoPtr->uVItemPadding / 2;
    else
        center_offset_v += infoPtr->uVItemPadding / 2;

    if (center_offset_v < 0)
      center_offset_v = 0;

    if(lStyle & TCS_VERTICAL)
      drawRect->left += center_offset_v;
    else
      drawRect->top += center_offset_v;

    /* Draw the text */
    if(lStyle & TCS_VERTICAL) /* if we are vertical rotate the text and each character */
    {
      static const WCHAR ArialW[] = { 'A','r','i','a','l',0 };
      LOGFONTW logfont;
      HFONT hFont = 0;
      INT nEscapement = 900;
      INT nOrientation = 900;

      if(lStyle & TCS_BOTTOM)
      {
        nEscapement = -900;
        nOrientation = -900;
      }

      /* to get a font with the escapement and orientation we are looking for, we need to */
      /* call CreateFontIndirectA, which requires us to set the values of the logfont we pass in */
      if (!GetObjectW((infoPtr->hFont) ?
                infoPtr->hFont : GetStockObject(SYSTEM_FONT),
                sizeof(LOGFONTW),&logfont))
      {
        INT iPointSize = 9;

        lstrcpyW(logfont.lfFaceName, ArialW);
        logfont.lfHeight = -MulDiv(iPointSize, GetDeviceCaps(hdc, LOGPIXELSY),
                                    72);
        logfont.lfWeight = FW_NORMAL;
        logfont.lfItalic = 0;
        logfont.lfUnderline = 0;
        logfont.lfStrikeOut = 0;
      }

      logfont.lfEscapement = nEscapement;
      logfont.lfOrientation = nOrientation;
      hFont = CreateFontIndirectW(&logfont);
      SelectObject(hdc, hFont);

      if (item->pszText)
      {
        ExtTextOutW(hdc,
        (lStyle & TCS_BOTTOM) ? drawRect->right : drawRect->left,
        (!(lStyle & TCS_BOTTOM)) ? drawRect->bottom : drawRect->top,
        ETO_CLIPPED,
        drawRect,
        item->pszText,
        lstrlenW(item->pszText),
        0);
      }

      DeleteObject(hFont);
    }
    else
    {
      TRACE("for <%s>, c_o_h=%d, c_o_v=%d, draw=(%s), textlen=%d\n",
	  debugstr_w(item->pszText), center_offset_h, center_offset_v,
          wine_dbgstr_rect(drawRect), (rcText.right-rcText.left));
      if (item->pszText)
      {
        DrawTextW
        (
          hdc,
          item->pszText,
          lstrlenW(item->pszText),
          drawRect,
          DT_LEFT | DT_SINGLELINE
        );
      }
    }

    *drawRect = rcTemp; /* restore drawRect */
  }

  /*
  * Cleanup
  */
  SelectObject(hdc, hOldFont);
  SetBkMode(hdc, oldBkMode);
  SelectObject(hdc, holdPen);
  DeleteObject( htextPen );
}

/******************************************************************************
 * TAB_DrawItem
 *
 * This method is used to draw a single tab into the tab control.
 */
static void TAB_DrawItem(const TAB_INFO *infoPtr, HDC  hdc, INT  iItem)
{
  LONG      lStyle  = GetWindowLongW(infoPtr->hwnd, GWL_STYLE);
  RECT      itemRect;
  RECT      selectedRect;
  BOOL      isVisible;
  RECT      r, fillRect, r1;
  INT       clRight = 0;
  INT       clBottom = 0;
  COLORREF  bkgnd, corner;
  HTHEME    theme;

  /*
   * Get the rectangle for the item.
   */
  isVisible = TAB_InternalGetItemRect(infoPtr,
				      iItem,
				      &itemRect,
				      &selectedRect);

  if (isVisible)
  {
    RECT rUD, rC;

    /* Clip UpDown control to not draw over it */
    if (infoPtr->needsScrolling)
    {
      GetWindowRect(infoPtr->hwnd, &rC);
      GetWindowRect(infoPtr->hwndUpDown, &rUD);
      ExcludeClipRect(hdc, rUD.left - rC.left, rUD.top - rC.top, rUD.right - rC.left, rUD.bottom - rC.top);
    }

    /* If you need to see what the control is doing,
     * then override these variables. They will change what
     * fill colors are used for filling the tabs, and the
     * corners when drawing the edge.
     */
    bkgnd = comctl32_color.clrBtnFace;
    corner = comctl32_color.clrBtnFace;

    if (lStyle & TCS_BUTTONS)
    {
      /* Get item rectangle */
      r = itemRect;

      /* Separators between flat buttons */
      if (lStyle & TCS_FLATBUTTONS)
      {
	r1 = r;
	r1.right += (FLAT_BTN_SPACINGX -2);
	DrawEdge(hdc, &r1, EDGE_ETCHED, BF_RIGHT);
      }

      if (iItem == infoPtr->iSelected)
      {
	DrawEdge(hdc, &r, EDGE_SUNKEN, BF_SOFT|BF_RECT);
	
	OffsetRect(&r, 1, 1);
      }
      else  /* ! selected */
      {
	if (!(lStyle & TCS_FLATBUTTONS))
	  DrawEdge(hdc, &r, EDGE_RAISED, BF_SOFT|BF_RECT);
      }
    }
    else /* !TCS_BUTTONS */
    {
      /* We draw a rectangle of different sizes depending on the selection
       * state. */
      if (iItem == infoPtr->iSelected) {
	RECT rect;
	GetClientRect (infoPtr->hwnd, &rect);
	clRight = rect.right;
	clBottom = rect.bottom;
        r = selectedRect;
      }
      else
        r = itemRect;

      /*
       * Erase the background. (Delay it but setup rectangle.)
       * This is necessary when drawing the selected item since it is larger
       * than the others, it might overlap with stuff already drawn by the
       * other tabs
       */
      fillRect = r;

      /* Draw themed tabs - but only if they are at the top.
       * Windows draws even side or bottom tabs themed, with wacky results.
       * However, since in Wine apps may get themed that did not opt in via
       * a manifest avoid theming when we know the result will be wrong */
      if ((theme = GetWindowTheme (infoPtr->hwnd)) 
          && ((lStyle & (TCS_VERTICAL | TCS_BOTTOM)) == 0))
      {
          static const int partIds[8] = {
              /* Normal item */
              TABP_TABITEM,
              TABP_TABITEMLEFTEDGE,
              TABP_TABITEMRIGHTEDGE,
              TABP_TABITEMBOTHEDGE,
              /* Selected tab */
              TABP_TOPTABITEM,
              TABP_TOPTABITEMLEFTEDGE,
              TABP_TOPTABITEMRIGHTEDGE,
              TABP_TOPTABITEMBOTHEDGE,
          };
          int partIndex = 0;
          int stateId = TIS_NORMAL;

          /* selected and unselected tabs have different parts */
          if (iItem == infoPtr->iSelected)
              partIndex += 4;
          /* The part also differs on the position of a tab on a line.
           * "Visually" determining the position works well enough. */
          if(selectedRect.left == 0)
              partIndex += 1;
          if(selectedRect.right == clRight)
              partIndex += 2;

          if (iItem == infoPtr->iSelected)
              stateId = TIS_SELECTED;
          else if (iItem == infoPtr->iHotTracked)
              stateId = TIS_HOT;
          else if (iItem == infoPtr->uFocus)
              stateId = TIS_FOCUSED;

          /* Adjust rectangle for bottommost row */
          if (TAB_GetItem(infoPtr, iItem)->rect.top == infoPtr->uNumRows-1)
            r.bottom += 3;

          DrawThemeBackground (theme, hdc, partIds[partIndex], stateId, &r, NULL);
          GetThemeBackgroundContentRect (theme, hdc, partIds[partIndex], stateId, &r, &r);
      }
      else if(lStyle & TCS_VERTICAL)
      {
	/* These are for adjusting the drawing of a Selected tab      */
	/* The initial values are for the normal case of non-Selected */
	int ZZ = 1;   /* Do not stretch if selected */
	if (iItem == infoPtr->iSelected) {
	    ZZ = 0;

	    /* if leftmost draw the line longer */
	    if(selectedRect.top == 0)
		fillRect.top += CONTROL_BORDER_SIZEY;
	    /* if rightmost draw the line longer */
	    if(selectedRect.bottom == clBottom)
		fillRect.bottom -= CONTROL_BORDER_SIZEY;
	}

        if (lStyle & TCS_BOTTOM)
        {
	  /* Adjust both rectangles to match native */
	  r.left += (1-ZZ);

          TRACE("<right> item=%d, fill=(%s), edge=(%s)\n",
                iItem, wine_dbgstr_rect(&fillRect), wine_dbgstr_rect(&r));

	  /* Clear interior */
	  SetBkColor(hdc, bkgnd);
	  ExtTextOutW(hdc, 0, 0, 2, &fillRect, NULL, 0, 0);

	  /* Draw rectangular edge around tab */
	  DrawEdge(hdc, &r, EDGE_RAISED, BF_SOFT|BF_RIGHT|BF_TOP|BF_BOTTOM);

	  /* Now erase the top corner and draw diagonal edge */
	  SetBkColor(hdc, corner);
	  r1.left = r.right - ROUND_CORNER_SIZE - 1;
	  r1.top = r.top;
	  r1.right = r.right;
	  r1.bottom = r1.top + ROUND_CORNER_SIZE;
	  ExtTextOutW(hdc, 0, 0, 2, &r1, NULL, 0, 0);
	  r1.right--;
	  DrawEdge(hdc, &r1, EDGE_RAISED, BF_SOFT|BF_DIAGONAL_ENDTOPLEFT);

	  /* Now erase the bottom corner and draw diagonal edge */
	  r1.left = r.right - ROUND_CORNER_SIZE - 1;
	  r1.bottom = r.bottom;
	  r1.right = r.right;
	  r1.top = r1.bottom - ROUND_CORNER_SIZE;
	  ExtTextOutW(hdc, 0, 0, 2, &r1, NULL, 0, 0);
	  r1.right--;
	  DrawEdge(hdc, &r1, EDGE_RAISED, BF_SOFT|BF_DIAGONAL_ENDBOTTOMLEFT);

	  if ((iItem == infoPtr->iSelected) && (selectedRect.top == 0)) {
	      r1 = r;
	      r1.right = r1.left;
	      r1.left--;
	      DrawEdge(hdc, &r1, EDGE_RAISED, BF_SOFT|BF_TOP);
	  }

        }
        else
        {
          TRACE("<left> item=%d, fill=(%s), edge=(%s)\n",
                iItem, wine_dbgstr_rect(&fillRect), wine_dbgstr_rect(&r));

	  /* Clear interior */
	  SetBkColor(hdc, bkgnd);
	  ExtTextOutW(hdc, 0, 0, 2, &fillRect, NULL, 0, 0);

	  /* Draw rectangular edge around tab */
	  DrawEdge(hdc, &r, EDGE_RAISED, BF_SOFT|BF_LEFT|BF_TOP|BF_BOTTOM);

	  /* Now erase the top corner and draw diagonal edge */
	  SetBkColor(hdc, corner);
	  r1.left = r.left;
	  r1.top = r.top;
	  r1.right = r1.left + ROUND_CORNER_SIZE + 1;
	  r1.bottom = r1.top + ROUND_CORNER_SIZE;
	  ExtTextOutW(hdc, 0, 0, 2, &r1, NULL, 0, 0);
	  r1.left++;
	  DrawEdge(hdc, &r1, EDGE_RAISED, BF_SOFT|BF_DIAGONAL_ENDTOPRIGHT);

	  /* Now erase the bottom corner and draw diagonal edge */
	  r1.left = r.left;
	  r1.bottom = r.bottom;
	  r1.right = r1.left + ROUND_CORNER_SIZE + 1;
	  r1.top = r1.bottom - ROUND_CORNER_SIZE;
	  ExtTextOutW(hdc, 0, 0, 2, &r1, NULL, 0, 0);
	  r1.left++;
	  DrawEdge(hdc, &r1, EDGE_SUNKEN, BF_DIAGONAL_ENDTOPLEFT);
        }
      }
      else  /* ! TCS_VERTICAL */
      {
	/* These are for adjusting the drawing of a Selected tab      */
	/* The initial values are for the normal case of non-Selected */
	if (iItem == infoPtr->iSelected) {
	    /* if leftmost draw the line longer */
	    if(selectedRect.left == 0)
		fillRect.left += CONTROL_BORDER_SIZEX;
	    /* if rightmost draw the line longer */
	    if(selectedRect.right == clRight)
		fillRect.right -= CONTROL_BORDER_SIZEX;
	}

        if (lStyle & TCS_BOTTOM)
        {
	  /* Adjust both rectangles for topmost row */
	  if (TAB_GetItem(infoPtr, iItem)->rect.top == infoPtr->uNumRows-1)
	  {
	    fillRect.top -= 2;
	    r.top -= 1;
	  }

          TRACE("<bottom> item=%d, fill=(%s), edge=(%s)\n",
                iItem, wine_dbgstr_rect(&fillRect), wine_dbgstr_rect(&r));

	  /* Clear interior */
	  SetBkColor(hdc, bkgnd);
	  ExtTextOutW(hdc, 0, 0, 2, &fillRect, NULL, 0, 0);

	  /* Draw rectangular edge around tab */
	  DrawEdge(hdc, &r, EDGE_RAISED, BF_SOFT|BF_LEFT|BF_BOTTOM|BF_RIGHT);

	  /* Now erase the righthand corner and draw diagonal edge */
	  SetBkColor(hdc, corner);
	  r1.left = r.right - ROUND_CORNER_SIZE;
	  r1.bottom = r.bottom;
	  r1.right = r.right;
	  r1.top = r1.bottom - ROUND_CORNER_SIZE - 1;
	  ExtTextOutW(hdc, 0, 0, 2, &r1, NULL, 0, 0);
	  r1.bottom--;
	  DrawEdge(hdc, &r1, EDGE_RAISED, BF_SOFT|BF_DIAGONAL_ENDBOTTOMLEFT);

	  /* Now erase the lefthand corner and draw diagonal edge */
	  r1.left = r.left;
	  r1.bottom = r.bottom;
	  r1.right = r1.left + ROUND_CORNER_SIZE;
	  r1.top = r1.bottom - ROUND_CORNER_SIZE - 1;
	  ExtTextOutW(hdc, 0, 0, 2, &r1, NULL, 0, 0);
	  r1.bottom--;
	  DrawEdge(hdc, &r1, EDGE_RAISED, BF_SOFT|BF_DIAGONAL_ENDTOPLEFT);

	  if (iItem == infoPtr->iSelected)
	  {
	    r.top += 2;
	    r.left += 1;
	    if (selectedRect.left == 0)
	    {
	      r1 = r;
	      r1.bottom = r1.top;
	      r1.top--;
	      DrawEdge(hdc, &r1, EDGE_RAISED, BF_SOFT|BF_LEFT);
	    }
	  }

        }
        else
        {
	  /* Adjust both rectangles for bottommost row */
	  if (TAB_GetItem(infoPtr, iItem)->rect.top == infoPtr->uNumRows-1)
	  {
	    fillRect.bottom += 3;
	    r.bottom += 2;
	  }

          TRACE("<top> item=%d, fill=(%s), edge=(%s)\n",
                iItem, wine_dbgstr_rect(&fillRect), wine_dbgstr_rect(&r));

	  /* Clear interior */
	  SetBkColor(hdc, bkgnd);
	  ExtTextOutW(hdc, 0, 0, 2, &fillRect, NULL, 0, 0);

	  /* Draw rectangular edge around tab */
	  DrawEdge(hdc, &r, EDGE_RAISED, BF_SOFT|BF_LEFT|BF_TOP|BF_RIGHT);

	  /* Now erase the righthand corner and draw diagonal edge */
	  SetBkColor(hdc, corner);
	  r1.left = r.right - ROUND_CORNER_SIZE;
	  r1.top = r.top;
	  r1.right = r.right;
	  r1.bottom = r1.top + ROUND_CORNER_SIZE + 1;
	  ExtTextOutW(hdc, 0, 0, 2, &r1, NULL, 0, 0);
	  r1.top++;
	  DrawEdge(hdc, &r1, EDGE_RAISED, BF_SOFT|BF_DIAGONAL_ENDBOTTOMRIGHT);

	  /* Now erase the lefthand corner and draw diagonal edge */
	  r1.left = r.left;
	  r1.top = r.top;
	  r1.right = r1.left + ROUND_CORNER_SIZE;
	  r1.bottom = r1.top + ROUND_CORNER_SIZE + 1;
	  ExtTextOutW(hdc, 0, 0, 2, &r1, NULL, 0, 0);
	  r1.top++;
	  DrawEdge(hdc, &r1, EDGE_RAISED, BF_SOFT|BF_DIAGONAL_ENDTOPRIGHT);
        }
      }
    }

    TAB_DumpItemInternal(infoPtr, iItem);

    /* This modifies r to be the text rectangle. */
    TAB_DrawItemInterior(infoPtr, hdc, iItem, &r);
  }
}

/******************************************************************************
 * TAB_DrawBorder
 *
 * This method is used to draw the raised border around the tab control
 * "content" area.
 */
static void TAB_DrawBorder(const TAB_INFO *infoPtr, HDC hdc)
{
  RECT rect;
  DWORD lStyle = GetWindowLongW(infoPtr->hwnd, GWL_STYLE);
  HTHEME theme = GetWindowTheme (infoPtr->hwnd);

  GetClientRect (infoPtr->hwnd, &rect);

  /*
   * Adjust for the style
   */

  if (infoPtr->uNumItem)
  {
    if ((lStyle & TCS_BOTTOM) && !(lStyle & TCS_VERTICAL))
      rect.bottom -= infoPtr->tabHeight * infoPtr->uNumRows + CONTROL_BORDER_SIZEX;
    else if((lStyle & TCS_BOTTOM) && (lStyle & TCS_VERTICAL))
      rect.right  -= infoPtr->tabHeight * infoPtr->uNumRows + CONTROL_BORDER_SIZEX;
    else if(lStyle & TCS_VERTICAL)
      rect.left   += infoPtr->tabHeight * infoPtr->uNumRows + CONTROL_BORDER_SIZEX;
    else /* not TCS_VERTICAL and not TCS_BOTTOM */
      rect.top    += infoPtr->tabHeight * infoPtr->uNumRows + CONTROL_BORDER_SIZEX;
  }

  TRACE("border=(%s)\n", wine_dbgstr_rect(&rect));

  if (theme)
      DrawThemeBackground (theme, hdc, TABP_PANE, 0, &rect, NULL);
  else
      DrawEdge(hdc, &rect, EDGE_RAISED, BF_SOFT|BF_RECT);
}

/******************************************************************************
 * TAB_Refresh
 *
 * This method repaints the tab control..
 */
static void TAB_Refresh (TAB_INFO *infoPtr, HDC hdc)
{
  HFONT hOldFont;
  INT i;

  if (!infoPtr->DoRedraw)
    return;

  hOldFont = SelectObject (hdc, infoPtr->hFont);

  if (GetWindowLongW(infoPtr->hwnd, GWL_STYLE) & TCS_BUTTONS)
  {
    for (i = 0; i < infoPtr->uNumItem; i++)
      TAB_DrawItem (infoPtr, hdc, i);
  }
  else
  {
    /* Draw all the non selected item first */
    for (i = 0; i < infoPtr->uNumItem; i++)
    {
      if (i != infoPtr->iSelected)
	TAB_DrawItem (infoPtr, hdc, i);
    }

    /* Now, draw the border, draw it before the selected item
     * since the selected item overwrites part of the border. */
    TAB_DrawBorder (infoPtr, hdc);

    /* Then, draw the selected item */
    TAB_DrawItem (infoPtr, hdc, infoPtr->iSelected);
  }

  SelectObject (hdc, hOldFont);
}

static inline DWORD TAB_GetRowCount (const TAB_INFO *infoPtr)
{
  return infoPtr->uNumRows;
}

static inline LRESULT TAB_SetRedraw (TAB_INFO *infoPtr, BOOL doRedraw)
{
  infoPtr->DoRedraw = doRedraw;
  return 0;
}

/******************************************************************************
 * TAB_EnsureSelectionVisible
 *
 * This method will make sure that the current selection is completely
 * visible by scrolling until it is.
 */
static void TAB_EnsureSelectionVisible(
  TAB_INFO* infoPtr)
{
  INT iSelected = infoPtr->iSelected;
  LONG lStyle = GetWindowLongW(infoPtr->hwnd, GWL_STYLE);
  INT iOrigLeftmostVisible = infoPtr->leftmostVisible;

  /* set the items row to the bottommost row or topmost row depending on
   * style */
  if ((infoPtr->uNumRows > 1) && !(lStyle & TCS_BUTTONS))
  {
      TAB_ITEM *selected = TAB_GetItem(infoPtr, iSelected);
      INT newselected;
      INT iTargetRow;

      if(lStyle & TCS_VERTICAL)
        newselected = selected->rect.left;
      else
        newselected = selected->rect.top;

      /* the target row is always (number of rows - 1)
         as row 0 is furthest from the clientRect */
      iTargetRow = infoPtr->uNumRows - 1;

      if (newselected != iTargetRow)
      {
         UINT i;
         if(lStyle & TCS_VERTICAL)
         {
           for (i=0; i < infoPtr->uNumItem; i++)
           {
             /* move everything in the row of the selected item to the iTargetRow */
             TAB_ITEM *item = TAB_GetItem(infoPtr, i);

             if (item->rect.left == newselected )
                 item->rect.left = iTargetRow;
             else
             {
               if (item->rect.left > newselected)
                 item->rect.left-=1;
             }
           }
         }
         else
         {
           for (i=0; i < infoPtr->uNumItem; i++)
           {
             TAB_ITEM *item = TAB_GetItem(infoPtr, i);

             if (item->rect.top == newselected )
                 item->rect.top = iTargetRow;
             else
             {
               if (item->rect.top > newselected)
                 item->rect.top-=1;
             }
          }
        }
        TAB_RecalcHotTrack(infoPtr, NULL, NULL, NULL);
      }
  }

  /*
   * Do the trivial cases first.
   */
  if ( (!infoPtr->needsScrolling) ||
       (infoPtr->hwndUpDown==0) || (lStyle & TCS_VERTICAL))
    return;

  if (infoPtr->leftmostVisible >= iSelected)
  {
    infoPtr->leftmostVisible = iSelected;
  }
  else
  {
     TAB_ITEM *selected = TAB_GetItem(infoPtr, iSelected);
     RECT r;
     INT width;
     UINT i;

     /* Calculate the part of the client area that is visible */
     GetClientRect(infoPtr->hwnd, &r);
     width = r.right;

     GetClientRect(infoPtr->hwndUpDown, &r);
     width -= r.right;

     if ((selected->rect.right -
          selected->rect.left) >= width )
     {
        /* Special case: width of selected item is greater than visible
         * part of control.
         */
        infoPtr->leftmostVisible = iSelected;
     }
     else
     {
        for (i = infoPtr->leftmostVisible; i < infoPtr->uNumItem; i++)
        {
           if ((selected->rect.right - TAB_GetItem(infoPtr, i)->rect.left) < width)
              break;
        }
        infoPtr->leftmostVisible = i;
     }
  }

  if (infoPtr->leftmostVisible != iOrigLeftmostVisible)
    TAB_RecalcHotTrack(infoPtr, NULL, NULL, NULL);

  SendMessageW(infoPtr->hwndUpDown, UDM_SETPOS, 0,
               MAKELONG(infoPtr->leftmostVisible, 0));
}

/******************************************************************************
 * TAB_InvalidateTabArea
 *
 * This method will invalidate the portion of the control that contains the
 * tabs. It is called when the state of the control changes and needs
 * to be redisplayed
 */
static void TAB_InvalidateTabArea(const TAB_INFO *infoPtr)
{
  RECT clientRect, rInvalidate, rAdjClient;
  DWORD lStyle = GetWindowLongW(infoPtr->hwnd, GWL_STYLE);
  INT lastRow = infoPtr->uNumRows - 1;
  RECT rect;

  if (lastRow < 0) return;

  GetClientRect(infoPtr->hwnd, &clientRect);
  rInvalidate = clientRect;
  rAdjClient = clientRect;

  TAB_AdjustRect(infoPtr, 0, &rAdjClient);

  TAB_InternalGetItemRect(infoPtr, infoPtr->uNumItem-1 , &rect, NULL);
  if ((lStyle & TCS_BOTTOM) && (lStyle & TCS_VERTICAL))
  {
    rInvalidate.left = rAdjClient.right;
    if (infoPtr->uNumRows == 1)
      rInvalidate.bottom = clientRect.top + rect.bottom + 2 * SELECTED_TAB_OFFSET;
  }
  else if(lStyle & TCS_VERTICAL)
  {
    rInvalidate.right = rAdjClient.left;
    if (infoPtr->uNumRows == 1)
      rInvalidate.bottom = clientRect.top + rect.bottom + 2 * SELECTED_TAB_OFFSET;
  }
  else if (lStyle & TCS_BOTTOM)
  {
    rInvalidate.top = rAdjClient.bottom;
    if (infoPtr->uNumRows == 1)
      rInvalidate.right = clientRect.left + rect.right + 2 * SELECTED_TAB_OFFSET;
  }
  else 
  {
    rInvalidate.bottom = rAdjClient.top;
    if (infoPtr->uNumRows == 1)
      rInvalidate.right = clientRect.left + rect.right + 2 * SELECTED_TAB_OFFSET;
  }
  
  /* Punch out the updown control */
  if (infoPtr->needsScrolling && (rInvalidate.right > 0)) {
    RECT r;
    GetClientRect(infoPtr->hwndUpDown, &r);
    if (rInvalidate.right > clientRect.right - r.left)
      rInvalidate.right = rInvalidate.right - (r.right - r.left);
    else
      rInvalidate.right = clientRect.right - r.left;
  }

  TRACE("invalidate (%s)\n", wine_dbgstr_rect(&rInvalidate));

  InvalidateRect(infoPtr->hwnd, &rInvalidate, TRUE);
}

static inline LRESULT TAB_Paint (TAB_INFO *infoPtr, HDC hdcPaint)
{
  HDC hdc;
  PAINTSTRUCT ps;

  if (hdcPaint)
    hdc = hdcPaint;
  else
  {
    hdc = BeginPaint (infoPtr->hwnd, &ps);
    TRACE("erase %d, rect=(%s)\n", ps.fErase, wine_dbgstr_rect(&ps.rcPaint));
  }

  TAB_Refresh (infoPtr, hdc);

  if (!hdcPaint)
    EndPaint (infoPtr->hwnd, &ps);

  return 0;
}

static LRESULT
TAB_InsertItemT (TAB_INFO *infoPtr, WPARAM wParam, LPARAM lParam, BOOL bUnicode)
{
  TAB_ITEM *item;
  TCITEMW *pti;
  INT iItem;
  RECT rect;

  GetClientRect (infoPtr->hwnd, &rect);
  TRACE("Rect: %p %s\n", infoPtr->hwnd, wine_dbgstr_rect(&rect));

  pti = (TCITEMW *)lParam;
  iItem = (INT)wParam;

  if (iItem < 0) return -1;
  if (iItem > infoPtr->uNumItem)
    iItem = infoPtr->uNumItem;

  TAB_DumpItemExternalT(pti, iItem, bUnicode);


  if (infoPtr->uNumItem == 0) {
    infoPtr->items = Alloc (TAB_ITEM_SIZE(infoPtr));
    infoPtr->uNumItem++;
    infoPtr->iSelected = 0;
  }
  else {
    LPBYTE oldItems = (LPBYTE)infoPtr->items;

    infoPtr->uNumItem++;
    infoPtr->items = Alloc (TAB_ITEM_SIZE(infoPtr) * infoPtr->uNumItem);

    /* pre insert copy */
    if (iItem > 0) {
      memcpy (infoPtr->items, oldItems,
              iItem * TAB_ITEM_SIZE(infoPtr));
    }

    /* post insert copy */
    if (iItem < infoPtr->uNumItem - 1) {
      memcpy (TAB_GetItem(infoPtr, iItem + 1),
              oldItems + iItem * TAB_ITEM_SIZE(infoPtr),
              (infoPtr->uNumItem - iItem - 1) * TAB_ITEM_SIZE(infoPtr));

    }

    if (iItem <= infoPtr->iSelected)
      infoPtr->iSelected++;

    Free (oldItems);
  }

  item = TAB_GetItem(infoPtr, iItem);

  item->pszText = NULL;

  if (pti->mask & TCIF_TEXT)
  {
    if (bUnicode)
      Str_SetPtrW (&item->pszText, pti->pszText);
    else
      Str_SetPtrAtoW (&item->pszText, (LPSTR)pti->pszText);
  }

  if (pti->mask & TCIF_IMAGE)
    item->iImage = pti->iImage;
  else
    item->iImage = -1;

  if (pti->mask & TCIF_PARAM)
    memcpy(item->extra, &pti->lParam, infoPtr->cbInfo);
  else
    memset(item->extra, 0, infoPtr->cbInfo);
  
  TAB_SetItemBounds(infoPtr);
  if (infoPtr->uNumItem > 1)
    TAB_InvalidateTabArea(infoPtr);
  else
    InvalidateRect(infoPtr->hwnd, NULL, TRUE);

  TRACE("[%p]: added item %d %s\n",
        infoPtr->hwnd, iItem, debugstr_w(item->pszText));

  /* If we haven't set the current focus yet, set it now. */
  if (infoPtr->uFocus == -1)
    TAB_SetCurFocus(infoPtr, iItem);

  return iItem;
}

static LRESULT
TAB_SetItemSize (TAB_INFO *infoPtr, LPARAM lParam)
{
  LONG lStyle = GetWindowLongW(infoPtr->hwnd, GWL_STYLE);
  LONG lResult = 0;
  BOOL bNeedPaint = FALSE;

  lResult = MAKELONG(infoPtr->tabWidth, infoPtr->tabHeight);

  /* UNDOCUMENTED: If requested Width or Height is 0 this means that program wants to use auto size. */
  if (lStyle & TCS_FIXEDWIDTH && (infoPtr->tabWidth != (INT)LOWORD(lParam)))
  {
    infoPtr->tabWidth = (INT)LOWORD(lParam);
    bNeedPaint = TRUE;
  }

  if (infoPtr->tabHeight != (INT)HIWORD(lParam))
  {
    if ((infoPtr->fHeightSet = ((INT)HIWORD(lParam) != 0)))
      infoPtr->tabHeight = (INT)HIWORD(lParam);

    bNeedPaint = TRUE;
  }
  TRACE("was h=%d,w=%d, now h=%d,w=%d\n",
       HIWORD(lResult), LOWORD(lResult),
       infoPtr->tabHeight, infoPtr->tabWidth);

  if (bNeedPaint)
  {
    TAB_SetItemBounds(infoPtr);
    RedrawWindow(infoPtr->hwnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_UPDATENOW);
  }

  return lResult;
}

static inline LRESULT TAB_SetMinTabWidth (TAB_INFO *infoPtr, INT cx)
{
  INT oldcx = 0;

  TRACE("(%p,%d)\n", infoPtr, cx);

  if (infoPtr->tabMinWidth < 0)
    oldcx = DEFAULT_MIN_TAB_WIDTH;
  else
    oldcx = infoPtr->tabMinWidth;
  infoPtr->tabMinWidth = cx;
  TAB_SetItemBounds(infoPtr);
  return oldcx;
}

static inline LRESULT 
TAB_HighlightItem (TAB_INFO *infoPtr, INT iItem, BOOL fHighlight)
{
  LPDWORD lpState;

  TRACE("(%p,%d,%s)\n", infoPtr, iItem, fHighlight ? "true" : "false");

  if (!infoPtr || iItem < 0 || iItem >= infoPtr->uNumItem)
    return FALSE;
  
  lpState = &TAB_GetItem(infoPtr, iItem)->dwState;

  if (fHighlight)
    *lpState |= TCIS_HIGHLIGHTED;
  else
    *lpState &= ~TCIS_HIGHLIGHTED;

  return TRUE;
}

static LRESULT
TAB_SetItemT (TAB_INFO *infoPtr, INT iItem, LPTCITEMW tabItem, BOOL bUnicode)
{
  TAB_ITEM *wineItem;

  TRACE("(%p,%d,%p,%s)\n", infoPtr, iItem, tabItem, bUnicode ? "true" : "false");

  if (iItem < 0 || iItem >= infoPtr->uNumItem)
    return FALSE;

  TAB_DumpItemExternalT(tabItem, iItem, bUnicode);

  wineItem = TAB_GetItem(infoPtr, iItem);

  if (tabItem->mask & TCIF_IMAGE)
    wineItem->iImage = tabItem->iImage;

  if (tabItem->mask & TCIF_PARAM)
    memcpy(wineItem->extra, &tabItem->lParam, infoPtr->cbInfo);

  if (tabItem->mask & TCIF_RTLREADING)
    FIXME("TCIF_RTLREADING\n");

  if (tabItem->mask & TCIF_STATE)
    wineItem->dwState = tabItem->dwState;

  if (tabItem->mask & TCIF_TEXT)
  {
    Free(wineItem->pszText);
    wineItem->pszText = NULL;
    if (bUnicode)
      Str_SetPtrW(&wineItem->pszText, tabItem->pszText);
    else
      Str_SetPtrAtoW(&wineItem->pszText, (LPSTR)tabItem->pszText);
  }

  /* Update and repaint tabs */
  TAB_SetItemBounds(infoPtr);
  TAB_InvalidateTabArea(infoPtr);

  return TRUE;
}

static inline LRESULT TAB_GetItemCount (const TAB_INFO *infoPtr)
{
   return infoPtr->uNumItem;
}


static LRESULT
TAB_GetItemT (TAB_INFO *infoPtr, INT iItem, LPTCITEMW tabItem, BOOL bUnicode)
{
  TAB_ITEM *wineItem;

  TRACE("(%p,%d,%p,%s)\n", infoPtr, iItem, tabItem, bUnicode ? "true" : "false");

  if (iItem < 0 || iItem >= infoPtr->uNumItem)
    return FALSE;

  wineItem = TAB_GetItem(infoPtr, iItem);

  if (tabItem->mask & TCIF_IMAGE)
    tabItem->iImage = wineItem->iImage;

  if (tabItem->mask & TCIF_PARAM)
    memcpy(&tabItem->lParam, wineItem->extra, infoPtr->cbInfo);

  if (tabItem->mask & TCIF_RTLREADING)
    FIXME("TCIF_RTLREADING\n");

  if (tabItem->mask & TCIF_STATE)
    tabItem->dwState = wineItem->dwState;

  if (tabItem->mask & TCIF_TEXT)
  {
    if (bUnicode)
      Str_GetPtrW (wineItem->pszText, tabItem->pszText, tabItem->cchTextMax);
    else
      Str_GetPtrWtoA (wineItem->pszText, (LPSTR)tabItem->pszText, tabItem->cchTextMax);
  }

  TAB_DumpItemExternalT(tabItem, iItem, bUnicode);

  return TRUE;
}


static LRESULT TAB_DeleteItem (TAB_INFO *infoPtr, INT iItem)
{
    BOOL bResult = FALSE;

    TRACE("(%p, %d)\n", infoPtr, iItem);

    if ((iItem >= 0) && (iItem < infoPtr->uNumItem))
    {
        TAB_ITEM *item = TAB_GetItem(infoPtr, iItem);
        LPBYTE oldItems = (LPBYTE)infoPtr->items;

	TAB_InvalidateTabArea(infoPtr);
        Free(item->pszText);
	infoPtr->uNumItem--;

	if (!infoPtr->uNumItem)
        {
            infoPtr->items = NULL;
            if (infoPtr->iHotTracked >= 0)
            {
                KillTimer(infoPtr->hwnd, TAB_HOTTRACK_TIMER);
                infoPtr->iHotTracked = -1;
            }
        }
        else
	{
	    infoPtr->items = Alloc(TAB_ITEM_SIZE(infoPtr) * infoPtr->uNumItem);

	    if (iItem > 0)
	        memcpy(infoPtr->items, oldItems, iItem * TAB_ITEM_SIZE(infoPtr));

	    if (iItem < infoPtr->uNumItem)
	        memcpy(TAB_GetItem(infoPtr, iItem),
                       oldItems + (iItem + 1) * TAB_ITEM_SIZE(infoPtr),
		       (infoPtr->uNumItem - iItem) * TAB_ITEM_SIZE(infoPtr));

            if (iItem <= infoPtr->iHotTracked)
            {
                /* When tabs move left/up, the hot track item may change */
                FIXME("Recalc hot track\n");
            }
	}
	Free(oldItems);

	/* Readjust the selected index */
	if ((iItem == infoPtr->iSelected) && (iItem > 0))
	    infoPtr->iSelected--;

	if (iItem < infoPtr->iSelected)
	    infoPtr->iSelected--;

	if (infoPtr->uNumItem == 0)
	    infoPtr->iSelected = -1;

	/* Reposition and repaint tabs */
	TAB_SetItemBounds(infoPtr);

	bResult = TRUE;
    }

    return bResult;
}

static inline LRESULT TAB_DeleteAllItems (TAB_INFO *infoPtr)
{
    TRACE("(%p)\n", infoPtr);
    while (infoPtr->uNumItem)
      TAB_DeleteItem (infoPtr, 0);
    return TRUE;
}


static inline LRESULT TAB_GetFont (const TAB_INFO *infoPtr)
{
  TRACE("(%p) returning %p\n", infoPtr, infoPtr->hFont);
  return (LRESULT)infoPtr->hFont;
}

static inline LRESULT TAB_SetFont (TAB_INFO *infoPtr, HFONT hNewFont)
{
  TRACE("(%p,%p)\n", infoPtr, hNewFont);

  infoPtr->hFont = hNewFont;

  TAB_SetItemBounds(infoPtr);

  TAB_InvalidateTabArea(infoPtr);

  return 0;
}


static inline LRESULT TAB_GetImageList (const TAB_INFO *infoPtr)
{
  TRACE("\n");
  return (LRESULT)infoPtr->himl;
}

static inline LRESULT TAB_SetImageList (TAB_INFO *infoPtr, HIMAGELIST himlNew)
{
    HIMAGELIST himlPrev = infoPtr->himl;
    TRACE("\n");
    infoPtr->himl = himlNew;
    TAB_SetItemBounds(infoPtr);
    InvalidateRect(infoPtr->hwnd, NULL, TRUE);
    return (LRESULT)himlPrev;
}

static inline LRESULT TAB_GetUnicodeFormat (const TAB_INFO *infoPtr)
{
    return infoPtr->bUnicode;
}

static inline LRESULT TAB_SetUnicodeFormat (TAB_INFO *infoPtr, BOOL bUnicode)
{
    BOOL bTemp = infoPtr->bUnicode;

    infoPtr->bUnicode = bUnicode;

    return bTemp;
}

static inline LRESULT TAB_Size (TAB_INFO *infoPtr)
{
/* I'm not really sure what the following code was meant to do.
   This is what it is doing:
   When WM_SIZE is sent with SIZE_RESTORED, the control
   gets positioned in the top left corner.

  RECT parent_rect;
  HWND parent;
  UINT uPosFlags,cx,cy;

  uPosFlags=0;
  if (!wParam) {
    parent = GetParent (hwnd);
    GetClientRect(parent, &parent_rect);
    cx=LOWORD (lParam);
    cy=HIWORD (lParam);
    if (GetWindowLongW(hwnd, GWL_STYLE) & CCS_NORESIZE)
        uPosFlags |= (SWP_NOSIZE | SWP_NOMOVE);

    SetWindowPos (hwnd, 0, parent_rect.left, parent_rect.top,
            cx, cy, uPosFlags | SWP_NOZORDER);
  } else {
    FIXME("WM_SIZE flag %x %lx not handled\n", wParam, lParam);
  } */

  /* Recompute the size/position of the tabs. */
  TAB_SetItemBounds (infoPtr);

  /* Force a repaint of the control. */
  InvalidateRect(infoPtr->hwnd, NULL, TRUE);

  return 0;
}


static LRESULT TAB_Create (HWND hwnd, LPARAM lParam)
{
  TAB_INFO *infoPtr;
  TEXTMETRICW fontMetrics;
  HDC hdc;
  HFONT hOldFont;
  DWORD dwStyle;

  infoPtr = Alloc (sizeof(TAB_INFO));

  SetWindowLongPtrW(hwnd, 0, (DWORD_PTR)infoPtr);

  infoPtr->hwnd            = hwnd;
  infoPtr->hwndNotify      = ((LPCREATESTRUCTW)lParam)->hwndParent;
  infoPtr->uNumItem        = 0;
  infoPtr->uNumRows        = 0;
  infoPtr->uHItemPadding   = 6;
  infoPtr->uVItemPadding   = 3;
  infoPtr->uHItemPadding_s = 6;
  infoPtr->uVItemPadding_s = 3;
  infoPtr->hFont           = 0;
  infoPtr->items           = 0;
  infoPtr->hcurArrow       = LoadCursorW (0, (LPWSTR)IDC_ARROW);
  infoPtr->iSelected       = -1;
  infoPtr->iHotTracked     = -1;
  infoPtr->uFocus          = -1;
  infoPtr->hwndToolTip     = 0;
  infoPtr->DoRedraw        = TRUE;
  infoPtr->needsScrolling  = FALSE;
  infoPtr->hwndUpDown      = 0;
  infoPtr->leftmostVisible = 0;
  infoPtr->fHeightSet      = FALSE;
  infoPtr->bUnicode        = IsWindowUnicode (hwnd);
  infoPtr->cbInfo          = sizeof(LPARAM);

  TRACE("Created tab control, hwnd [%p]\n", hwnd);

  /* The tab control always has the WS_CLIPSIBLINGS style. Even
     if you don't specify it in CreateWindow. This is necessary in
     order for paint to work correctly. This follows windows behaviour. */
  dwStyle = GetWindowLongW(hwnd, GWL_STYLE);
  SetWindowLongW(hwnd, GWL_STYLE, dwStyle|WS_CLIPSIBLINGS);

  if (dwStyle & TCS_TOOLTIPS) {
    /* Create tooltip control */
    infoPtr->hwndToolTip =
      CreateWindowExW (0, TOOLTIPS_CLASSW, NULL, WS_POPUP,
		       CW_USEDEFAULT, CW_USEDEFAULT,
		       CW_USEDEFAULT, CW_USEDEFAULT,
		       hwnd, 0, 0, 0);

    /* Send NM_TOOLTIPSCREATED notification */
    if (infoPtr->hwndToolTip) {
      NMTOOLTIPSCREATED nmttc;

      nmttc.hdr.hwndFrom = hwnd;
      nmttc.hdr.idFrom = GetWindowLongPtrW(hwnd, GWLP_ID);
      nmttc.hdr.code = NM_TOOLTIPSCREATED;
      nmttc.hwndToolTips = infoPtr->hwndToolTip;

      SendMessageW (infoPtr->hwndNotify, WM_NOTIFY,
		    (WPARAM)GetWindowLongPtrW(hwnd, GWLP_ID), (LPARAM)&nmttc);
    }
  }

  OpenThemeData (infoPtr->hwnd, themeClass);
  
  /*
   * We need to get text information so we need a DC and we need to select
   * a font.
   */
  hdc = GetDC(hwnd);
  hOldFont = SelectObject (hdc, GetStockObject (SYSTEM_FONT));

  /* Use the system font to determine the initial height of a tab. */
  GetTextMetricsW(hdc, &fontMetrics);

  /*
   * Make sure there is enough space for the letters + growing the
   * selected item + extra space for the selected item.
   */
  infoPtr->tabHeight = fontMetrics.tmHeight + SELECTED_TAB_OFFSET +
	               ((dwStyle & TCS_BUTTONS) ? 2 : 1) *
                        infoPtr->uVItemPadding;

  /* Initialize the width of a tab. */
  if (dwStyle & TCS_FIXEDWIDTH)
    infoPtr->tabWidth = GetDeviceCaps(hdc, LOGPIXELSX);

  infoPtr->tabMinWidth = -1;

  TRACE("tabH=%d, tabW=%d\n", infoPtr->tabHeight, infoPtr->tabWidth);

  SelectObject (hdc, hOldFont);
  ReleaseDC(hwnd, hdc);

  return 0;
}

static LRESULT
TAB_Destroy (TAB_INFO *infoPtr)
{
  UINT iItem;

  if (!infoPtr)
      return 0;

  SetWindowLongPtrW(infoPtr->hwnd, 0, 0);

  if (infoPtr->items) {
    for (iItem = 0; iItem < infoPtr->uNumItem; iItem++) {
      Free (TAB_GetItem(infoPtr, iItem)->pszText);
    }
    Free (infoPtr->items);
  }

  if (infoPtr->hwndToolTip)
    DestroyWindow (infoPtr->hwndToolTip);

  if (infoPtr->hwndUpDown)
    DestroyWindow(infoPtr->hwndUpDown);

  if (infoPtr->iHotTracked >= 0)
    KillTimer(infoPtr->hwnd, TAB_HOTTRACK_TIMER);

  CloseThemeData (GetWindowTheme (infoPtr->hwnd));
  
  Free (infoPtr);
  return 0;
}

/* update theme after a WM_THEMECHANGED message */
static LRESULT theme_changed(const TAB_INFO *infoPtr)
{
    HTHEME theme = GetWindowTheme (infoPtr->hwnd);
    CloseThemeData (theme);
    OpenThemeData (infoPtr->hwnd, themeClass);
    return 0;
}

static LRESULT TAB_NCCalcSize(WPARAM wParam)
{
  if (!wParam)
    return 0;
  return WVR_ALIGNTOP;
}

static inline LRESULT
TAB_SetItemExtra (TAB_INFO *infoPtr, INT cbInfo)
{
  if (!infoPtr || cbInfo <= 0)
    return FALSE;

  if (infoPtr->uNumItem)
  {
    /* FIXME: MSDN says this is not allowed, but this hasn't been verified */
    return FALSE;
  }
    
  infoPtr->cbInfo = cbInfo;
  return TRUE;
}

static LRESULT WINAPI
TAB_WindowProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TAB_INFO *infoPtr = TAB_GetInfoPtr(hwnd);

    TRACE("hwnd=%p msg=%x wParam=%lx lParam=%lx\n", hwnd, uMsg, wParam, lParam);
    if (!infoPtr && (uMsg != WM_CREATE))
      return DefWindowProcW (hwnd, uMsg, wParam, lParam);

    switch (uMsg)
    {
    case TCM_GETIMAGELIST:
      return TAB_GetImageList (infoPtr);

    case TCM_SETIMAGELIST:
      return TAB_SetImageList (infoPtr, (HIMAGELIST)lParam);

    case TCM_GETITEMCOUNT:
      return TAB_GetItemCount (infoPtr);

    case TCM_GETITEMA:
    case TCM_GETITEMW:
      return TAB_GetItemT (infoPtr, (INT)wParam, (LPTCITEMW)lParam, uMsg == TCM_GETITEMW);

    case TCM_SETITEMA:
    case TCM_SETITEMW:
      return TAB_SetItemT (infoPtr, (INT)wParam, (LPTCITEMW)lParam, uMsg == TCM_SETITEMW);

    case TCM_DELETEITEM:
      return TAB_DeleteItem (infoPtr, (INT)wParam);

    case TCM_DELETEALLITEMS:
     return TAB_DeleteAllItems (infoPtr);

    case TCM_GETITEMRECT:
     return TAB_GetItemRect (infoPtr, wParam, lParam);

    case TCM_GETCURSEL:
      return TAB_GetCurSel (infoPtr);

    case TCM_HITTEST:
      return TAB_HitTest (infoPtr, (LPTCHITTESTINFO)lParam);

    case TCM_SETCURSEL:
      return TAB_SetCurSel (infoPtr, (INT)wParam);

    case TCM_INSERTITEMA:
    case TCM_INSERTITEMW:
      return TAB_InsertItemT (infoPtr, wParam, lParam, uMsg == TCM_INSERTITEMW);

    case TCM_SETITEMEXTRA:
      return TAB_SetItemExtra (infoPtr, (int)wParam);

    case TCM_ADJUSTRECT:
      return TAB_AdjustRect (infoPtr, (BOOL)wParam, (LPRECT)lParam);

    case TCM_SETITEMSIZE:
      return TAB_SetItemSize (infoPtr, lParam);

    case TCM_REMOVEIMAGE:
      FIXME("Unimplemented msg TCM_REMOVEIMAGE\n");
      return 0;

    case TCM_SETPADDING:
      return TAB_SetPadding (infoPtr, lParam);

    case TCM_GETROWCOUNT:
      return TAB_GetRowCount(infoPtr);

    case TCM_GETUNICODEFORMAT:
      return TAB_GetUnicodeFormat (infoPtr);

    case TCM_SETUNICODEFORMAT:
      return TAB_SetUnicodeFormat (infoPtr, (BOOL)wParam);

    case TCM_HIGHLIGHTITEM:
      return TAB_HighlightItem (infoPtr, (INT)wParam, (BOOL)LOWORD(lParam));

    case TCM_GETTOOLTIPS:
      return TAB_GetToolTips (infoPtr);

    case TCM_SETTOOLTIPS:
      return TAB_SetToolTips (infoPtr, (HWND)wParam);

    case TCM_GETCURFOCUS:
      return TAB_GetCurFocus (infoPtr);

    case TCM_SETCURFOCUS:
      return TAB_SetCurFocus (infoPtr, (INT)wParam);

    case TCM_SETMINTABWIDTH:
      return TAB_SetMinTabWidth(infoPtr, (INT)lParam);

    case TCM_DESELECTALL:
      FIXME("Unimplemented msg TCM_DESELECTALL\n");
      return 0;

    case TCM_GETEXTENDEDSTYLE:
      FIXME("Unimplemented msg TCM_GETEXTENDEDSTYLE\n");
      return 0;

    case TCM_SETEXTENDEDSTYLE:
      FIXME("Unimplemented msg TCM_SETEXTENDEDSTYLE\n");
      return 0;

    case WM_GETFONT:
      return TAB_GetFont (infoPtr);

    case WM_SETFONT:
      return TAB_SetFont (infoPtr, (HFONT)wParam);

    case WM_CREATE:
      return TAB_Create (hwnd, lParam);

    case WM_NCDESTROY:
      return TAB_Destroy (infoPtr);

    case WM_GETDLGCODE:
      return DLGC_WANTARROWS | DLGC_WANTCHARS;

    case WM_LBUTTONDOWN:
      return TAB_LButtonDown (infoPtr, wParam, lParam);

    case WM_LBUTTONUP:
      return TAB_LButtonUp (infoPtr);

    case WM_NOTIFY:
      return SendMessageW(infoPtr->hwndNotify, WM_NOTIFY, wParam, lParam);

    case WM_RBUTTONDOWN:
      return TAB_RButtonDown (infoPtr);

    case WM_MOUSEMOVE:
      return TAB_MouseMove (infoPtr, wParam, lParam);

    case WM_PRINTCLIENT:
    case WM_PAINT:
      return TAB_Paint (infoPtr, (HDC)wParam);

    case WM_SIZE:
      return TAB_Size (infoPtr);

    case WM_SETREDRAW:
      return TAB_SetRedraw (infoPtr, (BOOL)wParam);

    case WM_HSCROLL:
      return TAB_OnHScroll(infoPtr, (int)LOWORD(wParam), (int)HIWORD(wParam));

    case WM_STYLECHANGED:
      TAB_SetItemBounds (infoPtr);
      InvalidateRect(hwnd, NULL, TRUE);
      return 0;

    case WM_SYSCOLORCHANGE:
      COMCTL32_RefreshSysColors();
      return 0;

    case WM_THEMECHANGED:
      return theme_changed (infoPtr);

    case WM_KILLFOCUS:
    case WM_SETFOCUS:
      TAB_FocusChanging(infoPtr);
      break;   /* Don't disturb normal focus behavior */

    case WM_KEYUP:
      return TAB_KeyUp(infoPtr, wParam);
    case WM_NCHITTEST:
      return TAB_NCHitTest(infoPtr, lParam);

    case WM_NCCALCSIZE:
      return TAB_NCCalcSize(wParam);

    default:
      if (uMsg >= WM_USER && uMsg < WM_APP && !COMCTL32_IsReflectedMessage(uMsg))
	WARN("unknown msg %04x wp=%08lx lp=%08lx\n",
	     uMsg, wParam, lParam);
      break;
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}


void
TAB_Register (void)
{
  WNDCLASSW wndClass;

  ZeroMemory (&wndClass, sizeof(WNDCLASSW));
  wndClass.style         = CS_GLOBALCLASS | CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
  wndClass.lpfnWndProc   = TAB_WindowProc;
  wndClass.cbClsExtra    = 0;
  wndClass.cbWndExtra    = sizeof(TAB_INFO *);
  wndClass.hCursor       = LoadCursorW (0, (LPWSTR)IDC_ARROW);
  wndClass.hbrBackground = (HBRUSH)(COLOR_BTNFACE+1);
  wndClass.lpszClassName = WC_TABCONTROLW;

  RegisterClassW (&wndClass);
}


void
TAB_Unregister (void)
{
    UnregisterClassW (WC_TABCONTROLW, NULL);
}
