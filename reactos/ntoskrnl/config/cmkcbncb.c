/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/config/cmkcbncb.c
 * PURPOSE:         Configuration Manager - Key Control and Name Control Blocks
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "ntoskrnl.h"
#include "cm.h"
#define NDEBUG
#include "debug.h"

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
}

VOID
NTAPI
CmpInitializeCache(VOID)
{
    ULONG Length;

    /* Calculate length for the table */
    Length = CmpHashTableSize * sizeof(PCM_KEY_HASH);

    /* Allocate it */
    CmpCacheTable = ExAllocatePoolWithTag(PagedPool, Length, TAG_CM);
    if (!CmpCacheTable) KeBugCheck(CONFIG_INITIALIZATION_FAILED);
    RtlZeroMemory(CmpCacheTable, Length);

    /* Calculate length for the name cache */
    Length = CmpHashTableSize * sizeof(PCM_NAME_HASH);

    /* Now allocate the name cache table */
    CmpNameCacheTable = ExAllocatePoolWithTag(PagedPool, Length, TAG_CM);
    if (!CmpCacheTable) KeBugCheck(CONFIG_INITIALIZATION_FAILED);
    RtlZeroMemory(CmpNameCacheTable, Length);

    /* Setup the delayed close lock */
    KeInitializeGuardedMutex(&CmpDelayedCloseTableLock);

    /* Setup the work item */
    ExInitializeWorkItem(&CmpDelayCloseWorkItem, CmpDelayCloseWorker, NULL);

    /* Setup the DPC and its timer */
    KeInitializeDpc(&CmpDelayCloseDpc, CmpDelayCloseDpcRoutine, NULL);
    KeInitializeTimer(&CmpDelayCloseTimer);
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

PCM_KEY_CONTROL_BLOCK
NTAPI
CmpCreateKeyControlBlock(IN PHHIVE Hive,
                         IN HCELL_INDEX Index,
                         IN PCM_KEY_NODE Node,
                         IN PCM_KEY_CONTROL_BLOCK Parent,
                         IN ULONG Flags,
                         IN PUNICODE_STRING KeyName)
{
    /* Temporary hack */
    return (PVOID)1;
}
