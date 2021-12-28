/*
 *  ReactOS Services
 *  Copyright (C) 2015 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Services
 * FILE:             base/services/schedsvc/schedsvc.c
 * PURPOSE:          Scheduling service
 * PROGRAMMER:       Eric Kohl <eric.kohl@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(schedsvc);

/* GLOBALS ******************************************************************/

static WCHAR ServiceName[] = L"Schedule";

static SERVICE_STATUS_HANDLE ServiceStatusHandle;
static SERVICE_STATUS ServiceStatus;

HANDLE Events[3] = {NULL, NULL, NULL}; // StopEvent, UpdateEvent, Timer


/* FUNCTIONS *****************************************************************/

static VOID
UpdateServiceStatus(DWORD dwState)
{
    ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    ServiceStatus.dwCurrentState = dwState;

    if (dwState == SERVICE_PAUSED || dwState == SERVICE_RUNNING)
        ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP |
                                           SERVICE_ACCEPT_SHUTDOWN |
                                           SERVICE_ACCEPT_PAUSE_CONTINUE;
    else
        ServiceStatus.dwControlsAccepted = 0;

    ServiceStatus.dwWin32ExitCode = 0;
    ServiceStatus.dwServiceSpecificExitCode = 0;
    ServiceStatus.dwCheckPoint = 0;

    if (dwState == SERVICE_START_PENDING ||
        dwState == SERVICE_STOP_PENDING ||
        dwState == SERVICE_PAUSE_PENDING ||
        dwState == SERVICE_CONTINUE_PENDING)
        ServiceStatus.dwWaitHint = 10000;
    else
        ServiceStatus.dwWaitHint = 0;

    SetServiceStatus(ServiceStatusHandle,
                     &ServiceStatus);
}


static DWORD WINAPI
ServiceControlHandler(DWORD dwControl,
                      DWORD dwEventType,
                      LPVOID lpEventData,
                      LPVOID lpContext)
{
    TRACE("ServiceControlHandler()\n");

    switch (dwControl)
    {
        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:
            TRACE("  SERVICE_CONTROL_STOP/SERVICE_CONTROL_SHUTDOWN received\n");
            UpdateServiceStatus(SERVICE_STOP_PENDING);
            /* Stop listening to incoming RPC messages */
            RpcMgmtStopServerListening(NULL);
            if (Events[0] != NULL)
                SetEvent(Events[0]);
            return ERROR_SUCCESS;

        case SERVICE_CONTROL_PAUSE:
            TRACE("  SERVICE_CONTROL_PAUSE received\n");
            UpdateServiceStatus(SERVICE_PAUSED);
            return ERROR_SUCCESS;

        case SERVICE_CONTROL_CONTINUE:
            TRACE("  SERVICE_CONTROL_CONTINUE received\n");
            UpdateServiceStatus(SERVICE_RUNNING);
            return ERROR_SUCCESS;

        case SERVICE_CONTROL_INTERROGATE:
            TRACE("  SERVICE_CONTROL_INTERROGATE received\n");
            SetServiceStatus(ServiceStatusHandle,
                             &ServiceStatus);
            return ERROR_SUCCESS;

#if 0
        case 128:
            TRACE("  Start Shell control received\n");
            return ERROR_SUCCESS;

        case 129:
            TRACE("  Logoff control received\n");
            return ERROR_SUCCESS;
#endif

        default:
            TRACE("  Control %lu received\n", dwControl);
            return ERROR_CALL_NOT_IMPLEMENTED;
    }
}


static
DWORD
ServiceInit(VOID)
{
    HANDLE hThread;
    DWORD dwError;

    /* Initialize the job list */
    InitializeListHead(&JobListHead);

    /* Initialize the job list lock */
    RtlInitializeResource(&JobListLock);

    /* Initialize the start list */
    InitializeListHead(&StartListHead);

    /* Initialize the start list lock */
    RtlInitializeResource(&StartListLock);

    /* Load stored jobs from the registry */
    dwError = LoadJobs();
    if (dwError != ERROR_SUCCESS)
        return dwError;

    /* Start the RPC thread */
    hThread = CreateThread(NULL,
                           0,
                           (LPTHREAD_START_ROUTINE)RpcThreadRoutine,
                           NULL,
                           0,
                           NULL);
    if (!hThread)
    {
        ERR("Could not create the RPC thread\n");
        return GetLastError();
    }

    CloseHandle(hThread);

    /* Create the stop event */
    Events[0] = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (Events[0] == NULL)
    {
        ERR("Could not create the stop event\n");
        return GetLastError();
    }

    /* Create the update event */
    Events[1] = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (Events[1] == NULL)
    {
        ERR("Could not create the update event\n");
        CloseHandle(Events[0]);
        return GetLastError();
    }

    Events[2] = CreateWaitableTimerW(NULL, FALSE, NULL);
    if (Events[2] == NULL)
    {
        ERR("Could not create the timer\n");
        CloseHandle(Events[1]);
        CloseHandle(Events[0]);
        return GetLastError();
    }

    return ERROR_SUCCESS;
}


VOID WINAPI
SchedServiceMain(DWORD argc, LPTSTR *argv)
{
    DWORD dwWait, dwError;

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    TRACE("SchedServiceMain()\n");

    ServiceStatusHandle = RegisterServiceCtrlHandlerExW(ServiceName,
                                                        ServiceControlHandler,
                                                        NULL);
    if (!ServiceStatusHandle)
    {
        ERR("RegisterServiceCtrlHandlerExW() failed! (Error %lu)\n", GetLastError());
        return;
    }

    UpdateServiceStatus(SERVICE_START_PENDING);

    dwError = ServiceInit();
    if (dwError != ERROR_SUCCESS)
    {
        ERR("Service stopped (dwError: %lu\n", dwError);
        UpdateServiceStatus(SERVICE_STOPPED);
        return;
    }

    UpdateServiceStatus(SERVICE_RUNNING);

    GetNextJobTimeout(Events[2]);

    for (;;)
    {
        /* Wait for the next event */
        TRACE("Wait for next event!\n");
        dwWait = WaitForMultipleObjects(3, Events, FALSE, INFINITE);
        if (dwWait == WAIT_OBJECT_0)
        {
            TRACE("Stop event signaled!\n");
            break;
        }
        else if (dwWait == WAIT_OBJECT_0 + 1)
        {
            TRACE("Update event signaled!\n");

            RtlAcquireResourceShared(&JobListLock, TRUE);
            GetNextJobTimeout(Events[2]);
            RtlReleaseResource(&JobListLock);
        }
        else if (dwWait == WAIT_OBJECT_0 + 2)
        {
            TRACE("Timeout: Start the next job!\n");

            RtlAcquireResourceExclusive(&JobListLock, TRUE);
            RunCurrentJobs();
            GetNextJobTimeout(Events[2]);
            RtlReleaseResource(&JobListLock);
        }
    }

    /* Close the start and update event handles */
    CloseHandle(Events[0]);
    CloseHandle(Events[1]);
    CloseHandle(Events[2]);

    /* Stop the service */
    UpdateServiceStatus(SERVICE_STOPPED);
}


BOOL WINAPI
DllMain(HINSTANCE hinstDLL,
        DWORD fdwReason,
        LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDLL);
            break;

        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}
