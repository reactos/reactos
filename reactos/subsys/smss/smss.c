/* $Id: smss.c,v 1.2 1999/12/01 15:18:54 ekohl Exp $
 *
 * smss.c - Session Manager
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
 * along with this software; see the file COPYING.LIB. If not, write
 * to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 * MA 02139, USA.  
 *
 * --------------------------------------------------------------------
 * 
 * 	19990529 (Emanuele Aliberti)
 * 		Compiled successfully with egcs 1.1.2
 */
#include <ddk/ntddk.h>

#include "smss.h"


void
DisplayString( LPCWSTR lpwString )
{
	UNICODE_STRING us;

	RtlInitUnicodeString (&us, lpwString);
	NtDisplayString (&us);
}


/* Native image's entry point */

void
NtProcessStartup( PSTARTUP_ARGUMENT StartupArgument )
{
	HANDLE	Children[2]; /* csrss, winlogon */
	
	DisplayString( L"Session Manager\n" );


	if (TRUE == InitSessionManager(Children))
	{
		LARGE_INTEGER	Time = {{(DWORD)-1,(DWORD)-1}}; /* infinite? */
		NTSTATUS	wws;

		DisplayString( L"SM: Waiting for process termination...\n" );

#if 0
		wws = NtWaitForMultipleObjects (
				((LONG) sizeof Children / sizeof (HANDLE)),
				Children,
				WaitAny,
				TRUE,	/* alertable */
				& Time 
				);
#endif
		wws = NtWaitForSingleObject (
				Children[CHILD_WINLOGON],
				TRUE,	/* alertable */
				& Time 
				);


//		if (!NT_SUCCESS(wws))
		if (wws > 1)
		{
			DisplayString( L"SM: NtWaitForMultipleObjects failed!\n" );
			/* FIXME: CRASH THE SYSTEM (BSOD) */
		}
		else
		{
			DisplayString( L"SM: Process terminated!\n" );
			/* FIXME: CRASH THE SYSTEM (BSOD) */
		}
	}
	else
	{
		DisplayString( L"SM: initialization failed!\n" );
		/* FIXME: CRASH SYSTEM (BSOD)*/
	}


	/*
	 * OK: CSRSS asked to shutdown the system;
	 * We die.
	 */
#if 0
	NtRaiseHardError (
		STATUS_SYSTEM_PROCESS_TERMINATED,
		...);
#endif

	NtTerminateProcess( NtCurrentProcess(), 0 );
}

/* EOF */
