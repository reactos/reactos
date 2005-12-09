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
WSPRecv(SOCKET Handle, 
        LPWSABUF lpBuffers, 
        DWORD dwBufferCount, 
        LPDWORD lpNumberOfBytesRead, 
        LPDWORD ReceiveFlags, 
        LPWSAOVERLAPPED lpOverlapped, 
        LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine, 
        LPWSATHREADID lpThreadId, 
        LPINT lpErrno)
{
    PIO_STATUS_BLOCK IoStatusBlock;
    IO_STATUS_BLOCK DummyIoStatusBlock;
    AFD_RECV_INFO RecvInfo;
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
    RecvInfo.BufferArray = lpBuffers;
    RecvInfo.BufferCount = dwBufferCount;
    RecvInfo.TdiFlags = 0;
    RecvInfo.AfdFlags = 0;

    /* Set the TDI Flags */
    if (!(*ReceiveFlags))
    {
        /* Use normal TDI Receive */
        RecvInfo.TdiFlags |= TDI_RECEIVE_NORMAL;
    }
    else
    {
        /* Check for valid flags */
        if ((*ReceiveFlags & ~(MSG_OOB | MSG_PEEK | MSG_PARTIAL)))
        {
            /* Fail */
            ErrorCode = WSAEOPNOTSUPP;
            goto error;
        }
    
        /* Check if OOB is being used */
        if (*ReceiveFlags & MSG_OOB)
        {
            /* Use Expedited Receive for OOB */
            RecvInfo.TdiFlags |= TDI_RECEIVE_EXPEDITED;
        }
        else
        {
            /* Use normal receive */
            RecvInfo.TdiFlags |= TDI_RECEIVE_NORMAL;
        }

        /* Use Peek Receive if enabled */
        if (*ReceiveFlags & MSG_PEEK) RecvInfo.TdiFlags |= TDI_RECEIVE_PEEK;

        /* Use Partial Receive if enabled */
        if (*ReceiveFlags & MSG_PARTIAL) RecvInfo.TdiFlags |= TDI_RECEIVE_PARTIAL;
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
            RecvInfo.AfdFlags = AFD_SKIP_FIO;
        }

        /* Use the overlapped's structure buffer for the I/O Status Block */
        IoStatusBlock = (PIO_STATUS_BLOCK)&lpOverlapped->Internal;

        /* Make this an overlapped I/O in AFD */
        RecvInfo.AfdFlags |= AFD_OVERLAPPED;
    }

    /* Set is as Pending for now */
    IoStatusBlock->Status = STATUS_PENDING;

    /* Send IOCTL */
    Status = NtDeviceIoControlFile((HANDLE)Handle,
                                   Event,
                                   ApcFunction,
                                   APCContext,
                                   IoStatusBlock,
                                   IOCTL_AFD_RECV,
                                   &RecvInfo,
                                   sizeof(RecvInfo),
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
                                              RECV_TIMEOUT);

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

            /* Get new status and normalize */
            Status = IoStatusBlock->Status;
            if (Status == STATUS_CANCELLED) Status = STATUS_IO_TIMEOUT;
        }
    }

    /* Return the Flags */
    *ReceiveFlags = 0;
    switch (Status)
    {
        /* Success */
        case STATUS_SUCCESS:
            break;

        /* Pending I/O */
        case STATUS_PENDING:
            ErrorCode = WSA_IO_PENDING;
            goto error;

        /* Buffer Overflow */
        case STATUS_BUFFER_OVERFLOW:
            /* Check if this was overlapped */
            if (lpOverlapped)
            {
                /* Return without bytes read */
                ErrorCode = WSA_IO_PENDING;
                goto error;
            }

            /* Return failure with bytes read */
            ErrorCode = WSAEMSGSIZE;
            break;

        /* OOB Receive */
        case STATUS_RECEIVE_EXPEDITED: 
            *ReceiveFlags = MSG_OOB; 
            break;

        /* Partial OOB Receive */
        case STATUS_RECEIVE_PARTIAL_EXPEDITED:
            *ReceiveFlags = MSG_PARTIAL | MSG_OOB; 
            break;
        
        /* Parial Receive */
        case STATUS_RECEIVE_PARTIAL: 
            *ReceiveFlags = MSG_PARTIAL; 
            break;

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

    /* Return the number of bytes read */
    *lpNumberOfBytesRead = PtrToUlong(IoStatusBlock->Information);

error:

    /* Check if async select was active */
    if (SockAsyncSelectCalled)
    {
        /* Get the socket */
        Socket = SockFindAndReferenceSocket(Handle, TRUE);
        if (Socket)
        {
            /* Lock it */
            EnterCriticalSection(&Socket->Lock);

            /* Check which event to re-enable */
            if (RecvInfo.TdiFlags & TDI_RECEIVE_EXPEDITED)
            {
                /* Re-enable the OOB event */
                SockReenableAsyncSelectEvent(Socket, FD_OOB);
            }
            else
            {
                /* Re-enable the regular read event */
                SockReenableAsyncSelectEvent(Socket, FD_READ);
            }

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
WSPRecvFrom(SOCKET Handle, 
            LPWSABUF lpBuffers, 
            DWORD dwBufferCount, 
            LPDWORD lpNumberOfBytesRead, 
            LPDWORD ReceiveFlags, 
            PSOCKADDR SocketAddress, 
            PINT SocketAddressLength, 
            LPWSAOVERLAPPED lpOverlapped, 
            LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine, 
            LPWSATHREADID lpThreadId, 
            LPINT lpErrno)
{
    PIO_STATUS_BLOCK IoStatusBlock;
    IO_STATUS_BLOCK DummyIoStatusBlock;
    AFD_RECV_INFO_UDP RecvInfo;
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

    /* Get the socket */
    Socket = SockFindAndReferenceSocket(Handle, TRUE);
    if (!Socket)
    {
        /* Fail */
        ErrorCode = WSAENOTSOCK;
        goto error;
    }

    /* Fail if the socket isn't bound */
    if (Socket->SharedData.State == SocketOpen)
    {
        /* Fail */
        ErrorCode = WSAEINVAL;
        goto error;
    }

    /* If this is an unconnected or non datagram socket */
    if (!(MSAFD_IS_DGRAM_SOCK(Socket)) ||
        (!SocketAddress && !SocketAddressLength))
    {
        /* Call WSP Recv */
        SockDereferenceSocket(Socket);
        return WSPRecv(Handle,
                       lpBuffers,
                       dwBufferCount,
                       lpNumberOfBytesRead,
                       ReceiveFlags,
                       lpOverlapped,
                       lpCompletionRoutine,
                       lpThreadId,
                       lpErrno);
    }

    /* If receive shutdown is enabled, fail */
    if (Socket->SharedData.ReceiveShutdown)
    {
        /* Fail */
        ErrorCode = WSAESHUTDOWN;
        goto error;
    }

    /* Check for valid Socket Address (Length) flags */
    if (!(SocketAddress) ^ (!SocketAddressLength || !(*SocketAddressLength)))
    {
        /* Fail */
        ErrorCode = WSAEFAULT;
        goto error;
    }

    /* Check for valid flags */
    if ((*ReceiveFlags & ~(MSG_PEEK | MSG_PARTIAL)))
    {
        /* Fail */
        ErrorCode = WSAEOPNOTSUPP;
        goto error;
    }

    /* Check that the length is respected */
    if (SocketAddressLength &&
        (*SocketAddressLength < Socket->HelperData->MinWSAddressLength))
    {
        /* Fail */
        ErrorCode = WSAEFAULT;
        goto error;
    }

    /* Set up the Receive Structure */
    RecvInfo.BufferArray = lpBuffers;
    RecvInfo.BufferCount = dwBufferCount;
    RecvInfo.TdiFlags = TDI_RECEIVE_NORMAL;
    RecvInfo.AfdFlags = 0;
    RecvInfo.Address = SocketAddress;
    RecvInfo.AddressLength = SocketAddressLength;

    /* Use Peek Receive if enabled */
    if (*ReceiveFlags & MSG_PEEK) RecvInfo.TdiFlags |= TDI_RECEIVE_PEEK;

    /* Use Partial Receive if enabled */
    if (*ReceiveFlags & MSG_PARTIAL) RecvInfo.TdiFlags |= TDI_RECEIVE_PARTIAL;

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
            RecvInfo.AfdFlags = AFD_SKIP_FIO;
        }

        /* Use the overlapped's structure buffer for the I/O Status Block */
        IoStatusBlock = (PIO_STATUS_BLOCK)&lpOverlapped->Internal;

        /* Make this an overlapped I/O in AFD */
        RecvInfo.AfdFlags |= AFD_OVERLAPPED;
    }

    /* Set is as Pending for now */
    IoStatusBlock->Status = STATUS_PENDING;

    /* Send IOCTL */
    Status = NtDeviceIoControlFile(Socket->WshContext.Handle,
                                   Event,
                                   ApcFunction,
                                   APCContext,
                                   IoStatusBlock,
                                   IOCTL_AFD_RECV_DATAGRAM,
                                   &RecvInfo,
                                   sizeof(RecvInfo),
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
                                              RECV_TIMEOUT);

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

            /* Get new status and normalize */
            Status = IoStatusBlock->Status;
            if (Status == STATUS_CANCELLED) Status = STATUS_IO_TIMEOUT;
        }
    }

    /* Return the Flags */
    *ReceiveFlags = 0;
    switch (Status)
    {
        /* Success */
        case STATUS_SUCCESS:
            break;

        /* Pending I/O */
        case STATUS_PENDING:
            ErrorCode = WSA_IO_PENDING;
            goto error;

        /* Buffer Overflow */
        case STATUS_BUFFER_OVERFLOW:
            /* Check if this was overlapped */
            if (lpOverlapped)
            {
                /* Return without bytes read */
                ErrorCode = WSA_IO_PENDING;
                goto error;
            }

            /* Return failure with bytes read */
            ErrorCode = WSAEMSGSIZE;
            break;

        /* OOB Receive */
        case STATUS_RECEIVE_EXPEDITED: 
            *ReceiveFlags = MSG_OOB; 
            break;

        /* Partial OOB Receive */
        case STATUS_RECEIVE_PARTIAL_EXPEDITED:
            *ReceiveFlags = MSG_PARTIAL | MSG_OOB; 
            break;
        
        /* Parial Receive */
        case STATUS_RECEIVE_PARTIAL: 
            *ReceiveFlags = MSG_PARTIAL; 
            break;

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

    /* Return the number of bytes read */
    *lpNumberOfBytesRead = PtrToUlong(IoStatusBlock->Information);

error:

    /* Check if we have a socket here */
    if (Socket)
    {
        /* Check if async select was active */
        if (SockAsyncSelectCalled)
        {
            /* Lock the socket */
            EnterCriticalSection(&Socket->Lock);

            /* Re-enable the regular read event */
            SockReenableAsyncSelectEvent(Socket, FD_READ);

            /* Unlock socket */
            LeaveCriticalSection(&Socket->Lock);
        }
    
        /* Dereference it */
        SockDereferenceSocket(Socket);
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