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

#ifndef __MEMORY_H
#include "mm.h"
#endif

#include <drivers/xbox/xgpu.h>

extern UCHAR BitmapFont8x16[256 * 16];

VOID XboxConsPutChar(int Ch);
BOOLEAN XboxConsKbHit(VOID);
int XboxConsGetCh(VOID);

VOID XboxVideoInit(VOID);
VOID XboxVideoClearScreen(UCHAR Attr);
VIDEODISPLAYMODE XboxVideoSetDisplayMode(char *DisplayModem, BOOLEAN Init);
VOID XboxVideoGetDisplaySize(PULONG Width, PULONG Height, PULONG Depth);
ULONG XboxVideoGetBufferSize(VOID);
VOID XboxVideoGetFontsFromFirmware(PULONG RomFontPointers);
VOID XboxVideoSetTextCursorPosition(UCHAR X, UCHAR Y);
VOID XboxVideoHideShowTextCursor(BOOLEAN Show);
VOID XboxVideoPutChar(int Ch, UCHAR Attr, unsigned X, unsigned Y);
VOID XboxVideoCopyOffScreenBufferToVRAM(PVOID Buffer);
BOOLEAN XboxVideoIsPaletteFixed(VOID);
VOID XboxVideoSetPaletteColor(UCHAR Color, UCHAR Red, UCHAR Green, UCHAR Blue);
VOID XboxVideoGetPaletteColor(UCHAR Color, UCHAR* Red, UCHAR* Green, UCHAR* Blue);
VOID XboxVideoSync(VOID);
VOID XboxVideoPrepareForReactOS(VOID);
VOID XboxVideoScrollUp(VOID);
VOID XboxPrepareForReactOS(VOID);

VOID XboxMemInit(VOID);
PFREELDR_MEMORY_DESCRIPTOR XboxMemGetMemoryMap(ULONG *MemoryMapSize);

VOID XboxDiskInit(BOOLEAN Init);
BOOLEAN XboxDiskReadLogicalSectors(UCHAR DriveNumber, ULONGLONG SectorNumber, ULONG SectorCount, PVOID Buffer);
BOOLEAN XboxDiskGetDriveGeometry(UCHAR DriveNumber, PGEOMETRY DriveGeometry);
ULONG XboxDiskGetCacheableBlockCount(UCHAR DriveNumber);

TIMEINFO* XboxGetTime(VOID);

PCONFIGURATION_COMPONENT_DATA XboxHwDetect(VOID);
VOID XboxHwIdle(VOID);

VOID XboxSetLED(PCSTR Pattern);

/* EOF */
