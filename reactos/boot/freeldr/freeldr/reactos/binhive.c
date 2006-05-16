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

#define NDEBUG
#include <debug.h>

#define  REG_HIVE_ID                   0x66676572 /* "regf" */
#define  REG_BIN_ID                    0x6e696268 /* "hbin" */
#define  REG_KEY_CELL_ID               0x6b6e
#define  REG_HASH_TABLE_BLOCK_ID       0x666c
#define  REG_VALUE_CELL_ID             0x6b76

#define  REG_BLOCK_SIZE                4096
#define  REG_HBIN_DATA_OFFSET          32
#define  REG_INIT_BLOCK_LIST_SIZE      32
#define  REG_INIT_HASH_TABLE_SIZE      3
#define  REG_EXTEND_HASH_TABLE_SIZE    4
#define  REG_VALUE_LIST_CELL_MULTIPLE  4


#define ABS_VALUE(V) (((V) < 0) ? -(V) : (V))


/* BLOCK_OFFSET = offset in file after header block */
typedef ULONG BLOCK_OFFSET, *PBLOCK_OFFSET;

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
  ULONGLONG  DateModified;

  /* Registry format major version (1) */
  ULONG  MajorVersion;

  /* Registry format minor version (3)
     Version 3 added fast indexes, version 5 has large value optimizations */
  ULONG  MinorVersion;

  /* Registry file type (0 - Primary, 1 - Log) */
  ULONG  Type;

  /* Registry format (1 is the only defined value so far) */
  ULONG  Format;

  /* Offset into file from the byte after the end of the base block.
     If the hive is volatile, this is the actual pointer to the KEY_CELL */
  BLOCK_OFFSET  RootKeyOffset;

  /* Size of each hive block ? */
  ULONG  BlockSize;

  /* (1?) */
  ULONG  Unused7;

  /* Name of hive file */
  WCHAR  FileName[48];

  ULONG  Reserved[99];

  /* Checksum of first 0x200 bytes */
  ULONG  Checksum;
} __attribute__((packed)) HIVE_HEADER, *PHIVE_HEADER;


typedef struct _BIN_HEADER
{
  /* Bin identifier "hbin" (0x6E696268) */
  ULONG  HeaderId;

  /* Block offset of this bin */
  BLOCK_OFFSET  BinOffset;

  /* Size in bytes, multiple of the block size (4KB) */
  ULONG  BinSize;

  ULONG  Reserved[2];

  /* When this bin was last modified */
  ULONGLONG  DateModified;

  /* ? (In-memory only) */
  ULONG  MemAlloc;
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

  /* Flags */
  USHORT  Flags;

  /* Time of last flush */
  ULONGLONG  LastWriteTime;		/* FILETIME */

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
  BLOCK_OFFSET  ValueListOffset;

  /* Block offset of security cell */
  BLOCK_OFFSET  SecurityKeyOffset;

  /* Block offset of registry key class */
  BLOCK_OFFSET  ClassNameOffset;

  /* ? */
  ULONG  Unused4[5];

  /* Size in bytes of key name */
  USHORT NameSize;

  /* Size of class name in bytes */
  USHORT ClassSize;

  /* Name of key (not zero terminated) */
  CHAR  Name[0];
} __attribute__((packed)) KEY_CELL, *PKEY_CELL;


/* KEY_CELL.Type constants */
#define  REG_LINK_KEY_CELL_TYPE        0x10
#define  REG_KEY_NAME_PACKED           0x20
#define  REG_ROOT_KEY_CELL_TYPE        0x0c


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
  BLOCK_OFFSET  ValueOffset[0];
} __attribute__((packed)) VALUE_LIST_CELL, *PVALUE_LIST_CELL;


typedef struct _VALUE_CELL
{
  LONG  CellSize;
  USHORT  Id;	// "kv"
  USHORT  NameSize;	// length of Name
  ULONG  DataSize;	// length of datas in the cell pointed by DataOffset
  BLOCK_OFFSET  DataOffset;// datas are here if high bit of DataSize is set
  ULONG  DataType;
  USHORT  Flags;
  USHORT Unused1;
  CHAR  Name[0]; /* warning : not zero terminated */
} __attribute__((packed)) VALUE_CELL, *PVALUE_CELL;

/* VALUE_CELL.Flags constants */
#define REG_VALUE_NAME_PACKED             0x0001

/* VALUE_CELL.DataSize mask constants */
#define REG_DATA_SIZE_MASK                 0x7FFFFFFF
#define REG_DATA_IN_OFFSET                 0x80000000


typedef struct _DATA_CELL
{
  LONG  CellSize;
  CHAR  Data[0];
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


static PVOID MbBase = NULL;
static ULONG MbSize = 0;

/* FUNCTIONS ****************************************************************/

static VOID
InitMbMemory (PVOID ChunkBase)
{
  MbBase = ChunkBase;
  MbSize = 0;
}


static PVOID
AllocateMbMemory (ULONG MemSize)
{
  PVOID CurBase;

  CurBase = MbBase;

  MbBase = (PVOID)((ULONG)MbBase + MemSize);
  MbSize += MemSize;

  return CurBase;
}

static VOID
FreeMbMemory (VOID)
{
  MbSize = 0;
}

static ULONG
GetMbAllocatedSize (VOID)
{
  return MbSize;
}


static VOID
CmiCreateDefaultHiveHeader (PHIVE_HEADER Header)
{
  assert(Header);
  memset (Header, 0, REG_BLOCK_SIZE);
  Header->BlockId = REG_HIVE_ID;
  Header->UpdateCounter1 = 0;
  Header->UpdateCounter2 = 0;
  Header->DateModified = 0;
  Header->MajorVersion = 1;
  Header->MinorVersion = 3;
  Header->Type = 0;
  Header->Format = 1;
  Header->Unused7 = 1;
  Header->RootKeyOffset = -1;
  Header->BlockSize = REG_BLOCK_SIZE;
  Header->Checksum = 0;
}


static VOID
CmiCreateDefaultBinCell (PHBIN BinCell)
{
  assert(BinCell);
  memset (BinCell, 0, REG_BLOCK_SIZE);
  BinCell->HeaderId = REG_BIN_ID;
  BinCell->DateModified = 0ULL;
  BinCell->BinSize = REG_BLOCK_SIZE;
}


static VOID
CmiCreateDefaultRootKeyCell (PKEY_CELL RootKeyCell, PCWSTR KeyName)
{
  PWCHAR BaseKeyName;
  ULONG NameSize;
  ULONG CellSize;
  ULONG i;
  BOOLEAN Packable = TRUE;

  assert (RootKeyCell);

  BaseKeyName = wcsrchr(KeyName, L'\\') + 1;
  NameSize = wcslen(BaseKeyName);
  for (i = 0; i < NameSize; i++)
    {
      if (KeyName[i] & 0xFF00)
        {
          Packable = FALSE;
          NameSize *= sizeof(WCHAR);
          break;
        }
    }

  CellSize = ROUND_UP(sizeof(KEY_CELL) + NameSize, 16);

  memset (RootKeyCell, 0, CellSize);
  RootKeyCell->CellSize = -CellSize;
  RootKeyCell->Id = REG_KEY_CELL_ID;
  RootKeyCell->Flags = REG_ROOT_KEY_CELL_TYPE;
  RootKeyCell->LastWriteTime = 0ULL;
  RootKeyCell->ParentKeyOffset = 0;
  RootKeyCell->NumberOfSubKeys = 0;
  RootKeyCell->HashTableOffset = -1;
  RootKeyCell->NumberOfValues = 0;
  RootKeyCell->ValueListOffset = -1;
  RootKeyCell->SecurityKeyOffset = 0;
  RootKeyCell->ClassNameOffset = -1;
  RootKeyCell->NameSize = NameSize;
  RootKeyCell->ClassSize = 0;
  if (Packable)
    {
      for(i = 0; i < NameSize; i++)
        {
          ((PCHAR)RootKeyCell->Name)[i] = BaseKeyName[i];
        }
      RootKeyCell->Flags |= REG_KEY_NAME_PACKED;
    }
  else
    {
      memcpy (RootKeyCell->Name, BaseKeyName, NameSize);
    }
}


static PREGISTRY_HIVE
CmiCreateHive (PCWSTR KeyName)
{
  PREGISTRY_HIVE Hive;
  PCELL_HEADER FreeCell;
  PKEY_CELL RootKeyCell;
  PHBIN BinCell;

  Hive = (PREGISTRY_HIVE) MmAllocateMemory (sizeof(REGISTRY_HIVE));
  if (Hive == NULL)
    {
      return NULL;
    }
  memset (Hive, 0, sizeof(REGISTRY_HIVE));

  DbgPrint((DPRINT_REGISTRY, "Hive %x\n", Hive));

  /* Create hive beader (aka 'base block') */
  Hive->HiveHeader = (PHIVE_HEADER) AllocateMbMemory (REG_BLOCK_SIZE);
  if (Hive->HiveHeader == NULL)
    {
      MmFreeMemory (Hive);
      return NULL;
    }
  CmiCreateDefaultHiveHeader(Hive->HiveHeader);
  Hive->FileSize = REG_BLOCK_SIZE;

  /* Allocate block list */
  Hive->BlockListSize = 1;
  Hive->BlockList = MmAllocateMemory (sizeof(PHBIN) * Hive->BlockListSize);
  if (Hive->BlockList == NULL)
    {
      MmFreeMemory (Hive);
      return NULL;
    }

  /* Allocate free cell list */
  Hive->FreeListMax = 32;
  Hive->FreeList = MmAllocateMemory(sizeof(PCELL_HEADER) * Hive->FreeListMax);
  if (Hive->FreeList == NULL)
    {
      MmFreeMemory (Hive->BlockList);
      MmFreeMemory (Hive);
      return NULL;
    }
  Hive->FreeListOffset = MmAllocateMemory(sizeof(BLOCK_OFFSET) * Hive->FreeListMax);
  if (Hive->FreeListOffset == NULL)
    {
      MmFreeMemory (Hive->FreeList);
      MmFreeMemory (Hive->BlockList);
      MmFreeMemory (Hive);
      return NULL;
    }

  /* Allocate first bin */
  Hive->BlockList[0] = (PHBIN) AllocateMbMemory (REG_BLOCK_SIZE);
  if (Hive->BlockList[0] == NULL)
    {
      MmFreeMemory (Hive->FreeListOffset);
      MmFreeMemory (Hive->FreeList);
      MmFreeMemory (Hive->BlockList);
      MmFreeMemory (Hive);
      return NULL;
    }
  Hive->FileSize += REG_BLOCK_SIZE;

  /* Init first bin */
  BinCell = (PHBIN)Hive->BlockList[0];
  CmiCreateDefaultBinCell(BinCell);
  BinCell->BinOffset = 0;

  /* Init root key cell */
  RootKeyCell = (PKEY_CELL)((ULONG)BinCell + REG_HBIN_DATA_OFFSET);
  CmiCreateDefaultRootKeyCell(RootKeyCell, KeyName);
  Hive->HiveHeader->RootKeyOffset = REG_HBIN_DATA_OFFSET;

  /* Init free cell */
  FreeCell = (PCELL_HEADER)((ULONG)RootKeyCell - RootKeyCell->CellSize);
  FreeCell->CellSize = REG_BLOCK_SIZE - (REG_HBIN_DATA_OFFSET - RootKeyCell->CellSize);

  Hive->FreeList[0] = FreeCell;
  Hive->FreeListOffset[0] = REG_HBIN_DATA_OFFSET - RootKeyCell->CellSize;
  Hive->FreeListSize++;

  return Hive;
}


static VOID
CmiCleanupHive(PREGISTRY_HIVE Hive, BOOLEAN Release)
{
  MmFreeMemory (Hive->FreeListOffset);
  MmFreeMemory (Hive->FreeList);
  MmFreeMemory (Hive->BlockList);
  MmFreeMemory (Hive);

  if (Release)
    {
      FreeMbMemory ();
    }
}


static PHBIN
CmiGetBin (PREGISTRY_HIVE Hive,
	   BLOCK_OFFSET BlockOffset)
{
  ULONG BlockIndex;

  if (BlockOffset == (ULONG) -1)
    return NULL;

  BlockIndex = BlockOffset / REG_BLOCK_SIZE;
  if (BlockIndex >= Hive->BlockListSize)
    return NULL;

  return Hive->BlockList[BlockIndex];
}


static BOOLEAN
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

  DbgPrint((DPRINT_REGISTRY, "CmiMergeFree(Block %lx  Offset %lx  Size %lx) called\n",
	   FreeBlock, FreeOffset, FreeBlock->CellSize));

  Bin = CmiGetBin (RegistryHive, FreeOffset);
  if (Bin == NULL)
    return FALSE;

  DbgPrint((DPRINT_REGISTRY, "Bin %p\n", Bin));

  BinOffset = Bin->BinOffset;
  BinSize = Bin->BinSize;
  DbgPrint((DPRINT_REGISTRY, "Bin %p  Offset %lx  Size %lx\n", Bin, BinOffset, BinSize));

  for (i = 0; i < RegistryHive->FreeListSize; i++)
    {
      BlockOffset = RegistryHive->FreeListOffset[i];
      BlockSize = RegistryHive->FreeList[i]->CellSize;
      if (BlockOffset > BinOffset &&
	  BlockOffset < BinOffset + BinSize)
	{
	  DbgPrint((DPRINT_REGISTRY, "Free block: Offset %lx  Size %lx\n",
		   BlockOffset, BlockSize));

	  if ((i < (RegistryHive->FreeListSize - 1)) &&
	      (BlockOffset + BlockSize == FreeOffset) &&
	      (FreeOffset + FreeBlock->CellSize == RegistryHive->FreeListOffset[i + 1]))
	    {
	      DbgPrint((DPRINT_REGISTRY, "Merge current block with previous and next block\n"));

	      RegistryHive->FreeList[i]->CellSize +=
		(FreeBlock->CellSize + RegistryHive->FreeList[i + 1]->CellSize);

	      FreeBlock->CellSize = 0;
	      RegistryHive->FreeList[i + 1]->CellSize = 0;


	      if ((i + 2) < RegistryHive->FreeListSize)
		{
		  memmove (&RegistryHive->FreeList[i + 1],
			   &RegistryHive->FreeList[i + 2],
			   sizeof(RegistryHive->FreeList[0])
			     * (RegistryHive->FreeListSize - i - 2));
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
	      DbgPrint((DPRINT_REGISTRY, "Merge current block with previous block\n"));

	      RegistryHive->FreeList[i]->CellSize += FreeBlock->CellSize;
	      FreeBlock->CellSize = 0;

	      return TRUE;
	    }
	  else if (FreeOffset + FreeBlock->CellSize == BlockOffset)
	    {
	      DbgPrint((DPRINT_REGISTRY, "Merge current block with next block\n"));

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


static BOOLEAN
CmiAddFree(PREGISTRY_HIVE RegistryHive,
	   PCELL_HEADER FreeBlock,
	   BLOCK_OFFSET FreeOffset,
	   BOOLEAN MergeFreeBlocks)
{
  PCELL_HEADER *tmpList;
  BLOCK_OFFSET *tmpListOffset;
  LONG minInd;
  LONG maxInd;
  LONG medInd;

  assert(RegistryHive);
  assert(FreeBlock);

  DbgPrint((DPRINT_REGISTRY, "FreeBlock %.08lx  FreeOffset %.08lx\n",
	   FreeBlock, FreeOffset));

  /* Merge free blocks */
  if (MergeFreeBlocks == TRUE)
    {
      if (CmiMergeFree(RegistryHive, FreeBlock, FreeOffset))
	return TRUE;
    }

  if ((RegistryHive->FreeListSize + 1) > RegistryHive->FreeListMax)
    {
      tmpList = MmAllocateMemory (sizeof(PCELL_HEADER) * (RegistryHive->FreeListMax + 32));
      if (tmpList == NULL)
	{
	  return FALSE;
	}

      tmpListOffset = MmAllocateMemory (sizeof(BLOCK_OFFSET) * (RegistryHive->FreeListMax + 32));
      if (tmpListOffset == NULL)
	{
	  MmFreeMemory (tmpList);
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
	  MmFreeMemory (RegistryHive->FreeList);
	  MmFreeMemory (RegistryHive->FreeListOffset);
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


static BOOLEAN
CmiAddBin(PREGISTRY_HIVE RegistryHive,
	  ULONG BlockCount,
	  PVOID *NewBlock,
	  PBLOCK_OFFSET NewBlockOffset)
{
  PCELL_HEADER tmpBlock;
  PHBIN *BlockList;
  PHBIN tmpBin;
  ULONG BinSize;
  ULONG i;

  BinSize = BlockCount * REG_BLOCK_SIZE;
  tmpBin = AllocateMbMemory (BinSize);
  if (tmpBin == NULL)
    {
      return FALSE;
    }
  memset (tmpBin, 0, BinSize);

  tmpBin->HeaderId = REG_BIN_ID;
  tmpBin->BinOffset = RegistryHive->FileSize - REG_BLOCK_SIZE;
  RegistryHive->FileSize += BinSize;
  tmpBin->BinSize = BinSize;
  tmpBin->DateModified = 0ULL;
  tmpBin->MemAlloc = 0;

  /* Increase size of list of blocks */
  BlockList = MmAllocateMemory (sizeof(PHBIN) * (RegistryHive->BlockListSize + BlockCount));
  if (BlockList == NULL)
    {
      return FALSE;
    }

  if (RegistryHive->BlockListSize > 0)
    {
      memcpy (BlockList,
	      RegistryHive->BlockList,
	      sizeof(PHBIN) * RegistryHive->BlockListSize);
      MmFreeMemory (RegistryHive->BlockList);
    }

  RegistryHive->BlockList = BlockList;
  for (i = 0; i < BlockCount; i++)
    RegistryHive->BlockList[RegistryHive->BlockListSize + i] = tmpBin;
  RegistryHive->BlockListSize += BlockCount;

  /* Initialize a free block in this heap : */
  tmpBlock = (PCELL_HEADER)((ULONG) tmpBin + REG_HBIN_DATA_OFFSET);
  tmpBlock->CellSize = (REG_BLOCK_SIZE - REG_HBIN_DATA_OFFSET);

  *NewBlock = (PVOID) tmpBlock;

  if (NewBlockOffset)
    *NewBlockOffset = tmpBin->BinOffset + REG_HBIN_DATA_OFFSET;

  return TRUE;
}


static BOOLEAN
CmiAllocateCell (PREGISTRY_HIVE RegistryHive,
		 LONG CellSize,
		 PVOID *Block,
		 PBLOCK_OFFSET pBlockOffset)
{
  PCELL_HEADER NewBlock;
  ULONG i;

  *Block = NULL;

  /* Round to 16 bytes multiple */
  CellSize = ROUND_UP(CellSize, 16);

  /* first search in free blocks */
  NewBlock = NULL;
  for (i = 0; i < RegistryHive->FreeListSize; i++)
    {
      if (RegistryHive->FreeList[i]->CellSize >= CellSize)
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

  /* Need to extend hive file */
  if (NewBlock == NULL)
    {
      /* Add a new block */
      if (!CmiAddBin(RegistryHive,
		     ((sizeof(HBIN) + CellSize - 1) / REG_BLOCK_SIZE) + 1,
		     (PVOID *)(PVOID)&NewBlock,
		     pBlockOffset))
	return FALSE;
    }

  *Block = NewBlock;

  /* Split the block in two parts */
  if (NewBlock->CellSize > CellSize)
    {
      NewBlock = (PCELL_HEADER) ((ULONG)NewBlock + CellSize);
      NewBlock->CellSize = ((PCELL_HEADER) (*Block))->CellSize - CellSize;
      CmiAddFree (RegistryHive,
		  NewBlock,
		  *pBlockOffset + CellSize,
		  TRUE);
    }
  else if (NewBlock->CellSize < CellSize)
    {
      return FALSE;
    }

  memset(*Block, 0, CellSize);
  ((PCELL_HEADER)(*Block))->CellSize = -CellSize;

  return TRUE;
}


static PVOID
CmiGetCell (PREGISTRY_HIVE Hive,
	    BLOCK_OFFSET BlockOffset)
{
  PHBIN Bin;
  ULONG BlockIndex;

  if (BlockOffset == (ULONG) -1)
    return NULL;

  BlockIndex = BlockOffset / REG_BLOCK_SIZE;
  if (BlockIndex >= Hive->BlockListSize)
    return NULL;

  Bin = Hive->BlockList[BlockIndex];
  if (Bin == NULL)
    return NULL;

  return (PVOID)((ULONG)Bin + (BlockOffset - Bin->BinOffset));
}


static BOOLEAN
CmiAllocateHashTableCell (PREGISTRY_HIVE Hive,
			  PBLOCK_OFFSET HBOffset,
			  ULONG SubKeyCount)
{
  PHASH_TABLE_CELL HashCell;
  ULONG NewHashSize;
  BOOLEAN Status;

  NewHashSize = sizeof(HASH_TABLE_CELL) +
		(SubKeyCount * sizeof(HASH_RECORD));
  Status = CmiAllocateCell (Hive,
			    NewHashSize,
			    (PVOID*)(PVOID)&HashCell,
			    HBOffset);
  if ((HashCell == NULL) || (Status == FALSE))
    {
      return FALSE;
    }

  HashCell->Id = REG_HASH_TABLE_BLOCK_ID;
  HashCell->HashTableSize = SubKeyCount;

  return TRUE;
}


static BOOLEAN
CmiAddKeyToParentHashTable (PREGISTRY_HIVE Hive,
			    BLOCK_OFFSET ParentKeyOffset,
			    PKEY_CELL NewKeyCell,
			    BLOCK_OFFSET NKBOffset)
{
  PHASH_TABLE_CELL HashBlock;
  PKEY_CELL ParentKeyCell;
  ULONG i;

  ParentKeyCell = CmiGetCell (Hive, ParentKeyOffset);
  if (ParentKeyCell == NULL)
    {
      DbgPrint((DPRINT_REGISTRY, "CmiGetCell() failed\n"));
      return FALSE;
    }

  HashBlock =CmiGetCell (Hive, ParentKeyCell->HashTableOffset);
  if (HashBlock == NULL)
    {
      DbgPrint((DPRINT_REGISTRY, "CmiGetCell() failed\n"));
      return FALSE;
    }

  for (i = 0; i < HashBlock->HashTableSize; i++)
    {
      if (HashBlock->Table[i].KeyOffset == 0)
	{
	  HashBlock->Table[i].KeyOffset = NKBOffset;
	  memcpy (&HashBlock->Table[i].HashValue,
		  NewKeyCell->Name,
		  min(NewKeyCell->NameSize, sizeof(ULONG)));
	  ParentKeyCell->NumberOfSubKeys++;
	  return TRUE;
	}
    }

  return FALSE;
}


static BOOLEAN
CmiAllocateValueListCell (PREGISTRY_HIVE Hive,
			  PBLOCK_OFFSET ValueListOffset,
			  ULONG ValueCount)
{
  PVALUE_LIST_CELL ValueListCell;
  ULONG ValueListSize;
  BOOLEAN Status;

  ValueListSize = sizeof(VALUE_LIST_CELL) +
		  (ValueCount * sizeof(BLOCK_OFFSET));
  Status = CmiAllocateCell (Hive,
			    ValueListSize,
			    (PVOID)&ValueListCell,
			    ValueListOffset);
  if ((ValueListCell == NULL) || (Status == FALSE))
    {
      DbgPrint((DPRINT_REGISTRY, "CmiAllocateCell() failed\n"));
      return FALSE;
    }

  return TRUE;
}


static BOOLEAN
CmiAllocateValueCell(PREGISTRY_HIVE Hive,
		     PVALUE_CELL *ValueCell,
		     BLOCK_OFFSET *ValueCellOffset,
		     PWCHAR ValueName)
{
  PVALUE_CELL NewValueCell;
  ULONG NameSize;
  BOOLEAN Status;
  BOOLEAN Packable = TRUE;
  ULONG i;

  NameSize = (ValueName == NULL) ? 0 : wcslen (ValueName);
  for (i = 0; i < NameSize; i++)
    {
      if (ValueName[i] & 0xFF00)
        {
          NameSize *= sizeof(WCHAR);
          Packable = FALSE;
          break;
        }
    }
  Status = CmiAllocateCell (Hive,
			    sizeof(VALUE_CELL) + NameSize,
			    (PVOID*)(PVOID)&NewValueCell,
			    ValueCellOffset);
  if ((NewValueCell == NULL) || (Status == FALSE))
    {
      DbgPrint((DPRINT_REGISTRY, "CmiAllocateCell() failed\n"));
      return FALSE;
    }

  NewValueCell->Id = REG_VALUE_CELL_ID;
  NewValueCell->NameSize = NameSize;
  NewValueCell->Flags = 0;
  if (NameSize > 0)
    {
      if (Packable)
        {
          for (i = 0; i < NameSize; i++)
            {
              ((PCHAR)NewValueCell->Name)[i] = (CHAR)ValueName[i];
            }
          NewValueCell->Flags |= REG_VALUE_NAME_PACKED;
        }
      else
        {
          memcpy (NewValueCell->Name,
	          ValueName,
	          NameSize);
        }
    }
  NewValueCell->DataType = 0;
  NewValueCell->DataSize = 0;
  NewValueCell->DataOffset = -1;

  *ValueCell = NewValueCell;

  return TRUE;
}


static BOOLEAN
CmiAddValueToKeyValueList(PREGISTRY_HIVE Hive,
			  BLOCK_OFFSET KeyCellOffset,
			  BLOCK_OFFSET ValueCellOffset)
{
  PVALUE_LIST_CELL ValueListCell;
  PKEY_CELL KeyCell;

  KeyCell = CmiGetCell (Hive, KeyCellOffset);
  if (KeyCell == NULL)
    {
      DbgPrint((DPRINT_REGISTRY, "CmiGetCell() failed\n"));
      return FALSE;
    }

  ValueListCell = CmiGetCell (Hive, KeyCell->ValueListOffset);
  if (ValueListCell == NULL)
    {
      DbgPrint((DPRINT_REGISTRY, "CmiGetCell() failed\n"));
      return FALSE;
    }

  ValueListCell->ValueOffset[KeyCell->NumberOfValues] = ValueCellOffset;
  KeyCell->NumberOfValues++;

  return TRUE;
}

static BOOLEAN
CmiExportValue (PREGISTRY_HIVE Hive,
		BLOCK_OFFSET KeyCellOffset,
		FRLDRHKEY Key,
		PVALUE Value)
{
  BLOCK_OFFSET ValueCellOffset;
  BLOCK_OFFSET DataCellOffset;
  PVALUE_CELL ValueCell;
  PDATA_CELL DataCell;
  ULONG DataSize;
  ULONG DataType;
  PCHAR Data;

  DbgPrint((DPRINT_REGISTRY, "CmiExportValue('%S') called\n",
	   (Value == NULL) ? "<default>" : (PCHAR)Value->Name));
  DbgPrint((DPRINT_REGISTRY, "DataSize %lu\n",
	   (Value == NULL) ? Key->DataSize : Value->DataSize));

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
      DataSize = Key->DataSize;
      Data = Key->Data;
    }
  else
    {
      DataType = Value->DataType;
      DataSize = Value->DataSize;
      Data = Value->Data;
    }

  if (DataSize <= sizeof(BLOCK_OFFSET))
    {
      ValueCell->DataSize = DataSize | REG_DATA_IN_OFFSET;
      ValueCell->DataType = DataType;
      memcpy (&ValueCell->DataOffset,
	      &Data,
	      DataSize);
    }
  else
    {
      /* Allocate data cell */
      if (!CmiAllocateCell (Hive,
			    sizeof(CELL_HEADER) + DataSize,
			    (PVOID *)(PVOID)&DataCell,
			    &DataCellOffset))
	{
	  return FALSE;
	}

      ValueCell->DataOffset = DataCellOffset;
      ValueCell->DataSize = DataSize;
      ValueCell->DataType = DataType;

      memcpy (DataCell->Data,
	      Data,
	      DataSize);
    }

  return TRUE;
}


static BOOLEAN
CmiExportSubKey (PREGISTRY_HIVE Hive,
		 BLOCK_OFFSET ParentKeyOffset,
		 FRLDRHKEY ParentKey,
		 FRLDRHKEY Key)
{
  BLOCK_OFFSET NKBOffset;
  PKEY_CELL NewKeyCell;
  ULONG KeyCellSize;
  ULONG SubKeyCount;
  ULONG ValueCount;
  PLIST_ENTRY Entry;
  FRLDRHKEY SubKey;
  PVALUE Value;
  BOOLEAN Packable = TRUE;
  ULONG i;
  ULONG NameSize;

  DbgPrint((DPRINT_REGISTRY, "CmiExportSubKey('%S') called\n", Key->Name));

  /* Don't export links */
  if (Key->DataType == REG_LINK)
    return TRUE;

  NameSize = (Key->NameSize - sizeof(WCHAR)) / sizeof(WCHAR);
  for (i = 0; i < NameSize; i++)
    {
      if (Key->Name[i] & 0xFF00)
        {
          Packable = FALSE;
          NameSize *= sizeof(WCHAR);
          break;
        }
    }
          
  /* Allocate key cell */
  KeyCellSize = sizeof(KEY_CELL) + NameSize;
  if (!CmiAllocateCell (Hive, KeyCellSize, (PVOID)&NewKeyCell, &NKBOffset))
    {
      DbgPrint((DPRINT_REGISTRY, "CmiAllocateCell() failed\n"));
      return FALSE;
    }

  /* Initialize key cell */
  NewKeyCell->Id = REG_KEY_CELL_ID;
  NewKeyCell->Flags = 0;
  NewKeyCell->LastWriteTime = 0ULL;
  NewKeyCell->ParentKeyOffset = ParentKeyOffset;
  NewKeyCell->NumberOfSubKeys = 0;
  NewKeyCell->HashTableOffset = -1;
  NewKeyCell->NumberOfValues = 0;
  NewKeyCell->ValueListOffset = -1;
  NewKeyCell->SecurityKeyOffset = -1;
  NewKeyCell->ClassNameOffset = -1;
  NewKeyCell->NameSize = NameSize;
  NewKeyCell->ClassSize = 0;
  if (Packable)
    {
      for (i = 0; i < NameSize; i++)
        {
          ((PCHAR)NewKeyCell->Name)[i] = (CHAR)Key->Name[i];
        }
      NewKeyCell->Flags |= REG_KEY_NAME_PACKED;

    }
  else
    {
      memcpy (NewKeyCell->Name,
	      Key->Name,
	      NameSize);
    }

  /* Add key cell to the parent key's hash table */
  if (!CmiAddKeyToParentHashTable (Hive,
				   ParentKeyOffset,
				   NewKeyCell,
				   NKBOffset))
    {
      DbgPrint((DPRINT_REGISTRY, "CmiAddKeyToParentHashTable() failed\n"));
      return FALSE;
    }

  ValueCount = RegGetValueCount (Key);
  DbgPrint((DPRINT_REGISTRY, "ValueCount: %u\n", ValueCount));
  if (ValueCount > 0)
    {
      /* Allocate value list cell */
      if (!CmiAllocateValueListCell (Hive,
				     &NewKeyCell->ValueListOffset,
				     ValueCount))
	{
	  DbgPrint((DPRINT_REGISTRY, "CmiAllocateValueListCell() failed\n"));
	  return FALSE;
	}

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
  DbgPrint((DPRINT_REGISTRY, "SubKeyCount: %u\n", SubKeyCount));
  if (SubKeyCount > 0)
    {
      /* Allocate hash table cell */
      if (!CmiAllocateHashTableCell (Hive,
				     &NewKeyCell->HashTableOffset,
				     SubKeyCount))
	{
	  DbgPrint((DPRINT_REGISTRY, "CmiAllocateHashTableCell() failed\n"));
	  return FALSE;
	}

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
  ULONG *Buffer;
  ULONG Sum;
  ULONG i;

  Buffer = (ULONG*)Hive->HiveHeader;
  Sum = 0;
  for (i = 0; i < 127; i++)
    Sum += Buffer[i];

  Hive->HiveHeader->Checksum = Sum;
}


static BOOLEAN
CmiExportHive (PREGISTRY_HIVE Hive,
	       PCWSTR KeyName)
{
  PKEY_CELL KeyCell;
  FRLDRHKEY Key;
  ULONG SubKeyCount;
  ULONG ValueCount;
  PLIST_ENTRY Entry;
  FRLDRHKEY SubKey;
  PVALUE Value;

  DbgPrint((DPRINT_REGISTRY, "CmiExportHive(%x, '%S') called\n", Hive, KeyName));

  if (RegOpenKey (NULL, KeyName, &Key) != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_REGISTRY, "RegOpenKey() failed\n"));
      return FALSE;
    }

  KeyCell = CmiGetCell (Hive, Hive->HiveHeader->RootKeyOffset);
  if (KeyCell == NULL)
    {
      DbgPrint((DPRINT_REGISTRY, "CmiGetBlock() failed\n"));
      return FALSE;
    }

  ValueCount = RegGetValueCount (Key);
  DbgPrint((DPRINT_REGISTRY, "ValueCount: %u\n", ValueCount));
  if (ValueCount > 0)
    {
      /* Allocate value list cell */
      if (!CmiAllocateValueListCell (Hive,
				     &KeyCell->ValueListOffset,
				     ValueCount))
	{
	  DbgPrint((DPRINT_REGISTRY, "CmiAllocateValueListCell() failed\n"));
	  return FALSE;
	}

      if (Key->DataSize != 0)
	{
	  if (!CmiExportValue (Hive, Hive->HiveHeader->RootKeyOffset, Key, NULL))
	    return FALSE;
	}

      /* Enumerate values */
      Entry = Key->ValueList.Flink;
      while (Entry != &Key->ValueList)
	{
	  Value = CONTAINING_RECORD(Entry,
				    VALUE,
				    ValueList);

	  if (!CmiExportValue (Hive, Hive->HiveHeader->RootKeyOffset, Key, Value))
	    return FALSE;

	  Entry = Entry->Flink;
	}
    }

  SubKeyCount = RegGetSubKeyCount (Key);
  DbgPrint((DPRINT_REGISTRY, "SubKeyCount: %u\n", SubKeyCount));
  if (SubKeyCount > 0)
    {
      /* Allocate hash table cell */
      if (!CmiAllocateHashTableCell (Hive,
				     &KeyCell->HashTableOffset,
				     SubKeyCount))
	{
	  DbgPrint((DPRINT_REGISTRY, "CmiAllocateHashTableCell() failed\n"));
	  return FALSE;
	}

      /* Enumerate subkeys */
      Entry = Key->SubKeyList.Flink;
      while (Entry != &Key->SubKeyList)
	{
	  SubKey = CONTAINING_RECORD(Entry,
				     KEY,
				     KeyList);

	  if (!CmiExportSubKey (Hive, Hive->HiveHeader->RootKeyOffset, Key, SubKey))
	    return FALSE;

	  Entry = Entry->Flink;
	}
    }

  CmiCalcHiveChecksum (Hive);

  return TRUE;
}


static BOOLEAN
RegImportValue (PHBIN RootBin,
		PVALUE_CELL ValueCell,
		FRLDRHKEY Key)
{
  PDATA_CELL DataCell;
  PWCHAR wName;
  LONG Error;
  ULONG DataSize;
  ULONG i;

  if (ValueCell->CellSize >= 0 || ValueCell->Id != REG_VALUE_CELL_ID)
    {
      DbgPrint((DPRINT_REGISTRY, "Invalid key cell!\n"));
      return FALSE;
    }

  if (ValueCell->Flags & REG_VALUE_NAME_PACKED)
    {
      wName = MmAllocateMemory ((ValueCell->NameSize + 1)*sizeof(WCHAR));
      for (i = 0; i < ValueCell->NameSize; i++)
        {
          wName[i] = ((PCHAR)ValueCell->Name)[i];
        }
      wName[ValueCell->NameSize] = 0;
    }
  else
    {
      wName = MmAllocateMemory (ValueCell->NameSize + sizeof(WCHAR));
      memcpy (wName,
	      ValueCell->Name,
	      ValueCell->NameSize);
      wName[ValueCell->NameSize / sizeof(WCHAR)] = 0;
    }

  DataSize = ValueCell->DataSize & REG_DATA_SIZE_MASK;

  DbgPrint((DPRINT_REGISTRY, "ValueName: '%S'\n", wName));
  DbgPrint((DPRINT_REGISTRY, "DataSize: %u\n", DataSize));

  if (DataSize <= sizeof(BLOCK_OFFSET) && (ValueCell->DataSize & REG_DATA_IN_OFFSET))
    {
      Error = RegSetValue(Key,
			  wName,
			  ValueCell->DataType,
			  (PCHAR)&ValueCell->DataOffset,
			  DataSize);
      if (Error != ERROR_SUCCESS)
	{
	  DbgPrint((DPRINT_REGISTRY, "RegSetValue() failed!\n"));
	  MmFreeMemory (wName);
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
	  MmFreeMemory (wName);
	  return FALSE;
	}

      Error = RegSetValue (Key,
			   wName,
			   ValueCell->DataType,
			   DataCell->Data,
			   DataSize);
	
      if (Error != ERROR_SUCCESS)
	{
	  DbgPrint((DPRINT_REGISTRY, "RegSetValue() failed!\n"));
	  MmFreeMemory (wName);
	  return FALSE;
	}
    }

  MmFreeMemory (wName);

  return TRUE;
}


static BOOLEAN
RegImportSubKey(PHBIN RootBin,
		PKEY_CELL KeyCell,
		FRLDRHKEY ParentKey)
{
  PHASH_TABLE_CELL HashCell;
  PKEY_CELL SubKeyCell;
  PVALUE_LIST_CELL ValueListCell;
  PVALUE_CELL ValueCell = NULL;
  PWCHAR wName;
  FRLDRHKEY SubKey;
  LONG Error;
  ULONG i;


  DbgPrint((DPRINT_REGISTRY, "KeyCell: %x\n", KeyCell));
  DbgPrint((DPRINT_REGISTRY, "KeyCell->CellSize: %x\n", KeyCell->CellSize));
  DbgPrint((DPRINT_REGISTRY, "KeyCell->Id: %x\n", KeyCell->Id));
  if (KeyCell->Id != REG_KEY_CELL_ID || KeyCell->CellSize >= 0)
    {
      DbgPrint((DPRINT_REGISTRY, "Invalid key cell id!\n"));
      return FALSE;
    }

  if (KeyCell->Flags & REG_KEY_NAME_PACKED)
    {
      wName = MmAllocateMemory ((KeyCell->NameSize + 1) * sizeof(WCHAR));
      for (i = 0; i < KeyCell->NameSize; i++)
        {
          wName[i] = ((PCHAR)KeyCell->Name)[i];
        }
      wName[KeyCell->NameSize] = 0;
    }
  else
    {
      wName = MmAllocateMemory (KeyCell->NameSize + sizeof(WCHAR));
      memcpy (wName,
	      KeyCell->Name,
	      KeyCell->NameSize);
      wName[KeyCell->NameSize/sizeof(WCHAR)] = 0;
    }

  DbgPrint((DPRINT_REGISTRY, "KeyName: '%S'\n", wName));

  /* Create new sub key */
  Error = RegCreateKey (ParentKey,
			wName,
			&SubKey);
  MmFreeMemory (wName);
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
      ValueListCell = (PVALUE_LIST_CELL)((PUCHAR)RootBin + KeyCell->ValueListOffset);
      DbgPrint((DPRINT_REGISTRY, "ValueListCell: %x\n", ValueListCell));

      for (i = 0; i < KeyCell->NumberOfValues; i++)
	{
	  DbgPrint((DPRINT_REGISTRY, "ValueOffset[%d]: %x\n", i, ValueListCell->ValueOffset[i]));

	  ValueCell = (PVALUE_CELL)((PUCHAR)RootBin + ValueListCell->ValueOffset[i]);

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


BOOLEAN
RegImportBinaryHive(PCHAR ChunkBase,
		    ULONG ChunkSize)
{
  PHIVE_HEADER HiveHeader;
  PHBIN RootBin;
  PKEY_CELL KeyCell;
  PHASH_TABLE_CELL HashCell;
  PKEY_CELL SubKeyCell;
  FRLDRHKEY SystemKey;
  ULONG i;
  LONG Error;

  DbgPrint((DPRINT_REGISTRY, "RegImportBinaryHive(%x, %u) called\n",ChunkBase,ChunkSize));

  HiveHeader = (PHIVE_HEADER)ChunkBase;
  DbgPrint((DPRINT_REGISTRY, "HiveHeader: %x\n", HiveHeader));
  if (HiveHeader->BlockId != REG_HIVE_ID)
    {
      DbgPrint((DPRINT_REGISTRY, "Invalid hive id!\n"));
      return FALSE;
    }

  RootBin = (PHBIN)((ULONG)HiveHeader + REG_BLOCK_SIZE);
  DbgPrint((DPRINT_REGISTRY, "RootBin: %x\n", RootBin));
  if (RootBin->HeaderId != REG_BIN_ID || RootBin->BinSize == 0)
    {
      DbgPrint((DPRINT_REGISTRY, "Invalid bin id!\n"));
      return FALSE;
    }

  KeyCell = (PKEY_CELL)((ULONG)RootBin + REG_HBIN_DATA_OFFSET);
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
		     L"\\Registry\\Machine\\SYSTEM",
		     &SystemKey);
  if (Error != ERROR_SUCCESS)
    {
      DbgPrint((DPRINT_REGISTRY, "Failed to open 'system' key!\n"));
      return FALSE;
    }

  /* Enumerate and add subkeys */
  if (KeyCell->NumberOfSubKeys > 0)
    {
      HashCell = (PHASH_TABLE_CELL)((ULONG)RootBin + KeyCell->HashTableOffset);
      DbgPrint((DPRINT_REGISTRY, "HashCell: %x\n", HashCell));

      for (i = 0; i < KeyCell->NumberOfSubKeys; i++)
	{
	  DbgPrint((DPRINT_REGISTRY, "KeyOffset[%d]: %x\n", i, HashCell->Table[i].KeyOffset));

	  SubKeyCell = (PKEY_CELL)((ULONG)RootBin + HashCell->Table[i].KeyOffset);

	  DbgPrint((DPRINT_REGISTRY, "SubKeyCell[%d]: %x\n", i, SubKeyCell));

	  if (!RegImportSubKey(RootBin, SubKeyCell, SystemKey))
	    return FALSE;
	}
    }

  return TRUE;
}


BOOLEAN
RegExportBinaryHive(PCWSTR KeyName,
		    PCHAR ChunkBase,
		    ULONG* ChunkSize)
{
  PREGISTRY_HIVE Hive;

  DbgPrint((DPRINT_REGISTRY, "Creating binary hardware hive\n"));

  *ChunkSize = 0;
  InitMbMemory (ChunkBase);

  Hive = CmiCreateHive (KeyName);
  if (Hive == NULL)
    return FALSE;

  if (!CmiExportHive (Hive, KeyName))
    {
      CmiCleanupHive (Hive, TRUE);
      return FALSE;
    }

  CmiCleanupHive (Hive, FALSE);

  *ChunkSize = GetMbAllocatedSize ();

  return TRUE;
}

/* EOF */
