/*
 * PROJECT:   registry manipulation library
 * LICENSE:   GPL - See COPYING in the top level directory
 * COPYRIGHT: Copyright 2005 Filip Navara <navaraf@reactos.org>
 *            Copyright 2001 - 2005 Eric Kohl
 */

#ifndef CMLIB_CMDATA_H
#define CMLIB_CMDATA_H

#define  REG_INIT_BLOCK_LIST_SIZE      32
#define  REG_INIT_HASH_TABLE_SIZE      3
#define  REG_EXTEND_HASH_TABLE_SIZE    4
#define  REG_VALUE_LIST_CELL_MULTIPLE  4

#define  REG_KEY_CELL_ID               0x6b6e
#define  REG_HASH_TABLE_CELL_ID        0x666c
#define  REG_VALUE_CELL_ID             0x6b76
#define  REG_SECURITY_CELL_ID          0x6b73

#include <pshpack1.h>

typedef struct _KEY_CELL
{
   /* Key cell identifier "kn" (0x6b6e) */
   USHORT Id;

   /* Flags */
   USHORT Flags;

   /* Time of last flush */
   LARGE_INTEGER LastWriteTime;

   /* ? */
   ULONG UnUsed1;

   /* Block offset of parent key cell */
   HCELL_INDEX ParentKeyOffset;

   /* Count of sub keys for the key in this key cell (stable & volatile) */
   ULONG NumberOfSubKeys[HvMaxStorageType];

   /* Block offset of has table for FIXME: subkeys/values? (stable & volatile) */
   HCELL_INDEX HashTableOffset[HvMaxStorageType];

   /* Count of values contained in this key cell */
   ULONG NumberOfValues;

   /* Block offset of VALUE_LIST_CELL */
   HCELL_INDEX ValueListOffset;

   /* Block offset of security cell */
   HCELL_INDEX SecurityKeyOffset;

   /* Block offset of registry key class */
   HCELL_INDEX ClassNameOffset;

   /* ? */
   ULONG Unused4[5];

   /* Size in bytes of key name */
   USHORT NameSize;

   /* Size of class name in bytes */
   USHORT ClassSize;

   /* Name of key (not zero terminated) */
   UCHAR Name[0];
} KEY_CELL, *PKEY_CELL;

/* KEY_CELL.Flags constants */
#define  REG_KEY_VOLATILE_CELL             0x01
#define  REG_KEY_ROOT_CELL                 0x0C
#define  REG_KEY_LINK_CELL                 0x10
#define  REG_KEY_NAME_PACKED               0x20

/*
 * Hash record
 *
 * HashValue:
 *	packed name: four letters of value's name
 *	otherwise: Zero!
 */
typedef struct _HASH_RECORD
{
  HCELL_INDEX  KeyOffset;
  ULONG  HashValue;
} HASH_RECORD, *PHASH_RECORD;

typedef struct _HASH_TABLE_CELL
{
  USHORT  Id;
  USHORT  HashTableSize;
  HASH_RECORD  Table[0];
} HASH_TABLE_CELL, *PHASH_TABLE_CELL;

typedef struct _VALUE_LIST_CELL
{
  HCELL_INDEX  ValueOffset[0];
} VALUE_LIST_CELL, *PVALUE_LIST_CELL;

typedef struct _VALUE_CELL
{
  USHORT Id;	// "kv"
  USHORT NameSize;	// length of Name
  ULONG  DataSize;	// length of datas in the cell pointed by DataOffset
  HCELL_INDEX  DataOffset;// datas are here if high bit of DataSize is set
  ULONG  DataType;
  USHORT Flags;
  USHORT Unused1;
  UCHAR  Name[0]; /* warning : not zero terminated */
} VALUE_CELL, *PVALUE_CELL;

/* VALUE_CELL.Flags constants */
#define REG_VALUE_NAME_PACKED             0x0001

/* VALUE_CELL.DataSize mask constants */
#define REG_DATA_SIZE_MASK                 0x7FFFFFFF
#define REG_DATA_IN_OFFSET                 0x80000000

typedef struct _SECURITY_CELL
{
  USHORT Id;	// "sk"
  USHORT Reserved;
  HCELL_INDEX PrevSecurityCell;
  HCELL_INDEX NextSecurityCell;
  ULONG RefCount;
  ULONG SdSize;
  UCHAR Data[0];
} SECURITY_CELL, *PSECURITY_CELL;

#include <poppack.h>

#endif /* CMLIB_CMDATA_H */
