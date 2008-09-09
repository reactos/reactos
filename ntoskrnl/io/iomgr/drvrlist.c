/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/iomgr/drvrlist.c
 * PURPOSE:         Driver List support for Grouping, Tagging, Sorting, etc.
 * PROGRAMMERS:     <UNKNOWN>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

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
  UNICODE_STRING ImagePath;

  ULONG Start;
  ULONG Type;
  ULONG ErrorControl;
  ULONG Tag;

/*  BOOLEAN ServiceRunning;*/	// needed ??
} SERVICE, *PSERVICE;

#define TAG_RTLREGISTRY TAG('R', 'q', 'r', 'v')

/* GLOBALS ********************************************************************/

LIST_ENTRY GroupListHead = {NULL, NULL};
LIST_ENTRY ServiceListHead  = {NULL, NULL};
extern BOOLEAN NoGuiBoot;

VOID
FASTCALL
INIT_FUNCTION
IopDisplayLoadingMessage(PVOID ServiceName,
                         BOOLEAN Unicode);

/* PRIVATE FUNCTIONS **********************************************************/

static NTSTATUS STDCALL
IopGetGroupOrderList(PWSTR ValueName,
		     ULONG ValueType,
		     PVOID ValueData,
		     ULONG ValueLength,
		     PVOID Context,
		     PVOID EntryContext)
{
  PSERVICE_GROUP Group;

  DPRINT("IopGetGroupOrderList(%S, %x, 0x%p, %x, 0x%p, 0x%p)\n",
         ValueName, ValueType, ValueData, ValueLength, Context, EntryContext);

  if (ValueType == REG_BINARY &&
      ValueData != NULL &&
      ValueLength >= sizeof(ULONG) &&
      ValueLength >= (*(PULONG)ValueData + 1) * sizeof(ULONG))
    {
      Group = (PSERVICE_GROUP)Context;
      Group->TagCount = ((PULONG)ValueData)[0];
      if (Group->TagCount > 0)
        {
	  if (ValueLength >= (Group->TagCount + 1) * sizeof(ULONG))
            {
              Group->TagArray = ExAllocatePool(NonPagedPool, Group->TagCount * sizeof(ULONG));
	      if (Group->TagArray == NULL)
	        {
		  Group->TagCount = 0;
	          return STATUS_INSUFFICIENT_RESOURCES;
		}
	      memcpy(Group->TagArray, (PULONG)ValueData + 1, Group->TagCount * sizeof(ULONG));
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
IopCreateGroupListEntry(PWSTR ValueName,
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
      DPRINT("GroupName: '%S'\n", (PWCHAR)ValueData);

      Group = ExAllocatePool(NonPagedPool,
			     sizeof(SERVICE_GROUP));
      if (Group == NULL)
	{
	  return(STATUS_INSUFFICIENT_RESOURCES);
	}

      RtlZeroMemory(Group, sizeof(SERVICE_GROUP));

      if (!RtlCreateUnicodeString(&Group->GroupName, (PWSTR)ValueData))
	{
	  ExFreePool(Group);
	  return(STATUS_INSUFFICIENT_RESOURCES);
	}

      RtlZeroMemory(&QueryTable, sizeof(QueryTable));
      QueryTable[0].Name = (PWSTR)ValueData;
      QueryTable[0].QueryRoutine = IopGetGroupOrderList;

      Status = RtlQueryRegistryValues(RTL_REGISTRY_CONTROL,
				      L"GroupOrderList",
				      QueryTable,
				      (PVOID)Group,
				      NULL);
      DPRINT("%x %d %S\n", Status, Group->TagCount, (PWSTR)ValueData);

      InsertTailList(&GroupListHead,
		     &Group->GroupListEntry);
    }

  return(STATUS_SUCCESS);
}


static NTSTATUS STDCALL
IopCreateServiceListEntry(PUNICODE_STRING ServiceName)
{
  RTL_QUERY_REGISTRY_TABLE QueryTable[7];
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

  QueryTable[5].Name = L"Tag";
  QueryTable[5].Flags = RTL_QUERY_REGISTRY_DIRECT;
  QueryTable[5].EntryContext = &Service->Tag;

  Status = RtlQueryRegistryValues(RTL_REGISTRY_SERVICES,
				  ServiceName->Buffer,
				  QueryTable,
				  NULL,
				  NULL);
  if (!NT_SUCCESS(Status) || Service->Start > 1)
    {
      /*
       * If something goes wrong during RtlQueryRegistryValues
       * it'll just drop everything on the floor and return,
       * so you have to check if the buffers were filled.
       * Luckily we zerofilled the Service.
       */
      if (Service->ServiceGroup.Buffer)
        {
          ExFreePoolWithTag(Service->ServiceGroup.Buffer, TAG_RTLREGISTRY);
        }
      if (Service->ImagePath.Buffer)
        {
          ExFreePoolWithTag(Service->ImagePath.Buffer, TAG_RTLREGISTRY);
        }
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
  DPRINT("Start %lx  Type %lx  Tag %lx ErrorControl %lx\n",
	 Service->Start, Service->Type, Service->Tag, Service->ErrorControl);

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
  UNICODE_STRING ServicesKeyName = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\System\\CurrentControlSet\\Services");
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
  InitializeObjectAttributes(&ObjectAttributes,
			     &ServicesKeyName,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);

  Status = ZwOpenKey(&KeyHandle,
		     KEY_ENUMERATE_SUB_KEYS,
		     &ObjectAttributes);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  KeyInfoLength = sizeof(KEY_BASIC_INFORMATION) + MAX_PATH * sizeof(WCHAR);
  KeyInfo = ExAllocatePool(NonPagedPool, KeyInfoLength);
  if (KeyInfo == NULL)
    {
      ZwClose(KeyHandle);
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  Index = 0;
  while (TRUE)
    {
      Status = ZwEnumerateKey(KeyHandle,
			      Index,
			      KeyBasicInformation,
			      KeyInfo,
			      KeyInfoLength,
			      &ReturnedLength);
      if (NT_SUCCESS(Status))
	{
	  if (KeyInfo->NameLength < MAX_PATH * sizeof(WCHAR))
	    {

	      SubKeyName.Length = (USHORT)KeyInfo->NameLength;
	      SubKeyName.MaximumLength = (USHORT)KeyInfo->NameLength + sizeof(WCHAR);
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
  ZwClose(KeyHandle);

  DPRINT("IoCreateDriverList() done\n");

  return(STATUS_SUCCESS);
}

NTSTATUS INIT_FUNCTION
IoDestroyDriverList(VOID)
{
    PSERVICE_GROUP CurrentGroup;
    PSERVICE CurrentService;
    PLIST_ENTRY NextEntry, TempEntry;

    DPRINT("IoDestroyDriverList() called\n");

    /* Destroy the Group List */
    for (NextEntry = GroupListHead.Flink, TempEntry = NextEntry->Flink;
         NextEntry != &GroupListHead;
         NextEntry = TempEntry, TempEntry = NextEntry->Flink)
    {
        /* Get the entry */
        CurrentGroup = CONTAINING_RECORD(NextEntry,
                                         SERVICE_GROUP,
                                         GroupListEntry);

        /* Remove it from the list */
        RemoveEntryList(&CurrentGroup->GroupListEntry);

        /* Free buffers */
        ExFreePool(CurrentGroup->GroupName.Buffer);
        if (CurrentGroup->TagArray)
            ExFreePool(CurrentGroup->TagArray);
        ExFreePool(CurrentGroup);
    }

    /* Destroy the Service List */
    for (NextEntry = ServiceListHead.Flink, TempEntry = NextEntry->Flink;
         NextEntry != &ServiceListHead;
         NextEntry = TempEntry, TempEntry = NextEntry->Flink)
    {
        /* Get the entry */
        CurrentService = CONTAINING_RECORD(NextEntry,
                                           SERVICE,
                                           ServiceListEntry);

        /* Remove it from the list */
        RemoveEntryList(&CurrentService->ServiceListEntry);

        /* Free buffers */
        ExFreePool(CurrentService->ServiceName.Buffer);
        ExFreePool(CurrentService->RegistryPath.Buffer);
        if (CurrentService->ServiceGroup.Buffer)
            ExFreePool(CurrentService->ServiceGroup.Buffer);
        if (CurrentService->ImagePath.Buffer)
            ExFreePool(CurrentService->ImagePath.Buffer);
        ExFreePool(CurrentService);
    }

    DPRINT("IoDestroyDriverList() done\n");

    /* Return success */
    return STATUS_SUCCESS;
}

static INIT_FUNCTION NTSTATUS
IopLoadDriver(PSERVICE Service)
{
   NTSTATUS Status = STATUS_UNSUCCESSFUL;

   IopDisplayLoadingMessage(Service->ServiceName.Buffer, TRUE);
   Status = ZwLoadDriver(&Service->RegistryPath);
   IopBootLog(&Service->ImagePath, NT_SUCCESS(Status) ? TRUE : FALSE);
   if (!NT_SUCCESS(Status))
   {
      DPRINT("IopLoadDriver() failed (Status %lx)\n", Status);
#if 0
      if (Service->ErrorControl == 1)
      {
         /* Log error */
      }
      else if (Service->ErrorControl == 2)
      {
         if (IsLastKnownGood == FALSE)
         {
            /* Boot last known good configuration */
         }
      }
      else if (Service->ErrorControl == 3)
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
   return Status;
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
VOID
FASTCALL
IopInitializeSystemDrivers(VOID)
{
   PSERVICE_GROUP CurrentGroup;
   PSERVICE CurrentService;
   NTSTATUS Status;
   ULONG i;
   PLIST_ENTRY NextGroupEntry, NextServiceEntry;

   DPRINT("IopInitializeSystemDrivers()\n");

    /* Start looping */
    for (NextGroupEntry = GroupListHead.Flink;
         NextGroupEntry != &GroupListHead;
         NextGroupEntry = NextGroupEntry->Flink)
    {
        /* Get the entry */
        CurrentGroup = CONTAINING_RECORD(NextGroupEntry,
                                         SERVICE_GROUP,
                                         GroupListEntry);

        DPRINT("Group: %wZ\n", &CurrentGroup->GroupName);

        /* Load all drivers with a valid tag */
        for (i = 0; i < CurrentGroup->TagCount; i++)
        {
            /* Start looping */
            for (NextServiceEntry = ServiceListHead.Flink;
                 NextServiceEntry != &ServiceListHead;
                 NextServiceEntry = NextServiceEntry->Flink)
            {
                /* Get the entry */
                CurrentService = CONTAINING_RECORD(NextServiceEntry,
                                                   SERVICE,
                                                   ServiceListEntry);

                if ((!RtlCompareUnicodeString(&CurrentGroup->GroupName,
                                             &CurrentService->ServiceGroup,
                                             TRUE)) &&
	                (CurrentService->Start == SERVICE_SYSTEM_START) &&
		            (CurrentService->Tag == CurrentGroup->TagArray[i]))

                {
                    DPRINT("  Path: %wZ\n", &CurrentService->RegistryPath);
                    Status = IopLoadDriver(CurrentService);
                }
            }
        }

        /* Load all drivers without a tag or with an invalid tag */
        for (NextServiceEntry = ServiceListHead.Flink;
             NextServiceEntry != &ServiceListHead;
             NextServiceEntry = NextServiceEntry->Flink)
        {
            /* Get the entry */
            CurrentService = CONTAINING_RECORD(NextServiceEntry,
                                               SERVICE,
                                               ServiceListEntry);

            if ((!RtlCompareUnicodeString(&CurrentGroup->GroupName,
                                         &CurrentService->ServiceGroup,
                                         TRUE)) &&
	            (CurrentService->Start == SERVICE_SYSTEM_START))
            {
                for (i = 0; i < CurrentGroup->TagCount; i++)
                {
                    if (CurrentGroup->TagArray[i] == CurrentService->Tag)
                    {
                        break;
                    }
                }

                if (i >= CurrentGroup->TagCount)
                {
                    DPRINT("  Path: %wZ\n", &CurrentService->RegistryPath);
                    Status = IopLoadDriver(CurrentService);
                }
            }
        }
    }

    DPRINT("IopInitializeSystemDrivers() done\n");
}
