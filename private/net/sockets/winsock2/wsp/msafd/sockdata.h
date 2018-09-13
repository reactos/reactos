/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    SockData.h

Abstract:

    This module contains global variable declarations for the WinSock
    DLL.

Author:

    David Treadwell (davidtr)    20-Feb-1992

Revision History:

--*/

#ifndef _SOCKDATA_
#define _SOCKDATA_

extern SOCK_RW_LOCK SocketGlobalLock;


extern LPCONTEXT_TABLE SockContextTable;

#if !defined(USE_TEB_FIELD)
extern DWORD SockTlsSlot;
#endif  // !USE_TEB_FIELD

extern LONG SockAsyncThreadReferenceCount;
extern HANDLE SockAsyncQueuePort;
extern BOOLEAN SockAsyncSelectCalled;
extern HANDLE SockAsyncSelectHelperHandle;
extern HANDLE SockAsyncConnectHelperHandle;

extern HMODULE SockModuleHandle;

extern DWORD SockWspStartupCount;
//extern BOOLEAN SockTerminating;
extern BOOLEAN SockProcessTerminating;

extern BOOL SockProductTypeWorkstation;

#ifdef _AFD_SAN_SWITCH_
extern BOOLEAN SockSanEnabled;
extern HANDLE SockSanHelperHandle;

extern LIST_ENTRY      SockSanProviderList;

extern DWORD NumOpenSanSocks;
extern LIST_ENTRY SockSanOpenList;
extern LIST_ENTRY SockSanClosingList;
extern LIST_ENTRY SockSanTrackerList;
extern CRITICAL_SECTION SockSanListCritSec;
extern BOOLEAN SockSanDeleteListCritSec;

extern LONG SockSanConnectThreadCreated;
extern LONG SockSanFlowThreadCreated;
extern BOOL RdmaReadSupported;
extern BOOL RdmaEnabled;
extern DWORD   FCEntrySize;
extern DWORD RdmaThreshold;

extern PVOID           SockSanAddressNotifyContext;

extern INT TcpBypassFlag;

// TCP IP provider info to pass to SAN providers on WSPStartupEx and WSPSocket calls
extern WSAPROTOCOL_INFOW  SockTcpProviderInfo;

extern LONG SockSanSocksDoingDup;


#endif //_AFD_SAN_SWITCH_


extern DWORD SockSendBufferWindow;
extern DWORD SockReceiveBufferWindow;

extern PVOID SockPrivateHeap;

extern WSPUPCALLTABLE *SockUpcallTable;
extern WSPUPCALLTABLE SockUpcallTableHack;
extern WSPPROC_TABLE SockProcTable;


#if DBG
extern ULONG WsDebug;
#endif

extern LIST_ENTRY SockHelperDllListHead;
extern LPWSAPROTOCOL_INFOW SockProtocolInfoArray;
extern INT SockProtocolInfoCount;


extern LONG SockProcessPendingAPCCount;
#endif // ndef _SOCKDATA_
