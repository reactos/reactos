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

#include "freeldr.h"
#include "machine.h"

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
#undef MachVideoPrepareForReactOS
#undef MachGetMemoryMap
#undef MachDiskReadLogicalSectors
#undef MachDiskGetPartitionEntry
#undef MachDiskGetDriveGeometry
#undef MachDiskGetCacheableBlockCount
#undef MachRTCGetCurrentDateTime
#undef MachHwDetect

MACHVTBL MachVtbl;

VOID
MachConsPutChar(int Ch)
{
  MachVtbl.ConsPutChar(Ch);
}

BOOL
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
MachVideoClearScreen(U8 Attr)
{
  MachVtbl.VideoClearScreen(Attr);
}

VIDEODISPLAYMODE
MachVideoSetDisplayMode(char *DisplayMode, BOOL Init)
{
  return MachVtbl.VideoSetDisplayMode(DisplayMode, Init);
}

VOID
MachVideoGetDisplaySize(PU32 Width, PU32 Height, PU32 Depth)
{
  return MachVtbl.VideoGetDisplaySize(Width, Height, Depth);
}

U32
MachVideoGetBufferSize(VOID)
{
  return MachVtbl.VideoGetBufferSize();
}

VOID
MachVideoSetTextCursorPosition(U32 X, U32 Y)
{
  return MachVtbl.VideoSetTextCursorPosition(X, Y);
}

VOID
MachVideoHideShowTextCursor(BOOL Show)
{
  MachVtbl.VideoHideShowTextCursor(Show);
}

VOID
MachVideoPutChar(int Ch, U8 Attr, unsigned X, unsigned Y)
{
  MachVtbl.VideoPutChar(Ch, Attr, X, Y);
}

VOID
MachVideoCopyOffScreenBufferToVRAM(PVOID Buffer)
{
  MachVtbl.VideoCopyOffScreenBufferToVRAM(Buffer);
}

BOOL
MachVideoIsPaletteFixed(VOID)
{
  return MachVtbl.VideoIsPaletteFixed();
}

VOID
MachVideoSetPaletteColor(U8 Color, U8 Red, U8 Green, U8 Blue)
{
  return MachVtbl.VideoSetPaletteColor(Color, Red, Green, Blue);
}

VOID
MachVideoGetPaletteColor(U8 Color, U8 *Red, U8 *Green, U8 *Blue)
{
  return MachVtbl.VideoGetPaletteColor(Color, Red, Green, Blue);
}

VOID
MachVideoSync(VOID)
{
  MachVtbl.VideoSync();
}

VOID
MachVideoPrepareForReactOS(VOID)
{
  MachVtbl.VideoPrepareForReactOS();
}

U32
MachGetMemoryMap(PBIOS_MEMORY_MAP BiosMemoryMap, U32 MaxMemoryMapSize)
{
  return MachVtbl.GetMemoryMap(BiosMemoryMap, MaxMemoryMapSize);
}

BOOL
MachDiskReadLogicalSectors(U32 DriveNumber, U64 SectorNumber, U32 SectorCount, PVOID Buffer)
{
  return MachVtbl.DiskReadLogicalSectors(DriveNumber, SectorNumber, SectorCount, Buffer);
}

BOOL
MachDiskGetPartitionEntry(U32 DriveNumber, U32 PartitionNumber, PPARTITION_TABLE_ENTRY PartitionTableEntry)
{
  return MachVtbl.DiskGetPartitionEntry(DriveNumber, PartitionNumber, PartitionTableEntry);
}

BOOL
MachDiskGetDriveGeometry(U32 DriveNumber, PGEOMETRY DriveGeometry)
{
  return MachVtbl.DiskGetDriveGeometry(DriveNumber, DriveGeometry);
}

U32
MachDiskGetCacheableBlockCount(U32 DriveNumber)
{
  return MachVtbl.DiskGetCacheableBlockCount(DriveNumber);
}

VOID
MachRTCGetCurrentDateTime(PU32 Year, PU32 Month, PU32 Day, PU32 Hour, PU32 Minute, PU32 Second)
{
  MachVtbl.RTCGetCurrentDateTime(Year, Month, Day, Hour, Minute, Second);
}

VOID
MachHwDetect(VOID)
{
  MachVtbl.HwDetect();
}

/* EOF */
