/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    hivebin.c

Abstract:

    This module implements HvpAddBin - used to grow a hive.

Author:

    Bryan M. Willman (bryanwi) 27-Mar-92

Environment:


Revision History:

--*/

#include    "cmp.h"

//
// Private function prototypes
//
BOOLEAN
HvpCoalesceDiscardedBins(
    IN PHHIVE Hive,
    IN ULONG NeededSize,
    IN HSTORAGE_TYPE Type
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,HvpAddBin)
#pragma alloc_text(PAGE,HvpFreeMap)
#pragma alloc_text(PAGE,HvpAllocateMap)
#pragma alloc_text(PAGE,HvpCoalesceDiscardedBins)
#endif



PHBIN
HvpAddBin(
    IN PHHIVE  Hive,
    IN ULONG   NewSize,
    IN HSTORAGE_TYPE   Type
    )
/*++

Routine Description:

    Grows either the Stable or Volatile storage of a hive by adding
    a new bin.  Bin will be allocated space in Stable store (e.g. file)
    if Type == Stable.  Memory image will be allocated and initialized.
    Map will be grown and filled in to describe the new bin.

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    NewSize - size of the object caller wishes to put in the hive.  New
                bin will be at least large enough to hold this.

    Type - Stable or Volatile

Return Value:

    Pointer to the new BIN if we succeeded, NULL if we failed.

--*/
{
    BOOLEAN         UseForIo;
    PHBIN           NewBin;
    ULONG           OldLength;
    ULONG           NewLength;
    ULONG           OldMap;
    ULONG           NewMap;
    ULONG           OldTable;
    ULONG           NewTable;
    PHMAP_DIRECTORY Dir;
    PHMAP_TABLE     newt;
    PHMAP_ENTRY     Me;
    PHCELL          t;
    ULONG           i;
    ULONG           j;
    PULONG          NewVector;
    PLIST_ENTRY     Entry;
    PFREE_HBIN      FreeBin;
    ULONG           TotalDiscardedSize;


    CMLOG(CML_FLOW, CMS_HIVE) {
        KdPrint(("HvpAddBin:\n"));
        KdPrint(("\tHive=%08lx NewSize=%08lx\n",Hive,NewSize));
    }

    //
    //  Round size up to account for bin overhead.  Caller should
    //  have accounted for cell overhead.
    //
    NewSize += sizeof(HBIN);
    if ((NewSize < HCELL_BIG_ROUND) &&
        ((NewSize % HBLOCK_SIZE) > HBIN_THRESHOLD)) {
        NewSize += HBLOCK_SIZE;
    }

    //
    // Try not to create HBINs smaller than the page size of the machine
    //  (it is not illegal to have bins smaller than the page size, but it
    //  is less efficient)
    //
    NewSize = ROUND_UP(NewSize, ((HBLOCK_SIZE >= PAGE_SIZE) ? HBLOCK_SIZE : PAGE_SIZE));

    //
    // see if there's a discarded HBIN of the right size
    //
    TotalDiscardedSize = 0;

Retry:

    Entry = Hive->Storage[Type].FreeBins.Flink;
    while (Entry != &Hive->Storage[Type].FreeBins) {
        FreeBin = CONTAINING_RECORD(Entry,
                                    FREE_HBIN,
                                    ListEntry);
        TotalDiscardedSize += FreeBin->Size;
        if (FreeBin->Size >= NewSize) {

            if (!HvMarkDirty(Hive,
                             FreeBin->FileOffset + (Type * HCELL_TYPE_MASK),
                             FreeBin->Size)) {
                goto ErrorExit1;
            }
            NewSize = FreeBin->Size;
            ASSERT_LISTENTRY(&FreeBin->ListEntry);
            RemoveEntryList(&FreeBin->ListEntry);
            if (FreeBin->Flags & FREE_HBIN_DISCARDABLE) {
                //
                // HBIN is still in memory, don't need any more allocs, just
                // fill in the block addresses.
                //
                for (i=0;i<NewSize;i+=HBLOCK_SIZE) {
                    Me = HvpGetCellMap(Hive, FreeBin->FileOffset+i+(Type*HCELL_TYPE_MASK));
                    VALIDATE_CELL_MAP(__LINE__,Me,Hive,FreeBin->FileOffset+i+(Type*HCELL_TYPE_MASK));
                    Me->BlockAddress = (Me->BinAddress & HMAP_BASE)+i;
                    Me->BinAddress &= ~HMAP_DISCARDABLE;
                }
                (Hive->Free)(FreeBin, sizeof(FREE_HBIN));
                return((PHBIN)(Me->BinAddress & HMAP_BASE));
            }
            break;
        }
        Entry = Entry->Flink;
    }

    if ((Entry == &Hive->Storage[Type].FreeBins) &&
        (TotalDiscardedSize >= NewSize)) {
        //
        // No sufficiently large discarded bin was found,
        // but the total discarded space is large enough.
        // Attempt to coalesce adjacent discarded bins into
        // a larger bin and retry.
        //
        if (HvpCoalesceDiscardedBins(Hive, NewSize, Type)) {
            goto Retry;
        }
    }

    //
    //  Attempt to allocate the bin.
    //
    UseForIo = (BOOLEAN)((Type == Stable) ? TRUE : FALSE);
    if (Entry != &Hive->Storage[Type].FreeBins) {
        //
        // Note we use ExAllocatePool directly here to avoid
        // charging quota for this bin again. When a bin
        // is discarded, its quota is not returned. This prevents
        // sparse hives from requiring more quota after
        // a reboot than on a running system.
        //
        NewBin = ExAllocatePoolWithTag((UseForIo) ? PagedPoolCacheAligned : PagedPool,
                                       NewSize,
                                       CM_POOL_TAG);
        if (NewBin == NULL) {
            InsertHeadList(&Hive->Storage[Type].FreeBins, Entry);
            HvMarkClean(Hive, FreeBin->FileOffset, FreeBin->Size);
            goto ErrorExit1;
        }
    } else {
        NewBin = (Hive->Allocate)(NewSize, UseForIo);
        if (NewBin == NULL) {
            goto ErrorExit1;
        }
    }

    //
    // Init the bin
    //
    NewBin->Signature = HBIN_SIGNATURE;
    NewBin->Size = NewSize;
    NewBin->MemAlloc = NewSize;

    t = (PHCELL)((PUCHAR)NewBin + sizeof(HBIN));
    t->Size = NewSize - sizeof(HBIN);
    if (USE_OLD_CELL(Hive)) {
        t->u.OldCell.Last = (ULONG)HBIN_NIL;
    }

    if (Entry != &Hive->Storage[Type].FreeBins) {
        //
        // found a discarded HBIN we can use, just fill in the map and we
        // are done.
        //
        for (i=0;i<NewSize;i+=HBLOCK_SIZE) {
            Me = HvpGetCellMap(Hive, FreeBin->FileOffset+i+(Type*HCELL_TYPE_MASK));
            VALIDATE_CELL_MAP(__LINE__,Me,Hive,FreeBin->FileOffset+i+(Type*HCELL_TYPE_MASK));
            Me->BlockAddress = (ULONG_PTR)NewBin + i;
            Me->BinAddress = (ULONG_PTR)NewBin;
            if (i==0) {
                Me->BinAddress |= HMAP_NEWALLOC;
            }
        }

        NewBin->FileOffset = FreeBin->FileOffset;

        (Hive->Free)(FreeBin, sizeof(FREE_HBIN));

        return(NewBin);
    }


    //
    // Compute map growth needed, grow the map
    //
    OldLength = Hive->Storage[Type].Length;
    NewLength = OldLength + NewSize;


    NewBin->FileOffset = OldLength;

    ASSERT((OldLength % HBLOCK_SIZE) == 0);
    ASSERT((NewLength % HBLOCK_SIZE) == 0);

    if (OldLength == 0) {
        //
        // Need to create the first table
        //
        newt = (PVOID)((Hive->Allocate)(sizeof(HMAP_TABLE), FALSE));
        if (newt == NULL) {
            goto ErrorExit2;
        }
        RtlZeroMemory(newt, sizeof(HMAP_TABLE));
        Hive->Storage[Type].SmallDir = newt;
        Hive->Storage[Type].Map = (PHMAP_DIRECTORY)&(Hive->Storage[Type].SmallDir);
    }

    if (OldLength > 0) {
        OldMap = (OldLength-1) / HBLOCK_SIZE;
    } else {
        OldMap = 0;
    }
    NewMap = (NewLength-1) / HBLOCK_SIZE;

    OldTable = OldMap / HTABLE_SLOTS;
    NewTable = NewMap / HTABLE_SLOTS;

    if (NewTable != OldTable) {

        //
        // Need some new Tables
        //
        if (OldTable == 0) {

            //
            // We can get here even if the real directory has already been created.
            // This can happen if we create the directory then fail on something 
            // later. So we need to handle the case where a directory already exists.
            //
            if (Hive->Storage[Type].Map == (PHMAP_DIRECTORY)&Hive->Storage[Type].SmallDir) {
                ASSERT(Hive->Storage[Type].SmallDir != NULL);

                //
                // Need a real directory
                //
                Dir = (Hive->Allocate)(sizeof(HMAP_DIRECTORY), FALSE);
                if (Dir == NULL) {
                    goto ErrorExit2;
                }
                RtlZeroMemory(Dir, sizeof(HMAP_DIRECTORY));
    
                Dir->Directory[0] = Hive->Storage[Type].SmallDir;
                Hive->Storage[Type].SmallDir = NULL;
    
                Hive->Storage[Type].Map = Dir;
            } else {
                ASSERT(Hive->Storage[Type].SmallDir == NULL);
            }

        }
        Dir = Hive->Storage[Type].Map;

        //
        // Fill in directory with new tables
        //
        if (HvpAllocateMap(Hive, Dir, OldTable+1, NewTable) ==  FALSE) {
            goto ErrorExit3;
        }
    }

    //
    // If Type == Stable, and the hive is not marked WholeHiveVolatile,
    // grow the file, the log, and the DirtyVector
    //
    Hive->Storage[Type].Length = NewLength;
    if ((Type == Stable) && (!(Hive->HiveFlags & HIVE_VOLATILE))) {

        //
        // Grow the dirtyvector
        //
        NewVector = (PULONG)(Hive->Allocate)(ROUND_UP(NewMap+1,sizeof(ULONG)), TRUE);
        if (NewVector == NULL) {
            goto ErrorExit3;
        }

        RtlZeroMemory(NewVector, NewMap+1);

        if (Hive->DirtyVector.Buffer != NULL) {

            RtlCopyMemory(
                (PVOID)NewVector,
                (PVOID)Hive->DirtyVector.Buffer,
                OldMap+1
                );
            (Hive->Free)(Hive->DirtyVector.Buffer, Hive->DirtyAlloc);
        }

        RtlInitializeBitMap(
            &(Hive->DirtyVector),
            NewVector,
            NewLength / HSECTOR_SIZE
            );
        Hive->DirtyAlloc = NewMap+1;

        //
        // Grow the log
        //
        if ( ! (HvpGrowLog2(Hive, NewSize))) {
            goto ErrorExit4;
        }

        //
        // Grow the primary
        //
        if ( !  (Hive->FileSetSize)(
                    Hive,
                    HFILE_TYPE_PRIMARY,
                    NewLength+HBLOCK_SIZE
                    ) )
        {
            goto ErrorExit4;
        }

        //
        // Grow the alternate if it exists
        //
        if (Hive->Alternate == TRUE) {
            if ( ! (Hive->FileSetSize)(
                        Hive,
                        HFILE_TYPE_ALTERNATE,
                        NewLength+HBLOCK_SIZE
                        ) )
            {
                (Hive->FileSetSize)(
                    Hive,
                    HFILE_TYPE_PRIMARY,
                    OldLength+HBLOCK_SIZE
                    );
                goto ErrorExit4;
            }
        }

        //
        // Mark new bin dirty so all control structures get written at next sync
        //
        if ( ! HvMarkDirty(Hive, OldLength, NewSize)) {
            goto ErrorExit4;
        }
    }

    //
    // Fill in the map, mark new allocation.
    //
    j = 0;
    for (i = OldLength; i < NewLength; i += HBLOCK_SIZE) {
        Me = HvpGetCellMap(Hive, i + (Type*HCELL_TYPE_MASK));
        VALIDATE_CELL_MAP(__LINE__,Me,Hive,i + (Type*HCELL_TYPE_MASK));
        Me->BlockAddress = (ULONG_PTR)NewBin + j;
        Me->BinAddress = (ULONG_PTR)NewBin;

        if (j == 0) {
            //
            // First block of allocation, mark it.
            //
            Me->BinAddress |= HMAP_NEWALLOC;
        }
        j += HBLOCK_SIZE;
    }

    return NewBin;

ErrorExit4:
    RtlInitializeBitMap(&Hive->DirtyVector,
                        NewVector,
                        OldLength / HSECTOR_SIZE);
    Hive->DirtyCount = RtlNumberOfSetBits(&Hive->DirtyVector);

ErrorExit3:
    Hive->Storage[Type].Length = OldLength;
    HvpFreeMap(Hive, Dir, OldTable+1, NewTable);

ErrorExit2:
    (Hive->Free)(NewBin, NewSize);

ErrorExit1:
    return NULL;
}


BOOLEAN
HvpCoalesceDiscardedBins(
    IN PHHIVE Hive,
    IN ULONG NeededSize,
    IN HSTORAGE_TYPE Type
    )

/*++

Routine Description:

    Walks through the list of discarded bins and attempts to
    coalesce adjacent discarded bins into one larger bin in
    order to satisfy an allocation request.

Arguments:

    Hive - Supplies pointer to hive control block.

    NeededSize - Supplies size of allocation needed.

    Type - Stable or Volatile

Return Value:

    TRUE - A bin of the desired size was created.

    FALSE - No bin of the desired size could be created.

--*/

{
    PLIST_ENTRY List;
    PFREE_HBIN FreeBin;
    PFREE_HBIN PreviousFreeBin;
    PFREE_HBIN NextFreeBin;
    PHMAP_ENTRY Map;
    PHMAP_ENTRY PreviousMap;
    PHMAP_ENTRY NextMap;
    ULONG MapBlock;

    List = Hive->Storage[Type].FreeBins.Flink;

    while (List != &Hive->Storage[Type].FreeBins) {
        FreeBin = CONTAINING_RECORD(List, FREE_HBIN, ListEntry);

        if ((FreeBin->Flags & FREE_HBIN_DISCARDABLE)==0) {

            Map = HvpGetCellMap(Hive, FreeBin->FileOffset);
            VALIDATE_CELL_MAP(__LINE__,Map,Hive,FreeBin->FileOffset);

            //
            // Scan backwards, coalescing previous discarded bins
            //
            while (FreeBin->FileOffset > 0) {
                PreviousMap = HvpGetCellMap(Hive, FreeBin->FileOffset - HBLOCK_SIZE);
                VALIDATE_CELL_MAP(__LINE__,PreviousMap,Hive,FreeBin->FileOffset - HBLOCK_SIZE);
                if (PreviousMap->BinAddress & HMAP_DISCARDABLE) {
                    PreviousFreeBin = (PFREE_HBIN)PreviousMap->BlockAddress;

                    if (PreviousFreeBin->Flags & FREE_HBIN_DISCARDABLE) {
                        break;
                    }
                    RemoveEntryList(&PreviousFreeBin->ListEntry);

                    //
                    // Fill in all the old map entries with the new one.
                    //
                    for (MapBlock = 0; MapBlock < PreviousFreeBin->Size; MapBlock += HBLOCK_SIZE) {
                        PreviousMap = HvpGetCellMap(Hive, PreviousFreeBin->FileOffset + MapBlock);
                        VALIDATE_CELL_MAP(__LINE__,PreviousMap,Hive,PreviousFreeBin->FileOffset + MapBlock);
                        PreviousMap->BlockAddress = (ULONG_PTR)FreeBin;
                    }

                    FreeBin->FileOffset = PreviousFreeBin->FileOffset;
                    FreeBin->Size += PreviousFreeBin->Size;
                    (Hive->Free)(PreviousFreeBin, sizeof(FREE_HBIN));
                } else {
                    break;
                }
            }

            //
            // Scan forwards, coalescing subsequent discarded bins
            //
            while ((FreeBin->FileOffset + FreeBin->Size) < Hive->BaseBlock->Length) {
                NextMap = HvpGetCellMap(Hive, FreeBin->FileOffset + FreeBin->Size);
                VALIDATE_CELL_MAP(__LINE__,NextMap,Hive,FreeBin->FileOffset + FreeBin->Size);
                if (NextMap->BinAddress & HMAP_DISCARDABLE) {
                    NextFreeBin = (PFREE_HBIN)NextMap->BlockAddress;

                    if (NextFreeBin->Flags & FREE_HBIN_DISCARDABLE) {
                        break;
                    }

                    RemoveEntryList(&NextFreeBin->ListEntry);

                    //
                    // Fill in all the old map entries with the new one.
                    //
                    for (MapBlock = 0; MapBlock < NextFreeBin->Size; MapBlock += HBLOCK_SIZE) {
                        NextMap = HvpGetCellMap(Hive, NextFreeBin->FileOffset + MapBlock);
                        VALIDATE_CELL_MAP(__LINE__,NextMap,Hive,NextFreeBin->FileOffset + MapBlock);
                        NextMap->BlockAddress = (ULONG_PTR)FreeBin;
                    }

                    FreeBin->Size += NextFreeBin->Size;
                    (Hive->Free)(NextFreeBin, sizeof(FREE_HBIN));
                } else {
                    break;
                }
            }
            if (FreeBin->Size >= NeededSize) {
                return(TRUE);
            }
        }
        List=List->Flink;
    }
    return(FALSE);
}


