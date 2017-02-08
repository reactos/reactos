/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 API
 * FILE:        dll/win32/ws2_32_new/src/dsocket.c
 * PURPOSE:     Socket Object
 * PROGRAMMER:  Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <ws2_32.h>

/* DATA **********************************************************************/

PWAH_HANDLE_TABLE WsSockHandleTable;

/* FUNCTIONS *****************************************************************/

INT
WSAAPI
WsSockStartup(VOID)
{
    /* Check if we have a socket table */
    if (!WsSockHandleTable)
    {
        /* Create it */
        return WahCreateHandleContextTable(&WsSockHandleTable);
    }

    /* Nothing to do */
    return NO_ERROR;
}

VOID
WSPAPI
WsSockCleanup(VOID)
{
    /* Check if we have a socket table */
    if (WsSockHandleTable)
    {
        /* Destroy it */
        WahDestroyHandleContextTable(WsSockHandleTable);
    }
}

PWSSOCKET
WSAAPI
WsSockAllocate(VOID)
{
    PWSSOCKET Socket;

    /* Allocate the socket object */
    Socket = HeapAlloc(WsSockHeap, HEAP_ZERO_MEMORY, sizeof(WSSOCKET));

    /* Setup default non-zero values */
    Socket->RefCount = 2;
    Socket->Overlapped = TRUE;

    /* Return it */
    return Socket;
}

INT
WSAAPI
WsSockInitialize(IN PWSSOCKET Socket,
                 IN PTCATALOG_ENTRY CatalogEntry)
{
    PWSTHREAD CurrentThread;

    /* Associate this catalog and reference it */
    Socket->CatalogEntry = CatalogEntry;
    InterlockedIncrement(&CatalogEntry->RefCount);

    /* Associate the Provider and Process Objects */
    Socket->Provider = CatalogEntry->Provider;

    /* Get the current Thread Object */
    if ((CurrentThread = TlsGetValue(TlsIndex)))
    {
        /* Set the overlapped mode */
        Socket->Overlapped = (CurrentThread->OpenType == 0);
    }

    /* Return status */
    return ERROR_SUCCESS;
}

PWSSOCKET
WSAAPI
WsSockGetSocketNoExport(IN SOCKET Handle)
{
    /* Let WAH do the translation */
    return (PWSSOCKET)WahReferenceContextByHandle(WsSockHandleTable,
                                                  (HANDLE)Handle);
}

PWSSOCKET
WSAAPI
WsSockFindIfsSocket(IN SOCKET Handle)
{
    INT ErrorCode;
    DWORD Flags;
    PWSSOCKET Socket = NULL;
    PWSPROCESS Process = NULL;
    PTCATALOG Catalog = NULL;

    /* Validate the socket and get handle info */
    if ((Handle != INVALID_SOCKET) &&
        (GetHandleInformation((HANDLE)Handle, &Flags)))
    {
        /* Get the process */
        if ((Process = WsGetProcess()))
        {
            /* Get the catalog */
            Catalog = WsProcGetTCatalog(Process);

            /* Get the IFS Provider */
            ErrorCode = WsTcFindIfsProviderForSocket(Catalog, Handle);

            /* Check for success */
            if (ErrorCode == ERROR_SUCCESS)
            {
                /* Get the Socket now */
                Socket = WsSockGetSocketNoExport(Handle);

                /* Mark it as an API Socket */
                if (Socket) Socket->ApiSocket = TRUE;
            }
        }
    }

    /* Return the socket */
    return Socket;
}

PWSSOCKET
WSAAPI
WsSockGetSocket(IN SOCKET Handle)
{
    PWSSOCKET Socket;

    /* Let WAH do the translation */
    if ((Socket = (PWSSOCKET)WahReferenceContextByHandle(WsSockHandleTable,
                                                         (HANDLE)Handle)))
    {
        return Socket;
    }
    else
    {
        /* WAH didn't find it, use IFS */
        return WsSockFindIfsSocket(Handle);
    }
}

INT
WSAAPI
WsSockAddApiReference(IN SOCKET Handle)
{
    PWSSOCKET Socket;

    /* Get the Socket now */
    if ((Socket = WsSockGetSocketNoExport(Handle)))
    {
        /* Mark it as an API Socket */
        if (Socket) Socket->ApiSocket = TRUE;

        /* Remove a reference and return */
        WsSockDereference(Socket);
        return ERROR_SUCCESS;
    }

    /* Return error */
    return WSASYSCALLFAILURE;
}

BOOL
WSAAPI
WsSockDeleteSockets(IN LPVOID Context,
                    IN PWAH_HANDLE Handle)
{
    /* Call the detach routine */
    return WsProcDetachSocket((PWSPROCESS)Context, Handle);
}

VOID
WSAAPI
WsSockDelete(IN PWSSOCKET Socket)
{
    /* Check if we have a catalog entry */
    if (Socket->CatalogEntry)
    {
        /* Dereference it */
        WsTcEntryDereference(Socket->CatalogEntry);
        Socket->CatalogEntry = NULL;
    }
}

VOID
WSAAPI
WsSockDereference(IN PWSSOCKET Socket)
{
    /* Dereference and check if it's now 0 */
    if (!(InterlockedDecrement(&Socket->RefCount)))
    {
        /* We can delete the Provider now */
        WsSockDelete(Socket);
    }
}

INT
WSAAPI
WsSockDisassociateHandle(IN PWSSOCKET Socket)
{
    /* Remove it from the list */
    return WahRemoveHandleContext(WsSockHandleTable, (PWAH_HANDLE)Socket);
}

INT
WSAAPI
WsSockAssociateHandle(IN PWSSOCKET Socket,
                      IN SOCKET Handle,
                      IN BOOLEAN IsProvider)
{
    INT ErrorCode = ERROR_SUCCESS;
    PWSSOCKET OldSocket;

    /* Save the socket and provider */
    Socket->IsProvider = IsProvider;
    Socket->Handle = (HANDLE)Handle;

    /* Insert it into the handle table */
    OldSocket = (PWSSOCKET)WahInsertHandleContext(WsSockHandleTable,
                                                  (PWAH_HANDLE)Socket);

    /* Check if a socket already existed */
    if (OldSocket != Socket)
    {
        /* We'll dereference it */
        WsSockDereference(OldSocket);
    }
    else if (!OldSocket)
    {
        /* No memory to allocate it */
        ErrorCode = WSAENOBUFS;
    }

    /* Return */
    return ErrorCode;
}
