/* $Id: slist.c,v 1.3 2000/06/07 13:05:09 ekohl Exp $
 *
 * COPYRIGHT:          See COPYING in the top level directory
 * PROJECT:            ReactOS project
 * FILE:               kernel/rtl/slist.c
 * PURPOSE:            Implements single linked lists
 * PROGRAMMER:         David Welch (welch@mcmail.com)
 * REVISION HISTORY:
 *              28/06/98: Created
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS ****************************************************************/

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


USHORT
STDCALL
ExQueryDepthSListHead (
	PSLIST_HEADER SListHead
	)
{
   UNIMPLEMENTED;
}


VOID
STDCALL
ExInitializeSListHead (
	PSLIST_HEADER	SListHead
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

PSINGLE_LIST_ENTRY PopEntryList(PSINGLE_LIST_ENTRY ListHead)
/*
 * FUNCTION: Removes an entry from the head of a single linked list
 * ARGUMENTS:
 *         ListHead = Head of the list
 * RETURNS: The removed entry
 */
{
   PSINGLE_LIST_ENTRY ListEntry = ListHead->Next;
   if (ListEntry!=NULL)
     {
	ListHead->Next = ListEntry->Next;
     }
   return(ListEntry);
}


VOID PushEntryList(PSINGLE_LIST_ENTRY ListHead, PSINGLE_LIST_ENTRY Entry)
{
   Entry->Next = ListHead->Next;
   ListHead->Next = Entry;
}

/* EOF */
