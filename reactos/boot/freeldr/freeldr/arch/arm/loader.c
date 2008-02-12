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
LOADER_PARAMETER_BLOCK ArmLoaderBlock;
LOADER_PARAMETER_EXTENSION ArmExtension;
extern ARM_TRANSLATION_TABLE ArmTranslationTable;
extern ROS_KERNEL_ENTRY_POINT KernelEntryPoint;

/* FUNCTIONS ******************************************************************/

VOID
ArmSetupPageDirectory(VOID)
{   
    ARM_TTB_REGISTER TtbRegister;
    ARM_DOMAIN_REGISTER DomainRegister;
    ARM_PTE Pte;
    ULONG i;
    PARM_TRANSLATION_TABLE TranslationTable;
    
    //
    // Allocate translation table buffer.
    // During bootstrap, this will be a simple L1 (Master) Page Table with
    // Section entries for KSEG0 and the first MB of RAM.
    //
    TranslationTable = &ArmTranslationTable;
    if (!TranslationTable) return;
    
    //
    // Set it as the TTB
    //
    TtbRegister.AsUlong = (ULONG)TranslationTable;
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
    RtlZeroMemory(TranslationTable, 4096 * sizeof(ARM_PTE));
    
    //
    // Build the template PTE
    //
    Pte.L1.Section.Type = SectionPte;
    Pte.L1.Section.Buffered = FALSE;
    Pte.L1.Section.Cached = FALSE;
    Pte.L1.Section.Reserved = 1; // ARM926EJ-S manual recommends setting to 1
    Pte.L1.Section.Domain = Domain0;
    Pte.L1.Section.Access = SupervisorAccess;
    Pte.L1.Section.BaseAddress = 0;
    Pte.L1.Section.Ignored = Pte.L1.Section.Ignored1 = 0;
    
    //
    // Map KSEG0 (0x80000000 - 0xA0000000) to 0x00000000 - 0x80000000
    // In this way, the KERNEL_PHYS_ADDR (0x800000) becomes 0x80800000
    // which is the entrypoint, just like on x86.
    //
    for (i = (KSEG0_BASE >> TTB_SHIFT); i < ((KSEG0_BASE + 0x20000000) >> TTB_SHIFT); i++)
    {
        //
        // Write PTE and update the base address (next MB) for the next one
        //
        TranslationTable->Pte[i] = Pte;
        Pte.L1.Section.BaseAddress++;
    }
    
    //
    // Identity map the first MB of memory as well
    //
    Pte.L1.Section.BaseAddress = 0;
    TranslationTable->Pte[0] = Pte;
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
    // Jump to Kernel
    //
    (*KernelEntryPoint)(Magic, (PVOID)&ArmLoaderBlock);
}

VOID
ArmPrepareForReactOS(IN BOOLEAN Setup)
{   
    //
    // Initialize the loader block
    //
    InitializeListHead(&ArmLoaderBlock.BootDriverListHead);
    InitializeListHead(&ArmLoaderBlock.LoadOrderListHead);
    InitializeListHead(&ArmLoaderBlock.MemoryDescriptorListHead);
    
    //
    // Setup the extension and setup block
    //
    ArmLoaderBlock.Extension = &ArmExtension;
    ArmLoaderBlock.SetupLdrBlock = NULL;
    
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
    // TODO: Setup ARM-specific block
    //
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
