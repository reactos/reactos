/*
 * COPYRIGHT:       See COPYING in the top directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/mm.c
 * PURPOSE:         Kernel memory managment functions
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *****************************************************************/

ULONG MmUserProbeAddress = 0;
PVOID MmHighestUserAddress = NULL;
PBOOLEAN Mm64BitPhysicalAddress = FALSE;
PVOID MmSystemRangeStart = NULL;
ULONG MmReadClusterSize;

MM_STATS MmStats;

PMM_AVL_TABLE MmKernelAddressSpace;

/* FUNCTIONS ****************************************************************/

VOID
FASTCALL
MiSyncForProcessAttach(IN PKTHREAD Thread,
                       IN PEPROCESS Process)
{
    PETHREAD Ethread = CONTAINING_RECORD(Thread, ETHREAD, Tcb);

    /* Hack Sync because Mm is broken */
    MmUpdatePageDir(Process, Ethread, sizeof(ETHREAD));
    MmUpdatePageDir(Process, Ethread->ThreadsProcess, sizeof(EPROCESS));
    MmUpdatePageDir(Process,
                    (PVOID)Thread->StackLimit,
                    Thread->LargeStack ?
                    KERNEL_LARGE_STACK_SIZE : KERNEL_STACK_SIZE);
}

VOID
FASTCALL
MiSyncForContextSwitch(IN PKTHREAD Thread)
{
    PVOID Process = PsGetCurrentProcess();
    PETHREAD Ethread = CONTAINING_RECORD(Thread, ETHREAD, Tcb);

    /* Hack Sync because Mm is broken */
    MmUpdatePageDir(Process, Ethread->ThreadsProcess, sizeof(EPROCESS));
    MmUpdatePageDir(Process,
                    (PVOID)Thread->StackLimit,
                    Thread->LargeStack ?
                    KERNEL_LARGE_STACK_SIZE : KERNEL_STACK_SIZE);
}

/*
 * @implemented
 */
BOOLEAN NTAPI MmIsNonPagedSystemAddressValid(PVOID VirtualAddress)
{
   return MmIsAddressValid(VirtualAddress);
}

/*
 * @implemented
 */
BOOLEAN NTAPI MmIsAddressValid(PVOID VirtualAddress)
/*
 * FUNCTION: Checks whether the given address is valid for a read or write
 * ARGUMENTS:
 *          VirtualAddress = address to check
 * RETURNS: True if the access would be valid
 *          False if the access would cause a page fault
 * NOTES: This function checks whether a byte access to the page would
 *        succeed. Is this realistic for RISC processors which don't
 *        allow byte granular access?
 */
{
   MEMORY_AREA* MemoryArea;
   PMM_AVL_TABLE AddressSpace;

   if (VirtualAddress >= MmSystemRangeStart)
   {
      AddressSpace = MmGetKernelAddressSpace();
   }
   else
   {
      AddressSpace = &PsGetCurrentProcess()->VadRoot;
   }

   MmLockAddressSpace(AddressSpace);
   MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace,
                                            VirtualAddress);

   if (MemoryArea == NULL || MemoryArea->DeleteInProgress)
   {
      MmUnlockAddressSpace(AddressSpace);
      return(FALSE);
   }
   MmUnlockAddressSpace(AddressSpace);
   return(TRUE);
}

NTSTATUS
NTAPI
MmpAccessFault(KPROCESSOR_MODE Mode,
                  ULONG_PTR Address,
                  BOOLEAN FromMdl)
{
   PMM_AVL_TABLE AddressSpace;
   MEMORY_AREA* MemoryArea;
   NTSTATUS Status;
   BOOLEAN Locked = FromMdl;

   DPRINT("MmAccessFault(Mode %d, Address %x)\n", Mode, Address);

   if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
   {
      DPRINT1("Page fault at high IRQL was %d\n", KeGetCurrentIrql());
      return(STATUS_UNSUCCESSFUL);
   }
   if (PsGetCurrentProcess() == NULL)
   {
      DPRINT("No current process\n");
      return(STATUS_UNSUCCESSFUL);
   }

   /*
    * Find the memory area for the faulting address
    */
   if (Address >= (ULONG_PTR)MmSystemRangeStart)
   {
      /*
       * Check permissions
       */
      if (Mode != KernelMode)
      {
         DPRINT1("MmAccessFault(Mode %d, Address %x)\n", Mode, Address);
         return(STATUS_ACCESS_VIOLATION);
      }
      AddressSpace = MmGetKernelAddressSpace();
   }
   else
   {
      AddressSpace = &PsGetCurrentProcess()->VadRoot;
   }

   if (!FromMdl)
   {
      MmLockAddressSpace(AddressSpace);
   }
   do
   {
      MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace, (PVOID)Address);
      if (MemoryArea == NULL || MemoryArea->DeleteInProgress)
      {
         if (!FromMdl)
         {
            MmUnlockAddressSpace(AddressSpace);
         }
         return (STATUS_ACCESS_VIOLATION);
      }

      switch (MemoryArea->Type)
      {
         case MEMORY_AREA_SYSTEM:
            Status = STATUS_ACCESS_VIOLATION;
            break;

         case MEMORY_AREA_PAGED_POOL:
            Status = STATUS_SUCCESS;
            break;

         case MEMORY_AREA_SECTION_VIEW:
            Status = MmAccessFaultSectionView(AddressSpace,
                                              MemoryArea,
                                              (PVOID)Address,
                                              Locked);
            break;

         case MEMORY_AREA_VIRTUAL_MEMORY:
            Status = STATUS_ACCESS_VIOLATION;
            break;

         case MEMORY_AREA_SHARED_DATA:
            Status = STATUS_ACCESS_VIOLATION;
            break;

         default:
            Status = STATUS_ACCESS_VIOLATION;
            break;
      }
   }
   while (Status == STATUS_MM_RESTART_OPERATION);

   DPRINT("Completed page fault handling\n");
   if (!FromMdl)
   {
      MmUnlockAddressSpace(AddressSpace);
   }
   return(Status);
}

NTSTATUS
NTAPI
MmNotPresentFault(KPROCESSOR_MODE Mode,
                           ULONG_PTR Address,
                           BOOLEAN FromMdl)
{
   PMM_AVL_TABLE AddressSpace;
   MEMORY_AREA* MemoryArea;
   NTSTATUS Status;
   BOOLEAN Locked = FromMdl;
   PFN_TYPE Pfn;

   DPRINT("MmNotPresentFault(Mode %d, Address %x)\n", Mode, Address);

   if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
   {
      DPRINT1("Page fault at high IRQL was %d, address %x\n", KeGetCurrentIrql(), Address);
      return(STATUS_UNSUCCESSFUL);
   }

   /*
    * Find the memory area for the faulting address
    */
   if (Address >= (ULONG_PTR)MmSystemRangeStart)
   {
      /*
       * Check permissions
       */
      if (Mode != KernelMode)
      {
	 DPRINT1("Address: %x\n", Address);
         return(STATUS_ACCESS_VIOLATION);
      }
      AddressSpace = MmGetKernelAddressSpace();
   }
   else
   {
      AddressSpace = &PsGetCurrentProcess()->VadRoot;
   }

   if (!FromMdl)
   {
      MmLockAddressSpace(AddressSpace);
   }

   /*
    * Call the memory area specific fault handler
    */
   do
   {
      MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace, (PVOID)Address);
      if (MemoryArea == NULL || MemoryArea->DeleteInProgress)
      {
         if (!FromMdl)
         {
            MmUnlockAddressSpace(AddressSpace);
         }
         return (STATUS_ACCESS_VIOLATION);
      }

      switch (MemoryArea->Type)
      {
         case MEMORY_AREA_PAGED_POOL:
            {
               Status = MmCommitPagedPoolAddress((PVOID)Address, Locked);
               break;
            }

         case MEMORY_AREA_SYSTEM:
            Status = STATUS_ACCESS_VIOLATION;
            break;

         case MEMORY_AREA_SECTION_VIEW:
            Status = MmNotPresentFaultSectionView(AddressSpace,
                                                  MemoryArea,
                                                  (PVOID)Address,
                                                  Locked);
            break;

         case MEMORY_AREA_VIRTUAL_MEMORY:
         case MEMORY_AREA_PEB_OR_TEB:
            Status = MmNotPresentFaultVirtualMemory(AddressSpace,
                                                    MemoryArea,
                                                    (PVOID)Address,
                                                    Locked);
            break;

         case MEMORY_AREA_SHARED_DATA:
            Pfn = MmSharedDataPagePhysicalAddress.LowPart >> PAGE_SHIFT;
            Status =
               MmCreateVirtualMapping(PsGetCurrentProcess(),
                                      (PVOID)PAGE_ROUND_DOWN(Address),
                                      PAGE_READONLY,
                                      &Pfn,
                                      1);
            break;

	  case MEMORY_AREA_CACHE_SEGMENT:
		    Status = CcReplaceCachePage(MemoryArea, (PVOID)PAGE_ROUND_DOWN(Address));
		    break;

         default:
            Status = STATUS_ACCESS_VIOLATION;
            break;
      }
   }
   while (Status == STATUS_MM_RESTART_OPERATION);

   DPRINT("Completed page fault handling\n");
   if (!FromMdl)
   {
      MmUnlockAddressSpace(AddressSpace);
   }
   return(Status);
}

extern BOOLEAN Mmi386MakeKernelPageTableGlobal(PVOID Address);

NTSTATUS
NTAPI
MmAccessFault(IN BOOLEAN StoreInstruction,
              IN PVOID Address,
              IN KPROCESSOR_MODE Mode,
              IN PVOID TrapInformation)
{
    /* Cute little hack for ROS */
    if ((ULONG_PTR)Address >= (ULONG_PTR)MmSystemRangeStart)
    {
#ifdef _M_IX86
        /* Check for an invalid page directory in kernel mode */
        if (Mmi386MakeKernelPageTableGlobal(Address))
        {
            /* All is well with the world */
            return STATUS_SUCCESS;
        }
#endif
    }

    /* Keep same old ReactOS Behaviour */
    if (StoreInstruction)
    {
        /* Call access fault */
        return MmpAccessFault(Mode, (ULONG_PTR)Address, TrapInformation ? FALSE : TRUE);
    }
    else
    {
        /* Call not present */
        return MmNotPresentFault(Mode, (ULONG_PTR)Address, TrapInformation ? FALSE : TRUE);
    }
}

NTSTATUS
NTAPI
MmCommitPagedPoolAddress(PVOID Address, BOOLEAN Locked)
{
   NTSTATUS Status;
   PFN_TYPE AllocatedPage;
   Status = MmRequestPageMemoryConsumer(MC_PPOOL, FALSE, &AllocatedPage);
   if (!NT_SUCCESS(Status))
   {
      MmUnlockAddressSpace(MmGetKernelAddressSpace());
      Status = MmRequestPageMemoryConsumer(MC_PPOOL, TRUE, &AllocatedPage);
      MmLockAddressSpace(MmGetKernelAddressSpace());
   }
   Status =
      MmCreateVirtualMapping(NULL,
                             (PVOID)PAGE_ROUND_DOWN(Address),
                             PAGE_READWRITE,
                             &AllocatedPage,
                             1);
   if (Locked)
   {
      MmLockPage(AllocatedPage);
   }
   return(Status);
}



/* Miscellanea functions: they may fit somewhere else */

/*
 * @implemented
 */
BOOLEAN
NTAPI
MmIsRecursiveIoFault (VOID)
{
    PETHREAD Thread = PsGetCurrentThread();

    return (Thread->DisablePageFaultClustering | Thread->ForwardClusterOnly);
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
MmMapUserAddressesToPage(IN PVOID BaseAddress,
                         IN SIZE_T NumberOfBytes,
                         IN PVOID PageAddress)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
ULONG NTAPI
MmAdjustWorkingSetSize (ULONG Unknown0,
                        ULONG Unknown1,
                        ULONG Unknown2,
                        ULONG Unknown3)
{
   UNIMPLEMENTED;
   return (0);
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
MmSetAddressRangeModified (
    IN PVOID    Address,
    IN ULONG    Length
)
{
   UNIMPLEMENTED;
   return (FALSE);
}

NTSTATUS
NTAPI
NtGetWriteWatch(IN HANDLE ProcessHandle,
                IN ULONG Flags,
                IN PVOID BaseAddress,
                IN ULONG RegionSize,
                IN PVOID *UserAddressArray,
                OUT PULONG EntriesInUserAddressArray,
                OUT PULONG Granularity)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtResetWriteWatch(IN HANDLE ProcessHandle,
                 IN PVOID BaseAddress,
                 IN ULONG RegionSize)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
