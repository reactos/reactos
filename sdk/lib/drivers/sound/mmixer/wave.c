/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            lib/drivers/sound/mmixer/wave.c
 * PURPOSE:         Wave Handling Functions
 * PROGRAMMER:      Johannes Anderwald
 */

#include "precomp.h"

#define YDEBUG
#include <debug.h>

const GUID KSPROPSETID_Connection               = {0x1D58C920L, 0xAC9B, 0x11CF, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};
const GUID KSDATAFORMAT_SPECIFIER_WAVEFORMATEX  = {0x05589f81L, 0xc356, 0x11ce, {0xbf, 0x01, 0x00, 0xaa, 0x00, 0x55, 0x59, 0x5a}};
const GUID KSDATAFORMAT_SUBTYPE_PCM             = {0x00000001L, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
const GUID KSDATAFORMAT_TYPE_AUDIO              = {0x73647561L, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
const GUID KSINTERFACESETID_Standard            = {0x1A8766A0L, 0x62CE, 0x11CF, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};
const GUID KSMEDIUMSETID_Standard               = {0x4747B320L, 0x62CE, 0x11CF, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};

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

PKSPIN_CONNECT
MMixerAllocatePinConnect(
    IN PMIXER_CONTEXT MixerContext,
    ULONG DataFormatSize)
{
    return MixerContext->Alloc(sizeof(KSPIN_CONNECT) + DataFormatSize);
}

MIXER_STATUS
MMixerGetWaveInfoByIndexAndType(
    IN  PMIXER_LIST MixerList,
    IN  ULONG DeviceIndex,
    IN  ULONG bWaveInType,
    OUT LPWAVE_INFO *OutWaveInfo)
{
    ULONG Index = 0;
    PLIST_ENTRY Entry, ListHead;
    LPWAVE_INFO WaveInfo;

    if (bWaveInType)
        ListHead = &MixerList->WaveInList;
    else
        ListHead = &MixerList->WaveOutList;

    /* get first entry */
    Entry = ListHead->Flink;

    while(Entry != ListHead)
    {
        WaveInfo = (LPWAVE_INFO)CONTAINING_RECORD(Entry, WAVE_INFO, Entry);

        if (Index == DeviceIndex)
        {
            *OutWaveInfo = WaveInfo;
            return MM_STATUS_SUCCESS;
        }
        Index++;
        Entry = Entry->Flink;
    }

    return MM_STATUS_INVALID_PARAMETER;
}




VOID
MMixerInitializeDataFormat(
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


MIXER_STATUS
MMixerGetAudioPinDataRanges(
    IN PMIXER_CONTEXT MixerContext,
    IN HANDLE hDevice,
    IN ULONG PinId,
    IN OUT PKSMULTIPLE_ITEM * OutMultipleItem)
{
    KSP_PIN PinProperty;
    ULONG BytesReturned = 0;
    MIXER_STATUS Status;
    PKSMULTIPLE_ITEM MultipleItem;

    /* retrieve size of data ranges buffer */
    PinProperty.Reserved = 0;
    PinProperty.PinId = PinId;
    PinProperty.Property.Set = KSPROPSETID_Pin;
    PinProperty.Property.Id = KSPROPERTY_PIN_DATARANGES;
    PinProperty.Property.Flags = KSPROPERTY_TYPE_GET;

    Status = MixerContext->Control(hDevice, IOCTL_KS_PROPERTY, (PVOID)&PinProperty, sizeof(KSP_PIN), (PVOID)NULL, 0, &BytesReturned);
    if (Status != MM_STATUS_MORE_ENTRIES)
    {
        return Status;
    }

    MultipleItem = MixerContext->Alloc(BytesReturned);
    if (!MultipleItem)
    {
        /* not enough memory */
        return MM_STATUS_NO_MEMORY;
    }

    Status = MixerContext->Control(hDevice, IOCTL_KS_PROPERTY, (PVOID)&PinProperty, sizeof(KSP_PIN), (PVOID)MultipleItem, BytesReturned, &BytesReturned);
    if (Status != MM_STATUS_SUCCESS)
    {
        /* failed */
        MixerContext->Free(MultipleItem);
        return Status;
    }

    /* save result */
    *OutMultipleItem = MultipleItem;
    return Status;
}

MIXER_STATUS
MMixerFindAudioDataRange(
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
                return MM_STATUS_SUCCESS;
            }
        }
        DataRange = (PKSDATARANGE)((ULONG_PTR)DataRange + DataRange->FormatSize);
    }
    return MM_STATUS_UNSUCCESSFUL;
}

MIXER_STATUS
MMixerOpenWavePin(
    IN PMIXER_CONTEXT MixerContext,
    IN PMIXER_LIST MixerList,
    IN ULONG DeviceId,
    IN ULONG PinId,
    IN LPWAVEFORMATEX WaveFormatEx,
    IN ACCESS_MASK DesiredAccess,
    IN PIN_CREATE_CALLBACK CreateCallback,
    IN PVOID Context,
    OUT PHANDLE PinHandle)
{
    PKSPIN_CONNECT PinConnect;
    PKSDATAFORMAT_WAVEFORMATEX DataFormat;
    LPMIXER_DATA MixerData;
    NTSTATUS Status;
    MIXER_STATUS MixerStatus;

    MixerData = MMixerGetDataByDeviceId(MixerList, DeviceId);
    if (!MixerData)
        return MM_STATUS_INVALID_PARAMETER;

    /* allocate pin connect */
    PinConnect = MMixerAllocatePinConnect(MixerContext, sizeof(KSDATAFORMAT_WAVEFORMATEX));
    if (!PinConnect)
    {
        /* no memory */
        return MM_STATUS_NO_MEMORY;
    }

    /* initialize pin connect struct */
    MMixerInitializePinConnect(PinConnect, PinId);

    /* get offset to dataformat */
    DataFormat = (PKSDATAFORMAT_WAVEFORMATEX) (PinConnect + 1);
    /* initialize with requested wave format */
    MMixerInitializeDataFormat(DataFormat, WaveFormatEx);

    if (CreateCallback)
    {
        /* let the callback handle the creation */
        MixerStatus = CreateCallback(Context, DeviceId, PinId, MixerData->hDevice, PinConnect, DesiredAccess, PinHandle);
    }
    else
    {
        /* now create the pin */
        Status = KsCreatePin(MixerData->hDevice, PinConnect, DesiredAccess, PinHandle);

        /* normalize status */
        if (Status == STATUS_SUCCESS)
            MixerStatus = MM_STATUS_SUCCESS;
        else
            MixerStatus = MM_STATUS_UNSUCCESSFUL;
    }

    /* free create info */
    MixerContext->Free(PinConnect);

    /* done */
    return MixerStatus;
}

VOID
MMixerCheckFormat(
    IN PKSDATARANGE_AUDIO DataRangeAudio,
    IN LPWAVE_INFO WaveInfo,
    IN ULONG bInput)
{
    ULONG Index, SampleFrequency;
    ULONG Result = 0;

    for(Index = 0; Index < AUDIO_TEST_RANGE; Index++)
    {
        SampleFrequency = TestRange[Index].SampleRate;

        if (DataRangeAudio->MinimumSampleFrequency <= SampleFrequency && DataRangeAudio->MaximumSampleFrequency >= SampleFrequency)
        {
            /* the audio adapter supports the sample frequency */
            if (DataRangeAudio->MinimumBitsPerSample <= 8 && DataRangeAudio->MaximumBitsPerSample >= 8)
            {
                Result |= TestRange[Index].Bit8Mono;

                if (DataRangeAudio->MaximumChannels > 1)
                {
                    /* check if pin supports the sample rate in 8-Bit Stereo */
                    Result |= TestRange[Index].Bit8Stereo;
                }
            }

            if (DataRangeAudio->MinimumBitsPerSample <= 16 && DataRangeAudio->MaximumBitsPerSample >= 16)
            {
                /* check if pin supports the sample rate in 16-Bit Mono */
                Result |= TestRange[Index].Bit16Mono;


                if (DataRangeAudio->MaximumChannels > 1)
                {
                    /* check if pin supports the sample rate in 16-Bit Stereo */
                    Result |= TestRange[Index].Bit16Stereo;
                }
            }
        }
    }


    if (bInput)
        WaveInfo->u.InCaps.dwFormats = Result;
    else
        WaveInfo->u.OutCaps.dwFormats = Result;

    DPRINT("Format %lx bInput %u\n", Result, bInput);
}

MIXER_STATUS
MMixerInitializeWaveInfo(
    IN PMIXER_CONTEXT MixerContext,
    IN PMIXER_LIST MixerList,
    IN LPMIXER_DATA MixerData,
    IN LPWSTR DeviceName,
    IN ULONG bWaveIn,
    IN ULONG PinCount,
    IN PULONG Pins)
{
    MIXER_STATUS Status;
    PKSMULTIPLE_ITEM MultipleItem;
    PKSDATARANGE_AUDIO DataRangeAudio;
    LPWAVE_INFO WaveInfo;

    WaveInfo = (LPWAVE_INFO)MixerContext->Alloc(sizeof(WAVE_INFO));
    if (!WaveInfo)
        return MM_STATUS_NO_MEMORY;

    if (PinCount > 1)
    {
        /* FIXME support multiple pins for wave device */
        DPRINT1("Implement support for multiple pins\n");
        //ASSERT(PinCount == 1);
    }

    /* initialize wave info */
    WaveInfo->DeviceId = MixerData->DeviceId;
    WaveInfo->PinId = Pins[0];

    /* sanity check */
    ASSERT(wcslen(DeviceName) < MAXPNAMELEN);

    /* copy device name */
    if (bWaveIn)
    {
        wcscpy(WaveInfo->u.InCaps.szPname, DeviceName);
    }
    else
    {
        wcscpy(WaveInfo->u.OutCaps.szPname, DeviceName);
    }

    /* FIXME determine manufacturer / product id */
    if (bWaveIn)
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

    /* get audio pin data ranges */
    Status = MMixerGetAudioPinDataRanges(MixerContext, MixerData->hDevice, Pins[0], &MultipleItem);
    if (Status != MM_STATUS_SUCCESS)
    {
        /* failed to get audio pin data ranges */
        MixerContext->Free(WaveInfo);
        return MM_STATUS_UNSUCCESSFUL;
    }

    /* find an KSDATARANGE_AUDIO range */
    Status = MMixerFindAudioDataRange(MultipleItem, &DataRangeAudio);
    if (Status != MM_STATUS_SUCCESS)
    {
        /* failed to find audio pin data range */
        MixerContext->Free(MultipleItem);
        MixerContext->Free(WaveInfo);
        return MM_STATUS_UNSUCCESSFUL;
    }

    /* store channel count */
    if (bWaveIn)
    {
        WaveInfo->u.InCaps.wChannels = DataRangeAudio->MaximumChannels;
    }
    else
    {
       WaveInfo->u.OutCaps.wChannels = DataRangeAudio->MaximumChannels;
    }

    /* get all supported formats */
    MMixerCheckFormat(DataRangeAudio, WaveInfo, bWaveIn);

    /* free dataranges buffer */
    MixerContext->Free(MultipleItem);

    if (bWaveIn)
    {
        InsertTailList(&MixerList->WaveInList, &WaveInfo->Entry);
        MixerList->WaveInListCount++;
    }
    else
    {
        InsertTailList(&MixerList->WaveOutList, &WaveInfo->Entry);
        MixerList->WaveOutListCount++;
    }

    return MM_STATUS_SUCCESS;
}

MIXER_STATUS
MMixerOpenWave(
    IN PMIXER_CONTEXT MixerContext,
    IN ULONG DeviceIndex,
    IN ULONG bWaveIn,
    IN LPWAVEFORMATEX WaveFormat,
    IN PIN_CREATE_CALLBACK CreateCallback,
    IN PVOID Context,
    OUT PHANDLE PinHandle)
{
    PMIXER_LIST MixerList;
    MIXER_STATUS Status;
    LPWAVE_INFO WaveInfo;
    ACCESS_MASK DesiredAccess = 0;

    /* verify mixer context */
    Status = MMixerVerifyContext(MixerContext);

    if (Status != MM_STATUS_SUCCESS)
    {
        /* invalid context passed */
        return Status;
    }

    /* grab mixer list */
    MixerList = (PMIXER_LIST)MixerContext->MixerContext;

    if (WaveFormat->wFormatTag != WAVE_FORMAT_PCM)
    {
        /* not implemented */
        return MM_STATUS_NOT_IMPLEMENTED;
    }

    /* find destination wave */
    Status = MMixerGetWaveInfoByIndexAndType(MixerList, DeviceIndex, bWaveIn, &WaveInfo);
    if (Status != MM_STATUS_SUCCESS)
    {
        /* failed to find wave info */
        return MM_STATUS_INVALID_PARAMETER;
    }

    /* get desired access */
    if (bWaveIn)
    {
        DesiredAccess |= GENERIC_READ;
    }
     else
    {
        DesiredAccess |= GENERIC_WRITE;
    }

    /* now try open the pin */
    return MMixerOpenWavePin(MixerContext, MixerList, WaveInfo->DeviceId, WaveInfo->PinId, WaveFormat, DesiredAccess, CreateCallback, Context, PinHandle);
}

MIXER_STATUS
MMixerWaveInCapabilities(
    IN PMIXER_CONTEXT MixerContext,
    IN ULONG DeviceIndex,
    OUT LPWAVEINCAPSW Caps)
{
    PMIXER_LIST MixerList;
    MIXER_STATUS Status;
    LPWAVE_INFO WaveInfo;

    /* verify mixer context */
    Status = MMixerVerifyContext(MixerContext);

    if (Status != MM_STATUS_SUCCESS)
    {
        /* invalid context passed */
        return Status;
    }

    /* grab mixer list */
    MixerList = (PMIXER_LIST)MixerContext->MixerContext;

    /* find destination wave */
    Status = MMixerGetWaveInfoByIndexAndType(MixerList, DeviceIndex, TRUE, &WaveInfo);
    if (Status != MM_STATUS_SUCCESS)
    {
        /* failed to find wave info */
        return MM_STATUS_UNSUCCESSFUL;
    }

    /* copy capabilities */
    MixerContext->Copy(Caps, &WaveInfo->u.InCaps, sizeof(WAVEINCAPSW));

    return MM_STATUS_SUCCESS;
}

MIXER_STATUS
MMixerWaveOutCapabilities(
    IN PMIXER_CONTEXT MixerContext,
    IN ULONG DeviceIndex,
    OUT LPWAVEOUTCAPSW Caps)
{
    PMIXER_LIST MixerList;
    MIXER_STATUS Status;
    LPWAVE_INFO WaveInfo;

    /* verify mixer context */
    Status = MMixerVerifyContext(MixerContext);

    if (Status != MM_STATUS_SUCCESS)
    {
        /* invalid context passed */
        return Status;
    }

    /* grab mixer list */
    MixerList = (PMIXER_LIST)MixerContext->MixerContext;

    /* find destination wave */
    Status = MMixerGetWaveInfoByIndexAndType(MixerList, DeviceIndex, FALSE, &WaveInfo);
    if (Status != MM_STATUS_SUCCESS)
    {
        /* failed to find wave info */
        return MM_STATUS_UNSUCCESSFUL;
    }

    /* copy capabilities */
    MixerContext->Copy(Caps, &WaveInfo->u.OutCaps, sizeof(WAVEOUTCAPSW));

    return MM_STATUS_SUCCESS;
}

ULONG
MMixerGetWaveInCount(
    IN PMIXER_CONTEXT MixerContext)
{
    PMIXER_LIST MixerList;
    MIXER_STATUS Status;

     /* verify mixer context */
    Status = MMixerVerifyContext(MixerContext);

    if (Status != MM_STATUS_SUCCESS)
    {
        /* invalid context passed */
        return Status;
    }

    /* grab mixer list */
    MixerList = (PMIXER_LIST)MixerContext->MixerContext;

    return MixerList->WaveInListCount;
}

ULONG
MMixerGetWaveOutCount(
    IN PMIXER_CONTEXT MixerContext)
{
    PMIXER_LIST MixerList;
    MIXER_STATUS Status;

    /* verify mixer context */
    Status = MMixerVerifyContext(MixerContext);

    if (Status != MM_STATUS_SUCCESS)
    {
        /* invalid context passed */
        return Status;
    }

    /* grab mixer list */
    MixerList = (PMIXER_LIST)MixerContext->MixerContext;

    return MixerList->WaveOutListCount;
}

MIXER_STATUS
MMixerSetWaveStatus(
    IN PMIXER_CONTEXT MixerContext,
    IN HANDLE PinHandle,
    IN KSSTATE State)
{
    KSPROPERTY Property;
    ULONG Length;
    MIXER_STATUS Status;

    /* verify mixer context */
    Status = MMixerVerifyContext(MixerContext);

    if (Status != MM_STATUS_SUCCESS)
    {
        /* invalid context passed */
        return Status;
    }

    /* setup property request */
    Property.Set = KSPROPSETID_Connection;
    Property.Id = KSPROPERTY_CONNECTION_STATE;
    Property.Flags = KSPROPERTY_TYPE_SET;

    return MixerContext->Control(PinHandle, IOCTL_KS_PROPERTY, &Property, sizeof(KSPROPERTY), &State, sizeof(KSSTATE), &Length);
}

MIXER_STATUS
MMixerSetWaveResetState(
    IN PMIXER_CONTEXT MixerContext,
    IN HANDLE PinHandle,
    IN ULONG bBegin)
{
    ULONG Length;
    MIXER_STATUS Status;
    KSRESET Reset;

    /* verify mixer context */
    Status = MMixerVerifyContext(MixerContext);

    if (Status != MM_STATUS_SUCCESS)
    {
        /* invalid context passed */
        return Status;
    }

    /* begin / stop reset */
    Reset = (bBegin ? KSRESET_BEGIN : KSRESET_END);

    return MixerContext->Control(PinHandle, IOCTL_KS_RESET_STATE, &Reset, sizeof(KSRESET), NULL, 0, &Length);
}

MIXER_STATUS
MMixerGetWaveDevicePath(
    IN PMIXER_CONTEXT MixerContext,
    IN ULONG bWaveIn,
    IN ULONG DeviceId,
    OUT LPWSTR * DevicePath)
{
    PMIXER_LIST MixerList;
    LPMIXER_DATA MixerData;
    LPWAVE_INFO WaveInfo;
    SIZE_T Length;
    MIXER_STATUS Status;

    /* verify mixer context */
    Status = MMixerVerifyContext(MixerContext);

    if (Status != MM_STATUS_SUCCESS)
    {
        /* invalid context passed */
        return Status;
    }

    /* grab mixer list */
    MixerList = (PMIXER_LIST)MixerContext->MixerContext;

    /* find destination wave */
    Status = MMixerGetWaveInfoByIndexAndType(MixerList, DeviceId, bWaveIn, &WaveInfo);
    if (Status != MM_STATUS_SUCCESS)
    {
        /* failed to find wave info */
        return MM_STATUS_INVALID_PARAMETER;
    }

    /* get associated device id */
    MixerData = MMixerGetDataByDeviceId(MixerList, WaveInfo->DeviceId);
    if (!MixerData)
        return MM_STATUS_INVALID_PARAMETER;

    /* calculate length */
    Length = wcslen(MixerData->DeviceName)+1;

    /* allocate destination buffer */
    *DevicePath = MixerContext->Alloc(Length * sizeof(WCHAR));

    if (!*DevicePath)
    {
        /* no memory */
        return MM_STATUS_NO_MEMORY;
    }

    /* copy device path */
    MixerContext->Copy(*DevicePath, MixerData->DeviceName, Length * sizeof(WCHAR));

    /* done */
    return MM_STATUS_SUCCESS;
}
