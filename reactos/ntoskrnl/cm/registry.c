/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/cm/registry.c
 * PURPOSE:         Registry functions
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

#include <ddk/ntddk.h>
#include <internal/ob.h>
#include <wchar.h>

//#define NDEBUG
#include <internal/debug.h>

//#define  PROTO_REG  1  /* Comment out to disable */

/*  -----------------------------------------------------  Typedefs  */

#if PROTO_REG

#define  REG_BLOCK_SIZE  4096
#define  REG_HEAP_BLOCK_DATA_OFFSET  32
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
  DWORD  SubBlockId;
  DWORD  Type;
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
  DWORD  SubBlockId;
  DWORD  HashTableSize;
  HASH_RECORD  Table[1];
} HASH_TABLE_BLOCK, *PHASH_TABLE_BLOCK;

typedef struct _REGISTRY_FILE
{
  PWSTR  Filename;
  PVOID  Data;
  PVOID  (*Extend)(PVOID *Data, ULONG NewSize);
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
static PKEY_OBJECT  CmiScanKeyList(PWSTR  KeyNameBuf);
static PREGISTRY_FILE  CmiCreateRegistry(PWSTR  Filename);
static PVOID  CmiExtendVolatileFile(PVOID *Data, ULONG NewSize);
static NTSTATUS  CmiCreateKey(PREGISTRY_FILE  RegistryFile,
                              PWSTR  KeyNameBuf,
                              PKEY_BLOCK  *KeyBlock,
                              ACCESS_MASK DesiredAccess,
                              ULONG TitleIndex,
                              PUNICODE_STRING Class, 
                              ULONG CreateOptions, 
                              PULONG Disposition);
static NTSTATUS  CmiFindKey(PREGISTRY_FILE  RegistryFile,
                            PWSTR  KeyNameBuf,
                            PKEY_BLOCK  *KeyBlock,
                            ACCESS_MASK DesiredAccess,
                            ULONG TitleIndex,
                            PUNICODE_STRING Class);
static PHASH_TABLE_BLOCK  CmiGetHashTableBlock(PREGISTRY_FILE  RegistryFile,
                                               PKEY_BLOCK  KeyBlock);
static PKEY_BLOCK  CmiGetKeyFromHashByIndex(PREGISTRY_FILE RegistryFile,
                                            PHASH_TABLE_BLOCK  HashTableBlock,
                                            ULONG  Index);
static VOID CmiLockBlock(PREGISTRY_FILE  RegistryFile,
                         PVOID  Block);
static VOID CmiUnlockBlock(PREGISTRY_FILE  RegistryFile,
                           PVOID  Block);
static PWSTR  CmiGetClassBlock(PREGISTRY_FILE  RegistryFile,
                               PKEY_BLOCK  KeyBlock);
static ULONG  CmiGetMaxNameLength(PREGISTRY_FILE  RegistryFile,
                                  PKEY_BLOCK  KeyBlock);
static ULONG  CmiGetMaxClassLength(PREGISTRY_FILE  RegistryFile,
                                   PKEY_BLOCK  KeyBlock);
static ULONG  CmiGetMaxValueNameLength(PREGISTRY_FILE  RegistryFile,
                                       PKEY_BLOCK  KeyBlock);
static ULONG  CmiGetMaxValueDataLength(PREGISTRY_FILE  RegistryFile,
                                       PKEY_BLOCK  KeyBlock);
#endif

/*  ---------------------------------------------  Public Interface  */

VOID
CmInitializeRegistry(VOID)
{
#if PROTO_REG
  HANDLE  RootKeyHandle;
  UNICODE_STRING  RootKeyName;
  OBJECT_ATTRIBUTES  ObjectAttributes;
  
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
  RtlInitUnicodeString(&RootKeyName, L"\\Registry");
  InitializeObjectAttributes(&ObjectAttributes, &RootKeyName, 0, NULL, NULL);
  ObCreateObject(&RootKeyHandle,
                 STANDARD_RIGHTS_REQUIRED,
                 &ObjectAttributes,
                 CmiKeyType);

  KeInitializeSpinLock(&CmiKeyListLock);

  /*  Build volitile registry store  */
  CmiVolatileFile = CmiCreateRegistry(NULL);

  /*  Build system registry store  */
  CmiSystemFile = CmiCreateRegistry(SYSTEM_REG_FILE);

  /* FIXME: Create initial predefined symbolic links  */
  /* HKEY_LOCAL_MACHINE  */
  /* HKEY_USERS  */
  /* FIXME: load volatile registry data from ROSDTECT  */

#endif
}

NTSTATUS 
NtCreateKey(OUT PHANDLE  KeyHandle, 
            IN ACCESS_MASK  DesiredAccess,
            IN POBJECT_ATTRIBUTES  ObjectAttributes, 
            IN ULONG  TitleIndex,
            IN PUNICODE_STRING  Class, 
            IN ULONG  CreateOptions, 
            OUT PULONG  Disposition)
{
  return ZwCreateKey(KeyHandle, 
                     DesiredAccess,
                     ObjectAttributes, 
                     TitleIndex,
                     Class, 
                     CreateOptions,
                     Disposition);
}

NTSTATUS 
ZwCreateKey(OUT PHANDLE  KeyHandle,
            IN ACCESS_MASK  DesiredAccess,
            IN POBJECT_ATTRIBUTES  ObjectAttributes, 
            IN ULONG  TitleIndex,
            IN PUNICODE_STRING  Class, 
            IN ULONG  CreateOptions,
            OUT PULONG  Disposition)
{
#if PROTO_REG
  PWSTR  KeyNameBuf;
  NTSTATUS  Status;
  PKEY_OBJECT  CurKey, NewKey;
  PREGISTRY_FILE  FileToUse;
  PKEY_BLOCK  KeyBlock;

  assert(ObjectAttributes != NULL);

  FileToUse = (CreateOptions & REG_OPTION_NON_VOLATILE) ? 
    CmiSystemFile : CmiVolatileFile;
  
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
NtDeleteKey(IN HANDLE  KeyHandle)
{
  return  ZwDeleteKey(KeyHandle);
}

NTSTATUS 
ZwDeleteKey(IN HANDLE  KeyHandle)
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
NtEnumerateKey(IN HANDLE  KeyHandle, 
               IN ULONG  Index,
               IN KEY_INFORMATION_CLASS  KeyInformationClass,
               OUT PVOID  KeyInformation,
               IN ULONG  Length,
               OUT PULONG  ResultLength)
{
  return  ZwEnumerateKey(KeyHandle,
                         Index,
                         KeyInformationClass,
                         KeyInformation,
                         Length,
                         ResultLength);
}

NTSTATUS 
ZwEnumerateKey(IN HANDLE  KeyHandle, 
               IN ULONG  Index,
               IN KEY_INFORMATION_CLASS  KeyInformationClass,
               OUT PVOID  KeyInformation,
               IN ULONG  Length,
               OUT PULONG  ResultLength)
{
#ifdef PROTO_REG
  NTSTATUS  Status;
  PKEY_OBJECT  KeyObject;
  PREGISTRY_FILE  RegistryFile;
  PKEY_BLOCK  KeyBlock, SubKeyBlock;
  PHASH_TABLE_BLOCK  HashTableBlock;
  PWSTR  ClassBlock;
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
  HashTableBlock = CmiGetHashTableBlock(RegistryFile, KeyBlock);
  SubKeyBlock = CmiGetKeyFromHashByIndex(RegistryFile, 
                                         HashTableBlock, 
                                         Index);
  if (SubKeyBlock == NULL)
    {
      return  STATUS_NO_MORE_ENTRIES;
    }

  CmiLockBlock(RegistryFile, SubKeyBlock);
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
              ClassBlock = CmiGetClassBlock(RegistryFile, SubKeyBlock);
              
              wcsncpy(NodeInformation->Name + SubKeyBlock->NameSize + 1,
                      ClassBlock,
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
          ClassBlock = CmiGetClassBlock(RegistryFile, SubKeyBlock);
          CmiLockBlock(RegistryFile, ClassBlock);
          wcsncpy(FullInformation->Class,
                  ClassBlock,
                  SubKeyBlock->ClassSize);
          CmiUnlockBlock(RegistryFile, ClassBlock);
          FullInformation->Class[SubKeyBlock->ClassSize] = 0;
          *ResultLength = sizeof(KEY_FULL_INFORMATION) +
            SubKeyBlock->ClassSize * sizeof(WCHAR);
        }
      break;
    }
  CmiUnlockBlock(RegistryFile, SubKeyBlock);

  return  Status;
#else
  UNIMPLEMENTED;
#endif
}

NTSTATUS 
NtEnumerateValueKey(IN HANDLE  KeyHandle, 
                    IN ULONG  Index, 
                    IN KEY_VALUE_INFORMATION_CLASS  KeyInformationClass,
                    OUT PVOID  KeyInformation,
                    IN ULONG  Length,
                    OUT PULONG  ResultLength)
{
  return  ZwEnumerateValueKey(KeyHandle, 
                              Index, 
                              KeyInformationClass,
                              KeyInformation,
                              Length,
                              ResultLength);
}

NTSTATUS 
ZwEnumerateValueKey(IN HANDLE  KeyHandle, 
                    IN ULONG  Index, 
                    IN KEY_VALUE_INFORMATION_CLASS  KeyInformationClass,
                    OUT PVOID  KeyInformation,
                    IN ULONG  Length,
                    OUT PULONG  ResultLength)
{
#ifdef PROTO_REG
  UNIMPLEMENTED;  
#else
  UNIMPLEMENTED;
#endif
}

NTSTATUS 
NtFlushKey(IN HANDLE  KeyHandle)
{
  return ZwFlushKey(KeyHandle);
}

NTSTATUS 
ZwFlushKey(IN HANDLE  KeyHandle)
{
#ifdef PROTO_REG
  return  STATUS_SUCCESS;
#else
  UNIMPLEMENTED;
#endif
}

NTSTATUS 
NtOpenKey(OUT PHANDLE  KeyHandle, 
          IN ACCESS_MASK  DesiredAccess,
          IN POBJECT_ATTRIBUTES  ObjectAttributes)
{
  return ZwOpenKey(KeyHandle, 
                   DesiredAccess,
                   ObjectAttributes);
}

NTSTATUS 
ZwOpenKey(OUT PHANDLE  KeyHandle, 
          IN ACCESS_MASK  DesiredAccess,
          IN POBJECT_ATTRIBUTES  ObjectAttributes)
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
NtQueryKey(IN HANDLE  KeyHandle, 
           IN KEY_INFORMATION_CLASS  KeyInformationClass,
           OUT PVOID  KeyInformation,
           IN ULONG  Length,
           OUT PULONG  ResultLength)
{
  return ZwQueryKey(KeyHandle, 
                    KeyInformationClass,
                    KeyInformation,
                    Length,
                    ResultLength);
}

NTSTATUS 
ZwQueryKey(IN HANDLE  KeyHandle, 
           IN KEY_INFORMATION_CLASS  KeyInformationClass,
           OUT PVOID  KeyInformation,
           IN ULONG  Length,
           OUT PULONG  ResultLength)
{
#ifdef PROTO_REG
  NTSTATUS  Status;
  PKEY_OBJECT  KeyObject;
  PREGISTRY_FILE  RegistryFile;
  PKEY_BLOCK  KeyBlock;
  PWSTR  ClassBlock;
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
    
  CmiLockBlock(RegistryFile, KeyBlock);
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
              ClassBlock = CmiGetClassBlock(RegistryFile, KeyBlock);
              CmiLockBlock(RegistryFile, ClassBlock);
              wcsncpy(NodeInformation->Name + KeyBlock->NameSize + 1,
                      ClassBlock,
                      KeyBlock->ClassSize);
              CmiUnlockBlock(RegistryFile, ClassBlock);
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
          ClassBlock = CmiGetClassBlock(RegistryFile, KeyBlock);
          CmiLockBlock(RegistryFile, ClassBlock);
          wcsncpy(FullInformation->Class,
                  ClassBlock,
                  KeyBlock->ClassSize);
          CmiUnlockBlock(RegistryFile, ClassBlock);
          FullInformation->Class[KeyBlock->ClassSize] = 0;
          *ResultLength = sizeof(KEY_FULL_INFORMATION) +
            KeyBlock->ClassSize * sizeof(WCHAR);
        }
      break;
    }
  CmiUnlockBlock(RegistryFile, KeyBlock);

  return  Status;
#else
  UNIMPLEMENTED;
#endif
}

NTSTATUS 
NtQueryValueKey(IN HANDLE  KeyHandle,
                IN PUNICODE_STRING  ValueName,
                IN KEY_VALUE_INFORMATION_CLASS  KeyValueInformationClass,
                OUT PVOID  KeyValueInformation,
                IN ULONG  Length,
                OUT PULONG  ResultLength)
{
  return ZwQueryValueKey(KeyHandle,
                         ValueName,
                         KeyValueInformationClass,
                         KeyValueInformation,
                         Length,
                         ResultLength);
}

NTSTATUS 
ZwQueryValueKey(IN HANDLE  KeyHandle,
                IN PUNICODE_STRING  ValueName,
                IN KEY_VALUE_INFORMATION_CLASS  KeyValueInformationClass,
                OUT PVOID  KeyValueInformation,
                IN ULONG  Length,
                OUT PULONG  ResultLength)
{
#ifdef PROTO_REG
  NTSTATUS  Status;
  PKEY_OBJECT  KeyObject;
  PREGISTRY_FILE  RegistryFile;
  PKEY_BLOCK  KeyBlock;
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
    
  Status = STATUS_SUCCESS;
  CmiLockBlock(RegistryFile, KeyBlock);
  
  /* FIXME: get value list block  */
  /* FIXME: loop through value list  */
    /* FIXME: get value block for current list entry  */
    /* FIXME: compare name with desired value name */
      /* FIXME: if name is correct */
      /* FIXME: check size of input buffer  */
      /* FIXME: copy data into buffer  */
      /* FIXME: set ResultLength  */
  
  CmiUnlockBlock(RegistryFile, KeyBlock);
  
  return  Status;
#else
   UNIMPLEMENTED;
#endif
}

NTSTATUS NtSetValueKey(HANDLE KeyHandle, PUNICODE_STRING ValueName,
                       ULONG TitleIndex, ULONG Type, PVOID Data,
                       ULONG DataSize)
{
  return ZwSetValueKey(KeyHandle, 
                       ValueName,
                       TitleIndex, 
                       Type, 
                       Data,
                       DataSize);
}

NTSTATUS ZwSetValueKey(HANDLE KeyHandle, PUNICODE_STRING ValueName,
                       ULONG TitleIndex, ULONG Type, PVOID Data,
                       ULONG DataSize)
{
   UNIMPLEMENTED;
}

NTSTATUS
STDCALL
NtDeleteValueKey(IN HANDLE KeyHandle,
                 IN PUNICODE_STRING ValueName)
{
  return ZwDeleteValueKey(KeyHandle,
                          ValueName);
}

NTSTATUS
STDCALL
ZwDeleteValueKey(
	IN HANDLE KeyHandle,
	IN PUNICODE_STRING ValueName	
		 )
{
  UNIMPLEMENTED;
}

NTSTATUS
STDCALL 
NtLoadKey(
	PHANDLE KeyHandle,
	OBJECT_ATTRIBUTES ObjectAttributes
	)
{
  return ZwLoadKey(KeyHandle,
                   ObjectAttributes);
}

NTSTATUS
STDCALL 
ZwLoadKey(
	PHANDLE KeyHandle,
	OBJECT_ATTRIBUTES ObjectAttributes
	)
{
  UNIMPLEMENTED;
}

NTSTATUS STDCALL NtLoadKey2(VOID)
{
  UNIMPLEMENTED;
}

NTSTATUS
STDCALL
NtNotifyChangeKey(
	IN HANDLE KeyHandle,
	IN HANDLE Event,
	IN PIO_APC_ROUTINE ApcRoutine OPTIONAL, 
	IN PVOID ApcContext OPTIONAL, 
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN ULONG CompletionFilter,
	IN BOOLEAN Asynchroneous, 
	OUT PVOID ChangeBuffer,
	IN ULONG Length,
	IN BOOLEAN WatchSubtree
	)
{
  return ZwNotifyChangeKey(KeyHandle,
                           Event,
                           ApcRoutine, 
                           ApcContext, 
                           IoStatusBlock,
                           CompletionFilter,
                           Asynchroneous, 
                           ChangeBuffer,
                           Length,
                           WatchSubtree);
}

NTSTATUS
STDCALL
ZwNotifyChangeKey(
	IN HANDLE KeyHandle,
	IN HANDLE Event,
	IN PIO_APC_ROUTINE ApcRoutine OPTIONAL, 
	IN PVOID ApcContext OPTIONAL, 
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN ULONG CompletionFilter,
	IN BOOLEAN Asynchroneous, 
	OUT PVOID ChangeBuffer,
	IN ULONG Length,
	IN BOOLEAN WatchSubtree
	)
{
  UNIMPLEMENTED;
}

NTSTATUS
STDCALL
NtQueryMultipleValueKey(
   HANDLE KeyHandle,	
   PVALENT ListOfValuesToQuery,	
   ULONG NumberOfItems,	
   PVOID MultipleValueInformation,		
   ULONG Length,
   PULONG  ReturnLength
)
{
  return ZwQueryMultipleValueKey(KeyHandle,	
                                 ListOfValuesToQuery,	
                                 NumberOfItems,	
                                 MultipleValueInformation,		
                                 Length,
                                 ReturnLength);
}

NTSTATUS
STDCALL
ZwQueryMultipleValueKey(
   HANDLE KeyHandle,	
   PVALENT ListOfValuesToQuery,	
   ULONG NumberOfItems,	
   PVOID MultipleValueInformation,		
   ULONG Length,
   PULONG  ReturnLength
)
{
  UNIMPLEMENTED;
}

NTSTATUS
STDCALL
NtReplaceKey(
	IN POBJECT_ATTRIBUTES ObjectAttributes, 
	IN HANDLE Key,
	IN POBJECT_ATTRIBUTES ReplacedObjectAttributes 
	)
{
  return ZwReplaceKey(ObjectAttributes, 
                      Key,
                      ReplacedObjectAttributes);
}

NTSTATUS
STDCALL
ZwReplaceKey(
	IN POBJECT_ATTRIBUTES ObjectAttributes, 
	IN HANDLE Key,
	IN POBJECT_ATTRIBUTES ReplacedObjectAttributes 
	)
{
  UNIMPLEMENTED;
}

NTSTATUS
STDCALL
NtRestoreKey(
	HANDLE KeyHandle,
	HANDLE FileHandle,
	ULONG RestoreFlags
	)
{
  return ZwRestoreKey(KeyHandle,
                      FileHandle,
                      RestoreFlags);
}

NTSTATUS
STDCALL
ZwRestoreKey(
	HANDLE KeyHandle,
	HANDLE FileHandle,
	ULONG RestoreFlags
	)
{
  UNIMPLEMENTED;
}

NTSTATUS
STDCALL
NtSaveKey(
	IN HANDLE KeyHandle,
	IN HANDLE FileHandle
	)
{
  return ZwSaveKey(KeyHandle,
                   FileHandle);
}

NTSTATUS
STDCALL
ZwSaveKey(
	IN HANDLE KeyHandle,
	IN HANDLE FileHandle
	)
{
  UNIMPLEMENTED;
}

NTSTATUS
STDCALL
NtSetInformationKey(
	IN HANDLE KeyHandle,
	IN CINT KeyInformationClass,
	IN PVOID KeyInformation,
	IN ULONG KeyInformationLength
	)
{
  return ZwSetInformationKey(KeyHandle,
                             KeyInformationClass,
                             KeyInformation,
                             KeyInformationLength);
}

NTSTATUS STDCALL ZwSetInformationKey(IN HANDLE KeyHandle,
				     IN CINT KeyInformationClass,
				     IN PVOID KeyInformation,
				     IN ULONG KeyInformationLength)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtUnloadKey(HANDLE KeyHandle)
{
  return ZwUnloadKey(KeyHandle);
}

NTSTATUS STDCALL ZwUnloadKey(HANDLE KeyHandle)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtInitializeRegistry(BOOLEAN SetUpBoot)
{
  return ZwInitializeRegistry(SetUpBoot);
}

NTSTATUS STDCALL ZwInitializeRegistry(BOOLEAN SetUpBoot)
{
   UNIMPLEMENTED;
}

NTSTATUS RtlCheckRegistryKey(ULONG RelativeTo, PWSTR Path)
{
   UNIMPLEMENTED;
}

NTSTATUS RtlCreateRegistryKey(ULONG RelativeTo, PWSTR Path)
{
   UNIMPLEMENTED;
}

NTSTATUS RtlDeleteRegistryValue(ULONG RelativeTo, PWSTR Path,
				PWSTR ValueName)
{
   UNIMPLEMENTED;
}

NTSTATUS RtlQueryRegistryValues(ULONG RelativeTo,
				PWSTR Path,
				PRTL_QUERY_REGISTRY_TABLE QueryTable,
				PVOID Context,
				PVOID Environment)
{
   UNIMPLEMENTED;
}

NTSTATUS RtlWriteRegistryValue(ULONG RelativeTo,
			       PWSTR Path,
			       PWSTR ValueName,
			       ULONG ValueType,
			       PVOID ValueData,
			       ULONG ValueLength)
{
   UNIMPLEMENTED;
}

/*  ------------------------------------------  Private Implementation  */

#if PROTO_REG

static PVOID 
CmiObjectParse(PVOID  ParsedObject, PWSTR  *Path)
{
  UNIMPLEMENTED
}

static VOID  
CmiObjectDelete(PVOID  DeletedObject)
{
  /* FIXME: if marked for delete, then call CmiDeleteKey  */
  UNIMPLEMENTED;
}

static NTSTATUS
CmiBuildKeyPath(PWSTR  *KeyPath, POBJECT_ATTRIBUTES  ObjectAttributes)
{
  ULONG  KeyNameSize;
  PWSTR  KeyNameBuf;

  /* FIXME: Verify ObjectAttributes is in \\Registry space and comppute size for path */
  KeyNameSize = 0;
  if (ObjectAttributes->RootDirectory != NULL)
    {
      /* FIXME: determine type of object for RootDirectory  */
      /* FIXME: if object type is ObDirType  */
        /* FIXME: fail if RootDirectory != '\\'  */
        /* FIXME: check for 'Registry' in ObjectName, fail if missing  */
        /* FIXME: add size for remainder to KeyNameSize  */
      /* FIXME: else if object type is CmiKeyType  */
        /* FIXME: add size of Name from RootDirectory object to KeyNameSize  */
        /* FIXME: add 1 to KeyNamesize for '\\'  */
        /* FIXME: add size of ObjectName to KeyNameSize  */
      /* FIXME: else fail on incorrect type  */
    }
  else
    {
      /* FIXME: check for \\Registry and fail if missing  */
      /* FIXME: add size of remainder to KeyNameSize  */
    }

  KeyNameBuf = ExAllocatePool(NonPagedPool, KeyNameSize);

  /* FIXME: Construct relative pathname  */
  KeyNameBuf[0] = 0;
  if (ObjectAttributes->RootDirectory != NULL)
    {
      /* FIXME: determine type of object for RootDirectory  */
      /* FIXME: if object type is ObDirType  */
        /* FIXME: fail if RootDirectory != '\\'  */
        /* FIXME: check for 'Registry' in ObjectName, fail if missing  */
        /* FIXME: copy remainder into KeyNameBuf  */
      /* FIXME: else if object type is CmiKeyType  */
        /* FIXME: copy Name from RootDirectory object into KeyNameBuf  */
        /* FIXME: append '\\' into KeyNameBuf  */
        /* FIXME: append ObjectName into KeyNameBuf  */
    }
  else
    {
      /* FIXME: check for \\Registry\\ and fail if missing  */
      /* FIXME: copy remainder into KeyNameBuf  */
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
  ULONG  Idx;
  PREGISTRY_FILE  RegistryFile;
  PHEADER_BLOCK  HeaderBlock;
  PHEAP_BLOCK  HeapBlock;
  PFREE_SUB_BLOCK  FreeSubBlock;

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
      RegistryFile->Data = NULL;
      RegistryFile->Extend = CmiExtendVolatileFile;
      RegistryFile->Flush = NULL;
      RegistryFile->Data = CmiExtendVolatileFile(NULL, REG_BLOCK_SIZE * 2);
      RtlZeroMemory(RegistryFile->Data, REG_BLOCK_SIZE * 2);
      HeaderBlock = (PHEADER_BLOCK) RegistryFile->Data;
      HeaderBlock->BlockId = 0x66676572;
      HeaderBlock->DateModified.QuadPart = 0;
      HeaderBlock->Unused2 = 1;
      HeaderBlock->Unused3 = 3;
      HeaderBlock->Unused5 = 1;
      HeaderBlock->RootKeyBlock = 0;
      HeaderBlock->BlockSize = REG_BLOCK_SIZE;
      HeaderBlock->Unused6 = 1;
      HeaderBlock->Checksum = 0;
      for (Idx = 0; Idx < 127; Idx++)
        {
          HeaderBlock->Checksum += ((DWORD *)HeaderBlock)[Idx];
        }
      HeapBlock = (PHEAP_BLOCK) 
        ((DWORD)RegistryFile->Data + REG_BLOCK_SIZE);
      HeapBlock->BlockId = 0x6e696268;
      HeapBlock->PreviousHeapBlock = -1;
      HeapBlock->NextHeapBlock = -1;
      HeapBlock->BlockSize = REG_BLOCK_SIZE;
      FreeSubBlock = (PFREE_SUB_BLOCK) 
        ((DWORD)HeapBlock + REG_HEAP_BLOCK_DATA_OFFSET);
      FreeSubBlock->SubBlockSize = 
        -(REG_BLOCK_SIZE - REG_HEAP_BLOCK_DATA_OFFSET);
    }

  return  RegistryFile;
}

static NTSTATUS
CmiCreateKey(PREGISTRY_FILE  RegistryFile,
             PWSTR  KeyNameBuf,
             PKEY_BLOCK  *KeyBlock,
             ACCESS_MASK DesiredAccess,
             ULONG TitleIndex,
             PUNICODE_STRING Class, 
             ULONG CreateOptions, 
             PULONG Disposition)
{
  UNIMPLEMENTED;
}

static NTSTATUS  
CmiFindKey(PREGISTRY_FILE  RegistryFile,
           PWSTR  KeyNameBuf,
           PKEY_BLOCK  *KeyBlock,
           ACCESS_MASK DesiredAccess,
           ULONG TitleIndex,
           PUNICODE_STRING Class)
{
  UNIMPLEMENTED;
}

static PHASH_TABLE_BLOCK  
CmiGetHashTableBlock(PREGISTRY_FILE  RegistryFile,
                     PKEY_BLOCK  KeyBlock)
{
  UNIMPLEMENTED;
}

static PKEY_BLOCK  
CmiGetKeyFromHashByIndex(PREGISTRY_FILE RegistryFile,
                         PHASH_TABLE_BLOCK  HashTableBlock,
                         ULONG  Index)
{
  UNIMPLEMENTED;
}

static VOID 
CmiLockBlock(PREGISTRY_FILE  RegistryFile,
             PVOID  Block)
{
  UNIMPLEMENTED;
}

static VOID 
CmiUnlockBlock(PREGISTRY_FILE  RegistryFile,
               PVOID  Block)
{
  UNIMPLEMENTED;
}

static PWSTR  
CmiGetClassBlock(PREGISTRY_FILE  RegistryFile,
                 PKEY_BLOCK  KeyBlock)
{
  UNIMPLEMENTED;
}

static ULONG  
CmiGetMaxNameLength(PREGISTRY_FILE  RegistryFile,
                    PKEY_BLOCK  KeyBlock)
{
  UNIMPLEMENTED;
}

static ULONG  
CmiGetMaxClassLength(PREGISTRY_FILE  RegistryFile,
                     PKEY_BLOCK  KeyBlock)
{
  UNIMPLEMENTED;
}

static ULONG  
CmiGetMaxValueNameLength(PREGISTRY_FILE  RegistryFile,
                         PKEY_BLOCK  KeyBlock)
{
  UNIMPLEMENTED;
}

static ULONG  
CmiGetMaxValueDataLength(PREGISTRY_FILE  RegistryFile,
                         PKEY_BLOCK  KeyBlock)
{
  UNIMPLEMENTED;
}

#endif


