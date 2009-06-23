/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/cont.c
 * PURPOSE:         Manages continuous memory
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

static VOID
MmFreeContinuousPage(PVOID Context, MEMORY_AREA* MemoryArea, PVOID Address,
                     PFN_TYPE Page, SWAPENTRY SwapEntry,
                     BOOLEAN Dirty)
{
   ASSERT(SwapEntry == 0);
   if (Page != 0)
   {
      MmReleasePageMemoryConsumer(MC_NPPOOL, Page);
   }
}

/**********************************************************************
 * NAME       EXPORTED
 * MmAllocateContiguousMemorySpecifyCache@32
 *
 * DESCRIPTION
  *  Allocates a range of physically contiguous memory
 * with a cache parameter.
 *
 * ARGUMENTS
 * NumberOfBytes
 *  Size of the memory block to allocate;
 *
 * LowestAcceptableAddress
 *  Lowest address valid for the caller.
 *
 * HighestAcceptableAddress
 *  Highest address valid for the caller.
 *
 * BoundaryAddressMultiple
 *  Address multiple not to be crossed by allocated buffer (optional).
 *
 * CacheType
 *  Type of caching to use.
 *
 * RETURN VALUE
 *  The virtual address of the memory block on success;
 * NULL on error.
 *
 * REVISIONS
 *
 * @implemented
 */
PVOID NTAPI
MmAllocateContiguousMemorySpecifyCache(IN SIZE_T NumberOfBytes,
                                       IN PHYSICAL_ADDRESS LowestAcceptableAddress OPTIONAL,
                                       IN PHYSICAL_ADDRESS HighestAcceptableAddress,
                                       IN PHYSICAL_ADDRESS BoundaryAddressMultiple OPTIONAL,
                                       IN MEMORY_CACHING_TYPE CacheType OPTIONAL)
{
   PMEMORY_AREA MArea;
   NTSTATUS Status;
   PVOID BaseAddress = NULL;
   PFN_TYPE PBase;
   ULONG Protect;
   ULONG i;

   Protect = PAGE_EXECUTE_READWRITE | PAGE_SYSTEM;
   if (CacheType == MmNonCached || CacheType == MmWriteCombined)
   {
      Protect |= PAGE_NOCACHE;
   }
   if (CacheType == MmWriteCombined)
   {
      Protect |= PAGE_WRITECOMBINE;
   }

   MmLockAddressSpace(MmGetKernelAddressSpace());
   Status = MmCreateMemoryArea(MmGetKernelAddressSpace(),
                               MEMORY_AREA_CONTINUOUS_MEMORY,
                               &BaseAddress,
                               NumberOfBytes,
                               PAGE_READWRITE,
                               &MArea,
                               FALSE,
                               0,
                               RtlConvertLongToLargeInteger(0));
   MmUnlockAddressSpace(MmGetKernelAddressSpace());

   if (!NT_SUCCESS(Status))
   {
      return(NULL);
   }
   DPRINT( "Base = %x\n", BaseAddress );
   PBase = MmGetContinuousPages(NumberOfBytes,
                                LowestAcceptableAddress,
                                HighestAcceptableAddress,
                                BoundaryAddressMultiple,
                                TRUE);
   if (PBase == 0)
   {
      MmLockAddressSpace(MmGetKernelAddressSpace());
      MmFreeMemoryArea(MmGetKernelAddressSpace(),
                       MArea,
                       NULL,
                       NULL);
      MmUnlockAddressSpace(MmGetKernelAddressSpace());
      return(NULL);
   }
   for (i = 0; i < (PAGE_ROUND_UP(NumberOfBytes) / PAGE_SIZE); i++, PBase++)
   {
      MmCreateVirtualMapping(NULL,
                             (char*)BaseAddress + (i * PAGE_SIZE),
                             Protect,
                             &PBase,
			     1);
   }
   return(BaseAddress);
}

/**********************************************************************
 * NAME       EXPORTED
 * MmAllocateContiguousMemory@12
 *
 * DESCRIPTION
 *  Allocates a range of physically contiguous cache aligned
 * memory from the non-paged pool.
 *
 * ARGUMENTS
 * NumberOfBytes
 *  Size of the memory block to allocate;
 *
 * HighestAcceptableAddress
 *  Highest address valid for the caller.
 *
 * RETURN VALUE
 *  The virtual address of the memory block on success;
 * NULL on error.
 *
 * NOTE
 *  Description taken from include/ddk/mmfuncs.h.
 *  Code taken from ntoskrnl/mm/special.c.
 *
 * REVISIONS
 *
 * @implemented
 */
PVOID NTAPI
MmAllocateContiguousMemory (IN ULONG NumberOfBytes,
                            IN PHYSICAL_ADDRESS HighestAcceptableAddress)
{
   return MmAllocateContiguousMemorySpecifyCache(NumberOfBytes,
                                                 RtlConvertLongToLargeInteger(0),
                                                 HighestAcceptableAddress,
                                                 RtlConvertLongToLargeInteger(0),
                                                 MmCached);
}


/**********************************************************************
 * NAME       EXPORTED
 * MmFreeContiguousMemory@4
 *
 * DESCRIPTION
 * Releases a range of physically contiguous memory allocated
 * with MmAllocateContiguousMemory.
 *
 * ARGUMENTS
 * BaseAddress
 *  Virtual address of the memory to be freed.
 *
 * RETURN VALUE
 *  None.
 *
 * NOTE
 *  Description taken from include/ddk/mmfuncs.h.
 *  Code taken from ntoskrnl/mm/special.c.
 *
 * REVISIONS
 *
 * @implemented
 */
VOID NTAPI
MmFreeContiguousMemory(IN PVOID BaseAddress)
{
   MmLockAddressSpace(MmGetKernelAddressSpace());
   MmFreeMemoryAreaByPtr(MmGetKernelAddressSpace(),
                         BaseAddress,
                         MmFreeContinuousPage,
                         NULL);
   MmUnlockAddressSpace(MmGetKernelAddressSpace());
}

/**********************************************************************
 * NAME       EXPORTED
 * MmFreeContiguousMemorySpecifyCache@12
 *
 * DESCRIPTION
 * Releases a range of physically contiguous memory allocated
 * with MmAllocateContiguousMemorySpecifyCache.
 *
 * ARGUMENTS
 * BaseAddress
 *  Virtual address of the memory to be freed.
 *
 * NumberOfBytes
 *  Size of the memory block to free.
 *
 * CacheType
 *  Type of caching used.
 *
 * RETURN VALUE
 *  None.
 *
 * REVISIONS
 *
 * @implemented
 */
VOID NTAPI
MmFreeContiguousMemorySpecifyCache(IN PVOID BaseAddress,
                                   IN ULONG NumberOfBytes,
                                   IN MEMORY_CACHING_TYPE CacheType)
{
   MmLockAddressSpace(MmGetKernelAddressSpace());
   MmFreeMemoryAreaByPtr(MmGetKernelAddressSpace(),
                         BaseAddress,
                         MmFreeContinuousPage,
                         NULL);
   MmUnlockAddressSpace(MmGetKernelAddressSpace());
}


/* EOF */
