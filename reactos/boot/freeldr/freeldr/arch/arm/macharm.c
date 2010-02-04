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
ULONG gDiskReadBuffer, gFileSysBuffer;

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
ArmDiskNormalizeSystemPath(IN OUT PCHAR SystemPath,
                           IN unsigned Size)
{
    TuiPrintf("Called: %s\n", SystemPath);
    while (TRUE);
    return TRUE;
}

BOOLEAN
ArmDiskGetBootPath(OUT PCHAR BootPath,
                   IN unsigned Size)
{
    PCCH Path = "ramdisk(0)";
    
    /* Make sure enough space exists */
    if (Size < sizeof(Path)) return FALSE;
    
    /* On ARM platforms, the loader is always in RAM */
    strcpy(BootPath, Path);
    return TRUE;
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
    // Register RAMDISK Device
    //
    RamDiskInitialize();
    
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
            TuiPrintf("Not implemented\n");
            while (TRUE);
            break;
            
        //
        // Check for ARM Versatile PB boards
        //
        case MACH_TYPE_VERSATILE_PB:
            
            /* Copy Machine Routines from Firmware Table */
            MachVtbl.ConsPutChar = ArmBoardBlock->ConsPutChar;
            MachVtbl.ConsKbHit = ArmBoardBlock->ConsKbHit;
            MachVtbl.ConsGetCh = ArmBoardBlock->ConsGetCh;
            MachVtbl.VideoClearScreen = ArmBoardBlock->VideoClearScreen;
            MachVtbl.VideoSetDisplayMode = ArmBoardBlock->VideoSetDisplayMode;
            MachVtbl.VideoGetDisplaySize = ArmBoardBlock->VideoGetDisplaySize;
            MachVtbl.VideoGetBufferSize = ArmBoardBlock->VideoGetBufferSize;
            MachVtbl.VideoSetTextCursorPosition = ArmBoardBlock->VideoSetTextCursorPosition;
            MachVtbl.VideoSetTextCursorPosition = ArmBoardBlock->VideoSetTextCursorPosition;
            MachVtbl.VideoHideShowTextCursor = ArmBoardBlock->VideoHideShowTextCursor;
            MachVtbl.VideoPutChar = ArmBoardBlock->VideoPutChar;
            MachVtbl.VideoCopyOffScreenBufferToVRAM = ArmBoardBlock->VideoCopyOffScreenBufferToVRAM;
            MachVtbl.VideoIsPaletteFixed = ArmBoardBlock->VideoIsPaletteFixed;
            MachVtbl.VideoSetPaletteColor = ArmBoardBlock->VideoSetPaletteColor;
            MachVtbl.VideoGetPaletteColor = ArmBoardBlock->VideoGetPaletteColor;
            MachVtbl.VideoSync = ArmBoardBlock->VideoSync;
                        
            /* Setup the disk and file system buffers */
            gDiskReadBuffer = 0x00090000;
            gFileSysBuffer = 0x00090000;
            break;
            
        //
        // Check for TI OMAP3 boards
        // For now that means only Beagle, but ZOOM and others should be ok too
        //
        case MACH_TYPE_OMAP3_BEAGLE:
            TuiPrintf("Not implemented\n");
            while (TRUE);
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
    MachVtbl.DiskGetBootPath = ArmDiskGetBootPath;
    MachVtbl.DiskNormalizeSystemPath = ArmDiskNormalizeSystemPath;
    
    //
    // We can now print to the console
    //
    TuiPrintf("%s for ARM\n", GetFreeLoaderVersionString());
    TuiPrintf("Bootargs: %s\n\n", CommandLine);
}
