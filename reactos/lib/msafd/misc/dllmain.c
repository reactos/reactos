/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Ancillary Function Driver DLL
 * FILE:        misc/dllmain.c
 * PURPOSE:     DLL entry point
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/09-2000 Created
 */
#include <msafd.h>
#include <helpers.h>

#ifdef DBG

/* See debug.h for debug/trace constants */
DWORD DebugTraceLevel = MIN_TRACE;

#endif /* DBG */

/* To make the linker happy */
VOID STDCALL KeBugCheck (ULONG	BugCheckCode) {}


HANDLE GlobalHeap;
WSPUPCALLTABLE Upcalls;
LPWPUCOMPLETEOVERLAPPEDREQUEST lpWPUCompleteOverlappedRequest;
CRITICAL_SECTION InitCriticalSection;
DWORD StartupCount = 0;

NTSTATUS OpenSocket(
    SOCKET *Socket,
    INT AddressFamily,
    INT SocketType,
    INT Protocol,
    PVOID HelperContext,
    DWORD NotificationEvents,
    PUNICODE_STRING TdiDeviceName)
/*
 * FUNCTION: Opens a socket
 * ARGUMENTS:
 *     Socket             = Address of buffer to place socket descriptor
 *     AddressFamily      = Address family
 *     SocketType         = Type of socket
 *     Protocol           = Protocol type
 *     HelperContext      = Pointer to context information for helper DLL
 *     NotificationEvents = Events for which helper DLL is to be notified
 *     TdiDeviceName      = Pointer to name of TDI device to use
 * RETURNS:
 *     Status of operation
 */
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    PAFD_SOCKET_INFORMATION SocketInfo;
    PFILE_FULL_EA_INFORMATION EaInfo;
    UNICODE_STRING DeviceName;
    IO_STATUS_BLOCK Iosb;
    HANDLE FileHandle;
    NTSTATUS Status;
    ULONG EaLength;
    ULONG EaShort;

    AFD_DbgPrint(MAX_TRACE, ("Called.\n"));

    EaShort = sizeof(FILE_FULL_EA_INFORMATION) +
        AFD_SOCKET_LENGTH +
        sizeof(AFD_SOCKET_INFORMATION);

    EaLength = EaShort + TdiDeviceName->Length + sizeof(WCHAR);

    EaInfo = (PFILE_FULL_EA_INFORMATION)HeapAlloc(GlobalHeap, 0, EaLength);
    if (!EaInfo) {
        AFD_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(EaInfo, EaLength);
    EaInfo->EaNameLength = AFD_SOCKET_LENGTH;
    RtlCopyMemory(EaInfo->EaName,
                  AfdSocket,
                  AFD_SOCKET_LENGTH);
    EaInfo->EaValueLength = sizeof(AFD_SOCKET_INFORMATION);

    SocketInfo = (PAFD_SOCKET_INFORMATION)(EaInfo->EaName + AFD_SOCKET_LENGTH);

    SocketInfo->AddressFamily      = AddressFamily;
    SocketInfo->SocketType         = SocketType;
    SocketInfo->Protocol           = Protocol;
    SocketInfo->HelperContext      = HelperContext;
    SocketInfo->NotificationEvents = NotificationEvents;
    /* Zeroed above so initialized to a wildcard address if a raw socket */
    SocketInfo->Name.sa_family     = AddressFamily;

    /* Store TDI device name last in buffer */
    SocketInfo->TdiDeviceName.Buffer = (PWCHAR)(EaInfo + EaShort);
    SocketInfo->TdiDeviceName.MaximumLength = TdiDeviceName->Length + sizeof(WCHAR);
    RtlCopyUnicodeString(&SocketInfo->TdiDeviceName, TdiDeviceName);

    AFD_DbgPrint(MAX_TRACE, ("EaInfo at (0x%X)  EaLength is (%d).\n", (UINT)EaInfo, (INT)EaLength));


    RtlInitUnicodeString(&DeviceName, L"\\Device\\Afd");
	  InitializeObjectAttributes(&ObjectAttributes,
        &DeviceName,
        0,
        NULL,
        NULL);

    Status = NtCreateFile(&FileHandle,
        FILE_GENERIC_READ | FILE_GENERIC_WRITE,
        &ObjectAttributes,
        &Iosb,
        NULL,
		    0,
		    0,
		    FILE_OPEN,
		    FILE_SYNCHRONOUS_IO_ALERT,
        EaInfo,
        EaLength);

    HeapFree(GlobalHeap, 0, EaInfo);

    if (!NT_SUCCESS(Status)) {
		  AFD_DbgPrint(MIN_TRACE, ("Error opening device (Status 0x%X).\n", (UINT)Status));
      return STATUS_INSUFFICIENT_RESOURCES;
    }

    *Socket = (SOCKET)FileHandle;

    return STATUS_SUCCESS;
}


SOCKET
WSPAPI
WSPSocket(
    IN  INT af,
    IN  INT type,
    IN  INT protocol,
    IN  LPWSAPROTOCOL_INFOW lpProtocolInfo,
    IN  GROUP g,
    IN  DWORD dwFlags,
    OUT LPINT lpErrno)
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
{
    WSAPROTOCOL_INFOW ProtocolInfo;
    UNICODE_STRING TdiDeviceName;
    DWORD NotificationEvents;
    PWSHELPER_DLL HelperDLL;
    PVOID HelperContext;
    INT AddressFamily;
    NTSTATUS NtStatus;
    INT SocketType;
    SOCKET Socket2;
    SOCKET Socket;
    INT Protocol;
    INT Status;

    AFD_DbgPrint(MAX_TRACE, ("af (%d)  type (%d)  protocol (%d).\n",
        af, type, protocol));

    if (!lpProtocolInfo) {
        CP
        lpProtocolInfo = &ProtocolInfo;
        ZeroMemory(&ProtocolInfo, sizeof(WSAPROTOCOL_INFOW));

        ProtocolInfo.iAddressFamily = af;
        ProtocolInfo.iSocketType    = type;
        ProtocolInfo.iProtocol      = protocol;
    }

    HelperDLL = LocateHelperDLL(lpProtocolInfo);
    if (!HelperDLL) {
        *lpErrno = WSAEAFNOSUPPORT;
        return INVALID_SOCKET;
    }

    AddressFamily = lpProtocolInfo->iAddressFamily;
    SocketType    = lpProtocolInfo->iSocketType;
    Protocol      = lpProtocolInfo->iProtocol;

    Status = HelperDLL->EntryTable.lpWSHOpenSocket2(&AddressFamily,
        &SocketType,
        &Protocol,
        0,
        0,
        &TdiDeviceName,
        &HelperContext,
        &NotificationEvents);
    if (Status != NO_ERROR) {
        *lpErrno = Status;
        return INVALID_SOCKET;
    }

    NtStatus = OpenSocket(&Socket,
        AddressFamily,
        SocketType,
        Protocol,
        HelperContext,
        NotificationEvents,
        &TdiDeviceName);

    RtlFreeUnicodeString(&TdiDeviceName);
    if (!NT_SUCCESS(NtStatus)) {
        CP
        *lpErrno = RtlNtStatusToDosError(Status);
        return INVALID_SOCKET;
    }

    /* FIXME: Assumes catalog entry id to be 1 */
    Socket2 = Upcalls.lpWPUModifyIFSHandle(1, Socket, lpErrno);

    if (Socket2 == INVALID_SOCKET) {
        /* FIXME: Cleanup */
        AFD_DbgPrint(MIN_TRACE, ("FIXME: Cleanup.\n"));
        return INVALID_SOCKET;
    }

    *lpErrno = NO_ERROR;

    AFD_DbgPrint(MID_TRACE, ("Returning socket descriptor (0x%X).\n", Socket2));

    return Socket2;
}


INT
WSPAPI
WSPCloseSocket(
    IN	SOCKET s,
    OUT	LPINT lpErrno)
/*
 * FUNCTION: Closes an open socket
 * ARGUMENTS:
 *     s       = Socket descriptor
 *     lpErrno = Address of buffer for error information
 * RETURNS:
 *     NO_ERROR, or SOCKET_ERROR if the socket could not be closed
 */
{
    NTSTATUS Status;

    AFD_DbgPrint(MAX_TRACE, ("s (0x%X).\n", s));

    Status = NtClose((HANDLE)s);

    if (NT_SUCCESS(Status)) {
        *lpErrno = NO_ERROR;
        return NO_ERROR;
    }

    *lpErrno = WSAENOTSOCK;
    return SOCKET_ERROR;
}


INT
WSPAPI
WSPBind(
    IN  SOCKET s,
    IN  CONST LPSOCKADDR name, 
    IN  INT namelen, 
    OUT LPINT lpErrno)
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
{
  AFD_DbgPrint(MAX_TRACE, ("s (0x%X)  name (0x%X)  namelen (%d).\n", s, name, namelen));

#if 0
  FILE_REQUEST_BIND Request;
  FILE_REPLY_BIND Reply;
  IO_STATUS_BLOCK Iosb;
  NTSTATUS Status;

  RtlCopyMemory(&Request.Name, name, sizeof(SOCKADDR));

  Status = NtDeviceIoControlFile((HANDLE)s,
    NULL,
		NULL,
		NULL,
		&Iosb,
		IOCTL_AFD_BIND,
		&Request,
		sizeof(FILE_REQUEST_BIND),
		&Reply,
		sizeof(FILE_REPLY_BIND));

	if (Status == STATUS_PENDING) {
		if (!NT_SUCCESS(NtWaitForSingleObject((HANDLE)s, FALSE, NULL))) {
      /* FIXME: What error code should be returned? */
			*lpErrno = WSAENOBUFS;
			return SOCKET_ERROR;
		}
  }

  if (!NT_SUCCESS(Status)) {
	  *lpErrno = WSAENOBUFS;
    return SOCKET_ERROR;
	}
#endif
    return 0;
}


INT
WSPAPI
WSPSelect(
    IN      INT nfds,
    IN OUT  LPFD_SET readfds,
    IN OUT  LPFD_SET writefds,
    IN OUT  LPFD_SET exceptfds,
    IN      CONST LPTIMEVAL timeout,
    OUT     LPINT lpErrno)
/*
 * FUNCTION: Returns status of one or more sockets
 * ARGUMENTS:
 *     nfds      = Always ignored
 *     readfds   = Pointer to socket set to be checked for readability (optional)
 *     writefds  = Pointer to socket set to be checked for writability (optional)
 *     exceptfds = Pointer to socket set to be checked for errors (optional)
 *     timeout   = Pointer to a TIMEVAL structure indicating maximum wait time
 *                 (NULL means wait forever)
 *     lpErrno   = Address of buffer for error information
 * RETURNS:
 *     Number of ready socket descriptors, or SOCKET_ERROR if an error ocurred
 */
{
    AFD_DbgPrint(MAX_TRACE, ("readfds (0x%X)  writefds (0x%X)  exceptfds (0x%X).\n",
        readfds, writefds, exceptfds));

    /* FIXME: For now, all reads are timed out immediately */
    if (readfds != NULL) {
        AFD_DbgPrint(MIN_TRACE, ("Timing out read query.\n"));
        *lpErrno = WSAETIMEDOUT;
        return SOCKET_ERROR;
    }

    /* FIXME: For now, always allow write */
    if (writefds != NULL) {
        AFD_DbgPrint(MIN_TRACE, ("Setting one socket writeable.\n"));
        *lpErrno = NO_ERROR;
        return 1;
    }

    *lpErrno = NO_ERROR;

    return 0;
}


INT
WSPAPI
WSPStartup(
    IN  WORD wVersionRequested,
    OUT LPWSPDATA lpWSPData,
    IN  LPWSAPROTOCOL_INFOW lpProtocolInfo,
    IN  WSPUPCALLTABLE UpcallTable,
    OUT LPWSPPROC_TABLE lpProcTable)
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
{
    HMODULE hWS2_32;
    INT Status;

    AFD_DbgPrint(MAX_TRACE, ("wVersionRequested (0x%X) \n", wVersionRequested));

    //EnterCriticalSection(&InitCriticalSection);

    Upcalls = UpcallTable;

    if (StartupCount == 0) {
        /* First time called */

        Status = WSAVERNOTSUPPORTED;

        hWS2_32 = GetModuleHandle(L"ws2_32.dll");

        if (hWS2_32 != NULL) {
            lpWPUCompleteOverlappedRequest = (LPWPUCOMPLETEOVERLAPPEDREQUEST)
                GetProcAddress(hWS2_32, "WPUCompleteOverlappedRequest");

            if (lpWPUCompleteOverlappedRequest != NULL) {
                Status = NO_ERROR;
                StartupCount++;
                CP
            }
        } else {
            AFD_DbgPrint(MIN_TRACE, ("GetModuleHandle() failed for ws2_32.dll\n"));
        }
    } else {
        Status = NO_ERROR;
        StartupCount++;
    }

    //LeaveCriticalSection(&InitCriticalSection);

    AFD_DbgPrint(MIN_TRACE, ("WSPSocket() is at 0x%X\n", WSPSocket));

    if (Status == NO_ERROR) {
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

    AFD_DbgPrint(MIN_TRACE, ("Status (%d).\n", Status));

    return Status;
}


INT
WSPAPI
WSPCleanup(
    OUT LPINT lpErrno)
/*
 * FUNCTION: Cleans up service provider for a client
 * ARGUMENTS:
 *     lpErrno = Address of buffer for error information
 * RETURNS:
 *     0 if successful, or SOCKET_ERROR if not
 */
{
    AFD_DbgPrint(MAX_TRACE, ("\n"));

    //EnterCriticalSection(&InitCriticalSection);

    if (StartupCount > 0) {
        StartupCount--;

        if (StartupCount == 0) {
            AFD_DbgPrint(MAX_TRACE, ("Cleaning up msafd.dll.\n"));
        }
    }

    //LeaveCriticalSection(&InitCriticalSection);

    *lpErrno = NO_ERROR;

    return 0;
}


BOOL
STDCALL
DllMain(HANDLE hInstDll,
        ULONG dwReason,
        PVOID Reserved)
{
    AFD_DbgPrint(MIN_TRACE, ("DllMain of msafd.dll\n"));

    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        /* Don't need thread attach notifications
           so disable them to improve performance */
        DisableThreadLibraryCalls(hInstDll);

        //InitializeCriticalSection(&InitCriticalSection);

        GlobalHeap = GetProcessHeap();
        //GlobalHeap = HeapCreate(0, 0, 0);
        if (!GlobalHeap) {
            AFD_DbgPrint(MIN_TRACE, ("Insufficient memory.\n"));
            return FALSE;
        }

        CreateHelperDLLDatabase();
        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:
        DestroyHelperDLLDatabase();
        //HeapDestroy(GlobalHeap);

        //DeleteCriticalSection(&InitCriticalSection);
        break;
    }

    return TRUE;
}

/* EOF */
