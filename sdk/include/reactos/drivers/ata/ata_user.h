/*
 * PROJECT:     ReactOS Storage Stack
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 *              or MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ATA driver user mode interface
 * COPYRIGHT:   Copyright 2026 Dmitry Borisov (di.sean@protonmail.com)
 */

#pragma once

#define DD_ATA_REG_ATA_DEVICE_TYPE               L"DeviceType"
#define DD_ATA_REG_SCSI_DEVICE_TYPE              L"ScsiDeviceType"
#define DD_ATA_REG_MAX_TARGET_ID                 L"MaxTargetId"
#define DD_ATA_REG_XFER_MODE_ALLOWED             L"UserTimingModeAllowed"
#define DD_ATA_REG_XFER_MODE_SUPPORTED           L"DeviceTimingModeSupported"
#define DD_ATA_REG_XFER_MODE_SELECTED            L"DeviceTimingMode"

/** PIO modes 0-4 */
#define PIO_ALL \
    (PIO_MODE0 | PIO_MODE1 | PIO_MODE2 | PIO_MODE3 | PIO_MODE4)

/** SWDMA modes 0-2 */
#define SWDMA_ALL \
    (SWDMA_MODE0 | SWDMA_MODE1 | SWDMA_MODE2)

/** MWDMA modes 0-2 */
#define MWDMA_ALL \
    (MWDMA_MODE0 | MWDMA_MODE1 | MWDMA_MODE2)

/** UDMA modes 0-6 */
#define UDMA_ALL \
    (UDMA_MODE0 | UDMA_MODE1 | UDMA_MODE2 | UDMA_MODE3 | UDMA_MODE4 | UDMA_MODE5 | UDMA_MODE6)

/** Converts the provided mode number into a mode index in the bit map */
/*@{*/
#define PIO_MODE(n)      (n)
#define SWDMA_MODE(n)    (5 + (n))
#define MWDMA_MODE(n)    (8 + (n))
#define UDMA_MODE(n)     (11 + (n))
/*@}*/

/**
 * @brief Private enum between the ATA driver and storprop.dll
 * @sa DD_ATA_REG_ATA_DEVICE_TYPE
 */
typedef enum _ATA_DEVICE_TYPE
{
    DEV_UNKNOWN = 0,
    DEV_ATA = 1,
    DEV_ATAPI = 2,
    DEV_NONE = 3
} ATA_DEVICE_TYPE;
