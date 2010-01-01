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

NTSTATUS
NTAPI
MiGetOnePage
(PMMSUPPORT AddressSpace,
 PMEMORY_AREA MemoryArea,
 PMM_REQUIRED_RESOURCES Required)
{
	int i;
	NTSTATUS Status = STATUS_SUCCESS;

	for (i = 0; i < Required->Amount; i++)
	{
		Status = MmRequestPageMemoryConsumer(Required->Consumer, TRUE, &Required->Page[i]);
		if (!NT_SUCCESS(Status))
		{
			while (i > 0)
			{
				MmReleasePageMemoryConsumer(Required->Consumer, Required->Page[i-1]);
				i--;
			}
			return Status;
		}
	}
	
	return Status;
}

NTSTATUS
NTAPI
MiReadFilePage
(PMMSUPPORT AddressSpace, 
 PMEMORY_AREA MemoryArea, 
 PMM_REQUIRED_RESOURCES RequiredResources)
{
	PFILE_OBJECT FileObject = RequiredResources->Context;
	PPFN_TYPE Page = &RequiredResources->Page[RequiredResources->Offset];
	PLARGE_INTEGER FileOffset = &RequiredResources->FileOffset;
	PFN_TYPE XPage;
	NTSTATUS Status;
	PVOID PageBuf = NULL;
	PMEMORY_AREA TmpArea;
	IO_STATUS_BLOCK IOSB;
	PHYSICAL_ADDRESS BoundaryAddressMultiple;

	BoundaryAddressMultiple.QuadPart = 0;

	DPRINT
		("Pulling page %08x%08x from disk\n", 
		 FileOffset->u.HighPart, FileOffset->u.LowPart);

	Status = MmRequestPageMemoryConsumer(RequiredResources->Consumer, TRUE, Page);
	if (!NT_SUCCESS(Status))
	{
		DPRINT1("Status: %x\n", Status);
		return Status;
	}

	MmLockAddressSpace(MmGetKernelAddressSpace());
	Status = MmCreateMemoryArea
		(MmGetKernelAddressSpace(),
		 MEMORY_AREA_VIRTUAL_MEMORY, 
		 &PageBuf,
		 PAGE_SIZE,
		 PAGE_READWRITE,
		 &TmpArea,
		 FALSE,
		 MEM_TOP_DOWN,
		 BoundaryAddressMultiple);
	
	DPRINT("Status %x, PageBuf %x\n", Status, PageBuf);
	if (!NT_SUCCESS(Status))
	{
		DPRINT1("STATUS_NO_MEMORY: %x\n", Status);
		MmUnlockAddressSpace(MmGetKernelAddressSpace());
		MmDereferencePage(*Page);
		return STATUS_NO_MEMORY;
	}
	
	Status = MmCreateVirtualMapping(NULL, PageBuf, PAGE_READWRITE, Page, 1);
	if (!NT_SUCCESS(Status))
	{
		MmFreeMemoryArea(MmGetKernelAddressSpace(), TmpArea, NULL, NULL);
		MmUnlockAddressSpace(MmGetKernelAddressSpace());
		MmDereferencePage(*Page);
		DPRINT1("Status: %x\n", Status);
		return Status;
	}
	
	MmUnlockAddressSpace(MmGetKernelAddressSpace());

	MiZeroPage(*Page);
	Status = MiSimpleRead
		(FileObject, 
		 FileOffset,
		 PageBuf,
		 PAGE_SIZE,
		 &IOSB);
	
	DPRINT("Read Status %x (Page %x)\n", Status, *Page);

	MmLockAddressSpace(MmGetKernelAddressSpace());
	MmDeleteVirtualMapping(NULL, PageBuf, FALSE, NULL, &XPage);
	ASSERT(XPage == *Page);
	MmFreeMemoryArea(MmGetKernelAddressSpace(), TmpArea, NULL, NULL);
	MmUnlockAddressSpace(MmGetKernelAddressSpace());

	if (!NT_SUCCESS(Status))
	{
		MmDereferencePage(*Page);
		DPRINT("Status: %x\n", Status);
		return Status;
	}

	return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
MiSwapInPage
(PMMSUPPORT AddressSpace,
 PMEMORY_AREA MemoryArea,
 PMM_REQUIRED_RESOURCES Resources)
{
	NTSTATUS Status;

	Status = MmRequestPageMemoryConsumer(Resources->Consumer, TRUE, &Resources->Page[Resources->Offset]);
	if (!NT_SUCCESS(Status))
	{
		DPRINT1("MmRequestPageMemoryConsumer failed, status = %x\n", Status);
		return Status;
	}
	
	Status = MmReadFromSwapPage(Resources->SwapEntry, Resources->Page[Resources->Offset]);
	if (!NT_SUCCESS(Status))
	{
		DPRINT1("MmReadFromSwapPage failed, status = %x\n", Status);
		return Status;
	}

	MmSetSavedSwapEntryPage(Resources->Page[Resources->Offset], Resources->SwapEntry);
	return Status;
}

NTSTATUS
NTAPI
MiWriteSwapPage
(PMMSUPPORT AddressSpace,
 PMEMORY_AREA MemoryArea,
 PMM_REQUIRED_RESOURCES Resources)
{
	return MmWriteToSwapPage(Resources->SwapEntry, Resources->Page[Resources->Offset]);
}

NTSTATUS
NTAPI
MiWriteFilePage
(PMMSUPPORT AddressSpace, 
 PMEMORY_AREA MemoryArea, 
 PMM_REQUIRED_RESOURCES Required)
{
	return MiWriteBackPage
		(Required->Context, 
		 &Required->FileOffset, 
		 PAGE_SIZE, 
		 Required->Page[Required->Offset]);
}
