/*
 * Copyright (C) 2002-2005 ReactOS Team (and the authors from the programmers section)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/anonmem.c
 * PURPOSE:         Implementing anonymous memory.
 *
 * PROGRAMMERS:     David Welch
 *                  Casper Hornstrup
 *                  KJK::Hyperion
 *                  Ge van Geldorp
 *                  Eric Kohl
 *                  Royce Mitchell III
 *                  Aleksey Bragin
 *                  Jason Filby
 *                  Art Yerkes
 *                  Gunnar Andre' Dalsnes
 *                  Filip Navara
 *                  Thomas Weidenmueller
 *                  Alex Ionescu
 *                  Trevor McCort
 *                  Steven Edwards
 */

/* INCLUDE *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
MmWritePageVirtualMemory(PMMSUPPORT AddressSpace,
                         PMEMORY_AREA MemoryArea,
                         PVOID Address)
{
   SWAPENTRY SwapEntry;
   PFN_TYPE Page;
   NTSTATUS Status;
   PEPROCESS Process = MmGetAddressSpaceOwner(AddressSpace);

   /*
    * Check for paging out from a deleted virtual memory area.
    */
   if (MemoryArea->DeleteInProgress)
   {
      return(STATUS_UNSUCCESSFUL);
   }

   Page = MmGetPfnForProcess(Process, Address);

   /*
    * Get that the page actually is dirty.
    */
   if (!MmIsDirtyPage(Process, Address))
   {
      return(STATUS_SUCCESS);
   }

   /*
    * Speculatively set the mapping to clean.
    */
   MmSetCleanPage(Process, Address);

   /*
    * If necessary, allocate an entry in the paging file for this page
    */
   SwapEntry = MmGetSavedSwapEntryPage(Page);
   if (SwapEntry == 0)
   {
      SwapEntry = MmAllocSwapPage();
      if (SwapEntry == 0)
      {
         MmSetDirtyPage(Process, Address);
         return(STATUS_PAGEFILE_QUOTA_EXCEEDED);
      }
   }

   /*
    * Write the page to the pagefile
    */
   Status = MmWriteToSwapPage(SwapEntry, Page);
   if (!NT_SUCCESS(Status))
   {
      DPRINT1("MM: Failed to write to swap page (Status was 0x%.8X)\n",
              Status);
      MmSetDirtyPage(Process, Address);
      return(STATUS_UNSUCCESSFUL);
   }

   /*
    * Otherwise we have succeeded.
    */
   MmSetSavedSwapEntryPage(Page, SwapEntry);
   return(STATUS_SUCCESS);
}

NTSTATUS
NTAPI
MmPageOutVirtualMemory(PMMSUPPORT AddressSpace,
                       PMEMORY_AREA MemoryArea,
                       PVOID Address,
					   PMM_REQUIRED_RESOURCES Required)
{
   PFN_TYPE Page;
   BOOLEAN WasDirty;
   SWAPENTRY SwapEntry;
   NTSTATUS Status;
   PEPROCESS Process = MmGetAddressSpaceOwner(AddressSpace);
    
   DPRINT("MmPageOutVirtualMemory(Address 0x%.8X) PID %d\n",
          Address, Process->UniqueProcessId);

   /*
    * Check for paging out from a deleted virtual memory area.
    */
   if (MemoryArea->DeleteInProgress)
   {
      return(STATUS_UNSUCCESSFUL);
   }

   MmDisableVirtualMapping(Process, Address, &WasDirty, &Page);

   if (Page == 0)
   {
      KeBugCheck(MEMORY_MANAGEMENT);
   }

   /*
    * If necessary, allocate an entry in the paging file for this page
    */
   SwapEntry = MmGetSavedSwapEntryPage(Page);
   if (SwapEntry == 0)
   {
      SwapEntry = MmAllocSwapPage();
      if (SwapEntry == 0)
      {
         MmShowOutOfSpaceMessagePagingFile();
         MmEnableVirtualMapping(Process, Address);
         return(STATUS_PAGEFILE_QUOTA);
      }
   }

   /*
    * Write the page to the pagefile
    */
   Status = MmWriteToSwapPage(SwapEntry, Page);
   if (!NT_SUCCESS(Status))
   {
      DPRINT1("MM: Failed to write to swap page (Status was 0x%.8X)\n",
              Status);
	  MmEnableVirtualMapping(Process, Address);
      return(STATUS_UNSUCCESSFUL);
   }

   /*
    * Otherwise we have succeeded, free the page
    */
   DPRINT("MM: Swapped out virtual memory page 0x%.8X!\n", Page << PAGE_SHIFT);
   MmDeleteAllRmaps(Page, NULL, NULL);
   MmDeleteVirtualMapping(Process, Address, FALSE, NULL, NULL);
   MmCreatePageFileMapping(Process, Address, SwapEntry);
   MmSetSavedSwapEntryPage(Page, 0);
   DPRINT("Freeing page %x\n", Page);
   MmDereferencePage(Page);
   return(STATUS_SUCCESS);
}

NTSTATUS
NTAPI
MmNotPresentFaultVirtualMemory
(PMMSUPPORT AddressSpace,
 MEMORY_AREA* MemoryArea,
 PVOID Address,
 BOOLEAN Locked,
 PMM_REQUIRED_RESOURCES Required)
/*
 * FUNCTION: Move data into memory to satisfy a page not present fault
 * ARGUMENTS:
 *      AddressSpace = Address space within which the fault occurred
 *      MemoryArea = The memory area within which the fault occurred
 *      Address = The absolute address of fault
 * RETURNS: Status
 * NOTES: This function is called with the address space lock held.
 */
{
   NTSTATUS Status;
   PMM_REGION Region;
   PEPROCESS Process = MmGetAddressSpaceOwner(AddressSpace);

   DPRINT("Not Present %x in %x:(%x-%x)\n", Address, Process, MemoryArea->StartingAddress, MemoryArea->EndingAddress);
    
   /*
    * There is a window between taking the page fault and locking the
    * address space when another thread could load the page so we check
    * that.
    */
   if (MmIsPagePresent(Process, Address))
   {
      return(STATUS_SUCCESS);
   }

   /*
    * Check for the virtual memory area being deleted.
    */
   if (MemoryArea->DeleteInProgress)
   {
	  DPRINT1("Area being deleted\n");
      return(STATUS_UNSUCCESSFUL);
   }

   /*
    * Get the segment corresponding to the virtual address
    */
   Region = MmFindRegion(MemoryArea->StartingAddress,
                         &MemoryArea->Data.VirtualMemoryData.RegionListHead,
                         Address, NULL);

   if (Region->Type == MEM_RESERVE || Region->Protect == PAGE_NOACCESS)
   {
	  DPRINT1("No access region\n");
      return(STATUS_ACCESS_VIOLATION);
   }

   /*
    * FIXME
    */
   if (Region->Protect & PAGE_GUARD)
   {
	   DPRINT1("Guard page\n");
	   return(STATUS_GUARD_PAGE_VIOLATION);
   }

   if (MmIsPagePresent(Process, Address))
   {
	   return STATUS_SUCCESS;
   }

   if (!Required->Page[0])
   {
	   Required->Consumer = MC_USER;
	   Required->Amount = 1;
	   Required->DoAcquisition = MiGetOnePage;
	   DPRINT("Allocate\n");
	   return STATUS_MORE_PROCESSING_REQUIRED;
   }

   DPRINT("Using page %x\n", Required->Page[0]);

   /*
    * Set the page. If we fail because we are out of memory then
    * try again
    */
   Status = MmCreateVirtualMapping(Process,
                                   (PVOID)PAGE_ROUND_DOWN(Address),
                                   Region->Protect,
                                   &Required->Page[0],
                                   1);

   if (!NT_SUCCESS(Status))
   {
      DPRINT1("MmCreateVirtualMapping failed, not out of memory\n");
      KeBugCheck(MEMORY_MANAGEMENT);
	  DPRINT1("Status %x\n", Status);
      return(Status);
   }

   /*
    * Add the page to the process's working set
    */
   MmInsertRmap(Required->Page[0], Process, (PVOID)PAGE_ROUND_DOWN(Address));

   /*
    * Finish the operation
    */
   return(STATUS_SUCCESS);
}

static VOID
MmModifyAttributes(PMMSUPPORT AddressSpace,
                   PVOID BaseAddress,
                   ULONG RegionSize,
                   ULONG OldType,
                   ULONG OldProtect,
                   ULONG NewType,
                   ULONG NewProtect)
/*
 * FUNCTION: Modify the attributes of a memory region
 */
{
   PEPROCESS Process = MmGetAddressSpaceOwner(AddressSpace);
    
   /*
    * If we are switching a previously committed region to reserved then
    * free any allocated pages within the region
    */
   if (NewType == MEM_RESERVE && OldType == MEM_COMMIT)
   {
      ULONG i;

      for (i=0; i < PAGE_ROUND_UP(RegionSize)/PAGE_SIZE; i++)
      {
         PFN_TYPE Page;

         if (MmIsPageSwapEntry(Process,
                               (char*)BaseAddress + (i * PAGE_SIZE)))
         {
            SWAPENTRY SwapEntry;

            MmDeletePageFileMapping(Process,
                                    (char*)BaseAddress + (i * PAGE_SIZE),
                                    &SwapEntry);
            MmFreeSwapPage(SwapEntry);
         }
         else
         {
            MmDeleteVirtualMapping(Process,
                                   (char*)BaseAddress + (i*PAGE_SIZE),
                                   FALSE, NULL, &Page);
            if (Page != 0)
            {
               SWAPENTRY SavedSwapEntry;
               SavedSwapEntry = MmGetSavedSwapEntryPage(Page);
               if (SavedSwapEntry != 0)
               {
                  MmFreeSwapPage(SavedSwapEntry);
                  MmSetSavedSwapEntryPage(Page, 0);
               }
               MmDeleteRmap(Page, Process,
                            (char*)BaseAddress + (i * PAGE_SIZE));
			   DPRINT("Freeing page %x\n", Page);
               MmDereferencePage(Page);
            }
         }
      }
   }

   /*
    * If we are changing the protection attributes of a committed region then
    * alter the attributes for any allocated pages within the region
    */
   if (NewType == MEM_COMMIT && OldType == MEM_COMMIT &&
       OldProtect != NewProtect)
   {
      ULONG i;

      for (i=0; i < PAGE_ROUND_UP(RegionSize)/PAGE_SIZE; i++)
      {
         if (MmIsPagePresent(Process,
                             (char*)BaseAddress + (i*PAGE_SIZE)))
         {
            MmSetPageProtect(Process,
                             (char*)BaseAddress + (i*PAGE_SIZE),
                             NewProtect);
         }
      }
   }
}

/*
 * @implemented
 */
NTSTATUS NTAPI
NtAllocateVirtualMemory(IN     HANDLE ProcessHandle,
                        IN OUT PVOID* UBaseAddress,
                        IN     ULONG_PTR  ZeroBits,
                        IN OUT PSIZE_T URegionSize,
                        IN     ULONG  AllocationType,
                        IN     ULONG  Protect)
/*
 * FUNCTION: Allocates a block of virtual memory in the process address space
 * ARGUMENTS:
 *      ProcessHandle = The handle of the process which owns the virtual memory
 *      BaseAddress   = A pointer to the virtual memory allocated. If you
 *                      supply a non zero value the system will try to
 *                      allocate the memory at the address supplied. It round
 *                      it down to a multiple  of the page size.
 *      ZeroBits  = (OPTIONAL) You can specify the number of high order bits
 *                      that must be zero, ensuring that the memory will be
 *                      allocated at a address below a certain value.
 *      RegionSize = The number of bytes to allocate
 *      AllocationType = Indicates the type of virtual memory you like to
 *                       allocated, can be a combination of MEM_COMMIT,
 *                       MEM_RESERVE, MEM_RESET, MEM_TOP_DOWN.
 *      Protect = Indicates the protection type of the pages allocated.
 * RETURNS: Status
 */
{
   PEPROCESS Process;
   MEMORY_AREA* MemoryArea;
   ULONG_PTR MemoryAreaLength;
   ULONG Type;
   NTSTATUS Status;
   PMMSUPPORT AddressSpace;
   PVOID BaseAddress;
   ULONG RegionSize;
   PVOID PBaseAddress;
   ULONG PRegionSize;
   ULONG MemProtection;
   PHYSICAL_ADDRESS BoundaryAddressMultiple;
   KPROCESSOR_MODE PreviousMode;

   PAGED_CODE();

   DPRINT("NtAllocateVirtualMemory(*UBaseAddress %x, "
          "ZeroBits %d, *URegionSize %x, AllocationType %x, Protect %x)\n",
          *UBaseAddress,ZeroBits,*URegionSize,AllocationType,
          Protect);

   /* Check for valid protection flags */
   MemProtection = Protect & ~(PAGE_GUARD|PAGE_NOCACHE);
   if (MemProtection != PAGE_NOACCESS &&
       MemProtection != PAGE_READONLY &&
       MemProtection != PAGE_READWRITE &&
       MemProtection != PAGE_WRITECOPY &&
       MemProtection != PAGE_EXECUTE &&
       MemProtection != PAGE_EXECUTE_READ &&
       MemProtection != PAGE_EXECUTE_READWRITE &&
       MemProtection != PAGE_EXECUTE_WRITECOPY)
   {
      DPRINT1("Invalid page protection\n");
      return STATUS_INVALID_PAGE_PROTECTION;
   }

   /* Check for valid Zero bits */
   if (ZeroBits > 21)
   {
      DPRINT1("Too many zero bits\n");
      return STATUS_INVALID_PARAMETER_3;
   }

   /* Check for valid Allocation Types */
   if ((AllocationType & ~(MEM_COMMIT | MEM_RESERVE | MEM_RESET | MEM_PHYSICAL |
                           MEM_TOP_DOWN | MEM_WRITE_WATCH)))
   {
      DPRINT1("Invalid Allocation Type\n");
      return STATUS_INVALID_PARAMETER_5;
   }

   /* Check for at least one of these Allocation Types to be set */
   if (!(AllocationType & (MEM_COMMIT | MEM_RESERVE | MEM_RESET)))
   {
      DPRINT1("No memory allocation base type\n");
      return STATUS_INVALID_PARAMETER_5;
   }

   /* MEM_RESET is an exclusive flag, make sure that is valid too */
   if ((AllocationType & MEM_RESET) && (AllocationType != MEM_RESET))
   {
      DPRINT1("Invalid use of MEM_RESET\n");
      return STATUS_INVALID_PARAMETER_5;
   }

   /* MEM_WRITE_WATCH can only be used if MEM_RESERVE is also used */
   if ((AllocationType & MEM_WRITE_WATCH) && !(AllocationType & MEM_RESERVE))
   {
      DPRINT1("MEM_WRITE_WATCH used without MEM_RESERVE\n");
      return STATUS_INVALID_PARAMETER_5;
   }

   /* MEM_PHYSICAL can only be used with MEM_RESERVE, and can only be R/W */
   if (AllocationType & MEM_PHYSICAL)
   {
      /* First check for MEM_RESERVE exclusivity */
      if (AllocationType != (MEM_RESERVE | MEM_PHYSICAL))
      {
         DPRINT1("MEM_PHYSICAL used with other flags then MEM_RESERVE or"
                 "MEM_RESERVE was not present at all\n");
         return STATUS_INVALID_PARAMETER_5;
      }

      /* Then make sure PAGE_READWRITE is used */
      if (Protect != PAGE_READWRITE)
      {
         DPRINT1("MEM_PHYSICAL used without PAGE_READWRITE\n");
         return STATUS_INVALID_PAGE_PROTECTION;
      }
   }

   PreviousMode = KeGetPreviousMode();

   _SEH2_TRY
   {
      if (PreviousMode != KernelMode)
      {
         ProbeForWritePointer(UBaseAddress);
         ProbeForWriteUlong(URegionSize);
      }
      PBaseAddress = *UBaseAddress;
      PRegionSize  = *URegionSize;
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
      /* Return the exception code */
      _SEH2_YIELD(return _SEH2_GetExceptionCode());
   }
   _SEH2_END;

   BoundaryAddressMultiple.QuadPart = 0;

   BaseAddress = (PVOID)PAGE_ROUND_DOWN(PBaseAddress);
   RegionSize = PAGE_ROUND_UP((ULONG_PTR)PBaseAddress + PRegionSize) -
                PAGE_ROUND_DOWN(PBaseAddress);

   /*
    * We've captured and calculated the data, now do more checks
    * Yes, MmCreateMemoryArea does similar checks, but they don't return
    * the right status codes that a caller of this routine would expect.
    */
   if ((ULONG_PTR)BaseAddress >= USER_SHARED_DATA)
   {
      DPRINT1("Virtual allocation base above User Space\n");
      return STATUS_INVALID_PARAMETER_2;
   }
   if (!RegionSize)
   {
      DPRINT1("Region size is invalid (zero)\n");
      return STATUS_INVALID_PARAMETER_4;
   }
   if ((USER_SHARED_DATA - (ULONG_PTR)BaseAddress) < RegionSize)
   {
      DPRINT1("Region size would overflow into kernel-memory\n");
      return STATUS_INVALID_PARAMETER_4;
   }

   /*
    * Copy on Write is reserved for system use. This case is a certain failure
    * but there may be other cases...needs more testing
    */
   if ((!BaseAddress || (AllocationType & MEM_RESERVE)) &&
       (Protect & (PAGE_WRITECOPY | PAGE_EXECUTE_WRITECOPY)))
   {
      DPRINT1("Copy on write is not supported by VirtualAlloc\n");
      return STATUS_INVALID_PAGE_PROTECTION;
   }


   Status = ObReferenceObjectByHandle(ProcessHandle,
                                      PROCESS_VM_OPERATION,
                                      PsProcessType,
                                      PreviousMode,
                                      (PVOID*)(&Process),
                                      NULL);
   if (!NT_SUCCESS(Status))
   {
      DPRINT("NtAllocateVirtualMemory() = %x\n",Status);
      return(Status);
   }

   Type = (AllocationType & MEM_COMMIT) ? MEM_COMMIT : MEM_RESERVE;
   DPRINT("Type %x\n", Type);

   AddressSpace = &Process->Vm;
   MmLockAddressSpace(AddressSpace);

   if (PBaseAddress != 0)
   {
      MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace, BaseAddress);

      if (MemoryArea != NULL)
      {
         MemoryAreaLength = (ULONG_PTR)MemoryArea->EndingAddress -
                            (ULONG_PTR)MemoryArea->StartingAddress;

         if (((ULONG)BaseAddress + RegionSize) > (ULONG)MemoryArea->EndingAddress)
         {
            DPRINT("BaseAddress + RegionSize %x is larger than MemoryArea's EndingAddress %x\n",
                  (ULONG)BaseAddress + RegionSize, MemoryArea->EndingAddress);

            MmUnlockAddressSpace(AddressSpace);
            ObDereferenceObject(Process);

            return STATUS_MEMORY_NOT_ALLOCATED;
         }

         if (AllocationType == MEM_RESET)
         {
            if (MmIsPagePresent(Process, BaseAddress))
            {
               /* FIXME: mark pages as not modified */
            }
            else
            {
               /* FIXME: if pages are in paging file discard them and bring in pages of zeros */
            }

            MmUnlockAddressSpace(AddressSpace);
            ObDereferenceObject(Process);

            /* MEM_RESET does not modify any attributes of region */
            return STATUS_SUCCESS;
         }

         if (MemoryArea->Type == MEMORY_AREA_VIRTUAL_MEMORY &&
             MemoryAreaLength >= RegionSize)
         {
            Status =
               MmAlterRegion(AddressSpace,
                             MemoryArea->StartingAddress,
                             &MemoryArea->Data.VirtualMemoryData.RegionListHead,
                             BaseAddress, RegionSize,
                             Type, Protect, MmModifyAttributes);
            MmUnlockAddressSpace(AddressSpace);
            ObDereferenceObject(Process);
            DPRINT("NtAllocateVirtualMemory() = %x\n",Status);

            /* Give the caller rounded BaseAddress and area length */
            if (NT_SUCCESS(Status))
            {
                *UBaseAddress = BaseAddress;
                *URegionSize = RegionSize;
                DPRINT("*UBaseAddress %x  *URegionSize %x\n", BaseAddress, RegionSize);
            }

            return(Status);
         }
         else if (MemoryAreaLength >= RegionSize)
         {
            /* Region list initialized? */
            if (MemoryArea->Data.SectionData.RegionListHead.Flink)
            {
               Status =
                  MmAlterRegion(AddressSpace,
                                MemoryArea->StartingAddress,
                                &MemoryArea->Data.SectionData.RegionListHead,
                                BaseAddress, RegionSize,
                                Type, Protect, MmModifyAttributes);
            }
            else
            {
               Status = STATUS_ACCESS_VIOLATION;
            }

            MmUnlockAddressSpace(AddressSpace);
            ObDereferenceObject(Process);
            DPRINT("NtAllocateVirtualMemory() = %x\n",Status);

            /* Give the caller rounded BaseAddress and area length */
            if (NT_SUCCESS(Status))
            {
                *UBaseAddress = BaseAddress;
                *URegionSize = RegionSize;
                DPRINT("*UBaseAddress %x  *URegionSize %x\n", BaseAddress, RegionSize);
            }

            return(Status);
         }
         else
         {
            MmUnlockAddressSpace(AddressSpace);
            ObDereferenceObject(Process);
            return(STATUS_UNSUCCESSFUL);
         }
      }
   }

   Status = MmCreateMemoryArea(AddressSpace,
                               MEMORY_AREA_VIRTUAL_MEMORY,
                               &BaseAddress,
                               RegionSize,
                               Protect,
                               &MemoryArea,
                               PBaseAddress != 0,
                               AllocationType & MEM_TOP_DOWN,
                               BoundaryAddressMultiple);

   MemoryArea->NotPresent = MmNotPresentFaultVirtualMemory;
   MemoryArea->AccessFault = NULL;
   MemoryArea->PageOut = MmPageOutVirtualMemory;

   if (!NT_SUCCESS(Status))
   {
      MmUnlockAddressSpace(AddressSpace);
      ObDereferenceObject(Process);
      DPRINT("NtAllocateVirtualMemory() = %x\n",Status);
      return(Status);
   }

   MemoryAreaLength = (ULONG_PTR)MemoryArea->EndingAddress -
                      (ULONG_PTR)MemoryArea->StartingAddress;

   MmInitializeRegion(&MemoryArea->Data.VirtualMemoryData.RegionListHead,
                      MemoryAreaLength, Type, Protect);

   if ((AllocationType & MEM_COMMIT) &&
       (Protect & (PAGE_READWRITE | PAGE_EXECUTE_READWRITE)))
   {
      const ULONG nPages = PAGE_ROUND_UP(MemoryAreaLength) >> PAGE_SHIFT;
      MmReserveSwapPages(nPages);
   }

   *UBaseAddress = BaseAddress;
   *URegionSize = MemoryAreaLength;
   DPRINT("*UBaseAddress %x  *URegionSize %x\n", BaseAddress, RegionSize);

   MmUnlockAddressSpace(AddressSpace);
   ObDereferenceObject(Process);
   return(STATUS_SUCCESS);
}

static VOID
MmFreeVirtualMemoryPage(PVOID Context,
                        MEMORY_AREA* MemoryArea,
                        PVOID Address,
                        PFN_TYPE Page,
                        SWAPENTRY SwapEntry,
                        BOOLEAN Dirty)
{
   PEPROCESS Process = (PEPROCESS)Context;

   if (Page != 0)
   {
      SWAPENTRY SavedSwapEntry;
      SavedSwapEntry = MmGetSavedSwapEntryPage(Page);
      if (SavedSwapEntry != 0)
      {
         MmFreeSwapPage(SavedSwapEntry);
         MmSetSavedSwapEntryPage(Page, 0);
      }
      MmDeleteRmap(Page, Process, Address);
	  DPRINT("Freeing page %x\n", Page);
	  ASSERT(MmGetReferenceCountPage(Page) == 1);
      MmDereferencePage(Page);
   }
   else if (SwapEntry != 0)
   {
      MmFreeSwapPage(SwapEntry);
   }
}

VOID
NTAPI
MmFreeVirtualMemory(PEPROCESS Process,
                    PMEMORY_AREA MemoryArea)
{
   PLIST_ENTRY current_entry;
   PMM_REGION current;

   DPRINT("MmFreeVirtualMemory(Process %p Address %x-%x MemoryArea %p)\n", Process,
		  MemoryArea->StartingAddress, MemoryArea->EndingAddress, MemoryArea);

   /* Mark this memory area as about to be deleted. */
   MemoryArea->DeleteInProgress = TRUE;

   /* Free all the individual segments. */
   current_entry = MemoryArea->Data.VirtualMemoryData.RegionListHead.Flink;
   while (current_entry != &MemoryArea->Data.VirtualMemoryData.RegionListHead)
   {
      current = CONTAINING_RECORD(current_entry, MM_REGION, RegionListEntry);
      current_entry = current_entry->Flink;
      ExFreePool(current);
   }

   /* Actually free the memory area. */
   MmFreeMemoryArea(&Process->Vm,
                    MemoryArea,
                    MmFreeVirtualMemoryPage,
                    (PVOID)Process);
}

/*
 * @implemented
 */
NTSTATUS NTAPI
NtFreeVirtualMemory(IN HANDLE ProcessHandle,
                    IN PVOID*  PBaseAddress,
                    IN PSIZE_T PRegionSize,
                    IN ULONG FreeType)
/*
 * FUNCTION: Frees a range of virtual memory
 * ARGUMENTS:
 *        ProcessHandle = Points to the process that allocated the virtual
 *                        memory
 *        BaseAddress = Points to the memory address, rounded down to a
 *                      multiple of the pagesize
 *        RegionSize = Limits the range to free, rounded up to a multiple of
 *                     the paging size
 *        FreeType = Can be one of the values:  MEM_DECOMMIT, or MEM_RELEASE
 * RETURNS: Status
 */
{
   MEMORY_AREA* MemoryArea;
   NTSTATUS Status;
   PEPROCESS Process;
   PMMSUPPORT AddressSpace;
   PVOID BaseAddress;
   ULONG RegionSize;

   PAGED_CODE();

   DPRINT("NtFreeVirtualMemory(ProcessHandle %x, *PBaseAddress %x, "
          "*PRegionSize %x, FreeType %x)\n",ProcessHandle,*PBaseAddress,
          *PRegionSize,FreeType);

    if (!(FreeType & (MEM_RELEASE | MEM_DECOMMIT)))
    {
        DPRINT1("Invalid FreeType\n");
        return STATUS_INVALID_PARAMETER_4;
    }

    if (ExGetPreviousMode() != KernelMode)
    {
        _SEH2_TRY
        {
            /* Probe user pointers */
            ProbeForWriteSize_t(PRegionSize);
            ProbeForWritePointer(PBaseAddress);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

   BaseAddress = (PVOID)PAGE_ROUND_DOWN((*PBaseAddress));
   RegionSize = PAGE_ROUND_UP((ULONG_PTR)(*PBaseAddress) + (*PRegionSize)) -
                PAGE_ROUND_DOWN((*PBaseAddress));

   Status = ObReferenceObjectByHandle(ProcessHandle,
                                      PROCESS_VM_OPERATION,
                                      PsProcessType,
                                      UserMode,
                                      (PVOID*)(&Process),
                                      NULL);
   if (!NT_SUCCESS(Status))
   {
      return(Status);
   }

   AddressSpace = &Process->Vm;

   MmLockAddressSpace(AddressSpace);
   MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace, BaseAddress);
   if (MemoryArea == NULL)
   {
      Status = STATUS_UNSUCCESSFUL;
      goto unlock_deref_and_return;
   }

   switch (FreeType)
   {
      case MEM_RELEASE:
         /* We can only free a memory area in one step. */
         if (MemoryArea->StartingAddress != BaseAddress ||
             MemoryArea->Type != MEMORY_AREA_VIRTUAL_MEMORY)
         {
            Status = STATUS_UNSUCCESSFUL;
            goto unlock_deref_and_return;
         }

         MmFreeVirtualMemory(Process, MemoryArea);
         Status = STATUS_SUCCESS;
         goto unlock_deref_and_return;

      case MEM_DECOMMIT:
         Status =
            MmAlterRegion(AddressSpace,
                          MemoryArea->StartingAddress,
                          (MemoryArea->Type == MEMORY_AREA_SECTION_VIEW) ?
                             &MemoryArea->Data.SectionData.RegionListHead :
                             &MemoryArea->Data.VirtualMemoryData.RegionListHead,
                          BaseAddress,
                          RegionSize,
                          MEM_RESERVE,
                          PAGE_NOACCESS,
                          MmModifyAttributes);
         goto unlock_deref_and_return;
   }

   Status = STATUS_NOT_IMPLEMENTED;

unlock_deref_and_return:

   MmUnlockAddressSpace(AddressSpace);
   ObDereferenceObject(Process);

   return(Status);
}

NTSTATUS
NTAPI
MmProtectAnonMem(PMMSUPPORT AddressSpace,
                 PMEMORY_AREA MemoryArea,
                 PVOID BaseAddress,
                 ULONG Length,
                 ULONG Protect,
                 PULONG OldProtect)
{
   PMM_REGION Region;
   NTSTATUS Status = STATUS_SUCCESS;
   ULONG LengthCount = 0;

   /* Search all Regions in MemoryArea up to Length */
   /* Every Region up to Length must be committed for success */
   for (;;)
   {
      Region = MmFindRegion(MemoryArea->StartingAddress,
                            &MemoryArea->Data.VirtualMemoryData.RegionListHead,
                            (PVOID)((ULONG_PTR)BaseAddress + (ULONG_PTR)LengthCount), NULL);

      /* If a Region was found and it is committed */
      if ((Region) && (Region->Type == MEM_COMMIT))
      {
         LengthCount += Region->Length;
         if (Length <= LengthCount) break;
         continue;
      }
      /* If Region was found and it is not commited */
      else if (Region)
      {
         Status = STATUS_NOT_COMMITTED;
         break;
      }
      /* If no Region was found at all */
      else if (LengthCount == 0)
      {
         Status = STATUS_INVALID_ADDRESS;
         break;
      }
   }

   if (NT_SUCCESS(Status))
   {
       *OldProtect = Region->Protect;
       Status = MmAlterRegion(AddressSpace, MemoryArea->StartingAddress,
                              &MemoryArea->Data.VirtualMemoryData.RegionListHead,
                              BaseAddress, Length, Region->Type, Protect,
                              MmModifyAttributes);
   }

   return (Status);
}

NTSTATUS NTAPI
MmQueryAnonMem(PMEMORY_AREA MemoryArea,
               PVOID Address,
               PMEMORY_BASIC_INFORMATION Info,
               PULONG ResultLength)
{
   PMM_REGION Region;
   PVOID RegionBase = NULL;

   Info->BaseAddress = (PVOID)PAGE_ROUND_DOWN(Address);

   Region = MmFindRegion(MemoryArea->StartingAddress,
                         &MemoryArea->Data.VirtualMemoryData.RegionListHead,
                         Address, &RegionBase);
   Info->BaseAddress = RegionBase;
   Info->AllocationBase = MemoryArea->StartingAddress;
   Info->AllocationProtect = MemoryArea->Protect;
   Info->RegionSize = Region->Length;
   Info->State = Region->Type;
   Info->Protect = Region->Protect;
   Info->Type = MEM_PRIVATE;

   *ResultLength = sizeof(MEMORY_BASIC_INFORMATION);
   return(STATUS_SUCCESS);
}

/* EOF */
