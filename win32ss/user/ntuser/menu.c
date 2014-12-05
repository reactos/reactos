/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Menus
 * FILE:             subsys/win32k/ntuser/menu.c
 * PROGRAMER:        Thomas Weidenmueller (w3seek@users.sourceforge.net)
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserMenu);

/* INTERNAL ******************************************************************/

BOOL FASTCALL IntSetMenuItemInfo(PMENU, PITEM, PROSMENUITEMINFO, PUNICODE_STRING);

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

/* Maximum number of menu items a menu can contain */
#define MAX_MENU_ITEMS (0x4000)
#define MAX_GOINTOSUBMENU (0x10)

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
   UINT i;
   if (!pMenu) return NULL;

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
      _SEH2_YIELD(return NULL);
   }
   _SEH2_END

   if ( UserObjectInDestroy(hMenu))
   {
      ERR("Menu is marked for destruction!\n");
      return NULL;
   }

   return pMenu;
}

BOOL IntDestroyMenu( PMENU pMenu, BOOL bRecurse)
{
    PMENU SubMenu;

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
   if(Menu)
   {
      PWND Window;

      /* Remove all menu items */
      IntDestroyMenu( Menu, bRecurse);

      if (PsGetCurrentProcessSessionId() == Menu->head.rpdesk->rpwinstaParent->dwSessionId)
      {
         BOOL ret;
         if (Menu->hWnd)
         {
            Window = UserGetWindowObject(Menu->hWnd);
            if (Window)
            {
               Window->IDMenu = 0;

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
         ret = UserDeleteObject(Menu->head.h, TYPE_MENU);
         if (!ret)
         {  // Make sure it is really dead or just marked for deletion.
            ret = UserObjectInDestroy(Menu->head.h);
            if (ret && EngGetLastError() == ERROR_INVALID_HANDLE) ret = FALSE;
         }  // See test_subpopup_locked_by_menu tests....
         return ret;
      }
   }
   return FALSE;
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
    for( i = 0; item, i < pmenu->cItems && subdepth <= MAXMENUDEPTH; i++, item++)
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
	for (i = 0; item, i < menu->cItems; i++, item++)
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

BOOL FASTCALL
IntRemoveMenuItem( PMENU pMenu, UINT nPos, UINT wFlags, BOOL bRecurse )
{
    PITEM item;

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
	while(nPos < pMenu->cItems)
	{
	    *item = *(item+1);
	    item++;
	    nPos++;
	}
	pMenu->rgItems = DesktopHeapReAlloc(pMenu->head.rpdesk, pMenu->rgItems, pMenu->cItems * sizeof(ITEM));
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

   TRACE("IntInsertMenuItemToList = %i %d\n", uItem, (BOOL)((INT)uItem >= 0));

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

   NewMenuItem = DesktopHeapAlloc(Destination->head.rpdesk, (Source->cItems+1) * sizeof(ITEM));
   if(!NewMenuItem) return FALSE;

   RtlZeroMemory(NewMenuItem, (Source->cItems+1) * sizeof(ITEM));

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
   Menu->cItems = Source->cItems;
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

         Source.Length = Source.MaximumLength = lpmii->cch * sizeof(WCHAR);
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
   if((MenuItem->wID == SC_CLOSE) && (res != uEnable))
   {
	if (MenuObject->fFlags & MNF_SYSSUBMENU && MenuObject->spwndNotify != 0)
	{
            RECTL rc = MenuObject->spwndNotify->rcWindow;

            /* Refresh the frame to reflect the change */
            IntMapWindowPoints(0, MenuObject->spwndNotify, (POINT *)&rc, 2);
            rc.bottom = 0;
            co_UserRedrawWindow(MenuObject->spwndNotify, &rc, 0, RDW_FRAME | RDW_INVALIDATE | RDW_NOCHILDREN);
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
IntHiliteMenuItem(PWND WindowObject,
                  PMENU MenuObject,
                  UINT uItemHilite,
                  UINT uHilite)
{
   PITEM MenuItem;

   if (!(MenuItem = MENU_FindItem( &MenuObject, &uItemHilite, uHilite ))) return FALSE;

   if (uHilite & MF_HILITE)
   {
      MenuItem->fState |= MF_HILITE;
   }
   else
   {
      MenuItem->fState &= ~MF_HILITE;
   }
   /* FIXME: Update the window's menu */

   return TRUE; // Always returns true!!!!
}

BOOL FASTCALL
UserSetMenuDefaultItem(PMENU MenuObject, UINT uItem, UINT fByPos)
{
   UINT i;
   PITEM MenuItem = MenuObject->rgItems;

   if (!MenuItem) return FALSE;   

   /* reset all default-item flags */
   for (i = 0; MenuItem, i < MenuObject->cItems; i++, MenuItem++)
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
      for (i = 0; MenuItem, i < MenuObject->cItems; i++, MenuItem++)
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
   if ( (MenuItem->fType & MF_POPUP) && (gmdiFlags & GMDI_GOINTOPOPUPS) && MenuItem->spSubMenu)
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

VOID FASTCALL
co_IntInitTracking(PWND Window, PMENU Menu, BOOL Popup,
                   UINT Flags)
{
   /* FIXME: Hide caret */

   if(!(Flags & TPM_NONOTIFY))
      co_IntSendMessage(Window->head.h, WM_SETCURSOR, (WPARAM)Window->head.h, HTCAPTION);

   /* FIXME: Send WM_SETCURSOR message */

   if(!(Flags & TPM_NONOTIFY))
      co_IntSendMessage(Window->head.h, WM_INITMENU, (WPARAM)Menu->head.h, 0);
}

VOID FASTCALL
co_IntExitTracking(PWND Window, PMENU Menu, BOOL Popup,
                   UINT Flags)
{
   if(!(Flags & TPM_NONOTIFY))
      co_IntSendMessage(Window->head.h, WM_EXITMENULOOP, 0 /* FIXME */, 0);

   /* FIXME: Show caret again */
}

INT FASTCALL
IntTrackMenu(PMENU Menu, PWND Window, INT x, INT y,
             RECTL lprect)
{
   return 0;
}

BOOL FASTCALL
co_IntTrackPopupMenu(PMENU Menu, PWND Window,
                     UINT Flags, POINT *Pos, UINT MenuPos, RECTL *ExcludeRect)
{
   co_IntInitTracking(Window, Menu, TRUE, Flags);

   co_IntExitTracking(Window, Menu, TRUE, Flags);
   return FALSE;
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
      return (pItem->spSubMenu->cItems << 8) | ((pItem->fState|pItem->fType) & 0xff);
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
    PMENU menu;
    HMENU hSubMenu;
    UINT i;
    PITEM item;

    if (((*hMenu)==(HMENU)0xffff) ||(!(menu = UserGetMenuObject(*hMenu))))
        return NO_SELECTED_ITEM;

    item = menu->rgItems;
    for (i = 0; i < menu->cItems; i++, item++)
    {
        if (!item->spSubMenu)
           continue;
        else
        {
           hSubMenu = UserHMGetHandle(item->spSubMenu);
           if (hSubMenu == hSubTarget)
           {
              return i;
           }
           else 
           {
              HMENU hsubmenu = hSubMenu;
              UINT pos = IntFindSubMenu( &hsubmenu, hSubTarget );
              if (pos != NO_SELECTED_ITEM)
              {
                  *hMenu = hsubmenu;
                  return pos;
              }
           }
        }
    }
    return NO_SELECTED_ITEM;
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
                     KernelMode,
                     0,
                     &WinStaObject);

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
   if (sizeof(MENUITEMINFOW) != Size
         && FIELD_OFFSET(MENUITEMINFOW, hbmpItem) != Size
         && sizeof(ROSMENUITEMINFO) != Size)
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
   if(Size < sizeof(MENUINFO) || sizeof(ROSMENUINFO) < Size)
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

VOID FASTCALL
MENU_AdjustMenuItemRect(PMENU menu, PRECTL rect)
{
    if (menu->dwArrowsOn)
    {
        UINT arrow_bitmap_height;
        //BITMAP bmp;
        //GetObjectW(get_up_arrow_bitmap(), sizeof(bmp), &bmp);
        arrow_bitmap_height = gpsi->oembmi[65].cy; ///// Menu up arrow! OBM_UPARROW DFCS_MENUARROWUP
        //arrow_bitmap_height = bmp.bmHeight;
        rect->top += arrow_bitmap_height - menu->iTop;
        rect->bottom += arrow_bitmap_height - menu->iTop;
    }
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

   if (!pWnd)
   {
      HWND hWnd = Menu->hWnd;
      if (!(pWnd = UserGetWindowObject(hWnd))) return FALSE;
   }

   if ((MenuItem = MENU_FindItem (&Menu, &uItem, MF_BYPOSITION)))
   {
      Rect->left   = MenuItem->xItem; 
      Rect->top    = MenuItem->yItem; 
      Rect->right  = MenuItem->cxItem; // Do this for now......
      Rect->bottom = MenuItem->cyItem;
   }
   else
   {
      ERR("Failed Item Lookup! %d\n", uItem);
      return FALSE;
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
   SysMenu->hWnd = Window->head.h;

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
         IntReleaseMenuObject(SysMenu);
         UserDestroyMenu(hSysMenu);
         return NULL;
      }
      Menu = UserGetMenuObject(hNewMenu);
      if (!Menu)
      {
         IntReleaseMenuObject(SysMenu);
         UserDestroyMenu(hSysMenu);
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
      ItemInfo.fType = 0;
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
      if ((!Window->SystemMenu || Menu->fFlags & MNF_SYSDESKMN) && Window->style & WS_SYSMENU)
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


/* FUNCTIONS *****************************************************************/

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
   UserEnterShared();

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

   switch (idObject)
   {
    case OBJID_CLIENT:
        if (!pWnd->pcls->fnid)
            RETURN(FALSE);
        if (pWnd->pcls->fnid != FNID_MENU)
        {
            WARN("called on invalid window: %d\n", pWnd->pcls->fnid);
            EngSetLastError(ERROR_INVALID_MENU_HANDLE);
            RETURN(FALSE);
        }
        // Windows does this! Wine checks for Atom and uses GetWindowLongPtrW.
        hMenu = (HMENU)co_IntSendMessage(hwnd, MN_GETHMENU, 0, 0);
        break;
    case OBJID_MENU:
        hMenu = UlongToHandle(pWnd->IDMenu);
        break;
    case OBJID_SYSMENU:
        if (!(pWnd->style & WS_SYSMENU)) RETURN(FALSE);
        Menu = IntGetSystemMenu(pWnd, FALSE);
        hMenu = Menu->head.h;
        break;
    default:
        RETURN(FALSE);
   }

   if (!hMenu)
      RETURN(FALSE);

   _SEH2_TRY
   {
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

   if (idItem < 0 || idItem > Menu->cItems)
       RETURN(FALSE);

   RECTL_vSetEmptyRect(&kmbi.rcBar);

   if (idItem == 0)
   {
      Ret = IntGetMenuItemRect(pWnd, Menu, 0, &kmbi.rcBar);
      kmbi.rcBar.right = kmbi.rcBar.left + Menu->cxMenu;
      kmbi.rcBar.bottom = kmbi.rcBar.top + Menu->cyMenu;
      TRACE("idItem 0 %d\n",Ret);
   }
   else
   {
      Ret = IntGetMenuItemRect(pWnd, Menu, idItem-1, &kmbi.rcBar);
      TRACE("idItem X %d\n", Ret);
   }

   kmbi.hMenu = hMenu;
   kmbi.hwndMenu = NULL;
   kmbi.fBarFocused = FALSE;
   kmbi.fFocused = FALSE;
   //kmbi.fBarFocused = top_popup_hmenu == hMenu;
   if (idItem)
   {
       kmbi.fFocused = Menu->iItem == idItem-1;
       if (kmbi.fFocused && (Menu->rgItems[idItem - 1].spSubMenu))
       {
          kmbi.hwndMenu = Menu->rgItems[idItem - 1].spSubMenu->hWnd;
       }
   }
/*   else
   {
       kmbi.fFocused = kmbi.fBarFocused;
   }
*/
   _SEH2_TRY
   {
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
   int i;
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
      Rect.right  = mi->cxItem; // Do this for now......
      Rect.bottom = mi->cyItem;
      //MENU_AdjustMenuItemRect(Menu, &Rect); Need gpsi OBMI via callback!
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

   if (!(Menu = UserGetMenuObject(hMenu)))
   {
      RETURN(FALSE);
   }

   RtlInitUnicodeString(&lstrCaption, 0);

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
   TRACE("Leave NtUserThunkedMenuItemInfo, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/* EOF */
