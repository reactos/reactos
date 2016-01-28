/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/legacy/wdmaud/main.c
 * PURPOSE:         System Audio graph builder
 * PROGRAMMER:      Andrew Greenwood
 *                  Johannes Anderwald
 */

#include "wdmaud.h"

#define NDEBUG
#include <debug.h>

const GUID KSCATEGORY_SYSAUDIO = {0xA7C7A5B1L, 0x5AF3, 0x11D1, {0x9C, 0xED, 0x00, 0xA0, 0x24, 0xBF, 0x04, 0x07}};
const GUID KSCATEGORY_WDMAUD   = {0x3E227E76L, 0x690D, 0x11D2, {0x81, 0x61, 0x00, 0x00, 0xF8, 0x77, 0x5B, 0xF1}};

IO_WORKITEM_ROUTINE WdmAudInitWorkerRoutine;
IO_TIMER_ROUTINE WdmAudTimerRoutine;

VOID
NTAPI
WdmAudInitWorkerRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context)
{
    NTSTATUS Status;
    PWDMAUD_DEVICE_EXTENSION DeviceExtension;
    ULONG DeviceCount;

    /* get device extension */
    DeviceExtension = (PWDMAUD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;


    if (DeviceExtension->FileObject == NULL)
    {
        /* find available sysaudio devices */
        Status = WdmAudOpenSysAudioDevices(DeviceObject, DeviceExtension);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("WdmAudOpenSysAudioDevices failed with %x\n", Status);
            return;
        }
    }


    /* get device count */
    DeviceCount = GetSysAudioDeviceCount(DeviceObject);

    DPRINT("WdmAudInitWorkerRoutine SysAudioDeviceCount %ld\n", DeviceCount);

    /* was a device added / removed */
    if (DeviceCount != DeviceExtension->SysAudioDeviceCount)
    {
        /* init mmixer library */
        Status = WdmAudMixerInitialize(DeviceObject);
        DPRINT("WdmAudMixerInitialize Status %x WaveIn %lu WaveOut %lu Mixer %lu\n", Status, WdmAudGetWaveInDeviceCount(), WdmAudGetWaveOutDeviceCount(), WdmAudGetMixerDeviceCount());

        /* store sysaudio device count */
        DeviceExtension->SysAudioDeviceCount = DeviceCount;
    }

    /* signal completion */
    KeSetEvent(&DeviceExtension->InitializationCompletionEvent, IO_NO_INCREMENT, FALSE);

    /* reset work item status indicator */
    InterlockedDecrement((volatile long *)&DeviceExtension->WorkItemActive);
}

VOID
NTAPI
WdmAudTimerRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context)
{
    PWDMAUD_DEVICE_EXTENSION DeviceExtension;

    /* get device extension */
    DeviceExtension = (PWDMAUD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    if (InterlockedCompareExchange((volatile long *)&DeviceExtension->WorkItemActive, 1, 0) == 0)
    {
        /* queue work item */
        IoQueueWorkItem(DeviceExtension->WorkItem, WdmAudInitWorkerRoutine, DelayedWorkQueue, (PVOID)DeviceExtension);
    }
}

NTSTATUS
NTAPI
WdmaudAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject)
{
    PDEVICE_OBJECT DeviceObject;
    NTSTATUS Status;
    PWDMAUD_DEVICE_EXTENSION DeviceExtension;

    DPRINT("WdmaudAddDevice called\n");

    Status = IoCreateDevice(DriverObject,
                            sizeof(WDMAUD_DEVICE_EXTENSION),
                            NULL,
                            FILE_DEVICE_KS,
                            0,
                            FALSE,
                            &DeviceObject);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IoCreateDevice failed with %x\n", Status);
        return Status;
    }

    /* get device extension */
    DeviceExtension = (PWDMAUD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    RtlZeroMemory(DeviceExtension, sizeof(WDMAUD_DEVICE_EXTENSION));

    /* allocate work item */
    DeviceExtension->WorkItem = IoAllocateWorkItem(DeviceObject);
    if (!DeviceExtension->WorkItem)
    {
        /* failed to allocate work item */
        IoDeleteDevice(DeviceObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* register device interfaces */
    Status = WdmAudRegisterDeviceInterface(PhysicalDeviceObject, DeviceExtension);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("WdmRegisterDeviceInterface failed with %x\n", Status);
        IoDeleteDevice(DeviceObject);
        return Status;
    }

    /* initialize sysaudio device list */
    InitializeListHead(&DeviceExtension->SysAudioDeviceList);

    /* initialize client context device list */
    InitializeListHead(&DeviceExtension->WdmAudClientList);

    /* initialize spinlock */
    KeInitializeSpinLock(&DeviceExtension->Lock);

    /* initialization completion event */
    KeInitializeEvent(&DeviceExtension->InitializationCompletionEvent, NotificationEvent, FALSE);

    /* initialize timer */
    IoInitializeTimer(DeviceObject, WdmAudTimerRoutine, (PVOID)WdmAudTimerRoutine);

    /* allocate ks device header */
    Status = KsAllocateDeviceHeader(&DeviceExtension->DeviceHeader, 0, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("KsAllocateDeviceHeader failed with %x\n", Status);
        IoDeleteDevice(DeviceObject);
        return Status;
    }

    /* attach to device stack */
    DeviceExtension->NextDeviceObject = IoAttachDeviceToDeviceStack(DeviceObject, PhysicalDeviceObject);
    KsSetDevicePnpAndBaseObject(DeviceExtension->DeviceHeader, DeviceExtension->NextDeviceObject, DeviceObject);


    /* start the timer */
    IoStartTimer(DeviceObject);

    DeviceObject->Flags |= DO_DIRECT_IO | DO_POWER_PAGABLE;
    DeviceObject->Flags &= ~ DO_DEVICE_INITIALIZING;

    return STATUS_SUCCESS;
}

VOID
NTAPI
WdmAudUnload(
    IN PDRIVER_OBJECT driver)
{
    DPRINT("WdmAudUnload called\n");
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

    /* get device extension */
    DeviceExtension = (PWDMAUD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

#if KS_IMPLEMENTED
    Status = KsReferenceSoftwareBusObject((KSDEVICE_HEADER)DeviceObject->DeviceExtension);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("KsReferenceSoftwareBusObject failed with %x\n", Status);
        return Status;
    }
#endif

    if (DeviceExtension->FileObject == NULL)
    {
        /* initialize */
        WdmAudInitWorkerRoutine(DeviceObject, NULL);
    }


    Status = WdmAudOpenSysaudio(DeviceObject, &pClient);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open sysaudio!\n");

        /* complete and forget */
        Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        /* done */
        return STATUS_UNSUCCESSFUL;
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
    /* nothing to do complete request */
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

    /* done */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
WdmAudCleanup(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PWDMAUD_CLIENT pClient;
    PWDMAUD_DEVICE_EXTENSION DeviceExtension;
    ULONG Index;
    KIRQL OldIrql;

    /* get device extension */
    DeviceExtension = (PWDMAUD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    /* get current irp stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* sanity check */
    ASSERT(IoStack->FileObject);

    /* get client context struct */
    pClient = (PWDMAUD_CLIENT)IoStack->FileObject->FsContext;

    /* sanity check */
    ASSERT(pClient);

    /* acquire client context list lock */
    KeAcquireSpinLock(&DeviceExtension->Lock, &OldIrql);

    /* remove entry */
    RemoveEntryList(&pClient->Entry);

    /* release lock */
    KeReleaseSpinLock(&DeviceExtension->Lock, OldIrql);

    /* check if all audio pins have been closed */
    for (Index = 0; Index < pClient->NumPins; Index++)
    {
       DPRINT("Index %u Pin %p Type %x\n", Index, pClient->hPins[Index].Handle, pClient->hPins[Index].Type);
       if (pClient->hPins[Index].Handle && pClient->hPins[Index].Type != MIXER_DEVICE_TYPE)
       {
           /* found an still open audio pin */
           ZwClose(pClient->hPins[Index].Handle);
       }
    }

    /* free pin array */
    if (pClient->hPins)
        FreeItem(pClient->hPins);

    /* free client context struct */
    FreeItem(pClient);

    /* clear old client pointer */
    IoStack->FileObject->FsContext = NULL;

    /* complete request */
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    /* done */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
DriverEntry(
    IN PDRIVER_OBJECT Driver,
    IN PUNICODE_STRING Registry_path
)
{
    DPRINT("Wdmaud.sys loaded\n");

    Driver->DriverUnload = WdmAudUnload;

    Driver->MajorFunction[IRP_MJ_CREATE] = WdmAudCreate;
    Driver->MajorFunction[IRP_MJ_CLOSE] = WdmAudClose;
    Driver->MajorFunction[IRP_MJ_PNP] = WdmAudPnp;
    Driver->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = KsDefaultForwardIrp; 
    Driver->MajorFunction[IRP_MJ_CLEANUP] = WdmAudCleanup;
    Driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = WdmAudDeviceControl;
    Driver->MajorFunction[IRP_MJ_WRITE] = WdmAudReadWrite;
    Driver->MajorFunction[IRP_MJ_READ] = WdmAudReadWrite;
    Driver->MajorFunction[IRP_MJ_POWER] = KsDefaultDispatchPower;
    Driver->DriverExtension->AddDevice = WdmaudAddDevice;

    return STATUS_SUCCESS;
}
