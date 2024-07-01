/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Legacy (non-PnP) IDE controllers support
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

/* GLOBALS ********************************************************************/

typedef struct _ATA_LEGACY_CHANNEL
{
    ULONG IoBase;
    ULONG Irq;
} ATA_LEGACY_CHANNEL, *PATA_LEGACY_CHANNEL;

/* FUNCTIONS ******************************************************************/

CODE_SEG("INIT")
VOID
AtaDetectLegacyDevices(
    _In_ PDRIVER_OBJECT DriverObject)
{
    // TODO
}
