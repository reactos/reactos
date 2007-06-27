/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Work Item implementation
 * FILE:              lib/rtl/workitem.c
 * PROGRAMMER:        
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ***************************************************************/

#define MAX_WORKERTHREADS   0x100
#define WORKERTHREAD_CREATION_THRESHOLD 0x5

typedef struct _RTLP_IOWORKERTHREAD
{
    LIST_ENTRY ListEntry;
    HANDLE ThreadHandle;
    ULONG Flags;
} RTLP_IOWORKERTHREAD, *PRTLP_IOWORKERTHREAD;

typedef struct _RTLP_WORKITEM
{
    WORKERCALLBACKFUNC Function;
    PVOID Context;
    ULONG Flags;
    HANDLE TokenHandle;
} RTLP_WORKITEM, *PRTLP_WORKITEM;

static LONG ThreadPoolInitialized = 0;
static RTL_CRITICAL_SECTION ThreadPoolLock;
static PRTLP_IOWORKERTHREAD PersistentIoThread;
static LIST_ENTRY ThreadPoolIOWorkerThreadsList;
static HANDLE ThreadPoolCompletionPort;
static LONG ThreadPoolWorkerThreads;
static LONG ThreadPoolWorkerThreadsRequests;
static LONG ThreadPoolWorkerThreadsLongRequests;
static LONG ThreadPoolIOWorkerThreads;
static LONG ThreadPoolIOWorkerThreadsRequests;
static LONG ThreadPoolIOWorkerThreadsLongRequests;

#define IsThreadPoolInitialized() ((volatile LONG)ThreadPoolInitialized == 1)

static NTSTATUS
RtlpInitializeThreadPool(VOID)
{
    NTSTATUS Status = STATUS_SUCCESS;
    LONG InitStatus;

    do
    {
        InitStatus = _InterlockedCompareExchange(&ThreadPoolInitialized,
                                                 2,
                                                 0);
        if (InitStatus == 0)
        {
            /* We're the first thread to initialize the thread pool */

            InitializeListHead(&ThreadPoolIOWorkerThreadsList);

            PersistentIoThread = NULL;

            ThreadPoolWorkerThreads = 0;
            ThreadPoolWorkerThreadsRequests = 0;
            ThreadPoolWorkerThreadsLongRequests = 0;
            ThreadPoolIOWorkerThreads = 0;
            ThreadPoolIOWorkerThreadsRequests = 0;
            ThreadPoolIOWorkerThreadsLongRequests = 0;

            /* Initialize the lock */
            Status = RtlInitializeCriticalSection(&ThreadPoolLock);
            if (!NT_SUCCESS(Status))
                goto Finish;

            /* Create the complection port */
            Status = NtCreateIoCompletion(&ThreadPoolCompletionPort,
                                          IO_COMPLETION_ALL_ACCESS,
                                          NULL,
                                          0);
            if (!NT_SUCCESS(Status))
            {
                RtlDeleteCriticalSection(&ThreadPoolLock);
                goto Finish;
            }

Finish:
            /* Initialization done */
            _InterlockedExchange(&ThreadPoolInitialized,
                                 1);
            break;
        }
        else if (InitStatus == 2)
        {
            LARGE_INTEGER Timeout;

            /* Another thread is currently initializing the thread pool!
               Poll after a short period of time to see if the initialization
               was completed */

            Timeout.QuadPart = -10000000LL; /* Wait for a second */
            NtDelayExecution(FALSE,
                             &Timeout);
        }
    } while (InitStatus != 1);

    return Status;
}

static NTSTATUS
RtlpGetImpersonationToken(OUT PHANDLE TokenHandle)
{
    NTSTATUS Status;

    Status = NtOpenThreadToken(NtCurrentThread(),
                               TOKEN_IMPERSONATE,
                               TRUE,
                               TokenHandle);
    if (Status == STATUS_NO_TOKEN || Status == STATUS_CANT_OPEN_ANONYMOUS)
    {
        *TokenHandle = NULL;
        Status = STATUS_SUCCESS;
    }

    return Status;
}

static NTSTATUS
RtlpStartWorkerThread(PTHREAD_START_ROUTINE StartRoutine)
{
    NTSTATUS Status;
    HANDLE ThreadHandle;
    LARGE_INTEGER Timeout;
    volatile LONG WorkerInitialized = 0;

    Timeout.QuadPart = -10000LL; /* Wait for 100ms */

    /* Start the thread */
    Status = RtlCreateUserThread(NtCurrentProcess(),
                                 NULL,
                                 FALSE,
                                 0,
                                 0,
                                 0,
                                 StartRoutine,
                                 (PVOID)&WorkerInitialized,
                                 &ThreadHandle,
                                 NULL);

    if (NT_SUCCESS(Status))
    {
        /* Poll until the thread got a chance to initialize */
        while (WorkerInitialized == 0)
        {
            NtDelayExecution(FALSE,
                             &Timeout);
        }

        NtClose(ThreadHandle);
    }

    return Status;
}

static VOID
NTAPI
RtlpExecuteWorkItem(IN OUT PVOID NormalContext,
                    IN OUT PVOID SystemArgument1,
                    IN OUT PVOID SystemArgument2)
{
    NTSTATUS Status;
    BOOLEAN Impersonated = FALSE;
    RTLP_WORKITEM WorkItem = *(volatile RTLP_WORKITEM *)SystemArgument2;

    RtlFreeHeap(RtlGetProcessHeap(),
                0,
                SystemArgument2);

    if (WorkItem.TokenHandle != NULL)
    {
        Status = NtSetInformationThread(NtCurrentThread(),
                                        ThreadImpersonationToken,
                                        &WorkItem.TokenHandle,
                                        sizeof(HANDLE));

        NtClose(WorkItem.TokenHandle);

        if (NT_SUCCESS(Status))
        {
            Impersonated = TRUE;
        }
    }

    _SEH_TRY
    {
        DPRINT("RtlpExecuteWorkItem: Function: 0x%p Context: 0x%p ImpersonationToken: 0x%p\n", WorkItem.Function, WorkItem.Context, WorkItem.TokenHandle);

        /* Execute the function */
        WorkItem.Function(WorkItem.Context);
    }
    _SEH_HANDLE
    {
        DPRINT1("Exception 0x%x while executing IO work item 0x%p\n", _SEH_GetExceptionCode(), WorkItem.Function);
    }
    _SEH_END;

    if (Impersonated)
    {
        WorkItem.TokenHandle = NULL;
        Status = NtSetInformationThread(NtCurrentThread(),
                                        ThreadImpersonationToken,
                                        &WorkItem.TokenHandle,
                                        sizeof(HANDLE));
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to revert worker thread to self!!! Status: 0x%x\n", Status);
        }
    }

    /* update the requests counter */
    _InterlockedDecrement(&ThreadPoolWorkerThreadsRequests);

    if (WorkItem.Flags & WT_EXECUTELONGFUNCTION)
    {
        _InterlockedDecrement(&ThreadPoolWorkerThreadsLongRequests);
    }
}


static NTSTATUS
RtlpQueueWorkerThread(IN OUT PRTLP_WORKITEM WorkItem)
{
    NTSTATUS Status = STATUS_SUCCESS;

    _InterlockedIncrement(&ThreadPoolWorkerThreadsRequests);

    if (WorkItem->Flags & WT_EXECUTELONGFUNCTION)
    {
        _InterlockedIncrement(&ThreadPoolWorkerThreadsLongRequests);
    }

    if (WorkItem->Flags & WT_EXECUTEINPERSISTENTTHREAD)
    {
        Status = RtlpInitializeTimerThread();

        if (NT_SUCCESS(Status))
        {
            /* Queue an APC in the timer thread */
            Status = NtQueueApcThread(TimerThreadHandle,
                                      RtlpExecuteWorkItem,
                                      NULL,
                                      NULL,
                                      WorkItem);
        }
    }
    else
    {
        /* Queue an IO completion message */
        Status = NtSetIoCompletion(ThreadPoolCompletionPort,
                                   RtlpExecuteWorkItem,
                                   WorkItem,
                                   STATUS_SUCCESS,
                                   0);
    }

    if (!NT_SUCCESS(Status))
    {
        _InterlockedDecrement(&ThreadPoolWorkerThreadsRequests);

        if (WorkItem->Flags & WT_EXECUTELONGFUNCTION)
        {
            _InterlockedDecrement(&ThreadPoolWorkerThreadsLongRequests);
        }
    }

    return Status;
}

static VOID
NTAPI
RtlpExecuteIoWorkItem(IN OUT PVOID NormalContext,
                      IN OUT PVOID SystemArgument1,
                      IN OUT PVOID SystemArgument2)
{
    NTSTATUS Status;
    BOOLEAN Impersonated = FALSE;
    PRTLP_IOWORKERTHREAD IoThread = (PRTLP_IOWORKERTHREAD)NormalContext;
    RTLP_WORKITEM WorkItem = *(volatile RTLP_WORKITEM *)SystemArgument2;

    ASSERT(IoThread != NULL);

    RtlFreeHeap(RtlGetProcessHeap(),
                0,
                SystemArgument2);

    if (WorkItem.TokenHandle != NULL)
    {
        Status = NtSetInformationThread(NtCurrentThread(),
                                        ThreadImpersonationToken,
                                        &WorkItem.TokenHandle,
                                        sizeof(HANDLE));

        NtClose(WorkItem.TokenHandle);

        if (NT_SUCCESS(Status))
        {
            Impersonated = TRUE;
        }
    }

    _SEH_TRY
    {
        DPRINT("RtlpExecuteIoWorkItem: Function: 0x%p Context: 0x%p ImpersonationToken: 0x%p\n", WorkItem.Function, WorkItem.Context, WorkItem.TokenHandle);

        /* Execute the function */
        WorkItem.Function(WorkItem.Context);
    }
    _SEH_HANDLE
    {
        DPRINT1("Exception 0x%x while executing IO work item 0x%p\n", _SEH_GetExceptionCode(), WorkItem.Function);
    }
    _SEH_END;

    if (Impersonated)
    {
        WorkItem.TokenHandle = NULL;
        Status = NtSetInformationThread(NtCurrentThread(),
                                        ThreadImpersonationToken,
                                        &WorkItem.TokenHandle,
                                        sizeof(HANDLE));
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to revert worker thread to self!!! Status: 0x%x\n", Status);
        }
    }

    /* remove the long function flag */
    if (WorkItem.Flags & WT_EXECUTELONGFUNCTION)
    {
        Status = RtlEnterCriticalSection(&ThreadPoolLock);
        if (NT_SUCCESS(Status))
        {
            IoThread->Flags &= ~WT_EXECUTELONGFUNCTION;
            RtlLeaveCriticalSection(&ThreadPoolLock);
        }
    }

    /* update the requests counter */
    _InterlockedDecrement(&ThreadPoolIOWorkerThreadsRequests);

    if (WorkItem.Flags & WT_EXECUTELONGFUNCTION)
    {
        _InterlockedDecrement(&ThreadPoolIOWorkerThreadsLongRequests);
    }
}

static NTSTATUS
RtlpQueueIoWorkerThread(IN OUT PRTLP_WORKITEM WorkItem)
{
    PLIST_ENTRY CurrentEntry;
    PRTLP_IOWORKERTHREAD IoThread = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    if (WorkItem->Flags & WT_EXECUTEINPERSISTENTIOTHREAD)
    {
        if (PersistentIoThread != NULL)
        {
            /* We already have a persistent IO worker thread */
            IoThread = PersistentIoThread;
        }
        else
        {
            /* We're not aware of any persistent IO worker thread. Search for a unused
               worker thread that doesn't have a long function queued */
            CurrentEntry = ThreadPoolIOWorkerThreadsList.Flink;
            while (CurrentEntry != &ThreadPoolIOWorkerThreadsList)
            {
                IoThread = CONTAINING_RECORD(CurrentEntry,
                                             RTLP_IOWORKERTHREAD,
                                             ListEntry);

                if (!(IoThread->Flags & WT_EXECUTELONGFUNCTION))
                    break;

                CurrentEntry = CurrentEntry->Flink;
            }

            if (CurrentEntry != &ThreadPoolIOWorkerThreadsList)
            {
                /* Found a worker thread we can use. */
                ASSERT(IoThread != NULL);

                IoThread->Flags |= WT_EXECUTEINPERSISTENTIOTHREAD;
                PersistentIoThread = IoThread;
            }
            else
            {
                DPRINT1("Failed to find a worker thread for the persistent IO thread!\n");
                return STATUS_NO_MEMORY;
            }
        }
    }
    else
    {
        /* Find a worker thread that is not currently executing a long function */
        CurrentEntry = ThreadPoolIOWorkerThreadsList.Flink;
        while (CurrentEntry != &ThreadPoolIOWorkerThreadsList)
        {
            IoThread = CONTAINING_RECORD(CurrentEntry,
                                         RTLP_IOWORKERTHREAD,
                                         ListEntry);

            if (!(IoThread->Flags & WT_EXECUTELONGFUNCTION))
            {
                /* if we're trying to queue a long function then make sure we're not dealing
                   with the persistent thread */
                if ((WorkItem->Flags & WT_EXECUTELONGFUNCTION) && !(IoThread->Flags & WT_EXECUTEINPERSISTENTIOTHREAD))
                {
                    /* found a candidate */
                    break;
                }
            }

            CurrentEntry = CurrentEntry->Flink;
        }

        if (CurrentEntry == &ThreadPoolIOWorkerThreadsList)
        {
            /* Couldn't find an appropriate thread, see if we can use the persistent thread (if it exists) for now */
            if (ThreadPoolIOWorkerThreads == 0)
            {
                DPRINT1("Failed to find a worker thread for the work item 0x%p!\n");
                ASSERT(IsListEmpty(&ThreadPoolIOWorkerThreadsList));
                return STATUS_NO_MEMORY;
            }
            else
            {
                /* pick the first worker thread */
                CurrentEntry = ThreadPoolIOWorkerThreadsList.Flink;
                IoThread = CONTAINING_RECORD(CurrentEntry,
                                             RTLP_IOWORKERTHREAD,
                                             ListEntry);

                /* Since this might be the persistent worker thread, don't run as a
                   long function */
                WorkItem->Flags &= ~WT_EXECUTELONGFUNCTION;
            }
        }

        /* Move the picked thread to the end of the list. Since we're always searching
           from the beginning, this improves distribution of work items */
        RemoveEntryList(&IoThread->ListEntry);
        InsertTailList(&ThreadPoolIOWorkerThreadsList,
                       &IoThread->ListEntry);
    }

    ASSERT(IoThread != NULL);

    _InterlockedIncrement(&ThreadPoolIOWorkerThreadsRequests);

    if (WorkItem->Flags & WT_EXECUTELONGFUNCTION)
    {
        /* We're about to queue a long function, mark the thread */
        IoThread->Flags |= WT_EXECUTELONGFUNCTION;

        _InterlockedIncrement(&ThreadPoolIOWorkerThreadsLongRequests);
    }

    /* It's time to queue the work item */
    Status = NtQueueApcThread(IoThread->ThreadHandle,
                              RtlpExecuteIoWorkItem,
                              IoThread,
                              NULL,
                              WorkItem);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to queue APC for work item 0x%p\n", WorkItem->Function);
        _InterlockedDecrement(&ThreadPoolIOWorkerThreadsRequests);

        if (WorkItem->Flags & WT_EXECUTELONGFUNCTION)
        {
            _InterlockedDecrement(&ThreadPoolIOWorkerThreadsLongRequests);
        }
    }

    return Status;
}

static BOOLEAN
RtlpIsIoPending(IN HANDLE ThreadHandle  OPTIONAL)
{
    NTSTATUS Status;
    ULONG IoPending;
    BOOLEAN CreatedHandle = FALSE;
    BOOLEAN IsIoPending = TRUE;

    if (ThreadHandle == NULL)
    {
        Status = NtDuplicateObject(NtCurrentProcess(),
                                   NtCurrentThread(),
                                   NtCurrentProcess(),
                                   &ThreadHandle,
                                   0,
                                   0,
                                   DUPLICATE_SAME_ACCESS);
        if (!NT_SUCCESS(Status))
        {
            return IsIoPending;
        }

        CreatedHandle = TRUE;
    }

    Status = NtQueryInformationThread(ThreadHandle,
                                      ThreadIsIoPending,
                                      &IoPending,
                                      sizeof(IoPending),
                                      NULL);
    if (NT_SUCCESS(Status) && IoPending == 0)
    {
        IsIoPending = FALSE;
    }

    if (CreatedHandle)
    {
        NtClose(ThreadHandle);
    }

    return IsIoPending;
}

static ULONG
NTAPI
RtlpIoWorkerThreadProc(IN PVOID Parameter)
{
    volatile RTLP_IOWORKERTHREAD ThreadInfo;
    LARGE_INTEGER Timeout;
    BOOLEAN Terminate;
    NTSTATUS Status = STATUS_SUCCESS;

    if (_InterlockedIncrement(&ThreadPoolIOWorkerThreads) > MAX_WORKERTHREADS)
    {
        /* Oops, too many worker threads... */
        goto InitFailed;
    }

    /* Get a thread handle to ourselves */
    Status = NtDuplicateObject(NtCurrentProcess(),
                               NtCurrentThread(),
                               NtCurrentProcess(),
                               (PHANDLE)&ThreadInfo.ThreadHandle,
                               0,
                               0,
                               DUPLICATE_SAME_ACCESS);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create handle to own thread! Status: 0x%x\n", Status);

InitFailed:
        _InterlockedDecrement(&ThreadPoolIOWorkerThreads);

        /* Signal initialization completion */
        _InterlockedExchange((PLONG)Parameter,
                            1);

        RtlExitUserThread(Status);
        return 0;
    }

    ThreadInfo.Flags = 0;

    /* Insert the thread into the list */
    InsertHeadList((PLIST_ENTRY)&ThreadPoolIOWorkerThreadsList,
                   (PLIST_ENTRY)&ThreadInfo.ListEntry);

    /* Signal initialization completion */
    _InterlockedExchange((PLONG)Parameter,
                         1);

    for (;;)
    {
        Timeout.QuadPart = -50000000LL; /* Wait for 5 seconds by default */

Wait:
        do
        {
            /* Perform an alertable wait, the work items are going to be executed as APCs */
            Status = NtDelayExecution(TRUE,
                                      &Timeout);

            /* Loop as long as we executed an APC */
        } while (Status != STATUS_SUCCESS);

        /* We timed out, let's see if we're allowed to terminate */
        Terminate = FALSE;

        Status = RtlEnterCriticalSection(&ThreadPoolLock);
        if (NT_SUCCESS(Status))
        {
            if (ThreadInfo.Flags & WT_EXECUTEINPERSISTENTIOTHREAD)
            {
                /* This thread is supposed to be persistent. Don't terminate! */
                RtlLeaveCriticalSection(&ThreadPoolLock);

                Timeout.QuadPart = -0x7FFFFFFFFFFFFFFFLL;
                goto Wait;
            }

            /* FIXME - figure out an effective method to determine if it's appropriate to
                       lower the number of threads. For now let's always terminate if there's
                       at least one thread and no queued items. */
            Terminate = ((volatile LONG)ThreadPoolIOWorkerThreads - (volatile LONG)ThreadPoolIOWorkerThreadsLongRequests >= WORKERTHREAD_CREATION_THRESHOLD) &&
                        ((volatile LONG)ThreadPoolIOWorkerThreadsRequests == 0);

            if (Terminate)
            {
                /* Prevent termination as long as IO is pending */
                Terminate = !RtlpIsIoPending(ThreadInfo.ThreadHandle);
            }

            if (Terminate)
            {
                /* Rundown the thread and unlink it from the list */
                _InterlockedDecrement(&ThreadPoolIOWorkerThreads);
                RemoveEntryList((PLIST_ENTRY)&ThreadInfo.ListEntry);
            }

            RtlLeaveCriticalSection(&ThreadPoolLock);

            if (Terminate)
            {
                /* Break the infinite loop and terminate */
                Status = STATUS_SUCCESS;
                break;
            }
        }
        else
        {
            DPRINT1("Failed to acquire the thread pool lock!!! Status: 0x%x\n", Status);
            break;
        }
    }

    NtClose(ThreadInfo.ThreadHandle);
    RtlExitUserThread(Status);
    return 0;
}

static ULONG
NTAPI
RtlpWorkerThreadProc(IN PVOID Parameter)
{
    LARGE_INTEGER Timeout;
    BOOLEAN Terminate;
    PVOID SystemArgument2;
    IO_STATUS_BLOCK IoStatusBlock;
    ULONG TimeoutCount = 0;
    PKNORMAL_ROUTINE ApcRoutine;
    NTSTATUS Status = STATUS_SUCCESS;

    if (_InterlockedIncrement(&ThreadPoolWorkerThreads) > MAX_WORKERTHREADS)
    {
        /* Signal initialization completion */
        _InterlockedExchange((PLONG)Parameter,
                             1);

        /* Oops, too many worker threads... */
        RtlExitUserThread(Status);
        return 0;
    }

    /* Signal initialization completion */
    _InterlockedExchange((PLONG)Parameter,
                         1);

    for (;;)
    {
        Timeout.QuadPart = -50000000LL; /* Wait for 5 seconds by default */

        /* Dequeue a completion message */
        Status = NtRemoveIoCompletion(ThreadPoolCompletionPort,
                                      (PVOID*)&ApcRoutine,
                                      &SystemArgument2,
                                      &IoStatusBlock,
                                      &Timeout);

        if (Status == STATUS_SUCCESS)
        {
            TimeoutCount = 0;

            _SEH_TRY
            {
                /* Call the APC routine */
                ApcRoutine(NULL,
                           (PVOID)IoStatusBlock.Information,
                           SystemArgument2);
            }
            _SEH_HANDLE
            {
            }
            _SEH_END;
        }
        else
        {
            Terminate = FALSE;

            if (!NT_SUCCESS(RtlEnterCriticalSection(&ThreadPoolLock)))
                continue;

            /* FIXME - this should be optimized, check if there's requests, etc */

            if (Status == STATUS_TIMEOUT)
            {
                /* FIXME - we might want to optimize this */
                if (TimeoutCount++ > 2 &&
                    (volatile LONG)ThreadPoolWorkerThreads - (volatile LONG)ThreadPoolWorkerThreadsLongRequests >= WORKERTHREAD_CREATION_THRESHOLD)
                {
                    Terminate = TRUE;
                }
            }
            else
                Terminate = TRUE;

            RtlLeaveCriticalSection(&ThreadPoolLock);

            if (Terminate)
            {
                /* Prevent termination as long as IO is pending */
                Terminate = !RtlpIsIoPending(NULL);
            }

            if (Terminate)
            {
                _InterlockedDecrement(&ThreadPoolWorkerThreads);
                Status = STATUS_SUCCESS;
                break;
            }
        }
    }

    RtlExitUserThread(Status);
    return 0;

}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlQueueWorkItem(IN WORKERCALLBACKFUNC Function,
                 IN PVOID Context  OPTIONAL,
                 IN ULONG Flags)
{
    LONG FreeWorkers;
    NTSTATUS Status;
    PRTLP_WORKITEM WorkItem;

    DPRINT("RtlQueueWorkItem(0x%p, 0x%p, 0x%x)\n", Function, Context, Flags);

    /* Initialize the thread pool if not already initialized */
    if (!IsThreadPoolInitialized())
    {
        Status = RtlpInitializeThreadPool();

        if (!NT_SUCCESS(Status))
            return Status;
    }

    /* Allocate a work item */
    WorkItem = RtlAllocateHeap(RtlGetProcessHeap(),
                               0,
                               sizeof(RTLP_WORKITEM));
    if (WorkItem == NULL)
        return STATUS_NO_MEMORY;

    WorkItem->Function = Function;
    WorkItem->Context = Context;
    WorkItem->Flags = Flags;

    if (Flags & WT_TRANSFER_IMPERSONATION)
    {
        Status = RtlpGetImpersonationToken(&WorkItem->TokenHandle);

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to get impersonation token! Status: 0x%x\n", Status);
            goto Cleanup;
        }
    }
    else
        WorkItem->TokenHandle = NULL;

    Status = RtlEnterCriticalSection(&ThreadPoolLock);
    if (NT_SUCCESS(Status))
    {
        if (Flags & (WT_EXECUTEINIOTHREAD | WT_EXECUTEINUITHREAD | WT_EXECUTEINPERSISTENTIOTHREAD))
        {
            /* FIXME - We should optimize the algorithm used to determine whether to grow the thread pool! */

            FreeWorkers = ThreadPoolIOWorkerThreads - ThreadPoolIOWorkerThreadsLongRequests;

            if (((Flags & (WT_EXECUTEINPERSISTENTIOTHREAD | WT_EXECUTELONGFUNCTION)) == WT_EXECUTELONGFUNCTION) &&
                PersistentIoThread != NULL)
            {
                /* We shouldn't queue a long function into the persistent IO thread */
                FreeWorkers--;
            }

            /* See if it's a good idea to grow the pool */
            if (ThreadPoolIOWorkerThreads < MAX_WORKERTHREADS &&
                (FreeWorkers <= 0 || ThreadPoolIOWorkerThreads - ThreadPoolIOWorkerThreadsRequests < WORKERTHREAD_CREATION_THRESHOLD))
            {
                /* Grow the thread pool */
                Status = RtlpStartWorkerThread(RtlpIoWorkerThreadProc);

                if (!NT_SUCCESS(Status) && (volatile LONG)ThreadPoolIOWorkerThreads != 0)
                {
                    /* We failed to create the thread, but there's at least one there so
                       we can at least queue the request */
                    Status = STATUS_SUCCESS;
                }
            }

            if (NT_SUCCESS(Status))
            {
                /* Queue a IO worker thread */
                Status = RtlpQueueIoWorkerThread(WorkItem);
            }
        }
        else
        {
            /* FIXME - We should optimize the algorithm used to determine whether to grow the thread pool! */

            FreeWorkers = ThreadPoolWorkerThreads - ThreadPoolWorkerThreadsLongRequests;

            /* See if it's a good idea to grow the pool */
            if (ThreadPoolWorkerThreads < MAX_WORKERTHREADS &&
                (FreeWorkers <= 0 || ThreadPoolWorkerThreads - ThreadPoolWorkerThreadsRequests < WORKERTHREAD_CREATION_THRESHOLD))
            {
                /* Grow the thread pool */
                Status = RtlpStartWorkerThread(RtlpWorkerThreadProc);

                if (!NT_SUCCESS(Status) && (volatile LONG)ThreadPoolWorkerThreads != 0)
                {
                    /* We failed to create the thread, but there's at least one there so
                       we can at least queue the request */
                    Status = STATUS_SUCCESS;
                }
            }

            if (NT_SUCCESS(Status))
            {
                /* Queue a normal worker thread */
                Status = RtlpQueueWorkerThread(WorkItem);
            }
        }

        RtlLeaveCriticalSection(&ThreadPoolLock);
    }

    if (!NT_SUCCESS(Status))
    {
        if (WorkItem->TokenHandle != NULL)
        {
            NtClose(WorkItem->TokenHandle);
        }

Cleanup:
        RtlFreeHeap(RtlGetProcessHeap(),
                    0,
                    WorkItem);
    }

    return Status;
}
