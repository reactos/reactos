/*
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
#undef MachDiskGetBootPath
#undef MachDiskReadLogicalSectors
#undef MachDiskGetDriveGeometry
#undef MachDiskGetCacheableBlockCount

MACHVTBL MachVtbl;

VOID
MachConsPutChar(int Ch)
{
    MachVtbl.ConsPutChar(Ch);
}

BOOLEAN
MachConsKbHit(VOID)
{
    return MachVtbl.ConsKbHit();
}

int
MachConsGetCh(VOID)
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
    MachVtbl.VideoGetDisplaySize(Width, Height, Depth);
}

ULONG
MachVideoGetBufferSize(VOID)
{
    return MachVtbl.VideoGetBufferSize();
}

VOID
MachVideoSetTextCursorPosition(UCHAR X, UCHAR Y)
{
    MachVtbl.VideoSetTextCursorPosition(X, Y);
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
    MachVtbl.VideoSetPaletteColor(Color, Red, Green, Blue);
}

VOID
MachVideoGetPaletteColor(UCHAR Color, UCHAR *Red, UCHAR *Green, UCHAR *Blue)
{
    MachVtbl.VideoGetPaletteColor(Color, Red, Green, Blue);
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

BOOLEAN
MachDiskGetBootPath(char *BootPath, unsigned Size)
{
    return MachVtbl.DiskGetBootPath(BootPath, Size);
}

BOOLEAN
MachDiskReadLogicalSectors(UCHAR DriveNumber, ULONGLONG SectorNumber, ULONG SectorCount, PVOID Buffer)
{
    return MachVtbl.DiskReadLogicalSectors(DriveNumber, SectorNumber, SectorCount, Buffer);
}

BOOLEAN
MachDiskGetDriveGeometry(UCHAR DriveNumber, PGEOMETRY DriveGeometry)
{
    return MachVtbl.DiskGetDriveGeometry(DriveNumber, DriveGeometry);
}

ULONG
MachDiskGetCacheableBlockCount(UCHAR DriveNumber)
{
    return MachVtbl.DiskGetCacheableBlockCount(DriveNumber);
}


/* ARC FUNCTIONS **************************************************************/

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

    TimeInfo = ArcGetTime();
    ret = ((TimeInfo->Hour * 24) + TimeInfo->Minute) * 60 + TimeInfo->Second;
    return ret;
}

/* EOF */
