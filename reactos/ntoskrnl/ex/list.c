/* $Id: list.c,v 1.6 2002/09/07 15:12:50 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/list.c
 * PURPOSE:         Manages double linked lists, single linked lists and
 *                  sequenced lists
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 *                  Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *   02-07-2001 CSH Implemented sequenced lists
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>

static KSPIN_LOCK ExpGlobalInterlockedLock = 0;

/* FUNCTIONS *************************************************************/

PLIST_ENTRY FASTCALL
ExInterlockedInsertHeadList(PLIST_ENTRY ListHead,
			    PLIST_ENTRY ListEntry,
			    PKSPIN_LOCK Lock)
/*
 * FUNCTION: Inserts an entry at the head of a doubly linked list
 * ARGUMENTS:
 *          ListHead  = Points to the head of the list
 *          ListEntry = Points to the entry to be inserted
 *          Lock      = Caller supplied spinlock used to synchronize access
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


PLIST_ENTRY FASTCALL
ExInterlockedInsertTailList(PLIST_ENTRY ListHead,
			    PLIST_ENTRY ListEntry,
			    PKSPIN_LOCK Lock)
/*
 * FUNCTION: Inserts an entry at the tail of a doubly linked list
 * ARGUMENTS:
 *          ListHead  = Points to the head of the list
 *          ListEntry = Points to the entry to be inserted
 *          Lock      = Caller supplied spinlock used to synchronize access
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
      Old = ListHead->Blink;
    }
  InsertTailList(ListHead,ListEntry);
  KeReleaseSpinLock(Lock,oldlvl);

  return(Old);
}


PLIST_ENTRY FASTCALL
ExInterlockedRemoveHeadList(PLIST_ENTRY Head,
			    PKSPIN_LOCK Lock)
/*
 * FUNCTION: Removes the head of a double linked list
 * ARGUMENTS:
 *          Head = Points to the head of the list
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

PLIST_ENTRY FASTCALL
ExInterlockedRemoveTailList(PLIST_ENTRY Head,
			    PKSPIN_LOCK Lock)
/*
 * FUNCTION: Removes the tail of a double linked list
 * ARGUMENTS:
 *          Head = Points to the head of the list
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

#undef ExInterlockedPopEntrySList

PSINGLE_LIST_ENTRY FASTCALL
ExInterlockedPopEntrySList(IN PSLIST_HEADER ListHead,
			   IN PKSPIN_LOCK Lock)
/*
 * FUNCTION: Removes (pops) an entry from a sequenced list
 * ARGUMENTS:
 *          ListHead = Points to the head of the list
 *          Lock     = Lock for synchronizing access to the list
 * RETURNS: The removed entry
 */
{
  PSINGLE_LIST_ENTRY ret;
  KIRQL oldlvl;

  KeAcquireSpinLock(Lock,&oldlvl);
  ret = PopEntryList(&ListHead->Next);
  if (ret)
    {
      ListHead->Depth--;
      ListHead->Sequence++;
    }
  KeReleaseSpinLock(Lock,oldlvl);
  return(ret);
}

#undef ExInterlockedPushEntrySList

PSINGLE_LIST_ENTRY FASTCALL
ExInterlockedPushEntrySList(IN PSLIST_HEADER ListHead,
			    IN PSINGLE_LIST_ENTRY ListEntry,
			    IN PKSPIN_LOCK Lock)
/*
 * FUNCTION: Inserts (pushes) an entry into a sequenced list
 * ARGUMENTS:
 *          ListHead  = Points to the head of the list
 *          ListEntry = Points to the entry to be inserted
 *          Lock      = Caller supplied spinlock used to synchronize access
 * RETURNS: The previous head of the list
 */
{
  KIRQL oldlvl;
  PSINGLE_LIST_ENTRY ret;

  KeAcquireSpinLock(Lock,&oldlvl);
  ret=ListHead->Next.Next;
  PushEntryList(&ListHead->Next,ListEntry);
  ListHead->Depth++;
  ListHead->Sequence++;
  KeReleaseSpinLock(Lock,oldlvl);
  return(ret);
}


PSINGLE_LIST_ENTRY FASTCALL
ExInterlockedPopEntryList(IN PSINGLE_LIST_ENTRY ListHead,
			  IN PKSPIN_LOCK Lock)
/*
 * FUNCTION: Removes (pops) an entry from a singly list
 * ARGUMENTS:
 *          ListHead = Points to the head of the list
 *          Lock     = Lock for synchronizing access to the list
 * RETURNS: The removed entry
 */
{
  PSINGLE_LIST_ENTRY ret;
  KIRQL oldlvl;

  KeAcquireSpinLock(Lock,&oldlvl);
  ret = PopEntryList(ListHead);
  KeReleaseSpinLock(Lock,oldlvl);
  return(ret);
}


PSINGLE_LIST_ENTRY FASTCALL
ExInterlockedPushEntryList(IN PSINGLE_LIST_ENTRY ListHead,
			   IN PSINGLE_LIST_ENTRY ListEntry,
			   IN PKSPIN_LOCK Lock)
/*
 * FUNCTION: Inserts (pushes) an entry into a singly linked list
 * ARGUMENTS:
 *          ListHead  = Points to the head of the list
 *          ListEntry = Points to the entry to be inserted
 *          Lock      = Caller supplied spinlock used to synchronize access
 * RETURNS: The previous head of the list
 */
{
  KIRQL oldlvl;
  PSINGLE_LIST_ENTRY ret;

  KeAcquireSpinLock(Lock,&oldlvl);
  ret=ListHead->Next;
  PushEntryList(ListHead,ListEntry);
  KeReleaseSpinLock(Lock,oldlvl);
  return(ret);
}


PLIST_ENTRY FASTCALL
ExfInterlockedInsertHeadList(IN PLIST_ENTRY ListHead,
			     IN PLIST_ENTRY ListEntry,
			     IN PKSPIN_LOCK Lock)
/*
 * FUNCTION: Inserts an entry at the head of a doubly linked list
 * ARGUMENTS:
 *          ListHead  = Points to the head of the list
 *          ListEntry = Points to the entry to be inserted
 *          Lock      = Caller supplied spinlock used to synchronize access
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


PLIST_ENTRY FASTCALL
ExfInterlockedInsertTailList(IN PLIST_ENTRY ListHead,
			     IN PLIST_ENTRY ListEntry,
			     IN PKSPIN_LOCK Lock)
/*
 * FUNCTION: Inserts an entry at the tail of a doubly linked list
 * ARGUMENTS:
 *          ListHead  = Points to the head of the list
 *          ListEntry = Points to the entry to be inserted
 *          Lock      = Caller supplied spinlock used to synchronize access
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
      Old = ListHead->Blink;
    }
  InsertTailList(ListHead,ListEntry);
  KeReleaseSpinLock(Lock,oldlvl);

  return(Old);
}


PSINGLE_LIST_ENTRY FASTCALL
ExfInterlockedPopEntryList(IN PSINGLE_LIST_ENTRY ListHead,
			   IN PKSPIN_LOCK Lock)
/*
 * FUNCTION: Removes (pops) an entry from a singly list
 * ARGUMENTS:
 *          ListHead = Points to the head of the list
 *          Lock     = Lock for synchronizing access to the list
 * RETURNS: The removed entry
 */
{
  PSINGLE_LIST_ENTRY ret;
  KIRQL oldlvl;

  KeAcquireSpinLock(Lock,&oldlvl);
  ret = PopEntryList(ListHead);
  KeReleaseSpinLock(Lock,oldlvl);
  return(ret);
}


PSINGLE_LIST_ENTRY FASTCALL
ExfInterlockedPushEntryList(IN PSINGLE_LIST_ENTRY ListHead,
			    IN PSINGLE_LIST_ENTRY ListEntry,
			    IN PKSPIN_LOCK Lock)
/*
 * FUNCTION: Inserts (pushes) an entry into a singly linked list
 * ARGUMENTS:
 *          ListHead  = Points to the head of the list
 *          ListEntry = Points to the entry to be inserted
 *          Lock      = Caller supplied spinlock used to synchronize access
 * RETURNS: The previous head of the list
 */
{
  KIRQL oldlvl;
  PSINGLE_LIST_ENTRY ret;

  KeAcquireSpinLock(Lock,&oldlvl);
  ret=ListHead->Next;
  PushEntryList(ListHead,ListEntry);
  KeReleaseSpinLock(Lock,oldlvl);
  return(ret);
}


PLIST_ENTRY FASTCALL
ExfInterlockedRemoveHeadList(IN PLIST_ENTRY Head,
			     IN PKSPIN_LOCK Lock)
/*
 * FUNCTION: Removes the head of a double linked list
 * ARGUMENTS:
 *          Head = Points to the head of the list
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


PSLIST_ENTRY
FASTCALL
InterlockedPopEntrySList(
  IN PSLIST_HEADER  ListHead)
{
	KIRQL OldIrql;
	PSLIST_ENTRY Value;

	KeAcquireSpinLock(&ExpGlobalInterlockedLock, &OldIrql);
	Value = PopEntryList(&ListHead->Next);
	if (Value)
		{
			ListHead->Depth--;
			ListHead->Sequence++;
	  }
	KeReleaseSpinLock(&ExpGlobalInterlockedLock, OldIrql);
	return(Value);
}


PSLIST_ENTRY
FASTCALL
InterlockedPushEntrySList(
  IN PSLIST_HEADER  ListHead,
  IN PSLIST_ENTRY  ListEntry)
{
	KIRQL OldIrql;
	PSLIST_ENTRY Value;

	KeAcquireSpinLock(&ExpGlobalInterlockedLock, &OldIrql);
	Value = ListHead->Next.Next;
	PushEntryList(&ListHead->Next, ListEntry);
	ListHead->Depth++;
	ListHead->Sequence++;
	KeReleaseSpinLock(&ExpGlobalInterlockedLock, OldIrql);
	return(Value);
}

/* EOF */
