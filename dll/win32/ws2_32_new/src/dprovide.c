/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 API
 * FILE:        dprovide.c
 * PURPOSE:     Transport Provider Object
 * PROGRAMMER:  Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/
#include "ws2_32.h"

/* FUNCTIONS *****************************************************************/

PTPROVIDER
WSAAPI
WsTpAllocate(VOID)
{
    PTPROVIDER Provider;
    
    /* Allocate the object */
    Provider = HeapAlloc(WsSockHeap, HEAP_ZERO_MEMORY, sizeof(*Provider));

    /* Setup non-zero data */
    Provider->RefCount = 1;
    
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
    WSPUPCALLTABLE UpcallTable;
    LPWSPSTARTUP WSPStartupProc;
    WSPDATA WspData;
    CHAR ExpandedDllPath[MAX_PATH];
    
    /* Clear the tables */
    RtlZeroMemory(&UpcallTable, sizeof(UpcallTable));
    RtlZeroMemory(&Provider->Service.lpWSPAccept, sizeof(WSPPROC_TABLE));

    /* Set up the Upcall Table */
    UpcallTable.lpWPUCloseEvent = WPUCloseEvent;
    UpcallTable.lpWPUCloseSocketHandle = WPUCloseSocketHandle;
    UpcallTable.lpWPUCreateEvent = WPUCreateEvent;
    UpcallTable.lpWPUCreateSocketHandle = WPUCreateSocketHandle;
    UpcallTable.lpWPUFDIsSet = WPUFDIsSet;
    UpcallTable.lpWPUGetProviderPath = WPUGetProviderPath;
    UpcallTable.lpWPUModifyIFSHandle = WPUModifyIFSHandle;
    UpcallTable.lpWPUPostMessage = WPUPostMessage;
    UpcallTable.lpWPUQueryBlockingCallback = WPUQueryBlockingCallback;
    UpcallTable.lpWPUQuerySocketHandleContext = WPUQuerySocketHandleContext;
    UpcallTable.lpWPUQueueApc = WPUQueueApc;
    UpcallTable.lpWPUResetEvent = WPUResetEvent;
    UpcallTable.lpWPUSetEvent = WPUSetEvent;
    UpcallTable.lpWPUOpenCurrentThread = WPUOpenCurrentThread;
    UpcallTable.lpWPUCloseThread = WPUCloseThread;

    /* Expand the DLL Path */
    ExpandEnvironmentStrings(DllName, ExpandedDllPath, MAX_PATH);

    /* Load the DLL */
    Provider->DllHandle = LoadLibrary(ExpandedDllPath);

    /* Get the pointer to WSPStartup */
    WSPStartupProc = (LPWSPSTARTUP)GetProcAddress(Provider->DllHandle, "WSPStartup");

    /* Call it */
    (*WSPStartupProc)(VersionRequested,
                      &WspData,
                      ProtocolInfo,
                      UpcallTable,
                      (LPWSPPROC_TABLE)&Provider->Service.lpWSPAccept);

    /* Return */
    return ERROR_SUCCESS;
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

        /* Clear the handle value */
        Provider->DllHandle = NULL;
    }
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
