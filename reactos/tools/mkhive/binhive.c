/*
 *  ReactOS kernel
 *  Copyright (C) 2003 ReactOS Team
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
/* $Id: binhive.c,v 1.2 2003/04/16 15:06:33 ekohl Exp $
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS hive maker
 * FILE:            tools/mkhive/binhive.c
 * PURPOSE:         Binary hive export code
 * PROGRAMMER:      Eric Kohl
 */

/* INCLUDES *****************************************************************/

//#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "mkhive.h"
#include "binhive.h"
#include "registry.h"


#define  REG_HIVE_ID                   0x66676572
#define  REG_BIN_ID                    0x6e696268
#define  REG_KEY_CELL_ID               0x6b6e
#define  REG_HASH_TABLE_BLOCK_ID       0x666c
#define  REG_VALUE_CELL_ID             0x6b76

#define  REG_BLOCK_SIZE                4096
#define  REG_HBIN_DATA_OFFSET          32
#define  REG_INIT_BLOCK_LIST_SIZE      32
#define  REG_INIT_HASH_TABLE_SIZE      3
#define  REG_EXTEND_HASH_TABLE_SIZE    4
#define  REG_VALUE_LIST_CELL_MULTIPLE  4

#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))
#define ROUND_DOWN(N, S) ((N) - ((N) % (S)))

#define ABS_VALUE(V) (((V) < 0) ? -(V) : (V))


// BLOCK_OFFSET = offset in file after header block
typedef ULONG BLOCK_OFFSET, *PBLOCK_OFFSET;

typedef unsigned long long FILETIME;

/* header for registry hive file : */
typedef struct _HIVE_HEADER
{
  /* Hive identifier "regf" (0x66676572) */
  ULONG  BlockId;

  /* Update counter */
  ULONG  UpdateCounter1;

  /* Update counter */
  ULONG  UpdateCounter2;

  /* When this hive file was last modified */
  FILETIME  DateModified;

  /* Registry format version ? (1?) */
  ULONG  Unused3;

  /* Registry format version ? (3?) */
  ULONG  Unused4;

  /* Registry format version ? (0?) */
  ULONG  Unused5;

  /* Registry format version ? (1?) */
  ULONG  Unused6;

  /* Offset into file from the byte after the end of the base block.
     If the hive is volatile, this is the actual pointer to the KEY_CELL */
  BLOCK_OFFSET  RootKeyCell;

  /* Size of each hive block ? */
  ULONG  BlockSize;

  /* (1?) */
  ULONG  Unused7;

  /* Name of hive file */
  WCHAR  FileName[64];

  /* ? */
  ULONG  Unused8[83];

  /* Checksum of first 0x200 bytes */
  ULONG  Checksum;
} __attribute__((packed)) HIVE_HEADER, *PHIVE_HEADER;

typedef struct _HBIN
{
  /* Bin identifier "hbin" (0x6E696268) */
  ULONG  BlockId;

  /* Block offset of this bin */
  BLOCK_OFFSET  BlockOffset;

  /* Size in bytes, multiple of the block size (4KB) */
  ULONG  BlockSize;

  /* ? */
  ULONG  Unused1;

  /* When this bin was last modified */
  FILETIME  DateModified;

  /* ? */
  ULONG  Unused2;
} __attribute__((packed)) HBIN, *PHBIN;

typedef struct _CELL_HEADER
{
  /* <0 if used, >0 if free */
  LONG  CellSize;
} __attribute__((packed)) CELL_HEADER, *PCELL_HEADER;

typedef struct _KEY_CELL
{
  /* Size of this cell */
  LONG  CellSize;

  /* Key cell identifier "kn" (0x6b6e) */
  USHORT  Id;

  /* ? */
  USHORT  Type;

  /* Time of last flush */
  FILETIME  LastWriteTime;

  /* ? */
  ULONG  UnUsed1;

  /* Block offset of parent key cell */
  BLOCK_OFFSET  ParentKeyOffset;

  /* Count of sub keys for the key in this key cell */
  ULONG  NumberOfSubKeys;

  /* ? */
  ULONG  UnUsed2;

  /* Block offset of has table for FIXME: subkeys/values? */
  BLOCK_OFFSET  HashTableOffset;

  /* ? */
  ULONG  UnUsed3;

  /* Count of values contained in this key cell */
  ULONG  NumberOfValues;

  /* Block offset of VALUE_LIST_CELL */
  BLOCK_OFFSET  ValuesOffset;

  /* Block offset of security cell */
  BLOCK_OFFSET  SecurityKeyOffset;

  /* Block offset of registry key class */
  BLOCK_OFFSET  ClassNameOffset;

  /* ? */
  ULONG  Unused4[5];

  /* Size in bytes of key name */
  USHORT  NameSize;

  /* Size of class name in bytes */
  USHORT  ClassSize;

  /* Name of key (not zero terminated) */
  UCHAR  Name[0];
} __attribute__((packed)) KEY_CELL, *PKEY_CELL;

/* KEY_CELL.Type constants */
#define  REG_LINK_KEY_CELL_TYPE        0x10
#define  REG_KEY_CELL_TYPE             0x20
#define  REG_ROOT_KEY_CELL_TYPE        0x2c


// hash record :
// HashValue=four letters of value's name
typedef struct _HASH_RECORD
{
  BLOCK_OFFSET  KeyOffset;
  ULONG  HashValue;
} __attribute__((packed)) HASH_RECORD, *PHASH_RECORD;

typedef struct _HASH_TABLE_CELL
{
  LONG  CellSize;
  USHORT  Id;
  USHORT  HashTableSize;
  HASH_RECORD  Table[0];
} __attribute__((packed)) HASH_TABLE_CELL, *PHASH_TABLE_CELL;

typedef struct _VALUE_LIST_CELL
{
  LONG  CellSize;
  BLOCK_OFFSET  Values[0];
} __attribute__((packed)) VALUE_LIST_CELL, *PVALUE_LIST_CELL;

typedef struct _VALUE_CELL
{
  LONG  CellSize;
  USHORT  Id;	// "kv"
  USHORT  NameSize;	// length of Name
  LONG  DataSize;	// length of datas in the cell pointed by DataOffset
  BLOCK_OFFSET  DataOffset;// datas are here if high bit of DataSize is set
  ULONG  DataType;
  USHORT  Flags;
  USHORT  Unused1;
  UCHAR  Name[0]; /* warning : not zero terminated */
} __attribute__((packed)) VALUE_CELL, *PVALUE_CELL;

/* VALUE_CELL.Flags constants */
#define REG_VALUE_NAME_PACKED             0x0001


typedef struct _DATA_CELL
{
  LONG  CellSize;
  UCHAR  Data[0];
} __attribute__((packed)) DATA_CELL, *PDATA_CELL;

typedef struct _REGISTRY_HIVE
{
  ULONG  FileSize;
  PHIVE_HEADER  HiveHeader;
  ULONG  BlockListSize;
  PHBIN  *BlockList;
  ULONG  FreeListSize;
  ULONG  FreeListMax;
  PCELL_HEADER *FreeList;
  BLOCK_OFFSET *FreeListOffset;
} REGISTRY_HIVE, *PREGISTRY_HIVE;

/* FUNCTIONS ****************************************************************/

static VOID
CmiCreateDefaultHiveHeader(PHIVE_HEADER Header)
{
  assert(Header);
  memset (Header, 0, REG_BLOCK_SIZE);
  Header->BlockId = REG_HIVE_ID;
  Header->UpdateCounter1 = 0;
  Header->UpdateCounter2 = 0;
  Header->DateModified = 0ULL;
  Header->Unused3 = 1;
  Header->Unused4 = 3;
  Header->Unused5 = 0;
  Header->Unused6 = 1;
  Header->Unused7 = 1;
  Header->RootKeyCell = 0;
  Header->BlockSize = REG_BLOCK_SIZE;
  Header->Unused6 = 1;
  Header->Checksum = 0;
}


static VOID
CmiCreateDefaultBinCell(PHBIN BinCell)
{
  assert(BinCell);
  memset (BinCell, 0, REG_BLOCK_SIZE);
  BinCell->BlockId = REG_BIN_ID;
  BinCell->DateModified = 0ULL;
  BinCell->BlockSize = REG_BLOCK_SIZE;
}


static VOID
CmiCreateDefaultRootKeyCell(PKEY_CELL RootKeyCell)
{
  assert(RootKeyCell);
  memset (RootKeyCell, 0, sizeof(KEY_CELL));
  RootKeyCell->CellSize = -sizeof(KEY_CELL);
  RootKeyCell->Id = REG_KEY_CELL_ID;
  RootKeyCell->Type = REG_ROOT_KEY_CELL_TYPE;
  RootKeyCell->LastWriteTime = 0ULL;
  RootKeyCell->ParentKeyOffset = 0;
  RootKeyCell->NumberOfSubKeys = 0;
  RootKeyCell->HashTableOffset = -1;
  RootKeyCell->NumberOfValues = 0;
  RootKeyCell->ValuesOffset = -1;
  RootKeyCell->SecurityKeyOffset = 0;
  RootKeyCell->ClassNameOffset = -1;
  RootKeyCell->NameSize = 0;
  RootKeyCell->ClassSize = 0;
}


static PREGISTRY_HIVE
CmiCreateRegistryHive (VOID)
{
  PREGISTRY_HIVE Hive;
  PCELL_HEADER FreeCell;
  PKEY_CELL RootKeyCell;
  PHBIN BinCell;

  Hive = (PREGISTRY_HIVE) malloc (sizeof(REGISTRY_HIVE));
  if (Hive == NULL)
    {
      return NULL;
    }
  memset (Hive, 0, sizeof(REGISTRY_HIVE));

  DPRINT("Hive %x\n", Hive);

  /* Create hive beader (aka 'base block') */
  Hive->HiveHeader = (PHIVE_HEADER) malloc (REG_BLOCK_SIZE);
  if (Hive->HiveHeader == NULL)
    {
      free (Hive);
      return NULL;
    }
  CmiCreateDefaultHiveHeader(Hive->HiveHeader);
  Hive->FileSize = REG_BLOCK_SIZE;

  /* Allocate block list */
  Hive->BlockListSize = 1;
  Hive->BlockList = malloc (sizeof(PHBIN) * Hive->BlockListSize);
  if (Hive->BlockList == NULL)
    {
      free (Hive->HiveHeader);
      free (Hive);
      return NULL;
    }

  /* Allocate free cell list */
  Hive->FreeListMax = 32;
  Hive->FreeList = malloc(sizeof(PCELL_HEADER) * Hive->FreeListMax);
  if (Hive->FreeList == NULL)
    {
      free (Hive->BlockList);
      free (Hive->HiveHeader);
      free (Hive);
      return NULL;
    }
  Hive->FreeListOffset = malloc(sizeof(BLOCK_OFFSET) * Hive->FreeListMax);
  if (Hive->FreeListOffset == NULL)
    {
      free (Hive->FreeList);
      free (Hive->BlockList);
      free (Hive->HiveHeader);
      free (Hive);
      return NULL;
    }

  /* Allocate first bin */
  Hive->BlockList[0] = (PHBIN) malloc (REG_BLOCK_SIZE);
  if (Hive->BlockList[0] == NULL)
    {
      free (Hive->FreeListOffset);
      free (Hive->FreeList);
      free (Hive->BlockList);
      free (Hive->HiveHeader);
      free (Hive);
      return NULL;
    }
  Hive->FileSize += REG_BLOCK_SIZE;

  /* Init first bin */
  BinCell = (PHBIN)Hive->BlockList[0];
  CmiCreateDefaultBinCell(BinCell);
  BinCell->BlockOffset = 0;

  /* Init root key cell */
  RootKeyCell = (PKEY_CELL)((ULONG_PTR)BinCell + REG_HBIN_DATA_OFFSET);
  CmiCreateDefaultRootKeyCell(RootKeyCell);
  Hive->HiveHeader->RootKeyCell = REG_HBIN_DATA_OFFSET;

  /* Init free cell */
  FreeCell = (PCELL_HEADER)((ULONG_PTR)RootKeyCell + sizeof(KEY_CELL));
  FreeCell->CellSize = REG_BLOCK_SIZE - (REG_HBIN_DATA_OFFSET + sizeof(KEY_CELL));

  Hive->FreeList[0] = FreeCell;
  Hive->FreeListOffset[0] = REG_HBIN_DATA_OFFSET + sizeof(KEY_CELL);
  Hive->FreeListSize++;

  return Hive;
}


static VOID
CmiDestroyRegistryHive (PREGISTRY_HIVE Hive)
{
  PHBIN Bin;
  ULONG i;

  if (Hive == NULL)
    return;

  /* Release free offset list */
  if (Hive->FreeListOffset != NULL)
    free (Hive->FreeListOffset);

  /* Release free list */
  if (Hive->FreeList != NULL)
    free (Hive->FreeList);

  if (Hive->BlockList != NULL)
    {
      /* Release bins */
      Bin = NULL;
      for (i = 0; i < Hive->BlockListSize; i++)
	{
	  if ((Hive->BlockList[i] != NULL) &&
	      (Hive->BlockList[i] != Bin))
	    {
	      Bin = Hive->BlockList[i];

	      DPRINT ("Bin[%lu]: Offset 0x%lx  Size 0x%lx\n",
		      i, Bin->BlockOffset, Bin->BlockSize);

	      free (Bin);
	    }
	}

      /* Release block list */
      free (Hive->BlockList);
    }

  /* Release hive header */
  if (Hive->HiveHeader != NULL)
    free (Hive->HiveHeader);

  /* Release hive */
  free (Hive);
}


static PVOID
CmiGetBlock(PREGISTRY_HIVE Hive,
	    BLOCK_OFFSET BlockOffset,
	    PHBIN * ppBin)
{
  PHBIN pBin;

  if (ppBin)
    *ppBin = NULL;

  if ((BlockOffset == 0) || (BlockOffset == (ULONG_PTR) -1))
    return NULL;

  pBin = Hive->BlockList[BlockOffset / 4096];
  if (ppBin)
    *ppBin = pBin;

  return (PVOID)((ULONG_PTR)pBin + (BlockOffset - pBin->BlockOffset));
}


static BOOL
CmiMergeFree(PREGISTRY_HIVE RegistryHive,
	     PCELL_HEADER FreeBlock,
	     BLOCK_OFFSET FreeOffset)
{
  BLOCK_OFFSET BlockOffset;
  BLOCK_OFFSET BinOffset;
  ULONG BlockSize;
  ULONG BinSize;
  PHBIN Bin;
  ULONG i;

  DPRINT("CmiMergeFree(Block %lx  Offset %lx  Size %lx) called\n",
	 FreeBlock, FreeOffset, FreeBlock->CellSize);

  CmiGetBlock(RegistryHive,
	      FreeOffset,
	      &Bin);
  DPRINT("Bin %p\n", Bin);
  if (Bin == NULL)
    return FALSE;

  BinOffset = Bin->BlockOffset;
  BinSize = Bin->BlockSize;
  DPRINT("Bin %p  Offset %lx  Size %lx\n", Bin, BinOffset, BinSize);

  for (i = 0; i < RegistryHive->FreeListSize; i++)
    {
      BlockOffset = RegistryHive->FreeListOffset[i];
      BlockSize = RegistryHive->FreeList[i]->CellSize;
      if (BlockOffset > BinOffset &&
	  BlockOffset < BinOffset + BinSize)
	{
	  DPRINT("Free block: Offset %lx  Size %lx\n",
		  BlockOffset, BlockSize);

	  if ((i < (RegistryHive->FreeListSize - 1)) &&
	      (BlockOffset + BlockSize == FreeOffset) &&
	      (FreeOffset + FreeBlock->CellSize == RegistryHive->FreeListOffset[i + 1]))
	    {
	      DPRINT("Merge current block with previous and next block\n");

	      RegistryHive->FreeList[i]->CellSize +=
		(FreeBlock->CellSize + RegistryHive->FreeList[i + 1]->CellSize);

	      FreeBlock->CellSize = 0;
	      RegistryHive->FreeList[i + 1]->CellSize = 0;


	      if ((i + 2) < RegistryHive->FreeListSize)
		{
		  memmove (&RegistryHive->FreeListOffset[i + 1],
			   &RegistryHive->FreeListOffset[i + 2],
			   sizeof(RegistryHive->FreeListOffset[0])
			     * (RegistryHive->FreeListSize - i - 2));
		}
	      RegistryHive->FreeListSize--;

	      return TRUE;
	    }
	  else if (BlockOffset + BlockSize == FreeOffset)
	    {
	      DPRINT("Merge current block with previous block\n");

	      RegistryHive->FreeList[i]->CellSize += FreeBlock->CellSize;
	      FreeBlock->CellSize = 0;

	      return TRUE;
	    }
	  else if (FreeOffset + FreeBlock->CellSize == BlockOffset)
	    {
	      DPRINT("Merge current block with next block\n");

	      FreeBlock->CellSize += RegistryHive->FreeList[i]->CellSize;
	      RegistryHive->FreeList[i]->CellSize = 0;
	      RegistryHive->FreeList[i] = FreeBlock;
	      RegistryHive->FreeListOffset[i] = FreeOffset;

	      return TRUE;
	    }
	}
    }

  return FALSE;
}


static BOOL
CmiAddFree(PREGISTRY_HIVE RegistryHive,
	   PCELL_HEADER FreeBlock,
	   BLOCK_OFFSET FreeOffset,
	   BOOL MergeFreeBlocks)
{
  PCELL_HEADER *tmpList;
  BLOCK_OFFSET *tmpListOffset;
  LONG minInd;
  LONG maxInd;
  LONG medInd;

  assert(RegistryHive);
  assert(FreeBlock);

  DPRINT("FreeBlock %.08lx  FreeOffset %.08lx\n",
	 FreeBlock, FreeOffset);

  /* Merge free blocks */
  if (MergeFreeBlocks == TRUE)
    {
      if (CmiMergeFree(RegistryHive, FreeBlock, FreeOffset))
	return TRUE;
    }

  if ((RegistryHive->FreeListSize + 1) > RegistryHive->FreeListMax)
    {
      tmpList = malloc (sizeof(PCELL_HEADER) * (RegistryHive->FreeListMax + 32));
      if (tmpList == NULL)
	{
	  return FALSE;
	}

      tmpListOffset = malloc (sizeof(BLOCK_OFFSET) * (RegistryHive->FreeListMax + 32));
      if (tmpListOffset == NULL)
	{
	  free (tmpList);
	  return FALSE;
	}

      if (RegistryHive->FreeListMax)
	{
	  memmove (tmpList,
		   RegistryHive->FreeList,
		   sizeof(PCELL_HEADER) * (RegistryHive->FreeListMax));
	  memmove (tmpListOffset,
		   RegistryHive->FreeListOffset,
		   sizeof(BLOCK_OFFSET) * (RegistryHive->FreeListMax));
	  free (RegistryHive->FreeList);
	  free (RegistryHive->FreeListOffset);
	}
      RegistryHive->FreeList = tmpList;
      RegistryHive->FreeListOffset = tmpListOffset;
      RegistryHive->FreeListMax += 32;
    }

  /* Add new offset to free list, maintaining list in ascending order */
  if ((RegistryHive->FreeListSize == 0)
     || (RegistryHive->FreeListOffset[RegistryHive->FreeListSize-1] < FreeOffset))
    {
      /* Add to end of list */
      RegistryHive->FreeList[RegistryHive->FreeListSize] = FreeBlock;
      RegistryHive->FreeListOffset[RegistryHive->FreeListSize++] = FreeOffset;
    }
  else if (RegistryHive->FreeListOffset[0] > FreeOffset)
    {
      /* Add to begin of list */
      memmove (&RegistryHive->FreeList[1],
	       &RegistryHive->FreeList[0],
	       sizeof(RegistryHive->FreeList[0]) * RegistryHive->FreeListSize);
      memmove (&RegistryHive->FreeListOffset[1],
	       &RegistryHive->FreeListOffset[0],
	       sizeof(RegistryHive->FreeListOffset[0]) * RegistryHive->FreeListSize);
      RegistryHive->FreeList[0] = FreeBlock;
      RegistryHive->FreeListOffset[0] = FreeOffset;
      RegistryHive->FreeListSize++;
    }
  else
    {
      /* Search where to insert */
      minInd = 0;
      maxInd = RegistryHive->FreeListSize - 1;
      while ((maxInd - minInd) > 1)
	{
	  medInd = (minInd + maxInd) / 2;
	  if (RegistryHive->FreeListOffset[medInd] > FreeOffset)
	    maxInd = medInd;
	  else
	    minInd = medInd;
	}

      /* Insert before maxInd */
      memmove (&RegistryHive->FreeList[maxInd+1],
	       &RegistryHive->FreeList[maxInd],
	       sizeof(RegistryHive->FreeList[0]) * (RegistryHive->FreeListSize - minInd));
      memmove (&RegistryHive->FreeListOffset[maxInd + 1],
	       &RegistryHive->FreeListOffset[maxInd],
	       sizeof(RegistryHive->FreeListOffset[0]) * (RegistryHive->FreeListSize-minInd));
      RegistryHive->FreeList[maxInd] = FreeBlock;
      RegistryHive->FreeListOffset[maxInd] = FreeOffset;
      RegistryHive->FreeListSize++;
    }

  return TRUE;
}


static BOOL
CmiAddBin(PREGISTRY_HIVE RegistryHive,
	  PVOID *NewBlock,
	  PBLOCK_OFFSET NewBlockOffset)
{
  PCELL_HEADER tmpBlock;
  PHBIN * tmpBlockList;
  PHBIN tmpBin;

  tmpBin = malloc (REG_BLOCK_SIZE);
  if (tmpBin == NULL)
    {
      return FALSE;
    }
  memset (tmpBin, 0, REG_BLOCK_SIZE);

  tmpBin->BlockId = REG_BIN_ID;
  tmpBin->BlockOffset = RegistryHive->FileSize - REG_BLOCK_SIZE;
  RegistryHive->FileSize += REG_BLOCK_SIZE;
  tmpBin->BlockSize = REG_BLOCK_SIZE;
  tmpBin->Unused1 = 0;
  tmpBin->DateModified = 0ULL;
  tmpBin->Unused2 = 0;

  /* Increase size of list of blocks */
  tmpBlockList = malloc (sizeof(PHBIN) * (RegistryHive->BlockListSize + 1));
  if (tmpBlockList == NULL)
    {
      free (tmpBin);
      return FALSE;
    }

  if (RegistryHive->BlockListSize > 0)
    {
      memcpy (tmpBlockList,
	      RegistryHive->BlockList,
	      sizeof(PHBIN) * RegistryHive->BlockListSize);
      free (RegistryHive->BlockList);
    }

  RegistryHive->BlockList = tmpBlockList;
  RegistryHive->BlockList[RegistryHive->BlockListSize++] = tmpBin;

  /* Initialize a free block in this heap : */
  tmpBlock = (PCELL_HEADER)((ULONG_PTR) tmpBin + REG_HBIN_DATA_OFFSET);
  tmpBlock->CellSize = (REG_BLOCK_SIZE - REG_HBIN_DATA_OFFSET);

  *NewBlock = (PVOID) tmpBlock;

  if (NewBlockOffset)
    *NewBlockOffset = tmpBin->BlockOffset + REG_HBIN_DATA_OFFSET;

  return TRUE;
}


static BOOL
CmiAllocateBlock(PREGISTRY_HIVE RegistryHive,
		 PVOID *Block,
		 LONG BlockSize,
		 PBLOCK_OFFSET pBlockOffset)
{
  PCELL_HEADER NewBlock;
  PHBIN pBin;
  ULONG i;

  *Block = NULL;

  /* Round to 16 bytes multiple */
  BlockSize = (BlockSize + sizeof(ULONG) + 15) & 0xfffffff0;

  /* first search in free blocks */
  NewBlock = NULL;
  for (i = 0; i < RegistryHive->FreeListSize; i++)
    {
      if (RegistryHive->FreeList[i]->CellSize >= BlockSize)
	{
	  NewBlock = RegistryHive->FreeList[i];
	  if (pBlockOffset)
	    *pBlockOffset = RegistryHive->FreeListOffset[i];

	  if ((i + 1) < RegistryHive->FreeListSize)
	    {
	      memmove (&RegistryHive->FreeList[i],
		       &RegistryHive->FreeList[i + 1],
		       sizeof(RegistryHive->FreeList[0])
		         * (RegistryHive->FreeListSize - i - 1));
	      memmove (&RegistryHive->FreeListOffset[i],
		       &RegistryHive->FreeListOffset[i + 1],
		       sizeof(RegistryHive->FreeListOffset[0])
		         * (RegistryHive->FreeListSize - i - 1));
	    }
	  RegistryHive->FreeListSize--;
	  break;
	}
    }

  /* Need to extend hive file : */
  if (NewBlock == NULL)
    {
      /* Add a new block */
      if (!CmiAddBin(RegistryHive, (PVOID *)&NewBlock , pBlockOffset))
	return FALSE;
    }

  *Block = NewBlock;

  /* Split the block in two parts */
  if (NewBlock->CellSize > BlockSize)
    {
      NewBlock = (PCELL_HEADER) ((ULONG_PTR) NewBlock+BlockSize);
      NewBlock->CellSize = ((PCELL_HEADER) (*Block))->CellSize - BlockSize;
      CmiAddFree (RegistryHive,
		  NewBlock,
		  *pBlockOffset + BlockSize,
		  TRUE);
    }
  else if (NewBlock->CellSize < BlockSize)
    {
      return FALSE;
    }

  memset(*Block, 0, BlockSize);
  ((PCELL_HEADER)(*Block))->CellSize = -BlockSize;

  return TRUE;
}


static BOOL
CmiAllocateHashTableCell (PREGISTRY_HIVE Hive,
			  PBLOCK_OFFSET HBOffset,
			  ULONG SubKeyCount)
{
  PHASH_TABLE_CELL HashCell;
  ULONG NewHashSize;
  BOOL Status;

  NewHashSize = ROUND_UP(sizeof(HASH_TABLE_CELL) + 
			 (SubKeyCount - 1) * sizeof(HASH_RECORD),
			 0x10);
  Status = CmiAllocateBlock (Hive,
			     (PVOID*) &HashCell,
			     NewHashSize,
			     HBOffset);
  if ((HashCell == NULL) || (Status == FALSE))
    {
      return FALSE;
    }

  HashCell->Id = REG_HASH_TABLE_BLOCK_ID;
  HashCell->HashTableSize = SubKeyCount;

  return TRUE;
}


static BOOL
CmiAddKeyToParentHashTable (PREGISTRY_HIVE Hive,
			    BLOCK_OFFSET ParentKeyOffset,
			    PKEY_CELL NewKeyCell,
			    BLOCK_OFFSET NKBOffset)
{
  PHASH_TABLE_CELL HashBlock;
  PKEY_CELL ParentKeyCell;
  ULONG i;

  ParentKeyCell = CmiGetBlock (Hive,
			       ParentKeyOffset,
			       NULL);
  if (ParentKeyCell == NULL)
    {
      DPRINT1 ("CmiGetBlock() failed\n");
      return FALSE;
    }

  HashBlock =CmiGetBlock (Hive,
			  ParentKeyCell->HashTableOffset,
			  NULL);
  if (HashBlock == NULL)
    {
      DPRINT1 ("CmiGetBlock() failed\n");
      return FALSE;
    }

  for (i = 0; i < HashBlock->HashTableSize; i++)
    {
      if (HashBlock->Table[i].KeyOffset == 0)
	{
	  HashBlock->Table[i].KeyOffset = NKBOffset;
	  memcpy (&HashBlock->Table[i].HashValue,
		  NewKeyCell->Name,
		  4);
	  ParentKeyCell->NumberOfSubKeys++;
	  return TRUE;
	}
    }

  return FALSE;
}


static BOOL
CmiAllocateValueListCell (PREGISTRY_HIVE Hive,
			  PBLOCK_OFFSET ValueListOffset,
			  ULONG ValueCount)
{
  PVALUE_LIST_CELL ValueListCell;
  ULONG ValueListSize;
  BOOL Status;

  ValueListSize = ROUND_UP (ValueCount * sizeof(BLOCK_OFFSET),
			    0x10);
  Status = CmiAllocateBlock (Hive,
			     (PVOID)&ValueListCell,
			     ValueListSize,
			     ValueListOffset);
  if ((ValueListCell == NULL) || (Status == FALSE))
    {
      DPRINT1 ("CmiAllocateBlock() failed\n");
      return FALSE;
    }

  return TRUE;
}


static BOOL
CmiAllocateValueCell(PREGISTRY_HIVE Hive,
		     PVALUE_CELL *ValueCell,
		     BLOCK_OFFSET *ValueCellOffset,
		     PCHAR ValueName)
{
  PVALUE_CELL NewValueCell;
  ULONG NameSize;
  BOOL Status;

  NameSize = (ValueName == NULL) ? 0 : strlen (ValueName);
  Status = CmiAllocateBlock(Hive,
			    (PVOID*)&NewValueCell,
			    sizeof(VALUE_CELL) + NameSize,
			    ValueCellOffset);
  if ((NewValueCell == NULL) || (Status == FALSE))
    {
      DPRINT1 ("CmiAllocateBlock() failed\n");
      return FALSE;
    }

  NewValueCell->Id = REG_VALUE_CELL_ID;
  NewValueCell->NameSize = NameSize;
  if (NameSize > 0)
    {
      memcpy (NewValueCell->Name,
	      ValueName,
	      NameSize);
      NewValueCell->Flags = REG_VALUE_NAME_PACKED;
    }
  NewValueCell->DataType = 0;
  NewValueCell->DataSize = 0;
  NewValueCell->DataOffset = -1;

  *ValueCell = NewValueCell;

  return TRUE;
}


static BOOL
CmiAddValueToKeyValueList(PREGISTRY_HIVE Hive,
			  BLOCK_OFFSET KeyCellOffset,
			  BLOCK_OFFSET ValueCellOffset)
{
  PVALUE_LIST_CELL ValueListCell;
  PKEY_CELL KeyCell;

  KeyCell = CmiGetBlock (Hive, KeyCellOffset, NULL);
  if (KeyCell == NULL)
    {
      DPRINT1 ("CmiGetBlock() failed\n");
      return FALSE;
    }

  ValueListCell = CmiGetBlock (Hive, KeyCell->ValuesOffset, NULL);
  if (ValueListCell == NULL)
    {
      DPRINT1 ("CmiGetBlock() failed\n");
      return FALSE;
    }

  ValueListCell->Values[KeyCell->NumberOfValues] = ValueCellOffset;
  KeyCell->NumberOfValues++;

  return TRUE;
}


static VOID
memexpand (PWCHAR Dst,
	   PCHAR Src,
	   ULONG Length)
{
  ULONG i;

  for (i = 0; i < Length; i++)
    Dst[i] = (WCHAR)Src[i];
}


static BOOL
CmiExportValue (PREGISTRY_HIVE Hive,
		BLOCK_OFFSET KeyCellOffset,
		HKEY Key,
		PVALUE Value)
{
  BLOCK_OFFSET ValueCellOffset;
  BLOCK_OFFSET DataCellOffset;
  PVALUE_CELL ValueCell;
  PDATA_CELL DataCell;
  ULONG SrcDataSize;
  ULONG DstDataSize;
  ULONG DataType;
  PUCHAR Data;
  BOOL Expand = FALSE;

  DPRINT ("CmiExportValue('%s') called\n", (Value == NULL) ? "<default>" : (PCHAR)Value->Name);
  DPRINT ("DataSize %lu\n", (Value == NULL) ? Key->DataSize : Value->DataSize);

  /* Allocate value cell */
  if (!CmiAllocateValueCell(Hive, &ValueCell, &ValueCellOffset, (Value == NULL) ? NULL : Value->Name))
    {
      return FALSE;
    }

  if (!CmiAddValueToKeyValueList(Hive, KeyCellOffset, ValueCellOffset))
    {
      return FALSE;
    }

  if (Value == NULL)
    {
      DataType = Key->DataType;
      SrcDataSize = Key->DataSize;
      Data = Key->Data;
    }
  else
    {
      DataType = Value->DataType;
      SrcDataSize = Value->DataSize;
      Data = Value->Data;
    }

  DstDataSize = SrcDataSize;
  if (DataType == REG_SZ ||
      DataType == REG_EXPAND_SZ ||
      DataType == REG_MULTI_SZ)
    {
      DstDataSize *= sizeof(WCHAR);
      Expand = TRUE;
    }

  if (DstDataSize <= sizeof(BLOCK_OFFSET))
    {
      ValueCell->DataSize = DstDataSize | 0x80000000;
      ValueCell->DataType = DataType;
      if (Expand)
	{
	  memexpand ((PWCHAR)&ValueCell->DataOffset,
		     (PCHAR)&Data,
		     SrcDataSize);
	}
      else
	{
	  memcpy (&ValueCell->DataOffset,
		  &Data,
		  SrcDataSize);
	}
    }
  else
    {
      if (!CmiAllocateBlock (Hive,
			     (PVOID *)&DataCell,
			     DstDataSize,
			     &DataCellOffset))
	{
	  return FALSE;
	}

      ValueCell->DataOffset = DataCellOffset;
      ValueCell->DataSize = DstDataSize;
      ValueCell->DataType = DataType;

      if (Expand)
	{
	  if (SrcDataSize <= sizeof(BLOCK_OFFSET))
	    {
	      memexpand ((PWCHAR)DataCell->Data,
			 (PCHAR)&Data,
			 SrcDataSize);
	    }
	  else
	    {
	      memexpand ((PWCHAR)DataCell->Data,
			 Data,
			 SrcDataSize);
	    }
	}
      else
	{
	  memcpy (DataCell->Data,
		  Data,
		  SrcDataSize);
	}
    }

  return TRUE;
}


static BOOL
CmiExportSubKey (PREGISTRY_HIVE Hive,
		 BLOCK_OFFSET ParentKeyOffset,
		 HKEY ParentKey,
		 HKEY Key)
{
  BLOCK_OFFSET NKBOffset;
  PKEY_CELL NewKeyCell;
  ULONG KeyCellSize;
  ULONG SubKeyCount;
  ULONG ValueCount;
  PLIST_ENTRY Entry;
  HKEY SubKey;
  PVALUE Value;

  DPRINT ("CmiExportSubKey('%s') called\n", Key->Name);

  /* Don't export links */
  if (Key->DataType == REG_LINK)
    return TRUE;

  /* Allocate key cell */
  KeyCellSize = sizeof(KEY_CELL) + Key->NameSize - 1;
  if (!CmiAllocateBlock (Hive, (PVOID)&NewKeyCell, KeyCellSize, &NKBOffset))
    {
      DPRINT1 ("CmiAllocateBlock() failed\n");
      return FALSE;
    }

  /* Initialize key cell */
  NewKeyCell->Id = REG_KEY_CELL_ID;
  NewKeyCell->Type = REG_KEY_CELL_TYPE;
  NewKeyCell->LastWriteTime = 0ULL;
  NewKeyCell->ParentKeyOffset = ParentKeyOffset;
  NewKeyCell->NumberOfSubKeys = 0;
  NewKeyCell->HashTableOffset = -1;
  NewKeyCell->NumberOfValues = 0;
  NewKeyCell->ValuesOffset = -1;
  NewKeyCell->SecurityKeyOffset = -1;
  NewKeyCell->NameSize = Key->NameSize - 1;
  NewKeyCell->ClassNameOffset = -1;
  memcpy (NewKeyCell->Name,
	  Key->Name,
	  Key->NameSize - 1);

  /* Add key cell to the parent key's hash table */
  if (!CmiAddKeyToParentHashTable (Hive,
				   ParentKeyOffset,
				   NewKeyCell,
				   NKBOffset))
    {
      DPRINT1 ("CmiAddKeyToParentHashTable() failed\n");
      return FALSE;
    }

  ValueCount = RegGetValueCount (Key);
  DPRINT ("ValueCount: %lu\n", ValueCount);
  if (ValueCount > 0)
    {
      /* Allocate value list cell */
      CmiAllocateValueListCell (Hive,
				&NewKeyCell->ValuesOffset,
				ValueCount);

      if (Key->DataSize != 0)
	{
	  if (!CmiExportValue (Hive, NKBOffset, Key, NULL))
	    return FALSE;
	}

      /* Enumerate values */
      Entry = Key->ValueList.Flink;
      while (Entry != &Key->ValueList)
	{
	  Value = CONTAINING_RECORD(Entry,
				    VALUE,
				    ValueList);

	  if (!CmiExportValue (Hive, NKBOffset, Key, Value))
	    return FALSE;

	  Entry = Entry->Flink;
	}
    }

  SubKeyCount = RegGetSubKeyCount (Key);
  DPRINT ("SubKeyCount: %lu\n", SubKeyCount);
  if (SubKeyCount > 0)
    {
      /* Allocate hash table cell */
      CmiAllocateHashTableCell (Hive,
				&NewKeyCell->HashTableOffset,
				SubKeyCount);

      /* Enumerate subkeys */
      Entry = Key->SubKeyList.Flink;
      while (Entry != &Key->SubKeyList)
	{
	  SubKey = CONTAINING_RECORD(Entry,
				     KEY,
				     KeyList);

	  if (!CmiExportSubKey (Hive, NKBOffset, Key, SubKey))
	    return FALSE;

	  Entry = Entry->Flink;
	}
    }

  return TRUE;
}


static VOID
CmiCalcHiveChecksum (PREGISTRY_HIVE Hive)
{
  PULONG Buffer;
  ULONG Sum;
  ULONG i;

  Buffer = (PULONG)Hive->HiveHeader;
  Sum = 0;
  for (i = 0; i < 127; i++)
    Sum += Buffer[i];

  Hive->HiveHeader->Checksum = Sum;
}


BOOL
CmiExportHive (PREGISTRY_HIVE Hive,
	       PCHAR KeyName)
{
  PKEY_CELL KeyCell;
  HKEY Key;
  ULONG i;
  ULONG SubKeyCount;
  ULONG ValueCount;
  PLIST_ENTRY Entry;
  HKEY SubKey;
  PVALUE Value;

  DPRINT ("CmiExportHive(%p, '%s') called\n", Hive, KeyName);

  if (RegOpenKey (NULL, KeyName, &Key) != ERROR_SUCCESS)
    {
      DPRINT1 ("RegOpenKey() failed\n");
      return FALSE;
    }

  DPRINT ("Name: %s\n", KeyName);

  KeyCell = CmiGetBlock (Hive,
			 Hive->HiveHeader->RootKeyCell,
			 NULL);
  if (KeyCell == NULL)
    {
      DPRINT1 ("CmiGetBlock() failed\n");
      return FALSE;
    }

  ValueCount = RegGetValueCount (Key);
  DPRINT ("ValueCount: %lu\n", ValueCount);
  if (ValueCount > 0)
    {
      /* Allocate value list cell */
      CmiAllocateValueListCell (Hive,
				&KeyCell->ValuesOffset,
				ValueCount);

      if (Key->DataSize != 0)
	{
	  if (!CmiExportValue (Hive, Hive->HiveHeader->RootKeyCell, Key, NULL))
	    return FALSE;
	}

      /* Enumerate values */
      Entry = Key->ValueList.Flink;
      while (Entry != &Key->ValueList)
	{
	  Value = CONTAINING_RECORD(Entry,
				    VALUE,
				    ValueList);

	  if (!CmiExportValue (Hive, Hive->HiveHeader->RootKeyCell, Key, Value))
	    return FALSE;

	  Entry = Entry->Flink;
	}
    }

  SubKeyCount = RegGetSubKeyCount (Key);
  DPRINT ("SubKeyCount: %lu\n", SubKeyCount);
  if (SubKeyCount > 0)
    {
      /* Allocate hash table cell */
      CmiAllocateHashTableCell (Hive,
				&KeyCell->HashTableOffset,
				SubKeyCount);

      /* Enumerate subkeys */
      Entry = Key->SubKeyList.Flink;
      while (Entry != &Key->SubKeyList)
	{
	  SubKey = CONTAINING_RECORD(Entry,
				     KEY,
				     KeyList);

	  if (!CmiExportSubKey (Hive, Hive->HiveHeader->RootKeyCell, Key, SubKey))
	    return FALSE;

	  Entry = Entry->Flink;
	}
    }

  CmiCalcHiveChecksum (Hive);

  return TRUE;
}


static BOOL
CmiWriteHive(PREGISTRY_HIVE Hive,
	     PCHAR FileName)
{
  PHBIN Bin;
  FILE *File;
  ULONG i;

  /* FIXME: Calculate header checksum */

  File = fopen (FileName, "w+b");
  if (File == NULL)
    {

      return FALSE;
    }

  fseek (File, 0, SEEK_SET);

  /* Write hive header */
  fwrite (Hive->HiveHeader, REG_BLOCK_SIZE, 1, File);

  Bin = NULL;
  for (i = 0; i < Hive->BlockListSize; i++)
    {
      if (Hive->BlockList[i] != Bin)
	{
	  Bin = Hive->BlockList[i];

	  DPRINT ("Bin[%lu]: Offset 0x%lx  Size 0x%lx\n",
		  i, Bin->BlockOffset, Bin->BlockSize);

	  fwrite (Bin, Bin->BlockSize, 1, File);
	}
    }

  fclose (File);

  return TRUE;
}


BOOL
ExportBinaryHive (PCHAR FileName,
		  PCHAR KeyName)
{
  PREGISTRY_HIVE Hive;

  printf ("  Creating binary hive: %s\n", FileName);

  Hive = CmiCreateRegistryHive ();
  if (Hive == NULL)
    return FALSE;

  if (!CmiExportHive (Hive, KeyName))
    {
      CmiDestroyRegistryHive (Hive);
      return FALSE;
    }

  if (!CmiWriteHive (Hive, FileName))
    {
      CmiDestroyRegistryHive (Hive);
      return FALSE;
    }

  CmiDestroyRegistryHive (Hive);

  return TRUE;
}

/* EOF */
