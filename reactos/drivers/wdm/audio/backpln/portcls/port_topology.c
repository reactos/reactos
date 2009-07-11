/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/port_topology.c
 * PURPOSE:         Topology Port driver
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.h"

typedef struct
{
    IPortTopologyVtbl *lpVtbl;
    ISubdeviceVtbl *lpVtblSubDevice;

    LONG ref;
    BOOL bInitialized;

    PMINIPORTTOPOLOGY pMiniport;
    PDEVICE_OBJECT pDeviceObject;
    PPINCOUNT pPinCount;
    PPOWERNOTIFY pPowerNotify;

    PPCFILTER_DESCRIPTOR pDescriptor;
    PSUBDEVICE_DESCRIPTOR SubDeviceDescriptor;
    IPortFilterTopology * Filter;
}IPortTopologyImpl;

typedef struct
{
    PIRP Irp;
    IIrpTarget *Filter;
    PIO_WORKITEM WorkItem;
}PIN_WORKER_CONTEXT, *PPIN_WORKER_CONTEXT;

static GUID InterfaceGuids[2] = 
{
    {
        /// KS_CATEGORY_TOPOLOGY
        0xDDA54A40, 0x1E4C, 0x11D1, {0xA0, 0x50, 0x40, 0x57, 0x05, 0xC1, 0x00, 0x00}
    },
    {
        /// KS_CATEGORY_AUDIO
        0x6994AD04, 0x93EF, 0x11D0, {0xA3, 0xCC, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}
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
// IUnknown interface functions
//

NTSTATUS
NTAPI
IPortTopology_fnQueryInterface(
    IPortTopology* iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    UNICODE_STRING GuidString;
    IPortTopologyImpl * This = (IPortTopologyImpl*)iface;

    DPRINT("IPortTopology_fnQueryInterface\n");

    if (IsEqualGUIDAligned(refiid, &IID_IPortTopology) ||
        IsEqualGUIDAligned(refiid, &IID_IPort) ||
        IsEqualGUIDAligned(refiid, &IID_IUnknown))
    {
        *Output = &This->lpVtbl;
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
    else if (IsEqualGUIDAligned(refiid, &IID_IUnregisterSubdevice))
    {
        return NewIUnregisterSubdevice((PUNREGISTERSUBDEVICE*)Output);
    }

    if (RtlStringFromGUID(refiid, &GuidString) == STATUS_SUCCESS)
    {
        DPRINT1("IPortTopology_fnQueryInterface no interface!!! iface %S\n", GuidString.Buffer);
        RtlFreeUnicodeString(&GuidString);
    }
    return STATUS_UNSUCCESSFUL;
}

ULONG
NTAPI
IPortTopology_fnAddRef(
    IPortTopology* iface)
{
    IPortTopologyImpl * This = (IPortTopologyImpl*)iface;

    return InterlockedIncrement(&This->ref);
}

ULONG
NTAPI
IPortTopology_fnRelease(
    IPortTopology* iface)
{
    IPortTopologyImpl * This = (IPortTopologyImpl*)iface;

    InterlockedDecrement(&This->ref);

    if (This->ref == 0)
    {
        FreeItem(This, TAG_PORTCLASS);
        return 0;
    }
    /* Return new reference count */
    return This->ref;
}


//---------------------------------------------------------------
// IPort interface functions
//

NTSTATUS
NTAPI
IPortTopology_fnGetDeviceProperty(
    IN IPortTopology * iface,
    IN DEVICE_REGISTRY_PROPERTY  DeviceRegistryProperty,
    IN ULONG  BufferLength,
    OUT PVOID  PropertyBuffer,
    OUT PULONG  ReturnLength)
{
    IPortTopologyImpl * This = (IPortTopologyImpl*)iface;
    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (!This->bInitialized)
    {
        DPRINT("IPortTopology_fnNewRegistryKey called w/o initiazed\n");
        return STATUS_UNSUCCESSFUL;
    }

    return IoGetDeviceProperty(This->pDeviceObject, DeviceRegistryProperty, BufferLength, PropertyBuffer, ReturnLength);
}

NTSTATUS
NTAPI
IPortTopology_fnInit(
    IN IPortTopology * iface,
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp,
    IN PUNKNOWN  UnknownMiniport,
    IN PUNKNOWN  UnknownAdapter  OPTIONAL,
    IN PRESOURCELIST  ResourceList)
{
    IMiniportTopology * Miniport;
    NTSTATUS Status;
    IPortTopologyImpl * This = (IPortTopologyImpl*)iface;

    DPRINT("IPortTopology_fnInit entered This %p DeviceObject %p Irp %p UnknownMiniport %p UnknownAdapter %p ResourceList %p\n",
            This, DeviceObject, Irp, UnknownMiniport, UnknownAdapter, ResourceList);
    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (This->bInitialized)
    {
        DPRINT1("IPortTopology_Init called again\n");
        return STATUS_SUCCESS;
    }

    Status = UnknownMiniport->lpVtbl->QueryInterface(UnknownMiniport, &IID_IMiniportTopology, (PVOID*)&Miniport);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IPortTopology_Init called with invalid IMiniport adapter\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* Initialize port object */
    This->pMiniport = Miniport;
    This->pDeviceObject = DeviceObject;
    This->bInitialized = TRUE;

    /* increment reference on miniport adapter */
    Miniport->lpVtbl->AddRef(Miniport);

    Status = Miniport->lpVtbl->Init(Miniport, UnknownAdapter, ResourceList, iface);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IPortTopology_Init failed with %x\n", Status);
        This->bInitialized = FALSE;
        Miniport->lpVtbl->Release(Miniport);
        return Status;
    }

    /* get the miniport device descriptor */
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
                                         This->pDescriptor);


    DPRINT("IPortTopology_fnInit success\n");
    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
IPortTopology_fnNewRegistryKey(
    IN IPortTopology * iface,
    OUT PREGISTRYKEY  *OutRegistryKey,
    IN PUNKNOWN  OuterUnknown  OPTIONAL,
    IN ULONG  RegistryKeyType,
    IN ACCESS_MASK  DesiredAccess,
    IN POBJECT_ATTRIBUTES  ObjectAttributes  OPTIONAL,
    IN ULONG  CreateOptions  OPTIONAL,
    OUT PULONG  Disposition  OPTIONAL)
{
    IPortTopologyImpl * This = (IPortTopologyImpl*)iface;
    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (!This->bInitialized)
    {
        DPRINT("IPortTopology_fnNewRegistryKey called w/o initialized\n");
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

static IPortTopologyVtbl vt_IPortTopology =
{
    /* IUnknown methods */
    IPortTopology_fnQueryInterface,
    IPortTopology_fnAddRef,
    IPortTopology_fnRelease,
    /* IPort methods */
    IPortTopology_fnInit,
    IPortTopology_fnGetDeviceProperty,
    IPortTopology_fnNewRegistryKey
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
    IPortTopologyImpl * This = (IPortTopologyImpl*)CONTAINING_RECORD(iface, IPortTopologyImpl, lpVtblSubDevice);

    return IPortTopology_fnQueryInterface((IPortTopology*)This, InterfaceId, Interface);
}

static
ULONG
NTAPI
ISubDevice_fnAddRef(
    IN ISubdevice *iface)
{
    IPortTopologyImpl * This = (IPortTopologyImpl*)CONTAINING_RECORD(iface, IPortTopologyImpl, lpVtblSubDevice);

    return IPortTopology_fnAddRef((IPortTopology*)This);
}

static
ULONG
NTAPI
ISubDevice_fnRelease(
    IN ISubdevice *iface)
{
    IPortTopologyImpl * This = (IPortTopologyImpl*)CONTAINING_RECORD(iface, IPortTopologyImpl, lpVtblSubDevice);

    return IPortTopology_fnRelease((IPortTopology*)This);
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
    IPortFilterTopology * Filter;
    IPortTopologyImpl * This = (IPortTopologyImpl*)CONTAINING_RECORD(iface, IPortTopologyImpl, lpVtblSubDevice);

    /* is there already an instance of the filter */
    if (This->Filter)
    {
        /* it is, let's return the result */
        *OutTarget = (IIrpTarget*)This->Filter;

        /* increment reference */
        This->Filter->lpVtbl->AddRef(This->Filter);
        return STATUS_SUCCESS;
    }

    /* create new instance of filter */
    Status = NewPortFilterTopology(&Filter);
    if (!NT_SUCCESS(Status))
    {
        /* not enough memory */
        return Status;
    }

    /* initialize the filter */
    Status = Filter->lpVtbl->Init(Filter, (IPortTopology*)This);
    if (!NT_SUCCESS(Status))
    {
        /* destroy filter */
        Filter->lpVtbl->Release(Filter);
        /* return status */
        return Status;
    }

    /* store result */
    *OutTarget = (IIrpTarget*)Filter;
    /* store for later re-use */
    This->Filter = Filter;
    /* return status */
    return Status;
}

static
NTSTATUS
NTAPI
ISubDevice_fnReleaseChildren(
    IN ISubdevice *iface)
{
    //IPortTopologyImpl * This = (IPortTopologyImpl*)CONTAINING_RECORD(iface, IPortTopologyImpl, lpVtblSubDevice);

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
    IPortTopologyImpl * This = (IPortTopologyImpl*)CONTAINING_RECORD(iface, IPortTopologyImpl, lpVtblSubDevice);

    DPRINT("ISubDevice_GetDescriptor this %p Descp %p\n", This, This->SubDeviceDescriptor);
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
    IPortTopologyImpl * This = (IPortTopologyImpl*)CONTAINING_RECORD(iface, IPortTopologyImpl, lpVtblSubDevice);

    DPRINT("ISubDevice_DataRangeIntersection this %p\n", This);

    if (This->pMiniport)
    {
        return This->pMiniport->lpVtbl->DataRangeIntersection (This->pMiniport, PinId, DataRange, MatchingDataRange, OutputBufferLength, ResultantFormat, ResultantFormatLength);
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
    IPortTopologyImpl * This = (IPortTopologyImpl*)CONTAINING_RECORD(iface, IPortTopologyImpl, lpVtblSubDevice);

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
    IPortTopologyImpl * This = (IPortTopologyImpl*)CONTAINING_RECORD(iface, IPortTopologyImpl, lpVtblSubDevice);

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

VOID
NTAPI
CreatePinWorkerRoutine(
    IN PDEVICE_OBJECT DeviceObject,
    IN PVOID  Context)
{
    NTSTATUS Status;
    IIrpTarget *Pin;
    PPIN_WORKER_CONTEXT WorkerContext = (PPIN_WORKER_CONTEXT)Context;

    DPRINT("CreatePinWorkerRoutine called\n");
    /* create the pin */
    Status = WorkerContext->Filter->lpVtbl->NewIrpTarget(WorkerContext->Filter,
                                                         &Pin,
                                                         NULL,
                                                         NULL,
                                                         NonPagedPool,
                                                         DeviceObject,
                                                         WorkerContext->Irp,
                                                         NULL);

    DPRINT("CreatePinWorkerRoutine Status %x\n", Status);

    if (NT_SUCCESS(Status))
    {
        /* create the dispatch object */
        Status = NewDispatchObject(WorkerContext->Irp, Pin, NULL);
        DPRINT("Pin %p\n", Pin);
    }

    DPRINT("CreatePinWorkerRoutine completing irp %p\n", WorkerContext->Irp);
    /* save status in irp */
    WorkerContext->Irp->IoStatus.Status = Status;
    WorkerContext->Irp->IoStatus.Information = 0;
    /* complete the request */
    IoCompleteRequest(WorkerContext->Irp, IO_SOUND_INCREMENT);
    /* free allocated work item */
    IoFreeWorkItem(WorkerContext->WorkItem);
    /* free context */
    FreeItem(WorkerContext, TAG_PORTCLASS);
}


NTSTATUS
NTAPI
PcCreateItemDispatch(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IoStack;
    ISubdevice * SubDevice;
    PPCLASS_DEVICE_EXTENSION DeviceExt;
    IIrpTarget *Filter;
    PKSOBJECT_CREATE_ITEM CreateItem;
    PPIN_WORKER_CONTEXT Context;
    LPWSTR Buffer;
    static LPWSTR KS_NAME_PIN = L"{146F1A80-4791-11D0-A5D6-28DB04C10000}";

    DPRINT1("PcCreateItemDispatch called DeviceObject %p\n", DeviceObject);

    /* access the create item */
    CreateItem = KSCREATE_ITEM_IRP_STORAGE(Irp);
    if (!CreateItem)
    {
        DPRINT1("PcCreateItemDispatch no CreateItem\n");
        return STATUS_UNSUCCESSFUL;
    }

    SubDevice = (ISubdevice*)CreateItem->Context;
    DeviceExt = (PPCLASS_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    if (!SubDevice || !DeviceExt)
    {
        DPRINT1("PcCreateItemDispatch SubDevice %p DeviceExt %p\n", SubDevice, DeviceExt);
        return STATUS_UNSUCCESSFUL;
    }

#if KS_IMPLEMENTED
    Status = KsReferenceSoftwareBusObject(DeviceExt->KsDeviceHeader);
    if (!NT_SUCCESS(Status) && Status != STATUS_NOT_IMPLEMENTED)
    {
        DPRINT1("PciCreateItemDispatch failed to reference device header\n");

        FreeItem(Entry, TAG_PORTCLASS);
        goto cleanup;
    }
#endif


    /* get current io stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);
    /* sanity check */
    ASSERT(IoStack->FileObject != NULL);

    if (IoStack->FileObject->FsContext != NULL)
    {
        /* nothing to do */
        DPRINT1("FsContext already exists\n");
        return STATUS_SUCCESS;
    }


    /* get filter object 
     * is implemented as a singleton
     */
    Status = SubDevice->lpVtbl->NewIrpTarget(SubDevice,
                                             &Filter,
                                             NULL,
                                             NULL,
                                             NonPagedPool,
                                             DeviceObject,
                                             Irp,
                                             NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to get filter object\n");
        return Status;
    }

    /* get the buffer */
    Buffer = IoStack->FileObject->FileName.Buffer;

    /* check if the request contains a pin request */
    if (!wcsstr(Buffer, KS_NAME_PIN))
    {
        /* creator just wants the filter object */
        Status = NewDispatchObject(Irp, Filter, CreateItem->ObjectClass.Buffer);

        DPRINT1("Filter %p\n", Filter);
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }
    else
    {
        /* try to create new pin */
        Context = AllocateItem(NonPagedPool, sizeof(PIN_WORKER_CONTEXT), TAG_PORTCLASS);
        if (!Context)
        {
            DPRINT("Failed to allocate worker context\n");
            Irp->IoStatus.Information = 0;
            Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        /* allocate work item */
        Context->WorkItem = IoAllocateWorkItem(DeviceObject);
        if (!Context->WorkItem)
        {
            DPRINT("Failed to allocate workitem\n");
            FreeItem(Context, TAG_PORTCLASS);
            Irp->IoStatus.Information = 0;
            Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        Context->Filter = Filter;
        Context->Irp = Irp;

        DPRINT("Queueing IRP %p Irql %u\n", Irp, KeGetCurrentIrql());
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_PENDING;
        IoMarkIrpPending(Irp);
        IoQueueWorkItem(Context->WorkItem, CreatePinWorkerRoutine, DelayedWorkQueue, (PVOID)Context);
        return STATUS_PENDING;
    }
}



NTSTATUS
NewPortTopology(
    OUT PPORT* OutPort)
{
    IPortTopologyImpl * This;

    This = AllocateItem(NonPagedPool, sizeof(IPortTopologyImpl), TAG_PORTCLASS);
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    This->lpVtbl = &vt_IPortTopology;
    This->lpVtblSubDevice = &vt_ISubdeviceVtbl;
    This->ref = 1;
    *OutPort = (PPORT)(&This->lpVtbl);
    DPRINT("NewPortTopology result %p\n", *OutPort);

    return STATUS_SUCCESS;
}

PMINIPORTTOPOLOGY
GetTopologyMiniport(
    PPORTTOPOLOGY Port)
{
    IPortTopologyImpl * This = (IPortTopologyImpl*)Port;
    return This->pMiniport;
}
