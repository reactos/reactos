/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/special.c
 * PURPOSE:         Special types of memory region
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

PVOID MmAllocateSection(ULONG Length)
{
   ULONG Result;
   MEMORY_AREA* marea;
   NTSTATUS Status;
   ULONG i;
   ULONG Attributes;
   
   DPRINT("MmAllocateSection(Length %x)\n",Length);
   
   Result = 0;
   Status = MmCreateMemoryArea(KernelMode,
			       MEMORY_AREA_SYSTEM,
			       &Result,
			       Length,
			       0,
			       &marea);
   if (Status!=STATUS_SUCCESS)
     {
	return(NULL);
     }
   DPRINT("Result %x\n",Result);
   Attributes = PA_WRITE | PA_READ | PA_EXECUTE | PA_SYSTEM;
   for (i=0;i<=(Length/PAGESIZE);i++)
     {
	set_page(Result+(i*PAGESIZE),Attributes,get_free_page());
     }
   return((PVOID)Result);
}

PVOID MmAllocateContiguousMemory(ULONG NumberOfBytes,
				 PHYSICAL_ADDRESS HighestAcceptableAddress)
{
   UNIMPLEMENTED;
}

VOID MmFreeContiguousMemory(PVOID BaseAddress)
{
   UNIMPLEMENTED;
}

PVOID MmMapIoSpace(PHYSICAL_ADDRESS PhysicalAddress,
		   ULONG NumberOfBytes,
		   BOOLEAN CacheEnable)
{
   ULONG Result;
   MEMORY_AREA* marea;
   NTSTATUS Status;
   ULONG i;
   ULONG Attributes;
   
   Result=0;
   Status = MmCreateMemoryArea(KernelMode,
			       MEMORY_AREA_IO_MAPPING,
			       &Result,
			       NumberOfBytes,
			       0,
			       &marea);
   if (Status!=STATUS_SUCCESS)
     {
	return(NULL);
     }
   Attributes = PA_WRITE | PA_READ | PA_EXECUTE | PA_SYSTEM;
   if (!CacheEnable)
     {
	Attributes = Attributes | PA_PWT | PA_PCD;
     }
   for (i=0;i<=(NumberOfBytes/PAGESIZE);i++)
     {
	set_page(Result+(i*PAGESIZE),Attributes,PhysicalAddress.LowPart);
     }
   return((PVOID)Result);
}
 
VOID MmUnmapIoSpace(PVOID BaseAddress, ULONG NumberOfBytes)
{
   (void)MmFreeMemoryArea(BaseAddress,NumberOfBytes,FALSE);
}

PVOID MmAllocateNonCachedMemory(ULONG NumberOfBytes)
{
   ULONG Result;
   MEMORY_AREA* marea;
   NTSTATUS Status;
   ULONG i;
   
   Result=0;
   Status = MmCreateMemoryArea(KernelMode,
			       MEMORY_AREA_NO_CACHE,
			       &Result,
			       NumberOfBytes,
			       0,
			       &marea);
   if (Status!=STATUS_SUCCESS)
     {
	return(NULL);
     }
   for (i=0;i<=(NumberOfBytes/PAGESIZE);i++)
     {
	set_page(Result+(i*PAGESIZE),
		 PA_WRITE | PA_READ | PA_EXECUTE | PA_SYSTEM | PA_PCD | PA_PWT,
		 get_free_page());
     }
   return((PVOID)Result);
}

VOID MmFreeNonCachedMemory(PVOID BaseAddress, ULONG NumberOfBytes)
{
   MmFreeMemoryArea(BaseAddress,NumberOfBytes,TRUE);
}
