/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/port_topology.cpp
 * PURPOSE:         Topology Port driver
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.hpp"

#define NDEBUG
#include <debug.h>

class CPortTopology : public CUnknownImpl<IPortTopology, ISubdevice, IPortEvents>
{
public:
    STDMETHODIMP QueryInterface( REFIID InterfaceId, PVOID* Interface);

    IMP_IPortTopology;
    IMP_ISubdevice;
    IMP_IPortEvents;
    CPortTopology(IUnknown *OuterUnknown){}
    virtual ~CPortTopology(){}

protected:
    BOOL m_bInitialized;

    PMINIPORTTOPOLOGY m_pMiniport;
    PDEVICE_OBJECT m_pDeviceObject;
    PPINCOUNT m_pPinCount;
    PPOWERNOTIFY m_pPowerNotify;

    PPCFILTER_DESCRIPTOR m_pDescriptor;
    PSUBDEVICE_DESCRIPTOR m_SubDeviceDescriptor;
    IPortFilterTopology * m_Filter;

    friend PMINIPORTTOPOLOGY GetTopologyMiniport(PPORTTOPOLOGY Port);

};

static GUID InterfaceGuids[2] =
{
    {
        /// KS_CATEGORY_AUDIO
        0x6994AD04, 0x93EF, 0x11D0, {0xA3, 0xCC, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}
    },
    {
        /// KS_CATEGORY_TOPOLOGY
        0xDDA54A40, 0x1E4C, 0x11D1, {0xA0, 0x50, 0x40, 0x57, 0x05, 0xC1, 0x00, 0x00}
    }
};

DEFINE_KSPROPERTY_TOPOLOGYSET(PortFilterTopologyTopologySet, TopologyPropertyHandler);
DEFINE_KSPROPERTY_PINPROPOSEDATAFORMAT(PortFilterTopologyPinSet, PinPropertyHandler, PinPropertyHandler, PinPropertyHandler);

KSPROPERTY_SET TopologyPropertySet[] =
{
    {
        &KSPROPSETID_Topology,
        sizeof(PortFilterTopologyTopologySet) / sizeof(KSPROPERTY_ITEM),
        (const KSPROPERTY_ITEM*)&PortFilterTopologyTopologySet,
        0,
        NULL
    },
    {
        &KSPROPSETID_Pin,
        sizeof(PortFilterTopologyPinSet) / sizeof(KSPROPERTY_ITEM),
        (const KSPROPERTY_ITEM*)&PortFilterTopologyPinSet,
        0,
        NULL
    }
};

//---------------------------------------------------------------
// IPortEvents
//

void
NTAPI
CPortTopology::AddEventToEventList(
    IN PKSEVENT_ENTRY EventEntry)
{
    UNIMPLEMENTED;
}

void
NTAPI
CPortTopology::GenerateEventList(
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
CPortTopology::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    UNICODE_STRING GuidString;

    DPRINT("IPortTopology_fnQueryInterface\n");

    if (IsEqualGUIDAligned(refiid, IID_IPortTopology) ||
        IsEqualGUIDAligned(refiid, IID_IPort) ||
        IsEqualGUIDAligned(refiid, IID_IUnknown))
    {
        *Output = PVOID(PUNKNOWN((IPortTopology*)this));
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
        DPRINT1("IPortTopology_fnQueryInterface no interface!!! iface %S\n", GuidString.Buffer);
        RtlFreeUnicodeString(&GuidString);
    }
    return STATUS_UNSUCCESSFUL;
}

//---------------------------------------------------------------
// IPort interface functions
//

NTSTATUS
NTAPI
CPortTopology::GetDeviceProperty(
    IN DEVICE_REGISTRY_PROPERTY  DeviceRegistryProperty,
    IN ULONG  BufferLength,
    OUT PVOID  PropertyBuffer,
    OUT PULONG  ReturnLength)
{
    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (!m_bInitialized)
    {
        DPRINT("IPortTopology_fnNewRegistryKey called w/o initialized\n");
        return STATUS_UNSUCCESSFUL;
    }

    return IoGetDeviceProperty(m_pDeviceObject, DeviceRegistryProperty, BufferLength, PropertyBuffer, ReturnLength);
}

NTSTATUS
NTAPI
CPortTopology::Init(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp,
    IN PUNKNOWN  UnknownMiniport,
    IN PUNKNOWN  UnknownAdapter  OPTIONAL,
    IN PRESOURCELIST  ResourceList)
{
    PPOWERNOTIFY PowerNotify;
    IMiniportTopology * Miniport;
    NTSTATUS Status;
    PPCLASS_DEVICE_EXTENSION DeviceExtension;

    DPRINT("IPortTopology_fnInit entered This %p DeviceObject %p Irp %p UnknownMiniport %p UnknownAdapter %p ResourceList %p\n",
            this, DeviceObject, Irp, UnknownMiniport, UnknownAdapter, ResourceList);

    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (m_bInitialized)
    {
        DPRINT("IPortTopology_Init called again\n");
        return STATUS_SUCCESS;
    }

    Status = UnknownMiniport->QueryInterface(IID_IMiniportTopology, (PVOID*)&Miniport);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IPortTopology_Init called with invalid IMiniport adapter\n");
        return STATUS_INVALID_PARAMETER;
    }

    // Initialize port object
    m_pMiniport = Miniport;
    m_pDeviceObject = DeviceObject;
    m_bInitialized = TRUE;

    // now initialize the miniport driver
    Status = Miniport->Init(UnknownAdapter, ResourceList, this);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IPortTopology_Init failed with %x\n", Status);
        m_bInitialized = FALSE;
        Miniport->Release();
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
                                         2,
                                         InterfaceGuids,
                                         0,
                                         NULL,
                                         2,
                                         TopologyPropertySet,
                                         0,
                                         0,
                                         0,
                                         NULL,
                                         0,
                                         NULL,
                                         m_pDescriptor);

    DPRINT("IPortTopology_fnInit success\n");
    if (NT_SUCCESS(Status))
    {
        // store for node property requests
        m_SubDeviceDescriptor->UnknownMiniport = UnknownMiniport;
    }

    // does the Miniport adapter support IPowerNotify interface*/
    Status = UnknownMiniport->QueryInterface(IID_IPowerNotify, (PVOID *)&PowerNotify);
    if (NT_SUCCESS(Status))
    {
        // get device extension
        DeviceExtension = (PPCLASS_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
        PENTRY_POWER_NOTIFY Notify =
            (PENTRY_POWER_NOTIFY)AllocateItem(NonPagedPool, sizeof(ENTRY_POWER_NOTIFY), TAG_PORTCLASS);
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
    }
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CPortTopology::NewRegistryKey(
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
        DPRINT("IPortTopology_fnNewRegistryKey called w/o initialized\n");
        return STATUS_UNSUCCESSFUL;
    }
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

//---------------------------------------------------------------
// ISubdevice interface
//

NTSTATUS
NTAPI
CPortTopology::NewIrpTarget(
    OUT struct IIrpTarget **OutTarget,
    IN PCWSTR Name,
    IN PUNKNOWN Unknown,
    IN POOL_TYPE PoolType,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN KSOBJECT_CREATE *CreateObject)
{
    NTSTATUS Status;
    IPortFilterTopology * Filter;

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
    Status = NewPortFilterTopology(&Filter);
    if (!NT_SUCCESS(Status))
    {
        // not enough memory
        return Status;
    }

    // initialize the filter
    Status = Filter->Init((IPortTopology*)this);
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
CPortTopology::ReleaseChildren()
{
    DPRINT("ISubDevice_fnReleaseChildren\n");

    // release the filter
    m_Filter->Release();

    // release the miniport
    DPRINT("Refs %u\n", m_pMiniport->Release());

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CPortTopology::GetDescriptor(
    IN SUBDEVICE_DESCRIPTOR ** Descriptor)
{
    DPRINT("ISubDevice_GetDescriptor this %p Descp %p\n", this, m_SubDeviceDescriptor);
    *Descriptor = m_SubDeviceDescriptor;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CPortTopology::DataRangeIntersection(
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
CPortTopology::PowerChangeNotify(
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
CPortTopology::PinCount(
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
NTAPI
PcCreatePinDispatch(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    NTSTATUS Status;
    IIrpTarget *Filter;
    IIrpTarget *Pin;
    PKSOBJECT_CREATE_ITEM CreateItem;

    // access the create item
    CreateItem = KSCREATE_ITEM_IRP_STORAGE(Irp);
    // sanity check
    PC_ASSERT(CreateItem);

    DPRINT("PcCreatePinDispatch called DeviceObject %p %S Name\n", DeviceObject, CreateItem->ObjectClass.Buffer);

    Filter = (IIrpTarget*)CreateItem->Context;

    // sanity checks
    PC_ASSERT(Filter != NULL);
    PC_ASSERT_IRQL(PASSIVE_LEVEL);

#if KS_IMPLEMENTED
    Status = KsReferenceSoftwareBusObject(DeviceExt->KsDeviceHeader);
    if (!NT_SUCCESS(Status) && Status != STATUS_NOT_IMPLEMENTED)
    {
        DPRINT("PcCreatePinDispatch failed to reference device header\n");

        FreeItem(Entry, TAG_PORTCLASS);
        goto cleanup;
    }
#endif

    Status = Filter->NewIrpTarget(&Pin,
                                  KSSTRING_Pin,
                                  NULL,
                                  NonPagedPool,
                                  DeviceObject,
                                  Irp,
                                  NULL);

    DPRINT("PcCreatePinDispatch Status %x\n", Status);

    if (NT_SUCCESS(Status))
    {
        // create the dispatch object
        // FIXME need create item for clock
        Status = NewDispatchObject(Irp, Pin, 0, NULL);
        DPRINT("Pin %p\n", Pin);
    }

    DPRINT("CreatePinWorkerRoutine completing irp %p\n", Irp);
    // save status in irp
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    // complete the request
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

NTSTATUS
NTAPI
PcCreateItemDispatch(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    NTSTATUS Status;
    ISubdevice * SubDevice;
    IIrpTarget *Filter;
    PKSOBJECT_CREATE_ITEM CreateItem, PinCreateItem;

    // access the create item
    CreateItem = KSCREATE_ITEM_IRP_STORAGE(Irp);

    DPRINT("PcCreateItemDispatch called DeviceObject %p %S Name\n", DeviceObject, CreateItem->ObjectClass.Buffer);

    // get the subdevice
    SubDevice = (ISubdevice*)CreateItem->Context;

    // sanity checks
    PC_ASSERT(SubDevice != NULL);

#if KS_IMPLEMENTED
    Status = KsReferenceSoftwareBusObject(DeviceExt->KsDeviceHeader);
    if (!NT_SUCCESS(Status) && Status != STATUS_NOT_IMPLEMENTED)
    {
        DPRINT("PcCreateItemDispatch failed to reference device header\n");

        FreeItem(Entry, TAG_PORTCLASS);
        goto cleanup;
    }
#endif

    // get filter object
    Status = SubDevice->NewIrpTarget(&Filter,
                                     NULL,
                                     NULL,
                                     NonPagedPool,
                                     DeviceObject,
                                     Irp,
                                     NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Failed to get filter object\n");
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }

    // allocate pin create item
    PinCreateItem = (PKSOBJECT_CREATE_ITEM)AllocateItem(NonPagedPool, sizeof(KSOBJECT_CREATE_ITEM), TAG_PORTCLASS);
    if (!PinCreateItem)
    {
        // not enough memory
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // initialize pin create item
    PinCreateItem->Context = (PVOID)Filter;
    PinCreateItem->Create = PcCreatePinDispatch;
    RtlInitUnicodeString(&PinCreateItem->ObjectClass, KSSTRING_Pin);
    // FIXME copy security descriptor

    // now allocate a dispatch object
    Status = NewDispatchObject(Irp, Filter, 1, PinCreateItem);

    // complete request
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS
NewPortTopology(
    OUT PPORT* OutPort)
{
    CPortTopology * This;
    NTSTATUS Status;

    This= new(NonPagedPool, TAG_PORTCLASS) CPortTopology(NULL);
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    Status = This->QueryInterface(IID_IPort, (PVOID*)OutPort);

    if (!NT_SUCCESS(Status))
    {
        delete This;
    }

    DPRINT("NewPortTopology %p Status %x\n", *OutPort, Status);
    return Status;
}

PMINIPORTTOPOLOGY
GetTopologyMiniport(
    PPORTTOPOLOGY Port)
{
    CPortTopology * This = (CPortTopology*)Port;
    return This->m_pMiniport;
}
