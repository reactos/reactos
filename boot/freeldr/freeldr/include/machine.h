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

#pragma once

#ifndef __DISK_H
#include "disk.h"
#endif

#ifndef __MEMORY_H
#include "mm.h"
#endif

#ifndef __FS_H
#include "fs.h"
#endif

typedef enum tagVIDEODISPLAYMODE
{
    VideoTextMode,
    VideoGraphicsMode
} VIDEODISPLAYMODE, *PVIDEODISPLAYMODE;

typedef struct tagMACHVTBL
{
    VOID (*ConsPutChar)(int Ch);
    BOOLEAN (*ConsKbHit)(VOID);
    int (*ConsGetCh)(VOID);

    VOID (*VideoClearScreen)(UCHAR Attr);
    VIDEODISPLAYMODE (*VideoSetDisplayMode)(char *DisplayMode, BOOLEAN Init);
    VOID (*VideoGetDisplaySize)(PULONG Width, PULONG Height, PULONG Depth);
    ULONG (*VideoGetBufferSize)(VOID);
    VOID (*VideoGetFontsFromFirmware)(PULONG RomFontPointers);
    VOID (*VideoSetTextCursorPosition)(UCHAR X, UCHAR Y);
    VOID (*VideoHideShowTextCursor)(BOOLEAN Show);
    VOID (*VideoPutChar)(int Ch, UCHAR Attr, unsigned X, unsigned Y);
    VOID (*VideoCopyOffScreenBufferToVRAM)(PVOID Buffer);
    BOOLEAN (*VideoIsPaletteFixed)(VOID);
    VOID (*VideoSetPaletteColor)(UCHAR Color, UCHAR Red, UCHAR Green, UCHAR Blue);
    VOID (*VideoGetPaletteColor)(UCHAR Color, UCHAR* Red, UCHAR* Green, UCHAR* Blue);
    VOID (*VideoSync)(VOID);
    VOID (*Beep)(VOID);
    VOID (*PrepareForReactOS)(VOID);

    // NOTE: Not in the machine.c ...
    FREELDR_MEMORY_DESCRIPTOR* (*GetMemoryDescriptor)(FREELDR_MEMORY_DESCRIPTOR* Current);
    PFREELDR_MEMORY_DESCRIPTOR (*GetMemoryMap)(PULONG MaxMemoryMapSize);
    VOID (*GetExtendedBIOSData)(PULONG ExtendedBIOSDataArea, PULONG ExtendedBIOSDataSize);

    UCHAR (*GetFloppyCount)(VOID);
    BOOLEAN (*DiskReadLogicalSectors)(UCHAR DriveNumber, ULONGLONG SectorNumber, ULONG SectorCount, PVOID Buffer);
    BOOLEAN (*DiskGetDriveGeometry)(UCHAR DriveNumber, PGEOMETRY DriveGeometry);
    ULONG (*DiskGetCacheableBlockCount)(UCHAR DriveNumber);

    // NOTE: In the machine.c under the name of "ArcGetXXXTime"
    TIMEINFO* (*GetTime)(VOID);
    ULONG (*GetRelativeTime)(VOID);

    // NOTE: Not in the machine.c ...
    BOOLEAN (*InitializeBootDevices)(VOID);
    PCONFIGURATION_COMPONENT_DATA (*HwDetect)(_In_opt_ PCSTR Options);
    VOID (*HwIdle)(VOID);
} MACHVTBL, *PMACHVTBL;

extern MACHVTBL MachVtbl;

/* NOTE: Implemented by each architecture */
VOID MachInit(const char *CmdLine);

#define MachConsPutChar(Ch) \
    MachVtbl.ConsPutChar(Ch)
#define MachConsKbHit()     \
    MachVtbl.ConsKbHit()
#define MachConsGetCh()     \
    MachVtbl.ConsGetCh()
#define MachVideoClearScreen(Attr)  \
    MachVtbl.VideoClearScreen(Attr)
#define MachVideoSetDisplayMode(Mode, Init) \
    MachVtbl.VideoSetDisplayMode((Mode), (Init))
#define MachVideoGetDisplaySize(W, H, D)    \
    MachVtbl.VideoGetDisplaySize((W), (H), (D))
#define MachVideoGetBufferSize()    \
    MachVtbl.VideoGetBufferSize()
#define MachVideoSetTextCursorPosition(X, Y)    \
    MachVtbl.VideoSetTextCursorPosition((X), (Y))
#define MachVideoHideShowTextCursor(Show)   \
    MachVtbl.VideoHideShowTextCursor(Show)
#define MachVideoPutChar(Ch, Attr, X, Y)    \
    MachVtbl.VideoPutChar((Ch), (Attr), (X), (Y))
#define MachVideoCopyOffScreenBufferToVRAM(Buf) \
    MachVtbl.VideoCopyOffScreenBufferToVRAM(Buf)
#define MachVideoIsPaletteFixed()   \
    MachVtbl.VideoIsPaletteFixed()
#define MachVideoSetPaletteColor(Col, R, G, B)  \
    MachVtbl.VideoSetPaletteColor((Col), (R), (G), (B))
#define MachVideoGetPaletteColor(Col, R, G, B)  \
    MachVtbl.VideoGetPaletteColor((Col), (R), (G), (B))
#define MachVideoSync() \
    MachVtbl.VideoSync()
#define MachBeep()  \
    MachVtbl.Beep()
#define MachGetFloppyCount() \
    MachVtbl.GetFloppyCount()
#define MachDiskReadLogicalSectors(Drive, Start, Count, Buf)    \
    MachVtbl.DiskReadLogicalSectors((Drive), (Start), (Count), (Buf))
#define MachDiskGetDriveGeometry(Drive, Geom)   \
    MachVtbl.DiskGetDriveGeometry((Drive), (Geom))
#define MachDiskGetCacheableBlockCount(Drive)   \
    MachVtbl.DiskGetCacheableBlockCount(Drive)

#define MachInitializeBootDevices() \
    MachVtbl.InitializeBootDevices()

#define MachHwIdle() \
    MachVtbl.HwIdle()


/* ARC FUNCTIONS **************************************************************/

TIMEINFO* ArcGetTime(VOID);
ULONG ArcGetRelativeTime(VOID);

PCONFIGURATION_COMPONENT_DATA MachHwDetect(_In_opt_ PCSTR Options);
VOID MachPrepareForReactOS(VOID);
VOID MachGetExtendedBIOSData(PULONG ExtendedBIOSDataArea, PULONG ExtendedBIOSDataSize);
VOID MachVideoGetFontsFromFirmware(PULONG RomFontPointers);
ULONG MachGetBootSectorLoadAddress(IN UCHAR DriveNumber);

/* EOF */
