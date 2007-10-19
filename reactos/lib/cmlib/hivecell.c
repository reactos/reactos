/*
 * PROJECT:   registry manipulation library
 * LICENSE:   GPL - See COPYING in the top level directory
 * COPYRIGHT: Copyright 2005 Filip Navara <navaraf@reactos.org>
 *            Copyright 2001 - 2005 Eric Kohl
 */

#include "cmlib.h"
#define NDEBUG
#include <debug.h>

static PHCELL __inline CMAPI
HvpGetCellHeader(
   PHHIVE RegistryHive,
   HCELL_INDEX CellIndex)
{
   PVOID Block;

   ASSERT(CellIndex != HCELL_NULL);
   if (!RegistryHive->Flat)
   {
      ULONG CellType;
      ULONG CellBlock;
      ULONG CellOffset;

      CellType = (CellIndex & HCELL_TYPE_MASK) >> HCELL_TYPE_SHIFT;
      CellBlock = (CellIndex & HCELL_BLOCK_MASK) >> HCELL_BLOCK_SHIFT;
      CellOffset = (CellIndex & HCELL_OFFSET_MASK) >> HCELL_OFFSET_SHIFT;
      ASSERT(CellBlock < RegistryHive->Storage[CellType].Length);
      Block = (PVOID)RegistryHive->Storage[CellType].BlockList[CellBlock].Block;
      ASSERT(Block != NULL);
      return (PVOID)((ULONG_PTR)Block + CellOffset);
   }
   else
   {
      ASSERT((CellIndex & HCELL_TYPE_MASK) == HvStable);
      return (PVOID)((ULONG_PTR)RegistryHive->HiveHeader + HV_BLOCK_SIZE +
                     CellIndex);
   }
}

BOOLEAN CMAPI
HvIsCellAllocated(IN PHHIVE RegistryHive,
                  IN HCELL_INDEX CellIndex)
{
    ULONG Type, Block;

    /* If it's a flat hive, the cell is always allocated */
    if (RegistryHive->Flat) return TRUE;

    /* Otherwise, get the type and make sure it's valid */
    Type = HvGetCellType(CellIndex);
    if (((CellIndex % ~HCELL_TYPE_MASK) > RegistryHive->Storage[Type].Length) ||
        (CellIndex % (RegistryHive->Version >= 2 ? 8 : 16)))
    {
        /* Invalid cell index */
        return FALSE;
    }

    /* Try to get the cell block */
    Block = (CellIndex & HCELL_BLOCK_MASK) >> HCELL_BLOCK_SHIFT;
    if (RegistryHive->Storage[Type].BlockList[Block].Block) return TRUE;

    /* No valid block, fail */
    return FALSE;
}

PVOID CMAPI
HvGetCell(
   PHHIVE RegistryHive,
   HCELL_INDEX CellIndex)
{
   return (PVOID)(HvpGetCellHeader(RegistryHive, CellIndex) + 1);
}

static LONG __inline CMAPI
HvpGetCellFullSize(
   PHHIVE RegistryHive,
   PVOID Cell)
{
   return ((PHCELL)Cell - 1)->Size;
}

LONG CMAPI
HvGetCellSize(IN PHHIVE Hive,
              IN PVOID Address)
{
    PHCELL CellHeader;
    LONG Size;

    CellHeader = (PHCELL)Address - 1;
    Size = CellHeader->Size * -1;
    Size -= sizeof(HCELL);
    return Size;
}

VOID CMAPI
HvMarkCellDirty(
   PHHIVE RegistryHive,
   HCELL_INDEX CellIndex)
{
   LONG CellSize;
   ULONG CellBlock;
   ULONG CellLastBlock;

   ASSERT(RegistryHive->ReadOnly == FALSE);

   if ((CellIndex & HCELL_TYPE_MASK) >> HCELL_TYPE_SHIFT != HvStable)
      return;

   CellBlock = (CellIndex & HCELL_BLOCK_MASK) >> HCELL_BLOCK_SHIFT;
   CellLastBlock = ((CellIndex + HV_BLOCK_SIZE - 1) & HCELL_BLOCK_MASK) >> HCELL_BLOCK_SHIFT;

   CellSize = HvpGetCellFullSize(RegistryHive, HvGetCell(RegistryHive, CellIndex));
   if (CellSize < 0)
      CellSize = -CellSize;

   RtlSetBits(&RegistryHive->DirtyVector,
              CellBlock, CellLastBlock - CellBlock);
}

BOOLEAN CMAPI
HvIsCellDirty(IN PHHIVE Hive,
              IN HCELL_INDEX Cell)
{
    /* Sanity checks */
    ASSERT(Hive->ReadOnly == FALSE);

    /* Volatile cells are always "dirty" */
    if (HvGetCellType(Cell) == HvVolatile) return TRUE;

    /* Check if the dirty bit is set */
    return RtlCheckBit(&Hive->DirtyVector, Cell / HV_BLOCK_SIZE);
}

static ULONG __inline CMAPI
HvpComputeFreeListIndex(
   ULONG Size)
{
   ULONG Index;
   static CCHAR FindFirstSet[256] = {
      0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
      4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
      5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
      5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
      6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
      6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
      6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
      6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
      7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
      7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
      7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
      7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
      7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
      7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
      7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
      7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7};

   Index = (Size >> 3) - 1;
   if (Index >= 16)
   {
      if (Index > 255)
         Index = 23;
      else
         Index = FindFirstSet[Index] + 7;
   }

   return Index;
}

static NTSTATUS CMAPI
HvpAddFree(
   PHHIVE RegistryHive,
   PHCELL FreeBlock,
   HCELL_INDEX FreeIndex)
{
   PHCELL_INDEX FreeBlockData;
   HV_STORAGE_TYPE Storage;
   ULONG Index;

   ASSERT(RegistryHive != NULL);
   ASSERT(FreeBlock != NULL);

   Storage = (FreeIndex & HCELL_TYPE_MASK) >> HCELL_TYPE_SHIFT;
   Index = HvpComputeFreeListIndex((ULONG)FreeBlock->Size);

   FreeBlockData = (PHCELL_INDEX)(FreeBlock + 1);
   *FreeBlockData = RegistryHive->Storage[Storage].FreeDisplay[Index];
   RegistryHive->Storage[Storage].FreeDisplay[Index] = FreeIndex;

   /* FIXME: Eventually get rid of free bins. */

   return STATUS_SUCCESS;
}

static VOID CMAPI
HvpRemoveFree(
   PHHIVE RegistryHive,
   PHCELL CellBlock,
   HCELL_INDEX CellIndex)
{
   PHCELL_INDEX FreeCellData;
   PHCELL_INDEX pFreeCellOffset;
   HV_STORAGE_TYPE Storage;
   ULONG Index;

   ASSERT(RegistryHive->ReadOnly == FALSE);

   Storage = (CellIndex & HCELL_TYPE_MASK) >> HCELL_TYPE_SHIFT;
   Index = HvpComputeFreeListIndex((ULONG)CellBlock->Size);

   pFreeCellOffset = &RegistryHive->Storage[Storage].FreeDisplay[Index];
   while (*pFreeCellOffset != HCELL_NULL)
   {
      FreeCellData = (PHCELL_INDEX)HvGetCell(RegistryHive, *pFreeCellOffset);
      if (*pFreeCellOffset == CellIndex)
      {
         *pFreeCellOffset = *FreeCellData;
         return;
      }
      pFreeCellOffset = FreeCellData;
   }

   //ASSERT(FALSE);
}

static HCELL_INDEX CMAPI
HvpFindFree(
   PHHIVE RegistryHive,
   ULONG Size,
   HV_STORAGE_TYPE Storage)
{
   PHCELL_INDEX FreeCellData;
   HCELL_INDEX FreeCellOffset;
   PHCELL_INDEX pFreeCellOffset;
   ULONG Index;

   for (Index = HvpComputeFreeListIndex(Size); Index < 24; Index++)
   {
      pFreeCellOffset = &RegistryHive->Storage[Storage].FreeDisplay[Index];
      while (*pFreeCellOffset != HCELL_NULL)
      {
         FreeCellData = (PHCELL_INDEX)HvGetCell(RegistryHive, *pFreeCellOffset);
         if ((ULONG)HvpGetCellFullSize(RegistryHive, FreeCellData) >= Size)
         {
            FreeCellOffset = *pFreeCellOffset;
            *pFreeCellOffset = *FreeCellData;
            return FreeCellOffset;
         }
         pFreeCellOffset = FreeCellData;
      }
   }

   return HCELL_NULL;
}

NTSTATUS CMAPI
HvpCreateHiveFreeCellList(
   PHHIVE Hive)
{
   HCELL_INDEX BlockOffset;
   PHCELL FreeBlock;
   ULONG BlockIndex;
   ULONG FreeOffset;
   PHBIN Bin;
   NTSTATUS Status;
   ULONG Index;

   /* Initialize the free cell list */
   for (Index = 0; Index < 24; Index++)
   {
      Hive->Storage[HvStable].FreeDisplay[Index] = HCELL_NULL;
      Hive->Storage[HvVolatile].FreeDisplay[Index] = HCELL_NULL;
   }

   BlockOffset = 0;
   BlockIndex = 0;
   while (BlockIndex < Hive->Storage[HvStable].Length)
   {
      Bin = (PHBIN)Hive->Storage[HvStable].BlockList[BlockIndex].Bin;

      /* Search free blocks and add to list */
      FreeOffset = sizeof(HBIN);
      while (FreeOffset < Bin->Size)
      {
         FreeBlock = (PHCELL)((ULONG_PTR)Bin + FreeOffset);
         if (FreeBlock->Size > 0)
         {
            Status = HvpAddFree(Hive, FreeBlock, Bin->FileOffset + FreeOffset);
            if (!NT_SUCCESS(Status))
               return Status;

            FreeOffset += FreeBlock->Size;
         }
         else
         {
            FreeOffset -= FreeBlock->Size;
         }
      }

      BlockIndex += Bin->Size / HV_BLOCK_SIZE;
      BlockOffset += Bin->Size;
   }

   return STATUS_SUCCESS;
}

HCELL_INDEX CMAPI
HvAllocateCell(
   PHHIVE RegistryHive,
   SIZE_T Size,
   HV_STORAGE_TYPE Storage)
{
   PHCELL FreeCell;
   HCELL_INDEX FreeCellOffset;
   PHCELL NewCell;
   PHBIN Bin;

   ASSERT(RegistryHive->ReadOnly == FALSE);

   /* Round to 16 bytes multiple. */
   Size = ROUND_UP(Size + sizeof(HCELL), 16);

   /* First search in free blocks. */
   FreeCellOffset = HvpFindFree(RegistryHive, Size, Storage);

   /* If no free cell was found we need to extend the hive file. */
   if (FreeCellOffset == HCELL_NULL)
   {
      Bin = HvpAddBin(RegistryHive, Size, Storage);
      if (Bin == NULL)
         return HCELL_NULL;
      FreeCellOffset = Bin->FileOffset + sizeof(HBIN);
      FreeCellOffset |= Storage << HCELL_TYPE_SHIFT;
   }

   FreeCell = HvpGetCellHeader(RegistryHive, FreeCellOffset);

   /* Split the block in two parts */
   /* FIXME: There is some minimal cell size that we must respect. */
   if ((ULONG)FreeCell->Size > Size + sizeof(HCELL_INDEX))
   {
      NewCell = (PHCELL)((ULONG_PTR)FreeCell + Size);
      NewCell->Size = FreeCell->Size - Size;
      FreeCell->Size = Size;
      HvpAddFree(RegistryHive, NewCell, FreeCellOffset + Size);
      if (Storage == HvStable)
         HvMarkCellDirty(RegistryHive, FreeCellOffset + Size);
   }

   if (Storage == HvStable)
      HvMarkCellDirty(RegistryHive, FreeCellOffset);
   FreeCell->Size = -FreeCell->Size;
   RtlZeroMemory(FreeCell + 1, Size - sizeof(HCELL));

   return FreeCellOffset;
}

HCELL_INDEX CMAPI
HvReallocateCell(
   PHHIVE RegistryHive,
   HCELL_INDEX CellIndex,
   ULONG Size)
{
   PVOID OldCell;
   PVOID NewCell;
   LONG OldCellSize;
   HCELL_INDEX NewCellIndex;
   HV_STORAGE_TYPE Storage;

   ASSERT(CellIndex != HCELL_NULL);

   Storage = (CellIndex & HCELL_TYPE_MASK) >> HCELL_TYPE_SHIFT;

   OldCell = HvGetCell(RegistryHive, CellIndex);
   OldCellSize = HvGetCellSize(RegistryHive, OldCell);
   ASSERT(OldCellSize > 0);

   /*
    * If new data size is larger than the current, destroy current
    * data block and allocate a new one.
    *
    * FIXME: Merge with adjacent free cell if possible.
    * FIXME: Implement shrinking.
    */
   if (Size > OldCellSize)
   {
      NewCellIndex = HvAllocateCell(RegistryHive, Size, Storage);
      if (NewCellIndex == HCELL_NULL)
         return HCELL_NULL;

      NewCell = HvGetCell(RegistryHive, NewCellIndex);
      RtlCopyMemory(NewCell, OldCell, (SIZE_T)OldCellSize);

      HvFreeCell(RegistryHive, CellIndex);

      return NewCellIndex;
   }

   return CellIndex;
}

VOID CMAPI
HvFreeCell(
   PHHIVE RegistryHive,
   HCELL_INDEX CellIndex)
{
   PHCELL Free;
   PHCELL Neighbor;
   PHBIN Bin;
   ULONG CellType;
   ULONG CellBlock;

   ASSERT(RegistryHive->ReadOnly == FALSE);

   Free = HvpGetCellHeader(RegistryHive, CellIndex);

   ASSERT(Free->Size < 0);

   Free->Size = -Free->Size;

   CellType = (CellIndex & HCELL_TYPE_MASK) >> HCELL_TYPE_SHIFT;
   CellBlock = (CellIndex & HCELL_BLOCK_MASK) >> HCELL_BLOCK_SHIFT;

   /* FIXME: Merge free blocks */
   Bin = (PHBIN)RegistryHive->Storage[CellType].BlockList[CellBlock].Bin;

   if ((CellIndex & ~HCELL_TYPE_MASK) + Free->Size <
       Bin->FileOffset + Bin->Size)
   {
      Neighbor = (PHCELL)((ULONG_PTR)Free + Free->Size);
      if (Neighbor->Size > 0)
      {
         HvpRemoveFree(RegistryHive, Neighbor,
                       ((HCELL_INDEX)((ULONG_PTR)Neighbor - (ULONG_PTR)Bin +
                       Bin->FileOffset)) | (CellIndex & HCELL_TYPE_MASK));
         Free->Size += Neighbor->Size;
      }
   }

   Neighbor = (PHCELL)(Bin + 1);
   while (Neighbor < Free)
   {
      if (Neighbor->Size > 0)
      {
         if ((ULONG_PTR)Neighbor + Neighbor->Size == (ULONG_PTR)Free)
         {
            Neighbor->Size += Free->Size;
            if (CellType == HvStable)
               HvMarkCellDirty(RegistryHive,
                               (HCELL_INDEX)((ULONG_PTR)Neighbor - (ULONG_PTR)Bin +
                               Bin->FileOffset));
            return;
         }
         Neighbor = (PHCELL)((ULONG_PTR)Neighbor + Neighbor->Size);
      }
      else
      {
         Neighbor = (PHCELL)((ULONG_PTR)Neighbor - Neighbor->Size);
      }
   }

   /* Add block to the list of free blocks */
   HvpAddFree(RegistryHive, Free, CellIndex);

   if (CellType == HvStable)
      HvMarkCellDirty(RegistryHive, CellIndex);
}
