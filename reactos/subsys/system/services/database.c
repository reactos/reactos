/* $Id: database.c,v 1.1 2002/06/07 20:09:56 ekohl Exp $
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
  PWSTR GroupName;

  BOOLEAN ServicesRunning;

} SERVICE_GROUP, *PSERVICE_GROUP;


typedef struct _SERVICE
{
  LIST_ENTRY ServiceListEntry;
  PWSTR ServiceName;
  PWSTR GroupName;

  PWSTR ImagePath;

  ULONG Start;
  ULONG Type;
  ULONG ErrorControl;
  ULONG Tag;

  BOOLEAN ServiceRunning;	// needed ??

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
	return(STATUS_INSUFFICIENT_RESOURCES);


      Group->GroupName = (PWSTR)HeapAlloc(GetProcessHeap(),
					  HEAP_ZERO_MEMORY,
					  ValueLength);
      if (Group->GroupName == NULL)
	return(STATUS_INSUFFICIENT_RESOURCES);

      wcscpy(Group->GroupName,
	     (PWSTR)ValueData);


      InsertTailList(&GroupListHead,
		     &Group->GroupListEntry);


    }

  return(STATUS_SUCCESS);
}


static NTSTATUS STDCALL
CreateServiceListEntry(PUNICODE_STRING ServiceName)
{
  RTL_QUERY_REGISTRY_TABLE QueryTable[6];
  WCHAR ServiceGroupBuffer[MAX_PATH];
  WCHAR ImagePathBuffer[MAX_PATH];
  UNICODE_STRING ServiceGroup;
  UNICODE_STRING ImagePath;
  PSERVICE_GROUP Group;
  PSERVICE Service;
  NTSTATUS Status;

//  PrintString("Service: '%wZ'\n", ServiceName);

  Service = (PSERVICE)HeapAlloc(GetProcessHeap(),
				HEAP_ZERO_MEMORY,
				sizeof(SERVICE));
  if (Service == NULL)
    {
      PrintString(" - HeapAlloc() (1) failed\n");
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  Service->ServiceName = (PWSTR)HeapAlloc(GetProcessHeap(),
					  HEAP_ZERO_MEMORY,
					  ServiceName->Length);
  if (Service->ServiceName == NULL)
    {
      PrintString(" - HeapAlloc() (2) failed\n");
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  wcscpy(Service->ServiceName,
	 ServiceName->Buffer);


  ServiceGroup.Length = 0;
  ServiceGroup.MaximumLength = MAX_PATH * sizeof(WCHAR);
  ServiceGroup.Buffer = ServiceGroupBuffer;
  RtlZeroMemory(ServiceGroupBuffer,
		MAX_PATH * sizeof(WCHAR));

  ImagePath.Length = 0;
  ImagePath.MaximumLength = MAX_PATH * sizeof(WCHAR);
  ImagePath.Buffer = ImagePathBuffer;
  RtlZeroMemory(ImagePathBuffer,
		MAX_PATH * sizeof(WCHAR));


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
  QueryTable[3].EntryContext = &ServiceGroup;

  QueryTable[4].Name = L"ImagePath";
  QueryTable[4].Flags = RTL_QUERY_REGISTRY_DIRECT;
  QueryTable[4].EntryContext = &ImagePath;


  Status = RtlQueryRegistryValues(RTL_REGISTRY_SERVICES,
				  ServiceName->Buffer,
				  QueryTable,
				  NULL,
				  NULL);
  if (!NT_SUCCESS(Status))
    {
      PrintString("RtlQueryRegistryValues() failed (Status %lx)\n", Status);
      return(Status);
    }

  /* Copy the service group name */
  if (ServiceGroup.Length > 0)
    {
      Service->GroupName = (PWSTR)HeapAlloc(GetProcessHeap(),
					    HEAP_ZERO_MEMORY,
					    ServiceGroup.Length + sizeof(WCHAR));
      if (Service->GroupName == NULL)
	{
	  PrintString(" - HeapAlloc() (3) failed\n");
	  return(STATUS_INSUFFICIENT_RESOURCES);
	}

      memcpy(Service->GroupName,
	     ServiceGroup.Buffer,
	     ServiceGroup.Length);
    }
  else
    {
      Service->GroupName = NULL;
    }

  /* Copy the image path */
  if (ImagePath.Length > 0)
    {
      Service->ImagePath = (PWSTR)HeapAlloc(GetProcessHeap(),
					    HEAP_ZERO_MEMORY,
					    ImagePath.Length + sizeof(WCHAR));
      if (Service->ImagePath == NULL)
	{
	  PrintString(" - HeapAlloc() (4) failed\n");
	  return(STATUS_INSUFFICIENT_RESOURCES);
	}

      memcpy(Service->ImagePath,
	     ImagePath.Buffer,
	     ImagePath.Length);
    }
  else
    {
      Service->ImagePath = NULL;
    }

//  PrintString("  Type: %lx\n", Service->Type);
//  PrintString("  Start: %lx\n", Service->Start);
//  PrintString("  Group: '%wZ'\n", &ServiceGroup);


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
  NTSTATUS Status;
  ULONG Index;

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

  SubKeyName.Length = 0;
  SubKeyName.MaximumLength = MAX_PATH * sizeof(WCHAR);
  SubKeyName.Buffer = NameBuffer;

  Index = 0;
  while (TRUE)
    {
      Status = RtlpNtEnumerateSubKey(ServicesKey,
				     &SubKeyName,
				     Index,
				     0);
      if (!NT_SUCCESS(Status))
	break;

      CreateServiceListEntry(&SubKeyName);

      Index++;
    }

//  PrintString("ScmCreateServiceDataBase() done\n");

  return(STATUS_SUCCESS);
}


VOID
ScmGetBootAndSystemDriverState(VOID)
{

}


static NTSTATUS
ScmLoadDriver(PSERVICE Service)
{
  WCHAR ServicePath[MAX_PATH];
  UNICODE_STRING DriverPath;

//  PrintString("ScmLoadDriver(%S) called\n", Service->ServiceName);

  if (Service->ImagePath == NULL)
    {
      wcscpy(ServicePath, L"\\SystemRoot\\system32\\drivers\\");
      wcscat(ServicePath, Service->ServiceName);
      wcscat(ServicePath, L".sys");
    }
  else
    {
      wcscpy(ServicePath, L"\\SystemRoot\\");
      wcscat(ServicePath, Service->ImagePath);
    }

  RtlInitUnicodeString(&DriverPath, ServicePath);

//  PrintString("  DriverPath: '%wZ'\n", &DriverPath);

  return(NtLoadDriver(&DriverPath));
}


static NTSTATUS
ScmStartService(PSERVICE Service)
{
#if 0
  PROCESS_INFORMATION ProcessInformation;
  STARTUPINFO StartupInfo;
  WCHAR CommandLine[MAX_PATH];
  BOOL Result;
#endif

  PrintString("ScmStartService(%S) called\n", Service->ServiceName);

#if 0
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

//      PrintString("  %S\n", CurrentGroup->GroupName);

      ServiceEntry = ServiceListHead.Flink;
      while (ServiceEntry != &ServiceListHead)
	{
	  CurrentService = CONTAINING_RECORD(ServiceEntry, SERVICE, ServiceListEntry);

	  if ((wcsicmp(CurrentGroup->GroupName, CurrentService->GroupName) == 0) &&
	      (CurrentService->Start == SERVICE_AUTO_START))
	    {
	      if (CurrentService->Type == SERVICE_KERNEL_DRIVER ||
		  CurrentService->Type == SERVICE_FILE_SYSTEM_DRIVER ||
		  CurrentService->Type == SERVICE_RECOGNIZER_DRIVER)
		{
		  /* Load driver */
		  Status = ScmLoadDriver(CurrentService);
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