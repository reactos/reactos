/*
 * PROJECT:         EFI Windows Loader
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            freeldr/winldr/i386/wlmemory.c
 * PURPOSE:         Memory related routines
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES ***************************************************************/

#include <freeldr.h>

#include <ndk/asm.h>
#include <debug.h>

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

DBG_DEFAULT_CHANNEL(WINDOWS);

/* GLOBALS ***************************************************************/

PHARDWARE_PTE PDE;
PHARDWARE_PTE HalPageTable;

PUCHAR PhysicalPageTablesBuffer;
PUCHAR KernelPageTablesBuffer;
ULONG PhysicalPageTables;
ULONG KernelPageTables;

ULONG PcrBasePage;
ULONG TssBasePage;
PVOID GdtIdt;

/* FUNCTIONS **************************************************************/

BOOLEAN
MempAllocatePageTables()
{
    ULONG NumPageTables, TotalSize;
    PUCHAR Buffer;
    // It's better to allocate PDE + PTEs contigiuos

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
        // We can't map this as it requires more than 1 PDE
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

VOID
WinLdrpMapApic()
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

BOOLEAN
WinLdrMapSpecialPages(void)
{

    //VideoDisplayString(L"Hello from VGA, going into the kernel\n");
    TRACE("HalPageTable: 0x%X\n", HalPageTable);

    // Page Tables have been setup, make special handling for PCR and TSS
    // (which is done in BlSetupFotNt in usual ntldr)
    HalPageTable[(KI_USER_SHARED_DATA - 0xFFC00000) >> MM_PAGE_SHIFT].PageFrameNumber = PcrBasePage+1;
    HalPageTable[(KI_USER_SHARED_DATA - 0xFFC00000) >> MM_PAGE_SHIFT].Valid = 1;
    HalPageTable[(KI_USER_SHARED_DATA - 0xFFC00000) >> MM_PAGE_SHIFT].Write = 1;

    HalPageTable[(KIP0PCRADDRESS - 0xFFC00000) >> MM_PAGE_SHIFT].PageFrameNumber = PcrBasePage;
    HalPageTable[(KIP0PCRADDRESS - 0xFFC00000) >> MM_PAGE_SHIFT].Valid = 1;
    HalPageTable[(KIP0PCRADDRESS - 0xFFC00000) >> MM_PAGE_SHIFT].Write = 1;

    // Map APIC
    WinLdrpMapApic();

    // Map VGA memory
    //VideoMemoryBase = MmMapIoSpace(0xb8000, 4000, MmNonCached);
    //TRACE("VideoMemoryBase: 0x%X\n", VideoMemoryBase);

    return TRUE;
}

#define ExtendedBIOSDataArea ((PULONG)0x740)
#define ExtendedBIOSDataSize ((PULONG)0x744)
#define RomFontPointers ((PULONG)0x700)

enum
{
    INT1FhFont = 0x00,
    INT43hFont = 0x01,
    ROM_8x14CharacterFont = 0x02,
    ROM_8x8DoubleDotFontLo = 0x03,
    ROM_8x8DoubleDotFontHi = 0x04,
    ROM_AlphaAlternate = 0x05,
    ROM_8x16Font = 0x06,
    ROM_Alternate9x16Font = 0x07,
    UltraVision_8x20Font = 0x11,
    UltraVision_8x10Font = 0x12,
};

void WinLdrSetupSpecialDataPointers()
{
    REGS BiosRegs;

    /* Get the address of the bios rom fonts. Win 2003 videoprt reads these
       values from address 0x700 .. 0x718 and store them in the registry
       in HKLM\System\CurrentControlSet\Control\Wow\RomFontPointers
       Int 10h, AX=1130h, BH = pointer specifier
       returns: es:bp = address */
    BiosRegs.d.eax = 0x1130;
    BiosRegs.b.bh = ROM_8x14CharacterFont;
    Int386(0x10, &BiosRegs, &BiosRegs);
    RomFontPointers[0] = BiosRegs.w.es << 4 | BiosRegs.w.bp;

    BiosRegs.b.bh = ROM_8x8DoubleDotFontLo;
    Int386(0x10, &BiosRegs, &BiosRegs);
    RomFontPointers[1] = BiosRegs.w.es << 16 | BiosRegs.w.bp;

    BiosRegs.b.bh = ROM_8x8DoubleDotFontHi;
    Int386(0x10, &BiosRegs, &BiosRegs);
    RomFontPointers[2] = BiosRegs.w.es << 16 | BiosRegs.w.bp;

    BiosRegs.b.bh = ROM_AlphaAlternate;
    Int386(0x10, &BiosRegs, &BiosRegs);
    RomFontPointers[3] = BiosRegs.w.es << 16 | BiosRegs.w.bp;

    BiosRegs.b.bh = ROM_8x16Font;
    Int386(0x10, &BiosRegs, &BiosRegs);
    RomFontPointers[4] = BiosRegs.w.es << 16 | BiosRegs.w.bp;

    BiosRegs.b.bh = ROM_Alternate9x16Font;
    Int386(0x10, &BiosRegs, &BiosRegs);
    RomFontPointers[5] = BiosRegs.w.es << 16 | BiosRegs.w.bp;

    /* Store address of the extended bios data area in 0x740 */
    BiosRegs.d.eax = 0xC100;
    Int386(0x15, &BiosRegs, &BiosRegs);
    if (INT386_SUCCESS(BiosRegs))
    {
        *ExtendedBIOSDataArea = BiosRegs.w.es << 4;
        *ExtendedBIOSDataSize = 1024;
        TRACE("*ExtendedBIOSDataArea = 0x%lx\n", *ExtendedBIOSDataArea);
    }
    else
    {
        WARN("Couldn't get address of extended BIOS data area\n");
        *ExtendedBIOSDataArea = 0;
        *ExtendedBIOSDataSize = 0;
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

    /* Allocate 2 pages for PCR */
    Pcr = (ULONG_PTR)MmAllocateMemoryWithType(2 * MM_PAGE_SIZE, LoaderStartupPcrPage);
    PcrBasePage = Pcr >> MM_PAGE_SHIFT;

    if (Pcr == 0)
    {
        UiMessageBox("Can't allocate PCR.");
        return;
    }

    /* Allocate TSS */
    TssSize = (sizeof(KTSS) + MM_PAGE_SIZE) & ~(MM_PAGE_SIZE - 1);
    //TssPages = TssSize / MM_PAGE_SIZE;

    Tss = (ULONG_PTR)MmAllocateMemoryWithType(TssSize, LoaderMemoryData);

    TssBasePage = Tss >> MM_PAGE_SHIFT;

    /* Allocate space for new GDT + IDT */
    BlockSize = NUM_GDT*sizeof(KGDTENTRY) + NUM_IDT*sizeof(KIDTENTRY);//FIXME: Use GDT/IDT limits here?
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

    TRACE("GDtIdt %p, Pcr %p, Tss 0x%08X\n",
        GdtIdt, Pcr, Tss);

    // Enable paging
    //BS->ExitBootServices(ImageHandle,MapKey);

    // Disable Interrupts
    _disable();

    // Re-initialize EFLAGS
    __writeeflags(0);

    // Set the PDBR
    __writecr3((ULONG_PTR)PDE);

    // Enable paging by modifying CR0
    __writecr0(__readcr0() | CR0_PG);

    // Kernel expects the PCR to be zero-filled on startup
    // FIXME: Why zero it here when we can zero it right after allocation?
    RtlZeroMemory((PVOID)Pcr, MM_PAGE_SIZE); //FIXME: Why zero only 1 page when we allocate 2?

    // Get old values of GDT and IDT
    Ke386GetGlobalDescriptorTable(&GdtDesc);
    __sidt(&IdtDesc);

    // Save old IDT
    OldIdt.Base = IdtDesc.Base;
    OldIdt.Limit = IdtDesc.Limit;

    // Prepare new IDT+GDT
    GdtDesc.Base  = KSEG0_BASE | (ULONG_PTR)GdtIdt;
    GdtDesc.Limit = NUM_GDT * sizeof(KGDTENTRY) - 1;
    IdtDesc.Base  = (ULONG)((PUCHAR)GdtDesc.Base + GdtDesc.Limit + 1);
    IdtDesc.Limit = NUM_IDT * sizeof(KIDTENTRY) - 1;

    // ========================
    // Fill all descriptors now
    // ========================

    pGdt = (PKGDTENTRY)GdtDesc.Base;
    pIdt = (PKIDTENTRY)IdtDesc.Base;

    //
    // Code selector (0x8)
    // Flat 4Gb
    //
    pGdt[1].LimitLow                = 0xFFFF;
    pGdt[1].BaseLow                    = 0;
    pGdt[1].HighWord.Bytes.BaseMid    = 0;
    pGdt[1].HighWord.Bytes.Flags1    = 0x9A;
    pGdt[1].HighWord.Bytes.Flags2    = 0xCF;
    pGdt[1].HighWord.Bytes.BaseHi    = 0;

    //
    // Data selector (0x10)
    // Flat 4Gb
    //
    pGdt[2].LimitLow                = 0xFFFF;
    pGdt[2].BaseLow                    = 0;
    pGdt[2].HighWord.Bytes.BaseMid    = 0;
    pGdt[2].HighWord.Bytes.Flags1    = 0x92;
    pGdt[2].HighWord.Bytes.Flags2    = 0xCF;
    pGdt[2].HighWord.Bytes.BaseHi    = 0;

    //
    // Selector (0x18)
    // Flat 2Gb
    //
    pGdt[3].LimitLow                = 0xFFFF;
    pGdt[3].BaseLow                    = 0;
    pGdt[3].HighWord.Bytes.BaseMid    = 0;
    pGdt[3].HighWord.Bytes.Flags1    = 0xFA;
    pGdt[3].HighWord.Bytes.Flags2    = 0xCF;
    pGdt[3].HighWord.Bytes.BaseHi    = 0;

    //
    // Selector (0x20)
    // Flat 2Gb
    //
    pGdt[4].LimitLow                = 0xFFFF;
    pGdt[4].BaseLow                    = 0;
    pGdt[4].HighWord.Bytes.BaseMid    = 0;
    pGdt[4].HighWord.Bytes.Flags1    = 0xF2;
    pGdt[4].HighWord.Bytes.Flags2    = 0xCF;
    pGdt[4].HighWord.Bytes.BaseHi    = 0;

    //
    // TSS Selector (0x28)
    //
    pGdt[5].LimitLow                = 0x78-1; //FIXME: Check this
    pGdt[5].BaseLow = (USHORT)(Tss & 0xffff);
    pGdt[5].HighWord.Bytes.BaseMid = (UCHAR)((Tss >> 16) & 0xff);
    pGdt[5].HighWord.Bytes.Flags1    = 0x89;
    pGdt[5].HighWord.Bytes.Flags2    = 0x00;
    pGdt[5].HighWord.Bytes.BaseHi  = (UCHAR)((Tss >> 24) & 0xff);

    //
    // PCR Selector (0x30)
    //
    pGdt[6].LimitLow                = 0x01;
    pGdt[6].BaseLow  = (USHORT)(Pcr & 0xffff);
    pGdt[6].HighWord.Bytes.BaseMid = (UCHAR)((Pcr >> 16) & 0xff);
    pGdt[6].HighWord.Bytes.Flags1    = 0x92;
    pGdt[6].HighWord.Bytes.Flags2    = 0xC0;
    pGdt[6].HighWord.Bytes.BaseHi  = (UCHAR)((Pcr >> 24) & 0xff);

    //
    // Selector (0x38)
    //
    pGdt[7].LimitLow                = 0xFFFF;
    pGdt[7].BaseLow                    = 0;
    pGdt[7].HighWord.Bytes.BaseMid    = 0;
    pGdt[7].HighWord.Bytes.Flags1    = 0xF3;
    pGdt[7].HighWord.Bytes.Flags2    = 0x40;
    pGdt[7].HighWord.Bytes.BaseHi    = 0;

    //
    // Some BIOS stuff (0x40)
    //
    pGdt[8].LimitLow                = 0xFFFF;
    pGdt[8].BaseLow                    = 0x400;
    pGdt[8].HighWord.Bytes.BaseMid    = 0;
    pGdt[8].HighWord.Bytes.Flags1    = 0xF2;
    pGdt[8].HighWord.Bytes.Flags2    = 0x0;
    pGdt[8].HighWord.Bytes.BaseHi    = 0;

    //
    // Selector (0x48)
    //
    pGdt[9].LimitLow                = 0;
    pGdt[9].BaseLow                    = 0;
    pGdt[9].HighWord.Bytes.BaseMid    = 0;
    pGdt[9].HighWord.Bytes.Flags1    = 0;
    pGdt[9].HighWord.Bytes.Flags2    = 0;
    pGdt[9].HighWord.Bytes.BaseHi    = 0;

    //
    // Selector (0x50)
    //
    pGdt[10].LimitLow                = 0xFFFF; //FIXME: Not correct!
    pGdt[10].BaseLow                = 0;
    pGdt[10].HighWord.Bytes.BaseMid    = 0x2;
    pGdt[10].HighWord.Bytes.Flags1    = 0x89;
    pGdt[10].HighWord.Bytes.Flags2    = 0;
    pGdt[10].HighWord.Bytes.BaseHi    = 0;

    //
    // Selector (0x58)
    //
    pGdt[11].LimitLow                = 0xFFFF;
    pGdt[11].BaseLow                = 0;
    pGdt[11].HighWord.Bytes.BaseMid    = 0x2;
    pGdt[11].HighWord.Bytes.Flags1    = 0x9A;
    pGdt[11].HighWord.Bytes.Flags2    = 0;
    pGdt[11].HighWord.Bytes.BaseHi    = 0;

    //
    // Selector (0x60)
    //
    pGdt[12].LimitLow                = 0xFFFF;
    pGdt[12].BaseLow                = 0; //FIXME: Maybe not correct, but noone cares
    pGdt[12].HighWord.Bytes.BaseMid    = 0x2;
    pGdt[12].HighWord.Bytes.Flags1    = 0x92;
    pGdt[12].HighWord.Bytes.Flags2    = 0;
    pGdt[12].HighWord.Bytes.BaseHi    = 0;

    //
    // Video buffer Selector (0x68)
    //
    pGdt[13].LimitLow                = 0x3FFF;
    pGdt[13].BaseLow                = 0x8000;
    pGdt[13].HighWord.Bytes.BaseMid    = 0x0B;
    pGdt[13].HighWord.Bytes.Flags1    = 0x92;
    pGdt[13].HighWord.Bytes.Flags2    = 0;
    pGdt[13].HighWord.Bytes.BaseHi    = 0;

    //
    // Points to GDT (0x70)
    //
    pGdt[14].LimitLow                = NUM_GDT*sizeof(KGDTENTRY) - 1;
    pGdt[14].BaseLow                = 0x7000;
    pGdt[14].HighWord.Bytes.BaseMid    = 0xFF;
    pGdt[14].HighWord.Bytes.Flags1    = 0x92;
    pGdt[14].HighWord.Bytes.Flags2    = 0;
    pGdt[14].HighWord.Bytes.BaseHi    = 0xFF;

    //
    // Some unused descriptors should go here
    //

    // Copy the old IDT
    RtlCopyMemory(pIdt, (PVOID)OldIdt.Base, OldIdt.Limit + 1);

    // Mask interrupts
    //asm("cli\n"); // they are already masked before enabling paged mode

    // Load GDT+IDT
    Ke386SetGlobalDescriptorTable(&GdtDesc);
    __lidt(&IdtDesc);

    // Jump to proper CS and clear prefetch queue
#if defined(__GNUC__)
    asm("ljmp    $0x08, $1f\n"
        "1:\n");
#elif defined(_MSC_VER)
    /* We can't express the above in MASM so we use this far return instead */
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

    // Set SS selector
    Ke386SetSs(0x10); // DataSelector=0x10

    // Set DS and ES selectors
    Ke386SetDs(0x10);
    Ke386SetEs(0x10); // this is vital for rep stosd

    // LDT = not used ever, thus set to 0
    Ke386SetLocalDescriptorTable(Ldt);

    // Load TSR
    Ke386SetTr(KGDT_TSS);

    // Clear GS
    Ke386SetGs(0);

    // Set FS to PCR
    Ke386SetFs(0x30);

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
MempDump()
{
    ULONG *PDE_Addr=(ULONG *)PDE;//0xC0300000;
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

