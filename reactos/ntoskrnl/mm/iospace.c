/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: iospace.c,v 1.17 2002/11/05 20:35:33 hbirr Exp $
 *
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
#include <internal/ps.h>

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
PVOID STDCALL 
MmMapIoSpace (IN PHYSICAL_ADDRESS PhysicalAddress,
	      IN ULONG NumberOfBytes,
	      IN BOOLEAN CacheEnable)
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
				MEMORY_AREA_IO_MAPPING,
				&Result,
				NumberOfBytes,
				0,
				&marea,
				FALSE);
   MmUnlockAddressSpace(MmGetKernelAddressSpace());

   if (!NT_SUCCESS(STATUS_SUCCESS))
     {
	return (NULL);
     }
   Attributes = PAGE_EXECUTE_READWRITE | PAGE_SYSTEM;
   if (!CacheEnable)
     {
	Attributes |= (PAGE_NOCACHE | PAGE_WRITETHROUGH);
     }
   for (i = 0; (i < (PAGE_ROUND_UP(NumberOfBytes) / PAGE_SIZE)); i++)
     {
	Status = 
	  MmCreateVirtualMappingForKernel(Result + (i * PAGE_SIZE),
					  Attributes,
					  (PHYSICAL_ADDRESS)
					  (PhysicalAddress.QuadPart + 
					   (i * PAGE_SIZE)));
	if (!NT_SUCCESS(Status))
	  {
	     DbgPrint("Unable to create virtual mapping\n");
	     KeBugCheck(0);
	  }
     }
   return ((PVOID)(Result + PhysicalAddress.QuadPart % PAGE_SIZE));
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
VOID STDCALL 
MmUnmapIoSpace (IN PVOID BaseAddress,
		IN ULONG NumberOfBytes)
{
   MmLockAddressSpace(MmGetKernelAddressSpace());
   MmFreeMemoryArea(MmGetKernelAddressSpace(),
		    (PVOID)(((ULONG)BaseAddress / PAGE_SIZE) * PAGE_SIZE),
		    NumberOfBytes,
		    NULL,
		    NULL);
   MmUnlockAddressSpace(MmGetKernelAddressSpace());
}


/**********************************************************************
 * NAME							EXPORTED
 *	MmMapVideoDisplay@16
 */
PVOID STDCALL
MmMapVideoDisplay (IN	PHYSICAL_ADDRESS	PhysicalAddress,
		   IN	ULONG			NumberOfBytes,
		   IN	MEMORY_CACHING_TYPE	CacheType)
{
  return MmMapIoSpace (PhysicalAddress, NumberOfBytes, CacheType);
}


VOID STDCALL
MmUnmapVideoDisplay (IN	PVOID	BaseAddress,
		     IN	ULONG	NumberOfBytes)
{
  MmUnmapIoSpace (BaseAddress, NumberOfBytes);
}


/* EOF */
