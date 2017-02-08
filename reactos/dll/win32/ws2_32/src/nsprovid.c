/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 API
 * FILE:        dll/win32/ws2_32_new/src/nsprovid.c
 * PURPOSE:     Namespace Provider Object
 * PROGRAMMER:  Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <ws2_32.h>

/* FUNCTIONS *****************************************************************/

PNSQUERY_PROVIDER
WSAAPI
WsNqProvAllocate(VOID)
{
    PNSQUERY_PROVIDER Provider;
    
    /* Allocate the object */
    Provider = HeapAlloc(WsSockHeap, HEAP_ZERO_MEMORY, sizeof(*Provider));

    /* Return it */
    return Provider;
}

DWORD
WSAAPI
WsNqProvInitialize(IN PNSQUERY_PROVIDER QueryProvider,
                   IN PNS_PROVIDER Provider)
{
    /* Reference the provider */
    InterlockedIncrement(&Provider->RefCount);

    /* Set it as our own */
    QueryProvider->Provider = Provider;

    /* Return success */
    return ERROR_SUCCESS;
}

VOID
WSAAPI
WsNqProvDelete(IN PNSQUERY_PROVIDER QueryProvider)
{
    /* Check if we have a provider */
    if (QueryProvider->Provider)
    {
        /* Dereference it */
        WsNpDereference(QueryProvider->Provider);

        /* Clear it */
        QueryProvider->Provider = NULL;
    }

    /* Delete us */
    HeapFree(WsSockHeap, 0, QueryProvider);
}

PNS_PROVIDER
WSAAPI
WsNpAllocate(VOID)
{
    PNS_PROVIDER Provider;
    
    /* Allocate the object */
    Provider = HeapAlloc(WsSockHeap, HEAP_ZERO_MEMORY, sizeof(*Provider));

    /* Set non-null data */
    Provider->RefCount = 1;
    Provider->Service.cbSize = sizeof(NSP_ROUTINE);

    /* Return us */
    return Provider;
}

DWORD
WSAAPI
WsNpInitialize(IN PNS_PROVIDER Provider,
               IN LPWSTR DllName,
               IN LPGUID ProviderId)
{
    INT ErrorCode = ERROR_SUCCESS;
    LPNSPSTARTUP NSPStartupProc;
    CHAR AnsiPath[MAX_PATH], ExpandedDllPath[MAX_PATH];
    
    /* Convert the path to ANSI */
    WideCharToMultiByte(CP_ACP,
                        0,
                        DllName,
                        -1,
                        AnsiPath,
                        MAX_PATH,
                        NULL,
                        NULL);

    /* Expand the DLL Path */
    ExpandEnvironmentStringsA(AnsiPath,
                              ExpandedDllPath,
                              MAX_PATH);

    /* Load the DLL */
    Provider->DllHandle = LoadLibraryA(ExpandedDllPath);
    if (!Provider->DllHandle)
    {
        /* Fail */
        ErrorCode = WSAEPROVIDERFAILEDINIT;
        goto Fail;
    }

    /* Get the pointer to NSPStartup */
    NSPStartupProc = (LPNSPSTARTUP)GetProcAddress(Provider->DllHandle,
                                                  "NSPStartup");
    if (!NSPStartupProc)
    {
        /* Fail */
        ErrorCode = WSAEPROVIDERFAILEDINIT;
        goto Fail;
    }

    /* Call it */
    (*NSPStartupProc)(ProviderId, (LPNSP_ROUTINE)&Provider->Service.cbSize);

    /* Save the provider ID */
    Provider->ProviderId = *ProviderId;
    return ErrorCode;

Fail:
    /* Bail out */
    if (Provider->DllHandle) FreeLibrary(Provider->DllHandle);
    return ErrorCode;
}

DWORD
WSAAPI
WsNpNSPCleanup(IN PNS_PROVIDER Provider)
{
    INT ErrorCode = ERROR_SUCCESS;
    LPNSPCLEANUP lpNSPCleanup = NULL;
    
    /* Make sure we have a loaded handle */
    if (Provider->DllHandle)
    {
        /* Get the pointer and clear it */
        lpNSPCleanup = InterlockedExchangePointer((PVOID*)&Provider->Service.NSPCleanup,
                                                  NULL);
        /* If it's not NULL, call it */
        if (lpNSPCleanup) ErrorCode = lpNSPCleanup(&Provider->ProviderId);
    }

    /* Return */
    return ErrorCode;
}

VOID
WSAAPI
WsNpDelete(IN PNS_PROVIDER Provider)
{
    /* Make sure we have a loaded handle */
    if (Provider->DllHandle)
    {
        /* Clean us up */
        WsNpNSPCleanup(Provider);

        /* Unload the library */
        FreeLibrary(Provider->DllHandle);

        /* Clear the handle value */
        Provider->DllHandle = NULL;
    }
}

VOID
WSAAPI
WsNpDereference(IN PNS_PROVIDER Provider)
{
    /* Decrease the reference count and check if it's zero */
    if (!InterlockedDecrement(&Provider->RefCount))
    {
        /* Delete us*/
        WsNpDelete(Provider);
    }
}

DWORD
WSAAPI
WsNqProvLookupServiceEnd(IN PNSQUERY_PROVIDER QueryProvider)
{
    /* Simply call the provider */
    return WsNpLookupServiceEnd(QueryProvider->Provider,
                                QueryProvider->LookupHandle);
}

DWORD
WSAAPI
WsNqProvLookupServiceNext(IN PNSQUERY_PROVIDER QueryProvider,
                          IN DWORD ControlFlags,
                          IN PDWORD BufferLength,
                          LPWSAQUERYSETW Results)
{
    /* Simply call the provider */
    return WsNpLookupServiceNext(QueryProvider->Provider,
                                 QueryProvider->LookupHandle,
                                 ControlFlags,
                                 BufferLength,
                                 Results);
}

DWORD
WSAAPI
WsNqProvLookupServiceBegin(IN PNSQUERY_PROVIDER QueryProvider,
                           IN LPWSAQUERYSETW Restrictions,
                           IN LPWSASERVICECLASSINFOW ServiceClassInfo,
                           IN DWORD ControlFlags)
{
    /* Simply call the provider */
    return WsNpLookupServiceBegin(QueryProvider->Provider,
                                  Restrictions,
                                  ServiceClassInfo,
                                  ControlFlags,
                                  &QueryProvider->LookupHandle);
}

DWORD
WSAAPI
WsNpLookupServiceEnd(IN PNS_PROVIDER Provider,
                     IN HANDLE LookupHandle)
{
    /* Call the NSP */
    return Provider->Service.NSPLookupServiceEnd(LookupHandle);
}

DWORD
WSAAPI
WsNpLookupServiceNext(IN PNS_PROVIDER Provider,
                      IN HANDLE LookupHandle,
                      IN DWORD ControlFlags,
                      OUT PDWORD BufferLength,
                      OUT LPWSAQUERYSETW Results)
{
    /* Call the NSP */
    return Provider->Service.NSPLookupServiceNext(LookupHandle,
                                                  ControlFlags,
                                                  BufferLength,
                                                  Results);
}

DWORD
WSAAPI
WsNpLookupServiceBegin(IN PNS_PROVIDER Provider,
                       IN LPWSAQUERYSETW Restrictions,
                       IN LPWSASERVICECLASSINFOW ServiceClassInfo,
                       IN DWORD ControlFlags,
                       OUT PHANDLE LookupHandle)
{
    /* Call the NSP */
    return Provider->Service.NSPLookupServiceBegin(&Provider->ProviderId,
                                                   Restrictions,
                                                   ServiceClassInfo,
                                                   ControlFlags,
                                                   LookupHandle);
}

