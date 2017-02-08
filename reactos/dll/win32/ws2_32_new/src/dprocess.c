/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 API
 * FILE:        dll/win32/ws2_32_new/src/dprocess.c
 * PURPOSE:     Process Object
 * PROGRAMMER:  Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <ws2_32.h>

/* DATA **********************************************************************/

PWSPROCESS CurrentWsProcess;

#define WsProcLock()      EnterCriticalSection((LPCRITICAL_SECTION)&Process->ThreadLock);
#define WsProcUnlock()    LeaveCriticalSection((LPCRITICAL_SECTION)&Process->ThreadLock);

/* FUNCTIONS *****************************************************************/

INT
WSAAPI
WsProcInitialize(IN PWSPROCESS Process)
{
    INT ErrorCode = WSAEFAULT;
    HKEY RootKey = NULL;

    /* Initialize the thread list lock */
    InitializeCriticalSection((LPCRITICAL_SECTION)&Process->ThreadLock);
    Process->LockReady = TRUE;

    /* Open the Winsock Key */
    RootKey = WsOpenRegistryRoot();

    /* Create the LP Catalog change event and catalog */
    Process->ProtocolCatalogEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    Process->ProtocolCatalog = WsTcAllocate();

    /* Initialize it */
    WsTcInitializeFromRegistry(Process->ProtocolCatalog,
                               RootKey,
                               Process->ProtocolCatalogEvent);

    /* Create the NS Catalog change event and catalog */
    Process->NamespaceCatalogEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    Process->NamespaceCatalog = WsNcAllocate();

    /* Initialize it */
    ErrorCode = WsNcInitializeFromRegistry(Process->NamespaceCatalog,
                                           RootKey,
                                           Process->NamespaceCatalogEvent);

    /* Close the root key */
    RegCloseKey(RootKey);
    return ErrorCode;
}

PWSPROCESS
WSAAPI
WsProcAllocate(VOID)
{
    PWSPROCESS Process;

    /* Allocate the structure */
    Process = HeapAlloc(WsSockHeap, HEAP_ZERO_MEMORY, sizeof(*Process));

    /* Set default non-zero values */
    Process->Version = MAKEWORD(2,2);

    /* Return it */
    return Process;
}

INT
WSAAPI
WsProcOpenAsyncHelperDevice(IN PWSPROCESS Process,
                            OUT PHANDLE Handle)
{
    INT ErrorCode = WSASYSCALLFAILURE;

    /* Lock the process */
    WsProcLock();

    /* Check if we have a handle, and if not, create one */
    if ((Process->ApcHelper) ||
        (WahOpenApcHelper(&Process->ApcHelper) == ERROR_SUCCESS))
    {
        /* Return the handle */
        *Handle = Process->ApcHelper;
        ErrorCode = ERROR_SUCCESS;
    }

    /* Unload the process and return */
    WsProcUnlock();
    return ErrorCode;
}

INT
WSAAPI
WsProcGetAsyncHelper(IN PWSPROCESS Process,
                     OUT PHANDLE Handle)
{
    /* Check if we have it already set up */
    if (Process->ApcHelper)
    {
        /* Just return it */
        *Handle = Process->ApcHelper;
        return ERROR_SUCCESS;
    }
    else
    {
        /* Open it for the first time */
        return WsProcOpenAsyncHelperDevice(Process, Handle);
    }
}

INT
WSAAPI
WsProcStartup(VOID)
{
    INT ErrorCode = WSAEFAULT;

    /* Create a new process */
    CurrentWsProcess = WsProcAllocate();

    /* Initialize it */
    if (CurrentWsProcess)
    {
        /* Initialize the process */
        ErrorCode = WsProcInitialize(CurrentWsProcess);
    }
    else
    {
        /* No memory for the process object */
        ErrorCode = WSA_NOT_ENOUGH_MEMORY;
    }

    return ErrorCode;
}

PTCATALOG
WSAAPI
WsProcGetTCatalog(IN PWSPROCESS Process)
{
    /* Check if the catalogs have been modified */
    if (WsCheckCatalogState(Process->ProtocolCatalogEvent))
    {
        /* Modification happened, reload them */
        WsTcRefreshFromRegistry(Process->ProtocolCatalog,
                                Process->ProtocolCatalogEvent);
    }

    /* Return it */
    return Process->ProtocolCatalog;
}

PNSCATALOG
WSAAPI
WsProcGetNsCatalog(IN PWSPROCESS Process)
{
    /* Check if the catalogs have been modified */
    if (WsCheckCatalogState(Process->NamespaceCatalogEvent))
    {
        /* Modification happened, reload them */
        WsNcRefreshFromRegistry(Process->NamespaceCatalog,
                                Process->NamespaceCatalogEvent);
    }

    /* Return it */
    return Process->NamespaceCatalog;
}

BOOL
WSAAPI
WsProcDetachSocket(IN PWSPROCESS Process,
                   IN PWAH_HANDLE Handle)
{
    PWSSOCKET Socket = (PWSSOCKET)Handle;

    /* Disassociate this socket from the table */
    WahRemoveHandleContext(WsSockHandleTable, Handle);

    /* If this is isn't an IFS socket */
    if (!Socket->Provider)
    {
        /* Check if we have an active handle helper */
        if (Process->HandleHelper)
        {
            /* Close it */
            WahCloseSocketHandle(Process->HandleHelper, (SOCKET)Socket->Handle);
        }
    }

    /* Remove a reference and return */
    WsSockDereference(Socket);
    return TRUE;
}

BOOL
WSAAPI
CleanupNamespaceProviders(IN PVOID Callback,
                          IN PNSCATALOG_ENTRY Entry)
{
    PNS_PROVIDER Provider;

    /* Get the provider */
    Provider = Entry->Provider;
    if (Provider)
    {
        /* Do cleanup */
        WsNpNSPCleanup(Provider);
    }

    /* Return success */
    return TRUE;
}

BOOL
WSAAPI
CleanupProtocolProviders(IN PVOID Callback,
                         IN PTCATALOG_ENTRY Entry)
{
    PTPROVIDER Provider;
    INT ErrorCode;

    /* Get the provider */
    Provider = Entry->Provider;
    if (Provider)
    {
        /* Do cleanup */
        WsTpWSPCleanup(Provider, &ErrorCode);
    }

    /* Return success */
    return TRUE;
}

VOID
WSAAPI
WsProcDelete(IN PWSPROCESS Process)
{
    /* Check if we didn't even initialize yet */
    if (!Process->LockReady) return;

    /* No more current process */
    CurrentWsProcess = NULL;

    /* If we have a socket table */
    if (WsSockHandleTable)
    {
        /* Enumerate the sockets with a delete callback */
        WahEnumerateHandleContexts(WsSockHandleTable,
                                   WsSockDeleteSockets,
                                   Process);
    }

    /* Close APC Helper */
    if (Process->ApcHelper) WahCloseApcHelper(Process->ApcHelper);

    /* Close handle helper */
    if (Process->HandleHelper) WahCloseHandleHelper(Process->HandleHelper);

    /* Check for notification helper */
    if (Process->NotificationHelper)
    {
        /* Close notification helper */
        WahCloseNotificationHandleHelper(Process->NotificationHelper);
    }

    /* Check if we have a protocol catalog*/
    if (Process->ProtocolCatalog)
    {
        /* Enumerate it to clean it up */
        WsTcEnumerateCatalogItems(Process->ProtocolCatalog,
                                  CleanupProtocolProviders,
                                  NULL);

        /* Delete it */
        WsTcDelete(Process->ProtocolCatalog);
        Process->ProtocolCatalog = NULL;
    }

    /* Check if we have a namespace catalog*/
    if (Process->NamespaceCatalog)
    {
        /* Enumerate it to clean it up */
        WsNcEnumerateCatalogItems(Process->NamespaceCatalog,
                                  CleanupNamespaceProviders,
                                  NULL);

        /* Delete it */
        WsNcDelete(Process->NamespaceCatalog);
        Process->NamespaceCatalog = NULL;
    }

    /* Delete the thread lock */
    DeleteCriticalSection((LPCRITICAL_SECTION)&Process->ThreadLock);
}

VOID
WSAAPI
WsProcSetVersion(IN PWSPROCESS Process,
                 IN WORD VersionRequested)
{
    WORD Major, Minor;
    WORD OldMajor, OldMinor;

    /* Get the version data */
    Major = LOBYTE(VersionRequested);
    Minor = HIBYTE(VersionRequested);
    OldMajor = LOBYTE(Process->Version);
    OldMinor = HIBYTE(Process->Version);

    /* Check if we're going lower */
    if ((Major < OldMajor) || ((Major == OldMajor) && (Minor < OldMinor)))
    {
        /* Set the new version */
        Process->Version = VersionRequested;
    }
}
