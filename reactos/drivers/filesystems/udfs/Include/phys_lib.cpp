////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
////////////////////////////////////////////////////////////////////
/*
 Module Name: Phys_lib.cpp

 Execution: Kernel mode only

 Description:

   Contains code that implement read/write operations for physical device
*/

#include "phys_lib.h"

static const char Signature [16] = {CDRW_SIGNATURE_v1};

// Local functions:

OSSTATUS
UDFSetSpeeds(
    IN PVCB Vcb
    );

NTSTATUS
UDFSetCaching(
    IN PVCB Vcb
    );

OSSTATUS
UDFRecoverFromError(
    IN PVCB Vcb,
    IN BOOLEAN WriteOp,
    IN OSSTATUS status,
    IN uint32 Lba,
    IN uint32 BCount,
 IN OUT uint32* retry);

#ifdef _BROWSE_UDF_

uint32
UDFFixFPAddress(
    IN PVCB           Vcb,               // Volume control block from this DevObj
    IN uint32         Lba
    );

#endif //_BROWSE_UDF_

NTSTATUS
UDFSyncCache(
    IN PVCB Vcb
    )
{
    KdPrint(("UDFSyncCache:\n"));
    OSSTATUS RC;
    RC = UDFPhSendIOCTL( IOCTL_CDRW_SYNC_CACHE, Vcb->TargetDeviceObject,
                    NULL,0, NULL,0, FALSE, NULL);
    if(OS_SUCCESS(RC)) {
        // clear LAST_WRITE flag
        Vcb->VCBFlags &= ~UDF_VCB_LAST_WRITE;
    }
    return RC;
} // end UDFSyncCache()


OSSTATUS
UDFReallocTrackMap(
    IN PVCB Vcb,
    IN uint32 TrackNum
    )
{
#ifdef _BROWSE_UDF_
    if(Vcb->TrackMap) {
        MyFreePool__(Vcb->TrackMap);
        Vcb->TrackMap = NULL;
    }
    Vcb->TrackMap = (PUDFTrackMap)
        MyAllocatePool__(NonPagedPool, TrackNum*sizeof(UDFTrackMap));
    if(!Vcb->TrackMap) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
#endif //_BROWSE_UDF_
    RtlZeroMemory(Vcb->TrackMap,TrackNum*sizeof(UDFTrackMap));
    return STATUS_SUCCESS;
} // end UDFReallocTrackMap()

#ifdef _BROWSE_UDF_


OSSTATUS
__fastcall
UDFTIOVerify(
    IN void* _Vcb,
    IN void* Buffer,     // Target buffer
    IN uint32 Length,
    IN uint32 LBA,
    OUT uint32* IOBytes,
    IN uint32 Flags
    )
{
    OSSTATUS RC = STATUS_SUCCESS;
    uint32 i, j;
    uint32 mask;
    uint32 lba0, len, lba1;
    PUCHAR tmp_buff;
    PUCHAR p;
    PCHAR cached_block;
    uint32 tmp_wb;
    BOOLEAN need_remap;
    OSSTATUS final_RC = STATUS_SUCCESS;
    BOOLEAN zero;
    BOOLEAN non_zero;
    BOOLEAN packet_ok;
    BOOLEAN free_tmp = FALSE;
    BOOLEAN single_packet = FALSE;

#define Vcb ((PVCB)_Vcb)
    // ATTENTION! Do not touch bad block bitmap here, since it describes PHYSICAL addresses WITHOUT remapping,
    // while here we work with LOGICAL addresses

    if(Vcb->VerifyCtx.ItemCount > UDF_MAX_VERIFY_CACHE) {
        UDFVVerify(Vcb, 0/*UFD_VERIFY_FLAG_WAIT*/);
    }

    UDFAcquireResourceExclusive(&(Vcb->IoResource), TRUE);
    Flags |= PH_IO_LOCKED;

    tmp_wb = (uint32)_Vcb;
    if(Flags & PH_EX_WRITE) {
        KdPrint(("IO-Write-Verify\n"));
        RC = UDFTWrite(_Vcb, Buffer, Length, LBA, &tmp_wb, Flags | PH_VCB_IN_RETLEN);
    } else {
        KdPrint(("IO-Read-Verify\n"));
        RC = UDFTRead(_Vcb, Buffer, Length, LBA, &tmp_wb, Flags | PH_VCB_IN_RETLEN);
    }
    (*IOBytes) = tmp_wb;

    switch(RC) {
    default:
        UDFReleaseResource(&(Vcb->IoResource));
        return RC;
    case STATUS_FT_WRITE_RECOVERY:
    case STATUS_DEVICE_DATA_ERROR:
    case STATUS_IO_DEVICE_ERROR:
        break;
        /* FALL THROUGH */
    } // end switch(RC)

    if(!Vcb->SparingCount ||
       !Vcb->SparingCountFree ||
       Vcb->CDR_Mode) {
        KdPrint(("Can't remap\n"));
        UDFReleaseResource(&(Vcb->IoResource));
        return RC;
    }

    if(Flags & PH_EX_WRITE) {
        KdPrint(("Write failed, try relocation\n"));
    } else {
        if(Vcb->Modified) {
            KdPrint(("Read failed, try relocation\n"));
        } else {
            KdPrint(("no remap on not modified volume\n"));
            UDFReleaseResource(&(Vcb->IoResource));
            return RC;
        }
    }
    if(Flags & PH_LOCK_CACHE) {
        UDFReleaseResource(&(Vcb->IoResource));
        WCacheStartDirect__(&(Vcb->FastCache), Vcb, TRUE);
        UDFAcquireResourceExclusive(&(Vcb->IoResource), TRUE);
    }

    Flags &= ~PH_KEEP_VERIFY_CACHE;

    // NOTE: SparingBlockSize may be not equal to PacketSize
    // perform recovery
    mask = Vcb->SparingBlockSize-1;
    lba0 = LBA & ~mask;
    len = ((LBA+(Length>>Vcb->BlockSizeBits)+mask) & ~mask) - lba0;
    j=0;
    if((lba0 == LBA) && (len == mask+1) && (len == (Length>>Vcb->BlockSizeBits))) {
        single_packet = TRUE;
        tmp_buff = NULL;
    } else {
        tmp_buff = (PUCHAR)DbgAllocatePoolWithTag(NonPagedPool, Vcb->SparingBlockSize << Vcb->BlockSizeBits, 'bNWD');
        if(!tmp_buff) {
            KdPrint(("  can't alloc tmp\n"));
            UDFReleaseResource(&(Vcb->IoResource));
            return STATUS_DEVICE_DATA_ERROR;
        }
        free_tmp = TRUE;
    }

    for(i=0; i<len; i++) {
        if(!Vcb->SparingCountFree) {
            KdPrint(("  no more free spare blocks, abort verification\n"));
            break;
        }
        KdPrint(("  read LBA %x (%x)\n", lba0+i, j));
        if(!j) {
            need_remap = FALSE;
            lba1 = lba0+i;
            non_zero = FALSE;
            if(single_packet) {
                // single packet requested
                tmp_buff = (PUCHAR)Buffer;
                if(Flags & PH_EX_WRITE) {
                    KdPrint(("  remap single write\n"));
                    KdPrint(("  try del from verify cache @ %x, %x\n", lba0, len));
                    UDFVForget(Vcb, len, UDFRelocateSector(Vcb, lba0), 0);
                    goto do_remap;
                } else {
                    KdPrint(("  recover and remap single read\n"));
                }
            }
        }
        p = tmp_buff+(j<<Vcb->BlockSizeBits);
        // not cached, try to read
        // prepare for error, if block cannot be read, assume it is zero-filled
        RtlZeroMemory(p, Vcb->BlockSize);

        // check if block valid
        if(Vcb->BSBM_Bitmap) {
            if(UDFGetBit__((uint32*)(Vcb->BSBM_Bitmap), UDFRelocateSector(Vcb, lba0+i))) {
                KdPrint(("  remap: known BB @ %x, mapped to %x\n", lba0+i, UDFRelocateSector(Vcb, lba0+i)));
                need_remap = TRUE;
            }
        }
        zero = FALSE;
        if(Vcb->FSBM_Bitmap) {
            if(UDFGetFreeBit((uint32*)(Vcb->FSBM_Bitmap), lba0+i)) {
                KdPrint(("  unused @ %x\n", lba0+i));
                zero = TRUE;
            }
        }
        if(!zero && Vcb->ZSBM_Bitmap) {
            if(UDFGetZeroBit((uint32*)(Vcb->ZSBM_Bitmap), lba0+i)) {
                KdPrint(("  unused @ %x (Z)\n", lba0+i));
                zero = TRUE;
            }
        }
        non_zero |= !zero;

        if(!j) {
            packet_ok = FALSE;
            if(!single_packet) {
                // try to read entire packet, this returs error more often then sequential reading of all blocks one by one
                tmp_wb = (uint32)_Vcb;
                RC = UDFTRead(_Vcb, p, Vcb->SparingBlockSize << Vcb->BlockSizeBits, lba0+i, &tmp_wb,
                              Flags | PH_READ_VERIFY_CACHE | PH_TMP_BUFFER | PH_VCB_IN_RETLEN);
            } else {
                // Note: we get here ONLY if original request failed
                // do not retry if it was single-packet request
                RC = STATUS_UNSUCCESSFUL;
            }
            if(RC == STATUS_SUCCESS) {
                KdPrint(("  packet ok @ %x\n", lba0+i));
                packet_ok = TRUE;
                i += Vcb->SparingBlockSize-1;
                continue;
            } else {
                need_remap = TRUE;
            }
        }

        if(!zero) {
            if(WCacheIsCached__(&(Vcb->FastCache), lba0+i, 1)) {
                // even if block is cached, we have to verify if it is readable
                if(!packet_ok && !UDFVIsStored(Vcb, lba0+i)) {

                    tmp_wb = (uint32)_Vcb;
                    RC = UDFTRead(_Vcb, p, Vcb->BlockSize, lba0+i, &tmp_wb,
                                  Flags | PH_FORGET_VERIFIED | PH_READ_VERIFY_CACHE | PH_TMP_BUFFER | PH_VCB_IN_RETLEN);
                    if(!OS_SUCCESS(RC)) {
                        KdPrint(("  Found BB @ %x\n", lba0+i));
                    }

                }
                RC = WCacheDirect__(&(Vcb->FastCache), _Vcb, lba0+i, FALSE, &cached_block, TRUE/* cached only */);
            } else {
                cached_block = NULL;
                if(!packet_ok) {
                    RC = STATUS_UNSUCCESSFUL;
                } else {
                    RC = STATUS_SUCCESS;
                }
            }
            if(OS_SUCCESS(RC)) {
                // cached or successfully read
                if(cached_block) {
                    // we can get from cache the most fresh data
                    RtlCopyMemory(p, cached_block, Vcb->BlockSize);
                }

            } else {
                if(!UDFVIsStored(Vcb, lba0+i)) {
                    tmp_wb = (uint32)_Vcb;
                    RC = UDFTRead(_Vcb, p, Vcb->BlockSize, lba0+i, &tmp_wb,
                                  Flags | PH_FORGET_VERIFIED | PH_READ_VERIFY_CACHE | PH_TMP_BUFFER | PH_VCB_IN_RETLEN);
                } else {
                    // get it from verify-cache
                    RC = STATUS_UNSUCCESSFUL;
                }
                if(!OS_SUCCESS(RC)) {
/*
                    KdPrint(("  retry @ %x\n", lba0+i));
                    tmp_wb = (uint32)_Vcb;
                    RC = UDFTRead(_Vcb, p, Vcb->BlockSize, lba0+i, &tmp_wb,
                                  Flags | PH_FORGET_VERIFIED | PH_READ_VERIFY_CACHE | PH_TMP_BUFFER | PH_VCB_IN_RETLEN);
*/
                    KdPrint(("  try get from verify cache @ %x\n", lba0+i));
                    RC = UDFVRead(Vcb, p, 1, UDFRelocateSector(Vcb, lba0+i),
                                  Flags | PH_FORGET_VERIFIED | PH_READ_VERIFY_CACHE | PH_TMP_BUFFER);
                    need_remap = TRUE;
                }
            }
        } else {
            RtlZeroMemory(p, Vcb->BlockSize);
        }
        if(!packet_ok) {
            KdPrint(("  try del from verify cache @ %x\n", lba0+i));
            RC = UDFVForget(Vcb, 1, UDFRelocateSector(Vcb, lba0+i), 0);
        }

        if(!packet_ok || need_remap) {
            KdPrint(("  block in bad packet @ %x\n", lba0+i));
            if(Vcb->BSBM_Bitmap) {
                UDFSetBit(Vcb->BSBM_Bitmap, lba0+i);
            }
            if(Vcb->FSBM_Bitmap) {
                UDFSetUsedBit(Vcb->FSBM_Bitmap, lba0+i);
            }
        }

        j++;
        if(j >= Vcb->SparingBlockSize) {
            // remap this packet
            if(need_remap) {
                ASSERT(!packet_ok);
                if(!non_zero) {
                    KdPrint(("  forget Z packet @ %x\n", lba1));
                    UDFUnmapRange(Vcb, lba1, Vcb->SparingBlockSize);
                    RC = STATUS_SUCCESS;
                } else {
do_remap:
                    for(j=0; j<3; j++) {
                        KdPrint(("  remap packet @ %x\n", lba1));
                        RC = UDFRemapPacket(Vcb, lba1, FALSE);
                        if(!OS_SUCCESS(RC)) {
                            if(RC == STATUS_SHARING_VIOLATION) {
                                KdPrint(("  remap2\n"));
                                // remapped location have died
                                RC = UDFRemapPacket(Vcb, lba1, TRUE);
                            }
                            if(!OS_SUCCESS(RC)) {
                                // packet cannot be remapped :(
                                RC = STATUS_DEVICE_DATA_ERROR;
                            }
                        }
                        KdPrint(("  remap status %x\n", RC));
                        if(OS_SUCCESS(RC)) {
                            // write to remapped area
                            tmp_wb = (uint32)_Vcb;
                            RC = UDFTWrite(_Vcb, tmp_buff, Vcb->SparingBlockSize << Vcb->BlockSizeBits, lba1, &tmp_wb,
                                          Flags | PH_FORGET_VERIFIED | PH_READ_VERIFY_CACHE | PH_TMP_BUFFER | PH_VCB_IN_RETLEN);
                            KdPrint(("  write status %x\n", RC));
                            if(RC != STATUS_SUCCESS) {
                                // will be remapped
                                KdPrint(("  retry remap\n"));

                                // Note: when remap of already remapped block is requested, verify of
                                // entire sparing are will be performed.

                            } else {
                                KdPrint(("  remap OK\n"));
                                break;
                            }
                        } else {
                            KdPrint(("  failed remap\n"));
                            break;
                        }
                    } // for
                }
                if(!OS_SUCCESS(RC) && !OS_SUCCESS(final_RC)) {
                    final_RC = RC;
                }
            } else {
                KdPrint(("  NO remap for @ %x\n", (lba0+i) & ~mask));
            }
            j=0;
        }
    }
    if(free_tmp) {
        DbgFreePool(tmp_buff);
    }

    tmp_wb = (uint32)_Vcb;
    if(Flags & PH_EX_WRITE) {
        KdPrint(("IO-Write-Verify (2)\n"));
        //RC = UDFTWrite(_Vcb, Buffer, Length, LBA, &tmp_wb, Flags | PH_FORGET_VERIFIED | PH_VCB_IN_RETLEN);
    } else {
        KdPrint(("IO-Read-Verify (2)\n"));
        RC = UDFTRead(_Vcb, Buffer, Length, LBA, &tmp_wb, Flags | PH_FORGET_VERIFIED | PH_VCB_IN_RETLEN);
    }
    (*IOBytes) = tmp_wb;
    KdPrint(("Final %x\n", RC));

    UDFReleaseResource(&(Vcb->IoResource));
    if(Flags & PH_LOCK_CACHE) {
        WCacheEODirect__(&(Vcb->FastCache), Vcb);
    }

    return RC;
} // end UDFTIOVerify()

OSSTATUS
UDFTWriteVerify(
    IN void* _Vcb,
    IN void* Buffer,     // Target buffer
    IN uint32 Length,
    IN uint32 LBA,
    OUT uint32* WrittenBytes,
    IN uint32 Flags
    )
{
    return UDFTIOVerify(_Vcb, Buffer, Length, LBA, WrittenBytes, Flags | PH_VCB_IN_RETLEN | PH_EX_WRITE | PH_KEEP_VERIFY_CACHE);
} // end UDFTWriteVerify()

OSSTATUS
UDFTReadVerify(
    IN void* _Vcb,
    IN void* Buffer,     // Target buffer
    IN uint32 Length,
    IN uint32 LBA,
    OUT uint32* ReadBytes,
    IN uint32 Flags
    )
{
    return UDFTIOVerify(_Vcb, Buffer, Length, LBA, ReadBytes, Flags | PH_VCB_IN_RETLEN | PH_KEEP_VERIFY_CACHE);
} // end UDFTReadVerify()
#endif //_BROWSE_UDF_

/*
    This routine performs low-level write

    ATTENTION! When we are in Variable-Packet mode (CDR_Mode = TRUE)
    LBA is ignored and assumed to be equal to NWA by CD-R(W) driver
 */
OSSTATUS
UDFTWrite(
    IN void* _Vcb,
    IN void* Buffer,     // Target buffer
    IN uint32 Length,
    IN uint32 LBA,
    OUT uint32* WrittenBytes,
    IN uint32 Flags
    )
{
#ifndef UDF_READ_ONLY_BUILD
#define Vcb ((PVCB)_Vcb)

#ifdef _BROWSE_UDF_
    PEXTENT_MAP RelocExtent;
    PEXTENT_MAP RelocExtent_saved = NULL;
#endif //_BROWSE_UDF_
    uint32 retry;
    BOOLEAN res_acq = FALSE;

    OSSTATUS RC = STATUS_SUCCESS;
    uint32 rLba;
    uint32 BCount;
    uint32 i;

#ifdef DBG
    //ASSERT(!(LBA & (32-1)));
#endif //DBG

    (*WrittenBytes) = 0;
    BCount = Length>>Vcb->BlockSizeBits;

    KdPrint(("TWrite %x (%x)\n", LBA, BCount));
#ifdef _BROWSE_UDF_
    if(Vcb->VCBFlags & UDF_VCB_FLAGS_DEAD) {
        KdPrint(("DEAD\n"));
        return STATUS_NO_SUCH_DEVICE;
    }

    Vcb->VCBFlags |= (UDF_VCB_SKIP_EJECT_CHECK | UDF_VCB_LAST_WRITE);
    if(!Vcb->CDR_Mode) {
        RelocExtent = UDFRelocateSectors(Vcb, LBA, BCount);
        if(!RelocExtent) {
            KdPrint(("can't relocate\n"));
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        rLba = LBA;
    } else {
        RelocExtent = UDF_NO_EXTENT_MAP;
        rLba = Vcb->NWA;
    }
#else //_BROWSE_UDF_
    rLba = LBA;
#endif //_BROWSE_UDF_

#ifdef DBG
    //ASSERT(!(rLba & (32-1)));
#endif //DBG

    _SEH2_TRY {
#ifdef _BROWSE_UDF_

        if(!(Flags & PH_IO_LOCKED)) {
            UDFAcquireResourceExclusive(&(Vcb->IoResource), TRUE);
            res_acq = TRUE;
        }

        if(RelocExtent == UDF_NO_EXTENT_MAP) {
#endif //_BROWSE_UDF_
            retry = UDF_WRITE_MAX_RETRY;
retry_1:
            RC = UDFPrepareForWriteOperation(Vcb, rLba, BCount);
            if(!OS_SUCCESS(RC)) {
                KdPrint(("prepare failed\n"));
                try_return(RC);
            }
            if(Flags & PH_VCB_IN_RETLEN) {
                (*WrittenBytes) = (ULONG)Vcb;
            }
            RC = UDFPhWriteVerifySynchronous(Vcb->TargetDeviceObject, Buffer, Length,
                       ((uint64)rLba) << Vcb->BlockSizeBits, WrittenBytes, Flags);
#ifdef _BROWSE_UDF_
            Vcb->VCBFlags |= UDF_VCB_SKIP_EJECT_CHECK;
#endif //_BROWSE_UDF_
            if(!OS_SUCCESS(RC) &&
                OS_SUCCESS(RC = UDFRecoverFromError(Vcb, TRUE, RC, rLba, BCount, &retry)) )
                goto retry_1;
            UDFUpdateNWA((PVCB)_Vcb, rLba, BCount, RC);
            try_return(RC);
#ifdef _BROWSE_UDF_
        }
        // write according to relocation table
        RelocExtent_saved = RelocExtent;
        for(i=0; RelocExtent->extLength; i++, RelocExtent++) {
            uint32 _WrittenBytes;
            rLba = RelocExtent->extLocation;
            BCount = RelocExtent->extLength>>Vcb->BlockSizeBits;
            retry = UDF_WRITE_MAX_RETRY;
retry_2:
            RC = UDFPrepareForWriteOperation(Vcb, rLba, BCount);
            if(!OS_SUCCESS(RC)) {
                KdPrint(("prepare failed (2)\n"));
                break;
            }
            if(Flags & PH_VCB_IN_RETLEN) {
                _WrittenBytes = (ULONG)Vcb;
            }
            RC = UDFPhWriteVerifySynchronous(Vcb->TargetDeviceObject, Buffer, RelocExtent->extLength,
                       ((uint64)rLba) << Vcb->BlockSizeBits, &_WrittenBytes, Flags);
            Vcb->VCBFlags |= UDF_VCB_SKIP_EJECT_CHECK;
            if(!OS_SUCCESS(RC) &&
                OS_SUCCESS(RC = UDFRecoverFromError(Vcb, TRUE, RC, rLba, BCount, &retry)) )
                goto retry_2;
            UDFUpdateNWA((PVCB)_Vcb, rLba, BCount, RC);
            LBA += BCount;
            (*WrittenBytes) += _WrittenBytes;
            if(!OS_SUCCESS(RC)) break;
            *((uint32*)&Buffer) += RelocExtent->extLength;
        }
#endif //_BROWSE_UDF_
try_exit: NOTHING;
    } _SEH2_FINALLY {
        if(res_acq) {
            UDFReleaseResource(&(Vcb->IoResource));
        }
#ifdef _BROWSE_UDF_
        if(RelocExtent_saved) {
            MyFreePool__(RelocExtent_saved);
        }
#endif //_BROWSE_UDF_
    } _SEH2_END;
    KdPrint(("TWrite: %x\n", RC));
    return RC;

#undef Vcb
#else //UDF_READ_ONLY_BUILD
    return STATUS_ACCESS_DENIED;
#endif //UDF_READ_ONLY_BUILD
} // end UDFTWrite()

/*
    This routine performs low-level read
 */
OSSTATUS
UDFTRead(
    IN void* _Vcb,
    IN void* Buffer,     // Target buffer
    IN uint32 Length,
    IN uint32 LBA,
    OUT uint32* ReadBytes,
    IN uint32 Flags
    ) 
{
    uint32 rLba;
    OSSTATUS RC = STATUS_SUCCESS;
    uint32 retry;
    PVCB Vcb = (PVCB)_Vcb;
    uint32 BCount = Length >> Vcb->BlockSizeBits;
    uint32 i;
#ifdef _BROWSE_UDF_
    PEXTENT_MAP RelocExtent;
    PEXTENT_MAP RelocExtent_saved = NULL;
    BOOLEAN res_acq = FALSE;
//    LARGE_INTEGER delay;
    Vcb->VCBFlags |= UDF_VCB_SKIP_EJECT_CHECK;

    ASSERT(Buffer);

    (*ReadBytes) = 0;

    if(Vcb->VCBFlags & UDF_VCB_FLAGS_DEAD)
        return STATUS_NO_SUCH_DEVICE;

    RelocExtent = UDFRelocateSectors(Vcb, LBA, BCount);
    if(!RelocExtent) return STATUS_INSUFFICIENT_RESOURCES;

    _SEH2_TRY {

        if(!(Flags & PH_IO_LOCKED)) {
            UDFAcquireResourceExclusive(&(Vcb->IoResource), TRUE);
            res_acq = TRUE;
        }

        if(RelocExtent == UDF_NO_EXTENT_MAP) {
            rLba = LBA;
            if(rLba >= (Vcb->CDR_Mode ? Vcb->NWA : Vcb->LastLBA + 1)) {
                RtlZeroMemory(Buffer, Length);
                try_return(RC = STATUS_SUCCESS);
            }
            retry = UDF_WRITE_MAX_RETRY;
retry_1:
            RC = UDFPrepareForReadOperation(Vcb, rLba, Length >> Vcb->BlockSizeBits);
            if(!OS_SUCCESS(RC)) try_return(RC);
            rLba = UDFFixFPAddress(Vcb, rLba);
#else
            rLba = LBA;
            retry = UDF_WRITE_MAX_RETRY;
retry_1:
            RC = UDFPrepareForReadOperation(Vcb, rLba, Length >> Vcb->BlockSizeBits);
            if(!OS_SUCCESS(RC)) return RC; // this is for !_BROWSE_UDF only
#endif //_BROWSE_UDF_
            if(Flags & PH_VCB_IN_RETLEN) {
                (*ReadBytes) = (ULONG)Vcb;
            }
            RC = UDFPhReadSynchronous(Vcb->TargetDeviceObject, Buffer, Length,
                       ((uint64)rLba) << Vcb->BlockSizeBits, ReadBytes, Flags);
            Vcb->VCBFlags &= ~UDF_VCB_LAST_WRITE;
#ifdef _BROWSE_UDF_
            Vcb->VCBFlags |= UDF_VCB_SKIP_EJECT_CHECK;
#endif //_BROWSE_UDF_
            if(!OS_SUCCESS(RC) &&
                OS_SUCCESS(RC = UDFRecoverFromError(Vcb, FALSE, RC, rLba, BCount, &retry)) ) {
                if(RC != STATUS_BUFFER_ALL_ZEROS) {
                    goto retry_1;
                }
                RtlZeroMemory(Buffer, Length);
                (*ReadBytes) = Length;
                RC = STATUS_SUCCESS;
            }
#ifdef _BROWSE_UDF_
            try_return(RC);
        }
        // read according to relocation table
        RelocExtent_saved = RelocExtent;
        for(i=0; RelocExtent->extLength; i++, RelocExtent++) {
            uint32 _ReadBytes;
            rLba = RelocExtent->extLocation;
            if(rLba >= (Vcb->CDR_Mode ? Vcb->NWA : Vcb->LastLBA + 1)) {
                RtlZeroMemory(Buffer, _ReadBytes = RelocExtent->extLength);
                RC = STATUS_SUCCESS;
                goto TR_continue;
            }
            BCount = RelocExtent->extLength>>Vcb->BlockSizeBits;
            retry = UDF_WRITE_MAX_RETRY;
retry_2:
            RC = UDFPrepareForReadOperation(Vcb, rLba, RelocExtent->extLength >> Vcb->BlockSizeBits);
            if(!OS_SUCCESS(RC)) break;
            rLba = UDFFixFPAddress(Vcb, rLba);
            if(Flags & PH_VCB_IN_RETLEN) {
                _ReadBytes = (ULONG)Vcb;
            }
            RC = UDFPhReadSynchronous(Vcb->TargetDeviceObject, Buffer, RelocExtent->extLength,
                       ((uint64)rLba) << Vcb->BlockSizeBits, &_ReadBytes, Flags);
            Vcb->VCBFlags &= ~UDF_VCB_LAST_WRITE;
            Vcb->VCBFlags |= UDF_VCB_SKIP_EJECT_CHECK;
            if(!OS_SUCCESS(RC) &&
                OS_SUCCESS(RC = UDFRecoverFromError(Vcb, FALSE, RC, rLba, BCount, &retry)) ) {
                if(RC != STATUS_BUFFER_ALL_ZEROS) {
                    goto retry_2;
                }
                RtlZeroMemory(Buffer, RelocExtent->extLength);
                _ReadBytes = RelocExtent->extLength;
                RC = STATUS_SUCCESS;
            }
TR_continue:
            (*ReadBytes) += _ReadBytes;
            if(!OS_SUCCESS(RC)) break;
            *((uint32*)&Buffer) += RelocExtent->extLength;
        }
try_exit: NOTHING;
    } _SEH2_FINALLY {
        if(res_acq) {
            UDFReleaseResource(&(Vcb->IoResource));
        }
        if(RelocExtent_saved) {
            MyFreePool__(RelocExtent_saved);
        }
    } _SEH2_END;
#endif //_BROWSE_UDF_
    return RC;
} // end UDFTRead()

#ifdef UDF_ASYNC_IO
/*
    This routine performs asynchronous low-level read
    Is not used now.
 */
OSSTATUS
UDFTReadAsync(
    IN void* _Vcb,
    IN void* _WContext,
    IN void* Buffer,     // Target buffer
    IN uint32 Length,
    IN uint32 LBA,
    OUT uint32* ReadBytes
    ) 
{
    PEXTENT_MAP RelocExtent;
    PEXTENT_MAP RelocExtent_saved;
    OSSTATUS RC = STATUS_SUCCESS;
//    LARGE_INTEGER delay;
    uint32 retry = UDF_READ_MAX_RETRY;
    PVCB Vcb = (PVCB)_Vcb;
    Vcb->VCBFlags |= UDF_VCB_SKIP_EJECT_CHECK;
    uint32 rLba;
    uint32 BCount;

    ASSERT(Buffer);

    (*ReadBytes) = 0;

    RelocExtent = UDFRelocateSectors(Vcb, LBA, BCount = Length >> Vcb->BlockSizeBits);
    if(!RelocExtent) return STATUS_INSUFFICIENT_RESOURCES;
    if(RelocExtent == UDF_NO_EXTENT_MAP) {
        rLba = LBA;
        if(rLba >= (Vcb->CDR_Mode ? Vcb->NWA : Vcb->LastLBA + 1)) {
            RtlZeroMemory(Buffer, Length);
            return STATUS_SUCCESS;
        }
retry_1:
        RC = UDFPrepareForReadOperation(Vcb, rLba, BCount);
        if(!OS_SUCCESS(RC)) return RC;
        rLba = UDFFixFPAddress(Vcb, rLba);
        RC = UDFPhReadSynchronous(Vcb->TargetDeviceObject, Buffer, Length,
                   ((uint64)rLba) << Vcb->BlockSizeBits, ReadBytes, 0);
        Vcb->VCBFlags &= ~UDF_VCB_LAST_WRITE;
        Vcb->VCBFlags |= UDF_VCB_SKIP_EJECT_CHECK;
        if(!OS_SUCCESS(RC) &&
            OS_SUCCESS(RC = UDFRecoverFromError(Vcb, FALSE, RC, rLba, BCount, &retry)) )
            goto retry_1;
        return RC;
    }
    // read according to relocation table
    RelocExtent_saved = RelocExtent;
    for(uint32 i=0; RelocExtent->extLength; i++, RelocExtent++) {
        uint32 _ReadBytes;
        rLba = RelocExtent->extLocation;
        if(rLba >= (Vcb->CDR_Mode ? Vcb->NWA : Vcb->LastLBA + 1)) {
            RtlZeroMemory(Buffer, _ReadBytes = RelocExtent->extLength);
            RC = STATUS_SUCCESS;
            goto TR_continue;
        }
        BCount = RelocExtent->extLength>>Vcb->BlockSizeBits;
retry_2:
        RC = UDFPrepareForReadOperation(Vcb, rLba, RelocExtent->extLength >> Vcb->BlockSizeBits);
        if(!OS_SUCCESS(RC)) break;
        rLba = UDFFixFPAddress(Vcb, rLba);
        RC = UDFPhReadSynchronous(Vcb->TargetDeviceObject, Buffer, RelocExtent->extLength,
                   ((uint64)rLba) << Vcb->BlockSizeBits, &_ReadBytes, 0);
        Vcb->VCBFlags &= ~UDF_VCB_LAST_WRITE;
        Vcb->VCBFlags |= UDF_VCB_SKIP_EJECT_CHECK;
        if(!OS_SUCCESS(RC) &&
            OS_SUCCESS(RC = UDFRecoverFromError(Vcb, FALSE, RC, rLba, BCount, &retry)) )
            goto retry_2;
TR_continue:
        (*ReadBytes) += _ReadBytes;
        if(!OS_SUCCESS(RC)) break;
        *((uint32*)&Buffer) += RelocExtent->extLength;
    }
    MyFreePool__(RelocExtent_saved);
    return RC;
} // end UDFTReadAsync()

#endif //UDF_ASYNC_IO

/*

 */
NTSTATUS
UDFSetMRWMode(
    IN PVCB Vcb
    )
{
    GET_MRW_MODE_USER_OUT MRWPage;
    OSSTATUS RC;

    if(Vcb->MediaClassEx != CdMediaClass_CDRW)
        return STATUS_SUCCESS;
//#ifdef _BROWSE_UDF_
    if(Vcb->CompatFlags & UDF_VCB_IC_MRW_ADDR_PROBLEM)
        return STATUS_SUCCESS;
//#endif //_BROWSE_UDF_

    if(!Vcb->MRWStatus) {
        KdPrint(("Non-MRW disk. Skip setting MRW_MODE\n"));
        return STATUS_SUCCESS;
    }
    KdPrint(("try set MRW_MODE\n"));
    RC = UDFPhSendIOCTL(IOCTL_CDRW_GET_MRW_MODE, Vcb->TargetDeviceObject,
                    NULL,0,
                    (PVOID)&MRWPage,sizeof(MRWPage),
                    FALSE, NULL);
    if(!NT_SUCCESS(RC)) {
        return RC;
    }
    KdPrint(("GET_MRW_MODE ok (current %x)\n", MRWPage.AddressMode));
    MRWPage.AddressMode = Vcb->MRWStatus ? 0 : MrwPage_use_GAA;
    KdPrint(("SET_MRW_MODE %x\n", MRWPage.AddressMode));
    RC = UDFPhSendIOCTL(IOCTL_CDRW_SET_MRW_MODE, Vcb->TargetDeviceObject,
                    (PVOID)&MRWPage,sizeof(MRWPage),
                    NULL,0,
                    FALSE, NULL);
    KdPrint(("SET_MRW_MODE status %x\n", RC));

    return STATUS_SUCCESS;
} // end UDFSetMRWMode()

OSSTATUS
UDFDoOPC(
    IN PVCB Vcb
    )
{
    OSSTATUS RC;
    if(Vcb->OPCNum && !Vcb->OPCDone) {
        KdPrint(("UDFDoOPC\n"));
        if(!Vcb->OPCh) {
            Vcb->OPCh =
                (PSEND_OPC_INFO_HEADER_USER_IN)MyAllocatePool__(NonPagedPool,
                    sizeof(SEND_OPC_INFO_HEADER_USER_IN) );
        }
        if(!Vcb->OPCh)
            return STATUS_INSUFFICIENT_RESOURCES;
        Vcb->OPCh->DoOpc = TRUE;
        Vcb->OPCh->OpcBlocksNumber = 0;
        RC = UDFPhSendIOCTL(IOCTL_CDRW_SEND_OPC_INFO, Vcb->TargetDeviceObject,
                        (void*)(Vcb->OPCh),sizeof(SEND_OPC_INFO_HEADER_USER_IN),
                        NULL,0,
                        FALSE, NULL);
        if(!OS_SUCCESS(RC)) {
            KdPrint(("UDFDoOPC failed\n"));
            Vcb->OPCNum = 0;
//            Vcb->VCBFlags |= UDF_VCB_FLAGS_OPC_FAILED;
        }
        Vcb->OPCDone = TRUE;
    }
    return RC;
} // end UDFDoOPC()

/*
    This routine performs media-type dependent preparations
    for write operation.

    For CDR/RW it sets WriteParameters according to track parameters,
    in some cases issues SYNC_CACHE command.
    It can also send OPC info if requered.
    If write-requested block is located beyond last formatted LBA
    on incompletely formatted DVD media, this routine performs
    all neccessary formatting operations in order to satisfy
    subsequent write request.
 */
OSSTATUS
UDFPrepareForWriteOperation(
    IN PVCB Vcb,
    IN uint32 Lba,
    IN uint32 BCount
    )
{
#ifndef UDF_READ_ONLY_BUILD
#ifdef UDF_FORMAT_MEDIA
    PUDFFmtState            fms = Vcb->fms;
#else 
  #define fms FALSE
#endif //UDF_FORMAT_MEDIA

#ifdef _UDF_STRUCTURES_H_
    if(Vcb->BSBM_Bitmap) {
        ULONG i;
        for(i=0; i<BCount; i++) {
            if(UDFGetBit__((uint32*)(Vcb->BSBM_Bitmap), Lba+i)) {
                KdPrint(("W: Known BB @ %#x\n", Lba));
                //return STATUS_FT_WRITE_RECOVERY; // this shall not be treated as error and
                                                   // we shall get IO request to BAD block
                return STATUS_DEVICE_DATA_ERROR;
            }
        }
    }
#endif //_UDF_STRUCTURES_H_

    Vcb->VCBFlags |= UDF_VCB_LAST_WRITE;

    if( 
#ifdef _BROWSE_UDF_
       (((Vcb->FsDeviceType != FILE_DEVICE_CD_ROM_FILE_SYSTEM) ||
       !(Vcb->VCBFlags & UDF_VCB_FLAGS_VOLUME_MOUNTED) ||
        (Vcb->VCBFlags & UDF_VCB_FLAGS_RAW_DISK))
        && !fms
        ) ||
#endif //_BROWSE_UDF_
#ifdef UDF_FORMAT_MEDIA
        (fms && fms->SkipPrepareW) ||
#endif //UDF_FORMAT_MEDIA
       !(Vcb->VCBFlags & UDF_VCB_FLAGS_OUR_DEVICE_DRIVER)
       ) {
        KdPrint(("Skip prepare for Write @%x\n", Lba));
        return STATUS_SUCCESS;
    }

    // check if the device requires OPC before each write operation
    UDFDoOPC(Vcb);

    if(Vcb->SyncCacheState == SYNC_CACHE_RECOVERY_ATTEMPT) {
        Vcb->SyncCacheState = SYNC_CACHE_RECOVERY_RETRY;
    } else {
        Vcb->SyncCacheState = SYNC_CACHE_RECOVERY_NONE;
    }
    if(Vcb->LastModifiedTrack &&
       (Vcb->TrackMap[Vcb->LastModifiedTrack].FirstLba <= Lba) &&
       (Vcb->TrackMap[Vcb->LastModifiedTrack].LastLba >= Lba) &&
       !( (Vcb->MediaClassEx == CdMediaClass_DVDRW ||
           Vcb->MediaClassEx == CdMediaClass_DVDpRW ||
           Vcb->MediaClassEx == CdMediaClass_DVDRAM ||
           Vcb->MRWStatus == DiscInfo_BGF_Interrupted ||
           Vcb->MRWStatus == DiscInfo_BGF_InProgress) && (Lba > Vcb->LastLBA))
      ) {
        // Ok, we needn't change Write Parameters
//        if(Vcb->TrackMap[Vcb->LastModifiedTrack].Flags & TrackMap_Try_variation)
//            Vcb->TrackMap[Vcb->LastModifiedTrack].Flags |= TrackMap_Use_variation;
        KdPrint(("Skip prepare for Write (2) @%x\n", Lba));
        return STATUS_SUCCESS;
    }

    UDFSetMRWMode(Vcb);

    if(!UDFIsWriteParamsReq(Vcb)) {
#ifdef UDF_FORMAT_MEDIA
        if(fms) {
            return STATUS_SUCCESS;
        }
#endif //UDF_FORMAT_MEDIA
    }

    for(uint32 i=Vcb->FirstTrackNum; i<=Vcb->LastTrackNum; i++) {
        if((Vcb->TrackMap[i].FirstLba > Lba) ||
           (Vcb->TrackMap[i].LastLba < Lba)) {
            //KdPrint(("not in track %d\n"));
            continue;
        }
        OSSTATUS RC;
        PGET_WRITE_MODE_USER_OUT WParams;

        if(!UDFIsWriteParamsReq(Vcb)) {
            RC = STATUS_SUCCESS;
            goto check_dvd_bg_format;
        }

        if(!Vcb->WParams) {
            Vcb->WParams =
                (PGET_WRITE_MODE_USER_OUT)MyAllocatePool__(NonPagedPool, 512);
        }
        if(!(WParams = Vcb->WParams)) {
            KdPrint(("!WParams\n"));
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RC = UDFPhSendIOCTL(IOCTL_CDRW_GET_WRITE_MODE, Vcb->TargetDeviceObject,
                        NULL,0,
                        (void*)(Vcb->WParams),sizeof(GET_WRITE_MODE_USER_OUT),
                        FALSE, NULL);
        if(!OS_SUCCESS(RC)) {
#ifdef UDF_FORMAT_MEDIA
            if(fms) {
                fms->SkipPrepareW = 1;
                MyFreePool__(WParams);
                return STATUS_SUCCESS;
            }
#endif //UDF_FORMAT_MEDIA
            KdPrint(("!get WParams\n"));
            return RC;
        }
        // clear unnecassary flags
        WParams->Byte2.Flags &= ~WParam_TestWrite;
        WParams->Byte2.Flags &= ~WParam_WType_Mask;
        // select packet writing
        WParams->Byte2.Flags |= WParam_WType_Packet;

        WParams->Byte3.Flags &= ~(WParam_TrkMode_Mask |
                                  WParam_TrkMode_AllowCpy |
                                  WParam_Copy);
        WParams->Byte3.Flags |= Vcb->TrackMap[i].TrackParam &
                                 (WParam_TrkMode_Mask |
                                  WParam_TrkMode_AllowCpy |
                                  WParam_Copy);

        // set packet type (VP/FP)
//        if(opt_partition == PT_VAT15 ||
//           opt_blank_vat15) 
        if(WParams->Byte2.Flags & WParam_LS_V) {
            WParams->LinkSize = 7;
        }

        if(Vcb->TrackMap[i].DataParam & TrkInfo_Packet) {
            if((Vcb->TrackMap[i].DataParam & TrkInfo_FP) &&
                !Vcb->CDR_Mode) {
                WParams->Byte3.Flags |= WParam_FP;
            } else {
                WParams->Byte3.Flags &= ~WParam_FP;
            }
        } else {
            if(!Vcb->CDR_Mode) {
                WParams->Byte3.Flags |= WParam_FP;
            } else {
                WParams->Byte3.Flags &= ~WParam_FP;
            }
        }

        // select multisession mode
        WParams->Byte3.Flags &= ~WParam_MultiSes_Mask;
        if((Vcb->DiscStat & DiscInfo_Disk_Mask) == DiscInfo_Disk_Appendable) {
            WParams->Byte3.Flags |= WParam_Multises_Multi;
        } else
        if(Vcb->LastSession > 1) {
            WParams->Byte3.Flags |= WParam_Multises_Final;
        } else {
            WParams->Byte3.Flags |= WParam_Multises_None;
        }
        // set sector mode (Mode1/XA)
        WParams->Byte4.Flags &= ~WParam_BlkType_Mask;
        if((Vcb->TrackMap[i].DataParam & TrkInfo_Dat_Mask) == TrkInfo_Dat_XA) {
            // XA Mode2
            WParams->Byte4.Flags |= WParam_BlkType_M2XAF1_2048;
            WParams->SesFmt = WParam_SesFmt_CdRomXa;
        } else if((Vcb->TrackMap[i].DataParam & TrkInfo_Dat_Mask) == TrkInfo_Dat_Mode1) {
            // Mode1
            WParams->Byte4.Flags |= WParam_BlkType_M1_2048;
            WParams->SesFmt = WParam_SesFmt_CdRom;
        } else {
#ifdef UDF_FORMAT_MEDIA
            if(fms) {
                fms->SkipPrepareW = 1;
                MyFreePool__(WParams);
                return STATUS_SUCCESS;
            }
#endif //UDF_FORMAT_MEDIA
            KdPrint(("  inv sector mode\n"));
            return STATUS_INVALID_PARAMETER;
        }
        // set packet size
        *((uint32*)&(WParams->PacketSize)) = BCount;
        *((uint32*)&(WParams->SubHeader)) = 0;
        // set additional flags for VP

        if(Vcb->CDR_Mode) {
//        if(opt_partition == PT_VAT15) 
            WParams->SubHeader.Params.Params1.SubMode = WParam_SubHdr_SubMode1;
        }
        WParams->PageLength = sizeof(GET_WRITE_MODE_USER_OUT)-2;
        WParams->PageCode = MODE_PAGE_WRITE_PARAMS;
        // apply write parameters
        RC = UDFPhSendIOCTL(IOCTL_CDRW_SET_WRITE_MODE, Vcb->TargetDeviceObject,
                        (void*)WParams,sizeof(SET_WRITE_MODE_USER_IN),
                        NULL,0,FALSE,NULL);

#ifdef UDF_FORMAT_MEDIA
        if(fms) {
            if(!NT_SUCCESS(RC)) {
                fms->SkipPrepareW = 1;
                MyFreePool__(WParams);
                return STATUS_SUCCESS;
            }

            RC = UDFPhSendIOCTL(IOCTL_CDRW_GET_WRITE_MODE, Vcb->TargetDeviceObject,
                            NULL,0,
                            (PVOID)WParams,sizeof(GET_WRITE_MODE_USER_OUT),
                            FALSE, NULL);
            if(!NT_SUCCESS(RC)) {
                MyFreePool__(WParams);
                return RC;
            }

            if(fms->opt_partition == PT_VAT15 ||
               fms->opt_blank_vat15) {
                if(WParams->Byte3.Flags & WParam_FP) {
                    MyFreePool__(WParams);
                    return STATUS_INVALID_DEVICE_STATE;
                }
            } else {
                if(!(WParams->Byte3.Flags & WParam_FP)) {
                    MyFreePool__(WParams);
                    return STATUS_INVALID_DEVICE_STATE;
                }
            }
        }
#endif //UDF_FORMAT_MEDIA
        
        // switch to random access mode
        ((PSET_RANDOM_ACCESS_USER_IN)WParams)->RandomAccessMode = Vcb->CDR_Mode ? FALSE : TRUE;
//        ((PSET_RANDOM_ACCESS_USER_IN)WParams)->RandomAccessMode = (opt_partition != PT_VAT15) ? TRUE : FALSE;
        RC = UDFPhSendIOCTL(IOCTL_CDRW_SET_RANDOM_ACCESS, Vcb->TargetDeviceObject,
                        (void*)WParams,sizeof(SET_RANDOM_ACCESS_USER_IN),
                        NULL,0,FALSE, NULL);

check_dvd_bg_format:

        KdPrint(("  check BGF\n"));
        if(!Vcb->CDR_Mode) {
            if(OS_SUCCESS(RC)) {
                Vcb->LastModifiedTrack = i;
                if(!(Vcb->TrackMap[i].Flags & TrackMap_Use_variation)) {
                    if(Vcb->TrackMap[i].Flags & TrackMap_Try_variation) {
                        Vcb->TrackMap[i].Flags |= TrackMap_Use_variation;
                    } else {
                        Vcb->TrackMap[i].Flags |= TrackMap_Try_variation;
                    }
                }
            }
        } else {
            Vcb->LastModifiedTrack = 0;
        }
//        fms->SkipPrepareW = 1;


        if((Vcb->MediaClassEx == CdMediaClass_DVDRW ||
            Vcb->MediaClassEx == CdMediaClass_DVDpRW ||
            Vcb->MediaClassEx == CdMediaClass_DVDRAM ||
            Vcb->MRWStatus == DiscInfo_BGF_Interrupted )
                 && (Lba > Vcb->LastLBA)) {
   
            ULONG fLba;
            ULONG WrittenBytes;
            ULONG PSz = BCount << Vcb->BlockSizeBits;
#ifdef _BROWSE_UDF_
            ULONG retry;
#endif //_BROWSE_UDF_
            PFORMAT_CDRW_PARAMETERS_USER_IN ForBuf;

            ASSERT((Vcb->LastLBA+1) == Vcb->NWA);

            if(Lba+BCount <= (Vcb->LastLBA+1) ) {
                KdPrint(("DVD cont. fmt, LBA+BCount<=NWA, exiting\n"));
                return STATUS_SUCCESS;
            }
            if((Vcb->MRWStatus != DiscInfo_BGF_Interrupted) &&
               (Lba <= (Vcb->LastLBA+1)) ) {
                KdPrint(("!PausedBGF + DVD cont. fmt, LBA<=NWA, exiting\n"));
                return STATUS_SUCCESS;
            }

            if(Vcb->MRWStatus == DiscInfo_BGF_Interrupted) {
                // This code also can restart background MRW formatting
                KdPrint(("DVD cont. fmt, LastLBA %x, Lba %x\n", Vcb->LastLBA, Lba));

                ForBuf = (PFORMAT_CDRW_PARAMETERS_USER_IN)DbgAllocatePoolWithTag(NonPagedPool, sizeof(FORMAT_CDRW_PARAMETERS_USER_IN), 'zNWD');
                if(ForBuf) {
                    RtlZeroMemory(ForBuf, sizeof(FORMAT_CDRW_PARAMETERS_USER_IN));
                    ForBuf->Flags.FlagsEx = FORMAT_UNIT_RESTART_MRW;
                    ForBuf->BlockCount = 0xffffffff;

                    RC = UDFPhSendIOCTL(IOCTL_CDRW_FORMAT_UNIT, Vcb->TargetDeviceObject,
                            ForBuf,sizeof(FORMAT_CDRW_PARAMETERS_USER_IN),
                            NULL,0,FALSE, NULL);
                    DbgFreePool(ForBuf);
                    if(OS_SUCCESS(RC)) {
                        KdPrint(("BGFormat restarted Interrupted->InProgress\n"));
                        Vcb->MRWStatus = DiscInfo_BGF_InProgress;
                    } else {
                        PGET_LAST_ERROR_USER_OUT Error = NULL;
                        if(!Vcb->Error) {
                            Vcb->Error = (PGET_LAST_ERROR_USER_OUT)
                                            MyAllocatePool__(NonPagedPool, sizeof(GET_LAST_ERROR_USER_OUT));
                        }
                        Error = Vcb->Error;
                        if(Error) {
                            UDFPhSendIOCTL( IOCTL_CDRW_GET_LAST_ERROR, Vcb->TargetDeviceObject,
                                            NULL,0,
                                            Error,sizeof(GET_LAST_ERROR_USER_OUT),
                                            TRUE,NULL);
                            KdPrint(("SK=%x ASC=%x, ASCQ=%x, IE=%x\n",
                                     Error->SenseKey, Error->AdditionalSenseCode, Error->AdditionalSenseCodeQualifier, Error->LastError));
                            // check for Long Write In Progress
                            if( (Error->SenseKey == SCSI_SENSE_NOT_READY) &&
                                (Error->AdditionalSenseCode == SCSI_ADSENSE_LUN_NOT_READY) &&
                                 ((Error->AdditionalSenseCodeQualifier == SCSI_SENSEQ_LONG_WRITE_IN_PROGRESS) ||
                                  (Error->AdditionalSenseCodeQualifier == SCSI_SENSEQ_FORMAT_IN_PROGRESS)) ) {
                                RC = STATUS_SUCCESS;
                                KdPrint(("Seems, BGFormat already restarted\n"));
                                Vcb->MRWStatus = DiscInfo_BGF_InProgress;
                            }
                        }
                    }
                }
            } else {
                RC = STATUS_SUCCESS;
            }

            KdPrint(("DVD cont. write, LastLBA %x, Lba %x\n", Vcb->LastLBA, Lba));

            ASSERT(Vcb->MediaClassEx == CdMediaClass_DVDRW);
            if(!Vcb->fZBuffer) {
                Vcb->fZBuffer = (PCHAR)DbgAllocatePoolWithTag(NonPagedPool, PSz, 'zNWD');
                RtlZeroMemory(Vcb->fZBuffer, PSz);
                Vcb->fZBufferSize = PSz;
            } else
            if(Vcb->fZBufferSize < PSz) {
                PSz = Vcb->fZBufferSize;
            }
            if(!Vcb->fZBuffer) {
                BrutePoint();
                RC = STATUS_INSUFFICIENT_RESOURCES;
            } else {
                for(fLba = Vcb->NWA; fLba < Lba; fLba+=BCount) {
#ifdef _BROWSE_UDF_
                    retry = UDF_WRITE_MAX_RETRY;
retry_1:
#endif //_BROWSE_UDF_
                    RC = UDFPhWriteVerifySynchronous(Vcb->TargetDeviceObject, Vcb->fZBuffer, PSz,
                           ((uint64)fLba) << Vcb->BlockSizeBits, &WrittenBytes, PH_TMP_BUFFER);
                    Vcb->VCBFlags |= UDF_VCB_SKIP_EJECT_CHECK;
                    KdPrint(("Fmt status: %x\n", RC));
#ifdef _BROWSE_UDF_
                    if(!OS_SUCCESS(RC) &&
                        OS_SUCCESS(RC = UDFRecoverFromError(Vcb, TRUE, RC, fLba, BCount, &retry)) ) {
                        goto retry_1;
                        KdPrint(("Fmt retry\n"));
                    }
#endif //_BROWSE_UDF_
                    if(!OS_SUCCESS(RC)) {
                        BrutePoint();
                        KdPrint(("Fmt break on ERROR\n"));
                        break;
                    }
                    UDFUpdateNWA(Vcb, fLba, BCount, RC);
                }
            }
        } else {
            KdPrint(("  no special processing\n"));
        }
        
        return RC;
    }
#endif //UDF_READ_ONLY_BUILD
    KdPrint(("  no suitable track!\n"));
    return STATUS_INVALID_PARAMETER;
} // end UDFPrepareForWriteOperation()

//#ifdef _BROWSE_UDF_
/*
    This routine tries to recover from hardware error
    Return: STATUS_SUCCESS - retry requst
            STATUS_XXX - unrecoverable error
 */
OSSTATUS
UDFRecoverFromError(
    IN PVCB Vcb,
    IN BOOLEAN WriteOp,
    IN OSSTATUS status,
    IN uint32 Lba,
    IN uint32 BCount,
 IN OUT uint32* retry
    )
{
    PGET_LAST_ERROR_USER_OUT Error = NULL;
    LARGE_INTEGER delay;
    OSSTATUS RC;
    uint32 i;
    BOOLEAN UpdateBB = FALSE;

    if(!(*retry) ||
       !(Vcb->VCBFlags & UDF_VCB_FLAGS_OUR_DEVICE_DRIVER) ||
        (Vcb->FsDeviceType != FILE_DEVICE_CD_ROM_FILE_SYSTEM))
        return status;
    (*retry)--;
    // allocate tmp buffer
    _SEH2_TRY {
        if(!Vcb->Error) {
            if(!(Vcb->Error = (PGET_LAST_ERROR_USER_OUT)
                            MyAllocatePool__(NonPagedPool, sizeof(GET_LAST_ERROR_USER_OUT))))
                try_return(status);
        }
        if(status == STATUS_NO_SUCH_DEVICE) {
            KdPrint(("Error recovery: STATUS_NO_SUCH_DEVICE, die.....\n"));
            Vcb->VCBFlags |= UDF_VCB_FLAGS_UNSAFE_IOCTL | UDF_VCB_FLAGS_DEAD;
            try_return(status);
        }

#ifdef _UDF_STRUCTURES_H_
        if(status == STATUS_NO_MEDIA_IN_DEVICE && !Vcb->EjectWaiter) {
            KdPrint(("Error recovery: STATUS_NO_MEDIA_IN_DEVICE, prevent further remount.....\n"));
            // Make sure, that volume will never be quick-remounted
            // It is very important for ChkUdf utility and
            // some CD-recording libraries
            Vcb->SerialNumber--;
            try_return(status);
        }
#endif //_UDF_STRUCTURES_H_

        Error = Vcb->Error;
        UDFPhSendIOCTL( IOCTL_CDRW_GET_LAST_ERROR, Vcb->TargetDeviceObject,
                        NULL,0,
                        Error,sizeof(GET_LAST_ERROR_USER_OUT),
                        TRUE,NULL);
        KdPrint(("SK=%x ASC=%x, ASCQ=%x, IE=%x\n",
                 Error->SenseKey, Error->AdditionalSenseCode, Error->AdditionalSenseCodeQualifier, Error->LastError));
        // check for Long Write In Progress
        if( ((Error->SenseKey == SCSI_SENSE_NOT_READY) &&
             (Error->AdditionalSenseCode == SCSI_ADSENSE_LUN_NOT_READY) &&
             (Error->AdditionalSenseCodeQualifier == SCSI_SENSEQ_LONG_WRITE_IN_PROGRESS)) ) {
            // we should wait...
            if(WriteOp) {
                if((*retry) == UDF_WRITE_MAX_RETRY-1) {
                    KdPrint(("Error recovery: reserve retry count for write retries\n"));
                    (*retry) = UDF_WRITE_MAX_RETRY*3;
                } else
                if((*retry) == UDF_WRITE_MAX_RETRY) {
                    KdPrint(("Error recovery: jump over UDF_WRITE_MAX_RETRY\n"));
                    (*retry)--;
                }
                delay.QuadPart = -500000; // 0.05 sec
                KeDelayExecutionThread(KernelMode, FALSE, &delay);
                if(WriteOp && ((*retry) > UDF_WRITE_MAX_RETRY-1)) {
                    KdPrint(("Error recovery: simple write retry with delay\n"));
                    try_return(status = STATUS_SUCCESS);
                }
            } else {
                delay.QuadPart = -500000; // 0.05 sec
                KeDelayExecutionThread(KernelMode, FALSE, &delay);
                if((*retry) == UDF_WRITE_MAX_RETRY-1) {
                    KdPrint(("Error recovery: retry read after small delay\n"));
                    try_return(status = STATUS_SUCCESS);
                }
            }
            KdPrint(("Error recovery: sync cache\n"));
            // ...flush device cache...
            RC = UDFSyncCache(Vcb);
            // wait again & retry
            delay.QuadPart = -1000000; // 0.1 sec
            KeDelayExecutionThread(KernelMode, FALSE, &delay);
#ifdef _UDF_STRUCTURES_H_
            if(Vcb->BGWriters) (*retry)++;
#endif //_UDF_STRUCTURES_H_
            try_return(status = STATUS_SUCCESS);
        } else
        // check for Long Write In Progress
        if((Error->SenseKey == SCSI_SENSE_NOT_READY) &&
           (Error->AdditionalSenseCode == SCSI_ADSENSE_LUN_NOT_READY) &&
          ((Error->AdditionalSenseCodeQualifier == SCSI_SENSEQ_FORMAT_IN_PROGRESS) ||
           (Error->AdditionalSenseCodeQualifier == SCSI_SENSEQ_BECOMING_READY) ||
           (Error->AdditionalSenseCodeQualifier == SCSI_SENSEQ_OPERATION_IN_PROGRESS) ) ) {
            // we should wait & retry
            KdPrint(("Error recovery: op. in progress, waiting 0.3 sec\n"));
            delay.QuadPart = -3000000; // 0.3 sec
            KeDelayExecutionThread(KernelMode, FALSE, &delay);
#ifdef _UDF_STRUCTURES_H_
            if(Vcb->BGWriters) (*retry)++;
#endif //_UDF_STRUCTURES_H_
            Vcb->SyncCacheState = SYNC_CACHE_RECOVERY_ATTEMPT;
            try_return(status = STATUS_SUCCESS);
        } else
        // check for non empty cache special case
        if((Error->SenseKey == SCSI_SENSE_ILLEGAL_REQUEST) &&
           (Error->AdditionalSenseCode == SCSI_ADSENSE_INVALID_CMD_SEQUENCE)) {
            // we should wait & retry
            if(!WriteOp) {
                KdPrint(("Error recovery: invalid command sequence on read\n"));
                delay.QuadPart = -1000000; // 0.1 sec
                KeDelayExecutionThread(KernelMode, FALSE, &delay);
                KdPrint(("Error recovery: sync cache\n"));
                // ...flush device cache...
                RC = UDFSyncCache(Vcb);
                // wait again & retry
                delay.QuadPart = -1000000; // 0.1 sec
                KeDelayExecutionThread(KernelMode, FALSE, &delay);
#ifdef _UDF_STRUCTURES_H_
                if(Vcb->BGWriters) (*retry)++;
#endif //_UDF_STRUCTURES_H_
                try_return(status = STATUS_SUCCESS);
            }
            goto reinit_sector_mode;
        } else
        // check for Bus Reset (sometimes it happends...)
        if((Error->SenseKey == SCSI_SENSE_UNIT_ATTENTION) &&
           (Error->AdditionalSenseCode == SCSI_ADSENSE_BUS_RESET) ) {
            // we should wait
            KdPrint(("Error recovery: bus reset...\n"));
            Vcb->MediaChangeCount = Error->MediaChangeCount;
            delay.QuadPart = -1000000; // 0.1 sec
            KeDelayExecutionThread(KernelMode, FALSE, &delay);
            // reset driver
            UDFResetDeviceDriver(Vcb, Vcb->TargetDeviceObject, FALSE);
            delay.QuadPart = -1000000; // 0.1 sec
            KeDelayExecutionThread(KernelMode, FALSE, &delay);
            // lock it
/*            ((PPREVENT_MEDIA_REMOVAL_USER_IN)(Error))->PreventMediaRemoval = TRUE;
            UDFPhSendIOCTL( IOCTL_STORAGE_MEDIA_REMOVAL,
                                 Vcb->TargetDeviceObject,
                                 Error,sizeof(PREVENT_MEDIA_REMOVAL_USER_IN),
                                 NULL,0,
                                 FALSE,NULL);
            delay.QuadPart = -1000000; // 0.1 sec
            KeDelayExecutionThread(KernelMode, FALSE, &delay);*/

            // reinit write mode the following is performed inside UDFResetDeviceDriver()
            //Vcb->LastModifiedTrack = 0;
            //Vcb->OPCDone = FALSE;

reinit_sector_mode:
            // reinit sector mode
            Vcb->LastModifiedTrack = 0;
            UDFPrepareForWriteOperation(Vcb, Lba, BCount);
            try_return(status = STATUS_SUCCESS);
        } else
        // check for Illegal Sector Mode.
        // We can get this error 'cause of 2 reasons:
        // a) Bus reset occured. We should reinit
        // b) CopyProtection settings missmatch
        // c) preblems with DNA of firmware developer, some TEACs fall into such state
        //    after failed streaming read
        if((Error->SenseKey == SCSI_SENSE_ILLEGAL_REQUEST) &&
           (Error->AdditionalSenseCode == SCSI_ADSENSE_ILLEGAL_MODE_FOR_THIS_TRACK)) {
bad_rw_seek_recovery:
            if(WriteOp) {

                if((*retry) <= 1) {
                    // Variate CopyProtection...
                    for(i=Vcb->FirstTrackNum; i<=Vcb->LastTrackNum; i++) {
                        if((Vcb->TrackMap[i].FirstLba > Lba) ||
                           (Vcb->TrackMap[i].LastLba < Lba))
                            continue;
    /*                    if(Vcb->TrackMap[i].Flags & TrackMap_CopyBit_variated)
                            // Last chance....
                            goto reinit_sector_mode;*/

                        // check if we have successuflly completed WriteOp
                        // using Variation.
                        // We should not variate these bits again in this case.
                        if(Vcb->TrackMap[i].Flags & TrackMap_Use_variation)
                            break;
                        Vcb->TrackMap[i].Flags &= ~TrackMap_Try_variation;
    /*                    if((Vcb->TrackMap[i].Flags & TrackMap_Try_variation) &&
                           (Vcb->TrackMap[i].Flags & (TrackMap_AllowCopyBit_variated |
                                                      TrackMap_CopyBit_variated)))
                            break;*/
    /*                    if(Vcb->TrackMap[i].Flags & TrackMap_Use_variation)
                            break;*/
                        Vcb->TrackMap[i].Flags |= TrackMap_Try_variation;
                        // Try variation. 
                        if(!(Vcb->TrackMap[i].Flags ^= TrackMap_AllowCopyBit_variated))
                            Vcb->TrackMap[i].Flags ^= TrackMap_CopyBit_variated;
                        if(Vcb->TrackMap[i].Flags & (TrackMap_AllowCopyBit_variated |
                                                     TrackMap_CopyBit_variated) ) {
                            (*retry) = 1;
                        } else {
                            Vcb->TrackMap[i].Flags &= ~TrackMap_Try_variation;
                        }
                        // reinit sector mode
                        Vcb->LastModifiedTrack = 0;
                        UDFPrepareForWriteOperation(Vcb, Lba, BCount);
                        break;
                    }
                } else {
                    // Reinit...
//reinit_sector_mode:
                    // we should wait
                    delay.QuadPart = -1000000; // 0.1 sec
                    KeDelayExecutionThread(KernelMode, FALSE, &delay);
                    // reinit sector mode
                    goto reinit_sector_mode;
/*
                    Vcb->LastModifiedTrack = 0;
                    UDFPrepareForWriteOperation(Vcb, Lba, BCount);
                    try_return(status = STATUS_SUCCESS);
*/
                }
            } else
            if((Vcb->CompatFlags & UDF_VCB_IC_BAD_RW_SEEK) &&
               (Vcb->IncrementalSeekState != INCREMENTAL_SEEK_DONE)) {
                KdPrint(("Using incremental seek workaround...\n"));
                Vcb->IncrementalSeekState = INCREMENTAL_SEEK_WORKAROUND;
                try_return(status = STATUS_SUCCESS);
            } else {
                KdPrint(("Seems to be BB @ %x\n", Lba));
                UpdateBB = TRUE;
            }
        } else
        if((Error->SenseKey == SCSI_SENSE_ILLEGAL_REQUEST) &&
           (Error->AdditionalSenseCode == SCSI_ADSENSE_INVALID_SESSION_MODE)) {
            if(WriteOp &&
               (Vcb->SavedFeatures & CDRW_FEATURE_STREAMING) &&
               Lba+BCount <= Vcb->LastLBA+1) {
                KdPrint(("bad Session in streaming mode. Lba %x, try fix-up\n", Lba));
                // ...flush device cache...
                RC = UDFSyncCache(Vcb);
                // we should wait
                delay.QuadPart = -10000000; // 1 sec
                KeDelayExecutionThread(KernelMode, FALSE, &delay);
                try_return(status = STATUS_SUCCESS);
            }
        } else
        if((Error->LastError == CDRW_ERR_WRITE_IN_PROGRESS_BUSY) ||
           (status == STATUS_DEVICE_BUSY)) {
            delay.QuadPart = -5000000; // 0.5 sec
            KdPrint(("CDRW_ERR_WRITE_IN_PROGRESS_BUSY || STATUS_DEVICE_BUSY\n"));
            KeDelayExecutionThread(KernelMode, FALSE, &delay);
#ifdef _UDF_STRUCTURES_H_
            if(Vcb->BGWriters) (*retry)++;
#endif //_UDF_STRUCTURES_H_
            try_return(status = STATUS_SUCCESS);
        } else
        // some devices (SONY) return such a strange sequence....
        if( ((Error->SenseKey == SCSI_SENSE_ILLEGAL_REQUEST) &&
             (Error->AdditionalSenseCode == SCSI_ADSENSE_INVALID_CDB)) &&
              WriteOp) {
            // reinit write mode
            Vcb->LastModifiedTrack = 0;
            UDFPrepareForWriteOperation(Vcb, Lba, BCount);
            try_return(status = STATUS_SUCCESS);
        } else
        // No seek on Read... to morgue, I'm afraid
        if((Error->SenseKey == SCSI_SENSE_MEDIUM_ERROR) /*&&
           ((Error->AdditionalSenseCode == SCSI_ADSENSE_CD_READ_ERROR) ||
            (Error->AdditionalSenseCode == SCSI_ADSENSE_NO_SENSE) ||
            (Error->AdditionalSenseCode == SCSI_ADSENSE_FORMAT_CORRUPTED) ||
            (Error->AdditionalSenseCode == SCSI_ADSENSE_SEEK_ERROR))*/ &&
           !WriteOp) {
            if(Error->AdditionalSenseCode == SCSI_ADSENSE_SEEK_ERROR) {
                KdPrint(("Seek error\n"));
                if(Vcb->CompatFlags & UDF_VCB_IC_BAD_RW_SEEK) {
                    KdPrint(("try recovery\n"));
                    goto bad_rw_seek_recovery;
                }
                KdPrint(("map error to STATUS_NONEXISTENT_SECTOR\n"));
                status = STATUS_NONEXISTENT_SECTOR;
            }
            KdPrint(("Seems to be BB @ %x (read 2)\n", Lba));
            UpdateBB = TRUE;
        } else
        // handle invalid block address
        if( ((Error->SenseKey == SCSI_SENSE_ILLEGAL_REQUEST) &&
             (Error->AdditionalSenseCode == SCSI_ADSENSE_ILLEGAL_BLOCK)) ) {
            if(!WriteOp &&
               (Vcb->SavedFeatures & CDRW_FEATURE_STREAMING) &&
               Lba+BCount <= Vcb->LastLBA+1) {
                KdPrint(("bad LBA %x in streaming mode, try fix-up\n", Lba));
                // ...flush device cache...
                RC = UDFSyncCache(Vcb);
                try_return(status = STATUS_SUCCESS);
            }

            if((Lba+BCount >= Vcb->LastLBA) &&
               (Vcb->MRWStatus == DiscInfo_BGF_Interrupted)) {
                KdPrint(("stupid drive, cannot read beyond formatted area on DiscInfo_BGF_Interrupted\n"));
                UpdateBB = FALSE;
                try_return(status = STATUS_BUFFER_ALL_ZEROS);
            }
            // prevent Bad Block Bitmap modification
        }

try_exit: NOTHING;

    } _SEH2_FINALLY {
#ifdef UDF_DBG
        if(OS_SUCCESS(status)) {
            KdPrint(("Retry\n"));
        }
#endif //UDF_DBG
    } _SEH2_END;
    if(!OS_SUCCESS(status)) {
        if((Vcb->MountPhErrorCount != -1) &&
           (Vcb->MountPhErrorCount < 0x7fffffff)) {
            Vcb->MountPhErrorCount++;
        }
//#ifdef _UDF_STRUCTURES_H_
        if(UpdateBB && (BCount == 1)) {
            uint32* bm;
            if(!(bm = (uint32*)(Vcb->BSBM_Bitmap))) {
                bm = (uint32*)(Vcb->BSBM_Bitmap = (int8*)DbgAllocatePoolWithTag(NonPagedPool, (i = (Vcb->LastPossibleLBA+1+7)>>3), 'mNWD' ));
                if(bm) {
                    RtlZeroMemory(bm, i);
                } else {
                    KdPrint(("Can't alloc BSBM for %x blocks\n", Vcb->LastPossibleLBA));
                }
            }
            if(bm) {
                UDFSetBit__(bm, Lba);
                KdPrint(("Set BB @ %#x\n", Lba));
            }
#ifdef _BROWSE_UDF_
            bm = (uint32*)(Vcb->FSBM_Bitmap);
            if(bm) {
                UDFSetUsedBit(bm, Lba);
                KdPrint(("Set BB @ %#x as used\n", Lba));
            }
#endif //_BROWSE_UDF_
        }
//#endif //_UDF_STRUCTURES_H_
    }
    return status;
} // end UDFRecoverFromError()

//#endif //_BROWSE_UDF_
/*
    This routine attempts to read disk layout using ReadDisk/Track info cmd
 */
OSSTATUS
UDFReadDiscTrackInfo(
    PDEVICE_OBJECT    DeviceObject,      // the target device object
    PVCB              Vcb                // Volume Control Block for ^ DevObj
    )
{
    OSSTATUS                    RC = STATUS_SUCCESS;
    PDISC_INFO_BLOCK_USER_OUT   DiscInfo = (PDISC_INFO_BLOCK_USER_OUT)MyAllocatePool__(NonPagedPool,sizeof(DISC_INFO_BLOCK_USER_OUT) );
    PTRACK_INFO_BLOCK_USER_OUT  TrackInfoOut = (PTRACK_INFO_BLOCK_USER_OUT)MyAllocatePool__(NonPagedPool,sizeof(TRACK_INFO_BLOCK_USER_OUT) );
    PTRACK_INFO_BLOCK_USER_IN   TrackInfoIn = (PTRACK_INFO_BLOCK_USER_IN)TrackInfoOut;
    READ_CAPACITY_USER_OUT      CapacityBuffer;
    LONG                        TrackNumber;
    BOOLEAN                     NotFP = FALSE;
    BOOLEAN                     ForceFP = FALSE;
    BOOLEAN                     PacketTrack = FALSE;
    BOOLEAN                     MRWRetry = FALSE;
    BOOLEAN                     ReadCapacityOk = FALSE;
#ifdef UDF_FORMAT_MEDIA
    PUDFFmtState            fms = Vcb->fms;
#endif
    
    _SEH2_TRY {
        if(!DiscInfo || !TrackInfoOut)
            try_return(RC = STATUS_INSUFFICIENT_RESOURCES);

MRWRetry_label:

        RC = UDFPhSendIOCTL(IOCTL_CDRW_READ_DISC_INFO, DeviceObject,
                NULL, 0, 
                DiscInfo,sizeof(DISC_INFO_BLOCK_USER_OUT), TRUE, NULL);
        if(!OS_SUCCESS(RC)) {
            KdPrint(("ReadDiskInfo failed. Use default.\n"));
            if(Vcb->MediaClassEx == CdMediaClass_DVDRW ||
                Vcb->MediaClassEx == CdMediaClass_DVDpRW ||
                Vcb->MediaClassEx == CdMediaClass_DVDRAM) {
                Vcb->LastPossibleLBA = DEFAULT_LAST_LBA_DVD;
            } else
            if(Vcb->MediaClassEx == CdMediaClass_BDRE) {
                Vcb->LastPossibleLBA = DEFAULT_LAST_LBA_BD;
            } else {
                Vcb->LastPossibleLBA = DEFAULT_LAST_LBA_FP_CD;
            }
            try_return(RC);
        }
#ifdef UDF_FORMAT_MEDIA
        if(fms && fms->opt_disk_info) {
            UserPrint(("ReadDiskInfo OK\n"));
        }
#endif //UDF_FORMAT_MEDIA

        RC = UDFPhSendIOCTL(IOCTL_CDRW_READ_CAPACITY, DeviceObject,
                NULL, 0, 
                &CapacityBuffer,sizeof(READ_CAPACITY_USER_OUT), TRUE, NULL);
        if(!OS_SUCCESS(RC)) {
            KdPrint(("ReadCapacity failed.\n"));
            if(Vcb->MediaClassEx == CdMediaClass_DVDpRW) {
                Vcb->LastPossibleLBA = DEFAULT_LAST_LBA_DVD;
            }
        } else {
            KdPrint(("ReadCapacity ok.\n"));
            KdPrint(("Last possible LBA %#x.\n", CapacityBuffer.LogicalBlockAddress));
            if(!(CapacityBuffer.LogicalBlockAddress  & 0xc0000000) &&
                (CapacityBuffer.LogicalBlockAddress != 0x7fffffff)) {
                // good value from ReadCapacity
                KdPrint(("Update Last possible LBA %#x.\n", CapacityBuffer.LogicalBlockAddress));
                Vcb->LastPossibleLBA = CapacityBuffer.LogicalBlockAddress;
                ReadCapacityOk = TRUE;
#ifdef UDF_FORMAT_MEDIA
                if(fms && fms->opt_disk_info) {
                    UserPrint(("ReadCapacity OK\n"));
                }
#endif //UDF_FORMAT_MEDIA
            }
        }

#ifdef _CONSOLE
        Vcb->PhDeviceType = FILE_DEVICE_CD_ROM;
#endif //_CONSOLE
        Vcb->PhSerialNumber = *((uint32*)&(DiscInfo->DiskId));
        Vcb->PhErasable = DiscInfo->DiscStat.Flags & DiscInfo_Disk_Erasable;
        Vcb->PhDiskType = DiscInfo->DiskType;
        // save OPC info
        if(DiscInfo->OPCNum)
            Vcb->OPCNum = DiscInfo->OPCNum;
        KdPrint(("DiskInfo: SN %x, OPCn %x(%x), Stat %x, Flg: %x\n",
            Vcb->PhSerialNumber, Vcb->OPCNum, DiscInfo->OPCNum, DiscInfo->DiscStat.Flags, DiscInfo->Flags.Flags));
#ifdef UDF_FORMAT_MEDIA
        if(fms && fms->opt_disk_info) {
            UserPrint(("Media type: "));
            switch(Vcb->MediaClassEx) {
            case CdMediaClass_CDROM     : UserPrint(("CD-ROM     \n")); break;
            case CdMediaClass_CDR       : UserPrint(("CD-R       \n")); break;
            case CdMediaClass_CDRW      : UserPrint(("CD-RW      \n")); break;
            case CdMediaClass_DVDROM    : UserPrint(("DVD-ROM    \n")); break;
            case CdMediaClass_DVDRAM    : UserPrint(("DVD-RAM    \n")); break;
            case CdMediaClass_DVDR      : UserPrint(("DVD-R      \n")); break;
            case CdMediaClass_DVDRW     : UserPrint(("DVD-RW     \n")); break;
            case CdMediaClass_DVDpR     : UserPrint(("DVD+R      \n")); break;
            case CdMediaClass_DVDpRW    : UserPrint(("DVD+RW     \n")); break;
            case CdMediaClass_DDCDROM   : UserPrint(("DDCD-ROM   \n")); break;
            case CdMediaClass_DDCDR     : UserPrint(("DDCD-R     \n")); break;
            case CdMediaClass_DDCDRW    : UserPrint(("DDCD-RW    \n")); break;
            case CdMediaClass_BDROM     : UserPrint(("BD-ROM     \n")); break;
            case CdMediaClass_BDRE      : UserPrint(("BD-RE      \n")); break;
            case CdMediaClass_BDR       : UserPrint(("BD-R       \n")); break;
            case CdMediaClass_HD_DVDROM : UserPrint(("HD DVD-ROM \n")); break;
            case CdMediaClass_HD_DVDRAM : UserPrint(("HD DVD-RAM \n")); break;
            case CdMediaClass_HD_DVDR   : UserPrint(("HD DVD-R   \n")); break;
            case CdMediaClass_HD_DVDRW  : UserPrint(("HD DVD-RW  \n")); break;
            default: UserPrint(("Unknown\n")); break;
            }
            UserPrint(("SN %#x, OPCn %#x\n",
                Vcb->PhSerialNumber, Vcb->OPCNum, DiscInfo->OPCNum));
            UserPrint(("Disk State: "));
            switch(DiscInfo->DiscStat.Flags & DiscInfo_Disk_Mask) {
            case DiscInfo_Disk_Empty:
                UserPrint(("Empty\n"));
                break;
            case DiscInfo_Disk_Appendable:
                UserPrint(("Appendable\n"));
                break;
            case DiscInfo_Disk_Complete:
                UserPrint(("Complete\n"));
                break;
            case DiscInfo_Disk_OtherRW:
                UserPrint(("RW in unknown state\n"));
                break;
            }
            UserPrint(("Last Session State: "));
            switch(DiscInfo->DiscStat.Flags & DiscInfo_Ses_Mask) {
            case DiscInfo_Ses_Empty:
                UserPrint(("Empty\n"));
                break;
            case DiscInfo_Ses_Incomplete:
                UserPrint(("Incomplete\n"));
                break;
            case DiscInfo_Ses_Complete:
                UserPrint(("Complete\n"));
                break;
            default:
                UserPrint(("unknown state\n"));
                break;
            }
            UserPrint(("Erasable: %s\n",
                (DiscInfo->DiscStat.Flags & DiscInfo_Disk_Erasable) ? "yes" : "no"
                ));
        }
#endif //UDF_FORMAT_MEDIA
        // Save disk status
        Vcb->DiscStat = DiscInfo->DiscStat.Flags;
        if((DiscInfo->DiscStat.Flags & DiscInfo_Disk_Mask) == DiscInfo_Disk_Empty) {
            KdPrint(("Blank\n"));
            Vcb->BlankCD = TRUE;
        }
        if( (DiscInfo->DiscStat.Flags & DiscInfo_Disk_Mask) == DiscInfo_Disk_Empty ||
            (DiscInfo->DiscStat.Flags & DiscInfo_Ses_Mask) == DiscInfo_Ses_Incomplete) {
            // we shall mount empty disk to make it possible for
            // external applications to perform format operation
            // or something like this
            KdPrint(("Try RAW_MOUNT\n"));
            Vcb->VCBFlags |= UDF_VCB_FLAGS_RAW_DISK;
            PacketTrack = TRUE;
        }

#ifndef _BROWSE_UDF_
        // If drive returned reasonable value from ReadCapacity, do not use
        // last LeadIn/LeadOut
        if(Vcb->MediaClassEx != CdMediaClass_DVDpRW &&
           !ReadCapacityOk) {
            // +RW returns bad value
            KdPrint(("+RW returns bad value\n"));
            Vcb->LastPossibleLBA = (DiscInfo->LastSesLeadOutLBA & 0x80000000) ?
                0 : DiscInfo->LastSesLeadOutLBA;
            if(!(DiscInfo->LastSesLeadInLBA & 0x80000000)) {
                Vcb->LastPossibleLBA = max(DiscInfo->LastSesLeadInLBA, Vcb->LastPossibleLBA);
            }
        }
#endif // _BROWSE_UDF_
        if((DiscInfo->Flags.Flags & DiscInfo_BGF_Mask) != 0) {
            KdPrint(("ForceFP + MRW\n"));
            ForceFP = TRUE;
            Vcb->MRWStatus = DiscInfo->Flags.Flags & DiscInfo_BGF_Mask;
            // update addressing mode
            if(!MRWRetry) {
                UDFSetMRWMode(Vcb);
                MRWRetry = TRUE;
                goto MRWRetry_label;
            }
        }
        KdPrint(("MRW state %x\n", Vcb->MRWStatus));
        if(Vcb->MediaClassEx == CdMediaClass_DVDRW) {
            if(Vcb->PhMediaCapFlags & CdCapFlags_RandomWritable) {
                KdPrint(("DVD-RW Rewritable\n"));
                ForceFP = TRUE;
            } else
            if((DiscInfo->DiscStat.Flags & DiscInfo_Disk_Mask) == DiscInfo_Disk_Empty) {
                KdPrint(("Blank DVD-RW\n"));
                ForceFP = TRUE;
            } else {
                KdPrint(("DVD-RW Sequential\n"));
                NotFP = TRUE;
            }
        } else
        if(CdrwIsDvdOverwritable(Vcb->MediaClassEx)) {
            KdPrint(("force Rewritable (2)\n"));
            ForceFP = TRUE;
        }
        // We have incomplete last session, so process each track from last to first
//            Vcb->LastPossibleLBA = DiscInfo->LastSesLeadInLBA;

        Vcb->LastSession   = DiscInfo->Status.NumOfSes;
        Vcb->LastTrackNum  = DiscInfo->Status.LastTrackNumLastSes;
        Vcb->FirstTrackNum = DiscInfo->FirstTrackNum;             
        // some devices report LastTrackNum=0 for full disks
        Vcb->LastTrackNum = max(Vcb->LastTrackNum, Vcb->FirstTrackNum);
        if(!Vcb->LastTrackNum) {
            KdPrint(("Try read 1st track...\n"));
            Vcb->LastTrackNum = 1;
        }
        KdPrint(("DiskInfo: 1st trk %x, last trk %x\n", Vcb->FirstTrackNum, Vcb->LastTrackNum));
#ifdef UDF_FORMAT_MEDIA
        if(fms && fms->opt_disk_info) {
            UserPrint(("First track: %d\n"
                       "Last track:  %d\n", Vcb->FirstTrackNum, Vcb->LastTrackNum));
            UserPrint(("------------------------------------------\n"));
        }
#endif //UDF_FORMAT_MEDIA

        RC = UDFReallocTrackMap(Vcb, Vcb->LastTrackNum+1);
        if(!OS_SUCCESS(RC))
            try_return(RC);

        // Get last LBA from invisible track (if any)
        RtlZeroMemory(TrackInfoOut,sizeof(TRACK_INFO_BLOCK_USER_OUT));

        TrackInfoIn->LBA_TrkNum = 0; // invisible track
        TrackInfoIn->Track = TRUE;

        RC = UDFPhSendIOCTL(IOCTL_CDRW_READ_TRACK_INFO, DeviceObject,
                TrackInfoIn, sizeof(TRACK_INFO_BLOCK_USER_IN),
                TrackInfoOut,sizeof(TRACK_INFO_BLOCK_USER_OUT), TRUE, NULL);
        if(OS_SUCCESS(RC)) {
            if((Vcb->LastTrackNum < TrackInfoOut->TrackNum) &&
                TrackInfoOut->TrackLength &&
               (TrackInfoOut->TrackStartLBA != TrackInfoOut->NextWriteLBA)) {
                Vcb->LastTrackNum = TrackInfoOut->TrackNum;
                if(!(TrackInfoOut->NextWriteLBA & 0x80000000))
                    Vcb->NWA = TrackInfoOut->NextWriteLBA;
                if(TrackInfoOut->TrackLength > 1) {
                    Vcb->LastPossibleLBA =
                        TrackInfoOut->TrackStartLBA + TrackInfoOut->TrackLength - (TrackInfoOut->TrackLength ? 1 : 0);
                    KdPrint((" set LastPossibleLBA=%x\n", Vcb->LastPossibleLBA));
                }
            }

            KdPrint(("Ses %d, Track %d (%x, len %x) PckSize %x: \n"
                     "  NWA: %x (%s)  DatType:%x, %s %s %s %s TrkType:%x %s %s\n"
                     "  LRA: %x (%s)  RC_LBA:%x\n",
                TrackInfoOut->SesNum,
                0,
                TrackInfoOut->TrackStartLBA,
                TrackInfoOut->TrackLength,
                TrackInfoOut->FixPacketSize,

                TrackInfoOut->NextWriteLBA,
                TrackInfoOut->NWA_V & TrkInfo_NWA_V ? "vld" : "inv",
                TrackInfoOut->DataParam.Flags & TrkInfo_Dat_Mask,
                (TrackInfoOut->DataParam.Flags & TrkInfo_Packet) ? "Pck" : "",
                (TrackInfoOut->DataParam.Flags & TrkInfo_FP) ? "FP" : "",
                (TrackInfoOut->DataParam.Flags & TrkInfo_Blank) ? "Blank" : "",
                (TrackInfoOut->DataParam.Flags & TrkInfo_RT) ? "RT" : "",

                TrackInfoOut->TrackParam.Flags & TrkInfo_Trk_Mask,
                (TrackInfoOut->TrackParam.Flags & TrkInfo_Copy) ? "Cpy" : "",
                (TrackInfoOut->TrackParam.Flags & TrkInfo_Damage) ? "Damage" : "",

                TrackInfoOut->LastRecordedAddr,
                (TrackInfoOut->NWA_V & TrkInfo_LRA_V) ? "vld" : "inv",

                TrackInfoOut->ReadCompatLBA
                ));
#ifdef UDF_FORMAT_MEDIA
            if(fms && fms->opt_disk_info) {
                UserPrint(("Invisible track: \n"));
                UserPrint(("  Ses %d, Track %d (%x, len %x) PckSize %x: \n"
                           "    NWA: %x (%s)  DatType:%x, %s %s %s %s TrkType:%x %s %s\n"
                           "    LRA: %x (%s)  RC_LBA:%x\n",
                    TrackInfoOut->SesNum,
                    0,
                    TrackInfoOut->TrackStartLBA,
                    TrackInfoOut->TrackLength,
                    TrackInfoOut->FixPacketSize,

                    TrackInfoOut->NextWriteLBA,
                    TrackInfoOut->NWA_V & TrkInfo_NWA_V ? "vld" : "inv",
                    TrackInfoOut->DataParam.Flags & TrkInfo_Dat_Mask,
                    (TrackInfoOut->DataParam.Flags & TrkInfo_Packet) ? "Pck" : "",
                    (TrackInfoOut->DataParam.Flags & TrkInfo_FP) ? "FP" : "",
                    (TrackInfoOut->DataParam.Flags & TrkInfo_Blank) ? "Blank" : "",
                    (TrackInfoOut->DataParam.Flags & TrkInfo_RT) ? "RT" : "",

                    TrackInfoOut->TrackParam.Flags & TrkInfo_Trk_Mask,
                    (TrackInfoOut->TrackParam.Flags & TrkInfo_Copy) ? "Cpy" : "",
                    (TrackInfoOut->TrackParam.Flags & TrkInfo_Damage) ? "Damage" : "",

                    TrackInfoOut->LastRecordedAddr,
                    (TrackInfoOut->NWA_V & TrkInfo_LRA_V) ? "vld" : "inv",

                    TrackInfoOut->ReadCompatLBA
                    ));
            }
#endif //UDF_FORMAT_MEDIA

        }

        for (TrackNumber=(LONG)DiscInfo->FirstTrackNum;TrackNumber <= (LONG)Vcb->LastTrackNum;TrackNumber++) {

            RtlZeroMemory(TrackInfoOut,sizeof(TRACK_INFO_BLOCK_USER_OUT));
            TrackInfoIn->LBA_TrkNum = TrackNumber;
            TrackInfoIn->Track = TRUE;

            RC = UDFPhSendIOCTL(IOCTL_CDRW_READ_TRACK_INFO, DeviceObject,
                    TrackInfoIn, sizeof(TRACK_INFO_BLOCK_USER_IN),
                    TrackInfoOut,sizeof(TRACK_INFO_BLOCK_USER_OUT), TRUE, NULL);
            // fill sector type map
            if(TrackInfoOut->TrackStartLBA & 0x80000000) {
                KdPrint(("TrkInfo: Bad FirstLba (%x), change to %x\n", TrackInfoOut->TrackStartLBA, 0));
                Vcb->TrackMap[TrackNumber].FirstLba = 0;
            } else {
                Vcb->TrackMap[TrackNumber].FirstLba = TrackInfoOut->TrackStartLBA;
            }
            if(TrackInfoOut->TrackLength & 0x80000000) {
                KdPrint(("TrkInfo: Bad TrackLength (%x), change to %x\n", TrackInfoOut->TrackLength,
                    Vcb->LastPossibleLBA - Vcb->TrackMap[TrackNumber].FirstLba + 1));
                TrackInfoOut->TrackLength = Vcb->LastPossibleLBA - Vcb->TrackMap[TrackNumber].FirstLba + 1;
            }
            Vcb->TrackMap[TrackNumber].LastLba = TrackInfoOut->TrackStartLBA + 
                                                 TrackInfoOut->TrackLength - 
                                            (TrackInfoOut->TrackLength ? 1 : 0);

            Vcb->TrackMap[TrackNumber].TrackParam = TrackInfoOut->TrackParam.Flags;
            Vcb->TrackMap[TrackNumber].DataParam = TrackInfoOut->DataParam.Flags;
            Vcb->TrackMap[TrackNumber].NWA_V = TrackInfoOut->NWA_V;
            if((TrackInfoOut->NextWriteLBA & 0x80000000) || 
               (TrackInfoOut->NextWriteLBA < TrackInfoOut->TrackStartLBA)) {
                if(!(Vcb->TrackMap[TrackNumber].LastLba & 0x8000000)) {
                    KdPrint(("TrkInfo: set NWA to LastLba (%x)\n", Vcb->TrackMap[TrackNumber].LastLba));
                    Vcb->TrackMap[TrackNumber].NWA =
                        Vcb->TrackMap[TrackNumber].LastLba;
                } else {
                    KdPrint(("TrkInfo: set NWA to INV (1)\n"));
                    Vcb->TrackMap[TrackNumber].NWA = 0;
                    Vcb->TrackMap[TrackNumber].NWA_V = 0;
                }
            } else {
                if(!(TrackInfoOut->NextWriteLBA & 0x80000000)) {
                    KdPrint(("TrkInfo: Good NWA (%x)\n", TrackInfoOut->NextWriteLBA));
                    Vcb->TrackMap[TrackNumber].NWA =
                        TrackInfoOut->NextWriteLBA;
                } else {
                    KdPrint(("TrkInfo: set NWA to INV (2)\n"));
                    Vcb->TrackMap[TrackNumber].NWA = 0;
                    Vcb->TrackMap[TrackNumber].NWA_V = 0;
                }
            }
            Vcb->TrackMap[TrackNumber].Session = TrackInfoOut->SesNum;
            // for FP tracks we shall get PacketSize from returned info
            // otherwise set to default UDF value (0x20)
            if(NotFP) {
                KdPrint(("Apply NotFP\n"));
                Vcb->TrackMap[TrackNumber].DataParam &= ~TrkInfo_FP;
#ifdef DBG
                TrackInfoOut->DataParam.Flags &= ~TrkInfo_FP;
#endif //DBG
            } else
            if(ForceFP) {
                KdPrint(("Apply ForceFP\n"));
                PacketTrack = TRUE;
                Vcb->TrackMap[TrackNumber].DataParam |= TrkInfo_FP;
#ifdef DBG
                TrackInfoOut->DataParam.Flags |= TrkInfo_FP;
#endif //DBG
            }
            if(Vcb->TrackMap[TrackNumber].DataParam & TrkInfo_FP) {
                Vcb->TrackMap[TrackNumber].PacketSize = TrackInfoOut->FixPacketSize;
                Vcb->VCBFlags |= UDF_VCB_FLAGS_RAW_DISK;
                Vcb->FP_disc = TRUE;
            } else {
                Vcb->TrackMap[TrackNumber].PacketSize = PACKETSIZE_UDF;
            }
            // presence of Damaged track means, that we should mount this disk in RAW mode
            if(Vcb->TrackMap[TrackNumber].TrackParam & TrkInfo_Damage) {
                KdPrint(("TrkInfo_Damage, Try RAW_MOUNT\n"));
                Vcb->VCBFlags |= UDF_VCB_FLAGS_RAW_DISK;
            }
            // presence of track with Unknown data type means, that we should mount
            // this disk in RAW mode
            if((TrackInfoOut->DataParam.Flags & TrkInfo_Dat_Mask) == TrkInfo_Trk_unknown) {
                KdPrint(("Unknown DatType, Try RAW_MOUNT\n"));
                Vcb->VCBFlags |= UDF_VCB_FLAGS_RAW_DISK;
            }

            PacketTrack |= ((TrackInfoOut->DataParam.Flags & TrkInfo_Packet) != 0);

            KdPrint(("Ses %d, Track %d (%x - %x) PckSize %x: \n"
                     "  NWA: %x (%s)  DatType:%x, %s %s %s %s TrkType:%x %s %s\n"
                     "  LRA: %x (%s)  RC_LBA:%x\n",
                TrackInfoOut->SesNum,
                TrackNumber,
                Vcb->TrackMap[TrackNumber].FirstLba,
                Vcb->TrackMap[TrackNumber].LastLba,
                TrackInfoOut->FixPacketSize,

                TrackInfoOut->NextWriteLBA,
                TrackInfoOut->NWA_V & TrkInfo_NWA_V ? "vld" : "inv",
                TrackInfoOut->DataParam.Flags & TrkInfo_Dat_Mask,
                (TrackInfoOut->DataParam.Flags & TrkInfo_Packet) ? "Pck" : "",
                (TrackInfoOut->DataParam.Flags & TrkInfo_FP) ? "FP" : "",
                (TrackInfoOut->DataParam.Flags & TrkInfo_Blank) ? "Blank" : "",
                (TrackInfoOut->DataParam.Flags & TrkInfo_RT) ? "RT" : "",

                TrackInfoOut->TrackParam.Flags & TrkInfo_Trk_Mask,
                (TrackInfoOut->TrackParam.Flags & TrkInfo_Copy) ? "Cpy" : "",
                (TrackInfoOut->TrackParam.Flags & TrkInfo_Damage) ? "Damage" : "",

                TrackInfoOut->LastRecordedAddr,
                (TrackInfoOut->NWA_V & TrkInfo_LRA_V) ? "vld" : "inv",

                TrackInfoOut->ReadCompatLBA
                ));
#ifdef UDF_FORMAT_MEDIA
            if(fms && fms->opt_disk_info) {
                UserPrint(("Track %d: \n", TrackNumber));
                UserPrint(("  Ses %d, Track %d (%x, len %x) PckSize %x: \n"
                           "    NWA: %x (%s)  DatType:%x, %s %s %s %s TrkType:%x %s %s\n"
                           "    LRA: %x (%s)  RC_LBA:%x\n",
                    TrackInfoOut->SesNum,
                    TrackNumber,
                    TrackInfoOut->TrackStartLBA,
                    TrackInfoOut->TrackLength,
                    TrackInfoOut->FixPacketSize,

                    TrackInfoOut->NextWriteLBA,
                    TrackInfoOut->NWA_V & TrkInfo_NWA_V ? "vld" : "inv",
                    TrackInfoOut->DataParam.Flags & TrkInfo_Dat_Mask,
                    (TrackInfoOut->DataParam.Flags & TrkInfo_Packet) ? "Pck" : "",
                    (TrackInfoOut->DataParam.Flags & TrkInfo_FP) ? "FP" : "",
                    (TrackInfoOut->DataParam.Flags & TrkInfo_Blank) ? "Blank" : "",
                    (TrackInfoOut->DataParam.Flags & TrkInfo_RT) ? "RT" : "",

                    TrackInfoOut->TrackParam.Flags & TrkInfo_Trk_Mask,
                    (TrackInfoOut->TrackParam.Flags & TrkInfo_Copy) ? "Cpy" : "",
                    (TrackInfoOut->TrackParam.Flags & TrkInfo_Damage) ? "Damage" : "",

                    TrackInfoOut->LastRecordedAddr,
                    (TrackInfoOut->NWA_V & TrkInfo_LRA_V) ? "vld" : "inv",

                    TrackInfoOut->ReadCompatLBA
                    ));
            }
#endif //UDF_FORMAT_MEDIA

            if(TrackNumber == DiscInfo->FirstTrackNum) {
                if(!(Vcb->TrackMap[TrackNumber].FirstLba & 0x80000000)) {
                    KdPrint(("TrkInfo: Update FirstLBA (%x)\n", Vcb->TrackMap[TrackNumber].FirstLba));
                    Vcb->FirstLBA = Vcb->TrackMap[TrackNumber].FirstLba;
                }
            }
            if((TrackInfoOut->SesNum == Vcb->LastSession) && !Vcb->FirstTrackNumLastSes) {
                if(!(Vcb->TrackMap[TrackNumber].FirstLba & 0x80000000)) {
                    KdPrint(("TrkInfo: Update FirstLBALastSes (%x)\n", Vcb->TrackMap[TrackNumber].FirstLba));
                    Vcb->FirstLBALastSes = Vcb->TrackMap[TrackNumber].FirstLba;
                }
                Vcb->FirstTrackNumLastSes = TrackNumber;
            }
        }

        if(!(TrackInfoOut->NextWriteLBA & 0x80000000) &&
           !(TrackInfoOut->TrackLength  & 0x80000000) &&
            (Vcb->NWA < TrackInfoOut->NextWriteLBA)
           ) {
            KdPrint((" set NWA to %x\n", TrackInfoOut->NextWriteLBA));
            if(Vcb->MediaClassEx != CdMediaClass_DVDpRW) {
                Vcb->NWA = TrackInfoOut->NextWriteLBA;
            } else {
                Vcb->NWA = 
                    TrackInfoOut->TrackStartLBA + TrackInfoOut->TrackLength - (TrackInfoOut->TrackLength ? 1 : 0);
            }
        }
        if(Vcb->MediaClassEx != CdMediaClass_DVDpRW &&
           !(TrackInfoOut->TrackLength & 0x80000000) &&
           TrackInfoOut->TrackLength > 1) {
            Vcb->LastPossibleLBA =
                TrackInfoOut->TrackStartLBA + TrackInfoOut->TrackLength - (TrackInfoOut->TrackLength ? 1 : 0);
            KdPrint((" set LastPossibleLBA=%x\n", Vcb->LastPossibleLBA));
        }
        TrackNumber = Vcb->LastTrackNum;
        // quick formatted +RW returns bogus value
        if(Vcb->MediaClassEx == CdMediaClass_DVDpRW) {
            KdPrint((" check quick formatted +RW\n"));
            if(Vcb->TrackMap[TrackNumber].LastLba &&
               !(Vcb->TrackMap[TrackNumber].LastLba & 0x80000000) &&
               Vcb->TrackMap[TrackNumber].LastLba < Vcb->LastPossibleLBA /*&&
               Vcb->TrackMap[TrackNumber].LastLba != Vcb->LastPossibleLBA*/
               ) {
                KdPrint((" track LastLBA %x != LastPossibleLBA %x, verify\n",
                    Vcb->TrackMap[TrackNumber].LastLba, Vcb->LastPossibleLBA));

                if(Vcb->MRWStatus == DiscInfo_BGF_Complete) {
                    KdPrint((" complete MRW state\n"));
#ifdef _BROWSE_UDF_
                    Vcb->LastPossibleLBA =
                    Vcb->NWA = 
                    Vcb->LastLBA =
                    Vcb->TrackMap[TrackNumber].LastLba;
                    goto valid_track_length;
#endif // _BROWSE_UDF_
                } else
                if(Vcb->MRWStatus) {
                    uint8* buff;
                    uint32 ReadBytes;

                    KdPrint((" MRW state %x\n", Vcb->MRWStatus));

                    buff = (uint8*)DbgAllocatePoolWithTag(NonPagedPool, Vcb->WriteBlockSize, 'bNWD' );
                    if(buff) {
                        RC = UDFTRead(Vcb,
                                       buff,
                                       Vcb->WriteBlockSize,
                                       Vcb->TrackMap[TrackNumber].LastLba+1,
                                       &ReadBytes,
                                       PH_TMP_BUFFER);
                        DbgFreePool(buff);
                        if(!OS_SUCCESS(RC)) {
                            KdPrint((" Can't read beyond track LastLBA (%x)\n", Vcb->TrackMap[TrackNumber].LastLba+1));
                            Vcb->LastLBA = Vcb->TrackMap[TrackNumber].LastLba;
                            Vcb->NWA = Vcb->LastLBA+1;
                            Vcb->TrackMap[TrackNumber].NWA_V = 1;
                            Vcb->TrackMap[TrackNumber].NWA = Vcb->NWA;
                            Vcb->TrackMap[TrackNumber].LastLba = Vcb->LastPossibleLBA;
                            RC = STATUS_SUCCESS;
                            goto valid_track_length;
                        }
                    }
                }
            }
            KdPrint((" set track LastLBA %x\n", Vcb->LastPossibleLBA));
            Vcb->NWA = 
            Vcb->LastLBA =
            Vcb->TrackMap[TrackNumber].LastLba =
                Vcb->LastPossibleLBA;
        }
valid_track_length:
        // Test for last empty session
        if((Vcb->TrackMap[TrackNumber].Session !=
            Vcb->TrackMap[TrackNumber-1].Session) &&
           (Vcb->LastSession > 1)) {
            // Note: some devices return negative track length
            if((Vcb->TrackMap[TrackNumber].LastLba <=  
                Vcb->TrackMap[TrackNumber].FirstLba) ||
               (Vcb->TrackMap[TrackNumber].FirstLba ==
                Vcb->TrackMap[TrackNumber].NWA)) {
                // empty last session...
                Vcb->LastTrackNum--;
//                TrackNumber--;
/*                for(SesNum = Vcb->TrackMap[TrackNumber].Session;
                    Vcb->TrackMap[TrackNumber].Session == SesNum;
                    TrackNumber--) {
                }*/
                if(TrackNumber>1)
                    Vcb->LastSession = Vcb->TrackMap[TrackNumber-1].Session;
            }
        }

        TrackNumber = Vcb->LastTrackNum;
#ifdef _BROWSE_UDF_
        Vcb->LastLBA = min(Vcb->TrackMap[TrackNumber].LastLba, Vcb->TrackMap[TrackNumber].NWA);
#endif //_BROWSE_UDF_

        if(Vcb->TrackMap[TrackNumber].NWA_V & TrkInfo_NWA_V) {
            KdPrint((" NWA ok, set LastLBA to min(Last %x, NWA %x\n",
                Vcb->TrackMap[TrackNumber].LastLba,
                Vcb->TrackMap[TrackNumber].NWA));
            Vcb->LastLBA = min(Vcb->TrackMap[TrackNumber].LastLba, Vcb->TrackMap[TrackNumber].NWA);
        } else {
            KdPrint((" no NWA, set LastLBA to Last %x\n", Vcb->TrackMap[TrackNumber].LastLba));
            Vcb->LastLBA = Vcb->TrackMap[TrackNumber].LastLba;
        }

        Vcb->VCBFlags |= UDF_VCB_FLAGS_TRACKMAP;
        if(!PacketTrack && Vcb->MediaClassEx != CdMediaClass_DVDRAM ) {
            KdPrint((" disable Raw mount\n"));
            Vcb->VCBFlags &= ~UDF_VCB_FLAGS_RAW_DISK;
        }

try_exit:    NOTHING;

    } _SEH2_FINALLY {
        if(DiscInfo) MyFreePool__(DiscInfo);
        if(TrackInfoOut) MyFreePool__(TrackInfoOut);
    } _SEH2_END;

    return RC;
} // end UDFReadDiscTrackInfo()

/*
    This routine attempts to read disk layout using ReadFullTOC cmd
 */
OSSTATUS
UDFReadAndProcessFullToc(
    PDEVICE_OBJECT DeviceObject, // the target device object
    PVCB           Vcb
    )
{
    OSSTATUS                RC = STATUS_SUCCESS;
    PREAD_FULL_TOC_USER_OUT toc = (PREAD_FULL_TOC_USER_OUT)MyAllocatePool__(NonPagedPool,sizeof(READ_FULL_TOC_USER_OUT) );
    uint32 index;
    uint8 POINT;
    uint8 CurTrack = 0;
    uint32 LastLeadOut = 0;
//    BOOLEAN IsMRW = FALSE;

    KdPrint(("UDFReadAndProcessFullToc\n"));

    if(!toc) return STATUS_INSUFFICIENT_RESOURCES;
    Vcb->FirstTrackNum = 0xFF;

    RtlZeroMemory(toc,sizeof(READ_FULL_TOC_USER_OUT));

    RC = UDFPhSendIOCTL(IOCTL_CDRW_READ_FULL_TOC,DeviceObject,
        NULL,0,
        toc,sizeof(READ_FULL_TOC_USER_OUT),
        TRUE,NULL);

    if(!OS_SUCCESS(RC)) {

        MyFreePool__(toc);
        return RC;
    }

#ifdef _CONSOLE
    Vcb->PhDeviceType = FILE_DEVICE_CD_ROM;
#endif //_CONSOLE
    Vcb->LastSession = toc->Sessions.Last_TrackSes;

    RC = UDFReallocTrackMap(Vcb, 0x100);
    if(!OS_SUCCESS(RC)) {
        MyFreePool__(toc);
        return RC;
    }

    // get LastPossibleLBA

    // Note: some drives return Full TOC items unordered.
    // So, LeadOut position may come before Track definition.
    // In order to handle such situation, we must initialize
    // CurTrack when First or Last Track descriptor comes
    for (index=0;(index<MAXIMUM_NUMBER_OF_SESSIONS);index++) {
/*        if((toc->SessionData[index].Adr == TOC_ADR_TrackInfo) &&
           ((toc->SessionData[index].Control == TOC_CTL_MRWTrackInfo) || (toc->SessionData[index].Control == TOC_CTL_MRWLastSes))) {
            IsMRW = TRUE;
        }*/
        if(toc->SessionData[index].Adr == 1) {
            switch (POINT = toc->SessionData[index].POINT) {
            case POINT_FirstTrackNum: {
                Vcb->FirstTrackNum = toc->SessionData[index].Params.FirstTrackNum.FirstTrackNum;
                if(!CurTrack)
                    CurTrack = (uint8)(Vcb->FirstTrackNum);
                break;
                }
            case POINT_LastTrackNum: {
                Vcb->LastTrackNum = toc->SessionData[index].Params.LastTrackNum.LastTrackNum;
                if(CurTrack < Vcb->LastTrackNum)
                    CurTrack = (uint8)(Vcb->FirstTrackNum);
                break;
                }
            case POINT_StartPositionOfLeadOut: {
#define TempMSF toc->SessionData[index].Params.StartPositionOfLeadOut.MSF
                Vcb->TrackMap[CurTrack].LastLba = MSF_TO_LBA(TempMSF[0],TempMSF[1],TempMSF[2]);
                LastLeadOut = max(LastLeadOut, Vcb->TrackMap[CurTrack].LastLba);
#undef TempMSF
                break;
                }
            default: {
                if( (Vcb->FirstTrackNum != 0x0FF) &&
                     (toc->SessionData[index].POINT == Vcb->FirstTrackNum) ) {
#define TempMSF toc->SessionData[index].Params.StartPositionOfTrack.MSF
                    Vcb->FirstLBA = MSF_TO_LBA(TempMSF[0],TempMSF[1],TempMSF[2]);
                    if(Vcb->FirstLBA & 0x80000000) {
                        Vcb->FirstLBA = 0;
                    }
#undef TempMSF
                }
                break;
                }
            }
            if((POINT >= POINT_StartPositionOfTrack_Min) &&
               (POINT <= POINT_StartPositionOfTrack_Max)) {
#define TempMSF toc->SessionData[index].Params.StartPositionOfTrack.MSF
                Vcb->TrackMap[POINT].FirstLba = MSF_TO_LBA(TempMSF[0],TempMSF[1],TempMSF[2])-1;
                if(Vcb->TrackMap[POINT].FirstLba & 0x80000000) {
                    if(POINT == 1) {
                        Vcb->TrackMap[POINT].FirstLba = 0;
                    } else {
                        if(Vcb->TrackMap[POINT-1].LastLba) {
                            Vcb->TrackMap[POINT].FirstLba = Vcb->TrackMap[POINT-1].LastLba+1;
                        }
                    }
                }
#undef TempMSF
                if(POINT > POINT_StartPositionOfTrack_Min) {
                    Vcb->TrackMap[POINT-1].LastLba = Vcb->TrackMap[POINT].FirstLba-1;
                }
                CurTrack = POINT;
            }
        } else
        if(toc->SessionData[index].Adr == 5) {
            switch (POINT = toc->SessionData[index].POINT) {
            case POINT_StartPositionOfNextProgramArea: {
#define TempMSF toc->SessionData[index].Params.StartPositionOfNextProgramArea.MaxLeadOut_MSF
                Vcb->LastPossibleLBA = MSF_TO_LBA(TempMSF[0],TempMSF[1],TempMSF[2]);
#undef TempMSF
                break;
                }
            default: {
                break;
                }
            }
        }

    }

/*    if(!IsMRW) {
        KdPrint(("No MRW\n"));
        Vcb->CompatFlags &= ~UDF_VCB_IC_MRW_ADDR_PROBLEM;
    }*/
//        Vcb->CompatFlags &= ~UDF_VCB_IC_MRW_ADDR_PROBLEM;
    // some devices report LastTrackNum=0 for full disks
    Vcb->LastTrackNum = max(Vcb->LastTrackNum, Vcb->FirstTrackNum);
    Vcb->TrackMap[Vcb->LastTrackNum].LastLba = max(LastLeadOut, Vcb->TrackMap[Vcb->LastTrackNum].LastLba);

    Vcb->LastLBA = Vcb->TrackMap[Vcb->LastTrackNum].LastLba;

    MyFreePool__(toc);
//    Vcb->LastLBA=PacketVariable2Fixed(Vcb->LastLBA)-2;
    return STATUS_SUCCESS;
} // end UDFReadAndProcessFullToc()

/*
    use standard way to determine disk layout (ReadTOC cmd)
 */
OSSTATUS
UDFUseStandard(
    PDEVICE_OBJECT DeviceObject, // the target device object
    PVCB           Vcb           // Volume control block from this DevObj
    )
{
    OSSTATUS                RC = STATUS_SUCCESS;
    PREAD_TOC_USER_OUT      toc = (PREAD_TOC_USER_OUT)MyAllocatePool__(NonPagedPool,max(Vcb->BlockSize, sizeof(READ_TOC_USER_OUT)) );
    PGET_LAST_SESSION_USER_OUT LastSes = (PGET_LAST_SESSION_USER_OUT)MyAllocatePool__(NonPagedPool,sizeof(GET_LAST_SESSION_USER_OUT) );
    uint32                  LocalTrackCount;
    uint32                  LocalTocLength;
    uint32                  TocEntry;
#ifdef _BROWSE_UDF_
    uint32                  OldTrkNum;
    uint32                  TrkNum;
    uint32                  ReadBytes, i, len;
#endif //_BROWSE_UDF_
#ifdef UDF_FORMAT_MEDIA
    PUDFFmtState            fms = Vcb->fms;
#else
  #define fms FALSE
#endif //UDF_FORMAT_MEDIA

    KdPrint(("UDFUseStandard\n"));

    _SEH2_TRY {

        if(!toc || !LastSes) {
            try_return (RC = STATUS_INSUFFICIENT_RESOURCES);
        }
        RtlZeroMemory(toc,sizeof(READ_TOC_TOC));

        Vcb->VCBFlags |= UDF_VCB_FLAGS_USE_STD;

        RC = UDFPhSendIOCTL(IOCTL_CDROM_READ_TOC,DeviceObject,
            toc,sizeof(READ_TOC_USER_OUT),
            toc,sizeof(READ_TOC_USER_OUT),
            TRUE,NULL );

        if((RC == STATUS_DEVICE_NOT_READY) || (RC == STATUS_NO_MEDIA_IN_DEVICE)) {
            try_return(RC);
        }
#ifdef UDF_FORMAT_MEDIA
        if(fms->opt_media == MT_none) {
            try_return(RC = STATUS_NO_MEDIA_IN_DEVICE);
        }
#endif //UDF_FORMAT_MEDIA
    
        // If even standard read toc does not work, then use default values
        if(!OS_SUCCESS(RC)) {

            RC = UDFReallocTrackMap(Vcb, 2);
            if(!OS_SUCCESS(RC)) {
                try_return(RC);
            }

            Vcb->LastSession=1;
            Vcb->FirstTrackNum=1;
//            Vcb->FirstLBA=0;
            Vcb->LastTrackNum=1;
            Vcb->TrackMap[1].FirstLba = Vcb->FirstLBA;
            Vcb->TrackMap[1].LastLba = Vcb->LastLBA;
            Vcb->TrackMap[1].PacketSize = PACKETSIZE_UDF;
#ifdef UDF_FORMAT_MEDIA
            if(!fms) {
#endif //UDF_FORMAT_MEDIA

#ifdef _BROWSE_UDF_
#ifdef UDF_HDD_SUPPORT
                if(UDFGetDevType(DeviceObject) == FILE_DEVICE_DISK) {
                    try_return(RC = STATUS_SUCCESS);
                }
#endif //UDF_HDD_SUPPORT
#endif //_BROWSE_UDF_

#ifdef UDF_FORMAT_MEDIA
            } else {

                if(fms->opt_media == MT_HD) {
                    Vcb->LastPossibleLBA = Vcb->LastLBA;
                    try_return(RC = STATUS_SUCCESS);
                }
            }
#endif //UDF_FORMAT_MEDIA
            Vcb->LastPossibleLBA = max(Vcb->LastLBA, DEFAULT_LAST_LBA_FP_CD);
            Vcb->TrackMap[1].DataParam = TrkInfo_Dat_XA | TrkInfo_FP | TrkInfo_Packet;
            Vcb->TrackMap[1].TrackParam = TrkInfo_Trk_XA;
            Vcb->TrackMap[1].NWA = 0xffffffff;
            Vcb->NWA = DEFAULT_LAST_LBA_FP_CD + 7 + 1;
            try_return(RC = STATUS_SUCCESS);
        }

#ifdef _CONSOLE
        Vcb->PhDeviceType = FILE_DEVICE_CD_ROM;
#endif //_CONSOLE
    
        LocalTrackCount = toc->Tracks.Last_TrackSes - toc->Tracks.First_TrackSes + 1;
        LocalTocLength = PtrOffset( toc, &(toc->TrackData[LocalTrackCount + 1]) );
      
        // Get out if there is an immediate problem with the TOC.
        if(toc->Tracks.First_TrackSes > toc->Tracks.Last_TrackSes) {
            try_return(RC = STATUS_DISK_CORRUPT_ERROR);
        }

#ifdef _BROWSE_UDF_        
        Vcb->LastTrackNum=toc->Tracks.Last_TrackSes;
        Vcb->FirstTrackNum=toc->Tracks.First_TrackSes;
        // some devices report LastTrackNum=0 for full disks
        Vcb->LastTrackNum = max(Vcb->LastTrackNum, Vcb->FirstTrackNum);

        RC = UDFReallocTrackMap(Vcb, MAXIMUM_NUMBER_OF_TRACKS+1);
/*        if(Vcb->TrackMap) {
            MyFreePool__(Vcb->TrackMap);
            Vcb->TrackMap = NULL;
        }
        Vcb->TrackMap = (PUDFTrackMap)
            MyAllocatePool__(NonPagedPool, (MAXIMUM_NUMBER_OF_TRACKS+1)*sizeof(UDFTrackMap));
        if(!Vcb->TrackMap) {
            MyFreePool__(toc);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlZeroMemory(Vcb->TrackMap,(MAXIMUM_NUMBER_OF_TRACKS+1)*sizeof(UDFTrackMap));
*/
        if(!OS_SUCCESS(RC)) {
            BrutePoint();
            try_return(RC);
        }
        // find 1st and last session
        RC = UDFPhSendIOCTL(IOCTL_CDROM_GET_LAST_SESSION,DeviceObject,
            LastSes,sizeof(GET_LAST_SESSION_USER_OUT),
            LastSes,sizeof(GET_LAST_SESSION_USER_OUT),
            TRUE,NULL );

        if(OS_SUCCESS(RC)) {
            TrkNum = LastSes->LastSes_1stTrack.TrackNum;
            Vcb->LastSession = LastSes->Sessions.First_TrackSes;
            for(TocEntry=0;TocEntry<LocalTrackCount + 1;TocEntry++) {
                if(toc->TrackData[TocEntry].TrackNum == TrkNum) {
                    Vcb->TrackMap[TrkNum].Session = Vcb->LastSession;
                }
            }
        }
    
        OldTrkNum = 0;
        // Scan toc for first & last LBA
        for(TocEntry=0;TocEntry<LocalTrackCount + 1;TocEntry++) {
#define TempMSF toc->TrackData[TocEntry].LBA
            TrkNum = toc->TrackData[TocEntry].TrackNum;
#ifdef UDF_DBG
            if (TrkNum >= MAXIMUM_NUMBER_OF_TRACKS &&
                TrkNum != TOC_LastTrack_ID) {
                KdPrint(("UDFUseStandard: Array out of bounds\n"));
                BrutePoint();
                try_return(RC = STATUS_SUCCESS);
            }
            KdPrint(("Track N %d (0x%x) first LBA %ld (%lx) \n",TrkNum,TrkNum,
                MSF_TO_LBA(TempMSF[1],TempMSF[2],TempMSF[3]),
                MSF_TO_LBA(TempMSF[1],TempMSF[2],TempMSF[3])));
#endif // UDF_DBG
            if(Vcb->FirstTrackNum == TrkNum) {
                Vcb->FirstLBA = MSF_TO_LBA(TempMSF[1],TempMSF[2],TempMSF[3]);
                if(Vcb->FirstLBA & 0x80000000) {
                    Vcb->FirstLBA = 0;
                }
            }
            if(TOC_LastTrack_ID   == TrkNum) {
                Vcb->LastLBA  = MSF_TO_LBA(TempMSF[1],TempMSF[2],TempMSF[3])-1;
                Vcb->TrackMap[OldTrkNum].LastLba = Vcb->LastLBA-1;
                KdPrint(("UDFUseStandard: Last track entry, break TOC scan\n"));
//                continue;
                break;
            } else {
                Vcb->TrackMap[TrkNum].FirstLba = MSF_TO_LBA(TempMSF[1],TempMSF[2],TempMSF[3]);
                if(Vcb->TrackMap[TrkNum].FirstLba & 0x80000000)
                    Vcb->TrackMap[TrkNum].FirstLba = 0;
                if(TrkNum) {
                    if (TOC_LastTrack_ID == OldTrkNum) {
                        KdPrint(("UDFUseStandard: Wrong previous track number\n"));
                        BrutePoint();
                    } else {
                        Vcb->TrackMap[OldTrkNum].LastLba = Vcb->TrackMap[TrkNum].FirstLba-1;
                    }
                }
            }
            // check track type
            switch(toc->TrackData[TocEntry].Control & TocControl_TrkMode_Mask) {
            case TocControl_TrkMode_Data:
            case TocControl_TrkMode_IncrData:
                Vcb->TrackMap[TrkNum].DataParam = TrkInfo_Dat_XA;
                Vcb->TrackMap[TrkNum].TrackParam = TrkInfo_Trk_XA;
                break;
            default:
                Vcb->TrackMap[TrkNum].DataParam = TrkInfo_Dat_unknown;
                Vcb->TrackMap[TrkNum].TrackParam = TrkInfo_Trk_unknown;
            }
            OldTrkNum = TrkNum;
#undef TempMSF
        }

        TrkNum = Vcb->LastTrackNum;
        RC = STATUS_SUCCESS;
        // find last _valid_ track
        for(;TrkNum;TrkNum--) {
            if((Vcb->TrackMap[TrkNum].DataParam  != TrkInfo_Dat_unknown) &&
               (Vcb->TrackMap[TrkNum].TrackParam != TrkInfo_Trk_unknown)) {
                RC = STATUS_UNSUCCESSFUL;
                Vcb->LastTrackNum = TrkNum;
                break;
            }
        }
        // no valid tracks...
        if(!TrkNum) {
            KdPrint(("UDFUseStandard: no valid tracks...\n"));
            try_return(RC = STATUS_UNRECOGNIZED_VOLUME);
        }
        i = 0;

        // Check for last VP track. Some last sectors may belong to Link-data &
        // be unreadable. We should forget about them, because UDF needs
        // last _readable_ sector.
        while(!OS_SUCCESS(RC) && (i<8)) {
            RC = UDFPhReadSynchronous(Vcb->TargetDeviceObject, (int8*)toc, Vcb->BlockSize,
                       ((uint64)(Vcb->TrackMap[TrkNum].LastLba-i)) << Vcb->BlockSizeBits, &ReadBytes, PH_TMP_BUFFER);
            i++;
        }
        if(OS_SUCCESS(RC)) {
            Vcb->LastLBA = Vcb->TrackMap[TrkNum].LastLba-i+1;
/*            if(i) {
                Vcb->TrackMap[TrkNum].PacketSize = PACKETSIZE_UDF;
                Vcb->TrackMap[TrkNum].;
            }*/
        } else {

            // Check for FP track. READ_TOC reports actual track length, but
            // Link-data is hidden & unreadable for us. So, available track
            // length may be less than actual. Here we assume that Packet-size
            // is PACKETSIZE_UDF.
            i = 0;
            len = Vcb->TrackMap[TrkNum].LastLba - Vcb->TrackMap[TrkNum].FirstLba + 1;
            len = (uint32)(((int64)len*PACKETSIZE_UDF) / (PACKETSIZE_UDF+7));

            while(!OS_SUCCESS(RC) && (i<9)) {
                RC = UDFPhReadSynchronous(Vcb->TargetDeviceObject, (int8*)toc, Vcb->BlockSize,
                           ((uint64)(Vcb->TrackMap[TrkNum].FirstLba-i+len)) << Vcb->BlockSizeBits, &ReadBytes, PH_TMP_BUFFER);
                i++;
            }
            if(OS_SUCCESS(RC)) {
                Vcb->LastLBA =
                Vcb->TrackMap[TrkNum].LastLba = Vcb->TrackMap[TrkNum].FirstLba-i+len+1;
                Vcb->TrackMap[TrkNum].PacketSize = PACKETSIZE_UDF;
//                Vcb->TrackMap[TrkNum].;
            } else
            if(RC == STATUS_INVALID_DEVICE_REQUEST) {
                // wrap return code from Audio-disk
                RC = STATUS_SUCCESS;
            }
        }

#ifdef UDF_CDRW_EMULATION_ON_ROM
        Vcb->LastPossibleLBA = Vcb->LastLBA+7+1+1024;
        Vcb->NWA = Vcb->LastLBA+7+1;
#else
        Vcb->LastPossibleLBA =
        Vcb->NWA = Vcb->LastLBA+7+1;
#endif //UDF_CDRW_EMULATION_ON_ROM

#else //_BROWSE_UDF_

        Vcb->FirstTrackNum=toc->Tracks.Last_TrackSes;
        Vcb->LastTrackNum=toc->Tracks.First_TrackSes;
    
        // Scan toc for first & last LBA
        for(TocEntry=0;TocEntry<LocalTrackCount + 1;TocEntry++) {
#define TempMSF toc->TrackData[TocEntry].LBA
            if(Vcb->FirstTrackNum == toc->TrackData[TocEntry].TrackNum) {
                Vcb->FirstLBA = MSF_TO_LBA(TempMSF[1],TempMSF[2],TempMSF[3]);
                if(Vcb->FirstLBA & 0x80000000) {
                    Vcb->FirstLBA = 0;
                }
            }
            if(TOC_LastTrack_ID   == toc->TrackData[TocEntry].TrackNum) {
                Vcb->LastLBA = MSF_TO_LBA(TempMSF[1],TempMSF[2],TempMSF[3])-1;
            }
#undef TempMSF
        }
    
//        Vcb->LastLBA=PacketVariable2Fixed(Vcb->LastLBA)-2;
        Vcb->LastPossibleLBA = DEFAULT_LAST_LBA_FP_CD;
#endif //_BROWSE_UDF_
try_exit: NOTHING;
    } _SEH2_FINALLY {
        if(toc) MyFreePool__(toc);
        if(LastSes) MyFreePool__(LastSes);
    } _SEH2_END;

    return RC;
} // end UDFUseStandard()

/*
    Get block size (for read operation)
 */
OSSTATUS
UDFGetBlockSize(
    IN PDEVICE_OBJECT DeviceObject,      // the target device object
    IN PVCB           Vcb                // Volume control block from this DevObj
    )
{
    OSSTATUS        RC = STATUS_SUCCESS;
    PDISK_GEOMETRY  DiskGeometry = (PDISK_GEOMETRY)MyAllocatePool__(NonPagedPool,sizeof(DISK_GEOMETRY));
    PPARTITION_INFORMATION  PartitionInfo = (PPARTITION_INFORMATION)MyAllocatePool__(NonPagedPool,sizeof(PARTITION_INFORMATION)*2);
#ifdef UDF_FORMAT_MEDIA
    PUDFFmtState            fms = Vcb->fms;
#else
  #define fms FALSE
#endif //UDF_FORMAT_MEDIA

    if(!DiskGeometry || !PartitionInfo)
        try_return (RC = STATUS_INSUFFICIENT_RESOURCES);

#ifdef _BROWSE_UDF_

#ifdef UDF_HDD_SUPPORT
    if(!fms) {
        if(UDFGetDevType(DeviceObject) == FILE_DEVICE_DISK) {
            KdPrint(("UDFGetBlockSize: HDD\n"));
            RC = UDFPhSendIOCTL(IOCTL_DISK_GET_DRIVE_GEOMETRY,DeviceObject,
                0,NULL,
                DiskGeometry,sizeof(DISK_GEOMETRY),
                TRUE,NULL );
            Vcb->BlockSize = (OS_SUCCESS(RC)) ? DiskGeometry->BytesPerSector : 512;
            if(!NT_SUCCESS(RC))
                try_return(RC);
            RC = UDFPhSendIOCTL(IOCTL_DISK_GET_PARTITION_INFO,DeviceObject,
                0,NULL,
                PartitionInfo,sizeof(PARTITION_INFORMATION),
                TRUE,NULL );
            if(!NT_SUCCESS(RC)) {
                KdPrint(("UDFGetBlockSize: IOCTL_DISK_GET_PARTITION_INFO failed\n"));
                if(RC = STATUS_INVALID_DEVICE_REQUEST)
                    RC = STATUS_UNRECOGNIZED_VOLUME;
                try_return(RC);
            }
            if(PartitionInfo->PartitionType != PARTITION_IFS) {
                KdPrint(("UDFGetBlockSize: PartitionInfo->PartitionType != PARTITION_IFS\n"));
                try_return(RC = STATUS_UNRECOGNIZED_VOLUME);
            }
        } else {
#endif //UDF_HDD_SUPPORT
            RC = UDFPhSendIOCTL(IOCTL_CDROM_GET_DRIVE_GEOMETRY,DeviceObject,
                DiskGeometry,sizeof(DISK_GEOMETRY),
                DiskGeometry,sizeof(DISK_GEOMETRY),
                TRUE,NULL );

            if(RC == STATUS_DEVICE_NOT_READY) {
                // probably, the device is really busy, may be by CD/DVD recording
                UserPrint(("  busy (0)\n"));
                try_return(RC);
            }

            Vcb->BlockSize = (OS_SUCCESS(RC)) ? DiskGeometry->BytesPerSector : 2048;
#ifdef UDF_HDD_SUPPORT
        }
    }
#endif //UDF_HDD_SUPPORT

#endif //_BROWSE_UDF_

#ifdef UDF_FORMAT_MEDIA
    if(fms) {
        RC = UDFPhSendIOCTL(IOCTL_CDROM_GET_DRIVE_GEOMETRY,DeviceObject,
            DiskGeometry,sizeof(DISK_GEOMETRY),
            DiskGeometry,sizeof(DISK_GEOMETRY),
            FALSE, NULL );

        if(!NT_SUCCESS(RC)) {
            RC = UDFPhSendIOCTL(IOCTL_DISK_GET_DRIVE_GEOMETRY,DeviceObject,
                DiskGeometry,sizeof(DISK_GEOMETRY),
                DiskGeometry,sizeof(DISK_GEOMETRY),
                FALSE, NULL );
            if(NT_SUCCESS(RC)) {
                fms->opt_media = MT_HD;
                RC = UDFPhSendIOCTL(IOCTL_DISK_GET_PARTITION_INFO,DeviceObject,
                    NULL,0,
                    PartitionInfo,sizeof(PARTITION_INFORMATION)*2,
                    FALSE, NULL );
                if(!NT_SUCCESS(RC)) {
                    LONG HiOffs=0;
                    RC = SetFilePointer(DeviceObject->h,0,&HiOffs,FILE_END);
                }
            }
        }

        if(RC == STATUS_DEVICE_NOT_READY) {
            // probably, the device is really busy, may be by CD/DVD recording
            UserPrint(("  busy\n"));
            try_return(RC );
        }

        Vcb->BlockSize = (NT_SUCCESS(RC)) ? DiskGeometry->BytesPerSector : 2048;
    }
#endif //UDF_FORMAT_MEDIA

    // Block size must be an even multiple of 512
    switch (Vcb->BlockSize) {
        case 2048: Vcb->BlockSizeBits = 11; break;
#ifdef UDF_HDD_SUPPORT
        case 512:  Vcb->BlockSizeBits = 9; break;
        case 1024: Vcb->BlockSizeBits = 10; break;
        case 4096: Vcb->BlockSizeBits = 12; break;
        case 8192: Vcb->BlockSizeBits = 13; break;
#endif //UDF_HDD_SUPPORT
        default:
        {
            UserPrint(("UDF: Bad block size (%ld)\n", Vcb->BlockSize));
            try_return(RC = STATUS_UNSUCCESSFUL);
        }
    }

#ifdef UDF_HDD_SUPPORT
    if(
#ifdef _BROWSE_UDF_
        (!fms && (UDFGetDevType(DeviceObject) == FILE_DEVICE_DISK))
                         ||
#endif //_BROWSE_UDF_
#ifdef UDF_FORMAT_MEDIA
        (fms && fms->opt_media == MT_HD)
                         ||
#endif //UDF_FORMAT_MEDIA
         FALSE ) {

#ifdef UDF_FORMAT_MEDIA
        if(fms && !NT_SUCCESS(RC))
            try_return(STATUS_UNSUCCESSFUL);
#endif //UDF_FORMAT_MEDIA

        Vcb->FirstLBA=0;//(ULONG)(PartitionInfo->StartingOffset.QuadPart >> Vcb->BlockSizeBits);
        Vcb->LastPossibleLBA =
        Vcb->LastLBA = (uint32)(PartitionInfo->PartitionLength.QuadPart >> Vcb->BlockSizeBits)/* + Vcb->FirstLBA*/ - 1;
    } else {
#endif //UDF_HDD_SUPPORT
        Vcb->FirstLBA=0;
        if(OS_SUCCESS(RC)) {
            Vcb->LastLBA = (uint32)(DiskGeometry->Cylinders.QuadPart *
                                    DiskGeometry->TracksPerCylinder *
                                    DiskGeometry->SectorsPerTrack - 1);
            if(Vcb->LastLBA == 0x7fffffff) {
                Vcb->LastLBA = UDFIsDvdMedia(Vcb) ? DEFAULT_LAST_LBA_DVD : DEFAULT_LAST_LBA_FP_CD;
            }
        } else {
            Vcb->LastLBA = UDFIsDvdMedia(Vcb) ? DEFAULT_LAST_LBA_DVD : DEFAULT_LAST_LBA_FP_CD;
        }
        Vcb->LastPossibleLBA = Vcb->LastLBA;
#ifdef UDF_HDD_SUPPORT
    }
#endif //UDF_HDD_SUPPORT

#ifdef _BROWSE_UDF_
//    if(UDFGetDevType(DeviceObject) == FILE_DEVICE_DISK) {
        Vcb->WriteBlockSize = PACKETSIZE_UDF*Vcb->BlockSize;
//    } else {
//        Vcb->WriteBlockSize = PACKETSIZE_UDF*Vcb->BlockSize;
//    }
#else //_BROWSE_UDF_
    if(fms->opt_media == MT_HD) {
        Vcb->WriteBlockSize = Vcb->BlockSize;
    } else {
        Vcb->WriteBlockSize = PACKETSIZE_UDF*Vcb->BlockSize;
    }
#endif //_BROWSE_UDF_

    RC = STATUS_SUCCESS;

try_exit:   NOTHING;

    KdPrint(("UDFGetBlockSize:\nBlock size is %x, Block size bits %x, Last LBA is %x\n",
              Vcb->BlockSize, Vcb->BlockSizeBits, Vcb->LastLBA));

    MyFreePool__(PartitionInfo);
    MyFreePool__(DiskGeometry);
    return RC;

} // end UDFGetBlockSize()

#ifdef _BROWSE_UDF_

OSSTATUS
UDFCheckTrackFPAddressing(
//    IN PDEVICE_OBJECT DeviceObject,      // the target device object
    IN PVCB           Vcb,               // Volume control block from this DevObj
    IN ULONG          TrackNum
    )
{
    OSSTATUS RC = STATUS_SUCCESS;
//    OSSTATUS RC2 = STATUS_UNSUCCESSFUL;
    uint32 lba=0;
    uint32 i;
    uint8* Buffer;
//    uint32 ReadBytes;

    uint8  user_data;

    ULONG FirstChunkLen = 0;

    ULONG NextChunkLen = 0;
    ULONG NextChunkLenCount = 0;

    ULONG NextChunkLenOth = 0;
    ULONG NextChunkLenOthCount = 0;
//    ULONG MRW_Offset = 0;

    PLL_READ_USER_IN pLLR_in;
    PCD_SECTOR_HEADER pHdr;
/*    uint8 cMSF[3] = {0,2,0};
    uint8 cMSF1[3] = {0,2,1};*/


    if(!Vcb->TrackMap) {
        Vcb->CompatFlags &= ~UDF_VCB_IC_FP_ADDR_PROBLEM;
        return STATUS_SUCCESS;
    }

    Buffer = (uint8*)DbgAllocatePoolWithTag(NonPagedPool, max(Vcb->BlockSize,
                                                     sizeof(LL_READ_USER_IN)+16), 'pNWD');
    if(!Buffer)
        return STATUS_INSUFFICIENT_RESOURCES;
    pLLR_in = (PLL_READ_USER_IN)Buffer;
    pHdr = (PCD_SECTOR_HEADER)(Buffer+sizeof(LL_READ_USER_IN));

/*    if(Vcb->CompatFlags & UDF_VCB_IC_MRW_ADDR_PROBLEM) {
        MRW_Offset = (MRW_DMA_OFFSET/32)*39;
    }*/

    user_data = 0;
    for(i=0; i<=0x200; i++) {

        RtlZeroMemory(pLLR_in, sizeof(pLLR_in)+16);
        pLLR_in->ExpectedBlkType = ReadCd_BlkType_Any;
        pLLR_in->LBA = i;
        pLLR_in->NumOfBlocks = 1;
        pLLR_in->Flags.Flags = ReadCd_Header_Hdr;
//        pLLR_in->UseMFS = FALSE; // already zero
//        MOV_MSF(pLLR_in->Starting_MSF, cMSF);
//        MOV_MSF(pLLR_in->Ending_MSF, cMSF1);
        RtlZeroMemory(pHdr, sizeof(CD_SECTOR_HEADER));
        RC = UDFPhSendIOCTL(IOCTL_CDRW_LL_READ, Vcb->TargetDeviceObject,
            pLLR_in, sizeof(LL_READ_USER_IN),
            pHdr, sizeof(CD_SECTOR_HEADER),
            TRUE, NULL );

/*        RC = UDFPhReadSynchronous(Vcb->TargetDeviceObject, Buffer, Vcb->BlockSize,
                   ((uint64)(i+MRW_Offset)) << Vcb->BlockSizeBits, &ReadBytes, 0);*/

        // skip unreadable
        if(!OS_SUCCESS(RC)) {
            KdPrint(("  Read error at lba %x\n", i));
            continue;
        }

        // skip strange (damaged ?) blocks
        if((pHdr->Mode.Flags & WParam_SubHdr_Mode_Mask) != WParam_SubHdr_Mode1 &&
           (pHdr->Mode.Flags & WParam_SubHdr_Mode_Mask) != WParam_SubHdr_Mode2) {
            KdPrint(("  Unexpected data type (%x) at lba %x\n", pHdr->Mode.Flags & WParam_SubHdr_Mode_Mask, i));
            continue;
        }

        if((pHdr->Mode.Flags & WParam_SubHdr_Format_Mask) == WParam_SubHdr_Format_UserData &&
            !user_data) {
            lba = i;
        }

/*        if(OS_SUCCESS(RC) && !OS_SUCCESS(RC2)) {
            lba = i;
        }*/

        if((pHdr->Mode.Flags & WParam_SubHdr_Format_Mask) != WParam_SubHdr_Format_UserData &&
            user_data) {
//        if(!OS_SUCCESS(RC) && OS_SUCCESS(RC2)) {
            KdPrint(("  %x - %x (%x sectors)\n", lba, i-1, i-lba));
            if(!FirstChunkLen) {
                FirstChunkLen = i-lba;
            } else {
                if(!NextChunkLen) {
                    NextChunkLen = i-lba;
                    NextChunkLenCount++;
                } else {
                    if(NextChunkLen == i-lba) {
                        NextChunkLenCount++;
                    } else {
                        if((NextChunkLenOth+1) % (NextChunkLen+1)) {
                            NextChunkLenOth = i-lba;
                            NextChunkLenOthCount++;
                        } else {
                            NextChunkLenCount++;
                        }
                    }
                }
            }
        }
        user_data = ((pHdr->Mode.Flags & WParam_SubHdr_Format_Mask) == WParam_SubHdr_Format_UserData);
//        RC2 = RC;
    }

    DbgFreePool(Buffer);

    if(!NextChunkLenCount && !NextChunkLenOthCount) {
        Vcb->CompatFlags &= ~UDF_VCB_IC_FP_ADDR_PROBLEM;
        return STATUS_SUCCESS;
    }
    if(NextChunkLenOthCount > NextChunkLenCount) {
        NextChunkLen = NextChunkLenOth;
    }
    if(NextChunkLen > PACKETSIZE_UDF+7) {
        Vcb->CompatFlags &= ~UDF_VCB_IC_FP_ADDR_PROBLEM;
        return STATUS_SUCCESS;
    }
    Vcb->TrackMap[TrackNum].DataParam &= ~TrkInfo_Dat_Mask;
    Vcb->TrackMap[TrackNum].DataParam |= TrkInfo_Dat_XA;
    Vcb->TrackMap[TrackNum].Flags |= TrackMap_FixFPAddressing;
    Vcb->TrackMap[TrackNum].PacketSize = 1;
    while(NextChunkLen >> Vcb->TrackMap[TrackNum].PacketSize) {
        Vcb->TrackMap[TrackNum].PacketSize++;
    }
    Vcb->TrackMap[TrackNum].PacketSize = 1 << (Vcb->TrackMap[TrackNum].PacketSize-1);
    Vcb->TrackMap[TrackNum].TrackFPOffset = NextChunkLen - FirstChunkLen;  // !!!!!
    Vcb->TrackMap[TrackNum].PacketFPOffset = Vcb->TrackMap[TrackNum].TrackFPOffset;//0;//NextChunkLenOth - FirstChunkLen;
    Vcb->TrackMap[TrackNum].LastLba = (Vcb->TrackMap[TrackNum].LastLba*Vcb->TrackMap[TrackNum].PacketSize) /
           (Vcb->TrackMap[TrackNum].PacketSize + 7);

    return STATUS_SUCCESS;
} // end UDFCheckTrackFPAddressing()

uint32
UDFFixFPAddress(
    IN PVCB           Vcb,               // Volume control block from this DevObj
    IN uint32         Lba
    )
{
    uint32 i = Vcb->LastReadTrack;    
    uint32 pk;
    uint32 rel;

//    if(Vcb->CompatFlags & UDF_VCB_IC_MRW_ADDR_PROBLEM) {
    if(Vcb->TrackMap[i].Flags & TrackMap_FixMRWAddressing) {
        pk = Lba / MRW_DA_SIZE;
        rel = Lba % MRW_DA_SIZE;
        Lba = pk*MRW_DMA_SEGMENT_SIZE + rel;
        Lba += MRW_DMA_OFFSET;
    }
    if(Vcb->TrackMap[i].Flags & TrackMap_FixFPAddressing) {
        if(Lba < 0x20)
            return Lba;
        pk = Lba / Vcb->TrackMap[i].PacketSize;
        rel = Lba % Vcb->TrackMap[i].PacketSize;
        KdPrint(("FixFPAddr: %x -> %x\n", Lba, pk*(Vcb->TrackMap[i].PacketSize+7) + rel));
        return pk*(Vcb->TrackMap[i].PacketSize+7) + rel /*- Vcb->TrackMap[i].PacketFPOffset*/;
    }
    return Lba;
} // end UDFFixFPAddress()

#endif //_BROWSE_UDF_

/*
    detect device driver & try to read disk layout (use all methods)
 */
OSSTATUS
UDFGetDiskInfo(
    IN PDEVICE_OBJECT DeviceObject,      // the target device object
    IN PVCB           Vcb                // Volume control block from this DevObj
    )
{
    OSSTATUS        RC = STATUS_UNRECOGNIZED_VOLUME;
    int8*           ioBuf = (int8*)MyAllocatePool__(NonPagedPool,4096);
    uint8 MediaType;
    PLUN_WRITE_PERF_DESC_USER WPerfDesc;
    uint32 i;
//    BOOLEAN MRW_problem = FALSE;
    uint32 SavedFeatures = 0;
#ifdef UDF_FORMAT_MEDIA
    PUDFFmtState            fms = Vcb->fms;
#else
  #define fms FALSE
#endif //UDF_FORMAT_MEDIA

    KdPrint(("UDFGetDiskInfo\n"));

    if(!ioBuf) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    _SEH2_TRY {
        RC = UDFGetBlockSize(DeviceObject, Vcb);
        if(!OS_SUCCESS(RC)) try_return(RC);
    
    
        // Get lower driver signature
        RC = UDFPhSendIOCTL(IOCTL_CDRW_GET_SIGNATURE,DeviceObject,
            ioBuf,sizeof(GET_SIGNATURE_USER_OUT),
            ioBuf,sizeof(GET_SIGNATURE_USER_OUT),
            TRUE,NULL);
    
        if(!OS_SUCCESS(RC)) {
    
            RC = UDFUseStandard(DeviceObject, Vcb);
#ifdef _BROWSE_UDF_
            if(!NT_SUCCESS(RC) || fms)
                try_return(RC);

            // assume Device Recordable for now
            goto GetSignatureFailed;
#endif //_BROWSE_UDF_
        }

        KdPrint(("UDF: Signature of low driver is : %s \n",
            ((PGET_SIGNATURE_USER_OUT)(ioBuf))->VendorId));
    
        if(!strncmp( (const char *)(&( ((PGET_SIGNATURE_USER_OUT)(ioBuf))->VendorId[0]) ),
            Signature,strlen(Signature) )) {
            KdPrint(("UDF: *****************************************\n"));
            KdPrint(("UDF: ********* Our Device Driver Found ******\n"));
            KdPrint(("UDF: *****************************************\n"));
    
            (Vcb->VCBFlags) |= UDF_VCB_FLAGS_OUR_DEVICE_DRIVER;
#ifndef _BROWSE_UDF_
            // reset driver
#ifdef UDF_FORMAT_MEDIA
            if(!fms->opt_probe) {
#endif //UDF_FORMAT_MEDIA
                UDFResetDeviceDriver(Vcb, Vcb->TargetDeviceObject, FALSE);
                // lock it
                ((PPREVENT_MEDIA_REMOVAL_USER_IN)(ioBuf))->PreventMediaRemoval = TRUE;
                UDFPhSendIOCTL( IOCTL_STORAGE_MEDIA_REMOVAL,
                                     DeviceObject,
                                     ioBuf,sizeof(PREVENT_MEDIA_REMOVAL_USER_IN),
                                     NULL,0,
                                     FALSE, NULL);
#ifdef UDF_FORMAT_MEDIA
            }
#endif //UDF_FORMAT_MEDIA
#endif //_BROWSE_UDF_
//#else //_BROWSE_UDF_
            // get device features
            UDFPhSendIOCTL( IOCTL_CDRW_GET_DEVICE_INFO,
                                 DeviceObject,
                                 NULL,0,
                                 ioBuf,sizeof(GET_DEVICE_INFO_USER_OUT),
                                 FALSE,NULL);

            Vcb->SavedFeatures =
                SavedFeatures = ((PGET_DEVICE_INFO_USER_OUT)ioBuf)->Features;
            if(!(SavedFeatures & CDRW_FEATURE_SYNC_ON_WRITE)) {
                KdPrint(("UDFGetDiskInfo: UDF_VCB_IC_NO_SYNCCACHE_AFTER_WRITE\n"));
                Vcb->CompatFlags |= UDF_VCB_IC_NO_SYNCCACHE_AFTER_WRITE;
            }
            if(!(SavedFeatures & CDRW_FEATURE_FORCE_SYNC_BEFORE_READ)) {
                KdPrint(("UDFGetDiskInfo: UDF_VCB_IC_SYNCCACHE_BEFORE_READ\n"));
                Vcb->CompatFlags |= UDF_VCB_IC_SYNCCACHE_BEFORE_READ;
            }
            if(SavedFeatures & CDRW_FEATURE_BAD_RW_SEEK) {
                KdPrint(("UDFGetDiskInfo: CDRW_FEATURE_BAD_RW_SEEK\n"));
                Vcb->CompatFlags |= UDF_VCB_IC_BAD_RW_SEEK;
            }
            // we must check if this is FP-formatted disk in old devices
            // independently of MediaType they report
            if(SavedFeatures & CDRW_FEATURE_FP_ADDRESSING_PROBLEM) {
                KdPrint(("UDFGetDiskInfo: CDRW_FEATURE_FP_ADDRESSING_PROBLEM ?\n"));
                Vcb->CompatFlags |= UDF_VCB_IC_FP_ADDR_PROBLEM;
            }
            if(SavedFeatures & CDRW_FEATURE_MRW_ADDRESSING_PROBLEM) {
                KdPrint(("UDFGetDiskInfo: CDRW_FEATURE_MRW_ADDRESSING_PROBLEM ?\n"));
            }
            if(SavedFeatures & CDRW_FEATURE_FORCE_SYNC_ON_WRITE) {
                KdPrint(("UDFGetDiskInfo: CDRW_FEATURE_FORCE_SYNC_ON_WRITE\n"));
                Vcb->VCBFlags |= UDF_VCB_FLAGS_FORCE_SYNC_CACHE;
            }
            if(SavedFeatures & CDRW_FEATURE_BAD_DVD_LAST_LBA) {
                KdPrint(("UDFGetDiskInfo: CDRW_FEATURE_BAD_DVD_LAST_LBA\n"));
                Vcb->CompatFlags |= UDF_VCB_IC_BAD_DVD_LAST_LBA;
            }
            if(SavedFeatures & CDRW_FEATURE_STREAMING) {
                KdPrint(("UDFGetDiskInfo: CDRW_FEATURE_STREAMING\n"));
            }
            if(SavedFeatures & CDRW_FEATURE_OPC) {
                KdPrint(("UDFGetDiskInfo: CDRW_FEATURE_OPC -> assume OPCNum=1\n"));
                Vcb->OPCNum = 1;
            }
#ifdef UDF_FORMAT_MEDIA
            if(SavedFeatures & CDRW_FEATURE_FULL_BLANK_ON_FORMAT) {
                KdPrint(("UDFGetDiskInfo: CDRW_FEATURE_FULL_BLANK_ON_FORMAT\n"));
                if((fms->opt_probe || fms->opt_smart_f)/* &&
                   (fms->format_media && fms->blank_media*/) {
                    KdPrint(("UDFGetDiskInfo: force Full Erase\n"));
                    fms->opt_qblank = FALSE;
                }
            }
#endif //UDF_FORMAT_MEDIA
#ifdef _BROWSE_UDF_
            // get device buffer size
            RC = UDFPhSendIOCTL( IOCTL_CDRW_BUFFER_CAPACITY,
                                 DeviceObject,
                                 NULL,0,
                                 ioBuf,sizeof(BUFFER_CAPACITY_BLOCK_USER_OUT),
                                 FALSE,NULL);
            if(NT_SUCCESS(RC)) {
                Vcb->CdrwBufferSize = ((PBUFFER_CAPACITY_BLOCK_USER_OUT)ioBuf)->BufferLength;
            } else {
                Vcb->CdrwBufferSize = 0;
            }
            KdPrint(("UDFGetDiskInfo: CdrwBufferSize = %dKb\n", Vcb->CdrwBufferSize / 1024));
            Vcb->CdrwBufferSizeCounter = 0;
#endif //_BROWSE_UDF_
            // get media type
            RC = UDFPhSendIOCTL(IOCTL_CDRW_GET_MEDIA_TYPE,DeviceObject,
                    NULL,0,ioBuf,sizeof(GET_MEDIA_TYPE_USER_OUT),
                    FALSE, NULL);
            if(!OS_SUCCESS(RC)) goto Try_FullToc;
            Vcb->MediaType =
            MediaType = ((PGET_MEDIA_TYPE_USER_OUT)ioBuf)->MediaType;
            KdPrint(("UDFGetDiskInfo: MediaType %x\n", MediaType));

#ifndef UDF_FORMAT_MEDIA
            // we shall ignore audio-disks
            switch(MediaType) {
            case MediaType_120mm_CDROM_AudioOnly:
            case MediaType_80mm_CDROM_AudioOnly:
            case MediaType_120mm_CDR_AudioOnly:
            case MediaType_80mm_CDR_AudioOnly:
            case MediaType_120mm_CDRW_AudioOnly:
            case MediaType_80mm_CDRW_AudioOnly:
//            case :
                KdPrint(("UDFGetDiskInfo: we shall ignore audio-disks...\n"));
                try_return(RC = STATUS_UNRECOGNIZED_VOLUME);
            }
#endif //UDF_FORMAT_MEDIA

            KdPrint(("UDFGetDiskInfo: Check DVD-disks...\n"));
            RC = UDFPhSendIOCTL(IOCTL_CDRW_GET_MEDIA_TYPE_EX,DeviceObject,
                    NULL,0,ioBuf,sizeof(GET_MEDIA_TYPE_EX_USER_OUT),
                    FALSE, NULL);
            if(!OS_SUCCESS(RC)) goto Try_FullToc;
            Vcb->MediaClassEx =
            MediaType = (((PGET_MEDIA_TYPE_EX_USER_OUT)ioBuf)->MediaClass);
            KdPrint(("UDFGetDiskInfo: MediaClassEx %x\n", MediaType));

#ifdef _BROWSE_UDF_
            if(!fms) {

                switch(MediaType) {

                case CdMediaClass_CDR:
                case CdMediaClass_DVDR:
                case CdMediaClass_DVDpR:
                case CdMediaClass_HD_DVDR:
                case CdMediaClass_BDR:
                    KdPrint(("UDFGetDiskInfo: MediaClass R\n"));
                    Vcb->MediaType = MediaType_UnknownSize_CDR;
                    break;
                case CdMediaClass_CDRW:

                    if(SavedFeatures & CDRW_FEATURE_MRW_ADDRESSING_PROBLEM) {
                        KdPrint(("UDFGetDiskInfo: CDRW_FEATURE_MRW_ADDRESSING_PROBLEM on CD-RW\n"));
                        Vcb->CompatFlags |= UDF_VCB_IC_MRW_ADDR_PROBLEM;
                    }

                case CdMediaClass_DVDRW:
                case CdMediaClass_DVDpRW:
                case CdMediaClass_DVDRAM:
                case CdMediaClass_HD_DVDRW:
                case CdMediaClass_HD_DVDRAM:
                case CdMediaClass_BDRE:
                    KdPrint(("UDFGetDiskInfo: MediaClass RW\n"));
                    Vcb->MediaType = MediaType_UnknownSize_CDRW;
                    break;
                case CdMediaClass_CDROM:
                case CdMediaClass_DVDROM:
                case CdMediaClass_HD_DVDROM:
                case CdMediaClass_BDROM:
                    KdPrint(("UDFGetDiskInfo: MediaClass ROM\n"));
                    Vcb->MediaType = MediaType_Unknown;
    //                    Vcb->MediaType = MediaType_UnknownSize_CDROM;
                    break;
                default:
                    KdPrint(("UDFGetDiskInfo: MediaClass Unknown\n"));
                    Vcb->MediaType = MediaType_Unknown;
                    break;
                }
                MediaType = Vcb->MediaType;

            }
#endif //_BROWSE_UDF_

#ifdef UDF_FORMAT_MEDIA

            if(fms) {

                switch(MediaType) {
                case CdMediaClass_CDR:
                    KdPrint(("CdMediaClass_CDR\n"));
                    MediaType = MediaType_UnknownSize_CDR;
                    if(fms->opt_media == MT_AUTO)
                        fms->opt_media = MT_CDR;
                    break;
                case CdMediaClass_DVDR:
                    KdPrint(("CdMediaClass_DVDR -> MediaType_UnknownSize_CDR\n"));
                    MediaType = MediaType_UnknownSize_CDR;
                    if(fms->opt_media == MT_AUTO)
                        fms->opt_media = MT_DVDR;
                    break;
                case CdMediaClass_DVDpR:
                    KdPrint(("CdMediaClass_DVDpR -> MediaType_UnknownSize_CDR\n"));
                    MediaType = MediaType_UnknownSize_CDR;
                    if(fms->opt_media == MT_AUTO)
                        fms->opt_media = MT_DVDpR;
                    break;
                case CdMediaClass_HD_DVDR:
                    KdPrint(("CdMediaClass_HD_DVDR -> MediaType_UnknownSize_CDR\n"));
                    MediaType = MediaType_UnknownSize_CDR;
                    if(fms->opt_media == MT_AUTO)
                        fms->opt_media = MT_DVDR;
                    break;
                case CdMediaClass_BDR:
                    KdPrint(("CdMediaClass_BDR -> MediaType_UnknownSize_CDR\n"));
                    MediaType = MediaType_UnknownSize_CDR;
                    if(fms->opt_media == MT_AUTO)
                        fms->opt_media = MT_DVDR;
                    break;
                case CdMediaClass_CDRW:
                    KdPrint(("CdMediaClass_CDRW\n"));
                    MediaType = MediaType_UnknownSize_CDRW;
                    if(fms->opt_media == MT_AUTO)
                        fms->opt_media = MT_CDRW;
                    if(SavedFeatures & CDRW_FEATURE_MRW_ADDRESSING_PROBLEM) {
                        KdPrint(("UDFGetDiskInfo: CDRW_FEATURE_MRW_ADDRESSING_PROBLEM on CD-RW\n"));
                        Vcb->CompatFlags |= UDF_VCB_IC_MRW_ADDR_PROBLEM;
                    }
                    break;
                case CdMediaClass_DVDRW:
                    KdPrint(("  CdMediaClass_DVDRW -> MediaType_UnknownSize_CDRW\n"));
                    if(fms->opt_media == MT_AUTO)
                        fms->opt_media = MT_DVDRW;
                    MediaType = MediaType_UnknownSize_CDRW;
                    break;
                case CdMediaClass_DVDpRW:
                    KdPrint(("  CdMediaClass_DVDpRW -> MediaType_UnknownSize_CDRW\n"));
                    if(fms->opt_media == MT_AUTO)
                        fms->opt_media = MT_DVDpRW;
                    MediaType = MediaType_UnknownSize_CDRW;
                    break;
                case CdMediaClass_DVDRAM:
                    KdPrint(("  CdMediaClass_DVDRAM -> MediaType_UnknownSize_CDRW\n"));
                    if(fms->opt_media == MT_AUTO)
                        fms->opt_media = MT_DVDRAM;
                    MediaType = MediaType_UnknownSize_CDRW;
                    break;
                case CdMediaClass_HD_DVDRW:
                    KdPrint(("  CdMediaClass_HD_DVDRW -> MediaType_UnknownSize_CDRW\n"));
                    if(fms->opt_media == MT_AUTO)
                        fms->opt_media = MT_DVDRW;
                    MediaType = MediaType_UnknownSize_CDRW;
                    break;
                case CdMediaClass_HD_DVDRAM:
                    KdPrint(("  CdMediaClass_HD_DVDRAM -> MediaType_UnknownSize_CDRW\n"));
                    if(fms->opt_media == MT_AUTO)
                        fms->opt_media = MT_DVDRAM;
                    MediaType = MediaType_UnknownSize_CDRW;
                    break;
                case CdMediaClass_BDRE:
                    KdPrint(("  CdMediaClass_BDRE -> MediaType_UnknownSize_CDRW\n"));
                    if(fms->opt_media == MT_AUTO)
                        fms->opt_media = MT_DVDRW;
                    MediaType = MediaType_UnknownSize_CDRW;
                    break;
                case CdMediaClass_NoDiscPresent:
                    KdPrint(("  CdMediaClass_NoDiscPresent -> MediaType_NoDiscPresent\n"));
                    MediaType = MediaType_NoDiscPresent;
                    fms->opt_media = MT_none;
                    break;
                case CdMediaClass_DoorOpen:
                    KdPrint(("  CdMediaClass_DoorOpen -> MediaType_DoorOpen\n"));
                    MediaType = MediaType_DoorOpen;
                    fms->opt_media = MT_none;
                    break;
                default:
                    KdPrint(("  MediaType_Unknown\n"));
                    MediaType = MediaType_Unknown;
                    break;
                }
                if(!apply_force_r(fms)) {
                    my_exit(fms, MKUDF_CANT_APPLY_R);
                }
            }

#endif //UDF_FORMAT_MEDIA

            Vcb->DVD_Mode = (((PGET_MEDIA_TYPE_EX_USER_OUT)ioBuf)->MediaClassEx == CdMediaClassEx_DVD);
            Vcb->PhMediaCapFlags = ((PGET_MEDIA_TYPE_EX_USER_OUT)ioBuf)->CapFlags;
            Vcb->WriteParamsReq = (Vcb->PhMediaCapFlags & CdCapFlags_WriteParamsReq) ? TRUE : FALSE;
            if(Vcb->DVD_Mode &&
                !(Vcb->PhMediaCapFlags & CdCapFlags_RandomWritable)) {
                KdPrint(("UDFGetDiskInfo: DVD && !CdCapFlags_RandomWritable\n"));
                KdPrint(("  Read-only volume\n"));
//                BrutePoint();
#ifndef UDF_CDRW_EMULATION_ON_ROM
                Vcb->VCBFlags |= UDF_VCB_FLAGS_VOLUME_READ_ONLY;
#endif
            }
#ifdef UDF_FORMAT_MEDIA
            if(fms) {
                if((MediaType == MediaType_NoDiscPresent) ||
                   (MediaType == MediaType_DoorOpen)) {
                    UserPrint(("No media in device\n"));
                    my_exit(fms, MKUDF_NO_MEDIA_IN_DEVICE);
                }
            }
#endif //UDF_FORMAT_MEDIA
            if(!Vcb->WriteParamsReq) {
                KdPrint(("UDFGetDiskInfo: do not use WriteParams\n"));
            }
            if(Vcb->PhMediaCapFlags & CdCapFlags_Cav) {
                KdPrint(("UDFGetDiskInfo: Use CAV (1)\n"));
                Vcb->VCBFlags |= UDF_VCB_FLAGS_USE_CAV;
            }

#ifdef _BROWSE_UDF_
            if(!fms) {
                // check if this device is capable to write on such media
                if(UDFIsDvdMedia(Vcb)) {
                    //RC =
                    KdPrint(("UDFGetDiskInfo: update defaulted LastLBA\n"));
                    UDFGetBlockSize(DeviceObject,Vcb);
                    //if(!OS_SUCCESS(RC)) goto Try_FullToc;
                } else {
                    if((SavedFeatures & CDRW_FEATURE_MRW_ADDRESSING_PROBLEM) &&
                       (SavedFeatures & UDF_VCB_IC_FP_ADDR_PROBLEM)) {
                        KdPrint(("UDFGetDiskInfo: CDRW_FEATURE_MRW_ADDRESSING_PROBLEM on old CD-ROM\n"));
                        Vcb->CompatFlags |= UDF_VCB_IC_MRW_ADDR_PROBLEM;
                    }
                }
            }
#endif //_BROWSE_UDF_

/*#ifdef UDF_FORMAT_MEDIA
            if(fms) {
                if(MediaType == CdMediaClass_DVDRW) {
                    UserPrint(("Not empty media. Erase required.\n"));
                    my_exit(fms, MKUDF_BLANK_FORMAT_REQUIRED);
                }
            }
#endif //UDF_FORMAT_MEDIA*/

#define cap ((PGET_CAPABILITIES_3_USER_OUT)ioBuf)
            // get device capabilities
            RC = UDFPhSendIOCTL(IOCTL_CDRW_GET_CAPABILITIES,DeviceObject,
                    NULL,0,ioBuf,sizeof(GET_CAPABILITIES_3_USER_OUT),
                    FALSE, NULL);
            if(!OS_SUCCESS(RC)) goto Try_FullToc;

            // check if this device is capable to write on such media
            RC = UDFPhSendIOCTL(IOCTL_DISK_IS_WRITABLE,DeviceObject,
                    NULL,0,NULL,0,FALSE, NULL);
            if(RC != STATUS_SUCCESS) {
                KdPrint(("IS_WRITABLE - false, doing additional check...\n"));
                if( ((MediaType >= MediaType_UnknownSize_CDRW) && !(cap->WriteCap & DevCap_write_cd_rw)) ||
                    ((MediaType >= MediaType_UnknownSize_CDR) && !(cap->WriteCap & DevCap_write_cd_r)) ||
                     (MediaType < MediaType_UnknownSize_CDR) ) {
                    UserPrint(("Hardware Read-only volume\n"));
#ifndef UDF_CDRW_EMULATION_ON_ROM
                    Vcb->VCBFlags |= UDF_VCB_FLAGS_VOLUME_READ_ONLY;
#endif  //UDF_CDRW_EMULATION_ON_ROM
#ifdef UDF_FORMAT_MEDIA
                    if(fms && !fms->opt_read_iso)
                        my_exit(fms, MKUDF_HW_READ_ONLY);
#endif //UDF_FORMAT_MEDIA
                }
            } else {
                KdPrint(("Writable disk\n"));
            }
            Vcb->MaxWriteSpeed = cap->MaximumWriteSpeedSupported;
            Vcb->MaxReadSpeed  = cap->MaximumSpeedSupported;
            if(cap->PageLength >= (sizeof(GET_CAPABILITIES_3_USER_OUT)-2)) {
                Vcb->CurSpeed = max(cap->CurrentSpeed, cap->CurrentWriteSpeed3);
                if(cap->LunWPerfDescriptorCount && cap->LunWPerfDescriptorCount != 0xffff) {
                    ULONG n;
                    KdPrint(("Write performance descriptor(s) found: %x\n", cap->LunWPerfDescriptorCount));
                    n = (4096 - sizeof(GET_CAPABILITIES_3_USER_OUT)) / sizeof(LUN_WRITE_PERF_DESC_USER);
                    n = min(n, cap->LunWPerfDescriptorCount);
                    // get device capabilities
                    RC = UDFPhSendIOCTL(IOCTL_CDRW_GET_CAPABILITIES,DeviceObject,
                            ioBuf,sizeof(GET_CAPABILITIES_3_USER_OUT)+n*sizeof(LUN_WRITE_PERF_DESC_USER),
                            ioBuf,sizeof(GET_CAPABILITIES_3_USER_OUT)+n*sizeof(LUN_WRITE_PERF_DESC_USER),
                            TRUE,NULL);
                    if(OS_SUCCESS(RC)) {
                        WPerfDesc = (PLUN_WRITE_PERF_DESC_USER)(ioBuf + sizeof(GET_CAPABILITIES_3_USER_OUT));
                        n = FALSE;
                        for(i = 0; i<n; i++) {
                            if((WPerfDesc[i].RotationControl & LunWPerf_RotCtrl_Mask) == LunWPerf_RotCtrl_CAV) {
                                Vcb->VCBFlags |= UDF_VCB_FLAGS_USE_CAV;
                                if(!n) {
                                    Vcb->CurSpeed = WPerfDesc[i].WriteSpeedSupported;
                                    n = TRUE;
                                    KdPrint(("Use CAV\n"));
                                } else {
                                    Vcb->CurSpeed = max(WPerfDesc[i].WriteSpeedSupported, Vcb->CurSpeed);
                                }
                                KdPrint(("supports speed %dX\n", Vcb->CurSpeed/176));
                                //break;
                            }
                        }
                        if(n) {
                            KdPrint(("Set r/w speeds to %dX\n", Vcb->CurSpeed/176));
                            Vcb->MaxWriteSpeed =
                            Vcb->MaxReadSpeed  = Vcb->CurSpeed;
                        }
                    }
                }
            } else {
                Vcb->CurSpeed = max(cap->CurrentSpeed, cap->CurrentWriteSpeed);
            }
            KdPrint((" Speeds r/w %dX/%dX\n", Vcb->CurSpeed/176, cap->CurrentWriteSpeed/176));

            if(Vcb->VCBFlags & UDF_VCB_FLAGS_USE_CAV) {
                // limit both read & write speed to last write speed for CAV mode
                // some drives damage data when speed is adjusted during recording process
                // even in packet mode
                UDFSetSpeeds(Vcb);
            }
            UDFSetCaching(Vcb);

#undef cap
#ifdef UDF_FORMAT_MEDIA
            if(fms) {
                if( (fms->auto_media || (fms->opt_media == MT_AUTO)) &&
                       (fms->opt_media < MT_DVDR) ) {
                    if(MediaType < MediaType_UnknownSize_CDRW) {
                        fms->opt_media = MT_CDR;
                    } else {
                        fms->opt_media = MT_CDRW;
                    }
                }
                if(!apply_force_r(fms)) {
                    my_exit(fms, MKUDF_CANT_APPLY_R);
                }
            }
#endif //UDF_FORMAT_MEDIA
            RC = UDFReadDiscTrackInfo(DeviceObject, Vcb);

            if(!OS_SUCCESS(RC)) {
                // may be we have a CD-ROM device
Try_FullToc:
                KdPrint(("Hardware Read-only volume (2)\n"));
//                BrutePoint();
#ifndef UDF_CDRW_EMULATION_ON_ROM
                Vcb->VCBFlags |= UDF_VCB_FLAGS_VOLUME_READ_ONLY;
#endif

                RC = UDFReadAndProcessFullToc(DeviceObject, Vcb);
    
                if(!OS_SUCCESS(RC)) {
                    RC = UDFUseStandard(DeviceObject,Vcb);
                    if(!OS_SUCCESS(RC)) try_return(RC);
                }
    
            }
        } else {
#ifdef _BROWSE_UDF_
GetSignatureFailed:
#endif
            RC = UDFUseStandard(DeviceObject, Vcb);
            if(!OS_SUCCESS(RC)) try_return(RC);
        }
    
try_exit:   NOTHING;

    } _SEH2_FINALLY {

        if(ioBuf) MyFreePool__(ioBuf);

        if(UDFIsDvdMedia(Vcb) &&
           (Vcb->CompatFlags & UDF_VCB_IC_BAD_DVD_LAST_LBA) &&
           (Vcb->VCBFlags & UDF_VCB_FLAGS_VOLUME_READ_ONLY) &&
            Vcb->LastLBA &&
           (Vcb->LastLBA < DEFAULT_LAST_LBA_DVD)) {
            KdPrint(("UDF: Bad DVD last LBA %x, fixup!\n", Vcb->LastLBA));
            Vcb->LastLBA = DEFAULT_LAST_LBA_DVD;
            Vcb->NWA = 0;
        }


        if(UDFIsDvdMedia(Vcb) && !Vcb->FirstLBA && !Vcb->LastPossibleLBA) {
            KdPrint(("UDF: Empty DVD. Use bogus values for now\n"));
            Vcb->LastPossibleLBA = DEFAULT_LAST_LBA_DVD;
            Vcb->LastLBA = 0;
        }
        
        if((Vcb->LastPossibleLBA & 0x80000000) || (Vcb->LastPossibleLBA < Vcb->LastLBA)) {
            KdPrint(("UDF: bad LastPossibleLBA %x -> %x\n", Vcb->LastPossibleLBA, Vcb->LastLBA));
            Vcb->LastPossibleLBA = Vcb->LastLBA;
        }
        if(!Vcb->WriteBlockSize)
            Vcb->WriteBlockSize = PACKETSIZE_UDF*Vcb->BlockSize;

#ifdef _BROWSE_UDF_
        if(Vcb->TrackMap) {
            if(Vcb->TrackMap[Vcb->LastTrackNum].LastLba > Vcb->NWA) {
                if(Vcb->NWA) {
                    if(Vcb->TrackMap[Vcb->LastTrackNum].DataParam & TrkInfo_FP) {
                        Vcb->LastLBA = Vcb->NWA-1;
                    } else {
                        Vcb->LastLBA = Vcb->NWA-7-1;
                    }
                }
            } else {
                if((Vcb->LastTrackNum > 1) &&
                   (Vcb->TrackMap[Vcb->LastTrackNum-1].FirstLba >= Vcb->TrackMap[Vcb->LastTrackNum-1].LastLba)) {
                    Vcb->LastLBA = Vcb->TrackMap[Vcb->LastTrackNum-1].LastLba;
                }
            }
        }

        for(i=0; i<32; i++) {
            if(!(Vcb->LastPossibleLBA >> i))
                break;
        }
        if(i > 20) {
            Vcb->WCacheBlocksPerFrameSh = max(Vcb->WCacheBlocksPerFrameSh, (2*i)/5+2);
            Vcb->WCacheBlocksPerFrameSh = min(Vcb->WCacheBlocksPerFrameSh, 16);
        }

        if(Vcb->CompatFlags & UDF_VCB_IC_FP_ADDR_PROBLEM) {
            // Check first 0x200 blocks
            UDFCheckTrackFPAddressing(Vcb, Vcb->FirstTrackNum);
            // if we really have such a problem, fix LastLBA
            if(Vcb->CompatFlags & UDF_VCB_IC_FP_ADDR_PROBLEM) {
                KdPrint(("UDF: Fix LastLBA: %x -> %x\n", Vcb->LastLBA, (Vcb->LastLBA*32) / 39));
                Vcb->LastLBA = (Vcb->LastLBA*32) / 39;
            }
        }
#endif //_BROWSE_UDF_

        if(Vcb->VCBFlags & UDF_VCB_FLAGS_VOLUME_READ_ONLY) {
            if(!Vcb->BlankCD && Vcb->MediaType != MediaType_UnknownSize_CDRW) {
                KdPrint(("UDFGetDiskInfo: R/O+!Blank+!RW -> !RAW\n"));
                Vcb->VCBFlags &= ~UDF_VCB_FLAGS_RAW_DISK;
            } else {
                KdPrint(("UDFGetDiskInfo: Blank or RW\n"));
            }
        }

        KdPrint(("UDF: ------------------------------------------\n"));
        KdPrint(("UDF: Media characteristics\n"));
        KdPrint(("UDF: Last session: %d\n",Vcb->LastSession));
        KdPrint(("UDF: First track in first session: %d\n",Vcb->FirstTrackNum));
        KdPrint(("UDF: First track in last session: %d\n",Vcb->FirstTrackNumLastSes));
        KdPrint(("UDF: Last track in last session: %d\n",Vcb->LastTrackNum));
        KdPrint(("UDF: First LBA in first session: %x\n",Vcb->FirstLBA));
        KdPrint(("UDF: First LBA in last session: %x\n",Vcb->FirstLBALastSes));
        KdPrint(("UDF: Last LBA in last session: %x\n",Vcb->LastLBA));
        KdPrint(("UDF: First writable LBA (NWA) in last session: %x\n",Vcb->NWA));
        KdPrint(("UDF: Last available LBA beyond end of last session: %x\n",Vcb->LastPossibleLBA));
        KdPrint(("UDF: blocks per frame: %x\n",1 << Vcb->WCacheBlocksPerFrameSh));
        KdPrint(("UDF: Flags: %s%s\n",
                 Vcb->VCBFlags & UDF_VCB_FLAGS_RAW_DISK ? "RAW " : "",
                 Vcb->VCBFlags & UDF_VCB_FLAGS_VOLUME_READ_ONLY ? "R/O " : "WR "
                 ));
        KdPrint(("UDF: ------------------------------------------\n"));

#ifdef UDF_FORMAT_MEDIA
        if(fms && fms->opt_disk_info) {
            UserPrint(("------------------------------------------\n"));
            UserPrint(("Media characteristics:\n"));
            UserPrint(("  First writable LBA (NWA) in last session: %x\n",Vcb->NWA));
            UserPrint(("  Last available LBA beyond end of last session: %x\n",Vcb->LastPossibleLBA));
            UserPrint(("------------------------------------------\n"));
        }
#endif //UDF_FORMAT_MEDIA

    } _SEH2_END;

    KdPrint(("UDFGetDiskInfo: %x\n", RC));
    return(RC);

} // end UDFGetDiskInfo()

//#ifdef _BROWSE_UDF_

OSSTATUS
UDFPrepareForReadOperation(
    IN PVCB Vcb,
    IN uint32 Lba,
    IN uint32 BCount
    )
{
    if( (Vcb->FsDeviceType != FILE_DEVICE_CD_ROM_FILE_SYSTEM) ) {
        Vcb->VCBFlags &= ~UDF_VCB_LAST_WRITE;
        return STATUS_SUCCESS;
    }
    uint32 i = Vcb->LastReadTrack;
    BOOLEAN speed_changed = FALSE;
#ifdef _BROWSE_UDF_
    PUCHAR tmp;
    OSSTATUS RC;
    ULONG ReadBytes;
#endif //_BROWSE_UDF_

#ifdef _UDF_STRUCTURES_H_
    if(Vcb->BSBM_Bitmap) {
        ULONG i;
        for(i=0; i<BCount; i++) {
            if(UDFGetBit__((uint32*)(Vcb->BSBM_Bitmap), Lba+i)) {
                KdPrint(("R: Known BB @ %#x\n", Lba));
                //return STATUS_FT_WRITE_RECOVERY; // this shall not be treated as error and
                                                   // we shall get IO request to BAD block
                return STATUS_DEVICE_DATA_ERROR;
            }
        }
    }
#endif //_UDF_STRUCTURES_H_

    if(!UDFIsDvdMedia(Vcb) &&
        (Vcb->VCBFlags & UDF_VCB_LAST_WRITE) &&
       !(Vcb->VCBFlags & UDF_VCB_FLAGS_NO_SYNC_CACHE) &&
       !(Vcb->CompatFlags & UDF_VCB_IC_NO_SYNCCACHE_AFTER_WRITE)){

        OSSTATUS RC;

        RC = UDFSyncCache(Vcb);
    }
    if( (Vcb->VCBFlags & UDF_VCB_LAST_WRITE) &&
       !(Vcb->VCBFlags & UDF_VCB_FLAGS_NO_SYNC_CACHE) &&
#ifndef UDF_FORMAT_MEDIA
        (Vcb->CompatFlags & UDF_VCB_IC_SYNCCACHE_BEFORE_READ) &&
#endif //UDF_FORMAT_MEDIA
        TRUE)
    {
        OSSTATUS RC;
        RC = UDFSyncCache(Vcb);
    }

#ifdef _BROWSE_UDF_
    if(!UDFIsDvdMedia(Vcb)) {
        // limit read speed after write operation
        // to avoid performance degrade durring speed-up/down
        // on read/write mode switching
        if(Vcb->VCBFlags & UDF_VCB_FLAGS_USE_CAV) {
            // limit both read & write speed to last write speed for CAV mode
            // some drives damage data when speed is adjusted during recording process
            // even in packet mode
            if(Vcb->CurSpeed != Vcb->MaxWriteSpeed ||
               Vcb->CurSpeed != Vcb->MaxReadSpeed) {
                Vcb->CurSpeed = Vcb->MaxWriteSpeed;
                speed_changed = TRUE;
            }
        } else
        if(Vcb->VCBFlags & UDF_VCB_LAST_WRITE) {
            // limit read speed to last write speed
            if(Vcb->CurSpeed > Vcb->MaxWriteSpeed) {
                Vcb->CurSpeed = Vcb->MaxWriteSpeed;
                speed_changed = TRUE;
            }
        } else
        if(Vcb->CurSpeed < Vcb->MaxReadSpeed ) {
            // increment read speed (+1X)
            Vcb->CurSpeed += 176/1;
            speed_changed = TRUE;
        }

        if(Vcb->CurSpeed > Vcb->MaxReadSpeed) {
            Vcb->CurSpeed = Vcb->MaxReadSpeed;
        }
        // send speed limits to drive
        if(speed_changed) {
            RtlZeroMemory(&(Vcb->SpeedBuf), sizeof(SET_CD_SPEED_EX_USER_IN));
            Vcb->SpeedBuf.ReadSpeed  = Vcb->CurSpeed;
            Vcb->SpeedBuf.WriteSpeed = Vcb->MaxWriteSpeed;
            if(Vcb->VCBFlags & UDF_VCB_FLAGS_USE_CAV) {
                Vcb->SpeedBuf.RotCtrl = CdSpeed_RotCtrl_CAV;
            }
            KdPrint(("    UDFPrepareForReadOperation: set speed to %s %dX/%dX\n",
                (Vcb->VCBFlags & UDF_VCB_FLAGS_USE_CAV) ? "CAV" : "CLV",
                Vcb->SpeedBuf.ReadSpeed,
                Vcb->SpeedBuf.WriteSpeed));
            UDFPhSendIOCTL(IOCTL_CDRW_SET_SPEED,
                                Vcb->TargetDeviceObject,
                                &(Vcb->SpeedBuf),sizeof(SET_CD_SPEED_EX_USER_IN),
                                NULL,0,TRUE,NULL);
        }
    }

    if(UDFIsDvdMedia(Vcb))
        return STATUS_SUCCESS;

    if(Vcb->LastReadTrack &&
       ((Vcb->TrackMap[i].FirstLba <= Lba) || (Vcb->TrackMap[i].FirstLba & 0x80000000)) &&
       (Vcb->TrackMap[i].LastLba >= Lba)) {
check_for_data_track:
        // check track mode (Mode1/XA)
        switch((Vcb->TrackMap[i].DataParam & TrkInfo_Dat_Mask)) {
        case TrkInfo_Dat_Mode1: // Mode1
        case TrkInfo_Dat_XA:    // XA Mode2
        case TrkInfo_Dat_Unknown: // for some stupid irons
            UDFSetMRWMode(Vcb);
            break;
        default:
            Vcb->IncrementalSeekState = INCREMENTAL_SEEK_NONE;
            return STATUS_INVALID_PARAMETER;
        }
    } else {
        for(i=Vcb->FirstTrackNum; i<=Vcb->LastTrackNum; i++) {
            if(((Vcb->TrackMap[i].FirstLba > Lba) && !(Vcb->TrackMap[i].FirstLba & 0x80000000)) ||
               (Vcb->TrackMap[i].LastLba < Lba))
                continue;
            Vcb->LastReadTrack = i;
            goto check_for_data_track;
        }
        Vcb->LastReadTrack = 0;
    }
    if(Vcb->IncrementalSeekState != INCREMENTAL_SEEK_WORKAROUND) {
        Vcb->IncrementalSeekState = INCREMENTAL_SEEK_NONE;
        return STATUS_SUCCESS;
    }
    KdPrint(("    UDFPrepareForReadOperation: seek workaround...\n"));
    Vcb->IncrementalSeekState = INCREMENTAL_SEEK_DONE;

    tmp = (PUCHAR)DbgAllocatePoolWithTag(NonPagedPool, Vcb->BlockSize, 'bNWD');
    if(!tmp) {
        Vcb->IncrementalSeekState = INCREMENTAL_SEEK_NONE;
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    for(i=0x1000; i<=Lba; i+=0x1000) {
        RC = UDFPhReadSynchronous(Vcb->TargetDeviceObject, tmp, Vcb->BlockSize,
                   ((uint64)UDFFixFPAddress(Vcb,i)) << Vcb->BlockSizeBits, &ReadBytes, 0);
        KdPrint(("    seek workaround, LBA %x, status %x\n", i, RC));
    }
    DbgFreePool(tmp);
#endif //_BROWSE_UDF_

    return STATUS_SUCCESS;
} // end UDFPrepareForReadOperation()

//#endif //_BROWSE_UDF_

void
UDFUpdateNWA(
    PVCB Vcb,
    uint32 LBA,  // physical
    uint32 BCount,
    OSSTATUS RC
    )
{
#ifndef UDF_READ_ONLY_BUILD
#ifdef _BROWSE_UDF_
    if(!OS_SUCCESS(RC)) {
        return;
    }
    if(!Vcb->CDR_Mode) {

        if((Vcb->MediaClassEx == CdMediaClass_DVDRW ||
            Vcb->MediaClassEx == CdMediaClass_DVDpRW ||
            Vcb->MediaClassEx == CdMediaClass_DVDRAM ||
            Vcb->MRWStatus == DiscInfo_BGF_Interrupted ||
            Vcb->MRWStatus == DiscInfo_BGF_InProgress)
              && (LBA+BCount-1) > Vcb->LastLBA) {
            ASSERT(Vcb->NWA > Vcb->LastLBA);
            Vcb->NWA = LBA+BCount;
            Vcb->LastLBA = Vcb->NWA-1;
        }
        if(Vcb->VCBFlags & UDF_VCB_FLAGS_FORCE_SYNC_CACHE)
            goto sync_cache;
/*        if(Vcb->CdrwBufferSize) {
            Vcb->CdrwBufferSizeCounter += BCount * 2048;
            if(Vcb->CdrwBufferSizeCounter >= Vcb->CdrwBufferSize + 2*2048) {
                KdPrint(("    UDFUpdateNWA: buffer is full, sync...\n"));
                Vcb->CdrwBufferSizeCounter = 0;
                goto sync_cache;
            }
        }*/
        if(Vcb->SyncCacheState == SYNC_CACHE_RECOVERY_RETRY) {
            Vcb->VCBFlags |= UDF_VCB_FLAGS_FORCE_SYNC_CACHE;
        }
        Vcb->SyncCacheState = SYNC_CACHE_RECOVERY_NONE;
        return;
    }
    if(Vcb->LastLBA < (LBA+BCount))
        Vcb->LastLBA = LBA+BCount;
    if(Vcb->NWA)
        Vcb->NWA+=BCount+7;
sync_cache:
    if(!(Vcb->CompatFlags & UDF_VCB_IC_NO_SYNCCACHE_AFTER_WRITE)) {
        KdPrint(("    UDFUpdateNWA: syncing...\n"));
        RC = UDFSyncCache(Vcb);
    }
#endif //_BROWSE_UDF_
#endif //UDF_READ_ONLY_BUILD
} // end UDFUpdateNWA()


/*
    This routine reads physical sectors
 */
/*OSSTATUS
UDFReadSectors(
    IN PVCB Vcb,
    IN BOOLEAN Translate,       // Translate Logical to Physical
    IN uint32 Lba,
    IN uint32 BCount,
    OUT int8* Buffer,
    OUT uint32* ReadBytes
    )
{

    if(Vcb->FastCache.ReadProc && (KeGetCurrentIrql() < DISPATCH_LEVEL)) {
        return WCacheReadBlocks__(&(Vcb->FastCache), Vcb, Buffer, Lba, BCount, ReadBytes);
    }
    return UDFTRead(Vcb, Buffer, BCount*Vcb->BlockSize, Lba, ReadBytes);
} // end UDFReadSectors()*/

#ifdef _BROWSE_UDF_

/*
    This routine reads physical sectors
 */
OSSTATUS
UDFReadInSector(
    IN PVCB Vcb,
    IN BOOLEAN Translate,       // Translate Logical to Physical
    IN uint32 Lba,
    IN uint32 i,                 // offset in sector
    IN uint32 l,                 // transfer length
    IN BOOLEAN Direct,          // Disable access to non-cached data
    OUT int8* Buffer,
    OUT uint32* ReadBytes
    )
{
    int8* tmp_buff;
    OSSTATUS status;
    uint32 _ReadBytes;

    (*ReadBytes) = 0;
    if(WCacheIsInitialized__(&(Vcb->FastCache)) && (KeGetCurrentIrql() < DISPATCH_LEVEL)) {
        status = WCacheDirect__(&(Vcb->FastCache), Vcb, Lba, FALSE, &tmp_buff, Direct);
        if(OS_SUCCESS(status)) {
            (*ReadBytes) += l;
            RtlCopyMemory(Buffer, tmp_buff+i, l);
        }
        if(!Direct) WCacheEODirect__(&(Vcb->FastCache), Vcb);
    } else {
        if(Direct) {
            return STATUS_INVALID_PARAMETER;
        }
        tmp_buff = (int8*)MyAllocatePool__(NonPagedPool, Vcb->BlockSize);
        if(!tmp_buff) return STATUS_INSUFFICIENT_RESOURCES;
        status = UDFReadSectors(Vcb, Translate, Lba, 1, FALSE, tmp_buff, &_ReadBytes);
        if(OS_SUCCESS(status)) {
            (*ReadBytes) += l;
            RtlCopyMemory(Buffer, tmp_buff+i, l);
        }
        MyFreePool__(tmp_buff);
    }
    return status;
} // end UDFReadInSector()

/*
    This routine reads data of unaligned offset & length
 */
OSSTATUS
UDFReadData(
    IN PVCB Vcb,
    IN BOOLEAN Translate,       // Translate Logical to Physical
    IN int64 Offset,
    IN uint32 Length,
    IN BOOLEAN Direct,          // Disable access to non-cached data
    OUT int8* Buffer,
    OUT uint32* ReadBytes
    )
{
    uint32 i, l, Lba, BS=Vcb->BlockSize;
    uint32 BSh=Vcb->BlockSizeBits;
    OSSTATUS status;
    uint32 _ReadBytes = 0;
    Vcb->VCBFlags |= UDF_VCB_SKIP_EJECT_CHECK;
    uint32 to_read;

    (*ReadBytes) = 0;
    if(!Length) return STATUS_SUCCESS;
    if(Vcb->VCBFlags & UDF_VCB_FLAGS_DEAD)
        return STATUS_NO_SUCH_DEVICE;
    // read tail of the 1st sector if Offset is not sector_size-aligned
    Lba = (uint32)(Offset >> BSh);
    if(i = (uint32)(Offset & (BS-1))) {
        l = (BS - i) < Length ?
            (BS - i) : Length;
        // here we use 'ReadBytes' 'cause now it's set to zero
        status = UDFReadInSector(Vcb, Translate, Lba, i, l, Direct, Buffer, ReadBytes);
        if(!OS_SUCCESS(status)) return status;
        if(!(Length = Length - l)) return STATUS_SUCCESS;
        Lba ++;
        Buffer += l;
    }
    // read sector_size-aligned part
    i = Length >> BSh;
    while(i) {
        to_read = min(i, 64);
        status = UDFReadSectors(Vcb, Translate, Lba, to_read, Direct, Buffer, &_ReadBytes);
        (*ReadBytes) += _ReadBytes;
        if(!OS_SUCCESS(status)) {
            return status;
        }
        Buffer += to_read<<BSh;
        Length -= to_read<<BSh;
        Lba += to_read;
        i -= to_read;
    }
    // read head of the last sector
    if(!Length) return STATUS_SUCCESS;
    status = UDFReadInSector(Vcb, Translate, Lba, 0, Length, Direct, Buffer, &_ReadBytes);
    (*ReadBytes) += _ReadBytes;

    return status;
} // end UDFReadData()

#endif //_BROWSE_UDF_

#ifndef UDF_READ_ONLY_BUILD
/*
    This routine writes physical sectors. This routine supposes Lba & Length
    alignment on WriteBlock (packet) size.
 */
OSSTATUS
UDFWriteSectors(
    IN PVCB Vcb,
    IN BOOLEAN Translate,       // Translate Logical to Physical
    IN uint32 Lba,
    IN uint32 BCount,
    IN BOOLEAN Direct,          // Disable access to non-cached data
    IN int8* Buffer,
    OUT uint32* WrittenBytes
    )
{
    OSSTATUS status;

#ifdef _BROWSE_UDF_
    if(!Vcb->Modified || (Vcb->IntegrityType == INTEGRITY_TYPE_CLOSE)) {
        UDFSetModified(Vcb);
        if(Vcb->LVid && !Direct) {
            status = UDFUpdateLogicalVolInt(Vcb,FALSE);
        }
    }

    if(Vcb->CDR_Mode) {
        if(Vcb->LastLBA < Lba+BCount-1)
            Vcb->LastLBA = Lba+BCount-1;
    }
#endif //_BROWSE_UDF_

    if(Vcb->FastCache.WriteProc && (KeGetCurrentIrql() < DISPATCH_LEVEL)) {
        status = WCacheWriteBlocks__(&(Vcb->FastCache), Vcb, Buffer, Lba, BCount, WrittenBytes, Direct);
        ASSERT(OS_SUCCESS(status));
#ifdef _BROWSE_UDF_
        UDFClrZeroBits(Vcb->ZSBM_Bitmap, Lba, BCount);
#endif //_BROWSE_UDF_
        return status;
    }
/*    void* buffer;
    OSSTATUS status;
    uint32 _ReadBytes;
    (*WrittenBytes) = 0;
    buffer = DbgAllocatePool(NonPagedPool, Vcb->WriteBlockSize);
    if(!buffer) return STATUS_INSUFFICIENT_RESOURCES;
    status = UDFTRead(Vcb, Buffer, BCount<<Vcb->BlockSizeBits, (Lba&~(Vcb->WriteBlockSize-1), _WrittenBytes);*/
#ifdef UDF_DBG
    status = UDFTWrite(Vcb, Buffer, BCount<<Vcb->BlockSizeBits, Lba, WrittenBytes);
    ASSERT(OS_SUCCESS(status));
    return status;
#else // UDF_DBG
    return UDFTWrite(Vcb, Buffer, BCount<<Vcb->BlockSizeBits, Lba, WrittenBytes);
#endif // UDF_DBG
} // end UDFWriteSectors()

OSSTATUS
UDFWriteInSector(
    IN PVCB Vcb,
    IN BOOLEAN Translate,       // Translate Logical to Physical
    IN uint32 Lba,
    IN uint32 i,                 // offset in sector
    IN uint32 l,                 // transfer length
    IN BOOLEAN Direct,          // Disable access to non-cached data
    OUT int8* Buffer,
    OUT uint32* WrittenBytes
    )
{
    int8* tmp_buff;
    OSSTATUS status;
#ifdef _BROWSE_UDF_
    uint32 _WrittenBytes;
    uint32 ReadBytes;

    if(!Vcb->Modified) {
        UDFSetModified(Vcb);
        if(Vcb->LVid)
            status = UDFUpdateLogicalVolInt(Vcb,FALSE);
    }

    if(Vcb->CDR_Mode) {
        if(Vcb->LastLBA < Lba)
            Vcb->LastLBA = Lba;
    }
#endif //_BROWSE_UDF_

    (*WrittenBytes) = 0;
#ifdef _BROWSE_UDF_
    if(WCacheIsInitialized__(&(Vcb->FastCache)) && (KeGetCurrentIrql() < DISPATCH_LEVEL)) {
#endif //_BROWSE_UDF_
        status = WCacheDirect__(&(Vcb->FastCache), Vcb, Lba, TRUE, &tmp_buff, Direct);
        if(OS_SUCCESS(status)) {
#ifdef _BROWSE_UDF_
            UDFClrZeroBit(Vcb->ZSBM_Bitmap, Lba);
#endif //_BROWSE_UDF_
            (*WrittenBytes) += l;
            RtlCopyMemory(tmp_buff+i, Buffer, l);
        }
        if(!Direct) WCacheEODirect__(&(Vcb->FastCache), Vcb);
#ifdef _BROWSE_UDF_
    } else {
        // If Direct = TRUE we should never get here, but...
        if(Direct) {
            BrutePoint();
            return STATUS_INVALID_PARAMETER;
        }
        tmp_buff = (int8*)MyAllocatePool__(NonPagedPool, Vcb->BlockSize);
        if(!tmp_buff) {
            BrutePoint();
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        // read packet
        status = UDFReadSectors(Vcb, Translate, Lba, 1, FALSE, tmp_buff, &ReadBytes);
        if(!OS_SUCCESS(status)) goto EO_WrSctD;
        // modify packet
        RtlCopyMemory(tmp_buff+i, Buffer, l);
        // write modified packet
        status = UDFWriteSectors(Vcb, Translate, Lba, 1, FALSE, tmp_buff, &_WrittenBytes);
        if(OS_SUCCESS(status))
            (*WrittenBytes) += l;
EO_WrSctD:
        MyFreePool__(tmp_buff);
    }
    ASSERT(OS_SUCCESS(status));
    if(!OS_SUCCESS(status)) {
        KdPrint(("UDFWriteInSector() for LBA %x failed\n", Lba));
    }
#endif //_BROWSE_UDF_
    return status;
} // end UDFWriteInSector()

/*
    This routine writes data at unaligned offset & length
 */
OSSTATUS
UDFWriteData(
    IN PVCB Vcb,
    IN BOOLEAN Translate,      // Translate Logical to Physical
    IN int64 Offset,
    IN uint32 Length,
    IN BOOLEAN Direct,         // setting this flag delays flushing of given
                               // data to indefinite term
    IN int8* Buffer,
    OUT uint32* WrittenBytes
    )
{
    uint32 i, l, Lba, BS=Vcb->BlockSize;
    uint32 BSh=Vcb->BlockSizeBits;
    OSSTATUS status;
    uint32 _WrittenBytes;
    Vcb->VCBFlags |= UDF_VCB_SKIP_EJECT_CHECK;

    (*WrittenBytes) = 0;
    if(!Length) return STATUS_SUCCESS;
    if(Vcb->VCBFlags & UDF_VCB_FLAGS_DEAD)
        return STATUS_NO_SUCH_DEVICE;
    // write tail of the 1st sector if Offset is not sector_size-aligned
    Lba = (uint32)(Offset >> BSh);
    if(i = ((uint32)Offset & (BS-1))) {
        l = (BS - i) < Length ?
            (BS - i) : Length;
        status = UDFWriteInSector(Vcb, Translate, Lba, i, l, Direct, Buffer, WrittenBytes);
        if(!OS_SUCCESS(status)) return status;
        if(!(Length = Length - l)) return STATUS_SUCCESS;
        Lba ++;
        Buffer += l;
    }
    // write sector_size-aligned part
    i = Length >> BSh;
    if(i) {
        status = UDFWriteSectors(Vcb, Translate, Lba, i, Direct, Buffer, &_WrittenBytes);
        (*WrittenBytes) += _WrittenBytes;
        if(!OS_SUCCESS(status)) return status;
        l = i<<BSh;
#ifdef _BROWSE_UDF_
        UDFClrZeroBits(Vcb->ZSBM_Bitmap, Lba, i);
#endif //_BROWSE_UDF_
        if(!(Length = Length - l)) return STATUS_SUCCESS;
        Lba += i;
        Buffer += l;
    }
    status = UDFWriteInSector(Vcb, Translate, Lba, 0, Length, Direct, Buffer, &_WrittenBytes);
    (*WrittenBytes) += _WrittenBytes;
#ifdef _BROWSE_UDF_
    UDFClrZeroBit(Vcb->ZSBM_Bitmap, Lba);
#endif //_BROWSE_UDF_

    return status;
} // end UDFWriteData()

#endif //UDF_READ_ONLY_BUILD

OSSTATUS
UDFResetDeviceDriver(
    IN PVCB Vcb,
    IN PDEVICE_OBJECT TargetDeviceObject,
    IN BOOLEAN Unlock
    )
{
    PCDRW_RESET_DRIVER_USER_IN tmp = (PCDRW_RESET_DRIVER_USER_IN)
        MyAllocatePool__(NonPagedPool, sizeof(CDRW_RESET_DRIVER_USER_IN));
    OSSTATUS RC;
    if(!tmp)
        return STATUS_INSUFFICIENT_RESOURCES;
    RtlZeroMemory(tmp, sizeof(CDRW_RESET_DRIVER_USER_IN));
    tmp->UnlockTray = (Unlock ? 1 : 0);
    tmp->MagicWord = 0x3a6 | (Unlock ? 1 : 0);
    RC = UDFPhSendIOCTL(IOCTL_CDRW_RESET_DRIVER_EX, TargetDeviceObject,
        tmp, sizeof(CDRW_RESET_DRIVER_USER_IN), NULL, 0, TRUE,NULL);
    if(Vcb) {
        Vcb->LastReadTrack = 0;
        Vcb->LastModifiedTrack = 0;
        Vcb->OPCDone = FALSE;
        if((Vcb->VCBFlags & UDF_VCB_FLAGS_USE_CAV) &&
           Vcb->TargetDeviceObject) {
            // limit both read & write speed to last write speed for CAV mode
            // some drives damage data when speed is adjusted during recording process
            // even in packet mode
            UDFSetSpeeds(Vcb);
        }
        UDFSetCaching(Vcb);
    }

    MyFreePool__(tmp);
    return RC;
} // end UDFResetDeviceDriver()

OSSTATUS
UDFSetSpeeds(
    IN PVCB Vcb
    )
{
    OSSTATUS RC;

    RtlZeroMemory(&(Vcb->SpeedBuf), sizeof(SET_CD_SPEED_EX_USER_IN));
    if(Vcb->VCBFlags & UDF_VCB_FLAGS_USE_CAV) {
        Vcb->SpeedBuf.RotCtrl = CdSpeed_RotCtrl_CAV;
        Vcb->CurSpeed =
        Vcb->SpeedBuf.ReadSpeed =
        Vcb->SpeedBuf.WriteSpeed = Vcb->MaxWriteSpeed;
    } else {
        Vcb->SpeedBuf.ReadSpeed  = Vcb->CurSpeed;
        Vcb->SpeedBuf.WriteSpeed = Vcb->MaxWriteSpeed;
    }
    KdPrint(("    UDFSetSpeeds: set speed to %s %dX/%dX\n",
        (Vcb->VCBFlags & UDF_VCB_FLAGS_USE_CAV) ? "CAV" : "CLV",
        Vcb->SpeedBuf.ReadSpeed / 176,
        Vcb->SpeedBuf.WriteSpeed / 176));
    RC = UDFPhSendIOCTL(IOCTL_CDRW_SET_SPEED,
                        Vcb->TargetDeviceObject,
                        &(Vcb->SpeedBuf),sizeof(SET_CD_SPEED_EX_USER_IN),
                        NULL,0,TRUE,NULL);
    KdPrint(("UDFSetSpeeds: %x\n", RC));
    return RC;
} // end UDFSetSpeeds()

NTSTATUS
UDFSetCaching(
    IN PVCB Vcb
    )
{
#pragma pack(push,1)
    struct {
        MODE_PARAMETER_HEADER    Header;
        MODE_CACHING_PAGE        Data;
        CHAR Padding [16];
    } CachingPage;

    struct {
        MODE_PARAMETER_HEADER         Header;
        MODE_READ_WRITE_RECOVERY_PAGE Data;
        CHAR Padding [16];
    } RecoveryPage;
#pragma pack(pop,1)

    MODE_SENSE_USER_IN ModeSenseCtl;
    OSSTATUS RC;

    KdPrint(("UDFSetCaching:\n"));

    ModeSenseCtl.PageCode.Byte = MODE_PAGE_ERROR_RECOVERY;
    RC = UDFPhSendIOCTL(IOCTL_CDRW_MODE_SENSE, Vcb->TargetDeviceObject,
                    &ModeSenseCtl,sizeof(ModeSenseCtl),
                    (PVOID)&RecoveryPage,sizeof(RecoveryPage),
                    FALSE, NULL);
    if(OS_SUCCESS(RC)) {
        KdPrint(("  Error recovery page:\n"
            "PageCode         %d\n"
            "PageLength       %d\n"

            "DCRBit %d\n"
            "DTEBit %d\n"
            "PERBit %d\n"
            "EERBit %d\n"
            "RCBit  %d\n"
            "TBBit  %d\n"
            "ARRE   %d\n"
            "AWRE   %d\n"

            "ReadRetryCount %d\n"
            "CorrectionSpan %d\n"
            "HeadOffsetCount %d\n"
            "DataStrobOffsetCount %d\n"

            "ErrorRecoveryParam2.Fields.EMCDR %d\n"

            "WriteRetryCount %d\n",

            RecoveryPage.Data.PageCode,
            RecoveryPage.Data.PageLength,

            RecoveryPage.Data.ErrorRecoveryParam.Fields.DCRBit,
            RecoveryPage.Data.ErrorRecoveryParam.Fields.DTEBit,
            RecoveryPage.Data.ErrorRecoveryParam.Fields.PERBit,
            RecoveryPage.Data.ErrorRecoveryParam.Fields.EERBit,
            RecoveryPage.Data.ErrorRecoveryParam.Fields.RCBit,
            RecoveryPage.Data.ErrorRecoveryParam.Fields.TBBit,
            RecoveryPage.Data.ErrorRecoveryParam.Fields.ARRE,
            RecoveryPage.Data.ErrorRecoveryParam.Fields.AWRE,

            RecoveryPage.Data.ReadRetryCount,
            RecoveryPage.Data.CorrectionSpan,
            RecoveryPage.Data.HeadOffsetCount,
            RecoveryPage.Data.DataStrobOffsetCount,

            RecoveryPage.Data.ErrorRecoveryParam2.Fields.EMCDR,

            RecoveryPage.Data.WriteRetryCount

        ));
    }

    ModeSenseCtl.PageCode.Byte = MODE_PAGE_CACHING;
    RC = UDFPhSendIOCTL(IOCTL_CDRW_MODE_SENSE, Vcb->TargetDeviceObject,
                    &ModeSenseCtl,sizeof(ModeSenseCtl),
                    (PVOID)&CachingPage,sizeof(CachingPage),
                    FALSE, NULL);
    if(!OS_SUCCESS(RC)) {
        return RC;
    }

    KdPrint(("  Caching page:\n"
        "PageCode         %d\n"
        "PageLength       %d\n"
        "ReadDisableCache %d\n"
        "MultiplicationFactor %d\n"
        "WriteCacheEnable %d\n"
        "WriteRetensionPriority %d\n"
        "ReadRetensionPriority  %d\n",

        CachingPage.Data.PageCode,
        CachingPage.Data.PageLength,
        CachingPage.Data.ReadDisableCache,
        CachingPage.Data.MultiplicationFactor,
        CachingPage.Data.WriteCacheEnable,
        CachingPage.Data.WriteRetensionPriority,
        CachingPage.Data.ReadRetensionPriority
    ));

    RtlZeroMemory(&CachingPage.Header, sizeof(CachingPage.Header));
    CachingPage.Data.PageCode = MODE_PAGE_CACHING;
    CachingPage.Data.PageSavable = 0;
    if( CachingPage.Data.ReadDisableCache ||
       !CachingPage.Data.WriteCacheEnable) {
        CachingPage.Data.ReadDisableCache = 0;
        CachingPage.Data.WriteCacheEnable = 1;
        RC = UDFPhSendIOCTL(IOCTL_CDRW_MODE_SELECT, Vcb->TargetDeviceObject,
                        (PVOID)&CachingPage,sizeof(CachingPage.Header) + 2 + CachingPage.Data.PageLength,
                        NULL,0,
                        FALSE, NULL);
    } else {
        RC = STATUS_SUCCESS;
    }
    KdPrint(("UDFSetCaching: %x\n", RC));
    return RC;
} // end UDFSetCaching()
