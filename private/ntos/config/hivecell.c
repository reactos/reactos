/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    hivecell.c

Abstract:

    This module implements hive cell procedures.

Author:

    Bryan M. Willman (bryanwi) 27-Mar-92

Environment:


Revision History:
    Dragos C. Sambotin (dragoss) 22-Dec-98
        Requests for cells bigger than 1K are doubled. This way 
        we avoid fragmentation and we make the value-growing 
        process more flexible.
    Dragos C. Sambotin (dragoss) 13-Jan-99
        At boot time, order the free cells list ascending.

--*/

#include    "cmp.h"

//
// Private procedures
//
HCELL_INDEX
HvpDoAllocateCell(
    PHHIVE          Hive,
    ULONG           NewSize,
    HSTORAGE_TYPE   Type
    );

ULONG
HvpAllocateInBin(
    PHHIVE  Hive,
    PHBIN   Bin,
    ULONG   Size,
    ULONG   Type
    );

BOOLEAN
HvpMakeBinPresent(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell,
    IN PHMAP_ENTRY Map
    );

BOOLEAN
HvpIsFreeNeighbor(
    PHHIVE  Hive,
    PHBIN   Bin,
    PHCELL  FreeCell,
    PHCELL  *FreeNeighbor,
    HSTORAGE_TYPE Type
    );

VOID
HvpDelistBinFreeCells(
    PHHIVE  Hive,
    PHBIN   Bin,
    HSTORAGE_TYPE Type,
    PHCELL_INDEX TailDisplay OPTIONAL
    );

#define CmpFindFirstSetRight KiFindFirstSetRight
extern CCHAR KiFindFirstSetRight[256];
#define CmpFindFirstSetLeft KiFindFirstSetLeft
extern CCHAR KiFindFirstSetLeft[256];

#define HvpComputeIndex(Index, Size)                                    \
    {                                                                   \
        Index = (Size >> HHIVE_FREE_DISPLAY_SHIFT) - 1;                 \
        if (Index >= HHIVE_LINEAR_INDEX ) {                             \
                                                                        \
            /*                                                          \
            ** Too big for the linear lists, compute the exponential    \
            ** list.                                                    \
            */                                                          \
                                                                        \
            if (Index > 255) {                                          \
                /*                                                      \
                ** Too big for all the lists, use the last index.       \
                */                                                      \
                Index = HHIVE_FREE_DISPLAY_SIZE-1;                      \
            } else {                                                    \
                Index = CmpFindFirstSetLeft[Index] +                    \
                        HHIVE_FREE_DISPLAY_BIAS;                        \
            }                                                           \
        }                                                               \
    }

#define ONE_K   1024

//  Double requests bigger  than 1KB                       
//  CmpSetValueKeyExisting  always allocates a bigger data 
//  value cell  exactly the required size. This creates    
//  problems when somebody  slowly grows a value one DWORD 
//  at a time to  some enormous size. An easy fix for this 
//  would be to set a  certain threshold (like 1K). Once a 
//  value size  crosses that threshold, allocate a new cell
//  that is twice  the old size. So the actual allocated   
//  size  would grow to 1k, then 2k, 4k, 8k, 16k, 32k,etc. 
//  This will reduce the fragmentation.                    

#define HvpAdjustCellSize(Size)                                         \
    {                                                                   \
        ULONG   onek = ONE_K;                                           \
        ULONG   Limit = 0;                                              \
                                                                        \
        while( Size > onek ) {                                          \
            onek<<=1;                                                   \
            Limit++;                                                    \
        }                                                               \
                                                                        \
        Size = Limit?onek:Size;                                         \
    }   


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,HvpGetCellPaged)
#pragma alloc_text(PAGE,HvpGetCellFlat)
#pragma alloc_text(PAGE,HvpGetCellMap)
#pragma alloc_text(PAGE,HvGetCellSize)
#pragma alloc_text(PAGE,HvAllocateCell)
#pragma alloc_text(PAGE,HvpDoAllocateCell)
#pragma alloc_text(PAGE,HvFreeCell)
#pragma alloc_text(PAGE,HvpIsFreeNeighbor)
#pragma alloc_text(PAGE,HvpEnlistFreeCell)
#pragma alloc_text(PAGE,HvpDelistFreeCell)
#pragma alloc_text(PAGE,HvReallocateCell)
#pragma alloc_text(PAGE,HvIsCellAllocated)
#pragma alloc_text(PAGE,HvpAllocateInBin)
#pragma alloc_text(PAGE,HvpMakeBinPresent)
#pragma alloc_text(PAGE,HvpDelistBinFreeCells)
#endif


//
//  Cell Procedures
//
struct _CELL_DATA *
HvpGetCellPaged(
    PHHIVE      Hive,
    HCELL_INDEX Cell
    )
/*++

Routine Description:

    Returns the memory address for the specified Cell.  Will never
    return failure, but may assert.  Use HvIsCellAllocated to check
    validity of Cell.

    This routine should never be called directly, always call it
    via the HvGetCell() macro.

    This routine provides GetCell support for hives with full maps.
    It is the normal version of the routine.

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    Cell - supplies HCELL_INDEX of cell to return address for

Return Value:

    Address of Cell in memory.  Assert or BugCheck if error.

--*/
{
    ULONG           Type;
    ULONG           Table;
    ULONG           Block;
    ULONG           Offset;
    PHCELL          pcell;
    PHMAP_ENTRY     Map;

    CMLOG(CML_FLOW, CMS_MAP) {
        KdPrint(("HvGetCellPaged:\n"));
        KdPrint(("\tHive=%08lx Cell=%08lx\n",Hive,Cell));
    }
    ASSERT(Hive->Signature == HHIVE_SIGNATURE);
    ASSERT(Cell != HCELL_NIL);
    ASSERT(Hive->Flat == FALSE);
    ASSERT((Cell & (HCELL_PAD(Hive)-1))==0);
    ASSERT_CM_LOCK_OWNED();
    #if DBG
        if (HvGetCellType(Cell) == Stable) {
            ASSERT(Cell >= sizeof(HBIN));
        } else {
            ASSERT(Cell >= (HCELL_TYPE_MASK + sizeof(HBIN)));
        }
    #endif

    Type = HvGetCellType(Cell);
    Table = (Cell & HCELL_TABLE_MASK) >> HCELL_TABLE_SHIFT;
    Block = (Cell & HCELL_BLOCK_MASK) >> HCELL_BLOCK_SHIFT;
    Offset = (Cell & HCELL_OFFSET_MASK);

    ASSERT((Cell - (Type * HCELL_TYPE_MASK)) < Hive->Storage[Type].Length);

    Map = &((Hive->Storage[Type].Map)->Directory[Table]->Table[Block]);
    ASSERT((Map->BinAddress & HMAP_BASE) != 0);
    ASSERT((Map->BinAddress & HMAP_DISCARDABLE) == 0);
    pcell = (PHCELL)((ULONG_PTR)(Map->BlockAddress) + Offset);
    if (USE_OLD_CELL(Hive)) {
        return (struct _CELL_DATA *)&(pcell->u.OldCell.u.UserData);
    } else {
        return (struct _CELL_DATA *)&(pcell->u.NewCell.u.UserData);
    }
}


struct _CELL_DATA *
HvpGetCellFlat(
    PHHIVE      Hive,
    HCELL_INDEX Cell
    )
/*++

Routine Description:

    Returns the memory address for the specified Cell.  Will never
    return failure, but may assert.  Use HvIsCellAllocated to check
    validity of Cell.

    This routine should never be called directly, always call it
    via the HvGetCell() macro.

    This routine provides GetCell support for read only hives with
    single allocation flat images.  Such hives do not have cell
    maps ("page tables"), instead, we compute addresses by
    arithmetic against the base image address.

    Such hives cannot have volatile cells.

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    Cell - supplies HCELL_INDEX of cell to return address for

Return Value:

    Address of Cell in memory.  Assert or BugCheck if error.

--*/
{
    PUCHAR          base;
    PHCELL          pcell;

    CMLOG(CML_FLOW, CMS_MAP) {
        KdPrint(("HvGetCellFlat:\n"));
        KdPrint(("\tHive=%08lx Cell=%08lx\n",Hive,Cell));
    }
    ASSERT(Hive->Signature == HHIVE_SIGNATURE);
    ASSERT(Cell != HCELL_NIL);
    ASSERT(Hive->Flat == TRUE);
    ASSERT(HvGetCellType(Cell) == Stable);
    ASSERT(Cell >= sizeof(HBIN));
    ASSERT(Cell < Hive->BaseBlock->Length);
    ASSERT((Cell & 0x7)==0);

    //
    // Address is base of Hive image + Cell
    //
    base = (PUCHAR)(Hive->BaseBlock) + HBLOCK_SIZE;
    pcell = (PHCELL)(base + Cell);
    if (USE_OLD_CELL(Hive)) {
        return (struct _CELL_DATA *)&(pcell->u.OldCell.u.UserData);
    } else {
        return (struct _CELL_DATA *)&(pcell->u.NewCell.u.UserData);
    }
}


PHMAP_ENTRY
HvpGetCellMap(
    PHHIVE      Hive,
    HCELL_INDEX Cell
    )
/*++

Routine Description:

    Returns the address of the HMAP_ENTRY for the cell.

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    Cell - supplies HCELL_INDEX of cell to return map entry address for

Return Value:

    Address of MAP_ENTRY in memory.  NULL if no such cell or other error.

--*/
{
    ULONG           Type;
    ULONG           Table;
    ULONG           Block;
    PHMAP_TABLE     ptab;

    CMLOG(CML_FLOW, CMS_MAP) {
        KdPrint(("HvpGetCellMapPaged:\n"));
        KdPrint(("\tHive=%08lx Cell=%08lx\n",Hive,Cell));
    }
    ASSERT(Hive->Signature == HHIVE_SIGNATURE);
    ASSERT(Hive->Flat == FALSE);
    ASSERT((Cell & (HCELL_PAD(Hive)-1))==0);

    Type = HvGetCellType(Cell);
    Table = (Cell & HCELL_TABLE_MASK) >> HCELL_TABLE_SHIFT;
    Block = (Cell & HCELL_BLOCK_MASK) >> HCELL_BLOCK_SHIFT;

    if ((Cell - (Type * HCELL_TYPE_MASK)) >= Hive->Storage[Type].Length) {
        return NULL;
    }

    ptab = (Hive->Storage[Type].Map)->Directory[Table];
    return &(ptab->Table[Block]);
}


LONG
HvGetCellSize(
    IN PHHIVE   Hive,
    IN PVOID    Address
    )
/*++

Routine Description:

    Returns the size of the specified Cell, based on its MEMORY
    ADDRESS.  Must always call HvGetCell first to get that
    address.

    NOTE:   This should be a macro if speed is issue.

    NOTE:   If you pass in some random pointer, you will get some
            random answer.  Only pass in valid Cell addresses.

Arguments:

    Hive - supplies hive control structure for the given cell

    Address - address in memory of the cell, returned by HvGetCell()

Return Value:

    Allocated size in bytes of the cell.

    If Negative, Cell is free, or Address is bogus.

--*/
{
    LONG    size;

    CMLOG(CML_FLOW, CMS_MAP) {
        KdPrint(("HvGetCellSize:\n"));
        KdPrint(("\tAddress=%08lx\n", Address));
    }

    if (USE_OLD_CELL(Hive)) {
        size = ( (CONTAINING_RECORD(Address, HCELL, u.OldCell.u.UserData))->Size ) * -1;
        size -= FIELD_OFFSET(HCELL, u.OldCell.u.UserData);
    } else {
        size = ( (CONTAINING_RECORD(Address, HCELL, u.NewCell.u.UserData))->Size ) * -1;
        size -= FIELD_OFFSET(HCELL, u.NewCell.u.UserData);
    }
    return size;
}



HCELL_INDEX
HvAllocateCell(
    PHHIVE          Hive,
    ULONG           NewSize,
    HSTORAGE_TYPE   Type
    )
/*++

Routine Description:

    Allocates the space and the cell index for a new cell.

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    NewSize - size in bytes of the cell to allocate

    Type - indicates whether Stable or Volatile storage is desired.

Return Value:

    New HCELL_INDEX if success, HCELL_NIL if failure.

--*/
{
    HCELL_INDEX NewCell;

    CMLOG(CML_MAJOR, CMS_HIVE) {
        KdPrint(("HvAllocateCell:\n"));
        KdPrint(("\tHive=%08lx NewSize=%08lx\n",Hive,NewSize));
    }
    ASSERT(Hive->Signature == HHIVE_SIGNATURE);
    ASSERT(Hive->ReadOnly == FALSE);
    ASSERT_CM_LOCK_OWNED_EXCLUSIVE();

    //
    // Make room for overhead fields and round up to HCELL_PAD boundary
    //
    if (USE_OLD_CELL(Hive)) {
        NewSize += FIELD_OFFSET(HCELL, u.OldCell.u.UserData);
    } else {
        NewSize += FIELD_OFFSET(HCELL, u.NewCell.u.UserData);
    }
    NewSize = ROUND_UP(NewSize, HCELL_PAD(Hive));

    // 
    // Adjust the size (an easy fix for granularity)
    //
    HvpAdjustCellSize(NewSize);
    //
    // reject impossible/unreasonable values
    //
    if (NewSize > HSANE_CELL_MAX) {
        return HCELL_NIL;
    }

    //
    // Do the actual storage allocation
    //
    NewCell = HvpDoAllocateCell(Hive, NewSize, Type);

#if DBG
    if (NewCell != HCELL_NIL) {
        ASSERT(HvIsCellAllocated(Hive, NewCell));
    }
#endif


    CMLOG(CML_FLOW, CMS_HIVE) {
        KdPrint(("\tNewCell=%08lx\n", NewCell));
    }
    return NewCell;
}


HCELL_INDEX
HvpDoAllocateCell(
    PHHIVE          Hive,
    ULONG           NewSize,
    HSTORAGE_TYPE   Type
    )
/*++

Routine Description:

    Allocates space in the hive.  Does not affect cell map in any way.

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    NewSize - size in bytes of the cell to allocate

    Type - indicates whether Stable or Volatile storage is desired.

Return Value:

    HCELL_INDEX of new cell, HCELL_NIL if failure

--*/
{
    ULONG       Index;
    ULONG       Summary;
    HCELL_INDEX cell;
    PHCELL      pcell;
    HCELL_INDEX tcell;
    PHCELL      ptcell;
    PHBIN       Bin;
    PHMAP_ENTRY Me;
    ULONG       offset;
    PHCELL      next;
    ULONG       MinFreeSize;


    CMLOG(CML_MINOR, CMS_HIVE) {
        KdPrint(("HvDoAllocateCell:\n"));
        KdPrint(("\tHive=%08lx NewSize=%08lx Type=%08lx\n",Hive,NewSize,Type));
    }
    ASSERT(Hive->ReadOnly == FALSE);


    //
    // Compute Index into Display
    //
    HvpComputeIndex(Index, NewSize);

    //
    // Compute Summary vector of Display entries that are non null
    //
    Summary = Hive->Storage[Type].FreeSummary;
    Summary = Summary & ~((1 << Index) - 1);

    //
    // We now have a summary of lists that are non-null and may
    // contain entries large enough to satisfy the request.
    // Iterate through the list and pull the first cell that is
    // big enough.  If no cells are big enough, advance to the
    // next non-null list.
    //

    ASSERT(HHIVE_FREE_DISPLAY_SIZE == 24);
    while (Summary != 0) {
        if (Summary & 0xff) {
            Index = CmpFindFirstSetRight[Summary & 0xff];
        } else if (Summary & 0xff00) {
            Index = CmpFindFirstSetRight[(Summary & 0xff00) >> 8] + 8;
        } else  {
            ASSERT(Summary & 0xff0000);
            Index = CmpFindFirstSetRight[(Summary & 0xff0000) >> 16] + 16;
        }

        //
        // Walk through the list until we find a cell large enough
        // to satisfy the allocation.  If we find one, pull it from
        // the list and use it.  If we don't find one, clear this
        // list's bit in the Summary and try the next larger list.
        //

        //
        // look for a large enough cell in the list
        //
        cell = Hive->Storage[Type].FreeDisplay[Index];
        while (cell != HCELL_NIL) {

            pcell = HvpGetHCell(Hive, cell);

            if (NewSize <= (ULONG)pcell->Size) {

                //
                // Found a big enough cell.
                //
                if (! HvMarkCellDirty(Hive, cell)) {
                    return HCELL_NIL;
                }

                HvpDelistFreeCell(Hive, pcell, Type, NULL);

                ASSERT(pcell->Size > 0);
                ASSERT(NewSize <= (ULONG)pcell->Size);
                goto UseIt;
            }
//            DbgPrint("cell %08lx (%lx) too small (%d < %d), trying next at",cell,pcell,pcell->Size,NewSize);
            if (USE_OLD_CELL(Hive)) {
                cell = pcell->u.OldCell.u.Next;
            } else {
                cell = pcell->u.NewCell.u.Next;
            }
//          DbgPrint(" %08lx\n",cell);
        }

        //
        // No suitable cell was found on that list.
        // Clear the bit in the summary and try the
        // next biggest list.
        //
//        DbgPrint("No suitable cell, Index %d, Summary %08lx -> ",Index, Summary);
        ASSERT(Summary & (1 << Index));
        Summary = Summary & ~(1 << Index);
//        DbgPrint("%08lx\n",Summary);
    }

    if (Summary == 0) {
        //
        // No suitable cells were found on any free list.
        //
        // Either there is no large enough cell, or we
        // have no free cells left at all.  In either case, allocate a
        // new bin, with a new free cell certain to be large enough in
        // it, and use that cell.
        //

        //
        // Attempt to create a new bin
        //
        if ((Bin = HvpAddBin(Hive, NewSize, Type)) != NULL) {

            //
            // It worked.  Use single large cell in Bin.
            //
            DHvCheckBin(Hive,Bin);
            cell = (Bin->FileOffset) + sizeof(HBIN) + (Type*HCELL_TYPE_MASK);
            pcell = HvpGetHCell(Hive, cell);
        } else {
            return HCELL_NIL;
        }
    }

UseIt:

    //
    // cell refers to a free cell we have pulled from its list
    // if it is too big, give the residue back
    // ("too big" means there is at least one HCELL of extra space)
    // always mark it allocated
    // return it as our function value
    //

    ASSERT(pcell->Size > 0);
    if (USE_OLD_CELL(Hive)) {
        MinFreeSize = FIELD_OFFSET(HCELL, u.OldCell.u.Next) + sizeof(HCELL_INDEX);
    } else {
        MinFreeSize = FIELD_OFFSET(HCELL, u.NewCell.u.Next) + sizeof(HCELL_INDEX);
    }
    if ((NewSize + MinFreeSize) < (ULONG)pcell->Size) {

        //
        // Crack the cell, use part we need, put rest on
        // free list.
        //
        Me = HvpGetCellMap(Hive, cell);
        VALIDATE_CELL_MAP(__LINE__,Me,Hive,cell);
        Bin = (PHBIN)((Me->BinAddress) & HMAP_BASE);
        offset = (ULONG)((ULONG_PTR)pcell - (ULONG_PTR)Bin);

        ptcell = (PHCELL)((PUCHAR)pcell + NewSize);
        if (USE_OLD_CELL(Hive)) {
            ptcell->u.OldCell.Last = offset;
        }
        ptcell->Size = pcell->Size - NewSize;

        if ((offset + pcell->Size) < Bin->Size) {
            next = (PHCELL)((PUCHAR)pcell + pcell->Size);
            if (USE_OLD_CELL(Hive)) {
                next->u.OldCell.Last = offset + NewSize;
            }
        }

        pcell->Size = NewSize;
        tcell = (HCELL_INDEX)((ULONG)cell + NewSize);

        HvpEnlistFreeCell(Hive, tcell, ptcell->Size, Type, TRUE,NULL);
    }

    //
    // return the cell we found.
    //
#if DBG
    if (USE_OLD_CELL(Hive)) {
        RtlFillMemory(
            &(pcell->u.OldCell.u.UserData),
            (pcell->Size - FIELD_OFFSET(HCELL, u.OldCell.u.UserData)),
            HCELL_ALLOCATE_FILL
            );
    } else {
        RtlFillMemory(
            &(pcell->u.NewCell.u.UserData),
            (pcell->Size - FIELD_OFFSET(HCELL, u.NewCell.u.UserData)),
            HCELL_ALLOCATE_FILL
            );
    }
#endif
    pcell->Size *= -1;

    return cell;
}




VOID
HvFreeCell(
    PHHIVE      Hive,
    HCELL_INDEX Cell
    )
/*++

Routine Description:


    Frees the storage for a cell.

    NOTE:   CALLER is expected to mark relevent data dirty, so as to
            allow this call to always succeed.

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    Cell - HCELL_INDEX of Cell to free.

Return Value:

    FALSE - failed, presumably for want of log space.

    TRUE - it worked

--*/
{
    PHBIN           Bin;
    PHCELL          tmp;
    HCELL_INDEX     newfreecell;
    PHCELL          freebase;
    ULONG           savesize;
    PHCELL          neighbor;
    ULONG           Type;
    PHMAP_ENTRY     Me;


    CMLOG(CML_MINOR, CMS_HIVE) {
        KdPrint(("HvFreeCell:\n"));
        KdPrint(("\tHive=%08lx Cell=%08lx\n",Hive,Cell));
    }
    ASSERT(Hive->ReadOnly == FALSE);
    ASSERT_CM_LOCK_OWNED_EXCLUSIVE();

    //
    // Get sizes and addresses
    //
    Me = HvpGetCellMap(Hive, Cell);
    VALIDATE_CELL_MAP(__LINE__,Me,Hive,Cell);
    Type = HvGetCellType(Cell);

    Bin = (PHBIN)((Me->BinAddress) & HMAP_BASE);
    DHvCheckBin(Hive,Bin);

    freebase = HvpGetHCell(Hive, Cell);

    //
    // go do actual frees, cannot fail from this point on
    //
    ASSERT(freebase->Size < 0);
    freebase->Size *= -1;

    savesize = freebase->Size;

    //
    // Look for free neighbors and coalesce them.  We will never travel
    // around this loop more than twice.
    //
    while (
        HvpIsFreeNeighbor(
            Hive,
            Bin,
            freebase,
            &neighbor,
            Type
            ) == TRUE
        )
    {

        if (neighbor > freebase) {

            //
            // Neighboring free cell is immediately above us in memory.
            //
            if (USE_OLD_CELL(Hive)) {
                tmp = (PHCELL)((PUCHAR)neighbor + neighbor->Size);
                if ( ((ULONG)((ULONG_PTR)tmp - (ULONG_PTR)Bin)) < Bin->Size) {
                        tmp->u.OldCell.Last = (ULONG)((ULONG_PTR)freebase - (ULONG_PTR)Bin);
                }
            }
            freebase->Size += neighbor->Size;

        } else {

            //
            // Neighboring free cell is immediately below us in memory.
            //

            if (USE_OLD_CELL(Hive)) {
                tmp = (PHCELL)((PUCHAR)freebase + freebase->Size);
                if ( ((ULONG)((ULONG_PTR)tmp - (ULONG_PTR)Bin)) < Bin->Size ) {
                    tmp->u.OldCell.Last = (ULONG)((ULONG_PTR)neighbor - (ULONG_PTR)Bin);
                }
            }
            neighbor->Size += freebase->Size;
            freebase = neighbor;
        }
    }

    //
    // freebase now points to the biggest free cell we could make, none
    // of which is on the free list.  So put it on the list.
    //
    newfreecell = (Bin->FileOffset) +
               ((ULONG)((ULONG_PTR)freebase - (ULONG_PTR)Bin)) +
               (Type*HCELL_TYPE_MASK);

    ASSERT(HvpGetHCell(Hive, newfreecell) == freebase);

#if DBG
    if (USE_OLD_CELL(Hive)) {
        RtlFillMemory(
            &(freebase->u.OldCell.u.UserData),
            (freebase->Size - FIELD_OFFSET(HCELL, u.OldCell.u.UserData)),
            HCELL_FREE_FILL
            );
    } else {
        RtlFillMemory(
            &(freebase->u.NewCell.u.UserData),
            (freebase->Size - FIELD_OFFSET(HCELL, u.NewCell.u.UserData)),
            HCELL_FREE_FILL
            );
    }
#endif

    HvpEnlistFreeCell(Hive, newfreecell, freebase->Size, Type, TRUE,NULL);

    return;
}


BOOLEAN
HvpIsFreeNeighbor(
    PHHIVE  Hive,
    PHBIN   Bin,
    PHCELL  FreeCell,
    PHCELL  *FreeNeighbor,
    HSTORAGE_TYPE   Type
    )
/*++

Routine Description:

    Reports on whether FreeCell has at least one free neighbor and
    if so where.  Free neighbor will be cut out of the free list.

Arguments:

    Hive - hive we're working on

    Bin - pointer to the storage bin

    FreeCell - supplies a pointer to a cell that has been freed, or
                the result of a coalesce.

    FreeNeighbor - supplies a pointer to a variable to receive the address
                    of a free neigbhor of FreeCell, if such exists

    Type - storage type of the cell

Return Value:

    TRUE if a free neighbor was found, else false.


--*/
{
    PHCELL  ptcell;

    CMLOG(CML_MINOR, CMS_HIVE) {
        KdPrint(("HvpIsFreeNeighbor:\n\tBin=%08lx",Bin));
        KdPrint(("FreeCell=%08lx\n", FreeCell));
    }
    ASSERT(Hive->ReadOnly == FALSE);

    //
    // Neighbor above us?
    //
    *FreeNeighbor = NULL;
    ptcell = (PHCELL)((PUCHAR)FreeCell + FreeCell->Size);
    ASSERT( ((ULONG)((ULONG_PTR)ptcell - (ULONG_PTR)Bin)) <= Bin->Size);
    if (((ULONG)((ULONG_PTR)ptcell - (ULONG_PTR)Bin)) < Bin->Size) {
        if (ptcell->Size > 0) {
            *FreeNeighbor = ptcell;
            goto FoundNeighbor;
        }
    }

    //
    // Neighbor below us?
    //
    if (USE_OLD_CELL(Hive)) {
        if (FreeCell->u.OldCell.Last != HBIN_NIL) {
            ptcell = (PHCELL)((PUCHAR)Bin + FreeCell->u.OldCell.Last);
            if (ptcell->Size > 0) {
                *FreeNeighbor = ptcell;
                goto FoundNeighbor;
            }
        }
    } else {
        ptcell = (PHCELL)(Bin+1);
        while (ptcell < FreeCell) {

            //
            // scan through the cells from the start of the bin looking for neighbor.
            //
            if (ptcell->Size > 0) {

                if ((PHCELL)((PUCHAR)ptcell + ptcell->Size) == FreeCell) {
                    *FreeNeighbor = ptcell;
                    //
                    // Try and mark it dirty, since we will be changing
                    // the size field.  If this fails, ignore
                    // the free neighbor, we will not fail the free
                    // just because we couldn't mark the cell dirty
                    // so it could be coalesced.
                    //
                    // Note we only bother doing this for new hives,
                    // for old format hives we always mark the whole
                    // bin dirty.
                    //
                    if ((Type == Volatile) ||
                        (HvMarkCellDirty(Hive, (ULONG)((ULONG_PTR)ptcell-(ULONG_PTR)Bin) + Bin->FileOffset))) {
                        goto FoundNeighbor;
                    } else {
                        return(FALSE);
                    }

                } else {
                    ptcell = (PHCELL)((PUCHAR)ptcell + ptcell->Size);
                }
            } else {
                ptcell = (PHCELL)((PUCHAR)ptcell - ptcell->Size);
            }
        }
    }

    return(FALSE);

FoundNeighbor:

    HvpDelistFreeCell(Hive, *FreeNeighbor, Type, NULL);
    return TRUE;
}


VOID
HvpEnlistFreeCell(
    PHHIVE  Hive,
    HCELL_INDEX Cell,
    ULONG      Size,
    HSTORAGE_TYPE   Type,
    BOOLEAN CoalesceForward,
    PHCELL_INDEX TailDisplay OPTIONAL
    )
/*++

Routine Description:

    Puts the newly freed cell on the appropriate list.

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    Cell - supplies index of cell to enlist

    Size - size of cell

    Type - indicates whether Stable or Volatile storage is desired.

    CoalesceForward - indicates whether we can coalesce forward or not.
        For the case where we have not finished scanning the hive and
        enlisting free cells, we do not want to coalesce forward.

Return Value:

    NONE.

--*/
{
    PHCELL_INDEX Last;
    PHCELL_INDEX First;
    PHMAP_ENTRY Map;
    PHCELL pcell;
    PHCELL pcellLast;
    PHCELL FirstCell;
    ULONG   Index;
    PHBIN   Bin;
    HCELL_INDEX FreeCell;
    PFREE_HBIN FreeBin;
    PHBIN   FirstBin;
    PHBIN   LastBin;

    HvpComputeIndex(Index, Size);

    
    First = &(Hive->Storage[Type].FreeDisplay[Index]);
    pcell = HvpGetHCell(Hive, Cell);
    ASSERT(pcell->Size > 0);
    ASSERT(Size == (ULONG)pcell->Size);

    if (ARGUMENT_PRESENT(TailDisplay)) {
        //
        // This should only happen once, at boot time; Then, we want to sort the list ascedending
        //
        Last = &TailDisplay[Index];
        if( *Last != HCELL_NIL ) {
        //
        // there is a last cell; insert current cell right after it.
        // and set the next cell pointer for the current cell to NIL
        //
            pcellLast = HvpGetHCell(Hive,*Last);
            ASSERT(pcellLast->Size > 0);

            if (USE_OLD_CELL(Hive)) {
                pcellLast->u.OldCell.u.Next = Cell;
                pcell->u.OldCell.u.Next = HCELL_NIL;
            } else {
                pcellLast->u.NewCell.u.Next = Cell;
                pcell->u.NewCell.u.Next = HCELL_NIL;
            }
        } else {
        //
        // No Last cell; Insert it at the begining and set the last cell pointing to it as well
        // Sanity check: First cell should be also NIL
        //
            ASSERT( *First == HCELL_NIL );

            if (USE_OLD_CELL(Hive)) {
                pcell->u.OldCell.u.Next = *First;
            } else {
                pcell->u.NewCell.u.Next = *First;
            }
            *First = Cell;
        }
        //
        // The current cell becomes the last cell
        //
        *Last = Cell;
    } else {
    //
    // Normal case, insert the cell at the begining of the list (speed reasons).
    //
        if (USE_OLD_CELL(Hive)) {
            pcell->u.OldCell.u.Next = *First;
        } else {
            pcell->u.NewCell.u.Next = *First;
        }
        *First = Cell;
    }


    //
    // Check to see if this is the first cell in the bin and if the entire
    // bin consists just of this cell.
    //
    Map = HvpGetCellMap(Hive, Cell);
    VALIDATE_CELL_MAP(__LINE__,Map,Hive,Cell);
    Bin = (PHBIN)(Map->BinAddress & ~HMAP_NEWALLOC);
    if ((pcell == (PHCELL)(Bin + 1)) &&
        (Size == Bin->Size-sizeof(HBIN))) {

        //
        // We have a bin that is entirely free.  But we cannot do anything with it
        // unless the memalloc that contains the bin is entirely free.  Walk the
        // bins backwards until we find the first one in the alloc, then walk forwards
        // until we find the last one.  If any of the other bins in the memalloc
        // are not free, bail out.
        //
        FirstBin = Bin;
        while (FirstBin->MemAlloc == 0) {
            Map=HvpGetCellMap(Hive,(FirstBin->FileOffset - HBLOCK_SIZE) +
                                   (Type * HCELL_TYPE_MASK));
            VALIDATE_CELL_MAP(__LINE__,Map,Hive,(FirstBin->FileOffset - HBLOCK_SIZE) +(Type * HCELL_TYPE_MASK));
            FirstBin = (PHBIN)(Map->BinAddress & HMAP_BASE);
            FirstCell = (PHCELL)(FirstBin+1);
            if ((ULONG)(FirstCell->Size) != FirstBin->Size-sizeof(HBIN)) {
                //
                // The first cell in the bin is either allocated, or not the only
                // cell in the HBIN.  We cannot free any HBINs.
                //
                goto Done;
            }
        }

        //
        // We can never discard the first bin of a hive as that always gets marked dirty
        // and written out.
        //
        if (FirstBin->FileOffset == 0) {
            goto Done;
        }

        LastBin = Bin;
        while (LastBin->FileOffset+LastBin->Size < FirstBin->FileOffset+FirstBin->MemAlloc) {
            if (!CoalesceForward) {
                //
                // We are at the end of what's been built up. Just return and this
                // will get freed up when the next HBIN is added.
                //
                goto Done;
            }
            Map = HvpGetCellMap(Hive, (LastBin->FileOffset+LastBin->Size) +
                                      (Type * HCELL_TYPE_MASK));
            VALIDATE_CELL_MAP(__LINE__,Map,Hive,(LastBin->FileOffset+LastBin->Size) + (Type * HCELL_TYPE_MASK));

            ASSERT(Map->BinAddress != 0);

            LastBin = (PHBIN)(Map->BinAddress & HMAP_BASE);
            FirstCell = (PHCELL)(LastBin + 1);
            if ((ULONG)(FirstCell->Size) != LastBin->Size-sizeof(HBIN)) {
                //
                // The first cell in the bin is either allocated, or not the only
                // cell in the HBIN.  We cannot free any HBINs.
                //
                goto Done;
            }
        }

        //
        // All the bins in this alloc are freed.  Coalesce all the bins into
        // one alloc-sized bin, then either discard the bin or mark it as
        // discardable.
        //
        if (FirstBin->Size != FirstBin->MemAlloc) {
            //
            // Mark the first HBLOCK of the first HBIN dirty, since
            // we will need to update the on disk field for the bin size
            //
            if (!HvMarkDirty(Hive,
                             FirstBin->FileOffset + (Type * HCELL_TYPE_MASK),
                             sizeof(HBIN) + sizeof(HCELL))) {
                goto Done;
            }

        }


        FreeBin = (Hive->Allocate)(sizeof(FREE_HBIN), FALSE);
        if (FreeBin == NULL) {
            goto Done;
        }

        //
        // Walk through the bins and delist each free cell
        //
        Bin = FirstBin;
        do {
            FirstCell = (PHCELL)(Bin+1);
            HvpDelistFreeCell(Hive, FirstCell, Type, TailDisplay);
            if (Bin==LastBin) {
                break;
            }
            Map = HvpGetCellMap(Hive, (Bin->FileOffset+Bin->Size)+
                                      (Type * HCELL_TYPE_MASK));
            VALIDATE_CELL_MAP(__LINE__,Map,Hive,(Bin->FileOffset+Bin->Size)+(Type * HCELL_TYPE_MASK));
            Bin = (PHBIN)(Map->BinAddress & HMAP_BASE);

        } while ( TRUE );

        //
        // Coalesce them all into one bin.
        //
        FirstBin->Size = FirstBin->MemAlloc;

        FreeBin->Size = FirstBin->Size;
        FreeBin->FileOffset = FirstBin->FileOffset;
        FirstCell = (PHCELL)(FirstBin+1);
        FirstCell->Size = FirstBin->Size - sizeof(HBIN);
        if (USE_OLD_CELL(Hive)) {
            FirstCell->u.OldCell.Last = (ULONG)HBIN_NIL;
        }

        InsertHeadList(&Hive->Storage[Type].FreeBins, &FreeBin->ListEntry);
        ASSERT_LISTENTRY(&FreeBin->ListEntry);
        ASSERT_LISTENTRY(FreeBin->ListEntry.Flink);

        FreeCell = FirstBin->FileOffset+(Type*HCELL_TYPE_MASK);
        FreeBin->Flags = FREE_HBIN_DISCARDABLE;
        while (FreeCell-FirstBin->FileOffset < FirstBin->Size) {
            Map = HvpGetCellMap(Hive, FreeCell);
            VALIDATE_CELL_MAP(__LINE__,Map,Hive,FreeCell);
            if (Map->BinAddress & HMAP_NEWALLOC) {
                Map->BinAddress = (ULONG_PTR)FirstBin | HMAP_DISCARDABLE | HMAP_NEWALLOC;
            } else {
                Map->BinAddress = (ULONG_PTR)FirstBin | HMAP_DISCARDABLE;
            }
            Map->BlockAddress = (ULONG_PTR)FreeBin;
            FreeCell += HBLOCK_SIZE;
        }
    }


Done:
    Hive->Storage[Type].FreeSummary |= (1 << Index);
    return;
}



VOID
HvpDelistFreeCell(
    PHHIVE  Hive,
    PHCELL  Pcell,
    HSTORAGE_TYPE Type,
    PHCELL_INDEX TailDisplay OPTIONAL
    )
/*++

Routine Description:

    Removes a free cell from its list, and clears the Summary
    bit for it if need be.

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    Pcell - supplies a pointer to the HCELL structure of interest

    Type - Stable vs. Volatile

Return Value:

    NONE.

--*/
{
    PHCELL_INDEX List;
    HCELL_INDEX Prev = HCELL_NIL;
    ULONG       Index;

    HvpComputeIndex(Index, Pcell->Size);
    List = &(Hive->Storage[Type].FreeDisplay[Index]);

    //
    // Find previous cell on list
    //
    ASSERT(*List != HCELL_NIL);
    while (HvpGetHCell(Hive,*List) != Pcell) {
        Prev = *List;
        List = (PHCELL_INDEX)HvGetCell(Hive,*List);
        ASSERT(*List != HCELL_NIL);
    }

    if (ARGUMENT_PRESENT(TailDisplay)) {
        //
        // This should only happen once, at boot time; Then, we want to sort the list ascedending
        //
        if( *List == TailDisplay[Index] ) {
            //
            // this cell is also the last cell on list; so it should be reset
            //

#if DBG
            //
            // consistency checks
            //
            if (USE_OLD_CELL(Hive)) {
                ASSERT(Pcell->u.OldCell.u.Next == HCELL_NIL);
            } else {
                ASSERT(Pcell->u.NewCell.u.Next == HCELL_NIL);
            }
#endif

            TailDisplay[Index] = Prev;
        }
    }

    //
    // Remove Pcell from list.
    //
    if (USE_OLD_CELL(Hive)) {
        *List = Pcell->u.OldCell.u.Next;
    } else {
        *List = Pcell->u.NewCell.u.Next;
    }

    if (Hive->Storage[Type].FreeDisplay[Index] == HCELL_NIL) {
        //
        // consistency check
        //
        ASSERT(Prev == HCELL_NIL);

        Hive->Storage[Type].FreeSummary &= ~(1 << Index);
    }

    return;
}


HCELL_INDEX
HvReallocateCell(
    PHHIVE  Hive,
    HCELL_INDEX Cell,
    ULONG    NewSize
    )
/*++

Routine Description:

    Grows or shrinks a cell.

    NOTE:

        MUST NOT FAIL if the cell is being made smaller.  Can be
        a noop, but must work.

    WARNING:

        If the cell is grown, it will get a NEW and DIFFERENT HCELL_INDEX!!!

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    Cell - supplies index of cell to grow or shrink

    NewSize - desired size of cell  (this is an absolute size, not an
            increment or decrement.)

Return Value:

    New HCELL_INDEX for cell, or HCELL_NIL if failure.

    If return is HCELL_NIL, either old cell did not exist, or it did exist
    and we could not make a new one.  In either case, nothing has changed.

    If return is NOT HCELL_NIL, then it is the HCELL_INDEX for the Cell,
    which very probably moved.

--*/
{
    PUCHAR      oldaddress;
    LONG        oldsize;
    ULONG       oldalloc;
    HCELL_INDEX NewCell;            // return value
    PUCHAR      newaddress;
    ULONG       Type;

    CMLOG(CML_MAJOR, CMS_HIVE) {
        KdPrint(("HvReallocateCell:\n"));
        KdPrint(("\tHive=%08lx  Cell=%08lx  NewSize=%08lx\n",Hive,Cell,NewSize));
    }
    ASSERT(Hive->Signature == HHIVE_SIGNATURE);
    ASSERT(Hive->ReadOnly == FALSE);
    ASSERT_CM_LOCK_OWNED_EXCLUSIVE();

    //
    // Make room for overhead fields and round up to HCELL_PAD boundary
    //
    if (USE_OLD_CELL(Hive)) {
        NewSize += FIELD_OFFSET(HCELL, u.OldCell.u.UserData);
    } else {
        NewSize += FIELD_OFFSET(HCELL, u.NewCell.u.UserData);
    }
    NewSize = ROUND_UP(NewSize, HCELL_PAD(Hive));

    // 
    // Adjust the size (an easy fix for granularity)
    //
    HvpAdjustCellSize(NewSize);

    //
    // reject impossible/unreasonable values
    //
    if (NewSize > HSANE_CELL_MAX) {
        CMLOG(CML_FLOW, CMS_HIVE) {
            KdPrint(("\tNewSize=%08lx\n", NewSize));
        }
        return HCELL_NIL;
    }

    //
    // Get sizes and addresses
    //
    oldaddress = (PUCHAR)HvGetCell(Hive, Cell);
    oldsize = HvGetCellSize(Hive, oldaddress);
    ASSERT(oldsize > 0);
    if (USE_OLD_CELL(Hive)) {
        oldalloc = (ULONG)(oldsize + FIELD_OFFSET(HCELL, u.OldCell.u.UserData));
    } else {
        oldalloc = (ULONG)(oldsize + FIELD_OFFSET(HCELL, u.NewCell.u.UserData));
    }
    Type = HvGetCellType(Cell);

    DHvCheckHive(Hive);

    if (NewSize == oldalloc) {

        //
        // This is a noop, return the same cell
        //
        NewCell = Cell;

    } else if (NewSize < oldalloc) {

        //
        // This is a shrink.
        //
        // PERFNOTE - IMPLEMENT THIS.  Do nothing for now.
        //
        NewCell = Cell;

    } else {

        //
        // This is a grow.
        //

        //
        // PERFNOTE - Someday we want to detect that there is a free neighbor
        //          above us and grow into that neighbor if possible.
        //          For now, always do the allocate, copy, free gig.
        //

        //
        // Allocate a new block of memory to hold the cell
        //

        if ((NewCell = HvpDoAllocateCell(Hive, NewSize, Type)) == HCELL_NIL) {
            return HCELL_NIL;
        }
        ASSERT(HvIsCellAllocated(Hive, NewCell));
        newaddress = (PUCHAR)HvGetCell(Hive, NewCell);

        //
        // oldaddress points to the old data block for the cell,
        // newaddress points to the new data block, copy the data
        //
        RtlMoveMemory(newaddress, oldaddress, oldsize);

        //
        // Free the old block of memory
        //
        HvFreeCell(Hive, Cell);
    }

    DHvCheckHive(Hive);
    return NewCell;
}



//
// Procedure used for checking only  (used in production systems, so
//  must always be here.)
//
BOOLEAN
HvIsCellAllocated(
    PHHIVE Hive,
    HCELL_INDEX Cell
    )
/*++

Routine Description:

    Report whether the requested cell is allocated or not.

Arguments:

    Hive - containing Hive.

    Cell - cel of interest

Return Value:

    TRUE if allocated, FALSE if not.

--*/
{
    ULONG   Type;
    PHCELL  Address;
    PHCELL  Below;
    PHMAP_ENTRY Me;
    PHBIN   Bin;
    ULONG   Offset;
    LONG    Size;


    ASSERT(Hive->Signature == HHIVE_SIGNATURE);

    if (Hive->Flat == TRUE) {
        return TRUE;
    }

    Type = HvGetCellType(Cell);

    if ( ((Cell & ~HCELL_TYPE_MASK) > Hive->Storage[Type].Length) || // off end
         (Cell % HCELL_PAD(Hive) != 0)                    // wrong alignment
       )
    {
        return FALSE;
    }

    Address = HvpGetHCell(Hive, Cell);
    Me = HvpGetCellMap(Hive, Cell);
    if (Me == NULL) {
        return FALSE;
    }
    Bin = (PHBIN)((ULONG_PTR)(Me->BinAddress) & HMAP_BASE);
    Offset = (ULONG)((ULONG_PTR)Address - (ULONG_PTR)Bin);
    Size = Address->Size * -1;

    if ( (Address->Size > 0) ||                     // not allocated
         ((Offset + (ULONG)Size) > Bin->Size) ||    // runs off bin, or too big
         (Offset < sizeof(HBIN))                    // pts into bin header
       )
    {
        return FALSE;
    }

    if (USE_OLD_CELL(Hive)) {
        if (Address->u.OldCell.Last != HBIN_NIL) {

            if (Address->u.OldCell.Last > Bin->Size) {            // bogus back pointer
                return FALSE;
            }

            Below = (PHCELL)((PUCHAR)Bin + Address->u.OldCell.Last);
            Size = (Below->Size < 0) ?
                        Below->Size * -1 :
                        Below->Size;

            if ( ((ULONG_PTR)Below + Size) != (ULONG_PTR)Address ) {    // no pt back
                return FALSE;
            }
        }
    }


    return TRUE;
}

VOID
HvpDelistBinFreeCells(
    PHHIVE  Hive,
    PHBIN   Bin,
    HSTORAGE_TYPE Type,
    PHCELL_INDEX TailDisplay OPTIONAL
    )
/*++

Routine Description:

    If we are here, the hive needs recovery.

    Walks through the entire bin and removes its free cells from the list.
    If the bin is marked as free, it just delist it from the freebins list.

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    Bin - supplies a pointer to the HBIN of interest

    Type - Stable vs. Volatile

Return Value:

    NONE.

--*/
{
    PHCELL  p;
    ULONG   size;
    HCELL_INDEX Cell;
    PHMAP_ENTRY Map;
    PFREE_HBIN FreeBin;
    PLIST_ENTRY Entry;

    Cell = Bin->FileOffset+(Type*HCELL_TYPE_MASK);
    Map = HvpGetCellMap(Hive, Cell);
    VALIDATE_CELL_MAP(__LINE__,Map,Hive,Cell);

    //
    // When loading, bins are always in separate chunks (each bin in it's owns chunk)
    //
    ASSERT( Map->BinAddress == ((ULONG_PTR)Bin | HMAP_NEWALLOC) );
    
    if( Map->BinAddress & HMAP_DISCARDABLE ) {
        //
        // The bin has been added to the freebins list
        // we have to take it out. No free cell from this bin is on the 
        // freecells list, so we don't have to delist them.
        //

        Entry = Hive->Storage[Type].FreeBins.Flink;
        while (Entry != &Hive->Storage[Type].FreeBins) {
            FreeBin = CONTAINING_RECORD(Entry,
                                        FREE_HBIN,
                                        ListEntry);

            
            if( FreeBin->FileOffset == Bin->FileOffset ){
                //
                // that's the bin we're looking for
                //
                
                // sanity checks
                ASSERT( FreeBin->Size == Bin->Size );
                ASSERT_LISTENTRY(&FreeBin->ListEntry);
                
                RemoveEntryList(&FreeBin->ListEntry);
                (Hive->Free)(FreeBin, sizeof(FREE_HBIN));
                return;
            }

            // advance to the new bin
            Entry = Entry->Flink;
        }

        // we shouldn't get here
        CM_BUGCHECK(REGISTRY_ERROR,14,(ULONG)Cell,(ULONG_PTR)Map,0);
        return;
    }

    //
    // Scan all the cells in the bin, total free and allocated, check
    // for impossible pointers.
    //
    p = (PHCELL)((PUCHAR)Bin + sizeof(HBIN));

    while (p < (PHCELL)((PUCHAR)Bin + Bin->Size)) {

        //
        // if free cell, remove it from the list of the hive
        //
        if (p->Size >= 0) {

            size = (ULONG)p->Size;

            //
            // Enlist this free cell, but do not coalesce with the next free cell
            // as we haven't gotten that far yet.
            //
            HvpDelistFreeCell(Hive, p, Type, TailDisplay);

        } else {

            size = (ULONG)(p->Size * -1);

        }

        ASSERT(size >= 0);
        p = (PHCELL)((PUCHAR)p + size);
    }

    return;
}
