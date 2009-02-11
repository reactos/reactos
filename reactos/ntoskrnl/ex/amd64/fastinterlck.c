/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/ex/fastinterlck.c
 * PURPOSE:         Portable Ex*Interlocked and REGISTER routines for amd64
 * PROGRAMMERS:     ReactOS Portable Systems Group
 *                  Timo Kreuzer
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#undef ExInterlockedAddLargeInteger
#undef ExInterlockedAddUlong
#undef ExInterlockedExtendZone
#undef ExInterlockedInsertHeadList
#undef ExInterlockedInsertTailList
#undef ExInterlockedPopEntryList
#undef ExInterlockedPushEntryList
#undef ExInterlockedRemoveHeadList
#undef ExpInterlockedFlushSList
#undef ExpInterlockedPopEntrySList
#undef ExpInterlockedPushEntrySList

/* FUNCTIONS ******************************************************************/

LARGE_INTEGER
ExInterlockedAddLargeInteger(IN PLARGE_INTEGER Addend,
                             IN LARGE_INTEGER Increment,
                             IN PKSPIN_LOCK Lock)
{
    LARGE_INTEGER Int;
    Int.QuadPart = _InterlockedExchangeAdd64(&Addend->QuadPart,
                                             Increment.QuadPart);
    return Int;
}

ULONG
ExInterlockedAddUlong(IN PULONG Addend,
                      IN ULONG Increment,
                      PKSPIN_LOCK Lock)
{
    return (ULONG)_InterlockedExchangeAdd((PLONG)Addend, Increment);
}

PLIST_ENTRY
ExInterlockedInsertHeadList(IN PLIST_ENTRY ListHead,
                            IN PLIST_ENTRY ListEntry,
                            IN PKSPIN_LOCK Lock)
{
    KIRQL OldIrql;
    PLIST_ENTRY OldHead = NULL;
    KeAcquireSpinLock(Lock, &OldIrql);
    if (!IsListEmpty(ListHead)) OldHead = ListEntry->Flink;
    InsertHeadList(ListHead, ListEntry);
    KeReleaseSpinLock(Lock, OldIrql);
    return OldHead;
}

PLIST_ENTRY
ExInterlockedInsertTailList(IN PLIST_ENTRY ListHead,
                            IN PLIST_ENTRY ListEntry,
                            IN PKSPIN_LOCK Lock)
{
    KIRQL OldIrql;
    PLIST_ENTRY OldHead = NULL;
    KeAcquireSpinLock(Lock, &OldIrql);
    if (!IsListEmpty(ListHead)) OldHead = ListEntry->Blink;
    InsertTailList(ListHead, ListEntry);
    KeReleaseSpinLock(Lock, OldIrql);
    return OldHead;
}

PSINGLE_LIST_ENTRY
ExInterlockedPopEntryList(IN PSINGLE_LIST_ENTRY ListHead,
                          IN PKSPIN_LOCK Lock)
{
    KIRQL OldIrql;
    PSINGLE_LIST_ENTRY OldHead = NULL;
    KeAcquireSpinLock(Lock, &OldIrql);
    if (!ListHead->Next) OldHead = PopEntryList(ListHead);
    KeReleaseSpinLock(Lock, OldIrql);
    return OldHead;
}

PSINGLE_LIST_ENTRY
ExInterlockedPushEntryList(IN PSINGLE_LIST_ENTRY ListHead,
                           IN PSINGLE_LIST_ENTRY ListEntry,
                           IN PKSPIN_LOCK Lock)
{
    KIRQL OldIrql;
    PSINGLE_LIST_ENTRY OldHead = NULL;
    KeAcquireSpinLock(Lock, &OldIrql);
    if (!ListHead->Next) OldHead = PushEntryList(ListHead, ListEntry);
    KeReleaseSpinLock(Lock, OldIrql);
    return OldHead;
}

PLIST_ENTRY
ExInterlockedRemoveHeadList(IN PLIST_ENTRY ListHead,
                            IN PKSPIN_LOCK Lock)
{
    KIRQL OldIrql;
    PLIST_ENTRY OldHead = NULL;
    KeAcquireSpinLock(Lock, &OldIrql);
    if (!IsListEmpty(ListHead)) OldHead = RemoveHeadList(ListHead);
    KeReleaseSpinLock(Lock, OldIrql);
    return OldHead;
}

PSLIST_ENTRY
ExpInterlockedFlushSList(
    PSLIST_HEADER ListHead)
{
    UNIMPLEMENTED;
    return NULL;
}

PSLIST_ENTRY
ExpInterlockedPopEntrySList(
    IN PSLIST_HEADER ListHead)
{
    UNIMPLEMENTED;
    return NULL;
}

PSLIST_ENTRY
ExpInterlockedPushEntrySList(
    PSLIST_HEADER ListHead,
    PSLIST_ENTRY ListEntry)
{
    UNIMPLEMENTED;
    return NULL;
}
