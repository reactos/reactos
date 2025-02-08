/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/port_wavecyclic.cpp
 * PURPOSE:         WaveCyclic Port Driver
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.hpp"

#define NDEBUG
#include <debug.h>

GUID IID_IDmaChannelSlave;

class CPortWaveCyclic : public CUnknownImpl<IPortWaveCyclic, IPortEvents, ISubdevice>
{
public:
    STDMETHODIMP QueryInterface( REFIID InterfaceId, PVOID* Interface);

    IMP_IPortWaveCyclic;
    IMP_ISubdevice;
    IMP_IPortEvents;
    CPortWaveCyclic(IUnknown *OuterUnknown) {}
    virtual ~CPortWaveCyclic(){}

protected:
    PDEVICE_OBJECT m_pDeviceObject;
    PMINIPORTWAVECYCLIC m_pMiniport;
    PPINCOUNT m_pPinCount;
    PPOWERNOTIFY m_pPowerNotify;
    PPCFILTER_DESCRIPTOR m_pDescriptor;
    PSUBDEVICE_DESCRIPTOR m_SubDeviceDescriptor;
    IPortFilterWaveCyclic * m_Filter;

    friend PMINIPORTWAVECYCLIC GetWaveCyclicMiniport(IN IPortWaveCyclic* iface);
    friend PDEVICE_OBJECT GetDeviceObject(PPORTWAVECYCLIC iface);
};

GUID KSPROPERTY_SETID_Topology                = {0x720D4AC0L, 0x7533, 0x11D0, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};

static GUID InterfaceGuids[4] =
{
    {
         //KS_CATEGORY_AUDIO
        0x6994AD04, 0x93EF, 0x11D0, {0xA3, 0xCC, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}
    },
    {
        /// KSCATEGORY_RENDER
        0x65E8773EL, 0x8F56, 0x11D0, {0xA3, 0xB9, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}
    },
    {
        /// KSCATEGORY_CAPTURE
        0x65E8773DL, 0x8F56, 0x11D0, {0xA3, 0xB9, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}
    },
    {
        ///KSCATEGORY_AUDIO_DEVICE
        0xFBF6F530L, 0x07B9, 0x11D2, {0xA7, 0x1E, 0x00, 0x00, 0xF8, 0x00, 0x47, 0x88}
    }

};

DEFINE_KSPROPERTY_TOPOLOGYSET(PortFilterWaveCyclicTopologySet, TopologyPropertyHandler);
DEFINE_KSPROPERTY_PINPROPOSEDATAFORMAT(PortFilterWaveCyclicPinSet, PinPropertyHandler, PinPropertyHandler, PinPropertyHandler);

KSPROPERTY_SET WaveCyclicPropertySet[] =
{
    {
        &KSPROPSETID_Topology,
        sizeof(PortFilterWaveCyclicTopologySet) / sizeof(KSPROPERTY_ITEM),
        (const KSPROPERTY_ITEM*)&PortFilterWaveCyclicTopologySet,
        0,
        NULL
    },
    {
        &KSPROPSETID_Pin,
        sizeof(PortFilterWaveCyclicPinSet) / sizeof(KSPROPERTY_ITEM),
        (const KSPROPERTY_ITEM*)&PortFilterWaveCyclicPinSet,
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
CPortWaveCyclic::AddEventToEventList(
    IN PKSEVENT_ENTRY EventEntry)
{
    UNIMPLEMENTED;
}

void
NTAPI
CPortWaveCyclic::GenerateEventList(
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
CPortWaveCyclic::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    UNICODE_STRING GuidString;

    if (IsEqualGUIDAligned(refiid, IID_IPortWaveCyclic) ||
        IsEqualGUIDAligned(refiid, IID_IPort) ||
        IsEqualGUIDAligned(refiid, IID_IUnknown))
    {
        *Output = PVOID(PPORTWAVECYCLIC(this));
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
        DPRINT1("IPortWaveCyclic_fnQueryInterface no interface!!! iface %S\n", GuidString.Buffer);
        RtlFreeUnicodeString(&GuidString);
    }

    return STATUS_UNSUCCESSFUL;
}

//---------------------------------------------------------------
// IPort interface functions
//

NTSTATUS
NTAPI
CPortWaveCyclic::GetDeviceProperty(
    IN DEVICE_REGISTRY_PROPERTY  DeviceRegistryProperty,
    IN ULONG  BufferLength,
    OUT PVOID  PropertyBuffer,
    OUT PULONG  ReturnLength)
{
    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    return IoGetDeviceProperty(m_pDeviceObject, DeviceRegistryProperty, BufferLength, PropertyBuffer, ReturnLength);
}

NTSTATUS
NTAPI
CPortWaveCyclic::Init(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp,
    IN PUNKNOWN  UnknownMiniport,
    IN PUNKNOWN  UnknownAdapter  OPTIONAL,
    IN PRESOURCELIST  ResourceList)
{
    IMiniportWaveCyclic * Miniport;
    NTSTATUS Status;
    PPINCOUNT PinCount;
    PPOWERNOTIFY PowerNotify;
    PPCLASS_DEVICE_EXTENSION DeviceExtension;

    DPRINT("IPortWaveCyclic_Init entered %p\n", this);
    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    Status = UnknownMiniport->QueryInterface(IID_IMiniportWaveCyclic, (PVOID*)&Miniport);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IPortWaveCyclic_Init called with invalid IMiniport adapter\n");
        return STATUS_INVALID_PARAMETER;
    }

    // Initialize port object
    m_pMiniport = Miniport;
    m_pDeviceObject = DeviceObject;

    // initialize miniport
    Status = Miniport->Init(UnknownAdapter, ResourceList, this);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IMiniportWaveCyclic_Init failed with %x\n", Status);
        Miniport->Release();
        return Status;
    }

    // get the miniport device descriptor
    Status = Miniport->GetDescription(&m_pDescriptor);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("failed to get description\n");
        Miniport->Release();
        return Status;
    }

    // create the subdevice descriptor
    Status = PcCreateSubdeviceDescriptor(&m_SubDeviceDescriptor,
                                         4,
                                         InterfaceGuids,
                                         0,
                                         NULL,
                                         2,
                                         WaveCyclicPropertySet,
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
        // get device extension
        DeviceExtension = (PPCLASS_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
        PENTRY_POWER_NOTIFY Notify = (PENTRY_POWER_NOTIFY)AllocateItem(NonPagedPool, sizeof(ENTRY_POWER_NOTIFY), TAG_PORTCLASS);
        if (Notify)
        {
            KIRQL OldLevel;

            // setup item
            Notify->PowerNotify = PowerNotify;

            // acquire lock
            KeAcquireSpinLock(&DeviceExtension->PowerNotifyListLock, &OldLevel);

            // insert item
            InsertTailList(&DeviceExtension->PowerNotifyList, &Notify->Entry);

            // release lock
            KeReleaseSpinLock(&DeviceExtension->PowerNotifyListLock, OldLevel);
        }
        // store reference
        m_pPowerNotify = PowerNotify;
    }

    DPRINT("IPortWaveCyclic successfully initialized\n");
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CPortWaveCyclic::NewRegistryKey(
    OUT PREGISTRYKEY  *OutRegistryKey,
    IN PUNKNOWN  OuterUnknown  OPTIONAL,
    IN ULONG  RegistryKeyType,
    IN ACCESS_MASK  DesiredAccess,
    IN POBJECT_ATTRIBUTES  ObjectAttributes  OPTIONAL,
    IN ULONG  CreateOptions  OPTIONAL,
    OUT PULONG  Disposition  OPTIONAL)
{
    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    return PcNewRegistryKey(OutRegistryKey, OuterUnknown, RegistryKeyType, DesiredAccess, m_pDeviceObject, (ISubdevice*)this, ObjectAttributes, CreateOptions, Disposition);
}

//---------------------------------------------------------------
// IPortWaveCyclic interface functions
//

NTSTATUS
NTAPI
CPortWaveCyclic::NewMasterDmaChannel(
    OUT PDMACHANNEL* DmaChannel,
    IN  PUNKNOWN OuterUnknown,
    IN  PRESOURCELIST ResourceList OPTIONAL,
    IN  ULONG MaximumLength,
    IN  BOOLEAN Dma32BitAddresses,
    IN  BOOLEAN Dma64BitAddresses,
    IN  DMA_WIDTH DmaWidth,
    IN  DMA_SPEED DmaSpeed)
{
    NTSTATUS Status;
    DEVICE_DESCRIPTION DeviceDescription;

    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    Status = PcDmaMasterDescription(ResourceList, (Dma32BitAddresses | Dma64BitAddresses), Dma32BitAddresses, 0, Dma64BitAddresses, DmaWidth, DmaSpeed, MaximumLength, 0, &DeviceDescription);
    if (NT_SUCCESS(Status))
    {
        return PcNewDmaChannel(DmaChannel, OuterUnknown, NonPagedPool, &DeviceDescription, m_pDeviceObject);
    }

    return Status;
}

NTSTATUS
NTAPI
CPortWaveCyclic::NewSlaveDmaChannel(
    OUT PDMACHANNELSLAVE* OutDmaChannel,
    IN  PUNKNOWN OuterUnknown,
    IN  PRESOURCELIST ResourceList OPTIONAL,
    IN  ULONG DmaIndex,
    IN  ULONG MaximumLength,
    IN  BOOLEAN DemandMode,
    IN  DMA_SPEED DmaSpeed)
{
    DEVICE_DESCRIPTION DeviceDescription;
    PDMACHANNEL DmaChannel;
    NTSTATUS Status;

    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    // FIXME
    // Check for F-Type DMA Support
    //

    Status = PcDmaSlaveDescription(ResourceList, DmaIndex, DemandMode, TRUE, DmaSpeed, MaximumLength, 0, &DeviceDescription);
    if (NT_SUCCESS(Status))
    {
        Status = PcNewDmaChannel(&DmaChannel, OuterUnknown, NonPagedPool, &DeviceDescription, m_pDeviceObject);
        if (NT_SUCCESS(Status))
        {
            Status = DmaChannel->QueryInterface(IID_IDmaChannelSlave, (PVOID*)OutDmaChannel);
            DmaChannel->Release();
        }
    }

    return Status;
}

VOID
NTAPI
CPortWaveCyclic::Notify(
    IN  PSERVICEGROUP ServiceGroup)
{
    ServiceGroup->RequestService ();
}

//---------------------------------------------------------------
// ISubdevice interface
//

NTSTATUS
NTAPI
CPortWaveCyclic::NewIrpTarget(
    OUT struct IIrpTarget **OutTarget,
    IN PCWSTR Name,
    IN PUNKNOWN Unknown,
    IN POOL_TYPE PoolType,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN KSOBJECT_CREATE *CreateObject)
{
    NTSTATUS Status;
    IPortFilterWaveCyclic * Filter;

    DPRINT("ISubDevice_NewIrpTarget this %p\n", this);

    // is there already an instance of the filter
    if (m_Filter)
    {
        // it is, let's return the result
        *OutTarget = (IIrpTarget*)m_Filter;

        // increment reference
        m_Filter->AddRef();
        return STATUS_SUCCESS;
    }

    // create new instance of filter
    Status = NewPortFilterWaveCyclic(&Filter);
    if (!NT_SUCCESS(Status))
    {
        // not enough memory
        return Status;
    }

    // initialize the filter
    Status = Filter->Init((IPortWaveCyclic*)this);
    if (!NT_SUCCESS(Status))
    {
        // destroy filter
        Filter->Release();
        // return status
        return Status;
    }

    // store result
    *OutTarget = (IIrpTarget*)Filter;
    // store for later re-use
    m_Filter = Filter;
    // return status
    return Status;
}

NTSTATUS
NTAPI
CPortWaveCyclic::ReleaseChildren()
{
    DPRINT("ISubDevice_fnReleaseChildren\n");

    // release the filter
    m_Filter->Release();

    if (m_pPinCount)
    {
        // release pincount interface
        m_pPinCount->Release();
    }

    if (m_pPowerNotify)
    {
        // release power notify interface
        m_pPowerNotify->Release();
    }

    // now release the miniport
    m_pMiniport->Release();

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CPortWaveCyclic::GetDescriptor(
    IN SUBDEVICE_DESCRIPTOR ** Descriptor)
{
    PC_ASSERT(m_SubDeviceDescriptor != NULL);

    *Descriptor = m_SubDeviceDescriptor;

    DPRINT("ISubDevice_GetDescriptor this %p desc %p\n", this, m_SubDeviceDescriptor);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CPortWaveCyclic::DataRangeIntersection(
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
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
CPortWaveCyclic::PowerChangeNotify(
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
CPortWaveCyclic::PinCount(
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

    // scan filter descriptor
    if (m_SubDeviceDescriptor && m_SubDeviceDescriptor->DeviceDescriptor)
    {
        if (PinId >= m_SubDeviceDescriptor->DeviceDescriptor->PinCount)
        {
            DPRINT1("PinId %u outside MaxPins %u\n", PinId, m_SubDeviceDescriptor->DeviceDescriptor->PinCount);
            return STATUS_UNSUCCESSFUL;
        }
        ASSERT(m_SubDeviceDescriptor->DeviceDescriptor->PinSize >= sizeof(PCPIN_DESCRIPTOR));

        ULONG_PTR Offset = PinId * m_SubDeviceDescriptor->DeviceDescriptor->PinSize;
        PPCPIN_DESCRIPTOR Descriptor = (PPCPIN_DESCRIPTOR)((ULONG_PTR)m_SubDeviceDescriptor->DeviceDescriptor->Pins + Offset);
        *FilterPossible = Descriptor->MaxFilterInstanceCount;
        *FilterNecessary = Descriptor->MinFilterInstanceCount;
        *FilterCurrent = m_SubDeviceDescriptor->Factory.Instances[PinId].CurrentPinInstanceCount;
        *GlobalCurrent = m_SubDeviceDescriptor->Factory.Instances[PinId].CurrentPinInstanceCount;
        *GlobalPossible = Descriptor->MaxGlobalInstanceCount;
        return STATUS_SUCCESS;
    }



    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

///--------------------------------------------------------------

PMINIPORTWAVECYCLIC
GetWaveCyclicMiniport(
    IN IPortWaveCyclic* iface)
{
    CPortWaveCyclic * This = (CPortWaveCyclic *)iface;
    return This->m_pMiniport;
}

PDEVICE_OBJECT
GetDeviceObject(
    PPORTWAVECYCLIC iface)
{
    CPortWaveCyclic * This = (CPortWaveCyclic *)iface;
    return This->m_pDeviceObject;
}

//---------------------------------------------------------------
// IPortWaveCyclic constructor
//

NTSTATUS
NewPortWaveCyclic(
    OUT PPORT* OutPort)
{
    NTSTATUS Status;
    CPortWaveCyclic * Port;

    Port = new(NonPagedPool, TAG_PORTCLASS)CPortWaveCyclic(NULL);
    if (!Port)
        return STATUS_INSUFFICIENT_RESOURCES;

    Status = Port->QueryInterface(IID_IPort, (PVOID*)OutPort);

    if (!NT_SUCCESS(Status))
    {
        delete Port;
    }

    DPRINT1("NewPortWaveCyclic %p Status %u\n", Port, Status);
    return Status;
}
