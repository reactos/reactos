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
#include <internal/kernel.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *************************************************************/

VOID RemoveEntryList(PLIST_ENTRY Entry)
/*
 * FUNCTION: Resets the links for an entry from a double linked list
 * ARGUMENTS:
 *        Entry = Entry to reset
 * NOTE: This isn't the same as ExInterlockedRemoveEntryList
 */
{
   if (Entry->Blink!=NULL) 
     {
	Entry->Blink->Flink=Entry->Flink;
     }
   if (Entry->Flink!=NULL)
     {
	Entry->Flink->Blink=Entry->Blink;
     }
}

PLIST_ENTRY RemoveTailList(PLIST_ENTRY ListHead)
/*
 * FUNCTION: Remove the tail entry from a double linked list
 * ARGUMENTS:
 *         ListHead = Head of the list to remove from
 * RETURNS: The removed entry
 */
{
   PLIST_ENTRY entry = ListHead->Blink;
   if (ListHead->Blink == ListHead->Flink)
     {
	ListHead->Flink = NULL;
     }
   if (entry!=NULL)
     {
	ListHead->Blink = entry->Blink;
     }
   return(entry);
}

VOID ExInterlockedRemoveEntryList(PLIST_ENTRY ListHead, PLIST_ENTRY Entry,
				  PKSPIN_LOCK Lock)
{
   KIRQL oldlvl;
   KeAcquireSpinLock(Lock,&oldlvl);
   if (ListHead->Flink == Entry && ListHead->Blink == Entry)
     {
	ListHead->Flink = ListHead->Blink = NULL;
	Entry->Flink = Entry->Blink = NULL;
	KeReleaseSpinLock(Lock,oldlvl);	
	return;
     }
   if (ListHead->Flink == Entry)
     {
	ListHead->Flink=Entry->Flink;
	Entry->Flink = Entry->Blink = NULL;
	KeReleaseSpinLock(Lock,oldlvl);
	return;
     }
   if (ListHead->Blink == Entry)
     {
	ListHead->Blink = Entry->Blink;
	Entry->Flink = Entry->Blink = NULL;
	KeReleaseSpinLock(Lock,oldlvl);
	return;
     }
   Entry->Flink->Blink = Entry->Blink;
   Entry->Blink->Flink = Entry->Flink;
   Entry->Flink = Entry->Blink = NULL;
   KeReleaseSpinLock(Lock,oldlvl);
}

PLIST_ENTRY RemoveHeadList(PLIST_ENTRY ListHead)
{
   PLIST_ENTRY Old;
   
   DPRINT("RemoveHeadList(ListHead %x)\n",ListHead);
   DPRINT("Flink %x Blink %x\n",ListHead->Flink,ListHead->Blink);
   
   Old = ListHead->Flink;
   
   if (ListHead->Flink==NULL)
     {
	return(NULL);
     }
   
   DPRINT("ListHead->Flink->Flink %x\n",ListHead->Flink->Flink);
   ListHead->Flink=ListHead->Flink->Flink;
   if (ListHead->Flink!=NULL)
     {
	DPRINT("ListHead->Flink->Blink %x\n",ListHead->Flink->Blink);
	ListHead->Flink->Blink=NULL;
     }
   return(Old);
}

VOID RemoveEntryFromList(PLIST_ENTRY ListHead, PLIST_ENTRY Entry)
{
   if (ListHead->Flink == Entry && ListHead->Blink == Entry)
     {
	ListHead->Flink = ListHead->Blink = NULL;
	Entry->Flink = Entry->Blink = NULL;
	return;
     }
   if (ListHead->Flink == Entry)
     {
	ListHead->Flink=Entry->Flink;
	Entry->Flink = Entry->Blink = NULL;
	return;
     }
   if (ListHead->Blink == Entry)
     {
	ListHead->Blink = Entry->Blink;
	Entry->Flink = Entry->Blink = NULL;
	return;
     }
   Entry->Flink->Blink = Entry->Blink;
   Entry->Blink->Flink = Entry->Flink;
   Entry->Flink = Entry->Blink = NULL;
}




VOID InitializeListHead(PLIST_ENTRY ListHead)
/*
 * FUNCTION: Initializes a double linked list
 * ARGUMENTS: 
 *         ListHead = Caller supplied storage for the head of the list
 */
{
   ListHead->Blink=NULL;
   ListHead->Flink=NULL;
}

VOID InsertTailList(PLIST_ENTRY ListHead, PLIST_ENTRY ListEntry)
/*
 * FUNCTION: Inserts an entry in a double linked list
 * ARGUMENTS:
 *        ListHead = Head of the list
 *        Entry = Entry to insert
 */
{
   assert(ListHead!=NULL);
   assert(ListEntry!=NULL);
   if (ListHead->Blink==NULL)
     {
	ListEntry->Blink = ListEntry->Flink = NULL;
	ListHead->Blink = ListHead->Flink = ListEntry;	
     }
   else
     {
	ListEntry->Blink = ListHead->Blink;
	ListEntry->Flink = NULL;
	ListHead->Blink->Flink = ListEntry;
	ListHead->Blink = ListEntry;
     }
}

VOID InsertHeadList(PLIST_ENTRY ListHead, PLIST_ENTRY ListEntry)
{
   /*
    * Insert the entry
    */   
   ListEntry->Blink=NULL;
   ListEntry->Flink=ListHead->Flink;
   
   if (ListHead->Flink==NULL)
     {
	assert(ListHead->Blink==NULL);
	ListHead->Blink = ListHead->Flink = ListEntry;
     }
   else
     {	   
	ListHead->Flink->Blink = ListEntry;
	ListHead->Flink = ListEntry;
     }
}

PLIST_ENTRY ExInterlockedInsertTailList(PLIST_ENTRY ListHead,
					PLIST_ENTRY ListEntry,
					PKSPIN_LOCK Lock)
{
   PLIST_ENTRY Old = NULL;
   KIRQL oldlvl;

   /*
    * Lock the spinlock
    */
   KeAcquireSpinLock(Lock,&oldlvl);
   DPRINT("oldlvl %x\n",oldlvl);
   /*
    * Insert the entry
    */
   Old = ListHead->Flink;
   InsertTailList(ListHead,ListEntry);
   
   /*
    * Unlock the spinlock
    */
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
   PLIST_ENTRY old = NULL;
   KIRQL oldlvl;
   
   /*
    * Block on the spinlock here
    */
   KeAcquireSpinLock(Lock,&oldlvl);
   
   /*
    * Insert the entry
    */
   old=ListHead->Flink;
   InsertHeadList(ListHead,ListEntry);
   
   /*
    * Release spin lock
    */
   KeReleaseSpinLock(Lock,oldlvl);
   
   return(old);
}

BOOLEAN IsListEmpty(PLIST_ENTRY ListHead)
/*
 * FUNCTION: Determines if a list is emptry
 * ARGUMENTS:
 *        ListHead = Head of the list
 * RETURNS: True if there are no entries in the list
 */
{
   return(ListHead->Flink==NULL);
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
   ret = RemoveHeadList(Head);
   KeReleaseSpinLock(Lock,oldlvl);
   return(ret);
}
