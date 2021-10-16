/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Menus
 * FILE:             win32ss/user/ntuser/menu.c
 * PROGRAMER:        Thomas Weidenmueller (w3seek@users.sourceforge.net)
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserMenu);

/* INTERNAL ******************************************************************/

HFONT ghMenuFont = NULL;
HFONT ghMenuFontBold = NULL;
static SIZE MenuCharSize;

/* Use global popup window because there's no way 2 menus can
 * be tracked at the same time.  */
static HWND top_popup = NULL;
static HMENU top_popup_hmenu = NULL;

BOOL fInsideMenuLoop = FALSE;
BOOL fInEndMenu = FALSE;

/* internal popup menu window messages */

#define MM_SETMENUHANDLE        (WM_USER + 0)
#define MM_GETMENUHANDLE        (WM_USER + 1)

/* internal flags for menu tracking */

#define TF_ENDMENU              0x10000
#define TF_SUSPENDPOPUP         0x20000
#define TF_SKIPREMOVE           0x40000


/* maximum allowed depth of any branch in the menu tree.
 * This value is slightly larger than in windows (25) to
 * stay on the safe side. */
#define MAXMENUDEPTH 30

#define MNS_STYLE_MASK (MNS_NOCHECK|MNS_MODELESS|MNS_DRAGDROP|MNS_AUTODISMISS|MNS_NOTIFYBYPOS|MNS_CHECKORBMP)

#define MENUITEMINFO_TYPE_MASK \
                (MFT_STRING | MFT_BITMAP | MFT_OWNERDRAW | MFT_SEPARATOR | \
                 MFT_MENUBARBREAK | MFT_MENUBREAK | MFT_RADIOCHECK | \
                 MFT_RIGHTORDER | MFT_RIGHTJUSTIFY /* same as MF_HELP */ )

#define TYPE_MASK  (MENUITEMINFO_TYPE_MASK | MF_POPUP | MF_SYSMENU)

#define STATE_MASK (~TYPE_MASK)

#define MENUITEMINFO_STATE_MASK (STATE_MASK & ~(MF_BYPOSITION | MF_MOUSESELECT))

#define MII_STATE_MASK (MFS_GRAYED|MFS_CHECKED|MFS_HILITE|MFS_DEFAULT)

#define IS_SYSTEM_MENU(MenuInfo)  \
	(!((MenuInfo)->fFlags & MNF_POPUP) && ((MenuInfo)->fFlags & MNF_SYSMENU))

#define IS_BITMAP_ITEM(flags) (MF_BITMAP == MENU_ITEM_TYPE(flags))

#define IS_MAGIC_BITMAP(id) ((id) && ((INT_PTR)(id) < 12) && ((INT_PTR)(id) >= -1))
#define IS_STRING_ITEM(flags) (MF_STRING == MENU_ITEM_TYPE(flags))

/* Maximum number of menu items a menu can contain */
#define MAX_MENU_ITEMS (0x4000)
#define MAX_GOINTOSUBMENU (0x10)

/* Space between 2 columns */
#define MENU_COL_SPACE 4

#define MENU_ITEM_HBMP_SPACE (5)
#define MENU_BAR_ITEMS_SPACE (12)
#define SEPARATOR_HEIGHT (5)
#define MENU_TAB_SPACE (8)

typedef struct
{
  UINT  TrackFlags;
  PMENU CurrentMenu; /* current submenu (can be equal to hTopMenu)*/
  PMENU TopMenu;     /* initial menu */
  PWND  OwnerWnd;    /* where notifications are sent */
  POINT Pt;
} MTRACKER;

/* Internal MenuTrackMenu() flags */
#define TPM_INTERNAL            0xF0000000
#define TPM_BUTTONDOWN          0x40000000              /* menu was clicked before tracking */
#define TPM_POPUPMENU           0x20000000              /* menu is a popup menu */

#define ITEM_PREV               -1
#define ITEM_NEXT                1

#define UpdateMenuItemState(state, change) \
{\
  if((change) & MF_GRAYED) { \
    (state) |= MF_GRAYED; \
  } else { \
    (state) &= ~MF_GRAYED; \
  } /* Separate the two for test_menu_resource_layout.*/ \
  if((change) & MF_DISABLED) { \
    (state) |= MF_DISABLED; \
  } else { \
    (state) &= ~MF_DISABLED; \
  } \
  if((change) & MFS_CHECKED) { \
    (state) |= MFS_CHECKED; \
  } else { \
    (state) &= ~MFS_CHECKED; \
  } \
  if((change) & MFS_HILITE) { \
    (state) |= MFS_HILITE; \
  } else { \
    (state) &= ~MFS_HILITE; \
  } \
  if((change) & MFS_DEFAULT) { \
    (state) |= MFS_DEFAULT; \
  } else { \
    (state) &= ~MFS_DEFAULT; \
  } \
  if((change) & MF_MOUSESELECT) { \
    (state) |= MF_MOUSESELECT; \
  } else { \
    (state) &= ~MF_MOUSESELECT; \
  } \
}

#if 0
void FASTCALL
DumpMenuItemList(PMENU Menu, PITEM MenuItem)
{
   UINT cnt = 0, i = Menu->cItems;
   while(i)
   {
      if(MenuItem->lpstr.Length)
         DbgPrint(" %d. %wZ\n", ++cnt, &MenuItem->lpstr);
      else
         DbgPrint(" %d. NO TEXT dwTypeData==%d\n", ++cnt, (DWORD)MenuItem->lpstr.Buffer);
      DbgPrint("   fType=");
      if(MFT_BITMAP & MenuItem->fType)
         DbgPrint("MFT_BITMAP ");
      if(MFT_MENUBARBREAK & MenuItem->fType)
         DbgPrint("MFT_MENUBARBREAK ");
      if(MFT_MENUBREAK & MenuItem->fType)
         DbgPrint("MFT_MENUBREAK ");
      if(MFT_OWNERDRAW & MenuItem->fType)
         DbgPrint("MFT_OWNERDRAW ");
      if(MFT_RADIOCHECK & MenuItem->fType)
         DbgPrint("MFT_RADIOCHECK ");
      if(MFT_RIGHTJUSTIFY & MenuItem->fType)
         DbgPrint("MFT_RIGHTJUSTIFY ");
      if(MFT_SEPARATOR & MenuItem->fType)
         DbgPrint("MFT_SEPARATOR ");
      if(MFT_STRING & MenuItem->fType)
         DbgPrint("MFT_STRING ");
      DbgPrint("\n   fState=");
      if(MFS_DISABLED & MenuItem->fState)
         DbgPrint("MFS_DISABLED ");
      else
         DbgPrint("MFS_ENABLED ");
      if(MFS_CHECKED & MenuItem->fState)
         DbgPrint("MFS_CHECKED ");
      else
         DbgPrint("MFS_UNCHECKED ");
      if(MFS_HILITE & MenuItem->fState)
         DbgPrint("MFS_HILITE ");
      else
         DbgPrint("MFS_UNHILITE ");
      if(MFS_DEFAULT & MenuItem->fState)
         DbgPrint("MFS_DEFAULT ");
      if(MFS_GRAYED & MenuItem->fState)
         DbgPrint("MFS_GRAYED ");
      DbgPrint("\n   wId=%d\n", MenuItem->wID);
      MenuItem++;
      i--;
   }
   DbgPrint("Entries: %d\n", cnt);
   return;
}
#endif

#define FreeMenuText(Menu,MenuItem) \
{ \
  if((MENU_ITEM_TYPE((MenuItem)->fType) == MF_STRING) && \
           (MenuItem)->lpstr.Length) { \
    DesktopHeapFree(((PMENU)Menu)->head.rpdesk, (MenuItem)->lpstr.Buffer); \
  } \
}

PMENU FASTCALL
IntGetMenuObject(HMENU hMenu)
{
   PMENU Menu = UserGetMenuObject(hMenu);
   if (Menu)
      Menu->head.cLockObj++;

   return Menu;
}

PMENU FASTCALL VerifyMenu(PMENU pMenu)
{
   HMENU hMenu;
   PITEM pItem;
   ULONG Error;
   UINT i;
   if (!pMenu) return NULL;

   Error = EngGetLastError();

   _SEH2_TRY
   {
      hMenu = UserHMGetHandle(pMenu);
      pItem = pMenu->rgItems;
      if (pItem)
      {
         i = pItem[0].wID;
         pItem[0].wID = i;
      }
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
      ERR("Run away LOOP!\n");
      EngSetLastError(Error);
      _SEH2_YIELD(return NULL);
   }
   _SEH2_END

   if ( UserObjectInDestroy(hMenu))
   {
      ERR("Menu is marked for destruction!\n");
      pMenu = NULL;
   }
   EngSetLastError(Error);
   return pMenu;
}

BOOL
FASTCALL
IntIsMenu(HMENU Menu)
{
  if (UserGetMenuObject(Menu)) return TRUE;
  return FALSE;
}


PMENU WINAPI
IntGetMenu(HWND hWnd)
{
       PWND Wnd = ValidateHwndNoErr(hWnd);

       if (!Wnd)
               return NULL;

       return UserGetMenuObject(UlongToHandle(Wnd->IDMenu));
}

PMENU get_win_sys_menu( HWND hwnd )   
{
   PMENU ret = 0;
   WND *win = ValidateHwndNoErr( hwnd );        
   if (win)
   {
      ret = UserGetMenuObject(win->SystemMenu);
   }
   return ret;
}

BOOL IntDestroyMenu( PMENU pMenu, BOOL bRecurse)
{
    PMENU SubMenu;

    ASSERT(UserIsEnteredExclusive());
    if (pMenu->rgItems) /* recursively destroy submenus */
    {
       int i;
       ITEM *item = pMenu->rgItems;
       for (i = pMenu->cItems; i > 0; i--, item++)
       {
           SubMenu = item->spSubMenu;
           item->spSubMenu = NULL;

           /* Remove Item Text */
           FreeMenuText(pMenu,item);

           /* Remove Item Bitmap and set it for this process */
           if (item->hbmp && !(item->fState & MFS_HBMMENUBMP))
           {
              GreSetObjectOwner(item->hbmp, GDI_OBJ_HMGR_POWNED);
              item->hbmp = NULL;
           }

           /* Remove Item submenu */
           if (bRecurse && SubMenu)//VerifyMenu(SubMenu))
           {
              /* Release submenu since it was referenced when inserted */
              IntReleaseMenuObject(SubMenu);
              IntDestroyMenuObject(SubMenu, bRecurse);
           }
       }
       /* Free the Item */
       DesktopHeapFree(pMenu->head.rpdesk, pMenu->rgItems );
       pMenu->rgItems = NULL;
       pMenu->cItems = 0;
    }
    return TRUE;
}

/* Callback for the object manager */
BOOLEAN
UserDestroyMenuObject(PVOID Object)
{
    return IntDestroyMenuObject(Object, TRUE);
}

BOOL FASTCALL
IntDestroyMenuObject(PMENU Menu, BOOL bRecurse)
{
   ASSERT(UserIsEnteredExclusive());
   if (Menu)
   {
      PWND Window;

      if (PsGetCurrentProcessSessionId() == Menu->head.rpdesk->rpwinstaParent->dwSessionId)
      {
         BOOL ret;
         if (Menu->hWnd)
         {
            Window = ValidateHwndNoErr(Menu->hWnd);
            if (Window)
            {
               //Window->IDMenu = 0; Only in Win9x!! wine win test_SetMenu test...

               /* DestroyMenu should not destroy system menu popup owner */
               if ((Menu->fFlags & (MNF_POPUP | MNF_SYSSUBMENU)) == MNF_POPUP)
               {
                  // Should we check it to see if it has Class?
                  ERR("FIXME Pop up menu window thing'ie\n");
                  //co_UserDestroyWindow( Window );
                  //Menu->hWnd = 0;
               }
            }
         }

         if (!UserMarkObjectDestroy(Menu)) return TRUE;

         /* Remove all menu items */
         IntDestroyMenu( Menu, bRecurse);

         ret = UserDeleteObject(Menu->head.h, TYPE_MENU);
         TRACE("IntDestroyMenuObject %d\n",ret);
         return ret;
      }
   }
   return FALSE;
}

BOOL
MenuInit(VOID)
{
  NONCLIENTMETRICSW ncm;

  /* get the menu font */
  if (!ghMenuFont || !ghMenuFontBold)
  {
    ncm.cbSize = sizeof(ncm);
    if(!UserSystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0))
    {
      ERR("MenuInit(): SystemParametersInfo(SPI_GETNONCLIENTMETRICS) failed!\n");
      return FALSE;
    }

    ghMenuFont = GreCreateFontIndirectW(&ncm.lfMenuFont);
    if (ghMenuFont == NULL)
    {
      ERR("MenuInit(): CreateFontIndirectW(hMenuFont) failed!\n");
      return FALSE;
    }
    ncm.lfMenuFont.lfWeight = min(ncm.lfMenuFont.lfWeight + (FW_BOLD - FW_NORMAL), FW_HEAVY);
    ghMenuFontBold = GreCreateFontIndirectW(&ncm.lfMenuFont);
    if (ghMenuFontBold == NULL)
    {
      ERR("MenuInit(): CreateFontIndirectW(hMenuFontBold) failed!\n");
      GreDeleteObject(ghMenuFont);
      ghMenuFont = NULL;
      return FALSE;
    }
    
    GreSetObjectOwner(ghMenuFont, GDI_OBJ_HMGR_PUBLIC);
    GreSetObjectOwner(ghMenuFontBold, GDI_OBJ_HMGR_PUBLIC);

    co_IntSetupOBM();
  }

  return TRUE;
}


/**********************************************************************
 *		MENU_depth
 *
 * detect if there are loops in the menu tree (or the depth is too large)
 */
int FASTCALL MENU_depth( PMENU pmenu, int depth)
{
    UINT i;
    ITEM *item;
    int subdepth;

    if (!pmenu) return depth;

    depth++;
    if( depth > MAXMENUDEPTH) return depth;
    item = pmenu->rgItems;
    subdepth = depth;
    for( i = 0; i < pmenu->cItems && subdepth <= MAXMENUDEPTH; i++, item++)
    {
        if( item->spSubMenu)//VerifyMenu(item->spSubMenu))
        {
            int bdepth = MENU_depth( item->spSubMenu, depth);
            if( bdepth > subdepth) subdepth = bdepth;
        }
        if( subdepth > MAXMENUDEPTH)
            TRACE("<- hmenu %p\n", item->spSubMenu);
    }
    return subdepth;
}


/******************************************************************************
 *
 *   UINT MenuGetStartOfNextColumn(
 *     PMENU Menu)
 *
 *****************************************************************************/

static UINT  MENU_GetStartOfNextColumn(
    PMENU  menu )
{
    PITEM pItem;
    UINT i;

    if(!menu)
	return NO_SELECTED_ITEM;

    i = menu->iItem + 1;
    if( i == NO_SELECTED_ITEM )
	return i;

    pItem = menu->rgItems;
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
 *     PMENU Menu)
 *
 *****************************************************************************/
static UINT  MENU_GetStartOfPrevColumn(
    PMENU  menu )
{
    UINT  i;
    PITEM pItem;

    if( !menu )
	return NO_SELECTED_ITEM;

    if( menu->iItem == 0 || menu->iItem == NO_SELECTED_ITEM )
	return NO_SELECTED_ITEM;

    pItem = menu->rgItems;
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
 *           MENU_FindItem
 *
 * Find a menu item. Return a pointer on the item, and modifies *hmenu
 * in case the item was in a sub-menu.
 */
PITEM FASTCALL MENU_FindItem( PMENU *pmenu, UINT *nPos, UINT wFlags )
{
    MENU *menu = *pmenu;
    ITEM *fallback = NULL;
    UINT fallback_pos = 0;
    UINT i;

    if (!menu) return NULL;

    if (wFlags & MF_BYPOSITION)
    {
        if (!menu->cItems) return NULL;
	if (*nPos >= menu->cItems) return NULL;
	return &menu->rgItems[*nPos];
    }
    else
    {
        PITEM item = menu->rgItems;
	for (i = 0; i < menu->cItems; i++, item++)
	{
	    if (item->spSubMenu)
	    {
		PMENU psubmenu = item->spSubMenu;//VerifyMenu(item->spSubMenu);
		PITEM subitem = MENU_FindItem( &psubmenu, nPos, wFlags );
		if (subitem)
		{
		    *pmenu = psubmenu;
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
 *           MenuFindSubMenu
 *
 * Find a Sub menu. Return the position of the submenu, and modifies
 * *hmenu in case it is found in another sub-menu.
 * If the submenu cannot be found, NO_SELECTED_ITEM is returned.
 */
static UINT FASTCALL MENU_FindSubMenu(PMENU *menu, PMENU SubTarget )
{
    UINT i;
    PITEM item;

    item = ((PMENU)*menu)->rgItems;
    for (i = 0; i < ((PMENU)*menu)->cItems; i++, item++)
    {
        if (!item->spSubMenu)
           continue;
        else
        {
           if (item->spSubMenu == SubTarget)
           {
              return i;
           }
           else 
           {
              PMENU pSubMenu = item->spSubMenu;
              UINT pos = MENU_FindSubMenu( &pSubMenu, SubTarget );
              if (pos != NO_SELECTED_ITEM)
              {
                  *menu = pSubMenu;
                  return pos;
              }
           }
        }
    }
    return NO_SELECTED_ITEM;
}

BOOL FASTCALL
IntRemoveMenuItem( PMENU pMenu, UINT nPos, UINT wFlags, BOOL bRecurse )
{
    PITEM item;
    PITEM newItems;

    TRACE("(menu=%p pos=%04x flags=%04x)\n",pMenu, nPos, wFlags);
    if (!(item = MENU_FindItem( &pMenu, &nPos, wFlags ))) return FALSE;

    /* Remove item */

    FreeMenuText(pMenu,item);
    if (bRecurse && item->spSubMenu)
    {
       IntDestroyMenuObject(item->spSubMenu, bRecurse);
    }
    ////// Use cAlloced with inc's of 8's....
    if (--pMenu->cItems == 0)
    {
        DesktopHeapFree(pMenu->head.rpdesk, pMenu->rgItems );
        pMenu->rgItems = NULL;
    }
    else
    {
        while (nPos < pMenu->cItems)
        {
            *item = *(item+1);
            item++;
            nPos++;
        }
        newItems = DesktopHeapReAlloc(pMenu->head.rpdesk, pMenu->rgItems, pMenu->cItems * sizeof(ITEM));
        if (newItems)
        {
            pMenu->rgItems = newItems;
        }
    }
    return TRUE;
}

/**********************************************************************
 *         MENU_InsertItem
 *
 * Insert (allocate) a new item into a menu.
 */
ITEM *MENU_InsertItem( PMENU menu, UINT pos, UINT flags, PMENU *submenu, UINT *npos )
{
    ITEM *newItems;

    /* Find where to insert new item */

    if (flags & MF_BYPOSITION) {
        if (pos > menu->cItems)
            pos = menu->cItems;
    } else {
        if (!MENU_FindItem( &menu, &pos, flags ))
        {
            if (submenu) *submenu = menu;
            if (npos) *npos = pos;
            pos = menu->cItems;
        }
    }

    /* Make sure that MDI system buttons stay on the right side.
     * Note: XP treats only bitmap handles 1 - 6 as "magic" ones
     * regardless of their id.
     */
    while ( pos > 0 &&
           (INT_PTR)menu->rgItems[pos - 1].hbmp >= (INT_PTR)HBMMENU_SYSTEM &&
           (INT_PTR)menu->rgItems[pos - 1].hbmp <= (INT_PTR)HBMMENU_MBAR_CLOSE_D)
        pos--;

    TRACE("inserting at %u flags %x\n", pos, flags);

    /* Create new items array */

    newItems = DesktopHeapAlloc(menu->head.rpdesk, sizeof(ITEM) * (menu->cItems+1) );
    if (!newItems)
    {
        WARN("allocation failed\n" );
        return NULL;
    }
    if (menu->cItems > 0)
    {
       /* Copy the old array into the new one */
       if (pos > 0) RtlCopyMemory( newItems, menu->rgItems, pos * sizeof(ITEM) );
       if (pos < menu->cItems) RtlCopyMemory( &newItems[pos+1], &menu->rgItems[pos], (menu->cItems-pos)*sizeof(ITEM) );
       DesktopHeapFree(menu->head.rpdesk, menu->rgItems );
    }
    menu->rgItems = newItems;
    menu->cItems++;
    RtlZeroMemory( &newItems[pos], sizeof(*newItems) );
    menu->cyMenu = 0; /* force size recalculate */
    return &newItems[pos];
}

BOOL FASTCALL
IntInsertMenuItem(
    _In_ PMENU MenuObject,
    UINT uItem,
    BOOL fByPosition,
    PROSMENUITEMINFO ItemInfo,
    PUNICODE_STRING lpstr)
{
   PITEM MenuItem;
   PMENU SubMenu = NULL;

   NT_ASSERT(MenuObject != NULL);

   if (MAX_MENU_ITEMS <= MenuObject->cItems)
   {
      EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
      return FALSE;
   }

   SubMenu = MenuObject;

   if(!(MenuItem = MENU_InsertItem( SubMenu, uItem, fByPosition ? MF_BYPOSITION : MF_BYCOMMAND, &SubMenu, &uItem ))) return FALSE;

   if(!IntSetMenuItemInfo(SubMenu, MenuItem, ItemInfo, lpstr))
   {
      IntRemoveMenuItem(SubMenu, uItem, fByPosition ? MF_BYPOSITION : MF_BYCOMMAND, FALSE);
      return FALSE;
   }

   /* Force size recalculation! */
   SubMenu->cyMenu = 0;
   MenuItem->hbmpChecked = MenuItem->hbmpUnchecked = 0;

   TRACE("IntInsertMenuItemToList = %u %i\n", uItem, (BOOL)((INT)uItem >= 0));

   return TRUE;
}

PMENU FASTCALL
IntCreateMenu(
    _Out_ PHANDLE Handle,
    _In_ BOOL IsMenuBar,
    _In_ PDESKTOP Desktop,
    _In_ PPROCESSINFO ppi)
{
   PMENU Menu;

   Menu = (PMENU)UserCreateObject( gHandleTable,
                                          Desktop,
                                          ppi->ptiList,
                                          Handle,
                                          TYPE_MENU,
                                          sizeof(MENU));
   if(!Menu)
   {
      *Handle = 0;
      return NULL;
   }

   Menu->cyMax = 0; /* Default */
   Menu->hbrBack = NULL; /* No brush */
   Menu->dwContextHelpId = 0; /* Default */
   Menu->dwMenuData = 0; /* Default */
   Menu->iItem = NO_SELECTED_ITEM; // Focused item
   Menu->fFlags = (IsMenuBar ? 0 : MNF_POPUP);
   Menu->spwndNotify = NULL;
   Menu->cyMenu = 0; // Height
   Menu->cxMenu = 0; // Width
   Menu->cItems = 0; // Item count
   Menu->iTop = 0;
   Menu->iMaxTop = 0;
   Menu->cxTextAlign = 0;
   Menu->rgItems = NULL;

   Menu->hWnd = NULL;
   Menu->TimeToHide = FALSE;

   return Menu;
}

BOOL FASTCALL
IntCloneMenuItems(PMENU Destination, PMENU Source)
{
   PITEM MenuItem, NewMenuItem = NULL;
   UINT i;

   if(!Source->cItems)
      return FALSE;

   NewMenuItem = DesktopHeapAlloc(Destination->head.rpdesk, Source->cItems * sizeof(ITEM));
   if(!NewMenuItem) return FALSE;

   RtlZeroMemory(NewMenuItem, Source->cItems * sizeof(ITEM));

   Destination->rgItems = NewMenuItem;

   MenuItem = Source->rgItems;
   for (i = 0; i < Source->cItems; i++, MenuItem++, NewMenuItem++)
   {
      NewMenuItem->fType = MenuItem->fType;
      NewMenuItem->fState = MenuItem->fState;
      NewMenuItem->wID = MenuItem->wID;
      NewMenuItem->spSubMenu = MenuItem->spSubMenu;
      NewMenuItem->hbmpChecked = MenuItem->hbmpChecked;
      NewMenuItem->hbmpUnchecked = MenuItem->hbmpUnchecked;
      NewMenuItem->dwItemData = MenuItem->dwItemData;
      if (MenuItem->lpstr.Length)
      {
         NewMenuItem->lpstr.Length = 0;
         NewMenuItem->lpstr.MaximumLength = MenuItem->lpstr.MaximumLength;
         NewMenuItem->lpstr.Buffer = DesktopHeapAlloc(Destination->head.rpdesk, MenuItem->lpstr.MaximumLength);
         if (!NewMenuItem->lpstr.Buffer)
         {
             DesktopHeapFree(Destination->head.rpdesk, NewMenuItem);
             break;
         }
         RtlCopyUnicodeString(&NewMenuItem->lpstr, &MenuItem->lpstr);
         NewMenuItem->lpstr.Buffer[MenuItem->lpstr.Length / sizeof(WCHAR)] = 0;
         NewMenuItem->Xlpstr = NewMenuItem->lpstr.Buffer;
      }
      else
      {
         NewMenuItem->lpstr.Buffer = MenuItem->lpstr.Buffer;
         NewMenuItem->Xlpstr = NewMenuItem->lpstr.Buffer;
      }
      NewMenuItem->hbmp = MenuItem->hbmp;
      Destination->cItems = i + 1;
   }
   return TRUE;
}

PMENU FASTCALL
IntCloneMenu(PMENU Source)
{
   HANDLE hMenu;
   PMENU Menu;

   if(!Source)
      return NULL;

   /* A menu is valid process wide. We can pass to the object manager any thread ptr */
   Menu = (PMENU)UserCreateObject( gHandleTable,
                                   Source->head.rpdesk,
                                   ((PPROCESSINFO)Source->head.hTaskWow)->ptiList,
                                   &hMenu,
                                   TYPE_MENU,
                                   sizeof(MENU));
   if(!Menu)
      return NULL;

   Menu->fFlags = Source->fFlags;
   Menu->cyMax = Source->cyMax;
   Menu->hbrBack = Source->hbrBack;
   Menu->dwContextHelpId = Source->dwContextHelpId;
   Menu->dwMenuData = Source->dwMenuData;
   Menu->iItem = NO_SELECTED_ITEM;
   Menu->spwndNotify = NULL;
   Menu->cyMenu = 0;
   Menu->cxMenu = 0;
   Menu->cItems = 0;
   Menu->iTop = 0;
   Menu->iMaxTop = 0;
   Menu->cxTextAlign = 0;
   Menu->rgItems = NULL;

   Menu->hWnd = NULL;
   Menu->TimeToHide = FALSE;

   IntCloneMenuItems(Menu, Source);

   return Menu;
}

BOOL FASTCALL
IntSetMenuFlagRtoL(PMENU Menu)
{
   ERR("SetMenuFlagRtoL\n");
   Menu->fFlags |= MNF_RTOL;
   return TRUE;
}

BOOL FASTCALL
IntSetMenuContextHelpId(PMENU Menu, DWORD dwContextHelpId)
{
   Menu->dwContextHelpId = dwContextHelpId;
   return TRUE;
}

BOOL FASTCALL
IntGetMenuInfo(PMENU Menu, PROSMENUINFO lpmi)
{
   if(lpmi->fMask & MIM_BACKGROUND)
      lpmi->hbrBack = Menu->hbrBack;
   if(lpmi->fMask & MIM_HELPID)
      lpmi->dwContextHelpID = Menu->dwContextHelpId;
   if(lpmi->fMask & MIM_MAXHEIGHT)
      lpmi->cyMax = Menu->cyMax;
   if(lpmi->fMask & MIM_MENUDATA)
      lpmi->dwMenuData = Menu->dwMenuData;
   if(lpmi->fMask & MIM_STYLE)
      lpmi->dwStyle = Menu->fFlags & MNS_STYLE_MASK;

   if (sizeof(MENUINFO) < lpmi->cbSize)
   {
     lpmi->cItems = Menu->cItems;

     lpmi->iItem = Menu->iItem;
     lpmi->cxMenu = Menu->cxMenu;
     lpmi->cyMenu = Menu->cyMenu;
     lpmi->spwndNotify = Menu->spwndNotify;
     lpmi->cxTextAlign = Menu->cxTextAlign;
     lpmi->iTop = Menu->iTop;
     lpmi->iMaxTop = Menu->iMaxTop;
     lpmi->dwArrowsOn = Menu->dwArrowsOn;

     lpmi->fFlags = Menu->fFlags;
     lpmi->Self = Menu->head.h;
     lpmi->TimeToHide = Menu->TimeToHide;
     lpmi->Wnd = Menu->hWnd;
   }
   return TRUE;
}

BOOL FASTCALL
IntSetMenuInfo(PMENU Menu, PROSMENUINFO lpmi)
{
   if(lpmi->fMask & MIM_BACKGROUND)
      Menu->hbrBack = lpmi->hbrBack;
   if(lpmi->fMask & MIM_HELPID)
      Menu->dwContextHelpId = lpmi->dwContextHelpID;
   if(lpmi->fMask & MIM_MAXHEIGHT)
      Menu->cyMax = lpmi->cyMax;
   if(lpmi->fMask & MIM_MENUDATA)
      Menu->dwMenuData = lpmi->dwMenuData;
   if(lpmi->fMask & MIM_STYLE)
      Menu->fFlags ^= (Menu->fFlags ^ lpmi->dwStyle) & MNS_STYLE_MASK;
   if(lpmi->fMask & MIM_APPLYTOSUBMENUS)
   {
      int i;
      PITEM item = Menu->rgItems;
      for ( i = Menu->cItems; i; i--, item++)
      {
         if ( item->spSubMenu )
         {
            IntSetMenuInfo( item->spSubMenu, lpmi);
         }
      }
   }
   if (sizeof(MENUINFO) < lpmi->cbSize)
   {
      Menu->iItem = lpmi->iItem;
      Menu->cyMenu = lpmi->cyMenu;
      Menu->cxMenu = lpmi->cxMenu;
      Menu->spwndNotify = lpmi->spwndNotify;
      Menu->cxTextAlign = lpmi->cxTextAlign;
      Menu->iTop = lpmi->iTop;
      Menu->iMaxTop = lpmi->iMaxTop;
      Menu->dwArrowsOn = lpmi->dwArrowsOn;

      Menu->TimeToHide = lpmi->TimeToHide;
      Menu->hWnd = lpmi->Wnd;
   }
   if ( lpmi->fMask & MIM_STYLE)
   {
      if (lpmi->dwStyle & MNS_AUTODISMISS) FIXME("MNS_AUTODISMISS unimplemented wine\n");
      if (lpmi->dwStyle & MNS_DRAGDROP)    FIXME("MNS_DRAGDROP unimplemented wine\n");
      if (lpmi->dwStyle & MNS_MODELESS)    FIXME("MNS_MODELESS unimplemented wine\n");
   }
   return TRUE;
}

BOOL FASTCALL
IntGetMenuItemInfo(PMENU Menu, /* UNUSED PARAM!! */
                   PITEM MenuItem, PROSMENUITEMINFO lpmii)
{
   NTSTATUS Status;

   if(lpmii->fMask & (MIIM_FTYPE | MIIM_TYPE))
   {
      lpmii->fType = MenuItem->fType;
   }
   if(lpmii->fMask & MIIM_BITMAP)
   {
      lpmii->hbmpItem = MenuItem->hbmp;
   }
   if(lpmii->fMask & MIIM_CHECKMARKS)
   {
      lpmii->hbmpChecked = MenuItem->hbmpChecked;
      lpmii->hbmpUnchecked = MenuItem->hbmpUnchecked;
   }
   if(lpmii->fMask & MIIM_DATA)
   {
      lpmii->dwItemData = MenuItem->dwItemData;
   }
   if(lpmii->fMask & MIIM_ID)
   {
      lpmii->wID = MenuItem->wID;
   }
   if(lpmii->fMask & MIIM_STATE)
   {
      lpmii->fState = MenuItem->fState;
   }
   if(lpmii->fMask & MIIM_SUBMENU)
   {
      lpmii->hSubMenu = MenuItem->spSubMenu ? MenuItem->spSubMenu->head.h : NULL;
   }

   if ((lpmii->fMask & MIIM_STRING) ||
      ((lpmii->fMask & MIIM_TYPE) && (MENU_ITEM_TYPE(lpmii->fType) == MF_STRING)))
   {
      if (lpmii->dwTypeData == NULL)
      {
         lpmii->cch = MenuItem->lpstr.Length / sizeof(WCHAR);
      }
      else
      {  //// lpmii->lpstr can be read in user mode!!!!
         Status = MmCopyToCaller(lpmii->dwTypeData, MenuItem->lpstr.Buffer,
                                 min(lpmii->cch * sizeof(WCHAR),
                                     MenuItem->lpstr.MaximumLength));
         if (! NT_SUCCESS(Status))
         {
            SetLastNtError(Status);
            return FALSE;
         }
      }
   }

   if (sizeof(ROSMENUITEMINFO) == lpmii->cbSize)
   {
      lpmii->Rect.left   = MenuItem->xItem;
      lpmii->Rect.top    = MenuItem->yItem;
      lpmii->Rect.right  = MenuItem->cxItem; // Do this for now......
      lpmii->Rect.bottom = MenuItem->cyItem;
      lpmii->dxTab = MenuItem->dxTab;
      lpmii->lpstr = MenuItem->lpstr.Buffer;
      lpmii->maxBmpSize.cx = MenuItem->cxBmp;
      lpmii->maxBmpSize.cy = MenuItem->cyBmp;
   }

   return TRUE;
}

BOOL FASTCALL
IntSetMenuItemInfo(PMENU MenuObject, PITEM MenuItem, PROSMENUITEMINFO lpmii, PUNICODE_STRING lpstr)
{
   PMENU SubMenuObject;
   BOOL circref = FALSE;

   if(!MenuItem || !MenuObject || !lpmii)
   {
      return FALSE;
   }
   if ( lpmii->fMask & MIIM_FTYPE )
   {
      MenuItem->fType &= ~MENUITEMINFO_TYPE_MASK;
      MenuItem->fType |= lpmii->fType & MENUITEMINFO_TYPE_MASK;
   }
   if (lpmii->fMask & MIIM_TYPE)
   {
      #if 0 //// Done in User32.
      if (lpmii->fMask & ( MIIM_STRING | MIIM_FTYPE | MIIM_BITMAP))
      {
         ERR("IntSetMenuItemInfo: Invalid combination of fMask bits used\n");
         KeRosDumpStackFrames(NULL, 20);
         /* This does not happen on Win9x/ME */
         SetLastNtError( ERROR_INVALID_PARAMETER);
         return FALSE;
      }
      #endif
      /*
       * Delete the menu item type when changing type from
       * MF_STRING.
       */
      if (MenuItem->fType != lpmii->fType &&
            MENU_ITEM_TYPE(MenuItem->fType) == MFT_STRING)
      {
         FreeMenuText(MenuObject,MenuItem);
         RtlInitUnicodeString(&MenuItem->lpstr, NULL);
         MenuItem->Xlpstr = NULL;
      }
      if(lpmii->fType & MFT_BITMAP)
      {
         if(lpmii->hbmpItem)
           MenuItem->hbmp = lpmii->hbmpItem;
         else
         { /* Win 9x/Me stuff */
           MenuItem->hbmp = (HBITMAP)((ULONG_PTR)(LOWORD(lpmii->dwTypeData)));
         }
         lpmii->dwTypeData = 0;
      }
   }
   if(lpmii->fMask & MIIM_BITMAP)
   {
      MenuItem->hbmp = lpmii->hbmpItem;
      if (MenuItem->hbmp <= HBMMENU_POPUP_MINIMIZE && MenuItem->hbmp >= HBMMENU_CALLBACK)
         MenuItem->fState |= MFS_HBMMENUBMP;
      else
         MenuItem->fState &= ~MFS_HBMMENUBMP;
   }
   if(lpmii->fMask & MIIM_CHECKMARKS)
   {
      MenuItem->hbmpChecked = lpmii->hbmpChecked;
      MenuItem->hbmpUnchecked = lpmii->hbmpUnchecked;
   }
   if(lpmii->fMask & MIIM_DATA)
   {
      MenuItem->dwItemData = lpmii->dwItemData;
   }
   if(lpmii->fMask & MIIM_ID)
   {
      MenuItem->wID = lpmii->wID;
   }
   if(lpmii->fMask & MIIM_STATE)
   {
      /* Remove MFS_DEFAULT flag from all other menu items if this item
         has the MFS_DEFAULT state */
      if(lpmii->fState & MFS_DEFAULT)
         UserSetMenuDefaultItem(MenuObject, -1, 0);
      /* Update the menu item state flags */
      UpdateMenuItemState(MenuItem->fState, lpmii->fState);
   }

   if(lpmii->fMask & MIIM_SUBMENU)
   {
      if (lpmii->hSubMenu)
      {
         SubMenuObject = UserGetMenuObject(lpmii->hSubMenu);
         if ( SubMenuObject && !(UserObjectInDestroy(lpmii->hSubMenu)) )
         {
            //// wine Bug 12171 : Adding Popup Menu to itself! Could create endless loops.
            //// CORE-7967.
            if (MenuObject == SubMenuObject)
            {
               HANDLE hMenu;
               ERR("Pop Up Menu Double Trouble!\n");
               SubMenuObject = IntCreateMenu(&hMenu,
                   FALSE,
                   MenuObject->head.rpdesk,
                   (PPROCESSINFO)MenuObject->head.hTaskWow); // It will be marked.
               if (!SubMenuObject) return FALSE;
               IntReleaseMenuObject(SubMenuObject); // This will be referenced again after insertion.
               circref = TRUE;
            }
            if ( MENU_depth( SubMenuObject, 0) > MAXMENUDEPTH )
            {
               ERR( "Loop detected in menu hierarchy or maximum menu depth exceeded!\n");
               if (circref) IntDestroyMenuObject(SubMenuObject, FALSE);
               return FALSE;
            }
            /* Make sure the submenu is marked as a popup menu */
            SubMenuObject->fFlags |= MNF_POPUP;
            // Now fix the test_subpopup_locked_by_menu tests....
            if (MenuItem->spSubMenu) IntReleaseMenuObject(MenuItem->spSubMenu);
            MenuItem->spSubMenu = SubMenuObject;
            UserReferenceObject(SubMenuObject);
         }
         else
         {
            EngSetLastError( ERROR_INVALID_PARAMETER);
            return FALSE;
         }
      }
      else
      {  // If submenu just dereference it.
         if (MenuItem->spSubMenu) IntReleaseMenuObject(MenuItem->spSubMenu);
         MenuItem->spSubMenu = NULL;
      }
   }

   if ((lpmii->fMask & MIIM_STRING) ||
      ((lpmii->fMask & MIIM_TYPE) && (MENU_ITEM_TYPE(lpmii->fType) == MF_STRING)))
   {
      /* free the string when used */
      FreeMenuText(MenuObject,MenuItem);
      RtlInitUnicodeString(&MenuItem->lpstr, NULL);
      MenuItem->Xlpstr = NULL;

      if(lpmii->dwTypeData && lpmii->cch && lpstr && lpstr->Buffer)
      {
         UNICODE_STRING Source;

         if (!NT_VERIFY(lpmii->cch <= UNICODE_STRING_MAX_CHARS))
         {
             return FALSE;
         }

         Source.Length = Source.MaximumLength = (USHORT)(lpmii->cch * sizeof(WCHAR));
         Source.Buffer = lpmii->dwTypeData;

         MenuItem->lpstr.Buffer = DesktopHeapAlloc( MenuObject->head.rpdesk, Source.Length + sizeof(WCHAR));
         if(MenuItem->lpstr.Buffer != NULL)
         {
            MenuItem->lpstr.Length = 0;
            MenuItem->lpstr.MaximumLength = Source.Length + sizeof(WCHAR);
            RtlCopyUnicodeString(&MenuItem->lpstr, &Source);
            MenuItem->lpstr.Buffer[MenuItem->lpstr.Length / sizeof(WCHAR)] = 0;

            MenuItem->cch = MenuItem->lpstr.Length / sizeof(WCHAR);
            MenuItem->Xlpstr = (USHORT*)MenuItem->lpstr.Buffer;
         }
      }
   }

   if( !(MenuObject->fFlags & MNF_SYSMENU) &&
       !MenuItem->Xlpstr &&
       !lpmii->dwTypeData &&
       !(MenuItem->fType & MFT_OWNERDRAW) &&
       !MenuItem->hbmp)
      MenuItem->fType |= MFT_SEPARATOR;

   if (sizeof(ROSMENUITEMINFO) == lpmii->cbSize)
   {
      MenuItem->xItem  = lpmii->Rect.left;
      MenuItem->yItem  = lpmii->Rect.top;
      MenuItem->cxItem = lpmii->Rect.right; // Do this for now......
      MenuItem->cyItem = lpmii->Rect.bottom;
      MenuItem->dxTab = lpmii->dxTab;
      lpmii->lpstr = MenuItem->lpstr.Buffer; /* Send back new allocated string or zero */
      MenuItem->cxBmp = lpmii->maxBmpSize.cx;
      MenuItem->cyBmp = lpmii->maxBmpSize.cy;
   }

   return TRUE;
}


UINT FASTCALL
IntEnableMenuItem(PMENU MenuObject, UINT uIDEnableItem, UINT uEnable)
{
   PITEM MenuItem;
   UINT res;

   if (!(MenuItem = MENU_FindItem( &MenuObject, &uIDEnableItem, uEnable ))) return (UINT)-1;

   res = MenuItem->fState & (MF_GRAYED | MF_DISABLED);

   MenuItem->fState ^= (res ^ uEnable) & (MF_GRAYED | MF_DISABLED);

   /* If the close item in the system menu change update the close button */
   if (res != uEnable)
   {
      switch (MenuItem->wID) // More than just close.
      {
        case SC_CLOSE:
        case SC_MAXIMIZE:
        case SC_MINIMIZE:
        case SC_MOVE:
        case SC_RESTORE:
        case SC_SIZE:
	if (MenuObject->fFlags & MNF_SYSSUBMENU && MenuObject->spwndNotify != 0)
	{
            //RECTL rc = MenuObject->spwndNotify->rcWindow;

            /* Refresh the frame to reflect the change */
            //IntMapWindowPoints(0, MenuObject->spwndNotify, (POINT *)&rc, 2);
            //rc.bottom = 0;
            //co_UserRedrawWindow(MenuObject->spwndNotify, &rc, 0, RDW_FRAME | RDW_INVALIDATE | RDW_NOCHILDREN);

            // Allow UxTheme!
            UserPaintCaption(MenuObject->spwndNotify, DC_BUTTONS);
	}
	default:
           break;
      }
   }
   return res;
}

DWORD FASTCALL
IntCheckMenuItem(PMENU MenuObject, UINT uIDCheckItem, UINT uCheck)
{
   PITEM MenuItem;
   DWORD res;

   if (!(MenuItem = MENU_FindItem( &MenuObject, &uIDCheckItem, uCheck ))) return -1;

   res = (DWORD)(MenuItem->fState & MF_CHECKED);

   MenuItem->fState ^= (res ^ uCheck) & MF_CHECKED;

   return res;
}

BOOL FASTCALL
UserSetMenuDefaultItem(PMENU MenuObject, UINT uItem, UINT fByPos)
{
   UINT i;
   PITEM MenuItem = MenuObject->rgItems;

   if (!MenuItem) return FALSE;

   /* reset all default-item flags */
   for (i = 0; i < MenuObject->cItems; i++, MenuItem++)
   {
       MenuItem->fState &= ~MFS_DEFAULT;
   }

   /* no default item */
   if(uItem == (UINT)-1)
   {
      return TRUE;
   }
   MenuItem = MenuObject->rgItems;
   if ( fByPos )
   {
      if ( uItem >= MenuObject->cItems ) return FALSE;
         MenuItem[uItem].fState |= MFS_DEFAULT;
      return TRUE;
   }
   else
   {
      for (i = 0; i < MenuObject->cItems; i++, MenuItem++)
      {
          if (MenuItem->wID == uItem)
          {
             MenuItem->fState |= MFS_DEFAULT;
             return TRUE;
          }
      }

   }
   return FALSE;
}

UINT FASTCALL
IntGetMenuDefaultItem(PMENU MenuObject, UINT fByPos, UINT gmdiFlags, DWORD *gismc)
{
   UINT i = 0;
   PITEM MenuItem = MenuObject->rgItems;

   /* empty menu */
   if (!MenuItem) return -1;

   while ( !( MenuItem->fState & MFS_DEFAULT ) )
   {
      i++; MenuItem++;
      if  (i >= MenuObject->cItems ) return -1;
   }

   /* default: don't return disabled items */
   if ( (!(GMDI_USEDISABLED & gmdiFlags)) && (MenuItem->fState & MFS_DISABLED )) return -1;

   /* search rekursiv when needed */
   if ( (gmdiFlags & GMDI_GOINTOPOPUPS) && MenuItem->spSubMenu )
   {
      UINT ret;
      (*gismc)++;
      ret = IntGetMenuDefaultItem( MenuItem->spSubMenu, fByPos, gmdiFlags, gismc );
      (*gismc)--;
      if ( -1 != ret ) return ret;

      /* when item not found in submenu, return the popup item */
   }
   return ( fByPos ) ? i : MenuItem->wID;
}

PMENU
FASTCALL
co_IntGetSubMenu(
  PMENU pMenu,
  int nPos)
{
  PITEM pItem;
  if (!(pItem = MENU_FindItem( &pMenu, (UINT*)&nPos, MF_BYPOSITION ))) return NULL;
  return pItem->spSubMenu;
}

/***********************************************************************
 *           MenuInitSysMenuPopup
 *
 * Grey the appropriate items in System menu.
 */
void FASTCALL MENU_InitSysMenuPopup(PMENU menu, DWORD style, DWORD clsStyle, LONG HitTest )
{
    BOOL gray;
    UINT DefItem;

    gray = !(style & WS_THICKFRAME) || (style & (WS_MAXIMIZE | WS_MINIMIZE));
    IntEnableMenuItem( menu, SC_SIZE, (gray ? MF_GRAYED : MF_ENABLED) );
    gray = ((style & WS_MAXIMIZE) != 0);
    IntEnableMenuItem( menu, SC_MOVE, (gray ? MF_GRAYED : MF_ENABLED) );
    gray = !(style & WS_MINIMIZEBOX) || (style & WS_MINIMIZE);
    IntEnableMenuItem( menu, SC_MINIMIZE, (gray ? MF_GRAYED : MF_ENABLED) );
    gray = !(style & WS_MAXIMIZEBOX) || (style & WS_MAXIMIZE);
    IntEnableMenuItem( menu, SC_MAXIMIZE, (gray ? MF_GRAYED : MF_ENABLED) );
    gray = !(style & (WS_MAXIMIZE | WS_MINIMIZE));
    IntEnableMenuItem( menu, SC_RESTORE, (gray ? MF_GRAYED : MF_ENABLED) );
    gray = (clsStyle & CS_NOCLOSE) != 0;

    /* The menu item must keep its state if it's disabled */
    if(gray)
        IntEnableMenuItem( menu, SC_CLOSE, MF_GRAYED);

    /* Set default menu item */
    if(style & WS_MINIMIZE) DefItem = SC_RESTORE;
    else if(HitTest == HTCAPTION) DefItem = ((style & (WS_MAXIMIZE | WS_MINIMIZE)) ? SC_RESTORE : SC_MAXIMIZE);
    else DefItem = SC_CLOSE;

    UserSetMenuDefaultItem(menu, DefItem, MF_BYCOMMAND);
}


/***********************************************************************
 *           MenuDrawPopupGlyph
 *
 * Draws popup magic glyphs (can be found in system menu).
 */
static void FASTCALL
MENU_DrawPopupGlyph(HDC dc, LPRECT r, INT_PTR popupMagic, BOOL inactive, BOOL hilite)
{
  LOGFONTW lf;
  HFONT hFont, hOldFont;
  COLORREF clrsave;
  INT bkmode;
  WCHAR symbol;
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
  RtlZeroMemory(&lf, sizeof(LOGFONTW));
  RECTL_vInflateRect(r, -2, -2);
  lf.lfHeight = r->bottom - r->top;
  lf.lfWidth = 0;
  lf.lfWeight = FW_NORMAL;
  lf.lfCharSet = DEFAULT_CHARSET;
  RtlCopyMemory(lf.lfFaceName, L"Marlett", sizeof(L"Marlett"));
  hFont = GreCreateFontIndirectW(&lf);
  /* save font and text color */
  hOldFont = NtGdiSelectFont(dc, hFont);
  clrsave = GreGetTextColor(dc);
  bkmode = GreGetBkMode(dc);
  /* set color and drawing mode */
  IntGdiSetBkMode(dc, TRANSPARENT);
  if (inactive)
  {
    /* draw shadow */
    if (!hilite)
    {
      IntGdiSetTextColor(dc, IntGetSysColor(COLOR_HIGHLIGHTTEXT));
      GreTextOutW(dc, r->left + 1, r->top + 1, &symbol, 1);
    }
  }
  IntGdiSetTextColor(dc, IntGetSysColor(inactive ? COLOR_GRAYTEXT : (hilite ? COLOR_HIGHLIGHTTEXT : COLOR_MENUTEXT)));
  /* draw selected symbol */
  GreTextOutW(dc, r->left, r->top, &symbol, 1);
  /* restore previous settings */
  IntGdiSetTextColor(dc, clrsave);
  NtGdiSelectFont(dc, hOldFont);
  IntGdiSetBkMode(dc, bkmode);
  GreDeleteObject(hFont);
}

/***********************************************************************
 *           MENU_AdjustMenuItemRect
 *
 * Adjust menu item rectangle according to scrolling state.
 */
VOID FASTCALL
MENU_AdjustMenuItemRect(PMENU menu, PRECTL rect)
{
    if (menu->dwArrowsOn)
    {
        UINT arrow_bitmap_height;
        arrow_bitmap_height = gpsi->oembmi[OBI_UPARROW].cy; ///// Menu up arrow! OBM_UPARROW
        rect->top += arrow_bitmap_height - menu->iTop;
        rect->bottom += arrow_bitmap_height - menu->iTop;
    }
}

/***********************************************************************
 *           MENU_FindItemByCoords
 *
 * Find the item at the specified coordinates (screen coords). Does
 * not work for child windows and therefore should not be called for
 * an arbitrary system menu.
 */
static ITEM *MENU_FindItemByCoords( MENU *menu, POINT pt, UINT *pos )
{
    ITEM *item;
    UINT i;
    INT cx, cy;
    RECT rect;
    PWND pWnd = ValidateHwndNoErr(menu->hWnd);

    if (!IntGetWindowRect(pWnd, &rect)) return NULL;

    cx = UserGetSystemMetrics(SM_CXDLGFRAME);
    cy = UserGetSystemMetrics(SM_CYDLGFRAME);
    RECTL_vInflateRect(&rect, -cx, -cy);

    if (pWnd->ExStyle & WS_EX_LAYOUTRTL)
       pt.x = rect.right - 1 - pt.x;
    else
       pt.x -= rect.left;
    pt.y -= rect.top;
    item = menu->rgItems;
    for (i = 0; i < menu->cItems; i++, item++)
    {
        //rect = item->rect;
        rect.left   = item->xItem;
        rect.top    = item->yItem; 
        rect.right  = item->cxItem; // Do this for now......
        rect.bottom = item->cyItem;

        MENU_AdjustMenuItemRect(menu, &rect);
	if (RECTL_bPointInRect(&rect, pt.x, pt.y))
	{
	    if (pos) *pos = i;
	    return item;
	}
    }
    return NULL;
}

INT FASTCALL IntMenuItemFromPoint(PWND pWnd, HMENU hMenu, POINT ptScreen)
{
    MENU *menu = UserGetMenuObject(hMenu);
    UINT pos;

    /*FIXME: Do we have to handle hWnd here? */
    if (!menu) return -1;
    if (!MENU_FindItemByCoords(menu, ptScreen, &pos)) return -1;
    return pos;
}

/***********************************************************************
 *           MenuFindItemByKey
 *
 * Find the menu item selected by a key press.
 * Return item id, -1 if none, -2 if we should close the menu.
 */
static UINT FASTCALL MENU_FindItemByKey(PWND WndOwner, PMENU menu,
                  WCHAR Key, BOOL ForceMenuChar)
{
  LRESULT MenuChar;
  WORD Flags = 0;

  TRACE("\tlooking for '%c' (0x%02x) in [%p]\n", (char)Key, Key, menu );

  if (!menu || !VerifyMenu(menu))
     menu = co_IntGetSubMenu( UserGetMenuObject(WndOwner->SystemMenu), 0 );
  if (menu)
  {
     ITEM *item = menu->rgItems;

     if ( !ForceMenuChar )
     {
        UINT i;
        BOOL cjk = UserGetSystemMetrics( SM_DBCSENABLED );

        for (i = 0; i < menu->cItems; i++, item++)
        {
           LPWSTR text = item->Xlpstr;
           if( text)
           {
              const WCHAR *p = text - 2;
              do
              {
                 const WCHAR *q = p + 2;
                 p = wcschr (q, '&');
                 if (!p && cjk) p = wcschr (q, '\036'); /* Japanese Win16 */
              }
              while (p != NULL && p [1] == '&');
              if (p && (towupper(p[1]) == towupper(Key))) return i;
           }
        }
     }

     Flags |= menu->fFlags & MNF_POPUP ? MF_POPUP : 0;
     Flags |= menu->fFlags & MNF_SYSMENU ? MF_SYSMENU : 0;

     MenuChar = co_IntSendMessage( UserHMGetHandle(WndOwner), WM_MENUCHAR,
                              MAKEWPARAM(Key, Flags), (LPARAM) UserHMGetHandle(menu));
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
static void FASTCALL MENU_GetBitmapItemSize(PITEM lpitem, SIZE *size, PWND WndOwner)
{
    BITMAP bm;
    HBITMAP bmp = lpitem->hbmp;

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
                measItem.itemWidth  = lpitem->cxItem - lpitem->xItem; //lpitem->Rect.right  - lpitem->Rect.left;
                measItem.itemHeight = lpitem->cyItem - lpitem->yItem; //lpitem->Rect.bottom - lpitem->Rect.top;
                measItem.itemData = lpitem->dwItemData;
                co_IntSendMessage( UserHMGetHandle(WndOwner), WM_MEASUREITEM, 0, (LPARAM)&measItem);
                size->cx = measItem.itemWidth;
                size->cy = measItem.itemHeight;
                TRACE("HBMMENU_CALLBACK Height %d Width %d\n",measItem.itemHeight,measItem.itemWidth);
                return;
            }
            break;

          case (INT_PTR) HBMMENU_SYSTEM:
            if (lpitem->dwItemData)
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
            size->cx = UserGetSystemMetrics(SM_CXSIZE) - 2;
            size->cy = UserGetSystemMetrics(SM_CYSIZE) - 4;
            return;
        }
    }

    if (GreGetObject(bmp, sizeof(BITMAP), &bm))
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
static void FASTCALL MENU_DrawBitmapItem(HDC hdc, PITEM lpitem, const RECT *rect,
                    PMENU Menu, PWND WndOwner, UINT odaction, BOOL MenuBar)
{
    BITMAP bm;
    DWORD rop;
    HDC hdcMem;
    HBITMAP bmp;
    int w = rect->right - rect->left;
    int h = rect->bottom - rect->top;
    int bmp_xoffset = 0;
    int left, top;
    BOOL flat_menu;
    HBITMAP hbmToDraw = lpitem->hbmp;
    bmp = hbmToDraw;

    UserSystemParametersInfo(SPI_GETFLATMENU, 0, &flat_menu, 0);

    /* Check if there is a magic menu item associated with this item */
    if (IS_MAGIC_BITMAP(hbmToDraw))
    {
        UINT flags = 0;
        RECT r;

      r = *rect;
      switch ((INT_PTR)hbmToDraw)
      {
        case (INT_PTR)HBMMENU_SYSTEM:
            if (lpitem->dwItemData)
            {
                if (ValidateHwndNoErr((HWND)lpitem->dwItemData))
                {
                   ERR("Get Item Data from this Window!!!\n");
                }

                ERR("Draw Bitmap\n");
                bmp = (HBITMAP)lpitem->dwItemData;
                if (!GreGetObject( bmp, sizeof(bm), &bm )) return;
            }
            else
            {
                PCURICON_OBJECT pIcon = NULL;
                //if (!BmpSysMenu) BmpSysMenu = LoadBitmapW(0, MAKEINTRESOURCEW(OBM_CLOSE));
                //bmp = BmpSysMenu;
                //if (! GreGetObject(bmp, sizeof(bm), &bm)) return;
                /* only use right half of the bitmap */
                //bmp_xoffset = bm.bmWidth / 2;
                //bm.bmWidth -= bmp_xoffset;
                if (WndOwner)
                {
                    pIcon = NC_IconForWindow(WndOwner);
                    // FIXME: NC_IconForWindow should reference it for us */
                    if (pIcon) UserReferenceObject(pIcon);
                }
                ERR("Draw ICON\n");
                if (pIcon)
                {
                   LONG cx = UserGetSystemMetrics(SM_CXSMICON);
                   LONG cy = UserGetSystemMetrics(SM_CYSMICON);
                   LONG x = rect->left - cx/2 + 1 + (rect->bottom - rect->top)/2; // this is really what Window does
                   LONG y = (rect->top + rect->bottom)/2 - cy/2; // center
                   UserDrawIconEx(hdc, x, y, pIcon, cx, cy, 0, NULL, DI_NORMAL);
                   UserDereferenceObject(pIcon);
                 }
                 return;
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
                drawItem.itemState |= (!(Menu->fFlags & MNF_UNDERLINE))?ODS_NOACCEL:0;
                drawItem.itemState |= (Menu->fFlags & MNF_INACTIVE)?ODS_INACTIVE:0;
                drawItem.hwndItem = (HWND)UserHMGetHandle(Menu);
                drawItem.hDC = hdc;
                drawItem.rcItem = *rect;
                drawItem.itemData = lpitem->dwItemData;
                /* some applications make this assumption on the DC's origin */
                GreSetViewportOrgEx( hdc, lpitem->xItem, lpitem->yItem, &origorg);
                RECTL_vOffsetRect(&drawItem.rcItem, -(LONG)lpitem->xItem, -(LONG)lpitem->yItem);
                co_IntSendMessage( UserHMGetHandle(WndOwner), WM_DRAWITEM, 0, (LPARAM)&drawItem);
                GreSetViewportOrgEx( hdc, origorg.x, origorg.y, NULL);
                return;
            }
            break;

          case (INT_PTR) HBMMENU_POPUP_CLOSE:
          case (INT_PTR) HBMMENU_POPUP_RESTORE:
          case (INT_PTR) HBMMENU_POPUP_MAXIMIZE:
          case (INT_PTR) HBMMENU_POPUP_MINIMIZE:
            MENU_DrawPopupGlyph(hdc, &r, (INT_PTR)hbmToDraw, lpitem->fState & MF_GRAYED, lpitem->fState & MF_HILITE);
            return;
      }
      RECTL_vInflateRect(&r, -1, -1);
      if (lpitem->fState & MF_HILITE) flags |= DFCS_PUSHED;
      DrawFrameControl(hdc, &r, DFC_CAPTION, flags);
      return;
    }

    if (!bmp || !GreGetObject( bmp, sizeof(bm), &bm )) return;

 got_bitmap:
    hdcMem = NtGdiCreateCompatibleDC( hdc );
    NtGdiSelectBitmap( hdcMem, bmp );
    /* handle fontsize > bitmap_height */
    top = (h>bm.bmHeight) ? rect->top+(h-bm.bmHeight)/2 : rect->top;
    left=rect->left;
    rop=((lpitem->fState & MF_HILITE) && !IS_MAGIC_BITMAP(hbmToDraw)) ? NOTSRCCOPY : SRCCOPY;
    if ((lpitem->fState & MF_HILITE) && lpitem->hbmp)
        IntGdiSetBkColor(hdc, IntGetSysColor(COLOR_HIGHLIGHT));
    if (MenuBar &&
        !flat_menu &&
        (lpitem->fState & (MF_HILITE | MF_GRAYED)) == MF_HILITE)
    {
        ++left;
        ++top;
    }
    NtGdiBitBlt( hdc, left, top, w, h, hdcMem, bmp_xoffset, 0, rop , 0, 0);
    IntGdiDeleteDC( hdcMem, FALSE );
}

LONG
IntGetDialogBaseUnits(VOID)
{
    static DWORD units;

    if (!units)
    {
        HDC hdc;
        SIZE size;

        if ((hdc = UserGetDCEx(NULL, NULL, DCX_CACHE)))
        {
            size.cx = IntGetCharDimensions( hdc, NULL, (PDWORD)&size.cy );
            if (size.cx) units = MAKELONG( size.cx, size.cy );
            UserReleaseDC( 0, hdc, FALSE);
        }
    }
    return units;
}


/***********************************************************************
 *           MenuCalcItemSize
 *
 * Calculate the size of the menu item and store it in lpitem->rect.
 */
static void FASTCALL MENU_CalcItemSize( HDC hdc, PITEM lpitem, PMENU Menu, PWND pwndOwner,
                 INT orgX, INT orgY, BOOL menuBar, BOOL textandbmp)
{
    WCHAR *p;
    UINT check_bitmap_width = UserGetSystemMetrics( SM_CXMENUCHECK );
    UINT arrow_bitmap_width;
    RECT Rect;
    INT itemheight = 0;

    TRACE("dc=%x owner=%x (%d,%d)\n", hdc, pwndOwner, orgX, orgY);

    arrow_bitmap_width = gpsi->oembmi[OBI_MNARROW].cx;

    MenuCharSize.cx = IntGetCharDimensions( hdc, NULL, (PDWORD)&MenuCharSize.cy );

    RECTL_vSetRect( &Rect, orgX, orgY, orgX, orgY );

    if (lpitem->fType & MF_OWNERDRAW)
    {
        MEASUREITEMSTRUCT mis;
        mis.CtlType    = ODT_MENU;
        mis.CtlID      = 0;
        mis.itemID     = lpitem->wID;
        mis.itemData   = lpitem->dwItemData;
        mis.itemHeight = HIWORD( IntGetDialogBaseUnits());
        mis.itemWidth  = 0;
        co_IntSendMessage( UserHMGetHandle(pwndOwner), WM_MEASUREITEM, 0, (LPARAM)&mis );
        /* Tests reveal that Windows ( Win95 thru WinXP) adds twice the average
         * width of a menufont character to the width of an owner-drawn menu. 
         */
        Rect.right += mis.itemWidth + 2 * MenuCharSize.cx;
        if (menuBar) {
            /* under at least win95 you seem to be given a standard
               height for the menu and the height value is ignored */
            Rect.bottom += UserGetSystemMetrics(SM_CYMENUSIZE);
        } else
            Rect.bottom += mis.itemHeight;
        // Or this,
        //lpitem->cxBmp = mis.itemWidth;
        //lpitem->cyBmp = mis.itemHeight;
        TRACE("MF_OWNERDRAW Height %d Width %d\n",mis.itemHeight,mis.itemWidth);
        TRACE("MF_OWNERDRAW id=%04lx size=%dx%d cx %d cy %d\n",
                lpitem->wID, Rect.right-Rect.left,
                 Rect.bottom-Rect.top, MenuCharSize.cx, MenuCharSize.cy);

        lpitem->xItem = Rect.left;
        lpitem->yItem = Rect.top;
        lpitem->cxItem = Rect.right;
        lpitem->cyItem = Rect.bottom;

        return;
    }

    lpitem->xItem  = orgX;
    lpitem->yItem  = orgY; 
    lpitem->cxItem = orgX;
    lpitem->cyItem = orgY;

    if (lpitem->fType & MF_SEPARATOR)
    {
        lpitem->cyItem += UserGetSystemMetrics( SM_CYMENUSIZE)/2;//SEPARATOR_HEIGHT;
        if( !menuBar)
            lpitem->cxItem += arrow_bitmap_width + MenuCharSize.cx;
        return;
    }

    lpitem->dxTab = 0;

    if (lpitem->hbmp)
    {
        SIZE size;

        if (!menuBar) {
            MENU_GetBitmapItemSize(lpitem, &size, pwndOwner );
            /* Keep the size of the bitmap in callback mode to be able
             * to draw it correctly */
            lpitem->cxBmp = size.cx;
            lpitem->cyBmp = size.cy;
            Menu->cxTextAlign = max(Menu->cxTextAlign, size.cx);
            lpitem->cxItem += size.cx + 2;
            itemheight = size.cy + 2;

            if( !((Menu->fFlags & MNS_STYLE_MASK) & MNS_NOCHECK))
                lpitem->cxItem += 2 * check_bitmap_width;
            lpitem->cxItem += 4 + MenuCharSize.cx;
            lpitem->dxTab = lpitem->cxItem;
            lpitem->cxItem += arrow_bitmap_width;
        } else /* hbmpItem & MenuBar */ {
            MENU_GetBitmapItemSize(lpitem, &size, pwndOwner );
            lpitem->cxItem  += size.cx;
            if( lpitem->Xlpstr) lpitem->cxItem  += 2;
            itemheight = size.cy;

            /* Special case: Minimize button doesn't have a space behind it. */
            if (lpitem->hbmp == (HBITMAP)HBMMENU_MBAR_MINIMIZE ||
                lpitem->hbmp == (HBITMAP)HBMMENU_MBAR_MINIMIZE_D)
            lpitem->cxItem -= 1;
        }
    }
    else if (!menuBar) {
        if( !((Menu->fFlags & MNS_STYLE_MASK) & MNS_NOCHECK))
             lpitem->cxItem += check_bitmap_width;
        lpitem->cxItem += 4 + MenuCharSize.cx;
        lpitem->dxTab = lpitem->cxItem;
        lpitem->cxItem += arrow_bitmap_width;
    }

    /* it must be a text item - unless it's the system menu */
    if (!(lpitem->fType & MF_SYSMENU) && lpitem->Xlpstr) {
        HFONT hfontOld = NULL;
        RECT rc;// = lpitem->Rect;
        LONG txtheight, txtwidth;

        rc.left   = lpitem->xItem;
        rc.top    = lpitem->yItem; 
        rc.right  = lpitem->cxItem; // Do this for now......
        rc.bottom = lpitem->cyItem;

        if ( lpitem->fState & MFS_DEFAULT ) {
            hfontOld = NtGdiSelectFont( hdc, ghMenuFontBold );
        }
        if (menuBar) {
            txtheight = DrawTextW( hdc, lpitem->Xlpstr, -1, &rc, DT_SINGLELINE|DT_CALCRECT); 

            lpitem->cxItem  += rc.right - rc.left;
            itemheight = max( max( itemheight, txtheight), UserGetSystemMetrics( SM_CYMENU) - 1);

            lpitem->cxItem +=  2 * MenuCharSize.cx;
        } else {
            if ((p = wcschr( lpitem->Xlpstr, '\t' )) != NULL) {
                RECT tmprc = rc;
                LONG tmpheight;
                int n = (int)( p - lpitem->Xlpstr);
                /* Item contains a tab (only meaningful in popup menus) */
                /* get text size before the tab */
                txtheight = DrawTextW( hdc, lpitem->Xlpstr, n, &rc,
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
                txtheight = DrawTextW( hdc, lpitem->Xlpstr, -1, &rc,
                        DT_SINGLELINE|DT_CALCRECT);
                txtwidth = rc.right - rc.left;
                lpitem->dxTab += txtwidth;
            }
            lpitem->cxItem  += 2 + txtwidth;
            itemheight = max( itemheight,
				    max( txtheight + 2, MenuCharSize.cy + 4));
        }
        if (hfontOld)
        {
           NtGdiSelectFont (hdc, hfontOld);
        }
    } else if( menuBar) {
        itemheight = max( itemheight, UserGetSystemMetrics(SM_CYMENU)-1);
    }
    lpitem->cyItem += itemheight;
    TRACE("(%ld,%ld)-(%ld,%ld)\n", lpitem->xItem, lpitem->yItem, lpitem->cxItem, lpitem->cyItem);
}

/***********************************************************************
 *           MENU_GetMaxPopupHeight
 */
static UINT
MENU_GetMaxPopupHeight(PMENU lppop)
{
    if (lppop->cyMax)
    {
       //ERR("MGMaxPH cyMax %d\n",lppop->cyMax);
       return lppop->cyMax;
    }
    //ERR("MGMaxPH SyMax %d\n",UserGetSystemMetrics(SM_CYSCREEN) - UserGetSystemMetrics(SM_CYBORDER));
    return UserGetSystemMetrics(SM_CYSCREEN) - UserGetSystemMetrics(SM_CYBORDER);
}

/***********************************************************************
 *           MenuPopupMenuCalcSize
 *
 * Calculate the size of a popup menu.
 */
static void FASTCALL MENU_PopupMenuCalcSize(PMENU Menu, PWND WndOwner)
{
    PITEM lpitem;
    HDC hdc;
    int start, i;
    int orgX, orgY, maxX, maxTab, maxTabWidth, maxHeight;
    BOOL textandbmp = FALSE;

    Menu->cxMenu = Menu->cyMenu = 0;
    if (Menu->cItems == 0) return;

    hdc = UserGetDCEx(NULL, NULL, DCX_CACHE);

    NtGdiSelectFont( hdc, ghMenuFont );

    start = 0;
    maxX = 0;

    Menu->cxTextAlign = 0;

    while (start < Menu->cItems)
    {
      lpitem = &Menu->rgItems[start];
      orgX = maxX;
      if( lpitem->fType & (MF_MENUBREAK | MF_MENUBARBREAK))
          orgX += MENU_COL_SPACE;
      orgY = 0;

      maxTab = maxTabWidth = 0;
      /* Parse items until column break or end of menu */
      for (i = start; i < Menu->cItems; i++, lpitem++)
      {
          if (i != start &&
               (lpitem->fType & (MF_MENUBREAK | MF_MENUBARBREAK))) break;

          MENU_CalcItemSize(hdc, lpitem, Menu, WndOwner, orgX, orgY, FALSE, textandbmp);
          maxX = max(maxX, lpitem->cxItem);
          orgY = lpitem->cyItem;
          if (IS_STRING_ITEM(lpitem->fType) && lpitem->dxTab )
          {
              maxTab = max( maxTab, lpitem->dxTab );
              maxTabWidth = max(maxTabWidth, lpitem->cxItem - lpitem->dxTab);
          }
          if( lpitem->Xlpstr && lpitem->hbmp) textandbmp = TRUE;
      }

        /* Finish the column (set all items to the largest width found) */
      maxX = max( maxX, maxTab + maxTabWidth );
      for (lpitem = &Menu->rgItems[start]; start < i; start++, lpitem++)
      {
          lpitem->cxItem = maxX;
           if (IS_STRING_ITEM(lpitem->fType) && lpitem->dxTab)
               lpitem->dxTab = maxTab;
      }
      Menu->cyMenu = max(Menu->cyMenu, orgY);
    }

    Menu->cxMenu  = maxX;
    /* if none of the items have both text and bitmap then
     * the text and bitmaps are all aligned on the left. If there is at
     * least one item with both text and bitmap then bitmaps are
     * on the left and texts left aligned with the right hand side
     * of the bitmaps */
    if( !textandbmp) Menu->cxTextAlign = 0;

    /* Adjust popup height if it exceeds maximum */
    maxHeight = MENU_GetMaxPopupHeight(Menu);
    Menu->iMaxTop = Menu->cyMenu;
    if (Menu->cyMenu >= maxHeight)
    {
       Menu->cyMenu = maxHeight;
       Menu->dwArrowsOn = 1;
    }
    else
    {   
       Menu->dwArrowsOn = 0;
    }
    UserReleaseDC( 0, hdc, FALSE );
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
static void MENU_MenuBarCalcSize( HDC hdc, LPRECT lprect, PMENU lppop, PWND pwndOwner )
{
    ITEM *lpitem;
    UINT start, i, helpPos;
    int orgX, orgY, maxY;

    if ((lprect == NULL) || (lppop == NULL)) return;
    if (lppop->cItems == 0) return;
    //TRACE("lprect %p %s\n", lprect, wine_dbgstr_rect( lprect));
    lppop->cxMenu  = lprect->right - lprect->left;
    lppop->cyMenu = 0;
    maxY = lprect->top;
    start = 0;
    helpPos = ~0U;
    lppop->cxTextAlign = 0;
    while (start < lppop->cItems)
    {
	lpitem = &lppop->rgItems[start];
	orgX = lprect->left;
	orgY = maxY;

	  /* Parse items until line break or end of menu */
	for (i = start; i < lppop->cItems; i++, lpitem++)
	{
	    if ((helpPos == ~0U) && (lpitem->fType & MF_RIGHTJUSTIFY)) helpPos = i;
	    if ((i != start) &&
		(lpitem->fType & (MF_MENUBREAK | MF_MENUBARBREAK))) break;

	    TRACE("calling MENU_CalcItemSize org=(%d, %d)\n", orgX, orgY );
	    //debug_print_menuitem ("  item: ", lpitem, "");
	    //MENU_CalcItemSize( hdc, lpitem, pwndOwner, orgX, orgY, TRUE, lppop );
            MENU_CalcItemSize(hdc, lpitem, lppop, pwndOwner, orgX, orgY, TRUE, FALSE);

	    if (lpitem->cxItem > lprect->right)
	    {
		if (i != start) break;
		else lpitem->cxItem = lprect->right;
	    }
	    maxY = max( maxY, lpitem->cyItem );
	    orgX = lpitem->cxItem;
	}

	  /* Finish the line (set all items to the largest height found) */

/* FIXME: Is this really needed? */ /*NO! it is not needed, why make the
   HBMMENU_MBAR_CLOSE, MINIMIZE & RESTORE, look the same size as the menu bar! */
#if 0
	while (start < i) lppop->rgItems[start++].cyItem = maxY;
#endif
	start = i; /* This works! */
    }

    lprect->bottom = maxY + 1;
    lppop->cyMenu = lprect->bottom - lprect->top;

    /* Flush right all items between the MF_RIGHTJUSTIFY and */
    /* the last item (if several lines, only move the last line) */
    if (helpPos == ~0U) return;
    lpitem = &lppop->rgItems[lppop->cItems-1];
    orgY = lpitem->yItem;
    orgX = lprect->right;
    for (i = lppop->cItems - 1; i >= helpPos; i--, lpitem--) {
        if (lpitem->yItem != orgY) break;	/* Other line */
        if (lpitem->cxItem >= orgX) break;	/* Too far right already */
        lpitem->xItem += orgX - lpitem->cxItem;
        lpitem->cxItem = orgX;
        orgX = lpitem->xItem;
    }
}

/***********************************************************************
 *           MENU_DrawScrollArrows
 *
 * Draw scroll arrows.
 */
static void MENU_DrawScrollArrows(PMENU lppop, HDC hdc)
{
    UINT arrow_bitmap_height;
    RECT rect;
    UINT Flags = 0;

    arrow_bitmap_height = gpsi->oembmi[OBI_DNARROW].cy;

    rect.left = 0;
    rect.top = 0;
    rect.right = lppop->cxMenu;
    rect.bottom = arrow_bitmap_height;
    FillRect(hdc, &rect, IntGetSysColorBrush(COLOR_MENU));
    DrawFrameControl(hdc, &rect, DFC_MENU, (lppop->iTop ? 0 : DFCS_INACTIVE)|DFCS_MENUARROWUP);

    rect.top = lppop->cyMenu - arrow_bitmap_height;
    rect.bottom = lppop->cyMenu;
    FillRect(hdc, &rect, IntGetSysColorBrush(COLOR_MENU));
    if (!(lppop->iTop < lppop->iMaxTop - (MENU_GetMaxPopupHeight(lppop) - 2 * arrow_bitmap_height)))
       Flags = DFCS_INACTIVE;
    DrawFrameControl(hdc, &rect, DFC_MENU, Flags|DFCS_MENUARROWDOWN);
}

/***********************************************************************
 *           MenuDrawMenuItem
 *
 * Draw a single menu item.
 */
static void FASTCALL MENU_DrawMenuItem(PWND Wnd, PMENU Menu, PWND WndOwner, HDC hdc,
                 PITEM lpitem, UINT Height, BOOL menuBar, UINT odaction)
{
    RECT rect;
    PWCHAR Text;
    BOOL flat_menu = FALSE;
    int bkgnd;
    UINT arrow_bitmap_width = 0;
    //RECT bmprc;

    if (!menuBar) {
        arrow_bitmap_width  = gpsi->oembmi[OBI_MNARROW].cx; 
    }

    if (lpitem->fType & MF_SYSMENU)
    {
        if (!(Wnd->style & WS_MINIMIZE))
        {
          NC_GetInsideRect(Wnd, &rect);
          UserDrawSysMenuButton(Wnd, hdc, &rect, lpitem->fState & (MF_HILITE | MF_MOUSESELECT));
	}
        return;
    }

    UserSystemParametersInfo (SPI_GETFLATMENU, 0, &flat_menu, 0);
    bkgnd = (menuBar && flat_menu) ? COLOR_MENUBAR : COLOR_MENU;
  
    /* Setup colors */

    if (lpitem->fState & MF_HILITE)
    {
        if(menuBar && !flat_menu) {
            IntGdiSetTextColor(hdc, IntGetSysColor(COLOR_MENUTEXT));
            IntGdiSetBkColor(hdc, IntGetSysColor(COLOR_MENU));
        } else {
            if (lpitem->fState & MF_GRAYED)
                IntGdiSetTextColor(hdc, IntGetSysColor(COLOR_GRAYTEXT));
            else
                IntGdiSetTextColor(hdc, IntGetSysColor(COLOR_HIGHLIGHTTEXT));
            IntGdiSetBkColor(hdc, IntGetSysColor(COLOR_HIGHLIGHT));
        }
    }
    else
    {
        if (lpitem->fState & MF_GRAYED)
            IntGdiSetTextColor( hdc, IntGetSysColor( COLOR_GRAYTEXT ) );
        else
            IntGdiSetTextColor( hdc, IntGetSysColor( COLOR_MENUTEXT ) );
        IntGdiSetBkColor( hdc, IntGetSysColor( bkgnd ) );
    }

    //TRACE("rect=%s\n", wine_dbgstr_rect( &lpitem->Rect));
    //rect = lpitem->Rect;
    rect.left   = lpitem->xItem;
    rect.top    = lpitem->yItem; 
    rect.right  = lpitem->cxItem; // Do this for now......
    rect.bottom = lpitem->cyItem;

    MENU_AdjustMenuItemRect(Menu, &rect);

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
        COLORREF old_bk, old_text;

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
        if (!(Menu->fFlags & MNF_UNDERLINE)) dis.itemState |= ODS_NOACCEL;
        if (Menu->fFlags & MNF_INACTIVE) dis.itemState |= ODS_INACTIVE;
        dis.itemAction = odaction; /* ODA_DRAWENTIRE | ODA_SELECT | ODA_FOCUS; */
        dis.hwndItem   = (HWND) UserHMGetHandle(Menu);
        dis.hDC        = hdc;
        dis.rcItem     = rect;
        TRACE("Ownerdraw: owner=%p itemID=%d, itemState=%d, itemAction=%d, "
	        "hwndItem=%p, hdc=%p, rcItem={%ld,%ld,%ld,%ld}\n", Wnd,
	        dis.itemID, dis.itemState, dis.itemAction, dis.hwndItem,
	        dis.hDC, dis.rcItem.left, dis.rcItem.top, dis.rcItem.right,
	        dis.rcItem.bottom);
        TRACE("Ownerdraw: Width %d Height %d\n", dis.rcItem.right-dis.rcItem.left, dis.rcItem.bottom-dis.rcItem.top);
        old_bk = GreGetBkColor(hdc);
        old_text = GreGetTextColor(hdc);
        co_IntSendMessage(UserHMGetHandle(WndOwner), WM_DRAWITEM, 0, (LPARAM) &dis);
        IntGdiSetBkColor(hdc, old_bk);
        IntGdiSetTextColor(hdc, old_text);
        /* Draw the popup-menu arrow */
        if (!menuBar && lpitem->spSubMenu)
        {
            RECT rectTemp;
            RtlCopyMemory(&rectTemp, &rect, sizeof(RECT));
            rectTemp.left = rectTemp.right - UserGetSystemMetrics(SM_CXMENUCHECK);
            DrawFrameControl(hdc, &rectTemp, DFC_MENU, DFCS_MENUARROW);
        }
        return;
    }

    if (menuBar && (lpitem->fType & MF_SEPARATOR)) return;

    if (lpitem->fState & MF_HILITE)
    {
        if (flat_menu)
        {
            RECTL_vInflateRect (&rect, -1, -1);
            FillRect(hdc, &rect, IntGetSysColorBrush(COLOR_MENUHILIGHT));
            RECTL_vInflateRect (&rect, 1, 1);
            FrameRect(hdc, &rect, IntGetSysColorBrush(COLOR_HIGHLIGHT));
        }
        else
        {
            if (menuBar)
            {
                FillRect(hdc, &rect, IntGetSysColorBrush(COLOR_MENU));
                DrawEdge(hdc, &rect, BDR_SUNKENOUTER, BF_RECT);
            }
            else
            {
                FillRect(hdc, &rect, IntGetSysColorBrush(COLOR_HIGHLIGHT));
            }
        }
    }
    else
        FillRect( hdc, &rect, IntGetSysColorBrush(bkgnd) );

    IntGdiSetBkMode( hdc, TRANSPARENT );

    /* vertical separator */
    if (!menuBar && (lpitem->fType & MF_MENUBARBREAK))
    {
        HPEN oldPen;
        RECT rc = rect;

        rc.left -= 3;//MENU_COL_SPACE / 2 + 1; == 3!!
        rc.top = 3;
        rc.bottom = Height - 3;
        if (flat_menu)
        {
            oldPen = NtGdiSelectPen( hdc, NtGdiGetStockObject(DC_PEN) );
            IntSetDCPenColor(hdc, IntGetSysColor(COLOR_BTNSHADOW));
            GreMoveTo( hdc, rc.left, rc.top, NULL );
            NtGdiLineTo( hdc, rc.left, rc.bottom );
            NtGdiSelectPen( hdc, oldPen );
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
        rc.top = (rc.top + rc.bottom) / 2 - 1;
        if (flat_menu)
        {
            oldPen = NtGdiSelectPen( hdc, NtGdiGetStockObject(DC_PEN) );
            IntSetDCPenColor( hdc, IntGetSysColor(COLOR_BTNSHADOW));
            GreMoveTo( hdc, rc.left, rc.top, NULL );
            NtGdiLineTo( hdc, rc.right, rc.top );
            NtGdiSelectPen( hdc, oldPen );
        }
        else
            DrawEdge (hdc, &rc, EDGE_ETCHED, BF_TOP);
        return;
    }
#if 0
    /* helper lines for debugging */
    /* This is a very good test tool when hacking menus! (JT) 07/16/2006 */
    FrameRect(hdc, &rect, NtGdiGetStockObject(BLACK_BRUSH));
    NtGdiSelectPen(hdc, NtGdiGetStockObject(DC_PEN));
    IntSetDCPenColor(hdc, IntGetSysColor(COLOR_WINDOWFRAME));
    GreMoveTo(hdc, rect.left, (rect.top + rect.bottom) / 2, NULL);
    NtGdiLineTo(hdc, rect.right, (rect.top + rect.bottom) / 2);
#endif
#if 0 // breaks mdi menu bar icons.
    if (lpitem->hbmp) {
        /* calculate the bitmap rectangle in coordinates relative
         * to the item rectangle */
        if( menuBar) {
            if( lpitem->hbmp == HBMMENU_CALLBACK)
                bmprc.left = 3;
            else 
                bmprc.left = lpitem->Xlpstr ? MenuCharSize.cx : 0;          
        }
        else if ((Menu->fFlags & MNS_STYLE_MASK) & MNS_NOCHECK)
            bmprc.left = 4;
        else if ((Menu->fFlags & MNS_STYLE_MASK) & MNS_CHECKORBMP)
            bmprc.left = 2;
        else
            bmprc.left = 4 + UserGetSystemMetrics(SM_CXMENUCHECK);

        bmprc.right =  bmprc.left + lpitem->cxBmp;

        if( menuBar && !(lpitem->hbmp == HBMMENU_CALLBACK))
            bmprc.top = 0;
        else
            bmprc.top = (rect.bottom - rect.top - lpitem->cyBmp) / 2; 

        bmprc.bottom =  bmprc.top + lpitem->cyBmp;
    }
#endif
    if (!menuBar)
    {
        HBITMAP bm;
        INT y = rect.top + rect.bottom;
        RECT rc = rect;
        BOOL checked = FALSE;
        UINT check_bitmap_width = UserGetSystemMetrics( SM_CXMENUCHECK );
        UINT check_bitmap_height = UserGetSystemMetrics( SM_CYMENUCHECK );
        /* Draw the check mark
         *
         * FIXME:
         * Custom checkmark bitmaps are monochrome but not always 1bpp.
         */
        if( !((Menu->fFlags & MNS_STYLE_MASK) & MNS_NOCHECK)) {
            bm = (lpitem->fState & MF_CHECKED) ? lpitem->hbmpChecked : 
                lpitem->hbmpUnchecked;
            if (bm)  /* we have a custom bitmap */
            {
                HDC hdcMem = NtGdiCreateCompatibleDC( hdc );

                NtGdiSelectBitmap( hdcMem, bm );
                NtGdiBitBlt( hdc, rc.left, (y - check_bitmap_height) / 2,
                        check_bitmap_width, check_bitmap_height,
                        hdcMem, 0, 0, SRCCOPY, 0,0);
                IntGdiDeleteDC( hdcMem, FALSE );
                checked = TRUE;
            }
            else if (lpitem->fState & MF_CHECKED) /* standard bitmaps */
            {
                RECT r;
                r = rect;
                r.right = r.left + check_bitmap_width;
                DrawFrameControl( hdc, &r, DFC_MENU,
                                 (lpitem->fType & MFT_RADIOCHECK) ?
                                 DFCS_MENUBULLET : DFCS_MENUCHECK);
                checked = TRUE;
            }
        }
        if ( lpitem->hbmp )//&& !( checked && ((Menu->fFlags & MNS_STYLE_MASK) & MNS_CHECKORBMP)))
        {
            RECT bmpRect = rect;
            if (!((Menu->fFlags & MNS_STYLE_MASK) & MNS_CHECKORBMP) && !((Menu->fFlags & MNS_STYLE_MASK) & MNS_NOCHECK))
                bmpRect.left += check_bitmap_width + 2;
            if (!(checked && ((Menu->fFlags & MNS_STYLE_MASK) & MNS_CHECKORBMP)))
            {
                bmpRect.right = bmpRect.left + lpitem->cxBmp;
                MENU_DrawBitmapItem(hdc, lpitem, &bmpRect, Menu, WndOwner, odaction, menuBar);
            }
        }
        /* Draw the popup-menu arrow */
        if (lpitem->spSubMenu)
        {
            RECT rectTemp;
            RtlCopyMemory(&rectTemp, &rect, sizeof(RECT));
            rectTemp.left = rectTemp.right - check_bitmap_width;
            DrawFrameControl(hdc, &rectTemp, DFC_MENU, DFCS_MENUARROW);
        }
        rect.left += 4;
        if( !((Menu->fFlags & MNS_STYLE_MASK) & MNS_NOCHECK))
            rect.left += check_bitmap_width;
        rect.right -= arrow_bitmap_width;
    }
    else if( lpitem->hbmp)
    { /* Draw the bitmap */
        MENU_DrawBitmapItem(hdc, lpitem, &rect/*bmprc*/, Menu, WndOwner, odaction, menuBar);
    }

    /* process text if present */
    if (lpitem->Xlpstr)
    {
        int i = 0;
        HFONT hfontOld = 0;

        UINT uFormat = menuBar ?
                       DT_CENTER | DT_VCENTER | DT_SINGLELINE :
                       DT_LEFT | DT_VCENTER | DT_SINGLELINE;

        if (((Menu->fFlags & MNS_STYLE_MASK) & MNS_CHECKORBMP))
             rect.left += max(0, (int)(Menu->cxTextAlign - UserGetSystemMetrics(SM_CXMENUCHECK)));
        else
             rect.left += Menu->cxTextAlign;

        if ( lpitem->fState & MFS_DEFAULT )
        {
            hfontOld = NtGdiSelectFont(hdc, ghMenuFontBold);            
        }

        if (menuBar) {
            if( lpitem->hbmp)
              rect.left += lpitem->cxBmp;
            if( !(lpitem->hbmp == HBMMENU_CALLBACK)) 
              rect.left += MenuCharSize.cx;
            rect.right -= MenuCharSize.cx;
        }

        Text = lpitem->Xlpstr;
        if(Text)
        {
            for (i = 0; Text[i]; i++)
                if (Text[i] == L'\t' || Text[i] == L'\b')
                    break;
        }

        if (menuBar &&
            !flat_menu &&
            (lpitem->fState & (MF_HILITE | MF_GRAYED)) == MF_HILITE)
        {
            RECTL_vOffsetRect(&rect, +1, +1);
        }

        if (!menuBar)
            --rect.bottom;

        if(lpitem->fState & MF_GRAYED)
        {
            if (!(lpitem->fState & MF_HILITE) )
            {
                ++rect.left; ++rect.top; ++rect.right; ++rect.bottom;
                IntGdiSetTextColor(hdc, IntGetSysColor(COLOR_BTNHIGHLIGHT));
                DrawTextW( hdc, Text, i, &rect, uFormat );
                --rect.left; --rect.top; --rect.right; --rect.bottom;
            }
            IntGdiSetTextColor(hdc, IntGetSysColor(COLOR_BTNSHADOW));
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
                    IntGdiSetTextColor(hdc, IntGetSysColor(COLOR_BTNHIGHLIGHT));
                    DrawTextW( hdc, Text + i + 1, -1, &rect, uFormat);
                    --rect.left; --rect.top; --rect.right; --rect.bottom;
                }
                IntGdiSetTextColor(hdc, IntGetSysColor(COLOR_BTNSHADOW));
            }
            DrawTextW( hdc, Text + i + 1, -1, &rect, uFormat );
        }

        if (!menuBar)
            ++rect.bottom;

        if (menuBar &&
            !flat_menu &&
            (lpitem->fState & (MF_HILITE | MF_GRAYED)) == MF_HILITE)
        {
            RECTL_vOffsetRect(&rect, -1, -1);
        }

        if (hfontOld)
        {
           NtGdiSelectFont (hdc, hfontOld);
        }
    }
}

/***********************************************************************
 *           MenuDrawPopupMenu
 *
 * Paint a popup menu.
 */
static void FASTCALL MENU_DrawPopupMenu(PWND wnd, HDC hdc, PMENU menu )
{
    HBRUSH hPrevBrush = 0, brush = IntGetSysColorBrush(COLOR_MENU);
    RECT rect;

    TRACE("DPM wnd=%p dc=%p menu=%p\n", wnd, hdc, menu);

    IntGetClientRect( wnd, &rect );

    if (menu && menu->hbrBack) brush = menu->hbrBack;
    if((hPrevBrush = NtGdiSelectBrush( hdc, brush ))
        && (NtGdiSelectFont( hdc, ghMenuFont)))
    {
        HPEN hPrevPen;

        /* FIXME: Maybe we don't have to fill the background manually */
        FillRect(hdc, &rect, brush);

        hPrevPen = NtGdiSelectPen( hdc, NtGdiGetStockObject( NULL_PEN ) );
        if ( hPrevPen )
        {
            TRACE("hmenu %p Style %08x\n", UserHMGetHandle(menu), (menu->fFlags & MNS_STYLE_MASK));
            /* draw menu items */
            if (menu && menu->cItems)
            {
                ITEM *item;
                UINT u;

                item = menu->rgItems;
                for( u = menu->cItems; u > 0; u--, item++)
                {
                    MENU_DrawMenuItem(wnd, menu, menu->spwndNotify, hdc, item,
                                         menu->cyMenu, FALSE, ODA_DRAWENTIRE);
                }
                /* draw scroll arrows */
                if (menu->dwArrowsOn)
                {
                   MENU_DrawScrollArrows(menu, hdc);
                }
            }
        }
        else
        {
            NtGdiSelectBrush( hdc, hPrevBrush );
        }
    }
}

/**********************************************************************
 *         MENU_IsMenuActive
 */
PWND MENU_IsMenuActive(VOID)
{
   return ValidateHwndNoErr(top_popup);
}

/**********************************************************************
 *         MENU_EndMenu
 *
 * Calls EndMenu() if the hwnd parameter belongs to the menu owner
 *
 * Does the (menu stuff) of the default window handling of WM_CANCELMODE
 */
void MENU_EndMenu( PWND pwnd )
{
    PMENU menu = NULL;
    menu = UserGetMenuObject(top_popup_hmenu);
    if ( menu && ( UserHMGetHandle(pwnd) == menu->hWnd || pwnd == menu->spwndNotify ) )
    {
       if (fInsideMenuLoop && top_popup)
       {
          fInsideMenuLoop = FALSE;

          if (fInEndMenu)
          {
             ERR("Already in End loop\n");
             return;
          }

          fInEndMenu = TRUE;
          UserPostMessage( top_popup, WM_CANCELMODE, 0, 0);
       }
    }
}

DWORD WINAPI
IntDrawMenuBarTemp(PWND pWnd, HDC hDC, LPRECT Rect, PMENU pMenu, HFONT Font)
{
  UINT i;
  HFONT FontOld = NULL;
  BOOL flat_menu = FALSE;

  UserSystemParametersInfo(SPI_GETFLATMENU, 0, &flat_menu, 0);

  if (!pMenu)
  {
      pMenu = UserGetMenuObject(UlongToHandle(pWnd->IDMenu));
  }

  if (!Font)
  {
      Font = ghMenuFont;
  }

  if (Rect == NULL || !pMenu)
  {
      return UserGetSystemMetrics(SM_CYMENU);
  }

  TRACE("(%x, %x, %p, %x, %x)\n", pWnd, hDC, Rect, pMenu, Font);

  FontOld = NtGdiSelectFont(hDC, Font);

  if (pMenu->cyMenu == 0)
  {
      MENU_MenuBarCalcSize(hDC, Rect, pMenu, pWnd);
  }

  Rect->bottom = Rect->top + pMenu->cyMenu;

  FillRect(hDC, Rect, IntGetSysColorBrush(flat_menu ? COLOR_MENUBAR : COLOR_MENU));

  NtGdiSelectPen(hDC, NtGdiGetStockObject(DC_PEN));
  IntSetDCPenColor(hDC, IntGetSysColor(COLOR_3DFACE));
  GreMoveTo(hDC, Rect->left, Rect->bottom - 1, NULL);
  NtGdiLineTo(hDC, Rect->right, Rect->bottom - 1);

  if (pMenu->cItems == 0)
  {
      NtGdiSelectFont(hDC, FontOld);
      return UserGetSystemMetrics(SM_CYMENU);
  }

  for (i = 0; i < pMenu->cItems; i++)
  {
      MENU_DrawMenuItem(pWnd, pMenu, pWnd, hDC, &pMenu->rgItems[i], pMenu->cyMenu, TRUE, ODA_DRAWENTIRE);
  }

  NtGdiSelectFont(hDC, FontOld);

  return pMenu->cyMenu;
}

UINT MENU_DrawMenuBar( HDC hDC, LPRECT lprect, PWND pWnd, BOOL suppress_draw )
{
    HFONT hfontOld = 0;
    PMENU lppop = UserGetMenuObject(UlongToHandle(pWnd->IDMenu));

    if (lppop == NULL)
    {
        // No menu. Do not reserve any space
        return 0;
    }

    if (lprect == NULL)
    {
        return UserGetSystemMetrics(SM_CYMENU);
    }

    if (suppress_draw)
    {
       hfontOld = NtGdiSelectFont(hDC, ghMenuFont);

       MENU_MenuBarCalcSize(hDC, lprect, lppop, pWnd);

       lprect->bottom = lprect->top + lppop->cyMenu;

       if (hfontOld) NtGdiSelectFont( hDC, hfontOld);

       return lppop->cyMenu;
    }   
    else
    {
       return IntDrawMenuBarTemp(pWnd, hDC, lprect, lppop, NULL);
    }
}

/***********************************************************************
 *           MENU_InitPopup
 *   
 * Popup menu initialization before WM_ENTERMENULOOP.
 */
static BOOL MENU_InitPopup( PWND pWndOwner, PMENU menu, UINT flags )
{
    PWND pWndCreated;
    PPOPUPMENU pPopupMenu;
    CREATESTRUCTW Cs;
    LARGE_STRING WindowName;
    UNICODE_STRING ClassName;
    DWORD ex_style = WS_EX_PALETTEWINDOW | WS_EX_DLGMODALFRAME;

    TRACE("owner=%p hmenu=%p\n", pWndOwner, menu);

    menu->spwndNotify = pWndOwner;

    if (flags & TPM_LAYOUTRTL || pWndOwner->ExStyle & WS_EX_LAYOUTRTL)
       ex_style |= WS_EX_LAYOUTRTL;

    ClassName.Buffer = WC_MENU;
    ClassName.Length = 0;

    RtlZeroMemory(&WindowName, sizeof(WindowName));
    RtlZeroMemory(&Cs, sizeof(Cs));
    Cs.style = WS_POPUP | WS_CLIPSIBLINGS | WS_BORDER;
    Cs.dwExStyle = ex_style;
    Cs.hInstance = hModClient; // hModuleWin; // Server side winproc!
    Cs.lpszName = (LPCWSTR) &WindowName;
    Cs.lpszClass = (LPCWSTR) &ClassName;
    Cs.lpCreateParams = UserHMGetHandle(menu);
    Cs.hwndParent = UserHMGetHandle(pWndOwner);

    /* NOTE: In Windows, top menu popup is not owned. */
    pWndCreated = co_UserCreateWindowEx( &Cs, &ClassName, &WindowName, NULL, WINVER );

    if( !pWndCreated ) return FALSE;

    //
    //  Setup pop up menu structure.
    //
    menu->hWnd = UserHMGetHandle(pWndCreated);

    pPopupMenu = ((PMENUWND)pWndCreated)->ppopupmenu;

    pPopupMenu->spwndActivePopup = pWndCreated; // top_popup = MenuInfo.Wnd or menu->hWnd
    pPopupMenu->spwndNotify = pWndOwner;        // Same as MenuInfo.spwndNotify(which could be wrong) or menu->hwndOwner
    //pPopupMenu->spmenu = menu; Should be set up already from WM_CREATE!

    pPopupMenu->fIsTrackPopup = !!(flags & TPM_POPUPMENU);
    pPopupMenu->fIsSysMenu    = !!(flags & TPM_SYSTEM_MENU);
    pPopupMenu->fNoNotify     = !!(flags & TPM_NONOTIFY);
    pPopupMenu->fRightButton  = !!(flags & TPM_RIGHTBUTTON);
    pPopupMenu->fSynchronous  = !!(flags & TPM_RETURNCMD);

    if (pPopupMenu->fRightButton)
       pPopupMenu->fFirstClick = !!(UserGetKeyState(VK_RBUTTON) & 0x8000);
    else
       pPopupMenu->fFirstClick = !!(UserGetKeyState(VK_LBUTTON) & 0x8000);

    if (gpsi->aiSysMet[SM_MENUDROPALIGNMENT] ||
        menu->fFlags & MNF_RTOL)
    {
       pPopupMenu->fDroppedLeft = TRUE;
    }
    return TRUE;
}


#define SHOW_DEBUGRECT      0

#if SHOW_DEBUGRECT
static void DebugRect(const RECT* rectl, COLORREF color)
{
    HBRUSH brush;
    RECT rr;
    HDC hdc;

    if (!rectl)
        return;

    hdc = UserGetDCEx(NULL, 0, DCX_USESTYLE);

    brush = IntGdiCreateSolidBrush(color);

    rr = *rectl;
    RECTL_vInflateRect(&rr, 1, 1);
    FrameRect(hdc, rectl, brush);
    FrameRect(hdc, &rr, brush);

    NtGdiDeleteObjectApp(brush);
    UserReleaseDC(NULL, hdc, TRUE);
}

static void DebugPoint(INT x, INT y, COLORREF color)
{
    RECT r1 = {x-10, y, x+10, y};
    RECT r2 = {x, y-10, x, y+10};
    DebugRect(&r1, color);
    DebugRect(&r2, color);
}
#endif

static BOOL RECTL_Intersect(const RECT* pRect, INT x, INT y, UINT width, UINT height)
{
    RECT other = {x, y, x + width, y + height};
    RECT dum;

    return RECTL_bIntersectRect(&dum, pRect, &other);
}

static BOOL MENU_MoveRect(UINT flags, INT* x, INT* y, INT width, INT height, const RECT* pExclude, PMONITOR monitor)
{
    /* Figure out if we should move vertical or horizontal */
    if (flags & TPM_VERTICAL)
    {
        /* Move in the vertical direction: TPM_BOTTOMALIGN means drop it above, otherways drop it below */
        if (flags & TPM_BOTTOMALIGN)
        {
            if (pExclude->top - height >= monitor->rcMonitor.top)
            {
                *y = pExclude->top - height;
                return TRUE;
            }
        }
        else
        {
            if (pExclude->bottom + height < monitor->rcMonitor.bottom)
            {
                *y = pExclude->bottom;
                return TRUE;
            }
        }
    }
    else
    {
        /* Move in the horizontal direction: TPM_RIGHTALIGN means drop it to the left, otherways go right */
        if (flags & TPM_RIGHTALIGN)
        {
            if (pExclude->left - width >= monitor->rcMonitor.left)
            {
                *x = pExclude->left - width;
                return TRUE;
            }
        }
        else
        {
            if (pExclude->right + width < monitor->rcMonitor.right)
            {
                *x = pExclude->right;
                return TRUE;
            }
        }
    }
    return FALSE;
}

/***********************************************************************
 *           MenuShowPopup
 *
 * Display a popup menu.
 */
static BOOL FASTCALL MENU_ShowPopup(PWND pwndOwner, PMENU menu, UINT id, UINT flags,
                              INT x, INT y, const RECT* pExclude)
{
    INT width, height;
    POINT ptx;
    PMONITOR monitor;
    PWND pWnd;
    USER_REFERENCE_ENTRY Ref;
    BOOL bIsPopup = (flags & TPM_POPUPMENU) != 0;

    TRACE("owner=%p menu=%p id=0x%04x x=0x%04x y=0x%04x\n",
          pwndOwner, menu, id, x, y);

    if (menu->iItem != NO_SELECTED_ITEM)
    {
        menu->rgItems[menu->iItem].fState &= ~(MF_HILITE|MF_MOUSESELECT);
        menu->iItem = NO_SELECTED_ITEM;
    }

#if SHOW_DEBUGRECT
    if (pExclude)
        DebugRect(pExclude, RGB(255, 0, 0));
#endif

    menu->dwArrowsOn = 0;
    MENU_PopupMenuCalcSize(menu, pwndOwner);

    /* adjust popup menu pos so that it fits within the desktop */

    width = menu->cxMenu + UserGetSystemMetrics(SM_CXDLGFRAME) * 2;
    height = menu->cyMenu + UserGetSystemMetrics(SM_CYDLGFRAME) * 2;

    if (flags & TPM_LAYOUTRTL)
        flags ^= TPM_RIGHTALIGN;

    if (flags & TPM_RIGHTALIGN)
        x -= width;
    if (flags & TPM_CENTERALIGN)
        x -= width / 2;

    if (flags & TPM_BOTTOMALIGN)
        y -= height;
    if (flags & TPM_VCENTERALIGN)
        y -= height / 2;

    /* FIXME: should use item rect */
    ptx.x = x;
    ptx.y = y;
#if SHOW_DEBUGRECT
    DebugPoint(x, y, RGB(0, 0, 255));
#endif
    monitor = UserMonitorFromPoint( ptx, MONITOR_DEFAULTTONEAREST );

    /* We are off the right side of the screen */
    if (x + width > monitor->rcMonitor.right)
    {
        if ((x - width) < monitor->rcMonitor.left || x >= monitor->rcMonitor.right)
            x = monitor->rcMonitor.right - width;
        else
            x -= width;
    }

    /* We are off the left side of the screen */
    if (x < monitor->rcMonitor.left)
    {
        /* Re-orient the menu around the x-axis */
        x += width;

        if (x < monitor->rcMonitor.left || x >= monitor->rcMonitor.right || bIsPopup)
            x = monitor->rcMonitor.left;
    }

    /* Same here, but then the top */
    if (y < monitor->rcMonitor.top)
    {
        y += height;

        if (y < monitor->rcMonitor.top || y >= monitor->rcMonitor.bottom || bIsPopup)
            y = monitor->rcMonitor.top;
    }

    /* And the bottom */
    if (y + height > monitor->rcMonitor.bottom)
    {
        if ((y - height) < monitor->rcMonitor.top || y >= monitor->rcMonitor.bottom)
            y = monitor->rcMonitor.bottom - height;
        else
            y -= height;
    }

    if (pExclude)
    {
        RECT Cleaned;

        if (RECTL_bIntersectRect(&Cleaned, pExclude, &monitor->rcMonitor) &&
            RECTL_Intersect(&Cleaned, x, y, width, height))
        {
            UINT flag_mods[] = {
                0,                                                  /* First try the 'normal' way */
                TPM_BOTTOMALIGN | TPM_RIGHTALIGN,                   /* Then try the opposite side */
                TPM_VERTICAL,                                       /* Then swap horizontal / vertical */
                TPM_BOTTOMALIGN | TPM_RIGHTALIGN | TPM_VERTICAL,    /* Then the other side again (still swapped hor/ver) */
            };

            UINT n;
            for (n = 0; n < RTL_NUMBER_OF(flag_mods); ++n)
            {
                INT tx = x;
                INT ty = y;

                /* Try to move a bit around */
                if (MENU_MoveRect(flags ^ flag_mods[n], &tx, &ty, width, height, &Cleaned, monitor) &&
                    !RECTL_Intersect(&Cleaned, tx, ty, width, height))
                {
                    x = tx;
                    y = ty;
                    break;
                }
            }
            /* If none worked, we go with the original x/y */
        }
    }

#if SHOW_DEBUGRECT
    {
        RECT rr = {x, y, x + width, y + height};
        DebugRect(&rr, RGB(0, 255, 0));
    }
#endif

    pWnd = ValidateHwndNoErr( menu->hWnd );

    if (!pWnd)
    {
       ERR("menu->hWnd bad hwnd %p\n",menu->hWnd);
       return FALSE;
    }

    if (!top_popup) {
        top_popup = menu->hWnd;
        top_popup_hmenu = UserHMGetHandle(menu);
    }

    /* Display the window */
    UserRefObjectCo(pWnd, &Ref);
    co_WinPosSetWindowPos( pWnd, HWND_TOPMOST, x, y, width, height, SWP_SHOWWINDOW | SWP_NOACTIVATE);

    co_IntUpdateWindows(pWnd, RDW_ALLCHILDREN, FALSE);

    IntNotifyWinEvent(EVENT_SYSTEM_MENUPOPUPSTART, pWnd, OBJID_CLIENT, CHILDID_SELF, 0);
    UserDerefObjectCo(pWnd);

    return TRUE;
}

/***********************************************************************
 *           MENU_EnsureMenuItemVisible
 */
void MENU_EnsureMenuItemVisible(PMENU lppop, UINT wIndex, HDC hdc)
{
    USER_REFERENCE_ENTRY Ref;
    if (lppop->dwArrowsOn)
    {
        ITEM *item = &lppop->rgItems[wIndex];
        UINT nMaxHeight = MENU_GetMaxPopupHeight(lppop);
        UINT nOldPos = lppop->iTop;
        RECT rc;
        UINT arrow_bitmap_height;
        PWND pWnd = ValidateHwndNoErr(lppop->hWnd);

        IntGetClientRect(pWnd, &rc);

        arrow_bitmap_height = gpsi->oembmi[OBI_DNARROW].cy;

        rc.top += arrow_bitmap_height;
        rc.bottom -= arrow_bitmap_height;

        nMaxHeight -= UserGetSystemMetrics(SM_CYBORDER) + 2 * arrow_bitmap_height;
        UserRefObjectCo(pWnd, &Ref);
        if (item->cyItem > lppop->iTop + nMaxHeight)
        {
            lppop->iTop = item->cyItem - nMaxHeight;
            IntScrollWindow(pWnd, 0, nOldPos - lppop->iTop, &rc, &rc);
            MENU_DrawScrollArrows(lppop, hdc);
            //ERR("Scroll Down iTop %d iMaxTop %d nMaxHeight %d\n",lppop->iTop,lppop->iMaxTop,nMaxHeight);
        }
        else if (item->yItem < lppop->iTop)
        {
            lppop->iTop = item->yItem;
            IntScrollWindow(pWnd, 0, nOldPos - lppop->iTop, &rc, &rc);
            MENU_DrawScrollArrows(lppop, hdc);
            //ERR("Scroll Up   iTop %d iMaxTop %d nMaxHeight %d\n",lppop->iTop,lppop->iMaxTop,nMaxHeight);
        }
        UserDerefObjectCo(pWnd);
    }
}

/***********************************************************************
 *           MenuSelectItem
 */
static void FASTCALL MENU_SelectItem(PWND pwndOwner, PMENU menu, UINT wIndex,
                                    BOOL sendMenuSelect, PMENU topmenu)
{
    HDC hdc;
    PWND pWnd;

    TRACE("M_SI: owner=%p menu=%p index=0x%04x select=0x%04x\n", pwndOwner, menu, wIndex, sendMenuSelect);

    if (!menu || !menu->cItems) return;

    pWnd = ValidateHwndNoErr(menu->hWnd);

    if (!pWnd) return;

    if (menu->iItem == wIndex) return;

    if (menu->fFlags & MNF_POPUP)
       hdc = UserGetDCEx(pWnd, 0, DCX_USESTYLE);
    else
       hdc = UserGetDCEx(pWnd, 0, DCX_CACHE | DCX_WINDOW);

    if (!top_popup) {
        top_popup = menu->hWnd;                  //pPopupMenu->spwndActivePopup or
                                                 //pPopupMenu->fIsTrackPopup set pPopupMenu->spwndPopupMenu;
        top_popup_hmenu = UserHMGetHandle(menu); //pPopupMenu->spmenu
    }

    NtGdiSelectFont( hdc, ghMenuFont );

     /* Clear previous highlighted item */
    if (menu->iItem != NO_SELECTED_ITEM)
    {
        menu->rgItems[menu->iItem].fState &= ~(MF_HILITE|MF_MOUSESELECT);
        MENU_DrawMenuItem(pWnd, menu, pwndOwner, hdc, &menu->rgItems[menu->iItem],
                       menu->cyMenu, !(menu->fFlags & MNF_POPUP),
                       ODA_SELECT);
    }

    /* Highlight new item (if any) */
    menu->iItem = wIndex;
    if (menu->iItem != NO_SELECTED_ITEM)
    {
        if (!(menu->rgItems[wIndex].fType & MF_SEPARATOR))
        {
             menu->rgItems[wIndex].fState |= MF_HILITE;
             MENU_EnsureMenuItemVisible(menu, wIndex, hdc);
             MENU_DrawMenuItem(pWnd, menu, pwndOwner, hdc,
                               &menu->rgItems[wIndex], menu->cyMenu, !(menu->fFlags & MNF_POPUP), ODA_SELECT);
        }
        if (sendMenuSelect)
        {
           ITEM *ip = &menu->rgItems[menu->iItem];
           WPARAM wParam = MAKEWPARAM( ip->spSubMenu ? wIndex : ip->wID, 
                                       ip->fType | ip->fState |
                                      (ip->spSubMenu ? MF_POPUP : 0) |
                                      (menu->fFlags & MNF_SYSMENU ? MF_SYSMENU : 0 ) );

           co_IntSendMessage(UserHMGetHandle(pwndOwner), WM_MENUSELECT, wParam, (LPARAM) UserHMGetHandle(menu));
        }
    }
    else if (sendMenuSelect) 
    {
        if (topmenu) 
        {
            int pos;
            pos = MENU_FindSubMenu(&topmenu, menu);
            if (pos != NO_SELECTED_ITEM)
            {
               ITEM *ip = &topmenu->rgItems[pos];
               WPARAM wParam = MAKEWPARAM( Pos, ip->fType | ip->fState |
                                           (ip->spSubMenu ? MF_POPUP : 0) |
                                           (topmenu->fFlags & MNF_SYSMENU ? MF_SYSMENU : 0 ) );

               co_IntSendMessage(UserHMGetHandle(pwndOwner), WM_MENUSELECT, wParam, (LPARAM) UserHMGetHandle(topmenu));
            }
        }
    }
    UserReleaseDC(pWnd, hdc, FALSE);
}

/***********************************************************************
 *           MenuMoveSelection
 *
 * Moves currently selected item according to the Offset parameter.
 * If there is no selection then it should select the last item if
 * Offset is ITEM_PREV or the first item if Offset is ITEM_NEXT.
 */
static void FASTCALL MENU_MoveSelection(PWND pwndOwner, PMENU menu, INT offset)
{
    INT i;

    TRACE("pwnd=%x menu=%x off=0x%04x\n", pwndOwner, menu, offset);

    if ((!menu) || (!menu->rgItems)) return;

    if ( menu->iItem != NO_SELECTED_ITEM )
    {
	if ( menu->cItems == 1 )
	   return;
	else
	for (i = menu->iItem + offset ; i >= 0 && i < menu->cItems
					    ; i += offset)
	    if (!(menu->rgItems[i].fType & MF_SEPARATOR))
	    {
		MENU_SelectItem( pwndOwner, menu, i, TRUE, 0 );
		return;
	    }
    }

    for ( i = (offset > 0) ? 0 : menu->cItems - 1;
		  i >= 0 && i < menu->cItems ; i += offset)
	if (!(menu->rgItems[i].fType & MF_SEPARATOR))
	{
	    MENU_SelectItem( pwndOwner, menu, i, TRUE, 0 );
	    return;
	}
}

/***********************************************************************
 *           MenuHideSubPopups
 *
 * Hide the sub-popup menus of this menu.
 */
static void FASTCALL MENU_HideSubPopups(PWND pWndOwner, PMENU Menu,
                               BOOL SendMenuSelect, UINT wFlags)
{
  TRACE("owner=%x menu=%x 0x%04x\n", pWndOwner, Menu, SendMenuSelect);

  if ( Menu && top_popup )
  {
      PITEM Item;

      if (Menu->iItem != NO_SELECTED_ITEM)
      {
         Item = &Menu->rgItems[Menu->iItem];
         if (!(Item->spSubMenu) ||
             !(Item->fState & MF_MOUSESELECT)) return;
         Item->fState &= ~MF_MOUSESELECT;
      }
      else
         return;

      if (Item->spSubMenu)
      {
          PWND pWnd;
          if (!VerifyMenu(Item->spSubMenu)) return;
          pWnd = ValidateHwndNoErr(Item->spSubMenu->hWnd);
          MENU_HideSubPopups(pWndOwner, Item->spSubMenu, FALSE, wFlags);
          MENU_SelectItem(pWndOwner, Item->spSubMenu, NO_SELECTED_ITEM, SendMenuSelect, NULL);
          TRACE("M_HSP top p hm %p  pWndOwner IDMenu %p\n",top_popup_hmenu,pWndOwner->IDMenu);
          co_UserDestroyWindow(pWnd);

          /* Native returns handle to destroyed window */
          if (!(wFlags & TPM_NONOTIFY))
          {
             co_IntSendMessage( UserHMGetHandle(pWndOwner), WM_UNINITMENUPOPUP, (WPARAM)UserHMGetHandle(Item->spSubMenu),
                                 MAKELPARAM(0, IS_SYSTEM_MENU(Item->spSubMenu)) );
          }
          ////
          // Call WM_UNINITMENUPOPUP FIRST before destroy!!
          // Fixes todo_wine User32 test menu.c line 2239 GetMenuBarInfo callback....
          //
          Item->spSubMenu->hWnd = NULL;
          ////
      }
  }
}

/***********************************************************************
 *           MenuShowSubPopup
 *
 * Display the sub-menu of the selected item of this menu.
 * Return the handle of the submenu, or menu if no submenu to display.
 */
static PMENU FASTCALL MENU_ShowSubPopup(PWND WndOwner, PMENU Menu, BOOL SelectFirst, UINT Flags)
{
  RECT Rect, ParentRect;
  ITEM *Item;
  HDC Dc;
  PWND pWnd;

  TRACE("owner=%x menu=%p 0x%04x\n", WndOwner, Menu, SelectFirst);

  if (!Menu) return Menu;

  if (Menu->iItem == NO_SELECTED_ITEM) return Menu;
  
  Item = &Menu->rgItems[Menu->iItem];
  if (!(Item->spSubMenu) || (Item->fState & (MF_GRAYED | MF_DISABLED)))
      return Menu;

  /* message must be sent before using item,
     because nearly everything may be changed by the application ! */

  /* Send WM_INITMENUPOPUP message only if TPM_NONOTIFY flag is not specified */
  if (!(Flags & TPM_NONOTIFY))
  {
      co_IntSendMessage(UserHMGetHandle(WndOwner), WM_INITMENUPOPUP,
                        (WPARAM) UserHMGetHandle(Item->spSubMenu),
                         MAKELPARAM(Menu->iItem, IS_SYSTEM_MENU(Menu)));
  }

  Item = &Menu->rgItems[Menu->iItem];
  //Rect = ItemInfo.Rect;
  Rect.left   = Item->xItem;
  Rect.top    = Item->yItem;
  Rect.right  = Item->cxItem; // Do this for now......
  Rect.bottom = Item->cyItem;

  pWnd = ValidateHwndNoErr(Menu->hWnd);

  /* Grab the rect of our (entire) parent menu, so we can try to not overlap it */
  if (Menu->fFlags & MNF_POPUP)
  {
    if (!IntGetWindowRect(pWnd, &ParentRect))
    {
        ERR("No pWnd\n");
        ParentRect = Rect;
    }

    /* Ensure we can slightly overlap our parent */
    RECTL_vInflateRect(&ParentRect, -UserGetSystemMetrics(SM_CXEDGE) * 2, 0);
  }
  else
  {
    /* Inside the menu bar, we do not want to grab the entire window... */
    ParentRect = Rect;
    if (pWnd)
        RECTL_vOffsetRect(&ParentRect, pWnd->rcWindow.left, pWnd->rcWindow.top);
  }

  /* correct item if modified as a reaction to WM_INITMENUPOPUP message */
  if (!(Item->fState & MF_HILITE))
  {
      if (Menu->fFlags & MNF_POPUP) Dc = UserGetDCEx(pWnd, NULL, DCX_USESTYLE);
      else Dc = UserGetDCEx(pWnd, 0, DCX_CACHE | DCX_WINDOW);

      NtGdiSelectFont(Dc, ghMenuFont);

      Item->fState |= MF_HILITE;
      MENU_DrawMenuItem(pWnd, Menu, WndOwner, Dc, Item, Menu->cyMenu,
                       !(Menu->fFlags & MNF_POPUP), ODA_DRAWENTIRE);

      UserReleaseDC(pWnd, Dc, FALSE);
  }

  if (!Item->yItem && !Item->xItem && !Item->cyItem && !Item->cxItem)
  {
      Item->xItem  = Rect.left; 
      Item->yItem  = Rect.top;  
      Item->cxItem = Rect.right; // Do this for now...... 
      Item->cyItem = Rect.bottom;
  }
  Item->fState |= MF_MOUSESELECT;

  if (IS_SYSTEM_MENU(Menu))
  {
      MENU_InitSysMenuPopup(Item->spSubMenu, pWnd->style, pWnd->pcls->style, HTSYSMENU);

      NC_GetSysPopupPos(pWnd, &Rect);
      /* Ensure we do not overlap this */
      ParentRect = Rect;
      if (Flags & TPM_LAYOUTRTL) Rect.left = Rect.right;
      Rect.top = Rect.bottom;
      Rect.right = UserGetSystemMetrics(SM_CXSIZE);
      Rect.bottom = UserGetSystemMetrics(SM_CYSIZE);
  }
  else
  {
      IntGetWindowRect(pWnd, &Rect);
      if (Menu->fFlags & MNF_POPUP)
      {
          RECT rc;
          rc.left   = Item->xItem;
          rc.top    = Item->yItem;
          rc.right  = Item->cxItem;
          rc.bottom = Item->cyItem;

          MENU_AdjustMenuItemRect(Menu, &rc);

          /* The first item in the popup menu has to be at the
             same y position as the focused menu item */
          if(Flags & TPM_LAYOUTRTL)
             Rect.left += UserGetSystemMetrics(SM_CXDLGFRAME);
          else
             Rect.left += rc.right - UserGetSystemMetrics(SM_CXDLGFRAME);

          Rect.top += rc.top;
      }
      else
      {
          if(Flags & TPM_LAYOUTRTL)
              Rect.left += Rect.right - Item->xItem; //ItemInfo.Rect.left;
          else
              Rect.left += Item->xItem; //ItemInfo.Rect.left;
          Rect.top += Item->cyItem; //ItemInfo.Rect.bottom;
          Rect.right  = Item->cxItem - Item->xItem; //ItemInfo.Rect.right - ItemInfo.Rect.left;
          Rect.bottom = Item->cyItem - Item->yItem; //ItemInfo.Rect.bottom - ItemInfo.Rect.top;
      }
  }

  /* Next menu does not need to be shown vertical anymore */
  if (Menu->fFlags & MNF_POPUP)
      Flags &= (~TPM_VERTICAL);



  /* use default alignment for submenus */
  Flags &= ~(TPM_CENTERALIGN | TPM_RIGHTALIGN | TPM_VCENTERALIGN | TPM_BOTTOMALIGN);

  MENU_InitPopup( WndOwner, Item->spSubMenu, Flags );

  MENU_ShowPopup( WndOwner, Item->spSubMenu, Menu->iItem, Flags,
                Rect.left, Rect.top, &ParentRect);
  if (SelectFirst)
  {
      MENU_MoveSelection(WndOwner, Item->spSubMenu, ITEM_NEXT);
  }
  return Item->spSubMenu;
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
static INT FASTCALL MENU_ExecFocusedItem(MTRACKER *pmt, PMENU Menu, UINT Flags)
{
  PITEM Item;

  TRACE("%p menu=%p\n", pmt, Menu);

  if (!Menu || !Menu->cItems || Menu->iItem == NO_SELECTED_ITEM)
  {
      return -1;
  }

  Item = &Menu->rgItems[Menu->iItem];

  TRACE("%p %08x %p\n", Menu, Item->wID, Item->spSubMenu);

  if (!(Item->spSubMenu))
  {
      if (!(Item->fState & (MF_GRAYED | MF_DISABLED)) && !(Item->fType & MF_SEPARATOR))
      {
          /* If TPM_RETURNCMD is set you return the id, but
            do not send a message to the owner */
          if (!(Flags & TPM_RETURNCMD))
          {
              if (Menu->fFlags & MNF_SYSMENU)
              {
                  UserPostMessage(UserHMGetHandle(pmt->OwnerWnd), WM_SYSCOMMAND, Item->wID,
                               MAKELPARAM((SHORT) pmt->Pt.x, (SHORT) pmt->Pt.y));
              }
              else
              {
                  DWORD dwStyle = ((Menu->fFlags & MNS_STYLE_MASK) | ( pmt->TopMenu ? (pmt->TopMenu->fFlags & MNS_STYLE_MASK) : 0) );

                  if (dwStyle & MNS_NOTIFYBYPOS)
                      UserPostMessage(UserHMGetHandle(pmt->OwnerWnd), WM_MENUCOMMAND, Menu->iItem, (LPARAM)UserHMGetHandle(Menu));
                  else
                      UserPostMessage(UserHMGetHandle(pmt->OwnerWnd), WM_COMMAND, Item->wID, 0);
              }
          }
          return Item->wID;
      }
  }
  else
  {
      pmt->CurrentMenu = MENU_ShowSubPopup(pmt->OwnerWnd, Menu, TRUE, Flags);
      return -2;
  }

  return -1;
}

/***********************************************************************
 *           MenuSwitchTracking
 *
 * Helper function for menu navigation routines.
 */
static void FASTCALL MENU_SwitchTracking(MTRACKER* pmt, PMENU PtMenu, UINT Index, UINT wFlags)
{
  TRACE("%x menu=%x 0x%04x\n", pmt, PtMenu, Index);

  if ( pmt->TopMenu != PtMenu &&
      !((PtMenu->fFlags | pmt->TopMenu->fFlags) & MNF_POPUP) )
  {
      /* both are top level menus (system and menu-bar) */
      MENU_HideSubPopups(pmt->OwnerWnd, pmt->TopMenu, FALSE, wFlags);
      MENU_SelectItem(pmt->OwnerWnd, pmt->TopMenu, NO_SELECTED_ITEM, FALSE, NULL);
      pmt->TopMenu = PtMenu;
  }
  else
  {
      MENU_HideSubPopups(pmt->OwnerWnd, PtMenu, FALSE, wFlags);
  }

  MENU_SelectItem(pmt->OwnerWnd, PtMenu, Index, TRUE, NULL);
}

/***********************************************************************
 *           MenuButtonDown
 *
 * Return TRUE if we can go on with menu tracking.
 */
static BOOL FASTCALL MENU_ButtonDown(MTRACKER* pmt, PMENU PtMenu, UINT Flags)
{
  TRACE("%x PtMenu=%p\n", pmt, PtMenu);

  if (PtMenu)
  {
      UINT id = 0;
      PITEM item;
      if (IS_SYSTEM_MENU(PtMenu))
      {
         item = PtMenu->rgItems;
      }
      else
      {
         item = MENU_FindItemByCoords( PtMenu, pmt->Pt, &id );
      }

      if (item)
      {
          if (PtMenu->iItem != id)
              MENU_SwitchTracking(pmt, PtMenu, id, Flags);

          /* If the popup menu is not already "popped" */
          if (!(item->fState & MF_MOUSESELECT))
          {
              pmt->CurrentMenu = MENU_ShowSubPopup(pmt->OwnerWnd, PtMenu, FALSE, Flags);
          }

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
static INT FASTCALL MENU_ButtonUp(MTRACKER *pmt, PMENU PtMenu, UINT Flags)
{
  TRACE("%p pmenu=%x\n", pmt, PtMenu);

  if (PtMenu)
  {
      UINT Id = 0;
      ITEM *item;
      
      if ( IS_SYSTEM_MENU(PtMenu) )
      {
          item = PtMenu->rgItems;
      }
      else
      {
          item = MENU_FindItemByCoords( PtMenu, pmt->Pt, &Id );
      }

      if (item && ( PtMenu->iItem == Id))
      {
          if (!(item->spSubMenu))
          {
              INT ExecutedMenuId = MENU_ExecFocusedItem( pmt, PtMenu, Flags);
              if (ExecutedMenuId == -1 || ExecutedMenuId == -2) return -1;
              return ExecutedMenuId;
          }

          /* If we are dealing with the menu bar                  */
          /* and this is a click on an already "popped" item:     */
          /* Stop the menu tracking and close the opened submenus */
          if (pmt->TopMenu == PtMenu && PtMenu->TimeToHide)
          {
              return 0;
          }
      }
      if ( IntGetMenu(PtMenu->hWnd) == PtMenu )
      {
          PtMenu->TimeToHide = TRUE;
      }
  }
  return -1;
}

/***********************************************************************
 *           MenuPtMenu
 *
 * Walks menu chain trying to find a menu pt maps to.
 */
static PMENU FASTCALL MENU_PtMenu(PMENU menu, POINT pt)
{
  PITEM pItem;
  PMENU ret = NULL;

  if (!menu) return NULL;

  /* try subpopup first (if any) */
  if (menu->iItem != NO_SELECTED_ITEM)
  {
     pItem = menu->rgItems;
     if ( pItem ) pItem = &pItem[menu->iItem];
     if ( pItem && pItem->spSubMenu && pItem->fState & MF_MOUSESELECT)
     {
        ret = MENU_PtMenu( pItem->spSubMenu, pt);
     }
  }

  /* check the current window (avoiding WM_HITTEST) */
  if (!ret)
  {
     PWND pWnd = ValidateHwndNoErr(menu->hWnd);
     INT ht = GetNCHitEx(pWnd, pt);
     if ( menu->fFlags & MNF_POPUP )
     {
        if (ht != HTNOWHERE && ht != HTERROR) ret = menu;
     }
     else if (ht == HTSYSMENU)
        ret = get_win_sys_menu(menu->hWnd);
     else if (ht == HTMENU)
        ret = IntGetMenu( menu->hWnd );
  }
  return ret;
}

/***********************************************************************
 *           MenuMouseMove
 *
 * Return TRUE if we can go on with menu tracking.
 */
static BOOL FASTCALL MENU_MouseMove(MTRACKER *pmt, PMENU PtMenu, UINT Flags)
{
  UINT Index = NO_SELECTED_ITEM;

  if ( PtMenu )
  {
      if (IS_SYSTEM_MENU(PtMenu))
      {
          Index = 0;
          //// ReactOS only HACK: CORE-2338
          // Windows tracks mouse moves to the system menu but does not open it.
          // Only keyboard tracking can do that.
          //
          TRACE("SystemMenu\n");
          return TRUE; // Stay inside the Loop!
      }
      else
          MENU_FindItemByCoords( PtMenu, pmt->Pt, &Index );
  }

  if (Index == NO_SELECTED_ITEM)
  {
      MENU_SelectItem(pmt->OwnerWnd, pmt->CurrentMenu, NO_SELECTED_ITEM, TRUE, pmt->TopMenu);
  }
  else if (PtMenu->iItem != Index)
  {
      MENU_SwitchTracking(pmt, PtMenu, Index, Flags);
      pmt->CurrentMenu = MENU_ShowSubPopup(pmt->OwnerWnd, PtMenu, FALSE, Flags);
  }
  return TRUE;
}

/***********************************************************************
 *           MenuGetSubPopup
 *
 * Return the handle of the selected sub-popup menu (if any).
 */
static PMENU MENU_GetSubPopup( PMENU menu )
{
    ITEM *item;

    if ((!menu) || (menu->iItem == NO_SELECTED_ITEM)) return 0;

    item = &menu->rgItems[menu->iItem];
    if (item && (item->spSubMenu) && (item->fState & MF_MOUSESELECT))
    {
       return item->spSubMenu;
    }
    return 0;
}

/***********************************************************************
 *           MenuDoNextMenu
 *
 * NOTE: WM_NEXTMENU documented in Win32 is a bit different.
 */
static LRESULT FASTCALL MENU_DoNextMenu(MTRACKER* pmt, UINT Vk, UINT wFlags)
{
    BOOL atEnd = FALSE;

    /* When skipping left, we need to do something special after the
       first menu.                                                  */
    if (Vk == VK_LEFT && pmt->TopMenu->iItem == 0)
    {
        atEnd = TRUE;
    }
    /* When skipping right, for the non-system menu, we need to
       handle the last non-special menu item (ie skip any window
       icons such as MDI maximize, restore or close)             */
    else if ((Vk == VK_RIGHT) && !IS_SYSTEM_MENU(pmt->TopMenu))
    {
        UINT i = pmt->TopMenu->iItem + 1;
        while (i < pmt->TopMenu->cItems) {
            if ((pmt->TopMenu->rgItems[i].wID >= SC_SIZE &&
                 pmt->TopMenu->rgItems[i].wID <= SC_RESTORE)) {
                i++;
            } else break;
        }
        if (i == pmt->TopMenu->cItems) {
            atEnd = TRUE;
        }
    }
    /* When skipping right, we need to cater for the system menu */
    else if ((Vk == VK_RIGHT) && IS_SYSTEM_MENU(pmt->TopMenu))
    {
        if (pmt->TopMenu->iItem == (pmt->TopMenu->cItems - 1)) {
            atEnd = TRUE;
        }
    }

   if ( atEnd )
   {
      MDINEXTMENU NextMenu;
      PMENU MenuTmp;
      PWND pwndTemp;
      HMENU hNewMenu;
      HWND hNewWnd;
      UINT Id = 0;

      MenuTmp = (IS_SYSTEM_MENU(pmt->TopMenu)) ? co_IntGetSubMenu(pmt->TopMenu, 0) : pmt->TopMenu;
      NextMenu.hmenuIn = UserHMGetHandle(MenuTmp);
      NextMenu.hmenuNext = NULL;
      NextMenu.hwndNext = NULL;
      co_IntSendMessage(UserHMGetHandle(pmt->OwnerWnd), WM_NEXTMENU, Vk, (LPARAM) &NextMenu);

      TRACE("%p [%p] -> %p [%p]\n",
             pmt->CurrentMenu, pmt->OwnerWnd, NextMenu.hmenuNext, NextMenu.hwndNext );

      if (NULL == NextMenu.hmenuNext || NULL == NextMenu.hwndNext)
      {
          hNewWnd = UserHMGetHandle(pmt->OwnerWnd);
          if (IS_SYSTEM_MENU(pmt->TopMenu))
          {
              /* switch to the menu bar */

              if (pmt->OwnerWnd->style & WS_CHILD || !(MenuTmp = IntGetMenu(hNewWnd))) return FALSE;

              if (Vk == VK_LEFT)
              {
                  Id = MenuTmp->cItems - 1;
   
                  /* Skip backwards over any system predefined icons,
                     eg. MDI close, restore etc icons                 */
                   while ((Id > 0) &&
                          (MenuTmp->rgItems[Id].wID >= SC_SIZE &&
                           MenuTmp->rgItems[Id].wID <= SC_RESTORE)) Id--;
 
              }
              hNewMenu = UserHMGetHandle(MenuTmp);
          }
          else if (pmt->OwnerWnd->style & WS_SYSMENU)
          {
              /* switch to the system menu */
              MenuTmp = get_win_sys_menu(hNewWnd);
              if (MenuTmp) hNewMenu = UserHMGetHandle(MenuTmp);
              else hNewMenu = NULL;
          }
          else
              return FALSE;
      }
      else    /* application returned a new menu to switch to */
      {
          hNewMenu = NextMenu.hmenuNext;
          hNewWnd = NextMenu.hwndNext;

          if ((MenuTmp = UserGetMenuObject(hNewMenu)) && (pwndTemp = ValidateHwndNoErr(hNewWnd)))
          {
              if ( pwndTemp->style & WS_SYSMENU && (get_win_sys_menu(hNewWnd) == MenuTmp) )
              {
                  /* get the real system menu */
                  MenuTmp = get_win_sys_menu(hNewWnd);
                  hNewMenu = UserHMGetHandle(MenuTmp);
              }
              else if (pwndTemp->style & WS_CHILD || IntGetMenu(hNewWnd) != MenuTmp)
              {
                  /* FIXME: Not sure what to do here;
                   * perhaps try to track NewMenu as a popup? */

                  WARN(" -- got confused.\n");
                  return FALSE;
              }
          }
          else return FALSE;
      }

      if (hNewMenu != UserHMGetHandle(pmt->TopMenu))
      {
          MENU_SelectItem(pmt->OwnerWnd, pmt->TopMenu, NO_SELECTED_ITEM, FALSE, 0 );

          if (pmt->CurrentMenu != pmt->TopMenu)
              MENU_HideSubPopups(pmt->OwnerWnd, pmt->TopMenu, FALSE, wFlags);
      }

      if (hNewWnd != UserHMGetHandle(pmt->OwnerWnd))
      {
          PTHREADINFO pti = PsGetCurrentThreadWin32Thread();
          pmt->OwnerWnd = ValidateHwndNoErr(hNewWnd);
          ///// Use thread pms!!!!
          MsqSetStateWindow(pti, MSQ_STATE_MENUOWNER, hNewWnd);
          pti->MessageQueue->QF_flags &= ~QF_CAPTURELOCKED;
          co_UserSetCapture(UserHMGetHandle(pmt->OwnerWnd));
          pti->MessageQueue->QF_flags |= QF_CAPTURELOCKED;
      }

      pmt->TopMenu = pmt->CurrentMenu = UserGetMenuObject(hNewMenu); /* all subpopups are hidden */
      MENU_SelectItem(pmt->OwnerWnd, pmt->TopMenu, Id, TRUE, 0);

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
static BOOL FASTCALL MENU_SuspendPopup(MTRACKER* pmt, UINT uMsg)
{
    MSG msg;

    msg.hwnd = UserHMGetHandle(pmt->OwnerWnd); ////// ? silly wine'isms?

    co_IntGetPeekMessage( &msg, 0, uMsg, uMsg, PM_NOYIELD | PM_REMOVE, FALSE);
    pmt->TrackFlags |= TF_SKIPREMOVE;

    switch( uMsg )
    {
    case WM_KEYDOWN:
        co_IntGetPeekMessage( &msg, 0, 0, 0, PM_NOYIELD | PM_NOREMOVE, FALSE);
        if( msg.message == WM_KEYUP || msg.message == WM_PAINT )
        {
            co_IntGetPeekMessage( &msg, 0, 0, 0, PM_NOYIELD | PM_REMOVE, FALSE);
            co_IntGetPeekMessage( &msg, 0, 0, 0, PM_NOYIELD | PM_NOREMOVE, FALSE);
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
static BOOL FASTCALL MENU_KeyEscape(MTRACKER *pmt, UINT Flags)
{
  BOOL EndMenu = TRUE;

  if (pmt->CurrentMenu != pmt->TopMenu)
  {
      if (pmt->CurrentMenu && (pmt->CurrentMenu->fFlags & MNF_POPUP))
      {
          PMENU MenuPrev, MenuTmp;

          MenuPrev = MenuTmp = pmt->TopMenu;

          /* close topmost popup */
          while (MenuTmp != pmt->CurrentMenu)
          {
              MenuPrev = MenuTmp;
              MenuTmp = MENU_GetSubPopup(MenuPrev);
          }

          MENU_HideSubPopups(pmt->OwnerWnd, MenuPrev, TRUE, Flags);
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
static void FASTCALL MENU_KeyLeft(MTRACKER* pmt, UINT Flags, UINT msg)
{
  PMENU MenuTmp, MenuPrev;
  UINT PrevCol;

  MenuPrev = MenuTmp = pmt->TopMenu;

  /* Try to move 1 column left (if possible) */
  if ( (PrevCol = MENU_GetStartOfPrevColumn(pmt->CurrentMenu)) != NO_SELECTED_ITEM)
  {
     MENU_SelectItem(pmt->OwnerWnd, pmt->CurrentMenu, PrevCol, TRUE, 0);
     return;
  }

  /* close topmost popup */
  while (MenuTmp != pmt->CurrentMenu)
  {
      MenuPrev = MenuTmp;
      MenuTmp = MENU_GetSubPopup(MenuPrev);
  }

  MENU_HideSubPopups(pmt->OwnerWnd, MenuPrev, TRUE, Flags);
  pmt->CurrentMenu = MenuPrev;

  if ((MenuPrev == pmt->TopMenu) && !(pmt->TopMenu->fFlags & MNF_POPUP))
  {
      /* move menu bar selection if no more popups are left */

      if (!MENU_DoNextMenu(pmt, VK_LEFT, Flags))
          MENU_MoveSelection(pmt->OwnerWnd, pmt->TopMenu, ITEM_PREV);

      if (MenuPrev != MenuTmp || pmt->TrackFlags & TF_SUSPENDPOPUP)
      {
          /* A sublevel menu was displayed - display the next one
           * unless there is another displacement coming up */

          if (!MENU_SuspendPopup(pmt, msg))
              pmt->CurrentMenu = MENU_ShowSubPopup(pmt->OwnerWnd, pmt->TopMenu,
                                                 TRUE, Flags);
      }
  }
}

/***********************************************************************
 *           MenuKeyRight
 *
 * Handle a VK_RIGHT key event in a menu.
 */
static void FASTCALL MENU_KeyRight(MTRACKER *pmt, UINT Flags, UINT msg)
{
    PMENU menutmp;
    UINT NextCol;

    TRACE("MenuKeyRight called, cur %p, top %p.\n",
         pmt->CurrentMenu, pmt->TopMenu);

    if ((pmt->TopMenu->fFlags & MNF_POPUP) || (pmt->CurrentMenu != pmt->TopMenu))
    {
      /* If already displaying a popup, try to display sub-popup */

      menutmp = pmt->CurrentMenu;
      pmt->CurrentMenu = MENU_ShowSubPopup(pmt->OwnerWnd, menutmp, TRUE, Flags);

      /* if subpopup was displayed then we are done */
      if (menutmp != pmt->CurrentMenu) return;
    }

    /* Check to see if there's another column */
    if ( (NextCol = MENU_GetStartOfNextColumn(pmt->CurrentMenu)) != NO_SELECTED_ITEM)
    {
       TRACE("Going to %d.\n", NextCol);
       MENU_SelectItem(pmt->OwnerWnd, pmt->CurrentMenu, NextCol, TRUE, 0);
       return;
    }

    if (!(pmt->TopMenu->fFlags & MNF_POPUP)) /* menu bar tracking */
    {
       if (pmt->CurrentMenu != pmt->TopMenu)
       {
          MENU_HideSubPopups(pmt->OwnerWnd, pmt->TopMenu, FALSE, Flags);
          menutmp = pmt->CurrentMenu = pmt->TopMenu;
       }
       else
       {
          menutmp = NULL;
       }

       /* try to move to the next item */
       if ( !MENU_DoNextMenu(pmt, VK_RIGHT, Flags))
           MENU_MoveSelection(pmt->OwnerWnd, pmt->TopMenu, ITEM_NEXT);

       if ( menutmp || pmt->TrackFlags & TF_SUSPENDPOPUP )
       {
           if ( !MENU_SuspendPopup(pmt, msg) )
               pmt->CurrentMenu = MENU_ShowSubPopup(pmt->OwnerWnd, pmt->TopMenu, TRUE, Flags);
       }
    }
}

/***********************************************************************
 *           MenuTrackMenu
 *
 * Menu tracking code.
 */
static INT FASTCALL MENU_TrackMenu(PMENU pmenu, UINT wFlags, INT x, INT y,
                            PWND pwnd)
{
    MSG msg;
    BOOL fRemove;
    INT executedMenuId = -1;
    MTRACKER mt;
    HWND capture_win;
    PMENU pmMouse;
    BOOL enterIdleSent = FALSE;
    PTHREADINFO pti = PsGetCurrentThreadWin32Thread();

    if (pti != pwnd->head.pti)
    {
       ERR("Not the same PTI!!!!\n");
    }

    mt.TrackFlags = 0;
    mt.CurrentMenu = pmenu;
    mt.TopMenu = pmenu;
    mt.OwnerWnd = pwnd;
    mt.Pt.x = x;
    mt.Pt.y = y;

    TRACE("MTM : hmenu=%p flags=0x%08x (%d,%d) hwnd=%x\n",
         UserHMGetHandle(pmenu), wFlags, x, y, UserHMGetHandle(pwnd));

    pti->MessageQueue->QF_flags &= ~QF_ACTIVATIONCHANGE;

    if (wFlags & TPM_BUTTONDOWN)
    {
        /* Get the result in order to start the tracking or not */
        fRemove = MENU_ButtonDown( &mt, pmenu, wFlags );
        fInsideMenuLoop = fRemove;
    }

    if (wFlags & TF_ENDMENU) fInsideMenuLoop = FALSE;

    if (wFlags & TPM_POPUPMENU && pmenu->cItems == 0) // Tracking empty popup menu...
    {
       MsqSetStateWindow(pti, MSQ_STATE_MENUOWNER, NULL);
       pti->MessageQueue->QF_flags &= ~QF_CAPTURELOCKED;
       co_UserSetCapture(NULL); /* release the capture */
       return 0;
    }

    capture_win = IntGetCapture();

    while (fInsideMenuLoop)
    {
        BOOL ErrorExit = FALSE;
        if (!VerifyMenu( mt.CurrentMenu )) /* sometimes happens if I do a window manager close */
           break;

        /* we have to keep the message in the queue until it's
         * clear that menu loop is not over yet. */

        for (;;)
        {
            if (co_IntGetPeekMessage( &msg, 0, 0, 0, PM_NOREMOVE, FALSE ))
            {
                if (!IntCallMsgFilter( &msg, MSGF_MENU )) break;
                /* remove the message from the queue */
                co_IntGetPeekMessage( &msg, 0, msg.message, msg.message, PM_REMOVE, FALSE );
            }
            else
            {
                /* ReactOS Checks */
                if (!VerifyWnd(mt.OwnerWnd)                            ||
                    !ValidateHwndNoErr(mt.CurrentMenu->hWnd)           ||
                     //pti->MessageQueue->QF_flags & QF_ACTIVATIONCHANGE || // See CORE-17338
                     capture_win != IntGetCapture() ) // Should not happen, but this is ReactOS...
                {
                   ErrorExit = TRUE; // Do not wait on dead windows, now win test_capture_4 works.
                   break;
                }

                if (!enterIdleSent)
                {
                  HWND win = mt.CurrentMenu->fFlags & MNF_POPUP ? mt.CurrentMenu->hWnd : NULL;
                  enterIdleSent = TRUE;
                  co_IntSendMessage( UserHMGetHandle(mt.OwnerWnd), WM_ENTERIDLE, MSGF_MENU, (LPARAM) win);
                }
                co_IntWaitMessage(NULL, 0, 0);
            }
        }

        if (ErrorExit) break; // Gracefully dropout.

        /* check if EndMenu() tried to cancel us, by posting this message */
        if (msg.message == WM_CANCELMODE)
        {
            /* we are now out of the loop */
            fInsideMenuLoop = FALSE;

            /* remove the message from the queue */
            co_IntGetPeekMessage( &msg, 0, msg.message, msg.message, PM_REMOVE, FALSE );

            /* break out of internal loop, ala ESCAPE */
            break;
        }

        mt.Pt = msg.pt;

        if ( (msg.hwnd == mt.CurrentMenu->hWnd) || ((msg.message!=WM_TIMER) && (msg.message!=WM_SYSTIMER)) )
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
            pmMouse = MENU_PtMenu( mt.TopMenu, mt.Pt );

            switch(msg.message)
            {
                /* no WM_NC... messages in captured state */

                case WM_RBUTTONDBLCLK:
                case WM_RBUTTONDOWN:
                     if (!(wFlags & TPM_RIGHTBUTTON))
                     {
                        if ( msg.message == WM_RBUTTONDBLCLK ) fInsideMenuLoop = FALSE; // Must exit or loop forever!
                        break;
                     }
                    /* fall through */
                case WM_LBUTTONDBLCLK:
                case WM_LBUTTONDOWN:
                    /* If the message belongs to the menu, removes it from the queue */
                    /* Else, end menu tracking */
                    fRemove = MENU_ButtonDown(&mt, pmMouse, wFlags);
                    fInsideMenuLoop = fRemove;
                    if ( msg.message == WM_LBUTTONDBLCLK ||
                         msg.message == WM_RBUTTONDBLCLK ) fInsideMenuLoop = FALSE; // Must exit or loop forever!
                    break;

                case WM_RBUTTONUP:
                    if (!(wFlags & TPM_RIGHTBUTTON)) break;
                    /* fall through */
                case WM_LBUTTONUP:
                    /* Check if a menu was selected by the mouse */
                    if (pmMouse)
                    {
                        executedMenuId = MENU_ButtonUp( &mt, pmMouse, wFlags);

                    /* End the loop if executedMenuId is an item ID */
                    /* or if the job was done (executedMenuId = 0). */
                        fRemove = (executedMenuId != -1);
                        fInsideMenuLoop = !fRemove;
                    }
                    /* No menu was selected by the mouse */
                    /* if the function was called by TrackPopupMenu, continue
                       with the menu tracking. If not, stop it */
                    else
                        fInsideMenuLoop = ((wFlags & TPM_POPUPMENU) ? TRUE : FALSE);

                    break;

                case WM_MOUSEMOVE:
                    /* the selected menu item must be changed every time */
                    /* the mouse moves. */

                    if (pmMouse)
                        fInsideMenuLoop |= MENU_MouseMove( &mt, pmMouse, wFlags );

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
                        fInsideMenuLoop = FALSE;
                        break;

                    case VK_HOME:
                    case VK_END:
                        MENU_SelectItem(mt.OwnerWnd, mt.CurrentMenu, NO_SELECTED_ITEM, FALSE, 0 );
                        MENU_MoveSelection(mt.OwnerWnd, mt.CurrentMenu, VK_HOME == msg.wParam ? ITEM_NEXT : ITEM_PREV);
                        break;

                    case VK_UP:
                    case VK_DOWN: /* If on menu bar, pull-down the menu */
                        if (!(mt.CurrentMenu->fFlags & MNF_POPUP))
                            mt.CurrentMenu = MENU_ShowSubPopup(mt.OwnerWnd, mt.TopMenu, TRUE, wFlags);
                        else      /* otherwise try to move selection */
                            MENU_MoveSelection(mt.OwnerWnd, mt.CurrentMenu, (msg.wParam == VK_UP)? ITEM_PREV : ITEM_NEXT );
                        break;

                    case VK_LEFT:
                        MENU_KeyLeft( &mt, wFlags, msg.message );
                        break;

                    case VK_RIGHT:
                        MENU_KeyRight( &mt, wFlags, msg.message );
                        break;

                    case VK_ESCAPE:
                        fInsideMenuLoop = !MENU_KeyEscape(&mt, wFlags);
                        break;

                    case VK_F1:
                    {
                        HELPINFO hi;
                        hi.cbSize = sizeof(HELPINFO);
                        hi.iContextType = HELPINFO_MENUITEM;
                        if (mt.CurrentMenu->iItem == NO_SELECTED_ITEM)
                            hi.iCtrlId = 0;
                        else
                            hi.iCtrlId = pmenu->rgItems[mt.CurrentMenu->iItem].wID;
                        hi.hItemHandle = UserHMGetHandle(mt.CurrentMenu);
                        hi.dwContextId = pmenu->dwContextHelpId;
                        hi.MousePos = msg.pt;
                        co_IntSendMessage( UserHMGetHandle(pwnd), WM_HELP, 0, (LPARAM)&hi);
                        break;
                    }

                    default:
                        IntTranslateKbdMessage(&msg, 0);
                        break;
                }
                break;  /* WM_KEYDOWN */

                case WM_CHAR:
                case WM_SYSCHAR:
                {
                    UINT pos;
                    BOOL fEndMenu;

                    if (msg.wParam == L'\r' || msg.wParam == L' ')
                    {
                        executedMenuId = MENU_ExecFocusedItem(&mt, mt.CurrentMenu, wFlags);
                        fEndMenu = (executedMenuId != -2);
                        fInsideMenuLoop = !fEndMenu;
                        break;
                    }

                    /* Hack to avoid control chars. */
                    /* We will find a better way real soon... */
                    if (msg.wParam < 32) break;

                    pos = MENU_FindItemByKey(mt.OwnerWnd, mt.CurrentMenu, LOWORD(msg.wParam), FALSE);

                    if (pos == (UINT)-2) fInsideMenuLoop = FALSE;
                    else if (pos == (UINT)-1) UserPostMessage(hwndSAS, WM_LOGONNOTIFY, LN_MESSAGE_BEEP, 0); //MessageBeep(0);
                    else
                    {
                        MENU_SelectItem(mt.OwnerWnd, mt.CurrentMenu, pos, TRUE, 0);
                        executedMenuId = MENU_ExecFocusedItem(&mt, mt.CurrentMenu, wFlags);
                        fEndMenu = (executedMenuId != -2);
                        fInsideMenuLoop = !fEndMenu;
                    }
                }
                break;
            }  /* switch(msg.message) - kbd */
        }
        else
        {
            co_IntGetPeekMessage( &msg, 0, msg.message, msg.message, PM_REMOVE, FALSE );
            IntDispatchMessage( &msg );
            continue;
        }

        if (fInsideMenuLoop) fRemove = TRUE;

        /* finally remove message from the queue */

        if (fRemove && !(mt.TrackFlags & TF_SKIPREMOVE) )
            co_IntGetPeekMessage( &msg, 0, msg.message, msg.message, PM_REMOVE, FALSE );
        else mt.TrackFlags &= ~TF_SKIPREMOVE;
    }

    MsqSetStateWindow(pti, MSQ_STATE_MENUOWNER, NULL);
    pti->MessageQueue->QF_flags &= ~QF_CAPTURELOCKED;
    co_UserSetCapture(NULL); /* release the capture */

    /* If dropdown is still painted and the close box is clicked on
       then the menu will be destroyed as part of the DispatchMessage above.
       This will then invalidate the menu handle in mt.hTopMenu. We should
       check for this first.  */
    if ( VerifyMenu( mt.TopMenu ) )
    {
       if (VerifyWnd(mt.OwnerWnd))
       {
           MENU_HideSubPopups(mt.OwnerWnd, mt.TopMenu, FALSE, wFlags);

           if (mt.TopMenu->fFlags & MNF_POPUP)
           {
              PWND pwndTM = ValidateHwndNoErr(mt.TopMenu->hWnd);
              if (pwndTM)
              {
                 IntNotifyWinEvent(EVENT_SYSTEM_MENUPOPUPEND, pwndTM, OBJID_CLIENT, CHILDID_SELF, 0);

                 co_UserDestroyWindow(pwndTM);
              }
              mt.TopMenu->hWnd = NULL;

              if (!(wFlags & TPM_NONOTIFY))
              {
                 co_IntSendMessage( UserHMGetHandle(mt.OwnerWnd), WM_UNINITMENUPOPUP, (WPARAM)UserHMGetHandle(mt.TopMenu),
                                 MAKELPARAM(0, IS_SYSTEM_MENU(mt.TopMenu)) );
              }
            }
            MENU_SelectItem( mt.OwnerWnd, mt.TopMenu, NO_SELECTED_ITEM, FALSE, 0 );
            co_IntSendMessage( UserHMGetHandle(mt.OwnerWnd), WM_MENUSELECT, MAKEWPARAM(0, 0xffff), 0 );
       }

       /* Reset the variable for hiding menu */
       mt.TopMenu->TimeToHide = FALSE;
    }

    EngSetLastError( ERROR_SUCCESS );
    /* The return value is only used by TrackPopupMenu */
    if (!(wFlags & TPM_RETURNCMD)) return TRUE;
    if (executedMenuId == -1) executedMenuId = 0;
    return executedMenuId;
}

/***********************************************************************
 *           MenuInitTracking
 */
static BOOL FASTCALL MENU_InitTracking(PWND pWnd, PMENU Menu, BOOL bPopup, UINT wFlags)
{
    HWND capture_win;
    PTHREADINFO pti = PsGetCurrentThreadWin32Thread();

    TRACE("hwnd=%p hmenu=%p\n", UserHMGetHandle(pWnd), UserHMGetHandle(Menu));

    co_UserHideCaret(0);

    /* This makes the menus of applications built with Delphi work.
     * It also enables menus to be displayed in more than one window,
     * but there are some bugs left that need to be fixed in this case.
     */
    if (!bPopup)
    {
        Menu->hWnd = UserHMGetHandle(pWnd);
    }

    if (!top_popup) {
       top_popup = Menu->hWnd;
       top_popup_hmenu = UserHMGetHandle(Menu);
    }

    fInsideMenuLoop = TRUE;
    fInEndMenu = FALSE;

    /* Send WM_ENTERMENULOOP and WM_INITMENU message only if TPM_NONOTIFY flag is not specified */
    if (!(wFlags & TPM_NONOTIFY))
    {
       co_IntSendMessage( UserHMGetHandle(pWnd), WM_ENTERMENULOOP, bPopup, 0 );
    }

    //
    // Capture is set before calling WM_INITMENU and after WM_ENTERMENULOOP, see msg_menu.
    //
    capture_win = (wFlags & TPM_POPUPMENU) ? Menu->hWnd : UserHMGetHandle(pWnd);
    MsqSetStateWindow(pti, MSQ_STATE_MENUOWNER, capture_win); // 1
    co_UserSetCapture(capture_win);                           // 2
    pti->MessageQueue->QF_flags |= QF_CAPTURELOCKED; // Set the Q bits so noone can change this!

    co_IntSendMessage( UserHMGetHandle(pWnd), WM_SETCURSOR, (WPARAM)UserHMGetHandle(pWnd), HTCAPTION );

    if (!(wFlags & TPM_NONOTIFY))
    {
       co_IntSendMessage( UserHMGetHandle(pWnd), WM_INITMENU, (WPARAM)UserHMGetHandle(Menu), 0 );
       /* If an app changed/recreated menu bar entries in WM_INITMENU
        * menu sizes will be recalculated once the menu created/shown.
        */
    }

    IntNotifyWinEvent( EVENT_SYSTEM_MENUSTART,
                       pWnd,
                       Menu->fFlags & MNF_SYSMENU ? OBJID_SYSMENU : OBJID_MENU,
                       CHILDID_SELF, 0);
    return TRUE;
}

/***********************************************************************
 *           MenuExitTracking
 */
static BOOL FASTCALL MENU_ExitTracking(PWND pWnd, BOOL bPopup, UINT wFlags)
{
    TRACE("Exit Track hwnd=%p bPopup %d\n", UserHMGetHandle(pWnd), bPopup);

    IntNotifyWinEvent( EVENT_SYSTEM_MENUEND, pWnd, OBJID_WINDOW, CHILDID_SELF, 0);

    if (!(wFlags & TPM_NONOTIFY))
       co_IntSendMessage( UserHMGetHandle(pWnd), WM_EXITMENULOOP, bPopup, 0 );

    co_UserShowCaret(0);

    top_popup = 0;
    top_popup_hmenu = NULL;

    return TRUE;
}

/***********************************************************************
 *           MenuTrackMouseMenuBar
 *
 * Menu-bar tracking upon a mouse event. Called from NC_HandleSysCommand().
 */
VOID MENU_TrackMouseMenuBar( PWND pWnd, ULONG ht, POINT pt)
{
    PMENU pMenu = (ht == HTSYSMENU) ? IntGetSystemMenu(pWnd, FALSE) : IntGetMenu( UserHMGetHandle(pWnd) ); // See 74276 and CORE-12801
    UINT wFlags = TPM_BUTTONDOWN | TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL;

    TRACE("wnd=%p ht=0x%04x (%ld,%ld)\n", pWnd, ht, pt.x, pt.y);

    if (pWnd->ExStyle & WS_EX_LAYOUTRTL) wFlags |= TPM_LAYOUTRTL;
    if (VerifyMenu(pMenu))
    {
        /* map point to parent client coordinates */
        PWND Parent = UserGetAncestor(pWnd, GA_PARENT );
        if (Parent != UserGetDesktopWindow())
        {
            IntScreenToClient(Parent, &pt);
        }

        MENU_InitTracking(pWnd, pMenu, FALSE, wFlags);
        /* fetch the window menu again, it may have changed */
        pMenu = (ht == HTSYSMENU) ? get_win_sys_menu( UserHMGetHandle(pWnd) ) : IntGetMenu( UserHMGetHandle(pWnd) );
        MENU_TrackMenu(pMenu, wFlags, pt.x, pt.y, pWnd);
        MENU_ExitTracking(pWnd, FALSE, wFlags);
    }
}

/***********************************************************************
 *           MenuTrackKbdMenuBar
 *
 * Menu-bar tracking upon a keyboard event. Called from NC_HandleSysCommand().
 */
VOID MENU_TrackKbdMenuBar(PWND pwnd, UINT wParam, WCHAR wChar)
{
    UINT uItem = NO_SELECTED_ITEM;
    PMENU TrackMenu;
    UINT wFlags = TPM_LEFTALIGN | TPM_LEFTBUTTON;

    TRACE("hwnd %p wParam 0x%04x wChar 0x%04x\n", UserHMGetHandle(pwnd), wParam, wChar);

    /* find window that has a menu */

    while (!( (pwnd->style & (WS_CHILD | WS_POPUP)) != WS_CHILD ) )
        if (!(pwnd = UserGetAncestor( pwnd, GA_PARENT ))) return;

    /* check if we have to track a system menu */

    TrackMenu = IntGetMenu( UserHMGetHandle(pwnd) );
    if (!TrackMenu || (pwnd->style & WS_MINIMIZE) != 0 || wChar == ' ' )
    {
        if (!(pwnd->style & WS_SYSMENU)) return;
        TrackMenu = get_win_sys_menu( UserHMGetHandle(pwnd) );
        uItem = 0;
        wParam |= HTSYSMENU; /* prevent item lookup */
    }

    if (!VerifyMenu( TrackMenu )) return;

    MENU_InitTracking( pwnd, TrackMenu, FALSE, wFlags );

    /* fetch the window menu again, it may have changed */
    TrackMenu = (wParam & HTSYSMENU) ? get_win_sys_menu( UserHMGetHandle(pwnd) ) : IntGetMenu( UserHMGetHandle(pwnd) );

    if( wChar && wChar != ' ' )
    {
        uItem = MENU_FindItemByKey( pwnd, TrackMenu, wChar, (wParam & HTSYSMENU) );
        if ( uItem >= (UINT)(-2) )
        {
            if( uItem == (UINT)(-1) ) UserPostMessage(hwndSAS, WM_LOGONNOTIFY, LN_MESSAGE_BEEP, 0); //MessageBeep(0);
            /* schedule end of menu tracking */
            wFlags |= TF_ENDMENU;
            goto track_menu;
        }
    }

    MENU_SelectItem( pwnd, TrackMenu, uItem, TRUE, 0 );

    if (!(wParam & HTSYSMENU) || wChar == ' ')
    {
        if( uItem == NO_SELECTED_ITEM )
            MENU_MoveSelection( pwnd, TrackMenu, ITEM_NEXT );
        else
            UserPostMessage( UserHMGetHandle(pwnd), WM_KEYDOWN, VK_RETURN, 0 );
    }

track_menu:
    MENU_TrackMenu( TrackMenu, wFlags, 0, 0, pwnd );
    MENU_ExitTracking( pwnd, FALSE, wFlags);
}

/**********************************************************************
 *           TrackPopupMenuEx   (USER32.@)
 */
BOOL WINAPI IntTrackPopupMenuEx( PMENU menu, UINT wFlags, int x, int y,
                              PWND pWnd, LPTPMPARAMS lpTpm)
{
    BOOL ret = FALSE;
    PTHREADINFO pti = PsGetCurrentThreadWin32Thread();

    if (pti != pWnd->head.pti)
    {
       ERR("Must be the same pti!\n");
       return ret;
    }

    TRACE("hmenu %p flags %04x (%d,%d) hwnd %p lpTpm %p \n", //rect %s\n",
            UserHMGetHandle(menu), wFlags, x, y, UserHMGetHandle(pWnd), lpTpm); //,
            //lpTpm ? wine_dbgstr_rect( &lpTpm->rcExclude) : "-" );

    if (menu->hWnd && IntIsWindow(menu->hWnd))
    {
        EngSetLastError( ERROR_POPUP_ALREADY_ACTIVE );
        return FALSE;
    }

    if (MENU_InitPopup( pWnd, menu, wFlags ))
    {
       MENU_InitTracking(pWnd, menu, TRUE, wFlags);

       /* Send WM_INITMENUPOPUP message only if TPM_NONOTIFY flag is not specified */
       if (!(wFlags & TPM_NONOTIFY))
       {
          co_IntSendMessage( UserHMGetHandle(pWnd), WM_INITMENUPOPUP, (WPARAM) UserHMGetHandle(menu), 0);
       }

       if (menu->fFlags & MNF_SYSMENU)
          MENU_InitSysMenuPopup( menu, pWnd->style, pWnd->pcls->style, HTSYSMENU);

       if (MENU_ShowPopup(pWnd, menu, 0, wFlags | TPM_POPUPMENU, x, y, lpTpm ? &lpTpm->rcExclude : NULL))
          ret = MENU_TrackMenu( menu, wFlags | TPM_POPUPMENU, 0, 0, pWnd);
       else
       {
          MsqSetStateWindow(pti, MSQ_STATE_MENUOWNER, NULL);
          pti->MessageQueue->QF_flags &= ~QF_CAPTURELOCKED;
          co_UserSetCapture(NULL); /* release the capture */
       }

       MENU_ExitTracking(pWnd, TRUE, wFlags);

       if (menu->hWnd)
       {
          PWND pwndM = ValidateHwndNoErr( menu->hWnd );
          if (pwndM) // wine hack around this with their destroy function.
          {
             if (!(pWnd->state & WNDS_DESTROYED))
                co_UserDestroyWindow( pwndM ); // Fix wrong error return.
          }
          menu->hWnd = 0;

          if (!(wFlags & TPM_NONOTIFY))
          {
             co_IntSendMessage( UserHMGetHandle(pWnd), WM_UNINITMENUPOPUP, (WPARAM)UserHMGetHandle(menu),
                                            MAKELPARAM(0, IS_SYSTEM_MENU(menu)) );
          }
       }
    }
    return ret;
}

//
// Menu Class Proc.
//
BOOL WINAPI
PopupMenuWndProc(
   PWND Wnd,
   UINT Message,
   WPARAM wParam,
   LPARAM lParam,
   LRESULT *lResult)
{
  PPOPUPMENU pPopupMenu;

  *lResult = 0;

  TRACE("PMWP : pwnd=%x msg=%d wp=0x%04lx lp=0x%08lx\n", Wnd, Message, wParam, lParam);

  if (Wnd)
  {
     if (!Wnd->fnid)
     {
        if (Message != WM_NCCREATE)
        {
           *lResult = IntDefWindowProc(Wnd, Message, wParam, lParam, FALSE);
           return TRUE;
        }
        Wnd->fnid = FNID_MENU;
        pPopupMenu = DesktopHeapAlloc( Wnd->head.rpdesk, sizeof(POPUPMENU) );
        if (pPopupMenu == NULL)
        {
            return TRUE;
        }
        pPopupMenu->posSelectedItem = NO_SELECTED_ITEM;
        pPopupMenu->spwndPopupMenu = Wnd;
        ((PMENUWND)Wnd)->ppopupmenu = pPopupMenu;
        TRACE("Pop Up Menu is Setup! Msg %d\n",Message);
        *lResult = 1;
        return TRUE;
     }
     else
     {
        if (Wnd->fnid != FNID_MENU)
        {
           ERR("Wrong window class for Menu! fnid %x\n",Wnd->fnid);
           return TRUE;
        }
        pPopupMenu = ((PMENUWND)Wnd)->ppopupmenu;
     }
  }

  switch(Message)
    {
    case WM_CREATE:
      {
        CREATESTRUCTW *cs = (CREATESTRUCTW *) lParam;
        pPopupMenu->spmenu = UserGetMenuObject(cs->lpCreateParams);
        if (pPopupMenu->spmenu)
        {
           UserReferenceObject(pPopupMenu->spmenu);
        }
        break;
      }

    case WM_MOUSEACTIVATE:  /* We don't want to be activated */
      *lResult = MA_NOACTIVATE;
      break;

    case WM_PAINT:
      {
        PAINTSTRUCT ps;
        IntBeginPaint(Wnd, &ps);
        MENU_DrawPopupMenu(Wnd, ps.hdc, pPopupMenu->spmenu);
        IntEndPaint(Wnd, &ps);
        break;
      }

    case WM_PRINTCLIENT:
      {
         MENU_DrawPopupMenu( Wnd, (HDC)wParam, pPopupMenu->spmenu);
         break;
      }

    case WM_ERASEBKGND:
      *lResult = 1;
      break;

    case WM_DESTROY:
      /* zero out global pointer in case resident popup window was destroyed. */
      if (pPopupMenu)
      {
         if (UserHMGetHandle(Wnd) == top_popup)
         {
             top_popup = NULL;
             top_popup_hmenu = NULL;
         }
      }
      else
      {
         ERR("No Window Pop Up!\n");
      }
      break;

    case WM_NCDESTROY:
      {
         if (pPopupMenu->spmenu)
         {
            IntReleaseMenuObject(pPopupMenu->spmenu);
         }
         DesktopHeapFree(Wnd->head.rpdesk, pPopupMenu );
         ((PMENUWND)Wnd)->ppopupmenu = 0;
         Wnd->fnid = FNID_DESTROY;
         break;
      }

    case MM_SETMENUHANDLE: // wine'isms
    case MN_SETHMENU:
      {
        PMENU pmenu = UserGetMenuObject((HMENU)wParam);
        if (!pmenu)
        {
           ERR("Bad Menu Handle\n");
           break;
        }
        UserReferenceObject(pmenu);
        if (pPopupMenu->spmenu)
        {
           IntReleaseMenuObject(pPopupMenu->spmenu);
        }
        pPopupMenu->spmenu = pmenu;
        break;
      }

    case MM_GETMENUHANDLE: // wine'isms
    case MN_GETHMENU:
         *lResult = (LRESULT)(pPopupMenu ? (pPopupMenu->spmenu ? UserHMGetHandle(pPopupMenu->spmenu) : NULL) : NULL);
         break;

    default:
      if (Message > MN_GETHMENU && Message < MN_GETHMENU+19)
      {
         ERR("Someone is passing unknown menu messages %d\n",Message);
      }
      TRACE("PMWP to IDWP %d\n",Message);
      *lResult = IntDefWindowProc(Wnd, Message, wParam, lParam, FALSE);
      break;
    }

  return TRUE;
}

BOOL FASTCALL
IntHiliteMenuItem(PWND WindowObject,
                  PMENU MenuObject,
                  UINT uItemHilite,
                  UINT uHilite)
{
   PITEM MenuItem;
   UINT uItem = uItemHilite;

   if (!(MenuItem = MENU_FindItem( &MenuObject, &uItem, uHilite ))) return TRUE;

   if (uHilite & MF_HILITE)
   {
      MenuItem->fState |= MF_HILITE;
   }
   else
   {
      MenuItem->fState &= ~MF_HILITE;
   }
   if (MenuObject->iItem == uItemHilite) return TRUE;
   MENU_HideSubPopups( WindowObject, MenuObject, FALSE, 0 );
   MENU_SelectItem( WindowObject, MenuObject, uItemHilite, TRUE, 0 );

   return TRUE; // Always returns true!!!!
}

BOOLEAN APIENTRY
intGetTitleBarInfo(PWND pWindowObject, PTITLEBARINFO bti)
{

    DWORD dwStyle = 0;
    DWORD dwExStyle = 0;
    BOOLEAN retValue = TRUE;

    if (bti->cbSize == sizeof(TITLEBARINFO))
    {
        RtlZeroMemory(&bti->rgstate[0],sizeof(DWORD)*(CCHILDREN_TITLEBAR+1));

        bti->rgstate[0] = STATE_SYSTEM_FOCUSABLE;

        dwStyle = pWindowObject->style;
        dwExStyle = pWindowObject->ExStyle;

        bti->rcTitleBar.top  = 0;
        bti->rcTitleBar.left = 0;
        bti->rcTitleBar.right  = pWindowObject->rcWindow.right - pWindowObject->rcWindow.left;
        bti->rcTitleBar.bottom = pWindowObject->rcWindow.bottom - pWindowObject->rcWindow.top;

        /* Is it iconiced ? */
        if ((dwStyle & WS_ICONIC)!=WS_ICONIC)
        {
            /* Remove frame from rectangle */
            if (HAS_THICKFRAME( dwStyle, dwExStyle ))
            {
                /* FIXME: Note this value should exists in pWindowObject for UserGetSystemMetrics(SM_CXFRAME) and UserGetSystemMetrics(SM_CYFRAME) */
                RECTL_vInflateRect( &bti->rcTitleBar, -UserGetSystemMetrics(SM_CXFRAME), -UserGetSystemMetrics(SM_CYFRAME) );
            }
            else if (HAS_DLGFRAME( dwStyle, dwExStyle ))
            {
                /* FIXME: Note this value should exists in pWindowObject for UserGetSystemMetrics(SM_CXDLGFRAME) and UserGetSystemMetrics(SM_CYDLGFRAME) */
                RECTL_vInflateRect( &bti->rcTitleBar, -UserGetSystemMetrics(SM_CXDLGFRAME), -UserGetSystemMetrics(SM_CYDLGFRAME));
            }
            else if (HAS_THINFRAME( dwStyle, dwExStyle))
            {
                /* FIXME: Note this value should exists in pWindowObject for UserGetSystemMetrics(SM_CXBORDER) and UserGetSystemMetrics(SM_CYBORDER) */
                RECTL_vInflateRect( &bti->rcTitleBar, -UserGetSystemMetrics(SM_CXBORDER), -UserGetSystemMetrics(SM_CYBORDER) );
            }

            /* We have additional border information if the window
             * is a child (but not an MDI child) */
            if ( (dwStyle & WS_CHILD)  &&
                 ((dwExStyle & WS_EX_MDICHILD) == 0 ) )
            {
                if (dwExStyle & WS_EX_CLIENTEDGE)
                {
                    /* FIXME: Note this value should exists in pWindowObject for UserGetSystemMetrics(SM_CXEDGE) and UserGetSystemMetrics(SM_CYEDGE) */
                    RECTL_vInflateRect (&bti->rcTitleBar, -UserGetSystemMetrics(SM_CXEDGE), -UserGetSystemMetrics(SM_CYEDGE));
                }

                if (dwExStyle & WS_EX_STATICEDGE)
                {
                    /* FIXME: Note this value should exists in pWindowObject for UserGetSystemMetrics(SM_CXBORDER) and UserGetSystemMetrics(SM_CYBORDER) */
                    RECTL_vInflateRect (&bti->rcTitleBar, -UserGetSystemMetrics(SM_CXBORDER), -UserGetSystemMetrics(SM_CYBORDER));
                }
            }
        }

        bti->rcTitleBar.top += pWindowObject->rcWindow.top;
        bti->rcTitleBar.left += pWindowObject->rcWindow.left;
        bti->rcTitleBar.right += pWindowObject->rcWindow.left;

        bti->rcTitleBar.bottom = bti->rcTitleBar.top;
        if (dwExStyle & WS_EX_TOOLWINDOW)
        {
            /* FIXME: Note this value should exists in pWindowObject for UserGetSystemMetrics(SM_CYSMCAPTION) */
            bti->rcTitleBar.bottom += UserGetSystemMetrics(SM_CYSMCAPTION);
        }
        else
        {
            /* FIXME: Note this value should exists in pWindowObject for UserGetSystemMetrics(SM_CYCAPTION) and UserGetSystemMetrics(SM_CXSIZE) */
            bti->rcTitleBar.bottom += UserGetSystemMetrics(SM_CYCAPTION);
            bti->rcTitleBar.left += UserGetSystemMetrics(SM_CXSIZE);
        }

        if (dwStyle & WS_CAPTION)
        {
            bti->rgstate[1] = STATE_SYSTEM_INVISIBLE;
            if (dwStyle & WS_SYSMENU)
            {
                if (!(dwStyle & (WS_MINIMIZEBOX|WS_MAXIMIZEBOX)))
                {
                    bti->rgstate[2] = STATE_SYSTEM_INVISIBLE;
                    bti->rgstate[3] = STATE_SYSTEM_INVISIBLE;
                }
                else
                {
                    if (!(dwStyle & WS_MINIMIZEBOX))
                    {
                        bti->rgstate[2] = STATE_SYSTEM_UNAVAILABLE;
                    }
                    if (!(dwStyle & WS_MAXIMIZEBOX))
                    {
                        bti->rgstate[3] = STATE_SYSTEM_UNAVAILABLE;
                    }
                }

                if (!(dwExStyle & WS_EX_CONTEXTHELP))
                {
                    bti->rgstate[4] = STATE_SYSTEM_INVISIBLE;
                }
                if (pWindowObject->pcls->style & CS_NOCLOSE)
                {
                    bti->rgstate[5] = STATE_SYSTEM_UNAVAILABLE;
                }
            }
            else
            {
                bti->rgstate[2] = STATE_SYSTEM_INVISIBLE;
                bti->rgstate[3] = STATE_SYSTEM_INVISIBLE;
                bti->rgstate[4] = STATE_SYSTEM_INVISIBLE;
                bti->rgstate[5] = STATE_SYSTEM_INVISIBLE;
            }
        }
        else
        {
            bti->rgstate[0] |= STATE_SYSTEM_INVISIBLE;
        }
    }
    else
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        retValue = FALSE;
    }

    return retValue;
}

DWORD FASTCALL
UserInsertMenuItem(
   PMENU Menu,
   UINT uItem,
   BOOL fByPosition,
   LPCMENUITEMINFOW UnsafeItemInfo,
   PUNICODE_STRING lpstr)
{
   NTSTATUS Status;
   ROSMENUITEMINFO ItemInfo;

   /* Try to copy the whole MENUITEMINFOW structure */
   Status = MmCopyFromCaller(&ItemInfo, UnsafeItemInfo, sizeof(MENUITEMINFOW));
   if (NT_SUCCESS(Status))
   {
      if (sizeof(MENUITEMINFOW) != ItemInfo.cbSize
         && FIELD_OFFSET(MENUITEMINFOW, hbmpItem) != ItemInfo.cbSize)
      {
         EngSetLastError(ERROR_INVALID_PARAMETER);
         return FALSE;
      }
      return IntInsertMenuItem(Menu, uItem, fByPosition, &ItemInfo, lpstr);
   }

   /* Try to copy without last field (not present in older versions) */
   Status = MmCopyFromCaller(&ItemInfo, UnsafeItemInfo, FIELD_OFFSET(MENUITEMINFOW, hbmpItem));
   if (NT_SUCCESS(Status))
   {
      if (FIELD_OFFSET(MENUITEMINFOW, hbmpItem) != ItemInfo.cbSize)
      {
         EngSetLastError(ERROR_INVALID_PARAMETER);
         return FALSE;
      }
      ItemInfo.hbmpItem = (HBITMAP)0;
      return IntInsertMenuItem(Menu, uItem, fByPosition, &ItemInfo, lpstr);
   }

   SetLastNtError(Status);
   return FALSE;
}

UINT FASTCALL IntGetMenuState( HMENU hMenu, UINT uId, UINT uFlags)
{
   PMENU MenuObject;
   PITEM pItem;

   if (!(MenuObject = UserGetMenuObject(hMenu)))
   {
      return (UINT)-1;
   }

   if (!(pItem = MENU_FindItem( &MenuObject, &uId, uFlags ))) return -1;

   if (pItem->spSubMenu)
   {
      return (pItem->spSubMenu->cItems << 8) | ((pItem->fState|pItem->fType|MF_POPUP) & 0xff);
   }
   else
      return (pItem->fType | pItem->fState);
}

HMENU FASTCALL IntGetSubMenu( HMENU hMenu, int nPos)
{
   PMENU MenuObject;
   PITEM pItem;

   if (!(MenuObject = UserGetMenuObject(hMenu)))
   {
      return NULL;
   }

   if (!(pItem = MENU_FindItem( &MenuObject, (UINT*)&nPos, MF_BYPOSITION ))) return NULL;

   if (pItem->spSubMenu)
   {
      HMENU hsubmenu = UserHMGetHandle(pItem->spSubMenu);
      return hsubmenu;
   }
   return NULL;
}

UINT FASTCALL IntFindSubMenu(HMENU *hMenu, HMENU hSubTarget )
{
    PMENU menu, pSubTarget;
    UINT Pos;
    if (((*hMenu)==(HMENU)0xffff) ||(!(menu = UserGetMenuObject(*hMenu))))
        return NO_SELECTED_ITEM;

    pSubTarget = UserGetMenuObject(hSubTarget);

    Pos = MENU_FindSubMenu(&menu, pSubTarget );

    *hMenu = (menu ? UserHMGetHandle(menu) : NULL);

    return Pos;
}


HMENU FASTCALL UserCreateMenu(PDESKTOP Desktop, BOOL PopupMenu)
{
   PWINSTATION_OBJECT WinStaObject;
   HANDLE Handle;
   PMENU Menu;
   NTSTATUS Status;
   PEPROCESS CurrentProcess = PsGetCurrentProcess();

   if (gpepCSRSS != CurrentProcess)
   {
      /*
       * gpepCSRSS does not have a Win32WindowStation
       */

      Status = IntValidateWindowStationHandle(CurrentProcess->Win32WindowStation,
                     UserMode,
                     0,
                     &WinStaObject,
                     0);

       if (!NT_SUCCESS(Status))
       {
          ERR("Validation of window station handle (%p) failed\n",
              CurrentProcess->Win32WindowStation);
          SetLastNtError(Status);
          return (HMENU)0;
       }
       Menu = IntCreateMenu(&Handle, !PopupMenu, Desktop, GetW32ProcessInfo());
       if (Menu && Menu->head.rpdesk->rpwinstaParent != WinStaObject)
       {
          ERR("Desktop Window Station does not match Process one!\n");
       }
       ObDereferenceObject(WinStaObject);
   }
   else
   {
       Menu = IntCreateMenu(&Handle, !PopupMenu, GetW32ThreadInfo()->rpdesk, GetW32ProcessInfo());
   }

   if (Menu) UserDereferenceObject(Menu);
   return (HMENU)Handle;
}

BOOL FASTCALL
IntMenuItemInfo(
   PMENU Menu,
   UINT Item,
   BOOL ByPosition,
   PROSMENUITEMINFO ItemInfo,
   BOOL SetOrGet,
   PUNICODE_STRING lpstr)
{
   PITEM MenuItem;
   BOOL Ret;

   if (!(MenuItem = MENU_FindItem( &Menu, &Item, (ByPosition ? MF_BYPOSITION : MF_BYCOMMAND) )))
   {
      EngSetLastError(ERROR_MENU_ITEM_NOT_FOUND);
      return( FALSE);
   }
   if (SetOrGet)
   {
      Ret = IntSetMenuItemInfo(Menu, MenuItem, ItemInfo, lpstr);
   }
   else
   {
      Ret = IntGetMenuItemInfo(Menu, MenuItem, ItemInfo);
   }
   return( Ret);
}

BOOL FASTCALL
UserMenuItemInfo(
   PMENU Menu,
   UINT Item,
   BOOL ByPosition,
   PROSMENUITEMINFO UnsafeItemInfo,
   BOOL SetOrGet,
   PUNICODE_STRING lpstr)
{
   PITEM MenuItem;
   ROSMENUITEMINFO ItemInfo;
   NTSTATUS Status;
   UINT Size;
   BOOL Ret;

   Status = MmCopyFromCaller(&Size, &UnsafeItemInfo->cbSize, sizeof(UINT));
   if (! NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      return( FALSE);
   }
   if ( Size != sizeof(MENUITEMINFOW) && 
        Size != FIELD_OFFSET(MENUITEMINFOW, hbmpItem) &&
        Size != sizeof(ROSMENUITEMINFO) )
   {
      EngSetLastError(ERROR_INVALID_PARAMETER);
      return( FALSE);
   }
   Status = MmCopyFromCaller(&ItemInfo, UnsafeItemInfo, Size);
   if (! NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      return( FALSE);
   }
   /* If this is a pre-0x0500 _WIN32_WINNT MENUITEMINFOW, you can't
      set/get hbmpItem */
   if (FIELD_OFFSET(MENUITEMINFOW, hbmpItem) == Size
         && 0 != (ItemInfo.fMask & MIIM_BITMAP))
   {
      EngSetLastError(ERROR_INVALID_PARAMETER);
      return( FALSE);
   }

   if (!(MenuItem = MENU_FindItem( &Menu, &Item, (ByPosition ? MF_BYPOSITION : MF_BYCOMMAND) )))
   {
      /* workaround for Word 95: pretend that SC_TASKLIST item exists. */
      if ( SetOrGet && Item == SC_TASKLIST && !ByPosition )
         return TRUE;

      EngSetLastError(ERROR_MENU_ITEM_NOT_FOUND);
      return( FALSE);
   }

   if (SetOrGet)
   {
      Ret = IntSetMenuItemInfo(Menu, MenuItem, &ItemInfo, lpstr);
   }
   else
   {
      Ret = IntGetMenuItemInfo(Menu, MenuItem, &ItemInfo);
      if (Ret)
      {
         Status = MmCopyToCaller(UnsafeItemInfo, &ItemInfo, Size);
         if (! NT_SUCCESS(Status))
         {
            SetLastNtError(Status);
            return( FALSE);
         }
      }
   }

   return( Ret);
}

BOOL FASTCALL
UserMenuInfo(
   PMENU Menu,
   PROSMENUINFO UnsafeMenuInfo,
   BOOL SetOrGet)
{
   BOOL Res;
   DWORD Size;
   NTSTATUS Status;
   ROSMENUINFO MenuInfo;

   Status = MmCopyFromCaller(&Size, &UnsafeMenuInfo->cbSize, sizeof(DWORD));
   if (! NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      return( FALSE);
   }
   if ( Size < sizeof(MENUINFO) || Size > sizeof(ROSMENUINFO) )
   {
      EngSetLastError(ERROR_INVALID_PARAMETER);
      return( FALSE);
   }
   Status = MmCopyFromCaller(&MenuInfo, UnsafeMenuInfo, Size);
   if (! NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      return( FALSE);
   }

   if(SetOrGet)
   {
      /* Set MenuInfo */
      Res = IntSetMenuInfo(Menu, &MenuInfo);
   }
   else
   {
      /* Get MenuInfo */
      Res = IntGetMenuInfo(Menu, &MenuInfo);
      if (Res)
      {
         Status = MmCopyToCaller(UnsafeMenuInfo, &MenuInfo, Size);
         if (! NT_SUCCESS(Status))
         {
            SetLastNtError(Status);
            return( FALSE);
         }
      }
   }

   return( Res);
}

BOOL FASTCALL
IntGetMenuItemRect(
   PWND pWnd,
   PMENU Menu,
   UINT uItem,
   PRECTL Rect)
{
   LONG XMove, YMove;
   PITEM MenuItem;
   UINT I = uItem;

   if ((MenuItem = MENU_FindItem (&Menu, &I, MF_BYPOSITION)))
   {
      Rect->left   = MenuItem->xItem;
      Rect->top    = MenuItem->yItem;
      Rect->right  = MenuItem->cxItem; // Do this for now......
      Rect->bottom = MenuItem->cyItem;
   }
   else
   {
      ERR("Failed Item Lookup! %u\n", uItem);
      return FALSE;
   }

   if (!pWnd)
   {
      HWND hWnd = Menu->hWnd;
      if (!(pWnd = UserGetWindowObject(hWnd))) return FALSE;
   }

   if (Menu->fFlags & MNF_POPUP)
   {
     XMove = pWnd->rcClient.left;
     YMove = pWnd->rcClient.top;
   }
   else
   {
     XMove = pWnd->rcWindow.left;
     YMove = pWnd->rcWindow.top;
   }

   Rect->left   += XMove;
   Rect->top    += YMove;
   Rect->right  += XMove;
   Rect->bottom += YMove;

   return TRUE;
}

PMENU FASTCALL MENU_GetSystemMenu(PWND Window, PMENU Popup)
{
   PMENU Menu, NewMenu = NULL, SysMenu = NULL;
   HMENU hSysMenu, hNewMenu = NULL;
   ROSMENUITEMINFO ItemInfoSet = {0};
   ROSMENUITEMINFO ItemInfo = {0};
   UNICODE_STRING MenuName;

   hSysMenu = UserCreateMenu(Window->head.rpdesk, FALSE);
   if (NULL == hSysMenu)
   {
      return NULL;
   }
   SysMenu = UserGetMenuObject(hSysMenu);
   if (NULL == SysMenu)
   {
       UserDestroyMenu(hSysMenu);
       return NULL;
   }

   SysMenu->fFlags |= MNF_SYSMENU;
   SysMenu->hWnd = UserHMGetHandle(Window);

   if (!Popup)
   {
      //hNewMenu = co_IntLoadSysMenuTemplate();
      if ( Window->ExStyle & WS_EX_MDICHILD )
      {
         RtlInitUnicodeString( &MenuName, L"SYSMENUMDI");
         hNewMenu = co_IntCallLoadMenu( hModClient, &MenuName);
      }
      else
      {
         RtlInitUnicodeString( &MenuName, L"SYSMENU");
         hNewMenu = co_IntCallLoadMenu( hModClient, &MenuName);
         //ERR("%wZ\n",&MenuName);
      }
      if (!hNewMenu)
      {
         ERR("No Menu!!\n");
         IntDestroyMenuObject(SysMenu, FALSE);
         return NULL;
      }
      Menu = UserGetMenuObject(hNewMenu);
      if (!Menu)
      {
         IntDestroyMenuObject(SysMenu, FALSE);
         return NULL;
      }

      // Do the rest in here.

      Menu->fFlags |= MNS_CHECKORBMP | MNF_SYSMENU  | MNF_POPUP;

      ItemInfoSet.cbSize = sizeof( MENUITEMINFOW);
      ItemInfoSet.fMask = MIIM_BITMAP;
      ItemInfoSet.hbmpItem = HBMMENU_POPUP_CLOSE;
      IntMenuItemInfo(Menu, SC_CLOSE, FALSE, &ItemInfoSet, TRUE, NULL);
      ItemInfoSet.hbmpItem = HBMMENU_POPUP_RESTORE;
      IntMenuItemInfo(Menu, SC_RESTORE, FALSE, &ItemInfoSet, TRUE, NULL);
      ItemInfoSet.hbmpItem = HBMMENU_POPUP_MAXIMIZE;
      IntMenuItemInfo(Menu, SC_MAXIMIZE, FALSE, &ItemInfoSet, TRUE, NULL);
      ItemInfoSet.hbmpItem = HBMMENU_POPUP_MINIMIZE;
      IntMenuItemInfo(Menu, SC_MINIMIZE, FALSE, &ItemInfoSet, TRUE, NULL);

      NewMenu = IntCloneMenu(Menu);
      if (NewMenu == NULL)
      {
         IntDestroyMenuObject(Menu, FALSE);
         IntDestroyMenuObject(SysMenu, FALSE);
         return NULL;
      }

      IntReleaseMenuObject(NewMenu);
      UserSetMenuDefaultItem(NewMenu, SC_CLOSE, FALSE);

      IntDestroyMenuObject(Menu, FALSE);
   }
   else
   {
      NewMenu = Popup;
   }
   if (NewMenu)
   {
      NewMenu->fFlags |= MNF_SYSMENU | MNF_POPUP;

      if (Window->pcls->style & CS_NOCLOSE)
         IntRemoveMenuItem(NewMenu, SC_CLOSE, MF_BYCOMMAND, TRUE);

      ItemInfo.cbSize = sizeof(MENUITEMINFOW);
      ItemInfo.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_STATE | MIIM_SUBMENU;
      ItemInfo.fType = MF_POPUP;
      ItemInfo.fState = MFS_ENABLED;
      ItemInfo.dwTypeData = NULL;
      ItemInfo.cch = 0;
      ItemInfo.hSubMenu = UserHMGetHandle(NewMenu);
      IntInsertMenuItem(SysMenu, (UINT) -1, TRUE, &ItemInfo, NULL);

      return SysMenu;
   }
   ERR("failed to load system menu!\n");
   return NULL;
}

PMENU FASTCALL
IntGetSystemMenu(PWND Window, BOOL bRevert)
{
   PMENU Menu;

   if (bRevert)
   {
      if (Window->SystemMenu)
      {
         Menu = UserGetMenuObject(Window->SystemMenu);
         if (Menu && !(Menu->fFlags & MNF_SYSDESKMN))
         {
            IntDestroyMenuObject(Menu, TRUE);
            Window->SystemMenu = NULL;
         }
      }
   }
   else
   {
      Menu = Window->SystemMenu ? UserGetMenuObject(Window->SystemMenu) : NULL;
      if ((!Menu || Menu->fFlags & MNF_SYSDESKMN) && Window->style & WS_SYSMENU)
      {
         Menu = MENU_GetSystemMenu(Window, NULL);
         Window->SystemMenu = Menu ? UserHMGetHandle(Menu) : NULL;
      }
   }

   if (Window->SystemMenu)
   {
      HMENU hMenu = IntGetSubMenu( Window->SystemMenu, 0);
      /* Store the dummy sysmenu handle to facilitate the refresh */
      /* of the close button if the SC_CLOSE item change */
      Menu = UserGetMenuObject(hMenu);
      if (Menu)
      {
         Menu->spwndNotify = Window;
         Menu->fFlags |= MNF_SYSSUBMENU;
      }
      return Menu;
   }
   return NULL;
}

BOOL FASTCALL
IntSetSystemMenu(PWND Window, PMENU Menu)
{
   PMENU OldMenu;

   if (!(Window->style & WS_SYSMENU)) return FALSE;

   if (Window->SystemMenu)
   {
      OldMenu = UserGetMenuObject(Window->SystemMenu);
      if (OldMenu)
      {
          OldMenu->fFlags &= ~MNF_SYSMENU;
          IntDestroyMenuObject(OldMenu, TRUE);
      }
   }

   OldMenu = MENU_GetSystemMenu(Window, Menu);
   if (OldMenu)
   {  // Use spmenuSys too!
      Window->SystemMenu = UserHMGetHandle(OldMenu);
   }
   else
      Window->SystemMenu = NULL;

   if (Menu && Window != Menu->spwndNotify)
   {
      Menu->spwndNotify = Window;
   }

   return TRUE;
}

BOOL FASTCALL
IntSetMenu(
   PWND Wnd,
   HMENU Menu,
   BOOL *Changed)
{
   PMENU OldMenu, NewMenu = NULL;

   if ((Wnd->style & (WS_CHILD | WS_POPUP)) == WS_CHILD)
   {
      ERR("SetMenu: Window is a Child 0x%p!\n",UserHMGetHandle(Wnd));
      EngSetLastError(ERROR_INVALID_WINDOW_HANDLE);
      return FALSE;
   }

   *Changed = (UlongToHandle(Wnd->IDMenu) != Menu);
   if (! *Changed)
   {
      return TRUE;
   }

   if (Wnd->IDMenu)
   {
      OldMenu = IntGetMenuObject(UlongToHandle(Wnd->IDMenu));
      ASSERT(NULL == OldMenu || OldMenu->hWnd == UserHMGetHandle(Wnd));
   }
   else
   {
      OldMenu = NULL;
   }

   if (NULL != Menu)
   {
      NewMenu = IntGetMenuObject(Menu);
      if (NULL == NewMenu)
      {
         if (NULL != OldMenu)
         {
            IntReleaseMenuObject(OldMenu);
         }
         EngSetLastError(ERROR_INVALID_MENU_HANDLE);
         return FALSE;
      }
      if (NULL != NewMenu->hWnd)
      {
         /* Can't use the same menu for two windows */
         if (NULL != OldMenu)
         {
            IntReleaseMenuObject(OldMenu);
         }
         EngSetLastError(ERROR_INVALID_MENU_HANDLE);
         return FALSE;
      }

   }

   Wnd->IDMenu = (UINT_PTR) Menu;
   if (NULL != NewMenu)
   {
      NewMenu->hWnd = UserHMGetHandle(Wnd);
      IntReleaseMenuObject(NewMenu);
   }
   if (NULL != OldMenu)
   {
      OldMenu->hWnd = NULL;
      IntReleaseMenuObject(OldMenu);
   }

   return TRUE;
}


/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
/* http://www.cyber-ta.org/releases/malware-analysis/public/SOURCES/b47155634ccb2c30630da7e3666d3d07/b47155634ccb2c30630da7e3666d3d07.trace.html#NtUserGetIconSize */
DWORD
APIENTRY
NtUserCalcMenuBar(
    HWND   hwnd,
    DWORD  leftBorder,
    DWORD  rightBorder,
    DWORD  top,
    LPRECT prc )
{
    HDC hdc;
    PWND Window;
    RECT Rect;
    DWORD ret;

    UserEnterExclusive();

    if(!(Window = UserGetWindowObject(hwnd)))
    {
        EngSetLastError(ERROR_INVALID_WINDOW_HANDLE);
        UserLeave();
        return 0;
    }

    hdc = UserGetDCEx(NULL, NULL, DCX_CACHE);
    if (!hdc)
    {
        UserLeave();
        return 0;
    }

    Rect.left = leftBorder;
    Rect.right = Window->rcWindow.right - Window->rcWindow.left - rightBorder;
    Rect.top = top;
    Rect.bottom = 0;

    ret = MENU_DrawMenuBar(hdc, &Rect, Window, TRUE);

    UserReleaseDC( 0, hdc, FALSE );

    UserLeave();

    return ret;
}

/*
 * @implemented
 */
DWORD APIENTRY
NtUserCheckMenuItem(
   HMENU hMenu,
   UINT uIDCheckItem,
   UINT uCheck)
{
   PMENU Menu;
   DECLARE_RETURN(DWORD);

   TRACE("Enter NtUserCheckMenuItem\n");
   UserEnterExclusive();

   if(!(Menu = UserGetMenuObject(hMenu)))
   {
      RETURN( (DWORD)-1);
   }

   RETURN( IntCheckMenuItem(Menu, uIDCheckItem, uCheck));

CLEANUP:
   TRACE("Leave NtUserCheckMenuItem, ret=%lu\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * @implemented
 */
BOOL APIENTRY
NtUserDeleteMenu(
   HMENU hMenu,
   UINT uPosition,
   UINT uFlags)
{
   PMENU Menu;
   DECLARE_RETURN(BOOL);

   TRACE("Enter NtUserDeleteMenu\n");
   UserEnterExclusive();

   if(!(Menu = UserGetMenuObject(hMenu)))
   {
      RETURN( FALSE);
   }

   RETURN( IntRemoveMenuItem(Menu, uPosition, uFlags, TRUE));

CLEANUP:
   TRACE("Leave NtUserDeleteMenu, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * NtUserGetSystemMenu
 *
 * The NtUserGetSystemMenu function allows the application to access the
 * window menu (also known as the system menu or the control menu) for
 * copying and modifying.
 *
 * Parameters
 *    hWnd
 *       Handle to the window that will own a copy of the window menu.
 *    bRevert
 *       Specifies the action to be taken. If this parameter is FALSE,
 *       NtUserGetSystemMenu returns a handle to the copy of the window menu
 *       currently in use. The copy is initially identical to the window menu
 *       but it can be modified.
 *       If this parameter is TRUE, GetSystemMenu resets the window menu back
 *       to the default state. The previous window menu, if any, is destroyed.
 *
 * Return Value
 *    If the bRevert parameter is FALSE, the return value is a handle to a
 *    copy of the window menu. If the bRevert parameter is TRUE, the return
 *    value is NULL.
 *
 * Status
 *    @implemented
 */

HMENU APIENTRY
NtUserGetSystemMenu(HWND hWnd, BOOL bRevert)
{
   PWND Window;
   PMENU Menu;
   DECLARE_RETURN(HMENU);

   TRACE("Enter NtUserGetSystemMenu\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN(NULL);
   }

   if (!(Menu = IntGetSystemMenu(Window, bRevert)))
   {
      RETURN(NULL);
   }

   RETURN(Menu->head.h);

CLEANUP:
   TRACE("Leave NtUserGetSystemMenu, ret=%p\n", _ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * NtUserSetSystemMenu
 *
 * Status
 *    @implemented
 */

BOOL APIENTRY
NtUserSetSystemMenu(HWND hWnd, HMENU hMenu)
{
   BOOL Result = FALSE;
   PWND Window;
   PMENU Menu;
   DECLARE_RETURN(BOOL);

   TRACE("Enter NtUserSetSystemMenu\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN( FALSE);
   }

   if (hMenu)
   {
      /*
       * Assign new menu handle and Up the Lock Count.
       */
      if (!(Menu = IntGetMenuObject(hMenu)))
      {
         RETURN( FALSE);
      }

      Result = IntSetSystemMenu(Window, Menu);
   }
   else
      EngSetLastError(ERROR_INVALID_MENU_HANDLE);

   RETURN( Result);

CLEANUP:
   TRACE("Leave NtUserSetSystemMenu, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * @implemented
 */
BOOLEAN APIENTRY
NtUserGetTitleBarInfo(
    HWND hwnd,
    PTITLEBARINFO bti)
{
    PWND WindowObject;
    TITLEBARINFO bartitleinfo;
    DECLARE_RETURN(BOOLEAN);
    BOOLEAN retValue = TRUE;

    TRACE("Enter NtUserGetTitleBarInfo\n");
    UserEnterExclusive();

    /* Vaildate the windows handle */
    if (!(WindowObject = UserGetWindowObject(hwnd)))
    {
        EngSetLastError(ERROR_INVALID_WINDOW_HANDLE);
        retValue = FALSE;
    }

    _SEH2_TRY
    {
        /* Copy our usermode buffer bti to local buffer bartitleinfo */
        ProbeForRead(bti, sizeof(TITLEBARINFO), 1);
        RtlCopyMemory(&bartitleinfo, bti, sizeof(TITLEBARINFO));
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Fail copy the data */
        EngSetLastError(ERROR_INVALID_PARAMETER);
        retValue = FALSE;
    }
    _SEH2_END

    /* Get the tile bar info */
    if (retValue)
    {
        retValue = intGetTitleBarInfo(WindowObject, &bartitleinfo);
        if (retValue)
        {
            _SEH2_TRY
            {
                /* Copy our buffer to user mode buffer bti */
                ProbeForWrite(bti, sizeof(TITLEBARINFO), 1);
                RtlCopyMemory(bti, &bartitleinfo, sizeof(TITLEBARINFO));
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Fail copy the data */
                EngSetLastError(ERROR_INVALID_PARAMETER);
                retValue = FALSE;
            }
            _SEH2_END
        }
    }

    RETURN( retValue );

CLEANUP:
    TRACE("Leave NtUserGetTitleBarInfo, ret=%u\n",_ret_);
    UserLeave();
    END_CLEANUP;
}

/*
 * @implemented
 */
BOOL FASTCALL UserDestroyMenu(HMENU hMenu)
{
   PMENU Menu;
   PTHREADINFO pti = PsGetCurrentThreadWin32Thread();

   if(!(Menu = UserGetMenuObject(hMenu)))
   {
      return FALSE;
   }

   if (Menu->head.rpdesk != pti->rpdesk)
   {
      EngSetLastError(ERROR_ACCESS_DENIED);
      return FALSE;
   }
   return IntDestroyMenuObject(Menu, FALSE);
}

/*
 * @implemented
 */
BOOL APIENTRY
NtUserDestroyMenu(
   HMENU hMenu)
{
   PMENU Menu;
   DECLARE_RETURN(BOOL);

   TRACE("Enter NtUserDestroyMenu\n");
   UserEnterExclusive();

   if(!(Menu = UserGetMenuObject(hMenu)))
   {
      RETURN( FALSE);
   }
   if (Menu->head.rpdesk != gptiCurrent->rpdesk)
   {
      EngSetLastError(ERROR_ACCESS_DENIED);
      RETURN( FALSE);
   }
   RETURN( IntDestroyMenuObject(Menu, TRUE));

CLEANUP:
   TRACE("Leave NtUserDestroyMenu, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * @implemented
 */
UINT APIENTRY
NtUserEnableMenuItem(
   HMENU hMenu,
   UINT uIDEnableItem,
   UINT uEnable)
{
   PMENU Menu;
   DECLARE_RETURN(UINT);

   TRACE("Enter NtUserEnableMenuItem\n");
   UserEnterExclusive();

   if(!(Menu = UserGetMenuObject(hMenu)))
   {
      RETURN(-1);
   }

   RETURN( IntEnableMenuItem(Menu, uIDEnableItem, uEnable));

CLEANUP:
   TRACE("Leave NtUserEnableMenuItem, ret=%u\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * @implemented
 */
BOOL APIENTRY
NtUserEndMenu(VOID)
{
   //PWND pWnd;
   TRACE("Enter NtUserEndMenu\n");
   UserEnterExclusive();
 /*  if ( gptiCurrent->pMenuState &&
        gptiCurrent->pMenuState->pGlobalPopupMenu )
   {
       pWnd = IntGetMSWND(gptiCurrent->pMenuState);
       if (pWnd)
       {
          UserPostMessage( UserHMGetHandle(pWnd), WM_CANCELMODE, 0, 0);
       }
       else
          gptiCurrent->pMenuState->fInsideMenuLoop = FALSE;
   }*/
   if (fInsideMenuLoop && top_popup)
   {
      fInsideMenuLoop = FALSE;
      UserPostMessage( top_popup, WM_CANCELMODE, 0, 0);
   }
   UserLeave();
   TRACE("Leave NtUserEndMenu\n");
   return TRUE;
}

/*
 * @implemented
 */
BOOL APIENTRY
NtUserGetMenuBarInfo(
   HWND hwnd,
   LONG idObject,
   LONG idItem,
   PMENUBARINFO pmbi)
{
   PWND pWnd;
   HMENU hMenu;
   MENUBARINFO kmbi;
   BOOL Ret;
   PPOPUPMENU pPopupMenu;
   USER_REFERENCE_ENTRY Ref;
   NTSTATUS Status = STATUS_SUCCESS;
   PMENU Menu = NULL;
   DECLARE_RETURN(BOOL);

   TRACE("Enter NtUserGetMenuBarInfo\n");
   UserEnterShared();

   if (!(pWnd = UserGetWindowObject(hwnd)))
   {
        EngSetLastError(ERROR_INVALID_WINDOW_HANDLE);
        RETURN(FALSE);
   }

   UserRefObjectCo(pWnd, &Ref);

   RECTL_vSetEmptyRect(&kmbi.rcBar);
   kmbi.hMenu = NULL;
   kmbi.hwndMenu = NULL;
   kmbi.fBarFocused = FALSE;
   kmbi.fFocused = FALSE;

   switch (idObject)
   {
    case OBJID_CLIENT:
        if (!pWnd->pcls->fnid)
            RETURN(FALSE);
        if (pWnd->pcls->fnid != FNID_MENU)
        {
            WARN("called on invalid window: %u\n", pWnd->pcls->fnid);
            EngSetLastError(ERROR_INVALID_MENU_HANDLE);
            RETURN(FALSE);
        }
        // Windows does this! Wine checks for Atom and uses GetWindowLongPtrW.
        hMenu = (HMENU)co_IntSendMessage(hwnd, MN_GETHMENU, 0, 0);
        pPopupMenu = ((PMENUWND)pWnd)->ppopupmenu;
        if (pPopupMenu && pPopupMenu->spmenu)
        {
           if (UserHMGetHandle(pPopupMenu->spmenu) != hMenu)
           {
              ERR("Window Pop Up hMenu %p not the same as Get hMenu %p!\n",pPopupMenu->spmenu->head.h,hMenu);
           }
        }
        break;
    case OBJID_MENU:
        if (pWnd->style & WS_CHILD) RETURN(FALSE);
        hMenu = UlongToHandle(pWnd->IDMenu);
        TRACE("GMBI: OBJID_MENU hMenu %p\n",hMenu);
        break;
    case OBJID_SYSMENU:
        if (!(pWnd->style & WS_SYSMENU)) RETURN(FALSE);
        Menu = IntGetSystemMenu(pWnd, FALSE);
        hMenu = UserHMGetHandle(Menu);
        break;
    default:
        RETURN(FALSE);
   }

   if (!hMenu)
      RETURN(FALSE);

   _SEH2_TRY
   {
       ProbeForRead(pmbi, sizeof(MENUBARINFO), 1);
       kmbi.cbSize = pmbi->cbSize;
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
       kmbi.cbSize = 0;
   }
   _SEH2_END

   if (kmbi.cbSize != sizeof(MENUBARINFO))
   {
       EngSetLastError(ERROR_INVALID_PARAMETER);
       RETURN(FALSE);
   }

   if (!Menu) Menu = UserGetMenuObject(hMenu);
   if (!Menu)
       RETURN(FALSE);

   if ((idItem < 0) || ((ULONG)idItem > Menu->cItems))
       RETURN(FALSE);

   if (idItem == 0)
   {
      Ret = IntGetMenuItemRect(pWnd, Menu, 0, &kmbi.rcBar);
      kmbi.rcBar.right = kmbi.rcBar.left + Menu->cxMenu;
      kmbi.rcBar.bottom = kmbi.rcBar.top + Menu->cyMenu;
      TRACE("idItem a 0 %d\n",Ret);
   }
   else
   {
      Ret = IntGetMenuItemRect(pWnd, Menu, idItem-1, &kmbi.rcBar);
      TRACE("idItem b %d %d\n", idItem-1, Ret);
   }

   kmbi.hMenu = hMenu;
   kmbi.fBarFocused = top_popup_hmenu == hMenu;
   TRACE("GMBI: top p hm %p hMenu %p\n",top_popup_hmenu, hMenu);
   if (idItem)
   {
       kmbi.fFocused = Menu->iItem == idItem-1;
       if (kmbi.fFocused && (Menu->rgItems[idItem - 1].spSubMenu))
       {
          kmbi.hwndMenu = Menu->rgItems[idItem - 1].spSubMenu->hWnd;
       }
   }
   else
   {
       kmbi.fFocused = kmbi.fBarFocused;
   }

   _SEH2_TRY
   {
      ProbeForWrite(pmbi, sizeof(MENUBARINFO), 1);
      RtlCopyMemory(pmbi, &kmbi, sizeof(MENUBARINFO));
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
      Status = _SEH2_GetExceptionCode();
   }
   _SEH2_END

   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN(FALSE);
   }

   RETURN(TRUE);

CLEANUP:
   if (pWnd) UserDerefObjectCo(pWnd);
   TRACE("Leave NtUserGetMenuBarInfo, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * @implemented
 */
UINT APIENTRY
NtUserGetMenuIndex(
   HMENU hMenu,
   HMENU hSubMenu)
{
   PMENU Menu, SubMenu;
   PITEM MenuItem;
   UINT i;
   DECLARE_RETURN(UINT);

   TRACE("Enter NtUserGetMenuIndex\n");
   UserEnterShared();

   if ( !(Menu = UserGetMenuObject(hMenu)) ||
        !(SubMenu = UserGetMenuObject(hSubMenu)) )
      RETURN(0xFFFFFFFF);

   MenuItem = Menu->rgItems;
   for (i = 0; i < Menu->cItems; i++, MenuItem++)
   {
       if (MenuItem->spSubMenu == SubMenu)
          RETURN(MenuItem->wID);
   }
   RETURN(0xFFFFFFFF);

CLEANUP:
   TRACE("Leave NtUserGetMenuIndex, ret=%u\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * @implemented
 */
BOOL APIENTRY
NtUserGetMenuItemRect(
   HWND hWnd,
   HMENU hMenu,
   UINT uItem,
   PRECTL lprcItem)
{
   PWND ReferenceWnd;
   LONG XMove, YMove;
   RECTL Rect;
   PMENU Menu;
   PITEM MenuItem;
   NTSTATUS Status = STATUS_SUCCESS;
   DECLARE_RETURN(BOOL);

   TRACE("Enter NtUserGetMenuItemRect\n");
   UserEnterShared();

   if (!(Menu = UserGetMenuObject(hMenu)))
   {
      RETURN(FALSE);
   }

   if ((MenuItem = MENU_FindItem (&Menu, &uItem, MF_BYPOSITION)))
   {
      Rect.left   = MenuItem->xItem;
      Rect.top    = MenuItem->yItem;
      Rect.right  = MenuItem->cxItem; // Do this for now......
      Rect.bottom = MenuItem->cyItem;
   }
   else
      RETURN(FALSE);

   if(!hWnd)
   {
       hWnd = Menu->hWnd;
   }

   if (lprcItem == NULL) RETURN( FALSE);

   if (!(ReferenceWnd = UserGetWindowObject(hWnd))) RETURN( FALSE);

   if (Menu->fFlags & MNF_POPUP)
   {
     XMove = ReferenceWnd->rcClient.left;
     YMove = ReferenceWnd->rcClient.top;
   }
   else
   {
     XMove = ReferenceWnd->rcWindow.left;
     YMove = ReferenceWnd->rcWindow.top;
   }

   Rect.left   += XMove;
   Rect.top    += YMove;
   Rect.right  += XMove;
   Rect.bottom += YMove;

   _SEH2_TRY
   {
      RtlCopyMemory(lprcItem, &Rect, sizeof(RECTL));
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
      Status = _SEH2_GetExceptionCode();
   }
   _SEH2_END

   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN(FALSE);
   }
   RETURN(TRUE);

CLEANUP:
   TRACE("Leave NtUserGetMenuItemRect, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * @implemented
 */
BOOL APIENTRY
NtUserHiliteMenuItem(
   HWND hWnd,
   HMENU hMenu,
   UINT uItemHilite,
   UINT uHilite)
{
   PMENU Menu;
   PWND Window;
   DECLARE_RETURN(BOOLEAN);

   TRACE("Enter NtUserHiliteMenuItem\n");
   UserEnterExclusive();

   if(!(Window = UserGetWindowObject(hWnd)))
   {
      EngSetLastError(ERROR_INVALID_WINDOW_HANDLE);
      RETURN(FALSE);
   }

   if(!(Menu = UserGetMenuObject(hMenu)))
   {
      EngSetLastError(ERROR_INVALID_MENU_HANDLE);
      RETURN(FALSE);
   }

   RETURN( IntHiliteMenuItem(Window, Menu, uItemHilite, uHilite));

CLEANUP:
   TRACE("Leave NtUserHiliteMenuItem, ret=%u\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * @implemented
 */
DWORD
APIENTRY
NtUserDrawMenuBarTemp(
   HWND hWnd,
   HDC hDC,
   PRECT pRect,
   HMENU hMenu,
   HFONT hFont)
{
   PMENU Menu;
   PWND Window;
   RECT Rect;
   NTSTATUS Status = STATUS_SUCCESS;
   DECLARE_RETURN(DWORD);

   ERR("Enter NtUserDrawMenuBarTemp\n");
   UserEnterExclusive();

   if(!(Window = UserGetWindowObject(hWnd)))
   {
      EngSetLastError(ERROR_INVALID_WINDOW_HANDLE);
      RETURN(0);
   }

   if(!(Menu = UserGetMenuObject(hMenu)))
   {
      EngSetLastError(ERROR_INVALID_MENU_HANDLE);
      RETURN(0);
   }

   _SEH2_TRY
   {
      ProbeForRead(pRect, sizeof(RECT), sizeof(ULONG));
      RtlCopyMemory(&Rect, pRect, sizeof(RECT));
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
      Status = _SEH2_GetExceptionCode();
   }
   _SEH2_END;

   if (Status != STATUS_SUCCESS)
   {
      SetLastNtError(Status);
      RETURN(0);
   }

   RETURN( IntDrawMenuBarTemp(Window, hDC, &Rect, Menu, hFont));

CLEANUP:
   ERR("Leave NtUserDrawMenuBarTemp, ret=%u\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * @implemented
 */
int APIENTRY
NtUserMenuItemFromPoint(
   HWND hWnd,
   HMENU hMenu,
   DWORD X,
   DWORD Y)
{
   PMENU Menu;
   PWND Window = NULL;
   PITEM mi;
   ULONG i;
   DECLARE_RETURN(int);

   TRACE("Enter NtUserMenuItemFromPoint\n");
   UserEnterExclusive();

   if (!(Menu = UserGetMenuObject(hMenu)))
   {
      RETURN( -1);
   }

   if (!(Window = UserGetWindowObject(Menu->hWnd)))
   {
      RETURN( -1);
   }

   X -= Window->rcWindow.left;
   Y -= Window->rcWindow.top;

   mi = Menu->rgItems;
   for (i = 0; i < Menu->cItems; i++, mi++)
   {
      RECTL Rect;

      Rect.left   = mi->xItem;
      Rect.top    = mi->yItem;
      Rect.right  = mi->cxItem;
      Rect.bottom = mi->cyItem;

      MENU_AdjustMenuItemRect(Menu, &Rect);

      if (RECTL_bPointInRect(&Rect, X, Y))
      {
         break;
      }
   }

   RETURN( (mi ? i : NO_SELECTED_ITEM));

CLEANUP:
   TRACE("Leave NtUserMenuItemFromPoint, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


DWORD
APIENTRY
NtUserPaintMenuBar(
    HWND hWnd,
    HDC hDC,
    ULONG leftBorder,
    ULONG rightBorder,
    ULONG top,
    BOOL bActive)
{
   PWND Window;
   RECT Rect;
   DWORD ret;

   UserEnterExclusive();

   if(!(Window = UserGetWindowObject(hWnd)))
   {
      EngSetLastError(ERROR_INVALID_WINDOW_HANDLE);
      UserLeave();
      return 0;
   }

   Rect.left = leftBorder;
   Rect.right = Window->rcWindow.right - Window->rcWindow.left - rightBorder;
   Rect.top = top;
   Rect.bottom = 0;

   ret = MENU_DrawMenuBar(hDC, &Rect, Window, FALSE);

   UserLeave();

   return ret;
}

/*
 * @implemented
 */
BOOL APIENTRY
NtUserRemoveMenu(
   HMENU hMenu,
   UINT uPosition,
   UINT uFlags)
{
   PMENU Menu;
   DECLARE_RETURN(BOOL);

   TRACE("Enter NtUserRemoveMenu\n");
   UserEnterExclusive();

   if(!(Menu = UserGetMenuObject(hMenu)))
   {
      RETURN( FALSE);
   }

   RETURN(IntRemoveMenuItem(Menu, uPosition, uFlags, FALSE));

CLEANUP:
   TRACE("Leave NtUserRemoveMenu, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;

}

/*
 * @implemented
 */
BOOL APIENTRY
NtUserSetMenu(
   HWND hWnd,
   HMENU Menu,
   BOOL Repaint)
{
   PWND Window;
   BOOL Changed;
   DECLARE_RETURN(BOOL);

   TRACE("Enter NtUserSetMenu\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN( FALSE);
   }

   if (!IntSetMenu(Window, Menu, &Changed))
   {
      RETURN( FALSE);
   }

   // Not minimized and please repaint!!!
   if (!(Window->style & WS_MINIMIZE) && (Repaint || Changed))
   {
      USER_REFERENCE_ENTRY Ref;
      UserRefObjectCo(Window, &Ref);
      co_WinPosSetWindowPos(Window, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED);
      UserDerefObjectCo(Window);
   }

   RETURN( TRUE);

CLEANUP:
   TRACE("Leave NtUserSetMenu, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * @implemented
 */
BOOL APIENTRY
NtUserSetMenuContextHelpId(
   HMENU hMenu,
   DWORD dwContextHelpId)
{
   PMENU Menu;
   DECLARE_RETURN(BOOL);

   TRACE("Enter NtUserSetMenuContextHelpId\n");
   UserEnterExclusive();

   if(!(Menu = UserGetMenuObject(hMenu)))
   {
      RETURN( FALSE);
   }

   RETURN(IntSetMenuContextHelpId(Menu, dwContextHelpId));

CLEANUP:
   TRACE("Leave NtUserSetMenuContextHelpId, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * @implemented
 */
BOOL APIENTRY
NtUserSetMenuDefaultItem(
   HMENU hMenu,
   UINT uItem,
   UINT fByPos)
{
   PMENU Menu;
   DECLARE_RETURN(BOOL);

   TRACE("Enter NtUserSetMenuDefaultItem\n");
   UserEnterExclusive();

   if(!(Menu = UserGetMenuObject(hMenu)))
   {
      RETURN( FALSE);
   }

   RETURN( UserSetMenuDefaultItem(Menu, uItem, fByPos));

CLEANUP:
   TRACE("Leave NtUserSetMenuDefaultItem, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * @implemented
 */
BOOL APIENTRY
NtUserSetMenuFlagRtoL(
   HMENU hMenu)
{
   PMENU Menu;
   DECLARE_RETURN(BOOL);

   TRACE("Enter NtUserSetMenuFlagRtoL\n");
   UserEnterExclusive();

   if(!(Menu = UserGetMenuObject(hMenu)))
   {
      RETURN( FALSE);
   }

   RETURN(IntSetMenuFlagRtoL(Menu));

CLEANUP:
   TRACE("Leave NtUserSetMenuFlagRtoL, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * @implemented
 */
BOOL APIENTRY
NtUserThunkedMenuInfo(
   HMENU hMenu,
   LPCMENUINFO lpcmi)
{
   PMENU Menu;
   DECLARE_RETURN(BOOL);

   TRACE("Enter NtUserThunkedMenuInfo\n");
   UserEnterExclusive();

   if (!(Menu = UserGetMenuObject(hMenu)))
   {
      RETURN(FALSE);
   }

   RETURN(UserMenuInfo(Menu, (PROSMENUINFO)lpcmi, TRUE));

CLEANUP:
   TRACE("Leave NtUserThunkedMenuInfo, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * @implemented
 */
BOOL APIENTRY
NtUserThunkedMenuItemInfo(
   HMENU hMenu,
   UINT uItem,
   BOOL fByPosition,
   BOOL bInsert,
   LPMENUITEMINFOW lpmii,
   PUNICODE_STRING lpszCaption)
{
   PMENU Menu;
   NTSTATUS Status;
   UNICODE_STRING lstrCaption;
   DECLARE_RETURN(BOOL);

   TRACE("Enter NtUserThunkedMenuItemInfo\n");
   UserEnterExclusive();

   /* lpszCaption may be NULL, check for it and call RtlInitUnicodeString()
      if bInsert == TRUE call UserInsertMenuItem() else UserSetMenuItemInfo()   */

   RtlInitEmptyUnicodeString(&lstrCaption, NULL, 0);

   if (!(Menu = UserGetMenuObject(hMenu)))
   {
      RETURN(FALSE);
   }

   /* Check if we got a Caption */
   if (lpszCaption && lpszCaption->Buffer)
   {
      /* Copy the string to kernel mode */
      Status = ProbeAndCaptureUnicodeString( &lstrCaption,
                                                 UserMode,
                                              lpszCaption);
      if (!NT_SUCCESS(Status))
      {
         ERR("Failed to capture MenuItem Caption (status 0x%08x)\n",Status);
         SetLastNtError(Status);
         RETURN(FALSE);
      }
   }

   if (bInsert) RETURN( UserInsertMenuItem(Menu, uItem, fByPosition, lpmii, &lstrCaption));

   RETURN( UserMenuItemInfo(Menu, uItem, fByPosition, (PROSMENUITEMINFO)lpmii, TRUE, &lstrCaption));

CLEANUP:
   if (lstrCaption.Buffer)
   {
      ReleaseCapturedUnicodeString(&lstrCaption, UserMode);
   }

   TRACE("Leave NtUserThunkedMenuItemInfo, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * @implemented
 */
BOOL APIENTRY
NtUserTrackPopupMenuEx(
   HMENU hMenu,
   UINT fuFlags,
   int x,
   int y,
   HWND hWnd,
   LPTPMPARAMS lptpm)
{
   PMENU menu;
   PWND pWnd;
   TPMPARAMS tpm;
   BOOL Ret = FALSE;
   USER_REFERENCE_ENTRY Ref;

   TRACE("Enter NtUserTrackPopupMenuEx\n");
   UserEnterExclusive();
   /* Parameter check */
   if (!(menu = UserGetMenuObject( hMenu )))
   {
      ERR("TPME : Invalid Menu handle.\n");
      EngSetLastError( ERROR_INVALID_MENU_HANDLE );
      goto Exit;
   }

   if (!(pWnd = UserGetWindowObject(hWnd)))
   {
      ERR("TPME : Invalid Window handle.\n");
      goto Exit;
   }

   if (lptpm)
   {
      _SEH2_TRY
      {
         ProbeForRead(lptpm, sizeof(TPMPARAMS), sizeof(ULONG));
         tpm = *lptpm;
      }
      _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
      { 
         _SEH2_YIELD(goto Exit);
      }
      _SEH2_END
   }
   UserRefObjectCo(pWnd, &Ref);
   Ret = IntTrackPopupMenuEx(menu, fuFlags, x, y, pWnd, lptpm ? &tpm : NULL);
   UserDerefObjectCo(pWnd);

Exit:
   TRACE("Leave NtUserTrackPopupMenuEx, ret=%i\n",Ret);
   UserLeave();
   return Ret;
}

/* EOF */
