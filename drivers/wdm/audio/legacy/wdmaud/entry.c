/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/legacy/wdmaud/main.c
 * PURPOSE:         System Audio graph builder
 * PROGRAMMER:      Andrew Greenwood
 *                  Johannes Anderwald
 */
#include "wdmaud.h"

const GUID KSCATEGORY_SYSAUDIO = {0xA7C7A5B1L, 0x5AF3, 0x11D1, {0x9C, 0xED, 0x00, 0xA0, 0x24, 0xBF, 0x04, 0x07}};
const GUID KSCATEGORY_WDMAUD   = {0x3E227E76L, 0x690D, 0x11D2, {0x81, 0x61, 0x00, 0x00, 0xF8, 0x77, 0x5B, 0xF1}};

NTSTATUS
NTAPI
WdmAudInstallDevice(
    IN  PDRIVER_OBJECT  DriverObject)
{
    UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\wdmaud");
    UNICODE_STRING SymlinkName = RTL_CONSTANT_STRING(L"\\DosDevices\\wdmaud");
    PDEVICE_OBJECT DeviceObject;
    NTSTATUS Status;
    PWDMAUD_DEVICE_EXTENSION DeviceExtension;

    DPRINT1("WdmAudInstallDevice called\n");

    Status = IoCreateDevice(DriverObject,
                            sizeof(WDMAUD_DEVICE_EXTENSION),
                            &DeviceName,
                            FILE_DEVICE_KS,
                            0,
                            FALSE,
                            &DeviceObject);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IoCreateDevice failed with %x\n", Status);
        return Status;
    }

    /* clear device extension */
    DeviceExtension = (PWDMAUD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    RtlZeroMemory(DeviceExtension, sizeof(WDMAUD_DEVICE_EXTENSION));

    /* register device interfaces */
    Status = WdmAudRegisterDeviceInterface(DeviceObject, DeviceExtension);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("WdmRegisterDeviceInterface failed with %x\n", Status);
        IoDeleteDevice(DeviceObject);
        return Status;
    }

    /* initialize sysaudio device list */
    InitializeListHead(&DeviceExtension->SysAudioDeviceList);

    /* initialize spinlock */
    KeInitializeSpinLock(&DeviceExtension->Lock);

    /* find available sysaudio devices */
    Status = WdmAudOpenSysAudioDevices(DeviceObject, DeviceExtension);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("WdmAudOpenSysAudioDevices failed with %x\n", Status);
        IoDeleteSymbolicLink(&SymlinkName);
        IoDeleteDevice(DeviceObject);
        return Status;
    }
    /* allocate ks device header */
    Status = KsAllocateDeviceHeader(&DeviceExtension->DeviceHeader, 0, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("KsAllocateDeviceHeader failed with %x\n", Status);
        IoDeleteSymbolicLink(&SymlinkName);
        IoDeleteDevice(DeviceObject);
        return Status;
    }

    DeviceObject->Flags |= DO_DIRECT_IO | DO_POWER_PAGABLE;
    DeviceObject->Flags &= ~ DO_DEVICE_INITIALIZING;

    return STATUS_SUCCESS;
}

VOID
NTAPI
WdmAudUnload(
    IN PDRIVER_OBJECT driver)
{
    DPRINT1("WdmAudUnload called\n");
}

NTSTATUS
NTAPI
WdmAudPnp(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PIO_STACK_LOCATION IrpStack;

    DPRINT("WdmAudPnp called\n");

    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    if (IrpStack->MinorFunction == IRP_MN_QUERY_PNP_DEVICE_STATE)
    {
        Irp->IoStatus.Information |= PNP_DEVICE_NOT_DISABLEABLE;
        return KsDefaultDispatchPnp(DeviceObject, Irp);
    }
    return KsDefaultDispatchPnp(DeviceObject, Irp);
}


NTSTATUS
NTAPI
WdmAudCreate(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    NTSTATUS Status;

    PIO_STACK_LOCATION IoStack;
    PWDMAUD_CLIENT pClient;

    PWDMAUD_DEVICE_EXTENSION DeviceExtension;

    DPRINT("WdmAudCreate\n");

    DeviceExtension = (PWDMAUD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

#if KS_IMPLEMENTED
    Status = KsReferenceSoftwareBusObject((KSDEVICE_HEADER)DeviceObject->DeviceExtension);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("KsReferenceSoftwareBusObject failed with %x\n", Status);
        return Status;
    }
#endif

    Status = WdmAudOpenSysaudio(DeviceObject, &pClient);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open sysaudio!\n");
        if (pClient)
            ExFreePool(pClient);
    }

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    ASSERT(IoStack->FileObject);

    /* store client context in file object */
    IoStack->FileObject->FsContext = pClient;
    Status = STATUS_SUCCESS;

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

NTSTATUS
NTAPI
WdmAudClose(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    DPRINT("WdmAudClose\n");

#if KS_IMPLEMENTED
    Status = KsDereferenceSoftwareBusObject(DeviceExtension->DeviceHeader);

    if (NT_SUCCESS(Status))
    {
        if (DeviceExtension->SysAudioNotification)
            Status = IoUnregisterPlugPlayNotification(DeviceExtension->SysAudioNotification);
    }
#endif

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
WdmAudCleanup(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    WDMAUD_CLIENT *pClient;
    ULONG Index;

    DPRINT("WdmAudCleanup\n");

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    pClient = (WDMAUD_CLIENT*)IoStack->FileObject->FsContext;

    if (pClient)
    {
        for (Index = 0; Index < pClient->NumPins; Index++)
        {
           if (pClient->hPins[Index])
           {
               ZwClose(pClient->hPins[Index]);
           }
        }

        if (pClient->hPins)
        {
            ExFreePool(pClient->hPins);
        }

        ObDereferenceObject(pClient->FileObject);
        ZwClose(pClient->hSysAudio);
        ExFreePool(pClient);
        IoStack->FileObject->FsContext = NULL;
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    DPRINT("WdmAudCleanup complete\n");
    return STATUS_SUCCESS;
}



NTSTATUS NTAPI
DriverEntry(
    IN PDRIVER_OBJECT Driver,
    IN PUNICODE_STRING Registry_path
)
{
    DPRINT1("Wdmaud.sys loaded\n");

    Driver->DriverUnload = WdmAudUnload;


    Driver->MajorFunction[IRP_MJ_CREATE] = WdmAudCreate;
    Driver->MajorFunction[IRP_MJ_CLOSE] = WdmAudClose;
    Driver->MajorFunction[IRP_MJ_PNP] = WdmAudPnp;
    Driver->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = KsDefaultForwardIrp; 
    Driver->MajorFunction[IRP_MJ_CLEANUP] = WdmAudCleanup;
    Driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = WdmAudDeviceControl;
    Driver->MajorFunction[IRP_MJ_WRITE] = WdmAudWrite;
    Driver->MajorFunction[IRP_MJ_POWER] = KsDefaultDispatchPower;

    return WdmAudInstallDevice(Driver);
}
