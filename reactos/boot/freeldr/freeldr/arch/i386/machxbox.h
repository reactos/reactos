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

#ifndef __I386_MACHXBOX_H_
#define __I386_MACHXBOX_H_

#ifndef __MEMORY_H
#include "mm.h"
#endif

UCHAR XboxFont8x16[256 * 16];

VOID XboxMachInit(VOID);

VOID XboxConsPutChar(int Ch);
BOOL XboxConsKbHit();
int XboxConsGetCh();

VOID XboxVideoInit(VOID);
VOID XboxVideoClearScreen(UCHAR Attr);
VIDEODISPLAYMODE XboxVideoSetDisplayMode(char *DisplayModem, BOOL Init);
VOID XboxVideoGetDisplaySize(PULONG Width, PULONG Height, PULONG Depth);
ULONG XboxVideoGetBufferSize(VOID);
VOID XboxVideoSetTextCursorPosition(ULONG X, ULONG Y);
VOID XboxVideoHideShowTextCursor(BOOL Show);
VOID XboxVideoPutChar(int Ch, UCHAR Attr, unsigned X, unsigned Y);
VOID XboxVideoCopyOffScreenBufferToVRAM(PVOID Buffer);
BOOL XboxVideoIsPaletteFixed(VOID);
VOID XboxVideoSetPaletteColor(UCHAR Color, UCHAR Red, UCHAR Green, UCHAR Blue);
VOID XboxVideoGetPaletteColor(UCHAR Color, UCHAR* Red, UCHAR* Green, UCHAR* Blue);
VOID XboxVideoSync(VOID);
VOID XboxVideoPrepareForReactOS(VOID);

VOID XboxMemInit(VOID);
PVOID XboxMemReserveMemory(ULONG MbToReserve);
ULONG XboxMemGetMemoryMap(PBIOS_MEMORY_MAP BiosMemoryMap, ULONG MaxMemoryMapSize);

BOOL XboxDiskReadLogicalSectors(ULONG DriveNumber, ULONGLONG SectorNumber, ULONG SectorCount, PVOID Buffer);
BOOL XboxDiskGetPartitionEntry(ULONG DriveNumber, ULONG PartitionNumber, PPARTITION_TABLE_ENTRY PartitionTableEntry);
BOOL XboxDiskGetDriveGeometry(ULONG DriveNumber, PGEOMETRY DriveGeometry);
ULONG XboxDiskGetCacheableBlockCount(ULONG DriveNumber);

VOID XboxRTCGetCurrentDateTime(PULONG Year, PULONG Month, PULONG Day, PULONG Hour, PULONG Minute, PULONG Second);

VOID XboxHwDetect(VOID);

#endif /* __I386_HWXBOX_H_ */

/* EOF */
