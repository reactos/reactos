/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Ancillary Function Driver DLL
 * FILE:        dll/win32/msafd/misc/dllmain.c
 * PURPOSE:     DLL entry point
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Alex Ionescu (alex@relsoft.net)
 * REVISIONS:
 *              CSH 01/09-2000 Created
 *              Alex 16/07/2004 - Complete Rewrite
 */

#include <msafd.h>

#include <winuser.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(msafd);

HANDLE GlobalHeap;
WSPUPCALLTABLE Upcalls;
LPWPUCOMPLETEOVERLAPPEDREQUEST lpWPUCompleteOverlappedRequest;
PSOCKET_INFORMATION SocketListHead = NULL;
CRITICAL_SECTION SocketListLock;
LIST_ENTRY SockHelpersListHead = { NULL, NULL };
ULONG SockAsyncThreadRefCount;
HANDLE SockAsyncHelperAfdHandle;
HANDLE SockAsyncCompletionPort = NULL;
BOOLEAN SockAsyncSelectCalled;



/*
 * FUNCTION: Creates a new socket
 * ARGUMENTS:
 *     af             = Address family
 *     type           = Socket type
 *     protocol       = Protocol type
 *     lpProtocolInfo = Pointer to protocol information
 *     g              = Reserved
 *     dwFlags        = Socket flags
 *     lpErrno        = Address of buffer for error information
 * RETURNS:
 *     Created socket, or INVALID_SOCKET if it could not be created
 */
SOCKET
WSPAPI
WSPSocket(int AddressFamily,
          int SocketType,
          int Protocol,
          LPWSAPROTOCOL_INFOW lpProtocolInfo,
          GROUP g,
          DWORD dwFlags,
          LPINT lpErrno)
{
    OBJECT_ATTRIBUTES           Object;
    IO_STATUS_BLOCK             IOSB;
    USHORT                      SizeOfPacket;
    ULONG                       SizeOfEA;
    PAFD_CREATE_PACKET          AfdPacket;
    HANDLE                      Sock;
    PSOCKET_INFORMATION         Socket = NULL;
    PFILE_FULL_EA_INFORMATION   EABuffer = NULL;
    PHELPER_DATA                HelperData;
    PVOID                       HelperDLLContext;
    DWORD                       HelperEvents;
    UNICODE_STRING              TransportName;
    UNICODE_STRING              DevName;
    LARGE_INTEGER               GroupData;
    INT                         Status;

    TRACE("Creating Socket, getting TDI Name - AddressFamily (%d)  SocketType (%d)  Protocol (%d).\n",
        AddressFamily, SocketType, Protocol);

    /* Get Helper Data and Transport */
    Status = SockGetTdiName (&AddressFamily,
                             &SocketType,
                             &Protocol,
                             g,
                             dwFlags,
                             &TransportName,
                             &HelperDLLContext,
                             &HelperData,
                             &HelperEvents);

    /* Check for error */
    if (Status != NO_ERROR)
    {
        ERR("SockGetTdiName: Status %x\n", Status);
        goto error;
    }

    /* AFD Device Name */
    RtlInitUnicodeString(&DevName, L"\\Device\\Afd\\Endpoint");

    /* Set Socket Data */
    Socket = HeapAlloc(GlobalHeap, 0, sizeof(*Socket));
    if (!Socket)
        return MsafdReturnWithErrno(STATUS_INSUFFICIENT_RESOURCES, lpErrno, 0, NULL);

    RtlZeroMemory(Socket, sizeof(*Socket));
    Socket->RefCount = 2;
    Socket->Handle = -1;
    Socket->SharedData.Listening = FALSE;
    Socket->SharedData.State = SocketOpen;
    Socket->SharedData.AddressFamily = AddressFamily;
    Socket->SharedData.SocketType = SocketType;
    Socket->SharedData.Protocol = Protocol;
    Socket->HelperContext = HelperDLLContext;
    Socket->HelperData = HelperData;
    Socket->HelperEvents = HelperEvents;
    Socket->LocalAddress = &Socket->WSLocalAddress;
    Socket->SharedData.SizeOfLocalAddress = HelperData->MaxWSAddressLength;
    Socket->RemoteAddress = &Socket->WSRemoteAddress;
    Socket->SharedData.SizeOfRemoteAddress = HelperData->MaxWSAddressLength;
    Socket->SharedData.UseDelayedAcceptance = HelperData->UseDelayedAcceptance;
    Socket->SharedData.CreateFlags = dwFlags;
    Socket->SharedData.ServiceFlags1 = lpProtocolInfo->dwServiceFlags1;
    Socket->SharedData.ProviderFlags = lpProtocolInfo->dwProviderFlags;
    Socket->SharedData.GroupID = g;
    Socket->SharedData.GroupType = 0;
    Socket->SharedData.UseSAN = FALSE;
    Socket->SharedData.NonBlocking = FALSE; /* Sockets start blocking */
    Socket->SanData = NULL;
    RtlCopyMemory(&Socket->ProtocolInfo, lpProtocolInfo, sizeof(Socket->ProtocolInfo));

    /* Ask alex about this */
    if( Socket->SharedData.SocketType == SOCK_DGRAM ||
        Socket->SharedData.SocketType == SOCK_RAW )
    {
        TRACE("Connectionless socket\n");
        Socket->SharedData.ServiceFlags1 |= XP1_CONNECTIONLESS;
    }

    /* Packet Size */
    SizeOfPacket = TransportName.Length + sizeof(AFD_CREATE_PACKET) + sizeof(WCHAR);

    /* EA Size */
    SizeOfEA = SizeOfPacket + sizeof(FILE_FULL_EA_INFORMATION) + AFD_PACKET_COMMAND_LENGTH;

    /* Set up EA Buffer */
    EABuffer = HeapAlloc(GlobalHeap, 0, SizeOfEA);
    if (!EABuffer)
        return MsafdReturnWithErrno(STATUS_INSUFFICIENT_RESOURCES, lpErrno, 0, NULL);

    RtlZeroMemory(EABuffer, SizeOfEA);
    EABuffer->NextEntryOffset = 0;
    EABuffer->Flags = 0;
    EABuffer->EaNameLength = AFD_PACKET_COMMAND_LENGTH;
    RtlCopyMemory (EABuffer->EaName,
                   AfdCommand,
                   AFD_PACKET_COMMAND_LENGTH + 1);
    EABuffer->EaValueLength = SizeOfPacket;

    /* Set up AFD Packet */
    AfdPacket = (PAFD_CREATE_PACKET)(EABuffer->EaName + EABuffer->EaNameLength + 1);
    AfdPacket->SizeOfTransportName = TransportName.Length;
    RtlCopyMemory (AfdPacket->TransportName,
                   TransportName.Buffer,
                   TransportName.Length + sizeof(WCHAR));
    AfdPacket->GroupID = g;

    /* Set up Endpoint Flags */
    if ((Socket->SharedData.ServiceFlags1 & XP1_CONNECTIONLESS) != 0)
    {
        if ((SocketType != SOCK_DGRAM) && (SocketType != SOCK_RAW))
        {
            /* Only RAW or UDP can be Connectionless */
            goto error;
        }
        AfdPacket->EndpointFlags |= AFD_ENDPOINT_CONNECTIONLESS;
    }

    if ((Socket->SharedData.ServiceFlags1 & XP1_MESSAGE_ORIENTED) != 0)
    {
        if (SocketType == SOCK_STREAM)
        {
            if ((Socket->SharedData.ServiceFlags1 & XP1_PSEUDO_STREAM) == 0)
            {
                /* The Provider doesn't actually support Message Oriented Streams */
                goto error;
            }
        }
        AfdPacket->EndpointFlags |= AFD_ENDPOINT_MESSAGE_ORIENTED;
    }

    if (SocketType == SOCK_RAW) AfdPacket->EndpointFlags |= AFD_ENDPOINT_RAW;

    if (dwFlags & (WSA_FLAG_MULTIPOINT_C_ROOT |
                   WSA_FLAG_MULTIPOINT_C_LEAF |
                   WSA_FLAG_MULTIPOINT_D_ROOT |
                   WSA_FLAG_MULTIPOINT_D_LEAF))
    {
        if ((Socket->SharedData.ServiceFlags1 & XP1_SUPPORT_MULTIPOINT) == 0)
        {
            /* The Provider doesn't actually support Multipoint */
            goto error;
        }
        AfdPacket->EndpointFlags |= AFD_ENDPOINT_MULTIPOINT;

        if (dwFlags & WSA_FLAG_MULTIPOINT_C_ROOT)
        {
            if (((Socket->SharedData.ServiceFlags1 & XP1_MULTIPOINT_CONTROL_PLANE) == 0)
                || ((dwFlags & WSA_FLAG_MULTIPOINT_C_LEAF) != 0))
            {
                /* The Provider doesn't support Control Planes, or you already gave a leaf */
                goto error;
            }
            AfdPacket->EndpointFlags |= AFD_ENDPOINT_C_ROOT;
        }

        if (dwFlags & WSA_FLAG_MULTIPOINT_D_ROOT)
        {
            if (((Socket->SharedData.ServiceFlags1 & XP1_MULTIPOINT_DATA_PLANE) == 0)
                || ((dwFlags & WSA_FLAG_MULTIPOINT_D_LEAF) != 0))
            {
                /* The Provider doesn't support Data Planes, or you already gave a leaf */
                goto error;
            }
            AfdPacket->EndpointFlags |= AFD_ENDPOINT_D_ROOT;
        }
    }

    /* Set up Object Attributes */
    InitializeObjectAttributes (&Object,
                                &DevName,
                                OBJ_CASE_INSENSITIVE | OBJ_INHERIT,
                                0,
                                0);

    /* Create the Socket as asynchronous. That means we have to block
    ourselves after every call to NtDeviceIoControlFile. This is
    because the kernel doesn't support overlapping synchronous I/O
    requests (made from multiple threads) at this time (Sep 2005) */
    Status = NtCreateFile(&Sock,
                          GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                          &Object,
                          &IOSB,
                          NULL,
                          0,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_OPEN_IF,
                          0,
                          EABuffer,
                          SizeOfEA);

    HeapFree(GlobalHeap, 0, EABuffer);

    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to open socket. Status 0x%08x\n", Status);

        HeapFree(GlobalHeap, 0, Socket);

        return MsafdReturnWithErrno(Status, lpErrno, 0, NULL);
    }

    /* Save Handle */
    Socket->Handle = (SOCKET)Sock;

    /* Save Group Info */
    if (g != 0)
    {
        GetSocketInformation(Socket, AFD_INFO_GROUP_ID_TYPE, NULL, NULL, &GroupData);
        Socket->SharedData.GroupID = GroupData.u.LowPart;
        Socket->SharedData.GroupType = GroupData.u.HighPart;
    }

    /* Get Window Sizes and Save them */
    GetSocketInformation (Socket,
                          AFD_INFO_SEND_WINDOW_SIZE,
                          NULL,
                          &Socket->SharedData.SizeOfSendBuffer,
                          NULL);

    GetSocketInformation (Socket,
                          AFD_INFO_RECEIVE_WINDOW_SIZE,
                          NULL,
                          &Socket->SharedData.SizeOfRecvBuffer,
                          NULL);

    /* Save in Process Sockets List */
    EnterCriticalSection(&SocketListLock);
    Socket->NextSocket = SocketListHead;
    SocketListHead = Socket;
    LeaveCriticalSection(&SocketListLock);

    /* Create the Socket Context */
    CreateContext(Socket);

    /* Notify Winsock */
    Upcalls.lpWPUModifyIFSHandle(Socket->ProtocolInfo.dwCatalogEntryId, (SOCKET)Sock, lpErrno);

    /* Return Socket Handle */
    TRACE("Success %x\n", Sock);

    return (SOCKET)Sock;

error:
    ERR("Ending %x\n", Status);

    if( Socket )
        HeapFree(GlobalHeap, 0, Socket);

    if( EABuffer )
        HeapFree(GlobalHeap, 0, EABuffer);

    if( lpErrno )
        *lpErrno = Status;

    return INVALID_SOCKET;
}

INT
TranslateNtStatusError(NTSTATUS Status)
{
    switch (Status)
    {
       case STATUS_CANT_WAIT:
          return WSAEWOULDBLOCK;

       case STATUS_TIMEOUT:
          return WSAETIMEDOUT;

       case STATUS_SUCCESS:
          return NO_ERROR;

       case STATUS_FILE_CLOSED:
       case STATUS_END_OF_FILE:
          return WSAESHUTDOWN;

       case STATUS_PENDING:
          return WSA_IO_PENDING;

       case STATUS_BUFFER_TOO_SMALL:
       case STATUS_BUFFER_OVERFLOW:
          return WSAEMSGSIZE;

       case STATUS_NO_MEMORY:
       case STATUS_INSUFFICIENT_RESOURCES:
          return WSAENOBUFS;

       case STATUS_INVALID_CONNECTION:
          return WSAENOTCONN;

       case STATUS_PROTOCOL_NOT_SUPPORTED:
          return WSAEAFNOSUPPORT;

       case STATUS_INVALID_ADDRESS:
          return WSAEADDRNOTAVAIL;

       case STATUS_REMOTE_NOT_LISTENING:
          return WSAECONNREFUSED;

       case STATUS_NETWORK_UNREACHABLE:
          return WSAENETUNREACH;

       case STATUS_INVALID_PARAMETER:
          return WSAEINVAL;

       case STATUS_CANCELLED:
          return WSA_OPERATION_ABORTED;

       case STATUS_ADDRESS_ALREADY_EXISTS:
          return WSAEADDRINUSE;

       case STATUS_LOCAL_DISCONNECT:
          return WSAECONNABORTED;

       case STATUS_REMOTE_DISCONNECT:
          return WSAECONNRESET;

       case STATUS_ACCESS_VIOLATION:
          return WSAEFAULT;

       case STATUS_ACCESS_DENIED:
          return WSAEACCES;

       default:
          ERR("MSAFD: Unhandled NTSTATUS value: 0x%x\n", Status);
          return WSAENETDOWN;
    }
}

/*
 * FUNCTION: Closes an open socket
 * ARGUMENTS:
 *     s       = Socket descriptor
 *     lpErrno = Address of buffer for error information
 * RETURNS:
 *     NO_ERROR, or SOCKET_ERROR if the socket could not be closed
 */
INT
WSPAPI
WSPCloseSocket(IN SOCKET Handle,
               OUT LPINT lpErrno)
{
    IO_STATUS_BLOCK IoStatusBlock;
    PSOCKET_INFORMATION Socket = NULL, CurrentSocket;
    NTSTATUS Status;
    HANDLE SockEvent;
    AFD_DISCONNECT_INFO DisconnectInfo;
    SOCKET_STATE OldState;
    LONG LingerWait = -1;

    /* Create the Wait Event */
    Status = NtCreateEvent(&SockEvent,
                           EVENT_ALL_ACCESS,
                           NULL,
                           1,
                           FALSE);

    if(!NT_SUCCESS(Status))
    {
        ERR("NtCreateEvent failed: 0x%08x", Status);
        return SOCKET_ERROR;
    }
    /* Get the Socket Structure associate to this Socket*/
    Socket = GetSocketStructure(Handle);
    if (!Socket)
    {
       NtClose(SockEvent);
       *lpErrno = WSAENOTSOCK;
       return SOCKET_ERROR;
    }

    if (Socket->HelperEvents & WSH_NOTIFY_CLOSE)
    {
        Status = Socket->HelperData->WSHNotify(Socket->HelperContext,
                                               Socket->Handle,
                                               Socket->TdiAddressHandle,
                                               Socket->TdiConnectionHandle,
                                               WSH_NOTIFY_CLOSE);

        if (Status)
        {
            if (lpErrno) *lpErrno = Status;
            ERR("WSHNotify failed. Error 0x%#x", Status);
            NtClose(SockEvent);
            return SOCKET_ERROR;
        }
    }

    /* If a Close is already in Process, give up */
    if (Socket->SharedData.State == SocketClosed)
    {
        WARN("Socket is closing.\n");
        NtClose(SockEvent);
        *lpErrno = WSAENOTSOCK;
        return SOCKET_ERROR;
    }

    /* Set the state to close */
    OldState = Socket->SharedData.State;
    Socket->SharedData.State = SocketClosed;

    /* If SO_LINGER is ON and the Socket is connected, we need to disconnect */
    /* FIXME: Should we do this on Datagram Sockets too? */
    if ((OldState == SocketConnected) && (Socket->SharedData.LingerData.l_onoff))
    {
        ULONG SendsInProgress;
        ULONG SleepWait;

        /* We need to respect the timeout */
        SleepWait = 100;
        LingerWait = Socket->SharedData.LingerData.l_linger * 1000;

        /* Loop until no more sends are pending, within the timeout */
        while (LingerWait)
        {
            /* Find out how many Sends are in Progress */
            if (GetSocketInformation(Socket,
                                     AFD_INFO_SENDS_IN_PROGRESS,
                                     NULL,
                                     &SendsInProgress,
                                     NULL))
            {
                /* Bail out if anything but NO_ERROR */
                LingerWait = 0;
                break;
            }

            /* Bail out if no more sends are pending */
            if (!SendsInProgress)
            {
                LingerWait = -1;
                break;
            }

            /*
             * We have to execute a sleep, so it's kind of like
             * a block. If the socket is Nonblock, we cannot
             * go on since asyncronous operation is expected
             * and we cannot offer it
             */
            if (Socket->SharedData.NonBlocking)
            {
                WARN("Would block!\n");
                NtClose(SockEvent);
                Socket->SharedData.State = OldState;
                *lpErrno = WSAEWOULDBLOCK;
                return SOCKET_ERROR;
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
    }

    if (OldState == SocketConnected)
    {
        if (LingerWait <= 0)
        {
            DisconnectInfo.Timeout = RtlConvertLongToLargeInteger(0);
            DisconnectInfo.DisconnectType = LingerWait < 0 ? AFD_DISCONNECT_SEND : AFD_DISCONNECT_ABORT;

            if (((DisconnectInfo.DisconnectType & AFD_DISCONNECT_SEND) && (!Socket->SharedData.SendShutdown)) ||
                ((DisconnectInfo.DisconnectType & AFD_DISCONNECT_ABORT) && (!Socket->SharedData.ReceiveShutdown)))
            {
                /* Send IOCTL */
                Status = NtDeviceIoControlFile((HANDLE)Handle,
                                               SockEvent,
                                               NULL,
                                               NULL,
                                               &IoStatusBlock,
                                               IOCTL_AFD_DISCONNECT,
                                               &DisconnectInfo,
                                               sizeof(DisconnectInfo),
                                               NULL,
                                               0);

                /* Wait for return */
                if (Status == STATUS_PENDING)
                {
                    WaitForSingleObject(SockEvent, INFINITE);
                    Status = IoStatusBlock.Status;
                }
            }
        }
    }

    /* Cleanup Time! */
    Socket->HelperContext = NULL;
    Socket->SharedData.AsyncDisabledEvents = -1;
    NtClose(Socket->TdiAddressHandle);
    Socket->TdiAddressHandle = NULL;
    NtClose(Socket->TdiConnectionHandle);
    Socket->TdiConnectionHandle = NULL;

    EnterCriticalSection(&SocketListLock);
    if (SocketListHead == Socket)
    {
        SocketListHead = SocketListHead->NextSocket;
    }
    else
    {
        CurrentSocket = SocketListHead;
        while (CurrentSocket->NextSocket)
        {
            if (CurrentSocket->NextSocket == Socket)
            {
                CurrentSocket->NextSocket = CurrentSocket->NextSocket->NextSocket;
                break;
            }

            CurrentSocket = CurrentSocket->NextSocket;
        }
    }
    LeaveCriticalSection(&SocketListLock);

    /* Close the handle */
    NtClose((HANDLE)Handle);
    NtClose(SockEvent);

    HeapFree(GlobalHeap, 0, Socket);
    return MsafdReturnWithErrno(Status, lpErrno, 0, NULL);
}


/*
 * FUNCTION: Associates a local address with a socket
 * ARGUMENTS:
 *     s       = Socket descriptor
 *     name    = Pointer to local address
 *     namelen = Length of name
 *     lpErrno = Address of buffer for error information
 * RETURNS:
 *     0, or SOCKET_ERROR if the socket could not be bound
 */
INT
WSPAPI
WSPBind(SOCKET Handle,
        const struct sockaddr *SocketAddress,
        int SocketAddressLength,
        LPINT lpErrno)
{
    IO_STATUS_BLOCK         IOSB;
    PAFD_BIND_DATA          BindData;
    PSOCKET_INFORMATION     Socket = NULL;
    NTSTATUS                Status;
    SOCKADDR_INFO           SocketInfo;
    HANDLE                  SockEvent;

    /* See below */
    BindData = HeapAlloc(GlobalHeap, 0, 0xA + SocketAddressLength);
    if (!BindData)
    {
        return MsafdReturnWithErrno(STATUS_INSUFFICIENT_RESOURCES, lpErrno, 0, NULL);
    }

    Status = NtCreateEvent(&SockEvent,
                           EVENT_ALL_ACCESS,
                           NULL,
                           1,
                           FALSE);

    if (!NT_SUCCESS(Status))
    {
        HeapFree(GlobalHeap, 0, BindData);
        return SOCKET_ERROR;
    }

    /* Get the Socket Structure associate to this Socket*/
    Socket = GetSocketStructure(Handle);
    if (!Socket)
    {
       HeapFree(GlobalHeap, 0, BindData);
       *lpErrno = WSAENOTSOCK;
       return SOCKET_ERROR;
    }

    /* Set up Address in TDI Format */
    BindData->Address.TAAddressCount = 1;
    BindData->Address.Address[0].AddressLength = SocketAddressLength - sizeof(SocketAddress->sa_family);
    BindData->Address.Address[0].AddressType = SocketAddress->sa_family;
    RtlCopyMemory (BindData->Address.Address[0].Address,
                   SocketAddress->sa_data,
                   SocketAddressLength - sizeof(SocketAddress->sa_family));

    /* Get Address Information */
    Socket->HelperData->WSHGetSockaddrType ((PSOCKADDR)SocketAddress,
                                            SocketAddressLength,
                                            &SocketInfo);

    /* Set the Share Type */
    if (Socket->SharedData.ExclusiveAddressUse)
    {
        BindData->ShareType = AFD_SHARE_EXCLUSIVE;
    }
    else if (SocketInfo.EndpointInfo == SockaddrEndpointInfoWildcard)
    {
        BindData->ShareType = AFD_SHARE_WILDCARD;
    }
    else if (Socket->SharedData.ReuseAddresses)
    {
        BindData->ShareType = AFD_SHARE_REUSE;
    }
    else
    {
        BindData->ShareType = AFD_SHARE_UNIQUE;
    }

    /* Send IOCTL */
    Status = NtDeviceIoControlFile((HANDLE)Socket->Handle,
                                   SockEvent,
                                   NULL,
                                   NULL,
                                   &IOSB,
                                   IOCTL_AFD_BIND,
                                   BindData,
                                   0xA + Socket->SharedData.SizeOfLocalAddress, /* Can't figure out a way to calculate this in C*/
                                   BindData,
                                   0xA + Socket->SharedData.SizeOfLocalAddress); /* Can't figure out a way to calculate this C */

    /* Wait for return */
    if (Status == STATUS_PENDING)
    {
        WaitForSingleObject(SockEvent, INFINITE);
        Status = IOSB.Status;
    }

    NtClose( SockEvent );
    HeapFree(GlobalHeap, 0, BindData);

    if (Status != STATUS_SUCCESS)
        return MsafdReturnWithErrno ( Status, lpErrno, 0, NULL );

    /* Set up Socket Data */
    Socket->SharedData.State = SocketBound;
    Socket->TdiAddressHandle = (HANDLE)IOSB.Information;

    if (Socket->HelperEvents & WSH_NOTIFY_BIND)
    {
        Status = Socket->HelperData->WSHNotify(Socket->HelperContext,
                                               Socket->Handle,
                                               Socket->TdiAddressHandle,
                                               Socket->TdiConnectionHandle,
                                               WSH_NOTIFY_BIND);

        if (Status)
        {
            if (lpErrno) *lpErrno = Status;
            return SOCKET_ERROR;
        }
    }

    return MsafdReturnWithErrno ( Status, lpErrno, 0, NULL );
}

int
WSPAPI
WSPListen(SOCKET Handle,
          int Backlog,
          LPINT lpErrno)
{
    IO_STATUS_BLOCK         IOSB;
    AFD_LISTEN_DATA         ListenData;
    PSOCKET_INFORMATION     Socket = NULL;
    HANDLE                  SockEvent;
    NTSTATUS                Status;

    /* Get the Socket Structure associate to this Socket*/
    Socket = GetSocketStructure(Handle);
    if (!Socket)
    {
       *lpErrno = WSAENOTSOCK;
       return SOCKET_ERROR;
    }

    if (Socket->SharedData.Listening)
        return 0;

    Status = NtCreateEvent(&SockEvent,
                           EVENT_ALL_ACCESS,
                           NULL,
                           1,
                           FALSE);

    if( !NT_SUCCESS(Status) )
        return -1;

    /* Set Up Listen Structure */
    ListenData.UseSAN = FALSE;
    ListenData.UseDelayedAcceptance = Socket->SharedData.UseDelayedAcceptance;
    ListenData.Backlog = Backlog;

    /* Send IOCTL */
    Status = NtDeviceIoControlFile((HANDLE)Socket->Handle,
                                   SockEvent,
                                   NULL,
                                   NULL,
                                   &IOSB,
                                   IOCTL_AFD_START_LISTEN,
                                   &ListenData,
                                   sizeof(ListenData),
                                   NULL,
                                   0);

    /* Wait for return */
    if (Status == STATUS_PENDING)
    {
        WaitForSingleObject(SockEvent, INFINITE);
        Status = IOSB.Status;
    }

    NtClose( SockEvent );

    if (Status != STATUS_SUCCESS)
       return MsafdReturnWithErrno ( Status, lpErrno, 0, NULL );

    /* Set to Listening */
    Socket->SharedData.Listening = TRUE;

    if (Socket->HelperEvents & WSH_NOTIFY_LISTEN)
    {
        Status = Socket->HelperData->WSHNotify(Socket->HelperContext,
                                               Socket->Handle,
                                               Socket->TdiAddressHandle,
                                               Socket->TdiConnectionHandle,
                                               WSH_NOTIFY_LISTEN);

        if (Status)
        {
           if (lpErrno) *lpErrno = Status;
           return SOCKET_ERROR;
        }
    }

    return MsafdReturnWithErrno ( Status, lpErrno, 0, NULL );
}


int
WSPAPI
WSPSelect(IN int nfds,
          IN OUT fd_set *readfds OPTIONAL,
          IN OUT fd_set *writefds OPTIONAL,
          IN OUT fd_set *exceptfds OPTIONAL,
          IN const struct timeval *timeout OPTIONAL,
          OUT LPINT lpErrno)
{
    IO_STATUS_BLOCK     IOSB;
    PAFD_POLL_INFO      PollInfo;
    NTSTATUS            Status;
    ULONG               HandleCount;
    LONG                OutCount = 0;
    ULONG               PollBufferSize;
    PVOID               PollBuffer;
    ULONG               i, j = 0, x;
    HANDLE              SockEvent;
    BOOL                HandleCounted;
    LARGE_INTEGER       Timeout;

    /* Find out how many sockets we have, and how large the buffer needs
     * to be */

    HandleCount = ( readfds ? readfds->fd_count : 0 ) +
                  ( writefds ? writefds->fd_count : 0 ) +
                  ( exceptfds ? exceptfds->fd_count : 0 );

    if ( HandleCount == 0 )
    {
        WARN("No handles! Returning SOCKET_ERROR\n", HandleCount);
        if (lpErrno) *lpErrno = WSAEINVAL;
        return SOCKET_ERROR;
    }

    PollBufferSize = sizeof(*PollInfo) + ((HandleCount - 1) * sizeof(AFD_HANDLE));

    TRACE("HandleCount: %u BufferSize: %u\n", HandleCount, PollBufferSize);

    /* Convert Timeout to NT Format */
    if (timeout == NULL)
    {
        Timeout.u.LowPart = -1;
        Timeout.u.HighPart = 0x7FFFFFFF;
        TRACE("Infinite timeout\n");
    }
    else
    {
        Timeout = RtlEnlargedIntegerMultiply
            ((timeout->tv_sec * 1000) + (timeout->tv_usec / 1000), -10000);
        /* Negative timeouts are illegal.  Since the kernel represents an
         * incremental timeout as a negative number, we check for a positive
         * result.
         */
        if (Timeout.QuadPart > 0)
        {
            if (lpErrno) *lpErrno = WSAEINVAL;
                return SOCKET_ERROR;
        }
        TRACE("Timeout: Orig %d.%06d kernel %d\n",
                     timeout->tv_sec, timeout->tv_usec,
                     Timeout.u.LowPart);
    }

    Status = NtCreateEvent(&SockEvent,
                           EVENT_ALL_ACCESS,
                           NULL,
                           1,
                           FALSE);

    if(!NT_SUCCESS(Status))
    {
        ERR("NtCreateEvent failed, 0x%08x\n", Status);
        return SOCKET_ERROR;
    }

    /* Allocate */
    PollBuffer = HeapAlloc(GlobalHeap, 0, PollBufferSize);

    if (!PollBuffer)
    {
        if (lpErrno)
            *lpErrno = WSAEFAULT;
        NtClose(SockEvent);
        return SOCKET_ERROR;
    }

    PollInfo = (PAFD_POLL_INFO)PollBuffer;

    RtlZeroMemory( PollInfo, PollBufferSize );

    /* Number of handles for AFD to Check */
    PollInfo->Exclusive = FALSE;
    PollInfo->Timeout = Timeout;

    if (readfds != NULL) {
        for (i = 0; i < readfds->fd_count; i++, j++)
        {
            PollInfo->Handles[j].Handle = readfds->fd_array[i];
            PollInfo->Handles[j].Events = AFD_EVENT_RECEIVE |
                                          AFD_EVENT_DISCONNECT |
                                          AFD_EVENT_ABORT |
                                          AFD_EVENT_CLOSE |
                                          AFD_EVENT_ACCEPT;
        }
    }
    if (writefds != NULL)
    {
        for (i = 0; i < writefds->fd_count; i++, j++)
        {
            PollInfo->Handles[j].Handle = writefds->fd_array[i];
            PollInfo->Handles[j].Events = AFD_EVENT_SEND | AFD_EVENT_CONNECT;
        }
    }
    if (exceptfds != NULL)
    {
        for (i = 0; i < exceptfds->fd_count; i++, j++)
        {
            PollInfo->Handles[j].Handle = exceptfds->fd_array[i];
            PollInfo->Handles[j].Events = AFD_EVENT_OOB_RECEIVE | AFD_EVENT_CONNECT_FAIL;
        }
    }

    PollInfo->HandleCount = j;
    PollBufferSize = FIELD_OFFSET(AFD_POLL_INFO, Handles) + PollInfo->HandleCount * sizeof(AFD_HANDLE);

    /* Send IOCTL */
    Status = NtDeviceIoControlFile((HANDLE)PollInfo->Handles[0].Handle,
                                   SockEvent,
                                   NULL,
                                   NULL,
                                   &IOSB,
                                   IOCTL_AFD_SELECT,
                                   PollInfo,
                                   PollBufferSize,
                                   PollInfo,
                                   PollBufferSize);

    TRACE("DeviceIoControlFile => %x\n", Status);

    /* Wait for Completition */
    if (Status == STATUS_PENDING)
    {
        WaitForSingleObject(SockEvent, INFINITE);
        Status = IOSB.Status;
    }

    /* Clear the Structures */
    if( readfds )
        FD_ZERO(readfds);
    if( writefds )
        FD_ZERO(writefds);
    if( exceptfds )
        FD_ZERO(exceptfds);

    /* Loop through return structure */
    HandleCount = PollInfo->HandleCount;

    /* Return in FDSET Format */
    for (i = 0; i < HandleCount; i++)
    {
        HandleCounted = FALSE;
        for(x = 1; x; x<<=1)
        {
            switch (PollInfo->Handles[i].Events & x)
            {
                case AFD_EVENT_RECEIVE:
                case AFD_EVENT_DISCONNECT:
                case AFD_EVENT_ABORT:
                case AFD_EVENT_ACCEPT:
                case AFD_EVENT_CLOSE:
                    TRACE("Event %x on handle %x\n",
                        PollInfo->Handles[i].Events,
                        PollInfo->Handles[i].Handle);
                    if (! HandleCounted)
                    {
                        OutCount++;
                        HandleCounted = TRUE;
                    }
                    if( readfds )
                        FD_SET(PollInfo->Handles[i].Handle, readfds);
                    break;
                case AFD_EVENT_SEND:
                case AFD_EVENT_CONNECT:
                    TRACE("Event %x on handle %x\n",
                        PollInfo->Handles[i].Events,
                        PollInfo->Handles[i].Handle);
                    if (! HandleCounted)
                    {
                        OutCount++;
                        HandleCounted = TRUE;
                    }
                    if( writefds )
                        FD_SET(PollInfo->Handles[i].Handle, writefds);
                    break;
                case AFD_EVENT_OOB_RECEIVE:
                case AFD_EVENT_CONNECT_FAIL:
                    TRACE("Event %x on handle %x\n",
                        PollInfo->Handles[i].Events,
                        PollInfo->Handles[i].Handle);
                    if (! HandleCounted)
                    {
                        OutCount++;
                        HandleCounted = TRUE;
                    }
                    if( exceptfds )
                        FD_SET(PollInfo->Handles[i].Handle, exceptfds);
                    break;
            }
        }
    }

    HeapFree( GlobalHeap, 0, PollBuffer );
    NtClose( SockEvent );

    if( lpErrno )
    {
        switch( IOSB.Status )
        {
            case STATUS_SUCCESS:
            case STATUS_TIMEOUT:
                *lpErrno = 0;
                break;
            default:
                *lpErrno = WSAEINVAL;
                break;
        }
        TRACE("*lpErrno = %x\n", *lpErrno);
    }

    TRACE("%d events\n", OutCount);

    return OutCount;
}

SOCKET
WSPAPI
WSPAccept(SOCKET Handle,
          struct sockaddr *SocketAddress,
          int *SocketAddressLength,
          LPCONDITIONPROC lpfnCondition,
          DWORD dwCallbackData,
          LPINT lpErrno)
{
    IO_STATUS_BLOCK             IOSB;
    PAFD_RECEIVED_ACCEPT_DATA   ListenReceiveData;
    AFD_ACCEPT_DATA             AcceptData;
    AFD_DEFER_ACCEPT_DATA       DeferData;
    AFD_PENDING_ACCEPT_DATA     PendingAcceptData;
    PSOCKET_INFORMATION         Socket = NULL;
    NTSTATUS                    Status;
    struct fd_set               ReadSet;
    struct timeval              Timeout;
    PVOID                       PendingData = NULL;
    ULONG                       PendingDataLength = 0;
    PVOID                       CalleeDataBuffer;
    WSABUF                      CallerData, CalleeID, CallerID, CalleeData;
    PSOCKADDR                   RemoteAddress =  NULL;
    GROUP                       GroupID = 0;
    ULONG                       CallBack;
    SOCKET                      AcceptSocket;
    PSOCKET_INFORMATION         AcceptSocketInfo;
    UCHAR                       ReceiveBuffer[0x1A];
    HANDLE                      SockEvent;

    Status = NtCreateEvent(&SockEvent,
                           EVENT_ALL_ACCESS,
                           NULL,
                           1,
                           FALSE);

    if( !NT_SUCCESS(Status) )
    {
        MsafdReturnWithErrno( Status, lpErrno, 0, NULL );
        return INVALID_SOCKET;
    }

    /* Dynamic Structure...ugh */
    ListenReceiveData = (PAFD_RECEIVED_ACCEPT_DATA)ReceiveBuffer;

    /* Get the Socket Structure associate to this Socket*/
    Socket = GetSocketStructure(Handle);
    if (!Socket)
    {
       NtClose(SockEvent);
       *lpErrno = WSAENOTSOCK;
       return INVALID_SOCKET;
    }

    /* If this is non-blocking, make sure there's something for us to accept */
    FD_ZERO(&ReadSet);
    FD_SET(Socket->Handle, &ReadSet);
    Timeout.tv_sec=0;
    Timeout.tv_usec=0;

    if (WSPSelect(0, &ReadSet, NULL, NULL, &Timeout, lpErrno) == SOCKET_ERROR)
    {
        NtClose(SockEvent);
        return INVALID_SOCKET;
    }

    if (ReadSet.fd_array[0] != Socket->Handle)
    {
        NtClose(SockEvent);
        *lpErrno = WSAEWOULDBLOCK;
        return INVALID_SOCKET;
    }

    /* Send IOCTL */
    Status = NtDeviceIoControlFile((HANDLE)Socket->Handle,
                                   SockEvent,
                                   NULL,
                                   NULL,
                                   &IOSB,
                                   IOCTL_AFD_WAIT_FOR_LISTEN,
                                   NULL,
                                   0,
                                   ListenReceiveData,
                                   0xA + sizeof(*ListenReceiveData));

    /* Wait for return */
    if (Status == STATUS_PENDING)
    {
        WaitForSingleObject(SockEvent, INFINITE);
        Status = IOSB.Status;
    }

    if (!NT_SUCCESS(Status))
    {
        NtClose( SockEvent );
        MsafdReturnWithErrno( Status, lpErrno, 0, NULL );
        return INVALID_SOCKET;
    }

    if (lpfnCondition != NULL)
    {
        if ((Socket->SharedData.ServiceFlags1 & XP1_CONNECT_DATA) != 0)
        {
            /* Find out how much data is pending */
            PendingAcceptData.SequenceNumber = ListenReceiveData->SequenceNumber;
            PendingAcceptData.ReturnSize = TRUE;

            /* Send IOCTL */
            Status = NtDeviceIoControlFile((HANDLE)Socket->Handle,
                                           SockEvent,
                                           NULL,
                                           NULL,
                                           &IOSB,
                                           IOCTL_AFD_GET_PENDING_CONNECT_DATA,
                                           &PendingAcceptData,
                                           sizeof(PendingAcceptData),
                                           &PendingAcceptData,
                                           sizeof(PendingAcceptData));

            /* Wait for return */
            if (Status == STATUS_PENDING)
            {
                WaitForSingleObject(SockEvent, INFINITE);
                Status = IOSB.Status;
            }

            if (!NT_SUCCESS(Status))
            {
                NtClose( SockEvent );
                MsafdReturnWithErrno( Status, lpErrno, 0, NULL );
                return INVALID_SOCKET;
            }

            /* How much data to allocate */
            PendingDataLength = IOSB.Information;

            if (PendingDataLength)
            {
                /* Allocate needed space */
                PendingData = HeapAlloc(GlobalHeap, 0, PendingDataLength);
                if (!PendingData)
                {
                    MsafdReturnWithErrno( STATUS_INSUFFICIENT_RESOURCES, lpErrno, 0, NULL );
                    return INVALID_SOCKET;
                }

                /* We want the data now */
                PendingAcceptData.ReturnSize = FALSE;

                /* Send IOCTL */
                Status = NtDeviceIoControlFile((HANDLE)Socket->Handle,
                                               SockEvent,
                                               NULL,
                                               NULL,
                                               &IOSB,
                                               IOCTL_AFD_GET_PENDING_CONNECT_DATA,
                                               &PendingAcceptData,
                                               sizeof(PendingAcceptData),
                                               PendingData,
                                               PendingDataLength);

                /* Wait for return */
                if (Status == STATUS_PENDING)
                {
                    WaitForSingleObject(SockEvent, INFINITE);
                    Status = IOSB.Status;
                }

                if (!NT_SUCCESS(Status))
                {
                    NtClose( SockEvent );
                    MsafdReturnWithErrno( Status, lpErrno, 0, NULL );
                    return INVALID_SOCKET;
                }
            }
        }

        if ((Socket->SharedData.ServiceFlags1 & XP1_QOS_SUPPORTED) != 0)
        {
            /* I don't support this yet */
        }

        /* Build Callee ID */
        CalleeID.buf = (PVOID)Socket->LocalAddress;
        CalleeID.len = Socket->SharedData.SizeOfLocalAddress;

        RemoteAddress = HeapAlloc(GlobalHeap, 0, sizeof(*RemoteAddress));
        if (!RemoteAddress)
        {
            MsafdReturnWithErrno(STATUS_INSUFFICIENT_RESOURCES, lpErrno, 0, NULL);
            return INVALID_SOCKET;
        }

        /* Set up Address in SOCKADDR Format */
        RtlCopyMemory (RemoteAddress,
                       &ListenReceiveData->Address.Address[0].AddressType,
                       sizeof(*RemoteAddress));

        /* Build Caller ID */
        CallerID.buf = (PVOID)RemoteAddress;
        CallerID.len = sizeof(*RemoteAddress);

        /* Build Caller Data */
        CallerData.buf = PendingData;
        CallerData.len = PendingDataLength;

        /* Check if socket supports Conditional Accept */
        if (Socket->SharedData.UseDelayedAcceptance != 0)
        {
            /* Allocate Buffer for Callee Data */
            CalleeDataBuffer = HeapAlloc(GlobalHeap, 0, 4096);
            if (!CalleeDataBuffer) {
                MsafdReturnWithErrno( STATUS_INSUFFICIENT_RESOURCES, lpErrno, 0, NULL );
                return INVALID_SOCKET;
            }
            CalleeData.buf = CalleeDataBuffer;
            CalleeData.len = 4096;
        }
        else
        {
            /* Nothing */
            CalleeData.buf = 0;
            CalleeData.len = 0;
        }

        /* Call the Condition Function */
        CallBack = (lpfnCondition)(&CallerID,
                                   CallerData.buf == NULL ? NULL : &CallerData,
                                   NULL,
                                   NULL,
                                   &CalleeID,
                                   CalleeData.buf == NULL ? NULL : &CalleeData,
                                   &GroupID,
                                   dwCallbackData);

        if (((CallBack == CF_ACCEPT) && GroupID) != 0)
        {
            /* TBD: Check for Validity */
        }

        if (CallBack == CF_ACCEPT)
        {
            if ((Socket->SharedData.ServiceFlags1 & XP1_QOS_SUPPORTED) != 0)
            {
                /* I don't support this yet */
            }
            if (CalleeData.buf)
            {
                // SockSetConnectData Sockets(SocketID), IOCTL_AFD_SET_CONNECT_DATA, CalleeData.Buffer, CalleeData.BuffSize, 0
            }
        }
        else
        {
            /* Callback rejected. Build Defer Structure */
            DeferData.SequenceNumber = ListenReceiveData->SequenceNumber;
            DeferData.RejectConnection = (CallBack == CF_REJECT);

            /* Send IOCTL */
            Status = NtDeviceIoControlFile((HANDLE)Socket->Handle,
                                           SockEvent,
                                           NULL,
                                           NULL,
                                           &IOSB,
                                           IOCTL_AFD_DEFER_ACCEPT,
                                           &DeferData,
                                           sizeof(DeferData),
                                           NULL,
                                           0);

            /* Wait for return */
            if (Status == STATUS_PENDING)
            {
                WaitForSingleObject(SockEvent, INFINITE);
                Status = IOSB.Status;
            }

            NtClose( SockEvent );

            if (!NT_SUCCESS(Status))
            {
                MsafdReturnWithErrno( Status, lpErrno, 0, NULL );
                return INVALID_SOCKET;
            }

            if (CallBack == CF_REJECT )
            {
                *lpErrno = WSAECONNREFUSED;
                return INVALID_SOCKET;
            }
            else
            {
                *lpErrno = WSAECONNREFUSED;
                return INVALID_SOCKET;
            }
        }
    }

    /* Create a new Socket */
    AcceptSocket = WSPSocket (Socket->SharedData.AddressFamily,
                              Socket->SharedData.SocketType,
                              Socket->SharedData.Protocol,
                              &Socket->ProtocolInfo,
                              GroupID,
                              Socket->SharedData.CreateFlags,
                              lpErrno);
    if (AcceptSocket == INVALID_SOCKET)
        return INVALID_SOCKET;

    /* Set up the Accept Structure */
    AcceptData.ListenHandle = (HANDLE)AcceptSocket;
    AcceptData.SequenceNumber = ListenReceiveData->SequenceNumber;

    /* Send IOCTL to Accept */
    Status = NtDeviceIoControlFile((HANDLE)Socket->Handle,
                                   SockEvent,
                                   NULL,
                                   NULL,
                                   &IOSB,
                                   IOCTL_AFD_ACCEPT,
                                   &AcceptData,
                                   sizeof(AcceptData),
                                   NULL,
                                   0);

    /* Wait for return */
    if (Status == STATUS_PENDING)
    {
        WaitForSingleObject(SockEvent, INFINITE);
        Status = IOSB.Status;
    }

    if (!NT_SUCCESS(Status))
    {
        NtClose(SockEvent);
        WSPCloseSocket( AcceptSocket, lpErrno );
        MsafdReturnWithErrno( Status, lpErrno, 0, NULL );
        return INVALID_SOCKET;
    }

    AcceptSocketInfo = GetSocketStructure(AcceptSocket);
    if (!AcceptSocketInfo)
    {
        NtClose(SockEvent);
        WSPCloseSocket( AcceptSocket, lpErrno );
        MsafdReturnWithErrno( STATUS_PROTOCOL_NOT_SUPPORTED, lpErrno, 0, NULL );
        return INVALID_SOCKET;
    }

    AcceptSocketInfo->SharedData.State = SocketConnected;

    /* Return Address in SOCKADDR FORMAT */
    if( SocketAddress )
    {
        RtlCopyMemory (SocketAddress,
                       &ListenReceiveData->Address.Address[0].AddressType,
                       sizeof(*RemoteAddress));
        if( SocketAddressLength )
            *SocketAddressLength = ListenReceiveData->Address.Address[0].AddressLength;
    }

    NtClose( SockEvent );

    /* Re-enable Async Event */
    SockReenableAsyncSelectEvent(Socket, FD_ACCEPT);

    TRACE("Socket %x\n", AcceptSocket);

    if (Status == STATUS_SUCCESS && (Socket->HelperEvents & WSH_NOTIFY_ACCEPT))
    {
        Status = Socket->HelperData->WSHNotify(Socket->HelperContext,
                                               Socket->Handle,
                                               Socket->TdiAddressHandle,
                                               Socket->TdiConnectionHandle,
                                               WSH_NOTIFY_ACCEPT);

        if (Status)
        {
            if (lpErrno) *lpErrno = Status;
            return INVALID_SOCKET;
        }
    }

    *lpErrno = 0;

    /* Return Socket */
    return AcceptSocket;
}

int
WSPAPI
WSPConnect(SOCKET Handle,
           const struct sockaddr * SocketAddress,
           int SocketAddressLength,
           LPWSABUF lpCallerData,
           LPWSABUF lpCalleeData,
           LPQOS lpSQOS,
           LPQOS lpGQOS,
           LPINT lpErrno)
{
    IO_STATUS_BLOCK         IOSB;
    PAFD_CONNECT_INFO       ConnectInfo = NULL;
    PSOCKET_INFORMATION     Socket;
    NTSTATUS                Status;
    INT                     Errno;
    ULONG                   ConnectDataLength;
    ULONG                   InConnectDataLength;
    INT                     BindAddressLength;
    PSOCKADDR               BindAddress;
    HANDLE                  SockEvent;
    int                     SocketDataLength;

    Status = NtCreateEvent(&SockEvent,
                           EVENT_ALL_ACCESS,
                           NULL,
                           1,
                           FALSE);

    if (!NT_SUCCESS(Status))
        return MsafdReturnWithErrno(Status, lpErrno, 0, NULL);

    TRACE("Called\n");

    /* Get the Socket Structure associate to this Socket*/
    Socket = GetSocketStructure(Handle);
    if (!Socket)
    {
        NtClose(SockEvent);
        if (lpErrno) *lpErrno = WSAENOTSOCK;
        return SOCKET_ERROR;
    }

    /* Bind us First */
    if (Socket->SharedData.State == SocketOpen)
    {
        /* Get the Wildcard Address */
        BindAddressLength = Socket->HelperData->MaxWSAddressLength;
        BindAddress = HeapAlloc(GetProcessHeap(), 0, BindAddressLength);
        if (!BindAddress)
        {
            NtClose(SockEvent);
            return MsafdReturnWithErrno(STATUS_INSUFFICIENT_RESOURCES, lpErrno, 0, NULL);
        }
        Socket->HelperData->WSHGetWildcardSockaddr (Socket->HelperContext,
                                                    BindAddress,
                                                    &BindAddressLength);
        /* Bind it */
        if (WSPBind(Handle, BindAddress, BindAddressLength, lpErrno) == SOCKET_ERROR)
            return SOCKET_ERROR;
    }

    /* Set the Connect Data */
    if (lpCallerData != NULL)
    {
        ConnectDataLength = lpCallerData->len;
        Status = NtDeviceIoControlFile((HANDLE)Handle,
                                        SockEvent,
                                        NULL,
                                        NULL,
                                        &IOSB,
                                        IOCTL_AFD_SET_CONNECT_DATA,
                                        lpCallerData->buf,
                                        ConnectDataLength,
                                        NULL,
                                        0);
        /* Wait for return */
        if (Status == STATUS_PENDING)
        {
            WaitForSingleObject(SockEvent, INFINITE);
            Status = IOSB.Status;
        }

        if (Status != STATUS_SUCCESS)
            goto notify;
    }

    /* Calculate the size of SocketAddress->sa_data */
    SocketDataLength = SocketAddressLength - FIELD_OFFSET(struct sockaddr, sa_data);

    /* Allocate a connection info buffer with SocketDataLength bytes of payload */
    ConnectInfo = HeapAlloc(GetProcessHeap(), 0,
                            FIELD_OFFSET(AFD_CONNECT_INFO,
                                         RemoteAddress.Address[0].Address[SocketDataLength]));
    if (!ConnectInfo)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto notify;
    }

    /* Set up Address in TDI Format */
    ConnectInfo->RemoteAddress.TAAddressCount = 1;
    ConnectInfo->RemoteAddress.Address[0].AddressLength = SocketDataLength;
    ConnectInfo->RemoteAddress.Address[0].AddressType = SocketAddress->sa_family;
    RtlCopyMemory(ConnectInfo->RemoteAddress.Address[0].Address,
                  SocketAddress->sa_data,
                  SocketDataLength);

    /*
    * Disable FD_WRITE and FD_CONNECT
    * The latter fixes a race condition where the FD_CONNECT is re-enabled
    * at the end of this function right after the Async Thread disables it.
    * This should only happen at the *next* WSPConnect
    */
    if (Socket->SharedData.AsyncEvents & FD_CONNECT)
    {
        Socket->SharedData.AsyncDisabledEvents |= FD_CONNECT | FD_WRITE;
    }

    /* Tell AFD that we want Connection Data back, have it allocate a buffer */
    if (lpCalleeData != NULL)
    {
        InConnectDataLength = lpCalleeData->len;
        Status = NtDeviceIoControlFile((HANDLE)Handle,
                                        SockEvent,
                                        NULL,
                                        NULL,
                                        &IOSB,
                                        IOCTL_AFD_SET_CONNECT_DATA_SIZE,
                                        &InConnectDataLength,
                                        sizeof(InConnectDataLength),
                                        NULL,
                                        0);

        /* Wait for return */
        if (Status == STATUS_PENDING)
        {
            WaitForSingleObject(SockEvent, INFINITE);
            Status = IOSB.Status;
        }

        if (Status != STATUS_SUCCESS)
            goto notify;
    }

    /* AFD doesn't seem to care if these are invalid, but let's 0 them anyways */
    ConnectInfo->Root = 0;
    ConnectInfo->UseSAN = FALSE;
    ConnectInfo->Unknown = 0;

    /* FIXME: Handle Async Connect */
    if (Socket->SharedData.NonBlocking)
    {
        ERR("Async Connect UNIMPLEMENTED!\n");
    }

    /* Send IOCTL */
    Status = NtDeviceIoControlFile((HANDLE)Handle,
                                   SockEvent,
                                   NULL,
                                   NULL,
                                   &IOSB,
                                   IOCTL_AFD_CONNECT,
                                   ConnectInfo,
                                   0x22,
                                   NULL,
                                   0);
    /* Wait for return */
    if (Status == STATUS_PENDING)
    {
        WaitForSingleObject(SockEvent, INFINITE);
        Status = IOSB.Status;
    }

    if (Status != STATUS_SUCCESS)
        goto notify;

    Socket->SharedData.State = SocketConnected;
    Socket->TdiConnectionHandle = (HANDLE)IOSB.Information;

    /* Get any pending connect data */
    if (lpCalleeData != NULL)
    {
        Status = NtDeviceIoControlFile((HANDLE)Handle,
                                       SockEvent,
                                       NULL,
                                       NULL,
                                       &IOSB,
                                       IOCTL_AFD_GET_CONNECT_DATA,
                                       NULL,
                                       0,
                                       lpCalleeData->buf,
                                       lpCalleeData->len);
        /* Wait for return */
        if (Status == STATUS_PENDING)
        {
            WaitForSingleObject(SockEvent, INFINITE);
            Status = IOSB.Status;
        }
    }

    TRACE("Ending\n");

notify:
    if (ConnectInfo) HeapFree(GetProcessHeap(), 0, ConnectInfo);

    /* Re-enable Async Event */
    SockReenableAsyncSelectEvent(Socket, FD_WRITE);

    /* FIXME: THIS IS NOT RIGHT!!! HACK HACK HACK! */
    SockReenableAsyncSelectEvent(Socket, FD_CONNECT);

    NtClose(SockEvent);

    if (Status == STATUS_SUCCESS && (Socket->HelperEvents & WSH_NOTIFY_CONNECT))
    {
        Errno = Socket->HelperData->WSHNotify(Socket->HelperContext,
                                              Socket->Handle,
                                              Socket->TdiAddressHandle,
                                              Socket->TdiConnectionHandle,
                                              WSH_NOTIFY_CONNECT);

        if (Errno)
        {
            if (lpErrno) *lpErrno = Errno;
            return SOCKET_ERROR;
        }
    }
    else if (Status != STATUS_SUCCESS && (Socket->HelperEvents & WSH_NOTIFY_CONNECT_ERROR))
    {
        Errno = Socket->HelperData->WSHNotify(Socket->HelperContext,
                                              Socket->Handle,
                                              Socket->TdiAddressHandle,
                                              Socket->TdiConnectionHandle,
                                              WSH_NOTIFY_CONNECT_ERROR);

        if (Errno)
        {
            if (lpErrno) *lpErrno = Errno;
            return SOCKET_ERROR;
        }
    }

    return MsafdReturnWithErrno(Status, lpErrno, 0, NULL);
}
int
WSPAPI
WSPShutdown(SOCKET Handle,
            int HowTo,
            LPINT lpErrno)

{
    IO_STATUS_BLOCK         IOSB;
    AFD_DISCONNECT_INFO     DisconnectInfo;
    PSOCKET_INFORMATION     Socket = NULL;
    NTSTATUS                Status;
    HANDLE                  SockEvent;

    Status = NtCreateEvent(&SockEvent,
                           EVENT_ALL_ACCESS,
                           NULL,
                           1,
                           FALSE);

    if( !NT_SUCCESS(Status) )
        return -1;

    TRACE("Called\n");

    /* Get the Socket Structure associate to this Socket*/
    Socket = GetSocketStructure(Handle);
    if (!Socket)
    {
       NtClose(SockEvent);
       *lpErrno = WSAENOTSOCK;
       return SOCKET_ERROR;
    }

    /* Set AFD Disconnect Type */
    switch (HowTo)
    {
        case SD_RECEIVE:
            DisconnectInfo.DisconnectType = AFD_DISCONNECT_RECV;
            Socket->SharedData.ReceiveShutdown = TRUE;
            break;
        case SD_SEND:
            DisconnectInfo.DisconnectType= AFD_DISCONNECT_SEND;
            Socket->SharedData.SendShutdown = TRUE;
            break;
        case SD_BOTH:
            DisconnectInfo.DisconnectType = AFD_DISCONNECT_RECV | AFD_DISCONNECT_SEND;
            Socket->SharedData.ReceiveShutdown = TRUE;
            Socket->SharedData.SendShutdown = TRUE;
            break;
    }

    DisconnectInfo.Timeout = RtlConvertLongToLargeInteger(-1000000);

    /* Send IOCTL */
    Status = NtDeviceIoControlFile((HANDLE)Handle,
                                   SockEvent,
                                   NULL,
                                   NULL,
                                   &IOSB,
                                   IOCTL_AFD_DISCONNECT,
                                   &DisconnectInfo,
                                   sizeof(DisconnectInfo),
                                   NULL,
                                   0);

    /* Wait for return */
    if (Status == STATUS_PENDING)
    {
        WaitForSingleObject(SockEvent, INFINITE);
        Status = IOSB.Status;
    }

    TRACE("Ending\n");

    NtClose( SockEvent );

    return MsafdReturnWithErrno( Status, lpErrno, 0, NULL );
}


INT
WSPAPI
WSPGetSockName(IN SOCKET Handle,
               OUT LPSOCKADDR Name,
               IN OUT LPINT NameLength,
               OUT LPINT lpErrno)
{
    IO_STATUS_BLOCK         IOSB;
    ULONG                   TdiAddressSize;
	PTDI_ADDRESS_INFO       TdiAddress;
    PTRANSPORT_ADDRESS      SocketAddress;
    PSOCKET_INFORMATION     Socket = NULL;
    NTSTATUS                Status;
    HANDLE                  SockEvent;

    Status = NtCreateEvent(&SockEvent,
                           EVENT_ALL_ACCESS,
                           NULL,
                           1,
                           FALSE);

    if( !NT_SUCCESS(Status) )
        return SOCKET_ERROR;

    /* Get the Socket Structure associate to this Socket*/
    Socket = GetSocketStructure(Handle);
    if (!Socket)
    {
       NtClose(SockEvent);
       *lpErrno = WSAENOTSOCK;
       return SOCKET_ERROR;
    }

    if (!Name || !NameLength)
    {
        NtClose(SockEvent);
        *lpErrno = WSAEFAULT;
        return SOCKET_ERROR;
    }

    /* Allocate a buffer for the address */
    TdiAddressSize =
		sizeof(TRANSPORT_ADDRESS) + Socket->SharedData.SizeOfLocalAddress;
    TdiAddress = HeapAlloc(GlobalHeap, 0, TdiAddressSize);

    if ( TdiAddress == NULL )
    {
        NtClose( SockEvent );
        *lpErrno = WSAENOBUFS;
        return SOCKET_ERROR;
    }

    SocketAddress = &TdiAddress->Address;

    /* Send IOCTL */
    Status = NtDeviceIoControlFile((HANDLE)Socket->Handle,
                                   SockEvent,
                                   NULL,
                                   NULL,
                                   &IOSB,
                                   IOCTL_AFD_GET_SOCK_NAME,
                                   NULL,
                                   0,
                                   TdiAddress,
                                   TdiAddressSize);

    /* Wait for return */
    if (Status == STATUS_PENDING)
    {
        WaitForSingleObject(SockEvent, INFINITE);
        Status = IOSB.Status;
    }

    NtClose( SockEvent );

    if (NT_SUCCESS(Status))
    {
        if (*NameLength >= Socket->SharedData.SizeOfLocalAddress)
        {
            Name->sa_family = SocketAddress->Address[0].AddressType;
            RtlCopyMemory (Name->sa_data,
                           SocketAddress->Address[0].Address,
                           SocketAddress->Address[0].AddressLength);
            *NameLength = Socket->SharedData.SizeOfLocalAddress;
            TRACE("NameLength %d Address: %x Port %x\n",
                          *NameLength, ((struct sockaddr_in *)Name)->sin_addr.s_addr,
                          ((struct sockaddr_in *)Name)->sin_port);
            HeapFree(GlobalHeap, 0, TdiAddress);
            return 0;
        }
        else
        {
            HeapFree(GlobalHeap, 0, TdiAddress);
            *lpErrno = WSAEFAULT;
            return SOCKET_ERROR;
        }
    }

    HeapFree(GlobalHeap, 0, TdiAddress);

    return MsafdReturnWithErrno ( Status, lpErrno, 0, NULL );
}


INT
WSPAPI
WSPGetPeerName(IN SOCKET s,
               OUT LPSOCKADDR Name,
               IN OUT LPINT NameLength,
               OUT LPINT lpErrno)
{
    IO_STATUS_BLOCK         IOSB;
    ULONG                   TdiAddressSize;
    PTRANSPORT_ADDRESS      SocketAddress;
    PSOCKET_INFORMATION     Socket = NULL;
    NTSTATUS                Status;
    HANDLE                  SockEvent;

    Status = NtCreateEvent(&SockEvent,
                           EVENT_ALL_ACCESS,
                           NULL,
                           1,
                           FALSE);

    if( !NT_SUCCESS(Status) )
        return SOCKET_ERROR;

    /* Get the Socket Structure associate to this Socket*/
    Socket = GetSocketStructure(s);
    if (!Socket)
    {
       NtClose(SockEvent);
       *lpErrno = WSAENOTSOCK;
       return SOCKET_ERROR;
    }

    if (Socket->SharedData.State != SocketConnected)
    {
        NtClose(SockEvent);
        *lpErrno = WSAENOTCONN;
        return SOCKET_ERROR;
    }

    if (!Name || !NameLength)
    {
        NtClose(SockEvent);
        *lpErrno = WSAEFAULT;
        return SOCKET_ERROR;
    }

    /* Allocate a buffer for the address */
    TdiAddressSize = sizeof(TRANSPORT_ADDRESS) + Socket->SharedData.SizeOfRemoteAddress;
    SocketAddress = HeapAlloc(GlobalHeap, 0, TdiAddressSize);

    if ( SocketAddress == NULL )
    {
        NtClose( SockEvent );
        *lpErrno = WSAENOBUFS;
        return SOCKET_ERROR;
    }

    /* Send IOCTL */
    Status = NtDeviceIoControlFile((HANDLE)Socket->Handle,
                                   SockEvent,
                                   NULL,
                                   NULL,
                                   &IOSB,
                                   IOCTL_AFD_GET_PEER_NAME,
                                   NULL,
                                   0,
                                   SocketAddress,
                                   TdiAddressSize);

    /* Wait for return */
    if (Status == STATUS_PENDING)
    {
        WaitForSingleObject(SockEvent, INFINITE);
        Status = IOSB.Status;
    }

    NtClose( SockEvent );

    if (NT_SUCCESS(Status))
    {
        if (*NameLength >= Socket->SharedData.SizeOfRemoteAddress)
        {
            Name->sa_family = SocketAddress->Address[0].AddressType;
            RtlCopyMemory (Name->sa_data,
                           SocketAddress->Address[0].Address,
                           SocketAddress->Address[0].AddressLength);
            *NameLength = Socket->SharedData.SizeOfRemoteAddress;
            TRACE("NameLength %d Address: %x Port %x\n",
                          *NameLength, ((struct sockaddr_in *)Name)->sin_addr.s_addr,
                          ((struct sockaddr_in *)Name)->sin_port);
            HeapFree(GlobalHeap, 0, SocketAddress);
            return 0;
        }
        else
        {
            HeapFree(GlobalHeap, 0, SocketAddress);
            *lpErrno = WSAEFAULT;
            return SOCKET_ERROR;
        }
    }

    HeapFree(GlobalHeap, 0, SocketAddress);

    return MsafdReturnWithErrno ( Status, lpErrno, 0, NULL );
}

INT
WSPAPI
WSPIoctl(IN  SOCKET Handle,
         IN  DWORD dwIoControlCode,
         IN  LPVOID lpvInBuffer,
         IN  DWORD cbInBuffer,
         OUT LPVOID lpvOutBuffer,
         IN  DWORD cbOutBuffer,
         OUT LPDWORD lpcbBytesReturned,
         IN  LPWSAOVERLAPPED lpOverlapped,
         IN  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
         IN  LPWSATHREADID lpThreadId,
         OUT LPINT lpErrno)
{
    PSOCKET_INFORMATION Socket = NULL;
	BOOLEAN NeedsCompletion;
    BOOLEAN NonBlocking;

    if (!lpcbBytesReturned)
    {
       *lpErrno = WSAEFAULT;
       return SOCKET_ERROR;
    }

    /* Get the Socket Structure associate to this Socket*/
    Socket = GetSocketStructure(Handle);
    if (!Socket)
    {
       *lpErrno = WSAENOTSOCK;
       return SOCKET_ERROR;
    }

	*lpcbBytesReturned = 0;

    switch( dwIoControlCode )
    {
        case FIONBIO:
            if( cbInBuffer < sizeof(INT) || IS_INTRESOURCE(lpvInBuffer) )
            {
                *lpErrno = WSAEFAULT;
                return SOCKET_ERROR;
            }
            NonBlocking = *((PULONG)lpvInBuffer) ? TRUE : FALSE;
            Socket->SharedData.NonBlocking = NonBlocking ? 1 : 0;
            *lpErrno = SetSocketInformation(Socket, AFD_INFO_BLOCKING_MODE, &NonBlocking, NULL, NULL);
			if (*lpErrno != NO_ERROR)
				return SOCKET_ERROR;
			else
				return NO_ERROR;
        case FIONREAD:
            if( cbOutBuffer < sizeof(INT) || IS_INTRESOURCE(lpvOutBuffer) )
            {
                *lpErrno = WSAEFAULT;
                return SOCKET_ERROR;
            }
            *lpErrno = GetSocketInformation(Socket, AFD_INFO_RECEIVE_CONTENT_SIZE, NULL, (PULONG)lpvOutBuffer, NULL);
			if (*lpErrno != NO_ERROR)
				return SOCKET_ERROR;
			else
			{
				*lpcbBytesReturned = sizeof(ULONG);
				return NO_ERROR;
			}
        case SIOCATMARK:
            if (cbOutBuffer < sizeof(BOOL) || IS_INTRESOURCE(lpvOutBuffer))
            {
                *lpErrno = WSAEFAULT;
                return SOCKET_ERROR;
            }

            /* FIXME: Return false for now */
            *(BOOL*)lpvOutBuffer = FALSE;

            *lpcbBytesReturned = sizeof(BOOL);
            *lpErrno = NO_ERROR;
            return NO_ERROR;
        case SIO_GET_EXTENSION_FUNCTION_POINTER:
            *lpErrno = WSAEINVAL;
            return SOCKET_ERROR;
        default:
			*lpErrno = Socket->HelperData->WSHIoctl(Socket->HelperContext,
													Handle,
													Socket->TdiAddressHandle,
													Socket->TdiConnectionHandle,
													dwIoControlCode,
													lpvInBuffer,
													cbInBuffer,
													lpvOutBuffer,
													cbOutBuffer,
													lpcbBytesReturned,
													lpOverlapped,
													lpCompletionRoutine,
													(LPBOOL)&NeedsCompletion);

			if (*lpErrno != NO_ERROR)
				return SOCKET_ERROR;
			else
				return NO_ERROR;
    }
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
    PSOCKET_INFORMATION Socket = NULL;
    PVOID Buffer;
    INT BufferSize;
    BOOL BoolBuffer;
    INT IntBuffer;

    /* Get the Socket Structure associate to this Socket*/
    Socket = GetSocketStructure(Handle);
    if (Socket == NULL)
    {
        *lpErrno = WSAENOTSOCK;
        return SOCKET_ERROR;
    }

    TRACE("Called\n");

    switch (Level)
    {
        case SOL_SOCKET:
            switch (OptionName)
            {
                case SO_TYPE:
                    Buffer = &Socket->SharedData.SocketType;
                    BufferSize = sizeof(INT);
                    break;

                case SO_RCVBUF:
                    Buffer = &Socket->SharedData.SizeOfRecvBuffer;
                    BufferSize = sizeof(INT);
                    break;

                case SO_SNDBUF:
                    Buffer = &Socket->SharedData.SizeOfSendBuffer;
                    BufferSize = sizeof(INT);
                    break;

                case SO_ACCEPTCONN:
                    BoolBuffer = Socket->SharedData.Listening;
                    Buffer = &BoolBuffer;
                    BufferSize = sizeof(BOOL);
                    break;

                case SO_BROADCAST:
                    BoolBuffer = Socket->SharedData.Broadcast;
                    Buffer = &BoolBuffer;
                    BufferSize = sizeof(BOOL);
                    break;

                case SO_DEBUG:
                    BoolBuffer = Socket->SharedData.Debug;
                    Buffer = &BoolBuffer;
                    BufferSize = sizeof(BOOL);
                    break;

                case SO_DONTLINGER:
                    BoolBuffer = (Socket->SharedData.LingerData.l_onoff == 0);
                    Buffer = &BoolBuffer;
                    BufferSize = sizeof(BOOL);
                    break;

                case SO_LINGER:
                    if (Socket->SharedData.SocketType == SOCK_DGRAM)
                    {
                        *lpErrno = WSAENOPROTOOPT;
                        return SOCKET_ERROR;
                    }
                    Buffer = &Socket->SharedData.LingerData;
                    BufferSize = sizeof(struct linger);
                    break;

                case SO_OOBINLINE:
                    BoolBuffer = (Socket->SharedData.OobInline != 0);
                    Buffer = &BoolBuffer;
                    BufferSize = sizeof(BOOL);
                    break;

                case SO_KEEPALIVE:
                case SO_DONTROUTE:
                   /* These guys go directly to the helper */
                   goto SendToHelper;

                case SO_CONDITIONAL_ACCEPT:
                    BoolBuffer = (Socket->SharedData.UseDelayedAcceptance != 0);
                    Buffer = &BoolBuffer;
                    BufferSize = sizeof(BOOL);
                    break;

                case SO_REUSEADDR:
                    BoolBuffer = (Socket->SharedData.ReuseAddresses != 0);
                    Buffer = &BoolBuffer;
                    BufferSize = sizeof(BOOL);
                    break;

                case SO_EXCLUSIVEADDRUSE:
                    BoolBuffer = (Socket->SharedData.ExclusiveAddressUse != 0);
                    Buffer = &BoolBuffer;
                    BufferSize = sizeof(BOOL);
                    break;

                case SO_ERROR:
                    /* HACK: This needs to be properly tracked */
                    IntBuffer = 0;
                    DbgPrint("MSAFD: Hacked SO_ERROR returning error %d\n", IntBuffer);

                    Buffer = &IntBuffer;
                    BufferSize = sizeof(INT);
                    break;
                case SO_SNDTIMEO:
                    Buffer = &Socket->SharedData.SendTimeout;
                    BufferSize = sizeof(DWORD);
                    break;
                case SO_RCVTIMEO:
                    Buffer = &Socket->SharedData.RecvTimeout;
                    BufferSize = sizeof(DWORD);
                    break;
                case SO_PROTOCOL_INFOW:
                    Buffer = &Socket->ProtocolInfo;
                    BufferSize = sizeof(Socket->ProtocolInfo);
                    break;

                case SO_GROUP_ID:
                case SO_GROUP_PRIORITY:
                case SO_MAX_MSG_SIZE:

                default:
                    DbgPrint("MSAFD: Get unknown optname %x\n", OptionName);
                    *lpErrno = WSAENOPROTOOPT;
                    return SOCKET_ERROR;
            }

            if (*OptionLength < BufferSize)
            {
                *lpErrno = WSAEFAULT;
                *OptionLength = BufferSize;
                return SOCKET_ERROR;
            }
            RtlCopyMemory(OptionValue, Buffer, BufferSize);

            return 0;

        default:
            break;
    }

SendToHelper:
    *lpErrno = Socket->HelperData->WSHGetSocketInformation(Socket->HelperContext,
                                                           Handle,
                                                           Socket->TdiAddressHandle,
                                                           Socket->TdiConnectionHandle,
                                                           Level,
                                                           OptionName,
                                                           OptionValue,
                                                           (LPINT)OptionLength);
    return (*lpErrno == 0) ? 0 : SOCKET_ERROR;
}

INT
WSPAPI
WSPSetSockOpt(
    IN  SOCKET s,
    IN  INT level,
    IN  INT optname,
    IN  CONST CHAR FAR* optval,
    IN  INT optlen,
    OUT LPINT lpErrno)
{
    PSOCKET_INFORMATION Socket;

    /* Get the Socket Structure associate to this Socket*/
    Socket = GetSocketStructure(s);
    if (Socket == NULL)
    {
        *lpErrno = WSAENOTSOCK;
        return SOCKET_ERROR;
    }


    /* FIXME: We should handle some more cases here */
    if (level == SOL_SOCKET)
    {
        switch (optname)
        {
           case SO_BROADCAST:
              if (optlen < sizeof(BOOL))
              {
                  *lpErrno = WSAEFAULT;
                  return SOCKET_ERROR;
              }
              Socket->SharedData.Broadcast = (*optval != 0) ? 1 : 0;
              return 0;

           case SO_DONTLINGER:
              if (optlen < sizeof(BOOL))
              {
                  *lpErrno = WSAEFAULT;
                  return SOCKET_ERROR;
              }
              Socket->SharedData.LingerData.l_onoff = (*optval != 0) ? 0 : 1;
              return 0;

           case SO_REUSEADDR:
              if (optlen < sizeof(BOOL))
              {
                  *lpErrno = WSAEFAULT;
                  return SOCKET_ERROR;
              }
              Socket->SharedData.ReuseAddresses = (*optval != 0) ? 1 : 0;
              return 0;

           case SO_EXCLUSIVEADDRUSE:
              if (optlen < sizeof(BOOL))
              {
                  *lpErrno = WSAEFAULT;
                  return SOCKET_ERROR;
              }
              Socket->SharedData.ExclusiveAddressUse = (*optval != 0) ? 1 : 0;
              return 0;

           case SO_LINGER:
              if (optlen < sizeof(struct linger))
              {
                  *lpErrno = WSAEFAULT;
                  return SOCKET_ERROR;
              }
              RtlCopyMemory(&Socket->SharedData.LingerData,
                            optval,
                            sizeof(struct linger));
              return 0;

           case SO_SNDBUF:
              if (optlen < sizeof(DWORD))
              {
                  *lpErrno = WSAEFAULT;
                  return SOCKET_ERROR;
              }

              /* TODO: The total per-socket buffer space reserved for sends */
              ERR("Setting send buf to %x is not implemented yet\n", optval);
              return 0;

           case SO_SNDTIMEO:
              if (optlen < sizeof(DWORD))
              {
                  *lpErrno = WSAEFAULT;
                  return SOCKET_ERROR;
              }

              RtlCopyMemory(&Socket->SharedData.SendTimeout,
                            optval,
                            sizeof(DWORD));
              return 0;

           case SO_RCVTIMEO:
              if (optlen < sizeof(DWORD))
              {
                  *lpErrno = WSAEFAULT;
                  return SOCKET_ERROR;
              }

              RtlCopyMemory(&Socket->SharedData.RecvTimeout,
                            optval,
                            sizeof(DWORD));
              return 0;

           case SO_KEEPALIVE:
           case SO_DONTROUTE:
              /* These go directly to the helper dll */
              goto SendToHelper;

           default:
              /* Obviously this is a hack */
              ERR("MSAFD: Set unknown optname %x\n", optname);
              return 0;
        }
    }

SendToHelper:
    *lpErrno = Socket->HelperData->WSHSetSocketInformation(Socket->HelperContext,
                                                           s,
                                                           Socket->TdiAddressHandle,
                                                           Socket->TdiConnectionHandle,
                                                           level,
                                                           optname,
                                                           (PCHAR)optval,
                                                           optlen);
    return (*lpErrno == 0) ? 0 : SOCKET_ERROR;
}

/*
 * FUNCTION: Initialize service provider for a client
 * ARGUMENTS:
 *     wVersionRequested = Highest WinSock SPI version that the caller can use
 *     lpWSPData         = Address of WSPDATA structure to initialize
 *     lpProtocolInfo    = Pointer to structure that defines the desired protocol
 *     UpcallTable       = Pointer to upcall table of the WinSock DLL
 *     lpProcTable       = Address of procedure table to initialize
 * RETURNS:
 *     Status of operation
 */
INT
WSPAPI
WSPStartup(IN  WORD wVersionRequested,
           OUT LPWSPDATA lpWSPData,
           IN  LPWSAPROTOCOL_INFOW lpProtocolInfo,
           IN  WSPUPCALLTABLE UpcallTable,
           OUT LPWSPPROC_TABLE lpProcTable)

{
    NTSTATUS Status;

    ERR("wVersionRequested (0x%X) \n", wVersionRequested);
    Status = NO_ERROR;
    Upcalls = UpcallTable;

    if (Status == NO_ERROR)
    {
        lpProcTable->lpWSPAccept = WSPAccept;
        lpProcTable->lpWSPAddressToString = WSPAddressToString;
        lpProcTable->lpWSPAsyncSelect = WSPAsyncSelect;
        lpProcTable->lpWSPBind = WSPBind;
        lpProcTable->lpWSPCancelBlockingCall = WSPCancelBlockingCall;
        lpProcTable->lpWSPCleanup = WSPCleanup;
        lpProcTable->lpWSPCloseSocket = WSPCloseSocket;
        lpProcTable->lpWSPConnect = WSPConnect;
        lpProcTable->lpWSPDuplicateSocket = WSPDuplicateSocket;
        lpProcTable->lpWSPEnumNetworkEvents = WSPEnumNetworkEvents;
        lpProcTable->lpWSPEventSelect = WSPEventSelect;
        lpProcTable->lpWSPGetOverlappedResult = WSPGetOverlappedResult;
        lpProcTable->lpWSPGetPeerName = WSPGetPeerName;
        lpProcTable->lpWSPGetSockName = WSPGetSockName;
        lpProcTable->lpWSPGetSockOpt = WSPGetSockOpt;
        lpProcTable->lpWSPGetQOSByName = WSPGetQOSByName;
        lpProcTable->lpWSPIoctl = WSPIoctl;
        lpProcTable->lpWSPJoinLeaf = WSPJoinLeaf;
        lpProcTable->lpWSPListen = WSPListen;
        lpProcTable->lpWSPRecv = WSPRecv;
        lpProcTable->lpWSPRecvDisconnect = WSPRecvDisconnect;
        lpProcTable->lpWSPRecvFrom = WSPRecvFrom;
        lpProcTable->lpWSPSelect = WSPSelect;
        lpProcTable->lpWSPSend = WSPSend;
        lpProcTable->lpWSPSendDisconnect = WSPSendDisconnect;
        lpProcTable->lpWSPSendTo = WSPSendTo;
        lpProcTable->lpWSPSetSockOpt = WSPSetSockOpt;
        lpProcTable->lpWSPShutdown = WSPShutdown;
        lpProcTable->lpWSPSocket = WSPSocket;
        lpProcTable->lpWSPStringToAddress = WSPStringToAddress;
        lpWSPData->wVersion     = MAKEWORD(2, 2);
        lpWSPData->wHighVersion = MAKEWORD(2, 2);
    }

    TRACE("Status (%d).\n", Status);
    return Status;
}


/*
 * FUNCTION: Cleans up service provider for a client
 * ARGUMENTS:
 *     lpErrno = Address of buffer for error information
 * RETURNS:
 *     0 if successful, or SOCKET_ERROR if not
 */
INT
WSPAPI
WSPCleanup(OUT LPINT lpErrno)

{
    TRACE("Leaving.\n");
    *lpErrno = NO_ERROR;

    return 0;
}



int
GetSocketInformation(PSOCKET_INFORMATION Socket,
                     ULONG AfdInformationClass,
                     PBOOLEAN Boolean OPTIONAL,
                     PULONG Ulong OPTIONAL,
                     PLARGE_INTEGER LargeInteger OPTIONAL)
{
    IO_STATUS_BLOCK     IOSB;
    AFD_INFO            InfoData;
    NTSTATUS            Status;
    HANDLE              SockEvent;

    Status = NtCreateEvent(&SockEvent,
                           EVENT_ALL_ACCESS,
                           NULL,
                           1,
                           FALSE);

    if( !NT_SUCCESS(Status) )
        return -1;

    /* Set Info Class */
    InfoData.InformationClass = AfdInformationClass;

    /* Send IOCTL */
    Status = NtDeviceIoControlFile((HANDLE)Socket->Handle,
                                   SockEvent,
                                   NULL,
                                   NULL,
                                   &IOSB,
                                   IOCTL_AFD_GET_INFO,
                                   &InfoData,
                                   sizeof(InfoData),
                                   &InfoData,
                                   sizeof(InfoData));

    /* Wait for return */
    if (Status == STATUS_PENDING)
    {
        WaitForSingleObject(SockEvent, INFINITE);
        Status = IOSB.Status;
    }

    if (Status != STATUS_SUCCESS)
        return -1;

    /* Return Information */
    if (Ulong != NULL)
    {
        *Ulong = InfoData.Information.Ulong;
    }
    if (LargeInteger != NULL)
    {
        *LargeInteger = InfoData.Information.LargeInteger;
    }
    if (Boolean != NULL)
    {
        *Boolean = InfoData.Information.Boolean;
    }

    NtClose( SockEvent );

    return 0;

}


int
SetSocketInformation(PSOCKET_INFORMATION Socket,
                     ULONG AfdInformationClass,
                     PBOOLEAN Boolean OPTIONAL,
                     PULONG Ulong OPTIONAL,
                     PLARGE_INTEGER LargeInteger OPTIONAL)
{
    IO_STATUS_BLOCK     IOSB;
    AFD_INFO            InfoData;
    NTSTATUS            Status;
    HANDLE              SockEvent;

    Status = NtCreateEvent(&SockEvent,
                           EVENT_ALL_ACCESS,
                           NULL,
                           1,
                           FALSE);

    if( !NT_SUCCESS(Status) )
        return -1;

    /* Set Info Class */
    InfoData.InformationClass = AfdInformationClass;

    /* Set Information */
    if (Ulong != NULL)
    {
        InfoData.Information.Ulong = *Ulong;
    }
    if (LargeInteger != NULL)
    {
        InfoData.Information.LargeInteger = *LargeInteger;
    }
    if (Boolean != NULL)
    {
        InfoData.Information.Boolean = *Boolean;
    }

    /* Send IOCTL */
    Status = NtDeviceIoControlFile((HANDLE)Socket->Handle,
                                   SockEvent,
                                   NULL,
                                   NULL,
                                   &IOSB,
                                   IOCTL_AFD_SET_INFO,
                                   &InfoData,
                                   sizeof(InfoData),
                                   NULL,
                                   0);

    /* Wait for return */
    if (Status == STATUS_PENDING)
    {
        WaitForSingleObject(SockEvent, INFINITE);
        Status = IOSB.Status;
    }

    NtClose( SockEvent );

    return Status == STATUS_SUCCESS ? 0 : -1;

}

PSOCKET_INFORMATION
GetSocketStructure(SOCKET Handle)
{
    PSOCKET_INFORMATION CurrentSocket;

    EnterCriticalSection(&SocketListLock);

    CurrentSocket = SocketListHead;
    while (CurrentSocket)
    {
        if (CurrentSocket->Handle == Handle)
        {
            LeaveCriticalSection(&SocketListLock);
            return CurrentSocket;
        }

        CurrentSocket = CurrentSocket->NextSocket;
    }

    LeaveCriticalSection(&SocketListLock);

    return NULL;
}

int CreateContext(PSOCKET_INFORMATION Socket)
{
    IO_STATUS_BLOCK     IOSB;
    SOCKET_CONTEXT      ContextData;
    NTSTATUS            Status;
    HANDLE              SockEvent;

    Status = NtCreateEvent(&SockEvent,
                           EVENT_ALL_ACCESS,
                           NULL,
                           1,
                           FALSE);

    if( !NT_SUCCESS(Status) )
        return -1;

    /* Create Context */
    ContextData.SharedData = Socket->SharedData;
    ContextData.SizeOfHelperData = 0;
    RtlCopyMemory (&ContextData.LocalAddress,
                   Socket->LocalAddress,
                   Socket->SharedData.SizeOfLocalAddress);
    RtlCopyMemory (&ContextData.RemoteAddress,
                   Socket->RemoteAddress,
                   Socket->SharedData.SizeOfRemoteAddress);

    /* Send IOCTL */
    Status = NtDeviceIoControlFile((HANDLE)Socket->Handle,
                                   SockEvent,
                                   NULL,
                                   NULL,
                                   &IOSB,
                                   IOCTL_AFD_SET_CONTEXT,
                                   &ContextData,
                                   sizeof(ContextData),
                                   NULL,
                                   0);

    /* Wait for Completition */
    if (Status == STATUS_PENDING)
    {
        WaitForSingleObject(SockEvent, INFINITE);
        Status = IOSB.Status;
    }

    NtClose( SockEvent );

    return Status == STATUS_SUCCESS ? 0 : -1;
}

BOOLEAN SockCreateOrReferenceAsyncThread(VOID)
{
    HANDLE hAsyncThread;
    DWORD AsyncThreadId;
    HANDLE AsyncEvent;
    OBJECT_HANDLE_ATTRIBUTE_INFORMATION HandleFlags;
    NTSTATUS Status;

    /* Check if the Thread Already Exists */
    if (SockAsyncThreadRefCount)
    {
        ASSERT(SockAsyncCompletionPort);
        return TRUE;
    }

    /* Create the Completion Port */
    if (!SockAsyncCompletionPort)
    {
        Status = NtCreateIoCompletion(&SockAsyncCompletionPort,
                                      IO_COMPLETION_ALL_ACCESS,
                                      NULL,
                                      2); // Allow 2 threads only
        if (!NT_SUCCESS(Status))
        {
             ERR("Failed to create completion port: 0x%08x\n", Status);
             return FALSE;
        }
        /* Protect Handle */
        HandleFlags.ProtectFromClose = TRUE;
        HandleFlags.Inherit = FALSE;
        Status = NtSetInformationObject(SockAsyncCompletionPort,
                                        ObjectHandleFlagInformation,
                                        &HandleFlags,
                                        sizeof(HandleFlags));
    }

    /* Create the Async Event */
    Status = NtCreateEvent(&AsyncEvent,
                           EVENT_ALL_ACCESS,
                           NULL,
                           NotificationEvent,
                           FALSE);

    /* Create the Async Thread */
    hAsyncThread = CreateThread(NULL,
                                0,
                                (LPTHREAD_START_ROUTINE)SockAsyncThread,
                                NULL,
                                0,
                                &AsyncThreadId);

    /* Close the Handle */
    NtClose(hAsyncThread);

    /* Increase the Reference Count */
    SockAsyncThreadRefCount++;
    return TRUE;
}

int SockAsyncThread(PVOID ThreadParam)
{
    PVOID AsyncContext;
    PASYNC_COMPLETION_ROUTINE AsyncCompletionRoutine;
    IO_STATUS_BLOCK IOSB;
    NTSTATUS Status;

    /* Make the Thread Higher Priority */
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

    /* Do a KQUEUE/WorkItem Style Loop, thanks to IoCompletion Ports */
    do
    {
        Status =  NtRemoveIoCompletion (SockAsyncCompletionPort,
                                        (PVOID*)&AsyncCompletionRoutine,
                                        &AsyncContext,
                                        &IOSB,
                                        NULL);
        /* Call the Async Function */
        if (NT_SUCCESS(Status))
        {
            (*AsyncCompletionRoutine)(AsyncContext, &IOSB);
        }
        else
        {
            /* It Failed, sleep for a second */
            Sleep(1000);
        }
    } while ((Status != STATUS_TIMEOUT));

    /* The Thread has Ended */
    return 0;
}

BOOLEAN SockGetAsyncSelectHelperAfdHandle(VOID)
{
    UNICODE_STRING AfdHelper;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoSb;
    FILE_COMPLETION_INFORMATION CompletionInfo;
    OBJECT_HANDLE_ATTRIBUTE_INFORMATION HandleFlags;

    /* First, make sure we're not already intialized */
    if (SockAsyncHelperAfdHandle)
    {
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
    NtCreateFile(&SockAsyncHelperAfdHandle,
                 GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                 &ObjectAttributes,
                 &IoSb,
                 NULL,
                 0,
                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                 FILE_OPEN_IF,
                 0,
                 NULL,
                 0);

    /*
     * Now Set up the Completion Port Information
     * This means that whenever a Poll is finished, the routine will be executed
     */
    CompletionInfo.Port = SockAsyncCompletionPort;
    CompletionInfo.Key = SockAsyncSelectCompletionRoutine;
    NtSetInformationFile(SockAsyncHelperAfdHandle,
                         &IoSb,
                         &CompletionInfo,
                         sizeof(CompletionInfo),
                         FileCompletionInformation);


    /* Protect the Handle */
    HandleFlags.ProtectFromClose = TRUE;
    HandleFlags.Inherit = FALSE;
    NtSetInformationObject(SockAsyncCompletionPort,
                           ObjectHandleFlagInformation,
                           &HandleFlags,
                           sizeof(HandleFlags));


    /* Set this variable to true so that Send/Recv/Accept will know wether to renable disabled events */
    SockAsyncSelectCalled = TRUE;
    return TRUE;
}

VOID SockAsyncSelectCompletionRoutine(PVOID Context, PIO_STATUS_BLOCK IoStatusBlock)
{

    PASYNC_DATA AsyncData = Context;
    PSOCKET_INFORMATION Socket;
    ULONG x;

    /* Get the Socket */
    Socket = AsyncData->ParentSocket;

    /* Check if the Sequence  Number Changed behind our back */
    if (AsyncData->SequenceNumber != Socket->SharedData.SequenceNumber )
    {
        return;
    }

    /* Check we were manually called b/c of a failure */
    if (!NT_SUCCESS(IoStatusBlock->Status))
    {
        /* FIXME: Perform Upcall */
        return;
    }

    for (x = 1; x; x<<=1)
    {
        switch (AsyncData->AsyncSelectInfo.Handles[0].Events & x)
        {
            case AFD_EVENT_RECEIVE:
                if (0 != (Socket->SharedData.AsyncEvents & FD_READ) &&
                    0 == (Socket->SharedData.AsyncDisabledEvents & FD_READ))
                {
                    /* Make the Notifcation */
                    (Upcalls.lpWPUPostMessage)(Socket->SharedData.hWnd,
                                               Socket->SharedData.wMsg,
                                               Socket->Handle,
                                               WSAMAKESELECTREPLY(FD_READ, 0));
                    /* Disable this event until the next read(); */
                    Socket->SharedData.AsyncDisabledEvents |= FD_READ;
                }
            break;

            case AFD_EVENT_OOB_RECEIVE:
            if (0 != (Socket->SharedData.AsyncEvents & FD_OOB) &&
                0 == (Socket->SharedData.AsyncDisabledEvents & FD_OOB))
            {
                /* Make the Notifcation */
                (Upcalls.lpWPUPostMessage)(Socket->SharedData.hWnd,
                                           Socket->SharedData.wMsg,
                                           Socket->Handle,
                                           WSAMAKESELECTREPLY(FD_OOB, 0));
                /* Disable this event until the next read(); */
                Socket->SharedData.AsyncDisabledEvents |= FD_OOB;
            }
            break;

            case AFD_EVENT_SEND:
                if (0 != (Socket->SharedData.AsyncEvents & FD_WRITE) &&
                    0 == (Socket->SharedData.AsyncDisabledEvents & FD_WRITE))
                {
                    /* Make the Notifcation */
                    (Upcalls.lpWPUPostMessage)(Socket->SharedData.hWnd,
                                               Socket->SharedData.wMsg,
                                               Socket->Handle,
                                               WSAMAKESELECTREPLY(FD_WRITE, 0));
                    /* Disable this event until the next write(); */
                    Socket->SharedData.AsyncDisabledEvents |= FD_WRITE;
                }
                break;

                /* FIXME: THIS IS NOT RIGHT!!! HACK HACK HACK! */
            case AFD_EVENT_CONNECT:
            case AFD_EVENT_CONNECT_FAIL:
                if (0 != (Socket->SharedData.AsyncEvents & FD_CONNECT) &&
                    0 == (Socket->SharedData.AsyncDisabledEvents & FD_CONNECT))
                {
                    /* Make the Notifcation */
                    (Upcalls.lpWPUPostMessage)(Socket->SharedData.hWnd,
                        Socket->SharedData.wMsg,
                        Socket->Handle,
                        WSAMAKESELECTREPLY(FD_CONNECT, 0));
                    /* Disable this event forever; */
                    Socket->SharedData.AsyncDisabledEvents |= FD_CONNECT;
                }
                break;

            case AFD_EVENT_ACCEPT:
                if (0 != (Socket->SharedData.AsyncEvents & FD_ACCEPT) &&
                    0 == (Socket->SharedData.AsyncDisabledEvents & FD_ACCEPT))
                {
                    /* Make the Notifcation */
                    (Upcalls.lpWPUPostMessage)(Socket->SharedData.hWnd,
                                               Socket->SharedData.wMsg,
                                               Socket->Handle,
                                               WSAMAKESELECTREPLY(FD_ACCEPT, 0));
                    /* Disable this event until the next accept(); */
                    Socket->SharedData.AsyncDisabledEvents |= FD_ACCEPT;
                }
                break;

            case AFD_EVENT_DISCONNECT:
            case AFD_EVENT_ABORT:
            case AFD_EVENT_CLOSE:
                if (0 != (Socket->SharedData.AsyncEvents & FD_CLOSE) &&
                    0 == (Socket->SharedData.AsyncDisabledEvents & FD_CLOSE))
                {
                    /* Make the Notifcation */
                    (Upcalls.lpWPUPostMessage)(Socket->SharedData.hWnd,
                                               Socket->SharedData.wMsg,
                                               Socket->Handle,
                                               WSAMAKESELECTREPLY(FD_CLOSE, 0));
                    /* Disable this event forever; */
                    Socket->SharedData.AsyncDisabledEvents |= FD_CLOSE;
                }
                 break;
            /* FIXME: Support QOS */
        }
    }

    /* Check if there are any events left for us to check */
    if ((Socket->SharedData.AsyncEvents & (~Socket->SharedData.AsyncDisabledEvents)) == 0 )
    {
        return;
    }

    /* Keep Polling */
    SockProcessAsyncSelect(Socket, AsyncData);
    return;
}

VOID SockProcessAsyncSelect(PSOCKET_INFORMATION Socket, PASYNC_DATA AsyncData)
{

    ULONG lNetworkEvents;
    NTSTATUS Status;

    /* Set up the Async Data Event Info */
    AsyncData->AsyncSelectInfo.Timeout.HighPart = 0x7FFFFFFF;
    AsyncData->AsyncSelectInfo.Timeout.LowPart = 0xFFFFFFFF;
    AsyncData->AsyncSelectInfo.HandleCount = 1;
    AsyncData->AsyncSelectInfo.Exclusive = TRUE;
    AsyncData->AsyncSelectInfo.Handles[0].Handle = Socket->Handle;
    AsyncData->AsyncSelectInfo.Handles[0].Events = 0;

    /* Remove unwanted events */
    lNetworkEvents = Socket->SharedData.AsyncEvents & (~Socket->SharedData.AsyncDisabledEvents);

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

    /* FIXME: THIS IS NOT RIGHT!!! HACK HACK HACK! */
    if (lNetworkEvents & FD_CONNECT)
    {
        AsyncData->AsyncSelectInfo.Handles[0].Events |= AFD_EVENT_CONNECT | AFD_EVENT_CONNECT_FAIL;
    }

    if (lNetworkEvents & FD_CLOSE)
    {
        AsyncData->AsyncSelectInfo.Handles[0].Events |= AFD_EVENT_DISCONNECT | AFD_EVENT_ABORT | AFD_EVENT_CLOSE;
    }

    if (lNetworkEvents & FD_QOS)
    {
        AsyncData->AsyncSelectInfo.Handles[0].Events |= AFD_EVENT_QOS;
    }

    if (lNetworkEvents & FD_GROUP_QOS)
    {
        AsyncData->AsyncSelectInfo.Handles[0].Events |= AFD_EVENT_GROUP_QOS;
    }

    /* Send IOCTL */
    Status = NtDeviceIoControlFile (SockAsyncHelperAfdHandle,
                                    NULL,
                                    NULL,
                                    AsyncData,
                                    &AsyncData->IoStatusBlock,
                                    IOCTL_AFD_SELECT,
                                    &AsyncData->AsyncSelectInfo,
                                    sizeof(AsyncData->AsyncSelectInfo),
                                    &AsyncData->AsyncSelectInfo,
                                    sizeof(AsyncData->AsyncSelectInfo));

    /* I/O Manager Won't call the completion routine, let's do it manually */
    if (NT_SUCCESS(Status))
    {
        return;
    }
    else
    {
        AsyncData->IoStatusBlock.Status = Status;
        SockAsyncSelectCompletionRoutine(AsyncData, &AsyncData->IoStatusBlock);
    }
}

VOID SockProcessQueuedAsyncSelect(PVOID Context, PIO_STATUS_BLOCK IoStatusBlock)
{
    PASYNC_DATA AsyncData = Context;
    BOOL FreeContext = TRUE;
    PSOCKET_INFORMATION Socket;

    /* Get the Socket */
    Socket = AsyncData->ParentSocket;

    /* If someone closed it, stop the function */
    if (Socket->SharedData.State != SocketClosed)
    {
        /* Check if the Sequence Number changed by now, in which case quit */
        if (AsyncData->SequenceNumber == Socket->SharedData.SequenceNumber)
        {
            /* Do the actuall select, if needed */
            if ((Socket->SharedData.AsyncEvents & (~Socket->SharedData.AsyncDisabledEvents)))
            {
                SockProcessAsyncSelect(Socket, AsyncData);
                FreeContext = FALSE;
            }
        }
    }

    /* Free the Context */
    if (FreeContext)
    {
        HeapFree(GetProcessHeap(), 0, AsyncData);
    }

    return;
}

VOID
SockReenableAsyncSelectEvent (IN PSOCKET_INFORMATION Socket,
                              IN ULONG Event)
{
    PASYNC_DATA AsyncData;

    /* Make sure the event is actually disabled */
    if (!(Socket->SharedData.AsyncDisabledEvents & Event))
    {
        return;
    }

    /* Re-enable it */
    Socket->SharedData.AsyncDisabledEvents &= ~Event;

    /* Return if no more events are being polled */
    if ((Socket->SharedData.AsyncEvents & (~Socket->SharedData.AsyncDisabledEvents)) == 0 )
    {
        return;
    }

    /* Wait on new events */
    AsyncData = HeapAlloc(GetProcessHeap(), 0, sizeof(ASYNC_DATA));
    if (!AsyncData) return;

    /* Create the Asynch Thread if Needed */
    SockCreateOrReferenceAsyncThread();

    /* Increase the sequence number to stop anything else */
    Socket->SharedData.SequenceNumber++;

    /* Set up the Async Data */
    AsyncData->ParentSocket = Socket;
    AsyncData->SequenceNumber = Socket->SharedData.SequenceNumber;

    /* Begin Async Select by using I/O Completion */
    NtSetIoCompletion(SockAsyncCompletionPort,
                     (PVOID)&SockProcessQueuedAsyncSelect,
                     AsyncData,
                     0,
                     0);

    /* All done */
    return;
}

BOOL
WINAPI
DllMain(HANDLE hInstDll,
        ULONG dwReason,
        PVOID Reserved)
{

    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:

        TRACE("Loading MSAFD.DLL \n");

        /* Don't need thread attach notifications
        so disable them to improve performance */
        DisableThreadLibraryCalls(hInstDll);

        /* List of DLL Helpers */
        InitializeListHead(&SockHelpersListHead);

        /* Heap to use when allocating */
        GlobalHeap = GetProcessHeap();

        /* Initialize the lock that protects our socket list */
        InitializeCriticalSection(&SocketListLock);

        TRACE("MSAFD.DLL has been loaded\n");

        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:

        /* Delete the socket list lock */
        DeleteCriticalSection(&SocketListLock);

        break;
    }

    TRACE("DllMain of msafd.dll (leaving)\n");

    return TRUE;
}

/* EOF */


