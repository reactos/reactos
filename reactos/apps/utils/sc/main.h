/*
 *  ReactOS test program - 
 *
 *  main.h
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

#ifndef __MAIN_H__
#define __MAIN_H__

#ifdef __cplusplus
extern "C" {
#endif


DWORD ReportLastError(void);
long getinput(char* Buffer, int buflen);
void dprintf(char* fmt, ...);

typedef enum tag_SC_CMDS {
    SC_CMD_QUERY,
    SC_CMD_QUERYEX,
    SC_CMD_START,
    SC_CMD_PAUSE,
    SC_CMD_INTERROGATE,
    SC_CMD_CONTINUE,
    SC_CMD_STOP,
    SC_CMD_CONFIG,
    SC_CMD_DESCRIPTION,
    SC_CMD_FAILURE,
    SC_CMD_QC,
    SC_CMD_QDESCRIPTION,
    SC_CMD_QFAILURE,
    SC_CMD_DELETE,
    SC_CMD_CREATE,
    SC_CMD_CONTROL,
    SC_CMD_SDSHOW,
    SC_CMD_SDSET,
    SC_CMD_GETDISPLAYNAME,
    SC_CMD_GETKEYNAME,
    SC_CMD_ENUMDEPEND,
    SC_CMD_BOOT,
    SC_CMD_LOCK,
    SC_CMD_QUERYLOCK
} SC_CMDS;

int sc_query(SC_HANDLE hSCManager, SC_CMDS sc_cmd, char* argv[]);
int sc_setup(SC_HANDLE hSCManager, SC_CMDS sc_cmd, char* argv[]);
int sc_config(SC_HANDLE hSCManager, SC_CMDS sc_cmd, char* argv[]);
int sc_command(SC_HANDLE hSCManager, SC_CMDS sc_cmd, char* argv[]);


#ifdef __cplusplus
};
#endif

#endif // __MAIN_H__
