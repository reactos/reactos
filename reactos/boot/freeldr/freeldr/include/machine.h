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
  BOOLEAN (*ConsKbHit)(VOID);
  int (*ConsGetCh)(VOID);

  VOID (*VideoClearScreen)(UCHAR Attr);
  VIDEODISPLAYMODE (*VideoSetDisplayMode)(char *DisplayMode, BOOLEAN Init);
  VOID (*VideoGetDisplaySize)(PULONG Width, PULONG Height, PULONG Depth);
  ULONG (*VideoGetBufferSize)(VOID);
  VOID (*VideoSetTextCursorPosition)(ULONG X, ULONG Y);
  VOID (*VideoHideShowTextCursor)(BOOLEAN Show);
  VOID (*VideoPutChar)(int Ch, UCHAR Attr, unsigned X, unsigned Y);
  VOID (*VideoCopyOffScreenBufferToVRAM)(PVOID Buffer);
  BOOLEAN (*VideoIsPaletteFixed)(VOID);
  VOID (*VideoSetPaletteColor)(UCHAR Color, UCHAR Red, UCHAR Green, UCHAR Blue);
  VOID (*VideoGetPaletteColor)(UCHAR Color, UCHAR* Red, UCHAR* Green, UCHAR* Blue);
  VOID (*VideoSync)(VOID);
  VOID (*Beep)(VOID);
  VOID (*PrepareForReactOS)(IN BOOLEAN Setup);

  ULONG (*GetMemoryMap)(PBIOS_MEMORY_MAP BiosMemoryMap, ULONG MaxMemoryMapSize);

  BOOLEAN (*DiskGetBootVolume)(PULONG DriveNumber, PULONGLONG StartSector, PULONGLONG SectorCount, int *FsType);
  BOOLEAN (*DiskGetSystemVolume)(char *SystemPath, char *RemainingPath, PULONG Device, PULONG DriveNumber, PULONGLONG StartSector, PULONGLONG SectorCount, int *FsType);
  BOOLEAN (*DiskGetBootPath)(char *BootPath, unsigned Size);
  VOID (*DiskGetBootDevice)(PULONG BootDevice);
  BOOLEAN (*DiskBootingFromFloppy)(VOID);
  BOOLEAN (*DiskNormalizeSystemPath)(char *SystemPath, unsigned Size);
  BOOLEAN (*DiskReadLogicalSectors)(ULONG DriveNumber, ULONGLONG SectorNumber, ULONG SectorCount, PVOID Buffer);
  BOOLEAN (*DiskGetPartitionEntry)(ULONG DriveNumber, ULONG PartitionNumber, PPARTITION_TABLE_ENTRY PartitionTableEntry);
  BOOLEAN (*DiskGetDriveGeometry)(ULONG DriveNumber, PGEOMETRY DriveGeometry);
  ULONG (*DiskGetCacheableBlockCount)(ULONG DriveNumber);

  VOID (*RTCGetCurrentDateTime)(PULONG Year, PULONG Month, PULONG Day, PULONG Hour, PULONG Minute, PULONG Second);

  PCONFIGURATION_COMPONENT_DATA (*HwDetect)(VOID);
} MACHVTBL, *PMACHVTBL;

VOID MachInit(const char *CmdLine);

extern MACHVTBL MachVtbl;

VOID MachConsPutChar(int Ch);
BOOLEAN MachConsKbHit();
int MachConsGetCh();
VOID MachVideoClearScreen(UCHAR Attr);
VIDEODISPLAYMODE MachVideoSetDisplayMode(char *DisplayMode, BOOLEAN Init);
VOID MachVideoGetDisplaySize(PULONG Width, PULONG Height, PULONG Depth);
ULONG MachVideoGetBufferSize(VOID);
VOID MachVideoSetTextCursorPosition(ULONG X, ULONG Y);
VOID MachVideoHideShowTextCursor(BOOLEAN Show);
VOID MachVideoPutChar(int Ch, UCHAR Attr, unsigned X, unsigned Y);
VOID MachVideoCopyOffScreenBufferToVRAM(PVOID Buffer);
BOOLEAN MachVideoIsPaletteFixed(VOID);
VOID MachVideoSetPaletteColor(UCHAR Color, UCHAR Red, UCHAR Green, UCHAR Blue);
VOID MachVideoGetPaletteColor(UCHAR Color, UCHAR *Red, UCHAR *Green, UCHAR *Blue);
VOID MachVideoSync(VOID);
VOID MachBeep(VOID);
ULONG MachGetMemoryMap(PBIOS_MEMORY_MAP BiosMemoryMap, ULONG MaxMemoryMapSize);
BOOLEAN MachDiskGetBootVolume(PULONG DriveNumber, PULONGLONG StartSector, PULONGLONG SectorCount, int *FsType);
BOOLEAN
MachDiskGetSystemVolume(char *SystemPath,
                        char *RemainingPath,
                        PULONG Device,
                        PULONG DriveNumber,
                        PULONGLONG StartSector,
                        PULONGLONG SectorCount,
                        int *FsType);
BOOLEAN MachDiskGetBootPath(char *BootPath, unsigned Size);
VOID MachDiskGetBootDevice(PULONG BootDevice);
BOOLEAN MachDiskBootingFromFloppy();
BOOLEAN MachDiskNormalizeSystemPath(char *SystemPath, unsigned Size);
BOOLEAN MachDiskReadLogicalSectors(ULONG DriveNumber, ULONGLONG SectorNumber, ULONG SectorCount, PVOID Buffer);
BOOLEAN MachDiskGetPartitionEntry(ULONG DriveNumber, ULONG PartitionNumber, PPARTITION_TABLE_ENTRY PartitionTableEntry);
BOOLEAN MachDiskGetDriveGeometry(ULONG DriveNumber, PGEOMETRY DriveGeometry);
ULONG MachDiskGetCacheableBlockCount(ULONG DriveNumber);
VOID MachRTCGetCurrentDateTime(PULONG Year, PULONG Month, PULONG Day, PULONG Hour, PULONG Minute, PULONG Second);
VOID MachHwDetect(VOID);
VOID MachPrepareForReactOS(IN BOOLEAN Setup);

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
#define MachBeep()                   MachVtbl.Beep()
#define MachPrepareForReactOS(a)		MachVtbl.PrepareForReactOS(a)
#define MachGetMemoryMap(MMap, Size)		MachVtbl.GetMemoryMap((MMap), (Size))
#define MachDiskGetBootVolume(Drv, Start, Cnt, FsType)	MachVtbl.DiskGetBootVolume((Drv), (Start), (Cnt), (FsType))
#define MachDiskGetSystemVolume(SysPath, RemPath, Dev, Drv, Start, Cnt, FsType)	MachVtbl.DiskGetSystemVolume((SysPath), (RemPath), (Dev), (Drv), (Start), (Cnt), (FsType))
#define MachDiskGetBootPath(Path, Size)		MachVtbl.DiskGetBootPath((Path), (Size))
#define MachDiskGetBootDevice(BootDevice)	MachVtbl.DiskGetBootDevice(BootDevice)
#define MachDiskBootingFromFloppy()		MachVtbl.DiskBootingFromFloppy()
#define MachDiskNormalizeSystemPath(Path, Size)	MachVtbl.DiskNormalizeSystemPath((Path), (Size))
#define MachDiskReadLogicalSectors(Drive, Start, Count, Buf)	MachVtbl.DiskReadLogicalSectors((Drive), (Start), (Count), (Buf))
#define MachDiskGetPartitionEntry(Drive, Part, Entry)	MachVtbl.DiskGetPartitionEntry((Drive), (Part), (Entry))
#define MachDiskGetDriveGeometry(Drive, Geom)	MachVtbl.DiskGetDriveGeometry((Drive), (Geom))
#define MachDiskGetCacheableBlockCount(Drive)	MachVtbl.DiskGetCacheableBlockCount(Drive)
#define MachRTCGetCurrentDateTime(Y, Mo, D, H, Mi, S)	MachVtbl.RTCGetCurrentDateTime((Y), (Mo), (D), (H), (Mi), (S));
#define MachHwDetect()				MachVtbl.HwDetect()

#endif /* __MACHINE_H_ */

/* EOF */
