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
/* $Id: menu.c,v 1.3 2003/08/01 11:56:19 weiden Exp $
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
    RemoveEntryList(&MenuObject->MenuItemsHead);
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
  
  InitializeListHead(&MenuObject->MenuItemsHead);
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


/* FUNCTIONS *****************************************************************/


/*
 * @unimplemented
 */
DWORD
STDCALL
NtUserBuildMenuItemList(
 HMENU hMenu,
 LPCMENUITEMINFO* lpmiil,
 ULONG nBufSize,
 DWORD Reserved)
{
  UNIMPLEMENTED

  return 0;
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
    ObDereferenceObject(WinStaObject);
    DPRINT("Validation of window station handle (0x%X) failed\n",
      PROCESS_WINDOW_STATION());
    SetLastNtError(Status);
    return (HMENU)0;
  }

  W32kCreateMenu(&Handle);
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
 * @unimplemented
 */
BOOL STDCALL
NtUserEnableMenuItem(
  HMENU hMenu,
  UINT uIDEnableItem,
  UINT uEnable)
{
  UNIMPLEMENTED

  return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
NtUserInsertMenuItem(
  HMENU hMenu,
  UINT uItem,
  WINBOOL fByPosition,
  LPCMENUITEMINFO lpmii)
{
  UNIMPLEMENTED
  
  return 0;
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
 * @unimplemented
 */
BOOL
STDCALL
NtUserMenuInfo(
 HMENU hmenu,
 LPCMENUINFO lpcmi,
 BOOL fsog)
{

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
 LPMENUITEMINFO lpmii,
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
  PMENU_OBJECT MenuObject = W32kGetMenuObject(hmenu);
  if(!MenuObject)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }

  BOOL res = W32kSetMenuContextHelpId(MenuObject, dwContextHelpId);
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
  PMENU_OBJECT MenuObject = W32kGetMenuObject(hMenu);
  if(!MenuObject)
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
    return FALSE;
  }

  BOOL res = W32kSetMenuFlagRtoL(MenuObject);
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
  LPMENUITEMINFO lpmii,
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
