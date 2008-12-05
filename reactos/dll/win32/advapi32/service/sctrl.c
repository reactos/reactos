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
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(advapi);


/* TYPES *********************************************************************/

typedef struct _ACTIVE_SERVICE
{
    CLIENT_HANDLE hService;
    UNICODE_STRING ServiceName;
    union
    {
        LPSERVICE_MAIN_FUNCTIONA lpFuncA;
        LPSERVICE_MAIN_FUNCTIONW lpFuncW;
    } Main;
    LPHANDLER_FUNCTION HandlerFunction;
    LPHANDLER_FUNCTION_EX HandlerFunctionEx;
    LPVOID HandlerContext;
    SERVICE_STATUS ServiceStatus;
    BOOL bUnicode;
    LPWSTR Arguments;
} ACTIVE_SERVICE, *PACTIVE_SERVICE;


/* GLOBALS *******************************************************************/

static DWORD dwActiveServiceCount = 0;
static PACTIVE_SERVICE lpActiveServices = NULL;


/* FUNCTIONS *****************************************************************/

static PACTIVE_SERVICE
ScLookupServiceByServiceName(LPCWSTR lpServiceName)
{
    DWORD i;

    for (i = 0; i < dwActiveServiceCount; i++)
    {
        if (_wcsicmp(lpActiveServices[i].ServiceName.Buffer, lpServiceName) == 0)
        {
            return &lpActiveServices[i];
        }
    }

    SetLastError(ERROR_SERVICE_DOES_NOT_EXIST);

    return NULL;
}


static DWORD WINAPI
ScServiceMainStub(LPVOID Context)
{
    PACTIVE_SERVICE lpService;
    DWORD dwArgCount = 0;
    DWORD dwLength = 0;
    DWORD dwLen;
    LPWSTR lpPtr;

    lpService = (PACTIVE_SERVICE)Context;

    TRACE("ScServiceMainStub() called\n");

    /* Count arguments */
    lpPtr = lpService->Arguments;
    while (*lpPtr)
    {
        TRACE("arg: %S\n", lpPtr);
        dwLen = wcslen(lpPtr) + 1;
        dwArgCount++;
        dwLength += dwLen;
        lpPtr += dwLen;
    }
    TRACE("dwArgCount: %ld\ndwLength: %ld\n", dwArgCount, dwLength);

    /* Build the argument vector and call the main service routine */
    if (lpService->bUnicode)
    {
        LPWSTR *lpArgVector;
        LPWSTR Ptr;

        lpArgVector = HeapAlloc(GetProcessHeap(),
                                HEAP_ZERO_MEMORY,
                                (dwArgCount + 1) * sizeof(LPWSTR));
        if (lpArgVector == NULL)
            return ERROR_OUTOFMEMORY;

        dwArgCount = 0;
        Ptr = lpService->Arguments;
        while (*Ptr)
        {
            lpArgVector[dwArgCount] = Ptr;

            dwArgCount++;
            Ptr += (wcslen(Ptr) + 1);
        }
        lpArgVector[dwArgCount] = NULL;

        (lpService->Main.lpFuncW)(dwArgCount, lpArgVector);

        HeapFree(GetProcessHeap(),
                 0,
                 lpArgVector);
    }
    else
    {
        LPSTR *lpArgVector;
        LPSTR Ptr;
        LPSTR AnsiString;
        DWORD AnsiLength;

        AnsiLength = WideCharToMultiByte(CP_ACP,
                                         0,
                                         lpService->Arguments,
                                         dwLength,
                                         NULL,
                                         0,
                                         NULL,
                                         NULL);
        if (AnsiLength == 0)
            return ERROR_INVALID_PARAMETER; /* ? */

        AnsiString = HeapAlloc(GetProcessHeap(),
                               0,
                               AnsiLength + 1);
        if (AnsiString == NULL)
            return ERROR_OUTOFMEMORY;

        WideCharToMultiByte(CP_ACP,
                            0,
                            lpService->Arguments,
                            dwLength,
                            AnsiString,
                            AnsiLength,
                            NULL,
                            NULL);

        AnsiString[AnsiLength] = ANSI_NULL;

        lpArgVector = HeapAlloc(GetProcessHeap(),
                                0,
                                (dwArgCount + 1) * sizeof(LPSTR));
        if (lpArgVector == NULL)
        {
            HeapFree(GetProcessHeap(),
                        0,
                        AnsiString);
            return ERROR_OUTOFMEMORY;
        }

        dwArgCount = 0;
        Ptr = AnsiString;
        while (*Ptr)
        {
            lpArgVector[dwArgCount] = Ptr;

            dwArgCount++;
            Ptr += (strlen(Ptr) + 1);
        }
        lpArgVector[dwArgCount] = NULL;

        (lpService->Main.lpFuncA)(dwArgCount, lpArgVector);

        HeapFree(GetProcessHeap(),
                 0,
                 lpArgVector);
        HeapFree(GetProcessHeap(),
                 0,
                 AnsiString);
    }

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

    if (!WaitNamedPipeW(NtControlPipeName, 15000))
    {
        ERR("WaitNamedPipe(%S) failed (Error %lu)\n", NtControlPipeName, GetLastError());
        return ERROR_FAILED_SERVICE_CONTROLLER_CONNECT;
    }

    *hPipe = CreateFileW(NtControlPipeName,
                         GENERIC_READ | GENERIC_WRITE,
                         0,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL);
    if (*hPipe == INVALID_HANDLE_VALUE)
    {
        ERR("CreateFileW() failed (Error %lu)\n", GetLastError());
        return ERROR_FAILED_SERVICE_CONTROLLER_CONNECT;
    }

    dwState = PIPE_READMODE_MESSAGE;
    if (!SetNamedPipeHandleState(*hPipe, &dwState, NULL, NULL))
    {
        CloseHandle(*hPipe);
        *hPipe = INVALID_HANDLE_VALUE;
        return ERROR_FAILED_SERVICE_CONTROLLER_CONNECT;
    }

    /* Share the SERVICE_HANDLE handle with the SCM */
    WriteFile(*hPipe,
              (DWORD *)&lpActiveServices->hService,
              sizeof(CLIENT_HANDLE),
              &dwBytesWritten,
              NULL);

    TRACE("Sent SERVICE_HANDLE %lu\n", lpActiveServices->hService);

    return ERROR_SUCCESS;
}


static DWORD
ScStartService(PSCM_CONTROL_PACKET ControlPacket)
{
    PACTIVE_SERVICE lpService;
    HANDLE ThreadHandle;
    DWORD ThreadId;

    TRACE("ScStartService() called\n");
    TRACE("client handle: %lu\n", ControlPacket->hClient);
    TRACE("Size: %lu\n", ControlPacket->dwSize);
    TRACE("Service: %S\n", &ControlPacket->szArguments[0]);

    lpService = (PACTIVE_SERVICE)(ULONG_PTR)ControlPacket->hClient;
    if (lpService == NULL)
    {
        TRACE("Service not found\n");
        return ERROR_SERVICE_DOES_NOT_EXIST;
    }

    lpService->Arguments = HeapAlloc(GetProcessHeap(),
                                     HEAP_ZERO_MEMORY,
                                     (ControlPacket->dwSize + 1) * sizeof(WCHAR));
    if (lpService->Arguments == NULL)
        return ERROR_OUTOFMEMORY;

    memcpy(lpService->Arguments,
           ControlPacket->szArguments,
           ControlPacket->dwSize * sizeof(WCHAR));

    /* invoke the services entry point and implement the command loop */
    ThreadHandle = CreateThread(NULL,
                                0,
                                ScServiceMainStub,
                                lpService,
                                CREATE_SUSPENDED,
                                &ThreadId);
    if (ThreadHandle == NULL)
        return ERROR_SERVICE_NO_THREAD;

    ResumeThread(ThreadHandle);
    CloseHandle(ThreadHandle);

    return ERROR_SUCCESS;
}


static DWORD
ScControlService(PSCM_CONTROL_PACKET ControlPacket)
{
    PACTIVE_SERVICE lpService;

    TRACE("ScControlService() called\n");
    TRACE("Size: %lu\n", ControlPacket->dwSize);
    TRACE("Service: %S\n", &ControlPacket->szArguments[0]);

    lpService = (PACTIVE_SERVICE)(ULONG_PTR)ControlPacket->hClient;
    if (lpService == NULL)
    {
        TRACE("Service not found\n");
        return ERROR_SERVICE_DOES_NOT_EXIST;
    }

    if (lpService->HandlerFunction)
    {
        (lpService->HandlerFunction)(ControlPacket->dwControl);
    }
    else if (lpService->HandlerFunctionEx)
    {
        /* FIXME: send correct params */
        (lpService->HandlerFunctionEx)(ControlPacket->dwControl, 0, NULL, NULL);
    }

    if (ControlPacket->dwControl == SERVICE_CONTROL_STOP)
    {
        HeapFree(GetProcessHeap(),
                 0,
                 lpService->Arguments);
    }

    TRACE("ScControlService() done\n");

    return ERROR_SUCCESS;
}


static BOOL
ScServiceDispatcher(HANDLE hPipe,
                    PUCHAR lpBuffer,
                    DWORD dwBufferSize)
{
    PSCM_CONTROL_PACKET ControlPacket;
    DWORD Count;
    BOOL bResult;
    DWORD dwRunningServices = 0;

    TRACE("ScDispatcherLoop() called\n");

    ControlPacket = HeapAlloc(GetProcessHeap(),
                              HEAP_ZERO_MEMORY,
                              1024);
    if (ControlPacket == NULL)
        return FALSE;

    while (TRUE)
    {
        /* Read command from the control pipe */
        bResult = ReadFile(hPipe,
                           ControlPacket,
                           1024,
                           &Count,
                           NULL);
        if (bResult == FALSE)
        {
            ERR("Pipe read failed (Error: %lu)\n", GetLastError());
            return FALSE;
        }

        /* Execute command */
        switch (ControlPacket->dwControl)
        {
            case SERVICE_CONTROL_START:
                TRACE("Start command - recieved SERVICE_CONTROL_START\n");
                if (ScStartService(ControlPacket) == ERROR_SUCCESS)
                    dwRunningServices++;
                break;

            case SERVICE_CONTROL_STOP:
                TRACE("Stop command - recieved SERVICE_CONTROL_STOP\n");
                if (ScControlService(ControlPacket) == ERROR_SUCCESS)
                    dwRunningServices--;
                break;

            default:
                TRACE("Unknown command %lu", ControlPacket->dwControl);
                continue;
        }

        if (dwRunningServices == 0)
            break;
    }

    HeapFree(GetProcessHeap(),
             0,
             ControlPacket);

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
        return INVALID_HANDLE_VALUE;
    }

    Service->HandlerFunction = lpHandlerProc;
    Service->HandlerFunctionEx = NULL;

    TRACE("RegisterServiceCtrlHandler returning %lu\n", Service->hService);

    return (SERVICE_STATUS_HANDLE)Service->hService;
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

    TRACE("RegisterServiceCtrlHandlerEx returning %lu", Service->hService);

    return (SERVICE_STATUS_HANDLE)Service->hService;
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

    _SEH2_TRY
    {
        /* Call to services.exe using RPC */
        bResult = RI_ScSetServiceBitsA((RPC_SERVICE_STATUS_HANDLE)hServiceStatus,
                                       dwServiceBits,
                                       bSetBitsOn,
                                       bUpdateImmediately,
                                       lpString);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError(ScmRpcStatusToWinError(RpcExceptionCode()));
        bResult = FALSE;
    }
    _SEH2_END;

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

    _SEH2_TRY
    {
        /* Call to services.exe using RPC */
        bResult = RI_ScSetServiceBitsW((RPC_SERVICE_STATUS_HANDLE)hServiceStatus,
                                       dwServiceBits,
                                       bSetBitsOn,
                                       bUpdateImmediately,
                                       lpString);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError(ScmRpcStatusToWinError(RpcExceptionCode()));
        bResult = FALSE;
    }
    _SEH2_END;

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

    /* Call to services.exe using RPC */
    dwError = RSetServiceStatus((RPC_SERVICE_STATUS_HANDLE)hServiceStatus,
                                lpServiceStatus);
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
StartServiceCtrlDispatcherA(LPSERVICE_TABLE_ENTRYA lpServiceStartTable)
{
    ULONG i;
    HANDLE hPipe;
    DWORD dwError;
    PUCHAR lpMessageBuffer;

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
        lpActiveServices[i].Main.lpFuncA = lpServiceStartTable[i].lpServiceProc;
        lpActiveServices[i].hService = (CLIENT_HANDLE)&lpActiveServices[i];
        lpActiveServices[i].bUnicode = FALSE;
    }

    dwError = ScConnectControlPipe(&hPipe);
    if (dwError != ERROR_SUCCESS)
    {
        /* Free the service table */
        for (i = 0; i < dwActiveServiceCount; i++)
        {
            RtlFreeUnicodeString(&lpActiveServices[i].ServiceName);
        }
        RtlFreeHeap(RtlGetProcessHeap(), 0, lpActiveServices);
        lpActiveServices = NULL;
        dwActiveServiceCount = 0;
        return FALSE;
    }

    lpMessageBuffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                      HEAP_ZERO_MEMORY,
                                      256);
    if (lpMessageBuffer == NULL)
    {
        /* Free the service table */
        for (i = 0; i < dwActiveServiceCount; i++)
        {
            RtlFreeUnicodeString(&lpActiveServices[i].ServiceName);
        }
        RtlFreeHeap(RtlGetProcessHeap(), 0, lpActiveServices);
        lpActiveServices = NULL;
        dwActiveServiceCount = 0;
        CloseHandle(hPipe);
        return FALSE;
    }

    ScServiceDispatcher(hPipe, lpMessageBuffer, 256);
    CloseHandle(hPipe);

    /* Free the message buffer */
    RtlFreeHeap(RtlGetProcessHeap(), 0, lpMessageBuffer);

    /* Free the service table */
    for (i = 0; i < dwActiveServiceCount; i++)
    {
        RtlFreeUnicodeString(&lpActiveServices[i].ServiceName);
    }
    RtlFreeHeap(RtlGetProcessHeap(), 0, lpActiveServices);
    lpActiveServices = NULL;
    dwActiveServiceCount = 0;

    return TRUE;
}


/**********************************************************************
 *	StartServiceCtrlDispatcherW
 *
 * @implemented
 */
BOOL WINAPI
StartServiceCtrlDispatcherW(LPSERVICE_TABLE_ENTRYW lpServiceStartTable)
{
    ULONG i;
    HANDLE hPipe;
    DWORD dwError;
    PUCHAR lpMessageBuffer;

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
        lpActiveServices[i].Main.lpFuncW = lpServiceStartTable[i].lpServiceProc;
        lpActiveServices[i].hService = (CLIENT_HANDLE)&lpActiveServices[i];
        lpActiveServices[i].bUnicode = TRUE;
    }

    dwError = ScConnectControlPipe(&hPipe);
    if (dwError != ERROR_SUCCESS)
    {
        /* Free the service table */
        for (i = 0; i < dwActiveServiceCount; i++)
        {
            RtlFreeUnicodeString(&lpActiveServices[i].ServiceName);
        }
        RtlFreeHeap(RtlGetProcessHeap(), 0, lpActiveServices);
        lpActiveServices = NULL;
        dwActiveServiceCount = 0;
        return FALSE;
    }

    lpMessageBuffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                      HEAP_ZERO_MEMORY,
                                      256);
    if (lpMessageBuffer == NULL)
    {
        /* Free the service table */
        for (i = 0; i < dwActiveServiceCount; i++)
        {
            RtlFreeUnicodeString(&lpActiveServices[i].ServiceName);
        }
        RtlFreeHeap(RtlGetProcessHeap(), 0, lpActiveServices);
        lpActiveServices = NULL;
        dwActiveServiceCount = 0;
        CloseHandle(hPipe);
        return FALSE;
    }

    ScServiceDispatcher(hPipe, lpMessageBuffer, 256);
    CloseHandle(hPipe);

    /* Free the message buffer */
    RtlFreeHeap(RtlGetProcessHeap(), 0, lpMessageBuffer);

    /* Free the service table */
    for (i = 0; i < dwActiveServiceCount; i++)
    {
        RtlFreeUnicodeString(&lpActiveServices[i].ServiceName);
    }
    RtlFreeHeap(RtlGetProcessHeap(), 0, lpActiveServices);
    lpActiveServices = NULL;
    dwActiveServiceCount = 0;

    return TRUE;
}

/* EOF */
