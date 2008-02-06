/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            boot/freeldr/arch/arm/stubs.c
 * PURPOSE:         Non-completed ARM hardware-specific routines
 * PROGRAMMERS:     alex@winsiderss.com
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

VOID
ArmConsPutChar(INT Char)
{
    while (TRUE);
}

BOOLEAN
ArmConsKbHit(VOID)
{
    while (TRUE);
    return FALSE;
}

INT
ArmConsGetCh(VOID)
{
    while (TRUE);
    return FALSE;
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
    // Setup ARM routines
    //
    MachVtbl.ConsPutChar = ArmConsPutChar;
    MachVtbl.ConsKbHit = ArmConsKbHit;
    MachVtbl.ConsGetCh = ArmConsGetCh;
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
}
