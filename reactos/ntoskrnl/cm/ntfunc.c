/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             ntoskrnl/cm/ntfunc.c
 * PURPOSE:          Ntxxx function for registry access
 * UPDATE HISTORY:
*/

/* INCLUDES *****************************************************************/

#ifdef WIN32_REGDBG
#include "cm_win32.h"
#else
#include <ddk/ntddk.h>
#include <roscfg.h>
#include <internal/ob.h>
#include <limits.h>
#include <string.h>
#include <internal/pool.h>
#include <internal/registry.h>

#define NDEBUG
#include <internal/debug.h>

#include "cm.h"
#endif


/* GLOBALS ******************************************************************/

extern POBJECT_TYPE  CmiKeyType;
extern PREGISTRY_HIVE  CmiVolatileHive;

static BOOLEAN CmiRegistryInitialized = FALSE;


/* FUNCTIONS ****************************************************************/

NTSTATUS STDCALL
NtCreateKey(OUT PHANDLE KeyHandle,
	    IN ACCESS_MASK DesiredAccess,
	    IN POBJECT_ATTRIBUTES ObjectAttributes,
	    IN ULONG TitleIndex,
	    IN PUNICODE_STRING Class,
	    IN ULONG CreateOptions,
	    OUT PULONG Disposition)
{
  UNICODE_STRING RemainingPath;
  PKEY_OBJECT KeyObject;
  NTSTATUS Status;
  PVOID Object;
  PWSTR End;

  DPRINT("NtCreateKey (Name %wZ  KeyHandle %x  Root %x)\n",
	 ObjectAttributes->ObjectName,
	 KeyHandle,
	 ObjectAttributes->RootDirectory);

  /* FIXME: check for standard handle prefix and adjust objectAttributes accordingly */

  Status = ObFindObject(ObjectAttributes,
			&Object,
			&RemainingPath,
			CmiKeyType);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  DPRINT("RemainingPath %wZ\n", &RemainingPath);

  if ((RemainingPath.Buffer == NULL) || (RemainingPath.Buffer[0] == 0))
    {
      /* Fail if the key has been deleted */
      if (((PKEY_OBJECT) Object)->Flags & KO_MARKED_FOR_DELETE)
	{
	  ObDereferenceObject(Object);
	  return(STATUS_UNSUCCESSFUL);
	}

      if (Disposition)
	*Disposition = REG_OPENED_EXISTING_KEY;

      Status = ObCreateHandle(PsGetCurrentProcess(),
			      Object,
			      DesiredAccess,
			      FALSE,
			      KeyHandle);

      DPRINT("Status %x\n", Status);
      ObDereferenceObject(Object);
      return Status;
    }

  /* If RemainingPath contains \ we must return error
     because NtCreateKey don't create trees */
  if (RemainingPath.Buffer[0] == '\\')
    End = wcschr(RemainingPath.Buffer + 1, '\\');
  else
    End = wcschr(RemainingPath.Buffer, '\\');

  if (End != NULL)
    {
      ObDereferenceObject(Object);
      return STATUS_OBJECT_NAME_NOT_FOUND;
    }

  DPRINT("RemainingPath %S  ParentObject %x\n", RemainingPath.Buffer, Object);

  Status = ObCreateObject(KeyHandle,
			  DesiredAccess,
			  NULL,
			  CmiKeyType,
			  (PVOID*)&KeyObject);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
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
  ExAcquireResourceExclusiveLite(&KeyObject->RegistryHive->HiveResource, TRUE);

  /* add key to subkeys of parent if needed */
  Status = CmiAddSubKey(KeyObject->RegistryHive,
			KeyObject->ParentKey,
			KeyObject,
			RemainingPath.Buffer,
			RemainingPath.Length,
			TitleIndex,
			Class,
			CreateOptions);
  if (!NT_SUCCESS(Status))
    {
      /* Release hive lock */
      ExReleaseResourceLite(&KeyObject->RegistryHive->HiveResource);
      ObDereferenceObject(KeyObject);
      ObDereferenceObject(Object);
      return STATUS_UNSUCCESSFUL;
    }

  KeyObject->Name = KeyObject->KeyCell->Name;
  KeyObject->NameSize = KeyObject->KeyCell->NameSize;

  if (KeyObject->RegistryHive == KeyObject->ParentKey->RegistryHive)
    {
      KeyObject->KeyCell->ParentKeyOffset = KeyObject->ParentKey->BlockOffset;
      KeyObject->KeyCell->SecurityKeyOffset = KeyObject->ParentKey->KeyCell->SecurityKeyOffset;
    }
  else
    {
      KeyObject->KeyCell->ParentKeyOffset = -1;
      KeyObject->KeyCell->SecurityKeyOffset = -1;
      /* This key must remain in memory unless it is deleted
	 or file is unloaded */
      ObReferenceObjectByPointer(KeyObject,
				 STANDARD_RIGHTS_REQUIRED,
				 NULL,
				 UserMode);
    }

  CmiAddKeyToList(KeyObject->ParentKey, KeyObject);

  VERIFY_KEY_OBJECT(KeyObject);

  /* Release hive lock */
  ExReleaseResourceLite(&KeyObject->RegistryHive->HiveResource);

  ObDereferenceObject(KeyObject);
  ObDereferenceObject(Object);

  if (Disposition)
    *Disposition = REG_CREATED_NEW_KEY;

  CmiSyncHives();

  return Status;
}


NTSTATUS STDCALL
NtDeleteKey(IN HANDLE KeyHandle)
{
  PKEY_OBJECT KeyObject;
  NTSTATUS Status;

  DPRINT("KeyHandle %x\n", KeyHandle);

  /* Verify that the handle is valid and is a registry key */
  Status = ObReferenceObjectByHandle(KeyHandle,
				     KEY_WRITE,
				     CmiKeyType,
				     UserMode,
				     (PVOID *)&KeyObject,
				     NULL);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  /* Acquire hive lock */
  ExAcquireResourceExclusiveLite(&KeyObject->RegistryHive->HiveResource, TRUE);

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
  ExReleaseResourceLite(&KeyObject->RegistryHive->HiveResource);

  DPRINT1("PointerCount %lu\n", ObGetObjectPointerCount((PVOID)KeyObject));

  /* Dereference the object */
  ObDereferenceObject(KeyObject);
  if(KeyObject->RegistryHive != KeyObject->ParentKey->RegistryHive)
    ObDereferenceObject(KeyObject);

  DPRINT1("PointerCount %lu\n", ObGetObjectPointerCount((PVOID)KeyObject));

  /*
   * Note:
   * Hive-Synchronization will not be triggered here. This is done in
   * CmiObjectDelete() (in regobj.c) after all key-related structures
   * have been released.
   */

  return(Status);
}


NTSTATUS STDCALL
NtEnumerateKey(
	IN	HANDLE			KeyHandle,
	IN	ULONG			  Index,
	IN	KEY_INFORMATION_CLASS	KeyInformationClass,
	OUT	PVOID			  KeyInformation,
	IN	ULONG			  Length,
	OUT	PULONG			ResultLength
	)
{
  NTSTATUS  Status;
  PKEY_OBJECT  KeyObject;
  PREGISTRY_HIVE  RegistryHive;
  PKEY_CELL  KeyCell, SubKeyCell;
  PHASH_TABLE_CELL  HashTableBlock;
  PKEY_BASIC_INFORMATION  BasicInformation;
  PKEY_NODE_INFORMATION  NodeInformation;
  PKEY_FULL_INFORMATION  FullInformation;
  PDATA_CELL pClassData;

  DPRINT("KH %x  I %d  KIC %x KI %x  L %d  RL %x\n",
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
		UserMode,
		(PVOID *) &KeyObject,
		NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("ObReferenceObjectByHandle() failed with status %x\n", Status);
      return(Status);
    }

  /* Acquire hive lock */
  ExAcquireResourceSharedLite(&KeyObject->RegistryHive->HiveResource, TRUE);

  VERIFY_KEY_OBJECT(KeyObject);

  /* Get pointer to KeyCell */
  KeyCell = KeyObject->KeyCell;
  RegistryHive = KeyObject->RegistryHive;

  /* Get pointer to SubKey */
  if (Index >= KeyCell->NumberOfSubKeys)
    {
      if (RegistryHive == CmiVolatileHive)
	{
	  ExReleaseResourceLite(&KeyObject->RegistryHive->HiveResource);
	  ObDereferenceObject(KeyObject);
	  DPRINT("No more volatile entries\n");
	  return(STATUS_NO_MORE_ENTRIES);
	}
      else
	{
	  ULONG i;
	  PKEY_OBJECT CurKey = NULL;

	  /* Search volatile keys */
	  for (i = 0; i < KeyObject->NumberOfSubKeys; i++)
	    {
	      CurKey = KeyObject->SubKeys[i];
	      if (CurKey->RegistryHive == CmiVolatileHive)
		{
		  if (Index-- == KeyObject->NumberOfSubKeys)
		    break;
		}
	    }
	  if (Index >= KeyCell->NumberOfSubKeys)
	    {
	      ExReleaseResourceLite(&KeyObject->RegistryHive->HiveResource);
	      ObDereferenceObject(KeyObject);
	      DPRINT("No more non-volatile entries\n");
	      return(STATUS_NO_MORE_ENTRIES);
	    }
	  SubKeyCell = CurKey->KeyCell;
	}
    }
  else
    {
      HashTableBlock = CmiGetBlock(RegistryHive, KeyCell->HashTableOffset, NULL);
      SubKeyCell = CmiGetKeyFromHashByIndex(RegistryHive,
					    HashTableBlock,
					    Index);
    }

  if (SubKeyCell == NULL)
    {
      ExReleaseResourceLite(&KeyObject->RegistryHive->HiveResource);
      ObDereferenceObject(KeyObject);
      DPRINT("No more entries\n");
      return(STATUS_NO_MORE_ENTRIES);
    }

  Status = STATUS_SUCCESS;
  switch (KeyInformationClass)
    {
    case KeyBasicInformation:
      /* Check size of buffer */
      *ResultLength = sizeof(KEY_BASIC_INFORMATION) + 
        (SubKeyCell->NameSize ) * sizeof(WCHAR);
      if (Length < *ResultLength)
        {
          Status = STATUS_BUFFER_OVERFLOW;
        }
      else
        {
          /* Fill buffer with requested info */
          BasicInformation = (PKEY_BASIC_INFORMATION) KeyInformation;
          BasicInformation->LastWriteTime.u.LowPart = SubKeyCell->LastWriteTime.dwLowDateTime;
          BasicInformation->LastWriteTime.u.HighPart = SubKeyCell->LastWriteTime.dwHighDateTime;
          BasicInformation->TitleIndex = Index;
          BasicInformation->NameLength = SubKeyCell->NameSize * sizeof(WCHAR);
          mbstowcs(BasicInformation->Name, 
            SubKeyCell->Name, 
            SubKeyCell->NameSize * 2);
//          BasicInformation->Name[SubKeyCell->NameSize] = 0;
        }
      break;
      
    case KeyNodeInformation:
      /* Check size of buffer */
      *ResultLength = sizeof(KEY_NODE_INFORMATION) +
        SubKeyCell->NameSize * sizeof(WCHAR) +
        SubKeyCell->ClassSize;
      if (Length < *ResultLength)
        {
          Status = STATUS_BUFFER_OVERFLOW;
        }
      else
        {
          /* Fill buffer with requested info */
          NodeInformation = (PKEY_NODE_INFORMATION) KeyInformation;
          NodeInformation->LastWriteTime.u.LowPart = SubKeyCell->LastWriteTime.dwLowDateTime;
          NodeInformation->LastWriteTime.u.HighPart = SubKeyCell->LastWriteTime.dwHighDateTime;
          NodeInformation->TitleIndex = Index;
          NodeInformation->ClassOffset = sizeof(KEY_NODE_INFORMATION) + 
            SubKeyCell->NameSize * sizeof(WCHAR);
          NodeInformation->ClassLength = SubKeyCell->ClassSize;
          NodeInformation->NameLength = SubKeyCell->NameSize * sizeof(WCHAR);
          mbstowcs(NodeInformation->Name, 
            SubKeyCell->Name,
            SubKeyCell->NameSize * 2);
//          NodeInformation->Name[SubKeyCell->NameSize] = 0;
          if (SubKeyCell->ClassSize != 0)
            {
              pClassData=CmiGetBlock(KeyObject->RegistryHive,
                SubKeyCell->ClassNameOffset,
                NULL);
              wcsncpy(NodeInformation->Name + SubKeyCell->NameSize ,
                      (PWCHAR) pClassData->Data,
                      SubKeyCell->ClassSize);
              CmiReleaseBlock(RegistryHive, pClassData);
            }
        }
      break;
      
    case KeyFullInformation:
      /*  check size of buffer  */
      *ResultLength = sizeof(KEY_FULL_INFORMATION) +
          SubKeyCell->ClassSize;
      if (Length < *ResultLength)
        {
          Status = STATUS_BUFFER_OVERFLOW;
        }
      else
        {
          /* fill buffer with requested info  */
          FullInformation = (PKEY_FULL_INFORMATION) KeyInformation;
          FullInformation->LastWriteTime.u.LowPart = SubKeyCell->LastWriteTime.dwLowDateTime;
          FullInformation->LastWriteTime.u.HighPart = SubKeyCell->LastWriteTime.dwHighDateTime;
          FullInformation->TitleIndex = Index;
          FullInformation->ClassOffset = sizeof(KEY_FULL_INFORMATION) - 
            sizeof(WCHAR);
          FullInformation->ClassLength = SubKeyCell->ClassSize;
          FullInformation->SubKeys = SubKeyCell->NumberOfSubKeys;
          FullInformation->MaxNameLen = 
            CmiGetMaxNameLength(RegistryHive, SubKeyCell);
          FullInformation->MaxClassLen = 
            CmiGetMaxClassLength(RegistryHive, SubKeyCell);
          FullInformation->Values = SubKeyCell->NumberOfValues;
          FullInformation->MaxValueNameLen = 
            CmiGetMaxValueNameLength(RegistryHive, SubKeyCell);
          FullInformation->MaxValueDataLen = 
            CmiGetMaxValueDataLength(RegistryHive, SubKeyCell);
          if (SubKeyCell->ClassSize != 0)
            {
              pClassData = CmiGetBlock(KeyObject->RegistryHive,
                SubKeyCell->ClassNameOffset,
                NULL);
              wcsncpy(FullInformation->Class,
                (PWCHAR) pClassData->Data,
                SubKeyCell->ClassSize);
              CmiReleaseBlock(RegistryHive, pClassData);
            }
        }
      break;
    }
  CmiReleaseBlock(RegistryHive, SubKeyCell);

  ExReleaseResourceLite(&KeyObject->RegistryHive->HiveResource);
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
  PKEY_VALUE_BASIC_INFORMATION  ValueBasicInformation;
  PKEY_VALUE_PARTIAL_INFORMATION  ValuePartialInformation;
  PKEY_VALUE_FULL_INFORMATION  ValueFullInformation;

  DPRINT("KH %x  I %d  KVIC %x  KVI %x  L %d  RL %x\n",
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
		UserMode,
		(PVOID *) &KeyObject,
		NULL);

  if (!NT_SUCCESS(Status))
    {
      return Status;
    }

  /* Acquire hive lock */
  ExAcquireResourceSharedLite(&KeyObject->RegistryHive->HiveResource, TRUE);

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
      ExReleaseResourceLite(&KeyObject->RegistryHive->HiveResource);
      ObDereferenceObject(KeyObject);
      return Status;
    }

  if (ValueCell != NULL)
    {
      switch (KeyValueInformationClass)
        {
        case KeyValueBasicInformation:
          if (ValueCell->Flags & REG_VALUE_NAME_PACKED)
            {
              *ResultLength = sizeof(KEY_VALUE_BASIC_INFORMATION) + 
                (ValueCell->NameSize + 1) * sizeof(WCHAR);
            }
          else
            {
              *ResultLength = sizeof(KEY_VALUE_BASIC_INFORMATION) + 
                ValueCell->NameSize + sizeof(WCHAR);
            }
          if (Length < *ResultLength)
            {
              Status = STATUS_BUFFER_OVERFLOW;
            }
          else
            {
              ValueBasicInformation = (PKEY_VALUE_BASIC_INFORMATION) 
                KeyValueInformation;
              ValueBasicInformation->TitleIndex = 0;
              ValueBasicInformation->Type = ValueCell->DataType;
              if (ValueCell->Flags & REG_VALUE_NAME_PACKED)
                {
                  ValueBasicInformation->NameLength =
                    (ValueCell->NameSize + 1) * sizeof(WCHAR);
                  CmiCopyPackedName(ValueBasicInformation->Name,
                                    ValueCell->Name,
                                    ValueCell->NameSize);
                  ValueBasicInformation->Name[ValueCell->NameSize] = 0;
                }
              else
                {
                  ValueBasicInformation->NameLength =
                    ValueCell->NameSize + sizeof(WCHAR);
                  RtlCopyMemory(ValueBasicInformation->Name,
                                ValueCell->Name,
                                ValueCell->NameSize * sizeof(WCHAR));
                  ValueBasicInformation->Name[ValueCell->NameSize / sizeof(WCHAR)] = 0;
                }
            }
          break;

        case KeyValuePartialInformation:
          *ResultLength = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 
            (ValueCell->DataSize & LONG_MAX);
          if (Length < *ResultLength)
            {
              Status = STATUS_BUFFER_OVERFLOW;
            }
          else
            {
              ValuePartialInformation = (PKEY_VALUE_PARTIAL_INFORMATION)
                KeyValueInformation;
              ValuePartialInformation->TitleIndex = 0;
              ValuePartialInformation->Type = ValueCell->DataType;
              ValuePartialInformation->DataLength = ValueCell->DataSize & LONG_MAX;
              if(ValueCell->DataSize >0)
              {
                DataCell = CmiGetBlock(RegistryHive, ValueCell->DataOffset, NULL);
                RtlCopyMemory(ValuePartialInformation->Data, 
                  DataCell->Data,
                  ValueCell->DataSize & LONG_MAX);
                CmiReleaseBlock(RegistryHive, DataCell);
              }
              else
              {
                RtlCopyMemory(ValuePartialInformation->Data, 
                  &ValueCell->DataOffset, 
                  ValueCell->DataSize & LONG_MAX);
              }
              DataCell = CmiGetBlock(RegistryHive, ValueCell->DataOffset, NULL);
            }
          break;

        case KeyValueFullInformation:
          if (ValueCell->Flags & REG_VALUE_NAME_PACKED)
            {
              *ResultLength = sizeof(KEY_VALUE_FULL_INFORMATION) + 
                (ValueCell->NameSize + 1) * sizeof(WCHAR) +
                (ValueCell->DataSize & LONG_MAX);
            }
          else
            {
              *ResultLength = sizeof(KEY_VALUE_FULL_INFORMATION) + 
                ValueCell->NameSize + sizeof(WCHAR) +
                (ValueCell->DataSize & LONG_MAX);
            }
          if (Length < *ResultLength)
            {
              Status = STATUS_BUFFER_OVERFLOW;
            }
          else
            {
              ValueFullInformation = (PKEY_VALUE_FULL_INFORMATION) 
                KeyValueInformation;
              ValueFullInformation->TitleIndex = 0;
              ValueFullInformation->Type = ValueCell->DataType;
              if (ValueCell->Flags & REG_VALUE_NAME_PACKED)
                {
                  ValueFullInformation->NameLength =
                    (ValueCell->NameSize + 1) * sizeof(WCHAR);

                  CmiCopyPackedName(ValueFullInformation->Name,
				    ValueCell->Name,
				    ValueCell->NameSize);
                  ValueFullInformation->Name[ValueCell->NameSize] = 0;
                }
              else
                {
                  ValueFullInformation->NameLength =
                    ValueCell->NameSize + sizeof(WCHAR);
                  RtlCopyMemory(ValueFullInformation->Name,
				ValueCell->Name,
				ValueCell->NameSize);
                  ValueFullInformation->Name[ValueCell->NameSize / sizeof(WCHAR)] = 0;
                }
              ValueFullInformation->DataOffset = 
                (ULONG)ValueFullInformation->Name - (ULONG)ValueFullInformation +
                ValueFullInformation->NameLength;
              ValueFullInformation->DataOffset =
                (ValueFullInformation->DataOffset + 3) & 0xfffffffc;
              ValueFullInformation->DataLength = ValueCell->DataSize & LONG_MAX;
              if (ValueCell->DataSize > 0)
                {
                  DataCell = CmiGetBlock(RegistryHive, ValueCell->DataOffset, NULL);
                  RtlCopyMemory((PCHAR) ValueFullInformation
                    + ValueFullInformation->DataOffset,
                    DataCell->Data,
                    ValueCell->DataSize & LONG_MAX);
                  CmiReleaseBlock(RegistryHive, DataCell);
                }
              else
                {
                  RtlCopyMemory((PCHAR) ValueFullInformation
                    + ValueFullInformation->DataOffset,
                    &ValueCell->DataOffset,
                    ValueCell->DataSize & LONG_MAX);
                }
            }
          break;
        }
    }
  else
    {
      Status = STATUS_UNSUCCESSFUL;
    }

  ExReleaseResourceLite(&KeyObject->RegistryHive->HiveResource);
  ObDereferenceObject(KeyObject);

  return Status;
}


NTSTATUS STDCALL
NtFlushKey(IN HANDLE KeyHandle)
{
  NTSTATUS Status;
  PKEY_OBJECT  KeyObject;
  PREGISTRY_HIVE  RegistryHive;
#if 0
  WCHAR LogName[MAX_PATH];
  UNICODE_STRING TmpFileName;
  HANDLE FileHandle;
//  HANDLE FileHandleLog;
  OBJECT_ATTRIBUTES ObjectAttributes;
  LARGE_INTEGER fileOffset;
  DWORD * pEntDword;
  ULONG i;
#endif

  DPRINT("KeyHandle %x\n", KeyHandle);

  /* Verify that the handle is valid and is a registry key */
  Status = ObReferenceObjectByHandle(KeyHandle,
				     KEY_QUERY_VALUE,
				     CmiKeyType,
				     UserMode,
				     (PVOID *)&KeyObject,
				     NULL);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  VERIFY_KEY_OBJECT(KeyObject);

  RegistryHive = KeyObject->RegistryHive;

  /* Acquire hive lock */
  ExAcquireResourceExclusiveLite(&RegistryHive->HiveResource,
				 TRUE);

  if (IsPermanentHive(RegistryHive))
    {
      /* Flush non-volatile hive */
      Status = CmiFlushRegistryHive(RegistryHive);
    }
  else
    {
      Status = STATUS_SUCCESS;
    }


#if 0
  /* Then write changed blocks in .log */
  wcscpy(LogName,RegistryHive->Filename.Buffer);
  wcscat(LogName,L".log");
  RtlInitUnicodeString (&TmpFileName, LogName);
  InitializeObjectAttributes(&ObjectAttributes,
			     &TmpFileName,
			     0,
			     NULL,
			     NULL);

/* BEGIN FIXME : actually (26 November 200) vfatfs.sys can't create new files
   so we can't create log file
  Status = ZwCreateFile(&FileHandleLog,
		FILE_ALL_ACCESS,
		&ObjectAttributes,
		NULL,
    0,
    FILE_ATTRIBUTE_NORMAL,
		0,
    FILE_SUPERSEDE,
    0,
    NULL,
    0);

  if (!NT_SUCCESS(Status))
    {
      ObDereferenceObject(KeyObject);
      return Status;
    }

  Status = ZwWriteFile(FileHandleLog, 
		0,
    0,
    0,
    0, 
		RegistryHive->HiveHeader, 
		sizeof(HIVE_HEADER), 
		0,
    0);

  if (!NT_SUCCESS(Status))
    {
      ZwClose(FileHandleLog);
      ObDereferenceObject(KeyObject);
      return Status;
    }

  for (i = 0; i < RegistryHive->BlockListSize ; i++)
  {
    if ( RegistryHive->BlockList[i]->DateModified.dwHighDateTime
	    > RegistryHive->HiveHeader->DateModified.dwHighDateTime
      || (RegistryHive->BlockList[i]->DateModified.dwHighDateTime
	        == RegistryHive->HiveHeader->DateModified.dwHighDateTime
          && RegistryHive->BlockList[i]->DateModified.dwLowDateTime
	        > RegistryHive->HiveHeader->DateModified.dwLowDateTime
         )
       )

      Status = ZwWriteFile(FileHandleLog, 
        0,
        0,
        0,
        0, 
        RegistryHive->BlockList[i],
        RegistryHive->BlockList[i]->BlockSize ,
        0,
        0);

  if (!NT_SUCCESS(Status))
    {
      ZwClose(FileHandleLog);
      ObDereferenceObject(KeyObject);
      return Status;
    }
  }
  ZwClose(FileHandleLog);
END FIXME*/

  /* Update header of RegistryHive with Version >VersionOld */
  /* this allow recover if system crash while updating hove file */
  InitializeObjectAttributes(&ObjectAttributes,
		&RegistryHive->Filename,
		0,
		NULL,
		NULL);

  Status = NtOpenFile(&FileHandle,
		      FILE_ALL_ACCESS,
		      &ObjectAttributes,
		      NULL,
		      0,
		      FILE_SYNCHRONOUS_IO_NONALERT);
  if (!NT_SUCCESS(Status))
    {
      ExReleaseResourceLite(&KeyObject->RegistryHive->HiveResource);
      ObDereferenceObject(KeyObject);
      return Status;
    }

  RegistryHive->HiveHeader->Version++;

  Status = ZwWriteFile(FileHandle,
		       0,
		       0,
		       0,
		       0,
		       RegistryHive->HiveHeader,
		       sizeof(HIVE_HEADER),
		       0,
		       0);
  if (!NT_SUCCESS(Status))
    {
      ZwClose(FileHandle);
      ExReleaseResourceLite(&KeyObject->RegistryHive->HiveResource);
      ObDereferenceObject(KeyObject);
      return Status;
    }

  /* Update changed blocks in file */
  fileOffset.u.HighPart = 0;
  for (i = 0; i < RegistryHive->BlockListSize ; i++)
    {
      if (RegistryHive->BlockList[i]->DateModified.dwHighDateTime
	    > RegistryHive->HiveHeader->DateModified.dwHighDateTime
	  || (RegistryHive->BlockList[i]->DateModified.dwHighDateTime
	        == RegistryHive->HiveHeader->DateModified.dwHighDateTime
	      && RegistryHive->BlockList[i]->DateModified.dwLowDateTime
	         > RegistryHive->HiveHeader->DateModified.dwLowDateTime))
	{
	  fileOffset.u.LowPart = RegistryHive->BlockList[i]->BlockOffset+4096;
	  Status = NtWriteFile(FileHandle,
			       0,
			       0,
			       0,
			       0,
			       RegistryHive->BlockList[i],
			       RegistryHive->BlockList[i]->BlockSize,
			       &fileOffset,
			       0);
	  if (!NT_SUCCESS(Status))
	    {
	      ZwClose(FileHandle);
	      ExReleaseResourceLite(&KeyObject->RegistryHive->HiveResource);
	      ObDereferenceObject(KeyObject);
	      return(Status);
	    }
	}
    }

  /* Change version in header */
  RegistryHive->HiveHeader->VersionOld = RegistryHive->HiveHeader->Version;
  ZwQuerySystemTime((PTIME) &RegistryHive->HiveHeader->DateModified);

  /* Calculate checksum */
  RegistryHive->HiveHeader->Checksum = 0;
  pEntDword = (DWORD *) RegistryHive->HiveHeader;
  for (i = 0; i < 127 ; i++)
    {
      RegistryHive->HiveHeader->Checksum ^= pEntDword[i];
    }

  /* Write new header */
  fileOffset.u.LowPart = 0;
  Status = ZwWriteFile(FileHandle,
		       0,
		       0,
		       0,
		       0,
		       RegistryHive->HiveHeader,
		       sizeof(HIVE_HEADER),
		       &fileOffset,
		       0);

  if (!NT_SUCCESS(Status))
    {
      ZwClose(FileHandle);
      ExReleaseResourceLite(&KeyObject->RegistryHive->HiveResource);
      ObDereferenceObject(KeyObject);
      return Status;
    }

  ZwClose(FileHandle);
#endif

  ExReleaseResourceLite(&RegistryHive->HiveResource);

  ObDereferenceObject(KeyObject);

  return STATUS_SUCCESS;
}


NTSTATUS STDCALL
NtOpenKey(OUT PHANDLE KeyHandle,
	  IN ACCESS_MASK DesiredAccess,
	  IN POBJECT_ATTRIBUTES ObjectAttributes)
{
  UNICODE_STRING RemainingPath;
  NTSTATUS Status;
  PVOID Object;

  DPRINT("NtOpenFile(KH %x  DA %x  OA %x  OA->ON '%wZ'\n",
	 KeyHandle,
	 DesiredAccess,
	 ObjectAttributes,
	 ObjectAttributes ? ObjectAttributes->ObjectName : NULL);

  RemainingPath.Buffer = NULL;
  Status = ObFindObject(ObjectAttributes,
			&Object,
			&RemainingPath,
			CmiKeyType);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  VERIFY_KEY_OBJECT((PKEY_OBJECT) Object);

  DPRINT("RemainingPath '%wZ'\n", &RemainingPath);

  if ((RemainingPath.Buffer != NULL) && (RemainingPath.Buffer[0] != 0))
    {
      ObDereferenceObject(Object);
      return(STATUS_UNSUCCESSFUL);
    }

  /* Fail if the key has been deleted */
  if (((PKEY_OBJECT)Object)->Flags & KO_MARKED_FOR_DELETE)
    {
      ObDereferenceObject(Object);
      return(STATUS_UNSUCCESSFUL);
    }

  Status = ObCreateHandle(PsGetCurrentProcess(),
			  Object,
			  DesiredAccess,
			  FALSE,
			  KeyHandle);
  ObDereferenceObject(Object);

  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  return(STATUS_SUCCESS);
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
  PDATA_CELL pClassData;
  PKEY_OBJECT KeyObject;
  PKEY_CELL KeyCell;
  NTSTATUS Status;

  DPRINT("KH %x  KIC %x  KI %x  L %d  RL %x\n",
	 KeyHandle,
	 KeyInformationClass,
	 KeyInformation,
	 Length,
	 ResultLength);

  /* Verify that the handle is valid and is a registry key */
  Status = ObReferenceObjectByHandle(KeyHandle,
		KEY_READ,
		CmiKeyType,
		UserMode,
		(PVOID *) &KeyObject,
		NULL);
  if (!NT_SUCCESS(Status))
    {
CHECKPOINT1;
      return Status;
    }
CHECKPOINT1;

  /* Acquire hive lock */
  ExAcquireResourceSharedLite(&KeyObject->RegistryHive->HiveResource, TRUE);
CHECKPOINT1;

  VERIFY_KEY_OBJECT(KeyObject);

  /* Get pointer to KeyCell */
  KeyCell = KeyObject->KeyCell;
  RegistryHive = KeyObject->RegistryHive;

  Status = STATUS_SUCCESS;
  switch (KeyInformationClass)
    {
    case KeyBasicInformation:
CHECKPOINT1;
      /* Check size of buffer */
      if (Length < sizeof(KEY_BASIC_INFORMATION) + 
          KeyObject->NameSize * sizeof(WCHAR))
        {
          Status = STATUS_BUFFER_OVERFLOW;
        }
      else
        {
          /* Fill buffer with requested info */
          BasicInformation = (PKEY_BASIC_INFORMATION) KeyInformation;
          BasicInformation->LastWriteTime.u.LowPart = KeyCell->LastWriteTime.dwLowDateTime;
          BasicInformation->LastWriteTime.u.HighPart = KeyCell->LastWriteTime.dwHighDateTime;
          BasicInformation->TitleIndex = 0;
          BasicInformation->NameLength = (KeyObject->NameSize) * sizeof(WCHAR);
          mbstowcs(BasicInformation->Name,
            KeyObject->Name, 
            KeyObject->NameSize*sizeof(WCHAR));
          *ResultLength = sizeof(KEY_BASIC_INFORMATION) + 
            KeyObject->NameSize * sizeof(WCHAR);
        }
CHECKPOINT1;
      break;

    case KeyNodeInformation:
      /* Check size of buffer */
      if (Length < sizeof(KEY_NODE_INFORMATION)
         + KeyObject->NameSize * sizeof(WCHAR)
         + KeyCell->ClassSize)
        {
          Status = STATUS_BUFFER_OVERFLOW;
        }
      else
        {
          /* Fill buffer with requested info */
          NodeInformation = (PKEY_NODE_INFORMATION) KeyInformation;
          NodeInformation->LastWriteTime.u.LowPart = KeyCell->LastWriteTime.dwLowDateTime;
          NodeInformation->LastWriteTime.u.HighPart = KeyCell->LastWriteTime.dwHighDateTime;
          NodeInformation->TitleIndex = 0;
          NodeInformation->ClassOffset = sizeof(KEY_NODE_INFORMATION) + 
            KeyObject->NameSize * sizeof(WCHAR);
          NodeInformation->ClassLength = KeyCell->ClassSize;
          NodeInformation->NameLength = KeyObject->NameSize * sizeof(WCHAR);
          mbstowcs(NodeInformation->Name,
            KeyObject->Name,
            KeyObject->NameSize * sizeof(WCHAR));

          if (KeyCell->ClassSize != 0)
            {
              pClassData = CmiGetBlock(KeyObject->RegistryHive,
                KeyCell->ClassNameOffset,
                NULL);
              wcsncpy(NodeInformation->Name + KeyObject->NameSize * sizeof(WCHAR),
                (PWCHAR)pClassData->Data,
                KeyCell->ClassSize);
              CmiReleaseBlock(RegistryHive, pClassData);
            }
          *ResultLength = sizeof(KEY_NODE_INFORMATION)
            + KeyObject->NameSize * sizeof(WCHAR)
            + KeyCell->ClassSize;
        }
      break;

    case KeyFullInformation:
      /* Check size of buffer */
      if (Length < sizeof(KEY_FULL_INFORMATION) + KeyCell->ClassSize)
        {
          Status = STATUS_BUFFER_OVERFLOW;
        }
      else
        {
          /* Fill buffer with requested info */
          FullInformation = (PKEY_FULL_INFORMATION) KeyInformation;
          FullInformation->LastWriteTime.u.LowPart = KeyCell->LastWriteTime.dwLowDateTime;
          FullInformation->LastWriteTime.u.HighPart = KeyCell->LastWriteTime.dwHighDateTime;
          FullInformation->TitleIndex = 0;
          FullInformation->ClassOffset = sizeof(KEY_FULL_INFORMATION) - sizeof(WCHAR);
          FullInformation->ClassLength = KeyCell->ClassSize;
          FullInformation->SubKeys = KeyCell->NumberOfSubKeys;
          FullInformation->MaxNameLen =
            CmiGetMaxNameLength(RegistryHive, KeyCell);
          FullInformation->MaxClassLen =
            CmiGetMaxClassLength(RegistryHive, KeyCell);
          FullInformation->Values = KeyCell->NumberOfValues;
          FullInformation->MaxValueNameLen =
            CmiGetMaxValueNameLength(RegistryHive, KeyCell);
          FullInformation->MaxValueDataLen = 
            CmiGetMaxValueDataLength(RegistryHive, KeyCell);
          if (KeyCell->ClassSize != 0)
            {
              pClassData=CmiGetBlock(KeyObject->RegistryHive,
                KeyCell->ClassNameOffset,
                NULL);
              wcsncpy(FullInformation->Class,
                (PWCHAR)pClassData->Data,
                KeyCell->ClassSize);
              CmiReleaseBlock(RegistryHive, pClassData);
            }
          *ResultLength = sizeof(KEY_FULL_INFORMATION) + KeyCell->ClassSize;
        }
      break;
    }

CHECKPOINT1;
  ExReleaseResourceLite(&KeyObject->RegistryHive->HiveResource);
CHECKPOINT1;
  ObDereferenceObject(KeyObject);
CHECKPOINT1;

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
  PKEY_OBJECT  KeyObject;
  PREGISTRY_HIVE  RegistryHive;
  PKEY_CELL  KeyCell;
  PVALUE_CELL  ValueCell;
  PDATA_CELL  DataCell;
  PKEY_VALUE_BASIC_INFORMATION  ValueBasicInformation;
  PKEY_VALUE_PARTIAL_INFORMATION  ValuePartialInformation;
  PKEY_VALUE_FULL_INFORMATION  ValueFullInformation;

  DPRINT("NtQueryValueKey(KeyHandle %x  ValueName %S  Length %x)\n",
    KeyHandle, ValueName->Buffer, Length);

  /* Verify that the handle is valid and is a registry key */
  Status = ObReferenceObjectByHandle(KeyHandle,
		KEY_QUERY_VALUE,
		CmiKeyType,
		UserMode,
		(PVOID *)&KeyObject,
		NULL);

  if (!NT_SUCCESS(Status))
    {
      DPRINT("ObReferenceObjectByHandle() failed with status %x\n", Status);
      return Status;
    }

  /* Acquire hive lock */
  ExAcquireResourceSharedLite(&KeyObject->RegistryHive->HiveResource, TRUE);

  VERIFY_KEY_OBJECT(KeyObject);

  /* Get pointer to KeyCell */
  KeyCell = KeyObject->KeyCell;
  RegistryHive = KeyObject->RegistryHive;

  /* Get Value block of interest */
  Status = CmiScanKeyForValue(RegistryHive,
			      KeyCell,
			      ValueName,
			      &ValueCell,
			      NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("CmiScanKeyForValue() failed with status %x\n", Status);
      ExReleaseResourceLite(&KeyObject->RegistryHive->HiveResource);
      ObDereferenceObject(KeyObject);
      return(Status);
    }
  else if (ValueCell != NULL)
    {
      switch (KeyValueInformationClass)
        {
        case KeyValueBasicInformation:
          if (ValueCell->Flags & REG_VALUE_NAME_PACKED)
            {
              *ResultLength = sizeof(KEY_VALUE_BASIC_INFORMATION) + 
                (ValueCell->NameSize + 1) * sizeof(WCHAR);
            }
          else
            {
              *ResultLength = sizeof(KEY_VALUE_BASIC_INFORMATION) + 
                ValueCell->NameSize + sizeof(WCHAR);
            }
          if (Length < *ResultLength)
            {
              Status = STATUS_BUFFER_TOO_SMALL;
            }
          else
            {
              ValueBasicInformation = (PKEY_VALUE_BASIC_INFORMATION) 
                KeyValueInformation;
              ValueBasicInformation->TitleIndex = 0;
              ValueBasicInformation->Type = ValueCell->DataType;
              if (ValueCell->Flags & REG_VALUE_NAME_PACKED)
                {
                  ValueBasicInformation->NameLength =
                    (ValueCell->NameSize + 1) * sizeof(WCHAR);
                  CmiCopyPackedName(ValueBasicInformation->Name,
                                    ValueCell->Name,
                                    ValueCell->NameSize);
                  ValueBasicInformation->Name[ValueCell->NameSize] = 0;
                }
              else
                {
                  ValueBasicInformation->NameLength =
                    ValueCell->NameSize + sizeof(WCHAR);
                  RtlCopyMemory(ValueBasicInformation->Name,
                                ValueCell->Name,
                                ValueCell->NameSize * sizeof(WCHAR));
                  ValueBasicInformation->Name[ValueCell->NameSize / sizeof(WCHAR)] = 0;
                }
            }
          break;

        case KeyValuePartialInformation:
          *ResultLength = sizeof(KEY_VALUE_PARTIAL_INFORMATION)
            + (ValueCell->DataSize & LONG_MAX);
          if (Length < *ResultLength)
            {
              Status = STATUS_BUFFER_TOO_SMALL;
            }
          else
            {
              ValuePartialInformation = (PKEY_VALUE_PARTIAL_INFORMATION)
                KeyValueInformation;
              ValuePartialInformation->TitleIndex = 0;
              ValuePartialInformation->Type = ValueCell->DataType;
              ValuePartialInformation->DataLength = ValueCell->DataSize & LONG_MAX;
              if (ValueCell->DataSize > 0)
                {
                  DataCell = CmiGetBlock(RegistryHive, ValueCell->DataOffset, NULL);
                  RtlCopyMemory(ValuePartialInformation->Data,
                    DataCell->Data,
                    ValueCell->DataSize & LONG_MAX);
                  CmiReleaseBlock(RegistryHive, DataCell);
                }
              else
                {
                  RtlCopyMemory(ValuePartialInformation->Data,
                    &ValueCell->DataOffset,
                    ValueCell->DataSize & LONG_MAX);
                }
            }
          break;

        case KeyValueFullInformation:
          if (ValueCell->Flags & REG_VALUE_NAME_PACKED)
            {
              *ResultLength = sizeof(KEY_VALUE_FULL_INFORMATION) + 
                (ValueCell->NameSize + 1) * sizeof(WCHAR) +
                (ValueCell->DataSize & LONG_MAX);
            }
          else
            {
              *ResultLength = sizeof(KEY_VALUE_FULL_INFORMATION) + 
                ValueCell->NameSize + sizeof(WCHAR) +
                (ValueCell->DataSize & LONG_MAX);
            }
          if (Length < *ResultLength)
            {
              Status = STATUS_BUFFER_TOO_SMALL;
            }
          else
            {
              ValueFullInformation = (PKEY_VALUE_FULL_INFORMATION)
                KeyValueInformation;
              ValueFullInformation->TitleIndex = 0;
              ValueFullInformation->Type = ValueCell->DataType;
              if (ValueCell->Flags & REG_VALUE_NAME_PACKED)
                {
                  ValueFullInformation->NameLength =
                    (ValueCell->NameSize + 1) * sizeof(WCHAR);
                  CmiCopyPackedName(ValueFullInformation->Name,
                                    ValueCell->Name,
                                    ValueCell->NameSize);
                  ValueFullInformation->Name[ValueCell->NameSize] = 0;
                }
              else
                {
                  ValueFullInformation->NameLength =
                    ValueCell->NameSize + sizeof(WCHAR);
                  RtlCopyMemory(ValueFullInformation->Name,
                                ValueCell->Name,
                                ValueCell->NameSize);
                  ValueFullInformation->Name[ValueCell->NameSize / sizeof(WCHAR)] = 0;
                }
              ValueFullInformation->DataOffset = 
                (ULONG)ValueFullInformation->Name - (ULONG)ValueFullInformation +
                ValueFullInformation->NameLength;
              ValueFullInformation->DataOffset =
                (ValueFullInformation->DataOffset + 3) & 0xfffffffc;
              ValueFullInformation->DataLength = ValueCell->DataSize & LONG_MAX;
              if (ValueCell->DataSize > 0)
                {
                  DataCell = CmiGetBlock(RegistryHive, ValueCell->DataOffset, NULL);
                  RtlCopyMemory((PCHAR) ValueFullInformation
                                + ValueFullInformation->DataOffset,
                                DataCell->Data,
                                ValueCell->DataSize & LONG_MAX);
                  CmiReleaseBlock(RegistryHive, DataCell);
                }
              else
                {
                  RtlCopyMemory((PCHAR) ValueFullInformation
                                + ValueFullInformation->DataOffset,
                                &ValueCell->DataOffset,
                  ValueCell->DataSize & LONG_MAX);
                }
            }
          break;
        }
    }
  else
    {
      Status = STATUS_OBJECT_NAME_NOT_FOUND;
    }

  ExReleaseResourceLite(&KeyObject->RegistryHive->HiveResource);
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
  BLOCK_OFFSET VBOffset;
  PDATA_CELL DataCell;
  PDATA_CELL NewDataCell;
  PHBIN pBin;
  ULONG DesiredAccess;

  DPRINT("NtSetValueKey(KeyHandle %x  ValueName %S  Type %d)\n",
	 KeyHandle, ValueName? ValueName->Buffer : NULL, Type);

  DesiredAccess = KEY_SET_VALUE;
  if (Type == REG_LINK)
    DesiredAccess |= KEY_CREATE_LINK;

  /* Verify that the handle is valid and is a registry key */
  Status = ObReferenceObjectByHandle(KeyHandle,
				     DesiredAccess,
				     CmiKeyType,
				     UserMode,
				     (PVOID *)&KeyObject,
				     NULL);
  if (!NT_SUCCESS(Status))
    return(Status);

  /* Acquire hive lock exclucively */
  ExAcquireResourceExclusiveLite(&KeyObject->RegistryHive->HiveResource, TRUE);

  VERIFY_KEY_OBJECT(KeyObject);

  /* Get pointer to key cell */
  KeyCell = KeyObject->KeyCell;
  RegistryHive = KeyObject->RegistryHive;
  Status = CmiScanKeyForValue(RegistryHive,
			      KeyCell,
			      ValueName,
			      &ValueCell,
			      &VBOffset);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("Value not found. Status 0x%X\n", Status);

      ExReleaseResourceLite(&KeyObject->RegistryHive->HiveResource);
      ObDereferenceObject(KeyObject);
      return(Status);
    }

  if (ValueCell == NULL)
    {
      DPRINT("Allocate new value cell\n");
      Status = CmiAddValueToKey(RegistryHive,
				KeyCell,
				ValueName,
				&ValueCell,
				&VBOffset);
      if (NT_SUCCESS(Status))
	{
	  CmiMarkBlockDirty(RegistryHive, VBOffset);
	}
    }

  if (!NT_SUCCESS(Status))
    {
      DPRINT1("Cannot add value. Status 0x%X\n", Status);

      ExReleaseResourceLite(&KeyObject->RegistryHive->HiveResource);
      ObDereferenceObject(KeyObject);
      return(Status);
    }

  DPRINT("DataSize %lu\n", DataSize);
  DPRINT("ValueCell %p\n", ValueCell);
  DPRINT("ValueCell->DataSize %lu\n", ValueCell->DataSize);

  if (DataSize <= 4)
    {
      /* If datasize <= 4 then write in valueblock directly */
      DPRINT("ValueCell->DataSize %lu\n", ValueCell->DataSize);
      if ((ValueCell->DataSize >= 0) &&
	  (DataCell = CmiGetBlock(RegistryHive, ValueCell->DataOffset, NULL)))
	{
	  CmiDestroyBlock(RegistryHive, DataCell, ValueCell->DataOffset);
	}

      RtlCopyMemory(&ValueCell->DataOffset, Data, DataSize);
      ValueCell->DataSize = DataSize | 0x80000000;
      ValueCell->DataType = Type;
      RtlMoveMemory(&ValueCell->DataOffset, Data, DataSize);
      CmiMarkBlockDirty(RegistryHive, VBOffset);
    }
  else if (DataSize <= (ULONG) (ValueCell->DataSize & 0x7fffffff))
    {
      /* If new data size is <= current then overwrite current data */
      DataCell = CmiGetBlock(RegistryHive, ValueCell->DataOffset,&pBin);
      RtlZeroMemory(DataCell->Data, ValueCell->DataSize);
      RtlCopyMemory(DataCell->Data, Data, DataSize);
      ValueCell->DataSize = DataSize;
      ValueCell->DataType = Type;
      CmiReleaseBlock(RegistryHive, DataCell);

      /* Update time of heap */
      if (IsPermanentHive(RegistryHive))
	{
	  ZwQuerySystemTime((PTIME) &pBin->DateModified);
	}
      CmiMarkBlockDirty(RegistryHive, ValueCell->DataOffset);
    }
  else
    {
      /*
       * New data size is larger than the current, destroy current
       * data block and allocate a new one.
       */
      BLOCK_OFFSET NewOffset;

      DPRINT("ValueCell->DataSize %lu\n", ValueCell->DataSize);

      if ((ValueCell->DataSize >= 0) &&
	  (DataCell = CmiGetBlock(RegistryHive, ValueCell->DataOffset, NULL)))
	{
	  CmiDestroyBlock(RegistryHive, DataCell, ValueCell->DataOffset);
	  ValueCell->DataSize = 0;
	  ValueCell->DataType = 0;
	  ValueCell->DataOffset = 0xffffffff;
	}

      Status = CmiAllocateBlock(RegistryHive,
				(PVOID *)&NewDataCell,
				DataSize,
				&NewOffset);
      if (!NT_SUCCESS(Status))
	{
	  DPRINT("CmiAllocateBlock() failed (Status %lx)\n", Status);

	  ExReleaseResourceLite(&KeyObject->RegistryHive->HiveResource);
	  ObDereferenceObject(KeyObject);

	  return(Status);
	}

      RtlCopyMemory(&NewDataCell->Data[0], Data, DataSize);
      ValueCell->DataSize = DataSize;
      ValueCell->DataType = Type;
      CmiReleaseBlock(RegistryHive, NewDataCell);
      ValueCell->DataOffset = NewOffset;
      CmiMarkBlockDirty(RegistryHive, ValueCell->DataOffset);
    }

  /* Mark link key */
  if ((_wcsicmp(ValueName->Buffer, L"SymbolicLinkValue") == 0) &&
      (Type == REG_LINK))
    {
      KeyCell->Type = REG_LINK_KEY_CELL_TYPE;
      CmiMarkBlockDirty(RegistryHive, KeyObject->BlockOffset);
    }

  /* Update time of heap */
  if (IsPermanentHive(RegistryHive) && CmiGetBlock(RegistryHive, VBOffset, &pBin))
    {
      ZwQuerySystemTime((PTIME) &pBin->DateModified);
    }

  ExReleaseResourceLite(&KeyObject->RegistryHive->HiveResource);
  ObDereferenceObject(KeyObject);

  CmiSyncHives();

  DPRINT("Return Status 0x%X\n", Status);

  return(Status);
}


NTSTATUS STDCALL
NtDeleteValueKey(IN HANDLE KeyHandle,
		 IN PUNICODE_STRING ValueName)
{
  PKEY_OBJECT KeyObject;
  NTSTATUS Status;

  /* Verify that the handle is valid and is a registry key */
  Status = ObReferenceObjectByHandle(KeyHandle,
		KEY_QUERY_VALUE,
		CmiKeyType,
		UserMode,
		(PVOID *)&KeyObject,
		NULL);
  if (!NT_SUCCESS(Status))
    {
      return Status;
    }

  /* Acquire hive lock */
  ExAcquireResourceExclusiveLite(&KeyObject->RegistryHive->HiveResource, TRUE);

  VERIFY_KEY_OBJECT(KeyObject);

  Status = CmiDeleteValueFromKey(KeyObject->RegistryHive,
				 KeyObject->KeyCell,
				 KeyObject->BlockOffset,
				 ValueName);

  /* Release hive lock */
  ExReleaseResourceLite(&KeyObject->RegistryHive->HiveResource);

  ObDereferenceObject(KeyObject);

  CmiSyncHives();

  return Status;
}


NTSTATUS STDCALL
NtLoadKey(PHANDLE KeyHandle,
	  POBJECT_ATTRIBUTES ObjectAttributes)
{
  return NtLoadKey2(KeyHandle, ObjectAttributes, 0);
}


NTSTATUS STDCALL
NtLoadKey2(IN PHANDLE KeyHandle,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	IN ULONG Flags)
{
	UNIMPLEMENTED;
}


NTSTATUS STDCALL
NtNotifyChangeKey(
	IN	HANDLE  KeyHandle,
	IN	HANDLE  Event,
	IN	PIO_APC_ROUTINE	 ApcRoutine  OPTIONAL,
	IN	PVOID  ApcContext  OPTIONAL,
	OUT	PIO_STATUS_BLOCK  IoStatusBlock,
	IN	ULONG  CompletionFilter,
	IN	BOOLEAN  Asynchroneous,
	OUT	PVOID  ChangeBuffer,
	IN	ULONG  Length,
	IN	BOOLEAN  WatchSubtree)
{
	UNIMPLEMENTED;
}


NTSTATUS STDCALL
NtQueryMultipleValueKey(IN HANDLE KeyHandle,
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

  /* Verify that the handle is valid and is a registry key */
  Status = ObReferenceObjectByHandle(KeyHandle,
				     KEY_QUERY_VALUE,
				     CmiKeyType,
				     UserMode,
				     (PVOID *) &KeyObject,
				     NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("ObReferenceObjectByHandle() failed with status %x\n", Status);
      return(Status);
    }

  /* Acquire hive lock */
  ExAcquireResourceSharedLite(&KeyObject->RegistryHive->HiveResource, TRUE);

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

      BufferLength = (BufferLength + 3) & 0xfffffffc;

      if (BufferLength + (ValueCell->DataSize & LONG_MAX) <= *Length)
	{
	  DataPtr = (PUCHAR)(((ULONG)DataPtr + 3) & 0xfffffffc);

	  ValueList[i].Type = ValueCell->DataType;
	  ValueList[i].DataLength = ValueCell->DataSize & LONG_MAX;
	  ValueList[i].DataOffset = (ULONG) DataPtr - (ULONG) Buffer;

	  if (ValueCell->DataSize > 0)
	    {
	      DataCell = CmiGetBlock(RegistryHive, ValueCell->DataOffset, NULL);
	      RtlCopyMemory(DataPtr, DataCell->Data, ValueCell->DataSize & LONG_MAX);
	      CmiReleaseBlock(RegistryHive, DataCell);
	    }
	  else
	    {
	      RtlCopyMemory(DataPtr,
			    &ValueCell->DataOffset,
			    ValueCell->DataSize & LONG_MAX);
	    }

	  DataPtr += ValueCell->DataSize & LONG_MAX;
	}
      else
	{
	  Status = STATUS_BUFFER_TOO_SMALL;
	}

      BufferLength +=  ValueCell->DataSize & LONG_MAX;
    }

  if (NT_SUCCESS(Status))
    *Length = BufferLength;

  *ReturnLength = BufferLength;

  /* Release hive lock */
  ExReleaseResourceLite(&KeyObject->RegistryHive->HiveResource);

  ObDereferenceObject(KeyObject);

  DPRINT("Return Status 0x%X\n", Status);

  return(Status);
}


NTSTATUS STDCALL
NtReplaceKey(
	IN	POBJECT_ATTRIBUTES	ObjectAttributes,
	IN	HANDLE  Key,
	IN	POBJECT_ATTRIBUTES	ReplacedObjectAttributes
	)
{
	UNIMPLEMENTED;
}


NTSTATUS STDCALL
NtRestoreKey(
	IN	HANDLE	KeyHandle,
	IN	HANDLE	FileHandle,
	IN	ULONG	RestoreFlags
	)
{
	UNIMPLEMENTED;
}


NTSTATUS STDCALL
NtSaveKey(
	IN	HANDLE	KeyHandle,
	IN	HANDLE	FileHandle)
{
	UNIMPLEMENTED;
}


NTSTATUS STDCALL
NtSetInformationKey(
	IN	HANDLE	KeyHandle,
	IN	CINT	KeyInformationClass,
	IN	PVOID	 KeyInformation,
	IN	ULONG	 KeyInformationLength)
{
  UNIMPLEMENTED;
}


NTSTATUS STDCALL
NtUnloadKey(IN HANDLE KeyHandle)
{
  UNIMPLEMENTED;
}


NTSTATUS STDCALL
NtInitializeRegistry(IN BOOLEAN SetUpBoot)
{
  NTSTATUS Status = STATUS_ACCESS_DENIED;

  if (CmiRegistryInitialized == FALSE)
    {
      /* FIXME: save boot log file */

      Status = CmiInitHives(SetUpBoot);

      CmiRegistryInitialized = TRUE;
    }

  return(Status);
}

/* EOF */
