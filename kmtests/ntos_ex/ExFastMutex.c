/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Fast Mutex test
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

#include <kmt_test.h>

//#define NDEBUG
#include <debug.h>

NTKERNELAPI VOID    FASTCALL ExiAcquireFastMutex(IN OUT PFAST_MUTEX FastMutex);
NTKERNELAPI VOID    FASTCALL ExiReleaseFastMutex(IN OUT PFAST_MUTEX FastMutex);
NTKERNELAPI BOOLEAN FASTCALL ExiTryToAcquireFastMutex(IN OUT PFAST_MUTEX FastMutex);

#define CheckMutex(Mutex, ExpectedCount, ExpectedOwner,                 \
                   ExpectedContention, ExpectedOldIrql,                 \
                   ExpectedIrql) do                                     \
{                                                                       \
    ok_eq_long((Mutex)->Count, ExpectedCount);                          \
    ok_eq_pointer((Mutex)->Owner, ExpectedOwner);                       \
    ok_eq_ulong((Mutex)->Contention, ExpectedContention);               \
    ok_eq_ulong((Mutex)->OldIrql, (ULONG)ExpectedOldIrql);              \
    ok_bool_false(KeAreApcsDisabled(), "KeAreApcsDisabled returned");   \
    ok_irql(ExpectedIrql);                                              \
} while (0)

static
VOID
TestFastMutex(
    PFAST_MUTEX Mutex,
    KIRQL OriginalIrql)
{
    PKTHREAD Thread = KeGetCurrentThread();

    ok_irql(OriginalIrql);

    /* acquire/release normally */
    ExAcquireFastMutex(Mutex);
    CheckMutex(Mutex, 0L, Thread, 0LU, OriginalIrql, APC_LEVEL);
    ok_bool_false(ExTryToAcquireFastMutex(Mutex), "ExTryToAcquireFastMutex returned");
    CheckMutex(Mutex, 0L, Thread, 0LU, OriginalIrql, APC_LEVEL);
    ExReleaseFastMutex(Mutex);
    CheckMutex(Mutex, 1L, NULL, 0LU, OriginalIrql, OriginalIrql);

    /* ntoskrnl's fastcall version */
    ExiAcquireFastMutex(Mutex);
    CheckMutex(Mutex, 0L, Thread, 0LU, OriginalIrql, APC_LEVEL);
    ok_bool_false(ExiTryToAcquireFastMutex(Mutex), "ExiTryToAcquireFastMutex returned");
    CheckMutex(Mutex, 0L, Thread, 0LU, OriginalIrql, APC_LEVEL);
    ExiReleaseFastMutex(Mutex);
    CheckMutex(Mutex, 1L, NULL, 0LU, OriginalIrql, OriginalIrql);

    /* acquire/release unsafe */
    ExAcquireFastMutexUnsafe(Mutex);
    CheckMutex(Mutex, 0L, Thread, 0LU, OriginalIrql, OriginalIrql);
    ExReleaseFastMutexUnsafe(Mutex);
    CheckMutex(Mutex, 1L, NULL, 0LU, OriginalIrql, OriginalIrql);

    /* try to acquire */
    ok_bool_true(ExTryToAcquireFastMutex(Mutex), "ExTryToAcquireFastMutex returned");
    CheckMutex(Mutex, 0L, Thread, 0LU, OriginalIrql, APC_LEVEL);
    ExReleaseFastMutex(Mutex);
    CheckMutex(Mutex, 1L, NULL, 0LU, OriginalIrql, OriginalIrql);

    /* shortcut functions with critical region */
    ExEnterCriticalRegionAndAcquireFastMutexUnsafe(Mutex);
    ok_bool_true(KeAreApcsDisabled(), "KeAreApcsDisabled returned");
    ExReleaseFastMutexUnsafeAndLeaveCriticalRegion(Mutex);

    /* mismatched acquire/release */
    ExAcquireFastMutex(Mutex);
    CheckMutex(Mutex, 0L, Thread, 0LU, OriginalIrql, APC_LEVEL);
    ExReleaseFastMutexUnsafe(Mutex);
    CheckMutex(Mutex, 1L, NULL, 0LU, OriginalIrql, APC_LEVEL);
    KmtSetIrql(OriginalIrql);
    CheckMutex(Mutex, 1L, NULL, 0LU, OriginalIrql, OriginalIrql);

    Mutex->OldIrql = 0x55555555LU;
    ExAcquireFastMutexUnsafe(Mutex);
    CheckMutex(Mutex, 0L, Thread, 0LU, 0x55555555LU, OriginalIrql);
    Mutex->OldIrql = PASSIVE_LEVEL;
    ExReleaseFastMutex(Mutex);
    CheckMutex(Mutex, 1L, NULL, 0LU, PASSIVE_LEVEL, PASSIVE_LEVEL);
    KmtSetIrql(OriginalIrql);
    CheckMutex(Mutex, 1L, NULL, 0LU, PASSIVE_LEVEL, OriginalIrql);

    /* release without acquire */
    ExReleaseFastMutexUnsafe(Mutex);
    CheckMutex(Mutex, 2L, NULL, 0LU, PASSIVE_LEVEL, OriginalIrql);
    --Mutex->Count;
    Mutex->OldIrql = OriginalIrql;
    ExReleaseFastMutex(Mutex);
    CheckMutex(Mutex, 2L, NULL, 0LU, OriginalIrql, OriginalIrql);
    ExReleaseFastMutex(Mutex);
    CheckMutex(Mutex, 3L, NULL, 0LU, OriginalIrql, OriginalIrql);
    Mutex->Count -= 2;

    /* make sure we survive this in case of error */
    ok_eq_long(Mutex->Count, 1L);
    Mutex->Count = 1;
    ok_irql(OriginalIrql);
    KmtSetIrql(OriginalIrql);
}

START_TEST(ExFastMutex)
{
    FAST_MUTEX Mutex;
    KIRQL Irql;

    memset(&Mutex, 0x55, sizeof Mutex);
    ExInitializeFastMutex(&Mutex);
    CheckMutex(&Mutex, 1L, NULL, 0LU, 0x55555555LU, PASSIVE_LEVEL);

    TestFastMutex(&Mutex, PASSIVE_LEVEL);
    KeRaiseIrql(APC_LEVEL, &Irql);
    TestFastMutex(&Mutex, APC_LEVEL);
    if (!KmtIsCheckedBuild)
    {
        KeRaiseIrql(DISPATCH_LEVEL, &Irql);
        TestFastMutex(&Mutex, DISPATCH_LEVEL);
        KeRaiseIrql(HIGH_LEVEL, &Irql);
        TestFastMutex(&Mutex, HIGH_LEVEL);
    }
    KeLowerIrql(PASSIVE_LEVEL);
}
