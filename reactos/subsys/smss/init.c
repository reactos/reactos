/* $Id: init.c,v 1.31 2002/03/18 16:16:47 ekohl Exp $
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

/* TYPES ********************************************************************/

/*
 * NOTE: This is only used until the dos device links are
 *       read from the registry!!
 */
typedef struct
{
   PWSTR DeviceName;
   PWSTR LinkName;
} LINKDATA, *PLINKDATA;

/* GLOBAL VARIABLES *********************************************************/

HANDLE SmApiPort = INVALID_HANDLE_VALUE;
HANDLE DbgSsApiPort = INVALID_HANDLE_VALUE;
HANDLE DbgUiApiPort = INVALID_HANDLE_VALUE;

PWSTR SmSystemEnvironment = NULL;


/* FUNCTIONS ****************************************************************/

static VOID
SmCreatePagingFiles (VOID)
{
  UNICODE_STRING FileName;
  LARGE_INTEGER InitialSize;
  LARGE_INTEGER MaximumSize;
  NTSTATUS Status;

  /* FIXME: Read file names from registry */
  
  RtlInitUnicodeString (&FileName,
			L"\\SystemRoot\\pagefile.sys");

  InitialSize.QuadPart = 50 * 4096;
  MaximumSize.QuadPart = 80 * 4096;

  Status = NtCreatePagingFile(&FileName,
			      &InitialSize,
			      &MaximumSize,
			      0);
  if (!NT_SUCCESS(Status))
    {
      PrintString("SM: Failed to create paging file (Status was 0x%.8X)\n", Status);
    }
}


static VOID
SmInitDosDevices(VOID)
{
   OBJECT_ATTRIBUTES ObjectAttributes;
   UNICODE_STRING DeviceName;
   UNICODE_STRING LinkName;
   HANDLE LinkHandle;
#if 0
   HANDLE DeviceHandle;
   IO_STATUS_BLOCK StatusBlock;
#endif
   NTSTATUS Status;
   WCHAR LinkBuffer[80];
   
   PLINKDATA LinkPtr;
   LINKDATA LinkData[] =
	{{L"\\Device\\NamedPipe", L"PIPE"},
	 {L"\\Device\\Null", L"NUL"},
	 {L"\\Device\\Mup", L"UNC"},
	 {L"\\Device\\MailSlot", L"MAILSLOT"},
	 {L"\\DosDevices\\COM1", L"AUX"},
	 {L"\\DosDevices\\LPT1", L"PRN"},
	 {NULL, NULL}};
   
   /* FIXME: Read the list of symbolic links from the registry!! */
   
   LinkPtr = &LinkData[0];
   while (LinkPtr->DeviceName != NULL)
     {
	swprintf(LinkBuffer, L"\\??\\%s",
		 LinkPtr->LinkName);
	RtlInitUnicodeString(&LinkName,
			     LinkBuffer);
	RtlInitUnicodeString(&DeviceName,
			     LinkPtr->DeviceName);
   
#if 0
	/* check if target device exists (can be opened) */
	InitializeObjectAttributes(&ObjectAttributes,
				   &DeviceName,
				   0,
				   NULL,
				   NULL);
   
	Status = NtOpenFile(&DeviceHandle,
			    0x10001,
			    &ObjectAttributes,
			    &StatusBlock,
			    1,
			    FILE_SYNCHRONOUS_IO_NONALERT);
	if (NT_SUCCESS(Status))
	  {
	     NtClose(DeviceHandle);
#endif
	     /* create symbolic link */
	     InitializeObjectAttributes(&ObjectAttributes,
					&LinkName,
					OBJ_PERMANENT,
					NULL,
					NULL);
   
	     Status = NtCreateSymbolicLinkObject(&LinkHandle,
						 SYMBOLIC_LINK_ALL_ACCESS,
						 &ObjectAttributes,
						 &DeviceName);
	     if (!NT_SUCCESS(Status))
	       {
		  PrintString("SM: NtCreateSymbolicLink( %wZ --> %wZ ) failed!\n",
			      &LinkName,
			      &DeviceName);
	       }
	     NtClose(LinkHandle);
#if 0
	  }
#endif
	LinkPtr++;
     }
}


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
  PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
  RTL_PROCESS_INFO ProcessInfo;
  HANDLE CsrssInitEvent;
  HANDLE WindowsDirectory;
  WCHAR UnicodeBuffer[MAX_PATH];
  PKUSER_SHARED_DATA SharedUserData = 
    (PKUSER_SHARED_DATA)USER_SHARED_DATA_BASE;

  /*
   * FIXME: The '\Windows' directory is created by csrss.exe but
   *        win32k.sys needs it at intialization and it is loaded
   *        before csrss.exe
   */

  /*
   * Create the '\Windows' directory
   */
  RtlInitUnicodeString(&UnicodeString,
		       L"\\Windows");

  InitializeObjectAttributes(&ObjectAttributes,
			     &UnicodeString,
			     0,
			     NULL,
			     NULL);

  Status = ZwCreateDirectoryObject(&WindowsDirectory,
				   0,
				   &ObjectAttributes);
  if (!NT_SUCCESS(Status))
    {
      DisplayString(L"SM: Could not create \\Windows directory!\n");
      return FALSE;
    }

  /* Create the "\SmApiPort" object (LPC) */
  RtlInitUnicodeString(&UnicodeString,
		       L"\\SmApiPort");
  InitializeObjectAttributes(&ObjectAttributes,
			     &UnicodeString,
			     PORT_ALL_ACCESS,
			     NULL,
			     NULL);

  Status = NtCreatePort(&SmApiPort,
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
  RtlCreateUserThread(NtCurrentProcess(),
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
  Status = RtlCreateEnvironment(FALSE,
				&SmSystemEnvironment);
  if (!NT_SUCCESS(Status))
    {
      return FALSE;
    }
#ifndef NDEBUG
  DisplayString (L"SM: System Environment created\n");
#endif

  /* Define symbolic links to kernel devices (MS-DOS names) */
  SmInitDosDevices();

  /* FIXME: Run all programs in the boot execution list */
//  SmRunBootApps();

  /* FIXME: Process the file rename list */
//  SmProcessFileRenameList();

  /* FIXME: Load the well known DLLs */
//  SmPreloadDlls();

  /* Create paging files */
  SmCreatePagingFiles();

  /* Load remaining registry hives */
  NtInitializeRegistry(FALSE);

  /* Set environment variables from registry */
  SmSetEnvironmentVariables();

  /* Load the kernel mode driver win32k.sys */
  RtlInitUnicodeString(&CmdLineW,
		       L"\\SystemRoot\\system32\\drivers\\win32k.sys");
  Status = NtLoadDriver(&CmdLineW);
#if 0
  if (!NT_SUCCESS(Status))
    {
      return FALSE;
    }
#endif

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

  /*
   * Start the Win32 subsystem (csrss.exe)
   */

  /* initialize executable path */
  wcscpy(UnicodeBuffer, L"\\??\\");
  wcscat(UnicodeBuffer, SharedUserData->NtSystemRoot);
  wcscat(UnicodeBuffer, L"\\system32\\csrss.exe");
  RtlInitUnicodeString(&UnicodeString,
		       UnicodeBuffer);

  RtlCreateProcessParameters(&ProcessParameters,
			     &UnicodeString,
			     NULL,
			     NULL,
			     NULL,
			     SmSystemEnvironment,
			     NULL,
			     NULL,
			     NULL,
			     NULL);

  Status = RtlCreateUserProcess(&UnicodeString,
				OBJ_CASE_INSENSITIVE,
				ProcessParameters,
				NULL,
				NULL,
				NULL,
				FALSE,
				NULL,
				NULL,
				&ProcessInfo);

  RtlDestroyProcessParameters (ProcessParameters);

  if (!NT_SUCCESS(Status))
    {
      DisplayString(L"SM: Loading csrss.exe failed!\n");
      return FALSE;
    }

  NtWaitForSingleObject(CsrssInitEvent,
			FALSE,
			NULL);

  Children[CHILD_CSRSS] = ProcessInfo.ProcessHandle;

  /*
   * Start the logon process (winlogon.exe)
   */

  /* initialize executable path */
  wcscpy(UnicodeBuffer, L"\\??\\");
  wcscat(UnicodeBuffer, SharedUserData->NtSystemRoot);
  wcscat(UnicodeBuffer, L"\\system32\\winlogon.exe");
  RtlInitUnicodeString(&UnicodeString,
		       UnicodeBuffer);

  RtlCreateProcessParameters(&ProcessParameters,
			     &UnicodeString,
			     NULL,
			     NULL,
			     NULL,
			     SmSystemEnvironment,
			     NULL,
			     NULL,
			     NULL,
			     NULL);

  Status = RtlCreateUserProcess(&UnicodeString,
				OBJ_CASE_INSENSITIVE,
				ProcessParameters,
				NULL,
				NULL,
				NULL,
				FALSE,
				NULL,
				NULL,
				&ProcessInfo);

  RtlDestroyProcessParameters(ProcessParameters);

  if (!NT_SUCCESS(Status))
    {
      DisplayString(L"SM: Loading winlogon.exe failed!\n");
      NtTerminateProcess(Children[CHILD_CSRSS],
			 0);
      return FALSE;
    }
  Children[CHILD_WINLOGON] = ProcessInfo.ProcessHandle;

  /* Create the \DbgSsApiPort object (LPC) */
  RtlInitUnicodeString(&UnicodeString,
		       L"\\DbgSsApiPort");
  InitializeObjectAttributes(&ObjectAttributes,
			     &UnicodeString,
			     PORT_ALL_ACCESS,
			     NULL,
			     NULL);

  Status = NtCreatePort(&DbgSsApiPort,
			&ObjectAttributes,
			0,
			0,
			0);

  if (!NT_SUCCESS(Status))
    {
      return FALSE;
    }
#ifndef NDEBUG
  DisplayString(L"SM: DbgSsApiPort created...\n");
#endif

  /* Create the \DbgUiApiPort object (LPC) */
  RtlInitUnicodeString(&UnicodeString,
		       L"\\DbgUiApiPort");
  InitializeObjectAttributes(&ObjectAttributes,
			     &UnicodeString,
			     PORT_ALL_ACCESS,
			     NULL,
			     NULL);

  Status = NtCreatePort(&DbgUiApiPort,
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
