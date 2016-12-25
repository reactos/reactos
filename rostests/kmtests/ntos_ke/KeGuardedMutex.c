/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Guarded Mutex test
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <kmt_test.h>

#define NDEBUG
#include <debug.h>

static
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_max_(DISPATCH_LEVEL)
BOOLEAN
(NTAPI
*pKeAreAllApcsDisabled)(VOID);

static
_Acquires_lock_(_Global_critical_region_)
_Requires_lock_not_held_(*Mutex)
_Acquires_lock_(*Mutex)
_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
VOID
(FASTCALL
*pKeAcquireGuardedMutex)(
    _Inout_ PKGUARDED_MUTEX GuardedMutex);

static
_Requires_lock_not_held_(*FastMutex)
_Acquires_lock_(*FastMutex)
_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
VOID
(FASTCALL
*pKeAcquireGuardedMutexUnsafe)(
    _Inout_ PKGUARDED_MUTEX GuardedMutex);

static
_Acquires_lock_(_Global_critical_region_)
_IRQL_requires_max_(APC_LEVEL)
VOID
(NTAPI
*pKeEnterGuardedRegion)(VOID);

static
_Releases_lock_(_Global_critical_region_)
_IRQL_requires_max_(APC_LEVEL)
VOID
(NTAPI
*pKeLeaveGuardedRegion)(VOID);

static
_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
VOID
(FASTCALL
*pKeInitializeGuardedMutex)(
    _Out_ PKGUARDED_MUTEX GuardedMutex);

static
_Requires_lock_held_(*FastMutex)
_Releases_lock_(*FastMutex)
_IRQL_requires_max_(APC_LEVEL)
VOID
(FASTCALL
*pKeReleaseGuardedMutexUnsafe)(
    _Inout_ PKGUARDED_MUTEX GuardedMutex);

static
_Releases_lock_(_Global_critical_region_)
_Requires_lock_held_(*Mutex)
_Releases_lock_(*Mutex)
_IRQL_requires_max_(APC_LEVEL)
VOID
(FASTCALL
*pKeReleaseGuardedMutex)(
    _Inout_ PKGUARDED_MUTEX GuardedMutex);

static
_Must_inspect_result_
_Success_(return != FALSE)
_IRQL_requires_max_(APC_LEVEL)
_Post_satisfies_(return == 1 || return == 0)
BOOLEAN
(FASTCALL
*pKeTryToAcquireGuardedMutex)(
    _When_ (return, _Requires_lock_not_held_(*_Curr_) _Acquires_exclusive_lock_(*_Curr_)) _Acquires_lock_(_Global_critical_region_)
        _Inout_ PKGUARDED_MUTEX GuardedMutex);

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
    ok_eq_bool(pKeAreAllApcsDisabled(), AllApcsDisabled);                       \
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
        pKeAcquireGuardedMutex(Mutex);
        CheckMutex(Mutex, 0L, Thread, 0LU, 0x5555, SpecialApcsDisabled - 1, KernelApcsDisabled, SpecialApcsDisabled - 1, TRUE, OriginalIrql);
        ok_bool_false(pKeTryToAcquireGuardedMutex(Mutex), "KeTryToAcquireGuardedMutex returned");
        CheckMutex(Mutex, 0L, Thread, 0LU, 0x5555, SpecialApcsDisabled - 1, KernelApcsDisabled, SpecialApcsDisabled - 1, TRUE, OriginalIrql);
        pKeReleaseGuardedMutex(Mutex);
        CheckMutex(Mutex, 1L, NULL, 0LU, 0x5555, SpecialApcsDisabled - 1, KernelApcsDisabled, SpecialApcsDisabled, AllApcsDisabled, OriginalIrql);

        /* try to acquire */
        ok_bool_true(pKeTryToAcquireGuardedMutex(Mutex), "KeTryToAcquireGuardedMutex returned");
        CheckMutex(Mutex, 0L, Thread, 0LU, 0x5555, SpecialApcsDisabled - 1, KernelApcsDisabled, SpecialApcsDisabled - 1, TRUE, OriginalIrql);
        pKeReleaseGuardedMutex(Mutex);
        CheckMutex(Mutex, 1L, NULL, 0LU, 0x5555, SpecialApcsDisabled - 1, KernelApcsDisabled, SpecialApcsDisabled, AllApcsDisabled, OriginalIrql);
    }
    else
        /* Make the following test happy */
        Mutex->SpecialApcDisable = SpecialApcsDisabled - 1;

    /* ASSERT */
    if (!KmtIsCheckedBuild || OriginalIrql == APC_LEVEL || SpecialApcsDisabled < 0)
    {
        /* acquire/release unsafe */
        pKeAcquireGuardedMutexUnsafe(Mutex);
        CheckMutex(Mutex, 0L, Thread, 0LU, 0x5555, SpecialApcsDisabled - 1, KernelApcsDisabled, SpecialApcsDisabled, AllApcsDisabled, OriginalIrql);
        pKeReleaseGuardedMutexUnsafe(Mutex);
        CheckMutex(Mutex, 1L, NULL, 0LU, 0x5555, SpecialApcsDisabled - 1, KernelApcsDisabled, SpecialApcsDisabled, AllApcsDisabled, OriginalIrql);
    }

    /* Bugchecks >= DISPATCH_LEVEL */
    if (!KmtIsCheckedBuild)
    {
        /* mismatched acquire/release */
        pKeAcquireGuardedMutex(Mutex);
        CheckMutex(Mutex, 0L, Thread, 0LU, 0x5555, SpecialApcsDisabled - 1, KernelApcsDisabled, SpecialApcsDisabled - 1, TRUE, OriginalIrql);
        pKeReleaseGuardedMutexUnsafe(Mutex);
        CheckMutex(Mutex, 1L, NULL, 0LU, 0x5555, SpecialApcsDisabled - 1, KernelApcsDisabled, SpecialApcsDisabled - 1, TRUE, OriginalIrql);
        pKeLeaveGuardedRegion();
        CheckMutex(Mutex, 1L, NULL, 0LU, 0x5555, SpecialApcsDisabled - 1, KernelApcsDisabled, SpecialApcsDisabled, AllApcsDisabled, OriginalIrql);

        pKeAcquireGuardedMutexUnsafe(Mutex);
        CheckMutex(Mutex, 0L, Thread, 0LU, 0x5555, SpecialApcsDisabled - 1, KernelApcsDisabled, SpecialApcsDisabled, AllApcsDisabled, OriginalIrql);
        pKeReleaseGuardedMutex(Mutex);
        CheckMutex(Mutex, 1L, NULL, 0LU, 0x5555, SpecialApcsDisabled - 1, KernelApcsDisabled, SpecialApcsDisabled + 1, OriginalIrql >= APC_LEVEL || SpecialApcsDisabled != -1, OriginalIrql);
        pKeEnterGuardedRegion();
        CheckMutex(Mutex, 1L, NULL, 0LU, 0x5555, SpecialApcsDisabled - 1, KernelApcsDisabled, SpecialApcsDisabled, AllApcsDisabled, OriginalIrql);

        /* release without acquire */
        pKeReleaseGuardedMutexUnsafe(Mutex);
        CheckMutex(Mutex, 0L, NULL, 0LU, 0x5555, SpecialApcsDisabled - 1, KernelApcsDisabled, SpecialApcsDisabled, AllApcsDisabled, OriginalIrql);
        pKeReleaseGuardedMutex(Mutex);
        CheckMutex(Mutex, 1L, NULL, 0LU, 0x5555, SpecialApcsDisabled, KernelApcsDisabled, SpecialApcsDisabled + 1, OriginalIrql >= APC_LEVEL || SpecialApcsDisabled != -1, OriginalIrql);
        pKeReleaseGuardedMutex(Mutex);
        /* TODO: here we see that Mutex->Count isn't actually just a count. Test the bits correctly! */
        CheckMutex(Mutex, 0L, NULL, 0LU, 0x5555, SpecialApcsDisabled, KernelApcsDisabled, SpecialApcsDisabled + 2, OriginalIrql >= APC_LEVEL || SpecialApcsDisabled != -2, OriginalIrql);
        pKeReleaseGuardedMutex(Mutex);
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

typedef VOID (FASTCALL *PMUTEX_FUNCTION)(PKGUARDED_MUTEX);
typedef BOOLEAN (FASTCALL *PMUTEX_TRY_FUNCTION)(PKGUARDED_MUTEX);

typedef struct
{
    HANDLE Handle;
    PKTHREAD Thread;
    KIRQL Irql;
    PKGUARDED_MUTEX Mutex;
    PMUTEX_FUNCTION Acquire;
    PMUTEX_TRY_FUNCTION TryAcquire;
    PMUTEX_FUNCTION Release;
    BOOLEAN Try;
    BOOLEAN RetExpected;
    KEVENT InEvent;
    KEVENT OutEvent;
} THREAD_DATA, *PTHREAD_DATA;

static
VOID
NTAPI
AcquireMutexThread(
    PVOID Parameter)
{
    PTHREAD_DATA ThreadData = Parameter;
    KIRQL Irql;
    BOOLEAN Ret = FALSE;
    NTSTATUS Status;

    DPRINT("Thread starting\n");
    KeRaiseIrql(ThreadData->Irql, &Irql);

    if (ThreadData->Try)
    {
        Ret = ThreadData->TryAcquire(ThreadData->Mutex);
        ok_eq_bool(Ret, ThreadData->RetExpected);
    }
    else
        ThreadData->Acquire(ThreadData->Mutex);

    ok_bool_false(KeSetEvent(&ThreadData->OutEvent, 0, TRUE), "KeSetEvent returned");
    DPRINT("Thread now waiting\n");
    Status = KeWaitForSingleObject(&ThreadData->InEvent, Executive, KernelMode, FALSE, NULL);
    DPRINT("Thread done waiting\n");
    ok_eq_hex(Status, STATUS_SUCCESS);

    if (!ThreadData->Try || Ret)
        ThreadData->Release(ThreadData->Mutex);

    KeLowerIrql(Irql);
    DPRINT("Thread exiting\n");
}

static
VOID
InitThreadData(
    PTHREAD_DATA ThreadData,
    PKGUARDED_MUTEX Mutex,
    PMUTEX_FUNCTION Acquire,
    PMUTEX_TRY_FUNCTION TryAcquire,
    PMUTEX_FUNCTION Release)
{
    ThreadData->Mutex = Mutex;
    KeInitializeEvent(&ThreadData->InEvent, NotificationEvent, FALSE);
    KeInitializeEvent(&ThreadData->OutEvent, NotificationEvent, FALSE);
    ThreadData->Acquire = Acquire;
    ThreadData->TryAcquire = TryAcquire;
    ThreadData->Release = Release;
}

static
NTSTATUS
StartThread(
    PTHREAD_DATA ThreadData,
    PLARGE_INTEGER Timeout,
    KIRQL Irql,
    BOOLEAN Try,
    BOOLEAN RetExpected)
{
    NTSTATUS Status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES Attributes;

    ThreadData->Try = Try;
    ThreadData->Irql = Irql;
    ThreadData->RetExpected = RetExpected;
    InitializeObjectAttributes(&Attributes, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
    Status = PsCreateSystemThread(&ThreadData->Handle, GENERIC_ALL, &Attributes, NULL, NULL, AcquireMutexThread, ThreadData);
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
TestGuardedMutexConcurrent(
    PKGUARDED_MUTEX Mutex)
{
    NTSTATUS Status;
    THREAD_DATA ThreadData;
    THREAD_DATA ThreadData2;
    THREAD_DATA ThreadDataUnsafe;
    THREAD_DATA ThreadDataTry;
    PKTHREAD Thread = KeGetCurrentThread();
    LARGE_INTEGER Timeout;
    Timeout.QuadPart = -50 * 1000 * 10; /* 50 ms */

    InitThreadData(&ThreadData, Mutex, pKeAcquireGuardedMutex, NULL, pKeReleaseGuardedMutex);
    InitThreadData(&ThreadData2, Mutex, pKeAcquireGuardedMutex, NULL, pKeReleaseGuardedMutex);
    InitThreadData(&ThreadDataUnsafe, Mutex, pKeAcquireGuardedMutexUnsafe, NULL, pKeReleaseGuardedMutexUnsafe);
    InitThreadData(&ThreadDataTry, Mutex, NULL, pKeTryToAcquireGuardedMutex, pKeReleaseGuardedMutex);

    /* have a thread acquire the mutex */
    Status = StartThread(&ThreadData, NULL, PASSIVE_LEVEL, FALSE, FALSE);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckMutex(Mutex, 0L, ThreadData.Thread, 0LU, 0x5555, -1, 0, 0, FALSE, PASSIVE_LEVEL);
    /* have a second thread try to acquire it -- should fail */
    Status = StartThread(&ThreadDataTry, NULL, PASSIVE_LEVEL, TRUE, FALSE);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckMutex(Mutex, 0L, ThreadData.Thread, 0LU, 0x5555, -1, 0, 0, FALSE, PASSIVE_LEVEL);
    FinishThread(&ThreadDataTry);

    /* have another thread acquire it -- should block */
    Status = StartThread(&ThreadData2, &Timeout, APC_LEVEL, FALSE, FALSE);
    ok_eq_hex(Status, STATUS_TIMEOUT);
    CheckMutex(Mutex, 4L, ThreadData.Thread, 1LU, 0x5555, -1, 0, 0, FALSE, PASSIVE_LEVEL);

    /* finish the first thread -- now the second should become available */
    FinishThread(&ThreadData);
    Status = KeWaitForSingleObject(&ThreadData2.OutEvent, Executive, KernelMode, FALSE, NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckMutex(Mutex, 0L, ThreadData2.Thread, 1LU, 0x5555, -1, 0, 0, FALSE, PASSIVE_LEVEL);

    /* block two more threads */
    Status = StartThread(&ThreadDataUnsafe, &Timeout, APC_LEVEL, FALSE, FALSE);
    ok_eq_hex(Status, STATUS_TIMEOUT);
    CheckMutex(Mutex, 4L, ThreadData2.Thread, 2LU, 0x5555, -1, 0, 0, FALSE, PASSIVE_LEVEL);

    Status = StartThread(&ThreadData, &Timeout, PASSIVE_LEVEL, FALSE, FALSE);
    ok_eq_hex(Status, STATUS_TIMEOUT);
    CheckMutex(Mutex, 8L, ThreadData2.Thread, 3LU, 0x5555, -1, 0, 0, FALSE, PASSIVE_LEVEL);

    /* finish 1 */
    FinishThread(&ThreadData2);
    Status = KeWaitForSingleObject(&ThreadDataUnsafe.OutEvent, Executive, KernelMode, FALSE, NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckMutex(Mutex, 4L, ThreadDataUnsafe.Thread, 3LU, 0x5555, -1, 0, 0, FALSE, PASSIVE_LEVEL);

    /* finish 2 */
    FinishThread(&ThreadDataUnsafe);
    Status = KeWaitForSingleObject(&ThreadData.OutEvent, Executive, KernelMode, FALSE, NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckMutex(Mutex, 0L, ThreadData.Thread, 3LU, 0x5555, -1, 0, 0, FALSE, PASSIVE_LEVEL);

    /* finish 3 */
    FinishThread(&ThreadData);

    CheckMutex(Mutex, 1L, NULL, 3LU, 0x5555, -1, 0, 0, FALSE, PASSIVE_LEVEL);
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

    pKeAreAllApcsDisabled = KmtGetSystemRoutineAddress(L"KeAreAllApcsDisabled");
    pKeInitializeGuardedMutex = KmtGetSystemRoutineAddress(L"KeInitializeGuardedMutex");
    pKeAcquireGuardedMutex = KmtGetSystemRoutineAddress(L"KeAcquireGuardedMutex");
    pKeAcquireGuardedMutexUnsafe = KmtGetSystemRoutineAddress(L"KeAcquireGuardedMutexUnsafe");
    pKeEnterGuardedRegion = KmtGetSystemRoutineAddress(L"KeEnterGuardedRegion");
    pKeLeaveGuardedRegion = KmtGetSystemRoutineAddress(L"KeLeaveGuardedRegion");
    pKeReleaseGuardedMutex = KmtGetSystemRoutineAddress(L"KeReleaseGuardedMutex");
    pKeReleaseGuardedMutexUnsafe = KmtGetSystemRoutineAddress(L"KeReleaseGuardedMutexUnsafe");
    pKeTryToAcquireGuardedMutex = KmtGetSystemRoutineAddress(L"KeTryToAcquireGuardedMutex");

    if (skip(pKeAreAllApcsDisabled &&
             pKeInitializeGuardedMutex &&
             pKeAcquireGuardedMutex &&
             pKeAcquireGuardedMutexUnsafe &&
             pKeEnterGuardedRegion &&
             pKeLeaveGuardedRegion &&
             pKeReleaseGuardedMutex &&
             pKeReleaseGuardedMutexUnsafe &&
             pKeTryToAcquireGuardedMutex, "No guarded mutexes\n"))
    {
        return;
    }

    for (i = 0; i < sizeof TestIterations / sizeof TestIterations[0]; ++i)
    {
        trace("Run %d\n", i);
        KeRaiseIrql(TestIterations[i].Irql, &OldIrql);
        Thread->KernelApcDisable = TestIterations[i].KernelApcsDisabled;
        Thread->SpecialApcDisable = TestIterations[i].SpecialApcsDisabled;

        RtlFillMemory(&Mutex, sizeof Mutex, 0x55);
        pKeInitializeGuardedMutex(&Mutex);
        CheckMutex(&Mutex, 1L, NULL, 0LU, 0x5555, 0x5555, TestIterations[i].KernelApcsDisabled, TestIterations[i].SpecialApcsDisabled, TestIterations[i].AllApcsDisabled, TestIterations[i].Irql);
        TestGuardedMutex(&Mutex, TestIterations[i].KernelApcsDisabled, TestIterations[i].SpecialApcsDisabled, TestIterations[i].AllApcsDisabled, TestIterations[i].Irql);

        Thread->SpecialApcDisable = 0;
        Thread->KernelApcDisable = 0;
        KeLowerIrql(OldIrql);
    }

    trace("Concurrent test\n");
    RtlFillMemory(&Mutex, sizeof Mutex, 0x55);
    pKeInitializeGuardedMutex(&Mutex);
    TestGuardedMutexConcurrent(&Mutex);
}
