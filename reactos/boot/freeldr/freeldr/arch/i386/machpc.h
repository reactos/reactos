/* $Id$
 *
 *  FreeLoader
 *
 *  Copyright (C) 2003  Eric Kohl
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

#ifndef __I386_MACHPC_H_
#define __I386_MACHPC_H_

#ifndef __MEMORY_H
#include "mm.h"
#endif

VOID PcMachInit(VOID);

VOID PcConsPutChar(int Ch);
BOOL PcConsKbHit();
int PcConsGetCh();

VOID PcVideoClearScreen(U8 Attr);
VIDEODISPLAYMODE PcVideoSetDisplayMode(char *DisplayMode, BOOL Init);
VOID PcVideoGetDisplaySize(PU32 Width, PU32 Height, PU32 Depth);
U32 PcVideoGetBufferSize(VOID);
VOID PcVideoSetTextCursorPosition(U32 X, U32 Y);
VOID PcVideoHideShowTextCursor(BOOL Show);
VOID PcVideoPutChar(int Ch, U8 Attr, unsigned X, unsigned Y);
VOID PcVideoCopyOffScreenBufferToVRAM(PVOID Buffer);
BOOL PcVideoIsPaletteFixed(VOID);
VOID PcVideoSetPaletteColor(U8 Color, U8 Red, U8 Green, U8 Blue);
VOID PcVideoGetPaletteColor(U8 Color, U8* Red, U8* Green, U8* Blue);
VOID PcVideoSync(VOID);
VOID PcVideoPrepareForReactOS(VOID);

U32 PcMemGetMemoryMap(PBIOS_MEMORY_MAP BiosMemoryMap, U32 MaxMemoryMapSize);

BOOL PcDiskReadLogicalSectors(U32 DriveNumber, U64 SectorNumber, U32 SectorCount, PVOID Buffer);
BOOL PcDiskGetPartitionEntry(U32 DriveNumber, U32 PartitionNumber, PPARTITION_TABLE_ENTRY PartitionTableEntry);
BOOL PcDiskGetDriveGeometry(U32 DriveNumber, PGEOMETRY DriveGeometry);
U32 PcDiskGetCacheableBlockCount(U32 DriveNumber);

VOID PcRTCGetCurrentDateTime(PU32 Year, PU32 Month, PU32 Day, PU32 Hour, PU32 Minute, PU32 Second);

VOID PcHwDetect(VOID);

#endif /* __I386_MACHPC_H_ */

/* EOF */
