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
const GUID KS_CATEGORY_TOPOLOGY                = {0xDDA54A40, 0x1E4C, 0x11D1, {0xA0, 0x50, 0x40, 0x57, 0x05, 0xC1, 0x00, 0x00}};
const GUID DMOCATEGORY_ACOUSTIC_ECHO_CANCEL    = {0xBF963D80L, 0xC559, 0x11D0, {0x8A, 0x2B, 0x00, 0xA0, 0xC9, 0x25, 0x5A, 0xC1}};

NTSTATUS
BuildPinDescriptor(
    IN PKSAUDIO_DEVICE_ENTRY DeviceEntry,
    IN ULONG Count)
{
    ULONG Index;
    KSP_PIN PinRequest;
    KSPIN_DATAFLOW DataFlow;
    KSPIN_COMMUNICATION Communication;
    ULONG NumWaveOutPin, NumWaveInPin;
    NTSTATUS Status;
    ULONG BytesReturned;

    NumWaveInPin = 0;
    NumWaveOutPin = 0;
    for(Index = 0; Index < Count; Index++)
    {
        /* retrieve data flow */
        PinRequest.PinId = Index;
        PinRequest.Property.Set = KSPROPSETID_Pin;
        PinRequest.Property.Flags = KSPROPERTY_TYPE_GET;

        /* get dataflow direction */
        PinRequest.Property.Id = KSPROPERTY_PIN_DATAFLOW;
        Status = KsSynchronousIoControlDevice(DeviceEntry->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&PinRequest, sizeof(KSP_PIN), (PVOID)&DataFlow, sizeof(KSPIN_DATAFLOW), &BytesReturned);
        if (NT_SUCCESS(Status))
        {
            DeviceEntry->PinDescriptors[Index].DataFlow = DataFlow;
        }

        /* get irp flow direction */
        PinRequest.Property.Id = KSPROPERTY_PIN_COMMUNICATION;
        Status = KsSynchronousIoControlDevice(DeviceEntry->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&PinRequest, sizeof(KSP_PIN), (PVOID)&Communication, sizeof(KSPIN_COMMUNICATION), &BytesReturned);
        if (NT_SUCCESS(Status))
        {
            DeviceEntry->PinDescriptors[Index].Communication = Communication;
        }

        if (Communication == KSPIN_COMMUNICATION_SINK && DataFlow == KSPIN_DATAFLOW_IN)
            NumWaveOutPin++;

        if (Communication == KSPIN_COMMUNICATION_SINK && DataFlow == KSPIN_DATAFLOW_OUT)
            NumWaveInPin++;

        /* FIXME query for interface, dataformat etc */
    }

    DPRINT("Num Pins %u Num WaveIn Pins %u Name WaveOut Pins %u\n", DeviceEntry->PinDescriptorsCount, NumWaveInPin, NumWaveOutPin);
    return STATUS_SUCCESS;
}

VOID
QueryFilterRoutine(
    IN PKSAUDIO_DEVICE_ENTRY DeviceEntry)
{
    KSPROPERTY PropertyRequest;
    ULONG Count;
    NTSTATUS Status;
    ULONG BytesReturned;

    DPRINT("Querying filter...\n");

    PropertyRequest.Set = KSPROPSETID_Pin;
    PropertyRequest.Flags = KSPROPERTY_TYPE_GET;
    PropertyRequest.Id = KSPROPERTY_PIN_CTYPES;

    /* query for num of pins */
    Status = KsSynchronousIoControlDevice(DeviceEntry->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&PropertyRequest, sizeof(KSPROPERTY), (PVOID)&Count, sizeof(ULONG), &BytesReturned);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to query number of pins Status %x\n", Status);
        return;
    }

    if (!Count)
    {
        DPRINT1("Filter has no pins!\n");
        return;
    }

    /* allocate pin descriptor array */
    DeviceEntry->PinDescriptors = ExAllocatePool(NonPagedPool, Count * sizeof(KSPIN_DESCRIPTOR));
    if (!DeviceEntry->PinDescriptors)
    {
        /* no memory */
        return;
    }

    /* zero array pin descriptor array */
    RtlZeroMemory(DeviceEntry->PinDescriptors, Count * sizeof(KSPIN_DESCRIPTOR));

    /* build the device descriptor */
    Status = BuildPinDescriptor(DeviceEntry, Count);
    if (!NT_SUCCESS(Status))
        return;


    /* allocate pin array */
    DeviceEntry->Pins = ExAllocatePool(NonPagedPool, Count * sizeof(PIN_INFO));
    if (!DeviceEntry->Pins)
    {
        /* no memory */
        DPRINT1("Failed to allocate memory Pins %u Block %x\n", Count, Count * sizeof(PIN_INFO));
        return;
    }

    /* clear array */
    RtlZeroMemory(DeviceEntry->Pins, sizeof(PIN_INFO) * Count);
    DeviceEntry->PinDescriptorsCount = Count;

}

VOID
NTAPI
FilterPinWorkerRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context)
{
    PKSAUDIO_DEVICE_ENTRY DeviceEntry;
    PFILTER_WORKER_CONTEXT Ctx = (PFILTER_WORKER_CONTEXT)Context;

    DeviceEntry = Ctx->DeviceEntry;

    QueryFilterRoutine(DeviceEntry);

    /* free work item */
    IoFreeWorkItem(Ctx->WorkItem);
    /* free work item context */
    ExFreePool(Ctx);
    return;

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
        DPRINT("ZwCreateFile failed with %x %S\n", Status, DeviceName->Buffer);
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
InsertAudioDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PUNICODE_STRING DeviceName)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PFILTER_WORKER_CONTEXT Ctx = NULL;
    PIO_WORKITEM WorkItem = NULL;
    PSYSAUDIODEVEXT DeviceExtension;
    PKSAUDIO_DEVICE_ENTRY DeviceEntry = NULL;
    PDEVICE_OBJECT AudioDeviceObject;
    UNICODE_STRING ReferenceString, SymbolicLinkName;

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
    DeviceEntry->DeviceName.MaximumLength = DeviceName->MaximumLength + 10 * sizeof(WCHAR);

    DeviceEntry->DeviceName.Buffer = ExAllocatePool(NonPagedPool, DeviceEntry->DeviceName.MaximumLength);

    if (!DeviceEntry->DeviceName.Buffer)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    RtlAppendUnicodeToString(&DeviceEntry->DeviceName, L"\\??\\");
    RtlAppendUnicodeStringToString(&DeviceEntry->DeviceName, DeviceName);

    Status = OpenDevice(&DeviceEntry->DeviceName, &DeviceEntry->Handle, &DeviceEntry->FileObject);

     if (!NT_SUCCESS(Status))
     {
         goto cleanup;
     }

    Ctx->DeviceEntry = DeviceEntry;
    Ctx->WorkItem = WorkItem;

    /* HACK
     * sysaudio should register the device object for itself 
     */
    AudioDeviceObject = IoGetRelatedDeviceObject(DeviceEntry->FileObject);
    RtlInitUnicodeString(&ReferenceString, L"sad0");
    Status = IoRegisterDeviceInterface(AudioDeviceObject, &KSCATEGORY_AUDIO_DEVICE, &ReferenceString, &SymbolicLinkName);
    if (NT_SUCCESS(Status))
    {
        IoSetDeviceInterfaceState(&SymbolicLinkName, TRUE);
        RtlFreeUnicodeString(&SymbolicLinkName);
    }
    else
	{
		DPRINT1("Failed to open %wZ with Status %x\n", &DeviceEntry->DeviceName, Status);
	DbgBreakPoint();

	}

    /* fetch device extension */
    DeviceExtension = (PSYSAUDIODEVEXT)DeviceObject->DeviceExtension;
    /* insert new audio device */
    ExInterlockedInsertTailList(&DeviceExtension->KsAudioDeviceList, &DeviceEntry->Entry, &DeviceExtension->Lock);
    InterlockedIncrement((PLONG)&DeviceExtension->NumberOfKsAudioDevices);

    DPRINT("Successfully opened audio device %u Device %S\n", DeviceExtension->NumberOfKsAudioDevices, DeviceEntry->DeviceName.Buffer);
    IoQueueWorkItem(WorkItem, FilterPinWorkerRoutine, DelayedWorkQueue, (PVOID)Ctx);
    return Status;

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
NTAPI
DeviceInterfaceChangeCallback(
    IN PVOID NotificationStructure,
    IN PVOID Context)
{
    DEVICE_INTERFACE_CHANGE_NOTIFICATION * Event;
    NTSTATUS Status = STATUS_SUCCESS;
    PSYSAUDIODEVEXT DeviceExtension;
    PDEVICE_OBJECT DeviceObject = (PDEVICE_OBJECT)Context;

    DeviceExtension = (PSYSAUDIODEVEXT)DeviceObject->DeviceExtension;

    Event = (DEVICE_INTERFACE_CHANGE_NOTIFICATION*)NotificationStructure;

    if (IsEqualGUIDAligned(&Event->Event,
                           &GUID_DEVICE_INTERFACE_ARRIVAL))
    {
        Status = InsertAudioDevice(DeviceObject, Event->SymbolicLinkName);
        return Status;
    }
    else
    {
        DPRINT("Remove interface to audio device!\n");
        UNIMPLEMENTED
        return STATUS_SUCCESS;
    }


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

