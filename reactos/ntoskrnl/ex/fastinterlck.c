/*
 * PROJECT:         ReactOS Kernel
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ex/powerpc/fastinterlck.c
 * PURPOSE:         Executive Atom Functions
 * PROGRAMMERS:     Art Yerkes
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <internal/debug.h>

NTKERNELAPI
PSLIST_ENTRY
NTAPI
InterlockedPushEntrySList(IN PSLIST_HEADER ListHead,
                          IN PSLIST_ENTRY ListEntry)
{
    
    PSINGLE_LIST_ENTRY FirstEntry, NextEntry, Entry = (PVOID)ListEntry, Head = (PVOID)ListHead;
    
    FirstEntry = Head->Next;
    do
    {
        Entry->Next = FirstEntry;
        NextEntry = FirstEntry;
        FirstEntry = (PVOID)_InterlockedCompareExchange((PLONG)Head, (LONG)Entry, (LONG)FirstEntry);
    } while (FirstEntry != NextEntry);
    
    return FirstEntry;
}

NTKERNELAPI
PSLIST_ENTRY
NTAPI
InterlockedPopEntrySList(IN PSLIST_HEADER ListHead)
{
    PSINGLE_LIST_ENTRY FirstEntry, NextEntry, Head = (PVOID)ListHead;
    
    FirstEntry = Head->Next;
    do
    {
        if (!FirstEntry) return NULL;

        NextEntry = FirstEntry;
        FirstEntry = (PVOID)_InterlockedCompareExchange((PLONG)Head, (LONG)FirstEntry->Next, (LONG)FirstEntry);
    } while (FirstEntry != NextEntry);

    return FirstEntry;    
}

NTKERNELAPI
PSINGLE_LIST_ENTRY
FASTCALL
ExInterlockedFlushSList(IN PSLIST_HEADER ListHead)
{
    return (PVOID)_InterlockedExchange((PLONG)&ListHead->Next.Next, (LONG)NULL);
}

#undef ExInterlockedPushEntrySList
NTKERNELAPI
PSLIST_ENTRY
FASTCALL
ExInterlockedPushEntrySList
(IN PSLIST_HEADER  ListHead,
 IN PSLIST_ENTRY  ListEntry)
{
    return InterlockedPushEntrySList(ListHead, ListEntry);
}

#undef ExInterlockedPopEntrySList
NTKERNELAPI
PSINGLE_LIST_ENTRY
NTAPI
ExInterlockedPopEntrySList(
  IN PSLIST_HEADER  ListHead,
  IN PKSPIN_LOCK  Lock)
{
    return InterlockedPopEntrySList(ListHead);
}

#undef ExInterlockedAddULong
NTKERNELAPI
ULONG
NTAPI
ExfInterlockedAddUlong(
  IN PULONG  Addend,
  IN ULONG  Increment,
  PKSPIN_LOCK  Lock)
{
    KIRQL OldIrql;
    KeAcquireSpinLock(Lock, &OldIrql);
    *Addend += Increment;
    KeReleaseSpinLock(Lock, OldIrql);
    return *Addend;
}

NTKERNELAPI
LONGLONG
FASTCALL
ExfInterlockedCompareExchange64(
  IN OUT LONGLONG volatile  *Destination,
  IN PLONGLONG  Exchange,
  IN PLONGLONG  Comperand)
{
    LONGLONG Result;
    
    Result = *Destination;
    if (*Destination == Result) *Destination = *Exchange;
    return Result;
}

NTKERNELAPI
PLIST_ENTRY
FASTCALL
ExfInterlockedInsertHeadList(
  IN PLIST_ENTRY  ListHead,
  IN PLIST_ENTRY  ListEntry,
  IN PKSPIN_LOCK  Lock)
{
    KIRQL OldIrql;
    PLIST_ENTRY OldHead = NULL;
    KeAcquireSpinLock(Lock, &OldIrql);
    if (!IsListEmpty(ListHead)) OldHead = ListEntry->Flink;
    InsertHeadList(ListHead, ListEntry);
    KeReleaseSpinLock(Lock, OldIrql);
    return OldHead;
}

NTKERNELAPI
PLIST_ENTRY
FASTCALL
ExfInterlockedInsertTailList(
  IN PLIST_ENTRY  ListHead,
  IN PLIST_ENTRY  ListEntry,
  IN PKSPIN_LOCK  Lock)
{
    KIRQL OldIrql;
    PLIST_ENTRY OldHead = NULL;
    KeAcquireSpinLock(Lock,&OldIrql);
    if (!IsListEmpty(ListHead)) OldHead = ListEntry->Blink;
    InsertTailList(ListHead, ListEntry);
    KeReleaseSpinLock(Lock, OldIrql);
    return OldHead;
}

NTKERNELAPI
PSINGLE_LIST_ENTRY
FASTCALL
ExfInterlockedPopEntryList(
  IN PSINGLE_LIST_ENTRY  ListHead,
  IN PKSPIN_LOCK  Lock)
{
    return NULL;
}

NTKERNELAPI
PSINGLE_LIST_ENTRY
FASTCALL
ExfInterlockedPushEntryList(
  IN PSINGLE_LIST_ENTRY  ListHead,
  IN PSINGLE_LIST_ENTRY  ListEntry,
  IN PKSPIN_LOCK  Lock)
{
    return NULL;
}

NTKERNELAPI
PLIST_ENTRY
FASTCALL
ExfInterlockedRemoveHeadList(
  IN PLIST_ENTRY  ListHead,
  IN PKSPIN_LOCK  Lock)
{
    return ExInterlockedRemoveHeadList(ListHead, Lock);
}

NTKERNELAPI
INTERLOCKED_RESULT
FASTCALL
Exfi386InterlockedIncrementLong(
  IN PLONG  Addend)
{
    return InterlockedIncrement(Addend);
}

NTKERNELAPI
INTERLOCKED_RESULT
FASTCALL
Exfi386InterlockedDecrementLong(
  IN PLONG  Addend)
{
    return InterlockedDecrement(Addend);
}

NTKERNELAPI
ULONG
FASTCALL
Exfi386InterlockedExchangeUlong(
  IN PULONG  Target,
  IN ULONG  Value)
{
    return (ULONG)_InterlockedExchange((PLONG)Target, Value);
}

NTKERNELAPI
LARGE_INTEGER
NTAPI
ExInterlockedAddLargeInteger(
  IN PLARGE_INTEGER  Addend,
  IN LARGE_INTEGER  Increment,
  IN PKSPIN_LOCK  Lock)
{
    LARGE_INTEGER Integer = {{0}};
    UNIMPLEMENTED;
    return Integer;
}

NTKERNELAPI
ULONG
NTAPI
ExInterlockedAddUlong(
  IN PULONG  Addend,
  IN ULONG  Increment,
  PKSPIN_LOCK  Lock)
{
    return (ULONG)_InterlockedExchangeAdd((PLONG)Addend, Increment);
}

#undef ExInterlockedIncrementLong
NTKERNELAPI
INTERLOCKED_RESULT
NTAPI
ExInterlockedIncrementLong(
    IN PLONG  Addend,
    IN PKSPIN_LOCK Lock)
{
    return _InterlockedIncrement(Addend);
}

#undef ExInterlockedDecrementLong
NTKERNELAPI
INTERLOCKED_RESULT
NTAPI
ExInterlockedDecrementLong(
    IN PLONG  Addend,
    IN PKSPIN_LOCK Lock)
{
    return _InterlockedDecrement(Addend);
}

NTKERNELAPI
ULONG
NTAPI
ExInterlockedExchangeUlong(
  IN PULONG  Target,
  IN ULONG  Value,
  IN PKSPIN_LOCK Lock)
{
    return (ULONG)_InterlockedExchange((PLONG)Target, Value);
}

NTKERNELAPI
PLIST_ENTRY
NTAPI
ExInterlockedInsertHeadList(
  IN PLIST_ENTRY  ListHead,
  IN PLIST_ENTRY  ListEntry,
  IN PKSPIN_LOCK  Lock)
{
    KIRQL OldIrql;
    PLIST_ENTRY OldHead = NULL;
    KeAcquireSpinLock(Lock,&OldIrql);
    if (!IsListEmpty(ListHead)) OldHead = ListEntry->Flink;
    InsertHeadList(ListHead, ListEntry);
    KeReleaseSpinLock(Lock, OldIrql);
    return OldHead;
}

NTKERNELAPI
PLIST_ENTRY
NTAPI
ExInterlockedInsertTailList(
  IN PLIST_ENTRY  ListHead,
  IN PLIST_ENTRY  ListEntry,
  IN PKSPIN_LOCK  Lock)
{
    KIRQL OldIrql;
    PLIST_ENTRY OldHead = NULL;
    KeAcquireSpinLock(Lock,&OldIrql);
    if (!IsListEmpty(ListHead)) OldHead = ListEntry->Blink;
    InsertTailList(ListHead, ListEntry);
    KeReleaseSpinLock(Lock, OldIrql);
    return OldHead;
}

NTKERNELAPI
PSINGLE_LIST_ENTRY
NTAPI
ExInterlockedPopEntryList(
  IN PSINGLE_LIST_ENTRY  ListHead,
  IN PKSPIN_LOCK  Lock)
{
    KIRQL OldIrql;
    PSINGLE_LIST_ENTRY OldHead = NULL;
    KeAcquireSpinLock(Lock,&OldIrql);
    OldHead = PopEntryList(ListHead);
    KeReleaseSpinLock(Lock, OldIrql);
    return OldHead;
}

NTKERNELAPI
PSINGLE_LIST_ENTRY
NTAPI
ExInterlockedPushEntryList(
  IN PSINGLE_LIST_ENTRY  ListHead,
  IN PSINGLE_LIST_ENTRY  ListEntry,
  IN PKSPIN_LOCK  Lock)
{
    KIRQL OldIrql;
    PSINGLE_LIST_ENTRY OldHead = NULL;
    KeAcquireSpinLock(Lock,&OldIrql);
    OldHead = PushEntryList(ListHead, ListEntry);
    KeReleaseSpinLock(Lock, OldIrql);
    return OldHead;
}

NTKERNELAPI
PLIST_ENTRY
NTAPI
ExInterlockedRemoveHeadList(
  IN PLIST_ENTRY  ListHead,
  IN PKSPIN_LOCK  Lock)
{
    KIRQL OldIrql;
    PLIST_ENTRY OldHead = NULL;
    KeAcquireSpinLock(Lock,&OldIrql);
    OldHead = RemoveHeadList(ListHead);
    KeReleaseSpinLock(Lock, OldIrql);
    return OldHead;
}

NTKERNELAPI
VOID
FASTCALL
ExInterlockedAddLargeStatistic
(IN PLARGE_INTEGER  Addend,
 IN ULONG  Increment)
{
    UNIMPLEMENTED;
}

NTKERNELAPI
LONGLONG
FASTCALL
ExInterlockedCompareExchange64(IN OUT PLONGLONG  Destination,
                               IN PLONGLONG  Exchange,
                               IN PLONGLONG  Comparand,
                               IN PKSPIN_LOCK  Lock)
{
    KIRQL OldIrql;
    LONGLONG Result;
    
    KeAcquireSpinLock(Lock, &OldIrql);
    Result = *Destination;
    if (*Destination == Result) *Destination = *Exchange;
    KeReleaseSpinLock(Lock, OldIrql);
    return Result;
}
