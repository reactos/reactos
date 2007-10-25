/*
 * PROJECT:   registry manipulation library
 * LICENSE:   GPL - See COPYING in the top level directory
 * COPYRIGHT: Copyright 2005 Filip Navara <navaraf@reactos.org>
 *            Copyright 2001 - 2005 Eric Kohl
 */

#ifndef CMLIB_H
#define CMLIB_H

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

#endif

#ifndef _TYPEDEFS_HOST_H
 #include <ntddk.h>
#else
 #define REG_OPTION_VOLATILE 1
 #define OBJ_CASE_INSENSITIVE 0x00000040L
 #define USHORT_MAX USHRT_MAX

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

#endif

#include <wchar.h>
#include "hivedata.h"
#include "cmdata.h"

#ifndef ROUND_UP
#define ROUND_UP(a,b)        ((((a)+(b)-1)/(b))*(b))
#define ROUND_DOWN(a,b)      (((a)/(b))*(b))
#endif

#define TAG_CM 0x68742020

#define CMAPI NTAPI

struct _HHIVE;

typedef struct _CELL_DATA* (CMAPI *PGET_CELL_ROUTINE)(
   struct _HHIVE *Hive,
   HCELL_INDEX Cell);

typedef VOID (CMAPI *PRELEASE_CELL_ROUTINE)(
   struct _HHIVE *Hive,
   HCELL_INDEX Cell);

typedef PVOID (CMAPI *PALLOCATE_ROUTINE)(
   SIZE_T Size,
   BOOLEAN Paged,
   ULONG Tag);

typedef VOID (CMAPI *PFREE_ROUTINE)(
   PVOID Ptr,
   ULONG Quota);

typedef BOOLEAN (CMAPI *PFILE_READ_ROUTINE)(
   struct _HHIVE *RegistryHive,
   ULONG FileType,
   PULONG FileOffset,
   PVOID Buffer,
   SIZE_T BufferLength);

typedef BOOLEAN (CMAPI *PFILE_WRITE_ROUTINE)(
   struct _HHIVE *RegistryHive,
   ULONG FileType,
   PULONG FileOffset,
   PVOID Buffer,
   SIZE_T BufferLength);

typedef BOOLEAN (CMAPI *PFILE_SET_SIZE_ROUTINE)(
   struct _HHIVE *RegistryHive,
   ULONG FileType,
   ULONG FileSize,
   ULONG OldfileSize);

typedef BOOLEAN (CMAPI *PFILE_FLUSH_ROUTINE)(
   struct _HHIVE *RegistryHive,
   ULONG FileType,
   PLARGE_INTEGER FileOffset,
   ULONG Length);

typedef struct _HMAP_ENTRY
{
    ULONG_PTR BlockAddress;
    ULONG_PTR BinAddress;
    struct _CM_VIEW_OF_FILE *CmView;
    ULONG MemAlloc;
} HMAP_ENTRY, *PHMAP_ENTRY;

typedef struct _HMAP_TABLE
{
    HMAP_ENTRY Table[512];
} HMAP_TABLE, *PHMAP_TABLE;

typedef struct _HMAP_DIRECTORY
{
    PHMAP_TABLE Directory[2048];
} HMAP_DIRECTORY, *PHMAP_DIRECTORY;

typedef struct _DUAL
{
    ULONG Length;
    PHMAP_DIRECTORY Map;
    PHMAP_ENTRY BlockList; // PHMAP_TABLE SmallDir;
    ULONG Guard;
    HCELL_INDEX FreeDisplay[24]; //FREE_DISPLAY FreeDisplay[24];
    ULONG FreeSummary;
    LIST_ENTRY FreeBins;
} DUAL, *PDUAL;

typedef struct _HHIVE
{
    ULONG Signature;
    PGET_CELL_ROUTINE GetCellRoutine;
    PRELEASE_CELL_ROUTINE ReleaseCellRoutine;
    PALLOCATE_ROUTINE Allocate;
    PFREE_ROUTINE Free;
    PFILE_READ_ROUTINE FileRead;
    PFILE_WRITE_ROUTINE FileWrite;
    PFILE_SET_SIZE_ROUTINE FileSetSize;
    PFILE_FLUSH_ROUTINE FileFlush;
    PHBASE_BLOCK BaseBlock;
    RTL_BITMAP DirtyVector;
    ULONG DirtyCount;
    ULONG DirtyAlloc;
    ULONG BaseBlockAlloc;
    ULONG Cluster;
    BOOLEAN Flat;
    BOOLEAN ReadOnly;
    BOOLEAN Log;
    BOOLEAN DirtyFlag;
    ULONG HvBinHeadersUse;
    ULONG HvFreeCellsUse;
    ULONG HvUsedcellsUse;
    ULONG CmUsedCellsUse;
    ULONG HiveFlags;
    ULONG LogSize;
    ULONG RefreshCount;
    ULONG StorageTypeCount;
    ULONG Version;
    DUAL Storage[HTYPE_COUNT];
} HHIVE, *PHHIVE;

#ifndef _CM_
typedef struct _EREGISTRY_HIVE
{
  HHIVE Hive;
  LIST_ENTRY  HiveList;
  UNICODE_STRING  HiveFileName;
  UNICODE_STRING  LogFileName;
  PCM_KEY_SECURITY  RootSecurityCell;
  ULONG  Flags;
  HANDLE  HiveHandle;
  HANDLE  LogHandle;
} EREGISTRY_HIVE, *PEREGISTRY_HIVE;
#endif

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
