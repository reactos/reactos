/* $Id: shutdown.c,v 1.2 2000/04/25 23:22:57 ea Exp $
 * 
 * EAU shutdown.c 1.4.1
 * 
 * Copyright (C) 1997,1998,1999 Emanuele Aliberti
 *
 * --------------------------------------------------------------------
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this software; see the file COPYING.LIB. If
 * not, write to the Free Software Foundation, Inc., 675 Mass Ave,
 * Cambridge, MA 02139, USA.  
 *
 * --------------------------------------------------------------------
 * 1999-05-14 (Emanuele Aliberti)
 * 	Released version 1.4.1 under GNU GPL for the ReactOS project.
 * --------------------------------------------------------------------
 */
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include "win32err.h"

#ifndef SE_PRIVILEGE_ENABLED
#define NTOS_MODE_USER
#include <ntos.h>
#endif


struct _EWX 
{
	CHAR	mode;
	UINT	id;
};

static
struct _EWX modes[] =
{
	{ 'f',		EWX_FORCE    },
	{ 'l',		EWX_LOGOFF   },
	{ 'p',		EWX_POWEROFF },
	{ 'r',		EWX_REBOOT   },
	{ 's',		EWX_SHUTDOWN },
	{ 0,		0 }
};


static
UINT
DecodeArg( CHAR * modestr )
{
	register int i;

	if (modestr[0] != '-' && modestr[0] != '/')
	{
		return (UINT) -1;
	}
	for ( i = 0; modes[i].mode; ++i)
	{
		if (modestr[1] == modes[i].mode)
		{
			return modes[i].id;
		}
	}
	return (UINT) -1;
}



static
const 
char * usage = "\
Shutdown ver. 1.4.1  (compiled on %s, at %s)\n\
Copyright (C) 1997-1999 Emanuele Aliberti\n\n\
usage: %s [-f] [-l] [-p] [-r] [-s]\n\
  f (FORCE)    processes are unconditionally terminated\n\
  l (LOGOFF)   logs the current user off\n\
  p (POWEROFF) turns off the power, if possibile\n\
  r (REBOOT)   reboots the system\n\
  s (SHUTDOWN) shuts down the system to a point at which\n\
               it is safe to turn off the power\n\n\
  Any other letter will print this help message.\n";

int
main(
	int	argc,
	char	* argv []
	)
{
	UINT             mode;
	HANDLE           h;
	TOKEN_PRIVILEGES tp;

	mode = (argc == 2) 
		? DecodeArg(argv[1])
		: DecodeArg("-?");
	if (mode == (UINT) -1)
	{
		fprintf(
			stderr,
			usage,
			__DATE__,
			__TIME__,
			argv[0]	
			);
		return EXIT_SUCCESS;
	}
	/* 
	 * Get the current process token handle 
	 * so we can get shutdown privilege. 
	 */
	if (FALSE == OpenProcessToken(
		GetCurrentProcess(),
		(TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY),
		& h
		)
	) {
		PrintWin32Error(
			L"while opening the process",
			GetLastError()
			);
		return EXIT_FAILURE;
	}
	/*
	 * Get the LUID for shutdown privilege.
	 */
	if (FALSE == LookupPrivilegeValue(
			NULL,
			SE_SHUTDOWN_NAME, 
			& tp.Privileges[0].Luid
			)
	) {
		PrintWin32Error(
			L"while looking up privileges",
			GetLastError()
			);
		return EXIT_FAILURE;
	}
	tp.PrivilegeCount = 1;	/* One privilege to seat */
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	/*
	 * Get shutdown privilege for this process.
	 */
	if (FALSE == AdjustTokenPrivileges(
			h,
			FALSE,
			& tp,
			0,
			(PTOKEN_PRIVILEGES) NULL,
			0
			)
	) {
		PrintWin32Error(
			L"while adjusting shutdown privilege",
			GetLastError()
			);
		return EXIT_FAILURE;
	}
	/* Now really exit! */
	if (FALSE == ExitWindowsEx(mode, 0))
	{
		PrintWin32Error(
			L"ExitWindowsEx",
			GetLastError()
			);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}


/* EOF */

