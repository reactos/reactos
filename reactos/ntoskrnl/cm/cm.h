#ifndef __INCLUDE_CM_H
#define __INCLUDE_CM_H

#ifdef DBG
#define CHECKED 1
#else
#define CHECKED 0
#endif

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

#define  REG_BLOCK_SIZE                4096
#define  REG_HBIN_DATA_OFFSET          32
#define  REG_BIN_ID                    0x6e696268
#define  REG_INIT_BLOCK_LIST_SIZE      32
#define  REG_INIT_HASH_TABLE_SIZE      3
#define  REG_EXTEND_HASH_TABLE_SIZE    4
#define  REG_VALUE_LIST_CELL_MULTIPLE  4
#define  REG_KEY_CELL_ID               0x6b6e
#define  REG_HASH_TABLE_BLOCK_ID       0x666c
#define  REG_VALUE_CELL_ID             0x6b76
#define  REG_LINK_KEY_CELL_TYPE        0x10
#define  REG_KEY_CELL_TYPE             0x20
#define  REG_ROOT_KEY_CELL_TYPE        0x2c
#define  REG_HIVE_ID                   0x66676572

#define  REGISTRY_FILE_MAGIC    "REGEDIT4"

#define  REG_MACHINE_STD_HANDLE_NAME  "HKEY_LOCAL_MACHINE"
#define  REG_CLASSES_STD_HANDLE_NAME  "HKEY_CLASSES_ROOT"
#define  REG_USERS_STD_HANDLE_NAME    "HKEY_USERS"
#define  REG_USER_STD_HANDLE_NAME     "HKEY_CURRENT_USER"
#define  REG_CONFIG_STD_HANDLE_NAME   "HKEY_CURRENT_CONFIG"
#define  REG_DYN_STD_HANDLE_NAME      "HKEY_DYN_DATA"
#define  MAX_REG_STD_HANDLE_NAME      19

// BLOCK_OFFSET = offset in file after header block
typedef DWORD BLOCK_OFFSET;

/* header for registry hive file : */
typedef struct _HIVE_HEADER
{
  /* Hive identifier "regf" (0x66676572) */
  ULONG  BlockId;

  /* File version ? */
  ULONG  Version;

  /* File version ? - same as Version */
  ULONG  VersionOld;

  /* When this hive file was last modified */
  FILETIME  DateModified;

  /* Registry format version ? (1?) */
  ULONG  Unused3;

  /* Registry format version ? (3?) */
  ULONG  Unused4;

  /* Registry format version ? (0?) */
  ULONG  Unused5;

  /* Registry format version ? (1?) */
  ULONG  Unused6;

  /* Offset into file from the byte after the end of the base block.
     If the hive is volatile, this is the actual pointer to the KEY_CELL */
  BLOCK_OFFSET  RootKeyCell;

  /* Size of each hive block ? */
  ULONG  BlockSize;

  /* (1?) */
  ULONG  Unused7;

  /* Name of hive file */
  WCHAR  FileName[64];

  /* ? */
  ULONG  Unused8[83];

  /* Checksum of first 0x200 bytes */
  ULONG  Checksum;
} __attribute__((packed)) HIVE_HEADER, *PHIVE_HEADER;

typedef struct _HBIN
{
  /* Bin identifier "hbin" (0x6E696268) */
  ULONG  BlockId;

  /* Block offset of this bin */
  BLOCK_OFFSET  BlockOffset;

  /* Size in bytes, multiple of the block size (4KB) */
  ULONG  BlockSize;

  /* ? */
  ULONG  Unused1;

  /* When this bin was last modified */
  FILETIME  DateModified;

  /* ? */
  ULONG  Unused2;
} __attribute__((packed)) HBIN, *PHBIN;

typedef struct _CELL_HEADER
{
  /* <0 if used, >0 if free */
  LONG  CellSize;
} __attribute__((packed)) CELL_HEADER, *PCELL_HEADER;

typedef struct _KEY_CELL
{
  /* Size of this cell */
  LONG  CellSize;

  /* Key cell identifier "kn" (0x6b6e) */
  USHORT  Id;

  /* ? */
  USHORT  Type;

  /* Time of last flush */
  FILETIME  LastWriteTime;

  /* ? */
  ULONG  UnUsed1;

  /* Block offset of parent key cell */
  BLOCK_OFFSET  ParentKeyOffset;

  /* Count of sub keys for the key in this key cell */
  ULONG  NumberOfSubKeys;

  /* ? */
  ULONG  UnUsed2;

  /* Block offset of has table for FIXME: subkeys/values? */
  BLOCK_OFFSET  HashTableOffset;

  /* ? */
  ULONG  UnUsed3;

  /* Count of values contained in this key cell */
  ULONG  NumberOfValues;

  /* Block offset of VALUE_LIST_CELL */
  BLOCK_OFFSET  ValuesOffset;

  /* Block offset of security cell */
  BLOCK_OFFSET  SecurityKeyOffset;

  /* Block offset of registry key class */
  BLOCK_OFFSET  ClassNameOffset;

  /* ? */
  ULONG  Unused4[5];

  /* Size in bytes of key name */
  USHORT NameSize;

  /* Size of class name in bytes */
  USHORT ClassSize;

  /* Name of key (not zero terminated) */
  UCHAR  Name[0];
} __attribute__((packed)) KEY_CELL, *PKEY_CELL;

// hash record :
// HashValue=four letters of value's name
typedef struct _HASH_RECORD
{
  BLOCK_OFFSET  KeyOffset;
  ULONG  HashValue;
} __attribute__((packed)) HASH_RECORD, *PHASH_RECORD;

typedef struct _HASH_TABLE_CELL
{
  LONG  CellSize;
  USHORT  Id;
  USHORT  HashTableSize;
  HASH_RECORD  Table[0];
} __attribute__((packed)) HASH_TABLE_CELL, *PHASH_TABLE_CELL;

typedef struct _VALUE_LIST_CELL
{
  LONG  CellSize;
  BLOCK_OFFSET  Values[0];
} __attribute__((packed)) VALUE_LIST_CELL, *PVALUE_LIST_CELL;

typedef struct _VALUE_CELL
{
  LONG  CellSize;
  USHORT Id;	// "kv"
  USHORT NameSize;	// length of Name
  LONG  DataSize;	// length of datas in the cell pointed by DataOffset
  BLOCK_OFFSET  DataOffset;// datas are here if high bit of DataSize is set
  ULONG  DataType;
  USHORT Flags;
  USHORT Unused1;
  UCHAR  Name[0]; /* warning : not zero terminated */
} __attribute__((packed)) VALUE_CELL, *PVALUE_CELL;

typedef struct _DATA_CELL
{
  LONG  CellSize;
  UCHAR  Data[0];
} __attribute__((packed)) DATA_CELL, *PDATA_CELL;

typedef struct _REGISTRY_HIVE
{
  ULONG  Flags;
  UNICODE_STRING  Filename;
  ULONG  FileSize;
  PFILE_OBJECT  FileObject;
  PVOID  Bcb;
  PHIVE_HEADER  HiveHeader;
  ULONG  BlockListSize;
  PHBIN  *BlockList;
  ULONG  FreeListSize;
  ULONG  FreeListMax;
  PCELL_HEADER *FreeList;
  BLOCK_OFFSET *FreeListOffset;
//  KSPIN_LOCK  RegLock;
  KSEMAPHORE RegSem;
//  NTSTATUS  (*Extend)(ULONG NewSize);
//  PVOID  (*Flush)(VOID);
} REGISTRY_HIVE, *PREGISTRY_HIVE;

/* REGISTRY_HIVE.Flags constants */
#define HIVE_VOLATILE   0x00000001

#define IsVolatileHive(Hive)(Hive->Flags & HIVE_VOLATILE)
#define IsPermanentHive(Hive)(!(Hive->Flags & HIVE_VOLATILE))

#define IsFreeCell(Cell)(Cell->CellSize >= 0)
#define IsUsedCell(Cell)(Cell->CellSize < 0)


/* KEY_OBJECT.Flags */

/* When set, the key is sheduled for deletion, and all
   atempts to access the key must not succeed */
#define KO_MARKED_FOR_DELETE              0x00000001


/* Type defining the Object Manager Key Object */
typedef struct _KEY_OBJECT
{
  /* Fields used by the Object Manager */
  CSHORT Type;
  CSHORT Size;

  /* Key flags */
  ULONG Flags;

  /* Length of Name */
  USHORT NameSize;

  /* Name of key */
  PCHAR Name;

  /* Registry hive the key belong to */
  PREGISTRY_HIVE RegistryHive;

  /* Block offset of the key cell this key belong in */
  BLOCK_OFFSET BlockOffset;

  /* KEY_CELL this key belong in */
  PKEY_CELL KeyCell;

  /* Link to the parent KEY_OBJECT for this key */
  struct _KEY_OBJECT *ParentKey;

  /* Subkeys loaded in SubKeys */
  ULONG NumberOfSubKeys;

  /* Space allocated in SubKeys */
  ULONG SizeOfSubKeys;

  /* List of subkeys loaded */
  struct _KEY_OBJECT **SubKeys;
} KEY_OBJECT, *PKEY_OBJECT;

/* Bits 31-22 (top 10 bits) of the cell index is the directory index */
#define CmiDirectoryIndex(CellIndex)(CellIndex & 0xffc000000)
/* Bits 21-12 (middle 10 bits) of the cell index is the table index */
#define CmiTableIndex(Cellndex)(CellIndex & 0x003ff000)
/* Bits 11-0 (bottom 12 bits) of the cell index is the byte offset */
#define CmiByteOffset(Cellndex)(CellIndex & 0x00000fff)


extern BOOLEAN CmiDoVerify;
extern PREGISTRY_HIVE CmiVolatileHive;
extern POBJECT_TYPE CmiKeyType;
extern KSPIN_LOCK CmiKeyListLock;


VOID
CmiVerifyBinCell(PHBIN BinCell);
VOID
CmiVerifyKeyCell(PKEY_CELL KeyCell);
VOID
CmiVerifyRootKeyCell(PKEY_CELL RootKeyCell);
VOID
CmiVerifyKeyObject(PKEY_OBJECT KeyObject);
VOID
CmiVerifyRegistryHive(PREGISTRY_HIVE RegistryHive);

#ifdef DBG
#define VERIFY_BIN_CELL CmiVerifyBinCell
#define VERIFY_KEY_CELL CmiVerifyKeyCell
#define VERIFY_ROOT_KEY_CELL CmiVerifyRootKeyCell
#define VERIFY_VALUE_CELL CmiVerifyValueCell
#define VERIFY_VALUE_LIST_CELL CmiVerifyValueListCell
#define VERIFY_KEY_OBJECT CmiVerifyKeyObject
#define VERIFY_REGISTRY_HIVE CmiVerifyRegistryHive
#else
#define VERIFY_BIN_CELL(x)
#define VERIFY_KEY_CELL(x)
#define VERIFY_ROOT_KEY_CELL(x)
#define VERIFY_VALUE_CELL(x)
#define VERIFY_VALUE_LIST_CELL(x)
#define VERIFY_KEY_OBJECT(x)
#define VERIFY_REGISTRY_HIVE(x)
#endif

NTSTATUS STDCALL
CmiObjectParse(IN PVOID  ParsedObject,
	       OUT PVOID  *NextObject,
	       IN PUNICODE_STRING  FullPath,
	       IN OUT PWSTR  *Path,
	       IN ULONG  Attribute);

NTSTATUS STDCALL
CmiObjectCreate(PVOID ObjectBody,
		PVOID Parent,
		PWSTR RemainingPath,
		struct _OBJECT_ATTRIBUTES* ObjectAttributes);

VOID STDCALL
CmiObjectDelete(PVOID  DeletedObject);

VOID
CmiAddKeyToList(PKEY_OBJECT ParentKey,
  IN PKEY_OBJECT  NewKey);

NTSTATUS
CmiRemoveKeyFromList(IN PKEY_OBJECT  NewKey);

PKEY_OBJECT  CmiScanKeyList(IN PKEY_OBJECT Parent,
  IN PCHAR  KeyNameBuf,
  IN ULONG  Attributes);

NTSTATUS
CmiCreateRegistryHive(PWSTR Filename,
  PREGISTRY_HIVE *RegistryHive,
  BOOLEAN CreateNew);

ULONG
CmiGetMaxNameLength(IN PREGISTRY_HIVE  RegistryHive,
  IN PKEY_CELL  KeyCell);

ULONG
CmiGetMaxClassLength(IN PREGISTRY_HIVE  RegistryHive,
  IN PKEY_CELL  KeyCell);

ULONG
CmiGetMaxValueNameLength(IN PREGISTRY_HIVE  RegistryHive,
  IN PKEY_CELL  KeyCell);

ULONG
CmiGetMaxValueDataLength(IN PREGISTRY_HIVE  RegistryHive,
  IN PKEY_CELL  KeyCell);

NTSTATUS
CmiScanForSubKey(IN PREGISTRY_HIVE  RegistryHive,
  IN PKEY_CELL  KeyCell,
  OUT PKEY_CELL  *SubKeyCell,
  OUT BLOCK_OFFSET *BlockOffset,
  IN PCHAR  KeyName,
  IN ACCESS_MASK  DesiredAccess,
  IN ULONG Attributes);

NTSTATUS
CmiAddSubKey(IN PREGISTRY_HIVE  RegistryHive,
  IN PKEY_OBJECT Parent,
  OUT PKEY_OBJECT SubKey,
  IN PWSTR  NewSubKeyName,
  IN USHORT  NewSubKeyNameSize,
  IN ULONG  TitleIndex,
  IN PUNICODE_STRING  Class,
  IN ULONG  CreateOptions);

NTSTATUS
CmiScanKeyForValue(IN PREGISTRY_HIVE  RegistryHive,
  IN PKEY_CELL  KeyCell,
  IN PCHAR  ValueName,
  OUT PVALUE_CELL  *ValueCell,
  OUT BLOCK_OFFSET *VBOffset);

NTSTATUS
CmiGetValueFromKeyByIndex(IN PREGISTRY_HIVE  RegistryHive,
  IN PKEY_CELL  KeyCell,
  IN ULONG  Index,
  OUT PVALUE_CELL  *ValueCell);

NTSTATUS
CmiAddValueToKey(IN PREGISTRY_HIVE  RegistryHive,
  IN PKEY_CELL  KeyCell,
  IN PCHAR  ValueNameBuf,
	OUT PVALUE_CELL *pValueCell,
	OUT BLOCK_OFFSET *pVBOffset);

NTSTATUS
CmiDeleteValueFromKey(IN PREGISTRY_HIVE  RegistryHive,
  IN PKEY_CELL  KeyCell,
  IN PCHAR  ValueName);

NTSTATUS
CmiAllocateHashTableBlock(IN PREGISTRY_HIVE  RegistryHive,
  OUT PHASH_TABLE_CELL  *HashBlock,
  OUT BLOCK_OFFSET  *HBOffset,
  IN ULONG  HashTableSize);

PKEY_CELL
CmiGetKeyFromHashByIndex(PREGISTRY_HIVE RegistryHive,
PHASH_TABLE_CELL  HashBlock,
ULONG  Index);

NTSTATUS
CmiAddKeyToHashTable(PREGISTRY_HIVE  RegistryHive,
  PHASH_TABLE_CELL  HashBlock,
  PKEY_CELL  NewKeyCell,
  BLOCK_OFFSET  NKBOffset);

NTSTATUS
CmiAllocateValueCell(IN PREGISTRY_HIVE  RegistryHive,
 OUT PVALUE_CELL  *ValueCell,
 OUT BLOCK_OFFSET  *VBOffset,
 IN PCHAR  ValueNameBuf);

NTSTATUS
CmiDestroyValueCell(PREGISTRY_HIVE  RegistryHive,
  PVALUE_CELL  ValueCell,
  BLOCK_OFFSET  VBOffset);

NTSTATUS
CmiAllocateBlock(PREGISTRY_HIVE  RegistryHive,
  PVOID  *Block,
  LONG  BlockSize,
	BLOCK_OFFSET * pBlockOffset);

NTSTATUS
CmiDestroyBlock(PREGISTRY_HIVE  RegistryHive,
  PVOID  Block,
  BLOCK_OFFSET Offset);

PVOID
CmiGetBlock(PREGISTRY_HIVE  RegistryHive,
  BLOCK_OFFSET  BlockOffset,
	OUT PHBIN * ppBin);

VOID
CmiLockBlock(PREGISTRY_HIVE  RegistryHive,
  PVOID  Block);

VOID
CmiReleaseBlock(PREGISTRY_HIVE  RegistryHive,
  PVOID  Block);

NTSTATUS
CmiAddFree(PREGISTRY_HIVE  RegistryHive,
	PCELL_HEADER FreeBlock,
  BLOCK_OFFSET FreeOffset);

NTSTATUS
CmiInitHives(BOOLEAN SetUpBoot);

NTSTATUS STDCALL
CmiObjectParse(PVOID ParsedObject,
  PVOID *NextObject,
  PUNICODE_STRING FullPath,
  PWSTR *Path,
  ULONG Attributes);

#endif /*__INCLUDE_CM_H*/
