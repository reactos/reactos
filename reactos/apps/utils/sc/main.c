/*
 *  ReactOS SC - service control console program
 *
 *  main.c
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
#include <tchar.h>
#include "main.h"


#define VERSION 1

#ifdef UNICODE
#define TARGET  "UNICODE"
#else
#define TARGET  "MBCS"
#endif

BOOL verbose_flagged = 0;
BOOL status_flagged = 0;

HANDLE OutputHandle;
HANDLE InputHandle;


void dprintf(char* fmt, ...)
{
   va_list args;
   char buffer[255];

   va_start(args, fmt);
   wvsprintfA(buffer, fmt, args);
   WriteConsoleA(OutputHandle, buffer, lstrlenA(buffer), NULL, NULL);
   va_end(args);
}

long getinput(char* buf, int buflen)
{
    DWORD result;

    ReadConsoleA(InputHandle, buf, buflen, &result, NULL);
    return (long)result;
}

DWORD ReportLastError(void)
{
    DWORD dwError = GetLastError();
    if (dwError != ERROR_SUCCESS) {
        PTSTR msg = NULL;
        if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
            0, dwError, MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT), (PTSTR)&msg, 0, NULL)) {
            if (msg != NULL) {
                dprintf("ReportLastError() %d - %s\n", dwError, msg);
            } else {
                dprintf("ERROR: ReportLastError() %d - returned TRUE but with no msg string!\n", dwError);
            }
        } else {
            dprintf("ReportLastError() %d - unknown error\n", dwError);
        }
        if (msg != NULL) {
            LocalFree(msg);
        }
    }
    return dwError;
}

int usage(char* argv0)
{
    dprintf("DESCRIPTION:\n");
    dprintf("\tSC is a command line program used for communicating with\n");
    dprintf("\tthe Service Control Manager and its services.\n");
    dprintf("USAGE:\n");
    dprintf("\tsc <server> [command] [service name] <option1> <option2>...\n");

    dprintf("\tThe optional parameter <server> has the form \"\\ServerName\"\n");
    dprintf("\tFurther help on commands can be obtained by typing: \"sc [command]\"\n");
    dprintf("\tService Commands:\n");
    dprintf("\t  query          : Queries the status for a service, or\n");
    dprintf("\t                   enumerates the status for types of services.\n");
    dprintf("\t  queryex        : Queries the extended status for a service, or\n");
    dprintf("\t                   enumerates the status for types of services.\n");
    dprintf("\t  start          : Starts a service.\n");
    dprintf("\t  pause          : Sends a PAUSE control request to a service.\n");
    dprintf("\t  interrogate    : Sends a INTERROGATE control request to a service.\n");
    dprintf("\t  continue       : Sends a CONTINUE control request to a service.\n");
    dprintf("\t  stop           : Sends a STOP request to a service.\n");
    dprintf("\t  config         : Changes the configuration of a service (persistant).\n");
    dprintf("\t  description    : Changes the description of a service.\n");
    dprintf("\t  failure        : Changes the actions taken by a service upon failure.\n");
    dprintf("\t  qc             : Queries the configuration information for a service.\n");
    dprintf("\t  qdescription   : Queries the description for a service.\n");
    dprintf("\t  qfailure       : Queries the actions taken by a service upon failure.\n");
    dprintf("\t  delete         : Deletes a service (from the registry).\n");
    dprintf("\t  create         : Creates a service. (adds it to the registry).\n");
    dprintf("\t  control        : Sends a control to a service.\n");
//    dprintf("\t  sdshow         : Displays a service's security descriptor.\n");
//    dprintf("\t  sdset          : Sets a service's security descriptor.\n");
    dprintf("\t  GetDisplayName : Gets the DisplayName for a service.\n");
    dprintf("\t  GetKeyName     : Gets the ServiceKeyName for a service.\n");
    dprintf("\t  EnumDepend     : Enumerates Service Dependencies.\n");
    dprintf("\n");
    dprintf("\tService Name Independant Commands:\n");
    dprintf("\t  boot           : (ok | bad) Indicates whether the last boot should\n");
    dprintf("\t                   be saved as the last-known-good boot configuration\n");
    dprintf("\t  Lock           : Locks the SCM Database\n");
    dprintf("\t  QueryLock      : Queries the LockStatus for the SCM Database\n");

    return 0;
}


typedef int (*sc_cmd_proc)(SC_HANDLE hSCManager, SC_CMDS sc_cmd, char* argv[]);

typedef struct {
    SC_CMDS sc_cmd;
    const char* cmd_str;
    sc_cmd_proc funcptr;
    DWORD dwDesiredAccess;
} sc_cmd_entry;

sc_cmd_entry sc_cmds_table[] = {
  { SC_CMD_QUERY, "query", sc_query, SC_MANAGER_ALL_ACCESS },
  { SC_CMD_QUERYEX, "queryex", sc_query, SC_MANAGER_ALL_ACCESS },
  { SC_CMD_START, "start", sc_command, 0L },
  { SC_CMD_PAUSE, "pause", sc_command, 0L },
  { SC_CMD_INTERROGATE, "interrogate", sc_command, 0L },
  { SC_CMD_CONTINUE, "continue", sc_command, 0L },
  { SC_CMD_STOP, "stop", sc_command, 0L },
  { SC_CMD_CONFIG, "config", sc_setup, 0L },
  { SC_CMD_DESCRIPTION, "description", sc_setup, 0L },
  { SC_CMD_FAILURE, "failure", sc_setup, 0L },
  { SC_CMD_QC, "qc", sc_query, 0L },
  { SC_CMD_QDESCRIPTION, "qdescription", sc_query, 0L },
  { SC_CMD_QFAILURE, "qfailure", sc_query, 0L },
  { SC_CMD_DELETE, "delete", sc_setup, 0L },
  { SC_CMD_CREATE, "create", sc_setup, 0L },
  { SC_CMD_CONTROL, "control", sc_setup, 0L },
  { SC_CMD_SDSHOW, "sdshow", sc_query, 0L },
  { SC_CMD_SDSET, "sdset", sc_setup, 0L },
  { SC_CMD_GETDISPLAYNAME, "GetDisplayName", sc_query, 0L },
  { SC_CMD_GETKEYNAME, "GetKeyName", sc_query, 0L },
  { SC_CMD_ENUMDEPEND, "EnumDepend", sc_query, 0L },
  { SC_CMD_BOOT, "boot", sc_config, 0L },
  { SC_CMD_LOCK, "Lock", sc_config, 0L },
  { SC_CMD_QUERYLOCK, "QueryLock", sc_config, 0L }
};


int sc_main(const sc_cmd_entry* cmd_entry,
            const char* sc_machine_name,
            char* sc_cmd_arg[])
{
    int result = 1;
    SC_HANDLE hSCManager;
    DWORD dwDesiredAccess;

    dprintf("sc_main(%s) - called.\n", cmd_entry->cmd_str);
    if (sc_machine_name) {
        dprintf("remote service control not yet implemented.\n");
        return 2;
    }
    dwDesiredAccess = cmd_entry->dwDesiredAccess;
    if (!dwDesiredAccess) dwDesiredAccess = SC_MANAGER_CONNECT + GENERIC_READ;

    hSCManager = OpenSCManagerA(sc_machine_name, NULL, dwDesiredAccess);
    if (hSCManager != NULL) {
        result =  cmd_entry->funcptr(hSCManager, cmd_entry->sc_cmd, sc_cmd_arg);
        if (!CloseServiceHandle(hSCManager)) {
            dprintf("Failed to CLOSE handle to SCM.\n");
            ReportLastError();
        }
    } else {
        dprintf("Failed to open Service Control Manager.\n");
        ReportLastError();
    }
    return result;
}


int __cdecl main(int argc, char* argv[])
{
    int i;
    const char* sc_machine_name = NULL;
    const char* sc_cmd_str = argv[1];
    char** sc_cmd_arg = NULL;

    AllocConsole();
    InputHandle = GetStdHandle(STD_INPUT_HANDLE);
    OutputHandle =  GetStdHandle(STD_OUTPUT_HANDLE);

    dprintf("%s application - build %03d (default: %s)\n", argv[0], VERSION, TARGET);
    if (argc < 2) {
        return usage(argv[0]);
    }

    if ((argv[1][0] == '\\') && (argv[1][1] == '\\')) {
        if (argc < 3) {
            return usage(argv[0]);
        }
        sc_machine_name = argv[1];
        sc_cmd_str = argv[2];
        sc_cmd_arg = &argv[3];
    } else {
        sc_machine_name = NULL;
        sc_cmd_str = argv[1];
        sc_cmd_arg = &argv[2];
    }

    for (i = 0; i < sizeof(sc_cmds_table)/sizeof(sc_cmd_entry); i++) {
        sc_cmd_entry* cmd_entry = &(sc_cmds_table[i]);
        if (lstrcmpiA(sc_cmd_str, cmd_entry->cmd_str) == 0) {
            return sc_main(cmd_entry, sc_machine_name, sc_cmd_arg);
        }
    }
    return usage(argv[0]);
}


#ifdef _NOCRT

//char* args[] = { "sc.exe", "pause", "smplserv" };
char* args[] = { "sc.exe", "EnumDepend", "" };

int __cdecl mainCRTStartup(void)
{
    main(3, args);
    return 0;
}

#endif /*_NOCRT*/
