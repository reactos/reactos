/* $Id: database.c,v 1.2 2002/06/12 23:33:15 ekohl Exp $
 *
 * service control manager
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
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.LIB. If not, write
 * to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 * MA 02139, USA.
 *
 */

/* INCLUDES *****************************************************************/

#define NTOS_MODE_USER
#include <ntos.h>

#include <windows.h>
#include <tchar.h>

#include "services.h"

#define NDEBUG
#include <debug.h>


/* TYPES *********************************************************************/

typedef struct _SERVICE_GROUP
{
  LIST_ENTRY GroupListEntry;
  UNICODE_STRING GroupName;

  BOOLEAN ServicesRunning;

} SERVICE_GROUP, *PSERVICE_GROUP;


typedef struct _SERVICE
{
  LIST_ENTRY ServiceListEntry;
  UNICODE_STRING ServiceName;
  UNICODE_STRING RegistryPath;
  UNICODE_STRING ServiceGroup;

  ULONG Start;
  ULONG Type;
  ULONG ErrorControl;
  ULONG Tag;

  BOOLEAN ServiceRunning;

} SERVICE, *PSERVICE;


/* GLOBALS *******************************************************************/

LIST_ENTRY GroupListHead = {NULL, NULL};
LIST_ENTRY ServiceListHead  = {NULL, NULL};


/* FUNCTIONS *****************************************************************/

static NTSTATUS STDCALL
CreateGroupListRoutine(PWSTR ValueName,
		       ULONG ValueType,
		       PVOID ValueData,
		       ULONG ValueLength,
		       PVOID Context,
		       PVOID EntryContext)
{
  PSERVICE_GROUP Group;

  if (ValueType == REG_SZ)
    {
//      PrintString("Data: '%S'\n", (PWCHAR)ValueData);

      Group = (PSERVICE_GROUP)HeapAlloc(GetProcessHeap(),
					HEAP_ZERO_MEMORY,
					sizeof(SERVICE_GROUP));
      if (Group == NULL)
	{
	  return(STATUS_INSUFFICIENT_RESOURCES);
	}

      if (!RtlCreateUnicodeString(&Group->GroupName,
				  (PWSTR)ValueData))
	{
	  return(STATUS_INSUFFICIENT_RESOURCES);
	}


      InsertTailList(&GroupListHead,
		     &Group->GroupListEntry);
    }

  return(STATUS_SUCCESS);
}


static NTSTATUS STDCALL
CreateServiceListEntry(PUNICODE_STRING ServiceName)
{
  RTL_QUERY_REGISTRY_TABLE QueryTable[6];
  PSERVICE Service = NULL;
  NTSTATUS Status;

//  PrintString("Service: '%wZ'\n", ServiceName);

  /* Allocate service entry */
  Service = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
		      sizeof(SERVICE));
  if (Service == NULL)
    {
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  /* Copy service name */
  Service->ServiceName.Length = ServiceName->Length;
  Service->ServiceName.MaximumLength = ServiceName->Length + sizeof(WCHAR);
  Service->ServiceName.Buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
					  Service->ServiceName.MaximumLength);
  if (Service->ServiceName.Buffer == NULL)
    {
      HeapFree(GetProcessHeap(), 0, Service);
      return(STATUS_INSUFFICIENT_RESOURCES);
    }
  RtlCopyMemory(Service->ServiceName.Buffer,
		ServiceName->Buffer,
		ServiceName->Length);
  Service->ServiceName.Buffer[ServiceName->Length / sizeof(WCHAR)] = 0;

  /* Build registry path */
  Service->RegistryPath.MaximumLength = MAX_PATH * sizeof(WCHAR);
  Service->RegistryPath.Buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
					   MAX_PATH * sizeof(WCHAR));
  if (Service->ServiceName.Buffer == NULL)
    {
      HeapFree(GetProcessHeap(), 0, Service->ServiceName.Buffer);
      HeapFree(GetProcessHeap(), 0, Service);
      return(STATUS_INSUFFICIENT_RESOURCES);
    }
  wcscpy(Service->RegistryPath.Buffer,
	 L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\");
  wcscat(Service->RegistryPath.Buffer,
	 Service->ServiceName.Buffer);
  Service->RegistryPath.Length = wcslen(Service->RegistryPath.Buffer) * sizeof(WCHAR);

  /* Get service data */
  RtlZeroMemory(&QueryTable,
		sizeof(QueryTable));

  QueryTable[0].Name = L"Start";
  QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
  QueryTable[0].EntryContext = &Service->Start;

  QueryTable[1].Name = L"Type";
  QueryTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
  QueryTable[1].EntryContext = &Service->Type;

  QueryTable[2].Name = L"ErrorControl";
  QueryTable[2].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
  QueryTable[2].EntryContext = &Service->ErrorControl;

  QueryTable[3].Name = L"Group";
  QueryTable[3].Flags = RTL_QUERY_REGISTRY_DIRECT;
  QueryTable[3].EntryContext = &Service->ServiceGroup;

  Status = RtlQueryRegistryValues(RTL_REGISTRY_SERVICES,
				  ServiceName->Buffer,
				  QueryTable,
				  NULL,
				  NULL);
  if (!NT_SUCCESS(Status))
    {
      PrintString("RtlQueryRegistryValues() failed (Status %lx)\n", Status);
      RtlFreeUnicodeString(&Service->RegistryPath);
      RtlFreeUnicodeString(&Service->ServiceName);
      HeapFree(GetProcessHeap(), 0, Service);
      return(Status);
    }

#if 0
  PrintString("ServiceName: '%wZ'\n", &Service->ServiceName);
  PrintString("RegistryPath: '%wZ'\n", &Service->RegistryPath);
  PrintString("ServiceGroup: '%wZ'\n", &Service->ServiceGroup);
  PrintString("Start %lx  Type %lx  ErrorControl %lx\n",
	      Service->Start, Service->Type, Service->ErrorControl);
#endif

  /* Append service entry */
  InsertTailList(&ServiceListHead,
		 &Service->ServiceListEntry);

  return(STATUS_SUCCESS);
}


NTSTATUS
ScmCreateServiceDataBase(VOID)
{
  RTL_QUERY_REGISTRY_TABLE QueryTable[2];
  WCHAR NameBuffer[MAX_PATH];
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING ServicesKeyName;
  UNICODE_STRING SubKeyName;
  HKEY ServicesKey;
  ULONG Index;
  NTSTATUS Status;

  PKEY_BASIC_INFORMATION KeyInfo = NULL;
  ULONG KeyInfoLength = 0;
  ULONG ReturnedLength;

//  PrintString("ScmCreateServiceDataBase() called\n");

  /* Initialize basic variables */
  InitializeListHead(&GroupListHead);
  InitializeListHead(&ServiceListHead);


  /* Build group order list */
  RtlZeroMemory(&QueryTable,
		sizeof(QueryTable));

  QueryTable[0].Name = L"List";
  QueryTable[0].QueryRoutine = CreateGroupListRoutine;

  Status = RtlQueryRegistryValues(RTL_REGISTRY_CONTROL,
				  L"ServiceGroupOrder",
				  QueryTable,
				  NULL,
				  NULL);
  if (!NT_SUCCESS(Status))
    return(Status);

  RtlInitUnicodeString(&ServicesKeyName,
		       L"\\Registry\\Machine\\System\\CurrentControlSet\\Services");

  InitializeObjectAttributes(&ObjectAttributes,
			     &ServicesKeyName,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);

  Status = RtlpNtOpenKey(&ServicesKey,
			 0x10001,
			 &ObjectAttributes,
			 0);
  if (!NT_SUCCESS(Status))
    return(Status);

  /* Allocate key info buffer */
  KeyInfoLength = sizeof(KEY_BASIC_INFORMATION) + MAX_PATH * sizeof(WCHAR);
  KeyInfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, KeyInfoLength);
  if (KeyInfo == NULL)
    {
      NtClose(ServicesKey);
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  Index = 0;
  while (TRUE)
    {
      Status = NtEnumerateKey(ServicesKey,
			      Index,
			      KeyBasicInformation,
			      KeyInfo,
			      KeyInfoLength,
			      &ReturnedLength);
      if (NT_SUCCESS(Status))
	{
	  if (KeyInfo->NameLength < MAX_PATH * sizeof(WCHAR))
	    {

	      SubKeyName.Length = KeyInfo->NameLength;
	      SubKeyName.MaximumLength = KeyInfo->NameLength + sizeof(WCHAR);
	      SubKeyName.Buffer = KeyInfo->Name;
	      SubKeyName.Buffer[SubKeyName.Length / sizeof(WCHAR)] = 0;

//	      PrintString("KeyName: '%wZ'\n", &SubKeyName);
	      Status = CreateServiceListEntry(&SubKeyName);
	    }
	}

      if (!NT_SUCCESS(Status))
	break;

      Index++;
    }

  HeapFree(GetProcessHeap(), 0, KeyInfo);
  NtClose(ServicesKey);

//  PrintString("ScmCreateServiceDataBase() done\n");

  return(STATUS_SUCCESS);
}


VOID
ScmGetBootAndSystemDriverState(VOID)
{

}


static NTSTATUS
ScmStartService(PSERVICE Service)
{
#if 0
  PROCESS_INFORMATION ProcessInformation;
  STARTUPINFO StartupInfo;
  WCHAR CommandLine[MAX_PATH];
  BOOL Result;

  PrintString("ScmStartService() called\n");

  GetSystemDirectoryW(CommandLine, MAX_PATH);
  _tcscat(CommandLine, "\\");
  _tcscat(CommandLine, FileName);

  PrintString("SCM: %s\n", CommandLine);

  /* FIXME: create '\\.\pipe\net\NtControlPipe' instance */

  StartupInfo.cb = sizeof(StartupInfo);
  StartupInfo.lpReserved = NULL;
  StartupInfo.lpDesktop = NULL;
  StartupInfo.lpTitle = NULL;
  StartupInfo.dwFlags = 0;
  StartupInfo.cbReserved2 = 0;
  StartupInfo.lpReserved2 = 0;

  Result = CreateProcessW(CommandLine,
			  NULL,
			  NULL,
			  NULL,
			  FALSE,
			  DETACHED_PROCESS,
			  NULL,
			  NULL,
			  &StartupInfo,
			  &ProcessInformation);
  if (!Result)
    {
      /* FIXME: close control pipe */

      PrintString("SCM: Failed to start '%s'\n", FileName);
      return(STATUS_UNSUCCESSFUL);
    }

  /* FIXME: connect control pipe */
#endif

  return(STATUS_SUCCESS);
}


VOID
ScmAutoStartServices(VOID)
{
  PLIST_ENTRY GroupEntry;
  PLIST_ENTRY ServiceEntry;
  PSERVICE_GROUP CurrentGroup;
  PSERVICE CurrentService;
  NTSTATUS Status;

  GroupEntry = GroupListHead.Flink;
  while (GroupEntry != &GroupListHead)
    {
      CurrentGroup = CONTAINING_RECORD(GroupEntry, SERVICE_GROUP, GroupListEntry);

//      PrintString("Group '%wZ'\n", &CurrentGroup->GroupName);

      ServiceEntry = ServiceListHead.Flink;
      while (ServiceEntry != &ServiceListHead)
	{
	  CurrentService = CONTAINING_RECORD(ServiceEntry, SERVICE, ServiceListEntry);

	  if ((RtlCompareUnicodeString(&CurrentGroup->GroupName, &CurrentService->ServiceGroup, TRUE) == 0) &&
	      (CurrentService->Start == SERVICE_AUTO_START))
	    {
	      if (CurrentService->Type == SERVICE_KERNEL_DRIVER ||
		  CurrentService->Type == SERVICE_FILE_SYSTEM_DRIVER ||
		  CurrentService->Type == SERVICE_RECOGNIZER_DRIVER)
		{
		  /* Load driver */
//		  PrintString("  Path: %wZ\n", &CurrentService->RegistryPath);
		  Status = NtLoadDriver(&CurrentService->RegistryPath);
		}
	      else
		{
		  /* Start service */
		  Status = ScmStartService(CurrentService);
		}

	      if (NT_SUCCESS(Status))
		{
		  CurrentGroup->ServicesRunning = TRUE;
		  CurrentService->ServiceRunning = TRUE;
		}
#if 0
	      else
		{
		  if (CurrentService->ErrorControl == 1)
		    {
		      /* Log error */

		    }
		  else if (CurrentService->ErrorControl == 2)
		    {
		      if (IsLastKnownGood == FALSE)
			{
			  /* Boot last known good configuration */

			}
		    }
		  else if (CurrentService->ErrorControl == 3)
		    {
		      if (IsLastKnownGood == FALSE)
			{
			  /* Boot last known good configuration */

			}
		      else
			{
			  /* BSOD! */

			}
		    }
		}
#endif

	    }
	  ServiceEntry = ServiceEntry->Flink;
	}
      GroupEntry = GroupEntry->Flink;
    }
}

/* EOF */
