/* $Id: driver.c,v 1.13 2003/07/10 15:47:00 royce Exp $
 *
 * COPYRIGHT:      See COPYING in the top level directory
 * PROJECT:        ReactOS kernel
 * FILE:           ntoskrnl/io/driver.c
 * PURPOSE:        Manage devices
 * PROGRAMMER:     David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                 15/05/98: Created
 */

/* INCLUDES ****************************************************************/

#include <limits.h>
#include <ddk/ntddk.h>
#include <internal/io.h>
#include <internal/po.h>
#include <internal/ldr.h>
#include <internal/id.h>
#include <internal/pool.h>
#include <internal/registry.h>

#include <roscfg.h>

#define NDEBUG
#include <internal/debug.h>


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
  UNICODE_STRING ImagePath;

  ULONG Start;
  ULONG Type;
  ULONG ErrorControl;
  ULONG Tag;

  BOOLEAN ServiceRunning;	// needed ??

} SERVICE, *PSERVICE;


/* GLOBALS *******************************************************************/

static LIST_ENTRY GroupListHead = {NULL, NULL};
static LIST_ENTRY ServiceListHead  = {NULL, NULL};

POBJECT_TYPE EXPORTED IoDriverObjectType = NULL;

#define TAG_DRIVER             TAG('D', 'R', 'V', 'R')
#define TAG_DRIVER_EXTENSION   TAG('D', 'R', 'V', 'E')


/* FUNCTIONS ***************************************************************/

/*
 * @implemented
 */
NTSTATUS STDCALL
IopCreateDriver(PVOID ObjectBody,
		PVOID Parent,
		PWSTR RemainingPath,
		POBJECT_ATTRIBUTES ObjectAttributes)
{
  DPRINT("LdrCreateModule(ObjectBody %x, Parent %x, RemainingPath %S)\n",
	 ObjectBody,
	 Parent,
	 RemainingPath);
  if (RemainingPath != NULL && wcschr(RemainingPath + 1, '\\') != NULL)
    {
      return(STATUS_UNSUCCESSFUL);
    }

  return(STATUS_SUCCESS);
}


VOID
IopInitDriverImplementation(VOID)
{
  /*  Register the process object type  */
  IoDriverObjectType = ExAllocatePool(NonPagedPool, sizeof(OBJECT_TYPE));
  IoDriverObjectType->Tag = TAG('D', 'R', 'V', 'R');
  IoDriverObjectType->TotalObjects = 0;
  IoDriverObjectType->TotalHandles = 0;
  IoDriverObjectType->MaxObjects = ULONG_MAX;
  IoDriverObjectType->MaxHandles = ULONG_MAX;
  IoDriverObjectType->PagedPoolCharge = 0;
  IoDriverObjectType->NonpagedPoolCharge = sizeof(DRIVER_OBJECT);
  IoDriverObjectType->Dump = NULL;
  IoDriverObjectType->Open = NULL;
  IoDriverObjectType->Close = NULL;
  IoDriverObjectType->Delete = NULL;
  IoDriverObjectType->Parse = NULL;
  IoDriverObjectType->Security = NULL;
  IoDriverObjectType->QueryName = NULL;
  IoDriverObjectType->OkayToClose = NULL;
  IoDriverObjectType->Create = IopCreateDriver;
  IoDriverObjectType->DuplicationNotify = NULL;
  RtlInitUnicodeStringFromLiteral(&IoDriverObjectType->TypeName, L"Driver");
}

/**********************************************************************
 * NAME							EXPORTED
 *	NtLoadDriver
 *
 * DESCRIPTION
 * 	Loads a device driver.
 * 	
 * ARGUMENTS
 *	DriverServiceName
 *		Name of the service to load (registry key).
 *		
 * RETURN VALUE
 * 	Status.
 *
 * REVISIONS
 *
 * @implemented
 */
NTSTATUS STDCALL
NtLoadDriver(IN PUNICODE_STRING DriverServiceName)
{
  RTL_QUERY_REGISTRY_TABLE QueryTable[3];
  WCHAR FullImagePathBuffer[MAX_PATH];
  UNICODE_STRING ImagePath;
  UNICODE_STRING FullImagePath;
  NTSTATUS Status;
  ULONG Type;
  PDEVICE_NODE DeviceNode;
  PMODULE_OBJECT ModuleObject;
  LPWSTR Start;

  DPRINT("NtLoadDriver(%wZ) called\n", DriverServiceName);

  RtlInitUnicodeString(&ImagePath, NULL);

  /* Get service data */
  RtlZeroMemory(&QueryTable,
		sizeof(QueryTable));

  QueryTable[0].Name = L"Type";
  QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
  QueryTable[0].EntryContext = &Type;

  QueryTable[1].Name = L"ImagePath";
  QueryTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
  QueryTable[1].EntryContext = &ImagePath;

  Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
				  DriverServiceName->Buffer,
				  QueryTable,
				  NULL,
				  NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("RtlQueryRegistryValues() failed (Status %lx)\n", Status);
      RtlFreeUnicodeString(&ImagePath);
      return(Status);
    }

  if (ImagePath.Length == 0)
    {
      wcscpy(FullImagePathBuffer, L"\\SystemRoot\\system32\\drivers");
      wcscat(FullImagePathBuffer, wcsrchr(DriverServiceName->Buffer, L'\\'));
      wcscat(FullImagePathBuffer, L".sys");
    }
  else if (ImagePath.Buffer[0] != L'\\')
    {
      wcscpy(FullImagePathBuffer, L"\\SystemRoot\\");
      wcscat(FullImagePathBuffer, ImagePath.Buffer);
    }
  else
    {
      wcscpy(FullImagePathBuffer, ImagePath.Buffer);
    }

  RtlFreeUnicodeString(&ImagePath);
  RtlInitUnicodeString(&FullImagePath, FullImagePathBuffer);

  DPRINT("FullImagePath: '%S'\n", FullImagePathBuffer);
  DPRINT("Type %lx\n", Type);

  /* Use IopRootDeviceNode for now */
  Status = IopCreateDeviceNode(IopRootDeviceNode, NULL, &DeviceNode);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("IopCreateDeviceNode() failed (Status %lx)\n", Status);
      return(Status);
    }

  ModuleObject = LdrGetModuleObject(DriverServiceName);
  if (ModuleObject != NULL)
    {
      return(STATUS_IMAGE_ALREADY_LOADED);
    }

  Status = LdrLoadModule(&FullImagePath, &ModuleObject);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("LdrLoadModule() failed (Status %lx)\n", Status);
      IopFreeDeviceNode(DeviceNode);
      return(Status);
    }

  /* Set a service name for the device node */
  Start = wcsrchr(DriverServiceName->Buffer, L'\\');
  if (Start == NULL)
    Start = DriverServiceName->Buffer;
  else
    Start++;
  RtlCreateUnicodeString(&DeviceNode->ServiceName, Start);

  Status = IopInitializeDriver(ModuleObject->EntryPoint,
			       DeviceNode,
			       (Type == 2 || Type == 8));
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("IopInitializeDriver() failed (Status %lx)\n", Status);
      LdrUnloadModule(ModuleObject);
      IopFreeDeviceNode(DeviceNode);
    }

  return(Status);
}


/*
 * @unimplemented
 */
NTSTATUS STDCALL
NtUnloadDriver(IN PUNICODE_STRING DriverServiceName)
{
  DPRINT("DriverServiceName: '%wZ'\n", DriverServiceName);

  return(STATUS_NOT_IMPLEMENTED);
}


/*
 * @implemented
 */
static NTSTATUS STDCALL
IopCreateGroupListEntry(PWSTR ValueName,
			ULONG ValueType,
			PVOID ValueData,
			ULONG ValueLength,
			PVOID Context,
			PVOID EntryContext)
{
  PSERVICE_GROUP Group;

  if (ValueType == REG_SZ)
    {
      DPRINT("GroupName: '%S'\n", (PWCHAR)ValueData);

      Group = ExAllocatePool(NonPagedPool,
			     sizeof(SERVICE_GROUP));
      if (Group == NULL)
	{
	  return(STATUS_INSUFFICIENT_RESOURCES);
	}

      RtlZeroMemory(Group, sizeof(SERVICE_GROUP));

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


/*
 * @implemented
 */
static NTSTATUS STDCALL
IopCreateServiceListEntry(PUNICODE_STRING ServiceName)
{
  RTL_QUERY_REGISTRY_TABLE QueryTable[6];
  PSERVICE Service;
  NTSTATUS Status;

  DPRINT("ServiceName: '%wZ'\n", ServiceName);

  /* Allocate service entry */
  Service = (PSERVICE)ExAllocatePool(NonPagedPool, sizeof(SERVICE));
  if (Service == NULL)
    {
      DPRINT1("ExAllocatePool() failed\n");
      return(STATUS_INSUFFICIENT_RESOURCES);
    }
  RtlZeroMemory(Service, sizeof(SERVICE));

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

  QueryTable[4].Name = L"ImagePath";
  QueryTable[4].Flags = RTL_QUERY_REGISTRY_DIRECT;
  QueryTable[4].EntryContext = &Service->ImagePath;

  Status = RtlQueryRegistryValues(RTL_REGISTRY_SERVICES,
				  ServiceName->Buffer,
				  QueryTable,
				  NULL,
				  NULL);
  if (!NT_SUCCESS(Status) || Service->Start > 1)
    {
      RtlFreeUnicodeString(&Service->ServiceGroup);
      RtlFreeUnicodeString(&Service->ImagePath);
      ExFreePool(Service);
      return(Status);
    }

  /* Copy service name */
  Service->ServiceName.Length = ServiceName->Length;
  Service->ServiceName.MaximumLength = ServiceName->Length + sizeof(WCHAR);
  Service->ServiceName.Buffer = ExAllocatePool(NonPagedPool,
					       Service->ServiceName.MaximumLength);
  RtlCopyMemory(Service->ServiceName.Buffer,
		ServiceName->Buffer,
		ServiceName->Length);
  Service->ServiceName.Buffer[ServiceName->Length / sizeof(WCHAR)] = 0;

  /* Build registry path */
  Service->RegistryPath.MaximumLength = MAX_PATH * sizeof(WCHAR);
  Service->RegistryPath.Buffer = ExAllocatePool(NonPagedPool,
						MAX_PATH * sizeof(WCHAR));
  wcscpy(Service->RegistryPath.Buffer,
	 L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\");
  wcscat(Service->RegistryPath.Buffer,
	 Service->ServiceName.Buffer);
  Service->RegistryPath.Length = wcslen(Service->RegistryPath.Buffer) * sizeof(WCHAR);

  DPRINT("ServiceName: '%wZ'\n", &Service->ServiceName);
  DPRINT("RegistryPath: '%wZ'\n", &Service->RegistryPath);
  DPRINT("ServiceGroup: '%wZ'\n", &Service->ServiceGroup);
  DPRINT("ImagePath: '%wZ'\n", &Service->ImagePath);
  DPRINT("Start %lx  Type %lx  ErrorControl %lx\n",
	 Service->Start, Service->Type, Service->ErrorControl);

  /* Append service entry */
  InsertTailList(&ServiceListHead,
		 &Service->ServiceListEntry);

  return(STATUS_SUCCESS);
}


NTSTATUS
IoCreateDriverList(VOID)
{
  RTL_QUERY_REGISTRY_TABLE QueryTable[2];
  PKEY_BASIC_INFORMATION KeyInfo = NULL;
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING ServicesKeyName;
  UNICODE_STRING SubKeyName;
  HANDLE KeyHandle;
  NTSTATUS Status;
  ULONG Index;

  ULONG KeyInfoLength = 0;
  ULONG ReturnedLength;

  DPRINT("IoCreateDriverList() called\n");

  /* Initialize basic variables */
  InitializeListHead(&GroupListHead);
  InitializeListHead(&ServiceListHead);

  /* Build group order list */
  RtlZeroMemory(&QueryTable,
		sizeof(QueryTable));

  QueryTable[0].Name = L"List";
  QueryTable[0].QueryRoutine = IopCreateGroupListEntry;

  Status = RtlQueryRegistryValues(RTL_REGISTRY_CONTROL,
				  L"ServiceGroupOrder",
				  QueryTable,
				  NULL,
				  NULL);
  if (!NT_SUCCESS(Status))
    return(Status);

  /* Enumerate services and create the service list */
  RtlInitUnicodeStringFromLiteral(&ServicesKeyName,
		       L"\\Registry\\Machine\\System\\CurrentControlSet\\Services");

  InitializeObjectAttributes(&ObjectAttributes,
			     &ServicesKeyName,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);

  Status = NtOpenKey(&KeyHandle,
		     0x10001,
		     &ObjectAttributes);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  KeyInfoLength = sizeof(KEY_BASIC_INFORMATION) + MAX_PATH * sizeof(WCHAR);
  KeyInfo = ExAllocatePool(NonPagedPool, KeyInfoLength);
  if (KeyInfo == NULL)
    {
      NtClose(KeyHandle);
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  Index = 0;
  while (TRUE)
    {
      Status = NtEnumerateKey(KeyHandle,
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
	      IopCreateServiceListEntry(&SubKeyName);
	    }
	}

      if (!NT_SUCCESS(Status))
	break;

      Index++;
    }

  ExFreePool(KeyInfo);
  NtClose(KeyHandle);

  DPRINT("IoCreateDriverList() done\n");

  return(STATUS_SUCCESS);
}


VOID
LdrLoadAutoConfigDrivers(VOID)
{
  PLIST_ENTRY GroupEntry;
  PLIST_ENTRY ServiceEntry;
  PSERVICE_GROUP CurrentGroup;
  PSERVICE CurrentService;
  NTSTATUS Status;

  CHAR TextBuffer [256];
  ULONG x, y, cx, cy;

  DPRINT("LdrLoadAutoConfigDrivers() called\n");

  GroupEntry = GroupListHead.Flink;
  while (GroupEntry != &GroupListHead)
    {
      CurrentGroup = CONTAINING_RECORD(GroupEntry, SERVICE_GROUP, GroupListEntry);

      DPRINT("Group: %wZ\n", &CurrentGroup->GroupName);

      ServiceEntry = ServiceListHead.Flink;
      while (ServiceEntry != &ServiceListHead)
	{
	  CurrentService = CONTAINING_RECORD(ServiceEntry, SERVICE, ServiceListEntry);

	  if ((RtlCompareUnicodeString(&CurrentGroup->GroupName, &CurrentService->ServiceGroup, TRUE) == 0) &&
	      (CurrentService->Start == 1 /*SERVICE_SYSTEM_START*/))
	    {

	      HalQueryDisplayParameters(&x, &y, &cx, &cy);
	      RtlFillMemory(TextBuffer, x, ' ');
	      TextBuffer[x] = '\0';
	      HalSetDisplayParameters(0, y-1);
	      HalDisplayString(TextBuffer);

	      sprintf(TextBuffer, "Loading %S...\n", CurrentService->ServiceName.Buffer);
	      HalSetDisplayParameters(0, y-1);
	      HalDisplayString(TextBuffer);
	      HalSetDisplayParameters(cx, cy);

	      DPRINT("  Path: %wZ\n", &CurrentService->RegistryPath);
	      Status = NtLoadDriver(&CurrentService->RegistryPath);
	      if (!NT_SUCCESS(Status))
		{
		  DPRINT("NtLoadDriver() failed (Status %lx)\n", Status);
#if 0
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
#endif
		}
	    }
	  ServiceEntry = ServiceEntry->Flink;
	}

      GroupEntry = GroupEntry->Flink;
    }

  DPRINT("LdrLoadAutoConfigDrivers() done\n");
}


NTSTATUS
IoDestroyDriverList(VOID)
{
  PLIST_ENTRY GroupEntry;
  PLIST_ENTRY ServiceEntry;
  PSERVICE_GROUP CurrentGroup;
  PSERVICE CurrentService;

  DPRINT("IoDestroyDriverList() called\n");

  /* Destroy group list */
  GroupEntry = GroupListHead.Flink;
  while (GroupEntry != &GroupListHead)
    {
      CurrentGroup = CONTAINING_RECORD(GroupEntry, SERVICE_GROUP, GroupListEntry);

      RtlFreeUnicodeString(&CurrentGroup->GroupName);
      RemoveEntryList(GroupEntry);
      ExFreePool(CurrentGroup);

      GroupEntry = GroupListHead.Flink;
    }

  /* Destroy service list */
  ServiceEntry = ServiceListHead.Flink;
  while (ServiceEntry != &ServiceListHead)
    {
      CurrentService = CONTAINING_RECORD(ServiceEntry, SERVICE, ServiceListEntry);

      RtlFreeUnicodeString(&CurrentService->ServiceName);
      RtlFreeUnicodeString(&CurrentService->RegistryPath);
      RtlFreeUnicodeString(&CurrentService->ServiceGroup);
      RtlFreeUnicodeString(&CurrentService->ImagePath);
      RemoveEntryList(ServiceEntry);
      ExFreePool(CurrentService);

      ServiceEntry = ServiceListHead.Flink;
    }

  DPRINT("IoDestroyDriverList() done\n");

  return(STATUS_SUCCESS);
}

/* EOF */
