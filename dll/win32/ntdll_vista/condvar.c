/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Condition Variable Routines
 * PROGRAMMERS:       Thomas Weidenmueller <w3seek@reactos.com>
 *                    Stephan A. R�ger
 */

/* NOTE: This functionality can be optimized for releasing single
   threads or for releasing all waiting threads at once. This
   implementation is optimized for releasing a single thread at a time.
   It wakes up sleeping threads in FIFO order. */

/* INCLUDES ******************************************************************/

#include <rtl_vista.h>

#define NDEBUG
#include <debug.h>

/* INTERNAL TYPES ************************************************************/

#define COND_VAR_UNUSED_FLAG         ((ULONG_PTR)1)
#define COND_VAR_LOCKED_FLAG         ((ULONG_PTR)2)
#define COND_VAR_FLAGS_MASK          ((ULONG_PTR)3)
#define COND_VAR_ADDRESS_MASK        (~COND_VAR_FLAGS_MASK)

typedef struct _COND_VAR_WAIT_ENTRY
{
    /* ListEntry must have an alignment of at least 32-bits, since we
       want COND_VAR_ADDRESS_MASK to cover all of the address. */
    LIST_ENTRY ListEntry;
    PVOID WaitKey;
    BOOLEAN ListRemovalHandled;
} COND_VAR_WAIT_ENTRY, * PCOND_VAR_WAIT_ENTRY;

#define CONTAINING_COND_VAR_WAIT_ENTRY(address, field) \
    CONTAINING_RECORD(address, COND_VAR_WAIT_ENTRY, field)

/* GLOBALS *******************************************************************/

static HANDLE CondVarKeyedEventHandle = NULL;

/* INTERNAL FUNCTIONS ********************************************************/

FORCEINLINE
ULONG_PTR
InternalCmpXChgCondVarAcq(IN OUT PRTL_CONDITION_VARIABLE ConditionVariable,
                          IN ULONG_PTR Exchange,
                          IN ULONG_PTR Comperand)
{
    return (ULONG_PTR)InterlockedCompareExchangePointerAcquire(&ConditionVariable->Ptr,
                                                               (PVOID)Exchange,
                                                               (PVOID)Comperand);
}

FORCEINLINE
ULONG_PTR
InternalCmpXChgCondVarRel(IN OUT PRTL_CONDITION_VARIABLE ConditionVariable,
                          IN ULONG_PTR Exchange,
                          IN ULONG_PTR Comperand)
{
    return (ULONG_PTR)InterlockedCompareExchangePointerRelease(&ConditionVariable->Ptr,
                                                               (PVOID)Exchange,
                                                               (PVOID)Comperand);
}

FORCEINLINE
BOOLEAN *
InternalGetListRemovalHandledFlag(IN PCOND_VAR_WAIT_ENTRY Entry)
{
    return (BOOLEAN *)&Entry->ListRemovalHandled;
}

static
PCOND_VAR_WAIT_ENTRY
InternalLockCondVar(IN OUT PRTL_CONDITION_VARIABLE ConditionVariable,
                    IN PCOND_VAR_WAIT_ENTRY InsertEntry OPTIONAL,
                    IN BOOLEAN * AbortIfLocked OPTIONAL)
{
    /* InsertEntry and AbortIfLocked may be NULL on entry. This routine
       will return NULL if the lock was not acquired. Otherwise it has
       successfully acquired the lock and the return value is a valid
       reference to the list head associated with ConditionVariable.
       The caller must in this case call InternalUnlockCondVar later
       in order to unlock the condition variable.

       If InsertEntry is NULL and there are no entries on the list, this
       routine will not acquire the lock and return NULL. If InsertEntry
       is not NULL this routine ensures that InsertEntry will be on the
       list when it returns successfully.

       If the lock is owned by another thread and AbortIfLocked is NULL,
       this routine will block until it acquires the lock. If AbortIfLocked
       is not NULL and the lock is owned by another thread, this routine
       will periodically check if *AbortIfLocked is nonzero and if so, will
       return NULL instead of continuing the wait. */

    ULONG_PTR OldVal = (ULONG_PTR)ConditionVariable->Ptr;

    for (;;)
    {
        ULONG_PTR NewVal, LockRes;
        PLIST_ENTRY OldListHead;

        if (OldVal & COND_VAR_LOCKED_FLAG)
        {
            /* The locked flag is set, indicating someone else currently
               holds the lock. We'll spin until this flag becomes
               clear or we're asked to abort. */
            YieldProcessor();

            if ((AbortIfLocked != NULL) && *AbortIfLocked)
            {
                /* The caller wants us to abort in this case. */
                return NULL;
            }

            /* Refresh OldVal and try again. */
            OldVal = *(ULONG_PTR *)&ConditionVariable->Ptr;
            continue;
        }

        /* Retrieve the list head currently associated with the
           condition variable. */
        OldListHead = (PLIST_ENTRY)(OldVal & COND_VAR_ADDRESS_MASK);
        if (InsertEntry == NULL)
        {
            /* The caller doesn't want to put any entry on the list. */
            if (OldListHead == NULL)
            {
                /* The list is empty, so there is nothing to lock. */
                return NULL;
            }

            /* The list isn't empty. In this case we need to preserve
               all of OldVal. */
            NewVal = OldVal;
        }
        else
        {
            /* Let InsertEntry be the new list head. Preserve only the
               bits inside the COND_VAR_FLAGS_MASK range. */
            NewVal = ((OldVal & COND_VAR_FLAGS_MASK) |
                      (ULONG_PTR)&InsertEntry->ListEntry);
        }

        /* Set the flag that indicates someone is holding the lock and
           try to update the condition variable thread-safe. */
        NewVal |= COND_VAR_LOCKED_FLAG;
        LockRes = InternalCmpXChgCondVarAcq(ConditionVariable, NewVal, OldVal);
        if (LockRes == OldVal)
        {
            /* We successfully updated ConditionVariable the way we
               wanted and now hold the lock. */
            if (InsertEntry == NULL)
            {
                /* We know that OldVal contains a valid address in
                   this case. */
                ASSERT(OldListHead != NULL);
                return CONTAINING_COND_VAR_WAIT_ENTRY(OldListHead, ListEntry);
            }

            /* InsertEntry is not on the list yet, so add it. In any
               case InsertEntry will be the new list head. */
            if (OldListHead == NULL)
            {
                /* List was empty before. */
                InitializeListHead(&InsertEntry->ListEntry);
            }
            else
            {
                /* Make InsertEntry the last entry of the old list.
                   As InsertEntry will take the role as new list head,
                   OldListHead will become the second entry (InsertEntry->Flink)
                   on the new list. */
                InsertTailList(OldListHead, &InsertEntry->ListEntry);
            }

            return InsertEntry;
        }

        /* We didn't manage to update ConditionVariable, so try again. */
        OldVal = LockRes;
    }
}

static
VOID
InternalUnlockCondVar(IN OUT PRTL_CONDITION_VARIABLE ConditionVariable,
                      IN PCOND_VAR_WAIT_ENTRY RemoveEntry OPTIONAL)
{
    /* This routine assumes that the lock is being held on entry.
       RemoveEntry may be NULL. If it is not NULL, this routine
       assumes that RemoveEntry is on the list and will remove it
       before releasing the lock. */
    ULONG_PTR OldVal = (ULONG_PTR)ConditionVariable->Ptr;
    PLIST_ENTRY NewHeadEntry;

    ASSERT((OldVal & COND_VAR_LOCKED_FLAG) &&
           (OldVal & COND_VAR_ADDRESS_MASK));

    NewHeadEntry = (PLIST_ENTRY)(OldVal & COND_VAR_ADDRESS_MASK);
    if (RemoveEntry != NULL)
    {
        /* We have to drop RemoveEntry from the list. */
        if (&RemoveEntry->ListEntry == NewHeadEntry)
        {
            /* RemoveEntry is the list head. */
            if (!IsListEmpty(NewHeadEntry))
            {
                /* The second entry in the list will become the new
                   list head. It's from the thread that arrived
                   right before the owner of RemoveEntry. */
                NewHeadEntry = NewHeadEntry->Flink;
                RemoveEntryList(&RemoveEntry->ListEntry);
            }
            else
            {
                /* The list will be empty, so discard the list. */
                NewHeadEntry = NULL;
            }
        }
        else
        {
            /* RemoveEntry is not the list head. The current list head
               will remain. */
            RemoveEntryList(&RemoveEntry->ListEntry);
        }

        /* Indicate to the owner of RemoveEntry that the entry
           was removed from the list. RemoveEntry may not be touched
           from here on. We don't use volatile semantics here since
           the cache will anyway be flushed soon when we update
           ConditionVariable. */
        RemoveEntry->ListRemovalHandled = TRUE;
    }

    /* Now unlock thread-safe, while preserving any flags within the
       COND_VAR_FLAGS_MASK range except for COND_VAR_LOCKED_FLAG. */
    for (;;)
    {
        ULONG_PTR NewVal = ((OldVal & (COND_VAR_FLAGS_MASK ^ COND_VAR_LOCKED_FLAG)) |
                            (ULONG_PTR)NewHeadEntry);
        ULONG_PTR LockRes = InternalCmpXChgCondVarRel(ConditionVariable, NewVal, OldVal);
        if (LockRes == OldVal)
        {
            /* We unlocked. */
            break;
        }

        /* Try again. */
        OldVal = LockRes;
    }
}

static
VOID
InternalWake(IN OUT PRTL_CONDITION_VARIABLE ConditionVariable,
             IN BOOLEAN ReleaseAll)
{
    /* If ReleaseAll is zero on entry, one thread at most will be woken.
       Otherwise all waiting threads are woken. Wakeups happen in FIFO
       order. */
    PCOND_VAR_WAIT_ENTRY CONST HeadEntry = InternalLockCondVar(ConditionVariable, NULL, NULL);
    PCOND_VAR_WAIT_ENTRY Entry;
    PCOND_VAR_WAIT_ENTRY NextEntry;
    LARGE_INTEGER Timeout;
    PCOND_VAR_WAIT_ENTRY RemoveOnUnlockEntry;

    ASSERT(CondVarKeyedEventHandle != NULL);

    if (HeadEntry == NULL)
    {
        /* There is noone there to wake up. In this case do nothing
           and return immediately. We don't stockpile releases. */
        return;
    }

    Timeout.QuadPart = 0;
    RemoveOnUnlockEntry = NULL;

    /* Release sleeping threads. We will iterate from the last entry on
       the list to the first. Note that the loop condition is always
       true for the initial test. */
    for (Entry = CONTAINING_COND_VAR_WAIT_ENTRY(HeadEntry->ListEntry.Blink, ListEntry);
         Entry != NULL;
         Entry = NextEntry)
    {
        NTSTATUS Status;

        if (HeadEntry == Entry)
        {
            /* After the current entry we've iterated through the
               entire list in backward direction. Then exit.*/
            NextEntry = NULL;
        }
        else
        {
            /* Store away the next reference right now, since we may
               not touch Entry anymore at the end of the block. */
            NextEntry = CONTAINING_COND_VAR_WAIT_ENTRY(Entry->ListEntry.Blink, ListEntry);
        }

        /* Wake the thread associated with this event. We will
           immediately return if we failed (zero timeout). */
        Status = NtReleaseKeyedEvent(CondVarKeyedEventHandle,
                                     &Entry->WaitKey,
                                     FALSE,
                                     &Timeout);

        if (!NT_SUCCESS(Status))
        {
            /* We failed to wake a thread. We'll keep trying. */
            ASSERT(STATUS_INVALID_HANDLE != Status);
            continue;
        }

        /* We've woken a thread and will make sure this thread
           is removed from the list. */
        if (HeadEntry == Entry)
        {
            /* This is the list head. We can't remove it as easily as
               other entries and will pass it to the unlock routine
               later (we will exit the loop after this round anyway). */
            RemoveOnUnlockEntry = HeadEntry;
        }
        else
        {
            /* We can remove the entry right away. */
            RemoveEntryList(&Entry->ListEntry);

            /* Now tell the woken thread that removal from the list was
               already taken care of here so that this thread can resume
               its normal operation more quickly. We may not touch
               Entry after signaling this, since it may lie in invalid
               memory from there on. */
            *InternalGetListRemovalHandledFlag(Entry) = TRUE;
        }

        if (!ReleaseAll)
        {
            /* We've successfully woken one thread as the caller
               demanded. */
            break;
        }
    }

    InternalUnlockCondVar(ConditionVariable, RemoveOnUnlockEntry);
}

VOID
NTAPI
RtlAcquireSRWLockExclusive(IN OUT PRTL_SRWLOCK SRWLock);
VOID
NTAPI
RtlAcquireSRWLockShared(IN OUT PRTL_SRWLOCK SRWLock);
VOID
NTAPI
RtlReleaseSRWLockExclusive(IN OUT PRTL_SRWLOCK SRWLock);
VOID
NTAPI
RtlReleaseSRWLockShared(IN OUT PRTL_SRWLOCK SRWLock);

static
NTSTATUS
InternalSleep(IN OUT PRTL_CONDITION_VARIABLE ConditionVariable,
              IN OUT PRTL_CRITICAL_SECTION CriticalSection OPTIONAL,
              IN OUT PRTL_SRWLOCK SRWLock OPTIONAL,
              IN ULONG SRWFlags,
              IN const LARGE_INTEGER * TimeOut OPTIONAL)
{
    /* Either CriticalSection or SRWLock must be NULL, but not both.
       These caller provided lock must be held on entry and will be
       held again on return. */

    COND_VAR_WAIT_ENTRY OwnEntry;
    NTSTATUS Status;

    ASSERT(CondVarKeyedEventHandle != NULL);
    ASSERT((CriticalSection == NULL) != (SRWLock == NULL));

    RtlZeroMemory(&OwnEntry, sizeof(OwnEntry));

    /* Put OwnEntry on the list. */
    InternalLockCondVar(ConditionVariable, &OwnEntry, NULL);
    InternalUnlockCondVar(ConditionVariable, NULL);

    /* We can now drop the caller provided lock as a preparation for
       going to sleep. */
    if (CriticalSection == NULL)
    {
        if (0 == (RTL_CONDITION_VARIABLE_LOCKMODE_SHARED & SRWFlags))
        {
            RtlReleaseSRWLockExclusive(SRWLock);
        }
        else
        {
            RtlReleaseSRWLockShared(SRWLock);
        }
    }
    else
    {
        RtlLeaveCriticalSection(CriticalSection);
    }

    /* Now sleep using the caller provided timeout. */
    Status = NtWaitForKeyedEvent(CondVarKeyedEventHandle,
                                 &OwnEntry.WaitKey,
                                 FALSE,
                                 (PLARGE_INTEGER)TimeOut);

    ASSERT(STATUS_INVALID_HANDLE != Status);

    if (!*InternalGetListRemovalHandledFlag(&OwnEntry))
    {
        /* Remove OwnEntry from the list again, since it still seems to
           be on the list. We will know for sure once we've acquired
           the lock. */
        if (InternalLockCondVar(ConditionVariable,
                                NULL,
                                InternalGetListRemovalHandledFlag(&OwnEntry)))
        {
            /* Unlock and potentially remove OwnEntry. Self-removal is
               usually only necessary when a timeout occurred. */
            InternalUnlockCondVar(ConditionVariable,
                                  !OwnEntry.ListRemovalHandled ?
                                  &OwnEntry : NULL);
        }
    }

#ifdef _DEBUG
    /* Clear OwnEntry to aid in detecting bugs. */
    RtlZeroMemory(&OwnEntry, sizeof(OwnEntry));
#endif

    /* Reacquire the caller provided lock, as we are about to return. */
    if (CriticalSection == NULL)
    {
        if (0 == (RTL_CONDITION_VARIABLE_LOCKMODE_SHARED & SRWFlags))
        {
            RtlAcquireSRWLockExclusive(SRWLock);
        }
        else
        {
            RtlAcquireSRWLockShared(SRWLock);
        }
    }
    else
    {
        RtlEnterCriticalSection(CriticalSection);
    }

    /* Return whatever NtWaitForKeyedEvent returned. */
    return Status;
}

VOID
RtlpInitializeKeyedEvent(VOID)
{
    ASSERT(CondVarKeyedEventHandle == NULL);
    NtCreateKeyedEvent(&CondVarKeyedEventHandle, EVENT_ALL_ACCESS, NULL, 0);
}

VOID
RtlpCloseKeyedEvent(VOID)
{
    ASSERT(CondVarKeyedEventHandle != NULL);
    NtClose(CondVarKeyedEventHandle);
    CondVarKeyedEventHandle = NULL;
}

/* EXPORTED FUNCTIONS ********************************************************/

VOID
NTAPI
RtlInitializeConditionVariable(OUT PRTL_CONDITION_VARIABLE ConditionVariable)
{
    ConditionVariable->Ptr = NULL;
}

VOID
NTAPI
RtlWakeConditionVariable(IN OUT PRTL_CONDITION_VARIABLE ConditionVariable)
{
    InternalWake(ConditionVariable, FALSE);
}

VOID
NTAPI
RtlWakeAllConditionVariable(IN OUT PRTL_CONDITION_VARIABLE ConditionVariable)
{
    InternalWake(ConditionVariable, TRUE);
}

NTSTATUS
NTAPI
RtlSleepConditionVariableCS(IN OUT PRTL_CONDITION_VARIABLE ConditionVariable,
                            IN OUT PRTL_CRITICAL_SECTION CriticalSection,
                            IN const LARGE_INTEGER * TimeOut OPTIONAL)
{
    return InternalSleep(ConditionVariable,
                         CriticalSection,
                         (PRTL_SRWLOCK)NULL,
                         0,
                         TimeOut);
}

NTSTATUS
NTAPI
RtlSleepConditionVariableSRW(IN OUT PRTL_CONDITION_VARIABLE ConditionVariable,
                             IN OUT PRTL_SRWLOCK SRWLock,
                             IN const LARGE_INTEGER * TimeOut OPTIONAL,
                             IN ULONG Flags)
{
    return InternalSleep(ConditionVariable,
                         (PRTL_CRITICAL_SECTION)NULL,
                         SRWLock,
                         Flags,
                         TimeOut);
}

/* EOF */
