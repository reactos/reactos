/* $Id: list.c,v 1.2 2000/10/07 13:41:50 dwelch Exp $
 *
 * COPYRIGHT:           See COPYING in the top level directory
 * PROJECT:             ReactOS kernel
 * FILE:                ntoskrnl/ex/list.c
 * PURPOSE:             Manages double linked lists, single linked lists and
 *                      sequenced lists
 * PROGRAMMER:          David Welch (welch@mcmail.com)
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *************************************************************/


PLIST_ENTRY STDCALL
ExInterlockedInsertHeadList (
	PLIST_ENTRY	ListHead,
	PLIST_ENTRY	ListEntry,
	PKSPIN_LOCK	Lock
	)
/*
 * FUNCTION: Inserts an entry at the head of a doubly linked list
 * ARGUMENTS:
 *          ListHead = Points to the head of the list
 *          ListEntry = Points to the entry to be inserted
 *          Lock = Caller supplied spinlock used to synchronise access
 * RETURNS: The previous head of the list
 */
{
   PLIST_ENTRY Old;
   KIRQL oldlvl;

   KeAcquireSpinLock(Lock,&oldlvl);
   if (IsListEmpty(ListHead))
     {
	Old = NULL;
     }
   else
     {
	Old = ListHead->Flink;
     }
   InsertHeadList(ListHead,ListEntry);
   KeReleaseSpinLock(Lock,oldlvl);

   return(Old);
}


PLIST_ENTRY STDCALL
ExInterlockedInsertTailList (
	PLIST_ENTRY	ListHead,
	PLIST_ENTRY	ListEntry,
	PKSPIN_LOCK	Lock
	)
{
   PLIST_ENTRY Old;
   KIRQL oldlvl;

   KeAcquireSpinLock(Lock,&oldlvl);
   if (IsListEmpty(ListHead))
     {
	Old = NULL;
     }
   else
     {
	Old = ListHead->Blink;
     }
   InsertTailList(ListHead,ListEntry);
   KeReleaseSpinLock(Lock,oldlvl);

   return(Old);
}


PLIST_ENTRY STDCALL
ExInterlockedRemoveHeadList (
	PLIST_ENTRY	Head,
	PKSPIN_LOCK	Lock
	)
/*
 * FUNCTION: Removes the head of a double linked list
 * ARGUMENTS:
 *          Head = List head
 *          Lock = Lock for synchronizing access to the list
 * RETURNS: The removed entry
 */
{
   PLIST_ENTRY ret;
   KIRQL oldlvl;

   KeAcquireSpinLock(Lock,&oldlvl);
   if (IsListEmpty(Head))
     {
	ret = NULL;
     }
   else
     {
	ret = RemoveHeadList(Head);
     }
   KeReleaseSpinLock(Lock,oldlvl);
   return(ret);
}

PLIST_ENTRY
ExInterlockedRemoveTailList (
	PLIST_ENTRY	Head,
	PKSPIN_LOCK	Lock
	)
/*
 * FUNCTION: Removes the tail of a double linked list
 * ARGUMENTS:
 *          Head = List head
 *          Lock = Lock for synchronizing access to the list
 * RETURNS: The removed entry
 */
{
   PLIST_ENTRY ret;
   KIRQL oldlvl;

   KeAcquireSpinLock(Lock,&oldlvl);
   if (IsListEmpty(Head))
     {
	ret = NULL;
     }
   else
     {
	ret = RemoveTailList(Head);
     }
   KeReleaseSpinLock(Lock,oldlvl);
   return(ret);
}


PSINGLE_LIST_ENTRY
STDCALL
ExInterlockedPopEntrySList (
	PSLIST_HEADER	ListHead,
	PKSPIN_LOCK	Lock
	)
{
   UNIMPLEMENTED;
}


PSINGLE_LIST_ENTRY
STDCALL
ExInterlockedPushEntrySList (
	PSLIST_HEADER		ListHead,
	PSINGLE_LIST_ENTRY	ListEntry,
	PKSPIN_LOCK		Lock
	)
{
   UNIMPLEMENTED;
}


PSINGLE_LIST_ENTRY
STDCALL
ExInterlockedPopEntryList (
	PSINGLE_LIST_ENTRY	ListHead,
	PKSPIN_LOCK		Lock
	)
{
   PSINGLE_LIST_ENTRY ret;
   KIRQL oldlvl;

   KeAcquireSpinLock(Lock,&oldlvl);
   ret = PopEntryList(ListHead);
   KeReleaseSpinLock(Lock,oldlvl);
   return(ret);
}


PSINGLE_LIST_ENTRY
STDCALL
ExInterlockedPushEntryList (
	PSINGLE_LIST_ENTRY	ListHead,
	PSINGLE_LIST_ENTRY	ListEntry,
	PKSPIN_LOCK		Lock
	)
{
   KIRQL oldlvl;
   PSINGLE_LIST_ENTRY ret;

   KeAcquireSpinLock(Lock,&oldlvl);
   ret=ListHead->Next;
   PushEntryList(ListHead,ListEntry);
   KeReleaseSpinLock(Lock,oldlvl);
   return(ret);
}

/* EOF */
