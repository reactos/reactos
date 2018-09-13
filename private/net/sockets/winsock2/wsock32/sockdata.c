/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    SockData.c

Abstract:

    This module contains global variable declarations for the WinSock
    DLL.

Author:

    David Treadwell (davidtr)    20-Feb-1992

Revision History:

--*/

#include "winsockp.h"

LIST_ENTRY SocketListHead = { NULL };

HMODULE SockModuleHandle = NULL;

CRITICAL_SECTION  SocketLock = { NULL };
PCRITICAL_SECTION pSocketLock = NULL;
CRITICAL_SECTION  csRnRLock = { NULL };
PCRITICAL_SECTION pcsRnRLock = NULL;

#if !defined(USE_TEB_FIELD)
DWORD SockTlsSlot = 0xFFFFFFFF;
#endif  // !USE_TEB_FIELD

BOOLEAN SockAsyncThreadInitialized = FALSE;
LIST_ENTRY SockAsyncQueueHead = { NULL };
HANDLE SockAsyncQueueEvent = NULL;

DWORD SockCurrentTaskHandle = 1;
DWORD SockCurrentAsyncThreadTaskHandle = 0;
DWORD SockCancelledAsyncTaskHandle = 0;

DWORD SockSocketSerialNumberCounter = 1;

DWORD SockWsaStartupCount = 0;
BOOLEAN SockTerminating = FALSE;
BOOLEAN SockProcessTerminating = FALSE;

LIST_ENTRY SockHelperDllListHead = { NULL };

PWINSOCK_POST_ROUTINE SockPostRoutine = NULL;

DWORD SockSendBufferWindow = 0;
DWORD SockReceiveBufferWindow = 0;

PVOID SockPrivateHeap = NULL;

#if DBG
ULONG WsDebug = 0;
#endif

