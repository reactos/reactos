/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Guarded Mutex test
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

#include <kmt_test.h>

#define NDEBUG
#include <debug.h>

#define CheckMutex(Mutex, ExpectedCount, ExpectedOwner, ExpectedContention,     \
                   ExpectedKernelApcDisable, ExpectedSpecialApcDisable,         \
                   KernelApcsDisabled, SpecialApcsDisabled, AllApcsDisabled,    \
                   ExpectedIrql) do                                             \
{                                                                               \
    ok_eq_long((Mutex)->Count, ExpectedCount);                                  \
    ok_eq_pointer((Mutex)->Owner, ExpectedOwner);                               \
    ok_eq_ulong((Mutex)->Contention, ExpectedContention);                       \
    ok_eq_int((Mutex)->KernelApcDisable, ExpectedKernelApcDisable);             \
    if (KmtIsCheckedBuild)                                                      \
        ok_eq_int((Mutex)->SpecialApcDisable, ExpectedSpecialApcDisable);       \
    else                                                                        \
        ok_eq_int((Mutex)->SpecialApcDisable, 0x5555);                          \
    ok_eq_bool(KeAreApcsDisabled(), KernelApcsDisabled || SpecialApcsDisabled); \
    ok_eq_int(Thread->KernelApcDisable, KernelApcsDisabled);                    \
    ok_eq_bool(KeAreAllApcsDisabled(), AllApcsDisabled);                        \
    ok_eq_int(Thread->SpecialApcDisable, SpecialApcsDisabled);                  \
    ok_irql(ExpectedIrql);                                                      \
} while (0)

static
VOID
TestGuardedMutex(
    PKGUARDED_MUTEX Mutex,
    SHORT KernelApcsDisabled,
    SHORT SpecialApcsDisabled,
    SHORT AllApcsDisabled,
    KIRQL OriginalIrql)
{
    PKTHREAD Thread = KeGetCurrentThread();

    ok_irql(OriginalIrql);
    CheckMutex(Mutex, 1L, NULL, 0LU, 0x5555, 0x5555, KernelApcsDisabled, SpecialApcsDisabled, AllApcsDisabled, OriginalIrql);

    /* these ASSERT */
    if (!KmtIsCheckedBuild || OriginalIrql <= APC_LEVEL)
    {
        /* acquire/release normally */
        KeAcquireGuardedMutex(Mutex);
        CheckMutex(Mutex, 0L, Thread, 0LU, 0x5555, SpecialApcsDisabled - 1, KernelApcsDisabled, SpecialApcsDisabled - 1, TRUE, OriginalIrql);
        ok_bool_false(KeTryToAcquireGuardedMutex(Mutex), "KeTryToAcquireGuardedMutex returned");
        CheckMutex(Mutex, 0L, Thread, 0LU, 0x5555, SpecialApcsDisabled - 1, KernelApcsDisabled, SpecialApcsDisabled - 1, TRUE, OriginalIrql);
        KeReleaseGuardedMutex(Mutex);
        CheckMutex(Mutex, 1L, NULL, 0LU, 0x5555, SpecialApcsDisabled - 1, KernelApcsDisabled, SpecialApcsDisabled, AllApcsDisabled, OriginalIrql);

        /* try to acquire */
        ok_bool_true(KeTryToAcquireGuardedMutex(Mutex), "KeTryToAcquireGuardedMutex returned");
        CheckMutex(Mutex, 0L, Thread, 0LU, 0x5555, SpecialApcsDisabled - 1, KernelApcsDisabled, SpecialApcsDisabled - 1, TRUE, OriginalIrql);
        KeReleaseGuardedMutex(Mutex);
        CheckMutex(Mutex, 1L, NULL, 0LU, 0x5555, SpecialApcsDisabled - 1, KernelApcsDisabled, SpecialApcsDisabled, AllApcsDisabled, OriginalIrql);
    }
    else
        /* Make the following test happy */
        Mutex->SpecialApcDisable = SpecialApcsDisabled - 1;

    /* ASSERT */
    if (!KmtIsCheckedBuild || OriginalIrql == APC_LEVEL || SpecialApcsDisabled < 0)
    {
        /* acquire/release unsafe */
        KeAcquireGuardedMutexUnsafe(Mutex);
        CheckMutex(Mutex, 0L, Thread, 0LU, 0x5555, SpecialApcsDisabled - 1, KernelApcsDisabled, SpecialApcsDisabled, AllApcsDisabled, OriginalIrql);
        KeReleaseGuardedMutexUnsafe(Mutex);
        CheckMutex(Mutex, 1L, NULL, 0LU, 0x5555, SpecialApcsDisabled - 1, KernelApcsDisabled, SpecialApcsDisabled, AllApcsDisabled, OriginalIrql);
    }

    /* Bugchecks >= DISPATCH_LEVEL */
    if (!KmtIsCheckedBuild)
    {
        /* mismatched acquire/release */
        KeAcquireGuardedMutex(Mutex);
        CheckMutex(Mutex, 0L, Thread, 0LU, 0x5555, SpecialApcsDisabled - 1, KernelApcsDisabled, SpecialApcsDisabled - 1, TRUE, OriginalIrql);
        KeReleaseGuardedMutexUnsafe(Mutex);
        CheckMutex(Mutex, 1L, NULL, 0LU, 0x5555, SpecialApcsDisabled - 1, KernelApcsDisabled, SpecialApcsDisabled - 1, TRUE, OriginalIrql);
        KeLeaveGuardedRegion();
        CheckMutex(Mutex, 1L, NULL, 0LU, 0x5555, SpecialApcsDisabled - 1, KernelApcsDisabled, SpecialApcsDisabled, AllApcsDisabled, OriginalIrql);

        KeAcquireGuardedMutexUnsafe(Mutex);
        CheckMutex(Mutex, 0L, Thread, 0LU, 0x5555, SpecialApcsDisabled - 1, KernelApcsDisabled, SpecialApcsDisabled, AllApcsDisabled, OriginalIrql);
        KeReleaseGuardedMutex(Mutex);
        CheckMutex(Mutex, 1L, NULL, 0LU, 0x5555, SpecialApcsDisabled - 1, KernelApcsDisabled, SpecialApcsDisabled + 1, OriginalIrql >= APC_LEVEL || SpecialApcsDisabled != -1, OriginalIrql);
        KeEnterGuardedRegion();
        CheckMutex(Mutex, 1L, NULL, 0LU, 0x5555, SpecialApcsDisabled - 1, KernelApcsDisabled, SpecialApcsDisabled, AllApcsDisabled, OriginalIrql);

        /* release without acquire */
        KeReleaseGuardedMutexUnsafe(Mutex);
        CheckMutex(Mutex, 0L, NULL, 0LU, 0x5555, SpecialApcsDisabled - 1, KernelApcsDisabled, SpecialApcsDisabled, AllApcsDisabled, OriginalIrql);
        KeReleaseGuardedMutex(Mutex);
        CheckMutex(Mutex, 1L, NULL, 0LU, 0x5555, SpecialApcsDisabled, KernelApcsDisabled, SpecialApcsDisabled + 1, OriginalIrql >= APC_LEVEL || SpecialApcsDisabled != -1, OriginalIrql);
        KeReleaseGuardedMutex(Mutex);
        /* TODO: here we see that Mutex->Count isn't actually just a count. Test the bits correctly! */
        CheckMutex(Mutex, 0L, NULL, 0LU, 0x5555, SpecialApcsDisabled, KernelApcsDisabled, SpecialApcsDisabled + 2, OriginalIrql >= APC_LEVEL || SpecialApcsDisabled != -2, OriginalIrql);
        KeReleaseGuardedMutex(Mutex);
        CheckMutex(Mutex, 1L, NULL, 0LU, 0x5555, SpecialApcsDisabled, KernelApcsDisabled, SpecialApcsDisabled + 3, OriginalIrql >= APC_LEVEL || SpecialApcsDisabled != -3, OriginalIrql);
        Thread->SpecialApcDisable -= 3;
    }

    /* make sure we survive this in case of error */
    ok_eq_long(Mutex->Count, 1L);
    Mutex->Count = 1;
    ok_eq_int(Thread->KernelApcDisable, KernelApcsDisabled);
    Thread->KernelApcDisable = KernelApcsDisabled;
    ok_eq_int(Thread->SpecialApcDisable, SpecialApcsDisabled);
    Thread->SpecialApcDisable = SpecialApcsDisabled;
    ok_irql(OriginalIrql);
}

START_TEST(KeGuardedMutex)
{
    KGUARDED_MUTEX Mutex;
    KIRQL OldIrql;
    PKTHREAD Thread = KeGetCurrentThread();
    struct {
        KIRQL Irql;
        SHORT KernelApcsDisabled;
        SHORT SpecialApcsDisabled;
        BOOLEAN AllApcsDisabled;
    } TestIterations[] =
    {
        { PASSIVE_LEVEL,   0,  0, FALSE },
        { PASSIVE_LEVEL,  -1,  0, FALSE },
        { PASSIVE_LEVEL,  -3,  0, FALSE },
        { PASSIVE_LEVEL,   0, -1, TRUE },
        { PASSIVE_LEVEL,  -1, -1, TRUE },
        { PASSIVE_LEVEL,  -3, -2, TRUE },
        // 6
        { APC_LEVEL,       0,  0, TRUE },
        { APC_LEVEL,      -1,  0, TRUE },
        { APC_LEVEL,      -3,  0, TRUE },
        { APC_LEVEL,       0, -1, TRUE },
        { APC_LEVEL,      -1, -1, TRUE },
        { APC_LEVEL,      -3, -2, TRUE },
        // 12
        { DISPATCH_LEVEL,  0,  0, TRUE },
        { DISPATCH_LEVEL, -1,  0, TRUE },
        { DISPATCH_LEVEL, -3,  0, TRUE },
        { DISPATCH_LEVEL,  0, -1, TRUE },
        { DISPATCH_LEVEL, -1, -1, TRUE },
        { DISPATCH_LEVEL, -3, -2, TRUE },
        // 18
        { HIGH_LEVEL,      0,  0, TRUE },
        { HIGH_LEVEL,     -1,  0, TRUE },
        { HIGH_LEVEL,     -3,  0, TRUE },
        { HIGH_LEVEL,      0, -1, TRUE },
        { HIGH_LEVEL,     -1, -1, TRUE },
        { HIGH_LEVEL,     -3, -2, TRUE },
    };
    int i;

    for (i = 0; i < sizeof TestIterations / sizeof TestIterations[0]; ++i)
    {
        trace("Run %d\n", i);
        KeRaiseIrql(TestIterations[i].Irql, &OldIrql);
        Thread->KernelApcDisable = TestIterations[i].KernelApcsDisabled;
        Thread->SpecialApcDisable = TestIterations[i].SpecialApcsDisabled;

        RtlFillMemory(&Mutex, sizeof Mutex, 0x55);
        KeInitializeGuardedMutex(&Mutex);
        CheckMutex(&Mutex, 1L, NULL, 0LU, 0x5555, 0x5555, TestIterations[i].KernelApcsDisabled, TestIterations[i].SpecialApcsDisabled, TestIterations[i].AllApcsDisabled, TestIterations[i].Irql);
        TestGuardedMutex(&Mutex, TestIterations[i].KernelApcsDisabled, TestIterations[i].SpecialApcsDisabled, TestIterations[i].AllApcsDisabled, TestIterations[i].Irql);

        Thread->SpecialApcDisable = 0;
        Thread->KernelApcDisable = 0;
        KeLowerIrql(OldIrql);
    }
}
