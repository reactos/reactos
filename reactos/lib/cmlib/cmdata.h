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

#ifndef _CM_

#ifdef CMLIB_HOST
#include <host/pshpack1.h>
#else
#include <pshpack1.h>
#endif

typedef struct _CM_VIEW_OF_FILE
{
    LIST_ENTRY LRUViewList;
    LIST_ENTRY PinViewList;
    ULONG FileOffset;
    ULONG Size;
    PULONG ViewAddress;
    PVOID Bcb;
    ULONG UseCount;
} CM_VIEW_OF_FILE, *PCM_VIEW_OF_FILE;

typedef struct _CHILD_LIST
{
    ULONG Count;
    HCELL_INDEX List;
} CHILD_LIST, *PCHILD_LIST;

typedef struct _CM_KEY_NODE
{
   /* Key cell identifier "kn" (0x6b6e) */
   USHORT Id;

   /* Flags */
   USHORT Flags;

   /* Time of last flush */
   LARGE_INTEGER LastWriteTime;

   ULONG Spare;

   /* BlockAddress offset of parent key cell */
   HCELL_INDEX Parent;

   /* Count of sub keys for the key in this key cell (stable & volatile) */
   ULONG SubKeyCounts[HvMaxStorageType];

   /* BlockAddress offset of has table for FIXME: subkeys/values? (stable & volatile) */
   HCELL_INDEX SubKeyLists[HvMaxStorageType];

   CHILD_LIST ValueList;

   /* BlockAddress offset of security cell */
   HCELL_INDEX SecurityKeyOffset;

   /* BlockAddress offset of registry key class */
   HCELL_INDEX ClassNameOffset;

   ULONG MaxNameLen;
   ULONG MaxClassLen;
   ULONG MaxValueNameLen;
   ULONG MaxValueDataLen;
   ULONG WorkVar;

   /* Size in bytes of key name */
   USHORT NameSize;

   /* Size of class name in bytes */
   USHORT ClassSize;

   /* Name of key (not zero terminated) */
   UCHAR Name[0];
} CM_KEY_NODE, *PCM_KEY_NODE;

/* CM_KEY_NODE.Flags constants */
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

typedef struct _CM_KEY_VALUE
{
  USHORT Id;	// "kv"
  USHORT NameSize;	// length of Name
  ULONG  DataSize;	// length of datas in the cell pointed by DataOffset
  HCELL_INDEX  DataOffset;// datas are here if high bit of DataSize is set
  ULONG  DataType;
  USHORT Flags;
  USHORT Unused1;
  UCHAR  Name[0]; /* warning : not zero terminated */
} CM_KEY_VALUE, *PCM_KEY_VALUE;

/* CM_KEY_VALUE.Flags constants */
#define REG_VALUE_NAME_PACKED             0x0001

/* CM_KEY_VALUE.DataSize mask constants */
#define REG_DATA_SIZE_MASK                 0x7FFFFFFF
#define REG_DATA_IN_OFFSET                 0x80000000

typedef struct _CM_KEY_SECURITY
{
    USHORT Signature; // "sk"
    USHORT Reserved;
    HCELL_INDEX Flink;
    HCELL_INDEX Blink;
    ULONG ReferenceCount;
    ULONG DescriptorLength;
    //SECURITY_DESCRIPTOR_RELATIVE Descriptor;
    UCHAR Data[0];
} CM_KEY_SECURITY, *PCM_KEY_SECURITY;

#ifdef CMLIB_HOST
#include <host/poppack.h>
#else
#include <poppack.h>
#endif

#endif

#endif /* CMLIB_CMDATA_H */
