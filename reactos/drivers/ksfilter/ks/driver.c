/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/ksfilter/ks/driver.c
 * PURPOSE:         KS Driver functions
 * PROGRAMMER:      Johannes Anderwald
 */

#include "priv.h"

#include "ksfunc.h"


/*
    @implemented
*/
KSDDKAPI
PKSDEVICE
NTAPI
KsGetDeviceForDeviceObject(
    IN PDEVICE_OBJECT FunctionalDeviceObject)
{
    PDEVICE_EXTENSION DeviceExtension;

    /* get device extension */
    DeviceExtension = (PDEVICE_EXTENSION)FunctionalDeviceObject->DeviceExtension;

    return &DeviceExtension->DeviceHeader->KsDevice;
}

/*
    @implemented
*/
KSDDKAPI
PKSDEVICE
NTAPI
KsGetDevice(
    IN PVOID Object)
{
    PKSBASIC_HEADER BasicHeader = (PKSBASIC_HEADER)(ULONG_PTR)Object - sizeof(KSBASIC_HEADER);

    DPRINT("KsGetDevice %p\n", Object);

    ASSERT(BasicHeader->Type == KsObjectTypeFilterFactory || BasicHeader->Type == KsObjectTypeFilter || BasicHeader->Type == BasicHeader->Type);

    return BasicHeader->KsDevice;
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsCreateDevice(
    IN  PDRIVER_OBJECT DriverObject,
    IN  PDEVICE_OBJECT PhysicalDeviceObject,
    IN  const KSDEVICE_DESCRIPTOR* Descriptor OPTIONAL,
    IN  ULONG ExtensionSize OPTIONAL,
    OUT PKSDEVICE* Device OPTIONAL)
{
    NTSTATUS Status = STATUS_DEVICE_REMOVED;
    PDEVICE_OBJECT FunctionalDeviceObject= NULL;
    PDEVICE_OBJECT OldHighestDeviceObject;
    if (!ExtensionSize)
        ExtensionSize = sizeof(KSDEVICE_HEADER);

    DPRINT("KsCreateDevice Descriptor %p ExtensionSize %lu\n", Descriptor, ExtensionSize);

    Status = IoCreateDevice(DriverObject, ExtensionSize, NULL, FILE_DEVICE_KS, FILE_DEVICE_SECURE_OPEN, FALSE, &FunctionalDeviceObject);
    if (!NT_SUCCESS(Status))
        return Status;

    OldHighestDeviceObject = IoAttachDeviceToDeviceStack(FunctionalDeviceObject, PhysicalDeviceObject);
    if (OldHighestDeviceObject)
    {
        Status = KsInitializeDevice(FunctionalDeviceObject, PhysicalDeviceObject, OldHighestDeviceObject, Descriptor);
    }
    else
    {
        Status = STATUS_DEVICE_REMOVED;
    }

    /* check if all succeeded */
    if (!NT_SUCCESS(Status))
    {
        if (OldHighestDeviceObject)
            IoDetachDevice(OldHighestDeviceObject);

        IoDeleteDevice(FunctionalDeviceObject);
        return Status;
    }

    /* set device flags */
    FunctionalDeviceObject->Flags |= DO_DIRECT_IO | DO_POWER_PAGABLE;
    FunctionalDeviceObject->Flags &= ~ DO_DEVICE_INITIALIZING;

    if (Device)
    {
        /* get PKSDEVICE struct */
        *Device = KsGetDeviceForDeviceObject(FunctionalDeviceObject);

        if (ExtensionSize > sizeof(KSDEVICE_HEADER))
        {
            /* caller needs a device extension */
            (*Device)->Context = (PVOID)((ULONG_PTR)FunctionalDeviceObject->DeviceExtension + sizeof(KSDEVICE_HEADER));
        }
    }

    return Status;
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsAddDevice(
    IN  PDRIVER_OBJECT DriverObject,
    IN  PDEVICE_OBJECT PhysicalDeviceObject)
{
    PKS_DRIVER_EXTENSION DriverObjectExtension;
    const KSDEVICE_DESCRIPTOR *Descriptor = NULL;

    /* get stored driver object extension */

    DriverObjectExtension = IoGetDriverObjectExtension(DriverObject, (PVOID)KsInitializeDriver);

    if (DriverObjectExtension)
    {
        /* get the stored descriptor see KsInitializeDriver */
        Descriptor = DriverObjectExtension->Descriptor;
    }

    return KsCreateDevice(DriverObject, PhysicalDeviceObject, Descriptor, 0, NULL);
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsInitializeDriver(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING  RegistryPath,
    IN const KSDEVICE_DESCRIPTOR  *Descriptor OPTIONAL
)
{
    PKS_DRIVER_EXTENSION DriverObjectExtension;
    NTSTATUS Status = STATUS_SUCCESS;

    if (Descriptor)
    {
        Status = IoAllocateDriverObjectExtension(DriverObject, (PVOID)KsInitializeDriver, sizeof(KS_DRIVER_EXTENSION), (PVOID*)&DriverObjectExtension);
        if (NT_SUCCESS(Status))
        {
            DriverObjectExtension->Descriptor = Descriptor;
        }
    }

    /* sanity check */
    ASSERT(Status == STATUS_SUCCESS);

    if (!NT_SUCCESS(Status))
        return Status;

    /* Setting our IRP handlers */
    DriverObject->MajorFunction[IRP_MJ_CREATE] = IKsDevice_Create;
    DriverObject->MajorFunction[IRP_MJ_PNP] = IKsDevice_Pnp;
    DriverObject->MajorFunction[IRP_MJ_POWER] = IKsDevice_Power;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = KsDefaultForwardIrp;

    /* The driver unload routine */
    DriverObject->DriverUnload = KsNullDriverUnload;

    /* The driver-supplied AddDevice */
    DriverObject->DriverExtension->AddDevice = KsAddDevice;

    /* KS handles these */
    DPRINT1("KsInitializeDriver Setting KS function handlers\n");
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_CLOSE);
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_DEVICE_CONTROL);


    return STATUS_SUCCESS;
}
