/* $Id: init.c,v 1.34 2002/05/22 15:55:51 ekohl Exp $
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

#include "smss.h"

#define NDEBUG

/* TYPES ********************************************************************/


/* GLOBAL VARIABLES *********************************************************/

HANDLE SmApiPort = INVALID_HANDLE_VALUE;
HANDLE DbgSsApiPort = INVALID_HANDLE_VALUE;
HANDLE DbgUiApiPort = INVALID_HANDLE_VALUE;

PWSTR SmSystemEnvironment = NULL;


/* FUNCTIONS ****************************************************************/


static NTSTATUS STDCALL
SmObjectDirectoryQueryRoutine(PWSTR ValueName,
			      ULONG ValueType,
			      PVOID ValueData,
			      ULONG ValueLength,
			      PVOID Context,
			      PVOID EntryContext)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING UnicodeString;
  HANDLE WindowsDirectory;
  NTSTATUS Status;

#ifndef NDEBUG
  PrintString("ValueName '%S'  Type %lu  Length %lu\n", ValueName, ValueType, ValueLength);
  PrintString("ValueData '%S'\n", (PWSTR)ValueData);
#endif

  RtlInitUnicodeString(&UnicodeString,
		       (PWSTR)ValueData);

  InitializeObjectAttributes(&ObjectAttributes,
			     &UnicodeString,
			     0,
			     NULL,
			     NULL);

  Status = ZwCreateDirectoryObject(&WindowsDirectory,
				   0,
				   &ObjectAttributes);

  return(Status);
}


static NTSTATUS
SmCreateObjectDirectories(VOID)
{
  RTL_QUERY_REGISTRY_TABLE QueryTable[2];
  NTSTATUS Status;

  RtlZeroMemory(&QueryTable,
		sizeof(QueryTable));

  QueryTable[0].Name = L"ObjectDirectories";
  QueryTable[0].QueryRoutine = SmObjectDirectoryQueryRoutine;

  Status = RtlQueryRegistryValues(RTL_REGISTRY_CONTROL,
				  L"\\Session Manager",
				  QueryTable,
				  NULL,
				  NULL);

  return(Status);
}


static NTSTATUS STDCALL
SmDosDevicesQueryRoutine(PWSTR ValueName,
			 ULONG ValueType,
			 PVOID ValueData,
			 ULONG ValueLength,
			 PVOID Context,
			 PVOID EntryContext)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING DeviceName;
  UNICODE_STRING LinkName;
  HANDLE LinkHandle;
  WCHAR LinkBuffer[80];
  NTSTATUS Status = STATUS_SUCCESS;

#ifndef NDEBUG
  PrintString("ValueName '%S'  Type %lu  Length %lu\n", ValueName, ValueType, ValueLength);
  PrintString("ValueData '%S'\n", (PWSTR)ValueData);
#endif

  if (ValueType = REG_SZ)
    {
      swprintf(LinkBuffer, L"\\??\\%s",
	       ValueName);
      RtlInitUnicodeString(&LinkName,
			   LinkBuffer);
      RtlInitUnicodeString(&DeviceName,
			   (PWSTR)ValueData);

#ifndef NDEBUG
      PrintString("SM: Linking %wZ --> %wZ\n",
		  &LinkName,
		  &DeviceName);
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
	  PrintString("SmDosDevicesQueryRoutine: NtCreateSymbolicLink( %wZ --> %wZ ) failed!\n",
		      &LinkName,
		      &DeviceName);
	}
      NtClose(LinkHandle);
    }

  return(Status);
}


static NTSTATUS
SmInitDosDevices(VOID)
{
  RTL_QUERY_REGISTRY_TABLE QueryTable[2];
  NTSTATUS Status;

  RtlZeroMemory(&QueryTable,
		sizeof(QueryTable));

  QueryTable[0].QueryRoutine = SmDosDevicesQueryRoutine;

  Status = RtlQueryRegistryValues(RTL_REGISTRY_CONTROL,
				  L"\\Session Manager\\DOS Devices",
				  QueryTable,
				  NULL,
				  NULL);
  return(Status);
}


static NTSTATUS STDCALL
SmRunBootAppsQueryRoutine(PWSTR ValueName,
			  ULONG ValueType,
			  PVOID ValueData,
			  ULONG ValueLength,
			  PVOID Context,
			  PVOID EntryContext)
{
#if 0
  PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
  RTL_PROCESS_INFO ProcessInfo;
  WCHAR Description[256];
  WCHAR ImagePath[256];
  WCHAR CommandLine[256];
  PWSTR p1, p2;
  ULONG len;
  NTSTATUS Status = STATUS_SUCCESS;

#ifndef NDEBUG
  PrintString("ValueName '%S'  Type %lu  Length %lu\n", ValueName, ValueType, ValueLength);
  PrintString("ValueData '%S'\n", (PWSTR)ValueData);
#endif

  /* Extract the description */
  p1 = wcschr((PWSTR)ValueData, L' ');
  len = p1 - (PWSTR)ValueData;
  memcpy(Description,ValueData, len * sizeof(WCHAR));
  Description[len] = 0;

  /* Extract the full image path */
  p1++;
  p2 = wcschr(p1, L' ');
  if (p2 != NULL)
    len = p2 - p1;
  else
    len = wcslen(p1);
  memcpy(ImagePath, p1, len * sizeof(WCHAR));
  ImagePath[len] = 0;

  /* Extract the command line */
  if (p2 == NULL)
    {
      CommandLine[0] = 0;
    }
  else
    {
      p2++;
      wcscpy(CommandLine, p2);
    }

#ifndef NDEBUG
  PrintString("Running %S...\n", Description);
  PrintString("Executable: '%S'\n", ImagePath);
  PrintString("CommandLine: '%S'\n", CommandLine);
#endif

#if 0
  /* initialize executable path */
  wcscpy(UnicodeBuffer, L"\\??\\");
  wcscat(UnicodeBuffer, SharedUserData->NtSystemRoot);
  wcscat(UnicodeBuffer, L"\\system32\\csrss.exe");
  RtlInitUnicodeString(&ImagePathString,
		       UnicodeBuffer);

  RtlInitUnicodeString(&CommandLineString,
		       CommandLine);

  RtlCreateProcessParameters(&ProcessParameters,
			     &ImagePathString,
			     NULL,
			     NULL,
			     &CommandLineString,
			     NULL,
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

  /* FIXME: wait for process termination */

#endif

  return(Status);
#endif
  return(STATUS_SUCCESS);
}


/*
 * Run native applications listed in the registry.
 *
 *  Key:
 *    \Registry\Machine\SYSTEM\CurrentControlSet\Control\Session Manager
 *
 *  Value (format: "<description> <executable> <command line>":
 *    BootExecute = "autocheck autochk *"
 */
static NTSTATUS
SmRunBootApps(VOID)
{
  RTL_QUERY_REGISTRY_TABLE QueryTable[2];
  NTSTATUS Status;

  RtlZeroMemory(&QueryTable,
		sizeof(QueryTable));

  QueryTable[0].Name = L"BootExecute";
  QueryTable[0].QueryRoutine = SmRunBootAppsQueryRoutine;

  Status = RtlQueryRegistryValues(RTL_REGISTRY_CONTROL,
				  L"\\Session Manager",
				  QueryTable,
				  NULL,
				  NULL);
  if (!NT_SUCCESS(Status))
    {
      PrintString("SmRunBootApps: RtlQueryRegistryValues() failed! (Status %lx)\n", Status);
    }

//  PrintString("*** System stopped ***\n");
//  for(;;);

  return(Status);
}


static NTSTATUS
SmProcessFileRenameList(VOID)
{
#ifndef NDEBUG
  PrintString("SmProcessFileRenameList() called\n");
#endif

#ifndef NDEBUG
  PrintString("SmProcessFileRenameList() done\n");
#endif

  return(STATUS_SUCCESS);
}


static NTSTATUS STDCALL
SmPagingFilesQueryRoutine(PWSTR ValueName,
			  ULONG ValueType,
			  PVOID ValueData,
			  ULONG ValueLength,
			  PVOID Context,
			  PVOID EntryContext)
{
  UNICODE_STRING FileName;
  LARGE_INTEGER InitialSize;
  LARGE_INTEGER MaximumSize;
  NTSTATUS Status;

#ifndef NDEBUG
  PrintString("ValueName '%S'  Type %lu  Length %lu\n", ValueName, ValueType, ValueLength);
  PrintString("ValueData '%S'\n", (PWSTR)ValueData);
#endif

  RtlInitUnicodeString(&FileName,
		       (PWSTR)ValueData);

  /*
   * FIXME:
   *  read initial and maximum size from the registry or use default values
   *
   * Format: "<path>[ <initial_size>[ <maximum_size>]]"
   */

  InitialSize.QuadPart = 50 * 4096;
  MaximumSize.QuadPart = 80 * 4096;

  Status = NtCreatePagingFile(&FileName,
			      &InitialSize,
			      &MaximumSize,
			      0);

  return(STATUS_SUCCESS);
}


static NTSTATUS
SmCreatePagingFiles(VOID)
{
  RTL_QUERY_REGISTRY_TABLE QueryTable[2];
  NTSTATUS Status;

  RtlZeroMemory(&QueryTable,
		sizeof(QueryTable));

  QueryTable[0].Name = L"PagingFiles";
  QueryTable[0].QueryRoutine = SmPagingFilesQueryRoutine;

  Status = RtlQueryRegistryValues(RTL_REGISTRY_CONTROL,
				  L"\\Session Manager\\Memory Management",
				  QueryTable,
				  NULL,
				  NULL);

  return(Status);
}


static NTSTATUS
SmSetEnvironmentVariables(VOID)
{
	UNICODE_STRING EnvVariable;
	UNICODE_STRING EnvValue;
	UNICODE_STRING EnvExpandedValue;
	ULONG ExpandedLength;
	WCHAR ExpandBuffer[512];
	WCHAR ValueBuffer[MAX_PATH];

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
	RtlInitUnicodeString (&EnvValue,
	                      ValueBuffer);
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

  return(STATUS_SUCCESS);
}


NTSTATUS
InitSessionManager(HANDLE Children[])
{
  NTSTATUS Status;
  UNICODE_STRING UnicodeString;
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING CmdLineW;
  PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
  RTL_PROCESS_INFO ProcessInfo;
  HANDLE CsrssInitEvent;
  WCHAR UnicodeBuffer[MAX_PATH];

  /* Create object directories */
  Status = SmCreateObjectDirectories();
  if (!NT_SUCCESS(Status))
    {
      PrintString("SM: Failed to create object directories! (Status %lx)\n", Status);
      return(Status);
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
      return(Status);
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

  /* Create the system environment */
  Status = RtlCreateEnvironment(FALSE,
				&SmSystemEnvironment);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }
#ifndef NDEBUG
  DisplayString(L"SM: System Environment created\n");
#endif

  /* Define symbolic links to kernel devices (MS-DOS names) */
  Status = SmInitDosDevices();
  if (!NT_SUCCESS(Status))
    {
      PrintString("SM: Failed to create dos device links! (Status %lx)\n", Status);
      return(Status);
    }

  /* Run all programs in the boot execution list */
  Status = SmRunBootApps();
  if (!NT_SUCCESS(Status))
    {
      PrintString("SM: Failed to run boot applications! (Status %lx)\n", Status);
      return(Status);
    }

  /* Process the file rename list */
  Status = SmProcessFileRenameList();
  if (!NT_SUCCESS(Status))
    {
      PrintString("SM: Failed to process the file rename list (Status %lx)\n", Status);
      return(Status);
    }

  /* FIXME: Load the well known DLLs */
//  SmPreloadDlls();

  /* Create paging files */
  Status = SmCreatePagingFiles();
  if (!NT_SUCCESS(Status))
    {
      PrintString("SM: Failed to create paging files (Status %lx)\n", Status);
      return(Status);
    }

  /* Load remaining registry hives */
  NtInitializeRegistry(FALSE);

  /* Set environment variables from registry */
  Status = SmSetEnvironmentVariables();
  if (!NT_SUCCESS(Status))
    {
      PrintString("SM: Failed to initialize the system environment (Status %lx)\n", Status);
      return(Status);
    }

  /* Load the kernel mode driver win32k.sys */
  RtlInitUnicodeString(&CmdLineW,
		       L"\\SystemRoot\\system32\\drivers\\win32k.sys");
  Status = NtLoadDriver(&CmdLineW);
#if 0
  if (!NT_SUCCESS(Status))
    {
      return(Status);
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
      return(Status);
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
      return(Status);
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
      return(Status);
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
      return(Status);
    }
#ifndef NDEBUG
  DisplayString (L"SM: DbgUiApiPort created...\n");
#endif

  return(STATUS_SUCCESS);
}

/* EOF */
