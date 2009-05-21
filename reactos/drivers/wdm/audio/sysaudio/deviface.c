/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/sysaudio/deviface.c
 * PURPOSE:         System Audio graph builder
 * PROGRAMMER:      Johannes Anderwald
 */

#include "sysaudio.h"

const GUID GUID_DEVICE_INTERFACE_ARRIVAL       = {0xCB3A4004L, 0x46F0, 0x11D0, {0xB0, 0x8F, 0x00, 0x60, 0x97, 0x13, 0x05, 0x3F}};
const GUID GUID_DEVICE_INTERFACE_REMOVAL       = {0xCB3A4005L, 0x46F0, 0x11D0, {0xB0, 0x8F, 0x00, 0x60, 0x97, 0x13, 0x05, 0x3F}};
const GUID KS_CATEGORY_AUDIO                   = {0x6994AD04L, 0x93EF, 0x11D0, {0xA3, 0xCC, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};
const GUID DMOCATEGORY_ACOUSTIC_ECHO_CANCEL    = {0xBF963D80L, 0xC559, 0x11D0, {0x8A, 0x2B, 0x00, 0xA0, 0xC9, 0x25, 0x5A, 0xC1}};

VOID
NTAPI
FilterPinWorkerRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context)
{
    KSPROPERTY PropertyRequest;
    KSP_PIN PinRequest;
    KSPIN_DATAFLOW DataFlow;
    KSPIN_COMMUNICATION Communication;
    KSPIN_CINSTANCES PinInstances;
    ULONG Count, Index;
    NTSTATUS Status;
    ULONG BytesReturned;
    PSYSAUDIODEVEXT DeviceExtension;
    PKSAUDIO_DEVICE_ENTRY DeviceEntry;
    PFILTER_WORKER_CONTEXT Ctx = (PFILTER_WORKER_CONTEXT)Context;

    DeviceEntry = Ctx->DeviceEntry;


    DPRINT("Querying filter...\n");

    PropertyRequest.Set = KSPROPSETID_Pin;
    PropertyRequest.Flags = KSPROPERTY_TYPE_GET;
    PropertyRequest.Id = KSPROPERTY_PIN_CTYPES;

    /* query for num of pins */
    Status = KsSynchronousIoControlDevice(DeviceEntry->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&PropertyRequest, sizeof(KSPROPERTY), (PVOID)&Count, sizeof(ULONG), &BytesReturned);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to query number of pins Status %x\n", Status);
        goto cleanup;
    }

    if (!Count)
    {
        DPRINT1("Filter has no pins!\n");
        goto cleanup;
    }

    /* allocate pin array */
    DeviceEntry->Pins = ExAllocatePool(NonPagedPool, Count * sizeof(PIN_INFO));
    if (!DeviceEntry->Pins)
    {
        /* no memory */
        DPRINT1("Failed to allocate memory Block %x\n", Count * sizeof(PIN_INFO));
        goto cleanup;
    }
    /* clear array */
    RtlZeroMemory(DeviceEntry->Pins, sizeof(PIN_INFO) * Count);
    DeviceEntry->NumberOfPins = Count;

    for(Index = 0; Index < Count; Index++)
    {
        /* get max instance count */
        PinRequest.PinId = Index;
        PinRequest.Property.Set = KSPROPSETID_Pin;
        PinRequest.Property.Flags = KSPROPERTY_TYPE_GET;
        PinRequest.Property.Id = KSPROPERTY_PIN_CINSTANCES;

        Status = KsSynchronousIoControlDevice(DeviceEntry->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&PinRequest, sizeof(KSP_PIN), (PVOID)&PinInstances, sizeof(KSPIN_CINSTANCES), &BytesReturned);
        if (NT_SUCCESS(Status))
        {
            DeviceEntry->Pins[Index].MaxPinInstanceCount = PinInstances.PossibleCount;
        }

        /* get dataflow direction */
        PinRequest.Property.Id = KSPROPERTY_PIN_DATAFLOW;
        Status = KsSynchronousIoControlDevice(DeviceEntry->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&PinRequest, sizeof(KSP_PIN), (PVOID)&DataFlow, sizeof(KSPIN_DATAFLOW), &BytesReturned);
        if (NT_SUCCESS(Status))
        {
            DeviceEntry->Pins[Index].DataFlow = DataFlow;
        }

        /* get irp flow direction */
        PinRequest.Property.Id = KSPROPERTY_PIN_COMMUNICATION;
        Status = KsSynchronousIoControlDevice(DeviceEntry->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&PinRequest, sizeof(KSP_PIN), (PVOID)&Communication, sizeof(KSPIN_COMMUNICATION), &BytesReturned);
        if (NT_SUCCESS(Status))
        {
            DeviceEntry->Pins[Index].Communication = Communication;
        }

        if (Communication == KSPIN_COMMUNICATION_SINK && DataFlow == KSPIN_DATAFLOW_IN)
            DeviceEntry->NumWaveOutPin++;

        if (Communication == KSPIN_COMMUNICATION_SINK && DataFlow == KSPIN_DATAFLOW_OUT)
            DeviceEntry->NumWaveInPin++;

    }

    DPRINT("Num Pins %u Num WaveIn Pins %u Name WaveOut Pins %u\n", DeviceEntry->NumberOfPins, DeviceEntry->NumWaveInPin, DeviceEntry->NumWaveOutPin);
    /* fetch device extension */
    DeviceExtension = (PSYSAUDIODEVEXT)DeviceObject->DeviceExtension;
    /* insert new audio device */
    ExInterlockedInsertTailList(&DeviceExtension->KsAudioDeviceList, &DeviceEntry->Entry, &DeviceExtension->Lock);
    /* increment audio device count */
    InterlockedIncrement((PLONG)&DeviceExtension->NumberOfKsAudioDevices);

    /* free work item */
    IoFreeWorkItem(Ctx->WorkItem);
    /* free work item context */
    ExFreePool(Ctx);
    return;

cleanup:

    ObDereferenceObject(DeviceEntry->FileObject);
    ZwClose(DeviceEntry->Handle);
    ExFreePool(DeviceEntry->DeviceName.Buffer);
    ExFreePool(DeviceEntry);
    IoFreeWorkItem(Ctx->WorkItem);
    ExFreePool(Ctx);
}

NTSTATUS
OpenDevice(
    IN PUNICODE_STRING DeviceName,
    IN PHANDLE HandleOut,
    IN PFILE_OBJECT * FileObjectOut)
{
    NTSTATUS Status;
    HANDLE NodeHandle;
    PFILE_OBJECT FileObject;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;

    InitializeObjectAttributes(&ObjectAttributes, DeviceName, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);

    Status = ZwCreateFile(&NodeHandle,
                          GENERIC_READ | GENERIC_WRITE,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          0,
                          0,
                          FILE_OPEN,
                          FILE_SYNCHRONOUS_IO_NONALERT,
                          NULL,
                          0);


    if (!NT_SUCCESS(Status))
    {
        DPRINT("ZwCreateFile failed with %x\n", Status);
        return Status;
    }

    Status = ObReferenceObjectByHandle(NodeHandle, GENERIC_READ | GENERIC_WRITE, IoFileObjectType, KernelMode, (PVOID*)&FileObject, NULL);
    if (!NT_SUCCESS(Status))
    {
        ZwClose(NodeHandle);
        DPRINT("ObReferenceObjectByHandle failed with %x\n", Status);
        return Status;
    }

    *HandleOut = NodeHandle;
    *FileObjectOut = FileObject;
    return Status;
}

NTSTATUS
NTAPI
DeviceInterfaceChangeCallback(
    IN PVOID NotificationStructure,
    IN PVOID Context)
{
    DEVICE_INTERFACE_CHANGE_NOTIFICATION * Event;
    NTSTATUS Status = STATUS_SUCCESS;
    PSYSAUDIODEVEXT DeviceExtension;
    PKSAUDIO_DEVICE_ENTRY DeviceEntry = NULL;
    PIO_WORKITEM WorkItem = NULL;
    PFILTER_WORKER_CONTEXT Ctx = NULL;
    PDEVICE_OBJECT DeviceObject = (PDEVICE_OBJECT)Context;

    DeviceExtension = (PSYSAUDIODEVEXT)DeviceObject->DeviceExtension;

    Event = (DEVICE_INTERFACE_CHANGE_NOTIFICATION*)NotificationStructure;

    if (IsEqualGUIDAligned(&Event->Event,
                           &GUID_DEVICE_INTERFACE_ARRIVAL))
    {
        /* a new device has arrived */
        DeviceEntry = ExAllocatePool(NonPagedPool, sizeof(KSAUDIO_DEVICE_ENTRY));
        if (!DeviceEntry)
        {
            /* no memory */
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* initialize audio device entry */
        RtlZeroMemory(DeviceEntry, sizeof(KSAUDIO_DEVICE_ENTRY));

        /* allocate filter ctx */
        Ctx = ExAllocatePool(NonPagedPool, sizeof(FILTER_WORKER_CONTEXT));
        if (!Ctx)
        {
            /* no memory */
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto cleanup;
        }

        /* allocate work item */
        WorkItem = IoAllocateWorkItem(DeviceObject);
        if (!WorkItem)
        {
            /* no memory */
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto cleanup;
        }

        /* set device name */
        DeviceEntry->DeviceName.Length = 0;
        DeviceEntry->DeviceName.MaximumLength = Event->SymbolicLinkName->Length + 5 * sizeof(WCHAR);
        DeviceEntry->DeviceName.Buffer = ExAllocatePool(NonPagedPool, DeviceEntry->DeviceName.MaximumLength);

        if (!DeviceEntry->DeviceName.Buffer)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto cleanup;
        }

        if (!NT_SUCCESS(RtlAppendUnicodeToString(&DeviceEntry->DeviceName, L"\\??\\")))
        {
            DPRINT1("RtlAppendUnicodeToString failed with %x\n", Status);
            goto cleanup;
        }

        if (!NT_SUCCESS(RtlAppendUnicodeStringToString(&DeviceEntry->DeviceName, Event->SymbolicLinkName)))
        {
            DPRINT1("RtlAppendUnicodeStringToString failed with %x\n", Status);
            goto cleanup;
        }

        Status = OpenDevice(&DeviceEntry->DeviceName, &DeviceEntry->Handle, &DeviceEntry->FileObject);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("ZwCreateFile failed with %x\n", Status);
            goto cleanup;
        }

        DPRINT("Successfully opened audio device %u handle %p file object %p device object %p\n", DeviceExtension->NumberOfKsAudioDevices, DeviceEntry->Handle, DeviceEntry->FileObject, DeviceEntry->FileObject->DeviceObject);

        Ctx->DeviceEntry = DeviceEntry;
        Ctx->WorkItem = WorkItem;

        IoQueueWorkItem(WorkItem, FilterPinWorkerRoutine, DelayedWorkQueue, (PVOID)Ctx);
        return Status;
    }
    else
    {
        DPRINT("Remove interface to audio device!\n");
        UNIMPLEMENTED
        return STATUS_SUCCESS;
    }

cleanup:
    if (Ctx)
        ExFreePool(Ctx);

    if (WorkItem)
        IoFreeWorkItem(WorkItem);

    if (DeviceEntry)
    {
        if (DeviceEntry->DeviceName.Buffer)
            ExFreePool(DeviceEntry->DeviceName.Buffer);

        ExFreePool(DeviceEntry);
    }

    return Status;
}

NTSTATUS
SysAudioRegisterNotifications(
    IN PDRIVER_OBJECT  DriverObject,
    IN PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS Status;
    PSYSAUDIODEVEXT DeviceExtension;

    DeviceExtension = (PSYSAUDIODEVEXT)DeviceObject->DeviceExtension;

    Status = IoRegisterPlugPlayNotification(EventCategoryDeviceInterfaceChange,
                                            PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
                                            (PVOID)&KS_CATEGORY_AUDIO,
                                            DriverObject,
                                            DeviceInterfaceChangeCallback,
                                            (PVOID)DeviceObject,
                                            (PVOID*)&DeviceExtension->KsAudioNotificationEntry);

    if (!NT_SUCCESS(Status))
    {
        DPRINT("IoRegisterPlugPlayNotification failed with %x\n", Status);
    }

    Status = IoRegisterPlugPlayNotification(EventCategoryDeviceInterfaceChange,
                                            PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
                                            (PVOID)&DMOCATEGORY_ACOUSTIC_ECHO_CANCEL,
                                            DriverObject,
                                            DeviceInterfaceChangeCallback,
                                            (PVOID)DeviceObject,
                                            (PVOID*)&DeviceExtension->EchoCancelNotificationEntry);

    if (!NT_SUCCESS(Status))
    {
        /* ignore failure for now */
        DPRINT("IoRegisterPlugPlayNotification failed for DMOCATEGORY_ACOUSTIC_ECHO_CANCEL\n", Status);
    }

    return STATUS_SUCCESS;
}



NTSTATUS
SysAudioRegisterDeviceInterfaces(
    IN PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS Status;
    UNICODE_STRING SymbolicLink;

    Status = IoRegisterDeviceInterface(DeviceObject, &KSCATEGORY_PREFERRED_MIDIOUT_DEVICE, NULL, &SymbolicLink);
    if (NT_SUCCESS(Status))
    {
        IoSetDeviceInterfaceState(&SymbolicLink, TRUE);
        RtlFreeUnicodeString(&SymbolicLink);
    }
    else
    {
        DPRINT("Failed to register KSCATEGORY_PREFERRED_MIDIOUT_DEVICE interface Status %x\n", Status);
        return Status;
    }

    Status = IoRegisterDeviceInterface(DeviceObject, &KSCATEGORY_PREFERRED_WAVEIN_DEVICE, NULL, &SymbolicLink);
    if (NT_SUCCESS(Status))
    {
        IoSetDeviceInterfaceState(&SymbolicLink, TRUE);
        RtlFreeUnicodeString(&SymbolicLink);
    }
    else
    {
        DPRINT("Failed to register KSCATEGORY_PREFERRED_WAVEIN_DEVICE interface Status %x\n", Status);
        return Status;
    }

    Status = IoRegisterDeviceInterface(DeviceObject, &KSCATEGORY_PREFERRED_WAVEOUT_DEVICE, NULL, &SymbolicLink);
    if (NT_SUCCESS(Status))
    {
        IoSetDeviceInterfaceState(&SymbolicLink, TRUE);
        RtlFreeUnicodeString(&SymbolicLink);
    }
    else
    {
        DPRINT("Failed to register KSCATEGORY_PREFERRED_WAVEOUT_DEVICE interface Status %x\n", Status);
    }

    Status = IoRegisterDeviceInterface(DeviceObject, &KSCATEGORY_SYSAUDIO, NULL, &SymbolicLink);
    if (NT_SUCCESS(Status))
    {
        IoSetDeviceInterfaceState(&SymbolicLink, TRUE);
        RtlFreeUnicodeString(&SymbolicLink);
    }
    else
    {
        DPRINT("Failed to register KSCATEGORY_SYSAUDIO interface Status %x\n", Status);
    }

    return Status;
}

