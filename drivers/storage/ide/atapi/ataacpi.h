/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ACPI interface definitions
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov <di.sean@protonmail.com>
 */

#include <pshpack1.h>

typedef struct _IDE_ACPI_TIMING_MODE_BLOCK
{
    struct
    {
        ULONG PioSpeed;
        ULONG DmaSpeed;
    } Drive[MAX_IDE_DEVICE];

    ULONG ModeFlags;
#define IDE_ACPI_TIMING_MODE_FLAG_UDMA                  0x01
#define IDE_ACPI_TIMING_MODE_FLAG_IORDY                 0x02
#define IDE_ACPI_TIMING_MODE_FLAG_UDMA2                 0x04
#define IDE_ACPI_TIMING_MODE_FLAG_IORDY2                0x08
#define IDE_ACPI_TIMING_MODE_FLAG_INDEPENDENT_TIMINGS   0x10
} IDE_ACPI_TIMING_MODE_BLOCK, *PIDE_ACPI_TIMING_MODE_BLOCK;

#define IDE_ACPI_TIMING_MODE_NOT_SUPPORTED              0xFFFFFFFF

typedef struct _ATA_ACPI_TASK_FILE
{
    UCHAR Feature;
    UCHAR SectorCount;
    UCHAR LowLba;
    UCHAR MidLba;
    UCHAR HighLba;
    UCHAR DriveSelect;
    UCHAR Command;
} ATA_ACPI_TASK_FILE, *PATA_ACPI_TASK_FILE;

#include <poppack.h>
