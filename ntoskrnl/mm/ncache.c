/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/ncache.c
 * PURPOSE:         Manages non-cached memory
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/


/**********************************************************************
 * NAME       EXPORTED
 *  MmAllocateNonCachedMemory@4
 *
 * DESCRIPTION
 *  Allocates a virtual address range of noncached and cache
 * aligned memory.
 *
 * ARGUMENTS
 * NumberOfBytes
 *  Size of region to allocate.
 *
 * RETURN VALUE
 * The base address of the range on success;
 * NULL on failure.
 *
 * NOTE
 *  Description taken from include/ddk/mmfuncs.h.
 *  Code taken from ntoskrnl/mm/special.c.
 *
 * REVISIONS
 *
 * @implemented
 */
PVOID STDCALL
MmAllocateNonCachedMemory(IN ULONG NumberOfBytes)
{
   PVOID Result;
   MEMORY_AREA* marea;
   NTSTATUS Status;
   ULONG Protect = PAGE_READWRITE|PAGE_SYSTEM|PAGE_NOCACHE|PAGE_WRITETHROUGH;
   PHYSICAL_ADDRESS BoundaryAddressMultiple;

   BoundaryAddressMultiple.QuadPart = 0;
   MmLockAddressSpace(MmGetKernelAddressSpace());
   Result = NULL;
   Status = MmCreateMemoryArea (MmGetKernelAddressSpace(),
                                MEMORY_AREA_NO_CACHE,
                                &Result,
                                NumberOfBytes,
                                Protect,
                                &marea,
                                FALSE,
                                0,
                                BoundaryAddressMultiple);
   MmUnlockAddressSpace(MmGetKernelAddressSpace());

   if (!NT_SUCCESS(Status))
   {
      DPRINT1("Allocating marea for noncached mem failed with Status "
          "0x%08X\n", Status);
      return (NULL);
   }

   /* Create a virtual mapping for this memory area */
   MmMapMemoryArea(Result, NumberOfBytes, MC_NPPOOL, Protect);

   return ((PVOID)Result);
}

VOID static
MmFreeNonCachedPage(PVOID Context, MEMORY_AREA* MemoryArea, PVOID Address,
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
 * MmFreeNonCachedMemory@8
 *
 * DESCRIPTION
 * Releases a range of noncached memory allocated with
 * MmAllocateNonCachedMemory.
 *
 * ARGUMENTS
 * BaseAddress
 *  Virtual address to be freed;
 *
 * NumberOfBytes
 *  Size of the region to be freed.
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
VOID STDCALL MmFreeNonCachedMemory (IN PVOID BaseAddress,
                                    IN ULONG NumberOfBytes)
{
   MmLockAddressSpace(MmGetKernelAddressSpace());
   MmFreeMemoryAreaByPtr(MmGetKernelAddressSpace(),
                         BaseAddress,
                         MmFreeNonCachedPage,
                         NULL);
   MmUnlockAddressSpace(MmGetKernelAddressSpace());
}

/* EOF */
