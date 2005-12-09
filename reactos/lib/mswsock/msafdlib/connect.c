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

BOOLEAN
WSPAPI
IsSockaddrEqualToZero(IN const struct sockaddr* SocketAddress,
                      IN INT SocketAddressLength)
{
    INT i;

    for (i = 0; i < SocketAddressLength; i++)
    {
        /* Make sure it's 0 */
        if (*(PULONG)SocketAddress + i)return FALSE;
    }

    /* All zeroes, succees! */
    return TRUE;
}

INT
WSPAPI
UnconnectDatagramSocket(IN PSOCKET_INFORMATION Socket)
{
    NTSTATUS Status;
    INT ErrorCode;
    PWINSOCK_TEB_DATA ThreadData = NtCurrentTeb()->WinSockData;
    AFD_DISCONNECT_INFO DisconnectInfo;
    IO_STATUS_BLOCK IoStatusBlock;

    /* Set up the disconnect information */
    DisconnectInfo.Timeout = RtlConvertLongToLargeInteger(-1);
    DisconnectInfo.DisconnectType = AFD_DISCONNECT_DATAGRAM;

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
                                Socket->Handle,
                                NO_BLOCKING_HOOK,
                                NO_TIMEOUT);
        EnterCriticalSection(&Socket->Lock);

        /* Get new status */
        Status = IoStatusBlock.Status;
    }

    /* Check for error */
    if (!NT_SUCCESS(Status))
    {
        /* Convert error code */
        ErrorCode = NtStatusToSocketError(Status);
    }
    else
    {
        /* Set us as disconnected (back to bound) */
        Socket->SharedData.State = SocketBound;
        ErrorCode = NO_ERROR;
    }

    /* Return to caller */
    return ErrorCode;
}

INT
WSPAPI
SockPostProcessConnect(IN PSOCKET_INFORMATION Socket)
{
    INT ErrorCode;

    /* Notify the helper DLL */
    ErrorCode = SockNotifyHelperDll(Socket, WSH_NOTIFY_CONNECT);
    if (ErrorCode != NO_ERROR) return ErrorCode;

    /* Set the new state and update the context in AFD */
    Socket->SharedData.State = SocketConnected;
    ErrorCode = SockSetHandleContext(Socket);
    if (ErrorCode != NO_ERROR) return ErrorCode;

    /* Update the window sizes */
    ErrorCode = SockUpdateWindowSizes(Socket, FALSE);

    /* Return to caller */
    return ErrorCode;
}

INT
WSPAPI
SockDoConnectReal(IN PSOCKET_INFORMATION Socket, 
                  IN const struct sockaddr *SocketAddress, 
                  IN INT SocketAddressLength, 
                  IN LPWSABUF lpCalleeData,
                  IN BOOLEAN UseSan)
{
    INT ErrorCode;
    NTSTATUS Status;
    DWORD ConnectDataLength;
    IO_STATUS_BLOCK IoStatusBlock;
    PWINSOCK_TEB_DATA ThreadData = NtCurrentTeb()->WinSockData;
    CHAR ConnectBuffer[FIELD_OFFSET(AFD_CONNECT_INFO, RemoteAddress) +
                       MAX_TDI_ADDRESS_LENGTH];
    PAFD_CONNECT_INFO ConnectInfo;
    ULONG ConnectInfoLength;

    /* Check if someone is waiting for FD_CONNECT */
    if (Socket->SharedData.AsyncEvents & FD_CONNECT) 
    {
        /* 
         * Disable FD_WRITE and FD_CONNECT 
         * The latter fixes a race condition where the FD_CONNECT is re-enabled
         * at the end of this function right after the Async Thread disables it.
         * This should only happen at the *next* WSPConnect
         */
        Socket->SharedData.AsyncDisabledEvents |= FD_CONNECT | FD_WRITE;
    }

    /* Calculate how much the connection structure will take */
    ConnectInfoLength = FIELD_OFFSET(AFD_CONNECT_INFO, RemoteAddress) +
                        Socket->HelperData->MaxTDIAddressLength;

    /* Check if our stack buffer is enough */
    if (ConnectInfoLength <= sizeof(ConnectBuffer))
    {
        /* Use the stack */
        ConnectInfo = (PVOID)ConnectBuffer;
    }
    else
    {
        /* Allocate from heap */
        ConnectInfo = SockAllocateHeapRoutine(SockPrivateHeap,
                                              0,
                                              ConnectInfoLength);
        if (!ConnectInfo)
        {
            /* Fail */
            ErrorCode = WSAENOBUFS;
            goto error;
        }
    }

    /* Create the TDI Address */
    ErrorCode = SockBuildTdiAddress(&ConnectInfo->RemoteAddress,
                                    (PSOCKADDR)SocketAddress,
                                    SocketAddressLength);
    if (ErrorCode != NO_ERROR) goto error;

    /* Set the SAN State */
    ConnectInfo->UseSAN = SockSanEnabled;

    /* Check if this is a non-blocking streaming socket */
    if ((Socket->SharedData.NonBlocking) && !(MSAFD_IS_DGRAM_SOCK(Socket)))
    {
        /* Create the Async Thread if Needed */  
        if (!SockCheckAndReferenceAsyncThread())
        {
            /* Fail */
            ErrorCode = WSAENOBUFS;
            goto error;
        }

        Status = 0;
    }
    else
    {
        /* Start the connect loop */
        do
        {
            /* Send IOCTL */
            IoStatusBlock.Status = STATUS_PENDING;
            Status = NtDeviceIoControlFile(Socket->WshContext.Handle,
                                           ThreadData->EventHandle,
                                           NULL,
                                           NULL,
                                           &IoStatusBlock,
                                           IOCTL_AFD_CONNECT,
                                           ConnectInfo,
                                           ConnectInfoLength,
                                           NULL,
                                           0);

            /* Check if we need to wait */
            if (Status == STATUS_PENDING)
            {
                /* Wait for completion outside the lock */
                LeaveCriticalSection(&Socket->Lock);
                SockWaitForSingleObject(ThreadData->EventHandle,
                                        Socket->Handle,
                                        NO_BLOCKING_HOOK,
                                        NO_TIMEOUT);
                EnterCriticalSection(&Socket->Lock);

                /* Get new status */
                Status = IoStatusBlock.Status;
            }

            /* Make sure we're not closed */
            if (Socket->SharedData.State == SocketClosed)
            {
                /* Fail */
                ErrorCode = WSAENOTSOCK;
                goto error;
            }

            /* Check if we failed */
            if (!NT_SUCCESS(Status))
            {
                /* Tell the helper DLL */
                ErrorCode = SockNotifyHelperDll(Socket, WSH_NOTIFY_CONNECT_ERROR);
            }

            /* Keep looping if the Helper DLL wants us to */
        } while (ErrorCode == WSATRY_AGAIN);
    }

    /* Check for error */
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        ErrorCode = NtStatusToSocketError(Status);
        goto error;
    }

    /* Now do post-processing */
    ErrorCode = SockPostProcessConnect(Socket);
    if (ErrorCode != NO_ERROR) goto error;

    /* Check if we had callee data */
    if ((lpCalleeData) && (lpCalleeData->buf) && (lpCalleeData->len > 0))
    {
        /* Set it */
        ErrorCode = SockGetConnectData(Socket,
                                       IOCTL_AFD_SET_CONNECT_DATA_SIZE,
                                       lpCalleeData->buf,
                                       ConnectDataLength,
                                       &ConnectDataLength);
        if (ErrorCode == NO_ERROR)
        {
            /* If we didn't get any data, then assume the buffer is empty */
            if (!lpCalleeData->len) lpCalleeData->buf = NULL;
        }
        else
        {
            /* This isn't fatal, assume we didn't get anything instead */
            lpCalleeData->len = 0;
            lpCalleeData->buf = NULL;
        }

        /* Assume success */
        ErrorCode = NO_ERROR;
    }

error:

    /* Check if we need to free the connect info from the heap */
    if (ConnectInfo && (ConnectInfo != (PVOID)ConnectBuffer))
    {
        /* Free it */
        RtlFreeHeap(SockPrivateHeap, 0, ConnectInfo);
    }

    /* Check if this the success path */
    if (ErrorCode == NO_ERROR)
    {
        /* Check if FD_WRITE is being select()ed */
        if (Socket->SharedData.AsyncEvents & FD_WRITE)
        {
            /* Re-enable it */
            SockReenableAsyncSelectEvent(Socket, FD_WRITE);
        }
    }

    /* Return the error */
    return ErrorCode;
}

INT
WSPAPI 
SockDoConnect(SOCKET Handle, 
              const struct sockaddr *SocketAddress, 
              INT SocketAddressLength, 
              LPWSABUF lpCallerData, 
              LPWSABUF lpCalleeData, 
              LPQOS lpSQOS, 
              LPQOS lpGQOS)
{
    PSOCKET_INFORMATION Socket;
    SOCKADDR_INFO SocketInfo;
    PSOCKADDR Sockaddr;
    PWINSOCK_TEB_DATA ThreadData;
    ULONG SockaddrLength;
    INT ErrorCode, ReturnValue;
    DWORD ConnectDataLength;
    DWORD BytesReturned;

    /* Enter prolog */
    ErrorCode = SockEnterApiFast(&ThreadData);
    if (ErrorCode != NO_ERROR) return ErrorCode;

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

    /* Make sure we're not already connected unless we are a datagram socket */
    if ((Socket->SharedData.State == SocketConnected) && 
        !(MSAFD_IS_DGRAM_SOCK(Socket)))
    {
        /* Fail */
        ErrorCode = WSAEISCONN;
        goto error;
    }

    /* Check if async connect was in progress */
    if (Socket->AsyncData)
    {
        /* We have to clean it up */
        SockIsSocketConnected(Socket);

        /* Check again */
        if (Socket->AsyncData)
        {
            /* Can't do anything but fail now */
            ErrorCode = WSAEALREADY;
            goto error;
        }
    }

    /* Make sure we're either unbound, bound, or connected */
    if ((Socket->SharedData.State != SocketOpen) &&
        (Socket->SharedData.State != SocketBound) &&
        (Socket->SharedData.State != SocketConnected))
    {
        /* Fail */
        ErrorCode = WSAEINVAL;
        goto error;
    }

    /* Normalize the address length */
    SocketAddressLength = min(SocketAddressLength,
                              Socket->HelperData->MaxWSAddressLength);

    /* Also make sure it's not too small */
    if (SocketAddressLength < Socket->HelperData->MinWSAddressLength)
    {
        /* Fail */
        ErrorCode = WSAEFAULT;
        goto error;
    }

    /* 
     * If this is a connected socket, and the address is null (0.0.0.0),
     * then do a partial disconnect if this is a datagram socket.
     */
    if ((Socket->SharedData.State == SocketConnected) && 
        (MSAFD_IS_DGRAM_SOCK(Socket)) && 
        (IsSockaddrEqualToZero(SocketAddress, SocketAddressLength)))
    {
        /* Disconnect the socket and return */
        return UnconnectDatagramSocket(Socket);
    }

    /* Make sure the Address Family is valid */
    if (Socket->SharedData.AddressFamily != SocketAddress->sa_family)
    {
        /* Fail */
        ErrorCode = WSAEAFNOSUPPORT;
        goto error;
    }

    /* If this is a non-broadcast datagram socket */
    if ((MSAFD_IS_DGRAM_SOCK(Socket) && !(Socket->SharedData.Broadcast)))
    {
        /* Find out what kind of address this is */
        ErrorCode = Socket->HelperData->WSHGetSockaddrType((PSOCKADDR)SocketAddress,
                                                           SocketAddressLength,
                                                           &SocketInfo);
        if (ErrorCode != NO_ERROR)
        {
            /* Find out if this is a broadcast address */
            if (SocketInfo.AddressInfo == SockaddrAddressInfoBroadcast)
            {
                /* Fail: SO_BROADCAST must be set first in WinSock 2.0+ */
                ErrorCode = WSAEACCES;
            }
        }

        /* A failure here isn't fatal */
        ErrorCode = NO_ERROR;
    }

    /* Check if this is a constrained group */
    if (Socket->SharedData.GroupType == SG_CONSTRAINED_GROUP)
    {
        /* Validate the address and fail if it's not consistent */
        ErrorCode = SockIsAddressConsistentWithConstrainedGroup(Socket,
                                                                Socket->SharedData.GroupID,
                                                                (PSOCKADDR)SocketAddress,
                                                                SocketAddressLength);
        if (ErrorCode != NO_ERROR) goto error;
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

            /* Bind it */
            ReturnValue = WSPBind(Handle,
                                  Sockaddr,
                                  SockaddrLength,
                                  &ErrorCode);

            /* Free memory */
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

    /* Check if we have caller data */
    if ((lpCallerData) && (lpCallerData->buf) && (lpCallerData->len > 0))
    {
        /* Set it */
        ConnectDataLength = lpCallerData->len;
        ErrorCode = SockGetConnectData(Socket,
                                       IOCTL_AFD_SET_CONNECT_DATA,
                                       lpCallerData->buf,
                                       ConnectDataLength,
                                       &ConnectDataLength);
        if (ErrorCode != NO_ERROR) goto error;
    }

    /* Now check if QOS is supported */
    if ((Socket->SharedData.ServiceFlags1 & XP1_QOS_SUPPORTED)) 
    {
        /* Check if we have QoS data */
        if (lpSQOS)
        {
            /* Send the IOCTL */
            ReturnValue = WSPIoctl(Handle,
                                   SIO_SET_QOS,
                                   lpSQOS,
                                   sizeof(*lpSQOS),
                                   NULL,
                                   0,
                                   &BytesReturned,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &ErrorCode);
            if (ReturnValue == SOCKET_ERROR) goto error;
        }

        /* Check if we have Group QoS data */
        if (lpGQOS)
        {
            /* Send the IOCTL */
            ReturnValue = WSPIoctl(Handle,
                                   SIO_SET_QOS,
                                   lpGQOS,
                                   sizeof(*lpGQOS),
                                   NULL,
                                   0,
                                   &BytesReturned,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &ErrorCode);
            if (ReturnValue == SOCKET_ERROR) goto error;
        }
    }

    /* Save the address */
    RtlCopyMemory(Socket->RemoteAddress, SocketAddress, SocketAddressLength);
    Socket->SharedData.SizeOfRemoteAddress = SocketAddressLength;

    /* Check if we have callee data */
    if ((lpCalleeData) && (lpCalleeData->buf) && (lpCalleeData->len > 0))
    {
        /* Set it */
        ConnectDataLength = lpCalleeData->len;
        ErrorCode = SockGetConnectData(Socket,
                                       IOCTL_AFD_SET_CONNECT_DATA_SIZE,
                                       lpCalleeData->buf,
                                       ConnectDataLength,
                                       &ConnectDataLength);
        if (ErrorCode != NO_ERROR) goto error;
    }

    /* Do the actual connect operation */
    ErrorCode = SockDoConnectReal(Socket,
                                  SocketAddress,
                                  SocketAddressLength,
                                  lpCalleeData,
                                  TRUE);

error:

    /* Check if we had a socket yet */
    if (Socket)
    {
        /* Release the lock and dereference the socket */
        LeaveCriticalSection(&Socket->Lock);
        SockDereferenceSocket(Socket);
    }
    
    /* Return to caller */
    return ErrorCode;
}

INT
WSPAPI 
WSPConnect(SOCKET Handle, 
           const struct sockaddr * SocketAddress, 
           INT SocketAddressLength, 
           LPWSABUF lpCallerData, 
           LPWSABUF lpCalleeData, 
           LPQOS lpSQOS, 
           LPQOS lpGQOS, 
           LPINT lpErrno)
{
    INT ErrorCode;

    /* Check for caller data */
    if (lpCallerData)
    {
        /* Validate it */
        if ((IsBadReadPtr(lpCallerData, sizeof(WSABUF))) || 
            (IsBadReadPtr(lpCallerData->buf, lpCallerData->len)))
        {
            /* The pointers are invalid, fail */
            ErrorCode = WSAEFAULT;
            goto error;
        }
    }

    /* Check for callee data */
    if (lpCalleeData)
    {
        /* Validate it */
        if ((IsBadReadPtr(lpCalleeData, sizeof(WSABUF))) || 
            (IsBadReadPtr(lpCalleeData->buf, lpCalleeData->len)))
        {
            /* The pointers are invalid, fail */
            ErrorCode = WSAEFAULT;
            goto error;
        }
    }

    /* Check for QoS */
    if (lpSQOS)
    {
        /* Validate it */
        if ((IsBadReadPtr(lpSQOS, sizeof(QOS))) || 
            ((lpSQOS->ProviderSpecific.buf) &&
             (lpSQOS->ProviderSpecific.len) &&
             (IsBadReadPtr(lpSQOS->ProviderSpecific.buf,
              lpSQOS->ProviderSpecific.len))))
        {
            /* The pointers are invalid, fail */
            ErrorCode = WSAEFAULT;
            goto error;
        }
    }

    /* Check for Group QoS */
    if (lpGQOS)
    {
        /* Validate it */
        if ((IsBadReadPtr(lpGQOS, sizeof(QOS))) || 
            ((lpGQOS->ProviderSpecific.buf) &&
             (lpGQOS->ProviderSpecific.len) &&
             (IsBadReadPtr(lpGQOS->ProviderSpecific.buf,
              lpGQOS->ProviderSpecific.len))))
        {
            /* The pointers are invalid, fail */
            ErrorCode = WSAEFAULT;
            goto error;
        }
    }

    /* Do the actual connect */
    ErrorCode = SockDoConnect(Handle,
                              SocketAddress,
                              SocketAddressLength,
                              lpCallerData,
                              lpCalleeData,
                              lpSQOS,
                              lpGQOS);

error:
    /* Check if this was an error */
    if (ErrorCode != NO_ERROR)
    {
        /* Return error */
        *lpErrno = ErrorCode;
        return SOCKET_ERROR;
    }

    /* Return success */
    return NO_ERROR;
}

SOCKET
WSPAPI
WSPJoinLeaf(IN SOCKET s,
            IN CONST SOCKADDR *name,
            IN INT namelen,
            IN LPWSABUF lpCallerData,
            OUT LPWSABUF lpCalleeData,
            IN LPQOS lpSQOS,
            IN LPQOS lpGQOS,
            IN DWORD dwFlags,
            OUT LPINT lpErrno)
{
    return (SOCKET)0;
}
