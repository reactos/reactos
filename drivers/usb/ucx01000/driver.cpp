/*
 * PROJECT:     ReactOS USB Host Controller Extension
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Driver entry, the control device and driver wide settings
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "ucx01000.h"

#define NDEBUG
#include <debug.h>

#define UCX_CONTROL_DEVICE_ATTEMPTS 100

extern "C" DRIVER_INITIALIZE DriverEntry;
static EVT_WDF_DRIVER_UNLOAD UcxEvtDriverUnload;

/* SDDL_DEVOBJ_KERNEL_ONLY; wdmsec.h is not available here */
DECLARE_CONST_UNICODE_STRING(UcxKernelOnlySddl, L"D:P");

/* The loader needs a named device; another instance may own the first name */
_IRQL_requires_(PASSIVE_LEVEL)
static
NTSTATUS
NTAPI
UcxCreateControlDevice(
    _In_ WDFDRIVER Driver)
{
    PWDFDEVICE_INIT DeviceInit;
    WDFDEVICE Device;
    ULONG Index;
    NTSTATUS Status = STATUS_OBJECT_NAME_COLLISION;

    PAGED_CODE();

    DeviceInit = WdfControlDeviceInitAllocate(Driver, &UcxKernelOnlySddl);
    if (DeviceInit == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlInitEmptyUnicodeString(&UcxDriver.ControlDeviceName,
                              UcxDriver.ControlDeviceNameBuffer,
                              sizeof(UcxDriver.ControlDeviceNameBuffer));

    for (Index = 0; Index < UCX_CONTROL_DEVICE_ATTEMPTS && Status == STATUS_OBJECT_NAME_COLLISION; Index++)
    {
        Status = RtlUnicodeStringPrintf(&UcxDriver.ControlDeviceName, L"\\Device\\UCX%lu", Index);
        if (!NT_SUCCESS(Status))
            break;

        Status = WdfDeviceInitAssignName(DeviceInit, &UcxDriver.ControlDeviceName);
        if (!NT_SUCCESS(Status))
            break;

        Status = WdfDeviceCreate(&DeviceInit, WDF_NO_OBJECT_ATTRIBUTES, &Device);
    }

    /* A successful create took the init block over */
    if (!NT_SUCCESS(Status))
    {
        if (DeviceInit != NULL)
            WdfDeviceInitFree(DeviceInit);
        return Status;
    }

    WdfControlFinishInitializing(Device);
    UcxDriver.ControlDevice = Device;

    return STATUS_SUCCESS;
}

/* Only a value of exactly 1 turns it on; anything missing leaves it off */
_IRQL_requires_(PASSIVE_LEVEL)
static
VOID
NTAPI
UcxReadUsbFlags(VOID)
{
    DECLARE_CONST_UNICODE_STRING(KeyName, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\usbflags");
    DECLARE_CONST_UNICODE_STRING(ValueName, L"Allow64KLowOrFullSpeedControlTransfers");
    WDFKEY Key;
    ULONG Value;
    NTSTATUS Status;

    PAGED_CODE();

    Status = WdfRegistryOpenKey(NULL, &KeyName, KEY_READ, WDF_NO_OBJECT_ATTRIBUTES, &Key);
    if (!NT_SUCCESS(Status))
    {
        if (Status != STATUS_OBJECT_NAME_NOT_FOUND)
            DPRINT1("Cannot open usbflags 0x%lx\n", Status);
        return;
    }

    Status = WdfRegistryQueryULong(Key, &ValueName, &Value);
    if (NT_SUCCESS(Status))
        UcxDriver.Allow64KLowOrFullSpeedControl = (Value == 1);
    else if (Status != STATUS_OBJECT_NAME_NOT_FOUND)
        DPRINT1("Cannot read Allow64KLowOrFullSpeedControlTransfers 0x%lx\n", Status);

    WdfRegistryClose(Key);
}

static
VOID
NTAPI
UcxEvtDriverUnload(
    _In_ WDFDRIVER Driver)
{
    UNREFERENCED_PARAMETER(Driver);

    PAGED_CODE();

    if (UcxDriver.ControlDevice != NULL)
    {
        WdfObjectDelete(UcxDriver.ControlDevice);
        UcxDriver.ControlDevice = NULL;
    }
}

extern "C"
NTSTATUS
NTAPI
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
{
    DECLARE_CONST_UNICODE_STRING(TryQueueName, L"IoTryQueueWorkItem");
    WDF_DRIVER_CONFIG Config;
    LARGE_INTEGER Ticks;
    NTSTATUS Status;

    KeQueryTickCount(&Ticks);
    UcxDriver.RandomSeed = Ticks.LowPart;

    InitializeListHead(&UcxDriver.ControllerList);
    KeInitializeSpinLock(&UcxDriver.ControllerListLock);
    UcxDriver.ControllerCount = 0;

    WDF_DRIVER_CONFIG_INIT(&Config, WDF_NO_EVENT_CALLBACK);
    Config.DriverInitFlags |= WdfDriverInitNonPnpDriver;
    Config.EvtDriverUnload = UcxEvtDriverUnload;
    Config.DriverPoolTag = UCX_POOL_TAG;

    Status = WdfDriverCreate(DriverObject, RegistryPath, WDF_NO_OBJECT_ATTRIBUTES, &Config, &UcxDriver.Driver);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("WdfDriverCreate failed 0x%lx\n", Status);
        return Status;
    }

    Status = UcxCreateControlDevice(UcxDriver.Driver);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Cannot create the control device 0x%lx\n", Status);
        return Status;
    }

    Status = WdfRegisterClassLibrary(&UcxClassLibraryInfo, RegistryPath, &UcxDriver.ControlDeviceName);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("WdfRegisterClassLibrary failed 0x%lx\n", Status);
        WdfObjectDelete(UcxDriver.ControlDevice);
        UcxDriver.ControlDevice = NULL;
        return Status;
    }

    UcxDriver.IoTryQueueWorkItem =
        (PFN_UCX_IO_TRY_QUEUE_WORKITEM)MmGetSystemRoutineAddress((PUNICODE_STRING)&TryQueueName);

    UcxReadUsbFlags();

    DPRINT("UCX loaded, control device %wZ\n", &UcxDriver.ControlDeviceName);
    return STATUS_SUCCESS;
}
