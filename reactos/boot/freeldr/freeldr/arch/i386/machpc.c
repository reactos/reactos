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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
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
    MachVtbl.Beep = PcBeep;
    MachVtbl.PrepareForReactOS = PcPrepareForReactOS;
    MachVtbl.GetMemoryMap = PcMemGetMemoryMap;
    MachVtbl.DiskGetBootPath = DiskGetBootPath;
    MachVtbl.DiskReadLogicalSectors = PcDiskReadLogicalSectors;
    MachVtbl.DiskGetDriveGeometry = PcDiskGetDriveGeometry;
    MachVtbl.DiskGetCacheableBlockCount = PcDiskGetCacheableBlockCount;
    MachVtbl.GetTime = PcGetTime;
    MachVtbl.HwDetect = PcHwDetect;
}

VOID
PcPrepareForReactOS(IN BOOLEAN Setup)
{
    //
    // On PC, prepare video and turn off the floppy motor
    //
    PcVideoPrepareForReactOS(Setup);
    DiskStopFloppyMotor();
}
/* EOF */
