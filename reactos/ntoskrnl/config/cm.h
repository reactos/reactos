/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/cm/cm.h
 * PURPOSE:         Internal header for the Configuration Manager
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */
#define _CM_
#include "cmlib.h"

//
// Define this if you want debugging support
//
#define _CM_DEBUG_                                      0x00

//
// These define the Debug Masks Supported
//
#define CM_HANDLE_DEBUG                                 0x01
#define CM_NAMESPACE_DEBUG                              0x02
#define CM_SECURITY_DEBUG                               0x04
#define CM_REFERENCE_DEBUG                              0x08
#define CM_CALLBACK_DEBUG                               0x10

//
// Debug/Tracing support
//
#if _CM_DEBUG_
#ifdef NEW_DEBUG_SYSTEM_IMPLEMENTED // enable when Debug Filters are implemented
#define CMTRACE DbgPrintEx
#else
#define CMTRACE(x, ...)                                 \
    if (x & CmpTraceLevel) DbgPrint(__VA_ARGS__)
#endif
#else
#define CMTRACE(x, ...) DPRINT(__VA_ARGS__)
#endif


//
// Hack since bigkeys are not yet supported
//
#define ASSERT_VALUE_BIG(h, s)                          \
    ASSERTMSG("Big keys not supported!", !CmpIsKeyValueBig(h, s));

//
// CM_KEY_CONTROL_BLOCK Flags
//
#define CM_KCB_NO_SUBKEY                                0x01
#define CM_KCB_SUBKEY_ONE                               0x02
#define CM_KCB_SUBKEY_HINT                              0x04
#define CM_KCB_SYM_LINK_FOUND                           0x08
#define CM_KCB_KEY_NON_EXIST                            0x10
#define CM_KCB_NO_DELAY_CLOSE                           0x20
#define CM_KCB_INVALID_CACHED_INFO                      0x40
#define CM_KEY_READ_ONLY_KEY                            0x80

//
// CM_KEY_VALUE Types
//
#define CM_KEY_VALUE_SMALL                              0x4
#define CM_KEY_VALUE_BIG                                0x3FD8
#define CM_KEY_VALUE_SPECIAL_SIZE                       0x80000000

//
// Number of various lists and hashes
//
#define CMP_SECURITY_HASH_LISTS                         64
#define CMP_MAX_CALLBACKS                               100

//
// Hashing Constants
//
#define CMP_HASH_IRRATIONAL                             314159269
#define CMP_HASH_PRIME                                  1000000007

//
// CmpCreateKeyControlBlock Flags
//
#define CMP_CREATE_FAKE_KCB                             0x1
#define CMP_LOCK_HASHES_FOR_KCB                         0x2

//
// Maximum size of Value Cache
//
#define MAXIMUM_CACHED_DATA                             2 * PAGE_SIZE

//
// Hives to load on startup
//
#define CM_NUMBER_OF_MACHINE_HIVES                      6

//
// Number of items that can fit inside an Allocation Page
//
#define CM_KCBS_PER_PAGE                                \
    PAGE_SIZE / sizeof(CM_KEY_CONTROL_BLOCK)
#define CM_DELAYS_PER_PAGE                              \
    PAGE_SIZE / sizeof(CM_DELAYED_CLOSE_ENTRY)

//
// Value Search Results
//
typedef enum _VALUE_SEARCH_RETURN_TYPE
{
    SearchSuccess,
    SearchNeedExclusiveLock,
    SearchFail
} VALUE_SEARCH_RETURN_TYPE;

//
// Key Hash
//
typedef struct _CM_KEY_HASH
{
    ULONG ConvKey;
    struct _CM_KEY_HASH *NextHash;
    PHHIVE KeyHive;
    HCELL_INDEX KeyCell;
} CM_KEY_HASH, *PCM_KEY_HASH;

//
// Key Hash Table Entry
//
typedef struct _CM_KEY_HASH_TABLE_ENTRY
{
    EX_PUSH_LOCK Lock;
    PKTHREAD Owner;
    PCM_KEY_HASH Entry;
} CM_KEY_HASH_TABLE_ENTRY, *PCM_KEY_HASH_TABLE_ENTRY;

//
// Name Hash
//
typedef struct _CM_NAME_HASH
{
    ULONG ConvKey;
    struct _CM_NAME_HASH *NextHash;
    USHORT NameLength;
    WCHAR Name[ANYSIZE_ARRAY];
} CM_NAME_HASH, *PCM_NAME_HASH;

//
// Name Hash Table Entry
//
typedef struct _CM_NAME_HASH_TABLE_ENTRY
{
    EX_PUSH_LOCK Lock;
    PCM_NAME_HASH Entry;
} CM_NAME_HASH_TABLE_ENTRY, *PCM_NAME_HASH_TABLE_ENTRY;

//
// Key Security Cache
//
typedef struct _CM_KEY_SECURITY_CACHE
{
    HCELL_INDEX Cell;
    ULONG ConvKey;
    LIST_ENTRY List;
    ULONG DescriptorLength;
    SECURITY_DESCRIPTOR_RELATIVE Descriptor;
} CM_KEY_SECURITY_CACHE, *PCM_KEY_SECURITY_CACHE;

//
// Key Security Cache Entry
//
typedef struct _CM_KEY_SECURITY_CACHE_ENTRY
{
    HCELL_INDEX Cell;
    PCM_KEY_SECURITY_CACHE CachedSecurity;
} CM_KEY_SECURITY_CACHE_ENTRY, *PCM_KEY_SECURITY_CACHE_ENTRY;

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

//
// Index Hint Block
//
typedef struct _CM_INDEX_HINT_BLOCK
{
    ULONG Count;
    ULONG HashKey[ANYSIZE_ARRAY];
} CM_INDEX_HINT_BLOCK, *PCM_INDEX_HINT_BLOCK;

//
// Key Body
//
typedef struct _CM_KEY_BODY
{
    ULONG Type;
    struct _CM_KEY_CONTROL_BLOCK *KeyControlBlock;
    struct _CM_NOTIFY_BLOCK *NotifyBlock;
    HANDLE ProcessID;
    LIST_ENTRY KeyBodyList;
} CM_KEY_BODY, *PCM_KEY_BODY;

//
// Name Control Block (NCB)
//
typedef struct _CM_NAME_CONTROL_BLOCK
{
    BOOLEAN Compressed;
    USHORT RefCount;
    union
    {
        CM_NAME_HASH NameHash;
        struct
        {
            ULONG ConvKey;
            PCM_KEY_HASH NextHash;
            USHORT NameLength;
            WCHAR Name[ANYSIZE_ARRAY];
        };
    };
} CM_NAME_CONTROL_BLOCK, *PCM_NAME_CONTROL_BLOCK;

//
// Key Control Block (KCB)
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
        CM_KEY_HASH KeyHash;
        struct
        {
            ULONG ConvKey;
            PCM_KEY_HASH NextHash;
            PHHIVE KeyHive;
            HCELL_INDEX KeyCell;
        };
    };
    struct _CM_KEY_CONTROL_BLOCK *ParentKcb;
    PCM_NAME_CONTROL_BLOCK NameBlock;
    PCM_KEY_SECURITY_CACHE CachedSecurity;
    CACHED_CHILD_LIST ValueCache;
    PCM_INDEX_HINT_BLOCK IndexHint;
    ULONG HashKey;
    ULONG SubKeyCount;
    union
    {
        LIST_ENTRY KeyBodyListHead;
        LIST_ENTRY FreeListEntry;
    };
    PCM_KEY_BODY KeyBodyArray[4];
    PVOID DelayCloseEntry;
    LARGE_INTEGER KcbLastWriteTime;
    USHORT KcbMaxNameLen;
    USHORT KcbMaxValueNameLen;
    ULONG KcbMaxValueDataLen;
    ULONG InDelayClose;
} CM_KEY_CONTROL_BLOCK, *PCM_KEY_CONTROL_BLOCK;

//
// Notify Block
//
typedef struct _CM_NOTIFY_BLOCK
{
    LIST_ENTRY HiveList;
    LIST_ENTRY PostList;
    PCM_KEY_CONTROL_BLOCK KeyControlBlock;
    PCM_KEY_BODY KeyBody;
    ULONG Filter:29;
    ULONG WatchTree:30;
    ULONG NotifyPending:31;
} CM_NOTIFY_BLOCK, *PCM_NOTIFY_BLOCK;

//
// Re-map Block
//
typedef struct _CM_CELL_REMAP_BLOCK
{
    HCELL_INDEX OldCell;
    HCELL_INDEX NewCell;
} CM_CELL_REMAP_BLOCK, *PCM_CELL_REMAP_BLOCK;

//
// Allocation Page
//
typedef struct _CM_ALLOC_PAGE
{
    ULONG FreeCount;
    ULONG Reserved;
    PVOID AllocPage;
} CM_ALLOC_PAGE, *PCM_ALLOC_PAGE;

//
// Allocation Page Entry
//
typedef struct _CM_DELAY_ALLOC
{
    LIST_ENTRY ListEntry;
    PCM_KEY_CONTROL_BLOCK Kcb;
} CM_DELAY_ALLOC, *PCM_DELAY_ALLOC;

//
// Delayed Close Entry
//
typedef struct _CM_DELAYED_CLOSE_ENTRY
{
    LIST_ENTRY DelayedLRUList;
    PCM_KEY_CONTROL_BLOCK KeyControlBlock;
} CM_DELAYED_CLOSE_ENTRY, *PCM_DELAYED_CLOSE_ENTRY;

//
// Delayed KCB Dereference Entry
//
typedef struct _CM_DELAY_DEREF_KCB_ITEM
{
    LIST_ENTRY ListEntry;
    PCM_KEY_CONTROL_BLOCK Kcb;
} CM_DELAY_DEREF_KCB_ITEM, *PCM_DELAY_DEREF_KCB_ITEM;

//
// Use Count Log and Entry
//
typedef struct _CM_USE_COUNT_LOG_ENTRY
{
    HCELL_INDEX Cell;
    PVOID Stack[7];
} CM_USE_COUNT_LOG_ENTRY, *PCM_USE_COUNT_LOG_ENTRY;

typedef struct _CM_USE_COUNT_LOG
{
    USHORT Next;
    USHORT Size;
    CM_USE_COUNT_LOG_ENTRY Log[32];
} CM_USE_COUNT_LOG, *PCM_USE_COUNT_LOG;

//
// Configuration Manager Hive Structure
//
typedef struct _CMHIVE
{
    HHIVE Hive;
    HANDLE FileHandles[3];
    LIST_ENTRY NotifyList;
    LIST_ENTRY HiveList;
    EX_PUSH_LOCK HiveLock;
    PKTHREAD HiveLockOwner;
    PKGUARDED_MUTEX ViewLock;
    PKTHREAD ViewLockOwner;
    EX_PUSH_LOCK WriterLock;
    PKTHREAD WriterLockOwner;
    PERESOURCE FlusherLock;
    EX_PUSH_LOCK SecurityLock;
    PKTHREAD HiveSecurityLockOwner;
    LIST_ENTRY LRUViewListHead;
    LIST_ENTRY PinViewListHead;
    PFILE_OBJECT FileObject;
    UNICODE_STRING FileFullPath;
    UNICODE_STRING FileUserName;
    USHORT MappedViews;
    USHORT PinnedViews;
    ULONG UseCount;
    ULONG SecurityCount;
    ULONG SecurityCacheSize;
    LONG SecurityHitHint;
    PCM_KEY_SECURITY_CACHE_ENTRY SecurityCache;
    LIST_ENTRY SecurityHash[CMP_SECURITY_HASH_LISTS];
    PKEVENT UnloadEvent;
    PCM_KEY_CONTROL_BLOCK RootKcb;
    BOOLEAN Frozen;
    PWORK_QUEUE_ITEM UnloadWorkItem;
    BOOLEAN GrowOnlyMode;
    ULONG GrowOffset;
    LIST_ENTRY KcbConvertListHead;
    LIST_ENTRY KnodeConvertListHead;
    PCM_CELL_REMAP_BLOCK CellRemapArray;
    CM_USE_COUNT_LOG UseCountLog;
    CM_USE_COUNT_LOG LockHiveLog;
    ULONG Flags;
    LIST_ENTRY TrustClassEntry;
    ULONG FlushCount;
    BOOLEAN HiveIsLoading;
    PKTHREAD CreatorOwner;
} CMHIVE, *PCMHIVE;

//
// Cached Value Index
//
typedef struct _CM_CACHED_VALUE_INDEX
{
    HCELL_INDEX CellIndex;
    union
    {
        CELL_DATA CellData;
        ULONG_PTR List[ANYSIZE_ARRAY];
    } Data;
} CM_CACHED_VALUE_INDEX, *PCM_CACHED_VALUE_INDEX;

//
// Cached Value
//
typedef struct _CM_CACHED_VALUE
{
    USHORT DataCacheType;
    USHORT ValueKeySize;
    ULONG HashKey;
    CM_KEY_VALUE KeyValue;
} CM_CACHED_VALUE, *PCM_CACHED_VALUE;

//
// Hive List Entry
//
typedef struct _HIVE_LIST_ENTRY
{
    PWSTR Name;
    PWSTR BaseName;
    PCMHIVE CmHive;
    ULONG HHiveFlags;
    ULONG CmHiveFlags;
    PCMHIVE CmHive2;
    BOOLEAN ThreadFinished;
    BOOLEAN ThreadStarted;
    BOOLEAN Allocate;
} HIVE_LIST_ENTRY, *PHIVE_LIST_ENTRY;

//
// Parse context for Key Object
//
typedef struct _CM_PARSE_CONTEXT
{
    ULONG TitleIndex;
    UNICODE_STRING Class;
    ULONG CreateOptions;
    ULONG Disposition;
    CM_KEY_REFERENCE ChildHive;
    HANDLE PredefinedHandle;
    BOOLEAN CreateLink;
    BOOLEAN CreateOperation;
    PCMHIVE OriginatingPoint;
} CM_PARSE_CONTEXT, *PCM_PARSE_CONTEXT;

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

//
// Structure for CmpQueryValueDataFromCache
//
typedef struct _KEY_VALUE_INFORMATION
{
    union
    {
        KEY_VALUE_BASIC_INFORMATION KeyValueBasicInformation;
        KEY_VALUE_FULL_INFORMATION KeyValueFullInformation;
        KEY_VALUE_PARTIAL_INFORMATION KeyValuePartialInformation;
        KEY_VALUE_PARTIAL_INFORMATION_ALIGN64 KeyValuePartialInformationAlign64;
    };
} KEY_VALUE_INFORMATION, *PKEY_VALUE_INFORMATION;

typedef struct _KEY_INFORMATION
{
    union
    {
        KEY_BASIC_INFORMATION KeyBasicInformation;
        KEY_FULL_INFORMATION KeyFullInformation;
        KEY_NODE_INFORMATION KeyNodeInformation;
    };
} KEY_INFORMATION, *PKEY_INFORMATION;

///////////////////////////////////////////////////////////////////////////////
//
// BUGBUG Old Hive Stuff for Temporary Support
//
typedef struct _KEY_OBJECT
{
    PCM_KEY_CONTROL_BLOCK KeyControlBlock;
    LIST_ENTRY KeyBodyEntry;
} KEY_OBJECT, *PKEY_OBJECT;
NTSTATUS
NTAPI
CmFindObject(POBJECT_CREATE_INFORMATION ObjectCreateInfo,
             PUNICODE_STRING ObjectName,
             PVOID* ReturnedObject,
             PUNICODE_STRING RemainingPath,
             POBJECT_TYPE ObjectType,
             IN PACCESS_STATE AccessState,
             IN PVOID ParseContext);
NTSTATUS CmiCallRegisteredCallbacks(IN REG_NOTIFY_CLASS Argument1, IN PVOID Argument2);
///////////////////////////////////////////////////////////////////////////////

//
// Mapped View Hive Functions
//
VOID
NTAPI
CmpInitHiveViewList(
    IN PCMHIVE Hive
);

//
// Security Cache Functions
//
VOID
NTAPI
CmpInitSecurityCache(
    IN PCMHIVE Hive
);

//
// Value Cache Functions
//
VALUE_SEARCH_RETURN_TYPE
NTAPI
CmpFindValueByNameFromCache(
    IN PCM_KEY_CONTROL_BLOCK Kcb,
    IN PCUNICODE_STRING Name,
    OUT PCM_CACHED_VALUE **CachedValue,
    OUT ULONG *Index,
    OUT PCM_KEY_VALUE *Value,
    OUT BOOLEAN *ValueIsCached,
    OUT PHCELL_INDEX CellToRelease
);

VALUE_SEARCH_RETURN_TYPE
NTAPI
CmpQueryKeyValueData(
    IN PCM_KEY_CONTROL_BLOCK Kcb,
    IN PCM_CACHED_VALUE *CachedValue,
    IN PCM_KEY_VALUE ValueKey,
    IN BOOLEAN ValueIsCached,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    IN PVOID KeyValueInformation,
    IN ULONG Length,
    OUT PULONG ResultLength,
    OUT PNTSTATUS Status
);

VALUE_SEARCH_RETURN_TYPE
NTAPI
CmpGetValueListFromCache(
    IN PCM_KEY_CONTROL_BLOCK Kcb,
    OUT PCELL_DATA *CellData,
    OUT BOOLEAN *IndexIsCached,
    OUT PHCELL_INDEX ValueListToRelease
);

VALUE_SEARCH_RETURN_TYPE
NTAPI
CmpGetValueKeyFromCache(
    IN PCM_KEY_CONTROL_BLOCK Kcb,
    IN PCELL_DATA CellData,
    IN ULONG Index,
    OUT PCM_CACHED_VALUE **CachedValue,
    OUT PCM_KEY_VALUE *Value,
    IN BOOLEAN IndexIsCached,
    OUT BOOLEAN *ValueIsCached,
    OUT PHCELL_INDEX CellToRelease
);

//
// Registry Validation Functions
//
ULONG
NTAPI
CmCheckRegistry(
    IN PCMHIVE Hive,
    IN ULONG Flags
);

//
// Notification Routines
//
VOID
NTAPI
CmpReportNotify(
    IN PCM_KEY_CONTROL_BLOCK Kcb,
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell,
    IN ULONG Filter
);

VOID
NTAPI
CmpInitCallback(
    VOID
);

//
// KCB Cache/Delay Routines
//
VOID
NTAPI
CmpInitializeCache(
    VOID
);

VOID
NTAPI
CmpInitCmPrivateDelayAlloc(
    VOID
);

VOID
NTAPI
CmpInitCmPrivateAlloc(
    VOID
);

VOID
NTAPI
CmpInitDelayDerefKCBEngine(
    VOID
);

//
// Key Object Routines
//
VOID
NTAPI
CmpCloseKeyObject(
    IN PEPROCESS Process OPTIONAL,
    IN PVOID Object,
    IN ACCESS_MASK GrantedAccess,
    IN ULONG ProcessHandleCount,
    IN ULONG SystemHandleCount
);

VOID
NTAPI
CmpDeleteKeyObject(
    IN PVOID Object
);

NTSTATUS
NTAPI
CmpParseKey(
    IN PVOID ParseObject,
    IN PVOID ObjectType,
    IN OUT PACCESS_STATE AccessState,
    IN KPROCESSOR_MODE AccessMode,
    IN ULONG Attributes,
    IN OUT PUNICODE_STRING CompleteName,
    IN OUT PUNICODE_STRING RemainingName,
    IN OUT PVOID Context OPTIONAL,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQos OPTIONAL,
    OUT PVOID *Object
);

NTSTATUS
NTAPI
CmpSecurityMethod(
    IN PVOID Object,
    IN SECURITY_OPERATION_CODE OperationType,
    IN PSECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN OUT PULONG CapturedLength,
    IN OUT PSECURITY_DESCRIPTOR *ObjectSecurityDescriptor,
    IN POOL_TYPE PoolType,
    IN PGENERIC_MAPPING GenericMapping
);

NTSTATUS
NTAPI
CmpQueryKeyName(
    IN PVOID Object,
    IN BOOLEAN HasObjectName,
    OUT POBJECT_NAME_INFORMATION ObjectNameInfo,
    IN ULONG Length,
    OUT PULONG ReturnLength,
    IN KPROCESSOR_MODE AccessMode
);

//
// Hive Routines
//
NTSTATUS
NTAPI
CmpInitializeHive(
    OUT PCMHIVE *CmHive,
    IN ULONG Operation,
    IN ULONG Flags,
    IN ULONG FileType,
    IN PVOID HiveData OPTIONAL,
    IN HANDLE Primary,
    IN HANDLE Log,
    IN HANDLE External,
    IN PCUNICODE_STRING FileName OPTIONAL,
    IN ULONG CheckFlags
);

PSECURITY_DESCRIPTOR
NTAPI
CmpHiveRootSecurityDescriptor(
    VOID
);

NTSTATUS
NTAPI
CmpLinkHiveToMaster(
    IN PUNICODE_STRING LinkName,
    IN HANDLE RootDirectory,
    IN PCMHIVE CmHive,
    IN BOOLEAN Allocate,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
);

NTSTATUS
NTAPI
CmpOpenHiveFiles(
    IN PCUNICODE_STRING BaseName,
    IN PCWSTR Extension OPTIONAL,
    IN PHANDLE Primary,
    IN PHANDLE Log,
    IN PULONG PrimaryDisposition,
    IN PULONG LogDisposition,
    IN BOOLEAN CreateAllowed,
    IN BOOLEAN MarkAsSystemHive,
    IN BOOLEAN NoBuffering,
    OUT PULONG ClusterSize OPTIONAL
);

NTSTATUS
NTAPI
CmpInitHiveFromFile(
    IN PCUNICODE_STRING HiveName,
    IN ULONG HiveFlags,
    OUT PCMHIVE *Hive,
    IN OUT PBOOLEAN New,
    IN ULONG CheckFlags
);

VOID
NTAPI
CmpInitializeHiveList(
    IN USHORT Flag
);

//
// Registry Utility Functions
//
BOOLEAN
NTAPI
CmpTestRegistryLockExclusive(
    VOID
);

BOOLEAN
NTAPI
CmpTestRegistryLock(
    VOID
);

VOID
NTAPI
CmpLockRegistryExclusive(
    VOID
);

VOID
NTAPI
CmpLockRegistry(
    VOID
);

VOID
NTAPI
CmpUnlockRegistry(
    VOID
);

//
// Delay Functions
//
PVOID
NTAPI
CmpAllocateDelayItem(
    VOID
);

VOID
NTAPI
CmpFreeDelayItem(
    PVOID Entry
);

VOID
NTAPI
CmpDelayDerefKeyControlBlock(
    IN PCM_KEY_CONTROL_BLOCK Kcb
);

VOID
NTAPI
CmpAddToDelayedClose(
    IN PCM_KEY_CONTROL_BLOCK Kcb,
    IN BOOLEAN LockHeldExclusively
);

VOID
NTAPI
CmpRemoveFromDelayedClose(IN PCM_KEY_CONTROL_BLOCK Kcb);

VOID
NTAPI
CmpInitializeDelayedCloseTable(
    VOID
);

//
// KCB Functions
//
PCM_KEY_CONTROL_BLOCK
NTAPI
CmpCreateKeyControlBlock(
    IN PHHIVE Hive,
    IN HCELL_INDEX Index,
    IN PCM_KEY_NODE Node,
    IN PCM_KEY_CONTROL_BLOCK Parent,
    IN ULONG Flags,
    IN PUNICODE_STRING KeyName
);

PCM_KEY_CONTROL_BLOCK
NTAPI
CmpAllocateKeyControlBlock(
    VOID
);

VOID
NTAPI
CmpFreeKeyControlBlock(
    IN PCM_KEY_CONTROL_BLOCK Kcb
);

VOID
NTAPI
CmpRemoveKeyControlBlock(
    IN PCM_KEY_CONTROL_BLOCK Kcb
);

VOID
NTAPI
CmpCleanUpKcbCacheWithLock(
    IN PCM_KEY_CONTROL_BLOCK Kcb,
    IN BOOLEAN LockHeldExclusively
);

VOID
NTAPI
CmpDereferenceKeyControlBlockWithLock(
    IN PCM_KEY_CONTROL_BLOCK Kcb,
    IN BOOLEAN LockHeldExclusively
);

VOID
NTAPI
CmpDereferenceKeyControlBlock(
    IN PCM_KEY_CONTROL_BLOCK Kcb
);

VOID
NTAPI
EnlistKeyBodyWithKCB(
    IN PCM_KEY_BODY KeyObject,
    IN ULONG Flags
);

VOID
NTAPI
EnlistKeyBodyWithKeyObject(
   IN PKEY_OBJECT KeyObject,
   IN ULONG Flags
);

NTSTATUS
NTAPI
CmpFreeKeyByCell(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell,
    IN BOOLEAN Unlink
);

VOID
NTAPI
CmpAcquireTwoKcbLocksExclusiveByKey(
    IN ULONG ConvKey1,
    IN ULONG ConvKey2
);

VOID
NTAPI
CmpReleaseTwoKcbLockByKey(
    IN ULONG ConvKey1,
    IN ULONG ConvKey2
);

//
// Name Functions
//
LONG
NTAPI
CmpCompareCompressedName(
    IN PCUNICODE_STRING SearchName,
    IN PWCHAR CompressedName,
    IN ULONG NameLength
);

USHORT
NTAPI
CmpNameSize(
    IN PHHIVE Hive,
    IN PUNICODE_STRING Name
);

USHORT
NTAPI
CmpCompressedNameSize(
    IN PWCHAR Name,
    IN ULONG Length
);

VOID
NTAPI
CmpCopyCompressedName(
    IN PWCHAR Destination,
    IN ULONG DestinationLength,
    IN PWCHAR Source,
    IN ULONG SourceLength
);

USHORT
NTAPI
CmpCopyName(
    IN PHHIVE Hive,
    IN PWCHAR Destination,
    IN PUNICODE_STRING Source
);

BOOLEAN
NTAPI
CmpFindNameInList(
    IN PHHIVE Hive,
    IN PCHILD_LIST ChildList,
    IN PUNICODE_STRING Name,
    IN PULONG ChildIndex,
    IN PHCELL_INDEX CellIndex
);

//
// Parse Routines
//
BOOLEAN
NTAPI
CmpGetNextName(
    IN OUT PUNICODE_STRING RemainingName,
    OUT PUNICODE_STRING NextName,
    OUT PBOOLEAN LastName
);

//
// Command Routines (Flush, Open, Close, Init);
//
BOOLEAN
NTAPI
CmpDoFlushAll(
    IN BOOLEAN ForceFlush
);

VOID
NTAPI
CmpShutdownWorkers(
    VOID
);

VOID
NTAPI
CmpCmdInit(
    IN BOOLEAN SetupBoot
);

NTSTATUS
NTAPI
CmpCmdHiveOpen(
    IN POBJECT_ATTRIBUTES FileAttributes,
    IN PSECURITY_CLIENT_CONTEXT ImpersonationContext,
    IN OUT PBOOLEAN Allocate,
    OUT PCMHIVE *NewHive,
    IN ULONG CheckFlags
);

VOID
NTAPI
CmpLazyFlush(
    VOID
);

//
// Open/Create Routines
//
NTSTATUS
NTAPI
CmpDoCreate(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell,
    IN PACCESS_STATE AccessState,
    IN PUNICODE_STRING Name,
    IN KPROCESSOR_MODE AccessMode,
    IN PCM_PARSE_CONTEXT Context,
    IN PCM_KEY_CONTROL_BLOCK ParentKcb,
    OUT PVOID *Object
);

NTSTATUS
NTAPI
CmpCreateLinkNode(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell,
    IN PACCESS_STATE AccessState,
    IN UNICODE_STRING Name,
    IN KPROCESSOR_MODE AccessMode,
    IN ULONG CreateOptions,
    IN PCM_PARSE_CONTEXT Context,
    IN PCM_KEY_CONTROL_BLOCK ParentKcb,
    OUT PVOID *Object
);

//
// Cell Index Routines
//

HCELL_INDEX
NTAPI
CmpFindSubKeyByName(
    IN PHHIVE Hive,
    IN PCM_KEY_NODE Parent,
    IN PCUNICODE_STRING SearchName
);

HCELL_INDEX
NTAPI
CmpFindSubKeyByNumber(
    IN PHHIVE Hive,
    IN PCM_KEY_NODE Node,
    IN ULONG Number
);

ULONG
NTAPI
CmpComputeHashKey(
    IN ULONG Hash,
    IN PCUNICODE_STRING Name,
    IN BOOLEAN AllowSeparators
);

BOOLEAN
NTAPI
CmpAddSubKey(
    IN PHHIVE Hive,
    IN HCELL_INDEX Parent,
    IN HCELL_INDEX Child
);

BOOLEAN
NTAPI
CmpRemoveSubKey(
    IN PHHIVE Hive,
    IN HCELL_INDEX ParentKey,
    IN HCELL_INDEX TargetKey
);

BOOLEAN
NTAPI
CmpMarkIndexDirty(
    IN PHHIVE Hive,
    HCELL_INDEX ParentKey,
    HCELL_INDEX TargetKey
);

//
// Cell Value Routines
//
HCELL_INDEX
NTAPI
CmpFindValueByName(
    IN PHHIVE Hive,
    IN PCM_KEY_NODE KeyNode,
    IN PUNICODE_STRING Name
);

PCELL_DATA
NTAPI
CmpValueToData(
    IN PHHIVE Hive,
    IN PCM_KEY_VALUE Value,
    OUT PULONG Length
);

NTSTATUS
NTAPI
CmpSetValueDataNew(
    IN PHHIVE Hive,
    IN PVOID Data,
    IN ULONG DataSize,
    IN ULONG StorageType,
    IN HCELL_INDEX ValueCell,
    OUT PHCELL_INDEX DataCell
);

NTSTATUS
NTAPI
CmpAddValueToList(
    IN PHHIVE Hive,
    IN HCELL_INDEX ValueCell,
    IN ULONG Index,
    IN ULONG Type,
    IN OUT PCHILD_LIST ChildList
);

BOOLEAN
NTAPI
CmpFreeValue(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell
);

BOOLEAN
NTAPI
CmpMarkValueDataDirty(
    IN PHHIVE Hive,
    IN PCM_KEY_VALUE Value
);

BOOLEAN
NTAPI
CmpFreeValueData(
    IN PHHIVE Hive,
    IN HCELL_INDEX DataCell,
    IN ULONG DataLength
);

NTSTATUS
NTAPI
CmpRemoveValueFromList(
    IN PHHIVE Hive,
    IN ULONG Index,
    IN OUT PCHILD_LIST ChildList
);

BOOLEAN
NTAPI
CmpGetValueData(
    IN PHHIVE Hive,
    IN PCM_KEY_VALUE Value,
    IN PULONG Length,
    OUT PVOID *Buffer,
    OUT PBOOLEAN BufferAllocated,
    OUT PHCELL_INDEX CellToRelease
);

//
// Boot Routines
//
HCELL_INDEX
NTAPI
CmpFindControlSet(
    IN PHHIVE SystemHive,
    IN HCELL_INDEX RootCell,
    IN PUNICODE_STRING SelectKeyName,
    OUT PBOOLEAN AutoSelect
);

VOID
NTAPI
CmGetSystemControlValues(
    IN PVOID SystemHiveData,
    IN PCM_SYSTEM_CONTROL_VECTOR ControlVector
);


//
// Hardware Configuration Routines
//
NTSTATUS
NTAPI
CmpInitializeRegistryNode(
    IN PCONFIGURATION_COMPONENT_DATA CurrentEntry,
    IN HANDLE NodeHandle,
    OUT PHANDLE NewHandle,
    IN INTERFACE_TYPE InterfaceType,
    IN ULONG BusNumber,
    IN PUSHORT DeviceIndexTable
);

NTSTATUS
NTAPI
CmpInitializeMachineDependentConfiguration(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

NTSTATUS
NTAPI
CmpInitializeHardwareConfiguration(
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

//
// Wrapper Routines
//
NTSTATUS
NTAPI
CmpCreateEvent(
    IN EVENT_TYPE EventType,
    OUT PHANDLE EventHandle,
    OUT PKEVENT *Event
);

PVOID
NTAPI
CmpAllocate(
    IN ULONG Size,
    IN BOOLEAN Paged,
    IN ULONG Tag
);

VOID
NTAPI
CmpFree(
    IN PVOID Ptr,
    IN ULONG Quota
);

BOOLEAN
NTAPI
CmpFileRead(
    IN PHHIVE RegistryHive,
    IN ULONG FileType,
    IN OUT PULONG FileOffset,
    OUT PVOID Buffer,
    IN SIZE_T BufferLength
);

BOOLEAN
NTAPI
CmpFileWrite(
    IN PHHIVE RegistryHive,
    IN ULONG FileType,
    IN OUT PULONG FileOffset,
    IN PVOID Buffer,
    IN SIZE_T BufferLength
);

BOOLEAN
NTAPI
CmpFileSetSize(
    IN PHHIVE RegistryHive,
    IN ULONG FileType,
    IN ULONG FileSize,
    IN ULONG OldFileSize
);

BOOLEAN
NTAPI
CmpFileFlush(
   IN PHHIVE RegistryHive,
   IN ULONG FileType,
   IN OUT PLARGE_INTEGER FileOffset,
   IN ULONG Length
);

//
// Configuration Manager side of Registry System Calls
//
NTSTATUS
NTAPI
CmEnumerateValueKey(
    IN PCM_KEY_CONTROL_BLOCK Kcb,
    IN ULONG Index,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    IN PVOID KeyValueInformation,
    IN ULONG Length,
    IN PULONG ResultLength);

NTSTATUS
NTAPI
CmSetValueKey(
    IN PCM_KEY_CONTROL_BLOCK Kcb,
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
    IN PULONG ResultLength
);

NTSTATUS
NTAPI
CmEnumerateKey(IN PCM_KEY_CONTROL_BLOCK Kcb,
    IN ULONG Index,
    IN KEY_INFORMATION_CLASS KeyInformationClass,
    IN PVOID KeyInformation,
    IN ULONG Length,
    IN PULONG ResultLength
);

NTSTATUS
NTAPI
CmDeleteKey(
    IN PCM_KEY_CONTROL_BLOCK Kcb
);

NTSTATUS
NTAPI
CmFlushKey(
    IN PCM_KEY_CONTROL_BLOCK Kcb,
    IN BOOLEAN EclusiveLock
);

NTSTATUS
NTAPI
CmDeleteValueKey(
    IN PCM_KEY_CONTROL_BLOCK Kcb,
    IN UNICODE_STRING ValueName
);

NTSTATUS
NTAPI
CmQueryValueKey(
    IN PCM_KEY_CONTROL_BLOCK Kcb,
    IN UNICODE_STRING ValueName,
    IN KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
    IN PVOID KeyValueInformation,
    IN ULONG Length,
    IN PULONG ResultLength
);

NTSTATUS
NTAPI
CmLoadKey(
    IN POBJECT_ATTRIBUTES TargetKey,
    IN POBJECT_ATTRIBUTES SourceFile,
    IN ULONG Flags,
    IN PKEY_OBJECT KeyBody
);

//
// Startup and Shutdown
//
VOID
NTAPI
CmShutdownSystem(
    VOID
);

//
// Global variables accessible from all of Cm
//
extern BOOLEAN CmpSpecialBootCondition;
extern BOOLEAN CmpFlushOnLockRelease;
extern BOOLEAN CmpShareSystemHives;
extern BOOLEAN CmpMiniNTBoot;
extern EX_PUSH_LOCK CmpHiveListHeadLock, CmpLoadHiveLock;
extern LIST_ENTRY CmpHiveListHead;
extern POBJECT_TYPE CmpKeyObjectType;
extern ERESOURCE CmpRegistryLock;
extern PCM_KEY_HASH_TABLE_ENTRY CmpCacheTable;
extern PCM_NAME_HASH_TABLE_ENTRY CmpNameCacheTable;
extern KGUARDED_MUTEX CmpDelayedCloseTableLock;
extern CMHIVE CmControlHive;
extern WCHAR CmDefaultLanguageId[];
extern ULONG CmDefaultLanguageIdLength;
extern ULONG CmDefaultLanguageIdType;
extern WCHAR CmInstallUILanguageId[];
extern ULONG CmInstallUILanguageIdLength;
extern ULONG CmInstallUILanguageIdType;
extern LANGID PsInstallUILanguageId;
extern LANGID PsDefaultUILanguageId;
extern CM_SYSTEM_CONTROL_VECTOR CmControlVector[];
extern ULONG CmpConfigurationAreaSize;
extern PCM_FULL_RESOURCE_DESCRIPTOR CmpConfigurationData;
extern UNICODE_STRING CmTypeName[];
extern HIVE_LIST_ENTRY CmpMachineHiveList[];
extern UNICODE_STRING CmSymbolicLinkValueName;
extern UNICODE_STRING CmpSystemStartOptions;
extern UNICODE_STRING CmpLoadOptions;
extern BOOLEAN CmSelfHeal;
extern BOOLEAN CmpSelfHeal;
extern ULONG CmpBootType;
extern HANDLE CmpRegistryRootHandle;
extern BOOLEAN ExpInTextModeSetup;
extern BOOLEAN InitIsWinPEMode;
extern ULONG CmpHashTableSize;
extern ULONG CmpDelayedCloseSize, CmpDelayedCloseIndex;
extern BOOLEAN CmpNoWrite;
extern BOOLEAN CmpForceForceFlush;
extern BOOLEAN CmpWasSetupBoot;
extern PCMHIVE CmiVolatileHive;
extern LIST_ENTRY CmiKeyObjectListHead;
extern BOOLEAN CmpHoldLazyFlush;

//
// Inlined functions
//
#include "cm_x.h"
