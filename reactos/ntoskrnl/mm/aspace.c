/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/aspace.c
 * PURPOSE:         Manages address spaces
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

STATIC MADDRESS_SPACE KernelAddressSpace;

#define TAG_PTRC      TAG('P', 'T', 'R', 'C')

/* FUNCTIONS *****************************************************************/

VOID
MmLockAddressSpace(PMADDRESS_SPACE AddressSpace)
{
   /*
    * Don't bother with locking if we are the first thread.
    */
   if (KeGetCurrentThread() == NULL)
   {
      return;
   }
   ExAcquireFastMutex(&AddressSpace->Lock);
}

VOID
MmUnlockAddressSpace(PMADDRESS_SPACE AddressSpace)
{
   /*
    * Don't bother locking if we are the first thread.
    */
   if (KeGetCurrentThread() == NULL)
   {
      return;
   }
   ExReleaseFastMutex(&AddressSpace->Lock);
}

VOID INIT_FUNCTION
MmInitializeKernelAddressSpace(VOID)
{
   MmInitializeAddressSpace(NULL, &KernelAddressSpace);
}

PMADDRESS_SPACE MmGetCurrentAddressSpace(VOID)
{
   return(&PsGetCurrentProcess()->AddressSpace);
}

PMADDRESS_SPACE MmGetKernelAddressSpace(VOID)
{
   return(&KernelAddressSpace);
}

NTSTATUS
MmInitializeAddressSpace(PEPROCESS Process,
                         PMADDRESS_SPACE AddressSpace)
{
   AddressSpace->MemoryAreaRoot = NULL;
   ExInitializeFastMutex(&AddressSpace->Lock);
   if (Process != NULL)
   {
      AddressSpace->LowestAddress = MM_LOWEST_USER_ADDRESS;
   }
   else
   {
      AddressSpace->LowestAddress = (PVOID)KERNEL_BASE;
   }
   AddressSpace->Process = Process;
   if (Process != NULL)
   {
      ULONG Count;
      Count = MiGetUserPageDirectoryCount();
      AddressSpace->PageTableRefCountTable =
         ExAllocatePoolWithTag(NonPagedPool, Count * sizeof(USHORT),
                               TAG_PTRC);
      RtlZeroMemory(AddressSpace->PageTableRefCountTable, Count * sizeof(USHORT));
      AddressSpace->PageTableRefCountTableSize = Count;
   }
   else
   {
      AddressSpace->PageTableRefCountTable = NULL;
      AddressSpace->PageTableRefCountTableSize = 0;
   }
   return(STATUS_SUCCESS);
}

NTSTATUS
MmDestroyAddressSpace(PMADDRESS_SPACE AddressSpace)
{
   if (AddressSpace->PageTableRefCountTable != NULL)
   {
      ExFreePool(AddressSpace->PageTableRefCountTable);
   }
   return(STATUS_SUCCESS);
}

/* EOF */
