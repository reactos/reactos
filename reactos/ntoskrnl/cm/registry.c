/* $Id: registry.c,v 1.89 2003/04/01 19:01:58 ekohl Exp $
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

#ifdef WIN32_REGDBG
#include "cm_win32.h"
#else
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
#endif

/*  -------------------------------------------------  File Statics  */

POBJECT_TYPE  CmiKeyType = NULL;
PREGISTRY_HIVE  CmiVolatileHive = NULL;
KSPIN_LOCK  CmiKeyListLock;

LIST_ENTRY CmiHiveListHead;
ERESOURCE CmiHiveListLock;

volatile BOOLEAN CmiHiveSyncEnabled = FALSE;
volatile BOOLEAN CmiHiveSyncPending = FALSE;
KDPC CmiHiveSyncDpc;
KTIMER CmiHiveSyncTimer;

static PKEY_OBJECT  CmiRootKey = NULL;
PKEY_OBJECT  CmiMachineKey = NULL;
static PKEY_OBJECT  CmiUserKey = NULL;
static PKEY_OBJECT  CmiHardwareKey = NULL;

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
  HANDLE RootKeyHandle;
  PKEY_OBJECT NewKey;
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
		(PVOID *) &NewKey);
  assert(NT_SUCCESS(Status));
  CmiRootKey = NewKey;
  Status = ObReferenceObjectByHandle(RootKeyHandle,
    STANDARD_RIGHTS_REQUIRED,
		CmiKeyType,
		KernelMode,
		(PVOID *) &CmiRootKey,
		NULL);
  assert(NT_SUCCESS(Status));
  CmiRootKey->RegistryHive = CmiVolatileHive;
  NewKey->BlockOffset = CmiVolatileHive->HiveHeader->RootKeyCell;
  NewKey->KeyCell = CmiGetBlock(CmiVolatileHive, NewKey->BlockOffset, NULL);
  CmiRootKey->Flags = 0;
  CmiRootKey->NumberOfSubKeys = 0;
  CmiRootKey->SubKeys = NULL;
  CmiRootKey->SizeOfSubKeys = 0;
  CmiRootKey->Name = ExAllocatePool(PagedPool, strlen("Registry"));
  CmiRootKey->NameSize = strlen("Registry");
  memcpy(CmiRootKey->Name, "Registry", strlen("Registry"));

  KeInitializeSpinLock(&CmiKeyListLock);

  /* Create '\Registry\Machine' key. */
  Status = ObCreateObject(&KeyHandle,
    STANDARD_RIGHTS_REQUIRED,
    NULL,
    CmiKeyType,
    (PVOID*) &NewKey);
  assert(NT_SUCCESS(Status));
  Status = CmiAddSubKey(CmiVolatileHive,
		CmiRootKey,
		NewKey,
		L"Machine",
		wcslen(L"Machine") * sizeof(WCHAR),
		0,
		NULL,
		0);
  assert(NT_SUCCESS(Status));
	NewKey->RegistryHive = CmiVolatileHive;
	NewKey->Flags = 0;
	NewKey->NumberOfSubKeys = 0;
	NewKey->SubKeys = NULL;
	NewKey->SizeOfSubKeys = NewKey->KeyCell->NumberOfSubKeys;
	NewKey->Name = ExAllocatePool(PagedPool, strlen("Machine"));
	NewKey->NameSize = strlen("Machine");
	memcpy(NewKey->Name, "Machine", strlen("Machine"));
  CmiAddKeyToList(CmiRootKey, NewKey);
  CmiMachineKey = NewKey;

  /* Create '\Registry\User' key. */
  Status = ObCreateObject(&KeyHandle,
		STANDARD_RIGHTS_REQUIRED,
		NULL,
		CmiKeyType,
		(PVOID*) &NewKey);
  assert(NT_SUCCESS(Status));
  Status = CmiAddSubKey(CmiVolatileHive,
    CmiRootKey,
    NewKey,
    L"User",
    wcslen(L"User") * sizeof(WCHAR),
    0,
    NULL,
    0);
  assert(NT_SUCCESS(Status));
	NewKey->RegistryHive = CmiVolatileHive;
	NewKey->Flags = 0;
	NewKey->NumberOfSubKeys = 0;
	NewKey->SubKeys = NULL;
	NewKey->SizeOfSubKeys = NewKey->KeyCell->NumberOfSubKeys;
	NewKey->Name = ExAllocatePool(PagedPool, strlen("User"));
	NewKey->NameSize = strlen("User");
	memcpy(NewKey->Name, "User", strlen("User"));
	CmiAddKeyToList(CmiRootKey, NewKey);
	CmiUserKey = NewKey;

  /* Create '\Registry\Machine\HARDWARE' key. */
  Status = ObCreateObject(&KeyHandle,
		STANDARD_RIGHTS_REQUIRED,
		NULL,
		CmiKeyType,
		(PVOID*)&NewKey);
  assert(NT_SUCCESS(Status));
  Status = CmiAddSubKey(CmiVolatileHive,
    CmiMachineKey,
    NewKey,
    L"HARDWARE",
    wcslen(L"HARDWARE") * sizeof(WCHAR),
    0,
    NULL,
    0);
  assert(NT_SUCCESS(Status));
  NewKey->RegistryHive = CmiVolatileHive;
  NewKey->Flags = 0;
  NewKey->NumberOfSubKeys = 0;
  NewKey->SubKeys = NULL;
  NewKey->SizeOfSubKeys = NewKey->KeyCell->NumberOfSubKeys;
  NewKey->Name = ExAllocatePool(PagedPool, strlen("HARDWARE"));
  NewKey->NameSize = strlen("HARDWARE");
  memcpy(NewKey->Name, "HARDWARE", strlen("HARDWARE"));
  CmiAddKeyToList(CmiMachineKey, NewKey);
  CmiHardwareKey = NewKey;

  /* Create '\Registry\Machine\HARDWARE\DESCRIPTION' key. */
  Status = ObCreateObject(&KeyHandle,
		STANDARD_RIGHTS_REQUIRED,
		NULL,
		CmiKeyType,
		(PVOID*) &NewKey);
  assert(NT_SUCCESS(Status));
  Status = CmiAddSubKey(CmiVolatileHive,
		CmiHardwareKey,
		NewKey,
		L"DESCRIPTION",
		wcslen(L"DESCRIPTION") * sizeof(WCHAR),
		0,
		NULL,
		0);
  assert(NT_SUCCESS(Status));
  NewKey->RegistryHive = CmiVolatileHive;
  NewKey->Flags = 0;
  NewKey->NumberOfSubKeys = 0;
  NewKey->SubKeys = NULL;
  NewKey->SizeOfSubKeys = NewKey->KeyCell->NumberOfSubKeys;
  NewKey->Name = ExAllocatePool(PagedPool, strlen("DESCRIPTION"));
  NewKey->NameSize = strlen("DESCRIPTION");
  memcpy(NewKey->Name, "DESCRIPTION", strlen("DESCRIPTION"));
  CmiAddKeyToList(CmiHardwareKey, NewKey);

  /* Create '\Registry\Machine\HARDWARE\DEVICEMAP' key. */
  Status = ObCreateObject(&KeyHandle,
		STANDARD_RIGHTS_REQUIRED,
		NULL,
		CmiKeyType,
		(PVOID*) &NewKey);
  assert(NT_SUCCESS(Status));
  Status = CmiAddSubKey(CmiVolatileHive,
    CmiHardwareKey,
    NewKey,
    L"DEVICEMAP",
		wcslen(L"DEVICEMAP") * sizeof(WCHAR),
    0,
    NULL,
    0);
  assert(NT_SUCCESS(Status));
  NewKey->RegistryHive = CmiVolatileHive;
  NewKey->Flags = 0;
  NewKey->NumberOfSubKeys = 0;
  NewKey->SubKeys = NULL;
  NewKey->SizeOfSubKeys = NewKey->KeyCell->NumberOfSubKeys;
  NewKey->Name = ExAllocatePool(PagedPool, strlen("DEVICEMAP"));
  NewKey->NameSize = strlen("DEVICEMAP");
  memcpy(NewKey->Name, "DEVICEMAP", strlen("DEVICEMAP"));
  CmiAddKeyToList(CmiHardwareKey,NewKey);

  /* Create '\Registry\Machine\HARDWARE\RESOURCEMAP' key. */
  Status = ObCreateObject(&KeyHandle,
		STANDARD_RIGHTS_REQUIRED,
		NULL,
		CmiKeyType,
		(PVOID*) &NewKey);
  assert(NT_SUCCESS(Status));
  Status = CmiAddSubKey(CmiVolatileHive,
		CmiHardwareKey,
		NewKey,
		L"RESOURCEMAP",
		wcslen(L"RESOURCEMAP") * sizeof(WCHAR),
		0,
		NULL,
		0);
  assert(NT_SUCCESS(Status));
  NewKey->RegistryHive = CmiVolatileHive;
  NewKey->Flags = 0;
  NewKey->NumberOfSubKeys = 0;
  NewKey->SubKeys = NULL;
  NewKey->SizeOfSubKeys = NewKey->KeyCell->NumberOfSubKeys;
  NewKey->Name = ExAllocatePool(PagedPool, strlen("RESOURCEMAP"));
  NewKey->NameSize = strlen("RESOURCEMAP");
  memcpy(NewKey->Name, "RESOURCEMAP", strlen("RESOURCEMAP"));
  CmiAddKeyToList(CmiHardwareKey, NewKey);
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
#ifndef WIN32_REGDBG
  if (!NT_SUCCESS(Status))
    {
      KeBugCheck(CONFIG_INITIALIZATION_FAILED);
    }
#endif

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
#ifndef WIN32_REGDBG
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
#endif
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
	       PUNICODE_STRING FullName,
	       PCHAR KeyName,
	       PKEY_OBJECT Parent)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  PKEY_OBJECT NewKey;
  HANDLE KeyHandle;
  NTSTATUS Status;

  DPRINT("CmiConnectHive(%p, %wZ, %s, %p) called.\n",
	 RegistryHive, FullName, KeyName, Parent);

  InitializeObjectAttributes(&ObjectAttributes,
			     FullName,
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
      DPRINT1("ObCreateObject() failed (Status %lx)\n", Status);
      KeBugCheck(0);
      return(Status);
    }

  NewKey->RegistryHive = RegistryHive;
  NewKey->BlockOffset = RegistryHive->HiveHeader->RootKeyCell;
  NewKey->KeyCell = CmiGetBlock(RegistryHive, NewKey->BlockOffset, NULL);
  NewKey->Flags = 0;
  NewKey->NumberOfSubKeys = 0;
  NewKey->SubKeys = ExAllocatePool(PagedPool,
  NewKey->KeyCell->NumberOfSubKeys * sizeof(DWORD));

  if ((NewKey->SubKeys == NULL) && (NewKey->KeyCell->NumberOfSubKeys != 0))
    {
      DPRINT("NumberOfSubKeys %d\n", NewKey->KeyCell->NumberOfSubKeys);
      ZwClose(NewKey);
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  NewKey->SizeOfSubKeys = NewKey->KeyCell->NumberOfSubKeys;
  NewKey->Name = ExAllocatePool(PagedPool, strlen(KeyName));

  if ((NewKey->Name == NULL) && (strlen(KeyName) != 0))
    {
      DPRINT("strlen(KeyName) %d\n", strlen(KeyName));
      if (NewKey->SubKeys != NULL)
	ExFreePool(NewKey->SubKeys);
      ZwClose(NewKey);
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  NewKey->NameSize = strlen(KeyName);
  memcpy(NewKey->Name, KeyName, strlen(KeyName));
  CmiAddKeyToList(Parent, NewKey);

  VERIFY_KEY_OBJECT(NewKey);

  return(STATUS_SUCCESS);
}


static NTSTATUS
CmiInitializeSystemHive(PWSTR FileName,
			PUNICODE_STRING FullName,
			PCHAR KeyName,
			PKEY_OBJECT Parent,
			BOOLEAN CreateNew)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING ControlSetKeyName;
  UNICODE_STRING ControlSetLinkName;
  UNICODE_STRING ControlSetValueName;
  HANDLE KeyHandle;
  NTSTATUS Status;
  PREGISTRY_HIVE RegistryHive;

  if (CreateNew == TRUE)
    {
      Status = CmiCreateRegistryHive(FileName,
				     &RegistryHive,
				     TRUE);
      if (!NT_SUCCESS(Status))
	{
	  DPRINT1("CmiCreateRegistryHive() failed (Status %lx)\n", Status);
	  KeBugCheck(0);
	  return(Status);
	}

      Status = CmiConnectHive(RegistryHive,
			      FullName,
			      KeyName,
			      Parent);
      if (!NT_SUCCESS(Status))
	{
	  DPRINT1("CmiConnectHive() failed (Status %lx)\n", Status);
	  CmiRemoveRegistryHive(RegistryHive);
	  return(Status);
	}

      /* Create 'ControlSet001' key */
      RtlInitUnicodeStringFromLiteral(&ControlSetKeyName,
				      L"\\Registry\\Machine\\SYSTEM\\ControlSet001");
      InitializeObjectAttributes(&ObjectAttributes,
				 &ControlSetKeyName,
				 OBJ_CASE_INSENSITIVE,
				 NULL,
				 NULL);
      Status = NtCreateKey(&KeyHandle,
			   KEY_ALL_ACCESS,
			   &ObjectAttributes,
			   0,
			   NULL,
			   REG_OPTION_NON_VOLATILE,
			   NULL);
      if (!NT_SUCCESS(Status))
	{
	  DPRINT1("NtCreateKey() failed (Status %lx)\n", Status);
	  return(Status);
	}
      NtClose(KeyHandle);

      /* Link 'CurrentControlSet' to 'ControlSet001' key */
      RtlInitUnicodeStringFromLiteral(&ControlSetLinkName,
				      L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet");
      InitializeObjectAttributes(&ObjectAttributes,
				 &ControlSetLinkName,
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

      RtlInitUnicodeStringFromLiteral(&ControlSetValueName,
				      L"SymbolicLinkValue");
      Status = NtSetValueKey(KeyHandle,
			     &ControlSetValueName,
			     0,
			     REG_LINK,
			     (PVOID)ControlSetKeyName.Buffer,
			     ControlSetKeyName.Length);
      if (!NT_SUCCESS(Status))
	{
	  DPRINT1("NtSetValueKey() failed (Status %lx)\n", Status);
	}
      NtClose(KeyHandle);
    }

  return(STATUS_SUCCESS);
}


NTSTATUS
CmiInitializeHive(PWSTR FileName,
		  PUNICODE_STRING FullName,
		  PCHAR KeyName,
		  PKEY_OBJECT Parent,
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
			  FullName,
			  KeyName,
			  Parent);
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
CmiInitHives(BOOLEAN SetUpBoot)
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

  UNICODE_STRING ParentKeyName;


  DPRINT("CmiInitHives() called\n");

  if (SetUpBoot == TRUE)
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
      if (ValueInfo == NULL)
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

  /* Connect the SYSTEM hive */
  wcscpy(EndPtr, REG_SYSTEM_FILE_NAME);
  DPRINT("ConfigPath: %S\n", ConfigPath);

  RtlInitUnicodeString (&ParentKeyName,
			REG_SYSTEM_KEY_NAME);
  Status = CmiInitializeSystemHive(ConfigPath,
				   &ParentKeyName,
				   "System",
				   CmiMachineKey,
				   SetUpBoot);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("CmiInitializeSystemHive() failed (Status %lx)\n", Status);
      return(Status);
    }

  /* Connect the SOFTWARE hive */
  wcscpy(EndPtr, REG_SOFTWARE_FILE_NAME);
  DPRINT("ConfigPath: %S\n", ConfigPath);

  RtlInitUnicodeString (&ParentKeyName,
			REG_SOFTWARE_KEY_NAME);
  Status = CmiInitializeHive(ConfigPath,
			     &ParentKeyName,
			     "Software",
			     CmiMachineKey,
			     SetUpBoot);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("CmiInitializeHive() failed (Status %lx)\n", Status);
      return(Status);
    }

  /* Connect the SAM hive */
  wcscpy(EndPtr, REG_SAM_FILE_NAME);
  DPRINT1("ConfigPath: %S\n", ConfigPath);

  RtlInitUnicodeString (&ParentKeyName,
			REG_SAM_KEY_NAME);
  Status = CmiInitializeHive(ConfigPath,
			     &ParentKeyName,
			     "Sam",
			     CmiMachineKey,
			     SetUpBoot);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("CmiInitializeHive() failed (Status %lx)\n", Status);
      return(Status);
    }

  /* Connect the SECURITY hive */
  wcscpy(EndPtr, REG_SEC_FILE_NAME);
  DPRINT1("ConfigPath: %S\n", ConfigPath);
  RtlInitUnicodeString (&ParentKeyName,
			REG_SEC_KEY_NAME);
  Status = CmiInitializeHive(ConfigPath,
			     &ParentKeyName,
			     "Security",
			     CmiMachineKey,
			     SetUpBoot);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("CmiInitializeHive() failed (Status %lx)\n", Status);
      return(Status);
    }

  /* Connect the DEFAULT hive */
  wcscpy(EndPtr, REG_USER_FILE_NAME);
  DPRINT1("ConfigPath: %S\n", ConfigPath);

  RtlInitUnicodeString (&ParentKeyName,
			REG_USER_KEY_NAME);
  Status = CmiInitializeHive(ConfigPath,
			     &ParentKeyName,
			     ".Default",
			     CmiUserKey,
			     SetUpBoot);
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

      if (IsPermanentHive(Hive))
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

  DPRINT1("CmiHiveSyncRoutine() called\n");

  CmiHiveSyncPending = FALSE;

  /* Acquire hive list lock exclusively */
  ExAcquireResourceExclusiveLite(&CmiHiveListLock, TRUE);

  Entry = CmiHiveListHead.Flink;
  while (Entry != &CmiHiveListHead)
    {
      Hive = CONTAINING_RECORD(Entry, REGISTRY_HIVE, HiveList);

      if (IsPermanentHive(Hive))
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

  DPRINT1("CmiHiveSyncRoutine() done\n");
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
