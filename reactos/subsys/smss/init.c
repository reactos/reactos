/* $Id: init.c,v 1.5 1999/12/04 21:11:00 ea Exp $
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
#include <ntdll/rtl.h>

#include "smss.h"

#define NDEBUG


/* GLOBAL VARIABLES *********************************************************/

HANDLE SmApiPort = INVALID_HANDLE_VALUE;
HANDLE DbgSsApiPort = INVALID_HANDLE_VALUE;
HANDLE DbgUiApiPort = INVALID_HANDLE_VALUE;

PVOID SmSystemEnvironment = NULL;


/* FUNCTIONS ****************************************************************/

#if 0
static VOID
SmCreatePagingFiles (VOID)
{
	UNICODE_STRING FileName;
	ULONG ulCurrentSize;

	/* FIXME: Read file names from registry */

	RtlInitUnicodeString (&FileName,
	                      L"\\??\\C:\\reactos\\pagefile.sys");

	NtCreatePagingFile (&FileName,
	                    50,
	                    80,
	                    &ulCurrentSize);
}
#endif


BOOL
InitSessionManager (
	HANDLE	Children[]
	)
{
	NTSTATUS Status;
	UNICODE_STRING UnicodeString;
	OBJECT_ATTRIBUTES ObjectAttributes;
	UNICODE_STRING CmdLineW;

	/* Create the "\SmApiPort" object (LPC) */
	RtlInitUnicodeString (&UnicodeString,
	                      L"\\SmApiPort");
	InitializeObjectAttributes (&ObjectAttributes,
	                            &UnicodeString,
	                            0xff,
	                            NULL,
	                            NULL);

	Status = NtCreatePort (&SmApiPort,
	                       0,
	                       &ObjectAttributes,
	                       0,
	                       0);

	if (!NT_SUCCESS(Status))
	{
		return FALSE;
	}

#ifndef NDEBUG
	DisplayString (L"SM: \\SmApiPort created...\n");
#endif

	/* Create two threads for "\SmApiPort" */
	RtlCreateUserThread (NtCurrentProcess (),
	                     NULL,
	                     FALSE,
	                     0,
	                     NULL,
	                     NULL,
	                     (PTHREAD_START_ROUTINE)SmApiThread,
	                     (PVOID)SmApiPort,
	                     NULL,
	                     NULL);

	RtlCreateUserThread (NtCurrentProcess (),
	                     NULL,
	                     FALSE,
	                     0,
	                     NULL,
	                     NULL,
	                     (PTHREAD_START_ROUTINE)SmApiThread,
	                     (PVOID)SmApiPort,
	                     NULL,
	                     NULL);

	/* Create the system environment */
	Status = RtlCreateEnvironment (TRUE,
	                               &SmSystemEnvironment);
	if (!NT_SUCCESS(Status))
		return FALSE;
#ifndef NDEBUG
	DisplayString (L"SM: System Environment created\n");
#endif

	/* FIXME: Define symbolic links to kernel devices (MS-DOS names) */

	/* FIXME: Run all programs in the boot execution list */

	/* FIXME: Process the file rename list */

	/* Create paging files */
#if 0
	SmCreatePagingFiles ();
#endif

	/* FIXME: Load the well known DLLs */

	/* FIXME: Load missing registry hives */

	/* FIXME: Set environment variables from registry */

	/* Load the kernel mode driver win32k.sys */
	RtlInitUnicodeString (&CmdLineW,
	                      L"\\??\\C:\\reactos\\system32\\drivers\\win32k.sys");
	Status = NtLoadDriver (&CmdLineW);

	if (!NT_SUCCESS(Status))
	{
		return FALSE;
	}

	/* Start the Win32 subsystem (csrss.exe) */
#if 0
	DisplayString (L"SM: Executing csrss.exe\n");
	RtlInitUnicodeString (&UnicodeString,
	                      L"\\??\\C:\\reactos\\system32\\csrss.exe");

	Status = RtlCreateUserProcess (&UnicodeString,
	                               NULL,
	                               NULL,
	                               FALSE,
	                               0,
	                               NULL,
	                               &Children[CHILD_CSRSS],
	                               NULL);

	if (!NT_SUCCESS(Status))
	{
		DisplayString (L"SM: Loading csrss.exe failed!\n");
		return FALSE;
	}
#endif

	/* Start the simple shell (shell.exe) */
	DisplayString (L"SM: Executing shell\n");
	RtlInitUnicodeString (&UnicodeString,
	                      L"\\??\\C:\\reactos\\system32\\shell.exe");

#if 0
	/* Start the logon process (winlogon.exe) */
	RtlInitUnicodeString (&CmdLineW,
	                      L"\\??\\C:\\reactos\\system32\\winlogon.exe");
#endif
	Status = RtlCreateUserProcess (&UnicodeString,
	                               NULL,
	                               NULL,
	                               FALSE,
	                               0,
	                               NULL,
	                               &Children[CHILD_WINLOGON],
	                               NULL);

	if (!NT_SUCCESS(Status))
	{
		DisplayString (L"SM: Loading shell.exe failed!\n");
#if 0
		NtTerminateProcess (Children[CHILD_CSRSS],
		                    0);
#endif
		return FALSE;
	}

	/* Create the \DbgSsApiPort object (LPC) */
	RtlInitUnicodeString (&UnicodeString,
	                      L"\\DbgSsApiPort");
	InitializeObjectAttributes (&ObjectAttributes,
	                            &UnicodeString,
	                            0xff,
	                            NULL,
	                            NULL);

	Status = NtCreatePort (&DbgSsApiPort,
	                       0,
	                       &ObjectAttributes,
	                       0,
	                       0);

	if (!NT_SUCCESS(Status))
	{
		return FALSE;
	}
#ifndef NDEBUG
	DisplayString (L"SM: DbgSsApiPort created...\n");
#endif

	/* Create the \DbgUiApiPort object (LPC) */
	RtlInitUnicodeString (&UnicodeString,
	                      L"\\DbgUiApiPort");
	InitializeObjectAttributes (&ObjectAttributes,
	                            &UnicodeString,
	                            0xff,
	                            NULL,
	                            NULL);

	Status = NtCreatePort (&DbgUiApiPort,
	                       0,
	                       &ObjectAttributes,
	                       0,
	                       0);

	if (!NT_SUCCESS(Status))
	{
		return FALSE;
	}
#ifndef NDEBUG
	DisplayString (L"SM: DbgUiApiPort created...\n");
#endif

	return TRUE;
}

/* EOF */
