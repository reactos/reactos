/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             ntoskrnl/cm/regfile.c
 * PURPOSE:          Registry file manipulation routines
 * UPDATE HISTORY:
*/

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


#define  REG_BLOCK_SIZE			4096
#define  REG_HEAP_BLOCK_DATA_OFFSET	32
#define  REG_HEAP_ID			0x6e696268
#define  REG_INIT_BLOCK_LIST_SIZE	32
#define  REG_INIT_HASH_TABLE_SIZE	3
#define  REG_EXTEND_HASH_TABLE_SIZE	4
#define  REG_VALUE_LIST_BLOCK_MULTIPLE	4
#define  REG_KEY_BLOCK_ID		0x6b6e
#define  REG_HASH_TABLE_BLOCK_ID	0x666c
#define  REG_VALUE_BLOCK_ID		0x6b76
#define  REG_KEY_BLOCK_TYPE		0x20
#define  REG_ROOT_KEY_BLOCK_TYPE	0x2c


extern PREGISTRY_FILE  CmiVolatileFile;

void CmiCreateDefaultHeaderBlock(PHEADER_BLOCK HeaderBlock)
{	
  RtlZeroMemory(HeaderBlock, sizeof(HEADER_BLOCK));
  HeaderBlock->BlockId = 0x66676572;
  HeaderBlock->DateModified.dwLowDateTime = 0;
  HeaderBlock->DateModified.dwHighDateTime = 0;
  HeaderBlock->Version = 1;
  HeaderBlock->Unused3 = 3;
  HeaderBlock->Unused5 = 1;
  HeaderBlock->RootKeyBlock = 0;
  HeaderBlock->BlockSize = REG_BLOCK_SIZE;
  HeaderBlock->Unused6 = 1;
  HeaderBlock->Checksum = 0;
}

void CmiCreateDefaultHeapBlock(PHEAP_BLOCK HeapBlock)
{
  RtlZeroMemory(HeapBlock, sizeof(HEAP_BLOCK));
  HeapBlock->BlockId = REG_HEAP_ID;
  HeapBlock->DateModified.dwLowDateTime = 0;
  HeapBlock->DateModified.dwHighDateTime = 0;
  HeapBlock->BlockSize = REG_BLOCK_SIZE;
}

void CmiCreateDefaultRootKeyBlock(PKEY_BLOCK RootKeyBlock)
{
  RtlZeroMemory(RootKeyBlock, sizeof(KEY_BLOCK));
  RootKeyBlock->SubBlockSize=-sizeof(KEY_BLOCK);
  RootKeyBlock->SubBlockId = REG_KEY_BLOCK_ID;
  RootKeyBlock->Type = REG_ROOT_KEY_BLOCK_TYPE;
  ZwQuerySystemTime((PTIME) &RootKeyBlock->LastWriteTime);
  RootKeyBlock->ParentKeyOffset = 0;
  RootKeyBlock->NumberOfSubKeys = 0;
  RootKeyBlock->HashTableOffset = -1;
  RootKeyBlock->NumberOfValues = 0;
  RootKeyBlock->ValuesOffset = -1;
  RootKeyBlock->SecurityKeyOffset = 0;
  RootKeyBlock->ClassNameOffset = -1;
  RootKeyBlock->NameSize = 0;
  RootKeyBlock->ClassSize = 0;
}

NTSTATUS CmiCreateNewRegFile( HANDLE FileHandle )
{
  NTSTATUS Status;
  PHEADER_BLOCK HeaderBlock;
  PHEAP_BLOCK HeapBlock;
  PKEY_BLOCK  RootKeyBlock;
  PFREE_SUB_BLOCK FreeSubBlock;
  
  IO_STATUS_BLOCK IoStatusBlock;
  char* tBuf;
  
  tBuf = (char*) ExAllocatePool(NonPagedPool, 2*REG_BLOCK_SIZE);
  if( tBuf == NULL )
    return STATUS_INSUFFICIENT_RESOURCES;
    
  HeaderBlock = (PHEADER_BLOCK)tBuf;
  HeapBlock = (PHEAP_BLOCK) (tBuf+REG_BLOCK_SIZE);
  RootKeyBlock = (PKEY_BLOCK) (tBuf+REG_BLOCK_SIZE+REG_HEAP_BLOCK_DATA_OFFSET);
  FreeSubBlock = (PFREE_SUB_BLOCK) (tBuf+REG_BLOCK_SIZE+REG_HEAP_BLOCK_DATA_OFFSET+sizeof(KEY_BLOCK));
  
  CmiCreateDefaultHeaderBlock(HeaderBlock);
  CmiCreateDefaultHeapBlock(HeapBlock);
  CmiCreateDefaultRootKeyBlock(RootKeyBlock);
  
  HeapBlock->BlockOffset = 0; //First block.
  HeaderBlock->RootKeyBlock = REG_HEAP_BLOCK_DATA_OFFSET; //Offset to root key block.
  //The rest of the block is free
  FreeSubBlock->SubBlockSize = REG_BLOCK_SIZE-(REG_HEAP_BLOCK_DATA_OFFSET+sizeof(KEY_BLOCK));
  
  Status = ZwWriteFile( FileHandle, NULL, NULL, NULL,
                        &IoStatusBlock, tBuf, 2*REG_BLOCK_SIZE, 0, NULL );
  
  ExFreePool( tBuf );
  return Status;
}

PREGISTRY_FILE  
CmiCreateRegistry(PWSTR  Filename)
{
 PREGISTRY_FILE  RegistryFile;
 HANDLE FileHandle;
 PKEY_BLOCK  RootKeyBlock;
 PHEAP_BLOCK tmpHeap;
 LARGE_INTEGER fileOffset;
 PFREE_SUB_BLOCK  FreeBlock;
 DWORD  FreeOffset;
 int i, j;
 BLOCK_OFFSET BlockOffset;
DPRINT("CmiCreateRegistry() Filename '%S'\n", Filename);
  RegistryFile = ExAllocatePool(NonPagedPool, sizeof(REGISTRY_FILE));

  if (RegistryFile == NULL)
    return NULL;

  if (Filename != NULL)
   {
     UNICODE_STRING TmpFileName;
     OBJECT_ATTRIBUTES  ObjectAttributes;
     NTSTATUS Status;
     FILE_STANDARD_INFORMATION fsi;
     IO_STATUS_BLOCK IoSB;

      /* Duplicate Filename  */
      RegistryFile->Filename = ExAllocatePool(NonPagedPool, MAX_PATH);
      wcscpy(RegistryFile->Filename , Filename);

      RtlInitUnicodeString (&TmpFileName, Filename);
      InitializeObjectAttributes(&ObjectAttributes,
                             &TmpFileName,
                             0,
                             NULL,
                             NULL);

      Status = NtCreateFile( &FileHandle,
                         FILE_ALL_ACCESS,
                         &ObjectAttributes,
                         &IoSB,
                         NULL,
                         FILE_ATTRIBUTE_NORMAL,
                         0,
                         FILE_OPEN_IF,
                         FILE_NON_DIRECTORY_FILE,
                         NULL,
                         0 );

      /* FIXME: create directory if IoSB.Status == STATUS_OBJECT_PATH_NOT_FOUND */
      if( !NT_SUCCESS(Status) )
      {
        ExFreePool(RegistryFile->Filename);
	RegistryFile->Filename = NULL;
	return NULL;
      }
      /*if file did not exist*/
      if( IoSB.Information == FILE_CREATED ){
        Status = CmiCreateNewRegFile( FileHandle );
        if( !NT_SUCCESS(Status) )
        {
          ExFreePool(RegistryFile->Filename);
	  RegistryFile->Filename = NULL;
	  return NULL;
        }
      }

      RegistryFile->HeaderBlock = (PHEADER_BLOCK) 
        ExAllocatePool(NonPagedPool, sizeof(HEADER_BLOCK));

      fileOffset.u.HighPart = 0;
      fileOffset.u.LowPart = 0;
      Status = ZwReadFile(FileHandle, 
                      0, 0, 0, 0, 
                      RegistryFile->HeaderBlock, 
                      sizeof(HEADER_BLOCK), 
                      &fileOffset, 0);
      if (!NT_SUCCESS(Status))
        {
          ExFreePool(RegistryFile->Filename);
	  RegistryFile->Filename = NULL;
	  return NULL;
        }

      Status = ZwQueryInformationFile(FileHandle,&IoSB,&fsi
		,sizeof(fsi),FileStandardInformation);
      if (!NT_SUCCESS(Status))
        {
          ExFreePool(RegistryFile->Filename);
	  RegistryFile->Filename = NULL;
	  return NULL;
        }
      RegistryFile->FileSize = fsi.EndOfFile.u.LowPart;
      RegistryFile->BlockListSize = RegistryFile->FileSize / 4096 -1;
//      RegistryFile->NumberOfBlocks = RegistryFile->BlockListSize;
      RegistryFile->BlockList = ExAllocatePool(NonPagedPool,
	   sizeof(PHEAP_BLOCK *) * (RegistryFile->BlockListSize ));
      BlockOffset=0;
      fileOffset.u.HighPart = 0;
      fileOffset.u.LowPart = 4096;
      RegistryFile->BlockList [0]
	   = ExAllocatePool(NonPagedPool,RegistryFile->FileSize-4096);
      if (RegistryFile->BlockList[0] == NULL)
      {
//        Status = STATUS_INSUFFICIENT_RESOURCES;
	DPRINT("error allocating %d bytes for registry\n"
	   ,RegistryFile->FileSize-4096);
	ZwClose(FileHandle);
	return NULL;
      }
      Status = ZwReadFile(FileHandle,
                      0, 0, 0, 0, 
                      RegistryFile->BlockList [0],
                      RegistryFile->FileSize-4096,
                      &fileOffset, 0);
      ZwClose(FileHandle);
      if (!NT_SUCCESS(Status))
      {
	DPRINT("error %x reading registry file at offset %x\n"
			,Status,fileOffset.u.LowPart);
        return NULL;
      }
      RegistryFile->FreeListSize = 0;
      RegistryFile->FreeListMax = 0;
      RegistryFile->FreeList = NULL;
      for(i=0 ; i <RegistryFile->BlockListSize; i++)
      {
        tmpHeap = (PHEAP_BLOCK)(((char *)RegistryFile->BlockList [0])+BlockOffset);
	if (tmpHeap->BlockId != REG_HEAP_ID )
	{
	  DPRINT("bad BlockId %x,offset %x\n",tmpHeap->BlockId,fileOffset.u.LowPart);
	}
	RegistryFile->BlockList [i]
	   = tmpHeap;
	if (tmpHeap->BlockSize >4096)
	{
	  for(j=1;j<tmpHeap->BlockSize/4096;j++)
            RegistryFile->BlockList[i+j] = RegistryFile->BlockList[i];
	  i = i+j-1;
	}
        /* search free blocks and add to list */
        FreeOffset=REG_HEAP_BLOCK_DATA_OFFSET;
        while(FreeOffset < tmpHeap->BlockSize)
        {
          FreeBlock = (PFREE_SUB_BLOCK)((char *)RegistryFile->BlockList[i]
			+FreeOffset);
          if ( FreeBlock->SubBlockSize>0)
	  {
	    CmiAddFree(RegistryFile,FreeBlock
		,RegistryFile->BlockList[i]->BlockOffset+FreeOffset);
	    FreeOffset += FreeBlock->SubBlockSize;
	  }
	  else
	    FreeOffset -= FreeBlock->SubBlockSize;
        }
	BlockOffset += tmpHeap->BlockSize;
      }
      Status = ObReferenceObjectByHandle(FileHandle,
                 FILE_ALL_ACCESS,
		 IoFileObjectType,
		 UserMode,
                 (PVOID*)&RegistryFile->FileObject,
                 NULL);
   }
  else
    {
      RegistryFile->Filename = NULL;
      RegistryFile->FileObject = NULL;

      RegistryFile->HeaderBlock = (PHEADER_BLOCK) 
        ExAllocatePool(NonPagedPool, sizeof(HEADER_BLOCK));
      CmiCreateDefaultHeaderBlock(RegistryFile->HeaderBlock); 
      
      RootKeyBlock = (PKEY_BLOCK) ExAllocatePool(NonPagedPool, sizeof(KEY_BLOCK));
      CmiCreateDefaultRootKeyBlock(RootKeyBlock);

      RegistryFile->HeaderBlock->RootKeyBlock = (BLOCK_OFFSET) RootKeyBlock;
    }
  KeInitializeSemaphore(&RegistryFile->RegSem, 1, 1);

  return  RegistryFile;
}

ULONG
CmiGetMaxNameLength(PREGISTRY_FILE  RegistryFile,
                    PKEY_BLOCK  KeyBlock)
{
  ULONG  Idx, MaxName;
  PHASH_TABLE_BLOCK  HashBlock;
  PKEY_BLOCK  CurSubKeyBlock;

  MaxName = 0;
  HashBlock = CmiGetBlock(RegistryFile, KeyBlock->HashTableOffset,NULL);
  if (HashBlock == 0)
    {
      return  0;
    }
  for (Idx = 0; Idx < HashBlock->HashTableSize; Idx++)
    {
      if (HashBlock->Table[Idx].KeyOffset != 0)
        {
          CurSubKeyBlock = CmiGetBlock(RegistryFile,
                                          HashBlock->Table[Idx].KeyOffset,NULL);
          if (MaxName < CurSubKeyBlock->NameSize)
            {
              MaxName = CurSubKeyBlock->NameSize;
            }
          CmiReleaseBlock(RegistryFile, CurSubKeyBlock);
        }
    }

  CmiReleaseBlock(RegistryFile, HashBlock);
  
  return  MaxName;
}

ULONG  
CmiGetMaxClassLength(PREGISTRY_FILE  RegistryFile,
                     PKEY_BLOCK  KeyBlock)
{
  ULONG  Idx, MaxClass;
  PHASH_TABLE_BLOCK  HashBlock;
  PKEY_BLOCK  CurSubKeyBlock;

  MaxClass = 0;
  HashBlock = CmiGetBlock(RegistryFile, KeyBlock->HashTableOffset,NULL);
  if (HashBlock == 0)
    {
      return  0;
    }
  for (Idx = 0; Idx < HashBlock->HashTableSize; Idx++)
    {
      if (HashBlock->Table[Idx].KeyOffset != 0)
        {
          CurSubKeyBlock = CmiGetBlock(RegistryFile,
                                          HashBlock->Table[Idx].KeyOffset,NULL);
          if (MaxClass < CurSubKeyBlock->ClassSize)
            {
              MaxClass = CurSubKeyBlock->ClassSize;
            }
          CmiReleaseBlock(RegistryFile, CurSubKeyBlock);
        }
    }

  CmiReleaseBlock(RegistryFile, HashBlock);
  
  return  MaxClass;
}

ULONG  
CmiGetMaxValueNameLength(PREGISTRY_FILE  RegistryFile,
                         PKEY_BLOCK  KeyBlock)
{
  ULONG  Idx, MaxValueName;
  PVALUE_LIST_BLOCK  ValueListBlock;
  PVALUE_BLOCK  CurValueBlock;

  ValueListBlock = CmiGetBlock(RegistryFile, 
                               KeyBlock->ValuesOffset,NULL);
  MaxValueName = 0;
  if (ValueListBlock == 0)
    {
      return  0;
    }
  for (Idx = 0; Idx < KeyBlock->NumberOfValues; Idx++)
    {
      CurValueBlock = CmiGetBlock(RegistryFile,
                                  ValueListBlock->Values[Idx],NULL);
      if (CurValueBlock != NULL &&
          MaxValueName < CurValueBlock->NameSize)
        {
          MaxValueName = CurValueBlock->NameSize;
        }
      CmiReleaseBlock(RegistryFile, CurValueBlock);
    }

  CmiReleaseBlock(RegistryFile, ValueListBlock);
  
  return  MaxValueName;
}

ULONG  
CmiGetMaxValueDataLength(PREGISTRY_FILE  RegistryFile,
                         PKEY_BLOCK  KeyBlock)
{
  ULONG  Idx, MaxValueData;
  PVALUE_LIST_BLOCK  ValueListBlock;
  PVALUE_BLOCK  CurValueBlock;

  ValueListBlock = CmiGetBlock(RegistryFile, 
                               KeyBlock->ValuesOffset,NULL);
  MaxValueData = 0;
  if (ValueListBlock == 0)
    {
      return  0;
    }
  for (Idx = 0; Idx < KeyBlock->NumberOfValues; Idx++)
    {
      CurValueBlock = CmiGetBlock(RegistryFile,
                                  ValueListBlock->Values[Idx],NULL);
      if (CurValueBlock != NULL &&
          MaxValueData < (CurValueBlock->DataSize & LONG_MAX) )
        {
          MaxValueData = CurValueBlock->DataSize & LONG_MAX;
        }
      CmiReleaseBlock(RegistryFile, CurValueBlock);
    }

  CmiReleaseBlock(RegistryFile, ValueListBlock);
  
  return  MaxValueData;
}

NTSTATUS
CmiScanForSubKey(IN PREGISTRY_FILE  RegistryFile,
                 IN PKEY_BLOCK  KeyBlock,
                 OUT PKEY_BLOCK  *SubKeyBlock,
                 OUT BLOCK_OFFSET *BlockOffset,
                 IN PCHAR  KeyName,
                 IN ACCESS_MASK  DesiredAccess,
                 IN ULONG Attributes)
{
  ULONG  Idx;
  PHASH_TABLE_BLOCK  HashBlock;
  PKEY_BLOCK  CurSubKeyBlock;
  WORD KeyLength = strlen(KeyName);

  HashBlock = CmiGetBlock(RegistryFile, KeyBlock->HashTableOffset,NULL);
  *SubKeyBlock = NULL;
  if (HashBlock == NULL)
    {
      return  STATUS_SUCCESS;
    }
//  for (Idx = 0; Idx < HashBlock->HashTableSize; Idx++)
  for (Idx = 0; Idx < KeyBlock->NumberOfSubKeys
		&& Idx < HashBlock->HashTableSize; Idx++)
    {
      if (Attributes & OBJ_CASE_INSENSITIVE)
        {
          if (HashBlock->Table[Idx].KeyOffset != 0 &&
              HashBlock->Table[Idx].KeyOffset != -1 &&
              !_strnicmp(KeyName, (PCHAR) &HashBlock->Table[Idx].HashValue, 4))
            {
              CurSubKeyBlock = CmiGetBlock(RegistryFile,
                                           HashBlock->Table[Idx].KeyOffset,NULL);
              if ( CurSubKeyBlock->NameSize == KeyLength
                  && !_strnicmp(KeyName, CurSubKeyBlock->Name, KeyLength))
                {
                  *SubKeyBlock = CurSubKeyBlock;
                  *BlockOffset = HashBlock->Table[Idx].KeyOffset;
                  break;
                }
              else
                {
                  CmiReleaseBlock(RegistryFile, CurSubKeyBlock);
                }
            }
        }
      else
        {
          if (HashBlock->Table[Idx].KeyOffset != 0 &&
              HashBlock->Table[Idx].KeyOffset != -1 &&
              !strncmp(KeyName, (PCHAR) &HashBlock->Table[Idx].HashValue, 4))
            {
              CurSubKeyBlock = CmiGetBlock(RegistryFile,
                                           HashBlock->Table[Idx].KeyOffset,NULL);
              if ( CurSubKeyBlock->NameSize == KeyLength
                  && !_strnicmp(KeyName, CurSubKeyBlock->Name, KeyLength))
                {
                  *SubKeyBlock = CurSubKeyBlock;
                  *BlockOffset = HashBlock->Table[Idx].KeyOffset;
                  break;
                }
              else
                {
                  CmiReleaseBlock(RegistryFile, CurSubKeyBlock);
                }
            }
        }
    }
  
  CmiReleaseBlock(RegistryFile, HashBlock);
  
  return  STATUS_SUCCESS;
}

NTSTATUS
CmiAddSubKey(PREGISTRY_FILE  RegistryFile, 
             PKEY_OBJECT Parent,
             PKEY_OBJECT  SubKey,
             PWSTR  NewSubKeyName,
             USHORT  NewSubKeyNameSize,
             ULONG  TitleIndex,
             PUNICODE_STRING  Class, 
             ULONG  CreateOptions)
{
  PKEY_BLOCK  KeyBlock = Parent->KeyBlock;
  NTSTATUS  Status;
  PHASH_TABLE_BLOCK  HashBlock, NewHashBlock;
  PKEY_BLOCK  NewKeyBlock; 
  BLOCK_OFFSET NKBOffset;
  ULONG  NewBlockSize;
  USHORT NameSize;

  if (NewSubKeyName[0] == L'\\')
  {
    NewSubKeyName++;
    NameSize = NewSubKeyNameSize/2-1;
  }
  else
    NameSize = NewSubKeyNameSize/2;
  Status = STATUS_SUCCESS;

  NewBlockSize = sizeof(KEY_BLOCK) + NameSize;
  Status = CmiAllocateBlock(RegistryFile, (PVOID) &NewKeyBlock 
		, NewBlockSize,&NKBOffset);
  if (NewKeyBlock == NULL)
  {
    Status = STATUS_INSUFFICIENT_RESOURCES;
  }
  else
  {
    NewKeyBlock->SubBlockId = REG_KEY_BLOCK_ID;
    NewKeyBlock->Type = REG_KEY_BLOCK_TYPE;
    ZwQuerySystemTime((PTIME) &NewKeyBlock->LastWriteTime);
    NewKeyBlock->ParentKeyOffset = -1;
    NewKeyBlock->NumberOfSubKeys = 0;
    NewKeyBlock->HashTableOffset = -1;
    NewKeyBlock->NumberOfValues = 0;
    NewKeyBlock->ValuesOffset = -1;
    NewKeyBlock->SecurityKeyOffset = -1;
    NewKeyBlock->NameSize = NameSize;
    wcstombs(NewKeyBlock->Name,NewSubKeyName,NameSize);
    NewKeyBlock->ClassNameOffset = -1;
    if (Class)
    {
     PDATA_BLOCK pClass;
      NewKeyBlock->ClassSize = Class->Length+sizeof(WCHAR);
      Status = CmiAllocateBlock(RegistryFile
			,(PVOID)&pClass
			,NewKeyBlock->ClassSize
			,&NewKeyBlock->ClassNameOffset );
      wcsncpy((PWSTR)pClass->Data,Class->Buffer,Class->Length);
      ( (PWSTR)(pClass->Data))[Class->Length]=0;
    }
  }

  if (!NT_SUCCESS(Status))
    {
      return  Status;
    }
  SubKey->KeyBlock = NewKeyBlock;
  SubKey->BlockOffset = NKBOffset;
  /* don't modify hash table if key is volatile and parent is not */
  if (RegistryFile == CmiVolatileFile && Parent->RegistryFile != RegistryFile)
  {
    return Status;
  }
  if (KeyBlock->HashTableOffset == -1)
    {
      Status = CmiAllocateHashTableBlock(RegistryFile, 
                                         &HashBlock,
					 &KeyBlock->HashTableOffset,
                                         REG_INIT_HASH_TABLE_SIZE);
      if (!NT_SUCCESS(Status))
        {
          return  Status;
        }
    }
  else
    {
      HashBlock = CmiGetBlock(RegistryFile, KeyBlock->HashTableOffset,NULL);
      if (KeyBlock->NumberOfSubKeys + 1 >= HashBlock->HashTableSize)
        {
       BLOCK_OFFSET HTOffset;

          /*  Reallocate the hash table block  */
          Status = CmiAllocateHashTableBlock(RegistryFile,
                                             &NewHashBlock,
					 &HTOffset,
                                             HashBlock->HashTableSize +
                                               REG_EXTEND_HASH_TABLE_SIZE);
          if (!NT_SUCCESS(Status))
            {
              return  Status;
            }
          RtlZeroMemory(&NewHashBlock->Table[0],
                        sizeof(NewHashBlock->Table[0]) * NewHashBlock->HashTableSize);
          RtlCopyMemory(&NewHashBlock->Table[0],
                        &HashBlock->Table[0],
                        sizeof(NewHashBlock->Table[0]) * HashBlock->HashTableSize);
          CmiDestroyBlock(RegistryFile, HashBlock
		, KeyBlock->HashTableOffset);
	  KeyBlock->HashTableOffset = HTOffset;
          HashBlock = NewHashBlock;
        }
    }
  Status = CmiAddKeyToHashTable(RegistryFile, HashBlock, NewKeyBlock,NKBOffset);
  if (NT_SUCCESS(Status))
    {
      KeyBlock->NumberOfSubKeys++;
    }
  
  return  Status;
}

NTSTATUS  
CmiScanKeyForValue(IN PREGISTRY_FILE  RegistryFile,
                   IN PKEY_BLOCK  KeyBlock,
                   IN PCHAR  ValueName,
                   OUT PVALUE_BLOCK  *ValueBlock,
		   OUT BLOCK_OFFSET *VBOffset)
{
  ULONG Length;
 ULONG  Idx;
 PVALUE_LIST_BLOCK  ValueListBlock;
 PVALUE_BLOCK  CurValueBlock;
  ValueListBlock = CmiGetBlock(RegistryFile, 
                               KeyBlock->ValuesOffset,NULL);
  *ValueBlock = NULL;
  if (ValueListBlock == NULL)
    {
      DPRINT("ValueListBlock is NULL\n");
      return  STATUS_SUCCESS;
    }
  for (Idx = 0; Idx < KeyBlock->NumberOfValues; Idx++)
    {
      CurValueBlock = CmiGetBlock(RegistryFile,
                                  ValueListBlock->Values[Idx],NULL);
      /* FIXME : perhaps we must not ignore case if NtCreateKey has not been */
      /*         called with OBJ_CASE_INSENSITIVE flag ? */
      Length = strlen(ValueName);
      if ((CurValueBlock != NULL) &&
          (CurValueBlock->NameSize == Length) &&
          (_strnicmp(CurValueBlock->Name, ValueName, Length) == 0))
        {
          *ValueBlock = CurValueBlock;
	  if(VBOffset) *VBOffset = ValueListBlock->Values[Idx];
        DPRINT("Found value %s\n", ValueName);
          break;
        }
      CmiReleaseBlock(RegistryFile, CurValueBlock);
    }

  CmiReleaseBlock(RegistryFile, ValueListBlock);
  
  return  STATUS_SUCCESS;
}


NTSTATUS
CmiGetValueFromKeyByIndex(IN PREGISTRY_FILE  RegistryFile,
                          IN PKEY_BLOCK  KeyBlock,
                          IN ULONG  Index,
                          OUT PVALUE_BLOCK  *ValueBlock)
{
 PVALUE_LIST_BLOCK  ValueListBlock;
 PVALUE_BLOCK  CurValueBlock;
  ValueListBlock = CmiGetBlock(RegistryFile,
                               KeyBlock->ValuesOffset,NULL);
  *ValueBlock = NULL;
  if (ValueListBlock == NULL)
    {
      return STATUS_NO_MORE_ENTRIES;
    }
  if (Index >= KeyBlock->NumberOfValues)
    {
      return STATUS_NO_MORE_ENTRIES;
    }
  CurValueBlock = CmiGetBlock(RegistryFile,
                              ValueListBlock->Values[Index],NULL);
  if (CurValueBlock != NULL)
    {
      *ValueBlock = CurValueBlock;
    }
  CmiReleaseBlock(RegistryFile, CurValueBlock);
  CmiReleaseBlock(RegistryFile, ValueListBlock);
  
  return  STATUS_SUCCESS;
}

NTSTATUS  
CmiAddValueToKey(IN PREGISTRY_FILE  RegistryFile,
                 IN PKEY_BLOCK  KeyBlock,
                 IN PCHAR  ValueNameBuf,
		 OUT PVALUE_BLOCK *pValueBlock,
		 OUT BLOCK_OFFSET *pVBOffset)
{
 NTSTATUS  Status;
 PVALUE_LIST_BLOCK  ValueListBlock, NewValueListBlock;
 BLOCK_OFFSET  VBOffset;
 BLOCK_OFFSET  VLBOffset;
 PVALUE_BLOCK NewValueBlock;

  Status = CmiAllocateValueBlock(RegistryFile,
                                 &NewValueBlock,
                                 &VBOffset,
                                 ValueNameBuf);
  *pVBOffset=VBOffset;
  if (!NT_SUCCESS(Status))
    {
      return  Status;
    }
  ValueListBlock = CmiGetBlock(RegistryFile, 
                               KeyBlock->ValuesOffset,NULL);
  if (ValueListBlock == NULL)
    {
      Status = CmiAllocateBlock(RegistryFile,
                                (PVOID) &ValueListBlock,
                                sizeof(BLOCK_OFFSET) * 3,
                                &VLBOffset);
      if (!NT_SUCCESS(Status))
        {
          CmiDestroyValueBlock(RegistryFile,
                               NewValueBlock,VBOffset);
          return  Status;
        }
      KeyBlock->ValuesOffset = VLBOffset;
    }
  else if ( KeyBlock->NumberOfValues 
		>= -(ValueListBlock->SubBlockSize-4)/sizeof(BLOCK_OFFSET))
    {
      Status = CmiAllocateBlock(RegistryFile,
                                (PVOID) &NewValueListBlock,
                                sizeof(BLOCK_OFFSET) *
                                  (KeyBlock->NumberOfValues + 
                                    REG_VALUE_LIST_BLOCK_MULTIPLE),&VLBOffset);
      if (!NT_SUCCESS(Status))
        {
          CmiDestroyValueBlock(RegistryFile,
                               NewValueBlock,VBOffset);
          return  Status;
        }
      RtlCopyMemory(&NewValueListBlock->Values[0],
                    &ValueListBlock->Values[0],
                    sizeof(BLOCK_OFFSET) * KeyBlock->NumberOfValues);
      CmiDestroyBlock(RegistryFile, ValueListBlock,KeyBlock->ValuesOffset);
      KeyBlock->ValuesOffset = VLBOffset;
      ValueListBlock = NewValueListBlock;
    }
  ValueListBlock->Values[KeyBlock->NumberOfValues] = VBOffset;
  KeyBlock->NumberOfValues++;
  CmiReleaseBlock(RegistryFile, ValueListBlock);
  CmiReleaseBlock(RegistryFile, NewValueBlock);
  *pValueBlock = NewValueBlock;

  return  STATUS_SUCCESS;
}

NTSTATUS  
CmiDeleteValueFromKey(IN PREGISTRY_FILE  RegistryFile,
                      IN PKEY_BLOCK  KeyBlock,
                      IN PCHAR  ValueName)
{
  ULONG  Idx;
  PVALUE_LIST_BLOCK  ValueListBlock;
  PVALUE_BLOCK  CurValueBlock;
  PHEAP_BLOCK pHeap;

  ValueListBlock = CmiGetBlock(RegistryFile, 
                               KeyBlock->ValuesOffset,NULL);
  if (ValueListBlock == 0)
    {
      return  STATUS_SUCCESS;
    }
  for (Idx = 0; Idx < KeyBlock->NumberOfValues; Idx++)
    {
      CurValueBlock = CmiGetBlock(RegistryFile,
                                  ValueListBlock->Values[Idx],&pHeap);
      if (CurValueBlock != NULL &&
          CurValueBlock->NameSize == strlen(ValueName) &&
          !memcmp(CurValueBlock->Name, ValueName,strlen(ValueName)))
        {
          if (KeyBlock->NumberOfValues - 1 < Idx)
            {
              RtlCopyMemory(&ValueListBlock->Values[Idx],
                            &ValueListBlock->Values[Idx + 1],
                            sizeof(BLOCK_OFFSET) * 
                              (KeyBlock->NumberOfValues - 1 - Idx));
            }
          else
            {
              RtlZeroMemory(&ValueListBlock->Values[Idx],
                            sizeof(BLOCK_OFFSET));
            }
          KeyBlock->NumberOfValues -= 1;
          CmiDestroyValueBlock(RegistryFile, CurValueBlock, ValueListBlock->Values[Idx]);
          /* update time of heap */
          ZwQuerySystemTime((PTIME) &pHeap->DateModified);

          break;
        }
      CmiReleaseBlock(RegistryFile, CurValueBlock);
    }

  CmiReleaseBlock(RegistryFile, ValueListBlock);
  
  return  STATUS_SUCCESS;
}

NTSTATUS
CmiAllocateHashTableBlock(IN PREGISTRY_FILE  RegistryFile,
                          OUT PHASH_TABLE_BLOCK  *HashBlock,
                          OUT BLOCK_OFFSET  *HBOffset,
                          IN ULONG  HashTableSize)
{
 NTSTATUS  Status;
 ULONG  NewHashSize;
 PHASH_TABLE_BLOCK  NewHashBlock;

  Status = STATUS_SUCCESS;
  *HashBlock = NULL;
  NewHashSize = sizeof(HASH_TABLE_BLOCK) + 
        (HashTableSize - 1) * sizeof(HASH_RECORD);
  Status = CmiAllocateBlock(RegistryFile,
                             (PVOID*)&NewHashBlock,
                             NewHashSize,HBOffset);
  if (NewHashBlock == NULL || !NT_SUCCESS(Status) )
  {
    Status = STATUS_INSUFFICIENT_RESOURCES;
  }
  else
  {
    NewHashBlock->SubBlockId = REG_HASH_TABLE_BLOCK_ID;
    NewHashBlock->HashTableSize = HashTableSize;
    *HashBlock = NewHashBlock;
  }

  return  Status;
}

PKEY_BLOCK  
CmiGetKeyFromHashByIndex(PREGISTRY_FILE RegistryFile,
                         PHASH_TABLE_BLOCK  HashBlock,
                         ULONG  Index)
{
  PKEY_BLOCK  KeyBlock;
  BLOCK_OFFSET KeyOffset;

  if( HashBlock == NULL)
    return NULL;
  if (RegistryFile->Filename == NULL)
    {
      KeyBlock = (PKEY_BLOCK) HashBlock->Table[Index].KeyOffset;
    }
  else
    {
      KeyOffset =  HashBlock->Table[Index].KeyOffset;
      KeyBlock = CmiGetBlock(RegistryFile,KeyOffset,NULL);
    }
  CmiLockBlock(RegistryFile, KeyBlock);

  return  KeyBlock;
}

NTSTATUS  
CmiAddKeyToHashTable(PREGISTRY_FILE  RegistryFile,
                     PHASH_TABLE_BLOCK  HashBlock,
                     PKEY_BLOCK  NewKeyBlock,
                     BLOCK_OFFSET  NKBOffset)
{
  ULONG i;

  for (i = 0; i < HashBlock->HashTableSize; i++)
    {
       if (HashBlock->Table[i].KeyOffset == 0)
         {
            HashBlock->Table[i].KeyOffset = NKBOffset;
            RtlCopyMemory(&HashBlock->Table[i].HashValue,
                          NewKeyBlock->Name,
                          4);
            return STATUS_SUCCESS;
         }
    }
  return STATUS_UNSUCCESSFUL;
}

NTSTATUS
CmiAllocateValueBlock(PREGISTRY_FILE  RegistryFile,
                      PVALUE_BLOCK  *ValueBlock,
                      BLOCK_OFFSET  *VBOffset,
                      IN PCHAR  ValueNameBuf)
{
  NTSTATUS  Status;
  ULONG  NewValueSize;
  PVALUE_BLOCK  NewValueBlock;

  Status = STATUS_SUCCESS;

      NewValueSize = sizeof(VALUE_BLOCK) + strlen(ValueNameBuf);
      Status = CmiAllocateBlock(RegistryFile,
                                    (PVOID*)&NewValueBlock,
                                    NewValueSize,VBOffset);
      if (NewValueBlock == NULL || !NT_SUCCESS(Status))
        {
          Status = STATUS_INSUFFICIENT_RESOURCES;
        }
      else
        {
          NewValueBlock->SubBlockId = REG_VALUE_BLOCK_ID;
          NewValueBlock->NameSize = strlen(ValueNameBuf);
          memcpy(NewValueBlock->Name, ValueNameBuf,strlen(ValueNameBuf));
          NewValueBlock->DataType = 0;
          NewValueBlock->DataSize = 0;
          NewValueBlock->DataOffset = 0xffffffff;
	  *ValueBlock = NewValueBlock;
        }

  return  Status;
}

NTSTATUS
CmiDestroyValueBlock(PREGISTRY_FILE  RegistryFile,
                     PVALUE_BLOCK  ValueBlock, BLOCK_OFFSET VBOffset)
{
 NTSTATUS  Status;
 PHEAP_BLOCK pHeap;
 PVOID pBlock;

  /* first, release datas : */
  if (ValueBlock->DataSize >0)
  {
    pBlock = CmiGetBlock(RegistryFile, ValueBlock->DataOffset,&pHeap);
    Status = CmiDestroyBlock(RegistryFile, pBlock, ValueBlock->DataOffset);
    if (!NT_SUCCESS(Status))
    {
      return  Status;
    }
    /* update time of heap */
    if(RegistryFile->Filename)
      ZwQuerySystemTime((PTIME) &pHeap->DateModified);
  }

  Status = CmiDestroyBlock(RegistryFile, ValueBlock, VBOffset);
  /* update time of heap */
  if(RegistryFile->Filename && CmiGetBlock(RegistryFile, VBOffset,&pHeap))
    ZwQuerySystemTime((PTIME) &pHeap->DateModified);
  return  Status;
}

NTSTATUS
CmiAddHeap(PREGISTRY_FILE  RegistryFile,PVOID *NewBlock,BLOCK_OFFSET *NewBlockOffset)
{
 PHEAP_BLOCK tmpHeap;
 PHEAP_BLOCK * tmpBlockList;
 PFREE_SUB_BLOCK tmpBlock;
  tmpHeap=ExAllocatePool(PagedPool, REG_BLOCK_SIZE);
  tmpHeap->BlockId = REG_HEAP_ID;
  tmpHeap->BlockOffset = RegistryFile->FileSize - REG_BLOCK_SIZE;
  RegistryFile->FileSize += REG_BLOCK_SIZE;
  tmpHeap->BlockSize = REG_BLOCK_SIZE;
  tmpHeap->Unused1 = 0;
  ZwQuerySystemTime((PTIME) &tmpHeap->DateModified);
  tmpHeap->Unused2 = 0;
  /* increase size of list of blocks */
  tmpBlockList=ExAllocatePool(NonPagedPool,
	   sizeof(PHEAP_BLOCK *) * (RegistryFile->BlockListSize +1));
  if (tmpBlockList == NULL)
  {
    KeBugCheck(0);
    return(STATUS_INSUFFICIENT_RESOURCES);
  }
  if(RegistryFile->BlockListSize > 0)
  {
    memcpy(tmpBlockList,RegistryFile->BlockList,
    	sizeof(PHEAP_BLOCK *)*(RegistryFile->BlockListSize ));
    ExFreePool(RegistryFile->BlockList);
  }
  RegistryFile->BlockList = tmpBlockList;
  RegistryFile->BlockList [RegistryFile->BlockListSize++] = tmpHeap;
  /* initialize a free block in this heap : */
  tmpBlock = (PFREE_SUB_BLOCK)((char *) tmpHeap + REG_HEAP_BLOCK_DATA_OFFSET);
  tmpBlock-> SubBlockSize =  (REG_BLOCK_SIZE - REG_HEAP_BLOCK_DATA_OFFSET) ;
  *NewBlock = (PVOID)tmpBlock;
  if (NewBlockOffset)
    *NewBlockOffset = tmpHeap->BlockOffset + REG_HEAP_BLOCK_DATA_OFFSET;
  /* FIXME : set first dword to block_offset of another free bloc */
  return STATUS_SUCCESS;
}

NTSTATUS
CmiAllocateBlock(PREGISTRY_FILE  RegistryFile,
                 PVOID  *Block,
                 LONG  BlockSize,
		 BLOCK_OFFSET * pBlockOffset)
{
 NTSTATUS  Status;
 PFREE_SUB_BLOCK NewBlock;
 PHEAP_BLOCK pHeap;

  Status = STATUS_SUCCESS;
  /* round to 16 bytes  multiple */
  BlockSize = (BlockSize + sizeof(DWORD) + 15) & 0xfffffff0;

  /*  Handle volatile files first  */
  if (RegistryFile->Filename == NULL)
  {
    NewBlock = ExAllocatePool(NonPagedPool, BlockSize);
    if (NewBlock == NULL)
    {
      Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
      RtlZeroMemory(NewBlock, BlockSize);
      NewBlock->SubBlockSize = BlockSize;
      CmiLockBlock(RegistryFile, NewBlock);
      *Block = NewBlock;
      if (pBlockOffset) *pBlockOffset = (BLOCK_OFFSET)NewBlock;
    }
  }
  else
  {
   int i;
    /* first search in free blocks */
    NewBlock = NULL;
    for (i=0 ; i<RegistryFile->FreeListSize ; i++)
    {
      if(RegistryFile->FreeList[i]->SubBlockSize >=BlockSize)
      {
         NewBlock = RegistryFile->FreeList[i];
         if(pBlockOffset)
	   *pBlockOffset = RegistryFile->FreeListOffset[i];
         /* update time of heap */
         if(RegistryFile->Filename
           && CmiGetBlock(RegistryFile, RegistryFile->FreeListOffset[i],&pHeap))
           ZwQuerySystemTime((PTIME) &pHeap->DateModified);
	 if( (i+1) <RegistryFile->FreeListSize)
	 {
           memmove( &RegistryFile->FreeList[i]
	       ,&RegistryFile->FreeList[i+1]
	       ,sizeof(RegistryFile->FreeList[0])
		*(RegistryFile->FreeListSize-i-1));
           memmove( &RegistryFile->FreeListOffset[i]
	       ,&RegistryFile->FreeListOffset[i+1]
	       ,sizeof(RegistryFile->FreeListOffset[0])
		*(RegistryFile->FreeListSize-i-1));
	 }
         RegistryFile->FreeListSize--;
         break;
      }
    }
    /* need to extend hive file : */
    if (NewBlock == NULL)
    {
      /*  add a new block : */
      Status = CmiAddHeap(RegistryFile, (PVOID *)&NewBlock , pBlockOffset);
    }
    if (NT_SUCCESS(Status))
    {
      *Block = NewBlock;
      /* split the block in two parts */
      if(NewBlock->SubBlockSize > BlockSize)
      {
	NewBlock = (PFREE_SUB_BLOCK)((char *)NewBlock+BlockSize);
	NewBlock->SubBlockSize=((PFREE_SUB_BLOCK) (*Block))->SubBlockSize-BlockSize;
	CmiAddFree(RegistryFile,NewBlock,*pBlockOffset+BlockSize);
      }
      else if(NewBlock->SubBlockSize < BlockSize)
	return STATUS_UNSUCCESSFUL;
      RtlZeroMemory(*Block, BlockSize);
      ((PFREE_SUB_BLOCK)(*Block)) ->SubBlockSize = - BlockSize;
      CmiLockBlock(RegistryFile, *Block);
    }
  }
  return  Status;
}

NTSTATUS
CmiDestroyBlock(PREGISTRY_FILE  RegistryFile,
                PVOID  Block,BLOCK_OFFSET Offset)
{
 NTSTATUS  Status;
 PHEAP_BLOCK pHeap;

  Status = STATUS_SUCCESS;

  if (RegistryFile->Filename == NULL)
  {
    CmiReleaseBlock(RegistryFile, Block);
    ExFreePool(Block);
  }
  else
  {  
   PFREE_SUB_BLOCK pFree = Block;
    if (pFree->SubBlockSize <0)
      pFree->SubBlockSize = -pFree->SubBlockSize;
    CmiAddFree(RegistryFile,Block,Offset);
    CmiReleaseBlock(RegistryFile, Block);
    /* update time of heap */
    if(RegistryFile->Filename && CmiGetBlock(RegistryFile, Offset,&pHeap))
      ZwQuerySystemTime((PTIME) &pHeap->DateModified);
      /* FIXME : set first dword to block_offset of another free bloc ? */
      /* FIXME : concatenate with previous and next block if free */
  }

  return  Status;
}

NTSTATUS
CmiAddFree(PREGISTRY_FILE  RegistryFile,
		PFREE_SUB_BLOCK FreeBlock,BLOCK_OFFSET FreeOffset)
{
 PFREE_SUB_BLOCK *tmpList;
 BLOCK_OFFSET *tmpListOffset;
 int minInd,maxInd,medInd;
  if( (RegistryFile->FreeListSize+1) > RegistryFile->FreeListMax)
  {
    tmpList=ExAllocatePool(PagedPool
		,sizeof(PFREE_SUB_BLOCK)*(RegistryFile->FreeListMax+32));
    if (tmpList == NULL)
  	return STATUS_INSUFFICIENT_RESOURCES;
    tmpListOffset=ExAllocatePool(PagedPool
		,sizeof(BLOCK_OFFSET *)*(RegistryFile->FreeListMax+32));
    if (tmpListOffset == NULL)
  	return STATUS_INSUFFICIENT_RESOURCES;
    if (RegistryFile->FreeListMax)
    {
	memcpy(tmpList,RegistryFile->FreeList
		,sizeof(PFREE_SUB_BLOCK)*(RegistryFile->FreeListMax));
	memcpy(tmpListOffset,RegistryFile->FreeListOffset
		,sizeof(BLOCK_OFFSET *)*(RegistryFile->FreeListMax));
	ExFreePool(RegistryFile->FreeList);
	ExFreePool(RegistryFile->FreeListOffset);
    }
    RegistryFile->FreeList = tmpList;
    RegistryFile->FreeListOffset = tmpListOffset;
    RegistryFile->FreeListMax +=32;
  }
  /* add new offset to free list, maintening list in ascending order */
  if (  RegistryFile->FreeListSize==0
     || RegistryFile->FreeListOffset[RegistryFile->FreeListSize-1] < FreeOffset)
  {
    /* add to end of list : */
    RegistryFile->FreeList[RegistryFile->FreeListSize] = FreeBlock;
    RegistryFile->FreeListOffset[RegistryFile->FreeListSize ++] = FreeOffset;
  }
  else if (RegistryFile->FreeListOffset[0] > FreeOffset)
  {
    /* add to begin of list : */
    memmove( &RegistryFile->FreeList[1],&RegistryFile->FreeList[0]
	   ,sizeof(RegistryFile->FreeList[0])*RegistryFile->FreeListSize);
    memmove( &RegistryFile->FreeListOffset[1],&RegistryFile->FreeListOffset[0]
	   ,sizeof(RegistryFile->FreeListOffset[0])*RegistryFile->FreeListSize);
    RegistryFile->FreeList[0] = FreeBlock;
    RegistryFile->FreeListOffset[0] = FreeOffset;
    RegistryFile->FreeListSize ++;
  }
  else
  {
    /* search where to insert : */
    minInd=0;
    maxInd=RegistryFile->FreeListSize-1;
    while( (maxInd-minInd) >1)
    {
      medInd=(minInd+maxInd)/2;
      if (RegistryFile->FreeListOffset[medInd] > FreeOffset)
        maxInd=medInd;
      else
        minInd=medInd;
    }
    /* insert before maxInd : */
    memmove( &RegistryFile->FreeList[maxInd+1],&RegistryFile->FreeList[maxInd]
	   ,sizeof(RegistryFile->FreeList[0])
		*(RegistryFile->FreeListSize-minInd));
    memmove(  &RegistryFile->FreeListOffset[maxInd+1]
	    , &RegistryFile->FreeListOffset[maxInd]
	    , sizeof(RegistryFile->FreeListOffset[0])
		*(RegistryFile->FreeListSize-minInd));
    RegistryFile->FreeList[maxInd] = FreeBlock;
    RegistryFile->FreeListOffset[maxInd] = FreeOffset;
    RegistryFile->FreeListSize ++;
  }
  return STATUS_SUCCESS;
}

PVOID
CmiGetBlock(PREGISTRY_FILE  RegistryFile,
            BLOCK_OFFSET  BlockOffset,
	    OUT PHEAP_BLOCK * ppHeap)
{
  if( BlockOffset == 0 || BlockOffset == -1) return NULL;

  if (RegistryFile->Filename == NULL)
  {
      return (PVOID)BlockOffset;
  }
  else
  {
   PHEAP_BLOCK pHeap;
    pHeap = RegistryFile->BlockList[BlockOffset/4096];
    if(ppHeap) *ppHeap = pHeap;
    return ((char *)pHeap
	+(BlockOffset - pHeap->BlockOffset));
  }
}

void 
CmiLockBlock(PREGISTRY_FILE  RegistryFile,
             PVOID  Block)
{
  if (RegistryFile->Filename != NULL)
    {
      /* FIXME : implement */
    }
}

void 
CmiReleaseBlock(PREGISTRY_FILE  RegistryFile,
               PVOID  Block)
{
  if (RegistryFile->Filename != NULL)
    {
      /* FIXME : implement */
    }
}
