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
/* $Id: mm.c,v 1.46 2001/03/16 18:11:23 dwelch Exp $
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
#include <internal/i386/segment.h>
#include <internal/mm.h>
#include <internal/ntoskrnl.h>
#include <internal/bitops.h>
#include <internal/io.h>
#include <internal/ps.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *****************************************************************/

PVOID EXPORTED MmUserProbeAddress = NULL; 
PVOID EXPORTED MmHighestUserAddress = NULL;
MM_STATS MmStats; 
extern PVOID MmSharedDataPagePhysicalAddress;

/* FUNCTIONS ****************************************************************/

NTSTATUS MmReleaseMemoryArea(PEPROCESS Process, PMEMORY_AREA Marea)
{
   NTSTATUS Status;
   
   DPRINT("MmReleaseMemoryArea(Process %x, Marea %x)\n",Process,Marea);
   
   DPRINT("Releasing %x between %x %x (type %d)\n",
	   Marea, Marea->BaseAddress, Marea->BaseAddress + Marea->Length,
	   Marea->Type);
   
   switch (Marea->Type)
     {
     case MEMORY_AREA_SECTION_VIEW_COMMIT:
     case MEMORY_AREA_SECTION_VIEW_RESERVE:
	Status = MmUnmapViewOfSection(Process, Marea->BaseAddress);
	assert(Status == STATUS_SUCCESS);
	return(STATUS_SUCCESS);

     case MEMORY_AREA_VIRTUAL_MEMORY:
       MmFreeVirtualMemory(Process, Marea);
       break;	

     case MEMORY_AREA_SHARED_DATA:
       Status = MmFreeMemoryArea(&Process->AddressSpace,
				 Marea->BaseAddress,
				 0,
				 NULL,
				 NULL);
       break;

     default:
       KeBugCheck(0);
     }
   
   return(STATUS_SUCCESS);
}

NTSTATUS MmReleaseMmInfo(PEPROCESS Process)
{
   PLIST_ENTRY CurrentEntry;
   PMEMORY_AREA Current;
   
   DPRINT("MmReleaseMmInfo(Process %x (%s))\n", Process,
	   Process->ImageFileName);
   
   MmLockAddressSpace(&Process->AddressSpace);

   CurrentEntry = Process->AddressSpace.MAreaListHead.Flink;
   while (CurrentEntry != &Process->AddressSpace.MAreaListHead)
     {
	Current = CONTAINING_RECORD(CurrentEntry, MEMORY_AREA, Entry);
	CurrentEntry = CurrentEntry->Flink;
	
	MmReleaseMemoryArea(Process, Current);
     }
   
   Mmi386ReleaseMmInfo(Process);
   
   MmUnlockAddressSpace(&Process->AddressSpace);
   MmDestroyAddressSpace(&Process->AddressSpace);
   
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
		       ULONG Address,
		       BOOLEAN FromMdl)
{
   PMADDRESS_SPACE AddressSpace;
   MEMORY_AREA* MemoryArea;
   NTSTATUS Status;
   BOOLEAN Locked = FromMdl;
   
   DPRINT("MmAccessFault(Mode %d, Address %x)\n", Mode, Address);
   
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
   
   if (!FromMdl)
     {
       MmLockAddressSpace(AddressSpace);
     }
   MemoryArea = MmOpenMemoryAreaByAddress(AddressSpace, (PVOID)Address);
   if (MemoryArea == NULL)
     {
	DbgPrint("%s:%d\n",__FILE__,__LINE__);
	if (!FromMdl)
	  {
	    MmUnlockAddressSpace(AddressSpace);
	  }
	return(STATUS_UNSUCCESSFUL);
     }
   
   switch (MemoryArea->Type)
     {
      case MEMORY_AREA_SYSTEM:
	Status = STATUS_UNSUCCESSFUL;
	break;
	
      case MEMORY_AREA_SECTION_VIEW_COMMIT:
	Status = MmAccessFaultSectionView(AddressSpace,
					  MemoryArea, 
					  (PVOID)Address,
					  Locked);
	break;
	
      case MEMORY_AREA_VIRTUAL_MEMORY:
	Status = STATUS_UNSUCCESSFUL;
	break;
	
      case MEMORY_AREA_SHARED_DATA:
	Status = STATUS_UNSUCCESSFUL;
	break;
	
      default:
	Status = STATUS_UNSUCCESSFUL;
	break;
     }
   DPRINT("Completed page fault handling\n");
   if (!FromMdl)
     {
       MmUnlockAddressSpace(AddressSpace);
     }
   return(Status);
}

NTSTATUS MmNotPresentFault(KPROCESSOR_MODE Mode,
			   ULONG Address, 
			   BOOLEAN FromMdl)
{
   PMADDRESS_SPACE AddressSpace;
   MEMORY_AREA* MemoryArea;
   NTSTATUS Status;
   BOOLEAN Locked = FromMdl;
   
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
   
   if (!FromMdl)
     {
       MmLockAddressSpace(AddressSpace);
     }
   MemoryArea = MmOpenMemoryAreaByAddress(AddressSpace, (PVOID)Address);
   if (MemoryArea == NULL)
     {
	DbgPrint("%s:%d\n",__FILE__,__LINE__);
	if (!FromMdl)
	  {
	    MmUnlockAddressSpace(AddressSpace);
	  }
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
					      (PVOID)Address,
					      Locked);
	break;
	
      case MEMORY_AREA_VIRTUAL_MEMORY:
	Status = MmNotPresentFaultVirtualMemory(AddressSpace,
						MemoryArea,
						(PVOID)Address,
						Locked);
	break;
	
      case MEMORY_AREA_SHARED_DATA:
	Status = 
	  MmCreateVirtualMapping(PsGetCurrentProcess(),
				 (PVOID)PAGE_ROUND_DOWN(Address),
				 PAGE_READONLY,
				 (ULONG)MmSharedDataPagePhysicalAddress);
	break;
	
      default:
	Status = STATUS_UNSUCCESSFUL;
	break;
     }
   DPRINT("Completed page fault handling\n");
   if (!FromMdl)
     {
       MmUnlockAddressSpace(AddressSpace);
     }
   return(Status);
}

/* Miscellanea functions: they may fit somewhere else */

DWORD STDCALL
MmAdjustWorkingSetSize (DWORD	Unknown0,
			DWORD	Unknown1,
			DWORD	Unknown2)
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
