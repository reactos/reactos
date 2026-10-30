////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////
/*
        Module name:

   alloc.cpp

        Abstract:

   This file contains filesystem-specific routines
   responsible for disk space management

*/

#include "udf.h"

#define         UDF_BUG_CHECK_ID                UDF_FILE_UDF_INFO_ALLOC

static const int8 bit_count_tab[] = {
    0, 1, 1, 2, 1, 2, 2, 3,   1, 2, 2, 3, 2, 3, 3, 4,
    1, 2, 2, 3, 2, 3, 3, 4,   2, 3, 3, 4, 3, 4, 4, 5,
    1, 2, 2, 3, 2, 3, 3, 4,   2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5,   3, 4, 4, 5, 4, 5, 5, 6,
    1, 2, 2, 3, 2, 3, 3, 4,   2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5,   3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5,   3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6,   4, 5, 5, 6, 5, 6, 6, 7,

    1, 2, 2, 3, 2, 3, 3, 4,   2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5,   3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5,   3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6,   4, 5, 5, 6, 5, 6, 6, 7,
    2, 3, 3, 4, 3, 4, 4, 5,   3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6,   4, 5, 5, 6, 5, 6, 6, 7,
    3, 4, 4, 5, 4, 5, 5, 6,   4, 5, 5, 6, 5, 6, 6, 7,
    4, 5, 5, 6, 5, 6, 6, 7,   5, 6, 6, 7, 6, 7, 7, 8
};

/*
    RtlFindNextForwardRunSet — not exported by ReactOS ntoskrnl,
    but statically linked into MS udfs.sys. Same algorithm here.
    Finds the next run of set (1) bits starting at or after FromIndex.
 */
static
ULONG
UDFBitmapFindNextRunSet(
    IN PRTL_BITMAP BitMapHeader,
    IN ULONG FromIndex,
    OUT PULONG StartingRunIndex
    )
{
    ULONG Size = BitMapHeader->SizeOfBitMap;
    PULONG Buf = BitMapHeader->Buffer;
    ULONG Idx = FromIndex;

    if (Size == 0) {
        *StartingRunIndex = Idx;
        return 0;
    }

    ULONG LastWord = (Size - 1) >> 5;

    // Phase 1: skip clear bits, find first set bit
    // Skip partial first word
    ULONG Word = Idx >> 5;
    if ((Idx & 31) && Word <= LastWord) {
        // Mask off bits below Idx within this word
        ULONG Val = Buf[Word] & ~((1UL << (Idx & 31)) - 1);
        if (Val) {
            // Found a set bit — count trailing zeros
            Idx = (Word << 5);
            while (!(Val & 1)) { Val >>= 1; Idx++; }
            goto FoundStart;
        }
        Idx = (Word + 1) << 5;
        Word++;
    }
    // Skip whole zero words
    while (Word <= LastWord && Buf[Word] == 0) { Word++; Idx += 32; }
    // Scan within the non-zero word
    if (Word <= LastWord) {
        ULONG Val = Buf[Word];
        Idx = Word << 5;
        while (!(Val & 1)) { Val >>= 1; Idx++; }
    }

FoundStart:
    if (Idx >= Size) {
        *StartingRunIndex = Size;
        return 0;
    }
    *StartingRunIndex = Idx;

    // Phase 2: count consecutive set bits
    ULONG End = Idx;
    Word = End >> 5;
    // Check partial first word
    if ((End & 31) && Word <= LastWord) {
        ULONG Val = Buf[Word] >> (End & 31);
        while ((End & 31) && (Val & 1)) { Val >>= 1; End++; }
        if (End & 31) goto Done;  // Run ended mid-word
        Word++;
    }
    // Skip whole 0xFFFFFFFF words
    while (Word <= LastWord && Buf[Word] == 0xFFFFFFFF) { Word++; End += 32; }
    // Scan partial word at run end
    if (Word <= LastWord) {
        ULONG Val = Buf[Word];
        while ((Val & 1)) { Val >>= 1; End++; }
    }

Done:
    if (End > Size) End = Size;
    return End - Idx;
}

/*
    Pin the bitmap cache page containing the given LBN (BitIndex).
    Initializes RTL_BITMAP on the pinned data for bounds-safe access.
    Stores result in Vcb->BitmapRtl, BitmapPageStartLbn, BitmapPageBitCount.
 */
VOID
UDFPinBitmapPage(
    IN PVCB Vcb,
    IN ULONG Lbn
    )
{
    // Calculate stream byte offset for this LBN
    ULONG streamOffset = Vcb->BitmapDataOffset + (Lbn >> 3);
    ULONG pinBase = streamOffset & ~(BITMAP_PIN_GRANULARITY - 1);

    if (Vcb->BitmapBcb && Vcb->BitmapPinnedOffset == pinBase) {
        // Already pinned at this offset — reuse
        return;
    }

    // Unpin current page if any
    if (Vcb->BitmapBcb) {
        CcUnpinData(Vcb->BitmapBcb);
        Vcb->BitmapBcb = NULL;
    }

    // Calculate pin length
    ULONG allocEnd = (ULONG)Vcb->BitmapFcb->Header.AllocationSize.QuadPart;
    ULONG pinEnd = min(pinBase + BITMAP_PIN_GRANULARITY, allocEnd);
    ULONG pinLength = pinEnd - pinBase;

    LARGE_INTEGER offset;
    offset.QuadPart = pinBase;
    PVOID buffer;

    CcPinRead(Vcb->BitmapStreamFileObject,
              &offset,
              pinLength,
              TRUE,
              &Vcb->BitmapBcb,
              &buffer);

    Vcb->BitmapPinnedData = (PUCHAR)buffer;
    Vcb->BitmapPinnedOffset = pinBase;
    Vcb->BitmapPinnedLength = pinLength;

    // Initialize RTL_BITMAP on the bitmap bit data within this pinned region.
    // The bitmap stream layout: [SBD header (BitmapDataOffset bytes)] [bit data...]
    // On page 0, skip the SBD header; on other pages, data starts at pinBase.
    ULONG dataStart = max(Vcb->BitmapDataOffset, pinBase);
    ULONG dataEnd = min(pinBase + pinLength,
                        Vcb->BitmapDataOffset + Vcb->FSBM_ByteCount);

    PULONG bitmapBits = (PULONG)(Vcb->BitmapPinnedData + (dataStart - pinBase));
    ULONG startLbn = (dataStart - Vcb->BitmapDataOffset) << 3;
    ULONG bitCount = (dataEnd - dataStart) << 3;

    // Clamp to total bitmap size
    if (startLbn + bitCount > Vcb->FSBM_BitCount) {
        bitCount = Vcb->FSBM_BitCount - startLbn;
    }

    RtlInitializeBitMap(&Vcb->BitmapRtl, bitmapBits, bitCount);
    Vcb->BitmapPageStartLbn = startLbn;
    Vcb->BitmapPageBitCount = bitCount;
}

VOID
UDFUnpinBitmapPage(
    IN PVCB Vcb
    )
{
    if (Vcb->BitmapBcb) {
        CcUnpinData(Vcb->BitmapBcb);
        Vcb->BitmapBcb = NULL;
        Vcb->BitmapPinnedData = NULL;
    }
}

VOID
UDFDirtyBitmapPage(
    IN PVCB Vcb
    )
{
    if (Vcb->BitmapBcb) {
        CcSetDirtyPinnedData(Vcb->BitmapBcb, NULL);
    }
}

/*
    Check if a single bitmap bit is free (1 = free in UDF).
    Pins the relevant page internally.
 */
BOOLEAN
UDFIsBitmapBitFree(
    IN PVCB Vcb,
    IN ULONG Lbn
    )
{
    BOOLEAN isFree;

    UDFPinBitmapPage(Vcb, Lbn);
    ULONG idx = Lbn - Vcb->BitmapPageStartLbn;
    isFree = RtlCheckBit(&Vcb->BitmapRtl, idx) ? TRUE : FALSE;
    /* A BCB resource belongs to the pinning thread. Never leave a read
       helper's pin in the VCB for a later request on another thread. */
    UDFUnpinBitmapPage(Vcb);
    return isFree;
}

/*
    Count consecutive free (set) bits starting from Start LBN, up to Limit.
    Uses RTL_BITMAP across pinned page boundaries.
 */
SIZE_T
UDFGetCachedBitmapLen(
    IN PVCB Vcb,
    IN ULONG Start,
    IN ULONG Limit
    )
{
    if (Start >= Limit) return 0;

    SIZE_T totalLen = 0;
    ULONG pos = Start;

    while (pos < Limit) {
        UDFPinBitmapPage(Vcb, pos);

        ULONG localIdx = pos - Vcb->BitmapPageStartLbn;
        ULONG pageEnd = min(Vcb->BitmapPageStartLbn + Vcb->BitmapPageBitCount, Limit);

        // Find run of set (free) bits starting at localIdx
        ULONG runStartIdx;
        ULONG runLen = UDFBitmapFindNextRunSet(&Vcb->BitmapRtl, localIdx, &runStartIdx);

        // If the run doesn't start at our position, the bit at pos is not free
        if (runLen == 0 || runStartIdx != localIdx) {
            break;
        }

        // Clamp to page/limit boundary
        ULONG maxBits = pageEnd - pos;
        if (runLen > maxBits) runLen = maxBits;

        totalLen += runLen;
        pos += runLen;

        // If run ended before page boundary, the next bit is used — stop
        if (pos < pageEnd) break;
    }

    UDFUnpinBitmapPage(Vcb);
    return totalLen;
}

static
ULONG
UDFCountFreeBits(
    IN PRTL_BITMAP Bitmap,
    IN ULONG Start,
    IN ULONG Count
)
{
    ULONG i = 0, n = 0;

    if (RtlAreBitsClear(Bitmap, Start, Count))
        return 0;

    if (RtlAreBitsSet(Bitmap, Start, Count)) 
        return Count;
    
    for (i = 0; i < Count; i++) {
        if (RtlCheckBit(Bitmap, Start + i)) 
            n++;
    }

    return n;
}

/*
    This routine converts physical address to logical in specified partition
 */
uint32
UDFPhysLbaToPart(
    IN PVCB Vcb,
    IN uint32 RefPartNum,
    IN uint32 Addr
    )
{
    uint32 retval = 0;
    if (RefPartNum < Vcb->PartitionMaps) {
        if (Vcb->MetadataFileInfo && (RefPartNum == Vcb->MetadataRef))
            retval = UDFPhysLbaToMetadata(Vcb, Addr);
        else
            retval = (Addr - Vcb->Partitions[RefPartNum].PartitionRoot);
    }

#ifdef UDF_DBG
    {
        // validate return value
        lb_addr locAddr;
        locAddr.logicalBlockNum = retval;
        locAddr.partitionReferenceNum = (uint16)RefPartNum;
        UDFPartLbaToPhys(Vcb, &locAddr);
    }
#endif // UDF_DBG

    return retval;
} // end UDFPhysLbaToPart()

/*
    This routine returns physical Lba for partition-relative addr
 */
uint32
__fastcall
UDFPartLbaToPhys(
  IN PVCB Vcb,
  IN lb_addr* Addr
  )
{
    uint32 i, a;
    if (Addr->partitionReferenceNum >= Vcb->PartitionMaps) {
        AdPrint(("UDFPartLbaToPhys: part %x, lbn %x (err)\n",
            Addr->partitionReferenceNum, Addr->logicalBlockNum));
        if (Vcb->PartitionMaps &&
           (Vcb->CompatFlags & UDF_VCB_IC_INSTANT_COMPAT_ALLOC_DESCS)) {
            AdPrint(("UDFPartLbaToPhys: try to recover: part %x -> %x\n",
                Addr->partitionReferenceNum, Vcb->PartitionMaps-1));
            Addr->partitionReferenceNum = (USHORT)(Vcb->PartitionMaps-1);
        } else {
            return LBA_OUT_OF_EXTENT;
        }
    }
    i = Addr->partitionReferenceNum;
    if (Addr->logicalBlockNum >= Vcb->Partitions[i].PartitionLen) {
        AdPrint(
            ("UDFPartLbaToPhys: partition %x, paritionRoot %x, LBN %x, partionLength %x\n",
                i, Vcb->Partitions[i].PartitionRoot, Addr->logicalBlockNum,
                Vcb->Partitions[i].PartitionLen)
        );
        BrutePoint();
        return LBA_OUT_OF_EXTENT;
    }
    if (Vcb->MetadataFileInfo && (i == Vcb->MetadataRef))
        return UDFMetadataLbaToPhys(Vcb, Addr->logicalBlockNum, NULL);
    return a = Vcb->Partitions[i].PartitionRoot + Addr->logicalBlockNum;
} // end UDFPartLbaToPhys()

uint32
__fastcall
UDFMetadataLbaToPhys(
    IN PVCB Vcb,
    IN uint32 Lbn,
    OUT PSIZE_T AvailLength
)
{
    SIZE_T avail;
    uint32 lba, flags;

    if (
        !Vcb->MetadataFileInfo ||
        !Vcb->MetadataFileInfo->Dloc->DataLoc.Mapping
    )
        return LBA_OUT_OF_EXTENT;

    lba = UDFExtentOffsetToLba(Vcb, Vcb->MetadataFileInfo->Dloc->DataLoc.Mapping,
                               ((int64)Lbn) << Vcb->SectorShift, NULL, &avail, &flags, NULL);

    if (
        (lba == LBA_OUT_OF_EXTENT) ||
        (flags == EXTENT_NOT_RECORDED_NOT_ALLOCATED)
    ) {
        AdPrint(("UDFMetadataLbaToPhys: Metadata File does not have lbn %x\n", Lbn));
        return LBA_OUT_OF_EXTENT;
    }

    if (AvailLength)
        *AvailLength = avail;
    
    return lba;
}

uint32
__fastcall
UDFPhysLbaToMetadata(
    IN PVCB Vcb,
    IN uint32 Lba
)
{
    PEXTENT_MAP Map;

    if (
        !Vcb->MetadataFileInfo ||
        !(Map = Vcb->MetadataFileInfo->Dloc->DataLoc.Mapping)
    )
        return LBA_OUT_OF_EXTENT;

    uint32 len, off = 0;    

    for (; Map->extLength; Map++) {
        len = (Map->extLength & UDF_EXTENT_LENGTH_MASK) >> Vcb->SectorShift;
        
        if (
            ((Map->extLength >> 30) != EXTENT_NOT_RECORDED_NOT_ALLOCATED) &&
            (Map->extLocation <= Lba) && (Lba < Map->extLocation + len)
        )
            return off + (Lba - Map->extLocation);
        
        off += len;
    }

    return LBA_OUT_OF_EXTENT;
}

typedef struct _UDF_METADATA_GROWTH_PROFILE {
    ULONGLONG ScanTicks;
    ULONGLONG AllocateTicks;
    ULONGLONG ZeroTicks;
    ULONGLONG CommitTicks;
    uint32 ScanCalls;
    uint32 AllocateCalls;
    uint32 AlignmentRejects;
} UDF_METADATA_GROWTH_PROFILE;

/* Search free runs once instead of allocating and rolling back each
   overlapping alignment window on a large physical partition.  Keep the
   bitmap lock across discovery and reservation so the selected run cannot
   be taken by another allocation. */
static NTSTATUS
UDFAllocAlignedMetadataRun(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN uint32 Blocks,
    IN uint32 PhysStart,
    IN uint32 PhysEnd,
    IN uint32 Align,
    OUT PEXTENT_INFO ExtInfo,
    OUT uint32* AllocStart,
    IN OUT UDF_METADATA_GROWTH_PROFILE* Profile
    )
{
    uint32 searchStart, runStart, runLength, candidate, remainder;
    NTSTATUS status = STATUS_DISK_FULL;
    LARGE_INTEGER phaseStart;

    ExtInfo->Mapping = NULL;
    ExtInfo->Length = 0;
    ExtInfo->Offset = 0;
    if (PhysEnd <= PhysStart || Blocks > PhysEnd - PhysStart)
        return status;

    UDFAcquireResourceExclusive(&(Vcb->BitMapResource1), TRUE);
    searchStart = PhysStart;
    while (searchStart <= PhysEnd - Blocks) {
        phaseStart = KeQueryPerformanceCounter(NULL);
        runStart = (uint32)UDFFindMinSuitableExtent(Vcb, Blocks,
                      searchStart, PhysEnd, &runLength,
                      EXTENT_FLAG_IGNORE_HINT);
        Profile->ScanTicks += KeQueryPerformanceCounter(NULL).QuadPart -
                              phaseStart.QuadPart;
        Profile->ScanCalls++;
        if (runLength < Blocks)
            break;
        if (runStart < searchStart || runStart >= PhysEnd ||
            runLength > PhysEnd - runStart) {
            status = STATUS_FILE_CORRUPT_ERROR;
            break;
        }

        candidate = runStart;
        remainder = (candidate - PhysStart) % Align;
        if (remainder && Align - remainder > runLength - Blocks) {
            Profile->AlignmentRejects++;
            searchStart = runStart + runLength;
            continue;
        }
        if (remainder)
            candidate += Align - remainder;
        phaseStart = KeQueryPerformanceCounter(NULL);
        status = UDFAllocFreeExtent(IrpContext, Vcb,
                     ((int64)Blocks) << Vcb->SectorShift,
                     candidate, candidate + Blocks, ExtInfo,
                     EXTENT_FLAG_VERIFY);
        Profile->AllocateTicks += KeQueryPerformanceCounter(NULL).QuadPart -
                                  phaseStart.QuadPart;
        Profile->AllocateCalls++;
        if (NT_SUCCESS(status)) {
            *AllocStart = candidate;
            break;
        }
        if (ExtInfo->Mapping) {
            UDFMarkSpaceAsXXX(Vcb, 0, ExtInfo->Mapping, AS_DISCARDED);
            MyFreePool__(ExtInfo->Mapping);
            ExtInfo->Mapping = NULL;
        }
        if (status != STATUS_DISK_FULL)
            break;
        searchStart = runStart + runLength;
    }
    UDFReleaseResource(&(Vcb->BitMapResource1));
    return status;
}

static NTSTATUS
UDFGrowMetadataFile(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN uint32 RequestedBlocks
    )
{
    uint32 unit, align, grow, preferredGrow, minGrow, maxGrow, nextGrow, fallbacks = 0;
    uint32 oldBlocks, newBlocks, oldMapBytes;
    uint32 newBitmapBytes, physStart, physEnd, candidate, mirrorCandidate, i;
    uint32 mainMapBytes, mirrorMapBytes;
    BOOLEAN duplicate;
    PCHAR newBitmap = NULL;
    PCHAR newPendingAlloc = NULL;
    int8* zeroBuffer = NULL;
    PEXTENT_MAP mainMap = NULL, mirrorMap = NULL;
    PEXTENT_MAP oldMainMap, oldMirrorMap;
    EXTENT_INFO physical, physicalMirror;
    SIZE_T written;
    NTSTATUS status = STATUS_DISK_FULL;
    UDF_METADATA_GROWTH_PROFILE profile;
    LARGE_INTEGER frequency, growthStart, phaseStart;
    BOOLEAN profileStarted = FALSE, commitStarted = FALSE;

    if (!Vcb->MetadataFileInfo || !Vcb->MetadataMirrorFileInfo ||
        !Vcb->MetadataBitmapFileInfo || !Vcb->MetadataPendingAlloc)
        return STATUS_NOT_SUPPORTED;

    duplicate = (Vcb->MetadataFlags & UDF_METADATA_DUPLICATED) != 0;
    oldBlocks = Vcb->MetadataBitCount;
    if (oldBlocks != Vcb->Partitions[Vcb->MetadataRef].PartitionLen ||
        oldBlocks != (uint32)(UDFGetFileSize(Vcb->MetadataFileInfo) >> Vcb->SectorShift) ||
        oldBlocks != (uint32)(UDFGetFileSize(Vcb->MetadataMirrorFileInfo) >> Vcb->SectorShift))
        return STATUS_FILE_CORRUPT_ERROR;

    oldMainMap = Vcb->MetadataFileInfo->Dloc->DataLoc.Mapping;
    oldMirrorMap = Vcb->MetadataMirrorFileInfo->Dloc->DataLoc.Mapping;
    mainMapBytes = UDFGetMappingLength(oldMainMap);
    mirrorMapBytes = UDFGetMappingLength(oldMirrorMap);
    if (!mainMapBytes || mirrorMapBytes != mainMapBytes)
        return STATUS_NOT_SUPPORTED;
    for (i = 0; i < mainMapBytes / sizeof(EXTENT_MAP); i++) {
        if (oldMainMap[i].extLength != oldMirrorMap[i].extLength ||
            (duplicate && oldMainMap[i].extLength &&
             oldMainMap[i].extLocation == oldMirrorMap[i].extLocation) ||
            (!duplicate &&
             oldMainMap[i].extLocation != oldMirrorMap[i].extLocation))
            return STATUS_NOT_SUPPORTED;
    }

    unit = Vcb->MetadataAllocUnit ? Vcb->MetadataAllocUnit : 1;
    align = Vcb->MetadataAlignUnit ? Vcb->MetadataAlignUnit : 1;
    // A fixed 512-block increment exhausts the Metadata File Entry's short
    // AD space on large directories long before the physical partition fills.
    // Prefer geometric growth to bound AD count, but a fragmented disk may
    // have only smaller aligned runs. Fall back before reporting ENOSPC.
    if (!RequestedBlocks)
        return STATUS_INVALID_PARAMETER;
    grow = max(RequestedBlocks, max((uint32)512, oldBlocks / 2));
    if (grow > MAXULONG - (unit - 1))
        return STATUS_DISK_FULL;
    minGrow = ((RequestedBlocks + unit - 1) / unit) * unit;
    grow = ((grow + unit - 1) / unit) * unit;
    /* A metadata-file AD stores the byte length in 30 bits. Keep each
     * geometric increment representable as one allocation-unit-aligned AD;
     * the physical allocator cannot safely split this particular mapping. */
    maxGrow = (UDF_EXTENT_LENGTH_MASK >> Vcb->SectorShift);
    maxGrow -= maxGrow % unit;
    if (!maxGrow || minGrow > maxGrow)
        return STATUS_NOT_SUPPORTED;
    if (grow > maxGrow)
        grow = maxGrow;
    preferredGrow = grow;
    if (oldBlocks > MAXULONG - grow)
        return STATUS_DISK_FULL;
    newBlocks = oldBlocks + grow;
    newBitmapBytes = (newBlocks >> 3) + ((newBlocks & 7) != 0);
    newBitmapBytes = max(newBitmapBytes, Vcb->MetadataBitmapByteCount);

    newBitmap = (PCHAR)DbgAllocatePool(NonPagedPool, newBitmapBytes);
    newPendingAlloc = (PCHAR)DbgAllocatePool(NonPagedPool, newBitmapBytes);
    zeroBuffer = (int8*)DbgAllocatePool(NonPagedPool, Vcb->SectorSize * 8);
    mainMap = (PEXTENT_MAP)MyAllocatePoolTag__(NonPagedPool,
                mainMapBytes + sizeof(EXTENT_MAP), MEM_EXTMAP_TAG);
    mirrorMap = (PEXTENT_MAP)MyAllocatePoolTag__(NonPagedPool,
                mirrorMapBytes + sizeof(EXTENT_MAP), MEM_EXTMAP_TAG);
    if (!newBitmap || !newPendingAlloc || !zeroBuffer || !mainMap || !mirrorMap) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }
    RtlFillMemory(newBitmap, newBitmapBytes, 0xff);
    RtlZeroMemory(newPendingAlloc, newBitmapBytes);
    RtlCopyMemory(newBitmap, Vcb->MetadataBitmap, Vcb->MetadataBitmapByteCount);
    for (i = oldBlocks; i < newBlocks; i++)
        newBitmap[i >> 3] |= (uint8)(1 << (i & 7));
    RtlZeroMemory(zeroBuffer, Vcb->SectorSize * 8);

    physical.Mapping = NULL;
    physicalMirror.Mapping = NULL;
    candidate = mirrorCandidate = 0;
    RtlZeroMemory(&profile, sizeof(profile));
    growthStart = KeQueryPerformanceCounter(&frequency);
    profileStarted = TRUE;
    physStart = UDFPartStart(Vcb, Vcb->MetadataPhysRef);
    physEnd = UDFPartEnd(Vcb, Vcb->MetadataPhysRef);
    for (;;) {
        status = UDFAllocAlignedMetadataRun(IrpContext, Vcb, grow,
                     physStart, physEnd, align, &physical, &candidate, &profile);
        if (!NT_SUCCESS(status)) {
            if (physical.Mapping) {
                status = STATUS_FILE_CORRUPT_ERROR;
                goto release_physical;
            }
            if (status == STATUS_DISK_FULL)
                goto reduce_growth;
            goto cleanup;
        }
        if (!physical.Mapping) {
            status = STATUS_DISK_FULL;
            goto reduce_growth;
        }
        if (physical.Mapping[0].extLocation != candidate ||
            physical.Mapping[1].extLength) {
            status = STATUS_FILE_CORRUPT_ERROR;
            goto release_physical;
        }

        // Duplicated metadata requires a second, disjoint physical allocation.
        // Its recorded AD has the same logical length as the primary AD.
        mirrorCandidate = candidate;
        if (duplicate) {
            status = UDFAllocAlignedMetadataRun(IrpContext, Vcb, grow,
                         physStart, physEnd, align, &physicalMirror,
                         &mirrorCandidate, &profile);
            if (!NT_SUCCESS(status)) {
                if (status == STATUS_DISK_FULL)
                    goto release_and_reduce_growth;
                goto release_physical;
            }
            if (!physicalMirror.Mapping) {
                status = STATUS_DISK_FULL;
                goto release_and_reduce_growth;
            }
            if (physicalMirror.Mapping[0].extLocation != mirrorCandidate ||
                physicalMirror.Mapping[1].extLength ||
                (mirrorCandidate < candidate + grow &&
                 candidate < mirrorCandidate + grow)) {
                status = STATUS_FILE_CORRUPT_ERROR;
                goto release_physical;
            }
        }
        break;

release_and_reduce_growth:
        if (physicalMirror.Mapping) {
            UDFMarkSpaceAsXXX(Vcb, 0, physicalMirror.Mapping, AS_DISCARDED);
            MyFreePool__(physicalMirror.Mapping);
            physicalMirror.Mapping = NULL;
        }
        UDFMarkSpaceAsXXX(Vcb, 0, physical.Mapping, AS_DISCARDED);
        MyFreePool__(physical.Mapping);
        physical.Mapping = NULL;
reduce_growth:
        if (grow <= minGrow)
            goto cleanup;
        nextGrow = max(minGrow, (grow / 2 / unit) * unit);
        if (nextGrow >= grow) {
            status = STATUS_DISK_FULL;
            goto cleanup;
        }
        grow = nextGrow;
        fallbacks++;
    }
    newBlocks = oldBlocks + grow;
    newBitmapBytes = max((newBlocks >> 3) + ((newBlocks & 7) != 0),
                         Vcb->MetadataBitmapByteCount);

    // Metadata File ADs must describe aligned, recorded allocation units.
    phaseStart = KeQueryPerformanceCounter(NULL);
    for (i = 0; i < grow; i += 8) {
        uint32 chunk = min((uint32)8, grow - i);
        status = UDFWriteSectors(IrpContext, Vcb, TRUE, candidate + i,
                                 chunk, FALSE, zeroBuffer, &written);
        if (!NT_SUCCESS(status) || written != ((SIZE_T)chunk << Vcb->SectorShift)) {
            if (NT_SUCCESS(status)) status = STATUS_IO_DEVICE_ERROR;
            profile.ZeroTicks += KeQueryPerformanceCounter(NULL).QuadPart -
                                 phaseStart.QuadPart;
            goto release_physical;
        }
        if (duplicate) {
            status = UDFWriteSectors(IrpContext, Vcb, TRUE, mirrorCandidate + i,
                                     chunk, FALSE, zeroBuffer, &written);
            if (!NT_SUCCESS(status) || written != ((SIZE_T)chunk << Vcb->SectorShift)) {
                if (NT_SUCCESS(status)) status = STATUS_IO_DEVICE_ERROR;
                profile.ZeroTicks += KeQueryPerformanceCounter(NULL).QuadPart -
                                     phaseStart.QuadPart;
                goto release_physical;
            }
        }
    }
    profile.ZeroTicks += KeQueryPerformanceCounter(NULL).QuadPart -
                         phaseStart.QuadPart;

    RtlCopyMemory(mainMap, oldMainMap, mainMapBytes - sizeof(EXTENT_MAP));
    RtlCopyMemory(mirrorMap, oldMirrorMap, mirrorMapBytes - sizeof(EXTENT_MAP));
    oldMapBytes = mainMapBytes - sizeof(EXTENT_MAP);
    mainMap[oldMapBytes / sizeof(EXTENT_MAP)].extLocation = candidate;
    mainMap[oldMapBytes / sizeof(EXTENT_MAP)].extLength = grow << Vcb->SectorShift;
    mirrorMap[oldMapBytes / sizeof(EXTENT_MAP)] =
        mainMap[oldMapBytes / sizeof(EXTENT_MAP)];
    mirrorMap[oldMapBytes / sizeof(EXTENT_MAP)].extLocation = mirrorCandidate;
    RtlZeroMemory((int8*)mainMap + mainMapBytes, sizeof(EXTENT_MAP));
    RtlZeroMemory((int8*)mirrorMap + mirrorMapBytes, sizeof(EXTENT_MAP));

    Vcb->MetadataFileInfo->Dloc->DataLoc.Mapping = mainMap;
    Vcb->MetadataMirrorFileInfo->Dloc->DataLoc.Mapping = mirrorMap;
    Vcb->MetadataFileInfo->Dloc->DataLoc.Length =
        Vcb->MetadataMirrorFileInfo->Dloc->DataLoc.Length =
        ((int64)newBlocks) << Vcb->SectorShift;
    Vcb->MetadataFileInfo->Dloc->DataLoc.Modified = TRUE;
    Vcb->MetadataFileInfo->Dloc->AllocLoc.Modified = TRUE;
    Vcb->MetadataMirrorFileInfo->Dloc->DataLoc.Modified = TRUE;
    Vcb->MetadataMirrorFileInfo->Dloc->AllocLoc.Modified = TRUE;
    UDFSetFileSize(Vcb->MetadataFileInfo, ((int64)newBlocks) << Vcb->SectorShift);
    UDFSetFileSize(Vcb->MetadataMirrorFileInfo, ((int64)newBlocks) << Vcb->SectorShift);
    MyFreePool__(oldMainMap);
    MyFreePool__(oldMirrorMap);
    mainMap = mirrorMap = NULL;
    MyFreePool__(physical.Mapping);
    physical.Mapping = NULL;
    if (physicalMirror.Mapping) {
        MyFreePool__(physicalMirror.Mapping);
        physicalMirror.Mapping = NULL;
    }

    phaseStart = KeQueryPerformanceCounter(NULL);
    commitStarted = TRUE;
    status = UDFFlushFile__(IrpContext, Vcb, Vcb->MetadataFileInfo, 0);
    if (!NT_SUCCESS(status)) goto failed_after_commit;
    status = UDFFlushFile__(IrpContext, Vcb, Vcb->MetadataMirrorFileInfo, 0);
    if (!NT_SUCCESS(status)) goto failed_after_commit;
    status = UDFResizeFile__(IrpContext, Vcb, Vcb->MetadataBitmapFileInfo,
                             sizeof(SPACE_BITMAP_DESC) + newBitmapBytes);
    if (!NT_SUCCESS(status)) goto failed_after_commit;

    /* Preserve allocations made while the new metadata extents were zeroed
     * and committed. Both the active and pending bitmaps share this lock. */
    UDFAcquireResourceExclusive(&(Vcb->BitMapResource1), TRUE);
    if (Vcb->MetadataBitCount != oldBlocks ||
        !Vcb->MetadataBitmap || !Vcb->MetadataPendingAlloc ||
        Vcb->MetadataBitmapByteCount > newBitmapBytes) {
        UDFReleaseResource(&(Vcb->BitMapResource1));
        status = STATUS_FILE_CORRUPT_ERROR;
        goto failed_after_commit;
    }
    RtlCopyMemory(newBitmap, Vcb->MetadataBitmap,
                  Vcb->MetadataBitmapByteCount);
    if (oldBlocks & 7)
        newBitmap[oldBlocks >> 3] |= (uint8)(0xffU << (oldBlocks & 7));
    RtlCopyMemory(newPendingAlloc, Vcb->MetadataPendingAlloc,
                  Vcb->MetadataBitmapByteCount);
    DbgFreePool(Vcb->MetadataPendingAlloc);
    Vcb->MetadataPendingAlloc = newPendingAlloc;
    newPendingAlloc = NULL;
    DbgFreePool(Vcb->MetadataBitmap);
    Vcb->MetadataBitmap = newBitmap;
    newBitmap = NULL;
    Vcb->MetadataBitmapByteCount = newBitmapBytes;
    Vcb->MetadataBitCount = newBlocks;
    Vcb->MetadataFreeCount += grow;
    Vcb->MetadataHint = oldBlocks;
    Vcb->Partitions[Vcb->MetadataRef].PartitionLen = newBlocks;
    Vcb->MetadataBitmapModified = TRUE;
    UDFSetModified(Vcb);
    UDFReleaseResource(&(Vcb->BitMapResource1));
    status = UDFFlushMetadataBitmap(IrpContext, Vcb);
    if (!NT_SUCCESS(status)) goto failed_after_commit;
    goto cleanup;

release_physical:
    if (physicalMirror.Mapping) {
        UDFMarkSpaceAsXXX(Vcb, 0, physicalMirror.Mapping, AS_DISCARDED);
        MyFreePool__(physicalMirror.Mapping);
    }
    UDFMarkSpaceAsXXX(Vcb, 0, physical.Mapping, AS_DISCARDED);
    MyFreePool__(physical.Mapping);
    /* Non-ENOSPC and post-reservation failures still fail closed. */
    Vcb->VcbState |= VCB_STATE_VOLUME_READ_ONLY;
    goto cleanup;
failed_after_commit:
    if (!NT_SUCCESS(status))
        Vcb->VcbState |= VCB_STATE_VOLUME_READ_ONLY;
cleanup:
    if (profileStarted) {
        LARGE_INTEGER end = KeQueryPerformanceCounter(NULL);
        if (commitStarted)
            profile.CommitTicks = end.QuadPart - phaseStart.QuadPart;
        /* One bounded line per metadata growth. Ticks and frequency avoid
         * discarding sub-millisecond scan and allocation times. */
        UDFPrint(("UDF metadata growth req=%x preferred=%x grow=%x fallback=%x status=%x main=%x mirror=%x scans=%x allocs=%x align_reject=%x ticks(scan/alloc/zero/commit/total)=%I64x/%I64x/%I64x/%I64x/%I64x freq=%I64x\n",
                  RequestedBlocks, preferredGrow, grow, fallbacks, status,
                  candidate, mirrorCandidate,
                  profile.ScanCalls, profile.AllocateCalls,
                  profile.AlignmentRejects, profile.ScanTicks,
                  profile.AllocateTicks, profile.ZeroTicks,
                  profile.CommitTicks, end.QuadPart - growthStart.QuadPart,
                  frequency.QuadPart));
    }
    if (newBitmap) DbgFreePool(newBitmap);
    if (newPendingAlloc) DbgFreePool(newPendingAlloc);
    if (zeroBuffer) DbgFreePool(zeroBuffer);
    if (mainMap) MyFreePool__(mainMap);
    if (mirrorMap) MyFreePool__(mirrorMap);
    return status;
}

NTSTATUS
UDFAllocMetadataExtent(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN int64 Length,
    OUT PEXTENT_INFO ExtInfo
    )
{
    uint32 blocks, bit, i, phys, pass, first, seenBlocks;
    BOOLEAN growthAttempted = FALSE;
    EXTENT_AD extent;

    ExtInfo->Mapping = NULL;
    ExtInfo->Offset = 0;
    ExtInfo->Length = 0;

    if (!Vcb->MetadataFileInfo || !Vcb->MetadataBitmap ||
        !Vcb->MetadataPendingAlloc)
        return STATUS_NOT_SUPPORTED;
    if (Length <= 0 || Length > UDF_EXTENT_LENGTH_MASK)
        return STATUS_INVALID_PARAMETER;

    blocks = (uint32)((Length + Vcb->SectorSize - 1) >> Vcb->SectorShift);
retry_after_growth:
    UDFAcquireResourceExclusive(&(Vcb->BitMapResource1), TRUE);
    first = Vcb->MetadataHint < Vcb->MetadataBitCount ? Vcb->MetadataHint : 0;
    for (pass = 0; pass < 2 && blocks <= Vcb->MetadataBitCount; pass++) {
      for (bit = pass ? 0 : first;
           bit <= Vcb->MetadataBitCount - blocks && (pass == 0 || bit < first);
           bit++) {
        phys = UDFMetadataLbaToPhys(Vcb, bit, NULL);
        if (phys == LBA_OUT_OF_EXTENT)
            continue;
        for (i = 0; i < blocks; i++) {
            if (!(Vcb->MetadataBitmap[(bit + i) >> 3] &
                  (1 << ((bit + i) & 7))) ||
                UDFMetadataLbaToPhys(Vcb, bit + i, NULL) != phys + i)
                break;
        }
        if (i != blocks)
            continue;

        extent.extLocation = phys;
        extent.extLength = (blocks << Vcb->SectorShift) |
                           (EXTENT_NOT_RECORDED_ALLOCATED << 30);
        ExtInfo->Mapping = UDFExtentToMapping(&extent);
        if (!ExtInfo->Mapping) {
            UDFReleaseResource(&(Vcb->BitMapResource1));
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        for (i = 0; i < blocks; i++) {
            Vcb->MetadataBitmap[(bit + i) >> 3] &=
                ~(1 << ((bit + i) & 7));
            Vcb->MetadataPendingAlloc[(bit + i) >> 3] |=
                (uint8)(1 << ((bit + i) & 7));
        }
        Vcb->MetadataFreeCount -= blocks;
        Vcb->MetadataHint = bit + blocks;
        if (Vcb->MetadataHint >= Vcb->MetadataBitCount)
            Vcb->MetadataHint = 0;
        Vcb->MetadataBitmapModified = TRUE;
        UDFSetModified(Vcb);
        ExtInfo->Length = Length;
        UDFReleaseResource(&(Vcb->BitMapResource1));
        return STATUS_SUCCESS;
      }
    }

    seenBlocks = Vcb->MetadataBitCount;
    UDFReleaseResource(&(Vcb->BitMapResource1));
    if (!growthAttempted) {
        NTSTATUS status;
        /* Two creates can run out of metadata space at once. Growth swaps
         * and frees the metadata file's extent maps, so only one thread may
         * grow; a thread that waited retries with what the other added.
         * A caller already owning the bitmap lock must not wait here, as
         * the growing thread needs that lock to finish. */
        if (!UDFAcquireResourceExclusive(&(Vcb->MetadataGrowResource),
                !ExIsResourceAcquiredExclusiveLite(&(Vcb->BitMapResource1))))
            return STATUS_DISK_FULL;
        if (Vcb->MetadataBitCount != seenBlocks) {
            UDFReleaseResource(&(Vcb->MetadataGrowResource));
            goto retry_after_growth;
        }
        status = UDFGrowMetadataFile(IrpContext, Vcb, blocks);
        UDFReleaseResource(&(Vcb->MetadataGrowResource));
        if (NT_SUCCESS(status)) {
            growthAttempted = TRUE;
            goto retry_after_growth;
        }
        return status;
    }
    return STATUS_DISK_FULL;
}

/*
    This routine returns physycal Lba for partition-relative addr
    No partition bounds check is performed.
    This routine only checks if requested partition exists.
    It is introduced for 'Adaptec DirectCD' compatibility,
    because it uses negative values as extent terminator (against standard)
 */
/*uint32
__fastcall
UDFPartLbaToPhysCompat(
  IN PVCB Vcb,
  IN lb_addr* Addr
  )
{
    uint32 i, a;
    if (Addr->partitionReferenceNum >= Vcb->PartitionMaps) return LBA_NOT_ALLOCATED;
    // walk through partition maps & transform relative address
    // to physical
    for(i=Addr->partitionReferenceNum; i<Vcb->PartitionMaps; i++) {
        if (Vcb->Partitions[i].PartitionNum == Addr->partitionReferenceNum) {
            a = Vcb->Partitions[i].PartitionRoot + Addr->logicalBlockNum;
            if (a > Vcb->LastPossibleLBA) {
                BrutePoint();
            }
            return a;
        }
    }
    a = Vcb->Partitions[i-1].PartitionRoot + Addr->logicalBlockNum;
    if (a > Vcb->LastPossibleLBA) {
        BrutePoint();
    }
    return a;
} // end UDFPartLbaToPhysCompat()*/


/*
    This routine looks for the partition containing given physical sector
 */
uint32
__fastcall
UDFGetRefPartNumByPhysLba(
    IN PVCB Vcb,
    IN uint32 Lba
    )
{
    uint32 i, root;
    PUDFPartMap pm = Vcb->Partitions;

    if (
        Vcb->MetadataFileInfo &&
        (UDFPhysLbaToMetadata(Vcb, Lba) != LBA_OUT_OF_EXTENT)
    )
        return Vcb->MetadataRef;

    for (i = 0; i < Vcb->PartitionMaps; i++, pm++) {

        if ((pm->PartitionType != UDF_SPARABLE_MAP15) &&
            (pm->PartitionType != UDF_TYPE1_MAP15)
        ) {
            continue;
        }

        if( ((root = pm->PartitionRoot) <= Lba) &&
             ((root + pm->PartitionLen) > Lba)
        ) {
            return i;
        }
    }
    return LBA_OUT_OF_EXTENT; // Lba doesn't belong to any partition
} // end UDFGetPartNumByPhysLba()

/*
    Very simple routine. It walks through the Partition Maps & returns
    the 1st Lba of the 1st suitable one
 */
uint32
__fastcall
UDFPartStart(
    PVCB Vcb,
    uint32 RefPartNum
    )
{
    if (RefPartNum == (uint32)-1) 
        return 0;

    if (RefPartNum == (uint32)-2) 
        RefPartNum = UDFGetLastPhysPartRef(Vcb);
    
    if(RefPartNum >= Vcb->PartitionMaps)
        return 0;

    return Vcb->Partitions[RefPartNum].PartitionRoot;
} // end UDFPartStart(

/*
   This routine does almost the same as previous.
   The only difference is changing First Lba to Last one...
 */
uint32
__fastcall
UDFPartEnd(
    PVCB Vcb,
    uint32 RefPartNum
    )
{
    if (RefPartNum == (uint32)-1)
        return Vcb->SessionEndLba;

    if (RefPartNum == (uint32)-2)
        RefPartNum = UDFGetLastPhysPartRef(Vcb);
    
    if(RefPartNum >= Vcb->PartitionMaps)
        RefPartNum = Vcb->PartitionMaps-1;

    return (
        Vcb->Partitions[RefPartNum].PartitionRoot +
        Vcb->Partitions[RefPartNum].PartitionLen
    );
} // end UDFPartEnd()

/*
    Very simple routine. It walks through the Partition Maps & returns
    the 1st Lba of the 1st suitable one
 */
uint32
__fastcall
UDFPartLen(
    PVCB Vcb, 
    uint32 RefPartNum
    )
{
    if (RefPartNum == (uint32)-1) 
        return Vcb->SessionEndLba;

    if (RefPartNum == (uint32)-2)
        RefPartNum = UDFGetLastPhysPartRef(Vcb);

    if(RefPartNum >= Vcb->PartitionMaps)
        RefPartNum = Vcb->PartitionMaps-1;
 
    return Vcb->Partitions[RefPartNum].PartitionLen;
} // end UDFPartLen()

uint32
__fastcall
UDFGetLastPhysPartRef(
    PVCB Vcb
)
{
    uint32 m;

    for (m = Vcb->PartitionMaps; m > 0; m--) {

        if ((Vcb->Partitions[m-1].PartitionType == UDF_TYPE1_MAP15) ||
            (Vcb->Partitions[m-1].PartitionType == UDF_SPARABLE_MAP15)
        ) {
            return m-1;
        }
    }

    return 0;
}

/*
    This routine returns length of bit-chain starting from Offs bit in
    array Bitmap. Bitmap scan is limited with Lim.
 */
SIZE_T
UDFGetBitmapLen(
    uint32* Bitmap,
    SIZE_T Offs,
    SIZE_T Lim          // NOT included
    )
{
    ASSERT(Offs <= Lim);
    if (Offs >= Lim) {
        return 0;//(Offs == Lim);
    }

    BOOLEAN bit = UDFGetBit(Bitmap, Offs);
    SIZE_T i=Offs>>5;
    SIZE_T len=0;
    uint8 j=(uint8)(Offs&31);
    uint8 lLim=(uint8)(Lim&31);

    Lim = Lim>>5;

    ASSERT((bit == 0) || (bit == 1));

    uint32 a;

    a = Bitmap[i] >> j;

    while(i<=Lim) {

        while( j < ((i<Lim) ? 32 : lLim) ) {
            if ( ((BOOLEAN)(a&1)) != bit)
                return len;
            len++;
            a>>=1;
            j++;
        }
        j=0;
While_3:
        i++;
        if (i > Lim) break;
        a = Bitmap[i];

        if (i<Lim) {
            if ((bit && (a==0xffffffff)) ||
               (!bit && !a)) {
                len+=32;
                goto While_3;
            }
        }
    }
    return len;
} // end UDFGetBitmapLen()

/*
 */
SIZE_T
UDFFindMinSuitableExtent(
    IN PVCB Vcb,
    IN uint32 LengthInBlocks,
    IN uint32 SearchStartPSN,  
    IN uint32 ExclusiveSearchLimPSN,    
    OUT uint32* MaxExtLen,
    IN uint8  AllocFlags
)
{
    SIZE_T i, len;
    ULONG pass;
    SIZE_T best_lba=0, best_len=0;
    SIZE_T max_lba=0, max_len=0;
    uint32 partRoot = Vcb->Partitions[0].PartitionRoot, lbnStart = SearchStartPSN - partRoot;
    uint32 lbnLim = ExclusiveSearchLimPSN - partRoot;
    uint32 hint = (AllocFlags & EXTENT_FLAG_FE_LOCALITY) ?
                  Vcb->FeSectorHint : Vcb->SectorHint;

    UDF_CHECK_BITMAP_RESOURCE(Vcb);

    if (LengthInBlocks > (uint32)(UDF_EXTENT_LENGTH_MASK >> Vcb->SectorShift)) {
        LengthInBlocks = (UDF_EXTENT_LENGTH_MASK >> Vcb->SectorShift);
    }

#define _bitmap_freebit(off) (Vcb->BitmapFcb ? UDFIsBitmapBitFree(Vcb, hint - off) : UDFGetFreeBit((uint32*)Vcb->FSBM_Bitmap, hint - off))
    /* The metadata-growth caller must traverse physical runs in increasing
     * order. Wrapping from SectorHint can select an unalignable later run,
     * then skip an earlier suitable run when it advances SearchStart. */
    if ((AllocFlags & EXTENT_FLAG_IGNORE_HINT) ||
        hint < lbnStart || hint >= lbnLim) {
        hint = lbnStart;
    } else if (
        hint > lbnStart && _bitmap_freebit(0)) {
        while ( hint > lbnStart && _bitmap_freebit(1)) {
            hint--;
        }
    }
#undef _bitmap_freebit

    for (pass = 0; pass < 2; pass++) {
        uint32 scanStart = pass ? lbnStart : hint;
        uint32 scanLim = pass ? hint : lbnLim;

        if (scanStart >= scanLim)
            continue;

        i = scanStart;
        if (Vcb->BitmapFcb) {
            ULONG CurrentRunStart = 0, CurrentRunLength = 0;
            ULONG CurrentLbn = (ULONG)scanStart;
        
            while (CurrentLbn < scanLim) {
                UDFPinBitmapPage(Vcb, CurrentLbn);
            
                ULONG runStartIndex;
                ULONG pageStart = Vcb->BitmapPageStartLbn;
                ULONG pageBits = Vcb->BitmapPageBitCount;
                ULONG fromIndex = CurrentLbn - pageStart;
                ULONG runLen = UDFBitmapFindNextRunSet(&Vcb->BitmapRtl, fromIndex, &runStartIndex);
            
                ULONG runStartLbn = runStartIndex + pageStart;
                ULONG pageEnd = min(pageStart + pageBits, scanLim);

                if (runStartLbn >= scanLim)
                    runLen = 0;
                else if (runLen > pageEnd - runStartLbn)
                    runLen = pageEnd - runStartLbn;
            
                if (CurrentRunLength != 0) {
                    if (runLen == 0 || runStartLbn != CurrentLbn) {
                        if (CurrentRunLength >= LengthInBlocks) {
                            if (!best_len || (best_len > CurrentRunLength)) {
                                best_lba = CurrentRunStart;
                                best_len = CurrentRunLength;
                            }
                        } else if(max_len < CurrentRunLength) {
                            max_lba = CurrentRunStart;
                            max_len = CurrentRunLength;
                        }
                        if (best_len || (Vcb->CDR_Mode && max_len)) break;
                        CurrentRunLength = runLen;
                        CurrentRunStart = runStartLbn;
                    } else {
                        CurrentRunLength += runLen;
                    }
                } else {
                     if(runLen != 0) {
                        CurrentRunLength = runLen;
                        CurrentRunStart = runStartLbn;
                    }
                }
            
                if (CurrentRunLength >= LengthInBlocks) {
                    best_len = CurrentRunLength;
                    best_lba = CurrentRunStart;
                    break;
                }
            
                if (best_len) break;
            
                if (runLen == 0)
                    CurrentLbn = pageEnd;
                else
                    CurrentLbn = CurrentRunStart + CurrentRunLength;
            
                if (CurrentLbn > scanLim)
                    CurrentLbn = scanLim;
            }
        
            if (CurrentRunLength != 0 && !best_len) {
                if (CurrentRunLength >= LengthInBlocks) {
                    if (!best_len || (best_len > CurrentRunLength)) {
                        best_lba = CurrentRunStart;
                        best_len = CurrentRunLength;
                    }
                } else if (max_len < CurrentRunLength) {
                    max_lba = CurrentRunStart;
                    max_len = CurrentRunLength;
                }
            }
        } else {
            while (i < scanLim) {

                ASSERT(i <= scanLim);
                
                len = UDFGetBitmapLen((uint32*)(Vcb->FSBM_Bitmap), i, scanLim);
                
                if (UDFGetFreeBit((uint32*)(Vcb->FSBM_Bitmap), i)) {
                    if (len >= LengthInBlocks) {
                        if (!best_len || (best_len > len)) {
                            best_lba = i;
                            best_len = len;
                        }
                        if(len == LengthInBlocks) break;
                    } else {
                        if (max_len < len) {
                            max_lba = i;
                            max_len = len;
                        }
                    }
                    if (best_len || (Vcb->CDR_Mode && max_len)) break;
                }
                i += len;
            }
        }
        if (best_len || (Vcb->CDR_Mode && max_len))
            break;
    }
    
    UDFUnpinBitmapPage(Vcb);
    
    if (!best_len && !max_len) {
        UDFPrint(("UDFFindMinSuitableExtent: Couldn't find free space lbnStart=%x lbnLim=%x Length=%x BitCount=%x\n",
            (ULONG)lbnStart, (ULONG)lbnLim, LengthInBlocks, Vcb->FSBM_BitCount));
    }

    if (best_len) {
        *MaxExtLen = best_len;
        return best_lba + partRoot;
    }

    *MaxExtLen = max_len;

    return max_lba + partRoot;
}

#ifdef UDF_CHECK_DISK_ALLOCATION
/*
    This routine checks space described by Mapping as Used/Freed (optionaly)
 */
void
UDFCheckSpaceAllocation_(
    IN PVCB Vcb,
    IN PEXTENT_MAP Map,
    IN uint32 asXXX
#ifdef UDF_TRACK_ONDISK_ALLOCATION
   ,IN uint32 FE_lba,
    IN uint32 BugCheckId,
    IN uint32 Line
#endif //UDF_TRACK_ONDISK_ALLOCATION
    )
{
    uint32 i=0;
    uint32 lba, j, len, BS, BSh;
    BOOLEAN asUsed = (asXXX == AS_USED);

    if (!Map) return;

    BS = Vcb->BlockSize;
    BSh = Vcb->BlockSizeBits;

    UDFAcquireResourceExclusive(&(Vcb->BitMapResource1),TRUE);
    // walk through all frags in data area specified
#ifdef UDF_TRACK_ONDISK_ALLOCATION
    AdPrint(("ChkAlloc:Map:%x:File:%x:Line:%d\n",
        Map,
        BugCheckId,
        Line
        ));
#endif //UDF_TRACK_ONDISK_ALLOCATION
    while(Map[i].extLength & UDF_EXTENT_LENGTH_MASK) {

#ifdef UDF_TRACK_ONDISK_ALLOCATION
        AdPrint(("ChkAlloc:%x:%s:%x:@:%x:(%x):File:%x:Line:%d\n",
            FE_lba,
            asUsed ? "U" : "F",
            (Map[i].extLength & UDF_EXTENT_LENGTH_MASK) >> BSh,
            Map[i].extLocation,
            (Map[i].extLength >> 30),
            BugCheckId,
            Line
            ));
#endif //UDF_TRACK_ONDISK_ALLOCATION
        if (asUsed) {
            UDFCheckUsedBitOwner(Vcb, (Map[i].extLength & UDF_EXTENT_LENGTH_MASK) >> BSh, FE_lba);
        } else {
            UDFCheckFreeBitOwner(Vcb, (Map[i].extLength & UDF_EXTENT_LENGTH_MASK) >> BSh);
        }

        if ((Map[i].extLength >> 30) == EXTENT_NOT_RECORDED_NOT_ALLOCATED) {
            // skip unallocated frags
//            ASSERT(!(Map[i].extLength & UDF_EXTENT_LENGTH_MASK));
            ASSERT(!Map[i].extLocation);
            i++;
            continue;
        } else {
//            ASSERT(!(Map[i].extLength & UDF_EXTENT_LENGTH_MASK));
            ASSERT(Map[i].extLocation);
        }

#ifdef UDF_CHECK_EXTENT_SIZE_ALIGNMENT
        ASSERT(!(Map[i].extLength & (BS-1)));
#endif //UDF_CHECK_EXTENT_SIZE_ALIGNMENT
        len = ((Map[i].extLength & UDF_EXTENT_LENGTH_MASK)+BS-1) >> BSh;
        lba = Map[i].extLocation;
        if (lba < Vcb->Partitions[0].PartitionRoot) {
            ASSERT(FALSE);
            i++;
            continue;
        }
        // The bitmap counts partition-relative blocks, not physical sectors.
        uint32 lbn = lba - Vcb->Partitions[0].PartitionRoot;
        if (lbn >= Vcb->FSBM_BitCount) {
            ASSERT(FALSE);
            i++;
            continue;
        }
        if (len > Vcb->FSBM_BitCount - lbn)
            len = Vcb->FSBM_BitCount - lbn;

        // mark frag as XXX (see asUsed parameter)
        if (asUsed) {

            ASSERT(len);
            for(j=0;j<len;j++) {
                if (lbn+j >= Vcb->FSBM_BitCount) {
                    BrutePoint();
                    AdPrint(("USED Mapping covers block(s) beyond bitmap @%x\n",lba+j));
                    break;
                }
                if (Vcb->BitmapFcb ? UDFIsBitmapBitFree(Vcb, lbn+j) : !UDFGetUsedBit(Vcb->FSBM_Bitmap, lbn+j)) {
                    BrutePoint();
                    AdPrint(("USED Mapping covers FREE block(s) @%x\n",lba+j));
                    break;
                }
            }

        } else {

            ASSERT(len);
            for(j=0;j<len;j++) {
                if (lbn+j >= Vcb->FSBM_BitCount) {
                    BrutePoint();
                    AdPrint(("USED Mapping covers block(s) beyond bitmap @%x\n",lba+j));
                    break;
                }
                if (Vcb->BitmapFcb ? !UDFIsBitmapBitFree(Vcb, lbn+j) : !UDFGetFreeBit(Vcb->FSBM_Bitmap, lbn+j)) {
                    BrutePoint();
                    AdPrint(("FREE Mapping covers USED block(s) @%x\n",lba+j));
                    break;
                }
            }
        }

        i++;
    }
    UDFReleaseResource(&(Vcb->BitMapResource1));
} // end UDFCheckSpaceAllocation_()
#endif //UDF_CHECK_DISK_ALLOCATION

void
UDFMarkBadSpaceAsUsed(
    IN PVCB Vcb,
    IN lba_t lba,
    IN ULONG len
    )
{
    uint32 j;
#define BIT_C   (sizeof(Vcb->BSBM_Bitmap[0])*8)
    len = (lba+len+BIT_C-1)/BIT_C;
    if (Vcb->BSBM_Bitmap) {
        if (Vcb->BitmapFcb) {
            // Per-page: AND bad-block mask into pinned bitmap data
            for(j=lba/BIT_C; j<len; j++) {
                if (Vcb->BSBM_Bitmap[j]) {
                    UDFPinBitmapPage(Vcb, j * BIT_C);
                    ULONG byteOff = (j * BIT_C - Vcb->BitmapPageStartLbn) / 8;
                    // Access raw pinned data for bad-block masking
                    PUCHAR rawData = (PUCHAR)Vcb->BitmapRtl.Buffer;
                    Vcb->FreeAllocUnits -= bit_count_tab[rawData[byteOff] & (UCHAR)Vcb->BSBM_Bitmap[j]];
                    rawData[byteOff] &= ~Vcb->BSBM_Bitmap[j];
                    UDFDirtyBitmapPage(Vcb);
                }
            }
        } else {
            for(j=lba/BIT_C; j<len; j++) {
                Vcb->FSBM_Bitmap[j] &= ~Vcb->BSBM_Bitmap[j];
            }
        }
    }
#undef BIT_C
    UDFUnpinBitmapPage(Vcb);
} // UDFMarkBadSpaceAsUsed()

/*
    This routine marks space described by Mapping as Used/Freed (optionaly)
 */
void
UDFMarkSpaceAsXXXNoProtect_(
    IN PVCB Vcb,
    IN PEXTENT_MAP Map,
    IN uint32 asXXX
#ifdef UDF_TRACK_ONDISK_ALLOCATION
   ,IN uint32 FE_lba,
    IN uint32 BugCheckId,
    IN uint32 Line
#endif //UDF_TRACK_ONDISK_ALLOCATION
    )
{
    uint32 i=0;
    uint32 lba, j, len, BS, BSh;
    uint32 root;
    BOOLEAN asUsed = (asXXX == AS_USED || (asXXX & AS_BAD));
    BOOLEAN physicalMarked = FALSE;
    BOOLEAN metadataFileMap;
#ifdef UDF_TRACK_ONDISK_ALLOCATION
    BOOLEAN bit_before, bit_after;
#endif //UDF_TRACK_ONDISK_ALLOCATION

    UDF_CHECK_BITMAP_RESOURCE(Vcb);

    if (!Map) return;

    BS = Vcb->SectorSize;
    BSh = Vcb->SectorShift;
    metadataFileMap = Vcb->MetadataFileInfo &&
        Map == Vcb->MetadataFileInfo->Dloc->DataLoc.Mapping;
    if (Vcb->MetadataMirrorFileInfo &&
        Map == Vcb->MetadataMirrorFileInfo->Dloc->DataLoc.Mapping)
        metadataFileMap = TRUE;
    uint32 partRoot = Vcb->Partitions[0].PartitionRoot;
    // walk through all frags in data area specified
    while(Map[i].extLength & UDF_EXTENT_LENGTH_MASK) {
        if ((Map[i].extLength >> 30) == EXTENT_NOT_RECORDED_NOT_ALLOCATED) {
            // skip unallocated frags
            i++;
            continue;
        }
        ASSERT(Map[i].extLocation);

#ifdef UDF_TRACK_ONDISK_ALLOCATION
        AdPrint(("Alloc:%x:%s:%x:@:%x:File:%x:Line:%d\n",
            FE_lba,
            asUsed ? ((asXXX & AS_BAD) ? "B" : "U") : "F",
            (Map[i].extLength & UDF_EXTENT_LENGTH_MASK) >> Vcb->BlockSizeBits,
            Map[i].extLocation,
            BugCheckId,
            Line
            ));
#endif //UDF_TRACK_ONDISK_ALLOCATION

#ifdef UDF_DBG
#ifdef UDF_CHECK_EXTENT_SIZE_ALIGNMENT
        ASSERT(!(Map[i].extLength & (BS-1)));
#endif //UDF_CHECK_EXTENT_SIZE_ALIGNMENT
//        len = ((Map[i].extLength & UDF_EXTENT_LENGTH_MASK)+BS-1) >> BSh;
#else // UDF_DBG
//        len = (Map[i].extLength & UDF_EXTENT_LENGTH_MASK) >> BSh;
#endif // UDF_DBG
        len = ((Map[i].extLength & UDF_EXTENT_LENGTH_MASK)+BS-1) >> BSh;
        lba = Map[i].extLocation;

        // These sectors are already reserved by the physical partition's
        // bitmap as part of the Metadata File.  Track child ICBs and directory
        // extents in the Metadata Bitmap instead; never release their physical
        // blocks to ordinary file data.
        if (Vcb->MetadataBitmap && !metadataFileMap &&
            !(asXXX & AS_BAD)) {
            if (!Vcb->MetadataPendingAlloc) {
                Vcb->VcbState |= VCB_STATE_VOLUME_READ_ONLY;
                return;
            }
            uint32 metadataLbn = UDFPhysLbaToMetadata(Vcb, lba);
            if (metadataLbn != LBA_OUT_OF_EXTENT &&
                metadataLbn < Vcb->MetadataBitCount &&
                len <= Vcb->MetadataBitCount - metadataLbn) {
                for (j = 0; j < len; j++) {
                    if (UDFMetadataLbaToPhys(Vcb, metadataLbn + j, NULL) != lba + j)
                        break;
                }
                if (j == len) {
                    if ((asXXX & AS_DISCARDED) &&
                        Vcb->TruncateTxnOwner == PsGetCurrentThread()) {
                        /* This transaction only prepared physical bits.
                         * Leave unexpected metadata bits reserved and make
                         * the outer resize fail closed. */
                        Vcb->TruncateTxnError = STATUS_NOT_SUPPORTED;
                        Map[i].extLength = (len << BSh) |
                            (EXTENT_NOT_RECORDED_NOT_ALLOCATED << 30);
                        Map[i].extLocation = 0;
                        i++;
                        continue;
                    }
                    for (j = 0; j < len; j++) {
                        uint32 bit = metadataLbn + j;
                        uint8 mask = (uint8)(1 << (bit & 7));
                        BOOLEAN wasFree = (Vcb->MetadataBitmap[bit >> 3] & mask) != 0;
                        if (asUsed && wasFree) {
                            Vcb->MetadataBitmap[bit >> 3] &= ~mask;
                            Vcb->MetadataPendingAlloc[bit >> 3] |= mask;
                            Vcb->MetadataFreeCount--;
                        } else if (!asUsed && !wasFree) {
                            Vcb->MetadataBitmap[bit >> 3] |= mask;
                            Vcb->MetadataFreeCount++;
                            if (bit < Vcb->MetadataHint)
                                Vcb->MetadataHint = bit;
                        }
                        if (!asUsed)
                            Vcb->MetadataPendingAlloc[bit >> 3] &= ~mask;
                    }
                    Vcb->MetadataBitmapModified = TRUE;
                    UDFSetModified(Vcb);
                    if (!asUsed) {
                        Map[i].extLength = (len << BSh) |
                            (EXTENT_NOT_RECORDED_NOT_ALLOCATED << 30);
                        Map[i].extLocation = 0;
                    }
                    i++;
                    continue;
                }
            }
        }

        if (lba < partRoot) {
            ASSERT(FALSE);
            i++;
            continue;
        }
        // Validate the same partition-relative range that is marked below.
        // Comparing the physical LBA with the bitmap's bit count leaves the
        // partition's final PartitionRoot blocks free after allocation.
        uint32 lbn = lba - partRoot;
        if (lbn >= Vcb->FSBM_BitCount) {
            ASSERT(FALSE);
            i++;
            continue;
        }
        if (len > Vcb->FSBM_BitCount - lbn)
            len = Vcb->FSBM_BitCount - lbn;

        if ((asXXX & AS_DISCARDED) && Vcb->TruncateTxnOwner ==
                PsGetCurrentThread()) {
            /* The shortened FE has not passed its device barrier yet.
             * Neither the RAM bitmap nor the lower TRIM may expose these
             * sectors, including to an allocation made by this thread
             * while serializing the new FE. */
            if (Vcb->TruncateTxnReleased && len &&
                Vcb->TruncateTxnCount < Vcb->TruncateTxnCapacity) {
                PEXTENT_MAP entry = &Vcb->TruncateTxnReleased[
                    Vcb->TruncateTxnCount++];
                entry->extLocation = lba;
                entry->extLength = len << BSh;
            } else {
                Vcb->TruncateTxnError = STATUS_INSUFFICIENT_RESOURCES;
            }
            Map[i].extLength = (len << BSh) |
                (EXTENT_NOT_RECORDED_NOT_ALLOCATED << 30);
            Map[i].extLocation = 0;
            i++;
            continue;
        }

        if (!physicalMarked) {
            Vcb->BitmapModified = TRUE;
            UDFSetModified(Vcb);
            physicalMarked = TRUE;
        }

        if (asUsed) {
            Vcb->SectorHint = lbn + len;
            if (Vcb->SectorHint >= Vcb->FSBM_BitCount) {
                Vcb->SectorHint = 0;
            }
        } else if (lbn < Vcb->SectorHint) {
            Vcb->SectorHint = lbn;
        }

        // mark frag as XXX (see asUsed parameter)
        if (asUsed) {
            ASSERT(len);
            if (Vcb->BitmapFcb) {
                // Per-page: clear bits (used = 0) across page boundaries
                ULONG remaining = len;
                ULONG pos = lbn;
                while (remaining > 0) {
                    UDFPinBitmapPage(Vcb, pos);
                    ULONG localIdx = pos - Vcb->BitmapPageStartLbn;
                    ULONG bitsInPage = min(remaining, Vcb->BitmapPageBitCount - localIdx);
                    Vcb->FreeAllocUnits -= UDFCountFreeBits(&Vcb->BitmapRtl, localIdx, bitsInPage);
                    RtlClearBits(&Vcb->BitmapRtl, localIdx, bitsInPage);
                    UDFDirtyBitmapPage(Vcb);
                    pos += bitsInPage;
                    remaining -= bitsInPage;
                }
            } else {
                UDFSetUsedBits(Vcb->FSBM_Bitmap, lbn, len);
            }

            if (Vcb->Vat) {
                for(j=0;j<len;j++) {
                    root = UDFPartStart(Vcb, UDFGetRefPartNumByPhysLba(Vcb, lba));
                    if ((Vcb->Vat[lba-root+j] == UDF_VAT_FREE_ENTRY) &&
                       (lba > Vcb->SessionEndLba)) {
                         Vcb->Vat[lba-root+j] = 0x7fffffff;
                    }
                }
            }
        } else {
            ASSERT(len);
            if (Vcb->BitmapFcb) {
                // Per-page: set bits (free = 1) across page boundaries
                ULONG remaining = len;
                ULONG pos = lbn;
                while (remaining > 0) {
                    UDFPinBitmapPage(Vcb, pos);
                    ULONG localIdx = pos - Vcb->BitmapPageStartLbn;
                    ULONG bitsInPage = min(remaining, Vcb->BitmapPageBitCount - localIdx);
                    Vcb->FreeAllocUnits += bitsInPage - UDFCountFreeBits(&Vcb->BitmapRtl, localIdx, bitsInPage);
                    RtlSetBits(&Vcb->BitmapRtl, localIdx, bitsInPage);
                    UDFDirtyBitmapPage(Vcb);
                    pos += bitsInPage;
                    remaining -= bitsInPage;
                }
            } else {
                UDFSetFreeBits(Vcb->FSBM_Bitmap, lbn, len);
            }
            if (asXXX & AS_BAD) {
                UDFSetBits(Vcb->BSBM_Bitmap, lbn, len);
            }
            UDFMarkBadSpaceAsUsed(Vcb, lbn, len);

            if (asXXX & AS_DISCARDED) {
                UDFUnmapRange(Vcb, lba, len);
            }
            if (Vcb->Vat) {
                for(j=0;j<len;j++) {
                    root = UDFPartStart(Vcb, UDFGetRefPartNumByPhysLba(Vcb, lba));
                    Vcb->Vat[lba-root+j] = UDF_VAT_FREE_ENTRY;
                }
            }
            Map[i].extLength = (len << BSh) | (EXTENT_NOT_RECORDED_NOT_ALLOCATED << 30);
            Map[i].extLocation = 0;
        }

        i++;
    }

    UDFUnpinBitmapPage(Vcb);
} // end UDFMarkSpaceAsXXXNoProtect_()

/*
    This routine marks space described by Mapping as Used/Freed (optionaly)
    It protects data with sync Resource
 */
void
UDFMarkSpaceAsXXX_(
    IN PVCB Vcb,
    IN PEXTENT_MAP Map,
    IN uint32 asXXX
#ifdef UDF_TRACK_ONDISK_ALLOCATION
   ,IN uint32 FE_lba,
    IN uint32 BugCheckId,
    IN uint32 Line
#endif //UDF_TRACK_ONDISK_ALLOCATION
    )
{
    if (!Map) return;
    if (!Map[0].extLength) {
#ifdef UDF_DBG
        ASSERT(!Map[0].extLocation);
#endif // UDF_DBG
        return;
    }

    UDFAcquireResourceExclusive(&(Vcb->BitMapResource1),TRUE);
#ifdef UDF_TRACK_ONDISK_ALLOCATION
    UDFMarkSpaceAsXXXNoProtect_(Vcb, Map, asXXX, FE_lba, BugCheckId, Line);
#else //UDF_TRACK_ONDISK_ALLOCATION
    UDFMarkSpaceAsXXXNoProtect_(Vcb, Map, asXXX);
#endif //UDF_TRACK_ONDISK_ALLOCATION
    UDFReleaseResource(&(Vcb->BitMapResource1));

} // end UDFMarkSpaceAsXXX_()

/*
    This routine builds mapping for Length bytes in FreeSpace
    It should be used when IN_ICB method is unavailable.
 */
NTSTATUS
UDFAllocFreeExtent_(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB   Vcb,
    IN int64  Length,
    IN uint32 SearchStart,
    IN uint32 SearchLim,     // NOT included
    OUT PEXTENT_INFO ExtInfo,
    IN uint8  AllocFlags
#ifdef UDF_TRACK_ALLOC_FREE_EXTENT
   ,IN uint32 src,
    IN uint32 line
#endif //UDF_TRACK_ALLOC_FREE_EXTENT
    )
{
    EXTENT_AD Ext;
    PEXTENT_MAP Map = NULL;
    uint32 len, LBS, BSh, blen;
    uint32 dataHint;
    BOOLEAN feLocality = (AllocFlags & EXTENT_FLAG_FE_LOCALITY) != 0;

    if (Vcb->MetadataFileInfo &&
        SearchStart == UDFPartStart(Vcb, Vcb->MetadataRef) &&
        SearchLim == UDFPartEnd(Vcb, Vcb->MetadataRef))
        return UDFAllocMetadataExtent(IrpContext, Vcb, Length, ExtInfo);

    LBS = Vcb->SectorSize;
    BSh = Vcb->SectorShift;
    uint32 MaxExtentLength = ALIGN_DOWN_BY(UDF_EXTENT_LENGTH_MASK, LBS);
    blen = (uint32)(((Length+LBS-1) & ~((int64)LBS-1)) >> BSh);
    ExtInfo->Mapping = NULL;
    ExtInfo->Offset = 0;

    ASSERT(blen <= (uint32)(MaxExtentLength >> BSh));

    UDFAcquireResourceExclusive(&(Vcb->BitMapResource1),TRUE);
    dataHint = Vcb->SectorHint;
    /* A normal data allocation must resume from its old cursor after a
     * separately placed FE. The bitmap lock covers both cursors and mark. */
    ASSERT(!feLocality || (Length > 0 && Length <= LBS && blen == 1));

    if (blen > (SearchLim - SearchStart)) {
        goto no_free_space_err;
    }
    // walk through the free space bitmap & find a single extent or a set of
    // frags giving in sum the Length specified
    while(blen) {
        Ext.extLocation = UDFFindMinSuitableExtent(Vcb, blen, SearchStart,
                                                               SearchLim, &len, AllocFlags);

        if (len >= blen) {
            // complete search
            Ext.extLength = blen<<BSh;
            blen = 0;
        } else if (len) {
            // we need still some frags to complete request &
            // probably we have the opportunity to do it
            Ext.extLength = len<<BSh;
            blen -= len;
        } else {
no_free_space_err:
            // no more free space. abort
            /* The preferred FE quarter can become full on a writable
             * volume. Do not rescan and log that entire range for every
             * later create; ordinary full-range allocation still runs. */
            if (feLocality)
                Vcb->FeSectorHint = MAXULONG;
            if (!feLocality)
                UDFPrint(("UDF BM: DISK_FULL blen=%x SearchStart=%x SearchLim=%x BitmapFcb=%p BitCount=%x\n",
                    blen, SearchStart, SearchLim, Vcb->BitmapFcb, Vcb->FSBM_BitCount));
            if (ExtInfo->Mapping) {
                UDFMarkSpaceAsXXXNoProtect(Vcb, 0, ExtInfo->Mapping, AS_DISCARDED); // free
                MyFreePool__(ExtInfo->Mapping);
                ExtInfo->Mapping = NULL;
            }
            UDFReleaseResource(&(Vcb->BitMapResource1));
            ExtInfo->Length = 0;//UDFGetExtentLength(ExtInfo->Mapping);
            if (!feLocality)
                AdPrint(("  DISK_FULL\n"));
            return STATUS_DISK_FULL;
        }
        // append the frag found to mapping
        ASSERT(!(Ext.extLength >> 30));
        ASSERT(Ext.extLocation);

        if ((Vcb->TargetDeviceObject->DeviceType != FILE_DEVICE_DISK) &&
            (AllocFlags & EXTENT_FLAG_VERIFY)) {
            if (!UDFCheckArea(IrpContext, Vcb, Ext.extLocation, Ext.extLength >> BSh)) {
                AdPrint(("newly allocated extent contains BB\n"));
                UDFMarkSpaceAsXXXNoProtect(Vcb, 0, ExtInfo->Mapping, AS_DISCARDED); // free
                UDFMarkBadSpaceAsUsed(Vcb, Ext.extLocation, Ext.extLength >> BSh); // bad -> bad+used
                // roll back
                blen += Ext.extLength>>BSh;
                continue;
            }
        }

        Ext.extLength |= EXTENT_NOT_RECORDED_ALLOCATED << 30;
        if (!(ExtInfo->Mapping)) {
            // create new
#ifdef UDF_TRACK_ALLOC_FREE_EXTENT
            ExtInfo->Mapping = UDFExtentToMapping_(&Ext, src, line);
#else // UDF_TRACK_ALLOC_FREE_EXTENT
            ExtInfo->Mapping = UDFExtentToMapping(&Ext);
#endif // UDF_TRACK_ALLOC_FREE_EXTENT
            if (!ExtInfo->Mapping) {
                BrutePoint();
                UDFReleaseResource(&(Vcb->BitMapResource1));
                ExtInfo->Length = 0;
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            UDFMarkSpaceAsXXXNoProtect(Vcb, 0, ExtInfo->Mapping, AS_USED); // used
            if (feLocality) {
                uint32 next = Ext.extLocation - Vcb->Partitions[0].PartitionRoot + 1;
                Vcb->FeSectorHint = next < SearchLim - Vcb->Partitions[0].PartitionRoot ?
                                    next : SearchStart - Vcb->Partitions[0].PartitionRoot;
                Vcb->SectorHint = dataHint;
            }
        } else {
            // update existing
            Map = UDFExtentToMapping(&Ext);
            if (!Map) {
                BrutePoint();
                UDFReleaseResource(&(Vcb->BitMapResource1));
                ExtInfo->Length = UDFGetExtentLength(ExtInfo->Mapping);
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            UDFMarkSpaceAsXXXNoProtect(Vcb, 0, Map, AS_USED); // used
            ExtInfo->Mapping = UDFMergeMappings(ExtInfo->Mapping, Map);
            MyFreePool__(Map);
        }
        if (!ExtInfo->Mapping) {
            BrutePoint();
            UDFReleaseResource(&(Vcb->BitMapResource1));
            ExtInfo->Length = 0;
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    UDFReleaseResource(&(Vcb->BitMapResource1));
    ExtInfo->Length = Length;
    return STATUS_SUCCESS;
} // end UDFAllocFreeExtent_()

/*
    Returns block-count
 */
uint32
__fastcall
UDFGetPartFreeSpace(
    IN PVCB Vcb,
    IN uint32 partNum
    )
{
    uint32 s=0;

    if (Vcb->MetadataFileInfo && partNum == Vcb->MetadataRef)
        return Vcb->MetadataFreeCount;

    if (Vcb->BitmapFcb) {
        // Even the readers should be exclusive because the pinned page is shared state.
        UDFAcquireResourceExclusive(&(Vcb->BitMapResource1),TRUE);
        // Per-page: iterate pinned pages, count free (set) bits via RTL_BITMAP
        ULONG pos = 0;
        while (pos < Vcb->FSBM_BitCount) {
            UDFPinBitmapPage(Vcb, pos);
            ULONG bits = Vcb->BitmapPageBitCount;
            ULONG startLbn = Vcb->BitmapPageStartLbn;
            ULONG bufWords = (bits + 31) / 32;
            // ULONG dataBytes = (Vcb->BitmapRtl.SizeOfBitMap + 7) / 8;
            if (bufWords * 4 > Vcb->BitmapPinnedLength) {
                UDFPrint(("UDF BM: FreeSpace OVERFLOW pos=%x bits=%x need=%x pinLen=%x pinOff=%x\n",
                    pos, bits, bufWords * 4, Vcb->BitmapPinnedLength, Vcb->BitmapPinnedOffset));
                break;
            }
            s += RtlNumberOfSetBits(&Vcb->BitmapRtl);
            pos = startLbn + bits;
        }
        UDFUnpinBitmapPage(Vcb);
        UDFReleaseResource(&(Vcb->BitMapResource1));
    } else if (Vcb->FSBM_Bitmap) {
        PUCHAR cur = (PUCHAR)(Vcb->FSBM_Bitmap);
        ULONG lim = (Vcb->FSBM_BitCount+7)/8;
        for(ULONG j=0; j<lim; j++) {
            s+=bit_count_tab[cur[j]];
        }
    } else {
        // Read-only optical media may have no free-space bitmap.
        // A missing bitmap cannot report allocatable blocks.
        UDFPrint(("UDF: no free-space bitmap for partition %u\n", partNum));
    }
    return s;
} // end UDFGetPartFreeSpace()

int64
__fastcall
UDFGetFreeSpace(
    IN PVCB Vcb
    )
{
    int64 s=0;
    uint32 i;

   if (!Vcb->CDR_Mode) {
        for(i=0; i<Vcb->PartitionMaps;i++) {
            if (Vcb->Partitions[i].PartitionType == UDF_METADATA_MAP25)
                continue;
            
            s += UDFGetPartFreeSpace(Vcb, i);
        }
    } else {
        ASSERT(Vcb->FSBM_BitCount >= max(Vcb->NWA, Vcb->SessionEndLba));
        s = Vcb->FSBM_BitCount - max(Vcb->NWA, Vcb->SessionEndLba);
    }
    return s;
} // end UDFGetFreeSpace()

/*
    Returns block-count
 */
int64
UDFGetTotalSpace(
    IN PVCB Vcb
    )
{
    uint32 i;
    int64 s=0;

    if (!Vcb->CDR_Mode) {
        for(i=0; i<Vcb->PartitionMaps;i++) {
            if (Vcb->Partitions[i].PartitionType == UDF_METADATA_MAP25)
                continue;

            s+=Vcb->Partitions[i].PartitionLen;
        }
    } else
        s = Vcb->Partitions[0].PartitionLen;

    return s;
} // end UDFGetTotalSpace()
