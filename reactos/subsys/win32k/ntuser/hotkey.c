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
/* $Id: hotkey.c,v 1.2 2003/11/03 18:52:21 ekohl Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          HotKey support
 * FILE:             subsys/win32k/ntuser/hotkey.c
 * PROGRAMER:        Eric Kohl
 * REVISION HISTORY:
 *       02-11-2003  EK  Created
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <win32k/win32k.h>
#include <win32k/userobj.h>
#include <include/msgqueue.h>

#define NDEBUG
#include <debug.h>


/* GLOBALS *******************************************************************/

typedef struct _HOT_KEY_ITEM
{
  LIST_ENTRY ListEntry;
  struct _ETHREAD *Thread;
  HWND hWnd;
  int id;
  UINT fsModifiers;
  UINT vk;
} HOT_KEY_ITEM, *PHOT_KEY_ITEM;


static LIST_ENTRY HotKeyListHead;
static FAST_MUTEX HotKeyListLock;


/* FUNCTIONS *****************************************************************/

NTSTATUS FASTCALL
InitHotKeyImpl(VOID)
{
  InitializeListHead(&HotKeyListHead);
  ExInitializeFastMutex(&HotKeyListLock);

  return STATUS_SUCCESS;
}


NTSTATUS FASTCALL
CleanupHotKeyImpl(VOID)
{

  return STATUS_SUCCESS;
}


BOOL
GetHotKey (UINT fsModifiers,
	   UINT vk,
	   struct _ETHREAD **Thread,
	   HWND *hWnd,
	   int *id)
{
  PLIST_ENTRY Entry;
  PHOT_KEY_ITEM HotKeyItem;

  ExAcquireFastMutex (&HotKeyListLock);

  Entry = HotKeyListHead.Flink;
  while (Entry != &HotKeyListHead)
    {
      HotKeyItem = (PHOT_KEY_ITEM) CONTAINING_RECORD(Entry,
						     HOT_KEY_ITEM,
						     ListEntry);
      if (HotKeyItem->fsModifiers == fsModifiers &&
	  HotKeyItem->vk == vk)
	{
	  if (Thread != NULL)
	    *Thread = HotKeyItem->Thread;

	  if (hWnd != NULL)
	    *hWnd = HotKeyItem->hWnd;

	  if (id != NULL)
	    *id = HotKeyItem->id;

	  ExReleaseFastMutex (&HotKeyListLock);

	  return TRUE;
	}

      Entry = Entry->Flink;
    }

  ExReleaseFastMutex (&HotKeyListLock);

  return FALSE;
}


VOID
UnregisterWindowHotKeys(HWND hWnd)
{
  PLIST_ENTRY Entry;
  PHOT_KEY_ITEM HotKeyItem;

  ExAcquireFastMutex (&HotKeyListLock);

  Entry = HotKeyListHead.Flink;
  while (Entry != &HotKeyListHead)
    {
      HotKeyItem = (PHOT_KEY_ITEM) CONTAINING_RECORD (Entry,
						      HOT_KEY_ITEM,
						      ListEntry);
      Entry = Entry->Flink;
      if (HotKeyItem->hWnd == hWnd)
	{
	  RemoveEntryList (&HotKeyItem->ListEntry);
	  ExFreePool (HotKeyItem);
	}
    }

  ExReleaseFastMutex (&HotKeyListLock);
}


VOID
UnregisterThreadHotKeys(struct _ETHREAD *Thread)
{
  PLIST_ENTRY Entry;
  PHOT_KEY_ITEM HotKeyItem;

  ExAcquireFastMutex (&HotKeyListLock);

  Entry = HotKeyListHead.Flink;
  while (Entry != &HotKeyListHead)
    {
      HotKeyItem = (PHOT_KEY_ITEM) CONTAINING_RECORD (Entry,
						      HOT_KEY_ITEM,
						      ListEntry);
      Entry = Entry->Flink;
      if (HotKeyItem->Thread == Thread)
	{
	  RemoveEntryList (&HotKeyItem->ListEntry);
	  ExFreePool (HotKeyItem);
	}
    }

  ExReleaseFastMutex (&HotKeyListLock);
}


static BOOL
IsHotKey (UINT fsModifiers,
	  UINT vk)
{
  PLIST_ENTRY Entry;
  PHOT_KEY_ITEM HotKeyItem;

  Entry = HotKeyListHead.Flink;
  while (Entry != &HotKeyListHead)
    {
      HotKeyItem = (PHOT_KEY_ITEM) CONTAINING_RECORD (Entry,
						      HOT_KEY_ITEM,
						      ListEntry);
      if (HotKeyItem->fsModifiers == fsModifiers &&
	  HotKeyItem->vk == vk)
	{
	  return TRUE;
	}

      Entry = Entry->Flink;
    }

  return FALSE;
}


BOOL STDCALL
NtUserRegisterHotKey(HWND hWnd,
		     int id,
		     UINT fsModifiers,
		     UINT vk)
{
  PHOT_KEY_ITEM HotKeyItem;

  ExAcquireFastMutex (&HotKeyListLock);

  /* Check for existing hotkey */
  if (IsHotKey (fsModifiers, vk))
    return FALSE;

  HotKeyItem = ExAllocatePool (PagedPool,
			       sizeof(HOT_KEY_ITEM));
  if (HotKeyItem == NULL)
    {
      ExReleaseFastMutex (&HotKeyListLock);
      return FALSE;
    }

  HotKeyItem->Thread = PsGetCurrentThread();
  HotKeyItem->hWnd = hWnd;
  HotKeyItem->id = id;
  HotKeyItem->fsModifiers = fsModifiers;
  HotKeyItem->vk = vk;

  InsertHeadList (&HotKeyListHead,
		  &HotKeyItem->ListEntry);

  ExReleaseFastMutex (&HotKeyListLock);

  return TRUE;
}


BOOL STDCALL
NtUserUnregisterHotKey(HWND hWnd,
		       int id)
{
  PLIST_ENTRY Entry;
  PHOT_KEY_ITEM HotKeyItem;

  ExAcquireFastMutex (&HotKeyListLock);

  Entry = HotKeyListHead.Flink;
  while (Entry != &HotKeyListHead)
    {
      HotKeyItem = (PHOT_KEY_ITEM) CONTAINING_RECORD (Entry,
						      HOT_KEY_ITEM,
						      ListEntry);
      if (HotKeyItem->hWnd == hWnd &&
	  HotKeyItem->id == id)
	{
	  RemoveEntryList (&HotKeyItem->ListEntry);
	  ExFreePool (HotKeyItem);
	  ExReleaseFastMutex (&HotKeyListLock);
	  return TRUE;
	}

      Entry = Entry->Flink;
    }

  ExReleaseFastMutex (&HotKeyListLock);

  return FALSE;
}

/* EOF */
