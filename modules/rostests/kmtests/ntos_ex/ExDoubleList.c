/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Doubly-linked list test
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

struct _LIST_ENTRY;
#ifdef _X86_
struct _LIST_ENTRY *__stdcall ExInterlockedInsertHeadList(struct _LIST_ENTRY *, struct _LIST_ENTRY *, unsigned long *);
struct _LIST_ENTRY *__stdcall ExInterlockedInsertTailList(struct _LIST_ENTRY *, struct _LIST_ENTRY *, unsigned long *);
struct _LIST_ENTRY *__stdcall ExInterlockedRemoveHeadList(struct _LIST_ENTRY *, unsigned long *);
#endif

#include <kmt_test.h>

LIST_ENTRY Entries[5];

#define ok_eq_free(Value, Expected) do              \
{                                                   \
    if (KmtIsCheckedBuild)                          \
        ok_eq_pointer(Value, (PVOID)0x0BADD0FF);    \
    else                                            \
        ok_eq_pointer(Value, Expected);             \
} while (0)

#define ok_eq_free2(Value, Expected) do              \
{                                                   \
    if (KmtIsCheckedBuild)                          \
        ok_eq_pointer(Value, (PVOID)(ULONG_PTR)0xBADDD0FFBADDD0FFULL);    \
    else                                            \
        ok_eq_pointer(Value, Expected);             \
} while (0)

START_TEST(ExDoubleList)
{
    KSPIN_LOCK SpinLock;
    LIST_ENTRY ListHead;
    PLIST_ENTRY Ret;

    KeInitializeSpinLock(&SpinLock);

    memset(&ListHead, 0x55, sizeof ListHead);
    InitializeListHead(&ListHead);
    ok_eq_pointer(ListHead.Flink, &ListHead);
    ok_eq_pointer(ListHead.Blink, &ListHead);
    ok_bool_true(KmtAreInterruptsEnabled(), "Interrupts enabled:");
    ok_irql(PASSIVE_LEVEL);

    Ret = ExInterlockedInsertHeadList(&ListHead, &Entries[0], &SpinLock);
    ok_eq_pointer(Ret, NULL);
    ok_eq_pointer(ListHead.Flink, &Entries[0]);
    ok_eq_pointer(ListHead.Blink, &Entries[0]);
    ok_eq_pointer(Entries[0].Flink, &ListHead);
    ok_eq_pointer(Entries[0].Blink, &ListHead);
    ok_bool_true(KmtAreInterruptsEnabled(), "Interrupts enabled:");
    ok_irql(PASSIVE_LEVEL);

    Ret = ExInterlockedRemoveHeadList(&ListHead, &SpinLock);
    ok_eq_pointer(Ret, &Entries[0]);
    ok_eq_pointer(ListHead.Flink, &ListHead);
    ok_eq_pointer(ListHead.Blink, &ListHead);
    ok_eq_free(Entries[0].Flink, &ListHead);
    ok_eq_free(Entries[0].Blink, &ListHead);
    ok_bool_true(KmtAreInterruptsEnabled(), "Interrupts enabled:");
    ok_irql(PASSIVE_LEVEL);

    Ret = ExInterlockedRemoveHeadList(&ListHead, &SpinLock);
    ok_eq_pointer(Ret, NULL);
    ok_eq_pointer(ListHead.Flink, &ListHead);
    ok_eq_pointer(ListHead.Blink, &ListHead);
    ok_eq_free(Entries[0].Flink, &ListHead);
    ok_eq_free(Entries[0].Blink, &ListHead);
    ok_bool_true(KmtAreInterruptsEnabled(), "Interrupts enabled:");
    ok_irql(PASSIVE_LEVEL);

    Ret = ExInterlockedInsertTailList(&ListHead, &Entries[0], &SpinLock);
    ok_eq_pointer(Ret, NULL);
    ok_eq_pointer(ListHead.Flink, &Entries[0]);
    ok_eq_pointer(ListHead.Blink, &Entries[0]);
    ok_eq_pointer(Entries[0].Flink, &ListHead);
    ok_eq_pointer(Entries[0].Blink, &ListHead);
    ok_bool_true(KmtAreInterruptsEnabled(), "Interrupts enabled:");
    ok_irql(PASSIVE_LEVEL);

    Ret = ExInterlockedRemoveHeadList(&ListHead, &SpinLock);
    ok_eq_pointer(Ret, &Entries[0]);
    ok_eq_pointer(ListHead.Flink, &ListHead);
    ok_eq_pointer(ListHead.Blink, &ListHead);
    ok_eq_free(Entries[0].Flink, &ListHead);
    ok_eq_free(Entries[0].Blink, &ListHead);
    ok_bool_true(KmtAreInterruptsEnabled(), "Interrupts enabled:");
    ok_irql(PASSIVE_LEVEL);

    Ret = ExInterlockedRemoveHeadList(&ListHead, &SpinLock);
    ok_eq_pointer(Ret, NULL);
    ok_eq_pointer(ListHead.Flink, &ListHead);
    ok_eq_pointer(ListHead.Blink, &ListHead);
    ok_eq_free(Entries[0].Flink, &ListHead);
    ok_eq_free(Entries[0].Blink, &ListHead);
    ok_bool_true(KmtAreInterruptsEnabled(), "Interrupts enabled:");
    ok_irql(PASSIVE_LEVEL);

    Ret = ExInterlockedInsertTailList(&ListHead, &Entries[0], &SpinLock);
    ok_eq_pointer(Ret, NULL);
    ok_eq_pointer(ListHead.Flink, &Entries[0]);
    ok_eq_pointer(ListHead.Blink, &Entries[0]);
    ok_eq_pointer(Entries[0].Flink, &ListHead);
    ok_eq_pointer(Entries[0].Blink, &ListHead);
    ok_bool_true(KmtAreInterruptsEnabled(), "Interrupts enabled:");
    ok_irql(PASSIVE_LEVEL);

    Ret = ExInterlockedInsertHeadList(&ListHead, &Entries[1], &SpinLock);
    ok_eq_pointer(Ret, &Entries[0]);
    ok_eq_pointer(ListHead.Flink, &Entries[1]);
    ok_eq_pointer(ListHead.Blink, &Entries[0]);
    ok_eq_pointer(Entries[0].Flink, &ListHead);
    ok_eq_pointer(Entries[0].Blink, &Entries[1]);
    ok_eq_pointer(Entries[1].Flink, &Entries[0]);
    ok_eq_pointer(Entries[1].Blink, &ListHead);
    ok_bool_true(KmtAreInterruptsEnabled(), "Interrupts enabled:");
    ok_irql(PASSIVE_LEVEL);

    Ret = ExInterlockedInsertTailList(&ListHead, &Entries[2], &SpinLock);
    ok_eq_pointer(Ret, &Entries[0]);
    ok_eq_pointer(ListHead.Flink, &Entries[1]);
    ok_eq_pointer(ListHead.Blink, &Entries[2]);
    ok_eq_pointer(Entries[0].Flink, &Entries[2]);
    ok_eq_pointer(Entries[0].Blink, &Entries[1]);
    ok_eq_pointer(Entries[1].Flink, &Entries[0]);
    ok_eq_pointer(Entries[1].Blink, &ListHead);
    ok_eq_pointer(Entries[2].Flink, &ListHead);
    ok_eq_pointer(Entries[2].Blink, &Entries[0]);
    ok_bool_true(KmtAreInterruptsEnabled(), "Interrupts enabled:");
    ok_irql(PASSIVE_LEVEL);

    memset(Entries, 0x55, sizeof Entries);
#undef ExInterlockedInsertHeadList
#undef ExInterlockedInsertTailList
#undef ExInterlockedRemoveHeadList

    memset(&ListHead, 0x55, sizeof ListHead);
    InitializeListHead(&ListHead);
    ok_eq_pointer(ListHead.Flink, &ListHead);
    ok_eq_pointer(ListHead.Blink, &ListHead);
    ok_bool_true(KmtAreInterruptsEnabled(), "Interrupts enabled:");
    ok_irql(PASSIVE_LEVEL);

    Ret = ExInterlockedInsertHeadList(&ListHead, &Entries[0], &SpinLock);
    ok_eq_pointer(Ret, NULL);
    ok_eq_pointer(ListHead.Flink, &Entries[0]);
    ok_eq_pointer(ListHead.Blink, &Entries[0]);
    ok_eq_pointer(Entries[0].Flink, &ListHead);
    ok_eq_pointer(Entries[0].Blink, &ListHead);
    ok_bool_true(KmtAreInterruptsEnabled(), "Interrupts enabled:");
    ok_irql(PASSIVE_LEVEL);

    Ret = ExInterlockedRemoveHeadList(&ListHead, &SpinLock);
    ok_eq_pointer(Ret, &Entries[0]);
    ok_eq_pointer(ListHead.Flink, &ListHead);
    ok_eq_pointer(ListHead.Blink, &ListHead);
    ok_eq_free2(Entries[0].Flink, &ListHead);
    ok_eq_free2(Entries[0].Blink, &ListHead);
    ok_bool_true(KmtAreInterruptsEnabled(), "Interrupts enabled:");
    ok_irql(PASSIVE_LEVEL);

    Ret = ExInterlockedRemoveHeadList(&ListHead, &SpinLock);
    ok_eq_pointer(Ret, NULL);
    ok_eq_pointer(ListHead.Flink, &ListHead);
    ok_eq_pointer(ListHead.Blink, &ListHead);
    ok_eq_free2(Entries[0].Flink, &ListHead);
    ok_eq_free2(Entries[0].Blink, &ListHead);
    ok_bool_true(KmtAreInterruptsEnabled(), "Interrupts enabled:");
    ok_irql(PASSIVE_LEVEL);

    Ret = ExInterlockedInsertTailList(&ListHead, &Entries[0], &SpinLock);
    ok_eq_pointer(Ret, NULL);
    ok_eq_pointer(ListHead.Flink, &Entries[0]);
    ok_eq_pointer(ListHead.Blink, &Entries[0]);
    ok_eq_pointer(Entries[0].Flink, &ListHead);
    ok_eq_pointer(Entries[0].Blink, &ListHead);
    ok_bool_true(KmtAreInterruptsEnabled(), "Interrupts enabled:");
    ok_irql(PASSIVE_LEVEL);

    Ret = ExInterlockedRemoveHeadList(&ListHead, &SpinLock);
    ok_eq_pointer(Ret, &Entries[0]);
    ok_eq_pointer(ListHead.Flink, &ListHead);
    ok_eq_pointer(ListHead.Blink, &ListHead);
    ok_eq_free2(Entries[0].Flink, &ListHead);
    ok_eq_free2(Entries[0].Blink, &ListHead);
    ok_bool_true(KmtAreInterruptsEnabled(), "Interrupts enabled:");
    ok_irql(PASSIVE_LEVEL);

    Ret = ExInterlockedRemoveHeadList(&ListHead, &SpinLock);
    ok_eq_pointer(Ret, NULL);
    ok_eq_pointer(ListHead.Flink, &ListHead);
    ok_eq_pointer(ListHead.Blink, &ListHead);
    ok_eq_free2(Entries[0].Flink, &ListHead);
    ok_eq_free2(Entries[0].Blink, &ListHead);
    ok_bool_true(KmtAreInterruptsEnabled(), "Interrupts enabled:");
    ok_irql(PASSIVE_LEVEL);

    Ret = ExInterlockedInsertTailList(&ListHead, &Entries[0], &SpinLock);
    ok_eq_pointer(Ret, NULL);
    ok_eq_pointer(ListHead.Flink, &Entries[0]);
    ok_eq_pointer(ListHead.Blink, &Entries[0]);
    ok_eq_pointer(Entries[0].Flink, &ListHead);
    ok_eq_pointer(Entries[0].Blink, &ListHead);
    ok_bool_true(KmtAreInterruptsEnabled(), "Interrupts enabled:");
    ok_irql(PASSIVE_LEVEL);

    Ret = ExInterlockedInsertHeadList(&ListHead, &Entries[1], &SpinLock);
    ok_eq_pointer(Ret, &Entries[0]);
    ok_eq_pointer(ListHead.Flink, &Entries[1]);
    ok_eq_pointer(ListHead.Blink, &Entries[0]);
    ok_eq_pointer(Entries[0].Flink, &ListHead);
    ok_eq_pointer(Entries[0].Blink, &Entries[1]);
    ok_eq_pointer(Entries[1].Flink, &Entries[0]);
    ok_eq_pointer(Entries[1].Blink, &ListHead);
    ok_bool_true(KmtAreInterruptsEnabled(), "Interrupts enabled:");
    ok_irql(PASSIVE_LEVEL);

    Ret = ExInterlockedInsertTailList(&ListHead, &Entries[2], &SpinLock);
    ok_eq_pointer(Ret, &Entries[0]);
    ok_eq_pointer(ListHead.Flink, &Entries[1]);
    ok_eq_pointer(ListHead.Blink, &Entries[2]);
    ok_eq_pointer(Entries[0].Flink, &Entries[2]);
    ok_eq_pointer(Entries[0].Blink, &Entries[1]);
    ok_eq_pointer(Entries[1].Flink, &Entries[0]);
    ok_eq_pointer(Entries[1].Blink, &ListHead);
    ok_eq_pointer(Entries[2].Flink, &ListHead);
    ok_eq_pointer(Entries[2].Blink, &Entries[0]);
    ok_bool_true(KmtAreInterruptsEnabled(), "Interrupts enabled:");
    ok_irql(PASSIVE_LEVEL);

    KmtSetIrql(PASSIVE_LEVEL);
}
