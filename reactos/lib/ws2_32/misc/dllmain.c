/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 DLL
 * FILE:        misc/dllmain.c
 * PURPOSE:     DLL entry point
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/09-2000 Created
 */
#include <ws2_32.h>
#include <catalog.h>
#include <upcall.h>

#ifdef DBG

/* See debug.h for debug/trace constants */
DWORD DebugTraceLevel = MIN_TRACE;

#endif /* DBG */

/* To make the linker happy */
VOID STDCALL KeBugCheck (ULONG	BugCheckCode) {}


HANDLE GlobalHeap;
WSPUPCALLTABLE UpcallTable;


INT
EXPORT
WSAGetLastError(VOID)
{
    PWINSOCK_THREAD_BLOCK p = NtCurrentTeb()->WinSockData;

    if (p) {
        return p->LastErrorValue;
    } else {
        /* FIXME: What error code should we use here? Can this even happen? */
        return ERROR_BAD_ENVIRONMENT;
    }
}


VOID
EXPORT
WSASetLastError(
    IN  INT iError)
{
    PWINSOCK_THREAD_BLOCK p = NtCurrentTeb()->WinSockData;

    if (p)
        p->LastErrorValue = iError;
}


INT
EXPORT
WSAStartup(
    IN  WORD wVersionRequested,
    OUT LPWSADATA lpWSAData)
{
    WS_DbgPrint(MIN_TRACE, ("WSAStartup of ws2_32.dll\n"));

    lpWSAData->wVersion     = wVersionRequested;
    lpWSAData->wHighVersion = 2;
    lstrcpyA(lpWSAData->szDescription, "WinSock 2.0");
    lstrcpyA(lpWSAData->szSystemStatus, "Running");
    lpWSAData->iMaxSockets  = 0;
    lpWSAData->iMaxUdpDg    = 0;
    lpWSAData->lpVendorInfo = NULL;

    WSASETINITIALIZED;

    return NO_ERROR;
}


INT
EXPORT
WSACleanup(VOID)
{
    WS_DbgPrint(MIN_TRACE, ("WSACleanup of ws2_32.dll\n"));

    if (!WSAINITIALIZED) {
        WSASetLastError(WSANOTINITIALISED);
        return WSANOTINITIALISED;
    }

    return NO_ERROR;
}


SOCKET
EXPORT
WSASocketA(
    IN  INT af,
    IN  INT type,
    IN  INT protocol,
    IN  LPWSAPROTOCOL_INFOA lpProtocolInfo,
    IN  GROUP g,
    IN  DWORD dwFlags)
/*
 * FUNCTION: Creates a new socket
 */
{
    WSAPROTOCOL_INFOW ProtocolInfoW;
    LPWSAPROTOCOL_INFOW p;
    UNICODE_STRING StringU;
    ANSI_STRING StringA;

    if (lpProtocolInfo) {
        memcpy(&ProtocolInfoW,
               lpProtocolInfo,
               sizeof(WSAPROTOCOL_INFOA) -
               sizeof(CHAR) * (WSAPROTOCOL_LEN + 1));

        RtlInitAnsiString(&StringA, (LPSTR)lpProtocolInfo->szProtocol);
        RtlInitUnicodeString(&StringU, (LPWSTR)&ProtocolInfoW.szProtocol);
        RtlAnsiStringToUnicodeString(&StringU,
		     	                     &StringA,
			     	                 FALSE);
        p = &ProtocolInfoW;
    } else {
        p = NULL;
    }

    return WSASocketW(af,
                      type,
                      protocol,
                      p,
                      g,
                      dwFlags);
}


SOCKET
EXPORT
WSASocketW(
    IN  INT af,
    IN  INT type,
    IN  INT protocol,
    IN  LPWSAPROTOCOL_INFOW lpProtocolInfo,
    IN  GROUP g,
    IN  DWORD dwFlags)
/*
 * FUNCTION: Creates a new socket
 * ARGUMENTS:
 *     af             = Address family
 *     type           = Socket type
 *     protocol       = Protocol type
 *     lpProtocolInfo = Pointer to protocol information
 *     g              = Reserved
 *     dwFlags        = Socket flags
 * RETURNS:
 *     Created socket, or INVALID_SOCKET if it could not be created
 */
{
    INT Status;
    SOCKET Socket;
    PCATALOG_ENTRY Provider;
    WSAPROTOCOL_INFOW ProtocolInfo;

    if (!WSAINITIALIZED) {
        WSASetLastError(WSANOTINITIALISED);
        return INVALID_SOCKET;
    }

    if (!lpProtocolInfo) {
        lpProtocolInfo = &ProtocolInfo;
        ZeroMemory(&ProtocolInfo, sizeof(WSAPROTOCOL_INFOW));

        ProtocolInfo.iAddressFamily = af;
        ProtocolInfo.iSocketType    = type;
        ProtocolInfo.iProtocol      = protocol;
    }

    Provider = LocateProvider(lpProtocolInfo);
    if (!Provider) {
        WSASetLastError(WSAEAFNOSUPPORT);
        return INVALID_SOCKET;
    }

    Status = LoadProvider(Provider, lpProtocolInfo);

    if (Status != NO_ERROR) {
        WSASetLastError(Status);
        return INVALID_SOCKET;
    }

    Socket = Provider->ProcTable.lpWSPSocket(af,
                                             type,
                                             protocol,
                                             lpProtocolInfo,
                                             g,
                                             dwFlags,
                                             &Status);
    if (Status != NO_ERROR) {
        WSASetLastError(Status);
        return INVALID_SOCKET;
    }

    return Socket;
}


BOOL
STDCALL
DllMain(PVOID hInstDll,
        ULONG dwReason,
        PVOID Reserved)
{
    WS_DbgPrint(MIN_TRACE, ("DllMain of ws2_32.dll\n"));

#if 0
    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        GlobalHeap = HeapCreate(0, 0, 0);
        if (!GlobalHeap)
            return FALSE;

        if (ReadCatalog() != NO_ERROR)
            return FALSE;

        UpcallTable.lpWPUCloseEvent         = WPUCloseEvent;
        UpcallTable.lpWPUCloseSocketHandle  = WPUCloseSocketHandle;
        UpcallTable.lpWPUCreateEvent        = WPUCreateEvent;
        UpcallTable.lpWPUCreateSocketHandle = WPUCreateSocketHandle;
        UpcallTable.lpWPUFDIsSet            = WPUFDIsSet;
        UpcallTable.lpWPUGetProviderPath    = WPUGetProviderPath;
        UpcallTable.lpWPUModifyIFSHandle    = WPUModifyIFSHandle;
        UpcallTable.lpWPUPostMessage        = WPUPostMessage;
        UpcallTable.lpWPUQueryBlockingCallback    = WPUQueryBlockingCallback;
        UpcallTable.lpWPUQuerySocketHandleContext = WPUQuerySocketHandleContext;
        UpcallTable.lpWPUQueueApc           = WPUQueueApc;
        UpcallTable.lpWPUResetEvent         = WPUResetEvent;
        UpcallTable.lpWPUSetEvent           = WPUSetEvent;
        UpcallTable.lpWPUOpenCurrentThread  = WPUOpenCurrentThread;
        UpcallTable.lpWPUCloseThread        = WPUCloseThread;
        break;

    case DLL_THREAD_ATTACH: {
        PWINSOCK_THREAD_BLOCK p;

        p = HeapAlloc(GlobalHeap, 0, sizeof(WINSOCK_THREAD_BLOCK));
        if (p) {
            p->LastErrorValue = NO_ERROR;
            p->Initialized    = FALSE;
            NtCurrentTeb()->WinSockData = p;
        }

        break;
    }

    case DLL_THREAD_DETACH: {
        PWINSOCK_THREAD_BLOCK p = NtCurrentTeb()->WinSockData;

        if (p)
            HeapFree(GlobalHeap, 0, p);

        break;
    }

    case DLL_PROCESS_DETACH:
        DestroyCatalog();
        HeapDestroy(GlobalHeap);
        break;
    }
#endif
    return TRUE;
}

/* EOF */
