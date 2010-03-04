/*
 * COPYRIGHT:       See COPYING in the top directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/rmap.c
 * PURPOSE:         Kernel memory managment functions
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, MmInitializeRmapList)
#endif

/* TYPES ********************************************************************/

typedef struct _MM_RMAP_ENTRY
{
   struct _MM_RMAP_ENTRY* Next;
   PEPROCESS Process;
   PVOID Address;
#if DBG
   PVOID Caller;
#endif
}
MM_RMAP_ENTRY, *PMM_RMAP_ENTRY;

/* GLOBALS ******************************************************************/

static FAST_MUTEX RmapListLock;
static FAST_MUTEX GlobalPageOperation;
static NPAGED_LOOKASIDE_LIST RmapLookasideList;
extern KEVENT MmWaitPageEvent;

/* FUNCTIONS ****************************************************************/

VOID
INIT_FUNCTION
NTAPI
MmInitializeRmapList(VOID)
{
   ExInitializeFastMutex(&RmapListLock);
   ExInitializeFastMutex(&GlobalPageOperation);
   ExInitializeNPagedLookasideList (&RmapLookasideList,
                                    NULL,
                                    NULL,
                                    0,
                                    sizeof(MM_RMAP_ENTRY),
                                    TAG_RMAP,
                                    50);
}

NTSTATUS
NTAPI
MmWritePagePhysicalAddress(PFN_TYPE Page)
{
   BOOLEAN ProcRef = FALSE, DirtyPage = FALSE;
   PMM_RMAP_ENTRY entry;
   PMM_SECTION_SEGMENT Segment;
   LARGE_INTEGER FileOffset;
   ULONG Protect;
   PVOID Address;
   PEPROCESS Process = NULL;
   NTSTATUS Status = STATUS_SUCCESS;
   KIRQL OldIrql;

   ExAcquireFastMutex(&GlobalPageOperation);
   if ((Segment = MmGetSectionAssociation(Page, &FileOffset)))
   {
	   if (Page == MM_WAIT_ENTRY)
	   {
		   DPRINT("In progress page out %x\n", Page);
		   ExReleaseFastMutex(&GlobalPageOperation);
		   return STATUS_UNSUCCESSFUL;
	   }

	   OldIrql = KfRaiseIrql(DISPATCH_LEVEL);

	   entry = MmGetRmapListHeadPage(Page);
	   
	   DPRINT("Entry %x\n", entry);
	   while (entry != NULL)
	   {
		   Process = entry->Process;
		   Address = entry->Address;
		   
		   DPRINT("Process %x Address %x Page %x\n", Process, Address, Page);

		   Protect = MmGetPageProtect(Process, Address);
		   if (Protect == PAGE_READWRITE)
			   MmSetPageProtect(Process, Address, PAGE_READONLY);
		   else if (Protect == PAGE_EXECUTE_READWRITE)
			   MmSetPageProtect(Process, Address, PAGE_EXECUTE_READ);

		   // When we're not in a section and all rmaps evicted, we win
		   
		   entry = entry->Next;
	   }
	   
	   KfLowerIrql(OldIrql);

	   /*
		* Do the actual page out work.
		*/
	   Status = MmWritePageSectionView(Segment, &FileOffset, Page, DirtyPage);
   }
   
   if (ProcRef)
   {
      ObDereferenceObject(Process);
   }
   ExReleaseFastMutex(&GlobalPageOperation);
   return(Status);
}

NTSTATUS
NTAPI
MmPageOutPhysicalAddress(PFN_TYPE Page)
{
   BOOLEAN ProcRef = FALSE;
   PFN_TYPE SectionPage = 0;
   PMM_RMAP_ENTRY entry;
   PMM_SECTION_SEGMENT Segment = NULL;
   LARGE_INTEGER FileOffset;
   PMEMORY_AREA MemoryArea;
   PMMSUPPORT AddressSpace = MmGetKernelAddressSpace();
   BOOLEAN Dirty = FALSE;
   PVOID Address;
   PEPROCESS Process = NULL;
   ULONG Evicted = FALSE;
   NTSTATUS Status = STATUS_SUCCESS;
   MM_REQUIRED_RESOURCES Resources = { 0 };

   DPRINT1("Page out %x (ref ct %x)\n", Page, MmGetReferenceCountPage(Page));
   ExAcquireFastMutex(&GlobalPageOperation);
   ExAcquireFastMutex(&RmapListLock);
   entry = MmGetRmapListHeadPage(Page);
   if (entry == NULL)
   {
      ExReleaseFastMutex(&RmapListLock);
	  ExReleaseFastMutex(&GlobalPageOperation);
      return(STATUS_UNSUCCESSFUL);
   }
   Process = entry->Process;
   Address = entry->Address;
   if ((((ULONG_PTR)Address) & 0xFFF) != 0)
   {
      KeBugCheck(MEMORY_MANAGEMENT);
   }
   ExReleaseFastMutex(&RmapListLock);

   Dirty = MmIsDirtyPageRmap(Page);

   if ((Segment = MmGetSectionAssociation(Page, &FileOffset)))
   {
	   DPRINT1("Withdrawing page (%x) %x:%x\n", Page, Segment, FileOffset.LowPart);
	   SectionPage = MmWithdrawSectionPage(Segment, &FileOffset, &Dirty);
	   DPRINT1("SectionPage %x\n", SectionPage);
	   if (SectionPage == MM_WAIT_ENTRY)
	   {
		   DPRINT1("In progress page out %x\n", SectionPage);
		   ExReleaseFastMutex(&GlobalPageOperation);
		   return STATUS_UNSUCCESSFUL;
	   }
	   ASSERT(SectionPage == Page);
	   Resources.State = Dirty ? 1 : 0;
   }
   else
   {
	   DPRINT("No segment association for %x\n", Page);
   }

   DPRINT1("Trying to unmap all instances of %x\n", Page);
   ExAcquireFastMutex(&RmapListLock);
   entry = MmGetRmapListHeadPage(Page);

   DPRINT1("Entry %x\n", entry);
   while (entry != NULL && NT_SUCCESS(Status))
   {
	   Process = entry->Process;
	   Address = entry->Address;

	   DPRINT1("Process %x Address %x Page %x\n", Process, Address, Page);

	   if (Process && Address < MmSystemRangeStart)
	   {
		   // Make sure we don't try to page out part of an exiting process
		   if (PsIsProcessExiting(Process))
		   {
			   DPRINT("bail\n");
			   ExReleaseFastMutex(&RmapListLock);
			   goto bail;
		   }
		   Status = ObReferenceObject(Process);
		   if (!NT_SUCCESS(Status))
		   {
			   DPRINT("bail\n");
			   ExReleaseFastMutex(&RmapListLock);
			   goto bail;
		   }
		   ProcRef = TRUE;
		   AddressSpace = &Process->Vm;
	   }
	   else
	   {
		   AddressSpace = MmGetKernelAddressSpace();
	   }
	   ExReleaseFastMutex(&RmapListLock);

	   RtlZeroMemory(&Resources, sizeof(Resources));

	   if ((((ULONG_PTR)Address) & 0xFFF) != 0)
	   {
		   KeBugCheck(MEMORY_MANAGEMENT);
	   }

	   if (!MmTryToLockAddressSpace(AddressSpace))
	   {
		   Status = STATUS_UNSUCCESSFUL;
		   goto bail;
	   }

	   do 
	   {
		   MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace, Address);
		   if (MemoryArea == NULL || 
			   MemoryArea->DeleteInProgress ||
			   !MemoryArea->PageOut)
		   {
			   Status = STATUS_UNSUCCESSFUL;
			   MmUnlockAddressSpace(AddressSpace);
			   DPRINT("bail\n");
			   goto bail;
		   }
		   
		   DPRINT1
			   ("Type %x (%x -> %x)\n", 
				MemoryArea->Type, 
				MemoryArea->StartingAddress, 
				MemoryArea->EndingAddress);
		   
		   Resources.DoAcquisition = NULL;
		   
		   Resources.Page[0] = Page;
		   
		   if (KeGetCurrentIrql() > APC_LEVEL)
		   {
			   DPRINT1("BAD IRQL from %x\n", MemoryArea->PageOut);
		   }
		   ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
		   
		   Status = MemoryArea->PageOut
			   (AddressSpace, MemoryArea, Address, &Resources);
		   
		   if (KeGetCurrentIrql() > APC_LEVEL)
		   {
			   DPRINT1("BAD IRQL from %x\n", MemoryArea->PageOut);
		   }
		   ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
		   
		   MmUnlockAddressSpace(AddressSpace);
		   
		   if (Status == STATUS_SUCCESS + 1)
		   {
			   // Wait page ... the other guy has it, so we'll just fail for now
			   Status = STATUS_UNSUCCESSFUL;
			   goto bail;
		   }
		   else if (Status == STATUS_MORE_PROCESSING_REQUIRED)
		   {
			   DPRINT1("DoAcquisition %x\n", Resources.DoAcquisition);
			   Status = Resources.DoAcquisition(AddressSpace, MemoryArea, &Resources);
			   DPRINT1("Status %x\n", Status);
			   if (!NT_SUCCESS(Status)) 
			   {
				   DPRINT("bail\n");
				   goto bail;
			   }
			   else Status = STATUS_MM_RESTART_OPERATION;
		   }
		   
		   MmLockAddressSpace(AddressSpace);

	   } 
	   while (Status == STATUS_MM_RESTART_OPERATION);
	   Dirty |= Resources.State & 1; // Accumulate dirty

	   MmUnlockAddressSpace(AddressSpace);

	   if (ProcRef)
	   {
		   ObDereferenceObject(Process);
		   ProcRef = FALSE;
	   }
	   
	   ExAcquireFastMutex(&RmapListLock);
	   ASSERT(!MM_IS_WAIT_PTE(MmGetPfnForProcess(Process, Address)));
	   entry = MmGetRmapListHeadPage(Page);

	   // When we're not in a section and all rmaps evicted, we win
	   if (!entry && !Segment) 
		   Evicted = TRUE;

	   DPRINT1("Entry %x\n", entry);
   }

   ExReleaseFastMutex(&RmapListLock);

bail:
   DPRINT1("BAIL %x\n", Status);

   if (Segment)
   {
	   DPRINT1("About to finalize section page %x (%x:%x) %s\n", Page, Segment, FileOffset.LowPart, Dirty ? "dirty" : "clean");

	   if (!NT_SUCCESS(Status) ||
		   !NT_SUCCESS
		   (Status = MmFinalizeSectionPageOut
			(Segment, &FileOffset, Page, Dirty)))
	   {
		   DPRINT1
			   ("Failed to page out %x, replacing %x at %x in segment %x\n",
				SectionPage, FileOffset.LowPart, Segment);
		   MmLockSectionSegment(Segment);
		   MiSetPageEntrySectionSegment(Segment, &FileOffset, Dirty ? DIRTY_SSE(MAKE_PFN_SSE(SectionPage)) : MAKE_PFN_SSE(SectionPage));
		   MmUnlockSectionSegment(Segment);
		   KeSetEvent(&MmWaitPageEvent, IO_NO_INCREMENT, FALSE);
	   }
	   else
	   {
		   Evicted = TRUE;
	   }
   }

   if (ProcRef)
   {
	   DPRINT1("Dereferencing process...\n");
	   ObDereferenceObject(Process);
   }

   ExReleaseFastMutex(&GlobalPageOperation);
   DPRINT1("%s %x %x\n", Evicted ? "Evicted" : "Spared", Page, Status);
   return Evicted ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

VOID
NTAPI
MmSetCleanAllRmaps(PFN_TYPE Page)
{
   PMM_RMAP_ENTRY current_entry;

   ExAcquireFastMutex(&RmapListLock);
   current_entry = MmGetRmapListHeadPage(Page);
   if (current_entry == NULL)
   {
      DPRINT1("MmIsDirtyRmap: No rmaps.\n");
      KeBugCheck(MEMORY_MANAGEMENT);
   }
   while (current_entry != NULL)
   {
      MmSetCleanPage(current_entry->Process, current_entry->Address);
      current_entry = current_entry->Next;
   }
   ExReleaseFastMutex(&RmapListLock);
}

VOID
NTAPI
MmSetDirtyAllRmaps(PFN_TYPE Page)
{
   PMM_RMAP_ENTRY current_entry;

   ExAcquireFastMutex(&RmapListLock);
   current_entry = MmGetRmapListHeadPage(Page);
   if (current_entry == NULL)
   {
      DPRINT1("MmIsDirtyRmap: No rmaps.\n");
      KeBugCheck(MEMORY_MANAGEMENT);
   }
   while (current_entry != NULL)
   {
      MmSetDirtyPage(current_entry->Process, current_entry->Address);
      current_entry = current_entry->Next;
   }
   ExReleaseFastMutex(&RmapListLock);
}

BOOLEAN
NTAPI
MmIsDirtyPageRmap(PFN_TYPE Page)
{
   PMM_RMAP_ENTRY current_entry;

   ExAcquireFastMutex(&RmapListLock);
   current_entry = MmGetRmapListHeadPage(Page);
   if (current_entry == NULL)
   {
      ExReleaseFastMutex(&RmapListLock);
      return(FALSE);
   }
   while (current_entry != NULL)
   {
      if (MmIsDirtyPage(current_entry->Process, current_entry->Address))
      {
         ExReleaseFastMutex(&RmapListLock);
         return(TRUE);
      }
      current_entry = current_entry->Next;
   }
   ExReleaseFastMutex(&RmapListLock);
   return(FALSE);
}

VOID
NTAPI
MmInsertRmap(PFN_TYPE Page, PEPROCESS Process,
             PVOID Address)
{
   PMM_RMAP_ENTRY current_entry;
   PMM_RMAP_ENTRY new_entry;
   ULONG PrevSize;

   DPRINT1("Insert Rmap: %x %x:%x\n", Page, Process, Address);

   Address = (PVOID)PAGE_ROUND_DOWN(Address);

   new_entry = ExAllocateFromNPagedLookasideList(&RmapLookasideList);
   if (new_entry == NULL)
   {
      KeBugCheck(MEMORY_MANAGEMENT);
   }
   new_entry->Address = Address;
   new_entry->Process = (PEPROCESS)Process;
#if DBG
#ifdef __GNUC__
   new_entry->Caller = __builtin_return_address(0);
#else
   new_entry->Caller = _ReturnAddress();
#endif
#endif

   if (MmGetPfnForProcess(Process, Address) != Page)
   {
      DPRINT1("Insert rmap (%d, 0x%.8X) 0x%.8X which doesn't match physical "
              "address 0x%.8X\n", Process->UniqueProcessId, Address,
              MmGetPfnForProcess(Process, Address) << PAGE_SHIFT,
              Page << PAGE_SHIFT);
      KeBugCheck(MEMORY_MANAGEMENT);
   }

   ExAcquireFastMutex(&RmapListLock);
   current_entry = MmGetRmapListHeadPage(Page);
   new_entry->Next = current_entry;
#if DBG
   while (current_entry)
   {
      if (current_entry->Address == new_entry->Address && current_entry->Process == new_entry->Process)
      {
          DbgPrint("MmInsertRmap tries to add a second rmap entry for address %p\n    current caller ",
                   current_entry->Address);
          DbgPrint("%p", new_entry->Caller);
          DbgPrint("\n    previous caller ");
          DbgPrint("%p", current_entry->Caller);
          DbgPrint("\n");
          KeBugCheck(MEMORY_MANAGEMENT);
      }
      current_entry = current_entry->Next;
   }
#endif
   MmSetRmapListHeadPage(Page, new_entry);
   ExReleaseFastMutex(&RmapListLock);
   if (Process == NULL)
   {
      Process = PsInitialSystemProcess;
   }
   if (Process)
   {
      PrevSize = InterlockedExchangeAddUL(&Process->Vm.WorkingSetSize, PAGE_SIZE);
      if (PrevSize >= Process->Vm.PeakWorkingSetSize)
      {
         Process->Vm.PeakWorkingSetSize = PrevSize + PAGE_SIZE;
      }
   }
}

VOID
NTAPI
MmDeleteAllRmaps(PFN_TYPE Page, PVOID Context,
                 VOID (*DeleteMapping)(PVOID Context, PEPROCESS Process,
                                       PVOID Address))
{
   PMM_RMAP_ENTRY current_entry;
   PMM_RMAP_ENTRY previous_entry;
   PEPROCESS Process;

   ExAcquireFastMutex(&RmapListLock);
   current_entry = MmGetRmapListHeadPage(Page);
   if (current_entry == NULL)
   {
      DPRINT1("MmDeleteAllRmaps: No rmaps.\n");
      KeBugCheck(MEMORY_MANAGEMENT);
   }
   MmSetRmapListHeadPage(Page, NULL);
   ExReleaseFastMutex(&RmapListLock);
   while (current_entry != NULL)
   {
      previous_entry = current_entry;
      current_entry = current_entry->Next;

	  DPRINT1("Delete Rmap: %x %x:%x\n", Page, previous_entry->Process, previous_entry->Address);
   
      if (DeleteMapping)
      {
         DeleteMapping(Context, previous_entry->Process,
                       previous_entry->Address);
      }
      Process = previous_entry->Process;
      ExFreeToNPagedLookasideList(&RmapLookasideList, previous_entry);
      if (Process == NULL)
      {
         Process = PsInitialSystemProcess;
      }
      if (Process)
      {
         (void)InterlockedExchangeAddUL(&Process->Vm.WorkingSetSize, -PAGE_SIZE);
      }
   }

   DPRINT1("MmDeleteAllRmaps(%x)\n", Page);
}

VOID
NTAPI
MmDeleteRmap(PFN_TYPE Page, PEPROCESS Process,
             PVOID Address)
{
   PMM_RMAP_ENTRY current_entry, previous_entry;

   DPRINT1("Delete Rmap: %x %x:%x\n", Page, Process, Address);

   Address = (PVOID)PAGE_ROUND_DOWN(Address);

   ExAcquireFastMutex(&RmapListLock);
   previous_entry = NULL;
   current_entry = MmGetRmapListHeadPage(Page);
   while (current_entry != NULL)
   {
      if (current_entry->Process == (PEPROCESS)Process &&
            current_entry->Address == Address)
      {
         if (previous_entry == NULL)
         {
            MmSetRmapListHeadPage(Page, current_entry->Next);
         }
         else
         {
            previous_entry->Next = current_entry->Next;
         }
         ExReleaseFastMutex(&RmapListLock);
         ExFreeToNPagedLookasideList(&RmapLookasideList, current_entry);
	 if (Process == NULL)
	 {
	    Process = PsInitialSystemProcess;
	 }
	 if (Process)
	 {
            (void)InterlockedExchangeAddUL(&Process->Vm.WorkingSetSize, -PAGE_SIZE);
	 }
         return;
      }
      previous_entry = current_entry;
      current_entry = current_entry->Next;
   }
   KeBugCheck(MEMORY_MANAGEMENT);
}
