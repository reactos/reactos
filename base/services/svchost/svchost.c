/*
 * PROJECT:     ReactOS Service Host
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * FILE:        base/services/svchost/svchost.c
 * PURPOSE:     Main Service Host Implementation Routines
 * PROGRAMMERS: ReactOS Portable Systems Group
 */

/* INCLUDES ******************************************************************/

#include "svchost.h"

#include <objidl.h>

/* GLOBALS *******************************************************************/

LIST_ENTRY DllList;
CRITICAL_SECTION ListLock;
DWORD ServiceCount;
LPCWSTR ServiceNames;
PSVCHOST_SERVICE ServiceArray;

/* FUNCTIONS *****************************************************************/

__callback
LONG
WINAPI
SvchostUnhandledExceptionFilter (
    _In_ struct _EXCEPTION_POINTERS * ExceptionInfo
    )
{
    /* Call the default OS handler */
    return RtlUnhandledExceptionFilter(ExceptionInfo);
}

VOID
WINAPI
DummySvchostCtrlHandler (
    _In_ DWORD dwControl
    )
{
    /* This is just a stub while we abort loading */
    return;
}

PSVCHOST_OPTIONS
WINAPI
BuildCommandOptions (
    _In_ LPWSTR pszCmdLine
    )
{
    ULONG cbCmdLine;
    PSVCHOST_OPTIONS pOptions;
    LPWSTR pch, pGroupNameStart, lpServiceGroup;
    LPWSTR *pGroupName = NULL;

    /* Nothing to do without a command-line */
    if (pszCmdLine == NULL) return ERROR_SUCCESS;

    /* Get the length if the command line*/
    cbCmdLine = sizeof(WCHAR) * lstrlenW(pszCmdLine) + sizeof(UNICODE_NULL);

    /* Allocate a buffer for the parsed options */
    pOptions = MemAlloc(HEAP_ZERO_MEMORY, cbCmdLine + sizeof(*pOptions));
    if (pOptions == NULL) return ERROR_SUCCESS;

    /* Copy the buffer into the parsed options block */
    pOptions->CmdLineBuffer = (PWCHAR)(pOptions + 1);
    memcpy(pOptions->CmdLineBuffer, pszCmdLine, cbCmdLine);

    /* Set the command-line as being inside the block itself */
    pch = pOptions->CmdLineBuffer;
    ASSERT(pch);
    pOptions->CmdLine = pch;

    /* Now scan it */
    while (*pch != UNICODE_NULL)
    {
        /* And look for the first space or TAB */
        if ((*pch == ' ') || (*pch == '\t'))
        {
            /* Terminate the command-line there */
            *pch++ = UNICODE_NULL;
            break;
        }

        /* Keep searching */
        pch++;
    }

    /* Lowercase the entire command line, but not the options */
    SvchostCharLowerW(pOptions->CmdLine);

    /* Loop over the command-line */
    while (*pch != UNICODE_NULL)
    {
        /* Do an inner loop within it */
        while (*pch != UNICODE_NULL)
        {
            /* And keep going until we find a space or TAB */
            if ((*pch != ' ') && (*pch != '\t')) break;
            pch++;
        }

        /* Did we reach the end? If so, bail out */
        if (*pch == UNICODE_NULL) break;

        /* We found the space -- is it followed by a dash or slash? */
        if ((*pch == '-' || *pch == '/') && (*(pch + 1) != UNICODE_NULL))
        {
            /* Yep, and there's at least one character. Is it k or K? */
            pch++;
            if (*pch == 'k' || *pch == 'K')
            {
                /* Yep, we have a proper command line with a group! */
                pOptions->HasServiceGroup = TRUE;
                pGroupName = &pOptions->ServiceGroupName;
            }

            /* Keep going -- the name follows between spaces or quotes */
            pch++;
        }
        else
        {
            /* Nope, so this might be the group being entered */
            pGroupNameStart = pch;

            /* Check if there's a quote */
            if ((*pch == '"') && (*(pch + 1) != UNICODE_NULL))
            {
                /* There is, keep going until the quote is over with */
                pGroupNameStart = ++pch;
                while (*pch) if (*pch++ == '"') break;
            }
            else
            {
                /* No quote, so keep going until the next space or TAB */
                while ((*pch) && ((*pch != ' ') && (*pch != '\t'))) pch++;
            }

            /* Now we have a space or quote deliminated name, terminate it */
            if (*pch) *pch++ = UNICODE_NULL;

            /* Ok, so we have a string -- was it preceeded by a -K or /K? */
            if (pGroupName)
            {
                /* Yes it was, remember this, and don't overwrite it later */
                *pGroupName = pGroupNameStart;
                pGroupName = NULL;
            }
        }
    }

    /* Print out the service group for the debugger, and the command line */
    DBG_TRACE("Command line     : %ws\n", pszCmdLine);
    lpServiceGroup = (pOptions->HasServiceGroup) ?
                      pOptions->ServiceGroupName : L"No";
    DBG_TRACE("Service Group    : %ws\n", lpServiceGroup);

    /* Was a group specified? */
    if (pOptions->HasServiceGroup == FALSE)
    {
        /* This isn't valid. Print out help on the debugger and fail */
        DBG_TRACE("Generic Service Host\n\n"
                  "%ws [-k <key>] | [-r] | <service>\n\n"
                  "   -k <key>   Host all services whose ImagePath matches\n"
                  "              %ws -k <key>.\n\n",
                  lpServiceGroup);
        MemFree(pOptions);
        pOptions = NULL;
    }

    /* Return the allocated option block, if valid*/
    return pOptions;
}

DWORD
WINAPI
ReadPerInstanceRegistryParameters (
    _In_ HKEY hKey,
    _In_ PSVCHOST_OPTIONS pOptions
    )
{
    DWORD dwError, dwData;
    HKEY hSubKey;

    /* We should have a name for this service group... */
    ASSERT(pOptions->ServiceGroupName);

    /* Query the group to see what services are part of it */
    dwError = RegQueryString(hKey,
                             pOptions->ServiceGroupName,
                             REG_MULTI_SZ,
                             (PBYTE*)&ServiceNames);
    if ((dwError == ERROR_SUCCESS) &&
        ((ServiceNames == NULL) || (*ServiceNames == UNICODE_NULL)))
    {
        /* If the key exists but there's no data, fail */
        return ERROR_INVALID_DATA;
    }

    /* Open the key for this group */
    if (RegOpenKeyExW(hKey,
                      pOptions->ServiceGroupName,
                      0,
                      KEY_READ,
                      &hSubKey) != ERROR_SUCCESS)
    {
        /* If we couldn't, bail out */
        return dwError;
    }

    /* Check if we should initialize COM */
    if (RegQueryDword(hSubKey,
                      L"CoInitializeSecurityParam",
                      &dwData) == ERROR_SUCCESS)
    {
        /* Yes, remember the parameter to be sent when we do so */
        pOptions->CoInitializeSecurityParam = dwData;
    }

    /* Also, if COM is requested, we must read a bunch more data... */
    if (pOptions->CoInitializeSecurityParam)
    {
        /* First, read the authentication level, use a default if none is set */
        if (RegQueryDword(hSubKey, L"AuthenticationLevel", &dwData))
        {
            pOptions->AuthenticationLevel = RPC_C_AUTHN_LEVEL_PKT;
        }
        else
        {
            pOptions->AuthenticationLevel = dwData;
        }

        /* Do the same for the impersonation level */
        if (RegQueryDword(hSubKey, L"ImpersonationLevel", &dwData))
        {
            pOptions->ImpersonationLevel = RPC_C_IMP_LEVEL_IDENTIFY;
        }
        else
        {
            pOptions->ImpersonationLevel = dwData;
        }

        /* Do the same for the authentication capabilities */
        if (RegQueryDword(hSubKey, L"AuthenticationCapabilities", &dwData))
        {
            pOptions->AuthenticationCapabilities = EOAC_NO_CUSTOM_MARSHAL |
                                                   EOAC_DISABLE_AAA;
        }
        else
        {
            pOptions->AuthenticationCapabilities = dwData;
        }
    }

    /* Check if we need a specific RPC stack size, if not, we'll set it later */
    if (!RegQueryDword(hSubKey, L"DefaultRpcStackSize", &dwData))
    {
        pOptions->DefaultRpcStackSize = dwData;
    }

    /* Finally, check if the services should be marked critical later on */
    if (!RegQueryDword(hSubKey, L"SystemCritical", &dwData))
    {
        pOptions->SystemCritical = dwData;
    }

    /* All done reading the settings */
    RegCloseKey(hSubKey);
    return dwError;
}

VOID
WINAPI
BuildServiceArray (
    _In_ PSVCHOST_OPTIONS lpOptions
    )
{
    DWORD dwError;
    PSVCHOST_SERVICE pService;
    LPCWSTR pszServiceName;
    HKEY hKey;

    /* Open the service host key */
    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Svchost",
                            0,
                            KEY_READ,
                            &hKey);
    if (dwError != ERROR_SUCCESS) return;

    /* Read all the parameters for this service group */
    dwError = ReadPerInstanceRegistryParameters(hKey, lpOptions);
    RegCloseKey(hKey);
    if (dwError != ERROR_SUCCESS) return;

    /* Acquire the database lock while we create the service group */
    EnterCriticalSection(&ListLock);

    /* Loop the array of services for this group */
    pszServiceName = ServiceNames;
    while (*pszServiceName != UNICODE_NULL)
    {
        /* For each one, increment the amount of services we'llhost */
        ServiceCount++;
        pszServiceName += lstrlenW(pszServiceName) + 1;
    }

    /* We should host at least one... */
    ASSERT(ServiceCount);

    /* Now allocate an array to hold all their pointers */
    ServiceArray = MemAlloc(HEAP_ZERO_MEMORY, sizeof(*pService) * ServiceCount);
    if (ServiceArray)
    {
        /* Start at the beginning of the array */
        pService = ServiceArray;

        /* Loop through all the services */
        pszServiceName = ServiceNames;
        while (*pszServiceName != UNICODE_NULL)
        {
            /* Save the service's name and move to the next entry */
            pService++->pszServiceName = pszServiceName;
            pszServiceName += lstrlenW(pszServiceName) + 1;
        }

        /* We should have gotten the same count as earlier! */
        ASSERT(pService == ServiceArray + ServiceCount);
    }

    /* We're done, drop the lock */
    LeaveCriticalSection(&ListLock);
}

BOOL
WINAPI
FDebugBreakForService (
    _In_ LPCWSTR ServiceName
    )
{
    DWORD Data;
    BOOL DebugBreakOn = FALSE;
    HKEY hKey, hSubKey;

    /* Open the key for Svchost itself */
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Svchost",
                      0,
                      KEY_READ,
                      &hKey) == ERROR_SUCCESS)
    {
        /* That worked, now open the key for this service, if it exists */
        if (RegOpenKeyExW(hKey, ServiceName, 0, KEY_READ, &hSubKey) == ERROR_SUCCESS)
        {
            /* It does -- is there a DebugBreak value set? */
            if (RegQueryDword(hSubKey, L"DebugBreak", &Data) == ERROR_SUCCESS)
            {
                /* There is! Check if it's 0 or 1 */
                DebugBreakOn = Data != 0;
            }

            /* Done, close the service key */
            RegCloseKey(hSubKey);
        }

        /* Done, close the svchost key */
        RegCloseKey(hKey);
    }

    /* Return if the value was set or not */
    return DebugBreakOn;
}

DWORD
WINAPI
OpenServiceParametersKey (
    _In_ LPCWSTR lpSubKey,
    _Out_ PHKEY phKey
    )
{
    DWORD dwError;
    HKEY hSubKey = NULL, hKey = NULL;
    ASSERT(phKey);

    /* Open the services key */
    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            L"System\\CurrentControlSet\\Services",
                            0,
                            KEY_READ,
                            &hKey);
    if (dwError == ERROR_SUCCESS)
    {
        /* Now open the service specific key, returning its handle */
        dwError = RegOpenKeyExW(hKey, lpSubKey, 0, KEY_READ, &hSubKey);
        if (dwError == ERROR_SUCCESS)
        {
            /* And if that worked, return a handle to the parameters key */
            dwError = RegOpenKeyExW(hSubKey,
                                    L"Parameters",
                                    0,
                                    KEY_READ,
                                    phKey);
        }
    }

    /* Clean up any leftovers*/
    if (hKey != NULL) RegCloseKey(hKey);
    if (hSubKey != NULL) RegCloseKey(hSubKey);
    return dwError;
}

VOID
WINAPI
UnloadServiceDll (
    _In_ PSVCHOST_SERVICE pService
    )
{
    DWORD cbData;
    DWORD Data;
    DWORD dwType;
    HKEY hKey = NULL;

    cbData = 4;
    Data = 0;

    /* Grab the lock while we touch the thread count and potentially unload */
    EnterCriticalSection(&ListLock);

    /* There is one less active callout into the DLL, it may be unloadabl now */
    ASSERT(pService->cServiceActiveThreadCount > 0);
    pService->cServiceActiveThreadCount--;

    /* Try to open the service registry key */
    if (OpenServiceParametersKey(pService->pszServiceName, &hKey) == ERROR_SUCCESS)
    {
        /* It worked -- check if we should unload the service when stopped */
        if (RegQueryValueExW(hKey,
                             L"ServiceDllUnloadOnStop",
                             0,
                             &dwType,
                             (LPBYTE)&Data,
                             &cbData) == ERROR_SUCCESS)
        {
            /* Make sure the data is correctly formatted */
            if ((dwType == REG_DWORD) && (Data == 1))
            {
                /* Does the service still have active threads? */
                if (pService->cServiceActiveThreadCount != 0)
                {
                    /* Yep, so we can't kill it just yet */
                    DBG_TRACE("Skip Unload dll %ws for service %ws, "
                              "active threads %d\n",
                              pService->pDll->pszDllPath,
                              pService->pszServiceName,
                              pService->cServiceActiveThreadCount);
                }
                else
                {
                    /* Nope, we're good to go */
                    DBG_TRACE("Unload dll %ws for service %ws\n",
                              pService->pDll->pszDllPath,
                              pService->pszServiceName);

                    /* Free the library and clear the module handle */
                    FreeLibrary(pService->pDll->hModule);
                    pService->pDll->hModule = NULL;
                }
            }
        }
    }

    /* Drop the lock, unload is complete */
    LeaveCriticalSection(&ListLock);

    /* Close the parameters key if we had it */
    if (hKey) RegCloseKey(hKey);
}

VOID
WINAPI
SvchostStopCallback (
    _In_ PVOID Context,
    _In_ BOOLEAN TimerOrWaitFired
    )
{
    PSVCHOST_CALLBACK_CONTEXT pSvcsStopCbContext = Context;
    PSVCHOST_STOP_CALLBACK pfnStopCallback;

    /* Hold the lock while we grab the callback */
    EnterCriticalSection(&ListLock);

    /* Grab the callback, then clear it */
    ASSERT(pSvcsStopCbContext->pService != NULL);
    pfnStopCallback = pSvcsStopCbContext->pService->pfnStopCallback;
    ASSERT(pfnStopCallback != NULL);
    pSvcsStopCbContext->pService->pfnStopCallback = NULL;

    /* Now release the lock, making sure the above was atomic */
    LeaveCriticalSection(&ListLock);

    /* Now make the callout */
    DBG_TRACE("Call stop callback for service %ws, active threads %d\n",
              pSvcsStopCbContext->pService->pszServiceName,
              pSvcsStopCbContext->pService->cServiceActiveThreadCount);
    pfnStopCallback(pSvcsStopCbContext->pContext, TimerOrWaitFired);

    /* Decrement the active threads -- maybe the DLL can unload now */
    UnloadServiceDll(pSvcsStopCbContext->pService);

    /* We no longer need the context */
    MemFree(pSvcsStopCbContext);
}

DWORD
WINAPI
SvcRegisterStopCallback (
    _In_ PHANDLE phNewWaitObject,
    _In_ LPCWSTR ServiceName,
    _In_ HANDLE hObject,
    _In_ PSVCHOST_STOP_CALLBACK pfnStopCallback,
    _In_ PVOID pContext,
    _In_ ULONG dwFlags
    )
{
    ULONG i;
    PSVCHOST_CALLBACK_CONTEXT pSvcsStopCbContext = NULL;
    PSVCHOST_SERVICE pService = NULL;
    DWORD dwError = ERROR_SUCCESS;

    /* All parameters must be present */
    if ((phNewWaitObject == NULL) ||
        (ServiceName == NULL) ||
        (hObject == NULL) ||
        (pfnStopCallback == NULL))
    {
        /* Fail otherwise */
        return ERROR_INVALID_PARAMETER;
    }

    /* Don't allow new DLLs while we send notifications */
    EnterCriticalSection(&ListLock);

    /* Scan all registered services */
    for (i = 0; i < ServiceCount; i++)
    {
        /* Search for the service entry matching this service name */
        if (lstrcmpiW(ServiceName, ServiceArray[i].pszServiceName) == 0)
        {
            /* Found it */
            pService = &ServiceArray[i];
            break;
        }
    }

    /* Do we have a service? Does it already have a callback? */
    if ((pService == NULL) || (pService->pfnStopCallback != NULL))
    {
        /* Affirmative, nothing for us to do */
        dwError = ERROR_INVALID_DATA;
        DBG_ERR("Service %ws missing or already registered for callback, "
                "status %d\n",
                ServiceName,
                dwError);
        goto Quickie;
    }

    /* Allocate our internal context */
    pSvcsStopCbContext = MemAlloc(HEAP_ZERO_MEMORY, sizeof(*pSvcsStopCbContext));
    if (pSvcsStopCbContext == NULL)
    {
        /* We failed, bail out */
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        DBG_ERR("MemAlloc for context block for service %ws failed, status "
                "%d\n",
                ServiceName,
                dwError);
        goto Quickie;
    }

    /* Setup the context and register for the wait */
    pSvcsStopCbContext->pContext = pContext;
    pSvcsStopCbContext->pService = pService;
    if (RegisterWaitForSingleObject(phNewWaitObject,
                                    hObject,
                                    SvchostStopCallback,
                                    pSvcsStopCbContext,
                                    INFINITE,
                                    dwFlags))
    {
        /* We now have an active thread associated with this wait */
        pService->cServiceActiveThreadCount++;
        pService->pfnStopCallback = pfnStopCallback;
        DBG_TRACE("Registered stop callback for service %ws, active threads %d\n",
                  ServiceName,
                  pService->cServiceActiveThreadCount);
    }
    else
    {
        /* Registration failed, bail out */
        dwError = GetLastError();
        DBG_ERR("Registration for stop callback for service %ws failed, "
                "status %d\n",
                ServiceName,
                dwError);
    }

Quickie:
    /* Drop the lock, and free the context if we failed */
    LeaveCriticalSection(&ListLock);
    if (dwError != ERROR_SUCCESS) MemFree(pSvcsStopCbContext);
    return dwError;
}

VOID
WINAPI
AbortSvchostService (
    _In_ LPCWSTR lpServiceName,
    _In_ DWORD dwExitCode
    )
{
    SERVICE_STATUS_HANDLE scHandle;
    SERVICE_STATUS ServiceStatus;

    /* Make us stopped and accept only query commands */
    ServiceStatus.dwCheckPoint = 0;
    ServiceStatus.dwWaitHint = 0;
    ServiceStatus.dwServiceSpecificExitCode = 0;
    ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    ServiceStatus.dwControlsAccepted = SERVICE_QUERY_CONFIG;
    ServiceStatus.dwServiceType = SERVICE_WIN32;
    ServiceStatus.dwWin32ExitCode = dwExitCode;

    /* Register a handler that will do nothing while we are being stopped */
    scHandle = RegisterServiceCtrlHandlerW(lpServiceName,
                                           DummySvchostCtrlHandler);
    if (scHandle)
    {
        /* Stop us */
        if (!SetServiceStatus(scHandle, &ServiceStatus))
        {
            /* Tell the debugger if this didn't work */
            DBG_ERR("AbortSvchostService: SetServiceStatus error %ld\n",
                    GetLastError());
        }
    }
    else
    {
        /* Tell the debugger if we couldn't register the handler */
        DBG_ERR("AbortSvchostService: RegisterServiceCtrlHandler failed %d\n",
                GetLastError());
    }
}

PVOID
WINAPI
GetServiceDllFunction (
    _In_ PSVCHOST_DLL pDll,
    _In_ LPCSTR lpProcName,
    _Out_ PDWORD lpdwError
    )
{
    HMODULE hModule;
    PVOID lpAddress = NULL;
    ULONG_PTR ulCookie = 0;

    /* Activate the context */
    if (ActivateActCtx(pDll->hActCtx, &ulCookie) != TRUE)
    {
        /* We couldn't, bail out */
        if (lpdwError) *lpdwError = GetLastError();
        DBG_ERR("ActivateActCtx for %ws failed.  Error %d.\n",
                pDll->pszDllPath,
                GetLastError());
        return lpAddress;
    }

    /* Check if we already have a loaded module for this service */
    hModule = pDll->hModule;
    if (!hModule)
    {
        /* We don't -- load it */
        hModule = LoadLibraryExW(pDll->pszDllPath,
                                    NULL,
                                    LOAD_WITH_ALTERED_SEARCH_PATH);
        if (!hModule)
        {
            /* We failed to load it, bail out */
            if (lpdwError) *lpdwError = GetLastError();
            DBG_ERR("LoadLibrary (%ws) failed.  Error %d.\n",
                    pDll->pszDllPath,
                    GetLastError());
            DeactivateActCtx(0, ulCookie);
            return lpAddress;
        }

        /* Loaded! */
        pDll->hModule = hModule;
    }

    /* Next, get the address being looked up*/
    lpAddress = GetProcAddress(hModule, lpProcName);
    if (!lpAddress)
    {
        /* We couldn't find it, write the error code and a debug statement */
        if (lpdwError) *lpdwError = GetLastError();
        DBG_ERR("GetProcAddress (%s) failed on DLL %ws.  Error %d.\n",
                lpProcName,
                pDll->pszDllPath,
                GetLastError());
    }

    /* All done, get rid of the activation context */
    DeactivateActCtx(0, ulCookie);
    return lpAddress;
}

PSVCHOST_DLL
WINAPI
FindDll (
    _In_ LPCWSTR pszManifestPath,
    _In_ LPCWSTR pszDllPath,
    _In_ PSVCHOST_SERVICE pService
    )
{
    PSVCHOST_DLL pDll, pFoundDll = NULL;
    PLIST_ENTRY pNextEntry;
    ASSERT(pszDllPath);

    /* Lock the DLL database */
    EnterCriticalSection(&ListLock);

    /* Scan through the database */
    pNextEntry = DllList.Flink;
    while (pNextEntry != &DllList)
    {
        /* Search for an existing DLL with the same parameters */
        pDll = CONTAINING_RECORD(pNextEntry, SVCHOST_DLL, DllList);
        if ((lstrcmpW(pDll->pszDllPath, pszDllPath) == 0) &&
            (lstrcmpW(pDll->pszManifestPath, pszManifestPath) == 0) &&
            (lstrcmpW(pDll->pService->pszServiceName, pService->pszServiceName) == 0))
        {
            /* Found it! */
            pFoundDll = pDll;
            break;
        }

        /* Keep searching */
        pNextEntry = pNextEntry->Flink;
    }

    /* All done, release the lock and return what we found, if anything */
    LeaveCriticalSection(&ListLock);
    return pFoundDll;
}

PSVCHOST_DLL
WINAPI
AddDll (
    _In_ LPCWSTR pszManifestPath,
    _In_ LPCWSTR pszDllPath,
    _In_ PSVCHOST_SERVICE pService,
    _Out_ PDWORD lpdwError
    )
{
    PSVCHOST_DLL pDll;
    ULONG nDllPathLength, nManifestPathLength;
    ASSERT(pszDllPath);
    ASSERT(*pszDllPath);

    /* Compute the length of the two paths */
    nDllPathLength = lstrlenW(pszDllPath);
    nManifestPathLength = lstrlenW(pszManifestPath);

    /* Allocate the DLL entry for this service */
    pDll =  MemAlloc(HEAP_ZERO_MEMORY,
                     sizeof(*pDll) +
                     (nDllPathLength + nManifestPathLength) * sizeof(WCHAR) +
                     2 * sizeof(UNICODE_NULL));
    if (pDll == NULL)
    {
        /* Bail out with failure */
        *lpdwError = ERROR_NOT_ENOUGH_MEMORY;
        return pDll;
    }

    /* Put the DLL path right after the DLL entry */
    pDll->pszDllPath = (LPCWSTR)(pDll + 1);

    /* Put the manifest path right past the DLL path (note the pointer math) */
    pDll->pszManifestPath = pDll->pszDllPath + nDllPathLength + 1;

    /* Now copy both paths */
    CopyMemory((PVOID)pDll->pszDllPath,
               pszDllPath,
               sizeof(WCHAR) * nDllPathLength);
    CopyMemory((PVOID)pDll->pszManifestPath,
               pszManifestPath,
               sizeof(WCHAR) * nManifestPathLength);

    /* Now set the service, and make sure both paths are NULL-terminated */
    pDll->pService = pService;
    ASSERT(pDll->hActCtx == NULL);
    ASSERT(pDll->pszDllPath[nDllPathLength] == UNICODE_NULL);
    ASSERT(pDll->pszManifestPath[nManifestPathLength] == UNICODE_NULL);

    /* Finally, add the entry to the DLL database, while holding the lock */
    EnterCriticalSection(&ListLock);
    InsertTailList(&DllList, &pDll->DllList);
    LeaveCriticalSection(&ListLock);

    /* And return it */
    return pDll;
}

VOID
WINAPI
GetServiceMainFunctions (
    _In_ PSVCHOST_SERVICE pService,
    _Out_ PVOID *pServiceMain,
    _Out_ PVOID *pPushServiceGlobals,
    _Out_ PDWORD lpdwError
    )
{
    DWORD dwError, cbDllLength, cbData, dwType;
    PSVCHOST_DLL pDll;
    ACTCTXW actCtx;
    LPCWSTR pszDllPath;
    HKEY hKey;
    HANDLE hActCtx;
    LPWSTR lpData;
    WCHAR szDllBuffer[MAX_PATH + 2], szManifestBuffer[MAX_PATH + 2];

    /* Initialize the activation context we might need later */
    RtlZeroMemory(&actCtx, sizeof(actCtx));
    actCtx.cbSize = sizeof(actCtx);

    /* We clean these up in our failure path so initialize them to NULL here */
    hActCtx = NULL;
    hKey = NULL;
    lpData = NULL;
    *lpdwError = ERROR_SUCCESS;

    /* Null terminate the string buffers */
    szDllBuffer[0] = UNICODE_NULL;
    szManifestBuffer[0] = UNICODE_NULL;

    /* Do we already have a DLL ready to go for this service? */
    pDll = pService->pDll;
    if (pDll != NULL) goto HaveDll;

    /* Nope, we're starting from scratch. Open a handle to parameters key */
    dwError = OpenServiceParametersKey(pService->pszServiceName, &hKey);
    if (dwError)
    {
        *lpdwError = dwError;
        ASSERT(*lpdwError != NO_ERROR);
        goto Quickie;
    }

    /* Allocate enough space to hold a unicode path (NULL-terminated) */
    cbData = MAX_PATH * sizeof(WCHAR) + sizeof(UNICODE_NULL);
    lpData = MemAlloc(0, cbData);
    if (lpData == NULL)
    {
        /* No memory, bail out */
        *lpdwError = ERROR_NOT_ENOUGH_MEMORY;
        goto Quickie;
    }

    /* Query the DLL path */
    lpData[0] = UNICODE_NULL;
    dwError = RegQueryValueExW(hKey,
                               L"ServiceDll",
                               0,
                               &dwType,
                               (LPBYTE)lpData,
                               &cbData);
    if (dwError != ERROR_SUCCESS)
    {
        *lpdwError = dwError;
        DBG_ERR("RegQueryValueEx for the ServiceDll parameter of the %ws "
                "service returned %u\n",
                pService->pszServiceName,
                dwError);
        goto Quickie;
    }

    /* Is the registry data valid and present? */
    if ((dwType != REG_EXPAND_SZ) || (lpData[0] == UNICODE_NULL))
    {
        /* Nope, bail out */
        *lpdwError = ERROR_FILE_NOT_FOUND;
        DBG_ERR("The ServiceDll parameter for the %ws service is not of type "
                "REG_EXPAND_SZ\n",
                pService->pszServiceName);
        goto Quickie;
    }

    /* Convert the expandable path into an absolute path */
    ExpandEnvironmentStringsW(lpData, szDllBuffer, MAX_PATH);
    SvchostCharLowerW(szDllBuffer);

    /* Check if the service has a manifest file associated with it */
    cbData = MAX_PATH * sizeof(WCHAR) + sizeof(UNICODE_NULL);
    dwError = RegQueryValueExW(hKey,
                               L"ServiceManifest",
                               NULL,
                               &dwType,
                               (LPBYTE)lpData,
                               &cbData);
    if (dwError != ERROR_SUCCESS)
    {
        /* Did we fail because one wasn't set? */
        if ((dwError != ERROR_PATH_NOT_FOUND) &&
            (dwError != ERROR_FILE_NOT_FOUND))
        {
            /* We failed for some other reason, bail out */
            *lpdwError = dwError;
            DBG_ERR("RegQueryValueEx for the ServiceManifest parameter of the "
                    "%ws service returned %u\n",
                    pService->pszServiceName,
                    dwError);
            goto Quickie;
        }

        /* We have no manifest, make sure the buffer is empty */
        szManifestBuffer[0] = UNICODE_NULL;

        /* We're done with this buffer */
        MemFree(lpData);
        lpData = NULL;

        /* Use the whole DLL path, since we don't have a manifest */
        pszDllPath = szDllBuffer;
    }
    else
    {
        /* Do we have invalid registry data? */
        if ((dwType != REG_EXPAND_SZ) || (*lpData == UNICODE_NULL))
        {
            /* Yes, pretend there's no manifest and bail out */
            *lpdwError = ERROR_FILE_NOT_FOUND;
            DBG_ERR("The ServiceManifest parameter for the %ws service is not "
                    "of type REG_EXPAND_SZ\n",
                    pService->pszServiceName);
            goto Quickie;
        }

        /* Expand the string into our stack buffer */
        ExpandEnvironmentStringsW(lpData, szManifestBuffer, MAX_PATH);

        /* We no longer need the heap buffer*/
        MemFree(lpData);
        lpData = NULL;

        /* Lowercase the manifest path */
        SvchostCharLowerW(szManifestBuffer);

        /* Now loop over the DLL path */
        cbDllLength = lstrlenW(szDllBuffer);
        while (cbDllLength)
        {
            /* From the end, until we find the first path separator */
            if (szDllBuffer[cbDllLength] == '\\' || szDllBuffer[cbDllLength] == '/')
            {
                /* Use just a short name (cut out the rest of the path) */
                pszDllPath = &szDllBuffer[cbDllLength + 1];
                break;
            }

            /* Try the next character */
            cbDllLength--;
        }
    }

    /* See if we already have a matching DLL and manifest for this service */
    pDll = FindDll(szManifestBuffer, pszDllPath, pService);
    if (pDll == NULL)
    {
        /* We don't have it yet -- does this DLL have a manifest? */
        if (szManifestBuffer[0] != UNICODE_NULL)
        {
            /* Create an activation context for it */
            actCtx.lpSource = szManifestBuffer;
            hActCtx = CreateActCtxW(&actCtx);
            if (hActCtx == INVALID_HANDLE_VALUE)
            {
                /* We've failed to create one, bail out */
                *lpdwError = GetLastError();
                DBG_ERR("CreateActCtxW(%ws) for the %ws service returned %u\n",
                        szManifestBuffer,
                        pService->pszServiceName,
                        *lpdwError);
                goto Quickie;
            }
        }

        /* Add this new DLL into the database */
        pDll = AddDll(szManifestBuffer, pszDllPath, pService, lpdwError);
        if (pDll)
        {
            /* Save the activation context and zero it so we don't release later */
            pDll->hActCtx = hActCtx;
            hActCtx = NULL;
        }
    }

    /* We either found, added, or failed to add, the DLL for this service */
    ASSERT(!pService->pDll);
    pService->pDll = pDll;

    /* In all cases, we will query the ServiceMain function, however*/
    RegQueryStringA(hKey,
                    L"ServiceMain",
                    REG_SZ,
                    &pService->pszServiceMain);

    /* And now we'll check if we were able to create it earlier (or find it) */
    if (!pService->pDll)
    {
        /* We were not, so there's no point in going on */
        ASSERT(*lpdwError != NO_ERROR);
        goto Quickie;
    }

    /* We do have a valid DLL, so now get the service main routine out of it */
HaveDll:
    *pServiceMain = GetServiceDllFunction(pDll,
                                          pService->pszServiceMain ?
                                          pService->pszServiceMain :
                                          "ServiceMain",
                                          lpdwError);

    /* And now get the globals routine out of it (this one is optional) */
    *pPushServiceGlobals = GetServiceDllFunction(pDll,
                                                 "SvchostPushServiceGlobals",
                                                 NULL);

Quickie:
    /* We're done, cleanup any resources we left behind */
    if (hKey != NULL) RegCloseKey(hKey);
    if (lpData != NULL) MemFree(lpData);
    if ((hActCtx) && (hActCtx != INVALID_HANDLE_VALUE)) ReleaseActCtx(hActCtx);
}

VOID
WINAPI
ServiceStarter (
    _In_ DWORD dwNumServicesArgs,
    _In_ LPWSTR *lpServiceArgVectors
    )
{
    DWORD i, dwError = ERROR_FILE_NOT_FOUND;
    PSVCHOST_SERVICE ServiceEntry;
    LPCWSTR lpServiceName;
    LPSERVICE_MAIN_FUNCTIONW ServiceMain = NULL;
    PSVCHOST_INIT_GLOBALS ServiceInitGlobals = NULL;

    /* Hold the lock while we start a service */
    EnterCriticalSection(&ListLock);

    /* Get this service's name, and loop the database */
    lpServiceName = *lpServiceArgVectors;
    for (i = 0; i < ServiceCount; i++)
    {
        /* Try to find a match by name */
        if (!lstrcmpiW(lpServiceName, ServiceArray[i].pszServiceName)) break;
    }

    /* Check if we didn't find it */
    if (i == ServiceCount)
    {
        /* This looks like a bogus attempt, bail out */
        LeaveCriticalSection(&ListLock);
        return;
    }

    /* We have it */
    ServiceEntry = &ServiceArray[i];

    /* Should we breakpoint before loading this service? */
    if (FDebugBreakForService(lpServiceName) != FALSE)
    {
        /* Yep -- do it */
        DBG_TRACE("Attaching debugger before getting ServiceMain for %ws...\n",
                  lpServiceName);
        DebugBreak();
    }

    /* Get the callbacks for this service */
    GetServiceMainFunctions(&ServiceArray[i],
                            (PVOID*)&ServiceMain,
                            (PVOID*)&ServiceInitGlobals,
                            &dwError);

    /* If this service is valid and needs globals, but we don't have them... */
    if ((ServiceMain != NULL) &&
        (ServiceInitGlobals != NULL) &&
        (g_pSvchostSharedGlobals == NULL))
    {
        /* Go and build them for the first time! */
        SvchostBuildSharedGlobals();
    }

    /* We call a service if it has a main, or if wants globals and we have them */
    if (((ServiceInitGlobals != NULL) && (g_pSvchostSharedGlobals != NULL)) ||
        ((ServiceMain != NULL) && (ServiceInitGlobals == NULL)))
    {
        /* Therefore, make sure it won't be unloaded as we drop the lock */
        ServiceEntry->cServiceActiveThreadCount++;
    }

    /* Drop the lock while we call into the service DLL */
    LeaveCriticalSection(&ListLock);

    /* First: does this service want globals? */
    if (ServiceInitGlobals != NULL)
    {
        /* Do we have any to give? */
        if (g_pSvchostSharedGlobals != NULL)
        {
            /* Yes, push them to the service */
            ServiceInitGlobals(g_pSvchostSharedGlobals);

            /* Does the service NOT have an entrypoint? */
            if (ServiceMain == NULL)
            {
                /* We're going to abort loading, it serves no use */
                if (lpServiceName != NULL)
                {
                    AbortSvchostService(lpServiceName, dwError);
                }

                /* And drop a reference count since it's not active anymore */
                UnloadServiceDll(ServiceEntry);
            }
        }
        else if (lpServiceName != NULL)
        {
            /* We have no globals to give, so don't even bother calling it */
            AbortSvchostService(lpServiceName, dwError);
        }
    }

    /* Next up, does it have an entrypoint? */
    if (ServiceMain != NULL)
    {
        /* It does, so call it */
        DBG_TRACE("Calling ServiceMain for %ws...\n", lpServiceName);
        ServiceMain(dwNumServicesArgs, lpServiceArgVectors);

        /* We're out of the service now, so we can drop a reference */
        UnloadServiceDll(ServiceEntry);
    }
    else if (lpServiceName != NULL)
    {
        /* It has no entrypoint, so abort its launch */
        AbortSvchostService(lpServiceName, dwError);
    }
}

SERVICE_TABLE_ENTRYW*
WINAPI
BuildServiceTable (
    VOID
    )
{
    SERVICE_TABLE_ENTRYW *pServiceTable;
    ULONG i;

    /* Acquire the database lock while we go over the services */
    EnterCriticalSection(&ListLock);

    /* Allocate space for a NULL entry at the end as well, Windows needs this */
    pServiceTable = MemAlloc(HEAP_ZERO_MEMORY,
                             (ServiceCount + 1) * sizeof(*pServiceTable));
    if (pServiceTable)
    {
        /* Go over all our registered services */
        for (i = 0; i < ServiceCount; i++)
        {
            /* And set their parameters in the service table */
            pServiceTable[i].lpServiceName = (LPWSTR)ServiceArray[i].pszServiceName;
            pServiceTable[i].lpServiceProc = ServiceStarter;
            DBG_TRACE("Added service table entry for %ws\n",
                      pServiceTable[i].lpServiceName);
        }
    }

    /* All done, can release the lock now */
    LeaveCriticalSection(&ListLock);
    return pServiceTable;
}

BOOLEAN
WINAPI
CallPerInstanceInitFunctions (
    _In_ PSVCHOST_OPTIONS pOptions
    )
{
    PIMAGE_NT_HEADERS pNtHeaders;
    BOOLEAN bResult;

    /* Is COM required for this host? */
    if (pOptions->CoInitializeSecurityParam != 0)
    {
        /* Yes, initialize COM security and parameters */
        bResult = InitializeSecurity(pOptions->CoInitializeSecurityParam,
                                     pOptions->AuthenticationLevel,
                                     pOptions->ImpersonationLevel,
                                     pOptions->AuthenticationCapabilities);
        if (bResult != TRUE) return FALSE;
    }

    /* Do we have a custom RPC stack size? */
    if (pOptions->DefaultRpcStackSize != 0)
    {
        /* Yes, go set it */
        RpcMgmtSetServerStackSize(pOptions->DefaultRpcStackSize << 10);
    }
    else
    {
        /* Nope, get the NT headers from the image */
        pNtHeaders = RtlImageNtHeader(NtCurrentPeb()->ImageBaseAddress);
        if (pNtHeaders)
        {
            /* And just use whatever the default thread stack is */
            RpcMgmtSetServerStackSize(pNtHeaders->OptionalHeader.SizeOfStackCommit);
        }
    }

    /* Is this host holding critical services? */
    if (pOptions->SystemCritical != FALSE)
    {
        /* Mark the process as critical if so */
        RtlSetProcessIsCritical(TRUE, NULL, TRUE);
    }

    /* All done */
    return TRUE;
}

VOID
wmainCRTStartup (
    VOID
    )
{
    PSVCHOST_OPTIONS lpOptions;
    SERVICE_TABLE_ENTRYW *pServiceTable;
    LPWSTR pszCmdLine;

    /* Set a generic SEH filter and hard fail all critical errors */
    SetUnhandledExceptionFilter(SvchostUnhandledExceptionFilter);
    SetErrorMode(SEM_FAILCRITICALERRORS);

    /* Initialize the heap allocator */
    MemInit(GetProcessHeap());

    /* Initialize the DLL database and lock */
    InitializeListHead(&DllList);
    InitializeCriticalSection(&ListLock);

    /* Get the command-line and parse it to get the service group */
    pszCmdLine = GetCommandLineW();
    lpOptions = BuildCommandOptions(pszCmdLine);
    if (lpOptions == NULL)
    {
        /* Without a valid command-line, there's nothing for us to do */
        DBG_TRACE("Calling ExitProcess for %ws\n", pszCmdLine);
        ExitProcess(0);
    }

    /* Now use the service group information to lookup all the services */
    BuildServiceArray(lpOptions);

    /* Convert the list of services in this group to the SCM format */
    pServiceTable = BuildServiceTable();
    if (pServiceTable == NULL)
    {
        /* This is critical, bail out without it */
        MemFree(lpOptions);
        return;
    }

    /* Initialize COM and RPC as needed for this service group */
    if (CallPerInstanceInitFunctions(lpOptions) == FALSE)
    {
        /* Exit with a special code indicating COM/RPC setup has failed */
        DBG_ERR("%s", "CallPerInstanceInitFunctions failed -- exiting!\n");
        ExitProcess(1);
    }

    /* We're ready to go -- free the options buffer */
    MemFree(lpOptions);

    /* And call into ADVAPI32 to get our services going */
    StartServiceCtrlDispatcherW(pServiceTable);
}
