/* $Id: registry.c,v 1.21 1999/11/24 11:51:46 dwelch Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/cm/registry.c
 * PURPOSE:         Registry functions
 * PROGRAMMERS:     Rex Jolliff
 *                  Matt Pyne
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

#undef WIN32_LEAN_AND_MEAN
#include <ddk/ntddk.h>
#include <internal/ob.h>
#include <wchar.h>

//#define NDEBUG
#include <internal/debug.h>

// #define  PROTO_REG  1  /* Comment out to disable */

/*  -----------------------------------------------------  Typedefs  */

#if PROTO_REG

#define  REG_BLOCK_SIZE  4096
#define  REG_HEAP_BLOCK_DATA_OFFSET  32
#define  REG_INIT_BLOCK_LIST_SIZE  32
#define  REG_INIT_HASH_TABLE_SIZE  32
#define  REG_EXTEND_HASH_TABLE_SIZE  32
#define  REG_VALUE_LIST_BLOCK_MULTIPLE  32
#define  REG_KEY_BLOCK_ID    0x6b6e
#define  REG_HASH_TABLE_BLOCK_ID  0x666c
#define  REG_VALUE_BLOCK_ID  0x6b76
#define  REG_KEY_BLOCK_TYPE  0x20
#define  REG_ROOT_KEY_BLOCK_TYPE  0x2c

#define  REG_ROOT_KEY_NAME  L"\\Registry"
#define  SYSTEM_REG_FILE  L"\\SystemDir\\System32\\Config\\SYSTEM.DAT"

typedef DWORD  BLOCK_OFFSET;

typedef struct _HEADER_BLOCK
{
  DWORD  BlockId;
  DWORD  Unused1;
  DWORD  Unused2;
  LARGE_INTEGER  DateModified;
  DWORD  Unused3;
  DWORD  Unused4;
  DWORD  Unused5;
  DWORD  Unused6;
  BLOCK_OFFSET  RootKeyBlock;
  DWORD  BlockSize;
  DWORD  Unused7;
  DWORD  Unused8[115];
  DWORD  Checksum;
} HEADER_BLOCK, *PHEADER_BLOCK;

typedef struct _HEAP_BLOCK
{
  DWORD  BlockId;
  BLOCK_OFFSET  PreviousHeapBlock;
  BLOCK_OFFSET  NextHeapBlock;
  DWORD  BlockSize;
} HEAP_BLOCK, *PHEAP_BLOCK;

typedef struct _FREE_SUB_BLOCK
{
  DWORD  SubBlockSize;
} FREE_SUB_BLOCK, *PFREE_SUB_BLOCK;

typedef struct _KEY_BLOCK
{
  WORD  SubBlockId;
  WORD  Type;
  LARGE_INTEGER  LastWriteTime;
  BLOCK_OFFSET  ParentKeyOffset;
  DWORD  NumberOfSubKeys;
  BLOCK_OFFSET  HashTableOffset;
  DWORD  NumberOfValues;
  BLOCK_OFFSET  ValuesOffset;
  BLOCK_OFFSET  SecurityKeyOffset;
  BLOCK_OFFSET  ClassNameOffset;
  DWORD  Unused1;
  DWORD  NameSize;
  DWORD  ClassSize;
  WCHAR  Name[1];
} KEY_BLOCK, *PKEY_BLOCK;

typedef struct _HASH_RECORD
{
  BLOCK_OFFSET  KeyOffset;
  ULONG  HashValue;
} HASH_RECORD, *PHASH_RECORD;

typedef struct _HASH_TABLE_BLOCK
{
  WORD  SubBlockId;
  WORD  HashTableSize;
  HASH_RECORD  Table[1];
} HASH_TABLE_BLOCK, *PHASH_TABLE_BLOCK;

typedef struct _VALUE_LIST_BLOCK
{
  BLOCK_OFFSET  Values[1];
} VALUE_LIST_BLOCK, *PVALUE_LIST_BLOCK;

typedef struct _VALUE_BLOCK
{
  WORD  SubBlockId;
  WORD  NameSize;
  DWORD  DataSize;
  BLOCK_OFFSET  DataOffset;
  DWORD  DataType;
  WORD  Flags;
  WORD  Unused1;
  WCHAR  Name[1];
} VALUE_BLOCK, *PVALUE_BLOCK;

typedef struct _IN_MEMORY_BLOCK
{
  DWORD  FileOffset;
  DWORD  BlockSize;
  PVOID *Data;
} IN_MEMORY_BLOCK, *PIN_MEMORY_BLOCK;

typedef struct _REGISTRY_FILE
{
  PWSTR  Filename;
  HANDLE  FileHandle;
  PHEADER_BLOCK  HeaderBlock;
  ULONG  NumberOfBlocks;
  ULONG  BlockListSize;
  PIN_MEMORY_BLOCK  *BlockList;

  NTSTATUS  (*Extend)(ULONG NewSize);
  PVOID  (*Flush)(VOID);
} REGISTRY_FILE, *PREGISTRY_FILE;

/*  Type defining the Object Manager Key Object  */
typedef struct _KEY_OBJECT
{
  CSHORT  Type;
  CSHORT  Size;
  
  ULONG  Flags;
  WCHAR  *Name;
  PREGISTRY_FILE  RegistryFile;
  PKEY_BLOCK  KeyBlock;
  struct _KEY_OBJECT  *NextKey;
} KEY_OBJECT, *PKEY_OBJECT;

#define  KO_MARKED_FOR_DELETE  0x00000001

/*  -------------------------------------------------  File Statics  */

static POBJECT_TYPE  CmiKeyType = NULL;
static PKEY_OBJECT  CmiKeyList = NULL;
static KSPIN_LOCK  CmiKeyListLock;
static PREGISTRY_FILE  CmiVolatileFile = NULL;
static PREGISTRY_FILE  CmiSystemFile = NULL;

/*  -----------------------------------------  Forward Declarations  */

static PVOID  CmiObjectParse(PVOID  ParsedObject, PWSTR  *Path);
static VOID  CmiObjectDelete(PVOID  DeletedObject);
static NTSTATUS  CmiBuildKeyPath(PWSTR  *KeyPath, 
                                 POBJECT_ATTRIBUTES  ObjectAttributes);
static VOID  CmiAddKeyToList(PKEY_OBJECT  NewKey);
static VOID  CmiRemoveKeyFromList(PKEY_OBJECT  NewKey);
static PKEY_OBJECT  CmiScanKeyList(PWSTR  KeyNameBuf);
static PREGISTRY_FILE  CmiCreateRegistry(PWSTR  Filename);
static NTSTATUS  CmiCreateKey(IN PREGISTRY_FILE  RegistryFile,
                              IN PWSTR  KeyNameBuf,
                              OUT PKEY_BLOCK  *KeyBlock,
                              IN ACCESS_MASK DesiredAccess,
                              IN ULONG TitleIndex,
                              IN PUNICODE_STRING Class, 
                              IN ULONG CreateOptions, 
                              OUT PULONG Disposition);
static NTSTATUS  CmiFindKey(IN PREGISTRY_FILE  RegistryFile,
                            IN PWSTR  KeyNameBuf,
                            OUT PKEY_BLOCK  *KeyBlock,
                            IN ACCESS_MASK DesiredAccess,
                            IN ULONG TitleIndex,
                            IN PUNICODE_STRING Class);
static ULONG  CmiGetMaxNameLength(PREGISTRY_FILE  RegistryFile,
                                  PKEY_BLOCK  KeyBlock);
static ULONG  CmiGetMaxClassLength(PREGISTRY_FILE  RegistryFile,
                                   PKEY_BLOCK  KeyBlock);
static ULONG  CmiGetMaxValueNameLength(PREGISTRY_FILE  RegistryFile,
                                       PKEY_BLOCK  KeyBlock);
static ULONG  CmiGetMaxValueDataLength(PREGISTRY_FILE  RegistryFile,
                                       PKEY_BLOCK  KeyBlock);
static NTSTATUS  CmiScanForSubKey(IN PREGISTRY_FILE  RegistryFile, 
                                  IN PKEY_BLOCK  KeyBlock, 
                                  OUT PKEY_BLOCK  *SubKeyBlock,
                                  IN PWSTR  KeyName,
                                  IN ACCESS_MASK  DesiredAccess);
static NTSTATUS  CmiAddSubKey(IN PREGISTRY_FILE  RegistryFile, 
                              IN PKEY_BLOCK  CurKeyBlock,
                              OUT PKEY_BLOCK  *SubKeyBlock,
                              IN PWSTR  NewSubKeyName,
                              IN ULONG  TitleIndex,
                              IN PWSTR  Class, 
                              IN ULONG  CreateOptions);
static NTSTATUS  CmiScanKeyForValue(IN PREGISTRY_FILE  RegistryFile,
                                    IN PKEY_BLOCK  KeyBlock,
                                    IN PWSTR  ValueName,
                                    OUT PVALUE_BLOCK  *ValueBlock);
static NTSTATUS  CmiAddValueToKey(IN PREGISTRY_FILE  RegistryFile,
                                  IN PKEY_BLOCK  KeyBlock,
                                  IN PWSTR  ValueNameBuf,
                                  IN ULONG  Type, 
                                  IN PVOID  Data,
                                  IN ULONG  DataSize);
static NTSTATUS  CmiDeleteValueFromKey(IN PREGISTRY_FILE  RegistryFile,
                                       IN PKEY_BLOCK  KeyBlock,
                                       IN PWSTR  ValueName);
static NTSTATUS  CmiAllocateKeyBlock(IN PREGISTRY_FILE  RegistryFile,
                                     OUT PKEY_BLOCK  *KeyBlock,
                                     IN PWSTR  NewSubKeyName,
                                     IN ULONG  TitleIndex,
                                     IN PWSTR  Class,
                                    IN ULONG  CreateOptions);
static PKEY_BLOCK  CmiGetKeyBlock(PREGISTRY_FILE  RegistryFile,
                                  BLOCK_OFFSET  KeyBlockOffset);
static NTSTATUS  CmiDestroyKeyBlock(PREGISTRY_FILE  RegistryFile,
                                    PKEY_BLOCK  KeyBlock);
static NTSTATUS  CmiAllocateHashTableBlock(IN PREGISTRY_FILE  RegistryFile,
                                           OUT PHASH_TABLE_BLOCK  *HashBlock,
                                           IN ULONG  HashTableSize);
static PHASH_TABLE_BLOCK  CmiGetHashTableBlock(PREGISTRY_FILE  RegistryFile,
                                               BLOCK_OFFSET  HashBlockOffset);
static PKEY_BLOCK  CmiGetKeyFromHashByIndex(PREGISTRY_FILE RegistryFile,
                                            PHASH_TABLE_BLOCK  HashBlock,
                                            ULONG  Index);
static NTSTATUS  CmiAddKeyToHashTable(PREGISTRY_FILE  RegistryFile,
                                      PHASH_TABLE_BLOCK  HashBlock,
                                      PKEY_BLOCK  NewKeyBlock);
static NTSTATUS  CmiDestroyHashTableBlock(PREGISTRY_FILE  RegistryFile,
                                          PHASH_TABLE_BLOCK  HashBlock);
static NTSTATUS  CmiAllocateValueBlock(IN PREGISTRY_FILE  RegistryFile,
                                       OUT PVALUE_BLOCK  *ValueBlock,
                                       IN PWSTR  ValueNameBuf,
                                       IN ULONG  Type, 
                                       IN PVOID  Data,
                                       IN ULONG  DataSize);
static NTSTATUS  CmiReplaceValueData(IN PREGISTRY_FILE  RegistryFile,
                                     IN PVALUE_BLOCK  ValueBlock,
                                     IN ULONG  Type, 
                                     IN PVOID  Data,
                                     IN ULONG  DataSize);
static NTSTATUS  CmiDestroyValueBlock(PREGISTRY_FILE  RegistryFile,
                                      PVALUE_BLOCK  ValueBlock);
static NTSTATUS  CmiAllocateBlock(PREGISTRY_FILE  RegistryFile,
                                  PVOID  *Block,
                                  ULONG  BlockSize);
static NTSTATUS  CmiDestroyBlock(PREGISTRY_FILE  RegistryFile,
                                 PVOID  Block);
static PVOID  CmiGetBlock(PREGISTRY_FILE  RegistryFile,
                          BLOCK_OFFSET  BlockOffset);
static BLOCK_OFFSET  CmiGetBlockOffset(PREGISTRY_FILE  RegistryFile,
                                       PVOID  Block);
static VOID CmiLockBlock(PREGISTRY_FILE  RegistryFile,
                         PVOID  Block);
static VOID  CmiReleaseBlock(PREGISTRY_FILE  RegistryFile,
                             PVOID  Block);

#endif

/*  ---------------------------------------------  Public Interface  */

VOID
CmInitializeRegistry(VOID)
{
#if PROTO_REG
  NTSTATUS  Status;
  HANDLE  RootKeyHandle;
  UNICODE_STRING  RootKeyName;
  OBJECT_ATTRIBUTES  ObjectAttributes;
  PKEY_BLOCK  KeyBlock;
  
  /*  Initialize the Key object type  */
  CmiKeyType = ExAllocatePool(NonPagedPool, sizeof(OBJECT_TYPE));
  CmiKeyType->TotalObjects = 0;
  CmiKeyType->TotalHandles = 0;
  CmiKeyType->MaxObjects = ULONG_MAX;
  CmiKeyType->MaxHandles = ULONG_MAX;
  CmiKeyType->PagedPoolCharge = 0;
  CmiKeyType->NonpagedPoolCharge = sizeof(KEY_OBJECT);
  CmiKeyType->Dump = NULL;
  CmiKeyType->Open = NULL;
  CmiKeyType->Close = NULL;
  CmiKeyType->Delete = CmiObjectDelete;
  CmiKeyType->Parse = CmiObjectParse;
  CmiKeyType->Security = NULL;
  CmiKeyType->QueryName = NULL;
  CmiKeyType->OkayToClose = NULL;
  RtlInitUnicodeString(&CmiKeyType->TypeName, L"Key");

  /*  Build the Root Key Object  */
  /*  FIXME: This should be split into two objects, 1 system and 1 user  */
  RtlInitUnicodeString(&RootKeyName, REG_ROOT_KEY_NAME);
  InitializeObjectAttributes(&ObjectAttributes, &RootKeyName, 0, NULL, NULL);
  ObCreateObject(&RootKeyHandle,
                 STANDARD_RIGHTS_REQUIRED,
                 &ObjectAttributes,
                 CmiKeyType);

  KeInitializeSpinLock(&CmiKeyListLock);

  /*  Build volitile registry store  */
CHECKPOINT;
  CmiVolatileFile = CmiCreateRegistry(NULL);

  /*  Build system registry store  */
CHECKPOINT;
  CmiSystemFile = NULL; // CmiCreateRegistry(SYSTEM_REG_FILE);

  /*  Create initial predefined symbolic links  */
  /* HKEY_LOCAL_MACHINE  */
CHECKPOINT;
  Status = CmiCreateKey(CmiVolatileFile,
                        L"Machine",
                        &KeyBlock,
                        KEY_ALL_ACCESS,
                        0, 
                        NULL, 
                        REG_OPTION_VOLATILE, 
                        0);
  if (!NT_SUCCESS(Status))
    {
      return;
    }
CHECKPOINT;
  CmiReleaseBlock(CmiVolatileFile, KeyBlock);
  
  /* HKEY_USERS  */
CHECKPOINT;
  Status = CmiCreateKey(CmiVolatileFile,
                        L"Users",
                        &KeyBlock,
                        KEY_ALL_ACCESS,
                        0, 
                        NULL, 
                        REG_OPTION_VOLATILE, 
                        0);
  if (!NT_SUCCESS(Status))
    {
      return;
    }
CHECKPOINT;
  CmiReleaseBlock(CmiVolatileFile, KeyBlock);

  /* FIXME: create remaining structure needed for default handles  */
  /* FIXME: load volatile registry data from ROSDTECT  */

#endif
}

VOID 
CmImportHive(PCHAR  Chunk)
{
  /*  FIXME: implemement this  */
  return; 
}

NTSTATUS 
STDCALL
NtCreateKey (
	OUT	PHANDLE			KeyHandle,
	IN	ACCESS_MASK		DesiredAccess,
	IN	POBJECT_ATTRIBUTES	ObjectAttributes, 
	IN	ULONG			TitleIndex,
	IN	PUNICODE_STRING		Class,
	IN	ULONG			CreateOptions,
	OUT	PULONG			Disposition
	)
{
#if PROTO_REG
  PWSTR  KeyNameBuf;
  NTSTATUS  Status;
  PKEY_OBJECT  CurKey, NewKey;
  PREGISTRY_FILE  FileToUse;
  PKEY_BLOCK  KeyBlock;

  assert(ObjectAttributes != NULL);

  FileToUse = (CreateOptions & REG_OPTION_VOLATILE) ? 
    CmiVolatileFile : CmiSystemFile;
  
  /*  Construct the complete registry relative pathname  */
  Status = CmiBuildKeyPath(&KeyNameBuf, ObjectAttributes);
  if (!NT_SUCCESS(Status))
    {
      return  Status;
    }

  /*  Scan the key list to see if key already open  */
  CurKey = CmiScanKeyList(KeyNameBuf);
  if (CurKey != NULL)
    {
      /*  Unmark the key if the key has been marked for Delete  */
      if (CurKey->Flags & KO_MARKED_FOR_DELETE)
        {
          CurKey->Flags &= ~KO_MARKED_FOR_DELETE;
        }
      
      /*  If so, return a reference to it  */
      Status = ObCreateHandle(PsGetCurrentProcess(),
                              CurKey,
                              DesiredAccess,
                              FALSE,
                              KeyHandle);
      ExFreePool(KeyNameBuf);

      return  Status;
    }

  /*  Create or open the key in the registry file  */
  Status = CmiCreateKey(FileToUse,
                        KeyNameBuf,
                        &KeyBlock,
                        DesiredAccess,
                        TitleIndex, 
                        Class, 
                        CreateOptions, 
                        Disposition);
  if (!NT_SUCCESS(Status))
    {
      ExFreePool(KeyNameBuf);
      
      return  Status;
    }

  /*  Create new key object and put into linked list  */
  NewKey = ObCreateObject(KeyHandle, 
                          DesiredAccess, 
                          NULL, 
                          CmiKeyType);
  if (NewKey == NULL)
    {
      return  STATUS_UNSUCCESSFUL;
    }
  NewKey->Flags = 0;
  NewKey->Name = KeyNameBuf;
  NewKey->KeyBlock = KeyBlock;
  NewKey->RegistryFile = FileToUse;
  CmiAddKeyToList(NewKey);
  Status = ObCreateHandle(PsGetCurrentProcess(),
                          CurKey,
                          DesiredAccess,
                          FALSE,
                          KeyHandle);

  return  Status;
#else
  UNIMPLEMENTED;
#endif
}


NTSTATUS 
STDCALL
NtDeleteKey (
	IN	HANDLE	KeyHandle
	)
{
#ifdef PROTO_REG
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
  ObDeleteHandle(KeyHandle);
  /* FIXME: I think that ObDeleteHandle should dereference the object  */
  ObDereferenceObject(KeyObject);

  return  STATUS_SUCCESS;
#else
  UNIMPLEMENTED;
#endif
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
#ifdef PROTO_REG
  NTSTATUS  Status;
  PKEY_OBJECT  KeyObject;
  PREGISTRY_FILE  RegistryFile;
  PKEY_BLOCK  KeyBlock, SubKeyBlock;
  PHASH_TABLE_BLOCK  HashTableBlock;
  PKEY_BASIC_INFORMATION  BasicInformation;
  PKEY_NODE_INFORMATION  NodeInformation;
  PKEY_FULL_INFORMATION  FullInformation;
    
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
  HashTableBlock = CmiGetHashTableBlock(RegistryFile, KeyBlock->HashTableOffset);
  SubKeyBlock = CmiGetKeyFromHashByIndex(RegistryFile, 
                                         HashTableBlock, 
                                         Index);
  if (SubKeyBlock == NULL)
    {
      return  STATUS_NO_MORE_ENTRIES;
    }

  Status = STATUS_SUCCESS;
  switch (KeyInformationClass)
    {
    case KeyBasicInformation:
      /*  Check size of buffer  */
      if (Length < sizeof(KEY_BASIC_INFORMATION) + 
          SubKeyBlock->NameSize * sizeof(WCHAR))
        {
          Status = STATUS_BUFFER_OVERFLOW;
        }
      else
        {
          /*  Fill buffer with requested info  */
          BasicInformation = (PKEY_BASIC_INFORMATION) KeyInformation;
          BasicInformation->LastWriteTime = SubKeyBlock->LastWriteTime;
          BasicInformation->TitleIndex = Index;
          BasicInformation->NameLength = SubKeyBlock->NameSize;
          wcsncpy(BasicInformation->Name, 
                  SubKeyBlock->Name, 
                  SubKeyBlock->NameSize);
          BasicInformation->Name[SubKeyBlock->NameSize] = 0;
          *ResultLength = sizeof(KEY_BASIC_INFORMATION) + 
            SubKeyBlock->NameSize * sizeof(WCHAR);
        }
      break;
      
    case KeyNodeInformation:
      /*  Check size of buffer  */
      if (Length < sizeof(KEY_NODE_INFORMATION) +
          SubKeyBlock->NameSize * sizeof(WCHAR) +
          (SubKeyBlock->ClassSize + 1) * sizeof(WCHAR))
        {
          Status = STATUS_BUFFER_OVERFLOW;
        }
      else
        {
          /*  Fill buffer with requested info  */
          NodeInformation = (PKEY_NODE_INFORMATION) KeyInformation;
          NodeInformation->LastWriteTime = SubKeyBlock->LastWriteTime;
          NodeInformation->TitleIndex = Index;
          NodeInformation->ClassOffset = sizeof(KEY_NODE_INFORMATION) + 
            SubKeyBlock->NameSize * sizeof(WCHAR);
          NodeInformation->ClassLength = SubKeyBlock->ClassSize;
          NodeInformation->NameLength = SubKeyBlock->NameSize;
          wcsncpy(NodeInformation->Name, 
                  SubKeyBlock->Name, 
                  SubKeyBlock->NameSize);
          NodeInformation->Name[SubKeyBlock->NameSize] = 0;
          if (SubKeyBlock->ClassSize != 0)
            {
              wcsncpy(NodeInformation->Name + SubKeyBlock->NameSize + 1,
                      &SubKeyBlock->Name[SubKeyBlock->NameSize + 1],
                      SubKeyBlock->ClassSize);
              NodeInformation->
                Name[SubKeyBlock->NameSize + 1 + SubKeyBlock->ClassSize] = 0;
            }
          *ResultLength = sizeof(KEY_NODE_INFORMATION) +
            SubKeyBlock->NameSize * sizeof(WCHAR) +
            (SubKeyBlock->ClassSize + 1) * sizeof(WCHAR);
        }
      break;
      
    case KeyFullInformation:
      /* FIXME: check size of buffer  */
      if (Length < sizeof(KEY_FULL_INFORMATION) +
          SubKeyBlock->ClassSize * sizeof(WCHAR))
        {
          Status = STATUS_BUFFER_OVERFLOW;
        }
      else
        {
          /* FIXME: fill buffer with requested info  */
          FullInformation = (PKEY_FULL_INFORMATION) KeyInformation;
          FullInformation->LastWriteTime = SubKeyBlock->LastWriteTime;
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
          wcsncpy(FullInformation->Class,
                  &SubKeyBlock->Name[SubKeyBlock->NameSize + 1],
                  SubKeyBlock->ClassSize);
          FullInformation->Class[SubKeyBlock->ClassSize] = 0;
          *ResultLength = sizeof(KEY_FULL_INFORMATION) +
            SubKeyBlock->ClassSize * sizeof(WCHAR);
        }
      break;
    }
  CmiReleaseBlock(RegistryFile, SubKeyBlock);

  return  Status;
#else
  UNIMPLEMENTED;
#endif
}


NTSTATUS 
STDCALL
NtEnumerateValueKey (
	IN	HANDLE				KeyHandle,
	IN	ULONG				Index,
	IN	KEY_VALUE_INFORMATION_CLASS	KeyInformationClass,
	OUT	PVOID				KeyInformation,
	IN	ULONG				Length,
	OUT	PULONG				ResultLength
	)
{
#ifdef PROTO_REG
  UNIMPLEMENTED;  
#else
  UNIMPLEMENTED;
#endif
}


NTSTATUS 
STDCALL
NtFlushKey (
	IN	HANDLE	KeyHandle
	)
{
#ifdef PROTO_REG
  return  STATUS_SUCCESS;
#else
  UNIMPLEMENTED;
#endif
}


NTSTATUS 
STDCALL
NtOpenKey (
	OUT	PHANDLE			KeyHandle, 
	IN	ACCESS_MASK		DesiredAccess,
	IN	POBJECT_ATTRIBUTES	ObjectAttributes
	)
{
#ifdef PROTO_REG
  NTSTATUS  Status;
  PWSTR  KeyNameBuf;
  PREGISTRY_FILE  FileToUse;
  PKEY_BLOCK  KeyBlock;
  PKEY_OBJECT  CurKey, NewKey;
  
  /*  Construct the complete registry relative pathname  */
  Status = CmiBuildKeyPath(&KeyNameBuf, ObjectAttributes);
  if (!NT_SUCCESS(Status))
    {
      return Status;
    }

  /*  Scan the key list to see if key already open  */
  CurKey = CmiScanKeyList(KeyNameBuf);
  if (CurKey != NULL)
    {
      /*  Fail if the key has been deleted  */
      if (CurKey->Flags & KO_MARKED_FOR_DELETE)
        {
          ExFreePool(KeyNameBuf);
          
          return STATUS_UNSUCCESSFUL;
        }
      
      /*  If so, return a reference to it  */
      Status = ObCreateHandle(PsGetCurrentProcess(),
                              CurKey,
                              DesiredAccess,
                              FALSE,
                              KeyHandle);
      ExFreePool(KeyNameBuf);

      return  Status;
    }

  /*  Open the key in the registry file  */
  FileToUse = CmiSystemFile;
  Status = CmiFindKey(FileToUse,
                      KeyNameBuf,
                      &KeyBlock,
                      DesiredAccess,
                      0,
                      NULL);
  if (!NT_SUCCESS(Status))
    {
      FileToUse = CmiVolatileFile;
      Status = CmiFindKey(FileToUse,
                          KeyNameBuf,
                          &KeyBlock,
                          DesiredAccess,
                          0,
                          NULL);
      if (!NT_SUCCESS(Status))
        {
          ExFreePool(KeyNameBuf);
      
          return  Status;
        }
    }

  /*  Create new key object and put into linked list  */
  NewKey = ObCreateObject(KeyHandle, 
                          DesiredAccess, 
                          NULL, 
                          CmiKeyType);
  if (NewKey == NULL)
    {
      return  STATUS_UNSUCCESSFUL;
    }
  NewKey->Flags = 0;
  NewKey->Name = KeyNameBuf;
  NewKey->RegistryFile = FileToUse;
  NewKey->KeyBlock = KeyBlock;
  CmiAddKeyToList(NewKey);
  Status = ObCreateHandle(PsGetCurrentProcess(),
                          CurKey,
                          DesiredAccess,
                          FALSE,
                          KeyHandle);
  
  return  Status;
#else
  UNIMPLEMENTED;
#endif
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
#ifdef PROTO_REG
  NTSTATUS  Status;
  PKEY_OBJECT  KeyObject;
  PREGISTRY_FILE  RegistryFile;
  PKEY_BLOCK  KeyBlock;
  PKEY_BASIC_INFORMATION  BasicInformation;
  PKEY_NODE_INFORMATION  NodeInformation;
  PKEY_FULL_INFORMATION  FullInformation;
    
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
          KeyBlock->NameSize * sizeof(WCHAR))
        {
          Status = STATUS_BUFFER_OVERFLOW;
        }
      else
        {
          /*  Fill buffer with requested info  */
          BasicInformation = (PKEY_BASIC_INFORMATION) KeyInformation;
          BasicInformation->LastWriteTime = KeyBlock->LastWriteTime;
          BasicInformation->TitleIndex = 0;
          BasicInformation->NameLength = KeyBlock->NameSize;
          wcsncpy(BasicInformation->Name, 
                  KeyBlock->Name, 
                  KeyBlock->NameSize);
          BasicInformation->Name[KeyBlock->NameSize] = 0;
          *ResultLength = sizeof(KEY_BASIC_INFORMATION) + 
            KeyBlock->NameSize * sizeof(WCHAR);
        }
      break;
      
    case KeyNodeInformation:
      /*  Check size of buffer  */
      if (Length < sizeof(KEY_NODE_INFORMATION) +
          KeyBlock->NameSize * sizeof(WCHAR) +
          (KeyBlock->ClassSize + 1) * sizeof(WCHAR))
        {
          Status = STATUS_BUFFER_OVERFLOW;
        }
      else
        {
          /*  Fill buffer with requested info  */
          NodeInformation = (PKEY_NODE_INFORMATION) KeyInformation;
          NodeInformation->LastWriteTime = KeyBlock->LastWriteTime;
          NodeInformation->TitleIndex = 0;
          NodeInformation->ClassOffset = sizeof(KEY_NODE_INFORMATION) + 
            KeyBlock->NameSize * sizeof(WCHAR);
          NodeInformation->ClassLength = KeyBlock->ClassSize;
          NodeInformation->NameLength = KeyBlock->NameSize;
          wcsncpy(NodeInformation->Name, 
                  KeyBlock->Name, 
                  KeyBlock->NameSize);
          NodeInformation->Name[KeyBlock->NameSize] = 0;
          if (KeyBlock->ClassSize != 0)
            {
              wcsncpy(NodeInformation->Name + KeyBlock->NameSize + 1,
                      &KeyBlock->Name[KeyBlock->NameSize + 1],
                      KeyBlock->ClassSize);
              NodeInformation->
                Name[KeyBlock->NameSize + 1 + KeyBlock->ClassSize] = 0;
            }
          *ResultLength = sizeof(KEY_NODE_INFORMATION) +
            KeyBlock->NameSize * sizeof(WCHAR) +
            (KeyBlock->ClassSize + 1) * sizeof(WCHAR);
        }
      break;
      
    case KeyFullInformation:
      /*  Check size of buffer  */
      if (Length < sizeof(KEY_FULL_INFORMATION) +
          KeyBlock->ClassSize * sizeof(WCHAR))
        {
          Status = STATUS_BUFFER_OVERFLOW;
        }
      else
        {
          /*  Fill buffer with requested info  */
          FullInformation = (PKEY_FULL_INFORMATION) KeyInformation;
          FullInformation->LastWriteTime = KeyBlock->LastWriteTime;
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
          wcsncpy(FullInformation->Class,
                  &KeyBlock->Name[KeyBlock->NameSize + 1],
                  KeyBlock->ClassSize);
          FullInformation->Class[KeyBlock->ClassSize] = 0;
          *ResultLength = sizeof(KEY_FULL_INFORMATION) +
            KeyBlock->ClassSize * sizeof(WCHAR);
        }
      break;
    }

  return  Status;
#else
  UNIMPLEMENTED;
#endif
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
#ifdef PROTO_REG
  NTSTATUS  Status;
  PKEY_OBJECT  KeyObject;
  PREGISTRY_FILE  RegistryFile;
  PKEY_BLOCK  KeyBlock;
  PVALUE_BLOCK  ValueBlock;
  PVOID  DataBlock;
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
  Status = CmiScanKeyForValue(RegistryFile, 
                              KeyBlock,
                              ValueName->Buffer,
                              &ValueBlock);
  if (!NT_SUCCESS(Status))
    {
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
              ValueBasicInformation->NameLength = ValueBlock->NameSize;
              wcscpy(ValueBasicInformation->Name, ValueBlock->Name);
            }
          break;

        case KeyValuePartialInformation:
          *ResultLength = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 
            ValueBlock->DataSize;
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
              ValuePartialInformation->DataLength = ValueBlock->DataSize;
              DataBlock = CmiGetBlock(RegistryFile, ValueBlock->DataOffset);
              RtlCopyMemory(ValuePartialInformation->Data, 
                            DataBlock, 
                            ValueBlock->DataSize);
              CmiReleaseBlock(RegistryFile, DataBlock);
            }
          break;

        case KeyValueFullInformation:
          *ResultLength = sizeof(KEY_VALUE_FULL_INFORMATION) + 
            ValueBlock->NameSize * sizeof(WCHAR) + ValueBlock->DataSize;
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
                sizeof(KEY_VALUE_FULL_INFORMATION) + 
                ValueBlock->NameSize * sizeof(WCHAR);
              ValueFullInformation->DataLength = ValueBlock->DataSize;
              ValueFullInformation->NameLength = ValueBlock->NameSize;
              wcscpy(ValueFullInformation->Name, ValueBlock->Name);
              DataBlock = CmiGetBlock(RegistryFile, ValueBlock->DataOffset);
              RtlCopyMemory(&ValueFullInformation->Name[ValueBlock->NameSize + 1], 
                            DataBlock, 
                            ValueBlock->DataSize);
              CmiReleaseBlock(RegistryFile, DataBlock);
            }
          break;
        }
    }
  else
    {
      Status = STATUS_UNSUCCESSFUL;
    }
  
  return  Status;
#else
   UNIMPLEMENTED;
#endif
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
#ifdef PROTO_REG
  NTSTATUS  Status;
  PKEY_OBJECT  KeyObject;
  PREGISTRY_FILE  RegistryFile;
  PKEY_BLOCK  KeyBlock;
  PVALUE_BLOCK  ValueBlock;

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
  Status = CmiScanKeyForValue(RegistryFile,
                              KeyBlock,
                              ValueName->Buffer,
                              &ValueBlock);
  if (!NT_SUCCESS(Status))
    {
      return  Status;
    }
  if (ValueBlock == NULL)
    {
      Status =  CmiAddValueToKey(RegistryFile,
                                 KeyBlock,
                                 ValueName->Buffer,
                                 Type,
                                 Data,
                                 DataSize);
    }
  else
    {
      Status = CmiReplaceValueData(RegistryFile,
                                   ValueBlock,
                                   Type,
                                   Data,
                                   DataSize);
    }
  
  return  Status;
#else
   UNIMPLEMENTED;
#endif
}

NTSTATUS
STDCALL
NtDeleteValueKey (
	IN	HANDLE		KeyHandle,
	IN	PUNICODE_STRING	ValueName
	)
{
#ifdef PROTO_REG
  NTSTATUS  Status;
  PKEY_OBJECT  KeyObject;
  PREGISTRY_FILE  RegistryFile;
  PKEY_BLOCK  KeyBlock;

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
  Status = CmiDeleteValueFromKey(RegistryFile,
                                 KeyBlock,
                                 ValueName->Buffer);

  return  Status;
#else
   UNIMPLEMENTED;
#endif
}

NTSTATUS
STDCALL 
NtLoadKey (
	PHANDLE			KeyHandle,
	OBJECT_ATTRIBUTES	ObjectAttributes
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtLoadKey2(VOID)
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
	IN	HANDLE	KeyHandle,	
	IN	PVALENT	ListOfValuesToQuery,	
	IN	ULONG	NumberOfItems,	
	OUT	PVOID	MultipleValueInformation,		
	IN	ULONG	Length,
	OUT	PULONG	ReturnLength)
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
	UNIMPLEMENTED;
}


NTSTATUS 
STDCALL
RtlCheckRegistryKey (
	ULONG	RelativeTo,
	PWSTR	Path
	)
{
	UNIMPLEMENTED;
}


NTSTATUS 
STDCALL
RtlCreateRegistryKey (
	IN	ULONG	RelativeTo,
	IN	PWSTR	Path
	)
{
	UNIMPLEMENTED;
}


NTSTATUS 
STDCALL
RtlDeleteRegistryValue (
	IN	ULONG	RelativeTo, 
	IN	PWSTR	Path,
	IN	PWSTR	ValueName
	)
{
	UNIMPLEMENTED;
}


NTSTATUS 
STDCALL
RtlQueryRegistryValues (
	IN	ULONG				RelativeTo,
	IN	PWSTR				Path,
		PRTL_QUERY_REGISTRY_TABLE	QueryTable,
		PVOID				Context,
		PVOID				Environment
	)
{
	UNIMPLEMENTED;
}


NTSTATUS 
STDCALL
RtlWriteRegistryValue (
	ULONG	RelativeTo,
	PWSTR	Path,
	PWSTR	ValueName,
	ULONG	ValueType,
	PVOID	ValueData,
	ULONG	ValueLength
	)
{
	UNIMPLEMENTED;
}

/*  ------------------------------------------  Private Implementation  */

#if PROTO_REG

static PVOID 
CmiObjectParse(PVOID  ParsedObject, PWSTR  *Path)
{
  NTSTATUS  Status;
  /* FIXME: this should be allocated based on the largest subkey name  */
  WCHAR  CurKeyName[256];
  PWSTR  Remainder, NextSlash;
  PREGISTRY_FILE  RegistryFile;
  PKEY_OBJECT  NewKeyObject;
  HANDLE  KeyHandle;
  PKEY_BLOCK  CurKeyBlock, SubKeyBlock;

  Status = STATUS_SUCCESS;

  /* FIXME: it should probably get this from ParsedObject  */
  RegistryFile = CmiVolatileFile;

  /*  Scan key object list for key already open  */
  NewKeyObject = CmiScanKeyList((*Path) + 1);
  if (NewKeyObject != NULL)
    {
      /*  Return reference if found  */
      ObReferenceObjectByPointer(NewKeyObject,
                                 STANDARD_RIGHTS_REQUIRED,
                                 NULL,
                                 UserMode);
      *Path = NULL;

      return  NewKeyObject;
    }

  /* FIXME: this access of RootKeyBlock should be guarded by spinlock  */
  CurKeyBlock = CmiGetKeyBlock(RegistryFile, 
                               RegistryFile->HeaderBlock->RootKeyBlock);

  /*  Loop through each key level and find the needed subkey  */
  Remainder = (*Path) + 1;
  while (NT_SUCCESS(Status) && *Remainder != 0)
    {
      NextSlash = wcschr(Remainder, L'\\');

      /*  Copy just the current subkey name to a buffer  */
      if (NextSlash != NULL)
        {
          wcsncpy(CurKeyName, Remainder, NextSlash - Remainder);
          CurKeyName[NextSlash - Remainder] = 0;
        }
      else
        {
          wcscpy(CurKeyName, Remainder);
        }

      /* Verify existance of CurKeyName  */
      Status = CmiScanForSubKey(RegistryFile, 
                                CurKeyBlock, 
                                &SubKeyBlock,
                                CurKeyName,
                                STANDARD_RIGHTS_REQUIRED);
      if (!NT_SUCCESS(Status))
        {
          continue;
        }
      if (SubKeyBlock == NULL)
        {
          Status = STATUS_UNSUCCESSFUL;
          continue;
        }
      CmiReleaseBlock(RegistryFile, CurKeyBlock);
      CurKeyBlock = SubKeyBlock;

      if (NextSlash != NULL)
        {
          Remainder = NextSlash + 1;
        }
      else
        {
          Remainder = NULL;
        }
    }
  
  /*  Create new key object and put into linked list  */
  NewKeyObject = ObCreateObject(&KeyHandle, 
                                STANDARD_RIGHTS_REQUIRED, 
                                NULL, 
                                CmiKeyType);
  if (NewKeyObject == NULL)
    {
      return  NULL;
    }
  NewKeyObject->Flags = 0;
  NewKeyObject->Name = ExAllocatePool(NonPagedPool, 
                                      wcslen(*Path) * sizeof(WCHAR));
  wcscpy(NewKeyObject->Name, (*Path) + 1);
  NewKeyObject->KeyBlock = CurKeyBlock;
  NewKeyObject->RegistryFile = RegistryFile;
  CmiAddKeyToList(NewKeyObject);
  *Path = (Remainder != NULL) ? Remainder - 1 : NULL;
  
  return  NewKeyObject;
}

static VOID  
CmiObjectDelete(PVOID  DeletedObject)
{
  PKEY_OBJECT  KeyObject;

  KeyObject = (PKEY_OBJECT) DeletedObject;
  if (KeyObject->Flags & KO_MARKED_FOR_DELETE)
    {
      CmiDestroyKeyBlock(KeyObject->RegistryFile,
                         KeyObject->KeyBlock);
    }
  else
    {
      CmiReleaseBlock(KeyObject->RegistryFile,
                      KeyObject->KeyBlock);
    }
  ExFreePool(KeyObject->Name);
  CmiRemoveKeyFromList(KeyObject);
}

static NTSTATUS
CmiBuildKeyPath(PWSTR  *KeyPath, POBJECT_ATTRIBUTES  ObjectAttributes)
{
  NTSTATUS  Status;
  ULONG  KeyNameSize;
  PWSTR  KeyNameBuf;
  PVOID  ObjectBody;
  PKEY_OBJECT  KeyObject;
  POBJECT_HEADER  ObjectHeader;

  /* FIXME: Verify ObjectAttributes is in \\Registry space and compute size for path */
  KeyNameSize = 0;
  ObjectHeader = 0;
  if (ObjectAttributes->RootDirectory != NULL)
    {
      /* FIXME: determine type of object for RootDirectory  */
      Status = ObReferenceObjectByHandle(ObjectAttributes->RootDirectory,
                                         KEY_READ,
                                         NULL,
                                         UserMode,
                                         (PVOID *)&ObjectBody,
                                         NULL);
      if (!NT_SUCCESS(Status))
        {
          return  Status;
        }
      ObjectHeader = BODY_TO_HEADER(ObjectBody);

      if (ObjectHeader->ObjectType != CmiKeyType)
        {
          /*  Fail if RootDirectory != '\\'  */
          if (ObjectBody == NameSpaceRoot)
            {
              /*  Check for 'Registry' in ObjectName, fail if missing  */
              if (wcsncmp(ObjectAttributes->ObjectName->Buffer, 
                          REG_ROOT_KEY_NAME + 1, 
                          wcslen(REG_ROOT_KEY_NAME + 1)) != 0 ||
                          ObjectAttributes->ObjectName->Buffer[wcslen(REG_ROOT_KEY_NAME + 1)] != L'\\')
                {
                  ObDereferenceObject(ObjectBody);

                  return  STATUS_OBJECT_PATH_INVALID;
                }
 
              /*  Compute size of registry portion of path to KeyNameSize  */
              KeyNameSize = (ObjectAttributes->ObjectName->Length -
                (wcslen(REG_ROOT_KEY_NAME + 1) + 1))
                * sizeof(WCHAR);
            }
          else if (!wcscmp(ObjectHeader->Name.Buffer, 
                           REG_ROOT_KEY_NAME + 1))
            {
              /*  Add size of ObjectName to KeyNameSize  */
              KeyNameSize = ObjectAttributes->ObjectName->Length;
            }
          else 
            {
              ObDereferenceObject(ObjectBody);

              return  STATUS_OBJECT_PATH_INVALID;
            }
        }
      else
        {
          KeyObject = (PKEY_OBJECT) ObjectBody;
        
          /*  Add size of Name from RootDirectory object to KeyNameSize  */
          KeyNameSize = wcslen(KeyObject->Name) * sizeof(WCHAR);

          /*  Add 1 to KeyNamesize for '\\'  */
          KeyNameSize += sizeof(WCHAR);

          /*  Add size of ObjectName to KeyNameSize  */
          KeyNameSize += ObjectAttributes->ObjectName->Length * sizeof(WCHAR);
        }
    }
  else
    {
      /*  Check for \\Registry and fail if missing  */
      if (wcsncmp(ObjectAttributes->ObjectName->Buffer, 
                  REG_ROOT_KEY_NAME, 
                  wcslen(REG_ROOT_KEY_NAME)) != 0 ||
          ObjectAttributes->ObjectName->Buffer[wcslen(REG_ROOT_KEY_NAME)] != L'\\')
        {
          return  STATUS_OBJECT_PATH_INVALID;
        }
 
      /*  Compute size of registry portion of path to KeyNameSize  */
      KeyNameSize = (ObjectAttributes->ObjectName->Length - 
        (wcslen(REG_ROOT_KEY_NAME) + 1)) * sizeof(WCHAR);
    }

  KeyNameBuf = ExAllocatePool(NonPagedPool, KeyNameSize + sizeof(WCHAR));

  /*  Construct relative pathname  */
  KeyNameBuf[0] = 0;
  if (ObjectAttributes->RootDirectory != NULL)
    {
      if (ObjectHeader->ObjectType != CmiKeyType)
        {
          /*  Fail if RootDirectory != '\\'  */
          if (ObjectBody == NameSpaceRoot)
            {
              /*  Copy remainder of ObjectName after 'Registry'  */
              wcscpy(KeyNameBuf, ObjectAttributes->ObjectName->Buffer + wcslen(REG_ROOT_KEY_NAME + 1));
            }
          else
            {
              /*  Copy all of ObjectName  */
              wcscpy(KeyNameBuf, ObjectAttributes->ObjectName->Buffer);
            }
        }
      else
        {
          KeyObject = (PKEY_OBJECT) ObjectBody;
        
          /*  Copy Name from RootDirectory object to KeyNameBuf  */
          wcscpy(KeyNameBuf, KeyObject->Name);

          /*  Append '\\' onto KeyNameBuf */
          wcscat(KeyNameBuf, L"\\");

          /*  Append ObjectName onto KeyNameBuf  */
          wcscat(KeyNameBuf, ObjectAttributes->ObjectName->Buffer);
        }
    }
  else
    {
      /*  Copy registry portion of path into KeyNameBuf  */
      wcscpy(KeyNameBuf, ObjectAttributes->ObjectName->Buffer + 
        (wcslen(REG_ROOT_KEY_NAME) + 1));
    }

  *KeyPath = KeyNameBuf;
  return  STATUS_SUCCESS;
}

static VOID
CmiAddKeyToList(PKEY_OBJECT  NewKey)
{
  KIRQL  OldIrql;
  
  KeAcquireSpinLock(&CmiKeyListLock, &OldIrql);
  NewKey->NextKey = CmiKeyList;
  CmiKeyList = NewKey;
  KeReleaseSpinLock(&CmiKeyListLock, OldIrql);
}

static VOID  
CmiRemoveKeyFromList(PKEY_OBJECT  KeyToRemove)
{
  KIRQL  OldIrql;
  PKEY_OBJECT  CurKey;

  KeAcquireSpinLock(&CmiKeyListLock, &OldIrql);
  if (CmiKeyList == KeyToRemove)
    {
      CmiKeyList = CmiKeyList->NextKey;
    }
  else
    {
      CurKey = CmiKeyList;
      while (CurKey != NULL && CurKey->NextKey != KeyToRemove)
        {
          CurKey = CurKey->NextKey;
        }
      if (CurKey != NULL)
        {
          CurKey->NextKey = KeyToRemove->NextKey;
        }
    }
  KeReleaseSpinLock(&CmiKeyListLock, OldIrql);
}

static PKEY_OBJECT
CmiScanKeyList(PWSTR  KeyName)
{
  KIRQL  OldIrql;
  PKEY_OBJECT  CurKey;

  KeAcquireSpinLock(&CmiKeyListLock, &OldIrql);
  CurKey = CmiKeyList;
  while (CurKey != NULL && wcscmp(KeyName, CurKey->Name) != 0)
    {
      CurKey = CurKey->NextKey;
    }
  KeReleaseSpinLock(&CmiKeyListLock, OldIrql);
  
  return CurKey;
}

static PREGISTRY_FILE  
CmiCreateRegistry(PWSTR  Filename)
{
  PREGISTRY_FILE  RegistryFile;
  PKEY_BLOCK  RootKeyBlock;

  RegistryFile = ExAllocatePool(NonPagedPool, sizeof(REGISTRY_FILE));
  if (Filename != NULL)
    {
      UNIMPLEMENTED;
      /* FIXME:  Duplicate Filename  */
      /* FIXME:  if file does not exist, create new file  */
      /* FIXME:  else attempt to map the file  */
    }
  else
    {
      RegistryFile->Filename = NULL;
      RegistryFile->FileHandle = NULL;

      RegistryFile->HeaderBlock = (PHEADER_BLOCK) 
        ExAllocatePool(NonPagedPool, sizeof(HEADER_BLOCK));
      RtlZeroMemory(RegistryFile->HeaderBlock, sizeof(HEADER_BLOCK));
      RegistryFile->HeaderBlock->BlockId = 0x66676572;
      RegistryFile->HeaderBlock->DateModified.QuadPart = 0;
      RegistryFile->HeaderBlock->Unused2 = 1;
      RegistryFile->HeaderBlock->Unused3 = 3;
      RegistryFile->HeaderBlock->Unused5 = 1;
      RegistryFile->HeaderBlock->RootKeyBlock = 0;
      RegistryFile->HeaderBlock->BlockSize = REG_BLOCK_SIZE;
      RegistryFile->HeaderBlock->Unused6 = 1;
      RegistryFile->HeaderBlock->Checksum = 0;
      RootKeyBlock = (PKEY_BLOCK) 
        ExAllocatePool(NonPagedPool, sizeof(KEY_BLOCK));
      RtlZeroMemory(RootKeyBlock, sizeof(KEY_BLOCK));
      RootKeyBlock->SubBlockId = REG_KEY_BLOCK_ID;
      RootKeyBlock->Type = REG_ROOT_KEY_BLOCK_TYPE;
      ZwQuerySystemTime((PTIME) &RootKeyBlock->LastWriteTime);
      RootKeyBlock->ParentKeyOffset = 0;
      RootKeyBlock->NumberOfSubKeys = 0;
      RootKeyBlock->HashTableOffset = 0;
      RootKeyBlock->NumberOfValues = 0;
      RootKeyBlock->ValuesOffset = 0;
      RootKeyBlock->SecurityKeyOffset = 0;
      RootKeyBlock->ClassNameOffset = sizeof(KEY_BLOCK);
      RootKeyBlock->NameSize = 0;
      RootKeyBlock->ClassSize = 0;
      RootKeyBlock->Name[0] = 0;
      RegistryFile->HeaderBlock->RootKeyBlock = (BLOCK_OFFSET) RootKeyBlock;
    }

  return  RegistryFile;
}

static NTSTATUS
CmiCreateKey(IN PREGISTRY_FILE  RegistryFile,
             IN PWSTR  KeyNameBuf,
             OUT PKEY_BLOCK  *KeyBlock,
             IN ACCESS_MASK DesiredAccess,
             IN ULONG TitleIndex,
             IN PUNICODE_STRING Class, 
             IN ULONG CreateOptions, 
             OUT PULONG Disposition)
{
  /* FIXME: this should be allocated based on the largest subkey name  */
  NTSTATUS  Status;
  WCHAR  CurKeyName[256];
  PWSTR  ClassName;
  PWSTR  Remainder, NextSlash;
  PKEY_BLOCK  CurKeyBlock, SubKeyBlock;

  /* FIXME:  Should handle search by Class/TitleIndex  */

CHECKPOINT;
  /*  Loop through each key level and find or build the needed subkey  */
  Status = STATUS_SUCCESS;
  /* FIXME: this access of RootKeyBlock should be guarded by spinlock  */
  CurKeyBlock = CmiGetKeyBlock(RegistryFile, 
                               RegistryFile->HeaderBlock->RootKeyBlock);
CHECKPOINT;
  Remainder = KeyNameBuf;
  while (NT_SUCCESS(Status)  &&
         (NextSlash = wcschr(Remainder, L'\\')) != NULL)
    {
      /*  Copy just the current subkey name to a buffer  */
      wcsncpy(CurKeyName, Remainder, NextSlash - Remainder);
      CurKeyName[NextSlash - Remainder] = 0;

      /* Verify existance of/Create CurKeyName  */
CHECKPOINT;
      Status = CmiScanForSubKey(RegistryFile, 
                                CurKeyBlock, 
                                &SubKeyBlock,
                                CurKeyName,
                                DesiredAccess);
      if (!NT_SUCCESS(Status))
        {
          continue;
        }
      if (SubKeyBlock == NULL)
        {
          Status = CmiAddSubKey(RegistryFile, 
                                CurKeyBlock,
                                &SubKeyBlock,
                                CurKeyName,
                                0,
                                NULL, 
                                0);
          if (!NT_SUCCESS(Status))
            {
              continue;
            }
        }
      CmiReleaseBlock(RegistryFile, CurKeyBlock);
      CurKeyBlock = SubKeyBlock;

      Remainder = NextSlash + 1;      
    }
CHECKPOINT;
  if (NT_SUCCESS(Status))
    {
CHECKPOINT;
      Status = CmiScanForSubKey(RegistryFile, 
                                CurKeyBlock, 
                                &SubKeyBlock,
                                CurKeyName,
                                DesiredAccess);
      if (NT_SUCCESS(Status))
        {
          if (SubKeyBlock == NULL)
            {
              if (Class != NULL)
                {
                  ClassName = ExAllocatePool(NonPagedPool, Class->Length + 1);
                  wcsncpy(ClassName, Class->Buffer, Class->Length);
                  ClassName[Class->Length] = 0;
                }
              else
                {
                  ClassName = 0;
                }
              Status = CmiAddSubKey(RegistryFile, 
                                    CurKeyBlock,
                                    &SubKeyBlock,
                                    Remainder,
                                    TitleIndex,
                                    ClassName, 
                                    CreateOptions);
              if (ClassName != NULL)
                {
                  ExFreePool(ClassName);
                }
              if (NT_SUCCESS(Status) && Disposition != NULL)
                {
                  *Disposition = REG_CREATED_NEW_KEY;
                }
            }
          else if (Disposition != NULL)
            {
              *Disposition = REG_OPENED_EXISTING_KEY;
            }
        }
      *KeyBlock = SubKeyBlock;
    }
  CmiReleaseBlock(RegistryFile, CurKeyBlock);
  
  return  Status;
}

static NTSTATUS  
CmiFindKey(IN PREGISTRY_FILE  RegistryFile,
           IN PWSTR  KeyNameBuf,
           OUT PKEY_BLOCK  *KeyBlock,
           IN ACCESS_MASK DesiredAccess,
           IN ULONG TitleIndex,
           IN PUNICODE_STRING Class)
{
  /* FIXME: this should be allocated based on the largest subkey name  */
  NTSTATUS  Status;
  WCHAR  CurKeyName[256];
  PWSTR  Remainder, NextSlash;
  PKEY_BLOCK  CurKeyBlock, SubKeyBlock;

  /* FIXME:  Should handle search by Class/TitleIndex  */

  /*  Loop through each key level and find the needed subkey  */
  Status = STATUS_SUCCESS;
  /* FIXME: this access of RootKeyBlock should be guarded by spinlock  */
  CurKeyBlock = CmiGetKeyBlock(RegistryFile, RegistryFile->HeaderBlock->RootKeyBlock);
  Remainder = KeyNameBuf;
  while (NT_SUCCESS(Status) &&
         (NextSlash = wcschr(Remainder, L'\\')) != NULL)
    {
      /*  Copy just the current subkey name to a buffer  */
      wcsncpy(CurKeyName, Remainder, NextSlash - Remainder);
      CurKeyName[NextSlash - Remainder] = 0;

      /* Verify existance of CurKeyName  */
      Status = CmiScanForSubKey(RegistryFile, 
                                CurKeyBlock, 
                                &SubKeyBlock,
                                CurKeyName,
                                DesiredAccess);
      if (!NT_SUCCESS(Status))
        {
          continue;
        }
      if (SubKeyBlock == NULL)
        {
          Status = STATUS_UNSUCCESSFUL;
          continue;
        }
      CmiReleaseBlock(RegistryFile, CurKeyBlock);
      CurKeyBlock = SubKeyBlock;

      Remainder = NextSlash + 1;      
    }
  if (NT_SUCCESS(Status))
    {
      Status = CmiScanForSubKey(RegistryFile, 
                                CurKeyBlock, 
                                &SubKeyBlock,
                                CurKeyName,
                                DesiredAccess);
      if (NT_SUCCESS(Status))
        {
          if (SubKeyBlock == NULL)
            {
              Status = STATUS_UNSUCCESSFUL;
            }
          else
            {
              *KeyBlock = SubKeyBlock;
            }
        }
    }
  CmiReleaseBlock(RegistryFile, CurKeyBlock);
  
  return  Status;
}

static ULONG  
CmiGetMaxNameLength(PREGISTRY_FILE  RegistryFile,
                    PKEY_BLOCK  KeyBlock)
{
  ULONG  Idx, MaxName;
  PHASH_TABLE_BLOCK  HashBlock;
  PKEY_BLOCK  CurSubKeyBlock;

  MaxName = 0;
  HashBlock = CmiGetHashTableBlock(RegistryFile, KeyBlock->HashTableOffset);
  if (HashBlock == 0)
    {
      return  0;
    }
  for (Idx = 0; Idx < HashBlock->HashTableSize; Idx++)
    {
      if (HashBlock->Table[Idx].KeyOffset != 0)
        {
          CurSubKeyBlock = CmiGetKeyBlock(RegistryFile,
                                          HashBlock->Table[Idx].KeyOffset);
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

static ULONG  
CmiGetMaxClassLength(PREGISTRY_FILE  RegistryFile,
                     PKEY_BLOCK  KeyBlock)
{
  ULONG  Idx, MaxClass;
  PHASH_TABLE_BLOCK  HashBlock;
  PKEY_BLOCK  CurSubKeyBlock;

  MaxClass = 0;
  HashBlock = CmiGetHashTableBlock(RegistryFile, KeyBlock->HashTableOffset);
  if (HashBlock == 0)
    {
      return  0;
    }
  for (Idx = 0; Idx < HashBlock->HashTableSize; Idx++)
    {
      if (HashBlock->Table[Idx].KeyOffset != 0)
        {
          CurSubKeyBlock = CmiGetKeyBlock(RegistryFile,
                                          HashBlock->Table[Idx].KeyOffset);
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

static ULONG  
CmiGetMaxValueNameLength(PREGISTRY_FILE  RegistryFile,
                         PKEY_BLOCK  KeyBlock)
{
  ULONG  Idx, MaxValueName;
  PVALUE_LIST_BLOCK  ValueListBlock;
  PVALUE_BLOCK  CurValueBlock;

  ValueListBlock = CmiGetBlock(RegistryFile, 
                               KeyBlock->ValuesOffset);
  MaxValueName = 0;
  if (ValueListBlock == 0)
    {
      return  0;
    }
  for (Idx = 0; Idx < KeyBlock->NumberOfValues; Idx++)
    {
      CurValueBlock = CmiGetBlock(RegistryFile,
                                  ValueListBlock->Values[Idx]);
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

static ULONG  
CmiGetMaxValueDataLength(PREGISTRY_FILE  RegistryFile,
                         PKEY_BLOCK  KeyBlock)
{
  ULONG  Idx, MaxValueData;
  PVALUE_LIST_BLOCK  ValueListBlock;
  PVALUE_BLOCK  CurValueBlock;

  ValueListBlock = CmiGetBlock(RegistryFile, 
                               KeyBlock->ValuesOffset);
  MaxValueData = 0;
  if (ValueListBlock == 0)
    {
      return  0;
    }
  for (Idx = 0; Idx < KeyBlock->NumberOfValues; Idx++)
    {
      CurValueBlock = CmiGetBlock(RegistryFile,
                                  ValueListBlock->Values[Idx]);
      if (CurValueBlock != NULL &&
          MaxValueData < CurValueBlock->DataSize)
        {
          MaxValueData = CurValueBlock->DataSize;
        }
      CmiReleaseBlock(RegistryFile, CurValueBlock);
    }

  CmiReleaseBlock(RegistryFile, ValueListBlock);
  
  return  MaxValueData;
}

static NTSTATUS
CmiScanForSubKey(IN PREGISTRY_FILE  RegistryFile, 
                 IN PKEY_BLOCK  KeyBlock, 
                 OUT PKEY_BLOCK  *SubKeyBlock,
                 IN PWSTR  KeyName,
                 IN ACCESS_MASK  DesiredAccess)
{
  ULONG  Idx;
  PHASH_TABLE_BLOCK  HashBlock;
  PKEY_BLOCK  CurSubKeyBlock;

  HashBlock = CmiGetHashTableBlock(RegistryFile, KeyBlock->HashTableOffset);
  *SubKeyBlock = NULL;
  if (HashBlock == 0)
    {
      return  STATUS_SUCCESS;
    }
  for (Idx = 0; Idx < HashBlock->HashTableSize; Idx++)
    {
      if (HashBlock->Table[Idx].KeyOffset != 0 &&
          !wcsncmp(KeyName, (PWSTR) &HashBlock->Table[Idx].HashValue, 4))
        {
          CurSubKeyBlock = CmiGetKeyBlock(RegistryFile,
                                          HashBlock->Table[Idx].KeyOffset);
          if (!wcscmp(KeyName, CurSubKeyBlock->Name))
            {
              *SubKeyBlock = CurSubKeyBlock;
              break;
            }
          else
            {
              CmiReleaseBlock(RegistryFile, CurSubKeyBlock);
            }
        }
    }

  CmiReleaseBlock(RegistryFile, HashBlock);
  
  return  STATUS_SUCCESS;
}

static NTSTATUS
CmiAddSubKey(PREGISTRY_FILE  RegistryFile, 
             PKEY_BLOCK  KeyBlock,
             PKEY_BLOCK  *SubKeyBlock,
             PWSTR  NewSubKeyName,
             ULONG  TitleIndex,
             PWSTR  Class, 
             ULONG  CreateOptions)
{
  NTSTATUS  Status;
  PHASH_TABLE_BLOCK  HashBlock, NewHashBlock;
  PKEY_BLOCK  NewKeyBlock;

  Status = CmiAllocateKeyBlock(RegistryFile,
                               &NewKeyBlock,
                               NewSubKeyName,
                               TitleIndex,
                               Class,
                               CreateOptions);
  if (!NT_SUCCESS(Status))
    {
      return  Status;
    }
  if (KeyBlock->HashTableOffset == 0)
    {
      Status = CmiAllocateHashTableBlock(RegistryFile, 
                                         &HashBlock,
                                         REG_INIT_HASH_TABLE_SIZE);
      if (!NT_SUCCESS(Status))
        {
          return  Status;
        }
      KeyBlock->HashTableOffset = CmiGetBlockOffset(RegistryFile, HashBlock);
    }
  else
    {
      HashBlock = CmiGetHashTableBlock(RegistryFile, KeyBlock->HashTableOffset);
      if (KeyBlock->NumberOfSubKeys + 1 >= HashBlock->HashTableSize)
        {

          /* FIXME: All Subkeys will need to be rehashed here!  */

          /*  Reallocate the hash table block  */
          Status = CmiAllocateHashTableBlock(RegistryFile, 
                                             &NewHashBlock,
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
          KeyBlock->HashTableOffset = CmiGetBlockOffset(RegistryFile, NewHashBlock);
          CmiDestroyHashTableBlock(RegistryFile, HashBlock);
          HashBlock = NewHashBlock;
        }
    }
  Status = CmiAddKeyToHashTable(RegistryFile, HashBlock, NewKeyBlock);
  if (NT_SUCCESS(Status))
    { 
      KeyBlock->NumberOfSubKeys++;
      *SubKeyBlock = NewKeyBlock;
    }
  CmiReleaseBlock(RegistryFile, HashBlock);
  
  return  Status;
}

static NTSTATUS  
CmiScanKeyForValue(IN PREGISTRY_FILE  RegistryFile,
                   IN PKEY_BLOCK  KeyBlock,
                   IN PWSTR  ValueName,
                   OUT PVALUE_BLOCK  *ValueBlock)
{
  ULONG  Idx;
  PVALUE_LIST_BLOCK  ValueListBlock;
  PVALUE_BLOCK  CurValueBlock;

  ValueListBlock = CmiGetBlock(RegistryFile, 
                               KeyBlock->ValuesOffset);
  *ValueBlock = NULL;
  if (ValueListBlock == 0)
    {
      return  STATUS_SUCCESS;
    }
  for (Idx = 0; Idx < KeyBlock->NumberOfValues; Idx++)
    {
      CurValueBlock = CmiGetBlock(RegistryFile,
                                  ValueListBlock->Values[Idx]);
      if (CurValueBlock != NULL &&
          !wcscmp(CurValueBlock->Name, ValueName))
        {
          *ValueBlock = CurValueBlock;
          break;
        }
      CmiReleaseBlock(RegistryFile, CurValueBlock);
    }

  CmiReleaseBlock(RegistryFile, ValueListBlock);
  
  return  STATUS_SUCCESS;
}

static NTSTATUS  
CmiAddValueToKey(IN PREGISTRY_FILE  RegistryFile,
                 IN PKEY_BLOCK  KeyBlock,
                 IN PWSTR  ValueNameBuf,
                 IN ULONG  Type, 
                 IN PVOID  Data,
                 IN ULONG  DataSize)
{
  NTSTATUS  Status;
  PVALUE_LIST_BLOCK  ValueListBlock, NewValueListBlock;
  PVALUE_BLOCK  ValueBlock;

  Status = CmiAllocateValueBlock(RegistryFile,
                                 &ValueBlock,
                                 ValueNameBuf,
                                 Type,
                                 Data,
                                 DataSize);
  if (!NT_SUCCESS(Status))
    {
      return  Status;
    }
  ValueListBlock = CmiGetBlock(RegistryFile, 
                               KeyBlock->ValuesOffset);
  if (ValueListBlock == NULL)
    {
      Status = CmiAllocateBlock(RegistryFile,
                                (PVOID) &ValueListBlock,
                                sizeof(BLOCK_OFFSET) *
                                  REG_VALUE_LIST_BLOCK_MULTIPLE);
      if (!NT_SUCCESS(Status))
        {
          CmiDestroyValueBlock(RegistryFile,
                               ValueBlock);
          return  Status;
        }
    }
  else if (KeyBlock->NumberOfValues % REG_VALUE_LIST_BLOCK_MULTIPLE)
    {
      Status = CmiAllocateBlock(RegistryFile,
                                (PVOID) &NewValueListBlock,
                                sizeof(BLOCK_OFFSET) *
                                  (KeyBlock->NumberOfValues + 
                                    REG_VALUE_LIST_BLOCK_MULTIPLE));
      if (!NT_SUCCESS(Status))
        {
          CmiDestroyValueBlock(RegistryFile,
                               ValueBlock);
          return  Status;
        }
      RtlCopyMemory(NewValueListBlock, 
                    ValueListBlock,
                    sizeof(BLOCK_OFFSET) * KeyBlock->NumberOfValues);
      KeyBlock->ValuesOffset = CmiGetBlockOffset(RegistryFile, 
                                                 NewValueListBlock);
      CmiDestroyBlock(RegistryFile, ValueListBlock);
      ValueListBlock = NewValueListBlock;
    }
  ValueListBlock->Values[KeyBlock->NumberOfValues] = 
    CmiGetBlockOffset(RegistryFile, ValueBlock);
  KeyBlock->NumberOfValues++;
  CmiReleaseBlock(RegistryFile, ValueBlock);
  CmiReleaseBlock(RegistryFile, ValueListBlock);

  return  STATUS_SUCCESS;
}

static NTSTATUS  
CmiDeleteValueFromKey(IN PREGISTRY_FILE  RegistryFile,
                      IN PKEY_BLOCK  KeyBlock,
                      IN PWSTR  ValueName)
{
  ULONG  Idx;
  PVALUE_LIST_BLOCK  ValueListBlock;
  PVALUE_BLOCK  CurValueBlock;

  ValueListBlock = CmiGetBlock(RegistryFile, 
                               KeyBlock->ValuesOffset);
  if (ValueListBlock == 0)
    {
      return  STATUS_SUCCESS;
    }
  for (Idx = 0; Idx < KeyBlock->NumberOfValues; Idx++)
    {
      CurValueBlock = CmiGetBlock(RegistryFile,
                                  ValueListBlock->Values[Idx]);
      if (CurValueBlock != NULL &&
          !wcscmp(CurValueBlock->Name, ValueName))
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
          CmiDestroyValueBlock(RegistryFile, CurValueBlock);

          break;
        }
      CmiReleaseBlock(RegistryFile, CurValueBlock);
    }

  CmiReleaseBlock(RegistryFile, ValueListBlock);
  
  return  STATUS_SUCCESS;
}

static NTSTATUS
CmiAllocateKeyBlock(IN PREGISTRY_FILE  RegistryFile,
                    OUT PKEY_BLOCK  *KeyBlock,
                    IN PWSTR  KeyName,
                    IN ULONG  TitleIndex,
                    IN PWSTR  Class,
                    IN ULONG  CreateOptions)
{
  NTSTATUS  Status;
  ULONG  NewKeySize;
  PKEY_BLOCK  NewKeyBlock;

  Status = STATUS_SUCCESS;

  /*  Handle volatile files first  */
  if (RegistryFile->Filename == NULL)
    {
      NewKeySize = sizeof(KEY_BLOCK) + 
        (wcslen(KeyName) + 1) * sizeof(WCHAR) + 
        (Class != NULL ? (wcslen(Class) + 1) * sizeof(WCHAR) : 0);
      NewKeyBlock = ExAllocatePool(NonPagedPool, NewKeySize);
      if (NewKeyBlock == NULL)
        {
          Status = STATUS_INSUFFICIENT_RESOURCES;
        }
      else
        {
          RtlZeroMemory(NewKeyBlock, NewKeySize);
          NewKeyBlock->SubBlockId = REG_KEY_BLOCK_ID;
          NewKeyBlock->Type = REG_KEY_BLOCK_TYPE;
          ZwQuerySystemTime((PTIME) &NewKeyBlock->LastWriteTime);
          NewKeyBlock->ParentKeyOffset = 0;
          NewKeyBlock->NumberOfSubKeys = 0;
          NewKeyBlock->HashTableOffset = 0;
          NewKeyBlock->NumberOfValues = 0;
          NewKeyBlock->ValuesOffset = 0;
          NewKeyBlock->SecurityKeyOffset = 0;
          NewKeyBlock->ClassNameOffset = sizeof(KEY_BLOCK) + wcslen(KeyName);
          NewKeyBlock->NameSize = wcslen(KeyName);
          NewKeyBlock->ClassSize = (Class != NULL) ? wcslen(Class) : 0;
          wcscpy(NewKeyBlock->Name, KeyName);
          if (Class != NULL)
            {
              wcscpy(&NewKeyBlock->Name[wcslen(KeyName) + 1], Class);
            }
          CmiLockBlock(RegistryFile, NewKeyBlock);
          *KeyBlock = NewKeyBlock;
        }
    }
  else
    {
      UNIMPLEMENTED;
    }

  return  Status;
}

static PKEY_BLOCK
CmiGetKeyBlock(PREGISTRY_FILE  RegistryFile,
               BLOCK_OFFSET  KeyBlockOffset)
{
  PKEY_BLOCK  KeyBlock;

  if (RegistryFile->Filename == NULL)
    {
      CmiLockBlock(RegistryFile, (PVOID) KeyBlockOffset);

      KeyBlock = (PKEY_BLOCK) KeyBlockOffset;
    }
  else
    {
      UNIMPLEMENTED;
    }

  return  KeyBlock;
}

static NTSTATUS
CmiDestroyKeyBlock(PREGISTRY_FILE  RegistryFile,
                   PKEY_BLOCK  KeyBlock)
{
  NTSTATUS  Status;

  Status = STATUS_SUCCESS;

  if (RegistryFile->Filename == NULL)
    {
      CmiReleaseBlock(RegistryFile, KeyBlock);
      ExFreePool(KeyBlock);
    }
  else
    {  
      UNIMPLEMENTED;
    }

  return  Status;
}

static NTSTATUS
CmiAllocateHashTableBlock(IN PREGISTRY_FILE  RegistryFile,
                          OUT PHASH_TABLE_BLOCK  *HashBlock,
                          IN ULONG  HashTableSize)
{
  NTSTATUS  Status;
  ULONG  NewHashSize;
  PHASH_TABLE_BLOCK  NewHashBlock;

  Status = STATUS_SUCCESS;

  /*  Handle volatile files first  */
  if (RegistryFile->Filename == NULL)
    {
      NewHashSize = sizeof(HASH_TABLE_BLOCK) + 
        (HashTableSize - 1) * sizeof(HASH_RECORD);
      NewHashBlock = ExAllocatePool(NonPagedPool, NewHashSize);
      if (NewHashBlock == NULL)
        {
          Status = STATUS_INSUFFICIENT_RESOURCES;
        }
      else
        {
          RtlZeroMemory(NewHashBlock, NewHashSize);
          NewHashBlock->SubBlockId = REG_HASH_TABLE_BLOCK_ID;
          NewHashBlock->HashTableSize = HashTableSize;
          CmiLockBlock(RegistryFile, NewHashBlock);
          *HashBlock = NewHashBlock;
        }
    }
  else
    {
      UNIMPLEMENTED;
    }

  return  Status;
}

static PHASH_TABLE_BLOCK  
CmiGetHashTableBlock(PREGISTRY_FILE  RegistryFile,
                     BLOCK_OFFSET  HashBlockOffset)
{
  PHASH_TABLE_BLOCK  HashBlock;

  if (RegistryFile->Filename == NULL)
    {
      CmiLockBlock(RegistryFile, (PVOID) HashBlockOffset);

      HashBlock = (PHASH_TABLE_BLOCK) HashBlockOffset;
    }
  else
    {
      UNIMPLEMENTED;
    }

  return  HashBlock;
}

static PKEY_BLOCK  
CmiGetKeyFromHashByIndex(PREGISTRY_FILE RegistryFile,
                         PHASH_TABLE_BLOCK  HashBlock,
                         ULONG  Index)
{
  PKEY_BLOCK  KeyBlock;

  if (RegistryFile->Filename == NULL)
    {
      KeyBlock = (PKEY_BLOCK) HashBlock->Table[Index].KeyOffset;
      CmiLockBlock(RegistryFile, KeyBlock);
    }
  else
    {
      UNIMPLEMENTED;
    }

  return  KeyBlock;
}

static NTSTATUS  
CmiAddKeyToHashTable(PREGISTRY_FILE  RegistryFile,
                     PHASH_TABLE_BLOCK  HashBlock,
                     PKEY_BLOCK  NewKeyBlock)
{
  HashBlock->Table[HashBlock->HashTableSize].KeyOffset = 
    CmiGetBlockOffset(RegistryFile, NewKeyBlock);
  RtlCopyMemory(&HashBlock->Table[HashBlock->HashTableSize].HashValue,
                NewKeyBlock->Name, 
                4);
  HashBlock->HashTableSize++;

  return  STATUS_SUCCESS;
}

static NTSTATUS
CmiDestroyHashTableBlock(PREGISTRY_FILE  RegistryFile,
                         PHASH_TABLE_BLOCK  HashBlock)
{
  NTSTATUS  Status;

  Status = STATUS_SUCCESS;

  if (RegistryFile->Filename == NULL)
    {
      CmiReleaseBlock(RegistryFile, HashBlock);
      ExFreePool(HashBlock);
    }
  else
    {  
      Status = STATUS_NOT_IMPLEMENTED;
    }

  return  Status;
}

static NTSTATUS
CmiAllocateValueBlock(PREGISTRY_FILE  RegistryFile,
                      PVALUE_BLOCK  *ValueBlock,
                      IN PWSTR  ValueNameBuf,
                      IN ULONG  Type, 
                      IN PVOID  Data,
                      IN ULONG  DataSize)
{
  NTSTATUS  Status;
  ULONG  NewValueSize;
  PVALUE_BLOCK  NewValueBlock;
  PVOID  DataBlock;

  Status = STATUS_SUCCESS;

  /*  Handle volatile files first  */
  if (RegistryFile->Filename == NULL)
    {
      NewValueSize = sizeof(VALUE_BLOCK) + wcslen(ValueNameBuf);
      NewValueBlock = ExAllocatePool(NonPagedPool, NewValueSize);
      if (NewValueBlock == NULL)
        {
          Status = STATUS_INSUFFICIENT_RESOURCES;
        }
      else
        {
          RtlZeroMemory(NewValueBlock, NewValueSize);
          NewValueBlock->SubBlockId = REG_VALUE_BLOCK_ID;
          NewValueBlock->NameSize = wcslen(ValueNameBuf);
          wcscpy(NewValueBlock->Name, ValueNameBuf);
          NewValueBlock->DataType = Type;
          NewValueBlock->DataSize = DataSize;
          Status = CmiAllocateBlock(RegistryFile,
                                    &DataBlock,
                                    DataSize);
          if (!NT_SUCCESS(Status))
            {
              ExFreePool(NewValueBlock);
            }
          else
            {
              RtlCopyMemory(DataBlock, Data, DataSize);
              NewValueBlock->DataOffset = CmiGetBlockOffset(RegistryFile,
                                                            DataBlock);
              CmiLockBlock(RegistryFile, NewValueBlock);
              CmiReleaseBlock(RegistryFile, DataBlock);
              *ValueBlock = NewValueBlock;
            }
        }
    }
  else
    {
      Status = STATUS_NOT_IMPLEMENTED;
    }

  return  Status;
}

static NTSTATUS  
CmiReplaceValueData(IN PREGISTRY_FILE  RegistryFile,
                    IN PVALUE_BLOCK  ValueBlock,
                    IN ULONG  Type, 
                    IN PVOID  Data,
                    IN ULONG  DataSize)
{
  NTSTATUS  Status;
  PVOID   DataBlock, NewDataBlock;

  Status = STATUS_SUCCESS;

  /* If new data size is <= current then overwrite current data  */
  if (DataSize <= ValueBlock->DataSize)
    {
      DataBlock = CmiGetBlock(RegistryFile, ValueBlock->DataOffset);
      RtlCopyMemory(DataBlock, Data, DataSize);
      ValueBlock->DataSize = DataSize;
      ValueBlock->DataType = Type;
      CmiReleaseBlock(RegistryFile, DataBlock);
    }
  else
    {
      /*  Destroy current data block and allocate a new one  */
      DataBlock = CmiGetBlock(RegistryFile, ValueBlock->DataOffset);
      Status = CmiAllocateBlock(RegistryFile,
                                &NewDataBlock,
                                DataSize);
      RtlCopyMemory(NewDataBlock, Data, DataSize);
      ValueBlock->DataOffset = CmiGetBlockOffset(RegistryFile, DataBlock);
      ValueBlock->DataSize = DataSize;
      ValueBlock->DataType = Type;
      CmiReleaseBlock(RegistryFile, NewDataBlock);
      CmiDestroyBlock(RegistryFile, DataBlock);
    }

  return  Status;
}

static NTSTATUS
CmiDestroyValueBlock(PREGISTRY_FILE  RegistryFile,
                     PVALUE_BLOCK  ValueBlock)
{
  NTSTATUS  Status;

  Status = CmiDestroyBlock(RegistryFile, 
                           CmiGetBlock(RegistryFile,
                                       ValueBlock->DataOffset));
  if (!NT_SUCCESS(Status))
    {
      return  Status;
    }
  return  CmiDestroyBlock(RegistryFile, ValueBlock);
}

static NTSTATUS
CmiAllocateBlock(PREGISTRY_FILE  RegistryFile,
                 PVOID  *Block,
                 ULONG  BlockSize)
{
  NTSTATUS  Status;
  PVOID  NewBlock;

  Status = STATUS_SUCCESS;

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
          CmiLockBlock(RegistryFile, NewBlock);
          *Block = NewBlock;
        }
    }
  else
    {
      Status = STATUS_NOT_IMPLEMENTED;
    }

  return  Status;
}

static NTSTATUS
CmiDestroyBlock(PREGISTRY_FILE  RegistryFile,
                PVOID  Block)
{
  NTSTATUS  Status;

  Status = STATUS_SUCCESS;

  if (RegistryFile->Filename == NULL)
    {
      CmiReleaseBlock(RegistryFile, Block);
      ExFreePool(Block);
    }
  else
    {  
      Status = STATUS_NOT_IMPLEMENTED;
    }

  return  Status;
}

static PVOID
CmiGetBlock(PREGISTRY_FILE  RegistryFile,
            BLOCK_OFFSET  BlockOffset)
{
  PVOID  Block;

  Block = NULL;
  if (RegistryFile->Filename == NULL)
    {
      CmiLockBlock(RegistryFile, (PVOID) BlockOffset);

      Block = (PVOID) BlockOffset;
    }
  else
    {
      UNIMPLEMENTED;
    }

  return  Block;
}

static BLOCK_OFFSET
CmiGetBlockOffset(PREGISTRY_FILE  RegistryFile,
                  PVOID  Block)
{
  BLOCK_OFFSET  BlockOffset;

  if (RegistryFile->Filename == NULL)
    {
      BlockOffset = (BLOCK_OFFSET) Block;
    }
  else
    {
      UNIMPLEMENTED;
    }

  return BlockOffset;
}

static VOID 
CmiLockBlock(PREGISTRY_FILE  RegistryFile,
             PVOID  Block)
{
  if (RegistryFile->Filename != NULL)
    {
      UNIMPLEMENTED;
    }
}

static VOID 
CmiReleaseBlock(PREGISTRY_FILE  RegistryFile,
               PVOID  Block)
{
  if (RegistryFile->Filename != NULL)
    {
      UNIMPLEMENTED;
    }
}

#endif

/* EOF */
