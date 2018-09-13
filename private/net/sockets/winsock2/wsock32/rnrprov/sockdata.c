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

#include <winsockp.h>


CRITICAL_SECTION  SocketLock = { NULL };
PCRITICAL_SECTION pSocketLock = NULL;
CRITICAL_SECTION  csRnRLock = { NULL };
PCRITICAL_SECTION pcsRnRLock = NULL;

#if !defined(USE_TEB_FIELD)
DWORD SockTlsSlot = 0xFFFFFFFF;
#endif  // !USE_TEB_FIELD



DWORD SockWsaStartupCount = 0;
BOOLEAN SockTerminating = FALSE;
BOOLEAN SockProcessTerminating = FALSE;


PVOID SockPrivateHeap = NULL;

#if DBG
ULONG WsDebug = 0;
#endif

