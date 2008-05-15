/*
 * Listbox controls
 *
 * Copyright 1996 Alexandre Julliard
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
 * of Comctl32.dll version 6.0 on Oct. 9, 2004, by Dimitrie O. Paun.
 * 
 * Unless otherwise noted, we believe this code to be complete, as per
 * the specification mentioned above.
 * If you discover missing features, or bugs, please note them below.
 *
 * TODO:
 *    - GetListBoxInfo()
 *    - LB_GETLISTBOXINFO
 *    - LBS_NODATA
 */

#include <user32.h>

#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(listbox);
WINE_DECLARE_DEBUG_CHANNEL(combo);

/* Start of hack section -------------------------------- */

typedef short *LPINT16;

BOOL is_old_app(HWND hwnd)
{
	return FALSE;
}

#define WM_LBTRACKPOINT     0x0131
#define WS_EX_DRAGDETECT    0x00000002L
#define WM_BEGINDRAG        0x022C

UINT STDCALL SetSystemTimer(HWND,UINT_PTR,UINT,TIMERPROC);
BOOL STDCALL KillSystemTimer(HWND,UINT_PTR);

/* End of hack section -------------------------------- */

/* Items array granularity */
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
    LB_ITEMDATA  *items;        /* Array of items */
    INT         nb_items;       /* Number of items */
    INT         top_item;       /* Top visible item */
    INT         selected_item;  /* Selected item */
    INT         focus_item;     /* Item that has the focus */
    INT         anchor_item;    /* Anchor item for extended selection */
    INT         item_height;    /* Default item height */
    INT         page_size;      /* Items per listbox page */
    INT         column_width;   /* Column width for multi-column listboxes */
    INT         horz_extent;    /* Horizontal extent (0 if no hscroll) */
    INT         horz_pos;       /* Horizontal position */
    INT         nb_tabs;        /* Number of tabs in array */
    INT        *tabs;           /* Array of tabs */
    INT         avg_char_width; /* Average width of characters */
    BOOL        caret_on;       /* Is caret on? */
    BOOL        captured;       /* Is mouse captured? */
    BOOL	in_focus;
    HFONT       font;           /* Current font */
    LCID          locale;       /* Current locale for string comparisons */
    LPHEADCOMBO   lphc;		/* ComboLBox */
    LONG        UIState;
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

static LRESULT WINAPI ListBoxWndProcA( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
static LRESULT WINAPI ListBoxWndProcW( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );

static LRESULT LISTBOX_GetItemRect( const LB_DESCR *descr, INT index, RECT *rect );

/*********************************************************************
 * listbox class descriptor
 */
const struct builtin_class_descr LISTBOX_builtin_class =
{
#ifdef __REACTOS__
    L"ListBox",            /* name */
    CS_DBLCLKS /*| CS_PARENTDC*/,  /* style */
    (WNDPROC)ListBoxWndProcW,      /* procW */
    (WNDPROC)ListBoxWndProcA,      /* procA */
    sizeof(LB_DESCR *),   /* extra */
    (LPCWSTR) IDC_ARROW,           /* cursor */
    0                     /* brush */
#else
    "ListBox",            /* name */
    CS_DBLCLKS /*| CS_PARENTDC*/,  /* style */
    ListBoxWndProcA,      /* procA */
    ListBoxWndProcW,      /* procW */
    sizeof(LB_DESCR *),   /* extra */
    IDC_ARROW,            /* cursor */
    0                     /* brush */
#endif
};


/*********************************************************************
 * combolbox class descriptor
 */
const struct builtin_class_descr COMBOLBOX_builtin_class =
{
#ifdef __REACTOS__
    L"ComboLBox",          /* name */
    CS_DBLCLKS | CS_SAVEBITS,  /* style */
    (WNDPROC)ListBoxWndProcW,      /* procW */
    (WNDPROC)ListBoxWndProcA,      /* procA */
    sizeof(LB_DESCR *),   /* extra */
    (LPCWSTR) IDC_ARROW,           /* cursor */
    0                     /* brush */
#else
    "ComboLBox",          /* name */
    CS_DBLCLKS | CS_SAVEBITS,  /* style */
    ListBoxWndProcA,      /* procA */
    ListBoxWndProcW,      /* procW */
    sizeof(LB_DESCR *),   /* extra */
    IDC_ARROW,            /* cursor */
    0                     /* brush */
#endif
};

#ifndef __REACTOS__
/* check whether app is a Win 3.1 app */
inline static BOOL is_old_app( HWND hwnd )
{
    return (GetExpWinVer16( GetWindowLongA(hwnd,GWL_HINSTANCE) ) & 0xFF00 ) == 0x0300;
}
#endif


/***********************************************************************
 *           LISTBOX_Dump
 */
void LISTBOX_Dump( HWND hwnd )
{
    INT i;
    LB_ITEMDATA *item;
    LB_DESCR *descr = (LB_DESCR *)GetWindowLongA( hwnd, 0 );

    TRACE( "Listbox:\n" );
    TRACE( "hwnd=%p descr=%08x items=%d top=%d\n",
           hwnd, (UINT)descr, descr->nb_items, descr->top_item );
    for (i = 0, item = descr->items; i < descr->nb_items; i++, item++)
    {
        TRACE( "%4d: %-40s %d %08lx %3d\n",
               i, debugstr_w(item->str), item->selected, item->data, item->height );
    }
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
        if ((height += descr->items[i].height) > descr->height) break;
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
            if ((page -= descr->items[max].height) < 0) break;
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

        if (descr->horz_extent)
        {
            info.nMin  = 0;
            info.nMax  = descr->horz_extent - 1;
            info.nPos  = descr->horz_pos;
            info.nPage = descr->width;
            info.fMask = SIF_RANGE | SIF_POS | SIF_PAGE;
            if (descr->style & LBS_DISABLENOSCROLL)
                info.fMask |= SIF_DISABLENOSCROLL;
            if (descr->style & WS_HSCROLL)
                SetScrollInfo( descr->self, SB_HORZ, &info, TRUE );
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
    if (descr->style & LBS_MULTICOLUMN)
    {
        INT diff = (descr->top_item - index) / descr->page_size * descr->column_width;
        if (scroll && (abs(diff) < descr->width))
            ScrollWindowEx( descr->self, diff, 0, NULL, NULL, 0, NULL,
                              SW_INVALIDATE | SW_ERASE | SW_SCROLLCHILDREN );

        else
            scroll = FALSE;
    }
    else if (scroll)
    {
        INT diff;
        if (descr->style & LBS_OWNERDRAWVARIABLE)
        {
            INT i;
            diff = 0;
            if (index > descr->top_item)
            {
                for (i = index - 1; i >= descr->top_item; i--)
                    diff -= descr->items[i].height;
            }
            else
            {
                for (i = index; i < descr->top_item; i++)
                    diff += descr->items[i].height;
            }
        }
        else
            diff = (descr->top_item - index) * descr->item_height;

        if (abs(diff) < descr->height)
            ScrollWindowEx( descr->self, 0, diff, NULL, NULL, 0, NULL,
                            SW_INVALIDATE | SW_ERASE | SW_SCROLLCHILDREN );
        else
            scroll = FALSE;
    }
    if (!scroll) NtUserInvalidateRect( descr->self, NULL, TRUE );
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
        NtUserInvalidateRect( descr->self, NULL, TRUE );
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
#ifndef __REACTOS__
            if (is_old_app(hwnd))
            { /* give a margin for error to 16 bits programs - if we need
                 less than the height of the nonclient area, round to the
                 *next* number of items */
                int ncheight = rect.bottom - rect.top - descr->height;
                if ((descr->item_height - remaining) <= ncheight)
                    remaining = remaining - descr->item_height;
            }
#endif
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
        NtUserInvalidateRect( descr->self, &rect, FALSE );
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
        memset(rect, 0, sizeof(*rect));
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
                    rect->top -= descr->items[i].height;
            }
            else
            {
                for (i = descr->top_item; i < index; i++)
                    rect->top += descr->items[i].height;
            }
            rect->bottom = rect->top + descr->items[index].height;

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
                if ((pos += descr->items[index].height) > y) break;
                index++;
            }
        }
        else
        {
            while (index > 0)
            {
                index--;
                if ((pos -= descr->items[index].height) <= y) break;
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
static void LISTBOX_PaintItem( LB_DESCR *descr, HDC hdc,
                               const RECT *rect, INT index, UINT action, BOOL ignoreFocus )
{
    LB_ITEMDATA *item = NULL;
    if (index < descr->nb_items) item = &descr->items[index];

    if (IS_OWNERDRAW(descr))
    {
        DRAWITEMSTRUCT dis;
        RECT r;
        HRGN hrgn;

	if (!item)
	{
	    if (action == ODA_FOCUS)
        {
            if (!(descr->UIState & UISF_HIDEFOCUS))
                DrawFocusRect( hdc, rect );
        }
	    else
	        FIXME("called with an out of bounds index %d(%d) in owner draw, Not good.\n",index,descr->nb_items);
	    return;
	}

        /* some programs mess with the clipping region when
        drawing the item, *and* restore the previous region
        after they are done, so a region has better to exist
        else everything ends clipped */
        GetClientRect(descr->self, &r);
        hrgn = CreateRectRgnIndirect(&r);
        SelectClipRgn( hdc, hrgn);
        DeleteObject( hrgn );

        dis.CtlType      = ODT_LISTBOX;
        dis.CtlID        = GetWindowLongPtrW( descr->self, GWLP_ID );
        dis.hwndItem     = descr->self;
        dis.itemAction   = action;
        dis.hDC          = hdc;
        dis.itemID       = index;
        dis.itemState    = 0;
        if (item->selected) dis.itemState |= ODS_SELECTED;
        if (!ignoreFocus && (descr->focus_item == index) &&
            (descr->caret_on) &&
            (descr->in_focus)) dis.itemState |= ODS_FOCUS;
        if (!IsWindowEnabled(descr->self)) dis.itemState |= ODS_DISABLED;
        dis.itemData     = item->data;
        dis.rcItem       = *rect;
        TRACE("[%p]: drawitem %d (%s) action=%02x state=%02x rect=%ld,%ld-%ld,%ld\n",
              descr->self, index, item ? debugstr_w(item->str) : "", action,
              dis.itemState, rect->left, rect->top, rect->right, rect->bottom );
        SendMessageW(descr->owner, WM_DRAWITEM, dis.CtlID, (LPARAM)&dis);
    }
    else
    {
        COLORREF oldText = 0, oldBk = 0;

        if (action == ODA_FOCUS)
        {
            if (!(descr->UIState & UISF_HIDEFOCUS))
                DrawFocusRect( hdc, rect );
            return;
        }
        if (item && item->selected)
        {
            oldBk = SetBkColor( hdc, GetSysColor( COLOR_HIGHLIGHT ) );
            oldText = SetTextColor( hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
        }

        TRACE("[%p]: painting %d (%s) action=%02x rect=%ld,%ld-%ld,%ld\n",
              descr->self, index, item ? debugstr_w(item->str) : "", action,
              rect->left, rect->top, rect->right, rect->bottom );
        if (!item)
            ExtTextOutW( hdc, rect->left + 1, rect->top,
                           ETO_OPAQUE | ETO_CLIPPED, rect, NULL, 0, NULL );
        else if (!(descr->style & LBS_USETABSTOPS))
            ExtTextOutW( hdc, rect->left + 1, rect->top,
                         ETO_OPAQUE | ETO_CLIPPED, rect, item->str,
                         strlenW(item->str), NULL );
        else
	{
	    /* Output empty string to paint background in the full width. */
            ExtTextOutW( hdc, rect->left + 1, rect->top,
                         ETO_OPAQUE | ETO_CLIPPED, rect, NULL, 0, NULL );
            TabbedTextOutW( hdc, rect->left + 1 , rect->top,
                            item->str, strlenW(item->str),
                            descr->nb_tabs, descr->tabs, 0);
	}
        if (item && item->selected)
        {
            SetBkColor( hdc, oldBk );
            SetTextColor( hdc, oldText );
        }
        if (!ignoreFocus && (descr->focus_item == index) &&
            (descr->caret_on) &&
            (descr->in_focus) &&
            !(descr->UIState & UISF_HIDEFOCUS)) DrawFocusRect( hdc, rect );
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
            NtUserInvalidateRect(descr->self, NULL, TRUE);
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
    LISTBOX_PaintItem( descr, hdc, &rect, index, action, FALSE );
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
    LISTBOX_PaintItem( descr, hdc, &rect, descr->focus_item, ODA_FOCUS, on ? FALSE : TRUE );
    if (oldFont) SelectObject( hdc, oldFont );
    ReleaseDC( descr->self, hdc );
}


/***********************************************************************
 *           LISTBOX_InitStorage
 */
static LRESULT LISTBOX_InitStorage( LB_DESCR *descr, INT nb_items )
{
    LB_ITEMDATA *item;

    nb_items += LB_ARRAY_GRANULARITY - 1;
    nb_items -= (nb_items % LB_ARRAY_GRANULARITY);
    if (descr->items) {
        nb_items += HeapSize( GetProcessHeap(), 0, descr->items ) / sizeof(*item);
	item = HeapReAlloc( GetProcessHeap(), 0, descr->items,
                              nb_items * sizeof(LB_ITEMDATA));
    }
    else {
	item = HeapAlloc( GetProcessHeap(), 0,
                              nb_items * sizeof(LB_ITEMDATA));
    }

    if (!item)
    {
        SEND_NOTIFICATION( descr, LBN_ERRSPACE );
        return LB_ERRSPACE;
    }
    descr->items = item;
    return LB_OKAY;
}


/***********************************************************************
 *           LISTBOX_SetTabStops
 */
static BOOL LISTBOX_SetTabStops( LB_DESCR *descr, INT count, LPINT tabs, BOOL short_ints )
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
#ifndef __REACTOS__
    if (short_ints)
    {
        INT i;
        LPINT16 p = (LPINT16)tabs;

        TRACE("[%p]: settabstops ", hwnd );
        for (i = 0; i < descr->nb_tabs; i++) {
	    descr->tabs[i] = *p++<<1; /* FIXME */
            TRACE("%hd ", descr->tabs[i]);
	}
        TRACE("\n");
    }
    else memcpy( descr->tabs, tabs, descr->nb_tabs * sizeof(INT) );
#else
    memcpy( descr->tabs, tabs, descr->nb_tabs * sizeof(INT) );
#endif

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
    if ((index < 0) || (index >= descr->nb_items))
    {
        SetLastError(ERROR_INVALID_INDEX);
        return LB_ERR;
    }
    if (HAS_STRINGS(descr))
    {
        if (!buffer)
        {
            DWORD len = strlenW(descr->items[index].str);
            if( unicode )
                return len;
            return WideCharToMultiByte( CP_ACP, 0, descr->items[index].str, len,
                                        NULL, 0, NULL, NULL );
        }

	TRACE("index %d (0x%04x) %s\n", index, index, debugstr_w(descr->items[index].str));

        if(unicode)
        {
            strcpyW( buffer, descr->items[index].str );
            return strlenW(buffer);
        }
        else
        {
            return WideCharToMultiByte(CP_ACP, 0, descr->items[index].str, -1, (LPSTR)buffer, 0x7FFFFFFF, NULL, NULL) - 1;
        }
    } else {
        if (buffer)
            *((LPDWORD)buffer)=*(LPDWORD)(&descr->items[index].data);
        return sizeof(DWORD);
    }
}

static __inline INT LISTBOX_lstrcmpiW( LCID lcid, LPCWSTR str1, LPCWSTR str2 )
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
    INT index, min, max, res = -1;

    if (!(descr->style & LBS_SORT)) return -1;  /* Add it at the end */
    min = 0;
    max = descr->nb_items;
    while (min != max)
    {
        index = (min + max) / 2;
        if (HAS_STRINGS(descr))
            res = LISTBOX_lstrcmpiW( descr->locale, str, descr->items[index].str);
        else
        {
            COMPAREITEMSTRUCT cis;
            UINT id = (UINT)GetWindowLongPtrW( descr->self, GWLP_ID );

            cis.CtlType    = ODT_LISTBOX;
            cis.CtlID      = id;
            cis.hwndItem   = descr->self;
            /* note that some application (MetaStock) expects the second item
             * to be in the listbox */
            cis.itemID1    = -1;
            cis.itemData1  = (ULONG_PTR)str;
            cis.itemID2    = index;
            cis.itemData2  = descr->items[index].data;
            cis.dwLocaleId = descr->locale;
            res = SendMessageW( descr->owner, WM_COMPAREITEM, id, (LPARAM)&cis );
        }
        if (!res) return index;
        if (res < 0) max = index;
        else min = index + 1;
    }
    return exact ? -1 : max;
}


/***********************************************************************
 *           LISTBOX_FindFileStrPos
 *
 * Find the nearest string located before a given string in directory
 * sort order (i.e. first files, then directories, then drives).
 */
static INT LISTBOX_FindFileStrPos( LB_DESCR *descr, LPCWSTR str )
{
    INT min, max, res = -1;

    if (!HAS_STRINGS(descr))
        return LISTBOX_FindStringPos( descr, str, FALSE );
    min = 0;
    max = descr->nb_items;
    while (min != max)
    {
        INT index = (min + max) / 2;
        LPCWSTR p = descr->items[index].str;
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
    INT i;
    LB_ITEMDATA *item;

    if (start >= descr->nb_items) start = -1;
    item = descr->items + start + 1;
    if (HAS_STRINGS(descr))
    {
        if (!str || ! str[0] ) return LB_ERR;
        if (exact)
        {
            for (i = start + 1; i < descr->nb_items; i++, item++)
                if (!LISTBOX_lstrcmpiW( descr->locale, str, item->str )) return i;
            for (i = 0, item = descr->items; i <= start; i++, item++)
                if (!LISTBOX_lstrcmpiW( descr->locale, str, item->str )) return i;
        }
        else
        {
 /* Special case for drives and directories: ignore prefix */
#define CHECK_DRIVE(item) \
    if ((item)->str[0] == '[') \
    { \
        if (!strncmpiW( str, (item)->str+1, len )) return i; \
        if (((item)->str[1] == '-') && !strncmpiW(str, (item)->str+2, len)) \
        return i; \
    }

            INT len = strlenW(str);
            for (i = start + 1; i < descr->nb_items; i++, item++)
            {
               if (!strncmpiW( str, item->str, len )) return i;
               CHECK_DRIVE(item);
            }
            for (i = 0, item = descr->items; i <= start; i++, item++)
            {
               if (!strncmpiW( str, item->str, len )) return i;
               CHECK_DRIVE(item);
            }
#undef CHECK_DRIVE
        }
    }
    else
    {
        if (exact && (descr->style & LBS_SORT))
            /* If sorted, use a WM_COMPAREITEM binary search */
            return LISTBOX_FindStringPos( descr, str, TRUE );

        /* Otherwise use a linear search */
        for (i = start + 1; i < descr->nb_items; i++, item++)
            if (item->data == (ULONG_PTR)str) return i;
        for (i = 0, item = descr->items; i <= start; i++, item++)
            if (item->data == (ULONG_PTR)str) return i;
    }
    return LB_ERR;
}


/***********************************************************************
 *           LISTBOX_GetSelCount
 */
static LRESULT LISTBOX_GetSelCount( const LB_DESCR *descr )
{
    INT i, count;
    const LB_ITEMDATA *item = descr->items;

    if (!(descr->style & LBS_MULTIPLESEL) ||
        (descr->style & LBS_NOSEL))
      return LB_ERR;
    for (i = count = 0; i < descr->nb_items; i++, item++)
        if (item->selected) count++;
    return count;
}


#ifndef __REACTOS__
/***********************************************************************
 *           LISTBOX_GetSelItems16
 */
static LRESULT LISTBOX_GetSelItems16( const LB_DESCR *descr, INT16 max, LPINT16 array )
{
    INT i, count;
    const LB_ITEMDATA *item = descr->items;

    if (!(descr->style & LBS_MULTIPLESEL)) return LB_ERR;
    for (i = count = 0; (i < descr->nb_items) && (count < max); i++, item++)
        if (item->selected) array[count++] = (INT16)i;
    return count;
}
#endif


/***********************************************************************
 *           LISTBOX_GetSelItems
 */
static LRESULT LISTBOX_GetSelItems( const LB_DESCR *descr, INT max, LPINT array )
{
    INT i, count;
    const LB_ITEMDATA *item = descr->items;

    if (!(descr->style & LBS_MULTIPLESEL)) return LB_ERR;
    for (i = count = 0; (i < descr->nb_items) && (count < max); i++, item++)
        if (item->selected) array[count++] = i;
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
            rect.bottom = rect.top + descr->items[i].height;

        if (i == descr->focus_item)
        {
	    /* keep the focus rect, to paint the focus item after */
	    focusRect.left = rect.left;
	    focusRect.right = rect.right;
	    focusRect.top = rect.top;
	    focusRect.bottom = rect.bottom;
        }
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
        NtUserInvalidateRect( descr->self, &rect, TRUE );
        if (descr->style & LBS_MULTICOLUMN)
        {
            /* Repaint the other columns */
            rect.left  = rect.right;
            rect.right = descr->width;
            rect.top   = 0;
            NtUserInvalidateRect( descr->self, &rect, TRUE );
        }
    }
}

static void LISTBOX_InvalidateItemRect( LB_DESCR *descr, INT index )
{
    RECT rect;

    if (LISTBOX_GetItemRect( descr, index, &rect ) == 1)
        NtUserInvalidateRect( descr->self, &rect, TRUE );
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
        return descr->items[index].height;
    }
    else return descr->item_height;
}


/***********************************************************************
 *           LISTBOX_SetItemHeight
 */
static LRESULT LISTBOX_SetItemHeight( LB_DESCR *descr, INT index, INT height, BOOL repaint )
{
    if (height > MAXBYTE)
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
        descr->items[index].height = height;
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
	    NtUserInvalidateRect( descr->self, 0, TRUE );
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
            NtUserInvalidateRect( descr->self, &rect, TRUE );
        ScrollWindowEx( descr->self, diff, 0, NULL, NULL, 0, NULL,
                          SW_INVALIDATE | SW_ERASE | SW_SCROLLCHILDREN );
    }
    else
        NtUserInvalidateRect( descr->self, NULL, TRUE );
}


/***********************************************************************
 *           LISTBOX_SetHorizontalExtent
 */
static LRESULT LISTBOX_SetHorizontalExtent( LB_DESCR *descr, INT extent )
{
    if (!descr->horz_extent || (descr->style & LBS_MULTICOLUMN))
        return LB_OKAY;
    if (extent <= 0) extent = 1;
    if (extent == descr->horz_extent) return LB_OKAY;
    TRACE("[%p]: new horz extent = %d\n", descr->self, extent );
    descr->horz_extent = extent;
    if (descr->horz_pos > extent - descr->width)
        LISTBOX_SetHorizontalPos( descr, extent - descr->width );
    else
        LISTBOX_UpdateScroll( descr );
    return LB_OKAY;
}


/***********************************************************************
 *           LISTBOX_SetColumnWidth
 */
static LRESULT LISTBOX_SetColumnWidth( LB_DESCR *descr, INT width)
{
    if (width == descr->column_width) return LB_OKAY;
    TRACE("[%p]: new column width = %d\n", descr->self, width );
    descr->column_width = width;
    LISTBOX_UpdatePage( descr );
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
        INT height = fully ? descr->items[index].height : 1;
        for (top = index; top > descr->top_item; top--)
            if ((height += descr->items[top-1].height) > descr->height) break;
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
    INT oldfocus = descr->focus_item;

    TRACE("old focus %d, index %d\n", oldfocus, index);

    if (descr->style & LBS_NOSEL) return LB_ERR;
    if ((index < 0) || (index >= descr->nb_items)) return LB_ERR;
    if (index == oldfocus) return LB_OKAY;

    LISTBOX_DrawFocusRect( descr, FALSE );
    descr->focus_item = index;

    LISTBOX_MakeItemVisible( descr, index, fully_visible );
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
            if (descr->items[i].selected) continue;
            descr->items[i].selected = TRUE;
            LISTBOX_InvalidateItemRect(descr, i);
        }
    }
    else  /* Turn selection off */
    {
        for (i = first; i <= last; i++)
        {
            if (!descr->items[i].selected) continue;
            descr->items[i].selected = FALSE;
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
            return LISTBOX_SelectItemRange( descr, 0, -1, on );
        else  /* Only one item */
            return LISTBOX_SelectItemRange( descr, index, index, on );
    }
    else
    {
        INT oldsel = descr->selected_item;
        if (index == oldsel) return LB_OKAY;
        if (oldsel != -1) descr->items[oldsel].selected = FALSE;
        if (index != -1) descr->items[index].selected = TRUE;
        if (oldsel != -1) LISTBOX_RepaintItem( descr, oldsel, ODA_SELECT );
        descr->selected_item = index;
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
    LB_ITEMDATA *item;
    INT max_items;
    INT oldfocus = descr->focus_item;

    if (index == -1) index = descr->nb_items;
    else if ((index < 0) || (index > descr->nb_items)) return LB_ERR;
    if (!descr->items) max_items = 0;
    else max_items = HeapSize( GetProcessHeap(), 0, descr->items ) / sizeof(*item);
    if (descr->nb_items == max_items)
    {
        /* We need to grow the array */
        max_items += LB_ARRAY_GRANULARITY;
	if (descr->items)
    {
    	    item = HeapReAlloc( GetProcessHeap(), 0, descr->items,
                                  max_items * sizeof(LB_ITEMDATA) );
            if (!item)
            {
                SEND_NOTIFICATION( descr, LBN_ERRSPACE );
                return LB_ERRSPACE;
            }
    }
	else
	    item = HeapAlloc( GetProcessHeap(), 0,
                                  max_items * sizeof(LB_ITEMDATA) );
        if (!item)
        {
            SEND_NOTIFICATION( descr, LBN_ERRSPACE );
            return LB_ERRSPACE;
        }
        descr->items = item;
    }

    /* Insert the item structure */

    item = &descr->items[index];
    if (index < descr->nb_items)
        RtlMoveMemory( item + 1, item,
                       (descr->nb_items - index) * sizeof(LB_ITEMDATA) );
    item->str      = str;
    item->data     = data;
    item->height   = 0;
    item->selected = FALSE;
    descr->nb_items++;

    /* Get item height */

    if (descr->style & LBS_OWNERDRAWVARIABLE)
    {
        MEASUREITEMSTRUCT mis;
        UINT id = (UINT)GetWindowLongPtrW( descr->self, GWLP_ID );

        mis.CtlType    = ODT_LISTBOX;
        mis.CtlID      = id;
        mis.itemID     = index;
        mis.itemData   = descr->items[index].data;
        mis.itemHeight = descr->item_height;
        SendMessageW( descr->owner, WM_MEASUREITEM, id, (LPARAM)&mis );
        item->height = mis.itemHeight ? mis.itemHeight : 1;
        TRACE("[%p]: measure item %d (%s) = %d\n",
              descr->self, index, str ? debugstr_w(str) : "", item->height );
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
static LRESULT LISTBOX_InsertString( LB_DESCR *descr, INT index,
                                     LPCWSTR str )
{
    LPWSTR new_str = NULL;
    ULONG_PTR data = 0;
    LRESULT ret;

    if (HAS_STRINGS(descr))
    {
        static const WCHAR empty_stringW[] = { 0 };
        if (!str) str = empty_stringW;
        if (!(new_str = HeapAlloc( GetProcessHeap(), 0, (strlenW(str) + 1) * sizeof(WCHAR) )))
        {
            SEND_NOTIFICATION( descr, LBN_ERRSPACE );
            return LB_ERRSPACE;
        }
        strcpyW(new_str, str);
    }
    else data = (ULONG_PTR)str;

    if (index == -1) index = descr->nb_items;
    if ((ret = LISTBOX_InsertItem( descr, index, new_str, data )) != 0)
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
    if (IS_OWNERDRAW(descr) || descr->items[index].data)
    {
        DELETEITEMSTRUCT dis;
        UINT id = (UINT)GetWindowLongPtrW( descr->self, GWLP_ID );

        dis.CtlType  = ODT_LISTBOX;
        dis.CtlID    = id;
        dis.itemID   = index;
        dis.hwndItem = descr->self;
        dis.itemData = descr->items[index].data;
        SendMessageW( descr->owner, WM_DELETEITEM, id, (LPARAM)&dis );
    }
    if (HAS_STRINGS(descr))
        HeapFree( GetProcessHeap(), 0, descr->items[index].str );
}


/***********************************************************************
 *           LISTBOX_RemoveItem
 *
 * Remove an item from the listbox and delete its content.
 */
static LRESULT LISTBOX_RemoveItem( LB_DESCR *descr, INT index )
{
    LB_ITEMDATA *item;
    INT max_items;

    if ((index < 0) || (index >= descr->nb_items)) return LB_ERR;

    /* We need to invalidate the original rect instead of the updated one. */
    LISTBOX_InvalidateItems( descr, index );

    LISTBOX_DeleteItem( descr, index );

    /* Remove the item */

    item = &descr->items[index];
    if (index < descr->nb_items-1)
        RtlMoveMemory( item, item + 1,
                       (descr->nb_items - index - 1) * sizeof(LB_ITEMDATA) );
    descr->nb_items--;
    if (descr->anchor_item == descr->nb_items) descr->anchor_item--;

    /* Shrink the item array if possible */

    max_items = HeapSize( GetProcessHeap(), 0, descr->items ) / sizeof(LB_ITEMDATA);
    if (descr->nb_items < max_items - 2*LB_ARRAY_GRANULARITY)
    {
        max_items -= LB_ARRAY_GRANULARITY;
        item = HeapReAlloc( GetProcessHeap(), 0, descr->items,
                            max_items * sizeof(LB_ITEMDATA) );
        if (item) descr->items = item;
    }
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

    for (i = 0; i < descr->nb_items; i++) LISTBOX_DeleteItem( descr, i );
    HeapFree( GetProcessHeap(), 0, descr->items );
    descr->nb_items      = 0;
    descr->top_item      = 0;
    descr->selected_item = -1;
    descr->focus_item    = 0;
    descr->anchor_item   = -1;
    descr->items         = NULL;
}


/***********************************************************************
 *           LISTBOX_SetCount
 */
static LRESULT LISTBOX_SetCount( LB_DESCR *descr, INT count )
{
    LRESULT ret;

    if (HAS_STRINGS(descr))
    {
        SetLastError(ERROR_SETCOUNT_ON_BAD_LB);
        return LB_ERR;
    }

    /* FIXME: this is far from optimal... */
    if (count > descr->nb_items)
    {
        while (count > descr->nb_items)
            if ((ret = LISTBOX_InsertString( descr, -1, 0 )) < 0)
                return ret;
    }
    else if (count < descr->nb_items)
    {
        while (count < descr->nb_items)
            if ((ret = LISTBOX_RemoveItem( descr, (descr->nb_items - 1) )) < 0)
                return ret;
    }
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
                        !strcmpW( entry.cFileName, dotW )) continue;
                    buffer[0] = '[';
                    if (!long_names && entry.cAlternateFileName[0])
                        strcpyW( buffer + 1, entry.cAlternateFileName );
                    else
                        strcpyW( buffer + 1, entry.cFileName );
                    strcatW(buffer, bracketW);
                }
                else  /* not a directory */
                {
#define ATTRIBS (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | \
                 FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_ARCHIVE | \
                 FILE_ATTRIBUTE_DIRECTORY)

                    if ((attrib & DDL_EXCLUSIVE) &&
                        ((attrib & ATTRIBS) != (entry.dwFileAttributes & ATTRIBS)))
                        continue;
#undef ATTRIBS
                    if (!long_names && entry.cAlternateFileName[0])
                        strcpyW( buffer, entry.cAlternateFileName );
                    else
                        strcpyW( buffer, entry.cFileName );
                }
                if (!long_names) CharLowerW( buffer );
                pos = LISTBOX_FindFileStrPos( descr, buffer );
                if ((ret = LISTBOX_InsertString( descr, pos, buffer )) < 0)
                    break;

                if (ret <= maxinsert)
                    maxinsert++;
                else
                    maxinsert = ret;

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
    short gcWheelDelta = 0;
    UINT pulScrollLines = 3;

    SystemParametersInfoW(SPI_GETWHEELSCROLLLINES,0, &pulScrollLines, 0);

    gcWheelDelta -= delta;

    if (abs(gcWheelDelta) >= WHEEL_DELTA && pulScrollLines)
    {
        int cLineScroll = (int) min((UINT) descr->page_size, pulScrollLines);
        cLineScroll *= (gcWheelDelta / WHEEL_DELTA);
        LISTBOX_SetTopItem( descr, descr->top_item + cLineScroll, TRUE );
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
                                  !descr->items[index].selected,
                                  (descr->style & LBS_NOTIFY) != 0);
        }
        else
        {
            LISTBOX_MoveCaret( descr, index, FALSE );

            if (descr->style & LBS_EXTENDEDSEL)
            {
                LISTBOX_SetSelection( descr, index,
                               descr->items[index].selected,
                              (descr->style & LBS_NOTIFY) != 0 );
            }
            else
            {
                LISTBOX_SetSelection( descr, index,
                               !descr->items[index].selected,
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
            if (descr->focus_item + descr->page_size < descr->nb_items)
                caret = descr->focus_item + descr->page_size;
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
                                  !descr->items[descr->focus_item].selected,
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
            if( descr->lphc )
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

/* Retrieve the UI state for the control */
static BOOL LISTBOX_update_uistate(LB_DESCR *descr)
{
    LONG prev_flags;

    prev_flags = descr->UIState;
    descr->UIState = DefWindowProcW(descr->self, WM_QUERYUISTATE, 0, 0);
    return prev_flags != descr->UIState;
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
    descr->items         = NULL;
    descr->nb_items      = 0;
    descr->top_item      = 0;
    descr->selected_item = -1;
    descr->focus_item    = 0;
    descr->anchor_item   = -1;
    descr->item_height   = 1;
    descr->page_size     = 1;
    descr->column_width  = 150;
    descr->horz_extent   = (descr->style & WS_HSCROLL) ? 1 : 0;
    descr->horz_pos      = 0;
    descr->nb_tabs       = 0;
    descr->tabs          = NULL;
    descr->caret_on      = lphc ? FALSE : TRUE;
    if (descr->style & LBS_NOSEL) descr->caret_on = FALSE;
    descr->in_focus 	 = FALSE;
    descr->captured      = FALSE;
    descr->font          = 0;
    descr->locale        = GetUserDefaultLCID();
    descr->lphc		 = lphc;

#ifndef __REACTOS__
    if (is_old_app(hwnd) && ( descr->style & ( WS_VSCROLL | WS_HSCROLL ) ) )
    {
	/* Win95 document "List Box Differences" from MSDN:
	   If a list box in a version 3.x application has either the
	   WS_HSCROLL or WS_VSCROLL style, the list box receives both
	   horizontal and vertical scroll bars.
	*/
	descr->style |= WS_VSCROLL | WS_HSCROLL;
    }
#endif

    if( lphc )
    {
        TRACE_(combo)("[%p]: resetting owner %p -> %p\n", hwnd, descr->owner, lphc->self );
        descr->owner = lphc->self;
    }

    SetWindowLongPtrW( descr->self, 0, (LONG_PTR)descr );

    LISTBOX_update_uistate(descr);

/*    if (wnd->dwExStyle & WS_EX_NOPARENTNOTIFY) descr->style &= ~LBS_NOTIFY;
 */
    if (descr->style & LBS_EXTENDEDSEL) descr->style |= LBS_MULTIPLESEL;
    if (descr->style & LBS_MULTICOLUMN) descr->style &= ~LBS_OWNERDRAWVARIABLE;
    if (descr->style & LBS_OWNERDRAWVARIABLE) descr->style |= LBS_NOINTEGRALHEIGHT;
    descr->item_height = LISTBOX_SetFont( descr, 0 );

    if (descr->style & LBS_OWNERDRAWFIXED)
    {
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

    TRACE("owner: %p, style: %08x, width: %d, height: %d\n", descr->owner, descr->style, descr->width, descr->height);
    return TRUE;
}


/***********************************************************************
 *           LISTBOX_Destroy
 */
static BOOL LISTBOX_Destroy( LB_DESCR *descr )
{
    LISTBOX_ResetContent( descr );
    SetWindowLongPtrW( descr->self, 0, 0 );
    HeapFree( GetProcessHeap(), 0, descr );
    return TRUE;
}


/***********************************************************************
 *           ListBoxWndProc_common
 */
static LRESULT WINAPI ListBoxWndProc_common( HWND hwnd, UINT msg,
                                             WPARAM wParam, LPARAM lParam, BOOL unicode )
{
    LB_DESCR *descr = (LB_DESCR *)GetWindowLongPtrW( hwnd, 0 );
    LPHEADCOMBO lphc = 0;
    LRESULT ret;

    if (!descr)
    {
        if (!IsWindow(hwnd)) return 0;

        if (msg == WM_CREATE)
        {
	    CREATESTRUCTW *lpcs = (CREATESTRUCTW *)lParam;
	    if (lpcs->style & LBS_COMBOBOX) lphc = (LPHEADCOMBO)lpcs->lpCreateParams;
            if (!LISTBOX_Create( hwnd, lphc )) return -1;
            TRACE("creating wnd=%p descr=%lx\n", hwnd, GetWindowLongPtrW( hwnd, 0 ) );
            return 0;
        }
        /* Ignore all other messages before we get a WM_CREATE */
        return unicode ? DefWindowProcW( hwnd, msg, wParam, lParam ) :
                         DefWindowProcA( hwnd, msg, wParam, lParam );
    }
    if (descr->style & LBS_COMBOBOX) lphc = descr->lphc;

    TRACE("[%p]: msg %s wp %08lx lp %08lx\n",
          hwnd, SPY_GetMsgName(msg, hwnd), wParam, lParam );

    switch(msg)
    {
#ifndef __REACTOS__
    case LB_RESETCONTENT16:
#endif
    case LB_RESETCONTENT:
        LISTBOX_ResetContent( descr );
        LISTBOX_UpdateScroll( descr );
        NtUserInvalidateRect( descr->self, NULL, TRUE );
        return 0;

#ifndef __REACTOS__
    case LB_ADDSTRING16:
        if (HAS_STRINGS(descr)) lParam = (LPARAM)MapSL(lParam);
        /* fall through */
#endif
    case LB_ADDSTRING:
    case LB_ADDSTRING_LOWER:
    case LB_ADDSTRING_UPPER:
    {
        INT ret;
        LPWSTR textW;
        if(unicode || !HAS_STRINGS(descr))
            textW = (LPWSTR)lParam;
        else
        {
            LPSTR textA = (LPSTR)lParam;
            INT countW = MultiByteToWideChar(CP_ACP, 0, textA, -1, NULL, 0);
            if((textW = HeapAlloc(GetProcessHeap(), 0, countW * sizeof(WCHAR))))
                MultiByteToWideChar(CP_ACP, 0, textA, -1, textW, countW);
        }
        /* in the unicode the version, the string is really overwritten
           during the converting case */
        if (msg == LB_ADDSTRING_LOWER)
            strlwrW(textW);
        else if (msg == LB_ADDSTRING_UPPER)
            struprW(textW);
        wParam = LISTBOX_FindStringPos( descr, textW, FALSE );
        ret = LISTBOX_InsertString( descr, wParam, textW );
        if (!unicode && HAS_STRINGS(descr))
            HeapFree(GetProcessHeap(), 0, textW);
        return ret;
    }

#ifndef __REACTOS__
    case LB_INSERTSTRING16:
        if (HAS_STRINGS(descr)) lParam = (LPARAM)MapSL(lParam);
        wParam = (INT)(INT16)wParam;
        /* fall through */
#endif
    case LB_INSERTSTRING:
    case LB_INSERTSTRING_UPPER:
    case LB_INSERTSTRING_LOWER:
    {
        INT ret;
        LPWSTR textW;
        if(unicode || !HAS_STRINGS(descr))
            textW = (LPWSTR)lParam;
        else
        {
            LPSTR textA = (LPSTR)lParam;
            INT countW = MultiByteToWideChar(CP_ACP, 0, textA, -1, NULL, 0);
            if((textW = HeapAlloc(GetProcessHeap(), 0, countW * sizeof(WCHAR))))
                MultiByteToWideChar(CP_ACP, 0, textA, -1, textW, countW);
        }
        /* in the unicode the version, the string is really overwritten
           during the converting case */
        if (msg == LB_INSERTSTRING_LOWER)
            strlwrW(textW);
        else if (msg == LB_INSERTSTRING_UPPER)
            struprW(textW);
        ret = LISTBOX_InsertString( descr, wParam, textW );
        if(!unicode && HAS_STRINGS(descr))
            HeapFree(GetProcessHeap(), 0, textW);
        return ret;
    }

#ifndef __REACTOS__
    case LB_ADDFILE16:
        if (HAS_STRINGS(descr)) lParam = (LPARAM)MapSL(lParam);
        /* fall through */
#endif
    case LB_ADDFILE:
    {
        INT ret;
        LPWSTR textW;
        if(unicode || !HAS_STRINGS(descr))
            textW = (LPWSTR)lParam;
        else
        {
            LPSTR textA = (LPSTR)lParam;
            INT countW = MultiByteToWideChar(CP_ACP, 0, textA, -1, NULL, 0);
            if((textW = HeapAlloc(GetProcessHeap(), 0, countW * sizeof(WCHAR))))
                MultiByteToWideChar(CP_ACP, 0, textA, -1, textW, countW);
        }
        wParam = LISTBOX_FindFileStrPos( descr, textW );
        ret = LISTBOX_InsertString( descr, wParam, textW );
        if(!unicode && HAS_STRINGS(descr))
            HeapFree(GetProcessHeap(), 0, textW);
        return ret;
    }

#ifndef __REACTOS__
    case LB_DELETESTRING16:
#endif
    case LB_DELETESTRING:
        if (LISTBOX_RemoveItem( descr, wParam) != LB_ERR)
            return descr->nb_items;
        else
        {
            SetLastError(ERROR_INVALID_INDEX);
            return LB_ERR;
        }

#ifndef __REACTOS__
    case LB_GETITEMDATA16:
#endif
    case LB_GETITEMDATA:
        if (((INT)wParam < 0) || ((INT)wParam >= descr->nb_items))
        {
            SetLastError(ERROR_INVALID_INDEX);
            return LB_ERR;
        }
        return descr->items[wParam].data;

#ifndef __REACTOS__
    case LB_SETITEMDATA16:
#endif
    case LB_SETITEMDATA:
        if (((INT)wParam < 0) || ((INT)wParam >= descr->nb_items))
        {
            SetLastError(ERROR_INVALID_INDEX);
            return LB_ERR;
        }
        descr->items[wParam].data = lParam;
        /* undocumented: returns TRUE, not LB_OKAY (0) */
        return TRUE;

#ifndef __REACTOS__
    case LB_GETCOUNT16:
#endif
    case LB_GETCOUNT:
        return descr->nb_items;

#ifndef __REACTOS__
    case LB_GETTEXT16:
        lParam = (LPARAM)MapSL(lParam);
        /* fall through */
#endif
    case LB_GETTEXT:
        return LISTBOX_GetText( descr, wParam, (LPWSTR)lParam, unicode );

#ifndef __REACTOS__
    case LB_GETTEXTLEN16:
        /* fall through */
#endif
    case LB_GETTEXTLEN:
        if ((INT)wParam >= descr->nb_items || (INT)wParam < 0)
        {
            SetLastError(ERROR_INVALID_INDEX);
            return LB_ERR;
        }
        if (!HAS_STRINGS(descr)) return sizeof(DWORD);
        if (unicode) return strlenW( descr->items[wParam].str );
        return WideCharToMultiByte( CP_ACP, 0, descr->items[wParam].str,
                                    strlenW(descr->items[wParam].str), NULL, 0, NULL, NULL );

#ifndef __REACTOS__
    case LB_GETCURSEL16:
#endif
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
#ifndef __REACTOS__
    case LB_GETTOPINDEX16:
#endif
    case LB_GETTOPINDEX:
        return descr->top_item;

#ifndef __REACTOS__
    case LB_GETITEMHEIGHT16:
#endif
    case LB_GETITEMHEIGHT:
        return LISTBOX_GetItemHeight( descr, wParam );

#ifndef __REACTOS__
    case LB_SETITEMHEIGHT16:
        lParam = LOWORD(lParam);
        /* fall through */
#endif
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

#ifndef __REACTOS__
    case LB_SETCARETINDEX16:
#endif
    case LB_SETCARETINDEX:
        if ((!IS_MULTISELECT(descr)) && (descr->selected_item != -1)) return LB_ERR;
        if (LISTBOX_SetCaretIndex( descr, wParam, !lParam ) == LB_ERR)
            return LB_ERR;
        else if (ISWIN31)
             return wParam;
        else
             return LB_OKAY;

#ifndef __REACTOS__
    case LB_GETCARETINDEX16:
#endif
    case LB_GETCARETINDEX:
        return descr->focus_item;

#ifndef __REACTOS__
    case LB_SETTOPINDEX16:
#endif
    case LB_SETTOPINDEX:
        return LISTBOX_SetTopItem( descr, wParam, TRUE );

#ifndef __REACTOS__
    case LB_SETCOLUMNWIDTH16:
#endif
    case LB_SETCOLUMNWIDTH:
        return LISTBOX_SetColumnWidth( descr, wParam );

#ifndef __REACTOS__
    case LB_GETITEMRECT16:
        {
            RECT rect;
            ret = LISTBOX_GetItemRect( descr, (INT16)wParam, &rect );
            CONV_RECT32TO16( &rect, MapSL(lParam) );
        }
	return ret;
#endif

    case LB_GETITEMRECT:
        return LISTBOX_GetItemRect( descr, wParam, (RECT *)lParam );

#ifndef __REACTOS__
    case LB_FINDSTRING16:
        wParam = (INT)(INT16)wParam;
        if (HAS_STRINGS(descr)) lParam = (LPARAM)MapSL(lParam);
        /* fall through */
#endif
    case LB_FINDSTRING:
    {
        INT ret;
        LPWSTR textW;
        if(unicode || !HAS_STRINGS(descr))
            textW = (LPWSTR)lParam;
        else
        {
            LPSTR textA = (LPSTR)lParam;
            INT countW = MultiByteToWideChar(CP_ACP, 0, textA, -1, NULL, 0);
            if((textW = HeapAlloc(GetProcessHeap(), 0, countW * sizeof(WCHAR))))
                MultiByteToWideChar(CP_ACP, 0, textA, -1, textW, countW);
        }
        ret = LISTBOX_FindString( descr, wParam, textW, FALSE );
        if(!unicode && HAS_STRINGS(descr))
            HeapFree(GetProcessHeap(), 0, textW);
        return ret;
    }

#ifndef __REACTOS__
    case LB_FINDSTRINGEXACT16:
        wParam = (INT)(INT16)wParam;
        if (HAS_STRINGS(descr)) lParam = (LPARAM)MapSL(lParam);
        /* fall through */
#endif
    case LB_FINDSTRINGEXACT:
    {
        INT ret;
        LPWSTR textW;
        if(unicode || !HAS_STRINGS(descr))
            textW = (LPWSTR)lParam;
        else
        {
            LPSTR textA = (LPSTR)lParam;
            INT countW = MultiByteToWideChar(CP_ACP, 0, textA, -1, NULL, 0);
            if((textW = HeapAlloc(GetProcessHeap(), 0, countW * sizeof(WCHAR))))
                MultiByteToWideChar(CP_ACP, 0, textA, -1, textW, countW);
        }
        ret = LISTBOX_FindString( descr, wParam, textW, TRUE );
        if(!unicode && HAS_STRINGS(descr))
            HeapFree(GetProcessHeap(), 0, textW);
        return ret;
    }

#ifndef __REACTOS__
    case LB_SELECTSTRING16:
        wParam = (INT)(INT16)wParam;
        if (HAS_STRINGS(descr)) lParam = (LPARAM)MapSL(lParam);
        /* fall through */
#endif
    case LB_SELECTSTRING:
    {
        INT index;
        LPWSTR textW;

	if(HAS_STRINGS(descr))
	    TRACE("LB_SELECTSTRING: %s\n", unicode ? debugstr_w((LPWSTR)lParam) :
						     debugstr_a((LPSTR)lParam));
        if(unicode || !HAS_STRINGS(descr))
            textW = (LPWSTR)lParam;
        else
        {
            LPSTR textA = (LPSTR)lParam;
            INT countW = MultiByteToWideChar(CP_ACP, 0, textA, -1, NULL, 0);
            if((textW = HeapAlloc(GetProcessHeap(), 0, countW * sizeof(WCHAR))))
                MultiByteToWideChar(CP_ACP, 0, textA, -1, textW, countW);
        }
        index = LISTBOX_FindString( descr, wParam, textW, FALSE );
        if(!unicode && HAS_STRINGS(descr))
            HeapFree(GetProcessHeap(), 0, textW);
        if (index != LB_ERR)
	{
            LISTBOX_MoveCaret( descr, index, TRUE );
            LISTBOX_SetSelection( descr, index, TRUE, FALSE );
	}
        return index;
    }

#ifndef __REACTOS__
    case LB_GETSEL16:
        wParam = (INT)(INT16)wParam;
        /* fall through */
#endif
    case LB_GETSEL:
        if (((INT)wParam < 0) || ((INT)wParam >= descr->nb_items))
            return LB_ERR;
        return descr->items[wParam].selected;

#ifndef __REACTOS__
    case LB_SETSEL16:
        lParam = (INT)(INT16)lParam;
        /* fall through */
#endif
    case LB_SETSEL:
        return LISTBOX_SetSelection( descr, lParam, wParam, FALSE );

#ifndef __REACTOS__
    case LB_SETCURSEL16:
        wParam = (INT)(INT16)wParam;
        /* fall through */
#endif
    case LB_SETCURSEL:
        if (IS_MULTISELECT(descr)) return LB_ERR;
        LISTBOX_SetCaretIndex( descr, wParam, FALSE );
        ret = LISTBOX_SetSelection( descr, wParam, TRUE, FALSE );
        if (ret != LB_ERR) ret = descr->selected_item;
        return ret;

#ifndef __REACTOS__
    case LB_GETSELCOUNT16:
#endif
    case LB_GETSELCOUNT:
        return LISTBOX_GetSelCount( descr );

#ifndef __REACTOS__
    case LB_GETSELITEMS16:
        return LISTBOX_GetSelItems16( descr, wParam, (LPINT16)MapSL(lParam) );
#endif

    case LB_GETSELITEMS:
        return LISTBOX_GetSelItems( descr, wParam, (LPINT)lParam );

#ifndef __REACTOS__
    case LB_SELITEMRANGE16:
#endif
    case LB_SELITEMRANGE:
        if (LOWORD(lParam) <= HIWORD(lParam))
            return LISTBOX_SelectItemRange( descr, LOWORD(lParam),
                                            HIWORD(lParam), wParam );
        else
            return LISTBOX_SelectItemRange( descr, HIWORD(lParam),
                                            LOWORD(lParam), wParam );

#ifndef __REACTOS__
    case LB_SELITEMRANGEEX16:
#endif
    case LB_SELITEMRANGEEX:
        if ((INT)lParam >= (INT)wParam)
            return LISTBOX_SelectItemRange( descr, wParam, lParam, TRUE );
        else
            return LISTBOX_SelectItemRange( descr, lParam, wParam, FALSE);

#ifndef __REACTOS__
    case LB_GETHORIZONTALEXTENT16:
#endif
    case LB_GETHORIZONTALEXTENT:
        return descr->horz_extent;

#ifndef __REACTOS__
    case LB_SETHORIZONTALEXTENT16:
#endif
    case LB_SETHORIZONTALEXTENT:
        return LISTBOX_SetHorizontalExtent( descr, wParam );

#ifndef __REACTOS__
    case LB_GETANCHORINDEX16:
#endif
    case LB_GETANCHORINDEX:
        return descr->anchor_item;

#ifndef __REACTOS__
    case LB_SETANCHORINDEX16:
        wParam = (INT)(INT16)wParam;
        /* fall through */
#endif
    case LB_SETANCHORINDEX:
        if (((INT)wParam < -1) || ((INT)wParam >= descr->nb_items))
        {
            SetLastError(ERROR_INVALID_INDEX);
            return LB_ERR;
        }
        descr->anchor_item = (INT)wParam;
        return LB_OKAY;

#ifndef __REACTOS__
    case LB_DIR16:
        /* according to Win16 docs, DDL_DRIVES should make DDL_EXCLUSIVE
         * be set automatically (this is different in Win32) */
        if (wParam & DDL_DRIVES) wParam |= DDL_EXCLUSIVE;
            lParam = (LPARAM)MapSL(lParam);
        /* fall through */
#endif
    case LB_DIR:
    {
        INT ret;
        LPWSTR textW;
        if(unicode)
            textW = (LPWSTR)lParam;
        else
        {
            LPSTR textA = (LPSTR)lParam;
            INT countW = MultiByteToWideChar(CP_ACP, 0, textA, -1, NULL, 0);
            if((textW = HeapAlloc(GetProcessHeap(), 0, countW * sizeof(WCHAR))))
                MultiByteToWideChar(CP_ACP, 0, textA, -1, textW, countW);
        }
        ret = LISTBOX_Directory( descr, wParam, textW, msg == LB_DIR );
        if(!unicode)
            HeapFree(GetProcessHeap(), 0, textW);
        return ret;
    }

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

#ifndef __REACTOS__
    case LB_SETTABSTOPS16:
        return LISTBOX_SetTabStops( descr, (INT)(INT16)wParam, MapSL(lParam), TRUE );
#endif

    case LB_SETTABSTOPS:
        return LISTBOX_SetTabStops( descr, wParam, (LPINT)lParam, FALSE );

#ifndef __REACTOS__
    case LB_CARETON16:
#endif
    case LB_CARETON:
        if (descr->caret_on)
            return LB_OKAY;
        descr->caret_on = TRUE;
        if ((descr->focus_item != -1) && (descr->in_focus))
            LISTBOX_RepaintItem( descr, descr->focus_item, ODA_FOCUS );
        return LB_OKAY;

#ifndef __REACTOS__
    case LB_CARETOFF16:
#endif
    case LB_CARETOFF:
        if (!descr->caret_on)
            return LB_OKAY;
        descr->caret_on = FALSE;
        if ((descr->focus_item != -1) && (descr->in_focus))
            LISTBOX_RepaintItem( descr, descr->focus_item, ODA_FOCUS );
        return LB_OKAY;

    case LB_GETLISTBOXINFO:
        FIXME("LB_GETLISTBOXINFO: stub!\n");
        return 0;

    case WM_DESTROY:
        return LISTBOX_Destroy( descr );

    case WM_ENABLE:
        NtUserInvalidateRect( hwnd, NULL, TRUE );
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
            if( !wParam ) EndPaint( hwnd, &ps );
        }
        return ret;
    case WM_SIZE:
        LISTBOX_UpdateSize( descr );
        return 0;
    case WM_GETFONT:
        return (LRESULT)descr->font;
    case WM_SETFONT:
        LISTBOX_SetFont( descr, (HFONT)wParam );
        if (lParam) NtUserInvalidateRect( hwnd, 0, TRUE );
        return 0;
    case WM_SETFOCUS:
        descr->in_focus = TRUE;
        descr->caret_on = TRUE;
        if (descr->focus_item != -1)
            LISTBOX_DrawFocusRect( descr, TRUE );
        SEND_NOTIFICATION( descr, LBN_SETFOCUS );
        return 0;
    case WM_KILLFOCUS:
        descr->in_focus = FALSE;
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
            return DefWindowProcW( hwnd, msg, wParam, lParam );
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

            GetClientRect(hwnd, &clientRect);

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
    {
        WCHAR charW;
        if(unicode)
            charW = (WCHAR)wParam;
        else
        {
            CHAR charA = (CHAR)wParam;
            MultiByteToWideChar(CP_ACP, 0, &charA, 1, &charW, 1);
        }
        return LISTBOX_HandleChar( descr, charW );
    }
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
        return unicode ? SendMessageW( descr->owner, msg, wParam, lParam ) :
                         SendMessageA( descr->owner, msg, wParam, lParam );

    case WM_NCDESTROY:
        if( lphc && (lphc->dwStyle & CBS_DROPDOWNLIST) != CBS_SIMPLE )
            lphc->hWndLBox = 0;
        break;

    case WM_NCACTIVATE:
        if (lphc) return 0;
	break;

    case WM_UPDATEUISTATE:
        if (unicode)
            DefWindowProcW(descr->self, msg, wParam, lParam);
        else
            DefWindowProcA(descr->self, msg, wParam, lParam);

        if (LISTBOX_update_uistate(descr))
        {
           /* redraw text */
           if (descr->focus_item != -1)
               LISTBOX_DrawFocusRect( descr, descr->in_focus );
        }
        break;

    default:
        if ((msg >= WM_USER) && (msg < 0xc000))
            WARN("[%p]: unknown msg %04x wp %08lx lp %08lx\n",
                 hwnd, msg, wParam, lParam );
        break;
    }

    return unicode ? DefWindowProcW( hwnd, msg, wParam, lParam ) :
                     DefWindowProcA( hwnd, msg, wParam, lParam );
}

/***********************************************************************
 *           ListBoxWndProcA
 *
 * This is just a wrapper for the real wndproc, it only does window locking
 * and unlocking.
 */
static LRESULT WINAPI ListBoxWndProcA( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    return ListBoxWndProc_common( hwnd, msg, wParam, lParam, FALSE );
}

/***********************************************************************
 *           ListBoxWndProcW
 */
static LRESULT WINAPI ListBoxWndProcW( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    return ListBoxWndProc_common( hwnd, msg, wParam, lParam, TRUE );
}

/***********************************************************************
 *           GetListBoxInfo !REACTOS!
 */
DWORD STDCALL
GetListBoxInfo(HWND hwnd)
{
  return NtUserGetListBoxInfo(hwnd); // Do it right! Have the message org from kmode!
}

