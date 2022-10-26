/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/include/internal/cm.h
 * PURPOSE:         Internal header for the Configuration Manager
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

#pragma once

#include <cmlib.h>
#include <cmreslist.h>
#include "cmboot.h"

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
#define CMTRACE(x, fmt, ...) DPRINT(fmt, ##__VA_ARGS__)
#endif

//
// CM_KEY_CONTROL_BLOCK Signatures
//
#define CM_KCB_SIGNATURE                                'bKmC'
#define CM_KCB_INVALID_SIGNATURE                        '4FmC'

//
// CM_KEY_CONTROL_BLOCK ExtFlags
//
#define CM_KCB_NO_SUBKEY                                0x01
#define CM_KCB_SUBKEY_ONE                               0x02
#define CM_KCB_SUBKEY_HINT                              0x04
#define CM_KCB_SYM_LINK_FOUND                           0x08
#define CM_KCB_KEY_NON_EXIST                            0x10
#define CM_KCB_NO_DELAY_CLOSE                           0x20
#define CM_KCB_INVALID_CACHED_INFO                      0x40
#define CM_KCB_READ_ONLY_KEY                            0x80

//
// CM_KEY_BODY Types
//
#define CM_KEY_BODY_TYPE                                0x6B793032  // 'ky02'

//
// Number of various lists and hashes
//
#if 0 // See sdk/lib/cmlib/cmlib.h
#define CMP_SECURITY_HASH_LISTS                         64
#endif
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
// CmpDoCreate and CmpDoOpen flags
//
#define CMP_CREATE_KCB_KCB_LOCKED                       0x2
#define CMP_OPEN_KCB_NO_CREATE                          0x4

//
// EnlistKeyBodyWithKCB Flags
//
#define CMP_ENLIST_KCB_LOCKED_SHARED                    0x1
#define CMP_ENLIST_KCB_LOCKED_EXCLUSIVE                 0x2

//
// CmpBuildAndLockKcbArray & CmpLockKcbArray Flags
//
#define CMP_LOCK_KCB_ARRAY_EXCLUSIVE                    0x1
#define CMP_LOCK_KCB_ARRAY_SHARED                       0x2

//
// Unload Flags
//
#define CMP_UNLOCK_KCB_LOCKED                    0x1
#define CMP_UNLOCK_REGISTRY_LOCKED               0x2

//
// Maximum size of Value Cache
//
#define MAXIMUM_CACHED_DATA                             (2 * PAGE_SIZE)

//
// Hives to load on startup
//
#define CM_NUMBER_OF_MACHINE_HIVES                      6

//
// Number of items that can fit inside an Allocation Page
//
#define CM_KCBS_PER_PAGE                                \
    ((PAGE_SIZE - FIELD_OFFSET(CM_ALLOC_PAGE, AllocPage)) / sizeof(CM_KEY_CONTROL_BLOCK))
#define CM_DELAYS_PER_PAGE                              \
    ((PAGE_SIZE - FIELD_OFFSET(CM_ALLOC_PAGE, AllocPage)) / sizeof(CM_DELAY_ALLOC))

//
// Cache Lookup & KCB Array constructs
//
#define CMP_SUBKEY_LEVELS_DEPTH_LIMIT   32
#define CMP_KCBS_IN_ARRAY_LIMIT         (CMP_SUBKEY_LEVELS_DEPTH_LIMIT + 2)

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
        struct _CM_KEY_CONTROL_BLOCK *RealKcb;
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

    /* ReactOS specific -- boolean flag to avoid recursive locking of the KCB */
    BOOLEAN KcbLocked;
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
    ULONG Signature;
    ULONG RefCount;
    struct
    {
        ULONG ExtFlags:8;
        ULONG PrivateAlloc:1;
        ULONG Delete:1;
        ULONG DelayedCloseIndex:12;
        ULONG TotalLevels:10;
    };
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
    union
    {
        PCM_INDEX_HINT_BLOCK IndexHint;
        ULONG HashKey;
        ULONG SubKeyCount;
    };
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
    struct
    {
         ULONG KcbUserFlags : 4;
         ULONG KcbVirtControlFlags : 4;
         ULONG KcbDebug : 8;
         ULONG Flags : 16;
    };
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
// Hash Cache Stack
//
typedef struct _CM_HASH_CACHE_STACK
{
    UNICODE_STRING NameOfKey;
    ULONG ConvKey;
} CM_HASH_CACHE_STACK, *PCM_HASH_CACHE_STACK;

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
// MultiFunction Adapter Recognizer Structure
//
typedef struct _CMP_MF_TYPE
{
    PCHAR Identifier;
    USHORT InterfaceType;
    USHORT Count;
} CMP_MF_TYPE, *PCMP_MF_TYPE;

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

VOID
NTAPI
CmpDestroyHiveViewList(
    IN PCMHIVE Hive
);

//
// Security Management Functions
//
NTSTATUS
CmpAssignSecurityDescriptor(
    IN PCM_KEY_CONTROL_BLOCK Kcb,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
);

//
// Security Cache Functions
//
VOID
NTAPI
CmpInitSecurityCache(
    IN PCMHIVE Hive
);

VOID
NTAPI
CmpDestroySecurityCache(
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

VALUE_SEARCH_RETURN_TYPE
NTAPI
CmpCompareNewValueDataAgainstKCBCache(
    IN PCM_KEY_CONTROL_BLOCK Kcb,
    IN PUNICODE_STRING ValueName,
    IN ULONG Type,
    IN PVOID Data,
    IN ULONG DataSize
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
// Hive List Routines
//
BOOLEAN
NTAPI
CmpGetHiveName(
    IN  PCMHIVE Hive,
    OUT PUNICODE_STRING HiveName
);

NTSTATUS
NTAPI
CmpAddToHiveFileList(
    IN PCMHIVE Hive
);

VOID
NTAPI
CmpRemoveFromHiveFileList(
    IN PCMHIVE Hive
);

//
// Quota Routines
//
VOID
NTAPI
CmpSetGlobalQuotaAllowed(
    VOID
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
CmpFlushNotify(
    IN PCM_KEY_BODY KeyBody,
    IN BOOLEAN LockHeld
);

CODE_SEG("INIT")
VOID
NTAPI
CmpInitCallback(
    VOID
);

//
// KCB Cache/Delay Routines
//
CODE_SEG("INIT")
VOID
NTAPI
CmpInitializeCache(
    VOID
);

CODE_SEG("INIT")
VOID
NTAPI
CmpInitCmPrivateDelayAlloc(
    VOID
);

CODE_SEG("INIT")
VOID
NTAPI
CmpInitCmPrivateAlloc(
    VOID
);

CODE_SEG("INIT")
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
    IN ULONG OperationType,
    IN ULONG HiveFlags,
    IN ULONG FileType,
    IN PVOID HiveData OPTIONAL,
    IN HANDLE Primary,
    IN HANDLE Log,
    IN HANDLE External,
    IN PCUNICODE_STRING FileName OPTIONAL,
    IN ULONG CheckFlags
);

NTSTATUS
NTAPI
CmpDestroyHive(
    IN PCMHIVE CmHive
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
    OUT PHANDLE Primary,
    OUT PHANDLE Log,
    OUT PULONG PrimaryDisposition,
    OUT PULONG LogDisposition,
    IN BOOLEAN CreateAllowed,
    IN BOOLEAN MarkAsSystemHive,
    IN BOOLEAN NoBuffering,
    OUT PULONG ClusterSize OPTIONAL
);

VOID
NTAPI
CmpCloseHiveFiles(
    IN PCMHIVE Hive
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
    VOID
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

VOID
NTAPI
CmpLockHiveFlusherExclusive(
    IN PCMHIVE Hive
);

VOID
NTAPI
CmpLockHiveFlusherShared(
    IN PCMHIVE Hive
);

BOOLEAN
NTAPI
CmpTestHiveFlusherLockExclusive(
    IN PCMHIVE Hive
);

BOOLEAN
NTAPI
CmpTestHiveFlusherLockShared(
    IN PCMHIVE Hive
);

VOID
NTAPI
CmpUnlockHiveFlusher(
    IN PCMHIVE Hive
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
CmpArmDelayedCloseTimer(
    VOID
);

VOID
NTAPI
CmpRemoveFromDelayedClose(IN PCM_KEY_CONTROL_BLOCK Kcb);

CODE_SEG("INIT")
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
CmpCleanUpKcbValueCache(
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
CmpCleanUpSubKeyInfo(
    IN PCM_KEY_CONTROL_BLOCK Kcb
);

PUNICODE_STRING
NTAPI
CmpConstructName(
    IN PCM_KEY_CONTROL_BLOCK Kcb
);

BOOLEAN
NTAPI
CmpReferenceKeyControlBlock(
    IN PCM_KEY_CONTROL_BLOCK Kcb
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
DelistKeyBodyFromKCB(
    IN PCM_KEY_BODY KeyBody,
    IN BOOLEAN LockHeld
);

VOID
CmpUnLockKcbArray(
    _In_ PULONG LockedKcbs
);

PULONG
NTAPI
CmpBuildAndLockKcbArray(
    _In_ PCM_HASH_CACHE_STACK HashCacheStack,
    _In_ ULONG KcbLockFlags,
    _In_ PCM_KEY_CONTROL_BLOCK Kcb,
    _Inout_ PULONG OuterStackArray,
    _In_ ULONG TotalRemainingSubkeys,
    _In_ ULONG MatchRemainSubkeyLevel
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

VOID
NTAPI
CmpFlushNotifiesOnKeyBodyList(
    IN PCM_KEY_CONTROL_BLOCK Kcb,
    IN BOOLEAN LockHeld
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
    IN PULONG KcbsLocked,
    OUT PVOID *Object
);

//
// Boot Routines
//
CODE_SEG("INIT")
VOID
NTAPI
CmGetSystemControlValues(
    IN PVOID SystemHiveData,
    IN PCM_SYSTEM_CONTROL_VECTOR ControlVector
);

NTSTATUS
NTAPI
CmpSaveBootControlSet(
    IN USHORT ControlSet
);

//
// Hardware Configuration Routines
//
CODE_SEG("INIT")
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

CODE_SEG("INIT")
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
    IN SIZE_T Size,
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
    _In_ PHHIVE RegistryHive,
    _In_ ULONG FileType,
    _In_ ULONG FileSize,
    _In_ ULONG OldFileSize
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
    IN PCM_KEY_BODY KeyBody
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
    IN PCM_KEY_BODY KeyBody
);

NTSTATUS
NTAPI
CmUnloadKey(
    IN PCM_KEY_CONTROL_BLOCK Kcb,
    IN ULONG Flags
);

ULONG
NTAPI
CmpEnumerateOpenSubKeys(
    IN PCM_KEY_CONTROL_BLOCK RootKcb,
    IN BOOLEAN RemoveEmptyCacheEntries,
    IN BOOLEAN DereferenceOpenedEntries
);

HCELL_INDEX
NTAPI
CmpCopyCell(
    IN PHHIVE SourceHive,
    IN HCELL_INDEX SourceCell,
    IN PHHIVE DestinationHive,
    IN HSTORAGE_TYPE StorageType
);

NTSTATUS
NTAPI
CmpDeepCopyKey(
    IN PHHIVE SourceHive,
    IN HCELL_INDEX SrcKeyCell,
    IN PHHIVE DestinationHive,
    IN HSTORAGE_TYPE StorageType,
    OUT PHCELL_INDEX DestKeyCell OPTIONAL
);

NTSTATUS
NTAPI
CmSaveKey(
    IN PCM_KEY_CONTROL_BLOCK Kcb,
    IN HANDLE FileHandle,
    IN ULONG Flags
);

NTSTATUS
NTAPI
CmSaveMergedKeys(
    IN PCM_KEY_CONTROL_BLOCK HighKcb,
    IN PCM_KEY_CONTROL_BLOCK LowKcb,
    IN HANDLE FileHandle
);

//
// Startup and Shutdown
//
CODE_SEG("INIT")
BOOLEAN
NTAPI
CmInitSystem1(
    VOID
);

VOID
NTAPI
CmShutdownSystem(
    VOID
);

VOID
NTAPI
CmSetLazyFlushState(
    IN BOOLEAN Enable
);

VOID
NTAPI
CmpSetVersionData(
    VOID
);

//
// Driver List Routines
//
CODE_SEG("INIT")
PUNICODE_STRING*
NTAPI
CmGetSystemDriverList(
    VOID
);

//
// Global variables accessible from all of Cm
//
extern ULONG CmpTraceLevel;
extern BOOLEAN CmpSpecialBootCondition;
extern BOOLEAN CmpFlushOnLockRelease;
extern BOOLEAN CmpShareSystemHives;
extern BOOLEAN CmpMiniNTBoot;
extern BOOLEAN CmpNoVolatileCreates;
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
extern ULONG CmNtGlobalFlag;
extern LANGID PsInstallUILanguageId;
extern LANGID PsDefaultUILanguageId;
extern CM_SYSTEM_CONTROL_VECTOR CmControlVector[];
extern ULONG CmpConfigurationAreaSize;
extern PCM_FULL_RESOURCE_DESCRIPTOR CmpConfigurationData;
extern UNICODE_STRING CmTypeName[];
extern UNICODE_STRING CmClassName[];
extern CMP_MF_TYPE CmpMultifunctionTypes[];
extern USHORT CmpUnknownBusCount;
extern ULONG CmpTypeCount[MaximumType + 1];
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
extern BOOLEAN CmpProfileLoaded;
extern PCMHIVE CmiVolatileHive;
extern LIST_ENTRY CmiKeyObjectListHead;
extern BOOLEAN CmpHoldLazyFlush;
extern BOOLEAN HvShutdownComplete;

//
// Inlined functions
//
#include "cm_x.h"
