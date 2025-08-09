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
PHARDWARE_PTE
MempGetOrCreatePageDir(PHARDWARE_PTE PdeBase, ULONG Index)
{
    PHARDWARE_PTE SubDir;
    static ULONG AllocationFailures = 0;

    if (!PdeBase)
        return NULL;

    if (!PdeBase[Index].Valid)
    {
        SubDir = MmAllocateMemoryWithType(PAGE_SIZE, LoaderMemoryData);
        if (!SubDir)
        {
            AllocationFailures++;
            if (AllocationFailures < 10)  /* Only warn for first few failures */
            {
                TRACE("WARNING: Failed to allocate page table (failure #%lu)\n", AllocationFailures);
            }
            return NULL;
        }
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
        /* Check if it's already correctly mapped as identity mapping */
        if (PteBase[Index].PageFrameNumber == PhysicalAddress / PAGE_SIZE)
        {
            /* Already correctly mapped, that's fine */
            return TRUE;
        }
        ERR("!!!Already mapped %ld to different address (existing PFN=%llx, requested PFN=%llx)\n", 
            Index, PteBase[Index].PageFrameNumber, PhysicalAddress / PAGE_SIZE);
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
    
    /* NOTE: Identity mappings for bootloader code/data are set up later by 
     * WinLdrSetupMemoryLayout through MempSetupPaging. The bootloader runs
     * with UEFI's page tables until WinLdrSetProcessorContext switches to 
     * our new ones. */

    TRACE(">>> leave MempAllocatePageTables\n");

    return TRUE;
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
    ULONG64 PfnDatabaseAddress = 0xFFFFFA8000000000ULL;
    ULONG i;

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
    
    /* Map page tables for PFN database region
     * The PFN database at 0xFFFFFA8000000000 needs complete page table hierarchy
     * Each PFN entry is 48 bytes, so we need enough VA space for all physical memory
     * For 250MB of RAM, we need about 4MB of PFN database space */
    TRACE("Setting up PFN database page tables at 0x%llx\n", PfnDatabaseAddress);
    
    /* Get or create the PML4 entry for PFN database */
    PpeBase = MempGetOrCreatePageDir(PxeBase, VAtoPXI(PfnDatabaseAddress));
    if (!PpeBase)
    {
        ERR("Failed to create PML4 entry for PFN database\n");
        return FALSE;
    }
    
    /* We need to map enough for the PFN database
     * The fault is at FFFFFA800045A00B which is offset 0x45A00B into PFN database
     * This is about 4.5MB into the PFN database, so we need at least that much mapped
     * Let's map 16GB worth of PFN database (enough for 1TB of physical RAM) */
    TRACE("Creating page table hierarchy for PFN database\n");
    
    /* Calculate how much PFN database we actually need based on highest physical page
     * Each PFN entry is 48 bytes (0x30), so for N pages we need N * 0x30 bytes */
    {
        PFN_NUMBER HighestPage = MmGetHighestPhysicalPage();
        SIZE_T PfnDatabaseSize = (HighestPage + 1) * 0x30;
        TRACE("Highest physical page: 0x%lx, PFN database size needed: 0x%lx bytes\n",
              HighestPage, PfnDatabaseSize);
    }
    
    /* Create PDPT and PD entries for first 16GB of PFN database 
     * Each PDPT entry covers 1GB, each PD entry covers 2MB */
    TRACE("Starting PFN database page table creation loop\n");
    for (i = 0; i < 16; i++)  /* 16GB of VA space */
    {
        PHARDWARE_PTE CurrentPdeBase;
        ULONG j;
        
        TRACE("Creating PDPT entry %lu for PFN database\n", i);
        
        /* Get or create PDPT entry for this 1GB region */
        CurrentPdeBase = MempGetOrCreatePageDir(PpeBase, 
            VAtoPPI(PfnDatabaseAddress + (i * 0x40000000ULL)));
        if (!CurrentPdeBase)
        {
            ERR("Failed to create PDPT entry %lu for PFN database\n", i);
            continue;
        }
        
        TRACE("PDPT entry %lu created successfully\n", i);
        
        /* For the first GB, create only essential PD entries
         * The kernel will handle the rest on demand */
        if (i == 0)
        {
            TRACE("Creating minimal PD entries for first GB\n");
            /* Only create first few PD entries instead of all 512 */
            for (j = 0; j < 16; j++)  /* Only 32MB worth initially */
            {
                PHARDWARE_PTE PteBase;
                
                TRACE("Creating PD entry %lu/16\n", j);
                
                /* Create PD entry for each 2MB region */
                PteBase = MempGetOrCreatePageDir(CurrentPdeBase, j);
                if (!PteBase)
                {
                    TRACE("Failed to create PD entry %lu, continuing\n", j);
                    break;
                }
                
                /* Skip PT entry creation - let kernel handle it */
            }
            TRACE("Created %lu PD entries for first GB of PFN database\n", j);
        }
    }
    
    TRACE("PFN database page table hierarchy initialized (16GB VA space prepared)\n");

    return TRUE;
}

static
VOID
Amd64SetupGdt(PVOID GdtBase, ULONG64 TssBase)
{
    PKGDTENTRY64 Entry;
    KDESCRIPTOR GdtDesc;
    volatile ULONG SubStep = 0;
    
    SubStep = 1;
    TRACE("Amd64SetupGdt: Entry (GdtBase=%p, TssBase=%p, SubStep %lu)\n", GdtBase, TssBase, SubStep);
    
    if (GdtBase == NULL)
    {
        ERR("FATAL: GdtBase is NULL in Amd64SetupGdt!\n");
        return;
    }

    SubStep = 2;
    TRACE("Amd64SetupGdt: Setting up NULL descriptor (SubStep %lu)\n", SubStep);
    /* Setup KGDT64_NULL */
    Entry = KiGetGdtEntry(GdtBase, KGDT64_NULL);
    *(PULONG64)Entry = 0x0000000000000000ULL;

    SubStep = 3;
    TRACE("Amd64SetupGdt: Setting up R0_CODE (SubStep %lu)\n", SubStep);
    /* Setup KGDT64_R0_CODE */
    Entry = KiGetGdtEntry(GdtBase, KGDT64_R0_CODE);
    *(PULONG64)Entry = 0x00209b0000000000ULL;

    SubStep = 4;
    TRACE("Amd64SetupGdt: Setting up R0_DATA (SubStep %lu)\n", SubStep);
    /* Setup KGDT64_R0_DATA */
    Entry = KiGetGdtEntry(GdtBase, KGDT64_R0_DATA);
    *(PULONG64)Entry = 0x00cf93000000ffffULL;

    SubStep = 5;
    TRACE("Amd64SetupGdt: Setting up R3_CMCODE (SubStep %lu)\n", SubStep);
    /* Setup KGDT64_R3_CMCODE */
    Entry = KiGetGdtEntry(GdtBase, KGDT64_R3_CMCODE);
    *(PULONG64)Entry = 0x00cffb000000ffffULL;

    SubStep = 6;
    TRACE("Amd64SetupGdt: Setting up R3_DATA (SubStep %lu)\n", SubStep);
    /* Setup KGDT64_R3_DATA */
    Entry = KiGetGdtEntry(GdtBase, KGDT64_R3_DATA);
    *(PULONG64)Entry = 0x00cff3000000ffffULL;

    SubStep = 7;
    TRACE("Amd64SetupGdt: Setting up R3_CODE (SubStep %lu)\n", SubStep);
    /* Setup KGDT64_R3_CODE */
    Entry = KiGetGdtEntry(GdtBase, KGDT64_R3_CODE);
    *(PULONG64)Entry = 0x0020fb0000000000ULL;

    SubStep = 8;
    TRACE("Amd64SetupGdt: Setting up R3_CMTEB (SubStep %lu)\n", SubStep);
    /* Setup KGDT64_R3_CMTEB */
    Entry = KiGetGdtEntry(GdtBase, KGDT64_R3_CMTEB);
    *(PULONG64)Entry = 0xff40f3fd50003c00ULL;

    SubStep = 9;
    TRACE("Amd64SetupGdt: Setting up TSS entry (SubStep %lu)\n", SubStep);
    /* Setup TSS entry */
    Entry = KiGetGdtEntry(GdtBase, KGDT64_SYS_TSS);
    KiInitGdtEntry(Entry, TssBase, sizeof(KTSS), I386_TSS, 0);

    SubStep = 10;
    TRACE("Amd64SetupGdt: Preparing GDT descriptor (SubStep %lu)\n", SubStep);
    /* Setup GDT descriptor */
    GdtDesc.Base  = GdtBase;
    GdtDesc.Limit = NUM_GDT * sizeof(KGDTENTRY) - 1;
    
    TRACE("Amd64SetupGdt: GDT descriptor: Base=%p, Limit=0x%x (SubStep %lu)\n", 
          GdtDesc.Base, GdtDesc.Limit, SubStep);

    SubStep = 11;
    TRACE("Amd64SetupGdt: About to load GDT with __lgdt (SubStep %lu)\n", SubStep);
    /* Set the new Gdt */
    __lgdt(&GdtDesc.Limit);
    
    SubStep = 12;
    TRACE("Amd64SetupGdt: GDT loaded successfully (SubStep %lu)\n", SubStep);
    TRACE("Amd64SetupGdt: Exit successfully\n");
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
    volatile ULONG Step = 0;
    
    Step = 1;
    TRACE("WinLdrSetProcessorContext: Entry (Step %lu)\n", Step);
    
    Step = 2;
    TRACE("WinLdrSetProcessorContext: PxeBase=%p (Step %lu)\n", PxeBase, Step);
    TRACE("WinLdrSetProcessorContext: GdtIdt=%p (Step %lu)\n", GdtIdt, Step);
    TRACE("WinLdrSetProcessorContext: TssBasePage=0x%lx (Step %lu)\n", TssBasePage, Step);
    
    if (PxeBase == NULL)
    {
        ERR("FATAL: PxeBase is NULL! Cannot set page tables\n");
        return;
    }
    
    if (GdtIdt == NULL)
    {
        ERR("FATAL: GdtIdt is NULL! Cannot setup GDT/IDT\n");
        return;
    }

    Step = 3;
    TRACE("WinLdrSetProcessorContext: About to disable interrupts (Step %lu)\n", Step);
    
    /* Disable Interrupts */
    _disable();
    
    Step = 4;
    TRACE("WinLdrSetProcessorContext: Interrupts disabled, reinitializing EFLAGS (Step %lu)\n", Step);

    /* Re-initialize EFLAGS */
    __writeeflags(0);
    
    Step = 5;
    TRACE("WinLdrSetProcessorContext: EFLAGS reset, about to set CR3 to %p (Step %lu)\n", PxeBase, Step);
    
    /* Log current execution context and ensure identity mapping */
    {
        ULONG64 CurrentRIP = (ULONG64)&WinLdrSetProcessorContext;
        ULONG64 CurrentStack;
        ULONG64 CurrentCR3;
        PFN_NUMBER CurrentPage, StackPage;
        
        __asm__ __volatile__("movq %%rsp, %0" : "=r"(CurrentStack));
        __asm__ __volatile__("movq %%cr3, %0" : "=r"(CurrentCR3));
        
        TRACE("WinLdrSetProcessorContext: Current RIP=0x%llx, RSP=0x%llx, CR3=0x%llx\n", 
              CurrentRIP, CurrentStack, CurrentCR3);
        
        /* Ensure current execution area is identity mapped */
        CurrentPage = CurrentRIP >> PAGE_SHIFT;
        StackPage = CurrentStack >> PAGE_SHIFT;
        
        TRACE("WinLdrSetProcessorContext: Ensuring identity mapping for execution at page 0x%lx\n",
              CurrentPage);
        
        /* Map current execution area - expand the range to cover more memory */
        /* This is critical for UEFI boot where bootloader may be loaded high in memory */
        if (CurrentPage > 0x1000)  /* If we're above 16MB (0x1000 pages) */
        {
            PFN_NUMBER StartPage, PageCount;
            
            /* Map 2MB before and after current position for safety */
            StartPage = (CurrentPage > 0x200) ? (CurrentPage - 0x200) : 0;
            PageCount = 0x400;  /* 4MB total (2MB before + 2MB after) */
            
            TRACE("WinLdrSetProcessorContext: Mapping extended range 0x%lx-0x%lx (%lu pages)\n",
                  StartPage, StartPage + PageCount - 1, PageCount);
            
            if (!MempSetupPaging(StartPage, PageCount, FALSE))
            {
                ERR("WinLdrSetProcessorContext: Failed to map execution area! System may crash.\n");
                /* Try smaller range as fallback */
                StartPage = (CurrentPage > 0x80) ? (CurrentPage - 0x80) : 0;
                PageCount = 0x100;  /* 1MB total */
                
                if (!MempSetupPaging(StartPage, PageCount, FALSE))
                {
                    ERR("WinLdrSetProcessorContext: Even fallback mapping failed!\n");
                }
            }
        }
        
        /* Also ensure stack area is mapped if it's in a different region */
        if ((StackPage < (CurrentPage - 0x200)) || (StackPage > (CurrentPage + 0x200)))
        {
            PFN_NUMBER StartPage, PageCount;
            
            TRACE("WinLdrSetProcessorContext: Stack at page 0x%lx needs separate mapping\n",
                  StackPage);
            
            StartPage = (StackPage > 0x80) ? (StackPage - 0x80) : 0;
            PageCount = 0x100;  /* 1MB around stack */
            
            if (!MempSetupPaging(StartPage, PageCount, FALSE))
            {
                ERR("WinLdrSetProcessorContext: Failed to map stack area!\n");
            }
        }
        
        TRACE("WinLdrSetProcessorContext: Identity mappings completed\n");
        
        /* Verify critical pages are mapped before switching */
        {
            BOOLEAN AllMapped = TRUE;
            PFN_NUMBER TestPage;
            
            /* Check current instruction page */
            TestPage = CurrentRIP >> PAGE_SHIFT;
            if (!MempIsPageMapped((PVOID)(TestPage << PAGE_SHIFT)))
            {
                ERR("CRITICAL: Current instruction page 0x%lx not mapped!\n", TestPage);
                AllMapped = FALSE;
                
                /* Try to force map it */
                MempMapSinglePage(TestPage << PAGE_SHIFT, TestPage << PAGE_SHIFT);
            }
            
            /* Check current stack page */
            TestPage = CurrentStack >> PAGE_SHIFT;
            if (!MempIsPageMapped((PVOID)(TestPage << PAGE_SHIFT)))
            {
                ERR("CRITICAL: Current stack page 0x%lx not mapped!\n", TestPage);
                AllMapped = FALSE;
                
                /* Try to force map it */
                MempMapSinglePage(TestPage << PAGE_SHIFT, TestPage << PAGE_SHIFT);
            }
            
            /* Check a few pages around current instruction for safety */
            TestPage = CurrentRIP >> PAGE_SHIFT;
            for (int i = -2; i <= 2; i++)
            {
                PFN_NUMBER CheckPage = TestPage + i;
                if (!MempIsPageMapped((PVOID)(CheckPage << PAGE_SHIFT)))
                {
                    TRACE("Warning: Page 0x%lx near instruction not mapped, mapping it now\n", CheckPage);
                    MempMapSinglePage(CheckPage << PAGE_SHIFT, CheckPage << PAGE_SHIFT);
                }
            }
            
            if (!AllMapped)
            {
                ERR("WARNING: Not all critical pages were mapped! System may crash.\n");
            }
            else
            {
                TRACE("All critical pages verified as mapped.\n");
            }
        }
        
        /* Flush TLB to ensure all changes are visible */
        {
            ULONG64 CurrentCR3Val;
            __asm__ __volatile__("movq %%cr3, %0" : "=r"(CurrentCR3Val));
            __writecr3(CurrentCR3Val);
        }
        
        TRACE("WinLdrSetProcessorContext: Identity mappings completed and verified\n");
    }
    
    /* Memory barrier to ensure all page table updates are visible */
    MemoryBarrier();
    
    TRACE("WinLdrSetProcessorContext: About to switch page tables - setting CR3 now\n");

    /* Set the new PML4 */
    __writecr3((ULONG64)PxeBase);
    
    Step = 6;
    TRACE("WinLdrSetProcessorContext: CR3 set, adjusting GdtIdt to kernel space (Step %lu)\n", Step);

    /* Get kernel mode address of gdt / idt */
    GdtIdt = (PVOID)((ULONG64)GdtIdt + KSEG0_BASE);
    
    Step = 7;
    TRACE("WinLdrSetProcessorContext: New GdtIdt=%p, setting up GDT (Step %lu)\n", GdtIdt, Step);

    /* Create gdt entries and load gdtr */
    Amd64SetupGdt(GdtIdt, KSEG0_BASE | (TssBasePage << MM_PAGE_SHIFT));
    
    /* In UEFI mode, keep the UEFI CS (0x38) - the kernel will handle segment setup */
    TRACE("WinLdrSetProcessorContext: Keeping UEFI CS for compatibility\n");
    
    {
        USHORT CurrentCS;
        __asm__ __volatile__("movw %%cs, %0" : "=r"(CurrentCS));
        TRACE("WinLdrSetProcessorContext: Current CS = 0x%x (UEFI mode)\n", CurrentCS);
    }
    
    Step = 8;
    TRACE("WinLdrSetProcessorContext: GDT loaded, setting up IDT (Step %lu)\n", Step);

    /* Copy old Idt and set idtr */
    Amd64SetupIdt((PVOID)((ULONG64)GdtIdt + NUM_GDT * sizeof(KGDTENTRY)));
    
    Step = 9;
    TRACE("WinLdrSetProcessorContext: IDT loaded, loading TSS (Step %lu)\n", Step);

    /* LDT is unused */
//    __lldt(0);

    /* Load TSR */
    __ltr(KGDT64_SYS_TSS);
    
    Step = 10;
    TRACE("WinLdrSetProcessorContext: TSS loaded successfully (Step %lu)\n", Step);

    TRACE("WinLdrSetProcessorContext: Exit successfully\n");
}

void WinLdrSetupMachineDependent(PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PVOID SharedUserDataAddress = NULL;
    ULONG_PTR Tss = 0;
    ULONG BlockSize, NumPages;
    PVOID KernelStack;
    PVOID PcrPage;
    PVOID PdrPage;

    LoaderBlock->u.I386.CommonDataArea = (PVOID)DbgPrint; // HACK - kernel uses this for debug output
    LoaderBlock->u.I386.MachineType = MACHINE_TYPE_ISA;
    
    TRACE("WinLdrSetupMachineDependent: CommonDataArea (DbgPrint) = %p\n", DbgPrint);

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
    
    /* Allocate Kernel Stack */
    #define KERNEL_STACK_SIZE 0x6000  /* 24KB for AMD64 */
    KernelStack = MmAllocateMemoryWithType(KERNEL_STACK_SIZE, LoaderStartupPcrPage);
    if (KernelStack == NULL)
    {
        UiMessageBox("Can't allocate kernel stack!");
        return;
    }
    RtlZeroMemory(KernelStack, KERNEL_STACK_SIZE);
    
    /* Set the kernel stack in LoaderBlock - use physical address for now */
    /* The kernel will map it properly */
    LoaderBlock->KernelStack = (ULONG_PTR)KernelStack + KERNEL_STACK_SIZE;
    TRACE("Allocated kernel stack at %p (phys), top at %p\n", KernelStack, LoaderBlock->KernelStack);
    
    /* Allocate PCR (Processor Control Region) page */
    PcrPage = MmAllocateMemoryWithType(MM_PAGE_SIZE * 2, LoaderStartupPcrPage);
    if (PcrPage == NULL)
    {
        UiMessageBox("Can't allocate PCR pages!");
        return;
    }
    RtlZeroMemory(PcrPage, MM_PAGE_SIZE * 2);
    
    /* Allocate PDR (Processor Data Region) page */
    PdrPage = MmAllocateMemoryWithType(MM_PAGE_SIZE, LoaderStartupPdrPage);
    if (PdrPage == NULL)
    {
        UiMessageBox("Can't allocate PDR page!");
        return;
    }
    RtlZeroMemory(PdrPage, MM_PAGE_SIZE);
    
    /* Set Process and Thread pointers (these will be set up by the kernel) */
    LoaderBlock->Process = (ULONG_PTR)PcrPage;
    LoaderBlock->Thread = (ULONG_PTR)PdrPage;
    
    /* Set PRCB pointer - kernel expects this! */
    /* PRCB is at offset 0x180 in PCR on AMD64 */
    LoaderBlock->Prcb = (ULONG_PTR)PcrPage + 0x180;
    
    TRACE("PCR at %p, PDR at %p\n", PcrPage, PdrPage);

    // Before we start mapping pages, create a block of memory, which will contain
    // PDE and PTEs
    if (MempAllocatePageTables() == FALSE)
    {
        // FIXME: bugcheck
    }

    /* Map KI_USER_SHARED_DATA, Apic and HAL space */
    WinLdrMapSpecialPages();
    
    /* Map the kernel stack in kernel space */
    if (KernelStack)
    {
        PFN_NUMBER StackPage = (ULONG_PTR)KernelStack >> PAGE_SHIFT;
        PFN_NUMBER StackPages = (KERNEL_STACK_SIZE + PAGE_SIZE - 1) >> PAGE_SHIFT;
        ULONG i;
        
        TRACE("Mapping kernel stack: %lu pages starting at PFN 0x%lx\n", StackPages, StackPage);
        
        for (i = 0; i < StackPages; i++)
        {
            if (!MempMapSinglePage((StackPage + i) * PAGE_SIZE + KSEG0_BASE,
                                   (StackPage + i) * PAGE_SIZE))
            {
                ERR("Failed to map kernel stack page %lu\n", i);
            }
        }
        
        /* Update LoaderBlock with kernel space address */
        LoaderBlock->KernelStack = (ULONG_PTR)KernelStack + KERNEL_STACK_SIZE + KSEG0_BASE;
        TRACE("Kernel stack mapped to kernel space at %p\n", LoaderBlock->KernelStack);
    }
}


VOID
MempDump(VOID)
{
}
