/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ACPI interface definitions
 * COPYRIGHT:   Copyright 2024-2025 Dmitry Borisov <di.sean@protonmail.com>
 */

#pragma once

#include <pshpack1.h>

/** IDE channel timing information block */
typedef struct _IDE_ACPI_TIMING_MODE_BLOCK
{
    struct
    {
        ULONG PioSpeed; ///< PIO cycle timing in ns
        ULONG DmaSpeed; ///< DMA cycle timing in ns
/** The mode is not supported */
#define IDE_ACPI_TIMING_MODE_NOT_SUPPORTED              0xFFFFFFFF // aka -1 ns
    } Drive[MAX_IDE_DEVICE];

    ULONG ModeFlags;
/** Use the UDMA mode on drive 0/1 */
#define IDE_ACPI_TIMING_MODE_FLAG_UDMA(Drive)           (0x01 << (2 * (Drive)))

/** Enable the IORDY signal on drive 0/1 */
#define IDE_ACPI_TIMING_MODE_FLAG_IORDY(Drive)          (0x02 << (2 * (Drive)))

/** Independent timing available */
#define IDE_ACPI_TIMING_MODE_FLAG_INDEPENDENT_TIMINGS   0x10
} IDE_ACPI_TIMING_MODE_BLOCK, *PIDE_ACPI_TIMING_MODE_BLOCK;

/** _GTF data buffer */
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
