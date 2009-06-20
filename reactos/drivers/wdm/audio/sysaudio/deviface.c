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

#define IOCTL_KS_OBJECT_CLASS CTL_CODE(FILE_DEVICE_KS, 0x7, METHOD_NEITHER, FILE_ANY_ACCESS)

VOID
QueryFilterRoutine(
    IN PKSAUDIO_SUBDEVICE_ENTRY DeviceEntry)
{
    KSPROPERTY PropertyRequest;
    KSP_PIN PinRequest;
    KSPIN_DATAFLOW DataFlow;
    KSPIN_COMMUNICATION Communication;
    KSPIN_CINSTANCES PinInstances;
    ULONG Count, Index;
    NTSTATUS Status;
    ULONG BytesReturned;
    ULONG NumWaveOutPin, NumWaveInPin;

    DPRINT("Querying filter...\n");

    Status = KsSynchronousIoControlDevice(DeviceEntry->FileObject, KernelMode, IOCTL_KS_OBJECT_CLASS, NULL, 0, &DeviceEntry->ObjectClass, sizeof(LPWSTR), &BytesReturned);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to query object class Status %x\n", Status);
        return;
    }

    DPRINT("ObjectClass %S\n", DeviceEntry->ObjectClass);

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
    DeviceEntry->NumberOfPins = Count;

    NumWaveInPin = 0;
    NumWaveOutPin = 0;
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
            NumWaveOutPin++;

        if (Communication == KSPIN_COMMUNICATION_SINK && DataFlow == KSPIN_DATAFLOW_OUT)
            NumWaveInPin++;

    }

    DPRINT("Num Pins %u Num WaveIn Pins %u Name WaveOut Pins %u\n", DeviceEntry->NumberOfPins, NumWaveInPin, NumWaveOutPin);
}




VOID
NTAPI
FilterPinWorkerRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID Context)
{
    PKSAUDIO_DEVICE_ENTRY DeviceEntry;
    PKSAUDIO_SUBDEVICE_ENTRY SubDeviceEntry;
    PLIST_ENTRY ListEntry;

    PFILTER_WORKER_CONTEXT Ctx = (PFILTER_WORKER_CONTEXT)Context;

    DeviceEntry = Ctx->DeviceEntry;

    ListEntry = DeviceEntry->SubDeviceList.Flink;
    while(ListEntry != &DeviceEntry->SubDeviceList)
    {
        SubDeviceEntry = (PKSAUDIO_SUBDEVICE_ENTRY)CONTAINING_RECORD(ListEntry, KSAUDIO_SUBDEVICE_ENTRY, Entry);
        QueryFilterRoutine(SubDeviceEntry);
        ListEntry = ListEntry->Flink;
    }


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
    PKSAUDIO_SUBDEVICE_ENTRY SubDeviceEntry;
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
        DeviceEntry->DeviceName.MaximumLength = Event->SymbolicLinkName->Length + 10 * sizeof(WCHAR);
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

        /* FIXME Ros does not support device interface strings */
        /* Workarround: repeatly call IoCreateFile untill ks wont find a create item which has no object header attached */

        InitializeListHead(&DeviceEntry->SubDeviceList);
        do
        {
            SubDeviceEntry = ExAllocatePool(NonPagedPool, sizeof(KSAUDIO_SUBDEVICE_ENTRY));
            if (SubDeviceEntry)
            {
                RtlZeroMemory(SubDeviceEntry,  sizeof(KSAUDIO_SUBDEVICE_ENTRY));
                Status = OpenDevice(&DeviceEntry->DeviceName, &SubDeviceEntry->Handle, &SubDeviceEntry->FileObject);
                if (NT_SUCCESS(Status))
                {
                    InsertTailList(&DeviceEntry->SubDeviceList, &SubDeviceEntry->Entry);
                    DeviceEntry->NumSubDevices++;
                    /* increment audio device count */
                    InterlockedIncrement((PLONG)&DeviceExtension->NumberOfKsAudioDevices);
                }
                else
                {
                    ExFreePool(SubDeviceEntry);
                    break;
                }
            }
        }while(NT_SUCCESS(Status) && SubDeviceEntry != NULL);

        DPRINT("Successfully opened audio device %u Device %S NumberOfSubDevices %u\n", DeviceExtension->NumberOfKsAudioDevices, DeviceEntry->DeviceName.Buffer, DeviceEntry->NumSubDevices);

        Ctx->DeviceEntry = DeviceEntry;
        Ctx->WorkItem = WorkItem;

        /* fetch device extension */
        DeviceExtension = (PSYSAUDIODEVEXT)DeviceObject->DeviceExtension;
        /* insert new audio device */
        ExInterlockedInsertTailList(&DeviceExtension->KsAudioDeviceList, &DeviceEntry->Entry, &DeviceExtension->Lock);

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

