/* $Id: init.c,v 1.13 2000/02/21 22:43:15 ekohl Exp $
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


static VOID
SmSetEnvironmentVariables (VOID)
{
	UNICODE_STRING EnvVariable;
	UNICODE_STRING EnvValue;

	/*
	 * The following environment variables are read from the registry.
	 * Since the registry does not work yet, the environment variables
	 * are set one by one, using hard-coded default values.
	 *
	 * Variables:
	 *    SystemRoot = C:\reactos
	 *    SystemDrive = C:
	 *
	 *    OS = ReactOS
	 *    Path = %SystemRoot%\system32;%SystemRoot%
	 *    windir = %SystemRoot%
	 */

	/* Set "SystemRoot = C:\reactos" */
	RtlInitUnicodeString (&EnvVariable,
	                      L"SystemRoot");
	RtlInitUnicodeString (&EnvValue,
	                      L"C:\\reactos");
	RtlSetEnvironmentVariable (&SmSystemEnvironment,
	                           &EnvVariable,
	                           &EnvValue);

	/* Set "SystemDrive = C:" */
	RtlInitUnicodeString (&EnvVariable,
	                      L"SystemDrive");
	RtlInitUnicodeString (&EnvValue,
	                      L"C:");
	RtlSetEnvironmentVariable (&SmSystemEnvironment,
	                           &EnvVariable,
	                           &EnvValue);


	/* Set "OS = ReactOS" */
	RtlInitUnicodeString (&EnvVariable,
	                      L"OS");
	RtlInitUnicodeString (&EnvValue,
	                      L"ReactOS");
	RtlSetEnvironmentVariable (&SmSystemEnvironment,
	                           &EnvVariable,
	                           &EnvValue);
}


BOOL
InitSessionManager (
	HANDLE	Children[]
	)
{
	NTSTATUS Status;
	UNICODE_STRING UnicodeString;
	OBJECT_ATTRIBUTES ObjectAttributes;
	UNICODE_STRING CmdLineW;
	UNICODE_STRING CurrentDirectoryW;
	PRTL_USER_PROCESS_PARAMETERS ProcessParameters;

	/* Create the "\SmApiPort" object (LPC) */
	RtlInitUnicodeString (&UnicodeString,
	                      L"\\SmApiPort");
	InitializeObjectAttributes (&ObjectAttributes,
	                            &UnicodeString,
	                            0xff,
	                            NULL,
	                            NULL);

	Status = NtCreatePort (&SmApiPort,
	                       &ObjectAttributes,
	                       0,
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

	/* FIXME: Load the well known DLLs */

	/* Create paging files */
#if 0
	SmCreatePagingFiles ();
#endif

	/* Load missing registry hives */
//	NtInitializeRegistry (FALSE);

	/* Set environment variables from registry */
	SmSetEnvironmentVariables ();

//#if 0
	/* Load the kernel mode driver win32k.sys */
	RtlInitUnicodeString (&CmdLineW,
	                      L"\\??\\C:\\reactos\\system32\\drivers\\win32k.sys");
	Status = NtLoadDriver (&CmdLineW);

	if (!NT_SUCCESS(Status))
	{
		return FALSE;
	}
//#endif

#if 0
	/* Start the Win32 subsystem (csrss.exe) */
	DisplayString (L"SM: Executing csrss.exe\n");

	RtlInitUnicodeString (&UnicodeString,
	                      L"\\??\\C:\\reactos\\system32\\csrss.exe");

	/* initialize current directory */
	RtlInitUnicodeString (&CurrentDirectoryW,
	                      L"C:\\reactos\\system32\\");

	RtlCreateProcessParameters (&ProcessParameters,
	                            &UnicodeString,
	                            NULL,
	                            &CurrentDirectoryW,
	                            NULL,
	                            SmSystemEnvironment,
	                            NULL,
	                            NULL,
	                            NULL,
	                            NULL);

	Status = RtlCreateUserProcess (&UnicodeString,
	                               0,
	                               ProcessParameters,
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

	RtlDestroyProcessParameters (ProcessParameters);
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

	/* initialize current directory (trailing backslash!!)*/
	RtlInitUnicodeString (&CurrentDirectoryW,
	                      L"C:\\reactos\\");

	RtlCreateProcessParameters (&ProcessParameters,
	                            &UnicodeString,
	                            NULL,
	                            &CurrentDirectoryW,
	                            NULL,
	                            SmSystemEnvironment,
	                            NULL,
	                            NULL,
	                            NULL,
	                            NULL);


	Status = RtlCreateUserProcess (&UnicodeString,
	                               0,
	                               ProcessParameters,
	                               NULL,
	                               NULL,
	                               FALSE,
	                               0,
	                               NULL,
	                               &Children[CHILD_WINLOGON],
	                               NULL);

	RtlDestroyProcessParameters (ProcessParameters);

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
	                       &ObjectAttributes,
	                       0,
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
	                       &ObjectAttributes,
	                       0,
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
