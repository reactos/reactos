/*
 *  ReactOS SC - service control console program
 *
 *  setup.c
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

int sc_delete(char* argv[])
{
    dprintf("sc_delete(%s) - not implemented.\n", argv[0]);
    return 0;
}

int sc_boot(char* arg)
{
    dprintf("sc_boot(%s) - not implemented.\n", arg);
    return 0;
}

int sc_setup(SC_HANDLE hSCManager, SC_CMDS sc_cmd, char* argv[])
{
    dprintf("sc_setup(%x, %d, %s) - called.\n", hSCManager, sc_cmd, argv[0]);

    switch (sc_cmd) {
//    case SC_CMD_DESCRIPTION:
//    case SC_CMD_FAILURE:
    case SC_CMD_DELETE:
        return sc_delete(argv);
    case SC_CMD_CREATE:
        dprintf(" - not implemented.\n");
        break;
//    case SC_CMD_SDSHOW:
//    case SC_CMD_SDSET:
    case SC_CMD_BOOT:
        return sc_boot(argv[0]);
    default:
        dprintf("sc_setup(%x, %d, %s) - unknown command.\n", hSCManager, sc_cmd, argv[0]);
        break;
    }
    return 0;
}

