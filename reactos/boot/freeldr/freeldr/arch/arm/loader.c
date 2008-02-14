/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            boot/freeldr/arch/arm/loader.c
 * PURPOSE:         ARM Kernel Loader
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <freeldr.h>
#include <internal/arm/ke.h>
#include <internal/arm/mm.h>
#include <internal/arm/intrin_i.h>

/* GLOBALS ********************************************************************/

ULONG PageDirectoryStart, PageDirectoryEnd;
PLOADER_PARAMETER_BLOCK ArmLoaderBlock;
CHAR ArmCommandLine[256];
CHAR ArmArcBootPath[64];
CHAR ArmArcHalPath[64];
CHAR ArmNtHalPath[64];
CHAR ArmNtBootPath[64];
PLOADER_PARAMETER_EXTENSION ArmExtension;
extern ARM_TRANSLATION_TABLE ArmTranslationTable;
extern ARM_COARSE_PAGE_TABLE BootTranslationTable, KernelTranslationTable;
extern ROS_KERNEL_ENTRY_POINT KernelEntryPoint;
extern ULONG_PTR KernelBase;

ULONG SizeBits[] =
{
    -1,      // INVALID
    -1,      // INVALID
    1 << 12, // 4KB
    1 << 13, // 8KB
    1 << 14, // 16KB
    1 << 15, // 32KB
    1 << 16, // 64KB
    1 << 17  // 128KB
};

ULONG AssocBits[] =
{
    -1,      // INVALID
    -1,      // INVALID
    4        // 4-way associative
};

ULONG LenBits[] =
{
    -1,      // INVALID
    -1,      // INVALID
    8        // 8 words per line (32 bytes)
};

//
// Where to map the serial port
//
#define UART_VIRTUAL 0xC0000000

/* FUNCTIONS ******************************************************************/

VOID
ArmSetupPageDirectory(VOID)
{   
    ARM_TTB_REGISTER TtbRegister;
    ARM_DOMAIN_REGISTER DomainRegister;
    ARM_PTE Pte;
    ULONG i, j;
    PARM_TRANSLATION_TABLE MasterTable;
    PARM_COARSE_PAGE_TABLE BootTable, KernelTable;
    
    //
    // Get the PDEs that we will use
    //
    MasterTable = &ArmTranslationTable;
    BootTable = &BootTranslationTable;
    KernelTable = &KernelTranslationTable;
    
    //
    // Set the master L1 PDE as the TTB
    //
    TtbRegister.AsUlong = (ULONG)MasterTable;
    ASSERT(TtbRegister.Reserved == 0);
    KeArmTranslationTableRegisterSet(TtbRegister);
    
    //
    // Use Domain 0, enforce AP bits (client)
    //
    DomainRegister.AsUlong = 0;
    DomainRegister.Domain0 = ClientDomain;
    KeArmDomainRegisterSet(DomainRegister);
    
    //
    // Set Fault PTEs everywhere
    //
    RtlZeroMemory(MasterTable, sizeof(ARM_TRANSLATION_TABLE));
    
    //
    // Identity map the first MB of memory
    //
    Pte.L1.Section.Type = SectionPte;
    Pte.L1.Section.Buffered = FALSE;
    Pte.L1.Section.Cached = FALSE;
    Pte.L1.Section.Reserved = 1; // ARM926EJ-S manual recommends setting to 1
    Pte.L1.Section.Domain = Domain0;
    Pte.L1.Section.Access = SupervisorAccess;
    Pte.L1.Section.BaseAddress = 0;
    Pte.L1.Section.Ignored = Pte.L1.Section.Ignored1 = 0;
    MasterTable->Pte[0] = Pte;

    //
    // Map the page in MMIO space that contains the serial port into virtual memory
    //
    Pte.L1.Section.BaseAddress = ArmBoardBlock->UartRegisterBase >> PDE_SHIFT;
    MasterTable->Pte[UART_VIRTUAL >> PDE_SHIFT] = Pte;

    //
    // Create template PTE for the coarse page tables which map the first 8MB
    //
    Pte.L1.Coarse.Type = CoarsePte;
    Pte.L1.Coarse.Domain = Domain0;
    Pte.L1.Coarse.Reserved = 1; // ARM926EJ-S manual recommends setting to 1
    Pte.L1.Coarse.BaseAddress = (ULONG)BootTable >> CPT_SHIFT;
    Pte.L1.Coarse.Ignored = Pte.L1.Coarse.Ignored1 = 0;

    //
    // Map 0x00000000 - 0x007FFFFF to 0x80000000 - 0x807FFFFF.
    // This is where the freeldr boot structures are located, and we need them.
    //
    for (i = (KSEG0_BASE >> PDE_SHIFT); i < ((KSEG0_BASE + 0x800000) >> PDE_SHIFT); i++)
    {
        //
        // Write PTE and update the base address (next MB) for the next one
        //
        MasterTable->Pte[i] = Pte;
        Pte.L1.Coarse.BaseAddress++;
    }
    
    //
    // Now create the template PTE for the coarse page tables for the next 6MB
    //
    Pte.L1.Coarse.BaseAddress = (ULONG)KernelTable >> CPT_SHIFT;
    TuiPrintf("Coarse Table at: %x\n", KernelTable);
    TuiPrintf("Coarse Table at: %x\n", Pte.L1.Coarse.BaseAddress);

    //
    // Map 0x00800000 - 0x00DFFFFF to 0x80800000 - 0x80DFFFFF
    // In this way, the KERNEL_PHYS_ADDR (0x800000) becomes 0x80800000
    // which is the kernel virtual base address, just like on x86.
    //
    ASSERT(KernelBase == 0x80800000);
    for (i = (KernelBase >> PDE_SHIFT); i < ((KernelBase + 0x600000) >> PDE_SHIFT); i++)
    {
        //
        // Write PTE and update the base address (next MB) for the next one
        //
        MasterTable->Pte[i] = Pte;
        Pte.L1.Coarse.BaseAddress++;
    }
    
    //
    // Now build the template PTE for the pages mapping the first 8MB
    //
    Pte.L2.Small.Type = SmallPte;
    Pte.L2.Small.Buffered = Pte.L2.Small.Cached = 0;
    Pte.L2.Small.Access0 =
    Pte.L2.Small.Access1 =
    Pte.L2.Small.Access2 =
    Pte.L2.Small.Access3 = SupervisorAccess;
    Pte.L2.Small.BaseAddress = 0;

    //
    // Loop each boot coarse page table (i).
    // Each PDE describes 1MB. We're mapping an area of 8MB, so 8 times.
    //
    for (i = 0; i < 8; i++)
    {
        //
        // Loop and set each the PTE (j).
        // Each PTE describes 4KB. We're mapping an area of 1MB, so 256 times.
        //
        for (j = 0; j < (PDE_SIZE / PAGE_SIZE); j++)
        {
            //
            // Write PTE and update the base address (next MB) for the next one
            //
            BootTable->Pte[j] = Pte;
            Pte.L2.Small.BaseAddress++;        
        }
        
        //
        // Next iteration
        //
        BootTable++;
    }
    
    //
    // Now create the template PTE for the pages mapping the next 6MB
    //
    Pte.L2.Small.BaseAddress = (ULONG)KERNEL_BASE_PHYS >> PTE_SHIFT;
    TuiPrintf("First Kernel PFN at: %x\n", Pte.L2.Small.BaseAddress);
    
    //
    // Loop each kernel coarse page table (i).
    // Each PDE describes 1MB. We're mapping an area of 6MB, so 6 times.
    //
    for (i = 0; i < 6; i++)
    {
        //
        // Loop and set each the PTE (j).
        // Each PTE describes 4KB. We're mapping an area of 1MB, so 256 times.
        //
        for (j = 0; j < (PDE_SIZE / PAGE_SIZE); j++)
        {
            //
            // Write PTE and update the base address (next MB) for the next one
            //
            KernelTable->Pte[j] = Pte;
            Pte.L2.Small.BaseAddress++;
        }

        //
        // Next iteration
        //
        KernelTable++;
        TuiPrintf("Coarse Table at: %x\n", KernelTable);
    }
}

VOID
ArmSetupPagingAndJump(IN ULONG Magic)
{
    ARM_CONTROL_REGISTER ControlRegister;
    
    //
    // Enable MMU, DCache and ICache
    //
    ControlRegister = KeArmControlRegisterGet();
    ControlRegister.MmuEnabled = TRUE;
    ControlRegister.ICacheEnabled = TRUE;
    ControlRegister.DCacheEnabled = TRUE;
    KeArmControlRegisterSet(ControlRegister);

    //
    // Reconfigure UART0
    //
	ArmBoardBlock->UartRegisterBase = UART_VIRTUAL |
                                      (ArmBoardBlock->UartRegisterBase &
                                       ((1 << PDE_SHIFT) - 1));
    TuiPrintf("Mapped serial port to 0x%x\n", ArmBoardBlock->UartRegisterBase);
	
    //
    // Jump to Kernel
    //
    (*KernelEntryPoint)(Magic, (PVOID)ArmLoaderBlock);
}

VOID
ArmPrepareForReactOS(IN BOOLEAN Setup)
{   
    ARM_CACHE_REGISTER CacheReg;
    PVOID Base;
    PCHAR BootPath, HalPath;
    
    //
    // Allocate the loader block and extension
    //
    ArmLoaderBlock = MmAllocateMemoryWithType(sizeof(LOADER_PARAMETER_BLOCK),
                                              LoaderOsloaderHeap);
    if (!ArmLoaderBlock) return;
    ArmExtension = MmAllocateMemoryWithType(sizeof(LOADER_PARAMETER_EXTENSION),
                                            LoaderOsloaderHeap);
    if (!ArmExtension) return;
    
    //
    // Initialize the loader block
    //
    InitializeListHead(&ArmLoaderBlock->BootDriverListHead);
    InitializeListHead(&ArmLoaderBlock->LoadOrderListHead);
    InitializeListHead(&ArmLoaderBlock->MemoryDescriptorListHead);
    
    //
    // Setup the extension and setup block
    //
    ArmLoaderBlock->Extension = ArmExtension;
    ArmLoaderBlock->SetupLdrBlock = NULL;
    
    //
    // TODO: Setup memory descriptors
    //
    
    //
    // TODO: Setup registry data
    //
    
    //
    // TODO: Setup ARC Hardware tree data
    //
    
    //
    // TODO: Setup NLS data
    //
    
    //
    // TODO: Setup boot-driver data
    //
    
    //
    // TODO: Setup extension parameters
    //
    
    //
    // Make a copy of the command line
    //
    ArmLoaderBlock->LoadOptions = ArmCommandLine;
    strcpy(ArmCommandLine, reactos_kernel_cmdline);
    
    //
    // Find the first \, separating the ARC path from NT path
    //
    BootPath = strchr(ArmCommandLine, '\\');
    *BootPath = ANSI_NULL;
    
    //
    // Set the ARC Boot Path
    //
    strncpy(ArmArcBootPath, ArmCommandLine, 63);
    ArmLoaderBlock->ArcBootDeviceName = ArmArcBootPath;
    
    //
    // The rest of the string is the NT path
    //
    HalPath = strchr(BootPath + 1, ' ');
    *HalPath = ANSI_NULL;
    ArmNtBootPath[0] = '\\';
    strncat(ArmNtBootPath, BootPath + 1, 63);
    strcat(ArmNtBootPath,"\\");
    ArmLoaderBlock->NtBootPathName = ArmNtBootPath;
    
    //
    // Set the HAL paths
    //
    strncpy(ArmArcHalPath, ArmArcBootPath, 63);
    ArmLoaderBlock->ArcHalDeviceName = ArmArcHalPath;
    strcpy(ArmNtHalPath, "\\");
    ArmLoaderBlock->NtHalPathName = ArmNtHalPath;
    
    /* Use this new command line */
    strncpy(ArmLoaderBlock->LoadOptions, HalPath + 2, 255);
    
    /* Parse it and change every slash to a space */
    BootPath = ArmLoaderBlock->LoadOptions;
    do {if (*BootPath == '/') *BootPath = ' ';} while (*BootPath++);

    //
    // Setup cache information
    //
    CacheReg = KeArmCacheRegisterGet();   
    ArmLoaderBlock->u.Arm.FirstLevelDcacheSize = SizeBits[CacheReg.DSize];
    ArmLoaderBlock->u.Arm.FirstLevelDcacheFillSize = LenBits[CacheReg.DLength];
    ArmLoaderBlock->u.Arm.FirstLevelDcacheFillSize <<= 2;
    ArmLoaderBlock->u.Arm.FirstLevelIcacheSize = SizeBits[CacheReg.ISize];
    ArmLoaderBlock->u.Arm.FirstLevelIcacheFillSize = LenBits[CacheReg.ILength];
    ArmLoaderBlock->u.Arm.FirstLevelIcacheFillSize <<= 2;
    ArmLoaderBlock->u.Arm.SecondLevelDcacheSize =
    ArmLoaderBlock->u.Arm.SecondLevelDcacheFillSize =
    ArmLoaderBlock->u.Arm.SecondLevelIcacheSize =
    ArmLoaderBlock->u.Arm.SecondLevelIcacheFillSize = 0;

    //
    // Allocate the Interrupt stack
    //
    Base = MmAllocateMemoryWithType(KERNEL_STACK_SIZE, LoaderStartupDpcStack);
    ArmLoaderBlock->u.Arm.InterruptStack = KSEG0_BASE | (ULONG)Base;
    ArmLoaderBlock->u.Arm.InterruptStack += KERNEL_STACK_SIZE;
    
    //
    // Allocate the Kernel Boot stack
    //
    Base = MmAllocateMemoryWithType(KERNEL_STACK_SIZE, LoaderStartupKernelStack);
    ArmLoaderBlock->KernelStack = KSEG0_BASE | (ULONG)Base;
    ArmLoaderBlock->KernelStack += KERNEL_STACK_SIZE;
    
    //
    // Allocate the Abort stack
    //
    Base = MmAllocateMemoryWithType(KERNEL_STACK_SIZE, LoaderStartupPanicStack);
    ArmLoaderBlock->u.Arm.PanicStack = KSEG0_BASE | (ULONG)Base;
    ArmLoaderBlock->u.Arm.PanicStack += KERNEL_STACK_SIZE;

    //
    // Allocate the PCR page -- align it to 1MB (we only need 4KB)
    //
    Base = MmAllocateMemoryWithType(2 * 1024 * 1024, LoaderStartupPcrPage);
    Base = (PVOID)ROUND_UP(Base, 1 * 1024 * 1024);
    ArmLoaderBlock->u.Arm.PcrPage = (ULONG)Base >> PDE_SHIFT;

    //
    // Allocate PDR pages -- align them to 1MB (we only need 3xKB)
    //
    Base = MmAllocateMemoryWithType(4 * 1024 * 1024, LoaderStartupPdrPage);
    Base = (PVOID)ROUND_UP(Base, 1 * 1024 * 1024);
    ArmLoaderBlock->u.Arm.PdrPage = (ULONG)Base >> PDE_SHIFT;
    
    //
    // Set initial PRCB, Thread and Process on the last PDR page
    //
    Base = (PVOID)((ULONG)Base + 2 * 1024 * 1024);
    ArmLoaderBlock->Prcb = KSEG0_BASE | (ULONG)Base;
    ArmLoaderBlock->Process = ArmLoaderBlock->Prcb + sizeof(KPRCB);
    ArmLoaderBlock->Thread = ArmLoaderBlock->Process + sizeof(EPROCESS);
}

VOID
FrLdrStartup(IN ULONG Magic)
{
    //
    // Disable interrupts (aleady done)
    //

    //
    // Set proper CPSR (already done)
    //

    //
    // Initialize the page directory
    //
    ArmSetupPageDirectory();

    //
    // Initialize paging and load NTOSKRNL
    //
    ArmSetupPagingAndJump(Magic);
}
