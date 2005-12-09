/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Winsock 2 SPI
 * FILE:        lib/mswsock/lib/init.c
 * PURPOSE:     DLL Initialization
 */

/* INCLUDES ******************************************************************/
#include "msafd.h"

#define MSAFD_CHECK_EVENT(e, s) \
    (!(s->SharedData.AsyncDisabledEvents & e) && \
     (s->SharedData.AsyncEvents & e))

#define HANDLES_IN_SET(s) \
    s == NULL ? 0 : (s->fd_count & 0xFFFF)

/* DATA **********************************************************************/

HANDLE SockAsyncSelectHelperHandle;
BOOLEAN SockAsyncSelectCalled;

/* FUNCTIONS *****************************************************************/

BOOLEAN
WSPAPI
SockCheckAndInitAsyncSelectHelper(VOID)
{
    UNICODE_STRING AfdHelper;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    FILE_COMPLETION_INFORMATION CompletionInfo;
    OBJECT_HANDLE_ATTRIBUTE_INFORMATION HandleFlags;

    /* First, make sure we're not already intialized */
    if (SockAsyncSelectHelperHandle) return TRUE;

    /* Acquire the global lock */
    SockAcquireRwLockExclusive(&SocketGlobalLock);

    /* Check again, under the lock */
    if (SockAsyncSelectHelperHandle)
    {
        /* Return without lock */
        SockReleaseRwLockExclusive(&SocketGlobalLock);
        return TRUE;
    }

    /* Set up Handle Name and Object */
    RtlInitUnicodeString(&AfdHelper, L"\\Device\\Afd\\AsyncSelectHlp" );
    InitializeObjectAttributes(&ObjectAttributes,
                               &AfdHelper,
                               OBJ_INHERIT | OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* Open the Handle to AFD */
    Status = NtCreateFile(&SockAsyncSelectHelperHandle,                
                          GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          0,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_OPEN_IF,
                          0,
                          NULL,
                          0);
    if (!NT_SUCCESS(Status))
    {
        /* Return without lock */
        SockReleaseRwLockExclusive(&SocketGlobalLock);
        return TRUE;
    }

    /* Check if the port exists, and if not, create it */
    if (SockAsyncQueuePort) SockCreateAsyncQueuePort();

    /* 
     * Now Set up the Completion Port Information 
     * This means that whenever a Poll is finished, the routine will be executed
     */
    CompletionInfo.Port = SockAsyncQueuePort;
    CompletionInfo.Key = SockAsyncSelectCompletion;
    Status = NtSetInformationFile(SockAsyncSelectHelperHandle,
                                  &IoStatusBlock,
                                  &CompletionInfo,
                                  sizeof(CompletionInfo),
                                  FileCompletionInformation);
                      
    /* Protect the Handle */
    HandleFlags.ProtectFromClose = TRUE;
    HandleFlags.Inherit = FALSE;
    Status = NtSetInformationObject(SockAsyncQueuePort,
                                    ObjectHandleInformation,
                                    &HandleFlags,
                                    sizeof(HandleFlags));

    /*
     * Set this variable to true so that Send/Recv/Accept will know whether
     * to renable disabled events
     */
    SockAsyncSelectCalled = TRUE;

    /* Release lock and return */
    SockReleaseRwLockExclusive(&SocketGlobalLock);
    return TRUE;
}

VOID
WSPAPI
SockAsyncSelectCompletion(PVOID Context,
                          PIO_STATUS_BLOCK IoStatusBlock)
{
    PASYNC_DATA AsyncData = Context;
    PSOCKET_INFORMATION Socket = AsyncData->ParentSocket;
    ULONG Events;
    INT ErrorCode;

    /* Acquire the socket lock */
    EnterCriticalSection(&Socket->Lock);

    /* Check if the socket was closed or the I/O cancelled */
    if ((Socket->SharedData.State == SocketClosed) ||
        (IoStatusBlock->Status == STATUS_CANCELLED))
    {
        /* Return */
        LeaveCriticalSection(&Socket->Lock);
        goto error;
    }
    
    /* Check if the Sequence  Number Changed behind our back */
    if (AsyncData->SequenceNumber != Socket->SharedData.SequenceNumber)
    {
        /* Return */
        LeaveCriticalSection(&Socket->Lock);
        goto error;
    }

    /* Check we were manually called b/c of a failure */
    if (!NT_SUCCESS(IoStatusBlock->Status))
    {
        /* Get the error and tell WPU about it */
        ErrorCode = NtStatusToSocketError(IoStatusBlock->Status);
        SockUpcallTable->lpWPUPostMessage(Socket->SharedData.hWnd,
                                          Socket->SharedData.wMsg,
                                          Socket->Handle,
                                          WSAMAKESELECTREPLY(0, ErrorCode));

        /* Return */
        LeaveCriticalSection(&Socket->Lock);
        goto error;
    }

    /* Select the event bits */
    Events = AsyncData->AsyncSelectInfo.Handles[0].Events;

    /* Check for receive event */
    if (MSAFD_CHECK_EVENT(FD_READ, Socket) && (Events & AFD_EVENT_RECEIVE))
    {            
        /* Make the Notifcation */
        SockUpcallTable->lpWPUPostMessage(Socket->SharedData.hWnd,
                                          Socket->SharedData.wMsg,
                                          Socket->Handle,
                                          WSAMAKESELECTREPLY(FD_READ, 0));

        /* Disable this event until the next read(); */
        Socket->SharedData.AsyncDisabledEvents |= FD_READ;
    }

    /* Check for oob receive event */
    if (MSAFD_CHECK_EVENT(FD_OOB, Socket) && (Events & AFD_EVENT_OOB_RECEIVE))
    {            
        /* Make the Notifcation */
        SockUpcallTable->lpWPUPostMessage(Socket->SharedData.hWnd,
                                          Socket->SharedData.wMsg,
                                          Socket->Handle,
                                          WSAMAKESELECTREPLY(FD_OOB, 0));

        /* Disable this event until the next read(); */
        Socket->SharedData.AsyncDisabledEvents |= FD_OOB;
    }

    /* Check for write event */
    if (MSAFD_CHECK_EVENT(FD_WRITE, Socket) && (Events & AFD_EVENT_SEND))
    {            
        /* Make the Notifcation */
        SockUpcallTable->lpWPUPostMessage(Socket->SharedData.hWnd,
                                          Socket->SharedData.wMsg,
                                          Socket->Handle,
                                          WSAMAKESELECTREPLY(FD_WRITE, 0));

        /* Disable this event until the next read(); */
        Socket->SharedData.AsyncDisabledEvents |= FD_WRITE;
    }

    /* Check for accept event */
    if (MSAFD_CHECK_EVENT(FD_ACCEPT, Socket) && (Events & AFD_EVENT_ACCEPT))
    {            
        /* Make the Notifcation */
        SockUpcallTable->lpWPUPostMessage(Socket->SharedData.hWnd,
                                          Socket->SharedData.wMsg,
                                          Socket->Handle,
                                          WSAMAKESELECTREPLY(FD_ACCEPT, 0));

        /* Disable this event until the next read(); */
        Socket->SharedData.AsyncDisabledEvents |= FD_ACCEPT;
    }    

    /* Check for close events */
    if (MSAFD_CHECK_EVENT(FD_CLOSE, Socket) && ((Events & AFD_EVENT_ACCEPT) ||
                                                (Events & AFD_EVENT_ABORT) ||
                                                (Events & AFD_EVENT_CLOSE)))
    {            
        /* Make the Notifcation */
        SockUpcallTable->lpWPUPostMessage(Socket->SharedData.hWnd,
                                          Socket->SharedData.wMsg,
                                          Socket->Handle,
                                          WSAMAKESELECTREPLY(FD_CLOSE, 0));

        /* Disable this event until the next read(); */
        Socket->SharedData.AsyncDisabledEvents |= FD_CLOSE;
    }

    /* Check for QOS event */
    if (MSAFD_CHECK_EVENT(FD_QOS, Socket) && (Events & AFD_EVENT_QOS))
    {            
        /* Make the Notifcation */
        SockUpcallTable->lpWPUPostMessage(Socket->SharedData.hWnd,
                                          Socket->SharedData.wMsg,
                                          Socket->Handle,
                                          WSAMAKESELECTREPLY(FD_QOS, 0));

        /* Disable this event until the next read(); */
        Socket->SharedData.AsyncDisabledEvents |= FD_QOS;
    }

    /* Check for Group QOS event */
    if (MSAFD_CHECK_EVENT(FD_GROUP_QOS, Socket) && (Events & AFD_EVENT_GROUP_QOS))
    {            
        /* Make the Notifcation */
        SockUpcallTable->lpWPUPostMessage(Socket->SharedData.hWnd,
                                          Socket->SharedData.wMsg,
                                          Socket->Handle,
                                          WSAMAKESELECTREPLY(FD_GROUP_QOS, 0));

        /* Disable this event until the next read(); */
        Socket->SharedData.AsyncDisabledEvents |= FD_GROUP_QOS;
    }

    /* Check for Routing Interface Change event */
    if (MSAFD_CHECK_EVENT(FD_ROUTING_INTERFACE_CHANGE, Socket) &&
        (Events & AFD_EVENT_ROUTING_INTERFACE_CHANGE))
    {            
        /* Make the Notifcation */
        SockUpcallTable->lpWPUPostMessage(Socket->SharedData.hWnd,
                                          Socket->SharedData.wMsg,
                                          Socket->Handle,
                                          WSAMAKESELECTREPLY(FD_ROUTING_INTERFACE_CHANGE, 0));

        /* Disable this event until the next read(); */
        Socket->SharedData.AsyncDisabledEvents |= FD_ROUTING_INTERFACE_CHANGE;
    }

    /* Check for Address List Change event */
    if (MSAFD_CHECK_EVENT(FD_ADDRESS_LIST_CHANGE, Socket) &&
        (Events & AFD_EVENT_ADDRESS_LIST_CHANGE))
    {            
        /* Make the Notifcation */
        SockUpcallTable->lpWPUPostMessage(Socket->SharedData.hWnd,
                                          Socket->SharedData.wMsg,
                                          Socket->Handle,
                                          WSAMAKESELECTREPLY(FD_ADDRESS_LIST_CHANGE, 0));

        /* Disable this event until the next read(); */
        Socket->SharedData.AsyncDisabledEvents |= FD_ADDRESS_LIST_CHANGE;
    }
    
    /* Check if there are any events left for us to check */
    if (!((Socket->SharedData.AsyncEvents) &
          (~Socket->SharedData.AsyncDisabledEvents)))
    {
        /* Nothing left, release lock and return */
        LeaveCriticalSection(&Socket->Lock);
        goto error;
    }

    /* Keep Polling */
    SockProcessAsyncSelect(Socket, AsyncData);

    /* Leave lock and return */
    LeaveCriticalSection(&Socket->Lock);
    return;

error:
    /* Dereference the socket and free the Async Data */
    SockDereferenceSocket(Socket);
    RtlFreeHeap(SockPrivateHeap, 0, AsyncData);
    
    /* Dereference this thread and return */
    InterlockedDecrement(&SockAsyncThreadReferenceCount);
    return;
}

VOID
WSPAPI
SockProcessAsyncSelect(PSOCKET_INFORMATION Socket,
                       PASYNC_DATA AsyncData)
{
    ULONG lNetworkEvents;
    NTSTATUS Status;

    /* Set up the Async Data Event Info */
    AsyncData->AsyncSelectInfo.Timeout.HighPart = 0x7FFFFFFF;
    AsyncData->AsyncSelectInfo.Timeout.LowPart = 0xFFFFFFFF;
    AsyncData->AsyncSelectInfo.HandleCount = 1;
    AsyncData->AsyncSelectInfo.Exclusive = TRUE;
    AsyncData->AsyncSelectInfo.Handles[0].Handle = Socket->WshContext.Handle;
    AsyncData->AsyncSelectInfo.Handles[0].Events = 0;

    /* Remove unwanted events */
    lNetworkEvents = Socket->SharedData.AsyncEvents &
                     (~Socket->SharedData.AsyncDisabledEvents);

    /* Set Events to wait for */
    if (lNetworkEvents & FD_READ)
    {
        AsyncData->AsyncSelectInfo.Handles[0].Events |= AFD_EVENT_RECEIVE;
    }
    if (lNetworkEvents & FD_WRITE)
    {
        AsyncData->AsyncSelectInfo.Handles[0].Events |= AFD_EVENT_SEND;
    }
    if (lNetworkEvents & FD_OOB)
    {
        AsyncData->AsyncSelectInfo.Handles[0].Events |= AFD_EVENT_OOB_RECEIVE;
    }
    if (lNetworkEvents & FD_ACCEPT)
    {
        AsyncData->AsyncSelectInfo.Handles[0].Events |= AFD_EVENT_ACCEPT;
    }
    if (lNetworkEvents & FD_CLOSE)
    {
        AsyncData->AsyncSelectInfo.Handles[0].Events |= AFD_EVENT_DISCONNECT |
                                                        AFD_EVENT_ABORT |
                                                        AFD_EVENT_CLOSE;
    }
    if (lNetworkEvents & FD_QOS)
    {
        AsyncData->AsyncSelectInfo.Handles[0].Events |= AFD_EVENT_QOS;
    }
    if (lNetworkEvents & FD_GROUP_QOS)
    {
        AsyncData->AsyncSelectInfo.Handles[0].Events |= AFD_EVENT_GROUP_QOS;
    }
    if (lNetworkEvents & FD_ROUTING_INTERFACE_CHANGE)
    {
        AsyncData->AsyncSelectInfo.Handles[0].Events |= AFD_EVENT_ROUTING_INTERFACE_CHANGE;
    }
    if (lNetworkEvents & FD_ADDRESS_LIST_CHANGE)
    {
        AsyncData->AsyncSelectInfo.Handles[0].Events |= AFD_EVENT_ADDRESS_LIST_CHANGE;
    }
    
    /* Send IOCTL */
    Status = NtDeviceIoControlFile(SockAsyncSelectHelperHandle,
                                   NULL,
                                   NULL,
                                   AsyncData,
                                   &AsyncData->IoStatusBlock,
                                   IOCTL_AFD_SELECT,
                                   &AsyncData->AsyncSelectInfo,
                                   sizeof(AsyncData->AsyncSelectInfo),
                                   &AsyncData->AsyncSelectInfo,
                                   sizeof(AsyncData->AsyncSelectInfo));
    /* Check for failure */
    if (NT_ERROR(Status))
    {
        /* I/O Manager Won't call the completion routine; do it manually */
        AsyncData->IoStatusBlock.Status = Status;
        SockAsyncSelectCompletion(AsyncData, &AsyncData->IoStatusBlock);
    }
}

VOID
WSPAPI
SockProcessQueuedAsyncSelect(PVOID Context,
                             PIO_STATUS_BLOCK IoStatusBlock)
{
    PASYNC_DATA AsyncData = Context;
    BOOL FreeContext = TRUE;
    PSOCKET_INFORMATION Socket = AsyncData->ParentSocket;

    /* Lock the socket */
    EnterCriticalSection(&Socket->Lock);

    /* Make sure it's not closed */
    if (Socket->SharedData.State == SocketClosed)
    {
        /* Return */
        LeaveCriticalSection(&Socket->Lock);
        goto error;
    }

    /* Check if the Sequence Number changed by now */
    if (AsyncData->SequenceNumber != Socket->SharedData.SequenceNumber)
    {
        /* Return */
        LeaveCriticalSection(&Socket->Lock);
        goto error;
    }

    /* Check if select is needed */
    if (!((Socket->SharedData.AsyncEvents &
          ~Socket->SharedData.AsyncDisabledEvents)))
    {
        /* Return */
        LeaveCriticalSection(&Socket->Lock);
        goto error;
    }
                
    /* Do the actual select */
    SockProcessAsyncSelect(Socket, AsyncData);

    /* Return */
    LeaveCriticalSection(&Socket->Lock);
    return;

error:
    /* Dereference the socket and free the async data */
    SockDereferenceSocket(Socket);
    RtlFreeHeap(SockPrivateHeap, 0, AsyncData);

    /* Dereference this thread */
    InterlockedDecrement(&SockAsyncThreadReferenceCount);
}

INT
WSPAPI
SockReenableAsyncSelectEvent(IN PSOCKET_INFORMATION Socket,
                             IN ULONG Event)
{
    PASYNC_DATA AsyncData;
    NTSTATUS Status;

    /* Make sure the event is actually disabled */
    if (!(Socket->SharedData.AsyncDisabledEvents & Event)) return NO_ERROR;

    /* Make sure we're not closed */
    if (Socket->SharedData.State == SocketClosed) return NO_ERROR;

    /* Re-enable it */
    Socket->SharedData.AsyncDisabledEvents &= ~Event;

    /* Return if no more events are being polled */
    if (!((Socket->SharedData.AsyncEvents &
          ~Socket->SharedData.AsyncDisabledEvents)))
    {
        return NO_ERROR;
    }

    /* Allocate Async Data */
    AsyncData = SockAllocateHeapRoutine(SockPrivateHeap, 0, sizeof(ASYNC_DATA));

    /* Increase the sequence number to stop anything else */
    Socket->SharedData.SequenceNumber++;
    
    /* Set up the Async Data */
    AsyncData->ParentSocket = Socket;
    AsyncData->SequenceNumber = Socket->SharedData.SequenceNumber;

    /* Begin Async Select by using I/O Completion */
    Status = NtSetIoCompletion(SockAsyncQueuePort,
                               (PVOID)&SockProcessQueuedAsyncSelect,
                               AsyncData,
                               0,
                               0);
    if (!NT_SUCCESS(Status))
    {
        /* Dereference the socket and fail */
        SockDereferenceSocket(Socket);
        RtlFreeHeap(SockPrivateHeap, 0, AsyncData);
        return NtStatusToSocketError(Status);
    }
    
    /* All done */
    return NO_ERROR;
}

INT
WSPAPI
SockAsyncSelectHelper(IN PSOCKET_INFORMATION Socket, 
                      IN HWND hWnd, 
                      IN UINT wMsg, 
                      IN LONG lEvent)
{
    PASYNC_DATA AsyncData = NULL;
    BOOLEAN BlockMode;
    NTSTATUS Status;
    INT ErrorCode;

    /* Allocate the Async Data Structure to pass on to the Thread later */
    AsyncData = SockAllocateHeapRoutine(SockPrivateHeap, 0, sizeof(*AsyncData));
    if (!AsyncData) return WSAENOBUFS;

    /* Acquire socket lock */
    EnterCriticalSection(&Socket->Lock);

    /* Is there an active WSPEventSelect? */
    if (Socket->SharedData.AsyncEvents)
    {
        /* Call the helper to process it */
        ErrorCode = SockEventSelectHelper(Socket, NULL, 0);
        if (ErrorCode != NO_ERROR) goto error;
    }

    /* Set Socket to Non-Blocking */
    BlockMode = TRUE;
    ErrorCode = SockSetInformation(Socket,
                                   AFD_INFO_BLOCKING_MODE,
                                   &BlockMode,
                                   NULL,
                                   NULL);
    if (ErrorCode != NO_ERROR) goto error;

    /* AFD was notified, set it locally as well */
    Socket->SharedData.NonBlocking = TRUE;

    /* Store Socket Data */
    Socket->SharedData.hWnd = hWnd;
    Socket->SharedData.wMsg = wMsg;
    Socket->SharedData.AsyncEvents = lEvent;
    Socket->SharedData.AsyncDisabledEvents = 0;

    /* Check if the socket is not connected and not a datagram socket */
    if ((!SockIsSocketConnected(Socket)) && !MSAFD_IS_DGRAM_SOCK(Socket))
    {
        /* Disable FD_WRITE for now, so we don't get it before FD_CONNECT */
        Socket->SharedData.AsyncDisabledEvents |= FD_WRITE;
    }

    /* Increase the sequence number */
    Socket->SharedData.SequenceNumber++;

    /* Return if there are no more Events */
    if (!(Socket->SharedData.AsyncEvents &
        (~Socket->SharedData.AsyncDisabledEvents)))
    {
        /* Release the lock, dereference the async thread and the socket */
        LeaveCriticalSection(&Socket->Lock);
        InterlockedDecrement(&SockAsyncThreadReferenceCount);
        SockDereferenceSocket(Socket);

        /* Free the Async Data */
        RtlFreeHeap(SockPrivateHeap, 0, AsyncData);
        return NO_ERROR;
    }

    /* Set up the Async Data */
    AsyncData->ParentSocket = Socket;
    AsyncData->SequenceNumber = Socket->SharedData.SequenceNumber;

    /* Release the lock now */
    LeaveCriticalSection(&Socket->Lock);

    /* Begin Async Select by using I/O Completion */
    Status = NtSetIoCompletion(SockAsyncQueuePort,
                               (PVOID)&SockProcessQueuedAsyncSelect,
                               AsyncData,
                               0,
                               0);
    if (!NT_SUCCESS(Status))
    {
        /* Dereference the async thread and fail */
        InterlockedDecrement(&SockAsyncThreadReferenceCount);
        ErrorCode = NtStatusToSocketError(Status);
    }

error:
    /* Check for error */
    if (ErrorCode != NO_ERROR)
    {
        /* Free the async data */
        RtlFreeHeap(SockPrivateHeap, 0, AsyncData);

        /* Fail */
        return SOCKET_ERROR;
    }

    /* Increment the socket reference */
    InterlockedIncrement(&Socket->RefCount);

    /* Return success */
    return NO_ERROR;
}

INT
WSPAPI
WSPAsyncSelect(IN SOCKET Handle, 
               IN HWND hWnd, 
               IN UINT wMsg, 
               IN LONG lEvent, 
               OUT LPINT lpErrno)
{
    PSOCKET_INFORMATION Socket;
    INT ErrorCode;
    PWINSOCK_TEB_DATA ThreadData;

    /* Enter prolog */
    ErrorCode = SockEnterApiFast(&ThreadData);
    if (ErrorCode != NO_ERROR)
    {
        /* Fail */
        *lpErrno = ErrorCode;
        return SOCKET_ERROR;
    }

    /* Check for valid events */
    if (lEvent & ~FD_ALL_EVENTS)
    {
        /* Fail */
        ErrorCode = WSAEINVAL;
        goto error;
    }

    /* Check for valid window handle */
    if (!IsWindow(hWnd))
    {
        /* Fail */
        ErrorCode = WSAEINVAL;
        goto error;
    }

    /* Create the Asynch Thread if Needed */  
    if (!SockCheckAndReferenceAsyncThread())
    {
        /* Fail */
        ErrorCode = WSAENOBUFS;
        goto error;
    }
    
    /* Open a Handle to AFD's Async Helper */
    if (!SockCheckAndInitAsyncSelectHelper())
    {
        /* Dereference async thread and fail */
        InterlockedDecrement(&SockAsyncThreadReferenceCount);
        ErrorCode = WSAENOBUFS;
        goto error;
    }

    /* Get the socket structure */
    Socket = SockFindAndReferenceSocket(Handle, TRUE);
    if (!Socket)
    {
        /* Fail */
        ErrorCode = WSAENOTSOCK;
        goto error;
    }

    /* Call the helper to do the work */
    ErrorCode = SockAsyncSelectHelper(Socket,
                                      hWnd,
                                      wMsg,
                                      lEvent);

    /* Dereference the socket */
    SockDereferenceSocket(Socket);

error:
    /* Check for error */
    if (ErrorCode != NO_ERROR)
    {
        /* Fail */
        *lpErrno = ErrorCode;
        return SOCKET_ERROR;
    }

    /* Return success */
    return NO_ERROR;
}

INT
WSPAPI 
WSPSelect(INT nfds, 
          PFD_SET readfds,
          PFD_SET writefds, 
          PFD_SET exceptfds, 
          IN CONST struct timeval *timeout, 
          LPINT lpErrno)
{
    IO_STATUS_BLOCK IoStatusBlock;
    PAFD_POLL_INFO PollInfo = NULL;
    NTSTATUS Status;
    CHAR PollBuffer[sizeof(AFD_POLL_INFO) + 3 * sizeof(AFD_HANDLE)];
    PAFD_HANDLE HandleArray;
    ULONG HandleCount, OutCount = 0;
    ULONG PollBufferSize;
    ULONG i;
    PWINSOCK_TEB_DATA ThreadData;
    LARGE_INTEGER uSec;
    ULONG BlockType;
    INT ErrorCode;

    /* Enter prolog */
    ErrorCode = SockEnterApiFast(&ThreadData);
    if (ErrorCode != NO_ERROR)
    {
        /* Fail */
        *lpErrno = ErrorCode;
        return SOCKET_ERROR;
    }
    
    /* How many sockets will we check? */
    HandleCount = HANDLES_IN_SET(readfds) + 
                  HANDLES_IN_SET(writefds) + 
                  HANDLES_IN_SET(exceptfds);

    /* Leave if none are */
    if (!HandleCount) return NO_ERROR;

    /* How much space will they require? */
    PollBufferSize = sizeof(*PollInfo) + (HandleCount * sizeof(AFD_HANDLE));

    /* Check if our stack is big enough to hold it */
    if (PollBufferSize <= sizeof(PollBuffer))
    {
        /* Use the stack */
        PollInfo = (PVOID)PollBuffer;
    }
    else
    {
        /* Allocate from heap instead */
        PollInfo = SockAllocateHeapRoutine(SockPrivateHeap, 0, PollBufferSize);
        if (!PollInfo)
        {
            /* Fail */
            ErrorCode = WSAENOBUFS;
            goto error;
        }
    }
   
    /* Number of handles for AFD to Check */
    PollInfo->HandleCount = HandleCount;
    PollInfo->Exclusive = FALSE;
    HandleArray = PollInfo->Handles;
   
    /* Select the Read Events */
    for (i = 0; readfds && i < (readfds->fd_count & 0xFFFF); i++)
    {
        /* Fill out handle info */
        HandleArray->Handle = (HANDLE)readfds->fd_array[i];
        HandleArray->Events = AFD_EVENT_RECEIVE | 
                              AFD_EVENT_DISCONNECT |
                              AFD_EVENT_ABORT;

        /* Move to the next one */
        HandleArray++;
    }
    for (i = 0; writefds && i < (writefds->fd_count & 0xFFFF); i++)
    {
        /* Fill out handle info */
        HandleArray->Handle = (HANDLE)writefds->fd_array[i];
        HandleArray->Events = AFD_EVENT_SEND;

        /* Move to the next one */
        HandleArray++;
    }
    for (i = 0; exceptfds && i < (exceptfds->fd_count & 0xFFFF); i++)
    {
        /* Fill out handle info */
        HandleArray->Handle = (HANDLE)exceptfds->fd_array[i];
        HandleArray->Events = AFD_EVENT_OOB_RECEIVE | AFD_EVENT_CONNECT_FAIL;

        /* Move to the next one */
        HandleArray++;
    }
    
    /* Check if a timeout was given */
    if (timeout) 
    {
        /* Inifinte Timeout */
        PollInfo->Timeout.u.LowPart = -1;
        PollInfo->Timeout.u.HighPart = 0x7FFFFFFF;
    } 
    else 
    {
        /* Calculate microseconds */
        uSec = RtlEnlargedIntegerMultiply(timeout->tv_usec, -10);

        /* Calculate seconds */
        PollInfo->Timeout = RtlEnlargedIntegerMultiply(timeout->tv_sec,
                                                       -1 * 1000 * 1000 * 10);

        /* Add microseconds */
        PollInfo->Timeout.QuadPart += uSec.QuadPart;
    }

    /* Send IOCTL */
    Status = NtDeviceIoControlFile(PollInfo->Handles[0].Handle,
                                   ThreadData->EventHandle,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_AFD_SELECT,
                                   PollInfo,
                                   PollBufferSize,
                                   PollInfo,
                                   PollBufferSize);
    
    /* Check if we have to wait */
    if (Status == STATUS_PENDING)
    {
        /* Check if we'll call the blocking hook */
        if (!PollInfo->Timeout.QuadPart) BlockType = NO_BLOCKING_HOOK;

        /* Wait for completion */
        SockWaitForSingleObject(ThreadData->EventHandle,
                                (SOCKET)PollInfo->Handles[0].Handle,
                                BlockType,
                                NO_TIMEOUT);

        /* Get new status */
        Status = IoStatusBlock.Status;
    }

    /* Check for failure */
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        ErrorCode = NtStatusToSocketError(Status);
        goto error;
    }

    /* Clear the Structures */
    if(readfds) FD_ZERO(readfds);
    if(writefds) FD_ZERO(writefds);
    if(exceptfds) FD_ZERO(exceptfds);
    
    /* Get the handle info again */
    HandleCount = PollInfo->HandleCount;
    HandleArray = PollInfo->Handles;
    
    /* Loop the Handles that got an event */
    for (i = 0; i < HandleCount; i++) 
    {
        /* Check for a match */
        if (HandleArray->Events & AFD_EVENT_RECEIVE) 
        {
            /* Check if it's not already set */
            if (!FD_ISSET((SOCKET)HandleArray->Handle, readfds))
            {   
                /* Increase Handles with an Event */
                OutCount++;

                /* Set this handle */
                FD_SET((SOCKET)HandleArray->Handle, readfds);
            }
        }
        /* Check for a match */
        if (HandleArray->Events & AFD_EVENT_SEND) 
        {
            /* Check if it's not already set */
            if (!FD_ISSET((SOCKET)HandleArray->Handle, writefds))
            {   
                /* Increase Handles with an Event */
                OutCount++;

                /* Set this handle */
                FD_SET((SOCKET)HandleArray->Handle, writefds);
            }
        }
        /* Check for a match */
        if (HandleArray->Events & AFD_EVENT_OOB_RECEIVE) 
        {
            /* Check if it's not already set */
            if (!FD_ISSET((SOCKET)HandleArray->Handle, exceptfds))
            {   
                /* Increase Handles with an Event */
                OutCount++;

                /* Set this handle */
                FD_SET((SOCKET)HandleArray->Handle, exceptfds);
            }
        }
        /* Check for a match */
        if (HandleArray->Events & AFD_EVENT_ACCEPT) 
        {
            /* Check if it's not already set */
            if (!FD_ISSET((SOCKET)HandleArray->Handle, readfds))
            {   
                /* Increase Handles with an Event */
                OutCount++;

                /* Set this handle */
                FD_SET((SOCKET)HandleArray->Handle, readfds);
            }
        }
        /* Check for a match */
        if (HandleArray->Events & AFD_EVENT_CONNECT) 
        {
            /* Check if it's not already set */
            if (!FD_ISSET((SOCKET)HandleArray->Handle, writefds))
            {   
                /* Increase Handles with an Event */
                OutCount++;

                /* Set this handle */
                FD_SET((SOCKET)HandleArray->Handle, writefds);
            }
        }
        /* Check for a match */
        if (HandleArray->Events & AFD_EVENT_CONNECT_FAIL) 
        {
            /* Check if it's not already set */
            if (!FD_ISSET((SOCKET)HandleArray->Handle, exceptfds))
            {   
                /* Increase Handles with an Event */
                OutCount++;

                /* Set this handle */
                FD_SET((SOCKET)HandleArray->Handle, exceptfds);
            }
        }
        /* Check for a match */
        if (HandleArray->Events & AFD_EVENT_DISCONNECT) 
        {
            /* Check if it's not already set */
            if (!FD_ISSET((SOCKET)HandleArray->Handle, readfds))
            {   
                /* Increase Handles with an Event */
                OutCount++;

                /* Set this handle */
                FD_SET((SOCKET)HandleArray->Handle, readfds);
            }
        }
        /* Check for a match */
        if (HandleArray->Events & AFD_EVENT_ABORT) 
        {
            /* Check if it's not already set */
            if (!FD_ISSET((SOCKET)HandleArray->Handle, readfds))
            {   
                /* Increase Handles with an Event */
                OutCount++;

                /* Set this handle */
                FD_SET((SOCKET)HandleArray->Handle, readfds);
            }
        }
        /* Check for a match */
        if (HandleArray->Events & AFD_EVENT_CLOSE) 
        {
            /* Check if it's not already set */
            if (!FD_ISSET((SOCKET)HandleArray->Handle, readfds))
            {   
                /* Increase Handles with an Event */
                OutCount++;

                /* Set this handle */
                FD_SET((SOCKET)HandleArray->Handle, readfds);
            }
        }

        /* Move to next entry */
        HandleArray++;
    }

error:

    /* Check if we should free the buffer */
    if (PollInfo && (PollInfo != (PVOID)PollBuffer))
    {
        /* Free it from the heap */
        RtlFreeHeap(SockPrivateHeap, 0, PollInfo);
    }

    /* Check for error */
    if (ErrorCode != NO_ERROR)
    {
        /* Return error */
        *lpErrno = ErrorCode;
        return SOCKET_ERROR;
    }

    /* Return the number of handles */
    return OutCount;
}
