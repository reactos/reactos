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
WSPRecvDisconnect(IN SOCKET s,
                  OUT LPWSABUF lpInboundDisconnectData,
                  OUT LPINT lpErrno)
{
    return 0;
}

INT
WSPAPI
WSPSendDisconnect(IN SOCKET s,
                  IN LPWSABUF lpOutboundDisconnectData,
                  OUT LPINT lpErrno)
{
    return 0;
}

INT
WSPAPI 
WSPShutdown(SOCKET Handle, 
            INT HowTo, 
            LPINT lpErrno)

{
    IO_STATUS_BLOCK IoStatusBlock;
    AFD_DISCONNECT_INFO DisconnectInfo;
    PSOCKET_INFORMATION Socket;
    PWINSOCK_TEB_DATA ThreadData;
    DWORD HelperEvent;
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

    /* If the socket is not connection-less, fail if it's not connected */
    if ((MSAFD_IS_DGRAM_SOCK(Socket)) && !(SockIsSocketConnected(Socket)))
    {
        /* Fail */
        ErrorCode = WSAENOTCONN;
        goto error;
    }

    /* Set AFD Disconnect Type and WSH Notification Type */
    switch (HowTo)
    {
        case SD_RECEIVE:
            /* Set receive disconnect */
            DisconnectInfo.DisconnectType = AFD_DISCONNECT_RECV;
            HelperEvent = WSH_NOTIFY_SHUTDOWN_RECEIVE;

            /* Save it for ourselves */
            Socket->SharedData.ReceiveShutdown = TRUE;
            break;
        
        case SD_SEND:
            /* Set receive disconnect */
            DisconnectInfo.DisconnectType = AFD_DISCONNECT_SEND;
            HelperEvent = WSH_NOTIFY_SHUTDOWN_SEND;

            /* Save it for ourselves */
            Socket->SharedData.SendShutdown = TRUE;
            break;

        case SD_BOTH:
            /* Set both */
            DisconnectInfo.DisconnectType = AFD_DISCONNECT_RECV | AFD_DISCONNECT_SEND;
            HelperEvent = WSH_NOTIFY_SHUTDOWN_ALL;

            /* Save it for ourselves */
            Socket->SharedData.ReceiveShutdown = Socket->SharedData.SendShutdown = TRUE;
            break;

        default:
            /* Fail, invalid type */
            ErrorCode = WSAEINVAL;
            goto error;
    }

    /* Inifite Timeout */
    DisconnectInfo.Timeout = RtlConvertLongToLargeInteger(-1);

    /* Send IOCTL */
    Status = NtDeviceIoControlFile(Socket->WshContext.Handle,
                                   ThreadData->EventHandle,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_AFD_DISCONNECT,
                                   &DisconnectInfo,
                                   sizeof(DisconnectInfo),
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
    ErrorCode = SockNotifyHelperDll(Socket, HelperEvent);
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