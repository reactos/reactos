/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/legacy/wdmaud/wave.c
 * PURPOSE:         Wave Out enumeration
 * PROGRAMMER:      Andrew Greenwood
 *                  Johannes Anderwald
 */
#include "wdmaud.h"


typedef struct
{
    ULONG SampleRate;
    ULONG Bit8Mono;
    ULONG Bit8Stereo;
    ULONG Bit16Mono;
    ULONG Bit16Stereo;
}AUDIO_RANGE;

#define AUDIO_TEST_RANGE (5)

static AUDIO_RANGE TestRange[AUDIO_TEST_RANGE] =
{
    {
        11025,
        WAVE_FORMAT_1M08,
        WAVE_FORMAT_1S08,
        WAVE_FORMAT_1M16,
        WAVE_FORMAT_1S16
    },
    {
        22050,
        WAVE_FORMAT_2M08,
        WAVE_FORMAT_2S08,
        WAVE_FORMAT_2M16,
        WAVE_FORMAT_2S16
    },
    {
        44100,
        WAVE_FORMAT_4M08,
        WAVE_FORMAT_4S08,
        WAVE_FORMAT_4M16,
        WAVE_FORMAT_4S16
    },
    {
        48000,
        WAVE_FORMAT_48M08,
        WAVE_FORMAT_48S08,
        WAVE_FORMAT_48M16,
        WAVE_FORMAT_48S16
    },
    {
        96000,
        WAVE_FORMAT_96M08,
        WAVE_FORMAT_96S08,
        WAVE_FORMAT_96M16,
        WAVE_FORMAT_96S16
    }
};

LPWAVE_INFO
AllocateWaveInfo()
{
    /* allocate wav info */
    LPWAVE_INFO WaveOutInfo = ExAllocatePool(NonPagedPool, sizeof(WAVE_INFO));
    if (!WaveOutInfo)
        return NULL;

    /* zero wave info struct */
    RtlZeroMemory(WaveOutInfo, sizeof(WAVE_INFO));

    return WaveOutInfo;
}

PKSPIN_CONNECT
AllocatePinConnect(
    ULONG DataFormatSize)
{
    PKSPIN_CONNECT Connect = ExAllocatePool(NonPagedPool, sizeof(KSPIN_CONNECT) + DataFormatSize);
    if (!Connect)
        return NULL;

    /* zero pin connect struct */
    RtlZeroMemory(Connect, sizeof(KSPIN_CONNECT) + DataFormatSize);

    return Connect;
}

NTSTATUS
GetWaveInfoByIndexAndType(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  ULONG DeviceIndex,
    IN  SOUND_DEVICE_TYPE DeviceType,
    OUT LPWAVE_INFO *OutWaveInfo)
{
    PWDMAUD_DEVICE_EXTENSION DeviceExtension;
    ULONG Index = 0;
    PLIST_ENTRY Entry, ListHead;
    LPWAVE_INFO WaveInfo;

    /* get device extension */
    DeviceExtension = (PWDMAUD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    if (DeviceType == WAVE_IN_DEVICE_TYPE)
        ListHead = &DeviceExtension->WaveInList;
    else
        ListHead = &DeviceExtension->WaveOutList;

    /* get first entry */
    Entry = ListHead->Flink;

    while(Entry != ListHead)
    {
        WaveInfo = (LPWAVE_INFO)CONTAINING_RECORD(Entry, WAVE_INFO, Entry);

        if (Index == DeviceIndex)
        {
            *OutWaveInfo = WaveInfo;
            return STATUS_SUCCESS;
        }
        Index++;
        Entry = Entry->Flink;
    }

    return STATUS_NOT_FOUND;
}


VOID
InitializePinConnect(
    IN OUT PKSPIN_CONNECT PinConnect,
    IN ULONG PinId)
{
    PinConnect->Interface.Set = KSINTERFACESETID_Standard;
    PinConnect->Interface.Id = KSINTERFACE_STANDARD_STREAMING;
    PinConnect->Interface.Flags = 0;
    PinConnect->Medium.Set = KSMEDIUMSETID_Standard;
    PinConnect->Medium.Id = KSMEDIUM_TYPE_ANYINSTANCE;
    PinConnect->Medium.Flags = 0;
    PinConnect->PinToHandle = NULL;
    PinConnect->PinId = PinId;
    PinConnect->Priority.PriorityClass = KSPRIORITY_NORMAL;
    PinConnect->Priority.PrioritySubClass = 1;
}

VOID
InitializeDataFormat(
    IN PKSDATAFORMAT_WAVEFORMATEX DataFormat,
    LPWAVEFORMATEX WaveFormatEx)
{

    DataFormat->WaveFormatEx.wFormatTag = WaveFormatEx->wFormatTag;
    DataFormat->WaveFormatEx.nChannels = WaveFormatEx->nChannels;
    DataFormat->WaveFormatEx.nSamplesPerSec = WaveFormatEx->nSamplesPerSec;
    DataFormat->WaveFormatEx.nBlockAlign = WaveFormatEx->nBlockAlign;
    DataFormat->WaveFormatEx.nAvgBytesPerSec = WaveFormatEx->nAvgBytesPerSec;
    DataFormat->WaveFormatEx.wBitsPerSample = WaveFormatEx->wBitsPerSample;
    DataFormat->WaveFormatEx.cbSize = 0;
    DataFormat->DataFormat.FormatSize = sizeof(KSDATAFORMAT) + sizeof(WAVEFORMATEX);
    DataFormat->DataFormat.Flags = 0;
    DataFormat->DataFormat.Reserved = 0;
    DataFormat->DataFormat.MajorFormat = KSDATAFORMAT_TYPE_AUDIO;

    DataFormat->DataFormat.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    DataFormat->DataFormat.Specifier = KSDATAFORMAT_SPECIFIER_WAVEFORMATEX;
    DataFormat->DataFormat.SampleSize = 4;
}


NTSTATUS
AttachToVirtualAudioDevice(
    IN PWDMAUD_DEVICE_EXTENSION DeviceExtension,
    IN ULONG VirtualDeviceId)
{
    ULONG BytesReturned;
    SYSAUDIO_INSTANCE_INFO InstanceInfo;

    /* setup property request */
    InstanceInfo.Property.Set = KSPROPSETID_Sysaudio;
    InstanceInfo.Property.Id = KSPROPERTY_SYSAUDIO_INSTANCE_INFO;
    InstanceInfo.Property.Flags = KSPROPERTY_TYPE_SET;
    InstanceInfo.Flags = 0;
    InstanceInfo.DeviceNumber = VirtualDeviceId;

    /* attach to virtual device */
   return KsSynchronousIoControlDevice(DeviceExtension->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&InstanceInfo, sizeof(SYSAUDIO_INSTANCE_INFO), NULL, 0, &BytesReturned);

}

NTSTATUS
GetAudioPinDataRanges(
    IN PWDMAUD_DEVICE_EXTENSION DeviceExtension,
    IN ULONG FilterId,
    IN ULONG PinId,
    IN OUT PKSMULTIPLE_ITEM * OutMultipleItem)
{
    KSP_PIN PinProperty;
    ULONG BytesReturned = 0;
    NTSTATUS Status;
    PKSMULTIPLE_ITEM MultipleItem;

    /* retrieve size of data ranges buffer */
    PinProperty.Reserved = FilterId;
    PinProperty.PinId = PinId;
    PinProperty.Property.Set = KSPROPSETID_Pin;
    PinProperty.Property.Id = KSPROPERTY_PIN_DATARANGES;
    PinProperty.Property.Flags = KSPROPERTY_TYPE_GET;

    Status = KsSynchronousIoControlDevice(DeviceExtension->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&PinProperty, sizeof(KSP_PIN), (PVOID)NULL, 0, &BytesReturned);
    if (Status != STATUS_MORE_ENTRIES)
    {
        return Status;
    }

    MultipleItem = ExAllocatePool(NonPagedPool, BytesReturned);
    if (!MultipleItem)
    {
        /* not enough memory */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = KsSynchronousIoControlDevice(DeviceExtension->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&PinProperty, sizeof(KSP_PIN), (PVOID)MultipleItem, BytesReturned, &BytesReturned);
    if (!NT_SUCCESS(Status))
    {
        /* failed */
        ExFreePool(MultipleItem);
        return Status;
    }

    /* save result */
    *OutMultipleItem = MultipleItem;
    return Status;
}

NTSTATUS
FindAudioDataRange(
    PKSMULTIPLE_ITEM MultipleItem,
    PKSDATARANGE_AUDIO * OutDataRangeAudio)
{
    ULONG Index;
    PKSDATARANGE_AUDIO DataRangeAudio;
    PKSDATARANGE DataRange;

    DataRange = (PKSDATARANGE) (MultipleItem + 1);
    for(Index = 0; Index < MultipleItem->Count; Index++)
    {
        if (DataRange->FormatSize == sizeof(KSDATARANGE_AUDIO))
        {
            DataRangeAudio = (PKSDATARANGE_AUDIO)DataRange;
            if (IsEqualGUIDAligned(&DataRangeAudio->DataRange.MajorFormat, &KSDATAFORMAT_TYPE_AUDIO) &&
                IsEqualGUIDAligned(&DataRangeAudio->DataRange.SubFormat, &KSDATAFORMAT_SUBTYPE_PCM) &&
                IsEqualGUIDAligned(&DataRangeAudio->DataRange.Specifier, &KSDATAFORMAT_SPECIFIER_WAVEFORMATEX))
            {
                DPRINT("Min Sample %u Max Sample %u Min Bits %u Max Bits %u Max Channel %u\n", DataRangeAudio->MinimumSampleFrequency, DataRangeAudio->MaximumSampleFrequency,
                                                         DataRangeAudio->MinimumBitsPerSample, DataRangeAudio->MaximumBitsPerSample, DataRangeAudio->MaximumChannels);
                *OutDataRangeAudio = DataRangeAudio;
                return STATUS_SUCCESS;
            }
        }
    }
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
OpenWavePin(
    IN PWDMAUD_DEVICE_EXTENSION DeviceExtension,
    IN ULONG FilterId,
    IN ULONG PinId,
    IN LPWAVEFORMATEX WaveFormatEx,
    IN ACCESS_MASK DesiredAccess,
    OUT PHANDLE PinHandle)
{
    PKSPIN_CONNECT PinConnect;
    PKSDATAFORMAT_WAVEFORMATEX DataFormat;
    NTSTATUS Status;

    /* allocate pin connect */
    PinConnect = AllocatePinConnect(sizeof(KSDATAFORMAT_WAVEFORMATEX));
    if (!PinConnect)
    {
        /* no memory */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* initialize pin connect struct */
    InitializePinConnect(PinConnect, PinId);

    /* get offset to dataformat */
    DataFormat = (PKSDATAFORMAT_WAVEFORMATEX) (PinConnect + 1);
    /* initialize with requested wave format */
    InitializeDataFormat(DataFormat, WaveFormatEx);

    /* first attach to the virtual device */
    Status = AttachToVirtualAudioDevice(DeviceExtension, FilterId);

    if (!NT_SUCCESS(Status))
    {
        /* failed */
        ExFreePool(PinConnect);
        return Status;
    }

    /* now create the pin */
    Status = KsCreatePin(DeviceExtension->hSysAudio, PinConnect, DesiredAccess, PinHandle);

    /* free create info */
    ExFreePool(PinConnect);

    return Status;
}

ULONG
GetPinInstanceCount(
    PWDMAUD_DEVICE_EXTENSION DeviceExtension,
    ULONG FilterId,
    ULONG PinId)
{
    KSP_PIN PinRequest;
    KSPIN_CINSTANCES PinInstances;
    ULONG BytesReturned;
    NTSTATUS Status;

    /* query the instance count */
    PinRequest.Reserved = FilterId;
    PinRequest.PinId = PinId;
    PinRequest.Property.Set = KSPROPSETID_Pin;
    PinRequest.Property.Flags = KSPROPERTY_TYPE_GET;
    PinRequest.Property.Id = KSPROPERTY_PIN_CINSTANCES;

    Status = KsSynchronousIoControlDevice(DeviceExtension->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&PinRequest, sizeof(KSP_PIN), (PVOID)&PinInstances, sizeof(KSPIN_CINSTANCES), &BytesReturned);
    ASSERT(Status == STATUS_SUCCESS);
    return PinInstances.CurrentCount;
}


NTSTATUS
CheckSampleFormat(
    PWDMAUD_DEVICE_EXTENSION DeviceExtension,
    LPWAVE_INFO WaveInfo,
    ULONG SampleRate,
    ULONG NumChannels,
    ULONG BitsPerSample)
{
    NTSTATUS Status = STATUS_SUCCESS;
#if 0

    WAVEFORMATEX WaveFormat;
    HANDLE PinHandle;

    /* clear wave format */
    RtlZeroMemory(&WaveFormat, sizeof(WAVEFORMATEX));

    WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
    WaveFormat.nChannels = NumChannels;
    WaveFormat.nSamplesPerSec = SampleRate;
    WaveFormat.nAvgBytesPerSec = SampleRate * NumChannels * (BitsPerSample/8);
    WaveFormat.nBlockAlign = (NumChannels * BitsPerSample) / 8;
    WaveFormat.wBitsPerSample = BitsPerSample;
    WaveFormat.cbSize = sizeof(WAVEFORMATEX);

    Status = OpenWavePin(DeviceExtension, WaveInfo->FilterId, WaveInfo->PinId, &WaveFormat, GENERIC_READ | GENERIC_WRITE, &PinHandle);

    if (NT_SUCCESS(Status))
    {
        /* success */
        ZwClose(PinHandle);

        while(GetPinInstanceCount(DeviceExtension, WaveInfo->FilterId, WaveInfo->PinId))
            KeStallExecutionProcessor(5);
    }

    DPRINT("SampleRate %u BitsPerSample %u NumChannels %u Status %x bInput %u\n", SampleRate, BitsPerSample, NumChannels, Status, WaveInfo->bInput);
#endif


    return Status;
}


NTSTATUS
CheckFormat(
    PWDMAUD_DEVICE_EXTENSION DeviceExtension,
    PKSDATARANGE_AUDIO DataRangeAudio,
    LPWAVE_INFO WaveInfo)
{
    ULONG Index, SampleFrequency;
    ULONG Result = 0;
    NTSTATUS Status;

    for(Index = 0; Index < AUDIO_TEST_RANGE; Index++)
    {
        SampleFrequency = TestRange[Index].SampleRate;

        if (DataRangeAudio->MinimumSampleFrequency <= SampleFrequency && DataRangeAudio->MaximumSampleFrequency >= SampleFrequency)
        {
            /* the audio adapter supports the sample frequency */
            if (DataRangeAudio->MinimumBitsPerSample <= 8 && DataRangeAudio->MaximumBitsPerSample >= 8)
            {
                /* check if pin supports the sample rate in 8-Bit Mono */
                Status = CheckSampleFormat(DeviceExtension, WaveInfo, SampleFrequency, 1, 8);
                if (NT_SUCCESS(Status))
                {
                    Result |= TestRange[Index].Bit8Mono;
                }

                if (DataRangeAudio->MaximumChannels > 1)
                {
                    /* check if pin supports the sample rate in 8-Bit Stereo */
                    Status = CheckSampleFormat(DeviceExtension, WaveInfo, SampleFrequency, 2, 8);
                    if (NT_SUCCESS(Status))
                    {
                        Result |= TestRange[Index].Bit8Stereo;
                    }
                }
            }

            if (DataRangeAudio->MinimumBitsPerSample <= 16 && DataRangeAudio->MaximumBitsPerSample >= 16)
            {
                /* check if pin supports the sample rate in 16-Bit Mono */
                Status = CheckSampleFormat(DeviceExtension, WaveInfo, SampleFrequency, 1, 16);
                if (NT_SUCCESS(Status))
                {
                    Result |= TestRange[Index].Bit16Mono;
                }

                if (DataRangeAudio->MaximumChannels > 1)
                {
                    /* check if pin supports the sample rate in 16-Bit Stereo */
                    Status = CheckSampleFormat(DeviceExtension, WaveInfo, SampleFrequency, 2, 16);
                    if (NT_SUCCESS(Status))
                    {
                        Result |= TestRange[Index].Bit16Stereo;
                    }
                }
            }
        }
    }


    if (WaveInfo->bInput)
        WaveInfo->u.InCaps.dwFormats = Result;
    else
        WaveInfo->u.OutCaps.dwFormats = Result;

    DPRINT("Format %x bInput %u\n", Result, WaveInfo->bInput);


    return STATUS_SUCCESS;
}

NTSTATUS
InitializeWaveInfo(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG FilterId,
    IN ULONG PinId,
    IN ULONG bInput)
{
    KSP_PIN PinProperty;
    KSCOMPONENTID ComponentId;
    NTSTATUS Status;
    ULONG BytesReturned;
    WCHAR DeviceName[MAX_PATH];
    PWDMAUD_DEVICE_EXTENSION DeviceExtension;
    PKSMULTIPLE_ITEM MultipleItem;
    PKSDATARANGE_AUDIO DataRangeAudio;
    LPWAVE_INFO WaveInfo = AllocateWaveInfo();


    if (!WaveInfo)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* initialize wave info */
    WaveInfo->bInput = bInput;
    WaveInfo->FilterId = FilterId;
    WaveInfo->PinId = PinId;

    /* setup request to return component id */
    PinProperty.PinId = FilterId;
    PinProperty.Property.Set = KSPROPSETID_Sysaudio;
    PinProperty.Property.Id = KSPROPERTY_SYSAUDIO_COMPONENT_ID;
    PinProperty.Property.Flags = KSPROPERTY_TYPE_GET;

    /* get device extension */
    DeviceExtension = (PWDMAUD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    /* query sysaudio for component id */
    Status = KsSynchronousIoControlDevice(DeviceExtension->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&PinProperty, sizeof(KSP_PIN), (PVOID)&ComponentId, sizeof(KSCOMPONENTID), &BytesReturned);
    if (NT_SUCCESS(Status))
    {
        if (bInput)
        {
            WaveInfo->u.InCaps.wMid = ComponentId.Manufacturer.Data1 - 0xd5a47fa7;
            WaveInfo->u.InCaps.vDriverVersion = MAKELONG(ComponentId.Version, ComponentId.Revision);
        }
        else
        {
            WaveInfo->u.OutCaps.wMid = ComponentId.Manufacturer.Data1 - 0xd5a47fa7;
            WaveInfo->u.OutCaps.vDriverVersion = MAKELONG(ComponentId.Version, ComponentId.Revision);
        }
    }
    else
    {
        /* set up something useful */
        if (bInput)
        {
            WaveInfo->u.InCaps.wMid = MM_MICROSOFT;
            WaveInfo->u.InCaps.wPid = MM_PID_UNMAPPED;
            WaveInfo->u.InCaps.vDriverVersion = 1;
        }
        else
        {
            WaveInfo->u.OutCaps.wMid = MM_MICROSOFT;
            WaveInfo->u.OutCaps.wPid = MM_PID_UNMAPPED;
            WaveInfo->u.OutCaps.vDriverVersion = 1;
        }
    }

    /* retrieve pnp base name */
    PinProperty.PinId = FilterId;
    PinProperty.Property.Set = KSPROPSETID_Sysaudio;
    PinProperty.Property.Id = KSPROPERTY_SYSAUDIO_DEVICE_INTERFACE_NAME;
    PinProperty.Property.Flags = KSPROPERTY_TYPE_GET;

    Status = KsSynchronousIoControlDevice(DeviceExtension->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&PinProperty, sizeof(KSP_PIN), (PVOID)DeviceName, sizeof(DeviceName), &BytesReturned);
    if (NT_SUCCESS(Status))
    {
        /* find product name */
        if (bInput)
            Status = FindProductName(DeviceName, MAXPNAMELEN, WaveInfo->u.OutCaps.szPname);
        else
            Status = FindProductName(DeviceName, MAXPNAMELEN, WaveInfo->u.InCaps.szPname);

        /* check for success */
        if (!NT_SUCCESS(Status))
        {
            if (bInput)
                WaveInfo->u.OutCaps.szPname[0] = L'\0';
            else
                WaveInfo->u.InCaps.szPname[0] = L'\0';
        }
    }

    Status = GetAudioPinDataRanges(DeviceExtension, FilterId, PinId, &MultipleItem);
    if (NT_SUCCESS(Status))
    {
        /* find a audio data range */
        Status = FindAudioDataRange(MultipleItem, &DataRangeAudio);

        if (NT_SUCCESS(Status))
        {
            if (bInput)
            {
                WaveInfo->u.InCaps.wChannels = DataRangeAudio->MaximumChannels;
            }
            else
            {
                WaveInfo->u.OutCaps.wChannels = DataRangeAudio->MaximumChannels;
            }
            CheckFormat(DeviceExtension, DataRangeAudio, WaveInfo);
        }
        ExFreePool(MultipleItem);
    }

    if (bInput)
    {
        InsertTailList(&DeviceExtension->WaveInList, &WaveInfo->Entry);
        DeviceExtension->WaveInDeviceCount++;
    }
    else
    {
        InsertTailList(&DeviceExtension->WaveOutList, &WaveInfo->Entry);
        DeviceExtension->WaveOutDeviceCount++;
    }


    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
WdmAudWaveInitialize(
    IN PDEVICE_OBJECT DeviceObject)
{
    KSP_PIN Pin;
    ULONG Count, BytesReturned, Index, SubIndex, Result, NumPins;
    NTSTATUS Status;
    KSPIN_COMMUNICATION Communication;
    KSPIN_DATAFLOW DataFlow;
    PWDMAUD_DEVICE_EXTENSION DeviceExtension;

    Pin.Property.Set = KSPROPSETID_Sysaudio;
    Pin.Property.Id = KSPROPERTY_SYSAUDIO_DEVICE_COUNT;
    Pin.Property.Flags = KSPROPERTY_TYPE_GET;

    DeviceExtension = (PWDMAUD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    /* set wave count to zero */
    DeviceExtension->WaveInDeviceCount = 0;
    DeviceExtension->WaveOutDeviceCount = 0;

    /* intialize list head */
    InitializeListHead(&DeviceExtension->WaveInList);
    InitializeListHead(&DeviceExtension->WaveOutList);

    Status = KsSynchronousIoControlDevice(DeviceExtension->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Pin, sizeof(KSPROPERTY), (PVOID)&Count, sizeof(ULONG), &BytesReturned);
    if (!NT_SUCCESS(Status))
        return STATUS_UNSUCCESSFUL;

    Result = 0;
    for(Index = 0; Index < Count; Index++)
    {
        /* query number of pins */
        Pin.Reserved = Index; // see sysaudio
        Pin.Property.Flags = KSPROPERTY_TYPE_GET;
        Pin.Property.Set = KSPROPSETID_Pin;
        Pin.Property.Id = KSPROPERTY_PIN_CTYPES;
        Pin.PinId = 0;

        Status = KsSynchronousIoControlDevice(DeviceExtension->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Pin, sizeof(KSP_PIN), (PVOID)&NumPins, sizeof(ULONG), &BytesReturned);
        if (NT_SUCCESS(Status))
        {
            /* enumerate now all pins */
            for(SubIndex = 0; SubIndex < NumPins; SubIndex++)
            {
                Pin.PinId = SubIndex;
                Pin.Property.Id = KSPROPERTY_PIN_COMMUNICATION;
                Communication = KSPIN_COMMUNICATION_NONE;

                /* get pin communication type */
                KsSynchronousIoControlDevice(DeviceExtension->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Pin, sizeof(KSP_PIN), (PVOID)&Communication, sizeof(KSPIN_COMMUNICATION), &BytesReturned);

                Pin.Property.Id = KSPROPERTY_PIN_DATAFLOW;
                DataFlow = 0;

                /* get pin dataflow type */
                KsSynchronousIoControlDevice(DeviceExtension->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Pin, sizeof(KSP_PIN), (PVOID)&DataFlow, sizeof(KSPIN_DATAFLOW), &BytesReturned);

                if (Communication == KSPIN_COMMUNICATION_SINK && DataFlow == KSPIN_DATAFLOW_IN)
                {
                    /* found a wave out device */
                    InitializeWaveInfo(DeviceObject, Index, SubIndex, FALSE);
                }
                else if (Communication == KSPIN_COMMUNICATION_SINK && DataFlow == KSPIN_DATAFLOW_OUT)
                {
                    /* found a wave in device */
                    InitializeWaveInfo(DeviceObject, Index, SubIndex, TRUE);
                }
            }
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
WdmAudControlOpenWave(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo)
{
    PWDMAUD_DEVICE_EXTENSION DeviceExtension;
    LPWAVE_INFO WaveInfo;
    NTSTATUS Status;
    ACCESS_MASK DesiredAccess = 0;
    HANDLE PinHandle;
    ULONG FreeIndex;

    if (DeviceInfo->u.WaveFormatEx.wFormatTag != WAVE_FORMAT_PCM)
    {
        DPRINT("FIXME: Only WAVE_FORMAT_PCM is supported RequestFormat %x\n", DeviceInfo->u.WaveFormatEx.wFormatTag);
        return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, 0);
    }

    /* get device extension */
    DeviceExtension = (PWDMAUD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    /* find destination wave */
    Status = GetWaveInfoByIndexAndType(DeviceObject, DeviceInfo->DeviceIndex, DeviceInfo->DeviceType, &WaveInfo);
    if (!NT_SUCCESS(Status))
    {
        /* failed to find wave info */
        DbgBreakPoint();
        return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, 0);
    }

    /* close pin handle which uses same virtual audio device id and pin id */
    FreeIndex = ClosePin(ClientInfo, WaveInfo->FilterId, WaveInfo->PinId, DeviceInfo->DeviceType);

    /* get desired access */
    if (DeviceInfo->DeviceType == WAVE_IN_DEVICE_TYPE)
    {
        DesiredAccess |= GENERIC_READ;
    }
     else if (DeviceInfo->DeviceType == WAVE_OUT_DEVICE_TYPE)
    {
        DesiredAccess |= GENERIC_WRITE;
    }

    /* now try open the pin */
    Status = OpenWavePin(DeviceExtension, WaveInfo->FilterId, WaveInfo->PinId, &DeviceInfo->u.WaveFormatEx, DesiredAccess, &PinHandle);

    if (!NT_SUCCESS(Status))
    {
        /* failed to open the pin */
        return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, 0);
    }

    /* store the handle */
    Status = InsertPinHandle(ClientInfo, WaveInfo->FilterId, WaveInfo->PinId, DeviceInfo->DeviceType, PinHandle, FreeIndex);
    if (!NT_SUCCESS(Status))
    {
        /* failed to insert handle */
        ZwClose(PinHandle);
        return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, 0);
    }

    /* store pin handle */
    DeviceInfo->hDevice = PinHandle;
    return SetIrpIoStatus(Irp, STATUS_SUCCESS, sizeof(WDMAUD_DEVICE_INFO));
}

NTSTATUS
WdmAudWaveCapabilities(
    IN PDEVICE_OBJECT DeviceObject,
    IN PWDMAUD_DEVICE_INFO DeviceInfo,
    IN PWDMAUD_CLIENT ClientInfo,
    IN PWDMAUD_DEVICE_EXTENSION DeviceExtension)
{
    LPWAVE_INFO WaveInfo;
    NTSTATUS Status;

    /* find destination wave */
    Status = GetWaveInfoByIndexAndType(DeviceObject, DeviceInfo->DeviceIndex, DeviceInfo->DeviceType, &WaveInfo);
    if (!NT_SUCCESS(Status))
    {
        /* failed to find wave info */
        DbgBreakPoint();
        return STATUS_UNSUCCESSFUL;
    }

    if (DeviceInfo->DeviceType == WAVE_IN_DEVICE_TYPE)
    {
        RtlMoveMemory(&DeviceInfo->u.WaveInCaps, &WaveInfo->u.InCaps, sizeof(WAVEINCAPSW));
    }
    else
    {
        RtlMoveMemory(&DeviceInfo->u.WaveOutCaps, &WaveInfo->u.OutCaps, sizeof(WAVEOUTCAPSW));
    }

    return STATUS_SUCCESS;
}

