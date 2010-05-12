/*
 * COPYRIGHT:       See COPYING in the top directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/mmfault.c
 * PURPOSE:         Kernel memory managment functions
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#define MODULE_INVOLVED_IN_ARM3
#include "ARM3/miarm.h"

/* PRIVATE FUNCTIONS **********************************************************/

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

NTSTATUS
NTAPI
MmpAccessFault(KPROCESSOR_MODE Mode,
                  ULONG_PTR Address,
                  BOOLEAN FromMdl)
{
   PMMSUPPORT AddressSpace;
   MEMORY_AREA* MemoryArea;
   NTSTATUS Status;
   BOOLEAN Locked = FromMdl;

   DPRINT("MmAccessFault(Mode %d, Address %x)\n", Mode, Address);

   if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
   {
      DPRINT1("Page fault at high IRQL was %d\n", KeGetCurrentIrql());
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
      AddressSpace = &PsGetCurrentProcess()->Vm;
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
   PMMSUPPORT AddressSpace;
   MEMORY_AREA* MemoryArea;
   NTSTATUS Status;
   BOOLEAN Locked = FromMdl;
   extern PMMPTE MmSharedUserDataPte;

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
      AddressSpace = &PsGetCurrentProcess()->Vm;
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
              *MiAddressToPte(USER_SHARED_DATA) = *MmSharedUserDataPte;
              Status = STATUS_SUCCESS;
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
    PMEMORY_AREA MemoryArea;

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
    
    /* 
     * Check if this is an ARM3 memory area or if there's no memory area at all.
     * The latter can happen early in the boot cycle when ARM3 paged pool is in
     * use before having defined the memory areas proper.
     * A proper fix would be to define memory areas in the ARM3 code, but we want
     * to avoid adding this ReactOS-specific construct to ARM3 code.
     * Either way, in the future, as ReactOS-paged pool is eliminated, this hack
     * can go away.
     */
    MemoryArea = MmLocateMemoryAreaByAddress(MmGetKernelAddressSpace(), Address);
    if ((!(MemoryArea) && ((ULONG_PTR)Address >= (ULONG_PTR)MmSystemRangeStart)) ||
        ((MemoryArea) && (MemoryArea->Type == MEMORY_AREA_OWNED_BY_ARM3)))
    {
        //
        // Hand it off to more competent hands...
        //
        DPRINT1("ARM3 fault\n");
        return MmArmAccessFault(StoreInstruction, Address, Mode, TrapInformation);
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
   return(Status);
}
