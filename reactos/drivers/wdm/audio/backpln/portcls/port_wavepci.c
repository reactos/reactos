#include "private.h"

typedef struct
{
    IPortWavePciVtbl *lpVtbl;
    IServiceSinkVtbl *lpVtblServiceSink;
    IPortEventsVtbl *lpVtblPortEvents;

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
}IPortWavePciImpl;


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

    DPRINT1("IServiceSink_fnQueryInterface entered\n");

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
    DPRINT1("IServiceSink_fnAddRef entered\n");
    return InterlockedIncrement(&This->ref);
}

static
ULONG
NTAPI
IPortEvents_fnRelease(
    IPortEvents* iface)
{
    IPortWavePciImpl * This = (IPortWavePciImpl*)CONTAINING_RECORD(iface, IPortWavePciImpl, lpVtblPortEvents);
    DPRINT1("IServiceSink_fnRelease entered\n");
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
    DPRINT1("IPortEvents_fnAddEventToEventList stub\n");
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
    DPRINT1("IPortEvents_fnGenerateEventList stub\n");
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

    DPRINT1("IServiceSink_fnQueryInterface entered\n");

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
    DPRINT1("IServiceSink_fnAddRef entered\n");
    return InterlockedIncrement(&This->ref);
}

static
ULONG
NTAPI
IServiceSink_fnRelease(
    IServiceSink* iface)
{
    IPortWavePciImpl * This = (IPortWavePciImpl*)CONTAINING_RECORD(iface, IPortWavePciImpl, lpVtblServiceSink);
    DPRINT1("IServiceSink_fnRelease entered\n");
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
    DPRINT1("IServiceSink_fnRequestService entered\n");
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
	WCHAR Buffer[100];
    IPortWavePciImpl * This = (IPortWavePciImpl*)iface;

    DPRINT1("IPortWavePci_fnQueryInterface entered\n");

    if (IsEqualGUIDAligned(refiid, &IID_IPortWavePci) || 
        IsEqualGUIDAligned(refiid, &IID_IUnknown))
    {
        *Output = &This->lpVtbl;
        InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }
    else if (IsEqualGUIDAligned(refiid, &IID_IServiceSink))
    {
        *Output = &This->lpVtbl;
        InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }
    else if (IsEqualGUIDAligned(refiid, &IID_IPortEvents))
    {
        *Output = &This->lpVtblPortEvents;
        InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }
    else if (IsEqualGUIDAligned(refiid, &IID_IPortClsVersion))
    {
        return NewPortClsVersion((PPORTCLSVERSION*)Output);
    }

    StringFromCLSID(refiid, Buffer);
    DPRINT1("IPortWavePci_fnQueryInterface no interface!!! iface %S\n", Buffer);
    KeBugCheckEx(0, 0, 0, 0, 0);
    return STATUS_UNSUCCESSFUL;
}

static
ULONG
NTAPI
IPortWavePci_fnAddRef(
    IPortWavePci* iface)
{
    IPortWavePciImpl * This = (IPortWavePciImpl*)iface;

    DPRINT1("IPortWavePci_fnAddRef entered\n");

    return InterlockedIncrement(&This->ref);
}

static
ULONG
NTAPI
IPortWavePci_fnRelease(
    IPortWavePci* iface)
{
    IPortWavePciImpl * This = (IPortWavePciImpl*)iface;

    DPRINT1("IPortWavePci_fnRelease entered\n");

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
    DPRINT1("ServiceNotifyRoutine entered %p %p %p\n", DeferredContext, SystemArgument1, SystemArgument2);

    IPortWavePciImpl * This = (IPortWavePciImpl*)DeferredContext;
    if (This->ServiceGroup && This->bInitialized)
    {
        DPRINT1("ServiceGroup %p\n", This->ServiceGroup);
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
    IPortWavePciImpl * This = (IPortWavePciImpl*)iface;

    DPRINT1("IPortWavePci::Init entered with This %p, DeviceObject %p Irp %p UnknownMiniport %p, UnknownAdapter %p ResourceList %p\n", 
            This, DeviceObject, Irp, UnknownMiniport, UnknownAdapter, ResourceList);

    if (This->bInitialized)
    {
        DPRINT("IPortWaveCyclic_Init called again\n");
        return STATUS_SUCCESS;
    }

    Status = UnknownMiniport->lpVtbl->QueryInterface(UnknownMiniport, &IID_IMiniportWavePci, (PVOID*)&Miniport);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IPortWaveCyclic_Init called with invalid IMiniport adapter\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* initialize the dpc */
    KeInitializeDpc(&This->Dpc, ServiceNotifyRoutine, (PVOID)This);


    Status = Miniport->lpVtbl->Init(Miniport, UnknownAdapter, ResourceList, iface, &ServiceGroup);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IMiniportWaveCyclic_Init failed with %x\n", Status);
        return Status;
    }
    DPRINT1("IPortWaveCyclic_Init Miniport adapter initialized\n");
    /* Initialize port object */
    This->Miniport = Miniport;
    This->pDeviceObject = DeviceObject;
    This->bInitialized = TRUE;
    This->pResourceList = ResourceList;
    This->ServiceGroup = ServiceGroup;


    /* increment reference on miniport adapter */
    Miniport->lpVtbl->AddRef(Miniport);
    /* increment reference on resource list */
    ResourceList->lpVtbl->AddRef(ResourceList);

    /* add ourselves to service group which is called when miniport receives an isr */
    ServiceGroup->lpVtbl->AddMember(ServiceGroup, (PSERVICESINK)&This->lpVtblServiceSink);

    /* increment reference on service group */
    ServiceGroup->lpVtbl->AddRef(ServiceGroup);

    DPRINT("IPortWaveCyclic_Init sucessfully initialized\n");
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

    DPRINT1("IPortWavePci_fnNewRegistryKey entered\n");

    if (!This->bInitialized)
    {
        DPRINT("IPortWaveCyclic_fnNewRegistryKey called w/o initiazed\n");
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

    DPRINT1("IPortWavePci_fnGetDeviceProperty entered\n");

    if (!This->bInitialized)
    {
        DPRINT("IPortWaveCyclic_fnNewRegistryKey called w/o initiazed\n");
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

    DPRINT1("IPortWavePci_fnNewMasterDmaChannel entered\n");

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


    DPRINT1("IPortWavePci_fnNotify entered %p, ServiceGroup %p\n", This, This->ServiceGroup);

    if (This->ServiceGroup)
    {
        This->ServiceGroup->lpVtbl->RequestService (This->ServiceGroup);
    }

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
    IPortWavePci_fnNewMasterDmaChannel,
    IPortWavePci_fnNotify
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
    This->lpVtblPortEvents = &vt_IPortEvents;
    This->ref = 1;

    *OutPort = (PPORT)&This->lpVtbl;
    DPRINT1("NewPortWavePci %p\n", *OutPort);
    return STATUS_SUCCESS;
}
