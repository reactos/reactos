/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/port_wavert.cpp
 * PURPOSE:         WaveRT Port Driver
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.hpp"

#define NDEBUG
#include <debug.h>

class CPortWaveRT : public CUnknownImpl<IPortWaveRT, IPortEvents, ISubdevice>
{
public:
    STDMETHODIMP QueryInterface( REFIID InterfaceId, PVOID* Interface);

    IMP_IPortWaveRT;
    IMP_ISubdevice;
    IMP_IPortEvents;
    CPortWaveRT(IUnknown *OuterUnknown) {}
    virtual ~CPortWaveRT() {}

protected:

    BOOL m_bInitialized;
    PDEVICE_OBJECT m_pDeviceObject;
    PMINIPORTWAVERT m_pMiniport;
    PRESOURCELIST m_pResourceList;
    PPINCOUNT m_pPinCount;
    PPOWERNOTIFY m_pPowerNotify;
    PPCFILTER_DESCRIPTOR m_pDescriptor;
    PSUBDEVICE_DESCRIPTOR m_SubDeviceDescriptor;
    IPortFilterWaveRT * m_Filter;

    friend PMINIPORTWAVERT GetWaveRTMiniport(IN IPortWaveRT* iface);
    friend PDEVICE_OBJECT GetDeviceObjectFromPortWaveRT(PPORTWAVERT iface);
};

static GUID InterfaceGuids[3] =
{
    {
        /// KS_CATEGORY_AUDIO
        0x6994AD04, 0x93EF, 0x11D0, {0xA3, 0xCC, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}
    },
    {
        /// KSCATEGORY_RENDER
        0x65E8773EL, 0x8F56, 0x11D0, {0xA3, 0xB9, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}
    },
    {
        /// KSCATEGORY_CAPTURE
        0x65E8773DL, 0x8F56, 0x11D0, {0xA3, 0xB9, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}
    }
};

DEFINE_KSPROPERTY_TOPOLOGYSET(PortFilterWaveRTTopologySet, TopologyPropertyHandler);
DEFINE_KSPROPERTY_PINPROPOSEDATAFORMAT(PortFilterWaveRTPinSet, PinPropertyHandler, PinPropertyHandler, PinPropertyHandler);

KSPROPERTY_SET WaveRTPropertySet[] =
{
    {
        &KSPROPSETID_Topology,
        sizeof(PortFilterWaveRTTopologySet) / sizeof(KSPROPERTY_ITEM),
        (const KSPROPERTY_ITEM*)&PortFilterWaveRTTopologySet,
        0,
        NULL
    },
    {
        &KSPROPSETID_Pin,
        sizeof(PortFilterWaveRTPinSet) / sizeof(KSPROPERTY_ITEM),
        (const KSPROPERTY_ITEM*)&PortFilterWaveRTPinSet,
        0,
        NULL
    }
};

//KSEVENTSETID_LoopedStreaming, Type = KSEVENT_LOOPEDSTREAMING_POSITION
//KSEVENTSETID_Connection, Type = KSEVENT_CONNECTION_ENDOFSTREAM,

//---------------------------------------------------------------
// IPortEvents
//

void
NTAPI
CPortWaveRT::AddEventToEventList(
    IN PKSEVENT_ENTRY EventEntry)
{
    UNIMPLEMENTED;
}

void
NTAPI
CPortWaveRT::GenerateEventList(
    IN  GUID* Set OPTIONAL,
    IN  ULONG EventId,
    IN  BOOL PinEvent,
    IN  ULONG PinId,
    IN  BOOL NodeEvent,
    IN  ULONG NodeId)
{
    UNIMPLEMENTED;
}

//---------------------------------------------------------------
// IUnknown interface functions
//

NTSTATUS
NTAPI
CPortWaveRT::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    UNICODE_STRING GuidString;

    if (IsEqualGUIDAligned(refiid, IID_IPortWaveRT) ||
        IsEqualGUIDAligned(refiid, IID_IPort) ||
        IsEqualGUIDAligned(refiid, IID_IUnknown))
    {
        *Output = PVOID(PPORTWAVERT(this));
        PUNKNOWN(*Output)->AddRef();
        return STATUS_SUCCESS;
    }
    else if (IsEqualGUIDAligned(refiid, IID_IPortEvents))
    {
        *Output = PVOID(PPORTEVENTS(this));
        PUNKNOWN(*Output)->AddRef();
        return STATUS_SUCCESS;
    }
    else if (IsEqualGUIDAligned(refiid, IID_ISubdevice))
    {
        *Output = PVOID(PSUBDEVICE(this));
        PUNKNOWN(*Output)->AddRef();
        return STATUS_SUCCESS;
    }
    else if (IsEqualGUIDAligned(refiid, IID_IPortClsVersion))
    {
        return NewPortClsVersion((PPORTCLSVERSION*)Output);
    }
    else if (IsEqualGUIDAligned(refiid, IID_IDrmPort) ||
             IsEqualGUIDAligned(refiid, IID_IDrmPort2))
    {
        return NewIDrmPort((PDRMPORT2*)Output);
    }
    else if (IsEqualGUIDAligned(refiid, IID_IUnregisterSubdevice))
    {
        return NewIUnregisterSubdevice((PUNREGISTERSUBDEVICE*)Output);
    }
    else if (IsEqualGUIDAligned(refiid, IID_IUnregisterPhysicalConnection))
    {
        return NewIUnregisterPhysicalConnection((PUNREGISTERPHYSICALCONNECTION*)Output);
    }

    if (RtlStringFromGUID(refiid, &GuidString) == STATUS_SUCCESS)
    {
        DPRINT1("IPortWaveRT_fnQueryInterface no interface!!! iface %S\n", GuidString.Buffer);
        RtlFreeUnicodeString(&GuidString);
    }

    return STATUS_UNSUCCESSFUL;
}
//---------------------------------------------------------------
// IPort interface functions
//

NTSTATUS
NTAPI
CPortWaveRT::GetDeviceProperty(
    IN DEVICE_REGISTRY_PROPERTY  DeviceRegistryProperty,
    IN ULONG  BufferLength,
    OUT PVOID  PropertyBuffer,
    OUT PULONG  ReturnLength)
{
    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (!m_bInitialized)
    {
        DPRINT("IPortWaveRT_fnNewRegistryKey called w/o initialized\n");
        return STATUS_UNSUCCESSFUL;
    }

    return IoGetDeviceProperty(m_pDeviceObject, DeviceRegistryProperty, BufferLength, PropertyBuffer, ReturnLength);
}

NTSTATUS
NTAPI
CPortWaveRT::Init(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp,
    IN PUNKNOWN  UnknownMiniport,
    IN PUNKNOWN  UnknownAdapter  OPTIONAL,
    IN PRESOURCELIST  ResourceList)
{
    IMiniportWaveRT * Miniport;
    NTSTATUS Status;
    PPINCOUNT PinCount;
    PPOWERNOTIFY PowerNotify;

    DPRINT("IPortWaveRT_Init entered %p DevObject %p Irp %p UnknownMiniport %p UnknownAdapter %p ResourceList %p\n", this,
        DeviceObject, Irp, UnknownMiniport, UnknownAdapter, ResourceList);
    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (m_bInitialized)
    {
        DPRINT("IPortWaveRT_Init called again\n");
        return STATUS_SUCCESS;
    }

    Status = UnknownMiniport->QueryInterface(IID_IMiniportWaveRT, (PVOID*)&Miniport);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IPortWaveRT_Init called with invalid IMiniport adapter\n");
        return STATUS_INVALID_PARAMETER;
    }

    // Initialize port object
    m_pMiniport = Miniport;
    m_pDeviceObject = DeviceObject;
    m_bInitialized = TRUE;
    m_pResourceList = ResourceList;

    // increment reference on miniport adapter
    Miniport->AddRef();

    Status = Miniport->Init(UnknownAdapter, ResourceList, this);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IMiniportWaveRT_Init failed with %x\n", Status);
        Miniport->Release();
        m_bInitialized = FALSE;
        return Status;
    }

    // get the miniport device descriptor
    Status = Miniport->GetDescription(&m_pDescriptor);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("failed to get description\n");
        Miniport->Release();
        m_bInitialized = FALSE;
        return Status;
    }

    // create the subdevice descriptor
    Status = PcCreateSubdeviceDescriptor(&m_SubDeviceDescriptor,
                                         3,
                                         InterfaceGuids,
                                         0,
                                         NULL,
                                         2,
                                         WaveRTPropertySet,
                                         0,
                                         0,
                                         0,
                                         NULL,
                                         0,
                                         NULL,
                                         m_pDescriptor);

    if (!NT_SUCCESS(Status))
    {
        DPRINT("PcCreateSubdeviceDescriptor failed with %x\n", Status);
        Miniport->Release();
        m_bInitialized = FALSE;
        return Status;
    }

    // check if it supports IPinCount interface
    Status = UnknownMiniport->QueryInterface(IID_IPinCount, (PVOID*)&PinCount);
    if (NT_SUCCESS(Status))
    {
        // store IPinCount interface
        m_pPinCount = PinCount;
    }

    // does the Miniport adapter support IPowerNotify interface*/
    Status = UnknownMiniport->QueryInterface(IID_IPowerNotify, (PVOID*)&PowerNotify);
    if (NT_SUCCESS(Status))
    {
        // store reference
        m_pPowerNotify = PowerNotify;
    }

    // increment reference on resource list
    ResourceList->AddRef();

    DPRINT("IPortWaveRT successfully initialized\n");
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CPortWaveRT::NewRegistryKey(
    OUT PREGISTRYKEY  *OutRegistryKey,
    IN PUNKNOWN  OuterUnknown  OPTIONAL,
    IN ULONG  RegistryKeyType,
    IN ACCESS_MASK  DesiredAccess,
    IN POBJECT_ATTRIBUTES  ObjectAttributes  OPTIONAL,
    IN ULONG  CreateOptions  OPTIONAL,
    OUT PULONG  Disposition  OPTIONAL)
{
    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (!m_bInitialized)
    {
        DPRINT("IPortWaveRT_fnNewRegistryKey called w/o initialized\n");
        return STATUS_UNSUCCESSFUL;
    }
    return PcNewRegistryKey(OutRegistryKey, OuterUnknown, RegistryKeyType, DesiredAccess, m_pDeviceObject, (ISubdevice*)this, ObjectAttributes, CreateOptions, Disposition);
}
//---------------------------------------------------------------
// ISubdevice interface
//

NTSTATUS
NTAPI
CPortWaveRT::NewIrpTarget(
    OUT struct IIrpTarget **OutTarget,
    IN PCWSTR Name,
    IN PUNKNOWN Unknown,
    IN POOL_TYPE PoolType,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN KSOBJECT_CREATE *CreateObject)
{
    NTSTATUS Status;
    IPortFilterWaveRT * Filter;

    DPRINT("ISubDevice_NewIrpTarget this %p\n", this);

    if (m_Filter)
    {
        *OutTarget = (IIrpTarget*)m_Filter;
        return STATUS_SUCCESS;
    }

    Status = NewPortFilterWaveRT(&Filter);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = Filter->Init(this);
    if (!NT_SUCCESS(Status))
    {
        Filter->Release();
        return Status;
    }

    *OutTarget = (IIrpTarget*)Filter;
    m_Filter = Filter;
    return Status;
}

NTSTATUS
NTAPI
CPortWaveRT::ReleaseChildren()
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
CPortWaveRT::GetDescriptor(
    IN SUBDEVICE_DESCRIPTOR ** Descriptor)
{
    PC_ASSERT(m_SubDeviceDescriptor != NULL);

    *Descriptor = m_SubDeviceDescriptor;

    DPRINT("ISubDevice_GetDescriptor this %p desc %p\n", this, m_SubDeviceDescriptor);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CPortWaveRT::DataRangeIntersection(
    IN  ULONG PinId,
    IN  PKSDATARANGE DataRange,
    IN  PKSDATARANGE MatchingDataRange,
    IN  ULONG OutputBufferLength,
    OUT PVOID ResultantFormat OPTIONAL,
    OUT PULONG ResultantFormatLength)
{
    DPRINT("ISubDevice_DataRangeIntersection this %p\n", this);

    if (m_pMiniport)
    {
        return m_pMiniport->DataRangeIntersection (PinId, DataRange, MatchingDataRange, OutputBufferLength, ResultantFormat, ResultantFormatLength);
    }

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
CPortWaveRT::PowerChangeNotify(
    IN POWER_STATE PowerState)
{
    if (m_pPowerNotify)
    {
        m_pPowerNotify->PowerChangeNotify(PowerState);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CPortWaveRT::PinCount(
    IN ULONG  PinId,
    IN OUT PULONG  FilterNecessary,
    IN OUT PULONG  FilterCurrent,
    IN OUT PULONG  FilterPossible,
    IN OUT PULONG  GlobalCurrent,
    IN OUT PULONG  GlobalPossible)
{
    if (m_pPinCount)
    {
       m_pPinCount->PinCount(PinId, FilterNecessary, FilterCurrent, FilterPossible, GlobalCurrent, GlobalPossible);
       return STATUS_SUCCESS;
    }

    // FIXME
    // scan filter descriptor

    return STATUS_UNSUCCESSFUL;
}

///--------------------------------------------------------------
PMINIPORTWAVERT
GetWaveRTMiniport(
    IN IPortWaveRT* iface)
{
    CPortWaveRT * This = (CPortWaveRT *)iface;
    return This->m_pMiniport;
}

PDEVICE_OBJECT
GetDeviceObjectFromPortWaveRT(
    PPORTWAVERT iface)
{
    CPortWaveRT * This = (CPortWaveRT *)iface;
    return This->m_pDeviceObject;
}

//---------------------------------------------------------------
// IPortWaveRT constructor
//

NTSTATUS
NewPortWaveRT(
    OUT PPORT* OutPort)
{
    CPortWaveRT * Port;
    NTSTATUS Status;

    Port = new(NonPagedPool, TAG_PORTCLASS) CPortWaveRT(NULL);
    if (!Port)
        return STATUS_INSUFFICIENT_RESOURCES;

    Status = Port->QueryInterface(IID_IPort, (PVOID*)OutPort);

    if (!NT_SUCCESS(Status))
    {
        delete Port;
    }

    DPRINT("NewPortWaveRT %p Status %u\n", Port, Status);
    return Status;
}
