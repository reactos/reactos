/*
 * PROJECT:   Registry manipulation library
 * LICENSE:   GPL - See COPYING in the top level directory
 * COPYRIGHT: Copyright 2005 Filip Navara <navaraf@reactos.org>
 *            Copyright 2001 - 2005 Eric Kohl
 */

#include "cmlib.h"
#define NDEBUG
#include <debug.h>

/* DECLARATIONS *************************************************************/

#if !defined(CMLIB_HOST) && !defined(_BLDR_)
VOID
NTAPI
CmpLazyFlush(VOID);
#endif

/* FUNCTIONS *****************************************************************/

static __inline PHCELL CMAPI
HvpGetCellHeader(
    PHHIVE RegistryHive,
    HCELL_INDEX CellIndex)
{
    PVOID Block;

    CMLTRACE(CMLIB_HCELL_DEBUG, "%s - Hive 0x%p, CellIndex 0x%x\n",
             __FUNCTION__, RegistryHive, CellIndex);

    ASSERT(CellIndex != HCELL_NIL);
    if (!RegistryHive->Flat)
    {
        ULONG CellType   = HvGetCellType(CellIndex);
        ULONG CellBlock  = HvGetCellBlock(CellIndex);
        ULONG CellOffset = (CellIndex & HCELL_OFFSET_MASK) >> HCELL_OFFSET_SHIFT;

        ASSERT(CellBlock < RegistryHive->Storage[CellType].Length);
        Block = (PVOID)RegistryHive->Storage[CellType].BlockList[CellBlock].BlockAddress;
        ASSERT(Block != NULL);
        return (PHCELL)((ULONG_PTR)Block + CellOffset);
    }
    else
    {
        ASSERT(HvGetCellType(CellIndex) == Stable);
        return (PHCELL)((ULONG_PTR)RegistryHive->BaseBlock + HBLOCK_SIZE +
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

PCELL_DATA CMAPI
HvpGetCellData(
    _In_ PHHIVE Hive,
    _In_ HCELL_INDEX CellIndex)
{
    return (PCELL_DATA)(HvpGetCellHeader(Hive, CellIndex) + 1);
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

    CMLTRACE(CMLIB_HCELL_DEBUG, "%s - Hive 0x%p, CellIndex 0x%x, HoldingLock %u\n",
             __FUNCTION__, RegistryHive, CellIndex, HoldingLock);

    if (HvGetCellType(CellIndex) != Stable)
        return TRUE;

    CellBlock     = HvGetCellBlock(CellIndex);
    CellLastBlock = HvGetCellBlock(CellIndex + HBLOCK_SIZE - 1);

    RtlSetBits(&RegistryHive->DirtyVector,
               CellBlock, CellLastBlock - CellBlock);
    RegistryHive->DirtyCount++;

    /*
     * FIXME: Querying a lazy flush operation is needed to
     * ensure that the dirty data is being flushed to disk
     * accordingly. However, this operation has to be done
     * in a helper like HvMarkDirty that marks specific parts
     * of the hive as dirty. Since we do not have such kind
     * of helper we have to perform an eventual lazy flush
     * when marking cells as dirty here for the moment being,
     * so that not only we flush dirty data but also write
     * logs.
     */
#if !defined(CMLIB_HOST) && !defined(_BLDR_)
    if (!(RegistryHive->HiveFlags & HIVE_NOLAZYFLUSH))
    {
        CmpLazyFlush();
    }
#endif
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
    if (RtlCheckBit(&Hive->DirtyVector, Cell / HBLOCK_SIZE))
        IsDirty = TRUE;

    /* Return result as boolean*/
    return IsDirty;
}

static __inline ULONG CMAPI
HvpComputeFreeListIndex(
    ULONG Size)
{
    ULONG Index;
    static CCHAR FindFirstSet[128] = {
        0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
        4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
        5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
        6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6};

    ASSERT(Size >= (1 << 3));
    Index = (Size >> 3) - 1;
    if (Index >= 16)
    {
        if (Index > 127)
            Index = 23;
        else
            Index = FindFirstSet[Index] + 16;
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

    Storage = HvGetCellType(FreeIndex);
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

    Storage = HvGetCellType(CellIndex);
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
    CMLTRACE(CMLIB_HCELL_DEBUG, "chosen free list index: %u\n", Index);
    for (FreeListIndex = 0; FreeListIndex < 24; FreeListIndex++)
    {
        CMLTRACE(CMLIB_HCELL_DEBUG, "free list [%u]: ", FreeListIndex);
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

        BlockIndex += Bin->Size / HBLOCK_SIZE;
        BlockOffset += Bin->Size;
    }

    return STATUS_SUCCESS;
}

HCELL_INDEX CMAPI
HvAllocateCell(
    PHHIVE RegistryHive,
    ULONG Size,
    HSTORAGE_TYPE Storage,
    HCELL_INDEX Vicinity)
{
    PHCELL FreeCell;
    HCELL_INDEX FreeCellOffset;
    PHCELL NewCell;
    PHBIN Bin;

    ASSERT(RegistryHive->ReadOnly == FALSE);

    CMLTRACE(CMLIB_HCELL_DEBUG, "%s - Hive 0x%p, Size 0x%x, %s, Vicinity 0x%x\n",
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

    CMLTRACE(CMLIB_HCELL_DEBUG, "%s - CellIndex 0x%x\n",
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

    CMLTRACE(CMLIB_HCELL_DEBUG, "%s - Hive 0x%p, CellIndex 0x%x, Size 0x%x\n",
             __FUNCTION__, RegistryHive, CellIndex, Size);

    Storage = HvGetCellType(CellIndex);

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

    CMLTRACE(CMLIB_HCELL_DEBUG, "%s - Hive 0x%p, CellIndex 0x%x\n",
             __FUNCTION__, RegistryHive, CellIndex);

    Free = HvpGetCellHeader(RegistryHive, CellIndex);

    ASSERT(Free->Size < 0);

    Free->Size = -Free->Size;

    CellType = HvGetCellType(CellIndex);
    CellBlock = HvGetCellBlock(CellIndex);

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
                    ((HCELL_INDEX)((ULONG_PTR)Neighbor - (ULONG_PTR)Bin +
                     Bin->FileOffset)) | (CellIndex & HCELL_TYPE_MASK);

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


#define CELL_REF_INCREMENT  10

BOOLEAN
CMAPI
HvTrackCellRef(
    IN OUT PHV_TRACK_CELL_REF CellRef,
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell)
{
    PHV_HIVE_CELL_PAIR NewCellArray;

    PAGED_CODE();

    /* Sanity checks */
    ASSERT(CellRef);
    ASSERT(Hive);
    ASSERT(Cell != HCELL_NIL);

    /* NOTE: The hive cell is already referenced! */

    /* Less than 4? Use the static array */
    if (CellRef->StaticCount < STATIC_CELL_PAIR_COUNT)
    {
        /* Add the reference */
        CellRef->StaticArray[CellRef->StaticCount].Hive = Hive;
        CellRef->StaticArray[CellRef->StaticCount].Cell = Cell;
        CellRef->StaticCount++;
        return TRUE;
    }

    DPRINT("HvTrackCellRef: Static array full, use dynamic array.\n");

    /* Sanity checks */
    if (CellRef->Max == 0)
    {
        /* The dynamic array must not have been allocated already */
        ASSERT(CellRef->CellArray == NULL);
        ASSERT(CellRef->Count == 0);
    }
    else
    {
        /* The dynamic array must be allocated */
        ASSERT(CellRef->CellArray);
    }
    ASSERT(CellRef->Count <= CellRef->Max);

    if (CellRef->Count == CellRef->Max)
    {
        /* Allocate a new reference table */
        NewCellArray = CmpAllocate((CellRef->Max + CELL_REF_INCREMENT) * sizeof(HV_HIVE_CELL_PAIR),
                                   TRUE,
                                   TAG_CM);
        if (!NewCellArray)
        {
            DPRINT1("HvTrackCellRef: Cannot reallocate the reference table.\n");
            /* We failed, dereference the hive cell */
            HvReleaseCell(Hive, Cell);
            return FALSE;
        }

        /* Free the old reference table and use the new one */
        if (CellRef->CellArray)
        {
            /* Copy the handles from the old table to the new one */
            RtlCopyMemory(NewCellArray,
                          CellRef->CellArray,
                          CellRef->Max * sizeof(HV_HIVE_CELL_PAIR));
            CmpFree(CellRef->CellArray, 0); // TAG_CM
        }
        CellRef->CellArray = NewCellArray;
        CellRef->Max += CELL_REF_INCREMENT;
    }

    // ASSERT(CellRef->Count < CellRef->Max);

    /* Add the reference */
    CellRef->CellArray[CellRef->Count].Hive = Hive;
    CellRef->CellArray[CellRef->Count].Cell = Cell;
    CellRef->Count++;
    return TRUE;
}

VOID
CMAPI
HvReleaseFreeCellRefArray(
    IN OUT PHV_TRACK_CELL_REF CellRef)
{
    ULONG i;

    PAGED_CODE();

    ASSERT(CellRef);

    /* Any references in the static array? */
    if (CellRef->StaticCount > 0)
    {
        /* Sanity check */
        ASSERT(CellRef->StaticCount <= STATIC_CELL_PAIR_COUNT);

        /* Loop over them and release them */
        for (i = 0; i < CellRef->StaticCount; i++)
        {
            HvReleaseCell(CellRef->StaticArray[i].Hive,
                          CellRef->StaticArray[i].Cell);
        }

        /* We can reuse the static array */
        CellRef->StaticCount = 0;
    }

    /* Any references in the dynamic array? */
    if (CellRef->Count > 0)
    {
        /* Sanity checks */
        ASSERT(CellRef->Count <= CellRef->Max);
        ASSERT(CellRef->CellArray);

        /* Loop over them and release them */
        for (i = 0; i < CellRef->Count; i++)
        {
            HvReleaseCell(CellRef->CellArray[i].Hive,
                          CellRef->CellArray[i].Cell);
        }

        /* We can reuse the dynamic array */
        CmpFree(CellRef->CellArray, 0); // TAG_CM
        CellRef->CellArray = NULL;
        CellRef->Count = CellRef->Max = 0;
    }
}
