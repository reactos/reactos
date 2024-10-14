/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/ksfilter/ks/filter.c
 * PURPOSE:         KS IKsFilter interface functions
 * PROGRAMMER:      Johannes Anderwald
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>

typedef struct
{
    KSBASIC_HEADER Header;
    KSFILTER Filter;

    IKsControlVtbl *lpVtblKsControl;
    IKsFilterFactory * FilterFactory;
    IKsProcessingObjectVtbl * lpVtblKsProcessingObject;
    LONG ref;

    PKSIOBJECT_HEADER ObjectHeader;
    KSTOPOLOGY Topology;
    PKSFILTERFACTORY Factory;
    PFILE_OBJECT FileObject;
    KMUTEX ControlMutex;
    KMUTEX ProcessingMutex;

    PKSWORKER Worker;
    WORK_QUEUE_ITEM WorkItem;
    KSGATE Gate;

    PFNKSFILTERPOWER Sleep;
    PFNKSFILTERPOWER Wake;

    ULONG *PinInstanceCount;
    PKSPIN * FirstPin;
    PKSPROCESSPIN_INDEXENTRY ProcessPinIndex;

}IKsFilterImpl;

const GUID IID_IKsControl = {0x28F54685L, 0x06FD, 0x11D2, {0xB2, 0x7A, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};
const GUID IID_IKsFilter  = {0x3ef6ee44L, 0x0D41, 0x11d2, {0xbe, 0xDA, 0x00, 0xc0, 0x4f, 0x8e, 0xF4, 0x57}};
const GUID KSPROPSETID_Topology                = {0x720D4AC0L, 0x7533, 0x11D0, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};
const GUID KSPROPSETID_Pin                     = {0x8C134960L, 0x51AD, 0x11CF, {0x87, 0x8A, 0x94, 0xF8, 0x01, 0xC1, 0x00, 0x00}};
const GUID KSPROPSETID_General =                 {0x1464EDA5L, 0x6A8F, 0x11D1, {0x9A, 0xA7, 0x00, 0xA0, 0xC9, 0x22, 0x31, 0x96}};

VOID
IKsFilter_RemoveFilterFromFilterFactory(
    IKsFilterImpl * This,
    PKSFILTERFACTORY FilterFactory);

NTSTATUS NTAPI FilterTopologyPropertyHandler(IN PIRP Irp, IN PKSIDENTIFIER  Request, IN OUT PVOID  Data);
NTSTATUS NTAPI FilterPinPropertyHandler(IN PIRP Irp, IN PKSIDENTIFIER  Request, IN OUT PVOID  Data);
NTSTATUS NTAPI FilterGeneralComponentIdHandler(IN PIRP Irp, IN PKSIDENTIFIER  Request, IN OUT PVOID  Data);

DEFINE_KSPROPERTY_TOPOLOGYSET(IKsFilterTopologySet, FilterTopologyPropertyHandler);
DEFINE_KSPROPERTY_PINPROPOSEDATAFORMAT(IKsFilterPinSet, FilterPinPropertyHandler, FilterPinPropertyHandler, FilterPinPropertyHandler);
DEFINE_KSPROPERTY_GENEREAL_COMPONENTID(IKsFilterGeneralSet, FilterGeneralComponentIdHandler);

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
    },
    {
        &KSPROPSETID_General,
        sizeof(IKsFilterGeneralSet) / sizeof(KSPROPERTY_ITEM),
        (const KSPROPERTY_ITEM*)&IKsFilterGeneralSet,
        0,
        NULL
    }
};

NTSTATUS
NTAPI
IKsProcessingObject_fnQueryInterface(
    IKsProcessingObject * iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(iface, IKsFilterImpl, lpVtblKsProcessingObject);

    if (IsEqualGUIDAligned(refiid, &IID_IUnknown))
    {
        *Output = &This->Header.OuterUnknown;
        _InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }
    return STATUS_UNSUCCESSFUL;
}

ULONG
NTAPI
IKsProcessingObject_fnAddRef(
    IKsProcessingObject * iface)
{
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(iface, IKsFilterImpl, lpVtblKsProcessingObject);

    return InterlockedIncrement(&This->ref);
}

ULONG
NTAPI
IKsProcessingObject_fnRelease(
    IKsProcessingObject * iface)
{
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(iface, IKsFilterImpl, lpVtblKsProcessingObject);

    InterlockedDecrement(&This->ref);

    /* Return new reference count */
    return This->ref;
}

VOID
NTAPI
IKsProcessingObject_fnProcessingObjectWork(
    IKsProcessingObject * iface)
{
    NTSTATUS Status;
    LARGE_INTEGER TimeOut;
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(iface, IKsFilterImpl, lpVtblKsProcessingObject);

    DPRINT1("processing object\n");
    /* first check if running at passive level */
    if (KeGetCurrentIrql() == PASSIVE_LEVEL)
    {
        /* acquire processing mutex */
        KeWaitForSingleObject(&This->ControlMutex, Executive, KernelMode, FALSE, NULL);
    }
    else
    {
        /* dispatch level processing */
        if (KeReadStateMutex(&This->ControlMutex) == 0)
        {
            /* some thread was faster */
            DPRINT1("processing object too slow\n");
            return;
        }

        /* acquire processing mutex */
        TimeOut.QuadPart = 0LL;
        Status = KeWaitForSingleObject(&This->ControlMutex, Executive, KernelMode, FALSE, &TimeOut);

        if (Status == STATUS_TIMEOUT)
        {
            /* some thread was faster */
            DPRINT1("processing object too slow\n");
            return;
        }
    }

    do
    {

        /* check if the and-gate has been enabled again */
        if (&This->Gate.Count != 0)
        {
            /* gate is open */
            DPRINT1("processing object gate open\n");
            break;
        }

        DPRINT1("IKsProcessingObject_fnProcessingObjectWork not implemented\n");
        ASSERT(0);

    }while(TRUE);

    /* release process mutex */
    KeReleaseMutex(&This->ProcessingMutex, FALSE);
}

PKSGATE
NTAPI
IKsProcessingObject_fnGetAndGate(
    IKsProcessingObject * iface)
{
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(iface, IKsFilterImpl, lpVtblKsProcessingObject);

    /* return and gate */
    return &This->Gate;
}

VOID
NTAPI
IKsProcessingObject_fnProcess(
    IKsProcessingObject * iface,
    IN BOOLEAN Asynchronous)
{
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(iface, IKsFilterImpl, lpVtblKsProcessingObject);

    /* should the action be asynchronous */
    if (Asynchronous)
    {
        /* queue work item */
        KsQueueWorkItem(This->Worker, &This->WorkItem);
DPRINT1("queueing\n");
        /* done */
        return;
    }

    /* does the filter require explicit deferred processing */
    if ((This->Filter.Descriptor->Flags & (KSFILTER_FLAG_DISPATCH_LEVEL_PROCESSING | KSFILTER_FLAG_CRITICAL_PROCESSING | KSFILTER_FLAG_HYPERCRITICAL_PROCESSING)) &&
         KeGetCurrentIrql() > PASSIVE_LEVEL)
    {
        /* queue work item */
        KsQueueWorkItem(This->Worker, &This->WorkItem);
DPRINT1("queueing\n");
        /* done */
        return;
    }
DPRINT1("invoke\n");
    /* call worker routine directly */
    iface->lpVtbl->ProcessingObjectWork(iface);
}

VOID
NTAPI
IKsProcessingObject_fnReset(
    IKsProcessingObject * iface)
{
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(iface, IKsFilterImpl, lpVtblKsProcessingObject);

    /* acquire processing mutex */
    KeWaitForSingleObject(&This->ProcessingMutex, Executive, KernelMode, FALSE, NULL);

    /* check if the filter supports dispatch routines */
    if (This->Filter.Descriptor->Dispatch)
    {
        /* has the filter a reset routine */
        if (This->Filter.Descriptor->Dispatch->Reset)
        {
            /* reset filter */
            This->Filter.Descriptor->Dispatch->Reset(&This->Filter);
        }
    }

    /* release process mutex */
    KeReleaseMutex(&This->ProcessingMutex, FALSE);
}

VOID
NTAPI
IKsProcessingObject_fnTriggerNotification(
    IKsProcessingObject * iface)
{

}

static IKsProcessingObjectVtbl vt_IKsProcessingObject =
{
    IKsProcessingObject_fnQueryInterface,
    IKsProcessingObject_fnAddRef,
    IKsProcessingObject_fnRelease,
    IKsProcessingObject_fnProcessingObjectWork,
    IKsProcessingObject_fnGetAndGate,
    IKsProcessingObject_fnProcess,
    IKsProcessingObject_fnReset,
    IKsProcessingObject_fnTriggerNotification
};


//---------------------------------------------------------------------------------------------------------
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
        *Output = &This->Header.OuterUnknown;
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
    NTSTATUS Status;
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(iface, IKsFilterImpl, Header.OuterUnknown);

    if (IsEqualGUIDAligned(refiid, &IID_IUnknown) ||
        IsEqualGUIDAligned(refiid, &IID_IKsFilter))
    {
        *Output = &This->Header.OuterUnknown;
        _InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }
    else if (IsEqualGUIDAligned(refiid, &IID_IKsControl))
    {
        *Output = &This->lpVtblKsControl;
        _InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }

    if (This->Header.ClientAggregate)
    {
         /* using client aggregate */
         Status = This->Header.ClientAggregate->lpVtbl->QueryInterface(This->Header.ClientAggregate, refiid, Output);

         if (NT_SUCCESS(Status))
         {
             /* client aggregate supports interface */
             return Status;
         }
    }

    DPRINT("IKsFilter_fnQueryInterface no interface\n");
    return STATUS_NOT_SUPPORTED;
}

ULONG
NTAPI
IKsFilter_fnAddRef(
    IKsFilter * iface)
{
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(iface, IKsFilterImpl, Header.OuterUnknown);

    return InterlockedIncrement(&This->ref);
}

ULONG
NTAPI
IKsFilter_fnRelease(
    IKsFilter * iface)
{
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(iface, IKsFilterImpl, Header.OuterUnknown);

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
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(iface, IKsFilterImpl, Header.OuterUnknown);

    return &This->Filter;
}

BOOL
NTAPI
IKsFilter_fnDoAllNecessaryPinsExist(
    IKsFilter * iface)
{
    UNIMPLEMENTED;
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
    UNIMPLEMENTED;
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
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
IKsFilter_fnUnbindProcessPinsFromPipeSection(
    IKsFilter * iface,
    IN struct KSPROCESSPIPESECTION *Section)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
IKsFilter_fnAddProcessPin(
    IKsFilter * iface,
    IN PKSPROCESSPIN ProcessPin)
{
    NTSTATUS Status;
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(iface, IKsFilterImpl, Header.OuterUnknown);

    /* first acquire processing mutex */
    KeWaitForSingleObject(&This->ProcessingMutex, Executive, KernelMode, FALSE, NULL);

    /* sanity check */
    ASSERT(This->Filter.Descriptor->PinDescriptorsCount > ProcessPin->Pin->Id);

    /* allocate new process pin array */
    Status = _KsEdit(This->Filter.Bag, (PVOID*)&This->ProcessPinIndex[ProcessPin->Pin->Id].Pins,
                     (This->Filter.Descriptor->PinDescriptorsCount + 1) * sizeof(PVOID),
                     This->Filter.Descriptor->PinDescriptorsCount * sizeof(PVOID),
                     0);

    if (NT_SUCCESS(Status))
    {
        /* store process pin */
        This->ProcessPinIndex[ProcessPin->Pin->Id].Pins[This->ProcessPinIndex[ProcessPin->Pin->Id].Count] = ProcessPin;
        This->ProcessPinIndex[ProcessPin->Pin->Id].Count++;
    }

    /* release process mutex */
    KeReleaseMutex(&This->ProcessingMutex, FALSE);

    return Status;
}

NTSTATUS
NTAPI
IKsFilter_fnRemoveProcessPin(
    IKsFilter * iface,
    IN PKSPROCESSPIN ProcessPin)
{
    ULONG Index;
    ULONG Count;
    PKSPROCESSPIN * Pins;

    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(iface, IKsFilterImpl, Header.OuterUnknown);

    /* first acquire processing mutex */
    KeWaitForSingleObject(&This->ProcessingMutex, Executive, KernelMode, FALSE, NULL);

    /* sanity check */
    ASSERT(ProcessPin->Pin);
    ASSERT(ProcessPin->Pin->Id);

    Count = This->ProcessPinIndex[ProcessPin->Pin->Id].Count;
    Pins =  This->ProcessPinIndex[ProcessPin->Pin->Id].Pins;

    /* search for current process pin */
    for(Index = 0; Index < Count; Index++)
    {
        if (Pins[Index] == ProcessPin)
        {
            RtlMoveMemory(&Pins[Index], &Pins[Index + 1], (Count - (Index + 1)) * sizeof(PVOID));
            break;
        }

    }

    /* decrement pin count */
    This->ProcessPinIndex[ProcessPin->Pin->Id].Count--;

    if (!This->ProcessPinIndex[ProcessPin->Pin->Id].Count)
    {
        /* clear entry object bag will delete it */
       This->ProcessPinIndex[ProcessPin->Pin->Id].Pins = NULL;
    }

    /* release process mutex */
    KeReleaseMutex(&This->ProcessingMutex, FALSE);

    /* done */
    return STATUS_SUCCESS;
}

BOOL
NTAPI
IKsFilter_fnReprepareProcessPipeSection(
    IKsFilter * iface,
    IN struct KSPROCESSPIPESECTION *PipeSection,
    IN PULONG Data)
{
    UNIMPLEMENTED;
    return FALSE;
}

VOID
NTAPI
IKsFilter_fnDeliverResetState(
    IKsFilter * iface,
    IN struct KSPROCESSPIPESECTION *PipeSection,
    IN KSRESET ResetState)
{
    UNIMPLEMENTED;
}

BOOL
NTAPI
IKsFilter_fnIsFrameHolding(
    IKsFilter * iface)
{
    UNIMPLEMENTED;
    return FALSE;
}

VOID
NTAPI
IKsFilter_fnRegisterForCopyCallbacks(
    IKsFilter * iface,
    IKsQueue *Queue,
    BOOL Register)
{
    UNIMPLEMENTED;
}

PKSPROCESSPIN_INDEXENTRY
NTAPI
IKsFilter_fnGetProcessDispatch(
    IKsFilter * iface)
{
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(iface, IKsFilterImpl, Header.OuterUnknown);

    return This->ProcessPinIndex;
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

    /* sanity check */
    ASSERT(IoStack->FileObject != NULL);

    ObjectHeader = (PKSIOBJECT_HEADER)IoStack->FileObject->FsContext2;

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
        CompleteRequest(Irp, IO_NO_INCREMENT);
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
    This = (IKsFilterImpl*)CONTAINING_RECORD(Filter, IKsFilterImpl, Header.OuterUnknown);

    /* does the driver support notifications */
    if (This->Filter.Descriptor && This->Filter.Descriptor->Dispatch && This->Filter.Descriptor->Dispatch->Close)
    {
        /* call driver's filter close function */
        Status = This->Filter.Descriptor->Dispatch->Close(&This->Filter, Irp);
    }

    if (NT_SUCCESS(Status) && Status != STATUS_PENDING)
    {
        /* save the result */
        Irp->IoStatus.Status = Status;
        /* complete irp */
        CompleteRequest(Irp, IO_NO_INCREMENT);

        /* remove our instance from the filter factory */
        IKsFilter_RemoveFilterFromFilterFactory(This, This->Factory);

        /* free object header */
        KsFreeObjectHeader(This->ObjectHeader);
    }
    else
    {
        /* complete and forget */
        Irp->IoStatus.Status = Status;
        /* complete irp */
        CompleteRequest(Irp, IO_NO_INCREMENT);
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

    if (!This->Filter.Descriptor || !This->Filter.Descriptor->PinDescriptorsCount)
    {
        /* no filter / pin descriptor */
        IoStatus->Status = STATUS_NOT_IMPLEMENTED;
        return STATUS_NOT_IMPLEMENTED;
    }

    /* ignore custom structs for now */
    ASSERT(This->Filter.Descriptor->PinDescriptorSize == sizeof(KSPIN_DESCRIPTOR_EX));
    ASSERT(This->Filter.Descriptor->PinDescriptorsCount > Pin->PinId);

    Instances = (KSPIN_CINSTANCES*)Data;
    /* max instance count */
    Instances->PossibleCount = This->Filter.Descriptor->PinDescriptors[Pin->PinId].InstancesPossible;
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

    if (!This->Filter.Descriptor || !This->Filter.Descriptor->PinDescriptorsCount)
    {
        /* no filter / pin descriptor */
        IoStatus->Status = STATUS_NOT_IMPLEMENTED;
        return STATUS_NOT_IMPLEMENTED;
    }

    /* ignore custom structs for now */
    ASSERT(This->Filter.Descriptor->PinDescriptorSize == sizeof(KSPIN_DESCRIPTOR_EX));
    ASSERT(This->Filter.Descriptor->PinDescriptorsCount > Pin->PinId);

    Result = (PULONG)Data;
    *Result = This->Filter.Descriptor->PinDescriptors[Pin->PinId].InstancesNecessary;

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
    PIO_STACK_LOCATION IoStack;
    KSP_PIN * Pin = (KSP_PIN*)Request;

    /* get stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* sanity check */
    ASSERT(DataLength == IoStack->Parameters.DeviceIoControl.OutputBufferLength);

    /* Access parameters */
    MultipleItem = (PKSMULTIPLE_ITEM)(Pin + 1);
    DataRange = (PKSDATARANGE)(MultipleItem + 1);

    /* FIXME make sure its 64 bit aligned */
    ASSERT(((ULONG_PTR)DataRange & 0x7) == 0);

    if (!This->Filter.Descriptor || !This->Filter.Descriptor->PinDescriptorsCount)
    {
        /* no filter / pin descriptor */
        IoStatus->Status = STATUS_NOT_IMPLEMENTED;
        return STATUS_NOT_IMPLEMENTED;
    }

    /* ignore custom structs for now */
    ASSERT(This->Filter.Descriptor->PinDescriptorSize == sizeof(KSPIN_DESCRIPTOR_EX));
    ASSERT(This->Filter.Descriptor->PinDescriptorsCount > Pin->PinId);

    if (This->Filter.Descriptor->PinDescriptors[Pin->PinId].IntersectHandler == NULL ||
        This->Filter.Descriptor->PinDescriptors[Pin->PinId].PinDescriptor.DataRanges == NULL ||
        This->Filter.Descriptor->PinDescriptors[Pin->PinId].PinDescriptor.DataRangesCount == 0)
    {
        /* no driver supported intersect handler / no provided data ranges */
        IoStatus->Status = STATUS_NOT_IMPLEMENTED;
        return STATUS_NOT_IMPLEMENTED;
    }

    for(Index = 0; Index < MultipleItem->Count; Index++)
    {
        UNICODE_STRING MajorFormat, SubFormat, Specifier;
        /* convert the guid to string */
        RtlStringFromGUID(&DataRange->MajorFormat, &MajorFormat);
        RtlStringFromGUID(&DataRange->SubFormat, &SubFormat);
        RtlStringFromGUID(&DataRange->Specifier, &Specifier);

        DPRINT("KspHandleDataIntersection Index %lu PinId %lu MajorFormat %S SubFormat %S Specifier %S FormatSize %lu SampleSize %lu Align %lu Flags %lx Reserved %lx DataLength %lu\n", Index, Pin->PinId, MajorFormat.Buffer, SubFormat.Buffer, Specifier.Buffer,
               DataRange->FormatSize, DataRange->SampleSize, DataRange->Alignment, DataRange->Flags, DataRange->Reserved, DataLength);

        /* FIXME implement KsPinDataIntersectionEx */
        /* Call miniport's proprietary handler */
        Status = This->Filter.Descriptor->PinDescriptors[Pin->PinId].IntersectHandler(&This->Filter,
                                                                                      Irp,
                                                                                      Pin,
                                                                                      DataRange,
                                                                                      This->Filter.Descriptor->PinDescriptors[Pin->PinId].PinDescriptor.DataRanges[0], /* HACK */
                                                                                      DataLength,
                                                                                      Data,
                                                                                      &Length);
        DPRINT("KspHandleDataIntersection Status %lx\n", Status);

        if (Status == STATUS_SUCCESS || Status == STATUS_BUFFER_OVERFLOW || Status == STATUS_BUFFER_TOO_SMALL)
        {
            ASSERT(Length);
            IoStatus->Information = Length;
            break;
        }

        DataRange = (PKSDATARANGE)((PUCHAR)DataRange + DataRange->FormatSize);
        /* FIXME make sure its 64 bit aligned */
        ASSERT(((ULONG_PTR)DataRange & 0x7) == 0);
    }
    IoStatus->Status = Status;
    return Status;
}

NTSTATUS
NTAPI
FilterTopologyPropertyHandler(
    IN PIRP Irp,
    IN PKSIDENTIFIER  Request,
    IN OUT PVOID  Data)
{
    IKsFilterImpl * This;

    /* get filter implementation */
    This = (IKsFilterImpl*)KSPROPERTY_ITEM_IRP_STORAGE(Irp);

    /* sanity check */
    ASSERT(This);

    return KsTopologyPropertyHandler(Irp, Request, Data, &This->Topology);

}

NTSTATUS
NTAPI
FilterGeneralComponentIdHandler(
    IN PIRP Irp,
    IN PKSIDENTIFIER  Request,
    IN OUT PVOID  Data)
{
    PIO_STACK_LOCATION IoStack;
    IKsFilterImpl * This;

    /* get filter implementation */
    This = (IKsFilterImpl*)KSPROPERTY_ITEM_IRP_STORAGE(Irp);

    /* sanity check */
    ASSERT(This);

    /* get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);
    ASSERT(IoStack->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(KSCOMPONENTID));

    if (This->Filter.Descriptor->ComponentId != NULL)
    {
        RtlMoveMemory(Data, This->Filter.Descriptor->ComponentId, sizeof(KSCOMPONENTID));
        Irp->IoStatus.Information = sizeof(KSCOMPONENTID);
        return STATUS_SUCCESS;
    }
    else
    {
        /* not valid */
        return STATUS_NOT_FOUND;
    }

}

NTSTATUS
NTAPI
FilterPinPropertyHandler(
    IN PIRP Irp,
    IN PKSIDENTIFIER  Request,
    IN OUT PVOID  Data)
{
    PIO_STACK_LOCATION IoStack;
    IKsFilterImpl * This;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    /* get filter implementation */
    This = (IKsFilterImpl*)KSPROPERTY_ITEM_IRP_STORAGE(Irp);

    /* sanity check */
    ASSERT(This);

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
        case KSPROPERTY_PIN_CONSTRAINEDDATARANGES:
            Status = KspPinPropertyHandler(Irp, Request, Data, This->Filter.Descriptor->PinDescriptorsCount, (const KSPIN_DESCRIPTOR*)This->Filter.Descriptor->PinDescriptors, This->Filter.Descriptor->PinDescriptorSize);
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
        default:
            UNIMPLEMENTED;
            Status = STATUS_NOT_FOUND;
    }
    DPRINT("KspPinPropertyHandler Pins %lu Request->Id %lu Status %lx\n", This->Filter.Descriptor->PinDescriptorsCount, Request->Id, Status);


    return Status;
}

NTSTATUS
NTAPI
IKsFilter_DispatchDeviceIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    IKsFilter * Filter;
    IKsFilterImpl * This;
    NTSTATUS Status;
    PKSFILTER FilterInstance;
    UNICODE_STRING GuidString;
    PKSPROPERTY Property;
    ULONG SetCount = 0;
    PKSP_NODE NodeProperty;
    PKSNODE_DESCRIPTOR NodeDescriptor;

    /* obtain filter from object header */
    Status = IKsFilter_GetFilterFromIrp(Irp, &Filter);
    if (!NT_SUCCESS(Status))
        return Status;

    /* get our real implementation */
    This = (IKsFilterImpl*)CONTAINING_RECORD(Filter, IKsFilterImpl, Header.OuterUnknown);

    /* current irp stack */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* get property from input buffer */
    Property = (PKSPROPERTY)IoStack->Parameters.DeviceIoControl.Type3InputBuffer;

    /* get filter instance */
    FilterInstance = Filter->lpVtbl->GetStruct(Filter);

    /* sanity check */
    ASSERT(IoStack->Parameters.DeviceIoControl.InputBufferLength >= sizeof(KSIDENTIFIER));
    ASSERT(FilterInstance);
    ASSERT(FilterInstance->Descriptor);
    ASSERT(FilterInstance->Descriptor->AutomationTable);

    /* acquire control mutex */
    KeWaitForSingleObject(This->Header.ControlMutex, Executive, KernelMode, FALSE, NULL);

    if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_KS_METHOD)
    {
        const KSMETHOD_SET *MethodSet = NULL;
        ULONG MethodItemSize = 0;

        /* check if the driver supports method sets */
        if (FilterInstance->Descriptor->AutomationTable->MethodSetsCount)
        {
            SetCount = FilterInstance->Descriptor->AutomationTable->MethodSetsCount;
            MethodSet = FilterInstance->Descriptor->AutomationTable->MethodSets;
            MethodItemSize = FilterInstance->Descriptor->AutomationTable->MethodItemSize;
        }

        /* call method set handler */
        Status = KspMethodHandlerWithAllocator(Irp, SetCount, MethodSet, NULL, MethodItemSize);
    }
    else if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_KS_PROPERTY)
    {
        const KSPROPERTY_SET *PropertySet = NULL;
        ULONG PropertyItemSize = 0;

        /* check if the driver supports method sets */
        if (Property->Flags & KSPROPERTY_TYPE_TOPOLOGY)
        {
            ASSERT(IoStack->Parameters.DeviceIoControl.InputBufferLength >= sizeof(KSP_NODE));
            NodeProperty = (PKSP_NODE)Property;
            NodeDescriptor = (PKSNODE_DESCRIPTOR)((ULONG_PTR)FilterInstance->Descriptor->NodeDescriptors + FilterInstance->Descriptor->NodeDescriptorSize * NodeProperty->NodeId);
            if (NodeDescriptor->AutomationTable != NULL)
            {
                SetCount = NodeDescriptor->AutomationTable->PropertySetsCount;
                PropertySet = NodeDescriptor->AutomationTable->PropertySets;
                PropertyItemSize = 0;
            }
        }
        else if (FilterInstance->Descriptor->AutomationTable->PropertySetsCount)
        {
            SetCount = FilterInstance->Descriptor->AutomationTable->PropertySetsCount;
            PropertySet = FilterInstance->Descriptor->AutomationTable->PropertySets;
            PropertyItemSize = FilterInstance->Descriptor->AutomationTable->PropertyItemSize;
            // FIXME: handle variable sized property items
            ASSERT(PropertyItemSize == sizeof(KSPROPERTY_ITEM));
            PropertyItemSize = 0;
        }

        /* needed for our property handlers */
        KSPROPERTY_ITEM_IRP_STORAGE(Irp) = (KSPROPERTY_ITEM*)This;

        /* call property handler */
        Status = KspPropertyHandler(Irp, SetCount, PropertySet, NULL, PropertyItemSize);
    }
    else
    {
        /* sanity check */
        ASSERT(IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_KS_ENABLE_EVENT ||
               IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_KS_DISABLE_EVENT);

        if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_KS_ENABLE_EVENT)
        {
            /* call enable event handlers */
            Status = KspEnableEvent(Irp,
                                    FilterInstance->Descriptor->AutomationTable->EventSetsCount,
                                    (PKSEVENT_SET)FilterInstance->Descriptor->AutomationTable->EventSets,
                                    &This->Header.EventList,
                                    KSEVENTS_SPINLOCK,
                                    (PVOID)&This->Header.EventListLock,
                                    NULL,
                                    FilterInstance->Descriptor->AutomationTable->EventItemSize);
        }
        else
        {
            /* disable event handler */
            Status = KsDisableEvent(Irp, &This->Header.EventList, KSEVENTS_SPINLOCK, &This->Header.EventListLock);
        }
    }

    RtlStringFromGUID(&Property->Set, &GuidString);
    DPRINT("IKsFilter_DispatchDeviceIoControl property PinCount %x\n", FilterInstance->Descriptor->PinDescriptorsCount);
    DPRINT("IKsFilter_DispatchDeviceIoControl property Set |%S| Id %u Flags %x Status %lx ResultLength %lu\n", GuidString.Buffer, Property->Id, Property->Flags, Status, Irp->IoStatus.Information);
    RtlFreeUnicodeString(&GuidString);

    /* release filter */
    Filter->lpVtbl->Release(Filter);

    /* release control mutex */
    KeReleaseMutex(This->Header.ControlMutex, FALSE);

    if (Status != STATUS_PENDING)
    {
        Irp->IoStatus.Status = Status;
        CompleteRequest(Irp, IO_NO_INCREMENT);
    }

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
    NTSTATUS Status;
    PKSNODE_DESCRIPTOR NodeDescriptor;

    /* initialize pin descriptors */
    This->FirstPin = NULL;
    This->PinInstanceCount = NULL;
    This->ProcessPinIndex = NULL;

    /* initialize topology descriptor */
    This->Topology.CategoriesCount = FilterDescriptor->CategoriesCount;
    This->Topology.Categories = FilterDescriptor->Categories;
    This->Topology.TopologyNodesCount = FilterDescriptor->NodeDescriptorsCount;
    This->Topology.TopologyConnectionsCount = FilterDescriptor->ConnectionsCount;
    This->Topology.TopologyConnections = FilterDescriptor->Connections;

    /* are there any templates */
    if (FilterDescriptor->PinDescriptorsCount)
    {
        /* sanity check */
        ASSERT(FilterDescriptor->PinDescriptors);

        /* FIXME handle variable sized pin descriptors */
        ASSERT(FilterDescriptor->PinDescriptorSize == sizeof(KSPIN_DESCRIPTOR_EX));

        /* store pin descriptors ex */
        Status = _KsEdit(This->Filter.Bag, (PVOID*)&This->Filter.Descriptor->PinDescriptors, FilterDescriptor->PinDescriptorSize * FilterDescriptor->PinDescriptorsCount,
                         FilterDescriptor->PinDescriptorSize * FilterDescriptor->PinDescriptorsCount, 0);

        if (!NT_SUCCESS(Status))
        {
            DPRINT("IKsFilter_CreateDescriptors _KsEdit failed %lx\n", Status);
            return Status;
        }

        /* store pin instance count */
        Status = _KsEdit(This->Filter.Bag, (PVOID*)&This->PinInstanceCount, sizeof(ULONG) * FilterDescriptor->PinDescriptorsCount,
                         sizeof(ULONG) * FilterDescriptor->PinDescriptorsCount, 0);

        if (!NT_SUCCESS(Status))
        {
            DPRINT("IKsFilter_CreateDescriptors _KsEdit failed %lx\n", Status);
            return Status;
        }

        /* store instantiated pin arrays */
        Status = _KsEdit(This->Filter.Bag, (PVOID*)&This->FirstPin, sizeof(PVOID) * FilterDescriptor->PinDescriptorsCount,
                         sizeof(PVOID) * FilterDescriptor->PinDescriptorsCount, 0);

        if (!NT_SUCCESS(Status))
        {
            DPRINT("IKsFilter_CreateDescriptors _KsEdit failed %lx\n", Status);
            return Status;
        }

        /* add new pin factory */
        RtlMoveMemory((PVOID)This->Filter.Descriptor->PinDescriptors, FilterDescriptor->PinDescriptors, FilterDescriptor->PinDescriptorSize * FilterDescriptor->PinDescriptorsCount);

        /* allocate process pin index */
        Status = _KsEdit(This->Filter.Bag, (PVOID*)&This->ProcessPinIndex, sizeof(KSPROCESSPIN_INDEXENTRY) * FilterDescriptor->PinDescriptorsCount,
                         sizeof(KSPROCESSPIN_INDEXENTRY) * FilterDescriptor->PinDescriptorsCount, 0);

        if (!NT_SUCCESS(Status))
        {
            DPRINT("IKsFilter_CreateDescriptors _KsEdit failed %lx\n", Status);
            return Status;
        }

    }


    if (FilterDescriptor->ConnectionsCount)
    {
        /* modify connections array */
        Status = _KsEdit(This->Filter.Bag,
                        (PVOID*)&This->Filter.Descriptor->Connections,
                         FilterDescriptor->ConnectionsCount * sizeof(KSTOPOLOGY_CONNECTION),
                         FilterDescriptor->ConnectionsCount * sizeof(KSTOPOLOGY_CONNECTION),
                         0);

       This->Topology.TopologyConnections = This->Filter.Descriptor->Connections;
       This->Topology.TopologyConnectionsCount = ((PKSFILTER_DESCRIPTOR)This->Filter.Descriptor)->ConnectionsCount = FilterDescriptor->ConnectionsCount;
    }

    if (FilterDescriptor->NodeDescriptorsCount)
    {
        /* sanity check */
        ASSERT(FilterDescriptor->NodeDescriptors);

        /* sanity check */
        ASSERT(FilterDescriptor->NodeDescriptorSize >= sizeof(KSNODE_DESCRIPTOR));

        This->Topology.TopologyNodes = AllocateItem(NonPagedPool, sizeof(GUID) * FilterDescriptor->NodeDescriptorsCount);
        /* allocate topology node types array */
        if (!This->Topology.TopologyNodes)
        {
            DPRINT("IKsFilter_CreateDescriptors OutOfMemory TopologyNodesCount %lu\n", FilterDescriptor->NodeDescriptorsCount);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        This->Topology.TopologyNodesNames = AllocateItem(NonPagedPool, sizeof(GUID) * FilterDescriptor->NodeDescriptorsCount);
        /* allocate topology names array */
        if (!This->Topology.TopologyNodesNames)
        {
            FreeItem((PVOID)This->Topology.TopologyNodes);
            DPRINT("IKsFilter_CreateDescriptors OutOfMemory TopologyNodesCount %lu\n", FilterDescriptor->NodeDescriptorsCount);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        DPRINT("NodeDescriptorCount %lu\n", FilterDescriptor->NodeDescriptorsCount);
        NodeDescriptor = (PKSNODE_DESCRIPTOR)FilterDescriptor->NodeDescriptors;
        for(Index = 0; Index < FilterDescriptor->NodeDescriptorsCount; Index++)
        {
            DPRINT("Index %lu Type %p Name %p\n", Index, NodeDescriptor->Type, NodeDescriptor->Name);

            /* copy topology type */
            if (NodeDescriptor->Type)
                RtlMoveMemory((PVOID)&This->Topology.TopologyNodes[Index], NodeDescriptor->Type, sizeof(GUID));

            /* copy topology name */
            if (NodeDescriptor->Name)
                RtlMoveMemory((PVOID)&This->Topology.TopologyNodesNames[Index], NodeDescriptor->Name, sizeof(GUID));

            // next node descriptor
            NodeDescriptor = (PKSNODE_DESCRIPTOR)((ULONG_PTR)NodeDescriptor + FilterDescriptor->NodeDescriptorSize);
        }
    }
    /* done! */
    return STATUS_SUCCESS;
}

NTSTATUS
IKsFilter_CopyFilterDescriptor(
    IKsFilterImpl * This,
    const KSFILTER_DESCRIPTOR* FilterDescriptor)
{
    NTSTATUS Status;
    KSAUTOMATION_TABLE AutomationTable;

    This->Filter.Descriptor = AllocateItem(NonPagedPool, sizeof(KSFILTER_DESCRIPTOR));
    if (!This->Filter.Descriptor)
        return STATUS_INSUFFICIENT_RESOURCES;

    Status = KsAddItemToObjectBag(This->Filter.Bag, (PVOID)This->Filter.Descriptor, NULL);
    if (!NT_SUCCESS(Status))
    {
        FreeItem((PVOID)This->Filter.Descriptor);
        This->Filter.Descriptor = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* copy filter descriptor fields */
    RtlMoveMemory((PVOID)This->Filter.Descriptor, FilterDescriptor, sizeof(KSFILTER_DESCRIPTOR));

    /* zero automation table */
    RtlZeroMemory(&AutomationTable, sizeof(KSAUTOMATION_TABLE));

    /* setup filter property sets */
    AutomationTable.PropertyItemSize = sizeof(KSPROPERTY_ITEM);
    AutomationTable.PropertySetsCount = 3;
    AutomationTable.PropertySets = FilterPropertySet;

    /* merge filter automation table */
    Status = KsMergeAutomationTables((PKSAUTOMATION_TABLE*)&This->Filter.Descriptor->AutomationTable, (PKSAUTOMATION_TABLE)FilterDescriptor->AutomationTable, &AutomationTable, This->Filter.Bag);

    return Status;
}


VOID
IKsFilter_AddPin(
    PKSFILTER Filter,
    PKSPIN Pin)
{
    PKSPIN NextPin, CurPin;
    PKSBASIC_HEADER BasicHeader;
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(Filter, IKsFilterImpl, Filter);

    /* sanity check */
    ASSERT(Pin->Id < This->Filter.Descriptor->PinDescriptorsCount);

    if (This->FirstPin[Pin->Id] == NULL)
    {
        /* welcome first pin */
        This->FirstPin[Pin->Id] = Pin;
        This->PinInstanceCount[Pin->Id]++;
        return;
    }

    /* get first pin */
    CurPin = This->FirstPin[Pin->Id];

    do
    {
        /* get next instantiated pin */
        NextPin = KsPinGetNextSiblingPin(CurPin);
        if (!NextPin)
            break;

        NextPin = CurPin;

    }while(NextPin != NULL);

    /* get basic header */
    BasicHeader = (PKSBASIC_HEADER)((ULONG_PTR)CurPin - sizeof(KSBASIC_HEADER));

    /* store pin */
    BasicHeader->Next.Pin = Pin;
}

VOID
IKsFilter_RemovePin(
    PKSFILTER Filter,
    PKSPIN Pin)
{
    PKSPIN NextPin, CurPin, LastPin;
    PKSBASIC_HEADER BasicHeader;
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(Filter, IKsFilterImpl, Filter);

    /* sanity check */
    ASSERT(Pin->Id < This->Filter.Descriptor->PinDescriptorsCount);

    /* get first pin */
    CurPin = This->FirstPin[Pin->Id];

    LastPin = NULL;
    do
    {
        /* get next instantiated pin */
        NextPin = KsPinGetNextSiblingPin(CurPin);

        if (CurPin == Pin)
        {
            if (LastPin)
            {
                /* get basic header of last pin */
                BasicHeader = (PKSBASIC_HEADER)((ULONG_PTR)LastPin - sizeof(KSBASIC_HEADER));

                BasicHeader->Next.Pin = NextPin;
            }
            else
            {
                /* erase last pin */
                This->FirstPin[Pin->Id] = NextPin;
            }
            /* decrement pin instance count */
            This->PinInstanceCount[Pin->Id]--;
            return;
        }

        if (!NextPin)
            break;

        LastPin = CurPin;
        NextPin = CurPin;

    }while(NextPin != NULL);

    /* pin not found */
    ASSERT(0);
}


NTSTATUS
NTAPI
IKsFilter_DispatchCreatePin(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    IKsFilterImpl * This;
    PKSOBJECT_CREATE_ITEM CreateItem;
    PKSPIN_CONNECT Connect;
    NTSTATUS Status;

    DPRINT("IKsFilter_DispatchCreatePin\n");

    /* get the create item */
    CreateItem = KSCREATE_ITEM_IRP_STORAGE(Irp);

    /* get the filter object */
    This = (IKsFilterImpl*)CreateItem->Context;

    /* sanity check */
    ASSERT(This->Header.Type == KsObjectTypeFilter);

    /* acquire control mutex */
    KeWaitForSingleObject(This->Header.ControlMutex, Executive, KernelMode, FALSE, NULL);

    /* now validate the connect request */
    Status = KspValidateConnectRequest(Irp, This->Filter.Descriptor->PinDescriptorsCount, (PVOID)This->Filter.Descriptor->PinDescriptors, This->Filter.Descriptor->PinDescriptorSize, &Connect);

    DPRINT("IKsFilter_DispatchCreatePin KsValidateConnectRequest %lx\n", Status);

    if (NT_SUCCESS(Status))
    {
        /* sanity check */
        ASSERT(Connect->PinId < This->Filter.Descriptor->PinDescriptorsCount);

        DPRINT("IKsFilter_DispatchCreatePin KsValidateConnectRequest PinId %lu CurrentInstanceCount %lu MaxPossible %lu\n", Connect->PinId,
               This->PinInstanceCount[Connect->PinId],
               This->Filter.Descriptor->PinDescriptors[Connect->PinId].InstancesPossible);

        if (This->PinInstanceCount[Connect->PinId] < This->Filter.Descriptor->PinDescriptors[Connect->PinId].InstancesPossible)
        {
            /* create the pin */
            Status = KspCreatePin(DeviceObject, Irp, This->Header.KsDevice, This->FilterFactory, (IKsFilter*)&This->Header.OuterUnknown, Connect, (KSPIN_DESCRIPTOR_EX*)&This->Filter.Descriptor->PinDescriptors[Connect->PinId]);

            DPRINT("IKsFilter_DispatchCreatePin  KspCreatePin %lx\n", Status);
        }
        else
        {
            /* maximum instance count reached, bye-bye */
            Status = STATUS_UNSUCCESSFUL;
            DPRINT("IKsFilter_DispatchCreatePin  MaxInstance %lu CurInstance %lu %lx\n", This->Filter.Descriptor->PinDescriptors[Connect->PinId].InstancesPossible, This->PinInstanceCount[Connect->PinId]);
        }
    }

    /* release control mutex */
    KeReleaseMutex(This->Header.ControlMutex, FALSE);

    if (Status != STATUS_PENDING)
    {
        /* complete request */
        Irp->IoStatus.Status = Status;
        CompleteRequest(Irp, IO_NO_INCREMENT);
    }

    /* done */
    DPRINT("IKsFilter_DispatchCreatePin Result %lx\n", Status);
    return Status;
}

NTSTATUS
NTAPI
IKsFilter_DispatchCreateNode(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    UNIMPLEMENTED;
    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
    CompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_UNSUCCESSFUL;
}


VOID
IKsFilter_AttachFilterToFilterFactory(
    IKsFilterImpl * This,
    PKSFILTERFACTORY FilterFactory)
{
    PKSBASIC_HEADER BasicHeader;
    PKSFILTER Filter;


    /* get filter factory basic header */
    BasicHeader = (PKSBASIC_HEADER)((ULONG_PTR)FilterFactory - sizeof(KSBASIC_HEADER));

    /* sanity check */
    ASSERT(BasicHeader->Type == KsObjectTypeFilterFactory);

    if (BasicHeader->FirstChild.FilterFactory == NULL)
    {
        /* welcome first instantiated filter */
        BasicHeader->FirstChild.Filter = &This->Filter;
        return;
    }

    /* set to first entry */
    Filter = BasicHeader->FirstChild.Filter;

    do
    {
        /* get basic header */
        BasicHeader = (PKSBASIC_HEADER)((ULONG_PTR)Filter - sizeof(KSBASIC_HEADER));
        /* sanity check */
        ASSERT(BasicHeader->Type == KsObjectTypeFilter);

        if (BasicHeader->Next.Filter)
        {
            /* iterate to next filter factory */
            Filter = BasicHeader->Next.Filter;
        }
        else
        {
            /* found last entry */
            break;
        }
    }while(TRUE);

    /* attach filter factory */
    BasicHeader->Next.Filter = &This->Filter;
}

VOID
IKsFilter_RemoveFilterFromFilterFactory(
    IKsFilterImpl * This,
    PKSFILTERFACTORY FilterFactory)
{
    PKSBASIC_HEADER BasicHeader;
    PKSFILTER Filter, LastFilter;

    /* get filter factory basic header */
    BasicHeader = (PKSBASIC_HEADER)((ULONG_PTR)FilterFactory - sizeof(KSBASIC_HEADER));

    /* sanity check */
    ASSERT(BasicHeader->Type == KsObjectTypeFilterFactory);
    ASSERT(BasicHeader->FirstChild.Filter != NULL);


    /* set to first entry */
    Filter = BasicHeader->FirstChild.Filter;
    LastFilter = NULL;

    do
    {
         if (Filter == &This->Filter)
         {
             if (LastFilter)
             {
                 /* get basic header */
                 BasicHeader = (PKSBASIC_HEADER)((ULONG_PTR)LastFilter - sizeof(KSBASIC_HEADER));
                 /* remove filter instance */
                 BasicHeader->Next.Filter = This->Header.Next.Filter;
                 break;
             }
             else
             {
                 /* remove filter instance */
                 BasicHeader->FirstChild.Filter = This->Header.Next.Filter;
                 break;
             }
         }

        /* get basic header */
        BasicHeader = (PKSBASIC_HEADER)((ULONG_PTR)Filter - sizeof(KSBASIC_HEADER));
        /* sanity check */
        ASSERT(BasicHeader->Type == KsObjectTypeFilter);

        LastFilter = Filter;
        if (BasicHeader->Next.Filter)
        {
            /* iterate to next filter factory */
            Filter = BasicHeader->Next.Filter;
        }
        else
        {
            /* filter is not in list */
            ASSERT(0);
            break;
        }
    }while(TRUE);
}

VOID
NTAPI
IKsFilter_FilterCentricWorker(
    IN PVOID Ctx)
{
    IKsProcessingObject * Object = (IKsProcessingObject*)Ctx;

    /* sanity check */
    ASSERT(Object);

    /* perform work */
    Object->lpVtbl->ProcessingObjectWork(Object);
}

NTSTATUS
NTAPI
KspCreateFilter(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN IKsFilterFactory *iface)
{
    IKsFilterImpl * This;
    IKsDevice *KsDevice;
    PKSFILTERFACTORY Factory;
    PIO_STACK_LOCATION IoStack;
    PDEVICE_EXTENSION DeviceExtension;
    NTSTATUS Status;
    PKSOBJECT_CREATE_ITEM CreateItem;

    /* get device extension */
    DeviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;

    /* get the filter factory */
    Factory = iface->lpVtbl->GetStruct(iface);

    if (!Factory || !Factory->FilterDescriptor)
    {
        /* Sorry it just will not work */
        return STATUS_UNSUCCESSFUL;
    }

    if (Factory->FilterDescriptor->Flags & KSFILTER_FLAG_DENY_USERMODE_ACCESS)
    {
        if (Irp->RequestorMode == UserMode)
        {
            /* filter not accessible from user mode */
            DPRINT1("Access denied\n");
            return STATUS_UNSUCCESSFUL;
        }
    }

    /* allocate filter instance */
    This = AllocateItem(NonPagedPool, sizeof(IKsFilterImpl));
    if (!This)
    {
        DPRINT1("KspCreateFilter OutOfMemory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* initialize object bag */
    This->Filter.Bag = AllocateItem(NonPagedPool, sizeof(KSIOBJECT_BAG));
    if (!This->Filter.Bag)
    {
        /* no memory */
        FreeItem(This);
        DPRINT1("KspCreateFilter OutOfMemory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    KsDevice = (IKsDevice*)&DeviceExtension->DeviceHeader->BasicHeader.OuterUnknown;
    KsDevice->lpVtbl->InitializeObjectBag(KsDevice, (PKSIOBJECT_BAG)This->Filter.Bag, NULL);

    /* copy filter descriptor */
    Status = IKsFilter_CopyFilterDescriptor(This, Factory->FilterDescriptor);
    if (!NT_SUCCESS(Status))
    {
        /* not enough memory */
        FreeItem(This->Filter.Bag);
        FreeItem(This);
        DPRINT("KspCreateFilter IKsFilter_CopyFilterDescriptor failed %lx\n", Status);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* get current irp stack */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* allocate create items */
    CreateItem = AllocateItem(NonPagedPool, sizeof(KSOBJECT_CREATE_ITEM) * 2);
    if (!CreateItem)
    {
        /* no memory */
        FreeItem(This->Filter.Bag);
        FreeItem(This);
        DPRINT1("KspCreateFilter OutOfMemory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* initialize pin create item */
    CreateItem[0].Create = IKsFilter_DispatchCreatePin;
    CreateItem[0].Context = (PVOID)This;
    CreateItem[0].Flags = KSCREATE_ITEM_FREEONSTOP;
    RtlInitUnicodeString(&CreateItem[0].ObjectClass, KSSTRING_Pin);
    /* initialize node create item */
    CreateItem[1].Create = IKsFilter_DispatchCreateNode;
    CreateItem[1].Context = (PVOID)This;
    CreateItem[1].Flags = KSCREATE_ITEM_FREEONSTOP;
    RtlInitUnicodeString(&CreateItem[1].ObjectClass, KSSTRING_TopologyNode);


    /* initialize filter instance */
    This->ref = 1;
    This->Header.OuterUnknown = (PUNKNOWN)&vt_IKsFilter;
    This->lpVtblKsControl = &vt_IKsControl;
    This->lpVtblKsProcessingObject = &vt_IKsProcessingObject;

    This->Factory = Factory;
    This->FilterFactory = iface;
    This->FileObject = IoStack->FileObject;
    KeInitializeMutex(&This->ProcessingMutex, 0);

    /* initialize basic header */
    This->Header.KsDevice = &DeviceExtension->DeviceHeader->KsDevice;
    This->Header.Parent.KsFilterFactory = iface->lpVtbl->GetStruct(iface);
    This->Header.Type = KsObjectTypeFilter;
    This->Header.ControlMutex = &This->ControlMutex;
    KeInitializeMutex(This->Header.ControlMutex, 0);
    InitializeListHead(&This->Header.EventList);
    KeInitializeSpinLock(&This->Header.EventListLock);

    /* initialize and gate */
    KsGateInitializeAnd(&This->Gate, NULL);

    /* FIXME initialize and gate based on pin flags */

    /* initialize work item */
    ExInitializeWorkItem(&This->WorkItem, IKsFilter_FilterCentricWorker, (PVOID)This->lpVtblKsProcessingObject);

    /* allocate counted work item */
    Status = KsRegisterCountedWorker(HyperCriticalWorkQueue, &This->WorkItem, &This->Worker);
    if (!NT_SUCCESS(Status))
    {
        /* what can go wrong, goes wrong */
        DPRINT1("KsRegisterCountedWorker failed with %lx\n", Status);
        FreeItem(This);
        FreeItem(CreateItem);
        return Status;
    }

    /* allocate the stream descriptors */
    Status = IKsFilter_CreateDescriptors(This, (PKSFILTER_DESCRIPTOR)Factory->FilterDescriptor);
    if (!NT_SUCCESS(Status))
    {
        /* what can go wrong, goes wrong */
        DPRINT1("IKsFilter_CreateDescriptors failed with %lx\n", Status);
        KsUnregisterWorker(This->Worker);
        FreeItem(This);
        FreeItem(CreateItem);
        return Status;
    }



    /* does the filter have a filter dispatch */
    if (Factory->FilterDescriptor->Dispatch)
    {
        /* does it have a create routine */
        if (Factory->FilterDescriptor->Dispatch->Create)
        {
            /* now let driver initialize the filter instance */

            ASSERT(This->Header.KsDevice);
            ASSERT(This->Header.KsDevice->Started);
            Status = Factory->FilterDescriptor->Dispatch->Create(&This->Filter, Irp);

            if (!NT_SUCCESS(Status) && Status != STATUS_PENDING)
            {
                /* driver failed to initialize */
                DPRINT1("Driver: Status %x\n", Status);

                /* free filter instance */
                KsUnregisterWorker(This->Worker);
                FreeItem(This);
                FreeItem(CreateItem);
                return Status;
            }
        }
    }

    /* now allocate the object header */
    Status = KsAllocateObjectHeader((PVOID*)&This->ObjectHeader, 2, CreateItem, Irp, &DispatchTable);
    if (!NT_SUCCESS(Status))
    {
        /* failed to allocate object header */
        DPRINT1("Failed to allocate object header %x\n", Status);

        return Status;
    }

    /* initialize object header extra fields */
    This->ObjectHeader->Type = KsObjectTypeFilter;
    This->ObjectHeader->Unknown = (PUNKNOWN)&This->Header.OuterUnknown;
    This->ObjectHeader->ObjectType = (PVOID)&This->Filter;

    /* attach filter to filter factory */
    IKsFilter_AttachFilterToFilterFactory(This, This->Header.Parent.KsFilterFactory);

    /* completed initialization */
    DPRINT1("KspCreateFilter done %lx KsDevice %p\n", Status, This->Header.KsDevice);
    return Status;
}

/*
    @implemented
*/
KSDDKAPI
VOID
NTAPI
KsFilterAcquireProcessingMutex(
    IN PKSFILTER Filter)
{
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(Filter, IKsFilterImpl, Filter);

    KeWaitForSingleObject(&This->ProcessingMutex, Executive, KernelMode, FALSE, NULL);
}

/*
    @implemented
*/
KSDDKAPI
VOID
NTAPI
KsFilterReleaseProcessingMutex(
    IN PKSFILTER Filter)
{
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(Filter, IKsFilterImpl, Filter);

    KeReleaseMutex(&This->ProcessingMutex, FALSE);
}


/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsFilterAddTopologyConnections (
    IN PKSFILTER Filter,
    IN ULONG NewConnectionsCount,
    IN const KSTOPOLOGY_CONNECTION *const NewTopologyConnections)
{
    ULONG Count;
    NTSTATUS Status;
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(Filter, IKsFilterImpl, Filter);

    DPRINT("KsFilterAddTopologyConnections\n");

    ASSERT(This->Filter.Descriptor);
    Count = This->Filter.Descriptor->ConnectionsCount + NewConnectionsCount;


    /* modify connections array */
    Status = _KsEdit(This->Filter.Bag,
                    (PVOID*)&This->Filter.Descriptor->Connections,
                     Count * sizeof(KSTOPOLOGY_CONNECTION),
                     This->Filter.Descriptor->ConnectionsCount * sizeof(KSTOPOLOGY_CONNECTION),
                     0);

    if (!NT_SUCCESS(Status))
    {
        /* failed */
        DPRINT("KsFilterAddTopologyConnections KsEdit failed with %lx\n", Status);
        return Status;
    }

    /* FIXME verify connections */

    /* copy new connections */
    RtlMoveMemory((PVOID)&This->Filter.Descriptor->Connections[This->Filter.Descriptor->ConnectionsCount],
                  NewTopologyConnections,
                  NewConnectionsCount * sizeof(KSTOPOLOGY_CONNECTION));

    /* update topology */
    This->Topology.TopologyConnectionsCount += NewConnectionsCount;
    ((PKSFILTER_DESCRIPTOR)This->Filter.Descriptor)->ConnectionsCount += NewConnectionsCount;
    This->Topology.TopologyConnections = This->Filter.Descriptor->Connections;

    return Status;
}

/*
    @implemented
*/
KSDDKAPI
VOID
NTAPI
KsFilterAttemptProcessing(
    IN PKSFILTER Filter,
    IN BOOLEAN Asynchronous)
{
    PKSGATE Gate;
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(Filter, IKsFilterImpl, Filter);

    /* get gate */
    Gate = This->lpVtblKsProcessingObject->GetAndGate((IKsProcessingObject*)This->lpVtblKsProcessingObject);

    if (!KsGateCaptureThreshold(Gate))
    {
        /* filter control gate is closed */
        return;
    }
DPRINT1("processing\n");
    /* try initiate processing */
    This->lpVtblKsProcessingObject->Process((IKsProcessingObject*)This->lpVtblKsProcessingObject, Asynchronous);
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
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsFilterCreatePinFactory (
    IN PKSFILTER Filter,
    IN const KSPIN_DESCRIPTOR_EX *const InPinDescriptor,
    OUT PULONG PinID)
{
    ULONG Count;
    NTSTATUS Status;
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(Filter, IKsFilterImpl, Filter);

    DPRINT("KsFilterCreatePinFactory\n");

    /* calculate new count */
    Count = This->Filter.Descriptor->PinDescriptorsCount + 1;

    /* sanity check */
    ASSERT(This->Filter.Descriptor->PinDescriptorSize == sizeof(KSPIN_DESCRIPTOR_EX));

    /* modify pin descriptors ex array */
    Status = _KsEdit(This->Filter.Bag, (PVOID*)&This->Filter.Descriptor->PinDescriptors, Count * This->Filter.Descriptor->PinDescriptorSize, This->Filter.Descriptor->PinDescriptorsCount * This->Filter.Descriptor->PinDescriptorSize, 0);
    if (!NT_SUCCESS(Status))
    {
        /* failed */
        DPRINT("KsFilterCreatePinFactory _KsEdit failed with %lx\n", Status);
        return Status;
    }

    /* modify pin instance count array */
    Status = _KsEdit(This->Filter.Bag,(PVOID*)&This->PinInstanceCount, sizeof(ULONG) * Count, sizeof(ULONG) * This->Filter.Descriptor->PinDescriptorsCount, 0);
    if (!NT_SUCCESS(Status))
    {
        /* failed */
        DPRINT("KsFilterCreatePinFactory _KsEdit failed with %lx\n", Status);
        return Status;
    }

    /* modify first pin array */
    Status = _KsEdit(This->Filter.Bag,(PVOID*)&This->FirstPin, sizeof(PVOID) * Count, sizeof(PVOID) * This->Filter.Descriptor->PinDescriptorsCount, 0);
    if (!NT_SUCCESS(Status))
    {
        /* failed */
        DPRINT("KsFilterCreatePinFactory _KsEdit failed with %lx\n", Status);
        return Status;
    }

    /* add new pin factory */
    RtlMoveMemory((PVOID)&This->Filter.Descriptor->PinDescriptors[This->Filter.Descriptor->PinDescriptorsCount], InPinDescriptor, sizeof(KSPIN_DESCRIPTOR_EX));

    /* allocate process pin index */
    Status = _KsEdit(This->Filter.Bag, (PVOID*)&This->ProcessPinIndex, sizeof(KSPROCESSPIN_INDEXENTRY) * Count,
                     sizeof(KSPROCESSPIN_INDEXENTRY) * This->Filter.Descriptor->PinDescriptorsCount, 0);

    if (!NT_SUCCESS(Status))
    {
        DPRINT("KsFilterCreatePinFactory _KsEdit failed %lx\n", Status);
        return Status;
    }

    /* store new pin id */
    *PinID = This->Filter.Descriptor->PinDescriptorsCount;

    /* increment pin descriptor count */
    ((PKSFILTER_DESCRIPTOR)This->Filter.Descriptor)->PinDescriptorsCount++;


    DPRINT("KsFilterCreatePinFactory done\n");
    return STATUS_SUCCESS;

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
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(Filter, IKsFilterImpl, Filter);

    /* return and-gate */
    return &This->Gate;
}

/*
    @implemented
*/
KSDDKAPI
ULONG
NTAPI
KsFilterGetChildPinCount(
    IN PKSFILTER Filter,
    IN ULONG PinId)
{
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(Filter, IKsFilterImpl, Filter);

    if (PinId >= This->Filter.Descriptor->PinDescriptorsCount)
    {
        /* index is out of bounds */
        return 0;
    }
    /* return pin instance count */
    return This->PinInstanceCount[PinId];
}

/*
    @implemented
*/
KSDDKAPI
PKSPIN
NTAPI
KsFilterGetFirstChildPin(
    IN PKSFILTER Filter,
    IN ULONG PinId)
{
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(Filter, IKsFilterImpl, Filter);

    if (PinId >= This->Filter.Descriptor->PinDescriptorsCount)
    {
        /* index is out of bounds */
        return NULL;
    }

    /* return first pin index */
    return This->FirstPin[PinId];
}

/*
    @implemented
*/
KSDDKAPI
VOID
NTAPI
KsFilterRegisterPowerCallbacks(
    IN PKSFILTER Filter,
    IN PFNKSFILTERPOWER Sleep OPTIONAL,
    IN PFNKSFILTERPOWER Wake OPTIONAL)
{
    IKsFilterImpl * This = (IKsFilterImpl*)CONTAINING_RECORD(Filter, IKsFilterImpl, Filter);

    This->Sleep = Sleep;
    This->Wake = Wake;
}

/*
    @implemented
*/
KSDDKAPI
PKSFILTER
NTAPI
KsGetFilterFromIrp(
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PKSIOBJECT_HEADER ObjectHeader;

    DPRINT("KsGetFilterFromIrp\n");

    /* get current irp stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* sanity check */
    ASSERT(IoStack->FileObject);

    /* get object header */
    ObjectHeader = (PKSIOBJECT_HEADER)IoStack->FileObject->FsContext2;

    if (ObjectHeader->Type == KsObjectTypeFilter)
    {
        /* irp is targeted at the filter */
        return (PKSFILTER)ObjectHeader->ObjectType;
    }
    else if (ObjectHeader->Type == KsObjectTypePin)
    {
        /* irp is for a pin */
        return KsPinGetParentFilter((PKSPIN)ObjectHeader->ObjectType);
    }
    else
    {
        /* irp is unappropiate to retrieve a filter */
        return NULL;
    }
}
