/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 API
 * FILE:        dll/win32/ws2_32_new/src/dprovide.c
 * PURPOSE:     Transport Provider Object
 * PROGRAMMER:  Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <ws2_32.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

PTPROVIDER
WSAAPI
WsTpAllocate(VOID)
{
    PTPROVIDER Provider;

    /* Allocate the object */
    Provider = HeapAlloc(WsSockHeap, HEAP_ZERO_MEMORY, sizeof(*Provider));
    if (Provider)
    {
        /* Setup non-zero data */
        Provider->RefCount = 1;
    }

    /* Return it */
    return Provider;
}

DWORD
WSAAPI
WsTpInitialize(IN PTPROVIDER Provider,
               IN LPSTR DllName,
               IN LPWSAPROTOCOL_INFOW ProtocolInfo)
{
    WORD VersionRequested = MAKEWORD(2,2);
    LPWSPSTARTUP WSPStartupProc;
    WSPDATA WspData;
    CHAR ExpandedDllPath[MAX_PATH];
    DWORD ErrorCode;
    DPRINT("WsTpInitialize: %p, %p, %p\n", Provider, DllName, ProtocolInfo);

    /* Clear the tables */
    RtlZeroMemory(&Provider->UpcallTable, sizeof(Provider->UpcallTable));
    RtlZeroMemory(&Provider->Service, sizeof(Provider->Service));

    /* Set up the Upcall Table */
    Provider->UpcallTable.lpWPUCloseEvent = WPUCloseEvent;
    Provider->UpcallTable.lpWPUCloseSocketHandle = WPUCloseSocketHandle;
    Provider->UpcallTable.lpWPUCreateEvent = WPUCreateEvent;
    Provider->UpcallTable.lpWPUCreateSocketHandle = WPUCreateSocketHandle;
    Provider->UpcallTable.lpWPUFDIsSet = WPUFDIsSet;
    Provider->UpcallTable.lpWPUGetProviderPath = WPUGetProviderPath;
    Provider->UpcallTable.lpWPUModifyIFSHandle = WPUModifyIFSHandle;
    Provider->UpcallTable.lpWPUPostMessage = WPUPostMessage;
    Provider->UpcallTable.lpWPUQueryBlockingCallback = WPUQueryBlockingCallback;
    Provider->UpcallTable.lpWPUQuerySocketHandleContext = WPUQuerySocketHandleContext;
    Provider->UpcallTable.lpWPUQueueApc = WPUQueueApc;
    Provider->UpcallTable.lpWPUResetEvent = WPUResetEvent;
    Provider->UpcallTable.lpWPUSetEvent = WPUSetEvent;
    Provider->UpcallTable.lpWPUOpenCurrentThread = WPUOpenCurrentThread;
    Provider->UpcallTable.lpWPUCloseThread = WPUCloseThread;

    /* Expand the DLL Path */
    ExpandEnvironmentStrings(DllName, ExpandedDllPath, MAX_PATH);

    /* Load the DLL */
    Provider->DllHandle = LoadLibrary(ExpandedDllPath);

    if (!Provider->DllHandle)
    {
        return SOCKET_ERROR;
    }
    /* Get the pointer to WSPStartup */
    WSPStartupProc = (LPWSPSTARTUP)GetProcAddress(Provider->DllHandle, "WSPStartup");

    if (!WSPStartupProc)
    {
        return SOCKET_ERROR;
    }
    /* Call it */
    ErrorCode = (*WSPStartupProc)(VersionRequested,
                      &WspData,
                      ProtocolInfo,
                      Provider->UpcallTable,
                      &Provider->Service);

    /* Return */
    return ErrorCode;
}

DWORD
WSAAPI
WsTpWSPCleanup(IN PTPROVIDER Provider,
               IN LPINT lpErrNo)
{
    LPWSPCLEANUP WSPCleanup = NULL;
    INT ErrorCode = ERROR_SUCCESS;

    /* Make sure we have a loaded handle */
    if (Provider->DllHandle)
    {
        /* Get the pointer and clear it */
        WSPCleanup = InterlockedExchangePointer((PVOID*)&Provider->Service.lpWSPCleanup,
                                                NULL);
        /* If it's not NULL, call it */
        if (WSPCleanup) ErrorCode = WSPCleanup(lpErrNo);
    }

    /* Return */
    return ErrorCode;
}

VOID
WSAAPI
WsTpDelete(IN PTPROVIDER Provider)
{
    INT ErrorCode;

    /* Make sure we have a loaded handle */
    if (Provider->DllHandle)
    {
        /* Clean us up */
        WsTpWSPCleanup(Provider, &ErrorCode);

        /* Unload the library */
        FreeLibrary(Provider->DllHandle);
        Provider->DllHandle = NULL;
    }

    /* Delete us */
    HeapFree(WsSockHeap, 0, Provider);
}

VOID
WSAAPI
WsTpDereference(IN PTPROVIDER Provider)
{
    /* Decrease the reference count and check if it's zero */
    if (!InterlockedDecrement(&Provider->RefCount))
    {
        /* Delete us*/
        WsTpDelete(Provider);
    }
}
