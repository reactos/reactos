/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for the SRW lock try functions
 * COPYRIGHT:   Copyright 2026 Tomas Srnka <tomas.srnka@e2b.dev>
 */

#include "precomp.h"

typedef VOID (NTAPI *PFN_SRW)(PRTL_SRWLOCK);
typedef BOOLEAN (NTAPI *PFN_TRY_SRW)(PRTL_SRWLOCK);

static PFN_SRW pRtlInitializeSRWLock;
static PFN_SRW pRtlAcquireSRWLockShared;
static PFN_SRW pRtlReleaseSRWLockShared;
static PFN_SRW pRtlAcquireSRWLockExclusive;
static PFN_SRW pRtlReleaseSRWLockExclusive;
static PFN_TRY_SRW pRtlTryAcquireSRWLockShared;
static PFN_TRY_SRW pRtlTryAcquireSRWLockExclusive;

/* Every wait is bounded, so a broken lock fails the test instead of hanging it */
#define WAIT_MS 10000

typedef enum _SRW_COMMAND
{
    CmdExit,
    CmdTryShared,
    CmdProbeShared,
    CmdReleaseShared,
    CmdTryExclusive,
    CmdReleaseExclusive,
    CmdAcquireExclusive,
} SRW_COMMAND;

typedef struct _SRW_WORKER
{
    HANDLE Thread;
    HANDLE CommandEvent;
    HANDLE DoneEvent;
    PRTL_SRWLOCK Lock;
    SRW_COMMAND Command;
    BOOLEAN Result;
    NTSTATUS Status;
    BOOLEAN Stuck;
} SRW_WORKER;

/* The release functions raise an exception on a lock in the wrong state */
static
NTSTATUS
TryReleaseShared(PRTL_SRWLOCK Lock)
{
    NTSTATUS Status = STATUS_SUCCESS;

    _SEH2_TRY
    {
        pRtlReleaseSRWLockShared(Lock);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    return Status;
}

static
NTSTATUS
TryReleaseExclusive(PRTL_SRWLOCK Lock)
{
    NTSTATUS Status = STATUS_SUCCESS;

    _SEH2_TRY
    {
        pRtlReleaseSRWLockExclusive(Lock);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    return Status;
}

static
VOID
ReleaseShared(PRTL_SRWLOCK Lock)
{
    NTSTATUS Status = TryReleaseShared(Lock);

    ok(Status == STATUS_SUCCESS, "Release shared raised 0x%08lx\n", Status);
}

static
VOID
ReleaseExclusive(PRTL_SRWLOCK Lock)
{
    NTSTATUS Status = TryReleaseExclusive(Lock);

    ok(Status == STATUS_SUCCESS, "Release exclusive raised 0x%08lx\n", Status);
}

static
DWORD
WINAPI
WorkerProc(PVOID Context)
{
    SRW_WORKER *Worker = Context;

    while (WaitForSingleObject(Worker->CommandEvent, 3 * WAIT_MS) == WAIT_OBJECT_0)
    {
        Worker->Result = TRUE;

        switch (Worker->Command)
        {
            case CmdExit:
                return 0;

            case CmdTryShared:
                Worker->Result = pRtlTryAcquireSRWLockShared(Worker->Lock);
                break;

            case CmdProbeShared:
                /* No checks here, so that their number does not depend on timing */
                Worker->Result = pRtlTryAcquireSRWLockShared(Worker->Lock);
                if (Worker->Result)
                    Worker->Status = TryReleaseShared(Worker->Lock);
                break;

            case CmdReleaseShared:
                ReleaseShared(Worker->Lock);
                break;

            case CmdTryExclusive:
                Worker->Result = pRtlTryAcquireSRWLockExclusive(Worker->Lock);
                break;

            case CmdReleaseExclusive:
                ReleaseExclusive(Worker->Lock);
                break;

            case CmdAcquireExclusive:
                pRtlAcquireSRWLockExclusive(Worker->Lock);
                break;
        }

        SetEvent(Worker->DoneEvent);
    }

    return 1;
}

static
BOOLEAN
StartWorker(SRW_WORKER *Worker, PRTL_SRWLOCK Lock)
{
    RtlZeroMemory(Worker, sizeof(*Worker));
    Worker->Lock = Lock;
    Worker->CommandEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    Worker->DoneEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (Worker->CommandEvent && Worker->DoneEvent)
        Worker->Thread = CreateThread(NULL, 0, WorkerProc, Worker, 0, NULL);

    ok(Worker->Thread != NULL, "Could not create the worker thread, error %lu\n", GetLastError());
    if (!Worker->Thread)
    {
        if (Worker->CommandEvent)
            CloseHandle(Worker->CommandEvent);
        if (Worker->DoneEvent)
            CloseHandle(Worker->DoneEvent);
        return FALSE;
    }

    return TRUE;
}

/*
 * A worker that does not exit may still be inside the lock, so its handles stay open.
 * The workers and locks are static for the same reason: they must outlive their test.
 */
static
VOID
StopWorker(SRW_WORKER *Worker)
{
    DWORD Wait;

    if (Worker->Stuck)
        return;

    Worker->Command = CmdExit;
    SetEvent(Worker->CommandEvent);
    Wait = WaitForSingleObject(Worker->Thread, WAIT_MS);
    ok(Wait == WAIT_OBJECT_0, "Worker thread did not exit, wait result %lu\n", Wait);
    if (Wait != WAIT_OBJECT_0)
        return;

    CloseHandle(Worker->Thread);
    CloseHandle(Worker->CommandEvent);
    CloseHandle(Worker->DoneEvent);
}

static
VOID
StartCommand(SRW_WORKER *Worker, SRW_COMMAND Command)
{
    Worker->Command = Command;
    SetEvent(Worker->CommandEvent);
}

static
BOOLEAN
WaitCommand(SRW_WORKER *Worker, DWORD Timeout)
{
    return !Worker->Stuck && WaitForSingleObject(Worker->DoneEvent, Timeout) == WAIT_OBJECT_0;
}

/*
 * Returns the result of the command. A command that does not finish within the time limit leaves
 * the worker stuck inside the lock, and the test must not touch that lock any more: a release
 * on a lock in the wrong state can spin forever.
 */
static
BOOLEAN
RunCommand(SRW_WORKER *Worker, SRW_COMMAND Command)
{
    if (Worker->Stuck)
        return FALSE;

    StartCommand(Worker, Command);
    if (!WaitCommand(Worker, WAIT_MS))
    {
        ok(FALSE, "Worker command %d did not finish\n", Command);
        Worker->Stuck = TRUE;
        return FALSE;
    }

    return Worker->Result;
}

/* The release functions only run after the acquire reported success */
static
VOID
CheckTryExclusiveFails(PRTL_SRWLOCK Lock, const char *Description)
{
    BOOLEAN Ret = pRtlTryAcquireSRWLockExclusive(Lock);

    ok(!Ret, "Try exclusive succeeded %s\n", Description);
    if (Ret)
        ReleaseExclusive(Lock);
}

static
VOID
CheckWorkerTrySharedFails(SRW_WORKER *Worker, const char *Description)
{
    BOOLEAN Ret = RunCommand(Worker, CmdTryShared);

    ok(!Ret, "Try shared succeeded %s\n", Description);
    if (Ret)
        RunCommand(Worker, CmdReleaseShared);
}

static
VOID
CheckWorkerTryExclusiveFails(SRW_WORKER *Worker, const char *Description)
{
    BOOLEAN Ret = RunCommand(Worker, CmdTryExclusive);

    ok(!Ret, "Try exclusive succeeded %s\n", Description);
    if (Ret)
        RunCommand(Worker, CmdReleaseExclusive);
}

static
VOID
CheckLockFree(PRTL_SRWLOCK Lock, const char *Description)
{
    BOOLEAN Ret;

    ok(Lock->Ptr == NULL, "Lock is %p %s\n", Lock->Ptr, Description);

    Ret = pRtlTryAcquireSRWLockExclusive(Lock);
    ok(Ret, "Try exclusive failed %s\n", Description);
    if (Ret)
    {
        ReleaseExclusive(Lock);
        ok(Lock->Ptr == NULL, "Lock is %p after the exclusive release %s\n",
           Lock->Ptr, Description);
    }
}

/* One thread, no nesting: a try on a free lock and the release that follows */
static
VOID
TestFreeLock(VOID)
{
    RTL_SRWLOCK Lock;
    BOOLEAN Ret;

    pRtlInitializeSRWLock(&Lock);
    Ret = pRtlTryAcquireSRWLockShared(&Lock);
    ok(Ret, "Try shared on a free lock failed\n");
    if (Ret)
    {
        ReleaseShared(&Lock);
        CheckLockFree(&Lock, "after the only shared owner released it");
    }

    pRtlInitializeSRWLock(&Lock);
    Ret = pRtlTryAcquireSRWLockExclusive(&Lock);
    ok(Ret, "Try exclusive on a free lock failed\n");
    if (Ret)
    {
        ReleaseExclusive(&Lock);
        CheckLockFree(&Lock, "after the exclusive owner released it");
    }
}

/* Two shared owners on different threads, taken with the try function and with the blocking one */
static
VOID
TestSharedOwners(VOID)
{
    static RTL_SRWLOCK Lock;
    static SRW_WORKER Worker;
    BOOLEAN Ret, WorkerRet;

    pRtlInitializeSRWLock(&Lock);
    if (!StartWorker(&Worker, &Lock))
        return;

    Ret = pRtlTryAcquireSRWLockShared(&Lock);
    ok(Ret, "Try shared on a free lock failed\n");
    if (Ret)
    {
        WorkerRet = RunCommand(&Worker, CmdTryShared);
        ok(WorkerRet, "Try shared on a shared lock failed\n");
        if (Worker.Stuck)
            return;

        ReleaseShared(&Lock);
        if (WorkerRet)
        {
            /* The other thread still owns one reference */
            CheckTryExclusiveFails(&Lock, "while a shared owner is left");
            RunCommand(&Worker, CmdReleaseShared);
            if (Worker.Stuck)
                return;
        }
        CheckLockFree(&Lock, "after both shared owners released it");
    }

    pRtlInitializeSRWLock(&Lock);
    pRtlAcquireSRWLockShared(&Lock);
    WorkerRet = RunCommand(&Worker, CmdTryShared);
    ok(WorkerRet, "Try shared on a lock held by a blocking shared acquire failed\n");
    if (Worker.Stuck)
        return;

    CheckWorkerTryExclusiveFails(&Worker, "on a lock held by a blocking shared acquire");
    if (WorkerRet)
        RunCommand(&Worker, CmdReleaseShared);
    if (Worker.Stuck)
        return;

    ReleaseShared(&Lock);
    CheckLockFree(&Lock, "after the blocking and the try owner released it");

    StopWorker(&Worker);
}

/* The exclusive owner and the shared try acquirer are different threads */
static
VOID
TestExclusiveOwner(VOID)
{
    static RTL_SRWLOCK Lock;
    static SRW_WORKER Worker;
    BOOLEAN Ret;

    pRtlInitializeSRWLock(&Lock);
    if (!StartWorker(&Worker, &Lock))
        return;

    pRtlAcquireSRWLockExclusive(&Lock);
    CheckWorkerTrySharedFails(&Worker, "on an exclusively owned lock");
    CheckWorkerTryExclusiveFails(&Worker, "on an exclusively owned lock");
    if (Worker.Stuck)
        return;

    ReleaseExclusive(&Lock);
    CheckLockFree(&Lock, "after the exclusive owner released it");

    /* The failed tries must not have left a trace that blocks a later owner */
    Ret = RunCommand(&Worker, CmdTryShared);
    ok(Ret, "Try shared on a released lock failed\n");
    if (Ret)
        RunCommand(&Worker, CmdReleaseShared);
    if (Worker.Stuck)
        return;

    CheckLockFree(&Lock, "after the failed tries and a shared owner");

    StopWorker(&Worker);
}

/* A shared owner and an exclusive waiter that is already queued: a new shared try has to fail */
static
VOID
TestQueuedWaiter(VOID)
{
    static RTL_SRWLOCK Lock;
    static SRW_WORKER Waiter, Prober;
    BOOLEAN Failed = FALSE, Done;
    ULONG i;

    pRtlInitializeSRWLock(&Lock);
    if (!StartWorker(&Waiter, &Lock))
        return;
    if (!StartWorker(&Prober, &Lock))
    {
        StopWorker(&Waiter);
        return;
    }

    pRtlAcquireSRWLockShared(&Lock);
    StartCommand(&Waiter, CmdAcquireExclusive);

    /*
     * The waiter blocks inside the acquire and cannot report that it is queued, so probe until
     * a try is refused. A try that succeeds before that is released again.
     */
    for (i = 0; i < 1000 && !Failed && !Prober.Stuck; i++)
    {
        if (RunCommand(&Prober, CmdProbeShared))
            Sleep(10);
        else if (!Prober.Stuck)
            Failed = TRUE;
    }
    ok(Failed, "Try shared kept succeeding with an exclusive waiter queued\n");
    if (Prober.Stuck)
        return;

    ok(Prober.Status == STATUS_SUCCESS, "Release shared after a probe raised 0x%08lx\n",
       Prober.Status);

    Done = WaitCommand(&Waiter, 100);
    ok(!Done, "Exclusive waiter got the lock while a shared owner holds it\n");
    CheckWorkerTrySharedFails(&Prober, "with an exclusive waiter queued");
    if (Prober.Stuck)
        return;

    ReleaseShared(&Lock);
    if (!Done)
        Done = WaitCommand(&Waiter, WAIT_MS);
    ok(Done, "Exclusive waiter did not get the lock after the shared owner released it\n");
    if (!Done)
    {
        Waiter.Stuck = TRUE;
        return;
    }

    CheckWorkerTrySharedFails(&Prober, "on a lock the former waiter owns exclusively");
    CheckWorkerTryExclusiveFails(&Prober, "on a lock the former waiter owns exclusively");
    RunCommand(&Waiter, CmdReleaseExclusive);
    if (Prober.Stuck || Waiter.Stuck)
        return;

    CheckLockFree(&Lock, "after the former waiter released it");

    StopWorker(&Prober);
    StopWorker(&Waiter);
}

START_TEST(RtlSRWLock)
{
    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");

    pRtlInitializeSRWLock = (PFN_SRW)GetProcAddress(hNtdll, "RtlInitializeSRWLock");
    pRtlAcquireSRWLockShared = (PFN_SRW)GetProcAddress(hNtdll, "RtlAcquireSRWLockShared");
    pRtlReleaseSRWLockShared = (PFN_SRW)GetProcAddress(hNtdll, "RtlReleaseSRWLockShared");
    pRtlAcquireSRWLockExclusive = (PFN_SRW)GetProcAddress(hNtdll, "RtlAcquireSRWLockExclusive");
    pRtlReleaseSRWLockExclusive = (PFN_SRW)GetProcAddress(hNtdll, "RtlReleaseSRWLockExclusive");
    pRtlTryAcquireSRWLockShared = (PFN_TRY_SRW)GetProcAddress(hNtdll, "RtlTryAcquireSRWLockShared");
    pRtlTryAcquireSRWLockExclusive =
        (PFN_TRY_SRW)GetProcAddress(hNtdll, "RtlTryAcquireSRWLockExclusive");
    if (!pRtlInitializeSRWLock || !pRtlAcquireSRWLockShared || !pRtlReleaseSRWLockShared ||
        !pRtlAcquireSRWLockExclusive || !pRtlReleaseSRWLockExclusive ||
        !pRtlTryAcquireSRWLockShared || !pRtlTryAcquireSRWLockExclusive)
    {
        skip("SRW lock functions are not available\n");
        return;
    }

    TestFreeLock();
    TestSharedOwners();
    TestExclusiveOwner();
    TestQueuedWaiter();
}
