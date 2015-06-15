////////////////////////////////////////////////////////////////////
// Copyright (C) Alexander Telyatnikov, Ivan Keliukh, Yegor Anchishkin, SKIF Software, 1999-2013. Kiev, Ukraine
// All rights reserved
// This file was released under the GPLv2 on June 2015.
////////////////////////////////////////////////////////////////////
/*
        Module name:

   remap.cpp

        Abstract:

   This file contains filesystem-specific routines
   responsible for disk space management

*/

#include "udf.h"

#define         UDF_BUG_CHECK_ID                UDF_FILE_UDF_INFO_REMAP

typedef struct _UDF_VERIFY_ITEM {
    lba_t      lba;
    ULONG      crc;
    PUCHAR     Buffer;
    LIST_ENTRY vrfList;
    BOOLEAN    queued;
} UDF_VERIFY_ITEM, *PUDF_VERIFY_ITEM;

typedef struct _UDF_VERIFY_REQ_RANGE {
    lba_t      lba;
    uint32     BCount;
} UDF_VERIFY_REQ_RANGE, *PUDF_VERIFY_REQ_RANGE;

#define MAX_VREQ_RANGES  128

typedef struct _UDF_VERIFY_REQ {
    PVCB       Vcb;
    PUCHAR     Buffer;
    ULONG      nReq;
    UDF_VERIFY_REQ_RANGE vr[MAX_VREQ_RANGES];
#ifndef _CONSOLE
    WORK_QUEUE_ITEM VerifyItem;
#endif
} UDF_VERIFY_REQ, *PUDF_VERIFY_REQ;

VOID
UDFVRemoveBlock(
    PUDF_VERIFY_CTX VerifyCtx,
    PUDF_VERIFY_ITEM vItem
    );

OSSTATUS
UDFVInit(
    IN PVCB Vcb
    )
{
    PUDF_VERIFY_CTX VerifyCtx = &Vcb->VerifyCtx;
    uint32 i;
    OSSTATUS status = STATUS_SUCCESS;
    BOOLEAN res_inited = FALSE;

    if(VerifyCtx->VInited) {
        KdPrint(("Already inited\n"));
        return STATUS_SUCCESS;
    }
    
    _SEH2_TRY {
        RtlZeroMemory(VerifyCtx, sizeof(UDF_VERIFY_CTX));
        if(!Vcb->VerifyOnWrite) {
            KdPrint(("Verify is disabled\n"));
            return STATUS_SUCCESS;
        }
        if(Vcb->CDR_Mode) {
            KdPrint(("Verify is not intended for CD/DVD-R\n"));
            return STATUS_SUCCESS;
        }
        if(!OS_SUCCESS(status = ExInitializeResourceLite(&(VerifyCtx->VerifyLock)))) {
            try_return(status);
        }
        res_inited = TRUE;
        VerifyCtx->ItemCount = 0;
        VerifyCtx->StoredBitMap = (uint8*)DbgAllocatePoolWithTag(PagedPool, (i = (Vcb->LastPossibleLBA+1+7)>>3), 'mNWD' );
        if(VerifyCtx->StoredBitMap) {
            RtlZeroMemory(VerifyCtx->StoredBitMap, i);
        } else {
            KdPrint(("Can't alloc verify bitmap for %x blocks\n", Vcb->LastPossibleLBA));
            try_return(status = STATUS_INSUFFICIENT_RESOURCES);
        }
        InitializeListHead(&(VerifyCtx->vrfList));
        KeInitializeEvent(&(VerifyCtx->vrfEvent), SynchronizationEvent, FALSE);
        VerifyCtx->WaiterCount = 0;
        VerifyCtx->VInited = TRUE;

try_exit: NOTHING;

    } _SEH2_FINALLY {

        if(!OS_SUCCESS(status)) {
            if(res_inited) {
                ExDeleteResourceLite(&(VerifyCtx->VerifyLock));
            }
        }
    } _SEH2_END;
    return status;
} // end UDFVInit()

VOID
UDFVWaitQueued(
    PUDF_VERIFY_CTX VerifyCtx
    )
{
    ULONG w;

    while(VerifyCtx->QueuedCount) {
        KdPrint(("UDFVWaitQueued: wait for completion (%d)\n", VerifyCtx->QueuedCount));
        w = InterlockedIncrement((PLONG)&(VerifyCtx->WaiterCount));
        KdPrint(("  %d waiters\n", w));
        DbgWaitForSingleObject(&(VerifyCtx->vrfEvent), NULL);
        if((w = InterlockedDecrement((PLONG)&(VerifyCtx->WaiterCount)))) {
            KdPrint(("  still %d waiters, q %d\n", w, VerifyCtx->QueuedCount));
            if(!VerifyCtx->QueuedCount) {
                KdPrint(("  pulse event\n", w));
                KeSetEvent(&(VerifyCtx->vrfEvent), 0, FALSE);
            }
        }
    }
    return;
} // end UDFVWaitQueued()

VOID
UDFVRelease(
    IN PVCB Vcb
    )
{
    PUDF_VERIFY_CTX VerifyCtx = &Vcb->VerifyCtx;
    PLIST_ENTRY Link;
    PUDF_VERIFY_ITEM vItem;

    if(!VerifyCtx->VInited) {
        return;
    }

    KdPrint(("UDFVRelease: wait for completion\n"));
    UDFVWaitQueued(VerifyCtx);

    UDFAcquireResourceExclusive(&(VerifyCtx->VerifyLock), TRUE);

    Link = VerifyCtx->vrfList.Flink;
    
    while(Link != &(VerifyCtx->vrfList)) {
        vItem = CONTAINING_RECORD( Link, UDF_VERIFY_ITEM, vrfList );
        Link = Link->Flink;
        //DbgFreePool(vItem);
        UDFVRemoveBlock(VerifyCtx, vItem);
    }
    VerifyCtx->VInited = FALSE;

    UDFReleaseResource(&(VerifyCtx->VerifyLock));

    ExDeleteResourceLite(&(VerifyCtx->VerifyLock));
    DbgFreePool(VerifyCtx->StoredBitMap);

    RtlZeroMemory(VerifyCtx, sizeof(UDF_VERIFY_CTX));

    return;
} // end UDFVRelease()

PUDF_VERIFY_ITEM
UDFVStoreBlock(
    IN PVCB Vcb,
    IN uint32 LBA,
    IN PVOID Buffer,
    PLIST_ENTRY Link
    )
{
    PUDF_VERIFY_CTX VerifyCtx = &Vcb->VerifyCtx;
    PUDF_VERIFY_ITEM vItem;

    KdPrint(("v-add %x\n", LBA));

    vItem = (PUDF_VERIFY_ITEM)DbgAllocatePoolWithTag(PagedPool, sizeof(UDF_VERIFY_ITEM)+Vcb->BlockSize, 'bvWD');
    if(!vItem)
        return NULL;
    RtlCopyMemory(vItem+1, Buffer, Vcb->BlockSize);
    vItem->lba = LBA;
    vItem->crc = crc32((PUCHAR)Buffer, Vcb->BlockSize);
    vItem->Buffer = (PUCHAR)(vItem+1);
    vItem->queued = FALSE;
    InitializeListHead(&(vItem->vrfList));
    InsertTailList(Link, &(vItem->vrfList));
    UDFSetBit(VerifyCtx->StoredBitMap, LBA);
    VerifyCtx->ItemCount++;
    return vItem;
} // end UDFVStoreBlock()

VOID
UDFVUpdateBlock(
    IN PVCB Vcb,
    IN PVOID Buffer,
    PUDF_VERIFY_ITEM vItem
    )
{
    KdPrint(("v-upd %x\n", vItem->lba));
    RtlCopyMemory(vItem+1, Buffer, Vcb->BlockSize);
    vItem->crc = crc32((PUCHAR)Buffer, Vcb->BlockSize);
    return;
} // end UDFVUpdateBlock()

VOID
UDFVRemoveBlock(
    PUDF_VERIFY_CTX VerifyCtx,
    PUDF_VERIFY_ITEM vItem
    )
{
    KdPrint(("v-del %x\n", vItem->lba));
    UDFClrBit(VerifyCtx->StoredBitMap, vItem->lba);
    RemoveEntryList(&(vItem->vrfList));
    VerifyCtx->ItemCount--;
    DbgFreePool(vItem);
    return;
} // end UDFVUpdateBlock()

OSSTATUS
UDFVWrite(
    IN PVCB Vcb,
    IN void* Buffer,     // Target buffer
    IN uint32 BCount,
    IN uint32 LBA,
//    OUT uint32* WrittenBytes,
    IN uint32 Flags
    )
{
    PLIST_ENTRY Link;
    PUDF_VERIFY_ITEM vItem;
    //PUDF_VERIFY_ITEM vItem1;
    PUDF_VERIFY_CTX VerifyCtx = &Vcb->VerifyCtx;
    ULONG i;
    ULONG n;
    //uint32 prev_lba;

    if(!VerifyCtx->VInited) {
        return STATUS_SUCCESS;
    }

    UDFAcquireResourceExclusive(&(VerifyCtx->VerifyLock), TRUE);

    for(i=0, n=0; i<BCount; i++) {
        if(UDFGetBit(VerifyCtx->StoredBitMap, LBA+i)) {
            // some blocks are remembered
            n++;
        }
    }

    if(n == BCount) {
        // update all blocks
        n = 0;
        Link = VerifyCtx->vrfList.Blink;
        while(Link != &(VerifyCtx->vrfList)) {
            vItem = CONTAINING_RECORD( Link, UDF_VERIFY_ITEM, vrfList );
            Link = Link->Blink;
            if(vItem->lba >= LBA && vItem->lba < LBA+BCount) {
                ASSERT(UDFGetBit(VerifyCtx->StoredBitMap, vItem->lba));
                UDFVUpdateBlock(Vcb, ((PUCHAR)Buffer)+(vItem->lba-LBA)*Vcb->BlockSize, vItem);
                n++;
                if(n == BCount) {
                    // all updated
                    break;
                }
            }
        }
    } else
    if(n) {
#if 0
        // find remembered blocks (the 1st one)
        Link = VerifyCtx->vrfList.Blink;
        while(Link != &(VerifyCtx->vrfList)) {
            vItem = CONTAINING_RECORD( Link, UDF_VERIFY_ITEM, vrfList );
            Link = Link->Blink;
            if(vItem->lba >= LBA && vItem->lba < LBA+BCount) {
                //UDFVRemoveBlock(VerifyCtx, vItem);
                break;
            }
        }

        // check if contiguous
        i=1;
        prev_lba = vItem->lba;
        vItem1 = vItem;
        Link = Link->Blink;
        while((i < n) && (Link != &(VerifyCtx->vrfList))) {
            vItem = CONTAINING_RECORD( Link, UDF_VERIFY_ITEM, vrfList );
            Link = Link->Blink;
            if(vItem->lba > LBA || vItem->lba >= LBA+BCount) {
                // end
                break;
            }
            if(vItem->lba < prev_lba) {
                // not sorted
                break;
            }
            prev_lba = vItem->lba;
            i++;
        }

        if(i == n) {
            // cont
        } else {
            // drop all and add again
        }

        vItem1 = vItem;
        for(i=0; i<BCount; i++) {
            if(vItem->lba == LBA+i) {
                ASSERT(UDFGetBit(VerifyCtx->StoredBitMap, LBA+i));
                UDFVUpdateBlock(Vcb, ((PUCHAR)Buffer)+i*Vcb->BlockSize, vItem);
                continue;
            }
            if(vItem1->lba == LBA+i) {
                ASSERT(UDFGetBit(VerifyCtx->StoredBitMap, LBA+i));
                UDFVUpdateBlock(Vcb, ((PUCHAR)Buffer)+i*Vcb->BlockSize, vItem1);
                continue;
            }
            if(vItem1->lba > LBA+i) {
                // just insert this block
                ASSERT(!UDFGetBit(VerifyCtx->StoredBitMap, LBA+i));
                UDFVStoreBlock(Vcb, LBA+i, ((PUCHAR)Buffer)+i*Vcb->BlockSize, &(vItem1->vrfList));
            } else {
                vItem = CONTAINING_RECORD( vItem->vrfList.Blink, UDF_VERIFY_ITEM, vrfList );
            }
        }
#else
        Link = VerifyCtx->vrfList.Blink;
        i=0;
        while(Link != &(VerifyCtx->vrfList)) {
            vItem = CONTAINING_RECORD( Link, UDF_VERIFY_ITEM, vrfList );
            Link = Link->Blink;
            if(vItem->lba >= LBA && vItem->lba < LBA+BCount) {
                UDFVRemoveBlock(VerifyCtx, vItem);
                i++;
                if(i == n) {
                    // all killed
                    break;
                }
            }
        }
        goto remember_all;
#endif

    } else {
remember_all:
        // remember all blocks
        for(i=0; i<BCount; i++) {
            ASSERT(!UDFGetBit(VerifyCtx->StoredBitMap, LBA+i));
            UDFVStoreBlock(Vcb, LBA+i, ((PUCHAR)Buffer)+i*Vcb->BlockSize, &(VerifyCtx->vrfList));
        }
    }

    if(VerifyCtx->ItemCount > UDF_MAX_VERIFY_CACHE) {
        UDFVVerify(Vcb, UFD_VERIFY_FLAG_LOCKED);
    }

    UDFReleaseResource(&(VerifyCtx->VerifyLock));

    if(VerifyCtx->ItemCount > UDF_MAX_VERIFY_CACHE*2) {
        //UDFVVerify(Vcb, UFD_VERIFY_FLAG_LOCKED);
        // TODO: make some delay
    }

    return STATUS_SUCCESS;

} // end UDFVWrite()

OSSTATUS
UDFVRead(
    IN PVCB Vcb,
    IN void* Buffer,     // Target buffer
    IN uint32 BCount,
    IN uint32 LBA,
//    OUT uint32* ReadBytes,
    IN uint32 Flags
    )
{
    PLIST_ENTRY Link;
    PUDF_VERIFY_ITEM vItem;
    PUDF_VERIFY_CTX VerifyCtx = &Vcb->VerifyCtx;
    ULONG crc;
    ULONG i;
    ULONG n;
    OSSTATUS status = STATUS_SUCCESS;
    uint32* bm;

    if(!VerifyCtx->VInited) {
        return STATUS_SUCCESS;
        //return STATUS_UNSUCCESSFUL;
    }

    UDFAcquireResourceExclusive(&(VerifyCtx->VerifyLock), TRUE);

    for(i=0, n=0; i<BCount; i++) {
        if(UDFGetBit(VerifyCtx->StoredBitMap, LBA+i)) {
            // some blocks are remembered
            n++;
        }
    }

    if(!n) {
        // no blocks are remembered
        UDFReleaseResource(&(VerifyCtx->VerifyLock));
        return STATUS_SUCCESS;
    }

    Link = VerifyCtx->vrfList.Flink;
    i=0;
    while(Link != &(VerifyCtx->vrfList)) {
        vItem = CONTAINING_RECORD( Link, UDF_VERIFY_ITEM, vrfList );
        Link = Link->Flink;
        if(vItem->lba >= LBA && vItem->lba < LBA+BCount) {
            ASSERT(UDFGetBit(VerifyCtx->StoredBitMap, vItem->lba));
            i++;
            if(!(Flags & PH_READ_VERIFY_CACHE)) {
                crc = crc32((PUCHAR)Buffer+(vItem->lba - LBA)*Vcb->BlockSize, Vcb->BlockSize);
                if(vItem->crc != crc) {
                    KdPrint(("UDFVRead: stored %x != %x\n", vItem->crc, crc));
                    RtlCopyMemory((PUCHAR)Buffer+(vItem->lba - LBA)*Vcb->BlockSize, vItem->Buffer, Vcb->BlockSize);
                    status = STATUS_FT_WRITE_RECOVERY;

                    if(!(bm = (uint32*)(Vcb->BSBM_Bitmap))) {
                        crc = (Vcb->LastPossibleLBA+1+7) >> 3; // reuse 'crc' variable
                        bm = (uint32*)(Vcb->BSBM_Bitmap = (int8*)DbgAllocatePoolWithTag(NonPagedPool, crc, 'mNWD' ));
                        if(bm) {
                            RtlZeroMemory(bm, crc);
                        } else {
                            KdPrint(("Can't alloc BSBM for %x blocks\n", Vcb->LastPossibleLBA));
                        }
                    }
                    if(bm) {
                        UDFSetBit(bm, vItem->lba);
                        KdPrint(("Set BB @ %#x\n", vItem->lba));
                    }
#ifdef _BROWSE_UDF_
                    bm = (uint32*)(Vcb->FSBM_Bitmap);
                    if(bm) {
                        UDFSetUsedBit(bm, vItem->lba);
                        KdPrint(("Set BB @ %#x as used\n", vItem->lba));
                    }
#endif //_BROWSE_UDF_
                } else {
                    // ok
                }
            } else {
                KdPrint(("UDFVRead: get cached @ %x\n", vItem->lba));
                RtlCopyMemory((PUCHAR)Buffer+(vItem->lba - LBA)*Vcb->BlockSize, vItem->Buffer, Vcb->BlockSize);
            }
            if(i >= n) {
                // no more blocks expected
                break;
            }
        }
    }

    if((status == STATUS_SUCCESS && !(Flags & PH_KEEP_VERIFY_CACHE)) || (Flags & PH_FORGET_VERIFIED)) {
        // ok, forget this, no errors found
        Link = VerifyCtx->vrfList.Flink;
        i = 0;
        while(Link != &(VerifyCtx->vrfList)) {
            vItem = CONTAINING_RECORD( Link, UDF_VERIFY_ITEM, vrfList );
            Link = Link->Flink;
            if(vItem->lba >= LBA && vItem->lba < LBA+BCount) {
                i++;
                UDFVRemoveBlock(VerifyCtx, vItem);
                if(i >= n) {
                    // no more blocks expected
                    break;
                }
            }
        }
    }

    UDFReleaseResource(&(VerifyCtx->VerifyLock));
    return status;

} // end UDFVRead()

OSSTATUS
UDFVForget(
    IN PVCB Vcb,
    IN uint32 BCount,
    IN uint32 LBA,
    IN uint32 Flags
    )
{
    PLIST_ENTRY Link;
    PUDF_VERIFY_ITEM vItem;
    PUDF_VERIFY_CTX VerifyCtx = &Vcb->VerifyCtx;
    ULONG i;
    ULONG n;
    OSSTATUS status = STATUS_SUCCESS;

    if(!VerifyCtx->VInited) {
        return STATUS_UNSUCCESSFUL;
    }

    UDFAcquireResourceExclusive(&(VerifyCtx->VerifyLock), TRUE);

    for(i=0, n=0; i<BCount; i++) {
        if(UDFGetBit(VerifyCtx->StoredBitMap, LBA+i)) {
            // some blocks are remembered
            n++;
        }
    }

    if(!n) {
        // no blocks are remembered
        UDFReleaseResource(&(VerifyCtx->VerifyLock));
        return STATUS_SUCCESS;
    }

    Link = VerifyCtx->vrfList.Flink;
    i = 0;
    while(Link != &(VerifyCtx->vrfList)) {
        vItem = CONTAINING_RECORD( Link, UDF_VERIFY_ITEM, vrfList );
        Link = Link->Flink;
        if(vItem->lba >= LBA && vItem->lba < LBA+BCount) {
            i++;
            UDFVRemoveBlock(VerifyCtx, vItem);
            if(i >= n) {
                // no more blocks expected
                break;
            }
        }
    }

    UDFReleaseResource(&(VerifyCtx->VerifyLock));
    return status;

} // end UDFVForget()

VOID
UDFVWorkItem(
    PUDF_VERIFY_REQ VerifyReq
    )
{
    PVCB Vcb = VerifyReq->Vcb;
    ULONG ReadBytes;
//    OSSTATUS RC;
    ULONG i;

    ReadBytes = (ULONG)Vcb;
#if 1
    if(Vcb->SparingCountFree) {
        WCacheStartDirect__(&(Vcb->FastCache), Vcb, TRUE);
        for(i=0; i<VerifyReq->nReq; i++) {
            UDFTIOVerify(Vcb,
                         VerifyReq->Buffer,     // Target buffer
                         VerifyReq->vr[i].BCount << Vcb->BlockSizeBits,
                         VerifyReq->vr[i].lba,
                         &ReadBytes,
                         PH_TMP_BUFFER | PH_VCB_IN_RETLEN /*| PH_LOCK_CACHE*/);
        }
        WCacheEODirect__(&(Vcb->FastCache), Vcb);
    } else {
        for(i=0; i<VerifyReq->nReq; i++) {
            KdPrint(("!!! No more space for remap !!!\n"));
            KdPrint(("  try del from verify cache @ %x\n", VerifyReq->vr[i].lba));
            UDFVRead(Vcb, VerifyReq->Buffer, VerifyReq->vr[i].BCount, VerifyReq->vr[i].lba,
                     PH_FORGET_VERIFIED | PH_READ_VERIFY_CACHE | PH_TMP_BUFFER);
        }
    }
#else
    for(i=0; i<VerifyReq->nReq; i++) {
        if(Vcb->SparingCountFree) {
            WCacheStartDirect__(&(Vcb->FastCache), Vcb, TRUE);
            RC = UDFTIOVerify(Vcb,
                           VerifyReq->Buffer,     // Target buffer
                           VerifyReq->vr[i].BCount << Vcb->BlockSizeBits,
                           VerifyReq->vr[i].lba,
                           &ReadBytes,
                           PH_TMP_BUFFER | PH_VCB_IN_RETLEN /*| PH_LOCK_CACHE*/);
            WCacheEODirect__(&(Vcb->FastCache), Vcb);
        } else {
            KdPrint(("!!! No more space for remap !!!\n"));
            KdPrint(("  try del from verify cache @ %x\n", VerifyReq->vr[i].lba));
            RC = UDFVRead(Vcb, VerifyReq->Buffer, VerifyReq->vr[i].BCount, VerifyReq->vr[i].lba,
                          PH_FORGET_VERIFIED | PH_READ_VERIFY_CACHE | PH_TMP_BUFFER);
        }
    }
#endif
    DbgFreePool(VerifyReq->Buffer);
    DbgFreePool(VerifyReq);
    InterlockedDecrement((PLONG)&(Vcb->VerifyCtx.QueuedCount));
    KdPrint(("  QueuedCount = %d\n", Vcb->VerifyCtx.QueuedCount));
    KdPrint(("  Setting event...\n"));
    KeSetEvent(&(Vcb->VerifyCtx.vrfEvent), 0, FALSE);
    return;
} // end UDFVWorkItem()

VOID
UDFVVerify(
    IN PVCB Vcb,
    IN ULONG Flags
    )
{
    PUDF_VERIFY_CTX VerifyCtx = &Vcb->VerifyCtx;
    PLIST_ENTRY Link;
    PUDF_VERIFY_ITEM vItem;
    PUDF_VERIFY_REQ VerifyReq = NULL;
    ULONG len, max_len=0;
    lba_t prev_lba;
    //PUCHAR tmp_buff;
    ULONG i;
    BOOLEAN do_vrf = FALSE;

    if(!VerifyCtx->VInited) {
        return;
    }
    if(VerifyCtx->QueuedCount) {
        if(Flags & UFD_VERIFY_FLAG_WAIT) {
            KdPrint(("  wait for verify flush\n"));
            goto wait;
        }
        KdPrint(("  verify flush already queued\n"));
        return;
    }

    if(!(Flags & (UFD_VERIFY_FLAG_FORCE | UFD_VERIFY_FLAG_BG))) {
        if(VerifyCtx->ItemCount < UDF_MAX_VERIFY_CACHE) {
            return;
        }

    }
    if(!(Flags & UFD_VERIFY_FLAG_LOCKED)) {
        UDFAcquireResourceExclusive(&(VerifyCtx->VerifyLock), TRUE);
    }

    if(Flags & UFD_VERIFY_FLAG_FORCE) {
        i = VerifyCtx->ItemCount;
    } else {
        if(VerifyCtx->ItemCount >= UDF_MAX_VERIFY_CACHE) {
            i = VerifyCtx->ItemCount - UDF_VERIFY_CACHE_LOW;
        } else {
            i = min(UDF_VERIFY_CACHE_GRAN, VerifyCtx->ItemCount);
        }
    }

    Link = VerifyCtx->vrfList.Flink;
    prev_lba = -2;
    len = 0;
    
    while(i) {
        ASSERT(Link != &(VerifyCtx->vrfList));
/*
        if(Link == &(VerifyCtx->vrfList)) {
            if(!len)
                break;
            i=1;
            goto queue_req;
        }
*/
        vItem = CONTAINING_RECORD( Link, UDF_VERIFY_ITEM, vrfList );
        Link = Link->Flink;

        //
        if(!vItem->queued && (prev_lba+len == vItem->lba)) {
            vItem->queued = TRUE;
            len++;
        } else {
            if(len) {
                do_vrf = TRUE;
            } else {
                len = 1;
                prev_lba = vItem->lba;
            }
        }
        if((i == 1) && len) {
            do_vrf = TRUE;
        }
        if(len >= 0x100) {
            do_vrf = TRUE;
        }
        if(do_vrf) {
//queue_req:
            if(!VerifyReq) {
                VerifyReq = (PUDF_VERIFY_REQ)DbgAllocatePoolWithTag(NonPagedPool, sizeof(UDF_VERIFY_REQ), 'bNWD');
                if(VerifyReq) {
                    RtlZeroMemory(VerifyReq, sizeof(UDF_VERIFY_REQ));
                    VerifyReq->Vcb    = Vcb;
                }
            }
            if(VerifyReq) {

                VerifyReq->vr[VerifyReq->nReq].lba    = prev_lba;
                VerifyReq->vr[VerifyReq->nReq].BCount = len;
                VerifyReq->nReq++;
                if(max_len < len) {
                    max_len = len;
                }

                if((VerifyReq->nReq >= MAX_VREQ_RANGES) || (i == 1)) {

                    VerifyReq->Buffer = (PUCHAR)DbgAllocatePoolWithTag(NonPagedPool, max_len * Vcb->BlockSize, 'bNWD');
                    if(VerifyReq->Buffer) {
                        InterlockedIncrement((PLONG)&(VerifyCtx->QueuedCount));
#ifndef _CONSOLE
                        ExInitializeWorkItem( &(VerifyReq->VerifyItem),
                                              (PWORKER_THREAD_ROUTINE) UDFVWorkItem,
                                              VerifyReq );
                        ExQueueWorkItem( &(VerifyReq->VerifyItem), CriticalWorkQueue );
#else
                        UDFVWorkItem(VerifyReq);
#endif
                    } else {
                        DbgFreePool(VerifyReq);
                    }
                    VerifyReq = NULL;
                    max_len = 0;
                } else {
                }
            }
            len = 1;
            prev_lba = vItem->lba;
            do_vrf = FALSE;
        }
        i--;
    }

    if(!(Flags & UFD_VERIFY_FLAG_LOCKED)) {
        UDFReleaseResource(&(VerifyCtx->VerifyLock));
    }
    if(Flags & UFD_VERIFY_FLAG_WAIT) {
wait:
        KdPrint(("UDFVVerify: wait for completion\n"));
        UDFVWaitQueued(VerifyCtx);
    }

    return;
} // end UDFVVerify()

VOID
UDFVFlush(
    IN PVCB Vcb
    )
{
    PUDF_VERIFY_CTX VerifyCtx = &Vcb->VerifyCtx;

    if(!VerifyCtx->VInited) {
        return;
    }

    KdPrint(("UDFVFlush: wait for completion\n"));
    UDFVWaitQueued(VerifyCtx);

    UDFVVerify(Vcb, UFD_VERIFY_FLAG_FORCE);

    KdPrint(("UDFVFlush: wait for completion (2)\n"));
    UDFVWaitQueued(VerifyCtx);
} // end UDFVFlush()

BOOLEAN
__fastcall
UDFCheckArea(
    IN PVCB Vcb,
    IN lba_t LBA,
    IN uint32 BCount
    )
{
    uint8* buff;
    OSSTATUS RC;
    uint32 ReadBytes;
    uint32 i, d;
    BOOLEAN ext_ok = TRUE;
    EXTENT_MAP Map[2];
    uint32 PS = Vcb->WriteBlockSize >> Vcb->BlockSizeBits;

    buff = (uint8*)DbgAllocatePoolWithTag(NonPagedPool, Vcb->WriteBlockSize, 'bNWD' );
    if(buff) {
        for(i=0; i<BCount; i+=d) {
            if(!((LBA+i) & (PS-1)) &&
               (i+PS <= BCount)) {
                d = PS;
            } else {
                d = 1;
            }
            RC = UDFTRead(Vcb,
                           buff,
                           d << Vcb->BlockSizeBits,
                           LBA+i,
                           &ReadBytes,
                           PH_TMP_BUFFER);

            if(RC != STATUS_SUCCESS) {
                Map[0].extLocation = LBA+i;
                Map[0].extLength = d << Vcb->BlockSizeBits;
                UDFMarkSpaceAsXXXNoProtect(Vcb, 0, &(Map[0]), AS_DISCARDED | AS_BAD); // free
                ext_ok = FALSE;
            }
        }
        DbgFreePool(buff);
    }
    return ext_ok;
} // end UDFCheckArea()

/*
    This routine remaps sectors from bad packet
 */
OSSTATUS
__fastcall
UDFRemapPacket(
    IN PVCB Vcb,
    IN uint32 Lba,
    IN BOOLEAN RemapSpared
    )
{
    uint32 i, max, BS, orig;
    PSPARING_MAP Map;
    BOOLEAN verified = FALSE;

    if(Vcb->SparingTable) {

        max = Vcb->SparingCount;
        BS = Vcb->SparingBlockSize;

        // use sparing table for relocation
        if(Vcb->SparingCountFree == (ULONG)-1) {
            KdPrint(("calculate free spare areas\n"));
re_check:
            KdPrint(("verify spare area\n"));
            Vcb->SparingCountFree = 0;
            Map = Vcb->SparingTable;
            for(i=0;i<max;i++,Map++) {
                if(Map->origLocation == SPARING_LOC_AVAILABLE) {
                    if(UDFCheckArea(Vcb, Map->mappedLocation, BS)) {
                        Vcb->SparingCountFree++;
                    } else {
                        KdPrint(("initial check: bad spare block @ %x\n", Map->mappedLocation));
                        Map->origLocation = SPARING_LOC_CORRUPTED;
                        Vcb->SparingTableModified = TRUE;
                    }
                }
            }
        }
        if(!Vcb->SparingCountFree) {
            KdPrint(("sparing table full\n"));
            return STATUS_DISK_FULL;
        }

        Map = Vcb->SparingTable;
        Lba &= ~(BS-1);
        for(i=0;i<max;i++,Map++) {
            orig = Map->origLocation;
            if(Lba == (orig & ~(BS-1)) ) {
                // already remapped

                KdPrint(("remap remapped: bad spare block @ %x\n", Map->mappedLocation));
                if(!verified) {
                    verified = TRUE;
                    goto re_check;
                }

                if(!RemapSpared) {
                    return STATUS_SHARING_VIOLATION;
                } else {
                    // look for another remap area
                    Map->origLocation = SPARING_LOC_CORRUPTED;
                    Vcb->SparingTableModified = TRUE;
                    Vcb->SparingCountFree--;
                    break;
                }
            }
        }
        Map = Vcb->SparingTable;
        for(i=0;i<max;i++,Map++) {
            if(Map->origLocation == SPARING_LOC_AVAILABLE) {
                KdPrint(("remap %x -> %x\n", Lba, Map->mappedLocation));
                Map->origLocation = Lba;
                Vcb->SparingTableModified = TRUE;
                Vcb->SparingCountFree--;
                return STATUS_SUCCESS;
            }
        }
        KdPrint(("sparing table full\n"));
        return STATUS_DISK_FULL;
    }
    return STATUS_UNSUCCESSFUL;
} // end UDFRemapPacket()

/*
    This routine releases sector mapping when entire packet is marked as free
 */
OSSTATUS
__fastcall
UDFUnmapRange(
    IN PVCB Vcb,
    IN uint32 Lba,
    IN uint32 BCount
    )
{
    uint32 i, max, BS, orig;
    PSPARING_MAP Map;

    if(Vcb->SparingTable) {
        // use sparing table for relocation

        max = Vcb->SparingCount;
        BS = Vcb->SparingBlockSize;
        Map = Vcb->SparingTable;
        for(i=0;i<max;i++,Map++) {
            orig = Map->origLocation;
            switch(orig) {
            case SPARING_LOC_AVAILABLE:
            case SPARING_LOC_CORRUPTED:
                continue;
            }
            if(orig >= Lba &&
              (orig+BS) <= (Lba+BCount)) {
                // unmap
                KdPrint(("unmap %x -> %x\n", orig, Map->mappedLocation));
                Map->origLocation = SPARING_LOC_AVAILABLE;
                Vcb->SparingTableModified = TRUE;
                Vcb->SparingCountFree++;
            }
        }
    }
    return STATUS_SUCCESS;
} // end UDFUnmapRange()

/*
    This routine returns physical address for relocated sector
 */
uint32
__fastcall
UDFRelocateSector(
    IN PVCB Vcb,
    IN uint32 Lba
    )
{
    uint32 i, max, BS, orig;

    if(Vcb->SparingTable) {
        // use sparing table for relocation
        uint32 _Lba;
        PSPARING_MAP Map = Vcb->SparingTable;

        max = Vcb->SparingCount;
        BS = Vcb->SparingBlockSize;
        _Lba = Lba & ~(BS-1);
        for(i=0;i<max;i++,Map++) {
            orig = Map->origLocation;
            if(_Lba == (orig & ~(BS-1)) ) {
            //if( (Lba >= (orig = Map->origLocation)) && (Lba < orig + BS) ) {
                return Map->mappedLocation + Lba - orig;
            }
        }
    } else if(Vcb->Vat) {
        // use VAT for relocation
        uint32* Map = Vcb->Vat;
        uint32 root;
        // check if given Lba lays in the partition covered by VAT
        if(Lba >= Vcb->NWA)
            return Vcb->NWA;
        if(Lba < (root = Vcb->Partitions[Vcb->VatPartNdx].PartitionRoot))
            return Lba;
        Map = &(Vcb->Vat[(i = Lba - root)]);
        if((i < Vcb->VatCount) && (i=(*Map)) ) {
            if(i != UDF_VAT_FREE_ENTRY) {
                return i + root;
            } else {
                return 0x7fffffff;
            }
        }
    }
    return Lba;
} // end UDFRelocateSector()

/*
    This routine checks if the extent specified requires relocation
 */
BOOLEAN
__fastcall
UDFAreSectorsRelocated(
    IN PVCB Vcb,
    IN uint32 Lba,
    IN uint32 BlockCount
    )
{

    if(Vcb->SparingTable) {
        // use sparing table for relocation
        uint32 i, BS, orig;
        BS = Vcb->SparingBlockSize;
        PSPARING_MAP Map;

        Map = Vcb->SparingTable;
        for(i=0;i<Vcb->SparingCount;i++,Map++) {
            if( ((Lba >= (orig = Map->origLocation)) && (Lba < orig + BS)) ||
                ((Lba+BlockCount-1 >= orig) && (Lba+BlockCount-1 < orig + BS)) ||
                ((orig >= Lba) && (orig < Lba+BlockCount)) ||
                ((orig+BS >= Lba) && (orig+BS < Lba+BlockCount)) ) {
                return TRUE;
            }
        }
    } else if(Vcb->Vat) {
        // use VAT for relocation
        uint32 i, root, j;
        uint32* Map;
        if(Lba < (root = Vcb->Partitions[Vcb->VatPartNdx].PartitionRoot))
            return FALSE;
        if(Lba+BlockCount >= Vcb->NWA)
            return TRUE;
        Map = &(Vcb->Vat[Lba-root/*+i*/]);
        for(i=0; i<BlockCount; i++, Map++) {
            if((j = (*Map)) &&
               (j != Lba-root+i) &&
               ((j != UDF_VAT_FREE_ENTRY) || ((Lba+i) < Vcb->LastLBA)))
                return TRUE;
        }
    }
    return FALSE;
} // end UDFAreSectorsRelocated()

/*
    This routine builds mapping for relocated extent
    If relocation is not required (-1) will be returned
 */
PEXTENT_MAP
__fastcall
UDFRelocateSectors(
    IN PVCB Vcb,
    IN uint32 Lba,
    IN uint32 BlockCount
    )
{
    if(!UDFAreSectorsRelocated(Vcb, Lba, BlockCount)) return UDF_NO_EXTENT_MAP;

    PEXTENT_MAP Extent=NULL, Extent2;
    uint32 NewLba, LastLba, j, i;
    EXTENT_AD locExt;

    LastLba = UDFRelocateSector(Vcb, Lba);
    for(i=0, j=1; i<BlockCount; i++, j++) {
        // create new entry if the extent in not contigous
        if( ((NewLba = UDFRelocateSector(Vcb, Lba+i+1)) != (LastLba+1)) ||
            (i==(BlockCount-1)) ) {
            locExt.extLength = j << Vcb->BlockSizeBits;
            locExt.extLocation = LastLba-j+1;
            Extent2 = UDFExtentToMapping(&locExt);
            if(!Extent) {
                Extent = Extent2;
            } else {
                Extent = UDFMergeMappings(Extent, Extent2);
                MyFreePool__(Extent2);
            }
            if(!Extent) return NULL;
            j = 0;
        }
        LastLba = NewLba;
    }
    return Extent;
} // end UDFRelocateSectors()

