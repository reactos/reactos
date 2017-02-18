/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/ksfilter/ks/clocks.c
 * PURPOSE:         KS Clocks functions
 * PROGRAMMER:      Johannes Anderwald
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>

typedef struct
{
    LONGLONG Time;
    KSPIN_LOCK TimeLock;
    KSSTATE State;
    KTIMER Timer;
    LONG ReferenceCount;

    PVOID Context;
    PFNKSSETTIMER SetTimer;
    PFNKSCANCELTIMER CancelTimer;
    PFNKSCORRELATEDTIME CorrelatedTime;
    LONGLONG Granularity;
    LONGLONG Error;
    ULONG Flags;

}KSIDEFAULTCLOCK, *PKSIDEFAULTCLOCK;

typedef struct
{
    LONG ref;
    PKSCLOCK_CREATE ClockCreate;
    PKSIDEFAULTCLOCK DefaultClock;
    PKSIOBJECT_HEADER ObjectHeader;
}KSICLOCK, *PKSICLOCK;

NTSTATUS NTAPI ClockPropertyTime(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);
NTSTATUS NTAPI ClockPropertyPhysicalTime(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);
NTSTATUS NTAPI ClockPropertyCorrelatedTime(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);
NTSTATUS NTAPI ClockPropertyCorrelatedPhysicalTime(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);
NTSTATUS NTAPI ClockPropertyResolution(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);
NTSTATUS NTAPI ClockPropertyState(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);
NTSTATUS NTAPI ClockPropertyFunctionTable(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);

DEFINE_KSPROPERTY_CLOCKSET(ClockPropertyTable, ClockPropertyTime, ClockPropertyPhysicalTime, ClockPropertyCorrelatedTime, ClockPropertyCorrelatedPhysicalTime, ClockPropertyResolution, ClockPropertyState, ClockPropertyFunctionTable);

KSPROPERTY_SET ClockPropertySet[] =
{
    {
        &KSPROPSETID_Clock,
        sizeof(ClockPropertyTable) / sizeof(KSPROPERTY_ITEM),
        (const KSPROPERTY_ITEM*)&ClockPropertyTable,
        0,
        NULL
    }
};

LONGLONG
FASTCALL
ClockGetPhysicalTime(
    IN PFILE_OBJECT FileObject)
{
    UNIMPLEMENTED;
    return 0;
}

LONGLONG
FASTCALL
ClockGetCorrelatedTime(
    IN PFILE_OBJECT FileObject,
    OUT PLONGLONG SystemTime)
{
    UNIMPLEMENTED;
    return 0;
}

LONGLONG
FASTCALL
ClockGetTime(
    IN PFILE_OBJECT FileObject)
{
    UNIMPLEMENTED;
    return 0;
}

LONGLONG
FASTCALL
ClockGetCorrelatedPhysicalTime(
    IN PFILE_OBJECT FileObject,
    OUT PLONGLONG SystemTime)
{
    UNIMPLEMENTED;
    return 0;
}

NTSTATUS
NTAPI
ClockPropertyTime(
    IN PIRP Irp,
    IN PKSIDENTIFIER Request,
    IN OUT PVOID Data)
{
    PLONGLONG Time = (PLONGLONG)Data;
    PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("ClockPropertyTime\n");

    *Time = ClockGetTime(IoStack->FileObject);

    Irp->IoStatus.Information = sizeof(LONGLONG);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
ClockPropertyPhysicalTime(
    IN PIRP Irp,
    IN PKSIDENTIFIER Request,
    IN OUT PVOID Data)
{
    PLONGLONG Time = (PLONGLONG)Data;
    PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("ClockPropertyPhysicalTime\n");

    *Time = ClockGetPhysicalTime(IoStack->FileObject);

    Irp->IoStatus.Information = sizeof(LONGLONG);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
ClockPropertyCorrelatedTime(
    IN PIRP Irp,
    IN PKSIDENTIFIER Request,
    IN OUT PVOID Data)
{
    PKSCORRELATED_TIME Time = (PKSCORRELATED_TIME)Data;
    PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("ClockPropertyCorrelatedTime\n");

    Time->Time = ClockGetCorrelatedTime(IoStack->FileObject, &Time->SystemTime);

    Irp->IoStatus.Information = sizeof(KSCORRELATED_TIME);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
ClockPropertyCorrelatedPhysicalTime(
    IN PIRP Irp,
    IN PKSIDENTIFIER Request,
    IN OUT PVOID Data)
{
    PKSCORRELATED_TIME Time = (PKSCORRELATED_TIME)Data;
    PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("ClockPropertyCorrelatedPhysicalTime\n");

    Time->Time = ClockGetCorrelatedPhysicalTime(IoStack->FileObject, &Time->SystemTime);

    Irp->IoStatus.Information = sizeof(KSCORRELATED_TIME);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
ClockPropertyResolution(
    IN PIRP Irp,
    IN PKSIDENTIFIER Request,
    IN OUT PVOID Data)
{
    PKSICLOCK Clock;
    PKSIOBJECT_HEADER ObjectHeader;
    PIO_STACK_LOCATION IoStack;
    PKSRESOLUTION Resolution = (PKSRESOLUTION)Data;

    DPRINT("ClockPropertyResolution\n");

    /* get stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* get the object header */
    ObjectHeader = (PKSIOBJECT_HEADER)IoStack->FileObject->FsContext2;

    /* sanity check */
    ASSERT(ObjectHeader);

    /* locate ks pin implementation from KSPIN offset */
    Clock = (PKSICLOCK)ObjectHeader->ObjectType;

    Resolution->Error = Clock->DefaultClock->Error;
    Resolution->Granularity = Clock->DefaultClock->Granularity;

    Irp->IoStatus.Information = sizeof(KSRESOLUTION);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
ClockPropertyState(
    IN PIRP Irp,
    IN PKSIDENTIFIER Request,
    IN OUT PVOID Data)
{
    PKSICLOCK Clock;
    PKSIOBJECT_HEADER ObjectHeader;
    PKSSTATE State = (PKSSTATE)Data;
    PIO_STACK_LOCATION IoStack;

    DPRINT("ClockPropertyState\n");

    /* get stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* get the object header */
    ObjectHeader = (PKSIOBJECT_HEADER)IoStack->FileObject->FsContext2;

    /* sanity check */
    ASSERT(ObjectHeader);

    /* locate ks pin implementation from KSPIN offset */
    Clock = (PKSICLOCK)ObjectHeader->ObjectType;

    *State = Clock->DefaultClock->State;
    Irp->IoStatus.Information = sizeof(KSSTATE);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
ClockPropertyFunctionTable(
    IN PIRP Irp,
    IN PKSIDENTIFIER Request,
    IN OUT PVOID Data)
{
    PKSCLOCK_FUNCTIONTABLE Table = (PKSCLOCK_FUNCTIONTABLE)Data;

    DPRINT("ClockPropertyFunctionTable\n");

    Table->GetCorrelatedPhysicalTime = ClockGetCorrelatedPhysicalTime;
    Table->GetCorrelatedTime = ClockGetCorrelatedTime;
    Table->GetPhysicalTime = ClockGetPhysicalTime;
    Table->GetTime = ClockGetTime;

    return STATUS_SUCCESS;
}


/*
    @implemented
*/
KSDDKAPI NTSTATUS NTAPI
KsCreateClock(
    IN  HANDLE ConnectionHandle,
    IN  PKSCLOCK_CREATE ClockCreate,
    OUT PHANDLE ClockHandle)
{
    return KspCreateObjectType(ConnectionHandle,
                               KSSTRING_Clock,
                               ClockCreate,
                               sizeof(KSCLOCK_CREATE),
                               GENERIC_READ,
                               ClockHandle);
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsValidateClockCreateRequest(
    IN  PIRP Irp,
    OUT PKSCLOCK_CREATE* OutClockCreate)
{
    PKSCLOCK_CREATE ClockCreate;
    NTSTATUS Status;
    ULONG Size;

    /* minimum request size */
    Size = sizeof(KSCLOCK_CREATE);

    /* copy create request */
    Status = KspCopyCreateRequest(Irp, 
                                  KSSTRING_Clock,
                                  &Size,
                                  (PVOID*)&ClockCreate);

    if (!NT_SUCCESS(Status))
        return Status;

    if (ClockCreate->CreateFlags != 0)
    {
        /* flags must be zero */
        FreeItem(ClockCreate);
        return STATUS_INVALID_PARAMETER;
    }

    *OutClockCreate = ClockCreate;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IKsClock_DispatchDeviceIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    UNICODE_STRING GuidString;
    PKSPROPERTY Property;
    NTSTATUS Status;

    DPRINT("IKsClock_DispatchDeviceIoControl\n");

    /* get current io stack */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* FIXME support events */
    ASSERT(IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_KS_PROPERTY);

    /* sanity check */
    ASSERT(IoStack->Parameters.DeviceIoControl.InputBufferLength >= sizeof(KSPROPERTY));

    /* call property handler */
    Status = KsPropertyHandler(Irp, 1, ClockPropertySet);

    /* get property from input buffer */
    Property = (PKSPROPERTY)IoStack->Parameters.DeviceIoControl.Type3InputBuffer;

    RtlStringFromGUID(&Property->Set, &GuidString);
    DPRINT("IKsClock_DispatchDeviceIoControl property Set |%S| Id %u Flags %x Status %lx ResultLength %lu\n", GuidString.Buffer, Property->Id, Property->Flags, Status, Irp->IoStatus.Information);
    RtlFreeUnicodeString(&GuidString);


    Irp->IoStatus.Status = STATUS_SUCCESS;
    CompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IKsClock_DispatchClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    UNIMPLEMENTED;

    Irp->IoStatus.Status = STATUS_SUCCESS;
    CompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

static KSDISPATCH_TABLE DispatchTable =
{
    IKsClock_DispatchDeviceIoControl,
    KsDispatchInvalidDeviceRequest,
    KsDispatchInvalidDeviceRequest,
    KsDispatchInvalidDeviceRequest,
    IKsClock_DispatchClose,
    KsDispatchQuerySecurity,
    KsDispatchSetSecurity,
    KsDispatchFastIoDeviceControlFailure,
    KsDispatchFastReadFailure,
    KsDispatchFastReadFailure,
};

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsCreateDefaultClock(
    IN  PIRP Irp,
    IN  PKSDEFAULTCLOCK DefaultClock)
{
    NTSTATUS Status;
    PKSCLOCK_CREATE ClockCreate;
    PKSICLOCK Clock;

    Status = KsValidateClockCreateRequest(Irp, &ClockCreate);
    if (!NT_SUCCESS(Status))
        return Status;

    /* let's allocate the clock struct */
    Clock = AllocateItem(NonPagedPool, sizeof(KSICLOCK));
    if (!Clock)
    {
        FreeItem(ClockCreate);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* now allocate the object header */
    Status = KsAllocateObjectHeader((PVOID*)&Clock->ObjectHeader, 0, NULL, Irp, &DispatchTable);

    /* did it work */
    if (!NT_SUCCESS(Status))
    {
        /* failed */
        FreeItem(ClockCreate);
        FreeItem(Clock);
        return Status;
    }

    /* initialize clock */
    /* FIXME IKsClock */
    Clock->ObjectHeader->ObjectType = (PVOID)Clock;
    Clock->ref = 1;
    Clock->ClockCreate = ClockCreate;
    Clock->DefaultClock = (PKSIDEFAULTCLOCK)DefaultClock;

    /* increment reference count */
    InterlockedIncrement(&Clock->DefaultClock->ReferenceCount);

    return Status;
}

/*
    @implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsAllocateDefaultClock(
    OUT PKSDEFAULTCLOCK* DefaultClock)
{
    return KsAllocateDefaultClockEx(DefaultClock, NULL, NULL, NULL, NULL, NULL, 0);
}

/*
    @unimplemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsAllocateDefaultClockEx(
    OUT PKSDEFAULTCLOCK* DefaultClock,
    IN  PVOID Context OPTIONAL,
    IN  PFNKSSETTIMER SetTimer OPTIONAL,
    IN  PFNKSCANCELTIMER CancelTimer OPTIONAL,
    IN  PFNKSCORRELATEDTIME CorrelatedTime OPTIONAL,
    IN  const KSRESOLUTION* Resolution OPTIONAL,
    IN  ULONG Flags)
{
    PKSIDEFAULTCLOCK Clock;

    if (!DefaultClock)
       return STATUS_INVALID_PARAMETER_1;

    /* allocate default clock */
    Clock = AllocateItem(NonPagedPool, sizeof(KSIDEFAULTCLOCK));
    if (!Clock)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* initialize default clock */
    KeInitializeSpinLock(&Clock->TimeLock);
    KeInitializeTimer(&Clock->Timer);
    Clock->ReferenceCount = 1;
    Clock->Context = Context;
    Clock->SetTimer = SetTimer;
    Clock->CancelTimer = CancelTimer;
    Clock->CorrelatedTime = CorrelatedTime;
    Clock->Flags = Flags;

    if (Resolution)
    {
        if (SetTimer)
        {
            Clock->Error = Resolution->Error;
        }

        if (CorrelatedTime)
        {
            Clock->Granularity = Resolution->Granularity;
        }
    }

    *DefaultClock = (PKSDEFAULTCLOCK)Clock;
    return STATUS_SUCCESS;
}

/*
    @implemented
*/
KSDDKAPI
VOID
NTAPI
KsFreeDefaultClock(
    IN  PKSDEFAULTCLOCK DefaultClock)
{
    PKSIDEFAULTCLOCK Clock = (PKSIDEFAULTCLOCK)DefaultClock;

    InterlockedDecrement(&Clock->ReferenceCount);

    if (Clock->ReferenceCount == 0)
    {
        /* free default clock */
        FreeItem(Clock);
    }
}

/*
    @implemented
*/
KSDDKAPI
KSSTATE
NTAPI
KsGetDefaultClockState(
    IN  PKSDEFAULTCLOCK DefaultClock)
{
    PKSIDEFAULTCLOCK Clock = (PKSIDEFAULTCLOCK)DefaultClock;
    return Clock->State;
}

/*
    @unimplemented
*/
KSDDKAPI
VOID
NTAPI
KsSetDefaultClockState(
    IN  PKSDEFAULTCLOCK DefaultClock,
    IN  KSSTATE State)
{
    PKSIDEFAULTCLOCK Clock = (PKSIDEFAULTCLOCK)DefaultClock;

    if (State != Clock->State)
    {
         /* FIXME set time etc */
         Clock->State = State;
    }

}

/*
    @implemented
*/
KSDDKAPI
LONGLONG
NTAPI
KsGetDefaultClockTime(
    IN  PKSDEFAULTCLOCK DefaultClock)
{
    LONGLONG Time = 0LL;
    PKSIDEFAULTCLOCK Clock = (PKSIDEFAULTCLOCK)DefaultClock;

    Time = ExInterlockedCompareExchange64(&Clock->Time, &Time, &Time, &Clock->TimeLock);

    return Time;
}

/*
    @implemented
*/
KSDDKAPI
VOID
NTAPI
KsSetDefaultClockTime(
    IN  PKSDEFAULTCLOCK DefaultClock,
    IN  LONGLONG Time)
{
    PKSIDEFAULTCLOCK Clock = (PKSIDEFAULTCLOCK)DefaultClock;

    /* set the time safely */
    ExInterlockedCompareExchange64(&Clock->Time, &Time, &Clock->Time, &Clock->TimeLock);
}
