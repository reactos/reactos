/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/cm/cmkcbncb.c
 * PURPOSE:         Routines for handling KCBs, NCBs, as well as key hashes.
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>
#include "cm.h"

/* GLOBALS *******************************************************************/

ULONG CmpHashTableSize = 2048;
PCM_KEY_HASH_TABLE_ENTRY CmpCacheTable;
PCM_NAME_HASH_TABLE_ENTRY CmpNameCacheTable;

LIST_ENTRY CmpFreeKCBListHead;

BOOLEAN CmpAllocInited;
KGUARDED_MUTEX CmpAllocBucketLock, CmpDelayAllocBucketLock;
WORK_QUEUE_ITEM CmpDelayDerefKCBWorkItem;
LIST_ENTRY CmpFreeDelayItemsListHead;

ULONG CmpDelayedCloseSize;
ULONG CmpDelayedCloseElements;
KGUARDED_MUTEX CmpDelayedCloseTableLock;
BOOLEAN CmpDelayCloseWorkItemActive;
WORK_QUEUE_ITEM CmpDelayCloseWorkItem;
LIST_ENTRY CmpDelayedLRUListHead;
ULONG CmpDelayCloseIntervalInSeconds = 5;
KDPC CmpDelayCloseDpc;
KTIMER CmpDelayCloseTimer;

KGUARDED_MUTEX CmpDelayDerefKCBLock;
BOOLEAN CmpDelayDerefKCBWorkItemActive;
LIST_ENTRY CmpDelayDerefKCBListHead;
ULONG CmpDelayDerefKCBIntervalInSeconds = 5;
KDPC CmpDelayDerefKCBDpc;
KTIMER CmpDelayDerefKCBTimer;

BOOLEAN CmpHoldLazyFlush;

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
CmpDelayCloseDpcRoutine(IN PKDPC Dpc,
                        IN PVOID DeferredContext,
                        IN PVOID SystemArgument1,
                        IN PVOID SystemArgument2)
{
    /* Sanity check */
    ASSERT(CmpDelayCloseWorkItemActive);

    /* Queue the work item */
    ExQueueWorkItem(&CmpDelayCloseWorkItem, DelayedWorkQueue);
}

VOID
NTAPI
CmpDelayCloseWorker(IN PVOID Context)
{
    PAGED_CODE();

    /* Sanity check */
    ASSERT(CmpDelayCloseWorkItemActive);

    /* FIXME: TODO */
    ASSERT(FALSE);
}

VOID
NTAPI
CmpInitializeDelayedCloseTable(VOID)
{
    
    /* Setup the delayed close lock */
    KeInitializeGuardedMutex(&CmpDelayedCloseTableLock);
    
    /* Setup the work item */
    ExInitializeWorkItem(&CmpDelayCloseWorkItem, CmpDelayCloseWorker, NULL);
    
    /* Setup the list head */
    InitializeListHead(&CmpDelayedLRUListHead);
    
    /* Setup the DPC and its timer */
    KeInitializeDpc(&CmpDelayCloseDpc, CmpDelayCloseDpcRoutine, NULL);
    KeInitializeTimer(&CmpDelayCloseTimer);
}

VOID
NTAPI
CmpInitializeCache(VOID)
{
    ULONG Length, i;
    
    /* Calculate length for the table */
    Length = CmpHashTableSize * sizeof(CM_KEY_HASH_TABLE_ENTRY);
    
    /* Allocate it */
    CmpCacheTable = ExAllocatePoolWithTag(PagedPool, Length, TAG_CM);
    if (!CmpCacheTable)
    {
        /* Take the system down */
        KeBugCheckEx(CONFIG_INITIALIZATION_FAILED, 3, 1, 0, 0);
    }
    
    /* Initialize the locks */
    for (i = 0;i < CmpHashTableSize; i++)
    {
        /* Setup the pushlock */
        ExInitializePushLock((PULONG_PTR)&CmpCacheTable[i].Lock);
    }
    
    /* Calculate length for the name cache */
    Length = CmpHashTableSize * sizeof(CM_NAME_HASH_TABLE_ENTRY);
    
    /* Now allocate the name cache table */
    CmpNameCacheTable = ExAllocatePoolWithTag(PagedPool, Length, TAG_CM);
    if (!CmpNameCacheTable)
    {
        /* Take the system down */
        KeBugCheckEx(CONFIG_INITIALIZATION_FAILED, 3, 3, 0, 0);
    }
    
    /* Initialize the locks */
    for (i = 0;i < CmpHashTableSize; i++)
    {
        /* Setup the pushlock */
        ExInitializePushLock((PULONG_PTR)&CmpNameCacheTable[i].Lock);
    }
    
    /* Setup the delayed close table */
    CmpInitializeDelayedCloseTable();
}

VOID
NTAPI
CmpInitCmPrivateAlloc(VOID)
{
    /* Make sure we didn't already do this */
    if (!CmpAllocInited)
    {
        /* Setup the lock and list */
        KeInitializeGuardedMutex(&CmpAllocBucketLock);
        InitializeListHead(&CmpFreeKCBListHead);
        CmpAllocInited = TRUE;
    }
}

VOID
NTAPI
CmpInitCmPrivateDelayAlloc(VOID)
{
    /* Initialize the delay allocation list and lock */
    KeInitializeGuardedMutex(&CmpDelayAllocBucketLock);
    InitializeListHead(&CmpFreeDelayItemsListHead);
}

VOID
NTAPI
CmpDelayDerefKCBDpcRoutine(IN PKDPC Dpc,
                           IN PVOID DeferredContext,
                           IN PVOID SystemArgument1,
                           IN PVOID SystemArgument2)
{
    /* Sanity check */
    ASSERT(CmpDelayDerefKCBWorkItemActive);

    /* Queue the work item */
    ExQueueWorkItem(&CmpDelayDerefKCBWorkItem, DelayedWorkQueue);
}

VOID
NTAPI
CmpDelayDerefKCBWorker(IN PVOID Context)
{
    PAGED_CODE();

    /* Sanity check */
    ASSERT(CmpDelayDerefKCBWorkItemActive);

    /* FIXME: TODO */
    ASSERT(FALSE);
}

VOID
NTAPI
CmpInitDelayDerefKCBEngine(VOID)
{
    /* Initialize lock and list */
    KeInitializeGuardedMutex(&CmpDelayDerefKCBLock);
    InitializeListHead(&CmpDelayDerefKCBListHead);

    /* Setup the work item */
    ExInitializeWorkItem(&CmpDelayDerefKCBWorkItem,
                         CmpDelayDerefKCBWorker,
                         NULL);

    /* Setup the DPC and timer for it */
    KeInitializeDpc(&CmpDelayDerefKCBDpc, CmpDelayDerefKCBDpcRoutine, NULL);
    KeInitializeTimer(&CmpDelayDerefKCBTimer);
}

VOID
NTAPI
CmpRemoveKeyHash(IN PCM_KEY_HASH KeyHash)
{
    PCM_KEY_HASH *Prev;
    PCM_KEY_HASH Current;

    /* Lookup all the keys in this index entry */
    Prev = &GET_HASH_ENTRY(CmpCacheTable, KeyHash->ConvKey).Entry;
    while (TRUE)
    {
        /* Save the current one and make sure it's valid */
        Current = *Prev;
        ASSERT(Current != NULL);

        /* Check if it matches */
        if (Current == KeyHash)
        {
            /* Then write the previous one */
            *Prev = Current->NextHash;
            break;
        }

        /* Otherwise, keep going */
        Prev = &Current->NextHash;
    }
}

PCM_KEY_CONTROL_BLOCK
NTAPI
CmpInsertKeyHash(IN PCM_KEY_HASH KeyHash,
                 IN BOOLEAN IsFake)
{
    ULONG i;
    PCM_KEY_HASH Entry;

    /* Get the hash index */
    i = GET_HASH_INDEX(KeyHash->ConvKey);

    /* If this is a fake key, increase the key cell to use the parent data */
    if (IsFake) KeyHash->KeyCell++;

    /* Loop the hash table */
    Entry = CmpCacheTable[i].Entry;
    while (Entry)
    {
        /* Check if this matches */
        if ((KeyHash->ConvKey == Entry->ConvKey) &&
            (KeyHash->KeyCell == Entry->KeyCell) &&
            (KeyHash->KeyHive == Entry->KeyHive))
        {
            /* Return it */
            return CONTAINING_RECORD(Entry, CM_KEY_CONTROL_BLOCK, KeyHash);
        }

        /* Keep looping */
        Entry = Entry->NextHash;
    }

    /* No entry found, add this one and return NULL since none existed */
    KeyHash->NextHash = CmpCacheTable[i].Entry;
    CmpCacheTable[i].Entry = KeyHash;
    return NULL;
}

// FIXME: NEEDS COMPLETION
PCM_NAME_CONTROL_BLOCK
NTAPI
CmpGetNcb(IN PUNICODE_STRING NodeName)
{
    PCM_NAME_CONTROL_BLOCK Ncb = NULL;
    ULONG ConvKey = 0;
    PWCHAR p, pp;
    ULONG i;
    BOOLEAN IsCompressed = TRUE, Found;
    PCM_NAME_HASH HashEntry;
    ULONG Length;

    /* Loop the name */
    p = NodeName->Buffer;
    for (i = 0; i < NodeName->Length; i += sizeof(WCHAR))
    {
        /* Make sure it's not a slash */
        if (*p != OBJ_NAME_PATH_SEPARATOR)
        {
            /* Add it to the hash */
            ConvKey = 37 * ConvKey + RtlUpcaseUnicodeChar(*p);
        }

        /* Next character */
        p++;
    }

    /* Set assumed lengh and loop to check */
    Length = NodeName->Length / sizeof(WCHAR);
    for (i = 0; i < (NodeName->Length / sizeof(WCHAR)); i++)
    {
        /* Check if this is a valid character */
        if (*NodeName->Buffer > (UCHAR)-1)
        {
            /* This is the actual size, and we know we're not compressed */
            Length = NodeName->Length;
            IsCompressed = FALSE;
        }
    }

    /* Get the hash entry */
    HashEntry = GET_HASH_ENTRY(CmpNameCacheTable, ConvKey).Entry;
    while (HashEntry)
    {
        /* Get the current NCB */
        Ncb = CONTAINING_RECORD(HashEntry, CM_NAME_CONTROL_BLOCK, NameHash);

        /* Check if the hash matches */
        if ((ConvKey = HashEntry->ConvKey) && (Length = Ncb->NameLength))
        {
            /* Assume success */
            Found = TRUE;

            /* If the NCB is compressed, do a compressed name compare */
            if (Ncb->Compressed)
            {
                /* Compare names */
                if (CmpCompareCompressedName(NodeName, Ncb->Name, Length))
                {
                    /* We failed */
                    Found = FALSE;
                }
            }
            else
            {
                /* Do a manual compare */
                p = NodeName->Buffer;
                pp = Ncb->Name;
                for (i = 0; i < Ncb->NameLength; i += sizeof(WCHAR))
                {
                    /* Compare the character */
                    if (RtlUpcaseUnicodeChar(*p) != RtlUpcaseUnicodeChar(*pp))
                    {
                        /* Failed */
                        Found = FALSE;
                        break;
                    }

                    /* Next chars */
                    //*p++;
                    //*pp++;
                }
            }

            /* Check if we found a name */
            if (Found)
            {

            }
        }

        /* Go to the next hash */
        HashEntry = HashEntry->NextHash;
    }

    /* Return the NCB found */
    return Ncb;
}

VOID
NTAPI
CmpRemoveKeyControlBlock(IN PCM_KEY_CONTROL_BLOCK Kcb)
{
    /* Make sure that the registry and KCB are utterly locked */
    ASSERT((CmpIsKcbLockedExclusive(Kcb) == TRUE) ||
           (CmpTestRegistryLockExclusive() == TRUE));

    /* Remove the key hash */
    CmpRemoveKeyHash(&Kcb->KeyHash);
}

VOID
NTAPI
CmpFreeKeyControlBlock(IN PCM_KEY_CONTROL_BLOCK Kcb)
{
    ULONG i;
    PCM_ALLOC_PAGE AllocPage;
    PAGED_CODE();

    /* Sanity checks */
    ASSERT(IsListEmpty(&(Kcb->KeyBodyListHead)) == TRUE);
    for (i = 0; i < 4; i++) ASSERT(Kcb->KeyBodyArray[i] == NULL);

    /* Check if it wasn't privately allocated */
    if (!Kcb->PrivateAlloc)
    {
        /* Free it from the pool */
        ExFreePool(Kcb);
        return;
    }
    
    /* Acquire the private allocation lock */
    KeAcquireGuardedMutex(&CmpAllocBucketLock);
    
    /* Sanity check on lock ownership */
    ASSERT((GET_HASH_ENTRY(CmpCacheTable, Kcb->ConvKey).Owner ==
            KeGetCurrentThread()) ||
           (CmpTestRegistryLockExclusive() == TRUE));
    
    /* Add us to the free list */
    InsertTailList(&CmpFreeKCBListHead, &Kcb->FreeListEntry);
    
    /* Get the allocation page */
    AllocPage = (PCM_ALLOC_PAGE)((ULONG_PTR)Kcb & 0xFFFFF000);
    
    /* Sanity check */
    ASSERT(AllocPage->FreeCount != CM_KCBS_PER_PAGE);
    
    /* Increase free count */
    if (++AllocPage->FreeCount == CM_KCBS_PER_PAGE)
    {
        /* Loop all the entries */
        for (i = CM_KCBS_PER_PAGE; i; i--)
        {
            /* Get the KCB */
            Kcb = (PVOID)((ULONG_PTR)AllocPage +
                          FIELD_OFFSET(CM_ALLOC_PAGE, AllocPage) +
                          i * sizeof(CM_KEY_CONTROL_BLOCK));
            
            /* Remove the entry */ 
            RemoveEntryList(&Kcb->FreeListEntry);
        }
        
        /* Free the page */
        ExFreePool(AllocPage);
    }
    
    /* Release the lock */
    KeReleaseGuardedMutex(&CmpAllocBucketLock);
}

VOID
NTAPI
CmpDereferenceNcbWithLock(IN PCM_NAME_CONTROL_BLOCK Ncb)
{
    PCM_NAME_HASH Current, Next;

    /* Lock the NCB */
    CmpAcquireNcbLockExclusive(Ncb);

    /* Decrease the reference count */
    if (!(--Ncb->RefCount))
    {
        /* Find the NCB in the table */
        Next = GET_HASH_ENTRY(CmpNameCacheTable, Ncb->ConvKey).Entry;
        while (TRUE)
        {
            /* Check the current entry */
            Current = Next;
            ASSERT(Current != NULL);
            if (Current == &Ncb->NameHash) break;

            /* Get to the next one */
            Next = Current->NextHash;
        }

        /* Found it, now unlink it and free it */
        Current->NextHash = Next->NextHash;
        ExFreePool(Ncb);
    }

    /* Release the lock */
    CmpReleaseNcbLock(Ncb);
}

VOID
NTAPI
CmpRemoveFromDelayedClose(IN PCM_KEY_CONTROL_BLOCK Kcb)
{
    PCM_DELAYED_CLOSE_ENTRY Entry;
    ULONG NewRefCount, OldRefCount;
    PAGED_CODE();

    /* Sanity checks */
    ASSERT((CmpIsKcbLockedExclusive(Kcb) == TRUE) ||
           (CmpTestRegistryLockExclusive() == TRUE));
    if (Kcb->DelayedCloseIndex == CmpDelayedCloseSize) ASSERT(FALSE);

    /* Get the entry and lock the table */
    Entry = Kcb->DelayCloseEntry;
    ASSERT(Entry);
    KeAcquireGuardedMutex(&CmpDelayedCloseTableLock);

    /* Remove the entry */
    RemoveEntryList(&Entry->DelayedLRUList);

    /* Release the lock */
    KeReleaseGuardedMutex(&CmpDelayedCloseTableLock);

    /* Free the entry */
    CmpFreeDelayItem(Entry);

    /* Reduce the number of elements */
    InterlockedDecrement((PLONG)&CmpDelayedCloseElements);

    /* Sanity check */
    if (!Kcb->InDelayClose) ASSERT(FALSE);

    /* Get the old reference count */
    OldRefCount = Kcb->InDelayClose;
    ASSERT(OldRefCount == 1);

    /* Set it to 0 */
    NewRefCount = InterlockedCompareExchange((PLONG)&Kcb->InDelayClose,
                                             0,
                                             OldRefCount);
    if (NewRefCount != OldRefCount) ASSERT(FALSE);

    /* Remove the link to the entry */
    Kcb->DelayCloseEntry = NULL;

    /* Set new delay size and remove the delete flag */
    Kcb->DelayedCloseIndex = CmpDelayedCloseSize;
}

BOOLEAN
NTAPI
CmpReferenceKeyControlBlock(IN PCM_KEY_CONTROL_BLOCK Kcb)
{
    /* Check if this is the KCB's first reference */
    if (Kcb->RefCount == 0)
    {
        /* Check if the KCB is locked in shared mode */
        if (!CmpIsKcbLockedExclusive(Kcb))
        {
            /* Convert it to exclusive */
            if (!CmpTryToConvertKcbSharedToExclusive(Kcb))
            {
                /* Set the delayed close index so that we can be ignored */
                Kcb->DelayedCloseIndex = 1;

                /* Increase the reference count while we release the lock */
                InterlockedIncrement((PLONG)&Kcb->RefCount);
                
                /* Go from shared to exclusive */
                CmpConvertKcbSharedToExclusive(Kcb);

                /* Decrement the reference count; the lock is now held again */
                InterlockedDecrement((PLONG)&Kcb->RefCount);
                
                /* Check if we still control the index */
                if (Kcb->DelayedCloseIndex == 1)
                {
                    /* Reset it */
                    Kcb->DelayedCloseIndex = 0;
                }
                else
                {
                    /* Sanity check */
                    ASSERT((Kcb->DelayedCloseIndex == CmpDelayedCloseSize) ||
                           (Kcb->DelayedCloseIndex == 0));
                }
            }
        }
    }

    /* Increase the reference count */
    if (InterlockedIncrement((PLONG)&Kcb->RefCount) == 0)
    {
        /* We've overflown to 64K references, bail out */
        InterlockedDecrement((PLONG)&Kcb->RefCount);
        return FALSE;
    }

    /* Check if this was the last close index */
    if (!Kcb->DelayedCloseIndex)
    {
        /* Check if the KCB is locked in shared mode */
        if (!CmpIsKcbLockedExclusive(Kcb))
        {
            /* Convert it to exclusive */
            if (!CmpTryToConvertKcbSharedToExclusive(Kcb))
            {
                /* Go from shared to exclusive */
                CmpConvertKcbSharedToExclusive(Kcb);
            }
        }

        /* If we're still the last entry, remove us */
        if (!Kcb->DelayedCloseIndex) CmpRemoveFromDelayedClose(Kcb);
    }

    /* Return success */
    return TRUE;
}

VOID
NTAPI
CmpDelayDerefKeyControlBlock(IN PCM_KEY_CONTROL_BLOCK Kcb)
{
    USHORT OldRefCount, NewRefCount;
    LARGE_INTEGER Timeout;
    PCM_DELAY_DEREF_KCB_ITEM Entry;
    PAGED_CODE();

    /* Get the previous reference count */
    OldRefCount = Kcb->RefCount;

    /* Write the new one */
    NewRefCount = (USHORT)InterlockedCompareExchange((PLONG)&Kcb->RefCount,
                                                     OldRefCount - 1,
                                                     OldRefCount);
    if (NewRefCount != OldRefCount) return;

    /* Allocate a delay item */
    Entry = CmpAllocateDelayItem();
    if (!Entry) return;

    /* Set the KCB */
    Entry->Kcb = Kcb;

    /* Acquire the delayed deref table lock */
    KeAcquireGuardedMutex(&CmpDelayDerefKCBLock);

    /* Insert the entry into the list */
    InsertTailList(&CmpDelayDerefKCBListHead, &Entry->ListEntry);

    /* Check if we need to enable anything */
    if (!CmpDelayDerefKCBWorkItemActive)
    {
        /* Yes, we have no work item, setup the interval */
        Timeout.QuadPart = CmpDelayDerefKCBIntervalInSeconds * -10000000;
        KeSetTimer(&CmpDelayDerefKCBTimer, Timeout, &CmpDelayDerefKCBDpc);
    }

    /* Release the table lock */
    KeReleaseGuardedMutex(&CmpDelayDerefKCBLock);
}

VOID
NTAPI
CmpCleanUpKcbValueCache(IN PCM_KEY_CONTROL_BLOCK Kcb)
{
    PULONG_PTR CachedList;
    ULONG i;

    /* Sanity check */
    ASSERT((CmpIsKcbLockedExclusive(Kcb) == TRUE) ||
           (CmpTestRegistryLockExclusive() == TRUE));

    /* Check if the value list is cached */
    if (CMP_IS_CELL_CACHED(Kcb->ValueCache.ValueList))
    {
        /* Get the cache list */
        CachedList = (PULONG_PTR)CMP_GET_CACHED_DATA(Kcb->ValueCache.ValueList);
        for (i = 0; i < Kcb->ValueCache.Count; i++)
        {
            /* Check if this cell is cached */
            if (CMP_IS_CELL_CACHED(CachedList[i]))
            {
                /* Free it */
                ExFreePool((PVOID)CMP_GET_CACHED_CELL(CachedList[i]));
            }
        }

        /* Now free the list */
        ExFreePool((PVOID)CMP_GET_CACHED_CELL(Kcb->ValueCache.ValueList));
        Kcb->ValueCache.ValueList = HCELL_NIL;
    }
    else if (Kcb->ExtFlags & CM_KCB_SYM_LINK_FOUND)
    {
        /* This is a sym link, check if there's only one reference left */
        if ((Kcb->ValueCache.RealKcb->RefCount == 1) &&
            !(Kcb->ValueCache.RealKcb->Delete))
        {
            /* Disable delay close for the KCB */
            Kcb->ValueCache.RealKcb->ExtFlags |= CM_KCB_NO_DELAY_CLOSE;
        }

        /* Dereference the KCB */
        CmpDelayDerefKeyControlBlock(Kcb->ValueCache.RealKcb);
        Kcb->ExtFlags &= ~ CM_KCB_SYM_LINK_FOUND;
    }
}

VOID
NTAPI
CmpCleanUpKcbCacheWithLock(IN PCM_KEY_CONTROL_BLOCK Kcb,
                           IN BOOLEAN LockHeldExclusively)
{
    PCM_KEY_CONTROL_BLOCK Parent;
    PAGED_CODE();

    /* Sanity checks */
    ASSERT((CmpIsKcbLockedExclusive(Kcb) == TRUE) ||
           (CmpTestRegistryLockExclusive() == TRUE));
    ASSERT(Kcb->RefCount == 0);

    /* Cleanup the value cache */
    CmpCleanUpKcbValueCache(Kcb);

    /* Reference the NCB */
    CmpDereferenceNcbWithLock(Kcb->NameBlock);

    /* Check if we have an index hint block and free it */
    if (Kcb->ExtFlags & CM_KCB_SUBKEY_HINT) ExFreePool(Kcb->IndexHint);

    /* Check if we were already deleted */
    Parent = Kcb->ParentKcb;
    if (!Kcb->Delete) CmpRemoveKeyControlBlock(Kcb);
    
    /* Free the KCB as well */
    CmpFreeKeyControlBlock(Kcb);

    /* Check if we have a parent */
    if (Parent)
    {
        /* Dereference the parent */
        LockHeldExclusively ?
            CmpDereferenceKeyControlBlockWithLock(Kcb,LockHeldExclusively) :
            CmpDelayDerefKeyControlBlock(Kcb);
    }
}

VOID
NTAPI
CmpArmDelayedCloseTimer(VOID)
{
    LARGE_INTEGER Timeout;
    PAGED_CODE();

    /* Setup the interval */
    Timeout.QuadPart = CmpDelayCloseIntervalInSeconds * -10000000;
    KeSetTimer(&CmpDelayCloseTimer, Timeout, &CmpDelayCloseDpc);
}

VOID
NTAPI
CmpAddToDelayedClose(IN PCM_KEY_CONTROL_BLOCK Kcb,
                     IN BOOLEAN LockHeldExclusively)
{
    ULONG i;
    ULONG OldRefCount, NewRefCount;
    PCM_DELAYED_CLOSE_ENTRY Entry;
    PAGED_CODE();

    /* Sanity check */
    ASSERT((CmpIsKcbLockedExclusive(Kcb) == TRUE) ||
           (CmpTestRegistryLockExclusive() == TRUE));

    /* Make sure it's valid */
    if (Kcb->DelayedCloseIndex != CmpDelayedCloseSize) ASSERT(FALSE);

    /* Sanity checks */
    ASSERT(Kcb->RefCount == 0);
    ASSERT(IsListEmpty(&Kcb->KeyBodyListHead) == TRUE);
    for (i = 0; i < 4; i++) ASSERT(Kcb->KeyBodyArray[i] == NULL);

    /* Allocate a delay item */
    Entry = CmpAllocateDelayItem();
    if (!Entry)
    {
        /* Cleanup immediately */
        CmpCleanUpKcbCacheWithLock(Kcb, LockHeldExclusively);
        return;
    }

    /* Sanity check */
    if (Kcb->InDelayClose) ASSERT(FALSE);

    /* Get the previous reference count */
    OldRefCount = Kcb->InDelayClose;
    ASSERT(OldRefCount == 0);

    /* Write the new one */
    NewRefCount = InterlockedCompareExchange((PLONG)&Kcb->InDelayClose,
                                             1,
                                             OldRefCount);
    if (NewRefCount != OldRefCount) ASSERT(FALSE);

    /* Reset the delayed close index */
    Kcb->DelayedCloseIndex = 0;

    /* Set up the close entry */
    Kcb->DelayCloseEntry = Entry;
    Entry->KeyControlBlock = Kcb;

    /* Increase the number of elements */
    InterlockedIncrement((PLONG)&CmpDelayedCloseElements);

    /* Acquire the delayed close table lock */
    KeAcquireGuardedMutex(&CmpDelayedCloseTableLock);

    /* Insert the entry into the list */
    InsertHeadList(&CmpDelayedLRUListHead, &Entry->DelayedLRUList);

    /* Check if we need to enable anything */
    if ((CmpDelayedCloseElements > CmpDelayedCloseSize) &&
        !(CmpDelayCloseWorkItemActive))
    {
        /* Yes, we have too many elements to close, and no work item */
        CmpArmDelayedCloseTimer();
    }

    /* Release the table lock */
    KeReleaseGuardedMutex(&CmpDelayedCloseTableLock);
}

PCM_KEY_CONTROL_BLOCK
NTAPI
CmpAllocateKeyControlBlock(VOID)
{
    PLIST_ENTRY NextEntry;
    PCM_KEY_CONTROL_BLOCK CurrentKcb;
    PCM_ALLOC_PAGE AllocPage;
    ULONG i;
    PAGED_CODE();

    /* Check if private allocations are initialized */
    if (CmpAllocInited)
    {
        /* They are, acquire the bucket lock */
        KeAcquireGuardedMutex(&CmpAllocBucketLock);

        /* See if there's something on the free KCB list */
SearchKcbList:
        if (!IsListEmpty(&CmpFreeKCBListHead))
        {
            /* Remove the entry */
            NextEntry = RemoveHeadList(&CmpFreeKCBListHead);

            /* Get the KCB */
            CurrentKcb = CONTAINING_RECORD(NextEntry,
                                           CM_KEY_CONTROL_BLOCK,
                                           FreeListEntry);

            /* Get the allocation page */
            AllocPage = (PCM_ALLOC_PAGE)((ULONG_PTR)CurrentKcb & 0xFFFFF000);

            /* Decrease the free count */
            ASSERT(AllocPage->FreeCount != 0);
            AllocPage->FreeCount--;

            /* Make sure this KCB is privately allocated */
            ASSERT(CurrentKcb->PrivateAlloc == 1);

            /* Release the allocation lock */
            KeReleaseGuardedMutex(&CmpAllocBucketLock);

            /* Return the KCB */
            return CurrentKcb;
        }

        /* Allocate an allocation page */
        AllocPage = ExAllocatePoolWithTag(PagedPool, PAGE_SIZE, TAG_CM);
        if (AllocPage)
        {
            /* Set default entries */
            AllocPage->FreeCount = CM_KCBS_PER_PAGE;

            /* Loop each entry */
            for (i = 0; i < CM_KCBS_PER_PAGE; i++)
            {
                /* Get this entry */
                CurrentKcb = (PVOID)((ULONG_PTR)AllocPage +
                                     FIELD_OFFSET(CM_ALLOC_PAGE, AllocPage) +
                                     i * sizeof(CM_KEY_CONTROL_BLOCK));

                /* Set it up */
                CurrentKcb->PrivateAlloc = TRUE;
                CurrentKcb->DelayCloseEntry = NULL;
                InsertHeadList(&CmpFreeKCBListHead,
                               &CurrentKcb->FreeListEntry);
            }
            
            /* Now go back and search the list */
            goto SearchKcbList;
        }
    }

    /* Allocate a KCB only */
    CurrentKcb = ExAllocatePoolWithTag(PagedPool,
                                       sizeof(CM_KEY_CONTROL_BLOCK),
                                       TAG_CM);
    if (CurrentKcb)
    {
        /* Set it up */
        CurrentKcb->PrivateAlloc = 0;
        CurrentKcb->DelayCloseEntry = NULL;
    }

    /* Return it */
    return CurrentKcb;
}

VOID
NTAPI
CmpDereferenceKeyControlBlockWithLock(IN PCM_KEY_CONTROL_BLOCK Kcb,
                                      IN BOOLEAN LockHeldExclusively)
{
    /* Sanity check */
    ASSERT((CmpIsKcbLockedExclusive(Kcb) == TRUE) ||
           (CmpTestRegistryLockExclusive() == TRUE));

    /* Check if this is the last reference */
    if (InterlockedDecrement((PLONG)&Kcb->RefCount) == 0)
    {
        /* Check if we should do a direct delete */
        if (((CmpHoldLazyFlush) &&
             !(Kcb->ExtFlags & CM_KCB_SYM_LINK_FOUND) &&
             !(Kcb->Flags & KEY_SYM_LINK)) ||
            (Kcb->ExtFlags & CM_KCB_NO_DELAY_CLOSE) ||
            (Kcb->Delete))
        {
            /* Clean up the KCB*/
            CmpCleanUpKcbCacheWithLock(Kcb, LockHeldExclusively);
        }
        else
        {
            /* Otherwise, use delayed close */
            CmpAddToDelayedClose(Kcb, LockHeldExclusively);
        }
    }
}

VOID
NTAPI
InitializeKCBKeyBodyList(IN PCM_KEY_CONTROL_BLOCK Kcb)
{
    /* Initialize the list */
    InitializeListHead(&Kcb->KeyBodyListHead);

    /* Clear the bodies */
    Kcb->KeyBodyArray[0] =
    Kcb->KeyBodyArray[1] =
    Kcb->KeyBodyArray[2] =
    Kcb->KeyBodyArray[3] = NULL;
}

PCM_KEY_CONTROL_BLOCK
NTAPI
CmpCreateKeyControlBlock(IN PHHIVE Hive,
                         IN HCELL_INDEX Index,
                         IN PCM_KEY_NODE Node,
                         IN PCM_KEY_CONTROL_BLOCK Parent,
                         IN ULONG Flags,
                         IN PUNICODE_STRING KeyName)
{
    PCM_KEY_CONTROL_BLOCK Kcb, FoundKcb = NULL;
    UNICODE_STRING NodeName;
    ULONG ConvKey = 0, i;
    BOOLEAN IsFake, HashLock;
    PWCHAR p;

    /* Make sure we own this hive */
    ASSERT(FALSE);
    if (((PCMHIVE)Hive)->CreatorOwner != KeGetCurrentThread()) return NULL;

    /* Check if this is a fake KCB */
    IsFake = (BOOLEAN)(Flags & CMP_CREATE_FAKE_KCB);

    /* If we have a parent, use its ConvKey */
    if (Parent) ConvKey = Parent->ConvKey;

    /* Make a copy of the name */
    NodeName = *KeyName;

    /* Remove leading slash */
    while ((NodeName.Length) && (*NodeName.Buffer == OBJ_NAME_PATH_SEPARATOR))
    {
        /* Move the buffer by one */
        NodeName.Buffer++;
        NodeName.Length -= sizeof(WCHAR);
    }

    /* Make sure we didn't get just a slash or something */
    ASSERT(NodeName.Length > 0);

    /* Now setup the hash */
    p = NodeName.Buffer;
    for (i = 0; i < NodeName.Length; i += sizeof(WCHAR))
    {
        /* Make sure it's a valid character */
        if ((*p != OBJ_NAME_PATH_SEPARATOR) && (*p))
        {
            /* Add this key to the hash */
            ConvKey = 37 * ConvKey + RtlUpcaseUnicodeChar(*p);
        }

        /* Move on */
        p++;
    }

    /* Allocate the KCB */
    Kcb = CmpAllocateKeyControlBlock();
    if (!Kcb) return NULL;

    /* Initailize the key list */
    InitializeKCBKeyBodyList(Kcb);

    /* Set it up */
    Kcb->Delete = FALSE;
    Kcb->RefCount = 1;
    Kcb->KeyHive = Hive;
    Kcb->KeyCell = Index;
    Kcb->ConvKey = ConvKey;
    Kcb->DelayedCloseIndex = CmpDelayedCloseSize;

    /* Check if we have two hash entires */
    HashLock = (BOOLEAN)(Flags & CMP_LOCK_HASHES_FOR_KCB);
    if (HashLock)
    {
        /* Not yet implemented */
        KeBugCheck(0);
    }

    /* Check if we already have a KCB */
    FoundKcb = CmpInsertKeyHash(&Kcb->KeyHash, IsFake);
    if (FoundKcb)
    {
        /* Sanity check */
        ASSERT(!FoundKcb->Delete);

        /* Free the one we allocated and reference this one */
        CmpFreeKeyControlBlock(Kcb);
        Kcb = FoundKcb;
        if (!CmpReferenceKeyControlBlock(Kcb))
        {
            /* We got too many handles */
            ASSERT(Kcb->RefCount + 1 != 0);
            Kcb = NULL;
        }
        else
        {
            /* Check if we're not creating a fake one, but it used to be fake */
            if ((Kcb->ExtFlags & CM_KCB_KEY_NON_EXIST) && !(IsFake))
            {
                /* Set the hive and cell */
                Kcb->KeyHive = Hive;
                Kcb->KeyCell = Index;

                /* This means that our current information is invalid */
                Kcb->ExtFlags = CM_KCB_INVALID_CACHED_INFO;
            }

            /* Check if we didn't have any valid data */
            if (!(Kcb->ExtFlags & (CM_KCB_NO_SUBKEY |
                                   CM_KCB_SUBKEY_ONE |
                                   CM_KCB_SUBKEY_HINT)))
            {
                /* Calculate the index hint */
                Kcb->SubKeyCount = Node->SubKeyCounts[0] +
                                   Node->SubKeyCounts[1];

                /* Cached information is now valid */
                Kcb->ExtFlags &= ~CM_KCB_INVALID_CACHED_INFO;
            }

            /* Setup the other data */
            Kcb->KcbLastWriteTime = Node->LastWriteTime;
            Kcb->KcbMaxNameLen = (USHORT)Node->MaxNameLen;
            Kcb->KcbMaxValueNameLen = (USHORT)Node->MaxValueNameLen;
            Kcb->KcbMaxValueDataLen = Node->MaxValueDataLen;
        }
    }
    else
    {
        /* No KCB, do we have a parent? */
        if (Parent)
        {
            /* Reference the parent */
            if (((Parent->TotalLevels + 1) < 512) &&
                (CmpReferenceKeyControlBlock(Parent)))
            {
                /* Link it */
                Kcb->ParentKcb = Parent;
                Kcb->TotalLevels = Parent->TotalLevels + 1;
            }
            else
            {
                /* Remove the KCB and free it */
                CmpRemoveKeyControlBlock(Kcb);
                CmpFreeKeyControlBlock(Kcb);
                Kcb = NULL;
            }
        }
        else
        {
            /* No parent, this is the root node */
            Kcb->ParentKcb = NULL;
            Kcb->TotalLevels = 1;
        }

        /* Check if we have a KCB */
        if (Kcb)
        {
            /* Get the NCB */
            Kcb->NameBlock = CmpGetNcb(&NodeName);
            if (Kcb->NameBlock)
            {
                /* Fill it out */
                Kcb->ValueCache.Count = Node->ValueList.Count;
                Kcb->ValueCache.ValueList = Node->ValueList.List;
                Kcb->Flags = Node->Flags;
                Kcb->ExtFlags = 0;
                Kcb->DelayedCloseIndex = CmpDelayedCloseSize;

                /* Remember if this is a fake key */
                if (IsFake) Kcb->ExtFlags |= CM_KCB_KEY_NON_EXIST;

                /* Setup the other data */
                Kcb->SubKeyCount = Node->SubKeyCounts[0] +
                                   Node->SubKeyCounts[1];
                Kcb->KcbLastWriteTime = Node->LastWriteTime;
                Kcb->KcbMaxNameLen = (USHORT)Node->MaxNameLen;
                Kcb->KcbMaxValueNameLen = (USHORT)Node->MaxValueNameLen;
                Kcb->KcbMaxValueDataLen = (USHORT)Node->MaxValueDataLen;
            }
            else
            {
                /* Dereference the KCB */
                CmpDereferenceKeyControlBlockWithLock(Parent, FALSE);

                /* Remove the KCB and free it */
                CmpRemoveKeyControlBlock(Kcb);
                CmpFreeKeyControlBlock(Kcb);
                Kcb = NULL;
            }
        }
    }

    /* Check if we had locked the hashes */
    if (HashLock)
    {
        /* Not yet implemented: Unlock hashes */
        KeBugCheck(0);
    }

    /* Return the KCB */
    return Kcb;
}

VOID
NTAPI
EnlistKeyBodyWithKCB(IN PCM_KEY_BODY KeyBody,
                     IN ULONG Flags)
{
    ASSERT(FALSE);
}

