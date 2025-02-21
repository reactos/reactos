/*
 * PROJECT:         ReactOS HDAudio Driver
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Topology miniport
 * COPYRIGHT:       Copyright 2025-2026 Oleg Dubinskiy <oleg.dubinskiy@reactos.org>
 */

#include "private.h"

#define NDEBUG
#include <debug.h>

NTSTATUS
NTAPI
CMiniportTopology::QueryInterface(IN REFIID refiid, OUT PVOID *Output)
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
        DPRINT1("CMiniportTopology::QueryInterface no interface!!! iface %S\n", GuidString.Buffer);
        RtlFreeUnicodeString(&GuidString);
    }
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
CMiniportTopology::GetDescription(OUT PPCFILTER_DESCRIPTOR* Description)
{
    *Description = m_FilterDescription;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CMiniportTopology::DataRangeIntersection(
    IN ULONG PinId,
    IN PKSDATARANGE DataRange,
    IN PKSDATARANGE MatchingDataRange,
    IN ULONG OutputBufferLength,
    OUT PVOID ResultantFormat OPTIONAL,
    OUT PULONG ResultantFormatLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CMiniportTopology::Init(
    IN PUNKNOWN UnknownAdapter,
    IN PRESOURCELIST ResourceList,
    IN PPORTTOPOLOGY Port)
{
    NTSTATUS Status;

    Status = Port->QueryInterface(IID_IPortEvents,
                                  (PVOID*)&m_PortEvents);
    if (!NT_SUCCESS(Status))
        return Status;

    m_Port = Port;
    m_Port->AddRef();
    return STATUS_SUCCESS;
}

VOID
NTAPI
CMiniportTopology::AddEventToEventList(
    IN PKSEVENT_ENTRY EventEntry)
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
    OUT PMINIPORTTOPOLOGY* OutMiniport,
    IN ULONG AssociatedPinsCount,
    IN PULONG AssociatedPinIds,
    IN CFunctionGroupNode * Node,
    IN CAdapterCommon * Adapter,
    IN PPCFILTER_DESCRIPTOR FilterDescription)
{
    CMiniportTopology *This;

    This = new (NonPagedPool, TAG_HDAUDIO) CMiniportTopology(NULL, AssociatedPinsCount, AssociatedPinIds, Node, Adapter, FilterDescription);
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
