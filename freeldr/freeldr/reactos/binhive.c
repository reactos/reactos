/*
 *  FreeLoader
 *
 *  Copyright (C) 2001  Rex Jolliff
 *  Copyright (C) 2001  Eric Kohl
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <freeldr.h>
#include <rtl.h>
#include <mm.h>
#include <debug.h>

#include "registry.h"


#define  REG_HIVE_ID                   0x66676572 /* "regf" */
#define  REG_BIN_ID                    0x6e696268 /* "hbin" */
#define  REG_KEY_CELL_ID               0x6b6e
#define  REG_HASH_TABLE_BLOCK_ID       0x666c
#define  REG_VALUE_CELL_ID             0x6b76

#define  REG_HBIN_DATA_OFFSET          32


/* BLOCK_OFFSET = offset in file after header block */
typedef U32 BLOCK_OFFSET;

/* header for registry hive file : */
typedef struct _HIVE_HEADER
{
  /* Hive identifier "regf" (0x66676572) */
  U32  BlockId;

  /* Update counter */
  U32  UpdateCounter1;

  /* Update counter */
  U32  UpdateCounter2;

  /* When this hive file was last modified */
  U64  DateModified;	/* FILETIME */

  /* Registry format version ? (1?) */
  U32  Unused3;

  /* Registry format version ? (3?) */
  U32  Unused4;

  /* Registry format version ? (0?) */
  U32  Unused5;

  /* Registry format version ? (1?) */
  U32  Unused6;

  /* Offset into file from the byte after the end of the base block.
     If the hive is volatile, this is the actual pointer to the KEY_CELL */
  BLOCK_OFFSET  RootKeyCell;

  /* Size of each hive block ? */
  U32  BlockSize;

  /* (1?) */
  U32  Unused7;

  /* Name of hive file */
  WCHAR  FileName[64];

  /* ? */
  U32  Unused8[83];

  /* Checksum of first 0x200 bytes */
  U32  Checksum;
} __attribute__((packed)) HIVE_HEADER, *PHIVE_HEADER;

typedef struct _HBIN
{
  /* Bin identifier "hbin" (0x6E696268) */
  U32  BlockId;

  /* Block offset of this bin */
  BLOCK_OFFSET  BlockOffset;

  /* Size in bytes, multiple of the block size (4KB) */
  U32  BlockSize;

  /* ? */
  U32  Unused1;

  /* When this bin was last modified */
  U64  DateModified;		/* FILETIME */

  /* ? */
  U32  Unused2;
} __attribute__((packed)) HBIN, *PHBIN;


typedef struct _CELL_HEADER
{
  /* <0 if used, >0 if free */
  S32  CellSize;
} __attribute__((packed)) CELL_HEADER, *PCELL_HEADER;


typedef struct _KEY_CELL
{
  /* Size of this cell */
  S32  CellSize;

  /* Key cell identifier "kn" (0x6b6e) */
  U16  Id;

  /* ? */
  U16  Type;

  /* Time of last flush */
  U64  LastWriteTime;		/* FILETIME */

  /* ? */
  U32  UnUsed1;

  /* Block offset of parent key cell */
  BLOCK_OFFSET  ParentKeyOffset;

  /* Count of sub keys for the key in this key cell */
  U32  NumberOfSubKeys;

  /* ? */
  U32  UnUsed2;

  /* Block offset of has table for FIXME: subkeys/values? */
  BLOCK_OFFSET  HashTableOffset;

  /* ? */
  U32  UnUsed3;

  /* Count of values contained in this key cell */
  U32  NumberOfValues;

  /* Block offset of VALUE_LIST_CELL */
  BLOCK_OFFSET  ValuesOffset;

  /* Block offset of security cell */
  BLOCK_OFFSET  SecurityKeyOffset;

  /* Block offset of registry key class */
  BLOCK_OFFSET  ClassNameOffset;

  /* ? */
  U32  Unused4[5];

  /* Size in bytes of key name */
  U16 NameSize;

  /* Size of class name in bytes */
  U16 ClassSize;

  /* Name of key (not zero terminated) */
  U8  Name[0];
} __attribute__((packed)) KEY_CELL, *PKEY_CELL;


// hash record :
// HashValue=four letters of value's name
typedef struct _HASH_RECORD
{
  BLOCK_OFFSET  KeyOffset;
  U32  HashValue;
} __attribute__((packed)) HASH_RECORD, *PHASH_RECORD;


typedef struct _HASH_TABLE_CELL
{
  S32  CellSize;
  U16  Id;
  U16  HashTableSize;
  HASH_RECORD  Table[0];
} __attribute__((packed)) HASH_TABLE_CELL, *PHASH_TABLE_CELL;


typedef struct _VALUE_LIST_CELL
{
  S32  CellSize;
  BLOCK_OFFSET  Values[0];
} __attribute__((packed)) VALUE_LIST_CELL, *PVALUE_LIST_CELL;


typedef struct _VALUE_CELL
{
  S32  CellSize;
  U16  Id;	// "kv"
  U16  NameSize;	// length of Name
  S32  DataSize;	// length of datas in the cell pointed by DataOffset
  BLOCK_OFFSET  DataOffset;// datas are here if high bit of DataSize is set
  U32  DataType;
  U16  Flags;
  U16 Unused1;
  UCHAR  Name[0]; /* warning : not zero terminated */
} __attribute__((packed)) VALUE_CELL, *PVALUE_CELL;

/* VALUE_CELL.Flags constants */
#define REG_VALUE_NAME_PACKED             0x0001


typedef struct _DATA_CELL
{
  S32  CellSize;
  UCHAR  Data[0];
} __attribute__((packed)) DATA_CELL, *PDATA_CELL;


/* FUNCTIONS ****************************************************************/

static BOOL
RegImportValue (PHBIN RootBin,
		PVALUE_CELL ValueCell,
		HKEY Key)
{
  PDATA_CELL DataCell;
  PWCHAR wName;
  PCHAR cName;
  S32 Error;
  S32 DataSize;
  PCHAR cBuffer;
  PWCHAR wBuffer;
  S32 i;

  if (ValueCell->CellSize >= 0 || ValueCell->Id != REG_VALUE_CELL_ID)
    {
      DbgPrint((DPRINT_REGISTRY, "Invalid key cell!\n"));
      return FALSE;
    }

  if (ValueCell->Flags & REG_VALUE_NAME_PACKED)
    {
      cName = MmAllocateMemory (ValueCell->NameSize + 1);
      memcpy (cName,
	      ValueCell->Name,
	      ValueCell->NameSize);
      cName[ValueCell->NameSize] = 0;
    }
  else
    {
      wName = (PWCHAR)ValueCell->Name;
      cName = MmAllocateMemory (ValueCell->NameSize / 2 + 1);
      for (i = 0; i < ValueCell->NameSize / 2; i++)
	cName[i] = (CHAR)wName[i];
      cName[ValueCell->NameSize / 2] = 0;
    }

  DataSize = ValueCell->DataSize & 0x7FFFFFFF;

  DbgPrint((DPRINT_REGISTRY, "ValueName: '%s'\n", cName));
  DbgPrint((DPRINT_REGISTRY, "DataSize: %u\n", DataSize));

  if (DataSize <= 4 && (ValueCell->DataSize & 0x80000000))
    {
      Error = RegSetValue(Key,
			  cName,
			  ValueCell->DataType,
			  (PUCHAR)&ValueCell->DataOffset,
			  DataSize);
      if (Error != ERROR_SUCCESS)
	{
	  DbgPrint((DPRINT_REGISTRY, "RegSetValue() failed!\n"));
	  MmFreeMemory (cName);
	  return FALSE;
	}
    }
  else
    {
      DataCell = (PDATA_CELL)((PUCHAR)RootBin + ValueCell->DataOffset);
      DbgPrint((DPRINT_REGISTRY, "DataCell: %x\n", DataCell));

      if (DataCell->CellSize >= 0)
	{
	  DbgPrint((DPRINT_REGISTRY, "Invalid data cell size!\n"));
	  MmFreeMemory (cName);
	  return FALSE;
	}

      if (ValueCell->DataType == REG_SZ ||
	  ValueCell->DataType == REG_EXPAND_SZ ||
	  ValueCell->DataType == REG_MULTI_SZ)
	{
	  wBuffer = (PWCHAR)DataCell->Data;
	  cBuffer = MmAllocateMemory(DataSize/2);
	  for (i = 0; i < DataSize / 2; i++)
	    cBuffer[i] = (CHAR)wBuffer[i];

	  Error = RegSetValue (Key,
			       cName,
			       ValueCell->DataType,
			       cBuffer,
			       DataSize/2);

	  MmFreeMemory(cBuffer);
	}
      else
	{
	  Error = RegSetValue (Key,
			       cName,
			       ValueCell->DataType,
			       (PUCHAR)DataCell->Data,
			       DataSize);
	}
      if (Error != ERROR_SUCCESS)
	{
	  DbgPrint((DPRINT_REGISTRY, "RegSetValue() failed!\n"));
	  MmFreeMemory (cName);
	  return FALSE;
	}
    }

  MmFreeMemory (cName);

  return TRUE;
}


static BOOL
RegImportSubKey(PHBIN RootBin,
		PKEY_CELL KeyCell,
		HKEY ParentKey)
{
  PHASH_TABLE_CELL HashCell;
  PKEY_CELL SubKeyCell;
  PVALUE_LIST_CELL ValueListCell;
  PVALUE_CELL ValueCell = NULL;
  PCHAR cName;
  HKEY SubKey;
  S32 Error;
  U32 i;


  DbgPrint((DPRINT_REGISTRY, "KeyCell: %x\n", KeyCell));
  DbgPrint((DPRINT_REGISTRY, "KeyCell->CellSize: %x\n", KeyCell->CellSize));
  DbgPrint((DPRINT_REGISTRY, "KeyCell->Id: %x\n", KeyCell->Id));
  if (KeyCell->Id != REG_KEY_CELL_ID || KeyCell->CellSize >= 0)
    {
      DbgPrint((DPRINT_REGISTRY, "Invalid key cell id!\n"));
      return FALSE;
    }

  /* FIXME: implement packed key names */
  cName = MmAllocateMemory (KeyCell->NameSize + 1);
  memcpy (cName,
	  KeyCell->Name,
	  KeyCell->NameSize);
  cName[KeyCell->NameSize] = 0;

  DbgPrint((DPRINT_REGISTRY, "KeyName: '%s'\n", cName));

  /* Create new sub key */
  Error = RegCreateKey (ParentKey,
			cName,
			&SubKey);
  MmFreeMemory (cName);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_REGISTRY, "RegCreateKey() failed!\n"));
      return FALSE;
    }
  DbgPrint((DPRINT_REGISTRY, "Subkeys: %u\n", KeyCell->NumberOfSubKeys));
  DbgPrint((DPRINT_REGISTRY, "Values: %u\n", KeyCell->NumberOfValues));

  /* Enumerate and add values */
  if (KeyCell->NumberOfValues > 0)
    {
      ValueListCell = (PVALUE_LIST_CELL)((PUCHAR)RootBin + KeyCell->ValuesOffset);
      DbgPrint((DPRINT_REGISTRY, "ValueListCell: %x\n", ValueListCell));

      for (i = 0; i < KeyCell->NumberOfValues; i++)
	{
	  DbgPrint((DPRINT_REGISTRY, "ValueOffset[%d]: %x\n", i, ValueListCell->Values[i]));

	  ValueCell = (PVALUE_CELL)((PUCHAR)RootBin + ValueListCell->Values[i]);

	  DbgPrint((DPRINT_REGISTRY, "ValueCell[%d]: %x\n", i, ValueCell));

	  if (!RegImportValue(RootBin, ValueCell, SubKey))
	    return FALSE;
	}
    }

  /* Enumerate and add subkeys */
  if (KeyCell->NumberOfSubKeys > 0)
    {
      HashCell = (PHASH_TABLE_CELL)((PUCHAR)RootBin + KeyCell->HashTableOffset);
      DbgPrint((DPRINT_REGISTRY, "HashCell: %x\n", HashCell));

      for (i = 0; i < KeyCell->NumberOfSubKeys; i++)
	{
	  DbgPrint((DPRINT_REGISTRY, "KeyOffset[%d]: %x\n", i, HashCell->Table[i].KeyOffset));

	  SubKeyCell = (PKEY_CELL)((PUCHAR)RootBin + HashCell->Table[i].KeyOffset);

	  DbgPrint((DPRINT_REGISTRY, "SubKeyCell[%d]: %x\n", i, SubKeyCell));

	  if (!RegImportSubKey(RootBin, SubKeyCell, SubKey))
	    return FALSE;
	}
    }

  return TRUE;
}


BOOL
RegImportBinaryHive(PCHAR ChunkBase,
		    U32 ChunkSize)
{
  PHIVE_HEADER HiveHeader;
  PHBIN RootBin;
  PKEY_CELL KeyCell;
  PHASH_TABLE_CELL HashCell;
  PKEY_CELL SubKeyCell;
  HKEY SystemKey;
  U32 i;
  S32 Error;

  DbgPrint((DPRINT_REGISTRY, "RegImportBinaryHive(%x, %u) called\n",ChunkBase,ChunkSize));

  HiveHeader = (PHIVE_HEADER)ChunkBase;
  DbgPrint((DPRINT_REGISTRY, "HiveHeader: %x\n", HiveHeader));
  if (HiveHeader->BlockId != REG_HIVE_ID)
    {
      DbgPrint((DPRINT_REGISTRY, "Invalid hive id!\n"));
      return FALSE;
    }

  RootBin = (PHBIN)((PUCHAR)HiveHeader + 0x1000);
  DbgPrint((DPRINT_REGISTRY, "RootBin: %x\n", RootBin));
  if (RootBin->BlockId != REG_BIN_ID || RootBin->BlockSize == 0)
    {
      DbgPrint((DPRINT_REGISTRY, "Invalid bin id!\n"));
      return FALSE;
    }

  KeyCell = (PKEY_CELL)((PUCHAR)RootBin + REG_HBIN_DATA_OFFSET);
  DbgPrint((DPRINT_REGISTRY, "KeyCell: %x\n", KeyCell));
  DbgPrint((DPRINT_REGISTRY, "KeyCell->CellSize: %x\n", KeyCell->CellSize));
  DbgPrint((DPRINT_REGISTRY, "KeyCell->Id: %x\n", KeyCell->Id));
  if (KeyCell->Id != REG_KEY_CELL_ID || KeyCell->CellSize >= 0)
    {
      DbgPrint((DPRINT_REGISTRY, "Invalid key cell id!\n"));
      return FALSE;
    }

  DbgPrint((DPRINT_REGISTRY, "Subkeys: %u\n", KeyCell->NumberOfSubKeys));
  DbgPrint((DPRINT_REGISTRY, "Values: %u\n", KeyCell->NumberOfValues));

  /* Open 'System' key */
  Error = RegOpenKey(NULL,
		     "\\Registry\\Machine\\SYSTEM",
		     &SystemKey);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_REGISTRY, "Failed to open 'system' key!\n"));
      return FALSE;
    }

  /* Enumerate and add subkeys */
  if (KeyCell->NumberOfSubKeys > 0)
    {
      HashCell = (PHASH_TABLE_CELL)((PUCHAR)RootBin + KeyCell->HashTableOffset);
      DbgPrint((DPRINT_REGISTRY, "HashCell: %x\n", HashCell));

      for (i = 0; i < KeyCell->NumberOfSubKeys; i++)
	{
	  DbgPrint((DPRINT_REGISTRY, "KeyOffset[%d]: %x\n", i, HashCell->Table[i].KeyOffset));

	  SubKeyCell = (PKEY_CELL)((PUCHAR)RootBin + HashCell->Table[i].KeyOffset);

	  DbgPrint((DPRINT_REGISTRY, "SubKeyCell[%d]: %x\n", i, SubKeyCell));

	  if (!RegImportSubKey(RootBin, SubKeyCell, SystemKey))
	    return FALSE;
	}
    }

  return TRUE;
}

#if 0
BOOL
RegExportHive(PCHAR ChunkBase, U32* ChunkSize)
{
  return(TRUE);
}
#endif

/* EOF */
