/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 2003 ReactOS Team
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
 * PURPOSE:          HotKey support
 * FILE:             subsys/win32k/ntuser/hotkey.c
 * PROGRAMER:        Eric Kohl
 * REVISION HISTORY:
 *       02-11-2003  EK  Created
 */



/*

FIXME: Hotkey notifications are triggered by keyboard input (physical or programatically)
and since only desktops on WinSta0 can recieve input in seems very wrong to allow
windows/threads on destops not belonging to WinSta0 to set hotkeys (recieve notifications).

-Gunnar
*/


/* INCLUDES ******************************************************************/

#include <w32k.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

LIST_ENTRY gHotkeyList;

/* FUNCTIONS *****************************************************************/

NTSTATUS FASTCALL
InitHotkeyImpl()
{
   InitializeListHead(&gHotkeyList);

   return STATUS_SUCCESS;
}


#if 0 //not used
NTSTATUS FASTCALL
CleanupHotKeys()
{

   return STATUS_SUCCESS;
}
#endif


BOOL FASTCALL
GetHotKey (UINT fsModifiers,
           UINT vk,
           struct _ETHREAD **Thread,
           HWND *hWnd,
           int *id)
{
   PHOT_KEY_ITEM HotKeyItem;

   LIST_FOR_EACH(HotKeyItem, &gHotkeyList, HOT_KEY_ITEM, ListEntry)
   {
      if (HotKeyItem->fsModifiers == fsModifiers &&
            HotKeyItem->vk == vk)
      {
         if (Thread != NULL)
            *Thread = HotKeyItem->Thread;

         if (hWnd != NULL)
            *hWnd = HotKeyItem->hWnd;

         if (id != NULL)
            *id = HotKeyItem->id;

         return TRUE;
      }
   }

   return FALSE;
}


VOID FASTCALL
UnregisterWindowHotKeys(PWINDOW_OBJECT Window)
{
   PHOT_KEY_ITEM HotKeyItem, tmp;
   
   LIST_FOR_EACH_SAFE(HotKeyItem, tmp, &gHotkeyList, HOT_KEY_ITEM, ListEntry)
   {
      if (HotKeyItem->hWnd == Window->hSelf)
      {
         RemoveEntryList (&HotKeyItem->ListEntry);
         ExFreePool (HotKeyItem);
      }
   }

}


VOID FASTCALL
UnregisterThreadHotKeys(struct _ETHREAD *Thread)
{
   PHOT_KEY_ITEM HotKeyItem, tmp;

   LIST_FOR_EACH_SAFE(HotKeyItem, tmp, &gHotkeyList, HOT_KEY_ITEM, ListEntry)   
   {
      if (HotKeyItem->Thread == Thread)
      {
         RemoveEntryList (&HotKeyItem->ListEntry);
         ExFreePool (HotKeyItem);
      }
   }

}


static 
BOOL FASTCALL
IsHotKey (UINT fsModifiers, UINT vk)
{
   PHOT_KEY_ITEM HotKeyItem;

   LIST_FOR_EACH(HotKeyItem, &gHotkeyList, HOT_KEY_ITEM, ListEntry)
   {
      if (HotKeyItem->fsModifiers == fsModifiers && HotKeyItem->vk == vk)
      {
         return TRUE;
      }
   }

   return FALSE;
}



/* SYSCALLS *****************************************************************/


BOOL STDCALL
NtUserRegisterHotKey(HWND hWnd,
                     int id,
                     UINT fsModifiers,
                     UINT vk)
{
   PHOT_KEY_ITEM HotKeyItem;
   PWINDOW_OBJECT Window;
   PETHREAD HotKeyThread;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserRegisterHotKey\n");
   UserEnterExclusive();

   if (hWnd == NULL)
   {
      HotKeyThread = PsGetCurrentThread();
   }
   else
   {
      if(!(Window = UserGetWindowObject(hWnd)))
      {
         RETURN( FALSE);
      }
      HotKeyThread = Window->OwnerThread;
   }

   /* Check for existing hotkey */
   if (IsHotKey (fsModifiers, vk))
   {
      RETURN( FALSE);
   }

   HotKeyItem = ExAllocatePoolWithTag (PagedPool, sizeof(HOT_KEY_ITEM), TAG_HOTKEY);
   if (HotKeyItem == NULL)
   {
      RETURN( FALSE);
   }

   HotKeyItem->Thread = HotKeyThread;
   HotKeyItem->hWnd = hWnd;
   HotKeyItem->id = id;
   HotKeyItem->fsModifiers = fsModifiers;
   HotKeyItem->vk = vk;

   InsertHeadList (&gHotkeyList, &HotKeyItem->ListEntry);

   RETURN( TRUE);

CLEANUP:
   DPRINT("Leave NtUserRegisterHotKey, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


BOOL STDCALL
NtUserUnregisterHotKey(HWND hWnd, int id)
{
   PHOT_KEY_ITEM HotKeyItem;
   PWINDOW_OBJECT Window;
   DECLARE_RETURN(BOOL);

   DPRINT("Enter NtUserUnregisterHotKey\n");
   UserEnterExclusive();

   if(!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN( FALSE);
   }

   LIST_FOR_EACH(HotKeyItem, &gHotkeyList, HOT_KEY_ITEM, ListEntry)
   {
      if (HotKeyItem->hWnd == hWnd && HotKeyItem->id == id)
      {
         RemoveEntryList (&HotKeyItem->ListEntry);
         ExFreePool (HotKeyItem);

         RETURN( TRUE);
      }
   }

   RETURN( FALSE);

CLEANUP:
   DPRINT("Leave NtUserUnregisterHotKey, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/* EOF */
