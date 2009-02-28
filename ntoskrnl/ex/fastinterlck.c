/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/ex/fastinterlck.c
 * PURPOSE:         Portable Ex*Interlocked and REGISTER routines for non-x86
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#if defined(_ARM_) || defined(_PPC_) || defined(NTOS_USE_GENERICS)

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#undef ExInterlockedPushEntrySList
#undef ExInterlockedPopEntrySList
#undef ExInterlockedAddULong
#undef ExInterlockedIncrementLong
#undef ExInterlockedDecrementLong

/* FUNCTIONS ******************************************************************/

PSLIST_ENTRY
NTAPI
InterlockedPushEntrySList(IN PSLIST_HEADER ListHead,
                          IN PSLIST_ENTRY ListEntry)
{
    
    PSINGLE_LIST_ENTRY FirstEntry, NextEntry;
    PSINGLE_LIST_ENTRY Entry = (PVOID)ListEntry, Head = (PVOID)ListHead;
    
    FirstEntry = Head->Next;
    do
    {
        Entry->Next = FirstEntry;
        NextEntry = FirstEntry;
        FirstEntry = (PVOID)_InterlockedCompareExchange((PLONG)Head,
                                                        (LONG)Entry,
                                                        (LONG)FirstEntry);
    } while (FirstEntry != NextEntry);
    
    return FirstEntry;
}

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
        FirstEntry = (PVOID)_InterlockedCompareExchange((PLONG)Head,
                                                        (LONG)FirstEntry->Next,
                                                        (LONG)FirstEntry);
    } while (FirstEntry != NextEntry);

    return FirstEntry;    
}

PSINGLE_LIST_ENTRY
FASTCALL
ExInterlockedFlushSList(IN PSLIST_HEADER ListHead)
{
    return (PVOID)_InterlockedExchange((PLONG)&ListHead->Next.Next, (LONG)NULL);
}

PSLIST_ENTRY
FASTCALL
ExInterlockedPushEntrySList(IN PSLIST_HEADER ListHead,
                            IN PSLIST_ENTRY ListEntry,
                            IN PKSPIN_LOCK Lock)
{
    return InterlockedPushEntrySList(ListHead, ListEntry);
}

PSINGLE_LIST_ENTRY
NTAPI
ExInterlockedPopEntrySList(IN PSLIST_HEADER ListHead,
                           IN PKSPIN_LOCK Lock)
{
    return InterlockedPopEntrySList(ListHead);
}

ULONG
FASTCALL
ExfInterlockedAddUlong(IN PULONG Addend,
                       IN ULONG Increment,
                       PKSPIN_LOCK Lock)
{
    KIRQL OldIrql;
    KeAcquireSpinLock(Lock, &OldIrql);
    *Addend += Increment;
    KeReleaseSpinLock(Lock, OldIrql);
    return *Addend;
}

LONGLONG
FASTCALL
ExfInterlockedCompareExchange64(IN OUT LONGLONG volatile *Destination,
                                IN PLONGLONG Exchange,
                                IN PLONGLONG Comparand)
{
    LONGLONG Result;
    
    Result = *Destination;
    if (*Destination == *Comparand) *Destination = *Exchange;
    return Result;
}

PLIST_ENTRY
FASTCALL
ExfInterlockedInsertHeadList(IN PLIST_ENTRY ListHead,
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
FASTCALL
ExfInterlockedInsertTailList(IN PLIST_ENTRY ListHead,
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
FASTCALL
ExfInterlockedPopEntryList(IN PSINGLE_LIST_ENTRY ListHead,
                           IN PKSPIN_LOCK Lock)
{
    UNIMPLEMENTED;
    return NULL;
}

PSINGLE_LIST_ENTRY
FASTCALL
ExfInterlockedPushEntryList(IN PSINGLE_LIST_ENTRY ListHead,
                            IN PSINGLE_LIST_ENTRY ListEntry,
                            IN PKSPIN_LOCK Lock)
{
    UNIMPLEMENTED;
    return NULL;
}

PLIST_ENTRY
FASTCALL
ExfInterlockedRemoveHeadList(IN PLIST_ENTRY ListHead,
                             IN PKSPIN_LOCK Lock)
{
    return ExInterlockedRemoveHeadList(ListHead, Lock);
}

LARGE_INTEGER
NTAPI
ExInterlockedAddLargeInteger(IN PLARGE_INTEGER Addend,
                             IN LARGE_INTEGER Increment,
                             IN PKSPIN_LOCK Lock)
{
    LARGE_INTEGER Integer = {{0}};
    UNIMPLEMENTED;
    return Integer;
}

ULONG
NTAPI
ExInterlockedAddUlong(IN PULONG Addend,
                      IN ULONG Increment,
                      PKSPIN_LOCK Lock)
{
    return (ULONG)_InterlockedExchangeAdd((PLONG)Addend, Increment);
}

INTERLOCKED_RESULT
NTAPI
ExInterlockedIncrementLong(IN PLONG Addend,
                           IN PKSPIN_LOCK Lock)
{
    return _InterlockedIncrement(Addend);
}

INTERLOCKED_RESULT
NTAPI
ExInterlockedDecrementLong(IN PLONG Addend,
                           IN PKSPIN_LOCK Lock)
{
    return _InterlockedDecrement(Addend);
}

ULONG
NTAPI
ExInterlockedExchangeUlong(IN PULONG Target,
                           IN ULONG Value,
                           IN PKSPIN_LOCK Lock)
{
    return (ULONG)_InterlockedExchange((PLONG)Target, Value);
}

PLIST_ENTRY
NTAPI
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
NTAPI
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
NTAPI
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
NTAPI
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
NTAPI
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

VOID
FASTCALL
ExInterlockedAddLargeStatistic(IN PLARGE_INTEGER Addend,
                               IN ULONG Increment)
{
    UNIMPLEMENTED;
}

LONGLONG
FASTCALL
ExInterlockedCompareExchange64(IN OUT PLONGLONG Destination,
                               IN PLONGLONG Exchange,
                               IN PLONGLONG Comparand,
                               IN PKSPIN_LOCK Lock)
{
    KIRQL OldIrql;
    LONGLONG Result;
    
    KeAcquireSpinLock(Lock, &OldIrql);
    Result = *Destination;
    if (*Destination == *Comparand) *Destination = *Exchange;
    KeReleaseSpinLock(Lock, OldIrql);
    return Result;
}

VOID
NTAPI
READ_REGISTER_BUFFER_UCHAR(IN PUCHAR Register,
                           IN PUCHAR Buffer,
                           IN ULONG Count)
{
    PUCHAR registerBuffer = Register;
    PUCHAR readBuffer = Buffer;
    ULONG readCount;
    
    for (readCount = Count; readCount--; readBuffer++, registerBuffer++)
    {
        *readBuffer = *(volatile UCHAR * const)registerBuffer;
    }
}

VOID
NTAPI
READ_REGISTER_BUFFER_ULONG(IN PULONG Register,
                           IN PULONG Buffer,
                           IN ULONG Count)
{
    PULONG registerBuffer = Register;
    PULONG readBuffer = Buffer;
    ULONG readCount;
    
    for (readCount = Count; readCount--; readBuffer++, registerBuffer++)
    {
        *readBuffer = *(volatile ULONG * const)registerBuffer;
    }
}

VOID
NTAPI
READ_REGISTER_BUFFER_USHORT(IN PUSHORT Register,
                            IN PUSHORT Buffer,
                            IN ULONG Count)
{
    PUSHORT registerBuffer = Register;
    PUSHORT readBuffer = Buffer;
    ULONG readCount;
    
    for (readCount = Count; readCount--; readBuffer++, registerBuffer++)
    {
        *readBuffer = *(volatile USHORT * const)registerBuffer;
    }
}

UCHAR
NTAPI
READ_REGISTER_UCHAR(IN PUCHAR Register)
{
    return *(volatile UCHAR * const)Register;
}

ULONG
NTAPI
READ_REGISTER_ULONG(IN PULONG Register)
{
    return *(volatile ULONG * const)Register;
}

USHORT
NTAPI
READ_REGISTER_USHORT(IN PUSHORT Register)
{
    return *(volatile USHORT * const)Register;  
}

VOID
NTAPI
WRITE_REGISTER_BUFFER_UCHAR(IN PUCHAR Register,
                            IN PUCHAR Buffer,
                            IN ULONG Count)
{
    PUCHAR registerBuffer = Register;
    PUCHAR writeBuffer = Buffer;
    ULONG writeCount;
    for (writeCount = Count; writeCount--; writeBuffer++, registerBuffer++)
    {
        *(volatile UCHAR * const)registerBuffer = *writeBuffer;
    }
    KeFlushWriteBuffer();
}

VOID
NTAPI
WRITE_REGISTER_BUFFER_ULONG(IN PULONG Register,
                            IN PULONG Buffer,
                            IN ULONG Count)
{
    PULONG registerBuffer = Register;
    PULONG writeBuffer = Buffer;
    ULONG writeCount;
    for (writeCount = Count; writeCount--; writeBuffer++, registerBuffer++)
    {
        *(volatile ULONG * const)registerBuffer = *writeBuffer;
    }
    KeFlushWriteBuffer();
}

VOID
NTAPI
WRITE_REGISTER_BUFFER_USHORT(IN PUSHORT Register,
                             IN PUSHORT Buffer,
                             IN ULONG Count)
{
    PUSHORT registerBuffer = Register;
    PUSHORT writeBuffer = Buffer;
    ULONG writeCount;
    for (writeCount = Count; writeCount--; writeBuffer++, registerBuffer++)
    {
        *(volatile USHORT * const)registerBuffer = *writeBuffer;
    }
    KeFlushWriteBuffer();
}

VOID
NTAPI
WRITE_REGISTER_UCHAR(IN PUCHAR Register,
                     IN UCHAR Value)
{
    *(volatile UCHAR * const)Register = Value;
    KeFlushWriteBuffer();   
}

VOID
NTAPI
WRITE_REGISTER_ULONG(IN PULONG Register,
                     IN ULONG Value)
{
    *(volatile ULONG * const)Register = Value;
    KeFlushWriteBuffer();  
}

VOID
NTAPI
WRITE_REGISTER_USHORT(IN PUSHORT Register,
                      IN USHORT Value)
{
    *(volatile USHORT * const)Register = Value;
    KeFlushWriteBuffer();  
}

#endif
