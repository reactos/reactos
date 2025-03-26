/*
 * PROJECT:     ReactOS DC21x4 Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Debug routines
 * COPYRIGHT:   Copyright 2023 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "dc21x4.h"

/* GLOBALS ********************************************************************/

static PCSTR MediaName[MEDIA_MAX] =
{
    "10Base-T",
    "10Base-2 (BNC)",
    "10Base-5 (AUI)",
    "100Base-TX HD",
    "10Base-T FD",
    "100Base-TX FD",
    "100Base-T4",
    "100Base-FX HD",
    "100Base-FX FD",
    "HomePNA",
    "MII",
};

/* FUNCTIONS ******************************************************************/

PCSTR
MediaNumber2Str(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ ULONG MediaNumber)
{
    switch (MediaNumber)
    {
        case MEDIA_100TX_HD:
        {
            if (Adapter->ChipType == DC21041)
                return "10Base-T HD";
            break;
        }

        default:
            break;
    }

    ASSERT(MediaNumber < MEDIA_MAX);

    return MediaName[MediaNumber];
}

PCSTR
DcDbgBusError(
    _In_ ULONG InterruptStatus)
{
    switch (InterruptStatus & DC_STATUS_SYSTEM_ERROR_MASK)
    {
        case DC_STATUS_SYSTEM_ERROR_PARITY:
            return "Parity Error";
        case DC_STATUS_SYSTEM_ERROR_MASTER_ABORT:
            return "Master Abort";
        case DC_STATUS_SYSTEM_ERROR_TARGET_ABORT:
            return "Target Abort";

        default:
            return "<unknown>";
    }
}
