/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/sysaudio/control.c
 * PURPOSE:         System Audio graph builder
 * PROGRAMMER:      Johannes Anderwald
 */

#include <ntifs.h>
#include <ntddk.h>
#include <portcls.h>
#include <ks.h>
#include <ksmedia.h>
#include <math.h>
#define YDEBUG
#include <debug.h>
#include "sysaudio.h"

const GUID KSPROPSETID_Sysaudio                 = {0xCBE3FAA0L, 0xCC75, 0x11D0, {0xB4, 0x65, 0x00, 0x00, 0x1A, 0x18, 0x18, 0xE6}};
const GUID KSPROPSETID_Sysaudio_Pin             = {0xA3A53220L, 0xC6E4, 0x11D0, {0xB4, 0x65, 0x00, 0x00, 0x1A, 0x18, 0x18, 0xE6}};
const GUID KSPROPSETID_General                  = {0x1464EDA5L, 0x6A8F, 0x11D1, {0x9A, 0xA7, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};
const GUID KSPROPSETID_Pin                     = {0x8C134960L, 0x51AD, 0x11CF, {0x87, 0x8A, 0x94, 0xF8, 0x01, 0xC1, 0x00, 0x00}};
const GUID KSPROPSETID_Connection              = {0x1D58C920L, 0xAC9B, 0x11CF, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};
const GUID KSDATAFORMAT_TYPE_AUDIO              = {0x73647561L, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
const GUID KSDATAFORMAT_SUBTYPE_PCM             = {0x00000001L, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
const GUID KSDATAFORMAT_SPECIFIER_WAVEFORMATEX  = {0x05589f81L, 0xc356, 0x11ce, {0xbf, 0x01, 0x00, 0xaa, 0x00, 0x55, 0x59, 0x5a}};

NTSTATUS
SetIrpIoStatus(
    IN PIRP Irp,
    IN NTSTATUS Status,
    IN ULONG Length)
{
    Irp->IoStatus.Information = Length;
    Irp->IoStatus.Status = Status;
    if (Status != STATUS_PENDING)
    {
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
    else
    {
        IoMarkIrpPending(Irp);
    }
    return Status;

}

PKSAUDIO_DEVICE_ENTRY
GetListEntry(
    IN PLIST_ENTRY Head,
    IN ULONG Index)
{
    PLIST_ENTRY Entry = Head->Flink;

    while(Index-- && Entry != Head)
        Entry = Entry->Flink;

    if (Entry == Head)
        return NULL;

    return (PKSAUDIO_DEVICE_ENTRY)CONTAINING_RECORD(Entry, KSAUDIO_DEVICE_ENTRY, Entry);
}

NTSTATUS
SysAudioOpenVirtualDevice(
    IN PIRP Irp,
    IN ULONG DeviceNumber,
    PSYSAUDIODEVEXT DeviceExtension)
{
    PSYSAUDIO_CLIENT ClientInfo;
    PKSAUDIO_DEVICE_ENTRY Entry;
    PKSOBJECT_CREATE_ITEM CreateItem;

    /* access the create item */
    CreateItem = KSCREATE_ITEM_IRP_STORAGE(Irp);
    ASSERT(CreateItem);

    if (DeviceNumber >= DeviceExtension->NumberOfKsAudioDevices)
    {
        /* invalid device index */
        return SetIrpIoStatus(Irp, STATUS_INVALID_PARAMETER, 0);
    }

    /* get client context */
    ClientInfo = (PSYSAUDIO_CLIENT)CreateItem->Context;

    /* sanity check */
    ASSERT(ClientInfo);

    /* check for valid device index */
    if (DeviceNumber >= ClientInfo->NumDevices)
    {
        /* invalid device index */
        return SetIrpIoStatus(Irp, STATUS_INVALID_PARAMETER, 0);
    }

    /* get device context */
    Entry = GetListEntry(&DeviceExtension->KsAudioDeviceList, DeviceNumber);
    ASSERT(Entry != NULL);

    /* increase usage count */
    Entry->NumberOfClients++;

    return SetIrpIoStatus(Irp, STATUS_SUCCESS, 0);
}

NTSTATUS
SetMixerInputOutputFormat(
    IN PFILE_OBJECT FileObject,
    IN PKSDATAFORMAT InputFormat,
    IN PKSDATAFORMAT OutputFormat)
{
    KSP_PIN PinRequest;
    ULONG BytesReturned;
    NTSTATUS Status;

    /* re-using pin */
    PinRequest.Property.Set = KSPROPSETID_Connection;
    PinRequest.Property.Flags = KSPROPERTY_TYPE_SET;
    PinRequest.Property.Id = KSPROPERTY_CONNECTION_DATAFORMAT;

    /* set the input format */
    PinRequest.PinId = 0;
    //DPRINT1("InputFormat %p Size %u WaveFormatSize %u DataFormat %u WaveEx %u\n", InputFormat, InputFormat->FormatSize, sizeof(KSDATAFORMAT_WAVEFORMATEX), sizeof(KSDATAFORMAT), sizeof(WAVEFORMATEX));
    Status = KsSynchronousIoControlDevice(FileObject, KernelMode, IOCTL_KS_PROPERTY,
                                          (PVOID)&PinRequest,
                                           sizeof(KSP_PIN),
                                          (PVOID)InputFormat,
                                           InputFormat->FormatSize,
                                          &BytesReturned);
    if (!NT_SUCCESS(Status))
        return Status;

    /* set the the output format */
    PinRequest.PinId = 1;
    //DPRINT1("OutputFormat %p Size %u WaveFormatSize %u DataFormat %u WaveEx %u\n", OutputFormat, OutputFormat->FormatSize, sizeof(KSDATAFORMAT_WAVEFORMATEX), sizeof(KSDATAFORMAT), sizeof(WAVEFORMATEX));
    Status = KsSynchronousIoControlDevice(FileObject, KernelMode, IOCTL_KS_PROPERTY,
                                          (PVOID)&PinRequest,
                                           sizeof(KSP_PIN),
                                          (PVOID)OutputFormat,
                                           OutputFormat->FormatSize,
                                          &BytesReturned);
    return Status;
}


NTSTATUS
CreateMixerPinAndSetFormat(
    IN HANDLE KMixerHandle,
    IN KSPIN_CONNECT *PinConnect,
    IN PKSDATAFORMAT InputFormat,
    IN PKSDATAFORMAT OutputFormat,
    OUT PHANDLE MixerPinHandle,
    OUT PFILE_OBJECT *MixerFileObject)
{
    NTSTATUS Status;
    HANDLE PinHandle;
    PFILE_OBJECT FileObject;

    Status = KsCreatePin(KMixerHandle, PinConnect, GENERIC_READ | GENERIC_WRITE, &PinHandle);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create Mixer Pin with %x\n", Status);
        return STATUS_UNSUCCESSFUL;
    }

    Status = ObReferenceObjectByHandle(PinHandle,
                                       GENERIC_READ | GENERIC_WRITE, 
                                       IoFileObjectType, KernelMode, (PVOID*)&FileObject, NULL);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to get file object with %x\n", Status);
        return STATUS_UNSUCCESSFUL;
    }

    Status = SetMixerInputOutputFormat(FileObject, InputFormat, OutputFormat);
    if (!NT_SUCCESS(Status))
    {
        ObDereferenceObject(FileObject);
        ZwClose(PinHandle);
    }

    *MixerPinHandle = PinHandle;
    *MixerFileObject = FileObject;
     return Status;
}


VOID
NTAPI
CreatePinWorkerRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID  Context)
{
    NTSTATUS Status;
    PSYSAUDIO_CLIENT AudioClient;
    HANDLE RealPinHandle, VirtualPinHandle;
    HANDLE Filter;
    ULONG NumHandels;
    PFILE_OBJECT FileObject;
    PSYSAUDIO_PIN_HANDLE ClientPinHandle;
    PPIN_WORKER_CONTEXT WorkerContext = (PPIN_WORKER_CONTEXT)Context;
    PKSPIN_CONNECT PinConnect = NULL;
    PKSDATAFORMAT ClientFormat;
    KSP_PIN PinRequest;
    ULONG BytesReturned;

    Filter = WorkerContext->PinConnect->PinToHandle;

    WorkerContext->PinConnect->PinToHandle = NULL;

    DPRINT("CreatePinWorkerRoutine entered\n");

    ASSERT(WorkerContext->Entry);
    ASSERT(WorkerContext->PinConnect);
    ASSERT(WorkerContext->Entry->Pins);
    ASSERT(WorkerContext->Entry->NumberOfPins > WorkerContext->PinConnect->PinId);

    PinConnect = WorkerContext->PinConnect;
    ClientFormat = (PKSDATAFORMAT)(PinConnect + 1);

    if (WorkerContext->CreateMixerPin)
    {
        PinConnect = ExAllocatePool(NonPagedPool, sizeof(KSPIN_CONNECT) + WorkerContext->MixerFormat->DataFormat.FormatSize);
        if (!PinConnect)
        {
            /* no memory */
            SetIrpIoStatus(WorkerContext->Irp, STATUS_NO_MEMORY, 0);
            ExFreePool(WorkerContext->DispatchContext);
            ExFreePool(WorkerContext->MixerFormat);
            ExFreePool(WorkerContext);
            return;
        }

        RtlMoveMemory(PinConnect, WorkerContext->PinConnect, sizeof(KSPIN_CONNECT));
        RtlMoveMemory((PinConnect + 1), WorkerContext->MixerFormat, WorkerContext->MixerFormat->DataFormat.FormatSize);

        Status = CreateMixerPinAndSetFormat(WorkerContext->DeviceExtension->KMixerHandle, 
                                            PinConnect,
                                            (PKSDATAFORMAT)(WorkerContext->PinConnect + 1),
                                            (PKSDATAFORMAT)WorkerContext->MixerFormat,
                                            &WorkerContext->DispatchContext->hMixerPin,
                                            &WorkerContext->DispatchContext->MixerFileObject);

        ExFreePool(WorkerContext->MixerFormat);

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to create Mixer Pin with %x\n", Status);
            SetIrpIoStatus(WorkerContext->Irp, STATUS_UNSUCCESSFUL, 0);
            ExFreePool(WorkerContext->DispatchContext);
            ExFreePool(WorkerContext);
            return;
        }
    }
    else
    {
        Status = CreateMixerPinAndSetFormat(WorkerContext->DeviceExtension->KMixerHandle, 
                                            WorkerContext->PinConnect,
                                            (PKSDATAFORMAT)(WorkerContext->PinConnect + 1),
                                            (PKSDATAFORMAT)(WorkerContext->PinConnect + 1),
                                            &WorkerContext->DispatchContext->hMixerPin,
                                            &WorkerContext->DispatchContext->MixerFileObject);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to create Mixer Pin with %x\n", Status);
            SetIrpIoStatus(WorkerContext->Irp, STATUS_UNSUCCESSFUL, 0);
            ExFreePool(WorkerContext->DispatchContext);
            ExFreePool(WorkerContext);
            return;
        }
    }

    if (WorkerContext->CreateRealPin)
    {
        /* create the real pin */
        DPRINT("Creating real pin\n");

        if (WorkerContext->CreateMixerPin)
            Status = KsCreatePin(WorkerContext->Entry->Handle, PinConnect, GENERIC_READ | GENERIC_WRITE, &RealPinHandle);
        else
            Status = KsCreatePin(WorkerContext->Entry->Handle, WorkerContext->PinConnect, GENERIC_READ | GENERIC_WRITE, &RealPinHandle);

        DPRINT1("Status %x\n", Status);
        if (!NT_SUCCESS(Status))
        {
            PKSDATAFORMAT_WAVEFORMATEX RequestedFormat = (PKSDATAFORMAT_WAVEFORMATEX)(WorkerContext->PinConnect + 1);
            DPRINT1("Failed to create Pin with %x\nNumChannels: %u BitsPerSample %u SampleRate %u\n", Status,
            RequestedFormat->WaveFormatEx.nChannels, RequestedFormat->WaveFormatEx.wBitsPerSample, RequestedFormat->WaveFormatEx.nSamplesPerSec);

            if (WorkerContext->CreateMixerPin)
            {
                /* The mixer pin format should have been accepted */
                ObDereferenceObject(WorkerContext->DispatchContext->MixerFileObject);
                ZwClose(WorkerContext->DispatchContext->hMixerPin);
                ASSERT(0);
            }

            SetIrpIoStatus(WorkerContext->Irp, STATUS_UNSUCCESSFUL, 0);
            ExFreePool(WorkerContext->DispatchContext);
            ExFreePool(WorkerContext);
            return;
        }

        /* get pin file object */
        Status = ObReferenceObjectByHandle(RealPinHandle,
                                           GENERIC_READ | GENERIC_WRITE, 
                                           IoFileObjectType, KernelMode, (PVOID*)&FileObject, NULL);

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to get file object with %x\n", Status);
            SetIrpIoStatus(WorkerContext->Irp, STATUS_UNSUCCESSFUL, 0);
            ExFreePool(WorkerContext->DispatchContext);
            ExFreePool(WorkerContext);
            return;
        }

        if (WorkerContext->Entry->Pins[WorkerContext->PinConnect->PinId].MaxPinInstanceCount == 1)
        {
            /* store the pin handle there if the pin can only be instantiated once*/
            WorkerContext->Entry->Pins[WorkerContext->PinConnect->PinId].PinHandle = RealPinHandle;
        }

        WorkerContext->Entry->Pins[WorkerContext->PinConnect->PinId].References = 0;
        WorkerContext->DispatchContext->Handle = RealPinHandle;
        WorkerContext->DispatchContext->PinId = WorkerContext->PinConnect->PinId;
        WorkerContext->DispatchContext->AudioEntry = WorkerContext->Entry;
        WorkerContext->DispatchContext->FileObject = FileObject;
    }
    else
    {
        WorkerContext->DispatchContext->AudioEntry = WorkerContext->Entry;
        WorkerContext->DispatchContext->Handle = WorkerContext->Entry->Pins[WorkerContext->PinConnect->PinId].PinHandle;
        WorkerContext->DispatchContext->PinId = WorkerContext->PinConnect->PinId;

        /* get pin file object */
        Status = ObReferenceObjectByHandle(WorkerContext->Entry->Pins[WorkerContext->PinConnect->PinId].PinHandle,
                                           GENERIC_READ | GENERIC_WRITE, 
                                           IoFileObjectType, KernelMode, (PVOID*)&FileObject, NULL);

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to get file object with %x %p\n", Status, WorkerContext->Entry->Pins[WorkerContext->PinConnect->PinId].PinHandle);
            SetIrpIoStatus(WorkerContext->Irp, STATUS_UNSUCCESSFUL, 0);
            ExFreePool(WorkerContext->DispatchContext);
            ExFreePool(WorkerContext);
            return;
        }
        WorkerContext->DispatchContext->FileObject = FileObject;


        /* re-using pin */
        PinRequest.Property.Set = KSPROPSETID_Connection;
        PinRequest.Property.Flags = KSPROPERTY_TYPE_SET;
        PinRequest.Property.Id = KSPROPERTY_CONNECTION_DATAFORMAT;
        PinRequest.PinId = PinConnect->PinId;

        if (WorkerContext->MixerFormat)
            ClientFormat = (PKSDATAFORMAT)WorkerContext->MixerFormat;
        else
            ClientFormat = (PKSDATAFORMAT)(WorkerContext->PinConnect + 1);

        /* set the format on the real pin */
        Status = KsSynchronousIoControlDevice(FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&PinRequest, sizeof(KSPROPERTY),
                                             (PVOID)ClientFormat, ClientFormat->FormatSize, &BytesReturned);

    }

    DPRINT("creating virtual pin\n");
    /* now create the virtual audio pin which is exposed to wdmaud */
    Status = KsCreatePin(Filter, WorkerContext->PinConnect, GENERIC_READ | GENERIC_WRITE, &VirtualPinHandle);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create virtual pin with %x\n", Status);
        if (WorkerContext->CreateRealPin)
        {
            /* mark pin as free to use */
            WorkerContext->Entry->Pins[WorkerContext->PinConnect->PinId].References = 0;
        }

        SetIrpIoStatus(WorkerContext->Irp, STATUS_UNSUCCESSFUL, 0);
        ExFreePool(WorkerContext->DispatchContext);
        ExFreePool(WorkerContext);
        return;
    }

   /* get pin file object */
    Status = ObReferenceObjectByHandle(VirtualPinHandle,
                                      GENERIC_READ | GENERIC_WRITE, 
                                      IoFileObjectType, KernelMode, (PVOID*)&FileObject, NULL);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to get file object with %x\n", Status);

        ZwClose(VirtualPinHandle);
        SetIrpIoStatus(WorkerContext->Irp, STATUS_UNSUCCESSFUL, 0);
        ExFreePool(WorkerContext->DispatchContext);
        ExFreePool(WorkerContext);
        return;
    }

    ASSERT(WorkerContext->DispatchContext);
    ASSERT(WorkerContext->DispatchContext->AudioEntry != NULL);
    ASSERT(WorkerContext->DispatchContext->FileObject != NULL);
    ASSERT(WorkerContext->DispatchContext->Handle != NULL);
    ASSERT(WorkerContext->AudioClient);
    ASSERT(WorkerContext->AudioClient->NumDevices > 0);
    ASSERT(WorkerContext->AudioClient->Devs != NULL);
    ASSERT(WorkerContext->Entry->Pins != NULL);

    AudioClient = WorkerContext->AudioClient;
    NumHandels = AudioClient->Devs[AudioClient->NumDevices -1].ClientHandlesCount;

    ClientPinHandle = ExAllocatePool(NonPagedPool, sizeof(SYSAUDIO_PIN_HANDLE) * (NumHandels+1));
    if (ClientPinHandle)
    {
        if (NumHandels)
        {
            ASSERT(AudioClient->Devs[AudioClient->NumDevices -1].ClientHandles != NULL);
            RtlMoveMemory(ClientPinHandle,
                          AudioClient->Devs[AudioClient->NumDevices -1].ClientHandles,
                          sizeof(SYSAUDIO_PIN_HANDLE) * NumHandels);
            ExFreePool(AudioClient->Devs[AudioClient->NumDevices -1].ClientHandles);
        }

        AudioClient->Devs[AudioClient->NumDevices -1].ClientHandles = ClientPinHandle;

        /// if the pin can be instantiated more than once
        /// then store the real pin handle in the client context
        /// otherwise just the pin id of the available pin
        if (WorkerContext->Entry->Pins[WorkerContext->PinConnect->PinId].MaxPinInstanceCount > 1)
        {
            AudioClient->Devs[AudioClient->NumDevices -1].ClientHandles[NumHandels].bHandle = TRUE;
            AudioClient->Devs[AudioClient->NumDevices -1].ClientHandles[NumHandels].hPin = RealPinHandle;
            AudioClient->Devs[AudioClient->NumDevices -1].ClientHandles[NumHandels].PinId = WorkerContext->PinConnect->PinId;
            AudioClient->Devs[AudioClient->NumDevices -1].ClientHandles[NumHandels].hMixer = WorkerContext->DispatchContext->hMixerPin;
            AudioClient->Devs[AudioClient->NumDevices -1].ClientHandles[NumHandels].DispatchContext = WorkerContext->DispatchContext;
        }
        else
        {
            AudioClient->Devs[AudioClient->NumDevices -1].ClientHandles[NumHandels].bHandle = FALSE;
            AudioClient->Devs[AudioClient->NumDevices -1].ClientHandles[NumHandels].hPin = VirtualPinHandle;
            AudioClient->Devs[AudioClient->NumDevices -1].ClientHandles[NumHandels].PinId = WorkerContext->PinConnect->PinId;
            AudioClient->Devs[AudioClient->NumDevices -1].ClientHandles[NumHandels].hMixer = WorkerContext->DispatchContext->hMixerPin;
            AudioClient->Devs[AudioClient->NumDevices -1].ClientHandles[NumHandels].DispatchContext = WorkerContext->DispatchContext;
        }

        /// increase reference count
        AudioClient->Devs[AudioClient->NumDevices -1].ClientHandlesCount++;
        WorkerContext->Entry->Pins[WorkerContext->PinConnect->PinId].References++;
    }


    /* store pin context */
    FileObject->FsContext2 = (PVOID)WorkerContext->DispatchContext;

    DPRINT("Successfully created virtual pin %p\n", VirtualPinHandle);
    *((PHANDLE)WorkerContext->Irp->UserBuffer) = VirtualPinHandle;

    SetIrpIoStatus(WorkerContext->Irp, STATUS_SUCCESS, sizeof(HANDLE));
    ExFreePool(WorkerContext);
}

NTSTATUS
HandleSysAudioFilterPinProperties(
    PIRP Irp,
    PKSPROPERTY Property,
    PSYSAUDIODEVEXT DeviceExtension)
{
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;
    PKSAUDIO_DEVICE_ENTRY Entry;
    ULONG BytesReturned;
    PKSP_PIN Pin;

    // in order to access pin properties of a sysaudio device
    // the caller must provide a KSP_PIN struct, where
    // Reserved member points to virtual device index

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    if (IoStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(KSP_PIN))
    {
        /* too small buffer */
        return SetIrpIoStatus(Irp, STATUS_BUFFER_TOO_SMALL, sizeof(KSPROPERTY) + sizeof(ULONG));
    }

    Pin = (PKSP_PIN)Property;

    Entry = GetListEntry(&DeviceExtension->KsAudioDeviceList, ((KSP_PIN*)Property)->Reserved);
    if (!Entry)
    {
        /* invalid device index */
        return SetIrpIoStatus(Irp, STATUS_INVALID_PARAMETER, 0);
    }

    if (!Entry->Pins)
    {
        /* expected pins */
        return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, 0);
    }

    if (Entry->NumberOfPins <= Pin->PinId)
    {
        /* invalid pin id */
        return SetIrpIoStatus(Irp, STATUS_INVALID_PARAMETER, 0);
    }

    if (Property->Id == KSPROPERTY_PIN_CTYPES)
    {
        if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG))
        {
            /* too small buffer */
            return SetIrpIoStatus(Irp, STATUS_BUFFER_TOO_SMALL, sizeof(ULONG));
        }
        /* store result */
        *((PULONG)Irp->UserBuffer) = Entry->NumberOfPins;
        return SetIrpIoStatus(Irp, STATUS_SUCCESS, sizeof(ULONG));
    }
    else if (Property->Id == KSPROPERTY_PIN_COMMUNICATION)
    {
        if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(KSPIN_COMMUNICATION))
        {
            /* too small buffer */
            return SetIrpIoStatus(Irp, STATUS_BUFFER_TOO_SMALL, sizeof(KSPIN_COMMUNICATION));
        }
        /* store result */
        *((KSPIN_COMMUNICATION*)Irp->UserBuffer) = Entry->Pins[Pin->PinId].Communication;
        return SetIrpIoStatus(Irp, STATUS_SUCCESS, sizeof(KSPIN_COMMUNICATION));

    }
    else if (Property->Id == KSPROPERTY_PIN_DATAFLOW)
    {
        if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(KSPIN_DATAFLOW))
        {
            /* too small buffer */
            return SetIrpIoStatus(Irp, STATUS_BUFFER_TOO_SMALL, sizeof(KSPIN_DATAFLOW));
        }
        /* store result */
        *((KSPIN_DATAFLOW*)Irp->UserBuffer) = Entry->Pins[Pin->PinId].DataFlow;
        return SetIrpIoStatus(Irp, STATUS_SUCCESS, sizeof(KSPIN_DATAFLOW));
    }
    else
    {
        /* forward request to the filter implementing the property */
        Status = KsSynchronousIoControlDevice(Entry->FileObject, KernelMode, IOCTL_KS_PROPERTY,
                                             (PVOID)IoStack->Parameters.DeviceIoControl.Type3InputBuffer,
                                             IoStack->Parameters.DeviceIoControl.InputBufferLength,
                                             Irp->UserBuffer,
                                             IoStack->Parameters.DeviceIoControl.OutputBufferLength,
                                             &BytesReturned);

        return SetIrpIoStatus(Irp, Status, BytesReturned);
    }
}

BOOL
ComputeCompatibleFormat(
    IN PKSAUDIO_DEVICE_ENTRY Entry,
    IN ULONG PinId,
    IN PSYSAUDIODEVEXT DeviceExtension,
    IN PKSDATAFORMAT_WAVEFORMATEX ClientFormat,
    OUT PKSDATAFORMAT_WAVEFORMATEX MixerFormat)
{
    BOOL bFound;
    ULONG BytesReturned;
    KSP_PIN PinRequest;
    PKSDATARANGE_AUDIO AudioRange;
    NTSTATUS Status;
    ULONG Index;
    PKSMULTIPLE_ITEM MultipleItem;

    if (!DeviceExtension->KMixerHandle || !DeviceExtension->KMixerFileObject)
    {
        DPRINT1("KMixer is not available\n");
        return STATUS_UNSUCCESSFUL;
    }

    PinRequest.PinId = PinId;
    PinRequest.Property.Set = KSPROPSETID_Pin;
    PinRequest.Property.Flags = KSPROPERTY_TYPE_GET;
    PinRequest.Property.Id = KSPROPERTY_PIN_DATARANGES;

    Status = KsSynchronousIoControlDevice(Entry->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&PinRequest, sizeof(KSP_PIN), NULL, 0, &BytesReturned);
    if (Status != STATUS_BUFFER_TOO_SMALL)
    {
        DPRINT1("Property Request KSPROPERTY_PIN_DATARANGES failed with %x\n", Status);
        return STATUS_UNSUCCESSFUL;
    }

    MultipleItem = ExAllocatePool(NonPagedPool, BytesReturned);
    if (!MultipleItem)
    {
        return STATUS_NO_MEMORY;
    }

    Status = KsSynchronousIoControlDevice(Entry->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&PinRequest, sizeof(KSP_PIN), (PVOID)MultipleItem, BytesReturned, &BytesReturned);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Property Request KSPROPERTY_PIN_DATARANGES failed with %x\n", Status);
        ExFreePool(MultipleItem);
        return STATUS_UNSUCCESSFUL;
    }

    AudioRange = (PKSDATARANGE_AUDIO)(MultipleItem + 1);
    bFound = FALSE;
    for(Index = 0; Index < MultipleItem->Count; Index++)
    {
        if (AudioRange->DataRange.FormatSize != sizeof(KSDATARANGE_AUDIO))
        {
            UNIMPLEMENTED
            AudioRange = (PKSDATARANGE_AUDIO)((PUCHAR)AudioRange + AudioRange->DataRange.FormatSize);
        }
        MixerFormat->DataFormat.FormatSize = sizeof(KSDATAFORMAT) + sizeof(WAVEFORMATEX);
        MixerFormat->DataFormat.Flags = 0;
        MixerFormat->DataFormat.Reserved = 0;
        MixerFormat->DataFormat.MajorFormat = KSDATAFORMAT_TYPE_AUDIO;
        MixerFormat->DataFormat.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
        MixerFormat->DataFormat.Specifier = KSDATAFORMAT_SPECIFIER_WAVEFORMATEX;
        MixerFormat->DataFormat.SampleSize = 4;
        MixerFormat->WaveFormatEx.wFormatTag = ClientFormat->WaveFormatEx.wFormatTag;
        MixerFormat->WaveFormatEx.nChannels = min(AudioRange->MaximumChannels, ClientFormat->WaveFormatEx.nChannels);
        MixerFormat->WaveFormatEx.nSamplesPerSec = max(AudioRange->MinimumSampleFrequency, min(AudioRange->MaximumSampleFrequency, ClientFormat->WaveFormatEx.nSamplesPerSec));
        MixerFormat->WaveFormatEx.wBitsPerSample = max(AudioRange->MinimumBitsPerSample, min(AudioRange->MaximumBitsPerSample, ClientFormat->WaveFormatEx.wBitsPerSample));
        MixerFormat->WaveFormatEx.cbSize = 0;
        MixerFormat->WaveFormatEx.nBlockAlign = (MixerFormat->WaveFormatEx.nChannels * MixerFormat->WaveFormatEx.wBitsPerSample) / 8;
        MixerFormat->WaveFormatEx.nAvgBytesPerSec = MixerFormat->WaveFormatEx.nChannels * MixerFormat->WaveFormatEx.nSamplesPerSec * (MixerFormat->WaveFormatEx.wBitsPerSample / 8);

        bFound = TRUE;
        break;

        AudioRange = (PKSDATARANGE_AUDIO)((PUCHAR)AudioRange + AudioRange->DataRange.FormatSize);
    }
    ExFreePool(MultipleItem);


#if 0
    DPRINT1("\nNum Channels %u Old Channels %u\n SampleRate %u Old SampleRate %u\n BitsPerSample %u Old BitsPerSample %u\n ClientFormat %p",
           MixerFormat->WaveFormatEx.nChannels, ClientFormat->WaveFormatEx.nChannels,
           MixerFormat->WaveFormatEx.nSamplesPerSec, ClientFormat->WaveFormatEx.nSamplesPerSec,
           MixerFormat->WaveFormatEx.wBitsPerSample, ClientFormat->WaveFormatEx.wBitsPerSample, ClientFormat);
#endif

    if (bFound)
        return STATUS_SUCCESS;
    else
        return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
HandleSysAudioFilterPinCreation(
    PIRP Irp,
    PKSPROPERTY Property,
    PSYSAUDIODEVEXT DeviceExtension,
    PDEVICE_OBJECT DeviceObject)
{
    ULONG Length, BytesReturned;
    PKSAUDIO_DEVICE_ENTRY Entry;
    KSPIN_CONNECT * PinConnect;
    PIO_STACK_LOCATION IoStack;
    PSYSAUDIO_INSTANCE_INFO InstanceInfo;
    PSYSAUDIO_CLIENT ClientInfo;
    PKSOBJECT_CREATE_ITEM CreateItem;
    KSP_PIN PinRequest;
    NTSTATUS Status;
    KSPIN_CINSTANCES PinInstances;
    PIO_WORKITEM WorkItem;
    PPIN_WORKER_CONTEXT WorkerContext;
    PDISPATCH_CONTEXT DispatchContext;
    ULONG Index, SubIndex;
    BOOL CreateMixerPin;
    ULONG DeviceId;
    PKSDATAFORMAT_WAVEFORMATEX MixerFormat = NULL, ClientFormat;


    IoStack = IoGetCurrentIrpStackLocation(Irp);

    Length = sizeof(KSDATAFORMAT) + sizeof(KSPIN_CONNECT) + sizeof(SYSAUDIO_INSTANCE_INFO);
    if (IoStack->Parameters.DeviceIoControl.InputBufferLength < Length ||
        IoStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(HANDLE))
    {
        /* invalid parameters */
        return SetIrpIoStatus(Irp, STATUS_INVALID_PARAMETER, 0);
    }

    /* access the create item */
    CreateItem = KSCREATE_ITEM_IRP_STORAGE(Irp);

    /* get input parameter */
    InstanceInfo = (PSYSAUDIO_INSTANCE_INFO)Property;
    if (DeviceExtension->NumberOfKsAudioDevices <= InstanceInfo->DeviceNumber)
    {
        /* invalid parameters */
        return SetIrpIoStatus(Irp, STATUS_INVALID_PARAMETER, 0);
    }

    /* get client context */
    ClientInfo = (PSYSAUDIO_CLIENT)CreateItem->Context;
    if (!ClientInfo || !ClientInfo->NumDevices || !ClientInfo->Devs)
    {
        /* we have a problem */
        KeBugCheckEx(0, 0, 0, 0, 0);
    }

    /* get sysaudio entry */
    Entry = GetListEntry(&DeviceExtension->KsAudioDeviceList, InstanceInfo->DeviceNumber);
    if (!Entry)
    {
        /* invalid device index */
        return SetIrpIoStatus(Irp, STATUS_INVALID_PARAMETER, 0);
    }

    if (!Entry->Pins)
    {
        /* should not happen */
        return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, 0);
    }

    /* get connect details */
    PinConnect = (KSPIN_CONNECT*)(InstanceInfo + 1);

    if (Entry->NumberOfPins <= PinConnect->PinId)
    {
        DPRINT("Invalid PinId %x\n", PinConnect->PinId);
        return SetIrpIoStatus(Irp, STATUS_INVALID_PARAMETER, 0);
    }

Retry:
    /* get the instances count */
    PinRequest.PinId = PinConnect->PinId;
    PinRequest.Property.Set = KSPROPSETID_Pin;
    PinRequest.Property.Flags = KSPROPERTY_TYPE_GET;
    PinRequest.Property.Id = KSPROPERTY_PIN_CINSTANCES;

    Status = KsSynchronousIoControlDevice(Entry->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&PinRequest, sizeof(KSP_PIN), (PVOID)&PinInstances, sizeof(KSPIN_CINSTANCES), &BytesReturned);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Property Request KSPROPERTY_PIN_GLOBALCINSTANCES failed with %x\n", Status);
        return SetIrpIoStatus(Irp, Status, 0);
    }

    if (PinInstances.PossibleCount == 0)
    {
        /* caller wanted to open an instance-less pin */
        return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, 0);
    }


    /* check the format */
    PinRequest.PinId = PinConnect->PinId;
    PinRequest.Property.Set = KSPROPSETID_Pin;
    PinRequest.Property.Flags = KSPROPERTY_TYPE_SET;
    PinRequest.Property.Id = KSPROPERTY_PIN_PROPOSEDATAFORMAT;

    CreateMixerPin = FALSE;
    BytesReturned = IoStack->Parameters.DeviceIoControl.InputBufferLength - sizeof(KSPIN_CONNECT) - sizeof(SYSAUDIO_INSTANCE_INFO);
    ClientFormat = (PKSDATAFORMAT_WAVEFORMATEX)(PinConnect + 1);

    Status = KsSynchronousIoControlDevice(Entry->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&PinRequest, sizeof(KSP_PIN), (PVOID)ClientFormat, BytesReturned, &BytesReturned);
    if (!NT_SUCCESS(Status))
    {
        //DPRINT("Property Request KSPROPERTY_PIN_PROPOSEDATAFORMAT failed with %x\n", Status);

        if (!DeviceExtension->KMixerHandle || !DeviceExtension->KMixerFileObject)
        {
            DPRINT1("KMixer is not available\n");
            DbgBreakPoint();
            return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, 0);
        }

        MixerFormat = ExAllocatePool(NonPagedPool, sizeof(KSDATAFORMAT_WAVEFORMATEX));
        if (!MixerFormat)
        {
            return SetIrpIoStatus(Irp, STATUS_NO_MEMORY, 0);
        }

        Status = ComputeCompatibleFormat(Entry, PinConnect->PinId, DeviceExtension, ClientFormat, MixerFormat);
        if (!NT_SUCCESS(Status))
        {
            ExFreePool(MixerFormat);
            return SetIrpIoStatus(Irp, Status, 0);
        }
        CreateMixerPin = TRUE;
    }

    DeviceId = (ULONG)-1;

    /* scan the clientinfo if the client has already opened device with the specified pin */
    for (Index = 0; Index < ClientInfo->NumDevices; Index++)
    {
        if (ClientInfo->Devs[Index].DeviceId == InstanceInfo->DeviceNumber)
        {
            DeviceId = Index;
            if (ClientInfo->Devs[Index].ClientHandlesCount)
            {
                for(SubIndex = 0; SubIndex < ClientInfo->Devs[Index].ClientHandlesCount; SubIndex++)
                {
                    if (ClientInfo->Devs[Index].ClientHandles[SubIndex].PinId == PinConnect->PinId)
                    {
                        /* the pin has been already opened by the client, re-use it */
                        ASSERT(ClientInfo->Devs[Index].ClientHandles[SubIndex].bHandle == FALSE);

                        DispatchContext = ClientInfo->Devs[Index].ClientHandles[SubIndex].DispatchContext;
                        ObDereferenceObject(DispatchContext->MixerFileObject);
                        ObDereferenceObject(DispatchContext->FileObject);
                        ZwClose(DispatchContext->hMixerPin);
                        ZwClose(DispatchContext->Handle);
                        ClientInfo->Devs[Index].ClientHandles[SubIndex].PinId = (ULONG)-1;
                        goto Retry;
                    }
                }
            }
        }
    }

    ASSERT(DeviceId != (ULONG)-1);

    if (PinInstances.CurrentCount == PinInstances.PossibleCount)
    {
        /* pin already exists */
        ASSERT(Entry->Pins[PinConnect->PinId].PinHandle != NULL);
        if (Entry->Pins[PinConnect->PinId].References)
        {
            /* FIXME need ksmixer */
            DPRINT1("Device %u Pin %u References %u is already occupied, try later\n", InstanceInfo->DeviceNumber, PinConnect->PinId, Entry->Pins[PinConnect->PinId].References);
            if (MixerFormat)
                ExFreePool(MixerFormat);
            return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, 0);
        }
    }

    WorkItem = IoAllocateWorkItem(DeviceObject);
    if (!WorkItem)
    {
        if (MixerFormat)
            ExFreePool(MixerFormat);
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* create worker context */
    WorkerContext = ExAllocatePool(NonPagedPool, sizeof(PIN_WORKER_CONTEXT));
    if (!WorkerContext)
    {
        /* invalid parameters */
        if (MixerFormat)
            ExFreePool(MixerFormat);
        IoFreeWorkItem(WorkItem);
        return SetIrpIoStatus(Irp, STATUS_NO_MEMORY, 0);
    }

    /* create worker context */
    DispatchContext = ExAllocatePool(NonPagedPool, sizeof(DISPATCH_CONTEXT));
    if (!DispatchContext)
    {
        /* invalid parameters */
        if (MixerFormat)
            ExFreePool(MixerFormat);
        IoFreeWorkItem(WorkItem);
        ExFreePool(WorkerContext);
        return SetIrpIoStatus(Irp, STATUS_NO_MEMORY, 0);
    }
    /* prepare context */
    RtlZeroMemory(WorkerContext, sizeof(PIN_WORKER_CONTEXT));
    RtlZeroMemory(DispatchContext, sizeof(DISPATCH_CONTEXT));

    DPRINT1("PinInstances.CurrentCount %u\n", PinInstances.CurrentCount);

    if (PinInstances.CurrentCount < PinInstances.PossibleCount)
    {
        WorkerContext->CreateRealPin = TRUE;
    }

    /* set up context */
    WorkerContext->DispatchContext = DispatchContext;
    WorkerContext->Entry = Entry;
    WorkerContext->Irp = Irp;
    WorkerContext->PinConnect = PinConnect;
    WorkerContext->AudioClient = ClientInfo;
    WorkerContext->CreateMixerPin = CreateMixerPin;
    WorkerContext->DeviceExtension = DeviceExtension;
    WorkerContext->MixerFormat = MixerFormat;

    DPRINT("Queing Irp %p\n", Irp);
    /* queue the work item */
    IoMarkIrpPending(Irp);
    Irp->IoStatus.Status = STATUS_PENDING;
    Irp->IoStatus.Information = 0;
    IoQueueWorkItem(WorkItem, CreatePinWorkerRoutine, DelayedWorkQueue, (PVOID)WorkerContext);

    /* mark irp as pending */
    return STATUS_PENDING;
}

NTSTATUS
SysAudioHandleProperty(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
    KSPROPERTY PropertyRequest;
    KSCOMPONENTID ComponentId;
    PULONG Index;
    PKSPROPERTY Property;
    PSYSAUDIODEVEXT DeviceExtension;
    PKSAUDIO_DEVICE_ENTRY Entry;
    PSYSAUDIO_INSTANCE_INFO InstanceInfo;
    ULONG BytesReturned;
    PKSOBJECT_CREATE_ITEM CreateItem;
    UNICODE_STRING GuidString;

    /* access the create item */
    CreateItem = KSCREATE_ITEM_IRP_STORAGE(Irp);

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    if (IoStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(KSPROPERTY))
    {
        /* buffer must be at least of sizeof KSPROPERTY */
        return SetIrpIoStatus(Irp, STATUS_BUFFER_TOO_SMALL, sizeof(KSPROPERTY));
    }

    Property = (PKSPROPERTY)IoStack->Parameters.DeviceIoControl.Type3InputBuffer;
    DeviceExtension = (PSYSAUDIODEVEXT)DeviceObject->DeviceExtension;

    if (IsEqualGUIDAligned(&Property->Set, &KSPROPSETID_Pin))
    {
        return HandleSysAudioFilterPinProperties(Irp, Property, DeviceExtension);
    }
    else if (IsEqualGUIDAligned(&Property->Set, &KSPROPSETID_Sysaudio))
    {
        if (Property->Id == KSPROPERTY_SYSAUDIO_COMPONENT_ID)
        {
            if (IoStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(KSPROPERTY) + sizeof(ULONG))
            {
                /* too small buffer */
                return SetIrpIoStatus(Irp, STATUS_BUFFER_TOO_SMALL, sizeof(KSPROPERTY) + sizeof(ULONG));
            }

            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(KSCOMPONENTID))
            {
                /* too small buffer */
                return SetIrpIoStatus(Irp, STATUS_BUFFER_TOO_SMALL, sizeof(KSCOMPONENTID));
            }

            Index = (PULONG)(Property + 1);

            if (DeviceExtension->NumberOfKsAudioDevices <= *Index)
            {
                /* invalid index */
                return SetIrpIoStatus(Irp, STATUS_INVALID_PARAMETER, 0);
            }
            Entry = GetListEntry(&DeviceExtension->KsAudioDeviceList, *Index);
            ASSERT(Entry != NULL);

            PropertyRequest.Set = KSPROPSETID_General;
            PropertyRequest.Id = KSPROPERTY_GENERAL_COMPONENTID;
            PropertyRequest.Flags = KSPROPERTY_TYPE_GET;

            /* call the filter */
            Status = KsSynchronousIoControlDevice(Entry->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&PropertyRequest, sizeof(KSPROPERTY), (PVOID)&ComponentId, sizeof(KSCOMPONENTID), &BytesReturned);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("KsSynchronousIoControlDevice failed with %x for KSPROPERTY_GENERAL_COMPONENTID\n", Status);
                return SetIrpIoStatus(Irp, Status, 0);
            }
            RtlMoveMemory(Irp->UserBuffer, &ComponentId, sizeof(KSCOMPONENTID));
            return SetIrpIoStatus(Irp, STATUS_SUCCESS, sizeof(KSCOMPONENTID));
        }
        else if (Property->Id == KSPROPERTY_SYSAUDIO_DEVICE_COUNT)
        {
            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG))
            {
                /* too small buffer */
                return SetIrpIoStatus(Irp, STATUS_BUFFER_TOO_SMALL, sizeof(ULONG));
            }

            *((PULONG)Irp->UserBuffer) = DeviceExtension->NumberOfKsAudioDevices;
            return SetIrpIoStatus(Irp, STATUS_SUCCESS, sizeof(ULONG));
        }
        else if (Property->Id == KSPROPERTY_SYSAUDIO_DEVICE_INSTANCE)
        {
            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG))
            {
                /* too small buffer */
                return SetIrpIoStatus(Irp, STATUS_BUFFER_TOO_SMALL, sizeof(ULONG));
            }

            if (Property->Flags & KSPROPERTY_TYPE_SET)
            {
                Index = (PULONG)Irp->UserBuffer;
                return SysAudioOpenVirtualDevice(Irp, *Index, DeviceExtension);
            }
        }
        else if (Property->Id == KSPROPERTY_SYSAUDIO_INSTANCE_INFO)
        {
            if (IoStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(SYSAUDIO_INSTANCE_INFO))
            {
                /* too small buffer */
                return SetIrpIoStatus(Irp, STATUS_BUFFER_TOO_SMALL, sizeof(SYSAUDIO_INSTANCE_INFO));
            }

            /* get input parameter */
            InstanceInfo = (PSYSAUDIO_INSTANCE_INFO)Property;

            if (Property->Flags & KSPROPERTY_TYPE_SET)
            {
                return SysAudioOpenVirtualDevice(Irp, InstanceInfo->DeviceNumber, DeviceExtension);
            }
        }
        else if (Property->Id == (ULONG)-1)
        {
            /* ros specific pin creation request */
            DPRINT1("Initiating create request\n");
            return HandleSysAudioFilterPinCreation(Irp, Property, DeviceExtension, DeviceObject);
        }
    }

    RtlStringFromGUID(&Property->Set, &GuidString);
    DPRINT1("Unhandeled property Set |%S| Id %u Flags %x\n", GuidString.Buffer, Property->Id, Property->Flags);
    DbgBreakPoint();
    RtlFreeUnicodeString(&GuidString);
    return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, 0);
}
