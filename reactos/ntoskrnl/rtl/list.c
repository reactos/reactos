/* $Id: list.c,v 1.7 2000/06/18 15:21:53 ekohl Exp $
 *
 * COPYRIGHT:           See COPYING in the top level directory
 * PROJECT:             ReactOS kernel
 * FILE:                ntoskrnl/rtl/list.c
 * PURPOSE:             Manages linked lists
 * PROGRAMMER:          David Welch (welch@mcmail.com)
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *************************************************************/

#if 0
static BOOLEAN CheckEntry(PLIST_ENTRY ListEntry)
{
   assert(ListEntry!=NULL);
   assert(ListEntry->Blink!=NULL);
   assert(ListEntry->Blink->Flink==ListEntry);
   assert(ListEntry->Flink!=NULL);
   assert(ListEntry->Flink->Blink==ListEntry);
   return(TRUE);
}

PLIST_ENTRY RemoveTailList(PLIST_ENTRY ListHead)
/*
 * FUNCTION: Remove the tail entry from a double linked list
 * ARGUMENTS:
 *         ListHead = Head of the list to remove from
 * RETURNS: The removed entry
 */
{
   PLIST_ENTRY Old = ListHead->Blink;
   RemoveEntryList(ListHead->Blink);
   return(Old);
}

PLIST_ENTRY RemoveHeadList(PLIST_ENTRY ListHead)
{
   PLIST_ENTRY Old;

   DPRINT("RemoveHeadList(ListHead %x)\n",ListHead);

   assert(CheckEntry(ListHead));

   Old = ListHead->Flink;
   RemoveEntryList(ListHead->Flink);

   DPRINT("RemoveHeadList()\n");

   return(Old);
}
#endif


PLIST_ENTRY
STDCALL
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

PLIST_ENTRY
STDCALL
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


PLIST_ENTRY
STDCALL
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

/* EOF */
