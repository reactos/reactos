/* $Id: lookas.c,v 1.5 2002/09/07 15:12:50 chorns Exp $
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

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>


/* GLOBALS *******************************************************************/

LIST_ENTRY ExpNonPagedLookasideListHead;
KSPIN_LOCK ExpNonPagedLookasideListLock;

LIST_ENTRY ExpPagedLookasideListHead;
KSPIN_LOCK ExpPagedLookasideListLock;

PLOOKASIDE_MINMAX_ROUTINE ExpMinMaxRoutine;

/* FUNCTIONS *****************************************************************/

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
  return ExAllocatePool(PoolType, NumberOfBytes);
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
}


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
    Entry = InterlockedPopEntrySList(&Lookaside->ListHead);
    if (!Entry)
      break;

    (*Lookaside->Free)(Entry);
  }

  KeAcquireSpinLock(&ExpPagedLookasideListLock, &OldIrql);
  RemoveEntryList(&Lookaside->ListEntry);
  KeReleaseSpinLock(&ExpPagedLookasideListLock, OldIrql);
}

PVOID
STDCALL
ExiAllocateFromNPagedLookasideList(
  IN PNPAGED_LOOKASIDE_LIST  Lookaside)
{
	PVOID Entry;

	Lookaside->TotalAllocates++;
  Entry = InterlockedPopEntrySList(&Lookaside->ListHead);
	if (Entry == NULL) {
		Lookaside->AllocateMisses++;
		Entry = (Lookaside->Allocate)(Lookaside->Type, Lookaside->Size, Lookaside->Tag);
	}
  return Entry;
}

PVOID
STDCALL
ExiAllocateFromPagedLookasideList(
  IN PPAGED_LOOKASIDE_LIST  Lookaside)
{
  PVOID Entry;

  Lookaside->TotalAllocates++;
  Entry = InterlockedPopEntrySList(&Lookaside->ListHead);
  if (Entry == NULL) {
    Lookaside->AllocateMisses++;
    Entry = (Lookaside->Allocate)(Lookaside->Type,
      Lookaside->Size, Lookaside->Tag);
  }
  return Entry;
}

VOID
STDCALL
ExiFreeToNPagedLookasideList(
  IN PNPAGED_LOOKASIDE_LIST  Lookaside,
  IN PVOID  Entry)
{
  Lookaside->TotalFrees++;
	if (ExQueryDepthSList(&Lookaside->ListHead) >= Lookaside->Depth) {
		Lookaside->FreeMisses++;
		(Lookaside->Free)(Entry);
  } else {
		InterlockedPushEntrySList(&Lookaside->ListHead,
      (PSLIST_ENTRY)Entry);
	}
}

VOID
STDCALL
ExiFreeToPagedLookasideList (
	PPAGED_LOOKASIDE_LIST	Lookaside,
	PVOID			Entry
	)
{
  Lookaside->TotalFrees++;
  if (ExQueryDepthSList(&Lookaside->ListHead) >= Lookaside->Depth) {
    Lookaside->FreeMisses++;
    (Lookaside->Free)(Entry);
  } else {
    InterlockedPushEntrySList(&Lookaside->ListHead, (PSINGLE_LIST_ENTRY)Entry);
  }
}

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

  ExInterlockedInsertTailList(
    &ExpNonPagedLookasideListHead,
    &Lookaside->ListEntry,
    &ExpNonPagedLookasideListLock);
}

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

  ExInterlockedInsertTailList(
    &ExpPagedLookasideListHead,
    &Lookaside->ListEntry,
    &ExpPagedLookasideListLock);
}

/* EOF */
