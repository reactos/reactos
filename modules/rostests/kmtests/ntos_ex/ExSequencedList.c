/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite sequenced singly-linked list test
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

struct _SINGLE_LIST_ENTRY;
union _SLIST_HEADER;
struct _SINGLE_LIST_ENTRY *__fastcall ExInterlockedPushEntrySList(union _SLIST_HEADER *, struct _SINGLE_LIST_ENTRY *, unsigned long *);
struct _SINGLE_LIST_ENTRY *__fastcall ExInterlockedPopEntrySList(union _SLIST_HEADER *, unsigned long *);

#include <kmt_test.h>

/* TODO: SLIST_HEADER is a lot different for x64 */
#ifndef _M_AMD64
#define CheckSListHeader(ListHead, ExpectedPointer, ExpectedDepth) do   \
{                                                                       \
    ok_eq_pointer((ListHead)->Next.Next, ExpectedPointer);              \
    /*ok_eq_pointer(FirstEntrySList(ListHead), ExpectedPointer);*/      \
    ok_eq_uint((ListHead)->Depth, ExpectedDepth);                       \
    ok_eq_uint((ListHead)->Sequence, ExpectedSequence);                 \
    ok_eq_uint(ExQueryDepthSList(ListHead), ExpectedDepth);             \
    ok_irql(HIGH_LEVEL);                                                \
    ok_bool_true(KmtAreInterruptsEnabled(), "Interrupts enabled:");     \
} while (0)

#define PXLIST_HEADER       PSLIST_HEADER
#define PXLIST_ENTRY        PSLIST_ENTRY
#define PushXList           ExInterlockedPushEntrySList
#define PopXList            ExInterlockedPopEntrySList
#define FlushXList          ExInterlockedFlushSList
#define ok_free_xlist       ok_eq_pointer
#define CheckXListHeader    CheckSListHeader
#define TestXListFunctional TestSListFunctional
#include "ExXList.h"

#undef ExInterlockedPushEntrySList
#undef ExInterlockedPopEntrySList
#define TestXListFunctional TestSListFunctionalExports
#include "ExXList.h"
#endif

START_TEST(ExSequencedList)
{
#ifndef _M_AMD64
    PSLIST_HEADER ListHead;
    KSPIN_LOCK SpinLock;
    USHORT ExpectedSequence = 0;
    PKSPIN_LOCK pSpinLock = &SpinLock;
    PCHAR Buffer;
    PSLIST_ENTRY Entries;
    SIZE_T EntriesSize = 5 * sizeof *Entries;
    KIRQL Irql;

    KeInitializeSpinLock(&SpinLock);
#ifdef _M_IX86
    pSpinLock = NULL;
#endif

    /* make sure stuff is as un-aligned as possible ;) */
    Buffer = ExAllocatePoolWithTag(NonPagedPool, sizeof *ListHead + EntriesSize + 1, 'TLqS');
    ListHead = (PVOID)&Buffer[1];
    Entries = (PVOID)&ListHead[1];
    KeRaiseIrql(HIGH_LEVEL, &Irql);

    RtlFillMemory(Entries, EntriesSize, 0x55);
    RtlFillMemory(ListHead, sizeof *ListHead, 0x55);
    InitializeSListHead(ListHead);
    CheckSListHeader(ListHead, NULL, 0);
    TestSListFunctional(ListHead, Entries, pSpinLock);

    RtlFillMemory(Entries, EntriesSize, 0x55);
    RtlFillMemory(ListHead, sizeof *ListHead, 0x55);
    ExInitializeSListHead(ListHead);
    CheckSListHeader(ListHead, NULL, 0);
    TestSListFunctionalExports(ListHead, Entries, pSpinLock);

    KeLowerIrql(Irql);
    ExFreePoolWithTag(Buffer, 'TLqS');
#endif
}
