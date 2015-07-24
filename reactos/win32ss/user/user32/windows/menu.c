/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS user32.dll
 * FILE:            user32/windows/menu.c
 * PURPOSE:         Menus
 *
 * PROGRAMMERS:     Casper S. Hornstrup
 *                  James Tabor
 */

/* INCLUDES ******************************************************************/

#include <user32.h>
#include <wine/debug.h>

LRESULT DefWndNCPaint(HWND hWnd, HRGN hRgn, BOOL Active);
BOOL WINAPI GdiValidateHandle(HGDIOBJ hobj);
LRESULT DefWndNCHitTest(HWND hWnd, POINT Point);
void FASTCALL NcGetSysPopupPos(HWND Wnd, RECT *Rect);

WINE_DEFAULT_DEBUG_CHANNEL(menu);

/* internal popup menu window messages */

#define MM_SETMENUHANDLE	(WM_USER + 0)
#define MM_GETMENUHANDLE	(WM_USER + 1)

/* internal flags for menu tracking */

#define TF_ENDMENU              0x10000
#define TF_SUSPENDPOPUP         0x20000
#define TF_SKIPREMOVE           0x40000

#define ITEM_PREV		-1
#define ITEM_NEXT		 1

/* Internal MenuTrackMenu() flags */
#define TPM_INTERNAL		0xF0000000
#define TPM_BUTTONDOWN		0x40000000		/* menu was clicked before tracking */
#define TPM_POPUPMENU           0x20000000              /* menu is a popup menu */

  /* Space between 2 columns */
#define MENU_COL_SPACE 4

/*  top and bottom margins for popup menus */
#define MENU_TOP_MARGIN 2 //3
#define MENU_BOTTOM_MARGIN 2

#define MENU_TYPE_MASK (MF_STRING | MF_BITMAP | MF_OWNERDRAW | MF_SEPARATOR)

#define MENU_ITEM_TYPE(flags) ((flags) & MENU_TYPE_MASK)

#define MNS_STYLE_MASK (MNS_NOCHECK|MNS_MODELESS|MNS_DRAGDROP|MNS_AUTODISMISS|MNS_NOTIFYBYPOS|MNS_CHECKORBMP)

#define MENUITEMINFO_TYPE_MASK \
                (MFT_STRING | MFT_BITMAP | MFT_OWNERDRAW | MFT_SEPARATOR | \
                 MFT_MENUBARBREAK | MFT_MENUBREAK | MFT_RADIOCHECK | \
                 MFT_RIGHTORDER | MFT_RIGHTJUSTIFY /* same as MF_HELP */ )

#define TYPE_MASK  (MENUITEMINFO_TYPE_MASK | MF_POPUP | MF_SYSMENU)

#define STATE_MASK (~TYPE_MASK)

#define MENUITEMINFO_STATE_MASK (STATE_MASK & ~(MF_BYPOSITION | MF_MOUSESELECT))

#define MII_STATE_MASK (MFS_GRAYED|MFS_CHECKED|MFS_HILITE|MFS_DEFAULT)

/* macro to test that flags do not indicate bitmap, ownerdraw or separator */
#define IS_STRING_ITEM(flags) (MF_STRING == MENU_ITEM_TYPE(flags))
#define IS_MAGIC_BITMAP(id) ((id) && ((INT_PTR)(id) < 12) && ((INT_PTR)(id) >= -1))

#define IS_SYSTEM_MENU(MenuInfo)  \
	(0 == ((MenuInfo)->fFlags & MNF_POPUP) && 0 != ((MenuInfo)->fFlags & MNF_SYSMENU))

#define IS_SYSTEM_POPUP(MenuInfo) \
	(0 != ((MenuInfo)->fFlags & MNF_POPUP) && 0 != ((MenuInfo)->fFlags & MNF_SYSMENU))

#define IS_BITMAP_ITEM(flags) (MF_BITMAP == MENU_ITEM_TYPE(flags))

/* Use global popup window because there's no way 2 menus can
 * be tracked at the same time.  */
static HWND top_popup;
static HMENU top_popup_hmenu;

/* Flag set by EndMenu() to force an exit from menu tracking */
static BOOL fEndMenu = FALSE;

#define MENU_ITEM_HBMP_SPACE (5)
#define MENU_BAR_ITEMS_SPACE (12)
#define SEPARATOR_HEIGHT (5)
#define MENU_TAB_SPACE (8)

typedef struct
{
  UINT  TrackFlags;
  HMENU CurrentMenu; /* current submenu (can be equal to hTopMenu)*/
  HMENU TopMenu;     /* initial menu */
  HWND  OwnerWnd;    /* where notifications are sent */
  POINT Pt;
} MTRACKER;


/*********************************************************************
 * PopupMenu class descriptor
 */
const struct builtin_class_descr POPUPMENU_builtin_class =
{
    WC_MENU,                     /* name */
    CS_SAVEBITS | CS_DBLCLKS,                  /* style  */
    NULL,                                      /* FIXME - procA */
    PopupMenuWndProcW,                         /* FIXME - procW */
    sizeof(MENUINFO *),                        /* extra */
    (LPCWSTR) IDC_ARROW,                       /* cursor */
    (HBRUSH)(COLOR_MENU + 1)                   /* brush */
};

#ifndef GET_WORD
#define GET_WORD(ptr)  (*(WORD *)(ptr))
#endif
#ifndef GET_DWORD
#define GET_DWORD(ptr) (*(DWORD *)(ptr))
#endif

HFONT hMenuFont = NULL;
HFONT hMenuFontBold = NULL;

/* Dimension of the menu bitmaps */
static HBITMAP BmpSysMenu = NULL;

static SIZE MenuCharSize;


/***********************************************************************
 *           MENU_GetMenu
 *
 * Validate the given menu handle and returns the menu structure pointer.
 */
FORCEINLINE PMENU MENU_GetMenu(HMENU hMenu)
{
    return ValidateHandleNoErr(hMenu, TYPE_MENU); 
}

/***********************************************************************
 *           get_win_sys_menu
 *
 * Get the system menu of a window
 */
static HMENU get_win_sys_menu( HWND hwnd )
{
    HMENU ret = 0;
    WND *win = ValidateHwnd( hwnd );
    if (win)
    {
        ret = win->SystemMenu;
    }
    return ret;
}

/***********************************************************************
 *           MENU_FindItem
 *
 * Find a menu item. Return a pointer on the item, and modifies *hmenu
 * in case the item was in a sub-menu.
 */
ITEM *MENU_FindItem( HMENU *hmenu, UINT *nPos, UINT wFlags )
{
    MENU *menu;
    ITEM *fallback = NULL;
    UINT fallback_pos = 0;
    UINT i;
    PITEM pItem;

    if ((*hmenu == (HMENU)0xffff) || (!(menu = MENU_GetMenu(*hmenu)))) return NULL;
    if (wFlags & MF_BYPOSITION)
    {
	if (*nPos >= menu->cItems) return NULL;
	pItem = menu->rgItems ? DesktopPtrToUser(menu->rgItems) : NULL;
	if (pItem) pItem = &pItem[*nPos];
	return pItem;
    }
    else
    {
        PITEM item = menu->rgItems ? DesktopPtrToUser(menu->rgItems) : NULL;
	for (i = 0; item, i < menu->cItems; i++, item++)
	{
	    if (item->spSubMenu)
	    {
	        PMENU pSubMenu = DesktopPtrToUser(item->spSubMenu);
		HMENU hsubmenu = UserHMGetHandle(pSubMenu);
		ITEM *subitem = MENU_FindItem( &hsubmenu, nPos, wFlags );
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

#define MAX_GOINTOSUBMENU (0x10)
UINT FASTCALL
IntGetMenuDefaultItem(PMENU Menu, BOOL fByPos, UINT gmdiFlags, DWORD *gismc)
{
   UINT i = 0;
   PITEM Item = Menu->rgItems ? DesktopPtrToUser(Menu->rgItems) : NULL;

   /* empty menu */
   if (!Item) return -1;

   while ( !( Item->fState & MFS_DEFAULT ) )
   {
      i++; Item++;
      if  (i >= Menu->cItems ) return -1;
   }

   /* default: don't return disabled items */
   if ( (!(GMDI_USEDISABLED & gmdiFlags)) && (Item->fState & MFS_DISABLED )) return -1;

   /* search rekursiv when needed */
   if ( (Item->fType & MF_POPUP) &&  (gmdiFlags & GMDI_GOINTOPOPUPS) && Item->spSubMenu)
   {
      UINT ret;
      (*gismc)++;
      ret = IntGetMenuDefaultItem( DesktopPtrToUser(Item->spSubMenu), fByPos, gmdiFlags, gismc );
      (*gismc)--;
      if ( -1 != ret ) return ret;

      /* when item not found in submenu, return the popup item */
   }
   return ( fByPos ) ? i : Item->wID;
}

static BOOL GetMenuItemInfo_common ( HMENU hmenu,
                                     UINT item,
                                     BOOL bypos,
                                     LPMENUITEMINFOW lpmii,
                                     BOOL unicode)
{
    ITEM *pItem = MENU_FindItem (&hmenu, &item, bypos ? MF_BYPOSITION : 0);

    //debug_print_menuitem("GetMenuItemInfo_common: ", pItem, "");

    if (!pItem)
    {
        SetLastError( ERROR_MENU_ITEM_NOT_FOUND);
        return FALSE;
    }

    if( lpmii->fMask & MIIM_TYPE)
    {
        if( lpmii->fMask & ( MIIM_STRING | MIIM_FTYPE | MIIM_BITMAP))
        {
            ERR("invalid combination of fMask bits used\n");
            /* this does not happen on Win9x/ME */
            SetLastError( ERROR_INVALID_PARAMETER);
            return FALSE;
        }
	lpmii->fType = pItem->fType & MENUITEMINFO_TYPE_MASK;
        if( pItem->hbmp) lpmii->fType |= MFT_BITMAP;
	lpmii->hbmpItem = pItem->hbmp; /* not on Win9x/ME */
        if( lpmii->fType & MFT_BITMAP)
        {
	    lpmii->dwTypeData = (LPWSTR) pItem->hbmp;
	    lpmii->cch = 0;
        }
        else if( lpmii->fType & (MFT_OWNERDRAW | MFT_SEPARATOR))
        {
            /* this does not happen on Win9x/ME */
	    lpmii->dwTypeData = 0;
	    lpmii->cch = 0;
        }
    }

    /* copy the text string */
    if ((lpmii->fMask & (MIIM_TYPE|MIIM_STRING)))
    {
         if( !pItem->Xlpstr )
         {                                         // Very strange this fixes a wine test with a crash.
                if(lpmii->dwTypeData && lpmii->cch && !(GdiValidateHandle((HGDIOBJ)lpmii->dwTypeData)) )
                {
                    if( unicode)
                        *((WCHAR *)lpmii->dwTypeData) = 0;
                    else
                        *((CHAR *)lpmii->dwTypeData) = 0;
                }
                lpmii->cch = 0;
         }
         else
         {
            int len;
            LPWSTR text = DesktopPtrToUser(pItem->Xlpstr);
            if (unicode)
            {
                len = strlenW(text);
                if(lpmii->dwTypeData && lpmii->cch)
                    lstrcpynW(lpmii->dwTypeData, text, lpmii->cch);
            }
            else
            {
                len = WideCharToMultiByte( CP_ACP, 0, text, -1, NULL, 0, NULL, NULL ) - 1;
                if(lpmii->dwTypeData && lpmii->cch)
                    if (!WideCharToMultiByte( CP_ACP, 0, text, -1,
                            (LPSTR)lpmii->dwTypeData, lpmii->cch, NULL, NULL ))
                        ((LPSTR)lpmii->dwTypeData)[lpmii->cch - 1] = 0;
            }
            /* if we've copied a substring we return its length */
            if(lpmii->dwTypeData && lpmii->cch)
                if (lpmii->cch <= len + 1)
                    lpmii->cch--;
                else
                    lpmii->cch = len;
            else
            {
                /* return length of string */
                /* not on Win9x/ME if fType & MFT_BITMAP */
                lpmii->cch = len;
            }
        }
    }

    if (lpmii->fMask & MIIM_FTYPE)
	lpmii->fType = pItem->fType & MENUITEMINFO_TYPE_MASK;

    if (lpmii->fMask & MIIM_BITMAP)
	lpmii->hbmpItem = pItem->hbmp;

    if (lpmii->fMask & MIIM_STATE)
	lpmii->fState = pItem->fState & MENUITEMINFO_STATE_MASK;

    if (lpmii->fMask & MIIM_ID)
	lpmii->wID = pItem->wID;

    if (lpmii->fMask & MIIM_SUBMENU && pItem->spSubMenu )
    {
        PMENU pSubMenu = DesktopPtrToUser(pItem->spSubMenu);
        HMENU hSubMenu = UserHMGetHandle(pSubMenu);
	lpmii->hSubMenu = hSubMenu;
    }
    else
    {
        /* hSubMenu is always cleared 
         * (not on Win9x/ME ) */
        lpmii->hSubMenu = 0;
    }

    if (lpmii->fMask & MIIM_CHECKMARKS)
    {
	lpmii->hbmpChecked = pItem->hbmpChecked;
	lpmii->hbmpUnchecked = pItem->hbmpUnchecked;
    }
    if (lpmii->fMask & MIIM_DATA)
	lpmii->dwItemData = pItem->dwItemData;

  return TRUE;
}

/***********************************************************************
 *           MenuGetRosMenuInfo
 *
 * Get full information about menu
 */
static BOOL FASTCALL
MenuGetRosMenuInfo(PROSMENUINFO MenuInfo, HMENU Menu)
{
  PMENU pMenu;
  if (!(pMenu = ValidateHandleNoErr(Menu, TYPE_MENU))) return FALSE;

  MenuInfo->hbrBack = pMenu->hbrBack;
  MenuInfo->dwContextHelpID = pMenu->dwContextHelpId;
  MenuInfo->cyMax = pMenu->cyMax;
  MenuInfo->dwMenuData = pMenu->dwMenuData;
  MenuInfo->dwStyle = pMenu->fFlags & MNS_STYLE_MASK;

  MenuInfo->cItems = pMenu->cItems;

  MenuInfo->iItem = pMenu->iItem;
  MenuInfo->cxMenu = pMenu->cxMenu;
  MenuInfo->cyMenu = pMenu->cyMenu;
  MenuInfo->spwndNotify = pMenu->spwndNotify;
  MenuInfo->cxTextAlign = pMenu->cxTextAlign;
  MenuInfo->iTop = pMenu->iTop;
  MenuInfo->iMaxTop = pMenu->iMaxTop;
  MenuInfo->dwArrowsOn = pMenu->dwArrowsOn;

  MenuInfo->fFlags = pMenu->fFlags;
  MenuInfo->Self = pMenu->head.h;
  MenuInfo->TimeToHide = pMenu->TimeToHide;
  MenuInfo->Wnd = pMenu->hWnd;
  return TRUE;
}

/***********************************************************************
 *           MenuSetRosMenuInfo
 *
 * Set full information about menu
 */
static BOOL FASTCALL
MenuSetRosMenuInfo(PROSMENUINFO MenuInfo)
{
  MenuInfo->cbSize = sizeof(ROSMENUINFO);
  MenuInfo->fMask = MIM_BACKGROUND | MIM_HELPID | MIM_MAXHEIGHT | MIM_MENUDATA | MIM_STYLE;

  return NtUserThunkedMenuInfo(MenuInfo->Self, (LPCMENUINFO)MenuInfo);
}

/***********************************************************************
 *           MenuInitRosMenuItemInfo
 *
 * Initialize a buffer for use with MenuGet/SetRosMenuItemInfo
 */
static VOID FASTCALL
MenuInitRosMenuItemInfo(PROSMENUITEMINFO ItemInfo)
{
  ZeroMemory(ItemInfo, sizeof(ROSMENUITEMINFO));
  ItemInfo->cbSize = sizeof(ROSMENUITEMINFO);
}

/***********************************************************************
 *           MenuGetRosMenuItemInfo
 *
 * Get full information about a menu item
 */
static BOOL FASTCALL
MenuGetRosMenuItemInfo(HMENU Menu, UINT Index, PROSMENUITEMINFO ItemInfo)
{
  PITEM pItem;
  UINT Save_Mask = ItemInfo->fMask; /* Save the org mask bits. */

  if (ItemInfo->dwTypeData != NULL)
  {
      HeapFree(GetProcessHeap(), 0, ItemInfo->dwTypeData);
  }

  ItemInfo->dwTypeData = NULL;

  if (!(pItem = MENU_FindItem(&Menu, &Index, MF_BYPOSITION)))
  {
      ItemInfo->fType = 0;
      return FALSE;
  }

  ItemInfo->fType = pItem->fType;
  ItemInfo->hbmpItem = pItem->hbmp;
  ItemInfo->hbmpChecked = pItem->hbmpChecked;
  ItemInfo->hbmpUnchecked = pItem->hbmpUnchecked;
  ItemInfo->dwItemData = pItem->dwItemData;
  ItemInfo->wID = pItem->wID;
  ItemInfo->fState = pItem->fState;

  if (pItem->spSubMenu)
  {
     PMENU pSubMenu = DesktopPtrToUser(pItem->spSubMenu);
     HMENU hSubMenu = UserHMGetHandle(pSubMenu);
     ItemInfo->hSubMenu = hSubMenu;
  }
  else
     ItemInfo->hSubMenu = NULL;

  if (MENU_ITEM_TYPE(ItemInfo->fType) == MF_STRING)
  {
     LPWSTR lpstr = pItem->lpstr.Buffer ? DesktopPtrToUser(pItem->lpstr.Buffer) : NULL;
     if (lpstr)
     {
        ItemInfo->cch = pItem->lpstr.Length / sizeof(WCHAR);
        ItemInfo->cch++;
        ItemInfo->dwTypeData = HeapAlloc(GetProcessHeap(), 0, ItemInfo->cch * sizeof(WCHAR));
        if (ItemInfo->dwTypeData == NULL)
        {
            return FALSE;
        }
        RtlCopyMemory(ItemInfo->dwTypeData, lpstr, min(ItemInfo->cch * sizeof(WCHAR), pItem->lpstr.MaximumLength));
     }
     else
     {
        ItemInfo->cch = 0;
     }
  }

  ItemInfo->Rect.left   = pItem->xItem; 
  ItemInfo->Rect.top    = pItem->yItem; 
  ItemInfo->Rect.right  = pItem->cxItem; // Do this for now......
  ItemInfo->Rect.bottom = pItem->cyItem;
  ItemInfo->dxTab = pItem->dxTab;
  ItemInfo->lpstr = pItem->lpstr.Buffer ? DesktopPtrToUser(pItem->lpstr.Buffer) : NULL;
  ItemInfo->maxBmpSize.cx = pItem->cxBmp;
  ItemInfo->maxBmpSize.cy = pItem->cyBmp;

  ItemInfo->fMask =  Save_Mask;
  return TRUE;
}

/***********************************************************************
 *           MenuSetRosMenuItemInfo
 *
 * Set selected information about a menu item, need to set the mask bits.
 */
static BOOL FASTCALL
MenuSetRosMenuItemInfo(HMENU Menu, UINT Index, PROSMENUITEMINFO ItemInfo)
{
  BOOL Ret;

  if (MENU_ITEM_TYPE(ItemInfo->fType) == MF_STRING &&
      ItemInfo->dwTypeData != NULL)
  {
    ItemInfo->cch = strlenW(ItemInfo->dwTypeData);
  }
  if (ItemInfo->hSubMenu)
  {
     if (!IsMenu(ItemInfo->hSubMenu)) ItemInfo->hSubMenu = NULL;
  }
  Ret = NtUserThunkedMenuItemInfo(Menu, Index, TRUE, FALSE, (LPMENUITEMINFOW)ItemInfo, NULL);
  return Ret;
}

/***********************************************************************
 *           MenuCleanupRosMenuItemInfo
 *
 * Cleanup after use of MenuGet/SetRosMenuItemInfo
 */
static VOID FASTCALL
MenuCleanupRosMenuItemInfo(PROSMENUITEMINFO ItemInfo)
{
  if (ItemInfo->dwTypeData != NULL)
  {
      HeapFree(GetProcessHeap(), 0, ItemInfo->dwTypeData);
      ItemInfo->dwTypeData = NULL;
  }
}

/***********************************************************************
 *           MenuInitSysMenuPopup
 *
 * Grey the appropriate items in System menu.
 */
void FASTCALL MenuInitSysMenuPopup(HMENU hmenu, DWORD style, DWORD clsStyle, LONG HitTest )
{
    BOOL gray;
    UINT DefItem;
    #if 0
    MENUITEMINFOW mii;
    #endif

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

    /* Set default menu item */
    if(style & WS_MINIMIZE) DefItem = SC_RESTORE;
    else if(HitTest == HTCAPTION) DefItem = ((style & (WS_MAXIMIZE | WS_MINIMIZE)) ? SC_RESTORE : SC_MAXIMIZE);
    else DefItem = SC_CLOSE;
#if 0
    mii.cbSize = sizeof(MENUITEMINFOW);
    mii.fMask |= MIIM_STATE;
    if((DefItem != SC_CLOSE) && GetMenuItemInfoW(hmenu, DefItem, FALSE, &mii) &&
       (mii.fState & (MFS_GRAYED | MFS_DISABLED))) DefItem = SC_CLOSE;
#endif
    SetMenuDefaultItem(hmenu, DefItem, MF_BYCOMMAND);
}

/******************************************************************************
 *
 *   UINT MenuGetStartOfNextColumn(
 *     PROSMENUINFO MenuInfo)
 *
 *****************************************************************************/

static UINT  MENU_GetStartOfNextColumn(
    HMENU  hMenu )
{
    MENU *menu = MENU_GetMenu(hMenu);
    PITEM pItem;
    UINT i;

    if(!menu)
	return NO_SELECTED_ITEM;

    i = menu->iItem + 1;
    if( i == NO_SELECTED_ITEM )
	return i;

    pItem = menu->rgItems ? DesktopPtrToUser(menu->rgItems) : NULL;
    if (!pItem) return NO_SELECTED_ITEM;
    for( ; i < menu->cItems; ++i ) {
	if (pItem[i].fType & (MF_MENUBREAK | MF_MENUBARBREAK))
	    return i;
    }

    return NO_SELECTED_ITEM;
}

/******************************************************************************
 *
 *   UINT MenuGetStartOfPrevColumn(
 *     PROSMENUINFO MenuInfo)
 *
 *****************************************************************************/
static UINT  MENU_GetStartOfPrevColumn(
    HMENU  hMenu )
{
    MENU *menu = MENU_GetMenu(hMenu);
    UINT  i;
    PITEM pItem;

    if( !menu )
	return NO_SELECTED_ITEM;

    if( menu->iItem == 0 || menu->iItem == NO_SELECTED_ITEM )
	return NO_SELECTED_ITEM;

    pItem = menu->rgItems ? DesktopPtrToUser(menu->rgItems) : NULL;
    if (!pItem) return NO_SELECTED_ITEM;

    /* Find the start of the column */

    for(i = menu->iItem; i != 0 &&
	 !(pItem[i].fType & (MF_MENUBREAK | MF_MENUBARBREAK));
	--i); /* empty */

    if(i == 0)
	return NO_SELECTED_ITEM;

    for(--i; i != 0; --i) {
	if (pItem[i].fType & (MF_MENUBREAK | MF_MENUBARBREAK))
	    break;
    }

    TRACE("ret %d.\n", i );

    return i;
}

/***********************************************************************
 *           MenuLoadBitmaps
 *
 * Load the arrow bitmap. We can't do this from MenuInit since user32
 * can also be used (and thus initialized) from text-mode.
 */
static void FASTCALL
MenuLoadBitmaps(VOID)
{
  /* Load system buttons bitmaps */
  if (BmpSysMenu == NULL)
  {
      BmpSysMenu = LoadBitmapW(0, MAKEINTRESOURCEW(OBM_CLOSE));
  }
}
/////////// Make gpsi OBMI via callback //////////////
/***********************************************************************
 *           get_arrow_bitmap
 */
HBITMAP get_arrow_bitmap(void)
{
    static HBITMAP arrow_bitmap;

    if (!arrow_bitmap) arrow_bitmap = LoadBitmapW(0, MAKEINTRESOURCEW(OBM_MNARROW));
    return arrow_bitmap;
}

/***********************************************************************
 *           get_down_arrow_bitmap DFCS_MENUARROWDOWN
 */
HBITMAP get_down_arrow_bitmap(void)
{
    static HBITMAP arrow_bitmap;

    if (!arrow_bitmap) arrow_bitmap = LoadBitmapW(0, MAKEINTRESOURCEW(OBM_DNARROW));
    return arrow_bitmap;
}

/***********************************************************************
 *           get_down_arrow_inactive_bitmap DFCS_MENUARROWDOWN | DFCS_INACTIVE
 */
HBITMAP get_down_arrow_inactive_bitmap(void)
{
    static HBITMAP arrow_bitmap;

    if (!arrow_bitmap) arrow_bitmap = LoadBitmapW(0, MAKEINTRESOURCEW(OBM_DNARROWI));
    return arrow_bitmap;
}

/***********************************************************************
 *           get_up_arrow_bitmap DFCS_MENUARROWUP
 */
HBITMAP get_up_arrow_bitmap(void)
{
    static HBITMAP arrow_bitmap;

    if (!arrow_bitmap) arrow_bitmap = LoadBitmapW(0, MAKEINTRESOURCEW(OBM_UPARROW));
    return arrow_bitmap;
}

/***********************************************************************
 *           get_up_arrow_inactive_bitmap DFCS_MENUARROWUP | DFCS_INACTIVE
 */
static HBITMAP get_up_arrow_inactive_bitmap(void)
{
    static HBITMAP arrow_bitmap;

    if (!arrow_bitmap) arrow_bitmap = LoadBitmapW(0, MAKEINTRESOURCEW(OBM_UPARROWI));
    return arrow_bitmap;
}
////////////////
/***********************************************************************
 *           MenuFindSubMenu
 *
 * Find a Sub menu. Return the position of the submenu, and modifies
 * *hmenu in case it is found in another sub-menu.
 * If the submenu cannot be found, NO_SELECTED_ITEM is returned.
 */
static UINT FASTCALL MenuFindSubMenu(HMENU *hmenu, HMENU hSubTarget )
{
    PMENU menu, pSubMenu;
    HMENU hSubMenu;
    UINT i;
    PITEM item;

    if (((*hmenu)==(HMENU)0xffff) ||(!(menu = MENU_GetMenu(*hmenu))))
        return NO_SELECTED_ITEM;

    item = menu->rgItems ? DesktopPtrToUser(menu->rgItems) : NULL;
    for (i = 0; i < menu->cItems; i++, item++)
    {
        if (!item->spSubMenu)
           continue;
        else
        {
           pSubMenu = DesktopPtrToUser(item->spSubMenu);
           hSubMenu = UserHMGetHandle(pSubMenu);
           if (hSubMenu == hSubTarget)
           {
              return i;
           }
           else 
           {
              HMENU hsubmenu = hSubMenu;
              UINT pos = MenuFindSubMenu( &hsubmenu, hSubTarget );
              if (pos != NO_SELECTED_ITEM)
              {
                  *hmenu = hsubmenu;
                  return pos;
              }
           }
        }
    }
    return NO_SELECTED_ITEM;
}

/***********************************************************************
 *           MENU_AdjustMenuItemRect
 *
 * Adjust menu item rectangle according to scrolling state.
 */
static void
MENU_AdjustMenuItemRect(PROSMENUINFO menu, LPRECT rect)
{
    if (menu->dwArrowsOn)
    {
        UINT arrow_bitmap_height;
        BITMAP bmp;

        GetObjectW(get_up_arrow_bitmap(), sizeof(bmp), &bmp);
        arrow_bitmap_height = bmp.bmHeight;
        rect->top += arrow_bitmap_height - menu->iTop;
        rect->bottom += arrow_bitmap_height - menu->iTop;
    }
}

/***********************************************************************
 *           MenuDrawPopupGlyph
 *
 * Draws popup magic glyphs (can be found in system menu).
 */
static void FASTCALL
MenuDrawPopupGlyph(HDC dc, LPRECT r, INT_PTR popupMagic, BOOL inactive, BOOL hilite)
{
  LOGFONTW lf;
  HFONT hFont, hOldFont;
  COLORREF clrsave;
  INT bkmode;
  TCHAR symbol;
  switch (popupMagic)
  {
  case (INT_PTR) HBMMENU_POPUP_RESTORE:
    symbol = '2';
    break;
  case (INT_PTR) HBMMENU_POPUP_MINIMIZE:
    symbol = '0';
    break;
  case (INT_PTR) HBMMENU_POPUP_MAXIMIZE:
    symbol = '1';
    break;
  case (INT_PTR) HBMMENU_POPUP_CLOSE:
    symbol = 'r';
    break;
  default:
    ERR("Invalid popup magic bitmap %d\n", (int)popupMagic);
    return;
  }
  ZeroMemory(&lf, sizeof(LOGFONTW));
  InflateRect(r, -2, -2);
  lf.lfHeight = r->bottom - r->top;
  lf.lfWidth = 0;
  lf.lfWeight = FW_NORMAL;
  lf.lfCharSet = DEFAULT_CHARSET;
  lstrcpy(lf.lfFaceName, TEXT("Marlett"));
  hFont = CreateFontIndirect(&lf);
  /* save font and text color */
  hOldFont = SelectObject(dc, hFont);
  clrsave = GetTextColor(dc);
  bkmode = GetBkMode(dc);
  /* set color and drawing mode */
  SetBkMode(dc, TRANSPARENT);
  if (inactive)
  {
    /* draw shadow */
    if (!hilite)
    {
      SetTextColor(dc, GetSysColor(COLOR_HIGHLIGHTTEXT));
      TextOut(dc, r->left + 1, r->top + 1, &symbol, 1);
    }
  }
  SetTextColor(dc, GetSysColor(inactive ? COLOR_GRAYTEXT : (hilite ? COLOR_HIGHLIGHTTEXT : COLOR_MENUTEXT)));
  /* draw selected symbol */
  TextOut(dc, r->left, r->top, &symbol, 1);
  /* restore previous settings */
  SetTextColor(dc, clrsave);
  SelectObject(dc, hOldFont);
  SetBkMode(dc, bkmode);
  DeleteObject(hFont);
}

/***********************************************************************
 *           MenuFindItemByKey
 *
 * Find the menu item selected by a key press.
 * Return item id, -1 if none, -2 if we should close the menu.
 */
static UINT FASTCALL MENU_FindItemByKey(HWND WndOwner, HMENU hmenu,
                  WCHAR Key, BOOL ForceMenuChar)
{
  LRESULT MenuChar;
  WORD Flags = 0;

  TRACE("\tlooking for '%c' (0x%02x) in [%p]\n", (char)Key, Key, hmenu );

  if (!IsMenu( hmenu )) hmenu = GetSubMenu( get_win_sys_menu(WndOwner), 0);
  if (hmenu)
  {
     MENU *menu = MENU_GetMenu( hmenu );
     ITEM *item = menu->rgItems ? DesktopPtrToUser(menu->rgItems) : NULL;

     if ( !ForceMenuChar )
     {
        UINT i;
        BOOL cjk = GetSystemMetrics( SM_DBCSENABLED );

        for (i = 0; i < menu->cItems; i++, item++)
        {
           LPWSTR text = item->Xlpstr ? DesktopPtrToUser(item->Xlpstr) : NULL;
           if( text)
           {
              const WCHAR *p = text - 2;
              do
              {
                 const WCHAR *q = p + 2;
                 p = strchrW (q, '&');
                 if (!p && cjk) p = strchrW (q, '\036'); /* Japanese Win16 */
              }
              while (p != NULL && p [1] == '&');
              if (p && (toupperW(p[1]) == toupperW(Key))) return i;
           }
        }
     }

     Flags |= menu->fFlags & MNF_POPUP ? MF_POPUP : 0;
     Flags |= menu->fFlags & MNF_SYSMENU ? MF_SYSMENU : 0;

     MenuChar = SendMessageW( WndOwner, WM_MENUCHAR,
                              MAKEWPARAM(Key, Flags), (LPARAM) hmenu);
     if (HIWORD(MenuChar) == MNC_EXECUTE) return LOWORD(MenuChar);
     if (HIWORD(MenuChar) == MNC_CLOSE) return (UINT)(-2);
  }
  return (UINT)(-1);
}

/***********************************************************************
 *           MenuGetBitmapItemSize
 *
 * Get the size of a bitmap item.
 */
static void FASTCALL MenuGetBitmapItemSize(PROSMENUITEMINFO lpitem, SIZE *size, HWND WndOwner)
{
    BITMAP bm;
    HBITMAP bmp = lpitem->hbmpItem;

    size->cx = size->cy = 0;

    /* check if there is a magic menu item associated with this item */
    if (IS_MAGIC_BITMAP(bmp))
    {
      switch((INT_PTR) bmp)
        {
            case (INT_PTR)HBMMENU_CALLBACK:
            {
                MEASUREITEMSTRUCT measItem;
                measItem.CtlType = ODT_MENU;
                measItem.CtlID = 0;
                measItem.itemID = lpitem->wID;
                measItem.itemWidth = lpitem->Rect.right - lpitem->Rect.left;
                measItem.itemHeight = lpitem->Rect.bottom - lpitem->Rect.top;
                measItem.itemData = lpitem->dwItemData;
                SendMessageW( WndOwner, WM_MEASUREITEM, lpitem->wID, (LPARAM)&measItem);
                size->cx = measItem.itemWidth;
                size->cy = measItem.itemHeight;
                return;
            }
            break;

          case (INT_PTR) HBMMENU_SYSTEM:
            if (0 != lpitem->dwItemData)
              {
                bmp = (HBITMAP) lpitem->dwItemData;
                break;
              }
            /* fall through */
          case (INT_PTR) HBMMENU_MBAR_RESTORE:
          case (INT_PTR) HBMMENU_MBAR_MINIMIZE:
          case (INT_PTR) HBMMENU_MBAR_CLOSE:
          case (INT_PTR) HBMMENU_MBAR_MINIMIZE_D:
          case (INT_PTR) HBMMENU_MBAR_CLOSE_D:
          case (INT_PTR) HBMMENU_POPUP_CLOSE:
          case (INT_PTR) HBMMENU_POPUP_RESTORE:
          case (INT_PTR) HBMMENU_POPUP_MAXIMIZE:
          case (INT_PTR) HBMMENU_POPUP_MINIMIZE:
            /* FIXME: Why we need to subtract these magic values? */
            /* to make them smaller than the menu bar? */
            size->cx = GetSystemMetrics(SM_CXSIZE) - 2;
            size->cy = GetSystemMetrics(SM_CYSIZE) - 4;
            return;
        }
    }

    if (GetObjectW(bmp, sizeof(BITMAP), &bm))
    {
        size->cx = bm.bmWidth;
        size->cy = bm.bmHeight;
    }
}

/***********************************************************************
 *           MenuDrawBitmapItem
 *
 * Draw a bitmap item.
 */
static void FASTCALL MenuDrawBitmapItem(HDC hdc, PROSMENUITEMINFO lpitem, const RECT *rect,
                    PROSMENUINFO MenuInfo, HWND WndOwner, UINT odaction, BOOL MenuBar)
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
        RECT r;

      r = *rect;
      switch ((INT_PTR)hbmToDraw)
        {
        case (INT_PTR)HBMMENU_SYSTEM:
            if (lpitem->dwTypeData)
            {
                bmp = (HBITMAP)lpitem->dwTypeData;
                if (!GetObjectW( bmp, sizeof(bm), &bm )) return;
            }
            else
            {
                if (!BmpSysMenu) BmpSysMenu = LoadBitmapW(0, MAKEINTRESOURCEW(OBM_CLOSE));
                bmp = BmpSysMenu;
                if (! GetObjectW(bmp, sizeof(bm), &bm)) return;
                /* only use right half of the bitmap */
                bmp_xoffset = bm.bmWidth / 2;
                bm.bmWidth -= bmp_xoffset;
            }
            goto got_bitmap;
        case (INT_PTR)HBMMENU_MBAR_RESTORE:
            flags = DFCS_CAPTIONRESTORE;
            break;
        case (INT_PTR)HBMMENU_MBAR_MINIMIZE:
            r.right += 1;
            flags = DFCS_CAPTIONMIN;
            break;
        case (INT_PTR)HBMMENU_MBAR_MINIMIZE_D:
            r.right += 1;
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
                POINT origorg;
                drawItem.CtlType = ODT_MENU;
                drawItem.CtlID = 0;
                drawItem.itemID = lpitem->wID;
                drawItem.itemAction = odaction;
                drawItem.itemState = (lpitem->fState & MF_CHECKED)?ODS_CHECKED:0;
                drawItem.itemState |= (lpitem->fState & MF_DEFAULT)?ODS_DEFAULT:0;
                drawItem.itemState |= (lpitem->fState & MF_DISABLED)?ODS_DISABLED:0;
                drawItem.itemState |= (lpitem->fState & MF_GRAYED)?ODS_GRAYED|ODS_DISABLED:0;
                drawItem.itemState |= (lpitem->fState & MF_HILITE)?ODS_SELECTED:0;
                //drawItem.itemState |= (!(MenuInfo->fFlags & MNF_UNDERLINE))?ODS_NOACCEL:0;
                //drawItem.itemState |= (MenuInfo->fFlags & MNF_INACTIVE)?ODS_INACTIVE:0;
                drawItem.hwndItem = (HWND)MenuInfo->Self;
                drawItem.hDC = hdc;
                drawItem.rcItem = *rect;
                drawItem.itemData = lpitem->dwItemData;
                /* some applications make this assumption on the DC's origin */
                SetViewportOrgEx( hdc, lpitem->Rect.left, lpitem->Rect.top, &origorg);
                OffsetRect( &drawItem.rcItem, - lpitem->Rect.left, - lpitem->Rect.top);
                SendMessageW( WndOwner, WM_DRAWITEM, 0, (LPARAM)&drawItem);
                SetViewportOrgEx( hdc, origorg.x, origorg.y, NULL);
                return;
            }
            break;

          case (INT_PTR) HBMMENU_POPUP_CLOSE:
          case (INT_PTR) HBMMENU_POPUP_RESTORE:
          case (INT_PTR) HBMMENU_POPUP_MAXIMIZE:
          case (INT_PTR) HBMMENU_POPUP_MINIMIZE:
            MenuDrawPopupGlyph(hdc, &r, (INT_PTR)hbmToDraw, lpitem->fState & MF_GRAYED, lpitem->fState & MF_HILITE);
            return;
        }
      InflateRect(&r, -1, -1);
      if (0 != (lpitem->fState & MF_HILITE))
      {
          flags |= DFCS_PUSHED;
      }
      DrawFrameControl(hdc, &r, DFC_CAPTION, flags);
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
 *           MenuCalcItemSize
 *
 * Calculate the size of the menu item and store it in lpitem->rect.
 */
static void FASTCALL MenuCalcItemSize( HDC hdc, PROSMENUITEMINFO lpitem, PROSMENUINFO MenuInfo, HWND hwndOwner,
                 INT orgX, INT orgY, BOOL menuBar, BOOL textandbmp)
{
    WCHAR *p;
    UINT check_bitmap_width = GetSystemMetrics( SM_CXMENUCHECK );
    UINT arrow_bitmap_width;
    BITMAP bm;
    INT itemheight = 0;

    TRACE("dc=%x owner=%x (%d,%d)\n", hdc, hwndOwner, orgX, orgY);

    GetObjectW( get_arrow_bitmap(), sizeof(bm), &bm );
    arrow_bitmap_width = bm.bmWidth;

    MenuCharSize.cx = GdiGetCharDimensions( hdc, NULL, &MenuCharSize.cy );

    SetRect( &lpitem->Rect, orgX, orgY, orgX, orgY );

    if (lpitem->fType & MF_OWNERDRAW)
    {
        MEASUREITEMSTRUCT mis;
        mis.CtlType    = ODT_MENU;
        mis.CtlID      = 0;
        mis.itemID     = lpitem->wID;
        mis.itemData   = lpitem->dwItemData;
        mis.itemHeight = HIWORD( GetDialogBaseUnits());
        mis.itemWidth  = 0;
        SendMessageW( hwndOwner, WM_MEASUREITEM, 0, (LPARAM)&mis );
        /* Tests reveal that Windows ( Win95 thru WinXP) adds twice the average
         * width of a menufont character to the width of an owner-drawn menu. 
         */
        lpitem->Rect.right += mis.itemWidth + 2 * MenuCharSize.cx;
        if (menuBar) {
            /* under at least win95 you seem to be given a standard
               height for the menu and the height value is ignored */
            lpitem->Rect.bottom += GetSystemMetrics(SM_CYMENUSIZE);
        } else
            lpitem->Rect.bottom += mis.itemHeight;
        // Or this,
        //Item->cxBmp = mis.itemWidth;
        //Item->cyBmp = mis.itemHeight;
        TRACE("id=%04lx size=%dx%d\n",
                lpitem->wID, lpitem->Rect.right-lpitem->Rect.left,
                 lpitem->Rect.bottom-lpitem->Rect.top);
        return;
    }

    if (lpitem->fType & MF_SEPARATOR)
    {
        lpitem->Rect.bottom += GetSystemMetrics( SM_CYMENUSIZE)/2;//SEPARATOR_HEIGHT;
        if( !menuBar)
            lpitem->Rect.right += arrow_bitmap_width/*check_bitmap_width*/ + MenuCharSize.cx;
        return;
    }

    lpitem->dxTab = 0;

    if (lpitem->hbmpItem)
    {
        SIZE size;

        if (!menuBar) {
            MenuGetBitmapItemSize(lpitem, &size, hwndOwner );
            /* Keep the size of the bitmap in callback mode to be able
             * to draw it correctly */
            lpitem->maxBmpSize = size;
            MenuInfo->cxTextAlign = max(MenuInfo->cxTextAlign, size.cx);
            MenuSetRosMenuInfo(MenuInfo);
            lpitem->Rect.right += size.cx + 2;
            itemheight = size.cy + 2;

            if( !(MenuInfo->dwStyle & MNS_NOCHECK))
                lpitem->Rect.right += 2 * check_bitmap_width;
            lpitem->Rect.right += 4 + MenuCharSize.cx;
            lpitem->dxTab = lpitem->Rect.right;
            lpitem->Rect.right += arrow_bitmap_width;//check_bitmap_width;
        } else /* hbmpItem & MenuBar */ {
            MenuGetBitmapItemSize(lpitem, &size, hwndOwner );
            lpitem->Rect.right  += size.cx;
            if( lpitem->lpstr) lpitem->Rect.right  += 2;
            itemheight = size.cy;

            /* Special case: Minimize button doesn't have a space behind it. */
            if (lpitem->hbmpItem == (HBITMAP)HBMMENU_MBAR_MINIMIZE ||
                lpitem->hbmpItem == (HBITMAP)HBMMENU_MBAR_MINIMIZE_D)
            lpitem->Rect.right -= 1;
        }
    }
    else if (!menuBar) {
        if( !(MenuInfo->dwStyle & MNS_NOCHECK))
             lpitem->Rect.right += check_bitmap_width;
        lpitem->Rect.right += 4 + MenuCharSize.cx;
        lpitem->dxTab = lpitem->Rect.right;
        lpitem->Rect.right += check_bitmap_width;
    }

    /* it must be a text item - unless it's the system menu */
    if (!(lpitem->fType & MF_SYSMENU) && lpitem->lpstr) {
        HFONT hfontOld = NULL;
        RECT rc = lpitem->Rect;
        LONG txtheight, txtwidth;

        if ( lpitem->fState & MFS_DEFAULT ) {
            hfontOld = SelectObject( hdc, hMenuFontBold );
        }
        if (menuBar) {
            txtheight = DrawTextW( hdc, lpitem->lpstr, -1, &rc,
                    DT_SINGLELINE|DT_CALCRECT); 
            lpitem->Rect.right  += rc.right - rc.left;
            itemheight = max( max( itemheight, txtheight),
                    GetSystemMetrics( SM_CYMENU) - 1);
            lpitem->Rect.right +=  2 * MenuCharSize.cx;
        } else {
            if ((p = strchrW( lpitem->lpstr, '\t' )) != NULL) {
                RECT tmprc = rc;
                LONG tmpheight;
                int n = (int)( p - lpitem->lpstr);
                /* Item contains a tab (only meaningful in popup menus) */
                /* get text size before the tab */
                txtheight = DrawTextW( hdc, lpitem->lpstr, n, &rc,
                        DT_SINGLELINE|DT_CALCRECT);
                txtwidth = rc.right - rc.left;
                p += 1; /* advance past the Tab */
                /* get text size after the tab */
                tmpheight = DrawTextW( hdc, p, -1, &tmprc,
                        DT_SINGLELINE|DT_CALCRECT);
                lpitem->dxTab += txtwidth;
                txtheight = max( txtheight, tmpheight);
                txtwidth += MenuCharSize.cx + /* space for the tab */
                    tmprc.right - tmprc.left; /* space for the short cut */
            } else {
                txtheight = DrawTextW( hdc, lpitem->lpstr, -1, &rc,
                        DT_SINGLELINE|DT_CALCRECT);
                txtwidth = rc.right - rc.left;
                lpitem->dxTab += txtwidth;
            }
            lpitem->Rect.right  += 2 + txtwidth;
            itemheight = max( itemheight,
				    max( txtheight + 2, MenuCharSize.cy + 4));
        }
        if (hfontOld) SelectObject (hdc, hfontOld);
    } else if( menuBar) {
        itemheight = max( itemheight, GetSystemMetrics(SM_CYMENU)-1);
    }
    lpitem->Rect.bottom += itemheight;
    TRACE("(%ld,%ld)-(%ld,%ld)\n", lpitem->Rect.left, lpitem->Rect.top, lpitem->Rect.right, lpitem->Rect.bottom);
}

/***********************************************************************
 *           MENU_GetMaxPopupHeight
 */
static UINT
MENU_GetMaxPopupHeight(PROSMENUINFO lppop)
{
    if (lppop->cyMax)
    {
       //ERR("MGMaxPH cyMax %d\n",lppop->cyMax);
       return lppop->cyMax;
    }
    //ERR("MGMaxPH SyMax %d\n",GetSystemMetrics(SM_CYSCREEN) - GetSystemMetrics(SM_CYBORDER));
    return GetSystemMetrics(SM_CYSCREEN) - GetSystemMetrics(SM_CYBORDER);
}

/***********************************************************************
 *           MenuPopupMenuCalcSize
 *
 * Calculate the size of a popup menu.
 */
static void FASTCALL MenuPopupMenuCalcSize(PROSMENUINFO MenuInfo, HWND WndOwner)
{
    ROSMENUITEMINFO lpitem;
    HDC hdc;
    int start, i;
    int orgX, orgY, maxX, maxTab, maxTabWidth, maxHeight;
    BOOL textandbmp = FALSE;

    MenuInfo->cxMenu = MenuInfo->cyMenu = 0;
    if (MenuInfo->cItems == 0)
    {
        MenuSetRosMenuInfo(MenuInfo);
        return;
    }

    hdc = GetDC(NULL);
    SelectObject( hdc, hMenuFont );

    start = 0;
    maxX = 2 + 1;

    MenuInfo->cxTextAlign = 0;

    MenuInitRosMenuItemInfo(&lpitem);
    //MenuGetRosMenuItemInfo(MenuInfo->Self, start, &lpitem);
    while (start < MenuInfo->cItems)
    {
      //lpitem = &lppop->items[start];
      orgX = maxX;
      //if( lpitem.fType & (MF_MENUBREAK | MF_MENUBARBREAK))
      //    orgX += MENU_COL_SPACE;
      orgY = MENU_TOP_MARGIN;

      maxTab = maxTabWidth = 0;
      /* Parse items until column break or end of menu */
      for (i = start; i < MenuInfo->cItems; i++)//, lpitem++)
      {
          if (! MenuGetRosMenuItemInfo(MenuInfo->Self, i, &lpitem))
          {
              MenuCleanupRosMenuItemInfo(&lpitem);
              MenuSetRosMenuInfo(MenuInfo);
              return;
          }
          if (i != start &&
               (lpitem.fType & (MF_MENUBREAK | MF_MENUBARBREAK))) break;


          MenuCalcItemSize(hdc, &lpitem, MenuInfo, WndOwner, orgX, orgY, FALSE, textandbmp);
          if (! MenuSetRosMenuItemInfo(MenuInfo->Self, i, &lpitem))
          {
              MenuCleanupRosMenuItemInfo(&lpitem);
              MenuSetRosMenuInfo(MenuInfo);
              return;
          }

          maxX = max(maxX, lpitem.Rect.right);
          orgY = lpitem.Rect.bottom;
          if (IS_STRING_ITEM(lpitem.fType) && lpitem.dxTab )
          {
              maxTab = max( maxTab, lpitem.dxTab );
              maxTabWidth = max(maxTabWidth, lpitem.Rect.right - lpitem.dxTab);
          }
          if( lpitem.lpstr && lpitem.hbmpItem) textandbmp = TRUE;
      }

        /* Finish the column (set all items to the largest width found) */
      maxX = max( maxX, maxTab + maxTabWidth );
      while (start < i)
      {
          if (MenuGetRosMenuItemInfo(MenuInfo->Self, start, &lpitem))
          {
              lpitem.Rect.right = maxX;
              if (IS_STRING_ITEM(lpitem.fType) && lpitem.dxTab)
              {
                  lpitem.dxTab = maxTab;
              }
              MenuSetRosMenuItemInfo(MenuInfo->Self, start, &lpitem);
          }
          start++;
      }
      MenuInfo->cyMenu = max(MenuInfo->cyMenu, orgY);
    }

    MenuInfo->cxMenu  = maxX;
    /* if none of the items have both text and bitmap then
     * the text and bitmaps are all aligned on the left. If there is at
     * least one item with both text and bitmap then bitmaps are
     * on the left and texts left aligned with the right hand side
     * of the bitmaps */
    if( !textandbmp) MenuInfo->cxTextAlign = 0;

    /* space for 3d border */
    MenuInfo->cyMenu += MENU_BOTTOM_MARGIN;
    MenuInfo->cxMenu += 2;

    /* Adjust popup height if it exceeds maximum */
    maxHeight = MENU_GetMaxPopupHeight(MenuInfo);
    MenuInfo->iMaxTop = MenuInfo->cyMenu - MENU_TOP_MARGIN;
    if (MenuInfo->cyMenu >= maxHeight)
    {
       MenuInfo->cyMenu = maxHeight;
       MenuInfo->dwArrowsOn = 1;
    }
    else
    {   
       MenuInfo->dwArrowsOn = 0;
    }

    MenuCleanupRosMenuItemInfo(&lpitem);
    MenuSetRosMenuInfo(MenuInfo);
    ReleaseDC( 0, hdc );
}

/***********************************************************************
 *           MenuMenuBarCalcSize
 *
 * FIXME: Word 6 implements its own MDI and its own 'close window' bitmap
 * height is off by 1 pixel which causes lengthy window relocations when
 * active document window is maximized/restored.
 *
 * Calculate the size of the menu bar.
 */
static void FASTCALL MenuMenuBarCalcSize( HDC hdc, LPRECT lprect,
                                          PROSMENUINFO MenuInfo, HWND hwndOwner )
{
    ROSMENUITEMINFO ItemInfo;
    int start, i, orgX, orgY, maxY, helpPos;

    if ((lprect == NULL) || (MenuInfo == NULL)) return;
    if (MenuInfo->cItems == 0) return;
    TRACE("lprect %p %s\n", lprect, wine_dbgstr_rect( lprect));
    MenuInfo->cxMenu = lprect->right - lprect->left;
    MenuInfo->cyMenu = 0;
    maxY = lprect->top + 1;
    start = 0;
    helpPos = -1;

    MenuInfo->cxTextAlign = 0;

    MenuInitRosMenuItemInfo(&ItemInfo);
    while (start < MenuInfo->cItems)
    {
        if (! MenuGetRosMenuItemInfo(MenuInfo->Self, start, &ItemInfo))
        {
            MenuCleanupRosMenuItemInfo(&ItemInfo);
            return;
        }
        orgX = lprect->left;
        orgY = maxY;

        /* Parse items until line break or end of menu */
        for (i = start; i < MenuInfo->cItems; i++)
	    {
            if ((helpPos == -1) && (ItemInfo.fType & MF_RIGHTJUSTIFY)) helpPos = i;
            if ((i != start) &&
               (ItemInfo.fType & (MF_MENUBREAK | MF_MENUBARBREAK))) break;

            TRACE("calling MENU_CalcItemSize org=(%d, %d)\n", orgX, orgY);
            MenuCalcItemSize(hdc, &ItemInfo, MenuInfo, hwndOwner, orgX, orgY, TRUE, FALSE);
            if (! MenuSetRosMenuItemInfo(MenuInfo->Self, i, &ItemInfo))
            {
                MenuCleanupRosMenuItemInfo(&ItemInfo);
                return;
            }

            if (ItemInfo.Rect.right > lprect->right)
            {
                if (i != start) break;
                else ItemInfo.Rect.right = lprect->right;
            }
            maxY = max( maxY, ItemInfo.Rect.bottom );
            orgX = ItemInfo.Rect.right;
            if (i + 1 < MenuInfo->cItems)
            {
                if (! MenuGetRosMenuItemInfo(MenuInfo->Self, i + 1, &ItemInfo))
                {
                    MenuCleanupRosMenuItemInfo(&ItemInfo);
                    return;
                }
            }
	}

/* FIXME: Is this really needed? */ /*NO! it is not needed, why make the
   HBMMENU_MBAR_CLOSE, MINIMIZE & RESTORE, look the same size as the menu bar! */
#if 0
        /* Finish the line (set all items to the largest height found) */
        while (start < i)
        {
            if (MenuGetRosMenuItemInfo(MenuInfo->Self, start, &ItemInfo))
            {
                ItemInfo.Rect.bottom = maxY;
                MenuSetRosMenuItemInfo(MenuInfo->Self, start, &ItemInfo);
            }
            start++;
        }
#else
        start = i; /* This works! */
#endif
    }

    lprect->bottom = maxY;
    MenuInfo->cyMenu = lprect->bottom - lprect->top;
    MenuSetRosMenuInfo(MenuInfo);

    if (helpPos != -1)
    {
      /* Flush right all items between the MF_RIGHTJUSTIFY and */
      /* the last item (if several lines, only move the last line) */
      if (! MenuGetRosMenuItemInfo(MenuInfo->Self, MenuInfo->cItems - 1, &ItemInfo))
      {
          MenuCleanupRosMenuItemInfo(&ItemInfo);
          return;
      }
      orgY = ItemInfo.Rect.top;
      orgX = lprect->right;
      for (i = MenuInfo->cItems - 1; helpPos <= i; i--)
      {
          if (i < helpPos) break; /* done */
          if (ItemInfo.Rect.top != orgY) break;	  /* Other line */
          if (orgX <= ItemInfo.Rect.right) break; /* Too far right already */
          ItemInfo.Rect.left += orgX - ItemInfo.Rect.right;
          ItemInfo.Rect.right = orgX;
          orgX = ItemInfo.Rect.left;
          MenuSetRosMenuItemInfo(MenuInfo->Self, i, &ItemInfo);
          if (helpPos + 1 <= i &&
              ! MenuGetRosMenuItemInfo(MenuInfo->Self, i - 1, &ItemInfo))
          {
                MenuCleanupRosMenuItemInfo(&ItemInfo);
                return;
          }
      }
    }

    MenuCleanupRosMenuItemInfo(&ItemInfo);
}

/***********************************************************************
 *           MENU_DrawScrollArrows
 *
 * Draw scroll arrows.
 */
static void
MENU_DrawScrollArrows(PROSMENUINFO lppop, HDC hdc)
{
    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hOrigBitmap;
    UINT arrow_bitmap_width, arrow_bitmap_height;
    BITMAP bmp;
    RECT rect;

    GetObjectW(get_down_arrow_bitmap(), sizeof(bmp), &bmp);
    arrow_bitmap_width = bmp.bmWidth;
    arrow_bitmap_height = bmp.bmHeight;

    if (lppop->iTop)
        hOrigBitmap = SelectObject(hdcMem, get_up_arrow_bitmap());
    else
        hOrigBitmap = SelectObject(hdcMem, get_up_arrow_inactive_bitmap());
    rect.left = 0;
    rect.top = 0;
    rect.right = lppop->cxMenu;
    rect.bottom = arrow_bitmap_height;
    FillRect(hdc, &rect, GetSysColorBrush(COLOR_MENU));
    BitBlt(hdc, (lppop->cxMenu - arrow_bitmap_width) / 2, 0,
           arrow_bitmap_width, arrow_bitmap_height, hdcMem, 0, 0, SRCCOPY);
    rect.top = lppop->cyMenu - arrow_bitmap_height;
    rect.bottom = lppop->cyMenu;
    FillRect(hdc, &rect, GetSysColorBrush(COLOR_MENU));
    if (lppop->iTop < lppop->iMaxTop - (MENU_GetMaxPopupHeight(lppop) - 2 * arrow_bitmap_height))
        SelectObject(hdcMem, get_down_arrow_bitmap());
    else
        SelectObject(hdcMem, get_down_arrow_inactive_bitmap());
    BitBlt(hdc, (lppop->cxMenu - arrow_bitmap_width) / 2,
           lppop->cyMenu - arrow_bitmap_height,
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
 *           MenuDrawMenuItem
 *
 * Draw a single menu item.
 */
static void FASTCALL MenuDrawMenuItem(HWND hWnd, PROSMENUINFO MenuInfo, HWND WndOwner, HDC hdc,
                 PROSMENUITEMINFO lpitem, UINT Height, BOOL menuBar, UINT odaction)
{
    RECT rect;
    PWCHAR Text;
    BOOL flat_menu = FALSE;
    int bkgnd;
    UINT arrow_bitmap_width = 0, arrow_bitmap_height = 0;
    PWND Wnd = ValidateHwndNoErr(hWnd);

    if (!Wnd)
      return;

    if (!menuBar) {
        BITMAP bmp;
        GetObjectW( get_arrow_bitmap(), sizeof(bmp), &bmp );
        arrow_bitmap_width = bmp.bmWidth;
        arrow_bitmap_height = bmp.bmHeight;
    }

    if (lpitem->fType & MF_SYSMENU)
    {
        if ( (Wnd->style & WS_MINIMIZE))
        {
          UserGetInsideRectNC(Wnd, &rect);
          UserDrawSysMenuButton(hWnd, hdc, &rect, lpitem->fState & (MF_HILITE | MF_MOUSESELECT));
	}
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
            if (lpitem->fState & MF_GRAYED)
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

    TRACE("rect=%s\n", wine_dbgstr_rect( &lpitem->Rect));
    rect = lpitem->Rect;
    MENU_AdjustMenuItemRect(MenuInfo, &rect);

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
        dis.itemData  = (DWORD)lpitem->dwItemData;
        dis.itemState = 0;
        if (lpitem->fState & MF_CHECKED)  dis.itemState |= ODS_CHECKED;
        if (lpitem->fState & MF_DEFAULT)  dis.itemState |= ODS_DEFAULT;
        if (lpitem->fState & MF_DISABLED) dis.itemState |= ODS_DISABLED;
        if (lpitem->fState & MF_GRAYED)   dis.itemState |= ODS_GRAYED | ODS_DISABLED;
        if (lpitem->fState & MF_HILITE)   dis.itemState |= ODS_SELECTED;
        //if (!(MenuInfo->fFlags & MNF_UNDERLINE)) dis.itemState |= ODS_NOACCEL;
        //if (MenuInfo->fFlags & MNF_INACTIVE) dis.itemState |= ODS_INACTIVE;
        dis.itemAction = odaction; /* ODA_DRAWENTIRE | ODA_SELECT | ODA_FOCUS; */
        dis.hwndItem   = (HWND) MenuInfo->Self;
        dis.hDC        = hdc;
        dis.rcItem     = rect;
        TRACE("Ownerdraw: owner=%p itemID=%d, itemState=%d, itemAction=%d, "
	        "hwndItem=%p, hdc=%p, rcItem={%ld,%ld,%ld,%ld}\n", hWnd,
	        dis.itemID, dis.itemState, dis.itemAction, dis.hwndItem,
	        dis.hDC, dis.rcItem.left, dis.rcItem.top, dis.rcItem.right,
	        dis.rcItem.bottom);
        SendMessageW(WndOwner, WM_DRAWITEM, 0, (LPARAM) &dis);
        /* Draw the popup-menu arrow */
        if (lpitem->hSubMenu)
        {
           /* RECT rectTemp;
            CopyRect(&rectTemp, &rect);
            rectTemp.left = rectTemp.right - GetSystemMetrics(SM_CXMENUCHECK);
            DrawFrameControl(hdc, &rectTemp, DFC_MENU, DFCS_MENUARROW);
            */
            draw_popup_arrow( hdc, rect, arrow_bitmap_width, arrow_bitmap_height);
        }
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

        rc.left -= 3;//MENU_COL_SPACE / 2 + 1;
        rc.top = 3;
        rc.bottom = Height - 3;
        if (flat_menu)
        {
            oldPen = SelectObject( hdc, GetStockObject(DC_PEN) );
            SetDCPenColor(hdc, GetSysColor(COLOR_BTNSHADOW));
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
        rc.top += SEPARATOR_HEIGHT / 2;
        if (flat_menu)
        {
            oldPen = SelectObject( hdc, GetStockObject(DC_PEN) );
            SetDCPenColor( hdc, GetSysColor(COLOR_BTNSHADOW));
            MoveToEx( hdc, rc.left, rc.top, NULL );
            LineTo( hdc, rc.right, rc.top );
            SelectObject( hdc, oldPen );
        }
        else
            DrawEdge (hdc, &rc, EDGE_ETCHED, BF_TOP);
        return;
    }

#if 0
    /* helper lines for debugging */
    /* This is a very good test tool when hacking menus! (JT) 07/16/2006 */
    FrameRect(hdc, &rect, GetStockObject(BLACK_BRUSH));
    SelectObject(hdc, GetStockObject(DC_PEN));
    SetDCPenColor(hdc, GetSysColor(COLOR_WINDOWFRAME));
    MoveToEx(hdc, rect.left, (rect.top + rect.bottom) / 2, NULL);
    LineTo(hdc, rect.right, (rect.top + rect.bottom) / 2);
#endif

    if (!menuBar)
    {
        HBITMAP bm;
        INT y = rect.top + rect.bottom;
        RECT rc = rect;
        BOOL checked = FALSE;
        UINT check_bitmap_width = GetSystemMetrics( SM_CXMENUCHECK );
        UINT check_bitmap_height = GetSystemMetrics( SM_CYMENUCHECK );
        /* Draw the check mark
         *
         * FIXME:
         * Custom checkmark bitmaps are monochrome but not always 1bpp.
         */
        if( !(MenuInfo->dwStyle & MNS_NOCHECK)) {
            bm = (lpitem->fState & MF_CHECKED) ? lpitem->hbmpChecked : 
                lpitem->hbmpUnchecked;
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
                //HBITMAP bm = CreateBitmap( check_bitmap_width, check_bitmap_height, 1, 1, NULL );
                //HDC hdcMem = CreateCompatibleDC( hdc );
                //SelectObject( hdcMem, bm );
                //SetRect( &r, 0, 0, check_bitmap_width, check_bitmap_height);
                CopyRect(&r, &rect);
                r.right = r.left + GetSystemMetrics(SM_CXMENUCHECK);
                DrawFrameControl( hdc, &r, DFC_MENU,
                                 (lpitem->fType & MFT_RADIOCHECK) ?
                                 DFCS_MENUBULLET : DFCS_MENUCHECK);
                //BitBlt( hdc, rc.left, (y - r.bottom) / 2, r.right, r.bottom, hdcMem, 0, 0, SRCCOPY );
                //DeleteDC( hdcMem );
                //DeleteObject( bm );
                checked = TRUE;
            }
        }
        if ( lpitem->hbmpItem )//&& !( checked && (MenuInfo->dwStyle & MNS_CHECKORBMP)))
        {
            RECT bmpRect;
            CopyRect(&bmpRect, &rect);
            if (!(MenuInfo->dwStyle & MNS_CHECKORBMP) && !(MenuInfo->dwStyle & MNS_NOCHECK))
                bmpRect.left += check_bitmap_width + 2;
            if (!(checked && (MenuInfo->dwStyle & MNS_CHECKORBMP)))
            {
                //POINT origorg;
                bmpRect.right = bmpRect.left + lpitem->maxBmpSize.cx;
                /* some applications make this assumption on the DC's origin */
                //SetViewportOrgEx( hdc, rect.left, rect.top, &origorg);
                MenuDrawBitmapItem(hdc, lpitem, &bmpRect, MenuInfo, WndOwner, odaction, menuBar);
                //SetViewportOrgEx( hdc, origorg.x, origorg.y, NULL);
            }
        }
        /* Draw the popup-menu arrow */
        if (lpitem->hSubMenu)
        {
           /* RECT rectTemp;
            CopyRect(&rectTemp, &rect);
            rectTemp.left = rectTemp.right - GetSystemMetrics(SM_CXMENUCHECK);
            DrawFrameControl(hdc, &rectTemp, DFC_MENU, DFCS_MENUARROW);
            */
            draw_popup_arrow( hdc, rect, arrow_bitmap_width, arrow_bitmap_height);
        }
        rect.left += 4;
        if( !(MenuInfo->dwStyle & MNS_NOCHECK))
            rect.left += check_bitmap_width;
        rect.right -= arrow_bitmap_width;//check_bitmap_width;
    }
    else if( lpitem->hbmpItem)
    { /* Draw the bitmap */
        //POINT origorg;

        //SetViewportOrgEx( hdc, rect.left, rect.top, &origorg);
        MenuDrawBitmapItem(hdc, lpitem, &rect, MenuInfo, WndOwner, odaction, menuBar);
        //SetViewportOrgEx( hdc, origorg.x, origorg.y, NULL);
    }

    /* process text if present */
    if (lpitem->lpstr)
    {
        int i = 0;
        HFONT hfontOld = 0;

        UINT uFormat = menuBar ?
                       DT_CENTER | DT_VCENTER | DT_SINGLELINE :
                       DT_LEFT | DT_VCENTER | DT_SINGLELINE;

        if ((MenuInfo->dwStyle & MNS_CHECKORBMP))
             rect.left += max(0, (int)(MenuInfo->cxTextAlign - GetSystemMetrics(SM_CXMENUCHECK)));
        else
             rect.left += MenuInfo->cxTextAlign;

        if ( lpitem->fState & MFS_DEFAULT )
        {
            hfontOld = SelectObject(hdc, hMenuFontBold);
        }

        if (menuBar) {
            if( lpitem->hbmpItem)
              rect.left += lpitem->maxBmpSize.cx;
            if( !(lpitem->hbmpItem == HBMMENU_CALLBACK)) 
              rect.left += MenuCharSize.cx;
            rect.right -= MenuCharSize.cx;
            //rect.left += MENU_BAR_ITEMS_SPACE / 2;
            //rect.right -= MENU_BAR_ITEMS_SPACE / 2;
        }

        Text = lpitem->lpstr;
        if(Text)
        {
            for (i = 0; L'\0' != Text[i]; i++)
                if (Text[i] == L'\t' || Text[i] == L'\b')
                    break;
        }

        if(lpitem->fState & MF_GRAYED)
        {
            if (!(lpitem->fState & MF_HILITE) )
            {
                ++rect.left; ++rect.top; ++rect.right; ++rect.bottom;
                SetTextColor(hdc, RGB(0xff, 0xff, 0xff));
                DrawTextW( hdc, Text, i, &rect, uFormat );
                --rect.left; --rect.top; --rect.right; --rect.bottom;
	        }
            SetTextColor(hdc, RGB(0x80, 0x80, 0x80));
        }

        DrawTextW( hdc, Text, i, &rect, uFormat);

        /* paint the shortcut text */
        if (!menuBar && L'\0' != Text[i])  /* There's a tab or flush-right char */
        {
            if (L'\t' == Text[i])
            {
                rect.left = lpitem->dxTab;
                uFormat = DT_LEFT | DT_VCENTER | DT_SINGLELINE;
            }
            else
            {
                rect.right = lpitem->dxTab;
                uFormat = DT_RIGHT | DT_VCENTER | DT_SINGLELINE;
            }

            if (lpitem->fState & MF_GRAYED)
            {
                if (!(lpitem->fState & MF_HILITE) )
                {
                    ++rect.left; ++rect.top; ++rect.right; ++rect.bottom;
                    SetTextColor(hdc, RGB(0xff, 0xff, 0xff));
                    DrawTextW( hdc, Text + i + 1, -1, &rect, uFormat);
                    --rect.left; --rect.top; --rect.right; --rect.bottom;
                }
                SetTextColor(hdc, RGB(0x80, 0x80, 0x80));
            }
            DrawTextW( hdc, Text + i + 1, -1, &rect, uFormat );
        }

        if (hfontOld)
          SelectObject (hdc, hfontOld);
    }
}

/***********************************************************************
 *           MenuDrawPopupMenu
 *
 * Paint a popup menu.
 */
static void FASTCALL MenuDrawPopupMenu(HWND hwnd, HDC hdc, HMENU hmenu )
{
    HBRUSH hPrevBrush = 0;
    RECT rect;

    TRACE("wnd=%p dc=%p menu=%p\n", hwnd, hdc, hmenu);

    GetClientRect( hwnd, &rect );

    if((hPrevBrush = SelectObject( hdc, GetSysColorBrush(COLOR_MENU) ))
        && (SelectObject( hdc, hMenuFont)))
    {
        HPEN hPrevPen;

        Rectangle( hdc, rect.left, rect.top, rect.right, rect.bottom );

        hPrevPen = SelectObject( hdc, GetStockObject( NULL_PEN ) );
        if ( hPrevPen )
        {
            BOOL flat_menu = FALSE;
            ROSMENUINFO MenuInfo;
            ROSMENUITEMINFO ItemInfo;

            SystemParametersInfoW (SPI_GETFLATMENU, 0, &flat_menu, 0);
            if (flat_menu)
               FrameRect(hdc, &rect, GetSysColorBrush(COLOR_BTNSHADOW));
            else
               DrawEdge (hdc, &rect, EDGE_RAISED, BF_RECT);

            //TRACE("hmenu %p Style %08x\n", hmenu, menu->dwStyle);
            /* draw menu items */
            if (MenuGetRosMenuInfo(&MenuInfo, hmenu) && MenuInfo.cItems)
            {
                UINT u;
                MenuInitRosMenuItemInfo(&ItemInfo);

                //for (u = MenuInfo.cItems; u > 0; u--, item++)
                for (u = 0; u < MenuInfo.cItems; u++)
                {
                    if (MenuGetRosMenuItemInfo(MenuInfo.Self, u, &ItemInfo))
                    {
                        HWND WndOwner = MenuInfo.spwndNotify ? MenuInfo.spwndNotify->head.h : NULL;
                        MenuDrawMenuItem(hwnd, &MenuInfo, WndOwner, hdc, &ItemInfo,
                                         MenuInfo.cyMenu, FALSE, ODA_DRAWENTIRE);
                    }
                }
                /* draw scroll arrows */
                if (MenuInfo.dwArrowsOn)
                {
                   MENU_DrawScrollArrows(&MenuInfo, hdc);
                }

                MenuSetRosMenuInfo(&MenuInfo);
                MenuCleanupRosMenuItemInfo(&ItemInfo);
            }
         } else
         {
             SelectObject( hdc, hPrevBrush );
         }
    }
}

/***********************************************************************
 *           MenuDrawMenuBar
 *
 * Paint a menu bar. Returns the height of the menu bar.
 * called from [windows/nonclient.c]
 */
UINT MenuDrawMenuBar( HDC hDC, LPRECT lprect, HWND hwnd,
                         BOOL suppress_draw)
{
    ROSMENUINFO lppop;
    HFONT hfontOld = 0;
    HMENU hMenu = GetMenu(hwnd);

    if (! MenuGetRosMenuInfo(&lppop, hMenu) || lprect == NULL)
    {
        return GetSystemMetrics(SM_CYMENU);
    }

    if (suppress_draw)
    {
        hfontOld = SelectObject(hDC, hMenuFont);

        MenuMenuBarCalcSize(hDC, lprect, &lppop, hwnd);

        lprect->bottom = lprect->top + lppop.cyMenu;

        if (hfontOld) SelectObject( hDC, hfontOld);
        return lppop.cyMenu;
    }
    else
        return DrawMenuBarTemp(hwnd, hDC, lprect, hMenu, NULL);
}

/***********************************************************************
 *           MENU_InitPopup
 *   
 * Popup menu initialization before WM_ENTERMENULOOP.
 */
static BOOL MENU_InitPopup( HWND hwndOwner, HMENU hmenu, UINT flags )
{
    MENU *menu;
    DWORD ex_style = WS_EX_TOOLWINDOW;
    ROSMENUINFO MenuInfo;

    TRACE("owner=%p hmenu=%p\n", hwndOwner, hmenu);

    if (!(menu = MENU_GetMenu( hmenu ))) return FALSE;

    /* store the owner for DrawItem */
    if (!IsWindow( hwndOwner ))
    {
        SetLastError( ERROR_INVALID_WINDOW_HANDLE );
        return FALSE;
    }
    MenuGetRosMenuInfo(&MenuInfo, menu->head.h);
    //menu->hwndOwner = hwndOwner;
    MenuInfo.spwndNotify = ValidateHwndNoErr( hwndOwner );

    if (flags & TPM_LAYOUTRTL)
        ex_style = WS_EX_LAYOUTRTL;

    /* NOTE: In Windows, top menu popup is not owned. */
    //menu->hWnd = CreateWindowExW( ex_style, WC_MENU, NULL,
    MenuInfo.Wnd = CreateWindowExW( ex_style, WC_MENU, NULL,
                                WS_POPUP, 0, 0, 0, 0,
                                hwndOwner, 0, (HINSTANCE)GetWindowLongPtrW(hwndOwner, GWLP_HINSTANCE),
                                (LPVOID)hmenu );
    MenuSetRosMenuInfo(&MenuInfo);
    if( !menu->hWnd ) return FALSE;
    return TRUE;
}

/***********************************************************************
 *           MenuShowPopup
 *
 * Display a popup menu.
 */
static BOOL FASTCALL MenuShowPopup(HWND hwndOwner, HMENU hmenu, UINT id, UINT flags,
                              INT x, INT y, INT xanchor, INT yanchor )
{
    ROSMENUINFO MenuInfo;
    ROSMENUITEMINFO ItemInfo;
    UINT width, height;
    POINT pt;
    HMONITOR monitor;
    MONITORINFO info;

    TRACE("owner=%p hmenu=%p id=0x%04x x=0x%04x y=0x%04x xa=0x%04x ya=0x%04x\n",
          hwndOwner, hmenu, id, x, y, xanchor, yanchor);

    if (! MenuGetRosMenuInfo(&MenuInfo, hmenu)) return FALSE;
    if (MenuInfo.iItem != NO_SELECTED_ITEM)
    {
        MenuInitRosMenuItemInfo(&ItemInfo);
        if (MenuGetRosMenuItemInfo(MenuInfo.Self, MenuInfo.iItem, &ItemInfo))
        {
            ItemInfo.fMask |= MIIM_STATE;
            ItemInfo.fState &= ~(MF_HILITE|MF_MOUSESELECT);
            MenuSetRosMenuItemInfo(MenuInfo.Self, MenuInfo.iItem, &ItemInfo);
        }
        MenuCleanupRosMenuItemInfo(&ItemInfo);
        MenuInfo.iItem = NO_SELECTED_ITEM;
    }

    //menu->dwArrowsOn = 0;
    MenuInfo.dwArrowsOn = 0;
    MenuSetRosMenuInfo(&MenuInfo);
    MenuPopupMenuCalcSize(&MenuInfo, hwndOwner);

    /* adjust popup menu pos so that it fits within the desktop */

    width = MenuInfo.cxMenu + GetSystemMetrics(SM_CXBORDER);
    height = MenuInfo.cyMenu + GetSystemMetrics(SM_CYBORDER);

    /* FIXME: should use item rect */
    pt.x = x;
    pt.y = y;
    monitor = MonitorFromPoint( pt, MONITOR_DEFAULTTONEAREST );
    info.cbSize = sizeof(info);
    GetMonitorInfoW( monitor, &info );

    if (flags & TPM_LAYOUTRTL) 
        flags ^= TPM_RIGHTALIGN;

    if( flags & TPM_RIGHTALIGN ) x -= width;
    if( flags & TPM_CENTERALIGN ) x -= width / 2;

    if( flags & TPM_BOTTOMALIGN ) y -= height;
    if( flags & TPM_VCENTERALIGN ) y -= height / 2;

    if( x + width > info.rcMonitor.right)
    {
        if( xanchor && x >= width - xanchor )
            x -= width - xanchor;

        if( x + width > info.rcMonitor.right)
            x = info.rcMonitor.right - width;
    }
    if( x < info.rcMonitor.left ) x = info.rcMonitor.left;

    if( y + height > info.rcMonitor.bottom)
    {
        if( yanchor && y >= height + yanchor )
            y -= height + yanchor;

        if( y + height > info.rcMonitor.bottom)
            y = info.rcMonitor.bottom - height;
    }
    if( y < info.rcMonitor.top ) y = info.rcMonitor.top;

    if (!top_popup) {
        top_popup = MenuInfo.Wnd;
        top_popup_hmenu = hmenu;
    }
    /* Display the window */

    SetWindowPos( MenuInfo.Wnd, HWND_TOPMOST, x, y, width, height,
                  SWP_SHOWWINDOW | SWP_NOACTIVATE);
    UpdateWindow( MenuInfo.Wnd );

    IntNotifyWinEvent(EVENT_SYSTEM_MENUPOPUPSTART, MenuInfo.Wnd, OBJID_CLIENT, CHILDID_SELF, 0);

    return TRUE;
}

/***********************************************************************
 *           MENU_EnsureMenuItemVisible
 */
void
MENU_EnsureMenuItemVisible(PROSMENUINFO lppop, PROSMENUITEMINFO item, HDC hdc)
{
    if (lppop->dwArrowsOn)
    {
        //ITEM *item = &lppop->items[wIndex];
        UINT nMaxHeight = MENU_GetMaxPopupHeight(lppop);
        UINT nOldPos = lppop->iTop;
        RECT rc;
        UINT arrow_bitmap_height;
        BITMAP bmp;
        
        GetClientRect(lppop->Wnd, &rc);

        GetObjectW(get_down_arrow_bitmap(), sizeof(bmp), &bmp);
        arrow_bitmap_height = bmp.bmHeight;

        rc.top += arrow_bitmap_height;
        rc.bottom -= arrow_bitmap_height + MENU_BOTTOM_MARGIN;
       
        nMaxHeight -= GetSystemMetrics(SM_CYBORDER) + 2 * arrow_bitmap_height;
        if (item->Rect.bottom > lppop->iTop + nMaxHeight)
        {
            lppop->iTop = item->Rect.bottom - nMaxHeight;
            ScrollWindow(lppop->Wnd, 0, nOldPos - lppop->iTop, &rc, &rc);
            MENU_DrawScrollArrows(lppop, hdc);
            //ERR("Scroll Down iTop %d iMaxTop %d nMaxHeight %d\n",lppop->iTop,lppop->iMaxTop,nMaxHeight);
        }
        else if (item->Rect.top - MENU_TOP_MARGIN < lppop->iTop)
        {
            lppop->iTop = item->Rect.top - MENU_TOP_MARGIN;
            ScrollWindow(lppop->Wnd, 0, nOldPos - lppop->iTop, &rc, &rc);
            MENU_DrawScrollArrows(lppop, hdc);
            //ERR("Scroll Up   iTop %d iMaxTop %d nMaxHeight %d\n",lppop->iTop,lppop->iMaxTop,nMaxHeight);
        }
        MenuSetRosMenuInfo(lppop);
    }
}

/***********************************************************************
 *           MenuSelectItem
 */
static void FASTCALL MenuSelectItem(HWND hwndOwner, PROSMENUINFO hmenu, UINT wIndex,
                                    BOOL sendMenuSelect, HMENU topmenu)
{
    ROSMENUITEMINFO ItemInfo;
    ROSMENUINFO TopMenuInfo;
    HDC hdc;

    TRACE("owner=%p menu=%p index=0x%04x select=0x%04x\n", hwndOwner, hmenu, wIndex, sendMenuSelect);

    if (!hmenu || !hmenu->cItems || !hmenu->Wnd) return;
    if (hmenu->iItem == wIndex) return;
    if (hmenu->fFlags & MNF_POPUP) hdc = GetDC(hmenu->Wnd);
    else hdc = GetDCEx(hmenu->Wnd, 0, DCX_CACHE | DCX_WINDOW);
    if (!top_popup) {
        top_popup = hmenu->Wnd;
        top_popup_hmenu = hmenu->Self;
    }

    SelectObject( hdc, hMenuFont );

    MenuInitRosMenuItemInfo(&ItemInfo);

     /* Clear previous highlighted item */
    if (hmenu->iItem != NO_SELECTED_ITEM)
    {
        if (MenuGetRosMenuItemInfo(hmenu->Self, hmenu->iItem, &ItemInfo))
        {
            ItemInfo.fMask |= MIIM_STATE;
            ItemInfo.fState &= ~(MF_HILITE|MF_MOUSESELECT);
            MenuSetRosMenuItemInfo(hmenu->Self, hmenu->iItem, &ItemInfo);
        }
        MenuDrawMenuItem(hmenu->Wnd, hmenu, hwndOwner, hdc, &ItemInfo,
                       hmenu->cyMenu, !(hmenu->fFlags & MNF_POPUP),
                       ODA_SELECT);
    }

    /* Highlight new item (if any) */
    hmenu->iItem = wIndex;
    MenuSetRosMenuInfo(hmenu);
    if (hmenu->iItem != NO_SELECTED_ITEM)
    {
        if (MenuGetRosMenuItemInfo(hmenu->Self, hmenu->iItem, &ItemInfo))
        {
            if (!(ItemInfo.fType & MF_SEPARATOR))
            {
                ItemInfo.fMask |= MIIM_STATE;
                ItemInfo.fState |= MF_HILITE;
                MenuSetRosMenuItemInfo(hmenu->Self, hmenu->iItem, &ItemInfo);
                MENU_EnsureMenuItemVisible(hmenu, &ItemInfo, hdc);
                MenuDrawMenuItem(hmenu->Wnd, hmenu, hwndOwner, hdc,
                               &ItemInfo, hmenu->cyMenu, !(hmenu->fFlags & MNF_POPUP), ODA_SELECT);
            }
            if (sendMenuSelect)
            {
                WPARAM wParam = MAKEWPARAM( ItemInfo.hSubMenu ? wIndex : ItemInfo.wID, 
                                            ItemInfo.fType | ItemInfo.fState |
                                           (ItemInfo.hSubMenu ? MF_POPUP : 0) |
                                           (hmenu->fFlags & MNF_SYSMENU ? MF_SYSMENU : 0 ) );

                SendMessageW(hwndOwner, WM_MENUSELECT, wParam, (LPARAM) hmenu->Self);
            }
        }
    }
    else if (sendMenuSelect) 
    {
        if(topmenu) 
        {
            int pos;
            pos = MenuFindSubMenu(&topmenu, hmenu->Self);
            if (pos != NO_SELECTED_ITEM)
            {
                if (MenuGetRosMenuInfo(&TopMenuInfo, topmenu)
                    && MenuGetRosMenuItemInfo(topmenu, pos, &ItemInfo))
                {
                    WPARAM wParam = MAKEWPARAM( Pos, ItemInfo.fType | ItemInfo.fState |
                                               (ItemInfo.hSubMenu ? MF_POPUP : 0) |
                                               (TopMenuInfo.fFlags & MNF_SYSMENU ? MF_SYSMENU : 0 ) );

                    SendMessageW(hwndOwner, WM_MENUSELECT, wParam, (LPARAM) topmenu);
                }
            }
        }
    }
    MenuCleanupRosMenuItemInfo(&ItemInfo);
    ReleaseDC(hmenu->Wnd, hdc);
}

/***********************************************************************
 *           MenuMoveSelection
 *
 * Moves currently selected item according to the Offset parameter.
 * If there is no selection then it should select the last item if
 * Offset is ITEM_PREV or the first item if Offset is ITEM_NEXT.
 */
static void FASTCALL
MenuMoveSelection(HWND WndOwner, PROSMENUINFO MenuInfo, INT Offset)
{
  INT i;
  ROSMENUITEMINFO ItemInfo;
  INT OrigPos;

  TRACE("hwnd=%x menu=%x off=0x%04x\n", WndOwner, MenuInfo, Offset);

  /* Prevent looping */
  if (0 == MenuInfo->cItems || 0 == Offset)
    return;
  else if (Offset < -1)
    Offset = -1;
  else if (Offset > 1)
    Offset = 1;

  MenuInitRosMenuItemInfo(&ItemInfo);

  OrigPos = MenuInfo->iItem;
  if (OrigPos == NO_SELECTED_ITEM) /* NO_SELECTED_ITEM is not -1 ! */
    {
       OrigPos = 0;
       i = -1;
    }
  else
    {
      i = MenuInfo->iItem;
    }

  do
    {
      /* Step */
      i += Offset;
      /* Clip and wrap around */
      if (i < 0)
        {
          i = MenuInfo->cItems - 1;
        }
      else if (i >= MenuInfo->cItems)
        {
          i = 0;
        }
      /* If this is a good candidate; */
      if (MenuGetRosMenuItemInfo(MenuInfo->Self, i, &ItemInfo) &&
          0 == (ItemInfo.fType & MF_SEPARATOR))
        {
          MenuSelectItem(WndOwner, MenuInfo, i, TRUE, NULL);
          MenuCleanupRosMenuItemInfo(&ItemInfo);
          return;
        }
    } while (i != OrigPos);

  /* Not found */
  MenuCleanupRosMenuItemInfo(&ItemInfo);
}

#if 0
LRESULT WINAPI
PopupMenuWndProcW(HWND Wnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
#ifdef __REACTOS__ // Do this now, remove after Server side is fixed.
  PWND pWnd;
  PPOPUPMENU pPopupMenu;

  pWnd = ValidateHwndNoErr(Wnd);
  if (pWnd)
  {
     if (!pWnd->fnid)
     {
        if (Message != WM_NCCREATE)
        {
           return DefWindowProcW(Wnd, Message, wParam, lParam);
        }
        NtUserSetWindowFNID(Wnd, FNID_MENU);
        pPopupMenu = HeapAlloc( GetProcessHeap(), 0, sizeof(POPUPMENU) );
        pPopupMenu->spwndPopupMenu = pWnd;
        SetWindowLongPtrW(Wnd, 0, (LONG_PTR)pPopupMenu);
     }
     else
     {
        if (pWnd->fnid != FNID_MENU)
        {
           ERR("Wrong window class for Menu!\n");
           return 0;
        }
        pPopupMenu = ((PMENUWND)pWnd)->ppopupmenu;
     }
  }
#endif    

  TRACE("hwnd=%x msg=0x%04x wp=0x%04lx lp=0x%08lx\n", Wnd, Message, wParam, lParam);

  switch(Message)
    {
    case WM_CREATE:
      {
        CREATESTRUCTW *cs = (CREATESTRUCTW *) lParam;
        pPopupMenu->spmenu = ValidateHandle(cs->lpCreateParams, TYPE_MENU);
        return 0;
      }

    case WM_MOUSEACTIVATE:  /* We don't want to be activated */
      return MA_NOACTIVATE;

    case WM_PAINT:
      {
        PAINTSTRUCT ps;
        BeginPaint(Wnd, &ps);
        MenuDrawPopupMenu(Wnd, ps.hdc, pPopupMenu->spmenu->head.h);
        EndPaint(Wnd, &ps);
        return 0;
      }

    case WM_PRINTCLIENT:
      {
         MenuDrawPopupMenu( Wnd, (HDC)wParam, pPopupMenu->spmenu->head.h);
         return 0;
      }

    case WM_ERASEBKGND:
      return 1;

    case WM_DESTROY:
      /* zero out global pointer in case resident popup window was destroyed. */
      if (Wnd == top_popup)
        {
          top_popup = NULL;
          top_popup_hmenu = NULL;
        }
      break;

    case WM_NCDESTROY:
      {
      HeapFree( GetProcessHeap(), 0, pPopupMenu );
      SetWindowLongPtrW(Wnd, 0, 0);
      NtUserSetWindowFNID(Wnd, FNID_DESTROY);
      break;
      }

    case WM_SHOWWINDOW:
      if (0 != wParam)
      {
          if (!pPopupMenu || !pPopupMenu->spmenu)
          {
              OutputDebugStringA("no menu to display\n");
          }
      }
      /*else
      {
          pPopupMenu->spmenu = NULL; ///// WTF?
      }*/
      break;

    case MM_SETMENUHANDLE:
      {
        PMENU pmenu = ValidateHandle((HMENU)wParam, TYPE_MENU);
        if (!pmenu)
        {
           ERR("Bad Menu Handle\n");
           break;
        }
        pPopupMenu->spmenu = pmenu;
        break;
      }

    case MM_GETMENUHANDLE:
    case MN_GETHMENU:
         return (LRESULT)(pPopupMenu ? (pPopupMenu->spmenu ? pPopupMenu->spmenu->head.h : NULL) : NULL);

    default:
      return DefWindowProcW(Wnd, Message, wParam, lParam);
    }

  return 0;
}
#endif

LRESULT WINAPI
PopupMenuWndProcW(HWND Wnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
#ifdef __REACTOS__ // Do this now, remove after Server side is fixed.
  PWND pWnd;

  pWnd = ValidateHwnd(Wnd);
  if (pWnd)
  {
     if (!pWnd->fnid)
     {
        if (Message != WM_NCCREATE)
        {
           return DefWindowProcW(Wnd, Message, wParam, lParam);
        }
        NtUserSetWindowFNID(Wnd, FNID_MENU);
     }
     else
     {
        if (pWnd->fnid != FNID_MENU)
        {
           ERR("Wrong window class for Menu!\n");
           return 0;
        }
     }
  }
#endif    

  TRACE("hwnd=%x msg=0x%04x wp=0x%04lx lp=0x%08lx\n", Wnd, Message, wParam, lParam);

  switch(Message)
    {
    case WM_CREATE:
      {
        CREATESTRUCTW *cs = (CREATESTRUCTW *) lParam;
        SetWindowLongPtrW(Wnd, 0, (LONG_PTR)cs->lpCreateParams);
        return 0;
      }

    case WM_MOUSEACTIVATE:  /* We don't want to be activated */
      return MA_NOACTIVATE;

    case WM_PAINT:
      {
        PAINTSTRUCT ps;
        BeginPaint(Wnd, &ps);
        MenuDrawPopupMenu(Wnd, ps.hdc, (HMENU)GetWindowLongPtrW(Wnd, 0));
        EndPaint(Wnd, &ps);
        return 0;
      }

    case WM_PRINTCLIENT:
      {
         MenuDrawPopupMenu( Wnd, (HDC)wParam,
                                (HMENU)GetWindowLongPtrW( Wnd, 0 ) );
         return 0;
      }

    case WM_ERASEBKGND:
      return 1;

    case WM_DESTROY:
      /* zero out global pointer in case resident popup window was destroyed. */
      if (Wnd == top_popup)
        {
          top_popup = NULL;
          top_popup_hmenu = NULL;
        }
      break;

#ifdef __REACTOS__
    case WM_NCDESTROY:
      NtUserSetWindowFNID(Wnd, FNID_DESTROY);
      break;
#endif

    case WM_SHOWWINDOW:
      if (0 != wParam)
        {
          if (0 == GetWindowLongPtrW(Wnd, 0))
            {
              OutputDebugStringA("no menu to display\n");
            }
        }
      else
        {
          SetWindowLongPtrW(Wnd, 0, 0);
        }
      break;

    case MM_SETMENUHANDLE:
      SetWindowLongPtrW(Wnd, 0, wParam);
      break;

    case MM_GETMENUHANDLE:
    case MN_GETHMENU:
      return GetWindowLongPtrW(Wnd, 0);

    default:
      return DefWindowProcW(Wnd, Message, wParam, lParam);
    }

  return 0;
}

//
// This breaks some test results. Should handle A2U if called!
//
LRESULT WINAPI PopupMenuWndProcA(HWND Wnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
  PWND pWnd;

  pWnd = ValidateHwnd(Wnd);
  if (pWnd && !pWnd->fnid && Message != WM_NCCREATE)
  {
     return DefWindowProcA(Wnd, Message, wParam, lParam);
  }    
  TRACE("YES! hwnd=%x msg=0x%04x wp=0x%04lx lp=0x%08lx\n", Wnd, Message, wParam, lParam);

  switch(Message)
    {
    case WM_NCCREATE:
    case WM_CREATE:
    case WM_MOUSEACTIVATE:
    case WM_PAINT:
    case WM_PRINTCLIENT:
    case WM_ERASEBKGND:
    case WM_DESTROY:
    case WM_NCDESTROY:
    case WM_SHOWWINDOW:
    case MM_SETMENUHANDLE:
    case MM_GETMENUHANDLE:
    case MN_GETHMENU: 
      return PopupMenuWndProcW(Wnd, Message, wParam, lParam);

    default:
      return DefWindowProcA(Wnd, Message, wParam, lParam);
    }
  return 0;
}

/**********************************************************************
 *         MENU_ParseResource
 *
 * Parse a standard menu resource and add items to the menu.
 * Return a pointer to the end of the resource.
 *
 * NOTE: flags is equivalent to the mtOption field
 */
static LPCSTR MENU_ParseResource( LPCSTR res, HMENU hMenu)
{
    WORD flags, id = 0;
    HMENU hSubMenu;
    LPCWSTR str;
    BOOL end = FALSE;

    do
    {
        flags = GET_WORD(res);

        /* remove MF_END flag before passing it to AppendMenu()! */
        end = (flags & MF_END);
        if(end) flags ^= MF_END;

        res += sizeof(WORD);
        if(!(flags & MF_POPUP))
        {
            id = GET_WORD(res);
            res += sizeof(WORD);
        }
        str = (LPCWSTR)res;
        res += (strlenW(str) + 1) * sizeof(WCHAR);

        if (flags & MF_POPUP)
        {
            hSubMenu = CreatePopupMenu();
            if(!hSubMenu) return NULL;
            if(!(res = MENU_ParseResource(res, hSubMenu))) return NULL;
            AppendMenuW(hMenu, flags, (UINT_PTR)hSubMenu, (LPCWSTR)str);
        }
        else  /* Not a popup */
        {
            AppendMenuW(hMenu, flags, id, *(LPCWSTR)str ? (LPCWSTR)str : NULL);
        }
    } while(!end);
    return res;
}

/**********************************************************************
 *         MENUEX_ParseResource
 *
 * Parse an extended menu resource and add items to the menu.
 * Return a pointer to the end of the resource.
 */
static LPCSTR MENUEX_ParseResource(LPCSTR res, HMENU hMenu)
{
    WORD resinfo;
    do
    {
        MENUITEMINFOW mii;

        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_STATE | MIIM_ID | MIIM_TYPE;
        mii.fType = GET_DWORD(res);
        res += sizeof(DWORD);
        mii.fState = GET_DWORD(res);
        res += sizeof(DWORD);
        mii.wID = GET_DWORD(res);
        res += sizeof(DWORD);
        resinfo = GET_WORD(res);
        res += sizeof(WORD);
        /* Align the text on a word boundary.  */
        res += (~((UINT_PTR)res - 1)) & 1;
        mii.dwTypeData = (LPWSTR)res;
        mii.cch = strlenW(mii.dwTypeData);
        res += (1 + strlenW(mii.dwTypeData)) * sizeof(WCHAR);
        /* Align the following fields on a dword boundary.  */
        res += (~((UINT_PTR)res - 1)) & 3;

        TRACE("Menu item: [%08x,%08x,%04x,%04x,%S]\n",
              mii.fType, mii.fState, mii.wID, resinfo, mii.dwTypeData);

        if (resinfo & 1) /* Pop-up? */
        {
            /* DWORD helpid = GET_DWORD(res); FIXME: use this. */
            res += sizeof(DWORD);
            mii.hSubMenu = CreatePopupMenu();
            if (!mii.hSubMenu)
            {
                ERR("CreatePopupMenu failed\n");
                return NULL;
            }

            if (!(res = MENUEX_ParseResource(res, mii.hSubMenu)))
            {
                ERR("MENUEX_ParseResource failed\n");
                DestroyMenu(mii.hSubMenu);
                return NULL;
            }
            mii.fMask |= MIIM_SUBMENU;
        }
        else if (!mii.dwTypeData[0] && !(mii.fType & MF_SEPARATOR))
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
 *           DrawMenuBarTemp   (USER32.@)
 *
 * UNDOCUMENTED !!
 *
 * called by W98SE desk.cpl Control Panel Applet
 *
 * Not 100% sure about the param names, but close.
 *
 * @implemented
 */
DWORD WINAPI
DrawMenuBarTemp(HWND Wnd, HDC DC, LPRECT Rect, HMENU Menu, HFONT Font)
{
  ROSMENUINFO MenuInfo;
  ROSMENUITEMINFO ItemInfo;
  UINT i;
  HFONT FontOld = NULL;
  BOOL flat_menu = FALSE;

  SystemParametersInfoW (SPI_GETFLATMENU, 0, &flat_menu, 0);

  if (NULL == Menu)
    {
      Menu = GetMenu(Wnd);
    }

  if (NULL == Font)
    {
      Font = hMenuFont;
    }

  if (NULL == Rect || ! MenuGetRosMenuInfo(&MenuInfo, Menu))
    {
      return GetSystemMetrics(SM_CYMENU);
    }

  TRACE("(%x, %x, %p, %x, %x)\n", Wnd, DC, Rect, Menu, Font);

  FontOld = SelectObject(DC, Font);

  if (0 == MenuInfo.cyMenu)
    {
      MenuMenuBarCalcSize(DC, Rect, &MenuInfo, Wnd);
    }

  Rect->bottom = Rect->top + MenuInfo.cyMenu;

  FillRect(DC, Rect, GetSysColorBrush(flat_menu ? COLOR_MENUBAR : COLOR_MENU));

  SelectObject(DC, GetStockObject(DC_PEN));
  SetDCPenColor(DC, GetSysColor(COLOR_3DFACE));
  MoveToEx(DC, Rect->left, Rect->bottom - 1, NULL);
  LineTo(DC, Rect->right, Rect->bottom - 1);

  if (0 == MenuInfo.cItems)
    {
      SelectObject(DC, FontOld);
      return GetSystemMetrics(SM_CYMENU);
    }

  MenuInitRosMenuItemInfo(&ItemInfo);
  for (i = 0; i < MenuInfo.cItems; i++)
    {
      if (MenuGetRosMenuItemInfo(MenuInfo.Self, i, &ItemInfo))
        {
          MenuDrawMenuItem(Wnd, &MenuInfo, Wnd, DC, &ItemInfo,
                           MenuInfo.cyMenu, TRUE, ODA_DRAWENTIRE);
        }
    }
  MenuCleanupRosMenuItemInfo(&ItemInfo);

  SelectObject(DC, FontOld);

  return MenuInfo.cyMenu;
}


/***********************************************************************
 *           MenuShowSubPopup
 *
 * Display the sub-menu of the selected item of this menu.
 * Return the handle of the submenu, or menu if no submenu to display.
 */
static HMENU FASTCALL
MenuShowSubPopup(HWND WndOwner, PROSMENUINFO MenuInfo, BOOL SelectFirst, UINT Flags)
{
  RECT Rect;
  ROSMENUITEMINFO ItemInfo;
  ROSMENUINFO SubMenuInfo;
  HDC Dc;
  HMENU Ret;

  TRACE("owner=%x menu=%p 0x%04x\n", WndOwner, MenuInfo, SelectFirst);

  if (MenuInfo->iItem == NO_SELECTED_ITEM) return MenuInfo->Self;

  MenuInitRosMenuItemInfo(&ItemInfo);
  if (! MenuGetRosMenuItemInfo(MenuInfo->Self, MenuInfo->iItem, &ItemInfo))
  {
      MenuCleanupRosMenuItemInfo(&ItemInfo);
      return MenuInfo->Self;
  }
  
  //item = &menu->rgItems[menu->iItem];
  if (!(ItemInfo.hSubMenu) || (ItemInfo.fState & (MF_GRAYED | MF_DISABLED)))
  {
      MenuCleanupRosMenuItemInfo(&ItemInfo);
      return MenuInfo->Self;
  }

  /* message must be sent before using item,
     because nearly everything may be changed by the application ! */

  /* Send WM_INITMENUPOPUP message only if TPM_NONOTIFY flag is not specified */
  if (!(Flags & TPM_NONOTIFY))
  {
      SendMessageW(WndOwner, WM_INITMENUPOPUP, (WPARAM) ItemInfo.hSubMenu,
                   MAKELPARAM(MenuInfo->iItem, IS_SYSTEM_MENU(MenuInfo)));
  }

  if (! MenuGetRosMenuItemInfo(MenuInfo->Self, MenuInfo->iItem, &ItemInfo))
  {
      MenuCleanupRosMenuItemInfo(&ItemInfo);
      return MenuInfo->Self;
  }

  //item = &menu->rgItems[menu->iItem];
  Rect = ItemInfo.Rect;

  /* correct item if modified as a reaction to WM_INITMENUPOPUP message */
  if (!(ItemInfo.fState & MF_HILITE))
  {
      if (MenuInfo->fFlags & MNF_POPUP) Dc = GetDC(MenuInfo->Wnd);
      else Dc = GetDCEx(MenuInfo->Wnd, 0, DCX_CACHE | DCX_WINDOW);

      SelectObject(Dc, hMenuFont);
      ItemInfo.fMask |= MIIM_STATE;
      ItemInfo.fState |= MF_HILITE;
      MenuSetRosMenuItemInfo(MenuInfo->Self, MenuInfo->iItem, &ItemInfo);
      MenuDrawMenuItem(MenuInfo->Wnd, MenuInfo, WndOwner, Dc, &ItemInfo, MenuInfo->cyMenu,
                       !(MenuInfo->fFlags & MNF_POPUP), ODA_DRAWENTIRE);
      ReleaseDC(MenuInfo->Wnd, Dc);
  }

  if (!ItemInfo.Rect.top && !ItemInfo.Rect.left && !ItemInfo.Rect.bottom && !ItemInfo.Rect.right)
      ItemInfo.Rect = Rect;

  ItemInfo.fMask |= MIIM_STATE;
  ItemInfo.fState |= MF_MOUSESELECT;
  MenuSetRosMenuItemInfo(MenuInfo->Self, MenuInfo->iItem, &ItemInfo);

  if (IS_SYSTEM_MENU(MenuInfo))
  {
      MenuInitSysMenuPopup(ItemInfo.hSubMenu,
                           GetWindowLongPtrW(MenuInfo->Wnd, GWL_STYLE),
                           GetClassLongPtrW(MenuInfo->Wnd, GCL_STYLE), HTSYSMENU);

      NcGetSysPopupPos(MenuInfo->Wnd, &Rect);
      if (Flags & TPM_LAYOUTRTL) Rect.left = Rect.right;
      Rect.top = Rect.bottom;
      Rect.right = GetSystemMetrics(SM_CXSIZE);
      Rect.bottom = GetSystemMetrics(SM_CYSIZE);
  }
  else
  {
      GetWindowRect(MenuInfo->Wnd, &Rect);
      if (MenuInfo->fFlags & MNF_POPUP)
      {
          RECT rc = ItemInfo.Rect;
          
          MENU_AdjustMenuItemRect(MenuInfo, &rc);

          /* The first item in the popup menu has to be at the
             same y position as the focused menu item */
          if(Flags & TPM_LAYOUTRTL)
             Rect.left += GetSystemMetrics(SM_CXBORDER);
          else
             Rect.left += rc.right /*ItemInfo.Rect.right*/ - GetSystemMetrics(SM_CXBORDER);
          Rect.top += rc.top - MENU_TOP_MARGIN;//3;
          Rect.right = rc.left - rc.right + GetSystemMetrics(SM_CXBORDER);
          Rect.bottom = rc.top - rc.bottom - MENU_TOP_MARGIN - MENU_BOTTOM_MARGIN/*2*/
                                                     - GetSystemMetrics(SM_CYBORDER);
      }
      else
      {
          if(Flags & TPM_LAYOUTRTL)
              Rect.left += Rect.right - ItemInfo.Rect.left;
          else
              Rect.left += ItemInfo.Rect.left;
          Rect.top += ItemInfo.Rect.bottom;
          Rect.right = ItemInfo.Rect.right - ItemInfo.Rect.left;
          Rect.bottom = ItemInfo.Rect.bottom - ItemInfo.Rect.top;
      }
  }

  /* use default alignment for submenus */
  Flags &= ~(TPM_CENTERALIGN | TPM_RIGHTALIGN | TPM_VCENTERALIGN | TPM_BOTTOMALIGN);

  MENU_InitPopup( WndOwner, ItemInfo.hSubMenu, Flags );

  MenuShowPopup(WndOwner, ItemInfo.hSubMenu, MenuInfo->iItem, Flags,
                Rect.left, Rect.top, Rect.right, Rect.bottom );
  if (SelectFirst && MenuGetRosMenuInfo(&SubMenuInfo, ItemInfo.hSubMenu))
  {
      MenuMoveSelection(WndOwner, &SubMenuInfo, ITEM_NEXT);
  }

  Ret = ItemInfo.hSubMenu;
  MenuCleanupRosMenuItemInfo(&ItemInfo);

  return Ret;
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
    MENU *menu;
    menu = top_popup_hmenu ? MENU_GetMenu( top_popup_hmenu ) : NULL;
    if (menu && ( hwnd == menu->hWnd ||  hwnd == (menu->spwndNotify ? menu->spwndNotify->head.h : NULL)) )
       EndMenu();
}

/***********************************************************************
 *           MenuHideSubPopups
 *
 * Hide the sub-popup menus of this menu.
 */
static void FASTCALL
MenuHideSubPopups(HWND WndOwner, PROSMENUINFO MenuInfo,
                               BOOL SendMenuSelect, UINT wFlags)
{
  ROSMENUINFO SubMenuInfo;
  ROSMENUITEMINFO ItemInfo;

  TRACE("owner=%x menu=%x 0x%04x\n", WndOwner, MenuInfo, SendMenuSelect);

  if (MenuInfo && top_popup && NO_SELECTED_ITEM != MenuInfo->iItem)
  {
      //item = &menu->rgItems[menu->iItem];
      MenuInitRosMenuItemInfo(&ItemInfo);
      ItemInfo.fMask |= MIIM_FTYPE | MIIM_STATE;
      if (! MenuGetRosMenuItemInfo(MenuInfo->Self, MenuInfo->iItem, &ItemInfo)
          || !(ItemInfo.hSubMenu)
          || !(ItemInfo.fState & MF_MOUSESELECT))
      {
          MenuCleanupRosMenuItemInfo(&ItemInfo);
          return;
      }
      ItemInfo.fState &= ~MF_MOUSESELECT;
      ItemInfo.fMask |= MIIM_STATE;
      MenuSetRosMenuItemInfo(MenuInfo->Self, MenuInfo->iItem, &ItemInfo);
      if (MenuGetRosMenuInfo(&SubMenuInfo, ItemInfo.hSubMenu))
      {
          MenuHideSubPopups(WndOwner, &SubMenuInfo, FALSE, wFlags);
          MenuSelectItem(WndOwner, &SubMenuInfo, NO_SELECTED_ITEM, SendMenuSelect, NULL);

          DestroyWindow(SubMenuInfo.Wnd);
          /* Native returns handle to destroyed window */
          if (!(wFlags & TPM_NONOTIFY))
             SendMessageW( WndOwner, WM_UNINITMENUPOPUP, (WPARAM)ItemInfo.hSubMenu,
                                 MAKELPARAM(0, IS_SYSTEM_MENU(&SubMenuInfo)) );
          ////
          // Call WM_UNINITMENUPOPUP FIRST before destroy!!
          // Fixes todo_wine User32 test menu.c line 2233 GetMenuBarInfo callback....
          //
          SubMenuInfo.Wnd = NULL;
          MenuSetRosMenuInfo(&SubMenuInfo);
          ////
      }
  }
}

/***********************************************************************
 *           MenuSwitchTracking
 *
 * Helper function for menu navigation routines.
 */
static void FASTCALL
MenuSwitchTracking(MTRACKER* pmt, PROSMENUINFO PtMenuInfo, UINT Index, UINT wFlags)
{
  ROSMENUINFO TopMenuInfo;

  TRACE("%x menu=%x 0x%04x\n", pmt, PtMenuInfo->Self, Index);

  if ( MenuGetRosMenuInfo(&TopMenuInfo, pmt->TopMenu) &&
       pmt->TopMenu != PtMenuInfo->Self &&
      !((PtMenuInfo->fFlags | TopMenuInfo.fFlags) & MNF_POPUP) )
  {
      /* both are top level menus (system and menu-bar) */
      MenuHideSubPopups(pmt->OwnerWnd, &TopMenuInfo, FALSE, wFlags);
      MenuSelectItem(pmt->OwnerWnd, &TopMenuInfo, NO_SELECTED_ITEM, FALSE, NULL);
      pmt->TopMenu = PtMenuInfo->Self;
  }
  else
  {
      MenuHideSubPopups(pmt->OwnerWnd, PtMenuInfo, FALSE, wFlags);
  }

  MenuSelectItem(pmt->OwnerWnd, PtMenuInfo, Index, TRUE, NULL);
}

/***********************************************************************
 *           MenuExecFocusedItem
 *
 * Execute a menu item (for instance when user pressed Enter).
 * Return the wID of the executed item. Otherwise, -1 indicating
 * that no menu item was executed, -2 if a popup is shown;
 * Have to receive the flags for the TrackPopupMenu options to avoid
 * sending unwanted message.
 *
 */
static INT FASTCALL
MenuExecFocusedItem(MTRACKER *pmt, PROSMENUINFO MenuInfo, UINT Flags)
{
  ROSMENUITEMINFO ItemInfo;
  UINT wID;

  TRACE("%p menu=%p\n", pmt, MenuInfo);

  if (0 == MenuInfo->cItems || NO_SELECTED_ITEM == MenuInfo->iItem)
  {
      return -1;
  }

  MenuInitRosMenuItemInfo(&ItemInfo);
  if (! MenuGetRosMenuItemInfo(MenuInfo->Self, MenuInfo->iItem, &ItemInfo))
  {
      MenuCleanupRosMenuItemInfo(&ItemInfo);
      return -1;
  }

  TRACE("%p %08x %p\n", MenuInfo, ItemInfo.wID, ItemInfo.hSubMenu);

  if (0 == (ItemInfo.hSubMenu))
  {
      if (0 == (ItemInfo.fState & (MF_GRAYED | MF_DISABLED))
          && 0 == (ItemInfo.fType & MF_SEPARATOR))
      {
          /* If TPM_RETURNCMD is set you return the id, but
            do not send a message to the owner */
          if (0 == (Flags & TPM_RETURNCMD))
          {
              if (0 != (MenuInfo->fFlags & MNF_SYSMENU))
              {
                  PostMessageW(pmt->OwnerWnd, WM_SYSCOMMAND, ItemInfo.wID,
                               MAKELPARAM((SHORT) pmt->Pt.x, (SHORT) pmt->Pt.y));
              }
              else
              {
                  ROSMENUINFO topmenuI;
                  BOOL ret = MenuGetRosMenuInfo(&topmenuI, pmt->TopMenu);
                  DWORD dwStyle = MenuInfo->dwStyle | (ret ? topmenuI.dwStyle : 0);

                  if (dwStyle & MNS_NOTIFYBYPOS)
                      PostMessageW(pmt->OwnerWnd, WM_MENUCOMMAND, MenuInfo->iItem, (LPARAM)MenuInfo->Self);
                  else
                      PostMessageW(pmt->OwnerWnd, WM_COMMAND, ItemInfo.wID, 0);
              }
          }
          wID = ItemInfo.wID;
          MenuCleanupRosMenuItemInfo(&ItemInfo);
	  return wID;
      }
  }
  else
  {
      pmt->CurrentMenu = MenuShowSubPopup(pmt->OwnerWnd, MenuInfo, TRUE, Flags);
      return -2;
  }

  return -1;
}

/***********************************************************************
 *           MenuButtonDown
 *
 * Return TRUE if we can go on with menu tracking.
 */
static BOOL FASTCALL
MENU_ButtonDown(MTRACKER* pmt, HMENU hPtMenu, UINT Flags)
{
  int Index;
  ROSMENUINFO MenuInfo;
  ROSMENUITEMINFO Item;

  TRACE("%x PtMenu=%p\n", pmt, hPtMenu);

  if (NULL != hPtMenu)
  {
      if (! MenuGetRosMenuInfo(&MenuInfo, hPtMenu))
      {
          return FALSE;
      }
      if (IS_SYSTEM_MENU(&MenuInfo))
      {
          Index = 0;
      }
      else
      {
          Index = NtUserMenuItemFromPoint(pmt->OwnerWnd, hPtMenu, pmt->Pt.x, pmt->Pt.y);
      }
      MenuInitRosMenuItemInfo(&Item);
      if (NO_SELECTED_ITEM == Index || ! MenuGetRosMenuItemInfo(hPtMenu, Index, &Item))
      {
          MenuCleanupRosMenuItemInfo(&Item);
          return FALSE;
      }

      if (!(Item.fType & MF_SEPARATOR) &&
          !(Item.fState & (MFS_DISABLED | MFS_GRAYED)) )
      {
          if (MenuInfo.iItem != Index)
          {
              MenuSwitchTracking(pmt, &MenuInfo, Index, Flags);
          }

          /* If the popup menu is not already "popped" */
          if (!(Item.fState & MF_MOUSESELECT))
          {
              pmt->CurrentMenu = MenuShowSubPopup(pmt->OwnerWnd, &MenuInfo, FALSE, Flags);
          }

          MenuCleanupRosMenuItemInfo(&Item);

          return TRUE;
      }
      /* Else the click was on the menu bar, finish the tracking */
  }
  return FALSE;
}

/***********************************************************************
 *           MenuButtonUp
 *
 * Return the value of MenuExecFocusedItem if
 * the selected item was not a popup. Else open the popup.
 * A -1 return value indicates that we go on with menu tracking.
 *
 */
static INT FASTCALL
MENU_ButtonUp(MTRACKER *pmt, HMENU hPtMenu, UINT Flags)
{
  INT Id;
  ROSMENUINFO MenuInfo;
  ROSMENUITEMINFO ItemInfo;

  TRACE("%p hmenu=%x\n", pmt, hPtMenu);

  if (hPtMenu)
  {
      Id = 0;
      if (! MenuGetRosMenuInfo(&MenuInfo, hPtMenu))
      {
          return -1;
      }

      if (! IS_SYSTEM_MENU(&MenuInfo))
      {
          Id = NtUserMenuItemFromPoint(pmt->OwnerWnd, MenuInfo.Self, pmt->Pt.x, pmt->Pt.y);
      }
      MenuInitRosMenuItemInfo(&ItemInfo);
      if (0 <= Id && MenuGetRosMenuItemInfo(MenuInfo.Self, Id, &ItemInfo) &&
          MenuInfo.iItem == Id)
      {
          if (0 == (ItemInfo.hSubMenu))
          {
              INT ExecutedMenuId = MenuExecFocusedItem( pmt, &MenuInfo, Flags);
              MenuCleanupRosMenuItemInfo(&ItemInfo);
              if (ExecutedMenuId == -1 || ExecutedMenuId == -2) return -1;
              return ExecutedMenuId;
          }
          MenuCleanupRosMenuItemInfo(&ItemInfo);

          /* If we are dealing with the menu bar                  */
          /* and this is a click on an already "popped" item:     */
          /* Stop the menu tracking and close the opened submenus */
          if (pmt->TopMenu == MenuInfo.Self && MenuInfo.TimeToHide)
          {
              MenuCleanupRosMenuItemInfo(&ItemInfo);
              return 0;
          }
      }
      MenuCleanupRosMenuItemInfo(&ItemInfo);
      if( GetMenu(MenuInfo.Wnd) == hPtMenu )
      {
          MenuInfo.TimeToHide = TRUE;
          MenuSetRosMenuInfo(&MenuInfo);
      }
  }
  return -1;
}

/***********************************************************************
 *           MenuPtMenu
 *
 * Walks menu chain trying to find a menu pt maps to.
 */
static HMENU FASTCALL
MENU_PtMenu(HMENU hMenu, POINT pt)
{
  MENU *menu;
  PITEM pItem;
  HMENU ret = NULL;

  menu = MENU_GetMenu( hMenu );
  if (!menu) return NULL;

  /* try subpopup first (if any) */
  if (menu->iItem != NO_SELECTED_ITEM)
  {
     pItem = menu->rgItems ? DesktopPtrToUser(menu->rgItems) : NULL;
     if ( pItem ) pItem = &pItem[menu->iItem];
     if ( pItem && pItem->spSubMenu && pItem->fState & MF_MOUSESELECT)
     {
        PMENU pSubMenu = DesktopPtrToUser(pItem->spSubMenu);
        ret = MENU_PtMenu( UserHMGetHandle(pSubMenu), pt);
     }
  }

  /* check the current window (avoiding WM_HITTEST) */
  if (!ret)
  {
     INT ht = DefWndNCHitTest(menu->hWnd, pt);
     if ( menu->fFlags & MNF_POPUP )
     {
        if (ht != HTNOWHERE && ht != HTERROR) ret = hMenu;
     }
     else if (ht == HTSYSMENU)
        ret = get_win_sys_menu(menu->hWnd);
     else if (ht == HTMENU)
        ret = GetMenu( menu->hWnd );
  }
  return ret;
}

/***********************************************************************
 *           MenuMouseMove
 *
 * Return TRUE if we can go on with menu tracking.
 */
static BOOL FASTCALL
MenuMouseMove(MTRACKER *pmt, HMENU PtMenu, UINT Flags)
{
  INT Index;
  ROSMENUINFO MenuInfo;
  ROSMENUITEMINFO ItemInfo;

  if (NULL != PtMenu)
  {
      if (! MenuGetRosMenuInfo(&MenuInfo, PtMenu))
      {
          return TRUE;
      }
      if (IS_SYSTEM_MENU(&MenuInfo))
      {
          Index = 0;
      }
      else
      {
          Index = NtUserMenuItemFromPoint(pmt->OwnerWnd, PtMenu, pmt->Pt.x, pmt->Pt.y);
      }
  }
  else
  {
      Index = NO_SELECTED_ITEM;
  }

  if (NO_SELECTED_ITEM == Index)
  {
      if (pmt->CurrentMenu == MenuInfo.Self ||
          MenuGetRosMenuInfo(&MenuInfo, pmt->CurrentMenu))
      {
          MenuSelectItem(pmt->OwnerWnd, &MenuInfo, NO_SELECTED_ITEM,
                         TRUE, pmt->TopMenu);
      }
  }
  else if (MenuInfo.iItem != Index)
  {
	MenuInitRosMenuItemInfo(&ItemInfo);
	if (MenuGetRosMenuItemInfo(MenuInfo.Self, Index, &ItemInfo) &&
            !(ItemInfo.fType & MF_SEPARATOR))
	{
	    MenuSwitchTracking(pmt, &MenuInfo, Index, Flags);
            if (!(ItemInfo.fState & (MFS_DISABLED | MFS_GRAYED)))
	        pmt->CurrentMenu = MenuShowSubPopup(pmt->OwnerWnd, &MenuInfo, FALSE, Flags);
	}
	MenuCleanupRosMenuItemInfo(&ItemInfo);
  }

  return TRUE;
}

/***********************************************************************
 *           MenuGetSubPopup
 *
 * Return the handle of the selected sub-popup menu (if any).
 */
static
HMENU MENU_GetSubPopup( HMENU hmenu )
{
    MENU *menu;
    ITEM *item;

    menu = MENU_GetMenu( hmenu );

    if ((!menu) || (menu->iItem == NO_SELECTED_ITEM)) return 0;

    //item = &menu->rgItems[menu->iItem];
    item = menu->rgItems ? DesktopPtrToUser(menu->rgItems) : NULL;
    if (item) item = &item[menu->iItem];
    if (item && (item->spSubMenu) && (item->fState & MF_MOUSESELECT))
    {
       PMENU pSubMenu = DesktopPtrToUser(item->spSubMenu);
       return UserHMGetHandle(pSubMenu);
    }
    return 0;
}

/***********************************************************************
 *           MenuDoNextMenu
 *
 * NOTE: WM_NEXTMENU documented in Win32 is a bit different.
 */
static LRESULT FASTCALL
MenuDoNextMenu(MTRACKER* pmt, UINT Vk, UINT wFlags)
{
  ROSMENUINFO TopMenuInfo;
  ROSMENUINFO MenuInfo;

  if (! MenuGetRosMenuInfo(&TopMenuInfo, pmt->TopMenu))
  {
      return (LRESULT) FALSE;
  }

  if ((VK_LEFT == Vk && 0 == TopMenuInfo.iItem)
      || (VK_RIGHT == Vk && TopMenuInfo.iItem == TopMenuInfo.cItems - 1))
  {
      MDINEXTMENU NextMenu;
      HMENU NewMenu;
      HWND NewWnd;
      UINT Id = 0;

      NextMenu.hmenuIn = (IS_SYSTEM_MENU(&TopMenuInfo)) ? GetSubMenu(pmt->TopMenu, 0) : pmt->TopMenu;
      NextMenu.hmenuNext = NULL;
      NextMenu.hwndNext = NULL;
      SendMessageW(pmt->OwnerWnd, WM_NEXTMENU, Vk, (LPARAM) &NextMenu);

      TRACE("%p [%p] -> %p [%p]\n",
             pmt->CurrentMenu, pmt->OwnerWnd, NextMenu.hmenuNext, NextMenu.hwndNext );

      if (NULL == NextMenu.hmenuNext || NULL == NextMenu.hwndNext)
      {
          DWORD Style = GetWindowLongPtrW(pmt->OwnerWnd, GWL_STYLE);
          NewWnd = pmt->OwnerWnd;
          if (IS_SYSTEM_MENU(&TopMenuInfo))
          {
              /* switch to the menu bar */

              if (0 != (Style & WS_CHILD)
                  || NULL == (NewMenu = GetMenu(NewWnd)))
              {
                  return FALSE;
              }

              if (VK_LEFT == Vk)
              {
                  if (! MenuGetRosMenuInfo(&MenuInfo, NewMenu))
                  {
                      return FALSE;
                  }
                  Id = MenuInfo.cItems - 1;
              }
          }
          else if (0 != (Style & WS_SYSMENU))
          {
              /* switch to the system menu */
              NewMenu = get_win_sys_menu(NewWnd);
          }
          else
          {
              return FALSE;
          }
      }
      else    /* application returned a new menu to switch to */
      {
          NewMenu = NextMenu.hmenuNext;
          NewWnd = NextMenu.hwndNext;

          if (IsMenu(NewMenu) && IsWindow(NewWnd))
          {
              DWORD Style = GetWindowLongPtrW(NewWnd, GWL_STYLE);

              if (0 != (Style & WS_SYSMENU)
                  && get_win_sys_menu(NewWnd) == NewMenu)
              {
                  /* get the real system menu */
                  NewMenu = get_win_sys_menu(NewWnd);
              }
              else if (0 != (Style & WS_CHILD) || GetMenu(NewWnd) != NewMenu)
              {
                  /* FIXME: Not sure what to do here;
                   * perhaps try to track NewMenu as a popup? */

                  WARN(" -- got confused.\n");
                  return FALSE;
              }
          }
          else
          {
              return FALSE;
          }
      }

      if (NewMenu != pmt->TopMenu)
      {
          MenuSelectItem(pmt->OwnerWnd, &TopMenuInfo, NO_SELECTED_ITEM,
                         FALSE, 0 );
          if (pmt->CurrentMenu != pmt->TopMenu)
          {
              MenuHideSubPopups(pmt->OwnerWnd, &TopMenuInfo, FALSE, wFlags);
          }
      }

      if (NewWnd != pmt->OwnerWnd)
      {
          pmt->OwnerWnd = NewWnd;
          NtUserxSetGUIThreadHandle(MSQ_STATE_MENUOWNER, pmt->OwnerWnd); // 1
          SetCapture(pmt->OwnerWnd);                                          // 2
      }

      pmt->TopMenu = pmt->CurrentMenu = NewMenu; /* all subpopups are hidden */
      if (MenuGetRosMenuInfo(&TopMenuInfo, pmt->TopMenu))
      {
          MenuSelectItem(pmt->OwnerWnd, &TopMenuInfo, Id, TRUE, 0);
      }

      return TRUE;
  }

  return FALSE;
}

/***********************************************************************
 *           MenuSuspendPopup
 *
 * The idea is not to show the popup if the next input message is
 * going to hide it anyway.
 */
static BOOL FASTCALL
MenuSuspendPopup(MTRACKER* pmt, UINT uMsg)
{
    MSG msg;

    msg.hwnd = pmt->OwnerWnd;

    PeekMessageW( &msg, 0, uMsg, uMsg, PM_NOYIELD | PM_REMOVE); // ported incorrectly since 8317 GvG
    //pmt->TrackFlags |= TF_SKIPREMOVE; // This sends TrackMenu into a loop with arrow keys!!!!

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
                 pmt->TrackFlags |= TF_SUSPENDPOPUP;
                 return TRUE;
            }
        }
        break;
    }
    /* failures go through this */
    pmt->TrackFlags &= ~TF_SUSPENDPOPUP;
    return FALSE;
}

/***********************************************************************
 *           MenuKeyEscape
 *
 * Handle a VK_ESCAPE key event in a menu.
 */
static BOOL FASTCALL
MenuKeyEscape(MTRACKER *pmt, UINT Flags)
{
  BOOL EndMenu = TRUE;
  ROSMENUINFO MenuInfo;
  HMENU MenuTmp, MenuPrev;

  if (pmt->CurrentMenu != pmt->TopMenu)
  {
      if (MenuGetRosMenuInfo(&MenuInfo, pmt->CurrentMenu)
          && 0 != (MenuInfo.fFlags & MNF_POPUP))
      {
          MenuPrev = MenuTmp = pmt->TopMenu;

          /* close topmost popup */
          while (MenuTmp != pmt->CurrentMenu)
          {
              MenuPrev = MenuTmp;
              MenuTmp = MENU_GetSubPopup(MenuPrev);
          }

          if (MenuGetRosMenuInfo(&MenuInfo, MenuPrev))
          {
              MenuHideSubPopups(pmt->OwnerWnd, &MenuInfo, TRUE, Flags);
          }
          pmt->CurrentMenu = MenuPrev;
          EndMenu = FALSE;
      }
  }

  return EndMenu;
}

/***********************************************************************
 *           MenuKeyLeft
 *
 * Handle a VK_LEFT key event in a menu.
 */
static void FASTCALL
MenuKeyLeft(MTRACKER* pmt, UINT Flags)
{
  ROSMENUINFO MenuInfo;
  ROSMENUINFO TopMenuInfo;
  ROSMENUINFO PrevMenuInfo;
  HMENU MenuTmp, MenuPrev;
  UINT PrevCol;

  MenuPrev = MenuTmp = pmt->TopMenu;

  /* Try to move 1 column left (if possible) */
  if ( (PrevCol = MENU_GetStartOfPrevColumn(pmt->CurrentMenu)) != NO_SELECTED_ITEM)
  {
     if (MenuGetRosMenuInfo(&MenuInfo, pmt->CurrentMenu))
     {
          MenuSelectItem(pmt->OwnerWnd, &MenuInfo, PrevCol, TRUE, 0);
     }
     return;
  }

  /* close topmost popup */
  while (MenuTmp != pmt->CurrentMenu)
  {
      MenuPrev = MenuTmp;
      MenuTmp = MENU_GetSubPopup(MenuPrev);
  }

  if (! MenuGetRosMenuInfo(&PrevMenuInfo, MenuPrev))
  {
      return;
  }
  MenuHideSubPopups(pmt->OwnerWnd, &PrevMenuInfo, TRUE, Flags);
  pmt->CurrentMenu = MenuPrev;

  if (! MenuGetRosMenuInfo(&TopMenuInfo, pmt->TopMenu))
  {
      return;
  }
  if ((MenuPrev == pmt->TopMenu) && !(TopMenuInfo.fFlags & MNF_POPUP))
  {
      /* move menu bar selection if no more popups are left */

      if (!MenuDoNextMenu(pmt, VK_LEFT, Flags))
      {
          MenuMoveSelection(pmt->OwnerWnd, &TopMenuInfo, ITEM_PREV);
      }

      if (MenuPrev != MenuTmp || pmt->TrackFlags & TF_SUSPENDPOPUP)
      {
          /* A sublevel menu was displayed - display the next one
           * unless there is another displacement coming up */

          if (! MenuSuspendPopup(pmt, WM_KEYDOWN)
              && MenuGetRosMenuInfo(&TopMenuInfo, pmt->TopMenu))
          {
              pmt->CurrentMenu = MenuShowSubPopup(pmt->OwnerWnd, &TopMenuInfo,
                                                 TRUE, Flags);
          }
      }
  }
}

/***********************************************************************
 *           MenuKeyRight
 *
 * Handle a VK_RIGHT key event in a menu.
 */
static void FASTCALL MenuKeyRight(MTRACKER *pmt, UINT Flags)
{
    HMENU hmenutmp;
    ROSMENUINFO MenuInfo;
    ROSMENUINFO CurrentMenuInfo;
    UINT NextCol;

    TRACE("MenuKeyRight called, cur %p, top %p.\n",
         pmt->CurrentMenu, pmt->TopMenu);

    if (! MenuGetRosMenuInfo(&MenuInfo, pmt->TopMenu)) return;
    if ((MenuInfo.fFlags & MNF_POPUP) || (pmt->CurrentMenu != pmt->TopMenu))
    {
      /* If already displaying a popup, try to display sub-popup */

      hmenutmp = pmt->CurrentMenu;
      if (MenuGetRosMenuInfo(&CurrentMenuInfo, pmt->CurrentMenu))
        {
          pmt->CurrentMenu = MenuShowSubPopup(pmt->OwnerWnd, &CurrentMenuInfo, TRUE, Flags);
        }

      /* if subpopup was displayed then we are done */
      if (hmenutmp != pmt->CurrentMenu) return;
    }

    /* Check to see if there's another column */
    if ( (NextCol = MENU_GetStartOfNextColumn(pmt->CurrentMenu)) != NO_SELECTED_ITEM)
    {
       TRACE("Going to %d.\n", NextCol);
       if (MenuGetRosMenuInfo(&MenuInfo, pmt->CurrentMenu))
       {
          MenuSelectItem(pmt->OwnerWnd, &MenuInfo, NextCol, TRUE, 0);
       }
       return;
    }

    if (!(MenuInfo.fFlags & MNF_POPUP)) /* menu bar tracking */
    {
      if (pmt->CurrentMenu != pmt->TopMenu)
      {
          MenuHideSubPopups(pmt->OwnerWnd, &MenuInfo, FALSE, Flags);
          hmenutmp = pmt->CurrentMenu = pmt->TopMenu;
      }
      else
      {
          hmenutmp = NULL;
      }

      /* try to move to the next item */
      if ( !MenuDoNextMenu(pmt, VK_RIGHT, Flags))
          MenuMoveSelection(pmt->OwnerWnd, &MenuInfo, ITEM_NEXT);

      if ( hmenutmp || pmt->TrackFlags & TF_SUSPENDPOPUP )
      {
          if (! MenuSuspendPopup(pmt, WM_KEYDOWN)
              && MenuGetRosMenuInfo(&MenuInfo, pmt->TopMenu))
          {
              pmt->CurrentMenu = MenuShowSubPopup(pmt->OwnerWnd, &MenuInfo,
                                                 TRUE, Flags);
          }
      }
    }
}

/***********************************************************************
 *           MenuTrackMenu
 *
 * Menu tracking code.
 */
static INT FASTCALL MenuTrackMenu(HMENU hmenu, UINT wFlags, INT x, INT y,
                            HWND hwnd, const RECT *lprect )
{
    MSG msg;
    ROSMENUINFO MenuInfo;
    ROSMENUITEMINFO ItemInfo;
    PMENU menu;
    BOOL fRemove;
    INT executedMenuId = -1;
    MTRACKER mt;
    HWND capture_win;
    BOOL enterIdleSent = FALSE;

    mt.TrackFlags = 0;
    mt.CurrentMenu = hmenu;
    mt.TopMenu = hmenu;
    mt.OwnerWnd = hwnd;
    mt.Pt.x = x;
    mt.Pt.y = y;

    TRACE("hmenu=%p flags=0x%08x (%d,%d) hwnd=%x (%ld,%ld)-(%ld,%ld)\n",
         hmenu, wFlags, x, y, hwnd, lprect ? lprect->left : 0, lprect ? lprect->top : 0,
         lprect ? lprect->right : 0, lprect ? lprect->bottom : 0);

    if (!IsMenu(hmenu))
    {
        WARN("Invalid menu handle %p\n", hmenu); // Error already set in IsMenu.
        return FALSE;
    }

    if (! MenuGetRosMenuInfo(&MenuInfo, hmenu))
    {
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
    capture_win = (wFlags & TPM_POPUPMENU) ? MenuInfo.Wnd : mt.OwnerWnd;
    NtUserxSetGUIThreadHandle(MSQ_STATE_MENUOWNER, capture_win); // 1
    SetCapture(capture_win);                                          // 2

    while (!fEndMenu)
    {
        BOOL ErrorExit = FALSE;
        menu = MENU_GetMenu( mt.CurrentMenu );
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
                /* ReactOS Check */
                if (!ValidateHwndNoErr(mt.OwnerWnd) || !ValidateHwndNoErr(MenuInfo.Wnd))
                {
                   ErrorExit = TRUE; // Do not wait on dead windows, now test_capture_4 works.
                   break;
                }
                if (!enterIdleSent)
                {
                  HWND win = MenuInfo.fFlags & MNF_POPUP ? MenuInfo.Wnd : NULL;
                  enterIdleSent = TRUE;
                  SendMessageW( mt.OwnerWnd, WM_ENTERIDLE, MSGF_MENU, (LPARAM) win);
                }
                WaitMessage();
            }
        }

        if (ErrorExit) break; // Gracefully dropout.

        /* check if EndMenu() tried to cancel us, by posting this message */
        if (msg.message == WM_CANCELMODE)
        {
            /* we are now out of the loop */
            fEndMenu = TRUE;

            /* remove the message from the queue */
            PeekMessageW( &msg, 0, msg.message, msg.message, PM_REMOVE );

            /* break out of internal loop, ala ESCAPE */
            break;
        }

        TranslateMessage( &msg );
        mt.Pt = msg.pt;

        if ( (msg.hwnd == MenuInfo.Wnd) || (msg.message!=WM_TIMER) )
            enterIdleSent=FALSE;

        fRemove = FALSE;
        if ((msg.message >= WM_MOUSEFIRST) && (msg.message <= WM_MOUSELAST))
        {
            /*
             * Use the mouse coordinates in lParam instead of those in the MSG
             * struct to properly handle synthetic messages. They are already
             * in screen coordinates.
             */
            mt.Pt.x = (short)LOWORD(msg.lParam);
            mt.Pt.y = (short)HIWORD(msg.lParam);

            /* Find a menu for this mouse event */
            hmenu = MENU_PtMenu(mt.TopMenu, mt.Pt);

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
                    fRemove = MENU_ButtonDown(&mt, hmenu, wFlags);
                    fEndMenu = !fRemove;
                    if (msg.message == WM_LBUTTONDBLCLK) fEndMenu = TRUE; // Must exit or loop forever!
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
                        fEndMenu |= !MenuMouseMove( &mt, hmenu, wFlags );

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
                        if (MenuGetRosMenuInfo(&MenuInfo, mt.CurrentMenu))
                        {
                            MenuSelectItem(mt.OwnerWnd, &MenuInfo, NO_SELECTED_ITEM, FALSE, 0 );
                            MenuMoveSelection(mt.OwnerWnd, &MenuInfo, VK_HOME == msg.wParam ? ITEM_NEXT : ITEM_PREV);
                        }
                        break;

                    case VK_UP:
                    case VK_DOWN: /* If on menu bar, pull-down the menu */
                        if (MenuGetRosMenuInfo(&MenuInfo, mt.CurrentMenu))
                        {
                            if (!(MenuInfo.fFlags & MNF_POPUP))
                            {
                                if (MenuGetRosMenuInfo(&MenuInfo, mt.TopMenu))
                                    mt.CurrentMenu = MenuShowSubPopup(mt.OwnerWnd, &MenuInfo, TRUE, wFlags);
                            }
                            else      /* otherwise try to move selection */
                                MenuMoveSelection(mt.OwnerWnd, &MenuInfo, (msg.wParam == VK_UP)? ITEM_PREV : ITEM_NEXT );
                        }
                        break;

                    case VK_LEFT:
                        MenuKeyLeft( &mt, wFlags );
                        break;

                    case VK_RIGHT:
                        MenuKeyRight( &mt, wFlags );
                        break;

                    case VK_ESCAPE:
                        fEndMenu = MenuKeyEscape(&mt, wFlags);
                        break;

                    case VK_F1:
                    {
                        HELPINFO hi;
                        hi.cbSize = sizeof(HELPINFO);
                        hi.iContextType = HELPINFO_MENUITEM;
                        if (MenuGetRosMenuInfo(&MenuInfo, mt.CurrentMenu))
                        {
                            if (MenuInfo.iItem == NO_SELECTED_ITEM)
                                hi.iCtrlId = 0;
                            else
                            {
                                MenuInitRosMenuItemInfo(&ItemInfo);
                                if (MenuGetRosMenuItemInfo(MenuInfo.Self,
                                                           MenuInfo.iItem,
                                                           &ItemInfo))
                                {
                                    hi.iCtrlId = ItemInfo.wID;
                                }
                                else
                                {
                                    hi.iCtrlId = 0;
                                }
                                MenuCleanupRosMenuItemInfo(&ItemInfo);
                            }
                        }
                        hi.hItemHandle = hmenu;
                        hi.dwContextId = MenuInfo.dwContextHelpID;
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
                    UINT pos;

                    if (! MenuGetRosMenuInfo(&MenuInfo, mt.CurrentMenu)) break;
                    if (msg.wParam == L'\r' || msg.wParam == L' ')
                    {
                        executedMenuId = MenuExecFocusedItem(&mt, &MenuInfo, wFlags);
                        fEndMenu = (executedMenuId != -2);
                        break;
                    }

                    /* Hack to avoid control chars. */
                    /* We will find a better way real soon... */
                    if (msg.wParam < 32) break;

                    pos = MENU_FindItemByKey(mt.OwnerWnd, mt.CurrentMenu, LOWORD(msg.wParam), FALSE);
                    if (pos == (UINT)-2) fEndMenu = TRUE;
                    else if (pos == (UINT)-1) MessageBeep(0);
                    else
                    {
                        MenuSelectItem(mt.OwnerWnd, &MenuInfo, pos, TRUE, 0);
                        executedMenuId = MenuExecFocusedItem(&mt, &MenuInfo, wFlags);
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

        if (fRemove && !(mt.TrackFlags & TF_SKIPREMOVE) )
            PeekMessageW( &msg, 0, msg.message, msg.message, PM_REMOVE );
        else mt.TrackFlags &= ~TF_SKIPREMOVE;
    }

    NtUserxSetGUIThreadHandle(MSQ_STATE_MENUOWNER, NULL);
    SetCapture(NULL);  /* release the capture */

    /* If dropdown is still painted and the close box is clicked on
       then the menu will be destroyed as part of the DispatchMessage above.
       This will then invalidate the menu handle in mt.hTopMenu. We should
       check for this first.  */
    if( IsMenu( mt.TopMenu ) )
    {
        if (IsWindow(mt.OwnerWnd))
        {
            if (MenuGetRosMenuInfo(&MenuInfo, mt.TopMenu))
            {
                MenuHideSubPopups(mt.OwnerWnd, &MenuInfo, FALSE, wFlags);

                if (MenuInfo.fFlags & MNF_POPUP)
                {
                    IntNotifyWinEvent(EVENT_SYSTEM_MENUPOPUPEND, MenuInfo.Wnd, OBJID_CLIENT, CHILDID_SELF, 0);
                    DestroyWindow(MenuInfo.Wnd);
                    MenuInfo.Wnd = NULL;
                    MenuSetRosMenuInfo(&MenuInfo);

                    if (!(wFlags & TPM_NONOTIFY))
                      SendMessageW( mt.OwnerWnd, WM_UNINITMENUPOPUP, (WPARAM)mt.TopMenu,
                                 MAKELPARAM(0, IS_SYSTEM_MENU(&MenuInfo)) );
                }
                MenuSelectItem( mt.OwnerWnd, &MenuInfo, NO_SELECTED_ITEM, FALSE, 0 );
            }

            SendMessageW( mt.OwnerWnd, WM_MENUSELECT, MAKEWPARAM(0, 0xffff), 0 );
        }

        /* Reset the variable for hiding menu */
        if (MenuGetRosMenuInfo(&MenuInfo, mt.TopMenu))
        {
            MenuInfo.TimeToHide = FALSE;
            MenuSetRosMenuInfo(&MenuInfo);
        }
    }

    /* The return value is only used by TrackPopupMenu */
    if (!(wFlags & TPM_RETURNCMD)) return TRUE;
    if (executedMenuId == -1) executedMenuId = 0;
    return executedMenuId;
}

/***********************************************************************
 *           MenuInitTracking
 */
static BOOL FASTCALL MenuInitTracking(HWND hWnd, HMENU hMenu, BOOL bPopup, UINT wFlags)
{
    ROSMENUINFO MenuInfo = {0};
    
    TRACE("hwnd=%p hmenu=%p\n", hWnd, hMenu);

    if (!MenuGetRosMenuInfo(&MenuInfo, hMenu)) return FALSE;

    HideCaret(0);

    /* This makes the menus of applications built with Delphi work.
     * It also enables menus to be displayed in more than one window,
     * but there are some bugs left that need to be fixed in this case.
     */
    if (!bPopup)
    {
        MenuInfo.Wnd = hWnd;
        MenuSetRosMenuInfo(&MenuInfo);
    }
    //if (!bPopup) menu->hWnd = hWnd;
    if (!top_popup)
    {
       top_popup = MenuInfo.Wnd;//menu->hWnd;
       top_popup_hmenu = hMenu;
    }

    fEndMenu = FALSE;

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

       if (!MenuInfo.cyMenu)
       {
          /* app changed/recreated menu bar entries in WM_INITMENU
             Recalculate menu sizes else clicks will not work */
             SetWindowPos(hWnd, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE |
                       SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED );

       }
    }

    IntNotifyWinEvent( EVENT_SYSTEM_MENUSTART,
                       hWnd,
                       MenuInfo.fFlags & MNF_SYSMENU ? OBJID_SYSMENU : OBJID_MENU,
                       CHILDID_SELF, 0);
    return TRUE;
}

/***********************************************************************
 *           MenuExitTracking
 */
static BOOL FASTCALL MenuExitTracking(HWND hWnd, BOOL bPopup)
{
    TRACE("hwnd=%p\n", hWnd);

    IntNotifyWinEvent( EVENT_SYSTEM_MENUEND, hWnd, OBJID_WINDOW, CHILDID_SELF, 0);
    SendMessageW( hWnd, WM_EXITMENULOOP, bPopup, 0 );
    ShowCaret(0);
    top_popup = 0;
    top_popup_hmenu = NULL;
    return TRUE;
}

/***********************************************************************
 *           MenuTrackMouseMenuBar
 *
 * Menu-bar tracking upon a mouse event. Called from NC_HandleSysCommand().
 */
VOID MenuTrackMouseMenuBar( HWND hWnd, ULONG ht, POINT pt)
{
    HMENU hMenu = (ht == HTSYSMENU) ? get_win_sys_menu( hWnd ) : GetMenu(hWnd);
    UINT wFlags = TPM_BUTTONDOWN | TPM_LEFTALIGN | TPM_LEFTBUTTON;

    TRACE("wnd=%p ht=0x%04x (%ld,%ld)\n", hWnd, ht, pt.x, pt.y);

    if (GetWindowLongW( hWnd, GWL_EXSTYLE ) & WS_EX_LAYOUTRTL) wFlags |= TPM_LAYOUTRTL;
    if (IsMenu(hMenu))
    {
        /* map point to parent client coordinates */
        HWND Parent = GetAncestor(hWnd, GA_PARENT );
        if (Parent != GetDesktopWindow())
        {
            ScreenToClient(Parent, &pt);
        }

        MenuInitTracking(hWnd, hMenu, FALSE, wFlags);
        /* fetch the window menu again, it may have changed */
        hMenu = (ht == HTSYSMENU) ? get_win_sys_menu( hWnd ) : GetMenu( hWnd );
        MenuTrackMenu(hMenu, wFlags, pt.x, pt.y, hWnd, NULL);
        MenuExitTracking(hWnd, FALSE);
    }
}

/***********************************************************************
 *           MenuTrackKbdMenuBar
 *
 * Menu-bar tracking upon a keyboard event. Called from NC_HandleSysCommand().
 */
VOID MenuTrackKbdMenuBar(HWND hwnd, UINT wParam, WCHAR wChar)
{
    UINT uItem = NO_SELECTED_ITEM;
    HMENU hTrackMenu;
    ROSMENUINFO MenuInfo;
    UINT wFlags = TPM_LEFTALIGN | TPM_LEFTBUTTON;

    TRACE("hwnd %p wParam 0x%04x wChar 0x%04x\n", hwnd, wParam, wChar);

    /* find window that has a menu */

    while (!((GetWindowLongPtrW( hwnd, GWL_STYLE ) &
                                         (WS_CHILD | WS_POPUP)) != WS_CHILD))
        if (!(hwnd = GetAncestor( hwnd, GA_PARENT ))) return;

    /* check if we have to track a system menu */

    hTrackMenu = GetMenu( hwnd );
    if (!hTrackMenu || IsIconic(hwnd) || wChar == ' ' )
    {
        if (!(GetWindowLongPtrW( hwnd, GWL_STYLE ) & WS_SYSMENU)) return;
        hTrackMenu = get_win_sys_menu(hwnd);
        uItem = 0;
        wParam |= HTSYSMENU; /* prevent item lookup */
    }

    if (!IsMenu( hTrackMenu )) return;

    MenuInitTracking( hwnd, hTrackMenu, FALSE, wFlags );

    /* fetch the window menu again, it may have changed */
    hTrackMenu = (wParam & HTSYSMENU) ? get_win_sys_menu( hwnd ) : GetMenu( hwnd );

    if (! MenuGetRosMenuInfo(&MenuInfo, hTrackMenu))
    {
      goto track_menu;
    }

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

    MenuSelectItem( hwnd, &MenuInfo, uItem, TRUE, 0 );

    if (!(wParam & HTSYSMENU) || wChar == ' ')
    {
        if( uItem == NO_SELECTED_ITEM )
            MenuMoveSelection( hwnd, &MenuInfo, ITEM_NEXT );
        else
            PostMessageW( hwnd, WM_KEYDOWN, VK_RETURN, 0 );
    }

track_menu:
    MenuTrackMenu( hTrackMenu, wFlags, 0, 0, hwnd, NULL );
    MenuExitTracking( hwnd, FALSE );
}

/**********************************************************************
 *           TrackPopupMenuEx   (USER32.@)
 */
BOOL WINAPI TrackPopupMenuEx( HMENU hMenu, UINT wFlags, int x, int y,
                              HWND hWnd, LPTPMPARAMS lpTpm)
{
    BOOL ret = FALSE;
    MENU *menu;

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

    if (MENU_InitPopup( hWnd, hMenu, wFlags ))
    {
       MenuInitTracking(hWnd, hMenu, TRUE, wFlags);

       /* Send WM_INITMENUPOPUP message only if TPM_NONOTIFY flag is not specified */
       if (!(wFlags & TPM_NONOTIFY))
           SendMessageW(hWnd, WM_INITMENUPOPUP, (WPARAM) hMenu, 0);

       if (MenuShowPopup(hWnd, hMenu, 0, wFlags, x, y, 0, 0 ))
          ret = MenuTrackMenu(hMenu, wFlags | TPM_POPUPMENU, 0, 0, hWnd,
                              lpTpm ? &lpTpm->rcExclude : NULL);
       MenuExitTracking(hWnd, TRUE);

       if (menu->hWnd)
       {
          ROSMENUINFO MenuInfo;
          if (IsWindow( menu->hWnd )) // wine hack around this with their destroy function.
             DestroyWindow( menu->hWnd ); // Fix wrong error return.
          //menu->hWnd = 0;
          MenuGetRosMenuInfo(&MenuInfo, menu->head.h);
          MenuInfo.Wnd = 0;
          MenuSetRosMenuInfo(&MenuInfo);
          
          if (!(wFlags & TPM_NONOTIFY))
             SendMessageW( hWnd, WM_UNINITMENUPOPUP, (WPARAM)hMenu,
                                            MAKELPARAM(0, IS_SYSTEM_MENU(menu)) );  
       }
    }
    return ret;
}

/**********************************************************************
 *           TrackPopupMenu     (USER32.@)
 */
BOOL WINAPI TrackPopupMenu( HMENU Menu, UINT Flags, int x, int y,
                            int Reserved, HWND Wnd, CONST RECT *Rect)
{
    return TrackPopupMenuEx( Menu, Flags, x, y, Wnd, NULL);
}

/**********************************************************************
 *         MENU_mnu2mnuii
 *
 * Uses flags, id and text ptr, passed by InsertMenu() and
 * ModifyMenu() to setup a MenuItemInfo structure.
 */
static void MENU_mnu2mnuii( UINT flags, UINT_PTR id, LPCWSTR str, LPMENUITEMINFOW pmii, BOOL Unicode)
{
    RtlZeroMemory( pmii, sizeof( MENUITEMINFOW));
    pmii->cbSize = sizeof( MENUITEMINFOW);
    pmii->fMask = MIIM_STATE | MIIM_ID | MIIM_FTYPE;
    /* setting bitmap clears text and vice versa */
    if( IS_STRING_ITEM(flags)) {
        pmii->fMask |= MIIM_STRING | MIIM_BITMAP;
        if( !str)
            flags |= MF_SEPARATOR;
        /* Item beginning with a backspace is a help item */
        /* FIXME: wrong place, this is only true in win16 */
        else
        {
            if (Unicode)
           {
              if (*str == '\b')
              {
                flags |= MF_HELP;
                str++;
              }
           }
           else
           {
              LPCSTR NewItemA = (LPCSTR) str;
              if (*NewItemA == '\b')
              {
                 flags |= MF_HELP;
                 NewItemA++;
                 str = (LPCWSTR) NewItemA;
              }
              TRACE("A cch %d\n",strlen(NewItemA));
           }
        }
        pmii->dwTypeData = (LPWSTR)str;
    } else if( flags & MFT_BITMAP){
        pmii->fMask |= MIIM_BITMAP | MIIM_STRING;
        pmii->hbmpItem = (HBITMAP)str;
    }
    if( flags & MF_OWNERDRAW){
        pmii->fMask |= MIIM_DATA;
        pmii->dwItemData = (ULONG_PTR) str;
    }
    if( flags & MF_POPUP && MENU_GetMenu((HMENU)id)) {
        pmii->fMask |= MIIM_SUBMENU;
        pmii->hSubMenu = (HMENU)id;
    }
    if( flags & MF_SEPARATOR) flags |= MF_GRAYED | MF_DISABLED;
    pmii->fState = flags & MENUITEMINFO_STATE_MASK & ~MFS_DEFAULT;
    pmii->fType = flags & MENUITEMINFO_TYPE_MASK;
    pmii->wID = (UINT)id;
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
    if( !pmii_in || (pmii_in->cbSize != sizeof( MENUITEMINFOW) &&
            pmii_in->cbSize != sizeof( MENUITEMINFOW) - sizeof( pmii_in->hbmpItem)) ) {
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
        ERR("invalid combination of fMask bits used\n");
        /* this does not happen on Win9x/ME */
        SetLastError( ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    /* convert old style (MIIM_TYPE) to the new and keep the old one too */
    if( pmii_out->fMask & MIIM_TYPE){
        pmii_out->fMask |= MIIM_FTYPE;
        if( IS_STRING_ITEM(pmii_out->fType)){
            pmii_out->fMask |= MIIM_STRING;
        } else if( (pmii_out->fType) & MFT_BITMAP){
            pmii_out->fMask |= MIIM_BITMAP;
            pmii_out->hbmpItem = UlongToHandle(LOWORD(pmii_out->dwTypeData));
        }
    }
    if (pmii_out->fMask & MIIM_FTYPE )
    {
        pmii_out->fType &= ~MENUITEMINFO_TYPE_MASK;
        pmii_out->fType |= pmii_in->fType & MENUITEMINFO_TYPE_MASK;
    }
    if (pmii_out->fMask & MIIM_STATE)
    /* Other menu items having MFS_DEFAULT are not converted
       to normal items */
       pmii_out->fState = pmii_in->fState & MENUITEMINFO_STATE_MASK;

    if (pmii_out->fMask & MIIM_SUBMENU)
    {
       if ((pmii_out->hSubMenu != NULL) && !IsMenu(pmii_out->hSubMenu))
          return FALSE;
    }

    return TRUE;
}

BOOL
MenuInit(VOID)
{
  NONCLIENTMETRICSW ncm;

  /* get the menu font */
  if(!hMenuFont || !hMenuFontBold)
  {
    ncm.cbSize = sizeof(ncm);
    if(!SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0))
    {
      ERR("MenuInit(): SystemParametersInfoW(SPI_GETNONCLIENTMETRICS) failed!\n");
      return FALSE;
    }

    hMenuFont = CreateFontIndirectW(&ncm.lfMenuFont);
    if(hMenuFont == NULL)
    {
      ERR("MenuInit(): CreateFontIndirectW(hMenuFont) failed!\n");
      return FALSE;
    }

    ncm.lfMenuFont.lfWeight = max(ncm.lfMenuFont.lfWeight + 300, 1000);
    hMenuFontBold = CreateFontIndirectW(&ncm.lfMenuFont);
    if(hMenuFontBold == NULL)
    {
      ERR("MenuInit(): CreateFontIndirectW(hMenuFontBold) failed!\n");
      DeleteObject(hMenuFont);
      hMenuFont = NULL;
      return FALSE;
    }
  }

  return TRUE;
}

VOID
MenuCleanup(VOID)
{
  if (hMenuFont)
  {
    DeleteObject(hMenuFont);
    hMenuFont = NULL;
  }

  if (hMenuFontBold)
  {
    DeleteObject(hMenuFontBold);
    hMenuFontBold = NULL;
  }
}

HMENU FASTCALL MENU_LoadSystemMenu(BOOL mdi)
{
  HMENU hmenu = LoadMenuW(User32Instance, L"SYSMENU");

  if (hmenu)
  {
     MENUINFO menuinfo = {0};
     MENUITEMINFOW info = {0};
     //WCHAR buf[128];

     // removing space for checkboxes from menu
     menuinfo.cbSize = sizeof(menuinfo);
     menuinfo.fMask = MIM_STYLE;
     GetMenuInfo(hmenu, &menuinfo);
     menuinfo.dwStyle |= MNS_CHECKORBMP;
     SetMenuInfo(hmenu, &menuinfo);

     // adding bitmaps to menu items
     info.cbSize = sizeof(info);
     info.fMask |= MIIM_BITMAP;
     info.hbmpItem = HBMMENU_POPUP_MINIMIZE;
     SetMenuItemInfoW(hmenu, SC_MINIMIZE, FALSE, &info);
     info.hbmpItem = HBMMENU_POPUP_RESTORE;
     SetMenuItemInfoW(hmenu, SC_RESTORE, FALSE, &info);
     info.hbmpItem = HBMMENU_POPUP_MAXIMIZE;
     SetMenuItemInfoW(hmenu, SC_MAXIMIZE, FALSE, &info);
     info.hbmpItem = HBMMENU_POPUP_CLOSE;
     SetMenuItemInfoW(hmenu, SC_CLOSE, FALSE, &info);
     if (mdi)
     {
        AppendMenuW(hmenu, MF_SEPARATOR, 0, NULL);
        //LoadStringW(User32Instance, IDS_MDI_NEXT, buf, sizeof(buf)/sizeof(WCHAR));
        //AppendMenuW(hmenu, MF_STRING, SC_NEXTWINDOW, buf);
     }
  }
  return hmenu;
}

NTSTATUS WINAPI
User32LoadSysMenuTemplateForKernel(PVOID Arguments, ULONG ArgumentLength)
{
  LRESULT Result = 0;

  // Will be converted to load bitmaps for OBMI!

  return(ZwCallbackReturn(&Result, sizeof(LRESULT), STATUS_SUCCESS));
}

NTSTATUS WINAPI
User32CallLoadMenuFromKernel(PVOID Arguments, ULONG ArgumentLength)
{
  PLOADMENU_CALLBACK_ARGUMENTS Common;
  LRESULT Result;

  Common = (PLOADMENU_CALLBACK_ARGUMENTS) Arguments;

  Result = (LRESULT)LoadMenuW( Common->hModule, Common->InterSource ? MAKEINTRESOURCE(Common->InterSource) : (LPCWSTR)&Common->MenuName);

  return ZwCallbackReturn(&Result, sizeof(LRESULT), STATUS_SUCCESS);
}


/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
BOOL WINAPI
AppendMenuA(HMENU hMenu,
	    UINT uFlags,
	    UINT_PTR uIDNewItem,
	    LPCSTR lpNewItem)
{
  MENUITEMINFOW mii;
  UNICODE_STRING UnicodeString;
  BOOL res;

  RtlInitUnicodeString(&UnicodeString, 0);

  MENU_mnu2mnuii( uFlags, uIDNewItem, (LPCWSTR)lpNewItem, &mii, FALSE);

  /* copy the text string, it will be one or the other */
  if (lpNewItem && mii.fMask & MIIM_STRING && !mii.hbmpItem && mii.dwTypeData)
  {
      if (!RtlCreateUnicodeStringFromAsciiz(&UnicodeString, (LPSTR)mii.dwTypeData))
      {
        SetLastError (ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
      }
      mii.dwTypeData = UnicodeString.Buffer;
      mii.cch = UnicodeString.Length / sizeof(WCHAR);
  }
  else
  {
      TRACE("AMA Handle bitmaps\n");
  }
  ////// Answer a question, why a -1? To hunt for the end of the item list. Get it, to Append?
  res = NtUserThunkedMenuItemInfo(hMenu, -1, TRUE, TRUE, &mii, &UnicodeString);
  if ( UnicodeString.Buffer ) RtlFreeUnicodeString ( &UnicodeString );
  return res;
}

/*
 * @implemented
 */
BOOL WINAPI
AppendMenuW(HMENU hMenu,
	    UINT uFlags,
	    UINT_PTR uIDNewItem,
	    LPCWSTR lpNewItem)
{
  MENUITEMINFOW mii;
  UNICODE_STRING MenuText;
  BOOL res;

  RtlInitUnicodeString(&MenuText, 0);    

  MENU_mnu2mnuii( uFlags, uIDNewItem, lpNewItem, &mii, TRUE);

  /* copy the text string, it will be one or the other */
  if (lpNewItem && mii.fMask & MIIM_STRING && !mii.hbmpItem && mii.dwTypeData)
  {
    RtlInitUnicodeString(&MenuText, (PWSTR)mii.dwTypeData);
    mii.dwTypeData = MenuText.Buffer;
    mii.cch = MenuText.Length / sizeof(WCHAR);
  }
  res = NtUserThunkedMenuItemInfo(hMenu, -1, TRUE, TRUE, &mii, &MenuText);
  return res;
}

/*
 * @implemented
 */
DWORD WINAPI
CheckMenuItem(HMENU hmenu,
	      UINT uIDCheckItem,
	      UINT uCheck)
{
  PITEM item;
  DWORD Ret;
  UINT uID = uIDCheckItem;

  if (!ValidateHandle(hmenu, TYPE_MENU))
     return -1;

  if (!(item = MENU_FindItem( &hmenu, &uID, uCheck ))) return -1;

  Ret = item->fState & MFS_CHECKED;
  if ( Ret == (uCheck & MFS_CHECKED)) return Ret; // Already Checked...

  return NtUserCheckMenuItem(hmenu, uIDCheckItem, uCheck);
}

/*
 * @implemented
 */
BOOL WINAPI
CheckMenuRadioItem(HMENU hMenu,
                   UINT first,
                   UINT last,
                   UINT check,
                   UINT bypos)
{
    BOOL done = FALSE;
    UINT i;
    PITEM mi_first = NULL, mi_check;
    HMENU m_first, m_check;
    MENUITEMINFOW mii;
    mii.cbSize = sizeof( mii);

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
            if (!(mi_check->fType & MFT_RADIOCHECK) || !(mi_check->fState & MFS_CHECKED))
            {
               mii.fMask = MIIM_FTYPE | MIIM_STATE;
               mii.fType = (mi_check->fType & MENUITEMINFO_TYPE_MASK) | MFT_RADIOCHECK;
               mii.fState = (mi_check->fState & MII_STATE_MASK) | MFS_CHECKED;
               NtUserThunkedMenuItemInfo(m_check, i, bypos, FALSE, &mii, NULL);
            }
            done = TRUE;
        }
        else
        {
            /* MSDN is wrong, Windows does not remove MFT_RADIOCHECK */
            if (mi_check->fState & MFS_CHECKED)
            {
               mii.fMask = MIIM_STATE;
               mii.fState = (mi_check->fState & MII_STATE_MASK) & ~MFS_CHECKED;
               NtUserThunkedMenuItemInfo(m_check, i, bypos, FALSE, &mii, NULL);
            }
        }
    }
    return done;
}

/*
 * @implemented
 */
HMENU WINAPI
CreateMenu(VOID)
{
  MenuLoadBitmaps();
  return NtUserxCreateMenu();
}

/*
 * @implemented
 */
HMENU WINAPI
CreatePopupMenu(VOID)
{
  MenuLoadBitmaps();
  return NtUserxCreatePopupMenu();
}

/*
 * @implemented
 */
BOOL WINAPI
DrawMenuBar(HWND hWnd)
{
  return NtUserxDrawMenuBar(hWnd);
}

/*
 * @implemented
 */
BOOL WINAPI
EnableMenuItem(HMENU hMenu,
	       UINT uIDEnableItem,
	       UINT uEnable)
{
  return NtUserEnableMenuItem(hMenu, uIDEnableItem, uEnable);
}

/*
 * @implemented
 */
BOOL WINAPI
EndMenu(VOID)
{
  GUITHREADINFO guii;
  guii.cbSize = sizeof(GUITHREADINFO);
  if(GetGUIThreadInfo(GetCurrentThreadId(), &guii) && guii.hwndMenuOwner)
  {
    if (!fEndMenu &&
         top_popup &&
         guii.hwndMenuOwner != top_popup )
    {
       ERR("Capture GUI pti hWnd does not match top_popup!\n");
    }
  }

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

BOOL WINAPI HiliteMenuItem( HWND hWnd, HMENU hMenu, UINT wItemID,
                                UINT wHilite )
{
    ROSMENUINFO MenuInfo;
    TRACE("(%p, %p, %04x, %04x);\n", hWnd, hMenu, wItemID, wHilite);
    // Force bits to be set call server side....
    // This alone works and passes all the menu test_menu_hilitemenuitem tests.
    if (!NtUserHiliteMenuItem(hWnd, hMenu, wItemID, wHilite)) return FALSE;
    // Without the above call we fail 3 out of the wine failed todo tests, see CORE-7967
    // Now redraw menu.
    if (MenuGetRosMenuInfo(&MenuInfo, hMenu))
    {
       if (MenuInfo.iItem == wItemID) return TRUE;
       MenuHideSubPopups( hWnd, &MenuInfo, FALSE, 0 );
       MenuSelectItem( hWnd, &MenuInfo, wItemID, TRUE, 0 );
    }
    return TRUE; // Always returns TRUE!
}

/*
 * @implemented
 */
HMENU WINAPI
GetMenu(HWND hWnd)
{
       PWND Wnd = ValidateHwnd(hWnd);

       if (!Wnd)
               return NULL;

       return UlongToHandle(Wnd->IDMenu);
}

/*
 * @implemented
 */
BOOL WINAPI GetMenuBarInfo( HWND hwnd, LONG idObject, LONG idItem, PMENUBARINFO pmbi )
{
    BOOL Ret;
    Ret = NtUserGetMenuBarInfo( hwnd, idObject, idItem, pmbi);
    // Reason to move to server side!!!!!
    if (!Ret) return Ret;
    // EL HAXZO!!!
    pmbi->fBarFocused = top_popup_hmenu == pmbi->hMenu;
    if (!idItem)
    {
        pmbi->fFocused = pmbi->fBarFocused;
    }

    return TRUE;
}

/*
 * @implemented
 */
LONG WINAPI
GetMenuCheckMarkDimensions(VOID)
{
  return(MAKELONG(GetSystemMetrics(SM_CXMENUCHECK),
		  GetSystemMetrics(SM_CYMENUCHECK)));
}

/*
 * @implemented
 */
DWORD
WINAPI
GetMenuContextHelpId(HMENU hmenu)
{
  PMENU pMenu;
  if ((pMenu = ValidateHandle(hmenu, TYPE_MENU)))
     return pMenu->dwContextHelpId;
  return 0;
}

/*
 * @implemented
 */
UINT WINAPI
GetMenuDefaultItem(HMENU hMenu,
		   UINT fByPos,
		   UINT gmdiFlags)
{
  PMENU pMenu;
  DWORD gismc = 0;
  if (!(pMenu = ValidateHandle(hMenu, TYPE_MENU)))
       return (UINT)-1;

  return IntGetMenuDefaultItem( pMenu, (BOOL)fByPos, gmdiFlags, &gismc);
}

/*
 * @implemented
 */
BOOL WINAPI
GetMenuInfo(HMENU hmenu,
	    LPMENUINFO lpcmi)
{
  PMENU pMenu;

  if (!lpcmi || (lpcmi->cbSize != sizeof(MENUINFO)))
  {
     SetLastError(ERROR_INVALID_PARAMETER);
     return FALSE;
  }

  if (!(pMenu = ValidateHandle(hmenu, TYPE_MENU)))
     return FALSE;

  if (lpcmi->fMask & MIM_BACKGROUND)
      lpcmi->hbrBack = pMenu->hbrBack;

  if (lpcmi->fMask & MIM_HELPID)
      lpcmi->dwContextHelpID = pMenu->dwContextHelpId;

  if (lpcmi->fMask & MIM_MAXHEIGHT)
      lpcmi->cyMax = pMenu->cyMax;

  if (lpcmi->fMask & MIM_MENUDATA)
      lpcmi->dwMenuData = pMenu->dwMenuData;

  if (lpcmi->fMask & MIM_STYLE)
      lpcmi->dwStyle = pMenu->fFlags & MNS_STYLE_MASK;

  return TRUE;
}

/*
 * @implemented
 */
int WINAPI
GetMenuItemCount(HMENU hmenu)
{
  PMENU pMenu;
  if ((pMenu = ValidateHandle(hmenu, TYPE_MENU)))
     return pMenu->cItems;
  return -1;
}

/*
 * @implemented
 */
UINT WINAPI
GetMenuItemID(HMENU hMenu,
	      int nPos)
{
  ITEM * lpmi;
  if (!(lpmi = MENU_FindItem(&hMenu,(UINT*)&nPos,MF_BYPOSITION))) return -1;
  if (lpmi->spSubMenu) return -1;
  return lpmi->wID;
}

/*
 * @implemented
 */
BOOL WINAPI
GetMenuItemInfoA(
   HMENU hmenu,
   UINT item,
   BOOL bypos,
   LPMENUITEMINFOA lpmii)
{
    BOOL ret;
    MENUITEMINFOA mii;

    if( lpmii->cbSize != sizeof( mii) &&
        lpmii->cbSize != sizeof( mii) - sizeof ( mii.hbmpItem))
    {
        SetLastError( ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    memcpy( &mii, lpmii, lpmii->cbSize);
    mii.cbSize = sizeof( mii);
    ret = GetMenuItemInfo_common (hmenu,
                                  item,
                                  bypos,
                                  (LPMENUITEMINFOW)&mii,
                                  FALSE);
    mii.cbSize = lpmii->cbSize;
    memcpy( lpmii, &mii, mii.cbSize);
    return ret;
}

/*
 * @implemented
 */
BOOL WINAPI
GetMenuItemInfoW(
   HMENU hMenu,
   UINT Item,
   BOOL bypos,
   LPMENUITEMINFOW lpmii)
{
   BOOL ret;
   MENUITEMINFOW mii;
   if( lpmii->cbSize != sizeof( mii) && lpmii->cbSize != sizeof( mii) - sizeof ( mii.hbmpItem))
   {
      SetLastError( ERROR_INVALID_PARAMETER);
      return FALSE;
   }
   memcpy( &mii, lpmii, lpmii->cbSize);
   mii.cbSize = sizeof( mii);
   ret = GetMenuItemInfo_common (hMenu, Item, bypos, &mii, TRUE);
   mii.cbSize = lpmii->cbSize;
   memcpy( lpmii, &mii, mii.cbSize);
   return ret;
}

/*
 * @implemented
 */
UINT
WINAPI
GetMenuState(
  HMENU hMenu,
  UINT uId,
  UINT uFlags)
{
  PITEM pItem;
  UINT Type = 0;
  TRACE("(menu=%p, id=%04x, flags=%04x);\n", hMenu, uId, uFlags);
  if (!(pItem = MENU_FindItem( &hMenu, &uId, uFlags ))) return -1;

  if (!pItem->Xlpstr && pItem->hbmp) Type = MFT_BITMAP;

  if (pItem->spSubMenu)
  {
     PMENU pSubMenu = DesktopPtrToUser(pItem->spSubMenu);
     HMENU hsubmenu = UserHMGetHandle(pSubMenu);
     if (!IsMenu(hsubmenu)) return (UINT)-1;
     else return (pSubMenu->cItems << 8) | ((pItem->fState|pItem->fType|Type) & 0xff);
  }
  else
     return (pItem->fType | pItem->fState | Type);  
}

/*
 * @implemented
 */
int
WINAPI
GetMenuStringA(
  HMENU hMenu,
  UINT uIDItem,
  LPSTR lpString,
  int nMaxCount,
  UINT uFlag)
{
  ITEM *item;
  LPWSTR text;
  ////// wine Code, seems to be faster.
  TRACE("menu=%p item=%04x ptr=%p len=%d flags=%04x\n", hMenu, uIDItem, lpString, nMaxCount, uFlag );

  if (lpString && nMaxCount) lpString[0] = '\0';

  if (!(item = MENU_FindItem( &hMenu, &uIDItem, uFlag )))
  {
      SetLastError( ERROR_MENU_ITEM_NOT_FOUND);
      return 0;
  }

  text = item->Xlpstr ? DesktopPtrToUser(item->Xlpstr) : NULL;

  if (!text) return 0;
  if (!lpString || !nMaxCount) return WideCharToMultiByte( CP_ACP, 0, text, -1, NULL, 0, NULL, NULL );
  if (!WideCharToMultiByte( CP_ACP, 0, text, -1, lpString, nMaxCount, NULL, NULL ))
      lpString[nMaxCount-1] = 0;
  TRACE("A returning %s\n", lpString);
  return strlen(lpString);
}

/*
 * @implemented
 */
int
WINAPI
GetMenuStringW(
  HMENU hMenu,
  UINT uIDItem,
  LPWSTR lpString,
  int nMaxCount,
  UINT uFlag)
{
  ITEM *item;
  LPWSTR text;

  TRACE("menu=%p item=%04x ptr=%p len=%d flags=%04x\n", hMenu, uIDItem, lpString, nMaxCount, uFlag );

  if (lpString && nMaxCount) lpString[0] = '\0';

  if (!(item = MENU_FindItem( &hMenu, &uIDItem, uFlag )))
  {
      SetLastError( ERROR_MENU_ITEM_NOT_FOUND);
      return 0;
  }

  text = item->Xlpstr ? DesktopPtrToUser(item->Xlpstr) : NULL;

  if (!lpString || !nMaxCount) return text ? strlenW(text) : 0;
  if( !(text))
  {
      lpString[0] = 0;
      return 0;
  }
  lstrcpynW( lpString, text, nMaxCount );
  TRACE("W returning %S\n", lpString);
  return strlenW(lpString);
}

/*
 * @implemented
 */
HMENU
WINAPI
GetSubMenu(
  HMENU hMenu,
  int nPos)
{
  PITEM pItem;
  if (!(pItem = MENU_FindItem( &hMenu, (UINT*)&nPos, MF_BYPOSITION ))) return NULL;

  if (pItem->spSubMenu)
  {
     PMENU pSubMenu = DesktopPtrToUser(pItem->spSubMenu);
     HMENU hsubmenu = UserHMGetHandle(pSubMenu);
     if (IsMenu(hsubmenu)) return hsubmenu;
  }
  return NULL;
}

/*
 * @implemented
 */
HMENU
WINAPI
GetSystemMenu(
  HWND hWnd,
  BOOL bRevert)
{
  return NtUserGetSystemMenu(hWnd, bRevert);
}

/*
 * @implemented
 */
BOOL
WINAPI
InsertMenuA(
  HMENU hMenu,
  UINT uPosition,
  UINT uFlags,
  UINT_PTR uIDNewItem,
  LPCSTR lpNewItem)
{
  MENUITEMINFOW mii;
  UNICODE_STRING UnicodeString;
  BOOL res;

  RtlInitUnicodeString(&UnicodeString, 0);

  MENU_mnu2mnuii( uFlags, uIDNewItem, (LPCWSTR)lpNewItem, &mii, FALSE);

  /* copy the text string, it will be one or the other */
  if (lpNewItem && mii.fMask & MIIM_STRING && !mii.hbmpItem && mii.dwTypeData)
  {
      if (!RtlCreateUnicodeStringFromAsciiz(&UnicodeString, (LPSTR)mii.dwTypeData))
      {
        SetLastError (ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
      }
      mii.dwTypeData = UnicodeString.Buffer;
      mii.cch = UnicodeString.Length / sizeof(WCHAR);
  }
  else
  {
      TRACE("Handle bitmaps\n");
  }
  res = NtUserThunkedMenuItemInfo(hMenu, uPosition, (BOOL)(uFlags & MF_BYPOSITION), TRUE, &mii, &UnicodeString);
  if ( UnicodeString.Buffer ) RtlFreeUnicodeString ( &UnicodeString );
  return res;
}

/*
 * @implemented
 */
BOOL
WINAPI
InsertMenuItemA(
  HMENU hMenu,
  UINT uItem,
  BOOL fByPosition,
  LPCMENUITEMINFOA lpmii)
{
  MENUITEMINFOW mii;
  UNICODE_STRING UnicodeString;
  BOOL res;

  TRACE("hmenu %p, item %04x, by pos %d, info %p\n", hMenu, uItem, fByPosition, lpmii);

  RtlInitUnicodeString(&UnicodeString, 0);

  if (!MENU_NormalizeMenuItemInfoStruct( (const MENUITEMINFOW *)lpmii, &mii )) return FALSE;

  /* copy the text string */
  if (((mii.fMask & MIIM_STRING) ||
      ((mii.fMask & MIIM_TYPE) && (MENU_ITEM_TYPE(mii.fType) == MF_STRING)))
        && mii.dwTypeData && !(GdiValidateHandle((HGDIOBJ)mii.dwTypeData)) )
  {
      if (!RtlCreateUnicodeStringFromAsciiz(&UnicodeString, (LPSTR)mii.dwTypeData))
      {
        SetLastError (ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
      }
      mii.dwTypeData = UnicodeString.Buffer;
      mii.cch = UnicodeString.Length / sizeof(WCHAR);
  }
  else
  {
      TRACE("Handle bitmaps\n");
  }
  res = NtUserThunkedMenuItemInfo(hMenu, uItem, fByPosition, TRUE, &mii, &UnicodeString);
  if ( UnicodeString.Buffer ) RtlFreeUnicodeString ( &UnicodeString );
  return res;
}

/*
 * @implemented
 */
BOOL
WINAPI
InsertMenuItemW(
  HMENU hMenu,
  UINT uItem,
  BOOL fByPosition,
  LPCMENUITEMINFOW lpmii)
{
  MENUITEMINFOW mii;
  UNICODE_STRING MenuText;
  BOOL res = FALSE;

  /* while we could just pass 'lpmii' to win32k, we make a copy so that
     if a bad user passes bad data, we crash his process instead of the
     entire kernel */

  TRACE("hmenu %p, item %04x, by pos %d, info %p\n", hMenu, uItem, fByPosition, lpmii);

  RtlInitUnicodeString(&MenuText, 0);    

  if (!MENU_NormalizeMenuItemInfoStruct( (const MENUITEMINFOW *)lpmii, &mii )) return FALSE;

  /* copy the text string */
  if (((mii.fMask & MIIM_STRING) ||
      ((mii.fMask & MIIM_TYPE) && (MENU_ITEM_TYPE(mii.fType) == MF_STRING)))
        && mii.dwTypeData && !(GdiValidateHandle((HGDIOBJ)mii.dwTypeData)) )
  {
    RtlInitUnicodeString(&MenuText, (PWSTR)lpmii->dwTypeData);
    mii.dwTypeData = MenuText.Buffer;
    mii.cch = MenuText.Length / sizeof(WCHAR);
  }
  res = NtUserThunkedMenuItemInfo(hMenu, uItem, fByPosition, TRUE, &mii, &MenuText);
  return res;
}

/*
 * @implemented
 */
BOOL
WINAPI
InsertMenuW(
  HMENU hMenu,
  UINT uPosition,
  UINT uFlags,
  UINT_PTR uIDNewItem,
  LPCWSTR lpNewItem)
{
  MENUITEMINFOW mii;
  UNICODE_STRING MenuText;
  BOOL res;

  RtlInitUnicodeString(&MenuText, 0);    

  MENU_mnu2mnuii( uFlags, uIDNewItem, lpNewItem, &mii, TRUE);

  /* copy the text string, it will be one or the other */
  if (lpNewItem && mii.fMask & MIIM_STRING && !mii.hbmpItem && mii.dwTypeData)
  {
    RtlInitUnicodeString(&MenuText, (PWSTR)mii.dwTypeData);
    mii.dwTypeData = MenuText.Buffer;
    mii.cch = MenuText.Length / sizeof(WCHAR);
  }
  res = NtUserThunkedMenuItemInfo(hMenu, uPosition, (BOOL)(uFlags & MF_BYPOSITION), TRUE, &mii, &MenuText);
  return res;
}

/*
 * @implemented
 */
BOOL
WINAPI
IsMenu(
  HMENU Menu)
{
  if (ValidateHandle(Menu, TYPE_MENU)) return TRUE;
  return FALSE;
}

/*
 * @implemented
 */
HMENU WINAPI
LoadMenuA(HINSTANCE hInstance,
	  LPCSTR lpMenuName)
{
  HANDLE Resource = FindResourceA(hInstance, lpMenuName, MAKEINTRESOURCEA(4));
  if (Resource == NULL)
    {
      return(NULL);
    }
  return(LoadMenuIndirectA((PVOID)LoadResource(hInstance, Resource)));
}

/*
 * @implemented
 */
HMENU WINAPI
LoadMenuIndirectA(CONST MENUTEMPLATE *lpMenuTemplate)
{
  return(LoadMenuIndirectW(lpMenuTemplate));
}

/*
 * @implemented
 */
HMENU WINAPI
LoadMenuIndirectW(CONST MENUTEMPLATE *lpMenuTemplate)
{
  HMENU hMenu;
  WORD version, offset;
  LPCSTR p = (LPCSTR)lpMenuTemplate;

  version = GET_WORD(p);
  p += sizeof(WORD);

  switch (version)
  {
    case 0: /* standard format is version of 0 */
      offset = GET_WORD(p);
      p += sizeof(WORD) + offset;
      if (!(hMenu = CreateMenu())) return 0;
      if (!MENU_ParseResource(p, hMenu))
      {
        DestroyMenu(hMenu);
        return 0;
      }
      return hMenu;
    case 1: /* extended format is version of 1 */
      offset = GET_WORD(p);
      p += sizeof(WORD) + offset;
      if (!(hMenu = CreateMenu())) return 0;
      if (!MENUEX_ParseResource(p, hMenu))
      {
        DestroyMenu( hMenu );
        return 0;
      }
      return hMenu;
    default:
      ERR("Menu template version %d not supported.\n", version);
      return 0;
  }
}

/*
 * @implemented
 */
HMENU WINAPI
LoadMenuW(HINSTANCE hInstance,
	  LPCWSTR lpMenuName)
{
  HANDLE Resource = FindResourceW(hInstance, lpMenuName, RT_MENU);
  if (Resource == NULL)
    {
      return(NULL);
    }
  return(LoadMenuIndirectW((PVOID)LoadResource(hInstance, Resource)));
}

/*
 * @implemented
 */
int
WINAPI
MenuItemFromPoint(
  HWND hWnd,
  HMENU hMenu,
  POINT ptScreen)
{
  return NtUserMenuItemFromPoint(hWnd, hMenu, ptScreen.x, ptScreen.y);
}

/*
 * @implemented
 */
BOOL
WINAPI
ModifyMenuA(
  HMENU hMenu,
  UINT uPosition,
  UINT uFlags,
  UINT_PTR uIDNewItem,
  LPCSTR lpNewItem)
{
  MENUITEMINFOW mii;
  UNICODE_STRING UnicodeString;
  BOOL res;

  RtlInitUnicodeString(&UnicodeString, 0);

  MENU_mnu2mnuii( uFlags, uIDNewItem, (LPCWSTR)lpNewItem, &mii, FALSE);

  /* copy the text string, it will be one or the other */
  if (lpNewItem && mii.fMask & MIIM_STRING && !mii.hbmpItem && mii.dwTypeData)
  {
      if (!RtlCreateUnicodeStringFromAsciiz(&UnicodeString, (LPSTR)mii.dwTypeData))
      {
        SetLastError (ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
      }
      mii.dwTypeData = UnicodeString.Buffer;
      mii.cch = UnicodeString.Length / sizeof(WCHAR);
  }
  else
  {
      TRACE("Handle bitmaps\n");
  }
  res = NtUserThunkedMenuItemInfo(hMenu, uPosition, (BOOL)(uFlags & MF_BYPOSITION), FALSE, &mii, &UnicodeString);
  if ( UnicodeString.Buffer ) RtlFreeUnicodeString ( &UnicodeString );
  return res;
}

/*
 * @implemented
 */
BOOL
WINAPI
ModifyMenuW(
  HMENU hMenu,
  UINT uPosition,
  UINT uFlags,
  UINT_PTR uIDNewItem,
  LPCWSTR lpNewItem)
{
  MENUITEMINFOW mii;
  UNICODE_STRING MenuText;
  BOOL res;

  RtlInitUnicodeString(&MenuText, 0);    

  MENU_mnu2mnuii( uFlags, uIDNewItem, lpNewItem, &mii, TRUE);

  /* copy the text string, it will be one or the other */
  if (lpNewItem && mii.fMask & MIIM_STRING && !mii.hbmpItem && mii.dwTypeData)
  {
    RtlInitUnicodeString(&MenuText, (PWSTR)mii.dwTypeData);
    mii.dwTypeData = MenuText.Buffer;
    mii.cch = MenuText.Length / sizeof(WCHAR);
  }
  else
  {
      TRACE("Handle bitmaps\n");
  }
  res = NtUserThunkedMenuItemInfo(hMenu, uPosition, (BOOL)(uFlags & MF_BYPOSITION), FALSE, &mii, &MenuText);
  return res;
}

/*
 * @implemented
 */
BOOL WINAPI
SetMenu(HWND hWnd,
	HMENU hMenu)
{
  return NtUserSetMenu(hWnd, hMenu, TRUE);
}

/*
 * @implemented
 */
BOOL
WINAPI
SetMenuInfo(
  HMENU hmenu,
  LPCMENUINFO lpcmi)
{
  ROSMENUINFO mi;
  BOOL res = FALSE;

  if (!lpcmi || (lpcmi->cbSize != sizeof(MENUINFO)))
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return res;
  }

  memcpy(&mi, lpcmi, sizeof(MENUINFO));
  return NtUserThunkedMenuInfo(hmenu, (LPCMENUINFO)&mi);
}

/*
 * @implemented
 */
BOOL
WINAPI
SetMenuItemBitmaps(
  HMENU hMenu,
  UINT uPosition,
  UINT uFlags,
  HBITMAP hBitmapUnchecked,
  HBITMAP hBitmapChecked)
{
  MENUITEMINFOW uItem;
  memset ( &uItem, 0, sizeof(uItem) );
  uItem.cbSize = sizeof(MENUITEMINFOW);
  uItem.fMask = MIIM_CHECKMARKS;
  uItem.hbmpUnchecked = hBitmapUnchecked;
  uItem.hbmpChecked = hBitmapChecked;
  return SetMenuItemInfoW(hMenu, uPosition, (BOOL)(uFlags & MF_BYPOSITION), &uItem);
}

/*
 * @implemented
 */
BOOL
WINAPI
SetMenuItemInfoA(
  HMENU hmenu,
  UINT item,
  BOOL bypos,
  LPCMENUITEMINFOA lpmii)
{
  MENUITEMINFOW mii;
  UNICODE_STRING UnicodeString;
  BOOL Ret;

  TRACE("hmenu %p, item %u, by pos %d, info %p\n", hmenu, item, bypos, lpmii);

  RtlInitUnicodeString(&UnicodeString, 0);

  if (!MENU_NormalizeMenuItemInfoStruct( (const MENUITEMINFOW *)lpmii, &mii )) return FALSE;
/*
 *  MIIM_STRING              == good
 *  MIIM_TYPE & MFT_STRING   == good
 *  MIIM_STRING & MFT_STRING == good
 *  MIIM_STRING & MFT_OWNERDRAW == good
 */
  if (((mii.fMask & MIIM_STRING) ||
      ((mii.fMask & MIIM_TYPE) && (MENU_ITEM_TYPE(mii.fType) == MF_STRING)))
        && mii.dwTypeData && !(GdiValidateHandle((HGDIOBJ)mii.dwTypeData)) )
  {
    /* cch is ignored when the content of a menu item is set by calling SetMenuItemInfo. */
     if (!RtlCreateUnicodeStringFromAsciiz(&UnicodeString, (LPSTR)mii.dwTypeData))
     {
        SetLastError (ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
     }
     mii.dwTypeData = UnicodeString.Buffer;
     mii.cch = UnicodeString.Length / sizeof(WCHAR);
  }
  else
  {
     UnicodeString.Buffer = NULL;
  }
  Ret = NtUserThunkedMenuItemInfo(hmenu, item, bypos, FALSE, &mii, &UnicodeString);
  if (UnicodeString.Buffer != NULL) RtlFreeUnicodeString(&UnicodeString);
  return Ret;
}

/*
 * @implemented
 */
BOOL
WINAPI
SetMenuItemInfoW(
  HMENU hMenu,
  UINT uItem,
  BOOL fByPosition,
  LPCMENUITEMINFOW lpmii)
{
  MENUITEMINFOW MenuItemInfoW;
  UNICODE_STRING UnicodeString;
  BOOL Ret;

  TRACE("hmenu %p, item %u, by pos %d, info %p\n", hMenu, uItem, fByPosition, lpmii);

  RtlInitUnicodeString(&UnicodeString, 0);

  if (!MENU_NormalizeMenuItemInfoStruct( (const MENUITEMINFOW *)lpmii, &MenuItemInfoW )) return FALSE;

  if (((MenuItemInfoW.fMask & MIIM_STRING) ||
      ((MenuItemInfoW.fMask & MIIM_TYPE) &&
                           (MENU_ITEM_TYPE(MenuItemInfoW.fType) == MF_STRING)))
        && MenuItemInfoW.dwTypeData && !(GdiValidateHandle((HGDIOBJ)MenuItemInfoW.dwTypeData)) )
  {
      RtlInitUnicodeString(&UnicodeString, (PCWSTR)MenuItemInfoW.dwTypeData);
      MenuItemInfoW.cch = strlenW(MenuItemInfoW.dwTypeData);
  }
  Ret = NtUserThunkedMenuItemInfo(hMenu, uItem, fByPosition, FALSE, &MenuItemInfoW, &UnicodeString);

  return Ret;
}

/*
 * @implemented
 */
BOOL
WINAPI
SetSystemMenu (
  HWND hwnd,
  HMENU hMenu)
{
  if(!hwnd)
  {
    SetLastError(ERROR_INVALID_WINDOW_HANDLE);
    return FALSE;
  }
  if(!hMenu)
  {
    SetLastError(ERROR_INVALID_MENU_HANDLE);
    return FALSE;
  }
  return NtUserSetSystemMenu(hwnd, hMenu);
}

//
// Example for the Win32/User32 rewrite.
// Def = TrackPopupMenuEx@24=NtUserTrackPopupMenuEx@24
//
//
BOOL
WINAPI
NEWTrackPopupMenu(
  HMENU Menu,
  UINT Flags,
  int x,
  int y,
  int Reserved,
  HWND Wnd,
  CONST RECT *Rect)
{
  return NtUserTrackPopupMenuEx( Menu,
                                Flags,
                                    x,
                                    y,
                                  Wnd,
                                 NULL); // LPTPMPARAMS is null
}

/*
 * @unimplemented
 */
BOOL
WINAPI
MenuWindowProcA(
		HWND   hWnd,
		ULONG_PTR Result,
		UINT   Msg,
		WPARAM wParam,
		LPARAM lParam
		)
{
  if ( Msg < WM_USER)
  {
     LRESULT lResult;
     lResult = PopupMenuWndProcA(hWnd, Msg, wParam, lParam );
     if (Result)
     {
        Result = (ULONG_PTR)lResult;
        return TRUE;
     }
     return FALSE;
  }
  return NtUserMessageCall(hWnd, Msg, wParam, lParam, Result, FNID_MENU, TRUE);
}

/*
 * @unimplemented
 */
BOOL
WINAPI
MenuWindowProcW(
		HWND   hWnd,
		ULONG_PTR Result,
		UINT   Msg,
		WPARAM wParam,
		LPARAM lParam
		)
{
  if ( Msg < WM_USER)
  {
     LRESULT lResult;
     lResult = PopupMenuWndProcW(hWnd, Msg, wParam, lParam );
     if (Result)
     {
        Result = (ULONG_PTR)lResult;
        return TRUE;
     }
     return FALSE;
  }
  return NtUserMessageCall(hWnd, Msg, wParam, lParam, Result, FNID_MENU, FALSE);
}

/*
 * @implemented
 */
BOOL
WINAPI
ChangeMenuW(
    HMENU hMenu,
    UINT cmd,
    LPCWSTR lpszNewItem,
    UINT cmdInsert,
    UINT flags)
{
    /*
        FIXME: Word passes the item id in 'cmd' and 0 or 0xffff as cmdInsert
        for MF_DELETE. We should check the parameters for all others
        MF_* actions also (anybody got a doc on ChangeMenu?).
    */

    switch(flags & (MF_APPEND | MF_DELETE | MF_CHANGE | MF_REMOVE | MF_INSERT))
    {
        case MF_APPEND :
            return AppendMenuW(hMenu, flags &~ MF_APPEND, cmdInsert, lpszNewItem);

        case MF_DELETE :
            return DeleteMenu(hMenu, cmd, flags &~ MF_DELETE);

        case MF_CHANGE :
            return ModifyMenuW(hMenu, cmd, flags &~ MF_CHANGE, cmdInsert, lpszNewItem);

        case MF_REMOVE :
            return RemoveMenu(hMenu, flags & MF_BYPOSITION ? cmd : cmdInsert,
                                flags &~ MF_REMOVE);

        default :   /* MF_INSERT */
            return InsertMenuW(hMenu, cmd, flags, cmdInsert, lpszNewItem);
    };
}

/*
 * @implemented
 */
BOOL
WINAPI
ChangeMenuA(
    HMENU hMenu,
    UINT cmd,
    LPCSTR lpszNewItem,
    UINT cmdInsert,
    UINT flags)
{
    /*
        FIXME: Word passes the item id in 'cmd' and 0 or 0xffff as cmdInsert
        for MF_DELETE. We should check the parameters for all others
        MF_* actions also (anybody got a doc on ChangeMenu?).
    */

    switch(flags & (MF_APPEND | MF_DELETE | MF_CHANGE | MF_REMOVE | MF_INSERT))
    {
        case MF_APPEND :
            return AppendMenuA(hMenu, flags &~ MF_APPEND, cmdInsert, lpszNewItem);

        case MF_DELETE :
            return DeleteMenu(hMenu, cmd, flags &~ MF_DELETE);

        case MF_CHANGE :
            return ModifyMenuA(hMenu, cmd, flags &~ MF_CHANGE, cmdInsert, lpszNewItem);

        case MF_REMOVE :
            return RemoveMenu(hMenu, flags & MF_BYPOSITION ? cmd : cmdInsert,
                                flags &~ MF_REMOVE);

        default :   /* MF_INSERT */
            return InsertMenuA(hMenu, cmd, flags, cmdInsert, lpszNewItem);
    };
}

