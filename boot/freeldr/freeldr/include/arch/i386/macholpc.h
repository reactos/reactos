/* $Id: machOlpc.h 25800 2007-02-14 20:30:33Z ion $
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

#ifndef __I386_MACHOLPC_H_
#define __I386_MACHOLPC_H_

#ifndef __MEMORY_H
#include "mm.h"
#endif

UCHAR OlpcFont8x16[256 * 16];

VOID OlpcMachInit(const char *CmdLine);

VOID OlpcConsPutChar(int Ch);
BOOLEAN OlpcConsKbHit();
int OlpcConsGetCh();

VOID OlpcVideoInit(VOID);
VOID OlpcVideoClearScreen(UCHAR Attr);
VIDEODISPLAYMODE OlpcVideoSetDisplayMode(char *DisplayModem, BOOLEAN Init);
VOID OlpcVideoGetDisplaySize(PULONG Width, PULONG Height, PULONG Depth);
ULONG OlpcVideoGetBufferSize(VOID);
VOID OlpcVideoSetTextCursorPosition(ULONG X, ULONG Y);
VOID OlpcVideoHideShowTextCursor(BOOLEAN Show);
VOID OlpcVideoPutChar(int Ch, UCHAR Attr, unsigned X, unsigned Y);
VOID OlpcVideoCopyOffScreenBufferToVRAM(PVOID Buffer);
BOOLEAN OlpcVideoIsPaletteFixed(VOID);
VOID OlpcVideoSetPaletteColor(UCHAR Color, UCHAR Red, UCHAR Green, UCHAR Blue);
VOID OlpcVideoGetPaletteColor(UCHAR Color, UCHAR* Red, UCHAR* Green, UCHAR* Blue);
VOID OlpcVideoSync(VOID);
VOID OlpcVideoPrepareForReactOS(IN BOOLEAN Setup);

VOID OlpcMemInit(VOID);
PVOID OlpcMemReserveMemory(ULONG MbToReserve);
ULONG OlpcMemGetMemoryMap(PBIOS_MEMORY_MAP BiosMemoryMap, ULONG MaxMemoryMapSize);

BOOLEAN OlpcDiskReadLogicalSectors(ULONG DriveNumber, ULONGLONG SectorNumber, ULONG SectorCount, PVOID Buffer);
BOOLEAN OlpcDiskGetPartitionEntry(ULONG DriveNumber, ULONG PartitionNumber, PPARTITION_TABLE_ENTRY PartitionTableEntry);
BOOLEAN OlpcDiskGetDriveGeometry(ULONG DriveNumber, PGEOMETRY DriveGeometry);
ULONG OlpcDiskGetCacheableBlockCount(ULONG DriveNumber);

BOOLEAN OlpcDiskGetBootVolume(PULONG DriveNumber, PULONGLONG StartSector, PULONGLONG SectorCount, int *FsType);
BOOLEAN OlpcDiskGetSystemVolume(char *SystemPath,
                             char *RemainingPath,
                             PULONG Device,
                             PULONG DriveNumber, 
                             PULONGLONG StartSector, 
                             PULONGLONG SectorCount, 
                             int *FsType);
BOOLEAN OlpcDiskGetBootPath(char *OutBootPath, unsigned Size);
VOID OlpcDiskGetBootDevice( PULONG BootDevice );
BOOLEAN OlpcDiskNormalizeSystemPath(char *SystemPath, unsigned Size);
BOOLEAN OlpcDiskBootingFromFloppy(VOID);

VOID OlpcRTCGetCurrentDateTime(PULONG Year, PULONG Month, PULONG Day, PULONG Hour, PULONG Minute, PULONG Second);

VOID OlpcHwDetect(VOID);

VOID OlpcSetLED(PCSTR Pattern);


#endif /* __I386_HWOLPC_H_ */

/* EOF */
