/*
 * Copyright 2007 Jacek Caban for CodeWeavers
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
#if 0
#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#define WINE_FIXME printf
#define WINE_TRACE printf

//#include "wine/debug.h"

//WINE_DEFAULT_DEBUG_CHANNEL(spoolsv);

static WCHAR telnetdW[] = {'T','e','l','n','e','t','D',0};

static SERVICE_STATUS_HANDLE service_handle;
static HANDLE stop_event;

static DWORD WINAPI service_handler( DWORD ctrl, DWORD event_type, LPVOID event_data, LPVOID context )
{
    SERVICE_STATUS status;

    status.dwServiceType             = SERVICE_WIN32;
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
    default:
        WINE_FIXME( "got service ctrl %x\n", ctrl );
        status.dwCurrentState = SERVICE_RUNNING;
        SetServiceStatus( service_handle, &status );
        return NO_ERROR;
    }
}

static void WINAPI serv_main(DWORD argc, LPWSTR *argv)
{
    SERVICE_STATUS status;
    int retval;

    WINE_TRACE( "starting service\n" );

    stop_event = CreateEventW( NULL, TRUE, FALSE, NULL );

    service_handle = RegisterServiceCtrlHandlerExW( telnetdW, service_handler, NULL );
    if (!service_handle)
        return;

    status.dwServiceType             = SERVICE_WIN32;
    status.dwCurrentState            = SERVICE_RUNNING;
    status.dwControlsAccepted        = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    status.dwWin32ExitCode           = 0;
    status.dwServiceSpecificExitCode = 0;
    status.dwCheckPoint              = 0;
    status.dwWaitHint                = 10000;
    SetServiceStatus( service_handle, &status );

    /* Argument Ignored for now */
    retval = kickoff_telnetd();

    WaitForSingleObject( stop_event, INFINITE );

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
#endif
/* EOF */

