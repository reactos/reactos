/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Header file for NEC PC-98 series
 * COPYRIGHT:   Copyright 2020 Dmitry Borisov (di.sean@protonmail.com)
 */

#pragma once

#ifndef __MEMORY_H
#include "mm.h"
#endif

/*
 * BIOS work area memory
 */

/* Extended RAM between 0x100000 and 0xFFFFFF in 128 kB */
#define MEM_EXPMMSZ 0x401

#define MEM_BIOS_FLAG5 0x458
    #define NESA_BUS_FLAG    0x80

#define MEM_SCSI_TABLE 0x460

/* Bit 3 and bit 6 - keyboard type */
#define MEM_KEYB_TYPE 0x481

/* Status about connected SCSI hard drives */
#define MEM_DISK_EQUIPS 0x482

/* Status about RAM drives */
#define MEM_RDISK_EQUIP 0x488

#define MEM_BIOS_FLAG1 0x501
    #define CONVENTIONAL_MEMORY_SIZE 0x07 /* In 128 kB */
    #define HIGH_RESOLUTION_FLAG     0x08
    #define SYSTEM_CLOCK_8MHZ_FLAG   0x80 /* 0 = PIT runs at 2.4576 MHz, 1 = at 1.9968 MHz */

/* Status about connected floppies */
#define MEM_DISK_EQUIP 0x55C

/* Device Address/Unit Address (DA/UA) */
#define MEM_DISK_BOOT 0x584

/* Extended RAM after 0x1000000, low part, in 1 MB */
#define MEM_EXPMMSZ16M_LOW 0x594

/* Extended RAM after 0x1000000, high part, in 1 MB */
#define MEM_EXPMMSZ16M_HIGH 0x595

/* Status about connected 1.44 MB floppies */
#define MEM_F144_SUPPORT 0x5AE

#define MEM_EXTENDED_NORMAL    0xF8E80
#define MEM_EXTENDED_HIGH_RESO 0xFFE80

VOID Pc98Beep(VOID);

VOID Pc98ConsPutChar(int Ch);
BOOLEAN Pc98ConsKbHit(VOID);
int Pc98ConsGetCh(VOID);

VOID Pc98VideoInit(VOID);
VOID Pc98VideoClearScreen(UCHAR Attr);
VIDEODISPLAYMODE Pc98VideoSetDisplayMode(PCSTR DisplayMode, BOOLEAN Init);
VOID Pc98VideoGetDisplaySize(PULONG Width, PULONG Height, PULONG Depth);
ULONG Pc98VideoGetBufferSize(VOID);
VOID Pc98VideoGetFontsFromFirmware(PULONG RomFontPointers);
VOID Pc98VideoSetTextCursorPosition(UCHAR X, UCHAR Y);
VOID Pc98VideoHideShowTextCursor(BOOLEAN Show);
VOID Pc98VideoPutChar(int Ch, UCHAR Attr, unsigned X, unsigned Y);
VOID Pc98VideoCopyOffScreenBufferToVRAM(PVOID Buffer);
BOOLEAN Pc98VideoIsPaletteFixed(VOID);
VOID Pc98VideoSetPaletteColor(UCHAR Color, UCHAR Red, UCHAR Green, UCHAR Blue);
VOID Pc98VideoGetPaletteColor(UCHAR Color, UCHAR* Red, UCHAR* Green, UCHAR* Blue);
VOID Pc98VideoSync(VOID);
VOID Pc98VideoPrepareForReactOS(VOID);

VOID Pc98PrepareForReactOS(VOID);
TIMEINFO* Pc98GetTime(VOID);
BOOLEAN Pc98InitializeBootDevices(VOID);

PCONFIGURATION_COMPONENT_DATA
Pc98HwDetect(
    _In_opt_ PCSTR Options);

VOID Pc98HwIdle(VOID);

/* pcmem.c */
extern BIOS_MEMORY_MAP PcBiosMemoryMap[];
extern ULONG PcBiosMapCount;

PFREELDR_MEMORY_DESCRIPTOR Pc98MemGetMemoryMap(ULONG *MemoryMapSize);

/* hwpci.c */
BOOLEAN PcFindPciBios(PPCI_REGISTRY_INFO BusData);

/*
 * Disk Variables and Functions
 */

typedef struct _PC98_DISK_DRIVE
{
    /* Disk geometry */
    GEOMETRY Geometry;

    /* PC-98 BIOS drive number */
    UCHAR DaUa;

    /* IDE driver drive number */
    UCHAR AtaUnitNumber;

    /* Drive type */
    UCHAR Type;
#define DRIVE_TYPE_HDD      0
#define DRIVE_TYPE_CDROM    1
#define DRIVE_TYPE_FDD      2

    /* Drive flags */
    UCHAR Flags;
#define DRIVE_FLAGS_IDE             0x01 // IDE drive, accessed by the IDE driver
#define DRIVE_FLAGS_LBA             0x02 // LBA access supported
#define DRIVE_FLAGS_REMOVABLE       0x04 // The drive is removable (e.g. floppy, CD-ROM...)
#define DRIVE_FLAGS_INITIALIZED     0x80 // The drive has been initialized
} PC98_DISK_DRIVE, *PPC98_DISK_DRIVE;

/* Platform-specific boot drive and partition numbers */
extern UCHAR FrldrBootDrive;
extern ULONG FrldrBootPartition;

CONFIGURATION_TYPE
DiskGetConfigType(
    _In_ UCHAR DriveNumber);

LONG
DiskReportError(
    _In_ BOOLEAN bShowError);

BOOLEAN
Pc98DiskReadLogicalSectors(
    _In_ UCHAR DriveNumber,
    _In_ ULONGLONG SectorNumber,
    _In_ ULONG SectorCount,
    _Out_ PVOID Buffer);

BOOLEAN
Pc98DiskGetDriveGeometry(
    _In_ UCHAR DriveNumber,
    _Out_ PGEOMETRY DriveGeometry);

ULONG
Pc98DiskGetCacheableBlockCount(
    _In_ UCHAR DriveNumber);

UCHAR
Pc98GetFloppyCount(VOID);

PPC98_DISK_DRIVE
Pc98DiskDriveNumberToDrive(
    _In_ UCHAR DriveNumber);

ULONG
Pc98GetBootSectorLoadAddress(
    _In_ UCHAR DriveNumber);

/* hwdisk.c */
BOOLEAN PcInitializeBootDevices(VOID);
