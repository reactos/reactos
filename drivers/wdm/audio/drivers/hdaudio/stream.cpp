/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/drivers/hdaudio/stream.cpp
 * PURPOSE:         HDAudio Driver
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.h"

#define YDEBUG
#include <debug.h>

NTSTATUS
NTAPI
CMiniportWaveRTStream::QueryInterface(IN REFIID refiid, OUT PVOID *Output)
{
    UNICODE_STRING GuidString;

    if (IsEqualGUIDAligned(refiid, IID_IMiniportWaveRTStreamNotification) || IsEqualGUIDAligned(refiid, IID_IUnknown) ||
        IsEqualGUIDAligned(refiid, IID_IMiniportWaveRTStream))
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
CMiniportWaveRTStream::AllocateAudioBuffer(
    ULONG RequestedBufferSize,
    PMDL* AudioBufferMdl,
    ULONG* ActualSize,
    ULONG* OffsetFromFirstPage,
    MEMORY_CACHING_TYPE* CacheType)
{
    return m_Interface.AllocateDmaBufferWithNotification(
        m_Interface.Context, m_DmaEngine, 1, RequestedBufferSize, AudioBufferMdl, ActualSize, OffsetFromFirstPage,
        &m_StreamId, &m_FifoSize);
}

NTSTATUS
NTAPI
CMiniportWaveRTStream::GetPositionRegister(
    OUT KSRTAUDIO_HWREGISTER* Register)
{
    NTSTATUS Status;
    Status = m_Interface.GetLinkPositionRegister(m_Interface.Context, m_DmaEngine, (PULONG*) & Register->Register);
    UNIMPLEMENTED_ONCE;
    return Status;
}

NTSTATUS
NTAPI
CMiniportWaveRTStream::GetClockRegister(
    OUT KSRTAUDIO_HWREGISTER* Register)
{
    m_Interface.GetWallClockRegister(
        m_Interface.Context,
        (PULONG*)&Register->Register);
    UNIMPLEMENTED_ONCE;
    return STATUS_SUCCESS;
}

VOID
NTAPI
CMiniportWaveRTStream::GetHWLatency(
    IN KSRTAUDIO_HWLATENCY* hwLatency)
{
    UNIMPLEMENTED_ONCE;
}

VOID
NTAPI
CMiniportWaveRTStream::FreeAudioBuffer(
    PMDL AudioBufferMdl,
    ULONG BufferSize)
{
    m_Interface.FreeDmaBuffer(m_Interface.Context, m_DmaEngine);
}

NTSTATUS
NTAPI
CMiniportWaveRTStream::GetPosition(
    OUT PKSAUDIO_POSITION Position)
{
    UNIMPLEMENTED_ONCE;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CMiniportWaveRTStream::SetFormat(
    IN PKSDATAFORMAT DataFormat)
{
    UNIMPLEMENTED_ONCE;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CMiniportWaveRTStream::SetState(
    IN KSSTATE State)
{
    NTSTATUS Status;
    HANDLE Handles[1] = {m_DmaEngine};
    if (State == KSSTATE_ACQUIRE)
    {
        Status = m_Interface.SetDmaEngineState(m_Interface.Context, ResetState, 1, Handles);
    }
    if (State == KSSTATE_PAUSE || State == KSSTATE_STOP)
    {
        Status = m_Interface.SetDmaEngineState(m_Interface.Context, PauseState, 1, Handles);
    }
    if (State == KSSTATE_RUN)
    {
        for (ULONG Index = 0; Index < m_NodeCount; Index++)
        {
            Status = m_OutNode->SetStreamFormat(m_Nodes[Index], m_Converter.ConverterFormat); 
            DPRINT1("Node %u SetStreamFormat Status %\n", m_Nodes[Index], Status);
        }
        for (ULONG Index = 0; Index < m_NodeCount; Index++)
        {
            Status = m_OutNode->SetConverterStream(m_Nodes[Index], m_StreamId);
            DPRINT1("Node %u SetConverterStream Status %\n", m_Nodes[Index], Status);
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
    return m_Interface.AllocateDmaBufferWithNotification(
        m_Interface.Context, m_DmaEngine, NotificationCount, RequestedBufferSize, AudioBufferMdl, ActualSize,
        OffsetFromFirstPage, &m_StreamId, &m_FifoSize);
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
    m_Interface.FreeDmaBufferWithNotification(m_Interface.Context, m_DmaEngine, AudioBufferMdl, BufferSize);
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
    IN PULONG Nodes)
{
    HDAUDIO_STREAM_FORMAT StreamFormat;
    HANDLE hDmaEngine = NULL;
    HDAUDIO_CONVERTER_FORMAT Converter;
    NTSTATUS Status;
    HDAUDIO_BUS_INTERFACE_V2 Interface;

    if (Pin != 0)
        return STATUS_INVALID_PARAMETER;

    
    // FIXME check guids of format
    if (DataFormat->FormatSize == sizeof(KSDATAFORMAT_WAVEFORMATEX))
    {
        PKSDATAFORMAT_WAVEFORMATEX WaveFormat = (PKSDATAFORMAT_WAVEFORMATEX)DataFormat;
        StreamFormat.SampleRate = WaveFormat->WaveFormatEx.nSamplesPerSec;
        StreamFormat.NumberOfChannels = WaveFormat->WaveFormatEx.nChannels;
        StreamFormat.ValidBitsPerSample = WaveFormat->WaveFormatEx.wBitsPerSample;
        StreamFormat.ContainerSize = WaveFormat->WaveFormatEx.wBitsPerSample;
    }
    else
    {
        // not supported
        return STATUS_NOT_SUPPORTED;
    }

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
        Status = Interface.AllocateRenderDmaEngine(
            Interface.Context,
            &StreamFormat,
            FALSE, // TODO
            &hDmaEngine,
            &Converter);
    }

    CMiniportWaveRTStream *This = new (NonPagedPool, TAG_HDAUDIO)
        CMiniportWaveRTStream(NULL, Adapter, Node, Pin, Capture, &StreamFormat, &Converter, hDmaEngine, NodeCount, Nodes);
    if (!This)
    {
        // out of memory
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // add reference
    This->AddRef();

    // return result
    *Stream = This;
    return STATUS_SUCCESS;
}
