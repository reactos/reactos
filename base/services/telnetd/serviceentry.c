/*
 * Copyright 2007 Jacek Caban for CodeWeavers
 * Copyright 2026 Aravinda Samarathunga (I-A-S)
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

#include "telnetd.hpp"

#include <stdarg.h>
#include <winbase.h>
#include <winsvc.h>
#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(telnetd);

static WCHAR telnetdW[] = {'t','e','l','n','e','t','d',0};

static SERVICE_STATUS_HANDLE service_handle;
static HANDLE stop_event, worker_thread_handle;

static DWORD WINAPI service_handler( DWORD ctrl, DWORD event_type, LPVOID event_data, LPVOID context )
{
    SERVICE_STATUS status;

    status.dwServiceType             = SERVICE_WIN32_OWN_PROCESS;
    status.dwControlsAccepted        = SERVICE_ACCEPT_STOP;
    status.dwWin32ExitCode           = 0;
    status.dwServiceSpecificExitCode = 0;
    status.dwCheckPoint              = 0;
    status.dwWaitHint                = 0;

    switch(ctrl)
    {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
        WINE_TRACE( "shutting down\n" );
        status.dwCurrentState     = SERVICE_STOP_PENDING;
        status.dwControlsAccepted = 0;
        SetServiceStatus( service_handle, &status );
        SetEvent( stop_event );
        return NO_ERROR;
    
    case SERVICE_CONTROL_INTERROGATE:
        return NO_ERROR;

    default:
        WINE_FIXME( "got service ctrl %x\n", ctrl );
        return NO_ERROR;
    }
}

static void WINAPI serv_main(DWORD argc, LPWSTR *argv)
{
    SERVICE_STATUS status;
    TELNETD_STATUS telnetd_status;
    DWORD wait_result;
    DWORD worker_thread_ec;
    HANDLE handles[2];

    (void)argc;
    (void)argv;

    WINE_TRACE( "starting service\n" );

    stop_event = CreateEventW( NULL, TRUE, FALSE, NULL );
    if (!stop_event) {

        WINE_TRACE( "failed to create stop event\n" );
        return;
    }

    service_handle = RegisterServiceCtrlHandlerExW( telnetdW, service_handler, NULL );
    if (!service_handle) {
        
        CloseHandle(stop_event);
        return;
    }

    status.dwServiceType             = SERVICE_WIN32_OWN_PROCESS;
    status.dwCurrentState            = SERVICE_START_PENDING;
    status.dwControlsAccepted        = 0;
    status.dwWin32ExitCode           = 0;
    status.dwServiceSpecificExitCode = 0;
    status.dwCheckPoint              = 0;
    status.dwWaitHint                = 10000;
    SetServiceStatus( service_handle, &status );

    telnetd_status = kickoff_telnetd(&worker_thread_handle);

    if(telnetd_status != TELNETD_STATUS_OK)
    {
        status.dwCurrentState     = SERVICE_STOPPED;
        status.dwControlsAccepted = 0;
        status.dwWin32ExitCode           = ERROR_SERVICE_SPECIFIC_ERROR;
        status.dwServiceSpecificExitCode = telnetd_status;
        SetServiceStatus( service_handle, &status );
        WINE_TRACE( "service failed to start: %d\n", telnetd_status );

        CloseHandle(stop_event);

        return;
    }

    status.dwCurrentState            = SERVICE_RUNNING;
    status.dwControlsAccepted        = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    SetServiceStatus( service_handle, &status );

    handles[0] = worker_thread_handle;
    handles[1] = stop_event;
    wait_result = WaitForMultipleObjects( 2, handles, FALSE, INFINITE );
    if (wait_result == WAIT_OBJECT_0) {
        if (!GetExitCodeThread(worker_thread_handle, &worker_thread_ec)) {
            worker_thread_ec = TELNETD_STATUS_ERR_UNKNOWN;
        }

        // In this case no one asked worker to stop
        // and yet it stopped when it should be an
        // infinite loop, so something went wrong and
        // reporting TELNETD_STATUS_OK can be confusing.
        if(worker_thread_ec == TELNETD_STATUS_OK)
            worker_thread_ec = TELNETD_STATUS_ERR_UNKNOWN;

        shutdown_telnetd();

        status.dwCurrentState     = SERVICE_STOPPED;
        status.dwControlsAccepted = 0;
        status.dwWin32ExitCode           = ERROR_SERVICE_SPECIFIC_ERROR;
        status.dwServiceSpecificExitCode = worker_thread_ec;
        SetServiceStatus( service_handle, &status );
        WINE_TRACE( "telnetd exited unexpectedly: %d\n", worker_thread_ec );

        CloseHandle(stop_event);

        return;
    } else if(wait_result == WAIT_FAILED) {
        status.dwWin32ExitCode = GetLastError();

        shutdown_telnetd();

        status.dwCurrentState     = SERVICE_STOPPED;
        status.dwControlsAccepted = 0;
        SetServiceStatus( service_handle, &status );
        WINE_TRACE( "wait failed: %lu\n",  status.dwWin32ExitCode);

        CloseHandle(stop_event);

        return;
    }

    shutdown_telnetd();
    CloseHandle(stop_event);

    status.dwCurrentState     = SERVICE_STOPPED;
    status.dwControlsAccepted = 0;
    SetServiceStatus( service_handle, &status );
    WINE_TRACE( "service stopped\n" );
}

int main(int argc, char **argv)
{
    static const SERVICE_TABLE_ENTRYW servtbl[] = {
        {telnetdW, serv_main},
        {NULL, NULL}
    };

    WINE_TRACE("(%d %p)\n", argc, argv);

    StartServiceCtrlDispatcherW(servtbl);
    return 0;
}
/* EOF */

