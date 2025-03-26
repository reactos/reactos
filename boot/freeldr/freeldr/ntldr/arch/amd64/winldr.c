/*
 * PROJECT:         EFI Windows Loader
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            boot/freeldr/freeldr/arch/amd64/winldr.c
 * PURPOSE:         Memory related routines
 * PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES ***************************************************************/

#include <freeldr.h>
#include <ndk/asm.h>
#include <internal/amd64/intrin_i.h>
#include "../../winldr.h"

#include <debug.h>
DBG_DEFAULT_CHANNEL(WINDOWS);

//extern ULONG LoaderPagesSpanned;

/* GLOBALS ***************************************************************/

PHARDWARE_PTE PxeBase;
//PHARDWARE_PTE HalPageTable;

PVOID GdtIdt;
PFN_NUMBER SharedUserDataPfn;
ULONG_PTR TssBasePage;

/* FUNCTIONS **************************************************************/

static
BOOLEAN
MempAllocatePageTables(VOID)
{
    TRACE(">>> MempAllocatePageTables\n");

    /* Allocate a page for the PML4 */
    PxeBase = MmAllocateMemoryWithType(PAGE_SIZE, LoaderMemoryData);
    if (!PxeBase)
    {
        ERR("failed to allocate PML4\n");
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

    TRACE(">>> leave MempAllocatePageTables\n");

    return TRUE;
}

static
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

static
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
        ERR("!!!No Dir %p, %p, %p, %p\n", PxeBase, PpeBase, PdeBase, PteBase);
        return FALSE;
    }

    Index = VAtoPTI(VirtualAddress);
    if (PteBase[Index].Valid)
    {
        ERR("!!!Already mapped %ld\n", Index);
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

static
PFN_NUMBER
MempMapRangeOfPages(ULONG64 VirtualAddress, ULONG64 PhysicalAddress, PFN_NUMBER cPages)
{
    PFN_NUMBER i;

    for (i = 0; i < cPages; i++)
    {
        if (!MempMapSinglePage(VirtualAddress, PhysicalAddress))
        {
            ERR("Failed to map page %ld from %p to %p\n",
                    i, (PVOID)VirtualAddress, (PVOID)PhysicalAddress);
            return i;
        }
        VirtualAddress += PAGE_SIZE;
        PhysicalAddress += PAGE_SIZE;
    }
    return i;
}

BOOLEAN
MempSetupPaging(IN PFN_NUMBER StartPage,
                IN PFN_NUMBER NumberOfPages,
                IN BOOLEAN KernelMapping)
{
    TRACE(">>> MempSetupPaging(0x%lx, %ld, %p)\n",
            StartPage, NumberOfPages, StartPage * PAGE_SIZE + KSEG0_BASE);

    /* Identity mapping */
    if (MempMapRangeOfPages(StartPage * PAGE_SIZE,
                            StartPage * PAGE_SIZE,
                            NumberOfPages) != NumberOfPages)
    {
        ERR("Failed to map pages %ld, %ld\n",
                StartPage, NumberOfPages);
        return FALSE;
    }

    /* Kernel mapping */
    if (KernelMapping)
    {
        if (MempMapRangeOfPages(StartPage * PAGE_SIZE + KSEG0_BASE,
                                StartPage * PAGE_SIZE,
                                NumberOfPages) != NumberOfPages)
        {
            ERR("Failed to map pages %ld, %ld\n",
                    StartPage, NumberOfPages);
            return FALSE;
        }
    }

    return TRUE;
}

VOID
MempUnmapPage(PFN_NUMBER Page)
{
   // TRACE(">>> MempUnmapPage\n");
}

static
VOID
WinLdrpMapApic(VOID)
{
    BOOLEAN LocalAPIC;
    LARGE_INTEGER MsrValue;
    ULONG CpuInfo[4];
    ULONG64 APICAddress;

    TRACE(">>> WinLdrpMapApic\n");

    /* Check if we have a local APIC */
    __cpuid((int*)CpuInfo, 1);
    LocalAPIC = (((CpuInfo[3] >> 9) & 1) != 0);

    /* If there is no APIC, just return */
    if (!LocalAPIC)
    {
        WARN("No APIC found.\n");
        return;
    }

    /* Read the APIC Address */
    MsrValue.QuadPart = __readmsr(0x1B);
    APICAddress = (MsrValue.LowPart & 0xFFFFF000);

    TRACE("Local APIC detected at address 0x%x\n",
        APICAddress);

    /* Map it */
    MempMapSinglePage(APIC_BASE, APICAddress);
}

static
BOOLEAN
WinLdrMapSpecialPages(VOID)
{
    PHARDWARE_PTE PpeBase, PdeBase;

    /* Map KI_USER_SHARED_DATA */
    if (!MempMapSinglePage(KI_USER_SHARED_DATA, SharedUserDataPfn * PAGE_SIZE))
    {
        ERR("Could not map KI_USER_SHARED_DATA\n");
        return FALSE;
    }

    /* Map the APIC page */
    WinLdrpMapApic();

    /* Map the page tables for 4 MB HAL address space. */
    PpeBase = MempGetOrCreatePageDir(PxeBase, VAtoPXI(MM_HAL_VA_START));
    PdeBase = MempGetOrCreatePageDir(PpeBase, VAtoPPI(MM_HAL_VA_START));
    MempGetOrCreatePageDir(PdeBase, VAtoPDI(MM_HAL_VA_START));
    MempGetOrCreatePageDir(PdeBase, VAtoPDI(MM_HAL_VA_START + 2 * 1024 * 1024));

    return TRUE;
}

static
VOID
Amd64SetupGdt(PVOID GdtBase, ULONG64 TssBase)
{
    PKGDTENTRY64 Entry;
    KDESCRIPTOR GdtDesc;
    TRACE("Amd64SetupGdt(GdtBase = %p, TssBase = %p)\n", GdtBase, TssBase);

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
    TRACE("Leave Amd64SetupGdt()\n");
}

static
VOID
Amd64SetupIdt(PVOID IdtBase)
{
    KDESCRIPTOR IdtDesc, OldIdt;
    //ULONG Size;
    TRACE("Amd64SetupIdt(IdtBase = %p)\n", IdtBase);

    /* Get old IDT */
    __sidt(&OldIdt.Limit);

    /* Copy the old IDT */
    //Size =  min(OldIdt.Limit + 1, NUM_IDT * sizeof(KIDTENTRY));
    //RtlCopyMemory(IdtBase, (PVOID)OldIdt.Base, Size);

    /* Setup the new IDT descriptor */
    IdtDesc.Base = IdtBase;
    IdtDesc.Limit = NUM_IDT * sizeof(KIDTENTRY) - 1;

    /* Set the new IDT */
    __lidt(&IdtDesc.Limit);
    TRACE("Leave Amd64SetupIdt()\n");
}

VOID
WinLdrSetProcessorContext(
    _In_ USHORT OperatingSystemVersion)
{
    TRACE("WinLdrSetProcessorContext\n");

    /* Disable Interrupts */
    _disable();

    /* Re-initialize EFLAGS */
    __writeeflags(0);

    /* Set the new PML4 */
    __writecr3((ULONG64)PxeBase);

    /* Get kernel mode address of gdt / idt */
    GdtIdt = (PVOID)((ULONG64)GdtIdt + KSEG0_BASE);

    /* Create gdt entries and load gdtr */
    Amd64SetupGdt(GdtIdt, KSEG0_BASE | (TssBasePage << MM_PAGE_SHIFT));

    /* Copy old Idt and set idtr */
    Amd64SetupIdt((PVOID)((ULONG64)GdtIdt + NUM_GDT * sizeof(KGDTENTRY)));

    /* LDT is unused */
//    __lldt(0);

    /* Load TSR */
    __ltr(KGDT64_SYS_TSS);

    TRACE("leave WinLdrSetProcessorContext\n");
}

void WinLdrSetupMachineDependent(PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PVOID SharedUserDataAddress = NULL;
    ULONG_PTR Tss = 0;
    ULONG BlockSize, NumPages;

    LoaderBlock->u.I386.CommonDataArea = (PVOID)DbgPrint; // HACK
    LoaderBlock->u.I386.MachineType = MACHINE_TYPE_ISA;

    /* Allocate 1 page for SharedUserData */
    SharedUserDataAddress = MmAllocateMemoryWithType(MM_PAGE_SIZE, LoaderStartupPcrPage);
    SharedUserDataPfn = (ULONG_PTR)SharedUserDataAddress >> MM_PAGE_SHIFT;
    if (SharedUserDataAddress == NULL)
    {
        UiMessageBox("Can't allocate SharedUserData page.");
        return;
    }
    RtlZeroMemory(SharedUserDataAddress, MM_PAGE_SIZE);

    /* Allocate TSS */
    BlockSize = (sizeof(KTSS) + MM_PAGE_SIZE) & ~(MM_PAGE_SIZE - 1);
    Tss = (ULONG_PTR)MmAllocateMemoryWithType(BlockSize, LoaderMemoryData);
    TssBasePage = Tss >> MM_PAGE_SHIFT;

    /* Allocate space for new GDT + IDT */
    BlockSize = NUM_GDT * sizeof(KGDTENTRY) + NUM_IDT * sizeof(KIDTENTRY);
    NumPages = (BlockSize + MM_PAGE_SIZE - 1) >> MM_PAGE_SHIFT;
    GdtIdt = (PKGDTENTRY)MmAllocateMemoryWithType(NumPages * MM_PAGE_SIZE, LoaderMemoryData);
    if (GdtIdt == NULL)
    {
        UiMessageBox("Can't allocate pages for GDT+IDT!");
        return;
    }

    /* Zero newly prepared GDT+IDT */
    RtlZeroMemory(GdtIdt, NumPages << MM_PAGE_SHIFT);

    // Before we start mapping pages, create a block of memory, which will contain
    // PDE and PTEs
    if (MempAllocatePageTables() == FALSE)
    {
        // FIXME: bugcheck
    }

    /* Map KI_USER_SHARED_DATA, Apic and HAL space */
    WinLdrMapSpecialPages();
}


VOID
MempDump(VOID)
{
}
