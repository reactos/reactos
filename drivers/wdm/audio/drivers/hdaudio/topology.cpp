/*
 * PROJECT:         ReactOS HDAudio Driver
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Topology miniport
 * COPYRIGHT:       Copyright 2026 Johannes Anderwald <johannes.anderwald@reactos.org>
 *                  Copyright 2025-2026 Oleg Dubinskiy <oleg.dubinskiy@reactos.org>
 */

#include "private.h"

#define NDEBUG
#include <debug.h>

NTSTATUS
NTAPI
CMiniportTopology::QueryInterface(_In_ REFIID refiid, _Out_ PVOID *Output)
{
    UNICODE_STRING GuidString;

    if (IsEqualGUIDAligned(refiid, IID_IMiniport) ||
        IsEqualGUIDAligned(refiid, IID_IUnknown) ||
        IsEqualGUIDAligned(refiid, IID_IMiniportTopology))
    {
        *Output = PVOID(PMINIPORTTOPOLOGY(this));
        PUNKNOWN(*Output)->AddRef();
        return STATUS_SUCCESS;
    }

    if (RtlStringFromGUID(refiid, &GuidString) == STATUS_SUCCESS)
    {
        DPRINT("CMiniportTopology::QueryInterface no interface!!! iface %S\n", GuidString.Buffer);
        RtlFreeUnicodeString(&GuidString);
    }
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
CMiniportTopology::GetDescription(_Out_ PPCFILTER_DESCRIPTOR* Description)
{
    *Description = m_FilterDescription;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CMiniportTopology::DataRangeIntersection(
    _In_ ULONG PinId,
    _In_ PKSDATARANGE DataRange,
    _In_ PKSDATARANGE MatchingDataRange,
    _In_ ULONG OutputBufferLength,
    _Out_opt_ PVOID ResultantFormat,
    _Out_ PULONG ResultantFormatLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CMiniportTopology::Init(
    _In_ PUNKNOWN UnknownAdapter,
    _In_ PRESOURCELIST ResourceList,
    _In_ PPORTTOPOLOGY Port)
{
    NTSTATUS Status;

    Status = Port->QueryInterface(IID_IPortEvents,
                                  (PVOID*)&m_PortEvents);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("QueryInterface failed with %x\n", Status);
        m_PortEvents = NULL;
        return Status;
    }

    m_Port = Port;
    m_Port->AddRef();
    return STATUS_SUCCESS;
}

VOID
NTAPI
CMiniportTopology::AddEventToEventList(
    _In_ PKSEVENT_ENTRY EventEntry)
{
    ASSERT(m_PortEvents);
    m_PortEvents->AddEventToEventList(EventEntry);
}

VOID
NTAPI
CMiniportTopology::GenerateEventList(
    _In_opt_ GUID *Set,
    _In_ ULONG EventId,
    _In_ BOOL PinEvent,
    _In_ ULONG PinId,
    _In_ BOOL NodeEvent,
    _In_ ULONG NodeId)
{
    ASSERT(m_PortEvents);
    m_PortEvents->GenerateEventList(Set, EventId, PinEvent, PinId, NodeEvent, NodeId);
}

NTSTATUS
HDAUDIO_NewMiniportTopology(
    _Out_ PMINIPORTTOPOLOGY* OutMiniport,
    _In_ PHDAUDIO_FILTER_CONTEXT FilterContext,
    _In_ CFunctionGroupNode * Node,
    _In_ CAdapterCommon * Adapter,
    _In_ PPCFILTER_DESCRIPTOR FilterDescription)
{
    CMiniportTopology *This;

    This = new (NonPagedPool, TAG_HDAUDIO) CMiniportTopology(NULL, FilterContext, Node, Adapter, FilterDescription);
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
