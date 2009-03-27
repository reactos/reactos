/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Ancillary Function Driver DLL
 * FILE:        misc/sndrcv.c
 * PURPOSE:     Send/receive routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Alex Ionescu (alex@relsoft.net)
 * REVISIONS:
 *              CSH 01/09-2000 Created
 *              Alex 16/07/2004 - Complete Rewrite
 */

#include <msafd.h>

#include <debug.h>

INT
WSPAPI
WSPAsyncSelect(IN  SOCKET Handle,
               IN  HWND hWnd,
               IN  UINT wMsg,
               IN  LONG lEvent,
               OUT LPINT lpErrno)
{
    PSOCKET_INFORMATION Socket = NULL;
    PASYNC_DATA                 AsyncData;
    NTSTATUS                    Status;
    ULONG                       BlockMode;

    /* Get the Socket Structure associated to this Socket */
    Socket = GetSocketStructure(Handle);

    /* Allocate the Async Data Structure to pass on to the Thread later */
    AsyncData = HeapAlloc(GetProcessHeap(), 0, sizeof(*AsyncData));

    /* Change the Socket to Non Blocking */
    BlockMode = 1;
    SetSocketInformation(Socket, AFD_INFO_BLOCKING_MODE, &BlockMode, NULL);
    Socket->SharedData.NonBlocking = TRUE;

    /* Deactive WSPEventSelect */
    if (Socket->SharedData.AsyncEvents)
    {
        WSPEventSelect(Handle, NULL, 0, NULL);
    }

    /* Create the Asynch Thread if Needed */  
    SockCreateOrReferenceAsyncThread();

    /* Open a Handle to AFD's Async Helper */
    SockGetAsyncSelectHelperAfdHandle();

    /* Store Socket Data */
    Socket->SharedData.hWnd = hWnd;
    Socket->SharedData.wMsg = wMsg;
    Socket->SharedData.AsyncEvents = lEvent;
    Socket->SharedData.AsyncDisabledEvents = 0;
    Socket->SharedData.SequenceNumber++;

    /* Return if there are no more Events */
    if ((Socket->SharedData.AsyncEvents & (~Socket->SharedData.AsyncDisabledEvents)) == 0)
    {
        HeapFree(GetProcessHeap(), 0, AsyncData);
        return 0;
    }

    /* Set up the Async Data */
    AsyncData->ParentSocket = Socket;
    AsyncData->SequenceNumber = Socket->SharedData.SequenceNumber;

    /* Begin Async Select by using I/O Completion */
    Status = NtSetIoCompletion(SockAsyncCompletionPort,
                               (PVOID)&SockProcessQueuedAsyncSelect,
                                AsyncData,
                                0,
                                0);

    /* Return */
    return ERROR_SUCCESS;
}


int 
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
    PIO_STATUS_BLOCK        IOSB;
    IO_STATUS_BLOCK         DummyIOSB;
    AFD_RECV_INFO           RecvInfo;
    NTSTATUS                Status;
    PVOID                   APCContext;
    PVOID                   APCFunction;
    HANDLE                  Event = NULL;
    HANDLE                  SockEvent;
    PSOCKET_INFORMATION     Socket;

    AFD_DbgPrint(MID_TRACE,("Called (%x)\n", Handle));

    /* Get the Socket Structure associate to this Socket*/
    Socket = GetSocketStructure(Handle);

    Status = NtCreateEvent( &SockEvent, GENERIC_READ | GENERIC_WRITE,
                            NULL, 1, FALSE );

    if( !NT_SUCCESS(Status) )
        return -1;

    /* Set up the Receive Structure */
    RecvInfo.BufferArray = (PAFD_WSABUF)lpBuffers;
    RecvInfo.BufferCount = dwBufferCount;
    RecvInfo.TdiFlags = 0;
    RecvInfo.AfdFlags = Socket->SharedData.NonBlocking ? AFD_IMMEDIATE : 0;

    /* Set the TDI Flags */
    if (*ReceiveFlags == 0)
    {
        RecvInfo.TdiFlags |= TDI_RECEIVE_NORMAL;
    }
    else
    {
        if (*ReceiveFlags & MSG_OOB)
        {
            RecvInfo.TdiFlags |= TDI_RECEIVE_EXPEDITED;
        }
        else
        {
            RecvInfo.TdiFlags |= TDI_RECEIVE_NORMAL;
        }

        if (*ReceiveFlags & MSG_PEEK)
        {
            RecvInfo.TdiFlags |= TDI_RECEIVE_PEEK;
        }

        if (*ReceiveFlags & MSG_PARTIAL) {
            RecvInfo.TdiFlags |= TDI_RECEIVE_NORMAL;
        }
    }

    /* Verifiy if we should use APC */

    if (lpOverlapped == NULL)
    {
        /* Not using Overlapped structure, so use normal blocking on event */
        APCContext = NULL;
        APCFunction = NULL;
        Event = SockEvent;
        IOSB = &DummyIOSB;
    } 
    else
    {
        if (lpCompletionRoutine == NULL)
        {
            /* Using Overlapped Structure, but no Completition Routine, so no need for APC */
            APCContext = lpOverlapped;
            APCFunction = NULL;
            Event = lpOverlapped->hEvent;
        }
        else
        {
            /* Using Overlapped Structure and a Completition Routine, so use an APC */
            APCFunction = NULL; // should be a private io completition function inside us
            APCContext = lpCompletionRoutine;
            RecvInfo.AfdFlags = AFD_SKIP_FIO;
        }

        IOSB = (PIO_STATUS_BLOCK)&lpOverlapped->Internal;
        RecvInfo.AfdFlags |= AFD_OVERLAPPED;
    }

    IOSB->Status = STATUS_PENDING;

    /* Send IOCTL */
    Status = NtDeviceIoControlFile((HANDLE)Handle,
        Event ? Event : SockEvent,
        APCFunction,
        APCContext,
        IOSB,
        IOCTL_AFD_RECV,
        &RecvInfo,
        sizeof(RecvInfo),
        NULL,
        0);

    /* Wait for completition of not overlapped */
    if (Status == STATUS_PENDING && lpOverlapped == NULL)
    {
        /* It's up to the protocol to time out recv.  We must wait
         * until the protocol decides it's had enough.
         */
        WaitForSingleObject(SockEvent, INFINITE);
        Status = IOSB->Status;
    }

    NtClose( SockEvent );

    AFD_DbgPrint(MID_TRACE,("Status %x Information %d\n", Status, IOSB->Information));

    /* Return the Flags */
    *ReceiveFlags = 0;

    switch (Status)
    {
        case STATUS_RECEIVE_EXPEDITED:
            *ReceiveFlags = MSG_OOB;
            break;
        case STATUS_RECEIVE_PARTIAL_EXPEDITED: 
            *ReceiveFlags = MSG_PARTIAL | MSG_OOB;
            break;
        case STATUS_RECEIVE_PARTIAL:
            *ReceiveFlags = MSG_PARTIAL;
            break;
    }

    /* Re-enable Async Event */
    if (*ReceiveFlags == MSG_OOB)
    {
        SockReenableAsyncSelectEvent(Socket, FD_OOB);
    }
    else
    {
        SockReenableAsyncSelectEvent(Socket, FD_READ);
    }

    return MsafdReturnWithErrno ( Status, lpErrno, IOSB->Information, lpNumberOfBytesRead );
}

int 
WSPAPI 
WSPRecvFrom(SOCKET Handle, 
            LPWSABUF lpBuffers, 
            DWORD dwBufferCount, 
            LPDWORD lpNumberOfBytesRead, 
            LPDWORD ReceiveFlags, 
            struct sockaddr *SocketAddress, 
            int *SocketAddressLength, 
            LPWSAOVERLAPPED lpOverlapped, 
            LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine, 
            LPWSATHREADID lpThreadId, 
            LPINT lpErrno )
{
    PIO_STATUS_BLOCK            IOSB;
    IO_STATUS_BLOCK             DummyIOSB;
    AFD_RECV_INFO_UDP           RecvInfo;
    NTSTATUS                    Status;
    PVOID                       APCContext;
    PVOID                       APCFunction;
    HANDLE                      Event = NULL;
    HANDLE                      SockEvent;
    PSOCKET_INFORMATION         Socket;

    /* Get the Socket Structure associate to this Socket*/
    Socket = GetSocketStructure(Handle);

    Status = NtCreateEvent( &SockEvent, GENERIC_READ | GENERIC_WRITE,
                            NULL, 1, FALSE );

    if( !NT_SUCCESS(Status) )
        return -1;

    /* Set up the Receive Structure */
    RecvInfo.BufferArray = (PAFD_WSABUF)lpBuffers;
    RecvInfo.BufferCount = dwBufferCount;
    RecvInfo.TdiFlags = 0;
    RecvInfo.AfdFlags = Socket->SharedData.NonBlocking ? AFD_IMMEDIATE : 0;
    RecvInfo.AddressLength = SocketAddressLength;
    RecvInfo.Address = SocketAddress;

    /* Set the TDI Flags */
    if (*ReceiveFlags == 0)
    {
        RecvInfo.TdiFlags |= TDI_RECEIVE_NORMAL;
    }
    else
    {
        if (*ReceiveFlags & MSG_OOB) 
        {
            RecvInfo.TdiFlags |= TDI_RECEIVE_EXPEDITED;
        }
        else
        {
            RecvInfo.TdiFlags |= TDI_RECEIVE_NORMAL;
        }

        if (*ReceiveFlags & MSG_PEEK)
        {
            RecvInfo.TdiFlags |= TDI_RECEIVE_PEEK;
        }

        if (*ReceiveFlags & MSG_PARTIAL)
        {
            RecvInfo.TdiFlags |= TDI_RECEIVE_NORMAL;
        }
    }

    /* Verifiy if we should use APC */

    if (lpOverlapped == NULL) 
    {
        /* Not using Overlapped structure, so use normal blocking on event */
        APCContext = NULL;
        APCFunction = NULL;
        Event = SockEvent;
        IOSB = &DummyIOSB;
    }
    else
    {
        if (lpCompletionRoutine == NULL)
        {
            /* Using Overlapped Structure, but no Completition Routine, so no need for APC */
            APCContext = lpOverlapped;
            APCFunction = NULL;
            Event = lpOverlapped->hEvent;
        }
        else
        {
            /* Using Overlapped Structure and a Completition Routine, so use an APC */
            APCFunction = NULL; // should be a private io completition function inside us
            APCContext = lpCompletionRoutine;
            RecvInfo.AfdFlags = AFD_SKIP_FIO;
        }

        IOSB = (PIO_STATUS_BLOCK)&lpOverlapped->Internal;
        RecvInfo.AfdFlags |= AFD_OVERLAPPED;
    }

    IOSB->Status = STATUS_PENDING;

    /* Send IOCTL */
    Status = NtDeviceIoControlFile((HANDLE)Handle,
                                    Event ? Event : SockEvent,
                                    APCFunction,
                                    APCContext,
                                    IOSB,
                                    IOCTL_AFD_RECV_DATAGRAM,
                                    &RecvInfo,
                                    sizeof(RecvInfo),
                                    NULL,
                                    0);

    /* Wait for completition of not overlapped */
    if (Status == STATUS_PENDING && lpOverlapped == NULL)
    {
        WaitForSingleObject(SockEvent, INFINITE); // BUGBUG, shouldn wait infintely for receive...
        Status = IOSB->Status;
    }

    NtClose( SockEvent );

    /* Return the Flags */
    *ReceiveFlags = 0;

    switch (Status)
    {
        case STATUS_RECEIVE_EXPEDITED: *ReceiveFlags = MSG_OOB;
            break;
        case STATUS_RECEIVE_PARTIAL_EXPEDITED: 
            *ReceiveFlags = MSG_PARTIAL | MSG_OOB;
            break;
        case STATUS_RECEIVE_PARTIAL:
            *ReceiveFlags = MSG_PARTIAL;
            break;
    }

    /* Re-enable Async Event */
    SockReenableAsyncSelectEvent(Socket, FD_READ);

    return MsafdReturnWithErrno ( Status, lpErrno, IOSB->Information, lpNumberOfBytesRead );
}


int
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
    PIO_STATUS_BLOCK        IOSB;
    IO_STATUS_BLOCK         DummyIOSB;
    AFD_SEND_INFO           SendInfo;
    NTSTATUS                Status;
    PVOID                   APCContext;
    PVOID                   APCFunction;
    HANDLE                  Event = NULL;
    HANDLE                  SockEvent;
    PSOCKET_INFORMATION     Socket;

    /* Get the Socket Structure associate to this Socket*/
    Socket = GetSocketStructure(Handle);

    Status = NtCreateEvent( &SockEvent, GENERIC_READ | GENERIC_WRITE,
                            NULL, 1, FALSE );

    if( !NT_SUCCESS(Status) )
        return -1;

    AFD_DbgPrint(MID_TRACE,("Called\n"));

    /* Set up the Send Structure */
    SendInfo.BufferArray = (PAFD_WSABUF)lpBuffers;
    SendInfo.BufferCount = dwBufferCount;
    SendInfo.TdiFlags = 0;
    SendInfo.AfdFlags = Socket->SharedData.NonBlocking ? AFD_IMMEDIATE : 0;

    /* Set the TDI Flags */
    if (iFlags)
    {
        if (iFlags & MSG_OOB)
        {
            SendInfo.TdiFlags |= TDI_SEND_EXPEDITED;
        }
        if (iFlags & MSG_PARTIAL)
        {
            SendInfo.TdiFlags |= TDI_SEND_PARTIAL;
        }
    }

    /* Verifiy if we should use APC */
    if (lpOverlapped == NULL)
    {
        /* Not using Overlapped structure, so use normal blocking on event */
        APCContext = NULL;
        APCFunction = NULL;
        Event = SockEvent;
        IOSB = &DummyIOSB;
    }
    else
    {
        if (lpCompletionRoutine == NULL)
        {
            /* Using Overlapped Structure, but no Completition Routine, so no need for APC */
            APCContext = lpOverlapped;
            APCFunction = NULL;
            Event = lpOverlapped->hEvent;
        }
        else
        {
            /* Using Overlapped Structure and a Completition Routine, so use an APC */
            APCFunction = NULL; // should be a private io completition function inside us
            APCContext = lpCompletionRoutine;
            SendInfo.AfdFlags = AFD_SKIP_FIO;
        }

        IOSB = (PIO_STATUS_BLOCK)&lpOverlapped->Internal;
        SendInfo.AfdFlags |= AFD_OVERLAPPED;
    }

    IOSB->Status = STATUS_PENDING;

    /* Send IOCTL */
    Status = NtDeviceIoControlFile((HANDLE)Handle,
                                    Event ? Event : SockEvent,
                                    APCFunction,
                                    APCContext,
                                    IOSB,
                                    IOCTL_AFD_SEND,
                                    &SendInfo,
                                    sizeof(SendInfo),
                                    NULL,
                                    0);

    /* Wait for completition of not overlapped */
    if (Status == STATUS_PENDING && lpOverlapped == NULL)
    {
        WaitForSingleObject(SockEvent, INFINITE); // BUGBUG, shouldn wait infintely for send...
        Status = IOSB->Status;
    }

    NtClose( SockEvent );

    if (Status == STATUS_PENDING)
    {
        AFD_DbgPrint(MID_TRACE,("Leaving (Pending)\n"));
        return WSA_IO_PENDING;
    }

    /* Re-enable Async Event */
    SockReenableAsyncSelectEvent(Socket, FD_WRITE);

    AFD_DbgPrint(MID_TRACE,("Leaving (Success, %d)\n", IOSB->Information));

    return MsafdReturnWithErrno( Status, lpErrno, IOSB->Information, lpNumberOfBytesSent );
}

int 
WSPAPI
WSPSendTo(SOCKET Handle,
          LPWSABUF lpBuffers,
          DWORD dwBufferCount,
          LPDWORD lpNumberOfBytesSent,
          DWORD iFlags,
          const struct sockaddr *SocketAddress,
          int SocketAddressLength,
          LPWSAOVERLAPPED lpOverlapped,
          LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
          LPWSATHREADID lpThreadId,
          LPINT lpErrno)
{
    PIO_STATUS_BLOCK        IOSB;
    IO_STATUS_BLOCK         DummyIOSB;
    AFD_SEND_INFO_UDP       SendInfo;
    NTSTATUS                Status;
    PVOID                   APCContext;
    PVOID                   APCFunction;
    HANDLE                  Event = NULL;
    PTRANSPORT_ADDRESS      RemoteAddress;
    UCHAR                   TdiBuffer[0x16];
    PSOCKADDR               BindAddress;
    INT                     BindAddressLength;
    HANDLE                  SockEvent;
    PSOCKET_INFORMATION     Socket;


    /* Get the Socket Structure associate to this Socket*/
    Socket = GetSocketStructure(Handle);

    Status = NtCreateEvent( &SockEvent, GENERIC_READ | GENERIC_WRITE,
        NULL, 1, FALSE );

    if( !NT_SUCCESS(Status) )
        return -1;

    /* Bind us First */
    if (Socket->SharedData.State == SocketOpen)
    {
        /* Get the Wildcard Address */
        BindAddressLength = Socket->HelperData->MaxWSAddressLength;
        BindAddress = HeapAlloc(GlobalHeap, 0, BindAddressLength);
        Socket->HelperData->WSHGetWildcardSockaddr (Socket->HelperContext,
                                                    BindAddress,
                                                    &BindAddressLength);

        /* Bind it */
        WSPBind(Handle, BindAddress, BindAddressLength, NULL);
    }

    /* Set up Address in TDI Format */
    RemoteAddress = (PTRANSPORT_ADDRESS)TdiBuffer;
    RemoteAddress->TAAddressCount = 1;
    RemoteAddress->Address[0].AddressLength = SocketAddressLength - sizeof(SocketAddress->sa_family);
    RtlCopyMemory(&RemoteAddress->Address[0].AddressType, SocketAddress, SocketAddressLength);

    /* Set up Structure */
    SendInfo.BufferArray = (PAFD_WSABUF)lpBuffers;
    SendInfo.AfdFlags = Socket->SharedData.NonBlocking ? AFD_IMMEDIATE : 0;
    SendInfo.BufferCount = dwBufferCount;
    SendInfo.RemoteAddress = RemoteAddress;
    SendInfo.SizeOfRemoteAddress = Socket->HelperData->MaxTDIAddressLength;

    /* Verifiy if we should use APC */
    if (lpOverlapped == NULL)
    {
        /* Not using Overlapped structure, so use normal blocking on event */
        APCContext = NULL;
        APCFunction = NULL;
        Event = SockEvent;
        IOSB = &DummyIOSB;
    }
    else
    {
        if (lpCompletionRoutine == NULL)
        {
            /* Using Overlapped Structure, but no Completition Routine, so no need for APC */
            APCContext = lpOverlapped;
            APCFunction = NULL;
            Event = lpOverlapped->hEvent;
        }
        else
        {
            /* Using Overlapped Structure and a Completition Routine, so use an APC */
            APCFunction = NULL; // should be a private io completition function inside us
            APCContext = lpCompletionRoutine;
            SendInfo.AfdFlags = AFD_SKIP_FIO;
        }

        IOSB = (PIO_STATUS_BLOCK)&lpOverlapped->Internal;
        SendInfo.AfdFlags |= AFD_OVERLAPPED;
    }

    /* Send IOCTL */
    Status = NtDeviceIoControlFile((HANDLE)Handle,
             Event ? Event : SockEvent,
             APCFunction,
             APCContext,
             IOSB,
             IOCTL_AFD_SEND_DATAGRAM,
             &SendInfo,
             sizeof(SendInfo),
             NULL,
             0);

    /* Wait for completition of not overlapped */
    if (Status == STATUS_PENDING && lpOverlapped == NULL)
    {
        WaitForSingleObject(SockEvent, INFINITE); // BUGBUG, shouldn wait infintely for send...
        Status = IOSB->Status;
    }

    NtClose( SockEvent );

    if (Status == STATUS_PENDING)
        return WSA_IO_PENDING;


    /* Re-enable Async Event */
    SockReenableAsyncSelectEvent(Socket, FD_WRITE);

    return MsafdReturnWithErrno ( Status, lpErrno, IOSB->Information, lpNumberOfBytesSent );
}
INT
WSPAPI
WSPRecvDisconnect(IN  SOCKET s,
                  OUT LPWSABUF lpInboundDisconnectData,
                  OUT LPINT lpErrno)
{
    UNIMPLEMENTED
    return 0;
}



INT
WSPAPI
WSPSendDisconnect(IN  SOCKET s,
                  IN  LPWSABUF lpOutboundDisconnectData,
                  OUT LPINT lpErrno)
{
    UNIMPLEMENTED
    return 0;
}

/* EOF */
