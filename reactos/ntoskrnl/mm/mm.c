/* $Id: mm.c,v 1.34 2000/07/07 10:30:56 dwelch Exp $
 *
 * COPYRIGHT:   See COPYING in the top directory
 * PROJECT:     ReactOS kernel 
 * FILE:        ntoskrnl/mm/mm.c
 * PURPOSE:     kernel memory managment functions
 * PROGRAMMER:  David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *              Created 9/4/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/hal/io.h>
#include <internal/i386/segment.h>
#include <internal/stddef.h>
#include <internal/mm.h>
#include <string.h>
#include <internal/string.h>
#include <internal/ntoskrnl.h>
#include <internal/bitops.h>
#include <internal/string.h>
#include <internal/io.h>
#include <internal/ps.h>
#include <internal/mmhal.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *****************************************************************/

PVOID EXPORTED MmUserProbeAddress = NULL; 
PVOID EXPORTED MmHighestUserAddress = NULL;
MM_STATS MmStats; 

/* FUNCTIONS ****************************************************************/

NTSTATUS MmReleaseMemoryArea(PEPROCESS Process, PMEMORY_AREA Marea)
{
   PVOID i;
   
   DPRINT("MmReleaseMemoryArea(Process %x, Marea %x)\n",Process,Marea);
   
   DPRINT("Releasing %x between %x %x\n",
	  Marea, Marea->BaseAddress, Marea->BaseAddress + Marea->Length);
   
   if (Marea->Type == MEMORY_AREA_SECTION_VIEW_COMMIT ||
       Marea->Type == MEMORY_AREA_SECTION_VIEW_RESERVE)
     {
	MmUnmapViewOfSection(Process, Marea);
     }
   
   for (i = Marea->BaseAddress; 
	i < (Marea->BaseAddress + Marea->Length);
	i = i+PAGESIZE)
     {
	MmDeletePageEntry(Process, i, TRUE);
     }
   ExFreePool(Marea);
   
   return(STATUS_SUCCESS);
}

NTSTATUS MmReleaseMmInfo(PEPROCESS Process)
{
   PLIST_ENTRY CurrentEntry;
   PMEMORY_AREA Current;
   
   DPRINT("MmReleaseMmInfo(Process %x)\n",Process);
   
   MmLockAddressSpace(&Process->AddressSpace);
   
   while (!IsListEmpty(&Process->AddressSpace.MAreaListHead))
     {
	CurrentEntry = RemoveHeadList(
				     &Process->AddressSpace.MAreaListHead);
	Current = CONTAINING_RECORD(CurrentEntry, MEMORY_AREA, Entry);
	
	MmReleaseMemoryArea(Process, Current);
     }
   
   Mmi386ReleaseMmInfo(Process);
   
   MmUnlockAddressSpace(&Process->AddressSpace);
   
   DPRINT("Finished MmReleaseMmInfo()\n");
   return(STATUS_SUCCESS);
}

BOOLEAN STDCALL MmIsNonPagedSystemAddressValid(PVOID VirtualAddress)
{
   UNIMPLEMENTED;
}

BOOLEAN STDCALL MmIsAddressValid(PVOID VirtualAddress)
/*
 * FUNCTION: Checks whether the given address is valid for a read or write
 * ARGUMENTS:
 *          VirtualAddress = address to check
 * RETURNS: True if the access would be valid
 *          False if the access would cause a page fault
 * NOTES: This function checks whether a byte access to the page would
 *        succeed. Is this realistic for RISC processors which don't
 *        allow byte granular access?
 */
{
   MEMORY_AREA* MemoryArea;
   PMADDRESS_SPACE AddressSpace;
   
   AddressSpace = &PsGetCurrentProcess()->AddressSpace;
   
   MmLockAddressSpace(AddressSpace);
   MemoryArea = MmOpenMemoryAreaByAddress(AddressSpace,
					  VirtualAddress);

   if (MemoryArea == NULL)
     {
	MmUnlockAddressSpace(AddressSpace);
	return(FALSE);
     }
   MmUnlockAddressSpace(AddressSpace);
   return(TRUE);
}

NTSTATUS MmAccessFault(KPROCESSOR_MODE Mode,
		       ULONG Address)
{
   return(STATUS_UNSUCCESSFUL);
}

NTSTATUS MmNotPresentFault(KPROCESSOR_MODE Mode,
			   ULONG Address)
{
   PMADDRESS_SPACE AddressSpace;
   MEMORY_AREA* MemoryArea;
   NTSTATUS Status;
   
   DPRINT("MmNotPresentFault(Mode %d, Address %x)\n", Mode, Address);
   
   if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
     {
	DbgPrint("Page fault at high IRQL was %d\n", KeGetCurrentIrql());
	return(STATUS_UNSUCCESSFUL);
     }
   if (PsGetCurrentProcess() == NULL)
     {
	DbgPrint("No current process\n");
	return(STATUS_UNSUCCESSFUL);
     }
   
   /*
    * Find the memory area for the faulting address
    */
   if (Address >= KERNEL_BASE)
     {
	/*
	 * Check permissions
	 */
	if (Mode != KernelMode)
	  {
	     DbgPrint("%s:%d\n",__FILE__,__LINE__);
	     return(STATUS_UNSUCCESSFUL);
	  }
	AddressSpace = MmGetKernelAddressSpace();
     }
   else
     {
	AddressSpace = &PsGetCurrentProcess()->AddressSpace;
     }
   
   MmLockAddressSpace(AddressSpace);
   MemoryArea = MmOpenMemoryAreaByAddress(AddressSpace, (PVOID)Address);
   if (MemoryArea == NULL)
     {
	DbgPrint("%s:%d\n",__FILE__,__LINE__);
	MmUnlockAddressSpace(AddressSpace);
	return(STATUS_UNSUCCESSFUL);
     }
   
   switch (MemoryArea->Type)
     {
      case MEMORY_AREA_SYSTEM:
	Status = STATUS_UNSUCCESSFUL;
	break;
	
      case MEMORY_AREA_SECTION_VIEW_COMMIT:
	Status = MmNotPresentFaultSectionView(AddressSpace,
					      MemoryArea, 
					      (PVOID)Address);
	break;
	
      case MEMORY_AREA_COMMIT:
	Status = MmNotPresentFaultVirtualMemory(AddressSpace,
						MemoryArea,
						(PVOID)Address);
	break;
	
      default:
	Status = STATUS_UNSUCCESSFUL;
	break;
     }
   DPRINT("Completed page fault handling\n");
   MmUnlockAddressSpace(AddressSpace);
   return(Status);
}

/* Miscellanea functions: they may fit somewhere else */

DWORD
STDCALL
MmAdjustWorkingSetSize (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
	UNIMPLEMENTED;
	return (0);
}


DWORD
STDCALL
MmDbgTranslatePhysicalAddress (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
	UNIMPLEMENTED;
	return (0);
}


NTSTATUS
STDCALL
MmGrowKernelStack (
	DWORD	Unknown0
	)
{
	UNIMPLEMENTED;
	return (STATUS_NOT_IMPLEMENTED);
}


BOOLEAN
STDCALL
MmSetAddressRangeModified (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
	UNIMPLEMENTED;
	return (FALSE);
}

/* EOF */
