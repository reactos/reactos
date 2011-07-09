/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Executive Resource test
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

#undef NTDDI_VERSION
#define NTDDI_VERSION NTDDI_WS03SP1
#include <ntddk.h>
#include <ntifs.h>
#include <ndk/extypes.h>
#include <kmt_test.h>
#include <pseh/pseh2.h>

//#define NDEBUG
#include <debug.h>

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
#if defined(_WIN64)
  PVOID Reserved2; /* TODO: not sure if this should be in here for 2k3 */
#endif
  _ANONYMOUS_UNION union {
    PVOID Address;
    ULONG_PTR CreatorBackTraceIndex;
  } DUMMYUNIONNAME;
  KSPIN_LOCK SpinLock;
} ERESOURCE_2K3, *PERESOURCE_2K3;

#define CheckResourceFields(Res) do                                                                             \
{                                                                                                               \
    ok_eq_pointer((Res)->SystemResourcesList.Flink->Blink, &(Res)->SystemResourcesList);                        \
    ok_eq_pointer((Res)->SystemResourcesList.Blink->Flink, &(Res)->SystemResourcesList);                        \
    ok_eq_pointer((Res)->OwnerTable, NULL);                                                                     \
    ok_eq_int((Res)->ActiveCount, 0);                                                                           \
    ok_eq_uint((Res)->Flag, 0);                                                                                 \
    ok_eq_pointer((Res)->SharedWaiters, NULL);                                                                  \
    ok_eq_pointer((Res)->ExclusiveWaiters, NULL);                                                               \
    ok_eq_pointer((PVOID)(Res)->OwnerThreads[0].OwnerThread, NULL);                                             \
    ok_eq_ulong((Res)->OwnerThreads[0].TableSize, 0LU);                                                         \
    ok_eq_pointer((PVOID)(Res)->OwnerThreads[1].OwnerThread, NULL);                                             \
    ok_eq_ulong((Res)->OwnerThreads[1].TableSize, 0LU);                                                         \
    ok_eq_ulong((Res)->ContentionCount, 0LU);                                                                   \
    ok_eq_uint((Res)->NumberOfSharedWaiters, 0);                                                                \
    ok_eq_uint((Res)->NumberOfExclusiveWaiters, 0);                                                             \
    /* ok_eq_pointer((Res)->Reserved2, NULL); */                                                                \
    ok_eq_pointer((Res)->Address, NULL);                                                                        \
    ok_eq_pointer((PVOID)(Res)->SpinLock, NULL);                                                                \
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

    /* this must not crash or deadlock (but can assert) */
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
    ok_eq_uint(KeAreAllApcsDisabled(), AreApcsDisabled);

    /* ExEnterCriticalRegionAndAcquireResourceShared, ExEnterCriticalRegionAndAcquireSharedWaitForExclusive */
    Count = 0;
    Ret = ExEnterCriticalRegionAndAcquireResourceShared(Res); ++Count;
    ok_eq_pointer(Ret, KeGetCurrentThread()->Win32Thread);
    ok_bool_true(KeAreApcsDisabled(), "KeAreApcsDisabled returned");
    ok_eq_bool(KeAreAllApcsDisabled(), AreApcsDisabled);
    CheckResourceStatus(Res, FALSE, Count, 0LU, 0LU);

    Ret = ExEnterCriticalRegionAndAcquireResourceShared(Res); ++Count;
    ok_eq_pointer(Ret, KeGetCurrentThread()->Win32Thread);
    ok_bool_true(KeAreApcsDisabled(), "KeAreApcsDisabled returned");
    ok_eq_bool(KeAreAllApcsDisabled(), AreApcsDisabled);
    CheckResourceStatus(Res, FALSE, Count, 0LU, 0LU);

    ExEnterCriticalRegionAndAcquireSharedWaitForExclusive(Res); ++Count;
    ok_eq_pointer(Ret, KeGetCurrentThread()->Win32Thread);
    ok_bool_true(KeAreApcsDisabled(), "KeAreApcsDisabled returned");
    ok_eq_bool(KeAreAllApcsDisabled(), AreApcsDisabled);
    CheckResourceStatus(Res, FALSE, Count, 0LU, 0LU);

    while (Count-- > 1)
    {
        ExReleaseResourceAndLeaveCriticalRegion(Res);
        ok_bool_true(KeAreApcsDisabled(), "KeAreApcsDisabled returned");
        ok_eq_bool(KeAreAllApcsDisabled(), AreApcsDisabled);
        CheckResourceStatus(Res, FALSE, Count, 0LU, 0LU);
    }

    ExReleaseResourceAndLeaveCriticalRegion(Res);
    ok_bool_false(KeAreApcsDisabled(), "KeAreApcsDisabled returned");
    ok_eq_bool(KeAreAllApcsDisabled(), AreApcsDisabled);
    CheckResourceStatus(Res, FALSE, Count, 0LU, 0LU);

    /* ExEnterCriticalRegionAndAcquireResourceExclusive */
    Count = 0;
    ok_bool_false(KeAreApcsDisabled(), "KeAreApcsDisabled returned");
    ok_eq_bool(KeAreAllApcsDisabled(), AreApcsDisabled);
    Ret = ExEnterCriticalRegionAndAcquireResourceExclusive(Res); ++Count;
    ok_eq_pointer(Ret, KeGetCurrentThread()->Win32Thread);
    ok_bool_true(KeAreApcsDisabled(), "KeAreApcsDisabled returned");
    ok_eq_bool(KeAreAllApcsDisabled(), AreApcsDisabled);
    CheckResourceStatus(Res, TRUE, Count, 0LU, 0LU);

    Ret = ExEnterCriticalRegionAndAcquireResourceExclusive(Res); ++Count;
    ok_eq_pointer(Ret, KeGetCurrentThread()->Win32Thread);
    ok_bool_true(KeAreApcsDisabled(), "KeAreApcsDisabled returned");
    ok_eq_bool(KeAreAllApcsDisabled(), AreApcsDisabled);
    CheckResourceStatus(Res, TRUE, Count, 0LU, 0LU);

    ExReleaseResourceAndLeaveCriticalRegion(Res); --Count;
    ok_bool_true(KeAreApcsDisabled(), "KeAreApcsDisabled returned");
    ok_eq_bool(KeAreAllApcsDisabled(), AreApcsDisabled);
    CheckResourceStatus(Res, TRUE, Count, 0LU, 0LU);

    ExReleaseResourceAndLeaveCriticalRegion(Res); --Count;
    ok_bool_false(KeAreApcsDisabled(), "KeAreApcsDisabled returned");
    ok_eq_uint(KeAreAllApcsDisabled(), AreApcsDisabled);
    CheckResourceStatus(Res, FALSE, Count, 0LU, 0LU);
}

START_TEST(ExResource)
{
    NTSTATUS Status;
    ERESOURCE Res;
    KIRQL Irql;

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
    CheckResourceFields((PERESOURCE_2K3)&Res);

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

    /* ExReinitializeResourceLite cleans up after us */
    Status = ExReinitializeResourceLite(&Res);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckResourceFields((PERESOURCE_2K3)&Res);
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
