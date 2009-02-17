typedef unsigned int size_t;
#include <ntddk.h>
#include <winddk.h>
#include <string.h>
#include <intrin.h>

NTKERNELAPI
LONG
FASTCALL
InterlockedExchange(
    LONG volatile *Target, LONG Value)
{
    return _InterlockedExchange(Target, Value);
}

NTKERNELAPI
LONG
FASTCALL
InterlockedExchangeAdd(
    LONG volatile *Target, LONG Value)
{
    return _InterlockedExchangeAdd(Target, Value);
}

NTKERNELAPI
LONG
WINAPI
InterlockedCompareExchange(
    LONG volatile *Destination,
    LONG Exchange, LONG Comparand)
{
    return _InterlockedCompareExchange(Destination, Exchange, Comparand);
}

NTKERNELAPI
LONG
FASTCALL
InterlockedIncrement
(IN OUT LONG volatile *Addend)
{
    return _InterlockedIncrement(Addend);
}

NTKERNELAPI
LONG
FASTCALL
InterlockedDecrement(
    IN OUT LONG volatile *Addend)
{
    return _InterlockedDecrement(Addend);
}

PSLIST_ENTRY
WINAPI 
InterlockedPopEntrySList(
    PSLIST_HEADER ListHead)
{
    PSLIST_ENTRY Result = NULL;
    KIRQL OldIrql;
    static BOOLEAN GLLInit = FALSE;
    static KSPIN_LOCK GlobalListLock;

    if(!GLLInit)
    {
        KeInitializeSpinLock(&GlobalListLock);
        GLLInit = TRUE;
    }

    KeAcquireSpinLock(&GlobalListLock, &OldIrql);
    if(ListHead->Next.Next)
    {
        Result = ListHead->Next.Next;
        ListHead->Next.Next = Result->Next;
    }
    KeReleaseSpinLock(&GlobalListLock, OldIrql);
    return Result;
}

NTKERNELAPI
PSLIST_ENTRY
FASTCALL
InterlockedPushEntrySList(
    IN PSLIST_HEADER ListHead,
    IN PSLIST_ENTRY ListEntry)
{
    PVOID PrevValue;

    do
    {
        PrevValue = ListHead->Next.Next;
        ListEntry->Next = PrevValue;
    }
    while (InterlockedCompareExchangePointer(&ListHead->Next.Next,
                                             ListEntry,
                                             PrevValue) != PrevValue);

    return (PSLIST_ENTRY)PrevValue;
}

NTKERNELAPI
VOID
FASTCALL
ExInterlockedAddLargeStatistic(
    IN PLARGE_INTEGER Addend,
    IN ULONG Increment)
{
    _InterlockedAddLargeStatistic(&Addend->QuadPart, Increment);
}

NTKERNELAPI
LONGLONG
FASTCALL
ExInterlockedCompareExchange64(
    IN OUT PLONGLONG  Destination,
    IN PLONGLONG  Exchange,
    IN PLONGLONG  Comparand,
    IN PKSPIN_LOCK  Lock)
{
    KIRQL OldIrql;
    LONGLONG Result;

    KeAcquireSpinLock(Lock, &OldIrql);
    Result = *Destination;
    if(*Destination == Result)
    *Destination = *Exchange;
    KeReleaseSpinLock(Lock, OldIrql);
    return Result;
}
