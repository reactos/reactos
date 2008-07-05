/*
 *  ReactOS kill
 *  Copyright (C) 2003 ReactOS Team
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
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kill.exe
 * FILE:            apps/utils/kill/kill.c
 * PURPOSE:         Kill a running Process
 * PROGRAMMER:      Steven Edwards (Steven_Ed4153@yahoo.com)
 */

/* Thanks to David, Capser, Eric and others for the example code
 * from the old shell.exe
 */

#define WIN32_LEAN_AND_MEAN	/* Exclude rarely-used stuff from Windows headers */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

int
ExecuteKill(char * lpPid)
{
	HANDLE	hProcess;
	DWORD	dwProcessId;

	dwProcessId = (DWORD) atol(lpPid);
		fprintf( stderr, "Killing PID %ld...\n",dwProcessId);
	hProcess = OpenProcess(
			PROCESS_TERMINATE,
			FALSE,
			dwProcessId
			);
	if (NULL == hProcess)
	{
		fprintf( stderr, "Could not open the process with PID = %ld\n", dwProcessId);
		return 0;
	}
	if (FALSE == TerminateProcess(
			hProcess,
			0
			)
	) {
		fprintf( stderr, "Could not terminate the process with PID = %ld\n",	dwProcessId);
		return 0;
	}
	CloseHandle(hProcess);
	return 0;
}

int main(int argc, char *argv[])
{
  char tail;

  if (argc < 2)
  {
      fprintf( stderr, "Usage: %s PID (Process ID) \n", argv[0] );
      return 1;
  }
  tail = ExecuteKill(argv[1]);
  return 0;
}
