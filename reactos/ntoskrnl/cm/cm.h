
#ifndef __INCLUDE_CM_H
#define __INCLUDE_CM_H

#define  REG_ROOT_KEY_NAME		L"\\Registry"
#define  REG_MACHINE_KEY_NAME		L"\\Registry\\Machine"
#define  REG_HARDWARE_KEY_NAME		L"\\Registry\\Machine\\HARDWARE"
#define  REG_DESCRIPTION_KEY_NAME	L"\\Registry\\Machine\\HARDWARE\\DESCRIPTION"
#define  REG_DEVICEMAP_KEY_NAME		L"\\Registry\\Machine\\HARDWARE\\DEVICEMAP"
#define  REG_RESOURCEMAP_KEY_NAME	L"\\Registry\\Machine\\HARDWARE\\RESOURCEMAP"
#define  REG_CLASSES_KEY_NAME		L"\\Registry\\Machine\\Software\\Classes"
#define  REG_SYSTEM_KEY_NAME		L"\\Registry\\Machine\\System"
#define  REG_SOFTWARE_KEY_NAME		L"\\Registry\\Machine\\Software"
#define  REG_SAM_KEY_NAME		L"\\Registry\\Machine\\Sam"
#define  REG_SEC_KEY_NAME		L"\\Registry\\Machine\\Security"
#define  REG_USERS_KEY_NAME		L"\\Registry\\User"
#define  REG_USER_KEY_NAME		L"\\Registry\\User\\CurrentUser"
#define  SYSTEM_REG_FILE		L"\\SystemRoot\\System32\\Config\\SYSTEM"
#define  SOFTWARE_REG_FILE		L"\\SystemRoot\\System32\\Config\\SOFTWARE"
#define  USER_REG_FILE			L"\\SystemRoot\\System32\\Config\\DEFAULT"
#define  SAM_REG_FILE			L"\\SystemRoot\\System32\\Config\\SAM"
#define  SEC_REG_FILE			L"\\SystemRoot\\System32\\Config\\SECURITY"

#define  REGISTRY_FILE_MAGIC    "REGEDIT4"

#define  REG_MACHINE_STD_HANDLE_NAME  "HKEY_LOCAL_MACHINE"
#define  REG_CLASSES_STD_HANDLE_NAME  "HKEY_CLASSES_ROOT"
#define  REG_USERS_STD_HANDLE_NAME    "HKEY_USERS"
#define  REG_USER_STD_HANDLE_NAME     "HKEY_CURRENT_USER"
#define  REG_CONFIG_STD_HANDLE_NAME   "HKEY_CURRENT_CONFIG"
#define  REG_DYN_STD_HANDLE_NAME      "HKEY_DYN_DATA"
#define  MAX_REG_STD_HANDLE_NAME      19

#define  KO_MARKED_FOR_DELETE  0x00000001

// BLOCK_OFFSET = offset in file after header block
typedef DWORD  BLOCK_OFFSET;

/* header for registry hive file : */
typedef struct _HEADER_BLOCK
{
  ULONG  BlockId;		/* ="regf" */
  ULONG  Version;		/* file version ?*/
  ULONG  VersionOld;		/* file version ?*/
  FILETIME  DateModified;	/* please don't replace with LARGE_INTEGER !*/
  ULONG  Unused3;		/* registry format version ? */
  ULONG  Unused4;		/* registry format version ? */
  ULONG  Unused5;		/* registry format version ? */
  ULONG  Unused6;		/* registry format version ? */
  BLOCK_OFFSET  RootKeyBlock;
  ULONG  BlockSize;
  ULONG  Unused7;
  WCHAR  FileName[64];		/* end of file name */
  ULONG  Unused8[83];
  ULONG  Checksum;
} HEADER_BLOCK, *PHEADER_BLOCK;

typedef struct _HEAP_BLOCK
{
  ULONG  BlockId;		/* = "hbin" */
  BLOCK_OFFSET  BlockOffset;	/* block offset of this heap */
  ULONG  BlockSize;		/* size in bytes, 4k multiple */
  ULONG  Unused1;
  FILETIME  DateModified;	/* please don't replace with LARGE_INTEGER !*/
  ULONG  Unused2;
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
  USHORT SubBlockId;
  USHORT Type;
  FILETIME  LastWriteTime;	/* please don't replace with LARGE_INTEGER !*/
  ULONG  UnUsed1;
  BLOCK_OFFSET  ParentKeyOffset;
  ULONG  NumberOfSubKeys;
  ULONG  UnUsed2;
  BLOCK_OFFSET  HashTableOffset;
  ULONG  UnUsed3;
  ULONG  NumberOfValues;
  BLOCK_OFFSET  ValuesOffset;
  BLOCK_OFFSET  SecurityKeyOffset;
  BLOCK_OFFSET  ClassNameOffset;
  ULONG  Unused4[5];
  USHORT NameSize;
  USHORT ClassSize; /* size of ClassName in bytes */
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
  USHORT SubBlockId;
  USHORT HashTableSize;
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
  USHORT SubBlockId;	// "kv"
  USHORT NameSize;	// length of Name
  LONG  DataSize;	// length of datas in the subblock pointed by DataOffset
  BLOCK_OFFSET  DataOffset;// datas are here if high bit of DataSize is set
  ULONG  DataType;
  USHORT Flags;
  USHORT Unused1;
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
  ULONG  FileSize;
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
  USHORT NameSize;	// length of Name
  UCHAR  *Name;
  PREGISTRY_FILE  RegistryFile;
  BLOCK_OFFSET BlockOffset;
  PKEY_BLOCK  KeyBlock;
  struct _KEY_OBJECT  *ParentKey;
  ULONG  NumberOfSubKeys;		/* subkeys loaded in SubKeys */
  ULONG  SizeOfSubKeys;			/* space allocated in SubKeys */
  struct _KEY_OBJECT  **SubKeys;		/* list of subkeys loaded */
} KEY_OBJECT, *PKEY_OBJECT;


NTSTATUS STDCALL
CmiObjectParse(PVOID ParsedObject,
	       PVOID *NextObject,
	       PUNICODE_STRING FullPath,
	       PWSTR *Path,
	       POBJECT_TYPE ObjectType,
	       ULONG Attribute);

NTSTATUS STDCALL
CmiObjectCreate(PVOID ObjectBody,
		PVOID Parent,
		PWSTR RemainingPath,
		struct _OBJECT_ATTRIBUTES* ObjectAttributes);

VOID STDCALL
CmiObjectDelete(PVOID  DeletedObject);

VOID  CmiAddKeyToList(PKEY_OBJECT ParentKey,PKEY_OBJECT  NewKey);
NTSTATUS  CmiRemoveKeyFromList(PKEY_OBJECT  NewKey);
PKEY_OBJECT  CmiScanKeyList(PKEY_OBJECT Parent,
                                   PCHAR KeyNameBuf,
                                   ULONG Attributes);

PREGISTRY_FILE  CmiCreateRegistry(PWSTR  Filename);

ULONG  CmiGetMaxNameLength(PREGISTRY_FILE  RegistryFile,
                                  PKEY_BLOCK  KeyBlock);
ULONG  CmiGetMaxClassLength(PREGISTRY_FILE  RegistryFile,
                                   PKEY_BLOCK  KeyBlock);
ULONG  CmiGetMaxValueNameLength(PREGISTRY_FILE  RegistryFile,
                                       PKEY_BLOCK  KeyBlock);
ULONG  CmiGetMaxValueDataLength(PREGISTRY_FILE  RegistryFile,
                                       PKEY_BLOCK  KeyBlock);

NTSTATUS  CmiScanForSubKey(IN PREGISTRY_FILE  RegistryFile,
                                  IN PKEY_BLOCK  KeyBlock,
                                  OUT PKEY_BLOCK  *SubKeyBlock,
                                  OUT BLOCK_OFFSET *BlockOffset,
                                  IN PCHAR  KeyName,
                                  IN ACCESS_MASK  DesiredAccess,
                                  IN ULONG Attributes);
NTSTATUS  CmiAddSubKey(IN PREGISTRY_FILE  RegistryFile,
                              IN PKEY_OBJECT Parent,
                              OUT PKEY_OBJECT SubKey,
                              IN PWSTR  NewSubKeyName,
                              IN USHORT  NewSubKeyNameSize,
                              IN ULONG  TitleIndex,
                              IN PUNICODE_STRING  Class,
                              IN ULONG  CreateOptions);

NTSTATUS  CmiScanKeyForValue(IN PREGISTRY_FILE  RegistryFile,
                                    IN PKEY_BLOCK  KeyBlock,
                                    IN PCHAR  ValueName,
                                    OUT PVALUE_BLOCK  *ValueBlock,
				    OUT BLOCK_OFFSET *VBOffset);
NTSTATUS  CmiGetValueFromKeyByIndex(IN PREGISTRY_FILE  RegistryFile,
                                           IN PKEY_BLOCK  KeyBlock,
                                           IN ULONG  Index,
                                           OUT PVALUE_BLOCK  *ValueBlock);
NTSTATUS  CmiAddValueToKey(IN PREGISTRY_FILE  RegistryFile,
                                  IN PKEY_BLOCK  KeyBlock,
                                  IN PCHAR  ValueNameBuf,
				  OUT PVALUE_BLOCK *pValueBlock,
				  OUT BLOCK_OFFSET *pVBOffset);
NTSTATUS  CmiDeleteValueFromKey(IN PREGISTRY_FILE  RegistryFile,
                                       IN PKEY_BLOCK  KeyBlock,
                                       IN PCHAR  ValueName);

NTSTATUS  CmiAllocateHashTableBlock(IN PREGISTRY_FILE  RegistryFile,
                                           OUT PHASH_TABLE_BLOCK  *HashBlock,
                                           OUT BLOCK_OFFSET  *HBOffset,
                                           IN ULONG  HashTableSize);
PKEY_BLOCK  CmiGetKeyFromHashByIndex(PREGISTRY_FILE RegistryFile,
                                            PHASH_TABLE_BLOCK  HashBlock,
                                            ULONG  Index);
NTSTATUS  CmiAddKeyToHashTable(PREGISTRY_FILE  RegistryFile,
                                      PHASH_TABLE_BLOCK  HashBlock,
                                      PKEY_BLOCK  NewKeyBlock,
                                      BLOCK_OFFSET  NKBOffset);

NTSTATUS  CmiAllocateValueBlock(IN PREGISTRY_FILE  RegistryFile,
                                       OUT PVALUE_BLOCK  *ValueBlock,
                                       OUT BLOCK_OFFSET  *VBOffset,
                                       IN PCHAR  ValueNameBuf);
NTSTATUS  CmiDestroyValueBlock(PREGISTRY_FILE  RegistryFile,
                     PVALUE_BLOCK  ValueBlock, BLOCK_OFFSET VBOffset);

NTSTATUS  CmiAllocateBlock(PREGISTRY_FILE  RegistryFile,
                                  PVOID  *Block,
                                  LONG  BlockSize,
				  BLOCK_OFFSET * pBlockOffset);
NTSTATUS  CmiDestroyBlock(PREGISTRY_FILE  RegistryFile,
                PVOID  Block,BLOCK_OFFSET Offset);
PVOID  CmiGetBlock(PREGISTRY_FILE  RegistryFile,
                          BLOCK_OFFSET  BlockOffset,
			  OUT PHEAP_BLOCK * ppHeap);
VOID CmiLockBlock(PREGISTRY_FILE  RegistryFile,
                         PVOID  Block);
VOID  CmiReleaseBlock(PREGISTRY_FILE  RegistryFile,
                             PVOID  Block);
NTSTATUS
CmiAddFree(PREGISTRY_FILE  RegistryFile,
		PFREE_SUB_BLOCK FreeBlock,BLOCK_OFFSET FreeOffset);

#endif /*__INCLUDE_CM_H*/
