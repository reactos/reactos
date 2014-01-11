
#include <ntifs.h>

#define NDEBUG
#include <debug.h>

NTSTATUS
NTAPI
DriverEntry (
    _In_ PDRIVER_OBJECT	DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
{
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

VOID
NTAPI
WdAllocateWatchdog(
    PVOID p1,
    PVOID p2,
    ULONG p3)
{
    UNIMPLEMENTED;
}

VOID
NTAPI
WdAllocateDeferredWatchdog(
    PVOID p1,
    PVOID p2,
    ULONG p3)
{
    UNIMPLEMENTED;
}

VOID
NTAPI
WdFreeWatchdog(
    PVOID p1)
{
    UNIMPLEMENTED;
}

VOID
NTAPI
WdFreeDeferredWatchdog(
    PVOID p1)
{
    UNIMPLEMENTED;
}

VOID
NTAPI
WdStartWatch(
    PVOID p1,
    LARGE_INTEGER p2,
    ULONG p3)
{
    UNIMPLEMENTED;
}

VOID
NTAPI
WdStartDeferredWatch(
    PVOID p1,
    PVOID p2,
    ULONG p3)
{
    UNIMPLEMENTED;
}

VOID
NTAPI
WdStopWatch(
    PVOID p1,
    ULONG p2)
{
    UNIMPLEMENTED;
}

VOID
NTAPI
WdStopDeferredWatch(
    PVOID p1)
{
    UNIMPLEMENTED;
}

VOID
NTAPI
WdSuspendWatch(
    PVOID p1)
{
    UNIMPLEMENTED;
}

VOID
FASTCALL
WdSuspendDeferredWatch(
    PVOID p1)
{
    UNIMPLEMENTED;
}

VOID
NTAPI
WdResumeWatch(
    PVOID p1,
    PVOID p2)
{
    UNIMPLEMENTED;
}

VOID
FASTCALL
WdResumeDeferredWatch(
    PVOID p1,
    PVOID p2)
{
    UNIMPLEMENTED;
}

VOID
NTAPI
WdResetWatch(
    PVOID p1)
{
    UNIMPLEMENTED;
}

VOID
FASTCALL
WdResetDeferredWatch(
    PVOID p1)
{
    UNIMPLEMENTED;
}

VOID
FASTCALL
WdEnterMonitoredSection(
    PVOID p1)
{
    UNIMPLEMENTED;
}

VOID
FASTCALL
WdExitMonitoredSection(
    PVOID p1)
{
    UNIMPLEMENTED;
}

VOID
NTAPI
WdAttachContext(
    PVOID p1,
    PVOID p2)
{
    UNIMPLEMENTED;
}

VOID
NTAPI
WdDetachContext(
    PVOID p1)
{
    UNIMPLEMENTED;
}

VOID
NTAPI
WdGetDeviceObject(
    PVOID p1)
{
    UNIMPLEMENTED;
}

VOID
NTAPI
WdGetLowestDeviceObject(
    PVOID p1)
{
    UNIMPLEMENTED;
}

VOID
NTAPI
WdGetLastEvent(
    PVOID p1)
{
    UNIMPLEMENTED;
}

VOID
NTAPI
WdCompleteEvent(
    PVOID p1,
    PVOID p2)
{
    UNIMPLEMENTED;
}

VOID
NTAPI
WdReferenceObject(
    PVOID p1)
{
    UNIMPLEMENTED;
}

VOID
NTAPI
WdDereferenceObject(
    PVOID p1)
{
    UNIMPLEMENTED;
}

BOOLEAN
NTAPI
WdMadeAnyProgress(
    PVOID p1)
{
    UNIMPLEMENTED;
    return FALSE;
}








