/*
 * COPYRIGHT:          See COPYING in the top level directory
 * PROJECT:            ReactOS project
 * FILE:               kernel/rtl/slist.c
 * PURPOSE:            Implements single linked lists
 * PROGRAMMER:         David Welch (welch@mcmail.com)
 * REVISION HISTORY:
 *              28/06/98: Created
 */

/* INCLUDES *****************************************************************/

#include <windows.h>
#include <ddk/ntddk.h>
#include <internal/kernel.h>

#include <internal/debug.h>

/* FUNCTIONS ****************************************************************/

PSINGLE_LIST_ENTRY ExInterlockedPopEntrySList(PSLIST_HEADER ListHead,
					      PKSPIN_LOCK Lock)
{
   UNIMPLEMENTED;
}

PSINGLE_LIST_ENTRY ExInterlockedPushEntrySList(PSLIST_HEADER ListHead,
					       PSINGLE_LIST_ENTRY ListEntry,
					       PKSPIN_LOCK Lock)
{
   UNIMPLEMENTED;
}

USHORT ExQueryDepthSListHead(PSLIST_HEADER SListHead)
{
   UNIMPLEMENTED;
}			      

VOID ExInitializeSListHead(PSLIST_HEADER SListHead)
{
   UNIMPLEMENTED;
}

PSINGLE_LIST_ENTRY PopEntryList(PSINGLE_LIST_ENTRY ListHead)
/*
 * FUNCTION: Removes an entry from the head of a single linked list
 * ARGUMENTS:
 *         ListHead = Head of the list
 * RETURNS: The removed entry
 */
{
   PSINGLE_LIST_ENTRY entry = ListHead->Next;
   if (entry==NULL)
     {
	return(NULL);
     }
   ListHead->Next = ListHead->Next->Next;
   return(entry);
}

PSINGLE_LIST_ENTRY ExInterlockedPopEntryList(PSINGLE_LIST_ENTRY ListHead,
					     PKSPIN_LOCK Lock)
{
   PSINGLE_LIST_ENTRY ret;
   KIRQL oldlvl;
   
   KeAcquireSpinLock(Lock,&oldlvl);
   ret = PopEntryList(ListHead);
   KeReleaseSpinLock(Lock,oldlvl);
   return(ret);
}

VOID PushEntryList(PSINGLE_LIST_ENTRY ListHead, PSINGLE_LIST_ENTRY Entry)
{
   Entry->Next = ListHead->Next;
   ListHead->Next = Entry;
}

PSINGLE_LIST_ENTRY ExInterlockedPushEntryList(PSINGLE_LIST_ENTRY ListHead,
					      PSINGLE_LIST_ENTRY ListEntry,
					      PKSPIN_LOCK Lock)
{
   KIRQL oldlvl;
   PSINGLE_LIST_ENTRY ret;
   
   KeAcquireSpinLock(Lock,&oldlvl);
   ret=ListHead->Next;
   PushEntryList(ListHead,ListEntry);
   KeReleaseSpinLock(Lock,oldlvl);
   return(ret);
}

