/*
 * PROJECT:     ReactOS advapi32
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/advapi32/service/sctrl.c
 * PURPOSE:     Service control manager functions
 * COPYRIGHT:   Copyright 1999 Emanuele Aliberti
 *              Copyright 2007 Ged Murphy <gedmurphy@reactos.org>
 *                             Gregor Brunmar <gregor.brunmar@home.se>
 */


/* INCLUDES ******************************************************************/

#include <advapi32.h>
#include <pseh/pseh2.h>

WINE_DEFAULT_DEBUG_CHANNEL(advapi);


/* TYPES *********************************************************************/

typedef struct _SERVICE_THREAD_PARAMSA
{
    LPSERVICE_MAIN_FUNCTIONA lpServiceMain;
    DWORD dwArgCount;
    LPSTR *lpArgVector;
    DWORD dwServiceTag;
} SERVICE_THREAD_PARAMSA, *PSERVICE_THREAD_PARAMSA;


typedef struct _SERVICE_THREAD_PARAMSW
{
    LPSERVICE_MAIN_FUNCTIONW lpServiceMain;
    DWORD dwArgCount;
    LPWSTR *lpArgVector;
    DWORD dwServiceTag;
} SERVICE_THREAD_PARAMSW, *PSERVICE_THREAD_PARAMSW;


typedef struct _ACTIVE_SERVICE
{
    SERVICE_STATUS_HANDLE hServiceStatus;
    UNICODE_STRING ServiceName;
    union
    {
        LPSERVICE_MAIN_FUNCTIONA A;
        LPSERVICE_MAIN_FUNCTIONW W;
    } ServiceMain;
    LPHANDLER_FUNCTION HandlerFunction;
    LPHANDLER_FUNCTION_EX HandlerFunctionEx;
    LPVOID HandlerContext;
    BOOL bUnicode;
    BOOL bOwnProcess;
    DWORD dwServiceTag;
} ACTIVE_SERVICE, *PACTIVE_SERVICE;


/* GLOBALS *******************************************************************/

static DWORD dwActiveServiceCount = 0;
static PACTIVE_SERVICE lpActiveServices = NULL;
static handle_t hStatusBinding = NULL;
static BOOL bSecurityServiceProcess = FALSE;


/* FUNCTIONS *****************************************************************/

handle_t __RPC_USER
RPC_SERVICE_STATUS_HANDLE_bind(RPC_SERVICE_STATUS_HANDLE hServiceStatus)
{
    return hStatusBinding;
}


void __RPC_USER
RPC_SERVICE_STATUS_HANDLE_unbind(RPC_SERVICE_STATUS_HANDLE hServiceStatus,
                                 handle_t hBinding)
{
}


static RPC_STATUS
ScCreateStatusBinding(VOID)
{
    LPWSTR pszStringBinding;
    RPC_STATUS status;

    TRACE("ScCreateStatusBinding()\n");

    status = RpcStringBindingComposeW(NULL,
                                      L"ncacn_np",
                                      NULL,
                                      L"\\pipe\\ntsvcs",
                                      NULL,
                                      &pszStringBinding);
    if (status != RPC_S_OK)
    {
        ERR("RpcStringBindingCompose returned 0x%x\n", status);
        return status;
    }

    /* Set the binding handle that will be used to bind to the server. */
    status = RpcBindingFromStringBindingW(pszStringBinding,
                                          &hStatusBinding);
    if (status != RPC_S_OK)
    {
        ERR("RpcBindingFromStringBinding returned 0x%x\n", status);
    }

    status = RpcStringFreeW(&pszStringBinding);
    if (status != RPC_S_OK)
    {
        ERR("RpcStringFree returned 0x%x\n", status);
    }

    return status;
}


static RPC_STATUS
ScDestroyStatusBinding(VOID)
{
    RPC_STATUS status;

    TRACE("ScDestroyStatusBinding()\n");

    if (hStatusBinding == NULL)
        return RPC_S_OK;

    status = RpcBindingFree(&hStatusBinding);
    if (status != RPC_S_OK)
    {
        ERR("RpcBindingFree returned 0x%x\n", status);
    }
    else
    {
        hStatusBinding = NULL;
    }

    return status;
}


static PACTIVE_SERVICE
ScLookupServiceByServiceName(LPCWSTR lpServiceName)
{
    DWORD i;

    TRACE("ScLookupServiceByServiceName(%S)\n",
          lpServiceName);

    if (lpActiveServices[0].bOwnProcess)
        return &lpActiveServices[0];

    for (i = 0; i < dwActiveServiceCount; i++)
    {
        TRACE("Checking %S\n", lpActiveServices[i].ServiceName.Buffer);
        if (_wcsicmp(lpActiveServices[i].ServiceName.Buffer, lpServiceName) == 0)
        {
            TRACE("Found!\n");
            return &lpActiveServices[i];
        }
    }

    TRACE("No service found!\n");
    return NULL;
}


static DWORD WINAPI
ScServiceMainStubA(LPVOID Context)
{
    PTEB Teb;
    PSERVICE_THREAD_PARAMSA ThreadParams = Context;

    TRACE("ScServiceMainStubA(%p)\n", Context);

    /* Set service tag */
    Teb = NtCurrentTeb();
    Teb->SubProcessTag = UlongToPtr(ThreadParams->dwServiceTag);

    /* Call the main service routine and free the arguments vector */
    (ThreadParams->lpServiceMain)(ThreadParams->dwArgCount,
                                  ThreadParams->lpArgVector);

    /* Reset service tag */
    Teb->SubProcessTag = 0;

    if (ThreadParams->lpArgVector != NULL)
    {
        HeapFree(GetProcessHeap(), 0, ThreadParams->lpArgVector);
    }
    HeapFree(GetProcessHeap(), 0, ThreadParams);

    return ERROR_SUCCESS;
}


static DWORD WINAPI
ScServiceMainStubW(LPVOID Context)
{
    PTEB Teb;
    PSERVICE_THREAD_PARAMSW ThreadParams = Context;

    TRACE("ScServiceMainStubW(%p)\n", Context);

    /* Set service tag */
    Teb = NtCurrentTeb();
    Teb->SubProcessTag = UlongToPtr(ThreadParams->dwServiceTag);

    /* Call the main service routine and free the arguments vector */
    (ThreadParams->lpServiceMain)(ThreadParams->dwArgCount,
                                  ThreadParams->lpArgVector);

    /* Reset service tag */
    Teb->SubProcessTag = 0;

    if (ThreadParams->lpArgVector != NULL)
    {
        HeapFree(GetProcessHeap(), 0, ThreadParams->lpArgVector);
    }
    HeapFree(GetProcessHeap(), 0, ThreadParams);

    return ERROR_SUCCESS;
}


static DWORD
ScConnectControlPipe(HANDLE *hPipe)
{
    DWORD dwBytesWritten;
    DWORD dwState;
    DWORD dwServiceCurrent = 1;
    NTSTATUS Status;
    WCHAR NtControlPipeName[MAX_PATH + 1];
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
    DWORD dwProcessId;

    TRACE("ScConnectControlPipe(%p)\n",
          hPipe);

    /* Get the service number and create the named pipe */
    if (bSecurityServiceProcess == FALSE)
    {
        RtlZeroMemory(&QueryTable, sizeof(QueryTable));

        QueryTable[0].Name = L"";
        QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
        QueryTable[0].EntryContext = &dwServiceCurrent;

        Status = RtlQueryRegistryValues(RTL_REGISTRY_CONTROL,
                                        L"ServiceCurrent",
                                        QueryTable,
                                        NULL,
                                        NULL);
        if (!NT_SUCCESS(Status))
        {
            ERR("RtlQueryRegistryValues() failed (Status %lx)\n", Status);
            return RtlNtStatusToDosError(Status);
        }
    }
    else
    {
        dwServiceCurrent = 0;
    }

    swprintf(NtControlPipeName, L"\\\\.\\pipe\\net\\NtControlPipe%u", dwServiceCurrent);
    TRACE("PipeName: %S\n", NtControlPipeName);

    if (!WaitNamedPipeW(NtControlPipeName, 30000))
    {
        ERR("WaitNamedPipe(%S) failed (Error %lu)\n", NtControlPipeName, GetLastError());
        return ERROR_FAILED_SERVICE_CONTROLLER_CONNECT;
    }

    *hPipe = CreateFileW(NtControlPipeName,
                         GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL);
    if (*hPipe == INVALID_HANDLE_VALUE)
    {
        ERR("CreateFileW() failed for pipe %S (Error %lu)\n", NtControlPipeName, GetLastError());
        return ERROR_FAILED_SERVICE_CONTROLLER_CONNECT;
    }

    dwState = PIPE_READMODE_MESSAGE;
    if (!SetNamedPipeHandleState(*hPipe, &dwState, NULL, NULL))
    {
        CloseHandle(*hPipe);
        *hPipe = INVALID_HANDLE_VALUE;
        return ERROR_FAILED_SERVICE_CONTROLLER_CONNECT;
    }

    /* Pass the ProcessId to the SCM */
    dwProcessId = GetCurrentProcessId();
    WriteFile(*hPipe,
              &dwProcessId,
              sizeof(dwProcessId),
              &dwBytesWritten,
              NULL);

    TRACE("Sent Process ID %lu\n", dwProcessId);

    return ERROR_SUCCESS;
}


/*
 * Ansi/Unicode argument layout of the vector passed to a service at startup,
 * depending on the different versions of Windows considered:
 *
 * - XP/2003:
 *   [argv array of pointers][parameter 1][parameter 2]...[service name]
 *
 * - Vista:
 *   [argv array of pointers][align to 8 bytes]
 *   [parameter 1][parameter 2]...[service name]
 *
 * - Win7/8:
 *   [argv array of pointers][service name]
 *   [parameter 1][align to 4 bytes][parameter 2][align to 4 bytes]...
 *
 * Space for parameters and service name is always enough to store
 * both the Ansi and the Unicode versions including NULL terminator.
 */

static DWORD
ScBuildUnicodeArgsVector(PSCM_CONTROL_PACKET ControlPacket,
                         LPDWORD lpArgCount,
                         LPWSTR **lpArgVector)
{
    PWSTR *lpVector;
    PWSTR pszServiceName;
    DWORD cbServiceName;
    DWORD cbArguments;
    DWORD cbTotal;
    DWORD i;

    if (ControlPacket == NULL || lpArgCount == NULL || lpArgVector == NULL)
        return ERROR_INVALID_PARAMETER;

    *lpArgCount  = 0;
    *lpArgVector = NULL;

    /* Retrieve and count the start command line (NULL-terminated) */
    pszServiceName = (PWSTR)((ULONG_PTR)ControlPacket + ControlPacket->dwServiceNameOffset);
    cbServiceName  = lstrlenW(pszServiceName) * sizeof(WCHAR) + sizeof(UNICODE_NULL);

    /*
     * The total size of the argument vector is equal to the entry for
     * the service name, plus the size of the original argument vector.
     */
    cbTotal = sizeof(PWSTR) + cbServiceName;
    if (ControlPacket->dwArgumentsCount > 0)
        cbArguments = ControlPacket->dwSize - ControlPacket->dwArgumentsOffset;
    else
        cbArguments = 0;
    cbTotal += cbArguments;

    /* Allocate the new argument vector */
    lpVector = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cbTotal);
    if (lpVector == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    /*
     * The first argument is reserved for the service name, which
     * will be appended to the end of the argument string list.
     */

    /* Copy the remaining arguments */
    if (ControlPacket->dwArgumentsCount > 0)
    {
        memcpy(&lpVector[1],
               (PWSTR)((ULONG_PTR)ControlPacket + ControlPacket->dwArgumentsOffset),
               cbArguments);

        for (i = 0; i < ControlPacket->dwArgumentsCount; i++)
        {
            lpVector[i + 1] = (PWSTR)((ULONG_PTR)&lpVector[1] + (ULONG_PTR)lpVector[i + 1]);
            TRACE("Unicode lpVector[%lu] = '%ls'\n", i + 1, lpVector[i + 1]);
        }
    }

    /* Now copy the service name */
    lpVector[0] = (PWSTR)((ULONG_PTR)&lpVector[1] + cbArguments);
    memcpy(lpVector[0], pszServiceName, cbServiceName);
    TRACE("Unicode lpVector[%lu] = '%ls'\n", 0, lpVector[0]);

    *lpArgCount  = ControlPacket->dwArgumentsCount + 1;
    *lpArgVector = lpVector;

    return ERROR_SUCCESS;
}


static DWORD
ScBuildAnsiArgsVector(PSCM_CONTROL_PACKET ControlPacket,
                      LPDWORD lpArgCount,
                      LPSTR **lpArgVector)
{
    DWORD dwError;
    NTSTATUS Status;
    DWORD ArgCount, i;
    PWSTR *lpVectorW;
    PSTR  *lpVectorA;
    UNICODE_STRING UnicodeString;
    ANSI_STRING AnsiString;

    if (ControlPacket == NULL || lpArgCount == NULL || lpArgVector == NULL)
        return ERROR_INVALID_PARAMETER;

    *lpArgCount  = 0;
    *lpArgVector = NULL;

    /* Build the UNICODE arguments vector */
    dwError = ScBuildUnicodeArgsVector(ControlPacket, &ArgCount, &lpVectorW);
    if (dwError != ERROR_SUCCESS)
        return dwError;

    /* Convert the vector to ANSI in place */
    lpVectorA = (PSTR*)lpVectorW;
    for (i = 0; i < ArgCount; i++)
    {
        RtlInitUnicodeString(&UnicodeString, lpVectorW[i]);
        RtlInitEmptyAnsiString(&AnsiString, lpVectorA[i], UnicodeString.MaximumLength);
        Status = RtlUnicodeStringToAnsiString(&AnsiString, &UnicodeString, FALSE);
        if (!NT_SUCCESS(Status))
        {
            /* Failed to convert to ANSI; free the allocated vector and return */
            dwError = RtlNtStatusToDosError(Status);
            HeapFree(GetProcessHeap(), 0, lpVectorW);
            return dwError;
        }

        /* NULL-terminate the string */
        AnsiString.Buffer[AnsiString.Length / sizeof(CHAR)] = ANSI_NULL;

        TRACE("Ansi lpVector[%lu] = '%s'\n", i, lpVectorA[i]);
    }

    *lpArgCount  = ArgCount;
    *lpArgVector = lpVectorA;

    return ERROR_SUCCESS;
}


static DWORD
ScStartService(PACTIVE_SERVICE lpService,
               PSCM_CONTROL_PACKET ControlPacket)
{
    HANDLE ThreadHandle;
    DWORD ThreadId;
    DWORD dwError;
    PSERVICE_THREAD_PARAMSA ThreadParamsA;
    PSERVICE_THREAD_PARAMSW ThreadParamsW;

    TRACE("ScStartService(%p %p)\n",
          lpService, ControlPacket);

    if (lpService == NULL || ControlPacket == NULL)
        return ERROR_INVALID_PARAMETER;

    TRACE("Size: %lu\n", ControlPacket->dwSize);
    TRACE("Service: %S\n", (PWSTR)((ULONG_PTR)ControlPacket + ControlPacket->dwServiceNameOffset));

    /* Set the service status handle */
    lpService->hServiceStatus = ControlPacket->hServiceStatus;
    /* Set the service tag */
    lpService->dwServiceTag = ControlPacket->dwServiceTag;

    /* Build the arguments vector */
    if (lpService->bUnicode != FALSE)
    {
        ThreadParamsW = HeapAlloc(GetProcessHeap(), 0, sizeof(*ThreadParamsW));
        if (ThreadParamsW == NULL)
            return ERROR_NOT_ENOUGH_MEMORY;
        dwError = ScBuildUnicodeArgsVector(ControlPacket,
                                           &ThreadParamsW->dwArgCount,
                                           &ThreadParamsW->lpArgVector);
        if (dwError != ERROR_SUCCESS)
        {
            HeapFree(GetProcessHeap(), 0, ThreadParamsW);
            return dwError;
        }
        ThreadParamsW->lpServiceMain = lpService->ServiceMain.W;
        ThreadParamsW->dwServiceTag = ControlPacket->dwServiceTag;
        ThreadHandle = CreateThread(NULL,
                                    0,
                                    ScServiceMainStubW,
                                    ThreadParamsW,
                                    0,
                                    &ThreadId);
        if (ThreadHandle == NULL)
        {
            if (ThreadParamsW->lpArgVector != NULL)
            {
                HeapFree(GetProcessHeap(), 0, ThreadParamsW->lpArgVector);
            }
            HeapFree(GetProcessHeap(), 0, ThreadParamsW);

            return ERROR_SERVICE_NO_THREAD;
        }

        CloseHandle(ThreadHandle);
    }
    else
    {
        ThreadParamsA = HeapAlloc(GetProcessHeap(), 0, sizeof(*ThreadParamsA));
        if (ThreadParamsA == NULL)
            return ERROR_NOT_ENOUGH_MEMORY;
        dwError = ScBuildAnsiArgsVector(ControlPacket,
                                        &ThreadParamsA->dwArgCount,
                                        &ThreadParamsA->lpArgVector);
        if (dwError != ERROR_SUCCESS)
        {
            HeapFree(GetProcessHeap(), 0, ThreadParamsA);
            return dwError;
        }
        ThreadParamsA->lpServiceMain = lpService->ServiceMain.A;
        ThreadParamsA->dwServiceTag = ControlPacket->dwServiceTag;
        ThreadHandle = CreateThread(NULL,
                                    0,
                                    ScServiceMainStubA,
                                    ThreadParamsA,
                                    0,
                                    &ThreadId);
        if (ThreadHandle == NULL)
        {
            if (ThreadParamsA->lpArgVector != NULL)
            {
                HeapFree(GetProcessHeap(), 0, ThreadParamsA->lpArgVector);
            }
            HeapFree(GetProcessHeap(), 0, ThreadParamsA);

            return ERROR_SERVICE_NO_THREAD;
        }

        CloseHandle(ThreadHandle);
    }

    return ERROR_SUCCESS;
}


static DWORD
ScControlService(PACTIVE_SERVICE lpService,
                 PSCM_CONTROL_PACKET ControlPacket)
{
    DWORD dwError = ERROR_SUCCESS;

    TRACE("ScControlService(%p %p)\n",
          lpService, ControlPacket);

    if (lpService == NULL || ControlPacket == NULL)
        return ERROR_INVALID_PARAMETER;

    TRACE("Size: %lu\n", ControlPacket->dwSize);
    TRACE("Service: %S\n", (PWSTR)((ULONG_PTR)ControlPacket + ControlPacket->dwServiceNameOffset));

    /* Set service tag */
    NtCurrentTeb()->SubProcessTag = UlongToPtr(lpService->dwServiceTag);

    if (lpService->HandlerFunction)
    {
        _SEH2_TRY
        {
            (lpService->HandlerFunction)(ControlPacket->dwControl);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            dwError = ERROR_EXCEPTION_IN_SERVICE;
        }
        _SEH2_END;
    }
    else if (lpService->HandlerFunctionEx)
    {
        _SEH2_TRY
        {
            /* FIXME: Send correct 2nd and 3rd parameters */
            (lpService->HandlerFunctionEx)(ControlPacket->dwControl,
                                           0, NULL,
                                           lpService->HandlerContext);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            dwError = ERROR_EXCEPTION_IN_SERVICE;
        }
        _SEH2_END;
    }
    else
    {
        dwError = ERROR_SERVICE_CANNOT_ACCEPT_CTRL;
    }

    /* Reset service tag */
    NtCurrentTeb()->SubProcessTag = 0;

    TRACE("ScControlService() done (Error %lu)\n", dwError);

    return dwError;
}


static BOOL
ScServiceDispatcher(HANDLE hPipe,
                    PSCM_CONTROL_PACKET ControlPacket,
                    DWORD dwBufferSize)
{
    DWORD Count;
    BOOL bResult;
    BOOL bRunning = TRUE;
    LPWSTR lpServiceName;
    PACTIVE_SERVICE lpService;
    SCM_REPLY_PACKET ReplyPacket;
    DWORD dwError;

    TRACE("ScServiceDispatcher(%p %p %lu)\n",
          hPipe, ControlPacket, dwBufferSize);

    if (ControlPacket == NULL || dwBufferSize < sizeof(SCM_CONTROL_PACKET))
        return FALSE;

    while (bRunning)
    {
        /* Read command from the control pipe */
        bResult = ReadFile(hPipe,
                           ControlPacket,
                           dwBufferSize,
                           &Count,
                           NULL);
        if (bResult == FALSE)
        {
            ERR("Pipe read failed (Error: %lu)\n", GetLastError());
            return FALSE;
        }

        lpServiceName = (LPWSTR)((PBYTE)ControlPacket + ControlPacket->dwServiceNameOffset);
        TRACE("Service: %S\n", lpServiceName);

        if ((ControlPacket->dwControl == SERVICE_CONTROL_STOP) &&
            (lpServiceName[0] == UNICODE_NULL))
        {
            TRACE("Stop dispatcher thread\n");
            bRunning = FALSE;
            dwError = ERROR_SUCCESS;
        }
        else
        {
            if (ControlPacket->dwControl == SERVICE_CONTROL_START_OWN)
                lpActiveServices[0].bOwnProcess = TRUE;

            lpService = ScLookupServiceByServiceName(lpServiceName);
            if (lpService != NULL)
            {
                /* Execute command */
                switch (ControlPacket->dwControl)
                {
                    case SERVICE_CONTROL_START_SHARE:
                    case SERVICE_CONTROL_START_OWN:
                        TRACE("Start command - received SERVICE_CONTROL_START\n");
                        dwError = ScStartService(lpService, ControlPacket);
                        break;

                    case SERVICE_CONTROL_STOP:
                        TRACE("Stop command - received SERVICE_CONTROL_STOP\n");
                        dwError = ScControlService(lpService, ControlPacket);
                        break;

                    default:
                        TRACE("Command %lu received\n", ControlPacket->dwControl);
                        dwError = ScControlService(lpService, ControlPacket);
                        break;
                }
            }
            else
            {
                dwError = ERROR_SERVICE_NOT_IN_EXE;
            }
        }

        ReplyPacket.dwError = dwError;

        /* Send the reply packet */
        bResult = WriteFile(hPipe,
                            &ReplyPacket,
                            sizeof(ReplyPacket),
                            &Count,
                            NULL);
        if (bResult == FALSE)
        {
            ERR("Pipe write failed (Error: %lu)\n", GetLastError());
            return FALSE;
        }
    }

    return TRUE;
}


/**********************************************************************
 *  RegisterServiceCtrlHandlerA
 *
 * @implemented
 */
SERVICE_STATUS_HANDLE WINAPI
RegisterServiceCtrlHandlerA(LPCSTR lpServiceName,
                            LPHANDLER_FUNCTION lpHandlerProc)
{
    ANSI_STRING ServiceNameA;
    UNICODE_STRING ServiceNameU;
    SERVICE_STATUS_HANDLE hServiceStatus;

    TRACE("RegisterServiceCtrlHandlerA(%s %p)\n",
          debugstr_a(lpServiceName), lpHandlerProc);

    RtlInitAnsiString(&ServiceNameA, lpServiceName);
    if (!NT_SUCCESS(RtlAnsiStringToUnicodeString(&ServiceNameU, &ServiceNameA, TRUE)))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    hServiceStatus = RegisterServiceCtrlHandlerW(ServiceNameU.Buffer,
                                                 lpHandlerProc);

    RtlFreeUnicodeString(&ServiceNameU);

    return hServiceStatus;
}


/**********************************************************************
 *  RegisterServiceCtrlHandlerW
 *
 * @implemented
 */
SERVICE_STATUS_HANDLE WINAPI
RegisterServiceCtrlHandlerW(LPCWSTR lpServiceName,
                            LPHANDLER_FUNCTION lpHandlerProc)
{
    PACTIVE_SERVICE Service;

    TRACE("RegisterServiceCtrlHandlerW(%s %p)\n",
          debugstr_w(lpServiceName), lpHandlerProc);

    Service = ScLookupServiceByServiceName(lpServiceName);
    if (Service == NULL)
    {
        SetLastError(ERROR_SERVICE_NOT_IN_EXE);
        return NULL;
    }

    if (!lpHandlerProc)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    Service->HandlerFunction = lpHandlerProc;
    Service->HandlerFunctionEx = NULL;

    TRACE("RegisterServiceCtrlHandler returning %p\n", Service->hServiceStatus);

    return Service->hServiceStatus;
}


/**********************************************************************
 *  RegisterServiceCtrlHandlerExA
 *
 * @implemented
 */
SERVICE_STATUS_HANDLE WINAPI
RegisterServiceCtrlHandlerExA(LPCSTR lpServiceName,
                              LPHANDLER_FUNCTION_EX lpHandlerProc,
                              LPVOID lpContext)
{
    ANSI_STRING ServiceNameA;
    UNICODE_STRING ServiceNameU;
    SERVICE_STATUS_HANDLE hServiceStatus;

    TRACE("RegisterServiceCtrlHandlerExA(%s %p %p)\n",
          debugstr_a(lpServiceName), lpHandlerProc, lpContext);

    RtlInitAnsiString(&ServiceNameA, lpServiceName);
    if (!NT_SUCCESS(RtlAnsiStringToUnicodeString(&ServiceNameU, &ServiceNameA, TRUE)))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    hServiceStatus = RegisterServiceCtrlHandlerExW(ServiceNameU.Buffer,
                                                   lpHandlerProc,
                                                   lpContext);

    RtlFreeUnicodeString(&ServiceNameU);

    return hServiceStatus;
}


/**********************************************************************
 *  RegisterServiceCtrlHandlerExW
 *
 * @implemented
 */
SERVICE_STATUS_HANDLE WINAPI
RegisterServiceCtrlHandlerExW(LPCWSTR lpServiceName,
                              LPHANDLER_FUNCTION_EX lpHandlerProc,
                              LPVOID lpContext)
{
    PACTIVE_SERVICE Service;

    TRACE("RegisterServiceCtrlHandlerExW(%s %p %p)\n",
          debugstr_w(lpServiceName), lpHandlerProc, lpContext);

    Service = ScLookupServiceByServiceName(lpServiceName);
    if (Service == NULL)
    {
        SetLastError(ERROR_SERVICE_NOT_IN_EXE);
        return NULL;
    }

    if (!lpHandlerProc)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    Service->HandlerFunction = NULL;
    Service->HandlerFunctionEx = lpHandlerProc;
    Service->HandlerContext = lpContext;

    TRACE("RegisterServiceCtrlHandlerEx returning %p\n", Service->hServiceStatus);

    return Service->hServiceStatus;
}


/**********************************************************************
 *  I_ScIsSecurityProcess
 *
 * Undocumented
 *
 * @implemented
 */
VOID
WINAPI
I_ScIsSecurityProcess(VOID)
{
    TRACE("I_ScIsSecurityProcess()\n");
    bSecurityServiceProcess = TRUE;
}


/**********************************************************************
 *  I_ScPnPGetServiceName
 *
 * Undocumented
 *
 * @implemented
 */
DWORD
WINAPI
I_ScPnPGetServiceName(IN SERVICE_STATUS_HANDLE hServiceStatus,
                      OUT LPWSTR lpServiceName,
                      IN DWORD cchServiceName)
{
    DWORD i;

    TRACE("I_ScPnPGetServiceName(%lu %p %lu)\n",
          hServiceStatus, lpServiceName, cchServiceName);

    for (i = 0; i < dwActiveServiceCount; i++)
    {
        if (lpActiveServices[i].hServiceStatus == hServiceStatus)
        {
            wcscpy(lpServiceName, lpActiveServices[i].ServiceName.Buffer);
            return ERROR_SUCCESS;
        }
    }

    return ERROR_SERVICE_NOT_IN_EXE;
}


/**********************************************************************
 *  I_ScSetServiceBitsA
 *
 * Undocumented
 *
 * @implemented
 */
BOOL WINAPI
I_ScSetServiceBitsA(SERVICE_STATUS_HANDLE hServiceStatus,
                    DWORD dwServiceBits,
                    BOOL bSetBitsOn,
                    BOOL bUpdateImmediately,
                    LPSTR lpString)
{
    BOOL bResult;

    TRACE("I_ScSetServiceBitsA(%lu %lx %u %u %s)\n",
          hServiceStatus, dwServiceBits, bSetBitsOn, bUpdateImmediately,
          debugstr_a(lpString));

    RpcTryExcept
    {
        bResult = RI_ScSetServiceBitsA((RPC_SERVICE_STATUS_HANDLE)hServiceStatus,
                                       dwServiceBits,
                                       bSetBitsOn,
                                       bUpdateImmediately,
                                       lpString);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError(ScmRpcStatusToWinError(RpcExceptionCode()));
        bResult = FALSE;
    }
    RpcEndExcept;

    return bResult;
}


/**********************************************************************
 *  I_ScSetServiceBitsW
 *
 * Undocumented
 *
 * @implemented
 */
BOOL WINAPI
I_ScSetServiceBitsW(SERVICE_STATUS_HANDLE hServiceStatus,
                    DWORD dwServiceBits,
                    BOOL bSetBitsOn,
                    BOOL bUpdateImmediately,
                    LPWSTR lpString)
{
    BOOL bResult;

    TRACE("I_ScSetServiceBitsW(%lu %lx %u %u %s)\n",
          hServiceStatus, dwServiceBits, bSetBitsOn, bUpdateImmediately,
          debugstr_w(lpString));

    RpcTryExcept
    {
        bResult = RI_ScSetServiceBitsW((RPC_SERVICE_STATUS_HANDLE)hServiceStatus,
                                       dwServiceBits,
                                       bSetBitsOn,
                                       bUpdateImmediately,
                                       lpString);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError(ScmRpcStatusToWinError(RpcExceptionCode()));
        bResult = FALSE;
    }
    RpcEndExcept;

    return bResult;
}


/**********************************************************************
 *  SetServiceBits
 *
 * @implemented
 */
BOOL WINAPI
SetServiceBits(SERVICE_STATUS_HANDLE hServiceStatus,
               DWORD dwServiceBits,
               BOOL bSetBitsOn,
               BOOL bUpdateImmediately)
{
    TRACE("SetServiceBits(%lu %lx %u %u)\n",
          hServiceStatus, dwServiceBits, bSetBitsOn, bUpdateImmediately);

    return I_ScSetServiceBitsW(hServiceStatus,
                               dwServiceBits,
                               bSetBitsOn,
                               bUpdateImmediately,
                               NULL);
}


/**********************************************************************
 *  SetServiceStatus
 *
 * @implemented
 */
BOOL WINAPI
SetServiceStatus(SERVICE_STATUS_HANDLE hServiceStatus,
                 LPSERVICE_STATUS lpServiceStatus)
{
    DWORD dwError;

    TRACE("SetServiceStatus(%lu %p)\n",
          hServiceStatus, lpServiceStatus);

    RpcTryExcept
    {
        dwError = RSetServiceStatus((RPC_SERVICE_STATUS_HANDLE)hServiceStatus,
                                    lpServiceStatus);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwError = ScmRpcStatusToWinError(RpcExceptionCode());
    }
    RpcEndExcept;

    if (dwError != ERROR_SUCCESS)
    {
        ERR("RSetServiceStatus() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    TRACE("SetServiceStatus() done\n");

    return TRUE;
}


/**********************************************************************
 *  StartServiceCtrlDispatcherA
 *
 * @implemented
 */
BOOL WINAPI
StartServiceCtrlDispatcherA(const SERVICE_TABLE_ENTRYA *lpServiceStartTable)
{
    ULONG i;
    HANDLE hPipe;
    DWORD dwError;
    PSCM_CONTROL_PACKET ControlPacket;
    DWORD dwBufSize;
    BOOL bRet = TRUE;

    TRACE("StartServiceCtrlDispatcherA(%p)\n",
          lpServiceStartTable);

    i = 0;
    while (lpServiceStartTable[i].lpServiceProc != NULL)
    {
        i++;
    }

    dwActiveServiceCount = i;

    /* Allocate the service table */
    lpActiveServices = RtlAllocateHeap(RtlGetProcessHeap(),
                                       HEAP_ZERO_MEMORY,
                                       dwActiveServiceCount * sizeof(ACTIVE_SERVICE));
    if (lpActiveServices == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    /* Copy service names and start procedure */
    for (i = 0; i < dwActiveServiceCount; i++)
    {
        RtlCreateUnicodeStringFromAsciiz(&lpActiveServices[i].ServiceName,
                                         lpServiceStartTable[i].lpServiceName);
        lpActiveServices[i].ServiceMain.A = lpServiceStartTable[i].lpServiceProc;
        lpActiveServices[i].hServiceStatus = NULL;
        lpActiveServices[i].bUnicode = FALSE;
        lpActiveServices[i].bOwnProcess = FALSE;
    }

    /* Connect to the SCM */
    dwError = ScConnectControlPipe(&hPipe);
    if (dwError != ERROR_SUCCESS)
    {
        bRet = FALSE;
        goto done;
    }

    dwBufSize = sizeof(SCM_CONTROL_PACKET) +
                (MAX_SERVICE_NAME_LENGTH + 1) * sizeof(WCHAR);

    ControlPacket = RtlAllocateHeap(RtlGetProcessHeap(),
                                    HEAP_ZERO_MEMORY,
                                    dwBufSize);
    if (ControlPacket == NULL)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        bRet = FALSE;
        goto done;
    }

    ScCreateStatusBinding();

    /* Call the dispatcher loop */
    ScServiceDispatcher(hPipe, ControlPacket, dwBufSize);


    ScDestroyStatusBinding();

    /* Close the connection */
    CloseHandle(hPipe);

    /* Free the control packet */
    RtlFreeHeap(RtlGetProcessHeap(), 0, ControlPacket);

done:
    /* Free the service table */
    for (i = 0; i < dwActiveServiceCount; i++)
    {
        RtlFreeUnicodeString(&lpActiveServices[i].ServiceName);
    }
    RtlFreeHeap(RtlGetProcessHeap(), 0, lpActiveServices);
    lpActiveServices = NULL;
    dwActiveServiceCount = 0;

    if (!bRet)
        SetLastError(dwError);

    return bRet;
}


/**********************************************************************
 *  StartServiceCtrlDispatcherW
 *
 * @implemented
 */
BOOL WINAPI
StartServiceCtrlDispatcherW(const SERVICE_TABLE_ENTRYW *lpServiceStartTable)
{
    ULONG i;
    HANDLE hPipe;
    DWORD dwError;
    PSCM_CONTROL_PACKET ControlPacket;
    DWORD dwBufSize;
    BOOL bRet = TRUE;

    TRACE("StartServiceCtrlDispatcherW(%p)\n",
          lpServiceStartTable);

    i = 0;
    while (lpServiceStartTable[i].lpServiceProc != NULL)
    {
        i++;
    }

    dwActiveServiceCount = i;

    /* Allocate the service table */
    lpActiveServices = RtlAllocateHeap(RtlGetProcessHeap(),
                                       HEAP_ZERO_MEMORY,
                                       dwActiveServiceCount * sizeof(ACTIVE_SERVICE));
    if (lpActiveServices == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    /* Copy service names and start procedure */
    for (i = 0; i < dwActiveServiceCount; i++)
    {
        RtlCreateUnicodeString(&lpActiveServices[i].ServiceName,
                               lpServiceStartTable[i].lpServiceName);
        lpActiveServices[i].ServiceMain.W = lpServiceStartTable[i].lpServiceProc;
        lpActiveServices[i].hServiceStatus = NULL;
        lpActiveServices[i].bUnicode = TRUE;
        lpActiveServices[i].bOwnProcess = FALSE;
    }

    /* Connect to the SCM */
    dwError = ScConnectControlPipe(&hPipe);
    if (dwError != ERROR_SUCCESS)
    {
        bRet = FALSE;
        goto done;
    }

    dwBufSize = sizeof(SCM_CONTROL_PACKET) +
                (MAX_SERVICE_NAME_LENGTH + 1) * sizeof(WCHAR);

    ControlPacket = RtlAllocateHeap(RtlGetProcessHeap(),
                                    HEAP_ZERO_MEMORY,
                                    dwBufSize);
    if (ControlPacket == NULL)
    {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        bRet = FALSE;
        goto done;
    }

    ScCreateStatusBinding();

    /* Call the dispatcher loop */
    ScServiceDispatcher(hPipe, ControlPacket, dwBufSize);

    ScDestroyStatusBinding();

    /* Close the connection */
    CloseHandle(hPipe);

    /* Free the control packet */
    RtlFreeHeap(RtlGetProcessHeap(), 0, ControlPacket);

done:
    /* Free the service table */
    for (i = 0; i < dwActiveServiceCount; i++)
    {
        RtlFreeUnicodeString(&lpActiveServices[i].ServiceName);
    }
    RtlFreeHeap(RtlGetProcessHeap(), 0, lpActiveServices);
    lpActiveServices = NULL;
    dwActiveServiceCount = 0;

    if (!bRet)
        SetLastError(dwError);

    return bRet;
}

/* EOF */
