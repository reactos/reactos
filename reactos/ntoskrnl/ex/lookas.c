/* $Id: lookas.c,v 1.9 2003/07/11 01:23:14 royce Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/lookas.c
 * PURPOSE:         Lookaside lists
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 *                  Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *   22-05-1998 DW  Created
 *   02-07-2001 CSH Implemented lookaside lists
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ex.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

LIST_ENTRY ExpNonPagedLookasideListHead;
KSPIN_LOCK ExpNonPagedLookasideListLock;

LIST_ENTRY ExpPagedLookasideListHead;
KSPIN_LOCK ExpPagedLookasideListLock;

PLOOKASIDE_MINMAX_ROUTINE ExpMinMaxRoutine;

/* FUNCTIONS *****************************************************************/

VOID ExpDefaultMinMax(
  POOL_TYPE PoolType,
  ULONG Size,
  PUSHORT MinimumDepth,
  PUSHORT MaximumDepth)
/*
 * FUNCTION: Determines the minimum and maximum depth of a new lookaside list
 * ARGUMENTS:
 *   Type         = Type of executive pool
 *   Size         = Size in bytes of each element in the new lookaside list
 *   MinimumDepth = Buffer to store minimum depth of the new lookaside list in
 *   MaximumDepth = Buffer to store maximum depth of the new lookaside list in
 */
{
  /* FIXME: Could probably do some serious computing here */
  if ((PoolType == NonPagedPool) ||
    (PoolType == NonPagedPoolMustSucceed))
  {
    *MinimumDepth = 10;
    *MaximumDepth = 100;
  }
  else
  {
    *MinimumDepth = 20;
    *MaximumDepth = 200;
  }
}


PVOID STDCALL
ExpDefaultAllocate(POOL_TYPE PoolType,
		   ULONG NumberOfBytes,
		   ULONG Tag)
/*
 * FUNCTION: Default allocate function for lookaside lists
 * ARGUMENTS:
 *   Type          = Type of executive pool
 *   NumberOfBytes = Number of bytes to allocate
 *   Tag           = Tag to use
 * RETURNS:
 *   Pointer to allocated memory, or NULL if there is not enough free resources
 */
{
  return ExAllocatePoolWithTag(PoolType, NumberOfBytes, Tag);
}


VOID STDCALL
ExpDefaultFree(PVOID Buffer)
/*
 * FUNCTION: Default free function for lookaside lists
 * ARGUMENTS:
 *   Buffer = Pointer to memory to free
 */
{
  return ExFreePool(Buffer);
}


VOID
ExpInitLookasideLists()
{
  InitializeListHead(&ExpNonPagedLookasideListHead);
  KeInitializeSpinLock(&ExpNonPagedLookasideListLock);

  InitializeListHead(&ExpPagedLookasideListHead);
  KeInitializeSpinLock(&ExpPagedLookasideListLock);

  /* FIXME: Possibly configure the algorithm using the registry */
  ExpMinMaxRoutine = ExpDefaultMinMax;
}

/*
 * @implemented
 */
PVOID
STDCALL
ExAllocateFromPagedLookasideList (
	PPAGED_LOOKASIDE_LIST	Lookaside
	)
{
  PVOID Entry;

  /* Try to obtain an entry from the lookaside list. If that fails, try to
     allocate a new entry with the allocate method for the lookaside list */

  Lookaside->TotalAllocates++;

//  ExAcquireFastMutex(&Lookaside->Lock);

  Entry = PopEntrySList(&Lookaside->ListHead);

//  ExReleaseFastMutex(&Lookaside->Lock);

  if (Entry)
    return Entry;

  Lookaside->AllocateMisses++;

  Entry = (*Lookaside->Allocate)(Lookaside->Type,
    Lookaside->Size,
    Lookaside->Tag);

  return Entry;
}

/*
 * @implemented
 */
VOID
STDCALL
ExDeleteNPagedLookasideList (
	PNPAGED_LOOKASIDE_LIST	Lookaside
	)
{
  KIRQL OldIrql;
  PVOID Entry;

  /* Pop all entries off the stack and release the resources allocated
     for them */
  while ((Entry = ExInterlockedPopEntrySList(
    &Lookaside->ListHead,
    &Lookaside->Lock)) != NULL)
  {
    (*Lookaside->Free)(Entry);
  }

  KeAcquireSpinLock(&ExpNonPagedLookasideListLock, &OldIrql);
  RemoveEntryList(&Lookaside->ListEntry);
  KeReleaseSpinLock(&ExpNonPagedLookasideListLock, OldIrql);
}

/*
 * @implemented
 */
VOID
STDCALL
ExDeletePagedLookasideList (
	PPAGED_LOOKASIDE_LIST	Lookaside
	)
{
  KIRQL OldIrql;
  PVOID Entry;

  /* Pop all entries off the stack and release the resources allocated
     for them */
  for (;;)
  {

//  ExAcquireFastMutex(&Lookaside->Lock);

    Entry = PopEntrySList(&Lookaside->ListHead);
    if (!Entry)
      break;

//  ExReleaseFastMutex(&Lookaside->Lock);

    (*Lookaside->Free)(Entry);
  }

  KeAcquireSpinLock(&ExpPagedLookasideListLock, &OldIrql);
  RemoveEntryList(&Lookaside->ListEntry);
  KeReleaseSpinLock(&ExpPagedLookasideListLock, OldIrql);
}

/*
 * @implemented
 */
VOID
STDCALL
ExFreeToPagedLookasideList (
	PPAGED_LOOKASIDE_LIST	Lookaside,
	PVOID			Entry
	)
{
	Lookaside->TotalFrees++;

	if (ExQueryDepthSList(&Lookaside->ListHead) >= Lookaside->MinimumDepth)
	{
		Lookaside->FreeMisses++;
		(*Lookaside->Free)(Entry);
	}
	else
	{
//  ExAcquireFastMutex(&Lookaside->Lock);
    PushEntrySList(&Lookaside->ListHead, (PSINGLE_LIST_ENTRY)Entry);
//  ExReleaseFastMutex(&Lookaside->Lock);
	}
}

/*
 * @implemented
 */
VOID
STDCALL
ExInitializeNPagedLookasideList (
	PNPAGED_LOOKASIDE_LIST	Lookaside,
	PALLOCATE_FUNCTION	Allocate,
	PFREE_FUNCTION		Free,
	ULONG			Flags,
	ULONG			Size,
	ULONG			Tag,
	USHORT			Depth)
{
  DPRINT("Initializing nonpaged lookaside list at 0x%X\n", Lookaside);

  Lookaside->TotalAllocates = 0;
  Lookaside->AllocateMisses = 0;
  Lookaside->TotalFrees = 0;
  Lookaside->FreeMisses = 0;
  Lookaside->Type = NonPagedPool;
  Lookaside->Tag = Tag;

  /* We use a field of type SINGLE_LIST_ENTRY as a link to the next entry in
     the lookaside list so we must allocate at least sizeof(SINGLE_LIST_ENTRY) */
  if (Size < sizeof(SINGLE_LIST_ENTRY))
    Lookaside->Size = sizeof(SINGLE_LIST_ENTRY);
  else
    Lookaside->Size = Size;

  if (Allocate)
    Lookaside->Allocate = Allocate;
  else
    Lookaside->Allocate = ExpDefaultAllocate;

  if (Free)
    Lookaside->Free = Free;
  else
    Lookaside->Free = ExpDefaultFree;

  ExInitializeSListHead(&Lookaside->ListHead);
  KeInitializeSpinLock(&Lookaside->Lock);

  /* Determine minimum and maximum number of entries on the lookaside list
     using the configured algorithm */
  (*ExpMinMaxRoutine)(
    NonPagedPool,
    Lookaside->Size,
    &Lookaside->MinimumDepth,
    &Lookaside->MaximumDepth);

  ExInterlockedInsertTailList(
    &ExpNonPagedLookasideListHead,
    &Lookaside->ListEntry,
    &ExpNonPagedLookasideListLock);
}

/*
 * @implemented
 */
VOID
STDCALL
ExInitializePagedLookasideList (
	PPAGED_LOOKASIDE_LIST	Lookaside,
	PALLOCATE_FUNCTION	Allocate,
	PFREE_FUNCTION		Free,
	ULONG			Flags,
	ULONG			Size,
	ULONG			Tag,
	USHORT			Depth
	)
{
  DPRINT("Initializing paged lookaside list at 0x%X\n", Lookaside);

  Lookaside->TotalAllocates = 0;
  Lookaside->AllocateMisses = 0;
  Lookaside->TotalFrees = 0;
  Lookaside->FreeMisses = 0;
  Lookaside->Type = PagedPool;
  Lookaside->Tag = Tag;

  /* We use a field of type SINGLE_LIST_ENTRY as a link to the next entry in
     the lookaside list so we must allocate at least sizeof(SINGLE_LIST_ENTRY) */
  if (Size < sizeof(SINGLE_LIST_ENTRY))
    Lookaside->Size = sizeof(SINGLE_LIST_ENTRY);
  else
    Lookaside->Size = Size;

  if (Allocate)
    Lookaside->Allocate = Allocate;
  else
    Lookaside->Allocate = ExpDefaultAllocate;

  if (Free)
    Lookaside->Free = Free;
  else
    Lookaside->Free = ExpDefaultFree;

  ExInitializeSListHead(&Lookaside->ListHead);
  //ExInitializeFastMutex(&Lookaside->Lock);

  /* Determine minimum and maximum number of entries on the lookaside list
     using the configured algorithm */
  (*ExpMinMaxRoutine)(
    PagedPool,
    Lookaside->Size,
    &Lookaside->MinimumDepth,
    &Lookaside->MaximumDepth);

  ExInterlockedInsertTailList(
    &ExpPagedLookasideListHead,
    &Lookaside->ListEntry,
    &ExpPagedLookasideListLock);
}

/* EOF */
