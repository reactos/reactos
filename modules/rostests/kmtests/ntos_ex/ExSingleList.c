/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Singly-linked list test
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

struct _SINGLE_LIST_ENTRY;
#ifdef _X86_
struct _SINGLE_LIST_ENTRY *__stdcall ExInterlockedPushEntryList(struct _SINGLE_LIST_ENTRY *, struct _SINGLE_LIST_ENTRY *, unsigned long *);
struct _SINGLE_LIST_ENTRY *__stdcall ExInterlockedPopEntryList(struct _SINGLE_LIST_ENTRY *, unsigned long *);
#endif

#include <kmt_test.h>

#define ok_eq_free2(Value, Expected) do              \
{                                                   \
    if (KmtIsCheckedBuild)                          \
        ok_eq_pointer(Value, (PVOID)(ULONG_PTR)0xBADDD0FFBADDD0FFULL);    \
    else                                            \
        ok_eq_pointer(Value, Expected);             \
} while (0)

PSINGLE_LIST_ENTRY FlushList(PSINGLE_LIST_ENTRY ListHead)
{
    PSINGLE_LIST_ENTRY Ret = ListHead->Next;
    ListHead->Next = NULL;
    return Ret;
}

USHORT QueryDepthList(PSINGLE_LIST_ENTRY ListHead)
{
    USHORT Depth = 0;
    while (ListHead->Next)
    {
        ++Depth;
        ListHead = ListHead->Next;
    }
    return Depth;
}

PSINGLE_LIST_ENTRY PushEntryListWrapper(PSINGLE_LIST_ENTRY ListHead, PSINGLE_LIST_ENTRY Entry, PKSPIN_LOCK Lock)
{
    PSINGLE_LIST_ENTRY Ret;
    UNREFERENCED_PARAMETER(Lock);
    Ret = ListHead->Next;
    PushEntryList(ListHead, Entry);
    return Ret;
}

#define CheckListHeader(ListHead, ExpectedPointer, ExpectedDepth) do    \
{                                                                       \
    ok_eq_pointer((ListHead)->Next, ExpectedPointer);                   \
    ok_eq_uint(QueryDepthList(ListHead), ExpectedDepth);                \
    ok_irql(HIGH_LEVEL);                                                \
    ok_bool_true(KmtAreInterruptsEnabled(), "Interrupts enabled:");     \
} while (0)

#define PXLIST_HEADER       PSINGLE_LIST_ENTRY
#define PXLIST_ENTRY        PSINGLE_LIST_ENTRY
#define PushXList           ExInterlockedPushEntryList
#define PopXList            ExInterlockedPopEntryList
#define FlushXList          FlushList
#define ok_free_xlist       ok_eq_free2
#define CheckXListHeader    CheckListHeader
#define TestXListFunctional TestListFunctional
#include "ExXList.h"

#undef ExInterlockedPushEntryList
#undef ExInterlockedPopEntryList
#define TestXListFunctional TestListFunctionalExports
#include "ExXList.h"

#undef  PushXList
#define PushXList           PushEntryListWrapper
#undef  PopXList
#define PopXList(h, s)      PopEntryList(h)
#undef  ok_free_xlist
#define ok_free_xlist       ok_eq_pointer
#define TestXListFunctional TestListFunctionalNoInterlocked
#include "ExXList.h"

START_TEST(ExSingleList)
{
    KSPIN_LOCK SpinLock;
    PSINGLE_LIST_ENTRY ListHead;
    PSINGLE_LIST_ENTRY Entries;
    SIZE_T EntriesSize = 5 * sizeof *Entries;
    PCHAR Buffer;
    KIRQL Irql;

    KeInitializeSpinLock(&SpinLock);

    /* make sure stuff is as un-aligned as possible ;) */
    Buffer = ExAllocatePoolWithTag(NonPagedPool, sizeof *ListHead + EntriesSize + 1, 'TLiS');
    ListHead = (PVOID)&Buffer[1];
    Entries = (PVOID)&ListHead[1];
    KeRaiseIrql(HIGH_LEVEL, &Irql);

    RtlFillMemory(Entries, sizeof(*Entries), 0x55);
    ListHead->Next = NULL;
    TestListFunctional(ListHead, Entries, &SpinLock);

    RtlFillMemory(Entries, sizeof(*Entries), 0x55);
    ListHead->Next = NULL;
    TestListFunctionalExports(ListHead, Entries, &SpinLock);

    RtlFillMemory(Entries, sizeof(*Entries), 0x55);
    ListHead->Next = NULL;
    TestListFunctionalNoInterlocked(ListHead, Entries, &SpinLock);

    KeLowerIrql(Irql);
    ExFreePoolWithTag(Buffer, 'TLiS');
}
