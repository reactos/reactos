/* $Id: cont.c,v 1.26 2003/05/17 15:28:58 ekohl Exp $
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/cont.c
 * PURPOSE:         Manages continuous memory
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/mm.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID STATIC
MmFreeContinuousPage(PVOID Context, MEMORY_AREA* MemoryArea, PVOID Address, 
		     PHYSICAL_ADDRESS PhysAddr, SWAPENTRY SwapEntry, 
		     BOOLEAN Dirty)
{
  assert(SwapEntry == 0);
  if (PhysAddr.QuadPart != 0)
    {
      MmReleasePageMemoryConsumer(MC_NPPOOL, PhysAddr);
    }
}

PVOID STDCALL
MmAllocateContiguousAlignedMemory(IN ULONG NumberOfBytes,
			          IN PHYSICAL_ADDRESS HighestAcceptableAddress,
				  IN ULONG Alignment)
{
   PMEMORY_AREA MArea;
   NTSTATUS Status;
   PVOID BaseAddress = 0;
   PHYSICAL_ADDRESS PBase;
   ULONG i;
   
   MmLockAddressSpace(MmGetKernelAddressSpace());
   Status = MmCreateMemoryArea(NULL,
			       MmGetKernelAddressSpace(),
			       MEMORY_AREA_CONTINUOUS_MEMORY,
			       &BaseAddress,
			       NumberOfBytes,
			       0,
			       &MArea,
			       FALSE,
			       FALSE);
   MmUnlockAddressSpace(MmGetKernelAddressSpace());

   if (!NT_SUCCESS(Status))
     {
	return(NULL);
     }
   DPRINT( "Base = %x\n", BaseAddress );
   PBase = MmGetContinuousPages(NumberOfBytes,
				HighestAcceptableAddress,
				Alignment);
   if (PBase.QuadPart == 0LL)
     {
       MmLockAddressSpace(MmGetKernelAddressSpace());
       MmFreeMemoryArea(MmGetKernelAddressSpace(),
			BaseAddress,
			0,
			NULL,
			NULL);
       MmUnlockAddressSpace(MmGetKernelAddressSpace());
       return(NULL);
     }
   for (i = 0; i < (PAGE_ROUND_UP(NumberOfBytes) / 4096); i++)
     {
	MmCreateVirtualMapping(NULL,
			       BaseAddress + (i * 4096),
			       PAGE_EXECUTE_READWRITE | PAGE_SYSTEM,
			       (LARGE_INTEGER)(PBase.QuadPart + (i * 4096)),
			       TRUE);
     }
   return(BaseAddress);
}

/**********************************************************************
 * NAME							EXPORTED
 *	MmAllocateContiguousMemory@12
 *
 * DESCRIPTION
 * 	Allocates a range of physically contiguous cache aligned
 *	memory from the non-paged pool.
 *	
 * ARGUMENTS
 *	NumberOfBytes
 *		Size of the memory block to allocate;
 *		
 *	HighestAcceptableAddress
 *		Highest address valid for the caller.
 *		
 * RETURN VALUE
 * 	The virtual address of the memory block on success;
 *	NULL on error.
 *
 * NOTE
 * 	Description taken from include/ddk/mmfuncs.h.
 * 	Code taken from ntoskrnl/mm/special.c.
 *
 * REVISIONS
 *
 */
PVOID STDCALL 
MmAllocateContiguousMemory (IN ULONG NumberOfBytes,
			    IN PHYSICAL_ADDRESS HighestAcceptableAddress)
{
  return(MmAllocateContiguousAlignedMemory(NumberOfBytes,
					   HighestAcceptableAddress,
					   PAGE_SIZE));
}


/**********************************************************************
 * NAME							EXPORTED
 *	MmFreeContiguousMemory@4
 *
 * DESCRIPTION
 *	Releases a range of physically contiguous memory allocated
 *	with MmAllocateContiguousMemory.
 *	
 * ARGUMENTS
 *	BaseAddress
 *		Virtual address of the memory to be freed.
 *
 * RETURN VALUE
 * 	None.
 *
 * NOTE
 * 	Description taken from include/ddk/mmfuncs.h.
 * 	Code taken from ntoskrnl/mm/special.c.
 *
 * REVISIONS
 *
 */
VOID STDCALL 
MmFreeContiguousMemory(IN PVOID BaseAddress)
{
   MmLockAddressSpace(MmGetKernelAddressSpace());
   MmFreeMemoryArea(MmGetKernelAddressSpace(),
		    BaseAddress,
		    0,
		    MmFreeContinuousPage,
		    NULL);
   MmUnlockAddressSpace(MmGetKernelAddressSpace());
}


/* EOF */
