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

#ifndef __MACHINE_H_
#define __MACHINE_H_

#ifndef __DISK_H
#include "disk.h"
#endif

#ifndef __MEMORY_H
#include "mm.h"
#endif

typedef enum tagVIDEODISPLAYMODE
{
  VideoTextMode,
  VideoGraphicsMode
} VIDEODISPLAYMODE, *PVIDEODISPLAYMODE;

typedef struct tagMACHVTBL
{
  VOID (*ConsPutChar)(int Ch);
  BOOL (*ConsKbHit)(VOID);
  int (*ConsGetCh)(VOID);
  
  VOID (*VideoClearScreen)(U8 Attr);
  VIDEODISPLAYMODE (*VideoSetDisplayMode)(char *DisplayMode, BOOL Init);
  VOID (*VideoGetDisplaySize)(PU32 Width, PU32 Height, PU32 Depth);
  U32 (*VideoGetBufferSize)(VOID);
  VOID (*VideoSetTextCursorPosition)(U32 X, U32 Y);
  VOID (*VideoHideShowTextCursor)(BOOL Show);
  VOID (*VideoPutChar)(int Ch, U8 Attr, unsigned X, unsigned Y);
  VOID (*VideoCopyOffScreenBufferToVRAM)(PVOID Buffer);
  BOOL (*VideoIsPaletteFixed)(VOID);
  VOID (*VideoSetPaletteColor)(U8 Color, U8 Red, U8 Green, U8 Blue);
  VOID (*VideoGetPaletteColor)(U8 Color, U8* Red, U8* Green, U8* Blue);
  VOID (*VideoSync)(VOID);
  VOID (*VideoPrepareForReactOS)(VOID);

  U32 (*GetMemoryMap)(PBIOS_MEMORY_MAP BiosMemoryMap, U32 MaxMemoryMapSize);

  BOOL (*DiskReadLogicalSectors)(U32 DriveNumber, U64 SectorNumber, U32 SectorCount, PVOID Buffer);
  BOOL (*DiskGetPartitionEntry)(U32 DriveNumber, U32 PartitionNumber, PPARTITION_TABLE_ENTRY PartitionTableEntry);
  BOOL (*DiskGetDriveGeometry)(U32 DriveNumber, PGEOMETRY DriveGeometry);
  U32 (*DiskGetCacheableBlockCount)(U32 DriveNumber);

  VOID (*RTCGetCurrentDateTime)(PU32 Year, PU32 Month, PU32 Day, PU32 Hour, PU32 Minute, PU32 Second);

  VOID (*HwDetect)(VOID);
} MACHVTBL, *PMACHVTBL;

VOID MachInit(VOID);

extern MACHVTBL MachVtbl;

#define MachConsPutChar(Ch)			MachVtbl.ConsPutChar(Ch)
#define MachConsKbHit()				MachVtbl.ConsKbHit()
#define MachConsGetCh()				MachVtbl.ConsGetCh()
#define MachVideoClearScreen(Attr)		MachVtbl.VideoClearScreen(Attr)
#define MachVideoSetDisplayMode(Mode, Init)	MachVtbl.VideoSetDisplayMode((Mode), (Init))
#define MachVideoGetDisplaySize(W, H, D)	MachVtbl.VideoGetDisplaySize((W), (H), (D))
#define MachVideoGetBufferSize()		MachVtbl.VideoGetBufferSize()
#define MachVideoSetTextCursorPosition(X, Y)	MachVtbl.VideoSetTextCursorPosition((X), (Y))
#define MachVideoHideShowTextCursor(Show)	MachVtbl.VideoHideShowTextCursor(Show)
#define MachVideoPutChar(Ch, Attr, X, Y)	MachVtbl.VideoPutChar((Ch), (Attr), (X), (Y))
#define MachVideoCopyOffScreenBufferToVRAM(Buf)	MachVtbl.VideoCopyOffScreenBufferToVRAM(Buf)
#define MachVideoIsPaletteFixed()		MachVtbl.VideoIsPaletteFixed()
#define MachVideoSetPaletteColor(Col, R, G, B)	MachVtbl.VideoSetPaletteColor((Col), (R), (G), (B))
#define MachVideoGetPaletteColor(Col, R, G, B)	MachVtbl.VideoGetPaletteColor((Col), (R), (G), (B))
#define MachVideoSync()				MachVtbl.VideoSync()
#define MachVideoPrepareForReactOS()		MachVtbl.VideoPrepareForReactOS()
#define MachGetMemoryMap(MMap, Size)		MachVtbl.GetMemoryMap((MMap), (Size))
#define MachDiskReadLogicalSectors(Drive, Start, Count, Buf)	MachVtbl.DiskReadLogicalSectors((Drive), (Start), (Count), (Buf))
#define MachDiskGetPartitionEntry(Drive, Part, Entry)	MachVtbl.DiskGetPartitionEntry((Drive), (Part), (Entry))
#define MachDiskGetDriveGeometry(Drive, Geom)	MachVtbl.DiskGetDriveGeometry((Drive), (Geom))
#define MachDiskGetCacheableBlockCount(Drive)	MachVtbl.DiskGetCacheableBlockCount(Drive)
#define MachRTCGetCurrentDateTime(Y, Mo, D, H, Mi, S)	MachVtbl.RTCGetCurrentDateTime((Y), (Mo), (D), (H), (Mi), (S));
#define MachHwDetect()				MachVtbl.HwDetect()

#endif /* __MACHINE_H_ */

/* EOF */
