/* $Id: registry.c,v 1.52 2000/12/01 12:44:15 jean Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/cm/registry.c
 * PURPOSE:         Registry functions
 * PROGRAMMERS:     Rex Jolliff
 *                  Matt Pyne
 *                  Jean Michault
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <defines.h>
#include <ddk/ntddk.h>
#include <internal/ob.h>

#define NDEBUG
#include <internal/debug.h>

/*  -----------------------------------------------------  Typedefs  */

//#define LONG_MAX 0x7fffffff

#define  REG_BLOCK_SIZE  		4096
#define  REG_HEAP_BLOCK_DATA_OFFSET  	32
#define  REG_HEAP_ID 			0x6e696268
#define  REG_INIT_BLOCK_LIST_SIZE  	32
#define  REG_INIT_HASH_TABLE_SIZE	3
#define  REG_EXTEND_HASH_TABLE_SIZE  	4
#define  REG_VALUE_LIST_BLOCK_MULTIPLE  4
#define  REG_KEY_BLOCK_ID    		0x6b6e
#define  REG_HASH_TABLE_BLOCK_ID  	0x666c
#define  REG_VALUE_BLOCK_ID  		0x6b76
#define  REG_KEY_BLOCK_TYPE  		0x20
#define  REG_ROOT_KEY_BLOCK_TYPE  	0x2c

#define  REG_ROOT_KEY_NAME  	L"\\Registry"
#define  REG_MACHINE_KEY_NAME  	L"\\Registry\\Machine"
#define  REG_SYSTEM_KEY_NAME  	L"\\Registry\\Machine\\System"
#define  REG_SOFTWARE_KEY_NAME  L"\\Registry\\Machine\\Software"
#define  REG_SAM_KEY_NAME  	L"\\Registry\\Machine\\Sam"
#define  REG_SEC_KEY_NAME  	L"\\Registry\\Machine\\Security"
#define  REG_USERS_KEY_NAME  	L"\\Registry\\User"
#define  REG_USER_KEY_NAME  	L"\\Registry\\User\\CurrentUser"

#define  SYSTEM_REG_FILE  	L"\\SystemRoot\\System32\\Config\\SYSTEM"
#define  SOFTWARE_REG_FILE  	L"\\SystemRoot\\System32\\Config\\SOFTWARE"
#define  USER_REG_FILE  	L"\\SystemRoot\\System32\\Config\\DEFAULT"
#define  SAM_REG_FILE  		L"\\SystemRoot\\System32\\Config\\SAM"
#define  SEC_REG_FILE  		L"\\SystemRoot\\System32\\Config\\SECURITY"

#define  KO_MARKED_FOR_DELETE  0x00000001

// BLOCK_OFFSET = offset in file after header block
typedef DWORD  BLOCK_OFFSET;

/* header for registry hive file : */
typedef struct _HEADER_BLOCK
{
  DWORD  BlockId;		/* ="regf" */
  DWORD  Version;		/* file version ?*/
  DWORD  VersionOld;		/* file version ?*/
  FILETIME  DateModified;
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
  FILETIME  DateModified;
  DWORD  Unused2;
} HEAP_BLOCK, *PHEAP_BLOCK;

// each sub_block begin with this struct :
// in a free subblock, higher bit of SubBlockSize is set
typedef struct _FREE_SUB_BLOCK
{
  LONG  SubBlockSize;/* <0 if used, >0 if free */
} FREE_SUB_BLOCK, *PFREE_SUB_BLOCK;

typedef struct _KEY_BLOCK
{
  LONG  SubBlockSize;
  WORD  SubBlockId;
  WORD  Type;
  FILETIME  LastWriteTime;
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
  LONG  SubBlockSize;
  WORD  SubBlockId;
  WORD  HashTableSize;
  HASH_RECORD  Table[0];
} HASH_TABLE_BLOCK, *PHASH_TABLE_BLOCK;

typedef struct _VALUE_LIST_BLOCK
{
  LONG  SubBlockSize;
  BLOCK_OFFSET  Values[0];
} VALUE_LIST_BLOCK, *PVALUE_LIST_BLOCK;

typedef struct _VALUE_BLOCK
{
  LONG  SubBlockSize;
  WORD  SubBlockId;	// "kv"
  WORD  NameSize;	// length of Name
  LONG  DataSize;	// length of datas in the subblock pointed by DataOffset
  BLOCK_OFFSET  DataOffset;// datas are here if high bit of DataSize is set
  DWORD  DataType;
  WORD  Flags;
  WORD  Unused1;
  UCHAR  Name[0]; /* warning : not zero terminated */
} VALUE_BLOCK, *PVALUE_BLOCK;

typedef struct _DATA_BLOCK
{
  LONG  SubBlockSize;
  UCHAR  Data[0];
} DATA_BLOCK, *PDATA_BLOCK;

typedef struct _REGISTRY_FILE
{
  PWSTR  Filename;
  DWORD FileSize;
  PFILE_OBJECT FileObject;
  PHEADER_BLOCK  HeaderBlock;
//  ULONG  NumberOfBlocks;
  ULONG  BlockListSize;
  PHEAP_BLOCK  *BlockList;
  ULONG  FreeListSize;
  ULONG  FreeListMax;
  PFREE_SUB_BLOCK *FreeList;
  BLOCK_OFFSET *FreeListOffset;
//  KSPIN_LOCK  RegLock;
  KSEMAPHORE RegSem;
  

//  NTSTATUS  (*Extend)(ULONG NewSize);
//  PVOID  (*Flush)(VOID);
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

/*  -----------------------------------------  Forward Declarations  */


static NTSTATUS RtlpGetRegistryHandle(ULONG RelativeTo,
				      PWSTR Path,
				      BOOLEAN Create,
				      PHANDLE KeyHandle);

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
static NTSTATUS  CmiRemoveKeyFromList(PKEY_OBJECT  NewKey);
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
             		      IN PKEY_OBJECT Parent,
                              OUT PKEY_OBJECT SubKey,
                              IN PWSTR  NewSubKeyName,
                              IN USHORT  NewSubKeyNameSize,
                              IN ULONG  TitleIndex,
                              IN PUNICODE_STRING  Class, 
                              IN ULONG  CreateOptions);
static NTSTATUS  CmiScanKeyForValue(IN PREGISTRY_FILE  RegistryFile,
                                    IN PKEY_BLOCK  KeyBlock,
                                    IN PCHAR  ValueName,
                                    OUT PVALUE_BLOCK  *ValueBlock,
				    OUT BLOCK_OFFSET *VBOffset);
static NTSTATUS  CmiGetValueFromKeyByIndex(IN PREGISTRY_FILE  RegistryFile,
                                           IN PKEY_BLOCK  KeyBlock,
                                           IN ULONG  Index,
                                           OUT PVALUE_BLOCK  *ValueBlock);
static NTSTATUS  CmiAddValueToKey(IN PREGISTRY_FILE  RegistryFile,
                                  IN PKEY_BLOCK  KeyBlock,
                                  IN PCHAR  ValueNameBuf,
				  OUT PVALUE_BLOCK *pValueBlock,
				  OUT BLOCK_OFFSET *pVBOffset);
static NTSTATUS  CmiDeleteValueFromKey(IN PREGISTRY_FILE  RegistryFile,
                                       IN PKEY_BLOCK  KeyBlock,
                                       IN PCHAR  ValueName);
static NTSTATUS  CmiAllocateHashTableBlock(IN PREGISTRY_FILE  RegistryFile,
                                           OUT PHASH_TABLE_BLOCK  *HashBlock,
                          		   OUT BLOCK_OFFSET  *HBOffset,
                                           IN ULONG  HashTableSize);
static PKEY_BLOCK  CmiGetKeyFromHashByIndex(PREGISTRY_FILE RegistryFile,
                                            PHASH_TABLE_BLOCK  HashBlock,
                                            ULONG  Index);
static NTSTATUS  CmiAddKeyToHashTable(PREGISTRY_FILE  RegistryFile,
                                      PHASH_TABLE_BLOCK  HashBlock,
                                      PKEY_BLOCK  NewKeyBlock,
                                      BLOCK_OFFSET  NKBOffset);
static NTSTATUS  CmiAllocateValueBlock(IN PREGISTRY_FILE  RegistryFile,
                                       OUT PVALUE_BLOCK  *ValueBlock,
                      		       OUT BLOCK_OFFSET  *VBOffset,
                                       IN PCHAR  ValueNameBuf);
static NTSTATUS  CmiDestroyValueBlock(PREGISTRY_FILE  RegistryFile,
                     PVALUE_BLOCK  ValueBlock, BLOCK_OFFSET VBOffset);
static NTSTATUS  CmiAllocateBlock(PREGISTRY_FILE  RegistryFile,
                                  PVOID  *Block,
                                  LONG  BlockSize,
				  BLOCK_OFFSET * pBlockOffset);
static NTSTATUS  CmiDestroyBlock(PREGISTRY_FILE  RegistryFile,
                PVOID  Block,BLOCK_OFFSET Offset);
static PVOID  CmiGetBlock(PREGISTRY_FILE  RegistryFile,
                          BLOCK_OFFSET  BlockOffset,
			  OUT PHEAP_BLOCK * ppHeap);
static VOID CmiLockBlock(PREGISTRY_FILE  RegistryFile,
                         PVOID  Block);
static VOID  CmiReleaseBlock(PREGISTRY_FILE  RegistryFile,
                             PVOID  Block);
static NTSTATUS
CmiAddFree(PREGISTRY_FILE  RegistryFile,
		PFREE_SUB_BLOCK FreeBlock,BLOCK_OFFSET FreeOffset);


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
DPRINT("Creating root\n");
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
    CmiRootKey->KeyBlock = CmiGetBlock(CmiVolatileFile,CmiVolatileFile->HeaderBlock->RootKeyBlock,NULL);

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
DPRINT("Creating HKLM\n");
  InitializeObjectAttributes(&ObjectAttributes, &RootKeyName, 0, NULL, NULL);
  NewKey=ObCreateObject(&KeyHandle,
                                STANDARD_RIGHTS_REQUIRED,
                 &ObjectAttributes,
                 CmiKeyType);
  Status = CmiAddSubKey(CmiVolatileFile,
                        CmiRootKey,
                        NewKey,
                        L"Machine",
                        14,
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
DPRINT("Creating HKU\n");
  InitializeObjectAttributes(&ObjectAttributes, &RootKeyName, 0, NULL, NULL);
  NewKey=ObCreateObject(&KeyHandle,
                                STANDARD_RIGHTS_REQUIRED,
                 &ObjectAttributes,
                 CmiKeyType);
  Status = CmiAddSubKey(CmiVolatileFile,
                        CmiRootKey,
                        NewKey,
                        L"User",
                        8,
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

NTSTATUS CmConnectHive(PWSTR FileName,PWSTR FullName,CHAR *KeyName,PKEY_OBJECT Parent)
{
 OBJECT_ATTRIBUTES  ObjectAttributes;
 PKEY_OBJECT  NewKey;
 HANDLE  KeyHandle;
 UNICODE_STRING  uKeyName;
 PREGISTRY_FILE  RegistryFile = NULL;
  RegistryFile = CmiCreateRegistry(FileName);
  if( RegistryFile )
  {
    RtlInitUnicodeString(&uKeyName, FullName);
DPRINT("CCH %S ;",FullName);
    InitializeObjectAttributes(&ObjectAttributes, &uKeyName, 0, NULL, NULL);
    NewKey=ObCreateObject(&KeyHandle,
                 STANDARD_RIGHTS_REQUIRED,
                 &ObjectAttributes,
                 CmiKeyType);
    NewKey->RegistryFile = RegistryFile;
    NewKey->BlockOffset = RegistryFile->HeaderBlock->RootKeyBlock;
    NewKey->KeyBlock = CmiGetBlock(RegistryFile,NewKey->BlockOffset,NULL);
    NewKey->Flags = 0;
    NewKey->NumberOfSubKeys=0;
    NewKey->SubKeys= ExAllocatePool(PagedPool
		, NewKey->KeyBlock->NumberOfSubKeys * sizeof(DWORD));
    NewKey->SizeOfSubKeys= NewKey->KeyBlock->NumberOfSubKeys;
    NewKey->Name=ExAllocatePool(PagedPool,strlen(KeyName));
    NewKey->NameSize=strlen(KeyName);
    memcpy(NewKey->Name,KeyName,strlen(KeyName));
    CmiAddKeyToList(Parent,NewKey);
  }
  else
    return STATUS_UNSUCCESSFUL;
  return STATUS_SUCCESS;
}

VOID
CmInitializeRegistry2(VOID)
{
 NTSTATUS Status;
  /* FIXME : delete temporary \Registry\Machine\System */
  /* connect the SYSTEM Hive */
  Status=CmConnectHive(SYSTEM_REG_FILE,REG_SYSTEM_KEY_NAME
			,"System",CmiMachineKey);
  if (!NT_SUCCESS(Status))
  {
    /* FIXME : search SYSTEM.alt, or create new */
    DbgPrint(" warning : registry file %S not found\n",SYSTEM_REG_FILE);
  }
  /* connect the SOFTWARE Hive */
  Status=CmConnectHive(SOFTWARE_REG_FILE,REG_SOFTWARE_KEY_NAME
			,"Software",CmiMachineKey);
  if (!NT_SUCCESS(Status))
  {
    /* FIXME : search SOFTWARE.alt, or create new */
    DbgPrint(" warning : registry file %S not found\n",SOFTWARE_REG_FILE);
  }
  /* connect the SAM Hive */
  Status=CmConnectHive(SAM_REG_FILE,REG_SAM_KEY_NAME
			,"Sam",CmiMachineKey);
  if (!NT_SUCCESS(Status))
  {
    /* FIXME : search SAM.alt, or create new */
    DbgPrint(" warning : registry file %S not found\n",SAM_REG_FILE);
  }
  /* connect the SECURITY Hive */
  Status=CmConnectHive(SEC_REG_FILE,REG_SEC_KEY_NAME
			,"Security",CmiMachineKey);
  if (!NT_SUCCESS(Status))
  {
    /* FIXME : search SECURITY.alt, or create new */
    DbgPrint(" warning : registry file %S not found\n",SEC_REG_FILE);
  }
  /* connect the DEFAULT Hive */
  Status=CmConnectHive(USER_REG_FILE,REG_USER_KEY_NAME
			,".Default",CmiUserKey);
  if (!NT_SUCCESS(Status))
  {
    /* FIXME : search DEFAULT.alt, or create new */
    DbgPrint(" warning : registry file %S not found\n",USER_REG_FILE);
  }
  /* FIXME : initialize standards symbolic links */
/*
for(;;)
{
__asm__ ("hlt\n\t");
}
*/
}

VOID 
CmImportHive(PCHAR  Chunk)
{
  /*  FIXME: implemement this  */
  return; 
}

VOID
CmShutdownRegistry(VOID)
{
  DPRINT1("CmShutdownRegistry()...\n");
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
 UNICODE_STRING RemainingPath;
 PWSTR end;
// KIRQL  OldIrql;

//  DPRINT("NtCreateKey (Name %wZ),KeyHandle=%x,Root=%x\n",
//         ObjectAttributes->ObjectName,KeyHandle
//	,ObjectAttributes->RootDirectory);
  Status = ObFindObject(ObjectAttributes,&Object,&RemainingPath,CmiKeyType);
  if (!NT_SUCCESS(Status))
  {
	return Status;
  }
DPRINT("RP=%wZ\n",&RemainingPath);
  if(RemainingPath.Buffer[0] ==0)
  {
    if (Disposition) *Disposition = REG_OPENED_EXISTING_KEY;
    Status = ObCreateHandle(
		PsGetCurrentProcess(),
		Object,
		DesiredAccess,
		FALSE,
		KeyHandle
		);
    ObDereferenceObject(Object);
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
  key = ObCreateObject(
		KeyHandle,
		DesiredAccess,
		NULL,
		CmiKeyType
		);

  if (key == NULL)
	return STATUS_INSUFFICIENT_RESOURCES;
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


NTSTATUS 
STDCALL
NtFlushKey (
	IN	HANDLE	KeyHandle
	)
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

/*
   if (ObjectAttributes->RootDirectory == HKEY_LOCAL_MACHINE)
   {
     Status = ObCreateHandle(
  		PsGetCurrentProcess(),
  		CmiMachineKey,
  		KEY_ALL_ACCESS,
  		FALSE,
  		&ObjectAttributes->RootDirectory
  		);
   }
   else if (ObjectAttributes->RootDirectory == HKEY_USERS)
   {
     Status = ObCreateHandle(
  		PsGetCurrentProcess(),
  		CmiUserKey,
  		KEY_ALL_ACCESS,
  		FALSE,
  		&ObjectAttributes->RootDirectory
  		);
   }
*/
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


NTSTATUS
STDCALL
RtlCheckRegistryKey (
	IN	ULONG	RelativeTo,
	IN	PWSTR	Path
	)
{
   HANDLE KeyHandle;
   NTSTATUS Status;

   Status = RtlpGetRegistryHandle(RelativeTo,
				  Path,
				  FALSE,
				  &KeyHandle);
   if (!NT_SUCCESS(Status))
     return Status;

   NtClose(KeyHandle);

   return STATUS_SUCCESS;
}


NTSTATUS
STDCALL
RtlCreateRegistryKey (
	IN	ULONG	RelativeTo,
	IN	PWSTR	Path
	)
{
   HANDLE KeyHandle;
   NTSTATUS Status;

   Status = RtlpGetRegistryHandle(RelativeTo,
				  Path,
				  TRUE,
				  &KeyHandle);
   if (!NT_SUCCESS(Status))
     return Status;

   NtClose(KeyHandle);

   return STATUS_SUCCESS;
}


NTSTATUS
STDCALL
RtlDeleteRegistryValue (
	IN	ULONG	RelativeTo,
	IN	PWSTR	Path,
	IN	PWSTR	ValueName
	)
{
   HANDLE KeyHandle;
   NTSTATUS Status;
   UNICODE_STRING Name;

   Status = RtlpGetRegistryHandle(RelativeTo,
				  Path,
				  TRUE,
				  &KeyHandle);
   if (!NT_SUCCESS(Status))
     return Status;

   RtlInitUnicodeString(&Name,
			ValueName);

   NtDeleteValueKey(KeyHandle,
		    &Name);

   NtClose(KeyHandle);

   return STATUS_SUCCESS;
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
   HANDLE KeyHandle;
   NTSTATUS Status;
   UNICODE_STRING Name;

   Status = RtlpGetRegistryHandle(RelativeTo,
				  Path,
				  TRUE,
				  &KeyHandle);
   if (!NT_SUCCESS(Status))
     return Status;

   RtlInitUnicodeString(&Name,
			ValueName);

   NtSetValueKey(KeyHandle,
		 &Name,
		 0,
		 ValueType,
		 ValueData,
		 ValueLength);

   NtClose(KeyHandle);

   return STATUS_SUCCESS;
}

NTSTATUS STDCALL
RtlFormatCurrentUserKeyPath(IN OUT PUNICODE_STRING KeyPath)
{
   return STATUS_UNSUCCESSFUL;
}

/*  ------------------------------------------  Private Implementation  */


static NTSTATUS RtlpGetRegistryHandle(ULONG RelativeTo,
				      PWSTR Path,
				      BOOLEAN Create,
				      PHANDLE KeyHandle)
{
   UNICODE_STRING KeyName;
   WCHAR KeyBuffer[MAX_PATH];
   OBJECT_ATTRIBUTES ObjectAttributes;
   NTSTATUS Status;

   if (RelativeTo & RTL_REGISTRY_HANDLE)
     {
	*KeyHandle = (HANDLE)Path;
	return STATUS_SUCCESS;
     }

   if (RelativeTo & RTL_REGISTRY_OPTIONAL)
     RelativeTo &= ~RTL_REGISTRY_OPTIONAL;

   if (RelativeTo >= RTL_REGISTRY_MAXIMUM)
     return STATUS_INVALID_PARAMETER;

   KeyName.Length = 0;
   KeyName.MaximumLength = MAX_PATH;
   KeyName.Buffer = KeyBuffer;
   KeyBuffer[0] = 0;

   switch (RelativeTo)
     {
	case RTL_REGISTRY_SERVICES:
	  RtlAppendUnicodeToString(&KeyName,
				   L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\");
	  break;

	case RTL_REGISTRY_CONTROL:
	  RtlAppendUnicodeToString(&KeyName,
				   L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\");
	  break;

	case RTL_REGISTRY_WINDOWS_NT:
	  RtlAppendUnicodeToString(&KeyName,
				   L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion\\");
	  break;

	case RTL_REGISTRY_DEVICEMAP:
	  RtlAppendUnicodeToString(&KeyName,
				   L"\\Registry\\Machine\\Hardware\\DeviceMap\\");
	  break;

	case RTL_REGISTRY_USER:
	  Status = RtlFormatCurrentUserKeyPath(&KeyName);
	  if (!NT_SUCCESS(Status))
	    return Status;
	  break;
     }

   if (Path[0] != L'\\')
     RtlAppendUnicodeToString(&KeyName,
			      L"\\");

   RtlAppendUnicodeToString(&KeyName,
			    Path);

   InitializeObjectAttributes(&ObjectAttributes,
			      &KeyName,
			      OBJ_CASE_INSENSITIVE,
			      NULL,
			      NULL);

   if (Create == TRUE)
     {
	Status = NtCreateKey(KeyHandle,
			     KEY_ALL_ACCESS,
			     &ObjectAttributes,
			     0,
			     NULL,
			     0,
			     NULL);
     }
   else
     {
	Status = NtOpenKey(KeyHandle,
			   KEY_ALL_ACCESS,
			   &ObjectAttributes);
     }

   return Status;
}

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
   /* FIXME : we must treat the OBJ_CASE_INSENTIVE Flag */
   /*        ... but actually CmiObjectParse don't receive this information */
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
DPRINT("COP %s;",cPath);
       FoundObject = ObCreateObject(NULL, 
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
   else
     ObReferenceObjectByPointer(FoundObject,
			      STANDARD_RIGHTS_REQUIRED,
			      NULL,
			      UserMode);
DPRINT("COP %6.6s ;",FoundObject->Name);   
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
   pKey->ParentKey = Parent;
   if (RemainingPath)
   {
     if(RemainingPath[0]== L'\\')
     {
       pKey->Name = (PCHAR) (&RemainingPath[1]);
       pKey->NameSize = wcslen(RemainingPath)-1;
     }
     else
     {
       pKey->Name = (PCHAR) RemainingPath;
       pKey->NameSize = wcslen(RemainingPath);
     }
   }
   else
     pKey->NameSize = 0;

   return STATUS_SUCCESS;
}

static VOID  
CmiObjectDelete(PVOID  DeletedObject)
{
  PKEY_OBJECT  KeyObject;

DPRINT("delete object key\n");
  KeyObject = (PKEY_OBJECT) DeletedObject;
  if(!NT_SUCCESS(CmiRemoveKeyFromList(KeyObject)))
  {
    DPRINT1("Key not found in parent list ???\n");
  }
  if (KeyObject->Flags & KO_MARKED_FOR_DELETE)
    {
DPRINT1("delete really key\n");
      CmiDestroyBlock(KeyObject->RegistryFile,
                         KeyObject->KeyBlock,
			 KeyObject->BlockOffset);
    }
  else
    {
      CmiReleaseBlock(KeyObject->RegistryFile,
                      KeyObject->KeyBlock);
    }
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
  /*      to allow a dichotomic search */
  ParentKey->SubKeys[ParentKey->NumberOfSubKeys++] = NewKey;
  ObReferenceObjectByPointer(ParentKey,
			      STANDARD_RIGHTS_REQUIRED,
			      NULL,
			      UserMode);
  NewKey->ParentKey = ParentKey;
  KeReleaseSpinLock(&CmiKeyListLock, OldIrql);
}

static NTSTATUS
CmiRemoveKeyFromList(PKEY_OBJECT  KeyToRemove)
{
  KIRQL  OldIrql;
  PKEY_OBJECT  ParentKey;
  DWORD Index;

  ParentKey=KeyToRemove->ParentKey;
  KeAcquireSpinLock(&CmiKeyListLock, &OldIrql);
  /* FIXME : if list maintained in alphabetic order, use dichotomic search */
  for (Index=0; Index < ParentKey->NumberOfSubKeys; Index++)
  {
    if(ParentKey->SubKeys[Index] == KeyToRemove)
    {
      if (Index < ParentKey->NumberOfSubKeys-1)
        memmove(&ParentKey->SubKeys[Index]
		,&ParentKey->SubKeys[Index+1]
		,(ParentKey->NumberOfSubKeys-Index-1)*sizeof(PKEY_OBJECT));
      ParentKey->NumberOfSubKeys--;
      KeReleaseSpinLock(&CmiKeyListLock, OldIrql);
      ObDereferenceObject(ParentKey);
      return STATUS_SUCCESS;
    }
  }
  KeReleaseSpinLock(&CmiKeyListLock, OldIrql);
  return STATUS_UNSUCCESSFUL;
}

static PKEY_OBJECT
CmiScanKeyList(PKEY_OBJECT Parent,PCHAR  KeyName)
{
 KIRQL  OldIrql;
 PKEY_OBJECT  CurKey;
 DWORD Index;
 WORD NameSize;
  NameSize=strlen(KeyName);
  KeAcquireSpinLock(&CmiKeyListLock, &OldIrql);
  /* FIXME : if list maintained in alphabetic order, use dichotomic search */
  for (Index=0; Index < Parent->NumberOfSubKeys; Index++)
  {
    CurKey=Parent->SubKeys[Index];
    /* FIXME : perhaps we must not ignore case if NtCreateKey has not been */
    /*         called with OBJ_CASE_INSENSITIVE flag ? */
    if( NameSize == CurKey->NameSize
	&& !_strnicmp(KeyName,CurKey->Name,NameSize))
    {
       KeReleaseSpinLock(&CmiKeyListLock, OldIrql);
       return CurKey;
    }
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
 PHEAP_BLOCK tmpHeap;
 LARGE_INTEGER fileOffset;
 PFREE_SUB_BLOCK  FreeBlock;
 DWORD  FreeOffset;
 int i, j;
 BLOCK_OFFSET BlockOffset;

  RegistryFile = ExAllocatePool(NonPagedPool, sizeof(REGISTRY_FILE));
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
      /* FIXME:  if file does not exist, create new file  */
      /* else attempt to map the file  */
      RtlInitUnicodeString (&TmpFileName, Filename);
      InitializeObjectAttributes(&ObjectAttributes,
                             &TmpFileName,
                             0,
                             NULL,
                             NULL);
      Status = NtOpenFile(&FileHandle,
                      FILE_ALL_ACCESS,
                      &ObjectAttributes,
                      NULL, 0, 0);
      /* FIXME:  if file does not exist, create new file  */
      if( !NT_SUCCESS(Status) )
      {
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
      ZwQueryInformationFile(FileHandle,&IoSB,&fsi
		,sizeof(fsi),FileStandardInformation);
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
	DPRINT1("error allocating %d bytes for registry\n"
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
	DPRINT1("error %x reading registry file at offset %x\n"
			,Status,fileOffset.u.LowPart);
        return NULL;
      }
      RegistryFile->FreeListSize = 0;
      RegistryFile->FreeListMax = 0;
      RegistryFile->FreeList = NULL;
      for(i=0 ; i <RegistryFile->BlockListSize; i++)
      {
        tmpHeap = (PHEAP_BLOCK)(((char *)RegistryFile->BlockList [0])+BlockOffset);
	if (tmpHeap->BlockId != 0x6e696268 )
	{
	  DPRINT1("bad BlockId %x,offset %x\n",tmpHeap->BlockId,fileOffset.u.LowPart);
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
        FreeOffset=32;
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
      RtlZeroMemory(RegistryFile->HeaderBlock, sizeof(HEADER_BLOCK));
      RegistryFile->HeaderBlock->BlockId = 0x66676572;
      RegistryFile->HeaderBlock->DateModified.dwLowDateTime = 0;
      RegistryFile->HeaderBlock->DateModified.dwHighDateTime = 0;
      RegistryFile->HeaderBlock->Version = 1;
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
  KeInitializeSemaphore(&RegistryFile->RegSem, 1, 1);

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

static ULONG  
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

static ULONG  
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

static ULONG  
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
      /* FIXME : perhaps we must not ignore case if NtCreateKey has not been */
      /*         called with OBJ_CASE_INSENSITIVE flag ? */
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

  CmiReleaseBlock(RegistryFile, HashBlock);
  
  return  STATUS_SUCCESS;
}

static NTSTATUS
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

static NTSTATUS  
CmiScanKeyForValue(IN PREGISTRY_FILE  RegistryFile,
                   IN PKEY_BLOCK  KeyBlock,
                   IN PCHAR  ValueName,
                   OUT PVALUE_BLOCK  *ValueBlock,
		   OUT BLOCK_OFFSET *VBOffset)
{
 ULONG  Idx;
 PVALUE_LIST_BLOCK  ValueListBlock;
 PVALUE_BLOCK  CurValueBlock;
  ValueListBlock = CmiGetBlock(RegistryFile, 
                               KeyBlock->ValuesOffset,NULL);
  *ValueBlock = NULL;
  if (ValueListBlock == NULL)
    {
      return  STATUS_SUCCESS;
    }
  for (Idx = 0; Idx < KeyBlock->NumberOfValues; Idx++)
    {
      CurValueBlock = CmiGetBlock(RegistryFile,
                                  ValueListBlock->Values[Idx],NULL);
      /* FIXME : perhaps we must not ignore case if NtCreateKey has not been */
      /*         called with OBJ_CASE_INSENSITIVE flag ? */
      if (CurValueBlock != NULL &&
          CurValueBlock->NameSize == strlen(ValueName) &&
          !_strnicmp(CurValueBlock->Name, ValueName,strlen(ValueName)))
        {
          *ValueBlock = CurValueBlock;
	  if(VBOffset) *VBOffset = ValueListBlock->Values[Idx];
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

static NTSTATUS  
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

static NTSTATUS  
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

static NTSTATUS
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

static PKEY_BLOCK  
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

static NTSTATUS  
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

static NTSTATUS
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

static NTSTATUS
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

static NTSTATUS
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

static NTSTATUS
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

static NTSTATUS
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

static NTSTATUS
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

static PVOID
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
