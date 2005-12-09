/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Winsock 2 SPI
 * FILE:        lib/mswsock/lib/init.c
 * PURPOSE:     DLL Initialization
 */

/* INCLUDES ******************************************************************/
#include "msafd.h"

/* DATA **********************************************************************/

DWORD SockSendBufferWindow;
DWORD SockReceiveBufferWindow;

/* FUNCTIONS *****************************************************************/

INT
WSPAPI
SockUpdateWindowSizes(IN PSOCKET_INFORMATION Socket,
                      IN BOOLEAN Force)
{
    INT ErrorCode;

    /* Check if this is a connection-less socket */
    if (Socket->SharedData.ServiceFlags1 & XP1_CONNECTIONLESS)
    {
        /* It must be bound */
        if (Socket->SharedData.State == SocketOpen) return NO_ERROR;
    }
    else
    {
        /* It must be connected */
        if (Socket->SharedData.State == SocketConnected) return NO_ERROR;

        /* Get the TDI handles for it */
        ErrorCode = SockGetTdiHandles(Socket);
        if (ErrorCode != NO_ERROR) return ErrorCode;

        /* Tell WSH the new size */
        ErrorCode = Socket->HelperData->WSHSetSocketInformation(Socket->HelperContext,
                                                                Socket->Handle,
                                                                Socket->TdiAddressHandle,
                                                                Socket->TdiConnectionHandle,
                                                                SOL_SOCKET,
                                                                SO_RCVBUF,
                                                                (PVOID)&Socket->SharedData.SizeOfRecvBuffer,
                                                                sizeof(DWORD));
    }
    
    /* Check if the buffer changed, or if this is a force */
    if ((Socket->SharedData.SizeOfRecvBuffer != SockReceiveBufferWindow) ||
        (Force))
    {
        /* Set the information in AFD */
        ErrorCode = SockSetInformation(Socket,
                                       AFD_INFO_RECEIVE_WINDOW_SIZE,
                                       NULL,
                                       &Socket->SharedData.SizeOfRecvBuffer,
                                       NULL);
        if (ErrorCode != NO_ERROR) return ErrorCode;
    }

    /* Do the same thing for the send buffer */
    if ((Socket->SharedData.SizeOfSendBuffer != SockSendBufferWindow) ||
        (Force))
    {
        /* Set the information in AFD */
        ErrorCode = SockSetInformation(Socket,
                                       AFD_INFO_SEND_WINDOW_SIZE,
                                       NULL,
                                       &Socket->SharedData.SizeOfSendBuffer,
                                       NULL);
        if (ErrorCode != NO_ERROR) return ErrorCode;
    }

    /* Return to caller */
    return NO_ERROR;
}

BOOLEAN
WSPAPI
IsValidOptionForSocket(IN PSOCKET_INFORMATION Socket,
                       IN INT Level,
                       IN INT OptionName)
{
    /* SOL_INTERNAL is always illegal when external, of course */
    if (Level == SOL_INTERNAL) return FALSE;

    /* Anything else but SOL_SOCKET we can't handle, so assume it's legal */
    if (Level != SOL_SOCKET) return TRUE;

    /* Check the option name */
    switch (OptionName)
    {
        case SO_DONTLINGER:
        case SO_KEEPALIVE:
        case SO_LINGER:
        case SO_OOBINLINE:
        case SO_ACCEPTCONN:
            /* Only valid on stream sockets */
            if (MSAFD_IS_DGRAM_SOCK(Socket)) return FALSE;

            /* It is one, suceed */
            return TRUE;

        case SO_BROADCAST:
            /* Only valid on datagram sockets */
            if (MSAFD_IS_DGRAM_SOCK(Socket)) return TRUE;

            /* It isn't one, fail */
            return FALSE;

        case SO_PROTOCOL_INFOA:
            /* Winsock 2 has a hack for this, we should get the W version */
            return FALSE;

        default:
            /* Anything else is always valid */
            return TRUE;
    }
}

INT
WSPAPI
SockGetConnectData(IN PSOCKET_INFORMATION Socket,
                   IN ULONG Ioctl,
                   IN PVOID Buffer,
                   IN ULONG BufferLength,
                   OUT PULONG BufferReturned)
{
    PWINSOCK_TEB_DATA ThreadData = NtCurrentTeb()->WinSockData;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    AFD_PENDING_ACCEPT_DATA ConnectData;

    /* Make sure we have Accept Info in the TEB for this Socket */
    if ((ThreadData->AcceptData) &&
        (ThreadData->AcceptData->ListenHandle == Socket->WshContext.Handle))
    {
        /* Set the connect data structure */
        ConnectData.SequenceNumber = ThreadData->AcceptData->SequenceNumber;
        ConnectData.ReturnSize = FALSE;

        /* Send it to AFD */
        Status = NtDeviceIoControlFile(Socket->WshContext.Handle,
                                       ThreadData->EventHandle,
                                       NULL,
                                       0,
                                       &IoStatusBlock,
                                       Ioctl,
                                       &ConnectData,
                                       sizeof(ConnectData),
                                       Buffer,
                                       BufferLength);
    }
    else
    {
        /* Request it from AFD */
        Status = NtDeviceIoControlFile(Socket->WshContext.Handle,
                                       ThreadData->EventHandle,
                                       NULL,
                                       0,
                                       &IoStatusBlock,
                                       Ioctl,
                                       NULL,
                                       0,
                                       Buffer,
                                       BufferLength);
    }

    /* Check if we need to wait */
    if (Status == STATUS_PENDING)
    {
        /* Wait for completion */
        SockWaitForSingleObject(ThreadData->EventHandle,
                                Socket->Handle,
                                NO_BLOCKING_HOOK,
                                NO_TIMEOUT);

        /* Get new status */
        Status = IoStatusBlock.Status;
    }

    /* Check for error */
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        return NtStatusToSocketError(Status);
    }

    /* Return the length */
    if (BufferReturned) *BufferReturned = PtrToUlong(IoStatusBlock.Information);

    /* Return success */
    return NO_ERROR;
}

INT
WSPAPI
WSPIoctl(IN SOCKET Handle,
         IN DWORD dwIoControlCode,
         IN LPVOID lpvInBuffer,
         IN DWORD cbInBuffer,
         OUT LPVOID lpvOutBuffer,
         IN DWORD cbOutBuffer,
         OUT LPDWORD lpcbBytesReturned,
         IN LPWSAOVERLAPPED lpOverlapped,
         IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
         IN LPWSATHREADID lpThreadId,
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

    /* Get the socket structure */
    Socket = SockFindAndReferenceSocket(Handle, TRUE);
    if (!Socket)
    {
        /* Fail */
        ErrorCode = WSAENOTSOCK;
        goto error;
    }

    /* Lock the socket */
    EnterCriticalSection(&Socket->Lock);

    switch(dwIoControlCode) {
        
        case FIONBIO:

            /* Check if the Buffer is OK */
            if(cbInBuffer < sizeof(ULONG))
            {
                /* Fail */
                ErrorCode = WSAEFAULT;
                goto error;
            }

            return 0;

        default:

            /* Unsupported for now */
            *lpErrno = WSAEINVAL;
            return SOCKET_ERROR;
    }

error:
    /* Check if we had a socket */
    if (Socket)
    {
        /* Release lock and dereference it */
        LeaveCriticalSection(&Socket->Lock);
        SockDereferenceSocket(Socket);
    }

    /* Check for error */
    if (ErrorCode != NO_ERROR)
    {
        /* Return error */
        *lpErrno = ErrorCode;
        return SOCKET_ERROR;
    }

    /* Return to caller */
    return NO_ERROR;
}


INT
WSPAPI
WSPGetSockOpt(IN SOCKET Handle,
              IN INT Level,
              IN INT OptionName,
              OUT CHAR FAR* OptionValue,
              IN OUT LPINT OptionLength,
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

    /* Get the socket structure */
    Socket = SockFindAndReferenceSocket(Handle, TRUE);
    if (!Socket)
    {
        /* Fail */
        *lpErrno = WSAENOTSOCK;
        return SOCKET_ERROR;
    }

    /* Lock the socket */
    EnterCriticalSection(&Socket->Lock);

    /* Make sure we're not closed */
    if (Socket->SharedData.State == SocketClosed)
    {
        /* Fail */
        ErrorCode = WSAENOTSOCK;
        goto error;
    }

    /* Validate the pointer and length */
    if (!(OptionValue) || 
        !(OptionLength) || 
        (*OptionLength < sizeof(CHAR)) ||
        (*OptionLength & 0x80000000))
    {
        /* Fail */
        ErrorCode = WSAEFAULT;
        goto error;
    }

    /* Validate option */
    if (!IsValidOptionForSocket(Socket, Level, OptionName))
    {
        /* Fail */
        ErrorCode = WSAENOPROTOOPT;
        goto error;
    }

    /* If it's one of the recognized options */
    if (Level == SOL_SOCKET &&
        (OptionName == SO_BROADCAST ||
         OptionName == SO_DEBUG ||
         OptionName == SO_DONTLINGER ||
         OptionName == SO_LINGER ||
         OptionName == SO_OOBINLINE ||
         OptionName == SO_RCVBUF ||
         OptionName == SO_REUSEADDR ||
         OptionName == SO_EXCLUSIVEADDRUSE ||
         OptionName == SO_CONDITIONAL_ACCEPT ||
         OptionName == SO_SNDBUF ||
         OptionName == SO_TYPE ||
         OptionName == SO_ACCEPTCONN ||
         OptionName == SO_ERROR))
    {
        /* Clear the buffer first */
        RtlZeroMemory(OptionValue, *OptionLength);
    }


    /* Check the Level first */
    switch (Level)
    {
        /* Handle SOL_SOCKET */
        case SOL_SOCKET:

            /* Now check the Option */
            switch (OptionName)
            {
                case SO_TYPE:

                    /* Validate the size */
                    if (*OptionLength < sizeof(INT))
                    {
                        /* Size is too small, fail */
                        ErrorCode = WSAEFAULT;
                        goto error;
                    }

                    /* Return the data */
                    *OptionValue = Socket->SharedData.SocketType;
                    *OptionLength = sizeof(INT);
                    break;

                case SO_RCVBUF:

                    /* Validate the size */
                    if (*OptionLength < sizeof(INT))
                    {
                        /* Size is too small, fail */
                        ErrorCode = WSAEFAULT;
                        goto error;
                    }

                    /* Return the data */
                    *(PINT)OptionValue = Socket->SharedData.SizeOfRecvBuffer;
                    *OptionLength = sizeof(INT);
                    break;

                case SO_SNDBUF:

                    /* Validate the size */
                    if (*OptionLength < sizeof(INT))
                    {
                        /* Size is too small, fail */
                        ErrorCode = WSAEFAULT;
                        goto error;
                    }

                    /* Return the data */
                    *(PINT)*OptionValue = Socket->SharedData.SizeOfSendBuffer;
                    *OptionLength = sizeof(INT);
                    break;

                case SO_ACCEPTCONN:

                    /* Return the data */
                    *OptionValue = Socket->SharedData.Listening;
                    *OptionLength = sizeof(BOOLEAN);
                    break;

                case SO_BROADCAST:

                    /* Return the data */
                    *OptionValue = Socket->SharedData.Broadcast;
                    *OptionLength = sizeof(BOOLEAN);
                    break;

                case SO_DEBUG:

                    /* Return the data */
                    *OptionValue = Socket->SharedData.Debug;
                    *OptionLength = sizeof(BOOLEAN);
                    break;

                case SO_CONDITIONAL_ACCEPT:
                case SO_DONTLINGER:
                case SO_DONTROUTE:
                case SO_ERROR:
                case SO_GROUP_ID:
                case SO_GROUP_PRIORITY:
                case SO_KEEPALIVE:
                case SO_LINGER:
                case SO_MAX_MSG_SIZE:
                case SO_OOBINLINE:
                case SO_PROTOCOL_INFO:
                case SO_REUSEADDR:

                    /* Unsupported */

                default:
                    
                    /* Unsupported by us, give it to the helper */
                    ErrorCode = SockGetTdiHandles(Socket);
                    if (ErrorCode != NO_ERROR) goto error;

                    /* Call the helper */
                    ErrorCode = Socket->HelperData->WSHGetSocketInformation(Socket->HelperContext,
                                                                            Handle,
                                                                            Socket->TdiAddressHandle,
                                                                            Socket->TdiConnectionHandle,
                                                                            Level,
                                                                            OptionName,
                                                                            OptionValue,
                                                                            OptionLength);
                    if (ErrorCode != NO_ERROR) goto error;
                    break;
            }

        default:

            /* Unsupported by us, give it to the helper */
            ErrorCode = SockGetTdiHandles(Socket);
            if (ErrorCode != NO_ERROR) goto error;

            /* Call the helper */
            ErrorCode = Socket->HelperData->WSHGetSocketInformation(Socket->HelperContext,
                                                                    Handle,
                                                                    Socket->TdiAddressHandle,
                                                                    Socket->TdiConnectionHandle,
                                                                    Level,
                                                                    OptionName,
                                                                    OptionValue,
                                                                    OptionLength);
            if (ErrorCode != NO_ERROR) goto error;
            break;
    }

error:
    /* Release the lock and dereference the socket */
    LeaveCriticalSection(&Socket->Lock);
    SockDereferenceSocket(Socket);

    /* Handle error case */
    if (ErrorCode != NO_ERROR)
    {
        /* Return error */
        *lpErrno = ErrorCode;
        return SOCKET_ERROR;
    }

    /* Return success */
    return NO_ERROR;
}

INT
WSPAPI
WSPSetSockOpt(IN SOCKET Handle,
              IN INT Level,    
              IN INT OptionName,
              IN CONST CHAR FAR *OptionValue,
              IN INT OptionLength,
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

    /* Get the socket structure */
    Socket = SockFindAndReferenceSocket(Handle, TRUE);
    if (!Socket)
    {
        /* Fail */
        *lpErrno = WSAENOTSOCK;
        return SOCKET_ERROR;
    }

    /* Lock the socket */
    EnterCriticalSection(&Socket->Lock);

    /* Make sure we're not closed */
    if (Socket->SharedData.State == SocketClosed)
    {
        /* Fail */
        ErrorCode = WSAENOTSOCK;
        goto error;
    }

    /* Validate the pointer */
    if (!OptionValue)
    {
        /* Fail */
        ErrorCode = WSAEFAULT;
        goto error;
    }

    /* Validate option */
    if (!IsValidOptionForSocket(Socket, Level, OptionName))
    {
        /* Fail */
        ErrorCode = WSAENOPROTOOPT;
        goto error;
    }

    /* FIXME: Write code */

error:

    /* Check if this is the failure path */
    if (ErrorCode != NO_ERROR)
    {
        /* Dereference and unlock the socket */
        LeaveCriticalSection(&Socket->Lock);
        SockDereferenceSocket(Socket);

        /* Return error */
        *lpErrno = ErrorCode;
        return SOCKET_ERROR;
    }

    /* Update the socket's state in AFD */
    ErrorCode = SockSetHandleContext(Socket);
    if (ErrorCode != NO_ERROR) goto error;

    /* Return success */
    return NO_ERROR;
}