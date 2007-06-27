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
XboxMachInit(const char *CmdLine)
{
  /* Set LEDs to red before anything is initialized */
  XboxSetLED("rrrr");

  /* Initialize our stuff */
  XboxMemInit();
  XboxVideoInit();

  /* Setup vtbl */
  MachVtbl.ConsPutChar = XboxConsPutChar;
  MachVtbl.ConsKbHit = XboxConsKbHit;
  MachVtbl.ConsGetCh = XboxConsGetCh;
  MachVtbl.VideoClearScreen = XboxVideoClearScreen;
  MachVtbl.VideoSetDisplayMode = XboxVideoSetDisplayMode;
  MachVtbl.VideoGetDisplaySize = XboxVideoGetDisplaySize;
  MachVtbl.VideoGetBufferSize = XboxVideoGetBufferSize;
  MachVtbl.VideoHideShowTextCursor = XboxVideoHideShowTextCursor;
  MachVtbl.VideoPutChar = XboxVideoPutChar;
  MachVtbl.VideoCopyOffScreenBufferToVRAM = XboxVideoCopyOffScreenBufferToVRAM;
  MachVtbl.VideoIsPaletteFixed = XboxVideoIsPaletteFixed;
  MachVtbl.VideoSetPaletteColor = XboxVideoSetPaletteColor;
  MachVtbl.VideoGetPaletteColor = XboxVideoGetPaletteColor;
  MachVtbl.VideoSync = XboxVideoSync;
  MachVtbl.VideoPrepareForReactOS = XboxVideoPrepareForReactOS;
  MachVtbl.GetMemoryMap = XboxMemGetMemoryMap;
  MachVtbl.DiskGetBootVolume = i386DiskGetBootVolume;
  MachVtbl.DiskGetSystemVolume = i386DiskGetSystemVolume;
  MachVtbl.DiskGetBootPath = i386DiskGetBootPath;
  MachVtbl.DiskGetBootDevice = i386DiskGetBootDevice;
  MachVtbl.DiskBootingFromFloppy = i386DiskBootingFromFloppy;
  MachVtbl.DiskNormalizeSystemPath = i386DiskNormalizeSystemPath;
  MachVtbl.DiskReadLogicalSectors = XboxDiskReadLogicalSectors;
  MachVtbl.DiskGetPartitionEntry = XboxDiskGetPartitionEntry;
  MachVtbl.DiskGetDriveGeometry = XboxDiskGetDriveGeometry;
  MachVtbl.DiskGetCacheableBlockCount = XboxDiskGetCacheableBlockCount;
  MachVtbl.RTCGetCurrentDateTime = XboxRTCGetCurrentDateTime;
  MachVtbl.HwDetect = XboxHwDetect;

  /* Set LEDs to orange after init */
  XboxSetLED("oooo");
}
