/* $Id: driver.c,v 1.36 2004/03/14 17:10:48 navaraf Exp $
 *
 * COPYRIGHT:      See COPYING in the top level directory
 * PROJECT:        ReactOS kernel
 * FILE:           ntoskrnl/io/driver.c
 * PURPOSE:        Loading and unloading of drivers
 * PROGRAMMER:     David Welch (welch@cwcom.net)
 *                 Filip Navara (xnavara@volny.cz)
 * UPDATE HISTORY:
 *                 15/05/98: Created
 */

/* INCLUDES *******************************************************************/

#include <limits.h>
#include <ddk/ntddk.h>
#include <internal/io.h>
#include <internal/po.h>
#include <internal/ldr.h>
#include <internal/id.h>
#include <internal/pool.h>
#include <internal/se.h>
#include <internal/mm.h>
#include <internal/ke.h>
#include <internal/kd.h>
#include <rosrtl/string.h>

#include <roscfg.h>

#define NDEBUG
#include <internal/debug.h>

/* ke/main.c */
extern LOADER_PARAMETER_BLOCK EXPORTED KeLoaderBlock;

NTSTATUS
IopInitializeService(
  PDEVICE_NODE DeviceNode,
  PUNICODE_STRING ImagePath);

NTSTATUS
LdrProcessModule(PVOID ModuleLoadBase,
		 PUNICODE_STRING ModuleName,
		 PMODULE_OBJECT *ModuleObject);

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

/*  BOOLEAN ServiceRunning;*/	// needed ??
} SERVICE, *PSERVICE;

typedef struct _DRIVER_REINIT_ITEM
{
  LIST_ENTRY ItemEntry;
  PDRIVER_OBJECT DriverObject;
  PDRIVER_REINITIALIZE ReinitRoutine;
  PVOID Context;
} DRIVER_REINIT_ITEM, *PDRIVER_REINIT_ITEM;

/* GLOBALS ********************************************************************/

static LIST_ENTRY DriverReinitListHead;
static PLIST_ENTRY DriverReinitTailEntry;
static KSPIN_LOCK DriverReinitListLock;

static LIST_ENTRY GroupListHead = {NULL, NULL};
static LIST_ENTRY ServiceListHead  = {NULL, NULL};

POBJECT_TYPE EXPORTED IoDriverObjectType = NULL;

#define TAG_DRIVER             TAG('D', 'R', 'V', 'R')
#define TAG_DRIVER_EXTENSION   TAG('D', 'R', 'V', 'E')

/* PRIVATE FUNCTIONS **********************************************************/

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


VOID INIT_FUNCTION
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
  RtlRosInitUnicodeStringFromLiteral(&IoDriverObjectType->TypeName, L"Driver");

  ObpCreateTypeObject(IoDriverObjectType);

  InitializeListHead(&DriverReinitListHead);
  KeInitializeSpinLock(&DriverReinitListLock);
  DriverReinitTailEntry = NULL;
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
	  ExFreePool(Group);
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


NTSTATUS INIT_FUNCTION
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
  RtlRosInitUnicodeStringFromLiteral(&ServicesKeyName,
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

NTSTATUS INIT_FUNCTION
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

VOID STATIC
MiFreeBootDriverMemory(PVOID StartAddress, ULONG Length)
{
  ULONG i;

  for (i = 0; i < PAGE_ROUND_UP(Length)/PAGE_SIZE; i++)
  {
     MmDeleteVirtualMapping(NULL, (char*)StartAddress + i * PAGE_SIZE, TRUE, NULL, NULL);
  }
}

/*
 * IopDisplayLoadingMessage
 *
 * Display 'Loading XXX...' message.
 */

VOID
IopDisplayLoadingMessage(PWCHAR ServiceName)
{
   CHAR TextBuffer[256];
   sprintf(TextBuffer, "Loading %S...\n", ServiceName);
   HalDisplayString(TextBuffer);
}

/*
 * IopInitializeBuiltinDriver
 *
 * Initialize a driver that is already loaded in memory.
 */

NTSTATUS INIT_FUNCTION
IopInitializeBuiltinDriver(
   PDEVICE_NODE ModuleDeviceNode,
   PVOID ModuleLoadBase,
   PCHAR FileName,
   ULONG ModuleLength)
{
   PMODULE_OBJECT ModuleObject;
   PDEVICE_NODE DeviceNode;
   NTSTATUS Status;
   CHAR TextBuffer[256];
   PCHAR FileNameWithoutPath;
   LPWSTR FileExtension;

   DPRINT("Initializing driver '%s' at %08lx, length 0x%08lx\n",
      FileName, ModuleLoadBase, ModuleLength);

   /*
    * Display 'Initializing XXX...' message
    */
   sprintf(TextBuffer, "Initializing %s...\n", FileName);
   HalDisplayString(TextBuffer);

   /*
    * Determine the right device object
    */
   if (ModuleDeviceNode == NULL)
   {
      /* Use IopRootDeviceNode for now */
      Status = IopCreateDeviceNode(IopRootDeviceNode, NULL, &DeviceNode);
      if (!NT_SUCCESS(Status))
      {
         CPRINT("Driver load failed, status (%x)\n", Status);
         return(Status);
      }
   } else
   {
      DeviceNode = ModuleDeviceNode;
   }

   /*
    * Generate filename without path (not needed by freeldr)
    */
   FileNameWithoutPath = strrchr(FileName, '\\');
   if (FileNameWithoutPath == NULL)
   {
      FileNameWithoutPath = FileName;
   }

   /*
    * Load the module
    */
   RtlCreateUnicodeStringFromAsciiz(&DeviceNode->ServiceName,
      FileNameWithoutPath);
   Status = LdrProcessModule(ModuleLoadBase, &DeviceNode->ServiceName,
      &ModuleObject);
   if (ModuleObject == NULL)
   {
      if (ModuleDeviceNode == NULL)
         IopFreeDeviceNode(DeviceNode);
      CPRINT("Driver load failed, status (%x)\n", Status);
      return STATUS_UNSUCCESSFUL;
   }

   /*
    * Strip the file extension from ServiceName
    */
   FileExtension = wcsrchr(DeviceNode->ServiceName.Buffer, '.');
   if (FileExtension != NULL)
   {
      DeviceNode->ServiceName.Length -= wcslen(DeviceNode->ServiceName.Buffer);
      FileExtension[0] = 0;
   }

   /*
    * Initialize the driver
    */
   Status = IopInitializeDriver(ModuleObject->EntryPoint, DeviceNode,
      FALSE, ModuleObject->Base, ModuleObject->Length, TRUE);
   if (!NT_SUCCESS(Status))
   {
      if (ModuleDeviceNode == NULL)
         IopFreeDeviceNode(DeviceNode);
      CPRINT("Driver load failed, status (%x)\n", Status);
   }

   return Status;
}

/*
 * IopInitializeBootDrivers
 *
 * Initialize boot drivers and free memory for boot files.
 *
 * Parameters
 *    None
 *		
 * Return Value
 *    None
 */

VOID INIT_FUNCTION
IopInitializeBootDrivers(VOID)
{
   ULONG BootDriverCount;
   ULONG ModuleStart;
   ULONG ModuleSize;
   ULONG ModuleLoaded;
   PCHAR ModuleName;
   PCHAR Extension;
   PLOADER_MODULE KeLoaderModules = (PLOADER_MODULE)KeLoaderBlock.ModsAddr;
   ULONG i;

   DPRINT("IopInitializeBootDrivers()\n");

   BootDriverCount = 0;
   for (i = 2; i < KeLoaderBlock.ModsCount; i++)
   {
      ModuleStart = KeLoaderModules[i].ModStart;
      ModuleSize = KeLoaderModules[i].ModEnd - ModuleStart;
      ModuleName = (PCHAR)KeLoaderModules[i].String;
      ModuleLoaded = KeLoaderModules[i].Reserved;
      Extension = strrchr(ModuleName, '.');
      if (Extension == NULL)
         Extension = "";

      /*
       * Pass symbol files to kernel debugger
       */
      if (!_stricmp(Extension, ".sym"))
      {
         KDB_SYMBOLFILE_HOOK((PVOID)ModuleStart, ModuleName, ModuleSize);
      } else

      /*
       * Load builtin driver
       */
      if (!_stricmp(Extension, ".sys"))
      {
         if (!ModuleLoaded)
         {
            IopInitializeBuiltinDriver(NULL, (PVOID)ModuleStart, ModuleName,
               ModuleSize);
         }
         ++BootDriverCount;
      }

      /*
       * Free memory for all boot files, except ntoskrnl.exe and hal.dll
       */
#ifdef KDBG
      /*
       * Do not free the memory from symbol files, if the kernel debugger
       * is active
       */
      if (_stricmp(Extension, ".sym"))
#endif
      {
         MiFreeBootDriverMemory((PVOID)KeLoaderModules[i].ModStart,
            KeLoaderModules[i].ModEnd - KeLoaderModules[i].ModStart);
      }
   }

   if (BootDriverCount == 0)
   {
      DbgPrint("No boot drivers available.\n");
      KEBUGCHECK(0);
   }
}

/*
 * IopInitializeSystemDrivers
 *
 * Load drivers marked as system start.
 *
 * Parameters
 *    None
 *		
 * Return Value
 *    None
 */

VOID INIT_FUNCTION
IopInitializeSystemDrivers(VOID)
{
   PLIST_ENTRY GroupEntry;
   PLIST_ENTRY ServiceEntry;
   PSERVICE_GROUP CurrentGroup;
   PSERVICE CurrentService;
   NTSTATUS Status;

   DPRINT("IopInitializeSystemDrivers()\n");

   GroupEntry = GroupListHead.Flink;
   while (GroupEntry != &GroupListHead)
   {
      CurrentGroup = CONTAINING_RECORD(GroupEntry, SERVICE_GROUP, GroupListEntry);

      DPRINT("Group: %wZ\n", &CurrentGroup->GroupName);

      ServiceEntry = ServiceListHead.Flink;
      while (ServiceEntry != &ServiceListHead)
      {
         CurrentService = CONTAINING_RECORD(ServiceEntry, SERVICE, ServiceListEntry);

         if ((RtlCompareUnicodeString(&CurrentGroup->GroupName,
              &CurrentService->ServiceGroup, TRUE) == 0) &&
	     (CurrentService->Start == 1 /*SERVICE_SYSTEM_START*/))
	 {
            DPRINT("  Path: %wZ\n", &CurrentService->RegistryPath);
            IopDisplayLoadingMessage(CurrentService->ServiceName.Buffer);
            Status = NtLoadDriver(&CurrentService->RegistryPath);
            if (!NT_SUCCESS(Status))
            {
               DPRINT("NtLoadDriver() failed (Status %lx)\n", Status);
#if 0
               if (CurrentService->ErrorControl == 1)
               {
                  /* Log error */
               } else
               if (CurrentService->ErrorControl == 2)
               {
                  if (IsLastKnownGood == FALSE)
                  {
                     /* Boot last known good configuration */
                  }
               } else
               if (CurrentService->ErrorControl == 3)
               {
                  if (IsLastKnownGood == FALSE)
                  {
                     /* Boot last known good configuration */
                  } else
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

   DPRINT("IopInitializeSystemDrivers() done\n");
}

/*
 * IopGetDriverNameFromServiceKey
 *
 * Returns a module path from service registry key.
 *
 * Parameters
 *    RelativeTo
 *       Relative path identifier.
 *    PathName
 *       Relative key path name.
 *    ImagePath
 *       The result path.
 *
 * Return Value
 *    Status
 */

NTSTATUS STDCALL
IopGetDriverNameFromServiceKey(
  ULONG RelativeTo,
  PWSTR PathName,
  PUNICODE_STRING ImagePath)
{
   RTL_QUERY_REGISTRY_TABLE QueryTable[2];
   UNICODE_STRING RegistryImagePath;
   NTSTATUS Status;
   PWSTR ServiceName;

   RtlZeroMemory(&QueryTable, sizeof(QueryTable));
   RtlInitUnicodeString(&RegistryImagePath, NULL);

   QueryTable[0].Name = L"ImagePath";
   QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
   QueryTable[0].EntryContext = &RegistryImagePath;

   Status = RtlQueryRegistryValues(RelativeTo,
      PathName, QueryTable, NULL, NULL);
   if (!NT_SUCCESS(Status))
   {
      DPRINT("RtlQueryRegistryValues() failed (Status %lx)\n", Status);
      RtlFreeUnicodeString(&RegistryImagePath);
      return STATUS_UNSUCCESSFUL;
   }

   if (RegistryImagePath.Length == 0)
   {
      ServiceName = wcsrchr(PathName, L'\\');
      if (ServiceName == NULL)
      {
         ServiceName = PathName;
      }
      else
      {
         ServiceName++;
      }
      
      ImagePath->Length = (33 + wcslen(ServiceName)) * sizeof(WCHAR);
      ImagePath->MaximumLength = ImagePath->Length + sizeof(UNICODE_NULL);
      ImagePath->Buffer = ExAllocatePool(NonPagedPool, ImagePath->MaximumLength);
      if (ImagePath->Buffer == NULL)
      {
         return STATUS_UNSUCCESSFUL;
      }
      wcscpy(ImagePath->Buffer, L"\\SystemRoot\\system32\\drivers\\");
      wcscat(ImagePath->Buffer, ServiceName);
      wcscat(ImagePath->Buffer, L".sys");
   } else
   if (RegistryImagePath.Buffer[0] != L'\\')
   {
      ImagePath->Length = (12 + wcslen(RegistryImagePath.Buffer)) * sizeof(WCHAR);
      ImagePath->MaximumLength = ImagePath->Length + sizeof(UNICODE_NULL);
      ImagePath->Buffer = ExAllocatePool(NonPagedPool, ImagePath->MaximumLength);
      if (ImagePath->Buffer == NULL)
      {
         RtlFreeUnicodeString(&RegistryImagePath);
         return STATUS_UNSUCCESSFUL;
      }
      wcscpy(ImagePath->Buffer, L"\\SystemRoot\\");
      wcscat(ImagePath->Buffer, RegistryImagePath.Buffer);
      RtlFreeUnicodeString(&RegistryImagePath);
   } else
   {
      ImagePath->Length = RegistryImagePath.Length;
      ImagePath->MaximumLength = RegistryImagePath.MaximumLength;
      ImagePath->Buffer = RegistryImagePath.Buffer;
   }

   return STATUS_SUCCESS;
}

/*
 * IopInitializeDeviceNodeService
 *
 * Initialize service for given device node.
 *
 * Parameters
 *    DeviceNode
 *       The device node to initialize service for.
 *    BootDriverOnly
 *       Initialize driver only if it's marked as boot start.
 *
 * Return Value
 *    Status
 */

NTSTATUS
IopInitializeDeviceNodeService(PDEVICE_NODE DeviceNode, BOOLEAN BootDriverOnly)
{
   NTSTATUS Status;
   ULONG ServiceStart;
   RTL_QUERY_REGISTRY_TABLE QueryTable[2];

   if (DeviceNode->ServiceName.Buffer == NULL)
   {
      return STATUS_UNSUCCESSFUL;
   }

   /*
    * Get service start value
    */

   RtlZeroMemory(QueryTable, sizeof(QueryTable));
   QueryTable[0].Name = L"Start";
   QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
   QueryTable[0].EntryContext = &ServiceStart;
   Status = RtlQueryRegistryValues(RTL_REGISTRY_SERVICES,
      DeviceNode->ServiceName.Buffer, QueryTable, NULL, NULL);
   if (!NT_SUCCESS(Status))
   {
      DPRINT("RtlQueryRegistryValues() failed (Status %x)\n", Status);
      return Status;
   }

   if (BootDriverOnly)
   {
      PLOADER_MODULE KeLoaderModules = (PLOADER_MODULE)KeLoaderBlock.ModsAddr;

      /*
       * Find and initialize boot driver
       */
      if (ServiceStart == 0 /*SERVICE_BOOT_START*/)
      {
         ULONG i;
         CHAR SearchName[256];
         ULONG ModuleStart, ModuleSize;
         PCHAR ModuleName;

         /* FIXME: Guard for buffer overflow */
         sprintf(SearchName, "%S.sys", DeviceNode->ServiceName.Buffer);
         for (i = 1; i < KeLoaderBlock.ModsCount; i++)
         {
            ModuleStart = KeLoaderModules[i].ModStart;
            ModuleSize = KeLoaderModules[i].ModEnd - ModuleStart;
            ModuleName = (PCHAR)KeLoaderModules[i].String;
            if (!strcmp(ModuleName, SearchName))
            {
               IopInitializeBuiltinDriver(DeviceNode,
                  (PVOID)ModuleStart, ModuleName, ModuleSize);
               /* Tell, that the module is already loaded */
               KeLoaderModules[i].Reserved = 1;
            }
         }
         return STATUS_SUCCESS;
      } else
      {
         return STATUS_UNSUCCESSFUL;
      }
   } else
   if (ServiceStart < 4)
   {
      UNICODE_STRING ImagePath;

      /*
       * Get service path
       */
      Status = IopGetDriverNameFromServiceKey(RTL_REGISTRY_SERVICES,
          DeviceNode->ServiceName.Buffer, &ImagePath);
      if (!NT_SUCCESS(Status))
      {
         DPRINT("IopGetDriverNameFromKeyNode() failed (Status %x)\n", Status);
         return Status;
      }

      /*
       * Display loading message
       */
      IopDisplayLoadingMessage(DeviceNode->ServiceName.Buffer);

      /*
       * Load the service
       */
      Status = IopInitializeService(DeviceNode, &ImagePath);

      /*
       * Free the service path
       */
      RtlFreeUnicodeString(&ImagePath);
   }
   else
      Status = STATUS_UNSUCCESSFUL;

   return Status;
}

/*
 * IopUnloadDriver
 *
 * Unloads a device driver.
 *
 * Parameters
 *    DriverServiceName
 *       Name of the service to unload (registry key).
 *    UnloadPnpDrivers
 *       Whether to unload Plug & Plug or only legacy drivers. If this
 *       parameter is set to FALSE, the routine will unload only legacy
 *       drivers.
 *		
 * Return Value
 *    Status
 *
 * To do
 *    Guard the whole function by SEH.
 */

NTSTATUS STDCALL
IopUnloadDriver(PUNICODE_STRING DriverServiceName, BOOLEAN UnloadPnpDrivers)
{
   UNICODE_STRING ImagePath;
   UNICODE_STRING ObjectName;
   PDRIVER_OBJECT DriverObject;
   PMODULE_OBJECT ModuleObject;
   NTSTATUS Status;
   LPWSTR Start;

   DPRINT("IopUnloadDriver('%wZ', %d)\n", DriverServiceName, UnloadPnpDrivers);

   /*
    * Get the service name from the module name
    */
   Start = wcsrchr(DriverServiceName->Buffer, L'\\');
   if (Start == NULL)
      Start = DriverServiceName->Buffer;
   else
      Start++;

   /*
    * Construct the driver object name
    */
   ObjectName.Length = (wcslen(Start) + 8) * sizeof(WCHAR);
   ObjectName.MaximumLength = ObjectName.Length + sizeof(WCHAR);
   ObjectName.Buffer = ExAllocatePool(NonPagedPool, ObjectName.MaximumLength);
   wcscpy(ObjectName.Buffer, L"\\Driver\\");
   memcpy(ObjectName.Buffer + 8, Start, (ObjectName.Length - 8) * sizeof(WCHAR));
   ObjectName.Buffer[ObjectName.Length/sizeof(WCHAR)] = 0;

   /*
    * Find the driver object
    */
   Status = ObReferenceObjectByName(&ObjectName, 0, 0, 0, IoDriverObjectType,
      KernelMode, 0, (PVOID*)&DriverObject);
   if (!NT_SUCCESS(Status))
   {
      DPRINT("Can't locate driver object for %wZ\n", ObjectName);
      return Status;
   }

   /*
    * Free the buffer for driver object name
    */
   ExFreePool(ObjectName.Buffer);

   /*
    * Get path of service...
    */
   Status = IopGetDriverNameFromServiceKey(RTL_REGISTRY_ABSOLUTE,
       DriverServiceName->Buffer, &ImagePath);
   if (!NT_SUCCESS(Status))
   {
      DPRINT("IopGetDriverNameFromKeyNode() failed (Status %x)\n", Status);
      return Status;
   }

   /*
    * ... and check if it's loaded
    */
   ModuleObject = LdrGetModuleObject(&ImagePath);
   if (ModuleObject == NULL)
   {
      return STATUS_UNSUCCESSFUL;
   }

   /*
    * Free the service path
    */
   RtlFreeUnicodeString(&ImagePath);

   /*
    * Unload the module and release the references to the device object
    */
   if (DriverObject->DriverUnload)
      (*DriverObject->DriverUnload)(DriverObject);
   ObDereferenceObject(DriverObject);
   ObDereferenceObject(DriverObject);
   LdrUnloadModule(ModuleObject);

   return STATUS_SUCCESS;
}

/* FUNCTIONS ******************************************************************/

/*
 * NtLoadDriver
 *
 * Loads a device driver.
 *
 * Parameters
 *    DriverServiceName
 *       Name of the service to load (registry key).
 *		
 * Return Value
 *    Status
 */

NTSTATUS STDCALL
NtLoadDriver(IN PUNICODE_STRING DriverServiceName)
{
   RTL_QUERY_REGISTRY_TABLE QueryTable[2];
   UNICODE_STRING ImagePath;
   NTSTATUS Status;
   ULONG Type;
   PDEVICE_NODE DeviceNode;
   PMODULE_OBJECT ModuleObject;
   LPWSTR Start;

   DPRINT("NtLoadDriver('%wZ')\n", DriverServiceName);

   /*
    * Check security privileges
    */
#if 0
   if (!SeSinglePrivilegeCheck(SeLoadDriverPrivilege, KeGetPreviousMode()))
      return STATUS_PRIVILEGE_NOT_HELD;
#endif

   RtlInitUnicodeString(&ImagePath, NULL);

   /*
    * Get service type
    */
   RtlZeroMemory(&QueryTable, sizeof(QueryTable));
   QueryTable[0].Name = L"Type";
   QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
   QueryTable[0].EntryContext = &Type;
   Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
      DriverServiceName->Buffer, QueryTable, NULL, NULL);
   if (!NT_SUCCESS(Status))
   {
      DPRINT("RtlQueryRegistryValues() failed (Status %lx)\n", Status);
      RtlFreeUnicodeString(&ImagePath);
      return Status;
   }

   /*
    * Get module path
    */
   Status = IopGetDriverNameFromServiceKey(RTL_REGISTRY_ABSOLUTE,
      DriverServiceName->Buffer, &ImagePath);
   if (!NT_SUCCESS(Status))
   {
      DPRINT("IopGetDriverNameFromKeyNode() failed (Status %x)\n", Status);
      return Status;
   }

   DPRINT("FullImagePath: '%S'\n", ImagePath.Buffer);
   DPRINT("Type: %lx\n", Type);

   /*
    * See, if the driver module isn't already loaded
    */
   ModuleObject = LdrGetModuleObject(&ImagePath);
   if (ModuleObject != NULL)
   {
      return STATUS_IMAGE_ALREADY_LOADED;
   }

   /*
    * Create device node
    */
   /* Use IopRootDeviceNode for now */
   Status = IopCreateDeviceNode(IopRootDeviceNode, NULL, &DeviceNode);
   if (!NT_SUCCESS(Status))
   {
      DPRINT("IopCreateDeviceNode() failed (Status %lx)\n", Status);
      return Status;
   }

   /*
    * Load the driver module
    */
   Status = LdrLoadModule(&ImagePath, &ModuleObject);
   if (!NT_SUCCESS(Status))
   {
      DPRINT("LdrLoadModule() failed (Status %lx)\n", Status);
      IopFreeDeviceNode(DeviceNode);
      return Status;
   }

   /*
    * Set a service name for the device node
    */
   Start = wcsrchr(DriverServiceName->Buffer, L'\\');
   if (Start == NULL)
      Start = DriverServiceName->Buffer;
   else
      Start++;
   RtlCreateUnicodeString(&DeviceNode->ServiceName, Start);

   /*
    * Initialize the driver module
    */
   Status = IopInitializeDriver(
      ModuleObject->EntryPoint,
      DeviceNode,
      (Type == 2 /*SERVICE_FILE_SYSTEM_DRIVER*/ ||
       Type == 8 /*SERVICE_RECOGNIZER_DRIVER*/),
      ModuleObject->Base,
      ModuleObject->Length,
      FALSE);
   if (!NT_SUCCESS(Status))
   {
      DPRINT1("IopInitializeDriver() failed (Status %lx)\n", Status);
      LdrUnloadModule(ModuleObject);
      IopFreeDeviceNode(DeviceNode);
   }

   return Status;
}

/*
 * NtUnloadDriver
 *
 * Unloads a legacy device driver.
 *
 * Parameters
 *    DriverServiceName
 *       Name of the service to unload (registry key).
 *		
 * Return Value
 *    Status
 */

NTSTATUS STDCALL
NtUnloadDriver(IN PUNICODE_STRING DriverServiceName)
{
   return IopUnloadDriver(DriverServiceName, FALSE);
}


/*
 * @implemented
 */
VOID STDCALL
IoRegisterDriverReinitialization(PDRIVER_OBJECT DriverObject,
				 PDRIVER_REINITIALIZE ReinitRoutine,
				 PVOID Context)
{
  PDRIVER_REINIT_ITEM ReinitItem;

  ReinitItem = ExAllocatePool(NonPagedPool,
			      sizeof(DRIVER_REINIT_ITEM));
  if (ReinitItem == NULL)
    return;

  ReinitItem->DriverObject = DriverObject;
  ReinitItem->ReinitRoutine = ReinitRoutine;
  ReinitItem->Context = Context;

  ExInterlockedInsertTailList(&DriverReinitListHead,
			      &ReinitItem->ItemEntry,
			      &DriverReinitListLock);
}


VOID
IopMarkLastReinitializeDriver(VOID)
{
  KIRQL Irql;

  KeAcquireSpinLock(&DriverReinitListLock,
		    &Irql);

  if (IsListEmpty(&DriverReinitListHead))
  {
    DriverReinitTailEntry = NULL;
  }
  else
  {
    DriverReinitTailEntry = DriverReinitListHead.Blink;
  }

  KeReleaseSpinLock(&DriverReinitListLock,
		    Irql);
}


VOID
IopReinitializeDrivers(VOID)
{
  PDRIVER_REINIT_ITEM ReinitItem;
  PLIST_ENTRY Entry;
  KIRQL Irql;

  KeAcquireSpinLock(&DriverReinitListLock,
		    &Irql);

  if (DriverReinitTailEntry == NULL)
  {
    KeReleaseSpinLock(&DriverReinitListLock,
		      Irql);
    return;
  }

  KeReleaseSpinLock(&DriverReinitListLock,
		    Irql);

  for (;;)
  {
    Entry = ExInterlockedRemoveHeadList(&DriverReinitListHead,
				        &DriverReinitListLock);
    if (Entry == NULL)
      return;

    ReinitItem = (PDRIVER_REINIT_ITEM)CONTAINING_RECORD(Entry, DRIVER_REINIT_ITEM, ItemEntry);

    /* Increment reinitialization counter */
    ReinitItem->DriverObject->DriverExtension->Count++;

    ReinitItem->ReinitRoutine(ReinitItem->DriverObject,
			      ReinitItem->Context,
			      ReinitItem->DriverObject->DriverExtension->Count);

    ExFreePool(Entry);

    if (Entry == DriverReinitTailEntry)
      return;
  }
}

typedef struct _PRIVATE_DRIVER_EXTENSIONS {
   struct _PRIVATE_DRIVER_EXTENSIONS *Link;
   PVOID ClientIdentificationAddress;
   CHAR Extension[1];
} PRIVATE_DRIVER_EXTENSIONS, *PPRIVATE_DRIVER_EXTENSIONS;

NTSTATUS STDCALL
IoAllocateDriverObjectExtension(
   PDRIVER_OBJECT DriverObject,
   PVOID ClientIdentificationAddress,
   ULONG DriverObjectExtensionSize,
   PVOID *DriverObjectExtension)
{
   KIRQL OldIrql;
   PPRIVATE_DRIVER_EXTENSIONS DriverExtensions;
   PPRIVATE_DRIVER_EXTENSIONS NewDriverExtension;

   NewDriverExtension = ExAllocatePoolWithTag(
      NonPagedPool,
      sizeof(PRIVATE_DRIVER_EXTENSIONS) - sizeof(CHAR) +
      DriverObjectExtensionSize,
      TAG_DRIVER_EXTENSION);

   if (NewDriverExtension == NULL)
   {
      return STATUS_INSUFFICIENT_RESOURCES;             
   }
   
   OldIrql = KeRaiseIrqlToDpcLevel();

   NewDriverExtension->Link = DriverObject->DriverSection;
   NewDriverExtension->ClientIdentificationAddress = ClientIdentificationAddress;

   for (DriverExtensions = DriverObject->DriverSection;
        DriverExtensions != NULL;
        DriverExtensions = DriverExtensions->Link)
   {
      if (DriverExtensions->ClientIdentificationAddress ==
          ClientIdentificationAddress)
         return STATUS_OBJECT_NAME_COLLISION;
   }

   DriverObject->DriverSection = NewDriverExtension;

   KfLowerIrql(OldIrql);

   *DriverObjectExtension = &NewDriverExtension->Extension;

   return STATUS_SUCCESS;      
}

PVOID STDCALL
IoGetDriverObjectExtension(
   PDRIVER_OBJECT DriverObject,
   PVOID ClientIdentificationAddress)
{
   KIRQL OldIrql;
   PPRIVATE_DRIVER_EXTENSIONS DriverExtensions;

   OldIrql = KeRaiseIrqlToDpcLevel();

   for (DriverExtensions = DriverObject->DriverSection;
        DriverExtensions != NULL &&
        DriverExtensions->ClientIdentificationAddress !=
          ClientIdentificationAddress;
        DriverExtensions = DriverExtensions->Link)
      ;

   KfLowerIrql(OldIrql);

   if (DriverExtensions == NULL)
      return NULL;

   return &DriverExtensions->Extension;
}

/* EOF */
