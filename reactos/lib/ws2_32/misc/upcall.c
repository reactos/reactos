/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 DLL
 * FILE:        misc/upcall.c
 * PURPOSE:     Upcall functions
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/09-2000 Created
 */
#include <ws2_32.h>
#include <catalog.h>

BOOL
WSPAPI
WPUCloseEvent(
    IN  WSAEVENT hEvent,
    OUT LPINT lpErrno)
{
    UNIMPLEMENTED

    return FALSE;
}


INT
WSPAPI
WPUCloseSocketHandle(
    IN  SOCKET s,
    OUT LPINT lpErrno)
{
    UNIMPLEMENTED

    return 0;
}


INT
WSPAPI
WPUCloseThread(
    IN  LPWSATHREADID lpThreadId,
    OUT LPINT lpErrno)
{
    UNIMPLEMENTED

    return 0;
}


WSAEVENT
WSPAPI
WPUCreateEvent(
    OUT LPINT lpErrno)
{
    UNIMPLEMENTED

    return (WSAEVENT)0;
}


SOCKET
WSPAPI
WPUCreateSocketHandle(
    IN  DWORD dwCatalogEntryId,
    IN  DWORD dwContext,
    OUT LPINT lpErrno)
{
    UNIMPLEMENTED

    return (SOCKET)0;
}


SOCKET
WSPAPI
WPUFDIsSet(
    IN  SOCKET s,
    IN  LPFD_SET set)
{
    UNIMPLEMENTED

    return (SOCKET)0;
}


INT
WSPAPI
WPUGetProviderPath(
    IN      LPGUID lpProviderId,
    OUT     LPWSTR lpszProviderDllPath,
    IN OUT  LPINT lpProviderDllPathLen,
    OUT     LPINT lpErrno)
{
    UNIMPLEMENTED

    return 0;
}


SOCKET
WSPAPI
WPUModifyIFSHandle(
    IN  DWORD dwCatalogEntryId,
    IN  SOCKET ProposedHandle,
    OUT LPINT lpErrno)
{
    PCATALOG_ENTRY Provider;
    SOCKET Socket;

    WS_DbgPrint(MAX_TRACE, ("dwCatalogEntryId (%d)  ProposedHandle (0x%X).\n",
        dwCatalogEntryId, ProposedHandle));

    Provider = LocateProviderById(dwCatalogEntryId);
    if (!Provider) {
        WS_DbgPrint(MIN_TRACE, ("Provider with catalog entry id (%d) was not found.\n",
            dwCatalogEntryId));
        *lpErrno = WSAEINVAL;
        return INVALID_SOCKET;
    }

    Socket = (SOCKET)CreateProviderHandle(ProposedHandle, Provider);

    *lpErrno = NO_ERROR;

    return Socket;
}


INT
WSPAPI
WPUOpenCurrentThread(
    OUT LPWSATHREADID lpThreadId,
    OUT LPINT lpErrno)
{
    UNIMPLEMENTED

    return 0;
}


BOOL
WSPAPI
WPUPostMessage(
    IN  HWND hWnd,
    IN  UINT Msg,
    IN  WPARAM wParam,
    IN  LPARAM lParam)
{
    UNIMPLEMENTED

    return FALSE;
}


INT
WSPAPI
WPUQueryBlockingCallback(
    IN  DWORD dwCatalogEntryId,
    OUT LPBLOCKINGCALLBACK FAR* lplpfnCallback,
    OUT LPDWORD lpdwContext,
    OUT LPINT lpErrno)
{
    UNIMPLEMENTED

    return 0;
}


INT
WSPAPI
WPUQuerySocketHandleContext(
    IN  SOCKET s,
    OUT LPDWORD lpContext,
    OUT LPINT lpErrno)
{
    UNIMPLEMENTED

    return 0;
}


INT
WSPAPI
WPUQueueApc(
    IN  LPWSATHREADID lpThreadId,
    IN  LPWSAUSERAPC lpfnUserApc,
    IN  DWORD dwContext,
    OUT LPINT lpErrno)
{
    UNIMPLEMENTED

    return 0;
}


BOOL
WSPAPI
WPUResetEvent(
    IN  WSAEVENT hEvent,
    OUT LPINT lpErrno)
{
    UNIMPLEMENTED

    return FALSE;
}


BOOL
WSPAPI
WPUSetEvent(
    IN  WSAEVENT hEvent,
    OUT LPINT lpErrno)
{
    UNIMPLEMENTED

    return FALSE;
}

/* EOF */
