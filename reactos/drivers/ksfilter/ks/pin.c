/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/ksfilter/ks/worker.c
 * PURPOSE:         KS pin functions
 * PROGRAMMER:      Johannes Anderwald
 */

#include "precomp.h"

#include <bdamedia.h>

#define NDEBUG
#include <debug.h>

typedef struct _KSISTREAM_POINTER
{
    PFNKSSTREAMPOINTER Callback;
    PIRP Irp;
    KTIMER Timer;
    KDPC TimerDpc;
    struct _KSISTREAM_POINTER *Next;
    PKSPIN Pin;
    PVOID Data;
    ULONG Offset;
    ULONG Length;
    KSSTREAM_POINTER StreamPointer;
    KSPIN_LOCK Lock;
}KSISTREAM_POINTER, *PKSISTREAM_POINTER;

typedef struct
{
    KSBASIC_HEADER BasicHeader;
    KSPIN Pin;
    PKSIOBJECT_HEADER ObjectHeader;
    KSPROCESSPIN ProcessPin;
    LIST_ENTRY Entry;

    LONG ref;

    IKsFilter * Filter;
    KMUTEX ProcessingMutex;
    PFILE_OBJECT FileObject;

    PKSGATE AttachedGate;
    BOOL OrGate;

    LIST_ENTRY IrpList;
    KSPIN_LOCK IrpListLock;
    volatile LONG IrpCount;

    PKSISTREAM_POINTER ClonedStreamPointer;
    KSISTREAM_POINTER LeadingEdgeStreamPointer;
    KSISTREAM_POINTER TrailingStreamPointer;

    PFNKSPINPOWER  Sleep;
    PFNKSPINPOWER  Wake;
    PFNKSPINHANDSHAKE  Handshake;
    PFNKSPINFRAMERETURN  FrameReturn;
    PFNKSPINIRPCOMPLETION  IrpCompletion;

    KSCLOCK_FUNCTIONTABLE ClockTable;
    PFILE_OBJECT ClockFileObject;
    IKsReferenceClockVtbl * lpVtblReferenceClock;
    PKSDEFAULTCLOCK DefaultClock;

    PKSWORKER PinWorker;
    WORK_QUEUE_ITEM PinWorkQueueItem;
    KEVENT FrameComplete;
    ULONG FrameSize;
    ULONG NumFrames;
    PDMA_ADAPTER Dma;
    ULONG MapRegisters;

}IKsPinImpl;

NTSTATUS NTAPI IKsPin_PinStatePropertyHandler(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);
NTSTATUS NTAPI IKsPin_PinDataFormatPropertyHandler(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);
NTSTATUS NTAPI IKsPin_PinAllocatorFramingPropertyHandler(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);
NTSTATUS NTAPI IKsPin_PinStreamAllocator(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);
NTSTATUS NTAPI IKsPin_PinMasterClock(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);
NTSTATUS NTAPI IKsPin_PinPipeId(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);



DEFINE_KSPROPERTY_CONNECTIONSET(PinConnectionSet, IKsPin_PinStatePropertyHandler, IKsPin_PinDataFormatPropertyHandler, IKsPin_PinAllocatorFramingPropertyHandler);
DEFINE_KSPROPERTY_STREAMSET(PinStreamSet, IKsPin_PinStreamAllocator, IKsPin_PinMasterClock, IKsPin_PinPipeId);

//TODO
// KSPROPSETID_Connection
//    KSPROPERTY_CONNECTION_ACQUIREORDERING
// KSPROPSETID_StreamInterface
//     KSPROPERTY_STREAMINTERFACE_HEADERSIZE

KSPROPERTY_SET PinPropertySet[] =
{
    {
        &KSPROPSETID_Connection,
        sizeof(PinConnectionSet) / sizeof(KSPROPERTY_ITEM),
        (const KSPROPERTY_ITEM*)&PinConnectionSet,
        0,
        NULL
    },
    {
        &KSPROPSETID_Stream,
        sizeof(PinStreamSet) / sizeof(KSPROPERTY_ITEM),
        (const KSPROPERTY_ITEM*)&PinStreamSet,
        0,
        NULL
    }
};

const GUID KSPROPSETID_Connection              = {0x1D58C920L, 0xAC9B, 0x11CF, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};
const GUID KSPROPSETID_Stream                  = {0x65aaba60L, 0x98ae, 0x11cf, {0xa1, 0x0d, 0x00, 0x20, 0xaf, 0xd1, 0x56, 0xe4}};
const GUID KSPROPSETID_Clock                   = {0xDF12A4C0L, 0xAC17, 0x11CF, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};

NTSTATUS
NTAPI
IKsPin_PinStreamAllocator(
    IN PIRP Irp,
    IN PKSIDENTIFIER Request,
    IN OUT PVOID Data)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
IKsPin_PinMasterClock(
    IN PIRP Irp,
    IN PKSIDENTIFIER Request,
    IN OUT PVOID Data)
{
    PIO_STACK_LOCATION IoStack;
    PKSIOBJECT_HEADER ObjectHeader;
    IKsPinImpl * This;
    NTSTATUS Status = STATUS_SUCCESS;
    PHANDLE Handle;
    PFILE_OBJECT FileObject;
    KPROCESSOR_MODE Mode;
    KSPROPERTY Property;
    ULONG BytesReturned;

    /* get current irp stack */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("IKsPin_PinMasterClock\n");

    /* sanity check */
    ASSERT(IoStack->FileObject);
    ASSERT(IoStack->FileObject->FsContext2);

    /* get the object header */
    ObjectHeader = (PKSIOBJECT_HEADER)IoStack->FileObject->FsContext2;

    /* sanity check */
    ASSERT(ObjectHeader);

    /* locate ks pin implemention from KSPIN offset */
    This = (IKsPinImpl*)CONTAINING_RECORD(ObjectHeader->ObjectType, IKsPinImpl, Pin);

    /* sanity check */
    ASSERT(This);

    Handle = (PHANDLE)Data;

    if (Request->Flags & KSPROPERTY_TYPE_GET)
    {
        if (This->Pin.Descriptor->PinDescriptor.Communication != KSPIN_COMMUNICATION_NONE &&
            This->Pin.Descriptor->Dispatch &&
            (This->Pin.Descriptor->Flags & KSPIN_FLAG_IMPLEMENT_CLOCK))
        {
            *Handle = NULL;
            Status = STATUS_SUCCESS;
        }
        else
        {
            /* no clock available */
            Status = STATUS_UNSUCCESSFUL;
        }
    }
    else if (Request->Flags & KSPROPERTY_TYPE_SET)
    {
        if (This->Pin.ClientState != KSSTATE_STOP)
        {
            /* can only set in stopped state */
            Status = STATUS_INVALID_DEVICE_STATE;
        }
        else
        {
            if (*Handle)
            {
                Mode = ExGetPreviousMode();

                Status = ObReferenceObjectByHandle(*Handle, SYNCHRONIZE | DIRECTORY_QUERY, *IoFileObjectType, Mode, (PVOID*)&FileObject, NULL);

                DPRINT("IKsPin_PinMasterClock ObReferenceObjectByHandle %lx\n", Status);
                if (NT_SUCCESS(Status))
                {
                    Property.Set = KSPROPSETID_Clock;
                    Property.Id = KSPROPERTY_CLOCK_FUNCTIONTABLE;
                    Property.Flags = KSPROPERTY_TYPE_GET;

                    Status = KsSynchronousIoControlDevice(FileObject, KernelMode, IOCTL_KS_PROPERTY, &Property, sizeof(KSPROPERTY), &This->ClockTable, sizeof(KSCLOCK_FUNCTIONTABLE), &BytesReturned);

                    DPRINT("IKsPin_PinMasterClock KSPROPERTY_CLOCK_FUNCTIONTABLE %lx\n", Status);

                    if (NT_SUCCESS(Status))
                    {
                        This->ClockFileObject = FileObject;
                    }
                    else
                    {
                        ObDereferenceObject(FileObject);
                    }
                }
            }
            else
            {
                /* zeroing clock handle */
                RtlZeroMemory(&This->ClockTable, sizeof(KSCLOCK_FUNCTIONTABLE));
                Status = STATUS_SUCCESS;
                if (This->ClockFileObject)
                {
                    FileObject = This->ClockFileObject;
                    This->ClockFileObject = NULL;

                    ObDereferenceObject(This->ClockFileObject);
                }
            }
        }
    }

    DPRINT("IKsPin_PinMasterClock Status %lx\n", Status);
    return Status;
}



NTSTATUS
NTAPI
IKsPin_PinPipeId(
    IN PIRP Irp,
    IN PKSIDENTIFIER Request,
    IN OUT PVOID Data)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
NTAPI
IKsPin_PinStatePropertyHandler(
    IN PIRP Irp,
    IN PKSIDENTIFIER Request,
    IN OUT PVOID Data)
{
    PIO_STACK_LOCATION IoStack;
    PKSIOBJECT_HEADER ObjectHeader;
    IKsPinImpl * This;
    NTSTATUS Status = STATUS_SUCCESS;
    KSSTATE OldState;
    PKSSTATE NewState;

    /* get current irp stack */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("IKsPin_PinStatePropertyHandler\n");

    /* sanity check */
    ASSERT(IoStack->FileObject);
    ASSERT(IoStack->FileObject->FsContext2);

    /* get the object header */
    ObjectHeader = (PKSIOBJECT_HEADER)IoStack->FileObject->FsContext2;

    /* locate ks pin implemention from KSPIN offset */
    This = (IKsPinImpl*)CONTAINING_RECORD(ObjectHeader->ObjectType, IKsPinImpl, Pin);

    /* acquire control mutex */
    KeWaitForSingleObject(This->BasicHeader.ControlMutex, Executive, KernelMode, FALSE, NULL);

    /* grab state */
    NewState = (PKSSTATE)Data;

    if (Request->Flags & KSPROPERTY_TYPE_GET)
    {
        *NewState = This->Pin.DeviceState;
        Irp->IoStatus.Information = sizeof(KSSTATE);
    }
    else if (Request->Flags & KSPROPERTY_TYPE_SET)
    {
        if (This->Pin.Descriptor->Dispatch->SetDeviceState)
        {
            /* backup old state */
            OldState = This->Pin.ClientState;

            /* set new state */
            This->Pin.ClientState  = *NewState;
            This->Pin.DeviceState = KSSTATE_RUN;

            /* check if it supported */
            Status = This->Pin.Descriptor->Dispatch->SetDeviceState(&This->Pin, *NewState, OldState);

            DPRINT("IKsPin_PinStatePropertyHandler NewState %lu Result %lx\n", *NewState, Status);

            if (!NT_SUCCESS(Status))
            {
                /* revert to old state */
                This->Pin.ClientState = OldState;
                This->Pin.DeviceState = OldState;
                DPRINT("IKsPin_PinStatePropertyHandler failed to set state %lx Result %lx\n", *NewState, Status);
                DbgBreakPoint();
            }
            else
            {
                /* update device state */
                This->Pin.DeviceState = *NewState;
            }
        }
        else
        {
            /* just set new state */
            This->Pin.DeviceState = *NewState;
            This->Pin.ClientState = *NewState;
        }
    }

    /* release processing mutex */
    KeReleaseMutex(This->BasicHeader.ControlMutex, FALSE);

    DPRINT("IKsPin_PinStatePropertyHandler Status %lx\n", Status);
    return Status;
}

NTSTATUS
NTAPI
IKsPin_PinAllocatorFramingPropertyHandler(
    IN PIRP Irp,
    IN PKSIDENTIFIER Request,
    IN OUT PVOID Data)
{
    PIO_STACK_LOCATION IoStack;
    PKSIOBJECT_HEADER ObjectHeader;
    IKsPinImpl * This;
    ULONG Size;
    NTSTATUS Status = STATUS_SUCCESS;

    /* get current irp stack */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* sanity check */
    ASSERT(IoStack->FileObject);
    ASSERT(IoStack->FileObject->FsContext2);

    /* get the object header */
    ObjectHeader = (PKSIOBJECT_HEADER)IoStack->FileObject->FsContext2;

    /* locate ks pin implemention from KSPIN offset */
    This = (IKsPinImpl*)CONTAINING_RECORD(ObjectHeader->ObjectType, IKsPinImpl, Pin);

    /* setting allocator flags is not supported */
    ASSERT(!(Request->Flags & KSPROPERTY_TYPE_SET));

    /* acquire control mutex */
    KeWaitForSingleObject(This->BasicHeader.ControlMutex, Executive, KernelMode, FALSE, NULL);

    if (This->Pin.Descriptor->AllocatorFraming)
    {
        /* calculate size */
        Size = FIELD_OFFSET(KSALLOCATOR_FRAMING_EX, FramingItem[0]) + This->Pin.Descriptor->AllocatorFraming->CountItems * sizeof(KS_FRAMING_ITEM);

        if (IoStack->Parameters.DeviceIoControl.OutputBufferLength == 0)
        {
            /* no buffer */
            Status = STATUS_BUFFER_OVERFLOW;
        }
        else if (Size > IoStack->Parameters.DeviceIoControl.OutputBufferLength)
        {
            /* buffer too small */
            Status = STATUS_BUFFER_TOO_SMALL;
        }
        else
        {
            /* copy buffer */
            RtlMoveMemory(Data, This->Pin.Descriptor->AllocatorFraming, Size);
        }

        /* store size */
        Irp->IoStatus.Information = Size;
    }
    else
    {
        /* no allocator framing details */
        Status = STATUS_NOT_FOUND;
    }

    /* release processing mutex */
    KeReleaseMutex(This->BasicHeader.ControlMutex, FALSE);

    DPRINT("IKsPin_PinAllocatorFramingPropertyHandler Status %lx\n", Status);

    return Status;
}

NTSTATUS
NTAPI
IKsPin_PinDataFormatPropertyHandler(
    IN PIRP Irp,
    IN PKSPROPERTY Request,
    IN OUT PVOID Data)
{
    PIO_STACK_LOCATION IoStack;
    PKSIOBJECT_HEADER ObjectHeader;
    IKsPinImpl * This;
    NTSTATUS Status = STATUS_SUCCESS;

    /* get current irp stack */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("IKsPin_PinDataFormatPropertyHandler\n");

    /* sanity check */
    ASSERT(IoStack->FileObject);
    ASSERT(IoStack->FileObject->FsContext2);

    /* get the object header */
    ObjectHeader = (PKSIOBJECT_HEADER)IoStack->FileObject->FsContext2;

    /* locate ks pin implemention from KSPIN offset */
    This = (IKsPinImpl*)CONTAINING_RECORD(ObjectHeader->ObjectType, IKsPinImpl, Pin);

    /* acquire control mutex */
    KeWaitForSingleObject(This->BasicHeader.ControlMutex, Executive, KernelMode, FALSE, NULL);

    if (Request->Flags & KSPROPERTY_TYPE_GET)
    {
        if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < This->Pin.ConnectionFormat->FormatSize)
        {
            /* buffer too small */
            Irp->IoStatus.Information = This->Pin.ConnectionFormat->FormatSize;
            Status = STATUS_BUFFER_TOO_SMALL;
        }
        else
        {
            /* copy format */
            RtlMoveMemory(Data, This->Pin.ConnectionFormat, This->Pin.ConnectionFormat->FormatSize);
        }
    }
    else if (Request->Flags & KSPROPERTY_TYPE_SET)
    {
        /* set format */
        if (This->Pin.Descriptor->Flags & KSPIN_FLAG_FIXED_FORMAT)
        {
            /* format cannot be changed */
            Status = STATUS_INVALID_DEVICE_REQUEST;
        }
        else
        {
            /* FIXME check if the format is supported */
            Status = _KsEdit(This->Pin.Bag, (PVOID*)&This->Pin.ConnectionFormat, IoStack->Parameters.DeviceIoControl.OutputBufferLength, This->Pin.ConnectionFormat->FormatSize, 0);

            if (NT_SUCCESS(Status))
            {
                /* store new format */
                RtlMoveMemory(This->Pin.ConnectionFormat, Data, IoStack->Parameters.DeviceIoControl.OutputBufferLength);
            }
        }
    }

    /* release processing mutex */
    KeReleaseMutex(This->BasicHeader.ControlMutex, FALSE);

    DPRINT("IKsPin_PinDataFormatPropertyHandler Status %lx\n", Status);

    return Status;
}

NTSTATUS
NTAPI
IKsPin_fnQueryInterface(
    IKsPin * iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    NTSTATUS Status;
    IKsPinImpl * This = (IKsPinImpl*)CONTAINING_RECORD(iface, IKsPinImpl, BasicHeader.OuterUnknown);

    if (IsEqualGUIDAligned(refiid, &IID_IUnknown))
    {
        *Output = &This->BasicHeader.OuterUnknown;
        _InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }


    if (This->BasicHeader.ClientAggregate)
    {
         /* using client aggregate */
         Status = This->BasicHeader.ClientAggregate->lpVtbl->QueryInterface(This->BasicHeader.ClientAggregate, refiid, Output);

         if (NT_SUCCESS(Status))
         {
             /* client aggregate supports interface */
             return Status;
         }
    }

    DPRINT("IKsPin_fnQueryInterface no interface\n");
    return STATUS_NOT_SUPPORTED;
}

ULONG
NTAPI
IKsPin_fnAddRef(
    IKsPin * iface)
{
    IKsPinImpl * This = (IKsPinImpl*)CONTAINING_RECORD(iface, IKsPinImpl, BasicHeader.OuterUnknown);

    return InterlockedIncrement(&This->ref);
}

ULONG
NTAPI
IKsPin_fnRelease(
    IKsPin * iface)
{
    IKsPinImpl * This = (IKsPinImpl*)CONTAINING_RECORD(iface, IKsPinImpl, BasicHeader.OuterUnknown);

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

NTSTATUS
NTAPI
IKsReferenceClock_fnQueryInterface(
    IKsReferenceClock * iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    IKsPinImpl * This = (IKsPinImpl*)CONTAINING_RECORD(iface, IKsPinImpl, lpVtblReferenceClock);

    return IKsPin_fnQueryInterface((IKsPin*)&This->BasicHeader.OuterUnknown, refiid, Output);
}

ULONG
NTAPI
IKsReferenceClock_fnAddRef(
    IKsReferenceClock * iface)
{
    IKsPinImpl * This = (IKsPinImpl*)CONTAINING_RECORD(iface, IKsPinImpl, lpVtblReferenceClock);

    return IKsPin_fnAddRef((IKsPin*)&This->BasicHeader.OuterUnknown);
}

ULONG
NTAPI
IKsReferenceClock_fnRelease(
    IKsReferenceClock * iface)
{
    IKsPinImpl * This = (IKsPinImpl*)CONTAINING_RECORD(iface, IKsPinImpl, lpVtblReferenceClock);

    return IKsPin_fnRelease((IKsPin*)&This->BasicHeader.OuterUnknown);
}

LONGLONG
NTAPI
IKsReferenceClock_fnGetTime(
    IKsReferenceClock * iface)
{
    LONGLONG Result;

    IKsPinImpl * This = (IKsPinImpl*)CONTAINING_RECORD(iface, IKsPinImpl, lpVtblReferenceClock);


    DPRINT1("IKsReferenceClock_fnGetTime\n");

    if (!This->ClockFileObject || !This->ClockTable.GetTime)
    {
        Result = 0;
    }
    else
    {
        Result = This->ClockTable.GetTime(This->ClockFileObject);
    }

    return Result;
}

LONGLONG
NTAPI
IKsReferenceClock_fnGetPhysicalTime(
    IKsReferenceClock * iface)
{
    LONGLONG Result;

    IKsPinImpl * This = (IKsPinImpl*)CONTAINING_RECORD(iface, IKsPinImpl, lpVtblReferenceClock);

    DPRINT1("IKsReferenceClock_fnGetPhysicalTime\n");


    if (!This->ClockFileObject || !This->ClockTable.GetPhysicalTime)
    {
        Result = 0;
    }
    else
    {
        Result = This->ClockTable.GetPhysicalTime(This->ClockFileObject);
    }

    return Result;
}


LONGLONG
NTAPI
IKsReferenceClock_fnGetCorrelatedTime(
    IKsReferenceClock * iface,
    OUT PLONGLONG SystemTime)
{
    LONGLONG Result;

    IKsPinImpl * This = (IKsPinImpl*)CONTAINING_RECORD(iface, IKsPinImpl, lpVtblReferenceClock);

    DPRINT1("IKsReferenceClock_fnGetCorrelatedTime\n");

    if (!This->ClockFileObject || !This->ClockTable.GetCorrelatedTime)
    {
        Result = 0;
    }
    else
    {
        Result = This->ClockTable.GetCorrelatedTime(This->ClockFileObject, SystemTime);
    }

    return Result;
}


LONGLONG
NTAPI
IKsReferenceClock_fnGetCorrelatedPhysicalTime(
    IKsReferenceClock * iface,
    OUT PLONGLONG SystemTime)
{
    LONGLONG Result;

    IKsPinImpl * This = (IKsPinImpl*)CONTAINING_RECORD(iface, IKsPinImpl, lpVtblReferenceClock);

    DPRINT1("IKsReferenceClock_fnGetCorrelatedPhysicalTime\n");

    if (!This->ClockFileObject || !This->ClockTable.GetCorrelatedPhysicalTime)
    {
        Result = 0;
    }
    else
    {
        Result = This->ClockTable.GetCorrelatedPhysicalTime(This->ClockFileObject, SystemTime);
    }

    return Result;
}

NTSTATUS
NTAPI
IKsReferenceClock_fnGetResolution(
    IKsReferenceClock * iface,
    OUT PKSRESOLUTION Resolution)
{
    KSPROPERTY Property;
    ULONG BytesReturned;

    IKsPinImpl * This = (IKsPinImpl*)CONTAINING_RECORD(iface, IKsPinImpl, lpVtblReferenceClock);

    DPRINT1("IKsReferenceClock_fnGetResolution\n");

    if (!This->ClockFileObject)
    {
        Resolution->Error = 0;
        Resolution->Granularity = 1;
        DPRINT1("IKsReferenceClock_fnGetResolution Using HACK\n");
        return STATUS_SUCCESS;
    }


    if (!This->ClockFileObject)
        return STATUS_DEVICE_NOT_READY;


    Property.Set = KSPROPSETID_Clock;
    Property.Id = KSPROPERTY_CLOCK_RESOLUTION;
    Property.Flags = KSPROPERTY_TYPE_GET;

    return KsSynchronousIoControlDevice(This->ClockFileObject, KernelMode, IOCTL_KS_PROPERTY, &Property, sizeof(KSPROPERTY), Resolution, sizeof(KSRESOLUTION), &BytesReturned);

}

NTSTATUS
NTAPI
IKsReferenceClock_fnGetState(
    IKsReferenceClock * iface,
     OUT PKSSTATE State)
{
    KSPROPERTY Property;
    ULONG BytesReturned;

    IKsPinImpl * This = (IKsPinImpl*)CONTAINING_RECORD(iface, IKsPinImpl, lpVtblReferenceClock);

    DPRINT1("IKsReferenceClock_fnGetState\n");

    if (!This->ClockFileObject)
    {
        *State = This->Pin.ClientState;
        DPRINT1("IKsReferenceClock_fnGetState Using HACK\n");
        return STATUS_SUCCESS;
    }


    if (!This->ClockFileObject)
        return STATUS_DEVICE_NOT_READY;


    Property.Set = KSPROPSETID_Clock;
    Property.Id = KSPROPERTY_CLOCK_RESOLUTION;
    Property.Flags = KSPROPERTY_TYPE_GET;

    return KsSynchronousIoControlDevice(This->ClockFileObject, KernelMode, IOCTL_KS_PROPERTY, &Property, sizeof(KSPROPERTY), State, sizeof(KSSTATE), &BytesReturned);
}

static IKsReferenceClockVtbl vt_ReferenceClock =
{
    IKsReferenceClock_fnQueryInterface,
    IKsReferenceClock_fnAddRef,
    IKsReferenceClock_fnRelease,
    IKsReferenceClock_fnGetTime,
    IKsReferenceClock_fnGetPhysicalTime,
    IKsReferenceClock_fnGetCorrelatedTime,
    IKsReferenceClock_fnGetCorrelatedPhysicalTime,
    IKsReferenceClock_fnGetResolution,
    IKsReferenceClock_fnGetState
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
    DbgBreakPoint();
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
    @implemented
*/
NTSTATUS
NTAPI
KsPinGetReferenceClockInterface(
    IN PKSPIN  Pin,
    OUT PIKSREFERENCECLOCK*  Interface)
{
    NTSTATUS Status = STATUS_DEVICE_NOT_READY;
    IKsPinImpl * This = (IKsPinImpl*)CONTAINING_RECORD(Pin, IKsPinImpl, Pin);

    if (This->ClockFileObject)
    {
        /* clock is available */
        *Interface = (PIKSREFERENCECLOCK)&This->lpVtblReferenceClock;
        Status = STATUS_SUCCESS;
    }

    DPRINT("KsPinGetReferenceClockInterface Pin %p Interface %p Status %x\n", Pin, Interface, Status);
    return Status;
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
    PKSPIN Pin;
    PKSBASIC_HEADER Header;
    PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("KsGetPinFromIrp\n");

    /* get object header */
    ObjectHeader = (PKSIOBJECT_HEADER)IoStack->FileObject->FsContext2;

    if (!ObjectHeader)
        return NULL;

    Pin = (PKSPIN)ObjectHeader->ObjectType;
    Header = (PKSBASIC_HEADER)((ULONG_PTR)Pin - sizeof(KSBASIC_HEADER));

    /* sanity check */
    ASSERT(Header->Type == KsObjectTypePin);

    /* return object type */
    return Pin;
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

NTSTATUS
IKsPin_PrepareStreamHeader(
    IN IKsPinImpl * This,
    IN PKSISTREAM_POINTER StreamPointer)
{
    PKSSTREAM_HEADER Header;
    ULONG Length;

    /* grab new irp */
    StreamPointer->Irp = KsRemoveIrpFromCancelableQueue(&This->IrpList, &This->IrpListLock, KsListEntryHead, KsAcquireAndRemoveOnlySingleItem);
    if (!StreamPointer->Irp)
    {
        /* run out of mappings */
        DPRINT("OutOfMappings\n");
        return STATUS_DEVICE_NOT_READY;
    }

    InterlockedDecrement(&This->IrpCount);
    KsDecrementCountedWorker(This->PinWorker);

    /* get stream header */
    if (StreamPointer->Irp->RequestorMode == UserMode)
        Header = (PKSSTREAM_HEADER)StreamPointer->Irp->AssociatedIrp.SystemBuffer;
    else
        Header = (PKSSTREAM_HEADER)StreamPointer->Irp->UserBuffer;

    /* initialize stream pointer */
    StreamPointer->Callback = NULL;
    StreamPointer->Length = max(Header->DataUsed, Header->FrameExtent);
    StreamPointer->Next = NULL;
    StreamPointer->Offset = 0;
    StreamPointer->Pin = &This->Pin;
    StreamPointer->Data = Header->Data;

    StreamPointer->StreamPointer.Context = NULL;
    StreamPointer->StreamPointer.Pin = &This->Pin;
    StreamPointer->StreamPointer.StreamHeader = Header;

    if (This->Pin.Descriptor->PinDescriptor.DataFlow == KSPIN_DATAFLOW_IN)
        StreamPointer->StreamPointer.Offset = &StreamPointer->StreamPointer.OffsetIn;
    else
    StreamPointer->StreamPointer.Offset = &StreamPointer->StreamPointer.OffsetOut;

#ifndef _WIN64
    StreamPointer->StreamPointer.Offset->Alignment = 0;
#endif
    StreamPointer->StreamPointer.Offset->Count = 0;
    StreamPointer->StreamPointer.Offset->Data = NULL;
    StreamPointer->StreamPointer.Offset->Remaining = 0;

    ASSERT(StreamPointer->StreamPointer.Offset->Remaining == 0);

    //StreamPointer->Offset += StreamPointer->StreamPointer.Offset->Count;

    ASSERT(StreamPointer->Length > StreamPointer->Offset);
    ASSERT(StreamPointer->StreamPointer.StreamHeader);
    ASSERT(This->FrameSize);

    /* calculate length */
    /* TODO split into frames */
    Length = StreamPointer->Length;

    /* FIXME */
    ASSERT(Length);

#ifndef _WIN64
    StreamPointer->StreamPointer.Offset->Alignment = 0;
#endif
    StreamPointer->StreamPointer.Context = NULL;
    StreamPointer->StreamPointer.Pin = &This->Pin;
    StreamPointer->StreamPointer.Offset->Count = Length;
    StreamPointer->StreamPointer.Offset->Remaining = Length;
    StreamPointer->StreamPointer.Offset->Data = (PVOID)((ULONG_PTR)StreamPointer->Data + StreamPointer->Offset);
    StreamPointer->StreamPointer.StreamHeader->FrameExtent = Length;
    if (StreamPointer->StreamPointer.StreamHeader->DataUsed)
        StreamPointer->StreamPointer.StreamHeader->DataUsed = Length;

    StreamPointer->StreamPointer.StreamHeader->Data = StreamPointer->StreamPointer.Offset->Data;

    return STATUS_SUCCESS;
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
    IKsPinImpl * This;
    NTSTATUS Status;

    This = (IKsPinImpl*)CONTAINING_RECORD(Pin, IKsPinImpl, Pin);

    DPRINT("KsPinGetLeadingEdgeStreamPointer Pin %p State %x Count %lu Remaining %lu\n", Pin, State,
           This->LeadingEdgeStreamPointer.Length,
           This->LeadingEdgeStreamPointer.Offset);

    /* sanity check */
    ASSERT(State == KSSTREAM_POINTER_STATE_LOCKED);

    if (State == KSSTREAM_POINTER_STATE_LOCKED)
    {
        if (!This->LeadingEdgeStreamPointer.Irp || This->LeadingEdgeStreamPointer.StreamPointer.Offset->Remaining == 0)
        {
            Status = IKsPin_PrepareStreamHeader(This, &This->LeadingEdgeStreamPointer);
            if (!NT_SUCCESS(Status))
                return NULL;
        }

        DPRINT("KsPinGetLeadingEdgeStreamPointer NewOffset %lu TotalLength %lu\n", This->LeadingEdgeStreamPointer.Offset, This->LeadingEdgeStreamPointer.Length);
    }

     return &This->LeadingEdgeStreamPointer.StreamPointer;
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
    PKSISTREAM_POINTER Pointer = (PKSISTREAM_POINTER)CONTAINING_RECORD(StreamPointer, KSISTREAM_POINTER, StreamPointer);

    DPRINT("KsStreamPointerUnlock StreamPointer %pEject %lu\n", StreamPointer, Eject);

    Pointer->Irp = NULL;
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
    DPRINT("KsStreamPointerAdvanceOffsets InUsed %lu OutUsed %lu Eject %lu\n", InUsed, OutUsed, Eject);
    DbgBreakPoint();
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
    PKSISTREAM_POINTER Pointer = (PKSISTREAM_POINTER)CONTAINING_RECORD(StreamPointer, KSISTREAM_POINTER, StreamPointer);

    DPRINT("KsStreamPointerDelete %p\n", Pointer);
DbgBreakPoint();
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
    @implemented
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
    IKsPinImpl * This;
    PKSISTREAM_POINTER CurFrame;
    PKSISTREAM_POINTER NewFrame;
    ULONG_PTR RefCount;
    NTSTATUS Status;
    ULONG Size;

    DPRINT("KsStreamPointerClone StreamPointer %p CancelCallback %p ContextSize %p CloneStreamPointer %p\n", StreamPointer, CancelCallback, ContextSize, CloneStreamPointer);

    /* get stream pointer */
    CurFrame = (PKSISTREAM_POINTER)CONTAINING_RECORD(StreamPointer, KSISTREAM_POINTER, StreamPointer);

    /* calculate context size */
    Size = sizeof(KSISTREAM_POINTER) + ContextSize;

    /* allocate new stream pointer */
    NewFrame = (PKSISTREAM_POINTER)AllocateItem(NonPagedPool, Size);

    if (!NewFrame)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* get current irp stack location */
    RefCount = (ULONG_PTR)CurFrame->Irp->Tail.Overlay.DriverContext[0];

    /* increment reference count */
    RefCount++;
    CurFrame->Irp->Tail.Overlay.DriverContext[0] = (PVOID)RefCount;

    /* copy stream pointer */
    RtlMoveMemory(NewFrame, CurFrame, sizeof(KSISTREAM_POINTER));

    /* locate pin */
    This = (IKsPinImpl*)CONTAINING_RECORD(CurFrame->Pin, IKsPinImpl, Pin);

    /* prepare stream header in case required */
    if (CurFrame->StreamPointer.Offset->Remaining == 0)
    {
        Status = IKsPin_PrepareStreamHeader(This, NewFrame);
        if (!NT_SUCCESS(Status))
        {
            FreeItem(NewFrame);
            return STATUS_DEVICE_NOT_READY;
        }
    }

    if (ContextSize)
        NewFrame->StreamPointer.Context = (NewFrame + 1);


    if (This->Pin.Descriptor->PinDescriptor.DataFlow == KSPIN_DATAFLOW_IN)
        NewFrame->StreamPointer.Offset = &NewFrame->StreamPointer.OffsetIn;
    else
        NewFrame->StreamPointer.Offset = &NewFrame->StreamPointer.OffsetOut;



    NewFrame->StreamPointer.Pin = &This->Pin;

    ASSERT(NewFrame->StreamPointer.Pin);
    ASSERT(NewFrame->StreamPointer.Context);
    ASSERT(NewFrame->StreamPointer.Offset);
    ASSERT(NewFrame->StreamPointer.StreamHeader);

    /* store result */
    *CloneStreamPointer = &NewFrame->StreamPointer;

    DPRINT("KsStreamPointerClone CloneStreamPointer %p\n", *CloneStreamPointer);

    return STATUS_SUCCESS;
}

/*
    @implemented
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
    PKSISTREAM_POINTER CurFrame;
    IKsPinImpl * This;
    NTSTATUS Status;

    DPRINT("KsStreamPointerAdvanceOffsets StreamPointer %p InUsed %lu OutUsed %lu Eject %lu\n", StreamPointer, InUsed, OutUsed, Eject);

    /* get stream pointer */
    CurFrame = (PKSISTREAM_POINTER)CONTAINING_RECORD(StreamPointer, KSISTREAM_POINTER, StreamPointer);

    /* locate pin */
    This = (IKsPinImpl*)CONTAINING_RECORD(CurFrame->Pin, IKsPinImpl, Pin);

    /* TODO */
    ASSERT(InUsed == 0);
    ASSERT(Eject == 0);
    ASSERT(OutUsed);

    DPRINT("KsStreamPointerAdvanceOffsets Offset %lu Length %lu NewOffset %lu Remaining %lu LeadingEdge %p DataUsed %lu\n", CurFrame->Offset, CurFrame->Length, CurFrame->Offset + OutUsed,
CurFrame->StreamPointer.OffsetOut.Remaining, &This->LeadingEdgeStreamPointer.StreamPointer, CurFrame->StreamPointer.StreamHeader->DataUsed);
DbgBreakPoint();

    if (This->Pin.Descriptor->PinDescriptor.DataFlow == KSPIN_DATAFLOW_IN)
    {
        ASSERT(CurFrame->StreamPointer.OffsetIn.Remaining >= InUsed);
        CurFrame->StreamPointer.OffsetIn.Remaining -= InUsed;
        CurFrame->StreamPointer.OffsetIn.Data = (PVOID)((ULONG_PTR)CurFrame->StreamPointer.OffsetIn.Data + InUsed);
    }
    else
    {
        if (!CurFrame->StreamPointer.OffsetOut.Remaining)
        {
            Status = IKsPin_PrepareStreamHeader(This, CurFrame);
            if (!NT_SUCCESS(Status))
            {
                return STATUS_DEVICE_NOT_READY;
            }
        }
        else
        {
            ASSERT(CurFrame->StreamPointer.OffsetOut.Remaining >= OutUsed);
            CurFrame->StreamPointer.OffsetOut.Remaining -= OutUsed;
            CurFrame->StreamPointer.OffsetOut.Data = (PVOID)((ULONG_PTR)CurFrame->StreamPointer.OffsetOut.Data + OutUsed);
        }
    }

    return STATUS_SUCCESS;
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
    DbgBreakPoint();
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
    PKSISTREAM_POINTER Pointer;

    /* get stream pointer */
    Pointer = (PKSISTREAM_POINTER)CONTAINING_RECORD(StreamPointer, KSISTREAM_POINTER, StreamPointer);

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
    PKSISTREAM_POINTER Pointer;

    /* get stream pointer */
    Pointer = (PKSISTREAM_POINTER)CONTAINING_RECORD(StreamPointer, KSISTREAM_POINTER, StreamPointer);

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

    if (!This->ClonedStreamPointer)
        return NULL;

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
    PKSISTREAM_POINTER Pointer;

    DPRINT("KsStreamPointerGetNextClone\n");
DbgBreakPoint();
    /* get stream pointer */
    Pointer = (PKSISTREAM_POINTER)CONTAINING_RECORD(StreamPointer, KSISTREAM_POINTER, StreamPointer);

    /* is there a another cloned stream pointer */
    if (!Pointer->Next)
        return NULL;

    /* return next stream pointer */
    return &Pointer->Next->StreamPointer;
}

VOID
NTAPI
IKsPin_PinCentricWorker(
    IN PVOID Parameter)
{
    NTSTATUS Status;
    IKsPinImpl * This = (IKsPinImpl*)Parameter;

    DPRINT("IKsPin_PinCentricWorker\n");

    /* sanity checks */
    ASSERT(This);
    ASSERT(This->Pin.Descriptor);
    ASSERT(This->Pin.Descriptor->Dispatch);
    ASSERT(This->Pin.Descriptor->Dispatch->Process);
    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
    ASSERT(!(This->Pin.Descriptor->Flags & KSPIN_FLAG_DISPATCH_LEVEL_PROCESSING));
    ASSERT(!(This->Pin.Descriptor->Flags & KSPIN_FLAG_GENERATE_MAPPINGS));

    do
    {
        DPRINT("IKsPin_PinCentricWorker calling Pin Process Routine\n");

        Status = This->Pin.Descriptor->Dispatch->Process(&This->Pin);
        DPRINT("IKsPin_PinCentricWorker Status %lx, Offset %lu Length %lu\n", Status,
               This->LeadingEdgeStreamPointer.Offset,
               This->LeadingEdgeStreamPointer.Length);
        break;

    }while(This->IrpCount);
}


NTSTATUS
NTAPI
IKsPin_DispatchKsStream(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    IKsPinImpl * This)
{
    PKSPROCESSPIN_INDEXENTRY ProcessPinIndex;
    PKSSTREAM_HEADER Header;
    ULONG NumHeaders;
    PKSFILTER Filter;
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("IKsPin_DispatchKsStream\n");

    /* FIXME handle reset states */
    ASSERT(This->Pin.ResetState == KSRESET_END);

    /* get current stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* probe stream pointer */
    if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_KS_WRITE_STREAM)
        Status = KsProbeStreamIrp(Irp, KSSTREAM_WRITE | KSPROBE_ALLOCATEMDL | KSPROBE_PROBEANDLOCK | KSPROBE_SYSTEMADDRESS, This->Pin.StreamHeaderSize);
    else
        Status = KsProbeStreamIrp(Irp, KSSTREAM_READ  | KSPROBE_ALLOCATEMDL | KSPROBE_PROBEANDLOCK | KSPROBE_SYSTEMADDRESS, This->Pin.StreamHeaderSize);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("KsProbeStreamIrp failed with %x\n", Status);

        Irp->IoStatus.Status = Status;
        CompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }

    if (Irp->RequestorMode == UserMode)
        Header = (PKSSTREAM_HEADER)Irp->AssociatedIrp.SystemBuffer;
    else
        Header = (PKSSTREAM_HEADER)Irp->UserBuffer;

    if (!Header)
    {
        DPRINT("NoHeader Canceling Irp %p\n", Irp);
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        CompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;
    }

    /* calculate num headers */
    NumHeaders = IoStack->Parameters.DeviceIoControl.OutputBufferLength / Header->Size;

    /* assume headers of same length */
    ASSERT(IoStack->Parameters.DeviceIoControl.OutputBufferLength % Header->Size == 0);

    /* FIXME support multiple stream headers */
    ASSERT(NumHeaders == 1);

    if (Irp->RequestorMode == UserMode)
    {
        /* prepare header */
        ASSERT(Irp->MdlAddress);
        Header->Data = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);

        if (!Header->Data)
        {
            DPRINT("NoHeader->Data Canceling Irp %p\n", Irp);
            Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            CompleteRequest(Irp, IO_NO_INCREMENT);
            return Status;
        }

    }



    if (This->Pin.Descriptor->Dispatch->Process)
    {
        /* it is a pin centric avstream */

        /* mark irp as pending */
        IoMarkIrpPending(Irp);

        /* add irp to cancelable queue */
        KsAddIrpToCancelableQueue(&This->IrpList, &This->IrpListLock, Irp, KsListEntryTail, NULL /* FIXME */);

        /* sanity checks */
        ASSERT(!(This->Pin.Descriptor->Flags & KSPIN_FLAG_DISPATCH_LEVEL_PROCESSING));
        ASSERT(This->PinWorker);

        InterlockedIncrement(&This->IrpCount);

        DPRINT("IKsPin_DispatchKsStream IrpCount %lu\n", This->IrpCount);

        /* start the processing loop */
        KsIncrementCountedWorker(This->PinWorker);

        Status = STATUS_PENDING;
    }
    else
    {
        /* filter-centric avstream */
        ASSERT(This->Filter);

        ProcessPinIndex = This->Filter->lpVtbl->GetProcessDispatch(This->Filter);
        Filter = This->Filter->lpVtbl->GetStruct(This->Filter);

        ASSERT(ProcessPinIndex);
        ASSERT(Filter);
        ASSERT(Filter->Descriptor);
        ASSERT(Filter->Descriptor->Dispatch);

        if (!Filter->Descriptor->Dispatch->Process)
        {
            /* invalid device request */
            DPRINT("Filter Centric Processing No Process Routine\n");
            Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
            CompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_UNSUCCESSFUL;
        }

        /* mark irp as pending */
        IoMarkIrpPending(Irp);

        /* add irp to cancelable queue */
        KsAddIrpToCancelableQueue(&This->IrpList, &This->IrpListLock, Irp, KsListEntryTail, NULL /* FIXME */);

        Status = Filter->Descriptor->Dispatch->Process(Filter, ProcessPinIndex);

        DPRINT("IKsPin_DispatchKsStream FilterCentric: Status %lx \n", Status);

    }

    return Status;
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
    NTSTATUS Status;
    UNICODE_STRING GuidString;
    PKSPROPERTY Property;
    ULONG SetCount = 0;

    /* get current irp stack */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* sanity check */
    ASSERT(IoStack->FileObject);
    ASSERT(IoStack->FileObject->FsContext2);

    /* get the object header */
    ObjectHeader = (PKSIOBJECT_HEADER)IoStack->FileObject->FsContext2;

    /* locate ks pin implemention from KSPIN offset */
    This = (IKsPinImpl*)CONTAINING_RECORD(ObjectHeader->ObjectType, IKsPinImpl, Pin);

    /* current irp stack */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_KS_READ_STREAM ||
        IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_KS_WRITE_STREAM)
    {
        /* handle ks stream packets */
        return IKsPin_DispatchKsStream(DeviceObject, Irp, This);
    }

    /* get property from input buffer */
    Property = (PKSPROPERTY)IoStack->Parameters.DeviceIoControl.Type3InputBuffer;

    /* sanity check */
    ASSERT(IoStack->Parameters.DeviceIoControl.InputBufferLength >= sizeof(KSIDENTIFIER));
    ASSERT(This->Pin.Descriptor->AutomationTable);

    RtlStringFromGUID(&Property->Set, &GuidString);
    DPRINT("IKsPin_DispatchDeviceIoControl property Set |%S| Id %u Flags %x\n", GuidString.Buffer, Property->Id, Property->Flags);
    RtlFreeUnicodeString(&GuidString);


    if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_KS_METHOD)
    {
        const KSMETHOD_SET *MethodSet = NULL;
        ULONG MethodItemSize = 0;

        /* check if the driver supports method sets */
        if (This->Pin.Descriptor->AutomationTable->MethodSetsCount)
        {
            SetCount = This->Pin.Descriptor->AutomationTable->MethodSetsCount;
            MethodSet = This->Pin.Descriptor->AutomationTable->MethodSets;
            MethodItemSize = This->Pin.Descriptor->AutomationTable->MethodItemSize;
        }

        /* call method set handler */
        Status = KspMethodHandlerWithAllocator(Irp, SetCount, MethodSet, NULL, MethodItemSize);
    }
    else if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_KS_PROPERTY)
    {
        const KSPROPERTY_SET *PropertySet = NULL;
        ULONG PropertyItemSize = 0;

        /* check if the driver supports method sets */
        if (This->Pin.Descriptor->AutomationTable->PropertySetsCount)
        {
            SetCount = This->Pin.Descriptor->AutomationTable->PropertySetsCount;
            PropertySet = This->Pin.Descriptor->AutomationTable->PropertySets;
            PropertyItemSize = This->Pin.Descriptor->AutomationTable->PropertyItemSize;
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
                                    This->Pin.Descriptor->AutomationTable->EventSetsCount,
                                    (PKSEVENT_SET)This->Pin.Descriptor->AutomationTable->EventSets,
                                    &This->BasicHeader.EventList,
                                    KSEVENTS_SPINLOCK,
                                    (PVOID)&This->BasicHeader.EventListLock,
                                    NULL,
                                    This->Pin.Descriptor->AutomationTable->EventItemSize);
        }
        else
        {
            /* disable event handler */
            Status = KsDisableEvent(Irp, &This->BasicHeader.EventList, KSEVENTS_SPINLOCK, &This->BasicHeader.EventListLock);
        }
    }

    RtlStringFromGUID(&Property->Set, &GuidString);
    DPRINT("IKsPin_DispatchDeviceIoControl property Set |%S| Id %u Flags %x Status %lx ResultLength %lu\n", GuidString.Buffer, Property->Id, Property->Flags, Status, Irp->IoStatus.Information);
    RtlFreeUnicodeString(&GuidString);

    if (Status != STATUS_PENDING)
    {
        Irp->IoStatus.Status = Status;
        CompleteRequest(Irp, IO_NO_INCREMENT);
    }

    /* done */
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

    if (This->Pin.Descriptor->Dispatch->Close)
    {
        /* call pin close routine */
        Status = This->Pin.Descriptor->Dispatch->Close(&This->Pin, Irp);

        if (!NT_SUCCESS(Status))
        {
            /* abort closing */
            Irp->IoStatus.Status = Status;
            CompleteRequest(Irp, IO_NO_INCREMENT);
            return Status;
        }

        /* remove pin from filter pin list and decrement reference count */
        IKsFilter_RemovePin(This->Filter->lpVtbl->GetStruct(This->Filter), &This->Pin);

        if (Status != STATUS_PENDING)
        {
            Irp->IoStatus.Status = Status;
            CompleteRequest(Irp, IO_NO_INCREMENT);
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
    CompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
IKsPin_DispatchCreateClock(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PKSPIN Pin;
    NTSTATUS Status = STATUS_SUCCESS;
    IKsPinImpl * This;
    KSRESOLUTION Resolution;
    PKSRESOLUTION pResolution = NULL;
    PKSOBJECT_CREATE_ITEM CreateItem;

    DPRINT("IKsPin_DispatchCreateClock\n");

    /* get the create item */
    CreateItem = KSCREATE_ITEM_IRP_STORAGE(Irp);

    /* sanity check */
    ASSERT(CreateItem);

    /* get the pin object */
    Pin = (PKSPIN)CreateItem->Context;

    /* sanity check */
    ASSERT(Pin);

    /* locate ks pin implemention fro KSPIN offset */
    This = (IKsPinImpl*)CONTAINING_RECORD(Pin, IKsPinImpl, Pin);

    /* sanity check */
    ASSERT(This->BasicHeader.Type == KsObjectTypePin);
    ASSERT(This->BasicHeader.ControlMutex);

    /* acquire control mutex */
    KsAcquireControl(Pin);

    if ((This->Pin.Descriptor->PinDescriptor.Communication != KSPIN_COMMUNICATION_NONE &&
        This->Pin.Descriptor->Dispatch) ||
        (This->Pin.Descriptor->Flags & KSPIN_FLAG_IMPLEMENT_CLOCK))
    {
        if (!This->DefaultClock)
        {
            if (This->Pin.Descriptor->Dispatch && This->Pin.Descriptor->Dispatch->Clock)
            {
                if (This->Pin.Descriptor->Dispatch->Clock->Resolution)
                {
                   This->Pin.Descriptor->Dispatch->Clock->Resolution(&This->Pin, &Resolution);
                   pResolution = &Resolution;
                }

                Status = KsAllocateDefaultClockEx(&This->DefaultClock,
                                                  (PVOID)&This->Pin,
                                                  (PFNKSSETTIMER)This->Pin.Descriptor->Dispatch->Clock->SetTimer,
                                                  (PFNKSCANCELTIMER)This->Pin.Descriptor->Dispatch->Clock->CancelTimer,
                                                  (PFNKSCORRELATEDTIME)This->Pin.Descriptor->Dispatch->Clock->CorrelatedTime,
                                                  pResolution,
                                                  0);
            }
            else
            {
                Status = KsAllocateDefaultClockEx(&This->DefaultClock, (PVOID)&This->Pin, NULL, NULL, NULL, NULL, 0);
            }
        }

        if (NT_SUCCESS(Status))
        {
            Status = KsCreateDefaultClock(Irp, This->DefaultClock);
        }
    }

    DPRINT("IKsPin_DispatchCreateClock %lx\n", Status);

    /* release control mutex */
    KsReleaseControl(Pin);

    /* done */
    Irp->IoStatus.Status = Status;
    CompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

NTSTATUS
NTAPI
IKsPin_DispatchCreateNode(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    UNIMPLEMENTED;

    Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
    CompleteRequest(Irp, IO_NO_INCREMENT);
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
    PKSDATAFORMAT DataFormat;
    PKSBASIC_HEADER BasicHeader;
    ULONG Index;
    ULONG FrameSize = 0;
    ULONG NumFrames = 0;
    KSAUTOMATION_TABLE AutomationTable;

    /* sanity checks */
    ASSERT(Descriptor->Dispatch);

    DPRINT("KspCreatePin PinId %lu Flags %x\n", Connect->PinId, Descriptor->Flags);

//Output Pin: KSPIN_FLAG_PROCESS_IN_RUN_STATE_ONLY
//Input Pin: KSPIN_FLAG_FIXED_FORMAT|KSPIN_FLAG_DO_NOT_USE_STANDARD_TRANSPORT|KSPIN_FLAG_FRAMES_NOT_REQUIRED_FOR_PROCESSING

    DPRINT("KspCreatePin Dataflow %lu\n", Descriptor->PinDescriptor.DataFlow);
    DPRINT("KspCreatePin Communication %lu\n", Descriptor->PinDescriptor.Communication);
    if (Descriptor->AllocatorFraming)
    {
        DPRINT("KspCreatePin CountItems %lu\n", Descriptor->AllocatorFraming->CountItems);
        DPRINT("KspCreatePin PinFlags %lx\n", Descriptor->AllocatorFraming->PinFlags);
        DPRINT("KspCreatePin OutputCompression RatioNumerator %lu RatioDenominator  %lu RatioConstantMargin %lu\n", Descriptor->AllocatorFraming->OutputCompression.RatioNumerator,
               Descriptor->AllocatorFraming->OutputCompression.RatioDenominator, Descriptor->AllocatorFraming->OutputCompression.RatioConstantMargin);
        DPRINT("KspCreatePin PinWeight %lx\n", Descriptor->AllocatorFraming->PinWeight);

        for(Index = 0; Index < Descriptor->AllocatorFraming->CountItems; Index++)
        {
            DPRINT("KspCreatePin Index %lu MemoryFlags %lx\n", Index, Descriptor->AllocatorFraming->FramingItem[Index].MemoryFlags);
            DPRINT("KspCreatePin Index %lu BusFlags %lx\n", Index, Descriptor->AllocatorFraming->FramingItem[Index].BusFlags);
            DPRINT("KspCreatePin Index %lu Flags %lx\n", Index, Descriptor->AllocatorFraming->FramingItem[Index].Flags);
            DPRINT("KspCreatePin Index %lu Frames %lu\n", Index, Descriptor->AllocatorFraming->FramingItem[Index].Frames);
            DPRINT("KspCreatePin Index %lu FileAlignment %lx\n", Index, Descriptor->AllocatorFraming->FramingItem[Index].FileAlignment);
            DPRINT("KspCreatePin Index %lu MemoryTypeWeight %lx\n", Index, Descriptor->AllocatorFraming->FramingItem[Index].MemoryTypeWeight);
            DPRINT("KspCreatePin Index %lu PhysicalRange MinFrameSize %lu MaxFrameSize %lu Stepping %lu\n", Index, Descriptor->AllocatorFraming->FramingItem[Index].PhysicalRange.MinFrameSize,
                   Descriptor->AllocatorFraming->FramingItem[Index].PhysicalRange.MaxFrameSize,
                   Descriptor->AllocatorFraming->FramingItem[Index].PhysicalRange.Stepping);

            DPRINT("KspCreatePin Index %lu FramingRange  MinFrameSize %lu MaxFrameSize %lu Stepping %lu InPlaceWeight %lu NotInPlaceWeight %lu\n",
                   Index,
                   Descriptor->AllocatorFraming->FramingItem[Index].FramingRange.Range.MinFrameSize,
                   Descriptor->AllocatorFraming->FramingItem[Index].FramingRange.Range.MaxFrameSize,
                   Descriptor->AllocatorFraming->FramingItem[Index].FramingRange.Range.Stepping,
                   Descriptor->AllocatorFraming->FramingItem[Index].FramingRange.InPlaceWeight,
                   Descriptor->AllocatorFraming->FramingItem[Index].FramingRange.NotInPlaceWeight);

           FrameSize = Descriptor->AllocatorFraming->FramingItem[Index].FramingRange.Range.MaxFrameSize;
           NumFrames = Descriptor->AllocatorFraming->FramingItem[Index].Frames;
        }
    }

     for (Index = 0; Index < Descriptor->PinDescriptor.DataRangesCount; Index++)
     {
         UNICODE_STRING GuidString;
         /* convert the guid to string */
         RtlStringFromGUID(&Descriptor->PinDescriptor.DataRanges[Index]->MajorFormat, &GuidString);
         DPRINT("Index %lu MajorFormat %S\n", Index, GuidString.Buffer);
         RtlStringFromGUID(&Descriptor->PinDescriptor.DataRanges[Index]->SubFormat, &GuidString);
         DPRINT("Index %lu SubFormat %S\n", Index, GuidString.Buffer);
         RtlStringFromGUID(&Descriptor->PinDescriptor.DataRanges[Index]->Specifier, &GuidString);
         DPRINT("Index %lu Specifier %S\n", Index, GuidString.Buffer);
         RtlStringFromGUID(&Descriptor->PinDescriptor.DataRanges[Index]->Specifier, &GuidString);
         DPRINT("Index %lu FormatSize %lu Flags %lu SampleSize %lu Reserved %lu KSDATAFORMAT %lu\n", Index,
                Descriptor->PinDescriptor.DataRanges[Index]->FormatSize, Descriptor->PinDescriptor.DataRanges[Index]->Flags, Descriptor->PinDescriptor.DataRanges[Index]->SampleSize, Descriptor->PinDescriptor.DataRanges[Index]->Reserved, sizeof(KSDATAFORMAT));

         if (IsEqualGUIDAligned(&Descriptor->PinDescriptor.DataRanges[Index]->SubFormat, &KSDATAFORMAT_SUBTYPE_BDA_MPEG2_TRANSPORT))
         {
#if !defined(NDEBUG)
             PKS_DATARANGE_BDA_TRANSPORT Transport = (PKS_DATARANGE_BDA_TRANSPORT)&Descriptor->PinDescriptor.DataRanges[Index];
             DPRINT("KSDATAFORMAT_SUBTYPE_BDA_MPEG2_TRANSPORT AvgTimePerFrame %I64u ulcbPhyiscalFrame %lu ulcbPhyiscalFrameAlignment %lu ulcbPhyiscalPacket %lu\n", Transport->BdaTransportInfo.AvgTimePerFrame, Transport->BdaTransportInfo.ulcbPhyiscalFrame,
                    Transport->BdaTransportInfo.ulcbPhyiscalFrameAlignment, Transport->BdaTransportInfo.ulcbPhyiscalPacket);
#endif
         }
    }
    if (!FrameSize)
    {
        /* default to 50 * 188 (MPEG2 TS packet size) */
        FrameSize = 9400;
    }

    if (!NumFrames)
    {
        NumFrames = 8;
    }

    /* get current irp stack */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* get device extension */
    DeviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;

    /* get ks device interface */
    Device = (IKsDevice*)&DeviceExtension->DeviceHeader->BasicHeader.OuterUnknown;

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
        DPRINT("KspCreatePin OutOfMemory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* initialize basic header */
    This->BasicHeader.KsDevice = KsDevice;
    This->BasicHeader.Type = KsObjectTypePin;
    This->BasicHeader.Parent.KsFilter = Filter->lpVtbl->GetStruct(Filter);
    This->BasicHeader.OuterUnknown = (PUNKNOWN)&vt_IKsPin;
    InitializeListHead(&This->BasicHeader.EventList);
    KeInitializeSpinLock(&This->BasicHeader.EventListLock);

    ASSERT(This->BasicHeader.Parent.KsFilter);

    BasicHeader = (PKSBASIC_HEADER)((ULONG_PTR)This->BasicHeader.Parent.KsFilter - sizeof(KSBASIC_HEADER));

    This->BasicHeader.ControlMutex = BasicHeader->ControlMutex;
    ASSERT(This->BasicHeader.ControlMutex);

    InitializeListHead(&This->BasicHeader.EventList);
    KeInitializeSpinLock(&This->BasicHeader.EventListLock);

    /* initialize pin */
    This->FrameSize = FrameSize;
    This->NumFrames = NumFrames;
    This->lpVtblReferenceClock = &vt_ReferenceClock;
    This->ref = 1;
    This->FileObject = IoStack->FileObject;
    This->Filter = Filter;
    KeInitializeMutex(&This->ProcessingMutex, 0);
    InitializeListHead(&This->IrpList);
    KeInitializeSpinLock(&This->IrpListLock);

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
    Device->lpVtbl->InitializeObjectBag(Device, This->Pin.Bag, NULL);

    /* allocate pin descriptor */
    This->Pin.Descriptor = AllocateItem(NonPagedPool, sizeof(KSPIN_DESCRIPTOR_EX));
    if (!This->Pin.Descriptor)
    {
        /* not enough memory */
        KsFreeObjectBag(This->Pin.Bag);
        FreeItem(This);
        FreeItem(CreateItem);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* copy pin descriptor */
    RtlMoveMemory((PVOID)This->Pin.Descriptor, Descriptor, sizeof(KSPIN_DESCRIPTOR_EX));

    /* initialize automation table */
    RtlZeroMemory(&AutomationTable, sizeof(KSAUTOMATION_TABLE));

    AutomationTable.PropertyItemSize = sizeof(KSPROPERTY_ITEM);
    AutomationTable.PropertySets = PinPropertySet;
    AutomationTable.PropertySetsCount = sizeof(PinPropertySet) / sizeof(KSPROPERTY_SET);

    /* merge in pin property sets */
    Status = KsMergeAutomationTables((PKSAUTOMATION_TABLE*)&This->Pin.Descriptor->AutomationTable, (PKSAUTOMATION_TABLE)Descriptor->AutomationTable, &AutomationTable, This->Pin.Bag);

    if (!NT_SUCCESS(Status))
    {
        /* not enough memory */
        KsFreeObjectBag(This->Pin.Bag);
        FreeItem(This);
        FreeItem(CreateItem);
        return Status;
    }

    /* get format */
    DataFormat = (PKSDATAFORMAT)(Connect + 1);

    /* initialize pin descriptor */
    This->Pin.Context = NULL;
    This->Pin.Id = Connect->PinId;
    This->Pin.Communication = Descriptor->PinDescriptor.Communication;
    This->Pin.ConnectionIsExternal = FALSE; //FIXME
    RtlMoveMemory(&This->Pin.ConnectionInterface, &Connect->Interface, sizeof(KSPIN_INTERFACE));
    RtlMoveMemory(&This->Pin.ConnectionMedium, &Connect->Medium, sizeof(KSPIN_MEDIUM));
    RtlMoveMemory(&This->Pin.ConnectionPriority, &Connect->Priority, sizeof(KSPRIORITY));

    /* allocate format */
    Status = _KsEdit(This->Pin.Bag, (PVOID*)&This->Pin.ConnectionFormat, DataFormat->FormatSize, DataFormat->FormatSize, 0);
    if (!NT_SUCCESS(Status))
    {
        /* failed to allocate format */
        KsFreeObjectBag((KSOBJECT_BAG)This->Pin.Bag);
        FreeItem(This);
        FreeItem(CreateItem);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* copy format */
    RtlMoveMemory((PVOID)This->Pin.ConnectionFormat, DataFormat, DataFormat->FormatSize);

    This->Pin.AttributeList = NULL; //FIXME
    This->Pin.StreamHeaderSize = sizeof(KSSTREAM_HEADER);
    This->Pin.DataFlow = Descriptor->PinDescriptor.DataFlow;
    This->Pin.DeviceState = KSSTATE_STOP;
    This->Pin.ResetState = KSRESET_END;
    This->Pin.ClientState = KSSTATE_STOP;

    /* intialize allocator create item */
    CreateItem[0].Context = (PVOID)&This->Pin;
    CreateItem[0].Create = IKsPin_DispatchCreateAllocator;
    CreateItem[0].Flags = KSCREATE_ITEM_FREEONSTOP;
    RtlInitUnicodeString(&CreateItem[0].ObjectClass, KSSTRING_Allocator);

    /* intialize clock create item */
    CreateItem[1].Context = (PVOID)&This->Pin;
    CreateItem[1].Create = IKsPin_DispatchCreateClock;
    CreateItem[1].Flags = KSCREATE_ITEM_FREEONSTOP;
    RtlInitUnicodeString(&CreateItem[1].ObjectClass, KSSTRING_Clock);

    /* intialize topology node create item */
    CreateItem[2].Context = (PVOID)&This->Pin;
    CreateItem[2].Create = IKsPin_DispatchCreateNode;
    CreateItem[2].Flags = KSCREATE_ITEM_FREEONSTOP;
    RtlInitUnicodeString(&CreateItem[2].ObjectClass, KSSTRING_TopologyNode);

    /* now allocate object header */
    Status = KsAllocateObjectHeader((KSOBJECT_HEADER*)&This->ObjectHeader, 3, CreateItem, Irp, &PinDispatchTable);
    if (!NT_SUCCESS(Status))
    {
        /* failed to create object header */
        DPRINT("KspCreatePin KsAllocateObjectHeader failed %lx\n", Status);
        KsFreeObjectBag((KSOBJECT_BAG)This->Pin.Bag);
        FreeItem(This);
        FreeItem(CreateItem);

        /* return failure code */
        return Status;
    }

     /* add extra info to object header */
    This->ObjectHeader->Type = KsObjectTypePin;
    This->ObjectHeader->Unknown = (PUNKNOWN)&This->BasicHeader.OuterUnknown;
    This->ObjectHeader->ObjectType = (PVOID)&This->Pin;

    if (!Descriptor->Dispatch || !Descriptor->Dispatch->Process)
    {
        /* the pin is part of filter-centric processing filter
         * add process pin to filter
         */
        This->ProcessPin.BytesAvailable = 0;
        This->ProcessPin.BytesUsed = 0;
        This->ProcessPin.CopySource = NULL;
        This->ProcessPin.Data = NULL;
        This->ProcessPin.DelegateBranch = NULL;
        This->ProcessPin.Flags = 0;
        This->ProcessPin.InPlaceCounterpart = NULL;
        This->ProcessPin.Pin = &This->Pin;
        This->ProcessPin.StreamPointer = (PKSSTREAM_POINTER)&This->LeadingEdgeStreamPointer.StreamPointer;
        This->ProcessPin.Terminate = FALSE;

        Status = Filter->lpVtbl->AddProcessPin(Filter, &This->ProcessPin);
        DPRINT("KspCreatePin AddProcessPin %lx\n", Status);

        if (!NT_SUCCESS(Status))
        {
            /* failed to add process pin */
            KsFreeObjectBag((KSOBJECT_BAG)This->Pin.Bag);
            KsFreeObjectHeader(&This->ObjectHeader);
            FreeItem(This);
            FreeItem(CreateItem);
            /* return failure code */
            return Status;
        }
    }
    else if (Descriptor->Dispatch && Descriptor->Dispatch->Process)
    {
        /* pin centric processing filter */

        /* initialize work item */
        ExInitializeWorkItem(&This->PinWorkQueueItem, IKsPin_PinCentricWorker, (PVOID)This);

        /* allocate counted work item */
        Status = KsRegisterCountedWorker(HyperCriticalWorkQueue, &This->PinWorkQueueItem, &This->PinWorker);

        if (!NT_SUCCESS(Status))
        {
            DPRINT("Failed to register Worker %lx\n", Status);
            KsFreeObjectBag((KSOBJECT_BAG)This->Pin.Bag);
            KsFreeObjectHeader(&This->ObjectHeader);
            FreeItem(This);
            FreeItem(CreateItem);
            return Status;
        }

        if (This->Pin.Descriptor->PinDescriptor.DataFlow == KSPIN_DATAFLOW_IN)
            This->LeadingEdgeStreamPointer.StreamPointer.Offset = &This->LeadingEdgeStreamPointer.StreamPointer.OffsetIn;
        else
            This->LeadingEdgeStreamPointer.StreamPointer.Offset = &This->LeadingEdgeStreamPointer.StreamPointer.OffsetOut;


        KeInitializeEvent(&This->FrameComplete, NotificationEvent, FALSE);

    }

    /* FIXME add pin instance to filter instance */
    IKsFilter_AddPin(Filter->lpVtbl->GetStruct(Filter), &This->Pin);

    if (Descriptor->Dispatch && Descriptor->Dispatch->SetDataFormat)
    {
        Status = Descriptor->Dispatch->SetDataFormat(&This->Pin, NULL, NULL, This->Pin.ConnectionFormat, NULL);
        DPRINT("KspCreatePin SetDataFormat %lx\n", Status);
    }


    /* does the driver have a pin dispatch */
    if (Descriptor->Dispatch && Descriptor->Dispatch->Create)
    {
        /*  now inform the driver to create a new pin */
        Status = Descriptor->Dispatch->Create(&This->Pin, Irp);
        DPRINT("KspCreatePin DispatchCreate %lx\n", Status);
    }


    DPRINT("KspCreatePin Status %lx KsDevice %p\n", Status, KsDevice);

    if (!NT_SUCCESS(Status) && Status != STATUS_PENDING)
    {
        /* failed to create pin, release resources */
        IKsFilter_RemovePin(Filter->lpVtbl->GetStruct(Filter), &This->Pin);
        KsFreeObjectHeader((KSOBJECT_HEADER)This->ObjectHeader);
        KsFreeObjectBag((KSOBJECT_BAG)This->Pin.Bag);
        FreeItem(This);

        /* return failure code */
        return Status;
    }

    return Status;
}
