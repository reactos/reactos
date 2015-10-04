/*
 * PROJECT:   registry manipulation library
 * LICENSE:   GPL - See COPYING in the top level directory
 * COPYRIGHT: Copyright 2005 Filip Navara <navaraf@reactos.org>
 *            Copyright 2001 - 2005 Eric Kohl
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

    #ifdef _WIN32
    #define strncasecmp _strnicmp
    #define strcasecmp _stricmp
    #endif//_WIN32

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
    #define STATUS_REGISTRY_CORRUPT          ((NTSTATUS)0xC000014C)
    #define STATUS_NOT_REGISTRY_FILE         ((NTSTATUS)0xC000015C)
    #define STATUS_REGISTRY_RECOVERED        ((NTSTATUS)0x40000009)

    #define REG_OPTION_VOLATILE              1
    #define OBJ_CASE_INSENSITIVE             0x00000040L
    #define USHORT_MAX                       USHRT_MAX

    #define OBJ_NAME_PATH_SEPARATOR          ((WCHAR)L'\\')

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
    RtlClearAllBits(
        IN PRTL_BITMAP BitMapHeader);

    #define RtlCheckBit(BMH,BP) (((((PLONG)(BMH)->Buffer)[(BP) / 32]) >> ((BP) % 32)) & 0x1)
    #define UNREFERENCED_PARAMETER(P) {(P)=(P);}

    #define PKTHREAD PVOID
    #define PKGUARDED_MUTEX PVOID
    #define PERESOURCE PVOID
    #define PFILE_OBJECT PVOID
    #define PKEVENT PVOID
    #define PWORK_QUEUE_ITEM PVOID
    #define EX_PUSH_LOCK PULONG_PTR

    #define CMLTRACE(x, ...)
    #undef PAGED_CODE
    #define PAGED_CODE()
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
    #include <ntddk.h>
    #undef PAGED_CODE
    #define PAGED_CODE()

    /* Prevent inclusion of Windows headers through <wine/unicode.h> */
    #define _WINDEF_
    #define _WINBASE_
    #define _WINNLS_
#endif


//
// These define the Debug Masks Supported
//
#define CMLIB_HCELL_DEBUG                                 0x01

#ifndef ROUND_UP
#define ROUND_UP(a,b)        ((((a)+(b)-1)/(b))*(b))
#define ROUND_DOWN(a,b)      (((a)/(b))*(b))
#endif

//
// PAGE_SIZE definition
//
#ifndef PAGE_SIZE
#if defined(TARGET_i386) || defined(TARGET_amd64) || defined(TARGET_arm)
#define PAGE_SIZE 0x1000
#else
#error Local PAGE_SIZE definition required when built as host
#endif
#endif

#define TAG_CM     '  MC'
#define TAG_KCB    'bkMC'
#define TAG_CMHIVE 'vHMC'
#define TAG_CMSD   'DSMC'

#define CMAPI NTAPI

#include <wine/unicode.h>
#include <wchar.h>
#include "hivedata.h"
#include "cmdata.h"

#if defined(_TYPEDEFS_HOST_H) || defined(__FREELDR_H)

#define PCM_KEY_SECURITY_CACHE_ENTRY PVOID
#define PCM_KEY_CONTROL_BLOCK PVOID
#define CMP_SECURITY_HASH_LISTS                         64
#define PCM_CELL_REMAP_BLOCK PVOID

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

#endif

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

/*
 * Public Hive functions.
 */
NTSTATUS CMAPI
HvInitialize(
   PHHIVE RegistryHive,
   ULONG Operation,
   ULONG HiveType,
   ULONG HiveFlags,
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

PVOID CMAPI
HvGetCell(
   PHHIVE RegistryHive,
   HCELL_INDEX CellOffset);

#define HvReleaseCell(h, c)             \
do {                                    \
    if ((h)->ReleaseCellRoutine)        \
        (h)->ReleaseCellRoutine(h, c);  \
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
HvTrackCellRef(
    PHV_TRACK_CELL_REF CellRef,
    PHHIVE Hive,
    HCELL_INDEX Cell
);

VOID
CMAPI
HvReleaseFreeCellRefArray(
    PHV_TRACK_CELL_REF CellRef
);

/*
 * Private functions.
 */

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


/* Old-style Public "Cmlib" functions */

BOOLEAN CMAPI
CmCreateRootNode(
   PHHIVE Hive,
   PCWSTR Name);

VOID CMAPI
CmPrepareHive(
   PHHIVE RegistryHive);

BOOLEAN
NTAPI
CmCompareHash(
    IN PCUNICODE_STRING KeyName,
    IN PCHAR HashString,
    IN BOOLEAN CaseInsensitive);

BOOLEAN
NTAPI
CmComparePackedNames(
    IN PCUNICODE_STRING Name,
    IN PVOID NameBuffer,
    IN USHORT NameBufferSize,
    IN BOOLEAN NamePacked,
    IN BOOLEAN CaseInsensitive);

BOOLEAN
NTAPI
CmCompareKeyName(
    IN PCM_KEY_NODE KeyCell,
    IN PCUNICODE_STRING KeyName,
    IN BOOLEAN CaseInsensitive);

BOOLEAN
NTAPI
CmCompareKeyValueName(
    IN PCM_KEY_VALUE ValueCell,
    IN PCUNICODE_STRING KeyName,
    IN BOOLEAN CaseInsensitive);

ULONG
NTAPI
CmCopyKeyName(
    _In_ PCM_KEY_NODE KeyNode,
    _Out_ PWCHAR KeyNameBuffer,
    _Inout_ ULONG BufferLength);

ULONG
NTAPI
CmCopyKeyValueName(
    _In_ PCM_KEY_VALUE ValueCell,
    _Out_ PWCHAR ValueNameBuffer,
    _Inout_ ULONG BufferLength);

/* NT-style Public Cm functions */

LONG
NTAPI
CmpCompareCompressedName(
    IN PCUNICODE_STRING SearchName,
    IN PWCHAR CompressedName,
    IN ULONG NameLength
    );

USHORT
NTAPI
CmpCompressedNameSize(
    IN PWCHAR Name,
    IN ULONG Length
    );

HCELL_INDEX
NTAPI
CmpFindSubKeyByName(
    IN PHHIVE Hive,
    IN PCM_KEY_NODE Parent,
    IN PCUNICODE_STRING SearchName
    );

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

VOID
NTAPI
CmpCopyCompressedName(
    IN PWCHAR Destination,
    IN ULONG DestinationLength,
    IN PWCHAR Source,
    IN ULONG SourceLength
    );

#endif /* _CMLIB_H_ */
