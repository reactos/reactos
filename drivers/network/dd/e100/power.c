/*
 * PROJECT:     Intel PRO/100 Ethernet Controller Driver
 * LICENSE:     BSD-2-Clause (https://spdx.org/licenses/BSD-2-Clause)
 * PURPOSE:     Power management
 * COPYRIGHT:   Copyright 2026 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "e100.h"

#include <debug.h>

/* FUNCTIONS ******************************************************************/

NDIS_STATUS
EeRemoveWakeUpPattern(
    _In_ PE100_ADAPTER Adapter,
    _In_ PNDIS_PM_PACKET_PATTERN PmPattern)
{
    // TODO: Not implemented
    ERR("FIXME: Not implemented\n");
    return NDIS_STATUS_NOT_SUPPORTED;
}

NDIS_STATUS
EeAddWakeUpPattern(
    _In_ PE100_ADAPTER Adapter,
    _In_ PNDIS_PM_PACKET_PATTERN PmPattern)
{
    // TODO: Not implemented
    ERR("FIXME: Not implemented\n");
    return NDIS_STATUS_NOT_SUPPORTED;
}
