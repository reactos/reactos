/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/wlansvc/wlansvc.c
 * PURPOSE:          WLAN Service
 * PROGRAMMER:       Christoph von Wittich
 */

/* INCLUDES *****************************************************************/

#define WIN32_NO_STATUS
#include <windows.h>
#include "wlansvc_s.h"

//#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

#define SERVICE_NAME L"WLAN Service"

SERVICE_STATUS_HANDLE ServiceStatusHandle;
SERVICE_STATUS SvcStatus;

/* FUNCTIONS *****************************************************************/
static DWORD WINAPI RpcThreadRoutine(LPVOID lpParameter)
{
    RPC_STATUS Status;

    Status = RpcServerUseProtseqEpW(L"ncalrpc", 20, L"wlansvc", NULL);
    if (Status != RPC_S_OK)
    {
        DPRINT("RpcServerUseProtseqEpW() failed (Status %lx)\n", Status);
        return 0;
    }

    Status = RpcServerRegisterIf(wlansvc_interface_v1_0_s_ifspec, NULL, NULL);
    if (Status != RPC_S_OK)
    {
        DPRINT("RpcServerRegisterIf() failed (Status %lx)\n", Status);
        return 0;
    }

    Status = RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, 0);
    if (Status != RPC_S_OK)
    {
        DPRINT("RpcServerListen() failed (Status %lx)\n", Status);
    }

    DPRINT("RpcServerListen finished\n");
    return 0;
}

static void UpdateServiceStatus(HANDLE hServiceStatus, DWORD NewStatus, DWORD Increment)
{
    if (Increment > 0)
        SvcStatus.dwCheckPoint += Increment;
    else
        SvcStatus.dwCheckPoint = 0;

    SvcStatus.dwCurrentState = NewStatus;
    SetServiceStatus(hServiceStatus, &SvcStatus);
}

static DWORD WINAPI
ServiceControlHandler(DWORD dwControl,
                      DWORD dwEventType,
                      LPVOID lpEventData,
                      LPVOID lpContext)
{
    switch (dwControl)
    {
        case SERVICE_CONTROL_SHUTDOWN:
        case SERVICE_CONTROL_STOP:
            UpdateServiceStatus(ServiceStatusHandle, SERVICE_STOP_PENDING, 1);
            RpcMgmtStopServerListening(NULL);
            break;
        case SERVICE_CONTROL_INTERROGATE:
            return NO_ERROR;
        default:
            return ERROR_CALL_NOT_IMPLEMENTED;
    }
    return NO_ERROR;
}

static VOID CALLBACK
ServiceMain(DWORD argc, LPWSTR *argv)
{
    HANDLE hThread;

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    DPRINT("ServiceMain() called\n");

    SvcStatus.dwServiceType             = SERVICE_WIN32_OWN_PROCESS;
    SvcStatus.dwControlsAccepted        = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    SvcStatus.dwCheckPoint              = 0;
    SvcStatus.dwWin32ExitCode           = 0;
    SvcStatus.dwServiceSpecificExitCode = 0;
    SvcStatus.dwWaitHint                = 4000;

    ServiceStatusHandle = RegisterServiceCtrlHandlerExW(SERVICE_NAME,
                                                        ServiceControlHandler,
                                                        NULL);

    UpdateServiceStatus(ServiceStatusHandle, SERVICE_RUNNING, 0);

    hThread = CreateThread(NULL,
                           0,
                           (LPTHREAD_START_ROUTINE)
                           RpcThreadRoutine,
                           NULL,
                           0,
                           NULL);

    if (!hThread)
        DPRINT("Can't create RpcThread\n");
    else
    {
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
    }

    UpdateServiceStatus(ServiceStatusHandle, SERVICE_STOPPED, 0);

    DPRINT("ServiceMain() done\n");
}

int
wmain(int argc, WCHAR *argv[])
{
    SERVICE_TABLE_ENTRYW ServiceTable[2] =
    {
        {SERVICE_NAME, ServiceMain},
        {NULL, NULL}
    };

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    DPRINT("wlansvc: main() started\n");

    StartServiceCtrlDispatcherW(ServiceTable);

    DPRINT("wlansvc: main() done\n");

    ExitThread(0);

    return 0;
}

/* EOF */
