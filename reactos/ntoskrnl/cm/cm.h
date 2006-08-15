/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/cm/cm.h
* PURPOSE:         Internal header for the Configuration Manager
* PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
*/

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
// Tag for all registry allocations
//
#define TAG_CM                                          \
    TAG('C', 'm', ' ', ' ')

//
// Hive operations
//
#define HINIT_CREATE                                    0
#define HINIT_MEMORY                                    1
#define HINIT_FILE                                      2
#define HINIT_MEMORY_INPLACE                            3
#define HINIT_FLAT                                      4
#define HINIT_MAPFILE                                   5

//
// Hive flags
//
#define HIVE_VOLATILE                                   1
#define HIVE_NOLAZYFLUSH                                2

//
// Hive types
//
#define HFILE_TYPE_PRIMARY                              0
#define HFILE_TYPE_ALTERNATE                            1
#define HFILE_TYPE_LOG                                  2
#define HFILE_TYPE_EXTERNAL                             3
#define HFILE_TYPE_MAX                                  4

//
// Hive sizes
//
#define HBLOCK_SIZE                                     0x1000
#define HSECTOR_SIZE                                    0x200
#define HSECTOR_COUNT                                   8

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
// CM_KEY_NODE Signature and Flags
//
#define CM_KEY_NODE_SIGNATURE                           \
    TAG('k', 'n', ' ', ' ')
#define KEY_IS_VOLATILE                                 0x01
#define KEY_HIVE_EXIT                                   0x02
#define KEY_HIVE_ENTRY                                  0x04
#define KEY_NO_DELETE                                   0x08
#define KEY_SYM_LINK                                    0x10
#define KEY_COMP_NAME                                   0x20
#define KEY_PREFEF_HANDLE                               0x40
#define KEY_VIRT_MIRRORED                               0x80
#define KEY_VIRT_TARGET                                 0x100
#define KEY_VIRTUAL_STORE                               0x200

//
// Number of various lists and hashes
//
#define CMP_SECURITY_HASH_LISTS                         64

//
// Hashing Constants
//
#define CMP_HASH_IRRATIONAL                             314159269
#define CMP_HASH_PRIME                                  1000000007

//
// CmpCreateKcb Flags
//
#define CMP_CREATE_FAKE_KCB                             0x1
#define CMP_LOCK_HASHES_FOR_KCB                         0x2

//
// Number of items that can fit inside an Allocation Page
//
#define CM_KCBS_PER_PAGE                                \
    PAGE_SIZE / sizeof(CM_KEY_CONTROL_BLOCK)
#define CM_DELAYS_PER_PAGE                       \
    PAGE_SIZE / sizeof(CM_DELAYED_CLOSE_ENTRY)

//
// A Cell Index is just a ULONG
//
typedef ULONG HCELL_INDEX;

//
// Mapped View of a Hive
//
typedef struct _CM_VIEW_OF_FILE
{
    LIST_ENTRY LRUViewList;
    LIST_ENTRY PinViewList;
    ULONG FileOffset;
    ULONG Size;
    PULONG ViewAddress;
    PVOID Bcb;
    ULONG UseCount;
} CM_VIEW_OF_FILE, *PCM_VIEW_OF_FILE;

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
// Child List
//
typedef struct _CHILD_LIST
{
    ULONG Count;
    HCELL_INDEX List;
} CHILD_LIST, *PCM_CHILD_LIST;

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
// Key Reference
//
typedef struct _CM_KEY_REFERENCE
{
    HCELL_INDEX KeyCell;
    PHHIVE KeyHive;
} CM_KEY_REFERENCE, *PCM_KEY_REFERENCE;

//
// Key Body
//
typedef struct _CM_KEY_BODY
{
    ULONG Type;
    struct _CM_KEY_CONTROL_BLOCK *KeyControlBlock;
    struct _CM_NOTIFY_BLOCK *NotifyBlock;
    HANDLE ProcessID;
    ULONG Callers;
    PVOID CallerAddress[10];
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
// Delayed Close Entry
//
typedef struct _CM_DELAYED_CLOSE_ENTRY
{
    LIST_ENTRY DelayedLRUList;
    PCM_KEY_CONTROL_BLOCK KeyControlBlock;
} CM_DELAYED_CLOSE_ENTRY, *PCM_DELAYED_CLOSE_ENTRY;

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
// Key Node
//
typedef struct _CM_KEY_NODE
{
    USHORT Signature;
    USHORT Flags;
    LARGE_INTEGER LastWriteTime;
    ULONG Spare;
    HCELL_INDEX Parent;
    ULONG SubKeyCounts[2];
    union
    {
        struct
        {
            HCELL_INDEX SubKeyLists[2];
            CHILD_LIST ValueList;
        };
        CM_KEY_REFERENCE ChildHiveReference;
    };
    HCELL_INDEX Security;
    HCELL_INDEX Class;
    ULONG MaxNameLen;
    ULONG MaxClassLen;
    ULONG MaxValueNameLen;
    ULONG MaxValueDataLen;
    ULONG WorkVar;
    USHORT NameLength;
    USHORT ClassLength;
    WCHAR Name[ANYSIZE_ARRAY];
} CM_KEY_NODE, *PCM_KEY_NODE;

//
// Key Value
//
typedef struct _CM_KEY_VALUE
{
    USHORT Signature;
    USHORT NameLenght;
    ULONG DataLength;
    HCELL_INDEX Data;
    ULONG Type;
    USHORT Flags;
    USHORT Spare;
    WCHAR Name[ANYSIZE_ARRAY];
} CM_KEY_VALUE, *PCM_KEY_VALUE;

//
// Key Security
//
typedef struct _CM_KEY_SECURITY
{
    USHORT Signature;
    USHORT Reserved;
    HCELL_INDEX Flink;
    HCELL_INDEX Blink;
    ULONG ReferenceCount;
    ULONG DescriptorLength;
    SECURITY_DESCRIPTOR_RELATIVE Descriptor;
} CM_KEY_SECURITY, *PCM_KEY_SECURITY;

//
// Key Index
//
typedef struct _CM_KEY_INDEX
{
    USHORT Signature;
    USHORT Count;
    HCELL_INDEX List[ANYSIZE_ARRAY];
} CM_KEY_INDEX, *PCM_KEY_INDEX;

//
// Cell Data
//
typedef struct _CELL_DATA
{
    union
    {
        CM_KEY_NODE KeyNode;
        CM_KEY_VALUE KeyValue;
        CM_KEY_SECURITY KeySecurity;
        CM_KEY_INDEX KeyIndex;
        HCELL_INDEX KeyList[ANYSIZE_ARRAY];
        WCHAR KeyString[ANYSIZE_ARRAY];
    } u;
} CELL_DATA, *PCELL_DATA;

//
// Registry Validation Functions
//
BOOLEAN
NTAPI
CmCheckRegistry(
    IN PCMHIVE Hive,
    IN BOOLEAN CleanFlag
);

//
// Registry Lock
//
BOOLEAN
NTAPI
CmpTestRegistryLockExclusive(
    VOID
);

//
// Global variables accessible from all of Cm
//

//
// Inlined functions
//
#include "cm_x.h"
