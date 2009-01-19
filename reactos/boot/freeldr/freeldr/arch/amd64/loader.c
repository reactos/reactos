/*
 *  FreeLoader
 *  Copyright (C) 2008 - 2009 Timo Kreuzer (timo.kreuzer@reactor.org)
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
#define _NTSYSTEM_
#include <freeldr.h>

#define NDEBUG
#include <debug.h>

/* Page Directory and Tables for non-PAE Systems */
extern ULONG_PTR NextModuleBase;
extern ULONG_PTR KernelBase;
ULONG_PTR GdtBase, IdtBase, TssBase;
extern ROS_KERNEL_ENTRY_POINT KernelEntryPoint;

PPAGE_DIRECTORY_AMD64 pPML4;
PVOID pIdt, pGdt;

/* FUNCTIONS *****************************************************************/

void
EnableA20()
{
    /* Already done */
}

void
DumpLoaderBlock()
{
	DbgPrint("LoaderBlock @ %p.\n", &LoaderBlock);
	DbgPrint("Flags = 0x%x.\n", LoaderBlock.Flags);
	DbgPrint("MemLower = 0x%p.\n", (PVOID)LoaderBlock.MemLower);
	DbgPrint("MemHigher = 0x%p.\n", (PVOID)LoaderBlock.MemHigher);
	DbgPrint("BootDevice = 0x%x.\n", LoaderBlock.BootDevice);
	DbgPrint("CommandLine = %s.\n", LoaderBlock.CommandLine);
	DbgPrint("ModsCount = 0x%x.\n", LoaderBlock.ModsCount);
	DbgPrint("ModsAddr = 0x%p.\n", LoaderBlock.ModsAddr);
	DbgPrint("Syms = 0x%s.\n", LoaderBlock.Syms);
	DbgPrint("MmapLength = 0x%x.\n", LoaderBlock.MmapLength);
	DbgPrint("MmapAddr = 0x%p.\n", (PVOID)LoaderBlock.MmapAddr);
	DbgPrint("RdLength = 0x%x.\n", LoaderBlock.RdLength);
	DbgPrint("RdAddr = 0x%p.\n", (PVOID)LoaderBlock.RdAddr);
	DbgPrint("DrivesCount = 0x%x.\n", LoaderBlock.DrivesCount);
	DbgPrint("DrivesAddr = 0x%p.\n", (PVOID)LoaderBlock.DrivesAddr);
	DbgPrint("ConfigTable = 0x%x.\n", LoaderBlock.ConfigTable);
	DbgPrint("BootLoaderName = 0x%x.\n", LoaderBlock.BootLoaderName);
	DbgPrint("PageDirectoryStart = 0x%p.\n", (PVOID)LoaderBlock.PageDirectoryStart);
	DbgPrint("PageDirectoryEnd = 0x%p.\n", (PVOID)LoaderBlock.PageDirectoryEnd);
	DbgPrint("KernelBase = 0x%p.\n", (PVOID)LoaderBlock.KernelBase);
	DbgPrint("ArchExtra = 0x%p.\n", (PVOID)LoaderBlock.ArchExtra);

}

/*++
 * FrLdrStartup
 * INTERNAL
 *
 *     Prepares the system for loading the Kernel.
 *
 * Params:
 *     Magic - Multiboot Magic
 *
 * Returns:
 *     None.
 *
 * Remarks:
 *     None.
 *
 *--*/
VOID
NTAPI
FrLdrStartup(ULONG Magic)
{
	/* Disable Interrupts */
	_disable();

	/* Re-initalize EFLAGS */
	KeAmd64EraseFlags();

	/* Initialize the page directory */
	FrLdrSetupPageDirectory();

	/* Set the new PML4 */
	__writecr3((ULONGLONG)pPML4);

	FrLdrSetupGdtIdt();

	LoaderBlock.FrLdrDbgPrint = DbgPrint;

//	DumpLoaderBlock();

	DbgPrint("Jumping to kernel @ %p.\n", KernelEntryPoint);

	/* Jump to Kernel */
	(*KernelEntryPoint)(Magic, &LoaderBlock);

}

PPAGE_DIRECTORY_AMD64
FrLdrGetOrCreatePageDir(PPAGE_DIRECTORY_AMD64 pDir, ULONG Index)
{
	PPAGE_DIRECTORY_AMD64 pSubDir;

	if (!pDir)
		return NULL;

	if (!pDir->Pde[Index].Valid)
	{
		pSubDir = MmAllocateMemoryWithType(PAGE_SIZE, LoaderSpecialMemory);
		if (!pSubDir)
			return NULL;
		RtlZeroMemory(pSubDir, PAGE_SIZE);
		pDir->Pde[Index].PageFrameNumber = PtrToPfn(pSubDir);
		pDir->Pde[Index].Valid = 1;
		pDir->Pde[Index].Write = 1;
	}
	else
	{
		pSubDir = (PPAGE_DIRECTORY_AMD64)((ULONGLONG)(pDir->Pde[Index].PageFrameNumber) * PAGE_SIZE);
	}
	return pSubDir;
}

BOOLEAN
FrLdrMapSinglePage(ULONGLONG VirtualAddress, ULONGLONG PhysicalAddress)
{
	PPAGE_DIRECTORY_AMD64 pDir3, pDir2, pDir1;
	ULONG Index;

	pDir3 = FrLdrGetOrCreatePageDir(pPML4, VAtoPXI(VirtualAddress));
	pDir2 = FrLdrGetOrCreatePageDir(pDir3, VAtoPPI(VirtualAddress));
	pDir1 = FrLdrGetOrCreatePageDir(pDir2, VAtoPDI(VirtualAddress));

	if (!pDir1)
		return FALSE;

	Index = VAtoPTI(VirtualAddress);
	if (pDir1->Pde[Index].Valid)
	{
		return FALSE;
	}

	pDir1->Pde[Index].Valid = 1;
	pDir1->Pde[Index].Write = 1;
	pDir1->Pde[Index].PageFrameNumber = PhysicalAddress / PAGE_SIZE;

	return TRUE;
}

ULONG
FrLdrMapRangeOfPages(ULONGLONG VirtualAddress, ULONGLONG PhysicalAddress, ULONG cPages)
{
	ULONG i;

	for (i = 0; i < cPages; i++)
	{
		if (!FrLdrMapSinglePage(VirtualAddress, PhysicalAddress))
		{
			return i;
		}
		VirtualAddress += PAGE_SIZE;
		PhysicalAddress += PAGE_SIZE;
	}
	return i;
}


/*++
 * FrLdrSetupPageDirectory
 * INTERNAL
 *
 *     Sets up the ReactOS Startup Page Directory.
 *
 * Params:
 *     None.
 *
 * Returns:
 *     None.
 *--*/
VOID
FASTCALL
FrLdrSetupPageDirectory(VOID)
{
	ULONG KernelPages;
	PVOID UserSharedData;

	/* Allocate a Page for the PML4 */
	pPML4 = MmAllocateMemoryWithType(PAGE_SIZE, LoaderSpecialMemory);

	ASSERT(pPML4);

	/* The page tables are located at 0xfffff68000000000 
	 * We create a recursive self mapping through all 4 levels at 
	 * virtual address 0xfffff6fb7dbedf68 */
	pPML4->Pde[VAtoPXI(PXE_BASE)].Valid = 1;
	pPML4->Pde[VAtoPXI(PXE_BASE)].Write = 1;
	pPML4->Pde[VAtoPXI(PXE_BASE)].PageFrameNumber = PtrToPfn(pPML4);

	/* Setup low memory pages */
	if (FrLdrMapRangeOfPages(0, 0, 1024) < 1024)
	{
		DbgPrint("Could not map low memory pages.\n");
	}

	/* Setup kernel pages */
	KernelPages = (ROUND_TO_PAGES(NextModuleBase - KERNEL_BASE_PHYS) / PAGE_SIZE);
	if (FrLdrMapRangeOfPages(KernelBase, KERNEL_BASE_PHYS, KernelPages) != KernelPages)
	{
		DbgPrint("Could not map %d kernel pages.\n", KernelPages);
	}

	/* Setup a page for the idt */
	pIdt = MmAllocateMemoryWithType(PAGE_SIZE, LoaderSpecialMemory);
	IdtBase = KernelBase + KernelPages * PAGE_SIZE;
	if (!FrLdrMapSinglePage(IdtBase, (ULONGLONG)pIdt))
	{
		DbgPrint("Could not map idt page.\n", KernelPages);
	}

	/* Setup a page for the gdt & tss */
	pGdt = MmAllocateMemoryWithType(PAGE_SIZE, LoaderSpecialMemory);
	GdtBase = IdtBase + PAGE_SIZE;
	TssBase = GdtBase + 20 * sizeof(ULONG64); // FIXME: don't hardcode
	if (!FrLdrMapSinglePage(GdtBase, (ULONGLONG)pGdt))
	{
		DbgPrint("Could not map gdt page.\n", KernelPages);
	}

	/* Setup KUSER_SHARED_DATA page */
	UserSharedData = MmAllocateMemoryWithType(PAGE_SIZE, LoaderSpecialMemory);
	if (!FrLdrMapSinglePage(KI_USER_SHARED_DATA, (ULONG64)UserSharedData))
	{
		DbgPrint("Could not map KUSER_SHARED_DATA page.\n", KernelPages);
	}

	/* Map APIC page */
	if (!FrLdrMapSinglePage(APIC_BASE, APIC_PHYS_BASE))
	{
		DbgPrint("Could not map APIC page.\n");
	}

}

VOID
FrLdrSetupGdtIdt()
{
	PKGDTENTRY64 Entry;
	KDESCRIPTOR Desc;

	RtlZeroMemory(pGdt, PAGE_SIZE);

	/* Setup KGDT_64_R0_CODE */
	Entry = KiGetGdtEntry(pGdt, KGDT_64_R0_CODE);
	*(PULONG64)Entry = 0x0020980000000000ULL;

	/* Setup KGDT_64_DATA */
	Entry = KiGetGdtEntry(pGdt, KGDT_64_DATA);
	*(PULONG64)Entry = 0x0000F00000000000ULL;

	/* Setup TSS entry */
	Entry = KiGetGdtEntry(pGdt, KGDT_TSS);
	KiInitGdtEntry(Entry, TssBase, I386_TSS, 0);

	/* Setup the gdt descriptor */
	Desc.Limit = 12 * sizeof(ULONG64) - 1;
	Desc.Base = (PVOID)GdtBase;

	/* Set the new Gdt */
	__lgdt(&Desc.Limit);
	DbgPrint("Gdtr.Base = %p\n", Desc.Base);

	/* Setup the idt descriptor */
	Desc.Limit = 12 * sizeof(ULONG64) - 1;
	Desc.Base = (PVOID)IdtBase;

	/* Set the new Idt */
	__lidt(&Desc.Limit);
	DbgPrint("Idtr.Base = %p\n", Desc.Base);

}
