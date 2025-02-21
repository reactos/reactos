/*
 * PROJECT:         ReactOS HDAudio Driver
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         WaveRT miniport
 * COPYRIGHT:       Copyright 2025 Johannes Anderwald <johannes.anderwald@reactos.org>
 *                  Copyright 2025-2026 Oleg Dubinskiy <oleg.dubinskiy@reactos.org>
 */

#include "private.h"

#define NDEBUG
#include <debug.h>

NTSTATUS
NTAPI
CMiniportWaveRT::QueryInterface(IN REFIID refiid, OUT PVOID *Output)
{
    UNICODE_STRING GuidString;

    if (IsEqualGUIDAligned(refiid, IID_IMiniport) ||
        IsEqualGUIDAligned(refiid, IID_IUnknown) ||
        IsEqualGUIDAligned(refiid, IID_IMiniportWaveRT))
    {
        *Output = PVOID(PMINIPORTWAVERT(this));
        PUNKNOWN(*Output)->AddRef();
        return STATUS_SUCCESS;
    }

    if (RtlStringFromGUID(refiid, &GuidString) == STATUS_SUCCESS)
    {
        DPRINT1("CMiniportWaveRT::QueryInterface no interface!!! iface %S\n", GuidString.Buffer);
        RtlFreeUnicodeString(&GuidString);
    }
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
CMiniportWaveRT::GetDescription(OUT PPCFILTER_DESCRIPTOR* Description)
{
    *Description = m_FilterDescription;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CMiniportWaveRT::DataRangeIntersection(
    IN ULONG PinId,
    IN PKSDATARANGE DataRange,
    IN PKSDATARANGE MatchingDataRange,
    IN ULONG OutputBufferLength,
    OUT PVOID ResultantFormat OPTIONAL,
    OUT PULONG ResultantFormatLength)
{
#if 0
    if (!OutputBufferLength || !ResultantFormat)
    {
        *ResultantFormatLength = sizeof(KSDATAFORMAT) + sizeof(WAVEFORMATEXTENSIBLE);
        return STATUS_BUFFER_OVERFLOW;
    }

    if (OutputBufferLength < (sizeof(KSDATAFORMAT) + sizeof(WAVEFORMATEXTENSIBLE)))
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    *(PKSDATAFORMAT)ResultantFormat = *DataRange;

    ((PKSDATAFORMAT)ResultantFormat)->FormatSize = sizeof(KSDATAFORMAT) + sizeof(WAVEFORMATEXTENSIBLE);

    PWAVEFORMATEXTENSIBLE WaveFormat = (PWAVEFORMATEXTENSIBLE)((PKSDATAFORMAT)ResultantFormat + 1);

    WaveFormat->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;

    PKSDATARANGE_AUDIO TargetRange = (PKSDATARANGE_AUDIO)DataRange;
    PKSDATARANGE_AUDIO MatchingRange = (PKSDATARANGE_AUDIO)MatchingDataRange;

    WaveFormat->Format.nChannels = (WORD)TargetRange->MaximumChannels;
    if (WaveFormat->Format.nChannels > MatchingRange->MaximumChannels)
        WaveFormat->Format.nChannels = (WORD)MatchingRange->MaximumChannels;
    WaveFormat->Format.nSamplesPerSec = TargetRange->MaximumSampleFrequency;
    if (WaveFormat->Format.nSamplesPerSec > MatchingRange->MaximumSampleFrequency)
        WaveFormat->Format.nSamplesPerSec = MatchingRange->MaximumSampleFrequency;
    WaveFormat->Format.wBitsPerSample = (WORD)TargetRange->MaximumBitsPerSample;
    if (WaveFormat->Format.wBitsPerSample > MatchingRange->MaximumBitsPerSample)
        WaveFormat->Format.wBitsPerSample = (WORD)MatchingRange->MaximumBitsPerSample;
    WaveFormat->Format.nBlockAlign = (WaveFormat->Format.wBitsPerSample * WaveFormat->Format.nChannels) / 8;
    WaveFormat->Format.nAvgBytesPerSec = WaveFormat->Format.nSamplesPerSec * WaveFormat->Format.nBlockAlign;
    WaveFormat->Format.cbSize = 22;
    WaveFormat->Samples.wValidBitsPerSample = WaveFormat->Format.wBitsPerSample;

    switch (WaveFormat->Format.nChannels)
    {
        case 1:
            WaveFormat->dwChannelMask = KSAUDIO_SPEAKER_MONO;
            break;
        case 2:
            WaveFormat->dwChannelMask = KSAUDIO_SPEAKER_STEREO;
            break;
        case 4:
            WaveFormat->dwChannelMask = KSAUDIO_SPEAKER_QUAD;
            break;
        case 6:
            WaveFormat->dwChannelMask = KSAUDIO_SPEAKER_5POINT1;
            break;
        case 8:
            WaveFormat->dwChannelMask = KSAUDIO_SPEAKER_7POINT1;
            break;
        default:
            DPRINT1("Unhandled dwChannelMask for %u nChannels\n", WaveFormat->Format.nChannels);
            WaveFormat->dwChannelMask = KSAUDIO_SPEAKER_STEREO;
            break;
    }

    WaveFormat->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;

    ((PKSDATAFORMAT)ResultantFormat)->SampleSize = WaveFormat->Format.nBlockAlign;

    *ResultantFormatLength = sizeof(KSDATAFORMAT) + sizeof(WAVEFORMATEXTENSIBLE);

    DPRINT1("DataRangeIntersection Frequency: %u, Channels: %u, bps: %u, ChannelMask: %x\n",
            WaveFormat->Format.nSamplesPerSec, WaveFormat->Format.nChannels,
            WaveFormat->Format.wBitsPerSample, WaveFormat->dwChannelMask);

    return STATUS_SUCCESS;
#else
    return STATUS_NOT_IMPLEMENTED;
#endif
}

NTSTATUS
NTAPI
CMiniportWaveRT::Init(
    IN PUNKNOWN UnknownAdapter,
    IN PRESOURCELIST ResourceList,
    IN PPORTWAVERT Port)
{
    m_Port = Port;
    m_Port->AddRef();
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CMiniportWaveRT::NewStream(
    OUT PMINIPORTWAVERTSTREAM* Stream,
    IN PPORTWAVERTSTREAM PortStream,
    IN ULONG Pin,
    IN BOOLEAN Capture,
    IN PKSDATAFORMAT DataFormat)
{
    return HDAUDIO_AllocateStream(Stream, m_Adapter, m_Node, Pin, Capture, DataFormat, m_AssociatedPinCount, m_AssociatedPins, m_FilterDescription); 
}

NTSTATUS
NTAPI
CMiniportWaveRT::GetDeviceDescription(
    OUT PDEVICE_DESCRIPTION DeviceDescription)
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

NTSTATUS
HDAUDIO_NewMiniportWaveRT(
    OUT PMINIPORTWAVERT* OutMiniport,
    IN ULONG AssociatedPinsCount,
    IN PULONG AssociatedPinIds,
    IN CFunctionGroupNode * Node,
    IN CAdapterCommon * Adapter,
    IN PPCFILTER_DESCRIPTOR FilterDescription)
{
    CMiniportWaveRT *This;

    This = new (NonPagedPool, TAG_HDAUDIO) CMiniportWaveRT(NULL, AssociatedPinsCount, AssociatedPinIds, Node, Adapter, FilterDescription);
    if (!This)
    {
        // out of memory
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // add reference
    This->AddRef();

    // return result
    *OutMiniport = This;

    // done
    return STATUS_SUCCESS;
}
