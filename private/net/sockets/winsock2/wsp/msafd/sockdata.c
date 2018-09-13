/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    sockdata.c

Abstract:

    This module contains global variable declarations for the WinSock
    DLL.

Author:

    David Treadwell (davidtr)    20-Feb-1992

Revision History:

--*/

#include "winsockp.h"

SOCK_RW_LOCK SocketGlobalLock = { 0 };

#if !defined(USE_TEB_FIELD)
DWORD SockTlsSlot = 0xFFFFFFFF;
#endif  // !USE_TEB_FIELD

LONG SockAsyncThreadReferenceCount = 0;
HANDLE SockAsyncQueuePort = NULL;
BOOLEAN SockAsyncSelectCalled = FALSE;
HANDLE SockAsyncSelectHelperHandle = NULL;
HANDLE SockAsyncConnectHelperHandle = NULL;

HMODULE SockModuleHandle = NULL;

DWORD SockWspStartupCount = 0;
BOOLEAN SockProcessTerminating = FALSE;

BOOL SockProductTypeWorkstation = FALSE;
#ifdef _AFD_SAN_SWITCH_
BOOLEAN SockSanEnabled = FALSE;
HANDLE SockSanHelperHandle;

#if DBG
ULONG SanDebug = 0;
#endif

DWORD NumOpenSanSocks;
LIST_ENTRY SockSanOpenList;
LIST_ENTRY SockSanClosingList;
LIST_ENTRY SockSanTrackerList;
CRITICAL_SECTION SockSanListCritSec;
BOOLEAN SockSanDeleteListCritSec;
BOOL RdmaReadSupported;
BOOL RdmaEnabled = TRUE;
DWORD   FCEntrySize;			// Size of one FC entry (including RDMA handle) 
DWORD RdmaThreshold;			// in number of small message buffers

LONG SockSanConnectThreadCreated = 0;
LONG SockSanFlowThreadCreated = 0;

// TCP IP provider info to pass to SAN providers on WSPStartupEx and WSPSocket calls
WSAPROTOCOL_INFOW   SockTcpProviderInfo = {
            XP1_GUARANTEED_DELIVERY                 // dwServiceFlags1
                | XP1_GUARANTEED_ORDER
                | XP1_GRACEFUL_CLOSE
                | XP1_EXPEDITED_DATA
                | XP1_IFS_HANDLES,
            0,                                      // dwServiceFlags2
            0,                                      // dwServiceFlags3
            0,                                      // dwServiceFlags4
            PFL_MATCHES_PROTOCOL_ZERO,              // dwProviderFlags
            { /* e70f1aa0-ab8b-11cf-8ca3-00805f48a192 */
                0xe70f1aa0,
                0xab8b,
                0x11cf,
                {0x8c, 0xa3, 0x00, 0x80, 0x5f, 0x48, 0xa1, 0x92}
            },
            0,                                      // dwCatalogEntryId
            {                                       // ProtocolChain
                BASE_PROTOCOL,                          // ChainLen
                { 0, 0, 0, 0, 0, 0, 0 }                 // ChainEntries
            },
            2,                                      // iVersion
            AF_INET,                                // iAddressFamily
            sizeof(SOCKADDR_IN),                    // iMaxSockAddr
            sizeof(SOCKADDR_IN),                    // iMinSockAddr
            SOCK_STREAM,                            // iSocketType
            IPPROTO_TCP,                            // iProtocol
            0,                                      // iProtocolMaxOffset
            BIGENDIAN,                              // iNetworkByteOrder
            SECURITY_PROTOCOL_NONE,                 // iSecurityScheme
            0,                                      // dwMessageSize
            0,                                      // dwProviderReserved
            L"MSAFD Tcpip [TCP/IP]"                 // szProtocol
        };
LONG SockSanSocksDoingDup = 0;

#endif //_AFD_SAN_SWITCH_

DWORD SockSendBufferWindow = 0;
DWORD SockReceiveBufferWindow = 0;

PVOID SockPrivateHeap = NULL;

WSPUPCALLTABLE *SockUpcallTable;
WSPUPCALLTABLE SockUpcallTableHack;


//
// The dispatch table used by the main WinSock 2 DLL to invoke
// our services.
//

WSPPROC_TABLE SockProcTable = {

        &WSPAccept,
        &WSPAddressToString,
        &WSPAsyncSelect,
        &WSPBind,
        &WSPCancelBlockingCall,
        &WSPCleanup,
        &WSPCloseSocket,
        &WSPConnect,
        &WSPDuplicateSocket,
        &WSPEnumNetworkEvents,
        &WSPEventSelect,
        &WSPGetOverlappedResult,
        &WSPGetPeerName,
        &WSPGetSockName,
        &WSPGetSockOpt,
        &WSPGetQOSByName,
        &WSPIoctl,
        &WSPJoinLeaf,
        &WSPListen,
        &WSPRecv,
        &WSPRecvDisconnect,
        &WSPRecvFrom,
        &WSPSelect,
        &WSPSend,
        &WSPSendDisconnect,
        &WSPSendTo,
        &WSPSetSockOpt,
        &WSPShutdown,
        &WSPSocket,
        &WSPStringToAddress

    };

LPCONTEXT_TABLE SockContextTable;

#if DBG
ULONG WsDebug = 0;
#endif


LIST_ENTRY SockHelperDllListHead = { NULL };
LPWSAPROTOCOL_INFOW SockProtocolInfoArray = { NULL };
INT SockProtocolInfoCount = 0;

LONG SockProcessPendingAPCCount = 0;
