/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    hive.h

Abstract:

    This module contains the private (internal) header file for the
    direct memory loaded hive manager.

Author:

    Bryan M. Willman (bryanwi) 28-May-91

Environment:

Revision History:

    26-Mar-92 bryanwi - changed to type 1.0 hive format

    13-Jan-99 Dragos C. Sambotin (dragoss) - factoring the data structure declarations
        in \nt\private\ntos\inc\hivedata.h :: to be available from outside.


--*/

#ifndef _HIVE_
#define _HIVE_

// Hive data structure declarations
// file location: \nt\private\ntos\inc
#include "hivedata.h"

#if DBG

extern ULONG HvHiveChecking;

#define DHvCheckHive(a) if(HvHiveChecking) ASSERT(HvCheckHive(a,NULL) == 0)
#define DHvCheckBin(h,a)  if(HvHiveChecking) ASSERT(HvCheckBin(h,a,NULL) == 0)

#else
#define DHvCheckHive(a)
#define DHvCheckBin(h,a)
#endif

#define ROUND_UP(a, b)  \
    ( ((ULONG)(a) + (ULONG)(b) - 1) & ~((ULONG)(b) - 1) )


//
// tombstone for an HBIN that is not resident in memory.  This list is searched
// before any new HBIN is added.
//

#define ASSERT_LISTENTRY(ListEntry) \
    ASSERT((ListEntry)->Flink->Blink==ListEntry); \
    ASSERT((ListEntry)->Blink->Flink==ListEntry);

//
// ===== Hive Private Procedure Prototypes =====
//
PHBIN
HvpAddBin(
    PHHIVE          Hive,
    ULONG           NewSize,
    HSTORAGE_TYPE   Type
    );

PHMAP_ENTRY
HvpGetCellMap(
    PHHIVE      Hive,
    HCELL_INDEX Cell
    );

VOID
HvpFreeMap(
    PHHIVE          Hive,
    PHMAP_DIRECTORY Dir,
    ULONG           Start,
    ULONG           End
    );

BOOLEAN
HvpAllocateMap(
    PHHIVE          Hive,
    PHMAP_DIRECTORY Dir,
    ULONG           Start,
    ULONG           End
    );

BOOLEAN
HvpGrowLog1(
    PHHIVE  Hive,
    ULONG   Count
    );

BOOLEAN
HvpGrowLog2(
    PHHIVE  Hive,
    ULONG   Size
    );

ULONG
HvpHeaderCheckSum(
    PHBASE_BLOCK    BaseBlock
    );

NTSTATUS
HvpBuildMap(
    PHHIVE  Hive,
    PVOID   Image,
    PHCELL_INDEX TailDisplay OPTIONAL
    );

NTSTATUS
HvpBuildMapAndCopy(
    PHHIVE  Hive,
    PVOID   Image,
    PHCELL_INDEX TailDisplay OPTIONAL
    );

NTSTATUS
HvpInitMap(
    PHHIVE  Hive
    );

VOID
HvpCleanMap(
    PHHIVE  Hive
    );

NTSTATUS
HvpEnlistBinInMap(
    PHHIVE  Hive,
    ULONG   Length,
    PHBIN   Bin,
    ULONG   Offset,
    PHCELL_INDEX TailDisplay OPTIONAL
    );

VOID
HvpFreeAllocatedBins(
    PHHIVE Hive
    );

BOOLEAN
HvpDoWriteHive(
    PHHIVE          Hive,
    ULONG           FileType
    );

struct _CELL_DATA *
HvpGetCellFlat(
    PHHIVE      Hive,
    HCELL_INDEX Cell
    );

struct _CELL_DATA *
HvpGetCellPaged(
    PHHIVE      Hive,
    HCELL_INDEX Cell
    );

VOID
HvpEnlistFreeCell(
    PHHIVE  Hive,
    HCELL_INDEX Cell,
    ULONG      Size,
    HSTORAGE_TYPE   Type,
    BOOLEAN CoalesceForward,
    PHCELL_INDEX TailDisplay OPTIONAL
    );

BOOLEAN
HvpEnlistFreeCells(
    PHHIVE  Hive,
    PHBIN   Bin,
    ULONG   BinOffset,
    PHCELL_INDEX TailDisplay OPTIONAL
    );


VOID
HvpDelistFreeCell(
    PHHIVE  Hive,
    PHCELL  Pcell,
    HSTORAGE_TYPE Type,
    PHCELL_INDEX TailDisplay OPTIONAL
    );

//
// ===== Hive Public Procedure Prototypes =====
//
#define HINIT_CREATE            0
#define HINIT_MEMORY            1
#define HINIT_FILE              2
#define HINIT_MEMORY_INPLACE    3
#define HINIT_FLAT              4

#define HIVE_VOLATILE           1
#define HIVE_NOLAZYFLUSH        2
#define HIVE_HAS_BEEN_REPLACED  4

NTSTATUS
HvInitializeHive(
    PHHIVE                  Hive,
    ULONG                   OperationType,
    ULONG                   HiveFlags,
    ULONG                   FileTypes,
    PVOID                   HiveData OPTIONAL,
    PALLOCATE_ROUTINE       AllocateRoutine,
    PFREE_ROUTINE           FreeRoutine,
    PFILE_SET_SIZE_ROUTINE  FileSetSizeRoutine,
    PFILE_WRITE_ROUTINE     FileWriteRoutine,
    PFILE_READ_ROUTINE      FileReadRoutine,
    PFILE_FLUSH_ROUTINE     FileFlushRoutine,
    ULONG                   Cluster,
    PUNICODE_STRING         FileName
    );

BOOLEAN
HvSyncHive(
    PHHIVE  Hive
    );

NTSTATUS
HvWriteHive(
    PHHIVE  Hive
    );

NTSTATUS
HvLoadHive(
    PHHIVE  Hive,
    PHCELL_INDEX TailDisplay OPTIONAL
    );

VOID
HvRefreshHive(
    PHHIVE  Hive
    );

NTSTATUS
HvReadInMemoryHive(
    PHHIVE  Hive,
    PVOID   *HiveImage
    );

ULONG
HvCheckHive(
    PHHIVE  Hive,
    PULONG  Storage OPTIONAL
    );

ULONG
HvCheckBin(
    PHHIVE  Hive,
    PHBIN   Bin,
    PULONG  Storage
    );

BOOLEAN
HvMarkCellDirty(
    PHHIVE      Hive,
    HCELL_INDEX Cell
    );

BOOLEAN
HvIsBinDirty(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell
    );

BOOLEAN
HvMarkDirty(
    PHHIVE      Hive,
    HCELL_INDEX Start,
    ULONG       Length
    );

BOOLEAN
HvMarkClean(
    PHHIVE      Hive,
    HCELL_INDEX Start,
    ULONG       Length
    );

#define HvGetCell(Hive, Cell)   (((Hive)->GetCellRoutine)(Hive, Cell))

#define HvpGetHCell(Hive, Cell) ( USE_OLD_CELL(Hive) ?                      \
                                  CONTAINING_RECORD(HvGetCell(Hive,Cell),   \
                                                    HCELL,                  \
                                                    u.OldCell.u.UserData) : \
                                  CONTAINING_RECORD(HvGetCell(Hive,Cell),   \
                                                    HCELL,                  \
                                                    u.NewCell.u.UserData))


LONG
HvGetCellSize(
    PHHIVE      Hive,
    PVOID       Address
    );

HCELL_INDEX
HvAllocateCell(
    PHHIVE          Hive,
    ULONG           NewSize,
    HSTORAGE_TYPE   Type
    );

VOID
HvFreeCell(
    PHHIVE      Hive,
    HCELL_INDEX Cell
    );

HCELL_INDEX
HvReallocateCell(
    PHHIVE      Hive,
    HCELL_INDEX Cell,
    ULONG       NewSize
    );

BOOLEAN
HvIsCellAllocated(
    PHHIVE      Hive,
    HCELL_INDEX Cell
    );

VOID
HvFreeHive(
    PHHIVE Hive
    );

VOID
HvFreeHivePartial(
    PHHIVE      Hive,
    HCELL_INDEX Start,
    HSTORAGE_TYPE Type
    );

#endif // _HIVE_
