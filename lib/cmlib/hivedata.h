/*
 * PROJECT:   registry manipulation library
 * LICENSE:   GPL - See COPYING in the top level directory
 * COPYRIGHT: Copyright 2005 Filip Navara <navaraf@reactos.org>
 *            Copyright 2001 - 2005 Eric Kohl
 */

#ifndef CMLIB_HIVEDATA_H
#define CMLIB_HIVEDATA_H

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
#define HIVE_HAS_BEEN_REPLACED                          4
#define HIVE_HAS_BEEN_FREED                             8
#define HIVE_UNKNOWN                                    0x10
#define HIVE_IS_UNLOADING                               0x20

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

#define HV_BLOCK_SIZE                  4096
#define HV_LOG_HEADER_SIZE             FIELD_OFFSET(HBASE_BLOCK, Reserved2)
#define HV_SIGNATURE                   0x66676572
#define HV_BIN_SIGNATURE               0x6e696268

//
// Hive versions
//
#define HSYS_MAJOR                                      1
#define HSYS_MINOR                                      3
#define HSYS_WHISTLER_BETA1                             4
#define HSYS_WHISTLER                                   5
#define HSYS_MINOR_SUPPORTED                            HSYS_WHISTLER

//
// Hive formats
//
#define HBASE_FORMAT_MEMORY                             1

//
// Hive storage
//
#define HTYPE_COUNT 2

/**
 * @name HCELL_INDEX
 *
 * A handle to cell index. The highest bit specifies the cell storage and
 * the other bits specify index into the hive file. The value HCELL_NULL
 * (-1) is reserved for marking invalid cells.
 */
typedef ULONG HCELL_INDEX, *PHCELL_INDEX;

//
// Cell Magic Values
//
#define HCELL_NIL                                       -1
#define HCELL_CACHED                                    1

#define HCELL_TYPE_MASK                0x80000000
#define HCELL_BLOCK_MASK               0x7ffff000
#define HCELL_OFFSET_MASK              0x00000fff
#define HCELL_TYPE_SHIFT               31
#define HCELL_BLOCK_SHIFT              12
#define HCELL_OFFSET_SHIFT             0

#define HvGetCellType(Cell)             \
    ((ULONG)((Cell & HCELL_TYPE_MASK) >> HCELL_TYPE_SHIFT))
#define HvGetCellBlock(Cell)            \
    ((ULONG)((Cell & HCELL_BLOCK_MASK) >> HCELL_BLOCK_SHIFT))

typedef enum
{
    Stable = 0,
    Volatile = 1
} HSTORAGE_TYPE;

#ifdef CMLIB_HOST
#include <host/pshpack1.h>
#else
#include <pshpack1.h>
#endif

/**
 * @name HBASE_BLOCK
 *
 * On-disk header for registry hive file.
 */

typedef struct _HBASE_BLOCK
{
   /* Hive identifier "regf" (0x66676572) */
   ULONG Signature;

   /* Update counter */
   ULONG Sequence1;

   /* Update counter */
   ULONG Sequence2;

   /* When this hive file was last modified */
   LARGE_INTEGER TimeStamp;

   /* Registry format major version (1) */
   ULONG Major;

   /* Registry format minor version (3)
      Version 3 added fast indexes, version 5 has large value optimizations */
   ULONG Minor;

   /* Registry file type (0 - Primary, 1 - Log) */
   ULONG Type;

   /* Registry format (1 is the only defined value so far) */
   ULONG Format;

   /* Offset into file from the byte after the end of the base block.
      If the hive is volatile, this is the actual pointer to the CM_KEY_NODE */
   HCELL_INDEX RootCell;

   /* Size of each hive block ? */
   ULONG Length;

   /* (1?) */
   ULONG Cluster;

   /* Name of hive file */
   CHAR FileName[64];

   ULONG Reserved1[99];

   /* Checksum of first 0x200 bytes */
   ULONG CheckSum;

   ULONG Reserved2[0x37E];
   ULONG BootType;
   ULONG BootRecover;
} HBASE_BLOCK, *PHBASE_BLOCK;

typedef struct _HBIN
{
   /* Bin identifier "hbin" (0x6E696268) */
   ULONG Signature;

   /* Block offset of this bin */
   HCELL_INDEX FileOffset;

   /* Size in bytes, multiple of the block size (4KB) */
   ULONG Size;

   ULONG Reserved1[2];

   /* When this bin was last modified */
   LARGE_INTEGER TimeStamp;

   /* ? (In-memory only) */
   ULONG Spare;
} HBIN, *PHBIN;

typedef struct _HCELL
{
   /* <0 if used, >0 if free */
   LONG Size;
} HCELL, *PHCELL;

#ifdef CMLIB_HOST
#include <host/poppack.h>
#else
#include <poppack.h>
#endif

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
    ULONG Length
);

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

#define IsFreeCell(Cell)(Cell->Size >= 0)
#define IsUsedCell(Cell)(Cell->Size < 0)

#endif /* CMLIB_HIVEDATA_H */
