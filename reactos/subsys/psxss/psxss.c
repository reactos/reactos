/* $Id: psxss.c,v 1.2 1999/09/07 17:12:39 ea Exp $
 *
 * reactos/subsys/psxss/psxss.c 
 * 
 * POSIX+ subsystem server process
 * 
 * ReactOS Operating System
 * 
 * --------------------------------------------------------------------
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING. If not, write
 * to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 * MA 02139, USA.  
 *
 * --------------------------------------------------------------------
 * 
 * 	19990605 (Emanuele Aliberti)
 * 		Compiled successfully with egcs 1.1.2
 */
#include <ddk/ntddk.h>
//#include <internal/lpc.h>

BOOL TerminationRequestPending = FALSE;

BOOL InitializeServer(void);


void
DisplayString(
	LPCWSTR	Message
	)
{
	UNICODE_STRING title;

	title.Buffer = (LPWSTR) Message;
	title.Length = wcslen(title.Buffer) * sizeof (WCHAR);
	title.MaximumLength = title.Length + sizeof (WCHAR);
	NtDisplayString( & title );
}


/* Native process' entry point */

void
NtProcessStartup( PSTARTUP_ARGUMENT StartupArgument )
{
	DisplayString( L"POSIX+ Subsystem\n" );

	if (TRUE == InitializeServer())
	{
		while (FALSE == TerminationRequestPending)
		{
			/* Do nothing! Should it
			 * be the SbApi port's
			 * thread instead?
			 */
			NtYieldExecution();
		}
	}
	else
	{
		DisplayString( L"PSX: Subsystem initialization failed.\n" );
		/*
		 * Tell SM we failed.
		 */
	}
	NtTerminateProcess( NtCurrentProcess(), 0 );
}

/* EOF */
