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

VOID
PcMachInit(const char *CmdLine)
{
  EnableA20();

  /* Setup vtbl */
  MachVtbl.ConsPutChar = PcConsPutChar;
  MachVtbl.ConsKbHit = PcConsKbHit;
  MachVtbl.ConsGetCh = PcConsGetCh;
  MachVtbl.VideoClearScreen = PcVideoClearScreen;
  MachVtbl.VideoSetDisplayMode = PcVideoSetDisplayMode;
  MachVtbl.VideoGetDisplaySize = PcVideoGetDisplaySize;
  MachVtbl.VideoGetBufferSize = PcVideoGetBufferSize;
  MachVtbl.VideoSetTextCursorPosition = PcVideoSetTextCursorPosition;
  MachVtbl.VideoSetTextCursorPosition = PcVideoSetTextCursorPosition;
  MachVtbl.VideoHideShowTextCursor = PcVideoHideShowTextCursor;
  MachVtbl.VideoPutChar = PcVideoPutChar;
  MachVtbl.VideoCopyOffScreenBufferToVRAM = PcVideoCopyOffScreenBufferToVRAM;
  MachVtbl.VideoIsPaletteFixed = PcVideoIsPaletteFixed;
  MachVtbl.VideoSetPaletteColor = PcVideoSetPaletteColor;
  MachVtbl.VideoGetPaletteColor = PcVideoGetPaletteColor;
  MachVtbl.VideoSync = PcVideoSync;
  MachVtbl.VideoPrepareForReactOS = PcVideoPrepareForReactOS;
  MachVtbl.GetMemoryMap = PcMemGetMemoryMap;
  MachVtbl.DiskGetBootVolume = i386DiskGetBootVolume;
  MachVtbl.DiskGetSystemVolume = i386DiskGetSystemVolume;
  MachVtbl.DiskGetBootPath = i386DiskGetBootPath;
  MachVtbl.DiskGetBootDevice = i386DiskGetBootDevice;
  MachVtbl.DiskBootingFromFloppy = i386DiskBootingFromFloppy;
  MachVtbl.DiskNormalizeSystemPath = i386DiskNormalizeSystemPath;
  MachVtbl.DiskReadLogicalSectors = PcDiskReadLogicalSectors;
  MachVtbl.DiskGetPartitionEntry = PcDiskGetPartitionEntry;
  MachVtbl.DiskGetDriveGeometry = PcDiskGetDriveGeometry;
  MachVtbl.DiskGetCacheableBlockCount = PcDiskGetCacheableBlockCount;
  MachVtbl.RTCGetCurrentDateTime = PcRTCGetCurrentDateTime;
  MachVtbl.HwDetect = PcHwDetect;
}

/* EOF */
