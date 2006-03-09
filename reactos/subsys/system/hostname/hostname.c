/*
 *  ReactOS Win32 Applications
 *  Copyright (C) 2005 ReactOS Team
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
/* $Id$
 *
 * COPYRIGHT : See COPYING in the top level directory
 * PROJECT   : ReactOS/Win32 get host name 
 * FILE      : subsys/system/hostname/hostname.c
 * PROGRAMMER: Emanuele Aliberti (ea@reactos.com)
 */
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main (int argc, char ** argv)
{
	if (1 == argc)
	{
		TCHAR ComputerName [MAX_COMPUTERNAME_LENGTH + 1];
		DWORD ComputerNameSize = sizeof ComputerName / sizeof ComputerName[0];

		ZeroMemory (ComputerName, sizeof ComputerName );
		if (GetComputerName(ComputerName, & ComputerNameSize))
		{
			printf ("%s\n", ComputerName);
			return EXIT_SUCCESS;
		}
		fprintf (stderr, "%s: Win32 error %ld.\n",
			argv[0], GetLastError());
		return EXIT_FAILURE;
	}else{
		if (0 == strcmp(argv[1],"-s"))
		{
			fprintf(stderr,"%s: -s not supported.\n",argv[0]);
			return EXIT_FAILURE;
		}else{
			printf("Print the current host's name.\n\nhostname\n");
		}
	}
	return EXIT_SUCCESS;
}
/* EOF */
