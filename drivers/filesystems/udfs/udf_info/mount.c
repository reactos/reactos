////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////
/*
        Module name:

   mount.cpp

        Abstract:

   This file contains filesystem-specific routines
   responsible for Mount/Umount

*/

#include "udf.h"

/* FIXME */
#ifdef XCHG_DD
#undef XCHG_DD
#endif

#define XCHG_DD(a,b)              \
{                                 \
    ULONG  _temp_;                \
    PULONG _from_, _to_;          \
    _from_ = ((PULONG)&(b));      \
    _to_ =   ((PULONG)&(a));      \
    _temp_ = *_from_;             \
    *_from_ = *_to_;              \
    *_to_ = _temp_;               \
}

#define         UDF_BUG_CHECK_ID                UDF_FILE_UDF_INFO_MOUNT

NTSTATUS
__fastcall
UDFSetDstring(
    IN PUNICODE_STRING UName,
    IN dstring* Dest,
    IN uint32 Length
    );

/*
    This routine loads specified bitmap.
    It is also allocate space if the bitmap is not allocated.
 */
NTSTATUS
UDFPrepareXSpaceBitmap(
    PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN uint32 RefPartNum,
 IN OUT PSHORT_AD XSpaceBitmap,
 IN OUT PEXTENT_INFO XSBMExtInfo,
 IN OUT int8** XSBM,
 IN OUT uint32* XSl
    )
{
    uint32 BS, j, LBS;
    uint32 plen;
    NTSTATUS status;
    EXTENT_MAP TmpExt;
    lb_addr locAddr;
    int8* _XSBM;
    uint16 Ident;

    if (!(XSpaceBitmap->extLength)) {
        *XSl = 0;
        *XSBM = NULL;
        return STATUS_SUCCESS;
    }

    if (RefPartNum >= Vcb->PartitionMaps ||
        Vcb->Partitions[RefPartNum].PartitionType == UDF_METADATA_MAP25)
        return STATUS_INVALID_PARAMETER;
    locAddr.partitionReferenceNum = (uint16)RefPartNum;
    /* Bitmap bit zero names partition-relative block zero. The descriptor
     * lives in the physical map supplied by the Partition Descriptor, even
     * when a shorter metadata map refers to that same physical partition. */
    plen = UDFPartLen(Vcb, RefPartNum);

    BS = Vcb->SectorSize;
    LBS = Vcb->SectorSize;

    *XSl = sizeof(SPACE_BITMAP_DESC) + ((plen+7)>>3);
    _XSBM = (int8*)DbgAllocatePool(NonPagedPool, (*XSl + BS - 1) & ~(BS-1) );
    *XSBM = _XSBM;
    if (!_XSBM) {
        *XSl = 0;
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    TmpExt.extLength = 0;
    TmpExt.extLocation = 0;
    locAddr.logicalBlockNum = 0;

    switch (XSpaceBitmap->extLength >> 30) {
    case EXTENT_RECORDED_ALLOCATED: {
        locAddr.logicalBlockNum = XSpaceBitmap->extPosition;
        *XSl = min(XSpaceBitmap->extLength, *XSl);
        UDFPrint(("XSpaceBitmap->extLength=%x, *XSl=%x\n", XSpaceBitmap->extLength, *XSl));
        // TmpExt.extLength = XSpaceBitmap->extLength = *XSl;
        TmpExt.extLength = XSpaceBitmap->extLength;
        TmpExt.extLocation = UDFPartLbaToPhys(Vcb, &locAddr);
        if (TmpExt.extLocation == LBA_OUT_OF_EXTENT) {
            BrutePoint();
        }
        XSBMExtInfo->Mapping = UDFExtentToMapping(&TmpExt);
        XSBMExtInfo->Offset = 0;
        XSBMExtInfo->Length = *XSl;
        break;
    }
    case EXTENT_NEXT_EXTENT_ALLOCDESC:
    case EXTENT_NOT_RECORDED_NOT_ALLOCATED: {
        // allocate space for bitmap
        if (!NT_SUCCESS(status = UDFAllocFreeExtent(IrpContext, Vcb, *XSl,
               UDFPartStart(Vcb, RefPartNum), UDFPartEnd(Vcb, RefPartNum), XSBMExtInfo, EXTENT_FLAG_ALLOC_SEQUENTIAL) )) {
            DbgFreePool(_XSBM);
            *XSBM = NULL;
            *XSl = 0;
            return status;
        }
        if (XSBMExtInfo->Mapping[1].extLength) {
            UDFPrint(("Can't allocate space for Freed Space bitmap\n"));
            *XSl = 0;
        } else {
            *XSl = (uint32)(XSBMExtInfo->Length);
            XSpaceBitmap->extPosition = UDFPhysLbaToPart(Vcb, RefPartNum, XSBMExtInfo->Mapping[0].extLocation);
        }
        break;
    }
    case EXTENT_NOT_RECORDED_ALLOCATED: {
        // record Alloc-Not-Rec
        locAddr.logicalBlockNum = XSpaceBitmap->extPosition;
        *XSl = min((XSpaceBitmap->extLength & UDF_EXTENT_LENGTH_MASK), *XSl);
        TmpExt.extLength = XSpaceBitmap->extLength = *XSl;
        TmpExt.extLocation = UDFPartLbaToPhys(Vcb, &locAddr);
        if (TmpExt.extLocation == LBA_OUT_OF_EXTENT) {
            BrutePoint();
        }
        XSBMExtInfo->Mapping = UDFExtentToMapping(&TmpExt);
        XSBMExtInfo->Offset = 0;
        XSBMExtInfo->Length = *XSl;
        break;
    }
    }

    if (!_XSBM) {
        BrutePoint();
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    switch (XSpaceBitmap->extLength >> 30) {
    case EXTENT_RECORDED_ALLOCATED: {
        // read descriptor & bitmap
        if ((!NT_SUCCESS(status = UDFReadTagged(IrpContext, Vcb, *XSBM, (j = TmpExt.extLocation),
                             locAddr.logicalBlockNum, &Ident))) ||
           (Ident != TID_SPACE_BITMAP_DESC) ||
           (!NT_SUCCESS(status = UDFReadExtent(IrpContext, Vcb, XSBMExtInfo, 0, *XSl, FALSE, *XSBM))) ) {
            if (NT_SUCCESS(status)) {
                BrutePoint();
                status = STATUS_FILE_CORRUPT_ERROR;
            }
            if (XSBMExtInfo->Mapping) {
                MyFreePool__(XSBMExtInfo->Mapping);
                XSBMExtInfo->Mapping = NULL;
            }
            DbgFreePool(*XSBM);
            *XSl = 0;
            *XSBM = NULL;
            return status;
        } else {
//            BrutePoint();
        }
        return STATUS_SUCCESS;
    }
#if 0
    case EXTENT_NEXT_EXTENT_ALLOCDESC:
    case EXTENT_NOT_RECORDED_NOT_ALLOCATED:
    case EXTENT_NOT_RECORDED_ALLOCATED: {
        break;
    }
#endif
    }

    PSPACE_BITMAP_DESC XSDesc = (PSPACE_BITMAP_DESC)(*XSBM);

    XSpaceBitmap->extLength = (*XSl + LBS -1) & ~(LBS-1);
    RtlZeroMemory(*XSBM, *XSl);
    XSDesc->descTag.tagIdent = TID_SPACE_BITMAP_DESC;
    UDFSetUpTag(Vcb, &(XSDesc->descTag), 0, XSpaceBitmap->extPosition, 0);
    XSDesc->numOfBits = plen;
    XSDesc->numOfBytes = (*XSl)-sizeof(SPACE_BITMAP_DESC);

    return STATUS_SUCCESS;
} // end UDFPrepareXSpaceBitmap()

/*
    This routine updates Freed & Unallocated space bitmaps
 */
NTSTATUS
UDFUpdateXSpaceBitmaps(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN uint32 RefPartNum,
    IN PPARTITION_HEADER_DESC phd // partition header pointing to Bitmaps
    )
{
    uint32 i,j,d;
    uint32 plen, pstart, pend;
    int8* bad_bm;
    int8* old_bm;
    int8* new_bm;
    int8* fpart_bm = NULL;
    int8* upart_bm = NULL;
    NTSTATUS status, status2;
    int8* USBM=NULL;
    int8* FSBM=NULL;
    uint32 USl = 0, FSl = 0;
    EXTENT_INFO FSBMExtInfo = {0}, USBMExtInfo = {0};
    SIZE_T WrittenBytes = 0;

    UDF_CHECK_BITMAP_RESOURCE(Vcb);

    plen = UDFPartLen(Vcb, RefPartNum);
    // prepare bitmaps for updating

    status =  UDFPrepareXSpaceBitmap(IrpContext, Vcb, RefPartNum, &phd->unallocatedSpaceBitmap, &USBMExtInfo, &USBM, &USl);
    status2 = UDFPrepareXSpaceBitmap(IrpContext, Vcb, RefPartNum, &phd->freedSpaceBitmap, &FSBMExtInfo, &FSBM, &FSl);
    if (!NT_SUCCESS(status) ||
       !NT_SUCCESS(status2)) {
        BrutePoint();
    }

    pstart = UDFPartStart(Vcb, RefPartNum);
    new_bm = Vcb->FSBM_Bitmap;
    old_bm = Vcb->FSBM_OldBitmap;
    bad_bm = Vcb->BSBM_Bitmap;

    if ((status  == STATUS_INSUFFICIENT_RESOURCES) ||
       (status2 == STATUS_INSUFFICIENT_RESOURCES)) {
        /* This fallback writes RAM bitmap bytes directly.  It cannot
         * preserve the disk-only FREE bits of pending unlink transactions. */
        if (Vcb->DeletePrefreeList) {
            if (USBM) DbgFreePool(USBM);
            if (FSBM) DbgFreePool(FSBM);
            if (USBMExtInfo.Mapping) MyFreePool__(USBMExtInfo.Mapping);
            if (FSBMExtInfo.Mapping) MyFreePool__(FSBMExtInfo.Mapping);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        // try to recover insufficient resources
        if (USl && USBMExtInfo.Mapping) {
            USl -= sizeof(SPACE_BITMAP_DESC);
            WrittenBytes = 0;
            status  = UDFWriteExtent(IrpContext, Vcb, &USBMExtInfo, sizeof(SPACE_BITMAP_DESC), USl, FALSE, new_bm, &WrittenBytes);
            if (NT_SUCCESS(status) && WrittenBytes != USl)
                status = STATUS_IO_DEVICE_ERROR;
#ifdef UDF_DBG
        } else {
            UDFPrint(("Can't update USBM\n"));
#endif // UDF_DBG
        }
        if (USBMExtInfo.Mapping) MyFreePool__(USBMExtInfo.Mapping);

        if (FSl && FSBMExtInfo.Mapping) {
            FSl -= sizeof(SPACE_BITMAP_DESC);
            WrittenBytes = 0;
            status2 = UDFWriteExtent(IrpContext, Vcb, &FSBMExtInfo, sizeof(SPACE_BITMAP_DESC), FSl, FALSE, new_bm, &WrittenBytes);
            if (NT_SUCCESS(status2) && WrittenBytes != FSl)
                status2 = STATUS_IO_DEVICE_ERROR;
        } else {
            status2 = status;
            UDFPrint(("Can't update FSBM\n"));
        }
        if (FSBMExtInfo.Mapping) MyFreePool__(FSBMExtInfo.Mapping);
    } else {
        // normal way to record BitMaps
        // Bitmap is LBN-indexed (0..PartitionLen), matching on-disk format
        if (USBM) upart_bm =  USBM + sizeof(SPACE_BITMAP_DESC);
        if (FSBM) fpart_bm =  FSBM + sizeof(SPACE_BITMAP_DESC);
        pend = min(plen, Vcb->FSBM_BitCount);

        d=1;
        // if we have some bad bits, mark corresponding area as BAD
        if (bad_bm) {
            for(i=0; i<pend; i++) {
                if (UDFGetBadBit(bad_bm, i)) {
                    // TODO: would be nice to add these blocks to unallocatable space
                    UDFSetUsedBits(new_bm, i & ~(d-1), d);
                }
            }
        }
        j=0;
        for(i=0; i<pend; i+=d) {
            if (UDFGetUsedBit(old_bm, i) && UDFGetFreeBit(new_bm, i)) {
                // sector was deallocated during last session
                if (USBM) UDFSetFreeBit(upart_bm, j);
                if (FSBM) UDFSetFreeBit(fpart_bm, j);
            } else if (UDFGetUsedBit(new_bm, i)) {
                // allocated
                if (USBM) UDFSetUsedBit(upart_bm, j);
                if (FSBM) UDFSetUsedBit(fpart_bm, j);
            }
            j++;
        }
        status = UDFOverlayDeletePrefreePhysical(Vcb, upart_bm,
                                                 fpart_bm, pend);
        if (!NT_SUCCESS(status)) {
            if (USBM) DbgFreePool(USBM);
            if (FSBM) DbgFreePool(FSBM);
            if (USBMExtInfo.Mapping) MyFreePool__(USBMExtInfo.Mapping);
            if (FSBMExtInfo.Mapping) MyFreePool__(FSBMExtInfo.Mapping);
            return status;
        }
        // flush updates
        if (USBM) {
            WrittenBytes = 0;
            status  = UDFWriteExtent(IrpContext, Vcb, &USBMExtInfo, 0, USl, FALSE, USBM, &WrittenBytes);
            if (NT_SUCCESS(status) && WrittenBytes != USl)
                status = STATUS_IO_DEVICE_ERROR;
            DbgFreePool(USBM);
            MyFreePool__(USBMExtInfo.Mapping);
        }
        if (FSBM) {
            WrittenBytes = 0;
            status2 = UDFWriteExtent(IrpContext, Vcb, &FSBMExtInfo, 0, FSl, FALSE, FSBM, &WrittenBytes);
            if (NT_SUCCESS(status2) && WrittenBytes != FSl)
                status2 = STATUS_IO_DEVICE_ERROR;
            DbgFreePool(FSBM);
            MyFreePool__(FSBMExtInfo.Mapping);
        } else {
            status2 = status;
        }
    }

    if (!NT_SUCCESS(status))
        return status;
    return status2;
} // end UDFUpdateXSpaceBitmaps()

/*
    This routine updates Partition Desc & associated data structures
 */
NTSTATUS
UDFUpdatePartDesc(
    PIRP_CONTEXT IrpContext,
    PVCB Vcb,
    int8* Buf
    )
{
    struct PartitionDesc *p = (struct PartitionDesc *)Buf;
    uint32 i; // PartNdx
    tag* PTag;
    SIZE_T WrittenBytes;
    NTSTATUS status;

    for(i=0; i<Vcb->PartitionMaps; i++)
    {
        /* Both the physical and metadata maps name this Partition Descriptor.
         * Its unallocated-space bitmap belongs to the physical map alone;
         * using the metadata map's shorter length here truncates that bitmap.
         * UDFFlushMetadataBitmap handles the separate metadata bitmap. */
        if ((Vcb->Partitions[i].PartitionType != UDF_METADATA_MAP25) &&
            (UDFGetPartNumByPartRef(Vcb,i) == p->partitionNumber) &&
           (!strcmp((int8*)&(p->partitionContents.ident), PARTITION_CONTENTS_NSR02) ||
            !strcmp((int8*)&(p->partitionContents.ident), PARTITION_CONTENTS_NSR03)))
        {
            PPARTITION_HEADER_DESC phd;

            phd = (PPARTITION_HEADER_DESC)(p->partitionContentsUse);
#ifdef UDF_DBG
            if (phd->unallocatedSpaceTable.extLength) {
                // rebuild unallocatedSpaceTable
                UDFPrint(("unallocatedSpaceTable (part %d)\n", i));
            }
            if (phd->freedSpaceTable.extLength) {
                // rebuild freedSpaceTable
                UDFPrint(("freedSpaceTable (part %d)\n", i));
            }
#endif // UDF_DBG
            if (Vcb->BitmapFcb) {
                // Per-page mode: flush dirty bitmap pages through Cache Manager
                UDFUnpinBitmapPage(Vcb);
                IO_STATUS_BLOCK FlushIoStatus;
                FlushIoStatus.Status = STATUS_SUCCESS;
                FlushIoStatus.Information = 0;
                CcFlushCache(&Vcb->BitmapNonpaged.SegmentObject, NULL, 0, &FlushIoStatus);
                if (!NT_SUCCESS(FlushIoStatus.Status))
                    return FlushIoStatus.Status;
            } else {
                status = UDFUpdateXSpaceBitmaps(IrpContext, Vcb, i, phd);
                if (!NT_SUCCESS(status))
                    return status;
            }
            PTag = (tag*)Buf;
            UDFSetUpTag(Vcb, PTag, PTag->descCRCLength + sizeof(tag), PTag->tagLocation, 0);
            status = UDFWriteSectors(IrpContext, Vcb, TRUE, PTag->tagLocation, 1, FALSE, Buf, &WrittenBytes);
            if (!NT_SUCCESS(status) || WrittenBytes != Vcb->SectorSize)
                return NT_SUCCESS(status) ? STATUS_IO_DEVICE_ERROR : status;
        }
    }
    return STATUS_SUCCESS;
} // end UDFUpdatePartDesc()

/*
    This routine blanks Unalloc Space Desc
 *//*
NTSTATUS
UDFUpdateUSpaceDesc(
    IN PVCB Vcb,
    int8* Buf
    )
{
    PUNALLOC_SPACE_DESC usd;
    SIZE_T WrittenBytes;

    usd = (PUNALLOC_SPACE_DESC)Buf;
    usd->numAllocDescs = 0;
    RtlZeroMemory(Buf+sizeof(UNALLOC_SPACE_DESC), Vcb->BlockSize - sizeof(UNALLOC_SPACE_DESC));
    UDFSetUpTag(Vcb, &(usd->descTag), 0, usd->descTag.tagLocation);
    UDFWriteSectors(Vcb, TRUE, usd->descTag.tagLocation, 1, FALSE, Buf, &WrittenBytes);
    return STATUS_SUCCESS;
}*/

/*
   update Logical volume integrity descriptor
 */
#define UDF_LVID_JOURNAL_IMP_LENGTH 70
#define UDF_LVID_JOURNAL_TOTAL_LENGTH 166
#define UDF_LVID_JOURNAL_CRC_LENGTH 150

static const uint8 UdfLvidJournalMagic[8] = {
    'R', 'O', 'S', 'L', 'V', 'I', 'D', '1'
};

static NTSTATUS
UDFJournalDiscoveryUnavailable(
    IN PVCB Vcb,
    IN BOOLEAN Required
    )
{
    if (!Required)
        return STATUS_SUCCESS; /* A volume without our journal uses legacy I/O. */
    /* A journaled final LVID must not silently fall back to unjournaled
     * writes when its reserved slots or layout no longer validate. */
    Vcb->VcbState |= VCB_STATE_VOLUME_READ_ONLY;
    UDFPrint(("UDF: required LVID journal unavailable; refusing writes\n"));
    return STATUS_FILE_CORRUPT_ERROR;
}

static BOOLEAN
UDFJournalSlotValid(
    IN uint8* Sector,
    IN uint32 SlotLba,
    IN uint32 TargetLba,
    OUT PULONGLONG Generation
    )
{
    tag* Tag = (tag*)Sector;
    struct LogicalVolIntegrityDesc* Lvid =
        (struct LogicalVolIntegrityDesc*)Sector;
    uint32 Checksum = 0, i, RecordedTarget, Flags;

    if (Tag->tagIdent != TID_LOGICAL_VOL_INTEGRITY_DESC ||
        Tag->tagLocation != SlotLba ||
        Tag->descCRCLength != UDF_LVID_JOURNAL_CRC_LENGTH ||
        Lvid->integrityType != INTEGRITY_TYPE_OPEN ||
        Lvid->nextIntegrityExt.extLength ||
        Lvid->nextIntegrityExt.extLocation ||
        Lvid->numOfPartitions != 2 ||
        Lvid->lengthOfImpUse != UDF_LVID_JOURNAL_IMP_LENGTH ||
        RtlCompareMemory(Sector + 142, UdfLvidJournalMagic, 8) != 8 ||
        Tag->descCRC != UDFCrc(Sector + sizeof(tag),
                                Tag->descCRCLength, 0))
        return FALSE;
    for (i = 0; i < sizeof(tag); ++i)
        if (i != 4) Checksum += Sector[i];
    if (Tag->tagChecksum != (uint8)Checksum) return FALSE;
    RtlCopyMemory(Generation, Sector + 150, sizeof(*Generation));
    RtlCopyMemory(&RecordedTarget, Sector + 158,
                  sizeof(RecordedTarget));
    RtlCopyMemory(&Flags, Sector + 162, sizeof(Flags));
    return *Generation != 0 && RecordedTarget == TargetLba && Flags <= 1;
}

static NTSTATUS
UDFDiscoverLvidJournal(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )
{
    uint8 Sector[512];
    uint32 i, j, Slot, PhysicalRoot;
    ULONGLONG Generation, Latest = 0;
    BOOLEAN Valid[2] = {FALSE, FALSE};
    BOOLEAN Empty[2] = {FALSE, FALSE};
    BOOLEAN Required;
    NTSTATUS Status;

    if (Vcb->LvidJournalState) return STATUS_SUCCESS;
    Required = Vcb->LvidJournalTarget != 0 ||
               (Vcb->LVid &&
                Vcb->LVid_loc.extLength == 512 &&
                Vcb->LVid->descTag.descCRCLength ==
                    UDF_LVID_JOURNAL_CRC_LENGTH &&
                Vcb->LVid->lengthOfImpUse ==
                    UDF_LVID_JOURNAL_IMP_LENGTH &&
                RtlCompareMemory((uint8*)Vcb->LVid + 142,
                                 UdfLvidJournalMagic, 8) == 8);
    Vcb->LvidJournalState = 2; /* unavailable unless all bounds validate */
    if (SectorSize(Vcb) != 512 ||
        Vcb->TargetDeviceObject->DeviceType != FILE_DEVICE_DISK ||
        Vcb->PartitionMaps != 2 || !Vcb->Partitions ||
        Vcb->MetadataRef >= Vcb->PartitionMaps ||
        Vcb->MetadataPhysRef >= Vcb->PartitionMaps ||
        Vcb->Partitions[Vcb->MetadataRef].PartitionType !=
            UDF_METADATA_MAP25 ||
        Vcb->Partitions[Vcb->MetadataPhysRef].PartitionType !=
            UDF_TYPE1_MAP15 ||
        Vcb->LVid_loc.extLength != 512 ||
        Vcb->LvidJournalTarget != Vcb->LVid_loc.extLocation ||
        !Vcb->LvidJournalSlot[0] ||
        Vcb->LvidJournalSlot[0] + 1 != Vcb->LvidJournalSlot[1] ||
        Vcb->LvidJournalSlot[1] == Vcb->LvidJournalTarget ||
        Vcb->LvidJournalSlot[0] == Vcb->LvidJournalTarget)
        return UDFJournalDiscoveryUnavailable(Vcb, Required);

    PhysicalRoot = Vcb->Partitions[Vcb->MetadataPhysRef].PartitionRoot;
    if (!PhysicalRoot || Vcb->LvidJournalSlot[1] >= PhysicalRoot)
        return UDFJournalDiscoveryUnavailable(Vcb, Required);
    for (i = 0; i < 2; ++i) {
        Slot = Vcb->LvidJournalSlot[i];
        for (j = 0; j < sizeof(Vcb->Anchor) /
                        sizeof(Vcb->Anchor[0]); ++j)
            if (Slot == Vcb->Anchor[j])
                return UDFJournalDiscoveryUnavailable(Vcb, Required);
        if ((Vcb->VDS1_Len && Slot >= Vcb->VDS1 &&
             (ULONGLONG)Slot <
                 (ULONGLONG)Vcb->VDS1 + Vcb->VDS1_Len) ||
            (Vcb->VDS2_Len && Slot >= Vcb->VDS2 &&
             (ULONGLONG)Slot <
                 (ULONGLONG)Vcb->VDS2 + Vcb->VDS2_Len))
            return UDFJournalDiscoveryUnavailable(Vcb, Required);
        Status = UDFReadSectors(IrpContext, Vcb, FALSE, Slot,
                                1, FALSE, (int8*)Sector);
        if (!NT_SUCCESS(Status)) return Status;
        Empty[i] = TRUE;
        for (j = 0; j < sizeof(Sector); ++j)
            if (Sector[j]) { Empty[i] = FALSE; break; }
        Valid[i] = UDFJournalSlotValid(Sector, Slot,
                        Vcb->LvidJournalTarget, &Generation);
        if (Valid[i]) {
            if (Generation == Latest)
                return UDFJournalDiscoveryUnavailable(Vcb, Required);
            if (Generation > Latest) {
                Latest = Generation;
                Vcb->LvidJournalLatestSlot = (uint8)i;
            }
        }
    }
    if (!Valid[0] && !Valid[1] && (!Empty[0] || !Empty[1]))
        return UDFJournalDiscoveryUnavailable(Vcb, Required);
    Vcb->LvidJournalGeneration = Latest;
    if (!Latest) Vcb->LvidJournalLatestSlot = 1; /* first write uses slot 0 */
    Vcb->LvidJournalState = 1;
    UDFPrint(("UDF: LVID journal slots %x/%x generation %I64u\n",
              Vcb->LvidJournalSlot[0], Vcb->LvidJournalSlot[1], Latest));
    return STATUS_SUCCESS;
}

NTSTATUS
UDFUpdateLogicalVolInt(
    PIRP_CONTEXT IrpContext,
    PVCB            Vcb,
    BOOLEAN         Close
    )
{
    NTSTATUS    RC = STATUS_SUCCESS;
    uint32      i, len;
    SIZE_T      WrittenBytes;
    SIZE_T      BackupWrittenBytes = 0;
    int8        JournalSector[512];
    int8        JournalCheck[512];
    ULONGLONG   JournalGeneration = 0;
    uint32      JournalSlot = 0, JournalTarget = 0, JournalFlags = 0;
    uint8       JournalIndex = 0;
//    uint32      lvid_count = 0;
    uint32      pSize;
    tag*        PTag;
    struct LogicalVolIntegrityDesc *lvid;
    struct LogicalVolIntegrityDescImpUse* LVID_iUse;
    struct LogicalVolHeaderDesc* LVID_hd;
    uint32*     partFreeSpace;
    uint32*     partSize;
    BOOLEAN     equal = FALSE;
    LONGLONG NtTime;
    uint32 OldIntegrityType;
    ULONGLONG CheckpointUniqueId;

    if (Vcb->CDR_Mode)
        return STATUS_SUCCESS;
    if (!Vcb->LVid) {
        return STATUS_UNSUCCESSFUL;
    }

    UDFPrint(("UDF: Updating LVID @%x (%x)\n", Vcb->LVid_loc.extLocation, Vcb->LVid_loc.extLength));
    len = max(Vcb->LVid_loc.extLength, Vcb->SectorSize);
    lvid = Vcb->LVid;
    if (lvid->descTag.tagSerialNum > UDF_LVID_TTL) {
        // TODO: allocate space for new LVID
    }

    LVID_iUse = UDFGetLVIDiUse(Vcb);

    if ((LVID_iUse->minUDFReadRev  == Vcb->minUDFReadRev) &&
       (LVID_iUse->minUDFWriteRev == Vcb->minUDFWriteRev) &&
       (LVID_iUse->maxUDFWriteRev == Vcb->maxUDFWriteRev) &&
       (LVID_iUse->numFiles == Vcb->numFiles) &&
       (LVID_iUse->numDirs  == Vcb->numDirs))
        equal = TRUE;

    LVID_iUse->minUDFReadRev  = Vcb->minUDFReadRev;
    LVID_iUse->minUDFWriteRev = Vcb->minUDFWriteRev;
    LVID_iUse->maxUDFWriteRev = Vcb->maxUDFWriteRev;

    LVID_iUse->numFiles = Vcb->numFiles;
    LVID_iUse->numDirs  = Vcb->numDirs;

    UDFSetEntityID_imp(&(LVID_iUse->impIdent), UDF_ID_DEVELOPER);

    if (Close){
        UDFPrint(("UDF: Opening LVID\n"));
        lvid->integrityType = INTEGRITY_TYPE_CLOSE;
    } else {
        UDFPrint(("UDF: Closing LVID\n"));
        lvid->integrityType = INTEGRITY_TYPE_OPEN;
    }

    equal = equal && (Vcb->IntegrityType == lvid->integrityType);

    // update Free Space Table
    partFreeSpace = (uint32*)(lvid+1);
    for(i=0; i<lvid->numOfPartitions; i++) {
        pSize = UDFGetPartFreeSpace(Vcb, i);
        equal = equal && (partFreeSpace[i] == pSize);
        partFreeSpace[i] = pSize;
    }

    // Metadata growth changes the logical partition size independently of
    // its free count. Include SizeTable in the unchanged-descriptor test.
    if (Vcb->MetadataFileInfo &&
        Vcb->MetadataRef < lvid->numOfPartitions &&
        Vcb->MetadataRef < Vcb->PartitionMaps) {
        partSize = partFreeSpace + lvid->numOfPartitions;
        pSize = Vcb->Partitions[Vcb->MetadataRef].PartitionLen;
        equal = equal && (partSize[Vcb->MetadataRef] == pSize);
        partSize[Vcb->MetadataRef] = pSize;
    }

    // Update LVID Header Descriptor
    LVID_hd = (struct LogicalVolHeaderDesc*)&(lvid->logicalVolContentsUse);
    /* Every journal checkpoint, including CLOSED, must retain the durable
     * exclusive ID bound. A torn final CLOSED sector may still contain the
     * preceding OPEN body; retreating the ID bound would make that transition
     * indistinguishable from an invalid rollback to recovery. */
    CheckpointUniqueId = Vcb->NextUniqueId;
    if (Vcb->LvidJournalState == 1 &&
        CheckpointUniqueId < Vcb->UniqueIdReservedEnd)
        CheckpointUniqueId = Vcb->UniqueIdReservedEnd;
    equal = equal && (LVID_hd->uniqueID == CheckpointUniqueId);
    LVID_hd->uniqueID = CheckpointUniqueId;

    if (equal) {
        UDFPrint(("UDF: equal Ids\n"));
        return STATUS_SUCCESS;
    }

    RC = UDFDiscoverLvidJournal(IrpContext, Vcb);
    if (!NT_SUCCESS(RC)) {
        Vcb->VcbState |= VCB_STATE_VOLUME_READ_ONLY;
        return RC;
    }

    KeQuerySystemTime((PLARGE_INTEGER)&NtTime);
    UDFTimeToUDF(NtTime, &(lvid->recordingDateAndTime));

    PTag = &(lvid->descTag);
    if (Vcb->LvidJournalState == 1) {
        if (sizeof(struct LogicalVolIntegrityDesc) +
            sizeof(uint32) * 2 * lvid->numOfPartitions +
            sizeof(struct LogicalVolIntegrityDescImpUse) != 142 ||
            len != 512 ||
            Vcb->LvidJournalGeneration == (ULONGLONG)-1) {
            Vcb->VcbState |= VCB_STATE_VOLUME_READ_ONLY;
            return STATUS_FILE_CORRUPT_ERROR;
        }
        JournalGeneration = Vcb->LvidJournalGeneration + 1;
        JournalIndex = (uint8)(1 - Vcb->LvidJournalLatestSlot);
        JournalSlot = Vcb->LvidJournalSlot[JournalIndex];
        JournalTarget = Vcb->LvidJournalTarget;
        lvid->lengthOfImpUse = UDF_LVID_JOURNAL_IMP_LENGTH;
        RtlCopyMemory((uint8*)lvid + 142, UdfLvidJournalMagic, 8);
        RtlCopyMemory((uint8*)lvid + 150, &JournalGeneration,
                      sizeof(JournalGeneration));
        RtlCopyMemory((uint8*)lvid + 158, &JournalTarget,
                      sizeof(JournalTarget));
        RtlCopyMemory((uint8*)lvid + 162, &JournalFlags,
                      sizeof(JournalFlags));
        UDFSetUpTag(Vcb, PTag, UDF_LVID_JOURNAL_TOTAL_LENGTH,
                    PTag->tagLocation, 0);
        RtlCopyMemory(JournalSector, lvid, sizeof(JournalSector));
        ((struct LogicalVolIntegrityDesc*)JournalSector)->integrityType =
            INTEGRITY_TYPE_OPEN;
        UDFSetUpTag(Vcb, (tag*)JournalSector,
                    UDF_LVID_JOURNAL_TOTAL_LENGTH, JournalSlot, 0);
    } else {
        lvid->lengthOfImpUse =
            sizeof(struct LogicalVolIntegrityDescImpUse);
        UDFSetUpTag(Vcb, PTag,
            (uint16)(sizeof(struct LogicalVolIntegrityDesc) +
            sizeof(uint32)*2*lvid->numOfPartitions +
            sizeof(struct LogicalVolIntegrityDescImpUse)),
            PTag->tagLocation, 0);
    }

    OldIntegrityType = Vcb->IntegrityType;
    Vcb->IntegrityType = INTEGRITY_TYPE_OPEN; // make happy auto-dirty
    if (Vcb->LvidJournalState == 1) {
        /* A durable OPEN slot always precedes the potentially torn final
         * sector write. Rotate slots so a failed slot update cannot erase
         * the preceding valid checkpoint. */
        RC = UDFWriteSectors(IrpContext, Vcb, TRUE, JournalSlot, 1,
                             FALSE, JournalSector, &BackupWrittenBytes);
        if (NT_SUCCESS(RC) && BackupWrittenBytes != sizeof(JournalSector))
            RC = STATUS_IO_DEVICE_ERROR;
        if (NT_SUCCESS(RC))
            RC = UDFBarrierFlushDevice(Vcb->TargetDeviceObject);
        if (NT_SUCCESS(RC)) {
            RC = UDFReadSectors(IrpContext, Vcb, FALSE, JournalSlot,
                                1, FALSE, JournalCheck);
            if (NT_SUCCESS(RC) &&
                RtlCompareMemory(JournalCheck, JournalSector,
                                 sizeof(JournalSector)) !=
                    sizeof(JournalSector))
                RC = STATUS_IO_DEVICE_ERROR;
        }
        if (!NT_SUCCESS(RC)) {
            Vcb->IntegrityType = OldIntegrityType;
            Vcb->VcbState |= VCB_STATE_VOLUME_READ_ONLY;
            return RC;
        }
        Vcb->LvidJournalGeneration = JournalGeneration;
        Vcb->LvidJournalLatestSlot = JournalIndex;
    }
    RC = UDFWriteSectors(IrpContext, Vcb, TRUE, PTag->tagLocation, len >> Vcb->SectorShift, FALSE, (int8*)(lvid), &WrittenBytes);
    // update it here to prevent recursion
    Vcb->IntegrityType = NT_SUCCESS(RC) && WrittenBytes == len ?
                         lvid->integrityType : OldIntegrityType;

    if (NT_SUCCESS(RC) && WrittenBytes != len)
        RC = STATUS_IO_DEVICE_ERROR;
    if (!NT_SUCCESS(RC) && Vcb->LvidJournalState == 1)
        Vcb->VcbState |= VCB_STATE_VOLUME_READ_ONLY;

    return RC;
} // end UDFUpdateLogicalVolInt()

/*
    This routine reads all sparing tables & stores them in contiguos memory
    space
 */
NTSTATUS
UDFUpdateSparingTable(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )
{
    PSPARING_MAP RelocMap;
//    PSPARING_MAP NewRelocMap;
    NTSTATUS status = STATUS_SUCCESS;
    NTSTATUS status2 = STATUS_SUCCESS;
    uint32 i=0, BC, BC2;
    PSPARING_TABLE SparTable;
//    uint32 n,m;
//    BOOLEAN merged;
    BOOLEAN sorted;

    UDFPrint(("UDF: Updating Sparable Part Map:\n"));
    if (!Vcb->SparingTableModified) return STATUS_SUCCESS;
    if (!Vcb->SparingTable) return STATUS_SUCCESS;

    BC = (Vcb->SparingTableLength >> Vcb->SectorShift) + 1;
    SparTable = (PSPARING_TABLE)MyAllocatePool__(NonPagedPool, BC*Vcb->SectorSize);
    if (!SparTable) return STATUS_INSUFFICIENT_RESOURCES;
    // if a part of Sparing Table is already loaded,
    // update it with data from another one
    RelocMap = Vcb->SparingTable;
    // sort sparing table
    //merged = FALSE;
    do {
        sorted = FALSE;
        for(i=1;i<Vcb->SparingCount;i++) {
            if (RelocMap[i-1].origLocation > RelocMap[i].origLocation) {
                XCHG_DD(RelocMap[i-1].origLocation,   RelocMap[i].origLocation);
swp_loc:
                XCHG_DD(RelocMap[i-1].mappedLocation, RelocMap[i].mappedLocation);
                //merged = TRUE;
                sorted = TRUE;
            } else
            if (RelocMap[i-1].origLocation == SPARING_LOC_AVAILABLE &&
               RelocMap[i].origLocation   == SPARING_LOC_AVAILABLE &&
               RelocMap[i-1].mappedLocation > RelocMap[i].mappedLocation) {
                goto swp_loc;
            }
        }
    } while(sorted);

    for(i=0;i<Vcb->SparingCount;i++) {
        UDFPrint(("  @%x -> %x \n",
            RelocMap[i].origLocation, RelocMap[i].mappedLocation));
    }

    Vcb->SparingTableModified = FALSE;
//    if (!merged) {
//        UDFPrint(("  sparing table unchanged\n"));
//        MyFreePool__(SparTable);
//        return STATUS_SUCCESS;
//    }

    // walk through all available Sparing Tables
    for(i=0;i<Vcb->SparingTableCount;i++) {
        // read (next) table
        UDFPrint(("  sparing table @%x\n", Vcb->SparingTableLoc[i]));
        status = UDFReadSectors(IrpContext, Vcb, FALSE, Vcb->SparingTableLoc[i], 1, FALSE, (int8*)SparTable);
        // tag should be set to TID_UNUSED_DESC
        if (NT_SUCCESS(status) && (SparTable->descTag.tagIdent == TID_UNUSED_DESC)) {

            BC2 = ((sizeof(SPARING_TABLE) +
                    SparTable->reallocationTableLen*sizeof(SparingEntry) +
                    Vcb->SectorSize-1)
                                      >> Vcb->SectorShift);
            if (BC2 > BC) {
                UDFPrint((" sizeSparingTable @%x too long: %x > %x\n",
                    Vcb->SparingTableLoc[i], BC2, BC
                    ));
                continue;
            }
            status = UDFReadSectors(IrpContext, Vcb, FALSE, Vcb->SparingTableLoc[i],
                BC2, FALSE, (int8*)SparTable);

            if (!NT_SUCCESS(status)) {
                UDFPrint((" Error reading sizeSparingTable @%x (%x)\n",
                    Vcb->SparingTableLoc[i], BC2
                    ));
                continue;
            }

            BC2 = ((sizeof(SPARING_TABLE) +
                    Vcb->SparingCount*sizeof(SparingEntry) +
                    Vcb->SectorSize-1)
                                      >> Vcb->SectorShift);
            if (BC2 > BC) {
                UDFPrint((" new sizeSparingTable @%x too long: %x > %x\n",
                    Vcb->SparingTableLoc[i], BC2, BC
                    ));
                continue;
            }

            SparTable->reallocationTableLen = (USHORT)Vcb->SparingCount;
            RtlCopyMemory((SparTable+1), RelocMap, Vcb->SparingCount*sizeof(SparingEntry));

            //  Convert origLocation back from physical (PSN) to
            //  partition-relative (LBN) for on-disk format.

            {
                PSPARING_MAP DiskMap = (PSPARING_MAP)(SparTable+1);
                uint32 PartRoot = 0;
                for (uint32 p = 0; p < Vcb->PartitionMaps; p++) {
                    if (Vcb->Partitions[p].PartitionType == UDF_SPARABLE_MAP15) {
                        PartRoot = Vcb->Partitions[p].PartitionRoot;
                        break;
                    }
                }
                for (uint32 s = 0; s < Vcb->SparingCount; s++, DiskMap++) {
                    if (DiskMap->origLocation != SPARING_LOC_AVAILABLE &&
                        DiskMap->origLocation != SPARING_LOC_CORRUPTED) {
                        DiskMap->origLocation -= PartRoot;
                    }
                }
            }
/*
            merged = FALSE;
            NewRelocMap = (PSPARING_MAP)(SparTable+1);
            for(n=0; n<SparTable->reallocationTableLen; n++) {
                for(m=0; m<Vcb->SparingCount; m++) {
                    if (RelocMap[m].mappedLocation == NewRelocMap[n].mappedLocation) {
                        if (RelocMap[m].origLocation != NewRelocMap[n].origLocation) {
                            UDFPrint(("  update @%x (%x) -> @%x (%x)\n",
                                NewRelocMap[m].origLocation, NewRelocMap[m].mappedLocation,
                                RelocMap[m].origLocation, RelocMap[m].mappedLocation));
                            merged = TRUE;
                        }
                    }
                }
            }
*/
//            if (merged) {
            UDFPrint(("UDF: record updated\n"));
            SIZE_T WrittenBytes;
            status = UDFWriteSectors(IrpContext, Vcb, FALSE, Vcb->SparingTableLoc[i], BC2, FALSE, (int8*)SparTable, &WrittenBytes);
            if (!NT_SUCCESS(status)) {
                if (!NT_SUCCESS(status2)) {
                    status2 = status;
                }
//                }
            }
        }
    }
    MyFreePool__(SparTable);
    if (!NT_SUCCESS(status2)) {
        status = status2;
    }
    return status;
} // end UDFUpdateSparingTable()

/*
    update Logical volume descriptor
 */
NTSTATUS
UDFUpdateLogicalVol(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB            Vcb,
    IN UDF_VDS_RECORD  Lba,
    IN PUNICODE_STRING VolIdent
    )
{
    struct LogicalVolDesc* lvd = NULL;
#define CUR_IDENT_SZ (sizeof(lvd->logicalVolIdent))
    dstring CS0[CUR_IDENT_SZ];
    uint16 ident;
    SIZE_T WrittenBytes;
    NTSTATUS status = STATUS_SUCCESS;
//    NTSTATUS status2 = STATUS_SUCCESS;

    status = UDFUpdateSparingTable(IrpContext, Vcb);
    if (!NT_SUCCESS(status))
        return status;

    lvd = (struct LogicalVolDesc*)MyAllocatePool__(NonPagedPool, max(Vcb->SectorSize, sizeof(struct LogicalVolDesc)) );

    if (!lvd) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Err_SetVI;
    }

    UDFPrint(("UDF: Updating LVD @%x (%x)\n", Lba.block, Vcb->SectorSize));

    status = UDFSetDstring(&(Vcb->VolIdent), (dstring*)&CS0, CUR_IDENT_SZ);
    if (!NT_SUCCESS(status)) {
        if (status == STATUS_INVALID_PARAMETER) {
            status = STATUS_INVALID_VOLUME_LABEL;
        }
        goto Err_SetVI;
    }

    if (!Lba.block) {
        status = STATUS_INVALID_PARAMETER;
        goto Err_SetVI;
    }
    status = UDFReadTagged(IrpContext, Vcb, (int8*)lvd, Lba.block, Lba.block, &ident);
    if (!NT_SUCCESS(status)) goto Err_SetVI;
    if (ident != TID_LOGICAL_VOL_DESC) {
        status = STATUS_FILE_CORRUPT_ERROR;
        goto Err_SetVI;
    }

    if (RtlCompareMemory(lvd->logicalVolIdent, CS0, CUR_IDENT_SZ) == CUR_IDENT_SZ) {
        // no changes
        UDFPrint(("UDF: equal VolIds\n"));
        status = STATUS_SUCCESS;
        goto Err_SetVI;
    }
    RtlCopyMemory(lvd->logicalVolIdent, CS0, CUR_IDENT_SZ);

    lvd->descTag.tagSerialNum --;
    UDFSetUpTag(Vcb, (tag*)lvd, lvd->descTag.descCRCLength + sizeof(tag), Lba.block, 0);

    status = UDFWriteSectors(IrpContext, Vcb, TRUE, Lba.block, 1, FALSE, (int8*)lvd, &WrittenBytes);

Err_SetVI:
    if (lvd)
        MyFreePool__(lvd);

#undef CUR_IDENT_SZ
//#endif //0

    return status;
} // end UDFUpdateLogicalVol()

/*
    This routine updates volume descriptor sequence
 */
NTSTATUS
UDFUpdateVDS(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN uint32 block,
    IN uint32 lastblock,
    IN uint32 flags
    )
{
    NTSTATUS status;
    int8*    Buf = (int8*)DbgAllocatePool(NonPagedPool,Vcb->SectorSize);
    UDF_VDS_RECORD vds[VDS_POS_LENGTH];
    uint32 i,j;
    uint16 ident;

    if (!Buf) return STATUS_INSUFFICIENT_RESOURCES;
    RtlZeroMemory(vds, sizeof(UDF_VDS_RECORD) * VDS_POS_LENGTH);
    if (!NT_SUCCESS(status = UDFReadVDS(IrpContext, Vcb, block, lastblock, (PUDF_VDS_RECORD)&vds, Buf))) {
        DbgFreePool(Buf);
        return status;
    }

/*
    // update USD (if any)
    for (i=0; i<VDS_POS_LENGTH; i++) {
        if (vds[i].block) {
            status = UDFReadTagged(Vcb, Buf, vds[i].block, vds[i].block, &ident);
            if (NT_SUCCESS(status) && (i == VDS_POS_PARTITION_DESC)) {
                // load partition descriptor(s)
                int8*  Buf2 = (int8*)DbgAllocatePool(NonPagedPool,Vcb->BlockSize);
                if (!Buf2) {
                    DbgFreePool(Buf);
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                for (j=vds[i].block+1; j<vds[VDS_POS_TERMINATING_DESC].block; j++) {
                    UDFReadTagged(Vcb,Buf2, j, j, &ident);
                    if (ident == TID_UNALLOC_SPACE_DESC)
                        // This implememtation doesn't support USD ;) recording
                        // So, we'll make'em blank, but record all bitmaps
                        UDFUpdateUSpaceDesc(Vcb,Buf2);
                }
                DbgFreePool(Buf2);
                break;
            }
        }
    }*/
    for (i=0; i<VDS_POS_LENGTH; i++) {
        if (vds[i].block) {
            status = UDFReadTagged(IrpContext, Vcb, Buf, vds[i].block, vds[i].block, &ident);
            if (!NT_SUCCESS(status))
                break;
            // update XBMs
            if (i == VDS_POS_PARTITION_DESC) {
                if (!(flags & 1))
                    continue;
                // update partition descriptor(s)
                int8*  Buf2 = (int8*)DbgAllocatePool(NonPagedPool,Vcb->SectorSize);
                if (!Buf2) {
                    DbgFreePool(Buf);
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                status = UDFUpdatePartDesc(IrpContext, Vcb, Buf);
                if (!NT_SUCCESS(status)) {
                    DbgFreePool(Buf2);
                    break;
                }
                for (j=vds[i].block+1; j<vds[VDS_POS_TERMINATING_DESC].block; j++) {
                    status = UDFReadTagged(IrpContext, Vcb,Buf2, j, j, &ident);
                    if (!NT_SUCCESS(status))
                        break;
                    if (ident == TID_PARTITION_DESC) {
                        status = UDFUpdatePartDesc(IrpContext, Vcb, Buf2);
                        if (!NT_SUCCESS(status))
                            break;
                    }
                }
                DbgFreePool(Buf2);
                if (!NT_SUCCESS(status))
                    break;
//                continue;
            } else
            // update Vol Ident Desc when volume label was changed
            if (Vcb->VolIdentChanged && (i == VDS_POS_LOGICAL_VOL_DESC)) {
                status = UDFUpdateLogicalVol(IrpContext, Vcb, vds[VDS_POS_LOGICAL_VOL_DESC], &Vcb->VolIdent);
                if (!NT_SUCCESS(status))
                    break;
            }
        }
    }

    DbgFreePool(Buf);
    return status;
} // end UDFUpdateVDS()

NTSTATUS
__fastcall
UDFSetDstring(
    IN PUNICODE_STRING UName,
    IN dstring* Dest,
    IN uint32 Length
    )
{
    uint8* CS0;
    SIZE_T len = Length-1;

    // A zero length string shall be recorded by a Dstring with all zeros bytes
    if (UName->Length == 0) {
        RtlZeroMemory(Dest, Length);
        return STATUS_SUCCESS;
    }

    UDFCompressUnicode(UName, &CS0, &len);
    if (!CS0)
        return STATUS_INSUFFICIENT_RESOURCES;
    if (len > Length-1) {
        MyFreePool__(CS0);
        return STATUS_INVALID_PARAMETER;
    }
    RtlCopyMemory(Dest, CS0, len);
    MyFreePool__(CS0);
    if (len < Length-1)
        RtlZeroMemory(Dest+len, Length-1-len);
    Dest[Length-1] = (uint8)len;
    return STATUS_SUCCESS;
} // end UDFSetDstring()

void
__fastcall
UDFGetDstring(
    IN OUT PUNICODE_STRING UName,
    IN dstring* Dest,
    IN uint32 Length
    )
{
    uint32 len = Dest[Length-1];

    UDFDecompressUnicode(UName, Dest, len, NULL);
    return;
} // end UDFGetDstring()

/*
    This routine updates Volume Label & some other features stored in
    VolIdentDesc
 */
NTSTATUS
UDFUpdateVolIdent(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN UDF_VDS_RECORD Lba,
    IN PUNICODE_STRING VolIdent
    )
{
#define CUR_IDENT_SZ (sizeof(pvoldesc->volIdent))
    struct PrimaryVolDesc* pvoldesc = (struct PrimaryVolDesc*)MyAllocatePool__(NonPagedPool, max(Vcb->SectorSize, sizeof(struct PrimaryVolDesc)) );
    NTSTATUS status;
    dstring CS0[CUR_IDENT_SZ];
    uint16 ident;
    SIZE_T WrittenBytes;

    if (!pvoldesc) return STATUS_INSUFFICIENT_RESOURCES;

    UDFPrint(("UDF: Updating PVD @%x (%x)\n", Lba.block, Vcb->SectorSize));

    status = UDFSetDstring(&(Vcb->VolIdent), (dstring*)&CS0, CUR_IDENT_SZ);
    if (!NT_SUCCESS(status)) {
        if (status == STATUS_INVALID_PARAMETER) {
            status = STATUS_INVALID_VOLUME_LABEL;
        }
        goto Err_SetVI;
    }

    if (!Lba.block) {
        status = STATUS_INVALID_PARAMETER;
        goto Err_SetVI;
    }
    status = UDFReadTagged(IrpContext, Vcb, (int8*)pvoldesc, Lba.block, Lba.block, &ident);
    if (!NT_SUCCESS(status)) goto Err_SetVI;
    if (ident != TID_PRIMARY_VOL_DESC) {
        status = STATUS_FILE_CORRUPT_ERROR;
        goto Err_SetVI;
    }

    if (RtlCompareMemory(pvoldesc->volIdent, CS0, CUR_IDENT_SZ) == CUR_IDENT_SZ) {
        // no changes
        status = STATUS_SUCCESS;
        goto Err_SetVI;
    }
    RtlCopyMemory(pvoldesc->volIdent, CS0, CUR_IDENT_SZ);

    pvoldesc->descTag.tagSerialNum --;
    UDFSetUpTag(Vcb, (tag*)pvoldesc, pvoldesc->descTag.descCRCLength + sizeof(tag), Lba.block, 0);

    status = UDFWriteSectors(IrpContext, Vcb, TRUE, Lba.block, 1, FALSE, (int8*)pvoldesc, &WrittenBytes);
Err_SetVI:
    MyFreePool__(pvoldesc);
    return status;

#undef CUR_IDENT_SZ
} // end UDFUpdateVolIdent()

NTSTATUS
UDFUpdateNonAllocated(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )
{
    uint32 RefPartNum;
    uint32 i;
    uint32 plen, pstart, pend;
    int8* bad_bm;
    EXTENT_AD Ext;
    PEXTENT_MAP Map = NULL;
    PEXTENT_INFO DataLoc;
    NTSTATUS status;

    UDFPrint(("UDFUpdateNonAllocated:\n"));
    if (!Vcb->NonAllocFileInfo) {
        return STATUS_SUCCESS;
    }
    if (!(bad_bm = Vcb->BSBM_Bitmap)) {
        return STATUS_SUCCESS;
    }

    DataLoc = &(Vcb->NonAllocFileInfo->Dloc->DataLoc);
    ASSERT(!DataLoc->Offset);
    if (Vcb->NonAllocFileInfo->Dloc->DataLoc.Offset) {
        UDFPrint(("NonAllocFileInfo in IN_ICB mode !!!\n"));
        return STATUS_SUCCESS;
    }
    RefPartNum = UDFGetRefPartNumByPhysLba(Vcb, Vcb->NonAllocFileInfo->Dloc->FELoc.Mapping[0].extLocation);
    pstart = UDFPartStart(Vcb, RefPartNum);
    plen = UDFPartLen(Vcb, RefPartNum);
    // Bitmap is LBN-indexed (0..PartitionLen)
    pend = min(plen, Vcb->FSBM_BitCount);

    //BrutePoint();
    for(i=0; i<pend; i++) {
        if (!UDFGetBadBit(bad_bm, i))
            continue;
        // add BAD blocks to unallocatable space
        // Convert LBN back to PSN for extent operations
        uint32 psn = i + pstart;
        // if the block is already in NonAllocatable, ignore it
        if (UDFLocateLbaInExtent(Vcb, DataLoc->Mapping, psn) != LBA_OUT_OF_EXTENT) {
            UDFPrint(("lba %#x is already in NonAllocFileInfo\n", psn));
            continue;
        }
        UDFPrint(("add lba %#x to NonAllocFileInfo\n", psn));
        DataLoc->Modified = TRUE;
        Ext.extLength = Vcb->SectorSize;
        Ext.extLocation = psn;
        Map = UDFExtentToMapping(&Ext);
        DataLoc->Mapping = UDFMergeMappings(DataLoc->Mapping, Map);
    }
    UDFPackMapping(Vcb, DataLoc);
    DataLoc->Length = UDFGetExtentLength(DataLoc->Mapping);
    status = UDFFlushFile__(IrpContext, Vcb, Vcb->NonAllocFileInfo, 0);
    if (!NT_SUCCESS(status))
        return status;

    // ensure that BAD space is marked as USED
    UDFMarkSpaceAsXXX(Vcb, 0, &(DataLoc->Mapping[0]), AS_USED); // mark as used

    UDFPrint(("UDFUpdateNonAllocated: done\n"));
    return STATUS_SUCCESS;
} // end UDFUpdateNonAllocated()

/*
    This routine rebuilds & flushes all system areas
 */
NTSTATUS
UDFUmount__(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    OUT PBOOLEAN CloseDeferred
    )
{
    uint32 flags = 0;
    NTSTATUS status;

    *CloseDeferred = FALSE;
    if ((Vcb->VcbState & VCB_STATE_VOLUME_READ_ONLY)
        || !Vcb->Modified)
        return STATUS_SUCCESS;
    /* Some metadata-only paths mark the VCB modified before issuing a
     * sector write. Open and persist the LVID before flushing their data. */
    status = UDFOpenIntegrityForWrite(IrpContext, Vcb, FALSE);
    if (!NT_SUCCESS(status))
        return status;
    // prevent discarding metadata
    Vcb->VcbState |= UDF_VCB_ASSUME_ALL_USED;
    if (Vcb->CDR_Mode) {

        // record VAT
        return UDFRecordVAT(IrpContext, Vcb);
    }

    /* Mount may have accepted only the main or only the reserve sequence.
     * An absent copy has a zero range, not a VDS at physical block zero.
     * At least one validated sequence is still required for a clean close. */
    if (!Vcb->VDS1_Len && !Vcb->VDS2_Len) {
        status = STATUS_FILE_CORRUPT_ERROR;
        goto clear_assume;
    }

    status = UDFFlushAllCachedAllocations(Vcb, UDF_PREALLOC_CLASS_DIR);
    if (!NT_SUCCESS(status))
        goto clear_assume;

    // synchronize BAD Block bitmap and NonAllocatable
    status = UDFUpdateNonAllocated(IrpContext, Vcb);
    if (!NT_SUCCESS(status))
        goto clear_assume;

    UDFAcquireResourceExclusive(&(Vcb->BitMapResource1),TRUE);

    // RAM mode
    if (Vcb->VolIdentChanged) {
        if (Vcb->VDS1_Len) {
            status = UDFUpdateVolIdent(IrpContext, Vcb, Vcb->PVolDescAddr, &Vcb->VolIdent);
            if (!NT_SUCCESS(status))
                goto release_bitmap;
        }
        if (Vcb->VDS2_Len) {
            status = UDFUpdateVolIdent(IrpContext, Vcb, Vcb->PVolDescAddr2, &Vcb->VolIdent);
            if (!NT_SUCCESS(status))
                goto release_bitmap;
        }
    }

    UDF_CHECK_BITMAP_RESOURCE(Vcb);
    // check if we should update BM
    if (Vcb->BitmapFcb) {
        // Per-page mode: use BitmapModified flag
        if (Vcb->BitmapModified) {
            flags |= 1;
        } else {
            flags &= ~1;
        }
    } else if (Vcb->FSBM_Bitmap && Vcb->FSBM_OldBitmap) {
        if (Vcb->FSBM_ByteCount == RtlCompareMemory(Vcb->FSBM_Bitmap, Vcb->FSBM_OldBitmap, Vcb->FSBM_ByteCount)) {
            flags &= ~1;
        } else {
            flags |= 1;
        }
    }

    /* Make the file entries and namespace drained by UDFFlushVolume durable
     * before UDFUpdatePartDesc writes either the RAM bitmap or dirty cache
     * pages. Otherwise an explicit close can publish allocated blocks while
     * their FE/FID owner is still only in the lower device's volatile cache.
     * Per-page lazy writeback before this checkpoint remains a separate case. */
    if (flags & 1) {
        status = UDFBarrierFlushDevice(Vcb->TargetDeviceObject);
        if (!NT_SUCCESS(status))
            goto release_bitmap;
    }

    if (Vcb->VDS1_Len) {
        status = UDFUpdateVDS(IrpContext, Vcb, Vcb->VDS1, Vcb->VDS1 + Vcb->VDS1_Len, flags);
        if (!NT_SUCCESS(status))
            goto release_bitmap;
    }
    if (Vcb->VDS2_Len) {
        status = UDFUpdateVDS(IrpContext, Vcb, Vcb->VDS2, Vcb->VDS2 + Vcb->VDS2_Len, flags);
        if (!NT_SUCCESS(status))
            goto release_bitmap;
    }

    /* A deleted FCB can outlive its last handle while Cache Manager owns a
     * FILE_OBJECT.  Its prepared disk FREE bits stay overlaid during the
     * bitmap writes above; new allocations are now durable too.  Complete
     * the requested flush, but leave LVID OPEN and Modified set until the
     * pending FCB finally releases its RAM reservation. */
    if (Vcb->DeletePrefreeList) {
        status = UDFBarrierFlushDevice(Vcb->TargetDeviceObject);
        if (NT_SUCCESS(status))
            *CloseDeferred = TRUE;
        goto release_bitmap;
    }

    // Update Integrity Desc if any
    if (Vcb->LVid && Vcb->origIntegrityType == INTEGRITY_TYPE_CLOSE) {
        status = UDFBarrierFlushDevice(Vcb->TargetDeviceObject);
        if (!NT_SUCCESS(status))
            goto release_bitmap;
        status = UDFUpdateLogicalVolInt(IrpContext, Vcb, TRUE);
        if (!NT_SUCCESS(status))
            goto release_bitmap;
        /* Do not report a clean volume while its CLOSED LVID may still be
         * only in the lower device's volatile cache. This also covers
         * internal lock/dismount callers without an outer flush IRP. */
        status = UDFBarrierFlushDevice(Vcb->TargetDeviceObject);
        if (!NT_SUCCESS(status)) {
            Vcb->VcbState |= VCB_STATE_VOLUME_READ_ONLY;
            goto release_bitmap;
        }
    }

    if (flags & 1) {
        if (!Vcb->BitmapFcb && Vcb->FSBM_Bitmap && Vcb->FSBM_OldBitmap) {
            RtlCopyMemory(Vcb->FSBM_OldBitmap, Vcb->FSBM_Bitmap, Vcb->FSBM_ByteCount);
        }
        // Per-page mode: no OldBitmap copy needed, BitmapModified cleared after flush
        Vcb->BitmapModified = FALSE;
    }

    Vcb->VolIdentChanged = FALSE;

//skip_update_bitmap:

release_bitmap:
    UDFReleaseResource(&(Vcb->BitMapResource1));

clear_assume:
    Vcb->VcbState &= ~UDF_VCB_ASSUME_ALL_USED;
    return status;
} // end UDFUmount__()

/*************************************************************************/

/*
    Find an anchor volume descriptor.
    The UDFGetDiskInfoAndVerify() will invoke this routine to find & check
    Anchor Volume Descriptors on the target device
*/
lba_t
UDFFindAnchorVolumeDescriptor(
    IN PIRP_CONTEXT IrpContext,
    PVCB Vcb
    )
{
//    NTSTATUS    RC = STATUS_SUCCESS;

    uint16 ident;
    uint32 i;
    uint32 LastBlock;
    NTSTATUS status;

    int8* Buf = (int8*)MyAllocatePool__(NonPagedPool, ROUND_TO_PAGES(SectorAlign(Vcb, sizeof(struct AnchorVolDescPtr))));

    if (!Buf)
        return 0;

    UDFPrint(("UDFFindAnchor\n"));
    // init probable locations...
    RtlZeroMemory(&(Vcb->Anchor), sizeof(Vcb->Anchor));
    Vcb->Anchor[0] = 256 + Vcb->SessionStartLba;
    Vcb->Anchor[1] = 512 + Vcb->SessionStartLba;
    Vcb->Anchor[2] = 256 + Vcb->TrackMap[Vcb->LastTrackNum].FirstLba;
    Vcb->Anchor[3] = 512 + Vcb->TrackMap[Vcb->LastTrackNum].FirstLba;
    Vcb->Anchor[4] = Vcb->SessionEndLba - 256;
    Vcb->Anchor[5] = Vcb->SessionEndLba - 256 + 1;
    Vcb->Anchor[6] = Vcb->SessionEndLba - 256 - 2;
    // vat locations
    Vcb->Anchor[7] = Vcb->SessionEndLba - 2;
    Vcb->Anchor[8] = Vcb->SessionEndLba;
    Vcb->Anchor[9] = Vcb->SessionEndLba - 512;

    LastBlock = 0;
    // ... and check them
    for (i=0; i<sizeof(Vcb->Anchor)/sizeof(int); i++) {
        if (Vcb->Anchor[i] > Vcb->SessionEndLba)
            Vcb->Anchor[i] = 0;

        if (Vcb->Anchor[i]) {
            UDFPrint(("check Anchor %x\n", Vcb->Anchor[i]));
            if (!NT_SUCCESS(status = UDFReadTagged(IrpContext, Vcb, Buf,
                Vcb->Anchor[i], Vcb->Anchor[i], &ident))) {

                Vcb->Anchor[i] = 0;
                if (status == STATUS_NONEXISTENT_SECTOR) {
                    UDFPrint(("UDF: disk seems to be incomplete\n"));
                    break;
                }
            } else {

                if ((ident != TID_ANCHOR_VOL_DESC_PTR) && ((i<6) ||
                    (ident != TID_FILE_ENTRY && ident != TID_EXTENDED_FILE_ENTRY))) {
                    Vcb->Anchor[i] = 0;
                } else {
                    UDFPrint(("UDF: Found AVD at %x (point %d)\n",Vcb->Anchor[i], i));
                    if (!LastBlock)
                        LastBlock = Vcb->SessionEndLba;
                }
            }
        }
    }

    UDFPrint(("UDF: -----------------\nUDF: Last block %x\n",LastBlock));
    MyFreePool__(Buf);
    return LastBlock;
} // end UDFFindAnchor()

/*
    Look for Volume recognition sequence
 */
uint32
UDFFindVRS(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
    )
{
    struct VolStructDesc  *vsd = NULL;
    uint32       i, offset, retStat = 0;
    uint32       Step = max(2048, Vcb->SectorSize) >> Vcb->SectorShift;
    NTSTATUS     RC;
    int8*        buffer = (int8*)MyAllocatePool__(NonPagedPool,Vcb->SectorSize);

    if (!buffer) return 0;
    // Relative to First LBA in Last Session
    offset = Vcb->SessionStartLba + (32768 >> Vcb->SectorShift);

    UDFPrint(("UDFFindVRS:\n"));

    // Process the sequence (if applicable)
    for (i = 0; i <= 0x10; i++, offset += Step) {
        // Read a block
        RC = UDFReadSectors(IrpContext, Vcb, FALSE, offset, 1, FALSE, buffer);
        if (!NT_SUCCESS(RC)) continue;

        // Look for ISO descriptors
        vsd = (struct VolStructDesc *)(buffer);

        if (vsd->stdIdent[0]) {
            if (!strncmp((int8*)(&vsd->stdIdent), STD_ID_CD001, STD_ID_LEN))
            {
                retStat |= VRS_ISO9660_FOUND;
                switch (vsd->structType)
                {
                    case 0:
                        UDFPrint(("UDF: ISO9660 Boot Record found\n"));
                        break;
                    case 1:
                        UDFPrint(("UDF: ISO9660 Primary Volume Descriptor found\n"));
                        break;
                    case 2:
                        UDFPrint(("UDF: ISO9660 Supplementary Volume Descriptor found\n"));
                        break;
                    case 3:
                        UDFPrint(("UDF: ISO9660 Volume Partition Descriptor found\n"));
                        break;
                    case 255:
                        UDFPrint(("UDF: ISO9660 Volume Descriptor Set Terminator found\n"));
                        break;
                    default:
                        UDFPrint(("UDF: ISO9660 VRS (%u) found\n", vsd->structType));
                        break;
                }
            }
            else if (!strncmp((int8*)(&vsd->stdIdent), STD_ID_BEA01, STD_ID_LEN))
            {
                UDFPrint(("UDF: BEA01 Found\n"));
            }
            else if (!strncmp((int8*)(&vsd->stdIdent), STD_ID_TEA01, STD_ID_LEN))
            {
                UDFPrint(("UDF: TEA01 Found\n"));
                break;
            }
            else if (!strncmp((int8*)(&vsd->stdIdent), STD_ID_NSR02, STD_ID_LEN))
            {
                retStat |= VRS_NSR02_FOUND;
                UDFPrint(("UDF: NSR02 Found\n"));
                break;
            }
            else if (!strncmp((int8*)(&vsd->stdIdent), STD_ID_NSR03, STD_ID_LEN))
            {
                retStat |= VRS_NSR03_FOUND;
                UDFPrint(("UDF: NSR03 Found\n"));
                break;
            }
        }
    }

    MyFreePool__(buffer);

    return retStat;
} // end UDFFindVRS()

static
BOOLEAN
UDFHasOtherFsBootSector(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb
)
{
    uint8* Buf = NULL;
    uint16 BytesPerSector = 0;
    BOOLEAN HasOther = FALSE;

    Buf = (uint8*)MyAllocatePool__(NonPagedPool, Vcb->SectorSize);
    
    if (!Buf) {
        return FALSE;
    }

    if (
        NT_SUCCESS(UDFReadSectors(IrpContext, Vcb, FALSE, Vcb->SessionStartLba, 1, FALSE, (int8*)Buf)) &&
        ((Buf[0] == 0xEB) || (Buf[0] == 0xE9)) &&
        (Buf[510] == 0x55) && (Buf[511] == 0xAA)
    ) {
        BytesPerSector = Buf[11] | (Buf[12] << 8);
        HasOther = 
                (RtlCompareMemory(Buf + 3, "EXFAT   ", 8) == 8) ||
                (RtlCompareMemory(Buf + 3, "NTFS    ", 8) == 8) ||
                ((BytesPerSector >= 512) && (BytesPerSector <= 4096) &&
                 !(BytesPerSector & (BytesPerSector - 1)) &&
                 Buf[13] && !(Buf[13] & (Buf[13] - 1)) &&
                 ((Buf[16] == 1) || (Buf[16] == 2)));
    }

    MyFreePool__(Buf);

    return HasOther;
}

/*
    process Primary volume descriptor
 */
void
UDFLoadPVolDesc(
    PVCB Vcb,
    int8* Buf // pointer to buffer containing PVD
    )
{
    struct PrimaryVolDesc *pvoldesc;
//    NTSTATUS    RC = STATUS_SUCCESS;

    pvoldesc = (struct PrimaryVolDesc *)Buf;
    UDFPrint(("UDF: PrimaryVolDesc:\n"));
    UDFPrint(("volDescSeqNum     = %d\n", pvoldesc->volDescSeqNum));
    UDFPrint(("primaryVolDescNum = %d\n", pvoldesc->primaryVolDescNum));
    // remember recording time...
    Vcb->VolCreationTime = UDFTimeToNT(&(pvoldesc->recordingDateAndTime));
    // ...VolIdent...
#define CUR_IDENT_SZ (sizeof(pvoldesc->volIdent))
    if (Vcb->VolIdent.Buffer) {
        MyFreePool__(Vcb->VolIdent.Buffer);
    }
    UDFGetDstring(&(Vcb->VolIdent), (dstring*)&(pvoldesc->volIdent), CUR_IDENT_SZ);
#undef CUR_IDENT_SZ
    UDFPrint(("volIdent[] = '%wZ'\n", &Vcb->VolIdent));
#ifdef UDF_DBG
    UDFPrint(("volSeqNum         = %d\n", pvoldesc->volSeqNum));
    UDFPrint(("maxVolSeqNum      = %d\n", pvoldesc->maxVolSeqNum));
    UDFPrint(("interchangeLvl    = %d\n", pvoldesc->interchangeLvl));
    UDFPrint(("maxInterchangeLvl = %d\n", pvoldesc->maxInterchangeLvl));
    UDFPrint(("charSetList       = %d\n", pvoldesc->charSetList));
    UDFPrint(("maxCharSetList    = %d\n", pvoldesc->maxCharSetList));
    // ...& just print VolSetIdent
    UNICODE_STRING      instr;
#define CUR_IDENT_SZ (sizeof(pvoldesc->volSetIdent))
    UDFGetDstring(&instr, (dstring*)&(pvoldesc->volSetIdent), CUR_IDENT_SZ);
#undef CUR_IDENT_SZ
    UDFPrint(("volSetIdent[] = '%wZ'\n", &instr));
//    UDFPrint(("maxInterchangeLvl = %d\n", pvoldesc->maxInterchangeLvl));
    UDFPrint(("flags             = %x\n", pvoldesc->flags));
    if (instr.Buffer) MyFreePool__(instr.Buffer);
#endif // UDF_DBG
} // end UDFLoadPVolDesc()

/*
   load Logical volume integrity descriptor
 */
NTSTATUS
UDFLoadLogicalVolInt(
    IN PIRP_CONTEXT IrpContext,
    PDEVICE_OBJECT  DeviceObject,
    PVCB            Vcb,
    extent_ad       loc
    )
{
    NTSTATUS    RC = STATUS_SUCCESS;
    uint32      Len;
    uint32      Lbn;
    int8*       Buf = NULL;
    int8*       TempBuf = NULL;
    uint16      ident;
    struct LogicalVolIntegrityDescImpUse* LVID_iUse;
    struct LogicalVolHeaderDesc* LVID_hd;
    uint32      MaxIterations = UDF_MAX_LVID_CHAIN_LENGTH;
    uint32      LvidCount = 0;
    BOOLEAN     HasContinuation = FALSE;
    BOOLEAN     EndsWithTerminator = FALSE;

    ASSERT(!Vcb->LVid);

    // Get extent length and starting LBN

    Len = loc.extLength;
    Lbn = loc.extLocation;


    // Validate: length must be non-zero and sector-aligned

    if (Len == 0 || ((Len & (SectorSize(Vcb) - 1)) != 0)) {
        return STATUS_DISK_CORRUPT_ERROR;
    }


    // Allocate sector buffer

    Buf = (int8*)MyAllocatePool__(NonPagedPool, SectorSize(Vcb));
    if (!Buf) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    _SEH2_TRY {

        // Process integrity descriptors - one sector at a time

        while (Len != 0) {

            UDFPrint(("UDF: Reading LVID @%x (%x)\n", Lbn, Len));

            //
            // Read one sector and verify tag
            //
            RC = UDFReadTagged(IrpContext, Vcb, Buf, Lbn, Lbn, &ident);
            if (!NT_SUCCESS(RC)) {
                UDFPrint(("UDF: Reading LVID @%x failed.\n", Lbn));
                Vcb->VcbState |= VCB_STATE_VOLUME_READ_ONLY;
                try_return(RC = STATUS_SUCCESS);
            }

            // Check for unrecorded sector (tag ident == 0)

            if (ident == 0) {
                break;
            }

            // Must be LVID or Terminating Descriptor

            if (ident == TID_TERMINATING_DESC) {
                // Terminating descriptor - done with this extent
                EndsWithTerminator = TRUE;
                break;
            }

            if (ident != TID_LOGICAL_VOL_INTEGRITY_DESC) {
                try_return(RC = STATUS_DISK_CORRUPT_ERROR);
            }

            // Valid LVID found - check integrity type

            struct LogicalVolIntegrityDesc* lvid = (struct LogicalVolIntegrityDesc*)Buf;
            LvidCount++;

            /* A fixed writable disk must not treat a CRC-less or
             * under-covered LVID as a clean checkpoint. UDFReadTagged
             * accepts a zero descriptor CRC for legacy media; that is too
             * weak for deciding that crash recovery may be skipped. Keep
             * the existing read-only policy on an untrusted LVID. */
            if (Vcb->TargetDeviceObject->DeviceType == FILE_DEVICE_DISK &&
                SectorSize(Vcb) == 512 &&
                (lvid->descTag.descCRCLength < 136 - sizeof(tag) ||
                 lvid->descTag.descCRCLength > SectorSize(Vcb) - sizeof(tag) ||
                 lvid->descTag.descCRC !=
                     UDFCrc((uint8*)Buf + sizeof(tag),
                            lvid->descTag.descCRCLength, 0))) {
                UDFPrint(("UDF: untrusted LVID CRC at %x; read-only mount\n",
                          Lbn));
                Vcb->VcbState |= VCB_STATE_VOLUME_READ_ONLY;
                try_return(RC = STATUS_SUCCESS);
            }

            if (lvid->integrityType != INTEGRITY_TYPE_OPEN &&
                lvid->integrityType != INTEGRITY_TYPE_CLOSE) {
                try_return(RC = STATUS_DISK_CORRUPT_ERROR);
            }


            // Check for continuation extent (nextIntegrityExt)

            if (lvid->nextIntegrityExt.extLength != 0) {
                uint32 NextLen = lvid->nextIntegrityExt.extLength;
                uint32 NextLba = lvid->nextIntegrityExt.extLocation;
                HasContinuation = TRUE;

                //
                // Decrement max iterations to prevent infinite loops
                //
                MaxIterations--;
                if (MaxIterations == 0) {
                    UDFPrint(("UDF: LVID chain too long, aborting.\n"));
                    try_return(RC = STATUS_DISK_CORRUPT_ERROR);
                }

                //
                // Validate next extent alignment
                //
                if ((NextLen & (SectorSize(Vcb) - 1)) != 0) {
                    try_return(RC = STATUS_DISK_CORRUPT_ERROR);
                }

                /* Remember two unused sectors at the end of this allocated
                 * integrity extent. The writer validates their contents and
                 * physical ownership after the partition maps are loaded. */
                if (lvid->integrityType == INTEGRITY_TYPE_CLOSE &&
                    SectorSize(Vcb) == 512 &&
                    Vcb->TargetDeviceObject->DeviceType == FILE_DEVICE_DISK &&
                    Len >= 5 * SectorSize(Vcb)) {
                    ULONGLONG End = (ULONGLONG)Lbn +
                                    Len / SectorSize(Vcb);
                    if (End <= MAXULONG && End >= (ULONGLONG)Lbn + 5 &&
                        NextLba != End - 2 && NextLba != End - 1) {
                        Vcb->LvidJournalSlot[0] = (uint32)(End - 2);
                        Vcb->LvidJournalSlot[1] = (uint32)(End - 1);
                        Vcb->LvidJournalTarget = NextLba;
                    }
                }


                // Prepare jump to continuation extent:
                // After "Len -= SectorSize; Lbn++;" below, we'll have
                // Len = NextLen and Lbn = nextIntegrityExt.extLocation

                Len = NextLen + SectorSize(Vcb);
                Lbn = lvid->nextIntegrityExt.extLocation - 1;
            }


            // Swap buffers - save current descriptor as result

            TempBuf = (int8*)Vcb->LVid;
            Vcb->LVid = (struct LogicalVolIntegrityDesc*)Buf;
            Buf = TempBuf;
            TempBuf = NULL;

            //
            // Allocate new buffer for next iteration if needed
            //
            if (Buf == NULL && Len > SectorSize(Vcb)) {
                Buf = (int8*)MyAllocatePool__(NonPagedPool, SectorSize(Vcb));
                if (!Buf) {
                    try_return(RC = STATUS_INSUFFICIENT_RESOURCES);
                }
            }

            // Advance to next sector

            Len -= SectorSize(Vcb);
            Lbn++;
        }


        // Check if we found a valid LVID

        if (Vcb->LVid == NULL) {
            UDFPrint(("UDF: No valid LVID found.\n"));
            Vcb->VcbState |= VCB_STATE_VOLUME_READ_ONLY;
            try_return(RC = STATUS_SUCCESS);
        }


        // Process the last (most recent) LVID

        Vcb->origIntegrityType =
            Vcb->IntegrityType = Vcb->LVid->integrityType;
        Vcb->LVid_loc.extLength = SectorSize(Vcb);
        Vcb->LVid_loc.extLocation = Lbn - 1;  // Last read sector
        if (Vcb->LvidJournalTarget != Vcb->LVid_loc.extLocation) {
            Vcb->LvidJournalSlot[0] = 0;
            Vcb->LvidJournalSlot[1] = 0;
            Vcb->LvidJournalTarget = 0;
        }
        /* The older contiguous LVID layout has no linked CLOSED
         * predecessor. Its first two descriptors and terminator still lie
         * in an allocated integrity extent with two spare tail sectors.
         * Journal those future writes only for this exact bounded layout;
         * discovery below verifies the spare sectors and physical bounds. */
        if (!Vcb->LvidJournalTarget && !HasContinuation &&
            EndsWithTerminator && LvidCount == 2 &&
            SectorSize(Vcb) == 512 &&
            Vcb->TargetDeviceObject->DeviceType == FILE_DEVICE_DISK &&
            loc.extLength >= 5 * SectorSize(Vcb) &&
            loc.extLength <= 256 * SectorSize(Vcb) &&
            Vcb->LVid_loc.extLocation == loc.extLocation + 1 &&
            Lbn == loc.extLocation + 2 &&
            (ULONGLONG)loc.extLocation +
                loc.extLength / SectorSize(Vcb) <= MAXULONG) {
            uint32 End = loc.extLocation +
                         loc.extLength / SectorSize(Vcb);
            if (End - 2 > Vcb->LVid_loc.extLocation) {
                Vcb->LvidJournalSlot[0] = End - 2;
                Vcb->LvidJournalSlot[1] = End - 1;
                Vcb->LvidJournalTarget = Vcb->LVid_loc.extLocation;
            }
        }

        LVID_iUse = UDFGetLVIDiUse(Vcb);

        UDFPrint(("UDF: Last LVID:\n"));
        UDFPrint(("     minR: %x\n",LVID_iUse->minUDFReadRev ));
        UDFPrint(("     minW: %x\n",LVID_iUse->minUDFWriteRev));
        UDFPrint(("     maxW: %x\n",LVID_iUse->maxUDFWriteRev));
        UDFPrint(("     Type: %s\n",!Vcb->IntegrityType ? "Open" : "Close"));

        Vcb->minUDFReadRev  = LVID_iUse->minUDFReadRev;
        Vcb->minUDFWriteRev = LVID_iUse->minUDFWriteRev;
        Vcb->maxUDFWriteRev = LVID_iUse->maxUDFWriteRev;

        Vcb->numFiles = LVID_iUse->numFiles;
        Vcb->numDirs  = LVID_iUse->numDirs;
        UDFPrint(("     nFiles: %x\n",Vcb->numFiles ));
        UDFPrint(("     nDirs: %x\n",Vcb->numDirs ));

        // Check if we can understand this format
        if (Vcb->minUDFReadRev > UDF_MAX_READ_REVISION) {
            RC = STATUS_UNRECOGNIZED_VOLUME;
        }
        // Check if we know how to write here
        if (Vcb->minUDFWriteRev > UDF_MAX_WRITE_REVISION) {
            UDFPrint(("     Target FS requires: %x Revision => ReadOnly\n",Vcb->minUDFWriteRev));
            Vcb->VcbState |= VCB_STATE_VOLUME_READ_ONLY;
        }

        LVID_hd = (struct LogicalVolHeaderDesc*)&(Vcb->LVid->logicalVolContentsUse);
        Vcb->NextUniqueId = LVID_hd->uniqueID;
        Vcb->UniqueIdReservedEnd = Vcb->NextUniqueId;
        UDFPrint(("     Next FID: %x\n",Vcb->NextUniqueId));

try_exit: NOTHING;

    } _SEH2_FINALLY {

        //
        // Free temporary buffer
        //
        if (Buf != NULL) {
            MyFreePool__(Buf);
        }

    } _SEH2_END;

    //
    // If failed, free any allocated LVID
    //
    if (!NT_SUCCESS(RC)) {
        if (Vcb->LVid != NULL) {
            MyFreePool__(Vcb->LVid);
            Vcb->LVid = NULL;
        }
    }

    return RC;
} // end UDFLoadLogicalVolInt()


/*
    load Logical volume descriptor
 */
NTSTATUS
UDFLoadLogicalVol(
    IN PIRP_CONTEXT IrpContext,
    PDEVICE_OBJECT  DeviceObject,
    PVCB            Vcb,
    int8*           Buf,
    lb_addr         *fileset
    )
{
    struct LogicalVolDesc *lvd = (struct LogicalVolDesc *)Buf;
    uint16 i, offset;
    uint8 type;
    NTSTATUS status = STATUS_SUCCESS;
    UDFPrint(("UDF: LogicalVolDesc\n"));
    // Validate partition map counter
    if (!(Vcb->Partitions)) {
        Vcb->PartitionMaps = lvd->numPartitionMaps;
        Vcb->Partitions = (PUDFPartMap)MyAllocatePool__(NonPagedPool, sizeof(UDFPartMap) * Vcb->PartitionMaps );
        if (!Vcb->Partitions)
            return STATUS_INSUFFICIENT_RESOURCES;
    } else {
        if (Vcb->PartitionMaps != lvd->numPartitionMaps)
            return STATUS_DISK_CORRUPT_ERROR;
    }
    UDFPrint(("UDF: volDescSeqNum = %x\n", lvd->volDescSeqNum));

    // Since UDF requires that all Volumes within a Volume Set have the same logical sector size,
    // the Logical Block Size will be the same as the logical sector size of the Volume.

    if (lvd->logicalBlockSize != Vcb->SectorSize) {

        return STATUS_UNRECOGNIZED_VOLUME;
    }

    // Get current UDF revision
    // Get Read-Only flags
    UDFReadEntityID_Domain(Vcb, &(lvd->domainIdent));

    UDFPrint(("UDF: mapTableLength = %x\n", lvd->mapTableLength));
    UDFPrint(("UDF: numPartitionMaps = %x\n", lvd->numPartitionMaps));
    // walk through all available part maps
    for (i=0,offset=0;
         i<Vcb->PartitionMaps && offset<lvd->mapTableLength;
         i++,offset+=((struct GenericPartitionMap *)( ((uint8*)(lvd+1))+offset) )->partitionMapLength)
    {
        struct GenericPartitionMap* gpm = (struct GenericPartitionMap *)(((uint8*)(lvd+1))+offset);
        type = gpm->partitionMapType;
        UDFPrint(("Partition (%d) type %x, len %x\n", i, type, gpm->partitionMapLength));
        if (type == PARTITION_MAP_TYPE_1)
        {
            struct GenericPartitionMap1 *gpm1 = (struct GenericPartitionMap1 *)(((uint8*)(lvd+1))+offset);

            Vcb->Partitions[i].PartitionType = UDF_TYPE1_MAP15;
            Vcb->Partitions[i].VolumeSeqNum = gpm1->volSeqNum;
            Vcb->Partitions[i].PartitionNum = gpm1->partitionNum;
            status = STATUS_SUCCESS;
        }
        else if (type == PARTITION_MAP_TYPE_2)
        {
            struct UdfPartitionMap2* upm2 = (struct UdfPartitionMap2 *)(((uint8*)(lvd+1))+offset);
            if (!strncmp((int8*)&(upm2->partIdent.ident), UDF_ID_VIRTUAL, strlen(UDF_ID_VIRTUAL)))
            {
                UDFIdentSuffix* udfis =
                    (UDFIdentSuffix*)&(upm2->partIdent.identSuffix);

                if ( (udfis->currentRev == 0x0150)/* ||
                    (Vcb->CurrentUDFRev == 0x0150)*/ ) {
                    UDFPrint(("Found VAT 1.50\n"));
                    Vcb->Partitions[i].PartitionType = UDF_VIRTUAL_MAP15;
                } else
                if ( (udfis->currentRev == 0x0200) ||
                    (udfis->currentRev == 0x0201) /*||
                    (Vcb->CurrentUDFRev == 0x0200) ||
                    (Vcb->CurrentUDFRev == 0x0201)*/ ) {
                    UDFPrint(("Found VAT 2.00\n"));
                    Vcb->Partitions[i].PartitionType = UDF_VIRTUAL_MAP20;
                }
                status = STATUS_SUCCESS;
            }
            else if (!strncmp((int8*)&(upm2->partIdent.ident), UDF_ID_SPARABLE, strlen(UDF_ID_SPARABLE)))
            {
                UDFPrint(("Load sparing table\n"));
                PSPARABLE_PARTITION_MAP spm = (PSPARABLE_PARTITION_MAP)(((uint8*)(lvd+1))+offset);
                Vcb->Partitions[i].PartitionType = UDF_SPARABLE_MAP15;
                status = UDFLoadSparingTable(IrpContext, Vcb, spm);
            }
            else if (!strncmp((int8*)&(upm2->partIdent.ident), UDF_ID_METADATA, strlen(UDF_ID_METADATA)))
            {
                UDFPrint(("Found metadata partition\n"));
                PMETADATA_PARTITION_MAP mpm = (PMETADATA_PARTITION_MAP)(((uint8*)(lvd+1))+offset);
                Vcb->Partitions[i].PartitionType = UDF_METADATA_MAP25;

                Vcb->MetadataRef = i;
                Vcb->MetadataMirrorFELoc = mpm->metadataMirrorFELocation;
                Vcb->MetadataFELoc = mpm->metadataFELocation;
                Vcb->MetadataAllocUnit = mpm->allocationUnit;
                Vcb->MetadataBitmapFELoc = mpm->metadataBitmapFELocation;
                Vcb->MetadataFlags = mpm->flags;
                Vcb->MetadataAlignUnit = mpm->alignmentUnit;
                
                UDFPrint(
                    (
                        "Metadata FE %x, mirror FE %x, bitmap FE %x, alloc unit %x, align %x, flags %x\n",
                        mpm->metadataFELocation, mpm->metadataMirrorFELocation, mpm->metadataBitmapFELocation,
                        mpm->allocationUnit, mpm->alignmentUnit, mpm->flags
                    )
                );
            }
            else
            {
                UDFPrint(("Unknown ident: %s\n", upm2->partIdent.ident));
                continue;
            }
            Vcb->Partitions[i].VolumeSeqNum = upm2->volSeqNum;
            Vcb->Partitions[i].PartitionNum = upm2->partitionNum;
        }
    }

    if (fileset) {
        // remember FileSet location
        long_ad *la = (long_ad *)&(lvd->logicalVolContentsUse[0]);
        *fileset = (la->extLocation);
        UDFPrint(("FileSet found in LogicalVolDesc at block=%x, partition=%d\n",
            fileset->logicalBlockNum,
            fileset->partitionReferenceNum));
    }
    if (NT_SUCCESS(status)) {
        // load Integrity Desc if any
        if (lvd->integritySeqExt.extLength)
            status = UDFLoadLogicalVolInt(IrpContext, DeviceObject, Vcb, lvd->integritySeqExt);
    }
    return status;
} // end UDFLoadLogicalVol()

NTSTATUS
UDFLoadBogusLogicalVol(
    PDEVICE_OBJECT  DeviceObject,
    PVCB            Vcb,
    int8*           Buf,
    lb_addr         *fileset
    )
{
//    LogicalVolDesc *lvd = (LogicalVolDesc *)Buf;
    UDFPrint(("UDF: Bogus LogicalVolDesc\n"));
    // Validate partition map counter
    if (!(Vcb->Partitions)) {
        Vcb->PartitionMaps = 1;
        Vcb->Partitions = (PUDFPartMap)MyAllocatePool__(NonPagedPool, sizeof(UDFPartMap) * Vcb->PartitionMaps );
        if (!Vcb->Partitions)
            return STATUS_INSUFFICIENT_RESOURCES;
    } else {
        if (Vcb->PartitionMaps != 1)
            return STATUS_DISK_CORRUPT_ERROR;
    }
    UDFPrint(("UDF: volDescSeqNum = %x\n", 0));

    // Get current UDF revision
    // Get Read-Only flags
//    UDFReadEntityID_Domain(Vcb, &(lvd->domainIdent));

    UDFPrint(("UDF: mapTableLength = %x\n", 0));
    UDFPrint(("UDF: numPartitionMaps = %x\n", 0));

// if (CDRW) {

    Vcb->Partitions[0].PartitionType = UDF_TYPE1_MAP15;
    Vcb->Partitions[0].VolumeSeqNum = 0;
    Vcb->Partitions[0].PartitionNum = 0;

/* } else if (CDR)
                if ()
                    UDFPrint(("Found VAT 1.50\n"));
                    Vcb->Partitions[i].PartitionType = UDF_VIRTUAL_MAP15;
                } else
                    UDFPrint(("Found VAT 2.00\n"));
                    Vcb->Partitions[i].PartitionType = UDF_VIRTUAL_MAP20;
                }
            }
    }
*/
    if (fileset) {
        // remember FileSet location
//        long_ad *la = (long_ad *)&(lvd->logicalVolContentsUse[0]);
        fileset->logicalBlockNum = 0;
        fileset->partitionReferenceNum = 0;
        UDFPrint(("FileSet found in LogicalVolDesc at block=%x, partition=%d\n",
            fileset->logicalBlockNum,
            fileset->partitionReferenceNum));
    }
    return STATUS_SUCCESS;
} // end UDFLoadBogusLogicalVol()

/*
    This routine adds given Bitmap to existing one
 */
NTSTATUS
UDFAddXSpaceBitmap(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN uint32 RefPartNum,
    IN PSHORT_AD bm
    )
{
    int8* tmp;
    int8* tmp_bm;
    uint32 i, lim, j, lba, l, lim2, l2, k;
    lb_addr locAddr;
    NTSTATUS status;
    uint16 Ident;
    uint32 flags;
    SIZE_T Length;
    BOOLEAN bit_set;

    UDF_CHECK_BITMAP_RESOURCE(Vcb);
    UDFPrint(("UDFAddXSpaceBitmap: at block=%x, partition=%d\n",
        bm->extPosition,
        RefPartNum));

    if (!(Length = (bm->extLength & UDF_EXTENT_LENGTH_MASK))) return STATUS_SUCCESS;
    // Bitmap is LBN-indexed, start at 0
    i=0;
    flags = bm->extLength >> 30;
    if (!flags /*|| flags == EXTENT_NOT_RECORDED_ALLOCATED*/) {
        tmp = (int8*)DbgAllocatePool(NonPagedPool, max(Length, Vcb->SectorSize));
        if (!tmp) return STATUS_INSUFFICIENT_RESOURCES;
        locAddr.partitionReferenceNum = (uint16)RefPartNum;
        locAddr.logicalBlockNum = bm->extPosition;
        // read header of the Bitmap
        if (!NT_SUCCESS(status = UDFReadTagged(IrpContext, Vcb, tmp, lba = UDFPartLbaToPhys(Vcb, &locAddr),
                             locAddr.logicalBlockNum, &Ident))) {
err_addxsbm_1:
            DbgFreePool(tmp);
            return status;
        }
        if (Ident != TID_SPACE_BITMAP_DESC) {
            status = STATUS_DISK_CORRUPT_ERROR;
            goto err_addxsbm_1;
        }

        // read the whole Bitmap
        if (!NT_SUCCESS(status = UDFReadData(IrpContext, Vcb, FALSE, ((uint64)lba)<<Vcb->SectorShift, Length, FALSE, tmp)))
            goto err_addxsbm_1;

        lim = min(i + (lim2 = ((PSPACE_BITMAP_DESC)tmp)->numOfBits), Vcb->FSBM_BitCount);
        tmp_bm = tmp + sizeof(SPACE_BITMAP_DESC);
        j = 0;
        for(;(l = UDFGetBitmapLen((uint32*)tmp_bm, j, lim2)) && (i<lim);) {
            // expand LBlocks to Sectors...
            l2 = l;
            // ...and mark them
            bit_set = UDFGetFreeBit(tmp_bm, j);
            for(k=0;(k<l2) && (i<lim);k++) {
                if (bit_set) {
                    // FREE block
                    UDFSetFreeBit(Vcb->FSBM_Bitmap, i);
                    UDFSetFreeBitOwner(Vcb, i);
                }
                i++;
            }
            j += l;
        }
        DbgFreePool(tmp);
/*    } else if ((bm->extLength >> 30) == EXTENT_NOT_RECORDED_ALLOCATED) {
        i=Vcb->Partitions[RefPartNum].PartitionRoot;
        lim = i + Vcb->Partitions[RefPartNum].PartitionLen;
        for(;i<lim;i++) {
            UDFSetUsedBit(Vcb->FSBM_Bitmap, i);
        }*/
    }
    return STATUS_SUCCESS;
} // end UDFAddXSpaceBitmap()

/*
    This routine adds given Bitmap to existing one
 */
NTSTATUS
UDFVerifyXSpaceBitmap(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN uint32 RefPartNum,
    IN PSHORT_AD bm
    )
{
    int8* tmp;
//    int8* tmp_bm;
//    uint32 i, l2, k, lim, j, lim2;
    uint32 lba;
    lb_addr locAddr;
    NTSTATUS status;
    uint16 Ident;
    uint32 flags;
    uint32 Length;
//    BOOLEAN bit_set;

    UDF_CHECK_BITMAP_RESOURCE(Vcb);

    UDFPrint((" UDFVerifyXSpaceBitmap: part %x\n", RefPartNum));

    if (!(Length = (bm->extLength & UDF_EXTENT_LENGTH_MASK))) return STATUS_SUCCESS;
//    i=UDFPartStart(Vcb, RefPartNum);
    flags = bm->extLength >> 30;
    if (!flags /*|| flags == EXTENT_NOT_RECORDED_ALLOCATED*/) {
        tmp = (int8*)DbgAllocatePool(NonPagedPool, max(Length, Vcb->SectorSize));
        if (!tmp) return STATUS_INSUFFICIENT_RESOURCES;
        locAddr.partitionReferenceNum = (uint16)RefPartNum;
        locAddr.logicalBlockNum = bm->extPosition;
        // read header of the Bitmap
        if (!NT_SUCCESS(status = UDFReadTagged(IrpContext, Vcb, tmp, lba = UDFPartLbaToPhys(Vcb, &locAddr),
                             locAddr.logicalBlockNum, &Ident))) {
err_vfyxsbm_1:
            DbgFreePool(tmp);
            return status;
        }
        UDFPrint((" BM Lba %x\n", lba));
        if (Ident != TID_SPACE_BITMAP_DESC) {
            status = STATUS_DISK_CORRUPT_ERROR;
            goto err_vfyxsbm_1;
        }
        // read the whole Bitmap
        if (!NT_SUCCESS(status = UDFReadData(IrpContext, Vcb, FALSE, ((uint64)lba)<<Vcb->SectorShift, Length, FALSE, tmp)))
            goto err_vfyxsbm_1;

//        lim = min(i + ((lim2 = ((PSPACE_BITMAP_DESC)tmp)->numOfBits) << Vcb->LB2B_Bits), Vcb->FSBM_BitCount);
//        tmp_bm = tmp + sizeof(SPACE_BITMAP_DESC);
//        j = 0;
/*        for(;(l = UDFGetBitmapLen((uint32*)tmp_bm, j, lim2)) && (i<lim);) {
            // expand LBlocks to Sectors...
            l2 = l << Vcb->LB2B_Bits;
            // ...and mark them
            if (bm_type == UDF_FSPACE_BM) {
                bit_set = UDFGetFreeBit(tmp_bm, j);
                for(k=0;(k<l2) && (i<lim);k++) {
                    if (bit_set) {
                        // FREE block
                        UDFSetFreeBit(Vcb->FSBM_Bitmap, i);
                        UDFSetFreeBitOwner(Vcb, i);
                        UDFSetZeroBit(Vcb->ZSBM_Bitmap, i);
                    } else {
                        // USED block
                        UDFClrZeroBit(Vcb->ZSBM_Bitmap, i);
                    }
                    i++;
                }
            } else {
                bit_set = UDFGetZeroBit(tmp_bm, j);
                for(k=0;(k<l2) && (i<lim);k++) {
                    if (bit_set) {
                        // ZERO block
                        UDFSetZeroBit(Vcb->ZSBM_Bitmap, i);
                    } else {
                        // DATA block
                        UDFClrZeroBit(Vcb->ZSBM_Bitmap, i);
                    }
                    i++;
                }
            }
            j += l;
        }*/
        DbgFreePool(tmp);
/*    } else if ((bm->extLength >> 30) == EXTENT_NOT_RECORDED_ALLOCATED) {
        i=Vcb->Partitions[RefPartNum].PartitionRoot;
        lim = i + Vcb->Partitions[RefPartNum].PartitionLen;
        for(;i<lim;i++) {
            UDFSetUsedBit(Vcb->FSBM_Bitmap, i);
        }*/
    }
    return STATUS_SUCCESS;
} // end UDFVerifyXSpaceBitmap()

/*
    This routine subtracts given Bitmap to existing one
 */
/*NTSTATUS
UDFDelXSpaceBitmap(
    IN PVCB Vcb,
    IN uint32 RefPartNum,
    IN PSHORT_AD bm
    )
{
    int8* tmp, tmp_bm;
    uint32 i, lim, j;
    lb_addr locAddr;
    NTSTATUS status;
    uint16 Ident;
    uint32 flags;
    uint32 Length;

    if (!(Length = (bm->extLength & UDF_EXTENT_LENGTH_MASK))) return STATUS_SUCCESS;
    i=0;
    flags = bm->extLength >> 30;
    if (!flags || flags == EXTENT_NOT_RECORDED_ALLOCATED) {
        tmp = (int8*)MyAllocatePool__(NonPagedPool, Length);
        if (!tmp) return STATUS_INSUFFICIENT_RESOURCES;
        locAddr.partitionReferenceNum = (uint16)RefPartNum;
        locAddr.logicalBlockNum = bm->extPosition;
        if ((!NT_SUCCESS(status = UDFReadTagged(Vcb, tmp, (j = UDFPartLbaToPhys(Vcb, &(locAddr))),
                             locAddr.logicalBlockNum, &Ident))) ||
           (Ident != TID_SPACE_BITMAP_DESC) ) {
            MyFreePool__(tmp);
            return status;
        }
        if (!NT_SUCCESS(status = UDFReadData(Vcb, FALSE, ((uint64)j)<<Vcb->BlockSizeBits, Length, FALSE, tmp))) {
            MyFreePool__(tmp);
            return status;
        }
        lim = i + ((PSPACE_BITMAP_DESC)tmp)->numOfBits;
        tmp_bm = tmp + sizeof(SPACE_BITMAP_DESC);
        j = 0;
        for(;i<lim;i++) {
            if (UDFGetUsedBit(tmp_bm, j)) UDFSetFreeBit(Vcb->FSBM_Bitmap, i);
            j++;
        }
        MyFreePool__(tmp);
//    } else if ((bm->extLength >> 30) == EXTENT_NOT_RECORDED_ALLOCATED) {
//        i=Vcb->Partitions[RefPartNum].PartitionRoot;
//        lim = i + Vcb->Partitions[RefPartNum].PartitionLen;
//        for(;i<lim;i++) {
//            UDFSetUsedBit(Vcb->FSBM_Bitmap, i);
//        }
    }
    return STATUS_SUCCESS;
} // end UDFDelXSpaceBitmap()  */

/*
    This routine verifues FreeSpaceBitmap (internal) according to media
    parameters & input data
 */
NTSTATUS
UDFVerifyFreeSpaceBitmap(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN uint32 RefPartNum,
    IN PPARTITION_HEADER_DESC phd, // partition header pointing to Bitmaps
    IN uint32 Lba                   // UnallocSpaceDesc
    )
{
    NTSTATUS status;
    uint32 i, l;
    uint16 Ident;
    int8* AllocDesc;
    PEXTENT_MAP Extent;
    lb_addr locAddr;

    status = STATUS_SUCCESS;
    UDFPrint(("UDFVerifyFreeSpaceBitmap:\n"));
    // read info for partition header (if any)
    if (phd) {
        // read unallocated Bitmap
        if (!NT_SUCCESS(status = UDFVerifyXSpaceBitmap(IrpContext, Vcb, RefPartNum, &(phd->unallocatedSpaceBitmap))))
            return status;

        if (phd->unallocatedSpaceTable.extPosition ||
            phd->freedSpaceTable.extPosition ||
            phd->freedSpaceBitmap.extPosition ) {

            Vcb->VcbState |= VCB_STATE_VOLUME_READ_ONLY;
        }
    }
    // read UnallocatedSpaceDesc & convert to Bitmap
    if (Lba) {
        UDFPrint((" Lba @%x\n", Lba));
        if (!(AllocDesc = (int8*)MyAllocatePool__(NonPagedPool, Vcb->SectorSize + sizeof(EXTENT_AD) )))
            return STATUS_INSUFFICIENT_RESOURCES;
        RtlZeroMemory(((int8*)AllocDesc) + Vcb->SectorSize, sizeof(EXTENT_AD));
        if (!NT_SUCCESS(status = UDFReadTagged(IrpContext, Vcb, AllocDesc, Lba, Lba, &Ident)) ||
           !(Extent = (PEXTENT_MAP)MyAllocatePool__(NonPagedPool, l = (((PUNALLOC_SPACE_DESC)AllocDesc)->numAllocDescs+1) * sizeof(EXTENT_AD) ))) {
            MyFreePool__(AllocDesc);
            return NT_SUCCESS(status) ? STATUS_INSUFFICIENT_RESOURCES : status;
        }

        RtlCopyMemory((int8*)Extent, AllocDesc+sizeof(UNALLOC_SPACE_DESC), (((PUNALLOC_SPACE_DESC)AllocDesc)->numAllocDescs+1) * sizeof(EXTENT_AD) );
        locAddr.partitionReferenceNum = (uint16)RefPartNum;
        // read extent is recorded with relative addresses
        // so, we should convert it to suitable form
        for(i=0; Extent[i].extLength; i++) {
            locAddr.logicalBlockNum = Extent[i].extLocation;
            Extent[i].extLocation = UDFPartLbaToPhys(Vcb, &locAddr);
            if (Extent[i].extLocation == LBA_OUT_OF_EXTENT) {
                BrutePoint();
                MyFreePool__(Extent);
                MyFreePool__(AllocDesc);
                return STATUS_DISK_CORRUPT_ERROR;
            }
            if (Extent[i].extLocation >= Vcb->FSBM_BitCount)
            {
                UDFPrint(("Bigger\n"));
                // continue;
            }
            if ((Extent[i].extLocation >> 30) == EXTENT_NEXT_EXTENT_ALLOCDESC) {
                // load continuation
                Lba = Extent[i].extLocation & UDF_EXTENT_LENGTH_MASK;
                /* The continuation is merged into Extent below; allocating a
                 * new Extent here lost the one being built. */
                if (!NT_SUCCESS(status = UDFReadTagged(IrpContext, Vcb, AllocDesc, Lba, Lba, &Ident))) {
                    MyFreePool__(Extent);
                    MyFreePool__(AllocDesc);
                    return status;
                }
                if (Ident == TID_UNALLOC_SPACE_DESC) {

                    if (!(l = MyReallocPool__((int8*)Extent, l, (int8**)&Extent, i*sizeof(EXTENT_MAP)))) {
                        MyFreePool__(Extent);
                        MyFreePool__(AllocDesc);
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }
                    Extent[i].extLength =
                    Extent[i].extLocation = 0;
                    Extent = UDFMergeMappings(Extent, (PEXTENT_MAP)(AllocDesc+sizeof(UNALLOC_SPACE_DESC)) );
#ifdef UDF_DBG
                } else {
                    UDFPrint(("Broken unallocated space descriptor sequence\n"));
#endif // UDF_DBG
                }
            }
        }
//        UDFMarkSpaceAsXXX(Vcb, (-1), Extent, AS_USED); // mark as used
        MyFreePool__(Extent);
        MyFreePool__(AllocDesc);
        status = STATUS_SUCCESS;
    }
    return status;
} // end UDFVerifyFreeSpaceBitmap()

/*
    This routine builds FreeSpaceBitmap (internal) according to media
    parameters & input data
 */
NTSTATUS
UDFBuildFreeSpaceBitmap(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN uint32 RefPartNum,
    IN PPARTITION_HEADER_DESC phd, // partition header pointing to Bitmaps
    IN uint32 Lba                   // UnallocSpaceDesc
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    uint32 i, l;
    uint16 Ident;
    int8* AllocDesc;
    PEXTENT_MAP Extent;
    lb_addr locAddr;
    BOOLEAN UnallocSpaceExtent = FALSE;

    if (!Vcb->FSBM_BitCount) {
        // Bitmap is LBN-indexed: bit 0 = first sector of partition
        uint32 bitmapLen = Vcb->Partitions[RefPartNum].PartitionLen;

        if (!Vcb->BitmapFcb) {
            // No bitmap stream — allocate NonPagedPool buffer (legacy path)
            Vcb->FSBM_Bitmap = (int8*)DbgAllocatePool(NonPagedPool, (i = (bitmapLen+7)>>3) );
            if (!(Vcb->FSBM_Bitmap)) return STATUS_INSUFFICIENT_RESOURCES;
            RtlZeroMemory(Vcb->FSBM_Bitmap, i);
            Vcb->FSBM_ByteCount = i;
            Vcb->FSBM_BitCount = bitmapLen;
        }
        // else: FSBM_BitCount, ByteCount already set from bitmap stream header

#ifdef UDF_TRACK_ONDISK_ALLOCATION_OWNERS
        Vcb->FSBM_Bitmap_owners = (uint32*)DbgAllocatePool(NonPagedPool, Vcb->FSBM_BitCount * sizeof(uint32));
        if (!(Vcb->FSBM_Bitmap_owners)) {
            goto free_fsbm;
        }
        RtlFillMemory(Vcb->FSBM_Bitmap_owners, Vcb->FSBM_BitCount * sizeof(uint32), 0xff);
#endif //UDF_TRACK_ONDISK_ALLOCATION_OWNERS
    }
    // read info for partition header (if any)
    if (phd) {
        if (!Vcb->BitmapFcb) {
            // Legacy path: read bitmap from disk into NonPagedPool buffer
            if (!NT_SUCCESS(status = UDFAddXSpaceBitmap(IrpContext, Vcb, RefPartNum, &phd->unallocatedSpaceBitmap)))
                return status;
        }
        // else: bitmap data already loaded from cache stream

        if (phd->unallocatedSpaceTable.extPosition ||
            phd->freedSpaceTable.extPosition ||
            phd->freedSpaceBitmap.extPosition ) {

            Vcb->VcbState |= VCB_STATE_VOLUME_READ_ONLY;
        }
    }
    // read UnallocatedSpaceDesc & convert to Bitmap
    if (Lba) {
        if (!(AllocDesc = (int8*)MyAllocatePool__(NonPagedPool, Vcb->SectorSize + sizeof(EXTENT_AD) )))
            return STATUS_INSUFFICIENT_RESOURCES;
        RtlZeroMemory(((int8*)AllocDesc) + Vcb->SectorSize, sizeof(EXTENT_AD));
        if (!NT_SUCCESS(status = UDFReadTagged(IrpContext, Vcb, AllocDesc, Lba, Lba, &Ident)) ||
           !(Extent = (PEXTENT_MAP)MyAllocatePool__(NonPagedPool, l = (((PUNALLOC_SPACE_DESC)AllocDesc)->numAllocDescs+1) * sizeof(EXTENT_AD) ))) {
            MyFreePool__(AllocDesc);
            return NT_SUCCESS(status) ? STATUS_INSUFFICIENT_RESOURCES : status;
        }

        RtlCopyMemory((int8*)Extent, AllocDesc+sizeof(UNALLOC_SPACE_DESC), (((PUNALLOC_SPACE_DESC)AllocDesc)->numAllocDescs+1) * sizeof(EXTENT_AD) );
        locAddr.partitionReferenceNum = (uint16)RefPartNum;
        // read extent is recorded with relative addresses
        // so, we should convert it to suitable form
        for (i = 0; i < ((PUNALLOC_SPACE_DESC)AllocDesc)->numAllocDescs; i++) {
            locAddr.logicalBlockNum = Extent[i].extLocation;
            Extent[i].extLocation = UDFPartLbaToPhys(Vcb, &locAddr);
            if (Extent[i].extLocation == LBA_OUT_OF_EXTENT) {
                BrutePoint();
                MyFreePool__(Extent);
                MyFreePool__(AllocDesc);
                return STATUS_DISK_CORRUPT_ERROR;
            }
            if (Extent[i].extLocation >= Vcb->FSBM_BitCount)
            {
                UDFPrint(("Bigger\n"));
                // continue;
            }
            if ((Extent[i].extLocation >> 30) == EXTENT_NEXT_EXTENT_ALLOCDESC) {
                // load continuation
                Lba = Extent[i].extLocation & UDF_EXTENT_LENGTH_MASK;
                /* The continuation is merged into Extent below; allocating a
                 * new Extent here lost the one being built. */
                if (!NT_SUCCESS(status = UDFReadTagged(IrpContext, Vcb, AllocDesc, Lba, Lba, &Ident))) {
                    MyFreePool__(Extent);
                    MyFreePool__(AllocDesc);
                    return status;
                }
                if (Ident == TID_UNALLOC_SPACE_DESC) {

                    if (!(l = MyReallocPool__((int8*)Extent, l, (int8**)&Extent, i*sizeof(EXTENT_MAP)))) {
                        MyFreePool__(Extent);
                        MyFreePool__(AllocDesc);
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }
                    Extent[i].extLength =
                    Extent[i].extLocation = 0;
                    Extent = UDFMergeMappings(Extent, (PEXTENT_MAP)(AllocDesc+sizeof(UNALLOC_SPACE_DESC)) );

                    // If the descriptor was not found, Extent will contain gibberish
                    UnallocSpaceExtent = TRUE;
#ifdef UDF_DBG
                } else {
                    UDFPrint(("Broken unallocated space descriptor sequence\n"));
#endif // UDF_DBG
                }
            }
        }

        if (UnallocSpaceExtent) {
            UDFMarkSpaceAsXXX(Vcb, (-1), Extent, AS_USED); // mark as used
        }

        MyFreePool__(Extent);
        MyFreePool__(AllocDesc);
    }
    return status;
} // end UDFVerifyFreeSpaceBitmap()

/*
    process Partition descriptor
 */
NTSTATUS
UDFLoadPartDesc(
    PIRP_CONTEXT IrpContext,
    PVCB      Vcb,
    int8*     Buf
    )
{
    struct PartitionDesc *p = (struct PartitionDesc *)Buf;
    uint32 i;
    NTSTATUS RC;
    BOOLEAN Found = FALSE;
    UDFPrint(("UDF: Pard Descr:\n"));
    UDFPrint((" volDescSeqNum   = %x\n", p->volDescSeqNum));
    UDFPrint((" partitionFlags  = %x\n", p->partitionFlags));
    UDFPrint((" partitionNumber = %x\n", p->partitionNumber));
    UDFPrint((" accessType      = %x\n", p->accessType));
    UDFPrint((" partitionStartingLocation = %x\n", p->partitionStartingLocation));
    UDFPrint((" partitionLength = %x\n", p->partitionLength));
    // There is nothing interesting to comment here
    // Just look at Names & Messages....
    for (i=0; i<Vcb->PartitionMaps; i++) {
        UDFPrint(("Searching map: (%d == %d)\n",
            Vcb->Partitions[i].PartitionNum, (p->partitionNumber) ));
        if (Vcb->Partitions[i].PartitionNum == (p->partitionNumber)) {
            Found = TRUE;

            if (
                (Vcb->Partitions[i].PartitionType == UDF_METADATA_MAP25) &&
                Vcb->MetadataFileInfo
            )
                continue;

            Vcb->Partitions[i].PartitionRoot = p->partitionStartingLocation;
            Vcb->Partitions[i].PartitionLen = p->partitionLength;

            //  Sparing table entries are stored on disk as partition-relative
            //  LBNs (origLocation) with physical PSNs (mappedLocation).
            //  Convert origLocation to physical so UDFRelocateSector can
            //  compare directly against physical LBAs at I/O time.

            if (Vcb->SparingTable &&
                Vcb->Partitions[i].PartitionType == UDF_SPARABLE_MAP15) {

                PSPARING_MAP Map = Vcb->SparingTable;
                for (uint32 s = 0; s < Vcb->SparingCount; s++, Map++) {
                    if (Map->origLocation != SPARING_LOC_AVAILABLE &&
                        Map->origLocation != SPARING_LOC_CORRUPTED) {
                        Map->origLocation += Vcb->Partitions[i].PartitionRoot;
                    }
                }
            }
            Vcb->Partitions[i].UspaceBitmap = 0xFFFFFFFF;
            Vcb->Partitions[i].FspaceBitmap = 0xFFFFFFFF;
            Vcb->Partitions[i].AccessType = p->accessType;
            UDFPrint(("Access mode %x\n", p->accessType));
            if (p->accessType == PARTITION_ACCESS_WO) {
                Vcb->CDR_Mode = TRUE;
//                Vcb->Partitions[i].PartitionLen = Vcb->LastPossibleLBA - p->partitionStartingLocation;
            } else if (p->accessType < PARTITION_ACCESS_WO) {
                // Soft-read-only volume
                UDFPrint(("Soft Read-only volume\n"));
                Vcb->VcbState |= VCB_STATE_VOLUME_READ_ONLY;
            } else if (p->accessType > PARTITION_ACCESS_MAX_KNOWN) {
                return STATUS_UNRECOGNIZED_MEDIA;
            }

            if (
                (Vcb->Partitions[i].PartitionType != UDF_METADATA_MAP25) &&
                (
                    !strcmp((int8*)&(p->partitionContents.ident), PARTITION_CONTENTS_NSR02) ||
                    !strcmp((int8*)&(p->partitionContents.ident), PARTITION_CONTENTS_NSR03)
                )
            )
            {
                PPARTITION_HEADER_DESC phd;

                phd = (PPARTITION_HEADER_DESC)(p->partitionContentsUse);
#ifdef UDF_DBG
                if (phd->unallocatedSpaceTable.extLength)
                    UDFPrint(("unallocatedSpaceTable (part %d)\n", i));
#endif // UDF_DBG
                if (phd->unallocatedSpaceBitmap.extLength) {
                    Vcb->Partitions[i].UspaceBitmap =
                        phd->unallocatedSpaceBitmap.extPosition;
                    UDFPrint(("unallocatedSpaceBitmap (part %d) @ %x\n",
                        i, Vcb->Partitions[i].UspaceBitmap ));
                }
#ifdef UDF_DBG
                if (phd->partitionIntegrityTable.extLength)
                    UDFPrint(("partitionIntegrityTable (part %d)\n", i));
                if (phd->freedSpaceTable.extLength)
                    UDFPrint(("freedSpaceTable (part %d)\n", i));
#endif // UDF_DBG
                if (phd->freedSpaceBitmap.extLength) {
                    Vcb->Partitions[i].FspaceBitmap =
                        phd->freedSpaceBitmap.extPosition;
                    UDFPrint(("freedSpaceBitmap (part %d)\n", i));
                }
                // Create bitmap cache stream and CcPinRead BEFORE building
                // the in-memory bitmap. This loads bitmap data from disk into
                /* With a bounded physical bitmap, defer its disk update to
                 * the volume checkpoint. Cc can otherwise persist newly
                 * allocated data blocks while their cached FE/FID owner is
                 * still unpublished, leaving orphans after a hard cut. The
                 * existing legacy bitmap path keeps those reservations in
                 * nonpaged memory and writes them after the namespace drain.
                 * Large volumes retain the paged bitmap stream. */
                BOOLEAN deferPhysicalBitmap =
                    Vcb->TargetDeviceObject->DeviceType == FILE_DEVICE_DISK &&
                    Vcb->MetadataRef < Vcb->PartitionMaps &&
                    Vcb->Partitions[Vcb->MetadataRef].PartitionType == UDF_METADATA_MAP25 &&
                    p->partitionLength <= 128U * 1024U * 1024U;
                if (deferPhysicalBitmap)
                    UDFPrint(("UDFS physical bitmap: volume-checkpoint writer, %lu blocks\n",
                              (ULONG)p->partitionLength));
                if (
                    phd->unallocatedSpaceBitmap.extLength &&
                    !(phd->unallocatedSpaceBitmap.extLength >> 30) &&
                    !Vcb->BitmapFcb &&
                    !deferPhysicalBitmap
                ) 
                {
                    lb_addr bmLocAddr;
                    bmLocAddr.partitionReferenceNum = (uint16)i;
                    bmLocAddr.logicalBlockNum = phd->unallocatedSpaceBitmap.extPosition;

                    ULONG bmPsn = UDFPartLbaToPhys(Vcb, &bmLocAddr);
                    ULONG bmLength = phd->unallocatedSpaceBitmap.extLength & UDF_EXTENT_LENGTH_MASK;

                    if (bmPsn != LBA_OUT_OF_EXTENT) {
                        RC = UDFCreateBitmapStream(IrpContext, Vcb, bmPsn, bmLength);
                        if (NT_SUCCESS(RC)) {
                            // Per-page CcPinRead: read SBD header to get bitmap sizes.
                            // Actual bitmap data is accessed on demand via UDFPinBitmapPage.
                            Vcb->BitmapDataOffset = sizeof(SPACE_BITMAP_DESC);

                            PVOID pinBuf;

                            _SEH2_TRY {
                                // Pin first page to read the SBD header
                                LARGE_INTEGER pinOfs;
                                pinOfs.QuadPart = 0;
                                ULONG pinLen = min((ULONG)BITMAP_PIN_GRANULARITY,
                                    (ULONG)Vcb->BitmapFcb->Header.AllocationSize.QuadPart);
                                CcPinRead(Vcb->BitmapStreamFileObject,
                                          &pinOfs, pinLen, TRUE,
                                          &Vcb->BitmapBcb, &pinBuf);
                            } _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER) {
                                RC = _SEH2_GetExceptionCode();
                                pinBuf = NULL;
                            } _SEH2_END;

                            if (NT_SUCCESS(RC) && pinBuf) {
                                PSPACE_BITMAP_DESC Sbd = (PSPACE_BITMAP_DESC)pinBuf;
                                ULONG maxDataBytes = bmLength - sizeof(SPACE_BITMAP_DESC);
                                Vcb->FSBM_ByteCount = min(Sbd->numOfBytes, maxDataBytes);
                                Vcb->FSBM_BitCount = min(Sbd->numOfBits, Vcb->FSBM_ByteCount * 8);

                                // Store pinned page state
                                Vcb->BitmapPinnedData = (PUCHAR)pinBuf;
                                Vcb->BitmapPinnedOffset = 0;
                                Vcb->BitmapPinnedLength = min((ULONG)BITMAP_PIN_GRANULARITY,
                                    (ULONG)Vcb->BitmapFcb->Header.AllocationSize.QuadPart);

                                UDFPrint(("UDF BM: SBD numOfBits=%x numOfBytes=%x bmLength=%x\n",
                                    Sbd->numOfBits, Sbd->numOfBytes, bmLength));
                                UDFPrint(("UDF BM: FSBM_BitCount=%x FSBM_ByteCount=%x BitmapDataOffset=%x\n",
                                    Vcb->FSBM_BitCount, Vcb->FSBM_ByteCount, Vcb->BitmapDataOffset));

                                // Unpin — data will be pinned on demand
                                UDFUnpinBitmapPage(Vcb);
                            } else {
                                UDFPrint(("CcPinRead bitmap header failed: %x\n", RC));
                                Vcb->BitmapBcb = NULL;
                                UDFDeleteBitmapStream(Vcb);
                                RC = STATUS_SUCCESS; // fall back to legacy
                            }
                        } else {
                            UDFPrint(("UDFCreateBitmapStream failed: %x\n", RC));
                            RC = STATUS_SUCCESS; // fall back to legacy path
                        }
                    }
                }

                RC = UDFBuildFreeSpaceBitmap(IrpContext, Vcb, i, phd, 0);
                //Vcb->Modified = FALSE;
                UDFPreClrModified(Vcb);
                UDFClrModified(Vcb);
                if (!NT_SUCCESS(RC))
                    return RC;

                if ((Vcb->Partitions[i].PartitionType == UDF_VIRTUAL_MAP15) ||
                    (Vcb->Partitions[i].PartitionType == UDF_VIRTUAL_MAP20)) {
                    RC = UDFLoadVAT(IrpContext, Vcb, i);
                    if (!NT_SUCCESS(RC))
                        return RC;
                    Vcb->LastModifiedTrack = 0;
                }
            }
        }
    }

    if (
        Found && !Vcb->MetadataFileInfo &&
        (Vcb->MetadataRef < Vcb->PartitionMaps) &&
        (Vcb->Partitions[Vcb->MetadataRef].PartitionType == UDF_METADATA_MAP25) &&
        (Vcb->Partitions[Vcb->MetadataRef].PartitionNum == p->partitionNumber)
    ) {
        RC = UDFLoadMetadata(IrpContext, Vcb, Vcb->MetadataRef);
        
        if (!NT_SUCCESS(RC))
            return RC;
    }

#ifdef UDF_DBG
    if (!Found) {
        UDFPrint(("Partition (%d) not found in partition map\n", (p->partitionNumber) ));
    } else {
        UDFPrint(("Partition (%d:%d type %x) starts at physical %x, length %x\n",
            p->partitionNumber, i-1, Vcb->Partitions[i-1].PartitionType,
            Vcb->Partitions[i-1].PartitionRoot, Vcb->Partitions[i-1].PartitionLen));
    }
#endif // UDF_DBG
    return STATUS_SUCCESS;
} // end UDFLoadPartDesc()

/*
    process Partition descriptor
 */
NTSTATUS
UDFVerifyPartDesc(
    IN PIRP_CONTEXT IrpContext,
    PVCB      Vcb,
    int8*     Buf
    )
{
    struct PartitionDesc *p = (struct PartitionDesc *)Buf;
    uint32 i;
    NTSTATUS RC;
    BOOLEAN Found = FALSE;
    UDFPrint(("UDF: Verify Part Descr:\n"));
    UDFPrint((" volDescSeqNum   = %x\n", p->volDescSeqNum));
    UDFPrint((" partitionFlags  = %x\n", p->partitionFlags));
    UDFPrint((" partitionNumber = %x\n", p->partitionNumber));
    UDFPrint((" accessType      = %x\n", p->accessType));
    UDFPrint((" partitionStartingLocation = %x\n", p->partitionStartingLocation));
    UDFPrint((" partitionLength = %x\n", p->partitionLength));
    // There is nothing interesting to comment here
    // Just look at Names & Messages....
    for (i=0; i<Vcb->PartitionMaps; i++) {
        UDFPrint(("Searching map: (%d == %d)\n",
            Vcb->Partitions[i].PartitionNum, (p->partitionNumber) ));
        if (Vcb->Partitions[i].PartitionNum == (p->partitionNumber)) {
            Found = TRUE;
            if(Vcb->Partitions[i].PartitionType == UDF_METADATA_MAP25)
                continue;
            if (Vcb->Partitions[i].PartitionRoot != p->partitionStartingLocation)
                return STATUS_DISK_CORRUPT_ERROR;
            if (Vcb->Partitions[i].PartitionLen != p->partitionLength)
                return STATUS_DISK_CORRUPT_ERROR;
//            Vcb->Partitions[i].UspaceBitmap = 0xFFFFFFFF;
//            Vcb->Partitions[i].FspaceBitmap = 0xFFFFFFFF;
            if (Vcb->Partitions[i].AccessType != p->accessType)
                return STATUS_DISK_CORRUPT_ERROR;
            UDFPrint(("Access mode %x\n", p->accessType));
            if (p->accessType == PARTITION_ACCESS_WO) {
                if (Vcb->CDR_Mode != TRUE)
                    return STATUS_DISK_CORRUPT_ERROR;
//                Vcb->Partitions[i].PartitionLen = Vcb->LastPossibleLBA - p->partitionStartingLocation;
            } else if (p->accessType < PARTITION_ACCESS_WO) {
                // Soft-read-only volume
                UDFPrint(("Soft Read-only volume\n"));
                if (!(Vcb->VcbState & VCB_STATE_VOLUME_READ_ONLY))
                    return STATUS_DISK_CORRUPT_ERROR;
            } else if (p->accessType > PARTITION_ACCESS_MAX_KNOWN) {
                return STATUS_UNRECOGNIZED_MEDIA;
            }

            if (!strcmp((int8*)&(p->partitionContents.ident), PARTITION_CONTENTS_NSR02) ||
                !strcmp((int8*)&(p->partitionContents.ident), PARTITION_CONTENTS_NSR03))
            {
                PPARTITION_HEADER_DESC phd;

                phd = (PPARTITION_HEADER_DESC)(p->partitionContentsUse);
#ifdef UDF_DBG
                if (phd->unallocatedSpaceTable.extLength)
                    UDFPrint(("unallocatedSpaceTable (part %d)\n", i));
#endif // UDF_DBG
                if (phd->unallocatedSpaceBitmap.extLength) {
                    if (Vcb->Partitions[i].UspaceBitmap ==
                        phd->unallocatedSpaceBitmap.extPosition) {
                        UDFPrint(("Warning: both USpaceBitmaps have same location\n"));
                    }
                    UDFPrint(("unallocatedSpaceBitmap (part %d) @ %x\n",
                        i, Vcb->Partitions[i].UspaceBitmap ));
                }
#ifdef UDF_DBG
                if (phd->partitionIntegrityTable.extLength)
                    UDFPrint(("partitionIntegrityTable (part %d)\n", i));
                if (phd->freedSpaceTable.extLength)
                    UDFPrint(("freedSpaceTable (part %d)\n", i));
#endif // UDF_DBG
                if (phd->freedSpaceBitmap.extLength) {
                    if (Vcb->Partitions[i].FspaceBitmap ==
                        phd->freedSpaceBitmap.extPosition) {
                        UDFPrint(("Warning: both FSpaceBitmaps have same location\n"));
                    }
                    UDFPrint(("freedSpaceBitmap (part %d)\n", i));
                }
                RC = UDFVerifyFreeSpaceBitmap(IrpContext, Vcb, i, phd, 0);
                //Vcb->Modified = FALSE;
                //UDFPreClrModified(Vcb);
                //UDFClrModified(Vcb);
                if (!NT_SUCCESS(RC))
                    return RC;

                if ((Vcb->Partitions[i].PartitionType == UDF_VIRTUAL_MAP15) ||
                    (Vcb->Partitions[i].PartitionType == UDF_VIRTUAL_MAP20)) {
/*                    RC = UDFLoadVAT(Vcb, i);
                    if (!NT_SUCCESS(RC))
                        return RC;
                    WCacheFlushAll__(&(Vcb->FastCache), Vcb);
                    WCacheSetMode__(&(Vcb->FastCache), WCACHE_MODE_R);
                    Vcb->LastModifiedTrack = 0;*/
                }
            }
        }
    }
#ifdef UDF_DBG
    if (!Found) {
        UDFPrint(("Partition (%d) not found in partition map\n", (p->partitionNumber) ));
    } else {
        UDFPrint(("Partition (%d:%d type %x) starts at physical %x, length %x\n",
            p->partitionNumber, i-1, Vcb->Partitions[i-1].PartitionType,
            Vcb->Partitions[i-1].PartitionRoot, Vcb->Partitions[i-1].PartitionLen));
    }
#endif // UDF_DBG
    return STATUS_SUCCESS;
} // end UDFVerifyPartDesc()

/*
    This routine scans VDS & fills special array with Desc locations
 */
NTSTATUS
UDFReadVDS(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN uint32 block,
    IN uint32 lastblock,
    IN PUDF_VDS_RECORD vds,
    IN int8* Buf
    )
{
    NTSTATUS status;
    struct GenericDesc* gd;
    BOOLEAN done=FALSE;
    uint32 vdsn;
    uint16 ident;

    UDFPrint(("UDF: Read VDS (%x - %x)\n", block, lastblock ));
    // Read the main descriptor sequence
    for (;(!done && block <= lastblock); block++)
    {
        status = UDFReadTagged(IrpContext, Vcb, Buf, block, block, &ident);
        if (!NT_SUCCESS(status))
            return status;

        // Process each descriptor (ISO 13346 3/8.3-8.4)
        gd = (struct GenericDesc *)Buf;
        vdsn = gd->volDescSeqNum;
        UDFPrint(("LBA %x, Ident = %x, vdsn = %x\n", block, ident, vdsn ));
        switch (ident)
        {
            case TID_PRIMARY_VOL_DESC: // ISO 13346 3/10.1
                if (vdsn >= vds[VDS_POS_PRIMARY_VOL_DESC].volDescSeqNum)
                {
                    vds[VDS_POS_PRIMARY_VOL_DESC].volDescSeqNum = vdsn;
                    vds[VDS_POS_PRIMARY_VOL_DESC].block = block;
                }
                break;
            case TID_VOL_DESC_PTR: // ISO 13346 3/10.3
                {
                    struct VolDescPtr* pVDP;
                    if (vdsn >= vds[VDS_POS_VOL_DESC_PTR].volDescSeqNum)
                    {
                        vds[VDS_POS_VOL_DESC_PTR].volDescSeqNum = vdsn;
                        vds[VDS_POS_VOL_DESC_PTR].block = block;
                        vds[VDS_POS_RECURSION_COUNTER].volDescSeqNum++;
                        if (vds[VDS_POS_RECURSION_COUNTER].volDescSeqNum > MAX_VDS_PARTS) {
                           UDFPrint(("too long multipart VDS -> abort\n"));
                            return STATUS_DISK_CORRUPT_ERROR;
                        }
                        pVDP = (struct VolDescPtr*)Buf;
                        UDFPrint(("multipart VDS...\n"));
                        return UDFReadVDS(IrpContext, Vcb, pVDP->nextVolDescSeqExt.extLocation,
                                             pVDP->nextVolDescSeqExt.extLocation + (pVDP->nextVolDescSeqExt.extLength >> Vcb->SectorShift),
                                             vds, Buf);
                    }
                }
                break;
            case TID_IMP_USE_VOL_DESC: // ISO 13346 3/10.4
                if (vdsn >= vds[VDS_POS_IMP_USE_VOL_DESC].volDescSeqNum)
                {
                    vds[VDS_POS_IMP_USE_VOL_DESC].volDescSeqNum = vdsn;
                    vds[VDS_POS_IMP_USE_VOL_DESC].block = block;
                }
                break;
            case TID_PARTITION_DESC: // ISO 13346 3/10.5
                if (!vds[VDS_POS_PARTITION_DESC].block)
                    vds[VDS_POS_PARTITION_DESC].block = block;
                break;
            case TID_LOGICAL_VOL_DESC: // ISO 13346 3/10.6
            case TID_ADAPTEC_LOGICAL_VOL_DESC: // Adaptec Compressed UDF extesion
                if (vdsn >= vds[VDS_POS_LOGICAL_VOL_DESC].volDescSeqNum)
                {
                    vds[VDS_POS_LOGICAL_VOL_DESC].volDescSeqNum = vdsn;
                    vds[VDS_POS_LOGICAL_VOL_DESC].block = block;
                }
                break;
            case TID_UNALLOC_SPACE_DESC: // ISO 13346 3/10.8
                if (vdsn >= vds[VDS_POS_UNALLOC_SPACE_DESC].volDescSeqNum)
                {
                    vds[VDS_POS_UNALLOC_SPACE_DESC].volDescSeqNum = vdsn;
                    vds[VDS_POS_UNALLOC_SPACE_DESC].block = block;
                }
                break;
            case TID_TERMINATING_DESC: // ISO 13346 3/10.9
                vds[VDS_POS_TERMINATING_DESC].block = block;
                done = TRUE;
                break;
        }
    }
    return STATUS_SUCCESS;
} // UDFReadVDS()

NTSTATUS
UDFLoadImpUseVolDesc(
    IN PVCB   Vcb,
    int8*     Buf
    )
{
#ifdef UDF_DBG
    struct ImpUseVolDesc* iuvd = (struct ImpUseVolDesc*)Buf;
    struct ImpUseVolDescImpUse* iuvdiu = (struct ImpUseVolDescImpUse*)&(iuvd->impUse);
    UDFPrint(("UDF: Imp Use Vol Desc:\n"));
    UDFPrint((" volDescSeqNum = %x\n", iuvd->volDescSeqNum));
    UDFPrint(("UDF: Imp Use Vol Desc Imp Use:\n"));
    KdDump(iuvdiu, sizeof(struct ImpUseVolDescImpUse));
#endif
    return STATUS_SUCCESS;
} // UDFLoadImpUseVolDesc()

NTSTATUS
UDFLoadUnallocatedSpaceDesc(
    IN PVCB   Vcb,
    int8*     Buf
    )
{
    UDFPrint(("UDF: Unallocated Space Desc:\n"));
//    UnallocatedSpaceDesc* usd = (UnallocatedSpaceDesc*)Buf;
    return STATUS_SUCCESS;
} // UDFLoadImpUseVolDesc()

/*
    Process a main/reserve volume descriptor sequence.
*/
NTSTATUS
UDFProcessSequence(
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT DeviceObject,
    IN PVCB Vcb,
    IN uint32 block,
    IN uint32 lastblock,
    OUT lb_addr *fileset,
    OUT UDF_VDS_RECORD *volDesc
    )
{
    NTSTATUS    RC = STATUS_SUCCESS;
    int8*       Buf = (int8*)MyAllocatePool__(NonPagedPool,Vcb->SectorSize);
    UDF_VDS_RECORD vds[VDS_POS_LENGTH];
//    GenericDesc   *gd;
    uint32   i,j;
    uint16  ident;
    int8*  Buf2 = NULL;

    _SEH2_TRY {
        if (!Buf) try_return(RC = STATUS_INSUFFICIENT_RESOURCES);
        RtlZeroMemory(vds, sizeof(UDF_VDS_RECORD) * VDS_POS_LENGTH);
        if (!NT_SUCCESS(RC = UDFReadVDS(IrpContext, Vcb, block, lastblock, (PUDF_VDS_RECORD)&vds, Buf)))
            try_return(RC);
        // walk through Vol Desc Sequence according to locations gained by
        // UDFReadVDS() & do some procesing for each one
        // It is very simple dispath routine...
        for (i=0; i<VDS_POS_LENGTH; i++)
        {
            if (vds[i].block)
            {
                if (!NT_SUCCESS(RC = UDFReadTagged(IrpContext, Vcb, Buf, vds[i].block, vds[i].block, &ident)))
                    try_return(RC);

                if (i == VDS_POS_PRIMARY_VOL_DESC) {
                    UDFLoadPVolDesc(Vcb,Buf);
                    *volDesc = vds[i];
                } else
                if (i == VDS_POS_LOGICAL_VOL_DESC) {
                    RC = UDFLoadLogicalVol(IrpContext, DeviceObject, Vcb, Buf, fileset);
                    if (!NT_SUCCESS(RC)) try_return(RC);
                } else

                if (i == VDS_POS_IMP_USE_VOL_DESC) {
                    UDFLoadImpUseVolDesc(Vcb, Buf);
                } else
                if (i == VDS_POS_UNALLOC_SPACE_DESC) {
                    UDFLoadUnallocatedSpaceDesc(Vcb, Buf);
                } else

                if (i == VDS_POS_PARTITION_DESC)
                {
                    Buf2 = (int8*)MyAllocatePool__(NonPagedPool,Vcb->SectorSize);
                    if (!Buf2) try_return(RC = STATUS_INSUFFICIENT_RESOURCES);
                    RC = UDFLoadPartDesc(IrpContext, Vcb, Buf);
                    if (!NT_SUCCESS(RC)) try_return(RC);
                    for (j=vds[i].block+1; j<vds[VDS_POS_TERMINATING_DESC].block; j++)
                    {
                        RC = UDFReadTagged(IrpContext, Vcb, Buf2, j, j, &ident);
                        if (!NT_SUCCESS(RC)) try_return(RC);

//                        gd = (struct GenericDesc *)Buf2;
                        if (ident == TID_PARTITION_DESC) {
                            RC = UDFLoadPartDesc(IrpContext, Vcb, Buf2);
                            if (!NT_SUCCESS(RC)) try_return(RC);
                        }
                    }
                    MyFreePool__(Buf2);
                    Buf2 = NULL;
                }
            } else {
                if (i == VDS_POS_LOGICAL_VOL_DESC) {

                    RC = UDFLoadBogusLogicalVol(DeviceObject,Vcb, Buf, fileset);
                    if (!NT_SUCCESS(RC)) try_return(RC);
                }
            }
        }

try_exit: NOTHING;

    } _SEH2_FINALLY {
        if (Buf) MyFreePool__(Buf);
        if (Buf2) MyFreePool__(Buf2);
    } _SEH2_END;

    return RC;
} // end UDFProcessSequence()

/*
    Verifies a main/reserve volume descriptor sequence.
*/
NTSTATUS
UDFVerifySequence(
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT    DeviceObject,
    IN PVCB              Vcb,
    IN uint32             block,
    IN uint32             lastblock,
    OUT lb_addr          *fileset,
    OUT UDF_VDS_RECORD   *volDesc
    )
{
    NTSTATUS    RC = STATUS_SUCCESS;
    int8*       Buf = (int8*)MyAllocatePool__(NonPagedPool,Vcb->SectorSize);
    UDF_VDS_RECORD vds[VDS_POS_LENGTH];
//    GenericDesc   *gd;
    uint32   i,j;
    uint16  ident;
    int8*  Buf2 = NULL;

    _SEH2_TRY {
        if (!Buf) try_return(RC = STATUS_INSUFFICIENT_RESOURCES);
        if (!block) try_return (RC = STATUS_SUCCESS);
        RtlZeroMemory(vds, sizeof(UDF_VDS_RECORD) * VDS_POS_LENGTH);
        if (!NT_SUCCESS(RC = UDFReadVDS(IrpContext, Vcb, block, lastblock, (PUDF_VDS_RECORD)&vds, Buf)))
            try_return(RC);

        for (i=0; i<VDS_POS_LENGTH; i++)
        {
            if (vds[i].block)
            {
                if (!NT_SUCCESS(RC = UDFReadTagged(IrpContext, Vcb, Buf, vds[i].block, vds[i].block, &ident)))
                    try_return(RC);

                if (i == VDS_POS_PRIMARY_VOL_DESC) {
                    *volDesc = vds[i];
                }
                else if (i == VDS_POS_LOGICAL_VOL_DESC) {
                }
                else if (i == VDS_POS_PARTITION_DESC)
                {
                    Buf2 = (int8*)MyAllocatePool__(NonPagedPool,Vcb->SectorSize);
                    if (!Buf2) try_return(RC = STATUS_INSUFFICIENT_RESOURCES);
                    RC = UDFVerifyPartDesc(IrpContext, Vcb, Buf);
                    if (!NT_SUCCESS(RC)) try_return(RC);
                    for (j=vds[i].block+1; j<vds[VDS_POS_TERMINATING_DESC].block; j++)
                    {
                        RC = UDFReadTagged(IrpContext, Vcb, Buf2, j, j, &ident);
                        if (!NT_SUCCESS(RC)) try_return(RC);

                        // gd = (struct GenericDesc *)Buf2;
                        if (ident == TID_PARTITION_DESC) {
                            RC = UDFVerifyPartDesc(IrpContext, Vcb, Buf2);
                            if (!NT_SUCCESS(RC)) {
                                try_return(RC);
                            }
                        }
                    }
                    MyFreePool__(Buf2);
                    Buf2 = NULL;
                }
            }
        }
try_exit: NOTHING;

    } _SEH2_FINALLY {
        if (Buf) MyFreePool__(Buf);
        if (Buf2) MyFreePool__(Buf2);
    } _SEH2_END;

    return RC;
} // end UDFVerifySequence()

static
ULONG
GetPhSerialNum(
    IN PFILE_SET_DESC  fset
)
{
    UCHAR Sum[4];
    ULONG Count = sizeof(FILE_SET_DESC);
    UCHAR* Byte = (UCHAR*)fset;
    
    RtlZeroMemory(Sum, sizeof(Sum));

    while (Count--)
        Sum[Count & 0b11] += *(Byte++);

    return ((ULONG)Sum[0] << 24) | ((ULONG)Sum[1] << 16) | ((ULONG)Sum[2] << 8) | (Sum[3] << 0);
}

/*
    remember some useful info about FileSet & RootDir location
 */
void
UDFLoadFileset(
     IN PVCB            Vcb,
     IN PFILE_SET_DESC  fset,
    OUT lb_addr         *root,
    OUT lb_addr         *sysstream
    )
{
    *root = fset->rootDirectoryICB.extLocation;
    Vcb->SerialNumber = fset->descTag.tagSerialNum;
    Vcb->PhSerialNumber = GetPhSerialNum(fset);
    UDFPrint(("Rootdir at block=%x, partition=%d\n",
        root->logicalBlockNum, root->partitionReferenceNum));
    if (sysstream) {
        *sysstream = fset->streamDirectoryICB.extLocation;
        UDFPrint(("SysStream at block=%x, partition=%d\n",
            sysstream->logicalBlockNum, sysstream->partitionReferenceNum));
    }
#define CUR_IDENT_SZ (sizeof(fset->logicalVolIdent))
    if (Vcb->VolIdent.Buffer) {
        MyFreePool__(Vcb->VolIdent.Buffer);
    }
    UDFGetDstring(&(Vcb->VolIdent), (dstring*)&(fset->logicalVolIdent), CUR_IDENT_SZ);
#undef CUR_IDENT_SZ
    UDFPrint(("volIdent[] = '%wZ'\n", &Vcb->VolIdent));
    // Get current UDF revision
    // Get Read-Only flags
    UDFReadEntityID_Domain(Vcb, &(fset->domainIdent));

} // end UDFLoadFileset()

/*
    load partition info
 */
NTSTATUS
UDFLoadPartition(
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT DeviceObject,
    IN PVCB Vcb,
    OUT lb_addr* fileset
    )
{
    NTSTATUS            RC = STATUS_UNRECOGNIZED_VOLUME;
    struct AnchorVolDescPtr    *anchor;
    uint16              ident;
    int8*               Buf = (int8*)MyAllocatePool__(NonPagedPool,Vcb->SectorSize);
    uint32              main_s, main_e;
    uint32              reserve_s, reserve_e;
    UDF_VDS_RECORD      mainVolDesc;
    UDF_VDS_RECORD      reserveVolDesc;
    int                 i;

    if (!Buf) return STATUS_INSUFFICIENT_RESOURCES;
    // Find first valid anchor
    for (i=0; i<MAX_ANCHOR_LOCATIONS; i++)
    {
        if (Vcb->Anchor[i] && (NT_SUCCESS(UDFReadTagged(IrpContext, Vcb, Buf,
            Vcb->Anchor[i], Vcb->Anchor[i] - Vcb->SessionStartLba, &ident))))
        {
            break;
        }
    }

    if (i >= MAX_ANCHOR_LOCATIONS) {
        UDFPrint(("No Anchor block found\n"));
        MyFreePool__(Buf);
        return STATUS_UNRECOGNIZED_VOLUME;
    }

    UDFPrint(("Using anchor in block %x\n", Vcb->Anchor[i]));
    anchor = (struct AnchorVolDescPtr *)Buf;

    // Locate the main sequence
    main_s = ( anchor->mainVolDescSeqExt.extLocation );
    main_e = ( anchor->mainVolDescSeqExt.extLength );
    main_e = main_e >> Vcb->SectorShift;
    main_e += main_s;

    // Locate the reserve sequence
    reserve_s = (anchor->reserveVolDescSeqExt.extLocation);
    reserve_e = (anchor->reserveVolDescSeqExt.extLength);
    reserve_e = reserve_e >> Vcb->SectorShift;
    reserve_e += reserve_s;

    // Process the main sequence
    UDFPrint(("-----------------------------------\n"));
    UDFPrint(("UDF: Main sequence:\n"));
    RC = UDFProcessSequence(IrpContext, DeviceObject, Vcb, main_s, main_e, fileset, &mainVolDesc);

    if (!NT_SUCCESS(RC)) {

        // Main failed — try the reserve sequence
        UDFPrint(("-----------------------------------\n"));
        UDFPrint(("UDF: Main sequence failed.\n"));
        UDFPrint(("UDF: Reserve sequence\n"));
        if (Vcb->LVid) MyFreePool__(Vcb->LVid);
        Vcb->LVid = NULL;

        RC = UDFProcessSequence(IrpContext, DeviceObject, Vcb, reserve_s, reserve_e, fileset, &reserveVolDesc);

        if (NT_SUCCESS(RC)) {
            Vcb->VDS2_Len = reserve_e - reserve_s;
            Vcb->VDS2 = reserve_s;
            Vcb->PVolDescAddr2 = reserveVolDesc;
        }
    } else {
        // Main succeeded — verify the reserve sequence
        Vcb->VDS1_Len = main_e - main_s;
        Vcb->VDS1 = main_s;
        Vcb->PVolDescAddr = mainVolDesc;

        if (NT_SUCCESS(UDFVerifySequence(IrpContext, DeviceObject, Vcb, reserve_s, reserve_e, fileset, &reserveVolDesc)))
        {
            UDFPrint(("-----------------------------------\n"));
            Vcb->VDS2_Len = reserve_e - reserve_s;
            Vcb->VDS2 = reserve_s;
            Vcb->PVolDescAddr2 = reserveVolDesc;
        } else {
            UDFPrint(("UDF: Reserve sequence verification failed.\n"));
        }
    }

    if (Vcb->SparingCount &&
       (Vcb->NoFreeRelocationSpaceVolumeAction != UDF_PART_DAMAGED_RW)) {
        UDFPrint(("UDF: No free Sparing Entries -> Switch to r/o mode.\n"));
        Vcb->VcbState |= VCB_STATE_VOLUME_READ_ONLY;
    }
    MyFreePool__(Buf);
    return RC;
} // end UDFLoadPartition()

/*
    This routine scans FileSet sequence & returns pointer to last valid
    FileSet
 */
NTSTATUS
UDFFindLastFileSet(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN lb_addr *Addr,  // Addr for the 1st FileSet
    IN OUT PFILE_SET_DESC FileSetDesc
    )
{
    NTSTATUS status;
    uint32 relLocExt = Addr->logicalBlockNum;
    uint32 locExt = UDFPartLbaToPhys(Vcb, Addr);
    uint16 Ident;
    uint32 relPrevExt, prevExt;

    relPrevExt = prevExt = 0;
    FileSetDesc->nextExt.extLength = 1;  // ;)
    // walk through FileSet chain
    // we've just pre-init'd extent length to read 1st FileSet
    while(FileSetDesc->nextExt.extLength) {
        status = UDFReadTagged(IrpContext, Vcb, (int8*)FileSetDesc, locExt, relLocExt, &Ident);
        if (!NT_SUCCESS(status)) {
            FileSetDesc->nextExt.extLength = 0;
            return status;
        }

        if ((locExt == LBA_OUT_OF_EXTENT) || (Ident != TID_FILE_SET_DESC)) {
            // try to read previous FileSet
            if (!prevExt) return STATUS_UNRECOGNIZED_VOLUME;
            status = UDFReadTagged(IrpContext, Vcb, (int8*)FileSetDesc, prevExt, relLocExt, &Ident);

            return status;
        }
        prevExt = locExt;
        relPrevExt = relLocExt;
        locExt = UDFPartLbaToPhys(Vcb, &(FileSetDesc->nextExt.extLocation));
    }
    return STATUS_SUCCESS;
} // end UDFFindLastFileSet()

/*
    This routine reads all sparing tables & stores them in contiguos memory
    space
 */
NTSTATUS
UDFLoadSparingTable(
    IN PIRP_CONTEXT IrpContext,
    IN PVCB Vcb,
    IN PSPARABLE_PARTITION_MAP PartMap
    )
{
    PSPARING_MAP RelocMap;
    PSPARING_MAP NewRelocMap;
    NTSTATUS status;
    uint32 i=0, BC, BC2;
    PSPARING_TABLE SparTable;
    uint32 TabSize, NewSize;
    uint32 SparTableLoc;
    uint32 n,m;
    BOOLEAN merged;

    Vcb->SparingCountFree = -1;

    UDFPrint(("UDF: Sparable Part Map:\n"));
    Vcb->SparingTableLength = PartMap->sizeSparingTable;
    BC = (PartMap->sizeSparingTable >> Vcb->SectorShift) + 1;
    UDFPrint((" partitionMapType   = %x\n", PartMap->partitionMapType));
    UDFPrint((" partitionMapLength = %x\n", PartMap->partitionMapLength));
    UDFPrint((" volSeqNum          = %x\n", PartMap->volSeqNum));
    UDFPrint((" partitionNum       = %x\n", PartMap->partitionNum));
    UDFPrint((" packetLength       = %x\n", PartMap->packetLength));
    UDFPrint((" numSparingTables   = %x\n", PartMap->numSparingTables));
    UDFPrint((" sizeSparingTable   = %x\n", PartMap->sizeSparingTable));
    SparTable = (PSPARING_TABLE)MyAllocatePool__(NonPagedPool, BC*Vcb->SectorSize);
    if (!SparTable) return STATUS_INSUFFICIENT_RESOURCES;
    if (Vcb->SparingTable) {
        // if a part of Sparing Table is already loaded,
        // update it with data from another one
        RelocMap = Vcb->SparingTable;
        TabSize = Vcb->SparingCount * sizeof(SPARING_ENTRY);
    } else {
        // do some init to load first part of Sparing Table
        RelocMap = (PSPARING_MAP)MyAllocatePool__(NonPagedPool, RELOC_MAP_GRAN);
        if (!RelocMap) {
            MyFreePool__(SparTable);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        TabSize = RELOC_MAP_GRAN;
        Vcb->SparingBlockSize = PartMap->packetLength;
    }
    // walk through all available Sparing Tables
    for(i=0;i<PartMap->numSparingTables;i++) {
        // read (next) table
        SparTableLoc = ((uint32*)(PartMap+1))[i];
        for(n=0; n<Vcb->SparingTableCount; n++) {
            if (Vcb->SparingTableLoc[i] == SparTableLoc) {
                UDFPrint((" already processed @%x\n",
                    SparTableLoc
                    ));
                continue;
            }
        }
        status = UDFReadSectors(IrpContext, Vcb, FALSE, SparTableLoc, 1, FALSE, (int8*)SparTable);
        // tag should be set to TID_UNUSED_DESC
        if (NT_SUCCESS(status) && (SparTable->descTag.tagIdent == TID_UNUSED_DESC)) {

            BC2 = ((sizeof(SPARING_TABLE) +
                    SparTable->reallocationTableLen*sizeof(SparingEntry) +
                    Vcb->SectorSize-1)
                                      >> Vcb->SectorShift);
            if (BC2 > BC) {
                UDFPrint((" sizeSparingTable @%x too long: %x > %x\n",
                    SparTableLoc, BC2, BC
                    ));
                continue;
            }
            status = UDFReadSectors(IrpContext, Vcb, FALSE, SparTableLoc,
                BC2, FALSE, (int8*)SparTable);

            if (!NT_SUCCESS(status)) {
                UDFPrint((" Error reading sizeSparingTable @%x (%x)\n",
                    SparTableLoc, BC2
                    ));
                continue;
            }
            // process sparing table
            NewSize = sizeof(SparingEntry)*SparTable->reallocationTableLen;
            TabSize = MyReallocPool__((int8*)RelocMap, TabSize, (int8**)&RelocMap, TabSize+NewSize);
            if (!TabSize) {
                MyFreePool__(SparTable);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            Vcb->SparingTableLoc[Vcb->SparingTableCount] = SparTableLoc;
            Vcb->SparingTableCount++;

            NewRelocMap = (PSPARING_MAP)(SparTable+1);
            for(n=0; n<SparTable->reallocationTableLen; n++) {
                merged = TRUE;
                for(m=0; m<Vcb->SparingCount; m++) {
                    if (RelocMap[m].mappedLocation == NewRelocMap[n].mappedLocation) {
                        UDFPrint(("  dup @%x (%x) vs @%x (%x)\n",
                            RelocMap[m].origLocation, RelocMap[m].mappedLocation,
                            NewRelocMap[m].origLocation, NewRelocMap[m].mappedLocation));
                        merged = FALSE;
                    }
                    if ((RelocMap[m].origLocation   == NewRelocMap[n].origLocation) &&
                       (RelocMap[m].mappedLocation != NewRelocMap[n].mappedLocation) &&
                       (RelocMap[m].origLocation != SPARING_LOC_AVAILABLE) &&
                       (RelocMap[m].origLocation != SPARING_LOC_CORRUPTED)) {
                        UDFPrint(("  conflict @%x (%x) vs @%x (%x)\n",
                            RelocMap[m].origLocation, RelocMap[m].mappedLocation,
                            NewRelocMap[n].origLocation, NewRelocMap[n].mappedLocation));
                        merged = FALSE;
                    }
                }
                if (merged) {
                    RelocMap[Vcb->SparingCount] = NewRelocMap[n];
                    UDFPrint(("  reloc %x -> %x\n",
                        RelocMap[Vcb->SparingCount].origLocation, RelocMap[Vcb->SparingCount].mappedLocation));
                    Vcb->SparingCount++;
                    if (RelocMap[Vcb->SparingCount].origLocation == SPARING_LOC_AVAILABLE) {
                        Vcb->NoFreeRelocationSpaceVolumeAction = UDF_PART_DAMAGED_RW;
                    }
                }
            }

/*
            RtlCopyMemory((int8*)(RelocMap+Vcb->SparingCount),
                          (int8*)(SparTable+1), NewSize);
            Vcb->SparingCount += NewSize/sizeof(SPARING_ENTRY);
*/
            if (Vcb->SparingTableCount >= MAX_SPARING_TABLE_LOCATIONS) {
                UDFPrint(("    too many Sparing Tables\n"));
                break;
            }
        }
    }
    Vcb->SparingTable = RelocMap;
    MyFreePool__(SparTable);
    return STATUS_SUCCESS;
} // end UDFLoadSparingTable()

/*
    This routine checks if buffer is ZERO-filled
 */
__inline
BOOLEAN
UDFCheckZeroBuf(
    IN int8* Buf,
    IN uint32 Length
    )
{
    uint32* tmp = (uint32*)Buf;
    uint32 i;

    for(i=0; i<Length/4; i++) {
        if (tmp[i]) return FALSE;
    }
    return TRUE;
} // end UDFCheckZeroBuf()

/*
    check if this is an UDF-formatted disk
*/
NTSTATUS
UDFGetDiskInfoAndVerify(
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT DeviceObject,      // the target device object
    IN PVCB           Vcb                // Volume control block from this DevObj
    )
{
    NTSTATUS        RC = STATUS_UNRECOGNIZED_VOLUME;
    uint32          NSRDesc;
    lb_addr         fileset;
    PFILE_SET_DESC  FileSetDesc = NULL;

    UDFPrint(("UDFGetDiskInfoAndVerify\n"));
    _SEH2_TRY {

        if (!UDFFindAnchorVolumeDescriptor(IrpContext, Vcb)) {

            try_return(RC = STATUS_UNRECOGNIZED_VOLUME);
        }

        if (
            (DeviceObject->DeviceType == FILE_DEVICE_DISK) &&
            (!(UDFFindVRS(IrpContext, Vcb) & (VRS_NSR02_FOUND | VRS_NSR03_FOUND)) ||
            UDFHasOtherFsBootSector(IrpContext, Vcb))
        ) {
            try_return(RC = STATUS_UNRECOGNIZED_VOLUME);
        }

        NSRDesc = VRS_NSR03_FOUND | VRS_NSR02_FOUND;
        Vcb->NSRDesc = NSRDesc;

        RC = UDFLoadPartition(IrpContext, DeviceObject, Vcb, &fileset);

        if (!NT_SUCCESS(RC)) {

            try_return(RC);
        }

        FileSetDesc = (PFILE_SET_DESC)FsRtlAllocatePoolWithTag(PagedPool, Vcb->SectorSize, TAG_FILE_SET_DESC);

        RC = UDFFindLastFileSet(IrpContext, Vcb, &fileset, FileSetDesc);
        if (!NT_SUCCESS(RC)) try_return(RC);

        UDFLoadFileset(Vcb, FileSetDesc, &Vcb->RootLbAddr, &Vcb->SysStreamLbAddr);

        if (!Vcb->BitmapFcb) {
            // Legacy: keep a copy for delta comparison during flush
            Vcb->FSBM_OldBitmap = (int8*)DbgAllocatePool(NonPagedPool, Vcb->FSBM_ByteCount);
            if (!(Vcb->FSBM_OldBitmap)) try_return(RC = STATUS_INSUFFICIENT_RESOURCES);
            RtlCopyMemory(Vcb->FSBM_OldBitmap, Vcb->FSBM_Bitmap, Vcb->FSBM_ByteCount);
        }
        // Per-page mode: no OldBitmap needed, dirty tracking via Cache Manager

try_exit:   NOTHING;
    } _SEH2_FINALLY {

        if (FileSetDesc) {

            ExFreePoolWithTag(FileSetDesc, TAG_FILE_SET_DESC);
        }

    } _SEH2_END;

    return(RC);

} // end UDFGetDiskInfoAndVerify()

