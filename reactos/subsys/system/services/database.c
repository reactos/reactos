/* $Id$
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
#include <rosrtl/string.h>

#include <windows.h>
#include <tchar.h>

#include <services/services.h>
#include "services.h"

#define NDEBUG
#include <debug.h>


/* TYPES *********************************************************************/

typedef struct _SERVICE_GROUP
{
  LIST_ENTRY GroupListEntry;
  UNICODE_STRING GroupName;

  BOOLEAN ServicesRunning;
  ULONG TagCount;
  PULONG TagArray;

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
  BOOLEAN ServiceVisited;

  HANDLE ControlPipeHandle;
  ULONG ProcessId;
  ULONG ThreadId;
} SERVICE, *PSERVICE;


/* GLOBALS *******************************************************************/

LIST_ENTRY GroupListHead;
LIST_ENTRY ServiceListHead;


/* FUNCTIONS *****************************************************************/

static NTSTATUS STDCALL
CreateGroupOrderListRoutine(PWSTR ValueName,
			    ULONG ValueType,
			    PVOID ValueData,
			    ULONG ValueLength,
			    PVOID Context,
			    PVOID EntryContext)
{
  PSERVICE_GROUP Group;

  DPRINT("IopGetGroupOrderList(%S, %x, %x, %x, %x, %x)\n",
         ValueName, ValueType, ValueData, ValueLength, Context, EntryContext);

  if (ValueType == REG_BINARY &&
      ValueData != NULL &&
      ValueLength >= sizeof(DWORD) &&
      ValueLength >= (*(PULONG)ValueData + 1) * sizeof(DWORD))
    {
      Group = (PSERVICE_GROUP)Context;
      Group->TagCount = ((PULONG)ValueData)[0];
      if (Group->TagCount > 0)
        {
	  if (ValueLength >= (Group->TagCount + 1) * sizeof(DWORD))
            {
              Group->TagArray = (PULONG)HeapAlloc(GetProcessHeap(),
					          HEAP_ZERO_MEMORY,
					          Group->TagCount * sizeof(DWORD));
	      if (Group->TagArray == NULL)
	        {
		  Group->TagCount = 0;
	          return STATUS_INSUFFICIENT_RESOURCES;
		}
	      RtlCopyMemory(Group->TagArray, 
		            (PULONG)ValueData + 1, 
			    Group->TagCount * sizeof(DWORD));
	    }
	  else
	    {
	      Group->TagCount = 0;
	      return STATUS_UNSUCCESSFUL;
	    }
	}
    }
  return STATUS_SUCCESS;
}


static NTSTATUS STDCALL
CreateGroupListRoutine(PWSTR ValueName,
		       ULONG ValueType,
		       PVOID ValueData,
		       ULONG ValueLength,
		       PVOID Context,
		       PVOID EntryContext)
{
  PSERVICE_GROUP Group;
  RTL_QUERY_REGISTRY_TABLE QueryTable[2];
  NTSTATUS Status;

  if (ValueType == REG_SZ)
    {
      DPRINT("Data: '%S'\n", (PWCHAR)ValueData);

      Group = (PSERVICE_GROUP)HeapAlloc(GetProcessHeap(),
					HEAP_ZERO_MEMORY,
					sizeof(SERVICE_GROUP));
      if (Group == NULL)
	{
	  return STATUS_INSUFFICIENT_RESOURCES;
	}

      if (!RtlCreateUnicodeString(&Group->GroupName,
				  (PWSTR)ValueData))
	{
	  return STATUS_INSUFFICIENT_RESOURCES;
	}

      RtlZeroMemory(&QueryTable, sizeof(QueryTable));
      QueryTable[0].Name = (PWSTR)ValueData;
      QueryTable[0].QueryRoutine = CreateGroupOrderListRoutine;

      Status = RtlQueryRegistryValues(RTL_REGISTRY_CONTROL,
				      L"GroupOrderList",
				      QueryTable,
				      (PVOID)Group,
				      NULL);
      DPRINT("%x %d %S\n", Status, Group->TagCount, (PWSTR)ValueData);


      InsertTailList(&GroupListHead,
		     &Group->GroupListEntry);
    }

  return STATUS_SUCCESS;
}


static NTSTATUS STDCALL
CreateServiceListEntry(PUNICODE_STRING ServiceName)
{
  RTL_QUERY_REGISTRY_TABLE QueryTable[6];
  PSERVICE Service = NULL;
  NTSTATUS Status;

  DPRINT("Service: '%wZ'\n", ServiceName);

  /* Allocate service entry */
  Service = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
		      sizeof(SERVICE));
  if (Service == NULL)
    {
      return STATUS_INSUFFICIENT_RESOURCES;
    }

  /* Copy service name */
  Service->ServiceName.Length = ServiceName->Length;
  Service->ServiceName.MaximumLength = ServiceName->Length + sizeof(WCHAR);
  Service->ServiceName.Buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
					  Service->ServiceName.MaximumLength);
  if (Service->ServiceName.Buffer == NULL)
    {
      HeapFree(GetProcessHeap(), 0, Service);
      return STATUS_INSUFFICIENT_RESOURCES;
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
      return STATUS_INSUFFICIENT_RESOURCES;
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

  QueryTable[4].Name = L"Tag";
  QueryTable[4].Flags = RTL_QUERY_REGISTRY_DIRECT;
  QueryTable[4].EntryContext = &Service->Tag;

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
      return Status;
    }

  DPRINT("ServiceName: '%wZ'\n", &Service->ServiceName);
  DPRINT("RegistryPath: '%wZ'\n", &Service->RegistryPath);
  DPRINT("ServiceGroup: '%wZ'\n", &Service->ServiceGroup);
  DPRINT("Start %lx  Type %lx  Tag %lx  ErrorControl %lx\n",
	 Service->Start, Service->Type, Service->Tag, Service->ErrorControl);

  /* Append service entry */
  InsertTailList(&ServiceListHead,
		 &Service->ServiceListEntry);

  return STATUS_SUCCESS;
}


NTSTATUS
ScmCreateServiceDataBase(VOID)
{
  RTL_QUERY_REGISTRY_TABLE QueryTable[2];
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING ServicesKeyName;
  UNICODE_STRING SubKeyName;
  HKEY ServicesKey;
  ULONG Index;
  NTSTATUS Status;

  PKEY_BASIC_INFORMATION KeyInfo = NULL;
  ULONG KeyInfoLength = 0;
  ULONG ReturnedLength;

  DPRINT("ScmCreateServiceDataBase() called\n");

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
    return Status;

  RtlRosInitUnicodeStringFromLiteral(&ServicesKeyName,
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
    return Status;

  /* Allocate key info buffer */
  KeyInfoLength = sizeof(KEY_BASIC_INFORMATION) + MAX_PATH * sizeof(WCHAR);
  KeyInfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, KeyInfoLength);
  if (KeyInfo == NULL)
    {
      NtClose(ServicesKey);
      return STATUS_INSUFFICIENT_RESOURCES;
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

	      DPRINT("KeyName: '%wZ'\n", &SubKeyName);
	      Status = CreateServiceListEntry(&SubKeyName);

	      /* Ignore services without proper registry. */
	      if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
	        {
	          Status = STATUS_SUCCESS;
	        }
	    }
	}

      if (!NT_SUCCESS(Status))
	break;

      Index++;
    }

  HeapFree(GetProcessHeap(), 0, KeyInfo);
  NtClose(ServicesKey);

  DPRINT("ScmCreateServiceDataBase() done\n");

  return STATUS_SUCCESS;
}


static NTSTATUS
ScmCheckDriver(PSERVICE Service)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING DirName;
  HANDLE DirHandle;
  NTSTATUS Status;
  POBJECT_DIRECTORY_INFORMATION DirInfo;
  ULONG BufferLength;
  ULONG DataLength;
  ULONG Index;
  PLIST_ENTRY GroupEntry;
  PSERVICE_GROUP CurrentGroup;

  DPRINT("ScmCheckDriver(%wZ) called\n", &Service->ServiceName);

  if (Service->Type == SERVICE_KERNEL_DRIVER)
    {
      RtlRosInitUnicodeStringFromLiteral(&DirName,
			   L"\\Driver");
    }
  else
    {
      RtlRosInitUnicodeStringFromLiteral(&DirName,
			   L"\\FileSystem");
    }

  InitializeObjectAttributes(&ObjectAttributes,
			     &DirName,
			     0,
			     NULL,
			     NULL);

  Status = NtOpenDirectoryObject(&DirHandle,
				 DIRECTORY_QUERY | DIRECTORY_TRAVERSE,
				 &ObjectAttributes);
  if (!NT_SUCCESS(Status))
    {
      return Status;
    }

  BufferLength = sizeof(OBJECT_DIRECTORY_INFORMATION) +
		 2 * MAX_PATH * sizeof(WCHAR);
  DirInfo = HeapAlloc(GetProcessHeap(),
		      HEAP_ZERO_MEMORY,
		      BufferLength);

  Index = 0;
  while (TRUE)
    {
      Status = NtQueryDirectoryObject(DirHandle,
				      DirInfo,
				      BufferLength,
				      TRUE,
				      FALSE,
				      &Index,
				      &DataLength);
      if (Status == STATUS_NO_MORE_ENTRIES)
	{
	  /* FIXME: Add current service to 'failed service' list */
	  DPRINT("Service '%wZ' failed\n", &Service->ServiceName);
	  break;
	}

      if (!NT_SUCCESS(Status))
	break;

      DPRINT("Comparing: '%wZ'  '%wZ'\n", &Service->ServiceName, &DirInfo->ObjectName);

      if (RtlEqualUnicodeString(&Service->ServiceName, &DirInfo->ObjectName, TRUE))
	{
	  DPRINT("Found: '%wZ'  '%wZ'\n", &Service->ServiceName, &DirInfo->ObjectName);

	  /* Mark service as 'running' */
	  Service->ServiceRunning = TRUE;

	  /* Find the driver's group and mark it as 'running' */
	  if (Service->ServiceGroup.Buffer != NULL)
	    {
	      GroupEntry = GroupListHead.Flink;
	      while (GroupEntry != &GroupListHead)
		{
		  CurrentGroup = CONTAINING_RECORD(GroupEntry, SERVICE_GROUP, GroupListEntry);

		  DPRINT("Checking group '%wZ'\n", &CurrentGroup->GroupName);
		  if (RtlEqualUnicodeString(&Service->ServiceGroup, &CurrentGroup->GroupName, TRUE))
		    {
		      CurrentGroup->ServicesRunning = TRUE;
		    }

		  GroupEntry = GroupEntry->Flink;
		}
	    }
	  break;
	}
    }

  HeapFree(GetProcessHeap(),
	   0,
	   DirInfo);
  NtClose(DirHandle);

  return STATUS_SUCCESS;
}


VOID
ScmGetBootAndSystemDriverState(VOID)
{
  PLIST_ENTRY ServiceEntry;
  PSERVICE CurrentService;

  DPRINT("ScmGetBootAndSystemDriverState() called\n");

  ServiceEntry = ServiceListHead.Flink;
  while (ServiceEntry != &ServiceListHead)
    {
      CurrentService = CONTAINING_RECORD(ServiceEntry, SERVICE, ServiceListEntry);

      if (CurrentService->Start == SERVICE_BOOT_START ||
	  CurrentService->Start == SERVICE_SYSTEM_START)
	{
	  /* Check driver */
	  DPRINT("  Checking service: %wZ\n", &CurrentService->ServiceName);

	  ScmCheckDriver(CurrentService);
	}
      ServiceEntry = ServiceEntry->Flink;
    }

  DPRINT("ScmGetBootAndSystemDriverState() done\n");
}


static NTSTATUS
ScmSendStartCommand(PSERVICE Service, LPWSTR Arguments)
{
  PSCM_START_PACKET StartPacket;
  DWORD TotalLength;
#if 0
  DWORD Length;
#endif
  PWSTR Ptr;
  DWORD Count;

  DPRINT("ScmSendStartCommand() called\n");

  /* Calculate the total length of the start command line */
  TotalLength = wcslen(Service->ServiceName.Buffer) + 1;
#if 0
  if (Arguments != NULL)
  {
    Ptr = Arguments;
    while (*Ptr)
    {
      Length = wcslen(Ptr) + 1;
      TotalLength += Length;
      Ptr += Length;
    }
  }
#endif
  TotalLength++;

  /* Allocate start command packet */
  StartPacket = HeapAlloc(GetProcessHeap(),
			  HEAP_ZERO_MEMORY,
			  sizeof(SCM_START_PACKET) + (TotalLength - 1) * sizeof(WCHAR));
  if (StartPacket == NULL)
    return STATUS_INSUFFICIENT_RESOURCES;

  StartPacket->Command = SCM_START_COMMAND;
  StartPacket->Size = TotalLength;
  Ptr = &StartPacket->Arguments[0];
  wcscpy(Ptr, Service->ServiceName.Buffer);
  Ptr += (wcslen(Service->ServiceName.Buffer) + 1);

  /* FIXME: Copy argument list */

  *Ptr = 0;

  /* Send the start command */
  WriteFile(Service->ControlPipeHandle,
	    StartPacket,
	    sizeof(SCM_START_PACKET) + (TotalLength - 1) * sizeof(WCHAR),
	    &Count,
	    NULL);

  /* FIXME: Read the reply */

  HeapFree(GetProcessHeap(),
           0,
           StartPacket);

  DPRINT("ScmSendStartCommand() done\n");

  return STATUS_SUCCESS;
}


static NTSTATUS
ScmStartUserModeService(PSERVICE Service)
{
  RTL_QUERY_REGISTRY_TABLE QueryTable[3];
  PROCESS_INFORMATION ProcessInformation;
  STARTUPINFOW StartupInfo;
  UNICODE_STRING ImagePath;
  ULONG Type;
  BOOL Result;
  NTSTATUS Status;

  RtlInitUnicodeString(&ImagePath, NULL);

  /* Get service data */
  RtlZeroMemory(&QueryTable,
		sizeof(QueryTable));

  QueryTable[0].Name = L"Type";
  QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
  QueryTable[0].EntryContext = &Type;

  QueryTable[1].Name = L"ImagePath";
  QueryTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
  QueryTable[1].EntryContext = &ImagePath;

  Status = RtlQueryRegistryValues(RTL_REGISTRY_SERVICES,
				  Service->ServiceName.Buffer,
				  QueryTable,
				  NULL,
				  NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("RtlQueryRegistryValues() failed (Status %lx)\n", Status);
      return Status;
    }
  DPRINT("ImagePath: '%S'\n", ImagePath.Buffer);
  DPRINT("Type: %lx\n", Type);

  /* Create '\\.\pipe\net\NtControlPipe' instance */
  Service->ControlPipeHandle = CreateNamedPipeW(L"\\\\.\\pipe\\net\\NtControlPipe",
						PIPE_ACCESS_DUPLEX,
						PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
						100,
						8000,
						4,
						30000,
						NULL);
  DPRINT("CreateNamedPipeW() done\n");
  if (Service->ControlPipeHandle == INVALID_HANDLE_VALUE)
    {
      DPRINT1("Failed to create control pipe!\n");
      return STATUS_UNSUCCESSFUL;
    }

  StartupInfo.cb = sizeof(StartupInfo);
  StartupInfo.lpReserved = NULL;
  StartupInfo.lpDesktop = NULL;
  StartupInfo.lpTitle = NULL;
  StartupInfo.dwFlags = 0;
  StartupInfo.cbReserved2 = 0;
  StartupInfo.lpReserved2 = 0;

  Result = CreateProcessW(ImagePath.Buffer,
			  NULL,
			  NULL,
			  NULL,
			  FALSE,
			  DETACHED_PROCESS | CREATE_SUSPENDED,
			  NULL,
			  NULL,
			  &StartupInfo,
			  &ProcessInformation);
  RtlFreeUnicodeString(&ImagePath);

  if (!Result)
    {
      /* Close control pipe */
      CloseHandle(Service->ControlPipeHandle);
      Service->ControlPipeHandle = INVALID_HANDLE_VALUE;

      DPRINT1("Starting '%S' failed!\n", Service->ServiceName.Buffer);
      return STATUS_UNSUCCESSFUL;
    }

  DPRINT("Process Id: %lu  Handle %lx\n",
	 ProcessInformation.dwProcessId,
	 ProcessInformation.hProcess);
  DPRINT("Thread Id: %lu  Handle %lx\n",
	 ProcessInformation.dwThreadId,
	 ProcessInformation.hThread);

  /* Get process and thread ids */
  Service->ProcessId = ProcessInformation.dwProcessId;
  Service->ThreadId = ProcessInformation.dwThreadId;

  /* Resume Thread */
  ResumeThread(ProcessInformation.hThread);

  /* Connect control pipe */
  if (ConnectNamedPipe(Service->ControlPipeHandle, NULL))
    {
      DWORD dwProcessId = 0;
      DWORD dwRead = 0;

      DPRINT("Control pipe connected!\n");

      /* Read thread id from pipe */
      if (!ReadFile(Service->ControlPipeHandle,
		    (LPVOID)&dwProcessId,
		    sizeof(DWORD),
		    &dwRead,
		    NULL))
	{
	  DPRINT1("Reading the service control pipe failed (Error %lu)\n",
		  GetLastError());
	  Status = STATUS_UNSUCCESSFUL;
	}
      else
	{
	  DPRINT("Received process id %lu\n", dwProcessId);

	  /* FIXME: Send start command */

	  Status = STATUS_SUCCESS;
	}
    }
  else
    {
      DPRINT("Connecting control pipe failed!\n");

      /* Close control pipe */
      CloseHandle(Service->ControlPipeHandle);
      Service->ControlPipeHandle = INVALID_HANDLE_VALUE;
      Service->ProcessId = 0;
      Service->ThreadId = 0;
      Status = STATUS_UNSUCCESSFUL;
    }

  ScmSendStartCommand(Service, NULL);

  /* Close process and thread handle */
  CloseHandle(ProcessInformation.hThread);
  CloseHandle(ProcessInformation.hProcess);

  return Status;
}


static NTSTATUS
ScmStartService(PSERVICE Service,
		PSERVICE_GROUP Group)
{
  NTSTATUS Status;

  DPRINT("ScmStartService() called\n");

  Service->ControlPipeHandle = INVALID_HANDLE_VALUE;
  DPRINT("Service->Type: %u\n", Service->Type);

  if (Service->Type == SERVICE_KERNEL_DRIVER ||
      Service->Type == SERVICE_FILE_SYSTEM_DRIVER ||
      Service->Type == SERVICE_RECOGNIZER_DRIVER)
    {
      /* Load driver */
      DPRINT("  Path: %wZ\n", &Service->RegistryPath);
      Status = NtLoadDriver(&Service->RegistryPath);
    }
  else
    {
      /* Start user-mode service */
      Status = ScmStartUserModeService(Service);
    }

  DPRINT("ScmStartService() done (Status %lx)\n", Status);

  if (NT_SUCCESS(Status))
    {
      if (Group != NULL)
	{
	  Group->ServicesRunning = TRUE;
	}
      Service->ServiceRunning = TRUE;
    }
#if 0
  else
    {
      switch (Service->ErrorControl)
	{
	  case SERVICE_ERROR_NORMAL:
	    /* FIXME: Log error */
	    break;

	  case SERVICE_ERROR_SEVERE:
	    if (IsLastKnownGood == FALSE)
	      {
		/* FIXME: Boot last known good configuration */
	      }
	    break;

	  case SERVICE_ERROR_CRITICAL:
	    if (IsLastKnownGood == FALSE)
	      {
		/* FIXME: Boot last known good configuration */
	      }
	    else
	      {
		/* FIXME: BSOD! */
	      }
	    break;
	}
    }
#endif

  return Status;
}


VOID
ScmAutoStartServices(VOID)
{
  PLIST_ENTRY GroupEntry;
  PLIST_ENTRY ServiceEntry;
  PSERVICE_GROUP CurrentGroup;
  PSERVICE CurrentService;
  ULONG i;

  /* Clear 'ServiceVisited' flag */
  ServiceEntry = ServiceListHead.Flink;
  while (ServiceEntry != &ServiceListHead)
    {
      CurrentService = CONTAINING_RECORD(ServiceEntry, SERVICE, ServiceListEntry);
      CurrentService->ServiceVisited = FALSE;
      ServiceEntry = ServiceEntry->Flink;
    }

  /* Start all services which are members of an existing group */
  GroupEntry = GroupListHead.Flink;
  while (GroupEntry != &GroupListHead)
    {
      CurrentGroup = CONTAINING_RECORD(GroupEntry, SERVICE_GROUP, GroupListEntry);

      DPRINT("Group '%wZ'\n", &CurrentGroup->GroupName);

      /* Start all services witch have a valid tag */
      for (i = 0; i < CurrentGroup->TagCount; i++)
        {
          ServiceEntry = ServiceListHead.Flink;
          while (ServiceEntry != &ServiceListHead)
	    {
	      CurrentService = CONTAINING_RECORD(ServiceEntry, SERVICE, ServiceListEntry);

	      if ((RtlEqualUnicodeString(&CurrentGroup->GroupName, &CurrentService->ServiceGroup, TRUE)) &&
	          (CurrentService->Start == SERVICE_AUTO_START) &&
	          (CurrentService->ServiceVisited == FALSE) &&
		  (CurrentService->Tag == CurrentGroup->TagArray[i]))
	        {
	          CurrentService->ServiceVisited = TRUE;
	          ScmStartService(CurrentService,
			          CurrentGroup);
	        }

	      ServiceEntry = ServiceEntry->Flink;
	    }
        }

      /* Start all services which have an invalid tag or which do not have a tag */ 
      ServiceEntry = ServiceListHead.Flink;
      while (ServiceEntry != &ServiceListHead)
	{
	  CurrentService = CONTAINING_RECORD(ServiceEntry, SERVICE, ServiceListEntry);

	  if ((RtlEqualUnicodeString(&CurrentGroup->GroupName, &CurrentService->ServiceGroup, TRUE)) &&
	      (CurrentService->Start == SERVICE_AUTO_START) &&
	      (CurrentService->ServiceVisited == FALSE))
	    {
	      CurrentService->ServiceVisited = TRUE;
	      ScmStartService(CurrentService,
			      CurrentGroup);
	    }

	  ServiceEntry = ServiceEntry->Flink;
	}

      GroupEntry = GroupEntry->Flink;
    }

  /* Start all services which are members of any non-existing group */
  ServiceEntry = ServiceListHead.Flink;
  while (ServiceEntry != &ServiceListHead)
    {
      CurrentService = CONTAINING_RECORD(ServiceEntry, SERVICE, ServiceListEntry);

      if ((CurrentService->ServiceGroup.Length != 0) &&
	  (CurrentService->Start == SERVICE_AUTO_START) &&
	  (CurrentService->ServiceVisited == FALSE))
	{
	  CurrentService->ServiceVisited = TRUE;
	  ScmStartService(CurrentService,
			  NULL);
	}

      ServiceEntry = ServiceEntry->Flink;
    }

  /* Start all services which are not a member of any group */
  ServiceEntry = ServiceListHead.Flink;
  while (ServiceEntry != &ServiceListHead)
    {
      CurrentService = CONTAINING_RECORD(ServiceEntry, SERVICE, ServiceListEntry);

      if ((CurrentService->ServiceGroup.Length == 0) &&
	  (CurrentService->Start == SERVICE_AUTO_START) &&
	  (CurrentService->ServiceVisited == FALSE))
	{
	  CurrentService->ServiceVisited = TRUE;
	  ScmStartService(CurrentService,
			  NULL);
	}

      ServiceEntry = ServiceEntry->Flink;
    }

  /* Clear 'ServiceVisited' flag again */
  ServiceEntry = ServiceListHead.Flink;
  while (ServiceEntry != &ServiceListHead)
    {
      CurrentService = CONTAINING_RECORD(ServiceEntry, SERVICE, ServiceListEntry);
      CurrentService->ServiceVisited = FALSE;
      ServiceEntry = ServiceEntry->Flink;
    }
}

/* EOF */
