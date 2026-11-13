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
                                         m_AssociatedPinCount,
                                         m_AssociatedPins,
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
