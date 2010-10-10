/*
 * PROJECT:   registry manipulation library
 * LICENSE:   GPL - See COPYING in the top level directory
 * COPYRIGHT: Copyright 2005 Filip Navara <navaraf@reactos.org>
 *            Copyright 2001 - 2005 Eric Kohl
 */

#include "cmlib.h"
#define NDEBUG
#include <debug.h>

static __inline PHCELL CMAPI
HvpGetCellHeader(
   PHHIVE RegistryHive,
   HCELL_INDEX CellIndex)
{
   PVOID Block;

   CMLTRACE(CMLIB_HCELL_DEBUG, "%s - Hive %p, CellIndex %08lx\n",
       __FUNCTION__, RegistryHive, CellIndex);

   ASSERT(CellIndex != HCELL_NIL);
   if (!RegistryHive->Flat)
   {
      ULONG CellType;
      ULONG CellBlock;
      ULONG CellOffset;

      CellType = (CellIndex & HCELL_TYPE_MASK) >> HCELL_TYPE_SHIFT;
      CellBlock = (CellIndex & HCELL_BLOCK_MASK) >> HCELL_BLOCK_SHIFT;
      CellOffset = (CellIndex & HCELL_OFFSET_MASK) >> HCELL_OFFSET_SHIFT;
      ASSERT(CellBlock < RegistryHive->Storage[CellType].Length);
      Block = (PVOID)RegistryHive->Storage[CellType].BlockList[CellBlock].BlockAddress;
      ASSERT(Block != NULL);
      return (PVOID)((ULONG_PTR)Block + CellOffset);
   }
   else
   {
      ASSERT((CellIndex & HCELL_TYPE_MASK) == Stable);
      return (PVOID)((ULONG_PTR)RegistryHive->BaseBlock + HV_BLOCK_SIZE +
                     CellIndex);
   }
}

BOOLEAN CMAPI
HvIsCellAllocated(IN PHHIVE RegistryHive,
                  IN HCELL_INDEX CellIndex)
{
   ULONG Type, Block;

   /* If it's a flat hive, the cell is always allocated */
   if (RegistryHive->Flat)
      return TRUE;

   /* Otherwise, get the type and make sure it's valid */
   Type = HvGetCellType(CellIndex);
   Block = HvGetCellBlock(CellIndex);
   if (Block >= RegistryHive->Storage[Type].Length)
      return FALSE;

   /* Try to get the cell block */
   if (RegistryHive->Storage[Type].BlockList[Block].BlockAddress)
      return TRUE;

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

static __inline LONG CMAPI
HvpGetCellFullSize(
   PHHIVE RegistryHive,
   PVOID Cell)
{
   UNREFERENCED_PARAMETER(RegistryHive);
   return ((PHCELL)Cell - 1)->Size;
}

LONG CMAPI
HvGetCellSize(IN PHHIVE Hive,
              IN PVOID Address)
{
   PHCELL CellHeader;
   LONG Size;

   UNREFERENCED_PARAMETER(Hive);

   CellHeader = (PHCELL)Address - 1;
   Size = CellHeader->Size * -1;
   Size -= sizeof(HCELL);
   return Size;
}

BOOLEAN CMAPI
HvMarkCellDirty(
   PHHIVE RegistryHive,
   HCELL_INDEX CellIndex,
   BOOLEAN HoldingLock)
{
   ULONG CellBlock;
   ULONG CellLastBlock;

   ASSERT(RegistryHive->ReadOnly == FALSE);

   CMLTRACE(CMLIB_HCELL_DEBUG, "%s - Hive %p, CellIndex %08lx, HoldingLock %b\n",
       __FUNCTION__, RegistryHive, CellIndex, HoldingLock);

   if ((CellIndex & HCELL_TYPE_MASK) >> HCELL_TYPE_SHIFT != Stable)
      return FALSE;

   CellBlock = (CellIndex & HCELL_BLOCK_MASK) >> HCELL_BLOCK_SHIFT;
   CellLastBlock = ((CellIndex + HV_BLOCK_SIZE - 1) & HCELL_BLOCK_MASK) >> HCELL_BLOCK_SHIFT;

   RtlSetBits(&RegistryHive->DirtyVector,
              CellBlock, CellLastBlock - CellBlock);
   return TRUE;
}

BOOLEAN CMAPI
HvIsCellDirty(IN PHHIVE Hive,
              IN HCELL_INDEX Cell)
{
   BOOLEAN IsDirty = FALSE;

   /* Sanity checks */
   ASSERT(Hive->ReadOnly == FALSE);

   /* Volatile cells are always "dirty" */
   if (HvGetCellType(Cell) == Volatile)
      return TRUE;

   /* Check if the dirty bit is set */
   if (RtlCheckBit(&Hive->DirtyVector, Cell / HV_BLOCK_SIZE))
       IsDirty = TRUE;

   /* Return result as boolean*/
   return IsDirty;
}

static __inline ULONG CMAPI
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
   HSTORAGE_TYPE Storage;
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
   HSTORAGE_TYPE Storage;
   ULONG Index, FreeListIndex;

   ASSERT(RegistryHive->ReadOnly == FALSE);

   Storage = (CellIndex & HCELL_TYPE_MASK) >> HCELL_TYPE_SHIFT;
   Index = HvpComputeFreeListIndex((ULONG)CellBlock->Size);

   pFreeCellOffset = &RegistryHive->Storage[Storage].FreeDisplay[Index];
   while (*pFreeCellOffset != HCELL_NIL)
   {
      FreeCellData = (PHCELL_INDEX)HvGetCell(RegistryHive, *pFreeCellOffset);
      if (*pFreeCellOffset == CellIndex)
      {
         *pFreeCellOffset = *FreeCellData;
         return;
      }
      pFreeCellOffset = FreeCellData;
   }

   /* Something bad happened, print a useful trace info and bugcheck */
   CMLTRACE(CMLIB_HCELL_DEBUG, "-- beginning of HvpRemoveFree trace --\n");
   CMLTRACE(CMLIB_HCELL_DEBUG, "block we are about to free: %08x\n", CellIndex);
   CMLTRACE(CMLIB_HCELL_DEBUG, "chosen free list index: %d\n", Index);
   for (FreeListIndex = 0; FreeListIndex < 24; FreeListIndex++)
   {
      CMLTRACE(CMLIB_HCELL_DEBUG, "free list [%d]: ", FreeListIndex);
      pFreeCellOffset = &RegistryHive->Storage[Storage].FreeDisplay[FreeListIndex];
      while (*pFreeCellOffset != HCELL_NIL)
      {
         CMLTRACE(CMLIB_HCELL_DEBUG, "%08x ", *pFreeCellOffset);
         FreeCellData = (PHCELL_INDEX)HvGetCell(RegistryHive, *pFreeCellOffset);
         pFreeCellOffset = FreeCellData;
      }
      CMLTRACE(CMLIB_HCELL_DEBUG, "\n");
   }
   CMLTRACE(CMLIB_HCELL_DEBUG, "-- end of HvpRemoveFree trace --\n");

   ASSERT(FALSE);
}

static HCELL_INDEX CMAPI
HvpFindFree(
   PHHIVE RegistryHive,
   ULONG Size,
   HSTORAGE_TYPE Storage)
{
   PHCELL_INDEX FreeCellData;
   HCELL_INDEX FreeCellOffset;
   PHCELL_INDEX pFreeCellOffset;
   ULONG Index;

   for (Index = HvpComputeFreeListIndex(Size); Index < 24; Index++)
   {
      pFreeCellOffset = &RegistryHive->Storage[Storage].FreeDisplay[Index];
      while (*pFreeCellOffset != HCELL_NIL)
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

   return HCELL_NIL;
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
      Hive->Storage[Stable].FreeDisplay[Index] = HCELL_NIL;
      Hive->Storage[Volatile].FreeDisplay[Index] = HCELL_NIL;
   }

   BlockOffset = 0;
   BlockIndex = 0;
   while (BlockIndex < Hive->Storage[Stable].Length)
   {
      Bin = (PHBIN)Hive->Storage[Stable].BlockList[BlockIndex].BinAddress;

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
   HSTORAGE_TYPE Storage,
   HCELL_INDEX Vicinity)
{
   PHCELL FreeCell;
   HCELL_INDEX FreeCellOffset;
   PHCELL NewCell;
   PHBIN Bin;

   ASSERT(RegistryHive->ReadOnly == FALSE);

   CMLTRACE(CMLIB_HCELL_DEBUG, "%s - Hive %p, Size %x, %s, Vicinity %08lx\n",
       __FUNCTION__, RegistryHive, Size, (Storage == 0) ? "Stable" : "Volatile", Vicinity);

   /* Round to 16 bytes multiple. */
   Size = ROUND_UP(Size + sizeof(HCELL), 16);

   /* First search in free blocks. */
   FreeCellOffset = HvpFindFree(RegistryHive, Size, Storage);

   /* If no free cell was found we need to extend the hive file. */
   if (FreeCellOffset == HCELL_NIL)
   {
      Bin = HvpAddBin(RegistryHive, Size, Storage);
      if (Bin == NULL)
         return HCELL_NIL;
      FreeCellOffset = Bin->FileOffset + sizeof(HBIN);
      FreeCellOffset |= Storage << HCELL_TYPE_SHIFT;
   }

   FreeCell = HvpGetCellHeader(RegistryHive, FreeCellOffset);

   /* Split the block in two parts */

   /* The free block that is created has to be at least
      sizeof(HCELL) + sizeof(HCELL_INDEX) big, so that free
      cell list code can work. Moreover we round cell sizes
      to 16 bytes, so creating a smaller block would result in
      a cell that would never be allocated. */
   if ((ULONG)FreeCell->Size > Size + 16)
   {
      NewCell = (PHCELL)((ULONG_PTR)FreeCell + Size);
      NewCell->Size = FreeCell->Size - Size;
      FreeCell->Size = Size;
      HvpAddFree(RegistryHive, NewCell, FreeCellOffset + Size);
      if (Storage == Stable)
         HvMarkCellDirty(RegistryHive, FreeCellOffset + Size, FALSE);
   }

   if (Storage == Stable)
      HvMarkCellDirty(RegistryHive, FreeCellOffset, FALSE);
   FreeCell->Size = -FreeCell->Size;
   RtlZeroMemory(FreeCell + 1, Size - sizeof(HCELL));

   CMLTRACE(CMLIB_HCELL_DEBUG, "%s - CellIndex %08lx\n",
       __FUNCTION__, FreeCellOffset);

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
   HSTORAGE_TYPE Storage;

   ASSERT(CellIndex != HCELL_NIL);

   CMLTRACE(CMLIB_HCELL_DEBUG, "%s - Hive %p, CellIndex %08lx, Size %x\n",
       __FUNCTION__, RegistryHive, CellIndex, Size);

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
   if (Size > (ULONG)OldCellSize)
   {
      NewCellIndex = HvAllocateCell(RegistryHive, Size, Storage, HCELL_NIL);
      if (NewCellIndex == HCELL_NIL)
         return HCELL_NIL;

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

   CMLTRACE(CMLIB_HCELL_DEBUG, "%s - Hive %p, CellIndex %08lx\n",
       __FUNCTION__, RegistryHive, CellIndex);

   Free = HvpGetCellHeader(RegistryHive, CellIndex);

   ASSERT(Free->Size < 0);

   Free->Size = -Free->Size;

   CellType = (CellIndex & HCELL_TYPE_MASK) >> HCELL_TYPE_SHIFT;
   CellBlock = (CellIndex & HCELL_BLOCK_MASK) >> HCELL_BLOCK_SHIFT;

   /* FIXME: Merge free blocks */
   Bin = (PHBIN)RegistryHive->Storage[CellType].BlockList[CellBlock].BinAddress;

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
            HCELL_INDEX NeighborCellIndex =
               (HCELL_INDEX)((ULONG_PTR)Neighbor - (ULONG_PTR)Bin +
               Bin->FileOffset) | (CellIndex & HCELL_TYPE_MASK);

            if (HvpComputeFreeListIndex(Neighbor->Size) !=
                HvpComputeFreeListIndex(Neighbor->Size + Free->Size))
            {
               HvpRemoveFree(RegistryHive, Neighbor, NeighborCellIndex);
               Neighbor->Size += Free->Size;
               HvpAddFree(RegistryHive, Neighbor, NeighborCellIndex);
            }
            else
               Neighbor->Size += Free->Size;

            if (CellType == Stable)
               HvMarkCellDirty(RegistryHive, NeighborCellIndex, FALSE);

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

   if (CellType == Stable)
      HvMarkCellDirty(RegistryHive, CellIndex, FALSE);
}
