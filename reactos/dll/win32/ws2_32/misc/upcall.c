/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 DLL
 * FILE:        dll/win32/ws2_32/misc/upcall.c
 * PURPOSE:     Upcall functions
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/09-2000 Created
 */

#include "ws2_32.h"

/*
 * @implemented
 */
BOOL
WSPAPI
WPUCloseEvent(IN  WSAEVENT hEvent,
              OUT LPINT lpErrno)
{
    UNIMPLEMENTED

    return FALSE;
}


/*
 * @unimplemented
 */
INT
WSPAPI
WPUCloseSocketHandle(IN  SOCKET s,
                     OUT LPINT lpErrno)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
WSPAPI
WPUCloseThread(IN  LPWSATHREADID lpThreadId,
               OUT LPINT lpErrno)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
WSAEVENT
WSPAPI
WPUCreateEvent(OUT LPINT lpErrno)
{
    UNIMPLEMENTED

    return (WSAEVENT)0;
}


/*
 * @unimplemented
 */
SOCKET
WSPAPI
WPUCreateSocketHandle(IN  DWORD dwCatalogEntryId,
                      IN  DWORD_PTR dwContext,
                      OUT LPINT lpErrno)
{
    UNIMPLEMENTED

    return (SOCKET)0;
}


/*
 * @unimplemented
 */
int
WSPAPI
WPUFDIsSet(IN  SOCKET s,
           IN  LPFD_SET set)
{
    UNIMPLEMENTED

    return (SOCKET)0;
}


/*
 * @unimplemented
 */
INT
WSPAPI
WPUGetProviderPath(IN      LPGUID lpProviderId,
                   OUT     LPWSTR lpszProviderDllPath,
                   IN OUT  LPINT lpProviderDllPathLen,
                   OUT     LPINT lpErrno)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @implemented
 */
SOCKET
WSPAPI
WPUModifyIFSHandle(IN  DWORD dwCatalogEntryId,
                   IN  SOCKET ProposedHandle,
                   OUT LPINT lpErrno)
{
    PCATALOG_ENTRY Provider;
    SOCKET Socket;

    WS_DbgPrint(MID_TRACE, ("dwCatalogEntryId (%d)  ProposedHandle (0x%X).\n",
    dwCatalogEntryId, ProposedHandle));

    Provider = LocateProviderById(dwCatalogEntryId);
    if (!Provider)
    {
        WS_DbgPrint(MIN_TRACE, ("Provider with catalog entry id (%d) was not found.\n",
                    dwCatalogEntryId));
        if( lpErrno ) *lpErrno = WSAEINVAL;
        WS_DbgPrint(MID_TRACE, ("Returning invalid socket\n"));
        return INVALID_SOCKET;
    }

    Socket = (SOCKET)CreateProviderHandle((HANDLE)ProposedHandle,
                                          Provider);

    if( lpErrno ) *lpErrno = NO_ERROR;

    WS_DbgPrint(MID_TRACE, ("Socket: %x\n", Socket));
    return Socket;
}


/*
 * @unimplemented
 */
INT
WSPAPI
WPUOpenCurrentThread(OUT LPWSATHREADID lpThreadId,
                     OUT LPINT lpErrno)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
WSPAPI
WPUQueryBlockingCallback(IN  DWORD dwCatalogEntryId,
                         OUT LPBLOCKINGCALLBACK FAR* lplpfnCallback,
                         OUT PDWORD_PTR lpdwContext,
                         OUT LPINT lpErrno)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
WSPAPI
WPUQuerySocketHandleContext(IN  SOCKET s,
                            OUT PDWORD_PTR lpContext,
                            OUT LPINT lpErrno)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
INT
WSPAPI
WPUQueueApc(IN  LPWSATHREADID lpThreadId,
            IN  LPWSAUSERAPC lpfnUserApc,
            IN  DWORD_PTR dwContext,
            OUT LPINT lpErrno)
{
    UNIMPLEMENTED

    return 0;
}


/*
 * @unimplemented
 */
BOOL
WSPAPI
WPUResetEvent(IN  WSAEVENT hEvent,
              OUT LPINT lpErrno)
{
    UNIMPLEMENTED

    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WSPAPI
WPUSetEvent(IN  WSAEVENT hEvent,
            OUT LPINT lpErrno)
{
    UNIMPLEMENTED

    return FALSE;
}

/* EOF */
