/*
 *  ReactOS SC - service control console program
 *
 *  query.c
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



int DisplayServiceInfo(BOOL display_ex)
{
    const char* type_str = "WIN32_SHARE_PROCESS";
    const char* state_str = "RUNNING";
    const char* flags_str = "RUNS_IN_SYSTEM_PROCESS";
    const char* features_str = "NOT_STOPABLE,NOT_PAUSABLE,IGNORES_SHUTDOWN";

    const char* ServiceName = "none";
    const char* DisplayName = "none";
    int Type;
    int State;
    int Win32ExitCode;
    int ServiceExitCode;
    int CheckPoint;
    int WaitHint;
    int Pid;
    int Flags;

    Type = 20;
    State = 4;
    Win32ExitCode = 0;
    ServiceExitCode = 0;
    CheckPoint = 0;
    WaitHint = 0;
    Pid = 0;
    Flags = 0;

    dprintf("\nSERVICE_NAME: %s\n", ServiceName);
    dprintf("DISPLAY_NAME: %s\n", DisplayName);
    dprintf("\tTYPE               : %d  %s\n", Type, type_str);
    dprintf("\tSTATE              : %d  %s\n", State, state_str);
    dprintf("\t                   :    (%s)\n", features_str);
    dprintf("\tWIN32_EXIT_CODE    : %d  (0x%01x)\n", Win32ExitCode, Win32ExitCode);
    dprintf("\tSERVICE_EXIT_CODE  : %d  (0x%01x)\n", ServiceExitCode, ServiceExitCode);
    dprintf("\tCHECKPOINT         : 0x%01x\n", CheckPoint);
    dprintf("\tWAIT_HINT          : 0x%01x\n", WaitHint);

    if (display_ex) {
        dprintf("\tPID                : %d\n", Pid);
        dprintf("\tFLAGS              : %s\n", flags_str);
    }

    return 0;
}

int EnumServicesInfo(void)
{
    int entriesRead = 0;
    int resumeIndex = 0;

    dprintf("Enum: entriesRead = %d\n", entriesRead);
    dprintf("Enum: resumeIndex = %d\n", resumeIndex);

    DisplayServiceInfo(1);

    return 0;
}

int sc_query(SC_HANDLE hSCManager, SC_CMDS sc_cmd, char* argv[])
{
    switch (sc_cmd) {
    case SC_CMD_QUERY:
    case SC_CMD_QUERYEX:
    case SC_CMD_QC:
    case SC_CMD_QDESCRIPTION:
    case SC_CMD_QFAILURE:
    case SC_CMD_SDSHOW:
    case SC_CMD_GETDISPLAYNAME:
    case SC_CMD_GETKEYNAME:
        dprintf("sc_query(%x, %d, %s) - command not implemented.\n", hSCManager, sc_cmd, argv[0]);
        break;
    case SC_CMD_ENUMDEPEND:
        return EnumServicesInfo();
//    case SC_CMD_QUERYLOCK:
//        return sc_QueryLock(hSCManager);
    default:
        dprintf("sc_query(%x, %d, %s) - unknown command.\n", hSCManager, sc_cmd, argv[0]);
        break;
    }
    return 0;
}

