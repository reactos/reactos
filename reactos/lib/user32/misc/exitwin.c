/* $Id: exitwin.c,v 1.1 1999/05/15 13:35:57 ea Exp $
 *
 * exitwin.c
 *
 * Copyright (c) 1999 Emanuele Aliberti
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
 *
 * ReactOS user32.dll ExitWindows functions.
 *
 * 19990515 EA	Naive implementation.
 */

#include <windows.h>
#include <ddk/ntddk.h>


/***********************************************************************
 * 	ExitWindowsEx
 *
 * ARGUMENTS
 * 	uFlags	shutdown operation
 *		EWX_FORCE	Processes are killed.
 *		
 *		EWX_LOGOFF	Caller owned processed are killed.
 *		
 *		EWX_POWEROFF	Turns off the power.
 *		
 *		EWX_REBOOT	Restart the system.
 *				SE_SHUTDOWN_NAME privilege reqired.
 *
 *		EWX_SHUTDOWN	Same as EWX_REBOOT, but rebooting.
 *				SE_SHUTDOWN_NAME privilege reqired.
 *				
 * 	dwReserved 	reserved
 * 	
 * REVISIONS
 * 	1999-05-15 EA
 */
BOOL
__stdcall
ExitWindowsEx(
	UINT	uFlags,
	DWORD	dwReserved
	)
{
	NTSTATUS rv;
	
	if (uFlags & (
			EWX_FORCE |
			EWX_LOGOFF |
			EWX_POWEROFF |
			EWX_REBOOT |
			EWX_SHUTDOWN
		)
	) {
		/* Unknown flag! */
		SetLastError(ERROR_INVALID_FLAG_NUMBER);
		return FALSE;
	}
	/* FIXME: call csrss.exe;
	 * 
	 * Naive implementation: call the kernel directly.
	 * This code will be moved in csrss.exe when
	 * available.
	 */
	if (EWX_POWEROFF & uFlags)
	{
		rv = NtShutdownSystem(ShutdownPowerOff);
	}
	else if (EWX_REBOOT & uFlags)
	{
		rv = NtShutdownSystem(ShutdownReboot);
	}
	else if (EWX_SHUTDOWN & uFlags)
	{
		rv = NtShutdownSystem(ShutdownNoReboot);
	}
	else
	{
		/* How to implement others flags semantics? */
		SetLastError(ERROR_INVALID_FLAG_NUMBER);
		rv = (NTSTATUS) -1;
	}
	return NT_SUCCESS(rv)
		? TRUE
		: FALSE;
}


/***********************************************************************
 * 	ExitWindows
 *
 * ARGUMENTS
 *	dwReserved,	reserved 
 * 	uReserved 	reserved
 */
BOOL
__stdcall
ExitWindows(
	DWORD	dwReserved,	// reserved 
	UINT	uReserved 	// reserved 
	)
{
	return ExitWindowsEx(
			EWX_SHUTDOWN,
			dwReserved
			);
}


/* EOF */
