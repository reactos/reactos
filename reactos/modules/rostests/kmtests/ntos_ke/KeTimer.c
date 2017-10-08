/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Timer test
 * PROGRAMMER:      Rafal Harabien <rafalh@reactos.org>
 */

#include <kmt_test.h>

#define CheckTimer(Timer, ExpectedType, State, ExpectedWaitNext,                \
                            Irql, ThreadList, ThreadCount) do                   \
{                                                                               \
    INT TheIndex;                                                               \
    PLIST_ENTRY TheEntry;                                                       \
    PKTHREAD TheThread;                                                         \
    ok_eq_uint((Timer)->Header.Type, ExpectedType);                             \
    ok_eq_uint((Timer)->Header.Hand, sizeof *(Timer) / sizeof(ULONG));          \
    ok_eq_hex((Timer)->Header.Lock & 0xFF00FF00L, 0x00005500L);                 \
    ok_eq_long((Timer)->Header.SignalState, State);                             \
    TheEntry = (Timer)->Header.WaitListHead.Flink;                              \
    for (TheIndex = 0; TheIndex < (ThreadCount); ++TheIndex)                    \
    {                                                                           \
        TheThread = CONTAINING_RECORD(TheEntry, KTHREAD,                        \
                                        WaitBlock[0].WaitListEntry);            \
        ok_eq_pointer(TheThread, (ThreadList)[TheIndex]);                       \
        ok_eq_pointer(TheEntry->Flink->Blink, TheEntry);                        \
        TheEntry = TheEntry->Flink;                                             \
    }                                                                           \
    ok_eq_pointer(TheEntry, &(Timer)->Header.WaitListHead);                     \
    ok_eq_pointer(TheEntry->Flink->Blink, TheEntry);                            \
    ok_eq_long(KeReadStateTimer(Timer), State);                                 \
    ok_eq_bool(Thread->WaitNext, ExpectedWaitNext);                             \
    ok_irql(Irql);                                                              \
} while (0)

static
VOID
TestTimerFunctional(
    IN PKTIMER Timer,
    IN TIMER_TYPE Type,
    IN KIRQL OriginalIrql)
{
    PKTHREAD Thread = KeGetCurrentThread();

    memset(Timer, 0x55, sizeof *Timer);
    KeInitializeTimerEx(Timer, Type);
    CheckTimer(Timer, TimerNotificationObject + Type, 0L, FALSE, OriginalIrql, (PVOID *)NULL, 0);
}

START_TEST(KeTimer)
{
    KTIMER Timer;
    KIRQL Irql;
    KIRQL Irqls[] = { PASSIVE_LEVEL, APC_LEVEL, DISPATCH_LEVEL, HIGH_LEVEL };
    INT i;

    for (i = 0; i < sizeof Irqls / sizeof Irqls[0]; ++i)
    {
        /* DRIVER_IRQL_NOT_LESS_OR_EQUAL (TODO: on MP only?) */
        if (Irqls[i] > DISPATCH_LEVEL && KmtIsCheckedBuild)
            return;
        KeRaiseIrql(Irqls[i], &Irql);
        TestTimerFunctional(&Timer, NotificationTimer, Irqls[i]);
        TestTimerFunctional(&Timer, SynchronizationTimer, Irqls[i]);
        KeLowerIrql(Irql);
    }

    ok_irql(PASSIVE_LEVEL);
    KmtSetIrql(PASSIVE_LEVEL);
}
