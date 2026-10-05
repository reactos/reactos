/*
 * PROJECT:     ReactOS SuperSpeed USB Hub Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Live kernel dump requests and their per boot throttle
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "usbhub3.h"

#define NDEBUG
#include <debug.h>

/* Work item context for a request made at DISPATCH_LEVEL */
struct HubReportWork
{
    HubReport Type;
    ULONG SubReason;
};

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(HubReportWork, HubGetReportWork);

/*
 * The throttle lives in a volatile subkey of the device's hardware key, so
 * it resets every boot. Any failure reads as "not throttled".
 */
_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
HubOpenThrottleKey(
    _In_ WDFDEVICE Device,
    _Out_ WDFKEY* WerKey)
{
    DECLARE_CONST_UNICODE_STRING(WerName, L"Wer");
    WDFKEY HardwareKey;
    NTSTATUS Status;

    *WerKey = NULL;

    Status = WdfDeviceOpenRegistryKey(Device,
                                      PLUGPLAY_REGKEY_DEVICE,
                                      KEY_READ | KEY_WRITE,
                                      WDF_NO_OBJECT_ATTRIBUTES,
                                      &HardwareKey);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Hub %p hardware key open failed 0x%lx\n", Device, Status);
        return Status;
    }

    Status = WdfRegistryCreateKey(HardwareKey,
                                  &WerName,
                                  KEY_READ | KEY_WRITE,
                                  REG_OPTION_VOLATILE,
                                  NULL,
                                  WDF_NO_OBJECT_ATTRIBUTES,
                                  WerKey);
    if (!NT_SUCCESS(Status))
        DPRINT1("Hub %p WER throttle key create failed 0x%lx\n", Device, Status);

    WdfRegistryClose(HardwareKey);
    return Status;
}

_IRQL_requires_(PASSIVE_LEVEL)
static
ULONG
NTAPI
HubReadThrottleMask(
    _In_ WDFDEVICE Device)
{
    DECLARE_CONST_UNICODE_STRING(MaskName, L"ThrottleMask");
    WDFKEY Key;
    ULONG Mask = 0;
    NTSTATUS Status;

    if (!NT_SUCCESS(HubOpenThrottleKey(Device, &Key)))
        return 0;

    Status = WdfRegistryQueryULong(Key, &MaskName, &Mask);
    if (!NT_SUCCESS(Status))
    {
        if (Status != STATUS_OBJECT_NAME_NOT_FOUND)
            DPRINT1("Reading the WER throttle failed 0x%lx\n", Status);
        Mask = 0;
    }

    WdfRegistryClose(Key);
    return Mask;
}

/* No live dump API exists, so the throttle is honored but nothing is captured and no bit is ever added */
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
HubSubmitLiveDump(
    _In_ WDFDEVICE Device,
    _In_ ULONG Code,
    _In_ ULONG SubReason)
{
    PAGED_CODE();

    if (HubReadThrottleMask(Device) & (1UL << (Code & 0x1F)))
    {
        DPRINT("Hub %p live dump 0x%lx throttled\n", Device, Code);
        return STATUS_UNSUCCESSFUL;
    }

    DPRINT1("USBHUB3 live dump 0x%lx, reason 0x%lx, not supported\n", Code, SubReason);

    return STATUS_NOT_SUPPORTED;
}

static
ULONG
NTAPI
HubReportCode(
    _In_ HubReport Type)
{
    switch (Type)
    {
        case HubReport::HubResetSucceeded:
            return HUB_LIVEDUMP_HUB_RESET_OK;

        case HubReport::HubResetFailed:
            return HUB_LIVEDUMP_HUB_RESET_FAILED;

        case HubReport::DeviceEnumerationFailed:
            return HUB_LIVEDUMP_ENUMERATION_FAILED;

        default:
            return 0;
    }
}

static
VOID
NTAPI
HubEvtReportWorkItem(
    _In_ WDFWORKITEM WorkItem)
{
    HubReportWork* Work = HubGetReportWork(WorkItem);

    HubCreateReport((WDFDEVICE)WdfWorkItemGetParentObject(WorkItem), Work->Type, Work->SubReason);
    WdfObjectDelete(WorkItem);
}

/** Device enumeration failures are throttled per hub, since no device is passed. */
NTSTATUS
NTAPI
HubCreateReport(
    _In_ WDFDEVICE HubFdo,
    _In_ HubReport Type,
    _In_ ULONG SubReason)
{
    WDF_OBJECT_ATTRIBUTES Attributes;
    WDF_WORKITEM_CONFIG Config;
    WDFWORKITEM WorkItem;
    NTSTATUS Status;

    if (KeGetCurrentIrql() < DISPATCH_LEVEL)
        return HubSubmitLiveDump(HubFdo, HubReportCode(Type), SubReason);

    WDF_WORKITEM_CONFIG_INIT(&Config, HubEvtReportWorkItem);
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&Attributes, HubReportWork);
    Attributes.ParentObject = HubFdo;

    Status = WdfWorkItemCreate(&Config, &Attributes, &WorkItem);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Hub %p report work item create failed 0x%lx\n", HubFdo, Status);
        return Status;
    }

    HubGetReportWork(WorkItem)->Type = Type;
    HubGetReportWork(WorkItem)->SubReason = SubReason;
    WdfWorkItemEnqueue(WorkItem);

    return STATUS_SUCCESS;
}
