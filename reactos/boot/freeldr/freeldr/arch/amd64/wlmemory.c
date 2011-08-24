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

#define HYPER_SPACE_ENTRY       0x1EE

/* GLOBALS ***************************************************************/

PHARDWARE_PTE PxeBase;
//PHARDWARE_PTE HalPageTable;


/* FUNCTIONS **************************************************************/

BOOLEAN
MempAllocatePageTables()
{
    DPRINTM(DPRINT_WINDOWS,">>> MempAllocatePageTables\n");

	/* Allocate a page for the PML4 */
	PxeBase = MmAllocateMemoryWithType(PAGE_SIZE, LoaderMemoryData);
    if (!PxeBase)
    {
        DPRINTM(DPRINT_WINDOWS,"failed to allocate PML4\n");
        return FALSE;
    }

	// FIXME: Physical PTEs = FirmwareTemporary ?

	/* Zero the PML4 */
	RtlZeroMemory(PxeBase, PAGE_SIZE);

	/* The page tables are located at 0xfffff68000000000 
	 * We create a recursive self mapping through all 4 levels at 
	 * virtual address 0xfffff6fb7dbedf68 */
	PxeBase[VAtoPXI(PXE_BASE)].Valid = 1;
	PxeBase[VAtoPXI(PXE_BASE)].Write = 1;
	PxeBase[VAtoPXI(PXE_BASE)].PageFrameNumber = PtrToPfn(PxeBase);

    // FIXME: map PDE's for hals memory mapping

    DPRINTM(DPRINT_WINDOWS,">>> leave MempAllocatePageTables\n");

	return TRUE;
}

PHARDWARE_PTE
MempGetOrCreatePageDir(PHARDWARE_PTE PdeBase, ULONG Index)
{
	PHARDWARE_PTE SubDir;

	if (!PdeBase)
		return NULL;

	if (!PdeBase[Index].Valid)
	{
		SubDir = MmAllocateMemoryWithType(PAGE_SIZE, LoaderMemoryData);
		if (!SubDir)
			return NULL;
		RtlZeroMemory(SubDir, PAGE_SIZE);
		PdeBase[Index].PageFrameNumber = PtrToPfn(SubDir);
		PdeBase[Index].Valid = 1;
		PdeBase[Index].Write = 1;
	}
	else
	{
		SubDir = (PVOID)((ULONG64)(PdeBase[Index].PageFrameNumber) * PAGE_SIZE);
	}
	return SubDir;
}

BOOLEAN
MempMapSinglePage(ULONG64 VirtualAddress, ULONG64 PhysicalAddress)
{
	PHARDWARE_PTE PpeBase, PdeBase, PteBase;
	ULONG Index;

	PpeBase = MempGetOrCreatePageDir(PxeBase, VAtoPXI(VirtualAddress));
	PdeBase = MempGetOrCreatePageDir(PpeBase, VAtoPPI(VirtualAddress));
	PteBase = MempGetOrCreatePageDir(PdeBase, VAtoPDI(VirtualAddress));

	if (!PteBase)
	{
        DPRINTM(DPRINT_WINDOWS,"!!!No Dir %p, %p, %p, %p\n", PxeBase, PpeBase, PdeBase, PteBase);
		return FALSE;
	}

	Index = VAtoPTI(VirtualAddress);
	if (PteBase[Index].Valid)
	{
        DPRINTM(DPRINT_WINDOWS,"!!!Already mapped %ld\n", Index);
		return FALSE;
	}

	PteBase[Index].Valid = 1;
	PteBase[Index].Write = 1;
	PteBase[Index].PageFrameNumber = PhysicalAddress / PAGE_SIZE;

	return TRUE;
}

BOOLEAN
MempIsPageMapped(PVOID VirtualAddress)
{
	PHARDWARE_PTE PpeBase, PdeBase, PteBase;
    ULONG Index;
    
    Index = VAtoPXI(VirtualAddress);
    if (!PxeBase[Index].Valid)
        return FALSE;

    PpeBase = (PVOID)((ULONG64)(PxeBase[Index].PageFrameNumber) * PAGE_SIZE);
    Index = VAtoPPI(VirtualAddress);
    if (!PpeBase[Index].Valid)
        return FALSE;

    PdeBase = (PVOID)((ULONG64)(PpeBase[Index].PageFrameNumber) * PAGE_SIZE);
    Index = VAtoPDI(VirtualAddress);
    if (!PdeBase[Index].Valid)
        return FALSE;

    PteBase = (PVOID)((ULONG64)(PdeBase[Index].PageFrameNumber) * PAGE_SIZE);
    Index = VAtoPTI(VirtualAddress);
    if (!PteBase[Index].Valid)
        return FALSE;

    return TRUE;
}

ULONG
MempMapRangeOfPages(ULONG64 VirtualAddress, ULONG64 PhysicalAddress, ULONG cPages)
{
	ULONG i;

	for (i = 0; i < cPages; i++)
	{
		if (!MempMapSinglePage(VirtualAddress, PhysicalAddress))
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

	/* Setup KGDT64_NULL */
	Entry = KiGetGdtEntry(GdtBase, KGDT64_NULL);
	*(PULONG64)Entry = 0x0000000000000000ULL;

	/* Setup KGDT64_R0_CODE */
	Entry = KiGetGdtEntry(GdtBase, KGDT64_R0_CODE);
	*(PULONG64)Entry = 0x00209b0000000000ULL;

	/* Setup KGDT64_R0_DATA */
	Entry = KiGetGdtEntry(GdtBase, KGDT64_R0_DATA);
	*(PULONG64)Entry = 0x00cf93000000ffffULL;

	/* Setup KGDT64_R3_CMCODE */
	Entry = KiGetGdtEntry(GdtBase, KGDT64_R3_CMCODE);
	*(PULONG64)Entry = 0x00cffb000000ffffULL;

	/* Setup KGDT64_R3_DATA */
	Entry = KiGetGdtEntry(GdtBase, KGDT64_R3_DATA);
	*(PULONG64)Entry = 0x00cff3000000ffffULL;

	/* Setup KGDT64_R3_CODE */
	Entry = KiGetGdtEntry(GdtBase, KGDT64_R3_CODE);
	*(PULONG64)Entry = 0x0020fb0000000000ULL;

	/* Setup KGDT64_R3_CMTEB */
	Entry = KiGetGdtEntry(GdtBase, KGDT64_R3_CMTEB);
	*(PULONG64)Entry = 0xff40f3fd50003c00ULL;

	/* Setup TSS entry */
	Entry = KiGetGdtEntry(GdtBase, KGDT64_SYS_TSS);
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
	__writecr3((ULONG64)PxeBase);

    /* Get kernel mode address of gdt / idt */
	GdtIdt = (PVOID)((ULONG64)GdtIdt + KSEG0_BASE);

    /* Create gdt entries and load gdtr */
    WinLdrSetupGdt(GdtIdt, Tss);

    /* Copy old Idt and set idtr */
    WinLdrSetupIdt((PVOID)((ULONG64)GdtIdt + 2048)); // HACK!

    /* LDT is unused */
//    __lldt(0);

	/* Load TSR */
	__ltr(KGDT64_SYS_TSS);

    DPRINTM(DPRINT_WINDOWS, "leave WinLdrSetProcessorContext\n");
}

VOID
MempDump()
{
}

