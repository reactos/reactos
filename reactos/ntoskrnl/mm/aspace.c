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
#define NDEBUG
#include <internal/debug.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, MmInitializeKernelAddressSpace)
#endif

/* GLOBALS ******************************************************************/

static MADDRESS_SPACE KernelAddressSpace;
FAST_MUTEX KernelAddressSpaceLock;

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
MmLockAddressSpace(PMADDRESS_SPACE AddressSpace)
{
   /*
    * Don't bother with locking if we are the first thread.
    */
   if (KeGetCurrentThread() == NULL)
   {
      return;
   }

   DPRINT("LockAddressSpace(%x)\n", AddressSpace);

   /* No need to lock the address space if we own it */
   if (AddressSpace->OwningThread == PsGetCurrentThread())
   {
      DPRINT("LockAddressSpace: We own it: %d\n", AddressSpace->LockCount);
      AddressSpace->LockCount++;
      return;
   }

   if (AddressSpace->Process)
   {
       DPRINT("LockAddressSpace: First owner (P): %x\n", PsGetCurrentThread());
       ExEnterCriticalRegionAndAcquireFastMutexUnsafe(&AddressSpace->Process->AddressCreationLock);
       AddressSpace->OwningThread = PsGetCurrentThread();
       AddressSpace->LockCount = 1;
   }
   else
   {
       DPRINT("LockAddressSpace: First owner: %x\n", PsGetCurrentThread());
       ExEnterCriticalRegionAndAcquireFastMutexUnsafe(&KernelAddressSpaceLock);
       AddressSpace->OwningThread = PsGetCurrentThread();
       AddressSpace->LockCount = 1;
   }

   DPRINT("LockAddressSpace: Done\n");
}

VOID
NTAPI
MmUnlockAddressSpace(PMADDRESS_SPACE AddressSpace)
{
   /*
    * Don't bother locking if we are the first thread.
    */
   if (KeGetCurrentThread() == NULL)
   {
      return;
   }

   DPRINT("UnlockAddressSpace(%x) -> count %d\n", AddressSpace, AddressSpace->LockCount);

   if (AddressSpace->Process)
   {
        if(--AddressSpace->LockCount == 0)
	{
	    DPRINT("UnlockAddressSpace (P): Really unlock (0)\n", AddressSpace, AddressSpace->LockCount);
	    AddressSpace->OwningThread = 0;
	    ExReleaseFastMutexUnsafeAndLeaveCriticalRegion(&AddressSpace->Process->AddressCreationLock);
	}
   }
   else
   {
        if(--AddressSpace->LockCount == 0)
	{
	    DPRINT("UnlockAddressSpace: Really unlock (0)\n", AddressSpace, AddressSpace->LockCount);
	    AddressSpace->OwningThread = 0;
	    ExReleaseFastMutexUnsafeAndLeaveCriticalRegion(&KernelAddressSpaceLock);
	}
   }
   DPRINT("UnlockAddressSpace: Done\n");
}

VOID
INIT_FUNCTION
NTAPI
MmInitializeKernelAddressSpace(VOID)
{
   MmInitializeAddressSpace(NULL, &KernelAddressSpace);
}

PMADDRESS_SPACE
NTAPI
MmGetCurrentAddressSpace(VOID)
{
   return((PMADDRESS_SPACE)&(PsGetCurrentProcess())->VadRoot);
}

PMADDRESS_SPACE
NTAPI
MmGetKernelAddressSpace(VOID)
{
   return(&KernelAddressSpace);
}

NTSTATUS
NTAPI
MmInitializeAddressSpace(PEPROCESS Process,
                         PMADDRESS_SPACE AddressSpace)
{
   AddressSpace->MemoryAreaRoot = NULL;
   if (Process)
   {
       ExInitializeFastMutex(&Process->AddressCreationLock);
   }
   else
   {
        ExInitializeFastMutex(&KernelAddressSpaceLock);
   }
   if (Process != NULL)
   {
      AddressSpace->LowestAddress = MM_LOWEST_USER_ADDRESS;
   }
   else
   {
      AddressSpace->LowestAddress = MmSystemRangeStart;
   }
   AddressSpace->Process = Process;
   AddressSpace->OwningThread = NULL;
   AddressSpace->LockCount = 0;
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
NTAPI
MmDestroyAddressSpace(PMADDRESS_SPACE AddressSpace)
{
   if (AddressSpace->PageTableRefCountTable != NULL)
   {
      ExFreePool(AddressSpace->PageTableRefCountTable);
   }
   return(STATUS_SUCCESS);
}

/* EOF */
