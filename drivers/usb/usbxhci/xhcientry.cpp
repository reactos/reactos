/*
 * PROJECT:     ReactOS xHCI Host Controller Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Driver entry and unload
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "xhcidrv.h"

#define NDEBUG
#include <debug.h>

#define XHCI_CODE_INTEGRITY_CLASS       103
#define XHCI_CODE_INTEGRITY_TESTSIGN    0x00000002

typedef struct _XHCI_CODE_INTEGRITY
{
    ULONG Length;
    ULONG CodeIntegrityOptions;
} XHCI_CODE_INTEGRITY;

extern "C"
NTSYSAPI
NTSTATUS
NTAPI
ZwQuerySystemInformation(
    _In_ ULONG SystemInformationClass,
    _Out_writes_bytes_opt_(Length) PVOID SystemInformation,
    _In_ ULONG Length,
    _Out_opt_ PULONG ReturnLength);

XhciDriverData XhciDriver;

static
BOOLEAN
NTAPI
XhciIsTestSigningOn(VOID)
{
    XHCI_CODE_INTEGRITY Info;
    NTSTATUS Status;

    Info.Length = sizeof(Info);
    Info.CodeIntegrityOptions = 0;

    Status = ZwQuerySystemInformation(XHCI_CODE_INTEGRITY_CLASS, &Info, sizeof(Info), NULL);
    if (!NT_SUCCESS(Status))
        return FALSE;

    return (Info.CodeIntegrityOptions & XHCI_CODE_INTEGRITY_TESTSIGN) != 0;
}

extern "C"
NTSTATUS
NTAPI
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
{
    UNICODE_STRING RoutineName;
    WDF_DRIVER_CONFIG Config;
    NTSTATUS Status;

    XhciDriver.DriverObject = DriverObject;

    /* Resolved at run time so older kernels without the export still load us */
    RtlInitUnicodeString(&RoutineName, L"KseQueryDeviceFlags");
    XhciDriver.QueryDeviceFlags = MmGetSystemRoutineAddress(&RoutineName);

    WDF_DRIVER_CONFIG_INIT(&Config, XhciEvtDeviceAdd);
    Config.EvtDriverUnload = XhciEvtDriverUnload;
    Config.DriverPoolTag = XHCI_TAG_DRIVER;

    Status = WdfDriverCreate(DriverObject, RegistryPath, WDF_NO_OBJECT_ATTRIBUTES, &Config, WDF_NO_HANDLE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("WdfDriverCreate failed 0x%lx\n", Status);
        return Status;
    }

    XhciDriver.TestMode = XhciIsTestSigningOn();
    if (XhciDriver.TestMode)
        DPRINT1("Test signing is on, test hooks enabled\n");

    Status = UcxStubBindClass();
    if (!NT_SUCCESS(Status))
        DPRINT1("Binding to ucx01000 failed 0x%lx\n", Status);

    return Status;
}

VOID
NTAPI
XhciEvtDriverUnload(
    _In_ WDFDRIVER Driver)
{
    UNREFERENCED_PARAMETER(Driver);

    UcxStubUnbindClass();
}
