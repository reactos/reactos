/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/freeldr/arch/arm/marcharm.c
 * PURPOSE:         Provides abstraction between the ARM Boot Loader and FreeLDR
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <freeldr.h>
#define RGB565(r, g, b) (((r >> 3) << 11)| ((g >> 2) << 5)| ((b >> 3) << 0))

/* GLOBALS ********************************************************************/

UCHAR BootStack[0x4000];
PUCHAR BootStackEnd = &BootStack[0x3FFF];
PARM_BOARD_CONFIGURATION_BLOCK ArmBoardBlock;
ULONG BootDrive, BootPartition;
VOID ArmPrepareForReactOS(IN BOOLEAN Setup);
ADDRESS_RANGE ArmBoardMemoryMap[16];
ULONG ArmBoardMemoryMapRangeCount;

/* FUNCTIONS ******************************************************************/

VOID
ArmInit(IN PARM_BOARD_CONFIGURATION_BLOCK BootContext)
{
    ULONG i;

    //
    // Remember the pointer
    //
    ArmBoardBlock = BootContext;
    
    //
    // Let's make sure we understand the LLB
    //
    ASSERT(ArmBoardBlock->MajorVersion == ARM_BOARD_CONFIGURATION_MAJOR_VERSION);
    ASSERT(ArmBoardBlock->MinorVersion == ARM_BOARD_CONFIGURATION_MINOR_VERSION);
    
    //
    // This should probably go away once we support more boards
    //
    ASSERT((ArmBoardBlock->BoardType == MACH_TYPE_FEROCEON) ||
           (ArmBoardBlock->BoardType == MACH_TYPE_VERSATILE_PB) ||
           (ArmBoardBlock->BoardType == MACH_TYPE_OMAP3_BEAGLE));

    //
    // Save data required for memory initialization
    //
    ArmBoardMemoryMapRangeCount = ArmBoardBlock->MemoryMapEntryCount;
    ASSERT(ArmBoardMemoryMapRangeCount != 0);
    ASSERT(ArmBoardMemoryMapRangeCount < 16);
    for (i = 0; i < ArmBoardMemoryMapRangeCount; i++)
    {
        //
        // Copy each entry
        //
        RtlCopyMemory(&ArmBoardMemoryMap[i],
                      &ArmBoardBlock->MemoryMap[i],
                      sizeof(ADDRESS_RANGE));
    }

    //
    // Call FreeLDR's portable entrypoint with our command-line
    //
    BootMain(ArmBoardBlock->CommandLine);
}

BOOLEAN
ArmDiskGetDriveGeometry(IN ULONG DriveNumber,
                        OUT PGEOMETRY Geometry)
{
    return FALSE;
}

BOOLEAN
ArmDiskReadLogicalSectors(IN ULONG DriveNumber,
                          IN ULONGLONG SectorNumber,
                          IN ULONG SectorCount,
                          IN PVOID Buffer)
{
    return FALSE;
}

ULONG
ArmDiskGetCacheableBlockCount(IN ULONG DriveNumber)
{
    return 0;
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
            break;
            
        //
        // Check for ARM Versatile PB boards
        //
        case MACH_TYPE_VERSATILE_PB:
            
            /* Copy Machine Routines from Firmware Table */
            MachVtbl.ConsPutChar = ArmBoardBlock->ConsPutChar;
            MachVtbl.ConsKbHit = ArmBoardBlock->ConsKbHit;
            MachVtbl.ConsGetCh = ArmBoardBlock->ConsGetCh;
            break;
            
        //
        // Check for TI OMAP3 boards
        // For now that means only Beagle, but ZOOM and others should be ok too
        //
        case MACH_TYPE_OMAP3_BEAGLE:
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
    // Setup disk I/O routines
    //
    MachVtbl.DiskReadLogicalSectors = ArmDiskReadLogicalSectors;
    MachVtbl.DiskGetDriveGeometry = ArmDiskGetDriveGeometry;
    MachVtbl.DiskGetCacheableBlockCount = ArmDiskGetCacheableBlockCount;
    
    //
    // Now set default disk handling routines -- we don't need to override
    //
    MachVtbl.DiskGetBootPath = DiskGetBootPath;
    MachVtbl.DiskNormalizeSystemPath = DiskNormalizeSystemPath;
    
    //
    // We can now print to the console
    //
    TuiPrintf("%s for ARM\n", GetFreeLoaderVersionString());
    TuiPrintf("Bootargs: %s\n\n", CommandLine);
}
