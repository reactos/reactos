/* $Id: ncache.c,v 1.4 2000/03/29 13:11:54 dwelch Exp $
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
PVOID STDCALL MmAllocateNonCachedMemory(IN ULONG NumberOfBytes)
{
   PVOID Result;
   MEMORY_AREA* marea;
   NTSTATUS Status;
   ULONG i;
   
   Result = NULL;
   Status = MmCreateMemoryArea (NULL,
				MmGetKernelAddressSpace(),
				MEMORY_AREA_NO_CACHE,
				&Result,
				NumberOfBytes,
				0,
				&marea);
   if (!NT_SUCCESS(Status))
     {
	return (NULL);
     }
   for (i = 0; (i <= (NumberOfBytes / PAGESIZE)); i++)
     {
	MmSetPage (NULL,
		   (Result + (i * PAGESIZE)),
		   PAGE_READWRITE,
		   (ULONG)MmAllocPage());
	}
   return ((PVOID)Result);
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
   MmFreeMemoryArea (&PsGetCurrentProcess()->Pcb.AddressSpace,
		     BaseAddress,
		     NumberOfBytes,
		     TRUE);
}


/* EOF */
