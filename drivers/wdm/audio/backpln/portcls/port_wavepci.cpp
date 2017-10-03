/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/port_wavepci.cpp
 * PURPOSE:         Wave PCI Port driver
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.hpp"

#ifndef YDEBUG
#define NDEBUG
#endif

#include <debug.h>

class CPortWavePci : public IPortWavePci,
                     public IPortEvents,
                     public ISubdevice,
                     public IServiceSink
{
public:
    STDMETHODIMP QueryInterface( REFIID InterfaceId, PVOID* Interface);

    STDMETHODIMP_(ULONG) AddRef()
    {
        InterlockedIncrement(&m_Ref);
        return m_Ref;
    }
    STDMETHODIMP_(ULONG) Release()
    {
        InterlockedDecrement(&m_Ref);

        if (!m_Ref)
        {
            delete this;
            return 0;
        }
        return m_Ref;
    }
    IMP_IPortWavePci;
    IMP_ISubdevice;
    IMP_IPortEvents;
    IMP_IServiceSink;
    CPortWavePci(IUnknown *OuterUnknown){}
    virtual ~CPortWavePci() {}

protected:

    PMINIPORTWAVEPCI m_Miniport;
    PDEVICE_OBJECT m_pDeviceObject;
    PSERVICEGROUP m_ServiceGroup;
    PPINCOUNT m_pPinCount;
    PPOWERNOTIFY m_pPowerNotify;
    PPCFILTER_DESCRIPTOR m_pDescriptor;
    PSUBDEVICE_DESCRIPTOR m_SubDeviceDescriptor;
    IPortFilterWavePci * m_Filter;

    LIST_ENTRY m_EventList;
    KSPIN_LOCK m_EventListLock;

    LONG m_Ref;

    friend PDEVICE_OBJECT GetDeviceObjectFromPortWavePci(IPortWavePci* iface);
    friend PMINIPORTWAVEPCI GetWavePciMiniport(PPORTWAVEPCI iface);

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

DEFINE_KSPROPERTY_TOPOLOGYSET(PortFilterWavePciTopologySet, TopologyPropertyHandler);
DEFINE_KSPROPERTY_PINPROPOSEDATAFORMAT(PortFilterWavePciPinSet, PinPropertyHandler, PinPropertyHandler, PinPropertyHandler);

KSPROPERTY_SET WavePciPropertySet[] =
{
    {
        &KSPROPSETID_Topology,
        sizeof(PortFilterWavePciTopologySet) / sizeof(KSPROPERTY_ITEM),
        (const KSPROPERTY_ITEM*)&PortFilterWavePciTopologySet,
        0,
        NULL
    },
    {
        &KSPROPSETID_Pin,
        sizeof(PortFilterWavePciPinSet) / sizeof(KSPROPERTY_ITEM),
        (const KSPROPERTY_ITEM*)&PortFilterWavePciPinSet,
        0,
        NULL
    }
};


//---------------------------------------------------------------
// IPortEvents
//

void
NTAPI
CPortWavePci::AddEventToEventList(
    IN PKSEVENT_ENTRY EventEntry)
{
    KIRQL OldIrql;

    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    KeAcquireSpinLock(&m_EventListLock, &OldIrql);
    InsertTailList(&m_EventList, &EventEntry->ListEntry);
    KeReleaseSpinLock(&m_EventListLock, OldIrql);
}



void
NTAPI
CPortWavePci::GenerateEventList(
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
// IServiceSink
//

VOID
NTAPI
CPortWavePci::RequestService()
{
    //DPRINT("IServiceSink_fnRequestService entered\n");
    if (m_Miniport)
    {
        m_Miniport->Service();
    }
}

//---------------------------------------------------------------
// IPortWavePci
//

NTSTATUS
NTAPI
CPortWavePci::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    UNICODE_STRING GuidString;

    DPRINT("IPortWavePci_fnQueryInterface entered\n");

    if (IsEqualGUIDAligned(refiid, IID_IPortWavePci) ||
        IsEqualGUIDAligned(refiid, IID_IUnknown) ||
        IsEqualGUIDAligned(refiid, IID_IPort))
    {
        *Output = PVOID(PPORTWAVEPCI(this));
        PUNKNOWN(*Output)->AddRef();
        return STATUS_SUCCESS;
    }
    else if (IsEqualGUIDAligned(refiid, IID_IServiceSink))
    {
        *Output = PVOID(PSERVICESINK(this));
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
    else if (IsEqualGUIDAligned(refiid, IID_IDrmPort) ||
             IsEqualGUIDAligned(refiid, IID_IDrmPort2))
    {
        return NewIDrmPort((PDRMPORT2*)Output);
    }
    else if (IsEqualGUIDAligned(refiid, IID_IPortClsVersion))
    {
        return NewPortClsVersion((PPORTCLSVERSION*)Output);
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
        DPRINT("IPortWavePci_fnQueryInterface no interface!!! iface %S\n", GuidString.Buffer);
        RtlFreeUnicodeString(&GuidString);
    }

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
CPortWavePci::Init(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp,
    IN PUNKNOWN  UnknownMiniport,
    IN PUNKNOWN  UnknownAdapter  OPTIONAL,
    IN PRESOURCELIST  ResourceList)
{
    IMiniportWavePci * Miniport;
    PSERVICEGROUP ServiceGroup = 0;
    NTSTATUS Status;
    PPINCOUNT PinCount;
    PPOWERNOTIFY PowerNotify;

    DPRINT("IPortWavePci_fnInit entered with This %p, DeviceObject %p Irp %p UnknownMiniport %p, UnknownAdapter %p ResourceList %p\n", 
            this, DeviceObject, Irp, UnknownMiniport, UnknownAdapter, ResourceList);
    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    Status = UnknownMiniport->QueryInterface(IID_IMiniportWavePci, (PVOID*)&Miniport);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IPortWavePci_fnInit called with invalid IMiniport adapter\n");
        return STATUS_INVALID_PARAMETER;
    }

    // Initialize port object
    m_Miniport = Miniport;
    m_pDeviceObject = DeviceObject;

    InitializeListHead(&m_EventList);
    KeInitializeSpinLock(&m_EventListLock);

    // increment reference on miniport adapter
    Miniport->AddRef();

    Status = Miniport->Init(UnknownAdapter, ResourceList, this, &ServiceGroup);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IPortWavePci_fnInit failed with %x\n", Status);

        // release reference on miniport adapter
        Miniport->Release();
        return Status;
    }

    // check if the miniport adapter provides a valid device descriptor
    Status = Miniport->GetDescription(&m_pDescriptor);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("failed to get description\n");
        Miniport->Release();
        return Status;
    }

   // create the subdevice descriptor
    Status = PcCreateSubdeviceDescriptor(&m_SubDeviceDescriptor, 
                                         3,
                                         InterfaceGuids,
                                         0, 
                                         NULL,
                                         2, 
                                         WavePciPropertySet,
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
        return Status;
    }

    // did we get a service group
   if (ServiceGroup)
    {
        // store service group in context
        m_ServiceGroup = ServiceGroup;

        // add ourselves to service group which is called when miniport receives an isr
        m_ServiceGroup->AddMember(PSERVICESINK(this));

        // increment reference on service group
        m_ServiceGroup->AddRef();
    }

    // store for node property requests
    m_SubDeviceDescriptor->UnknownMiniport = UnknownMiniport;

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

    DPRINT("IPortWavePci_Init successfully initialized\n");
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CPortWavePci::NewRegistryKey(
    OUT PREGISTRYKEY  *OutRegistryKey,
    IN PUNKNOWN  OuterUnknown  OPTIONAL,
    IN ULONG  RegistryKeyType,
    IN ACCESS_MASK  DesiredAccess,
    IN POBJECT_ATTRIBUTES  ObjectAttributes  OPTIONAL,
    IN ULONG  CreateOptions  OPTIONAL,
    OUT PULONG  Disposition  OPTIONAL)
{
    DPRINT("IPortWavePci_fnNewRegistryKey entered\n");
    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    return PcNewRegistryKey(OutRegistryKey, 
                            OuterUnknown,
                            RegistryKeyType,
                            DesiredAccess,
                            m_pDeviceObject,
                            (ISubdevice*)this,
                            ObjectAttributes,
                            CreateOptions,
                            Disposition);
}

NTSTATUS
NTAPI
CPortWavePci::GetDeviceProperty(
    IN DEVICE_REGISTRY_PROPERTY  DeviceRegistryProperty,
    IN ULONG  BufferLength,
    OUT PVOID  PropertyBuffer,
    OUT PULONG  ReturnLength)
{
    DPRINT("IPortWavePci_fnGetDeviceProperty entered\n");
    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    return IoGetDeviceProperty(m_pDeviceObject, DeviceRegistryProperty, BufferLength, PropertyBuffer, ReturnLength);
}

NTSTATUS
NTAPI
CPortWavePci::NewMasterDmaChannel(
    OUT PDMACHANNEL *DmaChannel,
    IN PUNKNOWN OuterUnknown OPTIONAL,
    IN POOL_TYPE PoolType,
    IN PRESOURCELIST ResourceList OPTIONAL,
    IN BOOLEAN ScatterGather,
    IN BOOLEAN Dma32BitAddresses,
    IN BOOLEAN Dma64BitAddresses,
    IN BOOLEAN IgnoreCount,
    IN DMA_WIDTH DmaWidth,
    IN DMA_SPEED DmaSpeed,
    IN ULONG  MaximumLength,
    IN ULONG  DmaPort)
{
    NTSTATUS Status;
    DEVICE_DESCRIPTION DeviceDescription;

    DPRINT("IPortWavePci_fnNewMasterDmaChannel This %p entered\n", this);
    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    Status = PcDmaMasterDescription(ResourceList, ScatterGather, Dma32BitAddresses, IgnoreCount, Dma64BitAddresses, DmaWidth, DmaSpeed, MaximumLength, DmaPort, &DeviceDescription);
    if (NT_SUCCESS(Status))
    {
        return PcNewDmaChannel(DmaChannel, OuterUnknown, PoolType, &DeviceDescription, m_pDeviceObject);
    }

    return Status;
}

VOID
NTAPI
CPortWavePci::Notify(
    IN  PSERVICEGROUP ServiceGroup)
{
    //DPRINT("IPortWavePci_fnNotify entered %p, ServiceGroup %p\n", This, ServiceGroup);

    if (ServiceGroup)
    {
        ServiceGroup->RequestService ();
    }
}

//---------------------------------------------------------------
// ISubdevice interface
//

NTSTATUS
NTAPI
CPortWavePci::NewIrpTarget(
    OUT struct IIrpTarget **OutTarget,
    IN PCWSTR Name,
    IN PUNKNOWN Unknown,
    IN POOL_TYPE PoolType,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp, 
    IN KSOBJECT_CREATE *CreateObject)
{
    NTSTATUS Status;
    IPortFilterWavePci * Filter;

    DPRINT("ISubDevice_NewIrpTarget this %p\n", this);

    if (m_Filter)
    {
        *OutTarget = (IIrpTarget*)m_Filter;
        return STATUS_SUCCESS;
    }

    Status = NewPortFilterWavePci(&Filter);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = Filter->Init((IPortWavePci*)this);
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
CPortWavePci::ReleaseChildren()
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
CPortWavePci::GetDescriptor(
    IN SUBDEVICE_DESCRIPTOR ** Descriptor)
{
    DPRINT("ISubDevice_GetDescriptor this %p\n", this);
    *Descriptor = m_SubDeviceDescriptor;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CPortWavePci::DataRangeIntersection(
    IN  ULONG PinId,
    IN  PKSDATARANGE DataRange,
    IN  PKSDATARANGE MatchingDataRange,
    IN  ULONG OutputBufferLength,
    OUT PVOID ResultantFormat OPTIONAL,
    OUT PULONG ResultantFormatLength)
{
    DPRINT("ISubDevice_DataRangeIntersection this %p\n", this);

    if (m_Miniport)
    {
        return m_Miniport->DataRangeIntersection (PinId, DataRange, MatchingDataRange, OutputBufferLength, ResultantFormat, ResultantFormatLength);
    }

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
CPortWavePci::PowerChangeNotify(
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
CPortWavePci::PinCount(
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


NTSTATUS
NewPortWavePci(
    OUT PPORT* OutPort)
{
    CPortWavePci * Port;
    NTSTATUS Status;

    Port = new(NonPagedPool, TAG_PORTCLASS) CPortWavePci(NULL);
    if (!Port)
        return STATUS_INSUFFICIENT_RESOURCES;

    Status = Port->QueryInterface(IID_IPort, (PVOID*)OutPort);

    if (!NT_SUCCESS(Status))
    {
        delete Port;
    }

    DPRINT("NewPortWavePci %p Status %u\n", Port, Status);
    return Status;

}


PDEVICE_OBJECT
GetDeviceObjectFromPortWavePci(
    IPortWavePci* iface)
{
    CPortWavePci * This = (CPortWavePci*)iface;
    return This->m_pDeviceObject;
}

PMINIPORTWAVEPCI
GetWavePciMiniport(
    PPORTWAVEPCI iface)
{
    CPortWavePci * This = (CPortWavePci*)iface;
    return This->m_Miniport;
}
