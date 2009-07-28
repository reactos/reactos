/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/ksfilter/ks/filter.c
 * PURPOSE:         KS IKsFilter interface functions
 * PROGRAMMER:      Johannes Anderwald
 */


#include "priv.h"

typedef struct
{
    KSBASIC_HEADER Header;
    KSFILTER Filter;

    IKsFilterVtbl *lpVtbl;
    IKsControlVtbl *lpVtblKsControl;
    IKsFilterFactory * FilterFactory;
    LONG ref;

    PKSIOBJECT_HEADER ObjectHeader;
    KSTOPOLOGY Topology;
    KSPIN_DESCRIPTOR * PinDescriptors;
    ULONG PinDescriptorCount;
    PKSFILTERFACTORY Factory;
    PFILE_OBJECT FileObject;


    ULONG *PinInstanceCount;
}IKsFilterImpl;

const GUID IID_IKsControl = {0x28F54685L, 0x06FD, 0x11D2, {0xB2, 0x7A, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};
const GUID IID_IKsFilter  = {0x3ef6ee44L, 0x0D41, 0x11d2, {0xbe, 0xDA, 0x00, 0xc0, 0x4f, 0x8e, 0xF4, 0x57}};
const GUID KSPROPSETID_Topology                = {0x720D4AC0L, 0x7533, 0x11D0, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};
const GUID KSPROPSETID_Pin                     = {0x8C134960L, 0x51AD, 0x11CF, {0x87, 0x8A, 0x94, 0xF8, 0x01, 0xC1, 0x00, 0x00}};


DEFINE_KSPROPERTY_TOPOLOGYSET(IKsFilterTopologySet, KspTopologyPropertyHandler);
DEFINE_KSPROPERTY_PINPROPOSEDATAFORMAT(IKsFilterPinSet, KspPinPropertyHandler, KspPinPropertyHandler, KspPinPropertyHandler);

KSPROPERTY_SET FilterPropertySet[] =
{
    {
        &KSPROPSETID_Topology,
        sizeof(IKsFilterTopologySet) / sizeof(KSPROPERTY_ITEM),
        (const KSPROPERTY_ITEM*)&IKsFilterTopologySet,
        0,
        NULL
    },
    {
        &KSPROPSETID_Pin,
        sizeof(IKsFilterPinSet) / sizeof(KSPROPERTY_ITEM),
        (const KSPROPERTY_ITEM*)&IKsFilterPinSet,
        0,
        NULL
    }
};

NTSTATUS
NTAPI
IKsControl_fnQueryInterface(
    IKsControl * iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(iface, IKsFilterImpl, lpVtblKsControl);

    if (IsEqualGUIDAligned(refiid, &IID_IUnknown))
    {
        *Output = &This->lpVtbl;
        _InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }
    return STATUS_UNSUCCESSFUL;
}

ULONG
NTAPI
IKsControl_fnAddRef(
    IKsControl * iface)
{
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(iface, IKsFilterImpl, lpVtblKsControl);

    return InterlockedIncrement(&This->ref);
}

ULONG
NTAPI
IKsControl_fnRelease(
    IKsControl * iface)
{
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(iface, IKsFilterImpl, lpVtblKsControl);

    InterlockedDecrement(&This->ref);

    /* Return new reference count */
    return This->ref;
}

NTSTATUS
NTAPI
IKsControl_fnKsProperty(
    IKsControl * iface,
    IN PKSPROPERTY Property,
    IN ULONG PropertyLength,
    IN OUT PVOID PropertyData,
    IN ULONG DataLength,
    OUT ULONG* BytesReturned)
{
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(iface, IKsFilterImpl, lpVtblKsControl);

    return KsSynchronousIoControlDevice(This->FileObject, KernelMode, IOCTL_KS_PROPERTY, Property, PropertyLength, PropertyData, DataLength, BytesReturned);
}


NTSTATUS
NTAPI
IKsControl_fnKsMethod(
    IKsControl * iface,
    IN PKSMETHOD Method,
    IN ULONG MethodLength,
    IN OUT PVOID MethodData,
    IN ULONG DataLength,
    OUT ULONG* BytesReturned)
{
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(iface, IKsFilterImpl, lpVtblKsControl);

    return KsSynchronousIoControlDevice(This->FileObject, KernelMode, IOCTL_KS_METHOD, Method, MethodLength, MethodData, DataLength, BytesReturned);
}


NTSTATUS
NTAPI
IKsControl_fnKsEvent(
    IKsControl * iface,
    IN PKSEVENT Event OPTIONAL,
    IN ULONG EventLength,
    IN OUT PVOID EventData,
    IN ULONG DataLength,
    OUT ULONG* BytesReturned)
{
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(iface, IKsFilterImpl, lpVtblKsControl);

    if (Event)
    {
       return KsSynchronousIoControlDevice(This->FileObject, KernelMode, IOCTL_KS_ENABLE_EVENT, Event, EventLength, EventData, DataLength, BytesReturned);
    }
    else
    {
       return KsSynchronousIoControlDevice(This->FileObject, KernelMode, IOCTL_KS_DISABLE_EVENT, EventData, DataLength, NULL, 0, BytesReturned);
    }

}

static IKsControlVtbl vt_IKsControl =
{
    IKsControl_fnQueryInterface,
    IKsControl_fnAddRef,
    IKsControl_fnRelease,
    IKsControl_fnKsProperty,
    IKsControl_fnKsMethod,
    IKsControl_fnKsEvent
};


NTSTATUS
NTAPI
IKsFilter_fnQueryInterface(
    IKsFilter * iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(iface, IKsFilterImpl, lpVtbl);

    if (IsEqualGUIDAligned(refiid, &IID_IUnknown) ||
        IsEqualGUIDAligned(refiid, &IID_IKsFilter))
    {
        *Output = &This->lpVtbl;
        _InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }
    else if (IsEqualGUIDAligned(refiid, &IID_IKsControl))
    {
        *Output = &This->lpVtblKsControl;
        _InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;
}

ULONG
NTAPI
IKsFilter_fnAddRef(
    IKsFilter * iface)
{
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(iface, IKsFilterImpl, lpVtbl);

    return InterlockedIncrement(&This->ref);
}

ULONG
NTAPI
IKsFilter_fnRelease(
    IKsFilter * iface)
{
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(iface, IKsFilterImpl, lpVtbl);

    InterlockedDecrement(&This->ref);

    if (This->ref == 0)
    {
        FreeItem(This);
        return 0;
    }
    /* Return new reference count */
    return This->ref;

}

PKSFILTER
NTAPI
IKsFilter_fnGetStruct(
    IKsFilter * iface)
{
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(iface, IKsFilterImpl, lpVtbl);

    return &This->Filter;
}

BOOL
NTAPI
IKsFilter_fnDoAllNecessaryPinsExist(
    IKsFilter * iface)
{
    UNIMPLEMENTED
    return FALSE;
}

NTSTATUS
NTAPI
IKsFilter_fnCreateNode(
    IKsFilter * iface,
    IN PIRP Irp,
    IN IKsPin * Pin,
    IN PLIST_ENTRY ListEntry)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
IKsFilter_fnBindProcessPinsToPipeSection(
    IKsFilter * iface,
    IN struct KSPROCESSPIPESECTION *Section,
    IN PVOID Create,
    IN PKSPIN KsPin,
    OUT IKsPin **Pin,
    OUT PKSGATE *OutGate)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
IKsFilter_fnUnbindProcessPinsFromPipeSection(
    IKsFilter * iface,
    IN struct KSPROCESSPIPESECTION *Section)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
IKsFilter_fnAddProcessPin(
    IKsFilter * iface,
    IN PKSPROCESSPIN ProcessPin)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
IKsFilter_fnRemoveProcessPin(
    IKsFilter * iface,
    IN PKSPROCESSPIN ProcessPin)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

BOOL
NTAPI
IKsFilter_fnReprepareProcessPipeSection(
    IKsFilter * iface,
    IN struct KSPROCESSPIPESECTION *PipeSection,
    IN PULONG Data)
{
    UNIMPLEMENTED
    return FALSE;
}

VOID
NTAPI
IKsFilter_fnDeliverResetState(
    IKsFilter * iface,
    IN struct KSPROCESSPIPESECTION *PipeSection,
    IN KSRESET ResetState)
{
    UNIMPLEMENTED
}

BOOL
NTAPI
IKsFilter_fnIsFrameHolding(
    IKsFilter * iface)
{
    UNIMPLEMENTED
    return FALSE;
}

VOID
NTAPI
IKsFilter_fnRegisterForCopyCallbacks(
    IKsFilter * iface,
    IKsQueue *Queue,
    BOOL Register)
{
    UNIMPLEMENTED
}

PKSPROCESSPIN_INDEXENTRY
NTAPI
IKsFilter_fnGetProcessDispatch(
    IKsFilter * iface)
{
    UNIMPLEMENTED
    return NULL;
}

static IKsFilterVtbl vt_IKsFilter =
{
    IKsFilter_fnQueryInterface,
    IKsFilter_fnAddRef,
    IKsFilter_fnRelease,
    IKsFilter_fnGetStruct,
    IKsFilter_fnDoAllNecessaryPinsExist,
    IKsFilter_fnCreateNode,
    IKsFilter_fnBindProcessPinsToPipeSection,
    IKsFilter_fnUnbindProcessPinsFromPipeSection,
    IKsFilter_fnAddProcessPin,
    IKsFilter_fnRemoveProcessPin,
    IKsFilter_fnReprepareProcessPipeSection,
    IKsFilter_fnDeliverResetState,
    IKsFilter_fnIsFrameHolding,
    IKsFilter_fnRegisterForCopyCallbacks,
    IKsFilter_fnGetProcessDispatch
};

NTSTATUS
IKsFilter_GetFilterFromIrp(
    IN PIRP Irp,
    OUT IKsFilter **Filter)
{
    PIO_STACK_LOCATION IoStack;
    PKSIOBJECT_HEADER ObjectHeader;
    NTSTATUS Status;

    /* get current irp stack */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* santiy check */
    ASSERT(IoStack->FileObject != NULL);

    ObjectHeader = (PKSIOBJECT_HEADER)IoStack->FileObject->FsContext;

    /* sanity is important */
    ASSERT(ObjectHeader != NULL);
    ASSERT(ObjectHeader->Type == KsObjectTypeFilter);
    ASSERT(ObjectHeader->Unknown != NULL);

    /* get our private interface */
    Status = ObjectHeader->Unknown->lpVtbl->QueryInterface(ObjectHeader->Unknown, &IID_IKsFilter, (PVOID*)Filter);

    if (!NT_SUCCESS(Status))
    {
        /* something is wrong here */
        DPRINT1("KS: Misbehaving filter %p\n", ObjectHeader->Unknown);
        Irp->IoStatus.Status = Status;

        /* complete and forget irp */
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }
    return Status;
}


NTSTATUS
NTAPI
IKsFilter_DispatchClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    IKsFilter * Filter;
    IKsFilterImpl * This;
    NTSTATUS Status;

    /* obtain filter from object header */
    Status = IKsFilter_GetFilterFromIrp(Irp, &Filter);
    if (!NT_SUCCESS(Status))
        return Status;

    /* get our real implementation */
    This = (IKsFilterImpl*)CONTAINING_RECORD(Filter, IKsFilterImpl, lpVtbl);

    /* does the driver support notifications */
    if (This->Factory->FilterDescriptor && This->Factory->FilterDescriptor->Dispatch && This->Factory->FilterDescriptor->Dispatch->Close)
    {
        /* call driver's filter close function */
        Status = This->Factory->FilterDescriptor->Dispatch->Close(&This->Filter, Irp);
    }

    if (Status != STATUS_PENDING)
    {
        /* save the result */
        Irp->IoStatus.Status = Status;
        /* complete irp */
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        /* remove our instance from the filter factory */
        This->FilterFactory->lpVtbl->RemoveFilterInstance(This->FilterFactory, Filter);

        /* now release the acquired interface */
        Filter->lpVtbl->Release(Filter);

        /* free object header */
        KsFreeObjectHeader(This->ObjectHeader);
    }

    /* done */
    return Status;
}

NTSTATUS
KspHandlePropertyInstances(
    IN PIO_STATUS_BLOCK IoStatus,
    IN PKSIDENTIFIER  Request,
    IN OUT PVOID  Data,
    IN IKsFilterImpl * This,
    IN BOOL Global)
{
    KSPIN_CINSTANCES * Instances;
    KSP_PIN * Pin = (KSP_PIN*)Request;

    if (!This->Factory->FilterDescriptor || !This->Factory->FilterDescriptor->PinDescriptorsCount)
    {
        /* no filter / pin descriptor */
        IoStatus->Status = STATUS_NOT_IMPLEMENTED;
        return STATUS_NOT_IMPLEMENTED;
    }

    /* ignore custom structs for now */
    ASSERT(This->Factory->FilterDescriptor->PinDescriptorSize == sizeof(KSPIN_DESCRIPTOR_EX)); 
    ASSERT(This->Factory->FilterDescriptor->PinDescriptorsCount > Pin->PinId);

    Instances = (KSPIN_CINSTANCES*)Data;
    /* max instance count */
    Instances->PossibleCount = This->Factory->FilterDescriptor->PinDescriptors[Pin->PinId].InstancesPossible;
    /* current instance count */
    Instances->CurrentCount = This->PinInstanceCount[Pin->PinId];

    IoStatus->Information = sizeof(KSPIN_CINSTANCES);
    IoStatus->Status = STATUS_SUCCESS;
    return STATUS_SUCCESS;
}

NTSTATUS
KspHandleNecessaryPropertyInstances(
    IN PIO_STATUS_BLOCK IoStatus,
    IN PKSIDENTIFIER  Request,
    IN OUT PVOID  Data,
    IN IKsFilterImpl * This)
{
    PULONG Result;
    KSP_PIN * Pin = (KSP_PIN*)Request;

    if (!This->Factory->FilterDescriptor || !This->Factory->FilterDescriptor->PinDescriptorsCount)
    {
        /* no filter / pin descriptor */
        IoStatus->Status = STATUS_NOT_IMPLEMENTED;
        return STATUS_NOT_IMPLEMENTED;
    }

    /* ignore custom structs for now */
    ASSERT(This->Factory->FilterDescriptor->PinDescriptorSize == sizeof(KSPIN_DESCRIPTOR_EX)); 
    ASSERT(This->Factory->FilterDescriptor->PinDescriptorsCount > Pin->PinId);

    Result = (PULONG)Data;
    *Result = This->Factory->FilterDescriptor->PinDescriptors[Pin->PinId].InstancesNecessary;

    IoStatus->Information = sizeof(ULONG);
    IoStatus->Status = STATUS_SUCCESS;
    return STATUS_SUCCESS;
}

NTSTATUS
KspHandleDataIntersection(
    IN PIRP Irp,
    IN PIO_STATUS_BLOCK IoStatus,
    IN PKSIDENTIFIER Request,
    IN OUT PVOID  Data,
    IN ULONG DataLength,
    IN IKsFilterImpl * This)
{
    PKSMULTIPLE_ITEM MultipleItem;
    PKSDATARANGE DataRange;
    NTSTATUS Status = STATUS_NO_MATCH;
    ULONG Index, Length;
    KSP_PIN * Pin = (KSP_PIN*)Request;

    /* Access parameters */
    MultipleItem = (PKSMULTIPLE_ITEM)(Pin + 1);
    DataRange = (PKSDATARANGE)(MultipleItem + 1);

    if (!This->Factory->FilterDescriptor || !This->Factory->FilterDescriptor->PinDescriptorsCount)
    {
        /* no filter / pin descriptor */
        IoStatus->Status = STATUS_NOT_IMPLEMENTED;
        return STATUS_NOT_IMPLEMENTED;
    }

    /* ignore custom structs for now */
    ASSERT(This->Factory->FilterDescriptor->PinDescriptorSize == sizeof(KSPIN_DESCRIPTOR_EX)); 
    ASSERT(This->Factory->FilterDescriptor->PinDescriptorsCount > Pin->PinId);

    if (This->Factory->FilterDescriptor->PinDescriptors[Pin->PinId].IntersectHandler == NULL ||
        This->Factory->FilterDescriptor->PinDescriptors[Pin->PinId].PinDescriptor.DataRanges == NULL ||
        This->Factory->FilterDescriptor->PinDescriptors[Pin->PinId].PinDescriptor.DataRangesCount == 0)
    {
        /* no driver supported intersect handler / no provided data ranges */
        IoStatus->Status = STATUS_NOT_IMPLEMENTED;
        return STATUS_NOT_IMPLEMENTED;
    }

    for(Index = 0; Index < MultipleItem->Count; Index++)
    {
        /* Call miniport's properitary handler */
        Status = This->Factory->FilterDescriptor->PinDescriptors[Pin->PinId].IntersectHandler(NULL, /* context */
                                                                                              Irp,
                                                                                              Pin,
                                                                                              DataRange,
                                                                                              (PKSDATAFORMAT)This->Factory->FilterDescriptor->PinDescriptors[Pin->PinId].PinDescriptor.DataRanges,
                                                                                              DataLength,
                                                                                              Data,
                                                                                              &Length);

        if (Status == STATUS_SUCCESS)
        {
            IoStatus->Information = Length;
            break;
        }
        DataRange =  UlongToPtr(PtrToUlong(DataRange) + DataRange->FormatSize);
    }

    IoStatus->Status = Status;
    return Status;
}

NTSTATUS
NTAPI
KspPinPropertyHandler(
    IN PIRP Irp,
    IN PKSIDENTIFIER  Request,
    IN OUT PVOID  Data)
{
    PIO_STACK_LOCATION IoStack;
    IKsFilterImpl * This;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    /* get filter implementation */
    This = (IKsFilterImpl*)KSPROPERTY_ITEM_IRP_STORAGE(Irp);

    /* get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    switch(Request->Id)
    {
        case KSPROPERTY_PIN_CTYPES:
        case KSPROPERTY_PIN_DATAFLOW:
        case KSPROPERTY_PIN_DATARANGES:
        case KSPROPERTY_PIN_INTERFACES:
        case KSPROPERTY_PIN_MEDIUMS:
        case KSPROPERTY_PIN_COMMUNICATION:
        case KSPROPERTY_PIN_CATEGORY:
        case KSPROPERTY_PIN_NAME:
        case KSPROPERTY_PIN_PROPOSEDATAFORMAT:
            Status = KsPinPropertyHandler(Irp, Request, Data, This->PinDescriptorCount, This->PinDescriptors);
            break;
        case KSPROPERTY_PIN_GLOBALCINSTANCES:
            Status = KspHandlePropertyInstances(&Irp->IoStatus, Request, Data, This, TRUE);
            break;
        case KSPROPERTY_PIN_CINSTANCES:
            Status = KspHandlePropertyInstances(&Irp->IoStatus, Request, Data, This, FALSE);
            break;
        case KSPROPERTY_PIN_NECESSARYINSTANCES:
            Status = KspHandleNecessaryPropertyInstances(&Irp->IoStatus, Request, Data, This);
            break;

        case KSPROPERTY_PIN_DATAINTERSECTION:
            Status = KspHandleDataIntersection(Irp, &Irp->IoStatus, Request, Data, IoStack->Parameters.DeviceIoControl.OutputBufferLength, This);
            break;
        case KSPROPERTY_PIN_PHYSICALCONNECTION:
        case KSPROPERTY_PIN_CONSTRAINEDDATARANGES:
            UNIMPLEMENTED
            Status = STATUS_NOT_IMPLEMENTED;
            break;
        default:
            UNIMPLEMENTED
            Status = STATUS_UNSUCCESSFUL;
    }

    return Status;
}

NTSTATUS
FindPropertyHandler(
    IN PIO_STATUS_BLOCK IoStatus,
    IN KSPROPERTY_SET * FilterPropertySet,
    IN ULONG FilterPropertySetCount,
    IN PKSPROPERTY Property,
    IN ULONG InputBufferLength,
    IN ULONG OutputBufferLength,
    OUT PFNKSHANDLER *PropertyHandler)
{
    ULONG Index, ItemIndex;

    for(Index = 0; Index < FilterPropertySetCount; Index++)
    {
        if (IsEqualGUIDAligned(&Property->Set, FilterPropertySet[Index].Set))
        {
            for(ItemIndex = 0; ItemIndex < FilterPropertySet[Index].PropertiesCount; ItemIndex++)
            {
                if (FilterPropertySet[Index].PropertyItem[ItemIndex].PropertyId == Property->Id)
                {
                    if (Property->Flags & KSPROPERTY_TYPE_SET)
                        *PropertyHandler = FilterPropertySet[Index].PropertyItem[ItemIndex].SetPropertyHandler;

                    if (Property->Flags & KSPROPERTY_TYPE_GET)
                        *PropertyHandler = FilterPropertySet[Index].PropertyItem[ItemIndex].GetPropertyHandler;

                    if (FilterPropertySet[Index].PropertyItem[ItemIndex].MinProperty > InputBufferLength)
                    {
                        /* too small input buffer */
                        IoStatus->Information = FilterPropertySet[Index].PropertyItem[ItemIndex].MinProperty;
                        IoStatus->Status = STATUS_BUFFER_TOO_SMALL;
                        return STATUS_BUFFER_TOO_SMALL;
                    }

                    if (FilterPropertySet[Index].PropertyItem[ItemIndex].MinData > OutputBufferLength)
                    {
                        /* too small output buffer */
                        IoStatus->Information = FilterPropertySet[Index].PropertyItem[ItemIndex].MinData;
                        IoStatus->Status = STATUS_BUFFER_TOO_SMALL;
                        return STATUS_BUFFER_TOO_SMALL;
                    }
                    return STATUS_SUCCESS;
                }
            }
        }
    }
    return STATUS_UNSUCCESSFUL;
}



NTSTATUS
NTAPI
IKsFilter_DispatchDeviceIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PFNKSHANDLER PropertyHandler = NULL;
    IKsFilter * Filter;
    IKsFilterImpl * This;
    NTSTATUS Status;

    /* obtain filter from object header */
    Status = IKsFilter_GetFilterFromIrp(Irp, &Filter);
    if (!NT_SUCCESS(Status))
        return Status;

    /* get our real implementation */
    This = (IKsFilterImpl*)CONTAINING_RECORD(Filter, IKsFilterImpl, lpVtbl);

    /* current irp stack */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    if (IoStack->Parameters.DeviceIoControl.IoControlCode != IOCTL_KS_PROPERTY)
    {
        UNIMPLEMENTED;

        /* release filter interface */
        Filter->lpVtbl->Release(Filter);

        /* complete and forget irp */
        Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_NOT_IMPLEMENTED;
    }

    /* find a supported property handler */
    Status = FindPropertyHandler(&Irp->IoStatus, FilterPropertySet, 2, IoStack->Parameters.DeviceIoControl.Type3InputBuffer, IoStack->Parameters.DeviceIoControl.InputBufferLength, IoStack->Parameters.DeviceIoControl.OutputBufferLength, &PropertyHandler);
    if (NT_SUCCESS(Status))
    {
        KSPROPERTY_ITEM_IRP_STORAGE(Irp) = (PVOID)This;
        DPRINT("Calling property handler %p\n", PropertyHandler);
        Status = PropertyHandler(Irp, IoStack->Parameters.DeviceIoControl.Type3InputBuffer, Irp->UserBuffer);
    }
    else
    {
        /* call driver's property handler */
        UNIMPLEMENTED
        Status = STATUS_NOT_IMPLEMENTED;
    }

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    /* done */
    return Status;
}

static KSDISPATCH_TABLE DispatchTable =
{
    IKsFilter_DispatchDeviceIoControl,
    KsDispatchInvalidDeviceRequest,
    KsDispatchInvalidDeviceRequest,
    KsDispatchInvalidDeviceRequest,
    IKsFilter_DispatchClose,
    KsDispatchQuerySecurity,
    KsDispatchSetSecurity,
    KsDispatchFastIoDeviceControlFailure,
    KsDispatchFastReadFailure,
    KsDispatchFastReadFailure,
};


NTSTATUS
IKsFilter_CreateDescriptors(
    IKsFilterImpl * This,
    KSFILTER_DESCRIPTOR* FilterDescriptor)
{
    ULONG Index = 0;

    /* initialize pin descriptors */
    if (FilterDescriptor->PinDescriptorsCount)
    {
        /* allocate pin instance count array */
        This->PinInstanceCount = AllocateItem(NonPagedPool, sizeof(ULONG) * FilterDescriptor->PinDescriptorsCount);
        if(!This->PinDescriptors)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }


        /* allocate pin descriptor array */
        This->PinDescriptors = AllocateItem(NonPagedPool, sizeof(KSPIN_DESCRIPTOR) * FilterDescriptor->PinDescriptorsCount);
        if(!This->PinDescriptors)
        {
            FreeItem(This->PinInstanceCount);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* set pin count */
        This->PinDescriptorCount = FilterDescriptor->PinDescriptorsCount;
        /* now copy those pin descriptors over */
        for(Index = 0; Index < FilterDescriptor->PinDescriptorsCount; Index++)
        {
            /* copy one pin per time */
            RtlMoveMemory(&This->PinDescriptors[Index], &FilterDescriptor->PinDescriptors[Index].PinDescriptor, sizeof(KSPIN_DESCRIPTOR));
        }
    }

    /* initialize topology descriptor */
    This->Topology.CategoriesCount = FilterDescriptor->CategoriesCount;
    This->Topology.Categories = FilterDescriptor->Categories;
    This->Topology.TopologyNodesCount = FilterDescriptor->NodeDescriptorsCount;
    This->Topology.TopologyConnectionsCount = FilterDescriptor->ConnectionsCount;
    This->Topology.TopologyConnections = FilterDescriptor->Connections;

    if (This->Topology.TopologyNodesCount > 0)
    {
        This->Topology.TopologyNodes = AllocateItem(NonPagedPool, sizeof(GUID) * This->Topology.TopologyNodesCount);
        /* allocate topology node types array */
        if (!This->Topology.TopologyNodes)
            return STATUS_INSUFFICIENT_RESOURCES;

        This->Topology.TopologyNodesNames = AllocateItem(NonPagedPool, sizeof(GUID) * This->Topology.TopologyNodesCount);
        /* allocate topology names array */
        if (!This->Topology.TopologyNodesNames)
        {
            FreeItem((PVOID)This->Topology.TopologyNodes);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        for(Index = 0; Index < This->Topology.TopologyNodesCount; Index++)
        {
            /* copy topology type */
            RtlMoveMemory((PVOID)&This->Topology.TopologyNodes[Index], FilterDescriptor->NodeDescriptors[Index].Type, sizeof(GUID));
            /* copy topology name */
            RtlMoveMemory((PVOID)&This->Topology.TopologyNodesNames[Index], FilterDescriptor->NodeDescriptors[Index].Name, sizeof(GUID));
        }
    }

    /* done! */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
KspCreateFilter(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN IKsFilterFactory *iface)
{
    IKsFilterImpl * This;
    PKSFILTERFACTORY Factory;
    PIO_STACK_LOCATION IoStack;
    PDEVICE_EXTENSION DeviceExtension;
    NTSTATUS Status;

    /* get device extension */
    DeviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;

    /* get the filter factory */
    Factory = iface->lpVtbl->GetStruct(iface);

    if (!Factory || !Factory->FilterDescriptor || !Factory->FilterDescriptor->Dispatch || !Factory->FilterDescriptor->Dispatch->Create)
    {
        /* Sorry it just will not work */
        return STATUS_UNSUCCESSFUL;
    }

    /* allocate filter instance */
    This = AllocateItem(NonPagedPool, sizeof(IKsFilterFactory));
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* get current irp stack */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* initialize filter instance */
    This->ref = 1;
    This->lpVtbl = &vt_IKsFilter;
    This->lpVtblKsControl = &vt_IKsControl;
    This->Filter.Descriptor = Factory->FilterDescriptor;
    This->Factory = Factory;
    This->FilterFactory = iface;
    This->FileObject = IoStack->FileObject;
    This->Header.KsDevice = &DeviceExtension->DeviceHeader->KsDevice;
    This->Header.Parent.KsFilterFactory = iface->lpVtbl->GetStruct(iface);
    This->Header.Type = KsObjectTypeFilter;

    /* allocate the stream descriptors */
    Status = IKsFilter_CreateDescriptors(This, (PKSFILTER_DESCRIPTOR)Factory->FilterDescriptor);
    if (!NT_SUCCESS(Status))
    {
        /* what can go wrong, goes wrong */
        FreeItem(This);
        return Status;
    }

    /* now add the filter instance to the filter factory */
    Status = iface->lpVtbl->AddFilterInstance(iface, (IKsFilter*)&This->lpVtbl);

    if (!NT_SUCCESS(Status))
    {
        /* failed to add filter */
        FreeItem(This);

        return Status;
    }

    /* now let driver initialize the filter instance */
    Status = Factory->FilterDescriptor->Dispatch->Create(&This->Filter, Irp);

    if (!NT_SUCCESS(Status) && Status != STATUS_PENDING)
    {
        /* driver failed to initialize */
        DPRINT1("Driver: Status %x\n", Status);

        /* remove filter instance from filter factory */
        iface->lpVtbl->RemoveFilterInstance(iface, (IKsFilter*)&This->lpVtbl);

        /* free filter instance */
        FreeItem(This);

        return Status;
    }

    /* now allocate the object header */
    Status = KsAllocateObjectHeader((PVOID*)&This->ObjectHeader, 0, NULL, Irp, &DispatchTable);
    if (!NT_SUCCESS(Status))
    {
        /* failed to allocate object header */
        DPRINT1("Failed to allocate object header %x\n", Status);

        return Status;
    }

    /* initialize object header */
    This->Header.Type = KsObjectTypeFilter;
    This->Header.KsDevice = &DeviceExtension->DeviceHeader->KsDevice;
    This->ObjectHeader->Type = KsObjectTypeFilter;
    This->ObjectHeader->Unknown = (PUNKNOWN)&This->lpVtbl;
    This->ObjectHeader->ObjectType = (PVOID)&This->Filter;


    /* completed initialization */
    return Status;
}

/*
    @unimplemented
*/
KSDDKAPI
VOID
NTAPI
KsFilterAcquireProcessingMutex(
    IN PKSFILTER Filter)
{
    UNIMPLEMENTED
}

/*
    @unimplemented
*/
KSDDKAPI
VOID
NTAPI
KsFilterReleaseProcessingMutex(
    IN PKSFILTER Filter)
{
    UNIMPLEMENTED
}

/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsFilterAddTopologyConnections (
    IN PKSFILTER Filter,
    IN ULONG NewConnectionsCount,
    IN const KSTOPOLOGY_CONNECTION *const NewTopologyConnections)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

/*
    @unimplemented
*/
KSDDKAPI
VOID
NTAPI
KsFilterAttemptProcessing(
    IN PKSFILTER Filter,
    IN BOOLEAN Asynchronous)
{
    UNIMPLEMENTED
}

/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsFilterCreateNode (
    IN PKSFILTER Filter,
    IN const KSNODE_DESCRIPTOR *const NodeDescriptor,
    OUT PULONG NodeID)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsFilterCreatePinFactory (
    IN PKSFILTER Filter,
    IN const KSPIN_DESCRIPTOR_EX *const PinDescriptor,
    OUT PULONG PinID)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

/*
    @unimplemented
*/
KSDDKAPI
PKSGATE
NTAPI
KsFilterGetAndGate(
    IN PKSFILTER Filter)
{
    UNIMPLEMENTED
    return NULL;
}

/*
    @unimplemented
*/
KSDDKAPI
ULONG
NTAPI
KsFilterGetChildPinCount(
    IN PKSFILTER Filter,
    IN ULONG PinId)
{
    UNIMPLEMENTED
    return 0;
}

/*
    @unimplemented
*/
KSDDKAPI
PKSPIN
NTAPI
KsFilterGetFirstChildPin(
    IN PKSFILTER Filter,
    IN ULONG PinId)
{
    UNIMPLEMENTED
    return NULL;
}

/*
    @unimplemented
*/
KSDDKAPI
VOID
NTAPI
KsFilterRegisterPowerCallbacks(
    IN PKSFILTER Filter,
    IN PFNKSFILTERPOWER Sleep OPTIONAL,
    IN PFNKSFILTERPOWER Wake OPTIONAL)
{
    UNIMPLEMENTED
}

/*
    @unimplemented
*/
KSDDKAPI
PKSFILTER
NTAPI
KsGetFilterFromIrp(
    IN PIRP Irp)
{
    UNIMPLEMENTED
    return NULL;
}

