/*
 * Listbox controls
 *
 * Copyright 1996 Alexandre Julliard
 * Copyright 2005 Frank Richter
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
 */

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "commctrl.h"
#include "uxtheme.h"
#include "vssym32.h"
#include "wine/exception.h"
#include "wine/debug.h"
#include "wine/heap.h"

#include "comctl32.h"

WINE_DEFAULT_DEBUG_CHANNEL(listbox);

/* Items array granularity (must be power of 2) */
#define LB_ARRAY_GRANULARITY 16

/* Scrolling timeout in ms */
#define LB_SCROLL_TIMEOUT 50

/* Listbox system timer id */
#define LB_TIMER_ID  2

/* flag listbox changed while setredraw false - internal style */
#define LBS_DISPLAYCHANGED 0x80000000

/* Item structure */
typedef struct
{
    LPWSTR    str;       /* Item text */
    BOOL      selected;  /* Is item selected? */
    UINT      height;    /* Item height (only for OWNERDRAWVARIABLE) */
    ULONG_PTR data;      /* User data */
} LB_ITEMDATA;

/* Listbox structure */
typedef struct
{
    HWND        self;           /* Our own window handle */
    HWND        owner;          /* Owner window to send notifications to */
    UINT        style;          /* Window style */
    INT         width;          /* Window width */
    INT         height;         /* Window height */
    union
    {
        LB_ITEMDATA *items;     /* Array of items */
        BYTE *nodata_items;     /* For multi-selection LBS_NODATA */
    } u;
    INT         nb_items;       /* Number of items */
    UINT        items_size;     /* Total number of allocated items in the array */
    INT         top_item;       /* Top visible item */
    INT         selected_item;  /* Selected item */
    INT         focus_item;     /* Item that has the focus */
    INT         anchor_item;    /* Anchor item for extended selection */
    INT         item_height;    /* Default item height */
    INT         page_size;      /* Items per listbox page */
    INT         column_width;   /* Column width for multi-column listboxes */
    INT         horz_extent;    /* Horizontal extent */
    INT         horz_pos;       /* Horizontal position */
    INT         nb_tabs;        /* Number of tabs in array */
    INT        *tabs;           /* Array of tabs */
    INT         avg_char_width; /* Average width of characters */
    INT         wheel_remain;   /* Left over scroll amount */
    BOOL        caret_on;       /* Is caret on? */
    BOOL        captured;       /* Is mouse captured? */
    BOOL        in_focus;
    HFONT       font;           /* Current font */
    LCID        locale;         /* Current locale for string comparisons */
    HEADCOMBO  *lphc;           /* ComboLBox */
} LB_DESCR;


#define IS_OWNERDRAW(descr) \
    ((descr)->style & (LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE))

#define HAS_STRINGS(descr) \
    (!IS_OWNERDRAW(descr) || ((descr)->style & LBS_HASSTRINGS))


#define IS_MULTISELECT(descr) \
    ((descr)->style & (LBS_MULTIPLESEL|LBS_EXTENDEDSEL) && \
     !((descr)->style & LBS_NOSEL))

#define SEND_NOTIFICATION(descr,code) \
    (SendMessageW( (descr)->owner, WM_COMMAND, \
     MAKEWPARAM( GetWindowLongPtrW((descr->self),GWLP_ID), (code)), (LPARAM)(descr->self) ))

#define ISWIN31 (LOWORD(GetVersion()) == 0x0a03)

/* Current timer status */
typedef enum
{
    LB_TIMER_NONE,
    LB_TIMER_UP,
    LB_TIMER_LEFT,
    LB_TIMER_DOWN,
    LB_TIMER_RIGHT
} TIMER_DIRECTION;

static TIMER_DIRECTION LISTBOX_Timer = LB_TIMER_NONE;

static LRESULT LISTBOX_GetItemRect( const LB_DESCR *descr, INT index, RECT *rect );

/*
   For listboxes without LBS_NODATA, an array of LB_ITEMDATA is allocated
   to store the states of each item into descr->u.items.

   For single-selection LBS_NODATA listboxes, no storage is allocated,
   and thus descr->u.nodata_items will always be NULL.

   For multi-selection LBS_NODATA listboxes, one byte per item is stored
   for the item's selection state into descr->u.nodata_items.
*/
static size_t get_sizeof_item( const LB_DESCR *descr )
{
    return (descr->style & LBS_NODATA) ? sizeof(BYTE) : sizeof(LB_ITEMDATA);
}

static BOOL resize_storage(LB_DESCR *descr, UINT items_size)
{
    LB_ITEMDATA *items;

    if (items_size > descr->items_size ||
        items_size + LB_ARRAY_GRANULARITY * 2 < descr->items_size)
    {
        items_size = (items_size + LB_ARRAY_GRANULARITY - 1) & ~(LB_ARRAY_GRANULARITY - 1);
        if ((descr->style & (LBS_NODATA | LBS_MULTIPLESEL | LBS_EXTENDEDSEL)) != LBS_NODATA)
        {
            items = heap_realloc(descr->u.items, items_size * get_sizeof_item(descr));
            if (!items)
            {
                SEND_NOTIFICATION(descr, LBN_ERRSPACE);
                return FALSE;
            }
            descr->u.items = items;
        }
        descr->items_size = items_size;
    }

    if ((descr->style & LBS_NODATA) && descr->u.nodata_items && items_size > descr->nb_items)
    {
        memset(descr->u.nodata_items + descr->nb_items, 0,
               (items_size - descr->nb_items) * get_sizeof_item(descr));
    }
    return TRUE;
}

static ULONG_PTR get_item_data( const LB_DESCR *descr, UINT index )
{
    return (descr->style & LBS_NODATA) ? 0 : descr->u.items[index].data;
}

static void set_item_data( LB_DESCR *descr, UINT index, ULONG_PTR data )
{
    if (!(descr->style & LBS_NODATA)) descr->u.items[index].data = data;
}

static WCHAR *get_item_string( const LB_DESCR *descr, UINT index )
{
    return HAS_STRINGS(descr) ? descr->u.items[index].str : NULL;
}

static void set_item_string( const LB_DESCR *descr, UINT index, WCHAR *string )
{
    if (!(descr->style & LBS_NODATA)) descr->u.items[index].str = string;
}

static UINT get_item_height( const LB_DESCR *descr, UINT index )
{
    return (descr->style & LBS_NODATA) ? 0 : descr->u.items[index].height;
}

static void set_item_height( LB_DESCR *descr, UINT index, UINT height )
{
    if (!(descr->style & LBS_NODATA)) descr->u.items[index].height = height;
}

static BOOL is_item_selected( const LB_DESCR *descr, UINT index )
{
    if (!(descr->style & (LBS_MULTIPLESEL | LBS_EXTENDEDSEL)))
        return index == descr->selected_item;
    if (descr->style & LBS_NODATA)
        return descr->u.nodata_items[index];
    else
        return descr->u.items[index].selected;
}

static void set_item_selected_state(LB_DESCR *descr, UINT index, BOOL state)
{
    if (descr->style & (LBS_MULTIPLESEL | LBS_EXTENDEDSEL))
    {
        if (descr->style & LBS_NODATA)
            descr->u.nodata_items[index] = state;
        else
            descr->u.items[index].selected = state;
    }
}

static void insert_item_data(LB_DESCR *descr, UINT index)
{
    size_t size = get_sizeof_item(descr);
    BYTE *p = descr->u.nodata_items + index * size;

    if (!descr->u.items) return;

    if (index < descr->nb_items)
        memmove(p + size, p, (descr->nb_items - index) * size);
}

static void remove_item_data(LB_DESCR *descr, UINT index)
{
    size_t size = get_sizeof_item(descr);
    BYTE *p = descr->u.nodata_items + index * size;

    if (!descr->u.items) return;

    if (index < descr->nb_items)
        memmove(p, p + size, (descr->nb_items - index) * size);
}

/***********************************************************************
 *           LISTBOX_GetCurrentPageSize
 *
 * Return the current page size
 */
static INT LISTBOX_GetCurrentPageSize( const LB_DESCR *descr )
{
    INT i, height;
    if (!(descr->style & LBS_OWNERDRAWVARIABLE)) return descr->page_size;
    for (i = descr->top_item, height = 0; i < descr->nb_items; i++)
    {
        if ((height += get_item_height(descr, i)) > descr->height) break;
    }
    if (i == descr->top_item) return 1;
    else return i - descr->top_item;
}


/***********************************************************************
 *           LISTBOX_GetMaxTopIndex
 *
 * Return the maximum possible index for the top of the listbox.
 */
static INT LISTBOX_GetMaxTopIndex( const LB_DESCR *descr )
{
    INT max, page;

    if (descr->style & LBS_OWNERDRAWVARIABLE)
    {
        page = descr->height;
        for (max = descr->nb_items - 1; max >= 0; max--)
            if ((page -= get_item_height(descr, max)) < 0) break;
        if (max < descr->nb_items - 1) max++;
    }
    else if (descr->style & LBS_MULTICOLUMN)
    {
        if ((page = descr->width / descr->column_width) < 1) page = 1;
        max = (descr->nb_items + descr->page_size - 1) / descr->page_size;
        max = (max - page) * descr->page_size;
    }
    else
    {
        max = descr->nb_items - descr->page_size;
    }
    if (max < 0) max = 0;
    return max;
}


/***********************************************************************
 *           LISTBOX_UpdateScroll
 *
 * Update the scrollbars. Should be called whenever the content
 * of the listbox changes.
 */
static void LISTBOX_UpdateScroll( LB_DESCR *descr )
{
    SCROLLINFO info;

    /* Check the listbox scroll bar flags individually before we call
       SetScrollInfo otherwise when the listbox style is WS_HSCROLL and
       no WS_VSCROLL, we end up with an uninitialized, visible horizontal
       scroll bar when we do not need one.
    if (!(descr->style & WS_VSCROLL)) return;
    */

    /*   It is important that we check descr->style, and not wnd->dwStyle,
       for WS_VSCROLL, as the former is exactly the one passed in
       argument to CreateWindow.
         In Windows (and from now on in Wine :) a listbox created
       with such a style (no WS_SCROLL) does not update
       the scrollbar with listbox-related data, thus letting
       the programmer use it for his/her own purposes. */

    if (descr->style & LBS_NOREDRAW) return;
    info.cbSize = sizeof(info);

    if (descr->style & LBS_MULTICOLUMN)
    {
        info.nMin  = 0;
        info.nMax  = (descr->nb_items - 1) / descr->page_size;
        info.nPos  = descr->top_item / descr->page_size;
        info.nPage = descr->width / descr->column_width;
        if (info.nPage < 1) info.nPage = 1;
        info.fMask = SIF_RANGE | SIF_POS | SIF_PAGE;
        if (descr->style & LBS_DISABLENOSCROLL)
            info.fMask |= SIF_DISABLENOSCROLL;
        if (descr->style & WS_HSCROLL)
            SetScrollInfo( descr->self, SB_HORZ, &info, TRUE );
        info.nMax = 0;
        info.fMask = SIF_RANGE;
        if (descr->style & WS_VSCROLL)
            SetScrollInfo( descr->self, SB_VERT, &info, TRUE );
    }
    else
    {
        info.nMin  = 0;
        info.nMax  = descr->nb_items - 1;
        info.nPos  = descr->top_item;
        info.nPage = LISTBOX_GetCurrentPageSize( descr );
        info.fMask = SIF_RANGE | SIF_POS | SIF_PAGE;
        if (descr->style & LBS_DISABLENOSCROLL)
            info.fMask |= SIF_DISABLENOSCROLL;
        if (descr->style & WS_VSCROLL)
            SetScrollInfo( descr->self, SB_VERT, &info, TRUE );

        if ((descr->style & WS_HSCROLL) && descr->horz_extent)
        {
            info.nPos  = descr->horz_pos;
            info.nPage = descr->width;
            info.fMask = SIF_POS | SIF_PAGE;
            if (descr->style & LBS_DISABLENOSCROLL)
                info.fMask |= SIF_DISABLENOSCROLL;
            SetScrollInfo( descr->self, SB_HORZ, &info, TRUE );
        }
        else
        {
            if (descr->style & LBS_DISABLENOSCROLL)
            {
                info.nMin  = 0;
                info.nMax  = 0;
                info.fMask = SIF_RANGE | SIF_DISABLENOSCROLL;
                SetScrollInfo( descr->self, SB_HORZ, &info, TRUE );
            }
            else
            {
                ShowScrollBar( descr->self, SB_HORZ, FALSE );
            }
        }
    }
}


/***********************************************************************
 *           LISTBOX_SetTopItem
 *
 * Set the top item of the listbox, scrolling up or down if necessary.
 */
static LRESULT LISTBOX_SetTopItem( LB_DESCR *descr, INT index, BOOL scroll )
{
    INT max = LISTBOX_GetMaxTopIndex( descr );

    TRACE("setting top item %d, scroll %d\n", index, scroll);

    if (index > max) index = max;
    if (index < 0) index = 0;
    if (descr->style & LBS_MULTICOLUMN) index -= index % descr->page_size;
    if (descr->top_item == index) return LB_OKAY;
    if (scroll)
    {
        INT dx = 0, dy = 0;
        if (descr->style & LBS_MULTICOLUMN)
            dx = (descr->top_item - index) / descr->page_size * descr->column_width;
        else if (descr->style & LBS_OWNERDRAWVARIABLE)
        {
            INT i;
            if (index > descr->top_item)
            {
                for (i = index - 1; i >= descr->top_item; i--)
                    dy -= get_item_height(descr, i);
            }
            else
            {
                for (i = index; i < descr->top_item; i++)
                    dy += get_item_height(descr, i);
            }
        }
        else
            dy = (descr->top_item - index) * descr->item_height;

        ScrollWindowEx( descr->self, dx, dy, NULL, NULL, 0, NULL,
                        SW_INVALIDATE | SW_ERASE | SW_SCROLLCHILDREN );
    }
    else
        InvalidateRect( descr->self, NULL, TRUE );
    descr->top_item = index;
    LISTBOX_UpdateScroll( descr );
    return LB_OKAY;
}


/***********************************************************************
 *           LISTBOX_UpdatePage
 *
 * Update the page size. Should be called when the size of
 * the client area or the item height changes.
 */
static void LISTBOX_UpdatePage( LB_DESCR *descr )
{
    INT page_size;

    if ((descr->item_height == 0) || (page_size = descr->height / descr->item_height) < 1)
                       page_size = 1;
    if (page_size == descr->page_size) return;
    descr->page_size = page_size;
    if (descr->style & LBS_MULTICOLUMN)
        InvalidateRect( descr->self, NULL, TRUE );
    LISTBOX_SetTopItem( descr, descr->top_item, FALSE );
}


/***********************************************************************
 *           LISTBOX_UpdateSize
 *
 * Update the size of the listbox. Should be called when the size of
 * the client area changes.
 */
static void LISTBOX_UpdateSize( LB_DESCR *descr )
{
    RECT rect;

    GetClientRect( descr->self, &rect );
    descr->width  = rect.right - rect.left;
    descr->height = rect.bottom - rect.top;
    if (!(descr->style & LBS_NOINTEGRALHEIGHT) && !(descr->style & LBS_OWNERDRAWVARIABLE))
    {
        INT remaining;
        RECT rect;

        GetWindowRect( descr->self, &rect );
        if(descr->item_height != 0)
            remaining = descr->height % descr->item_height;
        else
            remaining = 0;
        if ((descr->height > descr->item_height) && remaining)
        {
            TRACE("[%p]: changing height %d -> %d\n",
                  descr->self, descr->height, descr->height - remaining );
            SetWindowPos( descr->self, 0, 0, 0, rect.right - rect.left,
                          rect.bottom - rect.top - remaining,
                          SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE );
            return;
        }
    }
    TRACE("[%p]: new size = %d,%d\n", descr->self, descr->width, descr->height );
    LISTBOX_UpdatePage( descr );
    LISTBOX_UpdateScroll( descr );

    /* Invalidate the focused item so it will be repainted correctly */
    if (LISTBOX_GetItemRect( descr, descr->focus_item, &rect ) == 1)
    {
        InvalidateRect( descr->self, &rect, FALSE );
    }
}


/***********************************************************************
 *           LISTBOX_GetItemRect
 *
 * Get the rectangle enclosing an item, in listbox client coordinates.
 * Return 1 if the rectangle is (partially) visible, 0 if hidden, -1 on error.
 */
static LRESULT LISTBOX_GetItemRect( const LB_DESCR *descr, INT index, RECT *rect )
{
    /* Index <= 0 is legal even on empty listboxes */
    if (index && (index >= descr->nb_items))
    {
        SetRectEmpty(rect);
        SetLastError(ERROR_INVALID_INDEX);
        return LB_ERR;
    }
    SetRect( rect, 0, 0, descr->width, descr->height );
    if (descr->style & LBS_MULTICOLUMN)
    {
        INT col = (index / descr->page_size) -
                        (descr->top_item / descr->page_size);
        rect->left += col * descr->column_width;
        rect->right = rect->left + descr->column_width;
        rect->top += (index % descr->page_size) * descr->item_height;
        rect->bottom = rect->top + descr->item_height;
    }
    else if (descr->style & LBS_OWNERDRAWVARIABLE)
    {
        INT i;
        rect->right += descr->horz_pos;
        if ((index >= 0) && (index < descr->nb_items))
        {
            if (index < descr->top_item)
            {
                for (i = descr->top_item-1; i >= index; i--)
                    rect->top -= get_item_height(descr, i);
            }
            else
            {
                for (i = descr->top_item; i < index; i++)
                    rect->top += get_item_height(descr, i);
            }
            rect->bottom = rect->top + get_item_height(descr, index);

        }
    }
    else
    {
        rect->top += (index - descr->top_item) * descr->item_height;
        rect->bottom = rect->top + descr->item_height;
        rect->right += descr->horz_pos;
    }

    TRACE("item %d, rect %s\n", index, wine_dbgstr_rect(rect));

    return ((rect->left < descr->width) && (rect->right > 0) &&
            (rect->top < descr->height) && (rect->bottom > 0));
}


/***********************************************************************
 *           LISTBOX_GetItemFromPoint
 *
 * Return the item nearest from point (x,y) (in client coordinates).
 */
static INT LISTBOX_GetItemFromPoint( const LB_DESCR *descr, INT x, INT y )
{
    INT index = descr->top_item;

    if (!descr->nb_items) return -1;  /* No items */
    if (descr->style & LBS_OWNERDRAWVARIABLE)
    {
        INT pos = 0;
        if (y >= 0)
        {
            while (index < descr->nb_items)
            {
                if ((pos += get_item_height(descr, index)) > y) break;
                index++;
            }
        }
        else
        {
            while (index > 0)
            {
                index--;
                if ((pos -= get_item_height(descr, index)) <= y) break;
            }
        }
    }
    else if (descr->style & LBS_MULTICOLUMN)
    {
        if (y >= descr->item_height * descr->page_size) return -1;
        if (y >= 0) index += y / descr->item_height;
        if (x >= 0) index += (x / descr->column_width) * descr->page_size;
        else index -= (((x + 1) / descr->column_width) - 1) * descr->page_size;
    }
    else
    {
        index += (y / descr->item_height);
    }
    if (index < 0) return 0;
    if (index >= descr->nb_items) return -1;
    return index;
}


/***********************************************************************
 *           LISTBOX_PaintItem
 *
 * Paint an item.
 */
static void LISTBOX_PaintItem( LB_DESCR *descr, HDC hdc, const RECT *rect,
			       INT index, UINT action, BOOL ignoreFocus )
{
    BOOL selected = FALSE, focused;
    WCHAR *item_str = NULL;

    if (index < descr->nb_items)
    {
        item_str = get_item_string(descr, index);
        selected = is_item_selected(descr, index);
    }

    focused = !ignoreFocus && descr->focus_item == index && descr->caret_on && descr->in_focus;

    if (IS_OWNERDRAW(descr))
    {
        DRAWITEMSTRUCT dis;
        RECT r;
        HRGN hrgn;

	if (index >= descr->nb_items)
	{
	    if (action == ODA_FOCUS)
		DrawFocusRect( hdc, rect );
	    else
	        ERR("called with an out of bounds index %d(%d) in owner draw, Not good.\n",index,descr->nb_items);
	    return;
	}

        /* some programs mess with the clipping region when
        drawing the item, *and* restore the previous region
        after they are done, so a region has better to exist
        else everything ends clipped */
        GetClientRect(descr->self, &r);
        hrgn = set_control_clipping( hdc, &r );

        dis.CtlType      = ODT_LISTBOX;
        dis.CtlID        = GetWindowLongPtrW( descr->self, GWLP_ID );
        dis.hwndItem     = descr->self;
        dis.itemAction   = action;
        dis.hDC          = hdc;
        dis.itemID       = index;
        dis.itemState    = 0;
        if (selected)
            dis.itemState |= ODS_SELECTED;
        if (focused)
            dis.itemState |= ODS_FOCUS;
        if (!IsWindowEnabled(descr->self)) dis.itemState |= ODS_DISABLED;
        dis.itemData     = get_item_data(descr, index);
        dis.rcItem       = *rect;
        TRACE("[%p]: drawitem %d (%s) action=%02x state=%02x rect=%s\n",
              descr->self, index, debugstr_w(item_str), action,
              dis.itemState, wine_dbgstr_rect(rect) );
        SendMessageW(descr->owner, WM_DRAWITEM, dis.CtlID, (LPARAM)&dis);
        SelectClipRgn( hdc, hrgn );
        if (hrgn) DeleteObject( hrgn );
    }
    else
    {
        COLORREF oldText = 0, oldBk = 0;

        if (action == ODA_FOCUS)
        {
            DrawFocusRect( hdc, rect );
            return;
        }
        if (selected)
        {
            oldBk = SetBkColor( hdc, GetSysColor( COLOR_HIGHLIGHT ) );
            oldText = SetTextColor( hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
        }

        TRACE("[%p]: painting %d (%s) action=%02x rect=%s\n",
              descr->self, index, debugstr_w(item_str), action,
              wine_dbgstr_rect(rect) );
        if (!item_str)
            ExtTextOutW( hdc, rect->left + 1, rect->top,
                           ETO_OPAQUE | ETO_CLIPPED, rect, NULL, 0, NULL );
        else if (!(descr->style & LBS_USETABSTOPS))
            ExtTextOutW( hdc, rect->left + 1, rect->top,
                         ETO_OPAQUE | ETO_CLIPPED, rect, item_str,
                         lstrlenW(item_str), NULL );
        else
	{
	    /* Output empty string to paint background in the full width. */
            ExtTextOutW( hdc, rect->left + 1, rect->top,
                         ETO_OPAQUE | ETO_CLIPPED, rect, NULL, 0, NULL );
            TabbedTextOutW( hdc, rect->left + 1 , rect->top,
                            item_str, lstrlenW(item_str),
                            descr->nb_tabs, descr->tabs, 0);
	}
        if (selected)
        {
            SetBkColor( hdc, oldBk );
            SetTextColor( hdc, oldText );
        }
        if (focused)
            DrawFocusRect( hdc, rect );
    }
}


/***********************************************************************
 *           LISTBOX_SetRedraw
 *
 * Change the redraw flag.
 */
static void LISTBOX_SetRedraw( LB_DESCR *descr, BOOL on )
{
    if (on)
    {
        if (!(descr->style & LBS_NOREDRAW)) return;
        descr->style &= ~LBS_NOREDRAW;
        if (descr->style & LBS_DISPLAYCHANGED)
        {     /* page was changed while setredraw false, refresh automatically */
            InvalidateRect(descr->self, NULL, TRUE);
            if ((descr->top_item + descr->page_size) > descr->nb_items)
            {      /* reset top of page if less than number of items/page */
                descr->top_item = descr->nb_items - descr->page_size;
                if (descr->top_item < 0) descr->top_item = 0;
            }
            descr->style &= ~LBS_DISPLAYCHANGED;
        }
        LISTBOX_UpdateScroll( descr );
    }
    else descr->style |= LBS_NOREDRAW;
}


/***********************************************************************
 *           LISTBOX_RepaintItem
 *
 * Repaint a single item synchronously.
 */
static void LISTBOX_RepaintItem( LB_DESCR *descr, INT index, UINT action )
{
    HDC hdc;
    RECT rect;
    HFONT oldFont = 0;
    HBRUSH hbrush, oldBrush = 0;

    /* Do not repaint the item if the item is not visible */
    if (!IsWindowVisible(descr->self)) return;
    if (descr->style & LBS_NOREDRAW)
    {
        descr->style |= LBS_DISPLAYCHANGED;
        return;
    }
    if (LISTBOX_GetItemRect( descr, index, &rect ) != 1) return;
    if (!(hdc = GetDCEx( descr->self, 0, DCX_CACHE ))) return;
    if (descr->font) oldFont = SelectObject( hdc, descr->font );
    hbrush = (HBRUSH)SendMessageW( descr->owner, WM_CTLCOLORLISTBOX,
				   (WPARAM)hdc, (LPARAM)descr->self );
    if (hbrush) oldBrush = SelectObject( hdc, hbrush );
    if (!IsWindowEnabled(descr->self))
        SetTextColor( hdc, GetSysColor( COLOR_GRAYTEXT ) );
    SetWindowOrgEx( hdc, descr->horz_pos, 0, NULL );
    LISTBOX_PaintItem( descr, hdc, &rect, index, action, TRUE );
    if (oldFont) SelectObject( hdc, oldFont );
    if (oldBrush) SelectObject( hdc, oldBrush );
    ReleaseDC( descr->self, hdc );
}


/***********************************************************************
 *           LISTBOX_DrawFocusRect
 */
static void LISTBOX_DrawFocusRect( LB_DESCR *descr, BOOL on )
{
    HDC hdc;
    RECT rect;
    HFONT oldFont = 0;

    /* Do not repaint the item if the item is not visible */
    if (!IsWindowVisible(descr->self)) return;

    if (descr->focus_item == -1) return;
    if (!descr->caret_on || !descr->in_focus) return;

    if (LISTBOX_GetItemRect( descr, descr->focus_item, &rect ) != 1) return;
    if (!(hdc = GetDCEx( descr->self, 0, DCX_CACHE ))) return;
    if (descr->font) oldFont = SelectObject( hdc, descr->font );
    if (!IsWindowEnabled(descr->self))
        SetTextColor( hdc, GetSysColor( COLOR_GRAYTEXT ) );
    SetWindowOrgEx( hdc, descr->horz_pos, 0, NULL );
    LISTBOX_PaintItem( descr, hdc, &rect, descr->focus_item, ODA_FOCUS, !on );
    if (oldFont) SelectObject( hdc, oldFont );
    ReleaseDC( descr->self, hdc );
}


/***********************************************************************
 *           LISTBOX_InitStorage
 */
static LRESULT LISTBOX_InitStorage( LB_DESCR *descr, INT nb_items )
{
    UINT new_size = descr->nb_items + nb_items;

    if (new_size > descr->items_size && !resize_storage(descr, new_size))
        return LB_ERRSPACE;
    return descr->items_size;
}


/***********************************************************************
 *           LISTBOX_SetTabStops
 */
static BOOL LISTBOX_SetTabStops( LB_DESCR *descr, INT count, LPINT tabs )
{
    INT i;

    if (!(descr->style & LBS_USETABSTOPS))
    {
        SetLastError(ERROR_LB_WITHOUT_TABSTOPS);
        return FALSE;
    }

    HeapFree( GetProcessHeap(), 0, descr->tabs );
    if (!(descr->nb_tabs = count))
    {
        descr->tabs = NULL;
        return TRUE;
    }
    if (!(descr->tabs = HeapAlloc( GetProcessHeap(), 0,
                                            descr->nb_tabs * sizeof(INT) )))
        return FALSE;
    memcpy( descr->tabs, tabs, descr->nb_tabs * sizeof(INT) );

    /* convert into "dialog units"*/
    for (i = 0; i < descr->nb_tabs; i++)
        descr->tabs[i] = MulDiv(descr->tabs[i], descr->avg_char_width, 4);

    return TRUE;
}


/***********************************************************************
 *           LISTBOX_GetText
 */
static LRESULT LISTBOX_GetText( LB_DESCR *descr, INT index, LPWSTR buffer, BOOL unicode )
{
    DWORD len;

    if ((index < 0) || (index >= descr->nb_items))
    {
        SetLastError(ERROR_INVALID_INDEX);
        return LB_ERR;
    }

    if (HAS_STRINGS(descr))
    {
        WCHAR *str = get_item_string(descr, index);

        if (!buffer)
            return lstrlenW(str);

        TRACE("index %d (0x%04x) %s\n", index, index, debugstr_w(str));

        __TRY  /* hide a Delphi bug that passes a read-only buffer */
        {
            lstrcpyW(buffer, str);
            len = lstrlenW(buffer);
        }
        __EXCEPT_PAGE_FAULT
        {
            WARN( "got an invalid buffer (Delphi bug?)\n" );
            SetLastError( ERROR_INVALID_PARAMETER );
            return LB_ERR;
        }
        __ENDTRY
    } else
    {
        if (buffer)
            *((ULONG_PTR *)buffer) = get_item_data(descr, index);
        len = sizeof(ULONG_PTR);
    }
    return len;
}

static inline INT LISTBOX_lstrcmpiW( LCID lcid, LPCWSTR str1, LPCWSTR str2 )
{
    INT ret = CompareStringW( lcid, NORM_IGNORECASE, str1, -1, str2, -1 );
    if (ret == CSTR_LESS_THAN)
        return -1;
    if (ret == CSTR_EQUAL)
        return 0;
    if (ret == CSTR_GREATER_THAN)
        return 1;
    return -1;
}

/***********************************************************************
 *           LISTBOX_FindStringPos
 *
 * Find the nearest string located before a given string in sort order.
 * If 'exact' is TRUE, return an error if we don't get an exact match.
 */
static INT LISTBOX_FindStringPos( LB_DESCR *descr, LPCWSTR str, BOOL exact )
{
    INT index, min, max, res;

    if (!descr->nb_items || !(descr->style & LBS_SORT)) return -1;  /* Add it at the end */

    min = 0;
    max = descr->nb_items - 1;
    while (min <= max)
    {
        index = (min + max) / 2;
        if (HAS_STRINGS(descr))
            res = LISTBOX_lstrcmpiW( descr->locale, get_item_string(descr, index), str );
        else
        {
            COMPAREITEMSTRUCT cis;
            UINT id = (UINT)GetWindowLongPtrW( descr->self, GWLP_ID );

            cis.CtlType    = ODT_LISTBOX;
            cis.CtlID      = id;
            cis.hwndItem   = descr->self;
            /* note that some application (MetaStock) expects the second item
             * to be in the listbox */
            cis.itemID1    = index;
            cis.itemData1  = get_item_data(descr, index);
            cis.itemID2    = -1;
            cis.itemData2  = (ULONG_PTR)str;
            cis.dwLocaleId = descr->locale;
            res = SendMessageW( descr->owner, WM_COMPAREITEM, id, (LPARAM)&cis );
        }
        if (!res) return index;
        if (res > 0) max = index - 1;
        else min = index + 1;
    }
    return exact ? -1 : min;
}


/***********************************************************************
 *           LISTBOX_FindFileStrPos
 *
 * Find the nearest string located before a given string in directory
 * sort order (i.e. first files, then directories, then drives).
 */
static INT LISTBOX_FindFileStrPos( LB_DESCR *descr, LPCWSTR str )
{
    INT min, max, res;

    if (!HAS_STRINGS(descr))
        return LISTBOX_FindStringPos( descr, str, FALSE );
    min = 0;
    max = descr->nb_items;
    while (min != max)
    {
        INT index = (min + max) / 2;
        LPCWSTR p = get_item_string(descr, index);
        if (*p == '[')  /* drive or directory */
        {
            if (*str != '[') res = -1;
            else if (p[1] == '-')  /* drive */
            {
                if (str[1] == '-') res = str[2] - p[2];
                else res = -1;
            }
            else  /* directory */
            {
                if (str[1] == '-') res = 1;
                else res = LISTBOX_lstrcmpiW( descr->locale, str, p );
            }
        }
        else  /* filename */
        {
            if (*str == '[') res = 1;
            else res = LISTBOX_lstrcmpiW( descr->locale, str, p );
        }
        if (!res) return index;
        if (res < 0) max = index;
        else min = index + 1;
    }
    return max;
}


/***********************************************************************
 *           LISTBOX_FindString
 *
 * Find the item beginning with a given string.
 */
static INT LISTBOX_FindString( LB_DESCR *descr, INT start, LPCWSTR str, BOOL exact )
{
    INT i, index;

    if (descr->style & LBS_NODATA) return LB_ERR;

    start++;
    if (start >= descr->nb_items) start = 0;
    if (HAS_STRINGS(descr))
    {
        if (!str || ! str[0] ) return LB_ERR;
        if (exact)
        {
            for (i = 0, index = start; i < descr->nb_items; i++, index++)
            {
                if (index == descr->nb_items) index = 0;
                if (!LISTBOX_lstrcmpiW(descr->locale, str, get_item_string(descr, index)))
                    return index;
            }
        }
        else
        {
            /* Special case for drives and directories: ignore prefix */
            INT len = lstrlenW(str);
            WCHAR *item_str;

            for (i = 0, index = start; i < descr->nb_items; i++, index++)
            {
                if (index == descr->nb_items) index = 0;
                item_str = get_item_string(descr, index);

                if (!wcsnicmp(str, item_str, len)) return index;
                if (item_str[0] == '[')
                {
                    if (!wcsnicmp(str, item_str + 1, len)) return index;
                    if (item_str[1] == '-' && !wcsnicmp(str, item_str + 2, len)) return index;
                }
            }
        }
    }
    else
    {
        if (exact && (descr->style & LBS_SORT))
            /* If sorted, use a WM_COMPAREITEM binary search */
            return LISTBOX_FindStringPos( descr, str, TRUE );

        /* Otherwise use a linear search */
        for (i = 0, index = start; i < descr->nb_items; i++, index++)
        {
            if (index == descr->nb_items) index = 0;
            if (get_item_data(descr, index) == (ULONG_PTR)str) return index;
        }
    }
    return LB_ERR;
}


/***********************************************************************
 *           LISTBOX_GetSelCount
 */
static LRESULT LISTBOX_GetSelCount( const LB_DESCR *descr )
{
    INT i, count;

    if (!(descr->style & LBS_MULTIPLESEL) ||
        (descr->style & LBS_NOSEL))
      return LB_ERR;
    for (i = count = 0; i < descr->nb_items; i++)
        if (is_item_selected(descr, i)) count++;
    return count;
}


/***********************************************************************
 *           LISTBOX_GetSelItems
 */
static LRESULT LISTBOX_GetSelItems( const LB_DESCR *descr, INT max, LPINT array )
{
    INT i, count;

    if (!(descr->style & LBS_MULTIPLESEL)) return LB_ERR;
    for (i = count = 0; (i < descr->nb_items) && (count < max); i++)
        if (is_item_selected(descr, i)) array[count++] = i;
    return count;
}


/***********************************************************************
 *           LISTBOX_Paint
 */
static LRESULT LISTBOX_Paint( LB_DESCR *descr, HDC hdc )
{
    INT i, col_pos = descr->page_size - 1;
    RECT rect;
    RECT focusRect = {-1, -1, -1, -1};
    HFONT oldFont = 0;
    HBRUSH hbrush, oldBrush = 0;

    if (descr->style & LBS_NOREDRAW) return 0;

    SetRect( &rect, 0, 0, descr->width, descr->height );
    if (descr->style & LBS_MULTICOLUMN)
        rect.right = rect.left + descr->column_width;
    else if (descr->horz_pos)
    {
        SetWindowOrgEx( hdc, descr->horz_pos, 0, NULL );
        rect.right += descr->horz_pos;
    }

    if (descr->font) oldFont = SelectObject( hdc, descr->font );
    hbrush = (HBRUSH)SendMessageW( descr->owner, WM_CTLCOLORLISTBOX,
				   (WPARAM)hdc, (LPARAM)descr->self );
    if (hbrush) oldBrush = SelectObject( hdc, hbrush );
    if (!IsWindowEnabled(descr->self)) SetTextColor( hdc, GetSysColor( COLOR_GRAYTEXT ) );

    if (!descr->nb_items && (descr->focus_item != -1) && descr->caret_on &&
        (descr->in_focus))
    {
        /* Special case for empty listbox: paint focus rect */
        rect.bottom = rect.top + descr->item_height;
        ExtTextOutW( hdc, 0, 0, ETO_OPAQUE | ETO_CLIPPED,
                     &rect, NULL, 0, NULL );
        LISTBOX_PaintItem( descr, hdc, &rect, descr->focus_item, ODA_FOCUS, FALSE );
        rect.top = rect.bottom;
    }

    /* Paint all the item, regarding the selection
       Focus state will be painted after  */

    for (i = descr->top_item; i < descr->nb_items; i++)
    {
        if (!(descr->style & LBS_OWNERDRAWVARIABLE))
            rect.bottom = rect.top + descr->item_height;
        else
            rect.bottom = rect.top + get_item_height(descr, i);

        /* keep the focus rect, to paint the focus item after */
        if (i == descr->focus_item)
            focusRect = rect;

        LISTBOX_PaintItem( descr, hdc, &rect, i, ODA_DRAWENTIRE, TRUE );
        rect.top = rect.bottom;

        if ((descr->style & LBS_MULTICOLUMN) && !col_pos)
        {
            if (!IS_OWNERDRAW(descr))
            {
                /* Clear the bottom of the column */
                if (rect.top < descr->height)
                {
                    rect.bottom = descr->height;
                    ExtTextOutW( hdc, 0, 0, ETO_OPAQUE | ETO_CLIPPED,
                                   &rect, NULL, 0, NULL );
                }
            }

            /* Go to the next column */
            rect.left += descr->column_width;
            rect.right += descr->column_width;
            rect.top = 0;
            col_pos = descr->page_size - 1;
            if (rect.left >= descr->width) break;
        }
        else
        {
            col_pos--;
            if (rect.top >= descr->height) break;
        }
    }

    /* Paint the focus item now */
    if (focusRect.top != focusRect.bottom &&
        descr->caret_on && descr->in_focus)
        LISTBOX_PaintItem( descr, hdc, &focusRect, descr->focus_item, ODA_FOCUS, FALSE );

    if (!IS_OWNERDRAW(descr))
    {
        /* Clear the remainder of the client area */
        if (rect.top < descr->height)
        {
            rect.bottom = descr->height;
            ExtTextOutW( hdc, 0, 0, ETO_OPAQUE | ETO_CLIPPED,
                           &rect, NULL, 0, NULL );
        }
        if (rect.right < descr->width)
        {
            rect.left   = rect.right;
            rect.right  = descr->width;
            rect.top    = 0;
            rect.bottom = descr->height;
            ExtTextOutW( hdc, 0, 0, ETO_OPAQUE | ETO_CLIPPED,
                           &rect, NULL, 0, NULL );
        }
    }
    if (oldFont) SelectObject( hdc, oldFont );
    if (oldBrush) SelectObject( hdc, oldBrush );
    return 0;
}

static void LISTBOX_NCPaint( LB_DESCR *descr, HRGN region )
{
    DWORD exstyle = GetWindowLongW( descr->self, GWL_EXSTYLE);
    HTHEME theme = GetWindowTheme( descr->self );
    HRGN cliprgn = region;
    int cxEdge, cyEdge;
    HDC hdc;
    RECT r;

    if (!theme || !(exstyle & WS_EX_CLIENTEDGE))
        return;

    cxEdge = GetSystemMetrics(SM_CXEDGE);
    cyEdge = GetSystemMetrics(SM_CYEDGE);

    GetWindowRect(descr->self, &r);

    /* New clipping region passed to default proc to exclude border */
    cliprgn = CreateRectRgn(r.left + cxEdge, r.top + cyEdge,
        r.right - cxEdge, r.bottom - cyEdge);
    if (region != (HRGN)1)
        CombineRgn(cliprgn, cliprgn, region, RGN_AND);
    OffsetRect(&r, -r.left, -r.top);

#ifdef __REACTOS__ /* r73789 */
    hdc = GetWindowDC(descr->self);
    /* Exclude client part */
    ExcludeClipRect(hdc,
                    r.left + cxEdge,
                    r.top + cyEdge,
                    r.right - cxEdge,
                    r.bottom -cyEdge);
#else
    hdc = GetDCEx(descr->self, region, DCX_WINDOW|DCX_INTERSECTRGN);
    OffsetRect(&r, -r.left, -r.top);
#endif

    if (IsThemeBackgroundPartiallyTransparent (theme, 0, 0))
        DrawThemeParentBackground(descr->self, hdc, &r);
    DrawThemeBackground (theme, hdc, 0, 0, &r, 0);
    ReleaseDC(descr->self, hdc);
}

/***********************************************************************
 *           LISTBOX_InvalidateItems
 *
 * Invalidate all items from a given item. If the specified item is not
 * visible, nothing happens.
 */
static void LISTBOX_InvalidateItems( LB_DESCR *descr, INT index )
{
    RECT rect;

    if (LISTBOX_GetItemRect( descr, index, &rect ) == 1)
    {
        if (descr->style & LBS_NOREDRAW)
        {
            descr->style |= LBS_DISPLAYCHANGED;
            return;
        }
        rect.bottom = descr->height;
        InvalidateRect( descr->self, &rect, TRUE );
        if (descr->style & LBS_MULTICOLUMN)
        {
            /* Repaint the other columns */
            rect.left  = rect.right;
            rect.right = descr->width;
            rect.top   = 0;
            InvalidateRect( descr->self, &rect, TRUE );
        }
    }
}

static void LISTBOX_InvalidateItemRect( LB_DESCR *descr, INT index )
{
    RECT rect;

    if (LISTBOX_GetItemRect( descr, index, &rect ) == 1)
        InvalidateRect( descr->self, &rect, TRUE );
}

/***********************************************************************
 *           LISTBOX_GetItemHeight
 */
static LRESULT LISTBOX_GetItemHeight( const LB_DESCR *descr, INT index )
{
    if (descr->style & LBS_OWNERDRAWVARIABLE && descr->nb_items > 0)
    {
        if ((index < 0) || (index >= descr->nb_items))
        {
            SetLastError(ERROR_INVALID_INDEX);
            return LB_ERR;
        }
        return get_item_height(descr, index);
    }
    else return descr->item_height;
}


/***********************************************************************
 *           LISTBOX_SetItemHeight
 */
static LRESULT LISTBOX_SetItemHeight( LB_DESCR *descr, INT index, INT height, BOOL repaint )
{
    if (height > MAXWORD)
        return -1;

    if (!height) height = 1;

    if (descr->style & LBS_OWNERDRAWVARIABLE)
    {
        if ((index < 0) || (index >= descr->nb_items))
        {
            SetLastError(ERROR_INVALID_INDEX);
            return LB_ERR;
        }
        TRACE("[%p]: item %d height = %d\n", descr->self, index, height );
        set_item_height(descr, index, height);
        LISTBOX_UpdateScroll( descr );
	if (repaint)
	    LISTBOX_InvalidateItems( descr, index );
    }
    else if (height != descr->item_height)
    {
        TRACE("[%p]: new height = %d\n", descr->self, height );
        descr->item_height = height;
        LISTBOX_UpdatePage( descr );
        LISTBOX_UpdateScroll( descr );
	if (repaint)
	    InvalidateRect( descr->self, 0, TRUE );
    }
    return LB_OKAY;
}


/***********************************************************************
 *           LISTBOX_SetHorizontalPos
 */
static void LISTBOX_SetHorizontalPos( LB_DESCR *descr, INT pos )
{
    INT diff;

    if (pos > descr->horz_extent - descr->width)
        pos = descr->horz_extent - descr->width;
    if (pos < 0) pos = 0;
    if (!(diff = descr->horz_pos - pos)) return;
    TRACE("[%p]: new horz pos = %d\n", descr->self, pos );
    descr->horz_pos = pos;
    LISTBOX_UpdateScroll( descr );
    if (abs(diff) < descr->width)
    {
        RECT rect;
        /* Invalidate the focused item so it will be repainted correctly */
        if (LISTBOX_GetItemRect( descr, descr->focus_item, &rect ) == 1)
            InvalidateRect( descr->self, &rect, TRUE );
        ScrollWindowEx( descr->self, diff, 0, NULL, NULL, 0, NULL,
                          SW_INVALIDATE | SW_ERASE | SW_SCROLLCHILDREN );
    }
    else
        InvalidateRect( descr->self, NULL, TRUE );
}


/***********************************************************************
 *           LISTBOX_SetHorizontalExtent
 */
static LRESULT LISTBOX_SetHorizontalExtent( LB_DESCR *descr, INT extent )
{
    if (descr->style & LBS_MULTICOLUMN)
        return LB_OKAY;
    if (extent == descr->horz_extent) return LB_OKAY;
    TRACE("[%p]: new horz extent = %d\n", descr->self, extent );
    descr->horz_extent = extent;
    if (descr->style & WS_HSCROLL) {
        SCROLLINFO info;
        info.cbSize = sizeof(info);
        info.nMin  = 0;
        info.nMax = descr->horz_extent ? descr->horz_extent - 1 : 0;
        info.fMask = SIF_RANGE;
        if (descr->style & LBS_DISABLENOSCROLL)
            info.fMask |= SIF_DISABLENOSCROLL;
        SetScrollInfo( descr->self, SB_HORZ, &info, TRUE );
    }
    if (descr->horz_pos > extent - descr->width)
        LISTBOX_SetHorizontalPos( descr, extent - descr->width );
    return LB_OKAY;
}


/***********************************************************************
 *           LISTBOX_SetColumnWidth
 */
static LRESULT LISTBOX_SetColumnWidth( LB_DESCR *descr, INT column_width)
{
    RECT rect;

    TRACE("[%p]: new column width = %d\n", descr->self, column_width);

    GetClientRect(descr->self, &rect);
    descr->width = rect.right - rect.left;
    descr->height = rect.bottom - rect.top;
    descr->column_width = column_width;

    LISTBOX_UpdatePage(descr);
    LISTBOX_UpdateScroll(descr);
    return LB_OKAY;
}


/***********************************************************************
 *           LISTBOX_SetFont
 *
 * Returns the item height.
 */
static INT LISTBOX_SetFont( LB_DESCR *descr, HFONT font )
{
    HDC hdc;
    HFONT oldFont = 0;
    const char *alphabet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    SIZE sz;

    descr->font = font;

    if (!(hdc = GetDCEx( descr->self, 0, DCX_CACHE )))
    {
        ERR("unable to get DC.\n" );
        return 16;
    }
    if (font) oldFont = SelectObject( hdc, font );
    GetTextExtentPointA( hdc, alphabet, 52, &sz);
    if (oldFont) SelectObject( hdc, oldFont );
    ReleaseDC( descr->self, hdc );

    descr->avg_char_width = (sz.cx / 26 + 1) / 2;
    if (!IS_OWNERDRAW(descr))
        LISTBOX_SetItemHeight( descr, 0, sz.cy, FALSE );
    return sz.cy;
}


/***********************************************************************
 *           LISTBOX_MakeItemVisible
 *
 * Make sure that a given item is partially or fully visible.
 */
static void LISTBOX_MakeItemVisible( LB_DESCR *descr, INT index, BOOL fully )
{
    INT top;

    TRACE("current top item %d, index %d, fully %d\n", descr->top_item, index, fully);

    if (index <= descr->top_item) top = index;
    else if (descr->style & LBS_MULTICOLUMN)
    {
        INT cols = descr->width;
        if (!fully) cols += descr->column_width - 1;
        if (cols >= descr->column_width) cols /= descr->column_width;
        else cols = 1;
        if (index < descr->top_item + (descr->page_size * cols)) return;
        top = index - descr->page_size * (cols - 1);
    }
    else if (descr->style & LBS_OWNERDRAWVARIABLE)
    {
        INT height = fully ? get_item_height(descr, index) : 1;
        for (top = index; top > descr->top_item; top--)
            if ((height += get_item_height(descr, top - 1)) > descr->height) break;
    }
    else
    {
        if (index < descr->top_item + descr->page_size) return;
        if (!fully && (index == descr->top_item + descr->page_size) &&
            (descr->height > (descr->page_size * descr->item_height))) return;
        top = index - descr->page_size + 1;
    }
    LISTBOX_SetTopItem( descr, top, TRUE );
}

/***********************************************************************
 *           LISTBOX_SetCaretIndex
 *
 * NOTES
 *   index must be between 0 and descr->nb_items-1, or LB_ERR is returned.
 *
 */
static LRESULT LISTBOX_SetCaretIndex( LB_DESCR *descr, INT index, BOOL fully_visible )
{
    BOOL focus_changed = descr->focus_item != index;

    TRACE("old focus %d, index %d\n", descr->focus_item, index);

    if (descr->style & LBS_NOSEL) return LB_ERR;
    if ((index < 0) || (index >= descr->nb_items)) return LB_ERR;

    if (focus_changed)
    {
        LISTBOX_DrawFocusRect( descr, FALSE );
        descr->focus_item = index;
    }

    LISTBOX_MakeItemVisible( descr, index, fully_visible );

    if (focus_changed)
        LISTBOX_DrawFocusRect( descr, TRUE );

    return LB_OKAY;
}


/***********************************************************************
 *           LISTBOX_SelectItemRange
 *
 * Select a range of items. Should only be used on a MULTIPLESEL listbox.
 */
static LRESULT LISTBOX_SelectItemRange( LB_DESCR *descr, INT first,
                                        INT last, BOOL on )
{
    INT i;

    /* A few sanity checks */

    if (descr->style & LBS_NOSEL) return LB_ERR;
    if (!(descr->style & LBS_MULTIPLESEL)) return LB_ERR;

    if (!descr->nb_items) return LB_OKAY;

    if (last == -1 || last >= descr->nb_items) last = descr->nb_items - 1;
    if (first < 0) first = 0;
    if (last < first) return LB_OKAY;

    if (on)  /* Turn selection on */
    {
        for (i = first; i <= last; i++)
        {
            if (is_item_selected(descr, i)) continue;
            set_item_selected_state(descr, i, TRUE);
            LISTBOX_InvalidateItemRect(descr, i);
        }
    }
    else  /* Turn selection off */
    {
        for (i = first; i <= last; i++)
        {
            if (!is_item_selected(descr, i)) continue;
            set_item_selected_state(descr, i, FALSE);
            LISTBOX_InvalidateItemRect(descr, i);
        }
    }
    return LB_OKAY;
}

/***********************************************************************
 *           LISTBOX_SetSelection
 */
static LRESULT LISTBOX_SetSelection( LB_DESCR *descr, INT index,
                                     BOOL on, BOOL send_notify )
{
    TRACE( "cur_sel=%d index=%d notify=%s\n",
           descr->selected_item, index, send_notify ? "YES" : "NO" );

    if (descr->style & LBS_NOSEL)
    {
        descr->selected_item = index;
        return LB_ERR;
    }
    if ((index < -1) || (index >= descr->nb_items)) return LB_ERR;
    if (descr->style & LBS_MULTIPLESEL)
    {
        if (index == -1)  /* Select all items */
            return LISTBOX_SelectItemRange( descr, 0, descr->nb_items, on );
        else  /* Only one item */
            return LISTBOX_SelectItemRange( descr, index, index, on );
    }
    else
    {
        INT oldsel = descr->selected_item;
        if (index == oldsel) return LB_OKAY;
        if (oldsel != -1) set_item_selected_state(descr, oldsel, FALSE);
        if (index != -1) set_item_selected_state(descr, index, TRUE);
        descr->selected_item = index;
        if (oldsel != -1) LISTBOX_RepaintItem( descr, oldsel, ODA_SELECT );
        if (index != -1) LISTBOX_RepaintItem( descr, index, ODA_SELECT );
        if (send_notify && descr->nb_items) SEND_NOTIFICATION( descr,
                               (index != -1) ? LBN_SELCHANGE : LBN_SELCANCEL );
	else
	    if( descr->lphc ) /* set selection change flag for parent combo */
		descr->lphc->wState |= CBF_SELCHANGE;
    }
    return LB_OKAY;
}


/***********************************************************************
 *           LISTBOX_MoveCaret
 *
 * Change the caret position and extend the selection to the new caret.
 */
static void LISTBOX_MoveCaret( LB_DESCR *descr, INT index, BOOL fully_visible )
{
    TRACE("old focus %d, index %d\n", descr->focus_item, index);

    if ((index <  0) || (index >= descr->nb_items))
        return;

    /* Important, repaint needs to be done in this order if
       you want to mimic Windows behavior:
       1. Remove the focus and paint the item
       2. Remove the selection and paint the item(s)
       3. Set the selection and repaint the item(s)
       4. Set the focus to 'index' and repaint the item */

    /* 1. remove the focus and repaint the item */
    LISTBOX_DrawFocusRect( descr, FALSE );

    /* 2. then turn off the previous selection */
    /* 3. repaint the new selected item */
    if (descr->style & LBS_EXTENDEDSEL)
    {
        if (descr->anchor_item != -1)
        {
            INT first = min( index, descr->anchor_item );
            INT last  = max( index, descr->anchor_item );
            if (first > 0)
                LISTBOX_SelectItemRange( descr, 0, first - 1, FALSE );
            LISTBOX_SelectItemRange( descr, last + 1, -1, FALSE );
            LISTBOX_SelectItemRange( descr, first, last, TRUE );
        }
    }
    else if (!(descr->style & LBS_MULTIPLESEL))
    {
        /* Set selection to new caret item */
        LISTBOX_SetSelection( descr, index, TRUE, FALSE );
    }

    /* 4. repaint the new item with the focus */
    descr->focus_item = index;
    LISTBOX_MakeItemVisible( descr, index, fully_visible );
    LISTBOX_DrawFocusRect( descr, TRUE );
}


/***********************************************************************
 *           LISTBOX_InsertItem
 */
static LRESULT LISTBOX_InsertItem( LB_DESCR *descr, INT index,
                                   LPWSTR str, ULONG_PTR data )
{
    INT oldfocus = descr->focus_item;

    if (index == -1) index = descr->nb_items;
    else if ((index < 0) || (index > descr->nb_items)) return LB_ERR;
    if (!resize_storage(descr, descr->nb_items + 1)) return LB_ERR;

    insert_item_data(descr, index);
    descr->nb_items++;
    set_item_string(descr, index, str);
    set_item_data(descr, index, HAS_STRINGS(descr) ? 0 : data);
    set_item_height(descr, index, 0);
    set_item_selected_state(descr, index, FALSE);

    /* Get item height */

    if (descr->style & LBS_OWNERDRAWVARIABLE)
    {
        MEASUREITEMSTRUCT mis;
        UINT id = (UINT)GetWindowLongPtrW( descr->self, GWLP_ID );

        mis.CtlType    = ODT_LISTBOX;
        mis.CtlID      = id;
        mis.itemID     = index;
        mis.itemData   = data;
        mis.itemHeight = descr->item_height;
        SendMessageW( descr->owner, WM_MEASUREITEM, id, (LPARAM)&mis );
        set_item_height(descr, index, mis.itemHeight ? mis.itemHeight : 1);
        TRACE("[%p]: measure item %d (%s) = %d\n",
              descr->self, index, str ? debugstr_w(str) : "", get_item_height(descr, index));
    }

    /* Repaint the items */

    LISTBOX_UpdateScroll( descr );
    LISTBOX_InvalidateItems( descr, index );

    /* Move selection and focused item */
    /* If listbox was empty, set focus to the first item */
    if (descr->nb_items == 1)
         LISTBOX_SetCaretIndex( descr, 0, FALSE );
    /* single select don't change selection index in win31 */
    else if ((ISWIN31) && !(IS_MULTISELECT(descr)))
    {
        descr->selected_item++;
        LISTBOX_SetSelection( descr, descr->selected_item-1, TRUE, FALSE );
    }
    else
    {
        if (index <= descr->selected_item)
        {
            descr->selected_item++;
            descr->focus_item = oldfocus; /* focus not changed */
        }
    }
    return LB_OKAY;
}


/***********************************************************************
 *           LISTBOX_InsertString
 */
static LRESULT LISTBOX_InsertString( LB_DESCR *descr, INT index, LPCWSTR str )
{
    LPWSTR new_str = NULL;
    LRESULT ret;

    if (HAS_STRINGS(descr))
    {
        static const WCHAR empty_stringW[] = { 0 };
        if (!str) str = empty_stringW;
        if (!(new_str = HeapAlloc( GetProcessHeap(), 0, (lstrlenW(str) + 1) * sizeof(WCHAR) )))
        {
            SEND_NOTIFICATION( descr, LBN_ERRSPACE );
            return LB_ERRSPACE;
        }
        lstrcpyW(new_str, str);
    }

    if (index == -1) index = descr->nb_items;
    if ((ret = LISTBOX_InsertItem( descr, index, new_str, (ULONG_PTR)str )) != 0)
    {
        HeapFree( GetProcessHeap(), 0, new_str );
        return ret;
    }

    TRACE("[%p]: added item %d %s\n",
          descr->self, index, HAS_STRINGS(descr) ? debugstr_w(new_str) : "" );
    return index;
}


/***********************************************************************
 *           LISTBOX_DeleteItem
 *
 * Delete the content of an item. 'index' must be a valid index.
 */
static void LISTBOX_DeleteItem( LB_DESCR *descr, INT index )
{
    /* Note: Win 3.1 only sends DELETEITEM on owner-draw items,
     *       while Win95 sends it for all items with user data.
     *       It's probably better to send it too often than not
     *       often enough, so this is what we do here.
     */
    if (IS_OWNERDRAW(descr) || get_item_data(descr, index))
    {
        DELETEITEMSTRUCT dis;
        UINT id = (UINT)GetWindowLongPtrW( descr->self, GWLP_ID );

        dis.CtlType  = ODT_LISTBOX;
        dis.CtlID    = id;
        dis.itemID   = index;
        dis.hwndItem = descr->self;
        dis.itemData = get_item_data(descr, index);
        SendMessageW( descr->owner, WM_DELETEITEM, id, (LPARAM)&dis );
    }
    HeapFree( GetProcessHeap(), 0, get_item_string(descr, index) );
}


/***********************************************************************
 *           LISTBOX_RemoveItem
 *
 * Remove an item from the listbox and delete its content.
 */
static LRESULT LISTBOX_RemoveItem( LB_DESCR *descr, INT index )
{
    if ((index < 0) || (index >= descr->nb_items)) return LB_ERR;

    /* We need to invalidate the original rect instead of the updated one. */
    LISTBOX_InvalidateItems( descr, index );

    if (descr->nb_items == 1)
    {
        SendMessageW(descr->self, LB_RESETCONTENT, 0, 0);
        return LB_OKAY;
    }
    descr->nb_items--;
    LISTBOX_DeleteItem( descr, index );
    remove_item_data(descr, index);

    if (descr->anchor_item == descr->nb_items) descr->anchor_item--;
    resize_storage(descr, descr->nb_items);

    /* Repaint the items */

    LISTBOX_UpdateScroll( descr );
    /* if we removed the scrollbar, reset the top of the list
      (correct for owner-drawn ???) */
    if (descr->nb_items == descr->page_size)
        LISTBOX_SetTopItem( descr, 0, TRUE );

    /* Move selection and focused item */
    if (!IS_MULTISELECT(descr))
    {
        if (index == descr->selected_item)
            descr->selected_item = -1;
        else if (index < descr->selected_item)
        {
            descr->selected_item--;
            if (ISWIN31) /* win 31 do not change the selected item number */
               LISTBOX_SetSelection( descr, descr->selected_item + 1, TRUE, FALSE);
        }
    }

    if (descr->focus_item >= descr->nb_items)
    {
          descr->focus_item = descr->nb_items - 1;
          if (descr->focus_item < 0) descr->focus_item = 0;
    }
    return LB_OKAY;
}


/***********************************************************************
 *           LISTBOX_ResetContent
 */
static void LISTBOX_ResetContent( LB_DESCR *descr )
{
    INT i;

    if (!(descr->style & LBS_NODATA))
        for (i = descr->nb_items - 1; i >= 0; i--) LISTBOX_DeleteItem(descr, i);
    HeapFree( GetProcessHeap(), 0, descr->u.items );
    descr->nb_items      = 0;
    descr->top_item      = 0;
    descr->selected_item = -1;
    descr->focus_item    = 0;
    descr->anchor_item   = -1;
    descr->items_size    = 0;
    descr->u.items       = NULL;
}


/***********************************************************************
 *           LISTBOX_SetCount
 */
static LRESULT LISTBOX_SetCount( LB_DESCR *descr, UINT count )
{
    UINT orig_num = descr->nb_items;

    if (!(descr->style & LBS_NODATA)) return LB_ERR;

    if (!resize_storage(descr, count))
        return LB_ERRSPACE;
    descr->nb_items = count;

    if (count)
    {
        LISTBOX_UpdateScroll(descr);
        if (count < orig_num)
        {
            descr->anchor_item = min(descr->anchor_item, count - 1);
            if (descr->selected_item >= count)
                descr->selected_item = -1;

            /* If we removed the scrollbar, reset the top of the list */
            if (count <= descr->page_size && orig_num > descr->page_size)
                LISTBOX_SetTopItem(descr, 0, TRUE);

            descr->focus_item = min(descr->focus_item, count - 1);
        }

        /* If it was empty before growing, set focus to the first item */
        else if (orig_num == 0) LISTBOX_SetCaretIndex(descr, 0, FALSE);
    }
    else SendMessageW(descr->self, LB_RESETCONTENT, 0, 0);

    InvalidateRect( descr->self, NULL, TRUE );
    return LB_OKAY;
}


/***********************************************************************
 *           LISTBOX_Directory
 */
static LRESULT LISTBOX_Directory( LB_DESCR *descr, UINT attrib,
                                  LPCWSTR filespec, BOOL long_names )
{
    HANDLE handle;
    LRESULT ret = LB_OKAY;
    WIN32_FIND_DATAW entry;
    int pos;
    LRESULT maxinsert = LB_ERR;

    /* don't scan directory if we just want drives exclusively */
    if (attrib != (DDL_DRIVES | DDL_EXCLUSIVE)) {
        /* scan directory */
        if ((handle = FindFirstFileW(filespec, &entry)) == INVALID_HANDLE_VALUE)
        {
	     int le = GetLastError();
            if ((le != ERROR_NO_MORE_FILES) && (le != ERROR_FILE_NOT_FOUND)) return LB_ERR;
        }
        else
        {
            do
            {
                WCHAR buffer[270];
                if (entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    static const WCHAR bracketW[]  = { ']',0 };
                    static const WCHAR dotW[] = { '.',0 };
                    if (!(attrib & DDL_DIRECTORY) ||
                        !lstrcmpW( entry.cFileName, dotW )) continue;
                    buffer[0] = '[';
                    if (!long_names && entry.cAlternateFileName[0])
                        lstrcpyW( buffer + 1, entry.cAlternateFileName );
                    else
                        lstrcpyW( buffer + 1, entry.cFileName );
                    lstrcatW(buffer, bracketW);
                }
                else  /* not a directory */
                {
#define ATTRIBS (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | \
                 FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_ARCHIVE)

                    if ((attrib & DDL_EXCLUSIVE) &&
                        ((attrib & ATTRIBS) != (entry.dwFileAttributes & ATTRIBS)))
                        continue;
#undef ATTRIBS
                    if (!long_names && entry.cAlternateFileName[0])
                        lstrcpyW( buffer, entry.cAlternateFileName );
                    else
                        lstrcpyW( buffer, entry.cFileName );
                }
                if (!long_names) CharLowerW( buffer );
                pos = LISTBOX_FindFileStrPos( descr, buffer );
                if ((ret = LISTBOX_InsertString( descr, pos, buffer )) < 0)
                    break;
                if (ret <= maxinsert) maxinsert++; else maxinsert = ret;
            } while (FindNextFileW( handle, &entry ));
            FindClose( handle );
        }
    }
    if (ret >= 0)
    {
        ret = maxinsert;

        /* scan drives */
        if (attrib & DDL_DRIVES)
        {
            WCHAR buffer[] = {'[','-','a','-',']',0};
            WCHAR root[] = {'A',':','\\',0};
            int drive;
            for (drive = 0; drive < 26; drive++, buffer[2]++, root[0]++)
            {
                if (GetDriveTypeW(root) <= DRIVE_NO_ROOT_DIR) continue;
                if ((ret = LISTBOX_InsertString( descr, -1, buffer )) < 0)
                    break;
            }
        }
    }
    return ret;
}


/***********************************************************************
 *           LISTBOX_HandleVScroll
 */
static LRESULT LISTBOX_HandleVScroll( LB_DESCR *descr, WORD scrollReq, WORD pos )
{
    SCROLLINFO info;

    if (descr->style & LBS_MULTICOLUMN) return 0;
    switch(scrollReq)
    {
    case SB_LINEUP:
        LISTBOX_SetTopItem( descr, descr->top_item - 1, TRUE );
        break;
    case SB_LINEDOWN:
        LISTBOX_SetTopItem( descr, descr->top_item + 1, TRUE );
        break;
    case SB_PAGEUP:
        LISTBOX_SetTopItem( descr, descr->top_item -
                            LISTBOX_GetCurrentPageSize( descr ), TRUE );
        break;
    case SB_PAGEDOWN:
        LISTBOX_SetTopItem( descr, descr->top_item +
                            LISTBOX_GetCurrentPageSize( descr ), TRUE );
        break;
    case SB_THUMBPOSITION:
        LISTBOX_SetTopItem( descr, pos, TRUE );
        break;
    case SB_THUMBTRACK:
        info.cbSize = sizeof(info);
        info.fMask = SIF_TRACKPOS;
        GetScrollInfo( descr->self, SB_VERT, &info );
        LISTBOX_SetTopItem( descr, info.nTrackPos, TRUE );
        break;
    case SB_TOP:
        LISTBOX_SetTopItem( descr, 0, TRUE );
        break;
    case SB_BOTTOM:
        LISTBOX_SetTopItem( descr, descr->nb_items, TRUE );
        break;
    }
    return 0;
}


/***********************************************************************
 *           LISTBOX_HandleHScroll
 */
static LRESULT LISTBOX_HandleHScroll( LB_DESCR *descr, WORD scrollReq, WORD pos )
{
    SCROLLINFO info;
    INT page;

    if (descr->style & LBS_MULTICOLUMN)
    {
        switch(scrollReq)
        {
        case SB_LINELEFT:
            LISTBOX_SetTopItem( descr, descr->top_item-descr->page_size,
                                TRUE );
            break;
        case SB_LINERIGHT:
            LISTBOX_SetTopItem( descr, descr->top_item+descr->page_size,
                                TRUE );
            break;
        case SB_PAGELEFT:
            page = descr->width / descr->column_width;
            if (page < 1) page = 1;
            LISTBOX_SetTopItem( descr,
                             descr->top_item - page * descr->page_size, TRUE );
            break;
        case SB_PAGERIGHT:
            page = descr->width / descr->column_width;
            if (page < 1) page = 1;
            LISTBOX_SetTopItem( descr,
                             descr->top_item + page * descr->page_size, TRUE );
            break;
        case SB_THUMBPOSITION:
            LISTBOX_SetTopItem( descr, pos*descr->page_size, TRUE );
            break;
        case SB_THUMBTRACK:
            info.cbSize = sizeof(info);
            info.fMask  = SIF_TRACKPOS;
            GetScrollInfo( descr->self, SB_VERT, &info );
            LISTBOX_SetTopItem( descr, info.nTrackPos*descr->page_size,
                                TRUE );
            break;
        case SB_LEFT:
            LISTBOX_SetTopItem( descr, 0, TRUE );
            break;
        case SB_RIGHT:
            LISTBOX_SetTopItem( descr, descr->nb_items, TRUE );
            break;
        }
    }
    else if (descr->horz_extent)
    {
        switch(scrollReq)
        {
        case SB_LINELEFT:
            LISTBOX_SetHorizontalPos( descr, descr->horz_pos - 1 );
            break;
        case SB_LINERIGHT:
            LISTBOX_SetHorizontalPos( descr, descr->horz_pos + 1 );
            break;
        case SB_PAGELEFT:
            LISTBOX_SetHorizontalPos( descr,
                                      descr->horz_pos - descr->width );
            break;
        case SB_PAGERIGHT:
            LISTBOX_SetHorizontalPos( descr,
                                      descr->horz_pos + descr->width );
            break;
        case SB_THUMBPOSITION:
            LISTBOX_SetHorizontalPos( descr, pos );
            break;
        case SB_THUMBTRACK:
            info.cbSize = sizeof(info);
            info.fMask = SIF_TRACKPOS;
            GetScrollInfo( descr->self, SB_HORZ, &info );
            LISTBOX_SetHorizontalPos( descr, info.nTrackPos );
            break;
        case SB_LEFT:
            LISTBOX_SetHorizontalPos( descr, 0 );
            break;
        case SB_RIGHT:
            LISTBOX_SetHorizontalPos( descr,
                                      descr->horz_extent - descr->width );
            break;
        }
    }
    return 0;
}

static LRESULT LISTBOX_HandleMouseWheel(LB_DESCR *descr, SHORT delta )
{
    INT pulScrollLines = 3;

    SystemParametersInfoW(SPI_GETWHEELSCROLLLINES,0, &pulScrollLines, 0);

    /* if scrolling changes direction, ignore left overs */
    if ((delta < 0 && descr->wheel_remain < 0) ||
        (delta > 0 && descr->wheel_remain > 0))
        descr->wheel_remain += delta;
    else
        descr->wheel_remain = delta;

    if (descr->wheel_remain && pulScrollLines)
    {
        int cLineScroll;
        if (descr->style & LBS_MULTICOLUMN)
        {
            pulScrollLines = min(descr->width / descr->column_width, pulScrollLines);
            pulScrollLines = max(1, pulScrollLines);
            cLineScroll = pulScrollLines * descr->wheel_remain / WHEEL_DELTA;
            descr->wheel_remain -= WHEEL_DELTA * cLineScroll / pulScrollLines;
            cLineScroll *= descr->page_size;
        }
        else
        {
            pulScrollLines = min(descr->page_size, pulScrollLines);
            cLineScroll = pulScrollLines * descr->wheel_remain / WHEEL_DELTA;
            descr->wheel_remain -= WHEEL_DELTA * cLineScroll / pulScrollLines;
        }
        LISTBOX_SetTopItem( descr, descr->top_item - cLineScroll, TRUE );
    }
    return 0;
}

/***********************************************************************
 *           LISTBOX_HandleLButtonDown
 */
static LRESULT LISTBOX_HandleLButtonDown( LB_DESCR *descr, DWORD keys, INT x, INT y )
{
    INT index = LISTBOX_GetItemFromPoint( descr, x, y );

    TRACE("[%p]: lbuttondown %d,%d item %d, focus item %d\n",
          descr->self, x, y, index, descr->focus_item);

    if (!descr->caret_on && (descr->in_focus)) return 0;

    if (!descr->in_focus)
    {
        if( !descr->lphc ) SetFocus( descr->self );
        else SetFocus( (descr->lphc->hWndEdit) ? descr->lphc->hWndEdit : descr->lphc->self );
    }

    if (index == -1) return 0;

    if (!descr->lphc)
    {
        if (descr->style & LBS_NOTIFY )
            SendMessageW( descr->owner, WM_LBTRACKPOINT, index,
                            MAKELPARAM( x, y ) );
    }

    descr->captured = TRUE;
    SetCapture( descr->self );

    if (descr->style & (LBS_EXTENDEDSEL | LBS_MULTIPLESEL))
    {
        /* we should perhaps make sure that all items are deselected
           FIXME: needed for !LBS_EXTENDEDSEL, too ?
           if (!(keys & (MK_SHIFT|MK_CONTROL)))
           LISTBOX_SetSelection( descr, -1, FALSE, FALSE);
        */

        if (!(keys & MK_SHIFT)) descr->anchor_item = index;
        if (keys & MK_CONTROL)
        {
            LISTBOX_SetCaretIndex( descr, index, FALSE );
            LISTBOX_SetSelection( descr, index,
                                  !is_item_selected(descr, index),
                                  (descr->style & LBS_NOTIFY) != 0);
        }
        else
        {
            LISTBOX_MoveCaret( descr, index, FALSE );

            if (descr->style & LBS_EXTENDEDSEL)
            {
                LISTBOX_SetSelection( descr, index,
                               is_item_selected(descr, index),
                              (descr->style & LBS_NOTIFY) != 0 );
            }
            else
            {
                LISTBOX_SetSelection( descr, index,
                               !is_item_selected(descr, index),
                              (descr->style & LBS_NOTIFY) != 0 );
            }
        }
    }
    else
    {
        descr->anchor_item = index;
        LISTBOX_MoveCaret( descr, index, FALSE );
        LISTBOX_SetSelection( descr, index,
                              TRUE, (descr->style & LBS_NOTIFY) != 0 );
    }

    if (!descr->lphc)
    {
        if (GetWindowLongW( descr->self, GWL_EXSTYLE ) & WS_EX_DRAGDETECT)
        {
            POINT pt;

	    pt.x = x;
	    pt.y = y;

            if (DragDetect( descr->self, pt ))
                SendMessageW( descr->owner, WM_BEGINDRAG, 0, 0 );
        }
    }
    return 0;
}


/*************************************************************************
 * LISTBOX_HandleLButtonDownCombo [Internal]
 *
 * Process LButtonDown message for the ComboListBox
 *
 * PARAMS
 *     pWnd       [I] The windows internal structure
 *     pDescr     [I] The ListBox internal structure
 *     keys       [I] Key Flag (WM_LBUTTONDOWN doc for more info)
 *     x          [I] X Mouse Coordinate
 *     y          [I] Y Mouse Coordinate
 *
 * RETURNS
 *     0 since we are processing the WM_LBUTTONDOWN Message
 *
 * NOTES
 *  This function is only to be used when a ListBox is a ComboListBox
 */

static LRESULT LISTBOX_HandleLButtonDownCombo( LB_DESCR *descr, UINT msg, DWORD keys, INT x, INT y)
{
    RECT clientRect, screenRect;
    POINT mousePos;

    mousePos.x = x;
    mousePos.y = y;

    GetClientRect(descr->self, &clientRect);

    if(PtInRect(&clientRect, mousePos))
    {
       /* MousePos is in client, resume normal processing */
        if (msg == WM_LBUTTONDOWN)
        {
           descr->lphc->droppedIndex = descr->nb_items ? descr->selected_item : -1;
           return LISTBOX_HandleLButtonDown( descr, keys, x, y);
        }
        else if (descr->style & LBS_NOTIFY)
            SEND_NOTIFICATION( descr, LBN_DBLCLK );
    }
    else
    {
        POINT screenMousePos;
        HWND hWndOldCapture;

        /* Check the Non-Client Area */
        screenMousePos = mousePos;
        hWndOldCapture = GetCapture();
        ReleaseCapture();
        GetWindowRect(descr->self, &screenRect);
        ClientToScreen(descr->self, &screenMousePos);

        if(!PtInRect(&screenRect, screenMousePos))
        {
            LISTBOX_SetCaretIndex( descr, descr->lphc->droppedIndex, FALSE );
            LISTBOX_SetSelection( descr, descr->lphc->droppedIndex, FALSE, FALSE );
            COMBO_FlipListbox( descr->lphc, FALSE, FALSE );
        }
        else
        {
            /* Check to see the NC is a scrollbar */
            INT nHitTestType=0;
            LONG style = GetWindowLongW( descr->self, GWL_STYLE );
            /* Check Vertical scroll bar */
            if (style & WS_VSCROLL)
            {
                clientRect.right += GetSystemMetrics(SM_CXVSCROLL);
                if (PtInRect( &clientRect, mousePos ))
                    nHitTestType = HTVSCROLL;
            }
              /* Check horizontal scroll bar */
            if (style & WS_HSCROLL)
            {
                clientRect.bottom += GetSystemMetrics(SM_CYHSCROLL);
                if (PtInRect( &clientRect, mousePos ))
                    nHitTestType = HTHSCROLL;
            }
            /* Windows sends this message when a scrollbar is clicked
             */

            if(nHitTestType != 0)
            {
                SendMessageW(descr->self, WM_NCLBUTTONDOWN, nHitTestType,
                             MAKELONG(screenMousePos.x, screenMousePos.y));
            }
            /* Resume the Capture after scrolling is complete
             */
            if(hWndOldCapture != 0)
                SetCapture(hWndOldCapture);
        }
    }
    return 0;
}

/***********************************************************************
 *           LISTBOX_HandleLButtonUp
 */
static LRESULT LISTBOX_HandleLButtonUp( LB_DESCR *descr )
{
    if (LISTBOX_Timer != LB_TIMER_NONE)
        KillSystemTimer( descr->self, LB_TIMER_ID );
    LISTBOX_Timer = LB_TIMER_NONE;
    if (descr->captured)
    {
        descr->captured = FALSE;
        if (GetCapture() == descr->self) ReleaseCapture();
        if ((descr->style & LBS_NOTIFY) && descr->nb_items)
            SEND_NOTIFICATION( descr, LBN_SELCHANGE );
    }
    return 0;
}


/***********************************************************************
 *           LISTBOX_HandleTimer
 *
 * Handle scrolling upon a timer event.
 * Return TRUE if scrolling should continue.
 */
static LRESULT LISTBOX_HandleTimer( LB_DESCR *descr, INT index, TIMER_DIRECTION dir )
{
    switch(dir)
    {
    case LB_TIMER_UP:
        if (descr->top_item) index = descr->top_item - 1;
        else index = 0;
        break;
    case LB_TIMER_LEFT:
        if (descr->top_item) index -= descr->page_size;
        break;
    case LB_TIMER_DOWN:
        index = descr->top_item + LISTBOX_GetCurrentPageSize( descr );
        if (index == descr->focus_item) index++;
        if (index >= descr->nb_items) index = descr->nb_items - 1;
        break;
    case LB_TIMER_RIGHT:
        if (index + descr->page_size < descr->nb_items)
            index += descr->page_size;
        break;
    case LB_TIMER_NONE:
        break;
    }
    if (index == descr->focus_item) return FALSE;
    LISTBOX_MoveCaret( descr, index, FALSE );
    return TRUE;
}


/***********************************************************************
 *           LISTBOX_HandleSystemTimer
 *
 * WM_SYSTIMER handler.
 */
static LRESULT LISTBOX_HandleSystemTimer( LB_DESCR *descr )
{
    if (!LISTBOX_HandleTimer( descr, descr->focus_item, LISTBOX_Timer ))
    {
        KillSystemTimer( descr->self, LB_TIMER_ID );
        LISTBOX_Timer = LB_TIMER_NONE;
    }
    return 0;
}


/***********************************************************************
 *           LISTBOX_HandleMouseMove
 *
 * WM_MOUSEMOVE handler.
 */
static void LISTBOX_HandleMouseMove( LB_DESCR *descr,
                                     INT x, INT y )
{
    INT index;
    TIMER_DIRECTION dir = LB_TIMER_NONE;

    if (!descr->captured) return;

    if (descr->style & LBS_MULTICOLUMN)
    {
        if (y < 0) y = 0;
        else if (y >= descr->item_height * descr->page_size)
            y = descr->item_height * descr->page_size - 1;

        if (x < 0)
        {
            dir = LB_TIMER_LEFT;
            x = 0;
        }
        else if (x >= descr->width)
        {
            dir = LB_TIMER_RIGHT;
            x = descr->width - 1;
        }
    }
    else
    {
        if (y < 0) dir = LB_TIMER_UP;  /* above */
        else if (y >= descr->height) dir = LB_TIMER_DOWN;  /* below */
    }

    index = LISTBOX_GetItemFromPoint( descr, x, y );
    if (index == -1) index = descr->focus_item;
    if (!LISTBOX_HandleTimer( descr, index, dir )) dir = LB_TIMER_NONE;

    /* Start/stop the system timer */

    if (dir != LB_TIMER_NONE)
        SetSystemTimer( descr->self, LB_TIMER_ID, LB_SCROLL_TIMEOUT, NULL);
    else if (LISTBOX_Timer != LB_TIMER_NONE)
        KillSystemTimer( descr->self, LB_TIMER_ID );
    LISTBOX_Timer = dir;
}


/***********************************************************************
 *           LISTBOX_HandleKeyDown
 */
static LRESULT LISTBOX_HandleKeyDown( LB_DESCR *descr, DWORD key )
{
    INT caret = -1;
    BOOL bForceSelection = TRUE; /* select item pointed to by focus_item */
    if ((IS_MULTISELECT(descr)) || (descr->selected_item == descr->focus_item))
        bForceSelection = FALSE; /* only for single select list */

    if (descr->style & LBS_WANTKEYBOARDINPUT)
    {
        caret = SendMessageW( descr->owner, WM_VKEYTOITEM,
                                MAKEWPARAM(LOWORD(key), descr->focus_item),
                                (LPARAM)descr->self );
        if (caret == -2) return 0;
    }
    if (caret == -1) switch(key)
    {
    case VK_LEFT:
        if (descr->style & LBS_MULTICOLUMN)
        {
            bForceSelection = FALSE;
            if (descr->focus_item >= descr->page_size)
                caret = descr->focus_item - descr->page_size;
            break;
        }
        /* fall through */
    case VK_UP:
        caret = descr->focus_item - 1;
        if (caret < 0) caret = 0;
        break;
    case VK_RIGHT:
        if (descr->style & LBS_MULTICOLUMN)
        {
            bForceSelection = FALSE;
            caret = min(descr->focus_item + descr->page_size, descr->nb_items - 1);
            break;
        }
        /* fall through */
    case VK_DOWN:
        caret = descr->focus_item + 1;
        if (caret >= descr->nb_items) caret = descr->nb_items - 1;
        break;

    case VK_PRIOR:
        if (descr->style & LBS_MULTICOLUMN)
        {
            INT page = descr->width / descr->column_width;
            if (page < 1) page = 1;
            caret = descr->focus_item - (page * descr->page_size) + 1;
        }
        else caret = descr->focus_item-LISTBOX_GetCurrentPageSize(descr) + 1;
        if (caret < 0) caret = 0;
        break;
    case VK_NEXT:
        if (descr->style & LBS_MULTICOLUMN)
        {
            INT page = descr->width / descr->column_width;
            if (page < 1) page = 1;
            caret = descr->focus_item + (page * descr->page_size) - 1;
        }
        else caret = descr->focus_item + LISTBOX_GetCurrentPageSize(descr) - 1;
        if (caret >= descr->nb_items) caret = descr->nb_items - 1;
        break;
    case VK_HOME:
        caret = 0;
        break;
    case VK_END:
        caret = descr->nb_items - 1;
        break;
    case VK_SPACE:
        if (descr->style & LBS_EXTENDEDSEL) caret = descr->focus_item;
        else if (descr->style & LBS_MULTIPLESEL)
        {
            LISTBOX_SetSelection( descr, descr->focus_item,
                                  !is_item_selected(descr, descr->focus_item),
                                  (descr->style & LBS_NOTIFY) != 0 );
        }
        break;
    default:
        bForceSelection = FALSE;
    }
    if (bForceSelection) /* focused item is used instead of key */
        caret = descr->focus_item;
    if (caret >= 0)
    {
        if (((descr->style & LBS_EXTENDEDSEL) &&
            !(GetKeyState( VK_SHIFT ) & 0x8000)) ||
            !IS_MULTISELECT(descr))
            descr->anchor_item = caret;
        LISTBOX_MoveCaret( descr, caret, TRUE );

        if (descr->style & LBS_MULTIPLESEL)
            descr->selected_item = caret;
        else
            LISTBOX_SetSelection( descr, caret, TRUE, FALSE);
        if (descr->style & LBS_NOTIFY)
        {
            if (descr->lphc && IsWindowVisible( descr->self ))
            {
                /* make sure that combo parent doesn't hide us */
                descr->lphc->wState |= CBF_NOROLLUP;
            }
            if (descr->nb_items) SEND_NOTIFICATION( descr, LBN_SELCHANGE );
        }
    }
    return 0;
}


/***********************************************************************
 *           LISTBOX_HandleChar
 */
static LRESULT LISTBOX_HandleChar( LB_DESCR *descr, WCHAR charW )
{
    INT caret = -1;
    WCHAR str[2];

    str[0] = charW;
    str[1] = '\0';

    if (descr->style & LBS_WANTKEYBOARDINPUT)
    {
        caret = SendMessageW( descr->owner, WM_CHARTOITEM,
                                MAKEWPARAM(charW, descr->focus_item),
                                (LPARAM)descr->self );
        if (caret == -2) return 0;
    }
    if (caret == -1)
        caret = LISTBOX_FindString( descr, descr->focus_item, str, FALSE);
    if (caret != -1)
    {
        if ((!IS_MULTISELECT(descr)) && descr->selected_item == -1)
           LISTBOX_SetSelection( descr, caret, TRUE, FALSE);
        LISTBOX_MoveCaret( descr, caret, TRUE );
        if ((descr->style & LBS_NOTIFY) && descr->nb_items)
            SEND_NOTIFICATION( descr, LBN_SELCHANGE );
    }
    return 0;
}


/***********************************************************************
 *           LISTBOX_Create
 */
static BOOL LISTBOX_Create( HWND hwnd, LPHEADCOMBO lphc )
{
    LB_DESCR *descr;
    MEASUREITEMSTRUCT mis;
    RECT rect;

    if (!(descr = HeapAlloc( GetProcessHeap(), 0, sizeof(*descr) )))
        return FALSE;

    GetClientRect( hwnd, &rect );
    descr->self          = hwnd;
    descr->owner         = GetParent( descr->self );
    descr->style         = GetWindowLongW( descr->self, GWL_STYLE );
    descr->width         = rect.right - rect.left;
    descr->height        = rect.bottom - rect.top;
    descr->u.items       = NULL;
    descr->items_size    = 0;
    descr->nb_items      = 0;
    descr->top_item      = 0;
    descr->selected_item = -1;
    descr->focus_item    = 0;
    descr->anchor_item   = -1;
    descr->item_height   = 1;
    descr->page_size     = 1;
    descr->column_width  = 150;
    descr->horz_extent   = 0;
    descr->horz_pos      = 0;
    descr->nb_tabs       = 0;
    descr->tabs          = NULL;
    descr->wheel_remain  = 0;
    descr->caret_on      = !lphc;
    if (descr->style & LBS_NOSEL) descr->caret_on = FALSE;
    descr->in_focus 	 = FALSE;
    descr->captured      = FALSE;
    descr->font          = 0;
    descr->locale        = GetUserDefaultLCID();
    descr->lphc		 = lphc;

    if( lphc )
    {
        TRACE("[%p]: resetting owner %p -> %p\n", descr->self, descr->owner, lphc->self );
        descr->owner = lphc->self;
    }

    SetWindowLongPtrW( descr->self, 0, (LONG_PTR)descr );

/*    if (wnd->dwExStyle & WS_EX_NOPARENTNOTIFY) descr->style &= ~LBS_NOTIFY;
 */
    if (descr->style & LBS_EXTENDEDSEL) descr->style |= LBS_MULTIPLESEL;
    if (descr->style & LBS_MULTICOLUMN) descr->style &= ~LBS_OWNERDRAWVARIABLE;
    if (descr->style & LBS_OWNERDRAWVARIABLE) descr->style |= LBS_NOINTEGRALHEIGHT;
    if ((descr->style & (LBS_OWNERDRAWFIXED | LBS_HASSTRINGS | LBS_SORT)) != LBS_OWNERDRAWFIXED)
        descr->style &= ~LBS_NODATA;
    descr->item_height = LISTBOX_SetFont( descr, 0 );

    if (descr->style & LBS_OWNERDRAWFIXED)
    {
        descr->style &= ~LBS_OWNERDRAWVARIABLE;

	if( descr->lphc && (descr->lphc->dwStyle & CBS_DROPDOWN))
	{
	    /* WinWord gets VERY unhappy if we send WM_MEASUREITEM from here */
	  descr->item_height = lphc->fixedOwnerDrawHeight;
	}
	else
	{
            UINT id = (UINT)GetWindowLongPtrW( descr->self, GWLP_ID );
            mis.CtlType    = ODT_LISTBOX;
            mis.CtlID      = id;
            mis.itemID     = -1;
            mis.itemWidth  =  0;
            mis.itemData   =  0;
            mis.itemHeight = descr->item_height;
            SendMessageW( descr->owner, WM_MEASUREITEM, id, (LPARAM)&mis );
            descr->item_height = mis.itemHeight ? mis.itemHeight : 1;
	}
    }

    OpenThemeData( descr->self, WC_LISTBOXW );

    TRACE("owner: %p, style: %08x, width: %d, height: %d\n", descr->owner, descr->style, descr->width, descr->height);
    return TRUE;
}


/***********************************************************************
 *           LISTBOX_Destroy
 */
static BOOL LISTBOX_Destroy( LB_DESCR *descr )
{
    HTHEME theme = GetWindowTheme( descr->self );
    CloseThemeData( theme );
    LISTBOX_ResetContent( descr );
    SetWindowLongPtrW( descr->self, 0, 0 );
    HeapFree( GetProcessHeap(), 0, descr );
    return TRUE;
}


/***********************************************************************
 *           ListBoxWndProc_common
 */
static LRESULT CALLBACK LISTBOX_WindowProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    LB_DESCR *descr = (LB_DESCR *)GetWindowLongPtrW( hwnd, 0 );
    HEADCOMBO *lphc = NULL;
    HTHEME theme;
    LRESULT ret;

    if (!descr)
    {
        if (!IsWindow(hwnd)) return 0;

        if (msg == WM_CREATE)
        {
	    CREATESTRUCTW *lpcs = (CREATESTRUCTW *)lParam;
            if (lpcs->style & LBS_COMBOBOX) lphc = lpcs->lpCreateParams;
            if (!LISTBOX_Create( hwnd, lphc )) return -1;
            TRACE("creating hwnd %p descr %p\n", hwnd, (void *)GetWindowLongPtrW( hwnd, 0 ) );
            return 0;
        }
        /* Ignore all other messages before we get a WM_CREATE */
        return DefWindowProcW( hwnd, msg, wParam, lParam );
    }
    if (descr->style & LBS_COMBOBOX) lphc = descr->lphc;

    TRACE("[%p]: msg %#x wp %08lx lp %08lx\n", descr->self, msg, wParam, lParam );

    switch(msg)
    {
    case LB_RESETCONTENT:
        LISTBOX_ResetContent( descr );
        LISTBOX_UpdateScroll( descr );
        InvalidateRect( descr->self, NULL, TRUE );
        return 0;

    case LB_ADDSTRING:
    {
        const WCHAR *textW = (const WCHAR *)lParam;
        INT index = LISTBOX_FindStringPos( descr, textW, FALSE );
        return LISTBOX_InsertString( descr, index, textW );
    }

    case LB_INSERTSTRING:
        return LISTBOX_InsertString( descr, wParam, (const WCHAR *)lParam );

    case LB_ADDFILE:
    {
        const WCHAR *textW = (const WCHAR *)lParam;
        INT index = LISTBOX_FindFileStrPos( descr, textW );
        return LISTBOX_InsertString( descr, index, textW );
    }

    case LB_DELETESTRING:
        if (LISTBOX_RemoveItem( descr, wParam) != LB_ERR)
            return descr->nb_items;
        else
        {
            SetLastError(ERROR_INVALID_INDEX);
            return LB_ERR;
        }

    case LB_GETITEMDATA:
        if (((INT)wParam < 0) || ((INT)wParam >= descr->nb_items))
        {
            SetLastError(ERROR_INVALID_INDEX);
            return LB_ERR;
        }
        return get_item_data(descr, wParam);

    case LB_SETITEMDATA:
        if (((INT)wParam < 0) || ((INT)wParam >= descr->nb_items))
        {
            SetLastError(ERROR_INVALID_INDEX);
            return LB_ERR;
        }
        set_item_data(descr, wParam, lParam);
        /* undocumented: returns TRUE, not LB_OKAY (0) */
        return TRUE;

    case LB_GETCOUNT:
        return descr->nb_items;

    case LB_GETTEXT:
        return LISTBOX_GetText( descr, wParam, (LPWSTR)lParam, TRUE );

    case LB_GETTEXTLEN:
        if ((INT)wParam >= descr->nb_items || (INT)wParam < 0)
        {
            SetLastError(ERROR_INVALID_INDEX);
            return LB_ERR;
        }
        if (!HAS_STRINGS(descr)) return sizeof(ULONG_PTR);
        return lstrlenW(get_item_string(descr, wParam));

    case LB_GETCURSEL:
        if (descr->nb_items == 0)
            return LB_ERR;
        if (!IS_MULTISELECT(descr))
            return descr->selected_item;
        if (descr->selected_item != -1)
            return descr->selected_item;
        return descr->focus_item;
        /* otherwise, if the user tries to move the selection with the    */
        /* arrow keys, we will give the application something to choke on */
    case LB_GETTOPINDEX:
        return descr->top_item;

    case LB_GETITEMHEIGHT:
        return LISTBOX_GetItemHeight( descr, wParam );

    case LB_SETITEMHEIGHT:
        return LISTBOX_SetItemHeight( descr, wParam, lParam, TRUE );

    case LB_ITEMFROMPOINT:
        {
            POINT pt;
            RECT rect;
            int index;
            BOOL hit = TRUE;

            /* The hiword of the return value is not a client area
               hittest as suggested by MSDN, but rather a hittest on
               the returned listbox item. */

            if(descr->nb_items == 0)
                return 0x1ffff;      /* win9x returns 0x10000, we copy winnt */

            pt.x = (short)LOWORD(lParam);
            pt.y = (short)HIWORD(lParam);

            SetRect(&rect, 0, 0, descr->width, descr->height);

            if(!PtInRect(&rect, pt))
            {
                pt.x = min(pt.x, rect.right - 1);
                pt.x = max(pt.x, 0);
                pt.y = min(pt.y, rect.bottom - 1);
                pt.y = max(pt.y, 0);
                hit = FALSE;
            }

            index = LISTBOX_GetItemFromPoint(descr, pt.x, pt.y);

            if(index == -1)
            {
                index = descr->nb_items - 1;
                hit = FALSE;
            }
            return MAKELONG(index, hit ? 0 : 1);
        }

    case LB_SETCARETINDEX:
        if ((!IS_MULTISELECT(descr)) && (descr->selected_item != -1)) return LB_ERR;
        if (LISTBOX_SetCaretIndex( descr, wParam, !lParam ) == LB_ERR)
            return LB_ERR;
        else if (ISWIN31)
             return wParam;
        else
             return LB_OKAY;

    case LB_GETCARETINDEX:
        return descr->focus_item;

    case LB_SETTOPINDEX:
        return LISTBOX_SetTopItem( descr, wParam, TRUE );

    case LB_SETCOLUMNWIDTH:
        return LISTBOX_SetColumnWidth( descr, wParam );

    case LB_GETITEMRECT:
        return LISTBOX_GetItemRect( descr, wParam, (RECT *)lParam );

    case LB_FINDSTRING:
        return LISTBOX_FindString( descr, wParam, (const WCHAR *)lParam, FALSE );

    case LB_FINDSTRINGEXACT:
        return LISTBOX_FindString( descr, wParam, (const WCHAR *)lParam, TRUE );

    case LB_SELECTSTRING:
    {
        const WCHAR *textW = (const WCHAR *)lParam;
        INT index;

        if (HAS_STRINGS(descr))
            TRACE("LB_SELECTSTRING: %s\n", debugstr_w(textW));

        index = LISTBOX_FindString( descr, wParam, textW, FALSE );
        if (index != LB_ERR)
        {
            LISTBOX_MoveCaret( descr, index, TRUE );
            LISTBOX_SetSelection( descr, index, TRUE, FALSE );
        }
        return index;
    }

    case LB_GETSEL:
        if (((INT)wParam < 0) || ((INT)wParam >= descr->nb_items))
            return LB_ERR;
        return is_item_selected(descr, wParam);

    case LB_SETSEL:
        ret = LISTBOX_SetSelection( descr, lParam, wParam, FALSE );
        if (ret != LB_ERR && wParam)
        {
            descr->anchor_item = lParam;
            if (lParam != -1)
                LISTBOX_SetCaretIndex( descr, lParam, TRUE );
        }
        return ret;

    case LB_SETCURSEL:
        if (IS_MULTISELECT(descr)) return LB_ERR;
        LISTBOX_SetCaretIndex( descr, wParam, TRUE );
        ret = LISTBOX_SetSelection( descr, wParam, TRUE, FALSE );
	if (ret != LB_ERR) ret = descr->selected_item;
	return ret;

    case LB_GETSELCOUNT:
        return LISTBOX_GetSelCount( descr );

    case LB_GETSELITEMS:
        return LISTBOX_GetSelItems( descr, wParam, (LPINT)lParam );

    case LB_SELITEMRANGE:
        if (LOWORD(lParam) <= HIWORD(lParam))
            return LISTBOX_SelectItemRange( descr, LOWORD(lParam),
                                            HIWORD(lParam), wParam );
        else
            return LISTBOX_SelectItemRange( descr, HIWORD(lParam),
                                            LOWORD(lParam), wParam );

    case LB_SELITEMRANGEEX:
        if ((INT)lParam >= (INT)wParam)
            return LISTBOX_SelectItemRange( descr, wParam, lParam, TRUE );
        else
            return LISTBOX_SelectItemRange( descr, lParam, wParam, FALSE);

    case LB_GETHORIZONTALEXTENT:
        return descr->horz_extent;

    case LB_SETHORIZONTALEXTENT:
        return LISTBOX_SetHorizontalExtent( descr, wParam );

    case LB_GETANCHORINDEX:
        return descr->anchor_item;

    case LB_SETANCHORINDEX:
        if (((INT)wParam < -1) || ((INT)wParam >= descr->nb_items))
        {
            SetLastError(ERROR_INVALID_INDEX);
            return LB_ERR;
        }
        descr->anchor_item = (INT)wParam;
        return LB_OKAY;

    case LB_DIR:
        return LISTBOX_Directory( descr, wParam, (const WCHAR *)lParam, msg == LB_DIR );

    case LB_GETLOCALE:
        return descr->locale;

    case LB_SETLOCALE:
    {
        LCID ret;
        if (!IsValidLocale((LCID)wParam, LCID_INSTALLED))
            return LB_ERR;
        ret = descr->locale;
        descr->locale = (LCID)wParam;
        return ret;
    }

    case LB_INITSTORAGE:
        return LISTBOX_InitStorage( descr, wParam );

    case LB_SETCOUNT:
        return LISTBOX_SetCount( descr, (INT)wParam );

    case LB_SETTABSTOPS:
        return LISTBOX_SetTabStops( descr, wParam, (LPINT)lParam );

    case LB_CARETON:
        if (descr->caret_on)
            return LB_OKAY;
        descr->caret_on = TRUE;
        if ((descr->focus_item != -1) && (descr->in_focus))
            LISTBOX_RepaintItem( descr, descr->focus_item, ODA_FOCUS );
        return LB_OKAY;

    case LB_CARETOFF:
        if (!descr->caret_on)
            return LB_OKAY;
        descr->caret_on = FALSE;
        if ((descr->focus_item != -1) && (descr->in_focus))
            LISTBOX_RepaintItem( descr, descr->focus_item, ODA_FOCUS );
        return LB_OKAY;

    case LB_GETLISTBOXINFO:
        return descr->page_size;

    case WM_DESTROY:
        return LISTBOX_Destroy( descr );

    case WM_ENABLE:
        InvalidateRect( descr->self, NULL, TRUE );
        return 0;

    case WM_SETREDRAW:
        LISTBOX_SetRedraw( descr, wParam != 0 );
        return 0;

    case WM_GETDLGCODE:
        return DLGC_WANTARROWS | DLGC_WANTCHARS;

    case WM_PRINTCLIENT:
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = ( wParam ) ? ((HDC)wParam) :  BeginPaint( descr->self, &ps );
            ret = LISTBOX_Paint( descr, hdc );
            if( !wParam ) EndPaint( descr->self, &ps );
        }
        return ret;

    case WM_NCPAINT:
        LISTBOX_NCPaint( descr, (HRGN)wParam );
        break;

    case WM_SIZE:
        LISTBOX_UpdateSize( descr );
        return 0;
    case WM_GETFONT:
        return (LRESULT)descr->font;
    case WM_SETFONT:
        LISTBOX_SetFont( descr, (HFONT)wParam );
        if (lParam) InvalidateRect( descr->self, 0, TRUE );
        return 0;
    case WM_SETFOCUS:
        descr->in_focus = TRUE;
        descr->caret_on = TRUE;
        if (descr->focus_item != -1)
            LISTBOX_DrawFocusRect( descr, TRUE );
        SEND_NOTIFICATION( descr, LBN_SETFOCUS );
        return 0;
    case WM_KILLFOCUS:
        LISTBOX_HandleLButtonUp( descr ); /* Release capture if we have it */
        descr->in_focus = FALSE;
        descr->wheel_remain = 0;
        if ((descr->focus_item != -1) && descr->caret_on)
            LISTBOX_RepaintItem( descr, descr->focus_item, ODA_FOCUS );
        SEND_NOTIFICATION( descr, LBN_KILLFOCUS );
        return 0;
    case WM_HSCROLL:
        return LISTBOX_HandleHScroll( descr, LOWORD(wParam), HIWORD(wParam) );
    case WM_VSCROLL:
        return LISTBOX_HandleVScroll( descr, LOWORD(wParam), HIWORD(wParam) );
    case WM_MOUSEWHEEL:
        if (wParam & (MK_SHIFT | MK_CONTROL))
            return DefWindowProcW( descr->self, msg, wParam, lParam );
        return LISTBOX_HandleMouseWheel( descr, (SHORT)HIWORD(wParam) );
    case WM_LBUTTONDOWN:
	if (lphc)
            return LISTBOX_HandleLButtonDownCombo(descr, msg, wParam,
                                                  (INT16)LOWORD(lParam),
                                                  (INT16)HIWORD(lParam) );
        return LISTBOX_HandleLButtonDown( descr, wParam,
                                          (INT16)LOWORD(lParam),
                                          (INT16)HIWORD(lParam) );
    case WM_LBUTTONDBLCLK:
	if (lphc)
            return LISTBOX_HandleLButtonDownCombo(descr, msg, wParam,
                                                  (INT16)LOWORD(lParam),
                                                  (INT16)HIWORD(lParam) );
        if (descr->style & LBS_NOTIFY)
            SEND_NOTIFICATION( descr, LBN_DBLCLK );
        return 0;
    case WM_MOUSEMOVE:
        if ( lphc && ((lphc->dwStyle & CBS_DROPDOWNLIST) != CBS_SIMPLE) )
        {
            BOOL    captured = descr->captured;
            POINT   mousePos;
            RECT    clientRect;

            mousePos.x = (INT16)LOWORD(lParam);
            mousePos.y = (INT16)HIWORD(lParam);

            /*
             * If we are in a dropdown combobox, we simulate that
             * the mouse is captured to show the tracking of the item.
             */
            if (GetClientRect(descr->self, &clientRect) && PtInRect( &clientRect, mousePos ))
                descr->captured = TRUE;

            LISTBOX_HandleMouseMove( descr, mousePos.x, mousePos.y);

            descr->captured = captured;
        }
        else if (GetCapture() == descr->self)
        {
            LISTBOX_HandleMouseMove( descr, (INT16)LOWORD(lParam),
                                     (INT16)HIWORD(lParam) );
        }
        return 0;
    case WM_LBUTTONUP:
	if (lphc)
	{
            POINT mousePos;
            RECT  clientRect;

            /*
             * If the mouse button "up" is not in the listbox,
             * we make sure there is no selection by re-selecting the
             * item that was selected when the listbox was made visible.
             */
            mousePos.x = (INT16)LOWORD(lParam);
            mousePos.y = (INT16)HIWORD(lParam);

            GetClientRect(descr->self, &clientRect);

            /*
             * When the user clicks outside the combobox and the focus
             * is lost, the owning combobox will send a fake buttonup with
             * 0xFFFFFFF as the mouse location, we must also revert the
             * selection to the original selection.
             */
            if ( (lParam == (LPARAM)-1) || (!PtInRect( &clientRect, mousePos )) )
                LISTBOX_MoveCaret( descr, lphc->droppedIndex, FALSE );
        }
        return LISTBOX_HandleLButtonUp( descr );
    case WM_KEYDOWN:
        if( lphc && (lphc->dwStyle & CBS_DROPDOWNLIST) != CBS_SIMPLE )
        {
            /* for some reason Windows makes it possible to
             * show/hide ComboLBox by sending it WM_KEYDOWNs */

            if( (!(lphc->wState & CBF_EUI) && wParam == VK_F4) ||
                ( (lphc->wState & CBF_EUI) && !(lphc->wState & CBF_DROPPED)
                  && (wParam == VK_DOWN || wParam == VK_UP)) )
            {
                COMBO_FlipListbox( lphc, FALSE, FALSE );
                return 0;
            }
        }
        return LISTBOX_HandleKeyDown( descr, wParam );
    case WM_CHAR:
        return LISTBOX_HandleChar( descr, wParam );

    case WM_SYSTIMER:
        return LISTBOX_HandleSystemTimer( descr );
    case WM_ERASEBKGND:
        if ((IS_OWNERDRAW(descr)) && !(descr->style & LBS_DISPLAYCHANGED))
        {
            RECT rect;
            HBRUSH hbrush = (HBRUSH)SendMessageW( descr->owner, WM_CTLCOLORLISTBOX,
                                              wParam, (LPARAM)descr->self );
	    TRACE("hbrush = %p\n", hbrush);
	    if(!hbrush)
		hbrush = GetSysColorBrush(COLOR_WINDOW);
	    if(hbrush)
	    {
		GetClientRect(descr->self, &rect);
		FillRect((HDC)wParam, &rect, hbrush);
	    }
        }
        return 1;
    case WM_DROPFILES:
        if( lphc ) return 0;
        return SendMessageW( descr->owner, msg, wParam, lParam );

    case WM_NCDESTROY:
        if( lphc && (lphc->dwStyle & CBS_DROPDOWNLIST) != CBS_SIMPLE )
            lphc->hWndLBox = 0;
        break;

    case WM_NCACTIVATE:
        if (lphc) return 0;
	break;

    case WM_THEMECHANGED:
        theme = GetWindowTheme( hwnd );
        CloseThemeData( theme );
        OpenThemeData( hwnd, WC_LISTBOXW );
        break;

    default:
        if ((msg >= WM_USER) && (msg < 0xc000))
            WARN("[%p]: unknown msg %04x wp %08lx lp %08lx\n",
                 hwnd, msg, wParam, lParam );
    }

    return DefWindowProcW( hwnd, msg, wParam, lParam );
}

void LISTBOX_Register(void)
{
    WNDCLASSW wndClass;

    memset(&wndClass, 0, sizeof(wndClass));
    wndClass.style = CS_PARENTDC | CS_DBLCLKS | CS_GLOBALCLASS;
    wndClass.lpfnWndProc = LISTBOX_WindowProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = sizeof(LB_DESCR *);
    wndClass.hCursor = LoadCursorW(0, (LPWSTR)IDC_ARROW);
    wndClass.hbrBackground = NULL;
    wndClass.lpszClassName = WC_LISTBOXW;
    RegisterClassW(&wndClass);
}

void COMBOLBOX_Register(void)
{
    static const WCHAR combolboxW[] = {'C','o','m','b','o','L','B','o','x',0};
    WNDCLASSW wndClass;

    memset(&wndClass, 0, sizeof(wndClass));
    wndClass.style = CS_SAVEBITS | CS_DBLCLKS | CS_DROPSHADOW | CS_GLOBALCLASS;
    wndClass.lpfnWndProc = LISTBOX_WindowProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = sizeof(LB_DESCR *);
    wndClass.hCursor = LoadCursorW(0, (LPWSTR)IDC_ARROW);
    wndClass.hbrBackground = NULL;
    wndClass.lpszClassName = combolboxW;
    RegisterClassW(&wndClass);
}

#ifdef __REACTOS__
void LISTBOX_Unregister(void)
{
    UnregisterClassW(WC_LISTBOXW, NULL);
}

void COMBOLBOX_Unregister(void)
{
    static const WCHAR combolboxW[] = {'C','o','m','b','o','L','B','o','x',0};
    UnregisterClassW(combolboxW, NULL);
}
#endif
