/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Event test
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

#include <kmt_test.h>

/* TODO: more thread testing, exports vs macros */

#define CheckEvent(Event, ExpectedType, State, ExpectedWaitNext, Irql) do       \
{                                                                               \
    ok_eq_uint((Event)->Header.Type, ExpectedType);                             \
    ok_eq_uint((Event)->Header.Hand, sizeof *(Event) / sizeof(ULONG));          \
    ok_eq_long((Event)->Header.Lock & 0xFF00FF00L, 0x55005500L);                \
    ok_eq_long((Event)->Header.SignalState, State);                             \
    ok_eq_pointer((Event)->Header.WaitListHead.Flink,                           \
                    &(Event)->Header.WaitListHead);                             \
    ok_eq_pointer((Event)->Header.WaitListHead.Blink,                           \
                    &(Event)->Header.WaitListHead);                             \
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
    CheckEvent(Event, Type, 0L, FALSE, OriginalIrql);

    memset(Event, 0x55, sizeof *Event);
    KeInitializeEvent(Event, Type, TRUE);
    CheckEvent(Event, Type, 1L, FALSE, OriginalIrql);

    Event->Header.SignalState = 0x12345678L;
    CheckEvent(Event, Type, 0x12345678L, FALSE, OriginalIrql);

    State = KePulseEvent(Event, 0, FALSE);
    CheckEvent(Event, Type, 0L, FALSE, OriginalIrql);
    ok_eq_long(State, 0x12345678L);

    Event->Header.SignalState = 0x12345678L;
    KeClearEvent(Event);
    CheckEvent(Event, Type, 0L, FALSE, OriginalIrql);

    State = KeSetEvent(Event, 0, FALSE);
    CheckEvent(Event, Type, 1L, FALSE, OriginalIrql);
    ok_eq_long(State, 0L);

    State = KeResetEvent(Event);
    CheckEvent(Event, Type, 0L, FALSE, OriginalIrql);
    ok_eq_long(State, 1L);

    Event->Header.SignalState = 0x23456789L;
    State = KeSetEvent(Event, 0, FALSE);
    CheckEvent(Event, Type, 1L, FALSE, OriginalIrql);
    ok_eq_long(State, 0x23456789L);

    Event->Header.SignalState = 0x3456789AL;
    State = KeResetEvent(Event);
    CheckEvent(Event, Type, 0L, FALSE, OriginalIrql);
    ok_eq_long(State, 0x3456789AL);

    if (OriginalIrql <= DISPATCH_LEVEL || !KmtIsCheckedBuild)
    {
        Event->Header.SignalState = 0x456789ABL;
        State = KeSetEvent(Event, 0, TRUE);
        CheckEvent(Event, Type, 1L, TRUE, DISPATCH_LEVEL);
        ok_eq_long(State, 0x456789ABL);
        ok_eq_uint(Thread->WaitIrql, OriginalIrql);
        /* repair the "damage" */
        Thread->WaitNext = FALSE;
        KmtSetIrql(OriginalIrql);

        Event->Header.SignalState = 0x56789ABCL;
        State = KePulseEvent(Event, 0, TRUE);
        CheckEvent(Event, Type, 0L, TRUE, DISPATCH_LEVEL);
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
    PKEVENT Event1;
    PKEVENT Event2;
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
    Status = KeWaitForSingleObject(ThreadData->Event1, Executive, KernelMode, FALSE, NULL);
    ok_irql(PASSIVE_LEVEL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ThreadData->Signal = TRUE;
    Status = KeWaitForSingleObject(ThreadData->Event2, Executive, KernelMode, FALSE, NULL);
    ok_irql(PASSIVE_LEVEL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_irql(PASSIVE_LEVEL);
}

static
VOID
TestEventThreads(
    IN PKEVENT Event,
    IN EVENT_TYPE Type,
    IN KIRQL OriginalIrql)
{
    NTSTATUS Status;
    THREAD_DATA Threads[5];
    LARGE_INTEGER Timeout;
    KPRIORITY Priority;
    KEVENT WaitEvent;
    KEVENT TerminateEvent;
    int i;
    Timeout.QuadPart = -1000 * 10;

    KeInitializeEvent(Event, Type, FALSE);
    KeInitializeEvent(&WaitEvent, NotificationEvent, FALSE);
    KeInitializeEvent(&TerminateEvent, SynchronizationEvent, FALSE);

    for (i = 0; i < sizeof Threads / sizeof Threads[0]; ++i)
    {
        Threads[i].Event1 = Event;
        Threads[i].Event2 = &TerminateEvent;
        Threads[i].Signal = FALSE;
        Status = PsCreateSystemThread(&Threads[i].Handle, GENERIC_ALL, NULL, NULL, NULL, WaitForEventThread, &Threads[i]);
        ok_eq_hex(Status, STATUS_SUCCESS);
        Status = ObReferenceObjectByHandle(Threads[i].Handle, SYNCHRONIZE, PsThreadType, KernelMode, (PVOID *)&Threads[i].Thread, NULL);
        ok_eq_hex(Status, STATUS_SUCCESS);
        Priority = KeQueryPriorityThread(Threads[i].Thread);
        ok_eq_long(Priority, 8L);
        while (!Threads[i].Signal)
        {
            Status = KeWaitForSingleObject(&WaitEvent, Executive, KernelMode, FALSE, &Timeout);
            ok_eq_hex(Status, STATUS_TIMEOUT);
        }
        Threads[i].Signal = FALSE;
    }

    for (i = 0; i < sizeof Threads / sizeof Threads[0]; ++i)
    {
        KeSetEvent(Event, 1, FALSE);
        while (!Threads[i].Signal)
        {
            Status = KeWaitForSingleObject(&WaitEvent, Executive, KernelMode, FALSE, &Timeout);
            ok_eq_hex(Status, STATUS_TIMEOUT);
        }
        Priority = KeQueryPriorityThread(Threads[i].Thread);
        ok_eq_long(Priority, 9L);
        KeSetEvent(&TerminateEvent, 0, FALSE);
        Status = KeWaitForSingleObject(Threads[i].Thread, Executive, KernelMode, FALSE, NULL);
        ok_eq_hex(Status, STATUS_SUCCESS);

        ObDereferenceObject(Threads[i].Thread);
        Status = ZwClose(Threads[i].Handle);
        ok_eq_hex(Status, STATUS_SUCCESS);
    }
}

START_TEST(KeEvent)
{
    KEVENT Event;
    KIRQL Irql;
    KIRQL Irqls[] = { PASSIVE_LEVEL, APC_LEVEL, DISPATCH_LEVEL, HIGH_LEVEL };
    int i;

    for (i = 0; i < sizeof Irqls / sizeof Irqls[0]; ++i)
    {
        KeRaiseIrql(Irqls[i], &Irql);
        TestEventFunctional(&Event, NotificationEvent, Irqls[i]);
        TestEventFunctional(&Event, SynchronizationEvent, Irqls[i]);
        KeLowerIrql(Irql);
    }

    TestEventThreads(&Event, NotificationEvent, PASSIVE_LEVEL);

    ok_irql(PASSIVE_LEVEL);
    KmtSetIrql(PASSIVE_LEVEL);
}
