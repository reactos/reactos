/* $Id: registry.c,v 1.95 2003/05/13 21:28:26 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/cm/registry.c
 * PURPOSE:         Registry functions
 * PROGRAMMERS:     Rex Jolliff
 *                  Matt Pyne
 *                  Jean Michault
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

#include <ddk/ntddk.h>
#include <roscfg.h>
#include <limits.h>
#include <string.h>
#include <internal/pool.h>
#include <internal/registry.h>
#include <reactos/bugcodes.h>

#define NDEBUG
#include <internal/debug.h>

#include "cm.h"

/* GLOBALS ******************************************************************/

POBJECT_TYPE  CmiKeyType = NULL;
PREGISTRY_HIVE  CmiVolatileHive = NULL;
KSPIN_LOCK  CmiKeyListLock;

LIST_ENTRY CmiHiveListHead;
ERESOURCE CmiHiveListLock;

volatile BOOLEAN CmiHiveSyncEnabled = FALSE;
volatile BOOLEAN CmiHiveSyncPending = FALSE;
KDPC CmiHiveSyncDpc;
KTIMER CmiHiveSyncTimer;

static GENERIC_MAPPING CmiKeyMapping =
	{KEY_READ, KEY_WRITE, KEY_EXECUTE, KEY_ALL_ACCESS};



VOID
CmiCheckKey(BOOLEAN Verbose,
  HANDLE Key);

static NTSTATUS
CmiCreateCurrentControlSetLink(VOID);

static VOID STDCALL
CmiHiveSyncDpcRoutine(PKDPC Dpc,
		      PVOID DeferredContext,
		      PVOID SystemArgument1,
		      PVOID SystemArgument2);

/* FUNCTIONS ****************************************************************/

VOID
CmiCheckSubKeys(BOOLEAN Verbose,
  HANDLE Key)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  PKEY_NODE_INFORMATION KeyInfo;
  WCHAR KeyBuffer[MAX_PATH];
  UNICODE_STRING KeyPath;
  WCHAR Name[MAX_PATH];
  ULONG BufferSize;
  ULONG ResultSize;
  NTSTATUS Status;
  HANDLE SubKey;
  ULONG Index;

  Index = 0;
  while (TRUE)
    {
      BufferSize = sizeof(KEY_NODE_INFORMATION) + 4096;
      KeyInfo = ExAllocatePool(PagedPool, BufferSize);

      Status = NtEnumerateKey(Key,
			      Index,
			      KeyNodeInformation,
			      KeyInfo,
			      BufferSize,
			      &ResultSize);
      if (!NT_SUCCESS(Status))
	{
	  ExFreePool(KeyInfo);
	  if (Status == STATUS_NO_MORE_ENTRIES)
	    Status = STATUS_SUCCESS;
	  break;
	}

      wcsncpy(Name,
	      KeyInfo->Name,
	      KeyInfo->NameLength / sizeof(WCHAR));

      if (Verbose)
	{
	  DbgPrint("Key: %S\n", Name);
	}

      /* FIXME: Check info. */

      ExFreePool(KeyInfo);

      wcscpy(KeyBuffer, L"\\Registry\\");
      wcscat(KeyBuffer, Name);

      RtlInitUnicodeString(&KeyPath, KeyBuffer);

      InitializeObjectAttributes(&ObjectAttributes,
				 &KeyPath,
				 OBJ_CASE_INSENSITIVE,
				 NULL,
				 NULL);

      Status = NtOpenKey(&SubKey,
			 KEY_ALL_ACCESS,
			 &ObjectAttributes);

      assert(NT_SUCCESS(Status));

      CmiCheckKey(Verbose, SubKey);

      NtClose(SubKey);

      Index++;
    }

  assert(NT_SUCCESS(Status));
}


VOID
CmiCheckValues(BOOLEAN Verbose,
  HANDLE Key)
{
  PKEY_NODE_INFORMATION ValueInfo;
  WCHAR Name[MAX_PATH];
  ULONG BufferSize;
  ULONG ResultSize;
  NTSTATUS Status;
  ULONG Index;

  Index = 0;
  while (TRUE)
    {
      BufferSize = sizeof(KEY_NODE_INFORMATION) + 4096;
      ValueInfo = ExAllocatePool(PagedPool, BufferSize);

      Status = NtEnumerateValueKey(Key,
				   Index,
				   KeyNodeInformation,
				   ValueInfo,
				   BufferSize,
				   &ResultSize);
      if (!NT_SUCCESS(Status))
	{
	  ExFreePool(ValueInfo);
	  if (Status == STATUS_NO_MORE_ENTRIES)
	    Status = STATUS_SUCCESS;
	  break;
	}

      wcsncpy(Name,
	      ValueInfo->Name,
	      ValueInfo->NameLength / sizeof(WCHAR));

      if (Verbose)
	{
	  DbgPrint("Value: %S\n", Name);
	}

      /* FIXME: Check info. */

      ExFreePool(ValueInfo);

      Index++;
    }

  assert(NT_SUCCESS(Status));
}


VOID
CmiCheckKey(BOOLEAN Verbose,
  HANDLE Key)
{
  CmiCheckValues(Verbose, Key);
  CmiCheckSubKeys(Verbose, Key);
}


VOID
CmiCheckByName(BOOLEAN Verbose,
  PWSTR KeyName)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  WCHAR KeyPathBuffer[MAX_PATH];
  UNICODE_STRING KeyPath;
  NTSTATUS Status;
  HANDLE Key;

  wcscpy(KeyPathBuffer, L"\\Registry\\");
  wcscat(KeyPathBuffer, KeyName);

  RtlInitUnicodeString(&KeyPath, KeyPathBuffer);

  InitializeObjectAttributes(&ObjectAttributes,
		&KeyPath,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);

  Status = NtOpenKey(&Key,
		KEY_ALL_ACCESS,
		&ObjectAttributes);

  if (CHECKED)
    {
      if (!NT_SUCCESS(Status))
	{
          DbgPrint("KeyPath %wZ  Status: %.08x", KeyPath, Status);
          DbgPrint("KeyPath %S  Status: %.08x", KeyPath.Buffer, Status);
          assert(NT_SUCCESS(Status));
	}
    }

  CmiCheckKey(Verbose, Key);

  NtClose(Key);
}


VOID
CmiCheckRegistry(BOOLEAN Verbose)
{
  if (Verbose)
    DbgPrint("Checking registry internals\n");

  CmiCheckByName(Verbose, L"Machine");
  CmiCheckByName(Verbose, L"User");
}


VOID
CmInitializeRegistry(VOID)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING RootKeyName;
  PKEY_OBJECT RootKey;
  PKEY_OBJECT MachineKey;
  PKEY_OBJECT UserKey;
  HANDLE RootKeyHandle;
  HANDLE KeyHandle;
  NTSTATUS Status;

  /*  Initialize the Key object type  */
  CmiKeyType = ExAllocatePool(NonPagedPool, sizeof(OBJECT_TYPE));
  assert(CmiKeyType);
  CmiKeyType->TotalObjects = 0;
  CmiKeyType->TotalHandles = 0;
  CmiKeyType->MaxObjects = LONG_MAX;
  CmiKeyType->MaxHandles = LONG_MAX;
  CmiKeyType->PagedPoolCharge = 0;
  CmiKeyType->NonpagedPoolCharge = sizeof(KEY_OBJECT);
  CmiKeyType->Mapping = &CmiKeyMapping;
  CmiKeyType->Dump = NULL;
  CmiKeyType->Open = NULL;
  CmiKeyType->Close = NULL;
  CmiKeyType->Delete = CmiObjectDelete;
  CmiKeyType->Parse = CmiObjectParse;
  CmiKeyType->Security = CmiObjectSecurity;
  CmiKeyType->QueryName = NULL;
  CmiKeyType->OkayToClose = NULL;
  CmiKeyType->Create = CmiObjectCreate;
  CmiKeyType->DuplicationNotify = NULL;
  RtlInitUnicodeString(&CmiKeyType->TypeName, L"Key");

  /* Initialize the hive list */
  InitializeListHead(&CmiHiveListHead);
  ExInitializeResourceLite(&CmiHiveListLock);

  /*  Build volatile registry store  */
  Status = CmiCreateRegistryHive(NULL, &CmiVolatileHive, FALSE);
  assert(NT_SUCCESS(Status));

  /* Create '\Registry' key. */
  RtlInitUnicodeString(&RootKeyName, REG_ROOT_KEY_NAME);
  InitializeObjectAttributes(&ObjectAttributes, &RootKeyName, 0, NULL, NULL);
  Status = ObCreateObject(&RootKeyHandle,
		STANDARD_RIGHTS_REQUIRED,
		&ObjectAttributes,
		CmiKeyType,
		(PVOID *) &RootKey);
  assert(NT_SUCCESS(Status));
  Status = ObReferenceObjectByHandle(RootKeyHandle,
				     STANDARD_RIGHTS_REQUIRED,
				     CmiKeyType,
				     KernelMode,
				     (PVOID *)&RootKey,
				     NULL);
  assert(NT_SUCCESS(Status));
  RootKey->RegistryHive = CmiVolatileHive;
  RootKey->BlockOffset = CmiVolatileHive->HiveHeader->RootKeyCell;
  RootKey->KeyCell = CmiGetBlock(CmiVolatileHive, RootKey->BlockOffset, NULL);
  RootKey->Flags = 0;
  RootKey->NumberOfSubKeys = 0;
  RootKey->SubKeys = NULL;
  RootKey->SizeOfSubKeys = 0;
  RootKey->NameSize = strlen("Registry");
  RootKey->Name = ExAllocatePool(PagedPool, RootKey->NameSize);
  RtlCopyMemory(RootKey->Name, "Registry", RootKey->NameSize);

  KeInitializeSpinLock(&CmiKeyListLock);

  /* Create '\Registry\Machine' key. */
  Status = ObCreateObject(&KeyHandle,
			  STANDARD_RIGHTS_REQUIRED,
			  NULL,
			  CmiKeyType,
			  (PVOID*)&MachineKey);
  assert(NT_SUCCESS(Status));
  Status = CmiAddSubKey(CmiVolatileHive,
			RootKey,
			MachineKey,
			L"Machine",
			wcslen(L"Machine") * sizeof(WCHAR),
			0,
			NULL,
			0);
  assert(NT_SUCCESS(Status));
  MachineKey->RegistryHive = CmiVolatileHive;
  MachineKey->Flags = 0;
  MachineKey->NumberOfSubKeys = 0;
  MachineKey->SubKeys = NULL;
  MachineKey->SizeOfSubKeys = MachineKey->KeyCell->NumberOfSubKeys;
  MachineKey->NameSize = strlen("Machine");
  MachineKey->Name = ExAllocatePool(PagedPool, MachineKey->NameSize);
  RtlCopyMemory(MachineKey->Name, "Machine", MachineKey->NameSize);
  CmiAddKeyToList(RootKey, MachineKey);

  /* Create '\Registry\User' key. */
  Status = ObCreateObject(&KeyHandle,
			  STANDARD_RIGHTS_REQUIRED,
			  NULL,
			  CmiKeyType,
			  (PVOID*)&UserKey);
  assert(NT_SUCCESS(Status));
  Status = CmiAddSubKey(CmiVolatileHive,
			RootKey,
			UserKey,
			L"User",
			wcslen(L"User") * sizeof(WCHAR),
			0,
			NULL,
			0);
  assert(NT_SUCCESS(Status));
  UserKey->RegistryHive = CmiVolatileHive;
  UserKey->Flags = 0;
  UserKey->NumberOfSubKeys = 0;
  UserKey->SubKeys = NULL;
  UserKey->SizeOfSubKeys = UserKey->KeyCell->NumberOfSubKeys;
  UserKey->NameSize = strlen("User");
  UserKey->Name = ExAllocatePool(PagedPool, UserKey->NameSize);
  RtlCopyMemory(UserKey->Name, "User", UserKey->NameSize);
  CmiAddKeyToList(RootKey, UserKey);
}


VOID
CmInit2(PCHAR CommandLine)
{
  PCHAR p1, p2;
  ULONG PiceStart;
  NTSTATUS Status;

  /* FIXME: Store system start options */



  /* Create the 'CurrentControlSet' link. */
  Status = CmiCreateCurrentControlSetLink();
  if (!NT_SUCCESS(Status))
    {
      KeBugCheck(CONFIG_INITIALIZATION_FAILED);
    }

  /* Set PICE 'Start' value to 1, if PICE debugging is enabled */
  PiceStart = 4;
  p1 = (PCHAR)CommandLine;
  while (p1 && (p2 = strchr(p1, '/')))
    {
      p2++;
      if (_strnicmp(p2, "DEBUGPORT", 9) == 0)
	{
	  p2 += 9;
	  if (*p2 == '=')
	    {
	      p2++;
	      if (_strnicmp(p2, "PICE", 4) == 0)
		{
		  p2 += 4;
		  PiceStart = 1;
		}
	    }
	}
      p1 = p2;
    }

  Status = RtlWriteRegistryValue(RTL_REGISTRY_SERVICES,
				 L"\\Pice",
				 L"Start",
				 REG_DWORD,
				 &PiceStart,
				 sizeof(ULONG));
  if (!NT_SUCCESS(Status))
    {
      KeBugCheck(CONFIG_INITIALIZATION_FAILED);
    }
}


static NTSTATUS
CmiCreateCurrentControlSetLink(VOID)
{
  RTL_QUERY_REGISTRY_TABLE QueryTable[5];
  WCHAR TargetNameBuffer[80];
  ULONG TargetNameLength;
  UNICODE_STRING LinkName;
  UNICODE_STRING LinkValue;
  ULONG CurrentSet;
  ULONG DefaultSet;
  ULONG Failed;
  ULONG LastKnownGood;
  NTSTATUS Status;
  OBJECT_ATTRIBUTES ObjectAttributes;
  HANDLE KeyHandle;

  DPRINT("CmiCreateCurrentControlSetLink() called\n");

  RtlZeroMemory(&QueryTable, sizeof(QueryTable));

  QueryTable[0].Name = L"Current";
  QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
  QueryTable[0].EntryContext = &CurrentSet;

  QueryTable[1].Name = L"Default";
  QueryTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
  QueryTable[1].EntryContext = &DefaultSet;

  QueryTable[2].Name = L"Failed";
  QueryTable[2].Flags = RTL_QUERY_REGISTRY_DIRECT;
  QueryTable[2].EntryContext = &Failed;

  QueryTable[3].Name = L"LastKnownGood";
  QueryTable[3].Flags = RTL_QUERY_REGISTRY_DIRECT;
  QueryTable[3].EntryContext = &LastKnownGood;

  Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
				  L"\\Registry\\Machine\\SYSTEM\\Select",
				  QueryTable,
				  NULL,
				  NULL);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  DPRINT("Current %ld  Default %ld\n", CurrentSet, DefaultSet);

  swprintf(TargetNameBuffer,
	   L"\\Registry\\Machine\\SYSTEM\\ControlSet%03lu",
	   CurrentSet);
  TargetNameLength = wcslen(TargetNameBuffer) * sizeof(WCHAR);

  DPRINT("Link target '%S'\n", TargetNameBuffer);

  RtlInitUnicodeStringFromLiteral(&LinkName,
				  L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet");
  InitializeObjectAttributes(&ObjectAttributes,
			     &LinkName,
			     OBJ_CASE_INSENSITIVE | OBJ_OPENIF | OBJ_OPENLINK,
			     NULL,
			     NULL);
  Status = NtCreateKey(&KeyHandle,
		       KEY_ALL_ACCESS | KEY_CREATE_LINK,
		       &ObjectAttributes,
		       0,
		       NULL,
		       REG_OPTION_VOLATILE | REG_OPTION_CREATE_LINK,
		       NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("NtCreateKey() failed (Status %lx)\n", Status);
      return(Status);
    }

  RtlInitUnicodeStringFromLiteral(&LinkValue,
				  L"SymbolicLinkValue");
  Status = NtSetValueKey(KeyHandle,
			 &LinkValue,
			 0,
			 REG_LINK,
			 (PVOID)TargetNameBuffer,
			 TargetNameLength);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("NtSetValueKey() failed (Status %lx)\n", Status);
    }

  NtClose(KeyHandle);

  return(Status);
}


NTSTATUS
CmiConnectHive(PREGISTRY_HIVE RegistryHive,
	       PUNICODE_STRING KeyName)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING ParentKeyName;
  PKEY_OBJECT ParentKey;
  PKEY_OBJECT NewKey;
  HANDLE KeyHandle;
  NTSTATUS Status;
  PWSTR SubName;

  DPRINT("CmiConnectHive(%p, %wZ) called.\n",
	 RegistryHive, KeyName);

  SubName = wcsrchr (KeyName->Buffer, L'\\');
  if (SubName == NULL)
    {
      return STATUS_UNSUCCESSFUL;
    }

  ParentKeyName.Length = (USHORT)(SubName - KeyName->Buffer) * sizeof(WCHAR);
  ParentKeyName.MaximumLength = ParentKeyName.Length + sizeof(WCHAR);
  ParentKeyName.Buffer = ExAllocatePool (NonPagedPool,
					 ParentKeyName.MaximumLength);
  RtlCopyMemory (ParentKeyName.Buffer,
		 KeyName->Buffer,
		 ParentKeyName.Length);
  ParentKeyName.Buffer[ParentKeyName.Length / sizeof(WCHAR)] = 0;
  SubName++;

  Status = ObReferenceObjectByName (&ParentKeyName,
				    OBJ_CASE_INSENSITIVE,
				    NULL,
				    STANDARD_RIGHTS_REQUIRED,
				    CmiKeyType,
				    KernelMode,
				    NULL,
				    (PVOID*)&ParentKey);
  RtlFreeUnicodeString (&ParentKeyName);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1 ("ObReferenceObjectByName() failed (Status %lx)\n", Status);
      return Status;
    }

  InitializeObjectAttributes(&ObjectAttributes,
			     KeyName,
			     0,
			     NULL,
			     NULL);

  Status = ObCreateObject(&KeyHandle,
			  STANDARD_RIGHTS_REQUIRED,
			  &ObjectAttributes,
			  CmiKeyType,
			  (PVOID*)&NewKey);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1 ("ObCreateObject() failed (Status %lx)\n", Status);
      ObDereferenceObject (ParentKey);
      return Status;
    }

  NewKey->RegistryHive = RegistryHive;
  NewKey->BlockOffset = RegistryHive->HiveHeader->RootKeyCell;
  NewKey->KeyCell = CmiGetBlock(RegistryHive, NewKey->BlockOffset, NULL);
  NewKey->Flags = 0;
  NewKey->NumberOfSubKeys = 0;
  NewKey->SubKeys = ExAllocatePool(PagedPool,
  NewKey->KeyCell->NumberOfSubKeys * sizeof(ULONG));

  if ((NewKey->SubKeys == NULL) && (NewKey->KeyCell->NumberOfSubKeys != 0))
    {
      DPRINT("NumberOfSubKeys %d\n", NewKey->KeyCell->NumberOfSubKeys);
      NtClose(NewKey);
      ObDereferenceObject (ParentKey);
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  NewKey->SizeOfSubKeys = NewKey->KeyCell->NumberOfSubKeys;
  NewKey->NameSize = wcslen (SubName);
  NewKey->Name = ExAllocatePool(PagedPool, NewKey->NameSize);

  if ((NewKey->Name == NULL) && (NewKey->NameSize != 0))
    {
      DPRINT("NewKey->NameSize %d\n", NewKey->NameSize);
      if (NewKey->SubKeys != NULL)
	ExFreePool(NewKey->SubKeys);
      NtClose(KeyHandle);
      ObDereferenceObject (ParentKey);
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  wcstombs (NewKey->Name,
	    SubName,
	    NewKey->NameSize);

  CmiAddKeyToList (ParentKey, NewKey);
  ObDereferenceObject (ParentKey);

  VERIFY_KEY_OBJECT(NewKey);

  return(STATUS_SUCCESS);
}


static NTSTATUS
CmiInitializeSystemHive (PWSTR FileName,
			 PUNICODE_STRING KeyName)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING ControlSetKeyName;
  UNICODE_STRING ControlSetLinkName;
  UNICODE_STRING ControlSetValueName;
  HANDLE KeyHandle;
  NTSTATUS Status;
  PREGISTRY_HIVE RegistryHive;

  Status = CmiCreateRegistryHive (FileName,
				  &RegistryHive,
				  TRUE);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1 ("CmiCreateRegistryHive() failed (Status %lx)\n", Status);
      return Status;
    }

  Status = CmiConnectHive (RegistryHive,
			   KeyName);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1 ("CmiConnectHive() failed (Status %lx)\n", Status);
      CmiRemoveRegistryHive (RegistryHive);
      return Status;
    }

  /* Create 'ControlSet001' key */
  RtlInitUnicodeStringFromLiteral (&ControlSetKeyName,
				   L"\\Registry\\Machine\\SYSTEM\\ControlSet001");
  InitializeObjectAttributes (&ObjectAttributes,
			      &ControlSetKeyName,
			      OBJ_CASE_INSENSITIVE,
			      NULL,
			      NULL);
  Status = NtCreateKey (&KeyHandle,
			KEY_ALL_ACCESS,
			&ObjectAttributes,
			0,
			NULL,
			REG_OPTION_NON_VOLATILE,
			NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1 ("NtCreateKey() failed (Status %lx)\n", Status);
      return Status;
    }
  NtClose (KeyHandle);

  /* Link 'CurrentControlSet' to 'ControlSet001' key */
  RtlInitUnicodeStringFromLiteral (&ControlSetLinkName,
				   L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet");
  InitializeObjectAttributes (&ObjectAttributes,
			      &ControlSetLinkName,
			      OBJ_CASE_INSENSITIVE | OBJ_OPENIF | OBJ_OPENLINK,
			      NULL,
			      NULL);
  Status = NtCreateKey (&KeyHandle,
			KEY_ALL_ACCESS | KEY_CREATE_LINK,
			&ObjectAttributes,
			0,
			NULL,
			REG_OPTION_VOLATILE | REG_OPTION_CREATE_LINK,
			NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1 ("NtCreateKey() failed (Status %lx)\n", Status);
      return Status;
    }

  RtlInitUnicodeStringFromLiteral (&ControlSetValueName,
				   L"SymbolicLinkValue");
  Status = NtSetValueKey (KeyHandle,
			  &ControlSetValueName,
			  0,
			  REG_LINK,
			  (PVOID)ControlSetKeyName.Buffer,
			  ControlSetKeyName.Length);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1 ("NtSetValueKey() failed (Status %lx)\n", Status);
    }
  NtClose (KeyHandle);

  return STATUS_SUCCESS;
}


NTSTATUS
CmiInitializeHive(PWSTR FileName,
		  PUNICODE_STRING KeyName,
		  BOOLEAN CreateNew)
{
  PREGISTRY_HIVE RegistryHive;
  NTSTATUS Status;

  DPRINT("CmiInitializeHive(%s) called\n", KeyName);

  Status = CmiCreateRegistryHive(FileName,
				 &RegistryHive,
				 CreateNew);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("CmiCreateRegistryHive() failed (Status %lx)\n", Status);

      /* FIXME: Try to load the backup hive */

      KeBugCheck(0);
      return(Status);
    }

  /* Connect the hive */
  Status = CmiConnectHive(RegistryHive,
			  KeyName);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("CmiConnectHive() failed (Status %lx)\n", Status);
      CmiRemoveRegistryHive(RegistryHive);
      return(Status);
    }

  DPRINT("CmiInitializeHive() done\n");

  return(STATUS_SUCCESS);
}


NTSTATUS
CmiInitHives(BOOLEAN SetupBoot)
{
  PKEY_VALUE_PARTIAL_INFORMATION ValueInfo;
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING KeyName;
  UNICODE_STRING ValueName;
  HANDLE KeyHandle;

  NTSTATUS Status;

  WCHAR ConfigPath[MAX_PATH];

  ULONG BufferSize;
  ULONG ResultSize;
  PWSTR EndPtr;


  DPRINT("CmiInitHives() called\n");

  if (SetupBoot == TRUE)
    {
      RtlInitUnicodeStringFromLiteral(&KeyName,
				      L"\\Registry\\Machine\\HARDWARE");
      InitializeObjectAttributes(&ObjectAttributes,
				 &KeyName,
				 OBJ_CASE_INSENSITIVE,
				 NULL,
				 NULL);
      Status =  NtOpenKey(&KeyHandle,
			  KEY_ALL_ACCESS,
			  &ObjectAttributes);
      if (!NT_SUCCESS(Status))
	{
	  DPRINT1("NtOpenKey() failed (Status %lx)\n", Status);
	  return(Status);
	}

      RtlInitUnicodeStringFromLiteral(&ValueName,
				      L"InstallPath");

      BufferSize = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 4096;
      ValueInfo = ExAllocatePool(PagedPool,
				 BufferSize);
      if (ValueInfo == NULL)
	{
	  NtClose(KeyHandle);
	  return(STATUS_INSUFFICIENT_RESOURCES);
	}

      Status = NtQueryValueKey(KeyHandle,
			       &ValueName,
			       KeyValuePartialInformation,
			       ValueInfo,
			       BufferSize,
			       &ResultSize);
      NtClose(KeyHandle);
      if (!NT_SUCCESS(Status))
	{
	  ExFreePool(ValueInfo);
	  return(Status);
	}

      RtlCopyMemory(ConfigPath,
		    ValueInfo->Data,
		    ValueInfo->DataLength);
      ConfigPath[ValueInfo->DataLength / sizeof(WCHAR)] = (WCHAR)0;
      ExFreePool(ValueInfo);
    }
  else
    {
      wcscpy(ConfigPath, L"\\SystemRoot");
    }
  wcscat(ConfigPath, L"\\system32\\config");

  DPRINT("ConfigPath: %S\n", ConfigPath);

  EndPtr = ConfigPath + wcslen(ConfigPath);

  CmiDoVerify = TRUE;

  /* FIXME: Save boot log */

  /* Connect the SYSTEM hive only if it has been created */
  if (SetupBoot == TRUE)
    {
      wcscpy(EndPtr, REG_SYSTEM_FILE_NAME);
      DPRINT ("ConfigPath: %S\n", ConfigPath);

      RtlInitUnicodeString (&KeyName,
			    REG_SYSTEM_KEY_NAME);
      Status = CmiInitializeSystemHive (ConfigPath,
					&KeyName);
      if (!NT_SUCCESS(Status))
	{
	  DPRINT1("CmiInitializeSystemHive() failed (Status %lx)\n", Status);
	  return(Status);
	}
    }

  /* Connect the SOFTWARE hive */
  wcscpy(EndPtr, REG_SOFTWARE_FILE_NAME);
  DPRINT ("ConfigPath: %S\n", ConfigPath);

  RtlInitUnicodeString (&KeyName,
			REG_SOFTWARE_KEY_NAME);
  Status = CmiInitializeHive(ConfigPath,
			     &KeyName,
			     SetupBoot);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("CmiInitializeHive() failed (Status %lx)\n", Status);
      return(Status);
    }

  /* Connect the SAM hive */
  wcscpy(EndPtr, REG_SAM_FILE_NAME);
  DPRINT ("ConfigPath: %S\n", ConfigPath);

  RtlInitUnicodeString (&KeyName,
			REG_SAM_KEY_NAME);
  Status = CmiInitializeHive(ConfigPath,
			     &KeyName,
			     SetupBoot);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("CmiInitializeHive() failed (Status %lx)\n", Status);
      return(Status);
    }

  /* Connect the SECURITY hive */
  wcscpy(EndPtr, REG_SEC_FILE_NAME);
  DPRINT ("ConfigPath: %S\n", ConfigPath);

  RtlInitUnicodeString (&KeyName,
			REG_SEC_KEY_NAME);
  Status = CmiInitializeHive(ConfigPath,
			     &KeyName,
			     SetupBoot);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("CmiInitializeHive() failed (Status %lx)\n", Status);
      return(Status);
    }

  /* Connect the DEFAULT hive */
  wcscpy(EndPtr, REG_USER_FILE_NAME);
  DPRINT ("ConfigPath: %S\n", ConfigPath);

  RtlInitUnicodeString (&KeyName,
			REG_USER_KEY_NAME);
  Status = CmiInitializeHive(ConfigPath,
			     &KeyName,
			     SetupBoot);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("CmiInitializeHive() failed (Status %lx)\n", Status);
      return(Status);
    }

//  CmiCheckRegistry(TRUE);

  /* Start automatic hive synchronization */
  KeInitializeDpc(&CmiHiveSyncDpc,
		  CmiHiveSyncDpcRoutine,
		  NULL);
  KeInitializeTimer(&CmiHiveSyncTimer);
  CmiHiveSyncEnabled = TRUE;

  DPRINT("CmiInitHives() done\n");

  return(STATUS_SUCCESS);
}


VOID
CmShutdownRegistry(VOID)
{
  PREGISTRY_HIVE Hive;
  PLIST_ENTRY Entry;

  DPRINT1("CmShutdownRegistry() called\n");

  /* Stop automatic hive synchronization */
  CmiHiveSyncEnabled = FALSE;

  /* Acquire hive list lock exclusively */
  ExAcquireResourceExclusiveLite(&CmiHiveListLock, TRUE);

  Entry = CmiHiveListHead.Flink;
  while (Entry != &CmiHiveListHead)
    {
      Hive = CONTAINING_RECORD(Entry, REGISTRY_HIVE, HiveList);

      if (!IsVolatileHive(Hive))
	{
	  /* Acquire hive resource exclusively */
	  ExAcquireResourceExclusiveLite(&Hive->HiveResource,
					 TRUE);

	  /* Flush non-volatile hive */
	  CmiFlushRegistryHive(Hive);

	  /* Release hive resource */
	  ExReleaseResourceLite(&Hive->HiveResource);
	}

      Entry = Entry->Flink;
    }

  /* Release hive list lock */
  ExReleaseResourceLite(&CmiHiveListLock);

  DPRINT1("CmShutdownRegistry() done\n");
}


VOID STDCALL
CmiHiveSyncRoutine(PVOID DeferredContext)
{
  PREGISTRY_HIVE Hive;
  PLIST_ENTRY Entry;

  DPRINT("CmiHiveSyncRoutine() called\n");

  CmiHiveSyncPending = FALSE;

  /* Acquire hive list lock exclusively */
  ExAcquireResourceExclusiveLite(&CmiHiveListLock, TRUE);

  Entry = CmiHiveListHead.Flink;
  while (Entry != &CmiHiveListHead)
    {
      Hive = CONTAINING_RECORD(Entry, REGISTRY_HIVE, HiveList);

      if (!IsVolatileHive(Hive))
	{
	  /* Acquire hive resource exclusively */
	  ExAcquireResourceExclusiveLite(&Hive->HiveResource,
					 TRUE);

	  /* Flush non-volatile hive */
	  CmiFlushRegistryHive(Hive);

	  /* Release hive resource */
	  ExReleaseResourceLite(&Hive->HiveResource);
	}

      Entry = Entry->Flink;
    }

  /* Release hive list lock */
  ExReleaseResourceLite(&CmiHiveListLock);

  DPRINT("DeferredContext %x\n", DeferredContext);
  ExFreePool(DeferredContext);

  DPRINT("CmiHiveSyncRoutine() done\n");
}


static VOID STDCALL
CmiHiveSyncDpcRoutine(PKDPC Dpc,
		      PVOID DeferredContext,
		      PVOID SystemArgument1,
		      PVOID SystemArgument2)
{
  PWORK_QUEUE_ITEM WorkQueueItem;

  WorkQueueItem = ExAllocatePool(NonPagedPool,
				 sizeof(WORK_QUEUE_ITEM));
  if (WorkQueueItem == NULL)
    {
      DbgPrint("Failed to allocate work item\n");
      return;
    }

  ExInitializeWorkItem(WorkQueueItem,
		       CmiHiveSyncRoutine,
		       WorkQueueItem);

  DPRINT("DeferredContext %x\n", WorkQueueItem);
  ExQueueWorkItem(WorkQueueItem,
		  CriticalWorkQueue);
}


VOID
CmiSyncHives(VOID)
{
  LARGE_INTEGER Timeout;

  DPRINT("CmiSyncHives() called\n");

  if (CmiHiveSyncEnabled == FALSE ||
      CmiHiveSyncPending == TRUE)
    return;

  CmiHiveSyncPending = TRUE;


  Timeout.QuadPart = -50000000LL;
  KeSetTimer(&CmiHiveSyncTimer,
	     Timeout,
	     &CmiHiveSyncDpc);
}

/* EOF */
