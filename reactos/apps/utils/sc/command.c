/*
 *  ReactOS SC - service control console program
 *
 *  command.c
 *
 *  Copyright (C) 2002  Robert Dickenson <robd@reactos.org>
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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <windows.h>
#include <wchar.h>
#include <tchar.h>
#include "main.h"


int sc_Lock(SC_HANDLE hSCManager)
{
    dprintf("sc_Lock(%x) - not implemented.\n", hSCManager);
    return 0;
}

int sc_QueryLock(SC_HANDLE hSCManager)
{
    QUERY_SERVICE_LOCK_STATUS LockStatus;
    DWORD cbBufSize = sizeof(QUERY_SERVICE_LOCK_STATUS);
    DWORD cbBytesNeeded;

    dprintf("sc_QueryLock() - called.\n");

        if (QueryServiceLockStatus(hSCManager, &LockStatus, cbBufSize, &cbBytesNeeded)) {

        } else {
            dprintf("Failed to Query Service Lock Status.\n");
            ReportLastError();
        }

        if (!CloseServiceHandle(hSCManager)) {
            dprintf("Failed to CLOSE handle to SCM.\n");
            ReportLastError();
        }

    return 0;
}

int sc_control(SC_HANDLE hSCManager, char* service, DWORD dwControl)
{
    int ret = 0;
    SC_HANDLE schService;
    SERVICE_STATUS serviceStatus;

    dprintf("sc_control(%x, %s, %d) - called.\n", hSCManager, service, dwControl);

    schService = OpenServiceA(hSCManager, service, SERVICE_ALL_ACCESS);
    if (schService != NULL) {
        ret = ControlService(schService, dwControl, &serviceStatus);
        dprintf("ControlService(%x, %x, %x) returned %d\n", schService, dwControl, &serviceStatus, ret);
        if (!ret) {


        }
        CloseServiceHandle(schService);
    }
    return ret;
}

int sc_command(SC_HANDLE hSCManager, SC_CMDS sc_cmd, char* argv[])
{
//    dprintf("sc_command(%x, %d, %s) - called.\n", hSCManager, sc_cmd, argv[]);
    switch (sc_cmd) {
    case SC_CMD_START:
        //return sc_control(hSCManager, sc_cmd_arg, SERVICE_CONTROL_START);
        dprintf(" - not implemented.\n");
        break;
    case SC_CMD_PAUSE:
        return sc_control(hSCManager, argv[0], SERVICE_CONTROL_PAUSE);
    case SC_CMD_INTERROGATE:
        return sc_control(hSCManager, argv[0], SERVICE_CONTROL_INTERROGATE);
    case SC_CMD_CONTINUE:
        return sc_control(hSCManager, argv[0], SERVICE_CONTROL_CONTINUE);
    case SC_CMD_STOP:
        return sc_control(hSCManager, argv[0], SERVICE_CONTROL_STOP);
       
//    case SC_CMD_CONFIG:
//    case SC_CMD_DESCRIPTION:
//    case SC_CMD_CONTROL:

    case SC_CMD_LOCK:
        return sc_Lock(hSCManager);
    case SC_CMD_QUERYLOCK:
        return sc_QueryLock(hSCManager);
    default:
        dprintf("sc_command(%x, %d, %s) - unknown command.\n", hSCManager, sc_cmd, argv[0]);
        break;
    }
    return 0;
}
