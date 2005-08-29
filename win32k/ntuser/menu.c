/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id$
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Menus
 * FILE:             subsys/win32k/ntuser/menu.c
 * PROGRAMER:        Thomas Weidenmueller (w3seek@users.sourceforge.net)
 * REVISION HISTORY:
 *       07/30/2003  CSH  Created
 */
/* INCLUDES ******************************************************************/

#include <w32k.h>

#define NDEBUG
#include <debug.h>

/* INTERNAL ******************************************************************/

/* maximum number of menu items a menu can contain */
#define MAX_MENU_ITEMS (0x4000)
#define MAX_GOINTOSUBMENU (0x10)

#define UpdateMenuItemState(state, change) \
{\
  if((change) & MFS_DISABLED) { \
    (state) |= MFS_DISABLED; \
  } else { \
    (state) &= ~MFS_DISABLED; \
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

#define FreeMenuText(MenuItem) \
{ \
  if((MENU_ITEM_TYPE((MenuItem)->fType) == MF_STRING) && \
           (MenuItem)->Text.Length) { \
    RtlFreeUnicodeString(&(MenuItem)->Text); \
  } \
}

#define InRect(r, x, y) \
      ( ( ((r).right >=  x)) && \
        ( ((r).left <= x)) && \
        ( ((r).bottom >=  y)) && \
        ( ((r).top <= y)) )

NTSTATUS FASTCALL
InitMenuImpl(VOID)
{
  return(STATUS_SUCCESS);
}

NTSTATUS FASTCALL
CleanupMenuImpl(VOID)
{
  return(STATUS_SUCCESS);
}

#if 0
void FASTCALL
DumpMenuItemList(PMENU_ITEM MenuItem)
{
  UINT cnt = 0;
  while(MenuItem)
  {
    if(MenuItem->Text.Length)
      DbgPrint(" %d. %wZ\n", ++cnt, &MenuItem->Text);
    else
      DbgPrint(" %d. NO TEXT dwTypeData==%d\n", ++cnt, (DWORD)MenuItem->Text.Buffer);
    DbgPrint("   fType=");
    if(MFT_BITMAP & MenuItem->fType) DbgPrint("MFT_BITMAP ");
    if(MFT_MENUBARBREAK & MenuItem->fType) DbgPrint("MFT_MENUBARBREAK ");
    if(MFT_MENUBREAK & MenuItem->fType) DbgPrint("MFT_MENUBREAK ");
    if(MFT_OWNERDRAW & MenuItem->fType) DbgPrint("MFT_OWNERDRAW ");
    if(MFT_RADIOCHECK & MenuItem->fType) DbgPrint("MFT_RADIOCHECK ");
    if(MFT_RIGHTJUSTIFY & MenuItem->fType) DbgPrint("MFT_RIGHTJUSTIFY ");
    if(MFT_SEPARATOR & MenuItem->fType) DbgPrint("MFT_SEPARATOR ");
    if(MFT_STRING & MenuItem->fType) DbgPrint("MFT_STRING ");
    DbgPrint("\n   fState=");
    if(MFS_DISABLED & MenuItem->fState) DbgPrint("MFS_DISABLED ");
    else DbgPrint("MFS_ENABLED ");
    if(MFS_CHECKED & MenuItem->fState) DbgPrint("MFS_CHECKED ");
    else DbgPrint("MFS_UNCHECKED ");
    if(MFS_HILITE & MenuItem->fState) DbgPrint("MFS_HILITE ");
    else DbgPrint("MFS_UNHILITE ");
    if(MFS_DEFAULT & MenuItem->fState) DbgPrint("MFS_DEFAULT ");
    if(MFS_GRAYED & MenuItem->fState) DbgPrint("MFS_GRAYED ");
    DbgPrint("\n   wId=%d\n", MenuItem->wID);
    MenuItem = MenuItem->Next;
  }
  DbgPrint("Entries: %d\n", cnt);
  return;
}
#endif


inline VOID FASTCALL UserFreeMenuObject(PMENU_OBJECT Menu)
{
   UserFreeHandle(&gHandleTable, Menu->hSelf);  
   RtlZeroMemory(Menu, sizeof(MENU_OBJECT));
   UserFree(Menu);
}


PMENU_OBJECT FASTCALL UserAllocMenuObject(HMENU* h)
{
   PVOID mem;

   mem = UserAllocZero(sizeof(MENU_OBJECT));
   if (!mem) return NULL;

   *h = UserAllocHandle(&gHandleTable, mem, otMenu);
   if (!*h){
      UserFree(mem);
      return NULL;
   }
   return mem;
}




inline PMENU_OBJECT FASTCALL UserGetMenuObject(HMENU hMenu)
{
   return (PMENU_OBJECT)UserGetObject(&gHandleTable, hMenu, otMenu );
}


BOOL FASTCALL
IntFreeMenuItem(PMENU_OBJECT MenuObject, PMENU_ITEM MenuItem,
    BOOL RemoveFromList, BOOL bRecurse)
{
  FreeMenuText(MenuItem);
  if(RemoveFromList)
  {
    /* FIXME - Remove from List */
    MenuObject->MenuInfo.MenuItemCount--;
  }
  if(bRecurse && MenuItem->hSubMenu)
  {
    PMENU_OBJECT SubMenuObject;
    SubMenuObject = UserGetMenuObject(MenuItem->hSubMenu );
    if(SubMenuObject)
    {
      UserDestroyMenu(SubMenuObject, bRecurse);
    }
  }

  /* Free memory */
  UserFree(MenuItem);

  return TRUE;
}

BOOL FASTCALL
IntRemoveMenuItem(PMENU_OBJECT Menu, UINT uPosition, UINT uFlags,
                   BOOL bRecurse)
{
  PMENU_ITEM PrevMenuItem, MenuItem;
  if(UserGetMenuItemByFlag(Menu, uPosition, uFlags, &MenuItem,
                           &PrevMenuItem) > -1)
  {
    if(MenuItem)
    {
      if(PrevMenuItem)
        PrevMenuItem->Next = MenuItem->Next;
      else
      {
        Menu->MenuItemList = MenuItem->Next;
      }
      return IntFreeMenuItem(Menu, MenuItem, TRUE, bRecurse);
    }
  }
  return FALSE;
}

UINT FASTCALL
IntDeleteMenuItems(PMENU_OBJECT MenuObject, BOOL bRecurse)
{
  UINT res = 0;
  PMENU_ITEM NextItem;
  PMENU_ITEM CurItem = MenuObject->MenuItemList;
  while(CurItem)
  {
    NextItem = CurItem->Next;
    IntFreeMenuItem(MenuObject, CurItem, FALSE, bRecurse);
    CurItem = NextItem;
    res++;
  }
  MenuObject->MenuInfo.MenuItemCount = 0;
  MenuObject->MenuItemList = NULL;
  return res;
}

VOID FASTCALL
UserDestroyMenu(PMENU_OBJECT Menu, BOOL bRecurse)
{
   ASSERT(Menu);

   /* remove all menu items */
   IntDeleteMenuItems(Menu, bRecurse);

   RemoveEntryList(&Menu->ListEntry);
   UserFreeMenuObject(Menu);
}



PMENU_OBJECT FASTCALL
UserCreateMenu(BOOL PopupMenu /* oppsite of popup is MenuBar */ )
{
  PMENU_OBJECT Menu;
  HMENU hMenu;

  Menu = UserAllocMenuObject(&hMenu);

  if(!Menu)
  {
    return NULL;
  }

  Menu->Process = PsGetCurrentProcess();
  Menu->RtoL = FALSE; /* default */
  Menu->MenuInfo.cbSize = sizeof(MENUINFO); /* not used */
  Menu->MenuInfo.fMask = 0; /* not used */
  Menu->MenuInfo.dwStyle = 0; /* FIXME */
  Menu->MenuInfo.cyMax = 0; /* default */
  Menu->MenuInfo.hbrBack =
     NtGdiCreateSolidBrush(RGB(192, 192, 192)); /* FIXME: default background color */
  Menu->MenuInfo.dwContextHelpID = 0; /* default */
  Menu->MenuInfo.dwMenuData = 0; /* default */
  Menu->MenuInfo.Self = hMenu;
  Menu->MenuInfo.FocusedItem = NO_SELECTED_ITEM;
  Menu->MenuInfo.Flags = (PopupMenu ? MF_POPUP : 0);
  Menu->MenuInfo.Wnd = NULL;
  Menu->MenuInfo.WndOwner = NULL;
  Menu->MenuInfo.Height = 0;
  Menu->MenuInfo.Width = 0;
  Menu->MenuInfo.TimeToHide = FALSE;

  Menu->MenuInfo.MenuItemCount = 0;
  Menu->MenuItemList = NULL;

  /* Insert menu item into process menu handle list */
  InsertTailList(&PsGetWin32Process()->MenuListHead, &Menu->ListEntry);

  return Menu;
}

BOOL FASTCALL
IntCloneMenuItems(PMENU_OBJECT Destination, PMENU_OBJECT Source)
{
  PMENU_ITEM MenuItem, NewMenuItem = NULL;
  PMENU_ITEM Old = NULL;

  if(!Source->MenuInfo.MenuItemCount)
    return FALSE;

  MenuItem = Source->MenuItemList;
  while(MenuItem)
  {
    Old = NewMenuItem;
    if(NewMenuItem)
      NewMenuItem->Next = MenuItem;
    NewMenuItem = ExAllocatePoolWithTag(PagedPool, sizeof(MENU_ITEM), TAG_MENUITEM);
    if(!NewMenuItem)
      break;
    NewMenuItem->fType = MenuItem->fType;
    NewMenuItem->fState = MenuItem->fState;
    NewMenuItem->wID = MenuItem->wID;
    NewMenuItem->hSubMenu = MenuItem->hSubMenu;
    NewMenuItem->hbmpChecked = MenuItem->hbmpChecked;
    NewMenuItem->hbmpUnchecked = MenuItem->hbmpUnchecked;
    NewMenuItem->dwItemData = MenuItem->dwItemData;
    if((MENU_ITEM_TYPE(NewMenuItem->fType) == MF_STRING))
    {
      if(MenuItem->Text.Length)
      {
        NewMenuItem->Text.Length = 0;
        NewMenuItem->Text.MaximumLength = MenuItem->Text.MaximumLength;
        NewMenuItem->Text.Buffer = (PWSTR)ExAllocatePoolWithTag(PagedPool, MenuItem->Text.MaximumLength, TAG_STRING);
        if(!NewMenuItem->Text.Buffer)
        {
          ExFreePool(NewMenuItem);
          break;
        }
        RtlCopyUnicodeString(&NewMenuItem->Text, &MenuItem->Text);
      }
      else
      {
        NewMenuItem->Text.Buffer = MenuItem->Text.Buffer;
      }
    }
    else
    {
      NewMenuItem->Text.Buffer = MenuItem->Text.Buffer;
    }
    NewMenuItem->hbmpItem = MenuItem->hbmpItem;

    NewMenuItem->Next = NULL;
    if(Old)
      Old->Next = NewMenuItem;
    else
      Destination->MenuItemList = NewMenuItem;
    Destination->MenuInfo.MenuItemCount++;
    MenuItem = MenuItem->Next;
  }

  return TRUE;
}

PMENU_OBJECT FASTCALL
IntCloneMenu(PMENU_OBJECT Source)
{
  HMENU hMenu;
  PMENU_OBJECT Menu;

  if(!Source)
    return NULL;

  Menu = UserAllocMenuObject(&hMenu);

  if(!Menu)
    return NULL;

  Menu->Process = PsGetCurrentProcess();
  Menu->RtoL = Source->RtoL;
  Menu->MenuInfo.cbSize = sizeof(MENUINFO); /* not used */
  Menu->MenuInfo.fMask = Source->MenuInfo.fMask;
  Menu->MenuInfo.dwStyle = Source->MenuInfo.dwStyle;
  Menu->MenuInfo.cyMax = Source->MenuInfo.cyMax;
  Menu->MenuInfo.hbrBack = Source->MenuInfo.hbrBack;
  Menu->MenuInfo.dwContextHelpID = Source->MenuInfo.dwContextHelpID;
  Menu->MenuInfo.dwMenuData = Source->MenuInfo.dwMenuData;
  Menu->MenuInfo.Self = hMenu;
  Menu->MenuInfo.FocusedItem = NO_SELECTED_ITEM;
  Menu->MenuInfo.Wnd = NULL;
  Menu->MenuInfo.WndOwner = NULL;
  Menu->MenuInfo.Height = 0;
  Menu->MenuInfo.Width = 0;
  Menu->MenuInfo.TimeToHide = FALSE;

  Menu->MenuInfo.MenuItemCount = 0;
  Menu->MenuItemList = NULL;

  /* Insert menu item into process menu handle list */
  InsertTailList(&PsGetWin32Process()->MenuListHead, &Menu->ListEntry);

  IntCloneMenuItems(Menu, Source);

  return Menu;
}

BOOL FASTCALL
UserSetMenuFlagRtoL(PMENU_OBJECT MenuObject)
{
  MenuObject->RtoL = TRUE;
  return TRUE;
}

BOOL FASTCALL
IntSetMenuContextHelpId(PMENU_OBJECT MenuObject, DWORD dwContextHelpId)
{
  MenuObject->MenuInfo.dwContextHelpID = dwContextHelpId;
  return TRUE;
}

BOOL FASTCALL
UserGetMenuInfo(PMENU_OBJECT MenuObject, PROSMENUINFO lpmi)
{
  if(lpmi->fMask & MIM_BACKGROUND)
    lpmi->hbrBack = MenuObject->MenuInfo.hbrBack;
  if(lpmi->fMask & MIM_HELPID)
    lpmi->dwContextHelpID = MenuObject->MenuInfo.dwContextHelpID;
  if(lpmi->fMask & MIM_MAXHEIGHT)
    lpmi->cyMax = MenuObject->MenuInfo.cyMax;
  if(lpmi->fMask & MIM_MENUDATA)
    lpmi->dwMenuData = MenuObject->MenuInfo.dwMenuData;
  if(lpmi->fMask & MIM_STYLE)
    lpmi->dwStyle = MenuObject->MenuInfo.dwStyle;
  if (sizeof(MENUINFO) < lpmi->cbSize)
    {
      RtlCopyMemory((char *) lpmi + sizeof(MENUINFO),
                    (char *) &MenuObject->MenuInfo + sizeof(MENUINFO),
                    lpmi->cbSize - sizeof(MENUINFO));
    }

  return TRUE;
}


BOOL FASTCALL
IntIsMenu(HMENU hMenu)
{
  PMENU_OBJECT Menu;

  if((Menu = UserGetMenuObject(hMenu)))
  {
    return TRUE;
  }
  return FALSE;
}


BOOL FASTCALL
UserSetMenuInfo(PMENU_OBJECT MenuObject, PROSMENUINFO lpmi)
{
  if(lpmi->fMask & MIM_BACKGROUND)
    MenuObject->MenuInfo.hbrBack = lpmi->hbrBack;
  if(lpmi->fMask & MIM_HELPID)
    MenuObject->MenuInfo.dwContextHelpID = lpmi->dwContextHelpID;
  if(lpmi->fMask & MIM_MAXHEIGHT)
    MenuObject->MenuInfo.cyMax = lpmi->cyMax;
  if(lpmi->fMask & MIM_MENUDATA)
    MenuObject->MenuInfo.dwMenuData = lpmi->dwMenuData;
  if(lpmi->fMask & MIM_STYLE)
    MenuObject->MenuInfo.dwStyle = lpmi->dwStyle;
  if(lpmi->fMask & MIM_APPLYTOSUBMENUS)
    {
    /* FIXME */
    }
  if (sizeof(MENUINFO) < lpmi->cbSize)
    {
      MenuObject->MenuInfo.FocusedItem = lpmi->FocusedItem;
      MenuObject->MenuInfo.Height = lpmi->Height;
      MenuObject->MenuInfo.Width = lpmi->Width;
      MenuObject->MenuInfo.Wnd = lpmi->Wnd;
      MenuObject->MenuInfo.WndOwner = lpmi->WndOwner;
      MenuObject->MenuInfo.TimeToHide = lpmi->TimeToHide;
    }

  return TRUE;
}


int FASTCALL
UserGetMenuItemByFlag(PMENU_OBJECT MenuObject, UINT uSearchBy, UINT fFlag,
                     PMENU_ITEM *MenuItem, PMENU_ITEM *PrevMenuItem)
{
  PMENU_ITEM PrevItem = NULL;
  PMENU_ITEM CurItem = MenuObject->MenuItemList;
  int p;
  int ret;

  if(MF_BYPOSITION & fFlag)
  {
    p = uSearchBy;
    while(CurItem && (p > 0))
    {
      PrevItem = CurItem;
      CurItem = CurItem->Next;
      p--;
    }
    if(CurItem)
    {
      if(MenuItem) *MenuItem = CurItem;
      if(PrevMenuItem) *PrevMenuItem = PrevItem;
    }
    else
    {
      if(MenuItem) *MenuItem = NULL;
      if(PrevMenuItem) *PrevMenuItem = NULL; /* ? */
      return -1;
    }

    return uSearchBy - p;
  }
  else
  {
    p = 0;
    while(CurItem)
    {
      if(CurItem->wID == uSearchBy)
      {
        if(MenuItem) *MenuItem = CurItem;
        if(PrevMenuItem) *PrevMenuItem = PrevItem;
        return p;
      }
      else if (0 != (CurItem->fType & MF_POPUP))
      {
        MenuObject = UserGetMenuObject(CurItem->hSubMenu);
        if (NULL != MenuObject)
        {
          ret = UserGetMenuItemByFlag(MenuObject, uSearchBy, fFlag,
                                     MenuItem, PrevMenuItem);
          if (-1 != ret)
          {
            return ret;
          }
        }
      }
      PrevItem = CurItem;
      CurItem = CurItem->Next;
      p++;
    }
  }
  return -1;
}


int FASTCALL
IntInsertMenuItemToList(PMENU_OBJECT MenuObject, PMENU_ITEM MenuItem, int pos)
{
  PMENU_ITEM CurItem;
  PMENU_ITEM LastItem = NULL;
  UINT npos = 0;

  CurItem = MenuObject->MenuItemList;
  if(pos <= -1)
  {
    while(CurItem)
    {
      LastItem = CurItem;
      CurItem = CurItem->Next;
      npos++;
    }
  }
  else
  {
    while(CurItem && (pos > 0))
    {
      LastItem = CurItem;
      CurItem = CurItem->Next;
      pos--;
      npos++;
    }
  }

  if(CurItem)
  {
    if(LastItem)
    {
      /* insert the item before CurItem */
      MenuItem->Next = LastItem->Next;
      LastItem->Next = MenuItem;
    }
    else
    {
      /* insert at the beginning */
      MenuObject->MenuItemList = MenuItem;
      MenuItem->Next = CurItem;
    }
  }
  else
  {
    if(LastItem)
    {
      /* append item */
      LastItem->Next = MenuItem;
      MenuItem->Next = NULL;
    }
    else
    {
      /* insert first item */
      MenuObject->MenuItemList = MenuItem;
      MenuItem->Next = NULL;
    }
  }
  MenuObject->MenuInfo.MenuItemCount++;

  return npos;
}

BOOL FASTCALL
UserGetMenuItemInfo(PMENU_OBJECT MenuObject, PMENU_ITEM MenuItem, PROSMENUITEMINFO lpmii)
{
  NTSTATUS Status;

  if(lpmii->fMask & MIIM_BITMAP)
    {
      lpmii->hbmpItem = MenuItem->hbmpItem;
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
  if(lpmii->fMask & (MIIM_FTYPE | MIIM_TYPE))
    {
      lpmii->fType = MenuItem->fType;
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
      lpmii->hSubMenu = MenuItem->hSubMenu;
    }
  if (lpmii->fMask & (MIIM_STRING | MIIM_TYPE))
    {
      if (lpmii->dwTypeData == NULL)
        {
          lpmii->cch = MenuItem->Text.Length / sizeof(WCHAR);
        }
      else
        {
          Status = MmCopyToCaller(lpmii->dwTypeData, MenuItem->Text.Buffer,
                                  min(lpmii->cch * sizeof(WCHAR),
                                      MenuItem->Text.MaximumLength));
          if (! NT_SUCCESS(Status))
            {
              SetLastNtError(Status);
              return FALSE;
            }
        }
    }

  if (sizeof(ROSMENUITEMINFO) == lpmii->cbSize)
    {
      lpmii->Rect = MenuItem->Rect;
      lpmii->XTab = MenuItem->XTab;
    }

  return TRUE;
}

BOOL FASTCALL
UserSetMenuItemInfo(PMENU_OBJECT MenuObject, PMENU_ITEM MenuItem, PROSMENUITEMINFO lpmii)
{
  PMENU_OBJECT SubMenuObject;

  if(!MenuItem || !MenuObject || !lpmii)
  {
    return FALSE;
  }

  MenuItem->fType = lpmii->fType;

  if(lpmii->fMask & MIIM_BITMAP)
  {
    MenuItem->hbmpItem = lpmii->hbmpItem;
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
  if(lpmii->fMask & (MIIM_FTYPE | MIIM_TYPE))
  {
    /*
     * Delete the menu item type when changing type from
     * MF_STRING.
     */
    if (MenuItem->fType != lpmii->fType &&
        MENU_ITEM_TYPE(MenuItem->fType) == MF_STRING)
    {
      FreeMenuText(MenuItem);
      RtlInitUnicodeString(&MenuItem->Text, NULL);
    }
    MenuItem->fType = lpmii->fType;
  }
  if(lpmii->fMask & MIIM_ID)
  {
    MenuItem->wID = lpmii->wID;
  }
  if(lpmii->fMask & MIIM_STATE)
  {
    /* remove MFS_DEFAULT flag from all other menu items if this item
       has the MFS_DEFAULT state */
    if(lpmii->fState & MFS_DEFAULT)
      IntSetMenuDefaultItem(MenuObject, -1, 0);
    /* update the menu item state flags */
    UpdateMenuItemState(MenuItem->fState, lpmii->fState);
  }

  if(lpmii->fMask & MIIM_SUBMENU)
  {
    MenuItem->hSubMenu = lpmii->hSubMenu;
    /* Make sure the submenu is marked as a popup menu */
    if (MenuItem->hSubMenu)
    {
      SubMenuObject = UserGetMenuObject(MenuItem->hSubMenu);
      if (SubMenuObject != NULL)
      {
        SubMenuObject->MenuInfo.Flags |= MF_POPUP;
        MenuItem->fType |= MF_POPUP;
      }
      else
      {
        MenuItem->fType &= ~MF_POPUP;
      }
    }
    else
    {
      MenuItem->fType &= ~MF_POPUP;
    }
  }
  if ((lpmii->fMask & (MIIM_TYPE | MIIM_STRING)) &&
      (MENU_ITEM_TYPE(lpmii->fType) == MF_STRING))
  {
    FreeMenuText(MenuItem);

    if(lpmii->dwTypeData && lpmii->cch)
    {
      UNICODE_STRING Source;

      Source.Length =
      Source.MaximumLength = lpmii->cch * sizeof(WCHAR);
      Source.Buffer = lpmii->dwTypeData;

      MenuItem->Text.Buffer = (PWSTR)ExAllocatePoolWithTag(
        PagedPool, Source.Length + sizeof(WCHAR), TAG_STRING);
      if(MenuItem->Text.Buffer != NULL)
      {
        MenuItem->Text.Length = 0;
        MenuItem->Text.MaximumLength = Source.Length + sizeof(WCHAR);
        RtlCopyUnicodeString(&MenuItem->Text, &Source);
        MenuItem->Text.Buffer[MenuItem->Text.Length / sizeof(WCHAR)] = 0;
      }
      else
      {
        RtlInitUnicodeString(&MenuItem->Text, NULL);
      }
    }
    else
    {
      MenuItem->fType |= MF_SEPARATOR;
      RtlInitUnicodeString(&MenuItem->Text, NULL);
    }
  }

  if (sizeof(ROSMENUITEMINFO) == lpmii->cbSize)
    {
      MenuItem->Rect = lpmii->Rect;
      MenuItem->XTab = lpmii->XTab;
    }

  return TRUE;
}

BOOL FASTCALL
IntInsertMenuItem(PMENU_OBJECT MenuObject, UINT uItem, BOOL fByPosition,
                  PROSMENUITEMINFO ItemInfo)
{
  int pos = (int)uItem;
  PMENU_ITEM MenuItem;

  if (MAX_MENU_ITEMS <= MenuObject->MenuInfo.MenuItemCount)
    {
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return FALSE;
    }

  if (fByPosition)
    {
      /* calculate position */
      if(MenuObject->MenuInfo.MenuItemCount < pos)
        {
          pos = MenuObject->MenuInfo.MenuItemCount;
        }
    }
  else
    {
      pos = UserGetMenuItemByFlag(MenuObject, uItem, MF_BYCOMMAND, NULL, NULL);
    }
  if (pos < -1)
    {
      pos = -1;
    }

  MenuItem = ExAllocatePoolWithTag(PagedPool, sizeof(MENU_ITEM), TAG_MENUITEM);
  if (NULL == MenuItem)
    {
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return FALSE;
    }

  MenuItem->fType = MFT_STRING;
  MenuItem->fState = MFS_ENABLED | MFS_UNCHECKED;
  MenuItem->wID = 0;
  MenuItem->hSubMenu = (HMENU)0;
  MenuItem->hbmpChecked = (HBITMAP)0;
  MenuItem->hbmpUnchecked = (HBITMAP)0;
  MenuItem->dwItemData = 0;
  RtlInitUnicodeString(&MenuItem->Text, NULL);
  MenuItem->hbmpItem = (HBITMAP)0;

  if (! UserSetMenuItemInfo(MenuObject, MenuItem, ItemInfo))
    {
      ExFreePool(MenuItem);
      return FALSE;
    }

  /* Force size recalculation! */
  MenuObject->MenuInfo.Height = 0;

  pos = IntInsertMenuItemToList(MenuObject, MenuItem, pos);

  return pos >= 0;
}

UINT FASTCALL
IntEnableMenuItem(PMENU_OBJECT MenuObject, UINT uIDEnableItem, UINT uEnable)
{
  PMENU_ITEM MenuItem;
  UINT res = UserGetMenuItemByFlag(MenuObject, uIDEnableItem, uEnable, &MenuItem, NULL);
  if(!MenuItem || (res == (UINT)-1))
  {
    return (UINT)-1;
  }

  res = MenuItem->fState & (MF_GRAYED | MF_DISABLED);

  if(uEnable & MF_DISABLED)
  {
    if(!(MenuItem->fState & MF_DISABLED))
        MenuItem->fState |= MF_DISABLED;
    if(uEnable & MF_GRAYED)
    {
      if(!(MenuItem->fState & MF_GRAYED))
          MenuItem->fState |= MF_GRAYED;
    }
  }
  else
  {
    if(uEnable & MF_GRAYED)
    {
      if(!(MenuItem->fState & MF_GRAYED))
          MenuItem->fState |= MF_GRAYED;
      if(!(MenuItem->fState & MF_DISABLED))
          MenuItem->fState |= MF_DISABLED;
    }
    else
    {
      if(MenuItem->fState & MF_DISABLED)
          MenuItem->fState ^= MF_DISABLED;
      if(MenuItem->fState & MF_GRAYED)
          MenuItem->fState ^= MF_GRAYED;
    }
  }

  return res;
}


DWORD FASTCALL
IntBuildMenuItemList(PMENU_OBJECT MenuObject, PVOID Buffer, ULONG nMax)
{
  DWORD res = 0;
  UINT sz;
  ROSMENUITEMINFO mii;
  PVOID Buf;
  PMENU_ITEM CurItem = MenuObject->MenuItemList;
  PWCHAR StrOut;
  NTSTATUS Status;
  WCHAR NulByte;

  if (0 != nMax)
    {
      if (nMax < MenuObject->MenuInfo.MenuItemCount * sizeof(ROSMENUITEMINFO))
        {
          return 0;
        }
      StrOut = (PWCHAR)((char *) Buffer + MenuObject->MenuInfo.MenuItemCount
                                          * sizeof(ROSMENUITEMINFO));
      nMax -= MenuObject->MenuInfo.MenuItemCount * sizeof(ROSMENUITEMINFO);
      sz = sizeof(ROSMENUITEMINFO);
      Buf = Buffer;
      mii.cbSize = sizeof(ROSMENUITEMINFO);
      mii.fMask = 0;
      NulByte = L'\0';

      while (NULL != CurItem)
        {
          mii.cch = CurItem->Text.Length / sizeof(WCHAR);
          mii.dwItemData = CurItem->dwItemData;
          if (0 != CurItem->Text.Length)
            {
              mii.dwTypeData = StrOut;
            }
          else
            {
              mii.dwTypeData = NULL;
            }
          mii.fState = CurItem->fState;
          mii.fType = CurItem->fType;
          mii.hbmpChecked = CurItem->hbmpChecked;
          mii.hbmpItem = CurItem->hbmpItem;
          mii.hbmpUnchecked = CurItem->hbmpUnchecked;
          mii.hSubMenu = CurItem->hSubMenu;
          mii.Rect = CurItem->Rect;
          mii.XTab = CurItem->XTab;

          Status = MmCopyToCaller(Buf, &mii, sizeof(ROSMENUITEMINFO));
          if (! NT_SUCCESS(Status))
            {
              SetLastNtError(Status);
              return 0;
            }
          Buf = (PVOID)((ULONG_PTR)Buf + sizeof(ROSMENUITEMINFO));

          if (0 != CurItem->Text.Length
              && (nMax >= CurItem->Text.Length + sizeof(WCHAR)))
            {
              /* copy string */
              Status = MmCopyToCaller(StrOut, CurItem->Text.Buffer,
                                      CurItem->Text.Length);
              if (! NT_SUCCESS(Status))
                {
                  SetLastNtError(Status);
                  return 0;
                }
              StrOut += CurItem->Text.Length / sizeof(WCHAR);
              Status = MmCopyToCaller(StrOut, &NulByte, sizeof(WCHAR));
              if (! NT_SUCCESS(Status))
                {
                  SetLastNtError(Status);
                  return 0;
                }
              StrOut++;
              nMax -= CurItem->Text.Length + sizeof(WCHAR);
            }
          else if (0 != CurItem->Text.Length)
            {
              break;
            }

          CurItem = CurItem->Next;
          res++;
        }
    }
  else
    {
      while (NULL != CurItem)
        {
          res += sizeof(ROSMENUITEMINFO) + CurItem->Text.Length + sizeof(WCHAR);
          CurItem = CurItem->Next;
        }
    }

  return res;
}


DWORD FASTCALL
IntCheckMenuItem(PMENU_OBJECT MenuObject, UINT uIDCheckItem, UINT uCheck)
{
  PMENU_ITEM MenuItem;
  int res = -1;

  if((UserGetMenuItemByFlag(MenuObject, uIDCheckItem, uCheck, &MenuItem, NULL) < 0) || !MenuItem)
  {
    return -1;
  }

  res = (DWORD)(MenuItem->fState & MF_CHECKED);
  if(uCheck & MF_CHECKED)
  {
    if(!(MenuItem->fState & MF_CHECKED))
        MenuItem->fState |= MF_CHECKED;
  }
  else
  {
    if(MenuItem->fState & MF_CHECKED)
        MenuItem->fState ^= MF_CHECKED;
  }

  return (DWORD)res;
}

BOOL FASTCALL
IntHiliteMenuItem(PWINDOW_OBJECT WindowObject, PMENU_OBJECT MenuObject,
  UINT uItemHilite, UINT uHilite)
{
  PMENU_ITEM MenuItem;
  BOOL res = UserGetMenuItemByFlag(MenuObject, uItemHilite, uHilite, &MenuItem, NULL);
  if(!MenuItem || !res)
  {
    return FALSE;
  }

  if(uHilite & MF_HILITE)
  {
    if(!(MenuItem->fState & MF_HILITE))
        MenuItem->fState |= MF_HILITE;
  }
  else
  {
    if(MenuItem->fState & MF_HILITE)
        MenuItem->fState ^= MF_HILITE;
  }

  /* FIXME - update the window's menu */

  return TRUE;
}

BOOL FASTCALL
IntSetMenuDefaultItem(PMENU_OBJECT MenuObject, UINT uItem, UINT fByPos)
{
  BOOL ret = FALSE;
  PMENU_ITEM MenuItem = MenuObject->MenuItemList;

  if(uItem == (UINT)-1)
  {
    while(MenuItem)
    {
      if(MenuItem->fState & MFS_DEFAULT)
        MenuItem->fState ^= MFS_DEFAULT;
      MenuItem = MenuItem->Next;
    }
    return TRUE;
  }

  if(fByPos)
  {
    UINT pos = 0;
    while(MenuItem)
    {
      if(pos == uItem)
      {
        if(!(MenuItem->fState & MFS_DEFAULT))
          MenuItem->fState |= MFS_DEFAULT;
        ret = TRUE;
      }
      else
      {
        if(MenuItem->fState & MFS_DEFAULT)
          MenuItem->fState ^= MFS_DEFAULT;
      }
      pos++;
      MenuItem = MenuItem->Next;
    }
  }
  else
  {
    while(MenuItem)
    {
      if(!ret && (MenuItem->wID == uItem))
      {
        if(!(MenuItem->fState & MFS_DEFAULT))
          MenuItem->fState |= MFS_DEFAULT;
        ret = TRUE;
      }
      else
      {
        if(MenuItem->fState & MFS_DEFAULT)
          MenuItem->fState ^= MFS_DEFAULT;
      }
      MenuItem = MenuItem->Next;
    }
  }
  return ret;
}


UINT FASTCALL
IntGetMenuDefaultItem(PMENU_OBJECT MenuObject, UINT fByPos, UINT gmdiFlags,
  DWORD *gismc)
{
  UINT x = 0;
  UINT res = -1;
  UINT sres;
  PMENU_OBJECT SubMenuObject;
  PMENU_ITEM MenuItem = MenuObject->MenuItemList;

  while(MenuItem)
  {
    if(MenuItem->fState & MFS_DEFAULT)
    {

      if(!(gmdiFlags & GMDI_USEDISABLED) && (MenuItem->fState & MFS_DISABLED))
        break;

      if(fByPos & MF_BYPOSITION)
        res = x;
      else
        res = MenuItem->wID;

      if((*gismc < MAX_GOINTOSUBMENU) && (gmdiFlags & GMDI_GOINTOPOPUPS) &&
         MenuItem->hSubMenu)
      {

        SubMenuObject = UserGetMenuObject(MenuItem->hSubMenu);
        if(!SubMenuObject || (SubMenuObject == MenuObject))
          break;

        (*gismc)++;
        sres = IntGetMenuDefaultItem(SubMenuObject, fByPos, gmdiFlags, gismc);
        (*gismc)--;

        if(sres > (UINT)-1)
          res = sres;
      }

      break;
    }

    MenuItem = MenuItem->Next;
    x++;
  }

  return res;
}


BOOL FASTCALL
IntSetMenuItemRect(PMENU_OBJECT MenuObject, UINT Item, BOOL fByPos, RECT *rcRect)
{
  PMENU_ITEM mi;
  if(UserGetMenuItemByFlag(MenuObject, Item, (fByPos ? MF_BYPOSITION : MF_BYCOMMAND),
                          &mi, NULL) > -1)
  {
    mi->Rect = *rcRect;
    return TRUE;
  }
  return FALSE;
}


/*!
 * Internal function. Called when the process is destroyed to free the remaining menu handles.
*/
BOOL FASTCALL
IntCleanupMenus(PW32PROCESS WProcess)
{
  PMENU_OBJECT MenuObject;

  while (!IsListEmpty(&WProcess->MenuListHead))
  {
    MenuObject = CONTAINING_RECORD(WProcess->MenuListHead.Flink, MENU_OBJECT, ListEntry);
    /* UserDestroyMenu removes itself from list */
    UserDestroyMenu(MenuObject, FALSE);
  }

  return TRUE;
}

/* FUNCTIONS *****************************************************************/


/*
 * @implemented
 */
DWORD
STDCALL
NtUserBuildMenuItemList(
 HMENU hMenu,
 VOID* Buffer,
 ULONG nBufSize,
 DWORD Reserved)
{
  DWORD res = -1;
  DECLARE_RETURN(DWORD);  
  PMENU_OBJECT MenuObject;
  
  DPRINT("Enter NtUserBuildMenuItemList\n");
  UserEnterExclusive();
  
  MenuObject = UserGetMenuObject(hMenu);
  if(!MenuObject)
  {
    SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
    RETURN( (DWORD)-1);
  }

  if(Buffer)
  {
    res = IntBuildMenuItemList(MenuObject, Buffer, nBufSize);
  }
  else
  {
    res = MenuObject->MenuInfo.MenuItemCount;
  }

  RETURN( res);
  
CLEANUP:
  DPRINT("Leave NtUserBuildMenuItemList, ret=%i\n",_ret_);
  UserLeave();
  END_CLEANUP;
}


/*
 * @implemented
 */
DWORD STDCALL
NtUserCheckMenuItem(
  HMENU hmenu,
  UINT uIDCheckItem,
  UINT uCheck)
{
  DWORD res = 0;
  PMENU_OBJECT MenuObject;
  DECLARE_RETURN(DWORD); 
  
  DPRINT("Enter NtUserCheckMenuItem\n");
  UserEnterExclusive();
  
  MenuObject = UserGetMenuObject(hmenu);
  if(!MenuObject)
  {
    SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
    RETURN( (DWORD)-1);
  }
  res = IntCheckMenuItem(MenuObject, uIDCheckItem, uCheck);
  RETURN( res);
  
CLEANUP:
  DPRINT("Leave NtUserCheckMenuItem, ret=%i\n",_ret_);
  UserLeave();
  END_CLEANUP;  
}


#define GetHmenu(menu) ((menu) ? (menu)->MenuInfo.Self : NULL)

/*
 * @unimplemented
 */
HMENU STDCALL
NtUserCreateMenu(BOOL PopupMenu)
{
  DECLARE_RETURN(HMENU);  
  
  DPRINT("Enter NtUserCreateMenu\n");
  UserEnterExclusive();
  
  RETURN(GetHmenu( UserCreateMenu(PopupMenu)) );
  
CLEANUP:
  DPRINT("Leave NtUserCreateMenu, ret=%i\n",_ret_);
  UserLeave();
  END_CLEANUP;
}



/*
 * @implemented
 */
BOOL STDCALL
NtUserDeleteMenu(
  HMENU hMenu,
  UINT uPosition,
  UINT uFlags)
{
  PMENU_OBJECT MenuObject;
  DECLARE_RETURN(BOOL);  

  DPRINT("Enter NtUserDeleteMenu\n");
  UserEnterExclusive();
  
  MenuObject = UserGetMenuObject(hMenu);
  if(!MenuObject)
  {
    SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
    RETURN( FALSE);
  }

  RETURN(IntRemoveMenuItem(MenuObject, uPosition, uFlags, TRUE));

CLEANUP:
  DPRINT("Leave NtUserDeleteMenu, ret=%i\n",_ret_);
  UserLeave();
  END_CLEANUP;
}


/*
 * @implemented
 */
BOOL STDCALL
NtUserDestroyMenu(
  HMENU hMenu)
{
  PMENU_OBJECT MenuObject;
  DECLARE_RETURN(BOOL);  

  DPRINT("Enter NtUserDestroyMenu\n");
  UserEnterExclusive();
  
  MenuObject = UserGetMenuObject(hMenu);
  if(!MenuObject)
  {
    SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
    RETURN(FALSE);
  }
  if(MenuObject->Process != PsGetCurrentProcess())
  {
    SetLastWin32Error(ERROR_ACCESS_DENIED);
    RETURN(FALSE);
  }

  UserDestroyMenu(MenuObject, TRUE);//
  //RETURN(GetHmenu(UserDestroyMenu(MenuObject, TRUE)));
  RETURN(TRUE);

CLEANUP:
  DPRINT("Leave NtUserDestroyMenu, ret=%i\n",_ret_);
  UserLeave();
  END_CLEANUP;
}


/*
 * @implemented
 */
UINT STDCALL
NtUserEnableMenuItem(
  HMENU hMenu,
  UINT uIDEnableItem,
  UINT uEnable)
{
  PMENU_OBJECT MenuObject;
  DECLARE_RETURN(UINT);  

  DPRINT("Enter NtUserEnableMenuItem\n");
  UserEnterExclusive();
  
  MenuObject = UserGetMenuObject(hMenu);
  if(!MenuObject)
  {
    SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
    RETURN((UINT)-1);
  }

  RETURN(IntEnableMenuItem(MenuObject, uIDEnableItem, uEnable));

CLEANUP:
  DPRINT("Leave NtUserEnableMenuItem, ret=%i\n",_ret_);
  UserLeave();
  END_CLEANUP;
}


/*
 * @implemented
 */
DWORD STDCALL
NtUserInsertMenuItem(
  HMENU hMenu,
  UINT uItem,
  BOOL fByPosition,
  LPCMENUITEMINFOW UnsafeItemInfo)
{
  PMENU_OBJECT MenuObject;
  NTSTATUS Status;
  ROSMENUITEMINFO ItemInfo;
  DECLARE_RETURN(DWORD);  

  DPRINT("Enter NtUserInsertMenuItem\n");
  UserEnterExclusive();
  
  MenuObject = UserGetMenuObject(hMenu);
  if(!MenuObject)
    {
      SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
      RETURN(0);
    }

  Status = MmCopyFromCaller(&ItemInfo, UnsafeItemInfo, sizeof(MENUITEMINFOW));
  if (! NT_SUCCESS(Status))
    {
      SetLastNtError(Status);
      RETURN(FALSE);
    }
  if (ItemInfo.cbSize != sizeof(MENUITEMINFOW))
    {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      RETURN(FALSE);
    }

  RETURN(IntInsertMenuItem(MenuObject, uItem, fByPosition, &ItemInfo));
  
CLEANUP:
  DPRINT("Leave NtUserInsertMenuItem, ret=%i\n",_ret_);
  UserLeave();
  END_CLEANUP;
}


/*
 * @unimplemented
 */
BOOL STDCALL
NtUserEndMenu(VOID)
{
  UNIMPLEMENTED

  return 0;
}


/*
 * @implemented
 */
UINT STDCALL
NtUserGetMenuDefaultItem(
  HMENU hMenu,
  UINT fByPos,
  UINT gmdiFlags)
{
  PMENU_OBJECT MenuObject;
  DWORD gismc = 0;
  DECLARE_RETURN(UINT);  

  DPRINT("Enter NtUserGetMenuDefaultItem\n");
  UserEnterExclusive();
  
  MenuObject = UserGetMenuObject(hMenu);
  if(!MenuObject)
  {
    SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
    RETURN(-1);
  }

  RETURN(IntGetMenuDefaultItem(MenuObject, fByPos, gmdiFlags, &gismc));

CLEANUP:
  DPRINT("Leave NtUserGetMenuDefaultItem, ret=%i\n",_ret_);
  UserLeave();
  END_CLEANUP;
}


/*
 * @unimplemented
 */
BOOL STDCALL
NtUserGetMenuBarInfo(
  HWND hwnd,
  LONG idObject,
  LONG idItem,
  PMENUBARINFO pmbi)
{
  UNIMPLEMENTED

  return 0;
}


/*
 * @unimplemented
 */
UINT STDCALL
NtUserGetMenuIndex(
  HMENU hMenu,
  UINT wID)
{
  UNIMPLEMENTED

  return 0;
}


/*
 * @implemented
 */
BOOL STDCALL
NtUserGetMenuItemRect(
  HWND hWnd,
  HMENU hMenu,
  UINT uItem,
  LPRECT lprcItem)
{
     ROSMENUINFO mi;
     ROSMENUITEMINFO mii;
     HWND referenceHwnd;
     LPPOINT lpPoints;
     LPRECT lpRect;
     POINT FromOffset;
     LONG XMove, YMove;
     ULONG i;
     NTSTATUS Status;
     DECLARE_RETURN(BOOL);  
     PMENU_OBJECT Menu;

     DPRINT("Enter NtUserGetMenuItemRect\n");
     UserEnterExclusive();
  
     Menu = UserGetMenuObject(hMenu);
     if(!Menu)
     {
       SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
       RETURN(FALSE);
     }

     if(!UserMenuItemInfo(Menu, uItem, MF_BYPOSITION, &mii, FALSE))
     	RETURN(FALSE);
     	
     referenceHwnd = hWnd;

     if(!hWnd)
     {
        //uhm... vi hentet da allerede mii ovenfor??
   if(!UserGetMenuInfo(Menu, &mi)) RETURN(FALSE);
    if(mi.Wnd == 0) RETURN(FALSE);
	 referenceHwnd = mi.Wnd;
     }

     if (lprcItem == NULL) RETURN(FALSE);
     *lpRect = mii.Rect;
     lpPoints = (LPPOINT)lpRect;
      
    if(!UserGetClientOrigin(GetWnd(referenceHwnd), &FromOffset)) 
      RETURN(FALSE);

    XMove = FromOffset.x;
    YMove = FromOffset.y;

    for (i = 0; i < 2; i++)
      {
        lpPoints[i].x += XMove;
        lpPoints[i].y += YMove;
      }
      
    Status = MmCopyToCaller(lprcItem, lpPoints, sizeof(POINT));
    if (! NT_SUCCESS(Status))
      {
        SetLastNtError(Status);
        RETURN(FALSE);
      }                                                              
    RETURN(TRUE);
    
CLEANUP:
  DPRINT("Leave NtUserGetMenuItemRect, ret=%i\n",_ret_);
  UserLeave();
  END_CLEANUP;
}


/*
 * @implemented
 */
BOOL STDCALL
NtUserHiliteMenuItem(
  HWND hwnd,
  HMENU hmenu,
  UINT uItemHilite,
  UINT uHilite)
{
  PMENU_OBJECT MenuObject;
  PWINDOW_OBJECT WindowObject;
  DECLARE_RETURN(BOOLEAN);  

  DPRINT("Enter NtUserHiliteMenuItem\n");
  UserEnterExclusive();
  
  WindowObject = IntGetWindowObject(hwnd);
  if(!WindowObject)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    RETURN(FALSE);
  }
  MenuObject = UserGetMenuObject(hmenu);
  if(!MenuObject)
  {
    SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
    RETURN(FALSE);
  }
  if(WindowObject->IDMenu == (UINT)hmenu)
  {
    RETURN(IntHiliteMenuItem(WindowObject, MenuObject, uItemHilite, uHilite));
  }
  RETURN(FALSE); 
   
CLEANUP:
  DPRINT("Leave NtUserHiliteMenuItem, ret=%i\n",_ret_);
  UserLeave();
  END_CLEANUP;
}


/*
 * @implemented
 */
BOOL
STDCALL
NtUserMenuInfo(
 HMENU Menu,
 PROSMENUINFO UnsafeMenuInfo,
 BOOL Set)
{
  BOOL Res;
  PMENU_OBJECT MenuObject;
  DWORD Size;
  NTSTATUS Status;
  ROSMENUINFO MenuInfo;
  DECLARE_RETURN(BOOL);  

  DPRINT("Enter NtUserMenuInfo\n");
  UserEnterExclusive();
  
  Status = MmCopyFromCaller(&Size, &UnsafeMenuInfo->cbSize, sizeof(DWORD));
  if (! NT_SUCCESS(Status))
    {
      SetLastNtError(Status);
      return FALSE;
    }
  if(Size < sizeof(MENUINFO) || sizeof(ROSMENUINFO) < Size)
    {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      RETURN(FALSE);
    }
  Status = MmCopyFromCaller(&MenuInfo, UnsafeMenuInfo, Size);
  if (! NT_SUCCESS(Status))
    {
      SetLastNtError(Status);
      RETURN(FALSE);
    }

  MenuObject = UserGetMenuObject(Menu);
  if (NULL == MenuObject)
    {
      SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
      RETURN(FALSE);
    }

  if(Set)
    {
      /* Set MenuInfo */
      Res = UserSetMenuInfo(MenuObject, &MenuInfo);
    }
  else
    {
      /* Get MenuInfo */
      Res = UserGetMenuInfo(MenuObject, &MenuInfo);
      if (Res)
        {
          Status = MmCopyToCaller(UnsafeMenuInfo, &MenuInfo, Size);
          if (! NT_SUCCESS(Status))
            {
              SetLastNtError(Status);
              RETURN(FALSE);
            }
        }
    }

  RETURN(Res);
  
CLEANUP:
  DPRINT("Leave NtUserMenuInfo, ret=%i\n",_ret_);
  UserLeave();
  END_CLEANUP;
}


/*
 * @implemented
 */
int STDCALL
NtUserMenuItemFromPoint(
  HWND Wnd,
  HMENU Menu,
  DWORD X,
  DWORD Y)
{
  PMENU_OBJECT MenuObject;
  PWINDOW_OBJECT WindowObject = NULL;
  PMENU_ITEM mi;
  int i;
  DECLARE_RETURN(int);  

  DPRINT("Enter NtUserMenuItemFromPoint\n");
  UserEnterExclusive();
  
  MenuObject = UserGetMenuObject(Menu);
  if (NULL == MenuObject)
    {
      SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
      RETURN( -1);
    }

  WindowObject = IntGetWindowObject(MenuObject->MenuInfo.Wnd);
  if (NULL == WindowObject)
    {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      RETURN( -1);
    }
  X -= WindowObject->WindowRect.left;
  Y -= WindowObject->WindowRect.top;

  mi = MenuObject->MenuItemList;
  for (i = 0; NULL != mi; i++)
    {
      if (InRect(mi->Rect, X, Y))
        {
          break;
        }
      mi = mi->Next;
    }

  RETURN (mi ? i : NO_SELECTED_ITEM);
  
CLEANUP:
  DPRINT("Leave NtUserMenuItemFromPoint, ret=%i\n",_ret_);
  UserLeave();
  END_CLEANUP;
}


/*
 * @implemented
 */
BOOL
STDCALL
NtUserMenuItemInfo(
 HMENU Menu,
 UINT Item,
 BOOL ByPosition,
 PROSMENUITEMINFO UnsafeItemInfo,
 BOOL Set)
{
  PMENU_OBJECT MenuObject;
//  PMENU_ITEM MenuItem;
  ROSMENUITEMINFO ItemInfo;
  NTSTATUS Status;
  UINT Size;
  BOOL Ret;
  DECLARE_RETURN(BOOL);  

  DPRINT("Enter NtUserMenuItemInfo\n");
  UserEnterExclusive();
  
  MenuObject = UserGetMenuObject(Menu);
  if (NULL == MenuObject)
    {
      SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
      RETURN(FALSE);
    }
  Status = MmCopyFromCaller(&Size, &UnsafeItemInfo->cbSize, sizeof(UINT));
  if (! NT_SUCCESS(Status))
    {
      SetLastNtError(Status);
      RETURN(FALSE);
    }
  if (sizeof(MENUITEMINFOW) != Size
      && sizeof(MENUITEMINFOW) - sizeof(HBITMAP) != Size
      && sizeof(ROSMENUITEMINFO) != Size)
    {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      RETURN(FALSE);
    }
  Status = MmCopyFromCaller(&ItemInfo, UnsafeItemInfo, Size);
  if (! NT_SUCCESS(Status))
    {
      SetLastNtError(Status);
      RETURN(FALSE);
    }
  /* If this is a pre-0x0500 _WIN32_WINNT MENUITEMINFOW, you can't
     set/get hbmpItem */
  if (sizeof(MENUITEMINFOW) - sizeof(HBITMAP) == Size
      && 0 != (ItemInfo.fMask & MIIM_BITMAP))
    {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      RETURN(FALSE);
    }

   Ret = UserMenuItemInfo(MenuObject, Item,  ByPosition, &ItemInfo, Set);

    if (!Set && Ret)
    {
       Status = MmCopyToCaller(UnsafeItemInfo, &ItemInfo, Size);
       if (! NT_SUCCESS(Status))
         {
           SetLastNtError(Status);
           RETURN(FALSE);
         }
    }

  RETURN(Ret);
  
CLEANUP:
  DPRINT("Leave NtUserMenuItemInfo, ret=%i\n",_ret_);
  UserLeave();
  END_CLEANUP;
}




BOOL FASTCALL
UserMenuItemInfo(
   PMENU_OBJECT Menu,
   UINT Item,
   BOOL ByPosition,
   PROSMENUITEMINFO ItemInfo,
   BOOL Set
   )
{
  PMENU_ITEM MenuItem;

  if (UserGetMenuItemByFlag(Menu, Item,
                           (ByPosition ? MF_BYPOSITION : MF_BYCOMMAND),
                           &MenuItem, NULL) < 0)
  {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return(FALSE);
  }

  if (Set)
    {
      return UserSetMenuItemInfo(Menu, MenuItem, ItemInfo);
    }
  else
    {
      return UserGetMenuItemInfo(Menu, MenuItem, ItemInfo);
    }
}




/*
 * @implemented
 */
BOOL STDCALL
NtUserRemoveMenu(
  HMENU hMenu,
  UINT uPosition,
  UINT uFlags)
{
  PMENU_OBJECT MenuObject;
  DECLARE_RETURN(BOOL);  

  DPRINT("Enter NtUserRemoveMenu\n");
  UserEnterExclusive();
  
  MenuObject = UserGetMenuObject(hMenu);
  if(!MenuObject)
  {
    SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
    RETURN(FALSE);
  }

  RETURN(IntRemoveMenuItem(MenuObject, uPosition, uFlags, FALSE));
  
CLEANUP:
  DPRINT("Leave NtUserRemoveMenu, ret=%i\n",_ret_);
  UserLeave();
  END_CLEANUP;
}


/*
 * @implemented
 */
BOOL STDCALL
NtUserSetMenuContextHelpId(
  HMENU hmenu,
  DWORD dwContextHelpId)
{
  PMENU_OBJECT MenuObject;
  DECLARE_RETURN(BOOL);  

  DPRINT("Enter NtUserSetMenuContextHelpId\n");
  UserEnterExclusive();
  
  MenuObject = UserGetMenuObject(hmenu);
  if(!MenuObject)
  {
    SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
    RETURN(FALSE);
  }

  RETURN(IntSetMenuContextHelpId(MenuObject, dwContextHelpId));
  
CLEANUP:
  DPRINT("Leave NtUserSetMenuContextHelpId, ret=%i\n",_ret_);
  UserLeave();
  END_CLEANUP;
}


/*
 * @implemented
 */
BOOL STDCALL
NtUserSetMenuDefaultItem(
  HMENU hMenu,
  UINT uItem,
  UINT fByPos)
{
  PMENU_OBJECT MenuObject;
  DECLARE_RETURN(BOOL);  

  DPRINT("Enter NtUserSetMenuDefaultItem\n");
  UserEnterExclusive();
  
  MenuObject = UserGetMenuObject(hMenu);
  if(!MenuObject)
  {
    SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
    RETURN(FALSE);
  }

  RETURN( IntSetMenuDefaultItem(MenuObject, uItem, fByPos));
  
CLEANUP:
  DPRINT("Leave NtUserSetMenuDefaultItem, ret=%i\n",_ret_);
  UserLeave();
  END_CLEANUP;
}


/*
 * @implemented
 */
BOOL STDCALL
NtUserSetMenuFlagRtoL(
  HMENU hMenu)
{
  PMENU_OBJECT MenuObject;
  DECLARE_RETURN(BOOL);  

  DPRINT("Enter NtUserSetMenuFlagRtoL\n");
  UserEnterExclusive();
  
  MenuObject = UserGetMenuObject(hMenu);
  if(!MenuObject)
  {
    SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
    RETURN( FALSE);
  }

  RETURN( UserSetMenuFlagRtoL(MenuObject));

CLEANUP:
  DPRINT("Leave NtUserSetMenuFlagRtoL, ret=%i\n",_ret_);
  UserLeave();
  END_CLEANUP;
}


/*
 * @unimplemented
 */
DWORD STDCALL
NtUserThunkedMenuInfo(
  HMENU hMenu,
  LPCMENUINFO lpcmi)
{
  UNIMPLEMENTED
  /* This function seems just to call SetMenuInfo() */
  return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
NtUserThunkedMenuItemInfo(
  HMENU hMenu,
  UINT uItem,
  BOOL fByPosition,
  BOOL bInsert,
  LPMENUITEMINFOW lpmii,
  PUNICODE_STRING lpszCaption)
{
  UNIMPLEMENTED
  /* lpszCaption may be NULL, check for it and call RtlInitUnicodeString()
     if bInsert == TRUE call NtUserInsertMenuItem() else NtUserSetMenuItemInfo()
  */
  return 0;
}


/*
 * @implemented
 */
BOOL STDCALL
NtUserTrackPopupMenuEx(
  HMENU hmenu,
  UINT fuFlags,
  int x,
  int y,
  HWND hwnd,
  LPTPMPARAMS lptpm)
{
  /* unused/placeholder */
  UNIMPLEMENTED
  
  return FALSE;
}






PMENU_OBJECT FASTCALL
coUserGetSystemMenu(PWINDOW_OBJECT WindowObject, BOOL bRevert, BOOL RetMenu)
{
  PMENU_OBJECT MenuObject, NewMenuObject, SysMenuObject, ret = NULL;
  PW32THREAD WThread;
  HMENU NewMenu;//, SysMenu;
  ROSMENUITEMINFO ItemInfo;

  if(bRevert)
  {
    WThread = PsGetWin32Thread();

    if(!WThread->Desktop)
      return NULL;

    if(WindowObject->SystemMenu)
    {
      MenuObject = UserGetMenuObject(WindowObject->SystemMenu);
      if(MenuObject)
      {
        UserDestroyMenu(MenuObject, FALSE);
        WindowObject->SystemMenu = (HMENU)0;
      }
    }

    if(WThread->Desktop->WinSta->SystemMenuTemplate)
    {
      /* clone system menu */
      MenuObject = UserGetMenuObject(WThread->Desktop->WinSta->SystemMenuTemplate);
      if(!MenuObject)
        return NULL;

      NewMenuObject = IntCloneMenu(MenuObject);
      if(NewMenuObject)
      {
        WindowObject->SystemMenu = NewMenuObject->MenuInfo.Self;
        NewMenuObject->MenuInfo.Flags |= MF_SYSMENU;
        NewMenuObject->MenuInfo.Wnd = WindowObject->hSelf;
        ret = NewMenuObject;
      }

    }
    else
    {
//      SysMenu = UserCreateMenu(FALSE);
//      if (NULL == SysMenu)
//      {
//        return NULL;
//      }
//      SysMenuObject = UserGetMenuObject(SysMenu);
//      if (NULL == SysMenuObject)
//      {
         //FIXME: huh??
        //NtUserDestroyMenu(SysMenu);
//        return NULL;
//      }
      SysMenuObject = UserCreateMenu(FALSE);
      if (!SysMenuObject)
      {
         return NULL;
      }
      SysMenuObject->MenuInfo.Flags |= MF_SYSMENU;
      SysMenuObject->MenuInfo.Wnd = WindowObject->hSelf;//FIXME
      NewMenu = coUserLoadSysMenuTemplate();
      if(!NewMenu)
      {
        //NtUserDestroyMenu(SysMenu);
        UserDestroyMenu(SysMenuObject, TRUE /* dont care*/ );
        return NULL;
      }
      MenuObject = UserGetMenuObject(NewMenu);
      if(!MenuObject)
      {
        //NtUserDestroyMenu(SysMenu);
        UserDestroyMenu(SysMenuObject, TRUE /* dont care*/);
        return NULL;
      }

      NewMenuObject = IntCloneMenu(MenuObject);
      if(NewMenuObject)
      {
        NewMenuObject->MenuInfo.Flags |= MF_SYSMENU | MF_POPUP;
        IntSetMenuDefaultItem(NewMenuObject/*->MenuInfo.Self*/, SC_CLOSE, FALSE);

        ItemInfo.cbSize = sizeof(MENUITEMINFOW);
        ItemInfo.fMask = MIIM_FTYPE | MIIM_STRING | MIIM_STATE | MIIM_SUBMENU;
        ItemInfo.fType = MF_POPUP;
        ItemInfo.fState = MFS_ENABLED;
        ItemInfo.dwTypeData = NULL;
        ItemInfo.cch = 0;
        ItemInfo.hSubMenu = NewMenuObject->MenuInfo.Self;
        IntInsertMenuItem(SysMenuObject, (UINT) -1, TRUE, &ItemInfo);

        WindowObject->SystemMenu = SysMenuObject->MenuInfo.Self;

        ret = SysMenuObject;
      }
      UserDestroyMenu(MenuObject, FALSE);
    }
    if(RetMenu)
      return ret;
    else
      return NULL;
  }
  else
  {
    if(WindowObject->SystemMenu)
      return UserGetMenuObject((HMENU)WindowObject->SystemMenu);
    else
      return NULL;
  }
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

HMENU STDCALL
NtUserGetSystemMenu(HWND hWnd, BOOL bRevert)
{
   HMENU Result = 0;
   PWINDOW_OBJECT WindowObject;
   PMENU_OBJECT MenuObject;
   DECLARE_RETURN(HMENU);
   
   DPRINT("Enter NtUserGetSystemMenu\n");
   UserEnterExclusive();
  
   WindowObject = IntGetWindowObject((HWND)hWnd);
   if (WindowObject == NULL)
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      RETURN(0);
   }

   MenuObject = coUserGetSystemMenu(WindowObject, bRevert, FALSE);
   if (MenuObject)
   {
      Result = MenuObject->MenuInfo.Self;
   }

   RETURN(Result);
   
CLEANUP:
   DPRINT("Leave NtUserGetSystemMenu, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * NtUserSetSystemMenu
 *
 * Status
 *    @implemented
 */

BOOL STDCALL
NtUserSetSystemMenu(HWND hWnd, HMENU hMenu)
{
   BOOL Result = FALSE;
   PWINDOW_OBJECT WindowObject;
   PMENU_OBJECT MenuObject;
   DECLARE_RETURN(BOOL);
   
   DPRINT("Enter NtUserSetSystemMenu\n");
   UserEnterExclusive();
  
   WindowObject = IntGetWindowObject(hWnd);
   if (!WindowObject)
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      RETURN(FALSE);
   }

   if (hMenu)
   {
      /*
       * Assign new menu handle.
       */
      MenuObject = UserGetMenuObject(hMenu);
      if (!MenuObject)
      {
         SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
         RETURN(FALSE);
      }

      Result = UserSetSystemMenu(WindowObject, MenuObject);
   }

   RETURN(Result);
   
CLEANUP:
   DPRINT("Leave NtUserSetSystemMenu, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}



BOOL FASTCALL
UserSetMenu(
   PWINDOW_OBJECT WindowObject,
   HMENU Menu,
   BOOL *Changed)
{
  PMENU_OBJECT OldMenuObject, NewMenuObject = NULL;

  if ((WindowObject->Style & (WS_CHILD | WS_POPUP)) == WS_CHILD)
    {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return FALSE;
    }

  *Changed = (WindowObject->IDMenu != (UINT) Menu);
  if (! *Changed)
    {
      return TRUE;
    }

  if (WindowObject->IDMenu)
    {
      OldMenuObject = UserGetMenuObject((HMENU) WindowObject->IDMenu);
      ASSERT(NULL == OldMenuObject || OldMenuObject->MenuInfo.Wnd == WindowObject->hSelf);
    }
  else
    {
      OldMenuObject = NULL;
    }

  if (NULL != Menu)
    {
      NewMenuObject = UserGetMenuObject(Menu);
      if (NULL == NewMenuObject)
        {
          SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
          return FALSE;
        }
      if (NULL != NewMenuObject->MenuInfo.Wnd)
        {
          /* Can't use the same menu for two windows */
          SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
          return FALSE;
        }

    }

  WindowObject->IDMenu = (UINT) Menu;
  if (NULL != NewMenuObject)
    {
      NewMenuObject->MenuInfo.Wnd = WindowObject->hSelf;
    }
  if (NULL != OldMenuObject)
    {
      OldMenuObject->MenuInfo.Wnd = NULL;
    }

  return TRUE;
}



/*
 * @implemented
 */
BOOL STDCALL
NtUserSetMenu(
   HWND hWnd,
   HMENU Menu,
   BOOL Repaint)
{
  PWINDOW_OBJECT WindowObject;
  BOOL Changed;
  DECLARE_RETURN(BOOL);
  
  DPRINT("Enter NtUserSetMenu\n");
  UserEnterExclusive();

  WindowObject = IntGetWindowObject(hWnd);
  if (NULL == WindowObject)
    {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      RETURN(FALSE);
    }

  if (! UserSetMenu(WindowObject, Menu, &Changed))
    {
      RETURN(FALSE);
    }

  if (Changed && Repaint)
    {
      coWinPosSetWindowPos(hWnd, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE |
                         SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }

  RETURN(TRUE);
  
CLEANUP:
  DPRINT("Leave NtUserSetMenu, ret=%i\n",_ret_);
  UserLeave(); 
  END_CLEANUP;
}

/* EOF */
