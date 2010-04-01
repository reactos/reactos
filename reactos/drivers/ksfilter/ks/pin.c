/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/ksfilter/ks/worker.c
 * PURPOSE:         KS pin functions
 * PROGRAMMER:      Johannes Anderwald
 */


#include "priv.h"

typedef struct _KSISTREAM_POINTER
{
    KSSTREAM_POINTER StreamPointer;
    PFNKSSTREAMPOINTER Callback;
    PIRP Irp;
    KTIMER Timer;
    KDPC TimerDpc;
    struct _KSISTREAM_POINTER *Next;

}KSISTREAM_POINTER, *PKSISTREAM_POINTER;

typedef struct
{
    KSBASIC_HEADER BasicHeader;
    KSPIN Pin;
    PKSIOBJECT_HEADER ObjectHeader;
    KSPROCESSPIN ProcessPin;
    LIST_ENTRY Entry;

    IKsPinVtbl *lpVtbl;

    LONG ref;
    KMUTEX ProcessingMutex;
    PFILE_OBJECT FileObject;

    PKSGATE AttachedGate;
    BOOL OrGate;

    LIST_ENTRY IrpList;
    KSPIN_LOCK IrpListLock;

    PKSISTREAM_POINTER ClonedStreamPointer;
    PKSISTREAM_POINTER LeadingEdgeStreamPointer;
    PKSISTREAM_POINTER TrailingStreamPointer;

    PFNKSPINPOWER  Sleep;
    PFNKSPINPOWER  Wake;
    PFNKSPINHANDSHAKE  Handshake;
    PFNKSPINFRAMERETURN  FrameReturn;
    PFNKSPINIRPCOMPLETION  IrpCompletion;

}IKsPinImpl;

NTSTATUS
NTAPI
IKsPin_fnQueryInterface(
    IKsPin * iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    IKsPinImpl * This = (IKsPinImpl*)CONTAINING_RECORD(iface, IKsPinImpl, lpVtbl);

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
IKsPin_fnAddRef(
    IKsPin * iface)
{
    IKsPinImpl * This = (IKsPinImpl*)CONTAINING_RECORD(iface, IKsPinImpl, lpVtbl);

    return InterlockedIncrement(&This->ref);
}

ULONG
NTAPI
IKsPin_fnRelease(
    IKsPin * iface)
{
    IKsPinImpl * This = (IKsPinImpl*)CONTAINING_RECORD(iface, IKsPinImpl, lpVtbl);

    InterlockedDecrement(&This->ref);

    if (This->ref == 0)
    {
        FreeItem(This);
        return 0;
    }
    /* Return new reference count */
    return This->ref;
}

NTSTATUS
NTAPI
IKsPin_fnTransferKsIrp(
    IN IKsPin *iface,
    IN PIRP Irp,
    IN IKsTransport **OutTransport)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

VOID
NTAPI
IKsPin_fnDiscardKsIrp(
    IN IKsPin *iface,
    IN PIRP Irp,
    IN IKsTransport * *OutTransport)
{
    UNIMPLEMENTED
}


NTSTATUS
NTAPI
IKsPin_fnConnect(
    IN IKsPin *iface,
    IN IKsTransport * TransportIn,
    OUT IKsTransport ** OutTransportIn,
    OUT IKsTransport * *OutTransportOut,
    IN KSPIN_DATAFLOW DataFlow)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
IKsPin_fnSetDeviceState(
    IN IKsPin *iface,
    IN KSSTATE OldState,
    IN KSSTATE NewState,
    IN IKsTransport * *OutTransport)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

VOID
NTAPI
IKsPin_fnSetResetState(
    IN IKsPin *iface,
    IN KSRESET ResetState,
    OUT IKsTransport * * OutTransportOut)
{
    UNIMPLEMENTED
}

NTSTATUS
NTAPI
IKsPin_fnGetTransportConfig(
    IN IKsPin *iface,
    IN struct KSPTRANSPORTCONFIG * TransportConfig,
    OUT IKsTransport ** OutTransportIn,
    OUT IKsTransport ** OutTransportOut)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
IKsPin_fnSetTransportConfig(
    IN IKsPin *iface,
    IN struct KSPTRANSPORTCONFIG const * TransportConfig,
    OUT IKsTransport ** OutTransportIn,
    OUT IKsTransport ** OutTransportOut)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
IKsPin_fnResetTransportConfig(
    IN IKsPin *iface,
    OUT IKsTransport ** OutTransportIn,
    OUT IKsTransport ** OutTransportOut)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

PKSPIN
NTAPI
IKsPin_fnGetStruct(
    IN IKsPin *iface)
{
    UNIMPLEMENTED
    return NULL;
}

PKSPROCESSPIN
NTAPI
IKsPin_fnGetProcessPin(
    IN IKsPin *iface)
{
    UNIMPLEMENTED
    return NULL;
}

NTSTATUS
NTAPI
IKsPin_fnAttemptBypass(
    IN IKsPin *iface)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
IKsPin_fnAttemptUnbypass(
    IN IKsPin *iface)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

VOID
NTAPI
IKsPin_fnGenerateConnectionEvents(
    IN IKsPin *iface,
    IN ULONG EventMask)
{
    UNIMPLEMENTED
}

NTSTATUS
NTAPI
IKsPin_fnClientSetDeviceState(
    IN IKsPin *iface,
    IN KSSTATE StateIn,
    IN KSSTATE StateOut)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

static IKsPinVtbl vt_IKsPin =
{
    IKsPin_fnQueryInterface,
    IKsPin_fnAddRef,
    IKsPin_fnRelease,
    IKsPin_fnTransferKsIrp,
    IKsPin_fnDiscardKsIrp,
    IKsPin_fnConnect,
    IKsPin_fnSetDeviceState,
    IKsPin_fnSetResetState,
    IKsPin_fnGetTransportConfig,
    IKsPin_fnSetTransportConfig,
    IKsPin_fnResetTransportConfig,
    IKsPin_fnGetStruct,
    IKsPin_fnGetProcessPin,
    IKsPin_fnAttemptBypass,
    IKsPin_fnAttemptUnbypass,
    IKsPin_fnGenerateConnectionEvents,
    IKsPin_fnClientSetDeviceState
};


//==============================================================

/*
    @implemented
*/
VOID
NTAPI
KsPinAcquireProcessingMutex(
    IN PKSPIN  Pin)
{
    IKsPinImpl * This = (IKsPinImpl*)CONTAINING_RECORD(Pin, IKsPinImpl, Pin);

    KeWaitForSingleObject(&This->ProcessingMutex, Executive, KernelMode, FALSE, NULL);
}

/*
    @implemented
*/
VOID
NTAPI
KsPinAttachAndGate(
    IN PKSPIN Pin,
    IN PKSGATE AndGate OPTIONAL)
{
    IKsPinImpl * This = (IKsPinImpl*)CONTAINING_RECORD(Pin, IKsPinImpl, Pin);

    /* FIXME attach to filter's and gate (filter-centric processing) */

    This->AttachedGate = AndGate;
    This->OrGate = FALSE;
}

/*
    @implemented
*/
VOID
NTAPI
KsPinAttachOrGate(
    IN PKSPIN Pin,
    IN PKSGATE OrGate OPTIONAL)
{
    IKsPinImpl * This = (IKsPinImpl*)CONTAINING_RECORD(Pin, IKsPinImpl, Pin);

    /* FIXME attach to filter's and gate (filter-centric processing) */

    This->AttachedGate = OrGate;
    This->OrGate = TRUE;
}

/*
    @implemented
*/
PKSGATE
NTAPI
KsPinGetAndGate(
    IN PKSPIN  Pin)
{
    IKsPinImpl * This = (IKsPinImpl*)CONTAINING_RECORD(Pin, IKsPinImpl, Pin);

    return This->AttachedGate;
}

/*
    @unimplemented
*/
VOID
NTAPI
KsPinAttemptProcessing(
    IN PKSPIN  Pin,
    IN BOOLEAN  Asynchronous)
{
    DPRINT("KsPinAttemptProcessing\n");
    UNIMPLEMENTED
}

/*
    @unimplemented
*/
NTSTATUS
NTAPI
KsPinGetAvailableByteCount(
    IN PKSPIN  Pin,
    OUT PLONG  InputDataBytes OPTIONAL,
    OUT PLONG  OutputBufferBytes OPTIONAL)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

/*
    @unimplemented
*/
NTSTATUS
NTAPI
KsPinGetConnectedFilterInterface(
    IN PKSPIN  Pin,
    IN const GUID*  InterfaceId,
    OUT PVOID*  Interface)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

/*
    @unimplemented
*/
PDEVICE_OBJECT
NTAPI
KsPinGetConnectedPinDeviceObject(
    IN PKSPIN Pin)
{
    UNIMPLEMENTED
    return NULL;
}

/*
    @unimplemented
*/
PFILE_OBJECT
NTAPI
KsPinGetConnectedPinFileObject(
    IN PKSPIN Pin)
{
    UNIMPLEMENTED
    return NULL;
}

/*
    @unimplemented
*/
NTSTATUS
NTAPI
KsPinGetConnectedPinInterface(
    IN PKSPIN  Pin,
    IN const GUID*  InterfaceId,
    OUT PVOID*  Interface)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

/*
    @unimplemented
*/
VOID
NTAPI
KsPinGetCopyRelationships(
    IN PKSPIN Pin,
    OUT PKSPIN* CopySource,
    OUT PKSPIN* DelegateBranch)
{
    UNIMPLEMENTED
}

/*
    @implemented
*/
PKSPIN
NTAPI
KsPinGetNextSiblingPin(
    IN PKSPIN  Pin)
{
    return KsGetNextSibling((PVOID)Pin);
}

/*
    @implemented
*/
PKSFILTER
NTAPI
KsPinGetParentFilter(
    IN PKSPIN  Pin)
{
    IKsPinImpl * This = (IKsPinImpl*)CONTAINING_RECORD(Pin, IKsPinImpl, Pin);

    /* return parent filter */
    return This->BasicHeader.Parent.KsFilter;
}

/*
    @unimplemented
*/
NTSTATUS
NTAPI
  KsPinGetReferenceClockInterface(
    IN PKSPIN  Pin,
    OUT PIKSREFERENCECLOCK*  Interface)
{
    UNIMPLEMENTED
    DPRINT("KsPinGetReferenceClockInterface Pin %p Interface %p\n", Pin, Interface);
    return STATUS_UNSUCCESSFUL;
}

/*
    @implemented
*/
VOID
NTAPI
KsPinRegisterFrameReturnCallback(
    IN PKSPIN  Pin,
    IN PFNKSPINFRAMERETURN  FrameReturn)
{
    IKsPinImpl * This = (IKsPinImpl*)CONTAINING_RECORD(Pin, IKsPinImpl, Pin);

    /* register frame return callback */
    This->FrameReturn = FrameReturn;
}

/*
    @implemented
*/
VOID
NTAPI
KsPinRegisterHandshakeCallback(
    IN PKSPIN  Pin,
    IN PFNKSPINHANDSHAKE  Handshake)
{
    IKsPinImpl * This = (IKsPinImpl*)CONTAINING_RECORD(Pin, IKsPinImpl, Pin);

    /* register private protocol handshake callback */
    This->Handshake = Handshake;
}

/*
    @implemented
*/
VOID
NTAPI
KsPinRegisterIrpCompletionCallback(
    IN PKSPIN  Pin,
    IN PFNKSPINIRPCOMPLETION  IrpCompletion)
{
    IKsPinImpl * This = (IKsPinImpl*)CONTAINING_RECORD(Pin, IKsPinImpl, Pin);

    /* register irp completion callback */
    This->IrpCompletion = IrpCompletion;
}

/*
    @implemented
*/
VOID
NTAPI
KsPinRegisterPowerCallbacks(
    IN PKSPIN  Pin,
    IN PFNKSPINPOWER  Sleep OPTIONAL,
    IN PFNKSPINPOWER  Wake OPTIONAL)
{
    IKsPinImpl * This = (IKsPinImpl*)CONTAINING_RECORD(Pin, IKsPinImpl, Pin);

    /* register power callbacks */
    This->Sleep = Sleep;
    This->Wake = Wake;
}

/*
    @implemented
*/
VOID
NTAPI
KsPinReleaseProcessingMutex(
    IN PKSPIN  Pin)
{
    IKsPinImpl * This = (IKsPinImpl*)CONTAINING_RECORD(Pin, IKsPinImpl, Pin);

    /* release processing mutex */
    KeReleaseMutex(&This->ProcessingMutex, FALSE);
}

/*
    @implemented
*/
KSDDKAPI
PKSPIN
NTAPI
KsGetPinFromIrp(
    IN PIRP Irp)
{
    PKSIOBJECT_HEADER ObjectHeader;
    PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("KsGetPinFromIrp\n");

    /* get object header */
    ObjectHeader = (PKSIOBJECT_HEADER)IoStack->FileObject->FsContext2;
    /* return object type */
    return (PKSPIN)ObjectHeader->ObjectType;

}



/*
    @unimplemented
*/
VOID
NTAPI
KsPinSetPinClockTime(
    IN PKSPIN  Pin,
    IN LONGLONG  Time)
{
    UNIMPLEMENTED
}

/*
    @unimplemented
*/
NTSTATUS
NTAPI
KsPinSubmitFrame(
    IN PKSPIN  Pin,
    IN PVOID  Data  OPTIONAL,
    IN ULONG  Size  OPTIONAL,
    IN PKSSTREAM_HEADER  StreamHeader  OPTIONAL,
    IN PVOID  Context  OPTIONAL)
{
    UNIMPLEMENTED
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsPinSubmitFrameMdl(
    IN PKSPIN  Pin,
    IN PMDL  Mdl  OPTIONAL,
    IN PKSSTREAM_HEADER  StreamHeader  OPTIONAL,
    IN PVOID  Context  OPTIONAL)
{
    UNIMPLEMENTED
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI
BOOLEAN
NTAPI
KsProcessPinUpdate(
    IN PKSPROCESSPIN  ProcessPin)
{
    UNIMPLEMENTED
    return FALSE;
}

/*
    @unimplemented
*/
KSDDKAPI
PKSSTREAM_POINTER
NTAPI
KsPinGetLeadingEdgeStreamPointer(
    IN PKSPIN Pin,
    IN KSSTREAM_POINTER_STATE State)
{
    UNIMPLEMENTED
    DPRINT("KsPinGetLeadingEdgeStreamPointer Pin %p State %x\n", Pin, State);
    return NULL;
}

/*
    @unimplemented
*/
KSDDKAPI
PKSSTREAM_POINTER
NTAPI
KsPinGetTrailingEdgeStreamPointer(
    IN PKSPIN Pin,
    IN KSSTREAM_POINTER_STATE State)
{
    UNIMPLEMENTED
    return NULL;
}

/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsStreamPointerSetStatusCode(
    IN PKSSTREAM_POINTER StreamPointer,
    IN NTSTATUS Status)
{
    UNIMPLEMENTED
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsStreamPointerLock(
    IN PKSSTREAM_POINTER StreamPointer)
{
    UNIMPLEMENTED
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI
VOID
NTAPI
KsStreamPointerUnlock(
    IN PKSSTREAM_POINTER StreamPointer,
    IN BOOLEAN Eject)
{
    UNIMPLEMENTED
    DPRINT("KsStreamPointerUnlock\n");
}

/*
    @unimplemented
*/
KSDDKAPI
VOID
NTAPI
KsStreamPointerAdvanceOffsetsAndUnlock(
    IN PKSSTREAM_POINTER StreamPointer,
    IN ULONG InUsed,
    IN ULONG OutUsed,
    IN BOOLEAN Eject)
{
    DPRINT("KsStreamPointerAdvanceOffsets\n");

    UNIMPLEMENTED
}

/*
    @implemented
*/
KSDDKAPI
VOID
NTAPI
KsStreamPointerDelete(
    IN PKSSTREAM_POINTER StreamPointer)
{
    IKsPinImpl * This;
    PKSISTREAM_POINTER Cur, Last;
    PKSISTREAM_POINTER Pointer = (PKSISTREAM_POINTER)StreamPointer;

    DPRINT("KsStreamPointerDelete\n");

    This = (IKsPinImpl*)CONTAINING_RECORD(Pointer->StreamPointer.Pin, IKsPinImpl, Pin);

    /* point to first stream pointer */
    Last = NULL;
    Cur = This->ClonedStreamPointer;

    while(Cur != Pointer && Cur)
    {
        Last = Cur;
        /* iterate to next cloned pointer */
        Cur = Cur->Next;
    }

    if (!Cur)
    {
        /* you naughty driver */
        return;
    }

    if (!Last)
    {
        /* remove first cloned pointer */
        This->ClonedStreamPointer = Pointer->Next;
    }
    else
    {
        Last->Next = Pointer->Next;
    }

    /* FIXME make sure no timeouts are pending */
    FreeItem(Pointer);
}

/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsStreamPointerClone(
    IN PKSSTREAM_POINTER StreamPointer,
    IN PFNKSSTREAMPOINTER CancelCallback OPTIONAL,
    IN ULONG ContextSize,
    OUT PKSSTREAM_POINTER* CloneStreamPointer)
{
    UNIMPLEMENTED
    DPRINT("KsStreamPointerClone\n");
    return STATUS_NOT_IMPLEMENTED;
}

/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsStreamPointerAdvanceOffsets(
    IN PKSSTREAM_POINTER StreamPointer,
    IN ULONG InUsed,
    IN ULONG OutUsed,
    IN BOOLEAN Eject)
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
KsStreamPointerAdvance(
    IN PKSSTREAM_POINTER StreamPointer)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

/*
    @unimplemented
*/
KSDDKAPI
PMDL
NTAPI
KsStreamPointerGetMdl(
    IN PKSSTREAM_POINTER StreamPointer)
{
    UNIMPLEMENTED
    return NULL;
}

/*
    @unimplemented
*/
KSDDKAPI
PIRP
NTAPI
KsStreamPointerGetIrp(
    IN PKSSTREAM_POINTER StreamPointer,
    OUT PBOOLEAN FirstFrameInIrp OPTIONAL,
    OUT PBOOLEAN LastFrameInIrp OPTIONAL)
{
    UNIMPLEMENTED
    return NULL;
}

/*
    @implemented
*/
KSDDKAPI
VOID
NTAPI
KsStreamPointerScheduleTimeout(
    IN PKSSTREAM_POINTER StreamPointer,
    IN PFNKSSTREAMPOINTER Callback,
    IN ULONGLONG Interval)
{
    LARGE_INTEGER DueTime;
    PKSISTREAM_POINTER Pointer = (PKSISTREAM_POINTER)StreamPointer;

    /* setup timer callback */
    Pointer->Callback = Callback;

    /* setup expiration */
    DueTime.QuadPart = (LONGLONG)Interval;

    /* setup the timer */
    KeSetTimer(&Pointer->Timer, DueTime, &Pointer->TimerDpc);

}

/*
    @implemented
*/
KSDDKAPI
VOID
NTAPI
KsStreamPointerCancelTimeout(
    IN PKSSTREAM_POINTER StreamPointer)
{
    PKSISTREAM_POINTER Pointer = (PKSISTREAM_POINTER)StreamPointer;

    KeCancelTimer(&Pointer->Timer);

}

/*
    @implemented
*/
KSDDKAPI
PKSSTREAM_POINTER
NTAPI
KsPinGetFirstCloneStreamPointer(
    IN PKSPIN Pin)
{
    IKsPinImpl * This;

    DPRINT("KsPinGetFirstCloneStreamPointer %p\n", Pin);

    This = (IKsPinImpl*)CONTAINING_RECORD(Pin, IKsPinImpl, Pin);
    /* return first cloned stream pointer */
    return &This->ClonedStreamPointer->StreamPointer;
}

/*
    @implemented
*/
KSDDKAPI
PKSSTREAM_POINTER
NTAPI
KsStreamPointerGetNextClone(
    IN PKSSTREAM_POINTER StreamPointer)
{
    PKSISTREAM_POINTER Pointer = (PKSISTREAM_POINTER)StreamPointer;

    DPRINT("KsStreamPointerGetNextClone\n");

    /* is there a another cloned stream pointer */
    if (!Pointer->Next)
        return NULL;

    /* return next stream pointer */
    return &Pointer->Next->StreamPointer;
}

NTSTATUS
NTAPI
IKsPin_DispatchDeviceIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PKSIOBJECT_HEADER ObjectHeader;
    IKsPinImpl * This;
    NTSTATUS Status = STATUS_SUCCESS;

    /* get current irp stack */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* sanity check */
    ASSERT(IoStack->FileObject);
    ASSERT(IoStack->FileObject->FsContext2);

    /* get the object header */
    ObjectHeader = (PKSIOBJECT_HEADER)IoStack->FileObject->FsContext2;

    /* locate ks pin implemention fro KSPIN offset */
    This = (IKsPinImpl*)CONTAINING_RECORD(ObjectHeader->ObjectType, IKsPinImpl, Pin);

    if (IoStack->Parameters.DeviceIoControl.IoControlCode != IOCTL_KS_WRITE_STREAM && IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_KS_READ_STREAM)
    {
        UNIMPLEMENTED;
        Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_NOT_IMPLEMENTED;
    }

    /* mark irp as pending */
    IoMarkIrpPending(Irp);

    /* add irp to cancelable queue */
    KsAddIrpToCancelableQueue(&This->IrpList, &This->IrpListLock, Irp, KsListEntryTail, NULL /* FIXME */);

    if (This->Pin.Descriptor->Dispatch->Process)
    {
        /* it is a pin centric avstream */
        Status = This->Pin.Descriptor->Dispatch->Process(&This->Pin);

        /* TODO */
    }
    else
    {
        /* TODO
         * filter-centric avstream 
         */
        UNIMPLEMENTED
    }

    return Status;
}

NTSTATUS
NTAPI
IKsPin_Close(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PKSIOBJECT_HEADER ObjectHeader;
    IKsPinImpl * This;
    NTSTATUS Status = STATUS_SUCCESS;

    /* get current irp stack */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* sanity check */
    ASSERT(IoStack->FileObject);
    ASSERT(IoStack->FileObject->FsContext2);

    /* get the object header */
    ObjectHeader = (PKSIOBJECT_HEADER)IoStack->FileObject->FsContext2;

    /* locate ks pin implemention fro KSPIN offset */
    This = (IKsPinImpl*)CONTAINING_RECORD(ObjectHeader->ObjectType, IKsPinImpl, Pin);

    /* acquire filter control mutex */
    KsFilterAcquireControl(This->BasicHeader.Parent.KsFilter);

    if (This->Pin.Descriptor->Dispatch->Close)
    {
        /* call pin close routine */
        Status = This->Pin.Descriptor->Dispatch->Close(&This->Pin, Irp);

        if (!NT_SUCCESS(Status))
        {
            /* abort closing */
            Irp->IoStatus.Status = Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return Status;
        }

        /* FIXME remove pin from filter pin list and decrement reference count */

        if (Status != STATUS_PENDING)
        {
            Irp->IoStatus.Status = Status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return Status;
        }
    }

    return Status;
}

NTSTATUS
NTAPI
IKsPin_DispatchCreateAllocator(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    UNIMPLEMENTED;

    Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
IKsPin_DispatchCreateClock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    UNIMPLEMENTED;

    Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
IKsPin_DispatchCreateNode(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    UNIMPLEMENTED;

    Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_NOT_IMPLEMENTED;
}

static KSDISPATCH_TABLE PinDispatchTable = 
{
    IKsPin_DispatchDeviceIoControl,
    KsDispatchInvalidDeviceRequest,
    KsDispatchInvalidDeviceRequest,
    KsDispatchInvalidDeviceRequest,
    IKsPin_Close,
    KsDispatchQuerySecurity,
    KsDispatchSetSecurity,
    KsDispatchFastIoDeviceControlFailure,
    KsDispatchFastReadFailure,
    KsDispatchFastReadFailure
};

NTSTATUS
KspCreatePin(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp, 
    IN PKSDEVICE KsDevice,
    IN IKsFilterFactory * FilterFactory,
    IN IKsFilter* Filter,
    IN PKSPIN_CONNECT Connect,
    IN KSPIN_DESCRIPTOR_EX* Descriptor)
{
    IKsPinImpl * This;
    PIO_STACK_LOCATION IoStack;
    IKsDevice * Device;
    PDEVICE_EXTENSION DeviceExtension;
    PKSOBJECT_CREATE_ITEM CreateItem;
    NTSTATUS Status;

    /* sanity checks */
    ASSERT(Descriptor->Dispatch);
    ASSERT(Descriptor->Dispatch->Create);

    /* get current irp stack */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* get device extension */
    DeviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;

    /* get ks device interface */
    Device = (IKsDevice*)&DeviceExtension->DeviceHeader->lpVtblIKsDevice;

    /* first allocate pin ctx */
    This = AllocateItem(NonPagedPool, sizeof(IKsPinImpl));
    if (!This)
    {
        /* not enough memory */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* allocate create item */
    CreateItem = AllocateItem(NonPagedPool, sizeof(KSOBJECT_CREATE_ITEM) * 3);
    if (!CreateItem)
    {
        /* not enough memory */
        FreeItem(This);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* initialize basic header */
    This->BasicHeader.KsDevice = KsDevice;
    This->BasicHeader.Type = KsObjectTypePin;
    This->BasicHeader.Parent.KsFilter = Filter->lpVtbl->GetStruct(Filter);
    KeInitializeMutex(&This->BasicHeader.ControlMutex, 0);
    InitializeListHead(&This->BasicHeader.EventList);
    KeInitializeSpinLock(&This->BasicHeader.EventListLock);

    /* initialize pin */
    This->lpVtbl = &vt_IKsPin;
    This->ref = 1;
    This->FileObject = IoStack->FileObject;
    KeInitializeMutex(&This->ProcessingMutex, 0);
    InitializeListHead(&This->IrpList);
    KeInitializeSpinLock(&This->IrpListLock);

    /* initialize ks pin descriptor */
    This->Pin.Descriptor = Descriptor;
    This->Pin.Id = Connect->PinId;

    /* allocate object bag */
    This->Pin.Bag = AllocateItem(NonPagedPool, sizeof(KSIOBJECT_BAG));
    if (!This->Pin.Bag)
    {
        /* not enough memory */
        FreeItem(This);
        FreeItem(CreateItem);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* initialize object bag */
    Device->lpVtbl->InitializeObjectBag(Device, This->Pin.Bag, &This->BasicHeader.ControlMutex); /* is using control mutex right? */

    This->Pin.Communication = Descriptor->PinDescriptor.Communication;
    This->Pin.ConnectionIsExternal = FALSE; /* FIXME */
    //FIXME This->Pin.ConnectionInterface = Descriptor->PinDescriptor.Interfaces;
    //FIXME This->Pin.ConnectionMedium = Descriptor->PinDescriptor.Mediums;
    //FIXME This->Pin.ConnectionPriority = KSPRIORITY_NORMAL;
    This->Pin.ConnectionFormat = (PKSDATAFORMAT) (Connect + 1);
    This->Pin.AttributeList = NULL; //FIXME
    This->Pin.StreamHeaderSize = sizeof(KSSTREAM_HEADER);
    This->Pin.DataFlow = Descriptor->PinDescriptor.DataFlow;
    This->Pin.DeviceState = KSSTATE_STOP;
    This->Pin.ResetState = KSRESET_END;
    This->Pin.ClientState = KSSTATE_STOP;

    /* intialize allocator create item */
    CreateItem[0].Context = (PVOID)This;
    CreateItem[0].Create = IKsPin_DispatchCreateAllocator;
    CreateItem[0].Flags = KSCREATE_ITEM_FREEONSTOP;
    RtlInitUnicodeString(&CreateItem[0].ObjectClass, KSSTRING_Allocator);

    /* intialize clock create item */
    CreateItem[1].Context = (PVOID)This;
    CreateItem[1].Create = IKsPin_DispatchCreateClock;
    CreateItem[1].Flags = KSCREATE_ITEM_FREEONSTOP;
    RtlInitUnicodeString(&CreateItem[1].ObjectClass, KSSTRING_Clock);

    /* intialize topology node create item */
    CreateItem[2].Context = (PVOID)This;
    CreateItem[2].Create = IKsPin_DispatchCreateNode;
    CreateItem[2].Flags = KSCREATE_ITEM_FREEONSTOP;
    RtlInitUnicodeString(&CreateItem[2].ObjectClass, KSSTRING_TopologyNode);

    /* now allocate object header */
    Status = KsAllocateObjectHeader((KSOBJECT_HEADER*)&This->ObjectHeader, 3, CreateItem, Irp, &PinDispatchTable);
    if (!NT_SUCCESS(Status))
    {
        /* failed to create object header */
        KsFreeObjectBag((KSOBJECT_BAG)This->Pin.Bag);
        FreeItem(This);
        FreeItem(CreateItem);

        /* return failure code */
        return Status;
    }

     /* add extra info to object header */
    This->ObjectHeader->Type = KsObjectTypePin;
    This->ObjectHeader->Unknown = (PUNKNOWN)&This->lpVtbl;
    This->ObjectHeader->ObjectType = (PVOID)&This->Pin;

    /* setup process pin */
    This->ProcessPin.Pin = &This->Pin;
    This->ProcessPin.StreamPointer = (PKSSTREAM_POINTER)This->LeadingEdgeStreamPointer;

    if (!Descriptor->Dispatch || !Descriptor->Dispatch->Process)
    {
        /* the pin is part of filter-centric processing filter
         * add process pin to filter
         */

        Status = Filter->lpVtbl->AddProcessPin(Filter, &This->ProcessPin);
        if (!NT_SUCCESS(Status))
        {
            /* failed to add process pin */
            KsFreeObjectBag((KSOBJECT_BAG)This->Pin.Bag);
            KsFreeObjectHeader(&This->ObjectHeader);

            /* return failure code */
            return Status;
        }
    }

    /* FIXME add pin instance to filter instance */

    /* does the driver have a pin dispatch */
    if (Descriptor->Dispatch && Descriptor->Dispatch->Create)
    {
        /*  now inform the driver to create a new pin */
        Status = Descriptor->Dispatch->Create(&This->Pin, Irp);
    }

    if (!NT_SUCCESS(Status) && Status != STATUS_PENDING)
    {
        /* failed to create pin, release resources */
        KsFreeObjectHeader((KSOBJECT_HEADER)This->ObjectHeader);
        KsFreeObjectBag((KSOBJECT_BAG)This->Pin.Bag);
        FreeItem(This);

        /* return failure code */
        return Status;
    }

    return Status;
}
