/* $Id: init.c,v 1.2 1999/10/24 17:07:57 rex Exp $
 *
 * init.c - Session Manager initialization
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
 * 	19990530 (Emanuele Aliberti)
 * 		Compiled successfully with egcs 1.1.2
 */
#include <ddk/ntddk.h>
//#include <internal/lpc.h>

BOOL
InitSessionManager(
	HANDLE	Children[]
	)
{
        NTSTATUS Status;
        UNICODE_STRING CmdLineW;

	/* FIXME: Create the \SmApiPort object (LPC) */
	/* FIXME: Create two thread for \SmApiPort */
	/* FIXME: Create the system environment variables */
	/* FIXME: Define symbolic links to kernel devices (MS-DOS names) */
        /* FIXME: Create paging files (if any) other than the first one */
	/* FIXME: Load the well known DLLs */

	/* FIXME: Load the kernel mode driver win32k.sys */
        RtlInitUnicodeString(&CmdLineW,
                             L"\\??\\C:\\reactos\\system32\\drivers\\win32k.sys");
        Status = NtLoadDriver(&CmdLineW);

	if (!NT_SUCCESS(Status))
	{
		return FALSE;
	}

#if 0
	/* Start the Win32 subsystem (csrss.exe) */
	Status = NtCreateProcess(
                        L"\\??\\C:\\reactos\\system32\\csrss.exe",
			& Children[CHILD_CSRSS]
			);
#endif

        /* Start the simple shell (shell.exe) */
        RtlInitUnicodeString(&CmdLineW,
                             L"\\??\\C:\\reactos\\system32\\shell.exe");
        Status = RtlCreateUserProcess(&CmdLineW,
                                      NULL,
                                      NULL,
                                      FALSE,
                                      0,
                                      NULL,
                                      &Children[0],
                                      NULL);

	if (!NT_SUCCESS(Status))
	{
		return FALSE;
	}
#if 0
	/* Start winlogon.exe */
	Status = NtCreateProcess(
                        L"\\??\\C:\\reactos\\system32\\winlogon.exe",
			& Children[CHILD_WINLOGON]
			);
	if (!NT_SUCCESS(Status))
	{
		Status = NtTerminateProcess(
				Children[CHILD_CSRSS]
				);
		return FALSE;
	}
#endif
	/* FIXME: Create the \DbgSsApiPort object (LPC) */
	/* FIXME: Create the \DbgUiApiPort object (LPC) */
	return TRUE;
}


/* EOF */

