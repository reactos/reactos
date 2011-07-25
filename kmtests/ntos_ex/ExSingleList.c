/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Singly-linked list test
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

struct _SINGLE_LIST_ENTRY;
struct _SINGLE_LIST_ENTRY *__stdcall ExInterlockedPushEntryList(struct _SINGLE_LIST_ENTRY *, struct _SINGLE_LIST_ENTRY *, unsigned long *);
struct _SINGLE_LIST_ENTRY *__stdcall ExInterlockedPopEntryList(struct _SINGLE_LIST_ENTRY *, unsigned long *);

#include <ntddk.h>
#include <kmt_test.h>

SINGLE_LIST_ENTRY Entries[5];

#define ok_eq_free2(Value, Expected) do              \
{                                                   \
    if (KmtIsCheckedBuild)                          \
        ok_eq_pointer(Value, (PVOID)0xBADDD0FF);    \
    else                                            \
        ok_eq_pointer(Value, Expected);             \
} while (0)

START_TEST(ExSingleList)
{
    KSPIN_LOCK SpinLock;
    SINGLE_LIST_ENTRY ListHead;
    PSINGLE_LIST_ENTRY Ret;

    KeInitializeSpinLock(&SpinLock);

    memset(Entries, 0x55, sizeof Entries);
    ListHead.Next = NULL;
    Ret = ExInterlockedPushEntryList(&ListHead, &Entries[0], &SpinLock);
    ok_eq_pointer(Ret, NULL);
    ok_eq_pointer(ListHead.Next, &Entries[0]);
    ok_eq_pointer(Entries[0].Next, NULL);
    ok_bool_true(KmtAreInterruptsEnabled(), "Interrupts enabled:");
    ok_irql(PASSIVE_LEVEL);

    Ret = ExInterlockedPopEntryList(&ListHead, &SpinLock);
    ok_eq_pointer(Ret, &Entries[0]);
    ok_eq_pointer(ListHead.Next, NULL);
    ok_eq_free2(Entries[0].Next, NULL);
    ok_bool_true(KmtAreInterruptsEnabled(), "Interrupts enabled:");
    ok_irql(PASSIVE_LEVEL);

    Ret = ExInterlockedPopEntryList(&ListHead, &SpinLock);
    ok_eq_pointer(Ret, NULL);
    ok_eq_pointer(ListHead.Next, NULL);
    ok_eq_free2(Entries[0].Next, NULL);
    ok_bool_true(KmtAreInterruptsEnabled(), "Interrupts enabled:");
    ok_irql(PASSIVE_LEVEL);

    Ret = ExInterlockedPushEntryList(&ListHead, &Entries[0], &SpinLock);
    ok_eq_pointer(Ret, NULL);
    ok_eq_pointer(ListHead.Next, &Entries[0]);
    ok_eq_pointer(Entries[0].Next, NULL);
    ok_bool_true(KmtAreInterruptsEnabled(), "Interrupts enabled:");
    ok_irql(PASSIVE_LEVEL);

    Ret = ExInterlockedPushEntryList(&ListHead, &Entries[1], &SpinLock);
    ok_eq_pointer(Ret, &Entries[0]);
    ok_eq_pointer(ListHead.Next, &Entries[1]);
    ok_eq_pointer(Entries[1].Next, &Entries[0]);
    ok_eq_pointer(Entries[0].Next, NULL);
    ok_bool_true(KmtAreInterruptsEnabled(), "Interrupts enabled:");
    ok_irql(PASSIVE_LEVEL);

    Ret = ExInterlockedPopEntryList(&ListHead, &SpinLock);
    ok_eq_pointer(Ret, &Entries[1]);
    ok_eq_pointer(ListHead.Next, &Entries[0]);
    ok_eq_free2(Entries[1].Next, &Entries[0]);
    ok_eq_pointer(Entries[0].Next, NULL);
    ok_bool_true(KmtAreInterruptsEnabled(), "Interrupts enabled:");
    ok_irql(PASSIVE_LEVEL);

#undef ExInterlockedPushEntryList
#undef ExInterlockedPopEntryList
    memset(Entries, 0x55, sizeof Entries);
    ListHead.Next = NULL;
    Ret = ExInterlockedPushEntryList(&ListHead, &Entries[0], &SpinLock);
    ok_eq_pointer(Ret, NULL);
    ok_eq_pointer(ListHead.Next, &Entries[0]);
    ok_eq_pointer(Entries[0].Next, NULL);
    ok_bool_true(KmtAreInterruptsEnabled(), "Interrupts enabled:");
    ok_irql(PASSIVE_LEVEL);

    Ret = ExInterlockedPopEntryList(&ListHead, &SpinLock);
    ok_eq_pointer(Ret, &Entries[0]);
    ok_eq_pointer(ListHead.Next, NULL);
    ok_eq_free2(Entries[0].Next, NULL);
    ok_bool_true(KmtAreInterruptsEnabled(), "Interrupts enabled:");
    ok_irql(PASSIVE_LEVEL);

    Ret = ExInterlockedPopEntryList(&ListHead, &SpinLock);
    ok_eq_pointer(Ret, NULL);
    ok_eq_pointer(ListHead.Next, NULL);
    ok_eq_free2(Entries[0].Next, NULL);
    ok_bool_true(KmtAreInterruptsEnabled(), "Interrupts enabled:");
    ok_irql(PASSIVE_LEVEL);

    Ret = ExInterlockedPushEntryList(&ListHead, &Entries[0], &SpinLock);
    ok_eq_pointer(Ret, NULL);
    ok_eq_pointer(ListHead.Next, &Entries[0]);
    ok_eq_pointer(Entries[0].Next, NULL);
    ok_bool_true(KmtAreInterruptsEnabled(), "Interrupts enabled:");
    ok_irql(PASSIVE_LEVEL);

    Ret = ExInterlockedPushEntryList(&ListHead, &Entries[1], &SpinLock);
    ok_eq_pointer(Ret, &Entries[0]);
    ok_eq_pointer(ListHead.Next, &Entries[1]);
    ok_eq_pointer(Entries[1].Next, &Entries[0]);
    ok_eq_pointer(Entries[0].Next, NULL);
    ok_bool_true(KmtAreInterruptsEnabled(), "Interrupts enabled:");
    ok_irql(PASSIVE_LEVEL);

    Ret = ExInterlockedPopEntryList(&ListHead, &SpinLock);
    ok_eq_pointer(Ret, &Entries[1]);
    ok_eq_pointer(ListHead.Next, &Entries[0]);
    ok_eq_free2(Entries[1].Next, &Entries[0]);
    ok_eq_pointer(Entries[0].Next, NULL);
    ok_bool_true(KmtAreInterruptsEnabled(), "Interrupts enabled:");
    ok_irql(PASSIVE_LEVEL);

    KmtSetIrql(PASSIVE_LEVEL);
}
