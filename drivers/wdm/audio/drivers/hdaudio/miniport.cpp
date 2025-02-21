/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/drivers/hdaudio/hdaudio.cpp
 * PURPOSE:         HDAudio Driver
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.h"

#define YDEBUG
#include <debug.h>

class CMiniportWaveRT : public CUnknownImpl<IMiniportWaveRT>
{
  public:
    STDMETHODIMP QueryInterface(REFIID InterfaceId, PVOID *Interface);
    IMP_IMiniportWaveRT;
    CMiniportWaveRT(
        IUnknown *OuterUnknown,
        ULONG AssociatedPinCount,
        PULONG AssociatedPinIds,
        CFunctionGroupNode *Node,
        CAdapterCommon *Adapter,
        PPCFILTER_DESCRIPTOR Filter)
        : m_AssociatedPinCount(AssociatedPinCount), m_Node(Node), m_AssociatedPins(AssociatedPinIds),
          m_FilterDescription(Filter), m_Port(0), m_Adapter(Adapter)
    {
    }
    virtual ~CMiniportWaveRT()
    {
    }

  private:
    ULONG m_AssociatedPinCount;
    PULONG m_AssociatedPins;
    CAdapterCommon *m_Adapter;
    CFunctionGroupNode *m_Node;
    PIN_CONFIGURATION_DEFAULT m_PinConfiguration;
    PPCFILTER_DESCRIPTOR m_FilterDescription;
    PPORTWAVERT m_Port;
    HDAUDIO_BUS_INTERFACE_V2 m_Interface;
};

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
    UNIMPLEMENTED_ONCE;
    return STATUS_NOT_IMPLEMENTED;
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
    return HDAUDIO_AllocateStream(Stream, m_Adapter, m_Node, Pin, Capture, DataFormat, m_AssociatedPinCount, m_AssociatedPins); 
}

NTSTATUS
NTAPI
CMiniportWaveRT::GetDeviceDescription(
    OUT PDEVICE_DESCRIPTION DeviceDescription)
{
    UNIMPLEMENTED_ONCE;
    return STATUS_NOT_IMPLEMENTED;
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
