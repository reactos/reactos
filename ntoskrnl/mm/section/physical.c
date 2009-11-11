/*
 * Copyright (C) 1998-2005 ReactOS Team (and the authors from the programmers section)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/section.c
 * PURPOSE:         Implements section objects
 *
 * PROGRAMMERS:     Rex Jolliff
 *                  David Welch
 *                  Eric Kohl
 *                  Emanuele Aliberti
 *                  Eugene Ingerman
 *                  Casper Hornstrup
 *                  KJK::Hyperion
 *                  Guido de Jong
 *                  Ge van Geldorp
 *                  Royce Mitchell III
 *                  Filip Navara
 *                  Aleksey Bragin
 *                  Jason Filby
 *                  Thomas Weidenmueller
 *                  Gunnar Andre' Dalsnes
 *                  Mike Nordell
 *                  Alex Ionescu
 *                  Gregor Anich
 *                  Steven Edwards
 *                  Herve Poussineau
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>
#include <reactos/exeformat.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, MmCreatePhysicalMemorySection)
#pragma alloc_text(INIT, MmInitSectionImplementation)
#endif

NTSTATUS
NTAPI
MmNotPresentFaultPhysicalMemory
(PMMSUPPORT AddressSpace,
 MEMORY_AREA* MemoryArea,
 PVOID Address,
 BOOLEAN Locked)
{
	ULONG Offset;
	PFN_TYPE Page;
	NTSTATUS Status;
	PMM_REGION Region;
	PVOID PAddress;
	PEPROCESS Process = MmGetAddressSpaceOwner(AddressSpace);

	DPRINT("Not Present: %p %p (%p-%p)\n", AddressSpace, Address, MemoryArea->StartingAddress, MemoryArea->EndingAddress);

	/*
	 * Just map the desired physical page
	 */
	PAddress = MM_ROUND_DOWN(Address, PAGE_SIZE);
	Offset = (ULONG_PTR)PAddress - (ULONG_PTR)MemoryArea->StartingAddress + MemoryArea->Data.SectionData.ViewOffset;
	Region = MmFindRegion(MemoryArea->StartingAddress,
						  &MemoryArea->Data.SectionData.RegionListHead,
						  Address, NULL);
	Page = Offset >> PAGE_SHIFT;
	Status = MmCreateVirtualMappingUnsafe(Process,
										  Address,
										  Region->Protect,
										  &Page,
										  1);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("MmCreateVirtualMappingUnsafe failed, not out of memory\n");
		KeBugCheck(MEMORY_MANAGEMENT);
		return(Status);
	}
	
	/*
	 * Cleanup and release locks
	 */
	DPRINT("Address 0x%.8X\n", Address);
	return(STATUS_SUCCESS);
}

NTSTATUS
NTAPI
MmAccessFaultPhysicalMemory
(PMMSUPPORT AddressSpace,
 MEMORY_AREA* MemoryArea,
 PVOID Address,
 BOOLEAN Locked)
{
	return(STATUS_ACCESS_VIOLATION);
}

NTSTATUS
INIT_FUNCTION
NTAPI
MmCreatePhysicalMemorySection()
{
   NTSTATUS Status;
   PROS_SECTION_OBJECT PhysSection;
   OBJECT_ATTRIBUTES Obj;
   LARGE_INTEGER SectionSize;
   UNICODE_STRING Name = RTL_CONSTANT_STRING(L"\\Device\\PhysicalMemory");
   HANDLE Handle;

   /*
    * Create the section mapping physical memory
    */
   SectionSize.QuadPart = 0xFFFFFFFF;
   InitializeObjectAttributes(&Obj,
                              &Name,
                              OBJ_PERMANENT,
                              NULL,
                              NULL);
   Status = MmCreateSection((PVOID)&PhysSection,
                            SECTION_ALL_ACCESS,
                            &Obj,
                            &SectionSize,
                            PAGE_EXECUTE_READWRITE,
                            0,
                            NULL,
                            NULL);
   if (!NT_SUCCESS(Status))
   {
      DPRINT1("Failed to create PhysicalMemory section\n");
      ASSERT(FALSE);
   }
   Status = ObInsertObject(PhysSection,
                           NULL,
                           SECTION_ALL_ACCESS,
                           0,
                           NULL,
                           &Handle);
   if (!NT_SUCCESS(Status))
   {
      ObDereferenceObject(PhysSection);
   }
   ObCloseHandle(Handle, KernelMode);
   PhysSection->AllocationAttributes |= SEC_PHYSICALMEMORY;
   PhysSection->Segment->Flags = MM_PHYSIMEM_SEGMENT;

   return(STATUS_SUCCESS);
}

VOID
NTAPI
MmUnmapPhysicalMemorySegment
(PMMSUPPORT AddressSpace, 
 PMEMORY_AREA MemoryArea,
 PROS_SECTION_OBJECT Section,
 PMM_SECTION_SEGMENT Segment)
{
	MmFreeMemoryArea
		(AddressSpace,
		 MemoryArea,
		 NULL,
		 NULL);
}
