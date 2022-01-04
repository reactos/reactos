/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 DLL
 * FILE:        dll/win32/ws2help/context.c
 * PURPOSE:     WinSock 2 DLL header
 */

/* INCLUDES ******************************************************************/

#include "precomp.h"

#include <ws2help.h>

/* DATA **********************************************************************/

CRITICAL_SECTION WshHandleTableLock;
HANDLE ghWriterEvent;
DWORD gdwSpinCount = 0;
DWORD gHandleToIndexMask;

CONST DWORD SockPrimes[] =
{
    31, 61, 127, 257, 521, 1031, 2053,
    4099, 8191, 16381, 32749, 65537, 131071, 261983,
    -1
};

typedef volatile LONG VLONG;
typedef VLONG *PVLONG;

/* DEFINES *******************************************************************/

/* Yes, we "abuse" the lower bits */
#define WSH_SEARCH_TABLE_FROM_HANDLE(h, t) \
    (&t->SearchTables[(((ULONG_PTR)h >> 2) & t->Mask)])

#define WSH_HASH_FROM_HANDLE(h, hs) \
    (hs->Handles[((ULONG_PTR)h % hs->Size)])

#define AcquireWriteLock(t) \
    EnterCriticalSection(&t->Lock);

#define ReleaseWriteLock(t) \
    LeaveCriticalSection(&t->Lock);

/* FUNCTIONS *****************************************************************/

static __inline
VOID
AcquireReadLock(IN PWAH_SEARCH_TABLE Table,
                IN PVLONG *Count)
{
    LONG OldCount;

    /* Start acquire loop */
    do
    {
        /* Write and save count value */
        *Count = Table->CurrentCount;
        OldCount = **Count;

        /* Check if it's valid and try to increment it */
        if ((OldCount > 0) && (InterlockedCompareExchange(*Count,
                                                          OldCount + 1,
                                                          OldCount) == OldCount))
        {
            /* Everything went OK */
            break;
        }
    } while (TRUE);
}

static __inline
VOID
ReleaseReadLock(IN PWAH_SEARCH_TABLE Table,
                IN PVLONG Count)
{
    /* Decrement the count. If we went below 0, someone is waiting... */
    if (InterlockedDecrement(Count) < 0)
    {
        /* We use pulse since this is a shared event */
        PulseEvent(ghWriterEvent);
    }
}

VOID
WINAPI
DoWaitForReaders(IN PWAH_SEARCH_TABLE Table,
                 IN PVLONG Counter)
{
    HANDLE EventHandle;

    /* Do a context switch */
    SwitchToThread();

    /* Check if the counter is above one */
    if (*Counter > 0)
    {
        /*
         * This shouldn't happen unless priorities are messed up. Do a wait so
         * that the threads with lower priority will get their chance now.
         */
        if (!ghWriterEvent)
        {
            /* We don't even have an event! Allocate one manually... */
            EventHandle = CreateEvent(NULL, TRUE, FALSE, NULL);
            if (EventHandle)
            {
                /* Save the event handle atomically */
                if ((InterlockedCompareExchangePointer((PVOID*)&ghWriterEvent,
                                                       EventHandle,
                                                       NULL)))
                {
                    /* Someone beat us to it, close ours */
                    CloseHandle(EventHandle);
                }
            }
            else
            {
                /* Things couldn't get worse for us. Do a last-resort hack */
                while (*Counter > 0) Sleep(10);
            }
        }

        /*
         * Our event is ready. Tell the others to signal us by making sure
         * that the last counter will be -1, notifying the last thread of our
         * request.
         */
        if (InterlockedDecrement(Counter) >= 0)
        {
            /* Keep looping */
            do
            {
                /* Wait in tiny bursts, so we can catch the PulseEvent */
                WaitForSingleObject(ghWriterEvent, 10);
            } while (*Counter >= 0);
        }
    }
}

static __inline
VOID
TryWaitForReaders(IN PWAH_SEARCH_TABLE Table)
{
    PVLONG OldCount = Table->CurrentCount;
    LONG SpinCount;

    /* See which counter is being used */
    if (OldCount == &Table->Count1)
    {
        /* Use counter 2 now */
        Table->Count2 = 1;
        Table->CurrentCount = &Table->Count2;
    }
    else
    {
        /* Use counter 1 now */
        Table->Count1 = 1;
        Table->CurrentCount = &Table->Count1;
    }

    /* Decrease the old count to block new readers */
    if (InterlockedDecrement(OldCount) > 0)
    {
        /* On an MP machine, spin a bit first */
        if (Table->SpinCount)
        {
            /* Get the spincount and loop it */
            SpinCount = Table->SpinCount;
            while (*OldCount > 0)
            {
                /* Check if the spin failed */
                if (--SpinCount <= 0) break;
            }
        }

        /* Check one last time if someone is still active */
        if (*OldCount > 0)
        {
            /* Yep, we'll have to do a blocking (slow) wait */
            DoWaitForReaders(Table, OldCount);
        }
    }
}

DWORD
WINAPI
WahCreateHandleContextTable(OUT PWAH_HANDLE_TABLE *Table)
{
    DWORD ErrorCode;
    PWAH_HANDLE_TABLE LocalTable;
    DWORD i;

    /* Enter the prolog, make sure we're initialized */
    ErrorCode = WS2HELP_PROLOG();
    if (ErrorCode != ERROR_SUCCESS) return ErrorCode;

    /* Assume NULL */
    *Table = NULL;

    /* Allocate enough tables */
    LocalTable = HeapAlloc(GlobalHeap,
                           0,
                           FIELD_OFFSET(WSH_HANDLE_TABLE,
                                        SearchTables[gHandleToIndexMask + 1]));

    /* Make sure it was allocated */
    if (!LocalTable) return WSA_NOT_ENOUGH_MEMORY;

    /* Set the mask for the table */
    LocalTable->Mask = gHandleToIndexMask;

    /* Now initialize every table */
    for (i = 0; i <= gHandleToIndexMask; i++)
    {
        /* No hash table yet */
        LocalTable->SearchTables[i].HashTable = NULL;

        /* Set the current count */
        LocalTable->SearchTables[i].CurrentCount = &LocalTable->SearchTables[i].Count1;

        /* Initialize the counts */
        LocalTable->SearchTables[i].Count1 = 1;
        LocalTable->SearchTables[i].Count2 = 0;

        /* Set expanding state and spin count */
        LocalTable->SearchTables[i].Expanding = FALSE;
        LocalTable->SearchTables[i].SpinCount = gdwSpinCount;

        /* Initialize the lock */
        (VOID)InitializeCriticalSectionAndSpinCount(&LocalTable->
                                                    SearchTables[i].Lock,
                                                    gdwSpinCount);
    }

    /* Return pointer */
    *Table = LocalTable;

    /* Return success */
    return ERROR_SUCCESS;
}

DWORD
WINAPI
WahDestroyHandleContextTable(IN PWAH_HANDLE_TABLE Table)
{
    DWORD i;

    /* Make sure the table is valid */
    if (!Table)
    {
        /* No valid table */
        return ERROR_INVALID_PARAMETER;
    }

    /* Loop each search table */
    for (i = 0; i <= Table->Mask; i++)
    {
        /* Check if there's a table here */
        if (Table->SearchTables[i].HashTable)
        {
            /* Free it */
            HeapFree(GlobalHeap, 0, Table->SearchTables[i].HashTable);
        }

        /* Delete the lock */
        DeleteCriticalSection(&Table->SearchTables[i].Lock);
    }

    /* Delete the table */
    HeapFree(GlobalHeap, 0, Table);

    /* Return success */
    return ERROR_SUCCESS;
}

BOOL
WINAPI
WahEnumerateHandleContexts(IN PWAH_HANDLE_TABLE Table,
                           IN PWAH_HANDLE_ENUMERATE_PROC Callback,
                           IN PVOID Context)
{
    DWORD i, j;
    PWAH_SEARCH_TABLE SearchTable;
    PWAH_HASH_TABLE HashTable;
    PWAH_HANDLE Handle;
    BOOL GoOn = TRUE;

    /* Loop the table */
    for (i = 0; i <= Table->Mask; i++)
    {
        /* Get the Search table */
        SearchTable = &Table->SearchTables[i];

        /* Lock it */
        AcquireWriteLock(SearchTable);

        /* Mark us as expanding and wait for everyone */
        SearchTable->Expanding = TRUE;
        TryWaitForReaders(SearchTable);

        /* Get the hash table */
        HashTable = SearchTable->HashTable;

        /* Make sure it exists */
        if (HashTable)
        {
            /* Loop every handle in it */
            for (j = 0; j < HashTable->Size; j++)
            {
                /* Get this handle */
                Handle = HashTable->Handles[j];
                if (!Handle) continue;

                /* Call the callback proc */
                GoOn = Callback(Context, Handle);
                if (!GoOn) break;
            }
        }

        /* Disable the expansion bit and release the lock */
        SearchTable->Expanding = FALSE;
        ReleaseWriteLock(SearchTable);

        /* Check again if we should leave */
        if (!GoOn) break;
    }

    /* return */
    return GoOn;
}

PWAH_HANDLE
WINAPI
WahInsertHandleContext(IN PWAH_HANDLE_TABLE Table,
                       IN PWAH_HANDLE Handle)
{
    PWAH_HANDLE *HashHandle, OldHandle;
    PVLONG Count;
    PWAH_HASH_TABLE HashTable, NewHashTable;
    DWORD HandleCount, i;
    PWAH_SEARCH_TABLE SearchTable;

    /* Get the current Search Table */
    SearchTable = WSH_SEARCH_TABLE_FROM_HANDLE(Handle->Handle, Table);

    /* Start loop */
    do
    {
        /* Get reader lock */
        AcquireReadLock(SearchTable, &Count);

        /* Get the hash table */
        HashTable = SearchTable->HashTable;

        /* Make sure we are not expanding, and that the table is there */
        if (!(SearchTable->Expanding) && (HashTable))
        {
            /* Get the hash handle */
            HashHandle = &WSH_HASH_FROM_HANDLE(Handle->Handle, HashTable);

            /* Do the insert */
            if (InterlockedCompareExchangePointer((PVOID*)HashHandle,
                                                  Handle,
                                                  NULL) == NULL)
            {
                /* Success, release the reader lock */
                ReleaseReadLock(SearchTable, Count);

                /* Save old handle */
                OldHandle = Handle;
                break;
            }
        }

        /* Release the read lock since we're done with it now */
        ReleaseReadLock(SearchTable, Count);

        /* We need the writer lock to expand/create the table */
        AcquireWriteLock(SearchTable);

        /* Mark the table in use */
        SearchTable->Expanding = TRUE;

        /* Wait for all the readers to finish */
        TryWaitForReaders(SearchTable);

        /* Start loop */
        do
        {
            /* Get the hash table again */
            HashTable = SearchTable->HashTable;

            /* Check if exists now */
            if (HashTable)
            {
                /* It does! Do what we wanted to do earlier. Get the hash... */
                HashHandle = &WSH_HASH_FROM_HANDLE(Handle->Handle, HashTable);

                /* Check if it doesn't exist */
                if (!(*HashHandle))
                {
                    /* Write it (no need for interlock, we have the RW lock) */
                    OldHandle = Handle;
                    *HashHandle = Handle;
                    break;
                }
                else if ((*HashHandle)->Handle == Handle->Handle)
                {
                    /* Handle matches, write it (see comment above) */
                    OldHandle = *HashHandle;
                    *HashHandle = Handle;
                    break;
                }

                /* No go, we need to expand the table. Remember the size now */
                HandleCount = HashTable->Size;
            }
            else
            {
                /* Table is empty, we have to create it */
                HandleCount = 0;
            }

ExpandTable:
            /* Find a free prime */
            for (i = 0; HandleCount >= SockPrimes[i]; i++);

            /* Check if we found one */
            if (SockPrimes[i] != 0xFFFFFFFF)
            {
                /* Use the prime */
                HandleCount = SockPrimes[i];
            }
            else
            {
                /* No primes left. Table is quite large, so simply double it */
                HandleCount *= 2;
            }

            /* Allocate the table */
            NewHashTable = HeapAlloc(GlobalHeap,
                                     0,
                                     FIELD_OFFSET(WSH_HASH_TABLE,
                                                  Handles[HandleCount]));

            /* Hopefully we have one now */
            if (NewHashTable)
            {
                /* Set its size */
                NewHashTable->Size = HandleCount;

                /* Initialize it */
                RtlZeroMemory(NewHashTable->Handles, HandleCount * sizeof(PVOID));

                /* Insert us first */
                WSH_HASH_FROM_HANDLE(Handle->Handle, NewHashTable) = Handle;

                /* Now check if our old table had entries in it */
                if (HashTable)
                {
                    /* We need to move them */
                    for (i = 0; i < HashTable->Size; i++)
                    {
                        /* Make sure the hash handle exists */
                        if (HashTable->Handles[i])
                        {
                            /* Get it */
                            HashHandle = &WSH_HASH_FROM_HANDLE(HashTable->
                                                               Handles[i]->Handle,
                                                               NewHashTable);

                            /* Check if it has a value */
                            if (!(*HashHandle))
                            {
                                /* It's empty, so just write the handle */
                                *HashHandle = HashTable->Handles[i];
                            }
                            else
                            {
                                /* Not empty :/... that implies a collision */
                                HeapFree(GlobalHeap, 0, NewHashTable);
                                goto ExpandTable;
                            }
                        }
                    }

                    /* Write the new hash table */
                    SearchTable->HashTable = NewHashTable;

                    /* Wait for everyone to be done with it, then free it */
                    TryWaitForReaders(SearchTable);
                    HeapFree(GlobalHeap, 0, HashTable);
                }
                else
                {
                    /* It was empty, nothing to worry about */
                    SearchTable->HashTable = NewHashTable;
                }

                /* Save the old handle */
                OldHandle = Handle;
            }
            else
            {
                /* There was no old handle */
                OldHandle = Handle;
            }
        } while (0);

        /* Mark us as free, and release the write lock */
        SearchTable->Expanding = FALSE;
        ReleaseWriteLock(SearchTable);
        break;
    } while (1);

    /* Return the old handle */
    return OldHandle;
}

PWAH_HANDLE
WINAPI
WahReferenceContextByHandle(IN PWAH_HANDLE_TABLE Table,
                            IN HANDLE Handle)
{
    PWAH_HANDLE HashHandle;
    PWAH_SEARCH_TABLE SearchTable;
    PWAH_HASH_TABLE HashTable;
    PVLONG Count;

    /* Get the current Search Table */
    SearchTable = WSH_SEARCH_TABLE_FROM_HANDLE(Handle, Table);

    /* Lock it */
    AcquireReadLock(SearchTable, &Count);

    /* Get the hash table and handle */
    HashTable = SearchTable->HashTable;

    /* Check if it's valid, and if it's the one we want */
    if ((HashTable) &&
        (HashHandle = WSH_HASH_FROM_HANDLE(Handle, HashTable)) &&
        (HashHandle->Handle == Handle))
    {
        /* Reference the handle */
        InterlockedIncrement(&HashHandle->RefCount);
    }
    else
    {
        /* Invalid handle */
        HashHandle = NULL;
    }

    /* Release the lock */
    ReleaseReadLock(SearchTable, Count);

    /* Return */
    return HashHandle;
}

DWORD
WINAPI
WahRemoveHandleContext(IN PWAH_HANDLE_TABLE Table,
                       IN PWAH_HANDLE Handle)
{
    PWAH_HANDLE *HashHandle;
    PWAH_SEARCH_TABLE SearchTable;
    PWAH_HASH_TABLE HashTable;
    DWORD ErrorCode = ERROR_SUCCESS;

    /* Get the current Search Table */
    SearchTable = WSH_SEARCH_TABLE_FROM_HANDLE(Handle->Handle, Table);

    /* Lock it */
    AcquireWriteLock(SearchTable);

    /* Get the hash table and handle */
    HashTable = SearchTable->HashTable;
    HashHandle = &WSH_HASH_FROM_HANDLE(Handle->Handle, HashTable);

    /* Make sure we have a handle, and write the new pointer */
    if (HashHandle && (InterlockedCompareExchangePointer((PVOID*)HashHandle,
                                                         NULL,
                                                         Handle) == Handle))
    {
        /* Wait for everyone to be done with it */
        TryWaitForReaders(SearchTable);
    }
    else
    {
        /* Invalid handle */
        ErrorCode = ERROR_INVALID_PARAMETER;
    }

    /* Release the lock */
    ReleaseWriteLock(SearchTable);

    /* Return */
    return ErrorCode;
}

/* EOF */
