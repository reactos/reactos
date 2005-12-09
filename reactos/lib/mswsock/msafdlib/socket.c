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
SockSocket(INT AddressFamily, 
           INT SocketType, 
           INT Protocol, 
           LPGUID ProviderId, 
           GROUP g,
           DWORD dwFlags,
           DWORD ProviderFlags,
           DWORD ServiceFlags,
           DWORD CatalogEntryId,
           PSOCKET_INFORMATION *NewSocket)
{
    INT ErrorCode;
    UNICODE_STRING TransportName;
    PVOID HelperDllContext;
    PHELPER_DATA HelperData = NULL;
    DWORD HelperEvents;
    PFILE_FULL_EA_INFORMATION Ea = NULL;
    PAFD_CREATE_PACKET AfdPacket;
    SOCKET Handle = INVALID_SOCKET;
    PSOCKET_INFORMATION Socket = NULL;
    BOOLEAN LockInit = FALSE;
    USHORT SizeOfPacket;
    DWORD SizeOfEa, SocketLength;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING DevName;
    LARGE_INTEGER GroupData;
    DWORD CreateOptions = 0;
    IO_STATUS_BLOCK IoStatusBlock;
    PWAH_HANDLE WahHandle;
    NTSTATUS Status;
    CHAR AfdPacketBuffer[96];

    /* Initialize the transport name */
    RtlInitUnicodeString(&TransportName, NULL);

    /* Get Helper Data and Transport */
    ErrorCode = SockGetTdiName(&AddressFamily,
                               &SocketType,
                               &Protocol,
                               ProviderId,
                               g,
                               dwFlags,
                               &TransportName,
                               &HelperDllContext,
                               &HelperData,
                               &HelperEvents);

    /* Check for error */
    if (ErrorCode != NO_ERROR) goto error;

    /* Figure out the socket context structure size */
    SocketLength = sizeof(*Socket) + (HelperData->MinWSAddressLength * 2);

    /* Allocate a socket */
    Socket = SockAllocateHeapRoutine(SockPrivateHeap, 0, SocketLength);
    if (!Socket)
    {
        /* Couldn't create it; we need to tell WSH so it can cleanup */
        if (HelperEvents & WSH_NOTIFY_CLOSE)
        {
            HelperData->WSHNotify(HelperDllContext,
                                  INVALID_SOCKET,
                                  NULL,
                                  NULL,
                                  WSH_NOTIFY_CLOSE);
        }

        /* Fail and return */
        ErrorCode = WSAENOBUFS;
        goto error;
    }

    /* Initialize it */
    RtlZeroMemory(Socket, SocketLength);
    Socket->RefCount = 2;
    Socket->Handle = INVALID_SOCKET;
    Socket->SharedData.State = SocketUndefined;
    Socket->SharedData.AddressFamily = AddressFamily;
    Socket->SharedData.SocketType = SocketType;
    Socket->SharedData.Protocol = Protocol;
    Socket->ProviderId = *ProviderId;
    Socket->HelperContext = HelperDllContext;
    Socket->HelperData = HelperData;
    Socket->HelperEvents = HelperEvents;
    Socket->LocalAddress = (PVOID)(Socket + 1);
    Socket->SharedData.SizeOfLocalAddress = HelperData->MaxWSAddressLength;
    Socket->RemoteAddress = (PVOID)((ULONG_PTR)Socket->LocalAddress +
                                    HelperData->MaxWSAddressLength);
    Socket->SharedData.SizeOfRemoteAddress = HelperData->MaxWSAddressLength;
    Socket->SharedData.UseDelayedAcceptance = HelperData->UseDelayedAcceptance;
    Socket->SharedData.CreateFlags = dwFlags;
    Socket->SharedData.CatalogEntryId = CatalogEntryId;
    Socket->SharedData.ServiceFlags1 = ServiceFlags;
    Socket->SharedData.ProviderFlags = ProviderFlags;
    Socket->SharedData.GroupID = g;
    Socket->SharedData.GroupType = 0;
    Socket->SharedData.UseSAN = FALSE;
    Socket->SanData = NULL;
    Socket->DontUseSan = FALSE;

    /* Initialize the socket lock */
    InitializeCriticalSection(&Socket->Lock);
    LockInit = TRUE;

    /* Packet Size */
    SizeOfPacket = TransportName.Length +  sizeof(*AfdPacket) + sizeof(WCHAR);

    /* EA Size */
    SizeOfEa = SizeOfPacket +  sizeof(*Ea) + AFD_PACKET_COMMAND_LENGTH;

    /* See if our stack buffer is big enough to hold it */
    if (SizeOfEa <= sizeof(AfdPacketBuffer))
    {
        /* Use our stack */
        Ea = (PFILE_FULL_EA_INFORMATION)AfdPacketBuffer;
    }
    else
    {
        /* Allocate from heap */
        Ea = SockAllocateHeapRoutine(SockPrivateHeap, 0, SizeOfEa);
        if (!Ea)
        {
            /* Fail */
            ErrorCode = WSAENOBUFS;
            goto error;
        }
    }

    /* Set up EA */
    Ea->NextEntryOffset = 0;
    Ea->Flags = 0;
    Ea->EaNameLength = AFD_PACKET_COMMAND_LENGTH;
    RtlCopyMemory(Ea->EaName, AfdCommand, AFD_PACKET_COMMAND_LENGTH + 1);
    Ea->EaValueLength = SizeOfPacket;
    
    /* Set up AFD Packet */
    AfdPacket = (PAFD_CREATE_PACKET)(Ea->EaName + Ea->EaNameLength + 1);
    AfdPacket->SizeOfTransportName = TransportName.Length;
    RtlCopyMemory(AfdPacket->TransportName,
                  TransportName.Buffer, 
                  TransportName.Length + sizeof(WCHAR));
    AfdPacket->EndpointFlags = 0;

    /* Set up Endpoint Flags */
    if ((Socket->SharedData.ServiceFlags1 & XP1_CONNECTIONLESS)) 
    {
        /* Check the Socket Type */
        if ((SocketType != SOCK_DGRAM) && (SocketType != SOCK_RAW)) 
        {
            /* Only RAW or UDP can be Connectionless */
            ErrorCode = WSAEINVAL;
            goto error;
        }

        /* Set the flag for AFD */
        AfdPacket->EndpointFlags |= AFD_ENDPOINT_CONNECTIONLESS;
    }
    
    if ((Socket->SharedData.ServiceFlags1 & XP1_MESSAGE_ORIENTED)) 
    {
        /* Check if this is a Stream Socket */
        if (SocketType == SOCK_STREAM) 
        {
            /* Check if we actually support this */
            if (!(Socket->SharedData.ServiceFlags1 & XP1_PSEUDO_STREAM)) 
            {
                /* The Provider doesn't support Message Oriented Streams */
                ErrorCode = WSAEINVAL;
                goto error;
            }
        }

        /* Set the flag for AFD */
        AfdPacket->EndpointFlags |= AFD_ENDPOINT_MESSAGE_ORIENTED;
    }

    /* If this is a Raw Socket, let AFD know */
    if (SocketType == SOCK_RAW) AfdPacket->EndpointFlags |= AFD_ENDPOINT_RAW;

    /* Check if we are a Multipoint Control/Data Root or Leaf */
    if (dwFlags & (WSA_FLAG_MULTIPOINT_C_ROOT | 
                   WSA_FLAG_MULTIPOINT_C_LEAF | 
                   WSA_FLAG_MULTIPOINT_D_ROOT | 
                   WSA_FLAG_MULTIPOINT_D_LEAF)) 
    {
        /* First make sure we support Multipoint */
        if (!(Socket->SharedData.ServiceFlags1 & XP1_SUPPORT_MULTIPOINT)) 
        {
            /* The Provider doesn't actually support Multipoint */
            ErrorCode = WSAEINVAL;
            goto error;
        }

        /* Set the flag for AFD */
        AfdPacket->EndpointFlags |= AFD_ENDPOINT_MULTIPOINT;

        /* Check if we are a Control Plane Root */
        if (dwFlags & WSA_FLAG_MULTIPOINT_C_ROOT) 
        {    
            /* Check if we actually support this or if we're already a leaf */
            if ((!(Socket->SharedData.ServiceFlags1 & 
                   XP1_MULTIPOINT_CONTROL_PLANE)) ||
                ((dwFlags & WSA_FLAG_MULTIPOINT_C_LEAF))) 
            {
                ErrorCode = WSAEINVAL;
                goto error;
            }

            /* Set the flag for AFD */
            AfdPacket->EndpointFlags |= AFD_ENDPOINT_C_ROOT;
        }

        /* Check if we a Data Plane Root */
        if (dwFlags & WSA_FLAG_MULTIPOINT_D_ROOT) 
        {
            /* Check if we actually support this or if we're already a leaf */
            if ((!(Socket->SharedData.ServiceFlags1 & 
                   XP1_MULTIPOINT_DATA_PLANE)) || 
                ((dwFlags & WSA_FLAG_MULTIPOINT_D_LEAF))) 
            {
                ErrorCode = WSAEINVAL;
                goto error;
            }

            /* Set the flag for AFD */
            AfdPacket->EndpointFlags |= AFD_ENDPOINT_D_ROOT;
        }
    }

    /* Set the group ID */
    AfdPacket->GroupID = g;

    /* Set up Object Attributes */
    RtlInitUnicodeString(&DevName, L"\\Device\\Afd\\Endpoint");
    InitializeObjectAttributes(&ObjectAttributes,
                               &DevName, 
                               OBJ_CASE_INSENSITIVE | OBJ_INHERIT, 
                               NULL, 
                               NULL);

    /* Check if we're not using Overlapped I/O */
    if (!(dwFlags & WSA_FLAG_OVERLAPPED))
    {
        /* Set Synchronous I/O */
        CreateOptions = FILE_SYNCHRONOUS_IO_NONALERT;
    }

    /* Acquire the global lock */
    SockAcquireRwLockShared(&SocketGlobalLock);

    /* Create the Socket */
    Status = NtCreateFile((PHANDLE)&Handle,
                          GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          0,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_OPEN_IF,
                          CreateOptions,
                          Ea,
                          SizeOfEa);
    if (!NT_SUCCESS(Status))
    {
        /* Release the lock and fail */
        SockReleaseRwLockShared(&SocketGlobalLock);
        ErrorCode = NtStatusToSocketError(Status);
        goto error;
    }

    /* Save Handle */
    Socket->Handle = Handle;

    /* Check if a group was given */
    if (g != 0) 
    {
        /* Get Group Id and Type */
        ErrorCode = SockGetInformation(Socket,
                                       AFD_INFO_GROUP_ID_TYPE,
                                       NULL,
                                       0,
                                       NULL,
                                       NULL,
                                       &GroupData);

        /* Save them */
        Socket->SharedData.GroupID = GroupData.u.LowPart;
        Socket->SharedData.GroupType = GroupData.u.HighPart;
    }

    /* Check if we need to get the window sizes */
    if (!SockSendBufferWindow)
    {
        /* Get send window size */
        SockGetInformation(Socket,
                           AFD_INFO_SEND_WINDOW_SIZE,
                           NULL,
                           0,
                           NULL,
                           &SockSendBufferWindow, 
                           NULL);

        /* Get receive window size */
        SockGetInformation(Socket, 
                           AFD_INFO_RECEIVE_WINDOW_SIZE, 
                           NULL,
                           0,
                           NULL,
                           &SockReceiveBufferWindow, 
                           NULL);
    }

    /* Save window sizes */
    Socket->SharedData.SizeOfRecvBuffer = SockReceiveBufferWindow;
    Socket->SharedData.SizeOfSendBuffer = SockSendBufferWindow;

    /* Insert it into our table */
    WahHandle = WahInsertHandleContext(SockContextTable, &Socket->WshContext);

    /* We can release the lock now */
    SockReleaseRwLockShared(&SocketGlobalLock);

    /* Check if the handles don't match for some reason */
    if (WahHandle != &Socket->WshContext)
    {
        /* Do they not match? */
        if (WahHandle)
        {
            /* They don't... someone must've used CloseHandle */
            SockDereferenceSocket((PSOCKET_INFORMATION)WahHandle);

            /* Use the correct handle now */
            WahHandle = &Socket->WshContext;
        }
        else
        {
            /* It's not that they don't match: we don't have one at all! */
            ErrorCode = WSAENOBUFS;
            goto error;
        }
    }

error:
    /* Check if we can free the transport name */
    if ((SocketType == SOCK_RAW) && (TransportName.Buffer))
    {
        /* Free it */
        RtlFreeHeap(RtlGetProcessHeap(), 0, TransportName.Buffer);
    }

    /* Check if we have the EA from the heap */
    if ((Ea) && (Ea != (PVOID)AfdPacketBuffer))
    {
        /* Free it */
        RtlFreeHeap(SockPrivateHeap, 0, Ea);
    }

    /* Check if this is actually success */
    if (ErrorCode != NO_ERROR)
    {
        /* Check if we have a socket by now */
        if (Socket)
        {
            /* Tell the Helper DLL we're closing it */
            SockNotifyHelperDll(Socket, WSH_NOTIFY_CLOSE);

            /* Close its handle if it's valid */
            if (Socket->WshContext.Handle != INVALID_HANDLE_VALUE)
            {
                NtClose(Socket->WshContext.Handle);
            }

            /* Delete its lock */
            if (LockInit) DeleteCriticalSection(&Socket->Lock);

            /* Free it */
            RtlFreeHeap(SockPrivateHeap, 0, Socket);
    
            /* Remove our socket reference */
            SockDereferenceSocket(Socket);
        }
    }

    /* Return Socket and error code */
    *NewSocket = Socket;
    return ErrorCode;
}

INT
WSPAPI
SockCloseSocket(IN PSOCKET_INFORMATION Socket)
{
    INT ErrorCode;
    PWINSOCK_TEB_DATA ThreadData = NtCurrentTeb()->WinSockData;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    AFD_DISCONNECT_INFO DisconnectInfo;
    SOCKET_STATE OldState;
    ULONG LingerWait;
    ULONG SendsInProgress;
    ULONG SleepWait;
    BOOLEAN ActiveConnect;

    /* Lock the socket */
    EnterCriticalSection(&Socket->Lock);

    /* If a Close is already in Process... */
    if (Socket->SharedData.State == SocketClosed)
    {
        /* Release lock and fail */
        LeaveCriticalSection(&Socket->Lock);
        return WSAENOTSOCK;
    }

    /* Save the old state and set the new one to closed */
    OldState = Socket->SharedData.State;
    Socket->SharedData.State = SocketClosed;

    /* Check if the socket has an active async data */
    ActiveConnect = (Socket->AsyncData != NULL);

    /* We're done with the socket, release the lock */
    LeaveCriticalSection(&Socket->Lock);
    
    /* 
     * If SO_LINGER is ON and the Socket was connected or had an active async
     * connect context, then we'll disconnect it. Note that we won't do this
     * for connection-less (UDP/RAW) sockets or if a send shutdown is active.
     */
    if ((OldState == SocketConnected || ActiveConnect) &&
        !(Socket->SharedData.SendShutdown) &&
        !MSAFD_IS_DGRAM_SOCK(Socket) &&
        (Socket->SharedData.LingerData.l_onoff))
    {   
        /* We need to respect the timeout */
        SleepWait = 100;
        LingerWait = Socket->SharedData.LingerData.l_linger * 1000;
        
        /* Loop until no more sends are pending, within the timeout */
        while (LingerWait)
        {    
            /* Find out how many Sends are in Progress */
            if (SockGetInformation(Socket, 
                                   AFD_INFO_SENDS_IN_PROGRESS,
                                   NULL,
                                   0,
                                   NULL,
                                   &SendsInProgress,
                                   NULL))
            {
                /* Bail out if anything but NO_ERROR */
                LingerWait = 0;
                break;
            }

            /* Bail out if no more sends are pending */
            if (!SendsInProgress) break;
            
            /* 
             * We have to execute a sleep, so it's kind of like
             * a block. If the socket is Nonblock, we cannot
             * go on since asyncronous operation is expected
             * and we cannot offer it
             */
            if (Socket->SharedData.NonBlocking)
            {
                /* Acquire the socket lock */
                EnterCriticalSection(&Socket->Lock);

                /* Restore the socket state */
                Socket->SharedData.State = OldState;

                /* Release the lock again */
                LeaveCriticalSection(&Socket->Lock);

                /* Fail with error code */
                return WSAEWOULDBLOCK;
            }
            
            /* Now we can sleep, and decrement the linger wait */
            /* 
             * FIXME: It seems Windows does some funky acceleration
             * since the waiting seems to be longer and longer. I
             * don't think this improves performance so much, so we
             * wait a fixed time instead.
             */
            Sleep(SleepWait);
            LingerWait -= SleepWait;
        }
        
        /*
         * We have reached the timeout or sends are over.
         * Disconnect if the timeout has been reached. 
         */
        if (LingerWait <= 0)
        {
            /* There is no timeout, and this is an abortive disconnect */
            DisconnectInfo.Timeout = RtlConvertLongToLargeInteger(0);
            DisconnectInfo.DisconnectType = AFD_DISCONNECT_ABORT;
            
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
            /* Check if the operation is pending */
            if (Status == STATUS_PENDING)
            {
                /* Wait for completion */
                SockWaitForSingleObject(ThreadData->EventHandle,
                                        Socket->Handle,
                                        !Socket->SharedData.LingerData.l_onoff ?
                                        NO_BLOCKING_HOOK : ALWAYS_BLOCKING_HOOK,
                                        NO_TIMEOUT);

                /* Get new status */
                Status = IoStatusBlock.Status;
            }

            /* We actually accept errors, unless the driver wasn't ready */
            if (Status == STATUS_DEVICE_NOT_READY)
            {
                /* This is the equivalent of a WOULDBLOCK, which we fail */
                /* Acquire the socket lock */
                EnterCriticalSection(&Socket->Lock);

                /* Restore the socket state */
                Socket->SharedData.State = OldState;

                /* Release the lock again */
                LeaveCriticalSection(&Socket->Lock);

                /* Fail with error code */
                return WSAEWOULDBLOCK;
            }
        }
    }

    /* Acquire the global lock to protect the handle table */
    SockAcquireRwLockShared(&SocketGlobalLock);

    /* Protect the socket too */
    EnterCriticalSection(&Socket->Lock);
    
    /* Notify the Helper DLL of Socket Closure */
    ErrorCode = SockNotifyHelperDll(Socket, WSH_NOTIFY_CLOSE);

    /* Cleanup Time! */
    Socket->HelperContext = NULL;
    Socket->SharedData.AsyncDisabledEvents = -1;
    if (Socket->TdiAddressHandle)
    {
        /* Close and forget the handle */
        NtClose(Socket->TdiAddressHandle);
        Socket->TdiAddressHandle = NULL;
    }
    if (Socket->TdiConnectionHandle)
    {
        /* Close and forget the handle */
        NtClose(Socket->TdiConnectionHandle);
        Socket->TdiConnectionHandle = NULL;
    }

    /* Remove the handle from the table */
    ErrorCode = WahRemoveHandleContext(SockContextTable, &Socket->WshContext);
    if (ErrorCode == NO_ERROR)
    {
        /* Close the socket's handle */
        NtClose(Socket->WshContext.Handle);

        /* Dereference the socket */
        SockDereferenceSocket(Socket);
    }
    else
    {
        /* This isn't a socket anymore, or something */
        ErrorCode = WSAENOTSOCK;
    }

    /* Release both locks */
    LeaveCriticalSection(&Socket->Lock);
    SockReleaseRwLockShared(&SocketGlobalLock);

    /* Return success */
    return ErrorCode;
}

SOCKET
WSPAPI 
WSPSocket(INT AddressFamily, 
          INT SocketType, 
          INT Protocol, 
          LPWSAPROTOCOL_INFOW lpProtocolInfo, 
          GROUP g, 
          DWORD dwFlags, 
          LPINT lpErrno)  
{
    DWORD CatalogId;
    SOCKET Handle = INVALID_SOCKET;
    INT ErrorCode;
    DWORD ServiceFlags, ProviderFlags;
    PWINSOCK_TEB_DATA ThreadData;
    PSOCKET_INFORMATION Socket;
    GUID ProviderId;

    /* Enter prolog */
    ErrorCode = SockEnterApiFast(&ThreadData);
    if (ErrorCode != NO_ERROR)
    {
        /* Fail */
        *lpErrno = ErrorCode;
        return INVALID_SOCKET;
    }

    /* Get the catalog ID */
    CatalogId = lpProtocolInfo->dwCatalogEntryId;

    /* Check if this is a duplication */
    if(lpProtocolInfo->dwProviderReserved)
    {
        /* Get the duplicate handle */
        Handle = (SOCKET)lpProtocolInfo->dwProviderReserved;

        /* Get our structure for it */
        Socket = SockFindAndReferenceSocket(Handle, TRUE);
        if(Socket)
        {
            /* Tell Winsock about it */
            Socket->Handle = SockUpcallTable->lpWPUModifyIFSHandle(CatalogId,
                                                                   Handle,
                                                                   &ErrorCode);
            /* Check if we got an invalid handle back */
            if(Socket->Handle == INVALID_SOCKET)
            {
                /* Restore it for the error path */
                Socket->Handle = Handle;
            }
        }
        else
        {
            /* The duplicate handle is invalid */
            ErrorCode = WSAEINVAL;
        }

        /* Fail */
        goto error;        
    }

    /* See if the address family should be recovered from the protocl info */
    if (!AddressFamily || AddressFamily == FROM_PROTOCOL_INFO)
    {
        /* Use protocol info data */
        AddressFamily = lpProtocolInfo->iAddressFamily;
    }

    /* See if the address family should be recovered from the protocl info */
    if(!SocketType || SocketType == FROM_PROTOCOL_INFO )
    {
        /* Use protocol info data */
        SocketType = lpProtocolInfo->iSocketType;
    }

    /* See if the address family should be recovered from the protocl info */
    if(Protocol == FROM_PROTOCOL_INFO)
    {
        /* Use protocol info data */
        Protocol = lpProtocolInfo->iProtocol;
    }

    /* Save the service, provider flags and provider ID */
    ServiceFlags = lpProtocolInfo->dwServiceFlags1;
    ProviderFlags = lpProtocolInfo->dwProviderFlags;
    ProviderId = lpProtocolInfo->ProviderId;

    /* Create the actual socket */
    ErrorCode = SockSocket(AddressFamily,
                           SocketType,
                           Protocol,
                           &ProviderId,
                           g,
                           dwFlags,
                           ProviderFlags,
                           ServiceFlags,
                           CatalogId,
                           &Socket);
    if (ErrorCode == ERROR_SUCCESS)
    {
        /* Set status to opened */
        Socket->SharedData.State = SocketOpen;

        /* Create the Socket Context */
        ErrorCode = SockSetHandleContext(Socket);
        if (ErrorCode != NO_ERROR)
        {
            /* Release the lock, close the socket and fail */
            LeaveCriticalSection(&Socket->Lock);
            SockCloseSocket(Socket);
            goto error;
        }

        /* Notify Winsock */
        Handle = SockUpcallTable->lpWPUModifyIFSHandle(Socket->SharedData.CatalogEntryId,
                                                       (SOCKET)Socket->WshContext.Handle,
                                                       &ErrorCode);

        /* Does Winsock not like it? */
        if (Handle == INVALID_SOCKET)
        {
            /* Release the lock, close the socket and fail */
            LeaveCriticalSection(&Socket->Lock);
            SockCloseSocket(Socket);
            goto error;
        }

        /* Release the lock */
        LeaveCriticalSection(&Socket->Lock);
    }
    
error:
    /* Write return code */
    *lpErrno = ErrorCode;

    /* Check if we have a socket and dereference it */
    if (Socket) SockDereferenceSocket(Socket);

    /* Return handle */
    return Handle;
}

INT
WSPAPI
WSPCloseSocket(IN SOCKET Handle,
               OUT LPINT lpErrno)
{
    INT ErrorCode;
    PSOCKET_INFORMATION Socket;
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

    /* Close it */
    ErrorCode = SockCloseSocket(Socket);

    /* Remove the final reference */
    SockDereferenceSocket(Socket);

    /* Check if we got here by error */
    if (ErrorCode != NO_ERROR)
    {
        /* Fail */
        *lpErrno = ErrorCode;
        return SOCKET_ERROR;
    }

    /* Return success */
    return NO_ERROR;
}
