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
VIDEODISPLAYMODE Pc98VideoSetDisplayMode(char *DisplayMode, BOOLEAN Init);
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
PCONFIGURATION_COMPONENT_DATA Pc98HwDetect(VOID);
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

    /* BIOS drive number */
    UCHAR DaUa;

    /* IDE driver drive number */
    UCHAR IdeUnitNumber;

    /* Drive type flags */
    UCHAR Type;
#define DRIVE_SASI     0x00
#define DRIVE_IDE      0x01
#define DRIVE_SCSI     0x02
#define DRIVE_CDROM    0x04
#define DRIVE_FDD      0x08
#define DRIVE_MO       0x10
#define DRIVE_RAM      0x20

    /* TRUE when LBA access are supported */
    BOOLEAN LBASupported;

    /*
     * 'IsRemovable' flag: TRUE when the drive is removable (e.g. floppy, CD-ROM...).
     * In that case some of the cached information might need to be refreshed regularly.
     */
    BOOLEAN IsRemovable;

    /*
     * 'Initialized' flag: if TRUE then the drive has been initialized;
     * if FALSE then the disk isn't detected by BIOS/FreeLoader.
     */
    BOOLEAN Initialized;
} PC98_DISK_DRIVE, *PPC98_DISK_DRIVE;

/* Platform-specific boot drive and partition numbers */
extern UCHAR FrldrBootDrive;
extern ULONG FrldrBootPartition;

LONG DiskReportError(BOOLEAN bShowError);
BOOLEAN DiskResetController(IN PPC98_DISK_DRIVE DiskDrive);

BOOLEAN Pc98DiskReadLogicalSectors(UCHAR DriveNumber, ULONGLONG SectorNumber, ULONG SectorCount, PVOID Buffer);
BOOLEAN Pc98DiskGetDriveGeometry(UCHAR DriveNumber, PGEOMETRY DriveGeometry);
ULONG Pc98DiskGetCacheableBlockCount(UCHAR DriveNumber);
UCHAR Pc98GetFloppyCount(VOID);
PPC98_DISK_DRIVE Pc98DiskDriveNumberToDrive(IN UCHAR DriveNumber);

ULONG Pc98GetBootSectorLoadAddress(IN UCHAR DriveNumber);
VOID Pc98DiskPrepareForReactOS(VOID);

/* hwdisk.c */
BOOLEAN PcInitializeBootDevices(VOID);
