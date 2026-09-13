/*
 * PROJECT:     ReactOS kernel tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Push lock tests
 * COPYRIGHT:   Copyright 2026 Gleb Surikov <glebs.surikovs@gmail.com>
 */

#include <kmt_test.h>

#define PUSH_LOCK_TIMEOUT_MS        5000
#define PUSH_LOCK_POLL_INTERVAL_MS  1

#define PUSH_LOCK_RELATIVE_TIMEOUT(Milliseconds) (-((LONGLONG)(Milliseconds) * 10 * 1000))

#define PUSH_LOCK_MAX_WAITERS   4

#define PUSH_LOCK_RACE_ITERATIONS       64
#define PUSH_LOCK_CONTENTION_THREADS    6
#define PUSH_LOCK_CONTENTION_ITERATIONS 2048

#define PUSH_LOCK_CHECKSUM_XOR  0xA5A5A5A5UL

typedef enum PUSH_LOCK_MODE
{
    PushLockModeShared,
    PushLockModeExclusive
} PUSH_LOCK_MODE;

typedef enum PUSH_LOCK_RELEASE_KIND
{
    PushLockReleaseSpecific,
    PushLockReleaseGeneric
} PUSH_LOCK_RELEASE_KIND;

typedef enum PUSH_LOCK_WAIT_CHAIN_STATE
{
    PushLockWaitChainInProgress,
    PushLockWaitChainStable,
    PushLockWaitChainInvalid
} PUSH_LOCK_WAIT_CHAIN_STATE;

typedef struct PUSH_LOCK_TEST_STATE
{
    EX_PUSH_LOCK Lock;

    volatile LONG ActiveReaders;
    volatile LONG ActiveWriters;
    volatile LONG Violations;

    volatile LONG SharedAcquisitions;
    volatile LONG ExclusiveAcquisitions;
    volatile LONG CompletedOperations;

    /* These fields form one protected value. Writers deliberately update the
       fields separately so that a reader can detect any overlap with a writer. */
    volatile ULONG Sequence;
    volatile ULONG SequenceInverse;
    volatile ULONG Checksum;
} PUSH_LOCK_TEST_STATE, *PPUSH_LOCK_TEST_STATE;

typedef struct PUSH_LOCK_THREAD_CONTEXT
{
    PPUSH_LOCK_TEST_STATE State;
    PUSH_LOCK_MODE Mode;
    PUSH_LOCK_RELEASE_KIND ReleaseKind;
    PKEVENT StartGate;
    KEVENT ReadyEvent;
    KEVENT AcquiredEvent;
    KEVENT ReleaseEvent;
    KEVENT DoneEvent;
    ULONG Index;
    ULONG Iterations;
} PUSH_LOCK_THREAD_CONTEXT, *PPUSH_LOCK_THREAD_CONTEXT;

C_ASSERT(sizeof(EX_PUSH_LOCK) == sizeof(ULONG_PTR));

FORCEINLINE
NTSTATUS
PushLockWaitForEvent(
    _In_ PKEVENT Event)
{
    LARGE_INTEGER Timeout;

    Timeout.QuadPart = PUSH_LOCK_RELATIVE_TIMEOUT(PUSH_LOCK_TIMEOUT_MS);

    return KeWaitForSingleObject(Event,
                                 Executive,
                                 KernelMode,
                                 FALSE,
                                 &Timeout);
}

FORCEINLINE
VOID
PushLockDelay(VOID)
{
    LARGE_INTEGER Delay;

    Delay.QuadPart = PUSH_LOCK_RELATIVE_TIMEOUT(PUSH_LOCK_POLL_INTERVAL_MS);

    KeDelayExecutionThread(KernelMode, FALSE, &Delay);
}

static
EX_PUSH_LOCK
PushLockReadValue(
    _In_ PEX_PUSH_LOCK PushLock)
{
    EX_PUSH_LOCK Value;

    /* Use a cmpxchg with identical xchg and comparand values as an
       atomic read of the complete push lock word. A zero value remains zero,
       and a nonzero value never matches the comparand, so the operation doesn't
       modify the lock. */
    Value.Ptr = InterlockedCompareExchangePointer(&PushLock->Ptr, NULL, NULL);

    return Value;
}

/*
 * This checks only relationships that are valid for every externally visible
 * push lock state. In particular, an unlocked lock may still have Waiting set
 * while a selected waiter is becoming ready.
 */
static
BOOLEAN
PushLockValueIsPlausible(
    _In_ EX_PUSH_LOCK Value)
{
    ULONG_PTR WaitBlockAddress;

    if (Value.Waking && !Value.Waiting)
    {
        return FALSE;
    }

    if (Value.MultipleShared && (!Value.Locked || !Value.Waiting))
    {
        return FALSE;
    }

    if (!Value.Waiting && (Value.Waking || Value.MultipleShared))
    {
        return FALSE;
    }

    if (!Value.Locked && !Value.Waiting && Value.Value != 0)
    {
        return FALSE;
    }

    /* When Waiting is set, the upper bits contain the address of the newest wait
       block while the low bits retain the push lock flags */
    if (Value.Waiting)
    {
        WaitBlockAddress = Value.Value & ~EX_PUSH_LOCK_PTR_BITS;
        if (WaitBlockAddress == 0)
        {
            return FALSE;
        }
    }

    return TRUE;
}

FORCEINLINE
VOID
PushLockRecordViolation(
    _Inout_ PPUSH_LOCK_TEST_STATE State)
{
    InterlockedIncrement(&State->Violations);
}

static
VOID
PushLockSampleState(
    _Inout_ PPUSH_LOCK_TEST_STATE State)
{
    if (!PushLockValueIsPlausible(PushLockReadValue(&State->Lock)))
    {
        PushLockRecordViolation(State);
    }
}

static
VOID
PushLockAcquire(
    _Inout_ PEX_PUSH_LOCK PushLock,
    _In_ PUSH_LOCK_MODE Mode)
{
    if (Mode == PushLockModeExclusive)
    {
        ExfAcquirePushLockExclusive(PushLock);
    }
    else
    {
        ExfAcquirePushLockShared(PushLock);
    }
}

static
VOID
PushLockRelease(
    _Inout_ PEX_PUSH_LOCK PushLock,
    _In_ PUSH_LOCK_MODE Mode,
    _In_ PUSH_LOCK_RELEASE_KIND ReleaseKind)
{
    if (ReleaseKind == PushLockReleaseGeneric)
    {
        ExfReleasePushLock(PushLock);
    }
    else if (Mode == PushLockModeExclusive)
    {
        ExfReleasePushLockExclusive(PushLock);
    }
    else
    {
        ExfReleasePushLockShared(PushLock);
    }
}

/*
 * The counters below are independent of the push lock implementation. They
 * detect overlapping writers and any reader that enters while a writer owns
 * the protected region.
 */
static
VOID
PushLockEnterProtectedRegion(
    _Inout_ PPUSH_LOCK_TEST_STATE State,
    _In_ PUSH_LOCK_MODE Mode)
{
    LONG Count;

    if (Mode == PushLockModeExclusive)
    {
        Count = InterlockedIncrement(&State->ActiveWriters);
        if (Count != 1 || State->ActiveReaders != 0)
        {
            PushLockRecordViolation(State);
        }

        InterlockedIncrement(&State->ExclusiveAcquisitions);
    }
    else
    {
        /* Check on both sides of the reader increment. The second check detects a
           writer that entered after the first load but before this reader published
           itself. */
        if (State->ActiveWriters != 0)
        {
            PushLockRecordViolation(State);
        }

        InterlockedIncrement(&State->ActiveReaders);

        if (State->ActiveWriters != 0)
        {
            PushLockRecordViolation(State);
        }

        InterlockedIncrement(&State->SharedAcquisitions);
    }
}

static
VOID
PushLockLeaveProtectedRegion(
    _Inout_ PPUSH_LOCK_TEST_STATE State,
    _In_ PUSH_LOCK_MODE Mode)
{
    LONG Count;

    if (Mode == PushLockModeExclusive)
    {
        Count = InterlockedDecrement(&State->ActiveWriters);
        if (Count != 0)
        {
            PushLockRecordViolation(State);
        }
    }
    else
    {
        Count = InterlockedDecrement(&State->ActiveReaders);
        if (Count < 0)
        {
            PushLockRecordViolation(State);
        }
    }
}

static
VOID
PushLockReadProtectedValue(
    _Inout_ PPUSH_LOCK_TEST_STATE State)
{
    ULONG Sequence;
    ULONG SequenceInverse;
    ULONG Checksum;

    /* A reader must observe all 3 fields from the same completed writer
       update. Any mixture of old and new fields indicates overlap with an
       x owner. */
    Sequence = State->Sequence;
    SequenceInverse = State->SequenceInverse;
    Checksum = State->Checksum;

    if (SequenceInverse != ~Sequence ||
        Checksum != (Sequence ^ PUSH_LOCK_CHECKSUM_XOR))
    {
        PushLockRecordViolation(State);
    }
}

static
VOID
PushLockWriteProtectedValue(
    _Inout_ PPUSH_LOCK_TEST_STATE State)
{
    ULONG Sequence;

    /* Publish an intentionally inconsistent value while the update is in
       progress. A reader entering concurrently is likely to observe
       either an invalid inverse or an invalid checksum. */
    Sequence = State->Sequence + 1;
    State->Sequence = Sequence;
    /* Keep the 3 stores ordered. N.B. The barriers aren't intended to replace
       the synchronization supplied by the push lock. */
    KeMemoryBarrier();
    State->SequenceInverse = ~Sequence;
    KeMemoryBarrier();
    State->Checksum = Sequence ^ PUSH_LOCK_CHECKSUM_XOR;
}

static
VOID
PushLockInitializeState(
    _Out_ PPUSH_LOCK_TEST_STATE State)
{
    RtlZeroMemory(State, sizeof(*State));
    State->SequenceInverse = ~(ULONG)0;
    State->Checksum = PUSH_LOCK_CHECKSUM_XOR;
}

static
VOID
PushLockInitializeThreadContext(
    _Out_ PPUSH_LOCK_THREAD_CONTEXT Context,
    _Inout_ PPUSH_LOCK_TEST_STATE State,
    _In_ PUSH_LOCK_MODE Mode,
    _In_ PUSH_LOCK_RELEASE_KIND ReleaseKind)
{
    RtlZeroMemory(Context, sizeof(*Context));

    Context->State = State;
    Context->Mode = Mode;
    Context->ReleaseKind = ReleaseKind;

    KeInitializeEvent(&Context->ReadyEvent, NotificationEvent, FALSE);
    KeInitializeEvent(&Context->AcquiredEvent, NotificationEvent, FALSE);
    KeInitializeEvent(&Context->ReleaseEvent, NotificationEvent, FALSE);
    KeInitializeEvent(&Context->DoneEvent, NotificationEvent, FALSE);
}

/*
 * A controlled waiter acquires once and then remains in the protected region
 * until the test releases it. This makes queue construction and waking
 * observable without relying on timing after acquisition.
 */
static
VOID
NTAPI
PushLockControlledThread(
    _In_ PVOID Parameter)
{
    PPUSH_LOCK_THREAD_CONTEXT Context;
    PPUSH_LOCK_TEST_STATE State;
    NTSTATUS Status;

    Context = (PPUSH_LOCK_THREAD_CONTEXT)Parameter;
    State = Context->State;

    /* ReadyEvent reports only that the thread is running. It may still be held
       behind StartGate and hasn't yet attempted to acquire the push lock. */
    KeSetEvent(&Context->ReadyEvent, IO_NO_INCREMENT, FALSE);

    if (Context->StartGate != NULL)
    {
        Status = KeWaitForSingleObject(Context->StartGate,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       NULL);
        if (Status != STATUS_SUCCESS)
        {
            PushLockRecordViolation(State);
            KeSetEvent(&Context->DoneEvent, IO_NO_INCREMENT, FALSE);
            return;
        }
    }

    KeEnterCriticalRegion();

    PushLockAcquire(&State->Lock, Context->Mode);
    PushLockEnterProtectedRegion(State, Context->Mode);

    /* Publish acquisition only after updating the ownership counters,
       so the controlling thread can inspect a consistent protected
       state after this event is signaled */
    KeSetEvent(&Context->AcquiredEvent, IO_NO_INCREMENT, FALSE);

    /* Remain inside the protected region until the test has inspected the lock
       word and waiter chain for this acquisition */
    Status = KeWaitForSingleObject(&Context->ReleaseEvent,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   NULL);
    if (Status != STATUS_SUCCESS)
    {
        PushLockRecordViolation(State);
    }

    PushLockLeaveProtectedRegion(State, Context->Mode);
    PushLockRelease(&State->Lock, Context->Mode, Context->ReleaseKind);

    KeLeaveCriticalRegion();

    KeSetEvent(&Context->DoneEvent, IO_NO_INCREMENT, FALSE);
}

static
BOOLEAN
PushLockStartControlledThread(
    _Inout_ PPUSH_LOCK_THREAD_CONTEXT Context,
    _Out_ PKTHREAD *Thread)
{
    NTSTATUS Status;

    *Thread = KmtStartThread(PushLockControlledThread, Context);
    if (*Thread == NULL)
    {
        ok(FALSE, "Could not create push-lock test thread\n");
        return FALSE;
    }

    /* Don't examine the queue until the worker has started.
       N.B. ReadyEvent doesn't imply that acquisition or queue
            insertion has completed. */
    Status = PushLockWaitForEvent(&Context->ReadyEvent);
    ok_eq_hex(Status, STATUS_SUCCESS);

    return Status == STATUS_SUCCESS;
}

static
VOID
PushLockReleaseAndFinishThread(
    _In_opt_ PKTHREAD Thread,
    _Inout_ PPUSH_LOCK_THREAD_CONTEXT Context)
{
    NTSTATUS Status;

    if (Thread == NULL)
    {
        return;
    }

    KeSetEvent(&Context->ReleaseEvent, IO_NO_INCREMENT, FALSE);

    Status = PushLockWaitForEvent(&Context->DoneEvent);
    ok_eq_hex(Status, STATUS_SUCCESS);

    KmtFinishThread(Thread, NULL);
}

/*
 * Wait blocks are inserted in the newest-first manner. Once list optimization completes,
 * Head->Last points to the oldest waiter and Previous links point toward newer
 * waiters. The oldest block's Next field isn't a list terminator and isn't
 * inspected.
 *
 * ExpectedNewestFirst describes the expected waiter modes starting at the
 * wait block stored in the push lock and ending with the oldest waiter.
 *
 * The lock must remain owned while the chain is examined. InProgress is
 * returned while an expected waiter hasn't yet been inserted or while list
 * optimization is still in progress.
 */
static
PUSH_LOCK_WAIT_CHAIN_STATE
PushLockValidateWaitChain(
    _In_ PEX_PUSH_LOCK PushLock,
    _In_reads_(ExpectedCount) const PUSH_LOCK_MODE *ExpectedNewestFirst,
    _In_ ULONG ExpectedCount,
    _Out_opt_ PEX_PUSH_LOCK_WAIT_BLOCK *OldestWaitBlock)
{
    EX_PUSH_LOCK Value;
    EX_PUSH_LOCK CurrentValue;
    PVOID Seen[PUSH_LOCK_MAX_WAITERS];
    PUSH_LOCK_MODE ActualNewestFirst[PUSH_LOCK_MAX_WAITERS];
    PEX_PUSH_LOCK_WAIT_BLOCK Head;
    PEX_PUSH_LOCK_WAIT_BLOCK Current;
    PEX_PUSH_LOCK_WAIT_BLOCK Previous;
    PEX_PUSH_LOCK_WAIT_BLOCK Last;
    ULONG ActualCount;
    ULONG Index;
    ULONG SeenIndex;
    LONG Flags;

    if (ExpectedCount == 0 || ExpectedCount > PUSH_LOCK_MAX_WAITERS)
    {
        return PushLockWaitChainInvalid;
    }

    Value = PushLockReadValue(PushLock);

    if (!PushLockValueIsPlausible(Value))
    {
        return PushLockWaitChainInvalid;
    }

    /* The tests keep an owner in place while constructing the queue.
       Waiting may still be clear until the expected waiter is inserted,
       and Waking remains set while the list links are being optimized. */
    if (!Value.Locked)
    {
        return PushLockWaitChainInvalid;
    }

    if (!Value.Waiting || Value.Waking)
    {
        return PushLockWaitChainInProgress;
    }

    Head = (PEX_PUSH_LOCK_WAIT_BLOCK)(Value.Value & ~EX_PUSH_LOCK_PTR_BITS);
    if (!MmIsAddressValid(Head))
    {
        return PushLockWaitChainInvalid;
    }

    Last = Head->Last;
    if (Last == NULL)
        goto CheckForConcurrentChange;

    if (((ULONG_PTR)Last & EX_PUSH_LOCK_PTR_BITS) != 0 || !MmIsAddressValid(Last))
    {
        return PushLockWaitChainInvalid;
    }

    RtlZeroMemory(Seen, sizeof(Seen));

    Current = Head;
    Previous = NULL;
    ActualCount = 0;

    for (Index = 0; Index < PUSH_LOCK_MAX_WAITERS; Index++)
    {
        if (Current == NULL)
        {
            return PushLockWaitChainInvalid;
        }

        if (!MmIsAddressValid(Current))
        {
            return PushLockWaitChainInvalid;
        }

        if (((ULONG_PTR)Current & EX_PUSH_LOCK_PTR_BITS) != 0)
        {
            return PushLockWaitChainInvalid;
        }

        for (SeenIndex = 0; SeenIndex < ActualCount; SeenIndex++)
        {
            if (Seen[SeenIndex] == Current)
            {
                return PushLockWaitChainInvalid;
            }
        }

        Seen[ActualCount] = Current;

        /* Previous links are built by ExpOptimizePushLockList. If the
           push lock value changed while examining them, retry after the
           concurrent insertion/optimization completes. */
        if (Current->Previous != Previous)
        {
            goto CheckForConcurrentChange;
        }

        Flags = Current->Flags;

        /* WAIT is the wake/sleep handshake bit and may already be clear.
           EXCLUSIVE records the acquisition mode. No other flag bits are
           valid for these wait blocks. */
        if (Flags & ~(EX_PUSH_LOCK_FLAGS_EXCLUSIVE | EX_PUSH_LOCK_FLAGS_WAIT))
        {
            return PushLockWaitChainInvalid;
        }

        ActualNewestFirst[ActualCount] = Flags & EX_PUSH_LOCK_FLAGS_EXCLUSIVE
                                             ? PushLockModeExclusive
                                             : PushLockModeShared;

        ActualCount++;

        if (Current == Last)
        {
            break;
        }

        Previous = Current;
        Current = Current->Next;
    }

    if (Current != Last)
        return PushLockWaitChainInvalid;

    /* Make sure the lock word didn't change while the non-atomic wait block
       links were being examined */
    CurrentValue = PushLockReadValue(PushLock);
    if (CurrentValue.Value != Value.Value)
    {
        return PushLockWaitChainInProgress;
    }

    /* A shorter chain means that not all expected waiters have been queued yet.
       Additional waiters are invalid because the tests construct the queue in
       controlled steps. */
    if (ActualCount < ExpectedCount)
    {
        return PushLockWaitChainInProgress;
    }

    if (ActualCount > ExpectedCount)
    {
        return PushLockWaitChainInvalid;
    }

    for (Index = 0; Index < ExpectedCount; Index++)
    {
        /* EXCLUSIVE is the persistent mode bit, so the stable chain must
           match the acquisition modes requested by the test */
        if (ActualNewestFirst[Index] != ExpectedNewestFirst[Index])
        {
            return PushLockWaitChainInvalid;
        }
    }

    if (OldestWaitBlock != NULL)
    {
        *OldestWaitBlock = Last;
    }

    return PushLockWaitChainStable;

CheckForConcurrentChange:

    CurrentValue = PushLockReadValue(PushLock);
    if (CurrentValue.Value != Value.Value)
    {
        return PushLockWaitChainInProgress;
    }

    return PushLockWaitChainInvalid;
}

/*
 * ExpectedNewestFirst describes the expected mode of each waiter,
 * starting with the wait block encoded in the push lock and ending
 * with the oldest wait block.
 */
static
BOOLEAN
PushLockWaitForStableWaitChain(
    _In_ PEX_PUSH_LOCK PushLock,
    _In_reads_(ExpectedCount) const PUSH_LOCK_MODE *ExpectedNewestFirst,
    _In_ ULONG ExpectedCount,
    _Out_opt_ PEX_PUSH_LOCK_WAIT_BLOCK *OldestWaitBlock)
{
    PUSH_LOCK_WAIT_CHAIN_STATE ChainState;
    ULONG Elapsed;

    /* Queue insertion and list optimization are separate operations. Poll until
       the waiter chain reaches the backward linked form required by the
       structural checks below. */
    for (Elapsed = 0;
         Elapsed < PUSH_LOCK_TIMEOUT_MS;
         Elapsed += PUSH_LOCK_POLL_INTERVAL_MS)
    {
        ChainState = PushLockValidateWaitChain(PushLock,
                                               ExpectedNewestFirst,
                                               ExpectedCount,
                                               OldestWaitBlock);

        if (ChainState == PushLockWaitChainStable)
        {
            return TRUE;
        }

        if (ChainState == PushLockWaitChainInvalid)
        {
            return FALSE;
        }

        PushLockDelay();
    }

    return FALSE;
}

/* Verify the exact lock word encodings used by uncontended operations. */
static
VOID
TestPushLockUncontended(
    _In_ PUSH_LOCK_RELEASE_KIND ReleaseKind)
{
    PUSH_LOCK_TEST_STATE State;
    EX_PUSH_LOCK Value;
    ULONG Count;

    PushLockInitializeState(&State);
    ok_eq_ulongptr(State.Lock.Value, 0);

    KeEnterCriticalRegion();

    /* An uncontended exclusive acquisition is represented by the Locked bit alone */
    ExfAcquirePushLockExclusive(&State.Lock);
    ok_eq_ulongptr(PushLockReadValue(&State.Lock).Value, EX_PUSH_LOCK_LOCK);

    PushLockRelease(&State.Lock, PushLockModeExclusive, ReleaseKind);
    ok_eq_ulongptr(PushLockReadValue(&State.Lock).Value, 0);

    /* In the uncontended shared form, the lock word contains Locked plus one
       EX_PUSH_LOCK_SHARE_INC for each shared acquisition */
    for (Count = 1; Count <= 4; Count++)
    {
        ExfAcquirePushLockShared(&State.Lock);
        Value = PushLockReadValue(&State.Lock);
        ok_eq_ulongptr(Value.Value, EX_PUSH_LOCK_LOCK + Count * EX_PUSH_LOCK_SHARE_INC);
    }

    /* Verify every intermediate shared count, including the transition from the
       final shared owner back to the zero lock word */
    for (Count = 4; Count > 0; Count--)
    {
        PushLockRelease(&State.Lock, PushLockModeShared, ReleaseKind);
        Value = PushLockReadValue(&State.Lock);

        if (Count == 1)
        {
            ok_eq_ulongptr(Value.Value, 0);
        }
        else
        {
            ok_eq_ulongptr(Value.Value,
                           EX_PUSH_LOCK_LOCK + (Count - 1) * EX_PUSH_LOCK_SHARE_INC);
        }
    }

    KeLeaveCriticalRegion();
}

/*
 * Queue one exclusive waiter followed by two shared waiters. The oldest
 * exclusive waiter must be selected alone. After it releases, the two readers
 * may acquire together.
 */
static
VOID
TestPushLockWaiterSelection(VOID)
{
    static const PUSH_LOCK_MODE Modes[] = {
        PushLockModeExclusive,
        PushLockModeShared,
        PushLockModeShared
    };
    static const PUSH_LOCK_MODE ExpectedOne[] = {PushLockModeExclusive};
    static const PUSH_LOCK_MODE ExpectedTwo[] = {
        PushLockModeShared,
        PushLockModeExclusive
    };
    static const PUSH_LOCK_MODE ExpectedThree[] = {
        PushLockModeShared,
        PushLockModeShared,
        PushLockModeExclusive
    };
    static const PUSH_LOCK_MODE ExpectedReaders[] = {
        PushLockModeShared,
        PushLockModeShared
    };
    const PUSH_LOCK_MODE *Expected[] = {
        ExpectedOne,
        ExpectedTwo,
        ExpectedThree
    };
    PUSH_LOCK_TEST_STATE State;
    PUSH_LOCK_THREAD_CONTEXT Contexts[RTL_NUMBER_OF(Modes)];
    PKTHREAD Threads[RTL_NUMBER_OF(Modes)] = {NULL};
    NTSTATUS Status;
    ULONG Started;
    ULONG Index;

    PushLockInitializeState(&State);
    Started = 0;

    /* Hold the lock while constructing the waiter chain. Starting each waiter
       separately makes the newest-to-oldest order deterministic. */

    KeEnterCriticalRegion();

    ExfAcquirePushLockExclusive(&State.Lock);

    /* The resulting chains are:
        o exclusive
        o shared -> exclusive
        o shared -> shared -> exclusive
       where the leftmost entry is the newest waiter */

    for (Index = 0; Index < RTL_NUMBER_OF(Modes); Index++)
    {
        PushLockInitializeThreadContext(&Contexts[Index],
                                        &State,
                                        Modes[Index],
                                        PushLockReleaseSpecific);

        if (!PushLockStartControlledThread(&Contexts[Index], &Threads[Index]))
        {
            if (Threads[Index] != NULL)
            {
                Started++;
            }
            break;
        }

        Started++;

        ok(PushLockWaitForStableWaitChain(&State.Lock, Expected[Index], Index + 1, NULL),
           "Wait chain did not stabilize after waiter %lu\n", Index);
    }

    /* The oldest waiter is exclusive, so releasing the owner must select only
       that waiter and leave both shared waiters queued */
    ExfReleasePushLockExclusive(&State.Lock);

    KeLeaveCriticalRegion();

    if (Started == RTL_NUMBER_OF(Modes))
    {
        Status = PushLockWaitForEvent(&Contexts[0].AcquiredEvent);
        ok_eq_hex(Status, STATUS_SUCCESS);

        ok_eq_long(KeReadStateEvent(&Contexts[1].AcquiredEvent), 0);
        ok_eq_long(KeReadStateEvent(&Contexts[2].AcquiredEvent), 0);

        /* Once the x waiter owns the lock, the two newer s waiters must
           still form the complete remaining wait chain */
        ok(PushLockWaitForStableWaitChain(&State.Lock, ExpectedReaders,
               RTL_NUMBER_OF(ExpectedReaders), NULL),
           "Reader wait chain is invalid while the writer owns the lock\n");

        /* Releasing the oldest x waiter permits the remaining s batch to acquire together */
        PushLockReleaseAndFinishThread(Threads[0], &Contexts[0]);
        Threads[0] = NULL;

        Status = PushLockWaitForEvent(&Contexts[1].AcquiredEvent);
        ok_eq_hex(Status, STATUS_SUCCESS);
        Status = PushLockWaitForEvent(&Contexts[2].AcquiredEvent);
        ok_eq_hex(Status, STATUS_SUCCESS);

        ok_eq_long(State.ActiveReaders, 2);
        ok_eq_long(State.ActiveWriters, 0);
        ok_eq_ulongptr(PushLockReadValue(&State.Lock).Value,
                       EX_PUSH_LOCK_LOCK + 2 * EX_PUSH_LOCK_SHARE_INC);
    }

    for (Index = 0; Index < Started; Index++)
    {
        if (Threads[Index] != NULL)
        {
            PushLockReleaseAndFinishThread(Threads[Index], &Contexts[Index]);
            Threads[Index] = NULL;
        }
    }

    ok_eq_long(State.ActiveReaders, 0);
    ok_eq_long(State.ActiveWriters, 0);
    ok_eq_long(State.Violations, 0);
    ok_eq_ulongptr(PushLockReadValue(&State.Lock).Value, 0);
}

/*
 * When an x waiter is queued behind several s acquisitions, the
 * oldest wait block stores the number of outstanding shares.
 * A newer s waiter must remain queued until the x waiter has acquired and
 * released the lock.
 */
static
VOID
TestPushLockSharedOwnerDrain(
    _In_ PUSH_LOCK_RELEASE_KIND ReleaseKind)
{
    /*
     * shared
     * shared
     * shared
     *   |
     *   v
     * exclusive waiter     <- oldest, ShareCount = 3
     *   |
     *   v
     * shared waiter        <- newest
     *   |
     *   v
     * release share        ShareCount = 2, nobody wakes
     * release share        ShareCount = 1, nobody wakes
     * release share        ShareCount = 0
     *   |
     *   v
     * exclusive wakes      shared MUST remain queued
     *   |
     *   v
     * exclusive releases
     *   |
     *   v
     * shared wakes
     */
    static const PUSH_LOCK_MODE ExpectedExclusive[] = {PushLockModeExclusive};
    static const PUSH_LOCK_MODE ExpectedBoth[] = {PushLockModeShared, PushLockModeExclusive};
    static const PUSH_LOCK_MODE ExpectedShared[] = {PushLockModeShared};
    PUSH_LOCK_TEST_STATE State;
    PUSH_LOCK_THREAD_CONTEXT ExclusiveContext;
    PUSH_LOCK_THREAD_CONTEXT SharedContext;
    PKTHREAD ExclusiveThread;
    PKTHREAD SharedThread;
    PEX_PUSH_LOCK_WAIT_BLOCK OldestWaitBlock;
    EX_PUSH_LOCK Value;
    NTSTATUS Status;
    BOOLEAN Success;
    ULONG Count;

    PushLockInitializeState(&State);
    ExclusiveThread = NULL;
    SharedThread = NULL;
    OldestWaitBlock = NULL;

    PushLockInitializeThreadContext(&ExclusiveContext,
                                    &State,
                                    PushLockModeExclusive,
                                    PushLockReleaseSpecific);

    PushLockInitializeThreadContext(&SharedContext,
                                    &State,
                                    PushLockModeShared,
                                    PushLockReleaseSpecific);

    KeEnterCriticalRegion();

    /* 3 shared acquisitions force the 1st x waiter to use
       MultipleShared and store the outstanding count in its wait block */
    for (Count = 0; Count < 3; Count++)
    {
        ExfAcquirePushLockShared(&State.Lock);
    }

    if (!PushLockStartControlledThread(&ExclusiveContext, &ExclusiveThread))
    {
        goto Cleanup;
    }

    Success = PushLockWaitForStableWaitChain(&State.Lock,
                                             ExpectedExclusive,
                                             RTL_NUMBER_OF(ExpectedExclusive),
                                             &OldestWaitBlock);
    ok(Success, "Exclusive waiter did not reach a stable wait state\n");
    if (!Success)
    {
        goto Cleanup;
    }

    /* Queue an s waiter after the x waiter. Since Waiting is already set,
       it must queue instead of joining the current s owners */
    if (!PushLockStartControlledThread(&SharedContext, &SharedThread))
    {
        goto Cleanup;
    }

    Success = PushLockWaitForStableWaitChain(&State.Lock,
                                             ExpectedBoth,
                                             RTL_NUMBER_OF(ExpectedBoth),
                                             &OldestWaitBlock);
    ok(Success, "Exclusive/shared wait chain did not stabilize\n");
    if (!Success)
    {
        goto Cleanup;
    }

    Value = PushLockReadValue(&State.Lock);
    ok(Value.Locked, "Push lock is not locked\n");
    ok(Value.Waiting, "Push lock has no waiters\n");
    ok(!Value.Waking, "Push lock is waking\n");
    ok(Value.MultipleShared, "MultipleShared is not set\n");

    if (OldestWaitBlock != NULL)
    {
        ok_eq_long(OldestWaitBlock->ShareCount, 3);
        ok(OldestWaitBlock->Flags & EX_PUSH_LOCK_FLAGS_EXCLUSIVE, "Oldest waiter is not exclusive\n");
    }

    /* The first 2 releases only decrement the saved shared count.
       Neither waiter may acquire while an existing s owner remains. */
    for (Count = 3; Count > 1; Count--)
    {
        PushLockRelease(&State.Lock, PushLockModeShared, ReleaseKind);

        if (OldestWaitBlock != NULL)
        {
            ok_eq_long(OldestWaitBlock->ShareCount, Count - 1);
        }

        Value = PushLockReadValue(&State.Lock);
        ok(Value.Locked, "Push lock is not locked while shares remain\n");
        ok(Value.Waiting, "Push lock has no waiters while shares remain\n");
        ok(Value.MultipleShared, "MultipleShared is not set while shares remain\n");

        ok_eq_long(KeReadStateEvent(&ExclusiveContext.AcquiredEvent), 0);
        ok_eq_long(KeReadStateEvent(&SharedContext.AcquiredEvent), 0);
    }

    /* The final s release must select the oldest x waiter.
       The newer s waiter must remain queued behind it. */
    PushLockRelease(&State.Lock, PushLockModeShared, ReleaseKind);

    KeLeaveCriticalRegion();

    Status = PushLockWaitForEvent(&ExclusiveContext.AcquiredEvent);
    ok_eq_hex(Status, STATUS_SUCCESS);

    if (Status != STATUS_SUCCESS)
    {
        /* Allow either waiter to finish if the expected handoff failed.
           Signaling both avoids making cleanup depend on which one acquired. */
        KeSetEvent(&ExclusiveContext.ReleaseEvent, IO_NO_INCREMENT, FALSE);
        KeSetEvent(&SharedContext.ReleaseEvent, IO_NO_INCREMENT, FALSE);

        PushLockReleaseAndFinishThread(ExclusiveThread, &ExclusiveContext);
        PushLockReleaseAndFinishThread(SharedThread, &SharedContext);
        return;
    }

    ok_eq_long(KeReadStateEvent(&SharedContext.AcquiredEvent), 0);
    ok_eq_long(State.ActiveWriters, 1);
    ok_eq_long(State.ActiveReaders, 0);

    /* While the x waiter owns the lock, the newer s waiter must still be
       the complete remaining wait chain */
    Success = PushLockWaitForStableWaitChain(&State.Lock,
                                             ExpectedShared,
                                             RTL_NUMBER_OF(ExpectedShared),
                                             NULL);
    ok(Success, "Shared waiter did not remain queued behind the exclusive owner\n");

    /* Releasing the x owner now permits the remaining s waiter to acquire */
    PushLockReleaseAndFinishThread(ExclusiveThread, &ExclusiveContext);
    ExclusiveThread = NULL;

    Status = PushLockWaitForEvent(&SharedContext.AcquiredEvent);
    ok_eq_hex(Status, STATUS_SUCCESS);

    if (Status == STATUS_SUCCESS)
    {
        ok_eq_long(State.ActiveWriters, 0);
        ok_eq_long(State.ActiveReaders, 1);
    }

    PushLockReleaseAndFinishThread(SharedThread, &SharedContext);
    SharedThread = NULL;

    ok_eq_long(State.ActiveReaders, 0);
    ok_eq_long(State.ActiveWriters, 0);
    ok_eq_long(State.Violations, 0);
    ok_eq_ulongptr(PushLockReadValue(&State.Lock).Value, 0);

    return;

Cleanup:

    /* Let any successfully started waiter leave immediately after acquiring,
       regardless of how far queue construction progressed */
    if (ExclusiveThread != NULL)
    {
        KeSetEvent(&ExclusiveContext.ReleaseEvent, IO_NO_INCREMENT, FALSE);
    }

    if (SharedThread != NULL)
    {
        KeSetEvent(&SharedContext.ReleaseEvent, IO_NO_INCREMENT, FALSE);
    }

    for (Count = 0; Count < 3; Count++)
    {
        PushLockRelease(&State.Lock, PushLockModeShared, ReleaseKind);
    }

    KeLeaveCriticalRegion();

    PushLockReleaseAndFinishThread(ExclusiveThread, &ExclusiveContext);
    PushLockReleaseAndFinishThread(SharedThread, &SharedContext);
}

/*
 * TLA counter example: 1 waiter is already queued while another waiter is
 * allowed to arrive as the final shared owner releases.
 * A black box test can't stop the release routine between its load and cmpxchg,
 * so this only stresses the window...
 */
static
VOID
TestPushLockWaiterArrivalDuringRelease(VOID)
{
    static const PUSH_LOCK_MODE ExpectedOldest[] = {PushLockModeExclusive};
    PUSH_LOCK_TEST_STATE State;
    PUSH_LOCK_THREAD_CONTEXT OldestContext;
    PUSH_LOCK_THREAD_CONTEXT NewestContext;
    PKTHREAD OldestThread;
    PKTHREAD NewestThread;
    KEVENT StartGate;
    PUSH_LOCK_RELEASE_KIND ReleaseKind;
    ULONG Iteration;

    for (Iteration = 0; Iteration < PUSH_LOCK_RACE_ITERATIONS; Iteration++)
    {
        PushLockInitializeState(&State);
        OldestThread = NULL;
        NewestThread = NULL;
        KeInitializeEvent(&StartGate, NotificationEvent, FALSE);

        KeEnterCriticalRegion();
        ExfAcquirePushLockShared(&State.Lock);

        PushLockInitializeThreadContext(&OldestContext,
                                        &State,
                                        PushLockModeExclusive,
                                        PushLockReleaseSpecific);
        if (!PushLockStartControlledThread(&OldestContext, &OldestThread))
        {
            ExfReleasePushLockShared(&State.Lock);
            KeLeaveCriticalRegion();
            PushLockReleaseAndFinishThread(OldestThread, &OldestContext);
            continue;
        }

        ok(PushLockWaitForStableWaitChain(&State.Lock, ExpectedOldest,
               RTL_NUMBER_OF(ExpectedOldest), NULL),
           "Oldest waiter did not stabilize at iteration %lu\n",
           Iteration);

        PushLockInitializeThreadContext(&NewestContext,
                                        &State,
                                        PushLockModeExclusive,
                                        PushLockReleaseSpecific);
        NewestContext.StartGate = &StartGate;

        if (!PushLockStartControlledThread(&NewestContext, &NewestThread))
        {
            KeSetEvent(&StartGate, IO_NO_INCREMENT, FALSE);
            ExfReleasePushLockShared(&State.Lock);
            KeLeaveCriticalRegion();
            PushLockReleaseAndFinishThread(OldestThread, &OldestContext);
            PushLockReleaseAndFinishThread(NewestThread, &NewestContext);
            continue;
        }

        /* Don't hold either waiter after acquisition. The test is interested in
           completion of the entire queue, not in inspecting an intermediate owner... */
        KeSetEvent(&OldestContext.ReleaseEvent, IO_NO_INCREMENT, FALSE);
        KeSetEvent(&NewestContext.ReleaseEvent, IO_NO_INCREMENT, FALSE);

        /* Exercise the same arrival window through both the s specific and
           generic release */
        ReleaseKind = (Iteration & 1)
                          ? PushLockReleaseGeneric
                          : PushLockReleaseSpecific;

        /* Make the new waiter ready immediately before the final shared release.
           The scheduler may insert it before or during the release. This can't
           force the exact load/CAS interleaving, but repeatedly exposes the
           implementation to the execution found by the tla */
        KeSetEvent(&StartGate, IO_NO_INCREMENT, FALSE);
        PushLockRelease(&State.Lock, PushLockModeShared, ReleaseKind);

        KeLeaveCriticalRegion();

        PushLockReleaseAndFinishThread(OldestThread, &OldestContext);
        PushLockReleaseAndFinishThread(NewestThread, &NewestContext);

        ok_eq_long(KeReadStateEvent(&OldestContext.AcquiredEvent), 1);
        ok_eq_long(KeReadStateEvent(&NewestContext.AcquiredEvent), 1);
        ok_eq_long(State.Violations, 0);
        ok_eq_ulongptr(PushLockReadValue(&State.Lock).Value, 0);
    }
}

/*
 * The contention workers use fixed roles and a deterministic release pattern.
 * 2 writers and 4 readers start together, repeatedly validate protected data,
 * adn use both the generic and mode specific releases.
 */
static
VOID
NTAPI
PushLockContentionThread(
    _In_ PVOID Parameter)
{
    PPUSH_LOCK_THREAD_CONTEXT Context;
    PPUSH_LOCK_TEST_STATE State;
    PUSH_LOCK_RELEASE_KIND ReleaseKind;
    NTSTATUS Status;
    ULONG Iteration;

    Context = Parameter;
    State = Context->State;

    KeSetEvent(&Context->ReadyEvent, IO_NO_INCREMENT, FALSE);

    Status = KeWaitForSingleObject(Context->StartGate,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   NULL);
    if (Status != STATUS_SUCCESS)
    {
        PushLockRecordViolation(State);
        KeSetEvent(&Context->DoneEvent, IO_NO_INCREMENT, FALSE);
        return;
    }

    for (Iteration = 0; Iteration < Context->Iterations; Iteration++)
    {
        /* Alternate release per worker and iteration so every role uses
           both the generic and mode specific release functions */
        ReleaseKind = ((Iteration + Context->Index) & 1)
                          ? PushLockReleaseGeneric
                          : PushLockReleaseSpecific;

        KeEnterCriticalRegion();

        PushLockAcquire(&State->Lock, Context->Mode);
        /* The ownership counters and protected value checks validate the
           exclusion contract without relying on the internal push lock fields */
        PushLockEnterProtectedRegion(State, Context->Mode);

        if (Context->Mode == PushLockModeExclusive)
        {
            PushLockWriteProtectedValue(State);
        }
        else
        {
            PushLockReadProtectedValue(State);
        }

        PushLockLeaveProtectedRegion(State, Context->Mode);
        PushLockRelease(&State->Lock, Context->Mode, ReleaseKind);

        KeLeaveCriticalRegion();

        InterlockedIncrement(&State->CompletedOperations);

        /* Occasionally sample the encoded lock state and yield to increase useful nterleavings */
        if ((Iteration & 0x3f) == 0)
        {
            PushLockSampleState(State);
            YieldProcessor();
        }
    }

    KeSetEvent(&Context->DoneEvent, IO_NO_INCREMENT, FALSE);
}

static
VOID
TestPushLockContention(VOID)
{
    PUSH_LOCK_TEST_STATE State;
    PUSH_LOCK_THREAD_CONTEXT Contexts[PUSH_LOCK_CONTENTION_THREADS];
    PKTHREAD Threads[PUSH_LOCK_CONTENTION_THREADS] = {NULL};
    KEVENT StartGate;
    NTSTATUS Status;
    ULONG Index;
    ULONG Started;
    ULONG WriterCount;
    ULONG ReaderCount;

    PushLockInitializeState(&State);
    KeInitializeEvent(&StartGate, NotificationEvent, FALSE);

    Started = 0;
    WriterCount = 0;
    ReaderCount = 0;

    for (Index = 0; Index < PUSH_LOCK_CONTENTION_THREADS; Index++)
    {
        PUSH_LOCK_MODE Mode;

        /* Threads 0 and 3 are writers; the remaining 4 are readers */
        Mode = ((Index % 3) == 0) ? PushLockModeExclusive : PushLockModeShared;

        if (Mode == PushLockModeExclusive)
        {
            WriterCount++;
        }
        else
        {
            ReaderCount++;
        }

        PushLockInitializeThreadContext(&Contexts[Index],
                                        &State,
                                        Mode,
                                        PushLockReleaseSpecific);
        Contexts[Index].StartGate = &StartGate;
        Contexts[Index].Index = Index;
        Contexts[Index].Iterations = PUSH_LOCK_CONTENTION_ITERATIONS;

        Threads[Index] = KmtStartThread(PushLockContentionThread,
                                        &Contexts[Index]);
        if (Threads[Index] == NULL)
        {
            ok(FALSE, "Could not create contention thread %lu\n", Index);
            break;
        }

        Started++;

        Status = PushLockWaitForEvent(&Contexts[Index].ReadyEvent);
        ok_eq_hex(Status, STATUS_SUCCESS);

        if (Status != STATUS_SUCCESS)
        {
            break;
        }
    }

    /* Release all successfully created workers together so their first
       acquisitions contend on the same initially unlocked push lock */
    KeSetEvent(&StartGate, IO_NO_INCREMENT, FALSE);

    for (Index = 0; Index < Started; Index++)
    {
        Status = PushLockWaitForEvent(&Contexts[Index].DoneEvent);
        ok_eq_hex(Status, STATUS_SUCCESS);
        KmtFinishThread(Threads[Index], NULL);
    }

    ok_eq_long(State.ActiveReaders, 0);
    ok_eq_long(State.ActiveWriters, 0);
    ok_eq_long(State.Violations, 0);
    ok_eq_long(State.CompletedOperations,
               (LONG)(Started * PUSH_LOCK_CONTENTION_ITERATIONS));

    if (Started == PUSH_LOCK_CONTENTION_THREADS)
    {
        ok_eq_long(State.ExclusiveAcquisitions,
                   (LONG)(WriterCount * PUSH_LOCK_CONTENTION_ITERATIONS));
        ok_eq_long(State.SharedAcquisitions,
                   (LONG)(ReaderCount * PUSH_LOCK_CONTENTION_ITERATIONS));
    }

    ok_eq_ulongptr(PushLockReadValue(&State.Lock).Value, 0);
}

START_TEST(ExPushLock)
{
    TestPushLockUncontended(PushLockReleaseSpecific);
    TestPushLockUncontended(PushLockReleaseGeneric);

    TestPushLockWaiterSelection();

    TestPushLockSharedOwnerDrain(PushLockReleaseSpecific);
    TestPushLockSharedOwnerDrain(PushLockReleaseGeneric);

    TestPushLockWaiterArrivalDuringRelease();
    TestPushLockContention();
}
