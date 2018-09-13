/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    hivesync.c

Abstract:

    This module implements procedures to write dirty parts of a hive's
    stable store to backing media.

Author:

    Bryan M. Willman (bryanwi) 28-Mar-92

Environment:


Revision History:

--*/

#include    "cmp.h"

extern  BOOLEAN HvShutdownComplete;     // Set to true after shutdown
                                        // to disable any further I/O

#if DBG
#define DumpDirtyVector(Hive)    \
        {                                                               \
            PRTL_BITMAP BitMap;                                         \
            ULONG BitMapSize;                                           \
            PUCHAR BitBuffer;                                           \
            ULONG i;                                                    \
            UCHAR Byte;                                                 \
                                                                        \
            BitMap = &(Hive->DirtyVector);                              \
            BitMapSize = (BitMap->SizeOfBitMap) / 8;                    \
            BitBuffer = (PUCHAR)(BitMap->Buffer);                       \
            for (i = 0; i < BitMapSize; i++) {                          \
                if ((i % 8) == 0) {                                     \
                    KdPrint(("\n\t"));                                     \
                }                                                       \
                Byte = BitBuffer[i];                                    \
                KdPrint(("%02x ", Byte));                                 \
            }                                                           \
            KdPrint(("\n"));                                               \
        }
#else
#define DumpDirtyVector(Hive)
#endif

//
// Private prototypes
//
BOOLEAN
HvpWriteLog(
    PHHIVE          Hive
    );

BOOLEAN
HvpFindNextDirtyBlock(
    PHHIVE          Hive,
    PRTL_BITMAP     BitMap,
    PULONG          Current,
    PUCHAR          *Address,
    PULONG          Length,
    PULONG          Offset
    );

VOID
HvpDiscardBins(
    PHHIVE  Hive
    );

VOID
HvpTruncateBins(
    PHHIVE  Hive
    );

VOID
HvRefreshHive(
    PHHIVE  Hive
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,HvMarkCellDirty)
#pragma alloc_text(PAGE,HvIsBinDirty)
#pragma alloc_text(PAGE,HvMarkDirty)
#pragma alloc_text(PAGE,HvMarkClean)
#pragma alloc_text(PAGE,HvpGrowLog1)
#pragma alloc_text(PAGE,HvpGrowLog2)
#pragma alloc_text(PAGE,HvSyncHive)
#pragma alloc_text(PAGE,HvpDoWriteHive)
#pragma alloc_text(PAGE,HvpWriteLog)
#pragma alloc_text(PAGE,HvpFindNextDirtyBlock)
#pragma alloc_text(PAGE,HvWriteHive)
#pragma alloc_text(PAGE,HvRefreshHive)
#pragma alloc_text(PAGE,HvpDiscardBins)
#pragma alloc_text(PAGE,HvpTruncateBins)

#ifdef _WRITE_PROTECTED_REGISTRY_POOL
#pragma alloc_text(PAGE,HvpChangeBinAllocation)
#pragma alloc_text(PAGE,HvpMarkBinReadWrite)
#endif

#endif



//
// Code for tracking modifications and ensuring adequate log space
//
BOOLEAN
HvMarkCellDirty(
    PHHIVE      Hive,
    HCELL_INDEX Cell
    )
/*++

Routine Description:

    Marks the data for the specified cell dirty.

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    Cell - hcell_index of cell that is being edited

Return Value:

    TRUE - it worked

    FALSE - could not allocate log space, failure!

--*/
{
    ULONG       Type;
    ULONG       Size;
    PHCELL      pCell;
    PHMAP_ENTRY Me;
    HCELL_INDEX Base;
    PHBIN       Bin;

    CMLOG(CML_MINOR, CMS_IO) {
        KdPrint(("HvMarkCellDirty:\n\t"));
        KdPrint(("Hive:%08lx Cell:%08lx\n", Hive, Cell));
    }

    ASSERT(Hive->Signature == HHIVE_SIGNATURE);
    ASSERT(Hive->ReadOnly == FALSE);
    ASSERT(Hive->DirtyCount == RtlNumberOfSetBits(&Hive->DirtyVector));

    Type = HvGetCellType(Cell);

    if ( (Hive->HiveFlags & HIVE_VOLATILE) ||
         (Type == Volatile) )
    {
        return TRUE;
    }

    pCell = HvpGetHCell(Hive,Cell);
#if DBG
    Me = HvpGetCellMap(Hive, Cell);
    VALIDATE_CELL_MAP(__LINE__,Me,Hive,Cell);
    Bin = (PHBIN)(Me->BinAddress & HMAP_BASE);
    ASSERT(Bin->Signature == HBIN_SIGNATURE);
#endif
    //
    // If it's an old format hive, mark the entire
    // bin dirty, because the Last backpointers are
    // such a pain to deal with in the partial
    // alloc and free-coalescing cases.
    //

    if (USE_OLD_CELL(Hive)) {
        Me = HvpGetCellMap(Hive, Cell);
        VALIDATE_CELL_MAP(__LINE__,Me,Hive,Cell);
        Bin = (PHBIN)(Me->BinAddress & HMAP_BASE);
        Base = Bin->FileOffset;
        Size = Bin->Size;
        return HvMarkDirty(Hive, Base, Size);
    } else {
        if (pCell->Size < 0) {
            Size = -pCell->Size;
        } else {
            Size = pCell->Size;
        }
        ASSERT(Size < Bin->Size);
        return HvMarkDirty(Hive, Cell-FIELD_OFFSET(HCELL,u.NewCell), Size);
    }
}


BOOLEAN
HvIsBinDirty(
    IN PHHIVE Hive,
    IN HCELL_INDEX Cell
    )

/*++

Routine Description:

    Given a hive and a cell, checks whether the bin containing
    that cell has any dirty clusters or not.

Arguments:

    Hive - Supplies a pointer to the hive control structure

    Cell - Supplies the HCELL_INDEX of the Cell.

Return Value:

    TRUE - Bin contains dirty data.

    FALSE - Bin does not contain dirty data.

--*/

{
    ULONG       Type;
    PHCELL      pcell;
    PRTL_BITMAP Bitmap;
    ULONG       First;
    ULONG       Last;
    ULONG       i;
    PHMAP_ENTRY Map;
    PHBIN       Bin;

    CMLOG(CML_MINOR, CMS_IO) {
        KdPrint(("HvIsBinDirty:\n\t"));
        KdPrint(("Hive:%08lx Cell:%08lx\n", Hive, Cell));
    }

    ASSERT(Hive->Signature == HHIVE_SIGNATURE);
    ASSERT(Hive->ReadOnly == FALSE);

    Type = HvGetCellType(Cell);

    if ( (Hive->HiveFlags & HIVE_VOLATILE) ||
         (Type == Volatile) )
    {
        return FALSE;
    }

    Bitmap = &(Hive->DirtyVector);

    Map = HvpGetCellMap(Hive, Cell);
    VALIDATE_CELL_MAP(__LINE__,Map,Hive,Cell);
    Bin = (PHBIN)(Map->BinAddress & HMAP_BASE);
    First = Bin->FileOffset / HSECTOR_SIZE;
    Last = (Bin->FileOffset + Bin->Size - 1) / HSECTOR_SIZE;

    for (i=First; i<=Last; i++) {
        if (RtlCheckBit(Bitmap, i)==1) {
            return(TRUE);
        }
    }
    return(FALSE);
}


BOOLEAN
HvMarkDirty(
    PHHIVE      Hive,
    HCELL_INDEX Start,
    ULONG       Length
    )
/*++

Routine Description:

    Marks the relevent parts of a hive dirty, so that they will
    be flushed to backing store.

    If Hive->Cluster is not 1, then adjacent all logical sectors
    in the given cluster will be forced dirty (and log space
    allocated for them.)  This must be done here rather than in
    HvSyncHive so that we can know how much to grow the log.

    This is a noop for Volatile address range.

    NOTE:   Range will not be marked dirty if operation fails.

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    Start - supplies a hive virtual address (i.e., an HCELL_INDEX or
             like form address) of the start of the area to mark dirty.

    Length - inclusive length in bytes of area to mark dirty.

Return Value:

    TRUE - it worked

    FALSE - could not allocate log space, failure!

--*/
{
    ULONG       Type;
    PRTL_BITMAP BitMap;
    ULONG       First;
    ULONG       Last;
    ULONG       i;
    ULONG       Cluster;
    ULONG       OriginalDirtyCount;
    ULONG       DirtySectors;
    BOOLEAN     Result = TRUE;

    CMLOG(CML_MINOR, CMS_IO) {
        KdPrint(("HvMarkDirty:\n\t"));
        KdPrint(("Hive:%08lx Start:%08lx Length:%08lx\n", Hive, Start, Length));
    }


    ASSERT(Hive->Signature == HHIVE_SIGNATURE);
    ASSERT(Hive->ReadOnly == FALSE);
    ASSERT(Hive->DirtyCount == RtlNumberOfSetBits(&Hive->DirtyVector));

    Type = HvGetCellType(Start);

    if ( (Hive->HiveFlags & HIVE_VOLATILE) ||
         (Type == Volatile) )
    {
        return TRUE;
    }


    BitMap = &(Hive->DirtyVector);
    OriginalDirtyCount = Hive->DirtyCount;

    First = Start / HSECTOR_SIZE;
    Last = (Start + Length - 1) / HSECTOR_SIZE;

    Cluster = Hive->Cluster;
    if (Cluster > 1) {

        //
        // Force Start down to base of cluster
        // Force End up to top of cluster
        //
        First = First & ~(Cluster - 1);
        Last = ROUND_UP(Last+1, Cluster) - 1;
    }

    if (Last >= BitMap->SizeOfBitMap) {
        Last = BitMap->SizeOfBitMap-1;
    }

    //
    // Try and grow the log enough to accomodate all the dirty sectors.
    //
    DirtySectors = 0;
    for (i = First; i <= Last; i++) {
        if (RtlCheckBit(BitMap, i)==0) {
            ++DirtySectors;
        }
    }
    if (DirtySectors != 0) {
        if (HvpGrowLog1(Hive, DirtySectors) == FALSE) {
            return(FALSE);
        }
    
        if ((OriginalDirtyCount == 0) && (First != 0)) {
            Result = HvMarkDirty(Hive, 0, sizeof(HBIN));  // force header of 1st bin dirty
            if (Result==FALSE) {
                return(FALSE);
            }
        }
    
        //
        // Log has been successfully grown, go ahead
        // and set the dirty bits.
        //
        ASSERT(Hive->DirtyCount == RtlNumberOfSetBits(&Hive->DirtyVector));
        Hive->DirtyCount += DirtySectors;
        RtlSetBits(BitMap, First, Last-First+1);
    }

    // mark this bin as writable
    HvpMarkBinReadWrite(Hive,Start);
        
    if (!(Hive->HiveFlags & HIVE_NOLAZYFLUSH)) {
        CmpLazyFlush();
    }
    ASSERT(Hive->DirtyCount == RtlNumberOfSetBits(&Hive->DirtyVector));
    return(TRUE);
}


BOOLEAN
HvMarkClean(
    PHHIVE      Hive,
    HCELL_INDEX Start,
    ULONG       Length
    )
/*++

Routine Description:

    Clears the dirty bits for a given portion of a hive.  This is
    the inverse of HvMarkDirty, although it does not give up any
    file space in the primary or log that HvMarkDirty may have reserved.

    This is a noop for Volatile address range.

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    Start - supplies a hive virtual address (i.e., an HCELL_INDEX or
             like form address) of the start of the area to mark dirty.

    Length - inclusive length in bytes of area to mark dirty.

Return Value:

    TRUE - it worked

--*/
{
    ULONG       Type;
    PRTL_BITMAP BitMap;
    ULONG       First;
    ULONG       Last;
    ULONG       i;
    ULONG       Cluster;

    CMLOG(CML_MINOR, CMS_IO) {
        KdPrint(("HvMarkClean:\n\t"));
        KdPrint(("Hive:%08lx Start:%08lx Length:%08lx\n", Hive, Start, Length));
    }


    ASSERT(Hive->Signature == HHIVE_SIGNATURE);
    ASSERT(Hive->ReadOnly == FALSE);
    ASSERT(Hive->DirtyCount == RtlNumberOfSetBits(&Hive->DirtyVector));

    Type = HvGetCellType(Start);

    if ( (Hive->HiveFlags & HIVE_VOLATILE) ||
         (Type == Volatile) )
    {
        return TRUE;
    }

    BitMap = &(Hive->DirtyVector);

    First = Start / HSECTOR_SIZE;
    Last = (Start + Length - 1) / HSECTOR_SIZE;

    Cluster = Hive->Cluster;
    if (Cluster > 1) {

        //
        // Force Start down to base of cluster
        // Force End up to top of cluster
        //
        First = First & ~(Cluster - 1);
        Last = ROUND_UP(Last+1, Cluster) - 1;
    }

    if (Last >= BitMap->SizeOfBitMap) {
        Last = BitMap->SizeOfBitMap-1;
    }

    //
    // Subtract out the dirty count and
    // and clear the dirty bits.
    //
    for (i=First; i<=Last; i++) {
        if (RtlCheckBit(BitMap,i)==1) {
            --Hive->DirtyCount;
            RtlClearBits(BitMap, i, 1);
        }
    }
    ASSERT(Hive->DirtyCount == RtlNumberOfSetBits(&Hive->DirtyVector));

    return(TRUE);
}



BOOLEAN
HvpGrowLog1(
    PHHIVE  Hive,
    ULONG   Count
    )
/*++

Routine Description:

    Adjust the log for growth in the number of sectors of dirty
    data that are desired.

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    Count - number of additional logical sectors of log space needed

Return Value:

    TRUE - it worked

    FALSE - could not allocate log space, failure!

--*/
{
    ULONG   ClusterSize;
    ULONG   RequiredSize;
    ULONG   tmp;

    CMLOG(CML_MINOR, CMS_IO) {
        KdPrint(("HvpGrowLog1:\n\t"));
        KdPrint(("Hive:%08lx Count:%08lx\n", Hive, Count));
    }

    ASSERT(Hive->ReadOnly == FALSE);
    ASSERT(Hive->DirtyCount == RtlNumberOfSetBits(&Hive->DirtyVector));

    //
    // If logging is off, tell caller world is OK.
    //
    if (Hive->Log == FALSE) {
        return TRUE;
    }

    ClusterSize = Hive->Cluster * HSECTOR_SIZE;

    tmp = Hive->DirtyVector.SizeOfBitMap / 8;   // bytes
    tmp += sizeof(ULONG);                       // signature

    RequiredSize =
        ClusterSize  +                                  // 1 cluster for header
        ROUND_UP(tmp, ClusterSize) +
        ((Hive->DirtyCount + Count) * HSECTOR_SIZE);

    RequiredSize = ROUND_UP(RequiredSize, HLOG_GROW);

    ASSERT(Hive->DirtyCount == RtlNumberOfSetBits(&Hive->DirtyVector));

    if ( ! (Hive->FileSetSize)(Hive, HFILE_TYPE_LOG, RequiredSize)) {
        return FALSE;
    }

    Hive->LogSize = RequiredSize;
    ASSERT(Hive->DirtyCount == RtlNumberOfSetBits(&Hive->DirtyVector));
    return TRUE;
}


BOOLEAN
HvpGrowLog2(
    PHHIVE  Hive,
    ULONG   Size
    )
/*++

Routine Description:

    Adjust the log for growth in the size of the hive, in particular,
    account for the increased space needed for a bigger dirty vector.

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    Size - proposed growth in size in bytes.

Return Value:

    TRUE - it worked

    FALSE - could not allocate log space, failure!

--*/
{
    ULONG   ClusterSize;
    ULONG   RequiredSize;
    ULONG   DirtyBytes;

    CMLOG(CML_MINOR, CMS_IO) {
        KdPrint(("HvpGrowLog2:\n\t"));
        KdPrint(("Hive:%08lx Size:%08lx\n", Hive, Size));
    }

    ASSERT(Hive->ReadOnly == FALSE);
    ASSERT(Hive->DirtyCount == RtlNumberOfSetBits(&Hive->DirtyVector));


    //
    // If logging is off, tell caller world is OK.
    //
    if (Hive->Log == FALSE) {
        return TRUE;
    }

    ASSERT( (Size % HSECTOR_SIZE) == 0 );

    ClusterSize = Hive->Cluster * HSECTOR_SIZE;

    ASSERT( (((Hive->Storage[Stable].Length + Size) / HSECTOR_SIZE) % 8) == 0);

    DirtyBytes = (Hive->DirtyVector.SizeOfBitMap / 8) +
                    ((Size / HSECTOR_SIZE) / 8) +
                    sizeof(ULONG);                      // signature
    DirtyBytes = ROUND_UP(DirtyBytes, ClusterSize);

    RequiredSize =
        ClusterSize  +                                  // 1 cluster for header
        (Hive->DirtyCount * HSECTOR_SIZE) +
        DirtyBytes;

    RequiredSize = ROUND_UP(RequiredSize, HLOG_GROW);

    ASSERT(Hive->DirtyCount == RtlNumberOfSetBits(&Hive->DirtyVector));

    if ( ! (Hive->FileSetSize)(Hive, HFILE_TYPE_LOG, RequiredSize)) {
        return FALSE;
    }

    Hive->LogSize = RequiredSize;
    ASSERT(Hive->DirtyCount == RtlNumberOfSetBits(&Hive->DirtyVector));
    return TRUE;
}



//
// Code for syncing a hive to backing store
//

BOOLEAN
HvSyncHive(
    PHHIVE  Hive
    )
/*++

Routine Description:

    Force backing store to match the memory image of the Stable
    part of the hive's space.

    Logs, primary, and alternate data can be written.  Primary is
    always written.  Normally either a log or an alternate, but
    not both, will also be written.

    It is possible to write only the primary.

    All dirty bits will be set clear.

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

Return Value:

    TRUE - it worked

    FALSE - some failure.

--*/
{
    BOOLEAN oldFlag;

    CMLOG(CML_WORKER, CMS_IO) {
        KdPrint(("HvSyncHive:\n\t"));
        KdPrint(("Hive:%08lx\n", Hive));
    }

    ASSERT(Hive->Signature == HHIVE_SIGNATURE);
    ASSERT(Hive->ReadOnly == FALSE);

    //
    // Punt if post shutdown
    //
    if (HvShutdownComplete) {
        CMLOG(CML_BUGCHECK, CMS_IO) {
            KdPrint(("HvSyncHive:  Attempt to sync AFTER SHUTDOWN\n"));
        }
        return FALSE;
    }

    //
    // If nothing dirty, do nothing
    //
    if (Hive->DirtyCount == 0) {
        return TRUE;
    }

    HvpTruncateBins(Hive);

    //
    // If hive is volatile, do nothing
    //
    if (Hive->HiveFlags & HIVE_VOLATILE) {
        return TRUE;
    }

    CMLOG(CML_FLOW, CMS_IO) {
        KdPrint(("\tDirtyCount:%08lx\n", Hive->DirtyCount));
        KdPrint(("\tDirtyVector:"));
        DumpDirtyVector(Hive);
    }

    //
    // disable hard error popups, to avoid self deadlock on bogus devices
    //
    oldFlag = IoSetThreadHardErrorMode(FALSE);

    //
    // Write a log.
    //
    if (Hive->Log == TRUE) {
        if (HvpWriteLog(Hive) == FALSE) {
            IoSetThreadHardErrorMode(oldFlag);
            return FALSE;
        }
    }

    //
    // Write the primary
    //
    if (HvpDoWriteHive(Hive, HFILE_TYPE_PRIMARY) == FALSE) {
        IoSetThreadHardErrorMode(oldFlag);
        return FALSE;
    }

    //
    // Write an alternate
    //
    if (Hive->Alternate == TRUE) {
        if (HvpDoWriteHive(Hive, HFILE_TYPE_ALTERNATE) == FALSE) {
            IoSetThreadHardErrorMode(oldFlag);
            return FALSE;
        }
    }

    //
    // restore hard error popups mode
    //
    IoSetThreadHardErrorMode(oldFlag);
    
    //
    // Hive was successfully written out, discard any bins marked as
    // discardable.
    //
    HvpDiscardBins(Hive);

    //
    // Clear the dirty map
    //
    RtlClearAllBits(&(Hive->DirtyVector));
    Hive->DirtyCount = 0;

    return TRUE;
}


BOOLEAN
HvpDoWriteHive(
    PHHIVE          Hive,
    ULONG           FileType
    )
/*++

Routine Description:

    Write dirty parts of the hive out to either its primary or alternate
    file.  Write the header, flush, write all data, flush, update header,
    flush.  Assume either logging or primary/alternate pairs used.

    NOTE:   TimeStamp is not set, assumption is that HvpWriteLog set
            that.  It is only used for checking if Logs correspond anyway.

Arguments:

    Hive - pointer to Hive for which dirty data is to be written.

    FileType - indicated whether primary or alternate file should be written.

Return Value:

    TRUE - it worked

    FALSE - it failed

--*/
{
    PHBASE_BLOCK    BaseBlock;
    ULONG           Offset;
    PUCHAR          Address;
    ULONG           Length;
    BOOLEAN         rc;
    ULONG           Current;
    PRTL_BITMAP     BitMap;
    PHMAP_ENTRY     Me;
    PHBIN           Bin;
    BOOLEAN         ShrinkHive;
    PCMP_OFFSET_ARRAY offsetArray;
    CMP_OFFSET_ARRAY offsetElement;
    ULONG Count;
    ULONG SetBitCount;

    CMLOG(CML_MINOR, CMS_IO) {
        KdPrint(("HvpDoWriteHive:\n\t"));
        KdPrint(("Hive:%08lx FileType:%08lx\n", Hive, FileType));
    }

    //
    // flush first, so that the filesystem structures get written to
    // disk if we have grown the file.
    //
    if (!(Hive->FileFlush)(Hive, FileType)) {
        return(FALSE);
    }

    BaseBlock = Hive->BaseBlock;

    if (BaseBlock->Length > Hive->Storage[Stable].Length) {
        ShrinkHive = TRUE;
    } else {
        ShrinkHive = FALSE;
    }

    //
    // --- Write out header first time, flush ---
    //
    ASSERT(BaseBlock->Signature == HBASE_BLOCK_SIGNATURE);
    ASSERT(BaseBlock->Major == HSYS_MAJOR);
    ASSERT(BaseBlock->Format == HBASE_FORMAT_MEMORY);
    ASSERT(Hive->ReadOnly == FALSE);


    if (BaseBlock->Sequence1 != BaseBlock->Sequence2) {

        //
        // Some previous log attempt failed, or this hive needs to
        // be recovered, so punt.
        //
        return FALSE;
    }

    BaseBlock->Length = Hive->Storage[Stable].Length;

    BaseBlock->Sequence1++;
    BaseBlock->Type = HFILE_TYPE_PRIMARY;
    BaseBlock->Cluster = Hive->Cluster;
    BaseBlock->CheckSum = HvpHeaderCheckSum(BaseBlock);

    Offset = 0;
    offsetElement.FileOffset = Offset;
    offsetElement.DataBuffer = (PVOID) BaseBlock;
    offsetElement.DataLength = HSECTOR_SIZE * Hive->Cluster;
    rc = (Hive->FileWrite)(
            Hive,
            FileType,
            &offsetElement,
            1,
            &Offset
            );

    if (rc == FALSE) {
        return FALSE;
    }
    if ( ! (Hive->FileFlush)(Hive, FileType)) {
        return FALSE;
    }
    Offset = ROUND_UP(Offset, HBLOCK_SIZE);

    //
    // --- Write out dirty data (only if there is any) ---
    //

    if (Hive->DirtyVector.Buffer != NULL) {
        //
        // First sector of first bin will always be dirty, write it out
        // with the TimeStamp value overlaid on its Link field.
        //
        BitMap = &(Hive->DirtyVector);

        ASSERT(RtlCheckBit(BitMap, 0) == 1);
        ASSERT(RtlCheckBit(BitMap, (Hive->Cluster - 1)) == 1);
        ASSERT(sizeof(LIST_ENTRY) >= sizeof(LARGE_INTEGER));

        Me = HvpGetCellMap(Hive, 0);
        VALIDATE_CELL_MAP(__LINE__,Me,Hive,0);
        Address = (PUCHAR)Me->BlockAddress;
        Length = Hive->Cluster * HSECTOR_SIZE;
        Bin = (PHBIN)Address;
        Bin->TimeStamp = BaseBlock->TimeStamp;

        offsetElement.FileOffset = Offset;
        offsetElement.DataBuffer = (PVOID) Address;
        offsetElement.DataLength = Length;
        rc = (Hive->FileWrite)(
                Hive,
                FileType,
                &offsetElement,
                1,
                &Offset
                );
        ASSERT((Offset % (Hive->Cluster * HSECTOR_SIZE)) == 0);
        if (rc == FALSE) {
            return FALSE;
        }


        //
        // Write out the rest of the dirty data
        //
        Current = Hive->Cluster;        // don't rewrite 1st bin or header

        SetBitCount = RtlNumberOfSetBits(BitMap);
        offsetArray =
            (PCMP_OFFSET_ARRAY)
            ExAllocatePool(PagedPool,
                           sizeof(CMP_OFFSET_ARRAY) * SetBitCount);
        if (offsetArray == NULL) {
            return FALSE;
        }
        Count = 0;

        while (HvpFindNextDirtyBlock(
                    Hive,
                    BitMap,
                    &Current,
                    &Address,
                    &Length,
                    &Offset
                    ) == TRUE)
        {
            // Gather data into array.
            ASSERT(Count < SetBitCount);
            offsetArray[Count].FileOffset = Offset;
            offsetArray[Count].DataBuffer = Address;
            offsetArray[Count].DataLength = Length;
            Offset += Length;
            Count++;
            ASSERT((Offset % (Hive->Cluster * HSECTOR_SIZE)) == 0);
        }

            rc = (Hive->FileWrite)(
                    Hive,
                    FileType,
            offsetArray,
            Count,
            &Offset             // Just an OUT parameter which returns the point
                                // in the file after the last write.
                    );
        ExFreePool(offsetArray);
            if (rc == FALSE) {
                return FALSE;
        }
    }

    if ( ! (Hive->FileFlush)(Hive, FileType)) {
        return FALSE;
    }

    //
    // --- Write header again to report completion ---
    //
    BaseBlock->Sequence2++;
    BaseBlock->CheckSum = HvpHeaderCheckSum(BaseBlock);
    Offset = 0;

    offsetElement.FileOffset = Offset;
    offsetElement.DataBuffer = (PVOID) BaseBlock;
    offsetElement.DataLength = HSECTOR_SIZE * Hive->Cluster;
    rc = (Hive->FileWrite)(
            Hive,
            FileType,
            &offsetElement,
            1,
            &Offset
            );
    if (rc == FALSE) {
        return FALSE;
    }

    if (ShrinkHive) {
        //
        // Hive has shrunk, give up the excess space.
        //
        CmpDoFileSetSize(Hive, FileType, Hive->Storage[Stable].Length + HBLOCK_SIZE);
    }

    if ( ! (Hive->FileFlush)(Hive, FileType)) {
        return FALSE;
    }

    if ((Hive->Log) &&
        (Hive->LogSize > HLOG_MINSIZE(Hive))) {
        //
        // Shrink log back down, reserve at least two clusters
        // worth of space so that if all the disk space is
        // consumed, there will still be enough space prereserved
        // to allow a minimum of registry operations so the user
        // can log on.
        //
        CmpDoFileSetSize(Hive, HFILE_TYPE_LOG, HLOG_MINSIZE(Hive));
        Hive->LogSize = HLOG_MINSIZE(Hive);
    }

    return TRUE;
}


BOOLEAN
HvpWriteLog(
    PHHIVE          Hive
    )
/*++

Routine Description:

    Write a header, the DirtyVector, and all the dirty data into
    the log file.  Do flushes at the right places.  Update the header.

Arguments:

    Hive - pointer to Hive for which dirty data is to be logged.

Return Value:

    TRUE - it worked

    FALSE - it failed

--*/
{
    PHBASE_BLOCK    BaseBlock;
    ULONG           Offset;
    PUCHAR          Address;
    ULONG           Length;
    BOOLEAN         rc;
    ULONG           Current;
    ULONG           junk;
    ULONG           ClusterSize;
    ULONG           HeaderLength;
    PRTL_BITMAP     BitMap;
    ULONG           DirtyVectorSignature = HLOG_DV_SIGNATURE;
    LARGE_INTEGER   systemtime;
    PCMP_OFFSET_ARRAY offsetArray;
    CMP_OFFSET_ARRAY offsetElement;
    ULONG Count;
    ULONG SetBitCount;

    CMLOG(CML_MINOR, CMS_IO) {
        KdPrint(("HvpWriteLog:\n\t"));
        KdPrint(("Hive:%08lx\n", Hive));
    }

    BitMap = &Hive->DirtyVector;
    //
    // --- Write out header first time, flush ---
    //
    BaseBlock = Hive->BaseBlock;
    ASSERT(BaseBlock->Signature == HBASE_BLOCK_SIGNATURE);
    ASSERT(BaseBlock->Major == HSYS_MAJOR);
    ASSERT(BaseBlock->Format == HBASE_FORMAT_MEMORY);
    ASSERT(Hive->ReadOnly == FALSE);


    if (BaseBlock->Sequence1 != BaseBlock->Sequence2) {

        //
        // Some previous log attempt failed, or this hive needs to
        // be recovered, so punt.
        //
        return FALSE;
    }

    BaseBlock->Sequence1++;
    KeQuerySystemTime(&systemtime);
    BaseBlock->TimeStamp = systemtime;

    BaseBlock->Type = HFILE_TYPE_LOG;

    ClusterSize = Hive->Cluster * HSECTOR_SIZE;
    HeaderLength = ROUND_UP(HLOG_HEADER_SIZE, ClusterSize);
    BaseBlock->Cluster = Hive->Cluster;

    BaseBlock->CheckSum = HvpHeaderCheckSum(BaseBlock);

    Offset = 0;
    offsetElement.FileOffset = Offset;
    offsetElement.DataBuffer = (PVOID) BaseBlock;
    offsetElement.DataLength = HSECTOR_SIZE * Hive->Cluster;
    rc = (Hive->FileWrite)(
            Hive,
            HFILE_TYPE_LOG,
            &offsetElement,
            1,
            &Offset
            );
    if (rc == FALSE) {
        return FALSE;
    }
    Offset = ROUND_UP(Offset, HeaderLength);
    if ( ! (Hive->FileFlush)(Hive, HFILE_TYPE_LOG)) {
        return FALSE;
    }

    //
    // --- Write out dirty vector ---
    //
    ASSERT(sizeof(ULONG) == sizeof(DirtyVectorSignature));  // See GrowLog1 above
    offsetElement.FileOffset = Offset;
    offsetElement.DataBuffer = (PVOID) &DirtyVectorSignature;
    offsetElement.DataLength = sizeof(DirtyVectorSignature);
    rc = (Hive->FileWrite)(
            Hive,
            HFILE_TYPE_LOG,
            &offsetElement,
            1,
            &Offset
            );
    if (rc == FALSE) {
        return FALSE;
    }

    Length = Hive->DirtyVector.SizeOfBitMap / 8;
    Address = (PUCHAR)(Hive->DirtyVector.Buffer);
    offsetElement.FileOffset = Offset;
    offsetElement.DataBuffer = (PVOID) Address;
    offsetElement.DataLength = Length;
    rc = (Hive->FileWrite)(
            Hive,
            HFILE_TYPE_LOG,
            &offsetElement,
            1,
            &Offset
            );
    if (rc == FALSE) {
        return FALSE;
    }
    Offset = ROUND_UP(Offset, ClusterSize);

    //
    // --- Write out body of log ---
    //
    SetBitCount = RtlNumberOfSetBits(BitMap);
    offsetArray =
        (PCMP_OFFSET_ARRAY)
        ExAllocatePool(PagedPool,
                       sizeof(CMP_OFFSET_ARRAY) * SetBitCount);
    if (offsetArray == NULL) {
        return FALSE;
    }
    Count = 0;

    Current = 0;
    while (HvpFindNextDirtyBlock(
                Hive,
                BitMap,
                &Current,
                &Address,
                &Length,
                &junk
                ) == TRUE)
    {
        // Gather data into array.
        ASSERT(Count < SetBitCount);
        offsetArray[Count].FileOffset = Offset;
        offsetArray[Count].DataBuffer = Address;
        offsetArray[Count].DataLength = Length;
        Offset += Length;
        Count++;
        ASSERT((Offset % ClusterSize) == 0);
    }

        rc = (Hive->FileWrite)(
                Hive,
                HFILE_TYPE_LOG,
        offsetArray,
        Count,
        &Offset             // Just an OUT parameter which returns the point
                            // in the file after the last write.
                );
    ExFreePool(offsetArray);
        if (rc == FALSE) {
            return FALSE;
        }

    if ( ! (Hive->FileFlush)(Hive, HFILE_TYPE_LOG)) {
        return FALSE;
    }

    //
    // --- Write header again to report completion ---
    //
    BaseBlock->Sequence2++;
    BaseBlock->CheckSum = HvpHeaderCheckSum(BaseBlock);
    Offset = 0;
    offsetElement.FileOffset = Offset;
    offsetElement.DataBuffer = (PVOID) BaseBlock;
    offsetElement.DataLength = HSECTOR_SIZE * Hive->Cluster;
    rc = (Hive->FileWrite)(
            Hive,
            HFILE_TYPE_LOG,
            &offsetElement,
            1,
            &Offset
            );
    if (rc == FALSE) {
        return FALSE;
    }
    if ( ! (Hive->FileFlush)(Hive, HFILE_TYPE_LOG)) {
        return FALSE;
    }

    return TRUE;
}


BOOLEAN
HvpFindNextDirtyBlock(
    PHHIVE          Hive,
    PRTL_BITMAP     BitMap,
    PULONG          Current,
    PUCHAR          *Address,
    PULONG          Length,
    PULONG          Offset
    )
/*++

Routine Description:

    This routine finds and reports the largest run of dirty logical
    sectors in the hive, which are contiguous in memory and on disk.

Arguments:

    Hive - pointer to Hive of interest.

    BitMap - supplies a pointer to a bitmap structure, which
                describes what is dirty.

    Current - supplies a pointer to a varible that tracks position
                in the bitmap.  It is a bitnumber.  It is updated by
                this call.

    Address - supplies a pointer to a variable to receive a pointer
                to the area in memory to be written out.

    Length - supplies a pointer to a variable to receive the length
                of the region to read/write

    Offset - supplies a pointer to a variable to receive the offset
                in the backing file to which the data should be written.
                (not valid for log files)

Return Value:

    TRUE - more to write, ret values good

    FALSE - all data has been written

--*/
{
    ULONG       i;
    ULONG       EndOfBitMap;
    ULONG       Start;
    ULONG       End;
    HCELL_INDEX FileBaseAddress;
    HCELL_INDEX FileEndAddress;
    PHMAP_ENTRY Me;
    PUCHAR      Block;
    PUCHAR      StartBlock;
    PUCHAR      NextBlock;
    ULONG       RunSpan;
    ULONG       RunLength;
    ULONG       FileLength;
    PFREE_HBIN  FreeBin;

    CMLOG(CML_FLOW, CMS_IO) {
        KdPrint(("HvpFindNextDirtyBlock:\n\t"));
        KdPrint(("Hive:%08lx Current:%08lx\n", Hive, *Current));
    }


    EndOfBitMap = BitMap->SizeOfBitMap;

    if (*Current >= EndOfBitMap) {
        return FALSE;
    }

    //
    // Find next run of set bits
    //
    for (i = *Current; i < EndOfBitMap; i++) {
        if (RtlCheckBit(BitMap, i) == 1) {
            break;
        }
    }
    Start = i;

    for ( ; i < EndOfBitMap; i++) {
        if (RtlCheckBit(BitMap, i) == 0) {
            break;
        }
    }
    End = i;


    //
    // Compute hive virtual addresses, beginning file address, memory address
    //
    FileBaseAddress = Start * HSECTOR_SIZE;
    FileEndAddress = End * HSECTOR_SIZE;
    FileLength = FileEndAddress - FileBaseAddress;
    if (FileLength == 0) {
        *Address = NULL;
        *Current = 0xffffffff;
        *Length = 0;
        return FALSE;
    }
    Me = HvpGetCellMap(Hive, FileBaseAddress);
    VALIDATE_CELL_MAP(__LINE__,Me,Hive,FileBaseAddress);

    if (Me->BinAddress & HMAP_DISCARDABLE) {
        FreeBin = (PFREE_HBIN)Me->BlockAddress;
        StartBlock = (PUCHAR)((Me->BinAddress & HMAP_BASE) + FileBaseAddress - FreeBin->FileOffset );
    } else {
        StartBlock = (PUCHAR)Me->BlockAddress;
    }

    Block = StartBlock;
    ASSERT(((PHBIN)(Me->BinAddress & HMAP_BASE))->Signature == HBIN_SIGNATURE);
    *Address = Block + (FileBaseAddress & HCELL_OFFSET_MASK);

    *Offset = FileBaseAddress + HBLOCK_SIZE;

    //
    // Build up length.  First, account for sectors in first block.
    //
    RunSpan = HSECTOR_COUNT - (Start % HSECTOR_COUNT);

    if ((End - Start) <= RunSpan) {

        //
        // Entire length is in first block, return it
        //
        *Length = FileLength;
        *Current = End;
        return TRUE;

    } else {

        RunLength = RunSpan * HSECTOR_SIZE;
        FileBaseAddress = ROUND_UP(FileBaseAddress+1, HBLOCK_SIZE);

    }

    //
    // Scan forward through blocks, filling up length as we go.
    //
    // NOTE:    This loop grows forward 1 block at time.  If we were
    //          really clever we'd fill forward a bin at time, since
    //          bins are always contiguous.  But most bins will be
    //          one block long anyway, so we won't bother for now.
    //
    while (RunLength < FileLength) {

        Me = HvpGetCellMap(Hive, FileBaseAddress);
        VALIDATE_CELL_MAP(__LINE__,Me,Hive,FileBaseAddress);
        ASSERT(((PHBIN)(Me->BinAddress & HMAP_BASE))->Signature == HBIN_SIGNATURE);

        if (Me->BinAddress & HMAP_DISCARDABLE) {
            FreeBin = (PFREE_HBIN)Me->BlockAddress;
            NextBlock = (PUCHAR)((Me->BinAddress & HMAP_BASE) + FileBaseAddress - FreeBin->FileOffset );
        } else {
            NextBlock = (PUCHAR)Me->BlockAddress;
        }

        if ( (NextBlock - Block) != HBLOCK_SIZE) {

            //
            // We've hit a discontinuity in memory.  RunLength is
            // as long as it's going to get.
            //
            break;
        }


        if ((FileEndAddress - FileBaseAddress) <= HBLOCK_SIZE) {

            //
            // We've reached the tail block, all is contiguous,
            // fill up to end and return.
            //
            *Length = FileLength;
            *Current = End;
            return TRUE;
        }

        //
        // Just another contiguous block, fill forward
        //
        RunLength += HBLOCK_SIZE;
        RunSpan += HSECTOR_COUNT;
        FileBaseAddress += HBLOCK_SIZE;
        Block = NextBlock;
    }

    //
    // We either hit a discontinuity, OR, we're at the end of the range
    // we're trying to fill.  In either case, return.
    //
    *Length = RunLength;
    *Current = Start + RunSpan;
    return TRUE;
}


NTSTATUS
HvWriteHive(
    PHHIVE  Hive
    )
/*++

Routine Description:

    Write the hive out.  Write only to the Primary file, neither
    logs nor alternates will be updated.  The hive will be written
    to the HFILE_TYPE_EXTERNAL handle.

    Intended for use in applications like SaveKey.

    Only Stable storage will be written (as for any hive.)

    Presumption is that layer above has set HFILE_TYPE_EXTERNAL
    handle to point to correct place.

    Applying this call to an active hive will generally hose integrity
    measures.

    HOW IT WORKS:

        Make a new DirtyVector.  Fill it with 1s (all dirty).
        Make hive point at it.  We now have what looks like
        a completely dirty hive.

        Call HvpWriteHive, which will write the whole thing to disk.

        Put back DirtyVector, and free the extra one we used.

        In failure case, force Sequence numbers in Hive and BaseBlock
        to match.

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest.

Return Value:

    Status.

--*/
{
    PULONG          SaveDirtyVector;
    ULONG           SaveDirtyVectorSize;
    PULONG          AltDirtyVector;
    ULONG           AltDirtyVectorSize;
    PHBASE_BLOCK    SaveBaseBlock;
    PHBASE_BLOCK    AltBaseBlock;
    ULONG           Alignment;

    NTSTATUS        status;


    CMLOG(CML_MAJOR, CMS_IO) {
        KdPrint(("HvWriteHive: \n"));
        KdPrint(("\tHive = %08lx\n"));
    }
    ASSERT(Hive->Signature == HHIVE_SIGNATURE);
    ASSERT(Hive->ReadOnly == FALSE);


    //
    // Punt if post shutdown
    //
    if (HvShutdownComplete) {
        CMLOG(CML_BUGCHECK, CMS_IO) {
            KdPrint(("HvWriteHive: Attempt to write hive AFTER SHUTDOWN\n"));
        }
        return STATUS_REGISTRY_IO_FAILED;
    }

    //
    // Splice in a duplicate DirtyVector with all bits set.
    //
    SaveDirtyVector = Hive->DirtyVector.Buffer;
    SaveDirtyVectorSize = Hive->DirtyVector.SizeOfBitMap;
    SaveBaseBlock = Hive->BaseBlock;

    AltDirtyVectorSize = (Hive->Storage[Stable].Length / HSECTOR_SIZE) / 8;
    AltDirtyVector = (Hive->Allocate)(ROUND_UP(AltDirtyVectorSize,sizeof(ULONG)), FALSE);
    if (AltDirtyVector == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit1;
    }
    Hive->DirtyVector.Buffer = AltDirtyVector;
    Hive->DirtyVector.SizeOfBitMap = AltDirtyVectorSize * 8;
    RtlSetAllBits(&(Hive->DirtyVector));

    //
    // Splice in a duplicate BaseBlock
    //
    AltBaseBlock = (Hive->Allocate)(sizeof(HBASE_BLOCK), TRUE);
    if (AltBaseBlock == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Exit2;
    }
    //
    // Make sure the buffer we got back is cluster-aligned. If not, try
    // harder to get an aligned buffer.
    //
    Alignment = Hive->Cluster * HSECTOR_SIZE - 1;
    if (((ULONG_PTR)AltBaseBlock & Alignment) != 0) {
        (Hive->Free)(AltBaseBlock, sizeof(HBASE_BLOCK));
        AltBaseBlock = (PHBASE_BLOCK)((Hive->Allocate)(PAGE_SIZE, TRUE));
        if (AltBaseBlock == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto Exit2;
        }
        //
        // Return the quota for the extra allocation, as we are not really using
        // it and it will not be accounted for later when we free it.
        //
        CmpReleaseGlobalQuota(PAGE_SIZE - sizeof(HBASE_BLOCK));
    }

    RtlMoveMemory(AltBaseBlock, SaveBaseBlock, HSECTOR_SIZE);
    Hive->BaseBlock = AltBaseBlock;

    //
    // Ensure the file can be made big enough, then do the deed
    //
    status = CmpDoFileSetSize(Hive,
                              HFILE_TYPE_EXTERNAL,
                              Hive->Storage[Stable].Length);

    if (NT_SUCCESS(status)) {
        if (!HvpDoWriteHive(Hive, HFILE_TYPE_EXTERNAL)) {
            status = STATUS_REGISTRY_IO_FAILED;
        }
    }

    //
    // Clean up, success or failure
    //
    CmpFree(AltBaseBlock, sizeof(HBASE_BLOCK));

Exit2:
    CmpFree(AltDirtyVector, ROUND_UP(AltDirtyVectorSize,sizeof(ULONG)));

Exit1:
    Hive->DirtyVector.Buffer = SaveDirtyVector;
    Hive->DirtyVector.SizeOfBitMap = SaveDirtyVectorSize;
    Hive->BaseBlock = SaveBaseBlock;
    return status;
}


VOID
HvRefreshHive(
    PHHIVE  Hive
    )
/*++

Routine Description:

    Undo the last sync.  We do this by reading back all data which
    has been marked dirty from the file into memory.  Update the
    free list.  We will then clear the dirty vector.

    Any growth since the last sync will be discarded, and in fact,
    the size of the file will be set down.

    WARNNOTE:   Failure will cause a bugcheck, as we cannot
                keep the hive consistent if we fail part way through.

    All I/O is done via HFILE_TYPE_PRIMARY.

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest.

Return Value:

    NONE. Either works or BugChecks.

--*/
{
    ULONG       Offset;
    ULONG       ReadLength;
    ULONG       checkstatus;
    PUCHAR      Address;
    ULONG       Current;
    PRTL_BITMAP BitMap;
    BOOLEAN     rc;
    ULONG       Start;
    ULONG       End;
    ULONG       BitLength;
    HCELL_INDEX TailStart;
    HCELL_INDEX p;
    PHMAP_ENTRY t;
    ULONG       i;
    PHBIN       Bin;
    PLIST_ENTRY List;
    PFREE_HBIN  FreeBin;
    PHMAP_ENTRY Map;
    HCELL_INDEX RootCell;
    PCM_KEY_NODE RootNode;
    HCELL_INDEX LinkCell;

    //
    // this array stores the last elements in each free cell list for the stable storage
    //
    HCELL_INDEX     TailDisplay[HHIVE_FREE_DISPLAY_SIZE];

    //
    // noop or assert on various uninteresting or bogus conditions
    //
    if (Hive->DirtyCount == 0) {
        return;
    }
    ASSERT(Hive->HiveFlags & HIVE_NOLAZYFLUSH);
    ASSERT(Hive->Storage[Volatile].Length == 0);

    //
    // be sure the hive is not already trash
    //
    checkstatus = HvCheckHive(Hive, NULL);
    if (checkstatus != 0) {
        KeBugCheckEx(REGISTRY_ERROR,7,0,(ULONG_PTR)Hive,checkstatus);
    }

    Hive->RefreshCount++;

    //
    // Capture the LinkCell backpointer in the hive's root cell. We need this in case
    // the first bin is overwritten with what was on disk.
    //
    RootCell = Hive->BaseBlock->RootCell;
    RootNode = (PCM_KEY_NODE)HvGetCell(Hive, RootCell);
    LinkCell = RootNode->Parent;

    //
    // Any bins that have been marked as discardable, but not yet flushed to
    // disk, are going to be overwritten with old data.  Bring them back into
    // memory and remove their FREE_HBIN marker from the list.
    //
    List = Hive->Storage[Stable].FreeBins.Flink;
    while (List != &Hive->Storage[Stable].FreeBins) {

        FreeBin = CONTAINING_RECORD(List, FREE_HBIN, ListEntry);
        List = List->Flink;

        if (FreeBin->Flags & FREE_HBIN_DISCARDABLE) {
            for (i=0; i<FreeBin->Size; i+=HBLOCK_SIZE) {
                Map = HvpGetCellMap(Hive, FreeBin->FileOffset+i);
                VALIDATE_CELL_MAP(__LINE__,Map,Hive,FreeBin->FileOffset+i);
                Map->BlockAddress = (Map->BinAddress & HMAP_BASE)+i;
                Map->BinAddress &= ~HMAP_DISCARDABLE;
            }
            RemoveEntryList(&FreeBin->ListEntry);
            (Hive->Free)(FreeBin, sizeof(FREE_HBIN));
        }
    }

    //
    // OverRead base block.
    //
    Offset = 0;
    if ( (Hive->FileRead)(
            Hive,
            HFILE_TYPE_PRIMARY,
            &Offset,
            Hive->BaseBlock,
            HBLOCK_SIZE
            ) != TRUE)
    {
        KeBugCheckEx(REGISTRY_ERROR,7,1,0,0);
    }
    TailStart = (HCELL_INDEX)(Hive->BaseBlock->Length);

    //
    // Free "tail" memory and maps for it, update hive size pointers
    //
    HvFreeHivePartial(Hive, TailStart, Stable);

    //
    // Clear dirty vector for data past Hive->BaseBlock->Length
    //
    Start = Hive->BaseBlock->Length / HSECTOR_SIZE;
    End = Hive->DirtyVector.SizeOfBitMap;
    BitLength = End - Start;

    RtlClearBits(&(Hive->DirtyVector), Start, BitLength);

    //
    // Scan dirty blocks.  Read contiguous blocks off disk into hive.
    // Stop when we get to reduced length.
    //
    BitMap = &(Hive->DirtyVector);
    Current = 0;
    while (HvpFindNextDirtyBlock(
                Hive,
                &Hive->DirtyVector,
                &Current, &Address,
                &ReadLength,
                &Offset
                ))
    {
        ASSERT(Offset < (Hive->BaseBlock->Length + sizeof(HBASE_BLOCK)));
        rc = (Hive->FileRead)(
                Hive,
                HFILE_TYPE_PRIMARY,
                &Offset,
                (PVOID)Address,
                ReadLength
                );
        if (rc == FALSE) {
            KeBugCheckEx(REGISTRY_ERROR,7,2,(ULONG_PTR)&Offset,rc);
        }
    }

    //
    // If we read the start of any HBINs into memory, it is likely
    // their MemAlloc fields are invalid.  Walk through the HBINs
    // and write valid MemAlloc values for any HBINs whose first
    // sector was reread.
    //
    p=0;
    while (p < Hive->Storage[Stable].Length) {
        t = HvpGetCellMap(Hive, p);
        VALIDATE_CELL_MAP(__LINE__,t,Hive,p);
        Bin = (PHBIN)(t->BlockAddress & HMAP_BASE);

        if ((t->BinAddress & HMAP_DISCARDABLE)==0) {
            if (RtlCheckBit(&Hive->DirtyVector, p / HSECTOR_SIZE)==1) {
                //
                // The first sector in the HBIN is dirty.
                //
                // Reset the BinAddress to the Block address to cover
                // the case where a few smaller bins have been coalesced
                // into a larger bin. We want the smaller bins back now.
                //
                t->BinAddress = (t->BinAddress & ~HMAP_BASE) | t->BlockAddress;

                // Check the map to see if this is the start
                // of a memory allocation or not.
                //

                if (t->BinAddress & HMAP_NEWALLOC) {
                    //
                    // Walk through the map to determine the length
                    // of the allocation.
                    //
                    Bin->MemAlloc = 0;
                    do {
                        t = HvpGetCellMap(Hive, p+Bin->MemAlloc+HBLOCK_SIZE);
                        Bin->MemAlloc += HBLOCK_SIZE;
                        if (p+Bin->MemAlloc == Hive->Storage[Stable].Length) {
                            //
                            // Reached the end of the hive.
                            //
                            break;
                        }
                        VALIDATE_CELL_MAP(__LINE__,t,Hive,p+Bin->MemAlloc);
                    } while ( (t->BinAddress & HMAP_NEWALLOC) == 0);

                } else {
                    Bin->MemAlloc = 0;
                }
            }

            p += Bin->Size;

        } else {
            FreeBin = (PFREE_HBIN)t->BlockAddress;
            p += FreeBin->Size;
        }
    }

    //
    // be sure we haven't filled memory with trash
    //
    checkstatus = HvCheckHive(Hive, NULL);
    if (checkstatus != 0) {
        KeBugCheckEx(REGISTRY_ERROR,7,3,(ULONG_PTR)Hive,checkstatus);
    }

    //
    // reinit the free list
    //
    for (i = 0; i < HHIVE_FREE_DISPLAY_SIZE; i++) {
        Hive->Storage[Stable].FreeDisplay[i] = HCELL_NIL;
        TailDisplay[i] = HCELL_NIL;
    }
    Hive->Storage[Stable].FreeSummary = 0;

    //
    // rebuild the free list
    //
    p = 0;
    while (p < Hive->Storage[Stable].Length) {
        t = HvpGetCellMap(Hive, p);
        VALIDATE_CELL_MAP(__LINE__,t,Hive,p);

        if ((t->BinAddress & HMAP_DISCARDABLE) == 0) {
            Bin = (PHBIN)((t->BinAddress) & HMAP_BASE);

            if ( ! HvpEnlistFreeCells(Hive, Bin, Bin->FileOffset,TailDisplay)) {
                KeBugCheckEx(REGISTRY_ERROR,7,5,(ULONG_PTR)Bin,Bin->FileOffset);
            }

            p = (ULONG)p + Bin->Size;
        } else {
            FreeBin = (PFREE_HBIN)t->BlockAddress;
            p = (ULONG)p + FreeBin->Size;
        }
    }

    //
    // Finally we need to rewrite the parent field in the root hcell. This is
    // patched in at hive load time so the correct value could have just been
    // overwritten with whatever happened to be on disk.
    //
    RootNode = (PCM_KEY_NODE)HvGetCell(Hive, RootCell);
    RootNode->Parent = LinkCell;
    RootNode->Flags |= KEY_HIVE_ENTRY | KEY_NO_DELETE;


    //
    // be sure the structure of the thing is OK after all this
    //
    checkstatus = CmCheckRegistry((PCMHIVE)Hive, FALSE);
    if (checkstatus != 0) {
        KeBugCheckEx(REGISTRY_ERROR,7,6,(ULONG_PTR)Hive,checkstatus);
    }

    //
    // Clear dirty vector.
    //
    RtlClearAllBits(&(Hive->DirtyVector));
    Hive->DirtyCount = 0;


    //
    // Adjust the file size, if this fails, ignore it, since it just
    // means the file is too big.
    //
    (Hive->FileSetSize)(
        Hive,
        HFILE_TYPE_PRIMARY,
        (Hive->BaseBlock->Length + HBLOCK_SIZE)
        );

    return;
}


VOID
HvpDiscardBins(
    IN PHHIVE Hive
    )

/*++

Routine Description:

    Walks through the dirty bins in a hive to see if any are marked
    discardable.  If so, they are discarded and the map is updated to
    reflect this.

Arguments:

    Hive - Supplies the hive control structure.

Return Value:

    None.

--*/

{
    PHBIN Bin;
    PHMAP_ENTRY Map;
    PHMAP_ENTRY PreviousMap;
    PHMAP_ENTRY NextMap;
    PFREE_HBIN FreeBin;
    PFREE_HBIN PreviousFreeBin;
    PFREE_HBIN NextFreeBin;
    PLIST_ENTRY List;

    List = Hive->Storage[Stable].FreeBins.Flink;

    while (List != &Hive->Storage[Stable].FreeBins) {
        ASSERT_LISTENTRY(List);
        FreeBin = CONTAINING_RECORD(List, FREE_HBIN, ListEntry);

        if (FreeBin->Flags & FREE_HBIN_DISCARDABLE) {
            Map = HvpGetCellMap(Hive, FreeBin->FileOffset);
            VALIDATE_CELL_MAP(__LINE__,Map,Hive,FreeBin->FileOffset);
            Bin = (PHBIN)(Map->BinAddress & HMAP_BASE);
            ASSERT(Map->BinAddress & HMAP_DISCARDABLE);
            //
            // Note we use ExFreePool directly here to avoid
            // giving back the quota for this bin. By charging
            // registry quota for discarded bins, we prevent
            // sparse hives from requiring more quota after
            // a reboot than on a running system.
            //
            ExFreePool(Bin);
            FreeBin->Flags &= ~FREE_HBIN_DISCARDABLE;
        }
        List=List->Flink;
    }

}



VOID
HvpTruncateBins(
    IN PHHIVE Hive
    )

/*++

Routine Description:

    Attempts to shrink the hive by truncating any bins that are discardable at
    the end of the hive.  Applies to both stable and volatile storage.

Arguments:

    Hive - Supplies the hive to be truncated.

Return Value:

    None.

--*/

{
    HSTORAGE_TYPE i;
    PHMAP_ENTRY Map;
    ULONG NewLength;
    PFREE_HBIN FreeBin;

    //
    // stable and volatile
    //
    for (i=0;i<HTYPE_COUNT;i++) {

        //
        // find the last in-use bin in the hive
        //
        NewLength = Hive->Storage[i].Length;

        while (NewLength > 0) {
            Map = HvpGetCellMap(Hive, (NewLength - HBLOCK_SIZE) + (i*HCELL_TYPE_MASK));
            VALIDATE_CELL_MAP(__LINE__,Map,Hive,(NewLength - HBLOCK_SIZE) + (i*HCELL_TYPE_MASK));
            if (Map->BinAddress & HMAP_DISCARDABLE) {
                FreeBin = (PFREE_HBIN)Map->BlockAddress;
                NewLength = FreeBin->FileOffset;
            } else {
                break;
            }
        }

        if (NewLength < Hive->Storage[i].Length) {
            //
            // There are some free bins to truncate.
            //
            HvFreeHivePartial(Hive, NewLength, i);
        }
    }
}

#ifdef _WRITE_PROTECTED_REGISTRY_POOL

VOID
HvpChangeBinAllocation(
    PHBIN       Bin,
    BOOLEAN     ReadOnly
    )
{
    ASSERT(Bin->Signature == HBIN_SIGNATURE);
    //
    // Here to call the code to mark the memory pointed by Bin as Read/Write or ReadOnly, depending on the ReadOnly argument
    //
}

VOID
HvpMarkBinReadWrite(
    PHHIVE      Hive,
    HCELL_INDEX Cell
    )
/*++

Routine Description:

    Marks the memory allocated for the bin containing the specified cell as read/write.

Arguments:

    Hive - supplies a pointer to the hive control structure for the
            hive of interest

    Cell - hcell_index of cell 

Return Value:

    NONE (It should work!)

--*/
{
    ULONG       Type;
    PHMAP_ENTRY Me;
    PHBIN       Bin;

    ASSERT(Hive->Signature == HHIVE_SIGNATURE);
    ASSERT(Hive->DirtyCount == RtlNumberOfSetBits(&Hive->DirtyVector));

    Type = HvGetCellType(Cell);

    if ( (Hive->HiveFlags & HIVE_VOLATILE) ||
         (Type == Volatile) )
    {
        // nothing to do on a volatile hive
        return;
    }

    Me = HvpGetCellMap(Hive, Cell);
    VALIDATE_CELL_MAP(__LINE__,Me,Hive,Cell);
    Bin = (PHBIN)(Me->BinAddress & HMAP_BASE);
    
    HvpChangeBinAllocation(Bin,FALSE);

}

#endif

