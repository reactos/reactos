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
/* $Id: rmap.c,v 1.4.2.1 2002/05/13 20:37:00 chorns Exp $
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
MiTransitionAllRmaps(IN ULONG_PTR  PhysicalAddress,
  IN BOOLEAN  Reference,
  OUT PBOOLEAN  Modified  OPTIONAL)
{
  PMM_RMAP_ENTRY CurrentEntry;
  PMM_RMAP_ENTRY NextEntry;
  BOOLEAN WasDirty;
  BOOLEAN Dirty;
	
	DPRINT("MiTransitionAllRmaps(PhysicalAddress 0x%.08x)\n", PhysicalAddress);

  ExAcquireFastMutex(&MiRmapListLock);

  CurrentEntry = MmGetRmapListHeadPage(PhysicalAddress);

  assertmsg(CurrentEntry != NULL, ("MiTransitionAllRmaps: No rmaps.\n"))

  Dirty = FALSE;

  if (Reference)
    {
		  while (CurrentEntry != NULL)
		    {
          NextEntry = CurrentEntry->Next;

		      /*
		       * Reference the address space and rmap entry so they won't go away
           * while the page is in transition
			     */

          MmReferenceRmap(CurrentEntry);

          //FIXME: Deadlock here
          MmLockAddressSpace(&CurrentEntry->Process->AddressSpace);
		      MmReferenceAddressSpace(&CurrentEntry->Process->AddressSpace);

		      MmDisableVirtualMapping(CurrentEntry->Process,
		        CurrentEntry->Address,
		        &WasDirty,
		        NULL);

		      if (WasDirty)
		        {
		          Dirty = TRUE;
		        }

		      MiSetPageState(CurrentEntry->Process,
		        CurrentEntry->Address,
		        PAGE_STATE_TRANSITION);

          MmUnlockAddressSpace(&CurrentEntry->Process->AddressSpace);

		      CurrentEntry = NextEntry;
		    }
    }
	else
		{
		  while (CurrentEntry != NULL)
		    {
          NextEntry = CurrentEntry->Next;
		      /*
		       * Dereference the address space and rmap entry because the page is
           * not in transition anymore
		       */
          MmDereferenceRmap(CurrentEntry);
          MmLockAddressSpace(&CurrentEntry->Process->AddressSpace);
		      MmDereferenceAddressSpace(&CurrentEntry->Process->AddressSpace);

		      MiClearPageState(CurrentEntry->Process,
		        CurrentEntry->Address,
		        PAGE_STATE_TRANSITION);
		
		      MmEnableVirtualMapping(CurrentEntry->Process,
		        CurrentEntry->Address);

          MmUnlockAddressSpace(&CurrentEntry->Process->AddressSpace);

          CurrentEntry = NextEntry;
				}
    }

  ExReleaseFastMutex(&MiRmapListLock);

  if (Modified != NULL)
    {
      *Modified = Dirty;
    }
}


NTSTATUS
MiAbortTransition(IN ULONG_PTR  PhysicalAddress)
{
  ULONG Flags;

  DPRINT("MiAbortTransition(PhysicalAddress 0x%.08x)\n", PhysicalAddress);

  Flags = MmGetFlagsPage(PhysicalAddress);

  assertmsg(Flags != MM_PHYSICAL_PAGE_MPW, ("Page at 0x%.08x is beeing flushed to secondary storage\n",
    PhysicalAddress));

  if ((Flags != MM_PHYSICAL_PAGE_STANDBY) && (Flags != MM_PHYSICAL_PAGE_MODIFIED))
    {
      DPRINT("Page is not in transition.\n");
      assertmsg(FALSE, ("Page is not in transition.\n"));
      return STATUS_SUCCESS;
    }

  MiTransitionAllRmaps(PhysicalAddress, FALSE, NULL);

  if (Flags == MM_PHYSICAL_PAGE_STANDBY)
    {
      DPRINT("Page at 0x%.08x is on standby page list\n", PhysicalAddress);
      MmTransferOwnershipPage(PhysicalAddress, MC_USER);
      InterlockedDecrement(&MiStandbyPageListSize);
    }
	else if (Flags == MM_PHYSICAL_PAGE_MODIFIED)
		{
      DPRINT("Page at 0x%.08x is on modified page list\n", PhysicalAddress);
      MmTransferOwnershipPage(PhysicalAddress, MC_USER);
			InterlockedDecrement(&MiModifiedPageListSize);
		}
  else
	  {
      DPRINT("Page at 0x%.08x is not on any transition page list\n", PhysicalAddress);
      KeBugCheck(0);
	  }

  return STATUS_SUCCESS;
}


NTSTATUS
MiFinishTransition(IN ULONG_PTR  PhysicalAddress,
  IN BOOLEAN  Dirty)
{
  ULONG Flags;

  DPRINT("MiFinishTransition(PhysicalAddress 0x%.08x,  Dirty %d)\n",
    PhysicalAddress, Dirty);

  Flags = MmGetFlagsPage(PhysicalAddress);

  if (Flags != MM_PHYSICAL_PAGE_MPW)
		{
		  if ((Flags != MM_PHYSICAL_PAGE_STANDBY) && (Flags != MM_PHYSICAL_PAGE_MODIFIED))
			  {
		      DPRINT("Page is not in transition, enabling all rmaps.\n");
          assert(FALSE);
		      MiEnableAllRmaps(PhysicalAddress, Dirty);
		      return STATUS_SUCCESS;
			  }
		}

  /* Transition the PTEs and put the physical page back on the standby or the
     modified page list */
  MiTransitionAllRmaps(PhysicalAddress, FALSE, NULL);
  MiReclaimPage(PhysicalAddress, Dirty);
  return STATUS_SUCCESS;
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


/*
 * This routine is called from the Modified Page Writer (MPW) at DISPATCH_LEVEL
 * to prepare for a page flush. A pageop is created and saved in the Page Frame
 * Number (PFN) database. If a pageop already exists the routine returns
 * STATUS_UNSUCCESSFUL and the MPW will not try to flush the page.
 */
NTSTATUS
MmPrepareFlushPhysicalAddress(IN ULONG_PTR  PhysicalAddress)
{
  PMEMORY_AREA MemoryArea;
  PMM_RMAP_ENTRY entry;
  PMM_PAGEOP PageOp;
  PEPROCESS Process;
  PVOID Address;

  DPRINT("MmPrepareFlushPhysicalAddress(PhysicalAddress 0x%.08x)\n",
    PhysicalAddress);

 /*
  * When the page was put on the modified page list, the address space and
  * rmap entry was referenced so they won't go away now.
  * FIXME: Ensure that memory area stays too.
  */

  entry = MmGetRmapListHeadPage(PhysicalAddress);

  assertmsg(entry != NULL, ("MmPrepareFlushPhysicalAddress: No rmaps.\n"))

  Process = entry->Process;
  Address = entry->Address;

  MemoryArea = MmOpenMemoryAreaByAddress(&Process->AddressSpace, Address);
  assert(MemoryArea);

	/*
	 * Get or create a page operation
	 */
  if (MemoryArea->Type == MEMORY_AREA_SECTION_VIEW_COMMIT)
    {
      LARGE_INTEGER Offset;

      Offset.QuadPart = (ULONG_PTR)((Address - (ULONG_PTR)MemoryArea->BaseAddress)
        + MemoryArea->Data.SectionData.ViewOffset);

		  PageOp = MmGetPageOp(MemoryArea,
		    0,
				NULL,
			  MemoryArea->Data.SectionData.Segment, 
			  Offset.u.LowPart,
		    MM_PAGEOP_PAGESYNCH);
    }
	else if (MemoryArea->Type == MEMORY_AREA_VIRTUAL_MEMORY)
	  {
			PageOp = MmGetPageOp(MemoryArea,
			  Process->UniqueProcessId,
				Address,
			  NULL, 
			  0,
			  MM_PAGEOP_PAGESYNCH);
		}
  else
		{
      assertmsg(FALSE, ("Unknown type (%d) for memory area (0x%.08x)\n",
        MemoryArea->Type, MemoryArea));
		}

  MmCloseMemoryArea(MemoryArea);

  assertmsg(PageOp != NULL, ("MmGetPageOp() failed\n"));

  /* If the page operation is handled by another thread then abort */
  if (PageOp->Thread != PsGetCurrentThread())
		{
		  MmReleasePageOp(PageOp);
      DPRINT("MmPrepareFlushPhysicalAddress(PhysicalAddress 0x%.08x) STATUS_UNSUCCESSFUL\n",
        PhysicalAddress);
		  return(STATUS_UNSUCCESSFUL);
		}

  assertmsg((MmGetSavedPageOp(PhysicalAddress) == NULL),
    ("PhysicalAddress (0x%.08x) has saved page operation\n", PhysicalAddress));

  MmSetSavedPageOp(PhysicalAddress, PageOp);

  /* Keep a reference to the page operation. The MPW will release the page
     operation when the page is flushed to secondary storage */

  DPRINT("MmPrepareFlushPhysicalAddress(PhysicalAddress 0x%.08x) SUCCESS\n",
    PhysicalAddress);

  return(STATUS_SUCCESS);
}


/*
 * Flush a physical page to secondary storage. This routine retrieves the
 * page operation saved in the PFN database by MmPrepareFlushPhysicalAddress().
 * It then directs the memory area specific page flush routine to actually flush
 * the page.
 */
NTSTATUS
MmFlushPhysicalAddress(IN ULONG_PTR  PhysicalAddress)
{
  PMEMORY_AREA MemoryArea;
  PVOID Address;
  PEPROCESS Process;
  PMM_PAGEOP PageOp;
  NTSTATUS Status;
  PMM_RMAP_ENTRY entry;

  DPRINT("MmFlushPhysicalAddress(PhysicalAddress 0x%.08x)\n",
    PhysicalAddress);

  ExAcquireFastMutex(&MiRmapListLock);

  entry = MmGetRmapListHeadPage(PhysicalAddress);
  assert(entry);

  Process = entry->Process;
  Address = entry->Address;
  assertmsg(((((ULONG_PTR)Address) & (PAGESIZE - 1)) == 0), ("Address 0x%.08x is not page-aligned.\n", Address))

  //MmLockAddressSpace(&Process->AddressSpace);

  PageOp = MmGetSavedPageOp(PhysicalAddress);
  MmSetSavedPageOp(PhysicalAddress, NULL);

  assertmsg(PageOp != NULL, ("No PageOp. Flush not prepared for physical page 0x%.08x\n",
    PhysicalAddress));

  assertmsg((PageOp->Thread == PsGetCurrentThread()), ("Wrong thread handling PageOp (0x%.08x)\n",
    PageOp));

  MemoryArea = MmOpenMemoryAreaByAddress(&Process->AddressSpace, Address);
  //MmUnlockAddressSpace(&Process->AddressSpace);
  ExReleaseFastMutex(&MiRmapListLock);
  if (MemoryArea->Type == MEMORY_AREA_SECTION_VIEW_COMMIT)
    {
      DPRINT("Flushing section memory page at 0x%.08x\n", MemoryArea->BaseAddress);
#if 0
      /* Loose the data */
      Status = STATUS_UNSUCCESSFUL;
#else
      /*
       * Do the actual page flush
       */
      Status = MmFlushSectionView(&Process->AddressSpace,
        MemoryArea, 
				Address,
        PageOp);
#endif
    }
  else if (MemoryArea->Type == MEMORY_AREA_VIRTUAL_MEMORY)
    {
      DPRINT("Flushing virtual memory page at 0x%.08x\n", MemoryArea->BaseAddress);
#if 0
      Status = STATUS_UNSUCCESSFUL;
#else
      /*
       * Do the actual flushing.
       */
      Status = MmFlushVirtualMemory(&Process->AddressSpace, MemoryArea, 
				Address, PageOp);
#endif
    }
  else
		{
      assertmsg(FALSE, ("Unknown type (%d) for memory area (0x%.08x)\n",
        MemoryArea->Type, MemoryArea));
		}

  MmCloseMemoryArea(MemoryArea);

  if (NT_SUCCESS(Status))
    {
      MiFinishTransition(PhysicalAddress, FALSE);
    }
	else
		{
#if 0
      MiFinishTransition(PhysicalAddress, TRUE);
#else
      /* Loose the changes because we failed to write the data to secondary storage */
      DPRINT1("WARNING! Discarding data on page 0x%.08x.\n", PhysicalAddress);
      MiFinishTransition(PhysicalAddress, FALSE);
#endif
		}

	PageOp->Status = Status;
	KeSetEvent(&PageOp->CompletionEvent, IO_NO_INCREMENT, FALSE);
	MmReleasePageOp(PageOp);

  return(Status);
}


VOID
MmInsertRmap(IN ULONG_PTR  PhysicalAddress,
  IN PEPROCESS  Process,
  IN PVOID  Address,
  IN PRMAP_DELETE_CALLBACK  RmapDelete,
  IN PVOID  RmapDeleteContext)
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
  MmSetRmapCallback(PhysicalAddress, RmapDelete, RmapDeleteContext);
  ExReleaseFastMutex(&MiRmapListLock);
}


VOID
MmDeleteAllRmaps(IN ULONG_PTR  PhysicalAddress)
{
  PRMAP_DELETE_CALLBACK RmapDelete;
  PMM_RMAP_ENTRY Previous;
  PVOID RmapDeleteContext;
  PMM_RMAP_ENTRY Current;

  DPRINT("PhysicalAddress %x\n", PhysicalAddress);

  ExAcquireFastMutex(&MiRmapListLock);
  Current = MmGetRmapListHeadPage(PhysicalAddress);

  assertmsg(Current != NULL, ("MmDeleteAllRmaps: No rmaps.\n"))

  MmGetRmapCallback(PhysicalAddress, &RmapDelete, &RmapDeleteContext);
  while (Current != NULL)
    {
      Previous = Current;
      Current = Current->Next;

      if (RmapDelete)
				{
				  (*RmapDelete)(RmapDeleteContext,
            Previous->Process, 
						Previous->Address);
				}

      Current->ReferenceCount--;
		  assertmsg(Current->ReferenceCount == 0, ("Rmap has outstanding references (%d) for Page (0x%.08x)  "
        "Process (0x%.08x)  Addresss (0x%.08x)\n",
        Current->ReferenceCount, PhysicalAddress,
        Current->Process,
        Current->Address));

      ExFreePool(Previous);
    }
  MmSetRmapListHeadPage(PhysicalAddress, NULL);
  MmSetRmapCallback(PhysicalAddress, NULL, NULL);
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
