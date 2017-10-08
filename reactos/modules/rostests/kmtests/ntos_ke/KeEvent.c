/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Event test
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <kmt_test.h>

#define CheckEvent(Event, ExpectedType, State, ExpectedWaitNext,                \
                            Irql, ThreadList, ThreadCount) do                   \
{                                                                               \
    INT TheIndex;                                                               \
    PLIST_ENTRY TheEntry;                                                       \
    PKTHREAD TheThread;                                                         \
    ok_eq_uint((Event)->Header.Type, ExpectedType);                             \
    ok_eq_uint((Event)->Header.Hand, sizeof *(Event) / sizeof(ULONG));          \
    ok_eq_hex((Event)->Header.Lock & 0xFF00FF00L, 0x55005500L);                 \
    ok_eq_long((Event)->Header.SignalState, State);                             \
    TheEntry = (Event)->Header.WaitListHead.Flink;                              \
    for (TheIndex = 0; TheIndex < (ThreadCount); ++TheIndex)                    \
    {                                                                           \
        TheThread = CONTAINING_RECORD(TheEntry, KTHREAD,                        \
                                      WaitBlock[0].WaitListEntry);              \
        ok_eq_pointer(TheThread, (ThreadList)[TheIndex]);                       \
        ok_eq_pointer(TheEntry->Flink->Blink, TheEntry);                        \
        TheEntry = TheEntry->Flink;                                             \
    }                                                                           \
    ok_eq_pointer(TheEntry, &(Event)->Header.WaitListHead);                     \
    ok_eq_pointer(TheEntry->Flink->Blink, TheEntry);                            \
    ok_eq_long(KeReadStateEvent(Event), State);                                 \
    ok_eq_bool(Thread->WaitNext, ExpectedWaitNext);                             \
    ok_irql(Irql);                                                              \
} while (0)

static
VOID
TestEventFunctional(
    IN PKEVENT Event,
    IN EVENT_TYPE Type,
    IN KIRQL OriginalIrql)
{
    LONG State;
    PKTHREAD Thread = KeGetCurrentThread();

    memset(Event, 0x55, sizeof *Event);
    KeInitializeEvent(Event, Type, FALSE);
    CheckEvent(Event, Type, 0L, FALSE, OriginalIrql, (PVOID *)NULL, 0);

    memset(Event, 0x55, sizeof *Event);
    KeInitializeEvent(Event, Type, TRUE);
    CheckEvent(Event, Type, 1L, FALSE, OriginalIrql, (PVOID *)NULL, 0);

    Event->Header.SignalState = 0x12345678L;
    CheckEvent(Event, Type, 0x12345678L, FALSE, OriginalIrql, (PVOID *)NULL, 0);

    State = KePulseEvent(Event, 0, FALSE);
    CheckEvent(Event, Type, 0L, FALSE, OriginalIrql, (PVOID *)NULL, 0);
    ok_eq_long(State, 0x12345678L);

    Event->Header.SignalState = 0x12345678L;
    KeClearEvent(Event);
    CheckEvent(Event, Type, 0L, FALSE, OriginalIrql, (PVOID *)NULL, 0);

    State = KeSetEvent(Event, 0, FALSE);
    CheckEvent(Event, Type, 1L, FALSE, OriginalIrql, (PVOID *)NULL, 0);
    ok_eq_long(State, 0L);

    State = KeResetEvent(Event);
    CheckEvent(Event, Type, 0L, FALSE, OriginalIrql, (PVOID *)NULL, 0);
    ok_eq_long(State, 1L);

    Event->Header.SignalState = 0x23456789L;
    State = KeSetEvent(Event, 0, FALSE);
    CheckEvent(Event, Type, 1L, FALSE, OriginalIrql, (PVOID *)NULL, 0);
    ok_eq_long(State, 0x23456789L);

    Event->Header.SignalState = 0x3456789AL;
    State = KeResetEvent(Event);
    CheckEvent(Event, Type, 0L, FALSE, OriginalIrql, (PVOID *)NULL, 0);
    ok_eq_long(State, 0x3456789AL);

    /* Irql is raised to DISPATCH_LEVEL here, which kills checked build,
     * a spinlock is acquired and never released, which kills MP build */
    if ((OriginalIrql <= DISPATCH_LEVEL || !KmtIsCheckedBuild) &&
        !KmtIsMultiProcessorBuild)
    {
        Event->Header.SignalState = 0x456789ABL;
        State = KeSetEvent(Event, 0, TRUE);
        CheckEvent(Event, Type, 1L, TRUE, DISPATCH_LEVEL, (PVOID *)NULL, 0);
        ok_eq_long(State, 0x456789ABL);
        ok_eq_uint(Thread->WaitIrql, OriginalIrql);
        /* repair the "damage" */
        Thread->WaitNext = FALSE;
        KmtSetIrql(OriginalIrql);

        Event->Header.SignalState = 0x56789ABCL;
        State = KePulseEvent(Event, 0, TRUE);
        CheckEvent(Event, Type, 0L, TRUE, DISPATCH_LEVEL, (PVOID *)NULL, 0);
        ok_eq_long(State, 0x56789ABCL);
        ok_eq_uint(Thread->WaitIrql, OriginalIrql);
        /* repair the "damage" */
        Thread->WaitNext = FALSE;
        KmtSetIrql(OriginalIrql);
    }

    ok_irql(OriginalIrql);
    KmtSetIrql(OriginalIrql);
}

typedef struct
{
    HANDLE Handle;
    PKTHREAD Thread;
    PKEVENT Event;
    volatile BOOLEAN Signal;
} THREAD_DATA, *PTHREAD_DATA;

static
VOID
NTAPI
WaitForEventThread(
    IN OUT PVOID Context)
{
    NTSTATUS Status;
    PTHREAD_DATA ThreadData = Context;

    ok_irql(PASSIVE_LEVEL);
    ThreadData->Signal = TRUE;
    Status = KeWaitForSingleObject(ThreadData->Event, Executive, KernelMode, FALSE, NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_irql(PASSIVE_LEVEL);
}

typedef LONG (NTAPI *PSET_EVENT_FUNCTION)(PRKEVENT, KPRIORITY, BOOLEAN);

static
VOID
TestEventConcurrent(
    IN PKEVENT Event,
    IN EVENT_TYPE Type,
    IN KIRQL OriginalIrql,
    PSET_EVENT_FUNCTION SetEvent,
    KPRIORITY PriorityIncrement,
    LONG ExpectedState,
    BOOLEAN SatisfiesAll)
{
    NTSTATUS Status;
    THREAD_DATA Threads[5];
    const INT ThreadCount = sizeof Threads / sizeof Threads[0];
    KPRIORITY Priority;
    LARGE_INTEGER LongTimeout, ShortTimeout;
    INT i;
    KWAIT_BLOCK WaitBlock[RTL_NUMBER_OF(Threads)];
    PVOID ThreadObjects[RTL_NUMBER_OF(Threads)];
    LONG State;
    PKTHREAD Thread = KeGetCurrentThread();
    OBJECT_ATTRIBUTES ObjectAttributes;

    LongTimeout.QuadPart = -100 * MILLISECOND;
    ShortTimeout.QuadPart = -1 * MILLISECOND;

    KeInitializeEvent(Event, Type, FALSE);

    for (i = 0; i < ThreadCount; ++i)
    {
        Threads[i].Event = Event;
        Threads[i].Signal = FALSE;
        InitializeObjectAttributes(&ObjectAttributes,
                                   NULL,
                                   OBJ_KERNEL_HANDLE,
                                   NULL,
                                   NULL);
        Status = PsCreateSystemThread(&Threads[i].Handle, GENERIC_ALL, &ObjectAttributes, NULL, NULL, WaitForEventThread, &Threads[i]);
        ok_eq_hex(Status, STATUS_SUCCESS);
        Status = ObReferenceObjectByHandle(Threads[i].Handle, SYNCHRONIZE, *PsThreadType, KernelMode, (PVOID *)&Threads[i].Thread, NULL);
        ok_eq_hex(Status, STATUS_SUCCESS);
        ThreadObjects[i] = Threads[i].Thread;
        Priority = KeQueryPriorityThread(Threads[i].Thread);
        ok_eq_long(Priority, 8L);
        while (!Threads[i].Signal)
        {
            Status = KeDelayExecutionThread(KernelMode, FALSE, &ShortTimeout);
            if (Status != STATUS_SUCCESS)
            {
                ok_eq_hex(Status, STATUS_SUCCESS);
            }
        }
        CheckEvent(Event, Type, 0L, FALSE, OriginalIrql, ThreadObjects, i + 1);
    }

    /* the threads shouldn't wake up on their own */
    Status = KeDelayExecutionThread(KernelMode, FALSE, &ShortTimeout);
    ok_eq_hex(Status, STATUS_SUCCESS);

    for (i = 0; i < ThreadCount; ++i)
    {
        CheckEvent(Event, Type, 0L, FALSE, OriginalIrql, ThreadObjects + i, ThreadCount - i);
        State = SetEvent(Event, PriorityIncrement + i, FALSE);

        ok_eq_long(State, 0L);
        CheckEvent(Event, Type, ExpectedState, FALSE, OriginalIrql, ThreadObjects + i + 1, SatisfiesAll ? 0 : ThreadCount - i - 1);
        Status = KeWaitForMultipleObjects(ThreadCount, ThreadObjects, SatisfiesAll ? WaitAll : WaitAny, Executive, KernelMode, FALSE, &LongTimeout, WaitBlock);
        ok_eq_hex(Status, STATUS_WAIT_0 + i);
        if (SatisfiesAll)
        {
            for (; i < ThreadCount; ++i)
            {
                Priority = KeQueryPriorityThread(Threads[i].Thread);
                ok_eq_long(Priority, max(min(8L + PriorityIncrement, 15L), 8L));
            }
            break;
        }
        Priority = KeQueryPriorityThread(Threads[i].Thread);
        ok_eq_long(Priority, max(min(8L + PriorityIncrement + i, 15L), 8L));
        /* replace the thread with the current thread - which will never signal */
        if (!skip((Status & 0x3F) < ThreadCount, "Index out of bounds"))
            ThreadObjects[Status & 0x3F] = Thread;
        Status = KeWaitForMultipleObjects(ThreadCount, ThreadObjects, WaitAny, Executive, KernelMode, FALSE, &ShortTimeout, WaitBlock);
        ok_eq_hex(Status, STATUS_TIMEOUT);
    }

    for (i = 0; i < ThreadCount; ++i)
    {
        ObDereferenceObject(Threads[i].Thread);
        Status = ZwClose(Threads[i].Handle);
        ok_eq_hex(Status, STATUS_SUCCESS);
    }
}

#define NUM_SCHED_TESTS 1000

typedef struct
{
    KEVENT Event;
    KEVENT WaitEvent;
    ULONG Counter;
    KPRIORITY PriorityIncrement;
    ULONG CounterValues[NUM_SCHED_TESTS];
} COUNT_THREAD_DATA, *PCOUNT_THREAD_DATA;

static
VOID
NTAPI
CountThread(
    IN OUT PVOID Context)
{
    PCOUNT_THREAD_DATA ThreadData = Context;
    PKEVENT Event = &ThreadData->Event;
    volatile ULONG *Counter = &ThreadData->Counter;
    ULONG *CounterValue = ThreadData->CounterValues;
    KPRIORITY Priority;

    Priority = KeQueryPriorityThread(KeGetCurrentThread());
    ok_eq_long(Priority, 8L);

    while (CounterValue < &ThreadData->CounterValues[NUM_SCHED_TESTS])
    {
        KeSetEvent(&ThreadData->WaitEvent, IO_NO_INCREMENT, TRUE);
        KeWaitForSingleObject(Event, Executive, KernelMode, FALSE, NULL);
        *CounterValue++ = *Counter;
    }

    Priority = KeQueryPriorityThread(KeGetCurrentThread());
    ok_eq_long(Priority, 8L + min(ThreadData->PriorityIncrement, 7));
}

static
VOID
NTAPI
TestEventScheduling(
    _In_ PVOID Context)
{
    PCOUNT_THREAD_DATA ThreadData;
    PKTHREAD Thread;
    NTSTATUS Status;
    LONG PreviousState;
    ULONG i;
    volatile ULONG *Counter;
    KPRIORITY PriorityIncrement;
    KPRIORITY Priority;

    UNREFERENCED_PARAMETER(Context);

    ThreadData = ExAllocatePoolWithTag(PagedPool, sizeof(*ThreadData), 'CEmK');
    if (skip(ThreadData != NULL, "Out of memory\n"))
    {
        return;
    }
    KeInitializeEvent(&ThreadData->Event, SynchronizationEvent, FALSE);
    KeInitializeEvent(&ThreadData->WaitEvent, SynchronizationEvent, FALSE);
    Counter = &ThreadData->Counter;

    for (PriorityIncrement = 0; PriorityIncrement <= 8; PriorityIncrement++)
    {
        ThreadData->PriorityIncrement = PriorityIncrement;
        ThreadData->Counter = 0;
        RtlFillMemory(ThreadData->CounterValues,
                      sizeof(ThreadData->CounterValues),
                      0xFE);
        Thread = KmtStartThread(CountThread, ThreadData);
        Priority = KeQueryPriorityThread(KeGetCurrentThread());
        ok(Priority == 8, "[%lu] Priority = %lu\n", PriorityIncrement, Priority);
        for (i = 1; i <= NUM_SCHED_TESTS; i++)
        {
            Status = KeWaitForSingleObject(&ThreadData->WaitEvent, Executive, KernelMode, FALSE, NULL);
            ok_eq_hex(Status, STATUS_SUCCESS);
            PreviousState = KeSetEvent(&ThreadData->Event, PriorityIncrement, FALSE);
            *Counter = i;
            ok_eq_long(PreviousState, 0L);
        }
        Priority = KeQueryPriorityThread(KeGetCurrentThread());
        ok(Priority == 8, "[%lu] Priority = %lu\n", PriorityIncrement, Priority);
        KmtFinishThread(Thread, NULL);

        if (PriorityIncrement == 0)
        {
            /* Both threads have the same priority, so either can win the race */
            ok(ThreadData->CounterValues[0] == 0 || ThreadData->CounterValues[0] == 1,
               "[%lu] Counter 0 = %lu\n",
               PriorityIncrement, ThreadData->CounterValues[0]);
        }
        else
        {
            /* CountThread has the higher priority, it will always win */
            ok(ThreadData->CounterValues[0] == 0,
               "[%lu] Counter 0 = %lu\n",
               PriorityIncrement, ThreadData->CounterValues[0]);
        }
        for (i = 1; i < NUM_SCHED_TESTS; i++)
        {
            if (PriorityIncrement == 0)
            {
                ok(ThreadData->CounterValues[i] == i ||
                   ThreadData->CounterValues[i] == i + 1,
                   "[%lu] Counter %lu = %lu, expected %lu or %lu\n",
                   PriorityIncrement, i,
                   ThreadData->CounterValues[i], i, i + 1);
            }
            else
            {
                ok(ThreadData->CounterValues[i] == ThreadData->CounterValues[i - 1] + 1,
                   "[%lu] Counter %lu = %lu, expected %lu\n",
                   PriorityIncrement, i,
                   ThreadData->CounterValues[i], ThreadData->CounterValues[i - 1] + 1);
            }
        }
    }

    ExFreePoolWithTag(ThreadData, 'CEmK');
}

START_TEST(KeEvent)
{
    PKTHREAD Thread;
    KEVENT Event;
    KIRQL Irql;
    KIRQL Irqls[] = { PASSIVE_LEVEL, APC_LEVEL, DISPATCH_LEVEL };
    ULONG i;
    KPRIORITY PriorityIncrement;

    for (i = 0; i < RTL_NUMBER_OF(Irqls); ++i)
    {
        KeRaiseIrql(Irqls[i], &Irql);
        TestEventFunctional(&Event, NotificationEvent, Irqls[i]);
        TestEventFunctional(&Event, SynchronizationEvent, Irqls[i]);
        KeLowerIrql(Irql);
    }

    for (i = 0; i < RTL_NUMBER_OF(Irqls); ++i)
    {
        /* creating threads above DISPATCH_LEVEL... nope */
        if (Irqls[i] >= DISPATCH_LEVEL)
            continue;
        KeRaiseIrql(Irqls[i], &Irql);
        trace("IRQL: %u\n", Irqls[i]);
        for (PriorityIncrement = -1; PriorityIncrement <= 8; ++PriorityIncrement)
        {
            if (PriorityIncrement < 0 && KmtIsCheckedBuild)
                continue;
            trace("PriorityIncrement: %ld\n", PriorityIncrement);
            trace("-> Checking KeSetEvent, NotificationEvent\n");
            TestEventConcurrent(&Event, NotificationEvent, Irqls[i], KeSetEvent, PriorityIncrement, 1, TRUE);
            trace("-> Checking KeSetEvent, SynchronizationEvent\n");
            TestEventConcurrent(&Event, SynchronizationEvent, Irqls[i], KeSetEvent, PriorityIncrement, 0, FALSE);
            trace("-> Checking KePulseEvent, NotificationEvent\n");
            TestEventConcurrent(&Event, NotificationEvent, Irqls[i], KePulseEvent, PriorityIncrement, 0, TRUE);
            trace("-> Checking KePulseEvent, SynchronizationEvent\n");
            TestEventConcurrent(&Event, SynchronizationEvent, Irqls[i], KePulseEvent, PriorityIncrement, 0, FALSE);
        }
        KeLowerIrql(Irql);
    }

    ok_irql(PASSIVE_LEVEL);
    KmtSetIrql(PASSIVE_LEVEL);

    Thread = KmtStartThread(TestEventScheduling, NULL);
    KmtFinishThread(Thread, NULL);
}
