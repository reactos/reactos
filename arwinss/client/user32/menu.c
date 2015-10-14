/*
 * Menu functions
 *
 * Copyright 1993 Martin Ayotte
 * Copyright 1994 Alexandre Julliard
 * Copyright 1997 Morten Welinder
 * Copyright 2005 Maxime Bellengé
 * Copyright 2006 Phil Krylov
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

/*
 * Note: the style MF_MOUSESELECT is used to mark popup items that
 * have been selected, i.e. their popup menu is currently displayed.
 * This is probably not the meaning this style has in MS-Windows.
 *
 * Note 2: where there is a difference, these menu API's are according
 * the behavior of Windows 2k and Windows XP. Known differences with
 * Windows 9x/ME are documented in the comments, in case an application
 * is found to depend on the old behavior.
 * 
 * TODO:
 *    implements styles :
 *        - MNS_AUTODISMISS
 *        - MNS_DRAGDROP
 *        - MNS_MODELESS
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <string.h>

#define OEMRESOURCE

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winnls.h"
#include "wine/winternl.h"
#include "wine/server.h"
#include "wine/unicode.h"
#include "wine/exception.h"
#include "win.h"
#include "controls.h"
#include "user_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(menu);
WINE_DECLARE_DEBUG_CHANNEL(accel);

/* internal popup menu window messages */

#define MM_SETMENUHANDLE	(WM_USER + 0)
#define MM_GETMENUHANDLE	(WM_USER + 1)

/* Menu item structure */
typedef struct {
    /* ----------- MENUITEMINFO Stuff ----------- */
    UINT fType;			/* Item type. */
    UINT fState;		/* Item state.  */
    UINT_PTR wID;		/* Item id.  */
    HMENU hSubMenu;		/* Pop-up menu.  */
    HBITMAP hCheckBit;		/* Bitmap when checked.  */
    HBITMAP hUnCheckBit;	/* Bitmap when unchecked.  */
    LPWSTR text;		/* Item text. */
    ULONG_PTR dwItemData;	/* Application defined.  */
    LPWSTR dwTypeData;		/* depends on fMask */
    HBITMAP hbmpItem;		/* bitmap */
    /* ----------- Wine stuff ----------- */
    RECT      rect;		/* Item area (relative to menu window) */
    UINT      xTab;		/* X position of text after Tab */
    SIZE   bmpsize;             /* size needed for the HBMMENU_CALLBACK
                                 * bitmap */ 
} MENUITEM;

/* Popup menu structure */
typedef struct {
    struct user_object obj;
    WORD        wFlags;       /* Menu flags (MF_POPUP, MF_SYSMENU) */
    WORD	Width;        /* Width of the whole menu */
    WORD	Height;       /* Height of the whole menu */
    UINT        nItems;       /* Number of items in the menu */
    HWND        hWnd;         /* Window containing the menu */
    MENUITEM    *items;       /* Array of menu items */
    UINT        FocusedItem;  /* Currently focused item */
    HWND	hwndOwner;    /* window receiving the messages for ownerdraw */
    BOOL        bTimeToHide;  /* Request hiding when receiving a second click in the top-level menu item */
    BOOL        bScrolling;   /* Scroll arrows are active */
    UINT        nScrollPos;   /* Current scroll position */
    UINT        nTotalHeight; /* Total height of menu items inside menu */
    /* ------------ MENUINFO members ------ */
    DWORD	dwStyle;	/* Extended menu style */
    UINT	cyMax;		/* max height of the whole menu, 0 is screen height */
    HBRUSH	hbrBack;	/* brush for menu background */
    DWORD	dwContextHelpID;
    DWORD	dwMenuData;	/* application defined value */
    HMENU       hSysMenuOwner;  /* Handle to the dummy sys menu holder */
    WORD        textOffset;     /* Offset of text when items have both bitmaps and text */
} POPUPMENU, *LPPOPUPMENU;

/* internal flags for menu tracking */

#define TF_ENDMENU              0x10000
#define TF_SUSPENDPOPUP         0x20000
#define TF_SKIPREMOVE           0x40000

typedef struct
{
    UINT	trackFlags;
    HMENU	hCurrentMenu; /* current submenu (can be equal to hTopMenu)*/
    HMENU	hTopMenu;     /* initial menu */
    HWND	hOwnerWnd;    /* where notifications are sent */
    POINT	pt;
} MTRACKER;

#define MENU_MAGIC   0x554d  /* 'MU' */

#define ITEM_PREV		-1
#define ITEM_NEXT		 1

  /* Internal MENU_TrackMenu() flags */
#define TPM_INTERNAL		0xF0000000
#define TPM_BUTTONDOWN		0x40000000		/* menu was clicked before tracking */
#define TPM_POPUPMENU           0x20000000              /* menu is a popup menu */

  /* Space between 2 columns */
#define MENU_COL_SPACE 4

  /*  top and bottom margins for popup menus */
#define MENU_TOP_MARGIN 3
#define MENU_BOTTOM_MARGIN 2

/* maximum allowed depth of any branch in the menu tree.
 * This value is slightly larger than in windows (25) to
 * stay on the safe side. */
#define MAXMENUDEPTH 30

  /* (other menu->FocusedItem values give the position of the focused item) */
#define NO_SELECTED_ITEM  0xffff

#define MENU_ITEM_TYPE(flags) \
  ((flags) & (MF_STRING | MF_BITMAP | MF_OWNERDRAW | MF_SEPARATOR))

/* macro to test that flags do not indicate bitmap, ownerdraw or separator */
#define IS_STRING_ITEM(flags) (MENU_ITEM_TYPE ((flags)) == MF_STRING)
#define IS_MAGIC_BITMAP(id)     ((id) && ((INT_PTR)(id) < 12) && ((INT_PTR)(id) >= -1))

#define IS_SYSTEM_MENU(menu)  \
	(!((menu)->wFlags & MF_POPUP) && ((menu)->wFlags & MF_SYSMENU))

#define MENUITEMINFO_TYPE_MASK \
		(MFT_STRING | MFT_BITMAP | MFT_OWNERDRAW | MFT_SEPARATOR | \
		MFT_MENUBARBREAK | MFT_MENUBREAK | MFT_RADIOCHECK | \
		MFT_RIGHTORDER | MFT_RIGHTJUSTIFY /* same as MF_HELP */ )
#define TYPE_MASK  (MENUITEMINFO_TYPE_MASK | MF_POPUP | MF_SYSMENU)
#define STATE_MASK (~TYPE_MASK)
#define MENUITEMINFO_STATE_MASK (STATE_MASK & ~(MF_BYPOSITION | MF_MOUSESELECT))

#define WIN_ALLOWED_MENU(style) ((style & (WS_CHILD | WS_POPUP)) != WS_CHILD)

static SIZE     menucharsize;
static UINT     ODitemheight; /* default owner drawn item height */      

/* Use global popup window because there's no way 2 menus can
 * be tracked at the same time.  */
static HWND top_popup;
static HMENU top_popup_hmenu;

  /* Flag set by EndMenu() to force an exit from menu tracking */
static BOOL fEndMenu = FALSE;

DWORD WINAPI DrawMenuBarTemp(HWND hwnd, HDC hDC, LPRECT lprect, HMENU hMenu, HFONT hFont);

static BOOL SetMenuItemInfo_common( MENUITEM *, const MENUITEMINFOW *, BOOL);

/*********************************************************************
 * menu class descriptor
 */
const struct builtin_class_descr MENU_builtin_class =
{
    (LPCWSTR)POPUPMENU_CLASS_ATOM,  /* name */
    CS_DROPSHADOW | CS_SAVEBITS | CS_DBLCLKS,  /* style */
    WINPROC_MENU,                  /* proc */
    sizeof(HMENU),                 /* extra */
    IDC_ARROW,                     /* cursor */
    (HBRUSH)(COLOR_MENU+1)         /* brush */
};


/***********************************************************************
 *           debug_print_menuitem
 *
 * Print a menuitem in readable form.
 */

#define debug_print_menuitem(pre, mp, post) \
    do { if (TRACE_ON(menu)) do_debug_print_menuitem(pre, mp, post); } while (0)

#define MENUOUT(text) \
  TRACE("%s%s", (count++ ? "," : ""), (text))

#define MENUFLAG(bit,text) \
  do { \
    if (flags & (bit)) { flags &= ~(bit); MENUOUT ((text)); } \
  } while (0)

static void do_debug_print_menuitem(const char *prefix, const MENUITEM *mp,
				    const char *postfix)
{
    static const char * const hbmmenus[] = { "HBMMENU_CALLBACK", "", "HBMMENU_SYSTEM",
    "HBMMENU_MBAR_RESTORE", "HBMMENU_MBAR_MINIMIZE", "UNKNOWN BITMAP", "HBMMENU_MBAR_CLOSE",
    "HBMMENU_MBAR_CLOSE_D", "HBMMENU_MBAR_MINIMIZE_D", "HBMMENU_POPUP_CLOSE",
    "HBMMENU_POPUP_RESTORE", "HBMMENU_POPUP_MAXIMIZE", "HBMMENU_POPUP_MINIMIZE"};
    TRACE("%s ", prefix);
    if (mp) {
        UINT flags = mp->fType;
        TRACE( "{ ID=0x%lx", mp->wID);
        if ( mp->hSubMenu)
            TRACE( ", Sub=%p", mp->hSubMenu);
        if (flags) {
            int count = 0;
            TRACE( ", fType=");
            MENUFLAG( MFT_SEPARATOR, "sep");
            MENUFLAG( MFT_OWNERDRAW, "own");
            MENUFLAG( MFT_BITMAP, "bit");
            MENUFLAG(MF_POPUP, "pop");
            MENUFLAG(MFT_MENUBARBREAK, "barbrk");
            MENUFLAG(MFT_MENUBREAK, "brk");
            MENUFLAG(MFT_RADIOCHECK, "radio");
            MENUFLAG(MFT_RIGHTORDER, "rorder");
            MENUFLAG(MF_SYSMENU, "sys");
            MENUFLAG(MFT_RIGHTJUSTIFY, "right");  /* same as MF_HELP */
            if (flags)
                TRACE( "+0x%x", flags);
        }
        flags = mp->fState;
        if (flags) {
            int count = 0;
            TRACE( ", State=");
            MENUFLAG(MFS_GRAYED, "grey");
            MENUFLAG(MFS_DEFAULT, "default");
            MENUFLAG(MFS_DISABLED, "dis");
            MENUFLAG(MFS_CHECKED, "check");
            MENUFLAG(MFS_HILITE, "hi");
            MENUFLAG(MF_USECHECKBITMAPS, "usebit");
            MENUFLAG(MF_MOUSESELECT, "mouse");
            if (flags)
                TRACE( "+0x%x", flags);
        }
        if (mp->hCheckBit)
            TRACE( ", Chk=%p", mp->hCheckBit);
        if (mp->hUnCheckBit)
            TRACE( ", Unc=%p", mp->hUnCheckBit);
        if (mp->text)
            TRACE( ", Text=%s", debugstr_w(mp->text));
        if (mp->dwItemData)
            TRACE( ", ItemData=0x%08lx", mp->dwItemData);
        if (mp->hbmpItem)
        {
            if( IS_MAGIC_BITMAP(mp->hbmpItem))
                TRACE( ", hbitmap=%s", hbmmenus[ (INT_PTR)mp->hbmpItem + 1]);
            else
                TRACE( ", hbitmap=%p", mp->hbmpItem);
        }
        TRACE( " }");
    } else
        TRACE( "NULL");
    TRACE(" %s\n", postfix);
}

#undef MENUOUT
#undef MENUFLAG


/***********************************************************************
 *           MENU_GetMenu
 *
 * Validate the given menu handle and returns the menu structure pointer.
 */
static POPUPMENU *MENU_GetMenu(HMENU hMenu)
{
    POPUPMENU *menu = get_user_handle_ptr( hMenu, USER_MENU );

    if (menu == OBJ_OTHER_PROCESS)
    {
        WARN( "other process menu %p?\n", hMenu);
        return NULL;
    }
    if (menu) release_user_handle_ptr( menu );  /* FIXME! */
    else WARN("invalid menu handle=%p\n", hMenu);
    return menu;
}

/***********************************************************************
 *           get_win_sys_menu
 *
 * Get the system menu of a window
 */
static HMENU get_win_sys_menu( HWND hwnd )
{
    HMENU ret = 0;
    WND *win = WIN_GetPtr( hwnd );
    if (win && win != WND_OTHER_PROCESS && win != WND_DESKTOP)
    {
        ret = win->hSysMenu;
        WIN_ReleasePtr( win );
    }
    return ret;
}

/***********************************************************************
 *           get_menu_font
 */
static HFONT get_menu_font( BOOL bold )
{
    static HFONT hMenuFont, hMenuFontBold;

    HFONT ret = bold ? hMenuFontBold : hMenuFont;

    if (!ret)
    {
        NONCLIENTMETRICSW ncm;
        HFONT prev;

        ncm.cbSize = sizeof(NONCLIENTMETRICSW);
        SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICSW), &ncm, 0);

        if (bold)
        {
            ncm.lfMenuFont.lfWeight += 300;
            if (ncm.lfMenuFont.lfWeight > 1000) ncm.lfMenuFont.lfWeight = 1000;
        }
        if (!(ret = CreateFontIndirectW( &ncm.lfMenuFont ))) return 0;
        prev = InterlockedCompareExchangePointer( (void **)(bold ? &hMenuFontBold : &hMenuFont),
                                                  ret, NULL );
        if (prev)
        {
            /* another thread beat us to it */
            DeleteObject( ret );
            ret = prev;
        }
    }
    return ret;
}

/***********************************************************************
 *           get_arrow_bitmap
 */
static HBITMAP get_arrow_bitmap(void)
{
    static HBITMAP arrow_bitmap;

    if (!arrow_bitmap) arrow_bitmap = LoadBitmapW(0, MAKEINTRESOURCEW(OBM_MNARROW));
    return arrow_bitmap;
}

/***********************************************************************
 *           get_down_arrow_bitmap
 */
static HBITMAP get_down_arrow_bitmap(void)
{
    static HBITMAP arrow_bitmap;

    if (!arrow_bitmap) arrow_bitmap = LoadBitmapW(0, MAKEINTRESOURCEW(OBM_DNARROW));
    return arrow_bitmap;
}

/***********************************************************************
 *           get_down_arrow_inactive_bitmap
 */
static HBITMAP get_down_arrow_inactive_bitmap(void)
{
    static HBITMAP arrow_bitmap;

    if (!arrow_bitmap) arrow_bitmap = LoadBitmapW(0, MAKEINTRESOURCEW(OBM_DNARROWI));
    return arrow_bitmap;
}

/***********************************************************************
 *           get_up_arrow_bitmap
 */
static HBITMAP get_up_arrow_bitmap(void)
{
    static HBITMAP arrow_bitmap;

    if (!arrow_bitmap) arrow_bitmap = LoadBitmapW(0, MAKEINTRESOURCEW(OBM_UPARROW));
    return arrow_bitmap;
}

/***********************************************************************
 *           get_up_arrow_inactive_bitmap
 */
static HBITMAP get_up_arrow_inactive_bitmap(void)
{
    static HBITMAP arrow_bitmap;

    if (!arrow_bitmap) arrow_bitmap = LoadBitmapW(0, MAKEINTRESOURCEW(OBM_UPARROWI));
    return arrow_bitmap;
}

/***********************************************************************
 *           MENU_CopySysPopup
 *
 * Return the default system menu.
 */
static HMENU MENU_CopySysPopup(void)
{
    static const WCHAR sysmenuW[] = {'S','Y','S','M','E','N','U',0};
    HMENU hMenu = LoadMenuW(user32_module, sysmenuW);

    if( hMenu ) {
        MENUINFO minfo;
        MENUITEMINFOW miteminfo;
        POPUPMENU* menu = MENU_GetMenu(hMenu);
        menu->wFlags |= MF_SYSMENU | MF_POPUP;
        /* decorate the menu with bitmaps */
        minfo.cbSize = sizeof( MENUINFO);
        minfo.dwStyle = MNS_CHECKORBMP;
        minfo.fMask = MIM_STYLE;
        SetMenuInfo( hMenu, &minfo);
        miteminfo.cbSize = sizeof( MENUITEMINFOW);
        miteminfo.fMask = MIIM_BITMAP;
        miteminfo.hbmpItem = HBMMENU_POPUP_CLOSE;
        SetMenuItemInfoW( hMenu, SC_CLOSE, FALSE, &miteminfo);
        miteminfo.hbmpItem = HBMMENU_POPUP_RESTORE;
        SetMenuItemInfoW( hMenu, SC_RESTORE, FALSE, &miteminfo);
        miteminfo.hbmpItem = HBMMENU_POPUP_MAXIMIZE;
        SetMenuItemInfoW( hMenu, SC_MAXIMIZE, FALSE, &miteminfo);
        miteminfo.hbmpItem = HBMMENU_POPUP_MINIMIZE;
        SetMenuItemInfoW( hMenu, SC_MINIMIZE, FALSE, &miteminfo);
	SetMenuDefaultItem(hMenu, SC_CLOSE, FALSE);
    }
    else
	ERR("Unable to load default system menu\n" );

    TRACE("returning %p.\n", hMenu );

    return hMenu;
}


/**********************************************************************
 *           MENU_GetSysMenu
 *
 * Create a copy of the system menu. System menu in Windows is
 * a special menu bar with the single entry - system menu popup.
 * This popup is presented to the outside world as a "system menu".
 * However, the real system menu handle is sometimes seen in the
 * WM_MENUSELECT parameters (and Word 6 likes it this way).
 */
static HMENU MENU_GetSysMenu( HWND hWnd, HMENU hPopupMenu )
{
    HMENU hMenu;

    TRACE("loading system menu, hWnd %p, hPopupMenu %p\n", hWnd, hPopupMenu);
    if ((hMenu = CreateMenu()))
    {
	POPUPMENU *menu = MENU_GetMenu(hMenu);
	menu->wFlags = MF_SYSMENU;
	menu->hWnd = WIN_GetFullHandle( hWnd );
	TRACE("hWnd %p (hMenu %p)\n", menu->hWnd, hMenu);

	if (!hPopupMenu)
	    hPopupMenu = MENU_CopySysPopup();

	if (hPopupMenu)
	{
            if (GetClassLongW(hWnd, GCL_STYLE) & CS_NOCLOSE)
                DeleteMenu(hPopupMenu, SC_CLOSE, MF_BYCOMMAND);

	    InsertMenuW( hMenu, -1, MF_SYSMENU | MF_POPUP | MF_BYPOSITION,
                         (UINT_PTR)hPopupMenu, NULL );

            menu->items[0].fType = MF_SYSMENU | MF_POPUP;
            menu->items[0].fState = 0;
            if ((menu = MENU_GetMenu(hPopupMenu))) menu->wFlags |= MF_SYSMENU;

	    TRACE("hMenu=%p (hPopup %p)\n", hMenu, hPopupMenu );
	    return hMenu;
	}
	DestroyMenu( hMenu );
    }
    ERR("failed to load system menu!\n");
    return 0;
}


/***********************************************************************
 *           MENU_InitSysMenuPopup
 *
 * Grey the appropriate items in System menu.
 */
static void MENU_InitSysMenuPopup( HMENU hmenu, DWORD style, DWORD clsStyle )
{
    BOOL gray;

    gray = !(style & WS_THICKFRAME) || (style & (WS_MAXIMIZE | WS_MINIMIZE));
    EnableMenuItem( hmenu, SC_SIZE, (gray ? MF_GRAYED : MF_ENABLED) );
    gray = ((style & WS_MAXIMIZE) != 0);
    EnableMenuItem( hmenu, SC_MOVE, (gray ? MF_GRAYED : MF_ENABLED) );
    gray = !(style & WS_MINIMIZEBOX) || (style & WS_MINIMIZE);
    EnableMenuItem( hmenu, SC_MINIMIZE, (gray ? MF_GRAYED : MF_ENABLED) );
    gray = !(style & WS_MAXIMIZEBOX) || (style & WS_MAXIMIZE);
    EnableMenuItem( hmenu, SC_MAXIMIZE, (gray ? MF_GRAYED : MF_ENABLED) );
    gray = !(style & (WS_MAXIMIZE | WS_MINIMIZE));
    EnableMenuItem( hmenu, SC_RESTORE, (gray ? MF_GRAYED : MF_ENABLED) );
    gray = (clsStyle & CS_NOCLOSE) != 0;

    /* The menu item must keep its state if it's disabled */
    if(gray)
	EnableMenuItem( hmenu, SC_CLOSE, MF_GRAYED);
}


/******************************************************************************
 *
 *   UINT  MENU_GetStartOfNextColumn(
 *     HMENU  hMenu )
 *
 *****************************************************************************/

static UINT  MENU_GetStartOfNextColumn(
    HMENU  hMenu )
{
    POPUPMENU *menu = MENU_GetMenu(hMenu);
    UINT i;

    if(!menu)
	return NO_SELECTED_ITEM;

    i = menu->FocusedItem + 1;
    if( i == NO_SELECTED_ITEM )
	return i;

    for( ; i < menu->nItems; ++i ) {
	if (menu->items[i].fType & (MF_MENUBREAK | MF_MENUBARBREAK))
	    return i;
    }

    return NO_SELECTED_ITEM;
}


/******************************************************************************
 *
 *   UINT  MENU_GetStartOfPrevColumn(
 *     HMENU  hMenu )
 *
 *****************************************************************************/

static UINT  MENU_GetStartOfPrevColumn(
    HMENU  hMenu )
{
    POPUPMENU *menu = MENU_GetMenu(hMenu);
    UINT  i;

    if( !menu )
	return NO_SELECTED_ITEM;

    if( menu->FocusedItem == 0 || menu->FocusedItem == NO_SELECTED_ITEM )
	return NO_SELECTED_ITEM;

    /* Find the start of the column */

    for(i = menu->FocusedItem; i != 0 &&
	 !(menu->items[i].fType & (MF_MENUBREAK | MF_MENUBARBREAK));
	--i); /* empty */

    if(i == 0)
	return NO_SELECTED_ITEM;

    for(--i; i != 0; --i) {
	if (menu->items[i].fType & (MF_MENUBREAK | MF_MENUBARBREAK))
	    break;
    }

    TRACE("ret %d.\n", i );

    return i;
}



/***********************************************************************
 *           MENU_FindItem
 *
 * Find a menu item. Return a pointer on the item, and modifies *hmenu
 * in case the item was in a sub-menu.
 */
static MENUITEM *MENU_FindItem( HMENU *hmenu, UINT *nPos, UINT wFlags )
{
    POPUPMENU *menu;
    MENUITEM *fallback = NULL;
    UINT fallback_pos = 0;
    UINT i;

    if ((*hmenu == (HMENU)0xffff) || (!(menu = MENU_GetMenu(*hmenu)))) return NULL;
    if (wFlags & MF_BYPOSITION)
    {
	if (*nPos >= menu->nItems) return NULL;
	return &menu->items[*nPos];
    }
    else
    {
        MENUITEM *item = menu->items;
	for (i = 0; i < menu->nItems; i++, item++)
	{
	    if (item->fType & MF_POPUP)
	    {
		HMENU hsubmenu = item->hSubMenu;
		MENUITEM *subitem = MENU_FindItem( &hsubmenu, nPos, wFlags );
		if (subitem)
		{
		    *hmenu = hsubmenu;
		    return subitem;
		}
		else if (item->wID == *nPos)
		{
		    /* fallback to this item if nothing else found */
		    fallback_pos = i;
		    fallback = item;
		}
	    }
	    else if (item->wID == *nPos)
	    {
		*nPos = i;
		return item;
	    }
	}
    }

    if (fallback)
        *nPos = fallback_pos;

    return fallback;
}

/***********************************************************************
 *           MENU_FindSubMenu
 *
 * Find a Sub menu. Return the position of the submenu, and modifies
 * *hmenu in case it is found in another sub-menu.
 * If the submenu cannot be found, NO_SELECTED_ITEM is returned.
 */
static UINT MENU_FindSubMenu( HMENU *hmenu, HMENU hSubTarget )
{
    POPUPMENU *menu;
    UINT i;
    MENUITEM *item;
    if (((*hmenu)==(HMENU)0xffff) ||
            (!(menu = MENU_GetMenu(*hmenu))))
        return NO_SELECTED_ITEM;
    item = menu->items;
    for (i = 0; i < menu->nItems; i++, item++) {
        if(!(item->fType & MF_POPUP)) continue;
        if (item->hSubMenu == hSubTarget) {
            return i;
        }
        else  {
            HMENU hsubmenu = item->hSubMenu;
            UINT pos = MENU_FindSubMenu( &hsubmenu, hSubTarget );
            if (pos != NO_SELECTED_ITEM) {
                *hmenu = hsubmenu;
                return pos;
            }
        }
    }
    return NO_SELECTED_ITEM;
}

/***********************************************************************
 *           MENU_FreeItemData
 */
static void MENU_FreeItemData( MENUITEM* item )
{
    /* delete text */
    HeapFree( GetProcessHeap(), 0, item->text );
}

/***********************************************************************
 *           MENU_AdjustMenuItemRect
 *
 * Adjust menu item rectangle according to scrolling state.
 */
static void
MENU_AdjustMenuItemRect(const POPUPMENU *menu, LPRECT rect)
{
    if (menu->bScrolling)
    {
        UINT arrow_bitmap_height;
        BITMAP bmp;

        GetObjectW(get_up_arrow_bitmap(), sizeof(bmp), &bmp);
        arrow_bitmap_height = bmp.bmHeight;
        rect->top += arrow_bitmap_height - menu->nScrollPos;
        rect->bottom += arrow_bitmap_height - menu->nScrollPos;
    }
}


/***********************************************************************
 *           MENU_FindItemByCoords
 *
 * Find the item at the specified coordinates (screen coords). Does
 * not work for child windows and therefore should not be called for
 * an arbitrary system menu.
 */
static MENUITEM *MENU_FindItemByCoords( const POPUPMENU *menu,
					POINT pt, UINT *pos )
{
    MENUITEM *item;
    UINT i;
    RECT rect;

    if (!GetWindowRect(menu->hWnd, &rect)) return NULL;
    if (GetWindowLongW( menu->hWnd, GWL_EXSTYLE ) & WS_EX_LAYOUTRTL) pt.x = rect.right - 1 - pt.x;
    else pt.x -= rect.left;
    pt.y -= rect.top;
    item = menu->items;
    for (i = 0; i < menu->nItems; i++, item++)
    {
        rect = item->rect;
        MENU_AdjustMenuItemRect(menu, &rect);
	if (PtInRect(&rect, pt))
	{
	    if (pos) *pos = i;
	    return item;
	}
    }
    return NULL;
}


/***********************************************************************
 *           MENU_FindItemByKey
 *
 * Find the menu item selected by a key press.
 * Return item id, -1 if none, -2 if we should close the menu.
 */
static UINT MENU_FindItemByKey( HWND hwndOwner, HMENU hmenu,
                                WCHAR key, BOOL forceMenuChar )
{
    TRACE("\tlooking for '%c' (0x%02x) in [%p]\n", (char)key, key, hmenu );

    if (!IsMenu( hmenu )) hmenu = GetSubMenu( get_win_sys_menu(hwndOwner), 0);

    if (hmenu)
    {
	POPUPMENU *menu = MENU_GetMenu( hmenu );
	MENUITEM *item = menu->items;
	LRESULT menuchar;

	if( !forceMenuChar )
	{
	     UINT i;

	     for (i = 0; i < menu->nItems; i++, item++)
	     {
		if( item->text)
		{
		    WCHAR *p = item->text - 2;
		    do
		    {
		    	p = strchrW (p + 2, '&');
		    }
		    while (p != NULL && p [1] == '&');
		    if (p && (toupperW(p[1]) == toupperW(key))) return i;
		}
	     }
	}
	menuchar = SendMessageW( hwndOwner, WM_MENUCHAR,
                                 MAKEWPARAM( key, menu->wFlags ), (LPARAM)hmenu );
	if (HIWORD(menuchar) == 2) return LOWORD(menuchar);
	if (HIWORD(menuchar) == 1) return (UINT)(-2);
    }
    return (UINT)(-1);
}


/***********************************************************************
 *           MENU_GetBitmapItemSize
 *
 * Get the size of a bitmap item.
 */
static void MENU_GetBitmapItemSize( MENUITEM *lpitem, SIZE *size,
                                    HWND hwndOwner)
{
    BITMAP bm;
    HBITMAP bmp = lpitem->hbmpItem;

    size->cx = size->cy = 0;

    /* check if there is a magic menu item associated with this item */
    switch( (INT_PTR) bmp )
    {
    case (INT_PTR)HBMMENU_CALLBACK:
        {
            MEASUREITEMSTRUCT measItem;
            measItem.CtlType = ODT_MENU;
            measItem.CtlID = 0;
            measItem.itemID = lpitem->wID;
            measItem.itemWidth = lpitem->rect.right - lpitem->rect.left;
            measItem.itemHeight = lpitem->rect.bottom - lpitem->rect.top;
            measItem.itemData = lpitem->dwItemData;
            SendMessageW( hwndOwner, WM_MEASUREITEM, lpitem->wID, (LPARAM)&measItem);
            size->cx = measItem.itemWidth;
            size->cy = measItem.itemHeight;
            return;
        }
        break;
    case (INT_PTR)HBMMENU_SYSTEM:
        if (lpitem->dwItemData)
        {
            bmp = (HBITMAP)lpitem->dwItemData;
            break;
        }
        /* fall through */
    case (INT_PTR)HBMMENU_MBAR_RESTORE:
    case (INT_PTR)HBMMENU_MBAR_MINIMIZE:
    case (INT_PTR)HBMMENU_MBAR_MINIMIZE_D:
    case (INT_PTR)HBMMENU_MBAR_CLOSE:
    case (INT_PTR)HBMMENU_MBAR_CLOSE_D:
        size->cx = GetSystemMetrics( SM_CYMENU ) - 4;
        size->cy = size->cx;
        return;
    case (INT_PTR)HBMMENU_POPUP_CLOSE:
    case (INT_PTR)HBMMENU_POPUP_RESTORE:
    case (INT_PTR)HBMMENU_POPUP_MAXIMIZE:
    case (INT_PTR)HBMMENU_POPUP_MINIMIZE:
        size->cx = GetSystemMetrics( SM_CXMENUSIZE);
        size->cy = GetSystemMetrics( SM_CYMENUSIZE);
        return;
    }
    if (GetObjectW(bmp, sizeof(bm), &bm ))
    {
        size->cx = bm.bmWidth;
        size->cy = bm.bmHeight;
    }
}

/***********************************************************************
 *           MENU_DrawBitmapItem
 *
 * Draw a bitmap item.
 */
static void MENU_DrawBitmapItem( HDC hdc, MENUITEM *lpitem, const RECT *rect,
                    HMENU hmenu, HWND hwndOwner, UINT odaction, BOOL menuBar)
{
    BITMAP bm;
    DWORD rop;
    HDC hdcMem;
    HBITMAP bmp;
    int w = rect->right - rect->left;
    int h = rect->bottom - rect->top;
    int bmp_xoffset = 0;
    int left, top;
    HBITMAP hbmToDraw = lpitem->hbmpItem;
    bmp = hbmToDraw;

    /* Check if there is a magic menu item associated with this item */
    if (IS_MAGIC_BITMAP(hbmToDraw))
    {
        UINT flags = 0;
        WCHAR bmchr = 0;
        RECT r;

	switch((INT_PTR)hbmToDraw)
        {
        case (INT_PTR)HBMMENU_SYSTEM:
            if (lpitem->dwItemData)
            {
                bmp = (HBITMAP)lpitem->dwItemData;
                if (!GetObjectW( bmp, sizeof(bm), &bm )) return;
            }
            else
            {
                static HBITMAP hBmpSysMenu;

                if (!hBmpSysMenu) hBmpSysMenu = LoadBitmapW(0, MAKEINTRESOURCEW(OBM_CLOSE));
                bmp = hBmpSysMenu;
                if (!GetObjectW( bmp, sizeof(bm), &bm )) return;
                /* only use right half of the bitmap */
                bmp_xoffset = bm.bmWidth / 2;
                bm.bmWidth -= bmp_xoffset;
            }
            goto got_bitmap;
        case (INT_PTR)HBMMENU_MBAR_RESTORE:
            flags = DFCS_CAPTIONRESTORE;
            break;
        case (INT_PTR)HBMMENU_MBAR_MINIMIZE:
            flags = DFCS_CAPTIONMIN;
            break;
        case (INT_PTR)HBMMENU_MBAR_MINIMIZE_D:
            flags = DFCS_CAPTIONMIN | DFCS_INACTIVE;
            break;
        case (INT_PTR)HBMMENU_MBAR_CLOSE:
            flags = DFCS_CAPTIONCLOSE;
            break;
        case (INT_PTR)HBMMENU_MBAR_CLOSE_D:
            flags = DFCS_CAPTIONCLOSE | DFCS_INACTIVE;
            break;
        case (INT_PTR)HBMMENU_CALLBACK:
            {
                DRAWITEMSTRUCT drawItem;
                drawItem.CtlType = ODT_MENU;
                drawItem.CtlID = 0;
                drawItem.itemID = lpitem->wID;
                drawItem.itemAction = odaction;
                drawItem.itemState = (lpitem->fState & MF_CHECKED)?ODS_CHECKED:0;
                drawItem.itemState |= (lpitem->fState & MF_DEFAULT)?ODS_DEFAULT:0;
                drawItem.itemState |= (lpitem->fState & MF_DISABLED)?ODS_DISABLED:0;
                drawItem.itemState |= (lpitem->fState & MF_GRAYED)?ODS_GRAYED|ODS_DISABLED:0;
                drawItem.itemState |= (lpitem->fState & MF_HILITE)?ODS_SELECTED:0;
                drawItem.hwndItem = (HWND)hmenu;
                drawItem.hDC = hdc;
                drawItem.itemData = lpitem->dwItemData;
                drawItem.rcItem = *rect;
                SendMessageW( hwndOwner, WM_DRAWITEM, 0, (LPARAM)&drawItem);
                return;
            }
            break;
        case (INT_PTR)HBMMENU_POPUP_CLOSE:
            bmchr = 0x72;
            break;
        case (INT_PTR)HBMMENU_POPUP_RESTORE:
            bmchr = 0x32;
            break;
        case (INT_PTR)HBMMENU_POPUP_MAXIMIZE:
            bmchr = 0x31;
            break;
        case (INT_PTR)HBMMENU_POPUP_MINIMIZE:
            bmchr = 0x30;
            break;
        default:
            FIXME("Magic %p not implemented\n", hbmToDraw);
            return;
        }
        if (bmchr)
        {
            /* draw the magic bitmaps using marlett font characters */
            /* FIXME: fontsize and the position (x,y) could probably be better */
            HFONT hfont, hfontsav;
            LOGFONTW logfont = { 0, 0, 0, 0, FW_DONTCARE,
                0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0,
                { 'M','a','r','l','e','t','t',0 } };
            logfont.lfHeight =  min( h, w) - 5 ;
            TRACE(" height %d rect %s\n", logfont.lfHeight, wine_dbgstr_rect( rect));
            hfont = CreateFontIndirectW( &logfont);
            hfontsav = SelectObject(hdc, hfont);
            TextOutW( hdc,  rect->left, rect->top + 4, &bmchr, 1);
            SelectObject(hdc, hfontsav);
            DeleteObject( hfont);
        }
        else
        {
            r = *rect;
            InflateRect( &r, -1, -1 );
            if (lpitem->fState & MF_HILITE) flags |= DFCS_PUSHED;
            DrawFrameControl( hdc, &r, DFC_CAPTION, flags );
        }
        return;
    }

    if (!bmp || !GetObjectW( bmp, sizeof(bm), &bm )) return;

 got_bitmap:
    hdcMem = CreateCompatibleDC( hdc );
    SelectObject( hdcMem, bmp );

    /* handle fontsize > bitmap_height */
    top = (h>bm.bmHeight) ? rect->top+(h-bm.bmHeight)/2 : rect->top;
    left=rect->left;
    rop=((lpitem->fState & MF_HILITE) && !IS_MAGIC_BITMAP(hbmToDraw)) ? NOTSRCCOPY : SRCCOPY;
    if ((lpitem->fState & MF_HILITE) && lpitem->hbmpItem)
        SetBkColor(hdc, GetSysColor(COLOR_HIGHLIGHT));
    BitBlt( hdc, left, top, w, h, hdcMem, bmp_xoffset, 0, rop );
    DeleteDC( hdcMem );
}


/***********************************************************************
 *           MENU_CalcItemSize
 *
 * Calculate the size of the menu item and store it in lpitem->rect.
 */
static void MENU_CalcItemSize( HDC hdc, MENUITEM *lpitem, HWND hwndOwner,
			       INT orgX, INT orgY, BOOL menuBar, POPUPMENU* lppop )
{
    WCHAR *p;
    UINT check_bitmap_width = GetSystemMetrics( SM_CXMENUCHECK );
    UINT arrow_bitmap_width;
    BITMAP bm;
    INT itemheight;

    TRACE("dc=%p owner=%p (%d,%d)\n", hdc, hwndOwner, orgX, orgY);
    debug_print_menuitem("MENU_CalcItemSize: menuitem:", lpitem,
			 (menuBar ? " (MenuBar)" : ""));

    GetObjectW( get_arrow_bitmap(), sizeof(bm), &bm );
    arrow_bitmap_width = bm.bmWidth;

    /* not done in Menu_Init: GetDialogBaseUnits() breaks there */
    if( !menucharsize.cx ) {
        menucharsize.cx = GdiGetCharDimensions( hdc, NULL, &menucharsize.cy );
        /* Win95/98/ME will use menucharsize.cy here. Testing is possible
         * but it is unlikely an application will depend on that */
        ODitemheight = HIWORD( GetDialogBaseUnits());
    }

    SetRect( &lpitem->rect, orgX, orgY, orgX, orgY );

    if (lpitem->fType & MF_OWNERDRAW)
    {
        MEASUREITEMSTRUCT mis;
        mis.CtlType    = ODT_MENU;
        mis.CtlID      = 0;
        mis.itemID     = lpitem->wID;
        mis.itemData   = lpitem->dwItemData;
        mis.itemHeight = ODitemheight;
        mis.itemWidth  = 0;
        SendMessageW( hwndOwner, WM_MEASUREITEM, 0, (LPARAM)&mis );
        /* Tests reveal that Windows ( Win95 thru WinXP) adds twice the average
         * width of a menufont character to the width of an owner-drawn menu. 
         */
        lpitem->rect.right += mis.itemWidth + 2 * menucharsize.cx;
        if (menuBar) {
            /* under at least win95 you seem to be given a standard
               height for the menu and the height value is ignored */
            lpitem->rect.bottom += GetSystemMetrics(SM_CYMENUSIZE);
        } else
            lpitem->rect.bottom += mis.itemHeight;

        TRACE("id=%04lx size=%dx%d\n",
                lpitem->wID, lpitem->rect.right-lpitem->rect.left,
                lpitem->rect.bottom-lpitem->rect.top);
        return;
    }

    if (lpitem->fType & MF_SEPARATOR)
    {
        lpitem->rect.bottom += GetSystemMetrics( SM_CYMENUSIZE)/2;
        if( !menuBar)
            lpitem->rect.right += arrow_bitmap_width + menucharsize.cx;
        return;
    }

    itemheight = 0;
    lpitem->xTab = 0;

    if (!menuBar) {
        if (lpitem->hbmpItem) {
            SIZE size;

            MENU_GetBitmapItemSize(lpitem, &size, hwndOwner);
            /* Keep the size of the bitmap in callback mode to be able
             * to draw it correctly */
            lpitem->bmpsize = size;
            lppop->textOffset = max( lppop->textOffset, size.cx);
            lpitem->rect.right += size.cx + 2;
            itemheight = size.cy + 2;
        }
        if( !(lppop->dwStyle & MNS_NOCHECK))
            lpitem->rect.right += check_bitmap_width; 
        lpitem->rect.right += 4 + menucharsize.cx;
        lpitem->xTab = lpitem->rect.right;
        lpitem->rect.right += arrow_bitmap_width;
    } else if (lpitem->hbmpItem) { /* menuBar */
        SIZE size;

        MENU_GetBitmapItemSize( lpitem, &size, hwndOwner );
        lpitem->bmpsize = size;
        lpitem->rect.right  += size.cx;
        if( lpitem->text) lpitem->rect.right  += 2;
        itemheight = size.cy;
    }

    /* it must be a text item - unless it's the system menu */
    if (!(lpitem->fType & MF_SYSMENU) && lpitem->text) {
        HFONT hfontOld = NULL;
        RECT rc = lpitem->rect;
        LONG txtheight, txtwidth;

	if ( lpitem->fState & MFS_DEFAULT ) {
	     hfontOld = SelectObject( hdc, get_menu_font(TRUE) );
	}
        if (menuBar) {
            txtheight = DrawTextW( hdc, lpitem->text, -1, &rc,
                    DT_SINGLELINE|DT_CALCRECT); 
            lpitem->rect.right  += rc.right - rc.left;
            itemheight = max( max( itemheight, txtheight),
                    GetSystemMetrics( SM_CYMENU) - 1);
            lpitem->rect.right +=  2 * menucharsize.cx;
        } else {
            if ((p = strchrW( lpitem->text, '\t' )) != NULL) {
                RECT tmprc = rc;
                LONG tmpheight;
                int n = (int)( p - lpitem->text);
                /* Item contains a tab (only meaningful in popup menus) */
                /* get text size before the tab */
                txtheight = DrawTextW( hdc, lpitem->text, n, &rc,
                        DT_SINGLELINE|DT_CALCRECT);
                txtwidth = rc.right - rc.left;
                p += 1; /* advance past the Tab */
                /* get text size after the tab */
                tmpheight = DrawTextW( hdc, p, -1, &tmprc,
                        DT_SINGLELINE|DT_CALCRECT);
                lpitem->xTab += txtwidth;
                txtheight = max( txtheight, tmpheight);
                txtwidth += menucharsize.cx + /* space for the tab */
                    tmprc.right - tmprc.left; /* space for the short cut */
            } else {
                txtheight = DrawTextW( hdc, lpitem->text, -1, &rc,
                        DT_SINGLELINE|DT_CALCRECT);
                txtwidth = rc.right - rc.left;
                lpitem->xTab += txtwidth;
            }
            lpitem->rect.right  += 2 + txtwidth;
            itemheight = max( itemheight,
                    max( txtheight + 2, menucharsize.cy + 4));
        }
	if (hfontOld) SelectObject (hdc, hfontOld);
    } else if( menuBar) {
        itemheight = max( itemheight, GetSystemMetrics(SM_CYMENU)-1);
    }
    lpitem->rect.bottom += itemheight;
    TRACE("%s\n", wine_dbgstr_rect( &lpitem->rect));
}


/***********************************************************************
 *           MENU_GetMaxPopupHeight
 */
static UINT
MENU_GetMaxPopupHeight(const POPUPMENU *lppop)
{
    if (lppop->cyMax)
        return lppop->cyMax;
    return GetSystemMetrics(SM_CYSCREEN) - GetSystemMetrics(SM_CYBORDER);
}


/***********************************************************************
 *           MENU_PopupMenuCalcSize
 *
 * Calculate the size of a popup menu.
 */
static void MENU_PopupMenuCalcSize( LPPOPUPMENU lppop )
{
    MENUITEM *lpitem;
    HDC hdc;
    UINT start, i;
    int textandbmp = FALSE;
    int orgX, orgY, maxX, maxTab, maxTabWidth, maxHeight;

    lppop->Width = lppop->Height = 0;
    if (lppop->nItems == 0) return;
    hdc = GetDC( 0 );

    SelectObject( hdc, get_menu_font(FALSE));

    start = 0;
    maxX = 2 + 1;

    lppop->textOffset = 0;

    while (start < lppop->nItems)
    {
	lpitem = &lppop->items[start];
	orgX = maxX;
        if( lpitem->fType & (MF_MENUBREAK | MF_MENUBARBREAK))
            orgX += MENU_COL_SPACE; 
	orgY = MENU_TOP_MARGIN;

	maxTab = maxTabWidth = 0;
	  /* Parse items until column break or end of menu */
	for (i = start; i < lppop->nItems; i++, lpitem++)
	{
	    if ((i != start) &&
		(lpitem->fType & (MF_MENUBREAK | MF_MENUBARBREAK))) break;

	    MENU_CalcItemSize( hdc, lpitem, lppop->hwndOwner, orgX, orgY, FALSE, lppop );
	    maxX = max( maxX, lpitem->rect.right );
	    orgY = lpitem->rect.bottom;
	    if (IS_STRING_ITEM(lpitem->fType) && lpitem->xTab)
	    {
		maxTab = max( maxTab, lpitem->xTab );
		maxTabWidth = max(maxTabWidth,lpitem->rect.right-lpitem->xTab);
	    }
            if( lpitem->text && lpitem->hbmpItem) textandbmp = TRUE;
	}

	  /* Finish the column (set all items to the largest width found) */
	maxX = max( maxX, maxTab + maxTabWidth );
	for (lpitem = &lppop->items[start]; start < i; start++, lpitem++)
	{
	    lpitem->rect.right = maxX;
	    if (IS_STRING_ITEM(lpitem->fType) && lpitem->xTab)
		lpitem->xTab = maxTab;

	}
	lppop->Height = max( lppop->Height, orgY );
    }

    lppop->Width  = maxX;
    /* if none of the items have both text and bitmap then
     * the text and bitmaps are all aligned on the left. If there is at
     * least one item with both text and bitmap then bitmaps are
     * on the left and texts left aligned with the right hand side
     * of the bitmaps */
    if( !textandbmp) lppop->textOffset = 0;

    /* space for 3d border */
    lppop->Height += MENU_BOTTOM_MARGIN;
    lppop->Width += 2;

    /* Adjust popup height if it exceeds maximum */
    maxHeight = MENU_GetMaxPopupHeight(lppop);
    lppop->nTotalHeight = lppop->Height - MENU_TOP_MARGIN;
    if (lppop->Height >= maxHeight)
    {
        lppop->Height = maxHeight;
        lppop->bScrolling = TRUE;
    }
    else
    {
        lppop->bScrolling = FALSE;
    }

    ReleaseDC( 0, hdc );
}


/***********************************************************************
 *           MENU_MenuBarCalcSize
 *
 * FIXME: Word 6 implements its own MDI and its own 'close window' bitmap
 * height is off by 1 pixel which causes lengthy window relocations when
 * active document window is maximized/restored.
 *
 * Calculate the size of the menu bar.
 */
static void MENU_MenuBarCalcSize( HDC hdc, LPRECT lprect,
                                  LPPOPUPMENU lppop, HWND hwndOwner )
{
    MENUITEM *lpitem;
    UINT start, i, helpPos;
    int orgX, orgY, maxY;

    if ((lprect == NULL) || (lppop == NULL)) return;
    if (lppop->nItems == 0) return;
    TRACE("lprect %p %s\n", lprect, wine_dbgstr_rect( lprect));
    lppop->Width  = lprect->right - lprect->left;
    lppop->Height = 0;
    maxY = lprect->top+1;
    start = 0;
    helpPos = ~0U;
    lppop->textOffset = 0;
    while (start < lppop->nItems)
    {
	lpitem = &lppop->items[start];
	orgX = lprect->left;
	orgY = maxY;

	  /* Parse items until line break or end of menu */
	for (i = start; i < lppop->nItems; i++, lpitem++)
	{
	    if ((helpPos == ~0U) && (lpitem->fType & MF_RIGHTJUSTIFY)) helpPos = i;
	    if ((i != start) &&
		(lpitem->fType & (MF_MENUBREAK | MF_MENUBARBREAK))) break;

	    TRACE("calling MENU_CalcItemSize org=(%d, %d)\n", orgX, orgY );
	    debug_print_menuitem ("  item: ", lpitem, "");
	    MENU_CalcItemSize( hdc, lpitem, hwndOwner, orgX, orgY, TRUE, lppop );

	    if (lpitem->rect.right > lprect->right)
	    {
		if (i != start) break;
		else lpitem->rect.right = lprect->right;
	    }
	    maxY = max( maxY, lpitem->rect.bottom );
	    orgX = lpitem->rect.right;
	}

	  /* Finish the line (set all items to the largest height found) */
	while (start < i) lppop->items[start++].rect.bottom = maxY;
    }

    lprect->bottom = maxY;
    lppop->Height = lprect->bottom - lprect->top;

    /* Flush right all items between the MF_RIGHTJUSTIFY and */
    /* the last item (if several lines, only move the last line) */
    if (helpPos == ~0U) return;
    lpitem = &lppop->items[lppop->nItems-1];
    orgY = lpitem->rect.top;
    orgX = lprect->right;
    for (i = lppop->nItems - 1; i >= helpPos; i--, lpitem--) {
        if (lpitem->rect.top != orgY) break;	/* Other line */
        if (lpitem->rect.right >= orgX) break;	/* Too far right already */
        lpitem->rect.left += orgX - lpitem->rect.right;
        lpitem->rect.right = orgX;
        orgX = lpitem->rect.left;
    }
}


/***********************************************************************
 *           MENU_DrawScrollArrows
 *
 * Draw scroll arrows.
 */
static void
MENU_DrawScrollArrows(const POPUPMENU *lppop, HDC hdc)
{
    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hOrigBitmap;
    UINT arrow_bitmap_width, arrow_bitmap_height;
    BITMAP bmp;
    RECT rect;

    GetObjectW(get_down_arrow_bitmap(), sizeof(bmp), &bmp);
    arrow_bitmap_width = bmp.bmWidth;
    arrow_bitmap_height = bmp.bmHeight;

    
    if (lppop->nScrollPos)
        hOrigBitmap = SelectObject(hdcMem, get_up_arrow_bitmap());
    else
        hOrigBitmap = SelectObject(hdcMem, get_up_arrow_inactive_bitmap());
    rect.left = 0;
    rect.top = 0;
    rect.right = lppop->Width;
    rect.bottom = arrow_bitmap_height;
    FillRect(hdc, &rect, GetSysColorBrush(COLOR_MENU));
    BitBlt(hdc, (lppop->Width - arrow_bitmap_width) / 2, 0,
           arrow_bitmap_width, arrow_bitmap_height, hdcMem, 0, 0, SRCCOPY);
    rect.top = lppop->Height - arrow_bitmap_height;
    rect.bottom = lppop->Height;
    FillRect(hdc, &rect, GetSysColorBrush(COLOR_MENU));
    if (lppop->nScrollPos < lppop->nTotalHeight - (MENU_GetMaxPopupHeight(lppop) - 2 * arrow_bitmap_height))
        SelectObject(hdcMem, get_down_arrow_bitmap());
    else
        SelectObject(hdcMem, get_down_arrow_inactive_bitmap());
    BitBlt(hdc, (lppop->Width - arrow_bitmap_width) / 2,
           lppop->Height - arrow_bitmap_height,
           arrow_bitmap_width, arrow_bitmap_height, hdcMem, 0, 0, SRCCOPY);
    SelectObject(hdcMem, hOrigBitmap);
    DeleteDC(hdcMem);
}


/***********************************************************************
 *           draw_popup_arrow
 *
 * Draws the popup-menu arrow.
 */
static void draw_popup_arrow( HDC hdc, RECT rect, UINT arrow_bitmap_width,
        UINT arrow_bitmap_height)
{
    HDC hdcMem = CreateCompatibleDC( hdc );
    HBITMAP hOrigBitmap;

    hOrigBitmap = SelectObject( hdcMem, get_arrow_bitmap() );
    BitBlt( hdc, rect.right - arrow_bitmap_width - 1,
            (rect.top + rect.bottom - arrow_bitmap_height) / 2,
            arrow_bitmap_width, arrow_bitmap_height,
            hdcMem, 0, 0, SRCCOPY );
    SelectObject( hdcMem, hOrigBitmap );
    DeleteDC( hdcMem );
}
/***********************************************************************
 *           MENU_DrawMenuItem
 *
 * Draw a single menu item.
 */
static void MENU_DrawMenuItem( HWND hwnd, HMENU hmenu, HWND hwndOwner, HDC hdc, MENUITEM *lpitem,
			       UINT height, BOOL menuBar, UINT odaction )
{
    RECT rect;
    BOOL flat_menu = FALSE;
    int bkgnd;
    UINT arrow_bitmap_width = 0, arrow_bitmap_height = 0;
    POPUPMENU *menu = MENU_GetMenu(hmenu);
    RECT bmprc;

    debug_print_menuitem("MENU_DrawMenuItem: ", lpitem, "");

    if (!menuBar) {
        BITMAP bmp;
        GetObjectW( get_arrow_bitmap(), sizeof(bmp), &bmp );
        arrow_bitmap_width = bmp.bmWidth;
        arrow_bitmap_height = bmp.bmHeight;
    }

    if (lpitem->fType & MF_SYSMENU)
    {
	if( !IsIconic(hwnd) )
	    NC_DrawSysButton( hwnd, hdc, lpitem->fState & (MF_HILITE | MF_MOUSESELECT) );
	return;
    }

    SystemParametersInfoW (SPI_GETFLATMENU, 0, &flat_menu, 0);
    bkgnd = (menuBar && flat_menu) ? COLOR_MENUBAR : COLOR_MENU;
  
      /* Setup colors */

    if (lpitem->fState & MF_HILITE)
    {
        if(menuBar && !flat_menu) {
	    SetTextColor(hdc, GetSysColor(COLOR_MENUTEXT));
            SetBkColor(hdc, GetSysColor(COLOR_MENU));
	} else {
	    if(lpitem->fState & MF_GRAYED)
		SetTextColor(hdc, GetSysColor(COLOR_GRAYTEXT));
	    else
		SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
            SetBkColor(hdc, GetSysColor(COLOR_HIGHLIGHT));
	}
    }
    else
    {
	if (lpitem->fState & MF_GRAYED)
	    SetTextColor( hdc, GetSysColor( COLOR_GRAYTEXT ) );
	else
	    SetTextColor( hdc, GetSysColor( COLOR_MENUTEXT ) );
	SetBkColor( hdc, GetSysColor( bkgnd ) );
    }

    TRACE("rect=%s\n", wine_dbgstr_rect( &lpitem->rect));
    rect = lpitem->rect;
    MENU_AdjustMenuItemRect(MENU_GetMenu(hmenu), &rect);

    if (lpitem->fType & MF_OWNERDRAW)
    {
        /*
        ** Experimentation under Windows reveals that an owner-drawn
        ** menu is given the rectangle which includes the space it requested
        ** in its response to WM_MEASUREITEM _plus_ width for a checkmark
        ** and a popup-menu arrow.  This is the value of lpitem->rect.
        ** Windows will leave all drawing to the application except for
        ** the popup-menu arrow.  Windows always draws that itself, after
        ** the menu owner has finished drawing.
        */
        DRAWITEMSTRUCT dis;

        dis.CtlType   = ODT_MENU;
	dis.CtlID     = 0;
        dis.itemID    = lpitem->wID;
        dis.itemData  = lpitem->dwItemData;
        dis.itemState = 0;
        if (lpitem->fState & MF_CHECKED) dis.itemState |= ODS_CHECKED;
        if (lpitem->fState & MF_GRAYED)  dis.itemState |= ODS_GRAYED|ODS_DISABLED;
        if (lpitem->fState & MF_HILITE)  dis.itemState |= ODS_SELECTED;
        dis.itemAction = odaction; /* ODA_DRAWENTIRE | ODA_SELECT | ODA_FOCUS; */
        dis.hwndItem   = (HWND)hmenu;
        dis.hDC        = hdc;
        dis.rcItem     = rect;
        TRACE("Ownerdraw: owner=%p itemID=%d, itemState=%d, itemAction=%d, "
	      "hwndItem=%p, hdc=%p, rcItem=%s\n", hwndOwner,
	      dis.itemID, dis.itemState, dis.itemAction, dis.hwndItem,
	      dis.hDC, wine_dbgstr_rect( &dis.rcItem));
        SendMessageW( hwndOwner, WM_DRAWITEM, 0, (LPARAM)&dis );
        /* Draw the popup-menu arrow */
        if (lpitem->fType & MF_POPUP)
            draw_popup_arrow( hdc, rect, arrow_bitmap_width,
                    arrow_bitmap_height);
        return;
    }

    if (menuBar && (lpitem->fType & MF_SEPARATOR)) return;

    if (lpitem->fState & MF_HILITE)
    {
        if (flat_menu)
        {
            InflateRect (&rect, -1, -1);
            FillRect(hdc, &rect, GetSysColorBrush(COLOR_MENUHILIGHT));
            InflateRect (&rect, 1, 1);
            FrameRect(hdc, &rect, GetSysColorBrush(COLOR_HIGHLIGHT));
        }
        else
        {
            if(menuBar)
                DrawEdge(hdc, &rect, BDR_SUNKENOUTER, BF_RECT);
            else
                FillRect(hdc, &rect, GetSysColorBrush(COLOR_HIGHLIGHT));
        }
    }
    else
        FillRect( hdc, &rect, GetSysColorBrush(bkgnd) );

    SetBkMode( hdc, TRANSPARENT );

    /* vertical separator */
    if (!menuBar && (lpitem->fType & MF_MENUBARBREAK))
    {
        HPEN oldPen;
        RECT rc = rect;

        rc.left -= MENU_COL_SPACE / 2 + 1;
        rc.top = 3;
        rc.bottom = height - 3;
        if (flat_menu)
        {
            oldPen = SelectObject( hdc, SYSCOLOR_GetPen(COLOR_BTNSHADOW) );
            MoveToEx( hdc, rc.left, rc.top, NULL );
            LineTo( hdc, rc.left, rc.bottom );
            SelectObject( hdc, oldPen );
        }
        else
            DrawEdge (hdc, &rc, EDGE_ETCHED, BF_LEFT);
    }

    /* horizontal separator */
    if (lpitem->fType & MF_SEPARATOR)
    {
        HPEN oldPen;
        RECT rc = rect;

        rc.left++;
        rc.right--;
        rc.top = ( rc.top + rc.bottom) / 2;
        if (flat_menu)
        {
            oldPen = SelectObject( hdc, SYSCOLOR_GetPen(COLOR_BTNSHADOW) );
            MoveToEx( hdc, rc.left, rc.top, NULL );
            LineTo( hdc, rc.right, rc.top );
            SelectObject( hdc, oldPen );
        }
        else
            DrawEdge (hdc, &rc, EDGE_ETCHED, BF_TOP);
        return;
    }

	/* helper lines for debugging */
/*	FrameRect(hdc, &rect, GetStockObject(BLACK_BRUSH));
	SelectObject( hdc, SYSCOLOR_GetPen(COLOR_WINDOWFRAME) );
	MoveToEx( hdc, rect.left, (rect.top + rect.bottom)/2, NULL );
	LineTo( hdc, rect.right, (rect.top + rect.bottom)/2 );
*/

    if (lpitem->hbmpItem) {
        /* calculate the bitmap rectangle in coordinates relative
         * to the item rectangle */
        if( menuBar) {
            if( lpitem->hbmpItem == HBMMENU_CALLBACK)
                bmprc.left = 3;
            else 
                bmprc.left = lpitem->text ? menucharsize.cx : 0;          
        }
        else if (menu->dwStyle & MNS_NOCHECK)
            bmprc.left = 4;
        else if (menu->dwStyle & MNS_CHECKORBMP)
            bmprc.left = 2;
        else
            bmprc.left = 4 + GetSystemMetrics(SM_CXMENUCHECK);
        bmprc.right =  bmprc.left + lpitem->bmpsize.cx;
        if( menuBar && !(lpitem->hbmpItem == HBMMENU_CALLBACK))
            bmprc.top = 0;
        else
            bmprc.top = (rect.bottom - rect.top -
                    lpitem->bmpsize.cy) / 2; 
        bmprc.bottom =  bmprc.top + lpitem->bmpsize.cy;
    }

    if (!menuBar)
    {
        HBITMAP bm;
        INT y = rect.top + rect.bottom;
        RECT rc = rect;
        int checked = FALSE;
        UINT check_bitmap_width = GetSystemMetrics( SM_CXMENUCHECK );
        UINT check_bitmap_height = GetSystemMetrics( SM_CYMENUCHECK );
        /* Draw the check mark
         *
         * FIXME:
         * Custom checkmark bitmaps are monochrome but not always 1bpp.
         */
        if( !(menu->dwStyle & MNS_NOCHECK)) {
            bm = (lpitem->fState & MF_CHECKED) ? lpitem->hCheckBit :
                lpitem->hUnCheckBit;
            if (bm)  /* we have a custom bitmap */
            {
                HDC hdcMem = CreateCompatibleDC( hdc );

                SelectObject( hdcMem, bm );
                BitBlt( hdc, rc.left, (y - check_bitmap_height) / 2,
                        check_bitmap_width, check_bitmap_height,
                        hdcMem, 0, 0, SRCCOPY );
                DeleteDC( hdcMem );
                checked = TRUE;
            }
            else if (lpitem->fState & MF_CHECKED) /* standard bitmaps */
            {
                RECT r;
                HBITMAP bm = CreateBitmap( check_bitmap_width,
                        check_bitmap_height, 1, 1, NULL );
                HDC hdcMem = CreateCompatibleDC( hdc );

                SelectObject( hdcMem, bm );
                SetRect( &r, 0, 0, check_bitmap_width, check_bitmap_height);
                DrawFrameControl( hdcMem, &r, DFC_MENU,
                        (lpitem->fType & MFT_RADIOCHECK) ?
                        DFCS_MENUBULLET : DFCS_MENUCHECK );
                BitBlt( hdc, rc.left, (y - r.bottom) / 2, r.right, r.bottom,
                        hdcMem, 0, 0, SRCCOPY );
                DeleteDC( hdcMem );
                DeleteObject( bm );
                checked = TRUE;
            }
        }
        if( lpitem->hbmpItem &&
                !( checked && (menu->dwStyle & MNS_CHECKORBMP))) {
            POINT origorg;
            /* some applications make this assumption on the DC's origin */
            SetViewportOrgEx( hdc, rect.left, rect.top, &origorg);
            MENU_DrawBitmapItem(hdc, lpitem, &bmprc, hmenu, hwndOwner,
                    odaction, FALSE);
            SetViewportOrgEx( hdc, origorg.x, origorg.y, NULL);
        }
	/* Draw the popup-menu arrow */
        if (lpitem->fType & MF_POPUP)
            draw_popup_arrow( hdc, rect, arrow_bitmap_width,
                    arrow_bitmap_height);
	rect.left += 4;
        if( !(menu->dwStyle & MNS_NOCHECK))
            rect.left += check_bitmap_width;
	rect.right -= arrow_bitmap_width;
    }
    else if( lpitem->hbmpItem)
    {   /* Draw the bitmap */
        POINT origorg;
        
        SetViewportOrgEx( hdc, rect.left, rect.top, &origorg);
        MENU_DrawBitmapItem( hdc, lpitem, &bmprc, hmenu, hwndOwner,
                odaction, menuBar);
        SetViewportOrgEx( hdc, origorg.x, origorg.y, NULL);
    }
    /* process text if present */
    if (lpitem->text)
    {
	register int i;
	HFONT hfontOld = 0;

	UINT uFormat = (menuBar) ?
			DT_CENTER | DT_VCENTER | DT_SINGLELINE :
			DT_LEFT | DT_VCENTER | DT_SINGLELINE;

        if( !(menu->dwStyle & MNS_CHECKORBMP))
            rect.left += menu->textOffset;

	if ( lpitem->fState & MFS_DEFAULT )
	{
	     hfontOld = SelectObject( hdc, get_menu_font(TRUE) );
	}

	if (menuBar) {
            if( lpitem->hbmpItem)
                rect.left += lpitem->bmpsize.cx;
            if( !(lpitem->hbmpItem == HBMMENU_CALLBACK))
                rect.left += menucharsize.cx;
            rect.right -= menucharsize.cx;
	}

	for (i = 0; lpitem->text[i]; i++)
	    if ((lpitem->text[i] == '\t') || (lpitem->text[i] == '\b'))
	        break;

	if(lpitem->fState & MF_GRAYED)
	{
	    if (!(lpitem->fState & MF_HILITE) )
	    {
		++rect.left; ++rect.top; ++rect.right; ++rect.bottom;
		SetTextColor(hdc, RGB(0xff, 0xff, 0xff));
		DrawTextW( hdc, lpitem->text, i, &rect, uFormat );
		--rect.left; --rect.top; --rect.right; --rect.bottom;
	    }
	    SetTextColor(hdc, RGB(0x80, 0x80, 0x80));
	}

	DrawTextW( hdc, lpitem->text, i, &rect, uFormat);

	/* paint the shortcut text */
	if (!menuBar && lpitem->text[i])  /* There's a tab or flush-right char */
	{
	    if (lpitem->text[i] == '\t')
	    {
		rect.left = lpitem->xTab;
		uFormat = DT_LEFT | DT_VCENTER | DT_SINGLELINE;
	    }
	    else
	    {
		rect.right = lpitem->xTab;
		uFormat = DT_RIGHT | DT_VCENTER | DT_SINGLELINE;
	    }

	    if(lpitem->fState & MF_GRAYED)
	    {
		if (!(lpitem->fState & MF_HILITE) )
		{
		    ++rect.left; ++rect.top; ++rect.right; ++rect.bottom;
		    SetTextColor(hdc, RGB(0xff, 0xff, 0xff));
		    DrawTextW( hdc, lpitem->text + i + 1, -1, &rect, uFormat );
		    --rect.left; --rect.top; --rect.right; --rect.bottom;
		}
		SetTextColor(hdc, RGB(0x80, 0x80, 0x80));
	    }
	    DrawTextW( hdc, lpitem->text + i + 1, -1, &rect, uFormat );
	}

	if (hfontOld)
	    SelectObject (hdc, hfontOld);
    }
}


/***********************************************************************
 *           MENU_DrawPopupMenu
 *
 * Paint a popup menu.
 */
static void MENU_DrawPopupMenu( HWND hwnd, HDC hdc, HMENU hmenu )
{
    HBRUSH hPrevBrush = 0;
    RECT rect;

    TRACE("wnd=%p dc=%p menu=%p\n", hwnd, hdc, hmenu);

    GetClientRect( hwnd, &rect );

    if((hPrevBrush = SelectObject( hdc, GetSysColorBrush(COLOR_MENU) ))
        && (SelectObject( hdc, get_menu_font(FALSE))))
    {
	HPEN hPrevPen;

	Rectangle( hdc, rect.left, rect.top, rect.right, rect.bottom );

	hPrevPen = SelectObject( hdc, GetStockObject( NULL_PEN ) );
	if( hPrevPen )
	{
	    POPUPMENU *menu;
	    BOOL flat_menu = FALSE;

	    SystemParametersInfoW (SPI_GETFLATMENU, 0, &flat_menu, 0);
	    if (flat_menu)
		FrameRect(hdc, &rect, GetSysColorBrush(COLOR_BTNSHADOW));
	    else
		DrawEdge (hdc, &rect, EDGE_RAISED, BF_RECT);

            if( (menu = MENU_GetMenu( hmenu )))
            {
                TRACE("hmenu %p Style %08x\n", hmenu, menu->dwStyle);
                /* draw menu items */
                if( menu->nItems)
                {
                    MENUITEM *item;
                    UINT u;

                    item = menu->items;
                    for( u = menu->nItems; u > 0; u--, item++)
                        MENU_DrawMenuItem( hwnd, hmenu, menu->hwndOwner, hdc,
                                item, menu->Height, FALSE, ODA_DRAWENTIRE );
                }
                /* draw scroll arrows */
                if (menu->bScrolling)
                    MENU_DrawScrollArrows(menu, hdc);
            }
 	} else
	{
	    SelectObject( hdc, hPrevBrush );
	}
    }
}

/***********************************************************************
 *           MENU_DrawMenuBar
 *
 * Paint a menu bar. Returns the height of the menu bar.
 * called from [windows/nonclient.c]
 */
UINT MENU_DrawMenuBar( HDC hDC, LPRECT lprect, HWND hwnd,
                         BOOL suppress_draw)
{
    LPPOPUPMENU lppop;
    HFONT hfontOld = 0;
    HMENU hMenu = GetMenu(hwnd);

    lppop = MENU_GetMenu( hMenu );
    if (lppop == NULL || lprect == NULL)
    {
        return GetSystemMetrics(SM_CYMENU);
    }

    if (suppress_draw)
    {
	hfontOld = SelectObject( hDC, get_menu_font(FALSE));

	if (lppop->Height == 0)
		MENU_MenuBarCalcSize(hDC, lprect, lppop, hwnd);

	lprect->bottom = lprect->top + lppop->Height;

        if (hfontOld) SelectObject( hDC, hfontOld);
	return lppop->Height;
    }
    else
        return DrawMenuBarTemp(hwnd, hDC, lprect, hMenu, NULL);
}


/***********************************************************************
 *           MENU_ShowPopup
 *
 * Display a popup menu.
 */
static BOOL MENU_ShowPopup( HWND hwndOwner, HMENU hmenu, UINT id, UINT flags,
                              INT x, INT y, INT xanchor, INT yanchor )
{
    POPUPMENU *menu;
    INT width, height;
    POINT pt;
    HMONITOR monitor;
    MONITORINFO info;
    DWORD ex_style = 0;

    TRACE("owner=%p hmenu=%p id=0x%04x x=0x%04x y=0x%04x xa=0x%04x ya=0x%04x\n",
          hwndOwner, hmenu, id, x, y, xanchor, yanchor);

    if (!(menu = MENU_GetMenu( hmenu ))) return FALSE;
    if (menu->FocusedItem != NO_SELECTED_ITEM)
    {
	menu->items[menu->FocusedItem].fState &= ~(MF_HILITE|MF_MOUSESELECT);
	menu->FocusedItem = NO_SELECTED_ITEM;
    }

    /* store the owner for DrawItem */
    menu->hwndOwner = hwndOwner;

    menu->nScrollPos = 0;
    MENU_PopupMenuCalcSize( menu );

    /* adjust popup menu pos so that it fits within the desktop */

    width = menu->Width + GetSystemMetrics(SM_CXBORDER);
    height = menu->Height + GetSystemMetrics(SM_CYBORDER);

    /* FIXME: should use item rect */
    pt.x = x;
    pt.y = y;
    monitor = MonitorFromPoint( pt, MONITOR_DEFAULTTONEAREST );
    info.cbSize = sizeof(info);
    GetMonitorInfoW( monitor, &info );

    if (flags & TPM_LAYOUTRTL)
    {
        ex_style = WS_EX_LAYOUTRTL;
        flags ^= TPM_RIGHTALIGN;
    }

    if( flags & TPM_RIGHTALIGN ) x -= width;
    if( flags & TPM_CENTERALIGN ) x -= width / 2;

    if( flags & TPM_BOTTOMALIGN ) y -= height;
    if( flags & TPM_VCENTERALIGN ) y -= height / 2;

    if( x + width > info.rcWork.right)
    {
        if( xanchor && x >= width - xanchor )
            x -= width - xanchor;

        if( x + width > info.rcWork.right)
            x = info.rcWork.right - width;
    }
    if( x < info.rcWork.left ) x = info.rcWork.left;

    if( y + height > info.rcWork.bottom)
    {
        if( yanchor && y >= height + yanchor )
            y -= height + yanchor;

        if( y + height > info.rcWork.bottom)
            y = info.rcWork.bottom - height;
    }
    if( y < info.rcWork.top ) y = info.rcWork.top;

    /* NOTE: In Windows, top menu popup is not owned. */
    menu->hWnd = CreateWindowExW( ex_style, (LPCWSTR)POPUPMENU_CLASS_ATOM, NULL,
                                WS_POPUP, x, y, width, height,
                                hwndOwner, 0, (HINSTANCE)GetWindowLongPtrW(hwndOwner, GWLP_HINSTANCE),
                                (LPVOID)hmenu );
    if( !menu->hWnd ) return FALSE;
    if (!top_popup) {
        top_popup = menu->hWnd;
        top_popup_hmenu = hmenu;
    }
    /* Display the window */

    SetWindowPos( menu->hWnd, HWND_TOPMOST, 0, 0, 0, 0,
                  SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE );
    UpdateWindow( menu->hWnd );
    return TRUE;
}


/***********************************************************************
 *           MENU_EnsureMenuItemVisible
 */
static void
MENU_EnsureMenuItemVisible(LPPOPUPMENU lppop, UINT wIndex, HDC hdc)
{
    if (lppop->bScrolling)
    {
        MENUITEM *item = &lppop->items[wIndex];
        UINT nMaxHeight = MENU_GetMaxPopupHeight(lppop);
        UINT nOldPos = lppop->nScrollPos;
        RECT rc;
        UINT arrow_bitmap_height;
        BITMAP bmp;
        
        GetClientRect(lppop->hWnd, &rc);

        GetObjectW(get_down_arrow_bitmap(), sizeof(bmp), &bmp);
        arrow_bitmap_height = bmp.bmHeight;

        rc.top += arrow_bitmap_height;
        rc.bottom -= arrow_bitmap_height + MENU_BOTTOM_MARGIN;
       
        nMaxHeight -= GetSystemMetrics(SM_CYBORDER) + 2 * arrow_bitmap_height;
        if (item->rect.bottom > lppop->nScrollPos + nMaxHeight)
        {
            
            lppop->nScrollPos = item->rect.bottom - nMaxHeight;
            ScrollWindow(lppop->hWnd, 0, nOldPos - lppop->nScrollPos, &rc, &rc);
            MENU_DrawScrollArrows(lppop, hdc);
        }
        else if (item->rect.top - MENU_TOP_MARGIN < lppop->nScrollPos)
        {
            lppop->nScrollPos = item->rect.top - MENU_TOP_MARGIN;
            ScrollWindow(lppop->hWnd, 0, nOldPos - lppop->nScrollPos, &rc, &rc);
            MENU_DrawScrollArrows(lppop, hdc);
        }
    }
}


/***********************************************************************
 *           MENU_SelectItem
 */
static void MENU_SelectItem( HWND hwndOwner, HMENU hmenu, UINT wIndex,
                             BOOL sendMenuSelect, HMENU topmenu )
{
    LPPOPUPMENU lppop;
    HDC hdc;

    TRACE("owner=%p menu=%p index=0x%04x select=0x%04x\n", hwndOwner, hmenu, wIndex, sendMenuSelect);

    lppop = MENU_GetMenu( hmenu );
    if ((!lppop) || (!lppop->nItems) || (!lppop->hWnd)) return;

    if (lppop->FocusedItem == wIndex) return;
    if (lppop->wFlags & MF_POPUP) hdc = GetDC( lppop->hWnd );
    else hdc = GetDCEx( lppop->hWnd, 0, DCX_CACHE | DCX_WINDOW);
    if (!top_popup) {
        top_popup = lppop->hWnd;
        top_popup_hmenu = hmenu;
    }

    SelectObject( hdc, get_menu_font(FALSE));

      /* Clear previous highlighted item */
    if (lppop->FocusedItem != NO_SELECTED_ITEM)
    {
	lppop->items[lppop->FocusedItem].fState &= ~(MF_HILITE|MF_MOUSESELECT);
	MENU_DrawMenuItem(lppop->hWnd, hmenu, hwndOwner, hdc,&lppop->items[lppop->FocusedItem],
                          lppop->Height, !(lppop->wFlags & MF_POPUP),
			  ODA_SELECT );
    }

      /* Highlight new item (if any) */
    lppop->FocusedItem = wIndex;
    if (lppop->FocusedItem != NO_SELECTED_ITEM)
    {
        if(!(lppop->items[wIndex].fType & MF_SEPARATOR)) {
            lppop->items[wIndex].fState |= MF_HILITE;
            MENU_EnsureMenuItemVisible(lppop, wIndex, hdc);
            MENU_DrawMenuItem( lppop->hWnd, hmenu, hwndOwner, hdc,
                    &lppop->items[wIndex], lppop->Height,
                    !(lppop->wFlags & MF_POPUP), ODA_SELECT );
        }
        if (sendMenuSelect)
        {
            MENUITEM *ip = &lppop->items[lppop->FocusedItem];
	    SendMessageW( hwndOwner, WM_MENUSELECT,
                     MAKEWPARAM(ip->fType & MF_POPUP ? wIndex: ip->wID,
                     ip->fType | ip->fState |
                     (lppop->wFlags & MF_SYSMENU)), (LPARAM)hmenu);
        }
    }
    else if (sendMenuSelect) {
        if(topmenu){
            int pos;
            if((pos=MENU_FindSubMenu(&topmenu, hmenu))!=NO_SELECTED_ITEM){
                POPUPMENU *ptm = MENU_GetMenu( topmenu );
                MENUITEM *ip = &ptm->items[pos];
                SendMessageW( hwndOwner, WM_MENUSELECT, MAKEWPARAM(pos,
                         ip->fType | ip->fState |
                         (ptm->wFlags & MF_SYSMENU)), (LPARAM)topmenu);
            }
        }
    }
    ReleaseDC( lppop->hWnd, hdc );
}


/***********************************************************************
 *           MENU_MoveSelection
 *
 * Moves currently selected item according to the offset parameter.
 * If there is no selection then it should select the last item if
 * offset is ITEM_PREV or the first item if offset is ITEM_NEXT.
 */
static void MENU_MoveSelection( HWND hwndOwner, HMENU hmenu, INT offset )
{
    INT i;
    POPUPMENU *menu;

    TRACE("hwnd=%p hmenu=%p off=0x%04x\n", hwndOwner, hmenu, offset);

    menu = MENU_GetMenu( hmenu );
    if ((!menu) || (!menu->items)) return;

    if ( menu->FocusedItem != NO_SELECTED_ITEM )
    {
	if( menu->nItems == 1 ) return; else
	for (i = menu->FocusedItem + offset ; i >= 0 && i < menu->nItems
					    ; i += offset)
	    if (!(menu->items[i].fType & MF_SEPARATOR))
	    {
		MENU_SelectItem( hwndOwner, hmenu, i, TRUE, 0 );
		return;
	    }
    }

    for ( i = (offset > 0) ? 0 : menu->nItems - 1;
		  i >= 0 && i < menu->nItems ; i += offset)
	if (!(menu->items[i].fType & MF_SEPARATOR))
	{
	    MENU_SelectItem( hwndOwner, hmenu, i, TRUE, 0 );
	    return;
	}
}


/**********************************************************************
 *         MENU_InsertItem
 *
 * Insert (allocate) a new item into a menu.
 */
static MENUITEM *MENU_InsertItem( HMENU hMenu, UINT pos, UINT flags )
{
    MENUITEM *newItems;
    POPUPMENU *menu;

    if (!(menu = MENU_GetMenu(hMenu)))
        return NULL;

    /* Find where to insert new item */

    if (flags & MF_BYPOSITION) {
        if (pos > menu->nItems)
            pos = menu->nItems;
    } else {
        if (!MENU_FindItem( &hMenu, &pos, flags ))
            pos = menu->nItems;
        else {
            if (!(menu = MENU_GetMenu( hMenu )))
                return NULL;
        }
    }

    /* Make sure that MDI system buttons stay on the right side.
     * Note: XP treats only bitmap handles 1 - 6 as "magic" ones
     * regardless of their id.
     */
    while (pos > 0 && (INT_PTR)menu->items[pos - 1].hbmpItem >= (INT_PTR)HBMMENU_SYSTEM &&
           (INT_PTR)menu->items[pos - 1].hbmpItem <= (INT_PTR)HBMMENU_MBAR_CLOSE_D)
        pos--;

    TRACE("inserting at %u by pos %u\n", pos, flags & MF_BYPOSITION);

    /* Create new items array */

    newItems = HeapAlloc( GetProcessHeap(), 0, sizeof(MENUITEM) * (menu->nItems+1) );
    if (!newItems)
    {
        WARN("allocation failed\n" );
        return NULL;
    }
    if (menu->nItems > 0)
    {
	  /* Copy the old array into the new one */
	if (pos > 0) memcpy( newItems, menu->items, pos * sizeof(MENUITEM) );
	if (pos < menu->nItems) memcpy( &newItems[pos+1], &menu->items[pos],
					(menu->nItems-pos)*sizeof(MENUITEM) );
        HeapFree( GetProcessHeap(), 0, menu->items );
    }
    menu->items = newItems;
    menu->nItems++;
    memset( &newItems[pos], 0, sizeof(*newItems) );
    menu->Height = 0; /* force size recalculate */
    return &newItems[pos];
}


/**********************************************************************
 *         MENU_ParseResource
 *
 * Parse a standard menu resource and add items to the menu.
 * Return a pointer to the end of the resource.
 *
 * NOTE: flags is equivalent to the mtOption field
 */
static LPCSTR MENU_ParseResource( LPCSTR res, HMENU hMenu )
{
    WORD flags, id = 0;
    LPCWSTR str;
    BOOL end_flag;

    do
    {
        flags = GET_WORD(res);
        end_flag = flags & MF_END;
        /* Remove MF_END because it has the same value as MF_HILITE */
        flags &= ~MF_END;
        res += sizeof(WORD);
        if (!(flags & MF_POPUP))
        {
            id = GET_WORD(res);
            res += sizeof(WORD);
        }
        str = (LPCWSTR)res;
        res += (strlenW(str) + 1) * sizeof(WCHAR);
        if (flags & MF_POPUP)
        {
            HMENU hSubMenu = CreatePopupMenu();
            if (!hSubMenu) return NULL;
            if (!(res = MENU_ParseResource( res, hSubMenu ))) return NULL;
            AppendMenuW( hMenu, flags, (UINT_PTR)hSubMenu, str );
        }
        else  /* Not a popup */
        {
            AppendMenuW( hMenu, flags, id, *str ? str : NULL );
        }
    } while (!end_flag);
    return res;
}


/**********************************************************************
 *         MENUEX_ParseResource
 *
 * Parse an extended menu resource and add items to the menu.
 * Return a pointer to the end of the resource.
 */
static LPCSTR MENUEX_ParseResource( LPCSTR res, HMENU hMenu)
{
    WORD resinfo;
    do {
	MENUITEMINFOW mii;

	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_STATE | MIIM_ID | MIIM_TYPE;
	mii.fType = GET_DWORD(res);
        res += sizeof(DWORD);
	mii.fState = GET_DWORD(res);
        res += sizeof(DWORD);
	mii.wID = GET_DWORD(res);
        res += sizeof(DWORD);
	resinfo = GET_WORD(res); /* FIXME: for 16-bit apps this is a byte.  */
        res += sizeof(WORD);
	/* Align the text on a word boundary.  */
	res += (~((UINT_PTR)res - 1)) & 1;
	mii.dwTypeData = (LPWSTR) res;
	res += (1 + strlenW(mii.dwTypeData)) * sizeof(WCHAR);
	/* Align the following fields on a dword boundary.  */
	res += (~((UINT_PTR)res - 1)) & 3;

        TRACE("Menu item: [%08x,%08x,%04x,%04x,%s]\n",
              mii.fType, mii.fState, mii.wID, resinfo, debugstr_w(mii.dwTypeData));

	if (resinfo & 1) {	/* Pop-up? */
	    /* DWORD helpid = GET_DWORD(res); FIXME: use this.  */
	    res += sizeof(DWORD);
	    mii.hSubMenu = CreatePopupMenu();
	    if (!mii.hSubMenu)
		return NULL;
	    if (!(res = MENUEX_ParseResource(res, mii.hSubMenu))) {
		DestroyMenu(mii.hSubMenu);
                return NULL;
	    }
	    mii.fMask |= MIIM_SUBMENU;
	    mii.fType |= MF_POPUP;
        }
	else if(!*mii.dwTypeData && !(mii.fType & MF_SEPARATOR))
	{
	    WARN("Converting NULL menu item %04x, type %04x to SEPARATOR\n",
		mii.wID, mii.fType);
	    mii.fType |= MF_SEPARATOR;
	}
	InsertMenuItemW(hMenu, -1, MF_BYPOSITION, &mii);
    } while (!(resinfo & MF_END));
    return res;
}


/***********************************************************************
 *           MENU_GetSubPopup
 *
 * Return the handle of the selected sub-popup menu (if any).
 */
static HMENU MENU_GetSubPopup( HMENU hmenu )
{
    POPUPMENU *menu;
    MENUITEM *item;

    menu = MENU_GetMenu( hmenu );

    if ((!menu) || (menu->FocusedItem == NO_SELECTED_ITEM)) return 0;

    item = &menu->items[menu->FocusedItem];
    if ((item->fType & MF_POPUP) && (item->fState & MF_MOUSESELECT))
        return item->hSubMenu;
    return 0;
}


/***********************************************************************
 *           MENU_HideSubPopups
 *
 * Hide the sub-popup menus of this menu.
 */
static void MENU_HideSubPopups( HWND hwndOwner, HMENU hmenu,
                                BOOL sendMenuSelect, UINT wFlags )
{
    POPUPMENU *menu = MENU_GetMenu( hmenu );

    TRACE("owner=%p hmenu=%p 0x%04x\n", hwndOwner, hmenu, sendMenuSelect);

    if (menu && top_popup)
    {
	HMENU hsubmenu;
	POPUPMENU *submenu;
	MENUITEM *item;

	if (menu->FocusedItem != NO_SELECTED_ITEM)
	{
	    item = &menu->items[menu->FocusedItem];
	    if (!(item->fType & MF_POPUP) ||
		!(item->fState & MF_MOUSESELECT)) return;
	    item->fState &= ~MF_MOUSESELECT;
	    hsubmenu = item->hSubMenu;
	} else return;

	if (!(submenu = MENU_GetMenu( hsubmenu ))) return;
	MENU_HideSubPopups( hwndOwner, hsubmenu, FALSE, wFlags );
	MENU_SelectItem( hwndOwner, hsubmenu, NO_SELECTED_ITEM, sendMenuSelect, 0 );
        DestroyWindow( submenu->hWnd );
        submenu->hWnd = 0;

        if (!(wFlags & TPM_NONOTIFY))
           SendMessageW( hwndOwner, WM_UNINITMENUPOPUP, (WPARAM)hsubmenu,
                         MAKELPARAM(0, IS_SYSTEM_MENU(submenu)) );
    }
}


/***********************************************************************
 *           MENU_ShowSubPopup
 *
 * Display the sub-menu of the selected item of this menu.
 * Return the handle of the submenu, or hmenu if no submenu to display.
 */
static HMENU MENU_ShowSubPopup( HWND hwndOwner, HMENU hmenu,
                                  BOOL selectFirst, UINT wFlags )
{
    RECT rect;
    POPUPMENU *menu;
    MENUITEM *item;
    HDC hdc;

    TRACE("owner=%p hmenu=%p 0x%04x\n", hwndOwner, hmenu, selectFirst);

    if (!(menu = MENU_GetMenu( hmenu ))) return hmenu;

    if (menu->FocusedItem == NO_SELECTED_ITEM) return hmenu;

    item = &menu->items[menu->FocusedItem];
    if (!(item->fType & MF_POPUP) || (item->fState & (MF_GRAYED | MF_DISABLED)))
        return hmenu;

    /* message must be sent before using item,
       because nearly everything may be changed by the application ! */

    /* Send WM_INITMENUPOPUP message only if TPM_NONOTIFY flag is not specified */
    if (!(wFlags & TPM_NONOTIFY))
       SendMessageW( hwndOwner, WM_INITMENUPOPUP, (WPARAM)item->hSubMenu,
                     MAKELPARAM( menu->FocusedItem, IS_SYSTEM_MENU(menu) ));

    item = &menu->items[menu->FocusedItem];
    rect = item->rect;

    /* correct item if modified as a reaction to WM_INITMENUPOPUP message */
    if (!(item->fState & MF_HILITE))
    {
        if (menu->wFlags & MF_POPUP) hdc = GetDC( menu->hWnd );
        else hdc = GetDCEx( menu->hWnd, 0, DCX_CACHE | DCX_WINDOW);

        SelectObject( hdc, get_menu_font(FALSE));

        item->fState |= MF_HILITE;
        MENU_DrawMenuItem( menu->hWnd, hmenu, hwndOwner, hdc, item, menu->Height, !(menu->wFlags & MF_POPUP), ODA_DRAWENTIRE );
	ReleaseDC( menu->hWnd, hdc );
    }
    if (!item->rect.top && !item->rect.left && !item->rect.bottom && !item->rect.right)
      item->rect = rect;

    item->fState |= MF_MOUSESELECT;

    if (IS_SYSTEM_MENU(menu))
    {
	MENU_InitSysMenuPopup(item->hSubMenu,
                              GetWindowLongW( menu->hWnd, GWL_STYLE ),
                              GetClassLongW( menu->hWnd, GCL_STYLE));

	NC_GetSysPopupPos( menu->hWnd, &rect );
	if (wFlags & TPM_LAYOUTRTL) rect.left = rect.right;
	rect.top = rect.bottom;
	rect.right = GetSystemMetrics(SM_CXSIZE);
        rect.bottom = GetSystemMetrics(SM_CYSIZE);
    }
    else
    {
        GetWindowRect( menu->hWnd, &rect );
	if (menu->wFlags & MF_POPUP)
	{
            RECT rc = item->rect;

            MENU_AdjustMenuItemRect(menu, &rc);

	    /* The first item in the popup menu has to be at the
	       same y position as the focused menu item */
            if (wFlags & TPM_LAYOUTRTL)
                rect.left += GetSystemMetrics(SM_CXBORDER);
	    else
                rect.left += rc.right - GetSystemMetrics(SM_CXBORDER);
	    rect.top += rc.top - MENU_TOP_MARGIN;
	    rect.right = rc.left - rc.right + GetSystemMetrics(SM_CXBORDER);
	    rect.bottom = rc.top - rc.bottom - MENU_TOP_MARGIN
	                  - MENU_BOTTOM_MARGIN - GetSystemMetrics(SM_CYBORDER);
	}
	else
	{
            if (wFlags & TPM_LAYOUTRTL)
                rect.left = rect.right - item->rect.left;
	    else
                rect.left += item->rect.left;
	    rect.top += item->rect.bottom;
	    rect.right = item->rect.right - item->rect.left;
	    rect.bottom = item->rect.bottom - item->rect.top;
	}
    }

    MENU_ShowPopup( hwndOwner, item->hSubMenu, menu->FocusedItem, wFlags,
		    rect.left, rect.top, rect.right, rect.bottom );
    if (selectFirst)
        MENU_MoveSelection( hwndOwner, item->hSubMenu, ITEM_NEXT );
    return item->hSubMenu;
}



/**********************************************************************
 *         MENU_IsMenuActive
 */
HWND MENU_IsMenuActive(void)
{
    return top_popup;
}

/**********************************************************************
 *         MENU_EndMenu
 *
 * Calls EndMenu() if the hwnd parameter belongs to the menu owner
 *
 * Does the (menu stuff) of the default window handling of WM_CANCELMODE
 */
void MENU_EndMenu( HWND hwnd )
{
    POPUPMENU *menu;
    menu = top_popup_hmenu ? MENU_GetMenu( top_popup_hmenu ) : NULL;
    if (menu && hwnd == menu->hwndOwner) EndMenu();
}

/***********************************************************************
 *           MENU_PtMenu
 *
 * Walks menu chain trying to find a menu pt maps to.
 */
static HMENU MENU_PtMenu( HMENU hMenu, POINT pt )
{
   POPUPMENU *menu = MENU_GetMenu( hMenu );
   UINT item = menu->FocusedItem;
   HMENU ret;

   /* try subpopup first (if any) */
   ret = (item != NO_SELECTED_ITEM &&
          (menu->items[item].fType & MF_POPUP) &&
          (menu->items[item].fState & MF_MOUSESELECT))
        ? MENU_PtMenu(menu->items[item].hSubMenu, pt) : 0;

   if (!ret)  /* check the current window (avoiding WM_HITTEST) */
   {
       INT ht = NC_HandleNCHitTest( menu->hWnd, pt );
       if( menu->wFlags & MF_POPUP )
       {
           if (ht != HTNOWHERE && ht != HTERROR) ret = hMenu;
       }
       else if (ht == HTSYSMENU)
           ret = get_win_sys_menu( menu->hWnd );
       else if (ht == HTMENU)
           ret = GetMenu( menu->hWnd );
   }
   return ret;
}

/***********************************************************************
 *           MENU_ExecFocusedItem
 *
 * Execute a menu item (for instance when user pressed Enter).
 * Return the wID of the executed item. Otherwise, -1 indicating
 * that no menu item was executed, -2 if a popup is shown;
 * Have to receive the flags for the TrackPopupMenu options to avoid
 * sending unwanted message.
 *
 */
static INT MENU_ExecFocusedItem( MTRACKER* pmt, HMENU hMenu, UINT wFlags )
{
    MENUITEM *item;
    POPUPMENU *menu = MENU_GetMenu( hMenu );

    TRACE("%p hmenu=%p\n", pmt, hMenu);

    if (!menu || !menu->nItems ||
	(menu->FocusedItem == NO_SELECTED_ITEM)) return -1;

    item = &menu->items[menu->FocusedItem];

    TRACE("hMenu %p wID %08lx hSubMenu %p fType %04x\n", hMenu, item->wID, item->hSubMenu, item->fType);

    if (!(item->fType & MF_POPUP))
    {
	if (!(item->fState & (MF_GRAYED | MF_DISABLED)) && !(item->fType & MF_SEPARATOR))
	{
	    /* If TPM_RETURNCMD is set you return the id, but
	       do not send a message to the owner */
	    if(!(wFlags & TPM_RETURNCMD))
	    {
		if( menu->wFlags & MF_SYSMENU )
		    PostMessageW( pmt->hOwnerWnd, WM_SYSCOMMAND, item->wID,
				  MAKELPARAM((INT16)pmt->pt.x, (INT16)pmt->pt.y) );
		else
		{
                    POPUPMENU *topmenu = MENU_GetMenu( pmt->hTopMenu );
                    DWORD dwStyle = menu->dwStyle | (topmenu ? topmenu->dwStyle : 0);

                    if (dwStyle & MNS_NOTIFYBYPOS)
                        PostMessageW( pmt->hOwnerWnd, WM_MENUCOMMAND, menu->FocusedItem,
                                      (LPARAM)hMenu);
                    else
                        PostMessageW( pmt->hOwnerWnd, WM_COMMAND, item->wID, 0 );
		}
	    }
	    return item->wID;
	}
    }
    else
    {
	pmt->hCurrentMenu = MENU_ShowSubPopup(pmt->hOwnerWnd, hMenu, TRUE, wFlags);
	return -2;
    }

    return -1;
}

/***********************************************************************
 *           MENU_SwitchTracking
 *
 * Helper function for menu navigation routines.
 */
static void MENU_SwitchTracking( MTRACKER* pmt, HMENU hPtMenu, UINT id, UINT wFlags )
{
    POPUPMENU *ptmenu = MENU_GetMenu( hPtMenu );
    POPUPMENU *topmenu = MENU_GetMenu( pmt->hTopMenu );

    TRACE("%p hmenu=%p 0x%04x\n", pmt, hPtMenu, id);

    if( pmt->hTopMenu != hPtMenu &&
	!((ptmenu->wFlags | topmenu->wFlags) & MF_POPUP) )
    {
	/* both are top level menus (system and menu-bar) */
	MENU_HideSubPopups( pmt->hOwnerWnd, pmt->hTopMenu, FALSE, wFlags );
	MENU_SelectItem( pmt->hOwnerWnd, pmt->hTopMenu, NO_SELECTED_ITEM, FALSE, 0 );
        pmt->hTopMenu = hPtMenu;
    }
    else MENU_HideSubPopups( pmt->hOwnerWnd, hPtMenu, FALSE, wFlags );
    MENU_SelectItem( pmt->hOwnerWnd, hPtMenu, id, TRUE, 0 );
}


/***********************************************************************
 *           MENU_ButtonDown
 *
 * Return TRUE if we can go on with menu tracking.
 */
static BOOL MENU_ButtonDown( MTRACKER* pmt, HMENU hPtMenu, UINT wFlags )
{
    TRACE("%p hPtMenu=%p\n", pmt, hPtMenu);

    if (hPtMenu)
    {
	UINT id = 0;
	POPUPMENU *ptmenu = MENU_GetMenu( hPtMenu );
	MENUITEM *item;

	if( IS_SYSTEM_MENU(ptmenu) )
	    item = ptmenu->items;
	else
	    item = MENU_FindItemByCoords( ptmenu, pmt->pt, &id );

	if( item )
	{
	    if( ptmenu->FocusedItem != id )
		MENU_SwitchTracking( pmt, hPtMenu, id, wFlags );

	    /* If the popup menu is not already "popped" */
	    if(!(item->fState & MF_MOUSESELECT ))
	    {
		pmt->hCurrentMenu = MENU_ShowSubPopup( pmt->hOwnerWnd, hPtMenu, FALSE, wFlags );
	    }

	    return TRUE;
	}
	/* Else the click was on the menu bar, finish the tracking */
    }
    return FALSE;
}

/***********************************************************************
 *           MENU_ButtonUp
 *
 * Return the value of MENU_ExecFocusedItem if
 * the selected item was not a popup. Else open the popup.
 * A -1 return value indicates that we go on with menu tracking.
 *
 */
static INT MENU_ButtonUp( MTRACKER* pmt, HMENU hPtMenu, UINT wFlags)
{
    TRACE("%p hmenu=%p\n", pmt, hPtMenu);

    if (hPtMenu)
    {
	UINT id = 0;
	POPUPMENU *ptmenu = MENU_GetMenu( hPtMenu );
	MENUITEM *item;

        if( IS_SYSTEM_MENU(ptmenu) )
            item = ptmenu->items;
        else
            item = MENU_FindItemByCoords( ptmenu, pmt->pt, &id );

	if( item && (ptmenu->FocusedItem == id ))
	{
            debug_print_menuitem ("FocusedItem: ", item, "");

	    if( !(item->fType & MF_POPUP) )
	    {
	        INT executedMenuId = MENU_ExecFocusedItem( pmt, hPtMenu, wFlags);
                if (executedMenuId == -1 || executedMenuId == -2) return -1;
                return executedMenuId;
	    }

	    /* If we are dealing with the top-level menu            */
	    /* and this is a click on an already "popped" item:     */
	    /* Stop the menu tracking and close the opened submenus */
	    if((pmt->hTopMenu == hPtMenu) && ptmenu->bTimeToHide)
		return 0;
	}
	ptmenu->bTimeToHide = TRUE;
    }
    return -1;
}


/***********************************************************************
 *           MENU_MouseMove
 *
 * Return TRUE if we can go on with menu tracking.
 */
static BOOL MENU_MouseMove( MTRACKER* pmt, HMENU hPtMenu, UINT wFlags )
{
    UINT id = NO_SELECTED_ITEM;
    POPUPMENU *ptmenu = NULL;

    if( hPtMenu )
    {
	ptmenu = MENU_GetMenu( hPtMenu );
        if( IS_SYSTEM_MENU(ptmenu) )
	    id = 0;
        else
            MENU_FindItemByCoords( ptmenu, pmt->pt, &id );
    }

    if( id == NO_SELECTED_ITEM )
    {
	MENU_SelectItem( pmt->hOwnerWnd, pmt->hCurrentMenu,
			 NO_SELECTED_ITEM, TRUE, pmt->hTopMenu);

    }
    else if( ptmenu->FocusedItem != id )
    {
	    MENU_SwitchTracking( pmt, hPtMenu, id, wFlags );
	    pmt->hCurrentMenu = MENU_ShowSubPopup(pmt->hOwnerWnd, hPtMenu, FALSE, wFlags);
    }
    return TRUE;
}


/***********************************************************************
 *           MENU_DoNextMenu
 *
 * NOTE: WM_NEXTMENU documented in Win32 is a bit different.
 */
static LRESULT MENU_DoNextMenu( MTRACKER* pmt, UINT vk, UINT wFlags )
{
    POPUPMENU *menu = MENU_GetMenu( pmt->hTopMenu );
    BOOL atEnd = FALSE;

    /* When skipping left, we need to do something special after the
       first menu.                                                  */
    if (vk == VK_LEFT && menu->FocusedItem == 0)
    {
        atEnd = TRUE;
    }
    /* When skipping right, for the non-system menu, we need to
       handle the last non-special menu item (ie skip any window
       icons such as MDI maximize, restore or close)             */
    else if ((vk == VK_RIGHT) && !IS_SYSTEM_MENU(menu))
    {
        UINT i = menu->FocusedItem + 1;
        while (i < menu->nItems) {
            if ((menu->items[i].wID >= SC_SIZE &&
                 menu->items[i].wID <= SC_RESTORE)) {
                i++;
            } else break;
        }
        if (i == menu->nItems) {
            atEnd = TRUE;
        }
    }
    /* When skipping right, we need to cater for the system menu */
    else if ((vk == VK_RIGHT) && IS_SYSTEM_MENU(menu))
    {
        if (menu->FocusedItem == (menu->nItems - 1)) {
            atEnd = TRUE;
        }
    }

    if( atEnd )
    {
        MDINEXTMENU next_menu;
	HMENU hNewMenu;
	HWND  hNewWnd;
	UINT  id = 0;

        next_menu.hmenuIn = (IS_SYSTEM_MENU(menu)) ? GetSubMenu(pmt->hTopMenu,0) : pmt->hTopMenu;
        next_menu.hmenuNext = 0;
        next_menu.hwndNext = 0;
        SendMessageW( pmt->hOwnerWnd, WM_NEXTMENU, vk, (LPARAM)&next_menu );

        TRACE("%p [%p] -> %p [%p]\n",
              pmt->hCurrentMenu, pmt->hOwnerWnd, next_menu.hmenuNext, next_menu.hwndNext );

	if (!next_menu.hmenuNext || !next_menu.hwndNext)
	{
            DWORD style = GetWindowLongW( pmt->hOwnerWnd, GWL_STYLE );
	    hNewWnd = pmt->hOwnerWnd;
	    if( IS_SYSTEM_MENU(menu) )
	    {
		/* switch to the menu bar */

                if(style & WS_CHILD || !(hNewMenu = GetMenu(hNewWnd))) return FALSE;

	        if( vk == VK_LEFT )
	        {
		    menu = MENU_GetMenu( hNewMenu );
		    id = menu->nItems - 1;

                    /* Skip backwards over any system predefined icons,
                       eg. MDI close, restore etc icons                 */
                    while ((id > 0) &&
                           (menu->items[id].wID >= SC_SIZE &&
                            menu->items[id].wID <= SC_RESTORE)) id--;
	        }
	    }
	    else if (style & WS_SYSMENU )
	    {
		/* switch to the system menu */
	        hNewMenu = get_win_sys_menu( hNewWnd );
	    }
            else return FALSE;
	}
	else    /* application returned a new menu to switch to */
	{
            hNewMenu = next_menu.hmenuNext;
            hNewWnd = WIN_GetFullHandle( next_menu.hwndNext );

	    if( IsMenu(hNewMenu) && IsWindow(hNewWnd) )
	    {
                DWORD style = GetWindowLongW( hNewWnd, GWL_STYLE );

		if (style & WS_SYSMENU &&
		    GetSubMenu(get_win_sys_menu(hNewWnd), 0) == hNewMenu )
		{
	            /* get the real system menu */
		    hNewMenu =  get_win_sys_menu(hNewWnd);
		}
	        else if (style & WS_CHILD || GetMenu(hNewWnd) != hNewMenu )
		{
		    /* FIXME: Not sure what to do here;
		     * perhaps try to track hNewMenu as a popup? */

		    TRACE(" -- got confused.\n");
		    return FALSE;
		}
	    }
	    else return FALSE;
	}

	if( hNewMenu != pmt->hTopMenu )
	{
	    MENU_SelectItem( pmt->hOwnerWnd, pmt->hTopMenu, NO_SELECTED_ITEM,
                    FALSE, 0 );
	    if( pmt->hCurrentMenu != pmt->hTopMenu )
		MENU_HideSubPopups( pmt->hOwnerWnd, pmt->hTopMenu, FALSE, wFlags );
	}

	if( hNewWnd != pmt->hOwnerWnd )
	{
	    pmt->hOwnerWnd = hNewWnd;
            set_capture_window( pmt->hOwnerWnd, GUI_INMENUMODE, NULL );
	}

	pmt->hTopMenu = pmt->hCurrentMenu = hNewMenu; /* all subpopups are hidden */
	MENU_SelectItem( pmt->hOwnerWnd, pmt->hTopMenu, id, TRUE, 0 );

	return TRUE;
    }
    return FALSE;
}

/***********************************************************************
 *           MENU_SuspendPopup
 *
 * The idea is not to show the popup if the next input message is
 * going to hide it anyway.
 */
static BOOL MENU_SuspendPopup( MTRACKER* pmt, UINT16 uMsg )
{
    MSG msg;

    msg.hwnd = pmt->hOwnerWnd;

    PeekMessageW( &msg, 0, uMsg, uMsg, PM_NOYIELD | PM_REMOVE);
    pmt->trackFlags |= TF_SKIPREMOVE;

    switch( uMsg )
    {
	case WM_KEYDOWN:
	     PeekMessageW( &msg, 0, 0, 0, PM_NOYIELD | PM_NOREMOVE);
	     if( msg.message == WM_KEYUP || msg.message == WM_PAINT )
	     {
		 PeekMessageW( &msg, 0, 0, 0, PM_NOYIELD | PM_REMOVE);
	         PeekMessageW( &msg, 0, 0, 0, PM_NOYIELD | PM_NOREMOVE);
	         if( msg.message == WM_KEYDOWN &&
		    (msg.wParam == VK_LEFT || msg.wParam == VK_RIGHT))
	         {
		     pmt->trackFlags |= TF_SUSPENDPOPUP;
		     return TRUE;
	         }
	     }
	     break;
    }

    /* failures go through this */
    pmt->trackFlags &= ~TF_SUSPENDPOPUP;
    return FALSE;
}

/***********************************************************************
 *           MENU_KeyEscape
 *
 * Handle a VK_ESCAPE key event in a menu.
 */
static BOOL MENU_KeyEscape(MTRACKER* pmt, UINT wFlags)
{
    BOOL bEndMenu = TRUE;

    if (pmt->hCurrentMenu != pmt->hTopMenu)
    {
        POPUPMENU *menu = MENU_GetMenu(pmt->hCurrentMenu);

        if (menu->wFlags & MF_POPUP)
        {
            HMENU hmenutmp, hmenuprev;

            hmenuprev = hmenutmp = pmt->hTopMenu;

            /* close topmost popup */
            while (hmenutmp != pmt->hCurrentMenu)
            {
                hmenuprev = hmenutmp;
                hmenutmp = MENU_GetSubPopup( hmenuprev );
            }

            MENU_HideSubPopups( pmt->hOwnerWnd, hmenuprev, TRUE, wFlags );
            pmt->hCurrentMenu = hmenuprev;
            bEndMenu = FALSE;
        }
    }

    return bEndMenu;
}

/***********************************************************************
 *           MENU_KeyLeft
 *
 * Handle a VK_LEFT key event in a menu.
 */
static void MENU_KeyLeft( MTRACKER* pmt, UINT wFlags )
{
    POPUPMENU *menu;
    HMENU hmenutmp, hmenuprev;
    UINT  prevcol;

    hmenuprev = hmenutmp = pmt->hTopMenu;
    menu = MENU_GetMenu( hmenutmp );

    /* Try to move 1 column left (if possible) */
    if( (prevcol = MENU_GetStartOfPrevColumn( pmt->hCurrentMenu )) !=
	NO_SELECTED_ITEM ) {

	MENU_SelectItem( pmt->hOwnerWnd, pmt->hCurrentMenu,
			 prevcol, TRUE, 0 );
	return;
    }

    /* close topmost popup */
    while (hmenutmp != pmt->hCurrentMenu)
    {
	hmenuprev = hmenutmp;
	hmenutmp = MENU_GetSubPopup( hmenuprev );
    }

    MENU_HideSubPopups( pmt->hOwnerWnd, hmenuprev, TRUE, wFlags );
    pmt->hCurrentMenu = hmenuprev;

    if ( (hmenuprev == pmt->hTopMenu) && !(menu->wFlags & MF_POPUP) )
    {
	/* move menu bar selection if no more popups are left */

	if( !MENU_DoNextMenu( pmt, VK_LEFT, wFlags ) )
	     MENU_MoveSelection( pmt->hOwnerWnd, pmt->hTopMenu, ITEM_PREV );

	if ( hmenuprev != hmenutmp || pmt->trackFlags & TF_SUSPENDPOPUP )
	{
	   /* A sublevel menu was displayed - display the next one
	    * unless there is another displacement coming up */

	    if( !MENU_SuspendPopup( pmt, WM_KEYDOWN ) )
		pmt->hCurrentMenu = MENU_ShowSubPopup(pmt->hOwnerWnd,
						pmt->hTopMenu, TRUE, wFlags);
	}
    }
}


/***********************************************************************
 *           MENU_KeyRight
 *
 * Handle a VK_RIGHT key event in a menu.
 */
static void MENU_KeyRight( MTRACKER* pmt, UINT wFlags )
{
    HMENU hmenutmp;
    POPUPMENU *menu = MENU_GetMenu( pmt->hTopMenu );
    UINT  nextcol;

    TRACE("MENU_KeyRight called, cur %p (%s), top %p (%s).\n",
          pmt->hCurrentMenu,
          debugstr_w((MENU_GetMenu(pmt->hCurrentMenu))->items[0].text),
          pmt->hTopMenu, debugstr_w(menu->items[0].text) );

    if ( (menu->wFlags & MF_POPUP) || (pmt->hCurrentMenu != pmt->hTopMenu))
    {
	/* If already displaying a popup, try to display sub-popup */

	hmenutmp = pmt->hCurrentMenu;
	pmt->hCurrentMenu = MENU_ShowSubPopup(pmt->hOwnerWnd, hmenutmp, TRUE, wFlags);

	/* if subpopup was displayed then we are done */
	if (hmenutmp != pmt->hCurrentMenu) return;
    }

    /* Check to see if there's another column */
    if( (nextcol = MENU_GetStartOfNextColumn( pmt->hCurrentMenu )) !=
	NO_SELECTED_ITEM ) {
	TRACE("Going to %d.\n", nextcol );
	MENU_SelectItem( pmt->hOwnerWnd, pmt->hCurrentMenu,
			 nextcol, TRUE, 0 );
	return;
    }

    if (!(menu->wFlags & MF_POPUP))	/* menu bar tracking */
    {
	if( pmt->hCurrentMenu != pmt->hTopMenu )
	{
	    MENU_HideSubPopups( pmt->hOwnerWnd, pmt->hTopMenu, FALSE, wFlags );
	    hmenutmp = pmt->hCurrentMenu = pmt->hTopMenu;
	} else hmenutmp = 0;

	/* try to move to the next item */
	if( !MENU_DoNextMenu( pmt, VK_RIGHT, wFlags ) )
	     MENU_MoveSelection( pmt->hOwnerWnd, pmt->hTopMenu, ITEM_NEXT );

	if( hmenutmp || pmt->trackFlags & TF_SUSPENDPOPUP )
	    if( !MENU_SuspendPopup(pmt, WM_KEYDOWN) )
		pmt->hCurrentMenu = MENU_ShowSubPopup(pmt->hOwnerWnd,
						       pmt->hTopMenu, TRUE, wFlags);
    }
}

static void CALLBACK release_capture( BOOL __normal )
{
    set_capture_window( 0, GUI_INMENUMODE, NULL );
}

/***********************************************************************
 *           MENU_TrackMenu
 *
 * Menu tracking code.
 */
static BOOL MENU_TrackMenu( HMENU hmenu, UINT wFlags, INT x, INT y,
                            HWND hwnd, const RECT *lprect )
{
    MSG msg;
    POPUPMENU *menu;
    BOOL fRemove;
    INT executedMenuId = -1;
    MTRACKER mt;
    BOOL enterIdleSent = FALSE;
    HWND capture_win;

    mt.trackFlags = 0;
    mt.hCurrentMenu = hmenu;
    mt.hTopMenu = hmenu;
    mt.hOwnerWnd = WIN_GetFullHandle( hwnd );
    mt.pt.x = x;
    mt.pt.y = y;

    TRACE("hmenu=%p flags=0x%08x (%d,%d) hwnd=%p %s\n",
          hmenu, wFlags, x, y, hwnd, wine_dbgstr_rect( lprect));

    fEndMenu = FALSE;
    if (!(menu = MENU_GetMenu( hmenu )))
    {
        WARN("Invalid menu handle %p\n", hmenu);
        SetLastError(ERROR_INVALID_MENU_HANDLE);
        return FALSE;
    }

    if (wFlags & TPM_BUTTONDOWN)
    {
	/* Get the result in order to start the tracking or not */
	fRemove = MENU_ButtonDown( &mt, hmenu, wFlags );
	fEndMenu = !fRemove;
    }

    if (wFlags & TF_ENDMENU) fEndMenu = TRUE;

    /* owner may not be visible when tracking a popup, so use the menu itself */
    capture_win = (wFlags & TPM_POPUPMENU) ? menu->hWnd : mt.hOwnerWnd;
    set_capture_window( capture_win, GUI_INMENUMODE, NULL );

    /*__TRY*/ while (!fEndMenu)
    {
	menu = MENU_GetMenu( mt.hCurrentMenu );
	if (!menu) /* sometimes happens if I do a window manager close */
	    break;

	/* we have to keep the message in the queue until it's
	 * clear that menu loop is not over yet. */

        for (;;)
        {
            if (PeekMessageW( &msg, 0, 0, 0, PM_NOREMOVE ))
            {
                if (!CallMsgFilterW( &msg, MSGF_MENU )) break;
                /* remove the message from the queue */
                PeekMessageW( &msg, 0, msg.message, msg.message, PM_REMOVE );
            }
            else
            {
                if (!enterIdleSent)
                {
                    HWND win = menu->wFlags & MF_POPUP ? menu->hWnd : 0;
                    enterIdleSent = TRUE;
                    SendMessageW( mt.hOwnerWnd, WM_ENTERIDLE, MSGF_MENU, (LPARAM)win );
                }
                WaitMessage();
            }
        }

	/* check if EndMenu() tried to cancel us, by posting this message */
        if(msg.message == WM_CANCELMODE)
	{
	    /* we are now out of the loop */
    	    fEndMenu = TRUE;

	    /* remove the message from the queue */
	    PeekMessageW( &msg, 0, msg.message, msg.message, PM_REMOVE );

	    /* break out of internal loop, ala ESCAPE */
	    break;
	}

        TranslateMessage( &msg );
        mt.pt = msg.pt;

	if ( (msg.hwnd==menu->hWnd) || (msg.message!=WM_TIMER) )
	  enterIdleSent=FALSE;

        fRemove = FALSE;
	if ((msg.message >= WM_MOUSEFIRST) && (msg.message <= WM_MOUSELAST))
	{
            /*
             * Use the mouse coordinates in lParam instead of those in the MSG
             * struct to properly handle synthetic messages. They are already
             * in screen coordinates.
             */
            mt.pt.x = (short)LOWORD(msg.lParam);
            mt.pt.y = (short)HIWORD(msg.lParam);

	    /* Find a menu for this mouse event */
	    hmenu = MENU_PtMenu( mt.hTopMenu, mt.pt );

	    switch(msg.message)
	    {
		/* no WM_NC... messages in captured state */

		case WM_RBUTTONDBLCLK:
		case WM_RBUTTONDOWN:
		    if (!(wFlags & TPM_RIGHTBUTTON)) break;
		    /* fall through */
		case WM_LBUTTONDBLCLK:
		case WM_LBUTTONDOWN:
		    /* If the message belongs to the menu, removes it from the queue */
		    /* Else, end menu tracking */
		    fRemove = MENU_ButtonDown( &mt, hmenu, wFlags );
		    fEndMenu = !fRemove;
		    break;

		case WM_RBUTTONUP:
		    if (!(wFlags & TPM_RIGHTBUTTON)) break;
		    /* fall through */
		case WM_LBUTTONUP:
		    /* Check if a menu was selected by the mouse */
		    if (hmenu)
		    {
                        executedMenuId = MENU_ButtonUp( &mt, hmenu, wFlags);
                        TRACE("executedMenuId %d\n", executedMenuId);

			/* End the loop if executedMenuId is an item ID */
			/* or if the job was done (executedMenuId = 0). */
                        fEndMenu = fRemove = (executedMenuId != -1);
		    }
                    /* No menu was selected by the mouse */
                    /* if the function was called by TrackPopupMenu, continue
                       with the menu tracking. If not, stop it */
                    else
                        fEndMenu = ((wFlags & TPM_POPUPMENU) ? FALSE : TRUE);

		    break;

		case WM_MOUSEMOVE:
                    /* the selected menu item must be changed every time */
		     /* the mouse moves. */

                    if (hmenu)
			fEndMenu |= !MENU_MouseMove( &mt, hmenu, wFlags );

	    } /* switch(msg.message) - mouse */
	}
	else if ((msg.message >= WM_KEYFIRST) && (msg.message <= WM_KEYLAST))
	{
            fRemove = TRUE;  /* Keyboard messages are always removed */
	    switch(msg.message)
	    {
	    case WM_KEYDOWN:
	    case WM_SYSKEYDOWN:
		switch(msg.wParam)
		{
		case VK_MENU:
		case VK_F10:
		    fEndMenu = TRUE;
		    break;

		case VK_HOME:
		case VK_END:
		    MENU_SelectItem( mt.hOwnerWnd, mt.hCurrentMenu,
				     NO_SELECTED_ITEM, FALSE, 0 );
		    MENU_MoveSelection( mt.hOwnerWnd, mt.hCurrentMenu,
				       (msg.wParam == VK_HOME)? ITEM_NEXT : ITEM_PREV );
		    break;

		case VK_UP:
		case VK_DOWN: /* If on menu bar, pull-down the menu */

		    menu = MENU_GetMenu( mt.hCurrentMenu );
		    if (!(menu->wFlags & MF_POPUP))
			mt.hCurrentMenu = MENU_ShowSubPopup(mt.hOwnerWnd, mt.hTopMenu, TRUE, wFlags);
		    else      /* otherwise try to move selection */
			MENU_MoveSelection( mt.hOwnerWnd, mt.hCurrentMenu, 
                                       (msg.wParam == VK_UP)? ITEM_PREV : ITEM_NEXT );
		    break;

		case VK_LEFT:
		    MENU_KeyLeft( &mt, wFlags );
		    break;

		case VK_RIGHT:
		    MENU_KeyRight( &mt, wFlags );
		    break;

		case VK_ESCAPE:
                    fEndMenu = MENU_KeyEscape(&mt, wFlags);
		    break;

		case VK_F1:
		    {
			HELPINFO hi;
			hi.cbSize = sizeof(HELPINFO);
			hi.iContextType = HELPINFO_MENUITEM;
			if (menu->FocusedItem == NO_SELECTED_ITEM)
			    hi.iCtrlId = 0;
		        else
			    hi.iCtrlId = menu->items[menu->FocusedItem].wID;
			hi.hItemHandle = hmenu;
			hi.dwContextId = menu->dwContextHelpID;
			hi.MousePos = msg.pt;
			SendMessageW(hwnd, WM_HELP, 0, (LPARAM)&hi);
			break;
		    }

		default:
		    break;
		}
		break;  /* WM_KEYDOWN */

	    case WM_CHAR:
	    case WM_SYSCHAR:
		{
		    UINT	pos;

		    if (msg.wParam == '\r' || msg.wParam == ' ')
		    {
                        executedMenuId = MENU_ExecFocusedItem(&mt,mt.hCurrentMenu, wFlags);
                        fEndMenu = (executedMenuId != -2);

			break;
		    }

		      /* Hack to avoid control chars. */
		      /* We will find a better way real soon... */
		    if (msg.wParam < 32) break;

		    pos = MENU_FindItemByKey( mt.hOwnerWnd, mt.hCurrentMenu,
                                              LOWORD(msg.wParam), FALSE );
		    if (pos == (UINT)-2) fEndMenu = TRUE;
		    else if (pos == (UINT)-1) MessageBeep(0);
		    else
		    {
			MENU_SelectItem( mt.hOwnerWnd, mt.hCurrentMenu, pos,
                                TRUE, 0 );
                        executedMenuId = MENU_ExecFocusedItem(&mt,mt.hCurrentMenu, wFlags);
                        fEndMenu = (executedMenuId != -2);
		    }
		}
		break;
	    }  /* switch(msg.message) - kbd */
	}
	else
	{
	    PeekMessageW( &msg, 0, msg.message, msg.message, PM_REMOVE );
	    DispatchMessageW( &msg );
	    continue;
	}

	if (!fEndMenu) fRemove = TRUE;

	/* finally remove message from the queue */

        if (fRemove && !(mt.trackFlags & TF_SKIPREMOVE) )
	    PeekMessageW( &msg, 0, msg.message, msg.message, PM_REMOVE );
	else mt.trackFlags &= ~TF_SKIPREMOVE;
    }
    /*__FINALLY(*/ release_capture(TRUE); /*)*/

    /* If dropdown is still painted and the close box is clicked on
       then the menu will be destroyed as part of the DispatchMessage above.
       This will then invalidate the menu handle in mt.hTopMenu. We should
       check for this first.  */
    if( IsMenu( mt.hTopMenu ) )
    {
	menu = MENU_GetMenu( mt.hTopMenu );

        if( IsWindow( mt.hOwnerWnd ) )
        {
	    MENU_HideSubPopups( mt.hOwnerWnd, mt.hTopMenu, FALSE, wFlags );

	    if (menu && (menu->wFlags & MF_POPUP))
	    {
                DestroyWindow( menu->hWnd );
                menu->hWnd = 0;

                if (!(wFlags & TPM_NONOTIFY))
                   SendMessageW( mt.hOwnerWnd, WM_UNINITMENUPOPUP, (WPARAM)mt.hTopMenu,
                                 MAKELPARAM(0, IS_SYSTEM_MENU(menu)) );
	    }
	    MENU_SelectItem( mt.hOwnerWnd, mt.hTopMenu, NO_SELECTED_ITEM, FALSE, 0 );
	    SendMessageW( mt.hOwnerWnd, WM_MENUSELECT, MAKEWPARAM(0,0xffff), 0 );
        }

        /* Reset the variable for hiding menu */
        if( menu ) menu->bTimeToHide = FALSE;
    }

    /* The return value is only used by TrackPopupMenu */
    if (!(wFlags & TPM_RETURNCMD)) return TRUE;
    if (executedMenuId == -1) executedMenuId = 0;
    return executedMenuId;
}

/***********************************************************************
 *           MENU_InitTracking
 */
static BOOL MENU_InitTracking(HWND hWnd, HMENU hMenu, BOOL bPopup, UINT wFlags)
{
    POPUPMENU *menu;
    
    TRACE("hwnd=%p hmenu=%p\n", hWnd, hMenu);

    HideCaret(0);

    /* This makes the menus of applications built with Delphi work.
     * It also enables menus to be displayed in more than one window,
     * but there are some bugs left that need to be fixed in this case.
     */
    if ((menu = MENU_GetMenu( hMenu ))) menu->hWnd = hWnd;

    /* Send WM_ENTERMENULOOP and WM_INITMENU message only if TPM_NONOTIFY flag is not specified */
    if (!(wFlags & TPM_NONOTIFY))
       SendMessageW( hWnd, WM_ENTERMENULOOP, bPopup, 0 );

    SendMessageW( hWnd, WM_SETCURSOR, (WPARAM)hWnd, HTCAPTION );

    if (!(wFlags & TPM_NONOTIFY))
    {
       SendMessageW( hWnd, WM_INITMENU, (WPARAM)hMenu, 0 );
       /* If an app changed/recreated menu bar entries in WM_INITMENU
        * menu sizes will be recalculated once the menu created/shown.
        */
    }

    return TRUE;
}

/***********************************************************************
 *           MENU_ExitTracking
 */
static BOOL MENU_ExitTracking(HWND hWnd, BOOL bPopup)
{
    TRACE("hwnd=%p\n", hWnd);

    SendMessageW( hWnd, WM_EXITMENULOOP, bPopup, 0 );
    ShowCaret(0);
    top_popup = 0;
    top_popup_hmenu = NULL;
    return TRUE;
}

/***********************************************************************
 *           MENU_TrackMouseMenuBar
 *
 * Menu-bar tracking upon a mouse event. Called from NC_HandleSysCommand().
 */
void MENU_TrackMouseMenuBar( HWND hWnd, INT ht, POINT pt )
{
    HMENU hMenu = (ht == HTSYSMENU) ? get_win_sys_menu( hWnd ) : GetMenu( hWnd );
    UINT wFlags = TPM_BUTTONDOWN | TPM_LEFTALIGN | TPM_LEFTBUTTON;

    TRACE("wnd=%p ht=0x%04x %s\n", hWnd, ht, wine_dbgstr_point( &pt));

    if (GetWindowLongW( hWnd, GWL_EXSTYLE ) & WS_EX_LAYOUTRTL) wFlags |= TPM_LAYOUTRTL;
    if (IsMenu(hMenu))
    {
	MENU_InitTracking( hWnd, hMenu, FALSE, wFlags );
	MENU_TrackMenu( hMenu, wFlags, pt.x, pt.y, hWnd, NULL );
	MENU_ExitTracking(hWnd, FALSE);
    }
}


/***********************************************************************
 *           MENU_TrackKbdMenuBar
 *
 * Menu-bar tracking upon a keyboard event. Called from NC_HandleSysCommand().
 */
void MENU_TrackKbdMenuBar( HWND hwnd, UINT wParam, WCHAR wChar)
{
    UINT uItem = NO_SELECTED_ITEM;
    HMENU hTrackMenu;
    UINT wFlags = TPM_LEFTALIGN | TPM_LEFTBUTTON;

    TRACE("hwnd %p wParam 0x%04x wChar 0x%04x\n", hwnd, wParam, wChar);

    /* find window that has a menu */

    while (!WIN_ALLOWED_MENU(GetWindowLongW( hwnd, GWL_STYLE )))
        if (!(hwnd = GetAncestor( hwnd, GA_PARENT ))) return;

    /* check if we have to track a system menu */

    hTrackMenu = GetMenu( hwnd );
    if (!hTrackMenu || IsIconic(hwnd) || wChar == ' ' )
    {
        if (!(GetWindowLongW( hwnd, GWL_STYLE ) & WS_SYSMENU)) return;
        hTrackMenu = get_win_sys_menu( hwnd );
        uItem = 0;
        wParam |= HTSYSMENU; /* prevent item lookup */
    }
    if (GetWindowLongW( hwnd, GWL_EXSTYLE ) & WS_EX_LAYOUTRTL) wFlags |= TPM_LAYOUTRTL;

    if (!IsMenu( hTrackMenu )) return;

    MENU_InitTracking( hwnd, hTrackMenu, FALSE, wFlags );

    if( wChar && wChar != ' ' )
    {
        uItem = MENU_FindItemByKey( hwnd, hTrackMenu, wChar, (wParam & HTSYSMENU) );
        if ( uItem >= (UINT)(-2) )
        {
            if( uItem == (UINT)(-1) ) MessageBeep(0);
            /* schedule end of menu tracking */
            wFlags |= TF_ENDMENU;
            goto track_menu;
        }
    }

    MENU_SelectItem( hwnd, hTrackMenu, uItem, TRUE, 0 );

    if (!(wParam & HTSYSMENU) || wChar == ' ')
    {
        if( uItem == NO_SELECTED_ITEM )
            MENU_MoveSelection( hwnd, hTrackMenu, ITEM_NEXT );
        else
            PostMessageW( hwnd, WM_KEYDOWN, VK_DOWN, 0L );
    }

track_menu:
    MENU_TrackMenu( hTrackMenu, wFlags, 0, 0, hwnd, NULL );
    MENU_ExitTracking( hwnd, FALSE );
}

/**********************************************************************
 *           TrackPopupMenuEx   (USER32.@)
 */
BOOL WINAPI TrackPopupMenuEx( HMENU hMenu, UINT wFlags, INT x, INT y,
                              HWND hWnd, LPTPMPARAMS lpTpm )
{
    POPUPMENU *menu;
    BOOL ret = FALSE;

    TRACE("hmenu %p flags %04x (%d,%d) hwnd %p lpTpm %p rect %s\n",
            hMenu, wFlags, x, y, hWnd, lpTpm,
            lpTpm ? wine_dbgstr_rect( &lpTpm->rcExclude) : "-" );

    /* Parameter check */
    /* FIXME: this check is performed several times, here and in the called
       functions. That could be optimized */
    if (!(menu = MENU_GetMenu( hMenu )))
    {
        SetLastError( ERROR_INVALID_MENU_HANDLE );
        return FALSE;
    }

    if (IsWindow(menu->hWnd))
    {
        SetLastError( ERROR_POPUP_ALREADY_ACTIVE );
        return FALSE;
    }

    MENU_InitTracking(hWnd, hMenu, TRUE, wFlags);

    /* Send WM_INITMENUPOPUP message only if TPM_NONOTIFY flag is not specified */
    if (!(wFlags & TPM_NONOTIFY))
        SendMessageW( hWnd, WM_INITMENUPOPUP, (WPARAM)hMenu, 0);

    if (MENU_ShowPopup( hWnd, hMenu, 0, wFlags, x, y, 0, 0 ))
        ret = MENU_TrackMenu( hMenu, wFlags | TPM_POPUPMENU, 0, 0, hWnd,
                              lpTpm ? &lpTpm->rcExclude : NULL );
    MENU_ExitTracking(hWnd, TRUE);

    return ret;
}

/**********************************************************************
 *           TrackPopupMenu   (USER32.@)
 *
 * Like the win32 API, the function return the command ID only if the
 * flag TPM_RETURNCMD is on.
 *
 */
BOOL WINAPI TrackPopupMenu( HMENU hMenu, UINT wFlags, INT x, INT y,
                            INT nReserved, HWND hWnd, const RECT *lpRect )
{
    return TrackPopupMenuEx( hMenu, wFlags, x, y, hWnd, NULL);
}

/***********************************************************************
 *           PopupMenuWndProc
 *
 * NOTE: Windows has totally different (and undocumented) popup wndproc.
 */
LRESULT WINAPI PopupMenuWndProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    TRACE("hwnd=%p msg=0x%04x wp=0x%04lx lp=0x%08lx\n", hwnd, message, wParam, lParam);

    switch(message)
    {
    case WM_CREATE:
	{
	    CREATESTRUCTW *cs = (CREATESTRUCTW*)lParam;
	    SetWindowLongPtrW( hwnd, 0, (LONG_PTR)cs->lpCreateParams );
            return 0;
	}

    case WM_MOUSEACTIVATE:  /* We don't want to be activated */
        return MA_NOACTIVATE;

    case WM_PAINT:
	{
	    PAINTSTRUCT ps;
	    BeginPaint( hwnd, &ps );
	    MENU_DrawPopupMenu( hwnd, ps.hdc,
                                (HMENU)GetWindowLongPtrW( hwnd, 0 ) );
	    EndPaint( hwnd, &ps );
            return 0;
	}

    case WM_PRINTCLIENT:
	{
	    MENU_DrawPopupMenu( hwnd, (HDC)wParam,
                                (HMENU)GetWindowLongPtrW( hwnd, 0 ) );
            return 0;
        }

    case WM_ERASEBKGND:
        return 1;

    case WM_DESTROY:
        /* zero out global pointer in case resident popup window was destroyed. */
        if (hwnd == top_popup) {
            top_popup = 0;
            top_popup_hmenu = NULL;
        }
        break;

    case WM_SHOWWINDOW:

	if( wParam )
	{
            if (!GetWindowLongPtrW( hwnd, 0 )) ERR("no menu to display\n");
	}
	else
            SetWindowLongPtrW( hwnd, 0, 0 );
	break;

    case MM_SETMENUHANDLE:
        SetWindowLongPtrW( hwnd, 0, wParam );
        break;

    case MM_GETMENUHANDLE:
    case MN_GETHMENU:
        return GetWindowLongPtrW( hwnd, 0 );

    default:
        return DefWindowProcW( hwnd, message, wParam, lParam );
    }
    return 0;
}


/***********************************************************************
 *           MENU_GetMenuBarHeight
 *
 * Compute the size of the menu bar height. Used by NC_HandleNCCalcSize().
 */
UINT MENU_GetMenuBarHeight( HWND hwnd, UINT menubarWidth,
                              INT orgX, INT orgY )
{
    HDC hdc;
    RECT rectBar;
    LPPOPUPMENU lppop;

    TRACE("HWND %p, width %d, at (%d, %d).\n", hwnd, menubarWidth, orgX, orgY );

    if (!(lppop = MENU_GetMenu( GetMenu(hwnd) ))) return 0;

    hdc = GetDCEx( hwnd, 0, DCX_CACHE | DCX_WINDOW );
    SelectObject( hdc, get_menu_font(FALSE));
    SetRect(&rectBar, orgX, orgY, orgX+menubarWidth, orgY+GetSystemMetrics(SM_CYMENU));
    MENU_MenuBarCalcSize( hdc, &rectBar, lppop, hwnd );
    ReleaseDC( hwnd, hdc );
    return lppop->Height;
}


/*******************************************************************
 *         ChangeMenuA    (USER32.@)
 */
BOOL WINAPI ChangeMenuA( HMENU hMenu, UINT pos, LPCSTR data,
                             UINT id, UINT flags )
{
    TRACE("menu=%p pos=%d data=%p id=%08x flags=%08x\n", hMenu, pos, data, id, flags );
    if (flags & MF_APPEND) return AppendMenuA( hMenu, flags & ~MF_APPEND,
                                                 id, data );
    if (flags & MF_DELETE) return DeleteMenu(hMenu, pos, flags & ~MF_DELETE);
    if (flags & MF_CHANGE) return ModifyMenuA(hMenu, pos, flags & ~MF_CHANGE,
                                                id, data );
    if (flags & MF_REMOVE) return RemoveMenu( hMenu,
                                              flags & MF_BYPOSITION ? pos : id,
                                              flags & ~MF_REMOVE );
    /* Default: MF_INSERT */
    return InsertMenuA( hMenu, pos, flags, id, data );
}


/*******************************************************************
 *         ChangeMenuW    (USER32.@)
 */
BOOL WINAPI ChangeMenuW( HMENU hMenu, UINT pos, LPCWSTR data,
                             UINT id, UINT flags )
{
    TRACE("menu=%p pos=%d data=%p id=%08x flags=%08x\n", hMenu, pos, data, id, flags );
    if (flags & MF_APPEND) return AppendMenuW( hMenu, flags & ~MF_APPEND,
                                                 id, data );
    if (flags & MF_DELETE) return DeleteMenu(hMenu, pos, flags & ~MF_DELETE);
    if (flags & MF_CHANGE) return ModifyMenuW(hMenu, pos, flags & ~MF_CHANGE,
                                                id, data );
    if (flags & MF_REMOVE) return RemoveMenu( hMenu,
                                              flags & MF_BYPOSITION ? pos : id,
                                              flags & ~MF_REMOVE );
    /* Default: MF_INSERT */
    return InsertMenuW( hMenu, pos, flags, id, data );
}


/*******************************************************************
 *         CheckMenuItem    (USER32.@)
 */
DWORD WINAPI CheckMenuItem( HMENU hMenu, UINT id, UINT flags )
{
    MENUITEM *item;
    DWORD ret;

    if (!(item = MENU_FindItem( &hMenu, &id, flags ))) return -1;
    ret = item->fState & MF_CHECKED;
    if (flags & MF_CHECKED) item->fState |= MF_CHECKED;
    else item->fState &= ~MF_CHECKED;
    return ret;
}


/**********************************************************************
 *         EnableMenuItem    (USER32.@)
 */
BOOL WINAPI EnableMenuItem( HMENU hMenu, UINT wItemID, UINT wFlags )
{
    UINT    oldflags;
    MENUITEM *item;
    POPUPMENU *menu;

    TRACE("(%p, %04x, %04x) !\n", hMenu, wItemID, wFlags);

    /* Get the Popupmenu to access the owner menu */
    if (!(menu = MENU_GetMenu(hMenu)))
	return (UINT)-1;

    if (!(item = MENU_FindItem( &hMenu, &wItemID, wFlags )))
	return (UINT)-1;

    oldflags = item->fState & (MF_GRAYED | MF_DISABLED);
    item->fState ^= (oldflags ^ wFlags) & (MF_GRAYED | MF_DISABLED);

    /* If the close item in the system menu change update the close button */
    if((item->wID == SC_CLOSE) && (oldflags != wFlags))
    {
	if (menu->hSysMenuOwner != 0)
	{
            RECT rc;
	    POPUPMENU* parentMenu;

	    /* Get the parent menu to access*/
	    if (!(parentMenu = MENU_GetMenu(menu->hSysMenuOwner)))
		return (UINT)-1;

            /* Refresh the frame to reflect the change */
            WIN_GetRectangles( parentMenu->hWnd, COORDS_CLIENT, &rc, NULL );
            rc.bottom = 0;
            RedrawWindow(parentMenu->hWnd, &rc, 0, RDW_FRAME | RDW_INVALIDATE | RDW_NOCHILDREN);
	}
    }

    return oldflags;
}


/*******************************************************************
 *         GetMenuStringA    (USER32.@)
 */
INT WINAPI GetMenuStringA(
	HMENU hMenu,	/* [in] menuhandle */
	UINT wItemID,	/* [in] menu item (dep. on wFlags) */
	LPSTR str,	/* [out] outbuffer. If NULL, func returns entry length*/
	INT nMaxSiz,	/* [in] length of buffer. if 0, func returns entry len*/
	UINT wFlags	/* [in] MF_ flags */
) {
    MENUITEM *item;

    TRACE("menu=%p item=%04x ptr=%p len=%d flags=%04x\n", hMenu, wItemID, str, nMaxSiz, wFlags );
    if (str && nMaxSiz) str[0] = '\0';
    if (!(item = MENU_FindItem( &hMenu, &wItemID, wFlags ))) {
        SetLastError( ERROR_MENU_ITEM_NOT_FOUND);
        return 0;
    }
    if (!item->text) return 0;
    if (!str || !nMaxSiz) return strlenW(item->text);
    if (!WideCharToMultiByte( CP_ACP, 0, item->text, -1, str, nMaxSiz, NULL, NULL ))
        str[nMaxSiz-1] = 0;
    TRACE("returning %s\n", debugstr_a(str));
    return strlen(str);
}


/*******************************************************************
 *         GetMenuStringW    (USER32.@)
 */
INT WINAPI GetMenuStringW( HMENU hMenu, UINT wItemID,
                               LPWSTR str, INT nMaxSiz, UINT wFlags )
{
    MENUITEM *item;

    TRACE("menu=%p item=%04x ptr=%p len=%d flags=%04x\n", hMenu, wItemID, str, nMaxSiz, wFlags );
    if (str && nMaxSiz) str[0] = '\0';
    if (!(item = MENU_FindItem( &hMenu, &wItemID, wFlags ))) {
        SetLastError( ERROR_MENU_ITEM_NOT_FOUND);
        return 0;
    }
    if (!str || !nMaxSiz) return item->text ? strlenW(item->text) : 0;
    if( !(item->text)) {
        str[0] = 0;
        return 0;
    }
    lstrcpynW( str, item->text, nMaxSiz );
    TRACE("returning %s\n", debugstr_w(str));
    return strlenW(str);
}


/**********************************************************************
 *         HiliteMenuItem    (USER32.@)
 */
BOOL WINAPI HiliteMenuItem( HWND hWnd, HMENU hMenu, UINT wItemID,
                                UINT wHilite )
{
    LPPOPUPMENU menu;
    TRACE("(%p, %p, %04x, %04x);\n", hWnd, hMenu, wItemID, wHilite);
    if (!MENU_FindItem( &hMenu, &wItemID, wHilite )) return FALSE;
    if (!(menu = MENU_GetMenu(hMenu))) return FALSE;
    if (menu->FocusedItem == wItemID) return TRUE;
    MENU_HideSubPopups( hWnd, hMenu, FALSE, 0 );
    MENU_SelectItem( hWnd, hMenu, wItemID, TRUE, 0 );
    return TRUE;
}


/**********************************************************************
 *         GetMenuState    (USER32.@)
 */
UINT WINAPI GetMenuState( HMENU hMenu, UINT wItemID, UINT wFlags )
{
    MENUITEM *item;
    TRACE("(menu=%p, id=%04x, flags=%04x);\n", hMenu, wItemID, wFlags);
    if (!(item = MENU_FindItem( &hMenu, &wItemID, wFlags ))) return -1;
    debug_print_menuitem ("  item: ", item, "");
    if (item->fType & MF_POPUP)
    {
	POPUPMENU *menu = MENU_GetMenu( item->hSubMenu );
	if (!menu) return -1;
	else return (menu->nItems << 8) | ((item->fState|item->fType) & 0xff);
    }
    else
    {
	/* We used to (from way back then) mask the result to 0xff.  */
	/* I don't know why and it seems wrong as the documented */
	/* return flag MF_SEPARATOR is outside that mask.  */
	return (item->fType | item->fState);
    }
}


/**********************************************************************
 *         GetMenuItemCount    (USER32.@)
 */
INT WINAPI GetMenuItemCount( HMENU hMenu )
{
    LPPOPUPMENU	menu = MENU_GetMenu(hMenu);
    if (!menu) return -1;
    TRACE("(%p) returning %d\n", hMenu, menu->nItems );
    return menu->nItems;
}


/**********************************************************************
 *         GetMenuItemID    (USER32.@)
 */
UINT WINAPI GetMenuItemID( HMENU hMenu, INT nPos )
{
    MENUITEM * lpmi;

    if (!(lpmi = MENU_FindItem(&hMenu,(UINT*)&nPos,MF_BYPOSITION))) return -1;
    if (lpmi->fType & MF_POPUP) return -1;
    return lpmi->wID;

}


/**********************************************************************
 *         MENU_mnu2mnuii
 *
 * Uses flags, id and text ptr, passed by InsertMenu() and
 * ModifyMenu() to setup a MenuItemInfo structure.
 */
static void MENU_mnu2mnuii( UINT flags, UINT_PTR id, LPCWSTR str,
        LPMENUITEMINFOW pmii)
{
    ZeroMemory( pmii, sizeof( MENUITEMINFOW));
    pmii->cbSize = sizeof( MENUITEMINFOW);
    pmii->fMask = MIIM_STATE | MIIM_ID | MIIM_FTYPE;
    /* setting bitmap clears text and vice versa */
    if( IS_STRING_ITEM(flags)) {
        pmii->fMask |= MIIM_STRING | MIIM_BITMAP;
        if( !str)
            flags |= MF_SEPARATOR;
        /* Item beginning with a backspace is a help item */
        /* FIXME: wrong place, this is only true in win16 */
        else if( *str == '\b') {
            flags |= MF_HELP;
            str++;
        }
        pmii->dwTypeData = (LPWSTR)str;
    } else if( flags & MFT_BITMAP){
        pmii->fMask |= MIIM_BITMAP | MIIM_STRING;
        pmii->hbmpItem = ULongToHandle(LOWORD(str));
    }
    if( flags & MF_OWNERDRAW){
        pmii->fMask |= MIIM_DATA;
        pmii->dwItemData = (ULONG_PTR) str;
    }
    if( flags & MF_POPUP) {
        pmii->fMask |= MIIM_SUBMENU;
        pmii->hSubMenu = (HMENU)id;
    }
    if( flags & MF_SEPARATOR) flags |= MF_GRAYED | MF_DISABLED;
    pmii->fState = flags & MENUITEMINFO_STATE_MASK & ~MFS_DEFAULT;
    pmii->fType = flags & MENUITEMINFO_TYPE_MASK;
    pmii->wID = (UINT)id;
}


/*******************************************************************
 *         InsertMenuW    (USER32.@)
 */
BOOL WINAPI InsertMenuW( HMENU hMenu, UINT pos, UINT flags,
                         UINT_PTR id, LPCWSTR str )
{
    MENUITEM *item;
    MENUITEMINFOW mii;

    if (IS_STRING_ITEM(flags) && str)
        TRACE("hMenu %p, pos %d, flags %08x, id %04lx, str %s\n",
              hMenu, pos, flags, id, debugstr_w(str) );
    else TRACE("hMenu %p, pos %d, flags %08x, id %04lx, str %p (not a string)\n",
               hMenu, pos, flags, id, str );

    if (!(item = MENU_InsertItem( hMenu, pos, flags ))) return FALSE;
    MENU_mnu2mnuii( flags, id, str, &mii);
    if (!(SetMenuItemInfo_common( item, &mii, TRUE)))
    {
        RemoveMenu( hMenu, pos, flags );
        return FALSE;
    }

    item->hCheckBit = item->hUnCheckBit = 0;
    return TRUE;
}


/*******************************************************************
 *         InsertMenuA    (USER32.@)
 */
BOOL WINAPI InsertMenuA( HMENU hMenu, UINT pos, UINT flags,
                         UINT_PTR id, LPCSTR str )
{
    BOOL ret = FALSE;

    if (IS_STRING_ITEM(flags) && str)
    {
        INT len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
        LPWSTR newstr = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        if (newstr)
        {
            MultiByteToWideChar( CP_ACP, 0, str, -1, newstr, len );
            ret = InsertMenuW( hMenu, pos, flags, id, newstr );
            HeapFree( GetProcessHeap(), 0, newstr );
        }
        return ret;
    }
    else return InsertMenuW( hMenu, pos, flags, id, (LPCWSTR)str );
}


/*******************************************************************
 *         AppendMenuA    (USER32.@)
 */
BOOL WINAPI AppendMenuA( HMENU hMenu, UINT flags,
                         UINT_PTR id, LPCSTR data )
{
    return InsertMenuA( hMenu, -1, flags | MF_BYPOSITION, id, data );
}


/*******************************************************************
 *         AppendMenuW    (USER32.@)
 */
BOOL WINAPI AppendMenuW( HMENU hMenu, UINT flags,
                         UINT_PTR id, LPCWSTR data )
{
    return InsertMenuW( hMenu, -1, flags | MF_BYPOSITION, id, data );
}


/**********************************************************************
 *         RemoveMenu    (USER32.@)
 */
BOOL WINAPI RemoveMenu( HMENU hMenu, UINT nPos, UINT wFlags )
{
    LPPOPUPMENU	menu;
    MENUITEM *item;

    TRACE("(menu=%p pos=%04x flags=%04x)\n",hMenu, nPos, wFlags);
    if (!(item = MENU_FindItem( &hMenu, &nPos, wFlags ))) return FALSE;
    if (!(menu = MENU_GetMenu(hMenu))) return FALSE;

      /* Remove item */

    MENU_FreeItemData( item );

    if (--menu->nItems == 0)
    {
        HeapFree( GetProcessHeap(), 0, menu->items );
        menu->items = NULL;
    }
    else
    {
	while(nPos < menu->nItems)
	{
	    *item = *(item+1);
	    item++;
	    nPos++;
	}
        menu->items = HeapReAlloc( GetProcessHeap(), 0, menu->items,
                                   menu->nItems * sizeof(MENUITEM) );
    }
    return TRUE;
}


/**********************************************************************
 *         DeleteMenu    (USER32.@)
 */
BOOL WINAPI DeleteMenu( HMENU hMenu, UINT nPos, UINT wFlags )
{
    MENUITEM *item = MENU_FindItem( &hMenu, &nPos, wFlags );
    if (!item) return FALSE;
    if (item->fType & MF_POPUP) DestroyMenu( item->hSubMenu );
      /* nPos is now the position of the item */
    RemoveMenu( hMenu, nPos, wFlags | MF_BYPOSITION );
    return TRUE;
}


/*******************************************************************
 *         ModifyMenuW    (USER32.@)
 */
BOOL WINAPI ModifyMenuW( HMENU hMenu, UINT pos, UINT flags,
                         UINT_PTR id, LPCWSTR str )
{
    MENUITEM *item;
    MENUITEMINFOW mii;

    if (IS_STRING_ITEM(flags))
        TRACE("%p %d %04x %04lx %s\n", hMenu, pos, flags, id, debugstr_w(str) );
    else
        TRACE("%p %d %04x %04lx %p\n", hMenu, pos, flags, id, str );

    if (!(item = MENU_FindItem( &hMenu, &pos, flags ))) return FALSE;
    MENU_GetMenu(hMenu)->Height = 0; /* force size recalculate */
    MENU_mnu2mnuii( flags, id, str, &mii);
    return SetMenuItemInfo_common( item, &mii, TRUE);
}


/*******************************************************************
 *         ModifyMenuA    (USER32.@)
 */
BOOL WINAPI ModifyMenuA( HMENU hMenu, UINT pos, UINT flags,
                         UINT_PTR id, LPCSTR str )
{
    BOOL ret = FALSE;

    if (IS_STRING_ITEM(flags) && str)
    {
        INT len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
        LPWSTR newstr = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        if (newstr)
        {
            MultiByteToWideChar( CP_ACP, 0, str, -1, newstr, len );
            ret = ModifyMenuW( hMenu, pos, flags, id, newstr );
            HeapFree( GetProcessHeap(), 0, newstr );
        }
        return ret;
    }
    else return ModifyMenuW( hMenu, pos, flags, id, (LPCWSTR)str );
}


/**********************************************************************
 *         CreatePopupMenu    (USER32.@)
 */
HMENU WINAPI CreatePopupMenu(void)
{
    HMENU hmenu;
    POPUPMENU *menu;

    if (!(hmenu = CreateMenu())) return 0;
    menu = MENU_GetMenu( hmenu );
    menu->wFlags |= MF_POPUP;
    menu->bTimeToHide = FALSE;
    return hmenu;
}


/**********************************************************************
 *         GetMenuCheckMarkDimensions    (USER.417)
 *         GetMenuCheckMarkDimensions    (USER32.@)
 */
LONG WINAPI GetMenuCheckMarkDimensions(void)
{
    return MAKELONG( GetSystemMetrics(SM_CXMENUCHECK), GetSystemMetrics(SM_CYMENUCHECK) );
}


/**********************************************************************
 *         SetMenuItemBitmaps    (USER32.@)
 */
BOOL WINAPI SetMenuItemBitmaps( HMENU hMenu, UINT nPos, UINT wFlags,
                                    HBITMAP hNewUnCheck, HBITMAP hNewCheck)
{
    MENUITEM *item;

    if (!(item = MENU_FindItem( &hMenu, &nPos, wFlags ))) return FALSE;

    if (!hNewCheck && !hNewUnCheck)
    {
	item->fState &= ~MF_USECHECKBITMAPS;
    }
    else  /* Install new bitmaps */
    {
	item->hCheckBit = hNewCheck;
	item->hUnCheckBit = hNewUnCheck;
	item->fState |= MF_USECHECKBITMAPS;
    }
    return TRUE;
}


/**********************************************************************
 *         CreateMenu    (USER32.@)
 */
HMENU WINAPI CreateMenu(void)
{
    HMENU hMenu;
    LPPOPUPMENU menu;

    if (!(menu = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*menu) ))) return 0;
    menu->FocusedItem = NO_SELECTED_ITEM;
    menu->bTimeToHide = FALSE;

    if (!(hMenu = alloc_user_handle( &menu->obj, USER_MENU ))) HeapFree( GetProcessHeap(), 0, menu );

    TRACE("return %p\n", hMenu );

    return hMenu;
}


/**********************************************************************
 *         DestroyMenu    (USER32.@)
 */
BOOL WINAPI DestroyMenu( HMENU hMenu )
{
    LPPOPUPMENU lppop;

    TRACE("(%p)\n", hMenu);

    if (!(lppop = free_user_handle( hMenu, USER_MENU ))) return FALSE;
    if (lppop == OBJ_OTHER_PROCESS) return FALSE;

    /* DestroyMenu should not destroy system menu popup owner */
    if ((lppop->wFlags & (MF_POPUP | MF_SYSMENU)) == MF_POPUP && lppop->hWnd)
    {
        DestroyWindow( lppop->hWnd );
        lppop->hWnd = 0;
    }

    if (lppop->items) /* recursively destroy submenus */
    {
        int i;
        MENUITEM *item = lppop->items;
        for (i = lppop->nItems; i > 0; i--, item++)
        {
            if (item->fType & MF_POPUP) DestroyMenu(item->hSubMenu);
            MENU_FreeItemData( item );
        }
        HeapFree( GetProcessHeap(), 0, lppop->items );
    }
    HeapFree( GetProcessHeap(), 0, lppop );
    return TRUE;
}


/**********************************************************************
 *         GetSystemMenu    (USER32.@)
 */
HMENU WINAPI GetSystemMenu( HWND hWnd, BOOL bRevert )
{
    WND *wndPtr = WIN_GetPtr( hWnd );
    HMENU retvalue = 0;

    if (wndPtr == WND_DESKTOP) return 0;
    if (wndPtr == WND_OTHER_PROCESS)
    {
        if (IsWindow( hWnd )) FIXME( "not supported on other process window %p\n", hWnd );
    }
    else if (wndPtr)
    {
	if (wndPtr->hSysMenu && bRevert)
	{
            DestroyMenu(wndPtr->hSysMenu);
            wndPtr->hSysMenu = 0;
	}

	if(!wndPtr->hSysMenu && (wndPtr->dwStyle & WS_SYSMENU) )
	    wndPtr->hSysMenu = MENU_GetSysMenu( hWnd, 0 );

	if( wndPtr->hSysMenu )
        {
	    POPUPMENU *menu;
	    retvalue = GetSubMenu(wndPtr->hSysMenu, 0);

	    /* Store the dummy sysmenu handle to facilitate the refresh */
	    /* of the close button if the SC_CLOSE item change */
	    menu = MENU_GetMenu(retvalue);
	    if ( menu )
	       menu->hSysMenuOwner = wndPtr->hSysMenu;
        }
        WIN_ReleasePtr( wndPtr );
    }
    return bRevert ? 0 : retvalue;
}


/*******************************************************************
 *         SetSystemMenu    (USER32.@)
 */
BOOL WINAPI SetSystemMenu( HWND hwnd, HMENU hMenu )
{
    WND *wndPtr = WIN_GetPtr( hwnd );

    if (wndPtr && wndPtr != WND_OTHER_PROCESS && wndPtr != WND_DESKTOP)
    {
	if (wndPtr->hSysMenu) DestroyMenu( wndPtr->hSysMenu );
	wndPtr->hSysMenu = MENU_GetSysMenu( hwnd, hMenu );
        WIN_ReleasePtr( wndPtr );
	return TRUE;
    }
    return FALSE;
}


/**********************************************************************
 *         GetMenu    (USER32.@)
 */
HMENU WINAPI GetMenu( HWND hWnd )
{
    HMENU retvalue = (HMENU)GetWindowLongPtrW( hWnd, GWLP_ID );
    TRACE("for %p returning %p\n", hWnd, retvalue);
    return retvalue;
}

/**********************************************************************
 *         GetMenuBarInfo    (USER32.@)
 */
BOOL WINAPI GetMenuBarInfo( HWND hwnd, LONG idObject, LONG idItem, PMENUBARINFO pmbi )
{
    FIXME( "(%p,0x%08x,0x%08x,%p)\n", hwnd, idObject, idItem, pmbi );
    return FALSE;
}

/**********************************************************************
 *         MENU_SetMenu
 *
 * Helper for SetMenu. Also called by WIN_CreateWindowEx to avoid the
 * SetWindowPos call that would result if SetMenu were called directly.
 */
BOOL MENU_SetMenu( HWND hWnd, HMENU hMenu )
{
    TRACE("(%p, %p);\n", hWnd, hMenu);

    if (hMenu && !IsMenu(hMenu))
    {
        WARN("hMenu %p is not a menu handle\n", hMenu);
        return FALSE;
    }
    if (!WIN_ALLOWED_MENU(GetWindowLongW( hWnd, GWL_STYLE )))
        return FALSE;

    hWnd = WIN_GetFullHandle( hWnd );
    if (GetCapture() == hWnd)
        set_capture_window( 0, GUI_INMENUMODE, NULL );  /* release the capture */

    if (hMenu != 0)
    {
        LPPOPUPMENU lpmenu;

        if (!(lpmenu = MENU_GetMenu(hMenu))) return FALSE;

        lpmenu->hWnd = hWnd;
        lpmenu->Height = 0;  /* Make sure we recalculate the size */
    }
    SetWindowLongPtrW( hWnd, GWLP_ID, (LONG_PTR)hMenu );
    return TRUE;
}


/**********************************************************************
 *         SetMenu    (USER32.@)
 */
BOOL WINAPI SetMenu( HWND hWnd, HMENU hMenu )
{   
    if(!MENU_SetMenu(hWnd, hMenu))
        return FALSE;
 
    SetWindowPos( hWnd, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE |
                  SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED );
    return TRUE;
}


/**********************************************************************
 *         GetSubMenu    (USER32.@)
 */
HMENU WINAPI GetSubMenu( HMENU hMenu, INT nPos )
{
    MENUITEM * lpmi;

    if (!(lpmi = MENU_FindItem(&hMenu,(UINT*)&nPos,MF_BYPOSITION))) return 0;
    if (!(lpmi->fType & MF_POPUP)) return 0;
    return lpmi->hSubMenu;
}


/**********************************************************************
 *         DrawMenuBar    (USER32.@)
 */
BOOL WINAPI DrawMenuBar( HWND hWnd )
{
    LPPOPUPMENU lppop;
    HMENU hMenu = GetMenu(hWnd);

    if (!WIN_ALLOWED_MENU(GetWindowLongW( hWnd, GWL_STYLE )))
        return FALSE;
    if (!hMenu || !(lppop = MENU_GetMenu( hMenu ))) return FALSE;

    lppop->Height = 0; /* Make sure we call MENU_MenuBarCalcSize */
    lppop->hwndOwner = hWnd;
    SetWindowPos( hWnd, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE |
                  SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED );
    return TRUE;
}

/***********************************************************************
 *           DrawMenuBarTemp   (USER32.@)
 *
 * UNDOCUMENTED !!
 *
 * called by W98SE desk.cpl Control Panel Applet
 *
 * Not 100% sure about the param names, but close.
 */
DWORD WINAPI DrawMenuBarTemp(HWND hwnd, HDC hDC, LPRECT lprect, HMENU hMenu, HFONT hFont)
{
    LPPOPUPMENU lppop;
    UINT i,retvalue;
    HFONT hfontOld = 0;
    BOOL flat_menu = FALSE;

    SystemParametersInfoW (SPI_GETFLATMENU, 0, &flat_menu, 0);

    if (!hMenu)
        hMenu = GetMenu(hwnd);

    if (!hFont)
        hFont = get_menu_font(FALSE);

    lppop = MENU_GetMenu( hMenu );
    if (lppop == NULL || lprect == NULL)
    {
        retvalue = GetSystemMetrics(SM_CYMENU);
        goto END;
    }

    TRACE("(%p, %p, %p, %p, %p)\n", hwnd, hDC, lprect, hMenu, hFont);

    hfontOld = SelectObject( hDC, hFont);

    if (lppop->Height == 0)
        MENU_MenuBarCalcSize(hDC, lprect, lppop, hwnd);

    lprect->bottom = lprect->top + lppop->Height;

    FillRect(hDC, lprect, GetSysColorBrush(flat_menu ? COLOR_MENUBAR : COLOR_MENU) );

    SelectObject( hDC, SYSCOLOR_GetPen(COLOR_3DFACE));
    MoveToEx( hDC, lprect->left, lprect->bottom, NULL );
    LineTo( hDC, lprect->right, lprect->bottom );

    if (lppop->nItems == 0)
    {
        retvalue = GetSystemMetrics(SM_CYMENU);
        goto END;
    }

    for (i = 0; i < lppop->nItems; i++)
    {
        MENU_DrawMenuItem( hwnd, hMenu, hwnd,
                           hDC, &lppop->items[i], lppop->Height, TRUE, ODA_DRAWENTIRE );
    }
    retvalue = lppop->Height;

END:
    if (hfontOld) SelectObject (hDC, hfontOld);
    return retvalue;
}

/***********************************************************************
 *           EndMenu   (USER.187)
 *           EndMenu   (USER32.@)
 */
BOOL WINAPI EndMenu(void)
{
    /* if we are in the menu code, and it is active */
    if (!fEndMenu && top_popup)
    {
	/* terminate the menu handling code */
        fEndMenu = TRUE;

	/* needs to be posted to wakeup the internal menu handler */
	/* which will now terminate the menu, in the event that */
	/* the main window was minimized, or lost focus, so we */
	/* don't end up with an orphaned menu */
        PostMessageW( top_popup, WM_CANCELMODE, 0, 0);
    }
    return fEndMenu;
}


/*****************************************************************
 *        LoadMenuA   (USER32.@)
 */
HMENU WINAPI LoadMenuA( HINSTANCE instance, LPCSTR name )
{
    HRSRC hrsrc = FindResourceA( instance, name, (LPSTR)RT_MENU );
    if (!hrsrc) return 0;
    return LoadMenuIndirectA( LoadResource( instance, hrsrc ));
}


/*****************************************************************
 *        LoadMenuW   (USER32.@)
 */
HMENU WINAPI LoadMenuW( HINSTANCE instance, LPCWSTR name )
{
    HRSRC hrsrc = FindResourceW( instance, name, (LPWSTR)RT_MENU );
    if (!hrsrc) return 0;
    return LoadMenuIndirectW( LoadResource( instance, hrsrc ));
}


/**********************************************************************
 *	    LoadMenuIndirectW    (USER32.@)
 */
HMENU WINAPI LoadMenuIndirectW( LPCVOID template )
{
    HMENU hMenu;
    WORD version, offset;
    LPCSTR p = template;

    version = GET_WORD(p);
    p += sizeof(WORD);
    TRACE("%p, ver %d\n", template, version );
    switch (version)
    {
      case 0: /* standard format is version of 0 */
	offset = GET_WORD(p);
	p += sizeof(WORD) + offset;
	if (!(hMenu = CreateMenu())) return 0;
	if (!MENU_ParseResource( p, hMenu ))
	  {
	    DestroyMenu( hMenu );
	    return 0;
	  }
	return hMenu;
      case 1: /* extended format is version of 1 */
	offset = GET_WORD(p);
	p += sizeof(WORD) + offset;
	if (!(hMenu = CreateMenu())) return 0;
	if (!MENUEX_ParseResource( p, hMenu))
	  {
	    DestroyMenu( hMenu );
	    return 0;
	  }
	return hMenu;
      default:
        ERR("version %d not supported.\n", version);
        return 0;
    }
}


/**********************************************************************
 *	    LoadMenuIndirectA    (USER32.@)
 */
HMENU WINAPI LoadMenuIndirectA( LPCVOID template )
{
    return LoadMenuIndirectW( template );
}


/**********************************************************************
 *		IsMenu    (USER32.@)
 */
BOOL WINAPI IsMenu(HMENU hmenu)
{
    LPPOPUPMENU menu = MENU_GetMenu(hmenu);

    if (!menu)
    {
        SetLastError(ERROR_INVALID_MENU_HANDLE);
        return FALSE;
    }
    return TRUE;
}

/**********************************************************************
 *		GetMenuItemInfo_common
 */

static BOOL GetMenuItemInfo_common ( HMENU hmenu, UINT item, BOOL bypos,
					LPMENUITEMINFOW lpmii, BOOL unicode)
{
    MENUITEM *menu = MENU_FindItem (&hmenu, &item, bypos ? MF_BYPOSITION : 0);

    debug_print_menuitem("GetMenuItemInfo_common: ", menu, "");

    if (!menu) {
        SetLastError( ERROR_MENU_ITEM_NOT_FOUND);
        return FALSE;
    }
    
    if( lpmii->fMask & MIIM_TYPE) {
        if( lpmii->fMask & ( MIIM_STRING | MIIM_FTYPE | MIIM_BITMAP)) {
            WARN("invalid combination of fMask bits used\n");
            /* this does not happen on Win9x/ME */
            SetLastError( ERROR_INVALID_PARAMETER);
            return FALSE;
        }
	lpmii->fType = menu->fType & MENUITEMINFO_TYPE_MASK;
        if( menu->hbmpItem) lpmii->fType |= MFT_BITMAP;
	lpmii->hbmpItem = menu->hbmpItem; /* not on Win9x/ME */
        if( lpmii->fType & MFT_BITMAP) {
	    lpmii->dwTypeData = (LPWSTR) menu->hbmpItem;
	    lpmii->cch = 0;
        } else if( lpmii->fType & (MFT_OWNERDRAW | MFT_SEPARATOR)) {
            /* this does not happen on Win9x/ME */
	    lpmii->dwTypeData = 0;
	    lpmii->cch = 0;
        }
    }

    /* copy the text string */
    if ((lpmii->fMask & (MIIM_TYPE|MIIM_STRING))) {
         if( !menu->text ) {
                if(lpmii->dwTypeData && lpmii->cch) {
                    lpmii->cch = 0;
                    if( unicode)
                        *((WCHAR *)lpmii->dwTypeData) = 0;
                    else
                        *((CHAR *)lpmii->dwTypeData) = 0;
                }
         } else {
            int len;
            if (unicode)
            {
                len = strlenW(menu->text);
                if(lpmii->dwTypeData && lpmii->cch)
                    lstrcpynW(lpmii->dwTypeData, menu->text, lpmii->cch);
            }
            else
            {
                len = WideCharToMultiByte( CP_ACP, 0, menu->text, -1, NULL,
                        0, NULL, NULL ) - 1;
                if(lpmii->dwTypeData && lpmii->cch)
                    if (!WideCharToMultiByte( CP_ACP, 0, menu->text, -1,
                            (LPSTR)lpmii->dwTypeData, lpmii->cch, NULL, NULL ))
                        ((LPSTR)lpmii->dwTypeData)[lpmii->cch - 1] = 0;
            }
            /* if we've copied a substring we return its length */
            if(lpmii->dwTypeData && lpmii->cch)
                if (lpmii->cch <= len + 1)
                    lpmii->cch--;
                else
                    lpmii->cch = len;
            else {
                /* return length of string */
                /* not on Win9x/ME if fType & MFT_BITMAP */
                lpmii->cch = len;
            }
        }
    }

    if (lpmii->fMask & MIIM_FTYPE)
	lpmii->fType = menu->fType & MENUITEMINFO_TYPE_MASK;

    if (lpmii->fMask & MIIM_BITMAP)
	lpmii->hbmpItem = menu->hbmpItem;

    if (lpmii->fMask & MIIM_STATE)
	lpmii->fState = menu->fState & MENUITEMINFO_STATE_MASK;

    if (lpmii->fMask & MIIM_ID)
	lpmii->wID = menu->wID;

    if (lpmii->fMask & MIIM_SUBMENU)
	lpmii->hSubMenu = menu->hSubMenu;
    else {
        /* hSubMenu is always cleared 
         * (not on Win9x/ME ) */
        lpmii->hSubMenu = 0;
    }

    if (lpmii->fMask & MIIM_CHECKMARKS) {
	lpmii->hbmpChecked = menu->hCheckBit;
	lpmii->hbmpUnchecked = menu->hUnCheckBit;
    }
    if (lpmii->fMask & MIIM_DATA)
	lpmii->dwItemData = menu->dwItemData;

  return TRUE;
}

/**********************************************************************
 *		GetMenuItemInfoA    (USER32.@)
 */
BOOL WINAPI GetMenuItemInfoA( HMENU hmenu, UINT item, BOOL bypos,
                                  LPMENUITEMINFOA lpmii)
{
    BOOL ret;
    MENUITEMINFOA mii;
    if( lpmii->cbSize != sizeof( mii) &&
            lpmii->cbSize != sizeof( mii) - sizeof ( mii.hbmpItem)) {
        SetLastError( ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    memcpy( &mii, lpmii, lpmii->cbSize);
    mii.cbSize = sizeof( mii);
    ret = GetMenuItemInfo_common (hmenu, item, bypos,
                                    (LPMENUITEMINFOW)&mii, FALSE);
    mii.cbSize = lpmii->cbSize;
    memcpy( lpmii, &mii, mii.cbSize);
    return ret;
}

/**********************************************************************
 *		GetMenuItemInfoW    (USER32.@)
 */
BOOL WINAPI GetMenuItemInfoW( HMENU hmenu, UINT item, BOOL bypos,
                                  LPMENUITEMINFOW lpmii)
{
    BOOL ret;
    MENUITEMINFOW mii;
    if( lpmii->cbSize != sizeof( mii) &&
            lpmii->cbSize != sizeof( mii) - sizeof ( mii.hbmpItem)) {
        SetLastError( ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    memcpy( &mii, lpmii, lpmii->cbSize);
    mii.cbSize = sizeof( mii);
    ret = GetMenuItemInfo_common (hmenu, item, bypos, &mii, TRUE);
    mii.cbSize = lpmii->cbSize;
    memcpy( lpmii, &mii, mii.cbSize);
    return ret;
}


/* set a menu item text from a ASCII or Unicode string */
static inline void set_menu_item_text( MENUITEM *menu, LPCWSTR text, BOOL unicode )
{
    if (!text)
        menu->text = NULL;
    else if (unicode)
    {
        if ((menu->text = HeapAlloc( GetProcessHeap(), 0, (strlenW(text)+1) * sizeof(WCHAR) )))
            strcpyW( menu->text, text );
    }
    else
    {
        LPCSTR str = (LPCSTR)text;
        int len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
        if ((menu->text = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
            MultiByteToWideChar( CP_ACP, 0, str, -1, menu->text, len );
    }
}


/**********************************************************************
 *		MENU_depth
 *
 * detect if there are loops in the menu tree (or the depth is too large)
 */
static int MENU_depth( POPUPMENU *pmenu, int depth)
{
    int i;
    MENUITEM *item;
    int subdepth;

    depth++;
    if( depth > MAXMENUDEPTH) return depth;
    item = pmenu->items;
    subdepth = depth;
    for( i = 0; i < pmenu->nItems && subdepth <= MAXMENUDEPTH; i++, item++){
        POPUPMENU *psubmenu =  item->hSubMenu ? MENU_GetMenu( item->hSubMenu) : NULL;
        if( psubmenu){
            int bdepth = MENU_depth( psubmenu, depth);
            if( bdepth > subdepth) subdepth = bdepth;
        }
        if( subdepth > MAXMENUDEPTH)
            TRACE("<- hmenu %p\n", item->hSubMenu);
    }
    return subdepth;
}


/**********************************************************************
 *		SetMenuItemInfo_common
 *
 * Note: does not support the MIIM_TYPE flag. Use the MIIM_FTYPE,
 * MIIM_BITMAP and MIIM_STRING flags instead.
 */

static BOOL SetMenuItemInfo_common(MENUITEM * menu,
				       const MENUITEMINFOW *lpmii,
				       BOOL unicode)
{
    if (!menu) return FALSE;

    debug_print_menuitem("SetMenuItemInfo_common from: ", menu, "");

    if (lpmii->fMask & MIIM_FTYPE ) {
        menu->fType &= ~MENUITEMINFO_TYPE_MASK;
        menu->fType |= lpmii->fType & MENUITEMINFO_TYPE_MASK;
    }
    if (lpmii->fMask & MIIM_STRING ) {
        /* free the string when used */
        HeapFree(GetProcessHeap(), 0, menu->text);
        set_menu_item_text( menu, lpmii->dwTypeData, unicode );
    }

    if (lpmii->fMask & MIIM_STATE)
         /* Other menu items having MFS_DEFAULT are not converted
           to normal items */
         menu->fState = lpmii->fState & MENUITEMINFO_STATE_MASK;

    if (lpmii->fMask & MIIM_ID)
	menu->wID = lpmii->wID;

    if (lpmii->fMask & MIIM_SUBMENU) {
	menu->hSubMenu = lpmii->hSubMenu;
	if (menu->hSubMenu) {
	    POPUPMENU *subMenu = MENU_GetMenu(menu->hSubMenu);
	    if (subMenu) {
                if( MENU_depth( subMenu, 0) > MAXMENUDEPTH) {
                    ERR( "Loop detected in menu hierarchy or maximum menu depth exceeded!\n");
                    menu->hSubMenu = 0;
                    return FALSE;
                }
		subMenu->wFlags |= MF_POPUP;
		menu->fType |= MF_POPUP;
	    } else {
                SetLastError( ERROR_INVALID_PARAMETER);
                return FALSE;
            }
	}
	else
	    menu->fType &= ~MF_POPUP;
    }

    if (lpmii->fMask & MIIM_CHECKMARKS)
    {
	menu->hCheckBit = lpmii->hbmpChecked;
	menu->hUnCheckBit = lpmii->hbmpUnchecked;
    }
    if (lpmii->fMask & MIIM_DATA)
	menu->dwItemData = lpmii->dwItemData;

    if (lpmii->fMask & MIIM_BITMAP)
	menu->hbmpItem = lpmii->hbmpItem;

    if( !menu->text && !(menu->fType & MFT_OWNERDRAW) && !menu->hbmpItem)
        menu->fType |= MFT_SEPARATOR;

    debug_print_menuitem("SetMenuItemInfo_common to : ", menu, "");
    return TRUE;
}

/**********************************************************************
 *		MENU_NormalizeMenuItemInfoStruct
 *
 * Helper for SetMenuItemInfo and InsertMenuItemInfo:
 * check, copy and extend the MENUITEMINFO struct from the version that the application
 * supplied to the version used by wine source. */
static BOOL MENU_NormalizeMenuItemInfoStruct( const MENUITEMINFOW *pmii_in,
                                              MENUITEMINFOW *pmii_out )
{
    /* do we recognize the size? */
    if( pmii_in->cbSize != sizeof( MENUITEMINFOW) &&
            pmii_in->cbSize != sizeof( MENUITEMINFOW) - sizeof( pmii_in->hbmpItem)) {
        SetLastError( ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    /* copy the fields that we have */
    memcpy( pmii_out, pmii_in, pmii_in->cbSize);
    /* if the hbmpItem member is missing then extend */
    if( pmii_in->cbSize != sizeof( MENUITEMINFOW)) {
        pmii_out->cbSize = sizeof( MENUITEMINFOW);
        pmii_out->hbmpItem = NULL;
    }
    /* test for invalid bit combinations */
    if( (pmii_out->fMask & MIIM_TYPE &&
         pmii_out->fMask & (MIIM_STRING | MIIM_FTYPE | MIIM_BITMAP)) ||
        (pmii_out->fMask & MIIM_FTYPE && pmii_out->fType & MFT_BITMAP)) {
        WARN("invalid combination of fMask bits used\n");
        /* this does not happen on Win9x/ME */
        SetLastError( ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    /* convert old style (MIIM_TYPE) to the new */
    if( pmii_out->fMask & MIIM_TYPE){
        pmii_out->fMask |= MIIM_FTYPE;
        if( IS_STRING_ITEM(pmii_out->fType)){
            pmii_out->fMask |= MIIM_STRING;
        } else if( (pmii_out->fType) & MFT_BITMAP){
            pmii_out->fMask |= MIIM_BITMAP;
            pmii_out->hbmpItem = UlongToHandle(LOWORD(pmii_out->dwTypeData));
        }
    }
    return TRUE;
}

/**********************************************************************
 *		SetMenuItemInfoA    (USER32.@)
 */
BOOL WINAPI SetMenuItemInfoA(HMENU hmenu, UINT item, BOOL bypos,
                                 const MENUITEMINFOA *lpmii)
{
    MENUITEMINFOW mii;

    TRACE("hmenu %p, item %u, by pos %d, info %p\n", hmenu, item, bypos, lpmii);

    if (!MENU_NormalizeMenuItemInfoStruct( (const MENUITEMINFOW *)lpmii, &mii )) return FALSE;

    return SetMenuItemInfo_common(MENU_FindItem(&hmenu, &item, bypos? MF_BYPOSITION : 0),
                                  &mii, FALSE);
}

/**********************************************************************
 *		SetMenuItemInfoW    (USER32.@)
 */
BOOL WINAPI SetMenuItemInfoW(HMENU hmenu, UINT item, BOOL bypos,
                                 const MENUITEMINFOW *lpmii)
{
    MENUITEMINFOW mii;

    TRACE("hmenu %p, item %u, by pos %d, info %p\n", hmenu, item, bypos, lpmii);

    if (!MENU_NormalizeMenuItemInfoStruct( lpmii, &mii )) return FALSE;
    return SetMenuItemInfo_common(MENU_FindItem(&hmenu,
                &item, bypos? MF_BYPOSITION : 0), &mii, TRUE);
}

/**********************************************************************
 *		SetMenuDefaultItem    (USER32.@)
 *
 */
BOOL WINAPI SetMenuDefaultItem(HMENU hmenu, UINT uItem, UINT bypos)
{
	UINT i;
	POPUPMENU *menu;
	MENUITEM *item;

	TRACE("(%p,%d,%d)\n", hmenu, uItem, bypos);

	if (!(menu = MENU_GetMenu(hmenu))) return FALSE;

	/* reset all default-item flags */
	item = menu->items;
	for (i = 0; i < menu->nItems; i++, item++)
	{
	    item->fState &= ~MFS_DEFAULT;
	}

	/* no default item */
	if ( -1 == uItem)
	{
	    return TRUE;
	}

	item = menu->items;
	if ( bypos )
	{
	    if ( uItem >= menu->nItems ) return FALSE;
	    item[uItem].fState |= MFS_DEFAULT;
	    return TRUE;
	}
	else
	{
	    for (i = 0; i < menu->nItems; i++, item++)
	    {
		if (item->wID == uItem)
		{
		     item->fState |= MFS_DEFAULT;
		     return TRUE;
		}
	    }

	}
	return FALSE;
}

/**********************************************************************
 *		GetMenuDefaultItem    (USER32.@)
 */
UINT WINAPI GetMenuDefaultItem(HMENU hmenu, UINT bypos, UINT flags)
{
	POPUPMENU *menu;
	MENUITEM * item;
	UINT i = 0;

	TRACE("(%p,%d,%d)\n", hmenu, bypos, flags);

	if (!(menu = MENU_GetMenu(hmenu))) return -1;

	/* find default item */
	item = menu->items;

	/* empty menu */
	if (! item) return -1;

	while ( !( item->fState & MFS_DEFAULT ) )
	{
	    i++; item++;
	    if  (i >= menu->nItems ) return -1;
	}

	/* default: don't return disabled items */
	if ( (!(GMDI_USEDISABLED & flags)) && (item->fState & MFS_DISABLED )) return -1;

	/* search rekursiv when needed */
	if ( (item->fType & MF_POPUP) &&  (flags & GMDI_GOINTOPOPUPS) )
	{
	    UINT ret;
	    ret = GetMenuDefaultItem( item->hSubMenu, bypos, flags );
	    if ( -1 != ret ) return ret;

	    /* when item not found in submenu, return the popup item */
	}
	return ( bypos ) ? i : item->wID;

}


/**********************************************************************
 *		InsertMenuItemA    (USER32.@)
 */
BOOL WINAPI InsertMenuItemA(HMENU hMenu, UINT uItem, BOOL bypos,
                                const MENUITEMINFOA *lpmii)
{
    MENUITEM *item;
    MENUITEMINFOW mii;

    TRACE("hmenu %p, item %04x, by pos %d, info %p\n", hMenu, uItem, bypos, lpmii);

    if (!MENU_NormalizeMenuItemInfoStruct( (const MENUITEMINFOW *)lpmii, &mii )) return FALSE;

    item = MENU_InsertItem(hMenu, uItem, bypos ? MF_BYPOSITION : 0 );
    return SetMenuItemInfo_common(item, &mii, FALSE);
}


/**********************************************************************
 *		InsertMenuItemW    (USER32.@)
 */
BOOL WINAPI InsertMenuItemW(HMENU hMenu, UINT uItem, BOOL bypos,
                                const MENUITEMINFOW *lpmii)
{
    MENUITEM *item;
    MENUITEMINFOW mii;

    TRACE("hmenu %p, item %04x, by pos %d, info %p\n", hMenu, uItem, bypos, lpmii);

    if (!MENU_NormalizeMenuItemInfoStruct( lpmii, &mii )) return FALSE;

    item = MENU_InsertItem(hMenu, uItem, bypos ? MF_BYPOSITION : 0 );
    return SetMenuItemInfo_common(item, &mii, TRUE);
}

/**********************************************************************
 *		CheckMenuRadioItem    (USER32.@)
 */

BOOL WINAPI CheckMenuRadioItem(HMENU hMenu,
				   UINT first, UINT last, UINT check,
				   UINT bypos)
{
    BOOL done = FALSE;
    UINT i;
    MENUITEM *mi_first = NULL, *mi_check;
    HMENU m_first, m_check;

    for (i = first; i <= last; i++)
    {
        UINT pos = i;

        if (!mi_first)
        {
            m_first = hMenu;
            mi_first = MENU_FindItem(&m_first, &pos, bypos);
            if (!mi_first) continue;
            mi_check = mi_first;
            m_check = m_first;
        }
        else
        {
            m_check = hMenu;
            mi_check = MENU_FindItem(&m_check, &pos, bypos);
            if (!mi_check) continue;
        }

        if (m_first != m_check) continue;
        if (mi_check->fType == MFT_SEPARATOR) continue;

        if (i == check)
        {
            mi_check->fType |= MFT_RADIOCHECK;
            mi_check->fState |= MFS_CHECKED;
            done = TRUE;
        }
        else
        {
            /* MSDN is wrong, Windows does not remove MFT_RADIOCHECK */
            mi_check->fState &= ~MFS_CHECKED;
        }
    }

    return done;
}


/**********************************************************************
 *		GetMenuItemRect    (USER32.@)
 *
 *      ATTENTION: Here, the returned values in rect are the screen
 *                 coordinates of the item just like if the menu was
 *                 always on the upper left side of the application.
 *
 */
BOOL WINAPI GetMenuItemRect (HWND hwnd, HMENU hMenu, UINT uItem,
				 LPRECT rect)
{
     POPUPMENU *itemMenu;
     MENUITEM *item;
     HWND referenceHwnd;

     TRACE("(%p,%p,%d,%p)\n", hwnd, hMenu, uItem, rect);

     item = MENU_FindItem (&hMenu, &uItem, MF_BYPOSITION);
     referenceHwnd = hwnd;

     if(!hwnd)
     {
	 itemMenu = MENU_GetMenu(hMenu);
	 if (itemMenu == NULL)
	     return FALSE;

	 if(itemMenu->hWnd == 0)
	     return FALSE;
	 referenceHwnd = itemMenu->hWnd;
     }

     if ((rect == NULL) || (item == NULL))
	 return FALSE;

     *rect = item->rect;

     MapWindowPoints(referenceHwnd, 0, (LPPOINT)rect, 2);

     return TRUE;
}

/**********************************************************************
 *		SetMenuInfo    (USER32.@)
 *
 * FIXME
 *	actually use the items to draw the menu
 *      (recalculate and/or redraw)
 */
static BOOL menu_SetMenuInfo( HMENU hMenu, LPCMENUINFO lpmi)
{
    POPUPMENU *menu;
    if( !(menu = MENU_GetMenu(hMenu))) return FALSE;

    if (lpmi->fMask & MIM_BACKGROUND)
        menu->hbrBack = lpmi->hbrBack;

    if (lpmi->fMask & MIM_HELPID)
        menu->dwContextHelpID = lpmi->dwContextHelpID;

    if (lpmi->fMask & MIM_MAXHEIGHT)
        menu->cyMax = lpmi->cyMax;

    if (lpmi->fMask & MIM_MENUDATA)
        menu->dwMenuData = lpmi->dwMenuData;

    if (lpmi->fMask & MIM_STYLE)
        menu->dwStyle = lpmi->dwStyle;

    if( lpmi->fMask & MIM_APPLYTOSUBMENUS) {
        int i;
        MENUITEM *item = menu->items;
        for( i = menu->nItems; i; i--, item++)
            if( item->fType & MF_POPUP)
                menu_SetMenuInfo( item->hSubMenu, lpmi);
    }
    return TRUE;
}

BOOL WINAPI SetMenuInfo (HMENU hMenu, LPCMENUINFO lpmi)
{
    TRACE("(%p %p)\n", hMenu, lpmi);
    if( lpmi && (lpmi->cbSize == sizeof( MENUINFO)) && (menu_SetMenuInfo( hMenu, lpmi))) {
	if( lpmi->fMask & MIM_STYLE) {
	    if (lpmi->dwStyle & MNS_AUTODISMISS) FIXME("MNS_AUTODISMISS unimplemented\n");
	    if (lpmi->dwStyle & MNS_DRAGDROP) FIXME("MNS_DRAGDROP unimplemented\n");
	    if (lpmi->dwStyle & MNS_MODELESS) FIXME("MNS_MODELESS unimplemented\n");
	}
        return TRUE;
    }
    SetLastError( ERROR_INVALID_PARAMETER);
    return FALSE;
}

/**********************************************************************
 *		GetMenuInfo    (USER32.@)
 *
 *  NOTES
 *	win98/NT5.0
 *
 */
BOOL WINAPI GetMenuInfo (HMENU hMenu, LPMENUINFO lpmi)
{   POPUPMENU *menu;

    TRACE("(%p %p)\n", hMenu, lpmi);

    if (lpmi && (lpmi->cbSize == sizeof( MENUINFO)) && (menu = MENU_GetMenu(hMenu)))
    {

	if (lpmi->fMask & MIM_BACKGROUND)
	    lpmi->hbrBack = menu->hbrBack;

	if (lpmi->fMask & MIM_HELPID)
	    lpmi->dwContextHelpID = menu->dwContextHelpID;

	if (lpmi->fMask & MIM_MAXHEIGHT)
	    lpmi->cyMax = menu->cyMax;

	if (lpmi->fMask & MIM_MENUDATA)
	    lpmi->dwMenuData = menu->dwMenuData;

	if (lpmi->fMask & MIM_STYLE)
	    lpmi->dwStyle = menu->dwStyle;

	return TRUE;
    }
    SetLastError( ERROR_INVALID_PARAMETER);
    return FALSE;
}


/**********************************************************************
 *         SetMenuContextHelpId    (USER32.@)
 */
BOOL WINAPI SetMenuContextHelpId( HMENU hMenu, DWORD dwContextHelpID)
{
    LPPOPUPMENU menu;

    TRACE("(%p 0x%08x)\n", hMenu, dwContextHelpID);

    if ((menu = MENU_GetMenu(hMenu)))
    {
	menu->dwContextHelpID = dwContextHelpID;
	return TRUE;
    }
    return FALSE;
}


/**********************************************************************
 *         GetMenuContextHelpId    (USER32.@)
 */
DWORD WINAPI GetMenuContextHelpId( HMENU hMenu )
{
    LPPOPUPMENU menu;

    TRACE("(%p)\n", hMenu);

    if ((menu = MENU_GetMenu(hMenu)))
    {
	return menu->dwContextHelpID;
    }
    return 0;
}

/**********************************************************************
 *         MenuItemFromPoint    (USER32.@)
 */
INT WINAPI MenuItemFromPoint(HWND hWnd, HMENU hMenu, POINT ptScreen)
{
    POPUPMENU *menu = MENU_GetMenu(hMenu);
    UINT pos;

    /*FIXME: Do we have to handle hWnd here? */
    if (!menu) return -1;
    if (!MENU_FindItemByCoords(menu, ptScreen, &pos)) return -1;
    return pos;
}


/**********************************************************************
 *           translate_accelerator
 */
static BOOL translate_accelerator( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam,
                                   BYTE fVirt, WORD key, WORD cmd )
{
    INT mask = 0;
    UINT mesg = 0;

    if (wParam != key) return FALSE;

    if (GetKeyState(VK_CONTROL) & 0x8000) mask |= FCONTROL;
    if (GetKeyState(VK_MENU) & 0x8000) mask |= FALT;
    if (GetKeyState(VK_SHIFT) & 0x8000) mask |= FSHIFT;

    if (message == WM_CHAR || message == WM_SYSCHAR)
    {
        if ( !(fVirt & FVIRTKEY) && (mask & FALT) == (fVirt & FALT) )
        {
            TRACE_(accel)("found accel for WM_CHAR: ('%c')\n", LOWORD(wParam) & 0xff);
            goto found;
        }
    }
    else
    {
        if(fVirt & FVIRTKEY)
        {
            TRACE_(accel)("found accel for virt_key %04lx (scan %04x)\n",
                          wParam, 0xff & HIWORD(lParam));

            if(mask == (fVirt & (FSHIFT | FCONTROL | FALT))) goto found;
            TRACE_(accel)(", but incorrect SHIFT/CTRL/ALT-state\n");
        }
        else
        {
            if (!(lParam & 0x01000000))  /* no special_key */
            {
                if ((fVirt & FALT) && (lParam & 0x20000000))
                {                              /* ^^ ALT pressed */
                    TRACE_(accel)("found accel for Alt-%c\n", LOWORD(wParam) & 0xff);
                    goto found;
                }
            }
        }
    }
    return FALSE;

 found:
    if (message == WM_KEYUP || message == WM_SYSKEYUP)
        mesg = 1;
    else
    {
        HMENU hMenu, hSubMenu, hSysMenu;
        UINT uSysStat = (UINT)-1, uStat = (UINT)-1, nPos;

        hMenu = (GetWindowLongW( hWnd, GWL_STYLE ) & WS_CHILD) ? 0 : GetMenu(hWnd);
        hSysMenu = get_win_sys_menu( hWnd );

        /* find menu item and ask application to initialize it */
        /* 1. in the system menu */
        hSubMenu = hSysMenu;
        nPos = cmd;
        if(MENU_FindItem(&hSubMenu, &nPos, MF_BYCOMMAND))
        {
            if (GetCapture())
                mesg = 2;
            if (!IsWindowEnabled(hWnd))
                mesg = 3;
            else
            {
                SendMessageW(hWnd, WM_INITMENU, (WPARAM)hSysMenu, 0L);
                if(hSubMenu != hSysMenu)
                {
                    nPos = MENU_FindSubMenu(&hSysMenu, hSubMenu);
                    TRACE_(accel)("hSysMenu = %p, hSubMenu = %p, nPos = %d\n", hSysMenu, hSubMenu, nPos);
                    SendMessageW(hWnd, WM_INITMENUPOPUP, (WPARAM)hSubMenu, MAKELPARAM(nPos, TRUE));
                }
                uSysStat = GetMenuState(GetSubMenu(hSysMenu, 0), cmd, MF_BYCOMMAND);
            }
        }
        else /* 2. in the window's menu */
        {
            hSubMenu = hMenu;
            nPos = cmd;
            if(MENU_FindItem(&hSubMenu, &nPos, MF_BYCOMMAND))
            {
                if (GetCapture())
                    mesg = 2;
                if (!IsWindowEnabled(hWnd))
                    mesg = 3;
                else
                {
                    SendMessageW(hWnd, WM_INITMENU, (WPARAM)hMenu, 0L);
                    if(hSubMenu != hMenu)
                    {
                        nPos = MENU_FindSubMenu(&hMenu, hSubMenu);
                        TRACE_(accel)("hMenu = %p, hSubMenu = %p, nPos = %d\n", hMenu, hSubMenu, nPos);
                        SendMessageW(hWnd, WM_INITMENUPOPUP, (WPARAM)hSubMenu, MAKELPARAM(nPos, FALSE));
                    }
                    uStat = GetMenuState(hMenu, cmd, MF_BYCOMMAND);
                }
            }
        }

        if (mesg == 0)
        {
            if (uSysStat != (UINT)-1)
            {
                if (uSysStat & (MF_DISABLED|MF_GRAYED))
                    mesg=4;
                else
                    mesg=WM_SYSCOMMAND;
            }
            else
            {
                if (uStat != (UINT)-1)
                {
                    if (IsIconic(hWnd))
                        mesg=5;
                    else
                    {
                        if (uStat & (MF_DISABLED|MF_GRAYED))
                            mesg=6;
                        else
                            mesg=WM_COMMAND;
                    }
                }
                else
                    mesg=WM_COMMAND;
            }
        }
    }

    if( mesg==WM_COMMAND )
    {
        TRACE_(accel)(", sending WM_COMMAND, wParam=%0x\n", 0x10000 | cmd);
        SendMessageW(hWnd, mesg, 0x10000 | cmd, 0L);
    }
    else if( mesg==WM_SYSCOMMAND )
    {
        TRACE_(accel)(", sending WM_SYSCOMMAND, wParam=%0x\n", cmd);
        SendMessageW(hWnd, mesg, cmd, 0x00010000L);
    }
    else
    {
        /*  some reasons for NOT sending the WM_{SYS}COMMAND message:
         *   #0: unknown (please report!)
         *   #1: for WM_KEYUP,WM_SYSKEYUP
         *   #2: mouse is captured
         *   #3: window is disabled
         *   #4: it's a disabled system menu option
         *   #5: it's a menu option, but window is iconic
         *   #6: it's a menu option, but disabled
         */
        TRACE_(accel)(", but won't send WM_{SYS}COMMAND, reason is #%d\n",mesg);
        if(mesg==0)
            ERR_(accel)(" unknown reason - please report!\n");
    }
    return TRUE;
}

/**********************************************************************
 *      TranslateAcceleratorA     (USER32.@)
 *      TranslateAccelerator      (USER32.@)
 */
INT WINAPI TranslateAcceleratorA( HWND hWnd, HACCEL hAccel, LPMSG msg )
{
    switch (msg->message)
    {
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        return TranslateAcceleratorW( hWnd, hAccel, msg );

    case WM_CHAR:
    case WM_SYSCHAR:
        {
            MSG msgW = *msg;
            char ch = LOWORD(msg->wParam);
            WCHAR wch;
            MultiByteToWideChar(CP_ACP, 0, &ch, 1, &wch, 1);
            msgW.wParam = MAKEWPARAM(wch, HIWORD(msg->wParam));
            return TranslateAcceleratorW( hWnd, hAccel, &msgW );
        }

    default:
        return 0;
    }
}

/**********************************************************************
 *      TranslateAcceleratorW     (USER32.@)
 */
INT WINAPI TranslateAcceleratorW( HWND hWnd, HACCEL hAccel, LPMSG msg )
{
    ACCEL data[32], *ptr = data;
    int i, count;

    if (!hWnd) return 0;

    if (msg->message != WM_KEYDOWN &&
        msg->message != WM_SYSKEYDOWN &&
        msg->message != WM_CHAR &&
        msg->message != WM_SYSCHAR)
        return 0;

    TRACE_(accel)("hAccel %p, hWnd %p, msg->hwnd %p, msg->message %04x, wParam %08lx, lParam %08lx\n",
                  hAccel,hWnd,msg->hwnd,msg->message,msg->wParam,msg->lParam);

    if (!(count = CopyAcceleratorTableW( hAccel, NULL, 0 ))) return 0;
    if (count > sizeof(data)/sizeof(data[0]))
    {
        if (!(ptr = HeapAlloc( GetProcessHeap(), 0, count * sizeof(*ptr) ))) return 0;
    }
    count = CopyAcceleratorTableW( hAccel, ptr, count );
    for (i = 0; i < count; i++)
    {
        if (translate_accelerator( hWnd, msg->message, msg->wParam, msg->lParam,
                                   ptr[i].fVirt, ptr[i].key, ptr[i].cmd))
            break;
    }
    if (ptr != data) HeapFree( GetProcessHeap(), 0, ptr );
    return (i < count);
}
