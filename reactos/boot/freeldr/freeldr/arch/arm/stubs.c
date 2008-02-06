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
ArmDiskGetBootVolume(IN PULONG DriveNumber,
                     IN PULONGLONG StartSector,
                     IN PULONGLONG SectorCount, 
                     OUT PINT FsType)
{
    while (TRUE);
    return FALSE;
}

VOID
ArmDiskGetBootDevice(OUT PULONG BootDevice)
{
    while (TRUE);
}

BOOLEAN
ArmDiskBootingFromFloppy(VOID)
{
    while (TRUE);
    return FALSE;
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
ArmDiskGetBootPath(IN PCHAR BootPath,
                   IN unsigned Size)
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

BOOLEAN
ArmDiskGetDriveGeometry(IN ULONG DriveNumber,
                        OUT PGEOMETRY Geometry)
{
    while (TRUE);
    return FALSE;
}

BOOLEAN
ArmDiskGetPartitionEntry(IN ULONG DriveNumber,
                         IN ULONG PartitionNumber,
                         OUT PPARTITION_TABLE_ENTRY PartitionTableEntry)
{
    while (TRUE);
    return FALSE;
}

BOOLEAN
ArmDiskReadLogicalSectors(IN ULONG DriveNumber,
                          IN ULONGLONG SectorNumber,
                          IN ULONG SectorCount,
                          IN PVOID Buffer)
{
    while (TRUE);
    return FALSE;
}

ULONG
ArmDiskGetCacheableBlockCount(IN ULONG DriveNumber)
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
    while (TRUE);
    return FALSE;
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
    MachVtbl.DiskGetBootPath = ArmDiskGetBootPath;
    MachVtbl.DiskGetBootDevice = ArmDiskGetBootDevice;
    MachVtbl.DiskBootingFromFloppy = ArmDiskBootingFromFloppy;
    MachVtbl.DiskNormalizeSystemPath = ArmDiskNormalizeSystemPath;
    MachVtbl.DiskReadLogicalSectors = ArmDiskReadLogicalSectors;
    MachVtbl.DiskGetPartitionEntry = ArmDiskGetPartitionEntry;
    MachVtbl.DiskGetDriveGeometry = ArmDiskGetDriveGeometry;
    MachVtbl.DiskGetCacheableBlockCount = ArmDiskGetCacheableBlockCount;
    MachVtbl.HwDetect = ArmHwDetect;
    
    //
    // We can now print to the console
    //
    TuiPrintf("%s for ARM\n", GetFreeLoaderVersionString());
}
