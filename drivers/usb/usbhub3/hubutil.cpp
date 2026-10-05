/*
 * PROJECT:     ReactOS SuperSpeed USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Shared helpers
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "usbhub3.h"

#define NDEBUG
#include <debug.h>

_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubWaitForPnpEvent(
    _In_ PKEVENT Event,
    _In_ PCSTR What,
    _In_ WDFOBJECT Object)
{
    LARGE_INTEGER Timeout;
    ULONG Minutes = 0;

    PAGED_CODE();

    Timeout.QuadPart = -60LL * 1000 * 1000 * 10;

    while (KeWaitForSingleObject(Event, Executive, KernelMode, FALSE, &Timeout) == STATUS_TIMEOUT)
    {
        Minutes++;
        DbgPrint("USBHUB3 Watchdog: Thread 0x%p has waited %lu minutes for %s to complete for WDF object 0x%p\n",
                 KeGetCurrentThread(),
                 Minutes,
                 What,
                 Object);
    }
}

/* Hub numbers come from usbd.sys; 0 means none was free */
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubAllocateHubNumber(
    _Out_ PULONG HubNumber)
{
    *HubNumber = USBD_AllocateHubNumber();
    if (*HubNumber == 0)
        DPRINT1("No free hub number\n");

    return STATUS_SUCCESS;
}

_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
HubReleaseHubNumber(
    _In_ ULONG HubNumber)
{
    USBD_ReleaseHubNumber(HubNumber);
}
