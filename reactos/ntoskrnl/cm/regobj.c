/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             ntoskrnl/cm/regobj.c
 * PURPOSE:          Registry object manipulation routines.
 * UPDATE HISTORY:
*/

#include <ddk/ntddk.h>
#include <roscfg.h>
#include <internal/ob.h>
#include <limits.h>
#include <string.h>
#include <internal/registry.h>
#include <ntos/minmax.h>

#define NDEBUG
#include <internal/debug.h>

#include "cm.h"


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


  FoundObject = CmiScanKeyList(ParsedKey,
			       &KeyName,
			       Attributes);
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
      DPRINT("CmiObjectParse: %s\n", cPath);
      Status = ObCreateObject(NULL,
			      STANDARD_RIGHTS_REQUIRED,
			      NULL,
			      CmiKeyType,
			      (PVOID*)&FoundObject);
      if (!NT_SUCCESS(Status))
	{
	  RtlFreeUnicodeString(&KeyName);
	  return(Status);
	}

      FoundObject->Flags = 0;
      FoundObject->KeyCell = SubKeyCell;
      FoundObject->BlockOffset = BlockOffset;
      FoundObject->RegistryHive = ParsedKey->RegistryHive;
      RtlCreateUnicodeString(&FoundObject->Name,
			     KeyName.Buffer);
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

      ObReferenceObjectByPointer(FoundObject,
				 STANDARD_RIGHTS_REQUIRED,
				 NULL,
				 UserMode);
    }

  DPRINT("CmiObjectParse: %s\n", FoundObject->Name);

  *Path = EndPtr;

  VERIFY_KEY_OBJECT(FoundObject);

  *NextObject = FoundObject;

  RtlFreeUnicodeString(&KeyName);

  return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
CmiObjectCreate(PVOID ObjectBody,
		PVOID Parent,
		PWSTR RemainingPath,
		POBJECT_ATTRIBUTES ObjectAttributes)
{
  PKEY_OBJECT KeyObject = ObjectBody;
  PWSTR Start;

  KeyObject->ParentKey = Parent;
  if (RemainingPath)
    {
      Start = RemainingPath;
      if(*Start == L'\\')
	Start++;
      RtlCreateUnicodeString(&KeyObject->Name,
			     Start);
    }
   else
    {
      RtlInitUnicodeString(&KeyObject->Name,
			   NULL);
    }

  return STATUS_SUCCESS;
}


VOID STDCALL
CmiObjectDelete(PVOID DeletedObject)
{
  PKEY_OBJECT KeyObject;

  DPRINT("Delete key object (%p)\n", DeletedObject);

  KeyObject = (PKEY_OBJECT) DeletedObject;

  if (!NT_SUCCESS(CmiRemoveKeyFromList(KeyObject)))
    {
      DPRINT1("Key not found in parent list ???\n");
    }

  RtlFreeUnicodeString(&KeyObject->Name);

  if (KeyObject->Flags & KO_MARKED_FOR_DELETE)
    {
      DPRINT("delete really key\n");

      CmiRemoveSubKey(KeyObject->RegistryHive,
		      KeyObject->ParentKey,
		      KeyObject);

      if (!IsNoFileHive(KeyObject->RegistryHive))
	{
	  CmiSyncHives();
	}
    }
}


NTSTATUS STDCALL
CmiObjectSecurity(PVOID ObjectBody,
		  SECURITY_OPERATION_CODE OperationCode,
		  SECURITY_INFORMATION SecurityInformation,
		  PSECURITY_DESCRIPTOR SecurityDescriptor,
		  PULONG BufferLength)
{
  DPRINT1 ("CmiObjectSecurity() called\n");

  return STATUS_SUCCESS;
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
  KIRQL OldIrql;

  DPRINT("ParentKey %.08x\n", ParentKey);

  KeAcquireSpinLock(&CmiKeyListLock, &OldIrql);

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
  KeReleaseSpinLock(&CmiKeyListLock, OldIrql);
}


NTSTATUS
CmiRemoveKeyFromList(PKEY_OBJECT KeyToRemove)
{
  PKEY_OBJECT ParentKey;
  KIRQL OldIrql;
  DWORD Index;

  ParentKey = KeyToRemove->ParentKey;
  KeAcquireSpinLock(&CmiKeyListLock, &OldIrql);
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
	  KeReleaseSpinLock(&CmiKeyListLock, OldIrql);

	  DPRINT("Dereference parent key: 0x%x\n", ParentKey);
	
	  ObDereferenceObject(ParentKey);
	  return STATUS_SUCCESS;
	}
    }
  KeReleaseSpinLock(&CmiKeyListLock, OldIrql);

  return STATUS_UNSUCCESSFUL;
}


PKEY_OBJECT
CmiScanKeyList(PKEY_OBJECT Parent,
	       PUNICODE_STRING KeyName,
	       ULONG Attributes)
{
  PKEY_OBJECT CurKey;
  KIRQL OldIrql;
  ULONG Index;

  DPRINT("Scanning key list for: %wZ (Parent: %wZ)\n",
	 KeyName, &Parent->Name);

  KeAcquireSpinLock(&CmiKeyListLock, &OldIrql);
  /* FIXME: if list maintained in alphabetic order, use dichotomic search */
  for (Index=0; Index < Parent->NumberOfSubKeys; Index++)
    {
      CurKey = Parent->SubKeys[Index];
      if (Attributes & OBJ_CASE_INSENSITIVE)
	{
	  if ((KeyName->Length == CurKey->Name.Length)
	      && (_wcsicmp(KeyName->Buffer, CurKey->Name.Buffer) == 0))
	    {
	      KeReleaseSpinLock(&CmiKeyListLock, OldIrql);
	      return CurKey;
	    }
	}
      else
	{
	  if ((KeyName->Length == CurKey->Name.Length)
	      && (wcscmp(KeyName->Buffer, CurKey->Name.Buffer) == 0))
	    {
	      KeReleaseSpinLock(&CmiKeyListLock, OldIrql);
	      return CurKey;
	    }
	}
    }
  KeReleaseSpinLock(&CmiKeyListLock, OldIrql);

  return NULL;
}


static NTSTATUS
CmiGetLinkTarget(PREGISTRY_HIVE RegistryHive,
		 PKEY_CELL KeyCell,
		 PUNICODE_STRING TargetPath)
{
  UNICODE_STRING LinkName = UNICODE_STRING_INITIALIZER(L"SymbolicLinkValue");
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
      DataCell = CmiGetBlock(RegistryHive, ValueCell->DataOffset, NULL);
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
