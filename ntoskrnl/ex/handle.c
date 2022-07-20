/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ex/handle.c
 * PURPOSE:         Generic Executive Handle Tables
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Thomas Weidenmueller <w3seek@reactos.com>
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

LIST_ENTRY HandleTableListHead;
EX_PUSH_LOCK HandleTableListLock;
#define SizeOfHandle(x) (sizeof(HANDLE) * (x))
#define INDEX_TO_HANDLE_VALUE(x) ((x) << HANDLE_TAG_BITS)

/* PRIVATE FUNCTIONS *********************************************************/

CODE_SEG("INIT")
VOID
NTAPI
ExpInitializeHandleTables(VOID)
{
    /* Initialize the list of handle tables and the lock */
    InitializeListHead(&HandleTableListHead);
    ExInitializePushLock(&HandleTableListLock);
}

PHANDLE_TABLE_ENTRY
NTAPI
ExpLookupHandleTableEntry(IN PHANDLE_TABLE HandleTable,
                          IN EXHANDLE Handle)
{
    ULONG TableLevel;
    ULONG_PTR TableBase;
    PHANDLE_TABLE_ENTRY HandleArray, Entry;
    PVOID *PointerArray;

    /* Clear the tag bits */
    Handle.TagBits = 0;

    /* Check if the handle is in the allocated range */
    if (Handle.Value >= HandleTable->NextHandleNeedingPool)
    {
        return NULL;
    }

    /* Get the table code */
    TableBase = HandleTable->TableCode;

    /* Extract the table level and actual table base */
    TableLevel = (ULONG)(TableBase & 3);
    TableBase &= ~3;

    PointerArray = (PVOID*)TableBase;
    HandleArray = (PHANDLE_TABLE_ENTRY)TableBase;

    /* Check what level we're running at */
    switch (TableLevel)
    {
        case 2:

            /* Get the mid level pointer array */
            PointerArray = PointerArray[Handle.HighIndex];
            ASSERT(PointerArray != NULL);

            /* Fall through */
        case 1:

            /* Get the handle array */
            HandleArray = PointerArray[Handle.MidIndex];
            ASSERT(HandleArray != NULL);

            /* Fall through */
        case 0:

            /* Get the entry using the low index */
            Entry = &HandleArray[Handle.LowIndex];

            /* All done */
            break;

        default:

            ASSERT(FALSE);
            Entry = NULL;
    }

    /* Return the handle entry */
    return Entry;
}

PVOID
NTAPI
ExpAllocateTablePagedPool(IN PEPROCESS Process OPTIONAL,
                          IN SIZE_T Size)
{
    PVOID Buffer;
    NTSTATUS Status;

    /* Do the allocation */
    Buffer = ExAllocatePoolWithTag(PagedPool, Size, TAG_OBJECT_TABLE);
    if (Buffer)
    {
        /* Clear the memory */
        RtlZeroMemory(Buffer, Size);

        /* Check if we have a process to charge quota */
        if (Process)
        {
            /* Charge quota */
            Status = PsChargeProcessPagedPoolQuota(Process, Size);
            if (!NT_SUCCESS(Status))
            {
                ExFreePoolWithTag(Buffer, TAG_OBJECT_TABLE);
                return NULL;
            }
        }
    }

    /* Return the allocated memory */
    return Buffer;
}

PVOID
NTAPI
ExpAllocateTablePagedPoolNoZero(IN PEPROCESS Process OPTIONAL,
                                IN SIZE_T Size)
{
    PVOID Buffer;
    NTSTATUS Status;

    /* Do the allocation */
    Buffer = ExAllocatePoolWithTag(PagedPool, Size, TAG_OBJECT_TABLE);
    if (Buffer)
    {
        /* Check if we have a process to charge quota */
        if (Process)
        {
            /* Charge quota */
            Status = PsChargeProcessPagedPoolQuota(Process, Size);
            if (!NT_SUCCESS(Status))
            {
                ExFreePoolWithTag(Buffer, TAG_OBJECT_TABLE);
                return NULL;
            }
        }
    }

    /* Return the allocated memory */
    return Buffer;
}

VOID
NTAPI
ExpFreeTablePagedPool(IN PEPROCESS Process OPTIONAL,
                      IN PVOID Buffer,
                      IN SIZE_T Size)
{
    /* Free the buffer */
    ExFreePoolWithTag(Buffer, TAG_OBJECT_TABLE);
    if (Process)
    {
        /* Release quota */
        PsReturnProcessPagedPoolQuota(Process, Size);
    }
}

VOID
NTAPI
ExpFreeLowLevelTable(IN PEPROCESS Process,
                     IN PHANDLE_TABLE_ENTRY TableEntry)
{
    /* Check if we have an entry */
    if (TableEntry[0].Object)
    {
        /* Free the entry */
        ExpFreeTablePagedPool(Process,
                              TableEntry[0].Object,
                              LOW_LEVEL_ENTRIES *
                              sizeof(HANDLE_TABLE_ENTRY_INFO));
    }

    /* Free the table */
    ExpFreeTablePagedPool(Process, TableEntry, PAGE_SIZE);
}

VOID
NTAPI
ExpFreeHandleTable(IN PHANDLE_TABLE HandleTable)
{
    PEPROCESS Process = HandleTable->QuotaProcess;
    ULONG i, j;
    ULONG_PTR TableCode = HandleTable->TableCode;
    ULONG_PTR TableBase = TableCode & ~3;
    ULONG TableLevel = (ULONG)(TableCode & 3);
    PHANDLE_TABLE_ENTRY Level1, *Level2, **Level3;
    PAGED_CODE();

    /* Check which level we're at */
    if (TableLevel == 0)
    {
        /* Select the first level table base and just free it */
        Level1 = (PVOID)TableBase;
        ExpFreeLowLevelTable(Process, Level1);
    }
    else if (TableLevel == 1)
    {
        /* Select the second level table base */
        Level2 = (PVOID)TableBase;

        /* Loop each mid level entry */
        for (i = 0; i < MID_LEVEL_ENTRIES; i++)
        {
            /* Leave if we've reached the last entry */
            if (!Level2[i]) break;

            /* Free the second level table */
            ExpFreeLowLevelTable(Process, Level2[i]);
        }

        /* Free the second level table */
        ExpFreeTablePagedPool(Process, Level2, PAGE_SIZE);
    }
    else
    {
        /* Select the third level table base */
        Level3 = (PVOID)TableBase;

        /* Loop each high level entry */
        for (i = 0; i < HIGH_LEVEL_ENTRIES; i++)
        {
            /* Leave if we've reached the last entry */
            if (!Level3[i]) break;

            /* Loop each mid level entry */
            for (j = 0; j < MID_LEVEL_ENTRIES; j++)
            {
                /* Leave if we've reached the last entry */
                if (!Level3[i][j]) break;

                /* Free the second level table */
                ExpFreeLowLevelTable(Process, Level3[i][j]);
            }

            /* Free the third level table entry */
            ExpFreeTablePagedPool(Process, Level3[i], PAGE_SIZE);
        }

        /* Free the third level table */
        ExpFreeTablePagedPool(Process,
                              Level3,
                              SizeOfHandle(HIGH_LEVEL_ENTRIES));
    }

    /* Free the actual table and check if we need to release quota */
    ExFreePoolWithTag(HandleTable, TAG_OBJECT_TABLE);
    if (Process)
    {
        /* Release the quota it was taking up */
        PsReturnProcessPagedPoolQuota(Process, sizeof(HANDLE_TABLE));
    }
}

VOID
NTAPI
ExpFreeHandleTableEntry(IN PHANDLE_TABLE HandleTable,
                        IN EXHANDLE Handle,
                        IN PHANDLE_TABLE_ENTRY HandleTableEntry)
{
    ULONG OldValue, *Free;
    ULONG LockIndex;
    PAGED_CODE();

    /* Sanity checks */
    ASSERT(HandleTableEntry->Object == NULL);
    ASSERT(HandleTableEntry == ExpLookupHandleTableEntry(HandleTable, Handle));

    /* Decrement the handle count */
    InterlockedDecrement(&HandleTable->HandleCount);

    /* Mark the handle as free */
    Handle.TagBits = 0;

    /* Check if we're FIFO */
    if (!HandleTable->StrictFIFO)
    {
        /* Select a lock index */
        LockIndex = Handle.Index % 4;

        /* Select which entry to use */
        Free = (HandleTable->HandleTableLock[LockIndex].Locked) ?
                &HandleTable->FirstFree : &HandleTable->LastFree;
    }
    else
    {
        /* No need to worry about locking, take the last entry */
        Free = &HandleTable->LastFree;
    }

    /* Start value change loop */
    for (;;)
    {
        /* Get the current value and write */
        OldValue = *Free;
        HandleTableEntry->NextFreeTableEntry = OldValue;
        if (InterlockedCompareExchange((PLONG)Free, Handle.AsULONG, OldValue) == OldValue)
        {
            /* Break out, we're done. Make sure the handle value makes sense */
            ASSERT((OldValue & FREE_HANDLE_MASK) <
                   HandleTable->NextHandleNeedingPool);
            break;
        }
    }
}

PHANDLE_TABLE
NTAPI
ExpAllocateHandleTable(IN PEPROCESS Process OPTIONAL,
                       IN BOOLEAN NewTable)
{
    PHANDLE_TABLE HandleTable;
    PHANDLE_TABLE_ENTRY HandleTableTable, HandleEntry;
    ULONG i;
    NTSTATUS Status;
    PAGED_CODE();

    /* Allocate the table */
    HandleTable = ExAllocatePoolWithTag(PagedPool,
                                        sizeof(HANDLE_TABLE),
                                        TAG_OBJECT_TABLE);
    if (!HandleTable) return NULL;

    /* Check if we have a process */
    if (Process)
    {
        /* Charge quota */
        Status = PsChargeProcessPagedPoolQuota(Process, sizeof(HANDLE_TABLE));
        if (!NT_SUCCESS(Status))
        {
            ExFreePoolWithTag(HandleTable, TAG_OBJECT_TABLE);
            return NULL;
        }
    }

    /* Clear the table */
    RtlZeroMemory(HandleTable, sizeof(HANDLE_TABLE));

    /* Now allocate the first level structures */
    HandleTableTable = ExpAllocateTablePagedPoolNoZero(Process, PAGE_SIZE);
    if (!HandleTableTable)
    {
        /* Failed, free the table */
        ExFreePoolWithTag(HandleTable, TAG_OBJECT_TABLE);

        /* Return the quota it was taking up */
        if (Process)
        {
            PsReturnProcessPagedPoolQuota(Process, sizeof(HANDLE_TABLE));
        }

        return NULL;
    }

    /* Write the pointer to our first level structures */
    HandleTable->TableCode = (ULONG_PTR)HandleTableTable;

    /* Initialize the first entry */
    HandleEntry = &HandleTableTable[0];
    HandleEntry->NextFreeTableEntry = -2;
    HandleEntry->Value = 0;

    /* Check if this is a new table */
    if (NewTable)
    {
        /* Go past the root entry */
        HandleEntry++;

        /* Loop every low level entry */
        for (i = 1; i < (LOW_LEVEL_ENTRIES - 1); i++)
        {
            /* Set up the free data */
            HandleEntry->Value = 0;
            HandleEntry->NextFreeTableEntry = INDEX_TO_HANDLE_VALUE(i + 1);

            /* Move to the next entry */
            HandleEntry++;
        }

        /* Terminate the last entry */
        HandleEntry->Value = 0;
        HandleEntry->NextFreeTableEntry = 0;
        HandleTable->FirstFree = INDEX_TO_HANDLE_VALUE(1);
    }

    /* Set the next handle needing pool after our allocated page from above */
    HandleTable->NextHandleNeedingPool = INDEX_TO_HANDLE_VALUE(LOW_LEVEL_ENTRIES);

    /* Setup the rest of the handle table data */
    HandleTable->QuotaProcess = Process;
    HandleTable->UniqueProcessId = PsGetCurrentProcess()->UniqueProcessId;
    HandleTable->Flags = 0;

    /* Loop all the handle table locks */
    for (i = 0; i < 4; i++)
    {
        /* Initialize the handle table lock */
        ExInitializePushLock(&HandleTable->HandleTableLock[i]);
    }

    /* Initialize the contention event lock and return the lock */
    ExInitializePushLock(&HandleTable->HandleContentionEvent);
    return HandleTable;
}

PHANDLE_TABLE_ENTRY
NTAPI
ExpAllocateLowLevelTable(IN PHANDLE_TABLE HandleTable,
                         IN BOOLEAN DoInit)
{
    ULONG i, Base;
    PHANDLE_TABLE_ENTRY Low, HandleEntry;

    /* Allocate the low level table */
    Low = ExpAllocateTablePagedPoolNoZero(HandleTable->QuotaProcess,
                                          PAGE_SIZE);
    if (!Low) return NULL;

    /* Setup the initial entry */
    HandleEntry = &Low[0];
    HandleEntry->NextFreeTableEntry = -2;
    HandleEntry->Value = 0;

    /* Check if we're initializing */
    if (DoInit)
    {
        /* Go to the next entry and the base entry */
        HandleEntry++;
        Base = HandleTable->NextHandleNeedingPool + INDEX_TO_HANDLE_VALUE(2);

        /* Loop each entry */
        for (i = Base;
             i < Base + INDEX_TO_HANDLE_VALUE(LOW_LEVEL_ENTRIES - 2);
             i += INDEX_TO_HANDLE_VALUE(1))
        {
            /* Free this entry and move on to the next one */
            HandleEntry->NextFreeTableEntry = i;
            HandleEntry->Value = 0;
            HandleEntry++;
        }

        /* Terminate the last entry */
        HandleEntry->NextFreeTableEntry = 0;
        HandleEntry->Value = 0;
    }

    /* Return the low level table */
    return Low;
}

PHANDLE_TABLE_ENTRY*
NTAPI
ExpAllocateMidLevelTable(IN PHANDLE_TABLE HandleTable,
                         IN BOOLEAN DoInit,
                         OUT PHANDLE_TABLE_ENTRY *LowTableEntry)
{
    PHANDLE_TABLE_ENTRY *Mid, Low;

    /* Allocate the mid level table */
    Mid = ExpAllocateTablePagedPool(HandleTable->QuotaProcess, PAGE_SIZE);
    if (!Mid) return NULL;

    /* Allocate a new low level for it */
    Low = ExpAllocateLowLevelTable(HandleTable, DoInit);
    if (!Low)
    {
        /* We failed, free the mid table */
        ExpFreeTablePagedPool(HandleTable->QuotaProcess, Mid, PAGE_SIZE);
        return NULL;
    }

    /* Link the tables and return the pointer */
    Mid[0] = Low;
    *LowTableEntry = Low;
    return Mid;
}

BOOLEAN
NTAPI
ExpAllocateHandleTableEntrySlow(IN PHANDLE_TABLE HandleTable,
                                IN BOOLEAN DoInit)
{
    ULONG i, j, Index;
    PHANDLE_TABLE_ENTRY Low = NULL, *Mid, **High, *SecondLevel, **ThirdLevel;
    ULONG NewFree, FirstFree;
    PVOID Value;
    ULONG_PTR TableCode = HandleTable->TableCode;
    ULONG_PTR TableBase = TableCode & ~3;
    ULONG TableLevel = (ULONG)(TableCode & 3);
    PAGED_CODE();

    /* Check how many levels we already have */
    if (TableLevel == 0)
    {
        /* Allocate a mid level, since we only have a low level */
        Mid = ExpAllocateMidLevelTable(HandleTable, DoInit, &Low);
        if (!Mid) return FALSE;

        /* Link up the tables */
        Mid[1] = Mid[0];
        Mid[0] = (PVOID)TableBase;

        /* Write the new level and attempt to change the table code */
        TableBase = ((ULONG_PTR)Mid) | 1;
        Value = InterlockedExchangePointer((PVOID*)&HandleTable->TableCode, (PVOID)TableBase);
    }
    else if (TableLevel == 1)
    {
        /* Setup the 2nd level table */
        SecondLevel = (PVOID)TableBase;

        /* Get if the next index can fit in the table */
        i = HandleTable->NextHandleNeedingPool /
            INDEX_TO_HANDLE_VALUE(LOW_LEVEL_ENTRIES);
        if (i < MID_LEVEL_ENTRIES)
        {
            /* We need to allocate a new table */
            Low = ExpAllocateLowLevelTable(HandleTable, DoInit);
            if (!Low) return FALSE;

            /* Update the table */
            Value = InterlockedExchangePointer((PVOID*)&SecondLevel[i], Low);
            ASSERT(Value == NULL);
        }
        else
        {
            /* We need a new high level table */
            High = ExpAllocateTablePagedPool(HandleTable->QuotaProcess,
                                             SizeOfHandle(HIGH_LEVEL_ENTRIES));
            if (!High) return FALSE;

            /* Allocate a new mid level table as well */
            Mid = ExpAllocateMidLevelTable(HandleTable, DoInit, &Low);
            if (!Mid)
            {
                /* We failed, free the high level table as well */
                ExpFreeTablePagedPool(HandleTable->QuotaProcess,
                                      High,
                                      SizeOfHandle(HIGH_LEVEL_ENTRIES));
                return FALSE;
            }

            /* Link up the tables */
            High[0] = (PVOID)TableBase;
            High[1] = Mid;

            /* Write the new table and change the table code */
            TableBase = ((ULONG_PTR)High) | 2;
            Value = InterlockedExchangePointer((PVOID*)&HandleTable->TableCode,
                                               (PVOID)TableBase);
        }
    }
    else if (TableLevel == 2)
    {
        /* Setup the 3rd level table */
        ThirdLevel = (PVOID)TableBase;

        /* Get the index and check if it can fit */
        i = HandleTable->NextHandleNeedingPool / INDEX_TO_HANDLE_VALUE(MAX_MID_INDEX);
        if (i >= HIGH_LEVEL_ENTRIES) return FALSE;

        /* Check if there's no mid-level table */
        if (!ThirdLevel[i])
        {
            /* Allocate a new mid level table */
            Mid = ExpAllocateMidLevelTable(HandleTable, DoInit, &Low);
            if (!Mid) return FALSE;

            /* Update the table pointer */
            Value = InterlockedExchangePointer((PVOID*)&ThirdLevel[i], Mid);
            ASSERT(Value == NULL);
        }
        else
        {
            /* We have one, check at which index we should insert our entry */
            Index = (HandleTable->NextHandleNeedingPool / INDEX_TO_HANDLE_VALUE(1)) -
                     i * MAX_MID_INDEX;
            j = Index / LOW_LEVEL_ENTRIES;

            /* Allocate a new low level */
            Low = ExpAllocateLowLevelTable(HandleTable, DoInit);
            if (!Low) return FALSE;

            /* Update the table pointer */
            Value = InterlockedExchangePointer((PVOID*)&ThirdLevel[i][j], Low);
            ASSERT(Value == NULL);
        }
    }
    else
    {
        /* Something is really broken */
        ASSERT(FALSE);
    }

    /* Update the index of the next handle */
    Index = InterlockedExchangeAdd((PLONG) &HandleTable->NextHandleNeedingPool,
                                   INDEX_TO_HANDLE_VALUE(LOW_LEVEL_ENTRIES));

    /* Check if need to initialize the table */
    if (DoInit)
    {
        /* Create a new index number */
        Index += INDEX_TO_HANDLE_VALUE(1);

        /* Start free index change loop */
        for (;;)
        {
            /* Setup the first free index */
            FirstFree = HandleTable->FirstFree;
            Low[LOW_LEVEL_ENTRIES - 1].NextFreeTableEntry = FirstFree;

            /* Change the index */
            NewFree = InterlockedCompareExchange((PLONG) &HandleTable->FirstFree,
                                                 Index,
                                                 FirstFree);
            if (NewFree == FirstFree) break;
        }
    }

    /* All done */
    return TRUE;
}

ULONG
NTAPI
ExpMoveFreeHandles(IN PHANDLE_TABLE HandleTable)
{
    ULONG LastFree, i;

    /* Clear the last free index */
    LastFree = InterlockedExchange((PLONG) &HandleTable->LastFree, 0);

    /* Check if we had no index */
    if (!LastFree) return LastFree;

    /* Acquire the locks we need */
    for (i = 1; i < 4; i++)
    {
        /* Acquire this lock exclusively */
        ExWaitOnPushLock(&HandleTable->HandleTableLock[i]);
    }

    /* Check if we're not strict FIFO */
    if (!HandleTable->StrictFIFO)
    {
        /* Update the first free index */
        if (!InterlockedCompareExchange((PLONG) &HandleTable->FirstFree, LastFree, 0))
        {
            /* We're done, exit */
            return LastFree;
        }
    }

    /* We are strict FIFO, we need to reverse the entries */
    ASSERT(FALSE);
    return LastFree;
}

PHANDLE_TABLE_ENTRY
NTAPI
ExpAllocateHandleTableEntry(IN PHANDLE_TABLE HandleTable,
                            OUT PEXHANDLE NewHandle)
{
    ULONG OldValue, NewValue, NewValue1;
    PHANDLE_TABLE_ENTRY Entry;
    EXHANDLE Handle, OldHandle;
    BOOLEAN Result;
    ULONG i;

    /* Start allocation loop */
    for (;;)
    {
        /* Get the current link */
        OldValue = HandleTable->FirstFree;
        while (!OldValue)
        {
            /* No free entries remain, lock the handle table */
            KeEnterCriticalRegion();
            ExAcquirePushLockExclusive(&HandleTable->HandleTableLock[0]);

            /* Check the value again */
            OldValue = HandleTable->FirstFree;
            if (OldValue)
            {
                /* Another thread has already created a new level, bail out */
                ExReleasePushLockExclusive(&HandleTable->HandleTableLock[0]);
                KeLeaveCriticalRegion();
                break;
            }

            /* Now move any free handles */
            OldValue = ExpMoveFreeHandles(HandleTable);
            if (OldValue)
            {
                /* Another thread has already moved them, bail out */
                ExReleasePushLockExclusive(&HandleTable->HandleTableLock[0]);
                KeLeaveCriticalRegion();
                break;
            }

            /* We're the first one through, so do the actual allocation */
            Result = ExpAllocateHandleTableEntrySlow(HandleTable, TRUE);

            /* Unlock the table and get the value now */
            ExReleasePushLockExclusive(&HandleTable->HandleTableLock[0]);
            KeLeaveCriticalRegion();
            OldValue = HandleTable->FirstFree;

            /* Check if allocation failed */
            if (!Result)
            {
                /* Check if nobody else went through here */
                if (!OldValue)
                {
                    /* We're still the only thread around, so fail */
                    NewHandle->GenericHandleOverlay = NULL;
                    return NULL;
                }
            }
        }

        /* We made it, write the current value */
        Handle.Value = (OldValue & FREE_HANDLE_MASK);

        /* Lookup the entry for this handle */
        Entry = ExpLookupHandleTableEntry(HandleTable, Handle);

        /* Get an available lock and acquire it */
        OldHandle.Value = OldValue;
        i = OldHandle.Index % 4;
        KeEnterCriticalRegion();
        ExAcquirePushLockShared(&HandleTable->HandleTableLock[i]);

        /* Check if the value changed after acquiring the lock */
        if (OldValue != *(volatile ULONG*)&HandleTable->FirstFree)
        {
            /* It did, so try again */
            ExReleasePushLockShared(&HandleTable->HandleTableLock[i]);
            KeLeaveCriticalRegion();
            continue;
        }

        /* Now get the next value and do the compare */
        NewValue = *(volatile ULONG*)&Entry->NextFreeTableEntry;
        NewValue1 = InterlockedCompareExchange((PLONG) &HandleTable->FirstFree,
                                               NewValue,
                                               OldValue);

        /* The change was done, so release the lock */
        ExReleasePushLockShared(&HandleTable->HandleTableLock[i]);
        KeLeaveCriticalRegion();

        /* Check if the compare was successful */
        if (NewValue1 == OldValue)
        {
            /* Make sure that the new handle is in range, and break out */
            ASSERT((NewValue & FREE_HANDLE_MASK) <
                   HandleTable->NextHandleNeedingPool);
            break;
        }
        else
        {
            /* The compare failed, make sure we expected it */
            ASSERT((NewValue1 & FREE_HANDLE_MASK) !=
                   (OldValue & FREE_HANDLE_MASK));
        }
    }

    /* Increase the number of handles */
    InterlockedIncrement(&HandleTable->HandleCount);

    /* Return the handle and the entry */
    *NewHandle = Handle;
    return Entry;
}

PHANDLE_TABLE
NTAPI
ExCreateHandleTable(IN PEPROCESS Process OPTIONAL)
{
    PHANDLE_TABLE HandleTable;
    PAGED_CODE();

    /* Allocate the handle table */
    HandleTable = ExpAllocateHandleTable(Process, TRUE);
    if (!HandleTable) return NULL;

    /* Acquire the handle table lock */
    KeEnterCriticalRegion();
    ExAcquirePushLockExclusive(&HandleTableListLock);

    /* Insert it into the list */
    InsertTailList(&HandleTableListHead, &HandleTable->HandleTableList);

    /* Release the lock */
    ExReleasePushLockExclusive(&HandleTableListLock);
    KeLeaveCriticalRegion();

    /* Return the handle table */
    return HandleTable;
}

HANDLE
NTAPI
ExCreateHandle(IN PHANDLE_TABLE HandleTable,
               IN PHANDLE_TABLE_ENTRY HandleTableEntry)
{
    EXHANDLE Handle;
    PHANDLE_TABLE_ENTRY NewEntry;
    PAGED_CODE();

    /* Start with a clean handle */
    Handle.GenericHandleOverlay = NULL;

    /* Allocate a new entry */
    NewEntry = ExpAllocateHandleTableEntry(HandleTable, &Handle);
    if (NewEntry)
    {
        /* Enter a critical region */
        KeEnterCriticalRegion();

        /* Write the entry */
        *NewEntry = *HandleTableEntry;

        /* Unlock it and leave the critical region */
        ExUnlockHandleTableEntry(HandleTable, NewEntry);
        KeLeaveCriticalRegion();
    }

    /* Return the handle value */
    return Handle.GenericHandleOverlay;
}

VOID
NTAPI
ExpBlockOnLockedHandleEntry(IN PHANDLE_TABLE HandleTable,
                            IN PHANDLE_TABLE_ENTRY HandleTableEntry)
{
    LONG_PTR OldValue;
    EX_PUSH_LOCK_WAIT_BLOCK WaitBlock;

    /* Block on the pushlock */
    ExBlockPushLock(&HandleTable->HandleContentionEvent, &WaitBlock);

    /* Get the current value and check if it's been unlocked */
    OldValue = HandleTableEntry->Value;
    if (!(OldValue) || (OldValue & EXHANDLE_TABLE_ENTRY_LOCK_BIT))
    {
        /* Unblock the pushlock and return */
        ExfUnblockPushLock(&HandleTable->HandleContentionEvent, &WaitBlock);
    }
    else
    {
        /* Wait for it to be unblocked */
        ExWaitForUnblockPushLock(&HandleTable->HandleContentionEvent,
                                 &WaitBlock);
    }
}

BOOLEAN
NTAPI
ExpLockHandleTableEntry(IN PHANDLE_TABLE HandleTable,
                        IN PHANDLE_TABLE_ENTRY HandleTableEntry)
{
    LONG_PTR NewValue, OldValue;

    /* Sanity check */
    ASSERT((KeGetCurrentThread()->CombinedApcDisable != 0) ||
           (KeGetCurrentIrql() == APC_LEVEL));

    /* Start lock loop */
    for (;;)
    {
        /* Get the current value and check if it's locked */
        OldValue = *(volatile LONG_PTR *)&HandleTableEntry->Object;
        if (OldValue & EXHANDLE_TABLE_ENTRY_LOCK_BIT)
        {
            /* It's not locked, remove the lock bit to lock it */
            NewValue = OldValue & ~EXHANDLE_TABLE_ENTRY_LOCK_BIT;
            if (InterlockedCompareExchangePointer(&HandleTableEntry->Object,
                                                  (PVOID)NewValue,
                                                  (PVOID)OldValue) == (PVOID)OldValue)
            {
                /* We locked it, get out */
                return TRUE;
            }
        }
        else
        {
            /* We couldn't lock it, bail out if it's been freed */
            if (!OldValue) return FALSE;
        }

        /* It's locked, wait for it to be unlocked */
        ExpBlockOnLockedHandleEntry(HandleTable, HandleTableEntry);
    }
}

VOID
NTAPI
ExUnlockHandleTableEntry(IN PHANDLE_TABLE HandleTable,
                         IN PHANDLE_TABLE_ENTRY HandleTableEntry)
{
    LONG_PTR OldValue;
    PAGED_CODE();

    /* Sanity check */
    ASSERT((KeGetCurrentThread()->CombinedApcDisable != 0) ||
           (KeGetCurrentIrql() == APC_LEVEL));

    /* Set the lock bit and make sure it wasn't earlier */
    OldValue = InterlockedOr((PLONG) &HandleTableEntry->Value,
                             EXHANDLE_TABLE_ENTRY_LOCK_BIT);
    ASSERT((OldValue & EXHANDLE_TABLE_ENTRY_LOCK_BIT) == 0);

    /* Unblock any waiters */
    ExfUnblockPushLock(&HandleTable->HandleContentionEvent, NULL);
}

VOID
NTAPI
ExRemoveHandleTable(IN PHANDLE_TABLE HandleTable)
{
    PAGED_CODE();

    /* Acquire the table lock */
    KeEnterCriticalRegion();
    ExAcquirePushLockExclusive(&HandleTableListLock);

    /* Remove the table and reset the list */
    RemoveEntryList(&HandleTable->HandleTableList);
    InitializeListHead(&HandleTable->HandleTableList);

    /* Release the lock */
    ExReleasePushLockExclusive(&HandleTableListLock);
    KeLeaveCriticalRegion();
}

VOID
NTAPI
ExDestroyHandleTable(IN PHANDLE_TABLE HandleTable,
                     IN PVOID DestroyHandleProcedure OPTIONAL)
{
    PAGED_CODE();

    /* Remove the handle from the list */
    ExRemoveHandleTable(HandleTable);

    /* Check if we have a destroy callback */
    if (DestroyHandleProcedure)
    {
        /* FIXME: */
        ASSERT(FALSE);
    }

    /* Free the handle table */
    ExpFreeHandleTable(HandleTable);
}

BOOLEAN
NTAPI
ExDestroyHandle(IN PHANDLE_TABLE HandleTable,
                IN HANDLE Handle,
                IN PHANDLE_TABLE_ENTRY HandleTableEntry OPTIONAL)
{
    EXHANDLE ExHandle;
    PVOID Object;
    PAGED_CODE();

    /* Setup the actual handle value */
    ExHandle.GenericHandleOverlay = Handle;

    /* Enter a critical region and check if we have to lookup the handle */
    KeEnterCriticalRegion();
    if (!HandleTableEntry)
    {
        /* Lookup the entry */
        HandleTableEntry = ExpLookupHandleTableEntry(HandleTable, ExHandle);

        /* Make sure that we found an entry, and that it's valid */
        if (!(HandleTableEntry) ||
            !(HandleTableEntry->Object) ||
            (HandleTableEntry->NextFreeTableEntry == -2))
        {
            /* Invalid handle, fail */
            KeLeaveCriticalRegion();
            return FALSE;
        }

        /* Lock the entry */
        if (!ExpLockHandleTableEntry(HandleTable, HandleTableEntry))
        {
            /* Couldn't lock, fail */
            KeLeaveCriticalRegion();
            return FALSE;
        }
    }
    else
    {
        /* Make sure the handle is locked */
        ASSERT((HandleTableEntry->Value & EXHANDLE_TABLE_ENTRY_LOCK_BIT) == 0);
    }

    /* Clear the handle */
    Object = InterlockedExchangePointer((PVOID*)&HandleTableEntry->Object, NULL);

    /* Sanity checks */
    ASSERT(Object != NULL);
    ASSERT((((ULONG_PTR)Object) & EXHANDLE_TABLE_ENTRY_LOCK_BIT) == 0);

    /* Unblock the pushlock */
    ExfUnblockPushLock(&HandleTable->HandleContentionEvent, NULL);

    /* Free the actual entry */
    ExpFreeHandleTableEntry(HandleTable, ExHandle, HandleTableEntry);

    /* If we got here, return success */
    KeLeaveCriticalRegion();
    return TRUE;
}

PHANDLE_TABLE_ENTRY
NTAPI
ExMapHandleToPointer(IN PHANDLE_TABLE HandleTable,
                     IN HANDLE Handle)
{
    EXHANDLE ExHandle;
    PHANDLE_TABLE_ENTRY HandleTableEntry;
    PAGED_CODE();

    /* Set the handle value */
    ExHandle.GenericHandleOverlay = Handle;

    /* Fail if we got an invalid index */
    if (!(ExHandle.Index & (LOW_LEVEL_ENTRIES - 1))) return NULL;

    /* Do the lookup */
    HandleTableEntry = ExpLookupHandleTableEntry(HandleTable, ExHandle);
    if (!HandleTableEntry) return NULL;

    /* Lock it */
    if (!ExpLockHandleTableEntry(HandleTable, HandleTableEntry)) return NULL;

    /* Return the entry */
    return HandleTableEntry;
}

PHANDLE_TABLE
NTAPI
ExDupHandleTable(IN PEPROCESS Process,
                 IN PHANDLE_TABLE HandleTable,
                 IN PEX_DUPLICATE_HANDLE_CALLBACK DupHandleProcedure,
                 IN ULONG_PTR Mask)
{
    PHANDLE_TABLE NewTable;
    EXHANDLE Handle;
    PHANDLE_TABLE_ENTRY HandleTableEntry, NewEntry;
    BOOLEAN Failed = FALSE;
    PAGED_CODE();

    /* Allocate the duplicated copy */
    NewTable = ExpAllocateHandleTable(Process, FALSE);
    if (!NewTable) return NULL;

    /* Loop each entry */
    while (NewTable->NextHandleNeedingPool <
           HandleTable->NextHandleNeedingPool)
    {
        /* Insert it into the duplicated copy */
        if (!ExpAllocateHandleTableEntrySlow(NewTable, FALSE))
        {
            /* Insert failed, free the new copy and return */
            ExpFreeHandleTable(NewTable);
            return NULL;
        }
    }

    /* Setup the initial handle table data */
    NewTable->HandleCount = 0;
    NewTable->ExtraInfoPages = 0;
    NewTable->FirstFree = 0;

    /* Setup the first handle value  */
    Handle.Value = INDEX_TO_HANDLE_VALUE(1);

    /* Enter a critical region and lookup the new entry */
    KeEnterCriticalRegion();
    while ((NewEntry = ExpLookupHandleTableEntry(NewTable, Handle)))
    {
        /* Lookup the old entry */
        HandleTableEntry = ExpLookupHandleTableEntry(HandleTable, Handle);

        /* Loop each entry */
        do
        {
            /* Check if it doesn't match the audit mask */
            if (!(HandleTableEntry->Value & Mask))
            {
                /* Free it since we won't use it */
                Failed = TRUE;
            }
            else
            {
                /* Lock the entry */
                if (!ExpLockHandleTableEntry(HandleTable, HandleTableEntry))
                {
                    /* Free it since we can't lock it, so we won't use it */
                    Failed = TRUE;
                }
                else
                {
                    /* Copy the handle value */
                    *NewEntry = *HandleTableEntry;

                    /* Call the duplicate callback */
                    if (DupHandleProcedure(Process,
                                           HandleTable,
                                           HandleTableEntry,
                                           NewEntry))
                    {
                        /* Clear failure flag */
                        Failed = FALSE;

                        /* Lock the entry, increase the handle count */
                        NewEntry->Value |= EXHANDLE_TABLE_ENTRY_LOCK_BIT;
                        NewTable->HandleCount++;
                    }
                    else
                    {
                        /* Duplication callback refused, fail */
                        Failed = TRUE;
                    }
                }
            }

            /* Check if we failed earlier and need to free */
            if (Failed)
            {
                /* Free this entry */
                NewEntry->Object = NULL;
                NewEntry->NextFreeTableEntry = NewTable->FirstFree;
                NewTable->FirstFree = (ULONG)Handle.Value;
            }

            /* Increase the handle value and move to the next entry */
            Handle.Value += INDEX_TO_HANDLE_VALUE(1);
            NewEntry++;
            HandleTableEntry++;
        } while (Handle.Value % INDEX_TO_HANDLE_VALUE(LOW_LEVEL_ENTRIES));

        /* We're done, skip the last entry */
        Handle.Value += INDEX_TO_HANDLE_VALUE(1);
    }

    /* Acquire the table lock and insert this new table into the list */
    ExAcquirePushLockExclusive(&HandleTableListLock);
    InsertTailList(&HandleTableListHead, &NewTable->HandleTableList);
    ExReleasePushLockExclusive(&HandleTableListLock);

    /* Leave the critical region we entered previously and return the table */
    KeLeaveCriticalRegion();
    return NewTable;
}

BOOLEAN
NTAPI
ExChangeHandle(IN PHANDLE_TABLE HandleTable,
               IN HANDLE Handle,
               IN PEX_CHANGE_HANDLE_CALLBACK ChangeRoutine,
               IN ULONG_PTR Context)
{
    EXHANDLE ExHandle;
    PHANDLE_TABLE_ENTRY HandleTableEntry;
    BOOLEAN Result = FALSE;
    PAGED_CODE();

    /* Set the handle value */
    ExHandle.GenericHandleOverlay = Handle;

    /* Find the entry for this handle */
    HandleTableEntry = ExpLookupHandleTableEntry(HandleTable, ExHandle);

    /* Make sure that we found an entry, and that it's valid */
    if (!(HandleTableEntry) ||
        !(HandleTableEntry->Object) ||
        (HandleTableEntry->NextFreeTableEntry == -2))
    {
        /* It isn't, fail */
        return FALSE;
    }

    /* Enter a critical region */
    KeEnterCriticalRegion();

    /* Try locking the handle entry */
    if (ExpLockHandleTableEntry(HandleTable, HandleTableEntry))
    {
        /* Call the change routine and unlock the entry */
        Result = ChangeRoutine(HandleTableEntry, Context);
        ExUnlockHandleTableEntry(HandleTable, HandleTableEntry);
    }

    /* Leave the critical region and return the callback result */
    KeLeaveCriticalRegion();
    return Result;
}

VOID
NTAPI
ExSweepHandleTable(IN PHANDLE_TABLE HandleTable,
                   IN PEX_SWEEP_HANDLE_CALLBACK EnumHandleProcedure,
                   IN PVOID Context)
{
    EXHANDLE Handle;
    PHANDLE_TABLE_ENTRY HandleTableEntry;
    PAGED_CODE();

    /* Set the initial value and loop the entries */
    Handle.Value = INDEX_TO_HANDLE_VALUE(1);
    while ((HandleTableEntry = ExpLookupHandleTableEntry(HandleTable, Handle)))
    {
        /* Loop each handle */
        do
        {
            /* Lock the entry */
            if (ExpLockHandleTableEntry(HandleTable, HandleTableEntry))
            {
                /* Notify the callback routine */
                EnumHandleProcedure(HandleTableEntry,
                                    Handle.GenericHandleOverlay,
                                    Context);
            }

            /* Go to the next handle and entry */
            Handle.Value += INDEX_TO_HANDLE_VALUE(1);
            HandleTableEntry++;
        } while (Handle.Value % INDEX_TO_HANDLE_VALUE(LOW_LEVEL_ENTRIES));

        /* Skip past the last entry */
        Handle.Value += INDEX_TO_HANDLE_VALUE(1);
    }
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
ExEnumHandleTable(IN PHANDLE_TABLE HandleTable,
                  IN PEX_ENUM_HANDLE_CALLBACK EnumHandleProcedure,
                  IN OUT PVOID Context,
                  OUT PHANDLE EnumHandle OPTIONAL)
{
    EXHANDLE Handle;
    PHANDLE_TABLE_ENTRY HandleTableEntry;
    BOOLEAN Result = FALSE;
    PAGED_CODE();

    /* Enter a critical region */
    KeEnterCriticalRegion();

    /* Set the initial value and loop the entries */
    Handle.Value = 0;
    while ((HandleTableEntry = ExpLookupHandleTableEntry(HandleTable, Handle)))
    {
        /* Validate the entry */
        if ((HandleTableEntry->Object) &&
            (HandleTableEntry->NextFreeTableEntry != -2))
        {
            /* Lock the entry */
            if (ExpLockHandleTableEntry(HandleTable, HandleTableEntry))
            {
                /* Notify the callback routine */
                Result = EnumHandleProcedure(HandleTableEntry,
                                             Handle.GenericHandleOverlay,
                                             Context);

                /* Unlock it */
                ExUnlockHandleTableEntry(HandleTable, HandleTableEntry);

                /* Was this the one looked for? */
                if (Result)
                {
                    /* If so, return it if requested */
                    if (EnumHandle) *EnumHandle = Handle.GenericHandleOverlay;
                    break;
                }
            }
        }

        /* Go to the next entry */
        Handle.Value += INDEX_TO_HANDLE_VALUE(1);
    }

    /* Leave the critical region and return callback result */
    KeLeaveCriticalRegion();
    return Result;
}

#if DBG && defined(KDBG)
BOOLEAN ExpKdbgExtHandle(ULONG Argc, PCHAR Argv[])
{
    USHORT i;
    char *endptr;
    HANDLE ProcessId;
    EXHANDLE ExHandle;
    PLIST_ENTRY Entry;
    PEPROCESS Process;
    WCHAR KeyPath[256];
    PFILE_OBJECT FileObject;
    PHANDLE_TABLE HandleTable;
    POBJECT_HEADER ObjectHeader;
    PHANDLE_TABLE_ENTRY TableEntry;
    ULONG NeededLength = 0;
    ULONG NameLength;
    PCM_KEY_CONTROL_BLOCK Kcb, CurrentKcb;
    POBJECT_HEADER_NAME_INFO ObjectNameInfo;

    if (Argc > 1)
    {
        /* Get EPROCESS address or PID */
        i = 0;
        while (Argv[1][i])
        {
            if (!isdigit(Argv[1][i]))
            {
                i = 0;
                break;
            }

            ++i;
        }

        if (i == 0)
        {
            if (!KdbpGetHexNumber(Argv[1], (PVOID)&Process))
            {
                KdbpPrint("Invalid parameter: %s\n", Argv[1]);
                return TRUE;
            }

            /* In the end, we always want a PID */
            ProcessId = PsGetProcessId(Process);
        }
        else
        {
            ProcessId = (HANDLE)strtoul(Argv[1], &endptr, 10);
            if (*endptr != '\0')
            {
                KdbpPrint("Invalid parameter: %s\n", Argv[1]);
                return TRUE;
            }
        }
    }
    else
    {
        ProcessId = PsGetCurrentProcessId();
    }

    for (Entry = HandleTableListHead.Flink;
         Entry != &HandleTableListHead;
         Entry = Entry->Flink)
    {
        /* Only return matching PID
         * 0 matches everything
         */
        HandleTable = CONTAINING_RECORD(Entry, HANDLE_TABLE, HandleTableList);
        if (ProcessId != 0 && HandleTable->UniqueProcessId != ProcessId)
        {
            continue;
        }

        KdbpPrint("\n");

        KdbpPrint("Handle table at %p with %d entries in use\n", HandleTable, HandleTable->HandleCount);

        ExHandle.Value = 0;
        while ((TableEntry = ExpLookupHandleTableEntry(HandleTable, ExHandle)))
        {
            if ((TableEntry->Object) &&
                (TableEntry->NextFreeTableEntry != -2))
            {
                ObjectHeader = ObpGetHandleObject(TableEntry);

                KdbpPrint("%p: Object: %p GrantedAccess: %x Entry: %p\n", ExHandle.Value, &ObjectHeader->Body, TableEntry->GrantedAccess, TableEntry);
                KdbpPrint("Object: %p Type: (%x) %wZ\n", &ObjectHeader->Body, ObjectHeader->Type, &ObjectHeader->Type->Name);
                KdbpPrint("\tObjectHeader: %p\n", ObjectHeader);
                KdbpPrint("\t\tHandleCount: %u PointerCount: %u\n", ObjectHeader->HandleCount, ObjectHeader->PointerCount);

                /* Specific objects debug prints */

                /* For file, display path */
                if (ObjectHeader->Type == IoFileObjectType)
                {
                    FileObject = (PFILE_OBJECT)&ObjectHeader->Body;

                    KdbpPrint("\t\t\tName: %wZ\n", &FileObject->FileName);
                }

                /* For directory, and win32k objects, display object name */
                else if (ObjectHeader->Type == ObpDirectoryObjectType ||
                         ObjectHeader->Type == ExWindowStationObjectType ||
                         ObjectHeader->Type == ExDesktopObjectType ||
                         ObjectHeader->Type == MmSectionObjectType)
                {
                    ObjectNameInfo = OBJECT_HEADER_TO_NAME_INFO(ObjectHeader);
                    if (ObjectNameInfo != NULL && ObjectNameInfo->Name.Buffer != NULL)
                    {
                        KdbpPrint("\t\t\tName: %wZ\n", &ObjectNameInfo->Name);
                    }
                }

                /* For registry keys, display full path */
                else if (ObjectHeader->Type == CmpKeyObjectType)
                {
                    Kcb = ((PCM_KEY_BODY)&ObjectHeader->Body)->KeyControlBlock;
                    if (!Kcb->Delete)
                    {
                        CurrentKcb = Kcb;

                        /* See: CmpQueryNameInformation() */

                        while (CurrentKcb != NULL)
                        {
                            if (CurrentKcb->NameBlock->Compressed)
                                NeededLength += CmpCompressedNameSize(CurrentKcb->NameBlock->Name, CurrentKcb->NameBlock->NameLength);
                            else
                                NeededLength += CurrentKcb->NameBlock->NameLength;

                            NeededLength += sizeof(OBJ_NAME_PATH_SEPARATOR);

                            CurrentKcb = CurrentKcb->ParentKcb;
                        }

                        if (NeededLength < sizeof(KeyPath))
                        {
                            CurrentKcb = Kcb;

                            while (CurrentKcb != NULL)
                            {
                                if (CurrentKcb->NameBlock->Compressed)
                                {
                                    NameLength = CmpCompressedNameSize(CurrentKcb->NameBlock->Name, CurrentKcb->NameBlock->NameLength);
                                    CmpCopyCompressedName(&KeyPath[(NeededLength - NameLength)/sizeof(WCHAR)],
                                                          NameLength,
                                                          CurrentKcb->NameBlock->Name,
                                                          CurrentKcb->NameBlock->NameLength);
                                }
                                else
                                {
                                    NameLength = CurrentKcb->NameBlock->NameLength;
                                    RtlCopyMemory(&KeyPath[(NeededLength - NameLength)/sizeof(WCHAR)],
                                                  CurrentKcb->NameBlock->Name,
                                                  NameLength);
                                }

                                NeededLength -= NameLength;
                                NeededLength -= sizeof(OBJ_NAME_PATH_SEPARATOR);
                                KeyPath[NeededLength/sizeof(WCHAR)] = OBJ_NAME_PATH_SEPARATOR;

                                CurrentKcb = CurrentKcb->ParentKcb;
                            }
                        }

                        KdbpPrint("\t\t\tName: %S\n", KeyPath);
                    }
                }
            }

            ExHandle.Value += INDEX_TO_HANDLE_VALUE(1);
        }
    }

    return TRUE;
}
#endif
