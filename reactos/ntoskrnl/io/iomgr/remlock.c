/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/remlock.c
 * PURPOSE:         Remove Lock Support
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Filip Navara (navaraf@reactos.org)
 *                  Pierre Schweitzer (pierre.schweitzer@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

typedef struct _IO_REMOVE_LOCK_TRACKING_BLOCK
{
    SINGLE_LIST_ENTRY BlockEntry;
    PVOID Tag;
    LARGE_INTEGER LockMoment;
    LPCSTR File;
    ULONG Line;
} IO_REMOVE_LOCK_TRACKING_BLOCK;

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
VOID
NTAPI
IoInitializeRemoveLockEx(IN PIO_REMOVE_LOCK RemoveLock,
                         IN ULONG AllocateTag,
                         IN ULONG MaxLockedMinutes,
                         IN ULONG HighWatermark,
                         IN ULONG RemlockSize)
{
    PEXTENDED_IO_REMOVE_LOCK Lock = (PEXTENDED_IO_REMOVE_LOCK)RemoveLock;
    PAGED_CODE();

    ASSERT(HighWatermark < MAXLONG);

    /* If no lock given, nothing to do */
    if (!Lock)
    {
        return;
    }

    switch (RemlockSize)
    {
        /* Check if this is a debug lock */
        case sizeof(IO_REMOVE_LOCK_DBG_BLOCK):
            /* Setup debug parameters */
            Lock->Dbg.Signature = 'COLR';
            Lock->Dbg.HighWatermark = HighWatermark;
            Lock->Dbg.MaxLockedTicks = KeQueryTimeIncrement() * MaxLockedMinutes * 600000000;
            Lock->Dbg.AllocateTag = AllocateTag;
            KeInitializeSpinLock(&(Lock->Dbg.Spin));
            Lock->Dbg.LowMemoryCount = 0;
            Lock->Dbg.Blocks = NULL;

        case sizeof(IO_REMOVE_LOCK_COMMON_BLOCK):
            /* Setup a free block */
            Lock->Common.Removed = FALSE;
            Lock->Common.IoCount = 1;
            KeInitializeEvent(&Lock->Common.RemoveEvent,
                              SynchronizationEvent,
                              FALSE);
    }
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IoAcquireRemoveLockEx(IN PIO_REMOVE_LOCK RemoveLock,
                      IN OPTIONAL PVOID Tag,
                      IN LPCSTR File,
                      IN ULONG Line,
                      IN ULONG RemlockSize)
{
    KIRQL OldIrql;
    LONG LockValue;
    PIO_REMOVE_LOCK_TRACKING_BLOCK TrackingBlock;
    PEXTENDED_IO_REMOVE_LOCK Lock = (PEXTENDED_IO_REMOVE_LOCK)RemoveLock;

    /* Increase the lock count */
    LockValue = InterlockedIncrement(&(Lock->Common.IoCount));
    ASSERT(LockValue > 0);
    if (!Lock->Common.Removed)
    {
        /* Check what kind of lock this is */
        if (RemlockSize == sizeof(IO_REMOVE_LOCK_DBG_BLOCK))
        {
            ASSERT(Lock->Dbg.HighWatermark == 0 || LockValue <= Lock->Dbg.HighWatermark);

            /* Allocate a tracking block */
            TrackingBlock = ExAllocatePoolWithTag(NonPagedPool, sizeof(IO_REMOVE_LOCK_TRACKING_BLOCK), Lock->Dbg.AllocateTag);
            if (!TrackingBlock)
            {
                /* Keep count of failures for lock release and missing tags */ 
                InterlockedIncrement(&(Lock->Dbg.LowMemoryCount));
            }
            else
            {
                /* Initialize block */
                RtlZeroMemory(TrackingBlock, sizeof(IO_REMOVE_LOCK_TRACKING_BLOCK));
                TrackingBlock->Tag = Tag;
                TrackingBlock->File = File;
                TrackingBlock->Line = Line;
                KeQueryTickCount(&(TrackingBlock->LockMoment));

                /* Queue the block */
                KeAcquireSpinLock(&(Lock->Dbg.Spin), &OldIrql);
                PushEntryList((PSINGLE_LIST_ENTRY)&(Lock->Dbg.Blocks), &(TrackingBlock->BlockEntry));
                KeReleaseSpinLock(&(Lock->Dbg.Spin), OldIrql);
            }
        }
    }
    else
    {
        /* Otherwise, decrement the count and check if it's gone */
        if (!InterlockedDecrement(&(Lock->Common.IoCount)))
        {
            /* Signal the event */
            KeSetEvent(&(Lock->Common.RemoveEvent), IO_NO_INCREMENT, FALSE);
        }

        /* Return pending delete */
        return STATUS_DELETE_PENDING;
    }

    /* Otherwise, return success */
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
VOID
NTAPI
IoReleaseRemoveLockEx(IN PIO_REMOVE_LOCK RemoveLock,
                      IN PVOID Tag,
                      IN ULONG RemlockSize)
{
    KIRQL OldIrql;
    LONG LockValue;
    BOOLEAN TagFound;
    LARGE_INTEGER CurrentMoment;
    PSINGLE_LIST_ENTRY ListEntry;
    PIO_REMOVE_LOCK_TRACKING_BLOCK TrackingBlock;
    PEXTENDED_IO_REMOVE_LOCK Lock = (PEXTENDED_IO_REMOVE_LOCK)RemoveLock;

    /* Check what kind of lock this is */
    if (RemlockSize == sizeof(IO_REMOVE_LOCK_DBG_BLOCK))
    {
        /* Acquire blocks queue */
        KeAcquireSpinLock(&(Lock->Dbg.Spin), &OldIrql);

        /* Get the release moment */
        KeQueryTickCount(&CurrentMoment);

        /* Start browsing tracking blocks to find a block that would match given tag */
        TagFound = FALSE;
        for (ListEntry = (PSINGLE_LIST_ENTRY)&Lock->Dbg.Blocks; ListEntry; ListEntry = ListEntry->Next)
        {
            TrackingBlock = CONTAINING_RECORD(ListEntry, IO_REMOVE_LOCK_TRACKING_BLOCK, BlockEntry);

            /* First of all, check if the lock was locked for too long */
            if (CurrentMoment.QuadPart &&
                CurrentMoment.QuadPart - TrackingBlock->LockMoment.QuadPart >  Lock->Dbg.MaxLockedTicks)
            {
                DPRINT("Lock %#08lx (with tag %#08lx) was supposed to be held at max %I64d ticks but lasted longer\n",
                       Lock, TrackingBlock->Tag, Lock->Dbg.MaxLockedTicks);
                DPRINT("Lock was acquired in file %s at line %d\n", TrackingBlock->File, TrackingBlock->Line);
                ASSERT(FALSE);
            }

            /* If no tracking was found yet */
            if (TagFound == FALSE)
            {
                /* Check if the current one could match */
                if (TrackingBlock->Tag == Tag)
                {
                    /* Yes, then remove it from the queue and free it */
                    TagFound = TRUE;
                    if (ListEntry == (PSINGLE_LIST_ENTRY)&Lock->Dbg.Blocks)
                    {
                        /* Here it is head, remove it using macro */
                        PopEntryList((PSINGLE_LIST_ENTRY)&(Lock->Dbg.Blocks));
                        ExFreePoolWithTag(TrackingBlock, Lock->Dbg.AllocateTag);
                    }
                    else
                    {
                        /* It's not head, remove it "manually */
                        ListEntry->Next = TrackingBlock->BlockEntry.Next;
                        ExFreePoolWithTag(TrackingBlock, Lock->Dbg.AllocateTag);
                    }
                }
             }
        }
        /* We're done, release queue lock */
        KeReleaseSpinLock(&(Lock->Dbg.Spin), OldIrql);

        /* If we didn't find any matching block */
        if (TagFound == FALSE)
        {
            /* Check if it was because we were low in memory
             * If yes, then ignore, that's normal
             */
            if (InterlockedDecrement(&(Lock->Dbg.LowMemoryCount) < 0))
            {
                /* Otherwise signal the issue, it shouldn't happen */
                InterlockedIncrement(&(Lock->Dbg.LowMemoryCount));
                DPRINT("Failed finding block for tag: %#08lx\n", Tag);
                ASSERT(FALSE);
            }
        }
    }

    /* Decrement the lock count */
    LockValue = InterlockedDecrement(&(Lock->Common.IoCount));
    ASSERT(LockValue >= 0);

    if (!LockValue)
    {
        /* Someone should be waiting... */
        ASSERT(Lock->Common.Removed);

        /* Signal the event */
        KeSetEvent(&(Lock->Common.RemoveEvent), IO_NO_INCREMENT, FALSE);
    }
}

/*
 * @implemented
 */
VOID
NTAPI
IoReleaseRemoveLockAndWaitEx(IN PIO_REMOVE_LOCK RemoveLock,
                             IN PVOID Tag,
                             IN ULONG RemlockSize)
{
    LONG LockValue;
    PIO_REMOVE_LOCK_TRACKING_BLOCK TrackingBlock;
    PEXTENDED_IO_REMOVE_LOCK Lock = (PEXTENDED_IO_REMOVE_LOCK)RemoveLock;
    PAGED_CODE();

    /* Remove the lock and decrement the count */
    Lock->Common.Removed = TRUE;
    LockValue = InterlockedDecrement(&Lock->Common.IoCount);
    ASSERT(LockValue > 0);

    /* If we are still > 0, then wait for the others to remove lock */
    if (InterlockedDecrement(&Lock->Common.IoCount) > 0)
    {
        /* Wait for it */
        KeWaitForSingleObject(&(Lock->Common.RemoveEvent),
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
    }

    /* Check what kind of lock this is */
    if (RemlockSize == sizeof(IO_REMOVE_LOCK_DBG_BLOCK))
    {
        /* A block must be remaining */
        ASSERT(Lock->Dbg.Blocks);

        /* Get it */
        TrackingBlock = CONTAINING_RECORD(Lock->Dbg.Blocks, IO_REMOVE_LOCK_TRACKING_BLOCK, BlockEntry);
        /* Tag should match */
        if (TrackingBlock->Tag != Tag)
        {
            DPRINT("Last tracking block tag invalid! Expected: %x, having: %x\n", Tag, TrackingBlock->Tag);
            ASSERT(TrackingBlock->Tag != Tag);
        }

        /* Release block */
        ExFreePoolWithTag(Lock->Dbg.Blocks, Lock->Dbg.AllocateTag);
    }
}

/* EOF */
