/*
 * PROJECT:     ReactOS advapi32
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/advapi32/service/sctrl.c
 * PURPOSE:     Service control manager functions
 * COPYRIGHT:   Copyright 1999 Emanuele Aliberti
 *              Copyright 2007 Ged Murphy <gedmurphy@reactos.org>
 *                             Gregor Brunmar <gregor.brunmar@home.se>
 *
 */


/* INCLUDES ******************************************************************/

#include <advapi32.h>
WINE_DEFAULT_DEBUG_CHANNEL(advapi);


/* TYPES *********************************************************************/

typedef struct _SERVICE_THREAD_PARAMSA
{
    LPSERVICE_MAIN_FUNCTIONA lpServiceMain;
    DWORD dwArgCount;
    LPSTR *lpArgVector;
} SERVICE_THREAD_PARAMSA, *PSERVICE_THREAD_PARAMSA;


typedef struct _SERVICE_THREAD_PARAMSW
{
    LPSERVICE_MAIN_FUNCTIONW lpServiceMain;
    DWORD dwArgCount;
    LPWSTR *lpArgVector;
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
} ACTIVE_SERVICE, *PACTIVE_SERVICE;


/* GLOBALS *******************************************************************/

static DWORD dwActiveServiceCount = 0;
static PACTIVE_SERVICE lpActiveServices = NULL;
static handle_t hStatusBinding = NULL;


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

    TRACE("ScCreateStatusBinding() called\n");

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

    TRACE("ScDestroyStatusBinding() called\n");

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

    TRACE("ScLookupServiceByServiceName(%S) called\n", lpServiceName);

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

    SetLastError(ERROR_SERVICE_DOES_NOT_EXIST);

    return NULL;
}


static DWORD WINAPI
ScServiceMainStubA(LPVOID Context)
{
    PSERVICE_THREAD_PARAMSA ThreadParams = Context;

    TRACE("ScServiceMainStubA() called\n");

    /* Call the main service routine and free the arguments vector */
    (ThreadParams->lpServiceMain)(ThreadParams->dwArgCount,
                                  ThreadParams->lpArgVector);

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
    PSERVICE_THREAD_PARAMSW ThreadParams = Context;

    TRACE("ScServiceMainStubW() called\n");

    /* Call the main service routine and free the arguments vector */
    (ThreadParams->lpServiceMain)(ThreadParams->dwArgCount,
                                  ThreadParams->lpArgVector);

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
    DWORD dwServiceCurrent = 0;
    NTSTATUS Status;
    WCHAR NtControlPipeName[MAX_PATH + 1];
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
    DWORD dwProcessId;

    /* Get the service number and create the named pipe */
    RtlZeroMemory(&QueryTable,
                  sizeof(QueryTable));

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

    swprintf(NtControlPipeName, L"\\\\.\\pipe\\net\\NtControlPipe%u", dwServiceCurrent);

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
              sizeof(DWORD),
              &dwBytesWritten,
              NULL);

    TRACE("Sent Process ID %lu\n", dwProcessId);

    return ERROR_SUCCESS;
}


static DWORD
ScBuildUnicodeArgsVector(PSCM_CONTROL_PACKET ControlPacket,
                         LPDWORD lpArgCount,
                         LPWSTR **lpArgVector)
{
    LPWSTR *lpVector;
    LPWSTR *lpArg;
    LPWSTR pszServiceName;
    DWORD cbServiceName;
    DWORD cbTotal;
    DWORD i;

    if (ControlPacket == NULL || lpArgCount == NULL || lpArgVector == NULL)
        return ERROR_INVALID_PARAMETER;

    *lpArgCount = 0;
    *lpArgVector = NULL;

    pszServiceName = (PWSTR)((PBYTE)ControlPacket + ControlPacket->dwServiceNameOffset);
    cbServiceName = lstrlenW(pszServiceName) * sizeof(WCHAR) + sizeof(UNICODE_NULL);

    cbTotal = cbServiceName + sizeof(LPWSTR);
    if (ControlPacket->dwArgumentsCount > 0)
        cbTotal += ControlPacket->dwSize - ControlPacket->dwArgumentsOffset;

    lpVector = HeapAlloc(GetProcessHeap(),
                         HEAP_ZERO_MEMORY,
                         cbTotal);
    if (lpVector == NULL)
        return ERROR_OUTOFMEMORY;

    lpArg = lpVector;
    *lpArg = (LPWSTR)(lpArg + 1);
    lpArg++;

    memcpy(lpArg, pszServiceName, cbServiceName);
    lpArg = (LPWSTR*)((ULONG_PTR)lpArg + cbServiceName);

    if (ControlPacket->dwArgumentsCount > 0)
    {
        memcpy(lpArg,
               ((PBYTE)ControlPacket + ControlPacket->dwArgumentsOffset),
               ControlPacket->dwSize - ControlPacket->dwArgumentsOffset);

        for (i = 0; i < ControlPacket->dwArgumentsCount; i++)
        {
            *lpArg = (LPWSTR)((ULONG_PTR)lpArg + (ULONG_PTR)*lpArg);
            lpArg++;
        }
    }

    *lpArgCount = ControlPacket->dwArgumentsCount + 1;
    *lpArgVector = lpVector;

    return ERROR_SUCCESS;
}


static DWORD
ScBuildAnsiArgsVector(PSCM_CONTROL_PACKET ControlPacket,
                      LPDWORD lpArgCount,
                      LPSTR **lpArgVector)
{
    LPSTR *lpVector;
    LPSTR *lpPtr;
    LPWSTR lpUnicodeString;
    LPWSTR pszServiceName;
    LPSTR lpAnsiString;
    DWORD cbServiceName;
    DWORD dwVectorSize;
    DWORD dwUnicodeSize;
    DWORD dwAnsiSize = 0;
    DWORD dwAnsiNameSize = 0;
    DWORD i;

    if (ControlPacket == NULL || lpArgCount == NULL || lpArgVector == NULL)
        return ERROR_INVALID_PARAMETER;

    *lpArgCount = 0;
    *lpArgVector = NULL;

    pszServiceName = (PWSTR)((PBYTE)ControlPacket + ControlPacket->dwServiceNameOffset);
    cbServiceName = lstrlenW(pszServiceName) * sizeof(WCHAR) + sizeof(UNICODE_NULL);

    dwAnsiNameSize = WideCharToMultiByte(CP_ACP,
                                         0,
                                         pszServiceName,
                                         cbServiceName,
                                         NULL,
                                         0,
                                         NULL,
                                         NULL);

    dwVectorSize = ControlPacket->dwArgumentsCount * sizeof(LPWSTR);
    if (ControlPacket->dwArgumentsCount > 0)
    {
        lpUnicodeString = (LPWSTR)((PBYTE)ControlPacket +
                                   ControlPacket->dwArgumentsOffset +
                                   dwVectorSize);
        dwUnicodeSize = (ControlPacket->dwSize -
                         ControlPacket->dwArgumentsOffset -
                         dwVectorSize) / sizeof(WCHAR);

        dwAnsiSize = WideCharToMultiByte(CP_ACP,
                                         0,
                                         lpUnicodeString,
                                         dwUnicodeSize,
                                         NULL,
                                         0,
                                         NULL,
                                         NULL);
    }

    dwVectorSize += sizeof(LPWSTR);

    lpVector = HeapAlloc(GetProcessHeap(),
                         HEAP_ZERO_MEMORY,
                         dwVectorSize + dwAnsiNameSize + dwAnsiSize);
    if (lpVector == NULL)
        return ERROR_OUTOFMEMORY;

    lpPtr = (LPSTR*)lpVector;
    lpAnsiString = (LPSTR)((ULONG_PTR)lpVector + dwVectorSize);

    WideCharToMultiByte(CP_ACP,
                        0,
                        pszServiceName,
                        cbServiceName,
                        lpAnsiString,
                        dwAnsiNameSize,
                        NULL,
                        NULL);

    if (ControlPacket->dwArgumentsCount > 0)
    {
        lpAnsiString = (LPSTR)((ULONG_PTR)lpAnsiString + dwAnsiNameSize);

        WideCharToMultiByte(CP_ACP,
                            0,
                            lpUnicodeString,
                            dwUnicodeSize,
                            lpAnsiString,
                            dwAnsiSize,
                            NULL,
                            NULL);
    }

    lpAnsiString = (LPSTR)((ULONG_PTR)lpVector + dwVectorSize);
    for (i = 0; i < ControlPacket->dwArgumentsCount + 1; i++)
    {
        *lpPtr = lpAnsiString;

        lpPtr++;
        lpAnsiString += (strlen(lpAnsiString) + 1);
    }

    *lpArgCount = ControlPacket->dwArgumentsCount + 1;
    *lpArgVector = lpVector;

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

    if (lpService == NULL || ControlPacket == NULL)
        return ERROR_INVALID_PARAMETER;

    TRACE("ScStartService() called\n");
    TRACE("Size: %lu\n", ControlPacket->dwSize);
    TRACE("Service: %S\n", (PWSTR)((PBYTE)ControlPacket + ControlPacket->dwServiceNameOffset));

    /* Set the service status handle */
    lpService->hServiceStatus = ControlPacket->hServiceStatus;

    /* Build the arguments vector */
    if (lpService->bUnicode == TRUE)
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
        ThreadHandle = CreateThread(NULL,
                                    0,
                                    ScServiceMainStubW,
                                    ThreadParamsW,
                                    CREATE_SUSPENDED,
                                    &ThreadId);
        if (ThreadHandle == NULL)
        {
            if (ThreadParamsW->lpArgVector != NULL)
            {
                HeapFree(GetProcessHeap(),
                         0,
                         ThreadParamsW->lpArgVector);
            }
            HeapFree(GetProcessHeap(), 0, ThreadParamsW);
        }
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
        ThreadHandle = CreateThread(NULL,
                                    0,
                                    ScServiceMainStubA,
                                    ThreadParamsA,
                                    CREATE_SUSPENDED,
                                    &ThreadId);
        if (ThreadHandle == NULL)
        {
            if (ThreadParamsA->lpArgVector != NULL)
            {
                HeapFree(GetProcessHeap(),
                         0,
                         ThreadParamsA->lpArgVector);
            }
            HeapFree(GetProcessHeap(), 0, ThreadParamsA);
        }
    }

    ResumeThread(ThreadHandle);
    CloseHandle(ThreadHandle);

    return ERROR_SUCCESS;
}


static DWORD
ScControlService(PACTIVE_SERVICE lpService,
                 PSCM_CONTROL_PACKET ControlPacket)
{
    if (lpService == NULL || ControlPacket == NULL)
        return ERROR_INVALID_PARAMETER;

    TRACE("ScControlService() called\n");
    TRACE("Size: %lu\n", ControlPacket->dwSize);
    TRACE("Service: %S\n", (PWSTR)((PBYTE)ControlPacket + ControlPacket->dwServiceNameOffset));

    if (lpService->HandlerFunction)
    {
        (lpService->HandlerFunction)(ControlPacket->dwControl);
    }
    else if (lpService->HandlerFunctionEx)
    {
        /* FIXME: send correct params */
        (lpService->HandlerFunctionEx)(ControlPacket->dwControl, 0, NULL, NULL);
    }

    TRACE("ScControlService() done\n");

    return ERROR_SUCCESS;
}


static BOOL
ScServiceDispatcher(HANDLE hPipe,
                    PSCM_CONTROL_PACKET ControlPacket,
                    DWORD dwBufferSize)
{
    DWORD Count;
    BOOL bResult;
    DWORD dwRunningServices = 0;
    LPWSTR lpServiceName;
    PACTIVE_SERVICE lpService;
    SCM_REPLY_PACKET ReplyPacket;
    DWORD dwError;

    TRACE("ScDispatcherLoop() called\n");

    if (ControlPacket == NULL || dwBufferSize < sizeof(SCM_CONTROL_PACKET))
        return FALSE;

    while (TRUE)
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
                    if (dwError == ERROR_SUCCESS)
                        dwRunningServices++;
                    break;

                case SERVICE_CONTROL_STOP:
                    TRACE("Stop command - received SERVICE_CONTROL_STOP\n");
                    dwError = ScControlService(lpService, ControlPacket);
                    if (dwError == ERROR_SUCCESS)
                        dwRunningServices--;
                    break;

                default:
                    TRACE("Command %lu received", ControlPacket->dwControl);
                    dwError = ScControlService(lpService, ControlPacket);
                    break;
            }
        }
        else
        {
            dwError = ERROR_SERVICE_DOES_NOT_EXIST;
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

        if (dwRunningServices == 0)
            break;
    }

    return TRUE;
}


/**********************************************************************
 *	RegisterServiceCtrlHandlerA
 *
 * @implemented
 */
SERVICE_STATUS_HANDLE WINAPI
RegisterServiceCtrlHandlerA(LPCSTR lpServiceName,
                            LPHANDLER_FUNCTION lpHandlerProc)
{
    ANSI_STRING ServiceNameA;
    UNICODE_STRING ServiceNameU;
    SERVICE_STATUS_HANDLE SHandle;

    RtlInitAnsiString(&ServiceNameA, (LPSTR)lpServiceName);
    if (!NT_SUCCESS(RtlAnsiStringToUnicodeString(&ServiceNameU, &ServiceNameA, TRUE)))
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return (SERVICE_STATUS_HANDLE)0;
    }

    SHandle = RegisterServiceCtrlHandlerW(ServiceNameU.Buffer,
                                          lpHandlerProc);

    RtlFreeUnicodeString(&ServiceNameU);

    return SHandle;
}


/**********************************************************************
 *	RegisterServiceCtrlHandlerW
 *
 * @implemented
 */
SERVICE_STATUS_HANDLE WINAPI
RegisterServiceCtrlHandlerW(LPCWSTR lpServiceName,
                            LPHANDLER_FUNCTION lpHandlerProc)
{
    PACTIVE_SERVICE Service;

    Service = ScLookupServiceByServiceName((LPWSTR)lpServiceName);
    if (Service == NULL)
    {
        return (SERVICE_STATUS_HANDLE)NULL;
    }

    Service->HandlerFunction = lpHandlerProc;
    Service->HandlerFunctionEx = NULL;

    TRACE("RegisterServiceCtrlHandler returning %lu\n", Service->hServiceStatus);

    return Service->hServiceStatus;
}


/**********************************************************************
 *	RegisterServiceCtrlHandlerExA
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
    SERVICE_STATUS_HANDLE SHandle;

    RtlInitAnsiString(&ServiceNameA, (LPSTR)lpServiceName);
    if (!NT_SUCCESS(RtlAnsiStringToUnicodeString(&ServiceNameU, &ServiceNameA, TRUE)))
    {
        SetLastError(ERROR_OUTOFMEMORY);
        return (SERVICE_STATUS_HANDLE)0;
    }

    SHandle = RegisterServiceCtrlHandlerExW(ServiceNameU.Buffer,
                                            lpHandlerProc,
                                            lpContext);

    RtlFreeUnicodeString(&ServiceNameU);

    return SHandle;
}


/**********************************************************************
 *	RegisterServiceCtrlHandlerExW
 *
 * @implemented
 */
SERVICE_STATUS_HANDLE WINAPI
RegisterServiceCtrlHandlerExW(LPCWSTR lpServiceName,
                              LPHANDLER_FUNCTION_EX lpHandlerProc,
                              LPVOID lpContext)
{
    PACTIVE_SERVICE Service;

    Service = ScLookupServiceByServiceName(lpServiceName);
    if (Service == NULL)
    {
        return (SERVICE_STATUS_HANDLE)NULL;
    }

    Service->HandlerFunction = NULL;
    Service->HandlerFunctionEx = lpHandlerProc;
    Service->HandlerContext = lpContext;

    TRACE("RegisterServiceCtrlHandlerEx returning %lu\n", Service->hServiceStatus);

    return Service->hServiceStatus;
}


/**********************************************************************
 *	I_ScSetServiceBitsA
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

    RpcTryExcept
    {
        /* Call to services.exe using RPC */
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
 *	I_ScSetServiceBitsW
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

    RpcTryExcept
    {
        /* Call to services.exe using RPC */
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
 *	SetServiceBits
 *
 * @implemented
 */
BOOL WINAPI
SetServiceBits(SERVICE_STATUS_HANDLE hServiceStatus,
               DWORD dwServiceBits,
               BOOL bSetBitsOn,
               BOOL bUpdateImmediately)
{
    return I_ScSetServiceBitsW(hServiceStatus,
                               dwServiceBits,
                               bSetBitsOn,
                               bUpdateImmediately,
                               NULL);
}


/**********************************************************************
 *	SetServiceStatus
 *
 * @implemented
 */
BOOL WINAPI
SetServiceStatus(SERVICE_STATUS_HANDLE hServiceStatus,
                 LPSERVICE_STATUS lpServiceStatus)
{
    DWORD dwError;

    TRACE("SetServiceStatus() called\n");
    TRACE("hServiceStatus %lu\n", hServiceStatus);

    RpcTryExcept
    {
        /* Call to services.exe using RPC */
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
        ERR("ScmrSetServiceStatus() failed (Error %lu)\n", dwError);
        SetLastError(dwError);
        return FALSE;
    }

    TRACE("SetServiceStatus() done (ret %lu)\n", dwError);

    return TRUE;
}


/**********************************************************************
 *	StartServiceCtrlDispatcherA
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

    TRACE("StartServiceCtrlDispatcherA() called\n");

    i = 0;
    while (lpServiceStartTable[i].lpServiceProc != NULL)
    {
        i++;
    }

    dwActiveServiceCount = i;
    lpActiveServices = RtlAllocateHeap(RtlGetProcessHeap(),
                                       HEAP_ZERO_MEMORY,
                                       dwActiveServiceCount * sizeof(ACTIVE_SERVICE));
    if (lpActiveServices == NULL)
    {
        return FALSE;
    }

    /* Copy service names and start procedure */
    for (i = 0; i < dwActiveServiceCount; i++)
    {
        RtlCreateUnicodeStringFromAsciiz(&lpActiveServices[i].ServiceName,
                                         lpServiceStartTable[i].lpServiceName);
        lpActiveServices[i].ServiceMain.A = lpServiceStartTable[i].lpServiceProc;
        lpActiveServices[i].hServiceStatus = 0;
        lpActiveServices[i].bUnicode = FALSE;
        lpActiveServices[i].bOwnProcess = FALSE;
    }

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
        bRet = FALSE;
        goto done;
    }

    ScCreateStatusBinding();

    ScServiceDispatcher(hPipe, ControlPacket, dwBufSize);

    ScDestroyStatusBinding();

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

    return bRet;
}


/**********************************************************************
 *	StartServiceCtrlDispatcherW
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

    TRACE("StartServiceCtrlDispatcherW() called\n");

    i = 0;
    while (lpServiceStartTable[i].lpServiceProc != NULL)
    {
        i++;
    }

    dwActiveServiceCount = i;
    lpActiveServices = RtlAllocateHeap(RtlGetProcessHeap(),
                                       HEAP_ZERO_MEMORY,
                                       dwActiveServiceCount * sizeof(ACTIVE_SERVICE));
    if (lpActiveServices == NULL)
    {
        return FALSE;
    }

    /* Copy service names and start procedure */
    for (i = 0; i < dwActiveServiceCount; i++)
    {
        RtlCreateUnicodeString(&lpActiveServices[i].ServiceName,
                               lpServiceStartTable[i].lpServiceName);
        lpActiveServices[i].ServiceMain.W = lpServiceStartTable[i].lpServiceProc;
        lpActiveServices[i].hServiceStatus = 0;
        lpActiveServices[i].bUnicode = TRUE;
        lpActiveServices[i].bOwnProcess = FALSE;
    }

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
        bRet = FALSE;
        goto done;
    }

    ScCreateStatusBinding();

    ScServiceDispatcher(hPipe, ControlPacket, dwBufSize);

    ScDestroyStatusBinding();

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

    return bRet;
}

/* EOF */
