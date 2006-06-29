/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/cm/ntfunc.c
 * PURPOSE:         Ntxxx function for registry access
 *
 * PROGRAMMERS:     No programmer listed.
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#include "cm.h"


/* GLOBALS ******************************************************************/

extern POBJECT_TYPE  CmiKeyType;
extern PREGISTRY_HIVE  CmiVolatileHive;
extern LIST_ENTRY CmiKeyObjectListHead;

static BOOLEAN CmiRegistryInitialized = FALSE;

LIST_ENTRY CmiCallbackHead;
FAST_MUTEX CmiCallbackLock;

/* FUNCTIONS ****************************************************************/

NTSTATUS
NTAPI
CmpCreateHandle(PVOID ObjectBody,
                ACCESS_MASK GrantedAccess,
                ULONG HandleAttributes,
                PHANDLE HandleReturn)
                /*
                * FUNCTION: Add a handle referencing an object
                * ARGUMENTS:
                *         obj = Object body that the handle should refer to
                * RETURNS: The created handle
                * NOTE: The handle is valid only in the context of the current process
                */
{
    HANDLE_TABLE_ENTRY NewEntry;
    PEPROCESS Process, CurrentProcess;
    POBJECT_HEADER ObjectHeader;
    HANDLE Handle;
    KAPC_STATE ApcState;
    BOOLEAN AttachedToProcess = FALSE;

    PAGED_CODE();

    DPRINT("CmpCreateHandle(obj %p)\n",ObjectBody);

    ASSERT(ObjectBody);

    CurrentProcess = PsGetCurrentProcess();

    ObjectHeader = OBJECT_TO_OBJECT_HEADER(ObjectBody);

    /* check that this is a valid kernel pointer */
    ASSERT((ULONG_PTR)ObjectHeader & EX_HANDLE_ENTRY_LOCKED);

    if (GrantedAccess & MAXIMUM_ALLOWED)
    {
        GrantedAccess &= ~MAXIMUM_ALLOWED;
        GrantedAccess |= GENERIC_ALL;
    }

    if (GrantedAccess & GENERIC_ACCESS)
    {
        RtlMapGenericMask(&GrantedAccess,
            &ObjectHeader->Type->TypeInfo.GenericMapping);
    }

    NewEntry.Object = ObjectHeader;
    if(HandleAttributes & OBJ_INHERIT)
        NewEntry.ObAttributes |= EX_HANDLE_ENTRY_INHERITABLE;
    else
        NewEntry.ObAttributes &= ~EX_HANDLE_ENTRY_INHERITABLE;
    NewEntry.GrantedAccess = GrantedAccess;

    if ((HandleAttributes & OBJ_KERNEL_HANDLE) &&
        ExGetPreviousMode == KernelMode)
    {
        Process = PsInitialSystemProcess;
        if (Process != CurrentProcess)
        {
            KeStackAttachProcess(&Process->Pcb,
                &ApcState);
            AttachedToProcess = TRUE;
        }
    }
    else
    {
        Process = CurrentProcess;
        /* mask out the OBJ_KERNEL_HANDLE attribute */
        HandleAttributes &= ~OBJ_KERNEL_HANDLE;
    }

    Handle = ExCreateHandle(Process->ObjectTable,
        &NewEntry);

    if (AttachedToProcess)
    {
        KeUnstackDetachProcess(&ApcState);
    }

    if(Handle != NULL)
    {
        if (HandleAttributes & OBJ_KERNEL_HANDLE)
        {
            /* mark the handle value */
            Handle = ObMarkHandleAsKernelHandle(Handle);
        }

        if(InterlockedIncrement(&ObjectHeader->HandleCount) == 1)
        {
            ObReferenceObject(ObjectBody);
        }

        *HandleReturn = Handle;

        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;
}

/*
 * @implemented
 */
NTSTATUS STDCALL
CmRegisterCallback(IN PEX_CALLBACK_FUNCTION Function,
                   IN PVOID Context,
                   IN OUT PLARGE_INTEGER Cookie)
{
  PREGISTRY_CALLBACK Callback;

  PAGED_CODE();

  ASSERT(Function && Cookie);

  Callback = ExAllocatePoolWithTag(PagedPool,
                                   sizeof(REGISTRY_CALLBACK),
                                   TAG('C', 'M', 'c', 'b'));
  if(Callback != NULL)
  {
    /* initialize the callback */
    ExInitializeRundownProtection(&Callback->RundownRef);
    Callback->Function = Function;
    Callback->Context = Context;
    Callback->PendingDelete = FALSE;

    /* add it to the callback list and receive a cookie for the callback */
    ExAcquireFastMutex(&CmiCallbackLock);
    /* FIXME - to receive a unique cookie we'll just return the pointer to the
       callback object */
    Callback->Cookie.QuadPart = (ULONG_PTR)Callback;
    InsertTailList(&CmiCallbackHead, &Callback->ListEntry);

    ExReleaseFastMutex(&CmiCallbackLock);

    *Cookie = Callback->Cookie;
    return STATUS_SUCCESS;
  }

  return STATUS_INSUFFICIENT_RESOURCES;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
CmUnRegisterCallback(IN LARGE_INTEGER Cookie)
{
  PLIST_ENTRY CurrentEntry;

  PAGED_CODE();

  ExAcquireFastMutex(&CmiCallbackLock);

  for(CurrentEntry = CmiCallbackHead.Flink;
      CurrentEntry != &CmiCallbackHead;
      CurrentEntry = CurrentEntry->Flink)
  {
    PREGISTRY_CALLBACK CurrentCallback;

    CurrentCallback = CONTAINING_RECORD(CurrentEntry, REGISTRY_CALLBACK, ListEntry);
    if(CurrentCallback->Cookie.QuadPart == Cookie.QuadPart)
    {
      if(!CurrentCallback->PendingDelete)
      {
        /* found the callback, don't unlink it from the list yet so we don't screw
           the calling loop */
        CurrentCallback->PendingDelete = TRUE;
        ExReleaseFastMutex(&CmiCallbackLock);

        /* if the callback is currently executing, wait until it finished */
        ExWaitForRundownProtectionRelease(&CurrentCallback->RundownRef);

        /* time to unlink it. It's now safe because every attempt to acquire a
           runtime protection on this callback will fail */
        ExAcquireFastMutex(&CmiCallbackLock);
        RemoveEntryList(&CurrentCallback->ListEntry);
        ExReleaseFastMutex(&CmiCallbackLock);

        /* free the callback */
        ExFreePool(CurrentCallback);
        return STATUS_SUCCESS;
      }
      else
      {
        /* pending delete, pretend like it already is deleted */
        ExReleaseFastMutex(&CmiCallbackLock);
        return STATUS_UNSUCCESSFUL;
      }
    }
  }

  ExReleaseFastMutex(&CmiCallbackLock);

  return STATUS_UNSUCCESSFUL;
}


NTSTATUS
CmiCallRegisteredCallbacks(IN REG_NOTIFY_CLASS Argument1,
                           IN PVOID Argument2)
{
  PLIST_ENTRY CurrentEntry;
  NTSTATUS Status = STATUS_SUCCESS;

  PAGED_CODE();

  ExAcquireFastMutex(&CmiCallbackLock);

  for(CurrentEntry = CmiCallbackHead.Flink;
      CurrentEntry != &CmiCallbackHead;
      CurrentEntry = CurrentEntry->Flink)
  {
    PREGISTRY_CALLBACK CurrentCallback;

    CurrentCallback = CONTAINING_RECORD(CurrentEntry, REGISTRY_CALLBACK, ListEntry);
    if(!CurrentCallback->PendingDelete &&
       ExAcquireRundownProtection(&CurrentCallback->RundownRef))
    {
      /* don't hold locks during the callbacks! */
      ExReleaseFastMutex(&CmiCallbackLock);

      Status = CurrentCallback->Function(CurrentCallback->Context,
                                         Argument1,
                                         Argument2);

      ExAcquireFastMutex(&CmiCallbackLock);
      /* don't release the rundown protection before holding the callback lock
         so the pointer to the next callback isn't cleared in case this callback
         get's deleted */
      ExReleaseRundownProtection(&CurrentCallback->RundownRef);
      if(!NT_SUCCESS(Status))
      {
        /* one callback returned failure, don't call any more callbacks */
        break;
      }
    }
  }

  ExReleaseFastMutex(&CmiCallbackLock);

  return Status;
}


NTSTATUS STDCALL
NtCreateKey(OUT PHANDLE KeyHandle,
	    IN ACCESS_MASK DesiredAccess,
	    IN POBJECT_ATTRIBUTES ObjectAttributes,
	    IN ULONG TitleIndex,
	    IN PUNICODE_STRING Class,
	    IN ULONG CreateOptions,
	    OUT PULONG Disposition)
{
  UNICODE_STRING RemainingPath = {0};
  BOOLEAN FreeRemainingPath = TRUE;
  ULONG LocalDisposition;
  PKEY_OBJECT KeyObject;
  NTSTATUS Status = STATUS_SUCCESS;
  PVOID Object = NULL;
  PWSTR Start;
  UNICODE_STRING ObjectName;
  OBJECT_CREATE_INFORMATION ObjectCreateInfo;
  unsigned i;
  REG_PRE_CREATE_KEY_INFORMATION PreCreateKeyInfo;
  REG_POST_CREATE_KEY_INFORMATION PostCreateKeyInfo;
  KPROCESSOR_MODE PreviousMode;
  UNICODE_STRING CapturedClass = {0};
  HANDLE hKey;

  PAGED_CODE();

  PreviousMode = ExGetPreviousMode();

  if (PreviousMode != KernelMode)
  {
      _SEH_TRY
      {
          ProbeAndZeroHandle(KeyHandle);
          if (Disposition != NULL)
          {
              ProbeForWriteUlong(Disposition);
          }
      }
      _SEH_HANDLE
      {
          Status = _SEH_GetExceptionCode();
      }
      _SEH_END;
      
      if (!NT_SUCCESS(Status))
      {
          return Status;
      }
  }
  
  if (Class != NULL)
  {
      Status = ProbeAndCaptureUnicodeString(&CapturedClass,
                                            PreviousMode,
                                            Class);
      if (!NT_SUCCESS(Status))
      {
          return Status;
      }
  }

  /* Capture all the info */
  DPRINT("Capturing Create Info\n");
  Status = ObpCaptureObjectAttributes(ObjectAttributes,
                                      PreviousMode,
                                      FALSE,
                                      &ObjectCreateInfo,
                                      &ObjectName);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("ObpCaptureObjectAttributes() failed (Status %lx)\n", Status);
      return Status;
    }

  PostCreateKeyInfo.CompleteName = &ObjectName;
  PreCreateKeyInfo.CompleteName = &ObjectName;
  Status = CmiCallRegisteredCallbacks(RegNtPreCreateKey, &PreCreateKeyInfo);
  if (!NT_SUCCESS(Status))
    {
      goto Cleanup;
    }
    
  Status = CmFindObject(&ObjectCreateInfo,
                        &ObjectName,
                        (PVOID*)&Object,
                        &RemainingPath,
                        CmiKeyType,
                        NULL,
                        NULL);
  if (!NT_SUCCESS(Status))
    {
      PostCreateKeyInfo.Object = NULL;
      PostCreateKeyInfo.Status = Status;
      CmiCallRegisteredCallbacks(RegNtPostCreateKey, &PostCreateKeyInfo);

      DPRINT1("CmpFindObject failed, Status: 0x%x\n", Status);
      goto Cleanup;
    }

  DPRINT("RemainingPath %wZ\n", &RemainingPath);

  if (RemainingPath.Length == 0)
    {
      /* Fail if the key has been deleted */
      if (((PKEY_OBJECT) Object)->Flags & KO_MARKED_FOR_DELETE)
	{
          PostCreateKeyInfo.Object = NULL;
          PostCreateKeyInfo.Status = STATUS_UNSUCCESSFUL;
          CmiCallRegisteredCallbacks(RegNtPostCreateKey, &PostCreateKeyInfo);

	  DPRINT1("Object marked for delete!\n");
	  Status = STATUS_UNSUCCESSFUL;
	  goto Cleanup;
	}

      Status = CmpCreateHandle(Object,
			       DesiredAccess,
			       ObjectCreateInfo.Attributes,
			       &hKey);

      if (!NT_SUCCESS(Status))
        DPRINT1("CmpCreateHandle failed Status 0x%x\n", Status);

      PostCreateKeyInfo.Object = NULL;
      PostCreateKeyInfo.Status = Status;
      CmiCallRegisteredCallbacks(RegNtPostCreateKey, &PostCreateKeyInfo);

      LocalDisposition = REG_OPENED_EXISTING_KEY;
      goto SuccessReturn;
    }

  /* If RemainingPath contains \ we must return error
     because NtCreateKey doesn't create trees */
  Start = RemainingPath.Buffer;
  if (*Start == L'\\')
  {
    Start++;
    //RemainingPath.Length -= sizeof(WCHAR);
    //RemainingPath.MaximumLength -= sizeof(WCHAR);
    //RemainingPath.Buffer++;
    //DPRINT1("String: %wZ\n", &RemainingPath);
  }

  if (RemainingPath.Buffer[(RemainingPath.Length / sizeof(WCHAR)) - 1] == '\\')
  {
    RemainingPath.Buffer[(RemainingPath.Length / sizeof(WCHAR)) - 1] = UNICODE_NULL;
    RemainingPath.Length -= sizeof(WCHAR);
    RemainingPath.MaximumLength -= sizeof(WCHAR);
  }

  for (i = 1; i < RemainingPath.Length / sizeof(WCHAR); i++)
    {
      if (L'\\' == RemainingPath.Buffer[i])
        {
          DPRINT1("NtCreateKey() doesn't create trees! (found \'\\\' in remaining path: \"%wZ\"!)\n", &RemainingPath);

          PostCreateKeyInfo.Object = NULL;
          PostCreateKeyInfo.Status = STATUS_OBJECT_NAME_NOT_FOUND;
          CmiCallRegisteredCallbacks(RegNtPostCreateKey, &PostCreateKeyInfo);

          Status = STATUS_OBJECT_NAME_NOT_FOUND;
          goto Cleanup;
        }
    }

  DPRINT("RemainingPath %S  ParentObject 0x%p\n", RemainingPath.Buffer, Object);

  Status = ObCreateObject(PreviousMode,
			  CmiKeyType,
			  NULL,
			  PreviousMode,
			  NULL,
			  sizeof(KEY_OBJECT),
			  0,
			  0,
			  (PVOID*)&KeyObject);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("ObCreateObject() failed!\n");
      PostCreateKeyInfo.Object = NULL;
      PostCreateKeyInfo.Status = Status;
      CmiCallRegisteredCallbacks(RegNtPostCreateKey, &PostCreateKeyInfo);

      goto Cleanup;
    }

  Status = ObInsertObject((PVOID)KeyObject,
			  NULL,
			  DesiredAccess,
			  0,
			  NULL,
			  &hKey);
  if (!NT_SUCCESS(Status))
    {
      ObDereferenceObject(KeyObject);
      DPRINT1("ObInsertObject() failed!\n");

      PostCreateKeyInfo.Object = NULL;
      PostCreateKeyInfo.Status = Status;
      CmiCallRegisteredCallbacks(RegNtPostCreateKey, &PostCreateKeyInfo);

      goto Cleanup;
    }

  KeyObject->ParentKey = Object;

  if (CreateOptions & REG_OPTION_VOLATILE)
    KeyObject->RegistryHive = CmiVolatileHive;
  else
    KeyObject->RegistryHive = KeyObject->ParentKey->RegistryHive;

  KeyObject->Flags = 0;
  KeyObject->NumberOfSubKeys = 0;
  KeyObject->SizeOfSubKeys = 0;
  KeyObject->SubKeys = NULL;

  /* Acquire hive lock */
  KeEnterCriticalRegion();
  ExAcquireResourceExclusiveLite(&CmiRegistryLock, TRUE);

  InsertTailList(&CmiKeyObjectListHead, &KeyObject->ListEntry);

  /* add key to subkeys of parent if needed */
  Status = CmiAddSubKey(KeyObject->RegistryHive,
			KeyObject->ParentKey,
			KeyObject,
			&RemainingPath,
			TitleIndex,
			&CapturedClass,
			CreateOptions);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("CmiAddSubKey() failed (Status %lx)\n", Status);
      /* Release hive lock */
      ExReleaseResourceLite(&CmiRegistryLock);
      KeLeaveCriticalRegion();
      ObDereferenceObject(KeyObject);

      PostCreateKeyInfo.Object = NULL;
      PostCreateKeyInfo.Status = STATUS_UNSUCCESSFUL;
      CmiCallRegisteredCallbacks(RegNtPostCreateKey, &PostCreateKeyInfo);

      Status = STATUS_UNSUCCESSFUL;
      goto Cleanup;
    }

  if (Start == RemainingPath.Buffer)
    {
      KeyObject->Name = RemainingPath;
      FreeRemainingPath = FALSE;
    }
  else
    {
      RtlpCreateUnicodeString(&KeyObject->Name, Start, NonPagedPool);
    }

  if (KeyObject->RegistryHive == KeyObject->ParentKey->RegistryHive)
    {
      KeyObject->KeyCell->ParentKeyOffset = KeyObject->ParentKey->KeyCellOffset;
      KeyObject->KeyCell->SecurityKeyOffset = KeyObject->ParentKey->KeyCell->SecurityKeyOffset;
    }
  else
    {
      KeyObject->KeyCell->ParentKeyOffset = -1;
      KeyObject->KeyCell->SecurityKeyOffset = -1;
      /* This key must remain in memory unless it is deleted
	 or file is unloaded */
      ObReferenceObject(KeyObject);
    }

  CmiAddKeyToList(KeyObject->ParentKey, KeyObject);

  VERIFY_KEY_OBJECT(KeyObject);

  /* Release hive lock */
  ExReleaseResourceLite(&CmiRegistryLock);
  KeLeaveCriticalRegion();

  PostCreateKeyInfo.Object = KeyObject;
  PostCreateKeyInfo.Status = Status;
  CmiCallRegisteredCallbacks(RegNtPostCreateKey, &PostCreateKeyInfo);

  CmiSyncHives();
  
  LocalDisposition = REG_CREATED_NEW_KEY;

SuccessReturn:
  _SEH_TRY
  {
      *KeyHandle = hKey;
      if (Disposition != NULL)
      {
          *Disposition = LocalDisposition;
      }
  }
  _SEH_HANDLE
  {
      Status = _SEH_GetExceptionCode();
  }
  _SEH_END;

Cleanup:
  ObpReleaseCapturedAttributes(&ObjectCreateInfo);
  if (Class != NULL)
  {
    ReleaseCapturedUnicodeString(&CapturedClass,
                                 PreviousMode);
  }
  if (ObjectName.Buffer) ObpReleaseCapturedName(&ObjectName);
  if (FreeRemainingPath) RtlFreeUnicodeString(&RemainingPath);
  //if (Object != NULL) ObDereferenceObject(Object);

  return Status;
}


NTSTATUS STDCALL
NtDeleteKey(IN HANDLE KeyHandle)
{
  KPROCESSOR_MODE PreviousMode;
  PKEY_OBJECT KeyObject;
  NTSTATUS Status;
  REG_DELETE_KEY_INFORMATION DeleteKeyInfo;
  REG_POST_OPERATION_INFORMATION PostOperationInfo;

  PAGED_CODE();

  DPRINT("NtDeleteKey(KeyHandle 0x%p) called\n", KeyHandle);

  PreviousMode = ExGetPreviousMode();

  /* Verify that the handle is valid and is a registry key */
  Status = ObReferenceObjectByHandle(KeyHandle,
				     DELETE,
				     CmiKeyType,
				     PreviousMode,
				     (PVOID *)&KeyObject,
				     NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("ObReferenceObjectByHandle() failed (Status %lx)\n", Status);
      return Status;
    }

  PostOperationInfo.Object = (PVOID)KeyObject;
  DeleteKeyInfo.Object = (PVOID)KeyObject;
  Status = CmiCallRegisteredCallbacks(RegNtPreSetValueKey, &DeleteKeyInfo);
  if (!NT_SUCCESS(Status))
    {
      PostOperationInfo.Status = Status;
      CmiCallRegisteredCallbacks(RegNtDeleteKey, &PostOperationInfo);
      ObDereferenceObject(KeyObject);
      return Status;
    }

  /* Acquire hive lock */
  KeEnterCriticalRegion();
  ExAcquireResourceExclusiveLite(&CmiRegistryLock, TRUE);

  VERIFY_KEY_OBJECT(KeyObject);

  /* Check for subkeys */
  if (KeyObject->NumberOfSubKeys != 0)
    {
      Status = STATUS_CANNOT_DELETE;
    }
  else
    {
      /* Set the marked for delete bit in the key object */
      KeyObject->Flags |= KO_MARKED_FOR_DELETE;
      Status = STATUS_SUCCESS;
    }

  /* Release hive lock */
  ExReleaseResourceLite(&CmiRegistryLock);
  KeLeaveCriticalRegion();

  DPRINT("PointerCount %lu\n", ObGetObjectPointerCount((PVOID)KeyObject));

  /* Remove the keep-alive reference */
  ObDereferenceObject(KeyObject);

  if (KeyObject->RegistryHive != KeyObject->ParentKey->RegistryHive)
    ObDereferenceObject(KeyObject);

  PostOperationInfo.Status = Status;
  CmiCallRegisteredCallbacks(RegNtPostDeleteKey, &PostOperationInfo);

  /* Dereference the object */
  ObDereferenceObject(KeyObject);

  DPRINT("PointerCount %lu\n", ObGetObjectPointerCount((PVOID)KeyObject));
  DPRINT("HandleCount %lu\n", ObGetObjectHandleCount((PVOID)KeyObject));

  /*
   * Note:
   * Hive-Synchronization will not be triggered here. This is done in
   * CmiObjectDelete() (in regobj.c) after all key-related structures
   * have been released.
   */

  return Status;
}


NTSTATUS STDCALL
NtEnumerateKey(IN HANDLE KeyHandle,
	       IN ULONG Index,
	       IN KEY_INFORMATION_CLASS KeyInformationClass,
	       OUT PVOID KeyInformation,
	       IN ULONG Length,
	       OUT PULONG ResultLength)
{
  PKEY_OBJECT KeyObject;
  PKEY_OBJECT SubKeyObject;
  PREGISTRY_HIVE  RegistryHive;
  PKEY_CELL  KeyCell, SubKeyCell;
  PHASH_TABLE_CELL  HashTableBlock;
  PKEY_BASIC_INFORMATION  BasicInformation;
  PKEY_NODE_INFORMATION  NodeInformation;
  PKEY_FULL_INFORMATION  FullInformation;
  PDATA_CELL ClassCell;
  ULONG NameSize, ClassSize;
  KPROCESSOR_MODE PreviousMode;
  NTSTATUS Status;
  REG_ENUMERATE_KEY_INFORMATION EnumerateKeyInfo;
  REG_POST_OPERATION_INFORMATION PostOperationInfo;

  PAGED_CODE();

  PreviousMode = ExGetPreviousMode();

  DPRINT("KH 0x%p  I %d  KIC %x KI 0x%p  L %d  RL 0x%p\n",
	 KeyHandle,
	 Index,
	 KeyInformationClass,
	 KeyInformation,
	 Length,
	 ResultLength);

  /* Verify that the handle is valid and is a registry key */
  Status = ObReferenceObjectByHandle(KeyHandle,
		KEY_ENUMERATE_SUB_KEYS,
		CmiKeyType,
		PreviousMode,
		(PVOID *) &KeyObject,
		NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("ObReferenceObjectByHandle() failed with status %x\n", Status);
      return(Status);
    }

  PostOperationInfo.Object = (PVOID)KeyObject;
  EnumerateKeyInfo.Object = (PVOID)KeyObject;
  EnumerateKeyInfo.Index = Index;
  EnumerateKeyInfo.KeyInformationClass = KeyInformationClass;
  EnumerateKeyInfo.Length = Length;
  EnumerateKeyInfo.ResultLength = ResultLength;

  Status = CmiCallRegisteredCallbacks(RegNtEnumerateKey, &EnumerateKeyInfo);
  if (!NT_SUCCESS(Status))
    {
      ObDereferenceObject(KeyObject);
      return Status;
    }

  /* Acquire hive lock */
  KeEnterCriticalRegion();
  ExAcquireResourceSharedLite(&CmiRegistryLock, TRUE);

  VERIFY_KEY_OBJECT(KeyObject);

  /* Get pointer to KeyCell */
  KeyCell = KeyObject->KeyCell;
  RegistryHive = KeyObject->RegistryHive;

  SubKeyObject = NULL;

  /* Check for hightest possible sub key index */
  if (Index >= KeyCell->NumberOfSubKeys + KeyObject->NumberOfSubKeys)
    {
      ExReleaseResourceLite(&CmiRegistryLock);
      KeLeaveCriticalRegion();
      PostOperationInfo.Status = STATUS_NO_MORE_ENTRIES;
      CmiCallRegisteredCallbacks(RegNtPostDeleteKey, &PostOperationInfo);
      ObDereferenceObject(KeyObject);
      DPRINT("No more volatile entries\n");
      return STATUS_NO_MORE_ENTRIES;
    }

  /* Get pointer to SubKey */
  if (Index >= KeyCell->NumberOfSubKeys)
    {
      PKEY_OBJECT CurKey = NULL;
      ULONG i;
      ULONG j;

      /* Search for volatile or 'foreign' keys */
      j = KeyCell->NumberOfSubKeys;
      for (i = 0; i < KeyObject->NumberOfSubKeys; i++)
	{
	  CurKey = KeyObject->SubKeys[i];
	  if (CurKey->RegistryHive != RegistryHive)
	    {
	      if (j == Index)
		break;
	      j++;
	    }
	}

      if (i >= KeyObject->NumberOfSubKeys)
	{
	  ExReleaseResourceLite(&CmiRegistryLock);
	  KeLeaveCriticalRegion();
          PostOperationInfo.Status = STATUS_NO_MORE_ENTRIES;
          CmiCallRegisteredCallbacks(RegNtPostEnumerateKey, &PostOperationInfo);
	  ObDereferenceObject(KeyObject);
	  DPRINT("No more non-volatile entries\n");
	  return STATUS_NO_MORE_ENTRIES;
	}

      SubKeyObject = CurKey;
      SubKeyCell = CurKey->KeyCell;
    }
  else
    {
      if (KeyCell->HashTableOffset == (BLOCK_OFFSET)-1)
	{
	  ExReleaseResourceLite(&CmiRegistryLock);
	  KeLeaveCriticalRegion();
          PostOperationInfo.Status = STATUS_NO_MORE_ENTRIES;
          CmiCallRegisteredCallbacks(RegNtPostEnumerateKey, &PostOperationInfo);
	  ObDereferenceObject(KeyObject);
	  return STATUS_NO_MORE_ENTRIES;
	}

      HashTableBlock = CmiGetCell (RegistryHive, KeyCell->HashTableOffset, NULL);
      if (HashTableBlock == NULL)
	{
	  DPRINT("CmiGetBlock() failed\n");
	  ExReleaseResourceLite(&CmiRegistryLock);
	  KeLeaveCriticalRegion();
          PostOperationInfo.Status = STATUS_UNSUCCESSFUL;
          CmiCallRegisteredCallbacks(RegNtPostEnumerateKey, &PostOperationInfo);
	  ObDereferenceObject(KeyObject);
	  return STATUS_UNSUCCESSFUL;
	}

      SubKeyCell = CmiGetKeyFromHashByIndex(RegistryHive,
					    HashTableBlock,
					    Index);
    }

  if (SubKeyCell == NULL)
    {
      ExReleaseResourceLite(&CmiRegistryLock);
      KeLeaveCriticalRegion();
      PostOperationInfo.Status = STATUS_NO_MORE_ENTRIES;
      CmiCallRegisteredCallbacks(RegNtPostEnumerateKey, &PostOperationInfo);
      ObDereferenceObject(KeyObject);
      DPRINT("No more entries\n");
      return STATUS_NO_MORE_ENTRIES;
    }

  Status = STATUS_SUCCESS;
  switch (KeyInformationClass)
    {
      case KeyBasicInformation:
	/* Check size of buffer */
	if (SubKeyObject != NULL)
	  {
	    NameSize = SubKeyObject->Name.Length;
	  }
	else
	  {
	    NameSize = SubKeyCell->NameSize;
	    if (SubKeyCell->Flags & REG_KEY_NAME_PACKED)
	      {
		NameSize *= sizeof(WCHAR);
	      }
	  }

	*ResultLength = FIELD_OFFSET(KEY_BASIC_INFORMATION, Name[0]) + NameSize;

	/*
	 * NOTE: It's perfetly valid to call NtEnumerateKey to get
         * all the information but name. Actually the NT4 sound
         * framework does that while querying parameters from registry.
         * -- Filip Navara, 19/07/2004
         */
	if (Length < FIELD_OFFSET(KEY_BASIC_INFORMATION, Name[0]))
	  {
	    Status = STATUS_BUFFER_TOO_SMALL;
	  }
	else
	  {
	    /* Fill buffer with requested info */
	    BasicInformation = (PKEY_BASIC_INFORMATION) KeyInformation;
	    BasicInformation->LastWriteTime.u.LowPart = SubKeyCell->LastWriteTime.u.LowPart;
	    BasicInformation->LastWriteTime.u.HighPart = SubKeyCell->LastWriteTime.u.HighPart;
	    BasicInformation->TitleIndex = Index;
	    BasicInformation->NameLength = NameSize;

	    if (Length - FIELD_OFFSET(KEY_BASIC_INFORMATION, Name[0]) < NameSize)
	      {
	        NameSize = Length - FIELD_OFFSET(KEY_BASIC_INFORMATION, Name[0]);
	        Status = STATUS_BUFFER_OVERFLOW;
	        CHECKPOINT;
	      }

	    if (SubKeyObject != NULL)
	      {
		RtlCopyMemory(BasicInformation->Name,
			      SubKeyObject->Name.Buffer,
			      NameSize);
	      }
	    else
	      {
		if (SubKeyCell->Flags & REG_KEY_NAME_PACKED)
		  {
		    CmiCopyPackedName(BasicInformation->Name,
				      SubKeyCell->Name,
				      NameSize / sizeof(WCHAR));
		  }
		else
		  {
		    RtlCopyMemory(BasicInformation->Name,
				  SubKeyCell->Name,
				  NameSize);
		  }
	      }
	  }
	break;

      case KeyNodeInformation:
	/* Check size of buffer */
	if (SubKeyObject != NULL)
	  {
	    NameSize = SubKeyObject->Name.Length;
	  }
	else
	  {
	    NameSize = SubKeyCell->NameSize;
	    if (SubKeyCell->Flags & REG_KEY_NAME_PACKED)
	      {
		NameSize *= sizeof(WCHAR);
	      }
	  }
	ClassSize = SubKeyCell->ClassSize;

	*ResultLength = FIELD_OFFSET(KEY_NODE_INFORMATION, Name[0]) +
	  NameSize + ClassSize;

	if (Length < FIELD_OFFSET(KEY_NODE_INFORMATION, Name[0]))
	  {
	    Status = STATUS_BUFFER_TOO_SMALL;
	  }
	else
	  {
	    /* Fill buffer with requested info */
	    NodeInformation = (PKEY_NODE_INFORMATION) KeyInformation;
	    NodeInformation->LastWriteTime.u.LowPart = SubKeyCell->LastWriteTime.u.LowPart;
	    NodeInformation->LastWriteTime.u.HighPart = SubKeyCell->LastWriteTime.u.HighPart;
	    NodeInformation->TitleIndex = Index;
	    NodeInformation->ClassOffset = sizeof(KEY_NODE_INFORMATION) + NameSize;
	    NodeInformation->ClassLength = SubKeyCell->ClassSize;
	    NodeInformation->NameLength = NameSize;

	    if (Length - FIELD_OFFSET(KEY_NODE_INFORMATION, Name[0]) < NameSize)
	      {
	        NameSize = Length - FIELD_OFFSET(KEY_NODE_INFORMATION, Name[0]);
	        ClassSize = 0;
	        Status = STATUS_BUFFER_OVERFLOW;
	        CHECKPOINT;
	      }
	    else if (Length - FIELD_OFFSET(KEY_NODE_INFORMATION, Name[0]) -
	             NameSize < ClassSize)
	      {
	        ClassSize = Length - FIELD_OFFSET(KEY_NODE_INFORMATION, Name[0]) -
	                    NameSize;
	        Status = STATUS_BUFFER_OVERFLOW;
	        CHECKPOINT;
	      }

	    if (SubKeyObject != NULL)
	      {
		RtlCopyMemory(NodeInformation->Name,
			      SubKeyObject->Name.Buffer,
			      NameSize);
	      }
	    else
	      {
		if (SubKeyCell->Flags & REG_KEY_NAME_PACKED)
		  {
		    CmiCopyPackedName(NodeInformation->Name,
				      SubKeyCell->Name,
				      NameSize / sizeof(WCHAR));
		  }
		else
		  {
		    RtlCopyMemory(NodeInformation->Name,
				  SubKeyCell->Name,
				  NameSize);
		  }
	      }

	    if (ClassSize != 0)
	      {
		ClassCell = CmiGetCell (KeyObject->RegistryHive,
					SubKeyCell->ClassNameOffset,
					NULL);
		RtlCopyMemory (NodeInformation->Name + SubKeyCell->NameSize,
			       ClassCell->Data,
			       ClassSize);
	      }
	  }
	break;

      case KeyFullInformation:
	ClassSize = SubKeyCell->ClassSize;

	*ResultLength = FIELD_OFFSET(KEY_FULL_INFORMATION, Class[0]) +
	  ClassSize;

	/* Check size of buffer */
	if (Length < FIELD_OFFSET(KEY_FULL_INFORMATION, Class[0]))
	  {
	    Status = STATUS_BUFFER_TOO_SMALL;
	  }
	else
	  {
	    /* Fill buffer with requested info */
	    FullInformation = (PKEY_FULL_INFORMATION) KeyInformation;
	    FullInformation->LastWriteTime.u.LowPart = SubKeyCell->LastWriteTime.u.LowPart;
	    FullInformation->LastWriteTime.u.HighPart = SubKeyCell->LastWriteTime.u.HighPart;
	    FullInformation->TitleIndex = Index;
	    FullInformation->ClassOffset = sizeof(KEY_FULL_INFORMATION) -
	      sizeof(WCHAR);
	    FullInformation->ClassLength = SubKeyCell->ClassSize;
	    FullInformation->SubKeys = CmiGetNumberOfSubKeys(KeyObject); //SubKeyCell->NumberOfSubKeys;
	    FullInformation->MaxNameLen = CmiGetMaxNameLength(KeyObject);
	    FullInformation->MaxClassLen = CmiGetMaxClassLength(KeyObject);
	    FullInformation->Values = SubKeyCell->NumberOfValues;
	    FullInformation->MaxValueNameLen =
	      CmiGetMaxValueNameLength(RegistryHive, SubKeyCell);
	    FullInformation->MaxValueDataLen =
	      CmiGetMaxValueDataLength(RegistryHive, SubKeyCell);

	    if (Length - FIELD_OFFSET(KEY_FULL_INFORMATION, Class[0]) < ClassSize)
	      {
	        ClassSize = Length - FIELD_OFFSET(KEY_FULL_INFORMATION, Class[0]);
	        Status = STATUS_BUFFER_OVERFLOW;
	        CHECKPOINT;
	      }

	    if (ClassSize != 0)
	      {
		ClassCell = CmiGetCell (KeyObject->RegistryHive,
					SubKeyCell->ClassNameOffset,
					NULL);
		RtlCopyMemory (FullInformation->Class,
			       ClassCell->Data,
			       ClassSize);
	      }
	  }
	break;

      default:
	DPRINT1("Not handling 0x%x\n", KeyInformationClass);
	break;
    }

  ExReleaseResourceLite(&CmiRegistryLock);
  KeLeaveCriticalRegion();

  PostOperationInfo.Status = Status;
  CmiCallRegisteredCallbacks(RegNtPostEnumerateKey, &PostOperationInfo);

  ObDereferenceObject(KeyObject);

  DPRINT("Returning status %x\n", Status);

  return(Status);
}


NTSTATUS STDCALL
NtEnumerateValueKey(IN HANDLE KeyHandle,
	IN ULONG Index,
	IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
	OUT PVOID KeyValueInformation,
	IN ULONG Length,
	OUT PULONG ResultLength)
{
  NTSTATUS  Status;
  PKEY_OBJECT  KeyObject;
  PREGISTRY_HIVE  RegistryHive;
  PKEY_CELL  KeyCell;
  PVALUE_CELL  ValueCell;
  PDATA_CELL  DataCell;
  ULONG NameSize, DataSize;
  PKEY_VALUE_BASIC_INFORMATION  ValueBasicInformation;
  PKEY_VALUE_PARTIAL_INFORMATION  ValuePartialInformation;
  PKEY_VALUE_FULL_INFORMATION  ValueFullInformation;

  PAGED_CODE();

  DPRINT("KH 0x%p  I %d  KVIC %x  KVI 0x%p  L %d  RL 0x%p\n",
	 KeyHandle,
	 Index,
	 KeyValueInformationClass,
	 KeyValueInformation,
	 Length,
	 ResultLength);

  /*  Verify that the handle is valid and is a registry key  */
  Status = ObReferenceObjectByHandle(KeyHandle,
		KEY_QUERY_VALUE,
		CmiKeyType,
		ExGetPreviousMode(),
		(PVOID *) &KeyObject,
		NULL);

  if (!NT_SUCCESS(Status))
    {
      return Status;
    }

  /* Acquire hive lock */
  KeEnterCriticalRegion();
  ExAcquireResourceSharedLite(&CmiRegistryLock, TRUE);

  VERIFY_KEY_OBJECT(KeyObject);

  /* Get pointer to KeyCell */
  KeyCell = KeyObject->KeyCell;
  RegistryHive = KeyObject->RegistryHive;

  /* Get Value block of interest */
  Status = CmiGetValueFromKeyByIndex(RegistryHive,
		KeyCell,
		Index,
		&ValueCell);

  if (!NT_SUCCESS(Status))
    {
      ExReleaseResourceLite(&CmiRegistryLock);
      KeLeaveCriticalRegion();
      ObDereferenceObject(KeyObject);
      return Status;
    }

  if (ValueCell != NULL)
    {
      switch (KeyValueInformationClass)
        {
        case KeyValueBasicInformation:
	  NameSize = ValueCell->NameSize;
	  if (ValueCell->Flags & REG_VALUE_NAME_PACKED)
	    {
	      NameSize *= sizeof(WCHAR);
	    }

          *ResultLength = FIELD_OFFSET(KEY_VALUE_BASIC_INFORMATION, Name[0]) + NameSize;

          if (Length < FIELD_OFFSET(KEY_VALUE_BASIC_INFORMATION, Name[0]))
            {
              Status = STATUS_BUFFER_TOO_SMALL;
            }
          else
            {
              ValueBasicInformation = (PKEY_VALUE_BASIC_INFORMATION)
                KeyValueInformation;
              ValueBasicInformation->TitleIndex = 0;
              ValueBasicInformation->Type = ValueCell->DataType;
	      ValueBasicInformation->NameLength = NameSize;

	      if (Length - FIELD_OFFSET(KEY_VALUE_BASIC_INFORMATION, Name[0]) <
	          NameSize)
	        {
	          NameSize = Length - FIELD_OFFSET(KEY_VALUE_BASIC_INFORMATION, Name[0]);
	          Status = STATUS_BUFFER_OVERFLOW;
	          CHECKPOINT;
	        }

              if (ValueCell->Flags & REG_VALUE_NAME_PACKED)
                {
                  CmiCopyPackedName(ValueBasicInformation->Name,
                                    ValueCell->Name,
                                    NameSize / sizeof(WCHAR));
                }
              else
                {
                  RtlCopyMemory(ValueBasicInformation->Name,
                                ValueCell->Name,
                                NameSize);
                }
            }
          break;

        case KeyValuePartialInformation:
          DataSize = ValueCell->DataSize & REG_DATA_SIZE_MASK;

          *ResultLength = FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data[0]) +
            DataSize;

          if (Length < FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data[0]))
            {
              Status = STATUS_BUFFER_TOO_SMALL;
            }
          else
            {
              ValuePartialInformation = (PKEY_VALUE_PARTIAL_INFORMATION)
                KeyValueInformation;
              ValuePartialInformation->TitleIndex = 0;
              ValuePartialInformation->Type = ValueCell->DataType;
              ValuePartialInformation->DataLength = ValueCell->DataSize & REG_DATA_SIZE_MASK;

              if (Length - FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data[0]) <
                  DataSize)
                {
                  DataSize = Length - FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data[0]);
                  Status = STATUS_BUFFER_OVERFLOW;
                  CHECKPOINT;
                }

              if (!(ValueCell->DataSize & REG_DATA_IN_OFFSET))
              {
                DataCell = CmiGetCell (RegistryHive, ValueCell->DataOffset, NULL);
                RtlCopyMemory(ValuePartialInformation->Data,
                  DataCell->Data,
                  DataSize);
              }
              else
              {
                RtlCopyMemory(ValuePartialInformation->Data,
                  &ValueCell->DataOffset,
                  DataSize);
              }
            }
          break;

        case KeyValueFullInformation:
	  NameSize = ValueCell->NameSize;
          if (ValueCell->Flags & REG_VALUE_NAME_PACKED)
            {
	      NameSize *= sizeof(WCHAR);
	    }
	  DataSize = ValueCell->DataSize & REG_DATA_SIZE_MASK;

          *ResultLength = ROUND_UP(FIELD_OFFSET(KEY_VALUE_FULL_INFORMATION,
                          Name[0]) + NameSize, sizeof(PVOID)) + DataSize;

          if (Length < FIELD_OFFSET(KEY_VALUE_FULL_INFORMATION, Name[0]))
            {
              Status = STATUS_BUFFER_TOO_SMALL;
            }
          else
            {
              ValueFullInformation = (PKEY_VALUE_FULL_INFORMATION)
                KeyValueInformation;
              ValueFullInformation->TitleIndex = 0;
              ValueFullInformation->Type = ValueCell->DataType;
              ValueFullInformation->NameLength = NameSize;
              ValueFullInformation->DataOffset =
                (ULONG_PTR)ValueFullInformation->Name -
                (ULONG_PTR)ValueFullInformation +
                ValueFullInformation->NameLength;
              ValueFullInformation->DataOffset =
                  ROUND_UP(ValueFullInformation->DataOffset, sizeof(PVOID));
              ValueFullInformation->DataLength = ValueCell->DataSize & REG_DATA_SIZE_MASK;

              if (Length < ValueFullInformation->DataOffset)
	        {
	          NameSize = Length - FIELD_OFFSET(KEY_VALUE_FULL_INFORMATION, Name[0]);
	          DataSize = 0;
	          Status = STATUS_BUFFER_OVERFLOW;
	          CHECKPOINT;
	        }
              else if (Length - ValueFullInformation->DataOffset < DataSize) 
	        {
	          DataSize = Length - ValueFullInformation->DataOffset;
	          Status = STATUS_BUFFER_OVERFLOW;
	          CHECKPOINT;
	        }

              if (ValueCell->Flags & REG_VALUE_NAME_PACKED)
                {
                  CmiCopyPackedName(ValueFullInformation->Name,
				    ValueCell->Name,
				    NameSize / sizeof(WCHAR));
                }
              else
                {
                  RtlCopyMemory(ValueFullInformation->Name,
				ValueCell->Name,
				NameSize);
                }

              if (!(ValueCell->DataSize & REG_DATA_IN_OFFSET))
                {
                  DataCell = CmiGetCell (RegistryHive, ValueCell->DataOffset, NULL);
                  RtlCopyMemory((PCHAR) ValueFullInformation
                    + ValueFullInformation->DataOffset,
                    DataCell->Data, DataSize);
                }
              else
                {
                  RtlCopyMemory((PCHAR) ValueFullInformation
                    + ValueFullInformation->DataOffset,
                    &ValueCell->DataOffset, DataSize);
                }
            }
          break;

          default:
            DPRINT1("Not handling 0x%x\n", KeyValueInformationClass);
        	break;
        }
    }
  else
    {
      Status = STATUS_UNSUCCESSFUL;
    }

  ExReleaseResourceLite(&CmiRegistryLock);
  KeLeaveCriticalRegion();
  ObDereferenceObject(KeyObject);

  return Status;
}


NTSTATUS STDCALL
NtFlushKey(IN HANDLE KeyHandle)
{
  NTSTATUS Status;
  PKEY_OBJECT  KeyObject;
  PREGISTRY_HIVE  RegistryHive;
  KPROCESSOR_MODE  PreviousMode;

  PAGED_CODE();

  DPRINT("NtFlushKey (KeyHandle %lx) called\n", KeyHandle);

  PreviousMode = ExGetPreviousMode();

  /* Verify that the handle is valid and is a registry key */
  Status = ObReferenceObjectByHandle(KeyHandle,
				     0,
				     CmiKeyType,
				     PreviousMode,
				     (PVOID *)&KeyObject,
				     NULL);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  VERIFY_KEY_OBJECT(KeyObject);

  RegistryHive = KeyObject->RegistryHive;

  /* Acquire hive lock */
  KeEnterCriticalRegion();
  ExAcquireResourceExclusiveLite(&CmiRegistryLock, TRUE);

  if (IsNoFileHive(RegistryHive))
    {
      Status = STATUS_SUCCESS;
    }
  else
    {
      /* Flush non-volatile hive */
      Status = CmiFlushRegistryHive(RegistryHive);
    }

  ExReleaseResourceLite(&CmiRegistryLock);
  KeLeaveCriticalRegion();

  ObDereferenceObject(KeyObject);

  return STATUS_SUCCESS;
}


NTSTATUS STDCALL
NtOpenKey(OUT PHANDLE KeyHandle,
	  IN ACCESS_MASK DesiredAccess,
	  IN POBJECT_ATTRIBUTES ObjectAttributes)
{
  UNICODE_STRING RemainingPath;
  KPROCESSOR_MODE PreviousMode;
  PVOID Object = NULL;
  HANDLE hKey = NULL;
  NTSTATUS Status = STATUS_SUCCESS;
  UNICODE_STRING ObjectName;
  OBJECT_CREATE_INFORMATION ObjectCreateInfo;
  REG_PRE_OPEN_KEY_INFORMATION PreOpenKeyInfo;
  REG_POST_OPEN_KEY_INFORMATION PostOpenKeyInfo;

  PAGED_CODE();

  DPRINT("NtOpenKey(KH 0x%p  DA %x  OA 0x%p  OA->ON '%wZ'\n",
	 KeyHandle,
	 DesiredAccess,
	 ObjectAttributes,
	 ObjectAttributes ? ObjectAttributes->ObjectName : NULL);

  /* Check place for result handle, if it's null - return immediately */
  if (KeyHandle == NULL)
	  return(STATUS_INVALID_PARAMETER);

  PreviousMode = ExGetPreviousMode();

  if(PreviousMode != KernelMode)
  {
    _SEH_TRY
    {
      ProbeAndZeroHandle(KeyHandle);
    }
    _SEH_HANDLE
    {
      Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

    if(!NT_SUCCESS(Status))
    {
      return Status;
    }
  }

    if (ObjectAttributes->ObjectName->Buffer[(ObjectAttributes->ObjectName->Length / sizeof(WCHAR)) - 1] == '\\')
  {
    ObjectAttributes->ObjectName->Buffer[(ObjectAttributes->ObjectName->Length / sizeof(WCHAR)) - 1] = UNICODE_NULL;
    ObjectAttributes->ObjectName->Length -= sizeof(WCHAR);
    ObjectAttributes->ObjectName->MaximumLength -= sizeof(WCHAR);
  }

  /* WINE checks for the length also */
  /*if (ObjectAttributes->ObjectName->Length > MAX_NAME_LENGTH)
	  return(STATUS_BUFFER_OVERFLOW);*/

      /* Capture all the info */
   DPRINT("Capturing Create Info\n");
   Status = ObpCaptureObjectAttributes(ObjectAttributes,
                                       PreviousMode,
                                       FALSE,
                                       &ObjectCreateInfo,
                                       &ObjectName);
   if (!NT_SUCCESS(Status))
     {
	DPRINT("ObpCaptureObjectAttributes() failed (Status %lx)\n", Status);
	return Status;
     }

  PostOpenKeyInfo.CompleteName = &ObjectName;
  PreOpenKeyInfo.CompleteName = &ObjectName;
  Status = CmiCallRegisteredCallbacks(RegNtPreOpenKey, &PreOpenKeyInfo);
  if (!NT_SUCCESS(Status))
    {
      ObpReleaseCapturedAttributes(&ObjectCreateInfo);
      if (ObjectName.Buffer) ObpReleaseCapturedName(&ObjectName);
      return Status;
    }


  RemainingPath.Buffer = NULL;

  Status = CmFindObject(&ObjectCreateInfo,
                        &ObjectName,
	                (PVOID*)&Object,
                        &RemainingPath,
                        CmiKeyType,
                        NULL,
                        NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("CmpFindObject() returned 0x%08lx\n", Status);
      Status = STATUS_INVALID_HANDLE; /* Because ObFindObject returns STATUS_UNSUCCESSFUL */
      goto openkey_cleanup;
    }

  VERIFY_KEY_OBJECT((PKEY_OBJECT) Object);

  DPRINT("RemainingPath '%wZ'\n", &RemainingPath);

  if ((RemainingPath.Buffer != NULL) && (RemainingPath.Buffer[0] != 0))
    {
      RtlFreeUnicodeString(&RemainingPath);
      Status = STATUS_OBJECT_NAME_NOT_FOUND;
      goto openkey_cleanup;
    }

  RtlFreeUnicodeString(&RemainingPath);

  /* Fail if the key has been deleted */
  if (((PKEY_OBJECT)Object)->Flags & KO_MARKED_FOR_DELETE)
    {
      Status = STATUS_UNSUCCESSFUL;
      goto openkey_cleanup;
    }

  Status = CmpCreateHandle(Object,
			   DesiredAccess,
			   ObjectCreateInfo.Attributes,
			   &hKey);

openkey_cleanup:

  ObpReleaseCapturedAttributes(&ObjectCreateInfo);
  PostOpenKeyInfo.Object = NT_SUCCESS(Status) ? (PVOID)Object : NULL;
  PostOpenKeyInfo.Status = Status;
  CmiCallRegisteredCallbacks (RegNtPostOpenKey, &PostOpenKeyInfo);
  if (ObjectName.Buffer) ObpReleaseCapturedName(&ObjectName);

  if (Object)
    {
      ObDereferenceObject(Object);
    }

  if (NT_SUCCESS(Status))
  {
    _SEH_TRY
    {
      *KeyHandle = hKey;
    }
    _SEH_HANDLE
    {
      Status = _SEH_GetExceptionCode();
    }
    _SEH_END;
  }

  return Status;
}


NTSTATUS STDCALL
NtQueryKey(IN HANDLE KeyHandle,
	   IN KEY_INFORMATION_CLASS KeyInformationClass,
	   OUT PVOID KeyInformation,
	   IN ULONG Length,
	   OUT PULONG ResultLength)
{
  PKEY_BASIC_INFORMATION BasicInformation;
  PKEY_NODE_INFORMATION NodeInformation;
  PKEY_FULL_INFORMATION FullInformation;
  PREGISTRY_HIVE RegistryHive;
  PDATA_CELL ClassCell;
  PKEY_OBJECT KeyObject;
  PKEY_CELL KeyCell;
  ULONG NameSize, ClassSize;
  NTSTATUS Status;
  REG_QUERY_KEY_INFORMATION QueryKeyInfo;
  REG_POST_OPERATION_INFORMATION PostOperationInfo;

  PAGED_CODE();

  DPRINT("NtQueryKey(KH 0x%p  KIC %x  KI 0x%p  L %d  RL 0x%p)\n",
	 KeyHandle,
	 KeyInformationClass,
	 KeyInformation,
	 Length,
	 ResultLength);

  /* Verify that the handle is valid and is a registry key */
  Status = ObReferenceObjectByHandle(KeyHandle,
		(KeyInformationClass != KeyNameInformation ? KEY_QUERY_VALUE : 0),
		CmiKeyType,
		ExGetPreviousMode(),
		(PVOID *) &KeyObject,
		NULL);
  if (!NT_SUCCESS(Status))
    {
      return Status;
    }

  PostOperationInfo.Object = (PVOID)KeyObject;
  QueryKeyInfo.Object = (PVOID)KeyObject;
  QueryKeyInfo.KeyInformationClass = KeyInformationClass;
  QueryKeyInfo.KeyInformation = KeyInformation;
  QueryKeyInfo.Length = Length;
  QueryKeyInfo.ResultLength = ResultLength;
  
  Status = CmiCallRegisteredCallbacks(RegNtQueryKey, &QueryKeyInfo);
  if (!NT_SUCCESS(Status))
    {
      ObDereferenceObject(KeyObject);
      return Status;
    }

  /* Acquire hive lock */
  KeEnterCriticalRegion();
  ExAcquireResourceSharedLite(&CmiRegistryLock, TRUE);

  VERIFY_KEY_OBJECT(KeyObject);

  /* Get pointer to KeyCell */
  KeyCell = KeyObject->KeyCell;
  RegistryHive = KeyObject->RegistryHive;

  Status = STATUS_SUCCESS;
  switch (KeyInformationClass)
    {
      case KeyBasicInformation:
        NameSize = KeyObject->Name.Length;

	*ResultLength = FIELD_OFFSET(KEY_BASIC_INFORMATION, Name[0]);

	/* Check size of buffer */
	if (Length < FIELD_OFFSET(KEY_BASIC_INFORMATION, Name[0]))
	  {
	    Status = STATUS_BUFFER_TOO_SMALL;
	  }
	else
	  {
	    /* Fill buffer with requested info */
	    BasicInformation = (PKEY_BASIC_INFORMATION) KeyInformation;
	    BasicInformation->LastWriteTime.u.LowPart = KeyCell->LastWriteTime.u.LowPart;
	    BasicInformation->LastWriteTime.u.HighPart = KeyCell->LastWriteTime.u.HighPart;
	    BasicInformation->TitleIndex = 0;
	    BasicInformation->NameLength = KeyObject->Name.Length;

	    if (Length - FIELD_OFFSET(KEY_BASIC_INFORMATION, Name[0]) <
	        NameSize)
	      {
	        NameSize = Length - FIELD_OFFSET(KEY_BASIC_INFORMATION, Name[0]);
	        Status = STATUS_BUFFER_OVERFLOW;
	        CHECKPOINT;
	      }

	    RtlCopyMemory(BasicInformation->Name,
			  KeyObject->Name.Buffer,
			  NameSize);
	  }
	break;

      case KeyNodeInformation:
        NameSize = KeyObject->Name.Length;
        ClassSize = KeyCell->ClassSize;

	*ResultLength = FIELD_OFFSET(KEY_NODE_INFORMATION, Name[0]) +
	  NameSize + ClassSize;

	/* Check size of buffer */
	if (Length < *ResultLength)
	  {
	    Status = STATUS_BUFFER_TOO_SMALL;
	  }
	else
	  {
	    /* Fill buffer with requested info */
	    NodeInformation = (PKEY_NODE_INFORMATION) KeyInformation;
	    NodeInformation->LastWriteTime.u.LowPart = KeyCell->LastWriteTime.u.LowPart;
	    NodeInformation->LastWriteTime.u.HighPart = KeyCell->LastWriteTime.u.HighPart;
	    NodeInformation->TitleIndex = 0;
	    NodeInformation->ClassOffset = sizeof(KEY_NODE_INFORMATION) +
	      KeyObject->Name.Length;
	    NodeInformation->ClassLength = KeyCell->ClassSize;
	    NodeInformation->NameLength = KeyObject->Name.Length;

	    if (Length - FIELD_OFFSET(KEY_NODE_INFORMATION, Name[0]) < NameSize)
	      {
	        NameSize = Length - FIELD_OFFSET(KEY_NODE_INFORMATION, Name[0]);
	        ClassSize = 0;
	        Status = STATUS_BUFFER_OVERFLOW;
	        CHECKPOINT;
	      }
	    else if (Length - FIELD_OFFSET(KEY_NODE_INFORMATION, Name[0]) -
	             NameSize < ClassSize)
	      {
	        ClassSize = Length - FIELD_OFFSET(KEY_NODE_INFORMATION, Name[0]) -
	                    NameSize;
	        Status = STATUS_BUFFER_OVERFLOW;
	        CHECKPOINT;
	      }

	    RtlCopyMemory(NodeInformation->Name,
			  KeyObject->Name.Buffer,
			  NameSize);

	    if (ClassSize != 0)
	      {
		ClassCell = CmiGetCell (KeyObject->RegistryHive,
					KeyCell->ClassNameOffset,
					NULL);
		RtlCopyMemory (NodeInformation->Name + KeyObject->Name.Length,
			       ClassCell->Data,
			       ClassSize);
	      }
	  }
	break;

      case KeyFullInformation:
        ClassSize = KeyCell->ClassSize;

	*ResultLength = FIELD_OFFSET(KEY_FULL_INFORMATION, Class) +
	  ClassSize;

	/* Check size of buffer */
	if (Length < FIELD_OFFSET(KEY_FULL_INFORMATION, Class))
	  {
	    Status = STATUS_BUFFER_TOO_SMALL;
	  }
	else
	  {
	    /* Fill buffer with requested info */
	    FullInformation = (PKEY_FULL_INFORMATION) KeyInformation;
	    FullInformation->LastWriteTime.u.LowPart = KeyCell->LastWriteTime.u.LowPart;
	    FullInformation->LastWriteTime.u.HighPart = KeyCell->LastWriteTime.u.HighPart;
	    FullInformation->TitleIndex = 0;
	    FullInformation->ClassOffset = sizeof(KEY_FULL_INFORMATION) - sizeof(WCHAR);
	    FullInformation->ClassLength = KeyCell->ClassSize;
	    FullInformation->SubKeys = CmiGetNumberOfSubKeys(KeyObject); //KeyCell->NumberOfSubKeys;
	    FullInformation->MaxNameLen = CmiGetMaxNameLength(KeyObject);
	    FullInformation->MaxClassLen = CmiGetMaxClassLength(KeyObject);
	    FullInformation->Values = KeyCell->NumberOfValues;
	    FullInformation->MaxValueNameLen =
	      CmiGetMaxValueNameLength(RegistryHive, KeyCell);
	    FullInformation->MaxValueDataLen =
	      CmiGetMaxValueDataLength(RegistryHive, KeyCell);

	    if (Length - FIELD_OFFSET(KEY_FULL_INFORMATION, Class[0]) < ClassSize)
	      {
	        ClassSize = Length - FIELD_OFFSET(KEY_FULL_INFORMATION, Class[0]);
	        Status = STATUS_BUFFER_OVERFLOW;
	        CHECKPOINT;
	      }

	    if (ClassSize)
	      {
		ClassCell = CmiGetCell (KeyObject->RegistryHive,
					KeyCell->ClassNameOffset,
					NULL);
		RtlCopyMemory (FullInformation->Class,
			       ClassCell->Data, ClassSize);
	      }
	  }
	break;

      case KeyNameInformation:
      case KeyCachedInformation:
      case KeyFlagsInformation:
        DPRINT1("Key information class 0x%x not yet implemented!\n", KeyInformationClass);
        Status = STATUS_NOT_IMPLEMENTED;
        break;

      default:
	DPRINT1("Not handling 0x%x\n", KeyInformationClass);
	Status = STATUS_INVALID_INFO_CLASS;
	break;
    }

  ExReleaseResourceLite(&CmiRegistryLock);
  KeLeaveCriticalRegion();

  PostOperationInfo.Status = Status;
  CmiCallRegisteredCallbacks(RegNtPostQueryKey, &PostOperationInfo);

  ObDereferenceObject(KeyObject);

  return(Status);
}


NTSTATUS STDCALL
NtQueryValueKey(IN HANDLE KeyHandle,
	IN PUNICODE_STRING ValueName,
	IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
	OUT PVOID KeyValueInformation,
	IN ULONG Length,
	OUT PULONG ResultLength)
{
  NTSTATUS  Status;
  ULONG NameSize, DataSize;
  PKEY_OBJECT  KeyObject;
  PREGISTRY_HIVE  RegistryHive;
  PKEY_CELL  KeyCell;
  PVALUE_CELL  ValueCell;
  PDATA_CELL  DataCell;
  PKEY_VALUE_BASIC_INFORMATION  ValueBasicInformation;
  PKEY_VALUE_PARTIAL_INFORMATION  ValuePartialInformation;
  PKEY_VALUE_FULL_INFORMATION  ValueFullInformation;
  REG_QUERY_VALUE_KEY_INFORMATION QueryValueKeyInfo;
  REG_POST_OPERATION_INFORMATION PostOperationInfo;

  PAGED_CODE();

  DPRINT("NtQueryValueKey(KeyHandle 0x%p  ValueName %S  Length %x)\n",
    KeyHandle, ValueName->Buffer, Length);

  /* Verify that the handle is valid and is a registry key */
  Status = ObReferenceObjectByHandle(KeyHandle,
		KEY_QUERY_VALUE,
		CmiKeyType,
		ExGetPreviousMode(),
		(PVOID *)&KeyObject,
		NULL);

  if (!NT_SUCCESS(Status))
    {
      DPRINT1("ObReferenceObjectByHandle() failed with status %x %p\n", Status, KeyHandle);
      return Status;
    }
  
  PostOperationInfo.Object = (PVOID)KeyObject;
  QueryValueKeyInfo.Object = (PVOID)KeyObject;
  QueryValueKeyInfo.ValueName = ValueName;
  QueryValueKeyInfo.KeyValueInformationClass = KeyValueInformationClass;
  QueryValueKeyInfo.Length = Length;
  QueryValueKeyInfo.ResultLength = ResultLength;

  Status = CmiCallRegisteredCallbacks(RegNtPreQueryValueKey, &QueryValueKeyInfo);
  if (!NT_SUCCESS(Status))
    {
      ObDereferenceObject(KeyObject);
      return Status;
    }

  /* Acquire hive lock */
  KeEnterCriticalRegion();
  ExAcquireResourceSharedLite(&CmiRegistryLock, TRUE);

  VERIFY_KEY_OBJECT(KeyObject);

  /* Get pointer to KeyCell */
  KeyCell = KeyObject->KeyCell;
  RegistryHive = KeyObject->RegistryHive;

  /* Get value cell by name */
  Status = CmiScanKeyForValue(RegistryHive,
			      KeyCell,
			      ValueName,
			      &ValueCell,
			      NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("CmiScanKeyForValue() failed with status %x\n", Status);
      goto ByeBye;
    }

  Status = STATUS_SUCCESS;
  switch (KeyValueInformationClass)
    {
      case KeyValueBasicInformation:
	NameSize = ValueCell->NameSize;
	if (ValueCell->Flags & REG_VALUE_NAME_PACKED)
	  {
	    NameSize *= sizeof(WCHAR);
	  }

	*ResultLength = FIELD_OFFSET(KEY_VALUE_BASIC_INFORMATION, Name[0]) +
	                NameSize;

	if (Length < FIELD_OFFSET(KEY_VALUE_BASIC_INFORMATION, Name[0]))
	  {
	    Status = STATUS_BUFFER_TOO_SMALL;
	  }
	else
	  {
	    ValueBasicInformation = (PKEY_VALUE_BASIC_INFORMATION)
	      KeyValueInformation;
	    ValueBasicInformation->TitleIndex = 0;
	    ValueBasicInformation->Type = ValueCell->DataType;
	    ValueBasicInformation->NameLength = NameSize;

	    if (Length - FIELD_OFFSET(KEY_VALUE_BASIC_INFORMATION, Name[0]) <
	        NameSize)
	      {
	        NameSize = Length - FIELD_OFFSET(KEY_VALUE_BASIC_INFORMATION, Name[0]);
	        Status = STATUS_BUFFER_OVERFLOW;
	        CHECKPOINT;
	      }

	    if (ValueCell->Flags & REG_VALUE_NAME_PACKED)
	      {
		CmiCopyPackedName(ValueBasicInformation->Name,
				  ValueCell->Name,
				  NameSize / sizeof(WCHAR));
	      }
	    else
	      {
		RtlCopyMemory(ValueBasicInformation->Name,
			      ValueCell->Name,
			      NameSize);
	      }
	  }
	break;

      case KeyValuePartialInformation:
	DataSize = ValueCell->DataSize & REG_DATA_SIZE_MASK;

	*ResultLength = FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data[0]) +
	                DataSize;

	if (Length < FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data[0]))
	  {
	    Status = STATUS_BUFFER_TOO_SMALL;
	  }
	else
	  {
	    ValuePartialInformation = (PKEY_VALUE_PARTIAL_INFORMATION)
	      KeyValueInformation;
	    ValuePartialInformation->TitleIndex = 0;
	    ValuePartialInformation->Type = ValueCell->DataType;
	    ValuePartialInformation->DataLength = DataSize;

	    if (Length - FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data[0]) <
	        DataSize)
	      {
		DataSize = Length - FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data[0]);
		Status = STATUS_BUFFER_OVERFLOW;
		CHECKPOINT;
	      }

	    if (!(ValueCell->DataSize & REG_DATA_IN_OFFSET))
	      {
		DataCell = CmiGetCell (RegistryHive, ValueCell->DataOffset, NULL);
		RtlCopyMemory(ValuePartialInformation->Data,
			      DataCell->Data,
			      DataSize);
	      }
	    else
	      {
		RtlCopyMemory(ValuePartialInformation->Data,
			      &ValueCell->DataOffset,
			      DataSize);
	      }
	  }
	break;

      case KeyValueFullInformation:
	NameSize = ValueCell->NameSize;
	if (ValueCell->Flags & REG_VALUE_NAME_PACKED)
	  {
	    NameSize *= sizeof(WCHAR);
	  }
	DataSize = ValueCell->DataSize & REG_DATA_SIZE_MASK;

	*ResultLength = ROUND_UP(FIELD_OFFSET(KEY_VALUE_FULL_INFORMATION,
	                Name[0]) + NameSize, sizeof(PVOID)) + DataSize;

	if (Length < FIELD_OFFSET(KEY_VALUE_FULL_INFORMATION, Name[0]))
	  {
	    Status = STATUS_BUFFER_TOO_SMALL;
	  }
	else
	  {
	    ValueFullInformation = (PKEY_VALUE_FULL_INFORMATION)
	      KeyValueInformation;
	    ValueFullInformation->TitleIndex = 0;
	    ValueFullInformation->Type = ValueCell->DataType;
	    ValueFullInformation->NameLength = NameSize;
	    ValueFullInformation->DataOffset =
	      (ULONG_PTR)ValueFullInformation->Name -
	      (ULONG_PTR)ValueFullInformation +
	      ValueFullInformation->NameLength;
	    ValueFullInformation->DataOffset =
	      ROUND_UP(ValueFullInformation->DataOffset, sizeof(PVOID));
	    ValueFullInformation->DataLength = ValueCell->DataSize & REG_DATA_SIZE_MASK;

	    if (Length - FIELD_OFFSET(KEY_VALUE_FULL_INFORMATION, Name[0]) <
	        NameSize)
	      {
	        NameSize = Length - FIELD_OFFSET(KEY_VALUE_FULL_INFORMATION, Name[0]);
	        DataSize = 0;
	        Status = STATUS_BUFFER_OVERFLOW;
	        CHECKPOINT;
	      }
            else if (ROUND_UP(Length - FIELD_OFFSET(KEY_VALUE_FULL_INFORMATION,
                     Name[0]) - NameSize, sizeof(PVOID)) < DataSize)
	      {
	        DataSize = ROUND_UP(Length - FIELD_OFFSET(KEY_VALUE_FULL_INFORMATION,
	                            Name[0]) - NameSize, sizeof(PVOID));
	        Status = STATUS_BUFFER_OVERFLOW;
	        CHECKPOINT;
	      }

	    if (ValueCell->Flags & REG_VALUE_NAME_PACKED)
	      {
		CmiCopyPackedName(ValueFullInformation->Name,
				  ValueCell->Name,
				  NameSize / sizeof(WCHAR));
	      }
	    else
	      {
		RtlCopyMemory(ValueFullInformation->Name,
			      ValueCell->Name,
			      NameSize);
	      }
	    if (!(ValueCell->DataSize & REG_DATA_IN_OFFSET))
	      {
		DataCell = CmiGetCell (RegistryHive, ValueCell->DataOffset, NULL);
		RtlCopyMemory((PCHAR) ValueFullInformation
			      + ValueFullInformation->DataOffset,
			      DataCell->Data,
			      DataSize);
	      }
	    else
	      {
		RtlCopyMemory((PCHAR) ValueFullInformation
			      + ValueFullInformation->DataOffset,
			      &ValueCell->DataOffset,
			      DataSize);
	      }
	  }
	break;

      default:
	DPRINT1("Not handling 0x%x\n", KeyValueInformationClass);
	Status = STATUS_INVALID_INFO_CLASS;
	break;
    }

ByeBye:;
  ExReleaseResourceLite(&CmiRegistryLock);
  KeLeaveCriticalRegion();

  PostOperationInfo.Status = Status;
  CmiCallRegisteredCallbacks(RegNtPostQueryValueKey, &PostOperationInfo);
  ObDereferenceObject(KeyObject);

  return Status;
}


NTSTATUS STDCALL
NtSetValueKey(IN HANDLE KeyHandle,
	      IN PUNICODE_STRING ValueName,
	      IN ULONG TitleIndex,
	      IN ULONG Type,
	      IN PVOID Data,
	      IN ULONG DataSize)
{
  NTSTATUS  Status;
  PKEY_OBJECT  KeyObject;
  PREGISTRY_HIVE  RegistryHive;
  PKEY_CELL  KeyCell;
  PVALUE_CELL  ValueCell;
  BLOCK_OFFSET ValueCellOffset;
  PDATA_CELL DataCell;
  PDATA_CELL NewDataCell;
  ULONG DesiredAccess;
  REG_SET_VALUE_KEY_INFORMATION SetValueKeyInfo;
  REG_POST_OPERATION_INFORMATION PostOperationInfo;
  ULONG DataCellSize;

  PAGED_CODE();

  DPRINT("NtSetValueKey(KeyHandle 0x%p  ValueName '%wZ'  Type %d)\n",
	 KeyHandle, ValueName, Type);

  DesiredAccess = KEY_SET_VALUE;

  /* Verify that the handle is valid and is a registry key */
  Status = ObReferenceObjectByHandle(KeyHandle,
				     DesiredAccess,
				     CmiKeyType,
				     ExGetPreviousMode(),
				     (PVOID *)&KeyObject,
				     NULL);
  if (!NT_SUCCESS(Status))
    return(Status);

  PostOperationInfo.Object = (PVOID)KeyObject;
  SetValueKeyInfo.Object = (PVOID)KeyObject;
  SetValueKeyInfo.ValueName = ValueName;
  SetValueKeyInfo.TitleIndex = TitleIndex;
  SetValueKeyInfo.Type = Type;
  SetValueKeyInfo.Data = Data;
  SetValueKeyInfo.DataSize = DataSize;
  Status = CmiCallRegisteredCallbacks(RegNtPreSetValueKey, &SetValueKeyInfo);
  if (!NT_SUCCESS(Status))
    {
      ObDereferenceObject(KeyObject);
      return Status;
    }

  /* Acquire hive lock exclucively */
  KeEnterCriticalRegion();
  ExAcquireResourceExclusiveLite(&CmiRegistryLock, TRUE);

  VERIFY_KEY_OBJECT(KeyObject);

  /* Get pointer to key cell */
  KeyCell = KeyObject->KeyCell;
  RegistryHive = KeyObject->RegistryHive;
  Status = CmiScanKeyForValue(RegistryHive,
			      KeyCell,
			      ValueName,
			      &ValueCell,
			      &ValueCellOffset);
  if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
      DPRINT("Allocate new value cell\n");
      Status = CmiAddValueToKey(RegistryHive,
				KeyCell,
				KeyObject->KeyCellOffset,
				ValueName,
				&ValueCell,
				&ValueCellOffset);
    }

  if (!NT_SUCCESS(Status))
    {
      DPRINT("Cannot add value. Status 0x%X\n", Status);

      ExReleaseResourceLite(&CmiRegistryLock);
      KeLeaveCriticalRegion();
      PostOperationInfo.Status = Status;
      CmiCallRegisteredCallbacks(RegNtPostSetValueKey, &PostOperationInfo);
      ObDereferenceObject(KeyObject);
      return Status;
    }

  DPRINT("DataSize %lu\n", DataSize);
  DPRINT("ValueCell %p\n", ValueCell);
  DPRINT("ValueCell->DataSize %lu\n", ValueCell->DataSize);

  if (!(ValueCell->DataSize & REG_DATA_IN_OFFSET) &&
      (ValueCell->DataSize & REG_DATA_SIZE_MASK) != 0)
    {
      DataCell = CmiGetCell (RegistryHive, ValueCell->DataOffset, NULL);
      DataCellSize = (DataCell->CellSize < 0 ? -DataCell->CellSize : DataCell->CellSize) - sizeof(CELL_HEADER);
    }
  else
    {
      DataCell = NULL;
      DataCellSize = 0;
    }


  if (DataSize <= sizeof(BLOCK_OFFSET))
    {
      /* If data size <= sizeof(BLOCK_OFFSET) then store data in the data offset */
      DPRINT("ValueCell->DataSize %lu\n", ValueCell->DataSize);
      if (DataCell)
	{
	  CmiDestroyCell(RegistryHive, DataCell, ValueCell->DataOffset);
	}

      RtlCopyMemory(&ValueCell->DataOffset, Data, DataSize);
      ValueCell->DataSize = DataSize | REG_DATA_IN_OFFSET;
      ValueCell->DataType = Type;
      CmiMarkBlockDirty(RegistryHive, ValueCellOffset);
    }
  else 
    {
      if (DataSize > DataCellSize)
        {
         /*
          * New data size is larger than the current, destroy current
          * data block and allocate a new one.
          */
          BLOCK_OFFSET NewOffset;

          DPRINT("ValueCell->DataSize %lu\n", ValueCell->DataSize);

          Status = CmiAllocateCell (RegistryHive,
				    sizeof(CELL_HEADER) + DataSize,
				    (PVOID *)&NewDataCell,
				    &NewOffset);
          if (!NT_SUCCESS(Status))
	    {
	      DPRINT("CmiAllocateBlock() failed (Status %lx)\n", Status);

	      ExReleaseResourceLite(&CmiRegistryLock);
	      KeLeaveCriticalRegion();
              PostOperationInfo.Status = Status;
              CmiCallRegisteredCallbacks(RegNtPostSetValueKey, &PostOperationInfo);
	      ObDereferenceObject(KeyObject);

	      return Status;
	    }

          if (DataCell)
	    {
	      CmiDestroyCell(RegistryHive, DataCell, ValueCell->DataOffset);
	    }

          ValueCell->DataOffset = NewOffset;
          DataCell = NewDataCell;
        }

      RtlCopyMemory(DataCell->Data, Data, DataSize);
      ValueCell->DataSize = DataSize & REG_DATA_SIZE_MASK;
      ValueCell->DataType = Type;
      CmiMarkBlockDirty(RegistryHive, ValueCell->DataOffset);
      CmiMarkBlockDirty(RegistryHive, ValueCellOffset);
    }

  /* Mark link key */
  if ((Type == REG_LINK) &&
      (_wcsicmp(ValueName->Buffer, L"SymbolicLinkValue") == 0))
    {
      KeyCell->Flags |= REG_KEY_LINK_CELL;
    }

  KeQuerySystemTime (&KeyCell->LastWriteTime);
  CmiMarkBlockDirty (RegistryHive, KeyObject->KeyCellOffset);

  ExReleaseResourceLite(&CmiRegistryLock);
  KeLeaveCriticalRegion();
  PostOperationInfo.Status = Status;
  CmiCallRegisteredCallbacks(RegNtPostSetValueKey, &PostOperationInfo);
  ObDereferenceObject(KeyObject);

  CmiSyncHives();

  DPRINT("Return Status 0x%X\n", Status);

  return Status;
}


NTSTATUS STDCALL
NtDeleteValueKey (IN HANDLE KeyHandle,
		  IN PUNICODE_STRING ValueName)
{
  PKEY_OBJECT KeyObject;
  NTSTATUS Status;
  REG_DELETE_VALUE_KEY_INFORMATION DeleteValueKeyInfo;
  REG_POST_OPERATION_INFORMATION PostOperationInfo;
  KPROCESSOR_MODE PreviousMode;
  UNICODE_STRING CapturedValueName;

  PAGED_CODE();
  
  PreviousMode = ExGetPreviousMode();

  /* Verify that the handle is valid and is a registry key */
  Status = ObReferenceObjectByHandle(KeyHandle,
		KEY_SET_VALUE,
		CmiKeyType,
		PreviousMode,
		(PVOID *)&KeyObject,
		NULL);
  if (!NT_SUCCESS(Status))
    {
      return Status;
    }

  Status = ProbeAndCaptureUnicodeString(&CapturedValueName,
                                        PreviousMode,
                                        ValueName);
  if (!NT_SUCCESS(Status))
    {
      goto Fail;
    }
  DeleteValueKeyInfo.Object = (PVOID)KeyObject;
  DeleteValueKeyInfo.ValueName = &CapturedValueName;

  /* FIXME - check if value exists before calling the callbacks? */
  Status = CmiCallRegisteredCallbacks(RegNtPreDeleteValueKey, &DeleteValueKeyInfo);
  if (!NT_SUCCESS(Status))
    {
      ReleaseCapturedUnicodeString(&CapturedValueName,
                                   PreviousMode);
Fail:
      ObDereferenceObject(KeyObject);
      return Status;
    }

  /* Acquire hive lock */
  KeEnterCriticalRegion();
  ExAcquireResourceExclusiveLite(&CmiRegistryLock, TRUE);

  VERIFY_KEY_OBJECT(KeyObject);

  Status = CmiDeleteValueFromKey(KeyObject->RegistryHive,
				 KeyObject->KeyCell,
				 KeyObject->KeyCellOffset,
				 ValueName);

  KeQuerySystemTime (&KeyObject->KeyCell->LastWriteTime);
  CmiMarkBlockDirty (KeyObject->RegistryHive, KeyObject->KeyCellOffset);

  /* Release hive lock */
  ExReleaseResourceLite(&CmiRegistryLock);
  KeLeaveCriticalRegion();

  ReleaseCapturedUnicodeString(&CapturedValueName,
                               PreviousMode);

  PostOperationInfo.Object = (PVOID)KeyObject;
  PostOperationInfo.Status = Status;

  CmiCallRegisteredCallbacks(RegNtPostDeleteValueKey, &PostOperationInfo);

  ObDereferenceObject (KeyObject);

  CmiSyncHives ();

  return Status;
}


/*
 * NOTE:
 * KeyObjectAttributes->RootDirectory specifies the handle to the parent key and
 * KeyObjectAttributes->Name specifies the name of the key to load.
 */
NTSTATUS STDCALL
NtLoadKey (IN POBJECT_ATTRIBUTES KeyObjectAttributes,
	   IN POBJECT_ATTRIBUTES FileObjectAttributes)
{
  return NtLoadKey2 (KeyObjectAttributes,
		     FileObjectAttributes,
		     0);
}


/*
 * NOTE:
 * KeyObjectAttributes->RootDirectory specifies the handle to the parent key and
 * KeyObjectAttributes->Name specifies the name of the key to load.
 * Flags can be 0 or REG_NO_LAZY_FLUSH.
 */
NTSTATUS STDCALL
NtLoadKey2 (IN POBJECT_ATTRIBUTES KeyObjectAttributes,
	    IN POBJECT_ATTRIBUTES FileObjectAttributes,
	    IN ULONG Flags)
{
  POBJECT_NAME_INFORMATION NameInfo;
  PUNICODE_STRING NamePointer;
  PUCHAR Buffer;
  ULONG BufferSize;
  ULONG Length;
  NTSTATUS Status;

  PAGED_CODE();

  DPRINT ("NtLoadKey2() called\n");

#if 0
  if (!SeSinglePrivilegeCheck (SeRestorePrivilege, ExGetPreviousMode ()))
    return STATUS_PRIVILEGE_NOT_HELD;
#endif

  if (FileObjectAttributes->RootDirectory != NULL)
    {
      BufferSize =
	sizeof(OBJECT_NAME_INFORMATION) + MAX_PATH * sizeof(WCHAR);
      Buffer = ExAllocatePool (NonPagedPool,
			       BufferSize);
      if (Buffer == NULL)
	return STATUS_INSUFFICIENT_RESOURCES;

      Status = ZwQueryObject (FileObjectAttributes->RootDirectory,
			      ObjectNameInformation,
			      Buffer,
			      BufferSize,
			      &Length);
      if (!NT_SUCCESS(Status))
	{
	  DPRINT1 ("NtQueryObject() failed (Status %lx)\n", Status);
	  ExFreePool (Buffer);
	  return Status;
	}

      NameInfo = (POBJECT_NAME_INFORMATION)Buffer;
      DPRINT ("ObjectPath: '%wZ'  Length %hu\n",
	      &NameInfo->Name, NameInfo->Name.Length);

      NameInfo->Name.MaximumLength = MAX_PATH * sizeof(WCHAR);
      if (FileObjectAttributes->ObjectName->Buffer[0] != L'\\')
	{
	  RtlAppendUnicodeToString (&NameInfo->Name,
				    L"\\");
	  DPRINT ("ObjectPath: '%wZ'  Length %hu\n",
		  &NameInfo->Name, NameInfo->Name.Length);
	}
      RtlAppendUnicodeStringToString (&NameInfo->Name,
				      FileObjectAttributes->ObjectName);

      DPRINT ("ObjectPath: '%wZ'  Length %hu\n",
	      &NameInfo->Name, NameInfo->Name.Length);
      NamePointer = &NameInfo->Name;
    }
  else
    {
      if (FileObjectAttributes->ObjectName->Buffer[0] == L'\\')
	{
	  Buffer = NULL;
	  NamePointer = FileObjectAttributes->ObjectName;
	}
      else
	{
	  BufferSize =
	    sizeof(OBJECT_NAME_INFORMATION) + MAX_PATH * sizeof(WCHAR);
	  Buffer = ExAllocatePool (NonPagedPool,
				   BufferSize);
	  if (Buffer == NULL)
	    return STATUS_INSUFFICIENT_RESOURCES;

	  NameInfo = (POBJECT_NAME_INFORMATION)Buffer;
	  NameInfo->Name.MaximumLength = MAX_PATH * sizeof(WCHAR);
	  NameInfo->Name.Length = 0;
	  NameInfo->Name.Buffer = (PWSTR)((ULONG_PTR)Buffer + sizeof(OBJECT_NAME_INFORMATION));
	  NameInfo->Name.Buffer[0] = 0;

	  RtlAppendUnicodeToString (&NameInfo->Name,
				    L"\\");
	  RtlAppendUnicodeStringToString (&NameInfo->Name,
					  FileObjectAttributes->ObjectName);

	  NamePointer = &NameInfo->Name;
	}
    }

  DPRINT ("Full name: '%wZ'\n", NamePointer);

  /* Acquire hive lock */
  KeEnterCriticalRegion();
  ExAcquireResourceExclusiveLite(&CmiRegistryLock, TRUE);

  Status = CmiLoadHive (KeyObjectAttributes,
			NamePointer,
			Flags);
  if (!NT_SUCCESS (Status))
    {
      DPRINT1 ("CmiLoadHive() failed (Status %lx)\n", Status);
    }

  /* Release hive lock */
  ExReleaseResourceLite(&CmiRegistryLock);
  KeLeaveCriticalRegion();

  if (Buffer != NULL)
    ExFreePool (Buffer);

  return Status;
}


NTSTATUS STDCALL
NtNotifyChangeKey (IN HANDLE KeyHandle,
		   IN HANDLE Event,
		   IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
		   IN PVOID ApcContext OPTIONAL,
		   OUT PIO_STATUS_BLOCK IoStatusBlock,
		   IN ULONG CompletionFilter,
		   IN BOOLEAN WatchSubtree,
		   OUT PVOID Buffer,
		   IN ULONG Length,
		   IN BOOLEAN Asynchronous)
{
	UNIMPLEMENTED;
	return(STATUS_NOT_IMPLEMENTED);
}

#if 0
NTSTATUS STDCALL
NtNotifyChangeKey (IN HANDLE KeyHandle,
		   IN HANDLE Event,
		   IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
		   IN PVOID ApcContext OPTIONAL,
		   OUT PIO_STATUS_BLOCK IoStatusBlock,
		   IN ULONG CompletionFilter,
		   IN BOOLEAN WatchSubtree,
		   OUT PVOID Buffer,
		   IN ULONG Length,
		   IN BOOLEAN Asynchronous)
{
     return NtNotifyChangeMultipleKeys(KeyHandle,
                                       0,
                                       NULL,
                                       Event,
                                       ApcRoutine,
                                       ApcContext,
                                       IoStatusBlock,
                                       CompletionFilter,
                                       WatchTree,
                                       Buffer,
                                       Length,
                                       Asynchronous);
}

#endif

NTSTATUS STDCALL
NtQueryMultipleValueKey (IN HANDLE KeyHandle,
			 IN OUT PKEY_VALUE_ENTRY ValueList,
			 IN ULONG NumberOfValues,
			 OUT PVOID Buffer,
			 IN OUT PULONG Length,
			 OUT PULONG ReturnLength)
{
  PREGISTRY_HIVE RegistryHive;
  PVALUE_CELL ValueCell;
  PKEY_OBJECT KeyObject;
  PDATA_CELL DataCell;
  ULONG BufferLength = 0;
  PKEY_CELL KeyCell;
  NTSTATUS Status;
  PUCHAR DataPtr;
  ULONG i;
  REG_QUERY_MULTIPLE_VALUE_KEY_INFORMATION QueryMultipleValueKeyInfo;
  REG_POST_OPERATION_INFORMATION PostOperationInfo;

  PAGED_CODE();

  /* Verify that the handle is valid and is a registry key */
  Status = ObReferenceObjectByHandle(KeyHandle,
				     KEY_QUERY_VALUE,
				     CmiKeyType,
				     ExGetPreviousMode(),
				     (PVOID *) &KeyObject,
				     NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("ObReferenceObjectByHandle() failed with status %x\n", Status);
      return(Status);
    }

  PostOperationInfo.Object = (PVOID)KeyObject;
  QueryMultipleValueKeyInfo.Object = (PVOID)KeyObject;
  QueryMultipleValueKeyInfo.ValueEntries = ValueList;
  QueryMultipleValueKeyInfo.EntryCount = NumberOfValues;
  QueryMultipleValueKeyInfo.ValueBuffer = Buffer;
  QueryMultipleValueKeyInfo.BufferLength = Length;
  QueryMultipleValueKeyInfo.RequiredBufferLength = ReturnLength;

  Status = CmiCallRegisteredCallbacks(RegNtPreQueryMultipleValueKey, &QueryMultipleValueKeyInfo);
  if (!NT_SUCCESS(Status))
    {
      ObDereferenceObject(KeyObject);
      return Status;
    }

  /* Acquire hive lock */
  KeEnterCriticalRegion();
  ExAcquireResourceSharedLite(&CmiRegistryLock, TRUE);

  VERIFY_KEY_OBJECT(KeyObject);

  /* Get pointer to KeyCell */
  KeyCell = KeyObject->KeyCell;
  RegistryHive = KeyObject->RegistryHive;

  DataPtr = (PUCHAR) Buffer;

  for (i = 0; i < NumberOfValues; i++)
    {
      DPRINT("ValueName: '%wZ'\n", ValueList[i].ValueName);

      /* Get Value block of interest */
      Status = CmiScanKeyForValue(RegistryHive,
			  KeyCell,
			  ValueList[i].ValueName,
			  &ValueCell,
			  NULL);

      if (!NT_SUCCESS(Status))
	{
	  DPRINT("CmiScanKeyForValue() failed with status %x\n", Status);
	  break;
	}
      else if (ValueCell == NULL)
	{
	  Status = STATUS_OBJECT_NAME_NOT_FOUND;
	  break;
	}

      BufferLength = ROUND_UP(BufferLength, sizeof(PVOID));

      if (BufferLength + (ValueCell->DataSize & REG_DATA_SIZE_MASK) <= *Length)
	{
	  DataPtr = (PUCHAR)ROUND_UP((ULONG_PTR)DataPtr, sizeof(PVOID));

	  ValueList[i].Type = ValueCell->DataType;
	  ValueList[i].DataLength = ValueCell->DataSize & REG_DATA_SIZE_MASK;
	  ValueList[i].DataOffset = (ULONG_PTR)DataPtr - (ULONG_PTR)Buffer;

	  if (!(ValueCell->DataSize & REG_DATA_IN_OFFSET))
	    {
	      DataCell = CmiGetCell (RegistryHive,
				     ValueCell->DataOffset,
				     NULL);
	      RtlCopyMemory(DataPtr,
			    DataCell->Data,
			    ValueCell->DataSize & REG_DATA_SIZE_MASK);
	    }
	  else
	    {
	      RtlCopyMemory(DataPtr,
			    &ValueCell->DataOffset,
			    ValueCell->DataSize & REG_DATA_SIZE_MASK);
	    }

	  DataPtr += ValueCell->DataSize & REG_DATA_SIZE_MASK;
	}
      else
	{
	  Status = STATUS_BUFFER_TOO_SMALL;
	}

      BufferLength +=  ValueCell->DataSize & REG_DATA_SIZE_MASK;
    }

  if (NT_SUCCESS(Status))
    *Length = BufferLength;

  *ReturnLength = BufferLength;

  /* Release hive lock */
  ExReleaseResourceLite(&CmiRegistryLock);
  KeLeaveCriticalRegion();

  PostOperationInfo.Status = Status;
  CmiCallRegisteredCallbacks(RegNtPostQueryMultipleValueKey, &PostOperationInfo);

  ObDereferenceObject(KeyObject);

  DPRINT("Return Status 0x%X\n", Status);

  return Status;
}


NTSTATUS STDCALL
NtReplaceKey (IN POBJECT_ATTRIBUTES ObjectAttributes,
	      IN HANDLE Key,
	      IN POBJECT_ATTRIBUTES ReplacedObjectAttributes)
{
	UNIMPLEMENTED;
	return(STATUS_NOT_IMPLEMENTED);
}


NTSTATUS STDCALL
NtRestoreKey (IN HANDLE KeyHandle,
	      IN HANDLE FileHandle,
	      IN ULONG RestoreFlags)
{
	UNIMPLEMENTED;
	return(STATUS_NOT_IMPLEMENTED);
}


NTSTATUS STDCALL
NtSaveKey (IN HANDLE KeyHandle,
	   IN HANDLE FileHandle)
{
  PREGISTRY_HIVE TempHive;
  PKEY_OBJECT KeyObject;
  NTSTATUS Status;

  PAGED_CODE();

  DPRINT ("NtSaveKey() called\n");

#if 0
  if (!SeSinglePrivilegeCheck (SeBackupPrivilege, ExGetPreviousMode ()))
    return STATUS_PRIVILEGE_NOT_HELD;
#endif

  Status = ObReferenceObjectByHandle (KeyHandle,
				      0,
				      CmiKeyType,
				      ExGetPreviousMode(),
				      (PVOID *)&KeyObject,
				      NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1 ("ObReferenceObjectByHandle() failed (Status %lx)\n", Status);
      return Status;
    }

  /* Acquire hive lock exclucively */
  KeEnterCriticalRegion();
  ExAcquireResourceExclusiveLite(&CmiRegistryLock, TRUE);

  /* Refuse to save a volatile key */
  if (KeyObject->RegistryHive == CmiVolatileHive)
    {
      DPRINT1 ("Cannot save a volatile key\n");
      ExReleaseResourceLite(&CmiRegistryLock);
      KeLeaveCriticalRegion();
      ObDereferenceObject (KeyObject);
      return STATUS_ACCESS_DENIED;
    }

  Status = CmiCreateTempHive(&TempHive);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1 ("CmiCreateTempHive() failed (Status %lx)\n", Status);
      ExReleaseResourceLite(&CmiRegistryLock);
      KeLeaveCriticalRegion();
      ObDereferenceObject (KeyObject);
      return(Status);
    }

  Status = CmiCopyKey (TempHive,
		       NULL,
		       KeyObject->RegistryHive,
		       KeyObject->KeyCell);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1 ("CmiCopyKey() failed (Status %lx)\n", Status);
      CmiRemoveRegistryHive (TempHive);
      ExReleaseResourceLite(&CmiRegistryLock);
      KeLeaveCriticalRegion();
      ObDereferenceObject (KeyObject);
      return(Status);
    }

  Status = CmiSaveTempHive (TempHive,
			    FileHandle);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1 ("CmiSaveTempHive() failed (Status %lx)\n", Status);
    }

  CmiRemoveRegistryHive (TempHive);

  /* Release hive lock */
  ExReleaseResourceLite(&CmiRegistryLock);
  KeLeaveCriticalRegion();

  ObDereferenceObject (KeyObject);

  DPRINT ("NtSaveKey() done\n");

  return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtSaveKeyEx(
	IN HANDLE KeyHandle,
	IN HANDLE FileHandle,
	IN ULONG Flags // REG_STANDARD_FORMAT, etc..
	)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS STDCALL
NtSetInformationKey (IN HANDLE KeyHandle,
		     IN KEY_SET_INFORMATION_CLASS KeyInformationClass,
		     IN PVOID KeyInformation,
		     IN ULONG KeyInformationLength)
{
  PKEY_OBJECT KeyObject;
  NTSTATUS Status;
  REG_SET_INFORMATION_KEY_INFORMATION SetInformationKeyInfo;
  REG_POST_OPERATION_INFORMATION PostOperationInfo;

  PAGED_CODE();

  /* Verify that the handle is valid and is a registry key */
  Status = ObReferenceObjectByHandle (KeyHandle,
				      KEY_SET_VALUE,
				      CmiKeyType,
				      ExGetPreviousMode(),
				      (PVOID *)&KeyObject,
				      NULL);
  if (!NT_SUCCESS (Status))
    {
      DPRINT ("ObReferenceObjectByHandle() failed with status %x\n", Status);
      return Status;
    }

  PostOperationInfo.Object = (PVOID)KeyObject;
  SetInformationKeyInfo.Object = (PVOID)KeyObject;
  SetInformationKeyInfo.KeySetInformationClass = KeyInformationClass;
  SetInformationKeyInfo.KeySetInformation = KeyInformation;
  SetInformationKeyInfo.KeySetInformationLength = KeyInformationLength;

  Status = CmiCallRegisteredCallbacks(RegNtSetInformationKey, &SetInformationKeyInfo);
  if (!NT_SUCCESS(Status))
    {
      ObDereferenceObject (KeyObject);
      return Status;
    }

  if (KeyInformationClass != KeyWriteTimeInformation)
    {
      Status = STATUS_INVALID_INFO_CLASS;
    }

  else if (KeyInformationLength != sizeof (KEY_WRITE_TIME_INFORMATION))
    {
      Status = STATUS_INFO_LENGTH_MISMATCH;
    }
  else
    {
      /* Acquire hive lock */
      KeEnterCriticalRegion();
      ExAcquireResourceExclusiveLite(&CmiRegistryLock, TRUE);

      VERIFY_KEY_OBJECT(KeyObject);

      KeyObject->KeyCell->LastWriteTime.QuadPart =
        ((PKEY_WRITE_TIME_INFORMATION)KeyInformation)->LastWriteTime.QuadPart;

      CmiMarkBlockDirty (KeyObject->RegistryHive,
		         KeyObject->KeyCellOffset);

      /* Release hive lock */
      ExReleaseResourceLite(&CmiRegistryLock);
      KeLeaveCriticalRegion();
    }

  PostOperationInfo.Status = Status;
  CmiCallRegisteredCallbacks(RegNtPostSetInformationKey, &PostOperationInfo);

  ObDereferenceObject (KeyObject);

  if (NT_SUCCESS(Status))
    {
      CmiSyncHives ();
    }

  DPRINT ("NtSaveKey() done\n");

  return STATUS_SUCCESS;
}


/*
 * NOTE:
 * KeyObjectAttributes->RootDirectory specifies the handle to the parent key and
 * KeyObjectAttributes->Name specifies the name of the key to unload.
 */
NTSTATUS STDCALL
NtUnloadKey (IN POBJECT_ATTRIBUTES KeyObjectAttributes)
{
  PREGISTRY_HIVE RegistryHive;
  NTSTATUS Status;

  PAGED_CODE();

  DPRINT ("NtUnloadKey() called\n");

#if 0
  if (!SeSinglePrivilegeCheck (SeRestorePrivilege, ExGetPreviousMode ()))
    return STATUS_PRIVILEGE_NOT_HELD;
#endif

  /* Acquire registry lock exclusively */
  KeEnterCriticalRegion();
  ExAcquireResourceExclusiveLite(&CmiRegistryLock, TRUE);

  Status = CmiDisconnectHive (KeyObjectAttributes,
			      &RegistryHive);
  if (!NT_SUCCESS (Status))
    {
      DPRINT1 ("CmiDisconnectHive() failed (Status %lx)\n", Status);
      ExReleaseResourceLite (&CmiRegistryLock);
      KeLeaveCriticalRegion();
      return Status;
    }

  DPRINT ("RegistryHive %p\n", RegistryHive);

#if 0
  /* Flush hive */
  if (!IsNoFileHive (RegistryHive))
    CmiFlushRegistryHive (RegistryHive);
#endif

  CmiRemoveRegistryHive (RegistryHive);

  /* Release registry lock */
  ExReleaseResourceLite (&CmiRegistryLock);
  KeLeaveCriticalRegion();

  DPRINT ("NtUnloadKey() done\n");

  return STATUS_SUCCESS;
}


NTSTATUS STDCALL
NtInitializeRegistry (IN BOOLEAN SetUpBoot)
{
  NTSTATUS Status;

  PAGED_CODE();

  if (CmiRegistryInitialized == TRUE)
    return STATUS_ACCESS_DENIED;

  /* Save boot log file */
  IopSaveBootLogToFile();

  Status = CmiInitHives (SetUpBoot);

  CmiRegistryInitialized = TRUE;

  return Status;
}

/* EOF */
