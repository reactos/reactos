/* $Id: cont.c,v 1.9 2001/03/08 22:06:01 dwelch Exp $
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

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID STATIC
MmFreeContinuousPage(PVOID Context, PVOID Address, ULONG PhysAddr)
{
  if (PhysAddr != 0)
    {
      MmDereferencePage((PVOID)PhysAddr);
    }
}

PVOID STDCALL
MmAllocateContiguousAlignedMemory(IN ULONG NumberOfBytes,
			          IN PHYSICAL_ADDRESS HighestAcceptableAddress,
				  IN ULONG Alignment)
{
   PMEMORY_AREA MArea;
   NTSTATUS Status;
   PVOID BaseAddress;
   PVOID PBase;
   ULONG i;

   Status = MmCreateMemoryArea(NULL,
			       MmGetKernelAddressSpace(),
			       MEMORY_AREA_CONTINUOUS_MEMORY,
			       &BaseAddress,
			       NumberOfBytes,
			       0,
			       &MArea);
   if (!NT_SUCCESS(Status))
     {
	return(NULL);
     }
   
   PBase = MmGetContinuousPages(NumberOfBytes,
				HighestAcceptableAddress,
				Alignment);
   if (PBase == NULL)
     {
       MmFreeMemoryArea(MmGetKernelAddressSpace(),
			BaseAddress,
			0,
			NULL,
			NULL);
       return(NULL);
     }
   for (i = 0; i < (PAGE_ROUND_UP(NumberOfBytes) / 4096); i++)
     {
	MmCreateVirtualMapping(NULL,
			       BaseAddress + (i * 4096),
			       PAGE_EXECUTE_READWRITE | PAGE_SYSTEM,
			       (ULONG)(PBase + (i * 4096)));
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
					   PAGESIZE));
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
   MmFreeMemoryArea(MmGetKernelAddressSpace(),
		    BaseAddress,
		    0,
		    MmFreeContinuousPage,
		    NULL);
}


/* EOF */
