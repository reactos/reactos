#include "priv.h"

#include "ksfunc.h"


NTSTATUS
NTAPI
DriverEntry(
    IN PDRIVER_OBJECT Driver,
    IN PUNICODE_STRING Registry_path
)
{
    DPRINT1("ks.sys loaded\n");
    return STATUS_SUCCESS;
}

/*
    @unimplemented
*/
KSDDKAPI
PKSDEVICE
NTAPI
KsGetDeviceForDeviceObject(
    IN PDEVICE_OBJECT FunctionalDeviceObject)
{

    return NULL;
}

/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsInitializeDevice(
    IN PDEVICE_OBJECT FunctionalDeviceObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject,
    IN PDEVICE_OBJECT NextDeviceObject,
    IN const KSDEVICE_DESCRIPTOR* Descriptor OPTIONAL)
{
   return STATUS_UNSUCCESSFUL;
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
        ExtensionSize = sizeof(PVOID);

    Status = IoCreateDevice(DriverObject, ExtensionSize, NULL, FILE_DEVICE_KS, FILE_DEVICE_SECURE_OPEN, FALSE, &FunctionalDeviceObject);
    if (!NT_SUCCESS(Status))
        return Status;

    OldHighestDeviceObject = IoAttachDeviceToDeviceStack(FunctionalDeviceObject, PhysicalDeviceObject);
    if (OldHighestDeviceObject)
    {
        Status = KsInitializeDevice(FunctionalDeviceObject, PhysicalDeviceObject, OldHighestDeviceObject, Descriptor);
    }

    if (!NT_SUCCESS(Status))
    {
        if (OldHighestDeviceObject)
            IoDetachDevice(OldHighestDeviceObject);

        IoDeleteDevice(FunctionalDeviceObject);
        return Status;
    }

    if (Device)
    {
        *Device = KsGetDeviceForDeviceObject(FunctionalDeviceObject);
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
    PKSDEVICE_DESCRIPTOR *DriverObjectExtension;
    PKSDEVICE_DESCRIPTOR Descriptor = NULL;

    DriverObjectExtension = IoGetDriverObjectExtension(DriverObject, (PVOID)KsAddDevice);
    if (DriverObjectExtension)
    {
        Descriptor = *DriverObjectExtension;
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
    PKSDEVICE_DESCRIPTOR *DriverObjectExtension;
    NTSTATUS Status;

    if (Descriptor)
    {
        Status = IoAllocateDriverObjectExtension(DriverObject, (PVOID)KsAddDevice, sizeof(PKSDEVICE_DESCRIPTOR), (PVOID*)&DriverObjectExtension);
        if (NT_SUCCESS(Status))
        {
            *DriverObjectExtension = (KSDEVICE_DESCRIPTOR*)Descriptor;
        }
    }
    /* Setting our IRP handlers */
    //DriverObject->MajorFunction[IRP_MJ_CREATE] = KspDispatch;
    //DriverObject->MajorFunction[IRP_MJ_PNP] = KspDispatch;
    //DriverObject->MajorFunction[IRP_MJ_POWER] = KspDispatch;

    /* The driver-supplied AddDevice */
    DriverObject->DriverExtension->AddDevice = KsAddDevice;

    /* KS handles these */
    DPRINT1("Setting KS function handlers\n");
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_CLOSE);
    KsSetMajorFunctionHandler(DriverObject, IRP_MJ_DEVICE_CONTROL);


    return STATUS_SUCCESS;
}
