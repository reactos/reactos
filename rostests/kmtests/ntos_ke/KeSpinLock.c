/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Spin lock test
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#ifndef _WIN64
__declspec(dllimport) void __stdcall KeAcquireSpinLock(unsigned long *, unsigned char *);
__declspec(dllimport) void __stdcall KeReleaseSpinLock(unsigned long *, unsigned char);
__declspec(dllimport) void __stdcall KeAcquireSpinLockAtDpcLevel(unsigned long *);
__declspec(dllimport) void __stdcall KeReleaseSpinLockFromDpcLevel(unsigned long *);
#endif

/* this define makes KeInitializeSpinLock not use the inlined version */
#define WIN9X_COMPAT_SPINLOCK
#include <kmt_test.h>
#include <limits.h>

//#define NDEBUG
#include <debug.h>

static
_Must_inspect_result_
_IRQL_requires_min_(DISPATCH_LEVEL)
_Post_satisfies_(return == 1 || return == 0)
BOOLEAN
(FASTCALL
*pKeTryToAcquireSpinLockAtDpcLevel)(
    _Inout_ _Requires_lock_not_held_(*_Curr_)
    _When_(return!=0, _Acquires_lock_(*_Curr_))
        PKSPIN_LOCK SpinLock);

static
VOID
(FASTCALL
*pKeAcquireInStackQueuedSpinLockForDpc)(
  IN OUT PKSPIN_LOCK SpinLock,
  OUT PKLOCK_QUEUE_HANDLE LockHandle);

static
VOID
(FASTCALL
*pKeReleaseInStackQueuedSpinLockForDpc)(
  IN PKLOCK_QUEUE_HANDLE LockHandle);

static
_Must_inspect_result_
BOOLEAN
(FASTCALL
*pKeTestSpinLock)(
  _In_ PKSPIN_LOCK SpinLock);

/* TODO: multiprocessor testing */

struct _CHECK_DATA;
typedef struct _CHECK_DATA CHECK_DATA, *PCHECK_DATA;

typedef VOID (*PACQUIRE_FUNCTION)(PKSPIN_LOCK, PCHECK_DATA);
typedef VOID (*PRELEASE_FUNCTION)(PKSPIN_LOCK, PCHECK_DATA);
typedef BOOLEAN (*PTRY_FUNCTION)(PKSPIN_LOCK, PCHECK_DATA);

struct _CHECK_DATA
{
    enum
    {
        CheckQueueHandle,
        CheckQueue,
        CheckLock
    } Check;
    KIRQL IrqlWhenAcquired;
    PACQUIRE_FUNCTION Acquire;
    PRELEASE_FUNCTION Release;
    PTRY_FUNCTION TryAcquire;
    PACQUIRE_FUNCTION AcquireNoRaise;
    PRELEASE_FUNCTION ReleaseNoLower;
    PTRY_FUNCTION TryAcquireNoRaise;
    KSPIN_LOCK_QUEUE_NUMBER QueueNumber;
    BOOLEAN TryRetOnFailure;
    KIRQL OriginalIrql;
    BOOLEAN IsAcquired;
    _ANONYMOUS_UNION union
    {
        KLOCK_QUEUE_HANDLE QueueHandle;
        PKSPIN_LOCK_QUEUE Queue;
        KIRQL Irql;
    } DUMMYUNIONNAME;
    PVOID UntouchedValue;
};

#define DEFINE_ACQUIRE(LocalName, SetIsAcquired, DoCall)            \
static VOID LocalName(PKSPIN_LOCK SpinLock, PCHECK_DATA CheckData)  \
{                                                                   \
    ASSERT(!CheckData->IsAcquired);                                 \
    DoCall;                                                         \
    if (SetIsAcquired) CheckData->IsAcquired = TRUE;                \
}

#define DEFINE_RELEASE(LocalName, SetIsAcquired, DoCall)            \
static VOID LocalName(PKSPIN_LOCK SpinLock, PCHECK_DATA CheckData)  \
{                                                                   \
    DoCall;                                                         \
    if (SetIsAcquired) CheckData->IsAcquired = FALSE;               \
}

DEFINE_ACQUIRE(AcquireNormal,         TRUE,  KeAcquireSpinLock(SpinLock, &CheckData->Irql))
DEFINE_RELEASE(ReleaseNormal,         TRUE,  KeReleaseSpinLock(SpinLock, CheckData->Irql))
#ifdef _X86_
DEFINE_ACQUIRE(AcquireExp,            TRUE,  (KeAcquireSpinLock)(SpinLock, &CheckData->Irql))
DEFINE_RELEASE(ReleaseExp,            TRUE,  (KeReleaseSpinLock)(SpinLock, CheckData->Irql))
#else
DEFINE_ACQUIRE(AcquireExp,            TRUE,  KeAcquireSpinLock(SpinLock, &CheckData->Irql))
DEFINE_RELEASE(ReleaseExp,            TRUE,  KeReleaseSpinLock(SpinLock, CheckData->Irql))
#endif
DEFINE_ACQUIRE(AcquireSynch,          TRUE,  CheckData->Irql = KeAcquireSpinLockRaiseToSynch(SpinLock))

DEFINE_ACQUIRE(AcquireInStackQueued,  TRUE,  KeAcquireInStackQueuedSpinLock(SpinLock, &CheckData->QueueHandle))
DEFINE_ACQUIRE(AcquireInStackSynch,   TRUE,  KeAcquireInStackQueuedSpinLockRaiseToSynch(SpinLock, &CheckData->QueueHandle))
DEFINE_RELEASE(ReleaseInStackQueued,  TRUE,  KeReleaseInStackQueuedSpinLock(&CheckData->QueueHandle))

DEFINE_ACQUIRE(AcquireQueued,         TRUE,  CheckData->Irql = KeAcquireQueuedSpinLock(CheckData->QueueNumber))
DEFINE_ACQUIRE(AcquireQueuedSynch,    TRUE,  CheckData->Irql = KeAcquireQueuedSpinLockRaiseToSynch(CheckData->QueueNumber))
DEFINE_RELEASE(ReleaseQueued,         TRUE,  KeReleaseQueuedSpinLock(CheckData->QueueNumber, CheckData->Irql))

DEFINE_ACQUIRE(AcquireNoRaise,        FALSE, KeAcquireSpinLockAtDpcLevel(SpinLock))
DEFINE_RELEASE(ReleaseNoLower,        FALSE, KeReleaseSpinLockFromDpcLevel(SpinLock))
DEFINE_ACQUIRE(AcquireExpNoRaise,     FALSE, (KeAcquireSpinLockAtDpcLevel)(SpinLock))
DEFINE_RELEASE(ReleaseExpNoLower,     FALSE, (KeReleaseSpinLockFromDpcLevel)(SpinLock))

DEFINE_ACQUIRE(AcquireInStackNoRaise, FALSE, KeAcquireInStackQueuedSpinLockAtDpcLevel(SpinLock, &CheckData->QueueHandle))
DEFINE_RELEASE(ReleaseInStackNoRaise, FALSE, KeReleaseInStackQueuedSpinLockFromDpcLevel(&CheckData->QueueHandle))

/* TODO: test these functions. They behave weirdly, though */
#if 0
DEFINE_ACQUIRE(AcquireForDpc,         TRUE,  CheckData->Irql = KeAcquireSpinLockForDpc(SpinLock))
DEFINE_RELEASE(ReleaseForDpc,         TRUE,  KeReleaseSpinLockForDpc(SpinLock, CheckData->Irql))
#endif

DEFINE_ACQUIRE(AcquireInStackForDpc,  FALSE, pKeAcquireInStackQueuedSpinLockForDpc(SpinLock, &CheckData->QueueHandle))
DEFINE_RELEASE(ReleaseInStackForDpc,  FALSE, pKeReleaseInStackQueuedSpinLockForDpc(&CheckData->QueueHandle))

#ifdef _X86_
DEFINE_ACQUIRE(AcquireInt,            FALSE, KiAcquireSpinLock(SpinLock))
DEFINE_RELEASE(ReleaseInt,            FALSE, KiReleaseSpinLock(SpinLock))
#else
DEFINE_ACQUIRE(AcquireInt,            TRUE,  KeAcquireSpinLock(SpinLock, &CheckData->Irql))
DEFINE_RELEASE(ReleaseInt,            TRUE,  KeReleaseSpinLock(SpinLock, CheckData->Irql))
#endif

BOOLEAN TryQueued(PKSPIN_LOCK SpinLock, PCHECK_DATA CheckData) {
    LOGICAL Ret = KeTryToAcquireQueuedSpinLock(CheckData->QueueNumber, &CheckData->Irql);
    CheckData->IsAcquired = TRUE;
    ASSERT(Ret == FALSE || Ret == TRUE);
    return (BOOLEAN)Ret;
}
BOOLEAN TryQueuedSynch(PKSPIN_LOCK SpinLock, PCHECK_DATA CheckData) {
    BOOLEAN Ret = KeTryToAcquireQueuedSpinLockRaiseToSynch(CheckData->QueueNumber, &CheckData->Irql);
    CheckData->IsAcquired = TRUE;
    return Ret;
}
BOOLEAN TryNoRaise(PKSPIN_LOCK SpinLock, PCHECK_DATA CheckData) {
    BOOLEAN Ret = pKeTryToAcquireSpinLockAtDpcLevel(SpinLock);
    return Ret;
}

#define CheckSpinLockLock(SpinLock, CheckData, Value) do                            \
{                                                                                   \
    PKTHREAD Thread = KeGetCurrentThread();                                         \
    if (KmtIsMultiProcessorBuild)                                                   \
    {                                                                               \
        ok_eq_bool(Ret, (Value) == 0);                                              \
        if (SpinLock)                                                               \
            ok_eq_ulongptr(*(SpinLock),                                             \
                        (Value) ? (ULONG_PTR)Thread | 1 : 0);                       \
    }                                                                               \
    else                                                                            \
    {                                                                               \
        ok_bool_true(Ret, "KeTestSpinLock returned");                               \
        if (SpinLock)                                                               \
            ok_eq_ulongptr(*(SpinLock), 0);                                         \
    }                                                                               \
    ok_eq_uint((CheckData)->Irql, (CheckData)->OriginalIrql);                       \
} while (0)

#define CheckSpinLockQueue(SpinLock, CheckData, Value) do                           \
{                                                                                   \
    ok_eq_pointer((CheckData)->Queue->Next, NULL);                                  \
    ok_eq_pointer((CheckData)->Queue->Lock, NULL);                                  \
    ok_eq_uint((CheckData)->Irql, (CheckData)->OriginalIrql);                       \
} while (0)

#define CheckSpinLockQueueHandle(SpinLock, CheckData, Value) do                     \
{                                                                                   \
    if (KmtIsMultiProcessorBuild)                                                   \
    {                                                                               \
        ok_eq_bool(Ret, (Value) == 0);                                              \
        if (SpinLock)                                                               \
            ok_eq_ulongptr(*(SpinLock),                                             \
                        (Value) ? &(CheckData)->QueueHandle : 0);                   \
        ok_eq_pointer((CheckData)->QueueHandle.LockQueue.Next, NULL);               \
        ok_eq_pointer((CheckData)->QueueHandle.LockQueue.Lock,                      \
                (PVOID)((ULONG_PTR)SpinLock | ((Value) ? 2 : 0)));                  \
    }                                                                               \
    else                                                                            \
    {                                                                               \
        ok_bool_true(Ret, "KeTestSpinLock returned");                               \
        if (SpinLock)                                                               \
            ok_eq_ulongptr(*(SpinLock), 0);                                         \
        ok_eq_pointer((CheckData)->QueueHandle.LockQueue.Next, (CheckData)->UntouchedValue);                \
        ok_eq_pointer((CheckData)->QueueHandle.LockQueue.Lock, (CheckData)->UntouchedValue);                \
    }                                                                               \
    ok_eq_uint((CheckData)->QueueHandle.OldIrql, (CheckData)->OriginalIrql);        \
} while (0)

#define CheckSpinLock(SpinLock, CheckData, Value) do                                \
{                                                                                   \
    BOOLEAN Ret = SpinLock && pKeTestSpinLock ? pKeTestSpinLock(SpinLock) : TRUE;   \
    KIRQL ExpectedIrql = (CheckData)->OriginalIrql;                                 \
                                                                                    \
    switch ((CheckData)->Check)                                                     \
    {                                                                               \
        case CheckLock:                                                             \
            CheckSpinLockLock(SpinLock, CheckData, Value);                          \
            break;                                                                  \
        case CheckQueue:                                                            \
            CheckSpinLockQueue(SpinLock, CheckData, Value);                         \
            break;                                                                  \
        case CheckQueueHandle:                                                      \
            CheckSpinLockQueueHandle(SpinLock, CheckData, Value);                   \
            break;                                                                  \
    }                                                                               \
                                                                                    \
    if ((CheckData)->IsAcquired)                                                    \
        ExpectedIrql = (CheckData)->IrqlWhenAcquired;                               \
    ok_irql(ExpectedIrql);                                                          \
    ok_bool_false(KeAreApcsDisabled(), "KeAreApcsDisabled returned");               \
    ok_bool_true(KmtAreInterruptsEnabled(), "Interrupts enabled:");                 \
} while (0)

static
VOID
TestSpinLock(
    PKSPIN_LOCK SpinLock,
    PCHECK_DATA CheckData)
{
    static INT Run = 0;
    trace("Test SpinLock run %d\n", Run++);

    ok_irql(CheckData->OriginalIrql);

    if (SpinLock)
        ok_eq_ulongptr(*SpinLock, 0);
    CheckData->Acquire(SpinLock, CheckData);
    CheckSpinLock(SpinLock, CheckData, 1);
    CheckData->Release(SpinLock, CheckData);
    CheckSpinLock(SpinLock, CheckData, 0);

    if (CheckData->TryAcquire)
    {
        CheckSpinLock(SpinLock, CheckData, 0);
        ok_bool_true(CheckData->TryAcquire(SpinLock, CheckData), "TryAcquire returned");
        CheckSpinLock(SpinLock, CheckData, 1);
        if (!KmtIsCheckedBuild)
        {
            /* SPINLOCK_ALREADY_OWNED on checked build */
            ok_bool_true(CheckData->TryAcquire(SpinLock, CheckData), "TryAcquire returned");
            /* even a failing acquire sets irql */
            ok_eq_uint(CheckData->Irql, CheckData->IrqlWhenAcquired);
            CheckData->Irql = CheckData->OriginalIrql;
            CheckSpinLock(SpinLock, CheckData, 1);
        }
        CheckData->Release(SpinLock, CheckData);
        CheckSpinLock(SpinLock, CheckData, 0);
    }

    if (CheckData->AcquireNoRaise &&
        (CheckData->OriginalIrql >= DISPATCH_LEVEL || !KmtIsCheckedBuild) &&
        (CheckData->AcquireNoRaise != AcquireInStackForDpc ||
         !skip(pKeAcquireInStackQueuedSpinLockForDpc &&
               pKeReleaseInStackQueuedSpinLockForDpc, "No DPC spinlock functions\n")))
    {
        /* acquire/release without irql change */
        CheckData->AcquireNoRaise(SpinLock, CheckData);
        CheckSpinLock(SpinLock, CheckData, 1);
        CheckData->ReleaseNoLower(SpinLock, CheckData);
        CheckSpinLock(SpinLock, CheckData, 0);

        /* acquire without raise, but normal release */
        CheckData->AcquireNoRaise(SpinLock, CheckData);
        CheckSpinLock(SpinLock, CheckData, 1);
        CheckData->Release(SpinLock, CheckData);
        CheckSpinLock(SpinLock, CheckData, 0);

        /* acquire normally but release without lower */
        CheckData->Acquire(SpinLock, CheckData);
        CheckSpinLock(SpinLock, CheckData, 1);
        CheckData->ReleaseNoLower(SpinLock, CheckData);
        CheckSpinLock(SpinLock, CheckData, 0);
        CheckData->IsAcquired = FALSE;
        KmtSetIrql(CheckData->OriginalIrql);

        if (CheckData->TryAcquireNoRaise &&
            !skip(pKeTryToAcquireSpinLockAtDpcLevel != NULL, "KeTryToAcquireSpinLockAtDpcLevel unavailable\n"))
        {
            CheckSpinLock(SpinLock, CheckData, 0);
            ok_bool_true(CheckData->TryAcquireNoRaise(SpinLock, CheckData), "TryAcquireNoRaise returned");
            CheckSpinLock(SpinLock, CheckData, 1);
            if (!KmtIsCheckedBuild)
            {
                ok_bool_true(CheckData->TryAcquireNoRaise(SpinLock, CheckData), "TryAcquireNoRaise returned");
                CheckSpinLock(SpinLock, CheckData, 1);
            }
            CheckData->ReleaseNoLower(SpinLock, CheckData);
            CheckSpinLock(SpinLock, CheckData, 0);
        }
    }

    ok_irql(CheckData->OriginalIrql);
    /* make sure we survive this in case of error */
    KmtSetIrql(CheckData->OriginalIrql);
}

START_TEST(KeSpinLock)
{
    KSPIN_LOCK SpinLock = (KSPIN_LOCK)0x5555555555555555LL;
    PKSPIN_LOCK pSpinLock = &SpinLock;
    KIRQL Irql, SynchIrql = KmtIsMultiProcessorBuild ? IPI_LEVEL - 2 : DISPATCH_LEVEL;
    KIRQL OriginalIrqls[] = { PASSIVE_LEVEL, APC_LEVEL, DISPATCH_LEVEL, HIGH_LEVEL };
    CHECK_DATA TestData[] =
    {
        { CheckLock,        DISPATCH_LEVEL, AcquireNormal,        ReleaseNormal,        NULL,           AcquireNoRaise,        ReleaseNoLower,        TryNoRaise },
        { CheckLock,        DISPATCH_LEVEL, AcquireExp,           ReleaseExp,           NULL,           AcquireExpNoRaise,     ReleaseExpNoLower,     NULL },
        /* TODO: this one is just weird!
        { CheckLock,        DISPATCH_LEVEL, AcquireNormal,        ReleaseNormal,        NULL,           AcquireForDpc,         ReleaseForDpc,         NULL },*/
        { CheckLock,        DISPATCH_LEVEL, AcquireNormal,        ReleaseNormal,        NULL,           AcquireInt,            ReleaseInt,            NULL },
        { CheckLock,        SynchIrql,      AcquireSynch,         ReleaseNormal,        NULL,           NULL,                  NULL,                  NULL },
        { CheckQueueHandle, DISPATCH_LEVEL, AcquireInStackQueued, ReleaseInStackQueued, NULL,           AcquireInStackNoRaise, ReleaseInStackNoRaise, NULL },
        { CheckQueueHandle, SynchIrql,      AcquireInStackSynch,  ReleaseInStackQueued, NULL,           NULL,                  NULL,                  NULL },
        { CheckQueueHandle, DISPATCH_LEVEL, AcquireInStackQueued, ReleaseInStackQueued, NULL,           AcquireInStackForDpc,  ReleaseInStackForDpc,  NULL },
        { CheckQueue,       DISPATCH_LEVEL, AcquireQueued,        ReleaseQueued,        TryQueued,      NULL,                  NULL,                  NULL,       LockQueuePfnLock },
        { CheckQueue,       SynchIrql,      AcquireQueuedSynch,   ReleaseQueued,        TryQueuedSynch, NULL,                  NULL,                  NULL,       LockQueuePfnLock },
    };
    int i, iIrql;
    PKPRCB Prcb;

    pKeTryToAcquireSpinLockAtDpcLevel = KmtGetSystemRoutineAddress(L"KeTryToAcquireSpinLockAtDpcLevel");
    pKeAcquireInStackQueuedSpinLockForDpc = KmtGetSystemRoutineAddress(L"KeAcquireInStackQueuedSpinLockForDpc");
    pKeReleaseInStackQueuedSpinLockForDpc = KmtGetSystemRoutineAddress(L"KeReleaseInStackQueuedSpinLockForDpc");
    pKeTestSpinLock = KmtGetSystemRoutineAddress(L"KeTestSpinLock");

    Prcb = KeGetCurrentPrcb();

    /* KeInitializeSpinLock */
    memset(&SpinLock, 0x55, sizeof SpinLock);
    KeInitializeSpinLock(&SpinLock);
    ok_eq_ulongptr(SpinLock, 0);

    /* KeTestSpinLock */
    if (!skip(pKeTestSpinLock != NULL, "KeTestSpinLock unavailable\n"))
    {
        ok_bool_true(pKeTestSpinLock(&SpinLock), "KeTestSpinLock returned");
        SpinLock = 1;
        ok_bool_false(pKeTestSpinLock(&SpinLock), "KeTestSpinLock returned");
        SpinLock = 2;
        ok_bool_false(pKeTestSpinLock(&SpinLock), "KeTestSpinLock returned");
        SpinLock = (ULONG_PTR)-1;
        ok_bool_false(pKeTestSpinLock(&SpinLock), "KeTestSpinLock returned");
        SpinLock = (ULONG_PTR)1 << (sizeof(ULONG_PTR) * CHAR_BIT - 1);
        ok_bool_false(pKeTestSpinLock(&SpinLock), "KeTestSpinLock returned");
        SpinLock = 0;
        ok_bool_true(pKeTestSpinLock(&SpinLock), "KeTestSpinLock returned");
    }

    /* on UP none of the following functions actually looks at the spinlock! */
    if (!KmtIsMultiProcessorBuild && !KmtIsCheckedBuild)
        pSpinLock = NULL;

    for (i = 0; i < sizeof TestData / sizeof TestData[0]; ++i)
    {
        memset(&SpinLock, 0x55, sizeof SpinLock);
        KeInitializeSpinLock(&SpinLock);
        if (TestData[i].Check == CheckQueueHandle)
            memset(&TestData[i].QueueHandle, 0x55, sizeof TestData[i].QueueHandle);
        if (TestData[i].Check == CheckQueue)
        {
            TestData[i].Queue = &Prcb->LockQueue[TestData[i].QueueNumber];
            TestData[i].UntouchedValue = NULL;
        }
        else
            TestData[i].UntouchedValue = (PVOID)0x5555555555555555LL;

        for (iIrql = 0; iIrql < sizeof OriginalIrqls / sizeof OriginalIrqls[0]; ++iIrql)
        {
            if (KmtIsCheckedBuild && OriginalIrqls[iIrql] > DISPATCH_LEVEL)
                continue;
            KeRaiseIrql(OriginalIrqls[iIrql], &Irql);
            TestData[i].OriginalIrql = OriginalIrqls[iIrql];
            TestData[i].IsAcquired = FALSE;
            TestSpinLock(pSpinLock, &TestData[i]);
            KeLowerIrql(Irql);
        }
    }

    KmtSetIrql(PASSIVE_LEVEL);
}
