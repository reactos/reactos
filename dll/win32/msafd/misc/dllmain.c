/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Ancillary Function Driver DLL
 * FILE:        dll/win32/msafd/misc/dllmain.c
 * PURPOSE:     DLL entry point
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Alex Ionescu (alex@relsoft.net)
 *              Pierre Schweitzer (pierre@reactos.org)
 * REVISIONS:
 *              CSH 01/09-2000 Created
 *              Alex 16/07/2004 - Complete Rewrite
 */

#include <msafd.h>

#include <winuser.h>
#include <wchar.h>

HANDLE GlobalHeap;
WSPUPCALLTABLE Upcalls;
DWORD CatalogEntryId; /* CatalogEntryId for upcalls */
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
    PSOCK_SHARED_INFO           SharedData = NULL;

    TRACE("Creating Socket, getting TDI Name - AddressFamily (%d)  SocketType (%d)  Protocol (%d).\n",
        AddressFamily, SocketType, Protocol);

    if (lpProtocolInfo && lpProtocolInfo->dwServiceFlags3 != 0 && lpProtocolInfo->dwServiceFlags4 != 0)
    {
        /* Duplpicating socket from different process */
        if (UlongToPtr(lpProtocolInfo->dwServiceFlags3) == INVALID_HANDLE_VALUE)
        {
            Status = WSAEINVAL;
            goto error;
        }
        if (UlongToPtr(lpProtocolInfo->dwServiceFlags4) == INVALID_HANDLE_VALUE)
        {
            Status = WSAEINVAL;
            goto error;
        }
        SharedData = MapViewOfFile(UlongToPtr(lpProtocolInfo->dwServiceFlags3),
                                   FILE_MAP_ALL_ACCESS,
                                   0,
                                   0,
                                   sizeof(SOCK_SHARED_INFO));
        if (!SharedData)
        {
            Status = WSAEINVAL;
            goto error;
        }
        InterlockedIncrement(&SharedData->RefCount);
        AddressFamily = SharedData->AddressFamily;
        SocketType = SharedData->SocketType;
        Protocol = SharedData->Protocol;
    }

    if (AddressFamily == AF_UNSPEC && SocketType == 0 && Protocol == 0)
    {
        Status = WSAEINVAL;
        goto error;
    }

    /* Set the defaults */
    if (AddressFamily == AF_UNSPEC)
        AddressFamily = AF_INET;

    if (SocketType == 0)
    {
        switch (Protocol)
        {
        case IPPROTO_TCP:
            SocketType = SOCK_STREAM;
            break;
        case IPPROTO_UDP:
            SocketType = SOCK_DGRAM;
            break;
        case IPPROTO_RAW:
            SocketType = SOCK_RAW;
            break;
        default:
            TRACE("Unknown Protocol (%d). We will try SOCK_STREAM.\n", Protocol);
            SocketType = SOCK_STREAM;
            break;
        }
    }

    if (Protocol == 0)
    {
        switch (SocketType)
        {
        case SOCK_STREAM:
            Protocol = IPPROTO_TCP;
            break;
        case SOCK_DGRAM:
            Protocol = IPPROTO_UDP;
            break;
        case SOCK_RAW:
            Protocol = IPPROTO_RAW;
            break;
        default:
            TRACE("Unknown SocketType (%d). We will try IPPROTO_TCP.\n", SocketType);
            Protocol = IPPROTO_TCP;
            break;
        }
    }

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
    {
        Status = WSAENOBUFS;
        goto error;
    }
    RtlZeroMemory(Socket, sizeof(*Socket));
    if (SharedData)
    {
        Socket->SharedData = SharedData;
        Socket->SharedDataHandle = UlongToHandle(lpProtocolInfo->dwServiceFlags3);
        Sock = UlongToHandle(lpProtocolInfo->dwServiceFlags4);
        Socket->Handle = (SOCKET)lpProtocolInfo->dwServiceFlags4;
    }
    else
    {
        Socket->SharedDataHandle = INVALID_HANDLE_VALUE;
        Socket->SharedData = HeapAlloc(GlobalHeap, 0, sizeof(*Socket->SharedData));
        if (!Socket->SharedData)
        {
            Status = WSAENOBUFS;
            goto error;
        }
        RtlZeroMemory(Socket->SharedData, sizeof(*Socket->SharedData));
        Socket->SharedData->State = SocketOpen;
        Socket->SharedData->RefCount = 1L;
        Socket->SharedData->Listening = FALSE;
        Socket->SharedData->AddressFamily = AddressFamily;
        Socket->SharedData->SocketType = SocketType;
        Socket->SharedData->Protocol = Protocol;
        Socket->SharedData->SizeOfLocalAddress = HelperData->MaxWSAddressLength;
        Socket->SharedData->SizeOfRemoteAddress = HelperData->MaxWSAddressLength;
        Socket->SharedData->UseDelayedAcceptance = HelperData->UseDelayedAcceptance;
        Socket->SharedData->CreateFlags = dwFlags;
        Socket->SharedData->ServiceFlags1 = lpProtocolInfo->dwServiceFlags1;
        Socket->SharedData->ProviderFlags = lpProtocolInfo->dwProviderFlags;
        Socket->SharedData->UseSAN = FALSE;
        Socket->SharedData->NonBlocking = FALSE; /* Sockets start blocking */
        Socket->SharedData->RecvTimeout = INFINITE;
        Socket->SharedData->SendTimeout = INFINITE;
        Socket->SharedData->OobInline = FALSE;

        /* Ask alex about this */
        if( Socket->SharedData->SocketType == SOCK_DGRAM ||
            Socket->SharedData->SocketType == SOCK_RAW )
        {
            TRACE("Connectionless socket\n");
            Socket->SharedData->ServiceFlags1 |= XP1_CONNECTIONLESS;
        }
        Socket->Handle = INVALID_SOCKET;
    }

    Socket->HelperContext = HelperDLLContext;
    Socket->HelperData = HelperData;
    Socket->HelperEvents = HelperEvents;
    Socket->LocalAddress = &Socket->SharedData->WSLocalAddress;
    Socket->RemoteAddress = &Socket->SharedData->WSRemoteAddress;
    Socket->SanData = NULL;
    RtlCopyMemory(&Socket->ProtocolInfo, lpProtocolInfo, sizeof(Socket->ProtocolInfo));
    if (SharedData)
        goto ok;

    /* Packet Size */
    SizeOfPacket = TransportName.Length + sizeof(AFD_CREATE_PACKET) + sizeof(WCHAR);

    /* EA Size */
    SizeOfEA = SizeOfPacket + sizeof(FILE_FULL_EA_INFORMATION) + AFD_PACKET_COMMAND_LENGTH;

    /* Set up EA Buffer */
    EABuffer = HeapAlloc(GlobalHeap, 0, SizeOfEA);
    if (!EABuffer)
    {
        Status = WSAENOBUFS;
        goto error;
    }

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
    if ((Socket->SharedData->ServiceFlags1 & XP1_CONNECTIONLESS) != 0)
    {
        if ((SocketType != SOCK_DGRAM) && (SocketType != SOCK_RAW))
        {
            /* Only RAW or UDP can be Connectionless */
            Status = WSAEINVAL;
            goto error;
        }
        AfdPacket->EndpointFlags |= AFD_ENDPOINT_CONNECTIONLESS;
    }

    if ((Socket->SharedData->ServiceFlags1 & XP1_MESSAGE_ORIENTED) != 0)
    {
        if (SocketType == SOCK_STREAM)
        {
            if ((Socket->SharedData->ServiceFlags1 & XP1_PSEUDO_STREAM) == 0)
            {
                /* The Provider doesn't actually support Message Oriented Streams */
                Status = WSAEINVAL;
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
        if ((Socket->SharedData->ServiceFlags1 & XP1_SUPPORT_MULTIPOINT) == 0)
        {
            /* The Provider doesn't actually support Multipoint */
            Status = WSAEINVAL;
            goto error;
        }
        AfdPacket->EndpointFlags |= AFD_ENDPOINT_MULTIPOINT;

        if (dwFlags & WSA_FLAG_MULTIPOINT_C_ROOT)
        {
            if (((Socket->SharedData->ServiceFlags1 & XP1_MULTIPOINT_CONTROL_PLANE) == 0)
                || ((dwFlags & WSA_FLAG_MULTIPOINT_C_LEAF) != 0))
            {
                /* The Provider doesn't support Control Planes, or you already gave a leaf */
                Status = WSAEINVAL;
                goto error;
            }
            AfdPacket->EndpointFlags |= AFD_ENDPOINT_C_ROOT;
        }

        if (dwFlags & WSA_FLAG_MULTIPOINT_D_ROOT)
        {
            if (((Socket->SharedData->ServiceFlags1 & XP1_MULTIPOINT_DATA_PLANE) == 0)
                || ((dwFlags & WSA_FLAG_MULTIPOINT_D_LEAF) != 0))
            {
                /* The Provider doesn't support Data Planes, or you already gave a leaf */
                Status = WSAEINVAL;
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
        Status = TranslateNtStatusError(Status);
        goto error;
    }

    /* Save Handle */
    Socket->Handle = (SOCKET)Sock;

    /* Save Group Info */
    if (g != 0)
    {
        GetSocketInformation(Socket,
                             AFD_INFO_GROUP_ID_TYPE,
                             NULL,
                             NULL,
                             &GroupData,
                             NULL,
                             NULL);
        Socket->SharedData->GroupID = GroupData.u.LowPart;
        Socket->SharedData->GroupType = GroupData.u.HighPart;
    }

    /* Get Window Sizes and Save them */
    GetSocketInformation (Socket,
                          AFD_INFO_SEND_WINDOW_SIZE,
                          NULL,
                          &Socket->SharedData->SizeOfSendBuffer,
                          NULL,
                          NULL,
                          NULL);

    GetSocketInformation (Socket,
                          AFD_INFO_RECEIVE_WINDOW_SIZE,
                          NULL,
                          &Socket->SharedData->SizeOfRecvBuffer,
                          NULL,
                          NULL,
                          NULL);
ok:

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

    if( SharedData )
    {
        UnmapViewOfFile(SharedData);
        NtClose(UlongToHandle(lpProtocolInfo->dwServiceFlags3));
    }
    else
    {
        if( Socket && Socket->SharedData )
            HeapFree(GlobalHeap, 0, Socket->SharedData);
    }

    if( Socket )
        HeapFree(GlobalHeap, 0, Socket);

    if( EABuffer )
        HeapFree(GlobalHeap, 0, EABuffer);

    if( lpErrno )
        *lpErrno = Status;

    return INVALID_SOCKET;
}


INT
WSPAPI
WSPDuplicateSocket(
    IN  SOCKET Handle,
    IN  DWORD dwProcessId,
    OUT LPWSAPROTOCOL_INFOW lpProtocolInfo,
    OUT LPINT lpErrno)
{
    HANDLE hProcess, hDuplicatedSharedData, hDuplicatedHandle;
    PSOCKET_INFORMATION Socket;
    PSOCK_SHARED_INFO pSharedData, pOldSharedData;
    BOOL bDuplicated;

    if (Handle == INVALID_SOCKET)
        return MsafdReturnWithErrno(STATUS_INVALID_PARAMETER, lpErrno, 0, NULL);
    Socket = GetSocketStructure(Handle);
    if( !Socket )
    {
        if( lpErrno )
            *lpErrno = WSAENOTSOCK;
        return SOCKET_ERROR;
    }
    if ( !(hProcess = OpenProcess(PROCESS_DUP_HANDLE, FALSE, dwProcessId)) )
        return MsafdReturnWithErrno(STATUS_INVALID_PARAMETER, lpErrno, 0, NULL);

    /* It is a not yet duplicated socket, so map the memory, copy the SharedData and free heap */
    if( Socket->SharedDataHandle == INVALID_HANDLE_VALUE )
    {
        Socket->SharedDataHandle = CreateFileMapping(INVALID_HANDLE_VALUE,
                                                     NULL,
                                                     PAGE_READWRITE | SEC_COMMIT,
                                                     0,
                                                     (sizeof(SOCK_SHARED_INFO) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1),
                                                     NULL);
        if( Socket->SharedDataHandle == INVALID_HANDLE_VALUE )
            return MsafdReturnWithErrno(STATUS_INSUFFICIENT_RESOURCES, lpErrno, 0, NULL);
        pSharedData = MapViewOfFile(Socket->SharedDataHandle,
                                    FILE_MAP_ALL_ACCESS,
                                    0,
                                    0,
                                    sizeof(SOCK_SHARED_INFO));

        RtlCopyMemory(pSharedData, Socket->SharedData, sizeof(SOCK_SHARED_INFO));
        pOldSharedData = Socket->SharedData;
        Socket->SharedData = pSharedData;
        HeapFree(GlobalHeap, 0, pOldSharedData);
    }
    /* Duplicate the handles for the new process */
    bDuplicated = DuplicateHandle(GetCurrentProcess(),
                                  Socket->SharedDataHandle,
                                  hProcess,
                                  (LPHANDLE)&hDuplicatedSharedData,
                                  0,
                                  FALSE,
                                  DUPLICATE_SAME_ACCESS);
    if (!bDuplicated)
    {
        NtClose(hProcess);
        return MsafdReturnWithErrno(STATUS_ACCESS_DENIED, lpErrno, 0, NULL);
    }
    bDuplicated = DuplicateHandle(GetCurrentProcess(),
                                  (HANDLE)Socket->Handle,
                                  hProcess,
                                  (LPHANDLE)&hDuplicatedHandle,
                                  0,
                                  FALSE,
                                  DUPLICATE_SAME_ACCESS);
    NtClose(hProcess);
    if( !bDuplicated )
        return MsafdReturnWithErrno(STATUS_ACCESS_DENIED, lpErrno, 0, NULL);


    if (!lpProtocolInfo)
        return MsafdReturnWithErrno(STATUS_ACCESS_VIOLATION, lpErrno, 0, NULL);

    RtlCopyMemory(lpProtocolInfo, &Socket->ProtocolInfo, sizeof(*lpProtocolInfo));

    lpProtocolInfo->iAddressFamily = Socket->SharedData->AddressFamily;
    lpProtocolInfo->iProtocol = Socket->SharedData->Protocol;
    lpProtocolInfo->iSocketType = Socket->SharedData->SocketType;
    lpProtocolInfo->dwServiceFlags3 = HandleToUlong(hDuplicatedSharedData);
    lpProtocolInfo->dwServiceFlags4 = HandleToUlong(hDuplicatedHandle);

    if( lpErrno )
        *lpErrno = NO_ERROR;

    return NO_ERROR;
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
         return WSAECONNRESET;

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
       case STATUS_REMOTE_DISCONNECT:
          return WSAECONNREFUSED;

       case STATUS_NETWORK_UNREACHABLE:
          return WSAENETUNREACH;

       case STATUS_HOST_UNREACHABLE:
          return WSAEHOSTUNREACH;

       case STATUS_INVALID_PARAMETER:
          return WSAEINVAL;

       case STATUS_CANCELLED:
          return WSA_OPERATION_ABORTED;

       case STATUS_ADDRESS_ALREADY_EXISTS:
          return WSAEADDRINUSE;

       case STATUS_LOCAL_DISCONNECT:
          return WSAECONNABORTED;

       case STATUS_ACCESS_VIOLATION:
          return WSAEFAULT;

       case STATUS_ACCESS_DENIED:
          return WSAEACCES;

       case STATUS_NOT_IMPLEMENTED:
          return WSAEOPNOTSUPP;

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
    DWORD References;

    /* Get the Socket Structure associate to this Socket*/
    Socket = GetSocketStructure(Handle);
    if (!Socket)
    {
       if (lpErrno) *lpErrno = WSAENOTSOCK;
       return SOCKET_ERROR;
    }

    /* Create the Wait Event */
    Status = NtCreateEvent(&SockEvent,
                           EVENT_ALL_ACCESS,
                           NULL,
                           SynchronizationEvent,
                           FALSE);

    if(!NT_SUCCESS(Status))
    {
        ERR("NtCreateEvent failed: 0x%08x\n", Status);
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
            ERR("WSHNotify failed. Error 0x%#x\n", Status);
            NtClose(SockEvent);
            return SOCKET_ERROR;
        }
    }

    /* If a Close is already in Process, give up */
    if (Socket->SharedData->State == SocketClosed)
    {
        WARN("Socket is closing.\n");
        NtClose(SockEvent);
        if (lpErrno) *lpErrno = WSAENOTSOCK;
        return SOCKET_ERROR;
    }

    /* Decrement reference count on SharedData */
    References = InterlockedDecrement(&Socket->SharedData->RefCount);
    if (References)
        goto ok;

    /* Set the state to close */
    OldState = Socket->SharedData->State;
    Socket->SharedData->State = SocketClosed;

    /* If SO_LINGER is ON and the Socket is connected, we need to disconnect */
    /* FIXME: Should we do this on Datagram Sockets too? */
    if ((OldState == SocketConnected) && (Socket->SharedData->LingerData.l_onoff))
    {
        ULONG SendsInProgress;
        ULONG SleepWait;

        /* We need to respect the timeout */
        SleepWait = 100;
        LingerWait = Socket->SharedData->LingerData.l_linger * 1000;

        /* Loop until no more sends are pending, within the timeout */
        while (LingerWait)
        {
            /* Find out how many Sends are in Progress */
            if (GetSocketInformation(Socket,
                                     AFD_INFO_SENDS_IN_PROGRESS,
                                     NULL,
                                     &SendsInProgress,
                                     NULL,
                                     NULL,
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
             * go on since asynchronous operation is expected
             * and we cannot offer it
             */
            if (Socket->SharedData->NonBlocking)
            {
                WARN("Would block!\n");
                NtClose(SockEvent);
                Socket->SharedData->State = OldState;
                if (lpErrno) *lpErrno = WSAEWOULDBLOCK;
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

            if (((DisconnectInfo.DisconnectType & AFD_DISCONNECT_SEND) && (!Socket->SharedData->SendShutdown)) ||
                ((DisconnectInfo.DisconnectType & AFD_DISCONNECT_ABORT) && (!Socket->SharedData->ReceiveShutdown)))
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
    Socket->SharedData->AsyncDisabledEvents = -1;
    NtClose(Socket->TdiAddressHandle);
    Socket->TdiAddressHandle = NULL;
    NtClose(Socket->TdiConnectionHandle);
    Socket->TdiConnectionHandle = NULL;
ok:
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

    if( Socket->SharedDataHandle != INVALID_HANDLE_VALUE )
    {
        /* It is a duplicated socket, so unmap the memory */
        UnmapViewOfFile(Socket->SharedData);
        NtClose(Socket->SharedDataHandle);
        Socket->SharedData = NULL;
    }
    if( !References && Socket->SharedData )
    {
        HeapFree(GlobalHeap, 0, Socket->SharedData);
    }
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

    /* Get the Socket Structure associate to this Socket*/
    Socket = GetSocketStructure(Handle);
    if (!Socket)
    {
       if (lpErrno) *lpErrno = WSAENOTSOCK;
       return SOCKET_ERROR;
    }
    if (Socket->SharedData->State != SocketOpen)
    {
       if (lpErrno) *lpErrno = WSAEINVAL;
       return SOCKET_ERROR;
    }
    if (!SocketAddress || SocketAddressLength < Socket->SharedData->SizeOfLocalAddress)
    {
        if (lpErrno) *lpErrno = WSAEINVAL;
        return SOCKET_ERROR;
    }

    /* Get Address Information */
    Socket->HelperData->WSHGetSockaddrType ((PSOCKADDR)SocketAddress,
                                            SocketAddressLength,
                                            &SocketInfo);

    if (SocketInfo.AddressInfo == SockaddrAddressInfoBroadcast && !Socket->SharedData->Broadcast)
    {
       if (lpErrno) *lpErrno = WSAEADDRNOTAVAIL;
       return SOCKET_ERROR;
    }

    Status = NtCreateEvent(&SockEvent,
                           EVENT_ALL_ACCESS,
                           NULL,
                           SynchronizationEvent,
                           FALSE);

    if (!NT_SUCCESS(Status))
    {
        return SOCKET_ERROR;
    }

    /* See below */
    BindData = HeapAlloc(GlobalHeap, 0, 0xA + SocketAddressLength);
    if (!BindData)
    {
        return MsafdReturnWithErrno(STATUS_INSUFFICIENT_RESOURCES, lpErrno, 0, NULL);
    }

    /* Set up Address in TDI Format */
    BindData->Address.TAAddressCount = 1;
    BindData->Address.Address[0].AddressLength = (USHORT)(SocketAddressLength - sizeof(SocketAddress->sa_family));
    BindData->Address.Address[0].AddressType = SocketAddress->sa_family;
    RtlCopyMemory (BindData->Address.Address[0].Address,
                   SocketAddress->sa_data,
                   SocketAddressLength - sizeof(SocketAddress->sa_family));

    /* Set the Share Type */
    if (Socket->SharedData->ExclusiveAddressUse)
    {
        BindData->ShareType = AFD_SHARE_EXCLUSIVE;
    }
    else if (SocketInfo.EndpointInfo == SockaddrEndpointInfoWildcard)
    {
        BindData->ShareType = AFD_SHARE_WILDCARD;
    }
    else if (Socket->SharedData->ReuseAddresses)
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
                                   0xA + Socket->SharedData->SizeOfLocalAddress, /* Can't figure out a way to calculate this in C*/
                                   BindData,
                                   0xA + Socket->SharedData->SizeOfLocalAddress); /* Can't figure out a way to calculate this C */

    /* Wait for return */
    if (Status == STATUS_PENDING)
    {
        WaitForSingleObject(SockEvent, INFINITE);
        Status = IOSB.Status;
    }

    NtClose( SockEvent );
    HeapFree(GlobalHeap, 0, BindData);

    Socket->SharedData->SocketLastError = TranslateNtStatusError(Status);
    if (Status != STATUS_SUCCESS)
        return MsafdReturnWithErrno ( Status, lpErrno, 0, NULL );

    /* Set up Socket Data */
    Socket->SharedData->State = SocketBound;
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
       if (lpErrno) *lpErrno = WSAENOTSOCK;
       return SOCKET_ERROR;
    }

    if (Socket->SharedData->Listening)
        return NO_ERROR;

    Status = NtCreateEvent(&SockEvent,
                           EVENT_ALL_ACCESS,
                           NULL,
                           SynchronizationEvent,
                           FALSE);

    if( !NT_SUCCESS(Status) )
        return SOCKET_ERROR;

    /* Set Up Listen Structure */
    ListenData.UseSAN = FALSE;
    ListenData.UseDelayedAcceptance = Socket->SharedData->UseDelayedAcceptance;
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

    Socket->SharedData->SocketLastError = TranslateNtStatusError(Status);
    if (Status != STATUS_SUCCESS)
       return MsafdReturnWithErrno ( Status, lpErrno, 0, NULL );

    /* Set to Listening */
    Socket->SharedData->Listening = TRUE;

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
    ULONG               PollBufferSize;
    PVOID               PollBuffer;
    ULONG               i, j = 0, x;
    HANDLE              SockEvent;
    LARGE_INTEGER       Timeout;
    PSOCKET_INFORMATION Socket;
    SOCKET              Handle;
    ULONG               Events;
    fd_set              selectfds;

    /* Find out how many sockets we have, and how large the buffer needs
     * to be */
    FD_ZERO(&selectfds);
    if (readfds != NULL)
    {
        for (i = 0; i < readfds->fd_count; i++)
        {
            FD_SET(readfds->fd_array[i], &selectfds);
        }
    }
    if (writefds != NULL)
    {
        for (i = 0; i < writefds->fd_count; i++)
        {
            FD_SET(writefds->fd_array[i], &selectfds);
        }
    }
    if (exceptfds != NULL)
    {
        for (i = 0; i < exceptfds->fd_count; i++)
        {
            FD_SET(exceptfds->fd_array[i], &selectfds);
        }
    }

    HandleCount = selectfds.fd_count;

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
                           SynchronizationEvent,
                           FALSE);

    if(!NT_SUCCESS(Status))
    {
        if (lpErrno)
            *lpErrno = WSAEFAULT;

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

    for (i = 0; i < selectfds.fd_count; i++)
    {
        PollInfo->Handles[i].Handle = selectfds.fd_array[i];
    }
    if (readfds != NULL) {
        for (i = 0; i < readfds->fd_count; i++)
        {
            for (j = 0; j < HandleCount; j++)
            {
                if (PollInfo->Handles[j].Handle == readfds->fd_array[i])
                    break;
            }
            if (j >= HandleCount)
            {
                ERR("Error while counting readfds %ld > %ld\n", j, HandleCount);
                if (lpErrno) *lpErrno = WSAEFAULT;
                HeapFree(GlobalHeap, 0, PollBuffer);
                NtClose(SockEvent);
                return SOCKET_ERROR;
            }
            Socket = GetSocketStructure(readfds->fd_array[i]);
            if (!Socket)
            {
                ERR("Invalid socket handle provided in readfds %d\n", readfds->fd_array[i]);
                if (lpErrno) *lpErrno = WSAENOTSOCK;
                HeapFree(GlobalHeap, 0, PollBuffer);
                NtClose(SockEvent);
                return SOCKET_ERROR;
            }
            PollInfo->Handles[j].Events |= AFD_EVENT_RECEIVE |
                                           AFD_EVENT_DISCONNECT |
                                           AFD_EVENT_ABORT |
                                           AFD_EVENT_CLOSE |
                                           AFD_EVENT_ACCEPT;
            //if (Socket->SharedData->OobInline != 0)
            //    PollInfo->Handles[j].Events |= AFD_EVENT_OOB_RECEIVE;
        }
    }
    if (writefds != NULL)
    {
        for (i = 0; i < writefds->fd_count; i++)
        {
            for (j = 0; j < HandleCount; j++)
            {
                if (PollInfo->Handles[j].Handle == writefds->fd_array[i])
                    break;
            }
            if (j >= HandleCount)
            {
                ERR("Error while counting writefds %ld > %ld\n", j, HandleCount);
                if (lpErrno) *lpErrno = WSAEFAULT;
                HeapFree(GlobalHeap, 0, PollBuffer);
                NtClose(SockEvent);
                return SOCKET_ERROR;
            }
            Socket = GetSocketStructure(writefds->fd_array[i]);
            if (!Socket)
            {
                ERR("Invalid socket handle provided in writefds %d\n", writefds->fd_array[i]);
                if (lpErrno) *lpErrno = WSAENOTSOCK;
                HeapFree(GlobalHeap, 0, PollBuffer);
                NtClose(SockEvent);
                return SOCKET_ERROR;
            }
            PollInfo->Handles[j].Handle = writefds->fd_array[i];
            PollInfo->Handles[j].Events |= AFD_EVENT_SEND;
            if (Socket->SharedData->NonBlocking != 0)
                PollInfo->Handles[j].Events |= AFD_EVENT_CONNECT;
        }
    }
    if (exceptfds != NULL)
    {
        for (i = 0; i < exceptfds->fd_count; i++)
        {
            for (j = 0; j < HandleCount; j++)
            {
                if (PollInfo->Handles[j].Handle == exceptfds->fd_array[i])
                    break;
            }
            if (j > HandleCount)
            {
                ERR("Error while counting exceptfds %ld > %ld\n", j, HandleCount);
                if (lpErrno) *lpErrno = WSAEFAULT;
                HeapFree(GlobalHeap, 0, PollBuffer);
                NtClose(SockEvent);
                return SOCKET_ERROR;
            }
            Socket = GetSocketStructure(exceptfds->fd_array[i]);
            if (!Socket)
            {
                TRACE("Invalid socket handle provided in exceptfds %d\n", exceptfds->fd_array[i]);
                if (lpErrno) *lpErrno = WSAENOTSOCK;
                HeapFree(GlobalHeap, 0, PollBuffer);
                NtClose(SockEvent);
                return SOCKET_ERROR;
            }
            PollInfo->Handles[j].Handle = exceptfds->fd_array[i];
            if (Socket->SharedData->OobInline == 0)
                PollInfo->Handles[j].Events |= AFD_EVENT_OOB_RECEIVE;
            if (Socket->SharedData->NonBlocking != 0)
                PollInfo->Handles[j].Events |= AFD_EVENT_CONNECT_FAIL;
        }
    }

    PollInfo->HandleCount = HandleCount;
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

    /* Wait for Completion */
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
        Events = PollInfo->Handles[i].Events;
        Handle = PollInfo->Handles[i].Handle;
        for(x = 1; x; x<<=1)
        {
            Socket = GetSocketStructure(Handle);
            if (!Socket)
            {
                TRACE("Invalid socket handle found %d\n", Handle);
                if (lpErrno) *lpErrno = WSAENOTSOCK;
                HeapFree(GlobalHeap, 0, PollBuffer);
                NtClose(SockEvent);
                return SOCKET_ERROR;
            }
            switch (Events & x)
            {
                case AFD_EVENT_RECEIVE:
                case AFD_EVENT_DISCONNECT:
                case AFD_EVENT_ABORT:
                case AFD_EVENT_ACCEPT:
                case AFD_EVENT_CLOSE:
                    TRACE("Event %x on handle %x\n",
                        Events,
                        Handle);
                    if ((Events & x) == AFD_EVENT_DISCONNECT || (Events & x) == AFD_EVENT_CLOSE)
                        Socket->SharedData->SocketLastError = WSAECONNRESET;
                    if ((Events & x) == AFD_EVENT_ABORT)
                        Socket->SharedData->SocketLastError = WSAECONNABORTED;
                    if( readfds )
                        FD_SET(Handle, readfds);
                    break;
                case AFD_EVENT_SEND:
                    TRACE("Event %x on handle %x\n",
                        Events,
                        Handle);
                    if (writefds)
                        FD_SET(Handle, writefds);
                    break;
                case AFD_EVENT_CONNECT:
                    TRACE("Event %x on handle %x\n",
                        Events,
                        Handle);
                    if( writefds && Socket->SharedData->NonBlocking != 0 )
                        FD_SET(Handle, writefds);
                    break;
                case AFD_EVENT_OOB_RECEIVE:
                    TRACE("Event %x on handle %x\n",
                        Events,
                        Handle);
                    if( readfds && Socket->SharedData->OobInline != 0 )
                        FD_SET(Handle, readfds);
                    if( exceptfds && Socket->SharedData->OobInline == 0 )
                        FD_SET(Handle, exceptfds);
                    break;
                case AFD_EVENT_CONNECT_FAIL:
                    TRACE("Event %x on handle %x\n",
                        Events,
                        Handle);
                    if( exceptfds && Socket->SharedData->NonBlocking != 0 )
                        FD_SET(Handle, exceptfds);
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

    HandleCount = (readfds ? readfds->fd_count : 0) +
                  (writefds && writefds != readfds ? writefds->fd_count : 0) +
                  (exceptfds && exceptfds != readfds && exceptfds != writefds ? exceptfds->fd_count : 0);

    TRACE("%d events\n", HandleCount);

    return HandleCount;
}

DWORD
GetCurrentTimeInSeconds(VOID)
{
    SYSTEMTIME st1970 = { 1970, 1, 0, 1, 0, 0, 0, 0 };
    union
    {
        FILETIME ft;
        ULONGLONG ll;
    } u1970, Time;

    GetSystemTimeAsFileTime(&Time.ft);
    SystemTimeToFileTime(&st1970, &u1970.ft);
    return (DWORD)((Time.ll - u1970.ll) / 10000000ULL);
}

_Must_inspect_result_
SOCKET
WSPAPI
WSPAccept(
    _In_ SOCKET Handle,
    _Out_writes_bytes_to_opt_(*addrlen, *addrlen) struct sockaddr FAR *SocketAddress,
    _Inout_opt_ LPINT SocketAddressLength,
    _In_opt_ LPCONDITIONPROC lpfnCondition,
    _In_opt_ DWORD_PTR dwCallbackData,
    _Out_ LPINT lpErrno)
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

    /* Get the Socket Structure associate to this Socket*/
    Socket = GetSocketStructure(Handle);
    if (!Socket)
    {
       if (lpErrno) *lpErrno = WSAENOTSOCK;
       return SOCKET_ERROR;
    }
    if (!Socket->SharedData->Listening)
    {
       if (lpErrno) *lpErrno = WSAEINVAL;
       return SOCKET_ERROR;
    }
    if ((SocketAddress && !SocketAddressLength) ||
        (SocketAddressLength && !SocketAddress) ||
        (SocketAddressLength && *SocketAddressLength < sizeof(SOCKADDR)))
    {
       if (lpErrno) *lpErrno = WSAEFAULT;
       return INVALID_SOCKET;
    }

    Status = NtCreateEvent(&SockEvent,
                           EVENT_ALL_ACCESS,
                           NULL,
                           SynchronizationEvent,
                           FALSE);

    if( !NT_SUCCESS(Status) )
    {
        return SOCKET_ERROR;
    }

    /* Dynamic Structure...ugh */
    ListenReceiveData = (PAFD_RECEIVED_ACCEPT_DATA)ReceiveBuffer;

    /* If this is non-blocking, make sure there's something for us to accept */
    FD_ZERO(&ReadSet);
    FD_SET(Socket->Handle, &ReadSet);
    Timeout.tv_sec=0;
    Timeout.tv_usec=0;

    if (WSPSelect(0, &ReadSet, NULL, NULL, &Timeout, lpErrno) == SOCKET_ERROR)
    {
        NtClose(SockEvent);
        return SOCKET_ERROR;
    }

    if (ReadSet.fd_array[0] != Socket->Handle)
    {
        NtClose(SockEvent);
        if (lpErrno) *lpErrno = WSAEWOULDBLOCK;
        return SOCKET_ERROR;
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
        return MsafdReturnWithErrno( Status, lpErrno, 0, NULL );
    }

    if (lpfnCondition != NULL)
    {
        if ((Socket->SharedData->ServiceFlags1 & XP1_CONNECT_DATA) != 0)
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
                return MsafdReturnWithErrno( Status, lpErrno, 0, NULL );
            }

            /* How much data to allocate */
            PendingDataLength = IOSB.Information;

            if (PendingDataLength)
            {
                /* Allocate needed space */
                PendingData = HeapAlloc(GlobalHeap, 0, PendingDataLength);
                if (!PendingData)
                {
                    return MsafdReturnWithErrno( STATUS_INSUFFICIENT_RESOURCES, lpErrno, 0, NULL );
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
                    return MsafdReturnWithErrno( Status, lpErrno, 0, NULL );
                }
            }
        }

        if ((Socket->SharedData->ServiceFlags1 & XP1_QOS_SUPPORTED) != 0)
        {
            /* I don't support this yet */
        }

        /* Build Callee ID */
        CalleeID.buf = (PVOID)Socket->LocalAddress;
        CalleeID.len = Socket->SharedData->SizeOfLocalAddress;

        RemoteAddress = HeapAlloc(GlobalHeap, 0, sizeof(*RemoteAddress));
        if (!RemoteAddress)
        {
            return MsafdReturnWithErrno(STATUS_INSUFFICIENT_RESOURCES, lpErrno, 0, NULL);
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
        if (Socket->SharedData->UseDelayedAcceptance != 0)
        {
            /* Allocate Buffer for Callee Data */
            CalleeDataBuffer = HeapAlloc(GlobalHeap, 0, 4096);
            if (!CalleeDataBuffer) {
                return MsafdReturnWithErrno( STATUS_INSUFFICIENT_RESOURCES, lpErrno, 0, NULL );
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
            if ((Socket->SharedData->ServiceFlags1 & XP1_QOS_SUPPORTED) != 0)
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
                return MsafdReturnWithErrno( Status, lpErrno, 0, NULL );
            }

            if (CallBack == CF_REJECT )
            {
                if (lpErrno) *lpErrno = WSAECONNREFUSED;
                return SOCKET_ERROR;
            }
            else
            {
                if (lpErrno) *lpErrno = WSAECONNREFUSED;
                return SOCKET_ERROR;
            }
        }
    }

    /* Create a new Socket */
    AcceptSocket = WSPSocket (Socket->SharedData->AddressFamily,
                              Socket->SharedData->SocketType,
                              Socket->SharedData->Protocol,
                              &Socket->ProtocolInfo,
                              GroupID,
                              Socket->SharedData->CreateFlags,
                              lpErrno);
    if (AcceptSocket == INVALID_SOCKET)
        return SOCKET_ERROR;

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

    Socket->SharedData->SocketLastError = TranslateNtStatusError(Status);
    if (!NT_SUCCESS(Status))
    {
        NtClose(SockEvent);
        WSPCloseSocket( AcceptSocket, lpErrno );
        return MsafdReturnWithErrno( Status, lpErrno, 0, NULL );
    }

    AcceptSocketInfo = GetSocketStructure(AcceptSocket);
    if (!AcceptSocketInfo)
    {
        NtClose(SockEvent);
        WSPCloseSocket( AcceptSocket, lpErrno );
        return MsafdReturnWithErrno( STATUS_PROTOCOL_NOT_SUPPORTED, lpErrno, 0, NULL );
    }

    AcceptSocketInfo->SharedData->State = SocketConnected;
    AcceptSocketInfo->SharedData->ConnectTime = GetCurrentTimeInSeconds();

    /* Return Address in SOCKADDR FORMAT */
    if( SocketAddress )
    {
        RtlCopyMemory (SocketAddress,
                       &ListenReceiveData->Address.Address[0].AddressType,
                       sizeof(*RemoteAddress));
        if( SocketAddressLength )
            *SocketAddressLength = sizeof(*RemoteAddress);
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
            return SOCKET_ERROR;
        }
    }

    if (lpErrno) *lpErrno = NO_ERROR;

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

    TRACE("Called (%lx) %lx:%d\n", Handle, ((const struct sockaddr_in *)SocketAddress)->sin_addr, ((const struct sockaddr_in *)SocketAddress)->sin_port);

    /* Get the Socket Structure associate to this Socket*/
    Socket = GetSocketStructure(Handle);
    if (!Socket)
    {
        if (lpErrno) *lpErrno = WSAENOTSOCK;
        return SOCKET_ERROR;
    }

    Status = NtCreateEvent(&SockEvent,
                           EVENT_ALL_ACCESS,
                           NULL,
                           SynchronizationEvent,
                           FALSE);

    if (!NT_SUCCESS(Status))
        return SOCKET_ERROR;

    /* Bind us First */
    if (Socket->SharedData->State == SocketOpen)
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
    if (Socket->SharedData->AsyncEvents & FD_CONNECT)
    {
        Socket->SharedData->AsyncDisabledEvents |= FD_CONNECT | FD_WRITE;
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
    if (Socket->SharedData->NonBlocking)
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

    Socket->SharedData->SocketLastError = TranslateNtStatusError(Status);
    if (Status != STATUS_SUCCESS)
        goto notify;

    Socket->SharedData->State = SocketConnected;
    Socket->TdiConnectionHandle = (HANDLE)IOSB.Information;
    Socket->SharedData->ConnectTime = GetCurrentTimeInSeconds();

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

    TRACE("Ending %lx\n", IOSB.Status);

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

    TRACE("Called\n");

    /* Get the Socket Structure associate to this Socket*/
    Socket = GetSocketStructure(Handle);
    if (!Socket)
    {
       if (lpErrno) *lpErrno = WSAENOTSOCK;
       return SOCKET_ERROR;
    }

    Status = NtCreateEvent(&SockEvent,
                           EVENT_ALL_ACCESS,
                           NULL,
                           SynchronizationEvent,
                           FALSE);

    if( !NT_SUCCESS(Status) )
        return SOCKET_ERROR;

    /* Set AFD Disconnect Type */
    switch (HowTo)
    {
        case SD_RECEIVE:
            DisconnectInfo.DisconnectType = AFD_DISCONNECT_RECV;
            Socket->SharedData->ReceiveShutdown = TRUE;
            break;
        case SD_SEND:
            DisconnectInfo.DisconnectType= AFD_DISCONNECT_SEND;
            Socket->SharedData->SendShutdown = TRUE;
            break;
        case SD_BOTH:
            DisconnectInfo.DisconnectType = AFD_DISCONNECT_RECV | AFD_DISCONNECT_SEND;
            Socket->SharedData->ReceiveShutdown = TRUE;
            Socket->SharedData->SendShutdown = TRUE;
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

    Socket->SharedData->SocketLastError = TranslateNtStatusError(Status);
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

    /* Get the Socket Structure associate to this Socket*/
    Socket = GetSocketStructure(Handle);
    if (!Socket)
    {
       if (lpErrno) *lpErrno = WSAENOTSOCK;
       return SOCKET_ERROR;
    }

    if (!Name || !NameLength)
    {
        if (lpErrno) *lpErrno = WSAEFAULT;
        return SOCKET_ERROR;
    }

    Status = NtCreateEvent(&SockEvent,
                           EVENT_ALL_ACCESS,
                           NULL,
                           SynchronizationEvent,
                           FALSE);

    if( !NT_SUCCESS(Status) )
        return SOCKET_ERROR;

    /* Allocate a buffer for the address */
    TdiAddressSize =
        sizeof(TRANSPORT_ADDRESS) + Socket->SharedData->SizeOfLocalAddress;
    TdiAddress = HeapAlloc(GlobalHeap, 0, TdiAddressSize);

    if ( TdiAddress == NULL )
    {
        NtClose( SockEvent );
        if (lpErrno) *lpErrno = WSAENOBUFS;
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
        if (*NameLength >= Socket->SharedData->SizeOfLocalAddress)
        {
            Name->sa_family = SocketAddress->Address[0].AddressType;
            RtlCopyMemory (Name->sa_data,
                           SocketAddress->Address[0].Address,
                           SocketAddress->Address[0].AddressLength);
            *NameLength = Socket->SharedData->SizeOfLocalAddress;
            TRACE("NameLength %d Address: %x Port %x\n",
                          *NameLength, ((struct sockaddr_in *)Name)->sin_addr.s_addr,
                          ((struct sockaddr_in *)Name)->sin_port);
            HeapFree(GlobalHeap, 0, TdiAddress);
            return 0;
        }
        else
        {
            HeapFree(GlobalHeap, 0, TdiAddress);
            if (lpErrno) *lpErrno = WSAEFAULT;
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

    /* Get the Socket Structure associate to this Socket*/
    Socket = GetSocketStructure(s);
    if (!Socket)
    {
        if (lpErrno) *lpErrno = WSAENOTSOCK;
        return SOCKET_ERROR;
    }

    if (Socket->SharedData->State != SocketConnected)
    {
        if (lpErrno) *lpErrno = WSAENOTCONN;
        return SOCKET_ERROR;
    }

    if (!Name || !NameLength)
    {
        if (lpErrno) *lpErrno = WSAEFAULT;
        return SOCKET_ERROR;
    }

    Status = NtCreateEvent(&SockEvent,
                           EVENT_ALL_ACCESS,
                           NULL,
                           SynchronizationEvent,
                           FALSE);

    if( !NT_SUCCESS(Status) )
        return SOCKET_ERROR;

    /* Allocate a buffer for the address */
    TdiAddressSize = sizeof(TRANSPORT_ADDRESS) + Socket->SharedData->SizeOfRemoteAddress;
    SocketAddress = HeapAlloc(GlobalHeap, 0, TdiAddressSize);

    if ( SocketAddress == NULL )
    {
        NtClose( SockEvent );
        if (lpErrno) *lpErrno = WSAENOBUFS;
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
        if (*NameLength >= Socket->SharedData->SizeOfRemoteAddress)
        {
            Name->sa_family = SocketAddress->Address[0].AddressType;
            RtlCopyMemory (Name->sa_data,
                           SocketAddress->Address[0].Address,
                           SocketAddress->Address[0].AddressLength);
            *NameLength = Socket->SharedData->SizeOfRemoteAddress;
            TRACE("NameLength %d Address: %x Port %x\n",
                          *NameLength, ((struct sockaddr_in *)Name)->sin_addr.s_addr,
                          ((struct sockaddr_in *)Name)->sin_port);
            HeapFree(GlobalHeap, 0, SocketAddress);
            return 0;
        }
        else
        {
            HeapFree(GlobalHeap, 0, SocketAddress);
            if (lpErrno) *lpErrno = WSAEFAULT;
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
    BOOL NeedsCompletion = lpOverlapped != NULL;
    BOOLEAN NonBlocking;
    INT Errno = NO_ERROR, Ret = SOCKET_ERROR;
    DWORD cbRet = 0;

    /* Get the Socket Structure associate to this Socket*/
    Socket = GetSocketStructure(Handle);
    if (!Socket)
    {
        if(lpErrno)
            *lpErrno = WSAENOTSOCK;
        return SOCKET_ERROR;
    }

    if (!lpcbBytesReturned && !lpOverlapped)
    {
        if(lpErrno)
            *lpErrno = WSAEFAULT;
        return SOCKET_ERROR;
    }

    switch( dwIoControlCode )
    {
        case FIONBIO:
            if( cbInBuffer < sizeof(INT) || IS_INTRESOURCE(lpvInBuffer) )
            {
                Errno = WSAEFAULT;
                break;
            }
            NonBlocking = *((PULONG)lpvInBuffer) ? TRUE : FALSE;
            /* Don't allow to go in blocking mode if WSPAsyncSelect or WSPEventSelect is pending */
            if (!NonBlocking)
            {
                /* If there is an WSPAsyncSelect pending, fail with WSAEINVAL */
                if (Socket->SharedData->AsyncEvents & (~Socket->SharedData->AsyncDisabledEvents))
                {
                    Errno = WSAEINVAL;
                    break;
                }
                /* If there is an WSPEventSelect pending, fail with WSAEINVAL */
                if (Socket->NetworkEvents)
                {
                    Errno = WSAEINVAL;
                    break;
                }
            }
            Socket->SharedData->NonBlocking = NonBlocking ? 1 : 0;
            NeedsCompletion = FALSE;
            Errno = SetSocketInformation(Socket, AFD_INFO_BLOCKING_MODE, &NonBlocking, NULL, NULL, lpOverlapped, lpCompletionRoutine);
            if (Errno == NO_ERROR)
                Ret = NO_ERROR;
            break;
        case FIONREAD:
            if (IS_INTRESOURCE(lpvOutBuffer) || cbOutBuffer == 0)
            {
                cbRet = sizeof(ULONG);
                Errno = WSAEFAULT;
                break;
            }
            if (cbOutBuffer < sizeof(ULONG))
            {
                Errno = WSAEINVAL;
                break;
            }
            NeedsCompletion = FALSE;
            Errno = GetSocketInformation(Socket, AFD_INFO_RECEIVE_CONTENT_SIZE, NULL, (PULONG)lpvOutBuffer, NULL, lpOverlapped, lpCompletionRoutine);
            if (Errno == NO_ERROR)
            {
                cbRet = sizeof(ULONG);
                Ret = NO_ERROR;
            }
            break;
        case SIOCATMARK:
            if (IS_INTRESOURCE(lpvOutBuffer) || cbOutBuffer == 0)
            {
                cbRet = sizeof(BOOL);
                Errno = WSAEFAULT;
                break;
            }
            if (cbOutBuffer < sizeof(BOOL))
            {
                Errno = WSAEINVAL;
                break;
            }
            if (Socket->SharedData->SocketType != SOCK_STREAM)
            {
                Errno = WSAEINVAL;
                break;
            }

            /* FIXME: Return false if OOBINLINE is true for now
               We should MSG_PEEK|MSG_OOB check with driver
            */
            *(BOOL*)lpvOutBuffer = !Socket->SharedData->OobInline;

            cbRet = sizeof(BOOL);
            Errno = NO_ERROR;
            Ret = NO_ERROR;
            break;
        case SIO_GET_EXTENSION_FUNCTION_POINTER:
            if (cbOutBuffer == 0)
            {
                cbRet = sizeof(PVOID);
                Errno = WSAEFAULT;
                break;
            }

            if (cbInBuffer < sizeof(GUID) ||
                cbOutBuffer < sizeof(PVOID))
            {
                Errno = WSAEINVAL;
                break;
            }

            {
                GUID AcceptExGUID = WSAID_ACCEPTEX;
                GUID ConnectExGUID = WSAID_CONNECTEX;
                GUID DisconnectExGUID = WSAID_DISCONNECTEX;
                GUID GetAcceptExSockaddrsGUID = WSAID_GETACCEPTEXSOCKADDRS;

                if (IsEqualGUID(&AcceptExGUID, lpvInBuffer))
                {
                    *((PVOID *)lpvOutBuffer) = WSPAcceptEx;
                    cbRet = sizeof(PVOID);
                    Errno = NO_ERROR;
                    Ret = NO_ERROR;
                }
                else if (IsEqualGUID(&ConnectExGUID, lpvInBuffer))
                {
                    *((PVOID *)lpvOutBuffer) = WSPConnectEx;
                    cbRet = sizeof(PVOID);
                    Errno = NO_ERROR;
                    Ret = NO_ERROR;
                }
                else if (IsEqualGUID(&DisconnectExGUID, lpvInBuffer))
                {
                    *((PVOID *)lpvOutBuffer) = WSPDisconnectEx;
                    cbRet = sizeof(PVOID);
                    Errno = NO_ERROR;
                    Ret = NO_ERROR;
                }
                else if (IsEqualGUID(&GetAcceptExSockaddrsGUID, lpvInBuffer))
                {
                    *((PVOID *)lpvOutBuffer) = WSPGetAcceptExSockaddrs;
                    cbRet = sizeof(PVOID);
                    Errno = NO_ERROR;
                    Ret = NO_ERROR;
                }
                else
                {
                    ERR("Querying unknown extension function: %x\n", ((GUID*)lpvInBuffer)->Data1);
                    Errno = WSAEOPNOTSUPP;
                }
            }

            break;
        case SIO_ADDRESS_LIST_QUERY:
            if (IS_INTRESOURCE(lpvOutBuffer) || cbOutBuffer == 0)
            {
                cbRet = sizeof(SOCKET_ADDRESS_LIST) + sizeof(Socket->SharedData->WSLocalAddress);
                Errno = WSAEFAULT;
                break;
            }
            if (cbOutBuffer < sizeof(INT))
            {
                Errno = WSAEINVAL;
                break;
            }

            cbRet = sizeof(SOCKET_ADDRESS_LIST) + sizeof(Socket->SharedData->WSLocalAddress);

            ((SOCKET_ADDRESS_LIST*)lpvOutBuffer)->iAddressCount = 1;

            if (cbOutBuffer < (sizeof(SOCKET_ADDRESS_LIST) + sizeof(Socket->SharedData->WSLocalAddress)))
            {
                Errno = WSAEFAULT;
                break;
            }

            ((SOCKET_ADDRESS_LIST*)lpvOutBuffer)->Address[0].iSockaddrLength = sizeof(Socket->SharedData->WSLocalAddress);
            ((SOCKET_ADDRESS_LIST*)lpvOutBuffer)->Address[0].lpSockaddr = &Socket->SharedData->WSLocalAddress;

            Errno = NO_ERROR;
            Ret = NO_ERROR;
            break;
        default:
            Errno = Socket->HelperData->WSHIoctl(Socket->HelperContext,
                                                 Handle,
                                                 Socket->TdiAddressHandle,
                                                 Socket->TdiConnectionHandle,
                                                 dwIoControlCode,
                                                 lpvInBuffer,
                                                 cbInBuffer,
                                                 lpvOutBuffer,
                                                 cbOutBuffer,
                                                 &cbRet,
                                                 lpOverlapped,
                                                 lpCompletionRoutine,
                                                 &NeedsCompletion);

            if (Errno == NO_ERROR)
                Ret = NO_ERROR;
            break;
    }
    if (lpOverlapped && NeedsCompletion)
    {
        lpOverlapped->Internal = Errno;
        lpOverlapped->InternalHigh = cbRet;
        if (lpCompletionRoutine != NULL)
        {
            lpCompletionRoutine(Errno, cbRet, lpOverlapped, 0);
        }
        if (lpOverlapped->hEvent)
            SetEvent(lpOverlapped->hEvent);
        if (!PostQueuedCompletionStatus((HANDLE)Handle, cbRet, 0, lpOverlapped))
        {
            ERR("PostQueuedCompletionStatus failed %d\n", GetLastError());
        }
        return NO_ERROR;
    }
    if (lpErrno)
        *lpErrno = Errno;
    if (lpcbBytesReturned)
        *lpcbBytesReturned = cbRet;
    return Ret;
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
    DWORD DwordBuffer;
    INT Errno;

    TRACE("Called\n");

    /* Get the Socket Structure associate to this Socket*/
    Socket = GetSocketStructure(Handle);
    if (Socket == NULL)
    {
        if (lpErrno) *lpErrno = WSAENOTSOCK;
        return SOCKET_ERROR;
    }
    if (!OptionLength || !OptionValue)
    {
        if (lpErrno) *lpErrno = WSAEFAULT;
        return SOCKET_ERROR;
    }

    switch (Level)
    {
        case SOL_SOCKET:
            switch (OptionName)
            {
                case SO_TYPE:
                    Buffer = &Socket->SharedData->SocketType;
                    BufferSize = sizeof(INT);
                    break;

                case SO_RCVBUF:
                    Buffer = &Socket->SharedData->SizeOfRecvBuffer;
                    BufferSize = sizeof(ULONG);
                    break;

                case SO_SNDBUF:
                    Buffer = &Socket->SharedData->SizeOfSendBuffer;
                    BufferSize = sizeof(ULONG);
                    break;

                case SO_ACCEPTCONN:
                    BoolBuffer = Socket->SharedData->Listening;
                    Buffer = &BoolBuffer;
                    BufferSize = sizeof(BOOL);
                    break;

                case SO_BROADCAST:
                    BoolBuffer = Socket->SharedData->Broadcast;
                    Buffer = &BoolBuffer;
                    BufferSize = sizeof(BOOL);
                    break;

                case SO_DEBUG:
                    BoolBuffer = Socket->SharedData->Debug;
                    Buffer = &BoolBuffer;
                    BufferSize = sizeof(BOOL);
                    break;

                case SO_DONTLINGER:
                    BoolBuffer = (Socket->SharedData->LingerData.l_onoff == 0);
                    Buffer = &BoolBuffer;
                    BufferSize = sizeof(BOOL);
                    break;

                case SO_LINGER:
                    if (Socket->SharedData->SocketType == SOCK_DGRAM)
                    {
                        if (lpErrno) *lpErrno = WSAENOPROTOOPT;
                        return SOCKET_ERROR;
                    }
                    Buffer = &Socket->SharedData->LingerData;
                    BufferSize = sizeof(struct linger);
                    break;

                case SO_OOBINLINE:
                    BoolBuffer = (Socket->SharedData->OobInline != 0);
                    Buffer = &BoolBuffer;
                    BufferSize = sizeof(BOOL);
                    break;

                case SO_KEEPALIVE:
                case SO_DONTROUTE:
                   /* These guys go directly to the helper */
                   goto SendToHelper;

                case SO_CONDITIONAL_ACCEPT:
                    BoolBuffer = (Socket->SharedData->UseDelayedAcceptance != 0);
                    Buffer = &BoolBuffer;
                    BufferSize = sizeof(BOOL);
                    break;

                case SO_REUSEADDR:
                    BoolBuffer = (Socket->SharedData->ReuseAddresses != 0);
                    Buffer = &BoolBuffer;
                    BufferSize = sizeof(BOOL);
                    break;

                case SO_EXCLUSIVEADDRUSE:
                    BoolBuffer = (Socket->SharedData->ExclusiveAddressUse != 0);
                    Buffer = &BoolBuffer;
                    BufferSize = sizeof(BOOL);
                    break;

                case SO_ERROR:
                    Buffer = &Socket->SharedData->SocketLastError;
                    BufferSize = sizeof(INT);
                    break;

                case SO_CONNECT_TIME:
                    DwordBuffer = GetCurrentTimeInSeconds() - Socket->SharedData->ConnectTime;
                    Buffer = &DwordBuffer;
                    BufferSize = sizeof(DWORD);
                    break;

                case SO_SNDTIMEO:
                    Buffer = &Socket->SharedData->SendTimeout;
                    BufferSize = sizeof(DWORD);
                    break;
                case SO_RCVTIMEO:
                    Buffer = &Socket->SharedData->RecvTimeout;
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
                    if (lpErrno) *lpErrno = WSAENOPROTOOPT;
                    return SOCKET_ERROR;
            }

            if (*OptionLength < BufferSize)
            {
                if (lpErrno) *lpErrno = WSAEFAULT;
                *OptionLength = BufferSize;
                return SOCKET_ERROR;
            }
            RtlCopyMemory(OptionValue, Buffer, BufferSize);

            return 0;

        default:
            if (lpErrno) *lpErrno = WSAEINVAL;
            return SOCKET_ERROR;
    }

SendToHelper:
    Errno = Socket->HelperData->WSHGetSocketInformation(Socket->HelperContext,
                                                        Handle,
                                                        Socket->TdiAddressHandle,
                                                        Socket->TdiConnectionHandle,
                                                        Level,
                                                        OptionName,
                                                        OptionValue,
                                                        (LPINT)OptionLength);
    if (lpErrno) *lpErrno = Errno;
    return (Errno == NO_ERROR) ? NO_ERROR : SOCKET_ERROR;
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
    INT Errno;

    /* Get the Socket Structure associate to this Socket*/
    Socket = GetSocketStructure(s);
    if (Socket == NULL)
    {
        if (lpErrno) *lpErrno = WSAENOTSOCK;
        return SOCKET_ERROR;
    }
    if (!optval)
    {
        if (lpErrno) *lpErrno = WSAEFAULT;
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
                  if (lpErrno) *lpErrno = WSAEFAULT;
                  return SOCKET_ERROR;
              }
              Socket->SharedData->Broadcast = (*optval != 0) ? 1 : 0;
              return NO_ERROR;

           case SO_OOBINLINE:
              if (optlen < sizeof(BOOL))
              {
                  if (lpErrno) *lpErrno = WSAEFAULT;
                  return SOCKET_ERROR;
              }
              Socket->SharedData->OobInline = (*optval != 0) ? 1 : 0;
              return NO_ERROR;

           case SO_DONTLINGER:
              if (optlen < sizeof(BOOL))
              {
                  if (lpErrno) *lpErrno = WSAEFAULT;
                  return SOCKET_ERROR;
              }
              Socket->SharedData->LingerData.l_onoff = (*optval != 0) ? 0 : 1;
              return NO_ERROR;

           case SO_REUSEADDR:
              if (optlen < sizeof(BOOL))
              {
                  if (lpErrno) *lpErrno = WSAEFAULT;
                  return SOCKET_ERROR;
              }
              Socket->SharedData->ReuseAddresses = (*optval != 0) ? 1 : 0;
              return NO_ERROR;

           case SO_EXCLUSIVEADDRUSE:
              if (optlen < sizeof(BOOL))
              {
                  if (lpErrno) *lpErrno = WSAEFAULT;
                  return SOCKET_ERROR;
              }
              Socket->SharedData->ExclusiveAddressUse = (*optval != 0) ? 1 : 0;
              return NO_ERROR;

           case SO_LINGER:
              if (optlen < sizeof(struct linger))
              {
                  if (lpErrno) *lpErrno = WSAEFAULT;
                  return SOCKET_ERROR;
              }
              RtlCopyMemory(&Socket->SharedData->LingerData,
                            optval,
                            sizeof(struct linger));
              return NO_ERROR;

           case SO_SNDBUF:
              if (optlen < sizeof(ULONG))
              {
                  if (lpErrno) *lpErrno = WSAEFAULT;
                  return SOCKET_ERROR;
              }

              SetSocketInformation(Socket,
                                   AFD_INFO_SEND_WINDOW_SIZE,
                                   NULL,
                                   (PULONG)optval,
                                   NULL,
                                   NULL,
                                   NULL);
              GetSocketInformation(Socket,
                                   AFD_INFO_SEND_WINDOW_SIZE,
                                   NULL,
                                   &Socket->SharedData->SizeOfSendBuffer,
                                   NULL,
                                   NULL,
                                   NULL);

              return NO_ERROR;

           case SO_RCVBUF:
              if (optlen < sizeof(ULONG))
              {
                  if (lpErrno) *lpErrno = WSAEFAULT;
                  return SOCKET_ERROR;
              }

              /* FIXME: We should not have to limit the packet receive buffer size like this. workaround for CORE-15804 */
              if (*(PULONG)optval > 0x2000)
                  *(PULONG)optval = 0x2000;

              SetSocketInformation(Socket,
                                   AFD_INFO_RECEIVE_WINDOW_SIZE,
                                   NULL,
                                   (PULONG)optval,
                                   NULL,
                                   NULL,
                                   NULL);
              GetSocketInformation(Socket,
                                   AFD_INFO_RECEIVE_WINDOW_SIZE,
                                   NULL,
                                   &Socket->SharedData->SizeOfRecvBuffer,
                                   NULL,
                                   NULL,
                                   NULL);

              return NO_ERROR;

           case SO_ERROR:
              if (optlen < sizeof(INT))
              {
                  if (lpErrno) *lpErrno = WSAEFAULT;
                  return SOCKET_ERROR;
              }

              RtlCopyMemory(&Socket->SharedData->SocketLastError,
                            optval,
                            sizeof(INT));
              return NO_ERROR;

           case SO_SNDTIMEO:
              if (optlen < sizeof(DWORD))
              {
                  if (lpErrno) *lpErrno = WSAEFAULT;
                  return SOCKET_ERROR;
              }

              RtlCopyMemory(&Socket->SharedData->SendTimeout,
                            optval,
                            sizeof(DWORD));
              return NO_ERROR;

           case SO_RCVTIMEO:
              if (optlen < sizeof(DWORD))
              {
                  if (lpErrno) *lpErrno = WSAEFAULT;
                  return SOCKET_ERROR;
              }

              RtlCopyMemory(&Socket->SharedData->RecvTimeout,
                            optval,
                            sizeof(DWORD));
              return NO_ERROR;

           case SO_KEEPALIVE:
           case SO_DONTROUTE:
              /* These go directly to the helper dll */
              goto SendToHelper;

           default:
              /* Obviously this is a hack */
              ERR("MSAFD: Set unknown optname %x\n", optname);
              return NO_ERROR;
        }
    }

SendToHelper:
    Errno = Socket->HelperData->WSHSetSocketInformation(Socket->HelperContext,
                                                        s,
                                                        Socket->TdiAddressHandle,
                                                        Socket->TdiConnectionHandle,
                                                        level,
                                                        optname,
                                                        (PCHAR)optval,
                                                        optlen);
    if (lpErrno) *lpErrno = Errno;
    return (Errno == NO_ERROR) ? NO_ERROR : SOCKET_ERROR;
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
_Must_inspect_result_
int
WSPAPI
WSPStartup(
    _In_ WORD wVersionRequested,
    _In_ LPWSPDATA lpWSPData,
    _In_ LPWSAPROTOCOL_INFOW lpProtocolInfo,
    _In_ WSPUPCALLTABLE UpcallTable,
    _Out_ LPWSPPROC_TABLE lpProcTable)
{
    NTSTATUS Status;

    if (((LOBYTE(wVersionRequested) == 2) && (HIBYTE(wVersionRequested) < 2)) ||
        (LOBYTE(wVersionRequested) < 2))
    {
        ERR("WSPStartup NOT SUPPORTED for version 0x%X\n", wVersionRequested);
        return WSAVERNOTSUPPORTED;
    }
    else
        Status = NO_ERROR;
    /* FIXME: Enable all cases of WSPStartup status */
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
        /* Save CatalogEntryId for all upcalls */
        CatalogEntryId = lpProtocolInfo->dwCatalogEntryId;
    }

    TRACE("Status (%d).\n", Status);
    return Status;
}


INT
WSPAPI
WSPAddressToString(IN LPSOCKADDR lpsaAddress,
                   IN DWORD dwAddressLength,
                   IN LPWSAPROTOCOL_INFOW lpProtocolInfo,
                   OUT LPWSTR lpszAddressString,
                   IN OUT LPDWORD lpdwAddressStringLength,
                   OUT LPINT lpErrno)
{
    SIZE_T size;
    WCHAR buffer[54]; /* 32 digits + 7':' + '[' + '%" + 5 digits + ']:' + 5 digits + '\0' */
    WCHAR *p;

    if (!lpsaAddress || !lpszAddressString || !lpdwAddressStringLength)
    {
        if (lpErrno) *lpErrno = WSAEFAULT;
        return SOCKET_ERROR;
    }

    switch (lpsaAddress->sa_family)
    {
        case AF_INET:
            if (dwAddressLength < sizeof(SOCKADDR_IN))
            {
                if (lpErrno) *lpErrno = WSAEINVAL;
                return SOCKET_ERROR;
            }
            swprintf(buffer,
                     L"%u.%u.%u.%u:%u",
                     (unsigned int)(ntohl(((SOCKADDR_IN *)lpsaAddress)->sin_addr.s_addr) >> 24 & 0xff),
                     (unsigned int)(ntohl(((SOCKADDR_IN *)lpsaAddress)->sin_addr.s_addr) >> 16 & 0xff),
                     (unsigned int)(ntohl(((SOCKADDR_IN *)lpsaAddress)->sin_addr.s_addr) >> 8 & 0xff),
                     (unsigned int)(ntohl(((SOCKADDR_IN *)lpsaAddress)->sin_addr.s_addr) & 0xff),
                     ntohs(((SOCKADDR_IN *)lpsaAddress)->sin_port));

            p = wcschr(buffer, L':');
            if (!((SOCKADDR_IN *)lpsaAddress)->sin_port)
            {
                *p = 0;
            }
            break;
        default:
            if (lpErrno) *lpErrno = WSAEINVAL;
            return SOCKET_ERROR;
    }

    size = wcslen(buffer) + 1;

    if (*lpdwAddressStringLength < size)
    {
        *lpdwAddressStringLength = size;
        if (lpErrno) *lpErrno = WSAEFAULT;
        return SOCKET_ERROR;
    }

    *lpdwAddressStringLength = size;
    wcscpy(lpszAddressString, buffer);
    return 0;
}

INT
WSPAPI
WSPStringToAddress(IN LPWSTR AddressString,
                   IN INT AddressFamily,
                   IN LPWSAPROTOCOL_INFOW lpProtocolInfo,
                   OUT LPSOCKADDR lpAddress,
                   IN OUT LPINT lpAddressLength,
                   OUT LPINT lpErrno)
{
    int numdots = 0;
    USHORT port;
    LONG inetaddr = 0, ip_part;
    LPWSTR *bp = NULL;
    SOCKADDR_IN *sockaddr;

    if (!lpAddressLength || !lpAddress || !AddressString)
    {
        if (lpErrno) *lpErrno = WSAEINVAL;
        return SOCKET_ERROR;
    }

    sockaddr = (SOCKADDR_IN *)lpAddress;

    /* Set right address family */
    if (lpProtocolInfo != NULL)
    {
        sockaddr->sin_family = lpProtocolInfo->iAddressFamily;
    }
    else
    {
        sockaddr->sin_family = AddressFamily;
    }

    /* Report size */
    if (AddressFamily == AF_INET)
    {
        if (*lpAddressLength < (INT)sizeof(SOCKADDR_IN))
        {
            if (lpErrno) *lpErrno = WSAEFAULT;
        }
        else
        {
            // translate ip string to ip

            /* Get ip number */
            bp = &AddressString;
            inetaddr = 0;

            while (*bp < &AddressString[wcslen(AddressString)])
            {
                ip_part = wcstol(*bp, bp, 10);
                /* ip part number should be in range 0-255 */
                if (ip_part < 0 || ip_part > 255)
                {
                    if (lpErrno) *lpErrno = WSAEINVAL;
                    return SOCKET_ERROR;
                }
                inetaddr = (inetaddr << 8) + ip_part;
                /* we end on string end or port separator */
                if ((*bp)[0] == 0 || (*bp)[0] == L':')
                    break;
                /* ip parts are dot separated. verify it */
                if ((*bp)[0] != L'.')
                {
                    if (lpErrno) *lpErrno = WSAEINVAL;
                    return SOCKET_ERROR;
                }
                /* count the dots */
                numdots++;
                /* move over the dot to next ip part */
                (*bp)++;
            }

            /* check dots count */
            if (numdots != 3)
            {
                if (lpErrno) *lpErrno = WSAEINVAL;
                return SOCKET_ERROR;
            }

            /* Get port number */
            if ((*bp)[0] == L':')
            {
                /* move over the column to port part */
                (*bp)++;
                /* next char should be numeric */
                if ((*bp)[0] < L'0' || (*bp)[0] > L'9')
                {
                    if (lpErrno) *lpErrno = WSAEINVAL;
                    return SOCKET_ERROR;
                }
                port = wcstol(*bp, bp, 10);
            }
            else
            {
                port = 0;
            }

            if (lpErrno) *lpErrno = NO_ERROR;
            /* rest sockaddr.sin_addr.s_addr
            for we need to be sure it is zero when we come to while */
            *lpAddressLength = sizeof(*sockaddr);
            memset(lpAddress, 0, sizeof(*sockaddr));
            sockaddr->sin_family = AF_INET;
            sockaddr->sin_addr.s_addr = inetaddr;
            sockaddr->sin_port = port;
        }
    }

    if (lpErrno && !*lpErrno)
    {
        return 0;
    }

    return SOCKET_ERROR;
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

    if (lpErrno) *lpErrno = NO_ERROR;

    return 0;
}

VOID
NTAPI
AfdInfoAPC(PVOID ApcContext,
    PIO_STATUS_BLOCK IoStatusBlock,
    ULONG Reserved)
{
    PAFDAPCCONTEXT Context = ApcContext;

    Context->lpCompletionRoutine(IoStatusBlock->Status, IoStatusBlock->Information, Context->lpOverlapped, 0);
    HeapFree(GlobalHeap, 0, ApcContext);
}

int
GetSocketInformation(PSOCKET_INFORMATION Socket,
                     ULONG AfdInformationClass,
                     PBOOLEAN Boolean OPTIONAL,
                     PULONG Ulong OPTIONAL,
                     PLARGE_INTEGER LargeInteger OPTIONAL,
                     LPWSAOVERLAPPED Overlapped OPTIONAL,
                     LPWSAOVERLAPPED_COMPLETION_ROUTINE CompletionRoutine OPTIONAL)
{
    PIO_STATUS_BLOCK    IOSB;
    IO_STATUS_BLOCK     DummyIOSB;
    AFD_INFO            InfoData;
    NTSTATUS            Status;
    PAFDAPCCONTEXT      APCContext;
    PIO_APC_ROUTINE     APCFunction;
    HANDLE              Event = NULL;
    HANDLE              SockEvent;

    Status = NtCreateEvent(&SockEvent,
                           EVENT_ALL_ACCESS,
                           NULL,
                           SynchronizationEvent,
                           FALSE);

    if( !NT_SUCCESS(Status) )
        return SOCKET_ERROR;

    /* Set Info Class */
    InfoData.InformationClass = AfdInformationClass;

    /* Verify if we should use APC */
    if (Overlapped == NULL)
    {
        /* Not using Overlapped structure, so use normal blocking on event */
        APCContext = NULL;
        APCFunction = NULL;
        Event = SockEvent;
        IOSB = &DummyIOSB;
    }
    else
    {
        /* Overlapped request for non overlapped opened socket */
        if ((Socket->SharedData->CreateFlags & SO_SYNCHRONOUS_NONALERT) != 0)
        {
            TRACE("Opened without flag WSA_FLAG_OVERLAPPED. Do nothing.\n");
            NtClose( SockEvent );
            return 0;
        }
        if (CompletionRoutine == NULL)
        {
            /* Using Overlapped Structure, but no Completition Routine, so no need for APC */
            APCContext = (PAFDAPCCONTEXT)Overlapped;
            APCFunction = NULL;
            Event = Overlapped->hEvent;
        }
        else
        {
            /* Using Overlapped Structure and a Completition Routine, so use an APC */
            APCFunction = &AfdInfoAPC; // should be a private io completition function inside us
            APCContext = HeapAlloc(GlobalHeap, 0, sizeof(AFDAPCCONTEXT));
            if (!APCContext)
            {
                ERR("Not enough memory for APC Context\n");
                NtClose( SockEvent );
                return WSAEFAULT;
            }
            APCContext->lpCompletionRoutine = CompletionRoutine;
            APCContext->lpOverlapped = Overlapped;
            APCContext->lpSocket = Socket;
        }

        IOSB = (PIO_STATUS_BLOCK)&Overlapped->Internal;
    }

    IOSB->Status = STATUS_PENDING;

    /* Send IOCTL */
    Status = NtDeviceIoControlFile((HANDLE)Socket->Handle,
                                   Event,
                                   APCFunction,
                                   APCContext,
                                   IOSB,
                                   IOCTL_AFD_GET_INFO,
                                   &InfoData,
                                   sizeof(InfoData),
                                   &InfoData,
                                   sizeof(InfoData));

    /* Wait for return */
    if (Status == STATUS_PENDING && Overlapped == NULL)
    {
        WaitForSingleObject(SockEvent, INFINITE);
        Status = IOSB->Status;
    }

    NtClose( SockEvent );

    TRACE("Status %x Information %d\n", Status, IOSB->Information);

    if (Status == STATUS_PENDING)
    {
        TRACE("Leaving (Pending)\n");
        return WSA_IO_PENDING;
    }

    if (Status != STATUS_SUCCESS)
        return SOCKET_ERROR;

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

    return NO_ERROR;

}


int
SetSocketInformation(PSOCKET_INFORMATION Socket,
                     ULONG AfdInformationClass,
                     PBOOLEAN Boolean OPTIONAL,
                     PULONG Ulong OPTIONAL,
                     PLARGE_INTEGER LargeInteger OPTIONAL,
                     LPWSAOVERLAPPED Overlapped OPTIONAL,
                     LPWSAOVERLAPPED_COMPLETION_ROUTINE CompletionRoutine OPTIONAL)
{
    PIO_STATUS_BLOCK    IOSB;
    IO_STATUS_BLOCK     DummyIOSB;
    AFD_INFO            InfoData;
    NTSTATUS            Status;
    PAFDAPCCONTEXT      APCContext;
    PIO_APC_ROUTINE     APCFunction;
    HANDLE              Event = NULL;
    HANDLE              SockEvent;

    Status = NtCreateEvent(&SockEvent,
                           EVENT_ALL_ACCESS,
                           NULL,
                           SynchronizationEvent,
                           FALSE);

    if( !NT_SUCCESS(Status) )
        return SOCKET_ERROR;

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

    /* Verify if we should use APC */
    if (Overlapped == NULL)
    {
        /* Not using Overlapped structure, so use normal blocking on event */
        APCContext = NULL;
        APCFunction = NULL;
        Event = SockEvent;
        IOSB = &DummyIOSB;
    }
    else
    {
        /* Overlapped request for non overlapped opened socket */
        if ((Socket->SharedData->CreateFlags & SO_SYNCHRONOUS_NONALERT) != 0)
        {
            TRACE("Opened without flag WSA_FLAG_OVERLAPPED. Do nothing.\n");
            NtClose( SockEvent );
            return 0;
        }
        if (CompletionRoutine == NULL)
        {
            /* Using Overlapped Structure, but no Completition Routine, so no need for APC */
            APCContext = (PAFDAPCCONTEXT)Overlapped;
            APCFunction = NULL;
            Event = Overlapped->hEvent;
        }
        else
        {
            /* Using Overlapped Structure and a Completition Routine, so use an APC */
            APCFunction = &AfdInfoAPC; // should be a private io completition function inside us
            APCContext = HeapAlloc(GlobalHeap, 0, sizeof(AFDAPCCONTEXT));
            if (!APCContext)
            {
                ERR("Not enough memory for APC Context\n");
                NtClose( SockEvent );
                return WSAEFAULT;
            }
            APCContext->lpCompletionRoutine = CompletionRoutine;
            APCContext->lpOverlapped = Overlapped;
            APCContext->lpSocket = Socket;
        }

        IOSB = (PIO_STATUS_BLOCK)&Overlapped->Internal;
    }

    IOSB->Status = STATUS_PENDING;

    /* Send IOCTL */
    Status = NtDeviceIoControlFile((HANDLE)Socket->Handle,
                                   Event,
                                   APCFunction,
                                   APCContext,
                                   IOSB,
                                   IOCTL_AFD_SET_INFO,
                                   &InfoData,
                                   sizeof(InfoData),
                                   NULL,
                                   0);

    /* Wait for return */
    if (Status == STATUS_PENDING && Overlapped == NULL)
    {
        WaitForSingleObject(SockEvent, INFINITE);
        Status = IOSB->Status;
    }

    NtClose( SockEvent );

    TRACE("Status %x Information %d\n", Status, IOSB->Information);

    if (Status == STATUS_PENDING)
    {
        TRACE("Leaving (Pending)\n");
        return WSA_IO_PENDING;
    }

    return Status == STATUS_SUCCESS ? NO_ERROR : SOCKET_ERROR;

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
                           SynchronizationEvent,
                           FALSE);

    if( !NT_SUCCESS(Status) )
        return SOCKET_ERROR;

    /* Create Context */
    ContextData.SharedData = *Socket->SharedData;
    ContextData.SizeOfHelperData = 0;
    RtlCopyMemory (&ContextData.LocalAddress,
                   Socket->LocalAddress,
                   Socket->SharedData->SizeOfLocalAddress);
    RtlCopyMemory (&ContextData.RemoteAddress,
                   Socket->RemoteAddress,
                   Socket->SharedData->SizeOfRemoteAddress);

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

    /* Wait for Completion */
    if (Status == STATUS_PENDING)
    {
        WaitForSingleObject(SockEvent, INFINITE);
        Status = IOSB.Status;
    }

    NtClose( SockEvent );

    return Status == STATUS_SUCCESS ? NO_ERROR : SOCKET_ERROR;
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
                                SockAsyncThread,
                                NULL,
                                0,
                                &AsyncThreadId);

    /* Close the Handle */
    NtClose(hAsyncThread);

    /* Increase the Reference Count */
    SockAsyncThreadRefCount++;
    return TRUE;
}

ULONG
NTAPI
SockAsyncThread(PVOID ThreadParam)
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

    /* First, make sure we're not already initialized */
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
    if (AsyncData->SequenceNumber != Socket->SharedData->SequenceNumber )
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
                if (0 != (Socket->SharedData->AsyncEvents & FD_READ) &&
                    0 == (Socket->SharedData->AsyncDisabledEvents & FD_READ))
                {
                    /* Make the Notification */
                    (Upcalls.lpWPUPostMessage)(Socket->SharedData->hWnd,
                                               Socket->SharedData->wMsg,
                                               Socket->Handle,
                                               WSAMAKESELECTREPLY(FD_READ, 0));
                    /* Disable this event until the next read(); */
                    Socket->SharedData->AsyncDisabledEvents |= FD_READ;
                }
            break;

            case AFD_EVENT_OOB_RECEIVE:
            if (0 != (Socket->SharedData->AsyncEvents & FD_OOB) &&
                0 == (Socket->SharedData->AsyncDisabledEvents & FD_OOB))
            {
                /* Make the Notification */
                (Upcalls.lpWPUPostMessage)(Socket->SharedData->hWnd,
                                           Socket->SharedData->wMsg,
                                           Socket->Handle,
                                           WSAMAKESELECTREPLY(FD_OOB, 0));
                /* Disable this event until the next read(); */
                Socket->SharedData->AsyncDisabledEvents |= FD_OOB;
            }
            break;

            case AFD_EVENT_SEND:
                if (0 != (Socket->SharedData->AsyncEvents & FD_WRITE) &&
                    0 == (Socket->SharedData->AsyncDisabledEvents & FD_WRITE))
                {
                    /* Make the Notification */
                    (Upcalls.lpWPUPostMessage)(Socket->SharedData->hWnd,
                                               Socket->SharedData->wMsg,
                                               Socket->Handle,
                                               WSAMAKESELECTREPLY(FD_WRITE, 0));
                    /* Disable this event until the next write(); */
                    Socket->SharedData->AsyncDisabledEvents |= FD_WRITE;
                }
                break;

                /* FIXME: THIS IS NOT RIGHT!!! HACK HACK HACK! */
            case AFD_EVENT_CONNECT:
            case AFD_EVENT_CONNECT_FAIL:
                if (0 != (Socket->SharedData->AsyncEvents & FD_CONNECT) &&
                    0 == (Socket->SharedData->AsyncDisabledEvents & FD_CONNECT))
                {
                    /* Make the Notification */
                    (Upcalls.lpWPUPostMessage)(Socket->SharedData->hWnd,
                        Socket->SharedData->wMsg,
                        Socket->Handle,
                        WSAMAKESELECTREPLY(FD_CONNECT, 0));
                    /* Disable this event forever; */
                    Socket->SharedData->AsyncDisabledEvents |= FD_CONNECT;
                }
                break;

            case AFD_EVENT_ACCEPT:
                if (0 != (Socket->SharedData->AsyncEvents & FD_ACCEPT) &&
                    0 == (Socket->SharedData->AsyncDisabledEvents & FD_ACCEPT))
                {
                    /* Make the Notification */
                    (Upcalls.lpWPUPostMessage)(Socket->SharedData->hWnd,
                                               Socket->SharedData->wMsg,
                                               Socket->Handle,
                                               WSAMAKESELECTREPLY(FD_ACCEPT, 0));
                    /* Disable this event until the next accept(); */
                    Socket->SharedData->AsyncDisabledEvents |= FD_ACCEPT;
                }
                break;

            case AFD_EVENT_DISCONNECT:
            case AFD_EVENT_ABORT:
            case AFD_EVENT_CLOSE:
                if (0 != (Socket->SharedData->AsyncEvents & FD_CLOSE) &&
                    0 == (Socket->SharedData->AsyncDisabledEvents & FD_CLOSE))
                {
                    /* Make the Notification */
                    (Upcalls.lpWPUPostMessage)(Socket->SharedData->hWnd,
                                               Socket->SharedData->wMsg,
                                               Socket->Handle,
                                               WSAMAKESELECTREPLY(FD_CLOSE, 0));
                    /* Disable this event forever; */
                    Socket->SharedData->AsyncDisabledEvents |= FD_CLOSE;
                }
                 break;
            /* FIXME: Support QOS */
        }
    }

    /* Check if there are any events left for us to check */
    if ((Socket->SharedData->AsyncEvents & (~Socket->SharedData->AsyncDisabledEvents)) == 0 )
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
    lNetworkEvents = Socket->SharedData->AsyncEvents & (~Socket->SharedData->AsyncDisabledEvents);

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
    if (Socket->SharedData->State != SocketClosed)
    {
        /* Check if the Sequence Number changed by now, in which case quit */
        if (AsyncData->SequenceNumber == Socket->SharedData->SequenceNumber)
        {
            /* Do the actual select, if needed */
            if ((Socket->SharedData->AsyncEvents & (~Socket->SharedData->AsyncDisabledEvents)))
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
    if (!(Socket->SharedData->AsyncDisabledEvents & Event))
    {
        return;
    }

    /* Re-enable it */
    Socket->SharedData->AsyncDisabledEvents &= ~Event;

    /* Return if no more events are being polled */
    if ((Socket->SharedData->AsyncEvents & (~Socket->SharedData->AsyncDisabledEvents)) == 0 )
    {
        return;
    }

    /* Wait on new events */
    AsyncData = HeapAlloc(GetProcessHeap(), 0, sizeof(ASYNC_DATA));
    if (!AsyncData) return;

    /* Create the Asynch Thread if Needed */
    SockCreateOrReferenceAsyncThread();

    /* Increase the sequence number to stop anything else */
    Socket->SharedData->SequenceNumber++;

    /* Set up the Async Data */
    AsyncData->ParentSocket = Socket;
    AsyncData->SequenceNumber = Socket->SharedData->SequenceNumber;

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
