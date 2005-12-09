/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Winsock 2 SPI
 * FILE:        lib/mswsock/lib/init.c
 * PURPOSE:     DLL Initialization
 */

/* INCLUDES ******************************************************************/
#include "msafd.h"

/* DATA **********************************************************************/

HANDLE SockAsyncQueuePort;
LONG SockAsyncThreadReferenceCount;

/* FUNCTIONS *****************************************************************/

INT
WSPAPI
SockCreateAsyncQueuePort(VOID)
{
    NTSTATUS Status;
    OBJECT_HANDLE_ATTRIBUTE_INFORMATION HandleFlags;

    /* Create the port */
    Status = NtCreateIoCompletion(&SockAsyncQueuePort,
                                  IO_COMPLETION_ALL_ACCESS,
                                  NULL,
                                  -1);
    
    /* Protect Handle */    
    HandleFlags.ProtectFromClose = TRUE;
    HandleFlags.Inherit = FALSE;
    Status = NtSetInformationObject(SockAsyncQueuePort,
                                    ObjectHandleInformation,
                                    &HandleFlags,
                                    sizeof(HandleFlags));

    /* Return */
    return NO_ERROR;
}

VOID
WSPAPI
SockHandleAsyncIndication(IN PASYNC_COMPLETION_ROUTINE Callback,
                          IN PVOID Context,
                          IN PIO_STATUS_BLOCK IoStatusBlock)
{
    /* Call the completion routine */
    (*Callback)(Context, IoStatusBlock);
}

BOOLEAN
WSPAPI
SockCheckAndReferenceAsyncThread(VOID)
{
    LONG Count;
    HANDLE hAsyncThread;
    DWORD AsyncThreadId;
    HANDLE AsyncEvent;
    NTSTATUS Status;
    INT ErrorCode;
    HINSTANCE hInstance;
    PWINSOCK_TEB_DATA ThreadData;

    /* Loop while trying to increase the reference count */
    do
    {
        /* Get the count, and check if it's already been started */
        Count = SockAsyncThreadReferenceCount;
        if ((Count > 0) && (InterlockedCompareExchange(&SockAsyncThreadReferenceCount,
                                                       Count + 1,
                                                       Count) == Count))
        {
            /* Simply return */
            return TRUE;
        }
    } while (Count > 0);

    /* Acquire the lock */
    SockAcquireRwLockExclusive(&SocketGlobalLock);
    
    /* Check if no completion port exists already and create it */
    if (!SockAsyncQueuePort) SockCreateAsyncQueuePort();

    /* Create an extra reference so the thread stays alive */
    ErrorCode = GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                                  (LPSTR)WSPStartup,
                                  &hInstance);
    
    /* Create the Async Event */
    Status = NtCreateEvent(&AsyncEvent,
                           EVENT_ALL_ACCESS,
                           NULL,
                           NotificationEvent,
                           FALSE);

    /* Allocate the TEB Block */
    ThreadData = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(*ThreadData));
    if (!ThreadData)
    {
        /* Release the lock, close the event, free extra reference and fail */
        SockReleaseRwLockExclusive(&SocketGlobalLock);
        NtClose(AsyncEvent);
        FreeLibrary(hInstance);
        return FALSE;
    }

    /* Initialize thread data */
    RtlZeroMemory(ThreadData, sizeof(*ThreadData));
    ThreadData->EventHandle = AsyncEvent;
    ThreadData->SocketHandle = (SOCKET)hInstance;
    
    /* Create the Async Thread */
    hAsyncThread = CreateThread(NULL,
                                0,
                                (LPTHREAD_START_ROUTINE)SockAsyncThread,
                                ThreadData,
                                0,
                                &AsyncThreadId);

    /* Close the Handle */
    NtClose(hAsyncThread);

    /* Increase the Reference Count */
    InterlockedExchangeAdd(&SockAsyncThreadReferenceCount, 2);

    /* Release lock and return success */
    SockReleaseRwLockExclusive(&SocketGlobalLock);
    return TRUE;
}

INT
WSPAPI
SockAsyncThread(PVOID Context)
{
    PVOID AsyncContext;
    PASYNC_COMPLETION_ROUTINE AsyncCompletionRoutine;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    LARGE_INTEGER Timeout;
    PWINSOCK_TEB_DATA ThreadData = (PWINSOCK_TEB_DATA)Context;
    HINSTANCE hInstance = (HINSTANCE)ThreadData->SocketHandle;

    /* Return the socket handle back to its unhacked value */
    ThreadData->SocketHandle = INVALID_SOCKET;

    /* Setup the Thread Data pointer */
    NtCurrentTeb()->WinSockData = ThreadData;
                          
    /* Make the Thread Higher Priority */
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

    /* Setup timeout */
    Timeout.QuadPart = Int32x32To64(300, 10000000);
    
    /* Do a KQUEUE/WorkItem Style Loop, thanks to IoCompletion Ports */
    do {
        /* Get the next completion item */
        Status = NtRemoveIoCompletion(SockAsyncQueuePort,
                                      (PVOID*)&AsyncCompletionRoutine,
                                      &AsyncContext,
                                      &IoStatusBlock,
                                      &Timeout);
        /* Check for success */
        if (NT_SUCCESS(Status))
        {
            /* Check if this isn't the termination command */
            if (AsyncCompletionRoutine != (PVOID)-1)
            {
                /* Call the routine */
                SockHandleAsyncIndication(AsyncCompletionRoutine,
                                          Context,
                                          &IoStatusBlock);
            }
            else
            {
                /* We have to terminate, fake a timeout */
                Status = STATUS_TIMEOUT;
                InterlockedDecrement(&SockAsyncThreadReferenceCount);
            }
        }
        else if ((SockAsyncThreadReferenceCount > 1) && (NT_ERROR(Status)))
        {
            /* It Failed, sleep for a second */
            Sleep(1000);
        }
    } while (((Status != STATUS_TIMEOUT) &&
              (SockWspStartupCount > 0)) ||
              InterlockedCompareExchange(&SockAsyncThreadReferenceCount, 0, 1) != 1);

    /* Release the lock */
    SockReleaseRwLockShared(&SocketGlobalLock);

    /* Remove our extra reference */
    FreeLibraryAndExitThread(hInstance, NO_ERROR);
}