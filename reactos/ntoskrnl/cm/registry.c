/* $Id$
 *
 * PROJECT:         ReactOS Kernel
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/cm/registry.c
 * PURPOSE:         Registry functions
 *
 * PROGRAMMERS:     Hartmut Birr
 *                  Alex Ionescu
 *                  Rex Jolliff
 *                  Eric Kohl
 *                  Matt Pyne
 *                  Jean Michault
 *                  Art Yerkes
 */

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#include "cm.h"

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, CmInitSystem1)
#endif

/* GLOBALS ******************************************************************/

extern BOOLEAN ExpInTextModeSetup;

POBJECT_TYPE  CmiKeyType = NULL;
PEREGISTRY_HIVE  CmiVolatileHive = NULL;

LIST_ENTRY CmiHiveListHead;

ERESOURCE CmiRegistryLock;

KTIMER CmiWorkerTimer;
LIST_ENTRY CmiKeyObjectListHead;
LIST_ENTRY CmiConnectedHiveList;
ULONG CmiTimer = 0;

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

extern LIST_ENTRY CmiCallbackHead;
extern FAST_MUTEX CmiCallbackLock;

/* FUNCTIONS ****************************************************************/

VOID STDCALL
CmiWorkerThread(PVOID Param)
{
  NTSTATUS Status;
  PLIST_ENTRY CurrentEntry;
  PKEY_OBJECT CurrentKey;
  ULONG Count;


  while (1)
  {
    Status = KeWaitForSingleObject(&CmiWorkerTimer,
	                           Executive,
				   KernelMode,
				   FALSE,
				   NULL);
    if (Status == STATUS_SUCCESS)
    {
      DPRINT("CmiWorkerThread\n");

      /* Acquire hive lock */
      KeEnterCriticalRegion();
      ExAcquireResourceExclusiveLite(&CmiRegistryLock, TRUE);

      CmiTimer++;

      Count = 0;
      CurrentEntry = CmiKeyObjectListHead.Blink;
      while (CurrentEntry != &CmiKeyObjectListHead)
      {
         CurrentKey = CONTAINING_RECORD(CurrentEntry, KEY_OBJECT, ListEntry);
	 if (CurrentKey->TimeStamp + 120 > CmiTimer)
	 {
	    /* The object was accessed in the last 10min */
	    break;
	 }
	 if (1 == ObGetObjectPointerCount(CurrentKey) &&
	     !(CurrentKey->Flags & KO_MARKED_FOR_DELETE))
	 {
	    ObDereferenceObject(CurrentKey);
            CurrentEntry = CmiKeyObjectListHead.Blink;
	    Count++;
	 }
	 else
	 {
	    CurrentEntry = CurrentEntry->Blink;
	 }
      }
      ExReleaseResourceLite(&CmiRegistryLock);
      KeLeaveCriticalRegion();

      DPRINT("Removed %d key objects\n", Count);

    }
    else
    {
      KEBUGCHECK(0);
    }
  }
}

VOID INIT_FUNCTION
CmInit2(PCHAR CommandLine)
{
  ULONG PiceStart = 4;
  BOOLEAN MiniNT = FALSE;
  NTSTATUS Status;
  UNICODE_STRING TempString;

  /* Create the 'CurrentControlSet' link. */
  Status = CmiCreateCurrentControlSetLink();
  if (!NT_SUCCESS(Status))
    KEBUGCHECK(CONFIG_INITIALIZATION_FAILED);

  /*
   * Write the system boot device to registry.
   */
  RtlCreateUnicodeStringFromAsciiz(&TempString, KeLoaderBlock->ArcBootDeviceName);
  Status = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
				 L"\\Registry\\Machine\\System\\CurrentControlSet\\Control",
				 L"SystemBootDevice",
				 REG_SZ,
				 TempString.Buffer,
				 TempString.MaximumLength);
  RtlFreeUnicodeString(&TempString);
  if (!NT_SUCCESS(Status))
  {
    KEBUGCHECK(CONFIG_INITIALIZATION_FAILED);
  }

  /*
   * Parse the system start options.
   */
  if (strstr(KeLoaderBlock->LoadOptions, "DEBUGPORT=PICE") != NULL)
	PiceStart = 1;
  MiniNT = strstr(KeLoaderBlock->LoadOptions, "MININT") != NULL;

  /*
   * Write the system start options to registry.
   */
  RtlCreateUnicodeStringFromAsciiz(&TempString, KeLoaderBlock->LoadOptions);
  Status = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
				 L"\\Registry\\Machine\\System\\CurrentControlSet\\Control",
				 L"SystemStartOptions",
				 REG_SZ,
				 TempString.Buffer,
				 TempString.MaximumLength);
  RtlFreeUnicodeString(&TempString);
  if (!NT_SUCCESS(Status))
  {
    KEBUGCHECK(CONFIG_INITIALIZATION_FAILED);
  }

  /*
   * Create a CurrentControlSet\Control\MiniNT key that is used
   * to detect WinPE/MiniNT systems.
   */
  if (MiniNT)
    {
      Status = RtlCreateRegistryKey(RTL_REGISTRY_CONTROL, L"MiniNT");
      if (!NT_SUCCESS(Status))
        KEBUGCHECK(CONFIG_INITIALIZATION_FAILED);
    }

  /* Set PICE 'Start' value to 1, if PICE debugging is enabled */
  Status = RtlWriteRegistryValue(
    RTL_REGISTRY_SERVICES,
    L"\\Pice",
    L"Start",
    REG_DWORD,
    &PiceStart,
    sizeof(ULONG));
  if (!NT_SUCCESS(Status))
    KEBUGCHECK(CONFIG_INITIALIZATION_FAILED);
}


VOID
INIT_FUNCTION
STDCALL
CmInitHives(BOOLEAN SetupBoot)
{
    PCHAR BaseAddress;
    PLIST_ENTRY ListHead, NextEntry;
    PMEMORY_ALLOCATION_DESCRIPTOR MdBlock = NULL;

    /* Load Registry Hives. This one can be missing. */
    BaseAddress = KeLoaderBlock->RegistryBase;
    if (BaseAddress)
    {
        CmImportSystemHive(BaseAddress,
                           KeLoaderBlock->RegistryLength);
    }

    /* Loop the memory descriptors */
    ListHead = &KeLoaderBlock->MemoryDescriptorListHead;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        /* Get the current block */
        MdBlock = CONTAINING_RECORD(NextEntry,
                                    MEMORY_ALLOCATION_DESCRIPTOR,
                                    ListEntry);

        /* Check if this is an registry block */
        if (MdBlock->MemoryType == LoaderRegistryData)
        {
            /* Check if it's not the SYSTEM hive that we already initialized */
            if ((MdBlock->BasePage) != ((ULONG_PTR)BaseAddress >> PAGE_SHIFT))
            {
                /* Hardware hive break out */
                break;
            }
        }

        /* Go to the next block */
        NextEntry = MdBlock->ListEntry.Flink;
    }

    /* We need a hardware hive */
    ASSERT(MdBlock);

    BaseAddress = (PCHAR)(MdBlock->BasePage << PAGE_SHIFT);
    CmImportHardwareHive(BaseAddress,
                         MdBlock->PageCount << PAGE_SHIFT);

    /* Create dummy keys if no hardware hive was found */
    CmImportHardwareHive (NULL, 0);

    /* Initialize volatile registry settings */
    if (SetupBoot == FALSE) CmInit2(KeLoaderBlock->LoadOptions);
}

BOOLEAN
INIT_FUNCTION
NTAPI
CmInitSystem1(VOID)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING KeyName;
  PKEY_OBJECT RootKey;
#if 0
  PCM_KEY_SECURITY RootSecurityCell;
#endif
  HANDLE RootKeyHandle;
  HANDLE KeyHandle;
  NTSTATUS Status;
  LARGE_INTEGER DueTime;
  HANDLE ThreadHandle;
  CLIENT_ID ThreadId;
  OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
  UNICODE_STRING Name;

  DPRINT("Creating Registry Object Type\n");
  
  /*  Initialize the Key object type  */
  RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
  RtlInitUnicodeString(&Name, L"Key");
  ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
  ObjectTypeInitializer.DefaultPagedPoolCharge = sizeof(KEY_OBJECT);
  ObjectTypeInitializer.GenericMapping = CmiKeyMapping;
  ObjectTypeInitializer.PoolType = PagedPool;
  ObjectTypeInitializer.ValidAccessMask = KEY_ALL_ACCESS;
  ObjectTypeInitializer.UseDefaultObject = TRUE;
  ObjectTypeInitializer.DeleteProcedure = CmiObjectDelete;
  ObjectTypeInitializer.ParseProcedure = CmiObjectParse;
  ObjectTypeInitializer.SecurityProcedure = CmiObjectSecurity;
  ObjectTypeInitializer.QueryNameProcedure = CmiObjectQueryName;

  ObCreateObjectType(&Name, &ObjectTypeInitializer, NULL, &CmiKeyType);

  /* Initialize the hive list */
  InitializeListHead(&CmiHiveListHead);

  /* Initialize registry lock */
  ExInitializeResourceLite(&CmiRegistryLock);

  /* Initialize the key object list */
  InitializeListHead(&CmiKeyObjectListHead);
  InitializeListHead(&CmiConnectedHiveList);

  /* Initialize the worker timer */
  KeInitializeTimerEx(&CmiWorkerTimer, SynchronizationTimer);

  /* Initialize the worker thread */
  Status = PsCreateSystemThread(&ThreadHandle,
				THREAD_ALL_ACCESS,
				NULL,
				NULL,
				&ThreadId,
				CmiWorkerThread,
				NULL);
  if (!NT_SUCCESS(Status)) return FALSE;

  /* Start the timer */
  DueTime.QuadPart = -1;
  KeSetTimerEx(&CmiWorkerTimer, DueTime, 5000, NULL); /* 5sec */

  /*  Build volatile registry store  */
  Status = CmiCreateVolatileHive (&CmiVolatileHive);
  ASSERT(NT_SUCCESS(Status));

  InitializeListHead(&CmiCallbackHead);
  ExInitializeFastMutex(&CmiCallbackLock);

  /* Create '\Registry' key. */
  RtlInitUnicodeString(&KeyName, REG_ROOT_KEY_NAME);
  InitializeObjectAttributes(&ObjectAttributes, &KeyName, 0, NULL, NULL);
  Status = ObCreateObject(KernelMode,
			  CmiKeyType,
			  &ObjectAttributes,
			  KernelMode,
			  NULL,
			  sizeof(KEY_OBJECT),
			  0,
			  0,
			  (PVOID *) &RootKey);
  ASSERT(NT_SUCCESS(Status));
  Status = ObInsertObject(RootKey,
			  NULL,
			  KEY_ALL_ACCESS,
			  0,
			  NULL,
			  &RootKeyHandle);
  ASSERT(NT_SUCCESS(Status));
  RootKey->RegistryHive = CmiVolatileHive;
  RootKey->KeyCellOffset = CmiVolatileHive->Hive.HiveHeader->RootCell;
  RootKey->KeyCell = HvGetCell (&CmiVolatileHive->Hive, RootKey->KeyCellOffset);
  RootKey->ParentKey = RootKey;
  RootKey->Flags = 0;
  RootKey->SubKeyCounts = 0;
  RootKey->SubKeys = NULL;
  RootKey->SizeOfSubKeys = 0;
  InsertTailList(&CmiKeyObjectListHead, &RootKey->ListEntry);
  Status = RtlpCreateUnicodeString(&RootKey->Name, L"Registry", NonPagedPool);
  ASSERT(NT_SUCCESS(Status));

#if 0
  Status = CmiAllocateCell(CmiVolatileHive,
			   0x10, //LONG CellSize,
			   (PVOID *)&RootSecurityCell,
			   &RootKey->KeyCell->SecurityKeyOffset);
  ASSERT(NT_SUCCESS(Status));

  /* Copy the security descriptor */

  CmiVolatileHive->RootSecurityCell = RootSecurityCell;
#endif


  /* Create '\Registry\Machine' key. */
  RtlInitUnicodeString(&KeyName,
		       L"Machine");
  InitializeObjectAttributes(&ObjectAttributes,
			     &KeyName,
			     0,
			     RootKeyHandle,
			     NULL);
  Status = ZwCreateKey(&KeyHandle,
		       KEY_ALL_ACCESS,
		       &ObjectAttributes,
		       0,
		       NULL,
		       REG_OPTION_VOLATILE,
		       NULL);
  ASSERT(NT_SUCCESS(Status));

  /* Create '\Registry\User' key. */
  RtlInitUnicodeString(&KeyName,
		       L"User");
  InitializeObjectAttributes(&ObjectAttributes,
			     &KeyName,
			     0,
			     RootKeyHandle,
			     NULL);
  Status = ZwCreateKey(&KeyHandle,
		       KEY_ALL_ACCESS,
		       &ObjectAttributes,
		       0,
		       NULL,
		       REG_OPTION_VOLATILE,
		       NULL);
  ASSERT(NT_SUCCESS(Status));

  /* Import and Load Registry Hives */
  CmInitHives(ExpInTextModeSetup);
  return TRUE;
}



static NTSTATUS
CmiCreateCurrentControlSetLink(VOID)
{
  RTL_QUERY_REGISTRY_TABLE QueryTable[5];
  WCHAR TargetNameBuffer[80];
  ULONG TargetNameLength;
  UNICODE_STRING LinkName = RTL_CONSTANT_STRING(
                            L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet");
  UNICODE_STRING LinkValue = RTL_CONSTANT_STRING(L"SymbolicLinkValue");
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

  InitializeObjectAttributes(&ObjectAttributes,
			     &LinkName,
			     OBJ_CASE_INSENSITIVE | OBJ_OPENIF | OBJ_OPENLINK,
			     NULL,
			     NULL);
  Status = ZwCreateKey(&KeyHandle,
		       KEY_ALL_ACCESS | KEY_CREATE_LINK,
		       &ObjectAttributes,
		       0,
		       NULL,
		       REG_OPTION_VOLATILE | REG_OPTION_CREATE_LINK,
		       NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("ZwCreateKey() failed (Status %lx)\n", Status);
      return(Status);
    }

  Status = ZwSetValueKey(KeyHandle,
			 &LinkValue,
			 0,
			 REG_LINK,
			 (PVOID)TargetNameBuffer,
			 TargetNameLength);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("ZwSetValueKey() failed (Status %lx)\n", Status);
    }

  ZwClose(KeyHandle);

  return Status;
}

NTSTATUS
CmiConnectHive(IN POBJECT_ATTRIBUTES KeyObjectAttributes,
	       IN PEREGISTRY_HIVE RegistryHive)
{
    UNICODE_STRING RemainingPath;
    PKEY_OBJECT ParentKey;
    PKEY_OBJECT NewKey;
    NTSTATUS Status;
    PWSTR SubName;
    UNICODE_STRING ObjectName;
    OBJECT_CREATE_INFORMATION ObjectCreateInfo;

    DPRINT("CmiConnectHive(%p, %p) called.\n",
	   KeyObjectAttributes, RegistryHive);

    /* Capture all the info */
    DPRINT("Capturing Create Info\n");
    Status = ObpCaptureObjectAttributes(KeyObjectAttributes,
					KernelMode,
					FALSE,
					&ObjectCreateInfo,
					&ObjectName);

    if (!NT_SUCCESS(Status))
      {
	DPRINT("ObpCaptureObjectAttributes() failed (Status %lx)\n", Status);
	return Status;
      }

    Status = CmFindObject(&ObjectCreateInfo,
			  &ObjectName,
			  (PVOID*)&ParentKey,
			  &RemainingPath,
			  CmiKeyType,
			  NULL,
			  NULL);
    /* Yields a new reference */
    ObpReleaseCapturedAttributes(&ObjectCreateInfo);

    if (ObjectName.Buffer) ObpFreeObjectNameBuffer(&ObjectName);
    if (!NT_SUCCESS(Status))
      {
	return Status;
      }

    DPRINT ("RemainingPath %wZ\n", &RemainingPath);

    if ((RemainingPath.Buffer == NULL) || (RemainingPath.Buffer[0] == 0))
      {
	ObDereferenceObject (ParentKey);
	RtlFreeUnicodeString(&RemainingPath);
	return STATUS_OBJECT_NAME_COLLISION;
      }

    /* Ignore leading backslash */
    SubName = RemainingPath.Buffer;
    if (*SubName == L'\\')
	SubName++;

    /* If RemainingPath contains \ we must return error
       because CmiConnectHive() can not create trees */
    if (wcschr (SubName, L'\\') != NULL)
      {
	ObDereferenceObject (ParentKey);
	RtlFreeUnicodeString(&RemainingPath);
	return STATUS_OBJECT_NAME_NOT_FOUND;
      }

    DPRINT("RemainingPath %wZ  ParentKey %p\n",
	   &RemainingPath, ParentKey);

    Status = ObCreateObject(KernelMode,
			    CmiKeyType,
			    NULL,
			    KernelMode,
			    NULL,
			    sizeof(KEY_OBJECT),
			    0,
			    0,
			    (PVOID*)&NewKey);
        
    if (!NT_SUCCESS(Status))
      {
	DPRINT1 ("ObCreateObject() failed (Status %lx)\n", Status);
	ObDereferenceObject (ParentKey);
	RtlFreeUnicodeString(&RemainingPath);
	return Status;
      }
#if 0
    DPRINT("Inserting Key into Object Tree\n");
    Status =  ObInsertObject((PVOID)NewKey,
                             NULL,
                             KEY_ALL_ACCESS,
                             0,
                             NULL,
                             NULL);
  DPRINT("Status %x\n", Status);
#else
    /* Free the create information */
    ObpFreeAndReleaseCapturedAttributes(OBJECT_TO_OBJECT_HEADER(NewKey)->ObjectCreateInfo);
    OBJECT_TO_OBJECT_HEADER(NewKey)->ObjectCreateInfo = NULL;
#endif
  NewKey->Flags = 0;
  NewKey->SubKeyCounts = 0;
  NewKey->SubKeys = NULL;
  NewKey->SizeOfSubKeys = 0;
  InsertTailList(&CmiKeyObjectListHead, &NewKey->ListEntry);

  DPRINT ("SubName %S\n", SubName);

  Status = CmiAddSubKey(ParentKey->RegistryHive,
			ParentKey,
			NewKey,
			&RemainingPath,
			0,
			NULL,
			REG_OPTION_VOLATILE);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("CmiAddSubKey() failed (Status %lx)\n", Status);
      ObDereferenceObject (NewKey);
      ObDereferenceObject (ParentKey);
      return STATUS_INSUFFICIENT_RESOURCES;
    }

  NewKey->KeyCellOffset = RegistryHive->Hive.HiveHeader->RootCell;
  NewKey->KeyCell = HvGetCell (&RegistryHive->Hive, NewKey->KeyCellOffset);
  NewKey->RegistryHive = RegistryHive;

  Status = RtlpCreateUnicodeString(&NewKey->Name,
              SubName, NonPagedPool);
  RtlFreeUnicodeString(&RemainingPath);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("RtlpCreateUnicodeString() failed (Status %lx)\n", Status);
      ObDereferenceObject (NewKey);
      ObDereferenceObject (ParentKey);
      return STATUS_INSUFFICIENT_RESOURCES;
    }

  /* FN1 */
  ObReferenceObject (NewKey);

  CmiAddKeyToList (ParentKey, NewKey);
  ObDereferenceObject (ParentKey);

    VERIFY_KEY_OBJECT(NewKey);

    /* We're holding a pointer to the parent key ..  We must keep it 
     * referenced */
    /* Note: Do not dereference NewKey here! */

    return STATUS_SUCCESS;
}


NTSTATUS
CmiDisconnectHive (IN POBJECT_ATTRIBUTES KeyObjectAttributes,
		   OUT PEREGISTRY_HIVE *RegistryHive)
{
  PKEY_OBJECT KeyObject;
  PEREGISTRY_HIVE Hive;
  HANDLE KeyHandle;
  NTSTATUS Status = STATUS_OBJECT_NAME_NOT_FOUND;
  PLIST_ENTRY CurrentEntry;
  PKEY_OBJECT CurrentKey;

  DPRINT("CmiDisconnectHive() called\n");

  *RegistryHive = NULL;

  Status = ObOpenObjectByName (KeyObjectAttributes,
			       CmiKeyType,
			       KernelMode,
			       NULL,
			       STANDARD_RIGHTS_REQUIRED,
			       NULL,
			       &KeyHandle);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1 ("ObOpenObjectByName() failed (Status %lx)\n", Status);
      return Status;
    }

  Status = ObReferenceObjectByHandle (KeyHandle,
				      STANDARD_RIGHTS_REQUIRED,
				      CmiKeyType,
				      KernelMode,
				      (PVOID*)&KeyObject,
				      NULL);
  ZwClose (KeyHandle);

  if (!NT_SUCCESS(Status))
    {
      DPRINT1 ("ObReferenceObjectByName() failed (Status %lx)\n", Status);
      return Status;
    }
  DPRINT ("KeyObject %p  Hive %p\n", KeyObject, KeyObject->RegistryHive);

  /* Acquire registry lock exclusively */
  KeEnterCriticalRegion();
  ExAcquireResourceExclusiveLite(&CmiRegistryLock, TRUE);

  Hive = KeyObject->RegistryHive;

  CurrentEntry = CmiKeyObjectListHead.Flink;
  while (CurrentEntry != &CmiKeyObjectListHead)
  {
     CurrentKey = CONTAINING_RECORD(CurrentEntry, KEY_OBJECT, ListEntry);
     if (CurrentKey->RegistryHive == Hive &&
         1 == ObGetObjectPointerCount(CurrentKey) &&
	 !(CurrentKey->Flags & KO_MARKED_FOR_DELETE))
     {
        ObDereferenceObject(CurrentKey);
        CurrentEntry = CmiKeyObjectListHead.Flink;
     }
     else
     {
        CurrentEntry = CurrentEntry->Flink;
     }
  }

  /* FN1 */
  ObDereferenceObject (KeyObject);

  if (ObGetObjectHandleCount (KeyObject) != 0 ||
      ObGetObjectPointerCount (KeyObject) != 2)
    {
      DPRINT1 ("Hive is still in use (hc %d, rc %d)\n", ObGetObjectHandleCount (KeyObject), ObGetObjectPointerCount (KeyObject));
      ObDereferenceObject (KeyObject);

      /* Release registry lock */
      ExReleaseResourceLite (&CmiRegistryLock);
      KeLeaveCriticalRegion();

      return STATUS_UNSUCCESSFUL;
    }

  /* Dereference KeyObject twice to delete it */
  ObDereferenceObject (KeyObject);
  ObDereferenceObject (KeyObject);

  *RegistryHive = Hive;

  /* Release registry lock */
  ExReleaseResourceLite (&CmiRegistryLock);
  KeLeaveCriticalRegion();

  /* Release reference above */
  ObDereferenceObject (KeyObject);

  DPRINT ("CmiDisconnectHive() done\n");

  return Status;
}


static NTSTATUS
CmiInitControlSetLink (VOID)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING ControlSetKeyName = RTL_CONSTANT_STRING(
                                L"\\Registry\\Machine\\SYSTEM\\ControlSet001");
  UNICODE_STRING ControlSetLinkName =  RTL_CONSTANT_STRING(
                            L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet");
  UNICODE_STRING ControlSetValueName = RTL_CONSTANT_STRING(L"SymbolicLinkValue");
  HANDLE KeyHandle;
  NTSTATUS Status;

  /* Create 'ControlSet001' key */
  InitializeObjectAttributes (&ObjectAttributes,
			      &ControlSetKeyName,
			      OBJ_CASE_INSENSITIVE,
			      NULL,
			      NULL);
  Status = ZwCreateKey (&KeyHandle,
			KEY_ALL_ACCESS,
			&ObjectAttributes,
			0,
			NULL,
			REG_OPTION_NON_VOLATILE,
			NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1 ("ZwCreateKey() failed (Status %lx)\n", Status);
      return Status;
    }
  ZwClose (KeyHandle);

  /* Link 'CurrentControlSet' to 'ControlSet001' key */
  InitializeObjectAttributes (&ObjectAttributes,
			      &ControlSetLinkName,
			      OBJ_CASE_INSENSITIVE | OBJ_OPENIF | OBJ_OPENLINK,
			      NULL,
			      NULL);
  Status = ZwCreateKey (&KeyHandle,
			KEY_ALL_ACCESS | KEY_CREATE_LINK,
			&ObjectAttributes,
			0,
			NULL,
			REG_OPTION_VOLATILE | REG_OPTION_CREATE_LINK,
			NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1 ("ZwCreateKey() failed (Status %lx)\n", Status);
      return Status;
    }

  Status = ZwSetValueKey (KeyHandle,
			  &ControlSetValueName,
			  0,
			  REG_LINK,
			  (PVOID)ControlSetKeyName.Buffer,
			  ControlSetKeyName.Length);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1 ("ZwSetValueKey() failed (Status %lx)\n", Status);
    }
  ZwClose (KeyHandle);

  return STATUS_SUCCESS;
}


NTSTATUS
CmiInitHives(BOOLEAN SetupBoot)
{
  PKEY_VALUE_PARTIAL_INFORMATION ValueInfo;
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING FileName;
  UNICODE_STRING KeyName = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\HARDWARE");
  UNICODE_STRING ValueName = RTL_CONSTANT_STRING(L"InstallPath");
  HANDLE KeyHandle;

  NTSTATUS Status;

  WCHAR ConfigPath[MAX_PATH];

  ULONG BufferSize;
  ULONG ResultSize;
  PWSTR EndPtr;


  DPRINT("CmiInitHives() called\n");

  if (SetupBoot == TRUE)
    {
      InitializeObjectAttributes(&ObjectAttributes,
				 &KeyName,
				 OBJ_CASE_INSENSITIVE,
				 NULL,
				 NULL);
      Status =  ZwOpenKey(&KeyHandle,
			  KEY_ALL_ACCESS,
			  &ObjectAttributes);
      if (!NT_SUCCESS(Status))
	{
	  DPRINT1("ZwOpenKey() failed (Status %lx)\n", Status);
	  return(Status);
	}

      BufferSize = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 4096;
      ValueInfo = ExAllocatePool(PagedPool,
				 BufferSize);
      if (ValueInfo == NULL)
	{
	  ZwClose(KeyHandle);
	  return(STATUS_INSUFFICIENT_RESOURCES);
	}

      Status = ZwQueryValueKey(KeyHandle,
			       &ValueName,
			       KeyValuePartialInformation,
			       ValueInfo,
			       BufferSize,
			       &ResultSize);
      ZwClose(KeyHandle);
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

  /* FIXME: Save boot log */

  /* Connect the SYSTEM hive only if it has been created */
  if (SetupBoot == TRUE)
    {
      wcscpy(EndPtr, REG_SYSTEM_FILE_NAME);
      DPRINT ("ConfigPath: %S\n", ConfigPath);

      RtlInitUnicodeString (&KeyName,
			    REG_SYSTEM_KEY_NAME);
      InitializeObjectAttributes(&ObjectAttributes,
				 &KeyName,
				 OBJ_CASE_INSENSITIVE,
				 NULL,
				 NULL);

      RtlInitUnicodeString (&FileName,
			    ConfigPath);
      Status = CmiLoadHive (&ObjectAttributes,
			    &FileName,
			    0);
      if (!NT_SUCCESS(Status))
	{
	  DPRINT1 ("CmiLoadHive() failed (Status %lx)\n", Status);
	  return Status;
	}

      Status = CmiInitControlSetLink ();
      if (!NT_SUCCESS(Status))
	{
	  DPRINT1("CmiInitControlSetLink() failed (Status %lx)\n", Status);
	  return(Status);
	}
    }

  /* Connect the SOFTWARE hive */
  wcscpy(EndPtr, REG_SOFTWARE_FILE_NAME);
  RtlInitUnicodeString (&FileName,
			ConfigPath);
  DPRINT ("ConfigPath: %S\n", ConfigPath);

  RtlInitUnicodeString (&KeyName,
			REG_SOFTWARE_KEY_NAME);
  InitializeObjectAttributes(&ObjectAttributes,
			     &KeyName,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);

  Status = CmiLoadHive (&ObjectAttributes,
			&FileName,
			0);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("CmiInitializeHive() failed (Status %lx)\n", Status);
      return(Status);
    }

  /* Connect the SAM hive */
  wcscpy(EndPtr, REG_SAM_FILE_NAME);
  RtlInitUnicodeString (&FileName,
			ConfigPath);
  DPRINT ("ConfigPath: %S\n", ConfigPath);

  RtlInitUnicodeString (&KeyName,
			REG_SAM_KEY_NAME);
  InitializeObjectAttributes(&ObjectAttributes,
			     &KeyName,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);
  Status = CmiLoadHive (&ObjectAttributes,
			&FileName,
			0);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("CmiInitializeHive() failed (Status %lx)\n", Status);
      return(Status);
    }

  /* Connect the SECURITY hive */
  wcscpy(EndPtr, REG_SEC_FILE_NAME);
  RtlInitUnicodeString (&FileName,
			ConfigPath);
  DPRINT ("ConfigPath: %S\n", ConfigPath);

  RtlInitUnicodeString (&KeyName,
			REG_SEC_KEY_NAME);
  InitializeObjectAttributes(&ObjectAttributes,
			     &KeyName,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);
  Status = CmiLoadHive (&ObjectAttributes,
			&FileName,
			0);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("CmiInitializeHive() failed (Status %lx)\n", Status);
      return(Status);
    }

  /* Connect the DEFAULT hive */
  wcscpy(EndPtr, REG_DEFAULT_USER_FILE_NAME);
  RtlInitUnicodeString (&FileName,
			ConfigPath);
  DPRINT ("ConfigPath: %S\n", ConfigPath);

  RtlInitUnicodeString (&KeyName,
			REG_DEFAULT_USER_KEY_NAME);
  InitializeObjectAttributes(&ObjectAttributes,
			     &KeyName,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);
  Status = CmiLoadHive (&ObjectAttributes,
			&FileName,
			0);
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
  PEREGISTRY_HIVE Hive;
  PLIST_ENTRY Entry;

  DPRINT("CmShutdownRegistry() called\n");

  /* Stop automatic hive synchronization */
  CmiHiveSyncEnabled = FALSE;

  /* Cancel pending hive synchronization */
  if (CmiHiveSyncPending == TRUE)
    {
      KeCancelTimer(&CmiHiveSyncTimer);
      CmiHiveSyncPending = FALSE;
    }

  /* Acquire hive list lock exclusively */
  KeEnterCriticalRegion();
  ExAcquireResourceExclusiveLite(&CmiRegistryLock, TRUE);

  Entry = CmiHiveListHead.Flink;
  while (Entry != &CmiHiveListHead)
    {
      Hive = CONTAINING_RECORD(Entry, EREGISTRY_HIVE, HiveList);

      if (!(IsNoFileHive(Hive) || IsNoSynchHive(Hive)))
	{
	  /* Flush non-volatile hive */
	  CmiFlushRegistryHive(Hive);
	}

      Entry = Entry->Flink;
    }

  /* Release hive list lock */
  ExReleaseResourceLite(&CmiRegistryLock);
  KeLeaveCriticalRegion();

  DPRINT("CmShutdownRegistry() done\n");
}


VOID STDCALL
CmiHiveSyncRoutine(PVOID DeferredContext)
{
  PEREGISTRY_HIVE Hive;
  PLIST_ENTRY Entry;

  DPRINT("CmiHiveSyncRoutine() called\n");

  CmiHiveSyncPending = FALSE;

  /* Acquire hive list lock exclusively */
  KeEnterCriticalRegion();
  ExAcquireResourceExclusiveLite(&CmiRegistryLock, TRUE);

  Entry = CmiHiveListHead.Flink;
  while (Entry != &CmiHiveListHead)
    {
      Hive = CONTAINING_RECORD(Entry, EREGISTRY_HIVE, HiveList);

      if (!(IsNoFileHive(Hive) || IsNoSynchHive(Hive)))
	{
	  /* Flush non-volatile hive */
	  CmiFlushRegistryHive(Hive);
	}

      Entry = Entry->Flink;
    }

  /* Release hive list lock */
  ExReleaseResourceLite(&CmiRegistryLock);
  KeLeaveCriticalRegion();

  DPRINT("DeferredContext 0x%p\n", DeferredContext);
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

  DPRINT("DeferredContext 0x%p\n", WorkQueueItem);
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

  Timeout.QuadPart = (LONGLONG)-50000000;
  KeSetTimer(&CmiHiveSyncTimer,
	     Timeout,
	     &CmiHiveSyncDpc);
}

/* EOF */
