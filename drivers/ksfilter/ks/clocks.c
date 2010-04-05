/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/ksfilter/ks/clocks.c
 * PURPOSE:         KS Clocks functions
 * PROGRAMMER:      Johannes Anderwald
 */


#include "priv.h"

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
    KSRESOLUTION* Resolution;
    ULONG Flags;

}KSIDEFAULTCLOCK, *PKSIDEFAULTCLOCK;

typedef struct
{
    IKsClock *lpVtbl;
    LONG ref;
    PKSCLOCK_CREATE ClockCreate;
    PKSIDEFAULTCLOCK DefaultClock;
    PKSIOBJECT_HEADER ObjectHeader;
}KSICLOCK, *PKSICLOCK;


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
    UNIMPLEMENTED

    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IKsClock_DispatchClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp)
{
    UNIMPLEMENTED

    Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_NOT_IMPLEMENTED;
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
    PKSOBJECT_CREATE_ITEM CreateItem;

    Status = KsValidateClockCreateRequest(Irp, &ClockCreate);
    if (!NT_SUCCESS(Status))
        return Status;

    /* let's allocate the clock struct */
    Clock = AllocateItem(NonPagedPool, sizeof(KSICLOCK));
    if (!Clock)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* now allocate the object header */
    Status = KsAllocateObjectHeader((PVOID*)&Clock->ObjectHeader, 0, NULL, Irp, &DispatchTable);

    /* did it work */
    if (!NT_SUCCESS(Status))
    {
        /* failed */
        FreeItem(Clock);
        return Status;
    }

    /* initialize clock */
    /* FIXME IKsClock */
    Clock->ObjectHeader->Unknown = (PUNKNOWN)&Clock->lpVtbl;
    Clock->ref = 1;
    Clock->ClockCreate = ClockCreate;
    Clock->DefaultClock = (PKSIDEFAULTCLOCK)DefaultClock;

    /* increment reference count */
    InterlockedIncrement(&Clock->DefaultClock->ReferenceCount);

    /* get create item */
    CreateItem = KSCREATE_ITEM_IRP_STORAGE(Irp);

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
    Clock->Resolution = (PKSRESOLUTION)Resolution;
    Clock->Flags = Flags;

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
