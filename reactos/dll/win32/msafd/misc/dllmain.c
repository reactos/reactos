/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Ancillary Function Driver DLL
 * FILE:        misc/dllmain.c
 * PURPOSE:     DLL entry point
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Alex Ionescu (alex@relsoft.net)
 * REVISIONS:
 *              CSH 01/09-2000 Created
 *              Alex 16/07/2004 - Complete Rewrite
 */

#include <msafd.h>

#include <debug.h>

#ifdef DBG
//DWORD DebugTraceLevel = DEBUG_ULTRA;
DWORD DebugTraceLevel = 0;
#endif /* DBG */

HANDLE GlobalHeap;
WSPUPCALLTABLE Upcalls;
LPWPUCOMPLETEOVERLAPPEDREQUEST lpWPUCompleteOverlappedRequest;
ULONG SocketCount = 0;
PSOCKET_INFORMATION *Sockets = NULL;
LIST_ENTRY SockHelpersListHead = { NULL, NULL };
ULONG SockAsyncThreadRefCount;
HANDLE SockAsyncHelperAfdHandle;
HANDLE SockAsyncCompletionPort;
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
    PSOCKET_INFORMATION         Socket = NULL, PrevSocket = NULL;
    PFILE_FULL_EA_INFORMATION   EABuffer = NULL;
    PHELPER_DATA                HelperData;
    PVOID                       HelperDLLContext;
    DWORD                       HelperEvents;
    UNICODE_STRING              TransportName;
    UNICODE_STRING              DevName;
    LARGE_INTEGER               GroupData;
    INT                         Status;

    AFD_DbgPrint(MAX_TRACE, ("Creating Socket, getting TDI Name\n"));
    AFD_DbgPrint(MAX_TRACE, ("AddressFamily (%d)  SocketType (%d)  Protocol (%d).\n",
                             AddressFamily, SocketType, Protocol));

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
        AFD_DbgPrint(MID_TRACE,("SockGetTdiName: Status %x\n", Status));
        goto error;
    }

    /* AFD Device Name */
    RtlInitUnicodeString(&DevName, L"\\Device\\Afd\\Endpoint");

    /* Set Socket Data */
    Socket = HeapAlloc(GlobalHeap, 0, sizeof(*Socket));
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
    Socket->SharedData.CatalogEntryId = lpProtocolInfo->dwCatalogEntryId;
    Socket->SharedData.ServiceFlags1 = lpProtocolInfo->dwServiceFlags1;
    Socket->SharedData.ProviderFlags = lpProtocolInfo->dwProviderFlags;
    Socket->SharedData.GroupID = g;
    Socket->SharedData.GroupType = 0;
    Socket->SharedData.UseSAN = FALSE;
    Socket->SharedData.NonBlocking = FALSE; /* Sockets start blocking */
    Socket->SanData = NULL;

    /* Ask alex about this */
    if( Socket->SharedData.SocketType == SOCK_DGRAM ||
        Socket->SharedData.SocketType == SOCK_RAW )
    {
        AFD_DbgPrint(MID_TRACE,("Connectionless socket\n"));
        Socket->SharedData.ServiceFlags1 |= XP1_CONNECTIONLESS;
    }

    /* Packet Size */
    SizeOfPacket = TransportName.Length + sizeof(AFD_CREATE_PACKET) + sizeof(WCHAR);

    /* EA Size */
    SizeOfEA = SizeOfPacket + sizeof(FILE_FULL_EA_INFORMATION) + AFD_PACKET_COMMAND_LENGTH;

    /* Set up EA Buffer */
    EABuffer = HeapAlloc(GlobalHeap, 0, SizeOfEA);
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
    ZwCreateFile(&Sock,
                 GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                 &Object,
                 &IOSB,
                 NULL,
                 0,
                 FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_OPEN_IF,
                 0,
                 EABuffer,
                 SizeOfEA);

    /* Save Handle */
    Socket->Handle = (SOCKET)Sock;

    /* XXX See if there's a structure we can reuse -- We need to do this
    * more properly. */
    PrevSocket = GetSocketStructure( (SOCKET)Sock );

    if( PrevSocket )
    {
        RtlCopyMemory( PrevSocket, Socket, sizeof(*Socket) );
        RtlFreeHeap( GlobalHeap, 0, Socket );
        Socket = PrevSocket;
    }

    /* Save Group Info */
    if (g != 0)
    {
        GetSocketInformation(Socket, AFD_INFO_GROUP_ID_TYPE, 0, &GroupData);
        Socket->SharedData.GroupID = GroupData.u.LowPart;
        Socket->SharedData.GroupType = GroupData.u.HighPart;
    }

    /* Get Window Sizes and Save them */
    GetSocketInformation (Socket,
                          AFD_INFO_SEND_WINDOW_SIZE,
                          &Socket->SharedData.SizeOfSendBuffer,
                          NULL);

    GetSocketInformation (Socket,
                          AFD_INFO_RECEIVE_WINDOW_SIZE,
                          &Socket->SharedData.SizeOfRecvBuffer,
                          NULL);

    /* Save in Process Sockets List */
    Sockets[SocketCount] = Socket;
    SocketCount ++;

    /* Create the Socket Context */
    CreateContext(Socket);

    /* Notify Winsock */
    Upcalls.lpWPUModifyIFSHandle(1, (SOCKET)Sock, lpErrno);

    /* Return Socket Handle */
    AFD_DbgPrint(MID_TRACE,("Success %x\n", Sock));

    return (SOCKET)Sock;

error:
    AFD_DbgPrint(MID_TRACE,("Ending %x\n", Status));

    if( lpErrno )
        *lpErrno = Status;

    return INVALID_SOCKET;
}


DWORD MsafdReturnWithErrno(NTSTATUS Status,
                           LPINT Errno,
                           DWORD Received,
                           LPDWORD ReturnedBytes)
{
    if( ReturnedBytes )
        *ReturnedBytes = 0;
    if( Errno )
    {
        switch (Status)
        {
        case STATUS_CANT_WAIT: 
            *Errno = WSAEWOULDBLOCK;
            break;
        case STATUS_TIMEOUT:
            *Errno = WSAETIMEDOUT;
            break;
        case STATUS_SUCCESS: 
            /* Return Number of bytes Read */
            if( ReturnedBytes ) 
                *ReturnedBytes = Received;
            break;
        case STATUS_END_OF_FILE:
            *Errno = WSAESHUTDOWN;
            break;
        case STATUS_PENDING: 
            *Errno = WSA_IO_PENDING;
            break;
        case STATUS_BUFFER_TOO_SMALL:
        case STATUS_BUFFER_OVERFLOW:
            DbgPrint("MSAFD: STATUS_BUFFER_TOO_SMALL/STATUS_BUFFER_OVERFLOW\n");
            *Errno = WSAEMSGSIZE;
            break;
        case STATUS_NO_MEMORY: /* Fall through to STATUS_INSUFFICIENT_RESOURCES */
        case STATUS_INSUFFICIENT_RESOURCES:
            DbgPrint("MSAFD: STATUS_NO_MEMORY/STATUS_INSUFFICIENT_RESOURCES\n");
            *Errno = WSA_NOT_ENOUGH_MEMORY;
            break;
        case STATUS_INVALID_CONNECTION:
            DbgPrint("MSAFD: STATUS_INVALID_CONNECTION\n");
            *Errno = WSAEAFNOSUPPORT;
            break;
        case STATUS_REMOTE_NOT_LISTENING:
            DbgPrint("MSAFD: STATUS_REMOTE_NOT_LISTENING\n");
            *Errno = WSAECONNRESET;
            break;
        case STATUS_NETWORK_UNREACHABLE:
            DbgPrint("MSAFD: STATUS_NETWORK_UNREACHABLE\n");
            *Errno = WSAENETUNREACH;
            break;
        case STATUS_FILE_CLOSED:
            DbgPrint("MSAFD: STATUS_FILE_CLOSED\n");
            *Errno = WSAENOTSOCK;
            break;
        case STATUS_INVALID_PARAMETER:
            DbgPrint("MSAFD: STATUS_INVALID_PARAMETER\n");
            *Errno = WSAEINVAL;
            break;
		case STATUS_CANCELLED:
			DbgPrint("MSAFD: STATUS_CANCELLED\n");
			*Errno = WSAENOTSOCK;
			break;
        default:
            DbgPrint("MSAFD: Error %x is unknown\n", Status);
            *Errno = WSAEINVAL;
            break;
        }
    }

    /* Success */
    return Status == STATUS_SUCCESS ? 0 : SOCKET_ERROR;
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
    PSOCKET_INFORMATION Socket = NULL;
    NTSTATUS Status;
    HANDLE SockEvent;
    AFD_DISCONNECT_INFO DisconnectInfo;
    SOCKET_STATE OldState;

    /* Create the Wait Event */
    Status = NtCreateEvent(&SockEvent,
                           GENERIC_READ | GENERIC_WRITE,
                           NULL,
                           1,
                           FALSE);

    if(!NT_SUCCESS(Status))
        return SOCKET_ERROR;

    /* Get the Socket Structure associate to this Socket*/
    Socket = GetSocketStructure(Handle);

    /* If a Close is already in Process, give up */
    if (Socket->SharedData.State == SocketClosed)
    {
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
        ULONG LingerWait;
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
                                     &SendsInProgress,
                                     NULL))
            {
                /* Bail out if anything but NO_ERROR */
                LingerWait = 0;
                break;
            }

            /* Bail out if no more sends are pending */
            if (!SendsInProgress)
                break;
            /* 
             * We have to execute a sleep, so it's kind of like
             * a block. If the socket is Nonblock, we cannot
             * go on since asyncronous operation is expected
             * and we cannot offer it
             */
            if (Socket->SharedData.NonBlocking)
            {
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

        /*
        * We have reached the timeout or sends are over.
        * Disconnect if the timeout has been reached. 
        */
        if (LingerWait <= 0)
        {
            DisconnectInfo.Timeout = RtlConvertLongToLargeInteger(0);
            DisconnectInfo.DisconnectType = AFD_DISCONNECT_ABORT;

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
            }
        }
    }

    /* FIXME: We should notify the Helper DLL of WSH_NOTIFY_CLOSE */

    /* Cleanup Time! */
    Socket->HelperContext = NULL;
    Socket->SharedData.AsyncDisabledEvents = -1;
    NtClose(Socket->TdiAddressHandle);
    Socket->TdiAddressHandle = NULL;
    NtClose(Socket->TdiConnectionHandle);
    Socket->TdiConnectionHandle = NULL;

    /* Close the handle */
    NtClose((HANDLE)Handle);
    NtClose(SockEvent);

    return NO_ERROR;
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
    UCHAR                   BindBuffer[0x1A];
    SOCKADDR_INFO           SocketInfo;
    HANDLE                  SockEvent;

    Status = NtCreateEvent(&SockEvent,
                           GENERIC_READ | GENERIC_WRITE,
                           NULL,
                           1,
                           FALSE);

    if( !NT_SUCCESS(Status) )
        return -1;

    /* Get the Socket Structure associate to this Socket*/
    Socket = GetSocketStructure(Handle);

    /* Dynamic Structure...ugh */
    BindData = (PAFD_BIND_DATA)BindBuffer;

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

    /* Set up Socket Data */
    Socket->SharedData.State = SocketBound;
    Socket->TdiAddressHandle = (HANDLE)IOSB.Information;

    NtClose( SockEvent );

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

    if (Socket->SharedData.Listening)
        return 0;

    Status = NtCreateEvent(&SockEvent,
                           GENERIC_READ | GENERIC_WRITE,
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

    /* Set to Listening */
    Socket->SharedData.Listening = TRUE;

    NtClose( SockEvent );

    return MsafdReturnWithErrno ( Status, lpErrno, 0, NULL );
}


int
WSPAPI
WSPSelect(int nfds,
          fd_set *readfds,
          fd_set *writefds,
          fd_set *exceptfds,
          struct timeval *timeout,
          LPINT lpErrno)
{
    IO_STATUS_BLOCK     IOSB;
    PAFD_POLL_INFO      PollInfo;
    NTSTATUS            Status;
    LONG                HandleCount, OutCount = 0;
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

    if( HandleCount < 0 || nfds != 0 )
        HandleCount = nfds * 3;

    PollBufferSize = sizeof(*PollInfo) + (HandleCount * sizeof(AFD_HANDLE));

    AFD_DbgPrint(MID_TRACE,("HandleCount: %d BufferSize: %d\n", 
                 HandleCount, PollBufferSize));

    /* Convert Timeout to NT Format */
    if (timeout == NULL)
    {
        Timeout.u.LowPart = -1;
        Timeout.u.HighPart = 0x7FFFFFFF;
        AFD_DbgPrint(MAX_TRACE,("Infinite timeout\n"));
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
        AFD_DbgPrint(MAX_TRACE,("Timeout: Orig %d.%06d kernel %d\n",
                     timeout->tv_sec, timeout->tv_usec,
                     Timeout.u.LowPart));
    }

    Status = NtCreateEvent(&SockEvent,
                           GENERIC_READ | GENERIC_WRITE,
                           NULL,
                           1,
                           FALSE);

    if( !NT_SUCCESS(Status) )
        return SOCKET_ERROR;

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
    PollBufferSize = ((PCHAR)&PollInfo->Handles[j+1]) - ((PCHAR)PollInfo);

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

    AFD_DbgPrint(MID_TRACE,("DeviceIoControlFile => %x\n", Status));

    /* Wait for Completition */
    if (Status == STATUS_PENDING)
    {
        WaitForSingleObject(SockEvent, INFINITE);
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
                    AFD_DbgPrint(MID_TRACE,("Event %x on handle %x\n",
                                 PollInfo->Handles[i].Events,
                                 PollInfo->Handles[i].Handle));
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
                    AFD_DbgPrint(MID_TRACE,("Event %x on handle %x\n",
                                 PollInfo->Handles[i].Events,
                                 PollInfo->Handles[i].Handle));
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
                    AFD_DbgPrint(MID_TRACE,("Event %x on handle %x\n",
                                 PollInfo->Handles[i].Events,
                                 PollInfo->Handles[i].Handle));
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
        AFD_DbgPrint(MID_TRACE,("*lpErrno = %x\n", *lpErrno));
    }

    AFD_DbgPrint(MID_TRACE,("%d events\n", OutCount));

    return OutCount;
}

SOCKET
WSPAPI 
WSPAccept(SOCKET Handle,
          struct sockaddr *SocketAddress,
          int *SocketAddressLength,
          LPCONDITIONPROC lpfnCondition,
          DWORD_PTR dwCallbackData,
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
    WSAPROTOCOL_INFOW           ProtocolInfo;
    SOCKET                      AcceptSocket;
    UCHAR                       ReceiveBuffer[0x1A];
    HANDLE                      SockEvent;

    Status = NtCreateEvent(&SockEvent,
                           GENERIC_READ | GENERIC_WRITE,
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

    /* If this is non-blocking, make sure there's something for us to accept */
    FD_ZERO(&ReadSet);
    FD_SET(Socket->Handle, &ReadSet);
    Timeout.tv_sec=0;
    Timeout.tv_usec=0;

    WSPSelect(0, &ReadSet, NULL, NULL, &Timeout, NULL);

    if (ReadSet.fd_array[0] != Socket->Handle)
    {
        NtClose(SockEvent);
        return 0;
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
    ProtocolInfo.dwCatalogEntryId = Socket->SharedData.CatalogEntryId;
    ProtocolInfo.dwServiceFlags1 = Socket->SharedData.ServiceFlags1;
    ProtocolInfo.dwProviderFlags = Socket->SharedData.ProviderFlags;

    AcceptSocket = WSPSocket (Socket->SharedData.AddressFamily,
                              Socket->SharedData.SocketType,
                              Socket->SharedData.Protocol,
                              &ProtocolInfo,
                              GroupID,
                              Socket->SharedData.CreateFlags,
                              NULL);

    /* Set up the Accept Structure */
    AcceptData.ListenHandle = AcceptSocket;
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

    AFD_DbgPrint(MID_TRACE,("Socket %x\n", AcceptSocket));

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
    PAFD_CONNECT_INFO       ConnectInfo;
    PSOCKET_INFORMATION     Socket = NULL;
    NTSTATUS                Status;
    UCHAR                   ConnectBuffer[0x22];
    ULONG                   ConnectDataLength;
    ULONG                   InConnectDataLength;
    INT                     BindAddressLength;
    PSOCKADDR               BindAddress;
    HANDLE                  SockEvent;

    Status = NtCreateEvent(&SockEvent,
                           GENERIC_READ | GENERIC_WRITE,
                           NULL,
                           1,
                           FALSE);

    if( !NT_SUCCESS(Status) )
        return -1;

    AFD_DbgPrint(MID_TRACE,("Called\n"));

    /* Get the Socket Structure associate to this Socket*/
    Socket = GetSocketStructure(Handle);

    /* Bind us First */
    if (Socket->SharedData.State == SocketOpen)
    {
        /* Get the Wildcard Address */
        BindAddressLength = Socket->HelperData->MaxWSAddressLength;
        BindAddress = HeapAlloc(GetProcessHeap(), 0, BindAddressLength);
        Socket->HelperData->WSHGetWildcardSockaddr (Socket->HelperContext, 
                                                    BindAddress, 
                                                    &BindAddressLength);
        /* Bind it */
        WSPBind(Handle, BindAddress, BindAddressLength, NULL);
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
    }

    /* Dynamic Structure...ugh */
    ConnectInfo = (PAFD_CONNECT_INFO)ConnectBuffer;

    /* Set up Address in TDI Format */
    ConnectInfo->RemoteAddress.TAAddressCount = 1;
    ConnectInfo->RemoteAddress.Address[0].AddressLength = SocketAddressLength - sizeof(SocketAddress->sa_family);
    ConnectInfo->RemoteAddress.Address[0].AddressType = SocketAddress->sa_family;
    RtlCopyMemory (ConnectInfo->RemoteAddress.Address[0].Address,
                   SocketAddress->sa_data,
                   SocketAddressLength - sizeof(SocketAddress->sa_family));

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
    }

    /* AFD doesn't seem to care if these are invalid, but let's 0 them anyways */
    ConnectInfo->Root = 0;
    ConnectInfo->UseSAN = FALSE;
    ConnectInfo->Unknown = 0;

    /* FIXME: Handle Async Connect */
    if (Socket->SharedData.NonBlocking)
    {
        AFD_DbgPrint(MIN_TRACE, ("Async Connect UNIMPLEMENTED!\n"));
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

    /* Re-enable Async Event */
    SockReenableAsyncSelectEvent(Socket, FD_WRITE);

    /* FIXME: THIS IS NOT RIGHT!!! HACK HACK HACK! */
    SockReenableAsyncSelectEvent(Socket, FD_CONNECT);

    AFD_DbgPrint(MID_TRACE,("Ending\n"));

    NtClose( SockEvent );

    return MsafdReturnWithErrno( Status, lpErrno, 0, NULL );
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
                           GENERIC_READ | GENERIC_WRITE,
                           NULL,
                           1,
                           FALSE);

    if( !NT_SUCCESS(Status) )
        return -1;

    AFD_DbgPrint(MID_TRACE,("Called\n"));

    /* Get the Socket Structure associate to this Socket*/
    Socket = GetSocketStructure(Handle);

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

    DisconnectInfo.Timeout = RtlConvertLongToLargeInteger(-1);

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

    AFD_DbgPrint(MID_TRACE,("Ending\n"));

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
                           GENERIC_READ | GENERIC_WRITE,
                           NULL,
                           1,
                           FALSE);

    if( !NT_SUCCESS(Status) )
        return SOCKET_ERROR;

    /* Get the Socket Structure associate to this Socket*/
    Socket = GetSocketStructure(Handle);

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
        if (*NameLength >= SocketAddress->Address[0].AddressLength)
        {
            Name->sa_family = SocketAddress->Address[0].AddressType;
            RtlCopyMemory (Name->sa_data,
                           SocketAddress->Address[0].Address, 
                           SocketAddress->Address[0].AddressLength);
            *NameLength = 2 + SocketAddress->Address[0].AddressLength;
            AFD_DbgPrint (MID_TRACE, ("NameLength %d Address: %x Port %x\n",
                          *NameLength, ((struct sockaddr_in *)Name)->sin_addr.s_addr,
                          ((struct sockaddr_in *)Name)->sin_port));
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
                           GENERIC_READ | GENERIC_WRITE,
                           NULL,
                           1,
                           FALSE);

    if( !NT_SUCCESS(Status) )
        return SOCKET_ERROR;

    /* Get the Socket Structure associate to this Socket*/
    Socket = GetSocketStructure(s);

    /* Allocate a buffer for the address */
    TdiAddressSize = sizeof(TRANSPORT_ADDRESS) + *NameLength;
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
        if (*NameLength >= SocketAddress->Address[0].AddressLength)
        {
            Name->sa_family = SocketAddress->Address[0].AddressType;
            RtlCopyMemory (Name->sa_data,
                           SocketAddress->Address[0].Address, 
                           SocketAddress->Address[0].AddressLength);
            *NameLength = 2 + SocketAddress->Address[0].AddressLength;
            AFD_DbgPrint (MID_TRACE, ("NameLength %d Address: %s Port %x\n",
                          *NameLength, ((struct sockaddr_in *)Name)->sin_addr.s_addr,
                          ((struct sockaddr_in *)Name)->sin_port));
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

    /* Get the Socket Structure associate to this Socket*/
    Socket = GetSocketStructure(Handle);

    switch( dwIoControlCode )
    {
        case FIONBIO:
            if( cbInBuffer < sizeof(INT) )
                return SOCKET_ERROR;
            Socket->SharedData.NonBlocking = *((PINT)lpvInBuffer) ? 1 : 0;
            AFD_DbgPrint(MID_TRACE,("[%x] Set nonblocking %d\n", Handle, Socket->SharedData.NonBlocking));
            return 0;
        case FIONREAD:
            return GetSocketInformation(Socket, AFD_INFO_RECEIVE_CONTENT_SIZE, (PULONG)lpvOutBuffer, NULL);
        default:
            *lpErrno = WSAEINVAL;
            return SOCKET_ERROR;
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
    BOOLEAN BoolBuffer;

    /* Get the Socket Structure associate to this Socket*/
    Socket = GetSocketStructure(Handle);
    if (Socket == NULL)
    {
        *lpErrno = WSAENOTSOCK;
        return SOCKET_ERROR;
    }

    AFD_DbgPrint(MID_TRACE, ("Called\n"));

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
                    BufferSize = sizeof(BOOLEAN);
                    break;

                case SO_BROADCAST:
                    BoolBuffer = Socket->SharedData.Broadcast;
                    Buffer = &BoolBuffer;
                    BufferSize = sizeof(BOOLEAN);
                    break;

                case SO_DEBUG:
                    BoolBuffer = Socket->SharedData.Debug;
                    Buffer = &BoolBuffer;
                    BufferSize = sizeof(BOOLEAN);
                    break;

                /* case SO_CONDITIONAL_ACCEPT: */
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
                    AFD_DbgPrint(MID_TRACE, ("Unimplemented option (%x)\n",
                                 OptionName));

                default:
                    *lpErrno = WSAEINVAL;
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

        case IPPROTO_TCP: /* FIXME */
        default:
            *lpErrno = WSAEINVAL;
            return SOCKET_ERROR;
    }
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

    AFD_DbgPrint(MAX_TRACE, ("wVersionRequested (0x%X) \n", wVersionRequested));
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

    AFD_DbgPrint(MAX_TRACE, ("Status (%d).\n", Status));

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
    AFD_DbgPrint(MAX_TRACE, ("\n"));
    AFD_DbgPrint(MAX_TRACE, ("Leaving.\n"));
    *lpErrno = NO_ERROR;

    return 0;
}



int 
GetSocketInformation(PSOCKET_INFORMATION Socket, 
                     ULONG AfdInformationClass, 
                     PULONG Ulong OPTIONAL, 
                     PLARGE_INTEGER LargeInteger OPTIONAL)
{
    IO_STATUS_BLOCK     IOSB;
    AFD_INFO            InfoData;
    NTSTATUS            Status;
    HANDLE              SockEvent;

    Status = NtCreateEvent(&SockEvent,
                           GENERIC_READ | GENERIC_WRITE,
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
    }

    /* Return Information */
    *Ulong = InfoData.Information.Ulong;
    if (LargeInteger != NULL)
    {
        *LargeInteger = InfoData.Information.LargeInteger;
    }

    NtClose( SockEvent );

    return 0;

}


int 
SetSocketInformation(PSOCKET_INFORMATION Socket, 
                     ULONG AfdInformationClass, 
                     PULONG Ulong OPTIONAL, 
                     PLARGE_INTEGER LargeInteger OPTIONAL)
{
    IO_STATUS_BLOCK     IOSB;
    AFD_INFO            InfoData;
    NTSTATUS            Status;
    HANDLE              SockEvent;

    Status = NtCreateEvent(&SockEvent,
                           GENERIC_READ | GENERIC_WRITE,
                           NULL,
                           1,
                           FALSE);

    if( !NT_SUCCESS(Status) )
        return -1;

    /* Set Info Class */
    InfoData.InformationClass = AfdInformationClass;

    /* Set Information */
    InfoData.Information.Ulong = *Ulong;
    if (LargeInteger != NULL)
    {
        InfoData.Information.LargeInteger = *LargeInteger;
    }

    AFD_DbgPrint(MID_TRACE,("XXX Info %x (Data %x)\n",
        AfdInformationClass, *Ulong));

    /* Send IOCTL */
    Status = NtDeviceIoControlFile((HANDLE)Socket->Handle,
                                   SockEvent,
                                   NULL,
                                   NULL,
                                   &IOSB,
                                   IOCTL_AFD_GET_INFO,
                                   &InfoData,
                                   sizeof(InfoData),
                                   NULL,
                                   0);

    /* Wait for return */
    if (Status == STATUS_PENDING)
    {
        WaitForSingleObject(SockEvent, INFINITE);
    }

    NtClose( SockEvent );

    return 0;

}

PSOCKET_INFORMATION
GetSocketStructure(SOCKET Handle)
{
    ULONG i;

    for (i=0; i<SocketCount; i++) 
    {
        if (Sockets[i]->Handle == Handle)
        {
            return Sockets[i];
        }
    }
    return 0;
}

int CreateContext(PSOCKET_INFORMATION Socket)
{
    IO_STATUS_BLOCK     IOSB;
    SOCKET_CONTEXT      ContextData;
    NTSTATUS            Status;
    HANDLE              SockEvent;

    Status = NtCreateEvent(&SockEvent,
                           GENERIC_READ | GENERIC_WRITE,
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
    }

    NtClose( SockEvent );

    return 0;
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
        return TRUE;
    }

    /* Create the Completion Port */
    if (!SockAsyncCompletionPort)
    {
        Status = NtCreateIoCompletion(&SockAsyncCompletionPort,
                                      IO_COMPLETION_ALL_ACCESS,
                                      NULL,
                                      2); // Allow 2 threads only

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
    NTSTATUS Status;
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
    Status = NtCreateFile(&SockAsyncHelperAfdHandle,
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
    Status = NtSetInformationFile(SockAsyncHelperAfdHandle,
                                  &IoSb,
                                  &CompletionInfo,
                                  sizeof(CompletionInfo),
                                  FileCompletionInformation);


    /* Protect the Handle */
    HandleFlags.ProtectFromClose = TRUE;
    HandleFlags.Inherit = FALSE;
    Status = NtSetInformationObject(SockAsyncCompletionPort,
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
        AsyncData->AsyncSelectInfo.Handles[0].Events |= AFD_EVENT_DISCONNECT | AFD_EVENT_ABORT;
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

        AFD_DbgPrint(MAX_TRACE, ("Loading MSAFD.DLL \n"));

        /* Don't need thread attach notifications
        so disable them to improve performance */
        DisableThreadLibraryCalls(hInstDll);

        /* List of DLL Helpers */
        InitializeListHead(&SockHelpersListHead);

        /* Heap to use when allocating */
        GlobalHeap = GetProcessHeap();

        /* Allocate Heap for 1024 Sockets, can be expanded later */
        Sockets = HeapAlloc(GetProcessHeap(), 0, sizeof(PSOCKET_INFORMATION) * 1024);

        AFD_DbgPrint(MAX_TRACE, ("MSAFD.DLL has been loaded\n"));

        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:
        break;
    }

    AFD_DbgPrint(MAX_TRACE, ("DllMain of msafd.dll (leaving)\n"));

    return TRUE;
}

/* EOF */


