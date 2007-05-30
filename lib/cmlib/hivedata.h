/*
 * PROJECT:   registry manipulation library
 * LICENSE:   GPL - See COPYING in the top level directory
 * COPYRIGHT: Copyright 2005 Filip Navara <navaraf@reactos.org>
 *            Copyright 2001 - 2005 Eric Kohl
 */

#ifndef CMLIB_HIVEDATA_H
#define CMLIB_HIVEDATA_H

#define HV_BLOCK_SIZE                  4096
#define HV_LOG_HEADER_SIZE             FIELD_OFFSET(HBASE_BLOCK, Reserved2)
#define HV_SIGNATURE                   0x66676572
#define HV_BIN_SIGNATURE               0x6e696268

#define HV_MAJOR_VER                   1
#define HV_MINOR_VER                   3
#define HV_FORMAT_MEMORY               1

#define HV_TYPE_PRIMARY                0
#define HV_TYPE_ALTERNATE              1
#define HV_TYPE_LOG                    2
#define HV_TYPE_EXTERNAL               3
#define HV_TYPE_MAX                    4

/**
 * @name HCELL_INDEX
 *
 * A handle to cell index. The highest bit specifies the cell storage and
 * the other bits specify index into the hive file. The value HCELL_NULL
 * (-1) is reserved for marking invalid cells.
 */
typedef ULONG HCELL_INDEX, *PHCELL_INDEX;

#define HCELL_NULL                     ((HCELL_INDEX)-1)
#define HCELL_TYPE_MASK                0x80000000
#define HCELL_BLOCK_MASK               0x7ffff000
#define HCELL_OFFSET_MASK              0x00000fff
#define HCELL_TYPE_SHIFT               31
#define HCELL_BLOCK_SHIFT              12
#define HCELL_OFFSET_SHIFT             0

#define HvGetCellType(Cell)             \
    ((ULONG)((Cell & HCELL_TYPE_MASK) >> HCELL_TYPE_SHIFT))

#include <pshpack1.h>

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
   ULONG Checksum;

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
   ULONG MemAlloc;
} HBIN, *PHBIN;

typedef struct _HCELL
{
   /* <0 if used, >0 if free */
   LONG Size;
} HCELL, *PHCELL;

#include <poppack.h>

#define IsFreeCell(Cell)(Cell->Size >= 0)
#define IsUsedCell(Cell)(Cell->Size < 0)

typedef enum _HV_STORAGE_TYPE
{
   HvStable = 0,
   HvVolatile,
   HvMaxStorageType
} HV_STORAGE_TYPE;

#endif /* CMLIB_HIVEDATA_H */
