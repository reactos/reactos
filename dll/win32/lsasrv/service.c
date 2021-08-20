/*
 * PROJECT:     Local Security Authority Server DLL
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/lsasrv/service.c
 * PURPOSE:     Security service
 * COPYRIGHT:   Copyright 2016, 2019 Eric Kohl <eric.kohl@reactos.org>
 */

/* INCLUDES ****************************************************************/

#include "lsasrv.h"
#include <winsvc.h>

typedef VOID (WINAPI *PNETLOGONMAIN)(INT ArgCount, PWSTR *ArgVector);

VOID WINAPI I_ScIsSecurityProcess(VOID);

static VOID WINAPI NetlogonServiceMain(DWORD dwArgc, PWSTR *pszArgv);
static VOID WINAPI SamSsServiceMain(DWORD dwArgc, PWSTR *pszArgv);

SERVICE_TABLE_ENTRYW ServiceTable[] =
{
    {L"NETLOGON", NetlogonServiceMain},
    {L"SAMSS", SamSsServiceMain},
    {NULL, NULL}
};


/* FUNCTIONS ***************************************************************/

static
VOID
WINAPI
NetlogonServiceMain(
    _In_ DWORD dwArgc,
    _In_ PWSTR *pszArgv)
{
    HINSTANCE hNetlogon = NULL;
    PNETLOGONMAIN pNetlogonMain = NULL;

    TRACE("NetlogonServiceMain(%lu %p)\n", dwArgc, pszArgv);

    hNetlogon = LoadLibraryW(L"Netlogon.dll");
    if (hNetlogon == NULL)
    {
        ERR("LoadLibrary() failed!\n");
        return;
    }

    pNetlogonMain = (PNETLOGONMAIN)GetProcAddress(hNetlogon, "NlNetlogonMain");
    if (pNetlogonMain == NULL)
    {
        ERR("GetProcAddress(NlNetlogonMain) failed!\n");
        FreeLibrary(hNetlogon);
        return;
    }

    TRACE("NlNetlogonMain %p\n", pNetlogonMain);

    pNetlogonMain(dwArgc, pszArgv);
}


static
VOID
WINAPI
SamSsControlHandler(
    _In_ DWORD fdwControl)
{
    TRACE("SamSsControlHandler(%lu)\n", fdwControl);
}


static
VOID
WINAPI
SamSsServiceMain(
    _In_ DWORD dwArgc,
    _In_ PWSTR *pszArgv)
{
    SERVICE_STATUS_HANDLE hStatus;
    SERVICE_STATUS ServiceStatus;

    TRACE("SamSsServiceMain(%lu %p)\n", dwArgc, pszArgv);

    hStatus = RegisterServiceCtrlHandlerW(L"SAMSS",
                                          SamSsControlHandler);
    if (hStatus == NULL)
        return;

    ServiceStatus.dwServiceType = SERVICE_WIN32_SHARE_PROCESS;
    ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    ServiceStatus.dwControlsAccepted = 0;
    ServiceStatus.dwWin32ExitCode = ERROR_SUCCESS;
    ServiceStatus.dwServiceSpecificExitCode = ERROR_SUCCESS;
    ServiceStatus.dwCheckPoint = 1;
    ServiceStatus.dwWaitHint = 0x7530;

    SetServiceStatus(hStatus, &ServiceStatus);

    ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    ServiceStatus.dwCheckPoint = 0;
    ServiceStatus.dwWaitHint = 0;

    SetServiceStatus(hStatus, &ServiceStatus);
}


static
DWORD
WINAPI
DispatcherThread(
    _In_ PVOID pParameter)
{
    HANDLE hEvent;
    DWORD dwError;

    TRACE("DispatcherThread(%p)\n", pParameter);

    /* Create or open the SECURITY_SERVICES_STARTED event */
    hEvent = CreateEventW(NULL,
                          TRUE,
                          FALSE,
                          L"SECURITY_SERVICES_STARTED");
    if (hEvent == NULL)
    {
        dwError = GetLastError();
        if (dwError != ERROR_ALREADY_EXISTS)
            return dwError;

        hEvent = OpenEventW(SYNCHRONIZE,
                            FALSE,
                            L"SECURITY_SERVICES_STARTED");
        if (hEvent == NULL)
            return GetLastError();
    }

    /* Wait for the SECURITY_SERVICES_STARTED event to be signaled */
    TRACE("Waiting for the SECURITY_SERVICES_STARTED event!\n");
    dwError = WaitForSingleObject(hEvent, INFINITE);
    TRACE("WaitForSingleObject returned %lu\n", dwError);

    /* Close the event handle */
    CloseHandle(hEvent);

    /* Fail, if the event was not signaled */
    if (dwError != WAIT_OBJECT_0)
    {
        ERR("Wait failed!\n");
        return dwError;
    }

    /* This is the security process */
    I_ScIsSecurityProcess();

    /* Start the services */
    TRACE("Start the security services!\n");
    if (!StartServiceCtrlDispatcherW(ServiceTable))
        return GetLastError();

    TRACE("Done!\n");

    return ERROR_SUCCESS;
}


NTSTATUS
WINAPI
ServiceInit(VOID)
{
    HANDLE hThread;
    DWORD dwThreadId;

    TRACE("ServiceInit()\n");

    hThread = CreateThread(NULL,
                           0,
                           DispatcherThread,
                           NULL,
                           0,
                           &dwThreadId);
    if (hThread == NULL)
       return (NTSTATUS)GetLastError();

    return STATUS_SUCCESS;
}

/* EOF */
