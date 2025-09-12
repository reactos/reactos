/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS DNS Resolver
 * FILE:        base/services/dnsrslvr/dnsrslvr.c
 * PURPOSE:     DNS Resolver Service
 * PROGRAMMER:  Christoph von Wittich
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

HINSTANCE hDllInstance;
SERVICE_STATUS_HANDLE ServiceStatusHandle;
SERVICE_STATUS SvcStatus;
static WCHAR ServiceName[] = L"Dnscache";

DWORD WINAPI RpcThreadRoutine(LPVOID lpParameter);

/* FUNCTIONS *****************************************************************/

static
VOID
UpdateServiceStatus(
    HANDLE hServiceStatus,
    DWORD NewStatus,
    DWORD Increment)
{
    if (Increment > 0)
        SvcStatus.dwCheckPoint += Increment;
    else
        SvcStatus.dwCheckPoint = 0;

    SvcStatus.dwCurrentState = NewStatus;
    SetServiceStatus(hServiceStatus, &SvcStatus);
}

static
DWORD
WINAPI
ServiceControlHandler(
    DWORD dwControl,
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
            DnsIntCacheFree();
            UpdateServiceStatus(ServiceStatusHandle, SERVICE_STOPPED, 0);
            break;

        case SERVICE_CONTROL_INTERROGATE:
            return NO_ERROR;

        default:
            return ERROR_CALL_NOT_IMPLEMENTED;
    }
    return NO_ERROR;
}

VOID
WINAPI
ServiceMain(
    DWORD argc,
    LPWSTR *argv)
{
    HANDLE hThread;

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    DPRINT("ServiceMain() called\n");

    SvcStatus.dwServiceType             = SERVICE_WIN32_OWN_PROCESS;
    SvcStatus.dwCurrentState            = SERVICE_START_PENDING;
    SvcStatus.dwControlsAccepted        = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    SvcStatus.dwCheckPoint              = 0;
    SvcStatus.dwWin32ExitCode           = NO_ERROR;
    SvcStatus.dwServiceSpecificExitCode = 0;
    SvcStatus.dwWaitHint                = 4000;

    ServiceStatusHandle = RegisterServiceCtrlHandlerExW(ServiceName,
                                                        ServiceControlHandler,
                                                        NULL);

    if (!ServiceStatusHandle)
    {
        DPRINT1("DNSRSLVR: Unable to register service control handler (%lx)\n", GetLastError());
        return; // FALSE
    }

    DnsIntCacheInitialize();

    ReadHostsFile();

    hThread = CreateThread(NULL,
                           0,
                           (LPTHREAD_START_ROUTINE)
                           RpcThreadRoutine,
                           NULL,
                           0,
                           NULL);

    if (!hThread)
    {
        DnsIntCacheFree();
        DPRINT("Can't create RpcThread\n");
        UpdateServiceStatus(ServiceStatusHandle, SERVICE_STOPPED, 0);
    }
    else
    {
        CloseHandle(hThread);
    }

    DPRINT("ServiceMain() done\n");
    UpdateServiceStatus(ServiceStatusHandle, SERVICE_RUNNING, 0);
}

BOOL
WINAPI
DllMain(
    _In_ HINSTANCE hinstDLL,
    _In_ DWORD fdwReason,
    _In_ PVOID pvReserved)
{
    UNREFERENCED_PARAMETER(pvReserved);

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            hDllInstance = hinstDLL;
            break;

        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}

/* EOF */
