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

U8 XboxFont8x16[256 * 16];

VOID XboxMachInit(VOID);

VOID XboxConsPutChar(int Ch);
BOOL XboxConsKbHit();
int XboxConsGetCh();

VOID XboxVideoInit(VOID);
VOID XboxVideoClearScreen(U8 Attr);
VIDEODISPLAYMODE XboxVideoSetDisplayMode(char *DisplayModem, BOOL Init);
VOID XboxVideoGetDisplaySize(PU32 Width, PU32 Height, PU32 Depth);
U32 XboxVideoGetBufferSize(VOID);
VOID XboxVideoSetTextCursorPosition(U32 X, U32 Y);
VOID XboxVideoHideShowTextCursor(BOOL Show);
VOID XboxVideoPutChar(int Ch, U8 Attr, unsigned X, unsigned Y);
VOID XboxVideoCopyOffScreenBufferToVRAM(PVOID Buffer);
BOOL XboxVideoIsPaletteFixed(VOID);
VOID XboxVideoSetPaletteColor(U8 Color, U8 Red, U8 Green, U8 Blue);
VOID XboxVideoGetPaletteColor(U8 Color, U8* Red, U8* Green, U8* Blue);
VOID XboxVideoSync(VOID);
VOID XboxVideoPrepareForReactOS(VOID);

VOID XboxMemInit(VOID);
PVOID XboxMemReserveMemory(U32 MbToReserve);
U32 XboxMemGetMemoryMap(PBIOS_MEMORY_MAP BiosMemoryMap, U32 MaxMemoryMapSize);

BOOL XboxDiskReadLogicalSectors(U32 DriveNumber, U64 SectorNumber, U32 SectorCount, PVOID Buffer);
BOOL XboxDiskGetPartitionEntry(U32 DriveNumber, U32 PartitionNumber, PPARTITION_TABLE_ENTRY PartitionTableEntry);
BOOL XboxDiskGetDriveGeometry(U32 DriveNumber, PGEOMETRY DriveGeometry);
U32 XboxDiskGetCacheableBlockCount(U32 DriveNumber);

VOID XboxRTCGetCurrentDateTime(PU32 Year, PU32 Month, PU32 Day, PU32 Hour, PU32 Minute, PU32 Second);

VOID XboxHwDetect(VOID);

#endif /* __I386_HWXBOX_H_ */

/* EOF */
