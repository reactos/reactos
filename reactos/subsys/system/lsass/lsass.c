/* $Id: lsass.c,v 1.1 2001/07/24 10:18:05 ekohl Exp $
 *
 * reactos/services/lsass/lsass.c
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
 * 	19990704 (Emanuele Aliberti)
 * 		Compiled successfully with egcs 1.1.2
 */
#include <ddk/ntddk.h>
#include <wchar.h>

BOOL InitLsa(VOID); /* ./init.c */


void
DisplayString( LPCWSTR lpwString )
{
	UNICODE_STRING us;

	us.Buffer = (LPWSTR) lpwString;
	us.Length = wcslen(lpwString) * sizeof (WCHAR);
	us.MaximumLength = us.Length + sizeof (WCHAR);
	NtDisplayString( & us );
}


/* Native image's entry point */

void
NtProcessStartup( PSTARTUP_ARGUMENT StartupArgument )
{
	DisplayString( L"Local Security Authority Subsystem:\n" );
	DisplayString( L"\tInitializing...\n" );

	if (TRUE == InitLsa())
	{
		DisplayString( L"\tInitialization OK\n" );
		/* FIXME: do nothing loop */
		while (TRUE)
		{
			NtYieldExecution();
		}
	}
	else
	{
		DisplayString( L"\tInitialization failed!\n" );
	}
	NtTerminateProcess( NtCurrentProcess(), 0 );
}


/* EOF */
