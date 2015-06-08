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

/* FIXME*/
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

OSSTATUS
__fastcall
UDFSetDstring(
    IN PUNICODE_STRING UName,
    IN dstring* Dest,
    IN uint32 Length
    );

#ifndef UDF_READ_ONLY_BUILD
/*
    This routine loads specified bitmap.
    It is also allocate space if the bitmap is not allocated.
 */
OSSTATUS
UDFPrepareXSpaceBitmap(
    IN PVCB Vcb,
 IN OUT PSHORT_AD XSpaceBitmap,
 IN OUT PEXTENT_INFO XSBMExtInfo,
 IN OUT int8** XSBM,
 IN OUT uint32* XSl
    )
{
    uint32 BS, j, LBS;
    uint32 plen;
    OSSTATUS status;
    EXTENT_MAP TmpExt;
    lb_addr locAddr;
    int8* _XSBM;
    uint16 Ident;
    uint32 ReadBytes;
    uint32 PartNum;

    if(!(XSpaceBitmap->extLength)) {
        *XSl = 0;
        *XSBM = NULL;
        return STATUS_SUCCESS;
    }

    PartNum = UDFGetPartNumByPartNdx(Vcb, Vcb->PartitionMaps-1);
    locAddr.partitionReferenceNum = (uint16)PartNum;
    plen = UDFPartStart(Vcb, PartNum) + UDFPartLen(Vcb, PartNum);

    BS = Vcb->BlockSize;
    LBS = Vcb->LBlockSize;

    *XSl = sizeof(SPACE_BITMAP_DESC) + ((plen+7)>>3);
    _XSBM = (int8*)DbgAllocatePool(NonPagedPool, (*XSl + BS - 1) & ~(BS-1) );
    *XSBM = _XSBM;

    switch (XSpaceBitmap->extLength >> 30) {
    case EXTENT_RECORDED_ALLOCATED: {
        locAddr.logicalBlockNum = XSpaceBitmap->extPosition;
        *XSl = min(XSpaceBitmap->extLength, *XSl);
        TmpExt.extLength = XSpaceBitmap->extLength = *XSl;
        TmpExt.extLocation = UDFPartLbaToPhys(Vcb, &locAddr);
        if(TmpExt.extLocation == LBA_OUT_OF_EXTENT) {
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
        if(!OS_SUCCESS(status = UDFAllocFreeExtent(Vcb, *XSl,
               UDFPartStart(Vcb, PartNum), UDFPartEnd(Vcb, PartNum), XSBMExtInfo, EXTENT_FLAG_ALLOC_SEQUENTIAL) ))
            return status;
        if(XSBMExtInfo->Mapping[1].extLength) {
            KdPrint(("Can't allocate space for Freed Space bitmap\n"));
            *XSl = 0;
        } else {
            *XSl = (uint32)(XSBMExtInfo->Length);
            XSpaceBitmap->extPosition = UDFPhysLbaToPart(Vcb, PartNum, XSBMExtInfo->Mapping[0].extLocation);
        }
        break;
    }
    case EXTENT_NOT_RECORDED_ALLOCATED: {
        // record Alloc-Not-Rec
        locAddr.logicalBlockNum = XSpaceBitmap->extPosition;
        *XSl = min((XSpaceBitmap->extLength & UDF_EXTENT_LENGTH_MASK), *XSl);
        TmpExt.extLength = XSpaceBitmap->extLength = *XSl;
        TmpExt.extLocation = UDFPartLbaToPhys(Vcb, &locAddr);
        if(TmpExt.extLocation == LBA_OUT_OF_EXTENT) {
            BrutePoint();
        }
        XSBMExtInfo->Mapping = UDFExtentToMapping(&TmpExt);
        XSBMExtInfo->Offset = 0;
        XSBMExtInfo->Length = *XSl;
        break;
    }
    }

    if(!_XSBM) {
        BrutePoint();
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    switch (XSpaceBitmap->extLength >> 30) {
    case EXTENT_RECORDED_ALLOCATED: {
        // read descriptor & bitmap
        if((!OS_SUCCESS(status = UDFReadTagged(Vcb, *XSBM, (j = TmpExt.extLocation),
                             locAddr.logicalBlockNum, &Ident))) ||
           (Ident != TID_SPACE_BITMAP_DESC) ||
           (!OS_SUCCESS(status = UDFReadExtent(Vcb, XSBMExtInfo, 0, *XSl, FALSE, *XSBM, &ReadBytes))) ) {
            if(OS_SUCCESS(status)) {
                BrutePoint();
                status = STATUS_FILE_CORRUPT_ERROR;
            }
            if(XSBMExtInfo->Mapping) {
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
    UDFSetUpTag(Vcb, &(XSDesc->descTag), 0, XSpaceBitmap->extPosition);
    XSDesc->numOfBits = plen;
    XSDesc->numOfBytes = (*XSl)-sizeof(SPACE_BITMAP_DESC);

    return STATUS_SUCCESS;
} // end UDFPrepareXSpaceBitmap()

/*
    This routine updates Freed & Unallocated space bitmaps
 */
OSSTATUS
UDFUpdateXSpaceBitmaps(
    IN PVCB Vcb,
    IN uint32 PartNum,
    IN PPARTITION_HEADER_DESC phd // partition header pointing to Bitmaps
    )
{
    uint32 i,j,d;
    uint32 plen, pstart, pend;
    int8* bad_bm;
    int8* old_bm;
    int8* new_bm;
    int8* fpart_bm;
    int8* upart_bm;
    OSSTATUS status, status2;
    int8* USBM=NULL;
    int8* FSBM=NULL;
    uint32 USl, FSl;
    EXTENT_INFO FSBMExtInfo, USBMExtInfo;
    lb_addr locAddr;
    uint32 WrittenBytes;

    UDF_CHECK_BITMAP_RESOURCE(Vcb);

    plen = UDFPartLen(Vcb, PartNum);
    locAddr.partitionReferenceNum = (uint16)PartNum;
    // prepare bitmaps for updating

    status =  UDFPrepareXSpaceBitmap(Vcb, &(phd->unallocatedSpaceBitmap), &USBMExtInfo, &USBM, &USl);
    status2 = UDFPrepareXSpaceBitmap(Vcb, &(phd->freedSpaceBitmap), &FSBMExtInfo, &FSBM, &FSl);
    if(!OS_SUCCESS(status) ||
       !OS_SUCCESS(status2)) {
        BrutePoint();
    }

    pstart = UDFPartStart(Vcb, PartNum);
    new_bm = Vcb->FSBM_Bitmap;
    old_bm = Vcb->FSBM_OldBitmap;
    bad_bm = Vcb->BSBM_Bitmap;

    if((status  == STATUS_INSUFFICIENT_RESOURCES) ||
       (status2 == STATUS_INSUFFICIENT_RESOURCES)) {
        // try to recover insufficient resources
        if(USl && USBMExtInfo.Mapping) {
            USl -= sizeof(SPACE_BITMAP_DESC);
            status  = UDFWriteExtent(Vcb, &USBMExtInfo, sizeof(SPACE_BITMAP_DESC), USl, FALSE, new_bm, &WrittenBytes);
#ifdef UDF_DBG
        } else {
            KdPrint(("Can't update USBM\n"));
#endif // UDF_DBG
        }
        if(USBMExtInfo.Mapping) MyFreePool__(USBMExtInfo.Mapping);

        if(FSl && FSBMExtInfo.Mapping) {
            FSl -= sizeof(SPACE_BITMAP_DESC);
            status2 = UDFWriteExtent(Vcb, &FSBMExtInfo, sizeof(SPACE_BITMAP_DESC), FSl, FALSE, new_bm, &WrittenBytes);
        } else {
            status2 = status;
            KdPrint(("Can't update FSBM\n"));
        }
        if(FSBMExtInfo.Mapping) MyFreePool__(FSBMExtInfo.Mapping);
    } else {
        // normal way to record BitMaps
        if(USBM) upart_bm =  USBM + sizeof(SPACE_BITMAP_DESC);
        if(FSBM) fpart_bm =  FSBM + sizeof(SPACE_BITMAP_DESC);
        pend = min(pstart + plen, Vcb->FSBM_BitCount);

        d=1<<Vcb->LB2B_Bits;
        // if we have some bad bits, mark corresponding area as BAD
        if(bad_bm) {
            for(i=pstart; i<pend; i++) {
                if(UDFGetBadBit(bad_bm, i)) {
                    // TODO: would be nice to add these blocks to unallocatable space
                    UDFSetUsedBits(new_bm, i & ~(d-1), d);
                }
            }
        }
        j=0;
        for(i=pstart; i<pend; i+=d) {
            if(UDFGetUsedBit(old_bm, i) && UDFGetFreeBit(new_bm, i)) {
                // sector was deallocated during last session
                if(USBM) UDFSetFreeBit(upart_bm, j);
                if(FSBM) UDFSetFreeBit(fpart_bm, j);
            } else if(UDFGetUsedBit(new_bm, i)) {
                // allocated
                if(USBM) UDFSetUsedBit(upart_bm, j);
                if(FSBM) UDFSetUsedBit(fpart_bm, j);
            }
            j++;
        }
        // flush updates
        if(USBM) {
            status  = UDFWriteExtent(Vcb, &USBMExtInfo, 0, USl, FALSE, USBM, &WrittenBytes);
            DbgFreePool(USBM);
            MyFreePool__(USBMExtInfo.Mapping);
        }
        if(FSBM) {
            status2 = UDFWriteExtent(Vcb, &FSBMExtInfo, 0, FSl, FALSE, FSBM, &WrittenBytes);
            DbgFreePool(FSBM);
            MyFreePool__(FSBMExtInfo.Mapping);
        } else {
            status2 = status;
        }
    }

    if(!OS_SUCCESS(status))
        return status;
    return status2;
} // end UDFUpdateXSpaceBitmaps()

/*
    This routine updates Partition Desc & associated data structures
 */
OSSTATUS 
UDFUpdatePartDesc(
    PVCB Vcb,
    int8* Buf
    )
{
    PartitionDesc *p = (PartitionDesc *)Buf;
    uint32 i; // PartNdx
    tag* PTag;
    uint32 WrittenBytes;

    for(i=0; i<Vcb->PartitionMaps; i++)
    {
        if((UDFGetPartNumByPartNdx(Vcb,i) == p->partitionNumber) &&
           (!strcmp((int8*)&(p->partitionContents.ident), PARTITION_CONTENTS_NSR02) ||
            !strcmp((int8*)&(p->partitionContents.ident), PARTITION_CONTENTS_NSR03)))
        {
            PPARTITION_HEADER_DESC phd;
            
            phd = (PPARTITION_HEADER_DESC)(p->partitionContentsUse);
#ifdef UDF_DBG
            if(phd->unallocatedSpaceTable.extLength) {
                // rebuild unallocatedSpaceTable
                KdPrint(("unallocatedSpaceTable (part %d)\n", i));
            }
            if(phd->freedSpaceTable.extLength) {
                // rebuild freedSpaceTable
                KdPrint(("freedSpaceTable (part %d)\n", i));
            }
#endif // UDF_DBG
            UDFUpdateXSpaceBitmaps(Vcb, p->partitionNumber, phd);
            PTag = (tag*)Buf;
            UDFSetUpTag(Vcb, PTag, PTag->descCRCLength, PTag->tagLocation);
            UDFWriteSectors(Vcb, TRUE, PTag->tagLocation, 1, FALSE, Buf, &WrittenBytes);
        }
    }
    return STATUS_SUCCESS;
} // end UDFUpdatePartDesc()

/*
    This routine blanks Unalloc Space Desc
 *//*
OSSTATUS
UDFUpdateUSpaceDesc(
    IN PVCB Vcb,
    int8* Buf
    )
{
    PUNALLOC_SPACE_DESC usd;
    uint32 WrittenBytes;

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
OSSTATUS
UDFUpdateLogicalVolInt(
    PVCB            Vcb,
    BOOLEAN         Close
    )
{
    OSSTATUS    RC = STATUS_SUCCESS;
    uint32      i, len;
    uint32      WrittenBytes;
//    uint32      lvid_count = 0;
    uint32      pSize;
    tag*        PTag;
    LogicalVolIntegrityDesc *lvid;
    LogicalVolIntegrityDescImpUse* LVID_iUse;
    LogicalVolHeaderDesc* LVID_hd;
    uint32*     partFreeSpace;
    BOOLEAN     equal = FALSE;

    if(Vcb->CDR_Mode)
        return STATUS_SUCCESS;
    if(!Vcb->LVid) {
        return STATUS_UNSUCCESSFUL;
    }

    KdPrint(("UDF: Updating LVID @%x (%x)\n", Vcb->LVid_loc.extLocation, Vcb->LVid_loc.extLength));
    len = max(Vcb->LVid_loc.extLength, Vcb->BlockSize);
    lvid = Vcb->LVid;
    if(lvid->descTag.tagSerialNum > UDF_LVID_TTL) {
        // TODO: allocate space for new LVID
    }

    LVID_iUse = UDFGetLVIDiUse(Vcb);

    if((LVID_iUse->minUDFReadRev  == Vcb->minUDFReadRev) &&
       (LVID_iUse->minUDFReadRev  == Vcb->minUDFReadRev) &&
       (LVID_iUse->maxUDFWriteRev == Vcb->maxUDFWriteRev) &&
       (LVID_iUse->numFiles == Vcb->numFiles) &&
       (LVID_iUse->numDirs  == Vcb->numDirs))
        equal = TRUE;

    LVID_iUse->minUDFReadRev  = Vcb->minUDFReadRev;
    LVID_iUse->minUDFWriteRev = Vcb->minUDFWriteRev;
    LVID_iUse->maxUDFWriteRev = Vcb->maxUDFWriteRev;

    LVID_iUse->numFiles = Vcb->numFiles;
    LVID_iUse->numDirs  = Vcb->numDirs;

#if 0
    UDFSetEntityID_imp(&(LVID_iUse->impIdent), UDF_ID_DEVELOPER);
#endif

    if(Close){
        KdPrint(("UDF: Opening LVID\n"));
        lvid->integrityType = INTEGRITY_TYPE_CLOSE;
    } else {
        KdPrint(("UDF: Closing LVID\n"));
        lvid->integrityType = INTEGRITY_TYPE_OPEN;
    }

    equal = equal && (Vcb->IntegrityType == lvid->integrityType);

    // update Free Space Table
    partFreeSpace = (uint32*)(lvid+1);
    for(i=0; i<lvid->numOfPartitions; i++) {
        pSize = UDFGetPartFreeSpace(Vcb, i) >> Vcb->LB2B_Bits;
        equal = equal && (partFreeSpace[i] == pSize);
        partFreeSpace[i] = pSize;
    }

    // Update LVID Header Descriptor
    LVID_hd = (LogicalVolHeaderDesc*)&(lvid->logicalVolContentsUse);
    equal = equal && (LVID_hd->uniqueID == Vcb->NextUniqueId);
    LVID_hd->uniqueID = Vcb->NextUniqueId;

    if(equal) {
        KdPrint(("UDF: equal Ids\n"));
        return STATUS_SUCCESS;
    }

    PTag = &(lvid->descTag);
    lvid->lengthOfImpUse =
        sizeof(LogicalVolIntegrityDescImpUse);
    UDFSetUpTag(Vcb, PTag,
        sizeof(LogicalVolIntegrityDesc) +
        sizeof(uint32)*2*lvid->numOfPartitions +
        sizeof(LogicalVolIntegrityDescImpUse),
        PTag->tagLocation);

    Vcb->IntegrityType = INTEGRITY_TYPE_OPEN; // make happy auto-dirty
    RC = UDFWriteSectors(Vcb, TRUE, PTag->tagLocation, len >> Vcb->BlockSizeBits, FALSE, (int8*)(lvid), &WrittenBytes);
    WCacheFlushBlocks__(&(Vcb->FastCache), Vcb, PTag->tagLocation, len >> Vcb->BlockSizeBits);
    // update it here to prevent recursion
    Vcb->IntegrityType = lvid->integrityType;

    return RC;
} // end UDFUpdateLogicalVolInt()

/*
    This routine reads all sparing tables & stores them in contiguos memory
    space
 */
OSSTATUS
UDFUpdateSparingTable(
    IN PVCB Vcb
    )
{
    PSPARING_MAP RelocMap;
//    PSPARING_MAP NewRelocMap;
    OSSTATUS status = STATUS_SUCCESS;
    OSSTATUS status2 = STATUS_SUCCESS;
    uint32 i=0, BC, BC2;
    PSPARING_TABLE SparTable;
    uint32 ReadBytes;
//    uint32 n,m;
//    BOOLEAN merged;
    BOOLEAN sorted;

    KdPrint(("UDF: Updating Sparable Part Map:\n"));
    if(!Vcb->SparingTableModified) return STATUS_SUCCESS;
    if(!Vcb->SparingTable) return STATUS_SUCCESS;

    BC = (Vcb->SparingTableLength >> Vcb->BlockSizeBits) + 1;
    SparTable = (PSPARING_TABLE)MyAllocatePool__(NonPagedPool, BC*Vcb->BlockSize);
    if(!SparTable) return STATUS_INSUFFICIENT_RESOURCES;
    // if a part of Sparing Table is already loaded,
    // update it with data from another one
    RelocMap = Vcb->SparingTable;
    // sort sparing table
    //merged = FALSE;
    do {
        sorted = FALSE;
        for(i=1;i<Vcb->SparingCount;i++) {
            if(RelocMap[i-1].origLocation > RelocMap[i].origLocation) {
                XCHG_DD(RelocMap[i-1].origLocation,   RelocMap[i].origLocation);
swp_loc:
                XCHG_DD(RelocMap[i-1].mappedLocation, RelocMap[i].mappedLocation);
                //merged = TRUE;
                sorted = TRUE;
            } else
            if(RelocMap[i-1].origLocation == SPARING_LOC_AVAILABLE &&
               RelocMap[i].origLocation   == SPARING_LOC_AVAILABLE &&
               RelocMap[i-1].mappedLocation > RelocMap[i].mappedLocation) {
                goto swp_loc;
            }
        }
    } while(sorted);

    for(i=0;i<Vcb->SparingCount;i++) {
        KdPrint(("  @%x -> %x \n", 
            RelocMap[i].origLocation, RelocMap[i].mappedLocation));
    }

    Vcb->SparingTableModified = FALSE;
//    if(!merged) {
//        KdPrint(("  sparing table unchanged\n"));
//        MyFreePool__(SparTable);
//        return STATUS_SUCCESS;
//    }

    // walk through all available Sparing Tables
    for(i=0;i<Vcb->SparingTableCount;i++) {
        // read (next) table
        KdPrint(("  sparing table @%x\n", Vcb->SparingTableLoc[i]));
        status = UDFReadSectors(Vcb, FALSE, Vcb->SparingTableLoc[i], 1, FALSE, (int8*)SparTable, &ReadBytes);
        // tag should be set to TID_UNUSED_DESC
        if(OS_SUCCESS(status) && (SparTable->descTag.tagIdent == TID_UNUSED_DESC)) {

            BC2 = ((sizeof(SPARING_TABLE) + 
                    SparTable->reallocationTableLen*sizeof(SparingEntry) +
                    Vcb->BlockSize-1) 
                                      >> Vcb->BlockSizeBits);
            if(BC2 > BC) {
                KdPrint((" sizeSparingTable @%x too long: %x > %x\n", 
                    Vcb->SparingTableLoc[i], BC2, BC
                    ));
                continue;
            }
            status = UDFReadSectors(Vcb, FALSE, Vcb->SparingTableLoc[i],
                BC2, FALSE, (int8*)SparTable, &ReadBytes);
        
            if(!OS_SUCCESS(status)) {
                KdPrint((" Error reading sizeSparingTable @%x (%x)\n", 
                    Vcb->SparingTableLoc[i], BC2
                    ));
                continue;
            }

            BC2 = ((sizeof(SPARING_TABLE) + 
                    Vcb->SparingCount*sizeof(SparingEntry) +
                    Vcb->BlockSize-1) 
                                      >> Vcb->BlockSizeBits);
            if(BC2 > BC) {
                KdPrint((" new sizeSparingTable @%x too long: %x > %x\n", 
                    Vcb->SparingTableLoc[i], BC2, BC
                    ));
                continue;
            }

            SparTable->reallocationTableLen = (USHORT)Vcb->SparingCount;
            RtlCopyMemory((SparTable+1), RelocMap, Vcb->SparingCount*sizeof(SparingEntry));
/*
            merged = FALSE;
            NewRelocMap = (PSPARING_MAP)(SparTable+1);
            for(n=0; n<SparTable->reallocationTableLen; n++) {
                for(m=0; m<Vcb->SparingCount; m++) {
                    if(RelocMap[m].mappedLocation == NewRelocMap[n].mappedLocation) {
                        if(RelocMap[m].origLocation != NewRelocMap[n].origLocation) {
                            KdPrint(("  update @%x (%x) -> @%x (%x)\n", 
                                NewRelocMap[m].origLocation, NewRelocMap[m].mappedLocation,
                                RelocMap[m].origLocation, RelocMap[m].mappedLocation));
                            merged = TRUE;
                        }
                    }
                }
            }
*/
//            if(merged) {
            KdPrint(("UDF: record updated\n"));
            status = UDFWriteSectors(Vcb, FALSE, Vcb->SparingTableLoc[i], BC2, FALSE, (int8*)SparTable, &ReadBytes);
            if(!OS_SUCCESS(status)) {
                if(!OS_SUCCESS(status2)) {
                    status2 = status;
                }
//                }
            }
        }
    }
    MyFreePool__(SparTable);
    if(!OS_SUCCESS(status2)) {
        status = status2;
    }
    return status;
} // end UDFUpdateSparingTable()

/*
    update Logical volume descriptor
 */
OSSTATUS
UDFUpdateLogicalVol(
    IN PVCB            Vcb,
    IN UDF_VDS_RECORD  Lba,
    IN PUNICODE_STRING VolIdent
    )
{
    LogicalVolDesc* lvd = NULL;
#define CUR_IDENT_SZ (sizeof(lvd->logicalVolIdent))
    dstring CS0[CUR_IDENT_SZ];
    uint16 ident;
    uint32 WrittenBytes;
    OSSTATUS status = STATUS_SUCCESS;
//    OSSTATUS status2 = STATUS_SUCCESS;

    status = UDFUpdateSparingTable(Vcb);

    if(!(Vcb->CompatFlags & UDF_VCB_IC_W2K_COMPAT_VLABEL)) {
        goto Err_SetVI;
    }

    lvd = (LogicalVolDesc*)MyAllocatePool__(NonPagedPool, max(Vcb->BlockSize, sizeof(LogicalVolDesc)) );

    if(!lvd) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Err_SetVI;
    }

    KdPrint(("UDF: Updating LVD @%x (%x)\n", Lba.block, Vcb->BlockSize));

    status = UDFSetDstring(&(Vcb->VolIdent), (dstring*)&CS0, CUR_IDENT_SZ);
    if(!OS_SUCCESS(status)) {
        if(status == STATUS_INVALID_PARAMETER) {
            status = STATUS_INVALID_VOLUME_LABEL;
        }
        goto Err_SetVI;
    }

    if(!Lba.block) {
        status = STATUS_INVALID_PARAMETER;
        goto Err_SetVI;
    }
    status = UDFReadTagged(Vcb, (int8*)lvd, Lba.block, Lba.block, &ident);
    if(!OS_SUCCESS(status)) goto Err_SetVI;
    if(ident != TID_LOGICAL_VOL_DESC) {
        status = STATUS_FILE_CORRUPT_ERROR;
        goto Err_SetVI;
    }

    if(RtlCompareMemory(lvd->logicalVolIdent, CS0, CUR_IDENT_SZ) == CUR_IDENT_SZ) {
        // no changes
        KdPrint(("UDF: equal VolIds\n"));
        status = STATUS_SUCCESS;
        goto Err_SetVI;
    }
    RtlCopyMemory(lvd->logicalVolIdent, CS0, CUR_IDENT_SZ);

    lvd->descTag.tagSerialNum --;
    UDFSetUpTag(Vcb, (tag*)lvd, lvd->descTag.descCRCLength, Lba.block);

    status = UDFWriteSectors(Vcb, TRUE, Lba.block, 1, FALSE, (int8*)lvd, &WrittenBytes);

Err_SetVI:
    if(lvd)
        MyFreePool__(lvd);

#undef CUR_IDENT_SZ
//#endif //0

    return status;
} // end UDFUpdateLogicalVol()

/*
    This routine updates volume descriptor sequence
 */
OSSTATUS
UDFUpdateVDS(
    IN PVCB Vcb,
    IN uint32 block,
    IN uint32 lastblock,
    IN uint32 flags
    )
{
    OSSTATUS status;
    int8*    Buf = (int8*)DbgAllocatePool(NonPagedPool,Vcb->LBlockSize);
    UDF_VDS_RECORD vds[VDS_POS_LENGTH];
    uint32 i,j;
    uint16 ident;

    if (!Buf) return STATUS_INSUFFICIENT_RESOURCES;
    RtlZeroMemory(vds, sizeof(UDF_VDS_RECORD) * VDS_POS_LENGTH);
    if(!OS_SUCCESS(status = UDFReadVDS(Vcb, block, lastblock, (PUDF_VDS_RECORD)&vds, Buf))) {
        DbgFreePool(Buf);
        return status;
    }

/*
    // update USD (if any)
    for (i=0; i<VDS_POS_LENGTH; i++) {
        if (vds[i].block) {
            status = UDFReadTagged(Vcb, Buf, vds[i].block, vds[i].block, &ident);
            if(OS_SUCCESS(status) && (i == VDS_POS_PARTITION_DESC)) {
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
            status = UDFReadTagged(Vcb, Buf, vds[i].block, vds[i].block, &ident);
            if(!OS_SUCCESS(status))
                continue;
            // update XBMs
            if(i == VDS_POS_PARTITION_DESC) {
                if(!(flags & 1))
                    continue;
                // update partition descriptor(s)
                int8*  Buf2 = (int8*)DbgAllocatePool(NonPagedPool,Vcb->BlockSize);
                if (!Buf2) {
                    DbgFreePool(Buf);
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                UDFUpdatePartDesc(Vcb,Buf);
                for (j=vds[i].block+1; j<vds[VDS_POS_TERMINATING_DESC].block; j++) {
                    UDFReadTagged(Vcb,Buf2, j, j, &ident);
                    if (ident == TID_PARTITION_DESC)
                        UDFUpdatePartDesc(Vcb,Buf2);
                }
                DbgFreePool(Buf2);
//                continue;
            } else
            // update Vol Ident Desc
            if(i == VDS_POS_LOGICAL_VOL_DESC) {
                status = UDFUpdateLogicalVol(Vcb, vds[VDS_POS_LOGICAL_VOL_DESC], &(Vcb->VolIdent));
                if(!OS_SUCCESS(status))
                    continue;
            }
        }
    }

    DbgFreePool(Buf);
    return status;
} // end UDFUpdateVDS()
#endif //UDF_READ_ONLY_BUILD

OSSTATUS
__fastcall
UDFSetDstring(
    IN PUNICODE_STRING UName,
    IN dstring* Dest,
    IN uint32 Length
    )
{
    uint8* CS0;
    uint32 len = Length-1;

    UDFCompressUnicode(UName, &CS0, &len);
    if(!CS0)
        return STATUS_INSUFFICIENT_RESOURCES;
    if(len > Length-1) {
        MyFreePool__(CS0);
        return STATUS_INVALID_PARAMETER;
    }
    RtlCopyMemory(Dest, CS0, len);
    MyFreePool__(CS0);
    if(len < Length-1)
        RtlZeroMemory(Dest+len, Length-1-len);
    Dest[Length-1] = (uint8)len;
    return TRUE;
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

#ifndef UDF_READ_ONLY_BUILD
/*
    This routine updates Volume Label & some other features stored in
    VolIdentDesc
 */
OSSTATUS
UDFUpdateVolIdent(
    IN PVCB Vcb,
    IN UDF_VDS_RECORD Lba,
    IN PUNICODE_STRING VolIdent
    )
{
#define CUR_IDENT_SZ (sizeof(pvoldesc->volIdent))
    PrimaryVolDesc* pvoldesc = (PrimaryVolDesc*)MyAllocatePool__(NonPagedPool, max(Vcb->BlockSize, sizeof(PrimaryVolDesc)) );
    OSSTATUS status;
    dstring CS0[CUR_IDENT_SZ];
    uint16 ident;
    uint32 WrittenBytes;

    if(!pvoldesc) return STATUS_INSUFFICIENT_RESOURCES;

    KdPrint(("UDF: Updating PVD @%x (%x)\n", Lba.block, Vcb->BlockSize));

    status = UDFSetDstring(&(Vcb->VolIdent), (dstring*)&CS0, CUR_IDENT_SZ);
    if(!OS_SUCCESS(status)) {
        if(status == STATUS_INVALID_PARAMETER) {
            status = STATUS_INVALID_VOLUME_LABEL;
        }
        goto Err_SetVI;
    }

    if(!Lba.block) {
        status = STATUS_INVALID_PARAMETER;
        goto Err_SetVI;
    }
    status = UDFReadTagged(Vcb, (int8*)pvoldesc, Lba.block, Lba.block, &ident);
    if(!OS_SUCCESS(status)) goto Err_SetVI;
    if(ident != TID_PRIMARY_VOL_DESC) {
        status = STATUS_FILE_CORRUPT_ERROR;
        goto Err_SetVI;
    }

    if(RtlCompareMemory(pvoldesc->volIdent, CS0, CUR_IDENT_SZ) == CUR_IDENT_SZ) {
        // no changes
        status = STATUS_SUCCESS;
        goto Err_SetVI;
    }
    RtlCopyMemory(pvoldesc->volIdent, CS0, CUR_IDENT_SZ);

    pvoldesc->descTag.tagSerialNum --;
    UDFSetUpTag(Vcb, (tag*)pvoldesc, pvoldesc->descTag.descCRCLength, Lba.block);

    status = UDFWriteSectors(Vcb, TRUE, Lba.block, 1, FALSE, (int8*)pvoldesc, &WrittenBytes);
Err_SetVI:
    MyFreePool__(pvoldesc);
    return status;

#undef CUR_IDENT_SZ
} // end UDFUpdateVolIdent()
#endif //UDF_READ_ONLY_BUILD

OSSTATUS
UDFUpdateNonAllocated(
    IN PVCB Vcb
    )
{
    uint32 PartNum;
    uint32 i;
    uint32 plen, pstart, pend;
    int8* bad_bm;
    EXTENT_AD Ext;
    PEXTENT_MAP Map = NULL;
    PEXTENT_INFO DataLoc;

    KdPrint(("UDFUpdateNonAllocated:\n"));
    if(!Vcb->NonAllocFileInfo) {
        return STATUS_SUCCESS;
    }
    if(!(bad_bm = Vcb->BSBM_Bitmap)) {
        return STATUS_SUCCESS;
    }

    DataLoc = &(Vcb->NonAllocFileInfo->Dloc->DataLoc);
    ASSERT(!DataLoc->Offset);
    if(Vcb->NonAllocFileInfo->Dloc->DataLoc.Offset) {
        KdPrint(("NonAllocFileInfo in IN_ICB mode !!!\n"));
        return STATUS_SUCCESS;
    }
    PartNum = UDFGetPartNumByPhysLba(Vcb, Vcb->NonAllocFileInfo->Dloc->FELoc.Mapping[0].extLocation);
    pstart = UDFPartStart(Vcb, PartNum);
    plen = UDFPartLen(Vcb, PartNum);
    pend = min(pstart + plen, Vcb->FSBM_BitCount);

    //BrutePoint();
    for(i=pstart; i<pend; i++) {
        if(!UDFGetBadBit(bad_bm, i))
            continue;
        // add BAD blocks to unallocatable space
        // if the block is already in NonAllocatable, ignore it
        if(UDFLocateLbaInExtent(Vcb, DataLoc->Mapping, i) != LBA_OUT_OF_EXTENT) {
            KdPrint(("lba %#x is already in NonAllocFileInfo\n", i));
            continue;
        }
        KdPrint(("add lba %#x to NonAllocFileInfo\n", i));
        DataLoc->Modified = TRUE;
        Ext.extLength = Vcb->LBlockSize;
        // align lba on LogicalBlock boundary
        Ext.extLocation = i & ~((1<<Vcb->LB2B_Bits) - 1);
        Map = UDFExtentToMapping(&Ext);
        DataLoc->Mapping = UDFMergeMappings(DataLoc->Mapping, Map);
    }
    UDFPackMapping(Vcb, DataLoc);
    DataLoc->Length = UDFGetExtentLength(DataLoc->Mapping);
    UDFFlushFile__(Vcb, Vcb->NonAllocFileInfo);

    // ensure that BAD space is marked as USED
    UDFMarkSpaceAsXXX(Vcb, 0, &(DataLoc->Mapping[0]), AS_USED); // mark as used

    KdPrint(("UDFUpdateNonAllocated: done\n"));
    return STATUS_SUCCESS;
} // end UDFUpdateNonAllocated()

/*
    This routine rebuilds & flushes all system areas
 */
OSSTATUS
UDFUmount__(
    IN PVCB Vcb
    )
{
#ifndef UDF_READ_ONLY_BUILD
    uint32 flags = 0;

    if((Vcb->VCBFlags & UDF_VCB_FLAGS_VOLUME_READ_ONLY)
        || !Vcb->Modified)
        return STATUS_SUCCESS;
    // prevent discarding metadata
    Vcb->VCBFlags |= UDF_VCB_ASSUME_ALL_USED;
    if(Vcb->CDR_Mode) {
        // flush internal cache
        if(WCacheGetWriteBlockCount__(&(Vcb->FastCache)) >= (Vcb->WriteBlockSize >> Vcb->BlockSizeBits) )
            WCacheFlushAll__(&(Vcb->FastCache), Vcb);
        // record VAT
        return UDFRecordVAT(Vcb);
    }

    UDFFlushAllCachedAllocations(Vcb, UDF_PREALLOC_CLASS_FE);
    UDFFlushAllCachedAllocations(Vcb, UDF_PREALLOC_CLASS_DIR);

    if(Vcb->VerifyOnWrite) {
        KdPrint(("UDF: Flushing cache for verify\n"));
        //WCacheFlushAll__(&(Vcb->FastCache), Vcb);
        WCacheFlushBlocks__(&(Vcb->FastCache), Vcb, 0, Vcb->LastLBA);
        UDFVFlush(Vcb);
    }

    // synchronize BAD Block bitmap and NonAllocatable
    UDFUpdateNonAllocated(Vcb);

    UDFAcquireResourceExclusive(&(Vcb->BitMapResource1),TRUE);

    // RAM mode
#ifdef UDF_DBG
    if(!OS_SUCCESS(UDFUpdateVolIdent(Vcb, Vcb->PVolDescAddr, &(Vcb->VolIdent))))
        KdPrint(("Error updating VolIdent (1)\n"));
    if(!OS_SUCCESS(UDFUpdateVolIdent(Vcb, Vcb->PVolDescAddr2, &(Vcb->VolIdent))))
        KdPrint(("Error updating VolIdent (2)\n"));
#else
    UDFUpdateVolIdent(Vcb, Vcb->PVolDescAddr, &(Vcb->VolIdent));
    UDFUpdateVolIdent(Vcb, Vcb->PVolDescAddr2, &(Vcb->VolIdent));
#endif // UDF_DBG

    UDF_CHECK_BITMAP_RESOURCE(Vcb);
    // check if we should update BM
    if(Vcb->FSBM_ByteCount == RtlCompareMemory(Vcb->FSBM_Bitmap, Vcb->FSBM_OldBitmap, Vcb->FSBM_ByteCount)) {
        flags &= ~1;
    } else {
        flags |= 1;
    }

#ifdef UDF_DBG
    if(!OS_SUCCESS(UDFUpdateVDS(Vcb, Vcb->VDS1, Vcb->VDS1 + Vcb->VDS1_Len, flags)))
        KdPrint(("Error updating Main VDS\n"));
    if(!OS_SUCCESS(UDFUpdateVDS(Vcb, Vcb->VDS2, Vcb->VDS2 + Vcb->VDS2_Len, flags)))
        KdPrint(("Error updating Reserve VDS\n"));
#else
    UDFUpdateVDS(Vcb, Vcb->VDS1, Vcb->VDS1 + Vcb->VDS1_Len, flags);
    UDFUpdateVDS(Vcb, Vcb->VDS2, Vcb->VDS2 + Vcb->VDS2_Len, flags);
#endif // UDF_DBG

    // Update Integrity Desc if any
    if(Vcb->LVid && Vcb->origIntegrityType == INTEGRITY_TYPE_CLOSE) {
        UDFUpdateLogicalVolInt(Vcb, TRUE);
    }

    if(flags & 1)
        RtlCopyMemory(Vcb->FSBM_OldBitmap, Vcb->FSBM_Bitmap, Vcb->FSBM_ByteCount);

//skip_update_bitmap:

    Vcb->VCBFlags &= ~UDF_VCB_ASSUME_ALL_USED;

    UDFReleaseResource(&(Vcb->BitMapResource1));
#endif //UDF_READ_ONLY_BUILD

    return STATUS_SUCCESS;
} // end UDFUmount__()

/*************************************************************************

/*
    Find an anchor volume descriptor.
    The UDFGetDiskInfoAndVerify() will invoke this routine to find & check
    Anchor Volume Descriptors on the target device
*/
lba_t
UDFFindAnchor(
    PVCB           Vcb           // Volume control block
    )
{
//    OSSTATUS    RC = STATUS_SUCCESS;

    uint16 ident;
    uint32 i;
    uint32 LastBlock;
    OSSTATUS status;
    int8* Buf = (int8*)MyAllocatePool__(NonPagedPool,Vcb->BlockSize);
    BOOLEAN MRW_candidate;
    BOOLEAN IsMRW = (Vcb->MRWStatus != 0);
    if(!Buf)
        return 0;

    KdPrint(("UDFFindAnchor\n"));
    // init probable locations...
    RtlZeroMemory(&(Vcb->Anchor), sizeof(Vcb->Anchor));
    Vcb->Anchor[0] = 256 + Vcb->FirstLBALastSes;
    Vcb->Anchor[1] = 512 + Vcb->FirstLBALastSes;
    Vcb->Anchor[2] = 256 + Vcb->TrackMap[Vcb->LastTrackNum].FirstLba;
    Vcb->Anchor[3] = 512 + Vcb->TrackMap[Vcb->LastTrackNum].FirstLba;
    Vcb->Anchor[4] = Vcb->LastLBA - 256;
    Vcb->Anchor[5] = Vcb->LastLBA - 256 + 1;
    Vcb->Anchor[6] = Vcb->LastLBA - 256 - 2;
    // vat locations
    Vcb->Anchor[7] = Vcb->LastLBA - 2;
    Vcb->Anchor[8] = Vcb->LastLBA;
    Vcb->Anchor[9] = Vcb->LastLBA - 512;
//    Vcb->Anchor[7] = Vcb->LastLBA - 256 - 7;
//    Vcb->Anchor[8] = Vcb->LastLBA - 512 - 2;
//    Vcb->Anchor[9] = Vcb->LastLBA - 512 - 7;

    LastBlock = 0;
    // ... and check them
    for (i=0; i<sizeof(Vcb->Anchor)/sizeof(int); i++) {
        if(Vcb->Anchor[i] > Vcb->LastLBA)
            Vcb->Anchor[i] = 0;
        MRW_candidate = FALSE;
        if(Vcb->Anchor[i]) {
            KdPrint(("check Anchor %x\n", Vcb->Anchor[i]));
            if(!OS_SUCCESS(status = UDFReadTagged(Vcb,Buf,
                Vcb->Anchor[i], Vcb->Anchor[i], &ident))) {

                // Fucking MRW...
                if(!IsMRW && (i<2) &&
                   (Vcb->CompatFlags & UDF_VCB_IC_MRW_ADDR_PROBLEM)) {
                    if(OS_SUCCESS(status = UDFReadTagged(Vcb,Buf,
                        Vcb->Anchor[i]+MRW_DMA_OFFSET, Vcb->Anchor[i], &ident))) {
                        // do MRW workaround.....
                        KdPrint(("UDF: looks like we have MRW....\n"));
                        MRW_candidate = TRUE;
                        goto MRW_workaround;
                    }
                }

                Vcb->Anchor[i] = 0;
                if(status == STATUS_NONEXISTENT_SECTOR) {
                    KdPrint(("UDF: disk seems to be incomplete\n"));
                    break;
                }
            } else {
MRW_workaround:
                if((ident != TID_ANCHOR_VOL_DESC_PTR) && ((i<6) ||
                    (ident != TID_FILE_ENTRY && ident != TID_EXTENDED_FILE_ENTRY))) {
                    Vcb->Anchor[i] = 0;
                } else {
                    KdPrint(("UDF: Found AVD at %x (point %d)\n",Vcb->Anchor[i], i));
                    if(!LastBlock)
                        LastBlock = Vcb->LastLBA;
                    if(MRW_candidate) {
                        KdPrint(("UDF: looks like we _*really*_ have MRW....\n"));
                        IsMRW = TRUE;
                        ASSERT(Vcb->LastReadTrack == 1);
                        Vcb->TrackMap[Vcb->LastReadTrack].Flags |= TrackMap_FixMRWAddressing;
                        WCachePurgeAll__(&(Vcb->FastCache), Vcb);
                        KdPrint(("UDF: MRW on non-MRW drive => ReadOnly"));
                        Vcb->VCBFlags |= UDF_VCB_FLAGS_VOLUME_READ_ONLY;

                        UDFRegisterFsStructure(Vcb, Vcb->Anchor[i], Vcb->BlockSize);

                    }
                }
            }
        }
    }

    KdPrint(("UDF: -----------------\nUDF: Last block %x\n",LastBlock));
    MyFreePool__(Buf);
    return LastBlock;
} // end UDFFindAnchor()

/*
    Look for Volume recognition sequence
 */
uint32
UDFFindVRS(
    PVCB           Vcb
    )
{
    VolStructDesc  *vsd = NULL;
    uint32       offset;
    uint32       retStat = 0;
    uint32       BeginOffset = Vcb->FirstLBA;
    OSSTATUS     RC;
    int8*        buffer = (int8*)MyAllocatePool__(NonPagedPool,Vcb->BlockSize);
    uint32       ReadBytes;

    if(!buffer) return 0;
    // Relative to First LBA in Last Session
    offset = Vcb->FirstLBA + 0x10;

    KdPrint(("UDFFindVRS:\n"));

    // Process the sequence (if applicable)
    for (;(offset-BeginOffset <=0x20); offset ++) {
        // Read a block
        RC = UDFReadSectors(Vcb, FALSE, offset, 1, FALSE, buffer, &ReadBytes);
        if(!OS_SUCCESS(RC)) continue;

        // Look for ISO descriptors
        vsd = (VolStructDesc *)(buffer);

        if(vsd->stdIdent[0]) {
            if(!strncmp((int8*)(&vsd->stdIdent), STD_ID_CD001, STD_ID_LEN))
            {
                retStat |= VRS_ISO9660_FOUND;
                switch (vsd->structType)
                {
                    case 0:
                        KdPrint(("UDF: ISO9660 Boot Record found\n"));
                        break;
                    case 1:
                        KdPrint(("UDF: ISO9660 Primary Volume Descriptor found\n"));
                        break;
                    case 2:
                        KdPrint(("UDF: ISO9660 Supplementary Volume Descriptor found\n"));
                        break;
                    case 3:
                        KdPrint(("UDF: ISO9660 Volume Partition Descriptor found\n"));
                        break;
                    case 255:
                        KdPrint(("UDF: ISO9660 Volume Descriptor Set Terminator found\n"));
                        break;
                    default:
                        KdPrint(("UDF: ISO9660 VRS (%u) found\n", vsd->structType));
                        break;
                }
            }
            else if(!strncmp((int8*)(&vsd->stdIdent), STD_ID_BEA01, STD_ID_LEN))
            {
                KdPrint(("UDF: BEA01 Found\n"));
            }
            else if(!strncmp((int8*)(&vsd->stdIdent), STD_ID_TEA01, STD_ID_LEN))
            {
                KdPrint(("UDF: TEA01 Found\n"));
                break;
            }
            else if(!strncmp((int8*)(&vsd->stdIdent), STD_ID_NSR02, STD_ID_LEN))
            {
                retStat |= VRS_NSR02_FOUND;
                KdPrint(("UDF: NSR02 Found\n"));
                break;
            }
            else if(!strncmp((int8*)(&vsd->stdIdent), STD_ID_NSR03, STD_ID_LEN))
            {
                retStat |= VRS_NSR03_FOUND;
                KdPrint(("UDF: NSR03 Found\n"));
                break;
            }
        }
    }

    MyFreePool__(buffer);

    return retStat;
} // end UDFFindVRS()

/*
    process Primary volume descriptor
 */
void
UDFLoadPVolDesc(
    PVCB Vcb,
    int8* Buf // pointer to buffer containing PVD
    )
{
    PrimaryVolDesc *pvoldesc;
//    OSSTATUS    RC = STATUS_SUCCESS;

    pvoldesc = (PrimaryVolDesc *)Buf;
    KdPrint(("UDF: PrimaryVolDesc:\n"));
    KdPrint(("volDescSeqNum     = %d\n", pvoldesc->volDescSeqNum));
    KdPrint(("primaryVolDescNum = %d\n", pvoldesc->primaryVolDescNum));
    // remember recording time...
    Vcb->VolCreationTime = UDFTimeToNT(&(pvoldesc->recordingDateAndTime));
    // ...VolIdent...
#define CUR_IDENT_SZ (sizeof(pvoldesc->volIdent))
    if (Vcb->VolIdent.Buffer) {
        MyFreePool__(Vcb->VolIdent.Buffer);
    }
    UDFGetDstring(&(Vcb->VolIdent), (dstring*)&(pvoldesc->volIdent), CUR_IDENT_SZ);
#undef CUR_IDENT_SZ
    KdPrint(("volIdent[] = '%ws'\n", Vcb->VolIdent.Buffer));
#ifdef UDF_DBG
    KdPrint(("volSeqNum         = %d\n", pvoldesc->volSeqNum));
    KdPrint(("maxVolSeqNum      = %d\n", pvoldesc->maxVolSeqNum));
    KdPrint(("interchangeLvl    = %d\n", pvoldesc->interchangeLvl));
    KdPrint(("maxInterchangeLvl = %d\n", pvoldesc->maxInterchangeLvl));
    KdPrint(("charSetList       = %d\n", pvoldesc->charSetList));
    KdPrint(("maxCharSetList    = %d\n", pvoldesc->maxCharSetList));
    // ...& just print VolSetIdent
    UNICODE_STRING      instr;
#define CUR_IDENT_SZ (sizeof(pvoldesc->volSetIdent))
    UDFGetDstring(&instr, (dstring*)&(pvoldesc->volSetIdent), CUR_IDENT_SZ);
#undef CUR_IDENT_SZ
    KdPrint(("volSetIdent[] = '%ws'\n", instr.Buffer));
//    KdPrint(("maxInterchangeLvl = %d\n", pvoldesc->maxInterchangeLvl));
    KdPrint(("flags             = %x\n", pvoldesc->flags));
    if(instr.Buffer) MyFreePool__(instr.Buffer);
#endif // UDF_DBG
} // end UDFLoadPVolDesc()

/*
   load Logical volume integrity descriptor
 */
OSSTATUS
UDFLoadLogicalVolInt(
    PDEVICE_OBJECT  DeviceObject,
    PVCB            Vcb,
    extent_ad       loc
    )
{
    OSSTATUS    RC = STATUS_SUCCESS;
    uint32      len;
    uint32      _ReadBytes;
    int8*       Buf = NULL;
    uint16      ident;
    LogicalVolIntegrityDescImpUse* LVID_iUse;
    LogicalVolHeaderDesc* LVID_hd;
    extent_ad   last_loc;
    BOOLEAN     read_last = FALSE;
    uint32      lvid_count = 0;

    ASSERT(!Vcb->LVid);
    if(Vcb->LVid) {
        MyFreePool__(Vcb->LVid);
        Vcb->LVid = NULL;
    }
    // walk through all sectors inside LogicalVolumeIntegrityDesc
    while(loc.extLength) {
        KdPrint(("UDF: Reading LVID @%x (%x)\n", loc.extLocation, loc.extLength));
        len = max(loc.extLength, Vcb->BlockSize);
        Buf = (int8*)MyAllocatePool__(NonPagedPool,len);
        if(!Buf)
            return STATUS_INSUFFICIENT_RESOURCES;
        RC = UDFReadTagged(Vcb,Buf, loc.extLocation, loc.extLocation, &ident);
        if(!OS_SUCCESS(RC)) {
exit_with_err:
            KdPrint(("UDF: Reading LVID @%x (%x) failed.\n", loc.extLocation, loc.extLength));
            switch(Vcb->PartitialDamagedVolumeAction) {
            case UDF_PART_DAMAGED_RO:
                KdPrint(("UDF: Switch to r/o mode.\n"));
                Vcb->VCBFlags |= UDF_VCB_FLAGS_VOLUME_READ_ONLY;
                Vcb->UserFSFlags |= UDF_USER_FS_FLAGS_MEDIA_DEFECT_RO;
                RC = STATUS_SUCCESS;
                break;
            case UDF_PART_DAMAGED_NO:
                KdPrint(("UDF: Switch to raw mount mode, return UNRECOGNIZED_VOLUME.\n"));
                Vcb->VCBFlags |= UDF_VCB_FLAGS_RAW_DISK;
                //RC = STATUS_WRONG_VOLUME;
                break;
            case UDF_PART_DAMAGED_RW:
            default:
                KdPrint(("UDF: Keep r/w mode for your own risk.\n"));
                RC = STATUS_SUCCESS;
                // asume we have INTEGRITY_TYPE_CLOSE
                Vcb->IntegrityType = INTEGRITY_TYPE_CLOSE;
                break;
            }

            MyFreePool__(Buf);
            return RC;
        }
        UDFRegisterFsStructure(Vcb, loc.extLocation, len);
        // handle Terminal Entry
        if(ident == TID_TERMINAL_ENTRY) {
            read_last = TRUE;
            MyFreePool__(Buf);
            Vcb->LVid = NULL;
            loc = last_loc;
            continue;
        } else
        if(ident != TID_LOGICAL_VOL_INTEGRITY_DESC) {
            RC = STATUS_DISK_CORRUPT_ERROR;
            goto exit_with_err;
        }

        Vcb->LVid = (LogicalVolIntegrityDesc *)Buf;
        RC = UDFReadData(Vcb, TRUE, ((uint64)(loc.extLocation)) << Vcb->BlockSizeBits, len, FALSE, Buf, &_ReadBytes);
        // update info
        if( !read_last &&
            Vcb->LVid->nextIntegrityExt.extLength) {
            // go to next LVID
            last_loc = loc;
            loc = Vcb->LVid->nextIntegrityExt;
            Vcb->LVid = NULL;
            MyFreePool__(Buf);
            lvid_count++;
            if(lvid_count > UDF_MAX_LVID_CHAIN_LENGTH) {
                RC = STATUS_DISK_CORRUPT_ERROR;
                goto exit_with_err;
            }
            continue;
        }
        // process last LVID
        Vcb->origIntegrityType =
            Vcb->IntegrityType = Vcb->LVid->integrityType;
        Vcb->LVid_loc = loc;

        LVID_iUse = UDFGetLVIDiUse(Vcb);

        KdPrint(("UDF: Last LVID:\n"));
        KdPrint(("     minR: %x\n",LVID_iUse->minUDFReadRev ));
        KdPrint(("     minW: %x\n",LVID_iUse->minUDFWriteRev));
        KdPrint(("     maxW: %x\n",LVID_iUse->maxUDFWriteRev));
        KdPrint(("     Type: %s\n",!Vcb->IntegrityType ? "Open" : "Close"));

        Vcb->minUDFReadRev  = LVID_iUse->minUDFReadRev;
        Vcb->minUDFWriteRev = LVID_iUse->minUDFWriteRev;
        Vcb->maxUDFWriteRev = LVID_iUse->maxUDFWriteRev;

        Vcb->numFiles = LVID_iUse->numFiles;
        Vcb->numDirs  = LVID_iUse->numDirs;
        KdPrint(("     nFiles: %x\n",Vcb->numFiles ));
        KdPrint(("     nDirs: %x\n",Vcb->numDirs ));

        // Check if we can understand this format
        if(Vcb->minUDFReadRev > UDF_MAX_READ_REVISION)
            RC = STATUS_UNRECOGNIZED_VOLUME;
        // Check if we know how to write here
        if(Vcb->minUDFWriteRev > UDF_MAX_WRITE_REVISION) {
            KdPrint(("     Target FS requires: %x Revision => ReadOnly\n",Vcb->minUDFWriteRev));
            Vcb->VCBFlags |= UDF_VCB_FLAGS_VOLUME_READ_ONLY;
            Vcb->UserFSFlags |= UDF_USER_FS_FLAGS_NEW_FS_RO;
        }

        LVID_hd = (LogicalVolHeaderDesc*)&(Vcb->LVid->logicalVolContentsUse);
        Vcb->NextUniqueId = LVID_hd->uniqueID;
        KdPrint(("     Next FID: %x\n",Vcb->NextUniqueId));

        break;
    }

    return RC;
} // end UDFLoadLogicalVolInt()


/*
    load Logical volume descriptor
 */
OSSTATUS
UDFLoadLogicalVol(
    PDEVICE_OBJECT  DeviceObject,
    PVCB            Vcb,
    int8*           Buf,
    lb_addr         *fileset
    )
{
    LogicalVolDesc *lvd = (LogicalVolDesc *)Buf;
    uint16 i, offset;
    uint8 type;
    OSSTATUS status = STATUS_SUCCESS;
    KdPrint(("UDF: LogicalVolDesc\n"));
    // Validate partition map counter
    if(!(Vcb->Partitions)) {
        Vcb->PartitionMaps = lvd->numPartitionMaps;
        Vcb->Partitions = (PUDFPartMap)MyAllocatePool__(NonPagedPool, sizeof(UDFPartMap) * Vcb->PartitionMaps );
        if(!Vcb->Partitions)
            return STATUS_INSUFFICIENT_RESOURCES;
    } else {
        if(Vcb->PartitionMaps != lvd->numPartitionMaps)
            return STATUS_DISK_CORRUPT_ERROR;
    }
    KdPrint(("UDF: volDescSeqNum = %x\n", lvd->volDescSeqNum));
    // Get logical block size (may be different from physical)
    Vcb->LBlockSize = lvd->logicalBlockSize;
    // Get current UDF revision
    // Get Read-Only flags
    UDFReadEntityID_Domain(Vcb, &(lvd->domainIdent));

    if(Vcb->LBlockSize < Vcb->BlockSize)
        return STATUS_DISK_CORRUPT_ERROR;
    switch(Vcb->LBlockSize) {
    case 512: Vcb->LBlockSizeBits = 9; break;
    case 1024: Vcb->LBlockSizeBits = 10; break;
    case 2048: Vcb->LBlockSizeBits = 11; break;
    case 4096: Vcb->LBlockSizeBits = 12; break;
    case 8192: Vcb->LBlockSizeBits = 13; break;
    case 16384: Vcb->LBlockSizeBits = 14; break;
    case 32768: Vcb->LBlockSizeBits = 15; break;
    case 65536: Vcb->LBlockSizeBits = 16; break;
    default:
        KdPrint(("UDF: Bad block size (%ld)\n", Vcb->LBlockSize));
        return STATUS_DISK_CORRUPT_ERROR;
    }
    KdPrint(("UDF: logical block size (%ld)\n", Vcb->LBlockSize));
    Vcb->LB2B_Bits = Vcb->LBlockSizeBits - Vcb->BlockSizeBits;
    KdPrint(("UDF: mapTableLength = %x\n", lvd->mapTableLength));
    KdPrint(("UDF: numPartitionMaps = %x\n", lvd->numPartitionMaps));
    // walk through all available part maps
    for (i=0,offset=0;
         i<Vcb->PartitionMaps && offset<lvd->mapTableLength;
         i++,offset+=((GenericPartitionMap *)( ((uint8*)(lvd+1))+offset) )->partitionMapLength)
    {
        GenericPartitionMap* gpm = (GenericPartitionMap *)(((uint8*)(lvd+1))+offset);
        type = gpm->partitionMapType;
        KdPrint(("Partition (%d) type %x, len %x\n", i, type, gpm->partitionMapLength));
        if(type == PARTITION_MAP_TYPE_1)
        {
            GenericPartitionMap1 *gpm1 = (GenericPartitionMap1 *)(((uint8*)(lvd+1))+offset);

            Vcb->Partitions[i].PartitionType = UDF_TYPE1_MAP15;
            Vcb->Partitions[i].VolumeSeqNum = gpm1->volSeqNum;
            Vcb->Partitions[i].PartitionNum = gpm1->partitionNum;
            status = STATUS_SUCCESS;
        }
        else if(type == PARTITION_MAP_TYPE_2)
        {
            UdfPartitionMap2* upm2 = (UdfPartitionMap2 *)(((uint8*)(lvd+1))+offset);
            if(!strncmp((int8*)&(upm2->partIdent.ident), UDF_ID_VIRTUAL, strlen(UDF_ID_VIRTUAL)))
            {
                UDFIdentSuffix* udfis =
                    (UDFIdentSuffix*)&(upm2->partIdent.identSuffix);

                if( (udfis->currentRev == 0x0150)/* ||
                    (Vcb->CurrentUDFRev == 0x0150)*/ ) {
                    KdPrint(("Found VAT 1.50\n"));
                    Vcb->Partitions[i].PartitionType = UDF_VIRTUAL_MAP15;
                } else
                if( (udfis->currentRev == 0x0200) ||
                    (udfis->currentRev == 0x0201) /*||
                    (Vcb->CurrentUDFRev == 0x0200) ||
                    (Vcb->CurrentUDFRev == 0x0201)*/ ) {
                    KdPrint(("Found VAT 2.00\n"));
                    Vcb->Partitions[i].PartitionType = UDF_VIRTUAL_MAP20;
                }
                status = STATUS_SUCCESS;
            }
            else if(!strncmp((int8*)&(upm2->partIdent.ident), UDF_ID_SPARABLE, strlen(UDF_ID_SPARABLE)))
            {
                KdPrint(("Load sparing table\n"));
                PSPARABLE_PARTITION_MAP spm = (PSPARABLE_PARTITION_MAP)(((uint8*)(lvd+1))+offset);
                Vcb->Partitions[i].PartitionType = UDF_SPARABLE_MAP15;
                status = UDFLoadSparingTable(Vcb, spm);
            }
            else if(!strncmp((int8*)&(upm2->partIdent.ident), UDF_ID_METADATA, strlen(UDF_ID_METADATA)))
            {
                KdPrint(("Found metadata partition\n"));
//                PMETADATA_PARTITION_MAP mpm = (PMETADATA_PARTITION_MAP)(((uint8*)(lvd+1))+offset);
                Vcb->Partitions[i].PartitionType = UDF_METADATA_MAP25;
                //status = UDFLoadSparingTable(Vcb, spm);
            }
            else
            {
                KdPrint(("Unknown ident: %s\n", upm2->partIdent.ident));
                continue;
            }
            Vcb->Partitions[i].VolumeSeqNum = upm2->volSeqNum;
            Vcb->Partitions[i].PartitionNum = upm2->partitionNum;
        }
    }

    if(fileset) {
        // remember FileSet location
        long_ad *la = (long_ad *)&(lvd->logicalVolContentsUse[0]);
        *fileset = (la->extLocation);
        KdPrint(("FileSet found in LogicalVolDesc at block=%x, partition=%d\n",
            fileset->logicalBlockNum,
            fileset->partitionReferenceNum));
    }
    if(OS_SUCCESS(status)) {
        // load Integrity Desc if any
        if(lvd->integritySeqExt.extLength)
            status = UDFLoadLogicalVolInt(DeviceObject,Vcb,lvd->integritySeqExt);
    }
    return status;
} // end UDFLoadLogicalVol()

OSSTATUS
UDFLoadBogusLogicalVol(
    PDEVICE_OBJECT  DeviceObject,
    PVCB            Vcb,
    int8*           Buf,
    lb_addr         *fileset
    )
{
//    LogicalVolDesc *lvd = (LogicalVolDesc *)Buf;
    KdPrint(("UDF: Bogus LogicalVolDesc\n"));
    // Validate partition map counter
    if(!(Vcb->Partitions)) {
        Vcb->PartitionMaps = 1;
        Vcb->Partitions = (PUDFPartMap)MyAllocatePool__(NonPagedPool, sizeof(UDFPartMap) * Vcb->PartitionMaps );
        if(!Vcb->Partitions)
            return STATUS_INSUFFICIENT_RESOURCES;
    } else {
        if(Vcb->PartitionMaps != 1)
            return STATUS_DISK_CORRUPT_ERROR;
    }
    KdPrint(("UDF: volDescSeqNum = %x\n", 0));
    // Get logical block size (may be different from physical)
    Vcb->LBlockSize = 2048;
    // Get current UDF revision
    // Get Read-Only flags
//    UDFReadEntityID_Domain(Vcb, &(lvd->domainIdent));

    if(Vcb->LBlockSize < Vcb->BlockSize)
        return STATUS_DISK_CORRUPT_ERROR;
    Vcb->LBlockSizeBits = 11;
    KdPrint(("UDF: logical block size (%ld)\n", Vcb->LBlockSize));
    Vcb->LB2B_Bits = Vcb->LBlockSizeBits - Vcb->BlockSizeBits;
    KdPrint(("UDF: mapTableLength = %x\n", 0));
    KdPrint(("UDF: numPartitionMaps = %x\n", 0));

// if(CDRW) {

    Vcb->Partitions[0].PartitionType = UDF_TYPE1_MAP15;
    Vcb->Partitions[0].VolumeSeqNum = 0;
    Vcb->Partitions[0].PartitionNum = 0;

/* } else if(CDR)
                if()
                    KdPrint(("Found VAT 1.50\n"));
                    Vcb->Partitions[i].PartitionType = UDF_VIRTUAL_MAP15;
                } else
                    KdPrint(("Found VAT 2.00\n"));
                    Vcb->Partitions[i].PartitionType = UDF_VIRTUAL_MAP20;
                }
            }
    }
*/
    if(fileset) {
        // remember FileSet location
//        long_ad *la = (long_ad *)&(lvd->logicalVolContentsUse[0]);
        fileset->logicalBlockNum = 0;
        fileset->partitionReferenceNum = 0;
        KdPrint(("FileSet found in LogicalVolDesc at block=%x, partition=%d\n",
            fileset->logicalBlockNum,
            fileset->partitionReferenceNum));
    }
    return STATUS_SUCCESS;
} // end UDFLoadBogusLogicalVol()

/*
    This routine adds given Bitmap to existing one
 */
OSSTATUS
UDFAddXSpaceBitmap(
    IN PVCB Vcb,
    IN uint32 PartNum,
    IN PSHORT_AD bm,
    IN ULONG bm_type
    )
{
    int8* tmp;
    int8* tmp_bm;
    uint32 i, lim, j, lba, l, lim2, l2, k;
    lb_addr locAddr;
    OSSTATUS status;
    uint16 Ident;
    uint32 flags;
    uint32 Length;
    uint32 ReadBytes;
    BOOLEAN bit_set;

    UDF_CHECK_BITMAP_RESOURCE(Vcb);
    KdPrint(("UDFAddXSpaceBitmap: at block=%x, partition=%d\n",
        bm->extPosition,
        PartNum));

    if(!(Length = (bm->extLength & UDF_EXTENT_LENGTH_MASK))) return STATUS_SUCCESS;
    i=UDFPartStart(Vcb, PartNum);
    flags = bm->extLength >> 30;
    if(!flags /*|| flags == EXTENT_NOT_RECORDED_ALLOCATED*/) {
        tmp = (int8*)DbgAllocatePool(NonPagedPool, max(Length, Vcb->BlockSize));
        if(!tmp) return STATUS_INSUFFICIENT_RESOURCES;
        locAddr.partitionReferenceNum = (uint16)PartNum;
        locAddr.logicalBlockNum = bm->extPosition;
        // read header of the Bitmap
        if(!OS_SUCCESS(status = UDFReadTagged(Vcb, tmp, lba = UDFPartLbaToPhys(Vcb, &(locAddr)),
                             locAddr.logicalBlockNum, &Ident)) ) {
err_addxsbm_1:
            DbgFreePool(tmp);
            return status;
        }
        if(Ident != TID_SPACE_BITMAP_DESC) {
            status = STATUS_DISK_CORRUPT_ERROR;
            goto err_addxsbm_1;
        }
        UDFRegisterFsStructure(Vcb, lba, Vcb->BlockSize);
        // read the whole Bitmap
        if(!OS_SUCCESS(status = UDFReadData(Vcb, FALSE, ((uint64)lba)<<Vcb->BlockSizeBits, Length, FALSE, tmp, &ReadBytes)))
            goto err_addxsbm_1;
        UDFRegisterFsStructure(Vcb, lba, Length);
        lim = min(i + ((lim2 = ((PSPACE_BITMAP_DESC)tmp)->numOfBits) << Vcb->LB2B_Bits), Vcb->FSBM_BitCount);
        tmp_bm = tmp + sizeof(SPACE_BITMAP_DESC);
        j = 0;
        for(;(l = UDFGetBitmapLen((uint32*)tmp_bm, j, lim2)) && (i<lim);) {
            // expand LBlocks to Sectors...
            l2 = l << Vcb->LB2B_Bits;
            // ...and mark them
            if(bm_type == UDF_FSPACE_BM) {
                bit_set = UDFGetFreeBit(tmp_bm, j);
                for(k=0;(k<l2) && (i<lim);k++) {
                    if(bit_set) {
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
                    if(bit_set) {
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
        }
        DbgFreePool(tmp);
/*    } else if((bm->extLength >> 30) == EXTENT_NOT_RECORDED_ALLOCATED) {
        i=Vcb->Partitions[PartNum].PartitionRoot;
        lim = i + Vcb->Partitions[PartNum].PartitionLen;
        for(;i<lim;i++) {
            UDFSetUsedBit(Vcb->FSBM_Bitmap, i);
        }*/
    }
    return STATUS_SUCCESS;
} // end UDFAddXSpaceBitmap()

/*
    This routine adds given Bitmap to existing one
 */
OSSTATUS
UDFVerifyXSpaceBitmap(
    IN PVCB Vcb,
    IN uint32 PartNum,
    IN PSHORT_AD bm,
    IN ULONG bm_type
    )
{
    int8* tmp;
    int8* tmp_bm;
//    uint32 i, lim, j, lba, l, lim2, l2, k;
    uint32 i, lim, j, lba, lim2;
    lb_addr locAddr;
    OSSTATUS status;
    uint16 Ident;
    uint32 flags;
    uint32 Length;
    uint32 ReadBytes;
//    BOOLEAN bit_set;

    UDF_CHECK_BITMAP_RESOURCE(Vcb);

    KdPrint((" UDFVerifyXSpaceBitmap: part %x\n", PartNum));

    if(!(Length = (bm->extLength & UDF_EXTENT_LENGTH_MASK))) return STATUS_SUCCESS;
    i=UDFPartStart(Vcb, PartNum);
    flags = bm->extLength >> 30;
    if(!flags /*|| flags == EXTENT_NOT_RECORDED_ALLOCATED*/) {
        tmp = (int8*)DbgAllocatePool(NonPagedPool, max(Length, Vcb->BlockSize));
        if(!tmp) return STATUS_INSUFFICIENT_RESOURCES;
        locAddr.partitionReferenceNum = (uint16)PartNum;
        locAddr.logicalBlockNum = bm->extPosition;
        // read header of the Bitmap
        if(!OS_SUCCESS(status = UDFReadTagged(Vcb, tmp, lba = UDFPartLbaToPhys(Vcb, &(locAddr)),
                             locAddr.logicalBlockNum, &Ident)) ) {
err_vfyxsbm_1:
            DbgFreePool(tmp);
            return status;
        }
        KdPrint((" BM Lba %x\n", lba));
        if(Ident != TID_SPACE_BITMAP_DESC) {
            status = STATUS_DISK_CORRUPT_ERROR;
            goto err_vfyxsbm_1;
        }
        // read the whole Bitmap
        if(!OS_SUCCESS(status = UDFReadData(Vcb, FALSE, ((uint64)lba)<<Vcb->BlockSizeBits, Length, FALSE, tmp, &ReadBytes)))
            goto err_vfyxsbm_1;
        UDFRegisterFsStructure(Vcb, lba, Length);
        lim = min(i + ((lim2 = ((PSPACE_BITMAP_DESC)tmp)->numOfBits) << Vcb->LB2B_Bits), Vcb->FSBM_BitCount);
        tmp_bm = tmp + sizeof(SPACE_BITMAP_DESC);
        j = 0;
/*        for(;(l = UDFGetBitmapLen((uint32*)tmp_bm, j, lim2)) && (i<lim);) {
            // expand LBlocks to Sectors...
            l2 = l << Vcb->LB2B_Bits;
            // ...and mark them
            if(bm_type == UDF_FSPACE_BM) {
                bit_set = UDFGetFreeBit(tmp_bm, j);
                for(k=0;(k<l2) && (i<lim);k++) {
                    if(bit_set) {
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
                    if(bit_set) {
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
/*    } else if((bm->extLength >> 30) == EXTENT_NOT_RECORDED_ALLOCATED) {
        i=Vcb->Partitions[PartNum].PartitionRoot;
        lim = i + Vcb->Partitions[PartNum].PartitionLen;
        for(;i<lim;i++) {
            UDFSetUsedBit(Vcb->FSBM_Bitmap, i);
        }*/
    }
    return STATUS_SUCCESS;
} // end UDFVerifyXSpaceBitmap()

/*
    This routine subtracts given Bitmap to existing one
 */
/*OSSTATUS
UDFDelXSpaceBitmap(
    IN PVCB Vcb,
    IN uint32 PartNum,
    IN PSHORT_AD bm
    )
{
    int8* tmp, tmp_bm;
    uint32 i, lim, j;
    lb_addr locAddr;
    OSSTATUS status;
    uint16 Ident;
    uint32 flags;
    uint32 Length;
    uint32 ReadBytes;

    if(!(Length = (bm->extLength & UDF_EXTENT_LENGTH_MASK))) return STATUS_SUCCESS;
    i=0;
    flags = bm->extLength >> 30;
    if(!flags || flags == EXTENT_NOT_RECORDED_ALLOCATED) {
        tmp = (int8*)MyAllocatePool__(NonPagedPool, Length);
        if(!tmp) return STATUS_INSUFFICIENT_RESOURCES;
        locAddr.partitionReferenceNum = (uint16)PartNum;
        locAddr.logicalBlockNum = bm->extPosition;
        if((!OS_SUCCESS(status = UDFReadTagged(Vcb, tmp, (j = UDFPartLbaToPhys(Vcb, &(locAddr))),
                             locAddr.logicalBlockNum, &Ident))) ||
           (Ident != TID_SPACE_BITMAP_DESC) ) {
            MyFreePool__(tmp);
            return status;
        }
        if(!OS_SUCCESS(status = UDFReadData(Vcb, FALSE, ((uint64)j)<<Vcb->BlockSizeBits, Length, FALSE, tmp, &ReadBytes))) {
            MyFreePool__(tmp);
            return status;
        }
        lim = i + ((PSPACE_BITMAP_DESC)tmp)->numOfBits;
        tmp_bm = tmp + sizeof(SPACE_BITMAP_DESC);
        j = 0;
        for(;i<lim;i++) {
            if(UDFGetUsedBit(tmp_bm, j)) UDFSetFreeBit(Vcb->FSBM_Bitmap, i);
            j++;
        }
        MyFreePool__(tmp);
//    } else if((bm->extLength >> 30) == EXTENT_NOT_RECORDED_ALLOCATED) {
//        i=Vcb->Partitions[PartNum].PartitionRoot;
//        lim = i + Vcb->Partitions[PartNum].PartitionLen;
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
OSSTATUS
UDFVerifyFreeSpaceBitmap(
    IN PVCB Vcb,
    IN uint32 PartNdx,
    IN PPARTITION_HEADER_DESC phd, // partition header pointing to Bitmaps
    IN uint32 Lba                   // UnallocSpaceDesc
    )
{
    OSSTATUS status;
    uint32 i, l;
    uint16 Ident;
    int8* AllocDesc;
    PEXTENT_MAP Extent;
    lb_addr locAddr;
    uint32 PartNum;

    PartNum = UDFGetPartNumByPartNdx(Vcb, PartNdx);

    KdPrint(("UDFVerifyFreeSpaceBitmap:\n"));
    // read info for partition header (if any)
    if(phd) {
        // read unallocated Bitmap
        if(!OS_SUCCESS(status = UDFVerifyXSpaceBitmap(Vcb, PartNum, &(phd->unallocatedSpaceBitmap), UDF_FSPACE_BM)))
            return status;
        // read freed Bitmap
        if(!OS_SUCCESS(status = UDFVerifyXSpaceBitmap(Vcb, PartNum, &(phd->freedSpaceBitmap), UDF_ZSPACE_BM)))
            return status;
    }
    // read UnallocatedSpaceDesc & convert to Bitmap
    if(Lba) {
        KdPrint((" Lba @%x\n", Lba));
        if(!(AllocDesc = (int8*)MyAllocatePool__(NonPagedPool, Vcb->LBlockSize + sizeof(EXTENT_AD) )))
            return STATUS_INSUFFICIENT_RESOURCES;
        RtlZeroMemory(((int8*)AllocDesc) + Vcb->LBlockSize, sizeof(EXTENT_AD));
        if(!OS_SUCCESS(status = UDFReadTagged(Vcb, AllocDesc, Lba, Lba, &Ident)) ||
           !(Extent = (PEXTENT_MAP)MyAllocatePool__(NonPagedPool, l = (((PUNALLOC_SPACE_DESC)AllocDesc)->numAllocDescs+1) * sizeof(EXTENT_AD) ))) {
            MyFreePool__(AllocDesc);
            return status;
        }
        UDFRegisterFsStructure(Vcb, Lba, Vcb->BlockSize);
        RtlCopyMemory((int8*)Extent, AllocDesc+sizeof(UNALLOC_SPACE_DESC), (((PUNALLOC_SPACE_DESC)AllocDesc)->numAllocDescs+1) * sizeof(EXTENT_AD) );
        locAddr.partitionReferenceNum = (uint16)PartNum;
        // read extent is recorded with relative addresses
        // so, we should convert it to suitable form
        for(i=0; Extent[i].extLength; i++) {
            locAddr.logicalBlockNum = Extent[i].extLocation;
            Extent[i].extLocation = UDFPartLbaToPhys(Vcb, &locAddr);
            if(Extent[i].extLocation == LBA_OUT_OF_EXTENT) {
                BrutePoint();
                MyFreePool__(AllocDesc);
                return STATUS_DISK_CORRUPT_ERROR;
            }
            if((Extent[i].extLocation >> 30) == EXTENT_NEXT_EXTENT_ALLOCDESC) {
                // load continuation
                Lba = Extent[i].extLocation & UDF_EXTENT_LENGTH_MASK;
                if(!OS_SUCCESS(status = UDFReadTagged(Vcb, AllocDesc, Lba, Lba, &Ident)) ||
                   !(Extent = (PEXTENT_MAP)MyAllocatePool__(NonPagedPool, (((PUNALLOC_SPACE_DESC)AllocDesc)->numAllocDescs+1) * sizeof(EXTENT_AD) ))) {
                    MyFreePool__(AllocDesc);
                    return status;
                }
                if(Ident == TID_UNALLOC_SPACE_DESC) {
                    UDFRegisterFsStructure(Vcb, Lba, Vcb->BlockSize);
                    if(!(l = MyReallocPool__((int8*)Extent, l, (int8**)&Extent, i*sizeof(EXTENT_MAP)))) {
                        MyFreePool__(Extent);
                        MyFreePool__(AllocDesc);
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }
                    Extent[i].extLength =
                    Extent[i].extLocation = 0;
                    Extent = UDFMergeMappings(Extent, (PEXTENT_MAP)(AllocDesc+sizeof(UNALLOC_SPACE_DESC)) );
#ifdef UDF_DBG
                } else {
                    KdPrint(("Broken unallocated space descriptor sequence\n"));
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
} // end UDFBuildFreeSpaceBitmap()

/*
    This routine builds FreeSpaceBitmap (internal) according to media
    parameters & input data
 */
OSSTATUS
UDFBuildFreeSpaceBitmap(
    IN PVCB Vcb,
    IN uint32 PartNdx,
    IN PPARTITION_HEADER_DESC phd, // partition header pointing to Bitmaps
    IN uint32 Lba                   // UnallocSpaceDesc
    )
{
    OSSTATUS status;
    uint32 i, l;
    uint16 Ident;
    int8* AllocDesc;
    PEXTENT_MAP Extent;
    lb_addr locAddr;
    uint32 PartNum;

    PartNum = UDFGetPartNumByPartNdx(Vcb, PartNdx);
    if(!(Vcb->FSBM_Bitmap)) {
        // init Bitmap buffer if necessary
        Vcb->FSBM_Bitmap = (int8*)DbgAllocatePool(NonPagedPool, (i = (Vcb->LastPossibleLBA+1+7)>>3) );
        if(!(Vcb->FSBM_Bitmap)) return STATUS_INSUFFICIENT_RESOURCES;

        Vcb->ZSBM_Bitmap = (int8*)DbgAllocatePool(NonPagedPool, (i = (Vcb->LastPossibleLBA+1+7)>>3) );
        if(!(Vcb->ZSBM_Bitmap)) {
#ifdef UDF_TRACK_ONDISK_ALLOCATION_OWNERS
free_fsbm:
#endif //UDF_TRACK_ONDISK_ALLOCATION_OWNERS
            MyFreePool__(Vcb->FSBM_Bitmap);
            Vcb->FSBM_Bitmap = NULL;
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory(Vcb->FSBM_Bitmap, i);
        RtlZeroMemory(Vcb->ZSBM_Bitmap, i);
#ifdef UDF_TRACK_ONDISK_ALLOCATION_OWNERS
        Vcb->FSBM_Bitmap_owners = (uint32*)DbgAllocatePool(NonPagedPool, (Vcb->LastPossibleLBA+1)*sizeof(uint32));
        if(!(Vcb->FSBM_Bitmap_owners)) {
            MyFreePool__(Vcb->ZSBM_Bitmap);
            Vcb->ZSBM_Bitmap = NULL;
            goto free_fsbm;
        }
        RtlFillMemory(Vcb->FSBM_Bitmap_owners, (Vcb->LastPossibleLBA+1)*sizeof(uint32), 0xff);
#endif //UDF_TRACK_ONDISK_ALLOCATION_OWNERS
        Vcb->FSBM_ByteCount = i;
        Vcb->FSBM_BitCount = Vcb->LastPossibleLBA+1;
    }
    // read info for partition header (if any)
    if(phd) {
        // read unallocated Bitmap
        if(!OS_SUCCESS(status = UDFAddXSpaceBitmap(Vcb, PartNum, &(phd->unallocatedSpaceBitmap), UDF_FSPACE_BM)))
            return status;
        // read freed Bitmap
        if(!OS_SUCCESS(status = UDFAddXSpaceBitmap(Vcb, PartNum, &(phd->freedSpaceBitmap), UDF_ZSPACE_BM)))
            return status;
    }
    // read UnallocatedSpaceDesc & convert to Bitmap
    if(Lba) {
        if(!(AllocDesc = (int8*)MyAllocatePool__(NonPagedPool, Vcb->LBlockSize + sizeof(EXTENT_AD) )))
            return STATUS_INSUFFICIENT_RESOURCES;
        RtlZeroMemory(((int8*)AllocDesc) + Vcb->LBlockSize, sizeof(EXTENT_AD));
        if(!OS_SUCCESS(status = UDFReadTagged(Vcb, AllocDesc, Lba, Lba, &Ident)) ||
           !(Extent = (PEXTENT_MAP)MyAllocatePool__(NonPagedPool, l = (((PUNALLOC_SPACE_DESC)AllocDesc)->numAllocDescs+1) * sizeof(EXTENT_AD) ))) {
            MyFreePool__(AllocDesc);
            return status;
        }
        UDFRegisterFsStructure(Vcb, Lba, Vcb->BlockSize);
        RtlCopyMemory((int8*)Extent, AllocDesc+sizeof(UNALLOC_SPACE_DESC), (((PUNALLOC_SPACE_DESC)AllocDesc)->numAllocDescs+1) * sizeof(EXTENT_AD) );
        locAddr.partitionReferenceNum = (uint16)PartNum;
        // read extent is recorded with relative addresses
        // so, we should convert it to suitable form
        for(i=0; Extent[i].extLength; i++) {
            locAddr.logicalBlockNum = Extent[i].extLocation;
            Extent[i].extLocation = UDFPartLbaToPhys(Vcb, &locAddr);
            if(Extent[i].extLocation == LBA_OUT_OF_EXTENT) {
                BrutePoint();
                MyFreePool__(AllocDesc);
                return STATUS_DISK_CORRUPT_ERROR;
            }
            if((Extent[i].extLocation >> 30) == EXTENT_NEXT_EXTENT_ALLOCDESC) {
                // load continuation
                Lba = Extent[i].extLocation & UDF_EXTENT_LENGTH_MASK;
                if(!OS_SUCCESS(status = UDFReadTagged(Vcb, AllocDesc, Lba, Lba, &Ident)) ||
                   !(Extent = (PEXTENT_MAP)MyAllocatePool__(NonPagedPool, (((PUNALLOC_SPACE_DESC)AllocDesc)->numAllocDescs+1) * sizeof(EXTENT_AD) ))) {
                    MyFreePool__(AllocDesc);
                    return status;
                }
                if(Ident == TID_UNALLOC_SPACE_DESC) {
                    UDFRegisterFsStructure(Vcb, Lba, Vcb->BlockSize);
                    if(!(l = MyReallocPool__((int8*)Extent, l, (int8**)&Extent, i*sizeof(EXTENT_MAP)))) {
                        MyFreePool__(Extent);
                        MyFreePool__(AllocDesc);
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }
                    Extent[i].extLength =
                    Extent[i].extLocation = 0;
                    Extent = UDFMergeMappings(Extent, (PEXTENT_MAP)(AllocDesc+sizeof(UNALLOC_SPACE_DESC)) );
#ifdef UDF_DBG
                } else {
                    KdPrint(("Broken unallocated space descriptor sequence\n"));
#endif // UDF_DBG
                }
            }
        }
        UDFMarkSpaceAsXXX(Vcb, (-1), Extent, AS_USED); // mark as used
        MyFreePool__(Extent);
        MyFreePool__(AllocDesc);
    }
    return status;
} // end UDFVerifyFreeSpaceBitmap()

/*
    process Partition descriptor
 */
OSSTATUS
UDFLoadPartDesc(
    PVCB      Vcb,
    int8*     Buf
    )
{
    PartitionDesc *p = (PartitionDesc *)Buf;
    uint32 i;
    OSSTATUS RC;
    BOOLEAN Found = FALSE;
    KdPrint(("UDF: Pard Descr:\n"));
    KdPrint((" volDescSeqNum   = %x\n", p->volDescSeqNum));
    KdPrint((" partitionFlags  = %x\n", p->partitionFlags));
    KdPrint((" partitionNumber = %x\n", p->partitionNumber));
    KdPrint((" accessType      = %x\n", p->accessType));
    KdPrint((" partitionStartingLocation = %x\n", p->partitionStartingLocation));
    KdPrint((" partitionLength = %x\n", p->partitionLength));
    // There is nothing interesting to comment here
    // Just look at Names & Messages....
    for (i=0; i<Vcb->PartitionMaps; i++) {
        KdPrint(("Searching map: (%d == %d)\n",
            Vcb->Partitions[i].PartitionNum, (p->partitionNumber) ));
        if(Vcb->Partitions[i].PartitionNum == (p->partitionNumber)) {
            Found = TRUE;
            Vcb->Partitions[i].PartitionRoot = p->partitionStartingLocation + Vcb->FirstLBA;
            Vcb->Partitions[i].PartitionLen =
                min(p->partitionLength,
                    Vcb->LastPossibleLBA - Vcb->Partitions[i].PartitionRoot); /* sectors */
            Vcb->Partitions[i].UspaceBitmap = 0xFFFFFFFF;
            Vcb->Partitions[i].FspaceBitmap = 0xFFFFFFFF;
            Vcb->Partitions[i].AccessType = p->accessType;
            KdPrint(("Access mode %x\n", p->accessType));
            if(p->accessType == PARTITION_ACCESS_WO) {
                Vcb->CDR_Mode = TRUE;
//                Vcb->Partitions[i].PartitionLen = Vcb->LastPossibleLBA - p->partitionStartingLocation;
            } else if(p->accessType < PARTITION_ACCESS_WO) {
                // Soft-read-only volume
                KdPrint(("Soft Read-only volume\n"));
                Vcb->VCBFlags |= UDF_VCB_FLAGS_VOLUME_READ_ONLY;
                Vcb->UserFSFlags |= UDF_USER_FS_FLAGS_PART_RO;
            } else if(p->accessType > PARTITION_ACCESS_MAX_KNOWN) {
                return STATUS_UNRECOGNIZED_MEDIA;
            }

            if(!strcmp((int8*)&(p->partitionContents.ident), PARTITION_CONTENTS_NSR02) ||
                !strcmp((int8*)&(p->partitionContents.ident), PARTITION_CONTENTS_NSR03))
            {
                PPARTITION_HEADER_DESC phd;

                phd = (PPARTITION_HEADER_DESC)(p->partitionContentsUse);
#ifdef UDF_DBG
                if(phd->unallocatedSpaceTable.extLength)
                    KdPrint(("unallocatedSpaceTable (part %d)\n", i));
#endif // UDF_DBG
                if(phd->unallocatedSpaceBitmap.extLength) {
                    Vcb->Partitions[i].UspaceBitmap =
                        phd->unallocatedSpaceBitmap.extPosition;
                    KdPrint(("unallocatedSpaceBitmap (part %d) @ %x\n",
                        i, Vcb->Partitions[i].UspaceBitmap ));
                }
#ifdef UDF_DBG
                if(phd->partitionIntegrityTable.extLength)
                    KdPrint(("partitionIntegrityTable (part %d)\n", i));
                if(phd->freedSpaceTable.extLength)
                    KdPrint(("freedSpaceTable (part %d)\n", i));
#endif // UDF_DBG
                if(phd->freedSpaceBitmap.extLength) {
                    Vcb->Partitions[i].FspaceBitmap =
                        phd->freedSpaceBitmap.extPosition;
                    KdPrint(("freedSpaceBitmap (part %d)\n", i));
                }
                RC = UDFBuildFreeSpaceBitmap(Vcb, i, phd, 0);
                //Vcb->Modified = FALSE;
                UDFPreClrModified(Vcb);
                UDFClrModified(Vcb);
                if(!OS_SUCCESS(RC))
                    return RC;

                if ((Vcb->Partitions[i].PartitionType == UDF_VIRTUAL_MAP15) ||
                    (Vcb->Partitions[i].PartitionType == UDF_VIRTUAL_MAP20)) {
                    RC = UDFLoadVAT(Vcb, i);
                    if(!OS_SUCCESS(RC))
                        return RC;
                    WCacheFlushAll__(&(Vcb->FastCache), Vcb);
                    WCacheSetMode__(&(Vcb->FastCache), WCACHE_MODE_R);
                    Vcb->LastModifiedTrack = 0;
                }
            } 
        }
    }
#ifdef UDF_DBG
    if(!Found) {
        KdPrint(("Partition (%d) not found in partition map\n", (p->partitionNumber) ));
    } else {
        KdPrint(("Partition (%d:%d type %x) starts at physical %x, length %x\n",
            p->partitionNumber, i-1, Vcb->Partitions[i-1].PartitionType,
            Vcb->Partitions[i-1].PartitionRoot, Vcb->Partitions[i-1].PartitionLen));
    }
#endif // UDF_DBG
    return STATUS_SUCCESS;
} // end UDFLoadPartDesc()

/*
    process Partition descriptor
 */
OSSTATUS
UDFVerifyPartDesc(
    PVCB      Vcb,
    int8*     Buf
    )
{
    PartitionDesc *p = (PartitionDesc *)Buf;
    uint32 i;
    OSSTATUS RC;
    BOOLEAN Found = FALSE;
    KdPrint(("UDF: Verify Part Descr:\n"));
    KdPrint((" volDescSeqNum   = %x\n", p->volDescSeqNum));
    KdPrint((" partitionFlags  = %x\n", p->partitionFlags));
    KdPrint((" partitionNumber = %x\n", p->partitionNumber));
    KdPrint((" accessType      = %x\n", p->accessType));
    KdPrint((" partitionStartingLocation = %x\n", p->partitionStartingLocation));
    KdPrint((" partitionLength = %x\n", p->partitionLength));
    // There is nothing interesting to comment here
    // Just look at Names & Messages....
    for (i=0; i<Vcb->PartitionMaps; i++) {
        KdPrint(("Searching map: (%d == %d)\n",
            Vcb->Partitions[i].PartitionNum, (p->partitionNumber) ));
        if(Vcb->Partitions[i].PartitionNum == (p->partitionNumber)) {
            Found = TRUE;
            if(Vcb->Partitions[i].PartitionRoot != p->partitionStartingLocation + Vcb->FirstLBA)
                return STATUS_DISK_CORRUPT_ERROR;
            if(Vcb->Partitions[i].PartitionLen !=
                min(p->partitionLength,
                    Vcb->LastPossibleLBA - Vcb->Partitions[i].PartitionRoot)) /* sectors */
                return STATUS_DISK_CORRUPT_ERROR;
//            Vcb->Partitions[i].UspaceBitmap = 0xFFFFFFFF;
//            Vcb->Partitions[i].FspaceBitmap = 0xFFFFFFFF;
            if(Vcb->Partitions[i].AccessType != p->accessType)
                return STATUS_DISK_CORRUPT_ERROR;
            KdPrint(("Access mode %x\n", p->accessType));
            if(p->accessType == PARTITION_ACCESS_WO) {
                if(Vcb->CDR_Mode != TRUE)
                    return STATUS_DISK_CORRUPT_ERROR;
//                Vcb->Partitions[i].PartitionLen = Vcb->LastPossibleLBA - p->partitionStartingLocation;
            } else if(p->accessType < PARTITION_ACCESS_WO) {
                // Soft-read-only volume
                KdPrint(("Soft Read-only volume\n"));
                if(!(Vcb->VCBFlags & UDF_VCB_FLAGS_VOLUME_READ_ONLY))
                    return STATUS_DISK_CORRUPT_ERROR;
            } else if(p->accessType > PARTITION_ACCESS_MAX_KNOWN) {
                return STATUS_UNRECOGNIZED_MEDIA;
            }

            if(!strcmp((int8*)&(p->partitionContents.ident), PARTITION_CONTENTS_NSR02) ||
                !strcmp((int8*)&(p->partitionContents.ident), PARTITION_CONTENTS_NSR03))
            {
                PPARTITION_HEADER_DESC phd;

                phd = (PPARTITION_HEADER_DESC)(p->partitionContentsUse);
#ifdef UDF_DBG
                if(phd->unallocatedSpaceTable.extLength)
                    KdPrint(("unallocatedSpaceTable (part %d)\n", i));
#endif // UDF_DBG
                if(phd->unallocatedSpaceBitmap.extLength) {
                    if(Vcb->Partitions[i].UspaceBitmap ==
                        phd->unallocatedSpaceBitmap.extPosition) {
                        KdPrint(("Warning: both USpaceBitmaps have same location\n"));
                    }
                    KdPrint(("unallocatedSpaceBitmap (part %d) @ %x\n",
                        i, Vcb->Partitions[i].UspaceBitmap ));
                }
#ifdef UDF_DBG
                if(phd->partitionIntegrityTable.extLength)
                    KdPrint(("partitionIntegrityTable (part %d)\n", i));
                if(phd->freedSpaceTable.extLength)
                    KdPrint(("freedSpaceTable (part %d)\n", i));
#endif // UDF_DBG
                if(phd->freedSpaceBitmap.extLength) {
                    if(Vcb->Partitions[i].FspaceBitmap ==
                        phd->freedSpaceBitmap.extPosition) {
                        KdPrint(("Warning: both FSpaceBitmaps have same location\n"));
                    }
                    KdPrint(("freedSpaceBitmap (part %d)\n", i));
                }
                RC = UDFVerifyFreeSpaceBitmap(Vcb, i, phd, 0);
                //Vcb->Modified = FALSE;
                //UDFPreClrModified(Vcb);
                //UDFClrModified(Vcb);
                if(!OS_SUCCESS(RC))
                    return RC;

                if ((Vcb->Partitions[i].PartitionType == UDF_VIRTUAL_MAP15) ||
                    (Vcb->Partitions[i].PartitionType == UDF_VIRTUAL_MAP20)) {
/*                    RC = UDFLoadVAT(Vcb, i);
                    if(!OS_SUCCESS(RC))
                        return RC;
                    WCacheFlushAll__(&(Vcb->FastCache), Vcb);
                    WCacheSetMode__(&(Vcb->FastCache), WCACHE_MODE_R);
                    Vcb->LastModifiedTrack = 0;*/
                }
            } 
        }
    }
#ifdef UDF_DBG
    if(!Found) {
        KdPrint(("Partition (%d) not found in partition map\n", (p->partitionNumber) ));
    } else {
        KdPrint(("Partition (%d:%d type %x) starts at physical %x, length %x\n",
            p->partitionNumber, i-1, Vcb->Partitions[i-1].PartitionType,
            Vcb->Partitions[i-1].PartitionRoot, Vcb->Partitions[i-1].PartitionLen));
    }
#endif // UDF_DBG
    return STATUS_SUCCESS;
} // end UDFVerifyPartDesc()

/*
    This routine scans VDS & fills special array with Desc locations
 */
OSSTATUS
UDFReadVDS(
    IN PVCB Vcb,
    IN uint32 block,
    IN uint32 lastblock,
    IN PUDF_VDS_RECORD vds,
    IN int8* Buf
    )
{
    OSSTATUS status;
    GenericDesc* gd;
    BOOLEAN done=FALSE;
    uint32 vdsn;
    uint16 ident;

    KdPrint(("UDF: Read VDS (%x - %x)\n", block, lastblock ));
    // Read the main descriptor sequence
    for (;(!done && block <= lastblock); block++)
    {
        status = UDFReadTagged(Vcb, Buf, block, block, &ident);
        if(!OS_SUCCESS(status))
            return status;
        UDFRegisterFsStructure(Vcb, block, Vcb->BlockSize);

        // Process each descriptor (ISO 13346 3/8.3-8.4)
        gd = (struct GenericDesc *)Buf;
        vdsn = gd->volDescSeqNum;
        KdPrint(("LBA %x, Ident = %x, vdsn = %x\n", block, ident, vdsn ));
        switch (ident)
        {
            case TID_PRIMARY_VOL_DESC: // ISO 13346 3/10.1
                if(vdsn >= vds[VDS_POS_PRIMARY_VOL_DESC].volDescSeqNum)
                {
                    vds[VDS_POS_PRIMARY_VOL_DESC].volDescSeqNum = vdsn;
                    vds[VDS_POS_PRIMARY_VOL_DESC].block = block;
                }
                break;
            case TID_VOL_DESC_PTR: // ISO 13346 3/10.3
                struct VolDescPtr* pVDP;
                if(vdsn >= vds[VDS_POS_VOL_DESC_PTR].volDescSeqNum)
                {
                    vds[VDS_POS_VOL_DESC_PTR].volDescSeqNum = vdsn;
                    vds[VDS_POS_VOL_DESC_PTR].block = block;
                    vds[VDS_POS_RECURSION_COUNTER].volDescSeqNum++;
                    if(vds[VDS_POS_RECURSION_COUNTER].volDescSeqNum > MAX_VDS_PARTS) {
                       KdPrint(("too long multipart VDS -> abort\n"));
                        return STATUS_DISK_CORRUPT_ERROR;
                    }
                    pVDP = (struct VolDescPtr*)Buf;
                    KdPrint(("multipart VDS...\n"));
                    return UDFReadVDS(Vcb, pVDP->nextVolDescSeqExt.extLocation,
                                         pVDP->nextVolDescSeqExt.extLocation + (pVDP->nextVolDescSeqExt.extLocation >> Vcb->BlockSizeBits),
                                         vds, Buf);
                }
                break;
            case TID_IMP_USE_VOL_DESC: // ISO 13346 3/10.4
                if(vdsn >= vds[VDS_POS_IMP_USE_VOL_DESC].volDescSeqNum)
                {
                    vds[VDS_POS_IMP_USE_VOL_DESC].volDescSeqNum = vdsn;
                    vds[VDS_POS_IMP_USE_VOL_DESC].block = block;
                }
                break;
            case TID_PARTITION_DESC: // ISO 13346 3/10.5
                if(!vds[VDS_POS_PARTITION_DESC].block)
                    vds[VDS_POS_PARTITION_DESC].block = block;
                break;
            case TID_LOGICAL_VOL_DESC: // ISO 13346 3/10.6
            case TID_ADAPTEC_LOGICAL_VOL_DESC: // Adaptec Compressed UDF extesion
                if(vdsn >= vds[VDS_POS_LOGICAL_VOL_DESC].volDescSeqNum)
                {
                    vds[VDS_POS_LOGICAL_VOL_DESC].volDescSeqNum = vdsn;
                    vds[VDS_POS_LOGICAL_VOL_DESC].block = block;
                }
                break;
            case TID_UNALLOC_SPACE_DESC: // ISO 13346 3/10.8
                if(vdsn >= vds[VDS_POS_UNALLOC_SPACE_DESC].volDescSeqNum)
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

OSSTATUS
UDFLoadImpUseVolDesc(
    IN PVCB   Vcb,
    int8*     Buf
    )
{
    ImpUseVolDesc* iuvd = (ImpUseVolDesc*)Buf;
    ImpUseVolDescImpUse* iuvdiu = (ImpUseVolDescImpUse*)&(iuvd->impUse);
    KdPrint(("UDF: Imp Use Vol Desc:\n"));
    KdPrint((" volDescSeqNum = %x\n", iuvd->volDescSeqNum));
    KdPrint(("UDF: Imp Use Vol Desc Imp Use:\n"));
    KdDump(iuvdiu, sizeof(ImpUseVolDescImpUse));
    return STATUS_SUCCESS;
} // UDFLoadImpUseVolDesc()

OSSTATUS
UDFLoadUnallocatedSpaceDesc(
    IN PVCB   Vcb,
    int8*     Buf
    )
{
    KdPrint(("UDF: Unallocated Space Desc:\n"));
//    UnallocatedSpaceDesc* usd = (UnallocatedSpaceDesc*)Buf;
    return STATUS_SUCCESS;
} // UDFLoadImpUseVolDesc()

/*
    Process a main/reserve volume descriptor sequence.
*/
OSSTATUS
UDFProcessSequence(
     IN PDEVICE_OBJECT    DeviceObject,
     IN PVCB              Vcb,
     IN uint32            block,
     IN uint32            lastblock,
    OUT lb_addr           *fileset
    )
{
    OSSTATUS    RC = STATUS_SUCCESS;
    int8*       Buf = (int8*)MyAllocatePool__(NonPagedPool,Vcb->BlockSize);
    UDF_VDS_RECORD vds[VDS_POS_LENGTH];
    GenericDesc   *gd;
    uint32   i,j;
    uint16  ident;
    int8*  Buf2 = NULL;

    _SEH2_TRY {
        if(!Buf) try_return(RC = STATUS_INSUFFICIENT_RESOURCES);
        RtlZeroMemory(vds, sizeof(UDF_VDS_RECORD) * VDS_POS_LENGTH);
        if(!OS_SUCCESS(RC = UDFReadVDS(Vcb, block, lastblock, (PUDF_VDS_RECORD)&vds, Buf)))
            try_return(RC);
        // walk through Vol Desc Sequence according to locations gained by
        // UDFReadVDS() & do some procesing for each one
        // It is very simple dispath routine...
        for (i=0; i<VDS_POS_LENGTH; i++)
        {
            if(vds[i].block)
            {
                if(!OS_SUCCESS(RC = UDFReadTagged(Vcb, Buf, vds[i].block, vds[i].block, &ident)))
                    try_return(RC);
                UDFRegisterFsStructure(Vcb, vds[i].block, Vcb->BlockSize);

                if(i == VDS_POS_PRIMARY_VOL_DESC) {
                    UDFLoadPVolDesc(Vcb,Buf);
                    if(!Vcb->PVolDescAddr.block) {
                        Vcb->PVolDescAddr = vds[i];
                    } else {
                        Vcb->PVolDescAddr2 = vds[i];
                    }
                } else
                if(i == VDS_POS_LOGICAL_VOL_DESC) {
                    RC = UDFLoadLogicalVol(DeviceObject,Vcb, Buf, fileset);
                    if(!OS_SUCCESS(RC)) try_return(RC);
                } else

                if(i == VDS_POS_IMP_USE_VOL_DESC) {
                    UDFLoadImpUseVolDesc(Vcb, Buf);
                } else
                if(i == VDS_POS_UNALLOC_SPACE_DESC) {
                    UDFLoadUnallocatedSpaceDesc(Vcb, Buf);
                } else

                if(i == VDS_POS_PARTITION_DESC)
                {
                    Buf2 = (int8*)MyAllocatePool__(NonPagedPool,Vcb->BlockSize);
                    if(!Buf2) try_return(RC = STATUS_INSUFFICIENT_RESOURCES);
                    RC = UDFLoadPartDesc(Vcb,Buf);
                    if(!OS_SUCCESS(RC)) try_return(RC);
                    for (j=vds[i].block+1; j<vds[VDS_POS_TERMINATING_DESC].block; j++)
                    {
                        RC = UDFReadTagged(Vcb,Buf2, j, j, &ident);
                        if(!OS_SUCCESS(RC)) try_return(RC);
                        UDFRegisterFsStructure(Vcb, j, Vcb->BlockSize);
                        gd = (struct GenericDesc *)Buf2;
                        if(ident == TID_PARTITION_DESC) {
                            RC = UDFLoadPartDesc(Vcb,Buf2);
                            if(!OS_SUCCESS(RC)) try_return(RC);
                        } else if(ident == TID_UNALLOC_SPACE_DESC) {
                            RC = UDFBuildFreeSpaceBitmap(Vcb,0,NULL,j);
                            //Vcb->Modified = FALSE;
                            UDFPreClrModified(Vcb);
                            UDFClrModified(Vcb);
                            if(!OS_SUCCESS(RC))
                                try_return(RC);
                        }
                    }
                    MyFreePool__(Buf2);
                    Buf2 = NULL;
                }
            } else {
                if(i == VDS_POS_LOGICAL_VOL_DESC) {
                    RC = UDFLoadBogusLogicalVol(DeviceObject,Vcb, Buf, fileset);
                    if(!OS_SUCCESS(RC)) try_return(RC);
                }
            }
        }
    
try_exit: NOTHING;
  
    } _SEH2_FINALLY {
        if(Buf) MyFreePool__(Buf);
        if(Buf2) MyFreePool__(Buf2);
    } _SEH2_END;

    return RC;
} // end UDFProcessSequence()

/*
    Verifies a main/reserve volume descriptor sequence.
*/
OSSTATUS
UDFVerifySequence(
     IN PDEVICE_OBJECT    DeviceObject,
     IN PVCB              Vcb,
     IN uint32             block,
     IN uint32             lastblock,
     OUT lb_addr          *fileset
     )
{
    OSSTATUS    RC = STATUS_SUCCESS;
    int8*       Buf = (int8*)MyAllocatePool__(NonPagedPool,Vcb->BlockSize);
    UDF_VDS_RECORD vds[VDS_POS_LENGTH];
    GenericDesc   *gd;
    uint32   i,j;
    uint16  ident;
    int8*  Buf2 = NULL;

    _SEH2_TRY {
        if(!Buf) try_return(RC = STATUS_INSUFFICIENT_RESOURCES);
        if(!block) try_return (RC = STATUS_SUCCESS);
        RtlZeroMemory(vds, sizeof(UDF_VDS_RECORD) * VDS_POS_LENGTH);
        if(!OS_SUCCESS(RC = UDFReadVDS(Vcb, block, lastblock, (PUDF_VDS_RECORD)&vds, Buf)))
            try_return(RC);

        for (i=0; i<VDS_POS_LENGTH; i++)
        {
            if(vds[i].block)
            {
                if(!OS_SUCCESS(RC = UDFReadTagged(Vcb, Buf, vds[i].block, vds[i].block, &ident)))
                    try_return(RC);
                UDFRegisterFsStructure(Vcb, vds[i].block, Vcb->BlockSize);
    
    /*            if(i == VDS_POS_PRIMARY_VOL_DESC)
                    UDFLoadPVolDesc(Vcb,Buf);
                else if(i == VDS_POS_LOGICAL_VOL_DESC) {
                    RC = UDFLoadLogicalVol(DeviceObject,Vcb, Buf, fileset);
                    if(!OS_SUCCESS(RC)) try_return(RC);
                }
                else*/ if(i == VDS_POS_PARTITION_DESC)
                {
                    Buf2 = (int8*)MyAllocatePool__(NonPagedPool,Vcb->BlockSize);
                    if(!Buf2) try_return(RC = STATUS_INSUFFICIENT_RESOURCES);
                    RC = UDFVerifyPartDesc(Vcb,Buf);
                    if(!OS_SUCCESS(RC)) try_return(RC);
                    for (j=vds[i].block+1; j<vds[VDS_POS_TERMINATING_DESC].block; j++)
                    {
                        RC = UDFReadTagged(Vcb,Buf2, j, j, &ident);
                        if(!OS_SUCCESS(RC)) try_return(RC);
                        UDFRegisterFsStructure(Vcb, j, Vcb->BlockSize);
                        gd = (struct GenericDesc *)Buf2;
                        if(ident == TID_PARTITION_DESC) {
                            RC = UDFVerifyPartDesc(Vcb,Buf2);
                            if(!OS_SUCCESS(RC)) try_return(RC);
                        } else if(ident == TID_UNALLOC_SPACE_DESC) {
                            RC = UDFVerifyFreeSpaceBitmap(Vcb,0,NULL,j);
                            Vcb->Modified = FALSE;
                            if(!OS_SUCCESS(RC))
                                try_return(RC);
                        }
                    }
                    MyFreePool__(Buf2);
                    Buf2 = NULL;
                }
            }
        }
try_exit: NOTHING;

    } _SEH2_FINALLY {
        if(Buf) MyFreePool__(Buf);
        if(Buf2) MyFreePool__(Buf2);
    } _SEH2_END;

    return RC;
} // end UDFVerifySequence()

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
    KdPrint(("Rootdir at block=%x, partition=%d\n",
        root->logicalBlockNum, root->partitionReferenceNum));
    if(sysstream) {
        *sysstream = fset->streamDirectoryICB.extLocation;
        KdPrint(("SysStream at block=%x, partition=%d\n",
            sysstream->logicalBlockNum, sysstream->partitionReferenceNum));
    }
    // Get current UDF revision
    // Get Read-Only flags
    UDFReadEntityID_Domain(Vcb, &(fset->domainIdent));

} // end UDFLoadFileset()

OSSTATUS
UDFIsCachedBadSequence(
    IN PVCB Vcb,
    IN uint32 Lba
    )
{
    ULONG j;
    OSSTATUS RC = STATUS_SUCCESS;
    // Check if it is known bad sequence
    for(j=0; j<Vcb->BadSeqLocIndex; j++) {
        if(Vcb->BadSeqLoc[j] == Lba) {
            RC = Vcb->BadSeqStatus[j];
            break;
        }
    }
    return RC;
} // end UDFIsCachedBadSequence()

VOID
UDFRememberBadSequence(
    IN PVCB Vcb,
    IN uint32 Lba,
    IN OSSTATUS RC
    )
{
    int j;
    if(!OS_SUCCESS(UDFIsCachedBadSequence(Vcb, Lba)))
        return;
    // Remenber bad sequence
    j = Vcb->BadSeqLocIndex;
    Vcb->BadSeqLocIndex++;
    Vcb->BadSeqLoc[j]    = Lba;
    Vcb->BadSeqStatus[j] = RC;
} // end UDFRememberBadSequence()

/*
    load partition info
 */
OSSTATUS
UDFLoadPartition(
     IN PDEVICE_OBJECT  DeviceObject,
     IN PVCB            Vcb,
    OUT lb_addr         *fileset
    )
{
    OSSTATUS            RC = STATUS_UNRECOGNIZED_VOLUME;
    OSSTATUS            RC2 = STATUS_UNRECOGNIZED_VOLUME;
    AnchorVolDescPtr    *anchor;
    uint16              ident;
    int8*               Buf = (int8*)MyAllocatePool__(NonPagedPool,Vcb->BlockSize);
    uint32              main_s, main_e;
    uint32              reserve_s, reserve_e;
    int                 i;

    if(!Buf) return STATUS_INSUFFICIENT_RESOURCES;
    // walk through all available Anchors & load data
    for (i=0; i<MAX_ANCHOR_LOCATIONS; i++)
    {
        if(Vcb->Anchor[i] && (OS_SUCCESS(UDFReadTagged(Vcb, Buf,
            Vcb->Anchor[i], Vcb->Anchor[i] - Vcb->FirstLBA, &ident))))
        {
            anchor = (AnchorVolDescPtr *)Buf;

            // Locate the main sequence
            main_s = ( anchor->mainVolDescSeqExt.extLocation );
            main_e = ( anchor->mainVolDescSeqExt.extLength );
            main_e = main_e >> Vcb->BlockSizeBits;
            main_e += main_s;

            // Locate the reserve sequence
            reserve_s = (anchor->reserveVolDescSeqExt.extLocation);
            reserve_e = (anchor->reserveVolDescSeqExt.extLength);
            reserve_e = reserve_e >> Vcb->BlockSizeBits;
            reserve_e += reserve_s;

            // Check if it is known bad sequence
            RC = UDFIsCachedBadSequence(Vcb, main_s);
            if(OS_SUCCESS(RC)) {
                // Process the main & reserve sequences
                // responsible for finding the PartitionDesc(s)
                KdPrint(("-----------------------------------\n"));
                KdPrint(("UDF: Main sequence:\n"));
                RC = UDFProcessSequence(DeviceObject, Vcb, main_s, main_e, fileset);
            }

            if(!OS_SUCCESS(RC)) {
                // Remenber bad sequence
                UDFRememberBadSequence(Vcb, main_s, RC);

                KdPrint(("-----------------------------------\n"));
                KdPrint(("UDF: Main sequence failed.\n"));
                KdPrint(("UDF: Reserve sequence\n"));
                if(Vcb->LVid) MyFreePool__(Vcb->LVid);
                Vcb->LVid = NULL;

                RC2 = UDFIsCachedBadSequence(Vcb, reserve_s);
                if(OS_SUCCESS(RC2)) {
                    RC2 = UDFProcessSequence(DeviceObject, Vcb, reserve_s, reserve_e, fileset);
                }

                if(OS_SUCCESS(RC2)) {
                    KdPrint(("-----------------------------------\n"));
                    Vcb->VDS2_Len = reserve_e - reserve_s;
                    Vcb->VDS2 = reserve_s;
                    RC = STATUS_SUCCESS;
                    // Vcb is already Zero-filled
//                    Vcb->VDS1_Len = 0;
//                    Vcb->VDS1 = 0;
                    break;
                } else {
                    // This is also bad sequence. Remenber it too
                    UDFRememberBadSequence(Vcb, reserve_s, RC);
                }
            } else {
                // remember these values for umount__
                Vcb->VDS1_Len = main_e - main_s;
                Vcb->VDS1 = main_s;
/*                if(Vcb->LVid) MyFreePool__(Vcb->LVid);
                Vcb->LVid = NULL;*/
                if(OS_SUCCESS(UDFVerifySequence(DeviceObject, Vcb, reserve_s, reserve_e, fileset)))
                {
                    KdPrint(("-----------------------------------\n"));
                    Vcb->VDS2_Len = reserve_e - reserve_s;
                    Vcb->VDS2 = reserve_s;
                    break;
                } else {
                    KdPrint(("UDF: Reserve sequence verification failed.\n"));
                    switch(Vcb->PartitialDamagedVolumeAction) {
                    case UDF_PART_DAMAGED_RO:
                        KdPrint(("UDF: Switch to r/o mode.\n"));
                        Vcb->VCBFlags |= UDF_VCB_FLAGS_VOLUME_READ_ONLY;
                        break;
                    case UDF_PART_DAMAGED_NO:
                        KdPrint(("UDF: Switch to raw mount mode, return UNRECOGNIZED_VOLUME.\n"));
                        Vcb->VCBFlags |= UDF_VCB_FLAGS_RAW_DISK;
                        RC = STATUS_WRONG_VOLUME;
                        break;
                    case UDF_PART_DAMAGED_RW:
                    default:
                        KdPrint(("UDF: Keep r/w mode for your own risk.\n"));
                        break;
                    }
                }
                break;
            }
        }
    }

    if(Vcb->SparingCount &&
       (Vcb->NoFreeRelocationSpaceVolumeAction != UDF_PART_DAMAGED_RW)) {
        KdPrint(("UDF: No free Sparing Entries -> Switch to r/o mode.\n"));
        Vcb->VCBFlags |= UDF_VCB_FLAGS_VOLUME_READ_ONLY;
    }

    if(i == sizeof(Vcb->Anchor)/sizeof(int)) {
        KdPrint(("No Anchor block found\n"));
        RC = STATUS_UNRECOGNIZED_VOLUME;
#ifdef UDF_DBG
    } else {
        KdPrint(("Using anchor in block %x\n", Vcb->Anchor[i]));
#endif // UDF_DBG
    }
    MyFreePool__(Buf);
    return RC;
} // end UDFLoadPartition()

/*
    This routine scans FileSet sequence & returns pointer to last valid
    FileSet
 */
OSSTATUS
UDFFindLastFileSet(
    IN PVCB Vcb,
    IN lb_addr *Addr,  // Addr for the 1st FileSet
    IN OUT PFILE_SET_DESC FileSetDesc
    )
{
    OSSTATUS status;
    uint32 relLocExt = Addr->logicalBlockNum;
    uint32 locExt = UDFPartLbaToPhys(Vcb, Addr);
    uint16 Ident;
    uint32 relPrevExt, prevExt;

    relPrevExt, prevExt = NULL;
    FileSetDesc->nextExt.extLength = 1;  // ;)
    // walk through FileSet chain
    // we've just pre-init'd extent length to read 1st FileSet
    while(FileSetDesc->nextExt.extLength) {
        status = UDFReadTagged(Vcb, (int8*)FileSetDesc, locExt, relLocExt, &Ident);
        if(!OS_SUCCESS(status)) {
            FileSetDesc->nextExt.extLength = 0;
            return status;
        }
        UDFRegisterFsStructure(Vcb, locExt, Vcb->BlockSize);
        if((locExt == LBA_OUT_OF_EXTENT) || (Ident != TID_FILE_SET_DESC)) {
            // try to read previous FileSet
            if(!prevExt) return STATUS_UNRECOGNIZED_VOLUME;
            status = UDFReadTagged(Vcb, (int8*)FileSetDesc, prevExt, relLocExt, &Ident);
            if(OS_SUCCESS(status)) {
                UDFRegisterFsStructure(Vcb, prevExt, Vcb->BlockSize);
            }
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
OSSTATUS
UDFLoadSparingTable(
    IN PVCB Vcb,
    IN PSPARABLE_PARTITION_MAP PartMap
    )
{
    PSPARING_MAP RelocMap;
    PSPARING_MAP NewRelocMap;
    OSSTATUS status;
    uint32 i=0, BC, BC2;
    PSPARING_TABLE SparTable;
    uint32 TabSize, NewSize;
    uint32 ReadBytes;
    uint32 SparTableLoc;
#ifdef UDF_TRACK_FS_STRUCTURES
    uint32 j;
#endif //UDF_TRACK_FS_STRUCTURES
    uint32 n,m;
    BOOLEAN merged;

    Vcb->SparingCountFree = -1;

    KdPrint(("UDF: Sparable Part Map:\n"));
    Vcb->SparingTableLength = PartMap->sizeSparingTable;
    BC = (PartMap->sizeSparingTable >> Vcb->BlockSizeBits) + 1;
    KdPrint((" partitionMapType   = %x\n", PartMap->partitionMapType));
    KdPrint((" partitionMapLength = %x\n", PartMap->partitionMapLength));
    KdPrint((" volSeqNum          = %x\n", PartMap->volSeqNum));
    KdPrint((" partitionNum       = %x\n", PartMap->partitionNum));
    KdPrint((" packetLength       = %x\n", PartMap->packetLength));
    KdPrint((" numSparingTables   = %x\n", PartMap->numSparingTables));
    KdPrint((" sizeSparingTable   = %x\n", PartMap->sizeSparingTable));
    SparTable = (PSPARING_TABLE)MyAllocatePool__(NonPagedPool, BC*Vcb->BlockSize);
    if(!SparTable) return STATUS_INSUFFICIENT_RESOURCES;
    if(Vcb->SparingTable) {
        // if a part of Sparing Table is already loaded,
        // update it with data from another one
        RelocMap = Vcb->SparingTable;
        TabSize = Vcb->SparingCount * sizeof(SPARING_ENTRY);
    } else {
        // do some init to load first part of Sparing Table
        RelocMap = (PSPARING_MAP)MyAllocatePool__(NonPagedPool, RELOC_MAP_GRAN);
        if(!RelocMap) {
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
            if(Vcb->SparingTableLoc[i] == SparTableLoc) {
                KdPrint((" already processed @%x\n", 
                    SparTableLoc
                    ));
                continue;
            }
        }
        status = UDFReadSectors(Vcb, FALSE, SparTableLoc, 1, FALSE, (int8*)SparTable, &ReadBytes);
        // tag should be set to TID_UNUSED_DESC
        if(OS_SUCCESS(status) && (SparTable->descTag.tagIdent == TID_UNUSED_DESC)) {

            UDFRegisterFsStructure(Vcb,  SparTableLoc, Vcb->BlockSize);
            BC2 = ((sizeof(SPARING_TABLE) + 
                    SparTable->reallocationTableLen*sizeof(SparingEntry) +
                    Vcb->BlockSize-1) 
                                      >> Vcb->BlockSizeBits);
            if(BC2 > BC) {
                KdPrint((" sizeSparingTable @%x too long: %x > %x\n", 
                    SparTableLoc, BC2, BC
                    ));
                continue;
            }
            status = UDFReadSectors(Vcb, FALSE, SparTableLoc,
                BC2, FALSE, (int8*)SparTable, &ReadBytes);
            UDFRegisterFsStructure(Vcb,  SparTableLoc, BC2<<Vcb->BlockSizeBits);
        
            if(!OS_SUCCESS(status)) {
                KdPrint((" Error reading sizeSparingTable @%x (%x)\n", 
                    SparTableLoc, BC2
                    ));
                continue;
            }
            // process sparing table
            NewSize = sizeof(SparingEntry)*SparTable->reallocationTableLen;
            TabSize = MyReallocPool__((int8*)RelocMap, TabSize, (int8**)&RelocMap, TabSize+NewSize);
            if(!TabSize) {
                MyFreePool__(SparTable);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

#ifdef UDF_TRACK_FS_STRUCTURES
            for(j=0; j<SparTable->reallocationTableLen; j++) {
                UDFRegisterFsStructure(Vcb,  ((SparingEntry*)(SparTable+1))[j].mappedLocation, Vcb->WriteBlockSize);
            }
#endif //UDF_TRACK_FS_STRUCTURES

            Vcb->SparingTableLoc[Vcb->SparingTableCount] = SparTableLoc;
            Vcb->SparingTableCount++;

            NewRelocMap = (PSPARING_MAP)(SparTable+1);
            for(n=0; n<SparTable->reallocationTableLen; n++) {
                merged = TRUE;
                for(m=0; m<Vcb->SparingCount; m++) {
                    if(RelocMap[m].mappedLocation == NewRelocMap[n].mappedLocation) {
                        KdPrint(("  dup @%x (%x) vs @%x (%x)\n", 
                            RelocMap[m].origLocation, RelocMap[m].mappedLocation,
                            NewRelocMap[m].origLocation, NewRelocMap[m].mappedLocation));
                        merged = FALSE;
                    }
                    if((RelocMap[m].origLocation   == NewRelocMap[n].origLocation) &&
                       (RelocMap[m].mappedLocation != NewRelocMap[n].mappedLocation) &&
                       (RelocMap[m].origLocation != SPARING_LOC_AVAILABLE) &&
                       (RelocMap[m].origLocation != SPARING_LOC_CORRUPTED)) {
                        KdPrint(("  conflict @%x (%x) vs @%x (%x)\n", 
                            RelocMap[m].origLocation, RelocMap[m].mappedLocation,
                            NewRelocMap[n].origLocation, NewRelocMap[n].mappedLocation));
                        merged = FALSE;
                    }
                }
                if(merged) {
                    RelocMap[Vcb->SparingCount] = NewRelocMap[n];
                    KdPrint(("  reloc %x -> %x\n", 
                        RelocMap[Vcb->SparingCount].origLocation, RelocMap[Vcb->SparingCount].mappedLocation));
                    Vcb->SparingCount++;
                    if(RelocMap[Vcb->SparingCount].origLocation == SPARING_LOC_AVAILABLE) {
                        Vcb->NoFreeRelocationSpaceVolumeAction = UDF_PART_DAMAGED_RW;
                    }
                }
            }

/*
            RtlCopyMemory((int8*)(RelocMap+Vcb->SparingCount),
                          (int8*)(SparTable+1), NewSize);
            Vcb->SparingCount += NewSize/sizeof(SPARING_ENTRY);
*/
            if(Vcb->SparingTableCount >= MAX_SPARING_TABLE_LOCATIONS) {
                KdPrint(("    too many Sparing Tables\n"));
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
BOOLEAN
UDFCheckZeroBuf(
    IN int8* Buf,
    IN uint32 Length
    )
{
    BOOLEAN RC = FALSE;

//#ifdef _X86_
#ifdef _MSC_VER

    uint32 len = Length;
    __asm push  ecx
    __asm push  edi

    __asm mov   ecx,len
    __asm mov   edi,Buf
    __asm xor   eax,eax
    __asm shr   ecx,2
    __asm repe scasd
    __asm jne   short not_all_zeros
    __asm mov   RC,1

not_all_zeros:

    __asm pop   edi
    __asm pop   ecx

    return RC;

#else // _X86_

    uint32* tmp = (uint32*)Buf;
    uint32 i;

    for(i=0; i<Length/4; i++) {
        if(tmp[i]) return FALSE;
    }
    return TRUE;

#endif // _X86_

} // end UDFCheckZeroBuf()

/*
    check if this is an UDF-formatted disk
*/
OSSTATUS
UDFGetDiskInfoAndVerify(
    IN PDEVICE_OBJECT DeviceObject,      // the target device object
    IN PVCB           Vcb                // Volume control block from this DevObj
    )
{
    OSSTATUS        RC = STATUS_UNRECOGNIZED_VOLUME;
    uint32          NSRDesc;
    lb_addr         fileset;
    PFILE_SET_DESC  FileSetDesc = NULL;

    int8*           Buf = NULL;
    uint32          ReadBytes;

    KdPrint(("UDFGetDiskInfoAndVerify\n"));
    _SEH2_TRY {

        if(!UDFFindAnchor(Vcb)) {
            if(Vcb->FsDeviceType == FILE_DEVICE_CD_ROM_FILE_SYSTEM) {
                // check if this disc is mountable for CDFS
                KdPrint(("   FILE_DEVICE_CD_ROM_FILE_SYSTEM\n"));
check_NSR:
                NSRDesc = UDFFindVRS(Vcb);
                if(!(NSRDesc & VRS_ISO9660_FOUND)) {
                    // no CDFS VRS found
                    KdPrint(("UDFGetDiskInfoAndVerify: no CDFS VRS found\n"));
                    if(!Vcb->TrackMap[Vcb->LastTrackNum].LastLba &&
                       !Vcb->TrackMap[Vcb->FirstTrackNum].LastLba) {
                        // such a stupid method of Audio-CD detection...
                        KdPrint(("UDFGetDiskInfoAndVerify: set UDF_VCB_FLAGS_RAW_DISK\n"));
                        Vcb->VCBFlags |= UDF_VCB_FLAGS_RAW_DISK;
                    }
                }
                Vcb->NSRDesc = NSRDesc;

                Buf = (int8*)MyAllocatePool__(NonPagedPool, 0x10000);
                if(!Buf) try_return(RC = STATUS_INSUFFICIENT_RESOURCES);
                RC = UDFReadData(Vcb, FALSE, 0, 0x10000, FALSE, Buf, &ReadBytes);
                if(!OS_SUCCESS(RC))
                    try_return(RC = STATUS_UNRECOGNIZED_VOLUME);
                RC = STATUS_UNRECOGNIZED_VOLUME;
                if(!UDFCheckZeroBuf(Buf,0x10000)) {
                    KdPrint(("UDFGetDiskInfoAndVerify: possible FS detected, remove UDF_VCB_FLAGS_RAW_DISK\n"));
                    Vcb->VCBFlags &= ~UDF_VCB_FLAGS_RAW_DISK;
                }
                MyFreePool__(Buf);
                Buf = NULL;
            }
            try_return(RC = STATUS_UNRECOGNIZED_VOLUME);
        }

        RC = UDFLoadPartition(DeviceObject,Vcb,&fileset);
        if(!OS_SUCCESS(RC)) {
            if(RC == STATUS_UNRECOGNIZED_VOLUME) {
                KdPrint(("UDFGetDiskInfoAndVerify: check NSR presence\n"));
                goto check_NSR;
            }
            try_return(RC);
        }

        FileSetDesc = (PFILE_SET_DESC)MyAllocatePool__(NonPagedPool,Vcb->BlockSize);
        if(!FileSetDesc) try_return(RC = STATUS_INSUFFICIENT_RESOURCES);

        RC = UDFFindLastFileSet(Vcb,&fileset,FileSetDesc);
        if(!OS_SUCCESS(RC)) try_return(RC);

        UDFLoadFileset(Vcb,FileSetDesc, &(Vcb->RootLbAddr), &(Vcb->SysStreamLbAddr));

        Vcb->FSBM_OldBitmap = (int8*)DbgAllocatePool(NonPagedPool, Vcb->FSBM_ByteCount);
        if(!(Vcb->FSBM_OldBitmap)) try_return(RC = STATUS_INSUFFICIENT_RESOURCES);
        RtlCopyMemory(Vcb->FSBM_OldBitmap, Vcb->FSBM_Bitmap, Vcb->FSBM_ByteCount);

try_exit:   NOTHING;
    } _SEH2_FINALLY {
        if(FileSetDesc)   MyFreePool__(FileSetDesc);
        if(Buf)           MyFreePool__(Buf);
    } _SEH2_END;

    return(RC);

} // end UDFGetDiskInfoAndVerify()

