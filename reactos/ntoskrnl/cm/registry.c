/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/cm/registry.c
 * PURPOSE:         Registry functions
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

// #define  PROTO_REG  1  /* Comment out to disable */

/* FILE STATICS *************************************************************/

#if PROTO_REG

typedef DWORD  BLOCK_OFFSET;

typedef struct _KEY_BLOCK
{
  DWORD  SubBlockId;
  DWORD  Type;
  LARGE_INTEGER  LastWriteTime;
  BLOCK_OFFSET  ParentKeyOffset;
  DWORD  NumberOfSubKeys;
  BLOCK_OFFSET  HashTableOffset;
  DWORD  NumberValues;
  BLOCK_OFFSET  ValuesOffset;
  BLOCK_OFFSET  SecurityKeyOffset;
  BLOCK_OFFSET  ClassNameOffset;
  DWORD  Unused1;
  DWORD  NameSize;
  DWORD  ClassSize;
  CHAR  Name[1];
} KEY_BLOCK, *PKEY_BLOCK;

typedef struct _HASH_RECORD
{
  BLOCK_OFFSET  KeyOffset;
  ULONG  HashValue;
} HASH_RECORD, *PHASH_RECORD;

typedef struct _HASH_BLOCK
{
  DWORD  SubBlockId;
  DWORD  HashTableSize;
  HASH_RECORD  Table[1];
} HASH_BLOCK, *PHASH_BLOCK;

/*  Type defining the Object Manager Key Object  */
typedef struct _KEY_OBJECT
{
  CSHORT  Type;
  CSHORT  Size;
  
  ULONG  Flags;
  WCHAR  *Name;
  PKEY_BLOCK  KeyBlock;
  struct _KEY_OBJECT  *NextKey;
} KEY_OBJECT, *PKEY_OBJECT;

#define  KO_MARKED_FOR_DELETE  0x00000001

typedef struct _REGISTRY_FILE
{
  PWSTR  Filename;
  PVOID  Data;
  PVOID  (*Extend)(PVOID *Data, ULONG NewSize);
  PVOID  (*Flush)(VOID);
} REGISTRY_FILE, *PREGISTRY_FILE;

static POBJECT_TYPE  CmiKeyType = NULL;
static PKEY_OBJECT  CmiKeyList = NULL;
static KSPIN_LOCK  CmiKeyListLock;
static PREGISTRY_FILE  CmiVolatileFile = NULL;
static PREGISTRY_FILE  CmiSystemFile = NULL;

static PVOID  CmiObjectParse(PVOID  ParsedObject, PWSTR  *Path);
static PVOID  CmiObjectDelete(PVOID  DeletedObject);
static VOID  CmiAddKeyToList(PKEY_OBJECT  NewKey);
static PKEY_OBJECT  CmiScanKeyList(PWSTR  *KeyNameBuf);
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
#endif

/* FUNCTIONS *****************************************************************/

VOID
CmInitializeRegistry(VOID)
{
#if PROTO_REG
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
  CmiKeyType->Delete = CmiKeyDelete;
  CmiKeyType->Parse = CmiKeyParse;
  CmiKeyType->Security = NULL;
  CmiKeyType->QueryName = NULL;
  CmiKeyType->OkayToClose = NULL;
  RtlInitUnicodeString(&CmiKeyType->TypeName, L"Key");

  /*  Build the Root Key Object  */
  /*  FIXME: This should be split into two objects, 1 system and 1 user  */
  RtlInitUnicodeString(&RootKeyName, L"\\Registry");
  InitializeObjectAttributes(&ObjectAttributes, &RootKeyName, 0, NULL, NULL);
  Status = ObCreateObject(&RootKeyHandle,
                          STANDARD_RIGHTS_REQUIRED,
                          &ObjectAttributes,
                          CmiKeyType);

  KeInitializeSpinLock(&CmiKeylistLock);

  /* FIXME: build volitile registry store  */
  /* FIXME: map / build registry data  */
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
  PKEY_TYPE  CurKey;
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
  NewKey = ObGenericCreateObject(KeyHandle, 
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
  PHANDLE_REP  KeyHandleRep;
  POBJECT_HEADER  KeyObjectHeader;
  PKEY_OBJECT  KeyObject;
  
  /*  Verify that the handle is valid and is a registry key  */
  KeyHandleRep = ObTranslateHandle(PsGetCurrentProcess(), KeyHandle);
  if (KeyHandleRep == NULL)
    {
      return  STATUS_INVALID_HANDLE;
    }
  KeyObjectHeader = BODY_TO_HEADER(KeyHandleRep->ObjectBody);
  if (KeyObjectHeader->ObjectType != CmiKeyType)
    {
      return  STATUS_INVALID_HANDLE;
    }
  
  /*  Verify required access for delete  */
  if ((KeyHandleRep & KEY_DELETE_ACCESS) != KEY_DELETE_ACCESS)
    {
      return  STATUS_ACCESS_DENIED;
    }
  
  /*  Set the marked for delete bit in the key object  */
  KeyObject = (PKEY_OBJECT) KeyHandleRep->ObjectBody;
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
  PHANDLE_REP  KeyHandleRep;
  POBJECT_HEADER  KeyObjectHeader;
  PKEY_OBJECT  KeyObject;
  PKEY_BLOCK  KeyBlock, SubKeyBlock;
  PHASH_TABLE_BLOCK  HashTableBlock;
  PWSTR  ClassBlock;
  PKEY_BASIC_INFORMATION  KeyBasicInformation;
  PKEY_NODE_INFORMATION  KeyNodeInformation;
  PKEY_FULL_INFORMATION  KeyFullInformation;
    
  /*  Verify that the handle is valid and is a registry key  */
  KeyHandleRep = ObTranslateHandle(PsGetCurrentProcess(), KeyHandle);
  if (KeyHandleRep == NULL)
    {
      return  STATUS_INVALID_HANDLE;
    }
  KeyObjectHeader = BODY_TO_HEADER(KeyHandleRep->ObjectBody);
  if (KeyObjectHeader->ObjectType != CmiKeyObject)
    {
      return  STATUS_INVALID_HANDLE;
    }
  
  /*  Verify required access for enumerate  */
  if ((KeyHandleRep->AccessGranted & KEY_ENUMERATE_SUB_KEY) != 
      KEY_ENUMERATE_SUB_KEY)
    {
      return  STATUS_ACCESS_DENIED;
    }

  /*  Get pointer to KeyBlock  */
  KeyObject = (PKEY_OBJECT) KeyHandleRep->ObjectBody;
  KeyBlock = KeyObject->KeyBlock;
    
  /*  Get pointer to SubKey  */
  HashTableBlock = CmiGetHashTableBlock(KeyBlock);
  SubKeyBlock = CmiGetKeyFromHashByIndex(HashTableBlock, Index);
  if (SubKeyBlock == NULL)
    {
      return  STATUS_NO_MORE_ENTRIES;
    }

  CmiLockBlock(SubKeyBlock);
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
          KeyBasicInformation = (PKEY_BASIC_INFORMATION) KeyInformation;
          KeyBasicInformation->LastWriteTime = SubKeyBlock->LastWriteTime;
          KeyBasicInformation->TitleIndex = Index;
          KeyBasicInformation->NameLength = SubKeyBlock->NameSize;
          wcsncpy(KeyBasicInformation->Name, 
                  SubKeyBlock->Name, 
                  SubKeyBlock->NameSize);
          KeyBasicInformation->Name[SubKeyBlock->NameSize] = 0;
          *ReturnLength = sizeof(KEY_BASIC_INFORMATION) + 
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
          KeyNodeInformation = (PKEY_NODE_INFORMATION) KeyInformation;
          KeyNodeInformation->LastWriteTime = SubKeyBlock->LastWriteTime;
          KeyNodeInformation->TitleIndex = Index;
          KeyNodeInformation->ClassOffset = sizeof(KEY_NODE_INFORMATION) + 
            SubKeyBlock->NameSize * sizeof(WCHAR);
          KeyNodeInformation->ClassLength = SubKeyBlock->ClassSize;
          KeyNodeInformation->NameLength = SubKeyBlock->NameSize;
          wcsncpy(KeyNodeInformation->Name, 
                  SubKeyBlock->Name, 
                  SubKeyBlock->NameSize);
          KeyNodeInformation->Name[SubKeyBlock->NameSize] = 0;
          if (SubKeyBlock->ClassSize != 0)
            {
              ClassBlock = CmiGetClassBlock(SubKeyBlock);
              
              wcsncpy(KeyNodeInformation->Name + SubKeyBlock->NameSize + 1,
                      ClassBlock,
                      SubKeyBlock->ClassSize);
              KeyNodeInformation->
                Name[SubKeyBlock->NameSize + 1 + SubKeyBlock->ClassSize] = 0;
            }
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
          KeyFullInformation = (PKEY_FULL_INFORMATION) KeyInformation;
          KeyFullInformation->LastWriteTime = SubKeyBlock->LastWriteTime;
          KeyFullInformation->TitleIndex = Index;
          KeyFullInformation->ClassOffset = sizeof(KEY_FULL_INFORMATION) - 
            sizeof(WCHAR);
          KeyFullInformation->ClassLength = SubKeyBlock->ClassSize;
          KeyFullInformation->SubKeys = SubKeyBlock->NumberOfSubKeys;
          KeyFullInformation->MaxNameLen = CmiGetMaxNameLength(SubKeyBlock);
          KeyFullInformation->MaxClassLen = CmiGetMaxClassLength(SubKeyBlock);
          KeyFullInformation->Values = ;
          KeyFullInformation->MaxValueNameLen = 
            CmiGetMaxValueNameLength(SubKeyBlock);
          KeyFullInformation->MaxValueDataLen = 
            CmiGetMaxValueDataLength(SubKeyBlock);
          ClassBlock = CmiGetClassBlock(SubKeyBlock);
          CmiLockBlock(ClassBlock);
          wcsncpy(KeyFullInformation->Class,
                  ClassBlock,
                  SubKeyBlock->ClassSize);
          CmiUnlockBlock(ClassBlock);
          KeyFullInformation->Class[SubKeyBlock->ClassSize] = 0;
        }
      break;
    }
  CmiUnlockBlock(SubKeyBlock);

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
  PWSTR  KeyNameBuf;
  PKEY_BLOCK  KeyBlock;
  PKEY_OBJECT  CurKey;
  
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

  /*  Create new key object and put into linked list  */
  NewKey = ObGenericCreateObject(KeyHandle, 
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
  PHANDLE_REP  KeyHandleRep;
  POBJECT_HEADER  KeyObjectHeader;
  PKEY_OBJECT  KeyObject;
  PKEY_BLOCK  KeyBlock;
  PWSTR  ClassBlock;
  PKEY_BASIC_INFORMATION  KeyBasicInformation;
  PKEY_NODE_INFORMATION  KeyNodeInformation;
  PKEY_FULL_INFORMATION  KeyFullInformation;
    
  /*  Verify that the handle is valid and is a registry key  */
  KeyHandleRep = ObTranslateHandle(PsGetCurrentProcess(), KeyHandle);
  if (KeyHandleRep == NULL)
    {
      return  STATUS_INVALID_HANDLE;
    }
  KeyObjectHeader = BODY_TO_HEADER(KeyHandleRep->ObjectBody);
  if (KeyObjectHeader->ObjectType != CmiKeyObject)
    {
      return  STATUS_INVALID_HANDLE;
    }
  
  /*  Verify required access for enumerate  */
  if ((KeyHandleRep->AccessGranted & KEY_QUERY_KEY) != KEY_QUERY_KEY)
    {
      return  STATUS_ACCESS_DENIED;
    }

  /*  Get pointer to KeyBlock  */
  KeyObject = (PKEY_OBJECT) KeyHandleRep->ObjectBody;
  KeyBlock = KeyObject->KeyBlock;
    
  CmiLockBlock(KeyBlock);
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
          KeyBasicInformation = (PKEY_BASIC_INFORMATION) KeyInformation;
          KeyBasicInformation->LastWriteTime = KeyBlock->LastWriteTime;
          KeyBasicInformation->TitleIndex = 0;
          KeyBasicInformation->NameLength = KeyBlock->NameSize;
          wcsncpy(KeyBasicInformation->Name, 
                  KeyBlock->Name, 
                  KeyBlock->NameSize);
          KeyBasicInformation->Name[KeyBlock->NameSize] = 0;
          *ReturnLength = sizeof(KEY_BASIC_INFORMATION) + 
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
          KeyNodeInformation = (PKEY_NODE_INFORMATION) KeyInformation;
          KeyNodeInformation->LastWriteTime = KeyBlock->LastWriteTime;
          KeyNodeInformation->TitleIndex = 0;
          KeyNodeInformation->ClassOffset = sizeof(KEY_NODE_INFORMATION) + 
            KeyBlock->NameSize * sizeof(WCHAR);
          KeyNodeInformation->ClassLength = KeyBlock->ClassSize;
          KeyNodeInformation->NameLength = KeyBlock->NameSize;
          wcsncpy(KeyNodeInformation->Name, 
                  KeyBlock->Name, 
                  KeyBlock->NameSize);
          KeyNodeInformation->Name[KeyBlock->NameSize] = 0;
          if (KeyBlock->ClassSize != 0)
            {
              ClassBlock = CmiGetClassBlock(KeyBlock);
              CmiLockBlock(ClassBlock);
              wcsncpy(KeyNodeInformation->Name + KeyBlock->NameSize + 1,
                      ClassBlock,
                      KeyBlock->ClassSize);
              CmiUnlockBlock(ClassBlock);
              KeyNodeInformation->
                Name[KeyBlock->NameSize + 1 + KeyBlock->ClassSize] = 0;
            }
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
          /* FIXME: fill buffer with requested info  */
          KeyFullInformation = (PKEY_FULL_INFORMATION) KeyInformation;
          KeyFullInformation->LastWriteTime = KeyBlock->LastWriteTime;
          KeyFullInformation->TitleIndex = 0;
          KeyFullInformation->ClassOffset = sizeof(KEY_FULL_INFORMATION) - 
            sizeof(WCHAR);
          KeyFullInformation->ClassLength = KeyBlock->ClassSize;
          KeyFullInformation->SubKeys = KeyBlock->NumberOfSubKeys;
          KeyFullInformation->MaxNameLen = CmiGetMaxNameLength(KeyBlock);
          KeyFullInformation->MaxClassLen = CmiGetMaxClassLength(KeyBlock);
          KeyFullInformation->Values = ?;
          KeyFullInformation->MaxValueNameLen = 
            CmiGetMaxValueNameLength(KeyBlock);
          KeyFullInformation->MaxValueDataLen = 
            CmiGetMaxValueDataLength(KeyBlock);
          ClassBlock = CmiGetClassBlock(KeyBlock);
          CmiLockBlock(ClassBlock);
          wcsncpy(KeyFullInformation->Class,
                  ClassBlock,
                  KeyBlock->ClassSize);
          CmiUnlockBlock(ClassBlock);
          KeyFullInformation->Class[KeyBlock->ClassSize] = 0;
        }
      break;
    }
  CmiUnlockBlock(KeyBlock);

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
  PHANDLE_REP  KeyHandleRep;
  POBJECT_HEADER  KeyObjectHeader;
  PKEY_OBJECT  KeyObject;
  PKEY_BLOCK  KeyBlock;
  PKEY_VALUE_BASIC_INFORMATION  KeyValueBasicInformation;
  PKEY_VALUE_PARTIAL_INFORMATION  KeyValuePartialInformation;
  PKEY_VALUE_FULL_INFORMATION  KeyValueFullInformation;

  /*  Verify that the handle is valid and is a registry key  */
  KeyHandleRep = ObTranslateHandle(PsGetCurrentProcess(), KeyHandle);
  if (KeyHandleRep == NULL)
    {
      return  STATUS_INVALID_HANDLE;
    }
  KeyObjectHeader = BODY_TO_HEADER(KeyHandleRep->ObjectBody);
  if (KeyObjectHeader->ObjectType != CmiKeyObject)
    {
      return  STATUS_INVALID_HANDLE;
    }
  
  /*  Verify required access for enumerate  */
  if ((KeyHandleRep->AccessGranted & KEY_QUERY_KEY) != KEY_QUERY_KEY)
    {
      return  STATUS_ACCESS_DENIED;
    }

  /*  Get pointer to KeyBlock  */
  KeyObject = (PKEY_OBJECT) KeyHandleRep->ObjectBody;
  KeyBlock = KeyObject->KeyBlock;
    
  Status = STATUS_SUCCESS;
  CmiLockBlock(KeyBlock);
  
  /* FIXME: get value list block  */
  /* FIXME: loop through value list  */
    /* FIXME: get value block for current list entry  */
    /* FIXME: compare name with desired value name */
      /* FIXME: if name is correct */
      /* FIXME: check size of input buffer  */
      /* FIXME: copy data into buffer  */
      /* FIXME: set ResultLength  */
  
  CmiUnlockBlock(KeyBlock);
  
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
CmiObjectParse(PVOID ParsedObject, PWSTR* Path)
{
  PWSTR  S, SubKeyBuffer;
  PKEY_OBJECT  CurrentKey, ChildKey;

  UNIMPLEMENTED

  /*  If the path is an empty string, we're done  */
  if (Path == NULL || Path[0] == 0)
    {
/* FIXME: return the root key */
      return NULL;
    }

  /*  Extract subkey name from path  */
  S = *Path;
  while (*S != '\\')
    {
      S++;
    }
  SubKeyBuffer = ExAllocatePool(NonPagedPool, (S - *Path) * sizeof(WSTR));
  wstrncpy(SubKeyBuffer, *Path, (S - *Path));
  SubKeyBuffer[S - *Path] = 0;
  
  /* %%% Scan Key for matching SubKey  */
  CurrentKey = (PKEY_OBJECT) ParsedObject;
  ChildKey = CurrentKey->
          /*  Move Key Object pointer to first child  */
          ParentKey = ParentKey->SubKeys;
          
          /*  Extract the next path component from requested path  */
          wstrncpy(CurLevel, S, T-S);
          CurLevel[T-S] = 0;
          DPRINT("CurLevel:[%w]", CurLevel);
          
          /*  Walk through children looking for path component  */
          while (ParentKey != NULL)
            {
              if (wstrcmp(CurLevel, ParentKey->Name) == 0)
                {
                  break;
                }
              ParentKey = ParentKey->NextKey;
            }


  /* %%% If SubKey is not found return NULL  */
  /* %%% Adjust path to next level  */
  /* %%% Return object for SubKey  */

  ExFreePool(SubKeyBuffer);

}

static PVOID  CmiObjectDelete(PVOID  DeletedObject)
{
  /* FIXME: if marked for delete, then call CmiDeleteKey  */
  UNIMPLEMENTED;
}


static NTSTATUS
CmiBuildKeyPath(PWSTR  *KeyPath, POBJECT_ATTRIBUTES  ObjectAttributes)
{
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
CmiScanKeyList(PWSTR  *KeyNameBuf)
{
  KIRQL  OldIrql;
  PKEY_OBJECT  CurKey;

  KeAcquireSpinLock(CmiKeyListLock, &OldIrql);
  CurKey = CmiKeyList;
  while (CurKey != NULL && wcscmp(KeyPath, CurKey->Name) != 0)
    {
      CurKey = CurKey->Next;
    }
  KeReleaseSpinLock(CmiKeyListLock, OldIrql);
  
  return CurKey;
}

static NTSTATUS
CmiCreateKey(PREGISTRY_FILE  RegistryFile,
             PWSTR  KeyNameBuf,
             PKEY_BLOCK  KeyBlock,
             ACCESS_MASK DesiredAccess,
             ULONG TitleIndex,
             PUNICODE_STRING Class, 
             ULONG CreateOptions, 
             PULONG Disposition)
{
}
#endif


