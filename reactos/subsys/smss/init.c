/* $Id: init.c,v 1.47 2003/05/14 21:27:53 ekohl Exp $
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

/* INCLUDES *****************************************************************/

#include <ntos.h>
#include <ntdll/rtl.h>
#include <napi/lpc.h>

#include "smss.h"

#define NDEBUG


/* GLOBALS ******************************************************************/

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
  NTSTATUS Status = STATUS_SUCCESS;

#ifndef NDEBUG
  PrintString("ValueName '%S'  Type %lu  Length %lu\n", ValueName, ValueType, ValueLength);
  PrintString("ValueData '%S'\n", (PWSTR)ValueData);
#endif
  if (ValueType != REG_SZ)
    {
      return(STATUS_SUCCESS);
    }

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
  NTSTATUS Status;

#ifndef NDEBUG
  PrintString("ValueName '%S'  Type %lu  Length %lu\n", ValueName, ValueType, ValueLength);
  PrintString("ValueData '%S'\n", (PWSTR)ValueData);
#endif

  if (ValueType != REG_SZ)
    {
      return(STATUS_SUCCESS);
    }

  swprintf(LinkBuffer,
	   L"\\??\\%s",
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
  PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
  RTL_PROCESS_INFO ProcessInfo;
  UNICODE_STRING ImagePathString;
  UNICODE_STRING CommandLineString;
  WCHAR Description[256];
  WCHAR ImageName[256];
  WCHAR ImagePath[256];
  WCHAR CommandLine[256];
  PWSTR p1, p2;
  ULONG len;
  NTSTATUS Status;

#ifndef NDEBUG
  PrintString("ValueName '%S'  Type %lu  Length %lu\n", ValueName, ValueType, ValueLength);
  PrintString("ValueData '%S'\n", (PWSTR)ValueData);
#endif

  if (ValueType != REG_SZ)
    {
      return(STATUS_SUCCESS);
    }

  /* Extract the description */
  p1 = wcschr((PWSTR)ValueData, L' ');
  len = p1 - (PWSTR)ValueData;
  memcpy(Description,ValueData, len * sizeof(WCHAR));
  Description[len] = 0;

  /* Extract the image name */
  p1++;
  p2 = wcschr(p1, L' ');
  if (p2 != NULL)
    len = p2 - p1;
  else
    len = wcslen(p1);
  memcpy(ImageName, p1, len * sizeof(WCHAR));
  ImageName[len] = 0;

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

  PrintString("Running %S...\n", Description);
#ifndef NDEBUG
  PrintString("ImageName: '%S'\n", ImageName);
  PrintString("CommandLine: '%S'\n", CommandLine);
#endif

  /* initialize executable path */
  wcscpy(ImagePath, L"\\SystemRoot\\system32\\");
  wcscat(ImagePath, ImageName);
  wcscat(ImagePath, L".exe");

  RtlInitUnicodeString(&ImagePathString,
		       ImagePath);

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

  Status = RtlCreateUserProcess(&ImagePathString,
				OBJ_CASE_INSENSITIVE,
				ProcessParameters,
				NULL,
				NULL,
				NULL,
				FALSE,
				NULL,
				NULL,
				&ProcessInfo);
  if (!NT_SUCCESS(Status))
    {
      PrintString("Running %s failed (Status %lx)\n", Description, Status);
      return(STATUS_SUCCESS);
    }

  RtlDestroyProcessParameters(ProcessParameters);

  /* Wait for process termination */
  NtWaitForSingleObject(ProcessInfo.ProcessHandle,
			FALSE,
			NULL);

  NtClose(ProcessInfo.ThreadHandle);
  NtClose(ProcessInfo.ProcessHandle);

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

  return(Status);
}


static NTSTATUS
SmProcessFileRenameList(VOID)
{
#ifndef NDEBUG
  PrintString("SmProcessFileRenameList() called\n");
#endif

  /* FIXME: implement it! */

#ifndef NDEBUG
  PrintString("SmProcessFileRenameList() done\n");
#endif

  return(STATUS_SUCCESS);
}


static NTSTATUS STDCALL
SmKnownDllsQueryRoutine(PWSTR ValueName,
			ULONG ValueType,
			PVOID ValueData,
			ULONG ValueLength,
			PVOID Context,
			PVOID EntryContext)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  IO_STATUS_BLOCK IoStatusBlock;
  UNICODE_STRING ImageName;
  HANDLE FileHandle;
  HANDLE SectionHandle;
  NTSTATUS Status;

#ifndef NDEBUG
  PrintString("ValueName '%S'  Type %lu  Length %lu\n", ValueName, ValueType, ValueLength);
  PrintString("ValueData '%S'  Context %p  EntryContext %p\n", (PWSTR)ValueData, Context, EntryContext);
#endif

  /* Ignore the 'DllDirectory' value */
  if (!_wcsicmp(ValueName, L"DllDirectory"))
    return STATUS_SUCCESS;

  /* Open the DLL image file */
  RtlInitUnicodeString(&ImageName,
		       ValueData);
  InitializeObjectAttributes(&ObjectAttributes,
			     &ImageName,
			     OBJ_CASE_INSENSITIVE,
			     (HANDLE)Context,
			     NULL);
  Status = NtOpenFile(&FileHandle,
		      SYNCHRONIZE | FILE_EXECUTE,
		      &ObjectAttributes,
		      &IoStatusBlock,
		      FILE_SHARE_READ,
		      FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE);
  if (!NT_SUCCESS(Status))
    {
#ifndef NDEBUG
      PrintString("NtOpenFile() failed (Status %lx)\n", Status);
#endif
      return STATUS_SUCCESS;
    }

#ifndef NDEBUG
  PrintString("Opened file %wZ successfully\n", &ImageName);
#endif


  /* FIXME: Check dll image file */


  InitializeObjectAttributes(&ObjectAttributes,
			     &ImageName,
			     OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
			     (HANDLE)EntryContext,
			     NULL);
  Status = NtCreateSection(&SectionHandle,
			   SECTION_ALL_ACCESS,
			   &ObjectAttributes,
			   NULL,
			   PAGE_EXECUTE,
			   SEC_IMAGE,
			   FileHandle);
  if (NT_SUCCESS(Status))
    {
#ifndef NDEBUG
      PrintString("Created section successfully\n");
#endif
      NtClose(SectionHandle);
    }

  NtClose(FileHandle);

  return STATUS_SUCCESS;
}


static NTSTATUS
SmLoadKnownDlls(VOID)
{
  RTL_QUERY_REGISTRY_TABLE QueryTable[2];
  OBJECT_ATTRIBUTES ObjectAttributes;
  IO_STATUS_BLOCK IoStatusBlock;
  UNICODE_STRING DllDosPath;
  UNICODE_STRING DllNtPath;
  UNICODE_STRING Name;
  HANDLE ObjectDirHandle;
  HANDLE FileDirHandle;
  HANDLE SymlinkHandle;
  NTSTATUS Status;

#ifndef NDEBUG
  PrintString("SmLoadKnownDlls() called\n");
#endif

  /* Create 'KnownDlls' object directory */
  RtlInitUnicodeString(&Name,
		       L"\\KnownDlls");
  InitializeObjectAttributes(&ObjectAttributes,
			     &Name,
			     OBJ_PERMANENT | OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
			     NULL,
			     NULL);
  Status = NtCreateDirectoryObject(&ObjectDirHandle,
				   DIRECTORY_ALL_ACCESS,
				   &ObjectAttributes);
  if (!NT_SUCCESS(Status))
    {
#ifndef NDEBUG
      PrintString("NtCreateDirectoryObject() failed (Status %lx)\n", Status);
#endif
      return Status;
    }

  RtlInitUnicodeString(&DllDosPath, NULL);

  RtlZeroMemory(&QueryTable,
		sizeof(QueryTable));

  QueryTable[0].Name = L"DllDirectory";
  QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
  QueryTable[0].EntryContext = &DllDosPath;

  Status = RtlQueryRegistryValues(RTL_REGISTRY_CONTROL,
				  L"\\Session Manager\\KnownDlls",
				  QueryTable,
				  NULL,
				  SmSystemEnvironment);
  if (!NT_SUCCESS(Status))
    {
#ifndef NDEBUG
      PrintString("RtlQueryRegistryValues() failed (Status %lx)\n", Status);
#endif
      return Status;
    }

#ifndef NDEBUG
  PrintString("DllDosPath: '%wZ'\n", &DllDosPath);
#endif

  if (!RtlDosPathNameToNtPathName_U(DllDosPath.Buffer,
				    &DllNtPath,
				    NULL,
				    NULL))
    {
#ifndef NDEBUG
      PrintString("RtlDosPathNameToNtPathName_U() failed\n");
#endif
      return STATUS_OBJECT_NAME_INVALID;
    }

  PrintString("DllNtPath: '%wZ'\n", &DllNtPath);

  /* Open the dll path directory */
  InitializeObjectAttributes(&ObjectAttributes,
			     &DllNtPath,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);
  Status = NtOpenFile(&FileDirHandle,
		      SYNCHRONIZE | FILE_READ_DATA,
		      &ObjectAttributes,
		      &IoStatusBlock,
		      FILE_SHARE_READ | FILE_SHARE_WRITE,
		      FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE);
  if (!NT_SUCCESS(Status))
    {
#ifndef NDEBUG
      PrintString("NtOpenFile() failed (Status %lx)\n", Status);
#endif
      return Status;
    }

  /* Link 'KnownDllPath' the dll path directory */
  RtlInitUnicodeString(&Name,
		       L"KnownDllPath");
  InitializeObjectAttributes(&ObjectAttributes,
			     &Name,
			     OBJ_PERMANENT | OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
			     ObjectDirHandle,
			     NULL);
  Status = NtCreateSymbolicLinkObject(&SymlinkHandle,
				      SYMBOLIC_LINK_ALL_ACCESS,
				      &ObjectAttributes,
				      &DllDosPath);
  if (!NT_SUCCESS(Status))
    {
#ifndef NDEBUG
      PrintString("NtCreateSymbolicLink() failed (Status %lx)\n", Status);
#endif
      return Status;
    }

  NtClose(SymlinkHandle);

  RtlZeroMemory(&QueryTable,
		sizeof(QueryTable));

  QueryTable[0].QueryRoutine = SmKnownDllsQueryRoutine;
  QueryTable[0].EntryContext = ObjectDirHandle;

  Status = RtlQueryRegistryValues(RTL_REGISTRY_CONTROL,
				  L"\\Session Manager\\KnownDlls",
				  QueryTable,
				  (PVOID)FileDirHandle,
				  NULL);
  if (!NT_SUCCESS(Status))
    {
#ifndef NDEBUG
      PrintString("RtlQueryRegistryValues() failed (Status %lx)\n", Status);
#endif
    }

#ifndef NDEBUG
  PrintString("SmLoadKnownDlls() done\n");
#endif

  return Status;
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
  LPWSTR p;

#ifndef NDEBUG
  PrintString("ValueName '%S'  Type %lu  Length %lu\n", ValueName, ValueType, ValueLength);
  PrintString("ValueData '%S'\n", (PWSTR)ValueData);
#endif

  if (ValueType != REG_SZ)
    {
      return(STATUS_SUCCESS);
    }

  /*
   * Format: "<path>[ <initial_size>[ <maximum_size>]]"
   */
  if ((p = wcschr(ValueData, ' ')) != NULL)
    {
      *p = L'\0';
      InitialSize.QuadPart = wcstoul(p + 1, &p, 0) * 256 * 4096;
      if (*p == ' ')
	{
	  MaximumSize.QuadPart = wcstoul(p + 1, NULL, 0) * 256 * 4096;
	}
      else
	MaximumSize = InitialSize;
    }
  else
    {
      InitialSize.QuadPart = 50 * 4096;
      MaximumSize.QuadPart = 80 * 4096;
    }

  if (!RtlDosPathNameToNtPathName_U ((LPWSTR)ValueData,
				     &FileName,
				     NULL,
				     NULL))
    {
      return (STATUS_SUCCESS);
    }

  DbgPrint("SMSS: Created paging file %wZ with size %dKB\n",
	   &FileName, InitialSize.QuadPart / 1024);
  Status = NtCreatePagingFile(&FileName,
			      &InitialSize,
			      &MaximumSize,
			      0);

  RtlFreeUnicodeString(&FileName);

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


static NTSTATUS STDCALL
SmEnvironmentQueryRoutine(PWSTR ValueName,
			  ULONG ValueType,
			  PVOID ValueData,
			  ULONG ValueLength,
			  PVOID Context,
			  PVOID EntryContext)
{
  UNICODE_STRING EnvVariable;
  UNICODE_STRING EnvValue;

#ifndef NDEBUG
  PrintString("ValueName '%S'  Type %lu  Length %lu\n", ValueName, ValueType, ValueLength);
  PrintString("ValueData '%S'\n", (PWSTR)ValueData);
#endif

  if (ValueType != REG_SZ)
    {
      return(STATUS_SUCCESS);
    }

  RtlInitUnicodeString(&EnvVariable,
		       ValueName);
  RtlInitUnicodeString(&EnvValue,
		       (PWSTR)ValueData);
  RtlSetEnvironmentVariable(Context,
			    &EnvVariable,
			    &EnvValue);

  return(STATUS_SUCCESS);
}


static NTSTATUS
SmSetEnvironmentVariables(VOID)
{
  RTL_QUERY_REGISTRY_TABLE QueryTable[2];
  UNICODE_STRING EnvVariable;
  UNICODE_STRING EnvValue;
  WCHAR ValueBuffer[MAX_PATH];
  NTSTATUS Status;

  /*
   * The following environment variables must be set prior to reading
   * other variables from the registry.
   *
   * Variables (example):
   *    SystemRoot = "C:\reactos"
   *    SystemDrive = "C:"
   */

  /* Copy system root into value buffer */
  wcscpy(ValueBuffer,
	 SharedUserData->NtSystemRoot);

  /* Set SystemRoot = "C:\reactos" */
  RtlInitUnicodeStringFromLiteral(&EnvVariable,
		       L"SystemRoot");
  RtlInitUnicodeString(&EnvValue,
		       ValueBuffer);
  RtlSetEnvironmentVariable(&SmSystemEnvironment,
			    &EnvVariable,
			    &EnvValue);

  /* Cut off trailing path */
  ValueBuffer[2] = 0;

  /* Set SystemDrive = "C:" */
  RtlInitUnicodeStringFromLiteral(&EnvVariable,
		       L"SystemDrive");
  RtlInitUnicodeString(&EnvValue,
		       ValueBuffer);
  RtlSetEnvironmentVariable(&SmSystemEnvironment,
			    &EnvVariable,
			    &EnvValue);

  /* Read system environment from the registry. */
  RtlZeroMemory(&QueryTable,
		sizeof(QueryTable));

  QueryTable[0].QueryRoutine = SmEnvironmentQueryRoutine;

  Status = RtlQueryRegistryValues(RTL_REGISTRY_CONTROL,
				  L"\\Session Manager\\Environment",
				  QueryTable,
				  &SmSystemEnvironment,
				  SmSystemEnvironment);

  return(Status);
}


static NTSTATUS
SmLoadSubsystems(VOID)
{
  SYSTEM_LOAD_AND_CALL_IMAGE ImageInfo;
  NTSTATUS Status;

  /* Load kernel mode subsystem (aka win32k.sys) */
  RtlInitUnicodeStringFromLiteral(&ImageInfo.ModuleName,
		       L"\\SystemRoot\\system32\\win32k.sys");

  Status = NtSetSystemInformation(SystemLoadAndCallImage,
				  &ImageInfo,
				  sizeof(SYSTEM_LOAD_AND_CALL_IMAGE));

  PrintString("SMSS: Loaded win32k.sys (Status %lx)\n", Status);
#if 0
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }
#endif

  /* FIXME: load more subsystems (csrss!) */

  return(Status);
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
      PrintString("SM: Failed to create object directories (Status %lx)\n", Status);
      return(Status);
    }

  /* Create the SmApiPort object (LPC) */
  Status = SmCreateApiPort();
  if (!NT_SUCCESS(Status))
    {
      PrintString("SM: Failed to create SmApiPort (Status %lx)\n", Status);
      return(Status);
    }

  /* Create the system environment */
  Status = RtlCreateEnvironment(FALSE,
				&SmSystemEnvironment);
  if (!NT_SUCCESS(Status))
    {
      PrintString("SM: Failed to create the system environment (Status %lx)\n", Status);
      return(Status);
    }

  /* Set environment variables */
  Status = SmSetEnvironmentVariables();
  if (!NT_SUCCESS(Status))
    {
      PrintString("SM: Failed to set system environment variables (Status %lx)\n", Status);
      return(Status);
    }

  /* Define symbolic links to kernel devices (MS-DOS names) */
  Status = SmInitDosDevices();
  if (!NT_SUCCESS(Status))
    {
      PrintString("SM: Failed to create dos device links (Status %lx)\n", Status);
      return(Status);
    }

  /* Run all programs in the boot execution list */
  Status = SmRunBootApps();
  if (!NT_SUCCESS(Status))
    {
      PrintString("SM: Failed to run boot applications (Status %lx)\n", Status);
      return(Status);
    }

  /* Process the file rename list */
  Status = SmProcessFileRenameList();
  if (!NT_SUCCESS(Status))
    {
      PrintString("SM: Failed to process the file rename list (Status %lx)\n", Status);
      return(Status);
    }

  /* Load the well known DLLs */
  Status = SmLoadKnownDlls();
  if (!NT_SUCCESS(Status))
    {
      PrintString("SM: Failed to preload system DLLs (Status %lx)\n", Status);
      /* Don't crash ReactOS if DLLs cannot be loaded */
    }

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
#if 0
  Status = SmUpdateEnvironment();
  if (!NT_SUCCESS(Status))
    {
      PrintString("SM: Failed to update environment variables (Status %lx)\n", Status);
      return(Status);
    }
#endif

  /* Load the subsystems */
  Status = SmLoadSubsystems();
  if (!NT_SUCCESS(Status))
    {
      PrintString("SM: Failed to load subsystems (Status %lx)\n", Status);
      return(Status);
    }

  /* Run csrss.exe */
  RtlInitUnicodeStringFromLiteral(&UnicodeString,
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
  RtlInitUnicodeStringFromLiteral(&UnicodeString,
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
  RtlInitUnicodeStringFromLiteral(&UnicodeString,
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
