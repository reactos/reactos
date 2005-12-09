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
WSPListen(SOCKET Handle, 
          INT Backlog, 
          LPINT lpErrno)
{
    IO_STATUS_BLOCK IoStatusBlock;
    AFD_LISTEN_DATA ListenData;
    PSOCKET_INFORMATION Socket;
    PWINSOCK_TEB_DATA ThreadData;
    INT ErrorCode;
    NTSTATUS Status;

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

    /* If the socket is connection-less, fail */
    if (MSAFD_IS_DGRAM_SOCK(Socket));
    {
        /* Fail */
        ErrorCode = WSAEOPNOTSUPP;
        goto error;
    }

    /* If the socket is already listening, do nothing */
    if (Socket->SharedData.Listening)
    {
        /* Return happily */
        ErrorCode = NO_ERROR;
        goto error;
    }
    else if (Socket->SharedData.State != SocketConnected)
    {
        /* If we're not connected, fail */
        ErrorCode = WSAEINVAL;
        goto error;
    }

    /* Set Up Listen Structure */
    ListenData.UseSAN = SockSanEnabled;
    ListenData.UseDelayedAcceptance = Socket->SharedData.UseDelayedAcceptance;
    ListenData.Backlog = Backlog;

    /* Send IOCTL */
    Status = NtDeviceIoControlFile(Socket->WshContext.Handle,
                                   ThreadData->EventHandle,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_AFD_START_LISTEN,
                                   &ListenData,
                                   sizeof(ListenData),
                                   NULL,
                                   0);
    /* Check if we need to wait */
    if (Status == STATUS_PENDING)
    {
        /* Wait for completion outside the lock */
        LeaveCriticalSection(&Socket->Lock);
        SockWaitForSingleObject(ThreadData->EventHandle,
                                Handle,
                                NO_BLOCKING_HOOK,
                                NO_TIMEOUT);
        EnterCriticalSection(&Socket->Lock);

        /* Get new status */
        Status = IoStatusBlock.Status;
    }

    /* Check for error */
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        ErrorCode = NtStatusToSocketError(Status);
        goto error;
    }

    /* Notify helper DLL */
    ErrorCode = SockNotifyHelperDll(Socket, WSH_NOTIFY_LISTEN);
    if (ErrorCode != NO_ERROR) goto error;

    /* Set to Listening */
    Socket->SharedData.Listening = TRUE;

    /* Update context with AFD */
    ErrorCode = SockSetHandleContext(Socket);
    if (ErrorCode != NO_ERROR) goto error;

error:
    /* Check if we have a socket here */
    if (Socket)
    {
        /* Release the lock and dereference */
        LeaveCriticalSection(&Socket->Lock);
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
