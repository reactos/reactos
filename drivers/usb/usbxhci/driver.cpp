/*
 * PROJECT:     ReactOS xHCI Host Controller Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Driver entry and unload
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "usbxhci.h"

#define NDEBUG
#include <debug.h>

extern "C"
NTSTATUS
NTAPI
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
{
    WDF_DRIVER_CONFIG Config;
    NTSTATUS Status;

    WDF_DRIVER_CONFIG_INIT(&Config, XhciEvtDeviceAdd);
    Config.EvtDriverUnload = XhciEvtDriverUnload;
    Config.DriverPoolTag = XHCI_POOL_TAG;

    Status = WdfDriverCreate(DriverObject, RegistryPath, WDF_NO_OBJECT_ATTRIBUTES, &Config, WDF_NO_HANDLE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("WdfDriverCreate failed 0x%lx\n", Status);
        return Status;
    }

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
