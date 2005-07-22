/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/mm.c
 * PURPOSE:         Kernel memory managment functions
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *****************************************************************/

extern MODULE_OBJECT NtoskrnlModuleObject;
extern MODULE_OBJECT HalModuleObject;

ULONG EXPORTED MmUserProbeAddress = 0;
PVOID EXPORTED MmHighestUserAddress = NULL;
PBOOLEAN EXPORTED Mm64BitPhysicalAddress = FALSE;
PVOID EXPORTED MmSystemRangeStart = NULL;

MM_STATS MmStats;

/* FUNCTIONS ****************************************************************/


NTSTATUS MmReleaseMemoryArea(PEPROCESS Process, PMEMORY_AREA Marea)
{
   NTSTATUS Status;

   DPRINT("MmReleaseMemoryArea(Process %x, Marea %x)\n",Process,Marea);

   DPRINT("Releasing %x between %x %x (type %d)\n",
          Marea, Marea->StartingAddress, Marea->EndingAddress,
          Marea->Type);

   switch (Marea->Type)
   {
      case MEMORY_AREA_SECTION_VIEW:
         Status = MmUnmapViewOfSection(Process, (PVOID)Marea->StartingAddress);
         ASSERT(Status == STATUS_SUCCESS);
         return(STATUS_SUCCESS);

      case MEMORY_AREA_VIRTUAL_MEMORY:
      case MEMORY_AREA_PEB_OR_TEB:
         MmFreeVirtualMemory(Process, Marea);
         break;

      case MEMORY_AREA_SHARED_DATA:
      case MEMORY_AREA_NO_ACCESS:
         Status = MmFreeMemoryArea(&Process->AddressSpace,
                                   Marea,
                                   NULL,
                                   NULL);
         break;

      case MEMORY_AREA_MDL_MAPPING:
         KEBUGCHECK(PROCESS_HAS_LOCKED_PAGES);
         break;

      default:
         KEBUGCHECK(0);
   }

   return(STATUS_SUCCESS);
}

NTSTATUS MmReleaseMmInfo(PEPROCESS Process)
{
   DPRINT("MmReleaseMmInfo(Process %x (%s))\n", Process,
          Process->ImageFileName);

   MmLockAddressSpace(&Process->AddressSpace);

   while (Process->AddressSpace.MemoryAreaRoot != NULL)
      MmReleaseMemoryArea(Process, Process->AddressSpace.MemoryAreaRoot);

   Mmi386ReleaseMmInfo(Process);

   MmUnlockAddressSpace(&Process->AddressSpace);
   MmDestroyAddressSpace(&Process->AddressSpace);

   DPRINT("Finished MmReleaseMmInfo()\n");
   return(STATUS_SUCCESS);
}

/*
 * @implemented
 */
BOOLEAN STDCALL MmIsNonPagedSystemAddressValid(PVOID VirtualAddress)
{
   return MmIsAddressValid(VirtualAddress);
}

/*
 * @implemented
 */
BOOLEAN STDCALL MmIsAddressValid(PVOID VirtualAddress)
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
   PMADDRESS_SPACE AddressSpace;

   if (VirtualAddress >= MmSystemRangeStart)
   {
      AddressSpace = MmGetKernelAddressSpace();
   }
   else
   {
      AddressSpace = &PsGetCurrentProcess()->AddressSpace;
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

NTSTATUS MmAccessFault(KPROCESSOR_MODE Mode,
                       ULONG_PTR Address,
                       BOOLEAN FromMdl)
{
   PMADDRESS_SPACE AddressSpace;
   MEMORY_AREA* MemoryArea;
   NTSTATUS Status;
   BOOLEAN Locked = FromMdl;

   DPRINT("MmAccessFault(Mode %d, Address %x)\n", Mode, Address);

   if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
   {
      CPRINT("Page fault at high IRQL was %d\n", KeGetCurrentIrql());
      return(STATUS_UNSUCCESSFUL);
   }
   if (PsGetCurrentProcess() == NULL)
   {
      CPRINT("No current process\n");
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
         DbgPrint("%s:%d\n",__FILE__,__LINE__);
         return(STATUS_UNSUCCESSFUL);
      }
      AddressSpace = MmGetKernelAddressSpace();
   }
   else
   {
      AddressSpace = &PsGetCurrentProcess()->AddressSpace;
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
         return (STATUS_UNSUCCESSFUL);
      }

      switch (MemoryArea->Type)
      {
         case MEMORY_AREA_SYSTEM:
            Status = STATUS_UNSUCCESSFUL;
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
            Status = STATUS_UNSUCCESSFUL;
            break;

         case MEMORY_AREA_SHARED_DATA:
            Status = STATUS_UNSUCCESSFUL;
            break;

         default:
            Status = STATUS_UNSUCCESSFUL;
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

NTSTATUS MmCommitPagedPoolAddress(PVOID Address, BOOLEAN Locked)
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

NTSTATUS MmNotPresentFault(KPROCESSOR_MODE Mode,
                           ULONG_PTR Address,
                           BOOLEAN FromMdl)
{
   PMADDRESS_SPACE AddressSpace;
   MEMORY_AREA* MemoryArea;
   NTSTATUS Status;
   BOOLEAN Locked = FromMdl;
   PFN_TYPE Pfn;

   DPRINT("MmNotPresentFault(Mode %d, Address %x)\n", Mode, Address);

   if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
   {
      CPRINT("Page fault at high IRQL was %d, address %x\n", KeGetCurrentIrql(), Address);
      return(STATUS_UNSUCCESSFUL);
   }
   if (PsGetCurrentProcess() == NULL)
   {
      /* Allow this! It lets us page alloc much earlier! It won't be needed 
       * after my init patch anyways
       */
      CPRINT("No current process\n");
      if (Address < (ULONG_PTR)MmSystemRangeStart)
      {
         return(STATUS_UNSUCCESSFUL);
      }
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
	 CPRINT("Address: %x\n", Address);
         return(STATUS_UNSUCCESSFUL);
      }
      AddressSpace = MmGetKernelAddressSpace();
   }
   else
   {
      AddressSpace = &PsGetCurrentProcess()->AddressSpace;
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
         return (STATUS_UNSUCCESSFUL);
      }

      switch (MemoryArea->Type)
      {
         case MEMORY_AREA_PAGED_POOL:
            {
               Status = MmCommitPagedPoolAddress((PVOID)Address, Locked);
               break;
            }

         case MEMORY_AREA_SYSTEM:
            Status = STATUS_UNSUCCESSFUL;
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
	    Pfn = MmSharedDataPagePhysicalAddress.QuadPart >> PAGE_SHIFT;
            Status =
               MmCreateVirtualMapping(PsGetCurrentProcess(),
                                      (PVOID)PAGE_ROUND_DOWN(Address),
                                      PAGE_READONLY,
                                      &Pfn,
                                      1);
            break;

         default:
            Status = STATUS_UNSUCCESSFUL;
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

/* Miscellanea functions: they may fit somewhere else */

/*
 * @unimplemented
 */
DWORD STDCALL
MmAdjustWorkingSetSize (DWORD Unknown0,
                        DWORD Unknown1,
                        DWORD Unknown2)
{
   UNIMPLEMENTED;
   return (0);
}


DWORD
STDCALL
MmDbgTranslatePhysicalAddress (
   DWORD Unknown0,
   DWORD Unknown1
)
{
   UNIMPLEMENTED;
   return (0);
}


/*
 * @unimplemented
 */
NTSTATUS
STDCALL
MmGrowKernelStack (
   DWORD Unknown0
)
{
   UNIMPLEMENTED;
   return (STATUS_NOT_IMPLEMENTED);
}


/*
 * @unimplemented
 */
BOOLEAN
STDCALL
MmSetAddressRangeModified (
    IN PVOID    Address,
    IN ULONG    Length
)
{
   UNIMPLEMENTED;
   return (FALSE);
}

/*
 * @implemented
 */
PVOID
STDCALL
MmGetSystemRoutineAddress (
    IN PUNICODE_STRING SystemRoutineName
    )
{
  PVOID ProcAddress;
  ANSI_STRING AnsiRoutineName;
  NTSTATUS Status;

  if(!NT_SUCCESS(RtlUnicodeStringToAnsiString(&AnsiRoutineName,
                                              SystemRoutineName,
                                              TRUE)))
  {
    return NULL;
  }

  Status = LdrGetProcedureAddress(NtoskrnlModuleObject.Base,
                                  &AnsiRoutineName,
                                  0,
                                  &ProcAddress);

  if(!NT_SUCCESS(Status))
  {
    Status = LdrGetProcedureAddress(HalModuleObject.Base,
                                    &AnsiRoutineName,
                                    0,
                                    &ProcAddress);
  }

  RtlFreeAnsiString(&AnsiRoutineName);

  return (NT_SUCCESS(Status) ? ProcAddress : NULL);
}

/* EOF */
