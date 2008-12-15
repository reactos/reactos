#include "private.h"

typedef struct
{
    IPortWavePciVtbl *lpVtbl;
    IServiceSinkVtbl *lpVtblServiceSink;

#if 0
    IUnregisterSubdevice *lpVtblUnregisterSubDevice;
    IPortClsVersionVtbl  *lpVtblPortClsVersion;
#endif
    LONG ref;

    PMINIPORTWAVEPCI Miniport;
    PDEVICE_OBJECT pDeviceObject;
    KDPC Dpc;
    BOOL bInitialized;
    PRESOURCELIST pResourceList;
    PSERVICEGROUP ServiceGroup;
}IPortWavePciImpl;


const GUID IID_IPortWavePci;
const GUID IID_IMiniportWavePci;

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

    if (IsEqualGUIDAligned(refiid, &IID_IServiceSink))
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

    return InterlockedIncrement(&This->ref);
}

static
ULONG
NTAPI
IServiceSink_fnRelease(
    IServiceSink* iface)
{
    IPortWavePciImpl * This = (IPortWavePciImpl*)CONTAINING_RECORD(iface, IPortWavePciImpl, lpVtblServiceSink);

    InterlockedDecrement(&This->ref);

    if (This->ref == 0)
    {
        ExFreePoolWithTag(This, TAG_PORTCLASS);
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
    IPortWavePciImpl * This = (IPortWavePciImpl*)iface;

    if (IsEqualGUIDAligned(refiid, &IID_IPortWavePci))
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
    return STATUS_UNSUCCESSFUL;
}

static
ULONG
NTAPI
IPortWavePci_fnAddRef(
    IPortWavePci* iface)
{
    IPortWavePciImpl * This = (IPortWavePciImpl*)iface;
    return InterlockedIncrement(&This->ref);
}

static
ULONG
NTAPI
IPortWavePci_fnRelease(
    IPortWavePci* iface)
{
    IPortWavePciImpl * This = (IPortWavePciImpl*)iface;

    InterlockedDecrement(&This->ref);

    if (This->ref == 0)
    {
        ExFreePoolWithTag(This, TAG_PORTCLASS);
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
    IPortWavePciImpl * This = (IPortWavePciImpl*)DeferredContext;
    if (This->ServiceGroup)
    {
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

    Status = Miniport->lpVtbl->Init(Miniport, UnknownAdapter, ResourceList, iface, &ServiceGroup);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IMiniportWaveCyclic_Init failed with %x\n", Status);
        return Status;
    }

    /* Initialize port object */
    This->Miniport = Miniport;
    This->pDeviceObject = DeviceObject;
    This->bInitialized = TRUE;
    This->pResourceList = ResourceList;
    This->ServiceGroup = ServiceGroup;

    /* initialize the dpc */
    KeInitializeDpc(&This->Dpc, ServiceNotifyRoutine, (PVOID)This);

    /* increment reference on miniport adapter */
    Miniport->lpVtbl->AddRef(Miniport);
    /* increment reference on resource list */
    ResourceList->lpVtbl->AddRef(ResourceList);

    /* add ourselves to service group which is called when miniport receives an isr */
    ServiceGroup->lpVtbl->AddMember(ServiceGroup, (PSERVICESINK)&This->lpVtblServiceSink);

    /* increment reference on service group */
    ServiceGroup->lpVtbl->AddRef(ServiceGroup);

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
    return STATUS_UNSUCCESSFUL;
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
    OUT PDMACHANNEL* DmaChannel,
    IN  PUNKNOWN OuterUnknown,
    IN  POOL_TYPE PoolType,
    IN  PRESOURCELIST ResourceList OPTIONAL,
    IN  BOOL ScatterGather,
    IN  BOOL Dma32BitAddresses,
    IN  BOOL Dma64BitAddresses,
    IN  DMA_WIDTH DmaWidth,
    IN  DMA_SPEED DmaSpeed,
    IN  ULONG MaximumLength,
    IN  ULONG DmaPort)
{
    return STATUS_UNSUCCESSFUL;
}

VOID
NTAPI
IPortWavePci_fnNotify(
    IN IPortWavePci * iface,
    IN  PSERVICEGROUP ServiceGroup)
{
    IPortWavePciImpl * This = (IPortWavePciImpl*)iface;
    KeInsertQueueDpc(&This->Dpc, NULL, NULL);
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

    This = ExAllocatePoolWithTag(NonPagedPool, sizeof(IPortWavePciImpl), TAG_PORTCLASS);
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(This, sizeof(IPortWavePciImpl));
    This->lpVtblServiceSink = &vt_IServiceSink;
    This->lpVtbl = &vt_IPortWavePci;
    This->ref = 1;

    *OutPort = (PPORT)&This->lpVtbl;
    return STATUS_SUCCESS;
}
