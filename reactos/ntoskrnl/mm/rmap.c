/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
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
/* $Id: rmap.c,v 1.4 2002/05/13 18:10:40 chorns Exp $
 *
 * COPYRIGHT:   See COPYING in the top directory
 * PROJECT:     ReactOS kernel 
 * FILE:        ntoskrnl/mm/rmap.c
 * PURPOSE:     kernel memory managment functions
 * PROGRAMMER:  David Welch (welch@cwcom.net)
 *              Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *              Created 27/12/01
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/mm.h>
#include <internal/ps.h>

#define NDEBUG
#include <internal/debug.h>

/* TYPES ********************************************************************/

typedef struct _MM_RMAP_ENTRY
{
  struct _MM_RMAP_ENTRY* Next;
  PEPROCESS Process;
  PVOID Address;
  ULONG ReferenceCount;
} MM_RMAP_ENTRY, *PMM_RMAP_ENTRY;

/* GLOBALS ******************************************************************/

static FAST_MUTEX MiRmapListLock;

/* FUNCTIONS ****************************************************************/

#ifdef DBG

VOID
MiValidateRmapList(struct _MM_RMAP_ENTRY*  RmapList)
{
	if (RmapList != NULL)
		{
			PMM_RMAP_ENTRY CurrentEntry = RmapList;

			while (CurrentEntry != NULL)
				{
					PMM_RMAP_ENTRY Entry = RmapList;
					ULONG Count = 0;

					assertmsg(RmapList->ReferenceCount >= 0, ("Bad reference count %d for rmap entry\n",
						RmapList->ReferenceCount));

					while (Entry != NULL)
						{
							if (Entry->Process == CurrentEntry->Process)
								{
									Count++;
								}
								Entry = Entry->Next;
						}

					assertmsg(Count == 1, ("%d rmap entries for process 0x%.08x\n",
						Count, CurrentEntry->Process));

					CurrentEntry = CurrentEntry->Next;
				}
		}
}

#endif /* DBG */


VOID
MmInitializeRmapList(VOID)
{
  ExInitializeFastMutex(&MiRmapListLock);
}


NTSTATUS
MmPageOutPhysicalAddress(ULONG_PTR  PhysicalAddress)
{
  PMM_RMAP_ENTRY entry;
  PMEMORY_AREA MemoryArea;
  ULONG Type;
  PVOID Address;
  PEPROCESS Process;
  PMM_PAGEOP PageOp;
  LARGE_INTEGER Offset;
  NTSTATUS Status;

  ExAcquireFastMutex(&MiRmapListLock);
  entry = MmGetRmapListHeadPage(PhysicalAddress);
  if (entry == NULL)
    {
      ExReleaseFastMutex(&MiRmapListLock);
      return(STATUS_UNSUCCESSFUL);
    }
  Process = entry->Process;
  Address = entry->Address;
  if ((((ULONG)Address) & 0xFFF) != 0)
    {
      KeBugCheck(0);
    }

  MmLockAddressSpace(&Process->AddressSpace);
  MemoryArea = MmOpenMemoryAreaByAddress(&Process->AddressSpace, Address);
  Type = MemoryArea->Type;
  if (Type == MEMORY_AREA_SECTION_VIEW_COMMIT)
    {
      Offset.QuadPart = (ULONG)((Address - (ULONG)MemoryArea->BaseAddress) +
	MemoryArea->Data.SectionData.ViewOffset);

      /*
       * Get or create a pageop
       */
      PageOp = MmGetPageOp(MemoryArea, 0, 0,
			   MemoryArea->Data.SectionData.Segment,
			   Offset.u.LowPart, MM_PAGEOP_PAGEOUT);
      if (PageOp == NULL)
	{
	  DPRINT1("MmGetPageOp failed\n");
	  KeBugCheck(0);
	}

      if (PageOp->Thread != PsGetCurrentThread())
	{
	  MmReleasePageOp(PageOp);
	  MmUnlockAddressSpace(&Process->AddressSpace);
	  ExReleaseFastMutex(&MiRmapListLock);
	  return(STATUS_UNSUCCESSFUL);
	}

      /*
       * Release locks now we have a page op.
       */
      MmUnlockAddressSpace(&Process->AddressSpace);
      ExReleaseFastMutex(&MiRmapListLock);

      /*
       * Do the actual page out work.
       */
      Status = MmPageOutSectionView(&Process->AddressSpace, MemoryArea,
				    Address, PageOp);
    }
  else if (Type == MEMORY_AREA_VIRTUAL_MEMORY)
    {
      PageOp = MmGetPageOp(MemoryArea, Process->UniqueProcessId,
			   Address, NULL, 0, MM_PAGEOP_PAGEOUT);
      if (PageOp->Thread != PsGetCurrentThread())
	{
	  MmReleasePageOp(PageOp);
	  MmUnlockAddressSpace(&Process->AddressSpace);
	  ExReleaseFastMutex(&MiRmapListLock);
	  return(STATUS_UNSUCCESSFUL);
	}

      /*
       * Release locks now we have a page op.
       */
      MmUnlockAddressSpace(&Process->AddressSpace);
      ExReleaseFastMutex(&MiRmapListLock);

      /*
       * Do the actual page out work.
       */
      Status = MmPageOutVirtualMemory(&Process->AddressSpace, MemoryArea,
				      Address, PageOp);
    }
  else
    {
      KeBugCheck(0);
    }
  return(Status);
}


VOID
MmReferenceRmap(IN PMM_RMAP_ENTRY  RmapEntry)
{
  InterlockedIncrement(&RmapEntry->ReferenceCount);
}


VOID
MmDereferenceRmap(IN PMM_RMAP_ENTRY  RmapEntry)
{
	InterlockedDecrement(&RmapEntry->ReferenceCount);

	assertmsg(RmapEntry->ReferenceCount > 0, ("Bad reference count (%d) for "
	  "Process (0x%.08x)  Addresss (0x%.08x)\n",
	  RmapEntry->ReferenceCount, RmapEntry->Process, RmapEntry->Address));
}


VOID
MiDisableAllRmaps(IN ULONG_PTR  PhysicalAddress,
  IN PBOOLEAN  Modified)
{
  PMM_RMAP_ENTRY CurrentEntry;
  PMM_RMAP_ENTRY NextEntry;
  BOOLEAN WasDirty;

  ExAcquireFastMutex(&MiRmapListLock);

  CurrentEntry = MmGetRmapListHeadPage(PhysicalAddress);

  assertmsg(CurrentEntry != NULL, ("MiDisableAllRmaps: No rmaps.\n"))

  *Modified = FALSE;
  while (CurrentEntry != NULL)
    {
      NextEntry = CurrentEntry->Next;

			MmLockAddressSpace(&CurrentEntry->Process->AddressSpace);
			
      MmDisableVirtualMapping(CurrentEntry->Process,
        CurrentEntry->Address,
        &WasDirty,
        NULL);

      if (WasDirty)
        {
          *Modified = TRUE;
        }

      MmUnlockAddressSpace(&CurrentEntry->Process->AddressSpace);

      CurrentEntry = NextEntry;
    }

  ExReleaseFastMutex(&MiRmapListLock);
}


VOID
MiEnableAllRmaps(IN ULONG_PTR  PhysicalAddress,
  IN BOOLEAN  Modified)
{
  PMM_RMAP_ENTRY CurrentEntry;
  PMM_RMAP_ENTRY NextEntry;

  ExAcquireFastMutex(&MiRmapListLock);

  CurrentEntry = MmGetRmapListHeadPage(PhysicalAddress);

  assertmsg(CurrentEntry != NULL, ("MiEnableAllRmaps: No rmaps.\n"))

  while (CurrentEntry != NULL)
    {
      NextEntry = CurrentEntry->Next;

			MmLockAddressSpace(&CurrentEntry->Process->AddressSpace);
			
      MmEnableVirtualMapping(CurrentEntry->Process,
        CurrentEntry->Address);

      if (Modified)
        {
          MmSetDirtyPage(CurrentEntry->Process,
            CurrentEntry->Address);
        }

      MmUnlockAddressSpace(&CurrentEntry->Process->AddressSpace);

      CurrentEntry = NextEntry;
    }

  ExReleaseFastMutex(&MiRmapListLock);
}


VOID
MiGetDirtyAllRmaps(IN ULONG_PTR  PhysicalAddress,
  OUT PBOOLEAN  Dirty)
{
  PMM_RMAP_ENTRY CurrentEntry;
  PMM_RMAP_ENTRY NextEntry;
  BOOLEAN WasDirty;

  ExAcquireFastMutex(&MiRmapListLock);

  CurrentEntry = MmGetRmapListHeadPage(PhysicalAddress);

  assertmsg(CurrentEntry != NULL, ("MiGetDirtyAllRmaps: No rmaps.\n"))

  *Dirty = FALSE;
  while (CurrentEntry != NULL)
    {
      NextEntry = CurrentEntry->Next;

			MmLockAddressSpace(&CurrentEntry->Process->AddressSpace);

      WasDirty = MmIsPageDirty(CurrentEntry->Process,
        CurrentEntry->Address);

      MmUnlockAddressSpace(&CurrentEntry->Process->AddressSpace);

      if (WasDirty)
        {
          *Dirty = TRUE;
        }

      CurrentEntry = NextEntry;
    }

  ExReleaseFastMutex(&MiRmapListLock);
}


VOID
MiSetDirtyAllRmaps(IN ULONG_PTR  PhysicalAddress,
  IN BOOLEAN  Dirty)
{
  PMM_RMAP_ENTRY CurrentEntry;
  PMM_RMAP_ENTRY NextEntry;

  ExAcquireFastMutex(&MiRmapListLock);

  CurrentEntry = MmGetRmapListHeadPage(PhysicalAddress);

  assertmsg(CurrentEntry != NULL, ("MiSetDirtyAllRmaps: No rmaps.\n"))

  while (CurrentEntry != NULL)
    {
      NextEntry = CurrentEntry->Next;

			MmLockAddressSpace(&CurrentEntry->Process->AddressSpace);
			
      if (Dirty)
        {
          MmSetDirtyPage(CurrentEntry->Process,
            CurrentEntry->Address);
        }
      else
        {
          MmSetCleanPage(CurrentEntry->Process,
            CurrentEntry->Address);
        }

      MmUnlockAddressSpace(&CurrentEntry->Process->AddressSpace);

      CurrentEntry = NextEntry;
    }

  ExReleaseFastMutex(&MiRmapListLock);
}


VOID
MiGetPageStateAllRmaps(IN ULONG_PTR  PhysicalAddress,
  IN ULONG  PageState,
  OUT PBOOLEAN  Result)
{
  PMM_RMAP_ENTRY CurrentEntry;
  PMM_RMAP_ENTRY NextEntry;
  ULONG State;

  ExAcquireFastMutex(&MiRmapListLock);

  CurrentEntry = MmGetRmapListHeadPage(PhysicalAddress);

  assertmsg(CurrentEntry != NULL, ("MiGetPageStateAllRmaps: No rmaps.\n"))

  *Result = FALSE;
  while (CurrentEntry != NULL)
    {
      NextEntry = CurrentEntry->Next;

			MmLockAddressSpace(&CurrentEntry->Process->AddressSpace);
			
      State = MiPageState(CurrentEntry->Process,
        CurrentEntry->Address,
        PageState);

      MmUnlockAddressSpace(&CurrentEntry->Process->AddressSpace);

      if (State)
        {
          *Result = TRUE;
        }

      CurrentEntry = NextEntry;
    }

  ExReleaseFastMutex(&MiRmapListLock);
}


VOID
MiClearPageStateAllRmaps(IN ULONG_PTR  PhysicalAddress,
  IN ULONG  PageState)
{
  PMM_RMAP_ENTRY CurrentEntry;
  PMM_RMAP_ENTRY NextEntry;

  ExAcquireFastMutex(&MiRmapListLock);

  CurrentEntry = MmGetRmapListHeadPage(PhysicalAddress);

  assertmsg(CurrentEntry != NULL, ("MiClearPageStateAllRmaps: No rmaps.\n"))

  while (CurrentEntry != NULL)
    {
      NextEntry = CurrentEntry->Next;

			MmLockAddressSpace(&CurrentEntry->Process->AddressSpace);
			
      MiClearPageState(CurrentEntry->Process,
        CurrentEntry->Address,
        PageState);

      MmUnlockAddressSpace(&CurrentEntry->Process->AddressSpace);

      CurrentEntry = NextEntry;
    }

  ExReleaseFastMutex(&MiRmapListLock);
}


VOID
MiSetPageStateAllRmaps(IN ULONG_PTR  PhysicalAddress,
  IN ULONG  PageState)
{
  PMM_RMAP_ENTRY CurrentEntry;
  PMM_RMAP_ENTRY NextEntry;

  ExAcquireFastMutex(&MiRmapListLock);

  CurrentEntry = MmGetRmapListHeadPage(PhysicalAddress);

  assertmsg(CurrentEntry != NULL, ("MiSetPageStateAllRmaps: No rmaps.\n"))

  while (CurrentEntry != NULL)
    {
      NextEntry = CurrentEntry->Next;

			MmLockAddressSpace(&CurrentEntry->Process->AddressSpace);

      MiSetPageState(CurrentEntry->Process,
        CurrentEntry->Address,
        PageState);

			MmUnlockAddressSpace(&CurrentEntry->Process->AddressSpace);

      CurrentEntry = NextEntry;
    }

  ExReleaseFastMutex(&MiRmapListLock);
}


VOID
MmInsertRmap(ULONG_PTR PhysicalAddress, PEPROCESS Process, PVOID Address)
{
  PMM_RMAP_ENTRY Current;
  PMM_RMAP_ENTRY New;

  Address = (PVOID)PAGE_ROUND_DOWN(Address);

  New = ExAllocatePool(NonPagedPool, sizeof(MM_RMAP_ENTRY));
  assert(New);
  New->ReferenceCount = 1;
  New->Address = Address;
  New->Process = Process;

  if (MmGetPhysicalAddressForProcess(Process, Address) !=
      (ULONG_PTR)PhysicalAddress)
    {
      DPRINT("Insert rmap (%d, 0x%.8X) 0x%.8X which doesn't match physical "
	      "address 0x%.8X\n", Process->UniqueProcessId, Address,
	      MmGetPhysicalAddressForProcess(Process, Address),
	      PhysicalAddress)
      KeBugCheck(0);
    }

  ExAcquireFastMutex(&MiRmapListLock);
  Current = MmGetRmapListHeadPage(PhysicalAddress);
  New->Next = Current;
  MmSetRmapListHeadPage(PhysicalAddress, New);
  MmSetRmapCallback(PhysicalAddress, NULL, NULL);
  ExReleaseFastMutex(&MiRmapListLock);
}

VOID
MmDeleteAllRmaps(ULONG_PTR PhysicalAddress, PVOID Context,
		 VOID (*DeleteMapping)(PVOID Context, PEPROCESS Process,
				       PVOID Address))
{
  PMM_RMAP_ENTRY current_entry;
  PMM_RMAP_ENTRY previous_entry;

  ExAcquireFastMutex(&MiRmapListLock);
  current_entry = MmGetRmapListHeadPage(PhysicalAddress);
  if (current_entry == NULL)
    {
      DPRINT1("MmDeleteAllRmaps: No rmaps.\n");
      KeBugCheck(0);
    }
  while (current_entry != NULL)
    {
      previous_entry = current_entry;
      current_entry = current_entry->Next;
      if (DeleteMapping)
	{
	  DeleteMapping(Context, previous_entry->Process,
			previous_entry->Address);
	}
      ExFreePool(previous_entry);
    }
  MmSetRmapListHeadPage(PhysicalAddress, NULL);
  ExReleaseFastMutex(&MiRmapListLock);
}


VOID
MmDeleteRmap(IN ULONG_PTR  PhysicalAddress,
  IN PEPROCESS  Process,
  IN PVOID  Address)
{
  PMM_RMAP_ENTRY Current;
  PMM_RMAP_ENTRY Previous;

  ExAcquireFastMutex(&MiRmapListLock);
  Previous = NULL;
  Current = MmGetRmapListHeadPage(PhysicalAddress);
  while (Current != NULL)
    {
      if (Current->Process == Process && Current->Address == Address)
        {
          Current->ReferenceCount--;
				  assertmsg(Current->ReferenceCount == 0, ("Rmap has outstanding references (%d) for Page (0x%.08x)  "
            "Process (0x%.08x)  Addresss (0x%.08x)\n",
            Current->ReferenceCount, PhysicalAddress, Process, Address));

				  if (Previous == NULL)
				    {
				      MmSetRmapListHeadPage(PhysicalAddress, Current->Next);
				      ExReleaseFastMutex(&MiRmapListLock);
				      ExFreePool(Current);
				    }
				  else
				    {
				      Previous->Next = Current->Next;
				      ExReleaseFastMutex(&MiRmapListLock);
				      ExFreePool(Current);
				    }
				  return;
	      }
      Previous = Current;
      Current = Current->Next;
    }
  assertmsg(FALSE, ("No rmap entry for Page (0x%.08x) Process (0x%.08x)  Addresss (0x%.08x)\n",
    PhysicalAddress, Process, Address));
}
