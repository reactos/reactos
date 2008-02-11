/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            boot/freeldr/arch/arm/stubs.c
 * PURPOSE:         Non-completed ARM hardware-specific routines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <freeldr.h>

/* GLOBALS ********************************************************************/

typedef union _ARM_PTE
{
    union
    {
        struct
        {
            ULONG Type:2;
            ULONG Unused:30;
        } Fault;
        struct
        {
            ULONG Type:2;
            ULONG Reserved:3;
            ULONG Domain:4;
            ULONG Ignored:1;
            ULONG BaseAddress:22;
        } Coarse;
        struct
        {
            ULONG Type:2;
            ULONG Buffered:1;
            ULONG Cached:1;
            ULONG Reserved:1;
            ULONG Domain:4;
            ULONG Ignored:1;
            ULONG Access:2;
            ULONG Ignored1:8;
            ULONG BaseAddress:12;
        } Section;
        struct
        {
            ULONG Type:2;
            ULONG Reserved:3;
            ULONG Domain:4;
            ULONG Ignored:3;
            ULONG BaseAddress:20;
        } Fine;
    } L1;
    union
    {
        struct
        {
            ULONG Type:2;
            ULONG Unused:30;
        } Fault;
        struct
        {
            ULONG Type:2;
            ULONG Buffered:1;
            ULONG Cached:1;
            ULONG Access0:2;
            ULONG Access1:2;
            ULONG Access2:2;
            ULONG Access3:2;
            ULONG Ignored:4;
            ULONG BaseAddress:16;
        } Large;
        struct
        {
            ULONG Type:2;
            ULONG Buffered:1;
            ULONG Cached:1;
            ULONG Access0:2;
            ULONG Access1:2;
            ULONG Access2:2;
            ULONG Access3:2;
            ULONG BaseAddress:20;
        } Small;
        struct
        {
            ULONG Type:2;
            ULONG Buffered:1;
            ULONG Cached:1;
            ULONG Access0:2;
            ULONG Ignored:4;
            ULONG BaseAddress:22;
        } Tiny; 
    } L2;
    ULONG AsUlong;
} ARM_PTE, *PARM_PTE;

typedef struct _ARM_TRANSLATION_TABLE
{
    ARM_PTE Pte[4096];
} ARM_TRANSLATION_TABLE, *PARM_TRANSLATION_TABLE;

typedef union _ARM_TTB_REGISTER
{
    struct
    {
        ULONG Reserved:14;
        ULONG BaseAddress:18;
    };
    ULONG AsUlong;
} ARM_TTB_REGISTER;

typedef enum _ARM_L1_PTE_TYPE
{
    FaultPte,
    CoarsePte,
    SectionPte,
    FinePte
} ARM_L1_PTE_TYPE;

typedef enum _ARM_PTE_ACCESS
{
    FaultAccess,
    SupervisorAccess,
    SharedAccess,
    UserAccess
} ARM_PTE_ACCESS;

typedef enum _ARM_DOMAIN
{
    FaultDomain,
    ClientDomain,
    InvalidDomain,
    ManagerDomain
} ARM_DOMAIN;

typedef union _ARM_DOMAIN_REGISTER
{
    struct
    {
        ULONG Domain0:2;
        ULONG Domain1:2;
        ULONG Domain2:2;
        ULONG Domain3:2;
        ULONG Domain4:2;
        ULONG Domain5:2;
        ULONG Domain6:2;
        ULONG Domain7:2;
        ULONG Domain8:2;
        ULONG Domain9:2;
        ULONG Domain10:2;
        ULONG Domain11:2;
        ULONG Domain12:2;
        ULONG Domain13:2;
        ULONG Domain14:2;
        ULONG Domain15:2;
    };
    ULONG AsUlong;
} ARM_DOMAIN_REGISTER;

typedef union _ARM_CONTROL_REGISTER
{
    struct
    {
        ULONG MmuEnabled:1;
        ULONG AlignmentFaultsEnabled:1;
        ULONG DCacheEnabled:1;
        ULONG Sbo:3;
        ULONG BigEndianEnabled:1;
        ULONG System:1;
        ULONG Rom:1;
        ULONG Sbz:2;
        ULONG ICacheEnabled:1;
        ULONG HighVectors:1;
        ULONG RoundRobinReplacementEnabled:1;
        ULONG Armv4Compat:1;
        ULONG Sbo1:1;
        ULONG Sbz1:1;
        ULONG Sbo2:1;
        ULONG Reserved:14;
    };
    ULONG AsUlong;
} ARM_CONTROL_REGISTER, *PARM_CONTROL_REGISTER;

typedef enum _ARM_DOMAINS
{
    Domain0
} ARM_DOMAINS;

#define TTB_SHIFT 20

ULONG PageDirectoryStart, PageDirectoryEnd;
LOADER_PARAMETER_BLOCK ArmLoaderBlock;
LOADER_PARAMETER_EXTENSION ArmExtension;
extern ARM_TRANSLATION_TABLE ArmTranslationTable;

/* FUNCTIONS ******************************************************************/

BOOLEAN
ArmDiskGetDriveGeometry(IN ULONG DriveNumber,
                        OUT PGEOMETRY Geometry)
{
    ASSERT(gRamDiskBase == NULL);
    return FALSE;
}

BOOLEAN
ArmDiskReadLogicalSectors(IN ULONG DriveNumber,
                          IN ULONGLONG SectorNumber,
                          IN ULONG SectorCount,
                          IN PVOID Buffer)
{
    ASSERT(gRamDiskBase == NULL);
    return FALSE;
}

ULONG
ArmDiskGetCacheableBlockCount(IN ULONG DriveNumber)
{
    ASSERT(gRamDiskBase == NULL);
    return FALSE;
}

ARM_CONTROL_REGISTER
FORCEINLINE
ArmControlRegisterGet(VOID)
{
    ARM_CONTROL_REGISTER Value;
    __asm__ __volatile__ ("mrc p15, 0, %0, c1, c0, 0" : "=r"(Value.AsUlong) : : "cc");
    return Value;
}

VOID
FORCEINLINE
ArmControlRegisterSet(IN ARM_CONTROL_REGISTER ControlRegister)
{
    __asm__ __volatile__ ("mcr p15, 0, %0, c1, c0, 0; b ." : : "r"(ControlRegister.AsUlong) : "cc");    
}

VOID
FORCEINLINE
ArmMmuTtbSet(IN ARM_TTB_REGISTER Ttb)
{
    __asm__ __volatile__ ("mcr p15, 0, %0, c2, c0, 0" : : "r"(Ttb.AsUlong) : "cc");
}

VOID
FORCEINLINE
ArmMmuDomainRegisterSet(IN ARM_DOMAIN_REGISTER DomainRegister)
{
    __asm__ __volatile__ ("mcr p15, 0, %0, c3, c0, 0" : : "r"(DomainRegister.AsUlong) : "cc");
}

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
    TuiPrintf("CP15 C2: %x\n", TtbRegister);
    ArmMmuTtbSet(TtbRegister);
    
    //
    // Use Domain 0, enforce AP bits (client)
    //
    DomainRegister.AsUlong = 0;
    DomainRegister.Domain0 = ClientDomain;
    TuiPrintf("CP15 C3: %x\n", DomainRegister);
    ArmMmuDomainRegisterSet(DomainRegister);
    
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
    Pte.L1.Section.BaseAddress = KSEG0_BASE >> TTB_SHIFT;
    Pte.L1.Section.Ignored = Pte.L1.Section.Ignored1 = 0;
    TuiPrintf("Template PTE: %x\n", Pte.AsUlong);
    TuiPrintf("Base: %x %x\n", KSEG0_BASE, Pte.L1.Section.BaseAddress);
    
    //
    // Map KSEG0 (0x80000000 - 0xA0000000)
    //
    TuiPrintf("First PTE Index: %x\n", KSEG0_BASE >> TTB_SHIFT);
    TuiPrintf("Last PTE Index: %x\n", ((KSEG0_BASE + 0x20000000) >> TTB_SHIFT));
    for (i = (KSEG0_BASE >> TTB_SHIFT); i < ((KSEG0_BASE + 0x20000000) >> TTB_SHIFT); i++)
    {
        //
        // Update the PTE base address (next MB)
        //
        Pte.L1.Section.BaseAddress++;
        TranslationTable->Pte[i] = Pte;
    }
    
    //
    // Identity map the first MB of memory
    //
    TuiPrintf("Last KSEG0 PTE: %x %x\n", i, Pte.AsUlong);
    Pte.L1.Section.BaseAddress = 0;
    TranslationTable->Pte[0] = Pte;
}

VOID
ArmSetupPagingAndJump(IN ULONG Magic)
{
    ARM_CONTROL_REGISTER ControlRegister;
    
    //
    // This is it! Once we enable the MMU we're in a totally different universe.
    // Cross our fingers: We mapped the bottom 1MB of memory, so FreeLDR and
    // boot-data is still there. We also mapped the kernel, and made our
    // allocations | KSEG0_BASE. If any of this isn't true, we're dead.
    //
    TuiPrintf("Crossing the Rubicon!\n");
    
    //
    // Enable MMU, DCache and ICache
    //
    ControlRegister = ArmControlRegisterGet();
    TuiPrintf("CP15 C1: %x\n", ControlRegister);
    ControlRegister.MmuEnabled = TRUE;
    ControlRegister.ICacheEnabled = TRUE;
    ControlRegister.DCacheEnabled = TRUE;
    TuiPrintf("CP15 C1: %x\n", ControlRegister);
    ArmControlRegisterSet(ControlRegister);
    
    //
    // Are we still alive?
    //
    while (TRUE);
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

PCONFIGURATION_COMPONENT_DATA
ArmHwDetect(VOID)
{
    PCONFIGURATION_COMPONENT_DATA RootNode;
    
    //
    // Create the root node
    //
    FldrCreateSystemKey(&RootNode);
    
    //
    // Write null component information
    //
    FldrSetComponentInformation(RootNode,
                                0x0,
                                0x0,
                                0xFFFFFFFF);
    
    //
    // TODO:
    // There's no such thing as "PnP" on embedded hardware.
    // The boot loader will send us a device tree, similar to ACPI
    // or OpenFirmware device trees, and we will convert it to ARC.
    //

    //
    // Return the root node
    //
    return RootNode;
}

ULONG
ArmMemGetMemoryMap(OUT PBIOS_MEMORY_MAP BiosMemoryMap,
                   IN ULONG MaxMemoryMapSize)
{
    //
    // Return whatever the board returned to us (CS0 Base + Size and FLASH0)
    //
    RtlCopyMemory(BiosMemoryMap,
                  ArmBoardBlock->MemoryMap,
                  ArmBoardBlock->MemoryMapEntryCount * sizeof(BIOS_MEMORY_MAP));
    return ArmBoardBlock->MemoryMapEntryCount;
}

VOID
MachInit(IN PCCH CommandLine)
{
    //
    // Setup board-specific ARM routines
    //
    switch (ArmBoardBlock->BoardType)
    {
        //
        // Check for Feroceon-base boards
        //
        case MACH_TYPE_FEROCEON:
            
            //
            // These boards use a UART16550. Set us up for 115200 bps
            //
            ArmFeroSerialInit(115200);
            MachVtbl.ConsPutChar = ArmFeroPutChar;
            MachVtbl.ConsKbHit = ArmFeroKbHit;
            MachVtbl.ConsGetCh = ArmFeroGetCh;
            break;
            
        //
        // Check for ARM Versatile PB boards
        //
        case MACH_TYPE_VERSATILE_PB:
            
            //
            // These boards use a PrimeCell UART (PL011)
            //
            ArmVersaSerialInit(115200);
            MachVtbl.ConsPutChar = ArmVersaPutChar;
            MachVtbl.ConsKbHit = ArmVersaKbHit;
            MachVtbl.ConsGetCh = ArmVersaGetCh;
            break;
            
        default:
            ASSERT(FALSE);
    }
    
    //
    // Setup generic ARM routines for all boards
    //
    MachVtbl.PrepareForReactOS = ArmPrepareForReactOS;
    MachVtbl.GetMemoryMap = ArmMemGetMemoryMap;
    MachVtbl.HwDetect = ArmHwDetect;

    //
    // Setup disk I/O routines, switch to ramdisk ones for non-NAND boot
    //
    MachVtbl.DiskReadLogicalSectors = ArmDiskReadLogicalSectors;
    MachVtbl.DiskGetDriveGeometry = ArmDiskGetDriveGeometry;
    MachVtbl.DiskGetCacheableBlockCount = ArmDiskGetCacheableBlockCount;
    RamDiskSwitchFromBios();

    //
    // Now set default disk handling routines -- we don't need to override
    //
    MachVtbl.DiskGetBootVolume = DiskGetBootVolume;
    MachVtbl.DiskGetSystemVolume = DiskGetSystemVolume;
    MachVtbl.DiskGetBootPath = DiskGetBootPath;
    MachVtbl.DiskGetBootDevice = DiskGetBootDevice;
    MachVtbl.DiskBootingFromFloppy = DiskBootingFromFloppy;
    MachVtbl.DiskNormalizeSystemPath = DiskNormalizeSystemPath;
    MachVtbl.DiskGetPartitionEntry = DiskGetPartitionEntry;

    //
    // We can now print to the console
    //
    TuiPrintf("%s for ARM\n", GetFreeLoaderVersionString());
    TuiPrintf("Bootargs: %s\n", CommandLine);
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
