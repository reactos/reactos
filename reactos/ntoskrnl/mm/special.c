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
   PVOID Result;
   MEMORY_AREA* marea;
   NTSTATUS Status;
   ULONG i;
   
   DPRINT("MmAllocateSection(Length %x)\n",Length);
   
   Result = 0;
   Status = MmCreateMemoryArea(KernelMode,
			       PsGetCurrentProcess(),
			       MEMORY_AREA_SYSTEM,
			       &Result,
			       Length,
			       0,
			       &marea);
   if (Status!=STATUS_SUCCESS)
     {
	return(NULL);
     }
   DPRINT("Result %p\n",Result);
   for (i=0;i<=(Length/PAGESIZE);i++)
     {
	MmSetPage(NULL,
		  Result+(i*PAGESIZE),
		  PAGE_READWRITE,
		  get_free_page());
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
   PVOID Result;
   MEMORY_AREA* marea;
   NTSTATUS Status;
   ULONG i;
   ULONG Attributes;
   
   Result = 0;
   Status = MmCreateMemoryArea(KernelMode,
			       PsGetCurrentProcess(),
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
	MmSetPage(NULL,
		  Result + (i * PAGESIZE),
		  PAGE_READWRITE,
		  GET_LARGE_INTEGER_LOW_PART(PhysicalAddress));
     }
   return((PVOID)Result);
}
 
VOID MmUnmapIoSpace(PVOID BaseAddress, ULONG NumberOfBytes)
{
   (void)MmFreeMemoryArea(PsGetCurrentProcess(),BaseAddress,NumberOfBytes,
			  FALSE);
}

PVOID MmAllocateNonCachedMemory(ULONG NumberOfBytes)
{
   PVOID Result;
   MEMORY_AREA* marea;
   NTSTATUS Status;
   ULONG i;
   
   Result = 0;
   Status = MmCreateMemoryArea(KernelMode,
			       PsGetCurrentProcess(),
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
	MmSetPage(NULL,
		  Result+(i*PAGESIZE),
		  PAGE_READWRITE,
		  get_free_page());
     }
   return((PVOID)Result);
}

VOID MmFreeNonCachedMemory(PVOID BaseAddress, ULONG NumberOfBytes)
{
   MmFreeMemoryArea(PsGetCurrentProcess(),BaseAddress,NumberOfBytes,TRUE);
}
