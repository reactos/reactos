/* $Id: init.c,v 1.19 2000/10/09 00:18:00 ekohl Exp $
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
#include <ntos.h>
#include <ntdll/rtl.h>
#include <napi/lpc.h>
#include <napi/shared_data.h>

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
	                      L"\\SystemRoot\\pagefile.sys");

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
	UNICODE_STRING EnvExpandedValue;
	ULONG ExpandedLength;
	WCHAR ExpandBuffer[512];
	WCHAR ValueBuffer[MAX_PATH];
	PKUSER_SHARED_DATA SharedUserData = 
		(PKUSER_SHARED_DATA)USER_SHARED_DATA_BASE;

	/*
	 * The following environment variables are read from the registry.
	 * Because the registry does not work yet, the environment variables
	 * are set one by one, using information from the shared user page.
	 *
	 * Variables (example):
	 *    SystemRoot = C:\reactos
	 *    SystemDrive = C:
	 *
	 *    OS = ReactOS
	 *    Path = %SystemRoot%\system32;%SystemRoot%
	 *    windir = %SystemRoot%
	 */

	/* copy system root into value buffer */
	wcscpy (ValueBuffer, SharedUserData->NtSystemRoot);

	/* set "SystemRoot = C:\reactos" */
	RtlInitUnicodeString (&EnvVariable,
	                      L"SystemRoot");
	RtlInitUnicodeString (&EnvValue,
	                      ValueBuffer);
	RtlSetEnvironmentVariable (&SmSystemEnvironment,
	                           &EnvVariable,
	                           &EnvValue);

	/* cut off trailing path */
	ValueBuffer[2] = 0;

	/* Set "SystemDrive = C:" */
	RtlInitUnicodeString (&EnvVariable,
	                      L"SystemDrive");
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


	/* Set "Path = %SystemRoot%\system32;%SystemRoot%" */
	RtlInitUnicodeString (&EnvVariable,
	                      L"Path");
	RtlInitUnicodeString (&EnvValue,
	                      L"%SystemRoot%\\system32;%SystemRoot%");
	EnvExpandedValue.Length = 0;
	EnvExpandedValue.MaximumLength = 512 * sizeof(WCHAR);
	EnvExpandedValue.Buffer = ExpandBuffer;
	*ExpandBuffer = 0;
	RtlExpandEnvironmentStrings_U (SmSystemEnvironment,
	                               &EnvValue,
	                               &EnvExpandedValue,
	                               &ExpandedLength);
	RtlSetEnvironmentVariable (&SmSystemEnvironment,
	                           &EnvVariable,
	                           &EnvExpandedValue);

	/* Set "windir = %SystemRoot%" */
	RtlInitUnicodeString (&EnvVariable,
	                      L"windir");
	RtlInitUnicodeString (&EnvValue,
	                      L"%SystemRoot%");
	EnvExpandedValue.Length = 0;
	EnvExpandedValue.MaximumLength = 512 * sizeof(WCHAR);
	EnvExpandedValue.Buffer = ExpandBuffer;
	*ExpandBuffer = 0;
	RtlExpandEnvironmentStrings_U (SmSystemEnvironment,
	                               &EnvValue,
	                               &EnvExpandedValue,
	                               &ExpandedLength);
	RtlSetEnvironmentVariable (&SmSystemEnvironment,
	                           &EnvVariable,
	                           &EnvExpandedValue);
}


BOOL InitSessionManager (HANDLE	Children[])
{
   NTSTATUS Status;
   UNICODE_STRING UnicodeString;
   OBJECT_ATTRIBUTES ObjectAttributes;
   UNICODE_STRING CmdLineW;
   UNICODE_STRING CurrentDirectoryW;
   PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
   RTL_USER_PROCESS_INFO ProcessInfo;
   HANDLE CsrssInitEvent;
   
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
   
#if 0
   /* Create paging files */
   SmCreatePagingFiles ();
#endif
   
   /* Load remaining registry hives */
   NtInitializeRegistry (FALSE);
   
   /* Set environment variables from registry */
   SmSetEnvironmentVariables ();

   /* Load the kernel mode driver win32k.sys */
   RtlInitUnicodeString (&CmdLineW,
			 L"\\SystemRoot\\system32\\drivers\\win32k.sys");
   Status = NtLoadDriver (&CmdLineW);
   
   if (!NT_SUCCESS(Status))
     {
	return FALSE;
     }
   
   /* Run csrss.exe */
   RtlInitUnicodeString(&UnicodeString,
			L"\\CsrssInitDone");
   InitializeObjectAttributes(&ObjectAttributes,
			      &UnicodeString,
			      EVENT_ALL_ACCESS,
			      0,
			      NULL);
   Status = NtCreateEvent(&CsrssInitEvent,
			  EVENT_ALL_ACCESS,
			  &ObjectAttributes,
			  TRUE,
			  FALSE);
   if (!NT_SUCCESS(Status))
     {
	DbgPrint("Failed to create csrss notification event\n");
     }
   
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
				  0,
				  0,
				  &ProcessInfo);
   
   RtlDestroyProcessParameters (ProcessParameters);
   
   if (!NT_SUCCESS(Status))
     {
	DisplayString (L"SM: Loading csrss.exe failed!\n");
	return FALSE;
     }
   
   DbgPrint("SM: Waiting for csrss\n");
   NtWaitForSingleObject(CsrssInitEvent,
			 FALSE,
			 NULL);
   DbgPrint("SM: Finished waiting for csrss\n");
   
   Children[CHILD_CSRSS] = ProcessInfo.ProcessHandle;


	/* Start the simple shell (shell.exe) */
	DisplayString (L"SM: Executing shell\n");
	RtlInitUnicodeString (&UnicodeString,
	                      L"\\??\\C:\\reactos\\system32\\shell.exe");
#if 0
	/* Start the logon process (winlogon.exe) */
	DisplayString (L"SM: Running winlogon\n");
	RtlInitUnicodeString (&UnicodeString,
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
	                               0,
	                               0,
	                               &ProcessInfo);

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
	Children[CHILD_WINLOGON] = ProcessInfo.ProcessHandle;

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
