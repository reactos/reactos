/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             ntoskrnl/cm/ntfunc.c
 * PURPOSE:          Ntxxx function for registry access
 * UPDATE HISTORY:
*/

#include <ddk/ntddk.h>
#include <internal/config.h>
#include <internal/ob.h>
#include <limits.h>
#include <string.h>
#include <internal/pool.h>
#include <internal/registry.h>

#define NDEBUG
#include <internal/debug.h>

#include "cm.h"

extern POBJECT_TYPE  CmiKeyType;
extern PREGISTRY_FILE  CmiVolatileFile;


NTSTATUS STDCALL
NtCreateKey(OUT PHANDLE KeyHandle,
	    IN ACCESS_MASK DesiredAccess,
	    IN POBJECT_ATTRIBUTES ObjectAttributes,
	    IN ULONG TitleIndex,
	    IN PUNICODE_STRING Class,
	    IN ULONG CreateOptions,
	    OUT PULONG Disposition)
{
 NTSTATUS	Status;
 PVOID		Object;
 PKEY_OBJECT key;
 UNICODE_STRING RemainingPath;
 PWSTR end;
// KIRQL  OldIrql;

//  DPRINT("NtCreateKey (Name %wZ),KeyHandle=%x,Root=%x\n",
//         ObjectAttributes->ObjectName,KeyHandle
//	,ObjectAttributes->RootDirectory);

  /*FIXME: check for standard handle prefix and adjust objectAttributes accordingly */

  Status = ObFindObject(ObjectAttributes,&Object,&RemainingPath,CmiKeyType);
  if (!NT_SUCCESS(Status))
  {
	return Status;
  }
DPRINT("RP=%wZ\n",&RemainingPath);
  if ((RemainingPath.Buffer == NULL) || (RemainingPath.Buffer[0] ==0))
  {
    /*  Fail if the key has been deleted  */
    if (((PKEY_OBJECT)Object)->Flags & KO_MARKED_FOR_DELETE)
    {
      ObDereferenceObject(Object);
      return STATUS_UNSUCCESSFUL;
    }
    if (Disposition)
      *Disposition = REG_OPENED_EXISTING_KEY;
    Status = ObCreateHandle(PsGetCurrentProcess(),
			    Object,
			    DesiredAccess,
			    FALSE,
			    KeyHandle);
DPRINT("Status=%x\n",Status);
    ObDereferenceObject(Object);
    return Status;
  }
  /* if RemainingPath contains \ : must return error */
  if((RemainingPath.Buffer[0])=='\\')
    end = wcschr((RemainingPath.Buffer)+1, '\\');
  else
    end = wcschr((RemainingPath.Buffer), '\\');
  if (end != NULL)
  {
    ObDereferenceObject(Object);
    return STATUS_UNSUCCESSFUL;
  }
  /*   because NtCreateKey don't create tree */

DPRINT("NCK %S parent=%x\n",RemainingPath.Buffer,Object);
  Status = ObCreateObject(KeyHandle,
			  DesiredAccess,
			  NULL,
			  CmiKeyType,
			  (PVOID*)&key);

  if (!NT_SUCCESS(Status))
	return(Status);
  key->ParentKey = Object;
//    if ( (key->ParentKey ==NULL))
//      key->ParentKey = ObjectAttributes->RootDirectory;
  if (CreateOptions & REG_OPTION_VOLATILE)
    key->RegistryFile=CmiVolatileFile;
  else
    key->RegistryFile=key->ParentKey->RegistryFile;
  key->Flags = 0;
  key->NumberOfSubKeys = 0;
  key->SizeOfSubKeys = 0;
  key->SubKeys = NULL;
//  KeAcquireSpinLock(&key->RegistryFile->RegLock, &OldIrql);
  /* add key to subkeys of parent if needed */
  Status = CmiAddSubKey(key->RegistryFile,
                      key->ParentKey,
                      key,
                      RemainingPath.Buffer,
                      RemainingPath.Length,
                      TitleIndex,
                      Class,
                      CreateOptions);
  key->Name = key->KeyBlock->Name;
  key->NameSize = key->KeyBlock->NameSize;
  if (key->RegistryFile == key->ParentKey->RegistryFile)
  {
    key->KeyBlock->ParentKeyOffset = key->ParentKey->BlockOffset;
    key->KeyBlock->SecurityKeyOffset = key->ParentKey->KeyBlock->SecurityKeyOffset;
  }
  else
  {
    key->KeyBlock->ParentKeyOffset = -1;
    key->KeyBlock->SecurityKeyOffset = -1;
    /* this key must rest in memory unless it is deleted */
    /* , or file is unloaded*/
    ObReferenceObjectByPointer(key,
			      STANDARD_RIGHTS_REQUIRED,
			      NULL,
			      UserMode);
  }
  CmiAddKeyToList(key->ParentKey,key);
//  KeReleaseSpinLock(&key->RegistryFile->RegLock, OldIrql);
  ObDereferenceObject(key);
  ObDereferenceObject(Object);
  if (Disposition) *Disposition = REG_CREATED_NEW_KEY;

  if (!NT_SUCCESS(Status))
  {
  	return Status;
  }

  return STATUS_SUCCESS;
}


NTSTATUS STDCALL
NtDeleteKey(IN HANDLE KeyHandle)
{
  NTSTATUS  Status;
  PKEY_OBJECT  KeyObject;
  
  /*  Verify that the handle is valid and is a registry key  */
  Status = ObReferenceObjectByHandle(KeyHandle,
                                     KEY_WRITE,
                                     CmiKeyType,
                                     UserMode,
                                     (PVOID *)&KeyObject,
                                     NULL);
  if (!NT_SUCCESS(Status))
    {
      return  Status;
    }
  
  /*  Set the marked for delete bit in the key object  */
  KeyObject->Flags |= KO_MARKED_FOR_DELETE;
  
  /*  Dereference the object  */
  ObDereferenceObject(KeyObject);
  if(KeyObject->RegistryFile != KeyObject->ParentKey->RegistryFile)
    ObDereferenceObject(KeyObject);
  /* close the handle */
  ObDeleteHandle(PsGetCurrentProcess(),KeyHandle);
  /* FIXME: I think that ObDeleteHandle should dereference the object  */
  ObDereferenceObject(KeyObject);
  

  return  STATUS_SUCCESS;
}


NTSTATUS 
STDCALL
NtEnumerateKey (
	IN	HANDLE			KeyHandle,
	IN	ULONG			Index,
	IN	KEY_INFORMATION_CLASS	KeyInformationClass,
	OUT	PVOID			KeyInformation,
	IN	ULONG			Length,
	OUT	PULONG			ResultLength
	)
{
  NTSTATUS  Status;
  PKEY_OBJECT  KeyObject;
  PREGISTRY_FILE  RegistryFile;
  PKEY_BLOCK  KeyBlock, SubKeyBlock;
  PHASH_TABLE_BLOCK  HashTableBlock;
  PKEY_BASIC_INFORMATION  BasicInformation;
  PKEY_NODE_INFORMATION  NodeInformation;
  PKEY_FULL_INFORMATION  FullInformation;
  PDATA_BLOCK pClassData;
    
  /*  Verify that the handle is valid and is a registry key  */
  Status = ObReferenceObjectByHandle(KeyHandle,
                                     KEY_ENUMERATE_SUB_KEYS,
                                     CmiKeyType,
                                     UserMode,
                                     (PVOID *)&KeyObject,
                                     NULL);
  if (!NT_SUCCESS(Status))
    {
      return  Status;
    }

  /*  Get pointer to KeyBlock  */
  KeyBlock = KeyObject->KeyBlock;
  RegistryFile = KeyObject->RegistryFile;
    
  /*  Get pointer to SubKey  */
  if(Index >= KeyBlock->NumberOfSubKeys)
  {
    if (RegistryFile == CmiVolatileFile)
    {
      ObDereferenceObject (KeyObject);
      return  STATUS_NO_MORE_ENTRIES;
    }
    else
    {
     int i;
     PKEY_OBJECT CurKey=NULL;
      /* search volatile keys */
      for (i=0; i < KeyObject->NumberOfSubKeys; i++)
      {
        CurKey=KeyObject->SubKeys[i];
        if (CurKey->RegistryFile == CmiVolatileFile)
	{
	  if ( Index-- == KeyObject->NumberOfSubKeys) break;
	}
      }
      if(Index >= KeyBlock->NumberOfSubKeys)
      {
        ObDereferenceObject (KeyObject);
        return  STATUS_NO_MORE_ENTRIES;
      }
      SubKeyBlock = CurKey->KeyBlock;
    }
  }
  else
  {
    HashTableBlock = CmiGetBlock(RegistryFile, KeyBlock->HashTableOffset,NULL);
    SubKeyBlock = CmiGetKeyFromHashByIndex(RegistryFile, 
                                         HashTableBlock, 
                                         Index);
  }
  if (SubKeyBlock == NULL)
  {
    ObDereferenceObject (KeyObject);
    return  STATUS_NO_MORE_ENTRIES;
  }

  Status = STATUS_SUCCESS;
  switch (KeyInformationClass)
    {
    case KeyBasicInformation:
      /*  Check size of buffer  */
      if (Length < sizeof(KEY_BASIC_INFORMATION) + 
          (SubKeyBlock->NameSize ) * sizeof(WCHAR))
        {
          Status = STATUS_BUFFER_OVERFLOW;
        }
      else
        {
          /*  Fill buffer with requested info  */
          BasicInformation = (PKEY_BASIC_INFORMATION) KeyInformation;
          BasicInformation->LastWriteTime.u.LowPart = SubKeyBlock->LastWriteTime.dwLowDateTime;
          BasicInformation->LastWriteTime.u.HighPart = SubKeyBlock->LastWriteTime.dwHighDateTime;
          BasicInformation->TitleIndex = Index;
          BasicInformation->NameLength = (SubKeyBlock->NameSize ) * sizeof(WCHAR);
          mbstowcs(BasicInformation->Name, 
                  SubKeyBlock->Name, 
                  SubKeyBlock->NameSize*2);
//          BasicInformation->Name[SubKeyBlock->NameSize] = 0;
          *ResultLength = sizeof(KEY_BASIC_INFORMATION) + 
            SubKeyBlock->NameSize * sizeof(WCHAR);
        }
      break;
      
    case KeyNodeInformation:
      /*  Check size of buffer  */
      if (Length < sizeof(KEY_NODE_INFORMATION) +
          (SubKeyBlock->NameSize ) * sizeof(WCHAR) +
          (SubKeyBlock->ClassSize ))
        {
          Status = STATUS_BUFFER_OVERFLOW;
        }
      else
        {
          /*  Fill buffer with requested info  */
          NodeInformation = (PKEY_NODE_INFORMATION) KeyInformation;
          NodeInformation->LastWriteTime.u.LowPart = SubKeyBlock->LastWriteTime.dwLowDateTime;
          NodeInformation->LastWriteTime.u.HighPart = SubKeyBlock->LastWriteTime.dwHighDateTime;
          NodeInformation->TitleIndex = Index;
          NodeInformation->ClassOffset = sizeof(KEY_NODE_INFORMATION) + 
            SubKeyBlock->NameSize * sizeof(WCHAR);
          NodeInformation->ClassLength = SubKeyBlock->ClassSize;
          NodeInformation->NameLength = (SubKeyBlock->NameSize ) * sizeof(WCHAR);
          mbstowcs(NodeInformation->Name, 
                  SubKeyBlock->Name, 
                  SubKeyBlock->NameSize*2);
//          NodeInformation->Name[SubKeyBlock->NameSize] = 0;
          if (SubKeyBlock->ClassSize != 0)
            {
              pClassData=CmiGetBlock(KeyObject->RegistryFile
                                     ,SubKeyBlock->ClassNameOffset,NULL);
              wcsncpy(NodeInformation->Name + SubKeyBlock->NameSize ,
                      (PWCHAR)pClassData->Data,
                      SubKeyBlock->ClassSize);
              CmiReleaseBlock(RegistryFile, pClassData);
            }
          *ResultLength = sizeof(KEY_NODE_INFORMATION) +
            (SubKeyBlock->NameSize) * sizeof(WCHAR) +
            (SubKeyBlock->ClassSize );
        }
      break;
      
    case KeyFullInformation:
      /*  check size of buffer  */
      if (Length < sizeof(KEY_FULL_INFORMATION) +
          SubKeyBlock->ClassSize)
        {
          Status = STATUS_BUFFER_OVERFLOW;
        }
      else
        {
          /* fill buffer with requested info  */
          FullInformation = (PKEY_FULL_INFORMATION) KeyInformation;
          FullInformation->LastWriteTime.u.LowPart = SubKeyBlock->LastWriteTime.dwLowDateTime;
          FullInformation->LastWriteTime.u.HighPart = SubKeyBlock->LastWriteTime.dwHighDateTime;
          FullInformation->TitleIndex = Index;
          FullInformation->ClassOffset = sizeof(KEY_FULL_INFORMATION) - 
            sizeof(WCHAR);
          FullInformation->ClassLength = SubKeyBlock->ClassSize;
          FullInformation->SubKeys = SubKeyBlock->NumberOfSubKeys;
          FullInformation->MaxNameLen = 
            CmiGetMaxNameLength(RegistryFile, SubKeyBlock);
          FullInformation->MaxClassLen = 
            CmiGetMaxClassLength(RegistryFile, SubKeyBlock);
          FullInformation->Values = SubKeyBlock->NumberOfValues;
          FullInformation->MaxValueNameLen = 
            CmiGetMaxValueNameLength(RegistryFile, SubKeyBlock);
          FullInformation->MaxValueDataLen = 
            CmiGetMaxValueDataLength(RegistryFile, SubKeyBlock);
          if (SubKeyBlock->ClassSize != 0)
            {
              pClassData=CmiGetBlock(KeyObject->RegistryFile
                                     ,SubKeyBlock->ClassNameOffset,NULL);
              wcsncpy(FullInformation->Class,
                      (PWCHAR)pClassData->Data,
                      SubKeyBlock->ClassSize);
              CmiReleaseBlock(RegistryFile, pClassData);
            }
          *ResultLength = sizeof(KEY_FULL_INFORMATION) +
            SubKeyBlock->ClassSize ;
        }
      break;
    }
  CmiReleaseBlock(RegistryFile, SubKeyBlock);
  ObDereferenceObject (KeyObject);

  return  Status;
}


NTSTATUS 
STDCALL
NtEnumerateValueKey (
	IN	HANDLE				KeyHandle,
	IN	ULONG				Index,
	IN	KEY_VALUE_INFORMATION_CLASS	KeyValueInformationClass,
	OUT	PVOID				KeyValueInformation,
	IN	ULONG				Length,
	OUT	PULONG				ResultLength
	)
{
 NTSTATUS  Status;
 PKEY_OBJECT  KeyObject;
 PREGISTRY_FILE  RegistryFile;
 PKEY_BLOCK  KeyBlock;
 PVALUE_BLOCK  ValueBlock;
 PDATA_BLOCK  DataBlock;
 PKEY_VALUE_BASIC_INFORMATION  ValueBasicInformation;
 PKEY_VALUE_PARTIAL_INFORMATION  ValuePartialInformation;
 PKEY_VALUE_FULL_INFORMATION  ValueFullInformation;


  /*  Verify that the handle is valid and is a registry key  */
  Status = ObReferenceObjectByHandle(KeyHandle,
                                     KEY_QUERY_VALUE,
                                     CmiKeyType,
                                     UserMode,
                                     (PVOID *)&KeyObject,
                                     NULL);
  if (!NT_SUCCESS(Status))
    {
      return  Status;
    }

  /*  Get pointer to KeyBlock  */
  KeyBlock = KeyObject->KeyBlock;
  RegistryFile = KeyObject->RegistryFile;
    
  /*  Get Value block of interest  */
  Status = CmiGetValueFromKeyByIndex(RegistryFile,
                                     KeyBlock,
                                     Index,
                                     &ValueBlock);
  if (!NT_SUCCESS(Status))
    {
      ObDereferenceObject(KeyObject);
      return  Status;
    }
  else if (ValueBlock != NULL)
    {
      switch (KeyValueInformationClass)
        {
        case KeyValueBasicInformation:
          *ResultLength = sizeof(KEY_VALUE_BASIC_INFORMATION) + 
            (ValueBlock->NameSize + 1) * sizeof(WCHAR);
          if (Length < *ResultLength)
            {
              Status = STATUS_BUFFER_OVERFLOW;
            }
          else
            {
              ValueBasicInformation = (PKEY_VALUE_BASIC_INFORMATION) 
                KeyValueInformation;
              ValueBasicInformation->TitleIndex = 0;
              ValueBasicInformation->Type = ValueBlock->DataType;
              ValueBasicInformation->NameLength =
                (ValueBlock->NameSize + 1) * sizeof(WCHAR);
              mbstowcs(ValueBasicInformation->Name, ValueBlock->Name
			,ValueBlock->NameSize*2);
              ValueBasicInformation->Name[ValueBlock->NameSize]=0;
            }
          break;

        case KeyValuePartialInformation:
          *ResultLength = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 
            (ValueBlock->DataSize & LONG_MAX);
          if (Length < *ResultLength)
            {
              Status = STATUS_BUFFER_OVERFLOW;
            }
          else
            {
              ValuePartialInformation = (PKEY_VALUE_PARTIAL_INFORMATION)
                KeyValueInformation;
              ValuePartialInformation->TitleIndex = 0;
              ValuePartialInformation->Type = ValueBlock->DataType;
              ValuePartialInformation->DataLength = ValueBlock->DataSize & LONG_MAX;
              if(ValueBlock->DataSize >0)
              {
                DataBlock = CmiGetBlock(RegistryFile, ValueBlock->DataOffset,NULL);
                RtlCopyMemory(ValuePartialInformation->Data, 
                            DataBlock->Data,
                            ValueBlock->DataSize & LONG_MAX);
                CmiReleaseBlock(RegistryFile, DataBlock);
              }
              else
              {
                RtlCopyMemory(ValuePartialInformation->Data, 
                            &ValueBlock->DataOffset, 
                            ValueBlock->DataSize & LONG_MAX);
              }
              DataBlock = CmiGetBlock(RegistryFile, ValueBlock->DataOffset,NULL);
            }
          break;

        case KeyValueFullInformation:
          *ResultLength = sizeof(KEY_VALUE_FULL_INFORMATION) + 
            (ValueBlock->NameSize ) * sizeof(WCHAR) + (ValueBlock->DataSize & LONG_MAX);
          if (Length < *ResultLength)
            {
              Status = STATUS_BUFFER_OVERFLOW;
            }
          else
            {
              ValueFullInformation = (PKEY_VALUE_FULL_INFORMATION) 
                KeyValueInformation;
              ValueFullInformation->TitleIndex = 0;
              ValueFullInformation->Type = ValueBlock->DataType;
              ValueFullInformation->DataOffset = 
                (DWORD)ValueFullInformation->Name - (DWORD)ValueFullInformation
                + (ValueBlock->NameSize ) * sizeof(WCHAR);
              ValueFullInformation->DataOffset =
		(ValueFullInformation->DataOffset +3) &0xfffffffc;
              ValueFullInformation->DataLength = ValueBlock->DataSize & LONG_MAX;
              ValueFullInformation->NameLength =
                (ValueBlock->NameSize ) * sizeof(WCHAR);
              mbstowcs(ValueFullInformation->Name, ValueBlock->Name
			,ValueBlock->NameSize*2);
              if(ValueBlock->DataSize >0)
              {
                DataBlock = CmiGetBlock(RegistryFile, ValueBlock->DataOffset,NULL);
                RtlCopyMemory((char *)(ValueFullInformation)
		              + ValueFullInformation->DataOffset,
                            DataBlock->Data,
                            ValueBlock->DataSize & LONG_MAX);
                CmiReleaseBlock(RegistryFile, DataBlock);
              }
              else
              {
                RtlCopyMemory((char *)(ValueFullInformation)
		              + ValueFullInformation->DataOffset,
                            &ValueBlock->DataOffset, 
                            ValueBlock->DataSize & LONG_MAX);
              }
            }
          break;
        }
    }
  else
    {
      Status = STATUS_UNSUCCESSFUL;
    }
  ObDereferenceObject(KeyObject);

  return  Status;
}


NTSTATUS STDCALL
NtFlushKey(IN HANDLE KeyHandle)
{
 NTSTATUS Status;
 PKEY_OBJECT  KeyObject;
 PREGISTRY_FILE  RegistryFile;
 WCHAR LogName[MAX_PATH];
 HANDLE FileHandle;
// HANDLE FileHandleLog;
 OBJECT_ATTRIBUTES ObjectAttributes;
// KIRQL  OldIrql;
 UNICODE_STRING TmpFileName;
 int i;
 LARGE_INTEGER fileOffset;
 DWORD * pEntDword;
  /*  Verify that the handle is valid and is a registry key  */
  Status = ObReferenceObjectByHandle(KeyHandle,
                                     KEY_QUERY_VALUE,
                                     CmiKeyType,
                                     UserMode,
                                     (PVOID *)&KeyObject,
                                     NULL);
  if (!NT_SUCCESS(Status))
    {
      return  Status;
    }
  RegistryFile = KeyObject->RegistryFile;
//  KeAcquireSpinLock(&RegistryFile->RegLock, &OldIrql);
  /* then write changed blocks in .log */
  wcscpy(LogName,RegistryFile->Filename );
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
                      NULL, 0, FILE_ATTRIBUTE_NORMAL,
		0, FILE_SUPERSEDE, 0, NULL, 0);
  Status = ZwWriteFile(FileHandleLog, 
                      0, 0, 0, 0, 
                      RegistryFile->HeaderBlock, 
                      sizeof(HEADER_BLOCK), 
                      0, 0);
  for (i=0; i < RegistryFile->BlockListSize ; i++)
  {
    if( RegistryFile->BlockList[i]->DateModified.dwHighDateTime
	    > RegistryFile->HeaderBlock->DateModified.dwHighDateTime
       ||(  RegistryFile->BlockList[i]->DateModified.dwHighDateTime
	     == RegistryFile->HeaderBlock->DateModified.dwHighDateTime
          && RegistryFile->BlockList[i]->DateModified.dwLowDateTime
	      > RegistryFile->HeaderBlock->DateModified.dwLowDateTime)
       )
      Status = ZwWriteFile(FileHandleLog, 
                      0, 0, 0, 0, 
                      RegistryFile->BlockList[i],
                      RegistryFile->BlockList[i]->BlockSize ,
                      0, 0);
  }
  ZwClose(FileHandleLog);
END FIXME*/
  /* update header of registryfile with Version >VersionOld */
  /* this allow recover if system crash while updating hove file */
  RtlInitUnicodeString (&TmpFileName, RegistryFile->Filename);
  InitializeObjectAttributes(&ObjectAttributes,
                             &TmpFileName,
                             0,
                             NULL,
                             NULL);
  Status = NtOpenFile(&FileHandle,
                      FILE_ALL_ACCESS,
                      &ObjectAttributes,
                      NULL, 0, 0);
  RegistryFile->HeaderBlock->Version++;

  Status = ZwWriteFile(FileHandle, 
                      0, 0, 0, 0, 
                      RegistryFile->HeaderBlock, 
                      sizeof(HEADER_BLOCK), 
                      0, 0);

  /* update changed blocks in file */
  fileOffset.u.HighPart = 0;
  for (i=0; i < RegistryFile->BlockListSize ; i++)
  {
    if( RegistryFile->BlockList[i]->DateModified.dwHighDateTime
	    > RegistryFile->HeaderBlock->DateModified.dwHighDateTime
       ||(  RegistryFile->BlockList[i]->DateModified.dwHighDateTime
	     == RegistryFile->HeaderBlock->DateModified.dwHighDateTime
          && RegistryFile->BlockList[i]->DateModified.dwLowDateTime
	      > RegistryFile->HeaderBlock->DateModified.dwLowDateTime)
       )
    {
      fileOffset.u.LowPart = RegistryFile->BlockList[i]->BlockOffset+4096;
      Status = NtWriteFile(FileHandle, 
                      0, 0, 0, 0, 
                      RegistryFile->BlockList[i],
                      RegistryFile->BlockList[i]->BlockSize ,
                      &fileOffset, 0);
    }
  }
  /* change version in header */
  RegistryFile->HeaderBlock->VersionOld = RegistryFile->HeaderBlock->Version;
  ZwQuerySystemTime((PTIME) &RegistryFile->HeaderBlock->DateModified);
  /* calculate checksum */
  RegistryFile->HeaderBlock->Checksum = 0;
  pEntDword = (DWORD *) RegistryFile->HeaderBlock;
  for (i=0 ; i <127 ; i++)
  {
    RegistryFile->HeaderBlock->Checksum ^= pEntDword[i];
  }
  /* write new header */
  fileOffset.u.LowPart = 0;
  Status = ZwWriteFile(FileHandle, 
                      0, 0, 0, 0, 
                      RegistryFile->HeaderBlock, 
                      sizeof(HEADER_BLOCK), 
                      &fileOffset, 0);
  ZwClose(FileHandle);
//  KeReleaseSpinLock(&RegistryFile->RegLock, OldIrql);
  return  STATUS_SUCCESS;
}


NTSTATUS STDCALL
NtOpenKey(OUT PHANDLE KeyHandle,
	  IN ACCESS_MASK DesiredAccess,
	  IN POBJECT_ATTRIBUTES ObjectAttributes)
{
 NTSTATUS	Status;
 PVOID		Object;
 UNICODE_STRING RemainingPath;

   RemainingPath.Buffer=NULL;
   Status = ObFindObject(ObjectAttributes,&Object,&RemainingPath,CmiKeyType);
DPRINT("NTOpenKey : after ObFindObject\n");
DPRINT("RB.B=%x\n",RemainingPath.Buffer);
   if(RemainingPath.Buffer != 0 && RemainingPath.Buffer[0] !=0)
   {
DPRINT("NTOpenKey3 : after ObFindObject\n");
     ObDereferenceObject(Object);
DPRINT("RP=%wZ\n",&RemainingPath);
     return STATUS_UNSUCCESSFUL;
   }
DPRINT("NTOpenKey2 : after ObFindObject\n");
  /*  Fail if the key has been deleted  */
  if (((PKEY_OBJECT)Object)->Flags & KO_MARKED_FOR_DELETE)
  {
     ObDereferenceObject(Object);
    return STATUS_UNSUCCESSFUL;
  }
      
  Status = ObCreateHandle(
			PsGetCurrentProcess(),
			Object,
			DesiredAccess,
			FALSE,
			KeyHandle
			);
  ObDereferenceObject(Object);
  if (!NT_SUCCESS(Status))
  {
    return Status;
  }

  return STATUS_SUCCESS;
}


NTSTATUS 
STDCALL
NtQueryKey (
	IN	HANDLE			KeyHandle, 
	IN	KEY_INFORMATION_CLASS	KeyInformationClass,
	OUT	PVOID			KeyInformation,
	IN	ULONG			Length,
	OUT	PULONG			ResultLength
	)
{
  NTSTATUS  Status;
  PKEY_OBJECT  KeyObject;
  PREGISTRY_FILE  RegistryFile;
  PKEY_BLOCK  KeyBlock;
  PKEY_BASIC_INFORMATION  BasicInformation;
  PKEY_NODE_INFORMATION  NodeInformation;
  PKEY_FULL_INFORMATION  FullInformation;
  PDATA_BLOCK pClassData;
    
  /*  Verify that the handle is valid and is a registry key  */
  Status = ObReferenceObjectByHandle(KeyHandle,
                                     KEY_READ,
                                     CmiKeyType,
                                     UserMode,
                                     (PVOID *)&KeyObject,
                                     NULL);
  if (!NT_SUCCESS(Status))
    {
      return  Status;
    }

  /*  Get pointer to KeyBlock  */
  KeyBlock = KeyObject->KeyBlock;
  RegistryFile = KeyObject->RegistryFile;
    
  Status = STATUS_SUCCESS;
  switch (KeyInformationClass)
    {
    case KeyBasicInformation:
      /*  Check size of buffer  */
      if (Length < sizeof(KEY_BASIC_INFORMATION) + 
          KeyObject->NameSize * sizeof(WCHAR))
        {
          Status = STATUS_BUFFER_OVERFLOW;
        }
      else
        {
          /*  Fill buffer with requested info  */
          BasicInformation = (PKEY_BASIC_INFORMATION) KeyInformation;
          BasicInformation->LastWriteTime.u.LowPart = KeyBlock->LastWriteTime.dwLowDateTime;
          BasicInformation->LastWriteTime.u.HighPart = KeyBlock->LastWriteTime.dwHighDateTime;
          BasicInformation->TitleIndex = 0;
          BasicInformation->NameLength = 
                (KeyObject->NameSize ) * sizeof(WCHAR);
          mbstowcs(BasicInformation->Name, 
                  KeyObject->Name, 
                  KeyObject->NameSize*sizeof(WCHAR));
          *ResultLength = sizeof(KEY_BASIC_INFORMATION) + 
            KeyObject->NameSize * sizeof(WCHAR);
        }
      break;
      
    case KeyNodeInformation:
      /*  Check size of buffer  */
      if (Length < sizeof(KEY_NODE_INFORMATION) +
          (KeyObject->NameSize ) * sizeof(WCHAR) +
          KeyBlock->ClassSize )
        {
          Status = STATUS_BUFFER_OVERFLOW;
        }
      else
        {
          /*  Fill buffer with requested info  */
          NodeInformation = (PKEY_NODE_INFORMATION) KeyInformation;
          NodeInformation->LastWriteTime.u.LowPart = KeyBlock->LastWriteTime.dwLowDateTime;
          NodeInformation->LastWriteTime.u.HighPart = KeyBlock->LastWriteTime.dwHighDateTime;
          NodeInformation->TitleIndex = 0;
          NodeInformation->ClassOffset = sizeof(KEY_NODE_INFORMATION) + 
            KeyObject->NameSize * sizeof(WCHAR);
          NodeInformation->ClassLength = KeyBlock->ClassSize;
          NodeInformation->NameLength = 
                (KeyObject->NameSize ) * sizeof(WCHAR);
          mbstowcs(NodeInformation->Name, 
                  KeyObject->Name, 
                  KeyObject->NameSize*2);
          if (KeyBlock->ClassSize != 0)
            {
              pClassData=CmiGetBlock(KeyObject->RegistryFile
                                     ,KeyBlock->ClassNameOffset,NULL);
              wcsncpy(NodeInformation->Name + (KeyObject->NameSize )*sizeof(WCHAR),
                      (PWCHAR)pClassData->Data,
                      KeyBlock->ClassSize);
              CmiReleaseBlock(RegistryFile, pClassData);
            }
          *ResultLength = sizeof(KEY_NODE_INFORMATION) +
            (KeyObject->NameSize ) * sizeof(WCHAR) +
            KeyBlock->ClassSize;
        }
      break;
      
    case KeyFullInformation:
      /*  Check size of buffer  */
      if (Length < sizeof(KEY_FULL_INFORMATION) +
          KeyBlock->ClassSize )
        {
          Status = STATUS_BUFFER_OVERFLOW;
        }
      else
        {
          /*  Fill buffer with requested info  */
          FullInformation = (PKEY_FULL_INFORMATION) KeyInformation;
          FullInformation->LastWriteTime.u.LowPart = KeyBlock->LastWriteTime.dwLowDateTime;
          FullInformation->LastWriteTime.u.HighPart = KeyBlock->LastWriteTime.dwHighDateTime;
          FullInformation->TitleIndex = 0;
          FullInformation->ClassOffset = sizeof(KEY_FULL_INFORMATION) - 
            sizeof(WCHAR);
          FullInformation->ClassLength = KeyBlock->ClassSize;
          FullInformation->SubKeys = KeyBlock->NumberOfSubKeys;
          FullInformation->MaxNameLen = 
            CmiGetMaxNameLength(RegistryFile, KeyBlock);
          FullInformation->MaxClassLen = 
            CmiGetMaxClassLength(RegistryFile, KeyBlock);
          FullInformation->Values = KeyBlock->NumberOfValues;
          FullInformation->MaxValueNameLen = 
            CmiGetMaxValueNameLength(RegistryFile, KeyBlock);
          FullInformation->MaxValueDataLen = 
            CmiGetMaxValueDataLength(RegistryFile, KeyBlock);
          if (KeyBlock->ClassSize != 0)
            {
              pClassData=CmiGetBlock(KeyObject->RegistryFile
                                     ,KeyBlock->ClassNameOffset,NULL);
              wcsncpy(FullInformation->Class,
                      (PWCHAR)pClassData->Data,
                      KeyBlock->ClassSize);
              CmiReleaseBlock(RegistryFile, pClassData);
            }
          *ResultLength = sizeof(KEY_FULL_INFORMATION) +
            KeyBlock->ClassSize ;
        }
      break;
    }
  ObDereferenceObject (KeyObject);

  return  Status;
}


NTSTATUS 
STDCALL
NtQueryValueKey (
	IN	HANDLE				KeyHandle,
	IN	PUNICODE_STRING			ValueName,
	IN	KEY_VALUE_INFORMATION_CLASS	KeyValueInformationClass,
	OUT	PVOID				KeyValueInformation,
	IN	ULONG				Length,
	OUT	PULONG				ResultLength
	)
{
  NTSTATUS  Status;
  PKEY_OBJECT  KeyObject;
  PREGISTRY_FILE  RegistryFile;
  PKEY_BLOCK  KeyBlock;
  PVALUE_BLOCK  ValueBlock;
  PDATA_BLOCK  DataBlock;
  PKEY_VALUE_BASIC_INFORMATION  ValueBasicInformation;
  PKEY_VALUE_PARTIAL_INFORMATION  ValuePartialInformation;
  PKEY_VALUE_FULL_INFORMATION  ValueFullInformation;
  char ValueName2[MAX_PATH];

  wcstombs(ValueName2,ValueName->Buffer,ValueName->Length>>1);
  ValueName2[ValueName->Length>>1]=0;

  /*  Verify that the handle is valid and is a registry key  */
  Status = ObReferenceObjectByHandle(KeyHandle,
                                     KEY_QUERY_VALUE,
                                     CmiKeyType,
                                     UserMode,
                                     (PVOID *)&KeyObject,
                                     NULL);
  if (!NT_SUCCESS(Status))
    {
      return  Status;
    }

  /*  Get pointer to KeyBlock  */
  KeyBlock = KeyObject->KeyBlock;
  RegistryFile = KeyObject->RegistryFile;
  /*  Get Value block of interest  */
  Status = CmiScanKeyForValue(RegistryFile, 
                              KeyBlock,
                              ValueName2,
                              &ValueBlock,NULL);
  if (!NT_SUCCESS(Status))
    {
      ObDereferenceObject(KeyObject);
      return  Status;
    }
  else if (ValueBlock != NULL)
    {
      switch (KeyValueInformationClass)
        {
        case KeyValueBasicInformation:
          *ResultLength = sizeof(KEY_VALUE_BASIC_INFORMATION) + 
            ValueBlock->NameSize * sizeof(WCHAR);
          if (Length < *ResultLength)
            {
              Status = STATUS_BUFFER_OVERFLOW;
            }
          else
            {
              ValueBasicInformation = (PKEY_VALUE_BASIC_INFORMATION) 
                KeyValueInformation;
              ValueBasicInformation->TitleIndex = 0;
              ValueBasicInformation->Type = ValueBlock->DataType;
              ValueBasicInformation->NameLength = 
                (ValueBlock->NameSize + 1) * sizeof(WCHAR);
              mbstowcs(ValueBasicInformation->Name, ValueBlock->Name,ValueBlock->NameSize*2);
              ValueBasicInformation->Name[ValueBlock->NameSize]=0;
            }
          break;

        case KeyValuePartialInformation:
          *ResultLength = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 
            (ValueBlock->DataSize & LONG_MAX);
          if (Length < *ResultLength)
            {
              Status = STATUS_BUFFER_OVERFLOW;
            }
          else
            {
              ValuePartialInformation = (PKEY_VALUE_PARTIAL_INFORMATION) 
                KeyValueInformation;
              ValuePartialInformation->TitleIndex = 0;
              ValuePartialInformation->Type = ValueBlock->DataType;
              ValuePartialInformation->DataLength = ValueBlock->DataSize & LONG_MAX;
              if(ValueBlock->DataSize >0)
              {
                DataBlock = CmiGetBlock(RegistryFile, ValueBlock->DataOffset,NULL);
                RtlCopyMemory(ValuePartialInformation->Data, 
                            DataBlock->Data,
                            ValueBlock->DataSize & LONG_MAX);
                CmiReleaseBlock(RegistryFile, DataBlock);
              }
              else
              {
                RtlCopyMemory(ValuePartialInformation->Data, 
                            &ValueBlock->DataOffset, 
                            ValueBlock->DataSize & LONG_MAX);
              }
            }
          break;

        case KeyValueFullInformation:
          *ResultLength = sizeof(KEY_VALUE_FULL_INFORMATION)
                 + (ValueBlock->NameSize -1) * sizeof(WCHAR)
                 + (ValueBlock->DataSize & LONG_MAX);
          if (Length < *ResultLength)
            {
              Status = STATUS_BUFFER_OVERFLOW;
            }
          else
            {
              ValueFullInformation = (PKEY_VALUE_FULL_INFORMATION) 
                KeyValueInformation;
              ValueFullInformation->TitleIndex = 0;
              ValueFullInformation->Type = ValueBlock->DataType;
              ValueFullInformation->DataOffset = 
                (DWORD)ValueFullInformation->Name - (DWORD)ValueFullInformation
                + (ValueBlock->NameSize ) * sizeof(WCHAR);
              ValueFullInformation->DataOffset =
		(ValueFullInformation->DataOffset +3) &0xfffffffc;
              ValueFullInformation->DataLength = ValueBlock->DataSize & LONG_MAX;
              ValueFullInformation->NameLength =
                (ValueBlock->NameSize ) * sizeof(WCHAR);
              mbstowcs(ValueFullInformation->Name, ValueBlock->Name,ValueBlock->NameSize*2);
              if(ValueBlock->DataSize >0)
              {
                DataBlock = CmiGetBlock(RegistryFile, ValueBlock->DataOffset,NULL);
                RtlCopyMemory((char *)(ValueFullInformation)
		              + ValueFullInformation->DataOffset,
                            DataBlock->Data, 
                            ValueBlock->DataSize & LONG_MAX);
                CmiReleaseBlock(RegistryFile, DataBlock);
              }
              else
              {
                RtlCopyMemory((char *)(ValueFullInformation)
		              + ValueFullInformation->DataOffset,
                            &ValueBlock->DataOffset, 
                            ValueBlock->DataSize & LONG_MAX);
              }
            }
          break;
        }
    }
  else
    {
      Status = STATUS_UNSUCCESSFUL;
    }
  ObDereferenceObject(KeyObject);
  
  return  Status;
}


NTSTATUS 
STDCALL
NtSetValueKey (
	IN	HANDLE			KeyHandle, 
	IN	PUNICODE_STRING		ValueName,
	IN	ULONG			TitleIndex,
	IN	ULONG			Type, 
	IN	PVOID			Data,
	IN	ULONG			DataSize
	)
{
 NTSTATUS  Status;
 PKEY_OBJECT  KeyObject;
 PREGISTRY_FILE  RegistryFile;
 PKEY_BLOCK  KeyBlock;
 PVALUE_BLOCK  ValueBlock;
 BLOCK_OFFSET VBOffset;
 char ValueName2[MAX_PATH];
 PDATA_BLOCK   DataBlock, NewDataBlock;
 PHEAP_BLOCK pHeap;
// KIRQL  OldIrql;

  wcstombs(ValueName2,ValueName->Buffer,ValueName->Length>>1);
  ValueName2[ValueName->Length>>1]=0;

  /*  Verify that the handle is valid and is a registry key  */
  Status = ObReferenceObjectByHandle(KeyHandle,
                                     KEY_SET_VALUE,
                                     CmiKeyType,
                                     UserMode,
                                     (PVOID *)&KeyObject,
                                     NULL);
  if (!NT_SUCCESS(Status))
    return  Status;

  /*  Get pointer to KeyBlock  */
  KeyBlock = KeyObject->KeyBlock;
  RegistryFile = KeyObject->RegistryFile;
  Status = CmiScanKeyForValue(RegistryFile,
                              KeyBlock,
                              ValueName2,
                              &ValueBlock, &VBOffset);
  if (!NT_SUCCESS(Status))
  {
    ObDereferenceObject (KeyObject);
    return  Status;
  }
//  KeAcquireSpinLock(&RegistryFile->RegLock, &OldIrql);
  if (ValueBlock == NULL)
  {
    Status = CmiAddValueToKey(RegistryFile,
                                 KeyBlock,
                                 ValueName2,
				 &ValueBlock,
				 &VBOffset);
  }
  if (!NT_SUCCESS(Status))
  {
    ObDereferenceObject (KeyObject);
    return  Status;
  }
  else
  {
    /* FIXME if datasize <=4 then write in valueblock directly */
    if (DataSize <= 4)
    {
      if (( ValueBlock->DataSize <0 )
	&& (DataBlock = CmiGetBlock(RegistryFile, ValueBlock->DataOffset,NULL)))
        {
          CmiDestroyBlock(RegistryFile, DataBlock, ValueBlock->DataOffset);
        }
      RtlCopyMemory(&ValueBlock->DataOffset, Data, DataSize);
      ValueBlock->DataSize = DataSize | 0x80000000;
      ValueBlock->DataType = Type;
      memcpy(&ValueBlock->DataOffset, Data, DataSize);
    }
    /* If new data size is <= current then overwrite current data  */
    else if (DataSize <= (ValueBlock->DataSize & 0x7fffffff))
    {
      DataBlock = CmiGetBlock(RegistryFile, ValueBlock->DataOffset,&pHeap);
      RtlCopyMemory(DataBlock->Data, Data, DataSize);
      ValueBlock->DataSize = DataSize;
      ValueBlock->DataType = Type;
      CmiReleaseBlock(RegistryFile, DataBlock);
      /* update time of heap */
      if(RegistryFile->Filename)
        ZwQuerySystemTime((PTIME) &pHeap->DateModified);
    }
    else
    {
     BLOCK_OFFSET NewOffset;
      /*  Destroy current data block and allocate a new one  */
      if (( ValueBlock->DataSize <0 )
	&& (DataBlock = CmiGetBlock(RegistryFile, ValueBlock->DataOffset,NULL)))
        {
          CmiDestroyBlock(RegistryFile, DataBlock, ValueBlock->DataOffset);
        }
      Status = CmiAllocateBlock(RegistryFile,
                                (PVOID *)&NewDataBlock,
                                DataSize,&NewOffset);
      RtlCopyMemory(&NewDataBlock->Data[0], Data, DataSize);
      ValueBlock->DataSize = DataSize;
      ValueBlock->DataType = Type;
      CmiReleaseBlock(RegistryFile, NewDataBlock);
      ValueBlock->DataOffset = NewOffset;
    }
    /* update time of heap */
    if(RegistryFile->Filename && CmiGetBlock(RegistryFile, VBOffset,&pHeap))
      ZwQuerySystemTime((PTIME) &pHeap->DateModified);

  }
//  KeReleaseSpinLock(&RegistryFile->RegLock, OldIrql);
  ObDereferenceObject (KeyObject);
  
  return  Status;
}

NTSTATUS
STDCALL
NtDeleteValueKey (
	IN	HANDLE		KeyHandle,
	IN	PUNICODE_STRING	ValueName
	)
{
 NTSTATUS  Status;
 PKEY_OBJECT  KeyObject;
 PREGISTRY_FILE  RegistryFile;
 PKEY_BLOCK  KeyBlock;
 char ValueName2[MAX_PATH];
// KIRQL  OldIrql;

  wcstombs(ValueName2,ValueName->Buffer,ValueName->Length>>1);
  ValueName2[ValueName->Length>>1]=0;

  /*  Verify that the handle is valid and is a registry key  */
  Status = ObReferenceObjectByHandle(KeyHandle,
                                     KEY_QUERY_VALUE,
                                     CmiKeyType,
                                     UserMode,
                                     (PVOID *)&KeyObject,
                                     NULL);
  if (!NT_SUCCESS(Status))
    {
      return  Status;
    }

  /*  Get pointer to KeyBlock  */
  KeyBlock = KeyObject->KeyBlock;
  RegistryFile = KeyObject->RegistryFile;
//  KeAcquireSpinLock(&RegistryFile->RegLock, &OldIrql);
  Status = CmiDeleteValueFromKey(RegistryFile,
                                 KeyBlock,
                                 ValueName2);
//  KeReleaseSpinLock(&RegistryFile->RegLock, OldIrql);
  ObDereferenceObject(KeyObject);

  return  Status;
}

NTSTATUS
STDCALL 
NtLoadKey (
	PHANDLE			KeyHandle,
	POBJECT_ATTRIBUTES	ObjectAttributes
	)
{
  return  NtLoadKey2(KeyHandle,
                     ObjectAttributes,
                     0);
}


NTSTATUS
STDCALL
NtLoadKey2 (
	PHANDLE			KeyHandle,
	POBJECT_ATTRIBUTES	ObjectAttributes,
	ULONG			Unknown3
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtNotifyChangeKey (
	IN	HANDLE			KeyHandle,
	IN	HANDLE			Event,
	IN	PIO_APC_ROUTINE		ApcRoutine		OPTIONAL, 
	IN	PVOID			ApcContext		OPTIONAL, 
	OUT	PIO_STATUS_BLOCK	IoStatusBlock,
	IN	ULONG			CompletionFilter,
	IN	BOOLEAN			Asynchroneous, 
	OUT	PVOID			ChangeBuffer,
	IN	ULONG			Length,
	IN	BOOLEAN			WatchSubtree
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtQueryMultipleValueKey (
	IN	HANDLE		KeyHandle,
	IN	PWVALENT	ListOfValuesToQuery,
	IN	ULONG		NumberOfItems,
	OUT	PVOID		MultipleValueInformation,
	IN	ULONG		Length,
	OUT	PULONG		ReturnLength
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtReplaceKey (
	IN	POBJECT_ATTRIBUTES	ObjectAttributes,
	IN	HANDLE			Key,
	IN	POBJECT_ATTRIBUTES	ReplacedObjectAttributes
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtRestoreKey (
	IN	HANDLE	KeyHandle,
	IN	HANDLE	FileHandle,
	IN	ULONG	RestoreFlags
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtSaveKey (
	IN	HANDLE	KeyHandle,
	IN	HANDLE	FileHandle
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtSetInformationKey (
	IN	HANDLE	KeyHandle,
	IN	CINT	KeyInformationClass,
	IN	PVOID	KeyInformation,
	IN	ULONG	KeyInformationLength
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL 
NtUnloadKey (
	HANDLE	KeyHandle
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL 
NtInitializeRegistry (
	BOOLEAN	SetUpBoot
	)
{
//	UNIMPLEMENTED;
   return STATUS_SUCCESS;
}
