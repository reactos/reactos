/*
 * PROJECT:         EFI Windows Loader
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            freeldr/amd64/wlmemory.c
 * PURPOSE:         Memory related routines
 * PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ***************************************************************/

#include <freeldr.h>

#include <ndk/asm.h>
#include <debug.h>

//extern ULONG LoaderPagesSpanned;

#define HYPER_SPACE_ENTRY       0x300

// This is needed only for SetProcessorContext routine
#pragma pack(2)
	typedef struct
	{
		USHORT Limit;
		ULONG Base;
	} GDTIDT;
#pragma pack(4)

/* GLOBALS ***************************************************************/

//PHARDWARE_PTE PDE;
//PHARDWARE_PTE HalPageTable;

PPAGE_DIRECTORY_AMD64 pPML4;

/* FUNCTIONS **************************************************************/

BOOLEAN
MempAllocatePageTables()
{
	ULONG KernelPages;
	PVOID UserSharedData;

    DPRINTM(DPRINT_WINDOWS,">>> MempAllocatePageTables\n");

	/* Allocate a page for the PML4 */
	pPML4 = MmAllocateMemoryWithType(PAGE_SIZE, LoaderMemoryData);
    if (!pPML4)
    {
        DPRINTM(DPRINT_WINDOWS,"failed to allocate PML4\n");
        return FALSE;
    }

	// FIXME: Physical PTEs = FirmwareTemporary ?

	/* Zero the PML4 */
	RtlZeroMemory(pPML4, PAGE_SIZE);

	/* The page tables are located at 0xfffff68000000000 
	 * We create a recursive self mapping through all 4 levels at 
	 * virtual address 0xfffff6fb7dbedf68 */
	pPML4->Pde[VAtoPXI(PXE_BASE)].Valid = 1;
	pPML4->Pde[VAtoPXI(PXE_BASE)].Write = 1;
	pPML4->Pde[VAtoPXI(PXE_BASE)].PageFrameNumber = PtrToPfn(pPML4);

    // FIXME: map PDE's for hals memory mapping

    DPRINTM(DPRINT_WINDOWS,">>> leave MempAllocatePageTables\n");

	return TRUE;
}

PPAGE_DIRECTORY_AMD64
MempGetOrCreatePageDir(PPAGE_DIRECTORY_AMD64 pDir, ULONG Index)
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
MempMapSinglePage(ULONG64 VirtualAddress, ULONG64 PhysicalAddress)
{
	PPAGE_DIRECTORY_AMD64 pDir3, pDir2, pDir1;
	ULONG Index;

	pDir3 = MempGetOrCreatePageDir(pPML4, VAtoPXI(VirtualAddress));
	pDir2 = MempGetOrCreatePageDir(pDir3, VAtoPPI(VirtualAddress));
	pDir1 = MempGetOrCreatePageDir(pDir2, VAtoPDI(VirtualAddress));

	if (!pDir1)
	{
        DPRINTM(DPRINT_WINDOWS,"!!!No Dir %p, %p, %p, %p\n", pPML4, pDir3, pDir2, pDir1);
		return FALSE;
	}

	Index = VAtoPTI(VirtualAddress);
	if (pDir1->Pde[Index].Valid)
	{
        DPRINTM(DPRINT_WINDOWS,"!!!Already mapped %ld\n", Index);
		return FALSE;
	}

	pDir1->Pde[Index].Valid = 1;
	pDir1->Pde[Index].Write = 1;
	pDir1->Pde[Index].PageFrameNumber = PhysicalAddress / PAGE_SIZE;

	return TRUE;
}

BOOLEAN
MempIsPageMapped(PVOID VirtualAddress)
{
    PPAGE_DIRECTORY_AMD64 pDir;
    ULONG Index;
    
    pDir = pPML4;
    Index = VAtoPXI(VirtualAddress);
    if (!pDir->Pde[Index].Valid)
        return FALSE;

    pDir = (PPAGE_DIRECTORY_AMD64)((ULONG64)(pDir->Pde[Index].PageFrameNumber) * PAGE_SIZE);
    Index = VAtoPPI(VirtualAddress);
    if (!pDir->Pde[Index].Valid)
        return FALSE;

    pDir = (PPAGE_DIRECTORY_AMD64)((ULONG64)(pDir->Pde[Index].PageFrameNumber) * PAGE_SIZE);
    Index = VAtoPDI(VirtualAddress);
    if (!pDir->Pde[Index].Valid)
        return FALSE;

    pDir = (PPAGE_DIRECTORY_AMD64)((ULONG64)(pDir->Pde[Index].PageFrameNumber) * PAGE_SIZE);
    Index = VAtoPTI(VirtualAddress);
    if (!pDir->Pde[Index].Valid)
        return FALSE;

    return TRUE;
}

ULONG
MempMapRangeOfPages(ULONGLONG VirtualAddress, ULONGLONG PhysicalAddress, ULONG cPages)
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

BOOLEAN
MempSetupPaging(IN ULONG StartPage,
				IN ULONG NumberOfPages)
{
    DPRINTM(DPRINT_WINDOWS,">>> MempSetupPaging(0x%lx, %ld, %p)\n", 
            StartPage, NumberOfPages, StartPage * PAGE_SIZE + KSEG0_BASE);

    /* Identity mapping */
    if (MempMapRangeOfPages(StartPage * PAGE_SIZE,
                            StartPage * PAGE_SIZE,
                            NumberOfPages) != NumberOfPages)
    {
        DPRINTM(DPRINT_WINDOWS,"Failed to map pages 1\n");
        return FALSE;
    }

    /* Kernel mapping */
    if (MempMapRangeOfPages(StartPage * PAGE_SIZE + KSEG0_BASE,
                            StartPage * PAGE_SIZE,
                            NumberOfPages) != NumberOfPages)
    {
        DPRINTM(DPRINT_WINDOWS,"Failed to map pages 2\n");
        return FALSE;
    }

	return TRUE;
}

VOID
MempUnmapPage(ULONG Page)
{
   // DPRINTM(DPRINT_WINDOWS,">>> MempUnmapPage\n");
}

VOID
WinLdrpMapApic()
{
	BOOLEAN LocalAPIC;
	LARGE_INTEGER MsrValue;
	ULONG CpuInfo[4];
	ULONG64 APICAddress;

    DPRINTM(DPRINT_WINDOWS,">>> WinLdrpMapApic\n");

	/* Check if we have a local APIC */
	__cpuid((int*)CpuInfo, 1);
	LocalAPIC = (((CpuInfo[3] >> 9) & 1) != 0);

	/* If there is no APIC, just return */
	if (!LocalAPIC)
	{
        DPRINTM(DPRINT_WINDOWS,"No APIC found.\n");
		return;
	}

	/* Read the APIC Address */
	MsrValue.QuadPart = __readmsr(0x1B);
	APICAddress = (MsrValue.LowPart & 0xFFFFF000);

	DPRINTM(DPRINT_WINDOWS, "Local APIC detected at address 0x%x\n",
		APICAddress);

	/* Map it */
	MempMapSinglePage(APIC_BASE, APICAddress);
}

BOOLEAN
WinLdrMapSpecialPages(ULONG PcrBasePage)
{
    /* Map the PCR page */
    if (!MempMapSinglePage(KIP0PCRADDRESS, PcrBasePage * PAGE_SIZE))
    {
        DPRINTM(DPRINT_WINDOWS, "Could not map PCR @ %lx\n", PcrBasePage);
        return FALSE;
    }

    /* Map KI_USER_SHARED_DATA */
    if (!MempMapSinglePage(KI_USER_SHARED_DATA, (PcrBasePage+1) * PAGE_SIZE))
    {
        DPRINTM(DPRINT_WINDOWS, "Could not map KI_USER_SHARED_DATA\n");
        return FALSE;
    }

	/* Map the APIC page */
	WinLdrpMapApic();

    return TRUE;
}

VOID
WinLdrSetupGdt(PVOID GdtBase, ULONG64 TssBase)
{
	PKGDTENTRY64 Entry;
	KDESCRIPTOR GdtDesc;

	/* Setup KGDT_64_R0_CODE */
	Entry = KiGetGdtEntry(GdtBase, KGDT_64_R0_CODE);
	*(PULONG64)Entry = 0x00209b0000000000ULL;

	/* Setup KGDT_64_R0_SS */
	Entry = KiGetGdtEntry(GdtBase, KGDT_64_R0_SS);
	*(PULONG64)Entry = 0x00cf93000000ffffULL;

	/* Setup KGDT_64_DATA */
	Entry = KiGetGdtEntry(GdtBase, KGDT_64_DATA);
	*(PULONG64)Entry = 0x00cff3000000ffffULL;

	/* Setup KGDT_64_R3_CODE */
	Entry = KiGetGdtEntry(GdtBase, KGDT_64_R3_CODE);
	*(PULONG64)Entry = 0x0020fb0000000000ULL;

	/* Setup KGDT_32_R3_TEB */
	Entry = KiGetGdtEntry(GdtBase, KGDT_32_R3_TEB);
	*(PULONG64)Entry = 0xff40f3fd50003c00ULL;

	/* Setup TSS entry */
	Entry = KiGetGdtEntry(GdtBase, KGDT_TSS);
	KiInitGdtEntry(Entry, TssBase, sizeof(KTSS), I386_TSS, 0);

    /* Setup GDT descriptor */
	GdtDesc.Base  = GdtBase;
	GdtDesc.Limit = NUM_GDT * sizeof(KGDTENTRY) - 1;

	/* Set the new Gdt */
	__lgdt(&GdtDesc.Limit);
	DbgPrint("Gdtr.Base = %p, num = %ld\n", GdtDesc.Base, NUM_GDT);

}

VOID
WinLdrSetupIdt(PVOID IdtBase)
{
	KDESCRIPTOR IdtDesc, OldIdt;

    /* Get old IDT */
	__sidt(&OldIdt);

	/* Copy the old IDT */
	RtlCopyMemory(IdtBase, (PVOID)OldIdt.Base, OldIdt.Limit + 1);

	/* Setup the new IDT descriptor */
	IdtDesc.Base = IdtBase;
	IdtDesc.Limit = NUM_IDT * sizeof(KIDTENTRY) - 1;

	/* Set the new IDT */
	__lidt(&IdtDesc.Limit);
	DbgPrint("Idtr.Base = %p\n", IdtDesc.Base);

}

VOID
WinLdrSetProcessorContext(PVOID GdtIdt, IN ULONG64 Pcr, IN ULONG64 Tss)
{
    DPRINTM(DPRINT_WINDOWS, "WinLdrSetProcessorContext %p\n", Pcr);

	/* Disable Interrupts */
	_disable();

	/* Re-initalize EFLAGS */
	__writeeflags(0);

	/* Set the new PML4 */
	__writecr3((ULONGLONG)pPML4);

	RtlZeroMemory(GdtIdt, PAGE_SIZE);

    WinLdrSetupGdt(GdtIdt, Tss);

    WinLdrSetupIdt((PVOID)((ULONG64)GdtIdt + 2048)); // HACK!

    /* LDT is unused */
    __lldt(0);

    /* Load selectors for DS/ES/FS/GS/SS */
    Ke386SetDs(KGDT_64_DATA | RPL_MASK);   // 0x2b
    Ke386SetEs(KGDT_64_DATA | RPL_MASK);   // 0x2b
    Ke386SetFs(KGDT_32_R3_TEB | RPL_MASK); // 0x53
	Ke386SetGs(KGDT_64_DATA | RPL_MASK);   // 0x2b
	Ke386SetSs(KGDT_64_R0_SS);             // 0x18

	// Load TSR
	Ke386SetTr(KGDT_TSS);

    DPRINTM(DPRINT_WINDOWS, "leave WinLdrSetProcessorContext\n");
}

VOID
MempDump()
{
}

