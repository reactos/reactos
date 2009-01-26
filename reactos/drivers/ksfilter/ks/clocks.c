/* ===============================================================
    Clock Functions
*/

#include <ntddk.h>
#include <debug.h>
#include <ks.h>

typedef struct
{

}KS_DEFAULT_CLOCK;

/*
    @unimplemented
*/
KSDDKAPI NTSTATUS NTAPI
KsCreateClock(
    IN  HANDLE ConnectionHandle,
    IN  PKSCLOCK_CREATE ClockCreate,
    OUT PHANDLE ClockHandle)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI NTSTATUS NTAPI
KsCreateDefaultClock(
    IN  PIRP Irp,
    IN  PKSDEFAULTCLOCK DefaultClock)
{
    NTSTATUS Status;
    PKSCLOCK_CREATE ClockCreate;

    Status = KsValidateClockCreateRequest(Irp, &ClockCreate);
    if (!NT_SUCCESS(Status))
        return Status;

//    ExAllocatePoolWithTag(NonPagedPool, sizeof(KS_DEFAULT_CLOCK), 0);



    return STATUS_UNSUCCESSFUL;
}

/*
    @implemented
*/
KSDDKAPI NTSTATUS NTAPI
KsAllocateDefaultClock(
    OUT PKSDEFAULTCLOCK* DefaultClock)
{
    return KsAllocateDefaultClockEx(DefaultClock, NULL, NULL, NULL, NULL, NULL, 0);
}

/*
    @unimplemented
*/
KSDDKAPI NTSTATUS NTAPI
KsAllocateDefaultClockEx(
    OUT PKSDEFAULTCLOCK* DefaultClock,
    IN  PVOID Context OPTIONAL,
    IN  PFNKSSETTIMER SetTimer OPTIONAL,
    IN  PFNKSCANCELTIMER CancelTimer OPTIONAL,
    IN  PFNKSCORRELATEDTIME CorrelatedTime OPTIONAL,
    IN  const KSRESOLUTION* Resolution OPTIONAL,
    IN  ULONG Flags)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI VOID NTAPI
KsFreeDefaultClock(
    IN  PKSDEFAULTCLOCK DefaultClock)
{
    UNIMPLEMENTED;
}

/*
    @unimplemented
*/
KSDDKAPI NTSTATUS NTAPI
KsValidateClockCreateRequest(
    IN  PIRP Irp,
    OUT PKSCLOCK_CREATE* ClockCreate)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI KSSTATE NTAPI
KsGetDefaultClockState(
    IN  PKSDEFAULTCLOCK DefaultClock)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

/*
    @unimplemented
*/
KSDDKAPI VOID NTAPI
KsSetDefaultClockState(
    IN  PKSDEFAULTCLOCK DefaultClock,
    IN  KSSTATE State)
{
    UNIMPLEMENTED;
}

/*
    @unimplemented
*/
KSDDKAPI LONGLONG NTAPI
KsGetDefaultClockTime(
    IN  PKSDEFAULTCLOCK DefaultClock)
{
    UNIMPLEMENTED;
    return 0;
}

/*
    @unimplemented
*/
KSDDKAPI VOID NTAPI
KsSetDefaultClockTime(
    IN  PKSDEFAULTCLOCK DefaultClock,
    IN  LONGLONG Time)
{
    UNIMPLEMENTED;
}
