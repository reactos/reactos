/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/port_wavepci.c
 * PURPOSE:         Wave PCI Port driver
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.h"

typedef struct
{
    IPortWavePciVtbl *lpVtbl;
    IServiceSinkVtbl *lpVtblServiceSink;
    IPortEventsVtbl *lpVtblPortEvents;
    ISubdeviceVtbl *lpVtblSubDevice;

#if 0
    IUnregisterSubdevice *lpVtblUnregisterSubDevice;
#endif
    LONG ref;

    PMINIPORTWAVEPCI Miniport;
    PDEVICE_OBJECT pDeviceObject;
    KDPC Dpc;
    BOOL bInitialized;
    PRESOURCELIST pResourceList;
    PSERVICEGROUP ServiceGroup;
    PPINCOUNT pPinCount;
    PPOWERNOTIFY pPowerNotify;
    PPCFILTER_DESCRIPTOR pDescriptor;
    PSUBDEVICE_DESCRIPTOR SubDeviceDescriptor;
    IPortFilterWavePci * Filter;
}IPortWavePciImpl;

static GUID InterfaceGuids[3] = 
{
    {
        /// KSCATEGORY_RENDER
        0x65E8773EL, 0x8F56, 0x11D0, {0xA3, 0xB9, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}
    },
    {
        /// KSCATEGORY_CAPTURE
        0x65E8773DL, 0x8F56, 0x11D0, {0xA3, 0xB9, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}
    },
    {
        /// KS_CATEGORY_AUDIO
        0x6994AD04, 0x93EF, 0x11D0, {0xA3, 0xCC, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}
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

static
NTSTATUS
NTAPI
IPortEvents_fnQueryInterface(
    IPortEvents* iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    IPortWavePciImpl * This = (IPortWavePciImpl*)CONTAINING_RECORD(iface, IPortWavePciImpl, lpVtblPortEvents);

    DPRINT("IPortEvents_fnQueryInterface entered\n");

    if (IsEqualGUIDAligned(refiid, &IID_IPortEvents) ||
        IsEqualGUIDAligned(refiid, &IID_IUnknown))
    {
        *Output = &This->lpVtblServiceSink;
        InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }
    return STATUS_UNSUCCESSFUL;
}

static
ULONG
NTAPI
IPortEvents_fnAddRef(
    IPortEvents* iface)
{
    IPortWavePciImpl * This = (IPortWavePciImpl*)CONTAINING_RECORD(iface, IPortWavePciImpl, lpVtblPortEvents);
    DPRINT("IPortEvents_fnAddRef entered\n");
    return InterlockedIncrement(&This->ref);
}

static
ULONG
NTAPI
IPortEvents_fnRelease(
    IPortEvents* iface)
{
    IPortWavePciImpl * This = (IPortWavePciImpl*)CONTAINING_RECORD(iface, IPortWavePciImpl, lpVtblPortEvents);
    DPRINT("IPortEvents_fnRelease entered\n");
    InterlockedDecrement(&This->ref);

    if (This->ref == 0)
    {
        FreeItem(This, TAG_PORTCLASS);
        return 0;
    }
    /* Return new reference count */
    return This->ref;
}

static
void
NTAPI
IPortEvents_fnAddEventToEventList(
    IPortEvents* iface,
    IN PKSEVENT_ENTRY EventEntry)
{
    UNIMPLEMENTED
    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);
}


static
void
NTAPI
IPortEvents_fnGenerateEventList(
    IPortEvents* iface,
    IN  GUID* Set OPTIONAL,
    IN  ULONG EventId,
    IN  BOOL PinEvent,
    IN  ULONG PinId,
    IN  BOOL NodeEvent,
    IN  ULONG NodeId)
{
    UNIMPLEMENTED
}

static IPortEventsVtbl vt_IPortEvents = 
{
    IPortEvents_fnQueryInterface,
    IPortEvents_fnAddRef,
    IPortEvents_fnRelease,
    IPortEvents_fnAddEventToEventList,
    IPortEvents_fnGenerateEventList
};

//---------------------------------------------------------------
// IServiceSink
//

static
NTSTATUS
NTAPI
IServiceSink_fnQueryInterface(
    IServiceSink* iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    IPortWavePciImpl * This = (IPortWavePciImpl*)CONTAINING_RECORD(iface, IPortWavePciImpl, lpVtblServiceSink);

    DPRINT("IServiceSink_fnQueryInterface entered\n");

    if (IsEqualGUIDAligned(refiid, &IID_IServiceSink) ||
        IsEqualGUIDAligned(refiid, &IID_IUnknown))
    {
        *Output = &This->lpVtblServiceSink;
        InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }
    return STATUS_UNSUCCESSFUL;
}

static
ULONG
NTAPI
IServiceSink_fnAddRef(
    IServiceSink* iface)
{
    IPortWavePciImpl * This = (IPortWavePciImpl*)CONTAINING_RECORD(iface, IPortWavePciImpl, lpVtblServiceSink);
    DPRINT("IServiceSink_fnAddRef entered\n");
    return InterlockedIncrement(&This->ref);
}

static
ULONG
NTAPI
IServiceSink_fnRelease(
    IServiceSink* iface)
{
    IPortWavePciImpl * This = (IPortWavePciImpl*)CONTAINING_RECORD(iface, IPortWavePciImpl, lpVtblServiceSink);
    DPRINT("IServiceSink_fnRelease entered\n");
    InterlockedDecrement(&This->ref);

    if (This->ref == 0)
    {
        FreeItem(This, TAG_PORTCLASS);
        return 0;
    }
    /* Return new reference count */
    return This->ref;
}

static
VOID
NTAPI
IServiceSink_fnRequestService(
    IServiceSink* iface)
{
    IPortWavePciImpl * This = (IPortWavePciImpl*)CONTAINING_RECORD(iface, IPortWavePciImpl, lpVtblServiceSink);
    DPRINT("IServiceSink_fnRequestService entered\n");
    if (This->Miniport)
    {
        This->Miniport->lpVtbl->Service(This->Miniport);
    }
}

static IServiceSinkVtbl vt_IServiceSink = 
{
    IServiceSink_fnQueryInterface,
    IServiceSink_fnAddRef,
    IServiceSink_fnRelease,
    IServiceSink_fnRequestService
};

//---------------------------------------------------------------
// IPortWavePci
//

static
NTSTATUS
NTAPI
IPortWavePci_fnQueryInterface(
    IPortWavePci* iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    UNICODE_STRING GuidString;
    IPortWavePciImpl * This = (IPortWavePciImpl*)iface;

    DPRINT("IPortWavePci_fnQueryInterface entered\n");

    if (IsEqualGUIDAligned(refiid, &IID_IPortWavePci) || 
        IsEqualGUIDAligned(refiid, &IID_IUnknown))
    {
        *Output = &This->lpVtbl;
        InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }
    else if (IsEqualGUIDAligned(refiid, &IID_IServiceSink))
    {
        *Output = &This->lpVtblServiceSink;
        InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }
    else if (IsEqualGUIDAligned(refiid, &IID_IPortEvents))
    {
        *Output = &This->lpVtblPortEvents;
        InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }
    else if (IsEqualGUIDAligned(refiid, &IID_ISubdevice))
    {
        *Output = &This->lpVtblSubDevice;
        InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }
    else if (IsEqualGUIDAligned(refiid, &IID_IPortClsVersion))
    {
        return NewPortClsVersion((PPORTCLSVERSION*)Output);
    }

    if (RtlStringFromGUID(refiid, &GuidString) == STATUS_SUCCESS)
    {
        DPRINT1("IPortWavePci_fnQueryInterface no interface!!! iface %S\n", GuidString.Buffer);
        RtlFreeUnicodeString(&GuidString);
    }

    return STATUS_UNSUCCESSFUL;
}

static
ULONG
NTAPI
IPortWavePci_fnAddRef(
    IPortWavePci* iface)
{
    IPortWavePciImpl * This = (IPortWavePciImpl*)iface;

    DPRINT("IPortWavePci_fnAddRef entered\n");

    return InterlockedIncrement(&This->ref);
}

static
ULONG
NTAPI
IPortWavePci_fnRelease(
    IPortWavePci* iface)
{
    IPortWavePciImpl * This = (IPortWavePciImpl*)iface;

    DPRINT("IPortWavePci_fnRelease entered\n");

    InterlockedDecrement(&This->ref);

    if (This->ref == 0)
    {
        FreeItem(This, TAG_PORTCLASS);
        return 0;
    }
    /* Return new reference count */
    return This->ref;
}

VOID
NTAPI
ServiceNotifyRoutine(
    IN struct _KDPC  *Dpc,
    IN PVOID  DeferredContext,
    IN PVOID  SystemArgument1,
    IN PVOID  SystemArgument2)
{
    DPRINT("ServiceNotifyRoutine entered %p %p %p\n", DeferredContext, SystemArgument1, SystemArgument2);

    IPortWavePciImpl * This = (IPortWavePciImpl*)DeferredContext;
    if (This->ServiceGroup && This->bInitialized)
    {
        DPRINT("ServiceGroup %p\n", This->ServiceGroup);
        This->ServiceGroup->lpVtbl->RequestService(This->ServiceGroup);
    }
}



NTSTATUS
NTAPI
IPortWavePci_fnInit(
    IN IPortWavePci * iface,
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp,
    IN PUNKNOWN  UnknownMiniport,
    IN PUNKNOWN  UnknownAdapter  OPTIONAL,
    IN PRESOURCELIST  ResourceList)
{
    IMiniportWavePci * Miniport;
    PSERVICEGROUP ServiceGroup;
    NTSTATUS Status;
    PPINCOUNT PinCount;
    PPOWERNOTIFY PowerNotify;
    IPortWavePciImpl * This = (IPortWavePciImpl*)iface;

    DPRINT("IPortWavePci_fnInit entered with This %p, DeviceObject %p Irp %p UnknownMiniport %p, UnknownAdapter %p ResourceList %p\n", 
            This, DeviceObject, Irp, UnknownMiniport, UnknownAdapter, ResourceList);
    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (This->bInitialized)
    {
        DPRINT("IPortWavePci_fnInit called again\n");
        return STATUS_SUCCESS;
    }

    Status = UnknownMiniport->lpVtbl->QueryInterface(UnknownMiniport, &IID_IMiniportWavePci, (PVOID*)&Miniport);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IPortWavePci_fnInit called with invalid IMiniport adapter\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* initialize the dpc */
    KeInitializeDpc(&This->Dpc, ServiceNotifyRoutine, (PVOID)This);

    /* Initialize port object */
    This->Miniport = Miniport;
    This->pDeviceObject = DeviceObject;
    This->bInitialized = TRUE;
    This->pResourceList = ResourceList;


    /* increment reference on miniport adapter */
    Miniport->lpVtbl->AddRef(Miniport);
    /* increment reference on resource list */
    ResourceList->lpVtbl->AddRef(ResourceList);

    Status = Miniport->lpVtbl->Init(Miniport, UnknownAdapter, ResourceList, iface, &ServiceGroup);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IPortWavePci_fnInit failed with %x\n", Status);
        This->bInitialized = FALSE;
        /* release reference on miniport adapter */
        Miniport->lpVtbl->Release(Miniport);
        /* increment reference on resource list */
        ResourceList->lpVtbl->Release(ResourceList);
        return Status;
    }

    /* check if the miniport adapter provides a valid device descriptor */
    Status = Miniport->lpVtbl->GetDescription(Miniport, &This->pDescriptor);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("failed to get description\n");
        Miniport->lpVtbl->Release(Miniport);
        This->bInitialized = FALSE;
        return Status;
    }

   /* create the subdevice descriptor */
    Status = PcCreateSubdeviceDescriptor(&This->SubDeviceDescriptor, 
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
                                         This->pDescriptor);


    if (!NT_SUCCESS(Status))
    {
        DPRINT1("PcCreateSubdeviceDescriptor failed with %x\n", Status);
        Miniport->lpVtbl->Release(Miniport);
        This->bInitialized = FALSE;
        return Status;
    }

    /* did we get a service group */
    if (ServiceGroup)
    {
        /* store service group in context */
        This->ServiceGroup = ServiceGroup;

        /* add ourselves to service group which is called when miniport receives an isr */
        ServiceGroup->lpVtbl->AddMember(ServiceGroup, (PSERVICESINK)&This->lpVtblServiceSink);

        /* increment reference on service group */
        ServiceGroup->lpVtbl->AddRef(ServiceGroup);
    }

    /* check if it supports IPinCount interface */
    Status = UnknownMiniport->lpVtbl->QueryInterface(UnknownMiniport, &IID_IPinCount, (PVOID*)&PinCount);
    if (NT_SUCCESS(Status))
    {
        /* store IPinCount interface */
        This->pPinCount = PinCount;
    }

    /* does the Miniport adapter support IPowerNotify interface*/
    Status = UnknownMiniport->lpVtbl->QueryInterface(UnknownMiniport, &IID_IPowerNotify, (PVOID*)&PowerNotify);
    if (NT_SUCCESS(Status))
    {
        /* store reference */
        This->pPowerNotify = PowerNotify;
    }

    DPRINT("IPortWavePci_Init sucessfully initialized\n");
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IPortWavePci_fnNewRegistryKey(
    IN IPortWavePci * iface,
    OUT PREGISTRYKEY  *OutRegistryKey,
    IN PUNKNOWN  OuterUnknown  OPTIONAL,
    IN ULONG  RegistryKeyType,
    IN ACCESS_MASK  DesiredAccess,
    IN POBJECT_ATTRIBUTES  ObjectAttributes  OPTIONAL,
    IN ULONG  CreateOptions  OPTIONAL,
    OUT PULONG  Disposition  OPTIONAL)
{
    IPortWavePciImpl * This = (IPortWavePciImpl*)iface;

    DPRINT("IPortWavePci_fnNewRegistryKey entered\n");
    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (!This->bInitialized)
    {
        DPRINT("IPortWavePci_fnNewRegistryKey called w/o initiazed\n");
        return STATUS_UNSUCCESSFUL;
    }

    return PcNewRegistryKey(OutRegistryKey, 
                            OuterUnknown,
                            RegistryKeyType,
                            DesiredAccess,
                            This->pDeviceObject,
                            NULL,//FIXME
                            ObjectAttributes,
                            CreateOptions,
                            Disposition);
}

NTSTATUS
NTAPI
IPortWavePci_fnGetDeviceProperty(
    IN IPortWavePci * iface,
    IN DEVICE_REGISTRY_PROPERTY  DeviceRegistryProperty,
    IN ULONG  BufferLength,
    OUT PVOID  PropertyBuffer,
    OUT PULONG  ReturnLength)
{
    IPortWavePciImpl * This = (IPortWavePciImpl*)iface;

    DPRINT("IPortWavePci_fnGetDeviceProperty entered\n");
    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (!This->bInitialized)
    {
        DPRINT("IPortWavePci_fnNewRegistryKey called w/o initiazed\n");
        return STATUS_UNSUCCESSFUL;
    }

    return IoGetDeviceProperty(This->pDeviceObject, DeviceRegistryProperty, BufferLength, PropertyBuffer, ReturnLength);
}

NTSTATUS
NTAPI
IPortWavePci_fnNewMasterDmaChannel(
    IN IPortWavePci * iface,
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
    IPortWavePciImpl * This = (IPortWavePciImpl*)iface;

    DPRINT("IPortWavePci_fnNewMasterDmaChannel This %p entered\n", This);
    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    Status = PcDmaMasterDescription(ResourceList, ScatterGather, Dma32BitAddresses, IgnoreCount, Dma64BitAddresses, DmaWidth, DmaSpeed, MaximumLength, DmaPort, &DeviceDescription);
    if (NT_SUCCESS(Status))
    {
        return PcNewDmaChannel(DmaChannel, OuterUnknown, PoolType, &DeviceDescription, This->pDeviceObject);
    }

    return Status;
}

VOID
NTAPI
IPortWavePci_fnNotify(
    IN IPortWavePci * iface,
    IN  PSERVICEGROUP ServiceGroup)
{
    IPortWavePciImpl * This = (IPortWavePciImpl*)iface;


    DPRINT1("IPortWavePci_fnNotify entered %p, ServiceGroup %p\n", This, ServiceGroup);

    //if (This->ServiceGroup)
    //{
    //    ServiceGroup->lpVtbl->RequestService (ServiceGroup);
    //}

   // KeInsertQueueDpc(&This->Dpc, NULL, NULL);
}

static IPortWavePciVtbl vt_IPortWavePci =
{
    /* IUnknown methods */
    IPortWavePci_fnQueryInterface,
    IPortWavePci_fnAddRef,
    IPortWavePci_fnRelease,
    /* IPort methods */
    IPortWavePci_fnInit,
    IPortWavePci_fnGetDeviceProperty,
    IPortWavePci_fnNewRegistryKey,
    /* IPortWavePci methods */
    IPortWavePci_fnNotify,
    IPortWavePci_fnNewMasterDmaChannel,
};

//---------------------------------------------------------------
// ISubdevice interface
//

static
NTSTATUS
NTAPI
ISubDevice_fnQueryInterface(
    IN ISubdevice *iface,
    IN REFIID InterfaceId,
    IN PVOID* Interface)
{
    IPortWavePciImpl * This = (IPortWavePciImpl*)CONTAINING_RECORD(iface, IPortWavePciImpl, lpVtblSubDevice);

    return IPortWavePci_fnQueryInterface((IPortWavePci*)This, InterfaceId, Interface);
}

static
ULONG
NTAPI
ISubDevice_fnAddRef(
    IN ISubdevice *iface)
{
    IPortWavePciImpl * This = (IPortWavePciImpl*)CONTAINING_RECORD(iface, IPortWavePciImpl, lpVtblSubDevice);

    return IPortWavePci_fnAddRef((IPortWavePci*)This);
}

static
ULONG
NTAPI
ISubDevice_fnRelease(
    IN ISubdevice *iface)
{
    IPortWavePciImpl * This = (IPortWavePciImpl*)CONTAINING_RECORD(iface, IPortWavePciImpl, lpVtblSubDevice);

    return IPortWavePci_fnRelease((IPortWavePci*)This);
}

static
NTSTATUS
NTAPI
ISubDevice_fnNewIrpTarget(
    IN ISubdevice *iface,
    OUT struct IIrpTarget **OutTarget,
    IN WCHAR * Name,
    IN PUNKNOWN Unknown,
    IN POOL_TYPE PoolType,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp, 
    IN KSOBJECT_CREATE *CreateObject)
{
    NTSTATUS Status;
    IPortFilterWavePci * Filter;
    IPortWavePciImpl * This = (IPortWavePciImpl*)CONTAINING_RECORD(iface, IPortWavePciImpl, lpVtblSubDevice);

    DPRINT("ISubDevice_NewIrpTarget this %p\n", This);

    if (This->Filter)
    {
        *OutTarget = (IIrpTarget*)This->Filter;
        return STATUS_SUCCESS;
    }

    Status = NewPortFilterWavePci(&Filter);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = Filter->lpVtbl->Init(Filter, (IPortWavePci*)This);
    if (!NT_SUCCESS(Status))
    {
        Filter->lpVtbl->Release(Filter);
        return Status;
    }

    *OutTarget = (IIrpTarget*)Filter;
    return Status;
}

static
NTSTATUS
NTAPI
ISubDevice_fnReleaseChildren(
    IN ISubdevice *iface)
{
    //IPortWavePciImpl * This = (IPortWavePciImpl*)CONTAINING_RECORD(iface, IPortWavePciImpl, lpVtblSubDevice);

    UNIMPLEMENTED
    return STATUS_UNSUCCESSFUL;
}

static
NTSTATUS
NTAPI
ISubDevice_fnGetDescriptor(
    IN ISubdevice *iface,
    IN SUBDEVICE_DESCRIPTOR ** Descriptor)
{
    IPortWavePciImpl * This = (IPortWavePciImpl*)CONTAINING_RECORD(iface, IPortWavePciImpl, lpVtblSubDevice);

    DPRINT1("ISubDevice_GetDescriptor this %p\n", This);
    *Descriptor = This->SubDeviceDescriptor;
    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
ISubDevice_fnDataRangeIntersection(
    IN ISubdevice *iface,
    IN  ULONG PinId,
    IN  PKSDATARANGE DataRange,
    IN  PKSDATARANGE MatchingDataRange,
    IN  ULONG OutputBufferLength,
    OUT PVOID ResultantFormat OPTIONAL,
    OUT PULONG ResultantFormatLength)
{
    IPortWavePciImpl * This = (IPortWavePciImpl*)CONTAINING_RECORD(iface, IPortWavePciImpl, lpVtblSubDevice);

    DPRINT("ISubDevice_DataRangeIntersection this %p\n", This);

    if (This->Miniport)
    {
        return This->Miniport->lpVtbl->DataRangeIntersection (This->Miniport, PinId, DataRange, MatchingDataRange, OutputBufferLength, ResultantFormat, ResultantFormatLength);
    }

    return STATUS_UNSUCCESSFUL;
}

static
NTSTATUS
NTAPI
ISubDevice_fnPowerChangeNotify(
    IN ISubdevice *iface,
    IN POWER_STATE PowerState)
{
    IPortWavePciImpl * This = (IPortWavePciImpl*)CONTAINING_RECORD(iface, IPortWavePciImpl, lpVtblSubDevice);

    if (This->pPowerNotify)
    {
        This->pPowerNotify->lpVtbl->PowerChangeNotify(This->pPowerNotify, PowerState);
    }

    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
ISubDevice_fnPinCount(
    IN ISubdevice *iface,
    IN ULONG  PinId,
    IN OUT PULONG  FilterNecessary,
    IN OUT PULONG  FilterCurrent,
    IN OUT PULONG  FilterPossible,
    IN OUT PULONG  GlobalCurrent,
    IN OUT PULONG  GlobalPossible)
{
    IPortWavePciImpl * This = (IPortWavePciImpl*)CONTAINING_RECORD(iface, IPortWavePciImpl, lpVtblSubDevice);

    if (This->pPinCount)
    {
       This->pPinCount->lpVtbl->PinCount(This->pPinCount, PinId, FilterNecessary, FilterCurrent, FilterPossible, GlobalCurrent, GlobalPossible);
       return STATUS_SUCCESS;
    }

    /* FIXME
     * scan filter descriptor 
     */
    return STATUS_UNSUCCESSFUL;
}

static ISubdeviceVtbl vt_ISubdeviceVtbl = 
{
    ISubDevice_fnQueryInterface,
    ISubDevice_fnAddRef,
    ISubDevice_fnRelease,
    ISubDevice_fnNewIrpTarget,
    ISubDevice_fnReleaseChildren,
    ISubDevice_fnGetDescriptor,
    ISubDevice_fnDataRangeIntersection,
    ISubDevice_fnPowerChangeNotify,
    ISubDevice_fnPinCount
};

NTSTATUS
NewPortWavePci(
    OUT PPORT* OutPort)
{
    IPortWavePciImpl * This;

    This = AllocateItem(NonPagedPool, sizeof(IPortWavePciImpl), TAG_PORTCLASS);
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    This->lpVtblServiceSink = &vt_IServiceSink;
    This->lpVtbl = &vt_IPortWavePci;
    This->lpVtblSubDevice = &vt_ISubdeviceVtbl;
    This->lpVtblPortEvents = &vt_IPortEvents;
    This->ref = 1;

    *OutPort = (PPORT)&This->lpVtbl;
    DPRINT("NewPortWavePci %p\n", *OutPort);
    return STATUS_SUCCESS;
}

PDEVICE_OBJECT
GetDeviceObjectFromPortWavePci(
    IPortWavePci* iface)
{
    IPortWavePciImpl * This = (IPortWavePciImpl*)iface;
    return This->pDeviceObject;
}

PMINIPORTWAVEPCI
GetWavePciMiniport(
    PPORTWAVEPCI iface)
{
    IPortWavePciImpl * This = (IPortWavePciImpl*)iface;
    return This->Miniport;
}
