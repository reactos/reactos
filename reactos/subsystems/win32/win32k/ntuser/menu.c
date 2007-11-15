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

PMENU_OBJECT FASTCALL
IntGetSystemMenu(PWINDOW_OBJECT Window, BOOL bRevert, BOOL RetMenu);



/* STATIC FUNCTION ***********************************************************/

static
BOOL FASTCALL
UserMenuItemInfo(
   PMENU_OBJECT Menu,
   UINT Item,
   BOOL ByPosition,
   PROSMENUITEMINFO UnsafeItemInfo,
   BOOL SetOrGet);

static
BOOL FASTCALL
UserMenuInfo(
   PMENU_OBJECT Menu,
   PROSMENUINFO UnsafeMenuInfo,
   BOOL SetOrGet);

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

PMENU_OBJECT FASTCALL UserGetMenuObject(HMENU hMenu)
{
   PMENU_OBJECT Menu;

   if (!hMenu)
   {
      SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
      return NULL;
   }

   Menu = (PMENU_OBJECT)UserGetObject(gHandleTable, hMenu, otMenu);
   if (!Menu)
   {
      SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
      return NULL;
   }

   ASSERT(USER_BODY_TO_HEADER(Menu)->RefCount >= 0);
   return Menu;
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
      MenuItem = MenuItem->Next;
   }
   DbgPrint("Entries: %d\n", cnt);
   return;
}
#endif

PMENU_OBJECT FASTCALL
IntGetMenuObject(HMENU hMenu)
{
   PMENU_OBJECT Menu = UserGetMenuObject(hMenu);
   if (Menu)
   {
      ASSERT(USER_BODY_TO_HEADER(Menu)->RefCount >= 0);

      USER_BODY_TO_HEADER(Menu)->RefCount++;
   }
   return Menu;
}

BOOL FASTCALL
IntFreeMenuItem(PMENU_OBJECT Menu, PMENU_ITEM MenuItem,
                BOOL RemoveFromList, BOOL bRecurse)
{
   FreeMenuText(MenuItem);
   if(RemoveFromList)
   {
      PMENU_ITEM CurItem = Menu->MenuItemList;
      while(CurItem)
      {
         if (CurItem->Next == MenuItem)
         {
            CurItem->Next = MenuItem->Next;
            break;
         }
         else
         {
            CurItem = CurItem->Next;
         }
      }
      Menu->MenuInfo.MenuItemCount--;
   }
   if(bRecurse && MenuItem->hSubMenu)
   {
      PMENU_OBJECT SubMenu;
      SubMenu = UserGetMenuObject(MenuItem->hSubMenu );
      if(SubMenu)
      {
         IntDestroyMenuObject(SubMenu, bRecurse, TRUE);
      }
   }

   /* Free memory */
   ExFreePool(MenuItem);

   return TRUE;
}

BOOL FASTCALL
IntRemoveMenuItem(PMENU_OBJECT Menu, UINT uPosition, UINT uFlags,
                  BOOL bRecurse)
{
   PMENU_ITEM PrevMenuItem, MenuItem;
   if(IntGetMenuItemByFlag(Menu, uPosition, uFlags, NULL, &MenuItem,
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
IntDeleteMenuItems(PMENU_OBJECT Menu, BOOL bRecurse)
{
   UINT res = 0;
   PMENU_ITEM NextItem;
   PMENU_ITEM CurItem = Menu->MenuItemList;
   while(CurItem)
   {
      NextItem = CurItem->Next;
      IntFreeMenuItem(Menu, CurItem, FALSE, bRecurse);
      CurItem = NextItem;
      res++;
   }
   Menu->MenuInfo.MenuItemCount = 0;
   Menu->MenuItemList = NULL;
   return res;
}

BOOL FASTCALL
IntDestroyMenuObject(PMENU_OBJECT Menu,
                     BOOL bRecurse, BOOL RemoveFromProcess)
{
   if(Menu)
   {
      PWINDOW_OBJECT Window;
      PWINSTATION_OBJECT WindowStation;
      NTSTATUS Status;

      /* remove all menu items */
      IntDeleteMenuItems(Menu, bRecurse); /* do not destroy submenus */

      if(RemoveFromProcess)
      {
         RemoveEntryList(&Menu->ListEntry);
      }

      Status = ObReferenceObjectByHandle(Menu->Process->Win32WindowStation,
                                         0,
                                         ExWindowStationObjectType,
                                         KernelMode,
                                         (PVOID*)&WindowStation,
                                         NULL);
      if(NT_SUCCESS(Status))
      {
         if (Menu->MenuInfo.Wnd)
         {
            Window = UserGetWindowObject(Menu->MenuInfo.Wnd);
            if (Window)
            {
               Window->IDMenu = 0;
            }
         }
         ObmDeleteObject(Menu->MenuInfo.Self, otMenu);
         ObDereferenceObject(WindowStation);
         return TRUE;
      }
   }
   return FALSE;
}

PMENU_OBJECT FASTCALL
IntCreateMenu(PHANDLE Handle, BOOL IsMenuBar)
{
   PMENU_OBJECT Menu;

   Menu = (PMENU_OBJECT)ObmCreateObject(
             gHandleTable, Handle,
             otMenu, sizeof(MENU_OBJECT));

   if(!Menu)
   {
      *Handle = 0;
      return NULL;
   }

   Menu->Process = PsGetCurrentProcess();
   Menu->RtoL = FALSE; /* default */
   Menu->MenuInfo.cbSize = sizeof(MENUINFO); /* not used */
   Menu->MenuInfo.fMask = 0; /* not used */
   Menu->MenuInfo.dwStyle = 0; /* FIXME */
   Menu->MenuInfo.cyMax = 0; /* default */
   Menu->MenuInfo.hbrBack =
      NtGdiCreateSolidBrush(RGB(192, 192, 192), 0); /* FIXME: default background color */
   Menu->MenuInfo.dwContextHelpID = 0; /* default */
   Menu->MenuInfo.dwMenuData = 0; /* default */
   Menu->MenuInfo.Self = *Handle;
   Menu->MenuInfo.FocusedItem = NO_SELECTED_ITEM;
   Menu->MenuInfo.Flags = (IsMenuBar ? 0 : MF_POPUP);
   Menu->MenuInfo.Wnd = NULL;
   Menu->MenuInfo.WndOwner = NULL;
   Menu->MenuInfo.Height = 0;
   Menu->MenuInfo.Width = 0;
   Menu->MenuInfo.TimeToHide = FALSE;

   Menu->MenuInfo.MenuItemCount = 0;
   Menu->MenuItemList = NULL;

   /* Insert menu item into process menu handle list */
   InsertTailList(&PsGetCurrentProcessWin32Process()->MenuListHead, &Menu->ListEntry);

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
   HANDLE hMenu;
   PMENU_OBJECT Menu;

   if(!Source)
      return NULL;

   Menu = (PMENU_OBJECT)ObmCreateObject(
             gHandleTable, &hMenu,
             otMenu, sizeof(MENU_OBJECT));

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
   InsertTailList(&PsGetCurrentProcessWin32Process()->MenuListHead, &Menu->ListEntry);

   IntCloneMenuItems(Menu, Source);

   return Menu;
}

BOOL FASTCALL
IntSetMenuFlagRtoL(PMENU_OBJECT Menu)
{
   Menu->RtoL = TRUE;
   return TRUE;
}

BOOL FASTCALL
IntSetMenuContextHelpId(PMENU_OBJECT Menu, DWORD dwContextHelpId)
{
   Menu->MenuInfo.dwContextHelpID = dwContextHelpId;
   return TRUE;
}

BOOL FASTCALL
IntGetMenuInfo(PMENU_OBJECT Menu, PROSMENUINFO lpmi)
{
   if(lpmi->fMask & MIM_BACKGROUND)
      lpmi->hbrBack = Menu->MenuInfo.hbrBack;
   if(lpmi->fMask & MIM_HELPID)
      lpmi->dwContextHelpID = Menu->MenuInfo.dwContextHelpID;
   if(lpmi->fMask & MIM_MAXHEIGHT)
      lpmi->cyMax = Menu->MenuInfo.cyMax;
   if(lpmi->fMask & MIM_MENUDATA)
      lpmi->dwMenuData = Menu->MenuInfo.dwMenuData;
   if(lpmi->fMask & MIM_STYLE)
      lpmi->dwStyle = Menu->MenuInfo.dwStyle;
   if (sizeof(MENUINFO) < lpmi->cbSize)
   {
      RtlCopyMemory((char *) lpmi + sizeof(MENUINFO),
                    (char *) &Menu->MenuInfo + sizeof(MENUINFO),
                    lpmi->cbSize - sizeof(MENUINFO));
   }
   if (sizeof(ROSMENUINFO) == lpmi->cbSize)
   {
     lpmi->maxBmpSize.cx = Menu->MenuInfo.maxBmpSize.cx;
     lpmi->maxBmpSize.cy = Menu->MenuInfo.maxBmpSize.cy;
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
IntSetMenuInfo(PMENU_OBJECT Menu, PROSMENUINFO lpmi)
{
   if(lpmi->fMask & MIM_BACKGROUND)
      Menu->MenuInfo.hbrBack = lpmi->hbrBack;
   if(lpmi->fMask & MIM_HELPID)
      Menu->MenuInfo.dwContextHelpID = lpmi->dwContextHelpID;
   if(lpmi->fMask & MIM_MAXHEIGHT)
      Menu->MenuInfo.cyMax = lpmi->cyMax;
   if(lpmi->fMask & MIM_MENUDATA)
      Menu->MenuInfo.dwMenuData = lpmi->dwMenuData;
   if(lpmi->fMask & MIM_STYLE)
      Menu->MenuInfo.dwStyle = lpmi->dwStyle;
   if(lpmi->fMask & MIM_APPLYTOSUBMENUS)
   {
      /* FIXME */
   }
   if (sizeof(MENUINFO) < lpmi->cbSize)
   {
      Menu->MenuInfo.FocusedItem = lpmi->FocusedItem;
      Menu->MenuInfo.Height = lpmi->Height;
      Menu->MenuInfo.Width = lpmi->Width;
      Menu->MenuInfo.Wnd = lpmi->Wnd;
      Menu->MenuInfo.WndOwner = lpmi->WndOwner;
      Menu->MenuInfo.TimeToHide = lpmi->TimeToHide;
   }
   if (sizeof(ROSMENUINFO) == lpmi->cbSize)
   {
     Menu->MenuInfo.maxBmpSize.cx = lpmi->maxBmpSize.cx;
     Menu->MenuInfo.maxBmpSize.cy = lpmi->maxBmpSize.cy;
   }
   return TRUE;
}


int FASTCALL
IntGetMenuItemByFlag(PMENU_OBJECT Menu, UINT uSearchBy, UINT fFlag,
                     PMENU_OBJECT *SubMenu, PMENU_ITEM *MenuItem,
                     PMENU_ITEM *PrevMenuItem)
{
   PMENU_ITEM PrevItem = NULL;
   PMENU_ITEM CurItem = Menu->MenuItemList;
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
         if(MenuItem)
            *MenuItem = CurItem;
         if(PrevMenuItem)
            *PrevMenuItem = PrevItem;
      }
      else
      {
         if(MenuItem)
            *MenuItem = NULL;
         if(PrevMenuItem)
            *PrevMenuItem = NULL; /* ? */
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
            if(MenuItem)
               *MenuItem = CurItem;
            if(PrevMenuItem)
               *PrevMenuItem = PrevItem;
            if(SubMenu)
                *SubMenu = Menu;

            return p;
         }
         else
         {
            if(CurItem->fType & MF_POPUP)
            {
               PMENU_OBJECT NewMenu = UserGetMenuObject(CurItem->hSubMenu);
               if(Menu)
               {
                   ret = IntGetMenuItemByFlag(NewMenu, uSearchBy, fFlag,
                                              SubMenu, MenuItem, PrevMenuItem);
                   if(ret != -1)
                   {
                      return ret;
                   }
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
IntInsertMenuItemToList(PMENU_OBJECT Menu, PMENU_ITEM MenuItem, int pos)
{
   PMENU_ITEM CurItem;
   PMENU_ITEM LastItem = NULL;
   UINT npos = 0;

   CurItem = Menu->MenuItemList;
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
         Menu->MenuItemList = MenuItem;
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
         Menu->MenuItemList = MenuItem;
         MenuItem->Next = NULL;
      }
   }
   Menu->MenuInfo.MenuItemCount++;

   return npos;
}

BOOL FASTCALL
IntGetMenuItemInfo(PMENU_OBJECT Menu, /* UNUSED PARAM!! */
                   PMENU_ITEM MenuItem, PROSMENUITEMINFO lpmii)
{
   NTSTATUS Status;

   if(lpmii->fMask & (MIIM_FTYPE | MIIM_TYPE))
   {
      lpmii->fType = MenuItem->fType;
   }
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

   if ((lpmii->fMask & MIIM_STRING) ||
      ((lpmii->fMask & MIIM_TYPE) && (MENU_ITEM_TYPE(lpmii->fType) == MF_STRING)))
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
      lpmii->Text = MenuItem->Text.Buffer;
   }

   return TRUE;
}

BOOL FASTCALL
IntSetMenuItemInfo(PMENU_OBJECT MenuObject, PMENU_ITEM MenuItem, PROSMENUITEMINFO lpmii)
{
   PMENU_OBJECT SubMenuObject;
   UINT fTypeMask = (MFT_BITMAP | MFT_MENUBARBREAK | MFT_MENUBREAK | MFT_OWNERDRAW | MFT_RADIOCHECK | MFT_RIGHTJUSTIFY | MFT_SEPARATOR | MF_POPUP);

   if(!MenuItem || !MenuObject || !lpmii)
   {
      return FALSE;
   }
   if( lpmii->fType & ~fTypeMask)
   {
     DbgPrint("IntSetMenuItemInfo invalid fType flags %x\n", lpmii->fType & ~fTypeMask);
     lpmii->fMask &= ~(MIIM_TYPE | MIIM_FTYPE);
   }
   if(lpmii->fMask &  MIIM_TYPE)
   {
      if(lpmii->fMask & ( MIIM_STRING | MIIM_FTYPE | MIIM_BITMAP))
      {
         DbgPrint("IntSetMenuItemInfo: Invalid combination of fMask bits used\n");
         /* this does not happen on Win9x/ME */
         SetLastNtError( ERROR_INVALID_PARAMETER);
         return FALSE;
      }
      /*
       * Delete the menu item type when changing type from
       * MF_STRING.
       */
      if (MenuItem->fType != lpmii->fType &&
            MENU_ITEM_TYPE(MenuItem->fType) == MFT_STRING)
      {
         FreeMenuText(MenuItem);
         RtlInitUnicodeString(&MenuItem->Text, NULL);
      }
      if(lpmii->fType & MFT_BITMAP)
      {
         if(lpmii->hbmpItem)
           MenuItem->hbmpItem = lpmii->hbmpItem;
         else
         { /* Win 9x/Me stuff */
           MenuItem->hbmpItem = (HBITMAP)((ULONG_PTR)(LOWORD(lpmii->dwTypeData)));
         }
      }
      MenuItem->fType |= lpmii->fType;
   }
   if (lpmii->fMask & MIIM_FTYPE )
   {
      if(( lpmii->fType & MFT_BITMAP))
      {
         DbgPrint("IntSetMenuItemInfo: Can not use FTYPE and MFT_BITMAP.\n");
         SetLastNtError( ERROR_INVALID_PARAMETER);
         return FALSE;
      }
      MenuItem->fType |= lpmii->fType; /* Need to save all the flags, this fixed MFT_RIGHTJUSTIFY */
   }
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
   if(lpmii->fMask & MIIM_ID)
   {
      MenuItem->wID = lpmii->wID;
   }
   if(lpmii->fMask & MIIM_STATE)
   {
      /* remove MFS_DEFAULT flag from all other menu items if this item
         has the MFS_DEFAULT state */
      if(lpmii->fState & MFS_DEFAULT)
         UserSetMenuDefaultItem(MenuObject, -1, 0);
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

   if ((lpmii->fMask & MIIM_STRING) ||
      ((lpmii->fMask & MIIM_TYPE) && (MENU_ITEM_TYPE(lpmii->fType) == MF_STRING)))
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
         if (0 == (MenuObject->MenuInfo.Flags & MF_SYSMENU))
         {
            MenuItem->fType |= MF_SEPARATOR;
         }
         RtlInitUnicodeString(&MenuItem->Text, NULL);
      }
   }

   if (sizeof(ROSMENUITEMINFO) == lpmii->cbSize)
   {
      MenuItem->Rect = lpmii->Rect;
      MenuItem->XTab = lpmii->XTab;
      lpmii->Text = MenuItem->Text.Buffer; /* Send back new allocated string or zero */
   }

   return TRUE;
}

BOOL FASTCALL
IntInsertMenuItem(PMENU_OBJECT MenuObject, UINT uItem, BOOL fByPosition,
                  PROSMENUITEMINFO ItemInfo)
{
   int pos = (int)uItem;
   PMENU_ITEM MenuItem;
   PMENU_OBJECT SubMenu;

   if (MAX_MENU_ITEMS <= MenuObject->MenuInfo.MenuItemCount)
   {
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return FALSE;
   }

   if (fByPosition)
   {
      SubMenu = MenuObject;
      /* calculate position */
      if(MenuObject->MenuInfo.MenuItemCount < pos)
      {
         pos = MenuObject->MenuInfo.MenuItemCount;
      }
   }
   else
   {
      pos = IntGetMenuItemByFlag(MenuObject, uItem, MF_BYCOMMAND, &SubMenu, NULL, NULL);
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

   if(!IntSetMenuItemInfo(SubMenu, MenuItem, ItemInfo))
   {
      ExFreePool(MenuItem);
      return FALSE;
   }

   /* Force size recalculation! */
   MenuObject->MenuInfo.Height = 0;

   pos = IntInsertMenuItemToList(SubMenu, MenuItem, pos);

   DPRINT("IntInsertMenuItemToList = %i\n", pos);

   return (pos >= 0);
}

UINT FASTCALL
IntEnableMenuItem(PMENU_OBJECT MenuObject, UINT uIDEnableItem, UINT uEnable)
{
   PMENU_ITEM MenuItem;
   UINT res = IntGetMenuItemByFlag(MenuObject, uIDEnableItem, uEnable, NULL, &MenuItem, NULL);
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
         mii.Text = CurItem->Text.Buffer;

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

   if((IntGetMenuItemByFlag(MenuObject, uIDCheckItem, uCheck, NULL, &MenuItem, NULL) < 0) || !MenuItem)
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
   BOOL res = IntGetMenuItemByFlag(MenuObject, uItemHilite, uHilite, NULL, &MenuItem, NULL);
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
UserSetMenuDefaultItem(PMENU_OBJECT MenuObject, UINT uItem, UINT fByPos)
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

         if(fByPos)
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

VOID FASTCALL
co_IntInitTracking(PWINDOW_OBJECT Window, PMENU_OBJECT Menu, BOOL Popup,
                   UINT Flags)
{
   /* FIXME - hide caret */

   if(!(Flags & TPM_NONOTIFY))
      co_IntSendMessage(Window->hSelf, WM_SETCURSOR, (WPARAM)Window->hSelf, HTCAPTION);

   /* FIXME - send WM_SETCURSOR message */

   if(!(Flags & TPM_NONOTIFY))
      co_IntSendMessage(Window->hSelf, WM_INITMENU, (WPARAM)Menu->MenuInfo.Self, 0);
}

VOID FASTCALL
co_IntExitTracking(PWINDOW_OBJECT Window, PMENU_OBJECT Menu, BOOL Popup,
                   UINT Flags)
{
   if(!(Flags & TPM_NONOTIFY))
      co_IntSendMessage(Window->hSelf, WM_EXITMENULOOP, 0 /* FIXME */, 0);

   /* FIXME - Show caret again */
}

INT FASTCALL
IntTrackMenu(PMENU_OBJECT Menu, PWINDOW_OBJECT Window, INT x, INT y,
             RECT lprect)
{
   return 0;
}

BOOL FASTCALL
co_IntTrackPopupMenu(PMENU_OBJECT Menu, PWINDOW_OBJECT Window,
                     UINT Flags, POINT *Pos, UINT MenuPos, RECT *ExcludeRect)
{
   co_IntInitTracking(Window, Menu, TRUE, Flags);

   co_IntExitTracking(Window, Menu, TRUE, Flags);
   return FALSE;
}

BOOL FASTCALL
IntSetMenuItemRect(PMENU_OBJECT Menu, UINT Item, BOOL fByPos, RECT *rcRect)
{
   PMENU_ITEM mi;
   if(IntGetMenuItemByFlag(Menu, Item, (fByPos ? MF_BYPOSITION : MF_BYCOMMAND),
                           NULL, &mi, NULL) > -1)
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
IntCleanupMenus(struct _EPROCESS *Process, PW32PROCESS Win32Process)
{
   PEPROCESS CurrentProcess;
   PLIST_ENTRY LastHead = NULL;
   PMENU_OBJECT MenuObject;

   CurrentProcess = PsGetCurrentProcess();
   if (CurrentProcess != Process)
   {
      KeAttachProcess(&Process->Pcb);
   }

   while (Win32Process->MenuListHead.Flink != &(Win32Process->MenuListHead) &&
          Win32Process->MenuListHead.Flink != LastHead)
   {
      LastHead = Win32Process->MenuListHead.Flink;
      MenuObject = CONTAINING_RECORD(Win32Process->MenuListHead.Flink, MENU_OBJECT, ListEntry);

      IntDestroyMenuObject(MenuObject, FALSE, TRUE);
   }

   if (CurrentProcess != Process)
   {
      KeDetachProcess();
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
   PMENU_OBJECT Menu;
   DECLARE_RETURN(DWORD);

   DPRINT("Enter NtUserBuildMenuItemList\n");
   UserEnterExclusive();

   if(!(Menu = UserGetMenuObject(hMenu)))
   {
      RETURN( (DWORD)-1);
   }

   if(Buffer)
   {
      res = IntBuildMenuItemList(Menu, Buffer, nBufSize);
   }
   else
   {
      res = Menu->MenuInfo.MenuItemCount;
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
   HMENU hMenu,
   UINT uIDCheckItem,
   UINT uCheck)
{
   PMENU_OBJECT Menu;
   DECLARE_RETURN(DWORD);

   DPRINT("Enter NtUserCheckMenuItem\n");
   UserEnterExclusive();

   if(!(Menu = UserGetMenuObject(hMenu)))
   {
      RETURN( (DWORD)-1);
   }

   RETURN( IntCheckMenuItem(Menu, uIDCheckItem, uCheck));

CLEANUP:
   DPRINT("Leave NtUserCheckMenuItem, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


HMENU FASTCALL UserCreateMenu(BOOL PopupMenu)
{
   PWINSTATION_OBJECT WinStaObject;
   HANDLE Handle;
   NTSTATUS Status;
   PEPROCESS CurrentProcess = PsGetCurrentProcess();

   if (CsrProcess != CurrentProcess)
   {
      /*
       * CsrProcess does not have a Win32WindowStation
	   *
	   */

      Status = IntValidateWindowStationHandle(PsGetCurrentProcess()->Win32WindowStation,
                     KernelMode,
                     0,
                     &WinStaObject);

       if (!NT_SUCCESS(Status))
       {
          DPRINT1("Validation of window station handle (0x%X) failed\n",
             CurrentProcess->Win32WindowStation);
          SetLastNtError(Status);
          return (HMENU)0;
       }
       IntCreateMenu(&Handle, !PopupMenu);
       ObDereferenceObject(WinStaObject);
   }
   else
   {
       IntCreateMenu(&Handle, !PopupMenu);
   }

   return (HMENU)Handle;
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
   PMENU_OBJECT Menu;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserDeleteMenu\n");
   UserEnterExclusive();

   if(!(Menu = UserGetMenuObject(hMenu)))
   {
      RETURN( FALSE);
   }

   RETURN( IntRemoveMenuItem(Menu, uPosition, uFlags, TRUE));

CLEANUP:
   DPRINT("Leave NtUserDeleteMenu, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}



/*
 * @implemented
 */
BOOL FASTCALL UserDestroyMenu(HMENU hMenu)
{
   PMENU_OBJECT Menu;

   if(!(Menu = UserGetMenuObject(hMenu)))
   {
      return FALSE;
   }

   if(Menu->Process != PsGetCurrentProcess())
   {
      SetLastWin32Error(ERROR_ACCESS_DENIED);
      return FALSE;
   }

   return IntDestroyMenuObject(Menu, FALSE, TRUE);
}

/*
 * @implemented
 */
BOOL STDCALL
NtUserDestroyMenu(
   HMENU hMenu)
{
   PMENU_OBJECT Menu;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserDestroyMenu\n");
   UserEnterExclusive();

   if(!(Menu = UserGetMenuObject(hMenu)))
   {
      RETURN( FALSE);
   }

   if(Menu->Process != PsGetCurrentProcess())
   {
      SetLastWin32Error(ERROR_ACCESS_DENIED);
      RETURN( FALSE);
   }

   RETURN( IntDestroyMenuObject(Menu, FALSE, TRUE));

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
   PMENU_OBJECT Menu;
   DECLARE_RETURN(UINT);

   DPRINT("Enter NtUserEnableMenuItem\n");
   UserEnterExclusive();

   if(!(Menu = UserGetMenuObject(hMenu)))
   {
      RETURN(-1);
   }

   RETURN( IntEnableMenuItem(Menu, uIDEnableItem, uEnable));

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
   PMENU_OBJECT Menu;
   NTSTATUS Status;
   ROSMENUITEMINFO ItemInfo;
   DECLARE_RETURN(DWORD);

   DPRINT("Enter NtUserInsertMenuItem\n");
   UserEnterExclusive();

   if(!(Menu = UserGetMenuObject(hMenu)))
   {
      RETURN( FALSE);
   }

   /* Try to copy the whole MENUITEMINFOW structure */
   Status = MmCopyFromCaller(&ItemInfo, UnsafeItemInfo, sizeof(MENUITEMINFOW));
   if (NT_SUCCESS(Status))
   {
      if (sizeof(MENUITEMINFOW) != ItemInfo.cbSize
         && FIELD_OFFSET(MENUITEMINFOW, hbmpItem) != ItemInfo.cbSize)
      {
         SetLastWin32Error(ERROR_INVALID_PARAMETER);
         RETURN( FALSE);
      }
      RETURN( IntInsertMenuItem(Menu, uItem, fByPosition, &ItemInfo));
   }

   /* Try to copy without last field (not present in older versions) */
   Status = MmCopyFromCaller(&ItemInfo, UnsafeItemInfo, FIELD_OFFSET(MENUITEMINFOW, hbmpItem));
   if (NT_SUCCESS(Status))
   {
      if (FIELD_OFFSET(MENUITEMINFOW, hbmpItem) != ItemInfo.cbSize)
      {
         SetLastWin32Error(ERROR_INVALID_PARAMETER);
         RETURN( FALSE);
      }
      ItemInfo.hbmpItem = (HBITMAP)0;
      RETURN( IntInsertMenuItem(Menu, uItem, fByPosition, &ItemInfo));
   }

   SetLastNtError(Status);
   RETURN( FALSE);

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
   PMENU_OBJECT Menu;
   DWORD gismc = 0;
   DECLARE_RETURN(UINT);

   DPRINT("Enter NtUserGetMenuDefaultItem\n");
   UserEnterExclusive();

   if(!(Menu = UserGetMenuObject(hMenu)))
   {
      RETURN(-1);
   }

   RETURN( IntGetMenuDefaultItem(Menu, fByPos, gmdiFlags, &gismc));

CLEANUP:
   DPRINT("Leave NtUserGetMenuDefaultItem, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @implemented
 */
BOOL STDCALL
NtUserGetMenuBarInfo(
   HWND hwnd,
   LONG idObject,
   LONG idItem,
   PMENUBARINFO pmbi)
{
   BOOL Res = TRUE;
   PMENU_OBJECT MenuObject;
   PMENU_ITEM mi;
   PWINDOW_OBJECT WindowObject;
   HMENU hMenu;
   POINT Offset;
   RECT Rect;
   MENUBARINFO kmbi;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserGetMenuBarInfo\n");
   UserEnterShared();

   if (!(WindowObject = UserGetWindowObject(hwnd)))
     {
        SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
        RETURN(FALSE);
     }

   hMenu = (HMENU)WindowObject->IDMenu;

   if (!(MenuObject = UserGetMenuObject(hMenu)))
     {
       SetLastWin32Error(ERROR_INVALID_MENU_HANDLE);
       RETURN(FALSE);
     }

   if (pmbi->cbSize != sizeof(MENUBARINFO))
     {
       SetLastWin32Error(ERROR_INVALID_PARAMETER);
       RETURN(FALSE);
     }

   kmbi.cbSize = sizeof(MENUBARINFO);
   kmbi.fBarFocused = FALSE;
   kmbi.fFocused = FALSE;
   kmbi.hwndMenu = NULL;

   switch (idObject)
   {
      case OBJID_MENU:
      {
         PMENU_OBJECT SubMenuObject;
         kmbi.hMenu = hMenu;
         if (idItem) /* Non-Zero-Based. */
           {
              if (IntGetMenuItemByFlag(MenuObject, idItem-1, MF_BYPOSITION, NULL, &mi, NULL) > -1)
                   kmbi.rcBar = mi->Rect;
              else
                {
                   Res = FALSE;
                   break;
                }
           }
         else
           {
              /* If items is zero we assume info for the menu itself. */
              if (!(IntGetClientOrigin(WindowObject, &Offset)))
                {
                   Res = FALSE;
                   break;
                }
              Rect.left = Offset.x;
              Rect.right = Offset.x + MenuObject->MenuInfo.Width;
              Rect.top = Offset.y;
              Rect.bottom = Offset.y + MenuObject->MenuInfo.Height;
              kmbi.rcBar = Rect;
              DPRINT("Rect top = %d bottom = %d left = %d right = %d \n",
                       Rect.top, Rect.bottom, Rect.left, Rect.right);
           }
         if (idItem)
           {
              if (idItem-1 == MenuObject->MenuInfo.FocusedItem)
                    kmbi.fFocused = TRUE;
           }
         if (MenuObject->MenuInfo.FocusedItem != NO_SELECTED_ITEM)
               kmbi.fBarFocused = TRUE;
         SubMenuObject = UserGetMenuObject(MenuObject->MenuItemList->hSubMenu);
         if(SubMenuObject) kmbi.hwndMenu = SubMenuObject->MenuInfo.Wnd;
         DPRINT("OBJID_MENU, idItem = %d\n",idItem);
         break;
      }
      case OBJID_CLIENT:
      {
         PMENU_OBJECT SubMenuObject, XSubMenuObject;
         SubMenuObject = UserGetMenuObject(MenuObject->MenuItemList->hSubMenu);
         if(SubMenuObject) kmbi.hMenu = SubMenuObject->MenuInfo.Self;
         else
           {
              Res = FALSE;
              DPRINT1("OBJID_CLIENT, No SubMenu!\n");
              break;
           }
         if (idItem)
           {
              if (IntGetMenuItemByFlag(SubMenuObject, idItem-1, MF_BYPOSITION, NULL, &mi, NULL) > -1)
                   kmbi.rcBar = mi->Rect;
              else
                {
                   Res = FALSE;
                   break;
                }
           }
         else
           {
              PWINDOW_OBJECT SubWinObj;
              if (!(SubWinObj = UserGetWindowObject(SubMenuObject->MenuInfo.Wnd)))
                {
                   Res = FALSE;
                   break;
                }
              if (!(IntGetClientOrigin(SubWinObj, &Offset)))
                {
                   Res = FALSE;
                   break;
                }
              Rect.left = Offset.x;
              Rect.right = Offset.x + SubMenuObject->MenuInfo.Width;
              Rect.top = Offset.y;
              Rect.bottom = Offset.y + SubMenuObject->MenuInfo.Height;
              kmbi.rcBar = Rect;
           }
         if (idItem)
           {
              if (idItem-1 == SubMenuObject->MenuInfo.FocusedItem)
                   kmbi.fFocused = TRUE;
           }
         if (SubMenuObject->MenuInfo.FocusedItem != NO_SELECTED_ITEM)
               kmbi.fBarFocused = TRUE;
         XSubMenuObject = UserGetMenuObject(SubMenuObject->MenuItemList->hSubMenu);
         if (XSubMenuObject) kmbi.hwndMenu = XSubMenuObject->MenuInfo.Wnd;
         DPRINT("OBJID_CLIENT, idItem = %d\n",idItem);
         break;
      }
      case OBJID_SYSMENU:
      {
         PMENU_OBJECT SysMenuObject, SubMenuObject;
         if(!(SysMenuObject = IntGetSystemMenu(WindowObject, FALSE, FALSE)))
         {
           Res = FALSE;
           break;
         }
         kmbi.hMenu = SysMenuObject->MenuInfo.Self;
         if (idItem)
           {
              if (IntGetMenuItemByFlag(SysMenuObject, idItem-1, MF_BYPOSITION, NULL, &mi, NULL) > -1)
                   kmbi.rcBar = mi->Rect;
              else
                {
                   Res = FALSE;
                   break;
                }
           }
         else
           {
              PWINDOW_OBJECT SysWinObj;
              if (!(SysWinObj = UserGetWindowObject(SysMenuObject->MenuInfo.Wnd)))
                {
                   Res = FALSE;
                   break;
                }
              if (!(IntGetClientOrigin(SysWinObj, &Offset)))
                {
                   Res = FALSE;
                   break;
                }
              Rect.left = Offset.x;
              Rect.right = Offset.x + SysMenuObject->MenuInfo.Width;
              Rect.top = Offset.y;
              Rect.bottom = Offset.y + SysMenuObject->MenuInfo.Height;
              kmbi.rcBar = Rect;
           }
         if (idItem)
           {
              if (idItem-1 == SysMenuObject->MenuInfo.FocusedItem)
                    kmbi.fFocused = TRUE;
           }
         if (SysMenuObject->MenuInfo.FocusedItem != NO_SELECTED_ITEM)
               kmbi.fBarFocused = TRUE;
         SubMenuObject = UserGetMenuObject(SysMenuObject->MenuItemList->hSubMenu);
         if(SubMenuObject) kmbi.hwndMenu = SubMenuObject->MenuInfo.Wnd;
         DPRINT("OBJID_SYSMENU, idItem = %d\n",idItem);
         break;
      }
      default:
         Res = FALSE;
         DPRINT1("Unknown idObject = %d, idItem = %d\n",idObject,idItem);
   }
   if (Res)
     {
        NTSTATUS Status = MmCopyToCaller(pmbi, &kmbi, sizeof(MENUBARINFO));
        if (! NT_SUCCESS(Status))
          {
            SetLastNtError(Status);
            RETURN(FALSE);
          }
     }
   RETURN(Res);

CLEANUP:
   DPRINT("Leave NtUserGetMenuBarInfo, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
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
   HWND referenceHwnd;
   RECT Rect;
   NTSTATUS Status;
   PMENU_OBJECT Menu;
   PMENU_ITEM MenuItem;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserGetMenuItemRect\n");
   UserEnterShared();

   if (!(Menu = UserGetMenuObject(hMenu)))
   {
      RETURN(FALSE);
   }

   if (IntGetMenuItemByFlag(Menu, uItem, MF_BYPOSITION, NULL, &MenuItem, NULL) > -1)
        Rect = MenuItem->Rect;
   else
      RETURN(FALSE);

   referenceHwnd = hWnd;

   if(!hWnd)
   {
      if(!UserMenuInfo(Menu, &mi, FALSE))
         RETURN( FALSE);
      if(mi.Wnd == 0)
         RETURN( FALSE);
      referenceHwnd = mi.Wnd; /* Okay we found it, so now what do we do? */
   }

   if (lprcItem == NULL)
      RETURN( FALSE);

   Status = MmCopyToCaller(lprcItem, &Rect, sizeof(RECT));
   if (! NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      RETURN( FALSE);
   }
   RETURN( TRUE);

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
   HWND hWnd,
   HMENU hMenu,
   UINT uItemHilite,
   UINT uHilite)
{
   PMENU_OBJECT Menu;
   PWINDOW_OBJECT Window;
   DECLARE_RETURN(BOOLEAN);

   DPRINT("Enter NtUserHiliteMenuItem\n");
   UserEnterExclusive();

   if(!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN(FALSE);
   }

   if(!(Menu = UserGetMenuObject(hMenu)))
   {
      RETURN(FALSE);
   }

   if(Window->IDMenu == (UINT)hMenu)
   {
      RETURN( IntHiliteMenuItem(Window, Menu, uItemHilite, uHilite));
   }

   RETURN(FALSE);

CLEANUP:
   DPRINT("Leave NtUserHiliteMenuItem, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


static
BOOL FASTCALL
UserMenuInfo(
   PMENU_OBJECT Menu,
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
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
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





/*
 * @implemented
 */
BOOL
STDCALL
NtUserMenuInfo(
   HMENU hMenu,
   PROSMENUINFO UnsafeMenuInfo,
   BOOL SetOrGet)
{
   PMENU_OBJECT Menu;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserMenuInfo\n");
   UserEnterShared();

   if (!(Menu = UserGetMenuObject(hMenu)))
   {
      RETURN(FALSE);
   }

   RETURN(UserMenuInfo(Menu, UnsafeMenuInfo, SetOrGet));

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
   HWND hWnd,
   HMENU hMenu,
   DWORD X,
   DWORD Y)
{
   PMENU_OBJECT Menu;
   PWINDOW_OBJECT Window = NULL;
   PMENU_ITEM mi;
   int i;
   DECLARE_RETURN(int);

   DPRINT("Enter NtUserMenuItemFromPoint\n");
   UserEnterExclusive();

   if (!(Menu = UserGetMenuObject(hMenu)))
   {
      RETURN( -1);
   }

   if (!(Window = UserGetWindowObject(Menu->MenuInfo.Wnd)))
   {
      RETURN( -1);
   }

   X -= Window->Wnd->WindowRect.left;
   Y -= Window->Wnd->WindowRect.top;

   mi = Menu->MenuItemList;
   for (i = 0; NULL != mi; i++)
   {
      if (InRect(mi->Rect, X, Y))
      {
         break;
      }
      mi = mi->Next;
   }

   RETURN( (mi ? i : NO_SELECTED_ITEM));

CLEANUP:
   DPRINT("Leave NtUserMenuItemFromPoint, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}



static
BOOL FASTCALL
UserMenuItemInfo(
   PMENU_OBJECT Menu,
   UINT Item,
   BOOL ByPosition,
   PROSMENUITEMINFO UnsafeItemInfo,
   BOOL SetOrGet)
{
   PMENU_ITEM MenuItem;
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
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
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
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return( FALSE);
   }

   if (IntGetMenuItemByFlag(Menu, Item,
                            (ByPosition ? MF_BYPOSITION : MF_BYCOMMAND),
                            NULL, &MenuItem, NULL) < 0)
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return( FALSE);
   }

   if (SetOrGet)
   {
      Ret = IntSetMenuItemInfo(Menu, MenuItem, &ItemInfo);
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



/*
 * @implemented
 */
BOOL
STDCALL
NtUserMenuItemInfo(
   HMENU hMenu,
   UINT Item,
   BOOL ByPosition,
   PROSMENUITEMINFO UnsafeItemInfo,
   BOOL SetOrGet)
{
   PMENU_OBJECT Menu;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserMenuItemInfo\n");
   UserEnterExclusive();

   if (!(Menu = UserGetMenuObject(hMenu)))
   {
      RETURN(FALSE);
   }

   RETURN( UserMenuItemInfo(Menu, Item, ByPosition, UnsafeItemInfo, SetOrGet));

CLEANUP:
   DPRINT("Leave NtUserMenuItemInfo, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;

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
   PMENU_OBJECT Menu;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserRemoveMenu\n");
   UserEnterExclusive();

   if(!(Menu = UserGetMenuObject(hMenu)))
   {
      RETURN( FALSE);
   }

   RETURN(IntRemoveMenuItem(Menu, uPosition, uFlags, FALSE));

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
   HMENU hMenu,
   DWORD dwContextHelpId)
{
   PMENU_OBJECT Menu;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserSetMenuContextHelpId\n");
   UserEnterExclusive();

   if(!(Menu = UserGetMenuObject(hMenu)))
   {
      RETURN( FALSE);
   }

   RETURN(IntSetMenuContextHelpId(Menu, dwContextHelpId));

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
   PMENU_OBJECT Menu;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserSetMenuDefaultItem\n");
   UserEnterExclusive();

   if(!(Menu = UserGetMenuObject(hMenu)))
   {
      RETURN( FALSE);
   }

   RETURN( UserSetMenuDefaultItem(Menu, uItem, fByPos));

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
   PMENU_OBJECT Menu;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserSetMenuFlagRtoL\n");
   UserEnterExclusive();

   if(!(Menu = UserGetMenuObject(hMenu)))
   {
      RETURN( FALSE);
   }

   RETURN(IntSetMenuFlagRtoL(Menu));

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
/* NOTE: unused function */
BOOL STDCALL
NtUserTrackPopupMenuEx(
   HMENU hMenu,
   UINT fuFlags,
   int x,
   int y,
   HWND hWnd,
   LPTPMPARAMS lptpm)
{
   UNIMPLEMENTED

   return FALSE;
}


/* EOF */
