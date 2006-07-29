/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/cm/regobj.c
 * PURPOSE:         Registry object manipulation routines.
 *
 * PROGRAMMERS:     No programmer listed.
*/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#include "cm.h"

extern LIST_ENTRY CmiKeyObjectListHead;
extern ULONG CmiTimer;

static NTSTATUS
CmiGetLinkTarget(PEREGISTRY_HIVE RegistryHive,
		 PKEY_CELL KeyCell,
		 PUNICODE_STRING TargetPath);

/* FUNCTONS *****************************************************************/
NTSTATUS
NTAPI
CmFindObject(POBJECT_CREATE_INFORMATION ObjectCreateInfo,
             PUNICODE_STRING ObjectName,
             PVOID* ReturnedObject,
             PUNICODE_STRING RemainingPath,
             POBJECT_TYPE ObjectType,
             IN PACCESS_STATE AccessState,
             IN PVOID ParseContext)
{
    PVOID NextObject;
    PVOID CurrentObject;
    PVOID RootObject;
    POBJECT_HEADER CurrentHeader;
    NTSTATUS Status;
    PWSTR current;
    UNICODE_STRING PathString;
    ULONG Attributes;
    UNICODE_STRING CurrentUs;
    OBP_LOOKUP_CONTEXT Context;

    PAGED_CODE();

    DPRINT("CmindObject(ObjectCreateInfo %x, ReturnedObject %x, "
        "RemainingPath %x)\n",ObjectCreateInfo,ReturnedObject,RemainingPath);

    RtlInitUnicodeString (RemainingPath, NULL);

    if (ObjectCreateInfo->RootDirectory == NULL)
    {
        ObReferenceObjectByPointer(NameSpaceRoot,
            DIRECTORY_TRAVERSE,
            NULL,
            ObjectCreateInfo->ProbeMode);
        CurrentObject = NameSpaceRoot;
    }
    else
    {
        Status = ObReferenceObjectByHandle(ObjectCreateInfo->RootDirectory,
            0,
            NULL,
            ObjectCreateInfo->ProbeMode,
            &CurrentObject,
            NULL);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
    }

    if (ObjectName->Length == 0 ||
        ObjectName->Buffer[0] == UNICODE_NULL)
    {
        *ReturnedObject = CurrentObject;
        return STATUS_SUCCESS;
    }

    if (ObjectCreateInfo->RootDirectory == NULL &&
        ObjectName->Buffer[0] != L'\\')
    {
        ObDereferenceObject (CurrentObject);
        DPRINT1("failed\n");
        return STATUS_UNSUCCESSFUL;
    }

    /* Create a zero-terminated copy of the object name */
    PathString.Length = ObjectName->Length;
    PathString.MaximumLength = ObjectName->Length + sizeof(WCHAR);
    PathString.Buffer = ExAllocatePool (NonPagedPool,
        PathString.MaximumLength);
    if (PathString.Buffer == NULL)
    {
        ObDereferenceObject (CurrentObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory (PathString.Buffer,
        ObjectName->Buffer,
        ObjectName->Length);
    PathString.Buffer[PathString.Length / sizeof(WCHAR)] = UNICODE_NULL;

    current = PathString.Buffer;

    RootObject = CurrentObject;
    Attributes = ObjectCreateInfo->Attributes;
    if (ObjectType == ObSymbolicLinkType)
        Attributes |= OBJ_OPENLINK;
    Attributes |= OBJ_CASE_INSENSITIVE; // hello! My name is ReactOS CM and I'm brain-dead!

    while (TRUE)
    {
        CurrentHeader = OBJECT_TO_OBJECT_HEADER(CurrentObject);

        /* Loop as long as we're dealing with a directory */
        while (CurrentHeader->Type == ObDirectoryType)
        {
            PWSTR Start, End;
            PVOID FoundObject;
            UNICODE_STRING StartUs;
            NextObject = NULL;

            if (!current) goto Next;

            Start = current;
            if (*Start == L'\\') Start++;

            End = wcschr(Start, L'\\');
            if (End != NULL) *End = 0;

            RtlInitUnicodeString(&StartUs, Start);
            Context.DirectoryLocked = TRUE;
            Context.Directory = CurrentObject;
            FoundObject = ObpLookupEntryDirectory(CurrentObject, &StartUs, Attributes, FALSE, &Context);
            if (FoundObject == NULL)
            {
                if (End != NULL)
                {
                    *End = L'\\';
                }
                 goto Next;
            }

            ObReferenceObjectByPointer(FoundObject,
                STANDARD_RIGHTS_REQUIRED,
                NULL,
                UserMode);
            if (End != NULL)
            {
                *End = L'\\';
                current = End;
            }
            else
            {
                current = NULL;
            }

            NextObject = FoundObject;

Next:
            if (NextObject == NULL)
            {
                break;
            }
            ObDereferenceObject(CurrentObject);
            CurrentObject = NextObject;
            CurrentHeader = OBJECT_TO_OBJECT_HEADER(CurrentObject);
        }

        if (CurrentHeader->Type->TypeInfo.ParseProcedure == NULL)
        {
            DPRINT("Current object can't parse\n");
            break;
        }

        RtlInitUnicodeString(&CurrentUs, current);
        Status = CurrentHeader->Type->TypeInfo.ParseProcedure(CurrentObject,
            CurrentHeader->Type,
            AccessState,
            ExGetPreviousMode(), // fixme: should be a parameter, since caller decides.
            Attributes,
            &PathString,
            &CurrentUs,
            ParseContext,
            NULL, // fixme: where do we get this from? captured OBP?
            &NextObject);
        current = CurrentUs.Buffer;
        if (Status == STATUS_REPARSE)
        {
            /* reparse the object path */
            NextObject = NameSpaceRoot;
            current = PathString.Buffer;

            ObReferenceObjectByPointer(NextObject,
                DIRECTORY_TRAVERSE,
                NULL,
                ObjectCreateInfo->ProbeMode);
        }


        if (NextObject == NULL)
        {
            break;
        }
        ObDereferenceObject(CurrentObject);
        CurrentObject = NextObject;
    }

    if (current)
    {
        RtlpCreateUnicodeString (RemainingPath, current, NonPagedPool);
    }

    RtlFreeUnicodeString (&PathString);
    *ReturnedObject = CurrentObject;

    return STATUS_SUCCESS;
}

NTSTATUS STDCALL
CmiObjectParse(IN PVOID ParsedObject,
               IN PVOID ObjectType,
               IN OUT PACCESS_STATE AccessState,
               IN KPROCESSOR_MODE AccessMode,
               IN ULONG Attributes,
               IN OUT PUNICODE_STRING FullPath,
               IN OUT PUNICODE_STRING RemainingName,
               IN OUT PVOID Context OPTIONAL,
               IN PSECURITY_QUALITY_OF_SERVICE SecurityQos OPTIONAL,
               OUT PVOID *NextObject)
{
  HCELL_INDEX BlockOffset;
  PKEY_OBJECT FoundObject;
  PKEY_OBJECT ParsedKey;
  PKEY_CELL SubKeyCell;
  NTSTATUS Status;
  PWSTR StartPtr;
  PWSTR EndPtr;
  ULONG Length;
  UNICODE_STRING LinkPath;
  UNICODE_STRING TargetPath;
  UNICODE_STRING KeyName;
  PWSTR *Path = &RemainingName->Buffer;

  ParsedKey = ParsedObject;

  VERIFY_KEY_OBJECT(ParsedKey);

  *NextObject = NULL;

  if ((*Path) == NULL)
    {
      DPRINT("*Path is NULL\n");
      return STATUS_UNSUCCESSFUL;
    }

  DPRINT("Path '%S'\n", *Path);

  /* Extract relevant path name */
  StartPtr = *Path;
  if (*StartPtr == L'\\')
    StartPtr++;

  EndPtr = wcschr(StartPtr, L'\\');
  if (EndPtr != NULL)
    Length = ((PCHAR)EndPtr - (PCHAR)StartPtr) / sizeof(WCHAR);
  else
    Length = wcslen(StartPtr);


  KeyName.Length = Length * sizeof(WCHAR);
  KeyName.MaximumLength = KeyName.Length + sizeof(WCHAR);
  KeyName.Buffer = ExAllocatePool(NonPagedPool,
				  KeyName.MaximumLength);
  RtlCopyMemory(KeyName.Buffer,
		StartPtr,
		KeyName.Length);
  KeyName.Buffer[KeyName.Length / sizeof(WCHAR)] = 0;

  /* Acquire hive lock */
  KeEnterCriticalRegion();
  ExAcquireResourceExclusiveLite(&CmiRegistryLock, TRUE);

  Status = CmiScanKeyList(ParsedKey,
			  &KeyName,
			  Attributes,
			  &FoundObject);
  if (!NT_SUCCESS(Status))
  {
     ExReleaseResourceLite(&CmiRegistryLock);
     KeLeaveCriticalRegion();
     RtlFreeUnicodeString(&KeyName);
     return Status;
  }
  if (FoundObject == NULL)
    {
      Status = CmiScanForSubKey(ParsedKey->RegistryHive,
                                ParsedKey->KeyCell,
                                &SubKeyCell,
                                &BlockOffset,
                                &KeyName,
                                0,
                                Attributes);
      if (!NT_SUCCESS(Status))
        {
          ExReleaseResourceLite(&CmiRegistryLock);
          KeLeaveCriticalRegion();
          RtlFreeUnicodeString(&KeyName);
          return(STATUS_UNSUCCESSFUL);
        }

      if ((SubKeyCell->Flags & REG_KEY_LINK_CELL) &&
          !((Attributes & OBJ_OPENLINK) && (EndPtr == NULL)))
        {
          RtlInitUnicodeString(&LinkPath, NULL);
          Status = CmiGetLinkTarget(ParsedKey->RegistryHive,
                                    SubKeyCell,
                                    &LinkPath);
          if (NT_SUCCESS(Status))
            {
              ExReleaseResourceLite(&CmiRegistryLock);
              KeLeaveCriticalRegion();

              DPRINT("LinkPath '%wZ'\n", &LinkPath);

              /* build new FullPath for reparsing */
              TargetPath.MaximumLength = LinkPath.MaximumLength;
              if (EndPtr != NULL)
                {
                  TargetPath.MaximumLength += (wcslen(EndPtr) * sizeof(WCHAR));
                }
              TargetPath.Length = TargetPath.MaximumLength - sizeof(WCHAR);
              TargetPath.Buffer = ExAllocatePool(NonPagedPool,
                                                 TargetPath.MaximumLength);
              wcscpy(TargetPath.Buffer, LinkPath.Buffer);
              if (EndPtr != NULL)
                {
                  wcscat(TargetPath.Buffer, EndPtr);
                }

              RtlFreeUnicodeString(FullPath);
              RtlFreeUnicodeString(&LinkPath);
              FullPath->Length = TargetPath.Length;
              FullPath->MaximumLength = TargetPath.MaximumLength;
              FullPath->Buffer = TargetPath.Buffer;

              DPRINT("FullPath '%wZ'\n", FullPath);

              /* reinitialize Path for reparsing */
              *Path = FullPath->Buffer;

              *NextObject = NULL;

               RtlFreeUnicodeString(&KeyName);
               return(STATUS_REPARSE);
             }
        }

      /* Create new key object and put into linked list */
      DPRINT("CmiObjectParse: %S\n", *Path);
      Status = ObCreateObject(KernelMode,
                              CmiKeyType,
                              NULL,
                              KernelMode,
                              NULL,
                              sizeof(KEY_OBJECT),
                              0,
                              0,
                              (PVOID*)&FoundObject);
      if (!NT_SUCCESS(Status))
        {
          ExReleaseResourceLite(&CmiRegistryLock);
          KeLeaveCriticalRegion();
          RtlFreeUnicodeString(&KeyName);
          return(Status);
        }
      DPRINT("Inserting Key into Object Tree\n");
      Status = ObInsertObject((PVOID)FoundObject,
                              NULL,
                              KEY_ALL_ACCESS,
                              0,
                              NULL,
                              NULL);
      DPRINT("Status %x\n", Status);

      /* Add the keep-alive reference */
      ObReferenceObject(FoundObject);

      FoundObject->Flags = 0;
      FoundObject->KeyCell = SubKeyCell;
      FoundObject->KeyCellOffset = BlockOffset;
      FoundObject->RegistryHive = ParsedKey->RegistryHive;
      InsertTailList(&CmiKeyObjectListHead, &FoundObject->ListEntry);
      RtlpCreateUnicodeString(&FoundObject->Name, KeyName.Buffer, NonPagedPool);
      CmiAddKeyToList(ParsedKey, FoundObject);
      DPRINT("Created object 0x%p\n", FoundObject);
    }
  else
    {
      if ((FoundObject->KeyCell->Flags & REG_KEY_LINK_CELL) &&
	  !((Attributes & OBJ_OPENLINK) && (EndPtr == NULL)))
	{
	  DPRINT("Found link\n");

	  RtlInitUnicodeString(&LinkPath, NULL);
	  Status = CmiGetLinkTarget(FoundObject->RegistryHive,
				    FoundObject->KeyCell,
				    &LinkPath);
	  if (NT_SUCCESS(Status))
	    {
	      DPRINT("LinkPath '%wZ'\n", &LinkPath);

              ExReleaseResourceLite(&CmiRegistryLock);
              KeLeaveCriticalRegion();

	      ObDereferenceObject(FoundObject);

	      /* build new FullPath for reparsing */
	      TargetPath.MaximumLength = LinkPath.MaximumLength;
	      if (EndPtr != NULL)
		{
		  TargetPath.MaximumLength += (wcslen(EndPtr) * sizeof(WCHAR));
		}
	      TargetPath.Length = TargetPath.MaximumLength - sizeof(WCHAR);
	      TargetPath.Buffer = ExAllocatePool(NonPagedPool,
						 TargetPath.MaximumLength);
	      wcscpy(TargetPath.Buffer, LinkPath.Buffer);
	      if (EndPtr != NULL)
		{
		  wcscat(TargetPath.Buffer, EndPtr);
		}

	      RtlFreeUnicodeString(FullPath);
	      RtlFreeUnicodeString(&LinkPath);
	      FullPath->Length = TargetPath.Length;
	      FullPath->MaximumLength = TargetPath.MaximumLength;
	      FullPath->Buffer = TargetPath.Buffer;

	      DPRINT("FullPath '%wZ'\n", FullPath);

	      /* reinitialize Path for reparsing */
	      *Path = FullPath->Buffer;

	      *NextObject = NULL;

	      RtlFreeUnicodeString(&KeyName);
	      return(STATUS_REPARSE);
	    }
	}
    }

  RemoveEntryList(&FoundObject->ListEntry);
  InsertHeadList(&CmiKeyObjectListHead, &FoundObject->ListEntry);
  FoundObject->TimeStamp = CmiTimer;

  ExReleaseResourceLite(&CmiRegistryLock);
  KeLeaveCriticalRegion();

  DPRINT("CmiObjectParse: %wZ\n", &FoundObject->Name);

  *Path = EndPtr;

  VERIFY_KEY_OBJECT(FoundObject);

  *NextObject = FoundObject;

  RtlFreeUnicodeString(&KeyName);

  return(STATUS_SUCCESS);
}

VOID STDCALL
CmiObjectDelete(PVOID DeletedObject)
{
  PKEY_OBJECT ParentKeyObject;
  PKEY_OBJECT KeyObject;

  DPRINT("Delete key object (%p)\n", DeletedObject);

  KeyObject = (PKEY_OBJECT) DeletedObject;
  ParentKeyObject = KeyObject->ParentKey;

  ObReferenceObject (ParentKeyObject);

  /* Acquire hive lock */
  KeEnterCriticalRegion();
  ExAcquireResourceExclusiveLite(&CmiRegistryLock, TRUE);

  if (!NT_SUCCESS(CmiRemoveKeyFromList(KeyObject)))
    {
      DPRINT1("Key not found in parent list ???\n");
    }

  RemoveEntryList(&KeyObject->ListEntry);
  RtlFreeUnicodeString(&KeyObject->Name);

  if (KeyObject->Flags & KO_MARKED_FOR_DELETE)
    {
      DPRINT("delete really key\n");

      CmiRemoveSubKey(KeyObject->RegistryHive,
		      ParentKeyObject,
		      KeyObject);

      KeQuerySystemTime (&ParentKeyObject->KeyCell->LastWriteTime);
      HvMarkCellDirty (ParentKeyObject->RegistryHive->Hive,
                       ParentKeyObject->KeyCellOffset);

      if (!IsNoFileHive (KeyObject->RegistryHive) ||
	  !IsNoFileHive (ParentKeyObject->RegistryHive))
	{
	  CmiSyncHives ();
	}
    }

  ObDereferenceObject (ParentKeyObject);

  if (KeyObject->NumberOfSubKeys)
    {
      KEBUGCHECK(REGISTRY_ERROR);
    }

  if (KeyObject->SizeOfSubKeys)
    {
      ExFreePool(KeyObject->SubKeys);
    }

  ExReleaseResourceLite(&CmiRegistryLock);
  KeLeaveCriticalRegion();
}


static NTSTATUS
CmiQuerySecurityDescriptor(PKEY_OBJECT KeyObject,
			   SECURITY_INFORMATION SecurityInformation,
			   PISECURITY_DESCRIPTOR SecurityDescriptor,
			   PULONG BufferLength)
{
  ULONG_PTR Current;
  ULONG SidSize;
  ULONG SdSize;
  NTSTATUS Status;

  DPRINT("CmiQuerySecurityDescriptor() called\n");

  /*
   * FIXME:
   * This is a big hack!!
   * We need to retrieve the security descriptor from the keys security cell!
   */

  if (SecurityInformation == 0)
    {
      return STATUS_ACCESS_DENIED;
    }

  SidSize = RtlLengthSid(SeWorldSid);
  SdSize = sizeof(SECURITY_DESCRIPTOR) + (2 * SidSize);

  if (*BufferLength < SdSize)
    {
      *BufferLength = SdSize;
      return STATUS_BUFFER_TOO_SMALL;
    }

  *BufferLength = SdSize;

  Status = RtlCreateSecurityDescriptor(SecurityDescriptor,
				       SECURITY_DESCRIPTOR_REVISION);
  if (!NT_SUCCESS(Status))
    {
      return Status;
    }

  SecurityDescriptor->Control |= SE_SELF_RELATIVE;
  Current = (ULONG_PTR)SecurityDescriptor + sizeof(SECURITY_DESCRIPTOR);

  if (SecurityInformation & OWNER_SECURITY_INFORMATION)
    {
      RtlCopyMemory((PVOID)Current,
		    SeWorldSid,
		    SidSize);
      SecurityDescriptor->Owner = (PSID)((ULONG_PTR)Current - (ULONG_PTR)SecurityDescriptor);
      Current += SidSize;
    }

  if (SecurityInformation & GROUP_SECURITY_INFORMATION)
    {
      RtlCopyMemory((PVOID)Current,
		    SeWorldSid,
		    SidSize);
      SecurityDescriptor->Group = (PSID)((ULONG_PTR)Current - (ULONG_PTR)SecurityDescriptor);
      Current += SidSize;
    }

  if (SecurityInformation & DACL_SECURITY_INFORMATION)
    {
      SecurityDescriptor->Control |= SE_DACL_PRESENT;
    }

  if (SecurityInformation & SACL_SECURITY_INFORMATION)
    {
      SecurityDescriptor->Control |= SE_SACL_PRESENT;
    }

  return STATUS_SUCCESS;
}


static NTSTATUS
CmiAssignSecurityDescriptor(PKEY_OBJECT KeyObject,
			    PSECURITY_DESCRIPTOR SecurityDescriptor)
{
#if 0
  PEREGISTRY_HIVE Hive;

  DPRINT1("CmiAssignSecurityDescriptor() callled\n");

  DPRINT1("KeyObject %p\n", KeyObject);
  DPRINT1("KeyObject->RegistryHive %p\n", KeyObject->RegistryHive);

  Hive = KeyObject->RegistryHive;
  if (Hive == NULL)
    {
      DPRINT1("Create new root security cell\n");
      return STATUS_SUCCESS;
    }

  if (Hive->RootSecurityCell == NULL)
    {
      DPRINT1("Create new root security cell\n");

    }
  else
    {
      DPRINT1("Search for security cell\n");

    }
#endif

  return STATUS_SUCCESS;
}


NTSTATUS STDCALL
CmiObjectSecurity(PVOID ObjectBody,
		  SECURITY_OPERATION_CODE OperationCode,
		  SECURITY_INFORMATION SecurityInformation,
		  PSECURITY_DESCRIPTOR SecurityDescriptor,
		  PULONG BufferLength,
		  PSECURITY_DESCRIPTOR *OldSecurityDescriptor,
		  POOL_TYPE PoolType,
		  PGENERIC_MAPPING GenericMapping)
{
  DPRINT("CmiObjectSecurity() called\n");

  switch (OperationCode)
    {
      case SetSecurityDescriptor:
        DPRINT("Set security descriptor\n");
        return STATUS_SUCCESS;

      case QuerySecurityDescriptor:
        DPRINT("Query security descriptor\n");
        return CmiQuerySecurityDescriptor((PKEY_OBJECT)ObjectBody,
					  SecurityInformation,
					  SecurityDescriptor,
					  BufferLength);

      case DeleteSecurityDescriptor:
        DPRINT("Delete security descriptor\n");
        return STATUS_SUCCESS;

      case AssignSecurityDescriptor:
        DPRINT("Assign security descriptor\n");
        return CmiAssignSecurityDescriptor((PKEY_OBJECT)ObjectBody,
					   SecurityDescriptor);
    }

  return STATUS_UNSUCCESSFUL;
}


NTSTATUS STDCALL
CmiObjectQueryName (PVOID ObjectBody,
            IN BOOLEAN HasName,
		    POBJECT_NAME_INFORMATION ObjectNameInfo,
		    ULONG Length,
		    PULONG ReturnLength,
            IN KPROCESSOR_MODE PreviousMode)
{
  PKEY_OBJECT KeyObject;
  NTSTATUS Status;

  DPRINT ("CmiObjectQueryName() called\n");

  KeyObject = (PKEY_OBJECT)ObjectBody;

  if (KeyObject->ParentKey != KeyObject)
    {
      Status = ObQueryNameString (KeyObject->ParentKey,
				  ObjectNameInfo,
				  Length,
				  ReturnLength);
    }
  else
    {
      /* KeyObject is the root key */
      Status = ObQueryNameString (OBJECT_HEADER_TO_NAME_INFO(OBJECT_TO_OBJECT_HEADER(KeyObject))->Directory,
				  ObjectNameInfo,
				  Length,
				  ReturnLength);
    }

  if (!NT_SUCCESS(Status) && Status != STATUS_INFO_LENGTH_MISMATCH)
    {
      return Status;
    }
  (*ReturnLength) += sizeof(WCHAR) + KeyObject->Name.Length;

  if (Status == STATUS_INFO_LENGTH_MISMATCH || *ReturnLength > Length)
    {
      return STATUS_INFO_LENGTH_MISMATCH;
    }

  if (ObjectNameInfo->Name.Buffer == NULL)
    {
      ObjectNameInfo->Name.Buffer = (PWCHAR)(ObjectNameInfo + 1);
      ObjectNameInfo->Name.Length = 0;
      ObjectNameInfo->Name.MaximumLength = Length - sizeof(OBJECT_NAME_INFORMATION);
    }


  DPRINT ("Parent path: %wZ\n", ObjectNameInfo->Name);

  Status = RtlAppendUnicodeToString (&ObjectNameInfo->Name,
				     L"\\");
  if (!NT_SUCCESS (Status))
    return Status;

  Status = RtlAppendUnicodeStringToString (&ObjectNameInfo->Name,
					   &KeyObject->Name);
  if (NT_SUCCESS (Status))
    {
      DPRINT ("Total path: %wZ\n", &ObjectNameInfo->Name);
    }

  return Status;
}



VOID
CmiAddKeyToList(PKEY_OBJECT ParentKey,
		PKEY_OBJECT NewKey)
{

  DPRINT("ParentKey %.08x\n", ParentKey);


  if (ParentKey->SizeOfSubKeys <= ParentKey->NumberOfSubKeys)
    {
      PKEY_OBJECT *tmpSubKeys = ExAllocatePool(NonPagedPool,
	(ParentKey->NumberOfSubKeys + 1) * sizeof(ULONG));

      if (ParentKey->NumberOfSubKeys > 0)
	{
	  RtlCopyMemory (tmpSubKeys,
			 ParentKey->SubKeys,
			 ParentKey->NumberOfSubKeys * sizeof(ULONG));
	}

      if (ParentKey->SubKeys)
	ExFreePool(ParentKey->SubKeys);

      ParentKey->SubKeys = tmpSubKeys;
      ParentKey->SizeOfSubKeys = ParentKey->NumberOfSubKeys + 1;
    }

  /* FIXME: Please maintain the list in alphabetic order */
  /*      to allow a dichotomic search */
  ParentKey->SubKeys[ParentKey->NumberOfSubKeys++] = NewKey;

  DPRINT("Reference parent key: 0x%p\n", ParentKey);

  ObReferenceObjectByPointer(ParentKey,
		STANDARD_RIGHTS_REQUIRED,
		NULL,
		UserMode);
  NewKey->ParentKey = ParentKey;
}


NTSTATUS
CmiRemoveKeyFromList(PKEY_OBJECT KeyToRemove)
{
  PKEY_OBJECT ParentKey;
  DWORD Index;

  ParentKey = KeyToRemove->ParentKey;
  /* FIXME: If list maintained in alphabetic order, use dichotomic search */
  for (Index = 0; Index < ParentKey->NumberOfSubKeys; Index++)
    {
      if (ParentKey->SubKeys[Index] == KeyToRemove)
	{
	  if (Index < ParentKey->NumberOfSubKeys-1)
	    RtlMoveMemory(&ParentKey->SubKeys[Index],
			  &ParentKey->SubKeys[Index + 1],
			  (ParentKey->NumberOfSubKeys - Index - 1) * sizeof(PKEY_OBJECT));
	  ParentKey->NumberOfSubKeys--;

	  DPRINT("Dereference parent key: 0x%x\n", ParentKey);

	  ObDereferenceObject(ParentKey);
	  return STATUS_SUCCESS;
	}
    }

  return STATUS_UNSUCCESSFUL;
}


NTSTATUS
CmiScanKeyList(PKEY_OBJECT Parent,
	       PUNICODE_STRING KeyName,
	       ULONG Attributes,
	       PKEY_OBJECT* ReturnedObject)
{
  PKEY_OBJECT CurKey;
  ULONG Index;

  DPRINT("Scanning key list for: %wZ (Parent: %wZ)\n",
	 KeyName, &Parent->Name);

  /* FIXME: if list maintained in alphabetic order, use dichotomic search */
  for (Index=0; Index < Parent->NumberOfSubKeys; Index++)
    {
      CurKey = Parent->SubKeys[Index];
      if (Attributes & OBJ_CASE_INSENSITIVE)
	{
	  DPRINT("Comparing %wZ and %wZ\n", KeyName, &CurKey->Name);
	  if ((KeyName->Length == CurKey->Name.Length)
	      && (_wcsicmp(KeyName->Buffer, CurKey->Name.Buffer) == 0))
	    {
	      break;
	    }
	}
      else
	{
	  if ((KeyName->Length == CurKey->Name.Length)
	      && (wcscmp(KeyName->Buffer, CurKey->Name.Buffer) == 0))
	    {
	      break;
	    }
	}
    }

  if (Index < Parent->NumberOfSubKeys)
  {
     if (CurKey->Flags & KO_MARKED_FOR_DELETE)
     {
        CHECKPOINT;
        *ReturnedObject = NULL;
	return STATUS_UNSUCCESSFUL;
     }
     ObReferenceObject(CurKey);
     *ReturnedObject = CurKey;
  }
  else
  {
     *ReturnedObject = NULL;
  }
  return STATUS_SUCCESS;
}


static NTSTATUS
CmiGetLinkTarget(PEREGISTRY_HIVE RegistryHive,
		 PKEY_CELL KeyCell,
		 PUNICODE_STRING TargetPath)
{
  UNICODE_STRING LinkName = RTL_CONSTANT_STRING(L"SymbolicLinkValue");
  PVALUE_CELL ValueCell;
  PVOID DataCell;
  NTSTATUS Status;

  DPRINT("CmiGetLinkTarget() called\n");

  /* Get Value block of interest */
  Status = CmiScanKeyForValue(RegistryHive,
			      KeyCell,
			      &LinkName,
			      &ValueCell,
			      NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("CmiScanKeyForValue() failed (Status %lx)\n", Status);
      return(Status);
    }

  if (ValueCell->DataType != REG_LINK)
    {
      DPRINT1("Type != REG_LINK\n!");
      return(STATUS_UNSUCCESSFUL);
    }

  if (TargetPath->Buffer == NULL && TargetPath->MaximumLength == 0)
    {
      TargetPath->Length = 0;
      TargetPath->MaximumLength = ValueCell->DataSize + sizeof(WCHAR);
      TargetPath->Buffer = ExAllocatePool(NonPagedPool,
					  TargetPath->MaximumLength);
    }

  TargetPath->Length = min(TargetPath->MaximumLength - sizeof(WCHAR),
			   (ULONG) ValueCell->DataSize);

  if (ValueCell->DataSize > 0)
    {
      DataCell = HvGetCell (RegistryHive->Hive, ValueCell->DataOffset);
      RtlCopyMemory(TargetPath->Buffer,
		    DataCell,
		    TargetPath->Length);
      TargetPath->Buffer[TargetPath->Length / sizeof(WCHAR)] = 0;
    }
  else
    {
      RtlCopyMemory(TargetPath->Buffer,
		    &ValueCell->DataOffset,
		    TargetPath->Length);
      TargetPath->Buffer[TargetPath->Length / sizeof(WCHAR)] = 0;
    }

  DPRINT("TargetPath '%wZ'\n", TargetPath);

  return(STATUS_SUCCESS);
}

/* EOF */
