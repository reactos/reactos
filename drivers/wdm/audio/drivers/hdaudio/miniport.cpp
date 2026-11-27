/*
 * PROJECT:         ReactOS HDAudio Driver
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         WaveRT miniport
 * COPYRIGHT:       Copyright 2025-2026 Johannes Anderwald <johannes.anderwald@reactos.org>
 *                  Copyright 2025-2026 Oleg Dubinskiy <oleg.dubinskiy@reactos.org>
 */

#include "private.h"

#define NDEBUG
#include <debug.h>

NTSTATUS
NTAPI
CMiniportWaveRT::QueryInterface(_In_ REFIID refiid, _Out_ PVOID *Output)
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
        DPRINT("CMiniportWaveRT::QueryInterface no interface!!! iface %S\n", GuidString.Buffer);
        RtlFreeUnicodeString(&GuidString);
    }
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
CMiniportWaveRT::GetDescription(_Out_ PPCFILTER_DESCRIPTOR* Description)
{
    *Description = m_FilterDescription;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CMiniportWaveRT::DataRangeIntersection(
    _In_ ULONG PinId,
    _In_ PKSDATARANGE DataRange,
    _In_ PKSDATARANGE MatchingDataRange,
    _In_ ULONG OutputBufferLength,
    _Out_opt_ PVOID ResultantFormat,
    _Out_ PULONG ResultantFormatLength)
{
    HDAUDIO_STREAM_FORMAT StreamFormat;
    NTSTATUS Status;

    if (!ResultantFormatLength || PinId != 0 ||
        !DataRange || !MatchingDataRange ||
        MatchingDataRange->FormatSize < sizeof(KSDATARANGE_AUDIO))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (!IsEqualGUIDAligned(DataRange->MajorFormat,
                            MatchingDataRange->MajorFormat) ||
        !IsEqualGUIDAligned(DataRange->SubFormat,
                            MatchingDataRange->SubFormat) ||
        !IsEqualGUIDAligned(DataRange->Specifier,
                            MatchingDataRange->Specifier))
    {
        return STATUS_NO_MATCH;
    }

    Status = HDAUDIO_ValidateDataFormat(
        (PKSDATAFORMAT)DataRange,
        (PKSDATARANGE_AUDIO)MatchingDataRange,
        &StreamFormat);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = HDAUDIO_ValidateNodeFormats(m_Node,
                                         m_FilterContext->ConverterCount,
                                         m_FilterContext->Converters,
                                         &StreamFormat);
    if (!NT_SUCCESS(Status))
        return Status;

    *ResultantFormatLength = DataRange->FormatSize;
    if (!OutputBufferLength || !ResultantFormat)
        return STATUS_BUFFER_OVERFLOW;
    if (OutputBufferLength < DataRange->FormatSize)
        return STATUS_BUFFER_TOO_SMALL;

    RtlCopyMemory(ResultantFormat, DataRange, DataRange->FormatSize);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CMiniportWaveRT::Init(
    _In_ PUNKNOWN UnknownAdapter,
    _In_ PRESOURCELIST ResourceList,
    _In_ PPORTWAVERT Port)
{
    m_Port = Port;
    m_Port->AddRef();
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CMiniportWaveRT::NewStream(
    _Out_ PMINIPORTWAVERTSTREAM* Stream,
    _In_ PPORTWAVERTSTREAM PortStream,
    _In_ ULONG Pin,
    _In_ BOOLEAN Capture,
    _In_ PKSDATAFORMAT DataFormat)
{
    return HDAUDIO_AllocateStream(Stream, m_Adapter, m_Node, Pin, Capture, DataFormat, m_FilterContext->ConverterCount, m_FilterContext->Converters, m_FilterDescription);
}

NTSTATUS
NTAPI
CMiniportWaveRT::GetDeviceDescription(
    _Out_ PDEVICE_DESCRIPTION DeviceDescription)
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

NTSTATUS
HDAUDIO_NewMiniportWaveRT(
    _Out_ PMINIPORTWAVERT* OutMiniport,
    _In_ PHDAUDIO_FILTER_CONTEXT FilterContext,
    _In_ CFunctionGroupNode * Node,
    _In_ CAdapterCommon * Adapter,
    _In_ PPCFILTER_DESCRIPTOR FilterDescription)
{
    CMiniportWaveRT *This;

    This = new (NonPagedPool, TAG_HDAUDIO) CMiniportWaveRT(NULL, FilterContext, Node, Adapter, FilterDescription);
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
