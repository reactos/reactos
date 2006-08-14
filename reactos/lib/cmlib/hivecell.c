/*
 * PROJECT:   registry manipulation library
 * LICENSE:   GPL - See COPYING in the top level directory
 * COPYRIGHT: Copyright 2005 Filip Navara <navaraf@reactos.org>
 *            Copyright 2001 - 2005 Eric Kohl
 */

#include "cmlib.h"
#define NDEBUG
#include <debug.h>

static PCELL_HEADER __inline CMAPI
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
   return ((PCELL_HEADER)Cell - 1)->CellSize;
}

LONG CMAPI
HvGetCellSize(
   PHHIVE RegistryHive,
   PVOID Cell)
{
   PCELL_HEADER CellHeader;

   CellHeader = (PCELL_HEADER)Cell - 1;
   if (CellHeader->CellSize < 0)
      return CellHeader->CellSize + sizeof(CELL_HEADER);
   else
      return CellHeader->CellSize - sizeof(CELL_HEADER);
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
   PCELL_HEADER FreeBlock,
   HCELL_INDEX FreeIndex)
{
   PHCELL_INDEX FreeBlockData;
   HV_STORAGE_TYPE Storage;
   ULONG Index;

   ASSERT(RegistryHive != NULL);
   ASSERT(FreeBlock != NULL);

   Storage = (FreeIndex & HCELL_TYPE_MASK) >> HCELL_TYPE_SHIFT;
   Index = HvpComputeFreeListIndex(FreeBlock->CellSize);

   FreeBlockData = (PHCELL_INDEX)(FreeBlock + 1);
   *FreeBlockData = RegistryHive->Storage[Storage].FreeDisplay[Index];
   RegistryHive->Storage[Storage].FreeDisplay[Index] = FreeIndex;

   /* FIXME: Eventually get rid of free bins. */

   return STATUS_SUCCESS;
}

static VOID CMAPI
HvpRemoveFree(
   PHHIVE RegistryHive,
   PCELL_HEADER CellBlock,
   HCELL_INDEX CellIndex)
{
   PHCELL_INDEX FreeCellData;
   PHCELL_INDEX pFreeCellOffset;
   HV_STORAGE_TYPE Storage;
   ULONG Index;

   ASSERT(RegistryHive->ReadOnly == FALSE);

   Storage = (CellIndex & HCELL_TYPE_MASK) >> HCELL_TYPE_SHIFT;
   Index = HvpComputeFreeListIndex(CellBlock->CellSize);

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

   ASSERT(FALSE);
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
         if (HvpGetCellFullSize(RegistryHive, FreeCellData) >= Size)
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
   PCELL_HEADER FreeBlock;
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
      while (FreeOffset < Bin->BinSize)
      {
         FreeBlock = (PCELL_HEADER)((ULONG_PTR)Bin + FreeOffset);
         if (FreeBlock->CellSize > 0)
         {
            Status = HvpAddFree(Hive, FreeBlock, Bin->BinOffset + FreeOffset);
            if (!NT_SUCCESS(Status))
               return Status;

            FreeOffset += FreeBlock->CellSize;
         }
         else
         {
            FreeOffset -= FreeBlock->CellSize;
         }
      }

      BlockIndex += Bin->BinSize / HV_BLOCK_SIZE;
      BlockOffset += Bin->BinSize;
   }

   return STATUS_SUCCESS;
}

HCELL_INDEX CMAPI
HvAllocateCell(
   PHHIVE RegistryHive,
   ULONG Size,
   HV_STORAGE_TYPE Storage)
{
   PCELL_HEADER FreeCell;
   HCELL_INDEX FreeCellOffset;
   PCELL_HEADER NewCell;
   PHBIN Bin;

   ASSERT(RegistryHive->ReadOnly == FALSE);

   /* Round to 16 bytes multiple. */
   Size = ROUND_UP(Size + sizeof(CELL_HEADER), 16);

   /* First search in free blocks. */
   FreeCellOffset = HvpFindFree(RegistryHive, Size, Storage);

   /* If no free cell was found we need to extend the hive file. */
   if (FreeCellOffset == HCELL_NULL)
   {
      Bin = HvpAddBin(RegistryHive, Size, Storage);
      if (Bin == NULL)
         return HCELL_NULL;
      FreeCellOffset = Bin->BinOffset + sizeof(HBIN);
      FreeCellOffset |= Storage << HCELL_TYPE_SHIFT;
   }

   FreeCell = HvpGetCellHeader(RegistryHive, FreeCellOffset);

   /* Split the block in two parts */
   /* FIXME: There is some minimal cell size that we must respect. */
   if (FreeCell->CellSize > Size + sizeof(HCELL_INDEX))
   {
      NewCell = (PCELL_HEADER)((ULONG_PTR)FreeCell + Size);
      NewCell->CellSize = FreeCell->CellSize - Size;
      FreeCell->CellSize = Size;
      HvpAddFree(RegistryHive, NewCell, FreeCellOffset + Size);
      if (Storage == HvStable)
         HvMarkCellDirty(RegistryHive, FreeCellOffset + Size);
   }

   if (Storage == HvStable)
      HvMarkCellDirty(RegistryHive, FreeCellOffset);
   FreeCell->CellSize = -FreeCell->CellSize;
   RtlZeroMemory(FreeCell + 1, Size - sizeof(CELL_HEADER));

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
   ASSERT(OldCellSize < 0);
  
   /*
    * If new data size is larger than the current, destroy current
    * data block and allocate a new one.
    *
    * FIXME: Merge with adjacent free cell if possible.
    * FIXME: Implement shrinking.
    */
   if (Size > -OldCellSize)
   {
      NewCellIndex = HvAllocateCell(RegistryHive, Size, Storage);
      if (NewCellIndex == HCELL_NULL)
         return HCELL_NULL;

      NewCell = HvGetCell(RegistryHive, NewCellIndex);
      RtlCopyMemory(NewCell, OldCell, -OldCellSize);
      
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
   PCELL_HEADER Free;
   PCELL_HEADER Neighbor;
   PHBIN Bin;
   ULONG CellType;
   ULONG CellBlock;

   ASSERT(RegistryHive->ReadOnly == FALSE);
   
   Free = HvpGetCellHeader(RegistryHive, CellIndex);

   ASSERT(Free->CellSize < 0);   
   
   Free->CellSize = -Free->CellSize;

   CellType = (CellIndex & HCELL_TYPE_MASK) >> HCELL_TYPE_SHIFT;
   CellBlock = (CellIndex & HCELL_BLOCK_MASK) >> HCELL_BLOCK_SHIFT;

   /* FIXME: Merge free blocks */
   Bin = (PHBIN)RegistryHive->Storage[CellType].BlockList[CellBlock].Bin;

   if ((CellIndex & ~HCELL_TYPE_MASK) + Free->CellSize <
       Bin->BinOffset + Bin->BinSize)
   {
      Neighbor = (PCELL_HEADER)((ULONG_PTR)Free + Free->CellSize);
      if (Neighbor->CellSize > 0)
      {
         HvpRemoveFree(RegistryHive, Neighbor,
                       ((HCELL_INDEX)Neighbor - (HCELL_INDEX)Bin +
                       Bin->BinOffset) | (CellIndex & HCELL_TYPE_MASK));
         Free->CellSize += Neighbor->CellSize;
      }
   }

   Neighbor = (PCELL_HEADER)(Bin + 1);
   while (Neighbor < Free)
   {
      if (Neighbor->CellSize > 0)
      {
         if ((ULONG_PTR)Neighbor + Neighbor->CellSize == (ULONG_PTR)Free)
         {
            Neighbor->CellSize += Free->CellSize;
            if (CellType == HvStable)
               HvMarkCellDirty(RegistryHive,
                               (HCELL_INDEX)Neighbor - (HCELL_INDEX)Bin +
                               Bin->BinOffset);
            return;
         }
         Neighbor = (PCELL_HEADER)((ULONG_PTR)Neighbor + Neighbor->CellSize);
      }
      else
      {
         Neighbor = (PCELL_HEADER)((ULONG_PTR)Neighbor - Neighbor->CellSize);
      }
   }

   /* Add block to the list of free blocks */
   HvpAddFree(RegistryHive, Free, CellIndex);

   if (CellType == HvStable)
      HvMarkCellDirty(RegistryHive, CellIndex);
}
