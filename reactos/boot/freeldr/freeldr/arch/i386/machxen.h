/*
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

#ifndef __I386_MACHXEN_H_
#define __I386_MACHXEN_H_

#ifndef __MEMORY_H
#include "mm.h"
#endif

#include <rosxen.h>
#include <xen.h>
#include <io/domain_controller.h>

extern BOOL XenActive;
extern ctrl_front_ring_t XenCtrlIfTxRing;
extern ctrl_back_ring_t XenCtrlIfRxRing;
extern int XenCtrlIfEvtchn;

VOID XenMachInit(VOID);

VOID XenConsPutChar(int Ch);
BOOL XenConsKbHit();
int XenConsGetCh();

VOID XenVideoClearScreen(UCHAR Attr);
VIDEODISPLAYMODE XenVideoSetDisplayMode(char *DisplayMode, BOOL Init);
VOID XenVideoGetDisplaySize(PULONG Width, PULONG Height, PULONG Depth);
ULONG XenVideoGetBufferSize(VOID);
VOID XenVideoSetTextCursorPosition(ULONG X, ULONG Y);
VOID XenVideoHideShowTextCursor(BOOL Show);
VOID XenVideoPutChar(int Ch, UCHAR Attr, unsigned X, unsigned Y);
VOID XenVideoCopyOffScreenBufferToVRAM(PVOID Buffer);
BOOL XenVideoIsPaletteFixed(VOID);
VOID XenVideoSetPaletteColor(UCHAR Color, UCHAR Red, UCHAR Green, UCHAR Blue);
VOID XenVideoGetPaletteColor(UCHAR Color, UCHAR* Red, UCHAR* Green, UCHAR* Blue);
VOID XenVideoSync(VOID);
VOID XenVideoPrepareForReactOS(VOID);

ULONG XenMemGetMemoryMap(PBIOS_MEMORY_MAP BiosMemoryMap, ULONG MaxMemoryMapSize);

BOOL XenDiskReadLogicalSectors(ULONG DriveNumber, ULONGLONG SectorNumber, ULONG SectorCount, PVOID Buffer);
BOOL XenDiskGetPartitionEntry(ULONG DriveNumber, ULONG PartitionNumber, PPARTITION_TABLE_ENTRY PartitionTableEntry);
BOOL XenDiskGetDriveGeometry(ULONG DriveNumber, PGEOMETRY DriveGeometry);
ULONG XenDiskGetCacheableBlockCount(ULONG DriveNumber);

VOID XenRTCGetCurrentDateTime(PULONG Year, PULONG Month, PULONG Day, PULONG Hour, PULONG Minute, PULONG Second);

VOID XenHwDetect(VOID);

#endif /* __I386_MACHPC_H_ */

/* EOF */
