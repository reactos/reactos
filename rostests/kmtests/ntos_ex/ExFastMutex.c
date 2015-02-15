/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Fast Mutex test
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <kmt_test.h>

//#define NDEBUG
#include <debug.h>

static
VOID
(FASTCALL
*pExEnterCriticalRegionAndAcquireFastMutexUnsafe)(
    _Inout_ PFAST_MUTEX FastMutex
);

static
VOID
(FASTCALL
*pExReleaseFastMutexUnsafeAndLeaveCriticalRegion)(
    _Inout_ PFAST_MUTEX FastMutex
);

static VOID    (FASTCALL *pExiAcquireFastMutex)(IN OUT PFAST_MUTEX FastMutex);
static VOID    (FASTCALL *pExiReleaseFastMutex)(IN OUT PFAST_MUTEX FastMutex);
static BOOLEAN (FASTCALL *pExiTryToAcquireFastMutex)(IN OUT PFAST_MUTEX FastMutex);

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
    if (!skip(pExiAcquireFastMutex &&
              pExiReleaseFastMutex &&
              pExiTryToAcquireFastMutex, "No fastcall fast mutex functions\n"))
    {
        pExiAcquireFastMutex(Mutex);
        CheckMutex(Mutex, 0L, Thread, 0LU, OriginalIrql, APC_LEVEL);
        ok_bool_false(pExiTryToAcquireFastMutex(Mutex), "ExiTryToAcquireFastMutex returned");
        CheckMutex(Mutex, 0L, Thread, 0LU, OriginalIrql, APC_LEVEL);
        pExiReleaseFastMutex(Mutex);
        CheckMutex(Mutex, 1L, NULL, 0LU, OriginalIrql, OriginalIrql);
    }

    /* try to acquire */
    ok_bool_true(ExTryToAcquireFastMutex(Mutex), "ExTryToAcquireFastMutex returned");
    CheckMutex(Mutex, 0L, Thread, 0LU, OriginalIrql, APC_LEVEL);
    ExReleaseFastMutex(Mutex);
    CheckMutex(Mutex, 1L, NULL, 0LU, OriginalIrql, OriginalIrql);

    /* shortcut functions with critical region */
    if (!skip(pExEnterCriticalRegionAndAcquireFastMutexUnsafe &&
              pExReleaseFastMutexUnsafeAndLeaveCriticalRegion,
              "Shortcut functions not available"))
    {
        pExEnterCriticalRegionAndAcquireFastMutexUnsafe(Mutex);
        ok_bool_true(KeAreApcsDisabled(), "KeAreApcsDisabled returned");
        pExReleaseFastMutexUnsafeAndLeaveCriticalRegion(Mutex);
    }

    /* acquire/release unsafe */
    if (!KmtIsCheckedBuild || OriginalIrql == APC_LEVEL)
    {
        ExAcquireFastMutexUnsafe(Mutex);
        CheckMutex(Mutex, 0L, Thread, 0LU, OriginalIrql, OriginalIrql);
        ExReleaseFastMutexUnsafe(Mutex);
        CheckMutex(Mutex, 1L, NULL, 0LU, OriginalIrql, OriginalIrql);

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
    }

    if (!KmtIsCheckedBuild)
    {
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
    }

    /* make sure we survive this in case of error */
    ok_eq_long(Mutex->Count, 1L);
    Mutex->Count = 1;
    ok_irql(OriginalIrql);
    KmtSetIrql(OriginalIrql);
}

typedef VOID (FASTCALL *PMUTEX_FUNCTION)(PFAST_MUTEX);
typedef BOOLEAN (FASTCALL *PMUTEX_TRY_FUNCTION)(PFAST_MUTEX);

typedef struct
{
    HANDLE Handle;
    PKTHREAD Thread;
    KIRQL Irql;
    PFAST_MUTEX Mutex;
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

    KeRaiseIrql(ThreadData->Irql, &Irql);

    if (ThreadData->Try)
    {
        Ret = ThreadData->TryAcquire(ThreadData->Mutex);
        ok_eq_bool(Ret, ThreadData->RetExpected);
    }
    else
        ThreadData->Acquire(ThreadData->Mutex);

    ok_bool_false(KeSetEvent(&ThreadData->OutEvent, 0, TRUE), "KeSetEvent returned");
    Status = KeWaitForSingleObject(&ThreadData->InEvent, Executive, KernelMode, FALSE, NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);

    if (!ThreadData->Try || Ret)
        ThreadData->Release(ThreadData->Mutex);

    KeLowerIrql(Irql);
}

static
VOID
InitThreadData(
    PTHREAD_DATA ThreadData,
    PFAST_MUTEX Mutex,
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
TestFastMutexConcurrent(
    PFAST_MUTEX Mutex)
{
    NTSTATUS Status;
    THREAD_DATA ThreadData;
    THREAD_DATA ThreadData2;
    THREAD_DATA ThreadDataUnsafe;
    THREAD_DATA ThreadDataTry;
    LARGE_INTEGER Timeout;
    Timeout.QuadPart = -10 * 1000 * 10; /* 10 ms */

    InitThreadData(&ThreadData, Mutex, ExAcquireFastMutex, NULL, ExReleaseFastMutex);
    InitThreadData(&ThreadData2, Mutex, ExAcquireFastMutex, NULL, ExReleaseFastMutex);
    InitThreadData(&ThreadDataUnsafe, Mutex, ExAcquireFastMutexUnsafe, NULL, ExReleaseFastMutexUnsafe);
    InitThreadData(&ThreadDataTry, Mutex, NULL, ExTryToAcquireFastMutex, ExReleaseFastMutex);

    /* have a thread acquire the mutex */
    Status = StartThread(&ThreadData, NULL, PASSIVE_LEVEL, FALSE, FALSE);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckMutex(Mutex, 0L, ThreadData.Thread, 0LU, PASSIVE_LEVEL, PASSIVE_LEVEL);
    /* have a second thread try to acquire it -- should fail */
    Status = StartThread(&ThreadDataTry, NULL, PASSIVE_LEVEL, TRUE, FALSE);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckMutex(Mutex, 0L, ThreadData.Thread, 0LU, PASSIVE_LEVEL, PASSIVE_LEVEL);
    FinishThread(&ThreadDataTry);

    /* have another thread acquire it -- should block */
    Status = StartThread(&ThreadData2, &Timeout, APC_LEVEL, FALSE, FALSE);
    ok_eq_hex(Status, STATUS_TIMEOUT);
    CheckMutex(Mutex, -1L, ThreadData.Thread, 1LU, PASSIVE_LEVEL, PASSIVE_LEVEL);

    /* finish the first thread -- now the second should become available */
    FinishThread(&ThreadData);
    Status = KeWaitForSingleObject(&ThreadData2.OutEvent, Executive, KernelMode, FALSE, NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckMutex(Mutex, 0L, ThreadData2.Thread, 1LU, APC_LEVEL, PASSIVE_LEVEL);

    /* block two more threads */
    Status = StartThread(&ThreadDataUnsafe, &Timeout, APC_LEVEL, FALSE, FALSE);
    ok_eq_hex(Status, STATUS_TIMEOUT);
    CheckMutex(Mutex, -1L, ThreadData2.Thread, 2LU, APC_LEVEL, PASSIVE_LEVEL);

    Status = StartThread(&ThreadData, &Timeout, PASSIVE_LEVEL, FALSE, FALSE);
    ok_eq_hex(Status, STATUS_TIMEOUT);
    CheckMutex(Mutex, -2L, ThreadData2.Thread, 3LU, APC_LEVEL, PASSIVE_LEVEL);

    /* finish 1 */
    FinishThread(&ThreadData2);
    Status = KeWaitForSingleObject(&ThreadDataUnsafe.OutEvent, Executive, KernelMode, FALSE, NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckMutex(Mutex, -1L, ThreadDataUnsafe.Thread, 3LU, APC_LEVEL, PASSIVE_LEVEL);

    /* finish 2 */
    FinishThread(&ThreadDataUnsafe);
    Status = KeWaitForSingleObject(&ThreadData.OutEvent, Executive, KernelMode, FALSE, NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    CheckMutex(Mutex, 0L, ThreadData.Thread, 3LU, PASSIVE_LEVEL, PASSIVE_LEVEL);

    /* finish 3 */
    FinishThread(&ThreadData);

    CheckMutex(Mutex, 1L, NULL, 3LU, PASSIVE_LEVEL, PASSIVE_LEVEL);
}

START_TEST(ExFastMutex)
{
    FAST_MUTEX Mutex;
    KIRQL Irql;

    pExEnterCriticalRegionAndAcquireFastMutexUnsafe = KmtGetSystemRoutineAddress(L"ExEnterCriticalRegionAndAcquireFastMutexUnsafe");
    pExReleaseFastMutexUnsafeAndLeaveCriticalRegion = KmtGetSystemRoutineAddress(L"ExReleaseFastMutexUnsafeAndLeaveCriticalRegion");

    pExiAcquireFastMutex = KmtGetSystemRoutineAddress(L"ExiAcquireFastMutex");
    pExiReleaseFastMutex = KmtGetSystemRoutineAddress(L"ExiReleaseFastMutex");
    pExiTryToAcquireFastMutex = KmtGetSystemRoutineAddress(L"ExiTryToAcquireFastMutex");

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

    TestFastMutexConcurrent(&Mutex);
}
