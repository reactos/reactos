/*
 * PROJECT:     FreeLoader
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ATA/ATAPI programmed I/O driver header file.
 * COPYRIGHT:   Copyright 2019-2025 Dmitry Borisov (di.sean@protonmail.com)
 */

#pragma once

/** @brief Data structure for the ATA device. */
typedef struct _DEVICE_UNIT
{
    /** Number of cylinders on the device */
    ULONG Cylinders;

    /** Number of heads on the device */
    ULONG Heads;

    /** Number of sectors per track */
    ULONG SectorsPerTrack;

    /** Number of bytes per sector */
    ULONG SectorSize;

    /** Total number of device sectors/LBA blocks */
    ULONG64 TotalSectors;

    /** Device-specific flags */
    ULONG Flags;
#define ATA_DEVICE_ATAPI                         0x00000001
#define ATA_DEVICE_LBA                           0x00000002
#define ATA_DEVICE_LBA48                         0x00000004
#define ATA_DEVICE_IS_NEC_CDR260                 0x00000008
#define ATA_DEVICE_FLAG_IO32                     0x00000010
} DEVICE_UNIT, *PDEVICE_UNIT;

/* FUNCTIONS ******************************************************************/

BOOLEAN
AtaInit(
    _Out_ PUCHAR DetectedCount);

PDEVICE_UNIT
AtaGetDevice(
    _In_ UCHAR UnitNumber);

BOOLEAN
AtaReadLogicalSectors(
    _In_ PDEVICE_UNIT DeviceUnit,
    _In_ ULONG64 SectorNumber,
    _In_ ULONG SectorCount,
    _Out_writes_bytes_all_(SectorCount * DeviceUnit->SectorSize) PVOID Buffer);
