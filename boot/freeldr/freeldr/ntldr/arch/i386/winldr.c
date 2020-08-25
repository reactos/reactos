/*
 * PROJECT:         EFI Windows Loader
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            boot/freeldr/freeldr/arch/i386/winldr.c
 * PURPOSE:         Memory related routines
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <freeldr.h>
#include <ndk/asm.h>
#include "../../winldr.h"

#include <debug.h>
DBG_DEFAULT_CHANNEL(WINDOWS);

// This is needed because headers define wrong one for ReactOS
#undef KIP0PCRADDRESS
#define KIP0PCRADDRESS                      0xffdff000

#define SELFMAP_ENTRY       0x300

// This is needed only for SetProcessorContext routine
#pragma pack(2)
typedef struct
{
    USHORT Limit;
    ULONG Base;
} GDTIDT;
#pragma pack(4)

/*
 * Consider adding these definitions into
 * ntoskrnl/include/internal/i386/intrin_i.h and/or the NDK.
 */

#define DPL_SYSTEM  0
#define DPL_USER    3

#define TYPE_TSS16A 0x01    // 16-bit Task State Segment (Available)
#define TYPE_LDT    0x02    // Local Descriptor Table
#define TYPE_TSS16B 0x03    // 16-bit Task State Segment (Busy)
#define TYPE_CALL16 0x04    // 16-bit Call Gate
#define TYPE_TASK   0x05    // Task Gate (I386_TASK_GATE)
#define TYPE_INT16  0x06    // 16-bit Interrupt Gate
#define TYPE_TRAP16 0x07    // 16-bit Trap Gate
// #define TYPE_RESERVED_1 0x08
#define TYPE_TSS32A 0x09    // 32-bit Task State Segment (Available) (I386_TSS)
// #define TYPE_RESERVED_2 0x0A
#define TYPE_TSS32B 0x0B    // 32-bit Task State Segment (Busy) (I386_ACTIVE_TSS)
#define TYPE_CALL32 0x0C    // 32-bit Call Gate (I386_CALL_GATE)
// #define TYPE_RESERVED_3 0x0D
#define TYPE_INT32  0x0E    // 32-bit Interrupt Gate (I386_INTERRUPT_GATE)
#define TYPE_TRAP32 0x0F    // 32-bit Trap Gate (I386_TRAP_GATE)

#define DESCRIPTOR_ACCESSED     0x1
#define DESCRIPTOR_READ_WRITE   0x2
#define DESCRIPTOR_EXECUTE_READ 0x2
#define DESCRIPTOR_EXPAND_DOWN  0x4
#define DESCRIPTOR_CONFORMING   0x4
#define DESCRIPTOR_CODE         0x8

#define TYPE_CODE   (0x10 | DESCRIPTOR_CODE | DESCRIPTOR_EXECUTE_READ)
#define TYPE_DATA   (0x10 | DESCRIPTOR_READ_WRITE)

FORCEINLINE
PKGDTENTRY
KiGetGdtEntry(
    IN PVOID pGdt,
    IN USHORT Selector)
{
    return (PKGDTENTRY)((ULONG_PTR)pGdt + (Selector & ~RPL_MASK));
}

FORCEINLINE
VOID
KiSetGdtDescriptorBase(
    IN OUT PKGDTENTRY Entry,
    IN ULONG32 Base)
{
    Entry->BaseLow = (USHORT)(Base & 0xffff);
    Entry->HighWord.Bytes.BaseMid = (UCHAR)((Base >> 16) & 0xff);
    Entry->HighWord.Bytes.BaseHi  = (UCHAR)((Base >> 24) & 0xff);
    // Entry->BaseUpper = (ULONG)(Base >> 32);
}

FORCEINLINE
VOID
KiSetGdtDescriptorLimit(
    IN OUT PKGDTENTRY Entry,
    IN ULONG Limit)
{
    if (Limit < 0x100000)
    {
        Entry->HighWord.Bits.Granularity = 0;
    }
    else
    {
        Limit >>= 12;
        Entry->HighWord.Bits.Granularity = 1;
    }
    Entry->LimitLow = (USHORT)(Limit & 0xffff);
    Entry->HighWord.Bits.LimitHi = ((Limit >> 16) & 0x0f);
}

VOID
KiSetGdtEntryEx(
    IN OUT PKGDTENTRY Entry,
    IN ULONG32 Base,
    IN ULONG Limit,
    IN UCHAR Type,
    IN UCHAR Dpl,
    IN BOOLEAN Granularity,
    IN UCHAR SegMode) // 0: 16-bit, 1: 32-bit, 2: 64-bit
{
    KiSetGdtDescriptorBase(Entry, Base);
    KiSetGdtDescriptorLimit(Entry, Limit);
    Entry->HighWord.Bits.Type = (Type & 0x1f);
    Entry->HighWord.Bits.Dpl  = (Dpl & 0x3);
    Entry->HighWord.Bits.Pres = (Type != 0); // Present, must be 1 when the GDT entry is valid.
    Entry->HighWord.Bits.Sys  = 0;           // System
    Entry->HighWord.Bits.Reserved_0  = 0;    // LongMode = !!(SegMode & 1);
    Entry->HighWord.Bits.Default_Big = !!(SegMode & 2);
    Entry->HighWord.Bits.Granularity |= !!Granularity; // The flag may have been already set by KiSetGdtDescriptorLimit().
    // Entry->MustBeZero = 0;
}

FORCEINLINE
VOID
KiSetGdtEntry(
    IN OUT PKGDTENTRY Entry,
    IN ULONG32 Base,
    IN ULONG Limit,
    IN UCHAR Type,
    IN UCHAR Dpl,
    IN UCHAR SegMode) // 0: 16-bit, 1: 32-bit, 2: 64-bit
{
    KiSetGdtEntryEx(Entry, Base, Limit, Type, Dpl, FALSE, SegMode);
}

#if 0
VOID
DumpGDTEntry(ULONG_PTR Base, ULONG Selector)
{
    PKGDTENTRY pGdt = (PKGDTENTRY)((ULONG_PTR)Base + Selector);

    TRACE("\n"
          "Selector 0x%04x\n"
          "===============\n"
          "LimitLow               = 0x%04x\n"
          "BaseLow                = 0x%04x\n"
          "HighWord.Bytes.BaseMid = 0x%02x\n"
          "HighWord.Bytes.Flags1  = 0x%02x\n"
          "HighWord.Bytes.Flags2  = 0x%02x\n"
          "HighWord.Bytes.BaseHi  = 0x%02x\n"
          "\n",
          Selector,
          pGdt->LimitLow, pGdt->BaseLow,
          pGdt->HighWord.Bytes.BaseMid,
          pGdt->HighWord.Bytes.Flags1,
          pGdt->HighWord.Bytes.Flags2,
          pGdt->HighWord.Bytes.BaseHi);
}
#endif

/* GLOBALS *******************************************************************/

PHARDWARE_PTE PDE;
PHARDWARE_PTE HalPageTable;

PUCHAR PhysicalPageTablesBuffer;
PUCHAR KernelPageTablesBuffer;
ULONG PhysicalPageTables;
ULONG KernelPageTables;

ULONG PcrBasePage;
ULONG TssBasePage;
PVOID GdtIdt;

/* FUNCTIONS *****************************************************************/

static
BOOLEAN
MempAllocatePageTables(VOID)
{
    ULONG NumPageTables, TotalSize;
    PUCHAR Buffer;
    // It's better to allocate PDE + PTEs contiguous

    // Max number of entries = MaxPageNum >> 10
    // FIXME: This is a number to describe ALL physical memory
    // and windows doesn't expect ALL memory mapped...
    NumPageTables = TotalPagesInLookupTable >> 10;

    TRACE("NumPageTables = %d\n", NumPageTables);

    // Allocate memory block for all these things:
    // PDE, HAL mapping page table, physical mapping, kernel mapping
    TotalSize = (1 + 1 + NumPageTables * 2) * MM_PAGE_SIZE;

    // PDE+HAL+KernelPTEs == MemoryData
    Buffer = MmAllocateMemoryWithType(TotalSize, LoaderMemoryData);

    // Physical PTEs = FirmwareTemporary
    PhysicalPageTablesBuffer = (PUCHAR)Buffer + TotalSize - NumPageTables*MM_PAGE_SIZE;
    MmSetMemoryType(PhysicalPageTablesBuffer,
                    NumPageTables*MM_PAGE_SIZE,
                    LoaderFirmwareTemporary);

    // This check is now redundant
    if (Buffer + (TotalSize - NumPageTables*MM_PAGE_SIZE) !=
        PhysicalPageTablesBuffer)
    {
        TRACE("There was a problem allocating two adjacent blocks of memory!");
    }

    if (Buffer == NULL || PhysicalPageTablesBuffer == NULL)
    {
        UiMessageBox("Impossible to allocate memory block for page tables!");
        return FALSE;
    }

    // Zero all this memory block
    RtlZeroMemory(Buffer, TotalSize);

    // Set up pointers correctly now
    PDE = (PHARDWARE_PTE)Buffer;

    // Map the page directory at 0xC0000000 (maps itself)
    PDE[SELFMAP_ENTRY].PageFrameNumber = (ULONG)PDE >> MM_PAGE_SHIFT;
    PDE[SELFMAP_ENTRY].Valid = 1;
    PDE[SELFMAP_ENTRY].Write = 1;

    // The last PDE slot is allocated for HAL's memory mapping (Virtual Addresses 0xFFC00000 - 0xFFFFFFFF)
    HalPageTable = (PHARDWARE_PTE)&Buffer[MM_PAGE_SIZE*1];

    // Map it
    PDE[1023].PageFrameNumber = (ULONG)HalPageTable >> MM_PAGE_SHIFT;
    PDE[1023].Valid = 1;
    PDE[1023].Write = 1;

    // Store pointer to the table for easier access
    KernelPageTablesBuffer = &Buffer[MM_PAGE_SIZE*2];

    // Zero counters of page tables used
    PhysicalPageTables = 0;
    KernelPageTables = 0;

    return TRUE;
}

static
VOID
MempAllocatePTE(ULONG Entry, PHARDWARE_PTE *PhysicalPT, PHARDWARE_PTE *KernelPT)
{
    //Print(L"Creating PDE Entry %X\n", Entry);

    // Identity mapping
    *PhysicalPT = (PHARDWARE_PTE)&PhysicalPageTablesBuffer[PhysicalPageTables*MM_PAGE_SIZE];
    PhysicalPageTables++;

    PDE[Entry].PageFrameNumber = (ULONG)*PhysicalPT >> MM_PAGE_SHIFT;
    PDE[Entry].Valid = 1;
    PDE[Entry].Write = 1;

    if (Entry+(KSEG0_BASE >> 22) > 1023)
    {
        TRACE("WARNING! Entry: %X > 1023\n", Entry+(KSEG0_BASE >> 22));
    }

    // Kernel-mode mapping
    *KernelPT = (PHARDWARE_PTE)&KernelPageTablesBuffer[KernelPageTables*MM_PAGE_SIZE];
    KernelPageTables++;

    PDE[Entry+(KSEG0_BASE >> 22)].PageFrameNumber = ((ULONG)*KernelPT >> MM_PAGE_SHIFT);
    PDE[Entry+(KSEG0_BASE >> 22)].Valid = 1;
    PDE[Entry+(KSEG0_BASE >> 22)].Write = 1;
}

BOOLEAN
MempSetupPaging(IN PFN_NUMBER StartPage,
                IN PFN_COUNT NumberOfPages,
                IN BOOLEAN KernelMapping)
{
    PHARDWARE_PTE PhysicalPT;
    PHARDWARE_PTE KernelPT;
    PFN_COUNT Entry, Page;

    TRACE("MempSetupPaging: SP 0x%X, Number: 0x%X, Kernel: %s\n",
       StartPage, NumberOfPages, KernelMapping ? "yes" : "no");

    // HACK
    if (StartPage+NumberOfPages >= 0x80000)
    {
        //
        // We cannot map this as it requires more than 1 PDE
        // and in fact it's not possible at all ;)
        //
        //Print(L"skipping...\n");
        return TRUE;
    }

    //
    // Now actually set up the page tables for identity mapping
    //
    for (Page = StartPage; Page < StartPage + NumberOfPages; Page++)
    {
        Entry = Page >> 10;

        if (((PULONG)PDE)[Entry] == 0)
        {
            MempAllocatePTE(Entry, &PhysicalPT, &KernelPT);
        }
        else
        {
            PhysicalPT = (PHARDWARE_PTE)(PDE[Entry].PageFrameNumber << MM_PAGE_SHIFT);
            KernelPT = (PHARDWARE_PTE)(PDE[Entry+(KSEG0_BASE >> 22)].PageFrameNumber << MM_PAGE_SHIFT);
        }

        PhysicalPT[Page & 0x3ff].PageFrameNumber = Page;
        PhysicalPT[Page & 0x3ff].Valid = (Page != 0);
        PhysicalPT[Page & 0x3ff].Write = (Page != 0);

        if (KernelMapping)
        {
             if (KernelPT[Page & 0x3ff].Valid) WARN("xxx already mapped \n");
            KernelPT[Page & 0x3ff].PageFrameNumber = Page;
            KernelPT[Page & 0x3ff].Valid = (Page != 0);
            KernelPT[Page & 0x3ff].Write = (Page != 0);
        }
    }

    return TRUE;
}

VOID
MempUnmapPage(PFN_NUMBER Page)
{
    PHARDWARE_PTE KernelPT;
    PFN_NUMBER Entry = (Page >> 10) + (KSEG0_BASE >> 22);

    /* Don't unmap page directory or HAL entries */
    if (Entry == SELFMAP_ENTRY || Entry == 1023)
        return;

    if (PDE[Entry].Valid)
    {
        KernelPT = (PHARDWARE_PTE)(PDE[Entry].PageFrameNumber << MM_PAGE_SHIFT);

        if (KernelPT)
        {
            KernelPT[Page & 0x3ff].PageFrameNumber = 0;
            KernelPT[Page & 0x3ff].Valid = 0;
            KernelPT[Page & 0x3ff].Write = 0;
        }
    }
}

static
VOID
WinLdrpMapApic(VOID)
{
    BOOLEAN LocalAPIC;
    LARGE_INTEGER MsrValue;
    ULONG APICAddress, CpuInfo[4];

    /* Check if we have a local APIC */
    __cpuid((int*)CpuInfo, 1);
    LocalAPIC = (((CpuInfo[3] >> 9) & 1) != 0);

    /* If there is no APIC, just return */
    if (!LocalAPIC)
        return;

    /* Read the APIC Address */
    MsrValue.QuadPart = __readmsr(0x1B);
    APICAddress = (MsrValue.LowPart & 0xFFFFF000);

    TRACE("Local APIC detected at address 0x%x\n",
        APICAddress);

    /* Map it */
    HalPageTable[(APIC_BASE - 0xFFC00000) >> MM_PAGE_SHIFT].PageFrameNumber
        = APICAddress >> MM_PAGE_SHIFT;
    HalPageTable[(APIC_BASE - 0xFFC00000) >> MM_PAGE_SHIFT].Valid = 1;
    HalPageTable[(APIC_BASE - 0xFFC00000) >> MM_PAGE_SHIFT].Write = 1;
    HalPageTable[(APIC_BASE - 0xFFC00000) >> MM_PAGE_SHIFT].WriteThrough = 1;
    HalPageTable[(APIC_BASE - 0xFFC00000) >> MM_PAGE_SHIFT].CacheDisable = 1;
}

static
BOOLEAN
WinLdrMapSpecialPages(void)
{
    TRACE("HalPageTable: 0x%X\n", HalPageTable);

    /*
     * The Page Tables have been setup, make special handling
     * for the boot processor PCR and KI_USER_SHARED_DATA.
     */
    HalPageTable[(KI_USER_SHARED_DATA - 0xFFC00000) >> MM_PAGE_SHIFT].PageFrameNumber = PcrBasePage+1;
    HalPageTable[(KI_USER_SHARED_DATA - 0xFFC00000) >> MM_PAGE_SHIFT].Valid = 1;
    HalPageTable[(KI_USER_SHARED_DATA - 0xFFC00000) >> MM_PAGE_SHIFT].Write = 1;

    HalPageTable[(KIP0PCRADDRESS - 0xFFC00000) >> MM_PAGE_SHIFT].PageFrameNumber = PcrBasePage;
    HalPageTable[(KIP0PCRADDRESS - 0xFFC00000) >> MM_PAGE_SHIFT].Valid = 1;
    HalPageTable[(KIP0PCRADDRESS - 0xFFC00000) >> MM_PAGE_SHIFT].Write = 1;

    /* Map APIC */
    WinLdrpMapApic();

    /* Map VGA memory */
    //VideoMemoryBase = MmMapIoSpace(0xb8000, 4000, MmNonCached);
    //TRACE("VideoMemoryBase: 0x%X\n", VideoMemoryBase);

    return TRUE;
}

#define ExtendedBIOSDataArea ((PULONG)0x740)
#define ExtendedBIOSDataSize ((PULONG)0x744)
#define RomFontPointers ((PULONG)0x700)

static
void WinLdrSetupSpecialDataPointers(VOID)
{
    /* Get the address of the BIOS ROM fonts. Win 2003 videoprt reads these
       values from address 0x700 .. 0x718 and store them in the registry
       in HKLM\System\CurrentControlSet\Control\Wow\RomFontPointers */
    MachVideoGetFontsFromFirmware(RomFontPointers);

    /* Store address of the extended BIOS data area in 0x740 */
    MachGetExtendedBIOSData(ExtendedBIOSDataArea, ExtendedBIOSDataSize);

    if (*ExtendedBIOSDataArea == 0 && *ExtendedBIOSDataSize == 0)
    {
        WARN("Couldn't get address of extended BIOS data area\n");
    }
    else
    {
        TRACE("*ExtendedBIOSDataArea = 0x%lx\n", *ExtendedBIOSDataArea);
    }
}

void WinLdrSetupMachineDependent(PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    ULONG TssSize;
    //ULONG TssPages;
    ULONG_PTR Pcr = 0;
    ULONG_PTR Tss = 0;
    ULONG BlockSize, NumPages;

    LoaderBlock->u.I386.CommonDataArea = NULL; // Force No ABIOS support
    LoaderBlock->u.I386.MachineType = MACHINE_TYPE_ISA;

    /* Allocate 2 pages for PCR: one for the boot processor PCR and one for KI_USER_SHARED_DATA */
    Pcr = (ULONG_PTR)MmAllocateMemoryWithType(2 * MM_PAGE_SIZE, LoaderStartupPcrPage);
    PcrBasePage = Pcr >> MM_PAGE_SHIFT;
    if (Pcr == 0)
    {
        UiMessageBox("Could not allocate PCR.");
        return;
    }

    /* Allocate TSS */
    TssSize = (sizeof(KTSS) + MM_PAGE_SIZE) & ~(MM_PAGE_SIZE - 1);
    //TssPages = TssSize / MM_PAGE_SIZE;

    Tss = (ULONG_PTR)MmAllocateMemoryWithType(TssSize, LoaderMemoryData);
    TssBasePage = Tss >> MM_PAGE_SHIFT;
    if (Tss == 0)
    {
        UiMessageBox("Could not allocate TSS.");
        return;
    }

    /* Allocate space for new GDT + IDT */
    BlockSize = NUM_GDT*sizeof(KGDTENTRY) + NUM_IDT*sizeof(KIDTENTRY);//FIXME: Use GDT/IDT limits here?
    NumPages = (BlockSize + MM_PAGE_SIZE - 1) >> MM_PAGE_SHIFT;
    GdtIdt = (PKGDTENTRY)MmAllocateMemoryWithType(NumPages * MM_PAGE_SIZE, LoaderMemoryData);
    if (GdtIdt == NULL)
    {
        UiMessageBox("Could not allocate pages for GDT+IDT!");
        return;
    }

    /* Zero newly prepared GDT+IDT */
    RtlZeroMemory(GdtIdt, NumPages << MM_PAGE_SHIFT);

    // Before we start mapping pages, create a block of memory, which will contain
    // PDE and PTEs
    if (MempAllocatePageTables() == FALSE)
    {
        BugCheck("MempAllocatePageTables failed!\n");
    }

    /* Map stuff like PCR, KI_USER_SHARED_DATA and Apic */
    WinLdrMapSpecialPages();

    /* Set some special fields */
    WinLdrSetupSpecialDataPointers();
}


VOID
WinLdrSetProcessorContext(void)
{
    GDTIDT GdtDesc, IdtDesc, OldIdt;
    PKGDTENTRY    pGdt;
    PKIDTENTRY    pIdt;
    USHORT Ldt = 0;
    ULONG Pcr;
    ULONG Tss;
    //ULONG i;

    Pcr = KIP0PCRADDRESS;
    Tss = KSEG0_BASE | (TssBasePage << MM_PAGE_SHIFT);

    TRACE("GdtIdt %p, Pcr %p, Tss 0x%08x\n",
          GdtIdt, Pcr, Tss);

    /* Enable paging */
    //BS->ExitBootServices(ImageHandle,MapKey);

    /* Disable Interrupts */
    _disable();

    /* Re-initialize EFLAGS */
    __writeeflags(0);

    /* Set the PDBR */
    __writecr3((ULONG_PTR)PDE);

    /* Enable paging by modifying CR0 */
    __writecr0(__readcr0() | CR0_PG);

    /* The Kernel expects the boot processor PCR to be zero-filled on startup */
    RtlZeroMemory((PVOID)Pcr, MM_PAGE_SIZE);

    /* Get old values of GDT and IDT */
    Ke386GetGlobalDescriptorTable(&GdtDesc);
    __sidt(&IdtDesc);

    /* Save old IDT */
    OldIdt.Base  = IdtDesc.Base;
    OldIdt.Limit = IdtDesc.Limit;

    /* Prepare new IDT+GDT */
    GdtDesc.Base  = KSEG0_BASE | (ULONG_PTR)GdtIdt;
    GdtDesc.Limit = NUM_GDT * sizeof(KGDTENTRY) - 1;
    IdtDesc.Base  = (ULONG)((PUCHAR)GdtDesc.Base + GdtDesc.Limit + 1);
    IdtDesc.Limit = NUM_IDT * sizeof(KIDTENTRY) - 1;

    // ========================
    // Fill all descriptors now
    // ========================

    pGdt = (PKGDTENTRY)GdtDesc.Base;
    pIdt = (PKIDTENTRY)IdtDesc.Base;

    /* KGDT_NULL (0x00) Null Selector - Unused */
    KiSetGdtEntry(KiGetGdtEntry(pGdt, KGDT_NULL), 0x0000, 0, 0, DPL_SYSTEM, 0);
    // DumpGDTEntry(GdtDesc.Base, KGDT_NULL);

    /* KGDT_R0_CODE (0x08) Ring 0 Code selector - Flat 4Gb */
    KiSetGdtEntry(KiGetGdtEntry(pGdt, KGDT_R0_CODE), 0x0000, 0xFFFFFFFF,
                  TYPE_CODE, DPL_SYSTEM, 2);
    // DumpGDTEntry(GdtDesc.Base, KGDT_R0_CODE);

    /* KGDT_R0_DATA (0x10) Ring 0 Data selector - Flat 4Gb */
    KiSetGdtEntry(KiGetGdtEntry(pGdt, KGDT_R0_DATA), 0x0000, 0xFFFFFFFF,
                  TYPE_DATA, DPL_SYSTEM, 2);
    // DumpGDTEntry(GdtDesc.Base, KGDT_R0_DATA);

    /* KGDT_R3_CODE (0x18) Ring 3 Code Selector - Flat 2Gb */
    KiSetGdtEntry(KiGetGdtEntry(pGdt, KGDT_R3_CODE), 0x0000, 0xFFFFFFFF,
                  TYPE_CODE, DPL_USER, 2);
    // DumpGDTEntry(GdtDesc.Base, KGDT_R3_CODE);

    /* KGDT_R3_DATA (0x20) Ring 3 Data Selector - Flat 2Gb */
    KiSetGdtEntry(KiGetGdtEntry(pGdt, KGDT_R3_DATA), 0x0000, 0xFFFFFFFF,
                  TYPE_DATA, DPL_USER, 2);
    // DumpGDTEntry(GdtDesc.Base, KGDT_R3_DATA);

    /* KGDT_TSS (0x28) TSS Selector (Ring 0) */
    KiSetGdtEntry(KiGetGdtEntry(pGdt, KGDT_TSS), (ULONG32)Tss,
                  0x78-1 /* FIXME: Check this; would be sizeof(KTSS)-1 */,
                  TYPE_TSS32A, DPL_SYSTEM, 0);
    // DumpGDTEntry(GdtDesc.Base, KGDT_TSS);

    /*
     * KGDT_R0_PCR (0x30) PCR Selector (Ring 0)
     *
     * IMPORTANT COMPATIBILITY NOTE:
     * Windows <= Server 2003 sets a Base == KIP0PCRADDRESS (0xffdff000 on x86)
     * with a Limit of 1 page (LimitLow == 1, Granularity == 1, that is interpreted
     * as a "Limit" == 0x1fff but is incorrect, of course).
     * On Windows Longhorn/Vista+ however, the Base is not hardcoded to KIP0PCRADDRESS
     * and the limit is set in bytes (Granularity == 0):
     * Longhorn/Vista reports LimitLow == 0x0fff == MM_PAGE_SIZE - 1, whereas
     * Windows 7+ uses larger sizes there (not aligned on a page boundary).
     */
#if 1
    /* Server 2003 way */
    KiSetGdtEntryEx(KiGetGdtEntry(pGdt, KGDT_R0_PCR), (ULONG32)Pcr, 0x1,
                    TYPE_DATA, DPL_SYSTEM, TRUE, 2);
#else
    /* Vista+ way */
    KiSetGdtEntry(KiGetGdtEntry(pGdt, KGDT_R0_PCR), (ULONG32)Pcr, MM_PAGE_SIZE - 1,
                  TYPE_DATA, DPL_SYSTEM, 2);
#endif
    // DumpGDTEntry(GdtDesc.Base, KGDT_R0_PCR);

    /* KGDT_R3_TEB (0x38) Thread Environment Block Selector (Ring 3) */
    KiSetGdtEntry(KiGetGdtEntry(pGdt, KGDT_R3_TEB), 0x0000, 0x0FFF,
                  TYPE_DATA | DESCRIPTOR_ACCESSED, DPL_USER, 2);
    // DumpGDTEntry(GdtDesc.Base, KGDT_R3_TEB);

    /* KGDT_VDM_TILE (0x40) VDM BIOS Data Area Selector (Ring 3) */
    KiSetGdtEntry(KiGetGdtEntry(pGdt, KGDT_VDM_TILE), 0x0400, 0xFFFF,
                  TYPE_DATA, DPL_USER, 0);
    // DumpGDTEntry(GdtDesc.Base, KGDT_VDM_TILE);

    /* KGDT_LDT (0x48) LDT Selector (Reserved) */
    KiSetGdtEntry(KiGetGdtEntry(pGdt, KGDT_LDT), 0x0000, 0, 0, DPL_SYSTEM, 0);
    // DumpGDTEntry(GdtDesc.Base, KGDT_LDT);

    /* KGDT_DF_TSS (0x50) Double-Fault Task Switch Selector (Ring 0) */
    KiSetGdtEntry(KiGetGdtEntry(pGdt, KGDT_DF_TSS), 0x20000 /* FIXME: Not correct! */,
                  0xFFFF /* FIXME: Not correct! */,
                  TYPE_TSS32A, DPL_SYSTEM, 0);
    // DumpGDTEntry(GdtDesc.Base, KGDT_DF_TSS);

    /* KGDT_NMI_TSS (0x58) NMI Task Switch Selector (Ring 0) */
    KiSetGdtEntry(KiGetGdtEntry(pGdt, KGDT_NMI_TSS), 0x20000, 0xFFFF,
                  TYPE_CODE, DPL_SYSTEM, 0);
    // DumpGDTEntry(GdtDesc.Base, KGDT_NMI_TSS);

    /* Selector (0x60) - Unused (Reserved) on Windows Longhorn/Vista+ */
    KiSetGdtEntry(KiGetGdtEntry(pGdt, 0x60), 0x20000 /* FIXME: Maybe not correct, but noone cares */,
                  0xFFFF, TYPE_DATA, DPL_SYSTEM, 0);
    // DumpGDTEntry(GdtDesc.Base, 0x60);

    /* Video Display Buffer Selector (0x68) - Unused (Reserved) on Windows Longhorn/Vista+ */
    KiSetGdtEntry(KiGetGdtEntry(pGdt, 0x68), 0xB8000, 0x3FFF,
                  TYPE_DATA, DPL_SYSTEM, 0);
    // DumpGDTEntry(GdtDesc.Base, 0x68);

    /*
     * GDT Alias Selector (points to GDT) (0x70)
     *
     * IMPORTANT COMPATIBILITY NOTE:
     * The Base is not fixed to 0xFFFF7000 on Windows Longhorn/Vista+.
     */
    KiSetGdtEntry(KiGetGdtEntry(pGdt, 0x70), 0xFFFF7000 /* GdtDesc.Base ?? */, GdtDesc.Limit,
                  TYPE_DATA, DPL_SYSTEM, 0);
    // DumpGDTEntry(GdtDesc.Base, 0x70);

    /*
     * Windows <= Server 2003 defines three additional unused but valid
     * descriptors there, possibly for debugging purposes only.
     * They are not present on Windows Longhorn/Vista+.
     */
    // KiSetGdtEntry(KiGetGdtEntry(pGdt, 0x78), 0x80400000, 0xffff, TYPE_CODE, DPL_SYSTEM, 0);
    // KiSetGdtEntry(KiGetGdtEntry(pGdt, 0x80), 0x80400000, 0xffff, TYPE_DATA, DPL_SYSTEM, 0);
    // KiSetGdtEntry(KiGetGdtEntry(pGdt, 0x88), 0x0000, 0, TYPE_DATA, DPL_SYSTEM, 0);

    /* Copy the old IDT */
    RtlCopyMemory(pIdt, (PVOID)OldIdt.Base, OldIdt.Limit + 1);

    /* Mask interrupts */
    //asm("cli\n"); // they are already masked before enabling paged mode

    /* Load GDT+IDT */
    Ke386SetGlobalDescriptorTable(&GdtDesc);
    __lidt(&IdtDesc);

    /* Jump to proper CS and clear prefetch queue */
#if defined(__GNUC__) || defined(__clang__)
    asm("ljmp    $0x08, $1f\n"
        "1:\n");
#elif defined(_MSC_VER)
    /* We cannot express the above in MASM so we use this far return instead */
    __asm
    {
        push 8
        push offset resume
        retf
        resume:
    };
#else
#error
#endif

    /* Set SS selector */
    Ke386SetSs(KGDT_R0_DATA);

    /* Set DS and ES selectors */
    Ke386SetDs(KGDT_R0_DATA);
    Ke386SetEs(KGDT_R0_DATA); // This is vital for rep stosd

    /* LDT = not used ever, thus set to 0 */
    Ke386SetLocalDescriptorTable(Ldt);

    /* Load TSR */
    Ke386SetTr(KGDT_TSS);

    /* Clear GS */
    Ke386SetGs(0);

    /* Set FS to PCR */
    Ke386SetFs(KGDT_R0_PCR);

    // Real end of the function, just for information
    /* do not uncomment!
    pop edi;
    pop esi;
    pop ebx;
    mov esp, ebp;
    pop ebp;
    ret
    */
}

#if DBG
VOID
MempDump(VOID)
{
    PULONG PDE_Addr=(PULONG)PDE;//0xC0300000;
    int i, j;

    TRACE("\nPDE\n");

    for (i=0; i<128; i++)
    {
        TRACE("0x%04X | ", i*8);

        for (j=0; j<8; j++)
        {
            TRACE("0x%08X ", PDE_Addr[i*8+j]);
        }

        TRACE("\n");
    }
}
#endif
