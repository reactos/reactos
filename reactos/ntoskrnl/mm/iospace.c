/* $Id: iospace.c,v 1.3 2000/03/29 13:11:54 dwelch Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/iospace.c
 * PURPOSE:         Mapping I/O space
 * PROGRAMMER:      David Welch (welch@mcmail.com)
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
 *	MmMapIoSpace@16
 * 
 * DESCRIPTION
 * 	Maps a physical memory range into system space.
 * 	
 * ARGUMENTS
 *	PhysicalAddress
 *		First physical address to map;
 *		
 *	NumberOfBytes
 *		Number of bytes to map;
 *		
 *	CacheEnable
 *		TRUE if the range can be cached.
 *
 * RETURN VALUE
 *	The base virtual address which maps the region.
 *
 * NOTE
 * 	Description moved here from include/ddk/mmfuncs.h.
 * 	Code taken from ntoskrnl/mm/special.c.
 *
 * REVISIONS
 *
 */
PVOID STDCALL MmMapIoSpace (IN PHYSICAL_ADDRESS PhysicalAddress,
			    IN ULONG NumberOfBytes,
			    IN BOOLEAN CacheEnable)
{
   PVOID Result;
   MEMORY_AREA* marea;
   NTSTATUS Status;
   ULONG i;
   ULONG Attributes;
   
   Result = NULL;
   Status = MmCreateMemoryArea (NULL,
				MmGetKernelAddressSpace(),
				MEMORY_AREA_IO_MAPPING,
				&Result,
				NumberOfBytes,
				0,
				&marea);
   if (!NT_SUCCESS(STATUS_SUCCESS))
     {
	return (NULL);
     }
   Attributes = PA_WRITE | PA_READ | PA_EXECUTE | PA_SYSTEM;
   if (!CacheEnable)
     {
	Attributes |= (PA_PWT | PA_PCD);
     }
   for (i = 0; (i <= (NumberOfBytes / PAGESIZE)); i++)
     {
	MmSetPage (NULL,
		   (Result + (i * PAGESIZE)),
		   PAGE_READWRITE,
		   (PhysicalAddress.u.LowPart + (i * PAGESIZE)));
     }
   return ((PVOID)Result);
}
 

/**********************************************************************
 * NAME							EXPORTED
 *	MmUnmapIoSpace@8
 * 
 * DESCRIPTION
 * 	Unmaps a physical memory range from system space.
 * 	
 * ARGUMENTS
 *	BaseAddress
 *		The base virtual address which maps the region;	
 *		
 *	NumberOfBytes
 *		Number of bytes to unmap.
 *
 * RETURN VALUE
 *	None.
 *
 * NOTE
 * 	Code taken from ntoskrnl/mm/special.c.
 *
 * REVISIONS
 *
 */
VOID STDCALL MmUnmapIoSpace (IN PVOID BaseAddress,
			     IN ULONG NumberOfBytes)
{
   (VOID)MmFreeMemoryArea(&PsGetCurrentProcess()->Pcb.AddressSpace,
			  BaseAddress,
			  NumberOfBytes,
			  FALSE);
}


/* EOF */
