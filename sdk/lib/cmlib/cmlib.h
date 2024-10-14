/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Configuration Manager Library - CMLIB header
 * COPYRIGHT:   Copyright 2001 - 2005 Eric Kohl
 *              Copyright 2022 George Bișoc <george.bisoc@reactos.org>
 */

#ifndef _CMLIB_H_
#define _CMLIB_H_

//
// Debug support switch
//
#define _CMLIB_DEBUG_ 1

#ifdef CMLIB_HOST
    #include <typedefs.h>
    #include <stdio.h>
    #include <string.h>

    // NTDDI_xxx versions we allude to in the library (see psdk/sdkddkver.h)
    #define NTDDI_WS03SP4                       0x05020400
    #define NTDDI_WIN6                          0x06000000
    #define NTDDI_LONGHORN                      NTDDI_WIN6
    #define NTDDI_VISTA                         NTDDI_WIN6
    #define NTDDI_WIN7                          0x06010000

    #define NTDDI_VERSION   NTDDI_WS03SP4 // This is the ReactOS NT kernel version

    /* C_ASSERT Definition */
    #define C_ASSERT(expr) extern char (*c_assert(void)) [(expr) ? 1 : -1]

    #ifdef _WIN32
    #define strncasecmp _strnicmp
    #define strcasecmp _stricmp
    #endif // _WIN32

    #if (!defined(_MSC_VER) || (_MSC_VER < 1500))
    #define _In_
    #define _Out_
    #define _Inout_
    #define _In_opt_
    #define _In_range_(x, y)
    #endif

    #define __drv_aliasesMem

    #ifndef min
    #define min(a, b)  (((a) < (b)) ? (a) : (b))
    #endif

    // #ifndef max
    // #define max(a, b)  (((a) > (b)) ? (a) : (b))
    // #endif

    // Definitions copied from <ntstatus.h>
    // We only want to include host headers, so we define them manually
    #define STATUS_SUCCESS                   ((NTSTATUS)0x00000000)
    #define STATUS_NOT_IMPLEMENTED           ((NTSTATUS)0xC0000002)
    #define STATUS_NO_MEMORY                 ((NTSTATUS)0xC0000017)
    #define STATUS_INSUFFICIENT_RESOURCES    ((NTSTATUS)0xC000009A)
    #define STATUS_INVALID_PARAMETER         ((NTSTATUS)0xC000000D)
    #define STATUS_REGISTRY_CORRUPT          ((NTSTATUS)0xC000014C)
    #define STATUS_REGISTRY_IO_FAILED        ((NTSTATUS)0xC000014D)
    #define STATUS_NOT_REGISTRY_FILE         ((NTSTATUS)0xC000015C)
    #define STATUS_REGISTRY_RECOVERED        ((NTSTATUS)0x40000009)

    #define REG_OPTION_VOLATILE              1
    #define OBJ_CASE_INSENSITIVE             0x00000040L
    #define USHORT_MAX                       USHRT_MAX

    #define OBJ_NAME_PATH_SEPARATOR          ((WCHAR)L'\\')
    #define UNICODE_NULL                     ((WCHAR)0)

    VOID NTAPI
    RtlInitUnicodeString(
        IN OUT PUNICODE_STRING DestinationString,
        IN PCWSTR SourceString);

    LONG NTAPI
    RtlCompareUnicodeString(
        IN PCUNICODE_STRING String1,
        IN PCUNICODE_STRING String2,
        IN BOOLEAN CaseInSensitive);

    // FIXME: DECLSPEC_NORETURN
    VOID
    NTAPI
    KeBugCheckEx(
        IN ULONG BugCheckCode,
        IN ULONG_PTR BugCheckParameter1,
        IN ULONG_PTR BugCheckParameter2,
        IN ULONG_PTR BugCheckParameter3,
        IN ULONG_PTR BugCheckParameter4);

    VOID NTAPI
    KeQuerySystemTime(
        OUT PLARGE_INTEGER CurrentTime);

    WCHAR NTAPI
    RtlUpcaseUnicodeChar(
        IN WCHAR Source);

    VOID NTAPI
    RtlInitializeBitMap(
        IN PRTL_BITMAP BitMapHeader,
        IN PULONG BitMapBuffer,
        IN ULONG SizeOfBitMap);

    ULONG NTAPI
    RtlFindSetBits(
        IN PRTL_BITMAP BitMapHeader,
        IN ULONG NumberToFind,
        IN ULONG HintIndex);

    VOID NTAPI
    RtlSetBits(
        IN PRTL_BITMAP BitMapHeader,
        IN ULONG StartingIndex,
        IN ULONG NumberToSet);

    VOID NTAPI
    RtlSetAllBits(
        IN PRTL_BITMAP BitMapHeader);

    VOID NTAPI
    RtlClearAllBits(
        IN PRTL_BITMAP BitMapHeader);

    #define RtlCheckBit(BMH,BP) (((((PLONG)(BMH)->Buffer)[(BP) / 32]) >> ((BP) % 32)) & 0x1)
    #define UNREFERENCED_PARAMETER(P) ((void)(P))

    #define PKTHREAD PVOID
    #define PKGUARDED_MUTEX PVOID
    #define PERESOURCE PVOID
    #define PFILE_OBJECT PVOID
    #define PKEVENT PVOID
    #define PWORK_QUEUE_ITEM PVOID
    #define EX_PUSH_LOCK PULONG_PTR

    // Definitions copied from <ntifs.h>
    // We only want to include host headers, so we define them manually

    typedef USHORT SECURITY_DESCRIPTOR_CONTROL, *PSECURITY_DESCRIPTOR_CONTROL;

    typedef struct _SECURITY_DESCRIPTOR_RELATIVE
    {
        UCHAR Revision;
        UCHAR Sbz1;
        SECURITY_DESCRIPTOR_CONTROL Control;
        ULONG Owner;
        ULONG Group;
        ULONG Sacl;
        ULONG Dacl;
    } SECURITY_DESCRIPTOR_RELATIVE, *PISECURITY_DESCRIPTOR_RELATIVE;

    #define CMLTRACE(x, ...)
    #undef PAGED_CODE
    #define PAGED_CODE()
    #define REGISTRY_ERROR                   ((ULONG)0x00000051L)

#else

    //
    // Debug/Tracing support
    //
    #if _CMLIB_DEBUG_
    #ifdef NEW_DEBUG_SYSTEM_IMPLEMENTED // enable when Debug Filters are implemented
    #define CMLTRACE DbgPrintEx
    #else
    #define CMLTRACE(x, ...)                                 \
    if (x & CmlibTraceLevel) DbgPrint(__VA_ARGS__)
    #endif
    #else
    #define CMLTRACE(x, ...) DPRINT(__VA_ARGS__)
    #endif

    #include <ntdef.h>
    #include <ntifs.h>
    #include <bugcodes.h>

    /* Prevent inclusion of Windows headers through <wine/unicode.h> */
    #define _WINDEF_
    #define _WINBASE_
    #define _WINNLS_
#endif


//
// These define the Debug Masks Supported
//
#define CMLIB_HCELL_DEBUG       0x01

#ifndef ROUND_UP
#define ROUND_UP(a,b)        ((((a)+(b)-1)/(b))*(b))
#define ROUND_DOWN(a,b)      (((a)/(b))*(b))
#endif

//
// PAGE_SIZE definition
//
#ifndef PAGE_SIZE
#if defined(TARGET_i386) || defined(TARGET_amd64) || \
    defined(TARGET_arm)  || defined(TARGET_arm64)
#define PAGE_SIZE 0x1000
#else
#error Local PAGE_SIZE definition required when built as host
#endif
#endif

#define TAG_CM             '  MC'
#define TAG_KCB            'bkMC'
#define TAG_CMHIVE         'vHMC'
#define TAG_CMSD           'DSMC'
#define TAG_REGISTRY_STACK 'sRMC'

#define CMAPI NTAPI

//
// Check Registry status type definition
//
typedef ULONG CM_CHECK_REGISTRY_STATUS;

//
// Check Registry flags
//
#define CM_CHECK_REGISTRY_DONT_PURGE_VOLATILES        0x0
#define CM_CHECK_REGISTRY_PURGE_VOLATILES             0x2
#define CM_CHECK_REGISTRY_BOOTLOADER_PURGE_VOLATILES  0x4
#define CM_CHECK_REGISTRY_VALIDATE_HIVE               0x8
#define CM_CHECK_REGISTRY_FIX_HIVE                    0x10

//
// Check Registry status codes
//
#define CM_CHECK_REGISTRY_GOOD                         0
#define CM_CHECK_REGISTRY_INVALID_PARAMETER            1
#define CM_CHECK_REGISTRY_SD_INVALID                   2
#define CM_CHECK_REGISTRY_HIVE_CORRUPT_SIGNATURE       3
#define CM_CHECK_REGISTRY_BIN_SIZE_OR_OFFSET_CORRUPT   4
#define CM_CHECK_REGISTRY_BIN_SIGNATURE_HEADER_CORRUPT 5
#define CM_CHECK_REGISTRY_BAD_FREE_CELL                6
#define CM_CHECK_REGISTRY_BAD_ALLOC_CELL               7
#define CM_CHECK_REGISTRY_ALLOCATE_MEM_STACK_FAIL      8
#define CM_CHECK_REGISTRY_ROOT_CELL_NOT_FOUND          9
#define CM_CHECK_REGISTRY_BAD_LEXICOGRAPHICAL_ORDER    10
#define CM_CHECK_REGISTRY_NODE_NOT_FOUND               11
#define CM_CHECK_REGISTRY_SUBKEY_NOT_FOUND             12
#define CM_CHECK_REGISTRY_TREE_TOO_MANY_LEVELS         13
#define CM_CHECK_REGISTRY_KEY_CELL_NOT_ALLOCATED       14
#define CM_CHECK_REGISTRY_CELL_DATA_NOT_FOUND          15
#define CM_CHECK_REGISTRY_CELL_SIZE_NOT_SANE           16
#define CM_CHECK_REGISTRY_KEY_NAME_LENGTH_ZERO         17
#define CM_CHECK_REGISTRY_KEY_TOO_BIG_THAN_CELL        18
#define CM_CHECK_REGISTRY_BAD_KEY_NODE_PARENT          19
#define CM_CHECK_REGISTRY_BAD_KEY_NODE_SIGNATURE       20
#define CM_CHECK_REGISTRY_KEY_CLASS_UNALLOCATED        21
#define CM_CHECK_REGISTRY_VALUE_LIST_UNALLOCATED       22
#define CM_CHECK_REGISTRY_VALUE_LIST_DATA_NOT_FOUND    23
#define CM_CHECK_REGISTRY_VALUE_LIST_SIZE_NOT_SANE     24
#define CM_CHECK_REGISTRY_VALUE_CELL_NIL               25
#define CM_CHECK_REGISTRY_VALUE_CELL_UNALLOCATED       26
#define CM_CHECK_REGISTRY_VALUE_CELL_DATA_NOT_FOUND    27
#define CM_CHECK_REGISTRY_VALUE_CELL_SIZE_NOT_SANE     28
#define CM_CHECK_REGISTRY_CORRUPT_VALUE_DATA           29
#define CM_CHECK_REGISTRY_DATA_CELL_NOT_ALLOCATED      30
#define CM_CHECK_REGISTRY_BAD_KEY_VALUE_SIGNATURE      31
#define CM_CHECK_REGISTRY_STABLE_KEYS_ON_VOLATILE      32
#define CM_CHECK_REGISTRY_SUBKEYS_LIST_UNALLOCATED     33
#define CM_CHECK_REGISTRY_CORRUPT_SUBKEYS_INDEX        34
#define CM_CHECK_REGISTRY_BAD_SUBKEY_COUNT             35
#define CM_CHECK_REGISTRY_KEY_INDEX_CELL_UNALLOCATED   36
#define CM_CHECK_REGISTRY_CORRUPT_LEAF_ON_ROOT         37
#define CM_CHECK_REGISTRY_CORRUPT_LEAF_SIGNATURE       38
#define CM_CHECK_REGISTRY_CORRUPT_KEY_INDEX_SIGNATURE  39

//
// Check Registry success macro
//
#define CM_CHECK_REGISTRY_SUCCESS(StatusCode) ((ULONG)(StatusCode) == CM_CHECK_REGISTRY_GOOD)

#include <wine/unicode.h>
#include <wchar.h>
#include "hivedata.h"
#include "cmdata.h"

/* Forward declarations */
typedef struct _CM_KEY_SECURITY_CACHE_ENTRY *PCM_KEY_SECURITY_CACHE_ENTRY;
typedef struct _CM_KEY_CONTROL_BLOCK *PCM_KEY_CONTROL_BLOCK;
typedef struct _CM_CELL_REMAP_BLOCK *PCM_CELL_REMAP_BLOCK;

// See ntoskrnl/include/internal/cm.h
#define CMP_SECURITY_HASH_LISTS     64

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
    HANDLE FileHandles[HFILE_TYPE_MAX];
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

typedef struct _HV_HIVE_CELL_PAIR
{
    PHHIVE Hive;
    HCELL_INDEX Cell;
} HV_HIVE_CELL_PAIR, *PHV_HIVE_CELL_PAIR;

#define STATIC_CELL_PAIR_COUNT 4
typedef struct _HV_TRACK_CELL_REF
{
    USHORT Count;
    USHORT Max;
    PHV_HIVE_CELL_PAIR CellArray;
    HV_HIVE_CELL_PAIR StaticArray[STATIC_CELL_PAIR_COUNT];
    USHORT StaticCount;
} HV_TRACK_CELL_REF, *PHV_TRACK_CELL_REF;

extern ULONG CmlibTraceLevel;

//
// Hack since big keys are not yet supported
//
#define ASSERT_VALUE_BIG(h, s)  \
    ASSERTMSG("Big keys not supported!\n", !CmpIsKeyValueBig(h, s));

//
// Returns whether or not this is a small valued key
//
static inline
BOOLEAN
CmpIsKeyValueSmall(OUT PULONG RealLength,
                   IN ULONG Length)
{
    /* Check if the length has the special size value */
    if (Length >= CM_KEY_VALUE_SPECIAL_SIZE)
    {
        /* It does, so this is a small key: return the real length */
        *RealLength = Length - CM_KEY_VALUE_SPECIAL_SIZE;
        return TRUE;
    }

    /* This is not a small key, return the length we read */
    *RealLength = Length;
    return FALSE;
}

//
// Returns whether or not this is a big valued key
//
static inline
BOOLEAN
CmpIsKeyValueBig(IN PHHIVE Hive,
                 IN ULONG Length)
{
    /* Check if the hive is XP Beta 1 or newer */
    if (Hive->Version >= HSYS_WHISTLER_BETA1)
    {
        /* Check if the key length is valid for a big value key */
        if ((Length < CM_KEY_VALUE_SPECIAL_SIZE) && (Length > CM_KEY_VALUE_BIG))
        {
            /* Yes, this value is big */
            return TRUE;
        }
    }

    /* Not a big value key */
    return FALSE;
}

/*
 * Public Hive functions.
 */
NTSTATUS CMAPI
HvInitialize(
    PHHIVE RegistryHive,
    ULONG OperationType,
    ULONG HiveFlags,
    ULONG FileType,
    PVOID HiveData OPTIONAL,
    PALLOCATE_ROUTINE Allocate,
    PFREE_ROUTINE Free,
    PFILE_SET_SIZE_ROUTINE FileSetSize,
    PFILE_WRITE_ROUTINE FileWrite,
    PFILE_READ_ROUTINE FileRead,
    PFILE_FLUSH_ROUTINE FileFlush,
    ULONG Cluster OPTIONAL,
    PCUNICODE_STRING FileName OPTIONAL);

VOID CMAPI
HvFree(
   PHHIVE RegistryHive);

#define HvGetCell(Hive, Cell)   \
    (Hive)->GetCellRoutine(Hive, Cell)

#define HvReleaseCell(Hive, Cell)               \
do {                                            \
    if ((Hive)->ReleaseCellRoutine)             \
        (Hive)->ReleaseCellRoutine(Hive, Cell); \
} while(0)

LONG CMAPI
HvGetCellSize(
   PHHIVE RegistryHive,
   PVOID Cell);

HCELL_INDEX CMAPI
HvAllocateCell(
   PHHIVE RegistryHive,
   ULONG Size,
   HSTORAGE_TYPE Storage,
   IN HCELL_INDEX Vicinity);

BOOLEAN CMAPI
HvIsCellAllocated(
    IN PHHIVE RegistryHive,
    IN HCELL_INDEX CellIndex
);

HCELL_INDEX CMAPI
HvReallocateCell(
   PHHIVE RegistryHive,
   HCELL_INDEX CellOffset,
   ULONG Size);

VOID CMAPI
HvFreeCell(
   PHHIVE RegistryHive,
   HCELL_INDEX CellOffset);

BOOLEAN CMAPI
HvMarkCellDirty(
   PHHIVE RegistryHive,
   HCELL_INDEX CellOffset,
   BOOLEAN HoldingLock);

BOOLEAN CMAPI
HvIsCellDirty(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell
);

BOOLEAN
CMAPI
HvHiveWillShrink(
    IN PHHIVE RegistryHive
);

BOOLEAN CMAPI
HvSyncHive(
   PHHIVE RegistryHive);

BOOLEAN CMAPI
HvWriteHive(
   PHHIVE RegistryHive);

BOOLEAN
CMAPI
HvWriteAlternateHive(
    _In_ PHHIVE RegistryHive);

BOOLEAN
CMAPI
HvSyncHiveFromRecover(
    _In_ PHHIVE RegistryHive);

BOOLEAN
CMAPI
HvTrackCellRef(
    IN OUT PHV_TRACK_CELL_REF CellRef,
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell
);

VOID
CMAPI
HvReleaseFreeCellRefArray(
    IN OUT PHV_TRACK_CELL_REF CellRef
);

/*
 * Private functions.
 */

PCELL_DATA CMAPI
HvpGetCellData(
    _In_ PHHIVE Hive,
    _In_ HCELL_INDEX CellIndex);

PHBIN CMAPI
HvpAddBin(
   PHHIVE RegistryHive,
   ULONG Size,
   HSTORAGE_TYPE Storage);

NTSTATUS CMAPI
HvpCreateHiveFreeCellList(
   PHHIVE Hive);

ULONG CMAPI
HvpHiveHeaderChecksum(
   PHBASE_BLOCK HiveHeader);

BOOLEAN CMAPI
HvpVerifyHiveHeader(
    _In_ PHBASE_BLOCK BaseBlock,
    _In_ ULONG FileType);

//
// Registry Self-Heal Routines
//
BOOLEAN
CMAPI
CmIsSelfHealEnabled(
    _In_ BOOLEAN FixHive);

BOOLEAN
CMAPI
CmpRepairParentKey(
    _Inout_ PHHIVE Hive,
    _In_ HCELL_INDEX TargetKey,
    _In_ HCELL_INDEX ParentKey,
    _In_ BOOLEAN FixHive);

BOOLEAN
CMAPI
CmpRepairParentNode(
    _Inout_ PHHIVE Hive,
    _In_ HCELL_INDEX DirtyCell,
    _In_ HCELL_INDEX ParentCell,
    _Inout_ PCELL_DATA CellData,
    _In_ BOOLEAN FixHive);

BOOLEAN
CMAPI
CmpRepairKeyNodeSignature(
    _Inout_ PHHIVE Hive,
    _In_ HCELL_INDEX DirtyCell,
    _Inout_ PCELL_DATA CellData,
    _In_ BOOLEAN FixHive);

BOOLEAN
CMAPI
CmpRepairClassOfNodeKey(
    _Inout_ PHHIVE Hive,
    _In_ HCELL_INDEX DirtyCell,
    _Inout_ PCELL_DATA CellData,
    _In_ BOOLEAN FixHive);

BOOLEAN
CMAPI
CmpRepairValueList(
    _Inout_ PHHIVE Hive,
    _In_ HCELL_INDEX CurrentCell,
    _In_ BOOLEAN FixHive);

BOOLEAN
CMAPI
CmpRepairValueListCount(
    _Inout_ PHHIVE Hive,
    _In_ HCELL_INDEX CurrentCell,
    _In_ ULONG ListCountIndex,
    _Inout_ PCELL_DATA ValueListData,
    _In_ BOOLEAN FixHive);

BOOLEAN
CMAPI
CmpRepairSubKeyCounts(
    _Inout_ PHHIVE Hive,
    _In_ HCELL_INDEX CurrentCell,
    _In_ ULONG Count,
    _Inout_ PCELL_DATA CellData,
    _In_ BOOLEAN FixHive);

BOOLEAN
CMAPI
CmpRepairSubKeyList(
    _Inout_ PHHIVE Hive,
    _In_ HCELL_INDEX CurrentCell,
    _Inout_ PCELL_DATA CellData,
    _In_ BOOLEAN FixHive);

/* Old-style Public "Cmlib" functions */

BOOLEAN CMAPI
CmCreateRootNode(
   PHHIVE Hive,
   PCWSTR Name);

/* NT-style Public Cm functions */

//
// Check Registry Routines
//
CM_CHECK_REGISTRY_STATUS
NTAPI
HvValidateBin(
    _In_ PHHIVE Hive,
    _In_ PHBIN Bin);

CM_CHECK_REGISTRY_STATUS
NTAPI
HvValidateHive(
    _In_ PHHIVE Hive);

CM_CHECK_REGISTRY_STATUS
NTAPI
CmCheckRegistry(
    _In_ PCMHIVE RegistryHive,
    _In_ ULONG Flags);

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
    IN PCUNICODE_STRING Name
);

USHORT
NTAPI
CmpCompressedNameSize(
    IN PWCHAR Name,
    IN ULONG Length
);

USHORT
NTAPI
CmpCopyName(
    IN PHHIVE Hive,
    OUT PWCHAR Destination,
    IN PCUNICODE_STRING Source
);

VOID
NTAPI
CmpCopyCompressedName(
    OUT PWCHAR Destination,
    IN ULONG DestinationLength,
    IN PWCHAR Source,
    IN ULONG SourceLength
);

BOOLEAN
NTAPI
CmpFindNameInList(
    IN PHHIVE Hive,
    IN PCHILD_LIST ChildList,
    IN PCUNICODE_STRING Name,
    OUT PULONG ChildIndex OPTIONAL,
    OUT PHCELL_INDEX CellIndex
);


//
// Cell Value Routines
//
HCELL_INDEX
NTAPI
CmpFindValueByName(
    IN PHHIVE Hive,
    IN PCM_KEY_NODE KeyNode,
    IN PCUNICODE_STRING Name
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
    IN HSTORAGE_TYPE StorageType,
    IN HCELL_INDEX ValueCell,
    OUT PHCELL_INDEX DataCell
);

NTSTATUS
NTAPI
CmpAddValueToList(
    IN PHHIVE Hive,
    IN HCELL_INDEX ValueCell,
    IN ULONG Index,
    IN HSTORAGE_TYPE StorageType,
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
    OUT PULONG Length,
    OUT PVOID *Buffer,
    OUT PBOOLEAN BufferAllocated,
    OUT PHCELL_INDEX CellToRelease
);

NTSTATUS
NTAPI
CmpCopyKeyValueList(
    IN PHHIVE SourceHive,
    IN PCHILD_LIST SrcValueList,
    IN PHHIVE DestinationHive,
    IN OUT PCHILD_LIST DestValueList,
    IN HSTORAGE_TYPE StorageType
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
CmpRemoveSecurityCellList(
    IN PHHIVE Hive,
    IN HCELL_INDEX SecurityCell
);

VOID
NTAPI
CmpFreeSecurityDescriptor(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell
);

/******************************************************************************/

/* To be implemented by the user of this library */
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

#endif /* _CMLIB_H_ */
