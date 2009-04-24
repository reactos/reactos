/* $Id$
 *
 *  FreeLoader
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <freeldr.h>

#undef MachConsPutChar
#undef MachConsKbHit
#undef MachConsGetCh
#undef MachVideoClearScreen
#undef MachVideoSetDisplayMode
#undef MachVideoGetDisplaySize
#undef MachVideoGetBufferSize
#undef MachVideoSetTextCursorPosition
#undef MachVideoHideShowTextCursor
#undef MachVideoPutChar
#undef MachVideoCopyOffScreenBufferToVRAM
#undef MachVideoIsPaletteFixed
#undef MachVideoSetPaletteColor
#undef MachVideoGetPaletteColor
#undef MachVideoSync
#undef MachBeep
#undef MachPrepareForReactOS
#undef MachGetMemoryMap
#undef MachDiskGetBootVolume
#undef MachDiskGetSystemVolume
#undef MachDiskGetBootPath
#undef MachDiskGetBootDevice
#undef MachDiskBootingFromFloppy
#undef MachDiskNormalizeSystemPath
#undef MachDiskReadLogicalSectors
#undef MachDiskGetPartitionEntry
#undef MachDiskGetDriveGeometry
#undef MachDiskGetCacheableBlockCount
#undef MachHwDetect

MACHVTBL MachVtbl;

VOID
MachConsPutChar(int Ch)
{
  MachVtbl.ConsPutChar(Ch);
}

BOOLEAN
MachConsKbHit()
{
  return MachVtbl.ConsKbHit();
}

int
MachConsGetCh()
{
  return MachVtbl.ConsGetCh();
}

VOID
MachVideoClearScreen(UCHAR Attr)
{
  MachVtbl.VideoClearScreen(Attr);
}

VIDEODISPLAYMODE
MachVideoSetDisplayMode(char *DisplayMode, BOOLEAN Init)
{
  return MachVtbl.VideoSetDisplayMode(DisplayMode, Init);
}

VOID
MachVideoGetDisplaySize(PULONG Width, PULONG Height, PULONG Depth)
{
  return MachVtbl.VideoGetDisplaySize(Width, Height, Depth);
}

ULONG
MachVideoGetBufferSize(VOID)
{
  return MachVtbl.VideoGetBufferSize();
}

VOID
MachVideoSetTextCursorPosition(ULONG X, ULONG Y)
{
  return MachVtbl.VideoSetTextCursorPosition(X, Y);
}

VOID
MachVideoHideShowTextCursor(BOOLEAN Show)
{
  MachVtbl.VideoHideShowTextCursor(Show);
}

VOID
MachVideoPutChar(int Ch, UCHAR Attr, unsigned X, unsigned Y)
{
  MachVtbl.VideoPutChar(Ch, Attr, X, Y);
}

VOID
MachVideoCopyOffScreenBufferToVRAM(PVOID Buffer)
{
  MachVtbl.VideoCopyOffScreenBufferToVRAM(Buffer);
}

BOOLEAN
MachVideoIsPaletteFixed(VOID)
{
  return MachVtbl.VideoIsPaletteFixed();
}

VOID
MachVideoSetPaletteColor(UCHAR Color, UCHAR Red, UCHAR Green, UCHAR Blue)
{
  return MachVtbl.VideoSetPaletteColor(Color, Red, Green, Blue);
}

VOID
MachVideoGetPaletteColor(UCHAR Color, UCHAR *Red, UCHAR *Green, UCHAR *Blue)
{
  return MachVtbl.VideoGetPaletteColor(Color, Red, Green, Blue);
}

VOID
MachVideoSync(VOID)
{
  MachVtbl.VideoSync();
}

VOID
MachBeep(VOID)
{
  MachVtbl.Beep();
}

VOID
MachPrepareForReactOS(IN BOOLEAN Setup)
{
  MachVtbl.PrepareForReactOS(Setup);
}

ULONG
MachGetMemoryMap(PBIOS_MEMORY_MAP BiosMemoryMap, ULONG MaxMemoryMapSize)
{
  return MachVtbl.GetMemoryMap(BiosMemoryMap, MaxMemoryMapSize);
}

BOOLEAN
MachDiskGetBootVolume(PULONG DriveNumber, PULONGLONG StartSector, PULONGLONG SectorCount, int *FsType)
{
  return MachVtbl.DiskGetBootVolume(DriveNumber, StartSector, SectorCount, FsType);
}

BOOLEAN
MachDiskGetSystemVolume(char *SystemPath,
                        char *RemainingPath,
                        PULONG Device,
                        PULONG DriveNumber,
                        PULONGLONG StartSector,
                        PULONGLONG SectorCount,
                        int *FsType)
{
  return MachVtbl.DiskGetSystemVolume(SystemPath, RemainingPath, Device,
                                      DriveNumber, StartSector, SectorCount,
                                      FsType);
}

BOOLEAN
MachDiskGetBootPath(char *BootPath, unsigned Size)
{
  return MachVtbl.DiskGetBootPath(BootPath, Size);
}

VOID
MachDiskGetBootDevice(PULONG BootDevice)
{
  MachVtbl.DiskGetBootDevice(BootDevice);
}

BOOLEAN
MachDiskBootingFromFloppy()
{
  return MachVtbl.DiskBootingFromFloppy();
}

BOOLEAN
MachDiskNormalizeSystemPath(char *SystemPath, unsigned Size)
{
  return MachVtbl.DiskNormalizeSystemPath(SystemPath, Size);
}

BOOLEAN
MachDiskReadLogicalSectors(ULONG DriveNumber, ULONGLONG SectorNumber, ULONG SectorCount, PVOID Buffer)
{
  return MachVtbl.DiskReadLogicalSectors(DriveNumber, SectorNumber, SectorCount, Buffer);
}

BOOLEAN
MachDiskGetPartitionEntry(ULONG DriveNumber, ULONG PartitionNumber, PPARTITION_TABLE_ENTRY PartitionTableEntry)
{
  return MachVtbl.DiskGetPartitionEntry(DriveNumber, PartitionNumber, PartitionTableEntry);
}

BOOLEAN
MachDiskGetDriveGeometry(ULONG DriveNumber, PGEOMETRY DriveGeometry)
{
  return MachVtbl.DiskGetDriveGeometry(DriveNumber, DriveGeometry);
}

ULONG
MachDiskGetCacheableBlockCount(ULONG DriveNumber)
{
  return MachVtbl.DiskGetCacheableBlockCount(DriveNumber);
}

TIMEINFO*
ArcGetTime(VOID)
{
    return MachVtbl.GetTime();
}

ULONG
ArcGetRelativeTime(VOID)
{
    TIMEINFO* TimeInfo;
    ULONG ret;

    if (MachVtbl.GetRelativeTime)
        return MachVtbl.GetRelativeTime();

    TimeInfo = ArcGetTime();
    ret = ((TimeInfo->Hour * 24) + TimeInfo->Minute) * 60 + TimeInfo->Second;
    return ret;
}

VOID
MachHwDetect(VOID)
{
  MachVtbl.HwDetect();
}

/* EOF */
