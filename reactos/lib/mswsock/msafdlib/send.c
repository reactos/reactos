/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Winsock 2 SPI
 * FILE:        lib/mswsock/lib/init.c
 * PURPOSE:     DLL Initialization
 */

/* INCLUDES ******************************************************************/
#include "msafd.h"

/* DATA **********************************************************************/

/* FUNCTIONS *****************************************************************/

INT
WSPAPI 
WSPSend(SOCKET Handle, 
        LPWSABUF lpBuffers, 
        DWORD dwBufferCount, 
        LPDWORD lpNumberOfBytesSent, 
        DWORD iFlags, 
        LPWSAOVERLAPPED lpOverlapped, 
        LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine, 
        LPWSATHREADID lpThreadId, 
        LPINT lpErrno)
{
    PIO_STATUS_BLOCK IoStatusBlock;
    IO_STATUS_BLOCK DummyIoStatusBlock;
    AFD_SEND_INFO SendInfo;
    NTSTATUS Status;
    PVOID APCContext;
    PVOID ApcFunction;
    HANDLE Event;
    PWINSOCK_TEB_DATA ThreadData;
    PSOCKET_INFORMATION Socket;
    INT ErrorCode;
    BOOLEAN ReturnValue;

    /* Enter prolog */
    ErrorCode = SockEnterApiFast(&ThreadData);
    if (ErrorCode != NO_ERROR)
    {
        /* Fail */
        *lpErrno = ErrorCode;
        return SOCKET_ERROR;
    }

    /* Set up the Receive Structure */
    SendInfo.BufferArray = lpBuffers;
    SendInfo.BufferCount = dwBufferCount;
    SendInfo.TdiFlags = 0;
    SendInfo.AfdFlags = 0;

    /* Set the TDI Flags */
    if (iFlags)
    {
        /* Check for valid flags */
        if ((iFlags & ~(MSG_OOB | MSG_DONTROUTE | MSG_PARTIAL)))
        {
            /* Fail */
            ErrorCode = WSAEOPNOTSUPP;
            goto error;
        }
    
        /* Check if OOB is being used */
        if (iFlags & MSG_OOB)
        {
            /* Use Expedited Send for OOB */
            SendInfo.TdiFlags |= TDI_SEND_EXPEDITED;
        }

        /* Use Partial Send if enabled */
        if (iFlags & MSG_PARTIAL) SendInfo.TdiFlags |= TDI_SEND_PARTIAL;
    }

    /* Verifiy if we should use APC */
    if (!lpOverlapped)
    {
        /* Not using Overlapped structure, so use normal blocking on event */
        APCContext = NULL;
        ApcFunction = NULL;
        Event = ThreadData->EventHandle;
        IoStatusBlock = &DummyIoStatusBlock;
    }
    else
    {
        /* Using apc, check if we have a completion routine */
        if (!lpCompletionRoutine)
        {
            /* No need for APC */
            APCContext = lpOverlapped;
            ApcFunction = NULL;
            Event = lpOverlapped->hEvent;
        }
        else
        {
            /* Use APC */
            ApcFunction = SockIoCompletion;
            APCContext = lpCompletionRoutine;
            Event = NULL;

            /* Skip Fast I/O */
            SendInfo.AfdFlags = AFD_SKIP_FIO;
        }

        /* Use the overlapped's structure buffer for the I/O Status Block */
        IoStatusBlock = (PIO_STATUS_BLOCK)&lpOverlapped->Internal;

        /* Make this an overlapped I/O in AFD */
        SendInfo.AfdFlags |= AFD_OVERLAPPED;
    }

    /* Set is as Pending for now */
    IoStatusBlock->Status = STATUS_PENDING;

    /* Send IOCTL */
    Status = NtDeviceIoControlFile((HANDLE)Handle,
                                   Event,
                                   ApcFunction,
                                   APCContext,
                                   IoStatusBlock,
                                   IOCTL_AFD_SEND,
                                   &SendInfo,
                                   sizeof(SendInfo),
                                   NULL,
                                   0);

    /* Increase the pending APC Count if we're using an APC */
    if (!NT_ERROR(Status) && ApcFunction)
    {
        ThreadData->PendingAPCs++;
        InterlockedIncrement(&SockProcessPendingAPCCount);
    }

    /* Wait for completition if not overlapped */
    if ((Status == STATUS_PENDING) && !(lpOverlapped))
    {
        /* Wait for completion */
        ReturnValue = SockWaitForSingleObject(Event,
                                              Handle,
                                              MAYBE_BLOCKING_HOOK,
                                              SEND_TIMEOUT);

        /* Check if the wait was successful */
        if (ReturnValue)
        {
            /* Get new status */
            Status = IoStatusBlock->Status;
        }
        else
        {
            /* Cancel the I/O */
            SockCancelIo(Handle);
            Status = STATUS_IO_TIMEOUT;
        }
    }

    /* Check status */
    switch (Status)
    {
        /* Success */
        case STATUS_SUCCESS:
            break;

        /* Pending I/O */
        case STATUS_PENDING:
            ErrorCode = WSA_IO_PENDING;
            goto error;

        /* Other NT Error */
        default:
            if (!NT_SUCCESS(Status))
            {
                /* Fail */
                ErrorCode = NtStatusToSocketError(Status);
                goto error;
            }
            break;
    }

    /* Return the number of bytes sent */
    *lpNumberOfBytesSent = PtrToUlong(IoStatusBlock->Information);

error:

    /* Check if async select was active and this blocked */
    if (SockAsyncSelectCalled && (ErrorCode == WSAEWOULDBLOCK))
    {
        /* Get the socket */
        Socket = SockFindAndReferenceSocket(Handle, TRUE);
        if (Socket)
        {
            /* Lock it */
            EnterCriticalSection(&Socket->Lock);

            /* Re-enable the regular write event */
            SockReenableAsyncSelectEvent(Socket, FD_WRITE);

            /* Unlock and dereference socket */
            LeaveCriticalSection(&Socket->Lock);
            SockDereferenceSocket(Socket);
        }
    }

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
WSPSendTo(SOCKET Handle, 
          LPWSABUF lpBuffers, 
          DWORD dwBufferCount, 
          LPDWORD lpNumberOfBytesSent, 
          DWORD iFlags, 
          const struct sockaddr *SocketAddress, 
          INT SocketAddressLength, 
          LPWSAOVERLAPPED lpOverlapped, 
          LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine, 
          LPWSATHREADID lpThreadId, 
          LPINT lpErrno)
{
    PIO_STATUS_BLOCK IoStatusBlock;
    IO_STATUS_BLOCK DummyIoStatusBlock;
    AFD_SEND_INFO_UDP SendInfo;
    NTSTATUS Status;
    PVOID APCContext;
    PVOID ApcFunction;
    HANDLE Event;
    PWINSOCK_TEB_DATA ThreadData;
    PSOCKET_INFORMATION Socket;
    INT ErrorCode;
    BOOLEAN ReturnValue;
    CHAR AddressBuffer[FIELD_OFFSET(TDI_ADDRESS_INFO, Address) +
                       MAX_TDI_ADDRESS_LENGTH];
    PTRANSPORT_ADDRESS TdiAddress = (PTRANSPORT_ADDRESS)AddressBuffer;
    ULONG TdiAddressSize;
    DWORD SockaddrLength;
    PSOCKADDR Sockaddr;
    SOCKADDR_INFO SocketInfo;

    /* Enter prolog */
    ErrorCode = SockEnterApiFast(&ThreadData);
    if (ErrorCode != NO_ERROR)
    {
        /* Fail */
        *lpErrno = ErrorCode;
        return SOCKET_ERROR;
    }

    /* Get the socket structure */
    Socket = SockFindAndReferenceSocket(Handle, TRUE);
    if (!Socket)
    {
        /* Fail */
        ErrorCode = WSAENOTSOCK;
        goto error;
    }

    /* 
     * Check if this isn't a datagram socket or if it's a connected socket
     * without an address
     */
    if (!MSAFD_IS_DGRAM_SOCK(Socket) ||
        ((Socket->SharedData.State == SocketConnected) &&
         (!SocketAddress || !SocketAddressLength)))
    {
        /* Call WSPSend instead */
        SockDereferenceSocket(Socket);
        return WSPSend(Handle,
                       lpBuffers,
                       dwBufferCount,
                       lpNumberOfBytesSent,
                       iFlags,
                       lpOverlapped,
                       lpCompletionRoutine,
                       lpThreadId,
                       lpErrno);
    }

    /* If the socket isn't connected, we need an address*/
    if ((Socket->SharedData.State != SocketConnected) && (!SocketAddress))
    {
        /* Fail */
        ErrorCode = WSAENOTCONN;
        goto error;
    }

    /* Validate length */
    if (SocketAddressLength < Socket->HelperData->MaxWSAddressLength)
    {
        /* Fail */
        ErrorCode = WSAEFAULT;
        goto error;
    }

    /* Verify flags */
    if (iFlags & ~MSG_DONTROUTE)
    {
        /* Fail */
        ErrorCode = WSAEOPNOTSUPP;
        goto error;
    }

    /* Make sure send shutdown isn't active */
    if (Socket->SharedData.SendShutdown)
    {
        /* Fail */
        ErrorCode = WSAESHUTDOWN;
        goto error;
    }

    /* Make sure address families match */
    if (Socket->SharedData.AddressFamily != SocketAddress->sa_family)
    {
        /* Fail */
        ErrorCode = WSAEOPNOTSUPP;
        goto error;
    }

    /* Check if broadcast is enabled */
    if (!Socket->SharedData.Broadcast)
    {
        /* The caller might want to enable it; get the Sockaddr type */
        ErrorCode = Socket->HelperData->WSHGetSockaddrType((PSOCKADDR)SocketAddress,
                                                           SocketAddressLength,
                                                           &SocketInfo);
        if (ErrorCode != NO_ERROR) goto error;

        /* Check if this is a broadcast attempt */
        if (SocketInfo.AddressInfo == SockaddrAddressInfoBroadcast)
        {
            /* The socket won't allow it */
            ErrorCode = WSAEACCES;
            goto error;
        }
    }

    /* Check if this socket isn't bound yet */
    if (Socket->SharedData.State == SocketOpen)
    {
        /* Check if we can request the wildcard address */
        if (Socket->HelperData->WSHGetWildcardSockaddr)
        {
            /* Allocate a new Sockaddr */
            SockaddrLength = Socket->HelperData->MaxWSAddressLength;
            Sockaddr = SockAllocateHeapRoutine(SockPrivateHeap, 0, SockaddrLength);
            if (!Sockaddr)
            {
                /* Fail */
                ErrorCode = WSAENOBUFS;
                goto error;
            }

            /* Get the wildcard sockaddr */
            ErrorCode = Socket->HelperData->WSHGetWildcardSockaddr(Socket->HelperContext,
                                                                   Sockaddr,
                                                                   &SockaddrLength);
            if (ErrorCode != NO_ERROR)
            {
                /* Free memory and fail */
                RtlFreeHeap(SockPrivateHeap, 0, Sockaddr);
                goto error;
            }

            /* Lock the socket */
            EnterCriticalSection(&Socket->Lock);

            /* Make sure it's still unbound */
            if (Socket->SharedData.State == SocketOpen)
            {
                /* Bind it */
                ReturnValue = WSPBind(Handle,
                                      Sockaddr,
                                      SockaddrLength,
                                      &ErrorCode);
            }
            else
            {
                /* It's bound now, fake success */
                ReturnValue = NO_ERROR;
            }

            /* Release the lock and free memory */
            LeaveCriticalSection(&Socket->Lock);
            RtlFreeHeap(SockPrivateHeap, 0, Sockaddr);

            /* Check if we failed */
            if (ReturnValue == SOCKET_ERROR) goto error;
        }
        else
        {
            /* Unbound socket, but can't get the wildcard. Fail */
            ErrorCode = WSAEINVAL;
            goto error;
        }
    }

    /* Check how long the TDI Address is */
    TdiAddressSize = Socket->HelperData->MaxTDIAddressLength;

    /* See if it can fit in the stack */
    if (TdiAddressSize > sizeof(AddressBuffer))
    {
        /* Allocate from heap */
        TdiAddress = SockAllocateHeapRoutine(SockPrivateHeap, 0, TdiAddressSize);
        if (!TdiAddress)
        {
            /* Fail */
            ErrorCode = WSAENOBUFS;
            goto error;
        }
    }

    /* Build the TDI Address */
    ErrorCode = SockBuildTdiAddress(TdiAddress,
                                    (PSOCKADDR)SocketAddress,
                                    min(SocketAddressLength,
                                    Socket->HelperData->MaxWSAddressLength));
    if (ErrorCode != NO_ERROR) goto error;

    /* Set up the Send Structure */
    SendInfo.BufferArray = lpBuffers;
    SendInfo.BufferCount = dwBufferCount;
    SendInfo.AfdFlags = 0;
    SendInfo.TdiConnection.RemoteAddress = TdiAddress;
    SendInfo.TdiConnection.RemoteAddressLength = TdiAddressSize;

    /* Verifiy if we should use APC */
    if (!lpOverlapped)
    {
        /* Not using Overlapped structure, so use normal blocking on event */
        APCContext = NULL;
        ApcFunction = NULL;
        Event = ThreadData->EventHandle;
        IoStatusBlock = &DummyIoStatusBlock;
    }
    else
    {
        /* Using apc, check if we have a completion routine */
        if (!lpCompletionRoutine)
        {
            /* No need for APC */
            APCContext = lpOverlapped;
            ApcFunction = NULL;
            Event = lpOverlapped->hEvent;
        }
        else
        {
            /* Use APC */
            ApcFunction = SockIoCompletion;
            APCContext = lpCompletionRoutine;
            Event = NULL;

            /* Skip Fast I/O */
            SendInfo.AfdFlags = AFD_SKIP_FIO;
        }

        /* Use the overlapped's structure buffer for the I/O Status Block */
        IoStatusBlock = (PIO_STATUS_BLOCK)&lpOverlapped->Internal;

        /* Make this an overlapped I/O in AFD */
        SendInfo.AfdFlags |= AFD_OVERLAPPED;
    }

    /* Set is as Pending for now */
    IoStatusBlock->Status = STATUS_PENDING;

    /* Send IOCTL */
    Status = NtDeviceIoControlFile(Socket->WshContext.Handle,
                                   Event,
                                   ApcFunction,
                                   APCContext,
                                   IoStatusBlock,
                                   IOCTL_AFD_SEND_DATAGRAM,
                                   &SendInfo,
                                   sizeof(SendInfo),
                                   NULL,
                                   0);

    /* Increase the pending APC Count if we're using an APC */
    if (!NT_ERROR(Status) && ApcFunction)
    {
        ThreadData->PendingAPCs++;
        InterlockedIncrement(&SockProcessPendingAPCCount);
    }

    /* Wait for completition if not overlapped */
    if ((Status == STATUS_PENDING) && !(lpOverlapped))
    {
        /* Wait for completion */
        ReturnValue = SockWaitForSingleObject(Event,
                                              Handle,
                                              MAYBE_BLOCKING_HOOK,
                                              SEND_TIMEOUT);

        /* Check if the wait was successful */
        if (ReturnValue)
        {
            /* Get new status */
            Status = IoStatusBlock->Status;
        }
        else
        {
            /* Cancel the I/O */
            SockCancelIo(Handle);
            Status = STATUS_IO_TIMEOUT;
        }
    }

    /* Check status */
    switch (Status)
    {
        /* Success */
        case STATUS_SUCCESS:
            break;

        /* Pending I/O */
        case STATUS_PENDING:
            ErrorCode = WSA_IO_PENDING;
            goto error;

        /* Other NT Error */
        default:
            if (!NT_SUCCESS(Status))
            {
                /* Fail */
                ErrorCode = NtStatusToSocketError(Status);
                goto error;
            }
            break;
    }

    /* Return the number of bytes sent */
    *lpNumberOfBytesSent = PtrToUlong(IoStatusBlock->Information);

error:

    /* Check if we have a socket */
    if (Socket)
    {
        /* Check if async select was active and this blocked */
        if (SockAsyncSelectCalled && (ErrorCode == WSAEWOULDBLOCK))
        {
            /* Lock it */
            EnterCriticalSection(&Socket->Lock);

            /* Re-enable the regular write event */
            SockReenableAsyncSelectEvent(Socket, FD_WRITE);

            /* Unlock socket */
            LeaveCriticalSection(&Socket->Lock);
        }

        /* Dereference socket */
        SockDereferenceSocket(Socket);
    }
    
    /* Check if we should free the TDI Address */
    if (TdiAddress && (TdiAddress != (PVOID)AddressBuffer))
    {
        /* Free it from the heap */
        RtlFreeHeap(SockPrivateHeap, 0, TdiAddress);
    }

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