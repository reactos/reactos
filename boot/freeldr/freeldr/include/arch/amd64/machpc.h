/* $Id: machpc.h 32135 2008-02-05 11:13:17Z ros-arm-bringup $
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

#define MachInit PcMachInit
VOID PcMachInit(const char *CmdLine);

VOID PcConsPutChar(int Ch);
BOOLEAN PcConsKbHit();
int PcConsGetCh();

VOID PcVideoClearScreen(UCHAR Attr);
VIDEODISPLAYMODE PcVideoSetDisplayMode(char *DisplayMode, BOOLEAN Init);
VOID PcVideoGetDisplaySize(PULONG Width, PULONG Height, PULONG Depth);
ULONG PcVideoGetBufferSize(VOID);
VOID PcVideoSetTextCursorPosition(ULONG X, ULONG Y);
VOID PcVideoHideShowTextCursor(BOOLEAN Show);
VOID PcVideoPutChar(int Ch, UCHAR Attr, unsigned X, unsigned Y);
VOID PcVideoCopyOffScreenBufferToVRAM(PVOID Buffer);
BOOLEAN PcVideoIsPaletteFixed(VOID);
VOID PcVideoSetPaletteColor(UCHAR Color, UCHAR Red, UCHAR Green, UCHAR Blue);
VOID PcVideoGetPaletteColor(UCHAR Color, UCHAR* Red, UCHAR* Green, UCHAR* Blue);
VOID PcVideoSync(VOID);
VOID PcVideoPrepareForReactOS(IN BOOLEAN Setup);
VOID PcPrepareForReactOS(IN BOOLEAN Setup);

ULONG PcMemGetMemoryMap(PBIOS_MEMORY_MAP BiosMemoryMap, ULONG MaxMemoryMapSize);

BOOLEAN PcDiskReadLogicalSectors(ULONG DriveNumber, ULONGLONG SectorNumber, ULONG SectorCount, PVOID Buffer);
BOOLEAN PcDiskGetPartitionEntry(ULONG DriveNumber, ULONG PartitionNumber, PPARTITION_TABLE_ENTRY PartitionTableEntry);
BOOLEAN PcDiskGetDriveGeometry(ULONG DriveNumber, PGEOMETRY DriveGeometry);
ULONG PcDiskGetCacheableBlockCount(ULONG DriveNumber);

VOID PcRTCGetCurrentDateTime(PULONG Year, PULONG Month, PULONG Day, PULONG Hour, PULONG Minute, PULONG Second);

PCONFIGURATION_COMPONENT_DATA PcHwDetect(VOID);

#endif /* __I386_MACHPC_H_ */

/* EOF */
