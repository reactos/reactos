/*
 * PROJECT:     ReactOS Broadcom NetXtreme Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Interrupt handlers
 * COPYRIGHT:   Copyright 2021 Scott Maday <coldasdryice1@gmail.com>
 */

#include "nic.h"

#include "debug.h"

VOID
NTAPI
MiniportISR(OUT PBOOLEAN InterruptRecognized,
            OUT PBOOLEAN QueueMiniportHandleInterrupt,
            IN NDIS_HANDLE MiniportAdapterContext)
{
    //ULONG Value;
    //PB57XX_ADAPTER Adapter = (PB57XX_ADAPTER)MiniportAdapterContext;
    
    NDIS_MinDbgPrint("B57XX ISR\n");

    /* Reading the interrupt acknowledges them */
    /*B57XXReadUlong(Adapter, B57XX_REG_ICR, &Value);

    Value &= Adapter->InterruptMask;
    _InterlockedOr(&Adapter->InterruptPending, Value);

    if (Value)
    {
        *InterruptRecognized = TRUE;
        // Mark the events pending service
        *QueueMiniportHandleInterrupt = TRUE;
    }
    else
    {
        // This is not ours.
        *InterruptRecognized = FALSE;
        *QueueMiniportHandleInterrupt = FALSE;
    }*/
}

VOID
NTAPI
MiniportHandleInterrupt(IN NDIS_HANDLE MiniportAdapterContext)
{
    //ULONG InterruptPending;
    //PB57XX_ADAPTER Adapter = (PB57XX_ADAPTER)MiniportAdapterContext;

    NDIS_MinDbgPrint("B57XX HandleInterrupt\n");
}
