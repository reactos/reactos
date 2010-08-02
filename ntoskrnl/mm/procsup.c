/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/procsup.c
 * PURPOSE:         Memory functions related to Processes
 *
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
MmDeleteProcessAddressSpace(PEPROCESS Process)
{
   PVOID Address;
   PMEMORY_AREA MemoryArea;

   DPRINT("MmDeleteProcessAddressSpace(Process %x (%s))\n", Process,
          Process->ImageFileName);

   MmLockAddressSpace(&Process->Vm);

   while ((MemoryArea = (PMEMORY_AREA)Process->Vm.WorkingSetExpansionLinks.Flink) != NULL)
   {
      switch (MemoryArea->Type)
      {
         case MEMORY_AREA_SECTION_VIEW:
             Address = (PVOID)MemoryArea->StartingAddress;
             MmUnlockAddressSpace(&Process->Vm);
             MmUnmapViewOfSection(Process, Address);
             MmLockAddressSpace(&Process->Vm);
             break;

         case MEMORY_AREA_VIRTUAL_MEMORY:
             MmFreeVirtualMemory(Process, MemoryArea);
             break;

         case MEMORY_AREA_OWNED_BY_ARM3:
             MmFreeMemoryArea(&Process->Vm,
                              MemoryArea,
                              NULL,
                              NULL);
             break;

         default:
            KeBugCheck(MEMORY_MANAGEMENT);
      }
   }

   MmUnlockAddressSpace(&Process->Vm);

   DPRINT("Finished MmReleaseMmInfo()\n");
   return(STATUS_SUCCESS);
}

