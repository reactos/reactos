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
#include <internal/pool.h>
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
  CHAR cPath[MAX_PATH];
  NTSTATUS Status;
  PWSTR StartPtr;
  PWSTR EndPtr;
  ULONG Length;
  UNICODE_STRING LinkPath;
  UNICODE_STRING TargetPath;

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

  wcstombs(cPath, StartPtr, Length);
  cPath[Length] = 0;


  FoundObject = CmiScanKeyList(ParsedKey, cPath, Attributes);
  if (FoundObject == NULL)
    {
      Status = CmiScanForSubKey(ParsedKey->RegistryHive,
				ParsedKey->KeyCell,
				&SubKeyCell,
				&BlockOffset,
				cPath,
				0,
				Attributes);
      if (!NT_SUCCESS(Status) || (SubKeyCell == NULL))
	{
	  return(STATUS_UNSUCCESSFUL);
	}

      if ((SubKeyCell->Flags & REG_KEY_LINK_CELL) &&
	  !((Attributes & OBJ_OPENLINK) && (EndPtr == NULL) /*(end == NULL)*/))
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
	  return(Status);
	}

      FoundObject->Flags = 0;

      FoundObject->Name.Length = Length * sizeof(WCHAR);
      FoundObject->Name.MaximumLength = FoundObject->Name.Length + sizeof(WCHAR);
      FoundObject->Name.Buffer = ExAllocatePool(NonPagedPool, FoundObject->Name.MaximumLength);
      RtlCopyMemory(FoundObject->Name.Buffer, StartPtr, FoundObject->Name.Length);
      FoundObject->Name.Buffer[FoundObject->Name.Length / sizeof(WCHAR)] = 0;

      FoundObject->KeyCell = SubKeyCell;
      FoundObject->BlockOffset = BlockOffset;
      FoundObject->RegistryHive = ParsedKey->RegistryHive;
      CmiAddKeyToList(ParsedKey, FoundObject);
      DPRINT("Created object 0x%x\n", FoundObject);
    }
  else
    {
      if ((FoundObject->KeyCell->Flags & REG_KEY_LINK_CELL) &&
	  !((Attributes & OBJ_OPENLINK) && (EndPtr == NULL)/*(end == NULL)*/))
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

  DPRINT("Delete object key\n");

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

      if (!IsVolatileHive(KeyObject->RegistryHive))
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
  DPRINT1("CmiObjectSecurity() called\n");

  return(STATUS_SUCCESS);
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
	       PCHAR KeyName,
	       ULONG Attributes)
{
  PKEY_OBJECT CurKey;
  KIRQL OldIrql;
  ULONG Index;
  UNICODE_STRING UName;

  DPRINT("Scanning key list for: %s (Parent: %wZ)\n",
    KeyName, &Parent->Name);

  RtlCreateUnicodeStringFromAsciiz(&UName,
				   KeyName);

  KeAcquireSpinLock(&CmiKeyListLock, &OldIrql);
  /* FIXME: if list maintained in alphabetic order, use dichotomic search */
  for (Index=0; Index < Parent->NumberOfSubKeys; Index++)
    {
      CurKey = Parent->SubKeys[Index];
      if (Attributes & OBJ_CASE_INSENSITIVE)
	{
	  if ((UName.Length == CurKey->Name.Length)
	      && (_wcsicmp(UName.Buffer, CurKey->Name.Buffer) == 0))
	    {
	      KeReleaseSpinLock(&CmiKeyListLock, OldIrql);
	      RtlFreeUnicodeString(&UName);
	      return CurKey;
	    }
	}
      else
	{
	  if ((UName.Length == CurKey->Name.Length)
	      && (wcscmp(UName.Buffer, CurKey->Name.Buffer) == 0))
	    {
	      KeReleaseSpinLock(&CmiKeyListLock, OldIrql);
	      RtlFreeUnicodeString(&UName);
	      return CurKey;
	    }
	}
    }
  KeReleaseSpinLock(&CmiKeyListLock, OldIrql);
  RtlFreeUnicodeString(&UName);

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
