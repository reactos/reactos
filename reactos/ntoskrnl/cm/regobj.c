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
CmiGetLinkTarget(PREGISTRY_HIVE RegistryHive,
		 PKEY_CELL KeyCell,
		 PUNICODE_STRING TargetPath);

/* FUNCTONS *****************************************************************/

NTSTATUS STDCALL
CmiObjectParse(PVOID ParsedObject,
	       PVOID *NextObject,
	       PUNICODE_STRING FullPath,
	       PWSTR *Path,
	       ULONG Attributes)
{
  BLOCK_OFFSET BlockOffset;
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
      if (!NT_SUCCESS(Status) || (SubKeyCell == NULL))
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
      /* Add the keep-alive reference */
      ObReferenceObject(FoundObject);

      FoundObject->Flags = 0;
      FoundObject->KeyCell = SubKeyCell;
      FoundObject->KeyCellOffset = BlockOffset;
      FoundObject->RegistryHive = ParsedKey->RegistryHive;
      InsertTailList(&CmiKeyObjectListHead, &FoundObject->ListEntry);
      RtlpCreateUnicodeString(&FoundObject->Name,
              KeyName.Buffer, NonPagedPool);
      CmiAddKeyToList(ParsedKey, FoundObject);
      DPRINT("Created object 0x%x\n", FoundObject);
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

  DPRINT("CmiObjectParse: %s\n", FoundObject->Name);

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
      CmiMarkBlockDirty (ParentKeyObject->RegistryHive,
			 ParentKeyObject->KeyCellOffset);

      if (!IsNoFileHive (KeyObject->RegistryHive) ||
	  !IsNoFileHive (ParentKeyObject->RegistryHive))
	{
	  CmiSyncHives ();
	}
    }

  ObDereferenceObject (ParentKeyObject);

  ExReleaseResourceLite(&CmiRegistryLock);
  KeLeaveCriticalRegion();

  if (KeyObject->NumberOfSubKeys)
    {
      KEBUGCHECK(REGISTRY_ERROR);
    }

  if (KeyObject->SizeOfSubKeys)
    {
      ExFreePool(KeyObject->SubKeys);
    }
}


static NTSTATUS
CmiQuerySecurityDescriptor(PKEY_OBJECT KeyObject,
			   SECURITY_INFORMATION SecurityInformation,
			   PSECURITY_DESCRIPTOR SecurityDescriptor,
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
  PREGISTRY_HIVE Hive;

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
		  PULONG BufferLength)
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
		    POBJECT_NAME_INFORMATION ObjectNameInfo,
		    ULONG Length,
		    PULONG ReturnLength)
{
  POBJECT_NAME_INFORMATION LocalInfo;
  PKEY_OBJECT KeyObject;
  ULONG LocalReturnLength;
  NTSTATUS Status;

  DPRINT ("CmiObjectQueryName() called\n");

  KeyObject = (PKEY_OBJECT)ObjectBody;

  LocalInfo = ExAllocatePool (NonPagedPool,
			      sizeof(OBJECT_NAME_INFORMATION) +
				MAX_PATH * sizeof(WCHAR));
  if (LocalInfo == NULL)
    return STATUS_INSUFFICIENT_RESOURCES;

  if (KeyObject->ParentKey != KeyObject)
    {
      Status = ObQueryNameString (KeyObject->ParentKey,
				  LocalInfo,
				  MAX_PATH * sizeof(WCHAR),
				  &LocalReturnLength);
    }
  else
    {
      /* KeyObject is the root key */
      Status = ObQueryNameString (BODY_TO_HEADER(KeyObject)->Parent,
				  LocalInfo,
				  MAX_PATH * sizeof(WCHAR),
				  &LocalReturnLength);
    }

  if (!NT_SUCCESS (Status))
    {
      ExFreePool (LocalInfo);
      return Status;
    }
  DPRINT ("Parent path: %wZ\n", &LocalInfo->Name);

  Status = RtlAppendUnicodeStringToString (&ObjectNameInfo->Name,
					   &LocalInfo->Name);
  ExFreePool (LocalInfo);
  if (!NT_SUCCESS (Status))
    return Status;

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

  DPRINT("Reference parent key: 0x%x\n", ParentKey);

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
CmiGetLinkTarget(PREGISTRY_HIVE RegistryHive,
		 PKEY_CELL KeyCell,
		 PUNICODE_STRING TargetPath)
{
  UNICODE_STRING LinkName = ROS_STRING_INITIALIZER(L"SymbolicLinkValue");
  PVALUE_CELL ValueCell;
  PDATA_CELL DataCell;
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
      DataCell = CmiGetCell (RegistryHive, ValueCell->DataOffset, NULL);
      RtlCopyMemory(TargetPath->Buffer,
		    DataCell->Data,
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
