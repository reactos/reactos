/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * FILE:              lib/rtl/resource.c
 * PURPOSE:           Resource (multiple-reader-single-writer lock) functions
 * PROGRAMMER:        Partially takem from Wine:
 *                    Copyright 1996-1998 Marcus Meissner
 *                              1999 Alex Korobka
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ***************************************************************/

/*
 * @implemented
 */
VOID
NTAPI
RtlInitializeResource(PRTL_RESOURCE Resource)
{
    NTSTATUS Status;

    Status = RtlInitializeCriticalSection(&Resource->Lock);
    if (!NT_SUCCESS(Status))
    {
        RtlRaiseStatus(Status);
    }

    Status = NtCreateSemaphore(&Resource->SharedSemaphore,
                               SEMAPHORE_ALL_ACCESS,
                               NULL,
                               0,
                               65535);
    if (!NT_SUCCESS(Status))
    {
        RtlRaiseStatus(Status);
    }
    Resource->SharedWaiters = 0;

    Status = NtCreateSemaphore(&Resource->ExclusiveSemaphore,
                               SEMAPHORE_ALL_ACCESS,
                               NULL,
                               0,
                               65535);
    if (!NT_SUCCESS(Status))
    {
        RtlRaiseStatus(Status);
    }

    Resource->ExclusiveWaiters = 0;
    Resource->NumberActive = 0;
    Resource->OwningThread = NULL;
    Resource->TimeoutBoost = 0; /* no info on this one, default value is 0 */
}


/*
 * @implemented
 */
VOID
NTAPI
RtlDeleteResource(PRTL_RESOURCE Resource)
{
    RtlDeleteCriticalSection(&Resource->Lock);
    NtClose(Resource->ExclusiveSemaphore);
    NtClose(Resource->SharedSemaphore);
    Resource->OwningThread = NULL;
    Resource->ExclusiveWaiters = 0;
    Resource->SharedWaiters = 0;
    Resource->NumberActive = 0;
}


/*
 * @implemented
 */
BOOLEAN
NTAPI
RtlAcquireResourceExclusive(
    PRTL_RESOURCE Resource,
    BOOLEAN Wait)
{
    NTSTATUS Status;
    BOOLEAN retVal = FALSE;

start:
    RtlEnterCriticalSection(&Resource->Lock);
    if (Resource->NumberActive == 0) /* lock is free */
    {
        Resource->NumberActive = -1;
        retVal = TRUE;
    }
    else if (Resource->NumberActive < 0) /* exclusive lock in progress */
    {
        if (Resource->OwningThread == NtCurrentTeb()->ClientId.UniqueThread)
        {
            retVal = TRUE;
            Resource->NumberActive--;
            goto done;
        }
wait:
        if (Wait)
        {
            Resource->ExclusiveWaiters++;

            RtlLeaveCriticalSection(&Resource->Lock);
            Status = NtWaitForSingleObject(Resource->ExclusiveSemaphore,
                                           FALSE,
                                           NULL);
            if (!NT_SUCCESS(Status))
                goto done;
            goto start; /* restart the acquisition to avoid deadlocks */
        }
    }
    else  /* one or more shared locks are in progress */
    {
        if (Wait)
            goto wait;
    }
    if (retVal)
        Resource->OwningThread = NtCurrentTeb()->ClientId.UniqueThread;
done:
    RtlLeaveCriticalSection(&Resource->Lock);
    return retVal;
}


/*
 * @implemented
 */
BOOLEAN
NTAPI
RtlAcquireResourceShared(
    PRTL_RESOURCE Resource,
    BOOLEAN Wait)
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    BOOLEAN retVal = FALSE;

start:
    RtlEnterCriticalSection(&Resource->Lock);
    if (Resource->NumberActive < 0)
    {
        if (Resource->OwningThread == NtCurrentTeb()->ClientId.UniqueThread)
        {
            Resource->NumberActive--;
            retVal = TRUE;
            goto done;
        }

        if (Wait)
        {
            Resource->SharedWaiters++;
            RtlLeaveCriticalSection(&Resource->Lock);
            Status = NtWaitForSingleObject(Resource->SharedSemaphore,
                                           FALSE,
                                           NULL);
            if (!NT_SUCCESS(Status))
                goto done;
            goto start;
        }
    }
    else
    {
        if (Status != STATUS_WAIT_0) /* otherwise RtlReleaseResource() has already done it */
            Resource->NumberActive++;
        retVal = TRUE;
    }
done:
    RtlLeaveCriticalSection(&Resource->Lock);
    return retVal;
}


/*
 * @implemented
 */
VOID
NTAPI
RtlConvertExclusiveToShared(PRTL_RESOURCE Resource)
{
    RtlEnterCriticalSection(&Resource->Lock);

    if (Resource->NumberActive == -1)
    {
        Resource->OwningThread = NULL;

        if (Resource->SharedWaiters > 0)
        {
            ULONG n;
            /* prevent new writers from joining until
             * all queued readers have done their thing */
            n = Resource->SharedWaiters;
            Resource->NumberActive = Resource->SharedWaiters + 1;
            Resource->SharedWaiters = 0;
            NtReleaseSemaphore(Resource->SharedSemaphore,
                               n,
                               NULL);
        }
        else
        {
            Resource->NumberActive = 1;
        }
    }

    RtlLeaveCriticalSection(&Resource->Lock);
}


/*
 * @implemented
 */
VOID
NTAPI
RtlConvertSharedToExclusive(PRTL_RESOURCE Resource)
{
    NTSTATUS Status;

    RtlEnterCriticalSection(&Resource->Lock);

    if (Resource->NumberActive == 1)
    {
        Resource->OwningThread = NtCurrentTeb()->ClientId.UniqueThread;
        Resource->NumberActive = -1;
    }
    else
    {
        Resource->ExclusiveWaiters++;

        RtlLeaveCriticalSection(&Resource->Lock);
        Status = NtWaitForSingleObject(Resource->ExclusiveSemaphore,
                                       FALSE,
                                       NULL);
        if (!NT_SUCCESS(Status))
            return;

        RtlEnterCriticalSection(&Resource->Lock);
        Resource->OwningThread = NtCurrentTeb()->ClientId.UniqueThread;
        Resource->NumberActive = -1;
    }
    RtlLeaveCriticalSection(&Resource->Lock);
}


/*
 * @implemented
 */
VOID
NTAPI
RtlReleaseResource(PRTL_RESOURCE Resource)
{
    RtlEnterCriticalSection(&Resource->Lock);

    if (Resource->NumberActive > 0) /* have one or more readers */
    {
        Resource->NumberActive--;
        if (Resource->NumberActive == 0)
        {
            if (Resource->ExclusiveWaiters > 0)
            {
wake_exclusive:
                Resource->ExclusiveWaiters--;
                NtReleaseSemaphore(Resource->ExclusiveSemaphore,
                                   1,
                                   NULL);
            }
        }
    }
    else if (Resource->NumberActive < 0) /* have a writer, possibly recursive */
    {
        Resource->NumberActive++;
        if (Resource->NumberActive == 0)
        {
            Resource->OwningThread = 0;
            if (Resource->ExclusiveWaiters > 0)
            {
                goto wake_exclusive;
            }
            else
            {
                if (Resource->SharedWaiters > 0)
                {
                    ULONG n;
                    /* prevent new writers from joining until
                     * all queued readers have done their thing */
                    n = Resource->SharedWaiters;
                    Resource->NumberActive = Resource->SharedWaiters;
                    Resource->SharedWaiters = 0;
                    NtReleaseSemaphore(Resource->SharedSemaphore,
                                       n,
                                       NULL);
                }
            }
        }
    }
    RtlLeaveCriticalSection(&Resource->Lock);
}


/*
 * @implemented
 */
VOID
NTAPI
RtlDumpResource(PRTL_RESOURCE Resource)
{
    DbgPrint("RtlDumpResource(%p):\n\tactive count = %d\n\twaiting readers = %u\n\twaiting writers = %u\n",
             Resource,
             Resource->NumberActive,
             Resource->SharedWaiters,
             Resource->ExclusiveWaiters);

    if (Resource->NumberActive != 0)
    {
        DbgPrint("\towner thread = %p\n",
                 Resource->OwningThread);
    }
}

/* EOF */
