/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 Helper DLL for TCP/IP
 * FILE:        ws2help.c
 * PURPOSE:     DLL entry
 * PROGRAMMERS: Robert D. Dickenson (robertdickenson@users.sourceforge.net)
 * REVISIONS:
 *   RDD 18/06-2002 Created
 */
#include "ws2help.h"

#ifdef DBG

/* See debug.h for debug/trace constants */
DWORD DebugTraceLevel = MAX_TRACE;

#endif /* DBG */

/* To make the linker happy */
VOID STDCALL KeBugCheck (ULONG	BugCheckCode) {}


BOOL
EXPORT
DllMain(HANDLE hInstDll,
        ULONG dwReason,
        PVOID Reserved)
{
    WSH_DbgPrint(MIN_TRACE, ("DllMain of ws2help.dll\n"));

    switch (dwReason) {
    case DLL_PROCESS_ATTACH:
        /* Don't need thread attach notifications
           so disable them to improve performance */
        DisableThreadLibraryCalls(hInstDll);
        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}


INT
EXPORT
WahCloseApcHelper(
    IN  PVOID HelperDllSocketContext,
    IN  SOCKET SocketHandle
    )
{
    UNIMPLEMENTED

    return 0;
}

INT
EXPORT
WahCloseHandleHelper(
    IN  PVOID HelperDllSocketContext,
    IN  SOCKET SocketHandle
    )
{
    UNIMPLEMENTED

    return 0;
}

INT
EXPORT
WahCloseNotificationHelper(
    IN  PVOID HelperDllSocketContext,
    IN  SOCKET SocketHandle
    )
{
    UNIMPLEMENTED

    return 0;
}

INT
EXPORT
WahCloseSocketHandle(
    IN  PVOID HelperDllSocketContext,
    IN  SOCKET SocketHandle
    )
{
    UNIMPLEMENTED

    return 0;
}

INT
EXPORT
WahCloseThread(
    IN  PVOID HelperDllSocketContext,
    IN  SOCKET SocketHandle
    )
{
    UNIMPLEMENTED

    return 0;
}

INT
EXPORT
WahCompleteRequest(
    IN  PVOID HelperDllSocketContext,
    IN  SOCKET SocketHandle
    )
{
    UNIMPLEMENTED

    return 0;
}

INT
EXPORT
WahCreateHandleContextTable(
    IN  PVOID HelperDllSocketContext,
    IN  SOCKET SocketHandle
    )
{
    UNIMPLEMENTED

    return 0;
}

INT
EXPORT
WahCreateNotificationTable(
    IN  PVOID HelperDllSocketContext,
    IN  SOCKET SocketHandle
    )
{
    UNIMPLEMENTED

    return 0;
}

INT
EXPORT
WahCreateSocketHandle(
    IN  PVOID HelperDllSocketContext,
    IN  SOCKET SocketHandle
    )
{
    UNIMPLEMENTED

    return 0;
}

INT
EXPORT
WahDestroyHandleContextTable(
    IN  PVOID HelperDllSocketContext,
    IN  SOCKET SocketHandle
    )
{
    UNIMPLEMENTED

    return 0;
}

INT
EXPORT
WahDisableNonIFSHandleSupport(
    IN  PVOID HelperDllSocketContext,
    IN  SOCKET SocketHandle
    )
{
    UNIMPLEMENTED

    return 0;
}

INT
EXPORT
WahEnableNonIFSHandleSupport(
    IN  PVOID HelperDllSocketContext,
    IN  SOCKET SocketHandle
    )
{
    UNIMPLEMENTED

    return 0;
}

INT
EXPORT
WahEnumerateHandleContexts(
    IN  PVOID HelperDllSocketContext,
    IN  SOCKET SocketHandle
    )
{
    UNIMPLEMENTED

    return 0;
}

INT
EXPORT
WahInsertHandleContext(
    IN  PVOID HelperDllSocketContext,
    IN  SOCKET SocketHandle
    )
{
    UNIMPLEMENTED

    return 0;
}

INT
EXPORT
WahNotifyAllProcesses(
    IN  PVOID HelperDllSocketContext,
    IN  SOCKET SocketHandle
    )
{
    UNIMPLEMENTED

    return 0;
}

INT
EXPORT
WahOpenApcHelper(
    IN  PVOID HelperDllSocketContext,
    IN  SOCKET SocketHandle
    )
{
    UNIMPLEMENTED

    return 0;
}

INT
EXPORT
WahOpenCurrentThread(
    IN  PVOID HelperDllSocketContext,
    IN  SOCKET SocketHandle
    )
{
    UNIMPLEMENTED

    return 0;
}

INT
EXPORT
WahOpenHandleHelper(
    IN  PVOID HelperDllSocketContext,
    IN  SOCKET SocketHandle
    )
{
    UNIMPLEMENTED

    return 0;
}

INT
EXPORT
WahOpenNotificationHandleHelper(
    IN  PVOID HelperDllSocketContext,
    IN  SOCKET SocketHandle
    )
{
    UNIMPLEMENTED

    return 0;
}

INT
EXPORT
WahQueueUserApc(
    IN  PVOID HelperDllSocketContext,
    IN  SOCKET SocketHandle
    )
{
    UNIMPLEMENTED

    return 0;
}

INT
EXPORT
WahReferenceContextByHandle(
    IN  PVOID HelperDllSocketContext,
    IN  SOCKET SocketHandle
    )
{
    UNIMPLEMENTED

    return 0;
}

INT
EXPORT
WahRemoveHandleContext(
    IN  PVOID HelperDllSocketContext,
    IN  SOCKET SocketHandle
    )
{
    UNIMPLEMENTED

    return 0;
}

INT
EXPORT
WahWaitForNotification(
    IN  PVOID HelperDllSocketContext,
    IN  SOCKET SocketHandle
    )
{
    UNIMPLEMENTED

    return 0;
}

/* EOF */
