/*
 * PROJECT:     ReactOS VirtIO Sound Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     WaveCyclic miniport
 * COPYRIGHT:   Copyright 2026 Ralf Lang <ralf.lang@ralf-lang.de>
 */

#include "virtiosnd.h"

#include "kcom.h"
#include "virtio_snd_protocol.h"
#include <ksmedia.h>
#include <stdunk.h>

PVOID
__cdecl
operator new(size_t Size, POOL_TYPE PoolType, ULONG Tag);

#define VIRTIO_SND_POOL_TAG 'dnSV'
#define VIRTIO_SND_DMA_BUFFER_BYTES (VIRTIO_SND_PCM_BUFFER_BYTES * 4)

static KSDATARANGE_AUDIO g_VirtioSndRenderRange = {
    {sizeof(KSDATARANGE_AUDIO), 0, 0, 0, STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO), STATICGUIDOF(KSDATAFORMAT_SUBTYPE_PCM),
     STATICGUIDOF(KSDATAFORMAT_SPECIFIER_WAVEFORMATEX)},
    2,
    16,
    32,
    44100,
    96000};

struct VIRTIO_SND_WAVE_RATE
{
    ULONG SampleRate;
    ULONG RateIndex;
};

static const VIRTIO_SND_WAVE_RATE g_VirtioSndWaveRates[] = {
    {48000, VIRTIO_SND_PCM_RATE_48000},
    {44100, VIRTIO_SND_PCM_RATE_44100},
    {96000, VIRTIO_SND_PCM_RATE_96000}};

static PKSDATARANGE g_VirtioSndRenderRanges[] = {reinterpret_cast<PKSDATARANGE>(&g_VirtioSndRenderRange)};

static KSDATARANGE g_VirtioSndRenderBridgeRange = {
    sizeof(KSDATARANGE),
    0,
    0,
    0,
    STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
    STATICGUIDOF(KSDATAFORMAT_SUBTYPE_ANALOG),
    STATICGUIDOF(KSDATAFORMAT_SPECIFIER_NONE)};

static PKSDATARANGE g_VirtioSndRenderBridgeRanges[] = {&g_VirtioSndRenderBridgeRange};

static PCPIN_DESCRIPTOR g_VirtioSndRenderPins[] = {
    {1,
     1,
     0,
     NULL,
     {0, NULL, 0, NULL, SIZEOF_ARRAY(g_VirtioSndRenderRanges), g_VirtioSndRenderRanges, KSPIN_DATAFLOW_IN,
      KSPIN_COMMUNICATION_SINK, (GUID *)&KSCATEGORY_AUDIO, NULL, 0}},
    {0,
     0,
     0,
     NULL,
     {0, NULL, 0, NULL, SIZEOF_ARRAY(g_VirtioSndRenderBridgeRanges), g_VirtioSndRenderBridgeRanges, KSPIN_DATAFLOW_OUT,
      KSPIN_COMMUNICATION_NONE, (GUID *)&KSCATEGORY_AUDIO, NULL, 0}}};

static PCNODE_DESCRIPTOR g_VirtioSndRenderNodes[] = {{0, NULL, &KSNODETYPE_DAC, NULL}};

static PCCONNECTION_DESCRIPTOR g_VirtioSndRenderConnections[] = {{PCFILTER_NODE, 0, 0, 1}, {0, 0, PCFILTER_NODE, 1}};

static PCFILTER_DESCRIPTOR g_VirtioSndRenderFilter = {
    0,
    NULL,
    sizeof(PCPIN_DESCRIPTOR),
    SIZEOF_ARRAY(g_VirtioSndRenderPins),
    g_VirtioSndRenderPins,
    sizeof(PCNODE_DESCRIPTOR),
    SIZEOF_ARRAY(g_VirtioSndRenderNodes),
    g_VirtioSndRenderNodes,
    SIZEOF_ARRAY(g_VirtioSndRenderConnections),
    g_VirtioSndRenderConnections,
    0,
    NULL};

static WCHAR g_VirtioSndRenderSubdeviceName[] = L"Wave";

static NTSTATUS
VirtioSndValidateFormat(_In_ PKSDATAFORMAT DataFormat);

static NTSTATUS
VirtioSndGetWaveChannelMask(_In_ ULONG Channels, _Out_ PULONG ChannelMask);

class VirtioSndWaveStream : public CUnknownImpl<IMiniportWaveCyclicStream>
{
  public:
    VirtioSndWaveStream()
        : m_Port(NULL), m_DmaChannel(NULL), m_ServiceGroup(NULL), m_Playback(NULL), m_Buffer(NULL), m_BufferSize(0),
          m_BlockAlign(0), m_Channels(0), m_BitsPerSample(0), m_SampleRate(0), m_ThreadCreated(FALSE),
          m_RequestedState(KSSTATE_STOP), m_Exit(FALSE), m_Error(STATUS_SUCCESS), m_Position(0), m_LatencyBytes(0)
    {
        KeInitializeEvent(&m_StateChanged, SynchronizationEvent, FALSE);
        KeInitializeEvent(&m_ThreadExited, NotificationEvent, FALSE);
        KeInitializeSpinLock(&m_NotifyLock);
        KeInitializeDpc(&m_NotifyDpc, NotifyDpc, this);
    }

    STDMETHODIMP_(NTSTATUS) QueryInterface(_In_ REFIID Interface, _Out_ PVOID *Object);
    IMP_IMiniportWaveCyclicStream;

    ~VirtioSndWaveStream();

    NTSTATUS
    Init(
        _In_ PPORTWAVECYCLIC Port,
        _In_ PDMACHANNEL DmaChannel,
        _In_ PKSDATAFORMAT DataFormat,
        _Out_ PSERVICEGROUP *ServiceGroup);

  private:
    static VOID NTAPI
    WorkerThread(_In_ PVOID Context);

    static VOID NTAPI
    NotifyDpc(
        _In_ PKDPC Dpc,
        _In_opt_ PVOID DeferredContext,
        _In_opt_ PVOID SystemArgument1,
        _In_opt_ PVOID SystemArgument2);

    VOID
    RunWorker();

    NTSTATUS
    SetWorkerError(_In_ NTSTATUS Status);

    PPORTWAVECYCLIC m_Port;
    PDMACHANNEL m_DmaChannel;
    PSERVICEGROUP m_ServiceGroup;
    PVIRTIO_SND_PLAYBACK m_Playback;
    PVOID m_Buffer;
    ULONG m_BufferSize;
    ULONG m_BlockAlign;
    ULONG m_Channels;
    ULONG m_BitsPerSample;
    ULONG m_SampleRate;
    KEVENT m_StateChanged;
    KEVENT m_ThreadExited;
    KDPC m_NotifyDpc;
    KSPIN_LOCK m_NotifyLock;
    BOOLEAN m_ThreadCreated;
    volatile LONG m_RequestedState;
    volatile LONG m_Exit;
    volatile LONG m_Error;
    volatile LONG m_Position;
    volatile LONG m_LatencyBytes;
};

NTSTATUS
VirtioSndWaveStream::Init(
    _In_ PPORTWAVECYCLIC Port,
    _In_ PDMACHANNEL DmaChannel,
    _In_ PKSDATAFORMAT DataFormat,
    _Out_ PSERVICEGROUP *ServiceGroup)
{
    PAGED_CODE();

    if (Port == NULL || DmaChannel == NULL || DataFormat == NULL || ServiceGroup == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    *ServiceGroup = NULL;
    m_Port = Port;
    m_Port->AddRef();
    m_DmaChannel = DmaChannel;
    m_DmaChannel->AddRef();
    PWAVEFORMATEX Format = reinterpret_cast<PWAVEFORMATEX>(DataFormat + 1);
    m_BlockAlign = Format->nBlockAlign;
    m_Channels = Format->nChannels;
    m_BitsPerSample = Format->wBitsPerSample;
    m_SampleRate = Format->nSamplesPerSec;

    NTSTATUS Status = PcNewServiceGroup(&m_ServiceGroup, NULL);
    if (!NT_SUCCESS(Status))
        return Status;

    if (m_DmaChannel->AllocatedBufferSize() < VIRTIO_SND_DMA_BUFFER_BYTES)
    {
        if (m_DmaChannel->AllocatedBufferSize() != 0)
            m_DmaChannel->FreeBuffer();

        Status = m_DmaChannel->AllocateBuffer(VIRTIO_SND_DMA_BUFFER_BYTES, NULL);
        if (!NT_SUCCESS(Status))
            return Status;
    }

    m_DmaChannel->SetBufferSize(VIRTIO_SND_DMA_BUFFER_BYTES);
    m_Buffer = m_DmaChannel->SystemAddress();
    m_BufferSize = m_DmaChannel->BufferSize();
    if (m_Buffer == NULL || m_BufferSize < VIRTIO_SND_DMA_BUFFER_BYTES)
        return STATUS_INSUFFICIENT_RESOURCES;
    RtlZeroMemory(m_Buffer, m_BufferSize);

    HANDLE ThreadHandle = NULL;
    Status = PsCreateSystemThread(&ThreadHandle, THREAD_ALL_ACCESS, NULL, NULL, NULL, WorkerThread, this);
    if (!NT_SUCCESS(Status))
        return Status;

    m_ThreadCreated = TRUE;
    ZwClose(ThreadHandle);

    Status = VirtioSndOpenPlayback(m_Channels, m_BitsPerSample, Format->nSamplesPerSec, &m_StateChanged, &m_Playback);
    if (!NT_SUCCESS(Status))
        return Status;

    m_ServiceGroup->AddRef();
    *ServiceGroup = m_ServiceGroup;
    return STATUS_SUCCESS;
}

VirtioSndWaveStream::~VirtioSndWaveStream()
{
    PAGED_CODE();

    if (m_ThreadCreated)
    {
        InterlockedExchange(&m_Exit, TRUE);
        KeSetEvent(&m_StateChanged, IO_NO_INCREMENT, FALSE);
        KeWaitForSingleObject(&m_ThreadExited, Executive, KernelMode, FALSE, NULL);
        KeFlushQueuedDpcs();
    }

    if (m_Playback != NULL)
        VirtioSndClosePlayback(m_Playback);

    if (m_DmaChannel != NULL)
    {
        m_DmaChannel->FreeBuffer();
        m_DmaChannel->Release();
    }
    if (m_ServiceGroup != NULL)
        m_ServiceGroup->Release();
    if (m_Port != NULL)
        m_Port->Release();
}

VOID NTAPI
VirtioSndWaveStream::WorkerThread(_In_ PVOID Context)
{
    VirtioSndWaveStream *Stream = static_cast<VirtioSndWaveStream *>(Context);
    Stream->RunWorker();
    KeSetEvent(&Stream->m_ThreadExited, IO_NO_INCREMENT, FALSE);
    PsTerminateSystemThread(STATUS_SUCCESS);
}

VOID NTAPI
VirtioSndWaveStream::NotifyDpc(
    _In_ PKDPC Dpc,
    _In_opt_ PVOID DeferredContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2)
{
    VirtioSndWaveStream *Stream = static_cast<VirtioSndWaveStream *>(DeferredContext);
    PPORTWAVECYCLIC Port;
    PSERVICEGROUP ServiceGroup;
    KIRQL OldIrql;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    KeAcquireSpinLock(&Stream->m_NotifyLock, &OldIrql);
    Port = Stream->m_Port;
    ServiceGroup = Stream->m_ServiceGroup;
    if (Port != NULL && ServiceGroup != NULL)
    {
        Port->AddRef();
        ServiceGroup->AddRef();
    }
    KeReleaseSpinLock(&Stream->m_NotifyLock, OldIrql);

    if (Port != NULL && ServiceGroup != NULL)
    {
        Port->Notify(ServiceGroup);
        ServiceGroup->Release();
        Port->Release();
    }
}

NTSTATUS
VirtioSndWaveStream::SetWorkerError(_In_ NTSTATUS Status)
{
    if (NT_SUCCESS(Status))
        return Status;

    InterlockedCompareExchange(&m_Error, Status, STATUS_SUCCESS);
    DbgPrint("virtiosnd: Playback worker failed with status %08lx\n", (ULONG)Status);
    return Status;
}

VOID
VirtioSndWaveStream::RunWorker()
{
    BOOLEAN Running = FALSE;
    ULONG SubmittedPosition = 0;
    UCHAR Audio[VIRTIO_SND_PCM_PERIOD_BYTES];

    for (;;)
    {
        if (InterlockedCompareExchange(&m_Exit, 0, 0) != FALSE || VirtioSndTransportStopping())
            break;

        NTSTATUS Error = (NTSTATUS)InterlockedCompareExchange(&m_Error, 0, 0);
        if (!NT_SUCCESS(Error))
            break;

        KSSTATE RequestedState = (KSSTATE)InterlockedCompareExchange(&m_RequestedState, 0, 0);

        if (RequestedState != KSSTATE_RUN && Running)
        {
            NTSTATUS Status = VirtioSndStopPlayback(m_Playback);
            Running = FALSE;
            if (!NT_SUCCESS(Status))
            {
                SetWorkerError(Status);
                break;
            }

            if (RequestedState == KSSTATE_STOP)
            {
                InterlockedExchange(&m_Position, 0);
                InterlockedExchange(&m_LatencyBytes, 0);
                SubmittedPosition = 0;
            }
        }

        if (RequestedState == KSSTATE_RUN && !Running)
        {
            NTSTATUS Status = VirtioSndStartPlayback(m_Playback);
            if (!NT_SUCCESS(Status))
            {
                SetWorkerError(Status);
                break;
            }

            DbgPrint("virtiosnd: PCM playback started\n");
            SubmittedPosition = (ULONG)InterlockedCompareExchange(&m_Position, 0, 0) % m_BufferSize;
            Running = TRUE;
        }

        if (m_Playback != NULL)
        {
            ULONG XrunCount;
            NTSTATUS Status = VirtioSndProcessPlaybackEvents(m_Playback, &XrunCount);
            if (!NT_SUCCESS(Status))
            {
                SetWorkerError(Status);
                break;
            }

            if (XrunCount != 0 && Running &&
                (KSSTATE)InterlockedCompareExchange(&m_RequestedState, 0, 0) == KSSTATE_RUN)
            {
                DbgPrint("virtiosnd: Received %lu XRUN event(s) for stream. Restarting PCM playback\n", XrunCount);
                Status = VirtioSndStopPlayback(m_Playback);
                Running = FALSE;
                if (!NT_SUCCESS(Status))
                {
                    SetWorkerError(Status);
                    break;
                }

                SubmittedPosition = (ULONG)InterlockedCompareExchange(&m_Position, 0, 0) % m_BufferSize;
                Status = VirtioSndStartPlayback(m_Playback);
                if (!NT_SUCCESS(Status))
                {
                    SetWorkerError(Status);
                    break;
                }
                Running = TRUE;
            }
        }

        if (Running)
        {
            for (;;)
            {
                ULONG DeviceStatus;
                ULONG LatencyBytes;
                NTSTATUS Status = VirtioSndGetPlaybackCompletion(m_Playback, &DeviceStatus, &LatencyBytes);
                if (Status == STATUS_NO_MORE_ENTRIES)
                    break;
                if (!NT_SUCCESS(Status))
                {
                    SetWorkerError(Status);
                    break;
                }
                if (DeviceStatus != VIRTIO_SND_S_OK)
                {
                    DbgPrint("virtiosnd: PCM transfer failed with device status %08lx\n", DeviceStatus);
                    SetWorkerError(STATUS_UNSUCCESSFUL);
                    break;
                }

                InterlockedExchangeAdd(&m_Position, VIRTIO_SND_PCM_PERIOD_BYTES);
                InterlockedExchange(&m_LatencyBytes, (LONG)LatencyBytes);
                KeInsertQueueDpc(&m_NotifyDpc, NULL, NULL);
            }

            Error = (NTSTATUS)InterlockedCompareExchange(&m_Error, 0, 0);
            if (!NT_SUCCESS(Error))
                break;

            if ((KSSTATE)InterlockedCompareExchange(&m_RequestedState, 0, 0) == KSSTATE_RUN)
            {
                for (;;)
                {
                    ULONG FirstLength = min(VIRTIO_SND_PCM_PERIOD_BYTES, m_BufferSize - SubmittedPosition);
                    RtlCopyMemory(Audio, (PUCHAR)m_Buffer + SubmittedPosition, FirstLength);
                    if (FirstLength < VIRTIO_SND_PCM_PERIOD_BYTES)
                    {
                        RtlCopyMemory(Audio + FirstLength, m_Buffer, VIRTIO_SND_PCM_PERIOD_BYTES - FirstLength);
                    }
                    VirtioSndApplyMasterControls(Audio, sizeof(Audio), m_BitsPerSample);

                    NTSTATUS Status = VirtioSndQueuePlayback(m_Playback, Audio, sizeof(Audio));
                    if (Status == STATUS_DEVICE_BUSY)
                        break;
                    if (!NT_SUCCESS(Status))
                    {
                        SetWorkerError(Status);
                        break;
                    }

                    SubmittedPosition = (SubmittedPosition + sizeof(Audio)) % m_BufferSize;
                }
            }
        }

        if (Running && !VirtioSndArmPlaybackCompletion(m_Playback))
        {
            continue;
        }

        KeWaitForSingleObject(&m_StateChanged, Executive, KernelMode, FALSE, NULL);
    }

    if (Running)
    {
        NTSTATUS Status = VirtioSndStopPlayback(m_Playback);
        if (!NT_SUCCESS(Status))
            SetWorkerError(Status);
    }

    VirtioSndPlaybackWorkerQuiesced();
}

STDMETHODIMP_(NTSTATUS)
VirtioSndWaveStream::QueryInterface(REFIID Interface, PVOID *Object)
{
    if (Object == NULL)
        return STATUS_INVALID_PARAMETER;

    *Object = NULL;
    if (IsEqualGUIDAligned(Interface, IID_IUnknown) || IsEqualGUIDAligned(Interface, IID_IMiniportWaveCyclicStream))
    {
        *Object = static_cast<IMiniportWaveCyclicStream *>(this);
    }

    if (*Object == NULL)
        return STATUS_INVALID_PARAMETER;

    AddRef();
    return STATUS_SUCCESS;
}

STDMETHODIMP_(NTSTATUS)
VirtioSndWaveStream::SetFormat(PKSDATAFORMAT DataFormat)
{
    PAGED_CODE();
    NTSTATUS Status = VirtioSndValidateFormat(DataFormat);
    if (!NT_SUCCESS(Status))
        return Status;

    PWAVEFORMATEX Format = reinterpret_cast<PWAVEFORMATEX>(DataFormat + 1);
    return Format->nChannels == m_Channels && Format->wBitsPerSample == m_BitsPerSample &&
                   Format->nBlockAlign == m_BlockAlign && Format->nSamplesPerSec == m_SampleRate
               ? STATUS_SUCCESS
               : STATUS_NO_MATCH;
}

STDMETHODIMP_(ULONG)
VirtioSndWaveStream::SetNotificationFreq(ULONG Interval, PULONG FrameSize)
{
    if (FrameSize == NULL || Interval == 0 || m_BlockAlign == 0 || m_SampleRate == 0)
        return 0;

    ULONGLONG RequestedFrameSize = (ULONGLONG)m_SampleRate * m_BlockAlign * Interval / 1000;
    if (RequestedFrameSize > MAXULONG)
        return 0;

    *FrameSize = (ULONG)RequestedFrameSize;
    *FrameSize -= *FrameSize % m_BlockAlign;
    return Interval;
}

STDMETHODIMP_(NTSTATUS)
VirtioSndWaveStream::SetState(KSSTATE State)
{
    if (State < KSSTATE_STOP || State > KSSTATE_RUN)
        return STATUS_INVALID_PARAMETER;
    if (!VirtioSndPlaybackIsActive(m_Playback))
        return STATUS_DEVICE_NOT_READY;

    NTSTATUS Error = (NTSTATUS)InterlockedCompareExchange(&m_Error, 0, 0);
    if (!NT_SUCCESS(Error))
        return Error;

    InterlockedExchange(&m_RequestedState, State);
    KeSetEvent(&m_StateChanged, IO_NO_INCREMENT, FALSE);
    return STATUS_SUCCESS;
}

STDMETHODIMP_(NTSTATUS)
VirtioSndWaveStream::GetPosition(PULONG Position)
{
    if (Position == NULL || m_BufferSize == 0)
        return STATUS_INVALID_PARAMETER;

    ULONG Completed = (ULONG)InterlockedCompareExchange(&m_Position, 0, 0);
    ULONG Latency = (ULONG)InterlockedCompareExchange(&m_LatencyBytes, 0, 0);
    ULONG PlaybackPosition = Latency > Completed ? 0 : Completed - Latency;
    *Position = PlaybackPosition % m_BufferSize;
    return STATUS_SUCCESS;
}

STDMETHODIMP_(NTSTATUS)
VirtioSndWaveStream::NormalizePhysicalPosition(PLONGLONG PhysicalPosition)
{
    if (PhysicalPosition == NULL || m_BlockAlign == 0 || m_SampleRate == 0)
        return STATUS_INVALID_PARAMETER;

    *PhysicalPosition = (10000000 / m_BlockAlign * *PhysicalPosition) / m_SampleRate;
    return STATUS_SUCCESS;
}

STDMETHODIMP_(void)
VirtioSndWaveStream::Silence(PVOID Buffer, ULONG ByteCount)
{
    if (Buffer != NULL)
        RtlZeroMemory(Buffer, ByteCount);
}

static NTSTATUS
VirtioSndGetWaveChannelMask(_In_ ULONG Channels, _Out_ PULONG ChannelMask)
{
    UCHAR Positions[2];
    ULONG Mask = 0;
    ULONG PreviousSpeaker = 0;
    ULONG ChannelIndex;
    NTSTATUS Status;

    if (ChannelMask == NULL || Channels == 0 || Channels > RTL_NUMBER_OF(Positions))
        return STATUS_INVALID_PARAMETER;

    Status = VirtioSndGetPlaybackChannelMap(Channels, Positions);
    if (Status == STATUS_NOT_SUPPORTED)
    {
        *ChannelMask = Channels == 1 ? KSAUDIO_SPEAKER_MONO : KSAUDIO_SPEAKER_STEREO;
        return STATUS_SUCCESS;
    }
    if (!NT_SUCCESS(Status))
        return Status;

    for (ChannelIndex = 0; ChannelIndex < Channels; ++ChannelIndex)
    {
        ULONG Speaker;

        switch (Positions[ChannelIndex])
        {
            case VIRTIO_SND_CHMAP_MONO:
            case VIRTIO_SND_CHMAP_FC:
                Speaker = SPEAKER_FRONT_CENTER;
                break;
            case VIRTIO_SND_CHMAP_FL:
                Speaker = SPEAKER_FRONT_LEFT;
                break;
            case VIRTIO_SND_CHMAP_FR:
                Speaker = SPEAKER_FRONT_RIGHT;
                break;
            case VIRTIO_SND_CHMAP_RL:
                Speaker = SPEAKER_BACK_LEFT;
                break;
            case VIRTIO_SND_CHMAP_RR:
                Speaker = SPEAKER_BACK_RIGHT;
                break;
            case VIRTIO_SND_CHMAP_LFE:
                Speaker = SPEAKER_LOW_FREQUENCY;
                break;
            case VIRTIO_SND_CHMAP_SL:
                Speaker = SPEAKER_SIDE_LEFT;
                break;
            case VIRTIO_SND_CHMAP_SR:
                Speaker = SPEAKER_SIDE_RIGHT;
                break;
            case VIRTIO_SND_CHMAP_RC:
                Speaker = SPEAKER_BACK_CENTER;
                break;
            case VIRTIO_SND_CHMAP_FLC:
                Speaker = SPEAKER_FRONT_LEFT_OF_CENTER;
                break;
            case VIRTIO_SND_CHMAP_FRC:
                Speaker = SPEAKER_FRONT_RIGHT_OF_CENTER;
                break;
            case VIRTIO_SND_CHMAP_TC:
                Speaker = SPEAKER_TOP_CENTER;
                break;
            case VIRTIO_SND_CHMAP_TFL:
            case VIRTIO_SND_CHMAP_FLH:
                Speaker = SPEAKER_TOP_FRONT_LEFT;
                break;
            case VIRTIO_SND_CHMAP_TFC:
            case VIRTIO_SND_CHMAP_FCH:
                Speaker = SPEAKER_TOP_FRONT_CENTER;
                break;
            case VIRTIO_SND_CHMAP_TFR:
            case VIRTIO_SND_CHMAP_FRH:
                Speaker = SPEAKER_TOP_FRONT_RIGHT;
                break;
            case VIRTIO_SND_CHMAP_TRL:
                Speaker = SPEAKER_TOP_BACK_LEFT;
                break;
            case VIRTIO_SND_CHMAP_TRC:
                Speaker = SPEAKER_TOP_BACK_CENTER;
                break;
            case VIRTIO_SND_CHMAP_TRR:
                Speaker = SPEAKER_TOP_BACK_RIGHT;
                break;
            default:
                return STATUS_NO_MATCH;
        }

        if ((Mask & Speaker) != 0 || Speaker <= PreviousSpeaker)
            return STATUS_NO_MATCH;

        Mask |= Speaker;
        PreviousSpeaker = Speaker;
    }

    *ChannelMask = Mask;
    return STATUS_SUCCESS;
}

static NTSTATUS
VirtioSndValidateFormat(_In_ PKSDATAFORMAT DataFormat)
{
    if (DataFormat == NULL || !IsEqualGUIDAligned(DataFormat->MajorFormat, KSDATAFORMAT_TYPE_AUDIO) ||
        !IsEqualGUIDAligned(DataFormat->SubFormat, KSDATAFORMAT_SUBTYPE_PCM) ||
        !IsEqualGUIDAligned(DataFormat->Specifier, KSDATAFORMAT_SPECIFIER_WAVEFORMATEX) ||
        DataFormat->FormatSize < sizeof(KSDATAFORMAT) + sizeof(WAVEFORMATEX))
    {
        return STATUS_NO_MATCH;
    }

    PWAVEFORMATEX Format = reinterpret_cast<PWAVEFORMATEX>(DataFormat + 1);
    ULONG ChannelMask;
    ULONG BytesPerSample;
    if ((Format->wFormatTag != WAVE_FORMAT_PCM && Format->wFormatTag != WAVE_FORMAT_EXTENSIBLE) ||
        Format->nChannels == 0 || Format->nChannels > 2 ||
        (Format->wBitsPerSample != 16 && Format->wBitsPerSample != 32))
    {
        return STATUS_NO_MATCH;
    }
    BytesPerSample = Format->wBitsPerSample / 8;
    if (Format->nBlockAlign != Format->nChannels * BytesPerSample)
        return STATUS_NO_MATCH;

    ULONG RateIndex = MAXULONG;
    for (ULONG Rate = 0; Rate < RTL_NUMBER_OF(g_VirtioSndWaveRates); ++Rate)
    {
        if (g_VirtioSndWaveRates[Rate].SampleRate == Format->nSamplesPerSec)
        {
            RateIndex = g_VirtioSndWaveRates[Rate].RateIndex;
            break;
        }
    }
    if (RateIndex == MAXULONG)
        return STATUS_NO_MATCH;

    ULONGLONG SupportedRates;
    NTSTATUS Status = VirtioSndGetPlaybackRates(Format->nChannels, Format->wBitsPerSample, &SupportedRates);
    if (!NT_SUCCESS(Status) || (SupportedRates & ((ULONGLONG)1 << RateIndex)) == 0)
    {
        return STATUS_NO_MATCH;
    }

    if (!NT_SUCCESS(VirtioSndGetWaveChannelMask(Format->nChannels, &ChannelMask)))
    {
        return STATUS_NO_MATCH;
    }

    if (Format->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
    {
        if (DataFormat->FormatSize < sizeof(KSDATAFORMAT) + sizeof(WAVEFORMATEXTENSIBLE) ||
            Format->cbSize < sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX))
        {
            return STATUS_NO_MATCH;
        }

        PWAVEFORMATEXTENSIBLE Extensible = reinterpret_cast<PWAVEFORMATEXTENSIBLE>(Format);
        if (!IsEqualGUIDAligned(Extensible->SubFormat, KSDATAFORMAT_SUBTYPE_PCM) ||
            Extensible->dwChannelMask != ChannelMask ||
            Extensible->Samples.wValidBitsPerSample != Format->wBitsPerSample)
        {
            return STATUS_NO_MATCH;
        }
    }

    return STATUS_SUCCESS;
}

class VirtioSndWaveMiniport : public CUnknownImpl<IMiniportWaveCyclic>
{
  public:
    VirtioSndWaveMiniport() : m_Port(NULL), m_DmaChannel(NULL)
    {
    }

    STDMETHODIMP_(NTSTATUS) QueryInterface(_In_ REFIID Interface, _Out_ PVOID *Object);
    IMP_IMiniportWaveCyclic;

    ~VirtioSndWaveMiniport();

  private:
    PPORTWAVECYCLIC m_Port;
    PDMACHANNEL m_DmaChannel;
};

VirtioSndWaveMiniport::~VirtioSndWaveMiniport()
{
    if (m_Port != NULL)
    {
        m_Port->Release();
    }
    if (m_DmaChannel != NULL)
    {
        m_DmaChannel->Release();
    }
}

STDMETHODIMP_(NTSTATUS)
VirtioSndWaveMiniport::QueryInterface(REFIID Interface, PVOID *Object)
{
    if (Object == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    *Object = NULL;

    if (IsEqualGUIDAligned(Interface, IID_IUnknown))
    {
        *Object = static_cast<IUnknown *>(static_cast<IMiniportWaveCyclic *>(this));
    }
    else if (IsEqualGUIDAligned(Interface, IID_IMiniportWaveCyclic))
    {
        *Object = static_cast<IMiniportWaveCyclic *>(this);
    }
    else if (IsEqualGUIDAligned(Interface, IID_IMiniport))
    {
        *Object = static_cast<IMiniport *>(this);
    }

    if (*Object == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    AddRef();
    return STATUS_SUCCESS;
}

STDMETHODIMP_(NTSTATUS)
VirtioSndWaveMiniport::GetDescription(PPCFILTER_DESCRIPTOR *Description)
{
    if (Description == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    *Description = &g_VirtioSndRenderFilter;
    return STATUS_SUCCESS;
}

STDMETHODIMP_(NTSTATUS)
VirtioSndWaveMiniport::DataRangeIntersection(
    ULONG PinId,
    PKSDATARANGE ClientDataRange,
    PKSDATARANGE MyDataRange,
    ULONG OutputBufferLength,
    PVOID ResultantFormat,
    PULONG ResultantFormatLength)
{
    PAGED_CODE();

    const ULONG RequiredLength = sizeof(KSDATAFORMAT) + sizeof(WAVEFORMATEXTENSIBLE);
    if (ResultantFormatLength == NULL || PinId != 0 || ClientDataRange == NULL || MyDataRange == NULL)
    {
        DbgPrint("virtiosnd: DataRangeIntersection invalid request (pin %lu)\n", PinId);
        return STATUS_INVALID_PARAMETER;
    }

    if (ClientDataRange->FormatSize < sizeof(KSDATARANGE_AUDIO) || MyDataRange->FormatSize < sizeof(KSDATARANGE_AUDIO))
    {
        DbgPrint(
            "virtiosnd: DataRangeIntersection short range (client %lu device %lu)\n", ClientDataRange->FormatSize,
            MyDataRange->FormatSize);
        return STATUS_NO_MATCH;
    }

    PKSDATARANGE_AUDIO ClientRange = reinterpret_cast<PKSDATARANGE_AUDIO>(ClientDataRange);
    PKSDATARANGE_AUDIO DeviceRange = reinterpret_cast<PKSDATARANGE_AUDIO>(MyDataRange);
    DbgPrint(
        "virtiosnd: DataRangeIntersection pin %lu output %lu max-ch %lu bits %lu-%lu rate "
        "%lu-%lu device max-ch %lu bits %lu-%lu rate %lu-%lu\n",
        PinId, OutputBufferLength, ClientRange->MaximumChannels, ClientRange->MinimumBitsPerSample,
        ClientRange->MaximumBitsPerSample, ClientRange->MinimumSampleFrequency, ClientRange->MaximumSampleFrequency,
        DeviceRange->MaximumChannels, DeviceRange->MinimumBitsPerSample, DeviceRange->MaximumBitsPerSample,
        DeviceRange->MinimumSampleFrequency, DeviceRange->MaximumSampleFrequency);

    if (!IsEqualGUIDAligned(ClientDataRange->MajorFormat, KSDATAFORMAT_TYPE_AUDIO) ||
        !IsEqualGUIDAligned(ClientDataRange->SubFormat, KSDATAFORMAT_SUBTYPE_PCM) ||
        !IsEqualGUIDAligned(ClientDataRange->Specifier, KSDATAFORMAT_SPECIFIER_WAVEFORMATEX) ||
        !IsEqualGUIDAligned(MyDataRange->MajorFormat, KSDATAFORMAT_TYPE_AUDIO) ||
        !IsEqualGUIDAligned(MyDataRange->SubFormat, KSDATAFORMAT_SUBTYPE_PCM) ||
        !IsEqualGUIDAligned(MyDataRange->Specifier, KSDATAFORMAT_SPECIFIER_WAVEFORMATEX))
    {
        DbgPrint("virtiosnd: DataRangeIntersection rejected non-PCM format\n");
        return STATUS_NO_MATCH;
    }

    ULONG Channels;
    ULONG ChannelMask = 0;
    ULONG BitsPerSample = 0;
    ULONG SampleRate = 0;
    ULONGLONG SupportedRates;
    NTSTATUS Status;
    if (ClientRange->MaximumChannels == 0 || DeviceRange->MaximumChannels == 0 ||
        ClientRange->MinimumBitsPerSample > ClientRange->MaximumBitsPerSample ||
        DeviceRange->MinimumBitsPerSample > DeviceRange->MaximumBitsPerSample)
    {
        DbgPrint("virtiosnd: DataRangeIntersection rejected unsupported rate, depth, or channel "
                 "count\n");
        return STATUS_NO_MATCH;
    }

    static const ULONG SupportedBitDepths[] = {32, 16};
    for (ULONG BitDepthIndex = 0; BitDepthIndex < RTL_NUMBER_OF(SupportedBitDepths) && SampleRate == 0; ++BitDepthIndex)
    {
        ULONG CandidateBits = SupportedBitDepths[BitDepthIndex];
        if (CandidateBits < ClientRange->MinimumBitsPerSample || CandidateBits > ClientRange->MaximumBitsPerSample ||
            CandidateBits < DeviceRange->MinimumBitsPerSample || CandidateBits > DeviceRange->MaximumBitsPerSample)
        {
            continue;
        }

        ULONG CandidateChannels = min(min(ClientRange->MaximumChannels, DeviceRange->MaximumChannels), 2);
        while (CandidateChannels != 0 && SampleRate == 0)
        {
            Status = VirtioSndGetWaveChannelMask(CandidateChannels, &ChannelMask);
            if (Status == STATUS_NO_MATCH)
            {
                --CandidateChannels;
                continue;
            }
            if (!NT_SUCCESS(Status))
            {
                DbgPrint("virtiosnd: DataRangeIntersection channel map failed %08lx\n", (ULONG)Status);
                return Status;
            }

            Status = VirtioSndGetPlaybackRates(CandidateChannels, CandidateBits, &SupportedRates);
            if (NT_SUCCESS(Status))
            {
                for (ULONG Rate = 0; Rate < RTL_NUMBER_OF(g_VirtioSndWaveRates); ++Rate)
                {
                    ULONG CandidateRate = g_VirtioSndWaveRates[Rate].SampleRate;
                    ULONG RateIndex = g_VirtioSndWaveRates[Rate].RateIndex;

                    if (CandidateRate >= ClientRange->MinimumSampleFrequency &&
                        CandidateRate <= ClientRange->MaximumSampleFrequency &&
                        CandidateRate >= DeviceRange->MinimumSampleFrequency &&
                        CandidateRate <= DeviceRange->MaximumSampleFrequency &&
                        (SupportedRates & ((ULONGLONG)1 << RateIndex)) != 0)
                    {
                        SampleRate = CandidateRate;
                        BitsPerSample = CandidateBits;
                        Channels = CandidateChannels;
                        break;
                    }
                }
            }

            if (SampleRate == 0)
                --CandidateChannels;
        }
    }
    if (SampleRate == 0)
        return STATUS_NO_MATCH;

    if (OutputBufferLength == 0 || ResultantFormat == NULL)
    {
        *ResultantFormatLength = RequiredLength;
        DbgPrint("virtiosnd: DataRangeIntersection size query needs %lu bytes\n", RequiredLength);
        return STATUS_BUFFER_OVERFLOW;
    }
    if (OutputBufferLength < RequiredLength)
    {
        *ResultantFormatLength = RequiredLength;
        DbgPrint("virtiosnd: DataRangeIntersection buffer too small (%lu < %lu)\n", OutputBufferLength, RequiredLength);
        return STATUS_BUFFER_TOO_SMALL;
    }

    PKSDATAFORMAT Format = static_cast<PKSDATAFORMAT>(ResultantFormat);
    RtlCopyMemory(Format, MyDataRange, sizeof(KSDATAFORMAT));
    Format->FormatSize = RequiredLength;

    PWAVEFORMATEXTENSIBLE WaveFormat = reinterpret_cast<PWAVEFORMATEXTENSIBLE>(Format + 1);
    RtlZeroMemory(WaveFormat, sizeof(*WaveFormat));
    WaveFormat->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    WaveFormat->Format.nChannels = (WORD)Channels;
    WaveFormat->Format.nSamplesPerSec = SampleRate;
    WaveFormat->Format.wBitsPerSample = (WORD)BitsPerSample;
    WaveFormat->Format.nBlockAlign = WaveFormat->Format.nChannels * BitsPerSample / 8;
    WaveFormat->Format.nAvgBytesPerSec = WaveFormat->Format.nSamplesPerSec * WaveFormat->Format.nBlockAlign;
    WaveFormat->Format.cbSize = sizeof(*WaveFormat) - sizeof(WAVEFORMATEX);
    WaveFormat->Samples.wValidBitsPerSample = (WORD)BitsPerSample;
    WaveFormat->dwChannelMask = ChannelMask;
    WaveFormat->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    Format->SampleSize = WaveFormat->Format.nBlockAlign;
    *ResultantFormatLength = RequiredLength;
    DbgPrint(
        "virtiosnd: DataRangeIntersection accepted %u-channel %lu Hz S%lu, mask %08lx\n", Channels, SampleRate,
        BitsPerSample, ChannelMask);
    return STATUS_SUCCESS;
}

STDMETHODIMP_(NTSTATUS)
VirtioSndWaveMiniport::Init(PUNKNOWN UnknownAdapter, PRESOURCELIST ResourceList, PPORTWAVECYCLIC Port)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(UnknownAdapter);
    UNREFERENCED_PARAMETER(ResourceList);

    if (Port == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    m_Port = Port;
    m_Port->AddRef();
    return m_Port->NewMasterDmaChannel(
        &m_DmaChannel, NULL, NULL, VIRTIO_SND_DMA_BUFFER_BYTES, TRUE, FALSE, Width32Bits, MaximumDmaSpeed);
}

STDMETHODIMP_(NTSTATUS)
VirtioSndWaveMiniport::NewStream(
    PMINIPORTWAVECYCLICSTREAM *Stream,
    PUNKNOWN OuterUnknown,
    POOL_TYPE PoolType,
    ULONG Pin,
    BOOLEAN Capture,
    PKSDATAFORMAT DataFormat,
    PDMACHANNEL *DmaChannel,
    PSERVICEGROUP *ServiceGroup)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(OuterUnknown);
    UNREFERENCED_PARAMETER(PoolType);

    if (Stream == NULL || DmaChannel == NULL || ServiceGroup == NULL)
    {
        DbgPrint("virtiosnd: NewStream invalid output arguments\n");
        return STATUS_INVALID_PARAMETER;
    }

    *Stream = NULL;
    *DmaChannel = NULL;
    *ServiceGroup = NULL;

    DbgPrint("virtiosnd: NewStream pin %lu capture %u format %p\n", Pin, (ULONG)Capture, DataFormat);
    if (Pin != 0 || Capture)
    {
        DbgPrint("virtiosnd: NewStream rejected pin %lu capture %u\n", Pin, (ULONG)Capture);
        return STATUS_INVALID_PARAMETER;
    }

    NTSTATUS Status = VirtioSndValidateFormat(DataFormat);
    if (!NT_SUCCESS(Status))
    {
        DbgPrint("virtiosnd: NewStream rejected format with status %08lx\n", (ULONG)Status);
        return Status;
    }

    PWAVEFORMATEX Format = reinterpret_cast<PWAVEFORMATEX>(DataFormat + 1);
    DbgPrint(
        "virtiosnd: NewStream format %u-channel %lu Hz %u-bit\n", (ULONG)Format->nChannels, Format->nSamplesPerSec,
        (ULONG)Format->wBitsPerSample);

    VirtioSndWaveStream *WaveStream = new (PoolType, VIRTIO_SND_POOL_TAG) VirtioSndWaveStream();
    if (WaveStream == NULL)
    {
        DbgPrint("virtiosnd: NewStream allocation failed\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    WaveStream->AddRef();

    Status = WaveStream->Init(m_Port, m_DmaChannel, DataFormat, ServiceGroup);
    if (!NT_SUCCESS(Status))
    {
        DbgPrint("virtiosnd: NewStream initialization failed with status %08lx\n", (ULONG)Status);
        WaveStream->Release();
        return Status;
    }

    m_DmaChannel->AddRef();
    *DmaChannel = m_DmaChannel;
    *Stream = static_cast<IMiniportWaveCyclicStream *>(WaveStream);
    DbgPrint("virtiosnd: NewStream created successfully\n");
    return STATUS_SUCCESS;
}

static NTSTATUS
VirtioSndCreateWaveMiniport(
    _Out_ PUNKNOWN *Unknown,
    _In_ REFCLSID ClassId,
    _In_opt_ PUNKNOWN UnknownOuter,
    _In_ POOL_TYPE PoolType)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(ClassId);
    UNREFERENCED_PARAMETER(UnknownOuter);

    VirtioSndWaveMiniport *Object = new (PoolType, VIRTIO_SND_POOL_TAG) VirtioSndWaveMiniport();
    if (Object == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    *Unknown = static_cast<PUNKNOWN>(static_cast<PMINIPORTWAVECYCLIC>(Object));
    (*Unknown)->AddRef();
    return STATUS_SUCCESS;
}

NTSTATUS
VirtioSndInstallRenderSubdevice(PDEVICE_OBJECT DeviceObject, PIRP Irp, PRESOURCELIST ResourceList)
{
    PAGED_CODE();

    PPORT Port = NULL;
    PUNKNOWN Miniport = NULL;
    PUNKNOWN WaveUnknown = NULL;
    PUNKNOWN TopologyUnknown = NULL;
    NTSTATUS Status = PcNewPort(&Port, CLSID_PortWaveCyclic);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = VirtioSndCreateWaveMiniport(&Miniport, CLSID_PortWaveCyclic, NULL, NonPagedPool);
    if (NT_SUCCESS(Status))
    {
        Status = Port->Init(DeviceObject, Irp, Miniport, NULL, ResourceList);
    }

    if (NT_SUCCESS(Status))
    {
        Status = PcRegisterSubdevice(DeviceObject, g_VirtioSndRenderSubdeviceName, Port);
    }
    if (NT_SUCCESS(Status))
    {
        WaveUnknown = (PUNKNOWN)Port;
        WaveUnknown->AddRef();
    }

    if (Miniport != NULL)
    {
        Miniport->Release();
    }
    Port->Release();

    if (NT_SUCCESS(Status))
    {
        Status = VirtioSndInstallTopologySubdevice(DeviceObject, Irp, ResourceList, &TopologyUnknown);
    }

    if (NT_SUCCESS(Status))
    {
        Status = PcRegisterPhysicalConnection(DeviceObject, WaveUnknown, 1, TopologyUnknown, 0);
    }

    if (TopologyUnknown != NULL)
        TopologyUnknown->Release();
    if (WaveUnknown != NULL)
        WaveUnknown->Release();

    if (NT_SUCCESS(Status))
    {
        DbgPrint("virtiosnd: WaveCyclic render and topology subdevices registered\n");
    }

    return Status;
}
