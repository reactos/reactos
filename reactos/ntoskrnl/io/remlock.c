/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/remlock.c
 * PURPOSE:         Remove Lock Support
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Filip Navara (navaraf@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

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

    /* Check if this is a debug lock */
    if (RemlockSize == sizeof(IO_REMOVE_LOCK_DBG_BLOCK))
    {
        /* Clear the lock */
        RtlZeroMemory(Lock, RemlockSize);

        /* Setup debug parameters */
        Lock->Dbg.HighWatermark = HighWatermark;
        Lock->Dbg.MaxLockedTicks = MaxLockedMinutes * 600000000;
        Lock->Dbg.AllocateTag = AllocateTag;
        KeInitializeSpinLock(&Lock->Dbg.Spin);
    }
    else
    {
        /* Otherwise, setup a free block */
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
    PEXTENDED_IO_REMOVE_LOCK Lock = (PEXTENDED_IO_REMOVE_LOCK)RemoveLock;

    /* Increase the lock count */
    InterlockedIncrement(&Lock->Common.IoCount);
    if (!Lock->Common.Removed)
    {
        /* Check what kind of lock this is */
        if (RemlockSize == sizeof(IO_REMOVE_LOCK_DBG_BLOCK))
        {
            /* FIXME: Not yet supported */
            DPRINT1("UNIMPLEMENTED\n");
            KEBUGCHECK(0);
        }
    }
    else
    {
        /* Otherwise, decrement the count and check if it's gone */
        if (!InterlockedDecrement(&Lock->Common.IoCount))
        {
            /* Signal the event */
            KeSetEvent(&Lock->Common.RemoveEvent, IO_NO_INCREMENT, FALSE);
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
    PEXTENDED_IO_REMOVE_LOCK Lock = (PEXTENDED_IO_REMOVE_LOCK)RemoveLock;

    /* Check what kind of lock this is */
    if (RemlockSize == sizeof(IO_REMOVE_LOCK_DBG_BLOCK))
    {
        /* FIXME: Not yet supported */
        DPRINT1("UNIMPLEMENTED\n");
        KEBUGCHECK(0);
    }

    /* Decrement the lock count */
    if (!InterlockedDecrement(&Lock->Common.IoCount));
    {
        /* Signal the event */
        KeSetEvent(&Lock->Common.RemoveEvent, IO_NO_INCREMENT, FALSE);
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
    PEXTENDED_IO_REMOVE_LOCK Lock = (PEXTENDED_IO_REMOVE_LOCK)RemoveLock;
    PAGED_CODE();

    /* Remove the lock and decrement the count */
    Lock->Common.Removed = TRUE;
    if (InterlockedDecrement(&Lock->Common.IoCount) > 0)
    {
        /* Wait for it */
        KeWaitForSingleObject(&Lock->Common.RemoveEvent,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
    }

    /* Check what kind of lock this is */
    if (RemlockSize == sizeof(IO_REMOVE_LOCK_DBG_BLOCK))
    {
        /* FIXME: Not yet supported */
        DPRINT1("UNIMPLEMENTED\n");
        KEBUGCHECK(0);
    }
}

/* EOF */
