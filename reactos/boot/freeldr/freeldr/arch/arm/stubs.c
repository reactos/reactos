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

ULONG PageDirectoryStart, PageDirectoryEnd;

/* FUNCTIONS ******************************************************************/

VOID
FrLdrStartup(IN ULONG Magic)
{
    //
    // Start the OS
    //
}


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

BOOLEAN
ArmDiskGetBootVolume(IN PULONG DriveNumber,
                     IN PULONGLONG StartSector,
                     IN PULONGLONG SectorCount, 
                     OUT PINT FsType)
{
    //
    // We only support RAM disk for now -- add support for NAND later
    //
    ASSERT(gRamDiskBase);
    ASSERT(gRamDiskSize);

    //
    // Use magic ramdisk drive number and count the number of 512-byte sectors
    //
    *DriveNumber = 0x49;
    *StartSector = 63;
    *SectorCount = gRamDiskSize * 512;

    //
    // Ramdisk support is FAT-only for now
    //
    *FsType = FS_FAT;

    //
    // Now that ramdisk is enabled, use ramdisk routines
    //
    RamDiskSwitchFromBios();
    return TRUE;
}

BOOLEAN
ArmDiskGetSystemVolume(IN PCHAR SystemPath,
                       OUT PCHAR RemainingPath,
                       OUT PULONG Device,
                       OUT PULONG DriveNumber,
                       OUT PULONGLONG StartSector,
                       OUT PULONGLONG SectorCount,
                       OUT PINT FsType)
{
    while (TRUE);
    return FALSE;
}

BOOLEAN
ArmDiskNormalizeSystemPath(IN PCHAR SystemPath,
                           IN unsigned Size)
{
    while (TRUE);
    return FALSE;
}

VOID
ArmPrepareForReactOS(IN BOOLEAN Setup)
{
    while (TRUE);
}

PCONFIGURATION_COMPONENT_DATA
ArmHwDetect(VOID)
{
    while (TRUE);
    return NULL;
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
        case ARM_FEROCEON:
            
            //
            // These boards use a UART16550. Set us up for 115200 bps
            //
            ArmFeroSerialInit(115200);
            MachVtbl.ConsPutChar = ArmFeroPutChar;
            MachVtbl.ConsKbHit = ArmFeroKbHit;
            MachVtbl.ConsGetCh = ArmFeroGetCh;
            break;
            
        default:
            ASSERT(FALSE);
    }
    
    //
    // Setup generic ARM routines
    //
    MachVtbl.PrepareForReactOS = ArmPrepareForReactOS;
    MachVtbl.GetMemoryMap = ArmMemGetMemoryMap;
    MachVtbl.DiskGetBootVolume = ArmDiskGetBootVolume;
    MachVtbl.DiskGetSystemVolume = ArmDiskGetSystemVolume;
    MachVtbl.DiskNormalizeSystemPath = ArmDiskNormalizeSystemPath;
    MachVtbl.DiskReadLogicalSectors = ArmDiskReadLogicalSectors;
    MachVtbl.DiskGetDriveGeometry = ArmDiskGetDriveGeometry;
    MachVtbl.DiskGetCacheableBlockCount = ArmDiskGetCacheableBlockCount;
    MachVtbl.HwDetect = ArmHwDetect;
    
    //
    // We can now print to the console
    //
    TuiPrintf("%s for ARM\n", GetFreeLoaderVersionString());
    TuiPrintf("Bootargs: %s\n", CommandLine);
}
