/* $Id: driver.c,v 1.5 2002/06/13 15:13:54 ekohl Exp $
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
  RtlInitUnicodeString(&IoDriverObjectType->TypeName, L"Driver");
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
 */
NTSTATUS STDCALL
NtLoadDriver(IN PUNICODE_STRING DriverServiceName)
{
  RTL_QUERY_REGISTRY_TABLE QueryTable[3];
  WCHAR ImagePathBuffer[MAX_PATH];
  WCHAR FullImagePathBuffer[MAX_PATH];
  UNICODE_STRING ImagePath;
  UNICODE_STRING FullImagePath;
  NTSTATUS Status;
  ULONG Type;
  PDEVICE_NODE DeviceNode;
  PMODULE_OBJECT ModuleObject;
  LPWSTR Start;

  DPRINT("DriverServiceName: '%wZ'\n", DriverServiceName);

  ImagePath.Length = 0;
  ImagePath.MaximumLength = MAX_PATH * sizeof(WCHAR);
  ImagePath.Buffer = ImagePathBuffer;
  RtlZeroMemory(ImagePathBuffer,
		MAX_PATH * sizeof(WCHAR));

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
      return(Status);
    }

  if (ImagePath.Length == 0)
    {
      wcscpy(FullImagePathBuffer, L"\\SystemRoot\\system32\\drivers");
      wcscat(ImagePathBuffer, wcsrchr(DriverServiceName->Buffer, L'\\'));
      wcscat(ImagePathBuffer, L".sys");
    }
  else
    {
      wcscpy(FullImagePathBuffer, L"\\SystemRoot\\");
      wcscat(FullImagePathBuffer, ImagePathBuffer);
    }

  RtlInitUnicodeString(&FullImagePath, FullImagePathBuffer);

  DPRINT("FullImagePath: '%S'\n", FullImagePathBuffer);
  DPRINT("Type %lx\n", Type);

  /* Use IopRootDeviceNode for now */
  Status = IopCreateDeviceNode(IopRootDeviceNode, NULL, &DeviceNode);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
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
      DPRINT("IopInitializeDriver() failed (Status %lx)\n", Status);
      LdrUnloadModule(ModuleObject);
      IopFreeDeviceNode(DeviceNode);
    }

  return(Status);
}


NTSTATUS STDCALL
NtUnloadDriver(IN PUNICODE_STRING DriverServiceName)
{
  DPRINT("DriverServiceName: '%wZ'\n", DriverServiceName);

  return(STATUS_NOT_IMPLEMENTED);
}


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


static NTSTATUS STDCALL
IopCreateServiceListEntry(PUNICODE_STRING ServiceName)
{
  RTL_QUERY_REGISTRY_TABLE QueryTable[6];
  WCHAR ServiceGroupBuffer[MAX_PATH];
  WCHAR ImagePathBuffer[MAX_PATH];
  UNICODE_STRING ServiceGroup;
  UNICODE_STRING ImagePath;
  PSERVICE Service;
  NTSTATUS Status;
  ULONG Start, Type, ErrorControl;

  DPRINT("ServiceName: '%wZ'\n", ServiceName);

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
  QueryTable[0].EntryContext = &Start;

  QueryTable[1].Name = L"Type";
  QueryTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
  QueryTable[1].EntryContext = &Type;

  QueryTable[2].Name = L"ErrorControl";
  QueryTable[2].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
  QueryTable[2].EntryContext = &ErrorControl;

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
      DPRINT1("RtlQueryRegistryValues() failed (Status %lx)\n", Status);
      return(Status);
    }

  if (Start < 2)
    {
      /* Allocate service entry */
      Service = (PSERVICE)ExAllocatePool(NonPagedPool, sizeof(SERVICE));
      if (Service == NULL)
	{
	  DPRINT1("ExAllocatePool() failed\n");
	  return(STATUS_INSUFFICIENT_RESOURCES);
	}
      RtlZeroMemory(Service, sizeof(SERVICE));

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

      /* Copy service group */
      if (ServiceGroup.Length > 0)
	{
	  Service->ServiceGroup.Length = ServiceGroup.Length;
	  Service->ServiceGroup.MaximumLength = ServiceGroup.Length + sizeof(WCHAR);
	  Service->ServiceGroup.Buffer = ExAllocatePool(NonPagedPool,
							ServiceGroup.Length + sizeof(WCHAR));
	  RtlCopyMemory(Service->ServiceGroup.Buffer,
			ServiceGroup.Buffer,
			ServiceGroup.Length);
	  Service->ServiceGroup.Buffer[ServiceGroup.Length / sizeof(WCHAR)] = 0;
	}

      /* Copy image path */
      if (ImagePath.Length > 0)
	{
	  Service->ImagePath.Length = ImagePath.Length;
	  Service->ImagePath.MaximumLength = ImagePath.Length + sizeof(WCHAR);
	  Service->ImagePath.Buffer = ExAllocatePool(NonPagedPool,
						     ImagePath.Length + sizeof(WCHAR));
	  RtlCopyMemory(Service->ImagePath.Buffer,
			ImagePath.Buffer,
			ImagePath.Length);
	  Service->ImagePath.Buffer[ImagePath.Length / sizeof(WCHAR)] = 0;
	}

      Service->Start = Start;
      Service->Type = Type;
      Service->ErrorControl = ErrorControl;

      DPRINT("ServiceName: '%wZ'\n", &Service->ServiceName);
      DPRINT("RegistryPath: '%wZ'\n", &Service->RegistryPath);
      DPRINT("ServiceGroup: '%wZ'\n", &Service->ServiceGroup);
      DPRINT("ImagePath: '%wZ'\n", &Service->ImagePath);
      DPRINT("Start %lx  Type %lx  ErrorControl %lx\n",
	     Service->Start, Service->Type, Service->ErrorControl);

      /* Append service entry */
      InsertTailList(&ServiceListHead,
		     &Service->ServiceListEntry);
    }

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
  RtlInitUnicodeString(&ServicesKeyName,
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
      ServiceEntry = ServiceListHead.Flink;
    }

  DPRINT("IoDestroyDriverList() done\n");

  return(STATUS_SUCCESS);
}

/* EOF */
