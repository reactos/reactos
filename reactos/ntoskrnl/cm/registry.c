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

BOOLEAN
INIT_FUNCTION
CmImportSystemHive(PCHAR ChunkBase,
                   ULONG ChunkSize,
                   OUT PEREGISTRY_HIVE *RegistryHive);

BOOLEAN
INIT_FUNCTION
CmImportHardwareHive(PCHAR ChunkBase,
                     ULONG ChunkSize,
                     OUT PEREGISTRY_HIVE *RegistryHive);

NTSTATUS
NTAPI
CmpSetSystemValues(IN PLOADER_PARAMETER_BLOCK LoaderBlock);

NTSTATUS
NTAPI
CmpCreateControlSet(IN PLOADER_PARAMETER_BLOCK LoaderBlock);

static VOID STDCALL
CmiHiveSyncDpcRoutine(PKDPC Dpc,
		      PVOID DeferredContext,
		      PVOID SystemArgument1,
		      PVOID SystemArgument2);

extern LIST_ENTRY CmiCallbackHead;
extern FAST_MUTEX CmiCallbackLock;

extern UNICODE_STRING CmpSystemStartOptions;
extern UNICODE_STRING CmpLoadOptions;
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

PVOID
NTAPI
CmpRosGetHardwareHive(OUT PULONG Length)
{
    PLIST_ENTRY ListHead, NextEntry;
    PMEMORY_ALLOCATION_DESCRIPTOR MdBlock = NULL;

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
            if ((MdBlock->BasePage) !=
                ((ULONG_PTR)KeLoaderBlock->RegistryBase >> PAGE_SHIFT))
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
    *Length = MdBlock->PageCount << PAGE_SHIFT;
    return (PVOID)(MdBlock->BasePage << PAGE_SHIFT);
}

NTSTATUS
NTAPI
CmpLinkHiveToMaster(IN PUNICODE_STRING LinkName,
                    IN HANDLE RootDirectory,
                    IN PEREGISTRY_HIVE RegistryHive,
                    IN BOOLEAN Allocate,
                    IN PSECURITY_DESCRIPTOR SecurityDescriptor)
{
    OBJECT_ATTRIBUTES ObjectAttributes;

    /* Don't do anything if we don't actually have a hive */
    if (Allocate) return STATUS_SUCCESS;

    /* Setup the object attributes */
    InitializeObjectAttributes(&ObjectAttributes,
                               LinkName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               RootDirectory,
                               SecurityDescriptor);

    /* Connect the hive */
    return CmiConnectHive(&ObjectAttributes, RegistryHive);
}

BOOLEAN
NTAPI
CmpInitializeSystemHive(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PVOID HiveBase;
    ANSI_STRING LoadString;
    PVOID Buffer;
    ULONG Length;
    NTSTATUS Status;
    BOOLEAN Allocate;
    UNICODE_STRING KeyName;
    PEREGISTRY_HIVE SystemHive;
    PAGED_CODE();

    /* Setup the ansi string */
    RtlInitAnsiString(&LoadString, LoaderBlock->LoadOptions);

    /* Allocate the unicode buffer */
    Length = LoadString.Length * sizeof(WCHAR) + sizeof(UNICODE_NULL);
    Buffer = ExAllocatePoolWithTag(PagedPool, Length, 0);
    if (!Buffer)
    {
        /* Fail */
        KEBUGCHECKEX(BAD_SYSTEM_CONFIG_INFO, 3, 1, (ULONG_PTR)LoaderBlock, 0);
    }

    /* Setup the unicode string */
    RtlInitEmptyUnicodeString(&CmpLoadOptions, Buffer, Length);

    /* Add the load options and null-terminate */
    RtlAnsiStringToUnicodeString(&CmpLoadOptions, &LoadString, FALSE);
    CmpLoadOptions.Buffer[LoadString.Length] = UNICODE_NULL;
    CmpLoadOptions.Length += sizeof(WCHAR);

    /* Get the System Hive base address */
    HiveBase = LoaderBlock->RegistryBase;
    if (HiveBase)
    {
        /* Import it */
        Status = CmImportSystemHive(HiveBase,
                                    LoaderBlock->RegistryLength,
                                    &SystemHive);
        if (!NT_SUCCESS(Status)) return FALSE;

        /* We imported, no need to create a new hive */
        Allocate = FALSE;
    }
    else
    {
        /* FIXME: Create an empty hive */
        Allocate = TRUE;
    }

    /* Attach it to the system key */
    RtlInitUnicodeString(&KeyName, REG_SYSTEM_KEY_NAME);
    Status = CmpLinkHiveToMaster(&KeyName, NULL, SystemHive, Allocate, NULL);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Success! */
    return TRUE;
}

NTSTATUS
NTAPI
CmpCreateObjectTypes(VOID)
{
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    UNICODE_STRING Name;
    PAGED_CODE();

    /* Initialize the Key object type */
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

    /* Create it */
    return ObCreateObjectType(&Name, &ObjectTypeInitializer, NULL, &CmiKeyType);
}

BOOLEAN
NTAPI
CmpCreateRegistryRoot(VOID)
{
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PKEY_OBJECT RootKey;
    HANDLE RootKeyHandle;
    NTSTATUS Status;

    /* Create '\Registry' key. */
    RtlInitUnicodeString(&KeyName, REG_ROOT_KEY_NAME);
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = ObCreateObject(KernelMode,
                            CmiKeyType,
                            &ObjectAttributes,
                            KernelMode,
                            NULL,
                            sizeof(KEY_OBJECT),
                            0,
                            0,
                            (PVOID*)&RootKey);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Setup the root key */
    RootKey->RegistryHive = CmiVolatileHive;
    RootKey->KeyCellOffset = CmiVolatileHive->Hive.HiveHeader->RootCell;
    RootKey->KeyCell = HvGetCell(&CmiVolatileHive->Hive, RootKey->KeyCellOffset);
    RootKey->ParentKey = RootKey;
    RootKey->Flags = 0;
    RootKey->SubKeyCounts = 0;
    RootKey->SubKeys = NULL;
    RootKey->SizeOfSubKeys = 0;
    InsertTailList(&CmiKeyObjectListHead, &RootKey->ListEntry);
    Status = RtlpCreateUnicodeString(&RootKey->Name, L"Registry", NonPagedPool);

    /* Insert the key into the namespace */
    Status = ObInsertObject(RootKey,
                            NULL,
                            KEY_ALL_ACCESS,
                            0,
                            NULL,
                            &RootKeyHandle);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Reference the key again so that we never lose it */
    Status = ObReferenceObjectByHandle(RootKeyHandle,
                                       KEY_READ,
                                       NULL,
                                       KernelMode,
                                       (PVOID*)&RootKey,
                                       NULL);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Completely sucessful */
    return TRUE;
}

BOOLEAN
INIT_FUNCTION
NTAPI
CmInitSystem1(VOID)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    HANDLE KeyHandle;
    NTSTATUS Status;
    LARGE_INTEGER DueTime;
    HANDLE ThreadHandle;
    CLIENT_ID ThreadId;
    PEREGISTRY_HIVE HardwareHive;
    BOOLEAN Allocate = FALSE;
    PVOID BaseAddress;
    ULONG Length;
    PAGED_CODE();

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

    InitializeListHead(&CmiCallbackHead);
    ExInitializeFastMutex(&CmiCallbackLock);

    /* Create the key object types */
    Status = CmpCreateObjectTypes();
    if (!NT_SUCCESS(Status))
    {
        /* Bugcheck */
        KEBUGCHECKEX(CONFIG_INITIALIZATION_FAILED, 1, 1, Status, 0);
    }

    /* Build volatile registry store */
    Status = CmiCreateVolatileHive(&CmiVolatileHive);
    if (!NT_SUCCESS(Status))
    {
        /* Bugcheck */
        KEBUGCHECKEX(CONFIG_INITIALIZATION_FAILED, 1, 2, Status, 0);
    }

    /* Create the \REGISTRY key node */
    if (!CmpCreateRegistryRoot())
    {
        /* Bugcheck */
        KEBUGCHECKEX(CONFIG_INITIALIZATION_FAILED, 1, 4, 0, 0);
    }

    /* Create '\Registry\Machine' key. */
    RtlInitUnicodeString(&KeyName, L"\\REGISTRY\\MACHINE");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtCreateKey(&KeyHandle,
                         KEY_READ | KEY_WRITE,
                         &ObjectAttributes,
                         0,
                         NULL,
                         0,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        /* Bugcheck */
        KEBUGCHECKEX(CONFIG_INITIALIZATION_FAILED, 1, 5, Status, 0);
    }

    /* Close the handle */
    NtClose(KeyHandle);

    /* Create '\Registry\User' key. */
    RtlInitUnicodeString(&KeyName, L"\\REGISTRY\\USER");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtCreateKey(&KeyHandle,
                         KEY_READ | KEY_WRITE,
                         &ObjectAttributes,
                         0,
                         NULL,
                         0,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        /* Bugcheck */
        KEBUGCHECKEX(CONFIG_INITIALIZATION_FAILED, 1, 6, Status, 0);
    }

    /* Close the handle */
    NtClose(KeyHandle);

    /* Initialize the system hive */
    if (!CmpInitializeSystemHive(KeLoaderBlock))
    {
        /* Bugcheck */
        KEBUGCHECKEX(CONFIG_INITIALIZATION_FAILED, 1, 7, 0, 0);
    }

    /* Create the 'CurrentControlSet' link. */
    Status = CmpCreateControlSet(KeLoaderBlock);
    if (!NT_SUCCESS(Status))
    {
        /* Bugcheck */
        KEBUGCHECKEX(CONFIG_INITIALIZATION_FAILED, 1, 8, Status, 0);
    }

    /* Initialize the hardware hive */
    BaseAddress = CmpRosGetHardwareHive(&Length);
    if (!CmImportHardwareHive(BaseAddress, Length, &HardwareHive))
    {
        /* Don't actually link anything below */
        Allocate = TRUE;
    }

    /* Attach it to the machine key */
    RtlInitUnicodeString(&KeyName, REG_HARDWARE_KEY_NAME);
    Status = CmpLinkHiveToMaster(&KeyName, NULL, HardwareHive, FALSE, NULL);
    if (!NT_SUCCESS(Status))
    {
        /* Bugcheck */
        KEBUGCHECKEX(CONFIG_INITIALIZATION_FAILED, 1, 12, Status, 0);
    }

    /* Initialize volatile registry settings */
    Status = CmpSetSystemValues(KeLoaderBlock);
    if (!NT_SUCCESS(Status))
    {
        /* Bugcheck */
        KEBUGCHECKEX(CONFIG_INITIALIZATION_FAILED, 1, 15, Status, 0);
    }

    /* Free the load options */
    ExFreePool(CmpLoadOptions.Buffer);

    /* If we got here, all went well */
    return TRUE;
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
