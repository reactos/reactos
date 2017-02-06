/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/sysaudio/control.c
 * PURPOSE:         System Audio graph builder
 * PROGRAMMER:      Johannes Anderwald
 */

#include "sysaudio.h"

#define NDEBUG
#include <debug.h>

const GUID KSPROPSETID_Sysaudio                 = {0xCBE3FAA0L, 0xCC75, 0x11D0, {0xB4, 0x65, 0x00, 0x00, 0x1A, 0x18, 0x18, 0xE6}};
const GUID KSPROPSETID_Sysaudio_Pin             = {0xA3A53220L, 0xC6E4, 0x11D0, {0xB4, 0x65, 0x00, 0x00, 0x1A, 0x18, 0x18, 0xE6}};
const GUID KSPROPSETID_General                  = {0x1464EDA5L, 0x6A8F, 0x11D1, {0x9A, 0xA7, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};
const GUID KSPROPSETID_Pin                     = {0x8C134960L, 0x51AD, 0x11CF, {0x87, 0x8A, 0x94, 0xF8, 0x01, 0xC1, 0x00, 0x00}};
const GUID KSPROPSETID_Connection              = {0x1D58C920L, 0xAC9B, 0x11CF, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};
const GUID KSPROPSETID_Topology                 = {0x720D4AC0L, 0x7533, 0x11D0, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};
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
    PKSAUDIO_DEVICE_ENTRY Entry;
    PIO_STACK_LOCATION IoStack;

    /* get current irp stack */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* sanity check */
    ASSERT(IoStack->FileObject);

    if (DeviceNumber >= DeviceExtension->NumberOfKsAudioDevices)
    {
        /* invalid device index */
        return SetIrpIoStatus(Irp, STATUS_INVALID_PARAMETER, 0);
    }

    /* get device context */
    Entry = GetListEntry(&DeviceExtension->KsAudioDeviceList, DeviceNumber);
    ASSERT(Entry != NULL);

    /* store device entry in FsContext
     * see pin.c DispatchCreateSysAudioPin for details
     */
    IoStack->FileObject->FsContext = (PVOID)Entry;

    return SetIrpIoStatus(Irp, STATUS_SUCCESS, 0);
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

    // in order to access pin properties of a sysaudio device
    // the caller must provide a KSP_PIN struct, where
    // Reserved member points to virtual device index

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    if (IoStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(KSP_PIN))
    {
        /* too small buffer */
        return SetIrpIoStatus(Irp, STATUS_BUFFER_TOO_SMALL, sizeof(KSPROPERTY) + sizeof(ULONG));
    }

    Entry = GetListEntry(&DeviceExtension->KsAudioDeviceList, ((KSP_PIN*)Property)->Reserved);
    if (!Entry)
    {
        /* invalid device index */
        return SetIrpIoStatus(Irp, STATUS_INVALID_PARAMETER, 0);
    }

    /* forward request to the filter implementing the property */
    Status = KsSynchronousIoControlDevice(Entry->FileObject, KernelMode, IOCTL_KS_PROPERTY,
                                             (PVOID)IoStack->Parameters.DeviceIoControl.Type3InputBuffer,
                                             IoStack->Parameters.DeviceIoControl.InputBufferLength,
                                             Irp->UserBuffer,
                                             IoStack->Parameters.DeviceIoControl.OutputBufferLength,
                                             &BytesReturned);

    return SetIrpIoStatus(Irp, Status, BytesReturned);
}


NTSTATUS
ComputeCompatibleFormat(
    IN PKSAUDIO_DEVICE_ENTRY Entry,
    IN ULONG PinId,
    IN PKSDATAFORMAT_WAVEFORMATEX ClientFormat,
    OUT PKSDATAFORMAT_WAVEFORMATEX MixerFormat)
{
    BOOL bFound;
    ULONG BytesReturned;
    PKSP_PIN PinRequest;
    NTSTATUS Status;
    PKSMULTIPLE_ITEM MultipleItem;
    ULONG Length;
    PKSDATARANGE_AUDIO AudioRange;
    ULONG Index;

    Length = sizeof(KSP_PIN) + sizeof(KSMULTIPLE_ITEM) + ClientFormat->DataFormat.FormatSize;
    PinRequest = AllocateItem(NonPagedPool, Length);
    if (!PinRequest)
        return STATUS_UNSUCCESSFUL;

    PinRequest->PinId = PinId;
    PinRequest->Property.Set = KSPROPSETID_Pin;
    PinRequest->Property.Flags = KSPROPERTY_TYPE_GET;
    PinRequest->Property.Id = KSPROPERTY_PIN_DATAINTERSECTION;

    MultipleItem = (PKSMULTIPLE_ITEM)(PinRequest + 1);
    MultipleItem->Count = 1;
    MultipleItem->Size = ClientFormat->DataFormat.FormatSize;

    RtlMoveMemory(MultipleItem + 1, ClientFormat, ClientFormat->DataFormat.FormatSize);
    /* Query the miniport data intersection handler */
    Status = KsSynchronousIoControlDevice(Entry->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)PinRequest, Length, (PVOID)MixerFormat, sizeof(KSDATAFORMAT_WAVEFORMATEX), &BytesReturned);

    DPRINT("Status %x\n", Status);

    if (NT_SUCCESS(Status))
    {
        FreeItem(PinRequest);
        return Status;
    }

    /* Setup request block */
    PinRequest->Property.Id = KSPROPERTY_PIN_DATARANGES;
    /* Query pin data ranges */
    Status = KsSynchronousIoControlDevice(Entry->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)PinRequest, sizeof(KSP_PIN), NULL, 0, &BytesReturned);

    if (Status != STATUS_MORE_ENTRIES)
    {
        /* Failed to get data ranges */
        return Status;
    }

    MultipleItem = AllocateItem(NonPagedPool, BytesReturned);
    if (!MultipleItem)
    {
        FreeItem(PinRequest);
        return STATUS_NO_MEMORY;
    }

    Status = KsSynchronousIoControlDevice(Entry->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)PinRequest, sizeof(KSP_PIN), (PVOID)MultipleItem, BytesReturned, &BytesReturned);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Property Request KSPROPERTY_PIN_DATARANGES failed with %x\n", Status);
        FreeItem(MultipleItem);
        FreeItem(PinRequest);
        return STATUS_UNSUCCESSFUL;
    }

    AudioRange = (PKSDATARANGE_AUDIO)(MultipleItem + 1);
    bFound = FALSE;
    for(Index = 0; Index < MultipleItem->Count; Index++)
    {
        if (AudioRange->DataRange.FormatSize != sizeof(KSDATARANGE_AUDIO))
        {
            UNIMPLEMENTED;
            AudioRange = (PKSDATARANGE_AUDIO)((PUCHAR)AudioRange + AudioRange->DataRange.FormatSize);
            continue;
        }
        /* Select best quality available */

        MixerFormat->DataFormat.FormatSize = sizeof(KSDATAFORMAT) + sizeof(WAVEFORMATEX);
        MixerFormat->DataFormat.Flags = 0;
        MixerFormat->DataFormat.Reserved = 0;
        MixerFormat->DataFormat.MajorFormat = KSDATAFORMAT_TYPE_AUDIO;
        MixerFormat->DataFormat.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
        MixerFormat->DataFormat.Specifier = KSDATAFORMAT_SPECIFIER_WAVEFORMATEX;
        MixerFormat->DataFormat.SampleSize = 4;
        MixerFormat->WaveFormatEx.wFormatTag = ClientFormat->WaveFormatEx.wFormatTag;
#ifndef NO_AC97_HACK
        /* HACK: AC97 does not support mono render / record */
        MixerFormat->WaveFormatEx.nChannels = 2;
        /*HACK: AC97 only supports 16-Bit Bits */
        MixerFormat->WaveFormatEx.wBitsPerSample = 16;

#else
        MixerFormat->WaveFormatEx.nChannels = min(ClientFormat->WaveFormatEx.nChannels, AudioRange->MaximumChannels);
        MixerFormat->WaveFormatEx.wBitsPerSample = AudioRange->MaximumBitsPerSample;
#endif

#ifdef KMIXER_RESAMPLING_IMPLEMENTED
        MixerFormat->WaveFormatEx.nSamplesPerSec = AudioRange->MaximumSampleFrequency;
#else
        MixerFormat->WaveFormatEx.nSamplesPerSec = max(AudioRange->MinimumSampleFrequency, min(ClientFormat->WaveFormatEx.nSamplesPerSec, AudioRange->MaximumSampleFrequency));
#endif

        MixerFormat->WaveFormatEx.cbSize = 0;
        MixerFormat->WaveFormatEx.nBlockAlign = (MixerFormat->WaveFormatEx.nChannels * MixerFormat->WaveFormatEx.wBitsPerSample) / 8;
        MixerFormat->WaveFormatEx.nAvgBytesPerSec = MixerFormat->WaveFormatEx.nChannels * MixerFormat->WaveFormatEx.nSamplesPerSec * (MixerFormat->WaveFormatEx.wBitsPerSample / 8);

        bFound = TRUE;
        break;

        AudioRange = (PKSDATARANGE_AUDIO)((PUCHAR)AudioRange + AudioRange->DataRange.FormatSize);
    }

#if 0
    DPRINT1("\nNum Max Channels %u Channels %u Old Channels %u\n Max SampleRate %u SampleRate %u Old SampleRate %u\n Max BitsPerSample %u BitsPerSample %u Old BitsPerSample %u\n",
           AudioRange->MaximumChannels, MixerFormat->WaveFormatEx.nChannels, ClientFormat->WaveFormatEx.nChannels,
           AudioRange->MaximumSampleFrequency, MixerFormat->WaveFormatEx.nSamplesPerSec, ClientFormat->WaveFormatEx.nSamplesPerSec,
           AudioRange->MaximumBitsPerSample, MixerFormat->WaveFormatEx.wBitsPerSample, ClientFormat->WaveFormatEx.wBitsPerSample);


#endif

    FreeItem(MultipleItem);
    FreeItem(PinRequest);

    if (bFound)
        return STATUS_SUCCESS;
    else
        return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
GetPinInstanceCount(
    PKSAUDIO_DEVICE_ENTRY Entry,
    PKSPIN_CINSTANCES PinInstances,
    PKSPIN_CONNECT PinConnect)
{
    KSP_PIN PinRequest;
    ULONG BytesReturned;

    /* query the instance count */
    PinRequest.PinId = PinConnect->PinId;
    PinRequest.Property.Set = KSPROPSETID_Pin;
    PinRequest.Property.Flags = KSPROPERTY_TYPE_GET;
    PinRequest.Property.Id = KSPROPERTY_PIN_CINSTANCES;
	ASSERT(Entry->FileObject);
    return KsSynchronousIoControlDevice(Entry->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&PinRequest, sizeof(KSP_PIN), (PVOID)PinInstances, sizeof(KSPIN_CINSTANCES), &BytesReturned);

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
    UNICODE_STRING GuidString;
    PKSP_PIN Pin;
    LPWSTR DeviceName;

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
    else if(IsEqualGUIDAligned(&Property->Set, &KSPROPSETID_Topology))
    {
        if (IoStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(KSP_PIN))
        {
            /* too small buffer */
            return SetIrpIoStatus(Irp, STATUS_BUFFER_TOO_SMALL, sizeof(KSP_PIN));
        }
        Pin = (PKSP_PIN)IoStack->Parameters.DeviceIoControl.Type3InputBuffer;
        Entry = GetListEntry(&DeviceExtension->KsAudioDeviceList, Pin->Reserved);
        ASSERT(Entry != NULL);

        /* forward request to the filter implementing the property */
        Status = KsSynchronousIoControlDevice(Entry->FileObject, KernelMode, IOCTL_KS_PROPERTY,
                                             (PVOID)IoStack->Parameters.DeviceIoControl.Type3InputBuffer,
                                             IoStack->Parameters.DeviceIoControl.InputBufferLength,
                                             Irp->UserBuffer,
                                             IoStack->Parameters.DeviceIoControl.OutputBufferLength,
                                             &BytesReturned);

        return SetIrpIoStatus(Irp, Status, BytesReturned);
    }
    else if (IsEqualGUIDAligned(&Property->Set, &KSPROPSETID_Sysaudio))
    {
        if (Property->Id == KSPROPERTY_SYSAUDIO_DEVICE_INTERFACE_NAME)
        {
            if (IoStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(KSPROPERTY) + sizeof(ULONG))
            {
                /* invalid request */
                return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, sizeof(KSPROPERTY) + sizeof(ULONG));
            }
            Index = (PULONG)(Property + 1);

            if (DeviceExtension->NumberOfKsAudioDevices <= *Index)
            {
                /* invalid index */
                return SetIrpIoStatus(Irp, STATUS_INVALID_PARAMETER, 0);
            }

            Entry = GetListEntry(&DeviceExtension->KsAudioDeviceList, *Index);
            ASSERT(Entry != NULL);

            BytesReturned = Entry->DeviceName.Length + sizeof(WCHAR);
            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < BytesReturned)
            {
                /* too small buffer */
                return SetIrpIoStatus(Irp, STATUS_BUFFER_TOO_SMALL, BytesReturned);
            }

            /* copy device name */
            DeviceName = (LPWSTR)Irp->UserBuffer;

            RtlMoveMemory(DeviceName, Entry->DeviceName.Buffer, Entry->DeviceName.Length);
            DeviceName[Entry->DeviceName.Length / sizeof(WCHAR)] = L'\0';
            return SetIrpIoStatus(Irp, STATUS_SUCCESS, BytesReturned);
        }

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
                DPRINT("KsSynchronousIoControlDevice failed with %x for KSPROPERTY_GENERAL_COMPONENTID\n", Status);
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
    }

    RtlStringFromGUID(&Property->Set, &GuidString);
    DPRINT1("Unhandled property Set |%S| Id %u Flags %x\n", GuidString.Buffer, Property->Id, Property->Flags);
    RtlFreeUnicodeString(&GuidString);
    return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, 0);
}
