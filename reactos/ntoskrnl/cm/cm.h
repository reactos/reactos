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

//
// Cached Child List
//
typedef struct _CACHED_CHILD_LIST
{
    ULONG Count;
    union
    {
        ULONG ValueList;
        //struct _CM_KEY_CONTROL_BLOCK *RealKcb;
        struct _KEY_OBJECT *RealKcb;
    };
} CACHED_CHILD_LIST, *PCACHED_CHILD_LIST;

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

  CACHED_CHILD_LIST ValueCache;
} KEY_OBJECT, *PKEY_OBJECT;

//
// Key Control Block (KCB) for old Cm (just so it can talk to New CM)
//
typedef struct _CM_KEY_CONTROL_BLOCK
{
    USHORT RefCount;
    USHORT Flags;
    ULONG ExtFlags:8;
    ULONG PrivateAlloc:1;
    ULONG Delete:1;
    ULONG DelayedCloseIndex:12;
    ULONG TotalLevels:10;
    union
    {
        //CM_KEY_HASH KeyHash;
        struct
        {
            ULONG ConvKey;
            PVOID NextHash;
            PHHIVE KeyHive;
            HCELL_INDEX KeyCell;
        };
    };
    struct _CM_KEY_CONTROL_BLOCK *ParentKcb;
    PVOID NameBlock;
    PVOID CachedSecurity;
    CACHED_CHILD_LIST ValueCache;
    PVOID IndexHint;
    ULONG HashKey;
    ULONG SubKeyCount;
    union
    {
        LIST_ENTRY KeyBodyListHead;
        LIST_ENTRY FreeListEntry;
    };
    PVOID KeyBodyArray[4];
    PVOID DelayCloseEntry;
    LARGE_INTEGER KcbLastWriteTime;
    USHORT KcbMaxNameLen;
    USHORT KcbMaxValueNameLen;
    ULONG KcbMaxValueDataLen;
    ULONG InDelayClose;
} CM_KEY_CONTROL_BLOCK, *PCM_KEY_CONTROL_BLOCK;


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

VOID
CmiAddKeyToList(IN PKEY_OBJECT ParentKey,
		IN PKEY_OBJECT NewKey);

NTSTATUS
CmiScanKeyList(IN PKEY_OBJECT Parent,
	       IN PCUNICODE_STRING KeyName,
	       IN ULONG Attributes,
	       PKEY_OBJECT* ReturnedObject);

NTSTATUS
CmiLoadHive(POBJECT_ATTRIBUTES KeyObjectAttributes,
	    PCUNICODE_STRING FileName,
	    ULONG Flags);

NTSTATUS
CmiFlushRegistryHive(PEREGISTRY_HIVE RegistryHive);

ULONG
CmiGetMaxNameLength(IN PHHIVE RegistryHive, IN PCM_KEY_NODE KeyCell);

ULONG
CmiGetMaxClassLength(IN PHHIVE RegistryHive, IN PCM_KEY_NODE KeyCell);

ULONG
CmiGetMaxValueNameLength(IN PHHIVE RegistryHive,
			 IN PCM_KEY_NODE KeyCell);

ULONG
CmiGetMaxValueDataLength(IN PHHIVE RegistryHive,
			 IN PCM_KEY_NODE KeyCell);

NTSTATUS
CmiScanForSubKey(IN PEREGISTRY_HIVE RegistryHive,
		 IN PCM_KEY_NODE KeyCell,
		 OUT PCM_KEY_NODE *SubKeyCell,
		 OUT HCELL_INDEX *BlockOffset,
		 IN PCUNICODE_STRING KeyName,
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
CmiScanKeyForValue(IN PEREGISTRY_HIVE RegistryHive,
		   IN PCM_KEY_NODE KeyCell,
		   IN PUNICODE_STRING ValueName,
		   OUT PCM_KEY_VALUE *ValueCell,
		   OUT HCELL_INDEX *VBOffset);

NTSTATUS
NTAPI
CmDeleteValueKey(IN PCM_KEY_CONTROL_BLOCK Kcb,
                 IN UNICODE_STRING ValueName);

NTSTATUS
NTAPI
CmQueryValueKey(IN PCM_KEY_CONTROL_BLOCK Kcb,
                IN UNICODE_STRING ValueName,
                IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
                IN PVOID KeyValueInformation,
                IN ULONG Length,
                IN PULONG ResultLength);

NTSTATUS
NTAPI
CmEnumerateValueKey(IN PCM_KEY_CONTROL_BLOCK Kcb,
                    IN ULONG Index,
                    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
                    IN PVOID KeyValueInformation,
                    IN ULONG Length,
                    IN PULONG ResultLength);

NTSTATUS
NTAPI
CmSetValueKey(IN PCM_KEY_CONTROL_BLOCK Kcb,
              IN PUNICODE_STRING ValueName,
              IN ULONG Type,
              IN PVOID Data,
              IN ULONG DataSize);

NTSTATUS
NTAPI
CmQueryKey(IN PCM_KEY_CONTROL_BLOCK Kcb,
           IN KEY_INFORMATION_CLASS KeyInformationClass,
           IN PVOID KeyInformation,
           IN ULONG Length,
           IN PULONG ResultLength);

NTSTATUS
NTAPI
CmEnumerateKey(IN PCM_KEY_CONTROL_BLOCK Kcb,
               IN ULONG Index,
               IN KEY_INFORMATION_CLASS KeyInformationClass,
               IN PVOID KeyInformation,
               IN ULONG Length,
               IN PULONG ResultLength);

NTSTATUS
NTAPI
CmDeleteKey(IN PCM_KEY_CONTROL_BLOCK Kcb);

NTSTATUS
CmiAllocateHashTableCell(IN PEREGISTRY_HIVE RegistryHive,
			 OUT PHASH_TABLE_CELL *HashBlock,
			 OUT HCELL_INDEX *HBOffset,
			 IN ULONG HashTableSize,
			 IN HSTORAGE_TYPE Storage);

NTSTATUS
CmiAddKeyToHashTable(PEREGISTRY_HIVE RegistryHive,
		     PHASH_TABLE_CELL HashCell,
		     PCM_KEY_NODE KeyCell,
		     HSTORAGE_TYPE StorageType,
		     PCM_KEY_NODE NewKeyCell,
		     HCELL_INDEX NKBOffset);

NTSTATUS
CmiConnectHive(POBJECT_ATTRIBUTES KeyObjectAttributes,
	       PEREGISTRY_HIVE RegistryHive);

NTSTATUS
CmiInitHives(BOOLEAN SetupBoot);

NTSTATUS
NTAPI
CmpDoCreate(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell,
    IN PACCESS_STATE AccessState,
    IN PUNICODE_STRING Name,
    IN KPROCESSOR_MODE AccessMode,
    IN PUNICODE_STRING Class,
    IN ULONG CreateOptions,
    IN PKEY_OBJECT Parent,
    IN PVOID OriginatingHive OPTIONAL,
    OUT PVOID *Object
);

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
    IN PCUNICODE_STRING SearchName
);

VOID
CmiSyncHives(VOID);

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

NTSTATUS
NTAPI
CmpOpenHiveFiles(IN PCUNICODE_STRING BaseName,
                 IN PCWSTR Extension OPTIONAL,
                 IN PHANDLE Primary,
                 IN PHANDLE Log,
                 IN PULONG PrimaryDisposition,
                 IN PULONG LogDisposition,
                 IN BOOLEAN CreateAllowed,
                 IN BOOLEAN MarkAsSystemHive,
                 IN BOOLEAN NoBuffering,
                 OUT PULONG ClusterSize OPTIONAL);

NTSTATUS
NTAPI
CmpInitHiveFromFile(IN PCUNICODE_STRING HiveName,
                    IN ULONG HiveFlags,
                    OUT PEREGISTRY_HIVE *Hive,
                    IN OUT PBOOLEAN New,
                    IN ULONG CheckFlags);

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
