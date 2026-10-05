/*
 * PROJECT:     ReactOS USB Host Controller Extension
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Driver entry
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "ucx01000.h"

#define NDEBUG
#include <debug.h>

extern "C" DRIVER_INITIALIZE DriverEntry;

extern "C"
NTSTATUS
NTAPI
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
{
    WDF_DRIVER_CONFIG Config;
    NTSTATUS Status;

    WDF_DRIVER_CONFIG_INIT(&Config, WDF_NO_EVENT_CALLBACK);
    Config.DriverInitFlags |= WdfDriverInitNonPnpDriver;
    Config.DriverPoolTag = UCX_POOL_TAG;

    Status = WdfDriverCreate(DriverObject, RegistryPath, WDF_NO_OBJECT_ATTRIBUTES, &Config, WDF_NO_HANDLE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("WdfDriverCreate failed 0x%lx\n", Status);
        return Status;
    }

    Status = WdfRegisterClassLibrary(&UcxClassLibraryInfo, RegistryPath, NULL);
    if (!NT_SUCCESS(Status))
        DPRINT1("WdfRegisterClassLibrary failed 0x%lx\n", Status);

    return Status;
}
