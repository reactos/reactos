/*
 * PROJECT:         ReactOS tcpip.sys
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            drivers/network/tcpip/main.c
 * PURPOSE:         tcpip.sys TCP functionality
 */

#include "precomp.h"

#include <intsafe.h>

#define NDEBUG
#include <debug.h>

static KSPIN_LOCK PortBitmapSpinlock;
static RTL_BITMAP PortBitmap;
static ULONG PortBitmapBuffer[(USHORT_MAX + 1) / 32];
static USHORT PortNumberHint = 1;

void
TcpIpInitializeTcp(void)
{
    /* Initialize the port stuff */
    KeInitializeSpinLock(&PortBitmapSpinlock);
    RtlInitializeBitMap(&PortBitmap, PortBitmapBuffer, USHORT_MAX + 1);
    RtlClearAllBits(&PortBitmap);
    /* Reserve the port 0 */
    RtlSetBit(&PortBitmap, 0);
}

BOOLEAN
AllocateTcpPort(
    _Inout_ USHORT* PortNumber,
    _In_ BOOLEAN Shared)
{
    KIRQL OldIrql;
    ULONG_PTR Bit;

    KeAcquireSpinLock(&PortBitmapSpinlock, &OldIrql);

    if (*PortNumber)
    {
        if (RtlCheckBit(&PortBitmap, *PortNumber))
        {
            if (!Shared)
                *PortNumber = 0;
            KeReleaseSpinLock(&PortBitmapSpinlock, OldIrql);
            return TRUE;
        }

        RtlSetBit(&PortBitmap, *PortNumber);
        KeReleaseSpinLock(&PortBitmapSpinlock, OldIrql);
        return FALSE;
    }

    Bit = RtlFindClearBitsAndSet(&PortBitmap, 1, PortNumberHint);
    if (Bit == ULONG_PTR_MAX)
        *PortNumber = 0;
    else
        *PortNumber = Bit;
    PortNumberHint = *PortNumber;
	KeReleaseSpinLock(&PortBitmapSpinlock, OldIrql);
    return FALSE;
}

