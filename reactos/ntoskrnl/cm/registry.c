/* $Id: registry.c,v 1.38 2000/09/29 15:03:20 jean Exp $
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
#include <windows.h>
#include <defines.h>
#include <ddk/ntddk.h>
#include <internal/ob.h>
#include <wchar.h>

#define NDEBUG
#include <internal/debug.h>


/*  -----------------------------------------------------  Typedefs  */

//#define LONG_MAX 0x7fffffff

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
#define  REG_MACHINE_KEY_NAME  L"\\Registry\\Machine"
#define  REG_SYSTEM_KEY_NAME  L"\\Registry\\Machine\\System"
#define  REG_SOFTWARE_KEY_NAME  L"\\Registry\\Machine\\Software"
#define  REG_USERS_KEY_NAME  L"\\Registry\\User"
#define  REG_USER_KEY_NAME  L"\\Registry\\User\\CurrentUser"

#define  SYSTEM_REG_FILE  L"\\SystemRoot\\System32\\Config\\SYSTEM"
#define  SOFTWARE_REG_FILE  L"\\SystemRoot\\System32\\Config\\SOFTWARE"
#define  USER_REG_FILE  L"\\SystemRoot\\System32\\Config\\USER"

#define  KO_MARKED_FOR_DELETE  0x00000001

// BLOCK_OFFSET = offset in file after header block
typedef DWORD  BLOCK_OFFSET;

/* header for registry hive file : */
typedef struct _HEADER_BLOCK
{
  DWORD  BlockId;		/* ="regf" */
  DWORD  Unused1;		/* file version ?*/
  DWORD  Unused2;		/* file version ?*/
  LARGE_INTEGER  DateModified;
  DWORD  Unused3;		/* registry format version ? */
  DWORD  Unused4;		/* registry format version ? */
  DWORD  Unused5;		/* registry format version ? */
  DWORD  Unused6;		/* registry format version ? */
  BLOCK_OFFSET  RootKeyBlock;
  DWORD  BlockSize;
  DWORD  Unused7;
  WCHAR  FileName[64];		/* end of file name */
  DWORD  Unused8[83];
  DWORD  Checksum;
} HEADER_BLOCK, *PHEADER_BLOCK;

typedef struct _HEAP_BLOCK
{
  DWORD  BlockId;		/* = "hbin" */
  BLOCK_OFFSET  BlockOffset;	/* block offset of this heap */
  DWORD  BlockSize;		/* size in bytes, 4k multiple */
  DWORD  Unused1;
  LARGE_INTEGER  DateModified;
  DWORD  Unused2;
} HEAP_BLOCK, *PHEAP_BLOCK;

// each sub_block begin with this struct :
// in a free subblock, higher bit of SubBlockSize is set
typedef struct _FREE_SUB_BLOCK
{
  DWORD  SubBlockSize;/* <0 if used, >0 if free */
} FREE_SUB_BLOCK, *PFREE_SUB_BLOCK;

typedef struct _KEY_BLOCK
{
  DWORD  SubBlockSize;
  WORD  SubBlockId;
  WORD  Type;
  LARGE_INTEGER  LastWriteTime;
  DWORD UnUsed1;
  BLOCK_OFFSET  ParentKeyOffset;
  DWORD  NumberOfSubKeys;
  DWORD UnUsed2;
  BLOCK_OFFSET  HashTableOffset;
  DWORD UnUsed3;
  DWORD  NumberOfValues;
  BLOCK_OFFSET  ValuesOffset;
  BLOCK_OFFSET  SecurityKeyOffset;
  BLOCK_OFFSET  ClassNameOffset;
  DWORD  Unused4[5];
  WORD  NameSize;
  WORD  ClassSize; /* size of ClassName in bytes */
  UCHAR  Name[0]; /* warning : not zero terminated */
} KEY_BLOCK, *PKEY_BLOCK;

// hash record :
// HashValue=four letters of value's name
typedef struct _HASH_RECORD
{
  BLOCK_OFFSET  KeyOffset;
  ULONG  HashValue;
} HASH_RECORD, *PHASH_RECORD;

typedef struct _HASH_TABLE_BLOCK
{
  DWORD  SubBlockSize;
  WORD  SubBlockId;
  WORD  HashTableSize;
  HASH_RECORD  Table[0];
} HASH_TABLE_BLOCK, *PHASH_TABLE_BLOCK;

typedef struct _VALUE_LIST_BLOCK
{
  DWORD  SubBlockSize;
  BLOCK_OFFSET  Values[0];
} VALUE_LIST_BLOCK, *PVALUE_LIST_BLOCK;

typedef struct _VALUE_BLOCK
{
  DWORD  SubBlockSize;
  WORD  SubBlockId;	// "kv"
  WORD  NameSize;	// length of Name
  LONG  DataSize;	// length of datas in the subblock pinted by DataOffset
  BLOCK_OFFSET  DataOffset;	// datas are here if DataSize <=4
  DWORD  DataType;
  WORD  Flags;
  WORD  Unused1;
  UCHAR  Name[0]; /* warning : not zero terminated */
} VALUE_BLOCK, *PVALUE_BLOCK;

typedef struct _DATA_BLOCK
{
  DWORD  SubBlockSize;
  UCHAR  Data[0];
} DATA_BLOCK, *PDATA_BLOCK;

typedef struct _REGISTRY_FILE
{
  PWSTR  Filename;
//  HANDLE  FileHandle;
  PFILE_OBJECT FileObject;
  PHEADER_BLOCK  HeaderBlock;
  ULONG  NumberOfBlocks;
  ULONG  BlockListSize;
  PHEAP_BLOCK  *BlockList;

  NTSTATUS  (*Extend)(ULONG NewSize);
  PVOID  (*Flush)(VOID);
} REGISTRY_FILE, *PREGISTRY_FILE;

/*  Type defining the Object Manager Key Object  */
typedef struct _KEY_OBJECT
{
  CSHORT  Type;
  CSHORT  Size;
  
  ULONG  Flags;
  WORD  NameSize;	// length of Name
  UCHAR  *Name;
  PREGISTRY_FILE  RegistryFile;
  BLOCK_OFFSET BlockOffset;
  PKEY_BLOCK  KeyBlock;
  struct _KEY_OBJECT  *ParentKey;
  DWORD  NumberOfSubKeys;		/* subkeys loaded in SubKeys */
  DWORD  SizeOfSubKeys;			/* space allocated in SubKeys */
  struct _KEY_OBJECT  **SubKeys;		/* list of subkeys loaded */
} KEY_OBJECT, *PKEY_OBJECT;


/*  -------------------------------------------------  File Statics  */

POBJECT_TYPE  CmiKeyType = NULL;
static PREGISTRY_FILE  CmiVolatileFile = NULL;
static PKEY_OBJECT  CmiRootKey = NULL;
static PKEY_OBJECT  CmiMachineKey = NULL;
static PKEY_OBJECT  CmiUserKey = NULL;
static KSPIN_LOCK  CmiKeyListLock;
static PREGISTRY_FILE  CmiSystemFile = NULL;

/*  -----------------------------------------  Forward Declarations  */


static NTSTATUS CmiObjectParse(PVOID ParsedObject,
		     PVOID *NextObject,
		     PUNICODE_STRING FullPath,
		     PWSTR *Path,
		     POBJECT_TYPE ObjectType);
static NTSTATUS CmiObjectCreate(PVOID ObjectBody,
		      PVOID Parent,
		      PWSTR RemainingPath,
		      struct _OBJECT_ATTRIBUTES* ObjectAttributes);

static VOID  CmiObjectDelete(PVOID  DeletedObject);
static VOID  CmiAddKeyToList(PKEY_OBJECT ParentKey,PKEY_OBJECT  NewKey);
static VOID  CmiRemoveKeyFromList(PKEY_OBJECT  NewKey);
static PKEY_OBJECT  CmiScanKeyList(PKEY_OBJECT Parent,PCHAR  KeyNameBuf);
static PREGISTRY_FILE  CmiCreateRegistry(PWSTR  Filename);
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
				  OUT BLOCK_OFFSET *BlockOffset,
                                  IN PCHAR  KeyName,
                                  IN ACCESS_MASK  DesiredAccess);
static NTSTATUS  CmiAddSubKey(IN PREGISTRY_FILE  RegistryFile, 
                              IN PKEY_BLOCK  CurKeyBlock,
                              OUT PKEY_BLOCK  *SubKeyBlock,
                              IN PCHAR  NewSubKeyName,
                              IN ULONG  TitleIndex,
                              IN PWSTR  Class, 
                              IN ULONG  CreateOptions);
static NTSTATUS  CmiScanKeyForValue(IN PREGISTRY_FILE  RegistryFile,
                                    IN PKEY_BLOCK  KeyBlock,
                                    IN PCHAR  ValueName,
                                    OUT PVALUE_BLOCK  *ValueBlock);
static NTSTATUS  CmiGetValueFromKeyByIndex(IN PREGISTRY_FILE  RegistryFile,
                                           IN PKEY_BLOCK  KeyBlock,
                                           IN ULONG  Index,
                                           OUT PVALUE_BLOCK  *ValueBlock);
static NTSTATUS  CmiAddValueToKey(IN PREGISTRY_FILE  RegistryFile,
                                  IN PKEY_BLOCK  KeyBlock,
                                  IN PCHAR  ValueNameBuf,
                                  IN ULONG  Type, 
                                  IN PVOID  Data,
                                  IN ULONG  DataSize);
static NTSTATUS  CmiDeleteValueFromKey(IN PREGISTRY_FILE  RegistryFile,
                                       IN PKEY_BLOCK  KeyBlock,
                                       IN PCHAR  ValueName);
static NTSTATUS  CmiAllocateKeyBlock(IN PREGISTRY_FILE  RegistryFile,
                                     OUT PKEY_BLOCK  *KeyBlock,
                                     IN PCHAR  NewSubKeyName,
                                     IN ULONG  TitleIndex,
                                     IN PWSTR  Class,
                                    IN ULONG  CreateOptions);
static NTSTATUS  CmiDestroyKeyBlock(PREGISTRY_FILE  RegistryFile,
                                    PKEY_BLOCK  KeyBlock);
static NTSTATUS  CmiAllocateHashTableBlock(IN PREGISTRY_FILE  RegistryFile,
                                           OUT PHASH_TABLE_BLOCK  *HashBlock,
                                           IN ULONG  HashTableSize);
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
                                       IN PCHAR  ValueNameBuf,
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
                                  ULONG  BlockSize,
				  BLOCK_OFFSET *BlockOffset);
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


/*  ---------------------------------------------  Public Interface  */

VOID
CmInitializeRegistry(VOID)
{
  NTSTATUS  Status;
  HANDLE  RootKeyHandle;
  UNICODE_STRING  RootKeyName;
  OBJECT_ATTRIBUTES  ObjectAttributes;
 PKEY_OBJECT  NewKey;
 HANDLE  KeyHandle;
  
  /*  Initialize the Key object type  */
  CmiKeyType = ExAllocatePool(NonPagedPool, sizeof(OBJECT_TYPE));
  CmiKeyType->TotalObjects = 0;
  CmiKeyType->TotalHandles = 0;
  CmiKeyType->MaxObjects = LONG_MAX;
  CmiKeyType->MaxHandles = LONG_MAX;
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
  CmiKeyType->Create = CmiObjectCreate;
  RtlInitUnicodeString(&CmiKeyType->TypeName, L"Key");

  /*  Build volitile registry store  */
  CmiVolatileFile = CmiCreateRegistry(NULL);

  /*  Build the Root Key Object  */
  RtlInitUnicodeString(&RootKeyName, REG_ROOT_KEY_NAME);
  InitializeObjectAttributes(&ObjectAttributes, &RootKeyName, 0, NULL, NULL);
  NewKey=ObCreateObject(&RootKeyHandle,
                                STANDARD_RIGHTS_REQUIRED,
                 &ObjectAttributes,
                 CmiKeyType);
  CmiRootKey = NewKey;
  ObAddEntryDirectory(NameSpaceRoot, CmiRootKey, L"Registry");
  Status = ObReferenceObjectByHandle(RootKeyHandle,
                 STANDARD_RIGHTS_REQUIRED,
		 ObDirectoryType,
		 UserMode,
                 (PVOID*)&CmiRootKey,
                 NULL);
    CmiRootKey->RegistryFile = CmiVolatileFile;
    CmiRootKey->KeyBlock = CmiGetBlock(CmiVolatileFile,CmiVolatileFile->HeaderBlock->RootKeyBlock);
    CmiRootKey->BlockOffset = CmiVolatileFile->HeaderBlock->RootKeyBlock;
    CmiRootKey->Flags = 0;
    CmiRootKey->NumberOfSubKeys=0;
    CmiRootKey->SubKeys= NULL;
    CmiRootKey->SizeOfSubKeys= 0;
    CmiRootKey->Name=ExAllocatePool(PagedPool,strlen("Registry"));
    CmiRootKey->NameSize=strlen("Registry");
    memcpy(CmiRootKey->Name,"Registry",strlen("Registry"));

  KeInitializeSpinLock(&CmiKeyListLock);

  /*  Create initial predefined symbolic links  */
  /* HKEY_LOCAL_MACHINE  */
  RtlInitUnicodeString(&RootKeyName, REG_MACHINE_KEY_NAME);
  InitializeObjectAttributes(&ObjectAttributes, &RootKeyName, 0, NULL, NULL);
  NewKey=ObCreateObject(&KeyHandle,
                                STANDARD_RIGHTS_REQUIRED,
                 &ObjectAttributes,
                 CmiKeyType);
  Status = CmiAddSubKey(CmiVolatileFile,
                        CmiRootKey->KeyBlock,
                        &NewKey->KeyBlock,
                        "Machine",
                        0,
                        NULL,
                        0);
    NewKey->RegistryFile = CmiVolatileFile;
    NewKey->Flags = 0;
    NewKey->NumberOfSubKeys=0;
    NewKey->SubKeys= NULL;
    NewKey->SizeOfSubKeys= NewKey->KeyBlock->NumberOfSubKeys;
    NewKey->Name=ExAllocatePool(PagedPool,strlen("Machine"));
    NewKey->NameSize=strlen("Machine");
    memcpy(NewKey->Name,"Machine",strlen("Machine"));
  CmiAddKeyToList(CmiRootKey,NewKey);
    CmiMachineKey=NewKey;

  /* HKEY_USERS  */
  RtlInitUnicodeString(&RootKeyName, REG_USERS_KEY_NAME);
  InitializeObjectAttributes(&ObjectAttributes, &RootKeyName, 0, NULL, NULL);
  NewKey=ObCreateObject(&KeyHandle,
                                STANDARD_RIGHTS_REQUIRED,
                 &ObjectAttributes,
                 CmiKeyType);
  Status = CmiAddSubKey(CmiVolatileFile,
                        CmiRootKey->KeyBlock,
                        &NewKey->KeyBlock,
                        "User",
                        0,
                        NULL,
                        0);
    NewKey->RegistryFile = CmiVolatileFile;
    NewKey->Flags = 0;
    NewKey->NumberOfSubKeys=0;
    NewKey->SubKeys= NULL;
    NewKey->SizeOfSubKeys= NewKey->KeyBlock->NumberOfSubKeys;
    NewKey->Name=ExAllocatePool(PagedPool,strlen("User"));
    NewKey->NameSize=strlen("User");
    memcpy(NewKey->Name,"Machine",strlen("User"));
  CmiAddKeyToList(CmiRootKey,NewKey);
    CmiUserKey=NewKey;


  /* FIXME: create remaining structure needed for default handles  */
  /* FIXME: load volatile registry data from ROSDTECT  */

}

VOID
CmInitializeRegistry2(VOID)
{
 OBJECT_ATTRIBUTES  ObjectAttributes;
 PKEY_OBJECT  NewKey;
 HANDLE  KeyHandle;
 UNICODE_STRING  KeyName;
  /* FIXME : delete temporary \Registry\Machine\System */
  /* load the SYSTEM Hive */
  CmiSystemFile = CmiCreateRegistry(SYSTEM_REG_FILE);
  if( CmiSystemFile )
  {
    RtlInitUnicodeString(&KeyName, REG_SYSTEM_KEY_NAME);
    InitializeObjectAttributes(&ObjectAttributes, &KeyName, 0, NULL, NULL);
DPRINT("\\Registry=%x\n",CmiRootKey);
DPRINT("Machine=%x\n",CmiMachineKey);
    NewKey=ObCreateObject(&KeyHandle,
                 STANDARD_RIGHTS_REQUIRED,
                 &ObjectAttributes,
                 CmiKeyType);
DPRINT("System=%x\n",NewKey);
    NewKey->RegistryFile = CmiSystemFile;
    NewKey->KeyBlock = CmiGetBlock(CmiSystemFile,32);
    NewKey->BlockOffset = 32;
    NewKey->Flags = 0;
    NewKey->NumberOfSubKeys=0;
    NewKey->SubKeys= ExAllocatePool(PagedPool
		, NewKey->KeyBlock->NumberOfSubKeys * sizeof(DWORD));
    NewKey->SizeOfSubKeys= NewKey->KeyBlock->NumberOfSubKeys;
    NewKey->Name=ExAllocatePool(PagedPool,strlen("System"));
    NewKey->NameSize=strlen("System");
    memcpy(NewKey->Name,"System",strlen("System"));
  CmiAddKeyToList(CmiMachineKey,NewKey);
  }
  else
    /* FIXME : search SYSTEM.alt, or create new */
    DbgPrint(" warning : registry file \\reactos\\system32\\config\\SYSTEM not found\n");
  /* FIXME : initialize standards symbolic links */
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
 NTSTATUS	Status;
 PVOID		Object;
 PKEY_OBJECT key;
 PKEY_BLOCK KeyBlock;

  DPRINT("NtCreateKey (Name %wZ),KeyHandle=%x,Root=%x\n",
         ObjectAttributes->ObjectName,KeyHandle
	,ObjectAttributes->RootDirectory);

  Status = ObReferenceObjectByName(
  		ObjectAttributes->ObjectName,
  		ObjectAttributes->Attributes,
  		NULL,
  		DesiredAccess,
  		CmiKeyType,
  		UserMode,
  		NULL,
  		& Object
  		);
  if (NT_SUCCESS(Status))
  {
    if (Disposition) *Disposition = REG_OPENED_EXISTING_KEY;
    Status = ObCreateHandle(
  		PsGetCurrentProcess(),
  		Object,
  		DesiredAccess,
  		FALSE,
  		KeyHandle
  		);
  }
  else
  {
   BLOCK_OFFSET NewKeyOffset;
CHECKPOINT;
    key = ObCreateObject(
  		KeyHandle,
  		DesiredAccess,
  		ObjectAttributes,
  		CmiKeyType
  		);
/* FIXME : if key->Name contains \\ : must create tree */
    if (key == NULL)
  	return Status;
    if (CreateOptions & REG_OPTION_VOLATILE)
      key->RegistryFile=CmiVolatileFile;
    else
      key->RegistryFile=key->ParentKey->RegistryFile;
    key->Flags = 0;
    key->NumberOfSubKeys = 0;
    key->SizeOfSubKeys = 0;
    key->SubKeys = NULL;
CHECKPOINT;
    Status = CmiAllocateBlock(key->RegistryFile
			,(PVOID)&KeyBlock
			,sizeof(KEY_BLOCK) + key->NameSize
			,&NewKeyOffset );
    key->BlockOffset = NewKeyOffset;
    key->KeyBlock = KeyBlock;
    KeyBlock->SubBlockId = REG_KEY_BLOCK_ID;
    KeyBlock->Type = REG_KEY_BLOCK_TYPE;
    ZwQuerySystemTime((PTIME) &KeyBlock->LastWriteTime);
    if (key->RegistryFile == key->ParentKey->RegistryFile)
      KeyBlock->ParentKeyOffset = key->ParentKey->BlockOffset;
    else
      KeyBlock->ParentKeyOffset = -1;
    KeyBlock->HashTableOffset = -1;
    KeyBlock->ValuesOffset = -1;
    if (key->RegistryFile == key->ParentKey->RegistryFile)
      KeyBlock->SecurityKeyOffset = key->ParentKey->KeyBlock->SecurityKeyOffset;
    KeyBlock->ClassNameOffset = -1;
CHECKPOINT;
    wcstombs(KeyBlock->Name, (PWSTR)key->Name, key->NameSize);
CHECKPOINT;
    key->Name = KeyBlock->Name;
    if (Class)
    {
     PDATA_BLOCK pClass;
CHECKPOINT;
      KeyBlock->ClassSize = Class->Length+sizeof(WCHAR);
      Status = CmiAllocateBlock(key->RegistryFile
			,(PVOID)&pClass
			,KeyBlock->ClassSize
			,&KeyBlock->ClassNameOffset );
CHECKPOINT;
      wcsncpy((PWSTR)pClass->Data,Class->Buffer,Class->Length);
CHECKPOINT;
    }
DPRINT("key->ParentKey=%x,nbofSK=%d\n",key->ParentKey,key->ParentKey->NumberOfSubKeys);
DPRINT("sk0=%x\n",key->ParentKey->SubKeys[0]);
    CmiAddKeyToList(key->ParentKey,key);
DPRINT("key->ParentKey=%x,nbofSK=%d\n",key->ParentKey,key->ParentKey->NumberOfSubKeys);
DPRINT("sk0=%x\n",key->ParentKey->SubKeys[0]);
DPRINT("sk1=%x\n",key->ParentKey->SubKeys[1]);
    /* FIXME : add key to subkeys of parent if needed */
    if (Disposition) *Disposition = REG_CREATED_NEW_KEY;
    Status = ObCreateHandle(
  		PsGetCurrentProcess(),
  		Object,
  		DesiredAccess,
  		FALSE,
  		KeyHandle
  		);
  }

  if (!NT_SUCCESS(Status))
  {
  	return Status;
  }

  return STATUS_SUCCESS;
}


NTSTATUS 
STDCALL
NtDeleteKey (
	IN	HANDLE	KeyHandle
	)
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
  /* FIXME ? : this method don't get volatile keys */
  HashTableBlock = CmiGetBlock(RegistryFile, KeyBlock->HashTableOffset);
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
          (SubKeyBlock->NameSize ) * sizeof(WCHAR))
        {
          Status = STATUS_BUFFER_OVERFLOW;
        }
      else
        {
          /*  Fill buffer with requested info  */
          BasicInformation = (PKEY_BASIC_INFORMATION) KeyInformation;
          BasicInformation->LastWriteTime = SubKeyBlock->LastWriteTime;
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
          NodeInformation->LastWriteTime = SubKeyBlock->LastWriteTime;
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
                                     ,SubKeyBlock->ClassNameOffset);
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
          if (SubKeyBlock->ClassSize != 0)
            {
              pClassData=CmiGetBlock(KeyObject->RegistryFile
                                     ,SubKeyBlock->ClassNameOffset);
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
                DataBlock = CmiGetBlock(RegistryFile, ValueBlock->DataOffset);
                RtlCopyMemory(ValuePartialInformation->Data, 
                            DataBlock->Data,
                            ValueBlock->DataSize);
                CmiReleaseBlock(RegistryFile, DataBlock);
              }
              else
              {
                RtlCopyMemory(ValuePartialInformation->Data, 
                            &ValueBlock->DataOffset, 
                            ValueBlock->DataSize & LONG_MAX);
              }
              DataBlock = CmiGetBlock(RegistryFile, ValueBlock->DataOffset);
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
                sizeof(KEY_VALUE_FULL_INFORMATION) + 
                (ValueBlock->NameSize -1) * sizeof(WCHAR);
              ValueFullInformation->DataLength = ValueBlock->DataSize & LONG_MAX;
              ValueFullInformation->NameLength =
                (ValueBlock->NameSize ) * sizeof(WCHAR);
              mbstowcs(ValueFullInformation->Name, ValueBlock->Name
			,ValueBlock->NameSize*2);
              ValueFullInformation->Name[ValueBlock->NameSize]=0;
              if(ValueBlock->DataSize >0)
              {
                DataBlock = CmiGetBlock(RegistryFile, ValueBlock->DataOffset);
                RtlCopyMemory(&ValueFullInformation->Name[ValueBlock->NameSize + 1],
                            DataBlock->Data,
                            ValueBlock->DataSize);
                CmiReleaseBlock(RegistryFile, DataBlock);
              }
              else
              {
                RtlCopyMemory(&ValueFullInformation->Name[ValueBlock->NameSize + 1],
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
NtFlushKey (
	IN	HANDLE	KeyHandle
	)
{
  return  STATUS_SUCCESS;
}


NTSTATUS 
STDCALL
NtOpenKey (
	OUT	PHANDLE			KeyHandle, 
	IN	ACCESS_MASK		DesiredAccess,
	IN	POBJECT_ATTRIBUTES	ObjectAttributes
	)
{
 NTSTATUS	Status;
 PVOID		Object;
 UNICODE_STRING RemainingPath;

   DPRINT("NtOpenKey (Name %wZ),Root=%x\n",
	       ObjectAttributes->ObjectName
	,ObjectAttributes->RootDirectory);
   
   if (ObjectAttributes->RootDirectory == HKEY_LOCAL_MACHINE)
   {
CHECKPOINT;
     Status = ObCreateHandle(
  		PsGetCurrentProcess(),
  		CmiMachineKey,
  		KEY_ALL_ACCESS,
  		FALSE,
  		&ObjectAttributes->RootDirectory
  		);
   }
CHECKPOINT;
   Status = ObFindObject(ObjectAttributes,&Object,&RemainingPath,CmiKeyType);
/*
 Status = ObReferenceObjectByName(
			ObjectAttributes->ObjectName,
			ObjectAttributes->Attributes,
			NULL,
			DesiredAccess,
			CmiKeyType,
			UserMode,
			NULL,
			& Object
			);
	if (!NT_SUCCESS(Status))
	{
		return Status;
	}
*/
DPRINT("Status=%x,RP=%S\n",Status,RemainingPath.Buffer);
/*
   if ( RemainingPath.Buffer != NULL )
   {
     return STATUS_UNSUCCESSFUL;
   }
*/

	Status = ObCreateHandle(
			PsGetCurrentProcess(),
			Object,
			DesiredAccess,
			FALSE,
			KeyHandle
			);
	if (!NT_SUCCESS(Status))
	{
		return Status;
	}

	return STATUS_SUCCESS;
      /*  FIXME : Fail if the key has been deleted  */
//      if (CurKey->Flags & KO_MARKED_FOR_DELETE)
//        {
//          ExFreePool(KeyNameBuf);
//          
//          return STATUS_UNSUCCESSFUL;
//        }
      
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
          BasicInformation->LastWriteTime = KeyBlock->LastWriteTime;
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
          NodeInformation->LastWriteTime = KeyBlock->LastWriteTime;
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
                                     ,KeyBlock->ClassNameOffset);
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
CHECKPOINT;
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
          if (KeyBlock->ClassSize != 0)
            {
              pClassData=CmiGetBlock(KeyObject->RegistryFile
                                     ,KeyBlock->ClassNameOffset);
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
DPRINT1("apres OROBH\n");
      return  Status;
    }

  /*  Get pointer to KeyBlock  */
  KeyBlock = KeyObject->KeyBlock;
  RegistryFile = KeyObject->RegistryFile;
DPRINT("offset du pere = %x\n",KeyObject->BlockOffset);
  /*  Get Value block of interest  */
  Status = CmiScanKeyForValue(RegistryFile, 
                              KeyBlock,
                              ValueName2,
                              &ValueBlock);
  if (!NT_SUCCESS(Status))
    {
DPRINT("apres CSKFV\n");
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
CHECKPOINT;
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
                DataBlock = CmiGetBlock(RegistryFile, ValueBlock->DataOffset);
                RtlCopyMemory(ValuePartialInformation->Data, 
                            DataBlock->Data,
                            ValueBlock->DataSize);
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
                 + ValueBlock->NameSize * sizeof(WCHAR)
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
                sizeof(KEY_VALUE_FULL_INFORMATION) + 
                (ValueBlock->NameSize-1) * sizeof(WCHAR);
              ValueFullInformation->DataLength = ValueBlock->DataSize & LONG_MAX;
              ValueFullInformation->NameLength =
                (ValueBlock->NameSize ) * sizeof(WCHAR);
              mbstowcs(ValueFullInformation->Name, ValueBlock->Name,ValueBlock->NameSize*2);
//              ValueFullInformation->Name[ValueBlock->NameSize]=0;
//DPRINT("ValueBlock=%x,DataSize=%x,DataOffset=%x\n"
//,ValueBlock,ValueBlock->DataSize,&ValueBlock->DataOffset);
              if(ValueBlock->DataSize >0)
              {
                DataBlock = CmiGetBlock(RegistryFile, ValueBlock->DataOffset);
                RtlCopyMemory(&ValueFullInformation->Name[ValueBlock->NameSize +1 ], 
                            DataBlock->Data, 
                            ValueBlock->DataSize);
                CmiReleaseBlock(RegistryFile, DataBlock);
              }
              else
              {
                RtlCopyMemory(&ValueFullInformation->Name[ValueBlock->NameSize +1 ], 
                            &ValueBlock->DataOffset, 
                            ValueBlock->DataSize & LONG_MAX);
              }
            }
          break;
        }
    }
  else
    {
DPRINT("ValueBlock==NULL \n");
      Status = STATUS_UNSUCCESSFUL;
    }
  ObDereferenceObject(KeyObject);
DPRINT("fin normale \n");
  
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
  char ValueName2[MAX_PATH];

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
    {
      return  Status;
    }

  /*  Get pointer to KeyBlock  */
  KeyBlock = KeyObject->KeyBlock;
  RegistryFile = KeyObject->RegistryFile;
  Status = CmiScanKeyForValue(RegistryFile,
                              KeyBlock,
                              ValueName2,
                              &ValueBlock);
  if (!NT_SUCCESS(Status))
    {
      ObDereferenceObject (KeyObject);
      return  Status;
    }
  if (ValueBlock == NULL)
    {
      Status =  CmiAddValueToKey(RegistryFile,
                                 KeyBlock,
                                 ValueName2,
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
  Status = CmiDeleteValueFromKey(RegistryFile,
                                 KeyBlock,
                                 ValueName2);
  ObDereferenceObject(KeyObject);

  return  Status;
}

NTSTATUS
STDCALL 
NtLoadKey (
	PHANDLE			KeyHandle,
	OBJECT_ATTRIBUTES	ObjectAttributes
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
	OBJECT_ATTRIBUTES	ObjectAttributes,
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
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
RtlCheckRegistryKey (
	IN	ULONG	RelativeTo,
	IN	PWSTR	Path
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
	IN	PRTL_QUERY_REGISTRY_TABLE	QueryTable,
	IN	PVOID				Context,
	IN	PVOID				Environment
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
RtlWriteRegistryValue (
	IN	ULONG	RelativeTo,
	IN	PWSTR	Path,
	IN	PWSTR	ValueName,
	IN	ULONG	ValueType,
	IN	PVOID	ValueData,
	IN	ULONG	ValueLength
	)
{
	UNIMPLEMENTED;
}

/*  ------------------------------------------  Private Implementation  */


static NTSTATUS CmiObjectParse(PVOID ParsedObject,
		     PVOID *NextObject,
		     PUNICODE_STRING FullPath,
		     PWSTR *Path,
		     POBJECT_TYPE ObjectType)
{
 CHAR cPath[MAX_PATH];
 PWSTR end;
 PKEY_OBJECT FoundObject;
 PKEY_OBJECT ParsedKey=ParsedObject;
 PKEY_BLOCK SubKeyBlock;
 BLOCK_OFFSET BlockOffset;
 NTSTATUS Status;
 HANDLE  KeyHandle;
   *NextObject = NULL;
   if ((*Path) == NULL)
     {
	return STATUS_UNSUCCESSFUL;
     }
   
   if((*Path[0])=='\\')
   {
     end = wcschr((*Path)+1, '\\');
     if (end != NULL)
	*end = 0;
     wcstombs(cPath,(*Path)+1,wcslen((*Path)+1));
     cPath[wcslen( (*Path)+1)]=0;
   }
   else
   {
     end = wcschr((*Path), '\\');
     if (end != NULL)
	*end = 0;
     wcstombs(cPath,(*Path),wcslen((*Path)));
     cPath[wcslen( (*Path))]=0;
   }
   FoundObject = CmiScanKeyList(ParsedKey,cPath);
   if (FoundObject == NULL)
   {
      Status = CmiScanForSubKey(ParsedKey->RegistryFile,
                                ParsedKey->KeyBlock,
                                &SubKeyBlock,
                                &BlockOffset,
                                cPath,
                                0);
       if(!NT_SUCCESS(Status) || SubKeyBlock == NULL)
       {
	if (end != NULL)
	  {
	     *end = '\\';
	  }
	return STATUS_UNSUCCESSFUL;
       }
       /*  Create new key object and put into linked list  */
       FoundObject = ObCreateObject(&KeyHandle, 
                                     STANDARD_RIGHTS_REQUIRED, 
                                     NULL, 
                                     CmiKeyType);
       if (FoundObject == NULL)
         {
           //FIXME : return the good error code
           return  STATUS_UNSUCCESSFUL;
         }
       FoundObject->Flags = 0;
       FoundObject->Name = SubKeyBlock->Name;
       FoundObject->NameSize = SubKeyBlock->NameSize;
       FoundObject->KeyBlock = SubKeyBlock;
       FoundObject->BlockOffset = BlockOffset;
       FoundObject->RegistryFile = ParsedKey->RegistryFile;
       CmiAddKeyToList(ParsedKey,FoundObject);
   }
   
   ObReferenceObjectByPointer(FoundObject,
			      STANDARD_RIGHTS_REQUIRED,
			      NULL,
			      UserMode);
   
   if (end != NULL)
     {
	*end = '\\';
	*Path = end;
     }
   else
     {
	*Path = NULL;
     }

   *NextObject = FoundObject;

   return STATUS_SUCCESS;
}

static NTSTATUS CmiObjectCreate(PVOID ObjectBody,
		      PVOID Parent,
		      PWSTR RemainingPath,
		      struct _OBJECT_ATTRIBUTES* ObjectAttributes)
{
 PKEY_OBJECT pKey=ObjectBody;
DPRINT("ob=%x,par=%x,rem=%S\n",ObjectBody,Parent,RemainingPath);
   pKey->ParentKey = Parent;
   if (RemainingPath)
   {
     pKey->Name = (PCHAR) (&RemainingPath[1]);
     pKey->NameSize = wcslen(RemainingPath)-1;
//     DPRINT("rem=%S,len=%d\n",RemainingPath,pKey->NameSize);
   }
   else
     pKey->NameSize = 0;

   return STATUS_SUCCESS;
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

static VOID
CmiAddKeyToList(PKEY_OBJECT ParentKey,PKEY_OBJECT  NewKey)
{
 KIRQL  OldIrql;
  
  KeAcquireSpinLock(&CmiKeyListLock, &OldIrql);
  if (ParentKey->SizeOfSubKeys <= ParentKey->NumberOfSubKeys)
  {
    PKEY_OBJECT *tmpSubKeys = ExAllocatePool(PagedPool
		, (ParentKey->NumberOfSubKeys+1) * sizeof(DWORD));
    if(ParentKey->NumberOfSubKeys > 0)
      memcpy(tmpSubKeys,ParentKey->SubKeys
		,ParentKey->NumberOfSubKeys*sizeof(DWORD));
    if(ParentKey->SubKeys) ExFreePool(ParentKey->SubKeys);
    ParentKey->SubKeys=tmpSubKeys;
    ParentKey->SizeOfSubKeys = ParentKey->NumberOfSubKeys+1;
  }
  /* FIXME : please maintain the list in alphabetic order */
  ParentKey->SubKeys[ParentKey->NumberOfSubKeys++] = NewKey;
  NewKey->ParentKey = ParentKey;
  KeReleaseSpinLock(&CmiKeyListLock, OldIrql);
}

static VOID  
CmiRemoveKeyFromList(PKEY_OBJECT  KeyToRemove)
{
  KIRQL  OldIrql;
  PKEY_OBJECT  ParentKey;
  DWORD Index;

  ParentKey=KeyToRemove->ParentKey;
  KeAcquireSpinLock(&CmiKeyListLock, &OldIrql);
  for (Index=0; Index < ParentKey->NumberOfSubKeys; Index++)
  {
    if(ParentKey->SubKeys[Index] == KeyToRemove)
    {
      memcpy(&ParentKey->SubKeys[Index]
		,&ParentKey->SubKeys[Index+1]
		,(ParentKey->NumberOfSubKeys-Index-1)*sizeof(PKEY_OBJECT));
      ParentKey->NumberOfSubKeys--;
      break;
    }
  }
  KeReleaseSpinLock(&CmiKeyListLock, OldIrql);
}

static PKEY_OBJECT
CmiScanKeyList(PKEY_OBJECT Parent,PCHAR  KeyName)
{
  KIRQL  OldIrql;
  PKEY_OBJECT  CurKey;
  DWORD Index;
  WORD NameSize;
DPRINT("CmiScan : %s\n",KeyName);
  NameSize=strlen(KeyName);
  KeAcquireSpinLock(&CmiKeyListLock, &OldIrql);
  for (Index=0; Index < Parent->NumberOfSubKeys; Index++)
  {
DPRINT("NbOfSK=%d,Index=%d ",Parent->NumberOfSubKeys,Index);
    CurKey=Parent->SubKeys[Index];
DPRINT("Name=%4.4s\n",CurKey->Name);
    if( NameSize == CurKey->NameSize
	&& !memcmp(KeyName,CurKey->Name,NameSize))
    {
//CHECKPOINT;
DPRINT("subkey %s found in object list, blockoffset=%x\n"
  ,KeyName,CurKey->BlockOffset);
       KeReleaseSpinLock(&CmiKeyListLock, OldIrql);
       return CurKey;
    }
//CHECKPOINT;
  }
  KeReleaseSpinLock(&CmiKeyListLock, OldIrql);
  
  return NULL;
}

static PREGISTRY_FILE  
CmiCreateRegistry(PWSTR  Filename)
{
  PREGISTRY_FILE  RegistryFile;
  PKEY_BLOCK  RootKeyBlock;
  HANDLE FileHandle;

  RegistryFile = ExAllocatePool(NonPagedPool, sizeof(REGISTRY_FILE));
  if (Filename != NULL)
   {
     UNICODE_STRING TmpFileName;
     OBJECT_ATTRIBUTES  ObjectAttributes;
     NTSTATUS Status;

      /* Duplicate Filename  */
      RegistryFile->Filename = ExAllocatePool(NonPagedPool, MAX_PATH);
      wcscpy(RegistryFile->Filename , Filename);
      /* FIXME:  if file does not exist, create new file  */
      /* else attempt to map the file  */
      RtlInitUnicodeString (&TmpFileName, Filename);
      InitializeObjectAttributes(&ObjectAttributes,
                             &TmpFileName,
                             0,
                             NULL,
                             NULL);
      Status = ZwOpenFile(&FileHandle,
                      FILE_ALL_ACCESS,
                      &ObjectAttributes,
                      NULL, 0, 0);
      /* FIXME:  if file does not exist, create new file  */
      if( !NT_SUCCESS(Status) )
      {
	DPRINT("registry file not found\n");
	ExFreePool(RegistryFile->Filename);
	RegistryFile->Filename = NULL;
	return NULL;
      }
      RegistryFile->HeaderBlock = (PHEADER_BLOCK) 
        ExAllocatePool(NonPagedPool, sizeof(HEADER_BLOCK));
      Status = ZwReadFile(FileHandle, 
                      0, 0, 0, 0, 
                      RegistryFile->HeaderBlock, 
                      sizeof(HEADER_BLOCK), 
                      0, 0);
      RegistryFile->BlockListSize = 0;
      RegistryFile->BlockList = NULL;
//      RegistryFile->FileHandle = FileHandle;
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
      RootKeyBlock->HashTableOffset = -1;
      RootKeyBlock->NumberOfValues = 0;
      RootKeyBlock->ValuesOffset = -1;
      RootKeyBlock->SecurityKeyOffset = 0;
      RootKeyBlock->ClassNameOffset = -1;
      RootKeyBlock->NameSize = 0;
      RootKeyBlock->ClassSize = 0;
      RegistryFile->HeaderBlock->RootKeyBlock = (BLOCK_OFFSET) RootKeyBlock;
    }

  return  RegistryFile;
}

static ULONG  
CmiGetMaxNameLength(PREGISTRY_FILE  RegistryFile,
                    PKEY_BLOCK  KeyBlock)
{
  ULONG  Idx, MaxName;
  PHASH_TABLE_BLOCK  HashBlock;
  PKEY_BLOCK  CurSubKeyBlock;

  MaxName = 0;
  HashBlock = CmiGetBlock(RegistryFile, KeyBlock->HashTableOffset);
  if (HashBlock == 0)
    {
      return  0;
    }
  for (Idx = 0; Idx < HashBlock->HashTableSize; Idx++)
    {
      if (HashBlock->Table[Idx].KeyOffset != 0)
        {
          CurSubKeyBlock = CmiGetBlock(RegistryFile,
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
  HashBlock = CmiGetBlock(RegistryFile, KeyBlock->HashTableOffset);
  if (HashBlock == 0)
    {
      return  0;
    }
  for (Idx = 0; Idx < HashBlock->HashTableSize; Idx++)
    {
      if (HashBlock->Table[Idx].KeyOffset != 0)
        {
          CurSubKeyBlock = CmiGetBlock(RegistryFile,
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
          MaxValueData < (CurValueBlock->DataSize & LONG_MAX) )
        {
          MaxValueData = CurValueBlock->DataSize & LONG_MAX;
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
                 OUT BLOCK_OFFSET *BlockOffset,
                 IN PCHAR  KeyName,
                 IN ACCESS_MASK  DesiredAccess)
{
  ULONG  Idx;
  PHASH_TABLE_BLOCK  HashBlock;
  PKEY_BLOCK  CurSubKeyBlock;
  WORD KeyLength = strlen(KeyName);

  HashBlock = CmiGetBlock(RegistryFile, KeyBlock->HashTableOffset);
  *SubKeyBlock = NULL;
DPRINT("CmiScanFSK %s,file=%S,HBO=%x,HB=%x,NBSK=%d\n"
	,KeyName,RegistryFile->Filename,KeyBlock->HashTableOffset,HashBlock,KeyBlock->NumberOfSubKeys);
  if (HashBlock == NULL)
    {
      return  STATUS_SUCCESS;
    }
  for (Idx = 0; Idx < HashBlock->HashTableSize; Idx++)
//  for (Idx = 0; Idx < KeyBlock->NumberOfSubKeys; Idx++)
    {
      if (HashBlock->Table[Idx].KeyOffset != 0 &&
           HashBlock->Table[Idx].KeyOffset != -1 &&
          !strncmp(KeyName, (PCHAR) &HashBlock->Table[Idx].HashValue, 4))
        {
          CurSubKeyBlock = CmiGetBlock(RegistryFile,
                                          HashBlock->Table[Idx].KeyOffset);
          if ( CurSubKeyBlock->NameSize == KeyLength
                && !memcmp(KeyName, CurSubKeyBlock->Name, KeyLength))
            {
              *SubKeyBlock = CurSubKeyBlock;
	      *BlockOffset = HashBlock->Table[Idx].KeyOffset;
DPRINT("key %s found at blockoffset %x\n",KeyName,*BlockOffset);
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
             PCHAR  NewSubKeyName,
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
  if (KeyBlock->HashTableOffset == -1)
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
      HashBlock = CmiGetBlock(RegistryFile, KeyBlock->HashTableOffset);
      if (KeyBlock->NumberOfSubKeys + 1 >= HashBlock->HashTableSize)
        {

          /* FIXME: All Subkeys will need to be rehashed here?  */

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
      if(SubKeyBlock) *SubKeyBlock = NewKeyBlock;
    }
  CmiReleaseBlock(RegistryFile, HashBlock);
  
  return  Status;
}

static NTSTATUS  
CmiScanKeyForValue(IN PREGISTRY_FILE  RegistryFile,
                   IN PKEY_BLOCK  KeyBlock,
                   IN PCHAR  ValueName,
                   OUT PVALUE_BLOCK  *ValueBlock)
{
  ULONG  Idx;
  PVALUE_LIST_BLOCK  ValueListBlock;
  PVALUE_BLOCK  CurValueBlock;
DPRINT("search %s,NBOV=%d,BO values=%x\n"
,ValueName,KeyBlock->NumberOfValues,KeyBlock->ValuesOffset);
  ValueListBlock = CmiGetBlock(RegistryFile, 
                               KeyBlock->ValuesOffset);
  *ValueBlock = NULL;
  if (ValueListBlock == NULL)
    {
      return  STATUS_SUCCESS;
    }
DPRINT("SOv=%d\n",ValueListBlock->SubBlockSize);
  for (Idx = 0; Idx < KeyBlock->NumberOfValues; Idx++)
    {
      CurValueBlock = CmiGetBlock(RegistryFile,
                                  ValueListBlock->Values[Idx]);
DPRINT("compare %s,%4.4s\n",ValueName,CurValueBlock->Name);
      if (CurValueBlock != NULL &&
          CurValueBlock->NameSize == strlen(ValueName) &&
          !memcmp(CurValueBlock->Name, ValueName,strlen(ValueName)))
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
CmiGetValueFromKeyByIndex(IN PREGISTRY_FILE  RegistryFile,
                          IN PKEY_BLOCK  KeyBlock,
                          IN ULONG  Index,
                          OUT PVALUE_BLOCK  *ValueBlock)
{
  PVALUE_LIST_BLOCK  ValueListBlock;
  PVALUE_BLOCK  CurValueBlock;

  ValueListBlock = CmiGetBlock(RegistryFile,
                               KeyBlock->ValuesOffset);
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
                              ValueListBlock->Values[Index]);
  if (CurValueBlock != NULL)
    {
      *ValueBlock = CurValueBlock;
    }
  CmiReleaseBlock(RegistryFile, CurValueBlock);
  CmiReleaseBlock(RegistryFile, ValueListBlock);
  
  return  STATUS_SUCCESS;
}

static NTSTATUS  
CmiAddValueToKey(IN PREGISTRY_FILE  RegistryFile,
                 IN PKEY_BLOCK  KeyBlock,
                 IN PCHAR  ValueNameBuf,
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
                                  REG_VALUE_LIST_BLOCK_MULTIPLE,NULL);
      if (!NT_SUCCESS(Status))
        {
          CmiDestroyValueBlock(RegistryFile,
                               ValueBlock);
          return  Status;
        }
      KeyBlock->ValuesOffset = CmiGetBlockOffset(RegistryFile,
                                                 ValueListBlock);
    }
  else if (KeyBlock->NumberOfValues % REG_VALUE_LIST_BLOCK_MULTIPLE)
    {
      Status = CmiAllocateBlock(RegistryFile,
                                (PVOID) &NewValueListBlock,
                                sizeof(BLOCK_OFFSET) *
                                  (KeyBlock->NumberOfValues + 
                                    REG_VALUE_LIST_BLOCK_MULTIPLE),NULL);
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
  CmiReleaseBlock(RegistryFile, ValueListBlock);
  CmiReleaseBlock(RegistryFile, ValueBlock);

  return  STATUS_SUCCESS;
}

static NTSTATUS  
CmiDeleteValueFromKey(IN PREGISTRY_FILE  RegistryFile,
                      IN PKEY_BLOCK  KeyBlock,
                      IN PCHAR  ValueName)
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
                    IN PCHAR  KeyName,
                    IN ULONG  TitleIndex,
                    IN PWSTR  Class,
                    IN ULONG  CreateOptions)
{
  NTSTATUS  Status;
  ULONG  NewBlockSize;
  PKEY_BLOCK  NewKeyBlock;

  DPRINT("RegistryFile %p KeyBlock %p KeyName %s TitleIndex %x Class %S CreateOptions %x\n",
         RegistryFile, KeyBlock, KeyName, TitleIndex, Class,CreateOptions);

  Status = STATUS_SUCCESS;

      NewBlockSize = sizeof(KEY_BLOCK) + (strlen(KeyName) ) ;
DPRINT ("NewKeySize: %lu\n", NewBlockSize);
//CHECKPOINT;
      Status = CmiAllocateBlock(RegistryFile, (PVOID) &NewKeyBlock , NewBlockSize,NULL);
//CHECKPOINT;
      if (NewKeyBlock == NULL)
        {
          Status = STATUS_INSUFFICIENT_RESOURCES;
        }
      else
        {
          RtlZeroMemory(NewKeyBlock, NewBlockSize);
          NewKeyBlock->SubBlockId = REG_KEY_BLOCK_ID;
          NewKeyBlock->Type = REG_KEY_BLOCK_TYPE;
          ZwQuerySystemTime((PTIME) &NewKeyBlock->LastWriteTime);
          NewKeyBlock->ParentKeyOffset = -1;
          NewKeyBlock->NumberOfSubKeys = 0;
          NewKeyBlock->HashTableOffset = -1;
          NewKeyBlock->NumberOfValues = 0;
          NewKeyBlock->ValuesOffset = -1;
          NewKeyBlock->SecurityKeyOffset = -1;
          NewKeyBlock->NameSize = strlen(KeyName);
          NewKeyBlock->ClassSize = (Class != NULL) ? wcslen(Class) : 0;
          memcpy(NewKeyBlock->Name, KeyName, NewKeyBlock->NameSize );
          if (Class != NULL)
            {
          /* FIXME : ClassName is in a different Block !!! */
            }
          CmiLockBlock(RegistryFile, NewKeyBlock);
          *KeyBlock = NewKeyBlock;
        }

  return  Status;
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

static PKEY_BLOCK  
CmiGetKeyFromHashByIndex(PREGISTRY_FILE RegistryFile,
                         PHASH_TABLE_BLOCK  HashBlock,
                         ULONG  Index)
{
  PKEY_BLOCK  KeyBlock;
  BLOCK_OFFSET KeyOffset;

  if (RegistryFile->Filename == NULL)
    {
      KeyBlock = (PKEY_BLOCK) HashBlock->Table[Index].KeyOffset;
      CmiLockBlock(RegistryFile, KeyBlock);
    }
  else
    {
      KeyOffset =  HashBlock->Table[Index].KeyOffset;
      KeyBlock = CmiGetBlock(RegistryFile,KeyOffset);
      CmiLockBlock(RegistryFile, KeyBlock);
    }

  return  KeyBlock;
}

static NTSTATUS  
CmiAddKeyToHashTable(PREGISTRY_FILE  RegistryFile,
                     PHASH_TABLE_BLOCK  HashBlock,
                     PKEY_BLOCK  NewKeyBlock)
{
  ULONG i;

  for (i = 0; i < HashBlock->HashTableSize; i++)
    {
       if (HashBlock->Table[i].KeyOffset == 0)
         {
            HashBlock->Table[i].KeyOffset =
              CmiGetBlockOffset(RegistryFile, NewKeyBlock);
            RtlCopyMemory(&HashBlock->Table[i].HashValue,
                          NewKeyBlock->Name,
                          4);
            return STATUS_SUCCESS;
         }
    }
  return STATUS_UNSUCCESSFUL;
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
                      IN PCHAR  ValueNameBuf,
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
      NewValueSize = sizeof(VALUE_BLOCK) + strlen(ValueNameBuf);
      NewValueBlock = ExAllocatePool(NonPagedPool, NewValueSize);
      if (NewValueBlock == NULL)
        {
          Status = STATUS_INSUFFICIENT_RESOURCES;
        }
      else
        {
          RtlZeroMemory(NewValueBlock, NewValueSize);
          NewValueBlock->SubBlockId = REG_VALUE_BLOCK_ID;
          NewValueBlock->NameSize = strlen(ValueNameBuf);
          memcpy(NewValueBlock->Name, ValueNameBuf,strlen(ValueNameBuf));
          NewValueBlock->DataType = Type;
          NewValueBlock->DataSize = DataSize;
          Status = CmiAllocateBlock(RegistryFile,
                                    &DataBlock,
                                    DataSize,NULL);
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
                                DataSize,NULL);
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
                 ULONG  BlockSize,
		 BLOCK_OFFSET * BlockOffset)
{
  NTSTATUS  Status;
  PFREE_SUB_BLOCK NewBlock;

  Status = STATUS_SUCCESS;
  /* round to 16 bytes */
  BlockSize = (BlockSize+15) & 0xfffffff0;

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
	  if (BlockOffset) *BlockOffset = (BLOCK_OFFSET)NewBlock;
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
  DWORD CurBlock;
  NTSTATUS Status;
  if( BlockOffset == 0 || BlockOffset == -1) return NULL;

  Block = NULL;
  if (RegistryFile->Filename == NULL)
  {
      CmiLockBlock(RegistryFile, (PVOID) BlockOffset);

      Block = (PVOID) BlockOffset;
  }
  else
  {
    PHEAP_BLOCK * tmpBlockList;
    HEAP_BLOCK tmpHeap;
    LARGE_INTEGER fileOffset;
    HANDLE FileHandle;
     // search in the heap blocks currently in memory
     for (CurBlock =0; CurBlock  < RegistryFile->BlockListSize ; CurBlock ++)
     {
       if (  RegistryFile->BlockList[CurBlock ]->BlockOffset <= BlockOffset 
	      && (RegistryFile->BlockList[CurBlock ]->BlockOffset
                   +RegistryFile->BlockList[CurBlock ]->BlockSize > BlockOffset ))
	    return ((char *)RegistryFile->BlockList[CurBlock ]
			+(BlockOffset - RegistryFile->BlockList[CurBlock ]->BlockOffset));
     }
     /* not in memory : read from file */
     /* increase size of list of blocks */
     tmpBlockList=ExAllocatePool(NonPagedPool,
				   sizeof(PHEAP_BLOCK *)*(CurBlock +1));
     if (tmpBlockList == NULL)
     {
	     KeBugCheck(0);
	     return(FALSE);
     }
     if(RegistryFile->BlockListSize > 0)
     {
          memcpy(tmpBlockList,RegistryFile->BlockList,
	       sizeof(PHEAP_BLOCK *)*(RegistryFile->BlockListSize ));
	  ExFreePool(RegistryFile->BlockList);
     }
     RegistryFile->BlockList = tmpBlockList;
        /* try to find block at 4K limit under blockOffset */
	fileOffset.u.LowPart = (BlockOffset & 0xfffff000)+REG_BLOCK_SIZE;
	fileOffset.u.HighPart = 0;
    	Status = ObCreateHandle(
  		PsGetCurrentProcess(),
  		RegistryFile->FileObject,
  		FILE_READ_DATA,
  		FALSE,
  		&FileHandle
  		);
        Status = ZwReadFile(FileHandle, 
                      0, 0, 0, 0, 
                      &tmpHeap, 
                      sizeof(HEAP_BLOCK), 
                      &fileOffset, 0);
        if (!NT_SUCCESS(Status))
        {
	  DPRINT1("error %x reading registry file\n",Status);
	  ZwClose(FileHandle);
          return NULL;
        }
        /* if it's not a block, try page 4k before ... */
        /* FIXME : slower but better is to start from previous block in memory */
	while (tmpHeap.BlockId != 0x6e696268 && fileOffset.u.LowPart  >= REG_BLOCK_SIZE)
	{
	   fileOffset.u.LowPart  -= REG_BLOCK_SIZE;
           Status = ZwReadFile(FileHandle, 
                      0, 0, 0, 0, 
                      &tmpHeap, 
                      sizeof(HEAP_BLOCK), 
                      &fileOffset, 0);
        if (!NT_SUCCESS(Status))
        {
	  DPRINT1("error %x reading registry file at offset %x\n",Status,fileOffset.u.LowPart);
	  ZwClose(FileHandle);
          return NULL;
        }
	}
	if (tmpHeap.BlockId != 0x6e696268 )
	{
	  DPRINT1("bad BlockId %x,offset %x\n",tmpHeap.BlockId,fileOffset.u.LowPart);
	  ZwClose(FileHandle);
		return NULL;
	}
        RegistryFile->BlockListSize ++;
	RegistryFile->BlockList [CurBlock]
	   = ExAllocatePool(NonPagedPool,tmpHeap.BlockSize);
        Status = ZwReadFile(FileHandle, 
                      0, 0, 0, 0, 
                      RegistryFile->BlockList[CurBlock ],
                      tmpHeap.BlockSize,
                      &fileOffset, 0);
	ZwClose(FileHandle);
        if (!NT_SUCCESS(Status))
        {
	  DPRINT1("error %x reading registry file at offset %x\n",Status,fileOffset.u.LowPart);
          return NULL;
        }
      Block = ((char *)RegistryFile->BlockList[CurBlock]
			+(BlockOffset - RegistryFile->BlockList[CurBlock]->BlockOffset));
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
      /* FIXME : implement */
    }
}

static VOID 
CmiReleaseBlock(PREGISTRY_FILE  RegistryFile,
               PVOID  Block)
{
  if (RegistryFile->Filename != NULL)
    {
      /* FIXME : implement */
    }
}


/* EOF */
