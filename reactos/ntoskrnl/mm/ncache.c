/* $Id: ncache.c,v 1.16 2002/05/13 18:10:40 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/cont.c
 * PURPOSE:         Manages non-cached memory
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/mm.h>
#include <internal/ps.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/


/**********************************************************************
 * NAME							EXPORTED
 * 	MmAllocateNonCachedMemory@4
 *
 * DESCRIPTION
 * 	Allocates a virtual address range of noncached and cache
 *	aligned memory.
 *	
 * ARGUMENTS
 *	NumberOfBytes
 *		Size of region to allocate.
 *		
 * RETURN VALUE
 *	The base address of the range on success;
 *	NULL on failure.
 *
 * NOTE
 * 	Description taken from include/ddk/mmfuncs.h.
 * 	Code taken from ntoskrnl/mm/special.c.
 *
 * REVISIONS
 *
 */
PVOID STDCALL 
MmAllocateNonCachedMemory(IN ULONG NumberOfBytes)
{
   PVOID Result;
   MEMORY_AREA* marea;
   NTSTATUS Status;
   ULONG i;
   ULONG Attributes;

   MmLockAddressSpace(MmGetKernelAddressSpace());
   Result = NULL;
   Status = MmCreateMemoryArea (NULL,
				MmGetKernelAddressSpace(),
				MEMORY_AREA_NO_CACHE,
				&Result,
				NumberOfBytes,
				0,
				&marea,
				FALSE);
   MmUnlockAddressSpace(MmGetKernelAddressSpace());

   if (!NT_SUCCESS(Status))
     {
	return (NULL);
     }
   Attributes = PAGE_READWRITE | PAGE_SYSTEM | PAGE_NOCACHE | 
     PAGE_WRITETHROUGH;
   for (i = 0; i <= (NumberOfBytes / PAGESIZE); i++)
     {
       ULONG_PTR NPage;

       Status = MmRequestPageMemoryConsumer(MC_NPPOOL, TRUE, &NPage);
       MmCreateVirtualMapping (NULL,
			       Result + (i * PAGESIZE),
			       Attributes,
			       (ULONG)NPage,
			       TRUE);
     }
   return ((PVOID)Result);
}

VOID
MmFreeNonCachedPage(IN BOOLEAN Before,
  IN PVOID  Context,
  IN PMEMORY_AREA  MemoryArea,
  IN PVOID  Address,
  IN ULONG_PTR  PhysicalAddress,
  IN SWAPENTRY  SwapEntry,
  IN BOOLEAN  Dirty)
{
  if (!Before)
    {
		  assert(SwapEntry == 0);
		  if (PhysicalAddress != 0)
		    {
		      MmDereferencePage(PhysicalAddress);
		    }
    }
}

/**********************************************************************
 * NAME							EXPORTED
 *	MmFreeNonCachedMemory@8
 *
 * DESCRIPTION
 *	Releases a range of noncached memory allocated with 
 *	MmAllocateNonCachedMemory.
 *	
 * ARGUMENTS
 *	BaseAddress
 *		Virtual address to be freed;
 *		
 *	NumberOfBytes
 *		Size of the region to be freed.
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
VOID STDCALL MmFreeNonCachedMemory (IN PVOID BaseAddress,
				    IN ULONG NumberOfBytes)
{
  MmFreeMemoryArea (MmGetKernelAddressSpace(),
		    BaseAddress,
		    NumberOfBytes,
		    MmFreeNonCachedPage,
		    NULL);
}


/* EOF */
