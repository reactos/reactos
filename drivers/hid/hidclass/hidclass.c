/*
 * PROJECT:     ReactOS Universal Serial Bus Human Interface Device Driver
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/hid/hidclass/hidclass.c
 * PURPOSE:     HID Class Driver
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 *              Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "precomp.h"

static LPWSTR ClientIdentificationAddress = L"HIDCLASS";
static ULONG HidClassDeviceNumber = 0;

ULONG
NTAPI
DllInitialize(ULONG Unknown)
{
    return 0;
}

ULONG
NTAPI
DllUnload()
{
    return 0;
}

NTSTATUS
NTAPI
HidClassAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject)
{
    WCHAR CharDeviceName[64];
    NTSTATUS Status;
    UNICODE_STRING DeviceName;
    PDEVICE_OBJECT NewDeviceObject;
    PHIDCLASS_DEVICE_EXTENSION DeviceExtension;
    ULONG DeviceExtensionSize;
    PHIDCLASS_DRIVER_EXTENSION DriverExtension;


    /* increment device number */
    InterlockedIncrement((PLONG)&HidClassDeviceNumber);

    /* construct device name */
    swprintf(CharDeviceName, L"\\Device\\_HID%08x", HidClassDeviceNumber);

    /* initialize device name */
    RtlInitUnicodeString(&DeviceName, CharDeviceName);

    /* get driver object extension */
    DriverExtension = (PHIDCLASS_DRIVER_EXTENSION) IoGetDriverObjectExtension(DriverObject, ClientIdentificationAddress);
    if (!DriverExtension)
    {
        /* device removed */
        ASSERT(FALSE);
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    /* calculate device extension size */
    DeviceExtensionSize = sizeof(HIDCLASS_DEVICE_EXTENSION) + DriverExtension->DeviceExtensionSize;

    /* now create the device */
    Status = IoCreateDevice(DriverObject, DeviceExtensionSize, &DeviceName, FILE_DEVICE_UNKNOWN, 0, FALSE, &NewDeviceObject);
    if (!NT_SUCCESS(Status))
    {
        /* failed to create device object */
        ASSERT(FALSE);
        return Status;
    }

    /* get device extension */
    DeviceExtension = (PHIDCLASS_DEVICE_EXTENSION)NewDeviceObject->DeviceExtension;

    /* zero device extension */
    RtlZeroMemory(DeviceExtension, sizeof(HIDCLASS_DEVICE_EXTENSION));

    /* initialize device extension */
    DeviceExtension->HidDeviceExtension.PhysicalDeviceObject = PhysicalDeviceObject;
    DeviceExtension->HidDeviceExtension.MiniDeviceExtension = (PVOID)((ULONG_PTR)DeviceExtension + sizeof(HIDCLASS_DEVICE_EXTENSION));
    DeviceExtension->HidDeviceExtension.NextDeviceObject = IoAttachDeviceToDeviceStack(NewDeviceObject, PhysicalDeviceObject);
    DeviceExtension->IsFDO = TRUE;
    DeviceExtension->DriverExtension = DriverExtension;

    /* sanity check */
    ASSERT(DeviceExtension->HidDeviceExtension.NextDeviceObject);

    /* increment stack size */
    NewDeviceObject->StackSize++;

    /* init device object */
    NewDeviceObject->Flags |= DO_BUFFERED_IO | DO_POWER_PAGABLE;
    NewDeviceObject->Flags  &= ~DO_DEVICE_INITIALIZING;

    /* now call driver provided add device routine */
    ASSERT(DriverExtension->AddDevice != 0);
    Status = DriverExtension->AddDevice(DriverObject, NewDeviceObject);
    if (!NT_SUCCESS(Status))
    {
        /* failed */
        DPRINT1("HIDCLASS: AddDevice failed with %x\n", Status);
        IoDetachDevice(DeviceExtension->HidDeviceExtension.NextDeviceObject);
        IoDeleteDevice(NewDeviceObject);
        return Status;
    }

    /* succeeded */
    return Status;
}

NTSTATUS
NTAPI
HidClassDriverUnload(
    IN PDRIVER_OBJECT DriverObject)
{
    UNIMPLEMENTED
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
HidClassDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    UNIMPLEMENTED
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
HidRegisterMinidriver(
    IN PHID_MINIDRIVER_REGISTRATION MinidriverRegistration)
{
    NTSTATUS Status;
    PHIDCLASS_DRIVER_EXTENSION DriverExtension;

    /* check if the version matches */
    if (MinidriverRegistration->Revision > HID_REVISION)
    {
        /* revision mismatch */
        ASSERT(FALSE);
        return STATUS_REVISION_MISMATCH;
    }

    /* now allocate the driver object extension */
    Status = IoAllocateDriverObjectExtension(MinidriverRegistration->DriverObject, (PVOID)ClientIdentificationAddress, sizeof(HIDCLASS_DRIVER_EXTENSION), (PVOID*)&DriverExtension);
    if (!NT_SUCCESS(Status))
    {
        /* failed to allocate driver extension */
        ASSERT(FALSE);
        return Status;
    }

    /* zero driver extension */
    RtlZeroMemory(DriverExtension, sizeof(HIDCLASS_DRIVER_EXTENSION));

    /* init driver extension */
    DriverExtension->DriverObject = MinidriverRegistration->DriverObject;
    DriverExtension->DeviceExtensionSize = MinidriverRegistration->DeviceExtensionSize;
    DriverExtension->DevicesArePolled = MinidriverRegistration->DevicesArePolled;
    DriverExtension->AddDevice = MinidriverRegistration->DriverObject->DriverExtension->AddDevice;
    DriverExtension->DriverUnload = MinidriverRegistration->DriverObject->DriverUnload;

    /* copy driver dispatch routines */
    RtlCopyMemory(DriverExtension->MajorFunction, MinidriverRegistration->DriverObject->MajorFunction, sizeof(PDRIVER_DISPATCH) * IRP_MJ_MAXIMUM_FUNCTION);

    /* initialize lock */
    KeInitializeSpinLock(&DriverExtension->Lock);

    /* now replace dispatch routines */
    DriverExtension->DriverObject->DriverExtension->AddDevice = HidClassAddDevice;
    DriverExtension->DriverObject->DriverUnload = HidClassDriverUnload;
    DriverExtension->DriverObject->MajorFunction[IRP_MJ_CREATE] = HidClassDispatch;
    DriverExtension->DriverObject->MajorFunction[IRP_MJ_CLOSE] = HidClassDispatch;
    DriverExtension->DriverObject->MajorFunction[IRP_MJ_READ] = HidClassDispatch;
    DriverExtension->DriverObject->MajorFunction[IRP_MJ_WRITE] = HidClassDispatch;
    DriverExtension->DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = HidClassDispatch;
    DriverExtension->DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = HidClassDispatch;
    DriverExtension->DriverObject->MajorFunction[IRP_MJ_POWER] = HidClassDispatch;
    DriverExtension->DriverObject->MajorFunction[IRP_MJ_PNP] = HidClassDispatch;

    /* done */
    return STATUS_SUCCESS;
}
