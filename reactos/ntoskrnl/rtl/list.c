/*
 * COPYRIGHT:           See COPYING in the top level directory
 * PROJECT:             ReactOS kernel
 * FILE:                ntoskrnl/rtl/list.c
 * PURPOSE:             Manages linked lists
 * PROGRAMMER:          David Welch (welch@mcmail.com)
 */

/* INCLUDES *****************************************************************/

#include <windows.h>
#include <ddk/ntddk.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *************************************************************/

BOOLEAN IsListEmpty(PLIST_ENTRY ListHead)
/*
 * FUNCTION: Determines if a list is empty
 * ARGUMENTS:
 *        ListHead = Head of the list
 * RETURNS: True if there are no entries in the list
 */
{
   return(ListHead->Flink==ListHead);
}

VOID RemoveEntryList(PLIST_ENTRY ListEntry)
{
   PLIST_ENTRY OldFlink;
   PLIST_ENTRY OldBlink;
   DPRINT("RemoveEntryList(ListEntry %x)\n",ListEntry);
   OldFlink=ListEntry->Flink;
   OldBlink=ListEntry->Blink;
   DPRINT("OldFlink %x OldBlink %x\n",OldFlink,OldBlink);
   OldFlink->Blink=OldBlink;
   OldBlink->Flink=OldFlink;
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
   PLIST_ENTRY Old = ListHead->Flink;
   DPRINT("RemoveHeadList(ListHead %x)\n",ListHead);
   RemoveEntryList(ListHead->Flink);
   return(Old);
}

VOID InitializeListHead(PLIST_ENTRY ListHead)
/*
 * FUNCTION: Initializes a double linked list
 * ARGUMENTS: 
 *         ListHead = Caller supplied storage for the head of the list
 */
{
   ListHead->Flink = ListHead->Blink = ListHead;
}

VOID InsertTailList(PLIST_ENTRY ListHead, PLIST_ENTRY ListEntry)
/*
 * FUNCTION: Inserts an entry in a double linked list
 * ARGUMENTS:
 *        ListHead = Head of the list
 *        Entry = Entry to insert
 */
{
   PLIST_ENTRY Blink;
   
   Blink = ListHead->Blink;
   ListEntry->Flink=ListHead;
   ListEntry->Blink=Blink;
   Blink->Flink=ListEntry;
   ListHead->Blink=ListEntry;
}

VOID InsertHeadList(PLIST_ENTRY ListHead, PLIST_ENTRY ListEntry)
{
   PLIST_ENTRY OldFlink;
   
   OldFlink = ListHead->Flink;
   ListEntry->Flink = OldFlink;
   ListEntry->Blink = ListHead;
   OldFlink->Blink = ListEntry;
   ListHead->Flink = ListEntry;
}

PLIST_ENTRY ExInterlockedInsertTailList(PLIST_ENTRY ListHead,
					PLIST_ENTRY ListEntry,
					PKSPIN_LOCK Lock)
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

PLIST_ENTRY ExInterlockedInsertHeadList(PLIST_ENTRY ListHead,
					PLIST_ENTRY ListEntry,
					PKSPIN_LOCK Lock)
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


PLIST_ENTRY ExInterlockedRemoveHeadList(PLIST_ENTRY Head,
					PKSPIN_LOCK Lock)
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

PLIST_ENTRY ExInterlockedRemoveTailList(PLIST_ENTRY Head,
					PKSPIN_LOCK Lock)
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
