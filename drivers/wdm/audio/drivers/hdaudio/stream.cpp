/*
 * PROJECT:         ReactOS HDAudio Driver
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         WaveRT streaming
 * COPYRIGHT:       Copyright 2025 Johannes Anderwald <johannes.anderwald@reactos.org>
 *                  Copyright 2025-2026 Oleg Dubinskiy <oleg.dubinskiy@reactos.org>
 */

#include "private.h"

#define NDEBUG
#include <debug.h>

NTSTATUS
NTAPI
CMiniportWaveRTStream::QueryInterface(IN REFIID refiid, OUT PVOID *Output)
{
    UNICODE_STRING GuidString;

    if (IsEqualGUIDAligned(refiid, IID_IMiniportWaveRTStreamNotification) ||
        IsEqualGUIDAligned(refiid, IID_IUnknown) ||
        IsEqualGUIDAligned(refiid, IID_IMiniportWaveRTStream))
    {
        *Output = PVOID(PMINIPORTWAVERTSTREAM(this));
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
CMiniportWaveRTStream::AllocateAudioBuffer(
    ULONG RequestedBufferSize,
    PMDL* AudioBufferMdl,
    ULONG* ActualSize,
    ULONG* OffsetFromFirstPage,
    MEMORY_CACHING_TYPE* CacheType)
{
    *CacheType = MmWriteCombined;
    return m_Interface.AllocateDmaBufferWithNotification(
        m_Interface.Context, m_DmaEngine, 1, RequestedBufferSize, AudioBufferMdl, (PSIZE_T)ActualSize, (PSIZE_T)OffsetFromFirstPage,
        &m_StreamId, &m_FifoSize);
}

NTSTATUS
NTAPI
CMiniportWaveRTStream::GetPositionRegister(
    OUT KSRTAUDIO_HWREGISTER* Register)
{
    Register->Width = 32;
    return m_Interface.GetLinkPositionRegister(m_Interface.Context, m_DmaEngine, (PULONG*)&Register->Register);
}

NTSTATUS
NTAPI
CMiniportWaveRTStream::GetClockRegister(
    OUT KSRTAUDIO_HWREGISTER* Register)
{
    Register->Width = 32;
    m_Interface.GetWallClockRegister(m_Interface.Context, (PULONG*)&Register->Register);
    return STATUS_SUCCESS;
}

VOID
NTAPI
CMiniportWaveRTStream::GetHWLatency(
    IN KSRTAUDIO_HWLATENCY* hwLatency)
{
    hwLatency->FifoSize = m_FifoSize;
    //hwLatency->ChipsetDelay // FIXME
    //hwLatency->CodecDelay // FIXME
}

VOID
NTAPI
CMiniportWaveRTStream::FreeAudioBuffer(
    PMDL AudioBufferMdl,
    ULONG BufferSize)
{
    HANDLE Handles[1] = {m_DmaEngine};
    m_Interface.SetDmaEngineState(m_Interface.Context, ResetState, 1, Handles);
    m_Interface.FreeDmaBuffer(m_Interface.Context, m_DmaEngine);
}

NTSTATUS
NTAPI
CMiniportWaveRTStream::GetPosition(
    OUT PKSAUDIO_POSITION Position)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CMiniportWaveRTStream::SetFormat(
    IN PKSDATAFORMAT DataFormat)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CMiniportWaveRTStream::SetState(
    IN KSSTATE State)
{
    NTSTATUS Status = STATUS_SUCCESS;
    HANDLE Handles[1] = {m_DmaEngine};
    if (State == KSSTATE_ACQUIRE)
    {
        Status = m_Interface.SetDmaEngineState(m_Interface.Context, ResetState, 1, Handles);
        DPRINT1("SetDmaEngineState State %x Status %x\n", State, Status);
    }
    else if (State == KSSTATE_PAUSE || State == KSSTATE_STOP)
    {
        Status = m_Interface.SetDmaEngineState(m_Interface.Context, PauseState, 1, Handles);
        DPRINT1("SetDmaEngineState State %x Status %x\n", State, Status);
    }
    else if (State == KSSTATE_RUN)
    {
        for (ULONG Index = 0; Index < m_NodeCount; Index++)
        {
            Status = m_OutNode->SetStreamFormat(m_Nodes[Index], m_Converter.ConverterFormat);
            DPRINT1("Node %u SetStreamFormat Status %x\n", m_Nodes[Index], Status);
        }
        for (ULONG Index = 0; Index < m_NodeCount; Index++)
        {
            Status = m_OutNode->SetConverterStream(m_Nodes[Index], m_StreamId);
            DPRINT1("Node %u SetConverterStream Status %x\n", m_Nodes[Index], Status);
        }
        Status = m_Interface.SetDmaEngineState(m_Interface.Context, RunState, 1, Handles);
    }
    return Status;
}

NTSTATUS
NTAPI
CMiniportWaveRTStream::AllocateBufferWithNotification(
    ULONG NotificationCount,
    ULONG RequestedBufferSize,
    PMDL* AudioBufferMdl,
    ULONG* ActualSize,
    ULONG* OffsetFromFirstPage,
    MEMORY_CACHING_TYPE* CacheType)
{
    *CacheType = MmWriteCombined;
    return m_Interface.AllocateDmaBufferWithNotification(
        m_Interface.Context, m_DmaEngine, NotificationCount, RequestedBufferSize, AudioBufferMdl, (PSIZE_T)ActualSize,
        (PSIZE_T)OffsetFromFirstPage, &m_StreamId, &m_FifoSize);
}

NTSTATUS
NTAPI
CMiniportWaveRTStream::RegisterNotificationEvent(
    PKEVENT NotificationEvent)
{
    return m_Interface.RegisterNotificationEvent(
        m_Interface.Context,
        m_DmaEngine,
        NotificationEvent
    );
}

NTSTATUS
NTAPI
CMiniportWaveRTStream::UnregisterNotificationEvent(PKEVENT NotificationEvent)
{
    return m_Interface.UnregisterNotificationEvent(m_Interface.Context, m_DmaEngine, NotificationEvent);
}

VOID
CMiniportWaveRTStream::FreeBufferWithNotification(PMDL AudioBufferMdl, ULONG BufferSize)
{
    HANDLE Handles[1] = {m_DmaEngine};
    m_Interface.SetDmaEngineState(m_Interface.Context, ResetState, 1, Handles);
    m_Interface.FreeDmaBufferWithNotification(m_Interface.Context, m_DmaEngine, AudioBufferMdl, BufferSize);
}

NTSTATUS
HDAUDIO_ValidateDataFormat(
    IN PKSDATAFORMAT DataFormat,
    IN PKSDATARANGE_AUDIO PinRange,
    OUT PHDAUDIO_STREAM_FORMAT StreamFormat OPTIONAL)
{
    PKSDATAFORMAT_WAVEFORMATEX DataFormatWave;
    PWAVEFORMATEX WaveFormat;
    ULONG ExpectedFormatSize;
    ULONG ContainerBits;
    ULONG ValidBits;
    ULONG BlockAlign;

    if (!DataFormat || !PinRange ||
        !IsEqualGUIDAligned(DataFormat->MajorFormat, KSDATAFORMAT_TYPE_AUDIO) ||
        !IsEqualGUIDAligned(DataFormat->SubFormat, KSDATAFORMAT_SUBTYPE_PCM) ||
        !IsEqualGUIDAligned(DataFormat->Specifier, KSDATAFORMAT_SPECIFIER_WAVEFORMATEX) ||
        PinRange->DataRange.FormatSize < sizeof(KSDATARANGE_AUDIO))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (DataFormat->FormatSize < sizeof(KSDATAFORMAT_WAVEFORMATEX))
        return STATUS_INVALID_PARAMETER;

    DataFormatWave = (PKSDATAFORMAT_WAVEFORMATEX)DataFormat;
    WaveFormat = &DataFormatWave->WaveFormatEx;
    ContainerBits = WaveFormat->wBitsPerSample;

    if (WaveFormat->wFormatTag == WAVE_FORMAT_PCM)
    {
        ExpectedFormatSize = sizeof(KSDATAFORMAT_WAVEFORMATEX);
        if (WaveFormat->cbSize)
            return STATUS_INVALID_PARAMETER;
        ValidBits = ContainerBits;
    }
    else if (WaveFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
    {
        PWAVEFORMATEXTENSIBLE Extensible;

        ExpectedFormatSize = sizeof(KSDATAFORMAT) + sizeof(WAVEFORMATEXTENSIBLE);
        if (DataFormat->FormatSize != ExpectedFormatSize ||
            WaveFormat->cbSize !=
                sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX))
        {
            return STATUS_INVALID_PARAMETER;
        }

        Extensible = (PWAVEFORMATEXTENSIBLE)WaveFormat;
        if (!IsEqualGUIDAligned(Extensible->SubFormat,
                                KSDATAFORMAT_SUBTYPE_PCM))
        {
            return STATUS_NOT_SUPPORTED;
        }
        ValidBits = Extensible->Samples.wValidBitsPerSample;
    }
    else
    {
        return STATUS_NOT_SUPPORTED;
    }

    if (DataFormat->FormatSize != ExpectedFormatSize ||
        !WaveFormat->nChannels ||
        WaveFormat->nChannels > PinRange->MaximumChannels ||
        WaveFormat->nSamplesPerSec < PinRange->MinimumSampleFrequency ||
        WaveFormat->nSamplesPerSec > PinRange->MaximumSampleFrequency ||
        !ContainerBits ||
        (ContainerBits & 7) ||
        !ValidBits ||
        ValidBits > ContainerBits ||
        ValidBits < PinRange->MinimumBitsPerSample ||
        ValidBits > PinRange->MaximumBitsPerSample)
    {
        return STATUS_NOT_SUPPORTED;
    }

    BlockAlign = WaveFormat->nChannels * (ContainerBits / 8);
    if (!BlockAlign || BlockAlign > MAXUSHORT ||
        WaveFormat->nBlockAlign != BlockAlign ||
        WaveFormat->nSamplesPerSec > MAXULONG / BlockAlign ||
        WaveFormat->nAvgBytesPerSec !=
            WaveFormat->nSamplesPerSec * BlockAlign ||
        (DataFormat->SampleSize && DataFormat->SampleSize != BlockAlign))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (StreamFormat)
    {
        StreamFormat->SampleRate = WaveFormat->nSamplesPerSec;
        StreamFormat->NumberOfChannels = WaveFormat->nChannels;
        StreamFormat->ValidBitsPerSample = (USHORT)ValidBits;
        StreamFormat->ContainerSize = (USHORT)ContainerBits;
    }

    return STATUS_SUCCESS;
}

static BOOLEAN
HDAUDIO_IsSampleRateSupported(
    IN PNODE_PCM_RATES Rates,
    IN ULONG SampleRate)
{
    switch (SampleRate)
    {
        case 8000:   return Rates->Supported8Khz;
        case 11025:  return Rates->Supported11Khz;
        case 16000:  return Rates->Supported16Khz;
        case 22050:  return Rates->Supported22Khz;
        case 32000:  return Rates->Supported32Khz;
        case 44100:  return Rates->Supported44Khz;
        case 48000:  return Rates->Supported48Khz;
        case 88200:  return Rates->Supported88Khz;
        case 96000:  return Rates->Supported96Khz;
        case 176400: return Rates->Supported176Khz;
        case 192000: return Rates->Supported192Khz;
        case 384000: return Rates->Supported384Khz;
        default:     return FALSE;
    }
}

static BOOLEAN
HDAUDIO_IsSampleSizeSupported(
    IN PNODE_PCM_RATES Rates,
    IN ULONG ValidBits)
{
    switch (ValidBits)
    {
        case 8:  return Rates->AudioFormatSupported8Bit;
        case 16: return Rates->AudioFormatSupported16Bit;
        case 20: return Rates->AudioFormatSupported20Bit;
        case 24: return Rates->AudioFormatSupported24Bit;
        case 32: return Rates->AudioFormatSupported32Bit;
        default: return FALSE;
    }
}

NTSTATUS
HDAUDIO_ValidateNodeFormats(
    IN CFunctionGroupNode *Node,
    IN ULONG NodeCount,
    IN PULONG Nodes,
    IN PHDAUDIO_STREAM_FORMAT StreamFormat)
{
    NODE_PCM_RATES Rates;
    NTSTATUS Status;

    if (!Node || !NodeCount || !Nodes || !StreamFormat)
        return STATUS_INVALID_PARAMETER;

    for (ULONG Index = 0; Index < NodeCount; ++Index)
    {
        Status = Node->GetSupportedPCMSizeRates(Nodes[Index], &Rates);
        if (!NT_SUCCESS(Status))
            return Status;

        if (!Rates.PCMFormatSupported ||
            !HDAUDIO_IsSampleRateSupported(&Rates, StreamFormat->SampleRate) ||
            !HDAUDIO_IsSampleSizeSupported(&Rates,
                                           StreamFormat->ValidBitsPerSample))
        {
            return STATUS_NOT_SUPPORTED;
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
HDAUDIO_AllocateStream(
    OUT PMINIPORTWAVERTSTREAM* Stream,
    IN CAdapterCommon* Adapter,
    IN CFunctionGroupNode* Node,
    IN ULONG Pin,
    IN BOOLEAN Capture,
    IN PKSDATAFORMAT DataFormat,
    IN ULONG NodeCount,
    IN PULONG Nodes,
    IN PPCFILTER_DESCRIPTOR FilterDescription)
{
    HDAUDIO_STREAM_FORMAT StreamFormat;
    HANDLE hDmaEngine = NULL;
    HDAUDIO_CONVERTER_FORMAT Converter;
    NTSTATUS Status;
    HDAUDIO_BUS_INTERFACE_V2 Interface;

    if (!Stream || !Adapter || !Node || !DataFormat || !FilterDescription)
        return STATUS_INVALID_PARAMETER;

    if (Pin >= FilterDescription->PinCount)
        return STATUS_INVALID_PARAMETER;

    ULONG PinDataRangesCount = FilterDescription->Pins[Pin].KsPinDescriptor.DataRangesCount;
    PKSDATARANGE * PinDataRanges = (PKSDATARANGE*)FilterDescription->Pins[Pin].KsPinDescriptor.DataRanges;

    UCHAR bSupported = FALSE;
    for (ULONG Index = 0; Index < PinDataRangesCount; Index++)
    {
        PKSDATARANGE PinDataRange = PinDataRanges[Index];
        if (PinDataRange &&
            IsEqualGUIDAligned(DataFormat->MajorFormat, PinDataRange->MajorFormat) &&
            IsEqualGUIDAligned(DataFormat->SubFormat, PinDataRange->SubFormat) &&
            IsEqualGUIDAligned(DataFormat->Specifier, PinDataRange->Specifier))
        {
            // matches format
            // lets see if its pcm format
            if (IsEqualGUIDAligned(PinDataRange->MajorFormat, KSDATAFORMAT_TYPE_AUDIO) &&
                IsEqualGUIDAligned(PinDataRange->SubFormat, KSDATAFORMAT_SUBTYPE_PCM) &&
                IsEqualGUIDAligned(PinDataRange->Specifier, KSDATAFORMAT_SPECIFIER_WAVEFORMATEX))
            {
                Status = HDAUDIO_ValidateDataFormat(
                    DataFormat,
                    (PKSDATARANGE_AUDIO)PinDataRange,
                    &StreamFormat);
                if (NT_SUCCESS(Status))
                {
                    Status = HDAUDIO_ValidateNodeFormats(Node,
                                                         NodeCount,
                                                         Nodes,
                                                         &StreamFormat);
                    if (NT_SUCCESS(Status))
                    {
                        bSupported = TRUE;
                        break;
                    }
                }
            }
        }
    }
    if (!bSupported)
        return STATUS_NOT_SUPPORTED;

    Status = Adapter->GetInterface(&Interface);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    if (Capture)
    {
        Status = Interface.AllocateCaptureDmaEngine(
            Interface.Context, Adapter->GetCodecAddress(), &StreamFormat, &hDmaEngine, &Converter);
    }
    else
    {
        BOOL Stripe = Node->GetStripeBit();
        Status = Interface.AllocateRenderDmaEngine(
            Interface.Context,
            &StreamFormat,
            Stripe,
            &hDmaEngine,
            &Converter);
    }
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HDAUDIO: Failed to allocate DMA engine Status %x\n", Status);
        return Status;
    }

    CMiniportWaveRTStream *This = new (NonPagedPool, TAG_HDAUDIO)
        CMiniportWaveRTStream(NULL, Adapter, Node, Pin, Capture, &StreamFormat, &Converter, &Interface, hDmaEngine, NodeCount, Nodes, FilterDescription);
    if (!This)
    {
        // out of memory
        if (hDmaEngine)
            Interface.FreeDmaEngine(Interface.Context, hDmaEngine);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // add reference
    This->AddRef();

    // return result
    *Stream = This;
    return STATUS_SUCCESS;
}
