/*
 * msiexec.exe implementation
 *
 * Copyright 2007 Google (James Hawkins)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <stdio.h>

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msiexec);

static SERVICE_STATUS_HANDLE hstatus;

static HANDLE thread;
static HANDLE kill_event;

void KillService(void)
{
    WINE_TRACE("Killing service\n");
    SetEvent(kill_event);
}

static BOOL UpdateSCMStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode,
                            DWORD dwServiceSpecificExitCode)
{
    SERVICE_STATUS status;

    status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    status.dwCurrentState = dwCurrentState;

    if (dwCurrentState == SERVICE_START_PENDING)
        status.dwControlsAccepted = 0;
    else
    {
        status.dwControlsAccepted = SERVICE_ACCEPT_STOP |
                                    SERVICE_ACCEPT_PAUSE_CONTINUE |
                                    SERVICE_ACCEPT_SHUTDOWN;
    }

    if (dwServiceSpecificExitCode == 0)
    {
        status.dwWin32ExitCode = dwWin32ExitCode;
    }
    else
    {
        status.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
    }

    status.dwServiceSpecificExitCode = dwServiceSpecificExitCode;
    status.dwCheckPoint = 0;
    status.dwWaitHint = 0;

    if (!SetServiceStatus(hstatus, &status))
    {
        fprintf(stderr, "Failed to set service status\n");
        KillService();
        return FALSE;
    }

    return TRUE;
}

static void WINAPI ServiceCtrlHandler(DWORD code)
{
    WINE_TRACE("%d\n", code);

    switch (code)
    {
        case SERVICE_CONTROL_SHUTDOWN:
        case SERVICE_CONTROL_STOP:
            UpdateSCMStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);
            KillService();
            return;
        default:
            fprintf(stderr, "Unhandled service control code: %d\n", code);
            break;
    }

    UpdateSCMStatus(SERVICE_RUNNING, NO_ERROR, 0);
}

static DWORD WINAPI ServiceExecutionThread(LPVOID param)
{
    while (TRUE)
    {
        /* do nothing */
    }

    return 0;
}

static BOOL StartServiceThread(void)
{
    DWORD id;

    thread = CreateThread(0, 0, ServiceExecutionThread, 0, 0, &id);
    if (!thread)
    {
        fprintf(stderr, "Failed to create thread\n");
        return FALSE;
    }

    return TRUE;
}

static void WINAPI ServiceMain(DWORD argc, LPSTR *argv)
{
    hstatus = RegisterServiceCtrlHandlerA("MSIServer", ServiceCtrlHandler);
    if (!hstatus)
    {
        fprintf(stderr, "Failed to register service ctrl handler\n");
        return;
    }

    UpdateSCMStatus(SERVICE_START_PENDING, NO_ERROR, 0);

    kill_event = CreateEventW(0, TRUE, FALSE, 0);
    if (!kill_event)
    {
        fprintf(stderr, "Failed to create event\n");
        KillService();
        return;
    }

    if (!StartServiceThread())
    {
        KillService();
        return;
    }

    UpdateSCMStatus(SERVICE_RUNNING, NO_ERROR, 0);

    WaitForSingleObject(kill_event, INFINITE);
    KillService();
}

DWORD DoService(void)
{
    char service_name[] = "MSIServer";

    const SERVICE_TABLE_ENTRYA service[] =
    {
        {service_name, ServiceMain},
        {NULL, NULL},
    };

    WINE_TRACE("Starting MSIServer service\n");

    if (!StartServiceCtrlDispatcherA(service))
    {
        fprintf(stderr, "Failed to start MSIServer service\n");
        return 1;
    }

    return 0;
}
