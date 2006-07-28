/*
 * PROJECT:   registry manipulation library
 * LICENSE:   GPL - See COPYING in the top level directory
 * COPYRIGHT: Copyright 2005 Filip Navara <navaraf@reactos.org>
 *            Copyright 2001 - 2005 Eric Kohl
 */

#ifndef CMLIB_H
#define CMLIB_H

#include <ddk/ntddk.h>
#include "hivedata.h"
#include "cmdata.h"

#ifndef ROUND_UP
#define ROUND_UP(a,b)        ((((a)+(b)-1)/(b))*(b))
#define ROUND_DOWN(a,b)      (((a)/(b))*(b))
#endif

#define CMAPI

typedef struct _BLOCK_LIST_ENTRY
{
   PHBIN Bin;
   PVOID Block;
} BLOCK_LIST_ENTRY, *PBLOCK_LIST_ENTRY;

struct _REGISTRY_HIVE;

typedef PVOID (CMAPI *PHV_ALLOCATE)(
   ULONG Size,
   BOOLEAN Paged);

typedef VOID (CMAPI *PHV_FREE)(
   PVOID Ptr);

typedef BOOLEAN (CMAPI *PHV_FILE_READ)(
   struct _REGISTRY_HIVE *RegistryHive,
   ULONG FileType,
   ULONG FileOffset,
   PVOID Buffer,
   ULONG BufferLength);

typedef BOOLEAN (CMAPI *PHV_FILE_WRITE)(
   struct _REGISTRY_HIVE *RegistryHive,
   ULONG FileType,
   ULONG FileOffset,
   PVOID Buffer,
   ULONG BufferLength);

typedef BOOLEAN (CMAPI *PHV_FILE_SET_SIZE)(
   struct _REGISTRY_HIVE *RegistryHive,
   ULONG FileType,
   ULONG FileSize);

typedef BOOLEAN (CMAPI *PHV_FILE_FLUSH)(
   struct _REGISTRY_HIVE *RegistryHive,
   ULONG FileType);

typedef struct _REGISTRY_HIVE
{
   PHIVE_HEADER HiveHeader;
   BOOLEAN ReadOnly;
   BOOLEAN Flat;
   RTL_BITMAP DirtyBitmap;
   struct
   {
      ULONG BlockListSize;
      PBLOCK_LIST_ENTRY BlockList;
      HCELL_INDEX FreeListOffset[24];
   } Storage[HvMaxStorageType];

   PHV_ALLOCATE Allocate;
   PHV_FREE Free;
   PHV_FILE_READ FileRead;
   PHV_FILE_WRITE FileWrite;
   PHV_FILE_SET_SIZE FileSetSize;
   PHV_FILE_FLUSH FileFlush;
   PVOID Opaque;
} REGISTRY_HIVE, *PREGISTRY_HIVE;

/*
 * Public functions.
 */

#define HV_OPERATION_CREATE_HIVE    1
#define HV_OPERATION_MEMORY         2
#define HV_OPERATION_MEMORY_INPLACE 3

NTSTATUS CMAPI
HvInitialize(
   PREGISTRY_HIVE *RegistryHive,
   ULONG Operation,
   ULONG_PTR ChunkBase,
   SIZE_T ChunkSize,
   PHV_ALLOCATE Allocate,
   PHV_FREE Free,
   PHV_FILE_READ FileRead,
   PHV_FILE_WRITE FileWrite,
   PHV_FILE_SET_SIZE FileSetSize,
   PHV_FILE_FLUSH FileFlush,
   PVOID Opaque);

VOID CMAPI 
HvFree(
   PREGISTRY_HIVE RegistryHive);

PVOID CMAPI
HvGetCell(
   PREGISTRY_HIVE RegistryHive,
   HCELL_INDEX CellOffset);

LONG CMAPI
HvGetCellSize(
   PREGISTRY_HIVE RegistryHive,
   PVOID Cell);

HCELL_INDEX CMAPI
HvAllocateCell(
   PREGISTRY_HIVE RegistryHive,
   ULONG Size,
   HV_STORAGE_TYPE Storage);

HCELL_INDEX CMAPI
HvReallocateCell(
   PREGISTRY_HIVE RegistryHive,
   HCELL_INDEX CellOffset,
   ULONG Size);

VOID CMAPI
HvFreeCell(
   PREGISTRY_HIVE RegistryHive,
   HCELL_INDEX CellOffset);

VOID CMAPI
HvMarkCellDirty(
   PREGISTRY_HIVE RegistryHive,
   HCELL_INDEX CellOffset);

BOOLEAN CMAPI
HvSyncHive(
   PREGISTRY_HIVE RegistryHive);

BOOLEAN CMAPI
HvWriteHive(
   PREGISTRY_HIVE RegistryHive);

BOOLEAN CMAPI
CmCreateRootNode(
   PREGISTRY_HIVE Hive,
   PCWSTR Name);

VOID CMAPI
CmPrepareHive(
   PREGISTRY_HIVE RegistryHive);

/*
 * Private functions.
 */

PHBIN CMAPI
HvpAddBin(
   PREGISTRY_HIVE RegistryHive,
   ULONG Size,
   HV_STORAGE_TYPE Storage);

NTSTATUS CMAPI
HvpCreateHiveFreeCellList(
   PREGISTRY_HIVE Hive);

ULONG CMAPI
HvpHiveHeaderChecksum(
   PHIVE_HEADER HiveHeader);

#endif /* CMLIB_H */
