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
/* $Id: menu.c,v 1.6 2003/08/04 10:13:51 weiden Exp $
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

#include <ddk/ntddk.h>
#include <win32k/win32k.h>
#include <napi/win32.h>
#include <include/menu.h>
#include <include/error.h>
#include <include/winsta.h>
#include <include/object.h>
#include <include/guicheck.h>
#include <include/window.h>
#include <include/color.h>

#define NDEBUG
#include <debug.h>

/* INTERNAL ******************************************************************/

/* maximum number of menu items a menu can contain */
#define MAX_MENU_ITEMS (0x4000)

#ifndef MIIM_STRING
#define MIIM_STRING      (0x00000040)
#endif
#ifndef MIIM_BITMAP
#define MIIM_BITMAP      (0x00000080)
#endif
#ifndef MIIM_FTYPE
#define MIIM_FTYPE       (0x00000100)
#endif

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

PMENU_OBJECT FASTCALL
W32kGetMenuObject(HMENU hMenu)
{
  PMENU_OBJECT MenuObject;
  NTSTATUS Status = ObmReferenceObjectByHandle(PsGetWin32Process()->
                      WindowStation->HandleTable, hMenu, otMenu, 
                      (PVOID*)&MenuObject);
  if (!NT_SUCCESS(Status))
    {
      return(NULL);
    }
  return MenuObject;
}

VOID FASTCALL
W32kReleaseMenuObject(PMENU_OBJECT MenuObject)
{
  ObmDereferenceObject(MenuObject);
}

BOOL FASTCALL
W32kDestroyMenuObject(PMENU_OBJECT MenuObject)
{
  if(MenuObject)
  {  
    /* remove all menu items */
    ExAcquireFastMutexUnsafe (&MenuObject->MenuItemsLock);
    //RemoveEntryList(&MenuObject->MenuItemsHead);
    // FIXME Delete entries
    ExReleaseFastMutexUnsafe (&MenuObject->MenuItemsLock);
    
    W32kReleaseMenuObject(MenuObject);
    
    ObmCloseHandle(PsGetWin32Process()->WindowStation->HandleTable, MenuObject->Self);
  
    return TRUE;
  }
  return FALSE;
}

PMENU_OBJECT FASTCALL
W32kCreateMenu(PHANDLE Handle)
{
  PMENU_OBJECT MenuObject = (PMENU_OBJECT)ObmCreateObject(
      PsGetWin32Process()->WindowStation->HandleTable, Handle, 
      otMenu, sizeof(MENU_OBJECT));
  
  if(!MenuObject)
  {
    *Handle = 0;
    return NULL;
  }

  MenuObject->Self = *Handle;
  MenuObject->RtoL = FALSE; /* default */
  MenuObject->dwStyle = 0; /* FIXME */
  MenuObject->cyMax = 0; /* default */
  MenuObject->hbrBack = W32kGetSysColorBrush(COLOR_MENU); /*default background color */
  MenuObject->dwContextHelpID = 0; /* default */  
  MenuObject->dwMenuData = 0; /* default */
  
  MenuObject->MenuItemCount = 0;
  //InitializeListHead(&MenuObject->MenuItemsHead);
  MenuObject->MenuItemList = NULL;
  ExInitializeFastMutex(&MenuObject->MenuItemsLock);
  
  return MenuObject;
}

BOOL FASTCALL
W32kSetMenuFlagRtoL(PMENU_OBJECT MenuObject)
{
  if(MenuObject)
  {
    MenuObject->RtoL = TRUE;
    return TRUE;
  }
  return FALSE;
}

BOOL FASTCALL
W32kSetMenuContextHelpId(PMENU_OBJECT MenuObject, DWORD dwContextHelpId)
{
  if(MenuObject)
  {
    MenuObject->dwContextHelpID = dwContextHelpId;
    return TRUE;
  }
  return FALSE;
}

BOOL FASTCALL
W32kGetMenuInfo(PMENU_OBJECT MenuObject, LPMENUINFO lpmi)
{
  if(MenuObject)
  {
    if(lpmi->fMask & MIM_BACKGROUND)
      lpmi->hbrBack = MenuObject->hbrBack;
    if(lpmi->fMask & MIM_HELPID)
      lpmi->dwContextHelpID = MenuObject->dwContextHelpID;
    if(lpmi->fMask & MIM_MAXHEIGHT)
      lpmi->cyMax = MenuObject->cyMax;
    if(lpmi->fMask & MIM_MENUDATA)
      lpmi->dwMenuData = MenuObject->dwMenuData;
    if(lpmi->fMask & MIM_STYLE)
      lpmi->dwStyle = MenuObject->dwStyle;
    return TRUE;
  }
  return FALSE;
}

BOOL FASTCALL
W32kSetMenuInfo(PMENU_OBJECT MenuObject, LPMENUINFO lpmi)
{
  if(MenuObject)
  {
    if(lpmi->fMask & MIM_BACKGROUND)
      MenuObject->hbrBack = lpmi->hbrBack;
    if(lpmi->fMask & MIM_HELPID)
      MenuObject->dwContextHelpID = lpmi->dwContextHelpID;
    if(lpmi->fMask & MIM_MAXHEIGHT)
      MenuObject->cyMax = lpmi->cyMax;
    if(lpmi->fMask & MIM_MENUDATA)
      lpmi->dwMenuData = MenuObject->dwMenuData;
    if(lpmi->fMask & MIM_STYLE)
      lpmi->dwStyle = MenuObject->dwStyle;
    if(lpmi->fMask & MIM_APPLYTOSUBMENUS)
    {
      /* FIXME */
    }
    return TRUE;
  }
  return FALSE;
}

int FASTCALL
W32kGetMenuItemById(PMENU_OBJECT MenuObject, UINT wID, PMENU_ITEM *MenuItem)
{
  PMENU_ITEM CurItem;
  UINT p = 0;

  CurItem = MenuObject->MenuItemList;
  while(CurItem)
  {
    if(CurItem->MenuItem.wID == wID)
    {
      if(MenuItem) *MenuItem = CurItem;
      return p;
    }
    CurItem = CurItem->Next;
    p++;
  }

  return -1;
}

int FASTCALL
W32kGetMenuItemByIndex(PMENU_OBJECT MenuObject, UINT Index, PMENU_ITEM *MenuItem)
{
  PMENU_ITEM PrevItem = NULL;
  PMENU_ITEM CurItem;
  UINT p = Index;

  CurItem = MenuObject->MenuItemList;
  while(CurItem && (p > 0))
  {
    PrevItem = CurItem;
    CurItem = CurItem->Next;
    p--;
  }
  if(MenuItem)
  {
    if(!CurItem)
    {
      *MenuItem = CurItem;
    }
    else
    {
      *MenuItem = PrevItem;
    }
  }
  
  return Index - p;
}

int FASTCALL
W32kInsertMenuItemToList(PMENU_OBJECT MenuObject, PMENU_ITEM MenuItem, int pos)
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
      /* insert at the end */
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
  MenuObject->MenuItemCount++;
  
  return npos;
}

#if 0
void FASTCALL
DumpMenuItemList(PMENU_ITEM MenuItem)
{
  UINT cnt = 0;
  while(MenuItem)
  {
    if(MenuItem->MenuItem.dwTypeData)
      DbgPrint(" %d. %ws\n", ++cnt, (LPWSTR)MenuItem->MenuItem.dwTypeData);
    else
      DbgPrint(" %d. NO TEXT dwTypeData==%d\n", ++cnt, MenuItem->MenuItem.dwTypeData);
    DbgPrint("   fType=");
    if(MFT_BITMAP & MenuItem->MenuItem.fType) DbgPrint("MFT_BITMAP ");
    if(MFT_MENUBARBREAK & MenuItem->MenuItem.fType) DbgPrint("MFT_MENUBARBREAK ");
    if(MFT_MENUBREAK & MenuItem->MenuItem.fType) DbgPrint("MFT_MENUBREAK ");
    if(MFT_OWNERDRAW & MenuItem->MenuItem.fType) DbgPrint("MFT_OWNERDRAW ");
    if(MFT_RADIOCHECK & MenuItem->MenuItem.fType) DbgPrint("MFT_RADIOCHECK ");
    if(MFT_RIGHTJUSTIFY & MenuItem->MenuItem.fType) DbgPrint("MFT_RIGHTJUSTIFY ");
    if(MFT_SEPARATOR & MenuItem->MenuItem.fType) DbgPrint("MFT_SEPARATOR ");
    if(MFT_STRING & MenuItem->MenuItem.fType) DbgPrint("MFT_STRING ");
    DbgPrint("\n   fState=");
    if(MFS_CHECKED & MenuItem->MenuItem.fState) DbgPrint("MFS_CHECKED ");
    if(MFS_DEFAULT & MenuItem->MenuItem.fState) DbgPrint("MFS_DEFAULT ");
    if(MFS_DISABLED & MenuItem->MenuItem.fState) DbgPrint("MFS_DISABLED ");
    if(MFS_ENABLED & MenuItem->MenuItem.fState) DbgPrint("MFS_ENABLED ");
    if(MFS_GRAYED & MenuItem->MenuItem.fState) DbgPrint("MFS_GRAYED ");
    if(MFS_HILITE & MenuItem->MenuItem.fState) DbgPrint("MFS_HILITE ");
    if(MFS_UNCHECKED & MenuItem->MenuItem.fState) DbgPrint("MFS_UNCHECKED ");
    if(MFS_UNHILITE & MenuItem->MenuItem.fState) DbgPrint("MFS_UNHILITE ");
    DbgPrint("\n   wId=%d\n", MenuItem->MenuItem.wID);
    MenuItem = MenuItem->Next;
  }
  DbgPrint("Entries: %d\n", cnt);
  return;
}
#endif

/* TODO optimize this macro */
#define UpdateMenuItemState(state, change) \
{\
  if((change) & MFS_DISABLED) { \
    if(!((state) & MFS_DISABLED)) (state) |= MFS_DISABLED; \
  } else { \
    if((state) & MFS_DISABLED) (state) ^= MFS_DISABLED; \
  } \
  if((change) & MFS_CHECKED) { \
    if(!((state) & MFS_CHECKED)) (state) |= MFS_CHECKED; \
  } else { \
    if((state) & MFS_CHECKED) (state) ^= MFS_CHECKED; \
  } \
  if((change) & MFS_HILITE) { \
    if(!((state) & MFS_HILITE)) (state) |= MFS_HILITE; \
  } else { \
    if((state) & MFS_HILITE) (state) ^= MFS_HILITE; \
  } \
}

BOOL FASTCALL
W32kSetMenuItemInfo(PMENU_OBJECT MenuObject, PMENU_ITEM MenuItem, LPMENUITEMINFOW lpmii)
{
  if(!MenuItem || !MenuObject || !lpmii)
  {
    return FALSE;
  }

  if((MenuItem->MenuItem.fMask & (MIIM_TYPE | MIIM_STRING)) && 
           (MENU_ITEM_TYPE(MenuItem->MenuItem.fType) == MF_STRING) && 
           MenuItem->MenuItem.dwTypeData)
  {
    /* delete old string */
    ExFreePool(MenuItem->MenuItem.dwTypeData);
    MenuItem->MenuItem.dwTypeData = 0;
  }
  
  MenuItem->MenuItem.fType = lpmii->fType;
  MenuItem->MenuItem.cch = lpmii->cch;
  
  if(lpmii->fMask & MIIM_BITMAP)
  {
    MenuItem->MenuItem.hbmpItem = lpmii->hbmpItem;
  }
  if(lpmii->fMask & MIIM_CHECKMARKS)
  {
    MenuItem->MenuItem.hbmpChecked = lpmii->hbmpChecked;
    MenuItem->MenuItem.hbmpUnchecked = lpmii->hbmpUnchecked;
  }
  if(lpmii->fMask & MIIM_DATA)
  {
    MenuItem->MenuItem.dwItemData = lpmii->dwItemData;
  }
  if(lpmii->fMask & (MIIM_FTYPE | MIIM_TYPE))
  {
    MenuItem->MenuItem.fType = lpmii->fType;
  }
  if(lpmii->fMask & MIIM_ID)
  {
    MenuItem->MenuItem.wID = lpmii->wID;
  }
  if(lpmii->fMask & MIIM_STATE)
  {
    UpdateMenuItemState(MenuItem->MenuItem.fState, lpmii->fState);
    /* FIXME - only one item can have MFS_DEFAULT */
  }
  
  if(lpmii->fMask & MIIM_SUBMENU)
  {
    MenuItem->MenuItem.hSubMenu = lpmii->hSubMenu;
  }
  if((lpmii->fMask & (MIIM_TYPE | MIIM_STRING)) && 
           (MENU_ITEM_TYPE(lpmii->fType) == MF_STRING) && lpmii->dwTypeData)
  {
    MenuItem->MenuItem.dwTypeData = (LPWSTR)ExAllocatePool(PagedPool, (lpmii->cch + 1) * sizeof(WCHAR));
    if(!MenuItem->MenuItem.dwTypeData)
    {
      /* FIXME Set last error code? */
      SetLastWin32Error(STATUS_NO_MEMORY);
      return FALSE;
    }
    memcpy(MenuItem->MenuItem.dwTypeData, lpmii->dwTypeData, (lpmii->cch + 1) * sizeof(WCHAR));
  }
  
  return TRUE;
}

BOOL FASTCALL
W32kInsertMenuItem(PMENU_OBJECT MenuObject, UINT uItem, WINBOOL fByPosition,
                   LPMENUITEMINFOW lpmii)
{
  int pos = (int)uItem;
  PMENU_ITEM MenuItem;
  
  if(MenuObject->MenuItemCount >= MAX_MENU_ITEMS)
  {
    /* FIXME Set last error code? */
    SetLastWin32Error(STATUS_NO_MEMORY);
    return FALSE;
  }
  
  if(fByPosition)
  {
    /* calculate position */
    if(pos > MenuObject->MenuItemCount)
      pos = MenuObject->MenuItemCount;
  }
  else
  {
    pos = W32kGetMenuItemById(MenuObject, uItem, NULL);
  }
  if(pos < -1) pos = -1;
  
  MenuItem = ExAllocatePool(PagedPool, sizeof(MENU_ITEM));
  if(!MenuItem)
  {
    /* FIXME Set last error code? */
    SetLastWin32Error(STATUS_NO_MEMORY);
    return FALSE;
  }
  
  /* memcpy(&MenuItem->MenuItem, lpmii, sizeof(MENUITEMINFOW)); */
  RtlZeroMemory(&MenuItem->MenuItem, sizeof(MENUITEMINFOW));
  MenuItem->MenuItem.cbSize = sizeof(MENUITEMINFOW);
  
  if(!(lpmii->fMask & MIIM_STATE))
  {
    lpmii->fMask |= MIIM_STATE;
    lpmii->fState = MFS_ENABLED | MFS_CHECKED;
    UpdateMenuItemState(MenuItem->MenuItem.fState, lpmii->fState);
  }
  
  if(!W32kSetMenuItemInfo(MenuObject, MenuItem, lpmii))
  {
    ExFreePool(MenuObject);
    return FALSE;
  }
  
  pos = W32kInsertMenuItemToList(MenuObject, MenuItem, pos);
  
  #if 0
  DumpMenuItemList(MenuObject->MenuItemList);
  #endif
  
  return pos >= 0;
}

BOOL FASTCALL
W32kEnableMenuItem(PMENU_OBJECT MenuObject, UINT uIDEnableItem, UINT uEnable)
{
  PMENU_ITEM MenuItem;
  UINT oldflags;
  if(uEnable & MF_BYCOMMAND)
  {
    if(!W32kGetMenuItemById(MenuObject, uIDEnableItem, &MenuItem))
    {
      return FALSE;
    }
    if(!MenuItem) return FALSE;
  }
  else
  {
    if(!(uEnable & MF_BYPOSITION))
    {
      return FALSE;
    }
    if(!W32kGetMenuItemByIndex(MenuObject, uIDEnableItem, &MenuItem))
    {
      return FALSE;
      if(!MenuItem) return FALSE;
    }
  }
  
  /* FIXME from wine, hope  that works */
  oldflags = MenuItem->MenuItem.fState & (MF_GRAYED | MF_DISABLED);
  MenuItem->MenuItem.fState ^= (oldflags ^ uEnable) & (MF_GRAYED | MF_DISABLED);
  #if 0
  DumpMenuItemList(MenuObject->MenuItemList);
  #endif
  return TRUE;
}

/*
DWORD FASTCALL
W32kBuildMenuItemList(PMENU_OBJECT MenuObject, MENUITEMINFOW *lpmiil, ULONG nMax)
{
  DWORD Index = 0;
  PMENU_ITEM CurItem = MenuObject->MenuItemList;
  while(CurItem && (nMax > 0))
  {
    memcpy(&lpmiil[Index], &CurItem->MenuItem, sizeof(MENUITEMINFOW));
    CurItem = CurItem->Next;
    Index++;
    nMax--;
  }
  return Index;
}
*/

/* FUNCTIONS *****************************************************************/


/*
 * @implemented
 */
DWORD
STDCALL
NtUserBuildMenuItemList(
 HMENU hMenu,
 LPCMENUITEMINFO* lpmiil,
 ULONG nBufSize,
 DWORD Reserved)
{
  DWORD res = 0;
  PMENU_OBJECT MenuObject = W32kGetMenuObject(hMenu);
  if(!MenuObject)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }
  
  if(lpmiil)
  {
    /* FIXME need to pass the menu strings to user32 somehow....
    
    ExAcquireFastMutexUnsafe(&MenuObject->MenuItemsLock);
    res = W32kBuildMenuItemList(MenuObject, lpmiil, nBufSize / sizeof(LPCMENUITEMINFO));
    ExReleaseFastMutexUnsafe(&MenuObject->MenuItemsLock);
    */
  }
  else
  {
  #if 0
  DumpMenuItemList(MenuObject->MenuItemList);
  #endif
    res = MenuObject->MenuItemCount;
  }
  W32kReleaseMenuObject(MenuObject);
  return res;
}


/*
 * @unimplemented
 */
DWORD STDCALL
NtUserCheckMenuItem(
  HMENU hmenu,
  UINT uIDCheckItem,
  UINT uCheck)
{
  UNIMPLEMENTED

  return 0;
}


/*
 * @implemented
 */
HMENU STDCALL
NtUserCreateMenu(VOID)
{
  PWINSTATION_OBJECT WinStaObject;
  HANDLE Handle;
  
  NTSTATUS Status = ValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
				       KernelMode,
				       0,
				       &WinStaObject);
				       
  if (!NT_SUCCESS(Status))
  {
    DPRINT("Validation of window station handle (0x%X) failed\n",
      PROCESS_WINDOW_STATION());
    SetLastWin32Error(Status);
    return (HMENU)0;
  }

  W32kCreateMenu(&Handle);
  ObDereferenceObject(WinStaObject);
  return (HMENU)Handle;
}


/*
 * @unimplemented
 */
BOOL STDCALL
NtUserDeleteMenu(
  HMENU hMenu,
  UINT uPosition,
  UINT uFlags)
{

  UNIMPLEMENTED

  return 0;
}


/*
 * @implemented
 */
BOOL STDCALL
NtUserDestroyMenu(
  HMENU hMenu)
{
  /* FIXME, check if menu belongs to the process */
  
  PMENU_OBJECT MenuObject = W32kGetMenuObject(hMenu);
  if(!MenuObject)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }

  return W32kDestroyMenuObject(MenuObject);
}


/*
 * @implemented
 */
BOOL STDCALL
NtUserEnableMenuItem(
  HMENU hMenu,
  UINT uIDEnableItem,
  UINT uEnable)
{
  BOOL res = FALSE;
  PMENU_OBJECT MenuObject;
  MenuObject = W32kGetMenuObject(hMenu);
  if(!MenuObject)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return 0;
  }
  ExAcquireFastMutexUnsafe(&MenuObject->MenuItemsLock);
  res = W32kEnableMenuItem(MenuObject, uIDEnableItem, uEnable);
  ExReleaseFastMutexUnsafe(&MenuObject->MenuItemsLock);
  W32kReleaseMenuObject(MenuObject);
  return res;
}


/*
 * @implemented
 */
DWORD STDCALL
NtUserInsertMenuItem(
  HMENU hMenu,
  UINT uItem,
  WINBOOL fByPosition,
  LPMENUITEMINFOW lpmii)
{
  DWORD res = 0;
  PMENU_OBJECT MenuObject;
  MenuObject = W32kGetMenuObject(hMenu);
  if(!MenuObject)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return 0;
  }
  ExAcquireFastMutexUnsafe(&MenuObject->MenuItemsLock);
  res = W32kInsertMenuItem(MenuObject, uItem, fByPosition, lpmii);
  ExReleaseFastMutexUnsafe(&MenuObject->MenuItemsLock);
  W32kReleaseMenuObject(MenuObject);
  return res;
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
DWORD STDCALL
NtUserGetMenuIndex(
  DWORD Unknown0,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}


/*
 * @unimplemented
 */
BOOL STDCALL
NtUserGetMenuItemRect(
  HWND hWnd,
  HMENU hMenu,
  UINT uItem,
  LPRECT lprcItem)
{
  UNIMPLEMENTED

  return 0;
}


/*
 * @unimplemented
 */
BOOL STDCALL
NtUserHiliteMenuItem(
  HWND hwnd,
  HMENU hmenu,
  UINT uItemHilite,
  UINT uHilite)
{
  UNIMPLEMENTED

  return 0;
}


/*
 * @implemented
 */
BOOL
STDCALL
NtUserMenuInfo(
 HMENU hmenu,
 LPMENUINFO lpmi,
 BOOL fsog)
{
  BOOL res = FALSE;
  PMENU_OBJECT MenuObject;
  
  if(lpmi->cbSize != sizeof(MENUINFO))
  {
    /* FIXME - Set Last Error */
    return FALSE;
  }
  MenuObject = W32kGetMenuObject(hmenu);
  if(!MenuObject)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }  
  if(fsog)
  {
    /* Set MenuInfo */
    res = W32kSetMenuInfo(MenuObject, lpmi);  
  }
  else
  {
    /* Get MenuInfo */
    res = W32kGetMenuInfo(MenuObject, lpmi);
  }  
  W32kReleaseMenuObject(MenuObject);
  return res;
}


/*
 * @unimplemented
 */
int STDCALL
NtUserMenuItemFromPoint(
  HWND hWnd,
  HMENU hMenu,
  DWORD X,
  DWORD Y)
{
  UNIMPLEMENTED

  return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
NtUserMenuItemInfo(
 HMENU hMenu,
 UINT uItem,
 BOOL fByPosition,
 LPMENUITEMINFOW lpmii,
 BOOL fsog)
{
  UNIMPLEMENTED

  return 0;
}


/*
 * @unimplemented
 */
BOOL STDCALL
NtUserRemoveMenu(
  HMENU hMenu,
  UINT uPosition,
  UINT uFlags)
{
  UNIMPLEMENTED

  return 0;
}


/*
 * @implemented
 */
BOOL STDCALL
NtUserSetMenuContextHelpId(
  HMENU hmenu,
  DWORD dwContextHelpId)
{
  BOOL res;
  PMENU_OBJECT MenuObject = W32kGetMenuObject(hmenu);
  if(!MenuObject)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }

  res = W32kSetMenuContextHelpId(MenuObject, dwContextHelpId);
  W32kReleaseMenuObject(MenuObject);
  return res;
}


/*
 * @unimplemented
 */
BOOL STDCALL
NtUserSetMenuDefaultItem(
  HMENU hMenu,
  UINT uItem,
  UINT fByPos)
{
  UNIMPLEMENTED

  return 0;
}


/*
 * @implemented
 */
BOOL STDCALL
NtUserSetMenuFlagRtoL(
  HMENU hMenu)
{
  BOOL res;
  PMENU_OBJECT MenuObject = W32kGetMenuObject(hMenu);
  if(!MenuObject)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }

  res = W32kSetMenuFlagRtoL(MenuObject);
  W32kReleaseMenuObject(MenuObject);
  return res;
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
 * @unimplemented
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
  UNIMPLEMENTED

  return 0;
}


/* EOF */
