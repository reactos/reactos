/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/advapi32/service/sctrl.c
 * PURPOSE:         Service control manager functions
 * PROGRAMMER:      Emanuele Aliberti
 * UPDATE HISTORY:
 *	19990413 EA	created
 *	19990515 EA
 */

/* INCLUDES ******************************************************************/

#include <advapi32.h>

#define NDEBUG
#include <debug.h>


/* TYPES *********************************************************************/

typedef struct _ACTIVE_SERVICE
{
    DWORD ThreadId;
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


static PACTIVE_SERVICE
ScLookupServiceByThreadId(DWORD ThreadId)
{
    DWORD i;

    for (i = 0; i < dwActiveServiceCount; i++)
    {
        if (lpActiveServices[i].ThreadId == ThreadId)
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

    DPRINT("ScServiceMainStub() called\n");

    /* Count arguments */
    lpPtr = lpService->Arguments;
    while (*lpPtr)
    {
        DPRINT("arg: %S\n", lpPtr);
        dwLen = wcslen(lpPtr) + 1;
        dwArgCount++;
        dwLength += dwLen;
        lpPtr += dwLen;
    }
    DPRINT("dwArgCount: %ld\ndwLength: %ld\n", dwArgCount, dwLength);

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
            return ERROR_OUTOFMEMORY;

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
    DWORD dwProcessId;
    DWORD dwState;

    if (!WaitNamedPipeW(L"\\\\.\\pipe\\net\\NtControlPipe", 15000))
    {
        DPRINT1("WaitNamedPipe() failed (Error %lu)\n", GetLastError());
        return ERROR_FAILED_SERVICE_CONTROLLER_CONNECT;
    }

    *hPipe = CreateFileW(L"\\\\.\\pipe\\net\\NtControlPipe",
                         GENERIC_READ | GENERIC_WRITE,
                         0,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL);
    if (*hPipe == INVALID_HANDLE_VALUE)
    {
        DPRINT1("CreateFileW() failed (Error %lu)\n", GetLastError());
        return ERROR_FAILED_SERVICE_CONTROLLER_CONNECT;
    }

    dwState = PIPE_READMODE_MESSAGE;
    if (!SetNamedPipeHandleState(*hPipe, &dwState, NULL, NULL))
    {
        CloseHandle(hPipe);
        *hPipe = INVALID_HANDLE_VALUE;
        return ERROR_FAILED_SERVICE_CONTROLLER_CONNECT;
    }

    dwProcessId = GetCurrentProcessId();
    WriteFile(*hPipe,
              &dwProcessId,
              sizeof(DWORD),
              &dwBytesWritten,
              NULL);

    DPRINT("Sent process id %lu\n", dwProcessId);

    return ERROR_SUCCESS;
}



static DWORD
ScStartService(PSCM_CONTROL_PACKET ControlPacket)
{
    PACTIVE_SERVICE lpService;
    HANDLE ThreadHandle;

    DPRINT("ScStartService() called\n");
    DPRINT("Size: %lu\n", ControlPacket->dwSize);
    DPRINT("Service: %S\n", &ControlPacket->szArguments[0]);

    lpService = ScLookupServiceByServiceName(&ControlPacket->szArguments[0]);
    if (lpService == NULL)
        return ERROR_SERVICE_DOES_NOT_EXIST;

    lpService->Arguments = HeapAlloc(GetProcessHeap(),
                                     HEAP_ZERO_MEMORY,
                                     ControlPacket->dwSize * sizeof(WCHAR));
    if (lpService->Arguments == NULL)
        return ERROR_OUTOFMEMORY;

    memcpy(lpService->Arguments,
           ControlPacket->szArguments,
           ControlPacket->dwSize * sizeof(WCHAR));

    ThreadHandle = CreateThread(NULL,
                                0,
                                ScServiceMainStub,
                                lpService,
                                CREATE_SUSPENDED,
                                &lpService->ThreadId);
    if (ThreadHandle == NULL)
        return ERROR_SERVICE_NO_THREAD;

    ResumeThread(ThreadHandle);
    CloseHandle(ThreadHandle);

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

    DPRINT("ScDispatcherLoop() called\n");

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
            DPRINT1("Pipe read failed (Error: %lu)\n", GetLastError());
            return FALSE;
        }

        /* Execute command */
        switch (ControlPacket->dwControl)
        {
            case SERVICE_CONTROL_START:
                DPRINT("Start command\n");
                if (ScStartService(ControlPacket) == ERROR_SUCCESS)
                    dwRunningServices++;
                break;

            case SERVICE_CONTROL_STOP:
                DPRINT("Stop command\n");
                dwRunningServices--;
                break;

            default:
                DPRINT1("Unknown command %lu", ControlPacket->dwControl);
                break;
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
SERVICE_STATUS_HANDLE STDCALL
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
SERVICE_STATUS_HANDLE STDCALL
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

    return (SERVICE_STATUS_HANDLE)Service->ThreadId;
}


/**********************************************************************
 *	RegisterServiceCtrlHandlerExA
 *
 * @implemented
 */
SERVICE_STATUS_HANDLE STDCALL
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
SERVICE_STATUS_HANDLE STDCALL
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

    return (SERVICE_STATUS_HANDLE)Service->ThreadId;
}


/**********************************************************************
 *	SetServiceBits
 *
 * @unimplemented
 */
BOOL STDCALL
SetServiceBits(SERVICE_STATUS_HANDLE hServiceStatus,
               DWORD dwServiceBits,
               BOOL bSetBitsOn,
               BOOL bUpdateImmediately)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/**********************************************************************
 *	SetServiceStatus
 *
 * @implemented
 */
BOOL STDCALL
SetServiceStatus(SERVICE_STATUS_HANDLE hServiceStatus,
                 LPSERVICE_STATUS lpServiceStatus)
{
    PACTIVE_SERVICE Service;

    Service = ScLookupServiceByThreadId((DWORD)hServiceStatus);
    if (!Service)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    RtlCopyMemory(&Service->ServiceStatus,
                  lpServiceStatus,
                  sizeof(SERVICE_STATUS));

    return TRUE;
}


/**********************************************************************
 *	StartServiceCtrlDispatcherA
 *
 * @unimplemented
 */
BOOL STDCALL
StartServiceCtrlDispatcherA(LPSERVICE_TABLE_ENTRYA lpServiceStartTable)
{
    ULONG i;
    HANDLE hPipe;
    DWORD dwError;
    PUCHAR lpMessageBuffer;

    DPRINT("StartServiceCtrlDispatcherA() called\n");

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
        lpActiveServices[i].bUnicode = FALSE;
    }

    dwError = ScConnectControlPipe(&hPipe);
    if (dwError != ERROR_SUCCESS)
    {
        /* Free the service table */
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
    RtlFreeHeap(RtlGetProcessHeap(), 0, lpActiveServices);
    lpActiveServices = NULL;
    dwActiveServiceCount = 0;

    return TRUE;
}


/**********************************************************************
 *	StartServiceCtrlDispatcherW
 *
 * @unimplemented
 */
BOOL STDCALL
StartServiceCtrlDispatcherW(LPSERVICE_TABLE_ENTRYW lpServiceStartTable)
{
    ULONG i;
    HANDLE hPipe;
    DWORD dwError;
    PUCHAR lpMessageBuffer;

    DPRINT("StartServiceCtrlDispatcherW() called\n");

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
        lpActiveServices[i].bUnicode = TRUE;
    }

    dwError = ScConnectControlPipe(&hPipe);
    if (dwError != ERROR_SUCCESS)
    {
        /* Free the service table */
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
    RtlFreeHeap(RtlGetProcessHeap(), 0, lpActiveServices);
    lpActiveServices = NULL;
    dwActiveServiceCount = 0;

    return TRUE;
}

/* EOF */
