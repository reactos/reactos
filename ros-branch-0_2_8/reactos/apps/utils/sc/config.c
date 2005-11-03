/*
 *  ReactOS SC - service control console program
 *
 *  config.c
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


int sc_config(SC_HANDLE hSCManager, SC_CMDS sc_cmd, char* argv[])
{
    dprintf("sc_config(%x, %d, %s) - called.\n", hSCManager, sc_cmd, argv[0]);

    switch (sc_cmd) {
    case SC_CMD_CONFIG:
    case SC_CMD_DESCRIPTION:
    case SC_CMD_FAILURE:
    case SC_CMD_SDSHOW:
    case SC_CMD_SDSET:
        dprintf(" - not implemented.\n");
        break;
    default:
        dprintf("sc_config(%x, %d, %s) - unknown command.\n", hSCManager, sc_cmd, argv[0]);
        break;
    }
    return 0;
}
/*
    switch (sc_cmd) {
    case SC_CMD_QUERY:
    case SC_CMD_QUERYEX:
    case SC_CMD_START:
    case SC_CMD_PAUSE:
    case SC_CMD_INTERROGATE:
    case SC_CMD_CONTINUE:
    case SC_CMD_STOP:
    case SC_CMD_CONFIG:
    case SC_CMD_DESCRIPTION:
    case SC_CMD_FAILURE:
    case SC_CMD_QC:
    case SC_CMD_QDESCRIPTION:
    case SC_CMD_QFAILURE:
    case SC_CMD_DELETE:
    case SC_CMD_CREATE:
    case SC_CMD_CONTROL:
    case SC_CMD_SDSHOW:
    case SC_CMD_SDSET:
    case SC_CMD_GETDISPLAYNAME:
    case SC_CMD_GETKEYNAME:
    case SC_CMD_ENUMDEPEND:
    case SC_CMD_BOOT:
    case SC_CMD_LOCK:
    case SC_CMD_QUERYLOCK:
        break;
    }
 */
