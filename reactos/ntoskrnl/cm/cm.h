#ifndef __INCLUDE_CM_H
#define __INCLUDE_CM_H

#include <cmlib.h>

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
#define  REG_SYSTEM_KEY_NAME		L"\\Registry\\Machine\\SYSTEM"
#define  REG_SOFTWARE_KEY_NAME		L"\\Registry\\Machine\\SOFTWARE"
#define  REG_SAM_KEY_NAME		L"\\Registry\\Machine\\SAM"
#define  REG_SEC_KEY_NAME		L"\\Registry\\Machine\\SECURITY"
#define  REG_USER_KEY_NAME		L"\\Registry\\User"
#define  REG_DEFAULT_USER_KEY_NAME	L"\\Registry\\User\\.Default"
#define  REG_CURRENT_USER_KEY_NAME	L"\\Registry\\User\\CurrentUser"

#define  SYSTEM_REG_FILE		L"\\SystemRoot\\System32\\Config\\SYSTEM"
#define  SYSTEM_LOG_FILE		L"\\SystemRoot\\System32\\Config\\SYSTEM.log"
#define  SOFTWARE_REG_FILE		L"\\SystemRoot\\System32\\Config\\SOFTWARE"
#define  DEFAULT_USER_REG_FILE		L"\\SystemRoot\\System32\\Config\\DEFAULT"
#define  SAM_REG_FILE			L"\\SystemRoot\\System32\\Config\\SAM"
#define  SEC_REG_FILE			L"\\SystemRoot\\System32\\Config\\SECURITY"

#define  REG_SYSTEM_FILE_NAME		L"\\system"
#define  REG_SOFTWARE_FILE_NAME		L"\\software"
#define  REG_DEFAULT_USER_FILE_NAME	L"\\default"
#define  REG_SAM_FILE_NAME		L"\\sam"
#define  REG_SEC_FILE_NAME		L"\\security"

/* When set, the hive is not backed by a file.
   Therefore, it can not be flushed to disk. */
#define HIVE_NO_FILE    0x00000002

/* When set, a modified (dirty) hive is not synchronized automatically.
   Explicit synchronization (save/flush) works. */
#define HIVE_NO_SYNCH   0x00000004

#define IsNoFileHive(Hive)  ((Hive)->Flags & HIVE_NO_FILE)
#define IsNoSynchHive(Hive)  ((Hive)->Flags & HIVE_NO_SYNCH)

/* KEY_OBJECT.Flags */

/* When set, the key is scheduled for deletion, and all
   attempts to access the key must not succeed */
#define KO_MARKED_FOR_DELETE              0x00000001

/* Type defining the Object Manager Key Object */
typedef struct _KEY_OBJECT
{
  /* Fields used by the Object Manager */
  CSHORT Type;
  CSHORT Size;

  /* Key flags */
  ULONG Flags;

  /* Key name */
  UNICODE_STRING Name;

  /* Registry hive the key belongs to */
  PEREGISTRY_HIVE RegistryHive;

  /* Block offset of the key cell this key belongs in */
  HCELL_INDEX KeyCellOffset;

  /* CM_KEY_NODE this key belong in */
  PCM_KEY_NODE KeyCell;

  /* Link to the parent KEY_OBJECT for this key */
  struct _KEY_OBJECT *ParentKey;

  /* List entry into the global key object list */
  LIST_ENTRY ListEntry;

  /* Subkeys loaded in SubKeys */
  ULONG SubKeyCounts;

  /* Space allocated in SubKeys */
  ULONG SizeOfSubKeys;

  /* List of subkeys loaded */
  struct _KEY_OBJECT **SubKeys;

  /* Time stamp for the last access by the parse routine */
  ULONG TimeStamp;

  /* List entry for connected hives */
  LIST_ENTRY HiveList;
} KEY_OBJECT, *PKEY_OBJECT;

/* Bits 31-22 (top 10 bits) of the cell index is the directory index */
#define CmiDirectoryIndex(CellIndex)(CellIndex & 0xffc000000)
/* Bits 21-12 (middle 10 bits) of the cell index is the table index */
#define CmiTableIndex(Cellndex)(CellIndex & 0x003ff000)
/* Bits 11-0 (bottom 12 bits) of the cell index is the byte offset */
#define CmiByteOffset(Cellndex)(CellIndex & 0x00000fff)


extern PEREGISTRY_HIVE CmiVolatileHive;
extern POBJECT_TYPE CmpKeyObjectType;
extern KSPIN_LOCK CmiKeyListLock;

extern LIST_ENTRY CmpHiveListHead;

extern ERESOURCE CmpRegistryLock;
extern EX_PUSH_LOCK CmpHiveListHeadLock;

/* Registry Callback Function */
typedef struct _REGISTRY_CALLBACK
{
    LIST_ENTRY ListEntry;
    EX_RUNDOWN_REF RundownRef;
    PEX_CALLBACK_FUNCTION Function;
    PVOID Context;
    LARGE_INTEGER Cookie;
    BOOLEAN PendingDelete;
} REGISTRY_CALLBACK, *PREGISTRY_CALLBACK;

NTSTATUS
CmiCallRegisteredCallbacks(IN REG_NOTIFY_CLASS Argument1,
                           IN PVOID Argument2);

#define VERIFY_BIN_HEADER(x) ASSERT(x->HeaderId == REG_BIN_ID)
#define VERIFY_KEY_CELL(x) ASSERT(x->Id == REG_KEY_CELL_ID)
#define VERIFY_ROOT_KEY_CELL(x) ASSERT(x->Id == REG_KEY_CELL_ID)
#define VERIFY_VALUE_CELL(x) ASSERT(x->Id == REG_VALUE_CELL_ID)
#define VERIFY_VALUE_LIST_CELL(x)
#define VERIFY_KEY_OBJECT(x)
#define VERIFY_REGISTRY_HIVE(x)

NTSTATUS STDCALL
CmRegisterCallback(IN PEX_CALLBACK_FUNCTION Function,
                   IN PVOID                 Context,
                   IN OUT PLARGE_INTEGER    Cookie
                    );

NTSTATUS STDCALL
CmUnRegisterCallback(IN LARGE_INTEGER    Cookie);

NTSTATUS STDCALL
CmpParseKey(IN PVOID ParsedObject,
               IN PVOID ObjectType,
               IN OUT PACCESS_STATE AccessState,
               IN KPROCESSOR_MODE AccessMode,
               IN ULONG Attributes,
               IN OUT PUNICODE_STRING FullPath,
               IN OUT PUNICODE_STRING RemainingName,
               IN OUT PVOID Context OPTIONAL,
               IN PSECURITY_QUALITY_OF_SERVICE SecurityQos OPTIONAL,
               OUT PVOID *NextObject);

VOID STDCALL
CmpDeleteKeyObject(PVOID  DeletedObject);

NTSTATUS STDCALL
CmpSecurityMethod(PVOID ObjectBody,
		  SECURITY_OPERATION_CODE OperationCode,
		  PSECURITY_INFORMATION SecurityInformation,
		  PSECURITY_DESCRIPTOR SecurityDescriptor,
		  PULONG BufferLength,
		  PSECURITY_DESCRIPTOR *OldSecurityDescriptor,
		  POOL_TYPE PoolType,
		  PGENERIC_MAPPING GenericMapping);

NTSTATUS STDCALL
CmpQueryKeyName (PVOID ObjectBody,
                    IN BOOLEAN HasObjectName,
		    POBJECT_NAME_INFORMATION ObjectNameInfo,
		    ULONG Length,
		    PULONG ReturnLength,
            IN KPROCESSOR_MODE PreviousMode);

VOID
CmiAddKeyToList(IN PKEY_OBJECT ParentKey,
		IN PKEY_OBJECT NewKey);

NTSTATUS
CmiRemoveKeyFromList(IN PKEY_OBJECT NewKey);

NTSTATUS
CmiScanKeyList(IN PKEY_OBJECT Parent,
	       IN PUNICODE_STRING KeyName,
	       IN ULONG Attributes,
	       PKEY_OBJECT* ReturnedObject);

NTSTATUS
CmiCreateVolatileHive(PEREGISTRY_HIVE *RegistryHive);

NTSTATUS
CmiLoadHive(POBJECT_ATTRIBUTES KeyObjectAttributes,
	    PUNICODE_STRING FileName,
	    ULONG Flags);

NTSTATUS
CmiRemoveRegistryHive(PEREGISTRY_HIVE RegistryHive);

NTSTATUS
CmiFlushRegistryHive(PEREGISTRY_HIVE RegistryHive);

ULONG
CmiGetNumberOfSubKeys(PKEY_OBJECT KeyObject);

ULONG
CmiGetMaxNameLength(IN PKEY_OBJECT KeyObject);

ULONG
CmiGetMaxClassLength(IN PKEY_OBJECT KeyObject);

ULONG
CmiGetMaxValueNameLength(IN PEREGISTRY_HIVE RegistryHive,
			 IN PCM_KEY_NODE KeyCell);

ULONG
CmiGetMaxValueDataLength(IN PEREGISTRY_HIVE RegistryHive,
			 IN PCM_KEY_NODE KeyCell);

NTSTATUS
CmiScanForSubKey(IN PEREGISTRY_HIVE RegistryHive,
		 IN PCM_KEY_NODE KeyCell,
		 OUT PCM_KEY_NODE *SubKeyCell,
		 OUT HCELL_INDEX *BlockOffset,
		 IN PUNICODE_STRING KeyName,
		 IN ACCESS_MASK DesiredAccess,
		 IN ULONG Attributes);

NTSTATUS
CmiAddSubKey(IN PEREGISTRY_HIVE RegistryHive,
	     IN PKEY_OBJECT ParentKey,
	     OUT PKEY_OBJECT SubKey,
	     IN PUNICODE_STRING SubKeyName,
	     IN ULONG TitleIndex,
	     IN PUNICODE_STRING Class,
	     IN ULONG CreateOptions);

NTSTATUS
CmiRemoveSubKey(IN PEREGISTRY_HIVE RegistryHive,
		IN PKEY_OBJECT Parent,
		IN PKEY_OBJECT SubKey);

NTSTATUS
CmiScanKeyForValue(IN PEREGISTRY_HIVE RegistryHive,
		   IN PCM_KEY_NODE KeyCell,
		   IN PUNICODE_STRING ValueName,
		   OUT PCM_KEY_VALUE *ValueCell,
		   OUT HCELL_INDEX *VBOffset);

NTSTATUS
CmiGetValueFromKeyByIndex(IN PEREGISTRY_HIVE RegistryHive,
			  IN PCM_KEY_NODE KeyCell,
			  IN ULONG Index,
			  OUT PCM_KEY_VALUE *ValueCell);

NTSTATUS
CmiAddValueToKey(IN PEREGISTRY_HIVE RegistryHive,
		 IN PCM_KEY_NODE KeyCell,
		 IN HCELL_INDEX KeyCellOffset,
		 IN PUNICODE_STRING ValueName,
		 OUT PCM_KEY_VALUE *pValueCell,
		 OUT HCELL_INDEX *pValueCellOffset);

NTSTATUS
CmiDeleteValueFromKey(IN PEREGISTRY_HIVE RegistryHive,
		      IN PCM_KEY_NODE KeyCell,
		      IN HCELL_INDEX KeyCellOffset,
		      IN PUNICODE_STRING ValueName);

NTSTATUS
CmiAllocateHashTableCell(IN PEREGISTRY_HIVE RegistryHive,
			 OUT PHASH_TABLE_CELL *HashBlock,
			 OUT HCELL_INDEX *HBOffset,
			 IN ULONG HashTableSize,
			 IN HV_STORAGE_TYPE Storage);

PCM_KEY_NODE
CmiGetKeyFromHashByIndex(PEREGISTRY_HIVE RegistryHive,
			 PHASH_TABLE_CELL HashBlock,
			 ULONG Index);

NTSTATUS
CmiAddKeyToHashTable(PEREGISTRY_HIVE RegistryHive,
		     PHASH_TABLE_CELL HashCell,
		     PCM_KEY_NODE KeyCell,
		     HV_STORAGE_TYPE StorageType,
		     PCM_KEY_NODE NewKeyCell,
		     HCELL_INDEX NKBOffset);

NTSTATUS
CmiRemoveKeyFromHashTable(PEREGISTRY_HIVE RegistryHive,
			  PHASH_TABLE_CELL HashBlock,
			  HCELL_INDEX NKBOffset);

NTSTATUS
CmiAllocateValueCell(IN PEREGISTRY_HIVE RegistryHive,
		     OUT PCM_KEY_VALUE *ValueCell,
		     OUT HCELL_INDEX *VBOffset,
		     IN PUNICODE_STRING ValueName,
		     IN HV_STORAGE_TYPE Storage);

NTSTATUS
CmiDestroyValueCell(PEREGISTRY_HIVE RegistryHive,
		    PCM_KEY_VALUE ValueCell,
		    HCELL_INDEX VBOffset);

NTSTATUS
CmiConnectHive(POBJECT_ATTRIBUTES KeyObjectAttributes,
	       PEREGISTRY_HIVE RegistryHive);

NTSTATUS
CmiDisconnectHive (POBJECT_ATTRIBUTES KeyObjectAttributes,
		   PEREGISTRY_HIVE *RegistryHive);

NTSTATUS
CmiInitHives(BOOLEAN SetupBoot);

ULONG
CmiGetPackedNameLength(IN PUNICODE_STRING Name,
		       OUT PBOOLEAN Packable);

BOOLEAN
CmiComparePackedNames(IN PUNICODE_STRING Name,
		      IN PUCHAR NameBuffer,
		      IN USHORT NameBufferSize,
		      IN BOOLEAN NamePacked);

HCELL_INDEX
NTAPI
CmpFindValueByName(
    IN PHHIVE Hive,
    IN PCM_KEY_NODE KeyNode,
    IN PUNICODE_STRING Name
);

HCELL_INDEX
NTAPI
CmpFindSubKeyByName(
    IN PHHIVE Hive,
    IN PCM_KEY_NODE Parent,
    IN PUNICODE_STRING SearchName
);

VOID
CmiCopyPackedName(PWCHAR NameBuffer,
		  PUCHAR PackedNameBuffer,
		  ULONG PackedNameSize);

BOOLEAN
CmiCompareHash(PUNICODE_STRING KeyName,
	       PCHAR HashString);

BOOLEAN
CmiCompareHashI(PUNICODE_STRING KeyName,
		PCHAR HashString);

BOOLEAN
CmiCompareKeyNames(PUNICODE_STRING KeyName,
		   PCM_KEY_NODE KeyCell);

BOOLEAN
CmiCompareKeyNamesI(PUNICODE_STRING KeyName,
		    PCM_KEY_NODE KeyCell);


VOID
CmiSyncHives(VOID);


NTSTATUS
CmiCreateTempHive(PEREGISTRY_HIVE *RegistryHive);

NTSTATUS
CmiSaveTempHive (PEREGISTRY_HIVE Hive,
		 HANDLE FileHandle);

NTSTATUS
NTAPI
CmFindObject(
    POBJECT_CREATE_INFORMATION ObjectCreateInfo,
    PUNICODE_STRING ObjectName,
    PVOID* ReturnedObject,
    PUNICODE_STRING RemainingPath,
    POBJECT_TYPE ObjectType,
    IN PACCESS_STATE AccessState,
    IN PVOID ParseContext
);
PVOID CMAPI
CmpAllocate(
   ULONG Size,
   BOOLEAN Paged);

VOID CMAPI
CmpFree(
   PVOID Ptr);

BOOLEAN CMAPI
CmpFileRead(
   PHHIVE RegistryHive,
   ULONG FileType,
   ULONGLONG FileOffset,
   PVOID Buffer,
   SIZE_T BufferLength);

BOOLEAN CMAPI
CmpFileWrite(
   PHHIVE RegistryHive,
   ULONG FileType,
   ULONGLONG FileOffset,
   PVOID Buffer,
   SIZE_T BufferLength);

BOOLEAN CMAPI
CmpFileSetSize(
   PHHIVE RegistryHive,
   ULONG FileType,
   ULONGLONG FileSize);

BOOLEAN CMAPI
CmpFileFlush(
   PHHIVE RegistryHive,
   ULONG FileType);

VOID
CmiCheckKey(BOOLEAN Verbose,
  HANDLE Key);

NTSTATUS
NTAPI
CmpOpenHiveFiles(IN PUNICODE_STRING BaseName,
                 IN PWCHAR Extension OPTIONAL,
                 IN PHANDLE Primary,
                 IN PHANDLE Log,
                 IN PULONG PrimaryDisposition,
                 IN PULONG LogDisposition,
                 IN BOOLEAN CreateAllowed,
                 IN BOOLEAN MarkAsSystemHive,
                 IN BOOLEAN NoBuffering,
                 OUT PULONG ClusterSize OPTIONAL);

#if 0
static __inline PVOID xHvGetCell(char *file, int line, PHHIVE Hive, HCELL_INDEX Cell)
{
   DPRINT1("xHvGetCell @ %s:%d %x @ %x\n", file, line, Cell, Hive);
   return HvGetCell(Hive, Cell);
}

static __inline VOID xHvFreeCell(char *file, int line, PHHIVE Hive, HCELL_INDEX Cell)
{
   DPRINT1("xHvFreeCell @ %s:%d %x @ %x\n", file, line, Cell, Hive);
   HvFreeCell(Hive, Cell);
}

static __inline HCELL_INDEX xHvAllocateCell(char *file, int line, PHHIVE Hive, SIZE_T Size)
{
   HCELL_INDEX Offset = HvAllocateCell(Hive, Size);
   DPRINT1("xHvAllocateCell @ %s:%d (%x) %x @ %x\n", file, line, Size, Offset, Hive);
   return Offset;
}

#define HvGetCell(hive, cell) xHvGetCell(__FILE__, __LINE__, hive, cell)
#define HvFreeCell(hive, cell) xHvFreeCell(__FILE__, __LINE__, hive, cell)
#define HvAllocateCell(hive, size) xHvAllocateCell(__FILE__, __LINE__, hive, size)
#endif

// Some Ob definitions for debug messages in Cm
#define ObGetObjectPointerCount(x) OBJECT_TO_OBJECT_HEADER(x)->PointerCount
#define ObGetObjectHandleCount(x) OBJECT_TO_OBJECT_HEADER(x)->HandleCount

//
// System Control Vector
//
typedef struct _CM_SYSTEM_CONTROL_VECTOR
{
    PWCHAR KeyPath;
    PWCHAR ValueName;
    PVOID Buffer;
    PULONG BufferLength;
    PULONG Type;
} CM_SYSTEM_CONTROL_VECTOR, *PCM_SYSTEM_CONTROL_VECTOR;

VOID
NTAPI
CmGetSystemControlValues(
    IN PVOID SystemHiveData,
    IN PCM_SYSTEM_CONTROL_VECTOR ControlVector
);

extern CM_SYSTEM_CONTROL_VECTOR CmControlVector[];
#endif /*__INCLUDE_CM_H*/
