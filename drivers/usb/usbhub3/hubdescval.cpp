/*
 * PROJECT:     ReactOS SuperSpeed USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Descriptor validation of the hub's own descriptors
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "usbhub3.h"

#define NDEBUG
#include <debug.h>

/** Hub findings have no bitmap and no verifier break, only a debug print. */
static
VOID
NTAPI
HubLogHubFinding(
    _In_opt_ PVOID Context,
    _In_ HubDescCode Code,
    _In_ BOOLEAN Warning)
{
    if (Context == NULL)
        return;

    if (Warning)
        DPRINT("Hub %p descriptor check %s (%lu) ignored\n", Context, HubDescCodeName(Code), (ULONG)Code);
    else
        DPRINT1("Hub %p descriptor check %s (%lu) failed\n", Context, HubDescCodeName(Code), (ULONG)Code);
}

/* The global strictness flags never apply to a hub's own descriptors */
static
VOID
NTAPI
HubBuildDescContext(
    _In_ HubFdo* Hub,
    _Out_ HubDescContext* Context)
{
    HubDescInitHubContext(Context, Hub->m_ParentInfo.DeviceDescriptor.bcdUSB, Hub->m_Parent.HubSpeed);
    Context->Log = HubLogHubFinding;
    Context->LogContext = Hub;
}

BOOLEAN
NTAPI
HubValidateHubConfiguration(
    _In_ HubFdo* Hub,
    _In_reads_bytes_(Length) PUSB_CONFIGURATION_DESCRIPTOR Descriptor,
    _In_ ULONG Length)
{
    HubDescContext Context;

    HubBuildDescContext(Hub, &Context);
    return HubDescCheckConfiguration(&Context, Descriptor, Length, NULL);
}

BOOLEAN
NTAPI
HubCheckUsb2HubDescriptor(
    _In_ HubFdo* Hub,
    _In_reads_bytes_(Length) PUSB_HUB_DESCRIPTOR Descriptor,
    _In_ ULONG Length)
{
    HubDescContext Context;

    HubBuildDescContext(Hub, &Context);
    return HubDescCheck20Hub(&Context, Descriptor, Length);
}

BOOLEAN
NTAPI
HubCheckUsb3HubDescriptor(
    _In_ HubFdo* Hub,
    _In_reads_bytes_(Length) PUSB_30_HUB_DESCRIPTOR Descriptor,
    _In_ ULONG Length)
{
    HubDescContext Context;

    HubBuildDescContext(Hub, &Context);
    return HubDescCheck30Hub(&Context, Descriptor, Length);
}

VOID
NTAPI
HubLogConfigTotalLengthMismatch(
    _In_ HubFdo* Hub)
{
    HubDescContext Context;

    HubBuildDescContext(Hub, &Context);
    HubDescLog(&Context, HubDescCode::ConfigTotalLengthTooLarge, FALSE);
}
