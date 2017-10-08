/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/ex/interlocked.c
* PURPOSE:         Interlocked functions
* PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@reactos.org)
*/

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <debug.h>

#undef ExInterlockedAddUlong
#undef ExInterlockedInsertHeadList
#undef ExInterlockedInsertTailList
#undef ExInterlockedRemoveHeadList
#undef ExInterlockedPopEntryList
#undef ExInterlockedPushEntryList
#undef ExInterlockedIncrementLong
#undef ExInterlockedDecrementLong
#undef ExInterlockedExchangeUlong
#undef ExInterlockedCompareExchange64


/* FUNCTIONS ****************************************************************/

FORCEINLINE
BOOLEAN
_ExiDisableInterruptsAndAcquireSpinlock(
    IN OUT PKSPIN_LOCK Lock)
{
    BOOLEAN Enabled;

    /* Disable interrupts */
    Enabled = KeDisableInterrupts();

    /* Acquire the spinlock (inline) */
    KxAcquireSpinLock(Lock);

    return Enabled;
}

FORCEINLINE
VOID
_ExiReleaseSpinLockAndRestoreInterrupts(
    IN OUT PKSPIN_LOCK Lock,
    BOOLEAN Enable)
{
    /* Release the spinlock */
    KxReleaseSpinLock(Lock);

    /* Restore interrupts */
    KeRestoreInterrupts(Enable);
}


LARGE_INTEGER
NTAPI
ExInterlockedAddLargeInteger(
    IN OUT PLARGE_INTEGER Addend,
    IN LARGE_INTEGER Increment,
    IN OUT PKSPIN_LOCK Lock)
{
    LARGE_INTEGER OldValue;
    BOOLEAN Enable;

    /* Disable interrupts and acquire the spinlock */
    Enable = _ExiDisableInterruptsAndAcquireSpinlock(Lock);

    /* Save the old value */
    OldValue.QuadPart = Addend->QuadPart;

    /* Do the operation */
    Addend->QuadPart += Increment.QuadPart;

    /* Release the spinlock and restore interrupts */
    _ExiReleaseSpinLockAndRestoreInterrupts(Lock, Enable);

    /* Return the old value */
    return OldValue;
}

ULONG
NTAPI
ExInterlockedAddUlong(
    IN OUT PULONG Addend,
    IN ULONG Increment,
    IN OUT PKSPIN_LOCK Lock)
{
    BOOLEAN Enable;
    ULONG OldValue;

    /* Disable interrupts and acquire the spinlock */
    Enable = _ExiDisableInterruptsAndAcquireSpinlock(Lock);

    /* Save the old value */
    OldValue = *Addend;

    /* Do the operation */
    *Addend += Increment;

    /* Release the spinlock and restore interrupts */
    _ExiReleaseSpinLockAndRestoreInterrupts(Lock, Enable);

    /* Return the old value */
    return OldValue;
}

PLIST_ENTRY
NTAPI
ExInterlockedInsertHeadList(
    IN OUT PLIST_ENTRY ListHead,
    IN OUT PLIST_ENTRY ListEntry,
    IN OUT PKSPIN_LOCK Lock)
{
    BOOLEAN Enable;
    PLIST_ENTRY FirstEntry;

    /* Disable interrupts and acquire the spinlock */
    Enable = _ExiDisableInterruptsAndAcquireSpinlock(Lock);

    /* Save the first entry */
    FirstEntry = ListHead->Flink;

    /* Insert the new entry */
    InsertHeadList(ListHead, ListEntry);

    /* Release the spinlock and restore interrupts */
    _ExiReleaseSpinLockAndRestoreInterrupts(Lock, Enable);

    /* Return the old first entry or NULL for empty list */
    return (FirstEntry == ListHead) ? NULL : FirstEntry;
}

PLIST_ENTRY
NTAPI
ExInterlockedInsertTailList(
    IN OUT PLIST_ENTRY ListHead,
    IN OUT PLIST_ENTRY ListEntry,
    IN OUT PKSPIN_LOCK Lock)
{
    BOOLEAN Enable;
    PLIST_ENTRY LastEntry;

    /* Disable interrupts and acquire the spinlock */
    Enable = _ExiDisableInterruptsAndAcquireSpinlock(Lock);

    /* Save the last entry */
    LastEntry = ListHead->Blink;

    /* Insert the new entry */
    InsertTailList(ListHead, ListEntry);

    /* Release the spinlock and restore interrupts */
    _ExiReleaseSpinLockAndRestoreInterrupts(Lock, Enable);

    /* Return the old last entry or NULL for empty list */
    return (LastEntry == ListHead) ? NULL : LastEntry;
}

PLIST_ENTRY
NTAPI
ExInterlockedRemoveHeadList(
    IN OUT PLIST_ENTRY ListHead,
    IN OUT PKSPIN_LOCK Lock)
{
    BOOLEAN Enable;
    PLIST_ENTRY ListEntry;

    /* Disable interrupts and acquire the spinlock */
    Enable = _ExiDisableInterruptsAndAcquireSpinlock(Lock);

    /* Check if the list is empty */
    if (IsListEmpty(ListHead))
    {
        /* Return NULL */
        ListEntry = NULL;
    }
    else
    {
        /* Remove the first entry from the list head */
        ListEntry = RemoveHeadList(ListHead);
#if DBG
        ListEntry->Flink = (PLIST_ENTRY)0xBADDD0FF;
        ListEntry->Blink = (PLIST_ENTRY)0xBADDD0FF;
#endif
    }

    /* Release the spinlock and restore interrupts */
    _ExiReleaseSpinLockAndRestoreInterrupts(Lock, Enable);

    /* Return the entry */
    return ListEntry;
}

PSINGLE_LIST_ENTRY
NTAPI
ExInterlockedPopEntryList(
    IN OUT PSINGLE_LIST_ENTRY ListHead,
    IN OUT PKSPIN_LOCK Lock)
{
    BOOLEAN Enable;
    PSINGLE_LIST_ENTRY ListEntry;

    /* Disable interrupts and acquire the spinlock */
    Enable = _ExiDisableInterruptsAndAcquireSpinlock(Lock);

    /* Pop the first entry from the list */
    ListEntry = PopEntryList(ListHead);
#if DBG
    if (ListEntry)
        ListEntry->Next = (PSINGLE_LIST_ENTRY)0xBADDD0FF;
#endif

    /* Release the spinlock and restore interrupts */
    _ExiReleaseSpinLockAndRestoreInterrupts(Lock, Enable);

    /* Return the entry */
    return ListEntry;
}

PSINGLE_LIST_ENTRY
NTAPI
ExInterlockedPushEntryList(
    IN OUT PSINGLE_LIST_ENTRY ListHead,
    IN OUT PSINGLE_LIST_ENTRY ListEntry,
    IN OUT PKSPIN_LOCK Lock)
{
    BOOLEAN Enable;
    PSINGLE_LIST_ENTRY OldListEntry;

    /* Disable interrupts and acquire the spinlock */
    Enable = _ExiDisableInterruptsAndAcquireSpinlock(Lock);

    /* Save the old top entry */
    OldListEntry = ListHead->Next;

    /* Push a new entry on the list */
    PushEntryList(ListHead, ListEntry);

    /* Release the spinlock and restore interrupts */
    _ExiReleaseSpinLockAndRestoreInterrupts(Lock, Enable);

    /* Return the entry */
    return OldListEntry;
}

INTERLOCKED_RESULT
NTAPI
ExInterlockedIncrementLong(
  IN PLONG Addend,
  IN PKSPIN_LOCK Lock)
{
    LONG Result;

    Result = _InterlockedIncrement(Addend);
    return (Result < 0) ? ResultNegative :
           (Result > 0) ? ResultPositive :
           ResultZero;
}

INTERLOCKED_RESULT
NTAPI
ExInterlockedDecrementLong(
  IN PLONG Addend,
  IN PKSPIN_LOCK Lock)
{
    LONG Result;

    Result = _InterlockedDecrement(Addend);
    return (Result < 0) ? ResultNegative :
           (Result > 0) ? ResultPositive :
           ResultZero;
}

ULONG
NTAPI
ExInterlockedExchangeUlong(
  IN PULONG Target,
  IN ULONG Value,
  IN PKSPIN_LOCK Lock)
{
    return (ULONG)_InterlockedExchange((PLONG)Target, (LONG)Value);
}

#ifdef _M_IX86

ULONG
FASTCALL
ExfInterlockedAddUlong(
    IN OUT PULONG Addend,
    IN ULONG Increment,
    IN OUT PKSPIN_LOCK Lock)
{
    BOOLEAN Enable;
    ULONG OldValue;

    /* Disable interrupts and acquire the spinlock */
    Enable = _ExiDisableInterruptsAndAcquireSpinlock(Lock);

    /* Save the old value */
    OldValue = *Addend;

    /* Do the operation */
    *Addend += Increment;

    /* Release the spinlock and restore interrupts */
    _ExiReleaseSpinLockAndRestoreInterrupts(Lock, Enable);

    /* Return the old value */
    return OldValue;
}

PLIST_ENTRY
FASTCALL
ExfInterlockedInsertHeadList(
    IN OUT PLIST_ENTRY ListHead,
    IN PLIST_ENTRY ListEntry,
    IN OUT PKSPIN_LOCK Lock)
{
    BOOLEAN Enable;
    PLIST_ENTRY FirstEntry;

    /* Disable interrupts and acquire the spinlock */
    Enable = _ExiDisableInterruptsAndAcquireSpinlock(Lock);

    /* Save the first entry */
    FirstEntry = ListHead->Flink;

    /* Insert the new entry */
    InsertHeadList(ListHead, ListEntry);

    /* Release the spinlock and restore interrupts */
    _ExiReleaseSpinLockAndRestoreInterrupts(Lock, Enable);

    /* Return the old first entry or NULL for empty list */
    return (FirstEntry == ListHead) ? NULL : FirstEntry;
}

PLIST_ENTRY
FASTCALL
ExfInterlockedInsertTailList(
    IN OUT PLIST_ENTRY ListHead,
    IN PLIST_ENTRY ListEntry,
    IN OUT PKSPIN_LOCK Lock)
{
    BOOLEAN Enable;
    PLIST_ENTRY LastEntry;

    /* Disable interrupts and acquire the spinlock */
    Enable = _ExiDisableInterruptsAndAcquireSpinlock(Lock);

    /* Save the last entry */
    LastEntry = ListHead->Blink;

    /* Insert the new entry */
    InsertTailList(ListHead, ListEntry);

    /* Release the spinlock and restore interrupts */
    _ExiReleaseSpinLockAndRestoreInterrupts(Lock, Enable);

    /* Return the old last entry or NULL for empty list */
    return (LastEntry == ListHead) ? NULL : LastEntry;
}


PLIST_ENTRY
FASTCALL
ExfInterlockedRemoveHeadList(
    IN OUT PLIST_ENTRY ListHead,
    IN OUT PKSPIN_LOCK Lock)
{
    BOOLEAN Enable;
    PLIST_ENTRY ListEntry;

    /* Disable interrupts and acquire the spinlock */
    Enable = _ExiDisableInterruptsAndAcquireSpinlock(Lock);

    /* Check if the list is empty */
    if (IsListEmpty(ListHead))
    {
        /* Return NULL */
        ListEntry = NULL;
    }
    else
    {
        /* Remove the first entry from the list head */
        ListEntry = RemoveHeadList(ListHead);
#if DBG
        ListEntry->Flink = (PLIST_ENTRY)0x0BADD0FF;
        ListEntry->Blink = (PLIST_ENTRY)0x0BADD0FF;
#endif
    }

    /* Release the spinlock and restore interrupts */
    _ExiReleaseSpinLockAndRestoreInterrupts(Lock, Enable);

    /* return the entry */
    return ListEntry;
}

PSINGLE_LIST_ENTRY
FASTCALL
ExfInterlockedPopEntryList(
    IN OUT PSINGLE_LIST_ENTRY ListHead,
    IN OUT PKSPIN_LOCK Lock)
{
    BOOLEAN Enable;
    PSINGLE_LIST_ENTRY ListEntry;

    /* Disable interrupts and acquire the spinlock */
    Enable = _ExiDisableInterruptsAndAcquireSpinlock(Lock);

    /* Pop the first entry from the list */
    ListEntry = PopEntryList(ListHead);
#if DBG
    if (ListEntry)
        ListEntry->Next = (PSINGLE_LIST_ENTRY)0xBADDD0FF;
#endif

    /* Release the spinlock and restore interrupts */
    _ExiReleaseSpinLockAndRestoreInterrupts(Lock, Enable);

    /* return the entry */
    return ListEntry;
}

PSINGLE_LIST_ENTRY
FASTCALL
ExfInterlockedPushEntryList(
    IN OUT PSINGLE_LIST_ENTRY ListHead,
    IN PSINGLE_LIST_ENTRY ListEntry,
    IN OUT PKSPIN_LOCK Lock)
{
    BOOLEAN Enable;
    PSINGLE_LIST_ENTRY OldListEntry;

    /* Disable interrupts and acquire the spinlock */
    Enable = _ExiDisableInterruptsAndAcquireSpinlock(Lock);

    /* Save the old top entry */
    OldListEntry = ListHead->Next;

    /* Push a new entry on the list */
    PushEntryList(ListHead, ListEntry);

    /* Release the spinlock and restore interrupts */
    _ExiReleaseSpinLockAndRestoreInterrupts(Lock, Enable);

    /* return the entry */
    return OldListEntry;
}

INTERLOCKED_RESULT
NTAPI
Exi386InterlockedIncrementLong(
    IN PLONG Addend)
{
    LONG Result;

    Result = _InterlockedIncrement(Addend);
    return (Result < 0) ? ResultNegative :
           (Result > 0) ? ResultPositive :
           ResultZero;
}

INTERLOCKED_RESULT
FASTCALL
Exfi386InterlockedIncrementLong(
    IN OUT LONG volatile *Addend)
{
    LONG Result;

    Result = _InterlockedIncrement(Addend);
    return (Result < 0) ? ResultNegative :
           (Result > 0) ? ResultPositive :
           ResultZero;
}

INTERLOCKED_RESULT
NTAPI
Exi386InterlockedDecrementLong(
    IN PLONG Addend)
{
    LONG Result;

    Result = _InterlockedDecrement(Addend);
    return (Result < 0) ? ResultNegative :
           (Result > 0) ? ResultPositive :
           ResultZero;
}

INTERLOCKED_RESULT
FASTCALL
Exfi386InterlockedDecrementLong(
    IN OUT PLONG Addend)
{
    LONG Result;

    Result = _InterlockedDecrement(Addend);
    return (Result < 0) ? ResultNegative :
           (Result > 0) ? ResultPositive :
           ResultZero;
}

LONG
NTAPI
Exi386InterlockedExchangeUlong(
    PLONG Target,
    LONG Exchange)
{
    return _InterlockedExchange(Target, Exchange);
}

ULONG
FASTCALL
Exfi386InterlockedExchangeUlong(
    IN OUT PULONG Target,
    IN ULONG Exchange)
{
    return _InterlockedExchange((PLONG)Target, Exchange);
}

LONGLONG
FASTCALL
ExInterlockedCompareExchange64(
    IN OUT LONGLONG volatile *Destination,
    IN PLONGLONG Exchange,
    IN PLONGLONG Comparand,
    IN PKSPIN_LOCK Lock)
{
    return _InterlockedCompareExchange64(Destination, *Exchange, *Comparand);
}

LONGLONG
FASTCALL
ExfInterlockedCompareExchange64(
    IN OUT LONGLONG volatile *Destination,
    IN PLONGLONG Exchange,
    IN PLONGLONG Comparand)
{
    return _InterlockedCompareExchange64(Destination, *Exchange, *Comparand);
}
#endif

#if 0

VOID
FASTCALL
ExInterlockedAddLargeStatistic(
    IN OUT PLARGE_INTEGER Addend,
    IN ULONG Increment)
{
}


#endif

