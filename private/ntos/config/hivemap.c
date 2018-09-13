/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    hivemap.c

Abstract:

    This module implements HvpBuildMap - used to build the initial map for a hive

Author:

    Bryan M. Willman (bryanwi) 28-Mar-92

Environment:


Revision History:
    Dragos C. Sambotin (dragoss) 25-Jan-99
        Implementation of bin-size chunk loading of hives.
--*/

#include    "cmp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,HvpBuildMap)
#pragma alloc_text(PAGE,HvpFreeMap)
#pragma alloc_text(PAGE,HvpAllocateMap)
#pragma alloc_text(PAGE,HvpBuildMapAndCopy)
#pragma alloc_text(PAGE,HvpEnlistFreeCells)
#pragma alloc_text(PAGE,HvpInitMap)
#pragma alloc_text(PAGE,HvpCleanMap)
#pragma alloc_text(PAGE,HvpEnlistBinInMap)
#endif

extern struct {
    PHHIVE      Hive;
    ULONG       Status;
    ULONG       Space;
    HCELL_INDEX MapPoint;
    PHBIN       BinPoint;
} HvCheckHiveDebug;

NTSTATUS
HvpBuildMapAndCopy(
    PHHIVE  Hive,
    PVOID   Image,
    PHCELL_INDEX TailDisplay OPTIONAL
    )
/*++

Routine Description:

    Creates the map for the Stable storage of the hive, and inits
    the map for the volatile storage.

    Following fields in hive must already be filled in:

         Allocate, Free

    Will initialize Storage structure of HHIVE.

    The difference between this routine and HvpBuildMapAndCopy is that
    this routine will create a "sparse" map.  As it copies the SourceImage
    to the newly allocated memory for the destination, it will avoid
    allocating space for HBINs that contain nothing but free space.  The
    HBLOCKs in these HBINs will be marked as discarded, and HvGetCell
    will allocate memory for them if necessary.

Arguments:

    Hive - Pointer to hive control structure to build map for.

    Image - pointer to flat memory image of original hive.

Return Value:

    TRUE - it worked
    FALSE - either hive is corrupt or no memory for map

--*/
{
    PHBASE_BLOCK    BaseBlock;
    ULONG           Length;
    ULONG           MapSlots;
    ULONG           Tables;
    PHMAP_TABLE     t = NULL;
    PHMAP_DIRECTORY d = NULL;
    PHBIN           Bin;
    PHBIN           NewBins;
    PHBIN           CurrentBin;
    ULONG           Offset;
    ULONG_PTR        Address;
    PHMAP_ENTRY     Me;
    NTSTATUS        Status;
    PULONG          Vector;
    ULONG           Size;


    CMLOG(CML_FLOW, CMS_HIVE) {
        KdPrint(("HvpBuildMap:\n"));
        KdPrint(("\tHive=%08lx",Hive));
    }


    //
    // Compute size of data region to be mapped
    //
    BaseBlock = Hive->BaseBlock;
    Length = BaseBlock->Length;
    if ((Length % HBLOCK_SIZE) != 0 ) {
        Status = STATUS_REGISTRY_CORRUPT;
        goto ErrorExit1;
    }
    MapSlots = Length / HBLOCK_SIZE;
    if( MapSlots > 0 ) {
        Tables = (MapSlots-1) / HTABLE_SLOTS;
    } else {
        Tables = 0;
    }

    Hive->Storage[Stable].Length = Length;

    //
    // allocate dirty vector if one is not already present (from HvpRecoverData)
    //

    if (Hive->DirtyVector.Buffer == NULL) {
        Vector = (PULONG)((Hive->Allocate)(ROUND_UP(Length/HSECTOR_SIZE/8,sizeof(ULONG)), TRUE));
        if (Vector == NULL) {
            Status = STATUS_NO_MEMORY;
            goto ErrorExit1;
        }
        RtlZeroMemory(Vector, Length / HSECTOR_SIZE / 8);
        RtlInitializeBitMap(&Hive->DirtyVector, Vector, Length / HSECTOR_SIZE);
        Hive->DirtyAlloc = (Length/HSECTOR_SIZE/8);
    }

    //
    // allocate and build structure for map
    //
    if (Tables == 0) {

        //
        // Just 1 table, no need for directory
        //
        t = (Hive->Allocate)(sizeof(HMAP_TABLE), FALSE);
        if (t == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto ErrorExit1;
        }
        RtlZeroMemory(t, sizeof(HMAP_TABLE));
        Hive->Storage[Stable].Map =
            (PHMAP_DIRECTORY)&(Hive->Storage[Stable].SmallDir);
        Hive->Storage[Stable].SmallDir = t;

    } else {

        //
        // Need directory and multiple tables
        //
        d = (PHMAP_DIRECTORY)(Hive->Allocate)(sizeof(HMAP_DIRECTORY), FALSE);
        if (d == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto ErrorExit1;
        }
        RtlZeroMemory(d, sizeof(HMAP_DIRECTORY));

        //
        // Allocate tables and fill in dir
        //
        if (HvpAllocateMap(Hive, d, 0, Tables) == FALSE) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto ErrorExit2;
        }
        Hive->Storage[Stable].Map = d;
        Hive->Storage[Stable].SmallDir = 0;
    }

    //
    // Now we have to allocate the memory for the HBINs and fill in
    // the map appropriately.  We are careful never to allocate less
    // than a page to avoid fragmenting pool.  As long as the page
    // size is a multiple of HBLOCK_SIZE (a fairly good assumption as
    // long as HBLOCK_SIZE is 4k) this strategy will prevent pool
    // fragmentation.
    //
    // If we come across an HBIN that is entirely composed of a freed
    // HCELL, then we do not allocate memory, but mark its HBLOCKs in
    // the map as not present.  HvAllocateCell will allocate memory for
    // the bin when it is needed.
    //
    Offset = 0;
    Size = 0;
    Bin = (PHBIN)Image;

    while (Bin < (PHBIN)((PUCHAR)(Image) + Length)) {

        if ( (Bin->Size > (Length-Offset))               ||
             (Bin->Signature != HBIN_SIGNATURE)         ||
             (Bin->FileOffset != (Offset+Size))
           )
        {
            //
            // Bin is bogus
            //
            Status = STATUS_REGISTRY_CORRUPT;
            goto ErrorExit2;
        }

        Size += Bin->Size;
        if ((Size < PAGE_SIZE) &&
            (Size + Length - Offset > PAGE_SIZE)) {

            //
            // We haven't accumulated enough bins to fill up a page
            // yet, and there are still bins left, so group this one
            // in with the next one.
            //
            Bin = (PHBIN)((ULONG_PTR)Bin + Bin->Size);

            continue;

        }

        //
        // We now have a series of HBINs to group together in one
        // chunk of memory.
        //
        NewBins = (PHBIN)(Hive->Allocate)(Size, FALSE);
        if (NewBins==NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto ErrorExit2;        //fixfix
        }
        RtlCopyMemory(NewBins,
                      (PUCHAR)Image+Offset,
                      Size);
        NewBins->MemAlloc = Size;

        //
        // create map entries for each block/page in bin
        //
        Address = (ULONG_PTR)NewBins;
        do {
            CurrentBin = (PHBIN)Address;
            do {
                Me = HvpGetCellMap(Hive, Offset);
                VALIDATE_CELL_MAP(__LINE__,Me,Hive,Offset);
                Me->BlockAddress = Address;
                Me->BinAddress = (ULONG_PTR)CurrentBin;

                if (CurrentBin == NewBins) {
                    Me->BinAddress |= HMAP_NEWALLOC;
                } else {
                    CurrentBin->MemAlloc = 0;
                }
                Address += HBLOCK_SIZE;
                Offset += HBLOCK_SIZE;
            } while ( Address < ((ULONG_PTR)CurrentBin + CurrentBin->Size ));

            if (Hive->ReadOnly == FALSE) {

                //
                // add free cells in the bin to the appropriate free lists
                //
                if ( ! HvpEnlistFreeCells(Hive,
                                          CurrentBin,
                                          CurrentBin->FileOffset,
                                          TailDisplay
                                          )) {
                    Status = STATUS_REGISTRY_CORRUPT;
                    goto ErrorExit2;
                }

            }

        } while ( Address < (ULONG_PTR)NewBins + Size );

        Bin = (PHBIN)((ULONG_PTR)Bin + Bin->Size);
        Size = 0;
    }

    return STATUS_SUCCESS;


ErrorExit2:
    if (d != NULL) {

        //
        // directory was built and allocated, so clean it up
        //

        HvpFreeMap(Hive, d, 0, Tables);
        (Hive->Free)(d, sizeof(HMAP_DIRECTORY));
    }

ErrorExit1:
    return Status;
}

NTSTATUS
HvpInitMap(
    PHHIVE  Hive
    )
/*++

Routine Description:

    Initialize the map for the Stable Volatile storage of the hive.

    Following fields in hive must already be filled in:

         Allocate, Free

    Will initialize Storage structure of HHIVE.

Arguments:

    Hive - Pointer to hive control structure to build map for.

Return Value:

    STATUS_SUCCESS - it worked
    STATUS_xxx - the errorneous status

--*/
{
    PHBASE_BLOCK    BaseBlock;
    ULONG           Length;
    ULONG           MapSlots;
    ULONG           Tables;
    PHMAP_TABLE     t = NULL;
    PHMAP_DIRECTORY d = NULL;
    NTSTATUS        Status;
    PULONG          Vector;

    
    CMLOG(CML_FLOW, CMS_HIVE) {
        KdPrint(("HvpInitMap:\n"));
        KdPrint(("\tHive=%08lx",Hive));
    }

    //
    // Compute size of data region to be mapped
    //
    BaseBlock = Hive->BaseBlock;
    Length = BaseBlock->Length;
    if ((Length % HBLOCK_SIZE) != 0) {
        Status = STATUS_REGISTRY_CORRUPT;
        goto ErrorExit1;
    }
    MapSlots = Length / HBLOCK_SIZE;
    if( MapSlots > 0 ) {
        Tables = (MapSlots-1) / HTABLE_SLOTS;
    } else {
        Tables = 0;
    }

    Hive->Storage[Stable].Length = Length;

    //
    // allocate dirty vector if one is not already present (from HvpRecoverData)
    //

    if (Hive->DirtyVector.Buffer == NULL) {
        Vector = (PULONG)((Hive->Allocate)(ROUND_UP(Length/HSECTOR_SIZE/8,sizeof(ULONG)), TRUE));
        if (Vector == NULL) {
            Status = STATUS_NO_MEMORY;
            goto ErrorExit1;
        }
        RtlZeroMemory(Vector, Length / HSECTOR_SIZE / 8);
        RtlInitializeBitMap(&Hive->DirtyVector, Vector, Length / HSECTOR_SIZE);
        Hive->DirtyAlloc = (Length/HSECTOR_SIZE/8);
    }

    //
    // allocate and build structure for map
    //
    if (Tables == 0) {

        //
        // Just 1 table, no need for directory
        //
        t = (Hive->Allocate)(sizeof(HMAP_TABLE), FALSE);
        if (t == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto ErrorExit1;
        }
        RtlZeroMemory(t, sizeof(HMAP_TABLE));
        Hive->Storage[Stable].Map =
            (PHMAP_DIRECTORY)&(Hive->Storage[Stable].SmallDir);
        Hive->Storage[Stable].SmallDir = t;

    } else {

        //
        // Need directory and multiple tables
        //
        d = (PHMAP_DIRECTORY)(Hive->Allocate)(sizeof(HMAP_DIRECTORY), FALSE);
        if (d == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto ErrorExit1;
        }
        RtlZeroMemory(d, sizeof(HMAP_DIRECTORY));

        //
        // Allocate tables and fill in dir
        //
        if (HvpAllocateMap(Hive, d, 0, Tables) == FALSE) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto ErrorExit2;
        }
        Hive->Storage[Stable].Map = d;
        Hive->Storage[Stable].SmallDir = 0;
    }

    return STATUS_SUCCESS;

ErrorExit2:
    if (d != NULL) {

        //
        // directory was built and allocated, so clean it up
        //

        HvpFreeMap(Hive, d, 0, Tables);
        (Hive->Free)(d, sizeof(HMAP_DIRECTORY));
    }

ErrorExit1:
    return Status;
}

NTSTATUS
HvpEnlistBinInMap(
    PHHIVE  Hive,
    ULONG   Length,
    PHBIN   Bin,
    ULONG   Offset,
    PHCELL_INDEX TailDisplay OPTIONAL
    )
/*++

Routine Description:

    Creates map entries and enlist free cells for the specified bin 

Arguments:

    Hive - Pointer to hive control structure containing the target map

    Length - the Length of the hive image

    Bin - the bin to be enlisted

    Offset - the offset within the hive file

    TailDisplay - array containing the tail ends of the free cell lists - optional

Return Value:

    STATUS_SUCCESS - it worked
    STATUS_REGISTRY_CORRUPT - the bin is inconsistent

--*/
{
    NTSTATUS        Status;
    ULONG           BinOffset;
    ULONG_PTR       Address;
    PHMAP_ENTRY     Me;

    CMLOG(CML_FLOW, CMS_HIVE) {
        KdPrint(("HvpEnlistBinInMap:\n"));
        KdPrint(("\tHive=%08lx\t Offset=%08lx",Hive,Offset));
    }

    //
    // memory was allocated for this bin
    //
    Bin->MemAlloc = Bin->Size;
    
    CMLOG(CML_BIN, CMS_BIN_MAP) {
        KdPrint(("HvpEnlistBinInMap: BinAddress = 0x%08lx\t Size = 0x%lx\n", Bin, Bin->Size));
    }

    //
    // create map entries for each block/page in bin
    //
    BinOffset = Offset;
    for (Address = (ULONG_PTR)Bin;
         Address < ((ULONG_PTR)Bin + Bin->Size);
         Address += HBLOCK_SIZE
        )
    {
        Me = HvpGetCellMap(Hive, Offset);
        VALIDATE_CELL_MAP(__LINE__,Me,Hive,Offset);
        Me->BlockAddress = Address;
        Me->BinAddress = (ULONG_PTR)Bin;
        if (Offset == BinOffset) {
            Me->BinAddress |= HMAP_NEWALLOC;
        }
        Offset += HBLOCK_SIZE;
    }

    if (Hive->ReadOnly == FALSE) {

        //
        // add free cells in the bin to the appropriate free lists
        //
        if ( ! HvpEnlistFreeCells(Hive, Bin, BinOffset,TailDisplay)) {
            HvCheckHiveDebug.Hive = Hive;
            HvCheckHiveDebug.Status = 0xA002;
            HvCheckHiveDebug.Space = Length;
            HvCheckHiveDebug.MapPoint = BinOffset;
            HvCheckHiveDebug.BinPoint = Bin;
            Status = STATUS_REGISTRY_CORRUPT;
            goto ErrorExit;
        }

    }

    //
    // logical consistency check
    //
    ASSERT(Offset == (BinOffset + Bin->Size));

    return STATUS_SUCCESS;

ErrorExit:
    return Status;
}

NTSTATUS
HvpBuildMap(
    PHHIVE  Hive,
    PVOID   Image,
    PHCELL_INDEX TailDisplay OPTIONAL
    )
/*++

Routine Description:

    Creates the map for the Stable storage of the hive, and inits
    the map for the volatile storage.

    Following fields in hive must already be filled in:

         Allocate, Free

    Will initialize Storage structure of HHIVE.

Arguments:

    Hive - Pointer to hive control structure to build map for.

    Image - pointer to in memory image of the hive

Return Value:

    TRUE - it worked
    FALSE - either hive is corrupt or no memory for map

--*/
{
    PHBIN           Bin;
    ULONG           Offset;
    NTSTATUS        Status;
    ULONG           Length;


    CMLOG(CML_FLOW, CMS_HIVE) {
        KdPrint(("HvpBuildMap:\n"));
        KdPrint(("\tHive=%08lx",Hive));
    }

    //
    // Init the map
    //
    Status = HvpInitMap(Hive);

    if( !NT_SUCCESS(Status) ) {
        //
        // just return failure; HvpInitMap took care of cleanup
        //
        return Status;
    }

    //
    // Fill in the map
    //
    Offset = 0;
    Bin = (PHBIN)Image;
    Length = Hive->Storage[Stable].Length;

    while (Bin < (PHBIN)((PUCHAR)(Image) + Length)) {

        //
        // Check the validity of the bin header
        //
        if ( (Bin->Size > Length)                       ||
             (Bin->Size < HBLOCK_SIZE)                  ||
             (Bin->Signature != HBIN_SIGNATURE)         ||
             (Bin->FileOffset != Offset)) {
            //
            // Bin is bogus
            //
            Status = STATUS_REGISTRY_CORRUPT;
            HvCheckHiveDebug.Hive = Hive;
            HvCheckHiveDebug.Status = 0xA001;
            HvCheckHiveDebug.Space = Length;
            HvCheckHiveDebug.MapPoint = Offset;
            HvCheckHiveDebug.BinPoint = Bin;
            goto ErrorExit;
        }

        //
        // enlist this bin
        //
        Status = HvpEnlistBinInMap(Hive, Length, Bin, Offset, TailDisplay);

        if( !NT_SUCCESS(Status) ) {
            goto ErrorExit;
        }

        //
        // the next bin
        //
        Offset += Bin->Size;

        Bin = (PHBIN)((ULONG_PTR)Bin + Bin->Size);
    }

    return STATUS_SUCCESS;


ErrorExit:
    //
    // Clean up the directory table
    //
    HvpCleanMap( Hive );

    return Status;
}


BOOLEAN
HvpEnlistFreeCells(
    PHHIVE  Hive,
    PHBIN   Bin,
    ULONG   BinOffset,
    PHCELL_INDEX TailDisplay OPTIONAL
    )
/*++

Routine Description:

    Scan through the cells in the bin, locating the free ones.
    Enlist them in the hive's free list set.

    N.B.    Bin MUST already be mapped when this is called.

Arguments:

    Hive - pointer to hive control structure map is being built for

    Bin - pointer to bin to enlist cells from

    BinOffset - offset of Bin in image

Return Value:

    FALSE - registry is corrupt

    TRUE - it worked

--*/
{
    PHCELL  p;
    ULONG   celloffset;
    ULONG   size;
    HCELL_INDEX cellindex;

    // PERFNOTE -- Keep this in mind as a possible optimization for NT6.
    // Since now the hive is loaded in chunks of bins, we can drop the 
    // bins that are entirely free!!!!!!
    //

    //
    // Scan all the cells in the bin, total free and allocated, check
    // for impossible pointers.
    //
    celloffset = sizeof(HBIN);
    p = (PHCELL)((PUCHAR)Bin + sizeof(HBIN));

    while (p < (PHCELL)((PUCHAR)Bin + Bin->Size)) {

        //
        // if free cell, check it out, add it to free list for hive
        //
        if (p->Size >= 0) {

            size = (ULONG)p->Size;

            if ( (size > Bin->Size)               ||
                 ( (PHCELL)(size + (PUCHAR)p) >
                   (PHCELL)((PUCHAR)Bin + Bin->Size) ) ||
                 ((size % HCELL_PAD(Hive)) != 0) ||
                 (size == 0) )
            {
                return FALSE;
            }


            //
            // cell is free, and is not obviously corrupt, add to free list
            //
            celloffset = (ULONG)((PUCHAR)p - (PUCHAR)Bin);
            cellindex = BinOffset + celloffset;

            //
            // Enlist this free cell, but do not coalesce with the next free cell
            // as we haven't gotten that far yet.
            //
            HvpEnlistFreeCell(Hive, cellindex, size, Stable, FALSE,TailDisplay);

        } else {

            size = (ULONG)(p->Size * -1);

            if ( (size > Bin->Size)               ||
                 ( (PHCELL)(size + (PUCHAR)p) >
                   (PHCELL)((PUCHAR)Bin + Bin->Size) ) ||
                 ((size % HCELL_PAD(Hive)) != 0) ||
                 (size == 0) )
            {
                return FALSE;
            }

        }

        ASSERT(size >= 0);
        p = (PHCELL)((PUCHAR)p + size);
    }

    return TRUE;
}

VOID
HvpCleanMap(
    PHHIVE  Hive
    )
/*++

Routine Description:

    Cleans all the map allocations for the stable storage

  Arguments:

    Hive - Pointer to hive control structure to build map for.

Return Value:

    None
--*/
{
    ULONG           Length;
    ULONG           MapSlots;
    ULONG           Tables;
    PHMAP_DIRECTORY d = NULL;

    //
    // Compute MapSlots and Tables based on the Length
    //
    Length = Hive->Storage[Stable].Length;
    MapSlots = Length / HBLOCK_SIZE;
    if( MapSlots > 0 ) {
        Tables = (MapSlots-1) / HTABLE_SLOTS;
    } else {
        Tables = 0;
    }

    if( Hive->Storage[Stable].SmallDir == 0 ) {
        //
        // directory was built and allocated, so clean it up
        //

        d = Hive->Storage[Stable].Map;
        if( d != NULL ) {
            HvpFreeMap(Hive, d, 0, Tables);
            (Hive->Free)(d, sizeof(HMAP_DIRECTORY));
        }
    } else {
        //
        // no directory, just a smalldir
        //
        (Hive->Free)(Hive->Storage[Stable].SmallDir, sizeof(HMAP_TABLE));
    }
    
    Hive->Storage[Stable].SmallDir = NULL;
    Hive->Storage[Stable].Map = NULL;
}

VOID
HvpFreeMap(
    PHHIVE          Hive,
    PHMAP_DIRECTORY Dir,
    ULONG           Start,
    ULONG           End
    )
/*++

Routine Description:

    Sweeps through the directory Dir points to and frees Tables.
    Will free Start-th through End-th entries, INCLUSIVE.

Arguments:

    Hive - supplies pointer to hive control block of interest

    Dir - supplies address of an HMAP_DIRECTORY structure

    Start - index of first map table pointer to clean up

    End - index of last map table pointer to clean up

Return Value:

    NONE.

--*/
{
    ULONG   i;

    if (End >= HDIRECTORY_SLOTS) {
        End = HDIRECTORY_SLOTS - 1;
    }

    for (i = Start; i <= End; i++) {
        if (Dir->Directory[i] != NULL) {
            (Hive->Free)(Dir->Directory[i], sizeof(HMAP_TABLE));
            Dir->Directory[i] = NULL;
        }
    }
    return;
}


BOOLEAN
HvpAllocateMap(
    PHHIVE          Hive,
    PHMAP_DIRECTORY Dir,
    ULONG           Start,
    ULONG           End
    )
/*++

Routine Description:

    Sweeps through the directory Dir points to and allocates Tables.
    Will allocate Start-th through End-th entries, INCLUSIVE.

    Does NOT clean up when out of memory, call HvpFreeMap to do that.
Arguments:

    Hive - supplies pointer to hive control block of interest

    Dir - supplies address of an HMAP_DIRECTORY structure

    Start - index of first map table pointer to allocate for

    End - index of last map table pointer to allocate for

Return Value:

    TRUE - it worked

    FALSE - insufficient memory

--*/
{
    ULONG   i;
    PVOID   t;

    for (i = Start; i <= End; i++) {
        ASSERT(Dir->Directory[i] == NULL);
        t = (PVOID)((Hive->Allocate)(sizeof(HMAP_TABLE), FALSE));
        if (t == NULL) {
            return FALSE;
        }
        RtlZeroMemory(t, sizeof(HMAP_TABLE));
        Dir->Directory[i] = (PHMAP_TABLE)t;
    }
    return TRUE;
}
