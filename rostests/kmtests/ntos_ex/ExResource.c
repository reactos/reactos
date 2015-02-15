/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Executive Resource test
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <kmt_test.h>

//#define NDEBUG
#include <debug.h>

static
_IRQL_requires_max_(APC_LEVEL)
_Acquires_lock_(_Global_critical_region_)
PVOID
(NTAPI
*pExEnterCriticalRegionAndAcquireResourceShared)(
    _Inout_ _Requires_lock_not_held_(*_Curr_) _Acquires_shared_lock_(*_Curr_)
        PERESOURCE Resource);

static
_IRQL_requires_max_(APC_LEVEL)
_Acquires_lock_(_Global_critical_region_)
PVOID
(NTAPI
*pExEnterCriticalRegionAndAcquireResourceExclusive)(
    _Inout_ _Requires_lock_not_held_(*_Curr_) _Acquires_exclusive_lock_(*_Curr_)
        PERESOURCE Resource);

static
_IRQL_requires_max_(APC_LEVEL)
_Acquires_lock_(_Global_critical_region_)
PVOID
(NTAPI
*pExEnterCriticalRegionAndAcquireSharedWaitForExclusive)(
    _Inout_ _Requires_lock_not_held_(*_Curr_) _Acquires_lock_(*_Curr_)
        PERESOURCE Resource);

static
_IRQL_requires_max_(DISPATCH_LEVEL)
_Releases_lock_(_Global_critical_region_)
VOID
(FASTCALL
*pExReleaseResourceAndLeaveCriticalRegion)(
    _Inout_ _Requires_lock_held_(*_Curr_) _Releases_lock_(*_Curr_)
        PERESOURCE Resource);

static
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
BOOLEAN
(NTAPI
*pKeAreAllApcsDisabled)(VOID);

/* TODO: This is getting pretty long, make it somehow easier to read if possible */

/* TODO: this is the Windows Server 2003 version! ROS should use this!
 *       This declaration can be removed once ROS headers are corrected */
typedef struct _ERESOURCE_2K3 {
  LIST_ENTRY SystemResourcesList;
  POWNER_ENTRY OwnerTable;
  SHORT ActiveCount;
  USHORT Flag;
  volatile PKSEMAPHORE SharedWaiters;
  volatile PKEVENT ExclusiveWaiters;
  OWNER_ENTRY OwnerThreads[2];
  ULONG ContentionCount;
  USHORT NumberOfSharedWaiters;
  USHORT NumberOfExclusiveWaiters;
  _ANONYMOUS_UNION union {
    PVOID Address;
    ULONG_PTR CreatorBackTraceIndex;
  } DUMMYUNIONNAME;
  KSPIN_LOCK SpinLock;
} ERESOURCE_2K3, *PERESOURCE_2K3;

#define CheckResourceFields(Res, Reinit) do                                                                     \
{                                                                                                               \
    ok_eq_pointer((Res)->SystemResourcesList.Flink->Blink, &(Res)->SystemResourcesList);                        \
    ok_eq_pointer((Res)->SystemResourcesList.Blink->Flink, &(Res)->SystemResourcesList);                        \
    if (!Reinit) ok_eq_pointer((Res)->OwnerTable, NULL);                                                        \
    ok_eq_int((Res)->ActiveCount, 0);                                                                           \
    ok_eq_uint((Res)->Flag, 0);                                                                                 \
    if (!Reinit) ok_eq_pointer((Res)->SharedWaiters, NULL);                                                     \
    if (!Reinit) ok_eq_pointer((Res)->ExclusiveWaiters, NULL);                                                  \
    ok_eq_ulongptr((Res)->OwnerThreads[0].OwnerThread, 0);                                                      \
    ok_eq_ulong((Res)->OwnerThreads[0].TableSize, 0LU);                                                         \
    ok_eq_ulongptr((Res)->OwnerThreads[1].OwnerThread, 0);                                                      \
    ok_eq_ulong((Res)->OwnerThreads[1].TableSize, 0LU);                                                         \
    ok_eq_ulong((Res)->ContentionCount, 0LU);                                                                   \
    ok_eq_uint((Res)->NumberOfSharedWaiters, 0);                                                                \
    ok_eq_uint((Res)->NumberOfExclusiveWaiters, 0);                                                             \
    ok_eq_pointer((Res)->Address, NULL);                                                                        \
    ok_eq_ulongptr((Res)->SpinLock, 0);                                                                         \
} while (0)

#define CheckResourceStatus(Res, Exclusive, Shared, ExclusiveWaiters, SharedWaiters) do                         \
{                                                                                                               \
    if (Exclusive)                                                                                              \
        ok_bool_true(ExIsResourceAcquiredExclusiveLite(Res), "ExIsResourceAcquiredExclusiveLite returned");     \
    else                                                                                                        \
        ok_bool_false(ExIsResourceAcquiredExclusiveLite(Res), "ExIsResourceAcquiredExclusiveLite returned");    \
    ok_eq_ulong(ExIsResourceAcquiredSharedLite(Res), Shared);                                                   \
    ok_eq_ulong(ExGetExclusiveWaiterCount(Res), ExclusiveWaiters);                                              \
    ok_eq_ulong(ExGetSharedWaiterCount(Res), SharedWaiters);                                                    \
} while (0)

static
VOID
TestResourceSharedAccess(
    IN PERESOURCE Res)
{
    LONG Count = 0;

    KeEnterCriticalRegion();
    ok_bool_true(ExAcquireResourceSharedLite(Res, FALSE), "ExAcquireResourceSharedLite returned"); ++Count;
    CheckResourceStatus(Res, FALSE, Count, 0LU, 0LU);

    ok_bool_true(ExAcquireResourceSharedLite(Res, FALSE), "ExAcquireResourceSharedLite returned"); ++Count;
    ok_bool_true(ExAcquireResourceSharedLite(Res, TRUE), "ExAcquireResourceSharedLite returned"); ++Count;
    ok_bool_true(ExAcquireSharedStarveExclusive(Res, FALSE), "ExAcquireSharedStarveExclusive returned"); ++Count;
    ok_bool_true(ExAcquireSharedStarveExclusive(Res, TRUE), "ExAcquireSharedStarveExclusive returned"); ++Count;
    ok_bool_true(ExAcquireSharedWaitForExclusive(Res, FALSE), "ExAcquireSharedWaitForExclusive returned"); ++Count;
    ok_bool_true(ExAcquireSharedWaitForExclusive(Res, TRUE), "ExAcquireSharedWaitForExclusive returned"); ++Count;
    CheckResourceStatus(Res, FALSE, Count, 0LU, 0LU);

    /* this one fails, TRUE would deadlock */
    ok_bool_false(ExAcquireResourceExclusiveLite(Res, FALSE), "ExAcquireResourceExclusiveLite returned");
    CheckResourceStatus(Res, FALSE, Count, 0LU, 0LU);

    /* this asserts */
    if (!KmtIsCheckedBuild)
        ExConvertExclusiveToSharedLite(Res);
    CheckResourceStatus(Res, FALSE, Count, 0LU, 0LU);

    while (Count--)
        ExReleaseResourceLite(Res);
    KeLeaveCriticalRegion();
}

static
VOID
TestResourceExclusiveAccess(
    IN PERESOURCE Res)
{
    LONG Count = 0;

    KeEnterCriticalRegion();
    ok_bool_true(ExAcquireResourceExclusiveLite(Res, FALSE), "ExAcquireResourceExclusiveLite returned"); ++Count;

    CheckResourceStatus(Res, TRUE, Count, 0LU, 0LU);

    ok_bool_true(ExAcquireResourceExclusiveLite(Res, TRUE), "ExAcquireResourceExclusiveLite returned"); ++Count;
    CheckResourceStatus(Res, TRUE, Count, 0LU, 0LU);

    ok_bool_true(ExAcquireResourceSharedLite(Res, FALSE), "ExAcquireResourceSharedLite returned"); ++Count;
    ok_bool_true(ExAcquireResourceSharedLite(Res, TRUE), "ExAcquireResourceSharedLite returned"); ++Count;
    ok_bool_true(ExAcquireSharedStarveExclusive(Res, FALSE), "ExAcquireSharedStarveExclusive returned"); ++Count;
    ok_bool_true(ExAcquireSharedStarveExclusive(Res, TRUE), "ExAcquireSharedStarveExclusive returned"); ++Count;
    ok_bool_true(ExAcquireSharedWaitForExclusive(Res, FALSE), "ExAcquireSharedWaitForExclusive returned"); ++Count;
    ok_bool_true(ExAcquireSharedWaitForExclusive(Res, TRUE), "ExAcquireSharedWaitForExclusive returned"); ++Count;
    CheckResourceStatus(Res, TRUE, Count, 0LU, 0LU);

    ExConvertExclusiveToSharedLite(Res);
    CheckResourceStatus(Res, FALSE, Count, 0LU, 0LU);

    while (Count--)
        ExReleaseResourceLite(Res);
    KeLeaveCriticalRegion();
}

static
VOID
TestResourceUndocumentedShortcuts(
    IN PERESOURCE Res,
    IN BOOLEAN AreApcsDisabled)
{
    PVOID Ret;
    LONG Count = 0;

    ok_bool_false(KeAreApcsDisabled(), "KeAreApcsDisabled returned");
    if (pKeAreAllApcsDisabled)
        ok_eq_uint(pKeAreAllApcsDisabled(), AreApcsDisabled);

    if (skip(pExEnterCriticalRegionAndAcquireResourceShared &&
             pExEnterCriticalRegionAndAcquireSharedWaitForExclusive &&
             pExEnterCriticalRegionAndAcquireResourceExclusive &&
             pExReleaseResourceAndLeaveCriticalRegion, "No shortcuts\n"))
    {
        return;
    }
    /* ExEnterCriticalRegionAndAcquireResourceShared, ExEnterCriticalRegionAndAcquireSharedWaitForExclusive */
    Count = 0;
    Ret = pExEnterCriticalRegionAndAcquireResourceShared(Res); ++Count;
    ok_eq_pointer(Ret, KeGetCurrentThread()->Win32Thread);
    ok_bool_true(KeAreApcsDisabled(), "KeAreApcsDisabled returned");
    if (pKeAreAllApcsDisabled)
        ok_eq_bool(pKeAreAllApcsDisabled(), AreApcsDisabled);
    CheckResourceStatus(Res, FALSE, Count, 0LU, 0LU);

    Ret = pExEnterCriticalRegionAndAcquireResourceShared(Res); ++Count;
    ok_eq_pointer(Ret, KeGetCurrentThread()->Win32Thread);
    ok_bool_true(KeAreApcsDisabled(), "KeAreApcsDisabled returned");
    if (pKeAreAllApcsDisabled)
        ok_eq_bool(pKeAreAllApcsDisabled(), AreApcsDisabled);
    CheckResourceStatus(Res, FALSE, Count, 0LU, 0LU);

    pExEnterCriticalRegionAndAcquireSharedWaitForExclusive(Res); ++Count;
    ok_eq_pointer(Ret, KeGetCurrentThread()->Win32Thread);
    ok_bool_true(KeAreApcsDisabled(), "KeAreApcsDisabled returned");
    if (pKeAreAllApcsDisabled)
        ok_eq_bool(pKeAreAllApcsDisabled(), AreApcsDisabled);
    CheckResourceStatus(Res, FALSE, Count, 0LU, 0LU);

    while (Count-- > 1)
    {
        pExReleaseResourceAndLeaveCriticalRegion(Res);
        ok_bool_true(KeAreApcsDisabled(), "KeAreApcsDisabled returned");
        if (pKeAreAllApcsDisabled)
            ok_eq_bool(pKeAreAllApcsDisabled(), AreApcsDisabled);
        CheckResourceStatus(Res, FALSE, Count, 0LU, 0LU);
    }

    pExReleaseResourceAndLeaveCriticalRegion(Res);
    ok_bool_false(KeAreApcsDisabled(), "KeAreApcsDisabled returned");
    if (pKeAreAllApcsDisabled)
        ok_eq_bool(pKeAreAllApcsDisabled(), AreApcsDisabled);
    CheckResourceStatus(Res, FALSE, Count, 0LU, 0LU);

    /* ExEnterCriticalRegionAndAcquireResourceExclusive */
    Count = 0;
    ok_bool_false(KeAreApcsDisabled(), "KeAreApcsDisabled returned");
    if (pKeAreAllApcsDisabled)
        ok_eq_bool(pKeAreAllApcsDisabled(), AreApcsDisabled);
    Ret = pExEnterCriticalRegionAndAcquireResourceExclusive(Res); ++Count;
    ok_eq_pointer(Ret, KeGetCurrentThread()->Win32Thread);
    ok_bool_true(KeAreApcsDisabled(), "KeAreApcsDisabled returned");
    if (pKeAreAllApcsDisabled)
        ok_eq_bool(pKeAreAllApcsDisabled(), AreApcsDisabled);
    CheckResourceStatus(Res, TRUE, Count, 0LU, 0LU);

    Ret = pExEnterCriticalRegionAndAcquireResourceExclusive(Res); ++Count;
    ok_eq_pointer(Ret, KeGetCurrentThread()->Win32Thread);
    ok_bool_true(KeAreApcsDisabled(), "KeAreApcsDisabled returned");
    if (pKeAreAllApcsDisabled)
        ok_eq_bool(pKeAreAllApcsDisabled(), AreApcsDisabled);
    CheckResourceStatus(Res, TRUE, Count, 0LU, 0LU);

    pExReleaseResourceAndLeaveCriticalRegion(Res); --Count;
    ok_bool_true(KeAreApcsDisabled(), "KeAreApcsDisabled returned");
    if (pKeAreAllApcsDisabled)
        ok_eq_bool(pKeAreAllApcsDisabled(), AreApcsDisabled);
    CheckResourceStatus(Res, TRUE, Count, 0LU, 0LU);

    pExReleaseResourceAndLeaveCriticalRegion(Res); --Count;
    ok_bool_false(KeAreApcsDisabled(), "KeAreApcsDisabled returned");
    if (pKeAreAllApcsDisabled)
        ok_eq_uint(pKeAreAllApcsDisabled(), AreApcsDisabled);
    CheckResourceStatus(Res, FALSE, Count, 0LU, 0LU);
}

typedef BOOLEAN (NTAPI *PACQUIRE_FUNCTION)(PERESOURCE, BOOLEAN);

typedef struct
{
    HANDLE Handle;
    PKTHREAD Thread;
    PERESOURCE Res;
    KEVENT InEvent;
    KEVENT OutEvent;
    PACQUIRE_FUNCTION AcquireResource;
    BOOLEAN Wait;
    BOOLEAN RetExpected;
} THREAD_DATA, *PTHREAD_DATA;

static
VOID
NTAPI
AcquireResourceThread(
    PVOID Context)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PTHREAD_DATA ThreadData = Context;
    BOOLEAN Ret;

    KeEnterCriticalRegion();
    Ret = ThreadData->AcquireResource(ThreadData->Res, ThreadData->Wait);
    if (ThreadData->RetExpected)
        ok_bool_true(Ret, "AcquireResource returned");
    else
        ok_bool_false(Ret, "AcquireResource returned");

    ok_bool_false(KeSetEvent(&ThreadData->OutEvent, 0, TRUE), "KeSetEvent returned");
    Status = KeWaitForSingleObject(&ThreadData->InEvent, Executive, KernelMode, FALSE, NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);

    if (Ret)
        ExReleaseResource(ThreadData->Res);
    KeLeaveCriticalRegion();
}

static
VOID
InitThreadData(
    PTHREAD_DATA ThreadData,
    PERESOURCE Res,
    PACQUIRE_FUNCTION AcquireFunction)
{
    ThreadData->Res = Res;
    KeInitializeEvent(&ThreadData->InEvent, NotificationEvent, FALSE);
    KeInitializeEvent(&ThreadData->OutEvent, NotificationEvent, FALSE);
    ThreadData->AcquireResource = AcquireFunction;
}

static
NTSTATUS
StartThread(
    PTHREAD_DATA ThreadData,
    PLARGE_INTEGER Timeout,
    BOOLEAN Wait,
    BOOLEAN RetExpected)
{
    NTSTATUS Status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES Attributes;

    ThreadData->Wait = Wait;
    ThreadData->RetExpected = RetExpected;
    InitializeObjectAttributes(&Attributes, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
    Status = PsCreateSystemThread(&ThreadData->Handle, GENERIC_ALL, &Attributes, NULL, NULL, AcquireResourceThread, ThreadData);
    ok_eq_hex(Status, STATUS_SUCCESS);
    Status = ObReferenceObjectByHandle(ThreadData->Handle, SYNCHRONIZE, *PsThreadType, KernelMode, (PVOID *)&ThreadData->Thread, NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);

    return KeWaitForSingleObject(&ThreadData->OutEvent, Executive, KernelMode, FALSE, Timeout);
}

static
VOID
FinishThread(
    PTHREAD_DATA ThreadData)
{
    NTSTATUS Status = STATUS_SUCCESS;

    KeSetEvent(&ThreadData->InEvent, 0, TRUE);
    Status = KeWaitForSingleObject(ThreadData->Thread, Executive, KernelMode, FALSE, NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);

    ObDereferenceObject(ThreadData->Thread);
    Status = ZwClose(ThreadData->Handle);
    ok_eq_hex(Status, STATUS_SUCCESS);
    KeClearEvent(&ThreadData->InEvent);
    KeClearEvent(&ThreadData->OutEvent);
}

static
VOID
TestResourceWithThreads(
    IN PERESOURCE Res)
{
    NTSTATUS Status = STATUS_SUCCESS;
    THREAD_DATA ThreadDataShared;
    THREAD_DATA ThreadDataShared2;
    THREAD_DATA ThreadDataExclusive;
    THREAD_DATA ThreadDataSharedStarve;
    THREAD_DATA ThreadDataSharedWait;
    LARGE_INTEGER Timeout;
    Timeout.QuadPart = -10 * 1000 * 10; /* 10 ms */

    InitThreadData(&ThreadDataShared, Res, ExAcquireResourceSharedLite);
    InitThreadData(&ThreadDataShared2, Res, ExAcquireResourceSharedLite);
    InitThreadData(&ThreadDataExclusive, Res, ExAcquireResourceExclusiveLite);
    InitThreadData(&ThreadDataSharedStarve, Res, ExAcquireSharedStarveExclusive);
    InitThreadData(&ThreadDataSharedWait, Res, ExAcquireSharedWaitForExclusive);

    /* have a thread acquire the resource shared */
    Status = StartThread(&ThreadDataShared, NULL, FALSE, TRUE);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckResourceStatus(Res, FALSE, 0LU, 0LU, 0LU);
    ok_eq_int(Res->ActiveCount, 1);

    /* a second thread should be able to acquire the resource shared */
    Status = StartThread(&ThreadDataShared2, NULL, FALSE, TRUE);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckResourceStatus(Res, FALSE, 0LU, 0LU, 0LU);
    ok_eq_int(Res->ActiveCount, 2);
    FinishThread(&ThreadDataShared2);
    CheckResourceStatus(Res, FALSE, 0LU, 0LU, 0LU);
    ok_eq_int(Res->ActiveCount, 1);

    /* now have a thread that tries to acquire the resource exclusive -- it should fail */
    Status = StartThread(&ThreadDataExclusive, NULL, FALSE, FALSE);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckResourceStatus(Res, FALSE, 0LU, 0LU, 0LU);
    ok_eq_int(Res->ActiveCount, 1);
    FinishThread(&ThreadDataExclusive);
    CheckResourceStatus(Res, FALSE, 0LU, 0LU, 0LU);
    ok_eq_int(Res->ActiveCount, 1);

    /* as above, but this time it should block */
    Status = StartThread(&ThreadDataExclusive, &Timeout, TRUE, TRUE);
    ok_eq_hex(Status, STATUS_TIMEOUT);
    CheckResourceStatus(Res, FALSE, 0LU, 1LU, 0LU);
    ok_eq_int(Res->ActiveCount, 1);

    /* now try another shared one -- it should fail */
    Status = StartThread(&ThreadDataShared2, NULL, FALSE, FALSE);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckResourceStatus(Res, FALSE, 0LU, 1LU, 0LU);
    ok_eq_int(Res->ActiveCount, 1);
    FinishThread(&ThreadDataShared2);

    /* same for ExAcquireSharedWaitForExclusive */
    Status = StartThread(&ThreadDataSharedWait, NULL, FALSE, FALSE);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckResourceStatus(Res, FALSE, 0LU, 1LU, 0LU);
    ok_eq_int(Res->ActiveCount, 1);
    FinishThread(&ThreadDataSharedWait);

    /* ExAcquireSharedStarveExclusive must get access though! */
    Status = StartThread(&ThreadDataSharedStarve, NULL, TRUE, TRUE);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckResourceStatus(Res, FALSE, 0LU, 1LU, 0LU);
    ok_eq_int(Res->ActiveCount, 2);
    FinishThread(&ThreadDataSharedStarve);
    CheckResourceStatus(Res, FALSE, 0LU, 1LU, 0LU);
    ok_eq_int(Res->ActiveCount, 1);

    /* block another shared one */
    Status = StartThread(&ThreadDataShared2, &Timeout, TRUE, TRUE);
    ok_eq_hex(Status, STATUS_TIMEOUT);
    CheckResourceStatus(Res, FALSE, 0LU, 1LU, 1LU);
    ok_eq_int(Res->ActiveCount, 1);

    /* finish the very first one */
    FinishThread(&ThreadDataShared);

    /* now the blocked exclusive one should get the resource */
    Status = KeWaitForSingleObject(&ThreadDataExclusive.OutEvent, Executive, KernelMode, FALSE, NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckResourceStatus(Res, FALSE, 0LU, 0LU, 1LU);
    ok_eq_int(Res->ActiveCount, 1);
    ok_eq_uint((Res->Flag & ResourceOwnedExclusive) != 0, 1);

    FinishThread(&ThreadDataExclusive);
    CheckResourceStatus(Res, FALSE, 0LU, 0LU, 0LU);

    /* now the blocked shared one should resume */
    Status = KeWaitForSingleObject(&ThreadDataShared2.OutEvent, Executive, KernelMode, FALSE, NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckResourceStatus(Res, FALSE, 0LU, 0LU, 0LU);
    ok_eq_int(Res->ActiveCount, 1);
    FinishThread(&ThreadDataShared2);
    CheckResourceStatus(Res, FALSE, 0LU, 0LU, 0LU);
    ok_eq_int(Res->ActiveCount, 0);
}

START_TEST(ExResource)
{
    NTSTATUS Status;
    ERESOURCE Res;
    KIRQL Irql;

    pExEnterCriticalRegionAndAcquireResourceShared = KmtGetSystemRoutineAddress(L"ExEnterCriticalRegionAndAcquireResourceShared");
    pExEnterCriticalRegionAndAcquireSharedWaitForExclusive = KmtGetSystemRoutineAddress(L"ExEnterCriticalRegionAndAcquireSharedWaitForExclusive");
    pExEnterCriticalRegionAndAcquireResourceExclusive = KmtGetSystemRoutineAddress(L"ExEnterCriticalRegionAndAcquireResourceExclusive");
    pExReleaseResourceAndLeaveCriticalRegion = KmtGetSystemRoutineAddress(L"ExReleaseResourceAndLeaveCriticalRegion");
    pKeAreAllApcsDisabled = KmtGetSystemRoutineAddress(L"KeAreAllApcsDisabled");

    if (skip(pKeAreAllApcsDisabled != NULL, "KeAreAllApcsDisabled unavailable\n"))
    {
        /* We can live without this function here */
    }

    /* this must be true even with the different structure versions */
    ASSERT(sizeof(ERESOURCE) == sizeof(ERESOURCE_2K3));

    /* functional tests & internals */
    Irql = KeRaiseIrqlToDpcLevel();
      Status = ExInitializeResourceLite(&Res);
      ok_eq_hex(Status, STATUS_SUCCESS);
    KeLowerIrql(APC_LEVEL);

      Status = ExDeleteResourceLite(&Res);
      ok_eq_hex(Status, STATUS_SUCCESS);
    KeLowerIrql(Irql);

    memset(&Res, 0x55, sizeof Res);
    Status = ExInitializeResourceLite(&Res);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckResourceFields((PERESOURCE_2K3)&Res, FALSE);

    CheckResourceStatus(&Res, FALSE, 0LU, 0LU, 0LU);

    TestResourceSharedAccess(&Res);
    CheckResourceStatus(&Res, FALSE, 0LU, 0LU, 0LU);

    TestResourceExclusiveAccess(&Res);
    CheckResourceStatus(&Res, FALSE, 0LU, 0LU, 0LU);

    TestResourceUndocumentedShortcuts(&Res, FALSE);
    CheckResourceStatus(&Res, FALSE, 0LU, 0LU, 0LU);
    KeRaiseIrql(APC_LEVEL, &Irql);
      TestResourceUndocumentedShortcuts(&Res, TRUE);
    KeLowerIrql(Irql);
    ok_bool_false(KeAreApcsDisabled(), "KeAreApcsDisabled returned");
    CheckResourceStatus(&Res, FALSE, 0LU, 0LU, 0LU);

    TestResourceWithThreads(&Res);

    /* ExReinitializeResourceLite cleans up after us */
    Status = ExReinitializeResourceLite(&Res);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckResourceFields((PERESOURCE_2K3)&Res, TRUE);
    CheckResourceStatus(&Res, FALSE, 0LU, 0LU, 0LU);

    Status = ExDeleteResourceLite(&Res);
    ok_eq_hex(Status, STATUS_SUCCESS);

    /* parameter checks */
    Status = STATUS_SUCCESS;
    _SEH2_TRY {
        ExInitializeResourceLite(NULL);
    } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
        Status = _SEH2_GetExceptionCode();
    } _SEH2_END;
    ok_eq_hex(Status, STATUS_ACCESS_VIOLATION);

    /* these bugcheck
    ExDeleteResourceLite(NULL);
    Status = ExDeleteResourceLite(&Res);*/
}
