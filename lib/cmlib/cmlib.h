/*
 * PROJECT:   registry manipulation library
 * LICENSE:   GPL - See COPYING in the top level directory
 * COPYRIGHT: Copyright 2005 Filip Navara <navaraf@reactos.org>
 *            Copyright 2001 - 2005 Eric Kohl
 */

#ifndef CMLIB_H
#define CMLIB_H

//
// Debug support switch
//
#define _CMLIB_DEBUG_ 1

#ifdef CMLIB_HOST
    #include <host/typedefs.h>
    #include <stdio.h>
    #include <string.h>

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

    VOID NTAPI
    KeQuerySystemTime(
        OUT PLARGE_INTEGER CurrentTime);

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

    #define PKTHREAD PVOID
    #define PKGUARDED_MUTEX PVOID
    #define PERESOURCE PVOID
    #define PFILE_OBJECT PVOID
    #define PKEVENT PVOID
    #define PWORK_QUEUE_ITEM PVOID
    #define EX_PUSH_LOCK PULONG_PTR

    /* For <host/wcsfuncs.h> */
    #define USE_HOST_WCSFUNCS

    #define CMLTRACE(x, ...)
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


    #include <ntddk.h>
#endif

#include <host/wcsfuncs.h>

//
// These define the Debug Masks Supported
//
#define CMLIB_HCELL_DEBUG                                 0x01

#ifndef ROUND_UP
#define ROUND_UP(a,b)        ((((a)+(b)-1)/(b))*(b))
#define ROUND_DOWN(a,b)      (((a)/(b))*(b))
#endif

#define TAG_CM 0x68742020

#define CMAPI NTAPI

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

#endif

extern ULONG CmlibTraceLevel;

/*
 * Public functions.
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
   PUNICODE_STRING FileName);

VOID CMAPI
HvFree(
   PHHIVE RegistryHive);

PVOID CMAPI
HvGetCell(
   PHHIVE RegistryHive,
   HCELL_INDEX CellOffset);

#define HvReleaseCell(h, c)     \
    if (h->ReleaseCellRoutine) h->ReleaseCellRoutine(h, c)

LONG CMAPI
HvGetCellSize(
   PHHIVE RegistryHive,
   PVOID Cell);

HCELL_INDEX CMAPI
HvAllocateCell(
   PHHIVE RegistryHive,
   SIZE_T Size,
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

BOOLEAN CMAPI
HvSyncHive(
   PHHIVE RegistryHive);

BOOLEAN CMAPI
HvWriteHive(
   PHHIVE RegistryHive);

BOOLEAN CMAPI
CmCreateRootNode(
   PHHIVE Hive,
   PCWSTR Name);

VOID CMAPI
CmPrepareHive(
   PHHIVE RegistryHive);

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

#endif /* CMLIB_H */
