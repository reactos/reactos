/*
 * PROJECT:   registry manipulation library
 * LICENSE:   GPL - See COPYING in the top level directory
 * COPYRIGHT: Copyright 2005 Filip Navara <navaraf@reactos.org>
 *            Copyright 2001 - 2005 Eric Kohl
 */

#ifndef CMLIB_H
#define CMLIB_H

//#define WIN32_NO_STATUS
#include <ntddk.h>
#include "hivedata.h"
#include "cmdata.h"

#ifndef ROUND_UP
#define ROUND_UP(a,b)        ((((a)+(b)-1)/(b))*(b))
#define ROUND_DOWN(a,b)      (((a)/(b))*(b))
#endif

#define CMAPI NTAPI

struct _HHIVE;

typedef PVOID (CMAPI *PGET_CELL_ROUTINE)(
   struct _HHIVE *Hive,
   HCELL_INDEX Cell);

typedef VOID (CMAPI *PRELEASE_CELL_ROUTINE)(
   struct _HHIVE *Hive,
   HCELL_INDEX Cell);

typedef PVOID (CMAPI *PALLOCATE_ROUTINE)(
   SIZE_T Size,
   BOOLEAN Paged);

typedef VOID (CMAPI *PFREE_ROUTINE)(
   PVOID Ptr);

typedef BOOLEAN (CMAPI *PFILE_READ_ROUTINE)(
   struct _HHIVE *RegistryHive,
   ULONG FileType,
   ULONGLONG FileOffset,
   PVOID Buffer,
   SIZE_T BufferLength);

typedef BOOLEAN (CMAPI *PFILE_WRITE_ROUTINE)(
   struct _HHIVE *RegistryHive,
   ULONG FileType,
   ULONGLONG FileOffset,
   PVOID Buffer,
   SIZE_T BufferLength);

typedef BOOLEAN (CMAPI *PFILE_SET_SIZE_ROUTINE)(
   struct _HHIVE *RegistryHive,
   ULONG FileType,
   ULONGLONG FileSize);

typedef BOOLEAN (CMAPI *PFILE_FLUSH_ROUTINE)(
   struct _HHIVE *RegistryHive,
   ULONG FileType);

typedef struct _HMAP_ENTRY
{
    ULONG_PTR Bin;
    ULONG_PTR Block;
    struct _CM_VIEW_OF_FILE *CmHive;
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
    PHBASE_BLOCK HiveHeader;
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
    DUAL Storage[HvMaxStorageType];
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

#define HV_OPERATION_CREATE_HIVE    0
#define HV_OPERATION_MEMORY         1
#define HV_OPERATION_MEMORY_INPLACE 3

NTSTATUS CMAPI
HvInitialize(
   PHHIVE RegistryHive,
   ULONG Operation,
   ULONG HiveType,
   ULONG HiveFlags,
   ULONG_PTR HiveData OPTIONAL,
   ULONG Cluster OPTIONAL,
   PALLOCATE_ROUTINE Allocate,
   PFREE_ROUTINE Free,
   PFILE_READ_ROUTINE FileRead,
   PFILE_WRITE_ROUTINE FileWrite,
   PFILE_SET_SIZE_ROUTINE FileSetSize,
   PFILE_FLUSH_ROUTINE FileFlush,
   IN PUNICODE_STRING FileName);

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
   HV_STORAGE_TYPE Storage);

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

VOID CMAPI
HvMarkCellDirty(
   PHHIVE RegistryHive,
   HCELL_INDEX CellOffset);

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
   HV_STORAGE_TYPE Storage);

NTSTATUS CMAPI
HvpCreateHiveFreeCellList(
   PHHIVE Hive);

ULONG CMAPI
HvpHiveHeaderChecksum(
   PHBASE_BLOCK HiveHeader);

#endif /* CMLIB_H */
