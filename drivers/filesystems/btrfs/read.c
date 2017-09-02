/* Copyright (c) Mark Harmstone 2016
 * 
 * This file is part of WinBtrfs.
 * 
 * WinBtrfs is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public Licence as published by
 * the Free Software Foundation, either version 3 of the Licence, or
 * (at your option) any later version.
 * 
 * WinBtrfs is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public Licence for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public Licence
 * along with WinBtrfs.  If not, see <http://www.gnu.org/licenses/>. */

#include "btrfs_drv.h"

enum read_data_status {
    ReadDataStatus_Pending,
    ReadDataStatus_Success,
    ReadDataStatus_Cancelling,
    ReadDataStatus_Cancelled,
    ReadDataStatus_Error,
    ReadDataStatus_CRCError,
    ReadDataStatus_MissingDevice
};

struct read_data_context;

typedef struct {
    struct read_data_context* context;
    UINT8* buf;
    UINT16 stripenum;
    BOOL rewrite;
    PIRP Irp;
    IO_STATUS_BLOCK iosb;
    enum read_data_status status;
} read_data_stripe;

typedef struct {
    KEVENT Event;
    NTSTATUS Status;
    chunk* c;
    UINT64 address;
    UINT32 buflen;
    UINT64 num_stripes;
    LONG stripes_left;
    UINT64 type;
    UINT32 sector_size;
    UINT16 firstoff, startoffstripe, sectors_per_stripe, stripes_cancel;
    UINT32* csum;
    BOOL tree;
    BOOL check_nocsum_parity;
    read_data_stripe* stripes;
    KSPIN_LOCK spin_lock;
} read_data_context;

extern BOOL diskacc;
extern tPsUpdateDiskCounters PsUpdateDiskCounters;
extern tCcCopyReadEx CcCopyReadEx;

static NTSTATUS STDCALL read_data_completion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID conptr) {
    read_data_stripe* stripe = conptr;
    read_data_context* context = (read_data_context*)stripe->context;
    UINT64 i;
    LONG stripes_left;
    KIRQL irql;

    KeAcquireSpinLock(&context->spin_lock, &irql);
    
    stripes_left = InterlockedDecrement(&context->stripes_left);
    
    if (stripe->status == ReadDataStatus_Cancelling) {
        stripe->status = ReadDataStatus_Cancelled;
        goto end;
    }
    
    stripe->iosb = Irp->IoStatus;
    
    if (NT_SUCCESS(Irp->IoStatus.Status)) {
        if (context->type == BLOCK_FLAG_DUPLICATE) {
            stripe->status = ReadDataStatus_Success;

            if (stripes_left > 0 && stripes_left == context->stripes_cancel) {
                for (i = 0; i < context->num_stripes; i++) {
                    if (context->stripes[i].status == ReadDataStatus_Pending) {
                        context->stripes[i].status = ReadDataStatus_Cancelling;
                        IoCancelIrp(context->stripes[i].Irp);
                    }
                }
            }
        } else if (context->type == BLOCK_FLAG_RAID0) {
            stripe->status = ReadDataStatus_Success;
        } else if (context->type == BLOCK_FLAG_RAID10) {
            stripe->status = ReadDataStatus_Success;
            
            if (stripes_left > 0 && context->stripes_cancel != 0) {
                for (i = 0; i < context->num_stripes; i++) {
                    if (context->stripes[i].status == ReadDataStatus_Pending && context->stripes[i].stripenum == stripe->stripenum) {
                        context->stripes[i].status = ReadDataStatus_Cancelling;
                        IoCancelIrp(context->stripes[i].Irp);
                        break;
                    }
                }
            }
        } else if (context->type == BLOCK_FLAG_RAID5) {
            stripe->status = ReadDataStatus_Success;
            
            if (stripes_left > 0 && stripes_left == context->stripes_cancel && (context->csum || context->tree || !context->check_nocsum_parity)) {
                for (i = 0; i < context->num_stripes; i++) {
                    if (context->stripes[i].status == ReadDataStatus_Pending) {
                        context->stripes[i].status = ReadDataStatus_Cancelling;
                        IoCancelIrp(context->stripes[i].Irp);
                        break;
                    }
                }
            }
        } else if (context->type == BLOCK_FLAG_RAID6) {
            stripe->status = ReadDataStatus_Success;

            if (stripes_left > 0 && stripes_left == context->stripes_cancel && (context->csum || context->tree || !context->check_nocsum_parity)) {
                for (i = 0; i < context->num_stripes; i++) {
                    if (context->stripes[i].status == ReadDataStatus_Pending) {
                        context->stripes[i].status = ReadDataStatus_Cancelling;
                        IoCancelIrp(context->stripes[i].Irp);
                    }
                }
            }
        }
            
        goto end;
    } else {
        stripe->status = ReadDataStatus_Error;
    }
    
end:
    KeReleaseSpinLock(&context->spin_lock, irql);
    
    if (stripes_left == 0)
        KeSetEvent(&context->Event, 0, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

static void raid5_reconstruct(UINT64 off, UINT32 skip, read_data_context* context, CHUNK_ITEM* ci, UINT64* stripeoff, UINT64 maxsize,
                              BOOL first, UINT32 firststripesize, UINT16 missing) {
    UINT16 parity, stripe;
    UINT32 stripelen = first ? firststripesize : ci->stripe_length;
    UINT32 readlen;
    
    TRACE("(%llx, %x, %p, %p, %llx, %llx, %u, %x, %x)\n", off, skip, context, ci, *stripeoff, maxsize, first, firststripesize, missing);
    
    parity = ((off / ((ci->num_stripes - 1) * ci->stripe_length)) + ci->num_stripes - 1) % ci->num_stripes;
    
    readlen = min(min(ci->stripe_length - (skip % ci->stripe_length), stripelen), maxsize - *stripeoff);
    
    if (missing != parity) {
        UINT16 firststripe = missing == 0 ? 1 : 0;
        
        RtlCopyMemory(&context->stripes[missing].buf[*stripeoff], &context->stripes[firststripe].buf[*stripeoff], readlen);
        
        for (stripe = firststripe + 1; stripe < context->num_stripes; stripe++) {
            if (stripe != missing)
                do_xor(&context->stripes[missing].buf[*stripeoff], &context->stripes[stripe].buf[*stripeoff], readlen);
        }
    } else
        TRACE("parity == missing == %x, skipping\n", parity);
    
    *stripeoff += stripelen;
}

static void raid5_decode(UINT64 off, UINT32 skip, read_data_context* context, CHUNK_ITEM* ci, UINT64* stripeoff, UINT8* buf,
                         UINT32* pos, UINT32 length, UINT32 firststripesize) {
    UINT16 parity, stripe;
    BOOL first = *pos == 0;
    UINT32 stripelen = first ? firststripesize : ci->stripe_length;
    
    parity = ((off / ((ci->num_stripes - 1) * ci->stripe_length)) + ci->num_stripes - 1) % ci->num_stripes;
    
    stripe = (parity + 1) % ci->num_stripes;
    
    while (TRUE) {
        if (stripe == parity) {
            *stripeoff += stripelen;
            return;
        }
        
        if (skip >= ci->stripe_length) {
            skip -= ci->stripe_length;
        } else {
            UINT32 copylen = min(ci->stripe_length - skip, length - *pos);
            
            RtlCopyMemory(buf + *pos, &context->stripes[stripe].buf[*stripeoff + skip - ci->stripe_length + stripelen], copylen);
            
            *pos += copylen;
            
            if (*pos == length)
                return;
            
            skip = 0;
        }
        
        stripe = (stripe + 1) % ci->num_stripes;
    }
}

static BOOL raid5_decode_with_checksum(UINT64 off, UINT32 skip, read_data_context* context, CHUNK_ITEM* ci, UINT64* stripeoff, UINT8* buf,
                                       UINT32* pos, UINT32 length, UINT32 firststripesize, UINT32* csum, UINT32 sector_size) {
    UINT16 parity, stripe;
    BOOL first = *pos == 0;
    UINT32 stripelen = first ? firststripesize : ci->stripe_length;
    
    parity = ((off / ((ci->num_stripes - 1) * ci->stripe_length)) + ci->num_stripes - 1) % ci->num_stripes;
    
    stripe = (parity + 1) % ci->num_stripes;
    
    while (TRUE) {
        if (stripe == parity) {
            *stripeoff += stripelen;
            return TRUE;
        }
        
        if (skip >= ci->stripe_length) {
            skip -= ci->stripe_length;
        } else {
            UINT32 i;
            UINT32 copylen = min(ci->stripe_length - skip, length - *pos);
            
            RtlCopyMemory(buf + *pos, &context->stripes[stripe].buf[*stripeoff + skip - ci->stripe_length + stripelen], copylen);
            
            for (i = 0; i < copylen / sector_size; i ++) {
                UINT32 crc32 = ~calc_crc32c(0xffffffff, buf + *pos + (i * sector_size), sector_size);
                
                if (crc32 != csum[i]) {
                    UINT16 j, firststripe = stripe == 0 ? 1 : 0;
                    
                    RtlCopyMemory(buf + *pos + (i * sector_size),
                                  &context->stripes[firststripe].buf[*stripeoff + skip - ci->stripe_length + stripelen + (i * sector_size)], sector_size);
                    
                    for (j = firststripe + 1; j < ci->num_stripes; j++) {
                        if (j != stripe) {
                            do_xor(buf + *pos + (i * sector_size), &context->stripes[j].buf[*stripeoff + skip - ci->stripe_length + stripelen + (i * sector_size)], sector_size);
                        }
                    }
                    
                    crc32 = ~calc_crc32c(0xffffffff, buf + *pos + (i * sector_size), sector_size);
                    
                    if (crc32 != csum[i]) {
                        ERR("unrecoverable checksum error\n");
                        return FALSE;
                    }
                    
                    RtlCopyMemory(&context->stripes[stripe].buf[*stripeoff + skip - ci->stripe_length + stripelen + (i * sector_size)], buf + *pos + (i * sector_size), sector_size);
                    context->stripes[stripe].rewrite = TRUE;
                }
            }
            
            *pos += copylen;
            
            if (*pos == length)
                return TRUE;
            
            skip = 0;
        }
        
        stripe = (stripe + 1) % ci->num_stripes;
    }
    
    return FALSE;
}

static BOOL raid5_decode_with_checksum_metadata(UINT64 addr, UINT64 off, UINT32 skip, read_data_context* context, CHUNK_ITEM* ci, UINT64* stripeoff, UINT8* buf,
                                                UINT32* pos, UINT32 length, UINT32 firststripesize, UINT32 node_size) {
    UINT16 parity, stripe;
    BOOL first = *pos == 0;
    UINT32 stripelen = first ? firststripesize : ci->stripe_length;
    
    parity = ((off / ((ci->num_stripes - 1) * ci->stripe_length)) + ci->num_stripes - 1) % ci->num_stripes;
    
    stripe = (parity + 1) % ci->num_stripes;
    
    while (TRUE) {
        if (stripe == parity) {
            *stripeoff += stripelen;
            return TRUE;
        }
        
        if (skip >= ci->stripe_length) {
            skip -= ci->stripe_length;
        } else {
            UINT32 copylen = min(ci->stripe_length - skip, length - *pos);
            tree_header* th = (tree_header*)buf;
            UINT32 crc32;
            
            RtlCopyMemory(buf + *pos, &context->stripes[stripe].buf[*stripeoff + skip - ci->stripe_length + stripelen], copylen);
            
            crc32 = ~calc_crc32c(0xffffffff, (UINT8*)&th->fs_uuid, node_size - sizeof(th->csum));
            
            if (addr != th->address || crc32 != *((UINT32*)th->csum)) {
                UINT16 j, firststripe = stripe == 0 ? 1 : 0;
                
                RtlCopyMemory(buf + *pos, &context->stripes[firststripe].buf[*stripeoff + skip - ci->stripe_length + stripelen], copylen);
                
                for (j = firststripe + 1; j < ci->num_stripes; j++) {
                    if (j != stripe) {
                        do_xor(buf + *pos, &context->stripes[j].buf[*stripeoff + skip - ci->stripe_length + stripelen], copylen);
                    }
                }
                
                crc32 = ~calc_crc32c(0xffffffff, (UINT8*)&th->fs_uuid, node_size - sizeof(th->csum));
                
                if (addr != th->address || crc32 != *((UINT32*)th->csum)) {
                    ERR("unrecoverable checksum error\n");
                    return FALSE;
                }
            }
            
            RtlCopyMemory(&context->stripes[stripe].buf[*stripeoff + skip - ci->stripe_length + stripelen], buf + *pos, copylen);
            context->stripes[stripe].rewrite = TRUE;
            
            *pos += copylen;
            
            if (*pos == length)
                return TRUE;
            
            skip = 0;
        }
        
        stripe = (stripe + 1) % ci->num_stripes;
    }
    
    return FALSE;
}

static void raid6_reconstruct1(UINT64 off, UINT32 skip, read_data_context* context, CHUNK_ITEM* ci, UINT64* stripeoff, UINT64 maxsize,
                               BOOL first, UINT32 firststripesize, UINT16 missing) {
    UINT16 parity1, parity2, stripe;
    UINT32 stripelen = first ? firststripesize : ci->stripe_length;
    UINT32 readlen;
    
    TRACE("(%llx, %x, %p, %p, %llx, %llx, %u, %x, %x)\n", off, skip, context, ci, *stripeoff, maxsize, first, firststripesize, missing);
    
    parity1 = ((off / ((ci->num_stripes - 2) * ci->stripe_length)) + ci->num_stripes - 2) % ci->num_stripes;
    parity2 = (parity1 + 1) % ci->num_stripes;
    
    readlen = min(min(ci->stripe_length - (skip % ci->stripe_length), stripelen), maxsize - *stripeoff);
    
    if (missing != parity1 && missing != parity2) {
        RtlCopyMemory(&context->stripes[missing].buf[*stripeoff], &context->stripes[parity1].buf[*stripeoff], readlen);
        stripe = parity1 == 0 ? (ci->num_stripes - 1) : (parity1 - 1);
        
        do {
            if (stripe != missing)
                do_xor(&context->stripes[missing].buf[*stripeoff], &context->stripes[stripe].buf[*stripeoff], readlen);
            
            stripe = stripe == 0 ? (ci->num_stripes - 1) : (stripe - 1);
        } while (stripe != parity2);
    } else
        TRACE("skipping parity stripe\n");
    
    *stripeoff += stripelen;
}

static void raid6_reconstruct2(UINT64 off, UINT32 skip, read_data_context* context, CHUNK_ITEM* ci, UINT64* stripeoff, UINT64 maxsize,
                               BOOL first, UINT32 firststripesize, UINT16 missing1, UINT16 missing2) {
    UINT16 parity1, parity2, stripe;
    UINT32 stripelen = first ? firststripesize : ci->stripe_length;
    UINT32 readlen = min(min(ci->stripe_length - (skip % ci->stripe_length), stripelen), maxsize - *stripeoff);
    
    TRACE("(%llx, %x, %p, %p, %llx, %llx, %u, %x, %x, %x)\n", off, skip, context, ci, *stripeoff, maxsize,
          first, firststripesize, missing1, missing2);
    
    parity1 = ((off / ((ci->num_stripes - 2) * ci->stripe_length)) + ci->num_stripes - 2) % ci->num_stripes;
    parity2 = (parity1 + 1) % ci->num_stripes;
    
    // skip if missing stripes are p and q
    if ((parity1 == missing1 && parity2 == missing2) || (parity1 == missing2 && parity2 == missing1)) {
        *stripeoff += stripelen;
        return;
    }
    
    if (missing1 == parity2 || missing2 == parity2) { // reconstruct from p and data
        UINT16 missing = missing1 == parity2 ? missing2 : missing1;
        
        RtlCopyMemory(&context->stripes[missing].buf[*stripeoff], &context->stripes[parity1].buf[*stripeoff], readlen);
        stripe = parity1 == 0 ? (ci->num_stripes - 1) : (parity1 - 1);
        
        do {
            if (stripe != missing)
                do_xor(&context->stripes[missing].buf[*stripeoff], &context->stripes[stripe].buf[*stripeoff], readlen);
            
            stripe = stripe == 0 ? (ci->num_stripes - 1) : (stripe - 1);
        } while (stripe != parity2);
    } else if (missing1 == parity1 || missing2 == parity1) { // reconstruct from q and data
        UINT16 missing = missing1 == parity1 ? missing2 : missing1;
        UINT16 i, div;
        
        stripe = parity1 == 0 ? (ci->num_stripes - 1) : (parity1 - 1);
        
        i = ci->num_stripes - 3;
        
        if (stripe == missing) {
            RtlZeroMemory(&context->stripes[missing].buf[*stripeoff], readlen);
            div = i;
        } else
            RtlCopyMemory(&context->stripes[missing].buf[*stripeoff], &context->stripes[stripe].buf[*stripeoff], readlen);
        
        stripe = stripe == 0 ? (ci->num_stripes - 1) : (stripe - 1);
        
        i--;
        do {
            galois_double(&context->stripes[missing].buf[*stripeoff], readlen);
            
            if (stripe != missing)
                do_xor(&context->stripes[missing].buf[*stripeoff], &context->stripes[stripe].buf[*stripeoff], readlen);
            else
                div = i;
            
            stripe = stripe == 0 ? (ci->num_stripes - 1) : (stripe - 1);
            i--;
        } while (stripe != parity2);
        
        do_xor(&context->stripes[missing].buf[*stripeoff], &context->stripes[parity2].buf[*stripeoff], readlen);
        
        if (div != 0)
            galois_divpower(&context->stripes[missing].buf[*stripeoff], div, readlen);
    } else { // reconstruct from p and q
        UINT16 x, y, i;
        UINT8 gyx, gx, denom, a, b, *p, *q, *pxy, *qxy;
        UINT32 j;
        
        stripe = parity1 == 0 ? (ci->num_stripes - 1) : (parity1 - 1);
        
        // put qxy in missing1
        // put pxy in missing2
        
        i = ci->num_stripes - 3;
        if (stripe == missing1 || stripe == missing2) {
            RtlZeroMemory(&context->stripes[missing1].buf[*stripeoff], readlen);
            RtlZeroMemory(&context->stripes[missing2].buf[*stripeoff], readlen);
            
            if (stripe == missing1)
                x = i;
            else
                y = i;
        } else {
            RtlCopyMemory(&context->stripes[missing1].buf[*stripeoff], &context->stripes[stripe].buf[*stripeoff], readlen);
            RtlCopyMemory(&context->stripes[missing2].buf[*stripeoff], &context->stripes[stripe].buf[*stripeoff], readlen);
        }
        
        stripe = stripe == 0 ? (ci->num_stripes - 1) : (stripe - 1);
        
        i--;
        do {
            galois_double(&context->stripes[missing1].buf[*stripeoff], readlen);
            
            if (stripe != missing1 && stripe != missing2) {
                do_xor(&context->stripes[missing1].buf[*stripeoff], &context->stripes[stripe].buf[*stripeoff], readlen);
                do_xor(&context->stripes[missing2].buf[*stripeoff], &context->stripes[stripe].buf[*stripeoff], readlen);
            } else if (stripe == missing1)
                x = i;
            else if (stripe == missing2)
                y = i;
            
            stripe = stripe == 0 ? (ci->num_stripes - 1) : (stripe - 1);
            i--;
        } while (stripe != parity2);
        
        gyx = gpow2(y > x ? (y-x) : (255-x+y));
        gx = gpow2(255-x);

        denom = gdiv(1, gyx ^ 1);
        a = gmul(gyx, denom);
        b = gmul(gx, denom);
        
        p = &context->stripes[parity1].buf[*stripeoff];
        q = &context->stripes[parity2].buf[*stripeoff];
        pxy = &context->stripes[missing2].buf[*stripeoff];
        qxy = &context->stripes[missing1].buf[*stripeoff]; 
        
        for (j = 0; j < readlen; j++) {
            *qxy = gmul(a, *p ^ *pxy) ^ gmul(b, *q ^ *qxy);
            
            p++;
            q++;
            pxy++;
            qxy++;
        }
        
        do_xor(&context->stripes[missing2].buf[*stripeoff], &context->stripes[missing1].buf[*stripeoff], readlen);
        do_xor(&context->stripes[missing2].buf[*stripeoff], &context->stripes[parity1].buf[*stripeoff], readlen);
    }
    
    *stripeoff += stripelen;
}

static void raid6_decode(UINT64 off, UINT32 skip, read_data_context* context, CHUNK_ITEM* ci, UINT64* stripeoff, UINT8* buf,
                         UINT32* pos, UINT32 length, UINT32 firststripesize) {
    UINT16 parity1, stripe;
    BOOL first = *pos == 0;
    UINT32 stripelen = first ? firststripesize : ci->stripe_length;
    
    parity1 = ((off / ((ci->num_stripes - 2) * ci->stripe_length)) + ci->num_stripes - 2) % ci->num_stripes;
    
    stripe = (parity1 + 2) % ci->num_stripes;
    
    while (TRUE) {
        if (stripe == parity1) {
            *stripeoff += stripelen;
            return;
        }
        
        if (skip >= ci->stripe_length) {
            skip -= ci->stripe_length;
        } else {
            UINT32 copylen = min(ci->stripe_length - skip, length - *pos);
            
            RtlCopyMemory(buf + *pos, &context->stripes[stripe].buf[*stripeoff + skip - ci->stripe_length + stripelen], copylen);
            
            *pos += copylen;
            
            if (*pos == length)
                return;
            
            skip = 0;
        }
        
        stripe = (stripe + 1) % ci->num_stripes;
    }
}

static BOOL raid6_decode_with_checksum(UINT64 off, UINT32 skip, read_data_context* context, CHUNK_ITEM* ci, UINT64* stripeoff, UINT8* buf,
                                       UINT32* pos, UINT32 length, UINT32 firststripesize, UINT32* csum, UINT32 sector_size) {
    UINT16 parity1, parity2, stripe;
    BOOL first = *pos == 0;
    UINT32 stripelen = first ? firststripesize : ci->stripe_length;
    
    parity1 = ((off / ((ci->num_stripes - 2) * ci->stripe_length)) + ci->num_stripes - 2) % ci->num_stripes;
    parity2 = (parity1 + 1) % ci->num_stripes;
    stripe = (parity1 + 2) % ci->num_stripes;
    
    while (TRUE) {
        if (stripe == parity1) {
            *stripeoff += stripelen;
            return TRUE;
        }
        
        if (skip >= ci->stripe_length) {
            skip -= ci->stripe_length;
        } else {
            UINT32 i;
            UINT32 copylen = min(ci->stripe_length - skip, length - *pos);
            
            RtlCopyMemory(buf + *pos, &context->stripes[stripe].buf[*stripeoff + skip - ci->stripe_length + stripelen], copylen);
            
            for (i = 0; i < copylen / sector_size; i ++) {
                UINT32 crc32 = ~calc_crc32c(0xffffffff, buf + *pos + (i * sector_size), sector_size);
                
                if (crc32 != csum[i]) {
                    UINT16 j, firststripe;
                    
                    if (parity2 == 0 && stripe == 1)
                        firststripe = 2;
                    else if (parity2 == 0 || stripe == 0)
                        firststripe = 1;
                    else
                        firststripe = 0;
                    
                    RtlCopyMemory(buf + *pos + (i * sector_size),
                                  &context->stripes[firststripe].buf[*stripeoff + skip - ci->stripe_length + stripelen + (i * sector_size)], sector_size);
                    
                    for (j = firststripe + 1; j < ci->num_stripes; j++) {
                        if (j != stripe && j != parity2) {
                            do_xor(buf + *pos + (i * sector_size), &context->stripes[j].buf[*stripeoff + skip - ci->stripe_length + stripelen + (i * sector_size)], sector_size);
                        }
                    }
                    
                    crc32 = ~calc_crc32c(0xffffffff, buf + *pos + (i * sector_size), sector_size);
                    
                    if (crc32 != csum[i]) {
                        UINT8 *parity, *buf2;
                        UINT16 rs, div;
                        
                        // assume p is wrong
                        
                        parity = ExAllocatePoolWithTag(NonPagedPool, sector_size, ALLOC_TAG);
                        if (!parity) {
                            ERR("out of memory\n");
                            return FALSE;
                        }
                        
                        rs = (parity1 + ci->num_stripes - 1) % ci->num_stripes;
                        j = ci->num_stripes - 3;
                        
                        if (rs == stripe) {
                            RtlZeroMemory(parity, sector_size);
                            div = j;
                        } else
                            RtlCopyMemory(parity, &context->stripes[rs].buf[*stripeoff + skip - ci->stripe_length + stripelen + (i * sector_size)], sector_size);
                        
                        rs = (rs + ci->num_stripes - 1) % ci->num_stripes;
                        j--;
                        while (rs != parity2) {
                            galois_double(parity, sector_size);
                            
                            if (rs != stripe)
                                do_xor(parity, &context->stripes[rs].buf[*stripeoff + skip - ci->stripe_length + stripelen + (i * sector_size)], sector_size);
                            else
                                div = j;
            
                            rs = (rs + ci->num_stripes - 1) % ci->num_stripes;
                            j--;
                        }
                        
                        do_xor(parity, &context->stripes[parity2].buf[*stripeoff + skip - ci->stripe_length + stripelen + (i * sector_size)], sector_size);
                        
                        if (div != 0)
                            galois_divpower(parity, div, sector_size);
                        
                        crc32 = ~calc_crc32c(0xffffffff, parity, sector_size);
                        if (crc32 == csum[i]) {
                            RtlCopyMemory(buf + *pos + (i * sector_size), parity, sector_size);
                            
                            // recalculate p
                            RtlCopyMemory(&context->stripes[parity1].buf[*stripeoff + skip - ci->stripe_length + stripelen + (i * sector_size)], parity, sector_size);
                            
                            for (j = 0; j < ci->num_stripes; j++) {
                                if (j != stripe && j != parity1 && j != parity2) {
                                    do_xor(&context->stripes[parity1].buf[*stripeoff + skip - ci->stripe_length + stripelen + (i * sector_size)],
                                           &context->stripes[j].buf[*stripeoff + skip - ci->stripe_length + stripelen + (i * sector_size)], sector_size);
                                }
                            }
                            
                            context->stripes[parity1].rewrite = TRUE;
                            
                            ExFreePool(parity);
                            goto success;
                        }
                        
                        // assume another of the data stripes is wrong
                        
                        buf2 = ExAllocatePoolWithTag(NonPagedPool, sector_size, ALLOC_TAG);
                        if (!buf2) {
                            ERR("out of memory\n");
                            ExFreePool(parity);
                            return FALSE;
                        }
                        
                        j = (parity2 + 1) % ci->num_stripes;
                        
                        while (j != parity1) {
                            if (j != stripe) {
                                UINT16 curstripe, k;
                                UINT32 bufoff = *stripeoff + skip - ci->stripe_length + stripelen + (i * sector_size);
                                UINT16 x, y;
                                UINT8 gyx, gx, denom, a, b, *p, *q, *pxy, *qxy;
                            
                                curstripe = parity1 == 0 ? (ci->num_stripes - 1) : (parity1 - 1);
                                
                                // put qxy in parity
                                // put pxy in buf2
                                
                                k = ci->num_stripes - 3;
                                if (curstripe == stripe || curstripe == j) {
                                    RtlZeroMemory(parity, sector_size);
                                    RtlZeroMemory(buf2, sector_size);
                                    
                                    if (curstripe == stripe)
                                        x = k;
                                    else
                                        y = k;
                                } else {
                                    RtlCopyMemory(parity, &context->stripes[curstripe].buf[bufoff], sector_size);
                                    RtlCopyMemory(buf2, &context->stripes[curstripe].buf[bufoff], sector_size);
                                }
                                
                                curstripe = curstripe == 0 ? (ci->num_stripes - 1) : (curstripe - 1);
                                
                                k--;
                                do {
                                    galois_double(parity, sector_size);
                                    
                                    if (curstripe != stripe && curstripe != j) {
                                        do_xor(parity, &context->stripes[curstripe].buf[bufoff], sector_size);
                                        do_xor(buf2, &context->stripes[curstripe].buf[bufoff], sector_size);
                                    } else if (curstripe == stripe)
                                        x = k;
                                    else if (curstripe == j)
                                        y = k;
                                    
                                    curstripe = curstripe == 0 ? (ci->num_stripes - 1) : (curstripe - 1);
                                    k--;
                                } while (curstripe != parity2);
                                
                                gyx = gpow2(y > x ? (y-x) : (255-x+y));
                                gx = gpow2(255-x);

                                denom = gdiv(1, gyx ^ 1);
                                a = gmul(gyx, denom);
                                b = gmul(gx, denom);
                                
                                p = &context->stripes[parity1].buf[bufoff];
                                q = &context->stripes[parity2].buf[bufoff];
                                pxy = buf2;
                                qxy = parity; 
                                
                                for (k = 0; k < sector_size; k++) {
                                    *qxy = gmul(a, *p ^ *pxy) ^ gmul(b, *q ^ *qxy);
                                    
                                    p++;
                                    q++;
                                    pxy++;
                                    qxy++;
                                }
                                
                                crc32 = ~calc_crc32c(0xffffffff, parity, sector_size);
                                
                                if (crc32 == csum[i]) {
                                    do_xor(buf2, parity, sector_size);
                                    do_xor(buf2, &context->stripes[parity1].buf[bufoff], sector_size);
                                    
                                    RtlCopyMemory(&context->stripes[j].buf[bufoff], buf2, sector_size);
                                    context->stripes[j].rewrite = TRUE;
                                    
                                    RtlCopyMemory(buf + *pos + (i * sector_size), parity, sector_size);
                                    ExFreePool(parity);
                                    ExFreePool(buf2);
                                    goto success;
                                }
                            }
                            
                            j = (j + 1) % ci->num_stripes;
                        }
                            
                        ExFreePool(parity);
                        ExFreePool(buf2);
                        
                        ERR("unrecoverable checksum error\n");
                        return FALSE;
                    }
                    
success:
                    RtlCopyMemory(&context->stripes[stripe].buf[*stripeoff + skip - ci->stripe_length + stripelen + (i * sector_size)], buf + *pos + (i * sector_size), sector_size);
                    context->stripes[stripe].rewrite = TRUE;
                }
            }
            
            *pos += copylen;
            
            if (*pos == length)
                return TRUE;
            
            skip = 0;
        }
        
        stripe = (stripe + 1) % ci->num_stripes;
    }
}

static BOOL raid6_decode_with_checksum_metadata(UINT64 addr, UINT64 off, UINT32 skip, read_data_context* context, CHUNK_ITEM* ci, UINT64* stripeoff, UINT8* buf,
                                                UINT32* pos, UINT32 length, UINT32 firststripesize, UINT32 node_size) {
    UINT16 parity1, parity2, stripe;
    BOOL first = *pos == 0;
    UINT32 stripelen = first ? firststripesize : ci->stripe_length;
    
    parity1 = ((off / ((ci->num_stripes - 2) * ci->stripe_length)) + ci->num_stripes - 2) % ci->num_stripes;
    parity2 = (parity1 + 1) % ci->num_stripes;
    stripe = (parity1 + 2) % ci->num_stripes;
    
    while (TRUE) {
        if (stripe == parity1) {
            *stripeoff += stripelen;
            return TRUE;
        }
        
        if (skip >= ci->stripe_length) {
            skip -= ci->stripe_length;
        } else {
            UINT32 copylen = min(ci->stripe_length - skip, length - *pos);
            tree_header* th = (tree_header*)buf;
            UINT32 crc32;
            
            RtlCopyMemory(buf + *pos, &context->stripes[stripe].buf[*stripeoff + skip - ci->stripe_length + stripelen], copylen);
            
            crc32 = ~calc_crc32c(0xffffffff, (UINT8*)&th->fs_uuid, node_size - sizeof(th->csum));
            
            if (addr != th->address || crc32 != *((UINT32*)th->csum)) {
                UINT16 j, firststripe;
                
                if (parity2 == 0 && stripe == 1)
                    firststripe = 2;
                else if (parity2 == 0 || stripe == 0)
                    firststripe = 1;
                else
                    firststripe = 0;
                
                RtlCopyMemory(buf + *pos, &context->stripes[firststripe].buf[*stripeoff + skip - ci->stripe_length + stripelen], node_size);
                
                for (j = firststripe + 1; j < ci->num_stripes; j++) {
                    if (j != stripe && j != parity2) {
                        do_xor(buf + *pos, &context->stripes[j].buf[*stripeoff + skip - ci->stripe_length + stripelen], node_size);
                    }
                }
                
                crc32 = ~calc_crc32c(0xffffffff, (UINT8*)&th->fs_uuid, node_size - sizeof(th->csum));
                
                if (addr != th->address || crc32 != *((UINT32*)th->csum)) {
                    UINT8 *parity, *buf2;
                    UINT16 rs, div;
                    tree_header* th2;
                    
                    // assume p is wrong
                    
                    parity = ExAllocatePoolWithTag(NonPagedPool, node_size, ALLOC_TAG);
                    if (!parity) {
                        ERR("out of memory\n");
                        return FALSE;
                    }
                    
                    rs = (parity1 + ci->num_stripes - 1) % ci->num_stripes;
                    j = ci->num_stripes - 3;
                    
                    if (rs == stripe) {
                        RtlZeroMemory(parity, node_size);
                        div = j;
                    } else
                        RtlCopyMemory(parity, &context->stripes[rs].buf[*stripeoff + skip - ci->stripe_length + stripelen], node_size);
                    
                    rs = (rs + ci->num_stripes - 1) % ci->num_stripes;
                    j--;
                    while (rs != parity2) {
                        galois_double(parity, node_size);
                        
                        if (rs != stripe)
                            do_xor(parity, &context->stripes[rs].buf[*stripeoff + skip - ci->stripe_length + stripelen], node_size);
                        else
                            div = j;
        
                        rs = (rs + ci->num_stripes - 1) % ci->num_stripes;
                        j--;
                    }
                    
                    do_xor(parity, &context->stripes[parity2].buf[*stripeoff + skip - ci->stripe_length + stripelen], node_size);
                    
                    if (div != 0)
                        galois_divpower(parity, div, node_size);
                    
                    th2 = (tree_header*)parity;
                    
                    crc32 = ~calc_crc32c(0xffffffff, (UINT8*)&th2->fs_uuid, node_size - sizeof(th2->csum));
                
                    if (addr != th2->address || crc32 == *((UINT32*)th2->csum)) {
                        RtlCopyMemory(buf + *pos, parity, node_size);
                        
                        // recalculate p
                        RtlCopyMemory(&context->stripes[parity1].buf[*stripeoff + skip - ci->stripe_length + stripelen], parity, node_size);
                        
                        for (j = 0; j < ci->num_stripes; j++) {
                            if (j != stripe && j != parity1 && j != parity2) {
                                do_xor(&context->stripes[parity1].buf[*stripeoff + skip - ci->stripe_length + stripelen],
                                        &context->stripes[j].buf[*stripeoff + skip - ci->stripe_length + stripelen], node_size);
                            }
                        }
                        
                        context->stripes[parity1].rewrite = TRUE;
                        
                        ExFreePool(parity);
                        goto success;
                    }
                    
                    // assume another of the data stripes is wrong
                    
                    buf2 = ExAllocatePoolWithTag(NonPagedPool, node_size, ALLOC_TAG);
                    if (!buf2) {
                        ERR("out of memory\n");
                        ExFreePool(parity);
                        return FALSE;
                    }
                    
                    j = (parity2 + 1) % ci->num_stripes;
                    
                    while (j != parity1) {
                        if (j != stripe) {
                            UINT16 curstripe, k;
                            UINT32 bufoff = *stripeoff + skip - ci->stripe_length + stripelen;
                            UINT16 x, y;
                            UINT8 gyx, gx, denom, a, b, *p, *q, *pxy, *qxy;
                        
                            curstripe = parity1 == 0 ? (ci->num_stripes - 1) : (parity1 - 1);
                            
                            // put qxy in parity
                            // put pxy in buf2
                            
                            k = ci->num_stripes - 3;
                            if (curstripe == stripe || curstripe == j) {
                                RtlZeroMemory(parity, node_size);
                                RtlZeroMemory(buf2, node_size);
                                
                                if (curstripe == stripe)
                                    x = k;
                                else
                                    y = k;
                            } else {
                                RtlCopyMemory(parity, &context->stripes[curstripe].buf[bufoff], node_size);
                                RtlCopyMemory(buf2, &context->stripes[curstripe].buf[bufoff], node_size);
                            }
                            
                            curstripe = curstripe == 0 ? (ci->num_stripes - 1) : (curstripe - 1);
                            
                            k--;
                            do {
                                galois_double(parity, node_size);
                                
                                if (curstripe != stripe && curstripe != j) {
                                    do_xor(parity, &context->stripes[curstripe].buf[bufoff], node_size);
                                    do_xor(buf2, &context->stripes[curstripe].buf[bufoff], node_size);
                                } else if (curstripe == stripe)
                                    x = k;
                                else if (curstripe == j)
                                    y = k;
                                
                                curstripe = curstripe == 0 ? (ci->num_stripes - 1) : (curstripe - 1);
                                k--;
                            } while (curstripe != parity2);
                            
                            gyx = gpow2(y > x ? (y-x) : (255-x+y));
                            gx = gpow2(255-x);

                            denom = gdiv(1, gyx ^ 1);
                            a = gmul(gyx, denom);
                            b = gmul(gx, denom);
                            
                            p = &context->stripes[parity1].buf[bufoff];
                            q = &context->stripes[parity2].buf[bufoff];
                            pxy = buf2;
                            qxy = parity; 
                            
                            for (k = 0; k < node_size; k++) {
                                *qxy = gmul(a, *p ^ *pxy) ^ gmul(b, *q ^ *qxy);
                                
                                p++;
                                q++;
                                pxy++;
                                qxy++;
                            }
                            
                            th2 = (tree_header*)parity;
                    
                            crc32 = ~calc_crc32c(0xffffffff, (UINT8*)&th2->fs_uuid, node_size - sizeof(th2->csum));
                        
                            if (addr != th2->address || crc32 == *((UINT32*)th2->csum)) {
                                do_xor(buf2, parity, node_size);
                                do_xor(buf2, &context->stripes[parity1].buf[bufoff], node_size);
                                
                                RtlCopyMemory(&context->stripes[j].buf[bufoff], buf2, node_size);
                                context->stripes[j].rewrite = TRUE;
                                
                                RtlCopyMemory(buf + *pos, parity, node_size);
                                ExFreePool(parity);
                                ExFreePool(buf2);
                                goto success;
                            }
                        }
                        
                        j = (j + 1) % ci->num_stripes;
                    }
                        
                    ExFreePool(parity);
                    ExFreePool(buf2);
                    
                    ERR("unrecoverable checksum error\n");
                    return FALSE;
                }
                
success:
                RtlCopyMemory(&context->stripes[stripe].buf[*stripeoff + skip - ci->stripe_length + stripelen], buf + *pos, node_size);
                context->stripes[stripe].rewrite = TRUE;
            }
            
            *pos += copylen;
            
            if (*pos == length)
                return TRUE;
            
            skip = 0;
        }
        
        stripe = (stripe + 1) % ci->num_stripes;
    }
}

static NTSTATUS check_raid6_nocsum_parity(UINT64 off, UINT32 skip, read_data_context* context, CHUNK_ITEM* ci, UINT64* stripeoff, UINT64 maxsize,
                                          BOOL first, UINT32 firststripesize, UINT8* scratch) {
    UINT16 parity1, parity2, stripe;
    UINT32 stripelen = first ? firststripesize : ci->stripe_length;
    UINT32 readlen, i;
    BOOL bad = FALSE;
    
    TRACE("(%llx, %x, %p, %p, %llx, %llx, %u, %x, %p)\n", off, skip, context, ci, *stripeoff, maxsize, first, firststripesize, scratch);
    
    parity1 = ((off / ((ci->num_stripes - 2) * ci->stripe_length)) + ci->num_stripes - 2) % ci->num_stripes;
    parity2 = (parity1 + 1) % ci->num_stripes;
    
    readlen = min(min(ci->stripe_length - (skip % ci->stripe_length), stripelen), maxsize - *stripeoff);
    
    RtlCopyMemory(scratch, &context->stripes[parity1].buf[*stripeoff], readlen);
    stripe = parity1 == 0 ? (ci->num_stripes - 1) : (parity1 - 1);
    
    do {
        do_xor(scratch, &context->stripes[stripe].buf[*stripeoff], readlen);
        
        stripe = stripe == 0 ? (ci->num_stripes - 1) : (stripe - 1);
    } while (stripe != parity2);
    
    for (i = 0; i < readlen; i++) {
        if (scratch[i] != 0) {
            bad = TRUE;
            break;
        }
    }
    
    if (bad) {
        UINT16 missing;
        UINT8* buf2;
        
        // assume parity is bad
        stripe = parity1 == 0 ? (ci->num_stripes - 1) : (parity1 - 1);
        RtlCopyMemory(scratch, &context->stripes[stripe].buf[*stripeoff], readlen);
        stripe = stripe == 0 ? (ci->num_stripes - 1) : (stripe - 1);
        
        do {
            galois_double(scratch, readlen);
            
            do_xor(scratch, &context->stripes[stripe].buf[*stripeoff], readlen);
            
            stripe = stripe == 0 ? (ci->num_stripes - 1) : (stripe - 1);
        } while (stripe != parity2);
        
        if (RtlCompareMemory(scratch, &context->stripes[parity2].buf[*stripeoff], readlen) == readlen) {
            WARN("recovering from invalid parity stripe\n");
            
            // recalc p
            stripe = parity1 == 0 ? (ci->num_stripes - 1) : (parity1 - 1);
            RtlCopyMemory(&context->stripes[parity1].buf[*stripeoff], &context->stripes[stripe].buf[*stripeoff], readlen);
            stripe = stripe == 0 ? (ci->num_stripes - 1) : (stripe - 1);
        
            do {
                do_xor(&context->stripes[parity1].buf[*stripeoff], &context->stripes[stripe].buf[*stripeoff], readlen);
                
                stripe = stripe == 0 ? (ci->num_stripes - 1) : (stripe - 1);
            } while (stripe != parity2);
            
            context->stripes[parity1].rewrite = TRUE;
            goto end;
        }
        
        // assume one of the data stripes is bad
        
        buf2 = ExAllocatePoolWithTag(NonPagedPool, readlen, ALLOC_TAG);
        if (!buf2) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        missing = (parity2 + 1) % ci->num_stripes;
        while (missing != parity1) {
            RtlCopyMemory(scratch, &context->stripes[parity1].buf[*stripeoff], readlen);
            for (i = 0; i < ci->num_stripes; i++) {
                if (i != parity1 && i != parity2 && i != missing) {
                    do_xor(scratch, &context->stripes[i].buf[*stripeoff], readlen);
                }
            }
            
            stripe = parity1 == 0 ? (ci->num_stripes - 1) : (parity1 - 1);
            RtlCopyMemory(buf2, stripe == missing ? scratch : &context->stripes[stripe].buf[*stripeoff], readlen);
            stripe = stripe == 0 ? (ci->num_stripes - 1) : (stripe - 1);
            
            do {
                galois_double(buf2, readlen);
                
                do_xor(buf2, stripe == missing ? scratch : &context->stripes[stripe].buf[*stripeoff], readlen);
                
                stripe = stripe == 0 ? (ci->num_stripes - 1) : (stripe - 1);
            } while (stripe != parity2);
            
            if (RtlCompareMemory(buf2, &context->stripes[parity2].buf[*stripeoff], readlen) == readlen) {
                WARN("recovering from invalid data stripe\n");
                
                RtlCopyMemory(&context->stripes[missing].buf[*stripeoff], scratch, readlen);
                ExFreePool(buf2);
                
                context->stripes[missing].rewrite = TRUE;
                goto end;
            }
            
            missing = (missing + 1) % ci->num_stripes;
        }
        
        ExFreePool(buf2);
        
        ERR("unrecoverable checksum error\n");
        return STATUS_CRC_ERROR;
    }
    
end:
    *stripeoff += stripelen;
    
    return STATUS_SUCCESS;
}

static NTSTATUS check_csum(device_extension* Vcb, UINT8* data, UINT32 sectors, UINT32* csum) {
    NTSTATUS Status;
    calc_job* cj;
    UINT32* csum2;
    
    // From experimenting, it seems that 40 sectors is roughly the crossover
    // point where offloading the crc32 calculation becomes worth it.
    
    if (sectors < 40) {
        ULONG j;
        
        for (j = 0; j < sectors; j++) {
            UINT32 crc32 = ~calc_crc32c(0xffffffff, data + (j * Vcb->superblock.sector_size), Vcb->superblock.sector_size);
            
            if (crc32 != csum[j]) {
                return STATUS_CRC_ERROR;
            }
        }
        
        return STATUS_SUCCESS;
    }
    
    csum2 = ExAllocatePoolWithTag(PagedPool, sizeof(UINT32) * sectors, ALLOC_TAG);
    if (!csum2) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    Status = add_calc_job(Vcb, data, sectors, csum2, &cj);
    if (!NT_SUCCESS(Status)) {
        ERR("add_calc_job returned %08x\n", Status);
        return Status;
    }
    
    KeWaitForSingleObject(&cj->event, Executive, KernelMode, FALSE, NULL);
    
    if (RtlCompareMemory(csum2, csum, sectors * sizeof(UINT32)) != sectors * sizeof(UINT32)) {
        free_calc_job(cj);
        ExFreePool(csum2);
        return STATUS_CRC_ERROR;
    }
    
    free_calc_job(cj);
    ExFreePool(csum2);
    
    return STATUS_SUCCESS;
}

static NTSTATUS read_data_dup(device_extension* Vcb, UINT8* buf, UINT64 addr, UINT32 length, PIRP Irp, read_data_context* context,
                              CHUNK_ITEM* ci, device** devices, UINT64 *stripestart, UINT64 *stripeend) {
    UINT64 i;
    BOOL checksum_error = FALSE;
    UINT16 cancelled = 0;
    NTSTATUS Status;
    
    for (i = 0; i < ci->num_stripes; i++) {
        if (context->stripes[i].status == ReadDataStatus_Success) {
            if (context->tree) {
                tree_header* th = (tree_header*)context->stripes[i].buf;
                UINT32 crc32;
                
                crc32 = ~calc_crc32c(0xffffffff, (UINT8*)&th->fs_uuid, context->buflen - sizeof(th->csum));
                
                if (th->address != context->address || crc32 != *((UINT32*)th->csum)) {
                    context->stripes[i].status = ReadDataStatus_CRCError;
                    checksum_error = TRUE;
                }
            } else if (context->csum) {
#ifdef DEBUG_STATS
                LARGE_INTEGER time1, time2;
                
                time1 = KeQueryPerformanceCounter(NULL);
#endif
                Status = check_csum(Vcb, context->stripes[i].buf, context->stripes[i].Irp->IoStatus.Information / context->sector_size, context->csum);
                
                if (Status == STATUS_CRC_ERROR) {
                    context->stripes[i].status = ReadDataStatus_CRCError;
                    checksum_error = TRUE;
                    break;
                } else if (!NT_SUCCESS(Status)) {
                    ERR("check_csum returned %08x\n", Status);
                    return Status;
                }
#ifdef DEBUG_STATS
                time2 = KeQueryPerformanceCounter(NULL);
                
                Vcb->stats.read_csum_time += time2.QuadPart - time1.QuadPart;
#endif
            }
        } else if (context->stripes[i].status == ReadDataStatus_Cancelled) {
            cancelled++;
        }
    }
    
    if (checksum_error) {
        CHUNK_ITEM_STRIPE* cis = (CHUNK_ITEM_STRIPE*)&ci[1];
        
        // FIXME - update dev stats
        
        if (cancelled > 0) {
#ifdef DEBUG_STATS
            LARGE_INTEGER time1, time2;
#endif
            context->stripes_left = 0;
            
            for (i = 0; i < ci->num_stripes; i++) {
                if (context->stripes[i].status == ReadDataStatus_Cancelled) {
                    PIO_STACK_LOCATION IrpSp;
                    
                    // re-run Irp that we cancelled
                    
                    if (context->stripes[i].Irp) {
                        if (devices[i]->devobj->Flags & DO_DIRECT_IO) {
                            MmUnlockPages(context->stripes[i].Irp->MdlAddress);
                            IoFreeMdl(context->stripes[i].Irp->MdlAddress);
                        }
                        IoFreeIrp(context->stripes[i].Irp);
                    }
                    
                    if (!Irp) {
                        context->stripes[i].Irp = IoAllocateIrp(devices[i]->devobj->StackSize, FALSE);
                        
                        if (!context->stripes[i].Irp) {
                            ERR("IoAllocateIrp failed\n");
                            return STATUS_INSUFFICIENT_RESOURCES;
                        }
                    } else {
                        context->stripes[i].Irp = IoMakeAssociatedIrp(Irp, devices[i]->devobj->StackSize);
                        
                        if (!context->stripes[i].Irp) {
                            ERR("IoMakeAssociatedIrp failed\n");
                            return STATUS_INSUFFICIENT_RESOURCES;
                        }
                    }
                    
                    IrpSp = IoGetNextIrpStackLocation(context->stripes[i].Irp);
                    IrpSp->MajorFunction = IRP_MJ_READ;
                    
                    if (devices[i]->devobj->Flags & DO_BUFFERED_IO) {
                        FIXME("FIXME - buffered IO\n");
                    } else if (devices[i]->devobj->Flags & DO_DIRECT_IO) {
                        context->stripes[i].Irp->MdlAddress = IoAllocateMdl(context->stripes[i].buf, stripeend[i] - stripestart[i], FALSE, FALSE, NULL);
                        if (!context->stripes[i].Irp->MdlAddress) {
                            ERR("IoAllocateMdl failed\n");
                            return STATUS_INSUFFICIENT_RESOURCES;
                        }
                        
                        MmProbeAndLockPages(context->stripes[i].Irp->MdlAddress, KernelMode, IoWriteAccess);
                    } else {
                        context->stripes[i].Irp->UserBuffer = context->stripes[i].buf;
                    }

                    IrpSp->Parameters.Read.Length = stripeend[i] - stripestart[i];
                    IrpSp->Parameters.Read.ByteOffset.QuadPart = stripestart[i] + cis[i].offset;
                    
                    context->stripes[i].Irp->UserIosb = &context->stripes[i].iosb;
                    
                    IoSetCompletionRoutine(context->stripes[i].Irp, read_data_completion, &context->stripes[i], TRUE, TRUE, TRUE);
                    
                    context->stripes_left++;
                    context->stripes[i].status = ReadDataStatus_Pending;
                }
            }
            
            context->stripes_cancel = 0;
            KeClearEvent(&context->Event);
            
#ifdef DEBUG_STATS
            if (!context->tree)
                time1 = KeQueryPerformanceCounter(NULL);
#endif

            for (i = 0; i < ci->num_stripes; i++) {
                if (context->stripes[i].status == ReadDataStatus_Pending) {
                    IoCallDriver(devices[i]->devobj, context->stripes[i].Irp);
                }
            }
            
            KeWaitForSingleObject(&context->Event, Executive, KernelMode, FALSE, NULL);
            
#ifdef DEBUG_STATS
            if (!context->tree) {
                time2 = KeQueryPerformanceCounter(NULL);
                
                Vcb->stats.read_disk_time += time2.QuadPart - time1.QuadPart;
            }
#endif
            for (i = 0; i < ci->num_stripes; i++) {
                if (context->stripes[i].status == ReadDataStatus_Success) {
                    if (context->tree) {
                        tree_header* th = (tree_header*)context->stripes[i].buf;
                        UINT32 crc32;
                        
                        crc32 = ~calc_crc32c(0xffffffff, (UINT8*)&th->fs_uuid, context->buflen - sizeof(th->csum));
                        
                        if (th->address != context->address || crc32 != *((UINT32*)th->csum))
                            context->stripes[i].status = ReadDataStatus_CRCError;
                    } else if (context->csum) {
                        NTSTATUS Status;
#ifdef DEBUG_STATS
                        time1 = KeQueryPerformanceCounter(NULL);
#endif
                        Status = check_csum(Vcb, context->stripes[i].buf, context->stripes[i].Irp->IoStatus.Information / Vcb->superblock.sector_size, context->csum);
                        
                        if (Status == STATUS_CRC_ERROR)
                            context->stripes[i].status = ReadDataStatus_CRCError;
                        else if (!NT_SUCCESS(Status)) {
                            ERR("check_csum returned %08x\n", Status);
                            return Status;
                        }
#ifdef DEBUG_STATS
                        time2 = KeQueryPerformanceCounter(NULL);
                        
                        Vcb->stats.read_csum_time += time2.QuadPart - time1.QuadPart;
#endif
                    }
                }
            }
        }
        
        for (i = 0; i < ci->num_stripes; i++) {
            if (context->stripes[i].status == ReadDataStatus_Success) {
                RtlCopyMemory(buf, context->stripes[i].buf, length);
                goto raid1write;
            }
        }
        
        if (context->tree || ci->num_stripes == 1) { // unable to recover from checksum error
            ERR("unrecoverable checksum error at %llx\n", addr);
            
#ifdef _DEBUG
            if (context->tree) {
                for (i = 0; i < ci->num_stripes; i++) {
                    if (context->stripes[i].status == ReadDataStatus_CRCError) {
                        tree_header* th = (tree_header*)context->stripes[i].buf;
                        UINT32 crc32 = ~calc_crc32c(0xffffffff, (UINT8*)&th->fs_uuid, context->buflen - sizeof(th->csum));
                        
                        if (crc32 != *((UINT32*)th->csum)) {
                            WARN("crc32 was %08x, expected %08x\n", crc32, *((UINT32*)th->csum));
                            return STATUS_CRC_ERROR;
                        } else if (addr != th->address) {
                            WARN("address of tree was %llx, not %llx as expected\n", th->address, addr);
                            return STATUS_CRC_ERROR;
                        }
                    }
                }
            }
#endif
            return STATUS_CRC_ERROR;
        }
        
        // checksum errors on both stripes - we need to check sector by sector
        
        for (i = 0; i < (stripeend[0] - stripestart[0]) / context->sector_size; i++) {
            UINT16 j;
            BOOL success = FALSE;
#ifdef DEBUG_STATS
            LARGE_INTEGER time1, time2;
            
            time1 = KeQueryPerformanceCounter(NULL);
#endif
            
            for (j = 0; j < ci->num_stripes; j++) {
                if (context->stripes[j].status == ReadDataStatus_CRCError) {
                    UINT32 crc32 = ~calc_crc32c(0xffffffff, context->stripes[j].buf + (i * context->sector_size), context->sector_size);
                    
                    if (crc32 == context->csum[i]) {
                        RtlCopyMemory(buf + (i * context->sector_size), context->stripes[j].buf + (i * context->sector_size), context->sector_size);
                        success = TRUE;
                        break;
                    }
                }
            }
            
#ifdef DEBUG_STATS
            time2 = KeQueryPerformanceCounter(NULL);

            Vcb->stats.read_csum_time += time2.QuadPart - time1.QuadPart;
#endif
            if (!success) {
                ERR("unrecoverable checksum error at %llx\n", addr + (i * context->sector_size));
                return STATUS_CRC_ERROR;
            }
        }
        
raid1write:
        // write good data over bad
        
        if (!Vcb->readonly) {
            for (i = 0; i < ci->num_stripes; i++) {
                if (context->stripes[i].status == ReadDataStatus_CRCError && devices[i] && !devices[i]->readonly) {
                    Status = write_data_phys(devices[i]->devobj, cis[i].offset + stripestart[i], buf, length);
                    
                    if (!NT_SUCCESS(Status))
                        WARN("write_data_phys returned %08x\n", Status);
                }
            }
        }
        
        return STATUS_SUCCESS;
    }
    
    // check if any of the stripes succeeded
    
    for (i = 0; i < ci->num_stripes; i++) {
        if (context->stripes[i].status == ReadDataStatus_Success) {
            RtlCopyMemory(buf, context->stripes[i].buf, length);
            return STATUS_SUCCESS;
        }
    }
    
    // failing that, return the first error we encountered
    
    for (i = 0; i < ci->num_stripes; i++) {
        if (context->stripes[i].status == ReadDataStatus_Error)
            return context->stripes[i].iosb.Status;
    }
    
    // if we somehow get here, return STATUS_INTERNAL_ERROR
    
    return STATUS_INTERNAL_ERROR;
}

static NTSTATUS read_data_raid0(device_extension* Vcb, UINT8* buf, UINT64 addr, UINT32 length, read_data_context* context,
                                CHUNK_ITEM* ci, UINT64* stripestart, UINT64* stripeend, UINT16 startoffstripe) {
    UINT64 i;
    UINT32 pos, *stripeoff;
    UINT8 stripe;
    
    for (i = 0; i < ci->num_stripes; i++) {
        if (context->stripes[i].status == ReadDataStatus_Error) {
            WARN("stripe %llu returned error %08x\n", i, context->stripes[i].iosb.Status); 
            return context->stripes[i].iosb.Status;
        }
    }
    
    pos = 0;
    stripeoff = ExAllocatePoolWithTag(NonPagedPool, sizeof(UINT32) * ci->num_stripes, ALLOC_TAG);
    if (!stripeoff) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlZeroMemory(stripeoff, sizeof(UINT32) * ci->num_stripes);
    
    stripe = startoffstripe;
    while (pos < length) {
        if (pos == 0) {
            UINT32 readlen = min(stripeend[stripe] - stripestart[stripe], ci->stripe_length - (stripestart[stripe] % ci->stripe_length));
            
            RtlCopyMemory(buf, context->stripes[stripe].buf, readlen);
            stripeoff[stripe] += readlen;
            pos += readlen;
        } else if (length - pos < ci->stripe_length) {
            RtlCopyMemory(buf + pos, &context->stripes[stripe].buf[stripeoff[stripe]], length - pos);
            pos = length;
        } else {
            RtlCopyMemory(buf + pos, &context->stripes[stripe].buf[stripeoff[stripe]], ci->stripe_length);
            stripeoff[stripe] += ci->stripe_length;
            pos += ci->stripe_length;
        }
        
        stripe = (stripe + 1) % ci->num_stripes;
    }
    
    ExFreePool(stripeoff);
    
    // FIXME - handle the case where one of the stripes doesn't read everything, i.e. Irp->IoStatus.Information is short
    
    if (context->tree) { // shouldn't happen, as trees shouldn't cross stripe boundaries
        tree_header* th = (tree_header*)buf;
        UINT32 crc32 = ~calc_crc32c(0xffffffff, (UINT8*)&th->fs_uuid, Vcb->superblock.node_size - sizeof(th->csum));
        
        if (crc32 != *((UINT32*)th->csum)) {
            WARN("crc32 was %08x, expected %08x\n", crc32, *((UINT32*)th->csum));
            return STATUS_CRC_ERROR;
        } else if (addr != th->address) {
            WARN("address of tree was %llx, not %llx as expected\n", th->address, addr);
            return STATUS_CRC_ERROR;
        }
    } else if (context->csum) {
        NTSTATUS Status;
#ifdef DEBUG_STATS
        LARGE_INTEGER time1, time2;
        
        time1 = KeQueryPerformanceCounter(NULL);
#endif
        Status = check_csum(Vcb, buf, length / Vcb->superblock.sector_size, context->csum);
        
        if (Status == STATUS_CRC_ERROR) {
            WARN("checksum error\n");
            return Status;
        } else if (!NT_SUCCESS(Status)) {
            ERR("check_csum returned %08x\n", Status);
            return Status;
        }
#ifdef DEBUG_STATS
        time2 = KeQueryPerformanceCounter(NULL);
        
        Vcb->stats.read_csum_time += time2.QuadPart - time1.QuadPart;
#endif
    }
    
    return STATUS_SUCCESS;    
}

static NTSTATUS read_data_raid10(device_extension* Vcb, UINT8* buf, UINT64 addr, UINT32 length, PIRP Irp, read_data_context* context,
                                 CHUNK_ITEM* ci, device** devices, UINT64* stripestart, UINT64* stripeend, UINT16 startoffstripe) {
    UINT64 i;
    NTSTATUS Status;
    BOOL checksum_error = FALSE;
    UINT32 pos, *stripeoff;
    UINT8 stripe;
    read_data_stripe** stripes;

    stripes = ExAllocatePoolWithTag(NonPagedPool, sizeof(read_data_stripe*) * ci->num_stripes / ci->sub_stripes, ALLOC_TAG);
    if (!stripes) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlZeroMemory(stripes, sizeof(read_data_stripe*) * ci->num_stripes / ci->sub_stripes);
    
    for (i = 0; i < ci->num_stripes; i += ci->sub_stripes) {
        UINT16 j;
        
        for (j = 0; j < ci->sub_stripes; j++) {
            if (context->stripes[i+j].status == ReadDataStatus_Success) {
                stripes[i / ci->sub_stripes] = &context->stripes[i+j];
                break;
            }
        }
        
        if (!stripes[i / ci->sub_stripes]) {
            for (j = 0; j < ci->sub_stripes; j++) {
                if (context->stripes[i+j].status == ReadDataStatus_Error) {
                    // both stripes must have errored if we get here
                    WARN("stripe %llu returned error %08x\n", i+j, context->stripes[i+j].iosb.Status);
                    ExFreePool(stripes);
                    return context->stripes[i].iosb.Status;
                }
            }
        }
    }
    
    pos = 0;
    stripeoff = ExAllocatePoolWithTag(NonPagedPool, sizeof(UINT32) * ci->num_stripes / ci->sub_stripes, ALLOC_TAG);
    if (!stripeoff) {
        ERR("out of memory\n");
        ExFreePool(stripes);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlZeroMemory(stripeoff, sizeof(UINT32) * ci->num_stripes / ci->sub_stripes);
    
    stripe = startoffstripe / ci->sub_stripes;
    while (pos < length) {
        if (pos == 0) {
            UINT32 readlen = min(stripeend[stripe * ci->sub_stripes] - stripestart[stripe * ci->sub_stripes], ci->stripe_length - (stripestart[stripe * ci->sub_stripes] % ci->stripe_length));
            
            RtlCopyMemory(buf, stripes[stripe]->buf, readlen);
            stripeoff[stripe] += readlen;
            pos += readlen;
        } else if (length - pos < ci->stripe_length) {
            RtlCopyMemory(buf + pos, &stripes[stripe]->buf[stripeoff[stripe]], length - pos);
            
            pos = length;
        } else {
            RtlCopyMemory(buf + pos, &stripes[stripe]->buf[stripeoff[stripe]], ci->stripe_length);
            stripeoff[stripe] += ci->stripe_length;

            pos += ci->stripe_length;
        }
        
        stripe = (stripe + 1) % (ci->num_stripes / ci->sub_stripes);
    }
    
    if (context->tree) {
        tree_header* th = (tree_header*)buf;
        UINT32 crc32 = ~calc_crc32c(0xffffffff, (UINT8*)&th->fs_uuid, Vcb->superblock.node_size - sizeof(th->csum));
        
        if (crc32 != *((UINT32*)th->csum)) {
            WARN("crc32 was %08x, expected %08x\n", crc32, *((UINT32*)th->csum));
            checksum_error = TRUE;
            stripes[startoffstripe]->status = ReadDataStatus_CRCError;
        } else if (addr != th->address) {
            WARN("address of tree was %llx, not %llx as expected\n", th->address, addr);
            checksum_error = TRUE;
            stripes[startoffstripe]->status = ReadDataStatus_CRCError;
        }
    } else if (context->csum) {
        NTSTATUS Status;
#ifdef DEBUG_STATS
        LARGE_INTEGER time1, time2;
        
        time1 = KeQueryPerformanceCounter(NULL);
#endif
        Status = check_csum(Vcb, buf, length / Vcb->superblock.sector_size, context->csum);
        
        if (Status == STATUS_CRC_ERROR)
            checksum_error = TRUE;
        else if (!NT_SUCCESS(Status)) {
            ERR("check_csum returned %08x\n", Status);
            return Status;
        }
#ifdef DEBUG_STATS
        time2 = KeQueryPerformanceCounter(NULL);
        
        Vcb->stats.read_csum_time += time2.QuadPart - time1.QuadPart;
#endif
    }
    
    if (checksum_error) {
        CHUNK_ITEM_STRIPE* cis = (CHUNK_ITEM_STRIPE*)&ci[1];
#ifdef DEBUG_STATS
        LARGE_INTEGER time1, time2;
#endif
        
        // FIXME - update dev stats
        
        WARN("checksum error\n");
        
        if (!context->tree) {
            RtlZeroMemory(stripeoff, sizeof(UINT32) * ci->num_stripes / ci->sub_stripes);
            
            // find out which stripe the error was on
            pos = 0;
            stripe = startoffstripe / ci->sub_stripes;
            while (pos < length) {
                if (pos == 0) {
                    UINT32 readlen = min(stripeend[stripe * ci->sub_stripes] - stripestart[stripe * ci->sub_stripes], ci->stripe_length - (stripestart[stripe * ci->sub_stripes] % ci->stripe_length));
                    
                    stripeoff[stripe] += readlen;
                    pos += readlen;

                    for (i = 0; i < readlen / Vcb->superblock.sector_size; i++) {
                        UINT32 crc32 = ~calc_crc32c(0xffffffff, buf + (i * Vcb->superblock.sector_size), Vcb->superblock.sector_size);
                    
                        if (crc32 != context->csum[i])
                            stripes[stripe]->status = ReadDataStatus_CRCError;
                    }
                } else if (length - pos < ci->stripe_length) {
                    for (i = 0; i < (length - pos) / Vcb->superblock.sector_size; i++) {
                        UINT32 crc32 = ~calc_crc32c(0xffffffff, buf + pos + (i * Vcb->superblock.sector_size), Vcb->superblock.sector_size);
                    
                        if (crc32 != context->csum[(pos / Vcb->superblock.sector_size) + i])
                            stripes[stripe]->status = ReadDataStatus_CRCError;
                    }
                    
                    pos = length;
                } else {
                    stripeoff[stripe] += ci->stripe_length;
                    
                    for (i = 0; i < ci->stripe_length / Vcb->superblock.sector_size; i++) {
                        UINT32 crc32 = ~calc_crc32c(0xffffffff, buf + pos + (i * Vcb->superblock.sector_size), Vcb->superblock.sector_size);
                    
                        if (crc32 != context->csum[(pos / Vcb->superblock.sector_size) + i])
                            stripes[stripe]->status = ReadDataStatus_CRCError;
                    }
                    
                    pos += ci->stripe_length;
                }
                
                stripe = (stripe + 1) % (ci->num_stripes / ci->sub_stripes);
            }
        }
        
        context->stripes_left = 0;
        
        for (i = 0; i < ci->num_stripes; i++) {
            if (context->stripes[i].status == ReadDataStatus_CRCError) {
                UINT16 other_stripe = (i % 1) ? (i - 1) : (i + 1);
                
                if (context->stripes[other_stripe].status == ReadDataStatus_Cancelled) {
                    PIO_STACK_LOCATION IrpSp;
                    
                    // re-run Irp that we cancelled
                    
                    if (context->stripes[other_stripe].Irp) {
                        if (devices[other_stripe]->devobj->Flags & DO_DIRECT_IO) {
                            MmUnlockPages(context->stripes[other_stripe].Irp->MdlAddress);
                            IoFreeMdl(context->stripes[other_stripe].Irp->MdlAddress);
                        }
                        IoFreeIrp(context->stripes[other_stripe].Irp);
                    }
                    
                    if (!Irp) {
                        context->stripes[other_stripe].Irp = IoAllocateIrp(devices[other_stripe]->devobj->StackSize, FALSE);
                        
                        if (!context->stripes[other_stripe].Irp) {
                            ERR("IoAllocateIrp failed\n");
                            return STATUS_INSUFFICIENT_RESOURCES;
                        }
                    } else {
                        context->stripes[other_stripe].Irp = IoMakeAssociatedIrp(Irp, devices[other_stripe]->devobj->StackSize);
                        
                        if (!context->stripes[other_stripe].Irp) {
                            ERR("IoMakeAssociatedIrp failed\n");
                            return STATUS_INSUFFICIENT_RESOURCES;
                        }
                    }
                    
                    IrpSp = IoGetNextIrpStackLocation(context->stripes[other_stripe].Irp);
                    IrpSp->MajorFunction = IRP_MJ_READ;
                    
                    if (devices[other_stripe]->devobj->Flags & DO_BUFFERED_IO) {
                        FIXME("FIXME - buffered IO\n");
                    } else if (devices[other_stripe]->devobj->Flags & DO_DIRECT_IO) {
                        context->stripes[other_stripe].Irp->MdlAddress = IoAllocateMdl(context->stripes[other_stripe].buf, stripeend[other_stripe] - stripestart[other_stripe], FALSE, FALSE, NULL);
                        if (!context->stripes[other_stripe].Irp->MdlAddress) {
                            ERR("IoAllocateMdl failed\n");
                            return STATUS_INSUFFICIENT_RESOURCES;
                        }
                        
                        MmProbeAndLockPages(context->stripes[other_stripe].Irp->MdlAddress, KernelMode, IoWriteAccess);
                    } else {
                        context->stripes[other_stripe].Irp->UserBuffer = context->stripes[other_stripe].buf;
                    }

                    IrpSp->Parameters.Read.Length = stripeend[other_stripe] - stripestart[other_stripe];
                    IrpSp->Parameters.Read.ByteOffset.QuadPart = stripestart[other_stripe] + cis[other_stripe].offset;
                    
                    context->stripes[other_stripe].Irp->UserIosb = &context->stripes[other_stripe].iosb;
                    
                    IoSetCompletionRoutine(context->stripes[other_stripe].Irp, read_data_completion, &context->stripes[other_stripe], TRUE, TRUE, TRUE);
                    
                    context->stripes_left++;
                    context->stripes[other_stripe].status = ReadDataStatus_Pending;
                }
            }
        }
        
        if (context->stripes_left == 0) {
            WARN("could not recover from checksum error\n");
            ExFreePool(stripes);
            ExFreePool(stripeoff);
            return STATUS_CRC_ERROR;
        }
        
        context->stripes_cancel = 0;
        KeClearEvent(&context->Event);
        
#ifdef DEBUG_STATS
        if (!context->tree)
            time1 = KeQueryPerformanceCounter(NULL);
#endif

        for (i = 0; i < ci->num_stripes; i++) {
            if (context->stripes[i].status == ReadDataStatus_Pending) {
                IoCallDriver(devices[i]->devobj, context->stripes[i].Irp);
            }
        }
        
        KeWaitForSingleObject(&context->Event, Executive, KernelMode, FALSE, NULL);
        
#ifdef DEBUG_STATS
        if (!context->tree) {
            time2 = KeQueryPerformanceCounter(NULL);
            
            Vcb->stats.read_disk_time += time2.QuadPart - time1.QuadPart;
        }
#endif

        for (i = 0; i < ci->num_stripes; i++) {
            if (context->stripes[i].status == ReadDataStatus_CRCError) {
                UINT16 other_stripe = (i % 1) ? (i - 1) : (i + 1);
                
                if (context->stripes[other_stripe].status != ReadDataStatus_Success) {
                    WARN("could not recover from checksum error\n");
                    ExFreePool(stripes);
                    ExFreePool(stripeoff);
                    return STATUS_CRC_ERROR;
                }
            }
        }

        RtlZeroMemory(stripeoff, sizeof(UINT32) * ci->num_stripes / ci->sub_stripes);
    
        pos = 0;
        stripe = startoffstripe / ci->sub_stripes;
        while (pos < length) {
            if (pos == 0) {
                UINT32 readlen = min(stripeend[stripe * ci->sub_stripes] - stripestart[stripe * ci->sub_stripes], ci->stripe_length - (stripestart[stripe * ci->sub_stripes] % ci->stripe_length));
                
                stripeoff[stripe] += readlen;
                pos += readlen;
                
                if (context->csum && stripes[stripe]->status == ReadDataStatus_CRCError) {
                    for (i = 0; i < readlen / Vcb->superblock.sector_size; i++) {
                        UINT32 crc32 = ~calc_crc32c(0xffffffff, buf + (i * Vcb->superblock.sector_size), Vcb->superblock.sector_size);
                    
                        if (crc32 != context->csum[i]) {
                            UINT16 other_stripe = (stripe * ci->sub_stripes) + (context->stripes[stripe * ci->sub_stripes].status == ReadDataStatus_CRCError ? 1 : 0);
                            UINT32 crc32b = ~calc_crc32c(0xffffffff, context->stripes[other_stripe].buf + (i * Vcb->superblock.sector_size), Vcb->superblock.sector_size);
                            
                            if (crc32b == context->csum[i]) {
                                RtlCopyMemory(buf + (i * Vcb->superblock.sector_size), context->stripes[other_stripe].buf + (i * Vcb->superblock.sector_size), Vcb->superblock.sector_size);
                                RtlCopyMemory(stripes[stripe]->buf + (i * Vcb->superblock.sector_size), context->stripes[other_stripe].buf + (i * Vcb->superblock.sector_size),
                                                Vcb->superblock.sector_size);
                                stripes[stripe]->rewrite = TRUE;
                            } else {
                                WARN("could not recover from checksum error\n");
                                ExFreePool(stripes);
                                ExFreePool(stripeoff);
                                return STATUS_CRC_ERROR;
                            }
                        }
                    }
                } else if (context->tree) {
                    UINT16 other_stripe = (stripe * ci->sub_stripes) + (context->stripes[stripe * ci->sub_stripes].status == ReadDataStatus_CRCError ? 1 : 0);
                    tree_header* th = (tree_header*)buf;
                    UINT32 crc32;
                    
                    RtlCopyMemory(buf, context->stripes[other_stripe].buf, readlen);
                    
                    crc32 = ~calc_crc32c(0xffffffff, (UINT8*)&th->fs_uuid, Vcb->superblock.node_size - sizeof(th->csum));
                    
                    if (addr != th->address || crc32 != *((UINT32*)th->csum)) {
                        WARN("could not recover from checksum error\n");
                        ExFreePool(stripes);
                        ExFreePool(stripeoff);
                        return STATUS_CRC_ERROR;
                    }
                    
                    RtlCopyMemory(stripes[stripe]->buf, buf, readlen);
                    stripes[stripe]->rewrite = TRUE;
                }
            } else if (length - pos < ci->stripe_length) {
                if (context->csum && stripes[stripe]->status == ReadDataStatus_CRCError) {
                    for (i = 0; i < (length - pos) / Vcb->superblock.sector_size; i++) {
                        UINT32 crc32 = ~calc_crc32c(0xffffffff, buf + pos + (i * Vcb->superblock.sector_size), Vcb->superblock.sector_size);
                    
                        if (crc32 != context->csum[(pos / Vcb->superblock.sector_size) + i]) {
                            UINT16 other_stripe = (stripe * ci->sub_stripes) + (context->stripes[stripe * ci->sub_stripes].status == ReadDataStatus_CRCError ? 1 : 0);
                            UINT32 crc32b = ~calc_crc32c(0xffffffff, &context->stripes[other_stripe].buf[stripeoff[stripe] + (i * Vcb->superblock.sector_size)],
                                                            Vcb->superblock.sector_size);
                            
                            if (crc32b == context->csum[i]) {
                                RtlCopyMemory(buf + pos + (i * Vcb->superblock.sector_size),
                                                &context->stripes[other_stripe].buf[stripeoff[stripe] + (i * Vcb->superblock.sector_size)], Vcb->superblock.sector_size);
                                RtlCopyMemory(&stripes[stripe]->buf[stripeoff[stripe] + (i * Vcb->superblock.sector_size)],
                                                &context->stripes[other_stripe].buf[stripeoff[stripe] + (i * Vcb->superblock.sector_size)],
                                                Vcb->superblock.sector_size);
                                stripes[stripe]->rewrite = TRUE;
                            } else {
                                WARN("could not recover from checksum error\n");
                                ExFreePool(stripes);
                                ExFreePool(stripeoff);
                                return STATUS_CRC_ERROR;
                            }
                        }
                    }
                }
                
                pos = length;
            } else {
                if (context->csum && stripes[stripe]->status == ReadDataStatus_CRCError) {
                    for (i = 0; i < ci->stripe_length / Vcb->superblock.sector_size; i++) {
                        UINT32 crc32 = ~calc_crc32c(0xffffffff, buf + pos + (i * Vcb->superblock.sector_size), Vcb->superblock.sector_size);
                    
                        if (crc32 != context->csum[(pos / Vcb->superblock.sector_size) + i]) {
                            UINT16 other_stripe = (stripe * ci->sub_stripes) + (context->stripes[stripe * ci->sub_stripes].status == ReadDataStatus_CRCError ? 1 : 0);
                            UINT32 crc32b = ~calc_crc32c(0xffffffff, &context->stripes[other_stripe].buf[stripeoff[stripe] + (i * Vcb->superblock.sector_size)],
                                                            Vcb->superblock.sector_size);
                            
                            if (crc32b == context->csum[i]) {
                                RtlCopyMemory(buf + pos + (i * Vcb->superblock.sector_size),
                                                &context->stripes[other_stripe].buf[stripeoff[stripe] + (i * Vcb->superblock.sector_size)], Vcb->superblock.sector_size);
                                RtlCopyMemory(&stripes[stripe]->buf[stripeoff[stripe] + (i * Vcb->superblock.sector_size)],
                                                &context->stripes[other_stripe].buf[stripeoff[stripe] + (i * Vcb->superblock.sector_size)],
                                                Vcb->superblock.sector_size);
                                stripes[stripe]->rewrite = TRUE;
                            } else {
                                WARN("could not recover from checksum error\n");
                                ExFreePool(stripes);
                                ExFreePool(stripeoff);
                                return STATUS_CRC_ERROR;
                            }
                        }
                    }
                }
                
                stripeoff[stripe] += ci->stripe_length;
                pos += ci->stripe_length;
            }
            
            stripe = (stripe + 1) % (ci->num_stripes / ci->sub_stripes);
        }
        
        // write good data over bad
        
        if (!Vcb->readonly) {
            for (i = 0; i < ci->num_stripes; i++) {
                if (context->stripes[i].rewrite && devices[i] && !devices[i]->readonly) {
                    Status = write_data_phys(devices[i]->devobj, cis[i].offset + stripestart[i], context->stripes[i].buf, stripeend[i] - stripestart[i]);
                    
                    if (!NT_SUCCESS(Status))
                        WARN("write_data_phys returned %08x\n", Status);
                }
            }
        }
    }
    
    ExFreePool(stripes);
    ExFreePool(stripeoff);
    
    // FIXME - handle the case where one of the stripes doesn't read everything, i.e. Irp->IoStatus.Information is short
    
    return STATUS_SUCCESS;
}

static NTSTATUS read_data_raid5(device_extension* Vcb, UINT8* buf, UINT64 addr, UINT32 length, PIRP Irp, read_data_context* context, CHUNK_ITEM* ci,
                                device** devices, UINT64* stripestart, UINT64* stripeend, UINT64 offset, UINT32 firststripesize, BOOL check_nocsum_parity) {
    UINT32 pos, skip;
    NTSTATUS Status;
    int num_errors = 0;
    UINT64 i, off, stripeoff, origoff;
    BOOL needs_reconstruct = FALSE;
    UINT64 reconstruct_stripe;
    BOOL checksum_error = FALSE;
    
    for (i = 0; i < ci->num_stripes; i++) {
        if (context->stripes[i].status == ReadDataStatus_Error) {
            num_errors++;
            if (num_errors > 1)
                break;
        }
    }
    
    if (num_errors > 1) {
        for (i = 0; i < ci->num_stripes; i++) {
            if (context->stripes[i].status == ReadDataStatus_Error) {
                WARN("stripe %llu returned error %08x\n", i, context->stripes[i].iosb.Status); 
                return context->stripes[i].iosb.Status;
            }
        }
    }
    
    off = addr - offset;
    off -= off % ((ci->num_stripes - 1) * ci->stripe_length);
    skip = addr - offset - off;
    origoff = off;
    
    for (i = 0; i < ci->num_stripes; i++) {
        if (context->stripes[i].status == ReadDataStatus_Cancelled) {
            if (needs_reconstruct) {
                ERR("more than one stripe needs reconstruction\n");
                return STATUS_INTERNAL_ERROR;
            } else {
                needs_reconstruct = TRUE;
                reconstruct_stripe = i;
            }
        }
    }
    
    if (needs_reconstruct) {
        TRACE("reconstructing stripe %u\n", reconstruct_stripe);
        
        stripeoff = 0;
        
        raid5_reconstruct(off, skip, context, ci, &stripeoff, stripeend[reconstruct_stripe] - stripestart[reconstruct_stripe], TRUE, firststripesize, reconstruct_stripe);
        
        while (stripeoff < stripeend[0] - stripestart[0]) {
            off += (ci->num_stripes - 1) * ci->stripe_length;
            raid5_reconstruct(off, 0, context, ci, &stripeoff, stripeend[reconstruct_stripe] - stripestart[reconstruct_stripe], FALSE, 0, reconstruct_stripe);
        }
        
        off = addr - offset;
        off -= off % ((ci->num_stripes - 1) * ci->stripe_length);
    }
    
    pos = 0;
    stripeoff = 0;
    raid5_decode(off, skip, context, ci, &stripeoff, buf, &pos, length, firststripesize);
    
    while (pos < length) {
        off += (ci->num_stripes - 1) * ci->stripe_length;
        raid5_decode(off, 0, context, ci, &stripeoff, buf, &pos, length, 0);
    }
    
    if (context->tree) {
        tree_header* th = (tree_header*)buf;
        UINT32 crc32 = ~calc_crc32c(0xffffffff, (UINT8*)&th->fs_uuid, Vcb->superblock.node_size - sizeof(th->csum));
        
        if (addr != th->address || crc32 != *((UINT32*)th->csum))
            checksum_error = TRUE;
    } else if (context->csum) {
#ifdef DEBUG_STATS
        LARGE_INTEGER time1, time2;
        
        time1 = KeQueryPerformanceCounter(NULL);
#endif
        Status = check_csum(Vcb, buf, length / Vcb->superblock.sector_size, context->csum);
        
        if (Status == STATUS_CRC_ERROR) {
            WARN("checksum error\n");
            checksum_error = TRUE;
        } else if (!NT_SUCCESS(Status)) {
            ERR("check_csum returned %08x\n", Status);
            return Status;
        }

#ifdef DEBUG_STATS
        time2 = KeQueryPerformanceCounter(NULL);
        
        Vcb->stats.read_csum_time += time2.QuadPart - time1.QuadPart;
#endif
    }
    
    if (checksum_error) {
        CHUNK_ITEM_STRIPE* cis = (CHUNK_ITEM_STRIPE*)&ci[1];
        
        if (needs_reconstruct) {
            PIO_STACK_LOCATION IrpSp;
#ifdef DEBUG_STATS
            LARGE_INTEGER time1, time2;
#endif
            
            // re-run Irp that we cancelled
            
            if (context->stripes[reconstruct_stripe].Irp) {
                if (devices[reconstruct_stripe]->devobj->Flags & DO_DIRECT_IO) {
                    MmUnlockPages(context->stripes[reconstruct_stripe].Irp->MdlAddress);
                    IoFreeMdl(context->stripes[reconstruct_stripe].Irp->MdlAddress);
                }
                IoFreeIrp(context->stripes[reconstruct_stripe].Irp);
            }
            
            if (!Irp) {
                context->stripes[reconstruct_stripe].Irp = IoAllocateIrp(devices[reconstruct_stripe]->devobj->StackSize, FALSE);
                
                if (!context->stripes[reconstruct_stripe].Irp) {
                    ERR("IoAllocateIrp failed\n");
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
            } else {
                context->stripes[reconstruct_stripe].Irp = IoMakeAssociatedIrp(Irp, devices[reconstruct_stripe]->devobj->StackSize);
                
                if (!context->stripes[reconstruct_stripe].Irp) {
                    ERR("IoMakeAssociatedIrp failed\n");
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
            }
            
            IrpSp = IoGetNextIrpStackLocation(context->stripes[reconstruct_stripe].Irp);
            IrpSp->MajorFunction = IRP_MJ_READ;
            
            if (devices[reconstruct_stripe]->devobj->Flags & DO_BUFFERED_IO) {
                FIXME("FIXME - buffered IO\n");
            } else if (devices[reconstruct_stripe]->devobj->Flags & DO_DIRECT_IO) {
                context->stripes[reconstruct_stripe].Irp->MdlAddress = IoAllocateMdl(context->stripes[reconstruct_stripe].buf,
                                                                                        stripeend[reconstruct_stripe] - stripestart[reconstruct_stripe], FALSE, FALSE, NULL);
                if (!context->stripes[reconstruct_stripe].Irp->MdlAddress) {
                    ERR("IoAllocateMdl failed\n");
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                
                MmProbeAndLockPages(context->stripes[reconstruct_stripe].Irp->MdlAddress, KernelMode, IoWriteAccess);
            } else {
                context->stripes[reconstruct_stripe].Irp->UserBuffer = context->stripes[reconstruct_stripe].buf;
            }

            IrpSp->Parameters.Read.Length = stripeend[reconstruct_stripe] - stripestart[reconstruct_stripe];
            IrpSp->Parameters.Read.ByteOffset.QuadPart = stripestart[reconstruct_stripe] + cis[reconstruct_stripe].offset;
            
            context->stripes[reconstruct_stripe].Irp->UserIosb = &context->stripes[reconstruct_stripe].iosb;
            
            IoSetCompletionRoutine(context->stripes[reconstruct_stripe].Irp, read_data_completion, &context->stripes[reconstruct_stripe], TRUE, TRUE, TRUE);

            context->stripes[reconstruct_stripe].status = ReadDataStatus_Pending;
            
            context->stripes_left = 1;
            KeClearEvent(&context->Event);
            
#ifdef DEBUG_STATS
            if (!context->tree)
                time1 = KeQueryPerformanceCounter(NULL);
#endif

            IoCallDriver(devices[reconstruct_stripe]->devobj, context->stripes[reconstruct_stripe].Irp);
            
            KeWaitForSingleObject(&context->Event, Executive, KernelMode, FALSE, NULL);
            
#ifdef DEBUG_STATS
            if (!context->tree) {
                time2 = KeQueryPerformanceCounter(NULL);
                
                Vcb->stats.read_disk_time += time2.QuadPart - time1.QuadPart;
            }
#endif

            if (context->stripes[reconstruct_stripe].status != ReadDataStatus_Success) {
                ERR("unrecoverable checksum error\n");
                return STATUS_CRC_ERROR;
            }
        }
        
        if (context->tree) {
            off = origoff;
            pos = 0;
            stripeoff = 0;
            if (!raid5_decode_with_checksum_metadata(addr, off, skip, context, ci, &stripeoff, buf, &pos, length, firststripesize, Vcb->superblock.node_size)) {
                ERR("unrecoverable metadata checksum error\n");
                return STATUS_CRC_ERROR;
            }
        } else {
            off = origoff;
            pos = 0;
            stripeoff = 0;
            if (!raid5_decode_with_checksum(off, skip, context, ci, &stripeoff, buf, &pos, length, firststripesize, context->csum, Vcb->superblock.sector_size))
                return STATUS_CRC_ERROR;
            
            while (pos < length) {
                off += (ci->num_stripes - 1) * ci->stripe_length;
                if (!raid5_decode_with_checksum(off, 0, context, ci, &stripeoff, buf, &pos, length, 0, context->csum, Vcb->superblock.sector_size))
                    return STATUS_CRC_ERROR;
            }
        }
        
        // write good data over bad
        
        if (!Vcb->readonly) {
            for (i = 0; i < ci->num_stripes; i++) {
                if (context->stripes[i].rewrite && devices[i] && !devices[i]->readonly) {
                    Status = write_data_phys(devices[i]->devobj, cis[i].offset + stripestart[i], context->stripes[i].buf, stripeend[i] - stripestart[i]);
                    
                    if (!NT_SUCCESS(Status))
                        WARN("write_data_phys returned %08x\n", Status);
                }
            }
        }
    }
    
    if (check_nocsum_parity && !context->tree && !context->csum) {
        UINT32* parity_buf;
        
        // We are reading a nodatacsum extent. Even though there's no checksum, we
        // can still identify errors by checking if the parity is consistent.
        
        parity_buf = ExAllocatePoolWithTag(NonPagedPool, stripeend[0] - stripestart[0], ALLOC_TAG);
        
        if (!parity_buf) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        RtlCopyMemory(parity_buf, context->stripes[0].buf, stripeend[0] - stripestart[0]);
        
        for (i = 0; i < ci->num_stripes; i++) {
            do_xor((UINT8*)parity_buf, context->stripes[i].buf, stripeend[0] - stripestart[0]);
        }
        
        for (i = 0; i < (stripeend[0] - stripestart[0]) / sizeof(UINT32); i++) {
            if (parity_buf[i] != 0) {
                ERR("parity error on nodatacsum inode\n");
                ExFreePool(parity_buf);
                return STATUS_CRC_ERROR;
            }
        }
        
        ExFreePool(parity_buf);
    }
    
    return STATUS_SUCCESS;
}

static NTSTATUS read_data_raid6(device_extension* Vcb, UINT8* buf, UINT64 addr, UINT32 length, PIRP Irp, read_data_context* context, CHUNK_ITEM* ci,
                                device** devices, UINT64* stripestart, UINT64* stripeend, UINT64 offset, UINT32 firststripesize, BOOL check_nocsum_parity) {
    NTSTATUS Status;
    UINT32 pos, skip;
    int num_errors = 0;
    UINT64 i, off, stripeoff, origoff;
    UINT8 needs_reconstruct = 0;
    UINT16 missing1, missing2;
    BOOL checksum_error = FALSE;
    
    for (i = 0; i < ci->num_stripes; i++) {
        if (context->stripes[i].status == ReadDataStatus_Error) {
            num_errors++;
            if (num_errors > 2)
                break;
        }
    }
    
    if (num_errors > 2) {
        for (i = 0; i < ci->num_stripes; i++) {
            if (context->stripes[i].status == ReadDataStatus_Error) {
                WARN("stripe %llu returned error %08x\n", i, context->stripes[i].iosb.Status); 
                return context->stripes[i].iosb.Status;
            }
        }
    }
    
    off = addr - offset;
    off -= off % ((ci->num_stripes - 2) * ci->stripe_length);
    skip = addr - offset - off;
    origoff = off;
    
    for (i = 0; i < ci->num_stripes; i++) {
        if (context->stripes[i].status == ReadDataStatus_Cancelled) {
            if (needs_reconstruct == 2) {
                ERR("more than two stripes need reconstruction\n");
                return STATUS_INTERNAL_ERROR;
            } else if (needs_reconstruct == 1) {
                needs_reconstruct++;
                missing2 = i;
            } else {
                needs_reconstruct++;
                missing1 = i;
            }
        }
    }
    
    if (needs_reconstruct > 0) {
        stripeoff = 0;
        
        if (needs_reconstruct == 2) {
            TRACE("reconstructing stripes %u and %u\n", missing1, missing2);
        
            raid6_reconstruct2(off, skip, context, ci, &stripeoff, stripeend[missing1] - stripestart[missing1],
                                TRUE, firststripesize, missing1, missing2);
            
            while (stripeoff < stripeend[0] - stripestart[0]) {
                off += (ci->num_stripes - 2) * ci->stripe_length;
                raid6_reconstruct2(off, 0, context, ci, &stripeoff, stripeend[missing1] - stripestart[missing1],
                                    FALSE, 0, missing1, missing2);
            }
        } else {
            TRACE("reconstructing stripe %u\n", missing1);
            
            raid6_reconstruct1(off, skip, context, ci, &stripeoff, stripeend[missing1] - stripestart[missing1], TRUE, firststripesize, missing1);
            
            while (stripeoff < stripeend[0] - stripestart[0]) {
                off += (ci->num_stripes - 2) * ci->stripe_length;
                raid6_reconstruct1(off, 0, context, ci, &stripeoff, stripeend[missing1] - stripestart[missing1], FALSE, 0, missing1);
            }
        }
        
        off = origoff;
    }
    
    if (check_nocsum_parity && !context->tree && !context->csum) {
        UINT8* scratch;
        
        scratch = ExAllocatePoolWithTag(NonPagedPool, ci->stripe_length, ALLOC_TAG);
        if (!scratch) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        stripeoff = 0;
        Status = check_raid6_nocsum_parity(off, skip, context, ci, &stripeoff, stripeend[0] - stripestart[0], TRUE, firststripesize, scratch);
        if (!NT_SUCCESS(Status)) {
            ERR("check_raid6_nocsum_parity returned %08x\n", Status);
            ExFreePool(scratch);
            return Status;
        }
            
        while (stripeoff < stripeend[0] - stripestart[0]) {
            off += (ci->num_stripes - 2) * ci->stripe_length;
            Status = check_raid6_nocsum_parity(off, 0, context, ci, &stripeoff, stripeend[0] - stripestart[0], FALSE, 0, scratch);
            
            if (!NT_SUCCESS(Status)) {
                ERR("check_raid6_nocsum_parity returned %08x\n", Status);
                ExFreePool(scratch);
                return Status;
            }
        }
        
        ExFreePool(scratch);
        
        off = origoff;
    }
    
    pos = 0;
    stripeoff = 0;
    raid6_decode(off, skip, context, ci, &stripeoff, buf, &pos, length, firststripesize);
    
    while (pos < length) {
        off += (ci->num_stripes - 2) * ci->stripe_length;
        raid6_decode(off, 0, context, ci, &stripeoff, buf, &pos, length, 0);
    }
    
    if (context->tree) {
        tree_header* th = (tree_header*)buf;
        UINT32 crc32 = ~calc_crc32c(0xffffffff, (UINT8*)&th->fs_uuid, Vcb->superblock.node_size - sizeof(th->csum));
        
        if (addr != th->address || crc32 != *((UINT32*)th->csum))
            checksum_error = TRUE;
    } else if (context->csum) {
#ifdef DEBUG_STATS
        LARGE_INTEGER time1, time2;
        
        time1 = KeQueryPerformanceCounter(NULL);
#endif
        Status = check_csum(Vcb, buf, length / Vcb->superblock.sector_size, context->csum);
        
        if (Status == STATUS_CRC_ERROR) {
            WARN("checksum error\n");
            checksum_error = TRUE;
        } else if (!NT_SUCCESS(Status)) {
            ERR("check_csum returned %08x\n", Status);
            return Status;
        }
#ifdef DEBUG_STATS
        time2 = KeQueryPerformanceCounter(NULL);
        
        Vcb->stats.read_csum_time += time2.QuadPart - time1.QuadPart;
#endif
    }
    
    if (checksum_error) {
        CHUNK_ITEM_STRIPE* cis = (CHUNK_ITEM_STRIPE*)&ci[1];
        
        for (i = 0; i < needs_reconstruct; i++) {
            PIO_STACK_LOCATION IrpSp;
            UINT16 reconstruct_stripe = i == 0 ? missing1 : missing2;
            
            // re-run Irps that we cancelled
            
            if (context->stripes[reconstruct_stripe].Irp) {
                if (devices[reconstruct_stripe]->devobj->Flags & DO_DIRECT_IO) {
                    MmUnlockPages(context->stripes[reconstruct_stripe].Irp->MdlAddress);
                    IoFreeMdl(context->stripes[reconstruct_stripe].Irp->MdlAddress);
                }
                IoFreeIrp(context->stripes[reconstruct_stripe].Irp);
            }
            
            if (!Irp) {
                context->stripes[reconstruct_stripe].Irp = IoAllocateIrp(devices[reconstruct_stripe]->devobj->StackSize, FALSE);
                
                if (!context->stripes[reconstruct_stripe].Irp) {
                    ERR("IoAllocateIrp failed\n");
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
            } else {
                context->stripes[reconstruct_stripe].Irp = IoMakeAssociatedIrp(Irp, devices[reconstruct_stripe]->devobj->StackSize);
                
                if (!context->stripes[reconstruct_stripe].Irp) {
                    ERR("IoMakeAssociatedIrp failed\n");
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
            }
            
            IrpSp = IoGetNextIrpStackLocation(context->stripes[reconstruct_stripe].Irp);
            IrpSp->MajorFunction = IRP_MJ_READ;
            
            if (devices[reconstruct_stripe]->devobj->Flags & DO_BUFFERED_IO) {
                FIXME("FIXME - buffered IO\n");
            } else if (devices[reconstruct_stripe]->devobj->Flags & DO_DIRECT_IO) {
                context->stripes[reconstruct_stripe].Irp->MdlAddress = IoAllocateMdl(context->stripes[reconstruct_stripe].buf,
                                                                                        stripeend[reconstruct_stripe] - stripestart[reconstruct_stripe], FALSE, FALSE, NULL);
                if (!context->stripes[reconstruct_stripe].Irp->MdlAddress) {
                    ERR("IoAllocateMdl failed\n");
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                
                MmProbeAndLockPages(context->stripes[reconstruct_stripe].Irp->MdlAddress, KernelMode, IoWriteAccess);
            } else {
                context->stripes[reconstruct_stripe].Irp->UserBuffer = context->stripes[reconstruct_stripe].buf;
            }

            IrpSp->Parameters.Read.Length = stripeend[reconstruct_stripe] - stripestart[reconstruct_stripe];
            IrpSp->Parameters.Read.ByteOffset.QuadPart = stripestart[reconstruct_stripe] + cis[reconstruct_stripe].offset;
            
            context->stripes[reconstruct_stripe].Irp->UserIosb = &context->stripes[reconstruct_stripe].iosb;
            
            IoSetCompletionRoutine(context->stripes[reconstruct_stripe].Irp, read_data_completion, &context->stripes[reconstruct_stripe], TRUE, TRUE, TRUE);

            context->stripes[reconstruct_stripe].status = ReadDataStatus_Pending;
        }
            
        if (needs_reconstruct > 0) {
#ifdef DEBUG_STATS
            LARGE_INTEGER time1, time2;
#endif
            context->stripes_left = needs_reconstruct;
            KeClearEvent(&context->Event);
            
#ifdef DEBUG_STATS
            if (!context->tree)
                time1 = KeQueryPerformanceCounter(NULL);
#endif
            
            for (i = 0; i < needs_reconstruct; i++) {
                UINT16 reconstruct_stripe = i == 0 ? missing1 : missing2;
                
                IoCallDriver(devices[reconstruct_stripe]->devobj, context->stripes[reconstruct_stripe].Irp);
            }
            
            KeWaitForSingleObject(&context->Event, Executive, KernelMode, FALSE, NULL);
            
#ifdef DEBUG_STATS
            if (!context->tree) {
                time2 = KeQueryPerformanceCounter(NULL);
                
                Vcb->stats.read_disk_time += time2.QuadPart - time1.QuadPart;
            }
#endif

            for (i = 0; i < needs_reconstruct; i++) {
                UINT16 reconstruct_stripe = i == 0 ? missing1 : missing2;
                
                if (context->stripes[reconstruct_stripe].status != ReadDataStatus_Success) {
                    ERR("unrecoverable checksum error\n");
                    return STATUS_CRC_ERROR;
                }
            }
        }
        
        off = origoff;
        
        if (context->tree) {
            pos = 0;
            stripeoff = 0;
            if (!raid6_decode_with_checksum_metadata(addr, off, skip, context, ci, &stripeoff, buf, &pos, length, firststripesize, Vcb->superblock.node_size)) {
                ERR("unrecoverable metadata checksum error\n");
                return STATUS_CRC_ERROR;
            }
        } else {
            pos = 0;
            stripeoff = 0;
            if (!raid6_decode_with_checksum(off, skip, context, ci, &stripeoff, buf, &pos, length, firststripesize, context->csum, Vcb->superblock.sector_size))
                return STATUS_CRC_ERROR;
            
            while (pos < length) {
                off += (ci->num_stripes - 1) * ci->stripe_length;
                if (!raid6_decode_with_checksum(off, 0, context, ci, &stripeoff, buf, &pos, length, 0, context->csum, Vcb->superblock.sector_size))
                    return STATUS_CRC_ERROR;
            }
        }
    }
    
    // write good data over bad
    
    if (!Vcb->readonly) {
        CHUNK_ITEM_STRIPE* cis = (CHUNK_ITEM_STRIPE*)&ci[1];
        
        for (i = 0; i < ci->num_stripes; i++) {
            if (context->stripes[i].rewrite && devices[i] && !devices[i]->readonly) {
                Status = write_data_phys(devices[i]->devobj, cis[i].offset + stripestart[i], context->stripes[i].buf, stripeend[i] - stripestart[i]);
                
                if (!NT_SUCCESS(Status))
                    WARN("write_data_phys returned %08x\n", Status);
            }
        }
    }
    
    return STATUS_SUCCESS;
}

NTSTATUS STDCALL read_data(device_extension* Vcb, UINT64 addr, UINT32 length, UINT32* csum, BOOL is_tree, UINT8* buf, chunk* c, chunk** pc,
                           PIRP Irp, BOOL check_nocsum_parity) {
    CHUNK_ITEM* ci;
    CHUNK_ITEM_STRIPE* cis;
    read_data_context* context;
    UINT64 i, type, offset;
    NTSTATUS Status;
    device** devices;
    UINT64 *stripestart = NULL, *stripeend = NULL;
    UINT32 firststripesize;
    UINT16 startoffstripe, allowed_missing, missing_devices = 0;
#ifdef DEBUG_STATS
    LARGE_INTEGER time1, time2;
#endif
    
    if (Vcb->log_to_phys_loaded) {
        if (!c) {
            c = get_chunk_from_address(Vcb, addr);
            
            if (!c) {
                ERR("get_chunk_from_address failed\n");
                return STATUS_INTERNAL_ERROR;
            }
        }
        
        ci = c->chunk_item;
        offset = c->offset;
        devices = c->devices;
           
        if (pc)
            *pc = c;
    } else {
        LIST_ENTRY* le = Vcb->sys_chunks.Flink;
        
        ci = NULL;
        
        while (le != &Vcb->sys_chunks) {
            sys_chunk* sc = CONTAINING_RECORD(le, sys_chunk, list_entry);
            
            if (sc->key.obj_id == 0x100 && sc->key.obj_type == TYPE_CHUNK_ITEM && sc->key.offset <= addr) {
                CHUNK_ITEM* chunk_item = sc->data;
                
                if ((addr - sc->key.offset) < chunk_item->size && chunk_item->num_stripes > 0) {
                    ci = chunk_item;
                    offset = sc->key.offset;
                    cis = (CHUNK_ITEM_STRIPE*)&chunk_item[1];
                    
                    devices = ExAllocatePoolWithTag(PagedPool, sizeof(device*) * ci->num_stripes, ALLOC_TAG);
                    if (!devices) {
                        ERR("out of memory\n");
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }
                    
                    for (i = 0; i < ci->num_stripes; i++) {
                        devices[i] = find_device_from_uuid(Vcb, &cis[i].dev_uuid);
                    }
                    
                    break;
                }
            }
            
            le = le->Flink;
        }
        
        if (!ci) {
            ERR("could not find chunk for %llx in bootstrap\n", addr);
            return STATUS_INTERNAL_ERROR;
        }
        
        if (pc)
            *pc = NULL;
    }
    
    if (ci->type & BLOCK_FLAG_DUPLICATE) {
        type = BLOCK_FLAG_DUPLICATE;
        allowed_missing = 0;
    } else if (ci->type & BLOCK_FLAG_RAID0) {
        type = BLOCK_FLAG_RAID0;
        allowed_missing = 0;
    } else if (ci->type & BLOCK_FLAG_RAID1) {
        type = BLOCK_FLAG_DUPLICATE;
        allowed_missing = 1;
    } else if (ci->type & BLOCK_FLAG_RAID10) {
        type = BLOCK_FLAG_RAID10;
        allowed_missing = 1;
    } else if (ci->type & BLOCK_FLAG_RAID5) {
        type = BLOCK_FLAG_RAID5;
        allowed_missing = 1;
    } else if (ci->type & BLOCK_FLAG_RAID6) {
        type = BLOCK_FLAG_RAID6;
        allowed_missing = 2;
    } else { // SINGLE
        type = BLOCK_FLAG_DUPLICATE;
        allowed_missing = 0;
    }

    cis = (CHUNK_ITEM_STRIPE*)&ci[1];

    context = ExAllocatePoolWithTag(NonPagedPool, sizeof(read_data_context), ALLOC_TAG);
    if (!context) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlZeroMemory(context, sizeof(read_data_context));
    KeInitializeEvent(&context->Event, NotificationEvent, FALSE);
    
    context->stripes = ExAllocatePoolWithTag(NonPagedPool, sizeof(read_data_stripe) * ci->num_stripes, ALLOC_TAG);
    if (!context->stripes) {
        ERR("out of memory\n");
        ExFreePool(context);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlZeroMemory(context->stripes, sizeof(read_data_stripe) * ci->num_stripes);
    
    context->buflen = length;
    context->num_stripes = ci->num_stripes;
    context->stripes_left = context->num_stripes;
    context->sector_size = Vcb->superblock.sector_size;
    context->csum = csum;
    context->tree = is_tree;
    context->type = type;
    context->check_nocsum_parity = check_nocsum_parity;
    
    stripestart = ExAllocatePoolWithTag(NonPagedPool, sizeof(UINT64) * ci->num_stripes, ALLOC_TAG);
    if (!stripestart) {
        ERR("out of memory\n");
        ExFreePool(context);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    stripeend = ExAllocatePoolWithTag(NonPagedPool, sizeof(UINT64) * ci->num_stripes, ALLOC_TAG);
    if (!stripeend) {
        ERR("out of memory\n");
        ExFreePool(stripestart);
        ExFreePool(context);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    if (type == BLOCK_FLAG_RAID0) {
        UINT64 startoff, endoff;
        UINT16 endoffstripe;
        
        get_raid0_offset(addr - offset, ci->stripe_length, ci->num_stripes, &startoff, &startoffstripe);
        get_raid0_offset(addr + length - offset - 1, ci->stripe_length, ci->num_stripes, &endoff, &endoffstripe);
        
        for (i = 0; i < ci->num_stripes; i++) {
            if (startoffstripe > i) {
                stripestart[i] = startoff - (startoff % ci->stripe_length) + ci->stripe_length;
            } else if (startoffstripe == i) {
                stripestart[i] = startoff;
            } else {
                stripestart[i] = startoff - (startoff % ci->stripe_length);
            }
            
            if (endoffstripe > i) {
                stripeend[i] = endoff - (endoff % ci->stripe_length) + ci->stripe_length;
            } else if (endoffstripe == i) {
                stripeend[i] = endoff + 1;
            } else {
                stripeend[i] = endoff - (endoff % ci->stripe_length);
            }
        }
    } else if (type == BLOCK_FLAG_RAID10) {
        UINT64 startoff, endoff;
        UINT16 endoffstripe, j;
        
        get_raid0_offset(addr - offset, ci->stripe_length, ci->num_stripes / ci->sub_stripes, &startoff, &startoffstripe);
        get_raid0_offset(addr + length - offset - 1, ci->stripe_length, ci->num_stripes / ci->sub_stripes, &endoff, &endoffstripe);
        
        if ((ci->num_stripes % ci->sub_stripes) != 0) {
            ERR("chunk %llx: num_stripes %x was not a multiple of sub_stripes %x!\n", offset, ci->num_stripes, ci->sub_stripes);
            Status = STATUS_INTERNAL_ERROR;
            goto exit;
        }
        
        context->firstoff = (startoff % ci->stripe_length) / Vcb->superblock.sector_size;
        context->startoffstripe = startoffstripe;
        context->sectors_per_stripe = ci->stripe_length / Vcb->superblock.sector_size;
        
        startoffstripe *= ci->sub_stripes;
        endoffstripe *= ci->sub_stripes;
        
        for (i = 0; i < ci->num_stripes; i += ci->sub_stripes) {
            if (startoffstripe > i) {
                stripestart[i] = startoff - (startoff % ci->stripe_length) + ci->stripe_length;
            } else if (startoffstripe == i) {
                stripestart[i] = startoff;
            } else {
                stripestart[i] = startoff - (startoff % ci->stripe_length);
            }
            
            if (endoffstripe > i) {
                stripeend[i] = endoff - (endoff % ci->stripe_length) + ci->stripe_length;
            } else if (endoffstripe == i) {
                stripeend[i] = endoff + 1;
            } else {
                stripeend[i] = endoff - (endoff % ci->stripe_length);
            }
            
            for (j = 1; j < ci->sub_stripes; j++) {
                stripestart[i+j] = stripestart[i];
                stripeend[i+j] = stripeend[i];
            }
        }
        
        context->stripes_cancel = 1;
    } else if (type == BLOCK_FLAG_DUPLICATE) {
        for (i = 0; i < ci->num_stripes; i++) {
            stripestart[i] = addr - offset;
            stripeend[i] = stripestart[i] + length;
        }
        
        context->stripes_cancel = ci->num_stripes - 1;
    } else if (type == BLOCK_FLAG_RAID5) {
        UINT64 startoff, endoff;
        UINT16 endoffstripe;
        UINT64 start = 0xffffffffffffffff, end = 0;
        
        get_raid0_offset(addr - offset, ci->stripe_length, ci->num_stripes - 1, &startoff, &startoffstripe);
        get_raid0_offset(addr + length - offset - 1, ci->stripe_length, ci->num_stripes - 1, &endoff, &endoffstripe);
        
        for (i = 0; i < ci->num_stripes - 1; i++) {
            UINT64 ststart, stend;
            
            if (startoffstripe > i) {
                ststart = startoff - (startoff % ci->stripe_length) + ci->stripe_length;
            } else if (startoffstripe == i) {
                ststart = startoff;
            } else {
                ststart = startoff - (startoff % ci->stripe_length);
            }
              
            if (endoffstripe > i) {
                stend = endoff - (endoff % ci->stripe_length) + ci->stripe_length;
            } else if (endoffstripe == i) {
                stend = endoff + 1;
            } else {
                stend = endoff - (endoff % ci->stripe_length);
            }
            
            if (ststart != stend) {
                if (ststart < start) {
                    start = ststart;
                    firststripesize = ci->stripe_length - (ststart % ci->stripe_length);
                }
                
                if (stend > end)
                    end = stend;
            }
        }
        
        for (i = 0; i < ci->num_stripes; i++) {
            stripestart[i] = start;
            stripeend[i] = end;
        }
        
        context->stripes_cancel = Vcb->options.raid5_recalculation;
    } else if (type == BLOCK_FLAG_RAID6) {
        UINT64 startoff, endoff;
        UINT16 endoffstripe;
        UINT64 start = 0xffffffffffffffff, end = 0;
        
        get_raid0_offset(addr - offset, ci->stripe_length, ci->num_stripes - 2, &startoff, &startoffstripe);
        get_raid0_offset(addr + length - offset - 1, ci->stripe_length, ci->num_stripes - 2, &endoff, &endoffstripe);
        
        for (i = 0; i < ci->num_stripes - 2; i++) {
            UINT64 ststart, stend;
            
            if (startoffstripe > i) {
                ststart = startoff - (startoff % ci->stripe_length) + ci->stripe_length;
            } else if (startoffstripe == i) {
                ststart = startoff;
            } else {
                ststart = startoff - (startoff % ci->stripe_length);
            }
              
            if (endoffstripe > i) {
                stend = endoff - (endoff % ci->stripe_length) + ci->stripe_length;
            } else if (endoffstripe == i) {
                stend = endoff + 1;
            } else {
                stend = endoff - (endoff % ci->stripe_length);
            }
            
            if (ststart != stend) {
                if (ststart < start) {
                    start = ststart;
                    firststripesize = ci->stripe_length - (ststart % ci->stripe_length);
                }
                
                if (stend > end)
                    end = stend;
            }
        }
        
        for (i = 0; i < ci->num_stripes; i++) {
            stripestart[i] = start;
            stripeend[i] = end;
        }
        
        context->stripes_cancel = Vcb->options.raid6_recalculation;
    }
    
    KeInitializeSpinLock(&context->spin_lock);
    
    context->address = addr;
    
    for (i = 0; i < ci->num_stripes; i++) {
        if (!devices[i] || stripestart[i] == stripeend[i]) {
            context->stripes[i].status = ReadDataStatus_MissingDevice;
            context->stripes[i].buf = NULL;
            context->stripes_left--;
            
            if (!devices[i])
                missing_devices++;
        }
    }
      
    if (missing_devices > allowed_missing) {
        ERR("not enough devices to service request (%u missing)\n", missing_devices);
        Status = STATUS_UNEXPECTED_IO_ERROR;
        goto exit;
    }
    
    for (i = 0; i < ci->num_stripes; i++) {
        PIO_STACK_LOCATION IrpSp;
        
        if (devices[i] && stripestart[i] != stripeend[i]) {
            context->stripes[i].context = (struct read_data_context*)context;
            context->stripes[i].buf = ExAllocatePoolWithTag(NonPagedPool, stripeend[i] - stripestart[i], ALLOC_TAG);
            
            if (!context->stripes[i].buf) {
                ERR("out of memory\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto exit;
            }
            
            if (type == BLOCK_FLAG_RAID10) {
                context->stripes[i].stripenum = i / ci->sub_stripes;
            }

            if (!Irp) {
                context->stripes[i].Irp = IoAllocateIrp(devices[i]->devobj->StackSize, FALSE);
                
                if (!context->stripes[i].Irp) {
                    ERR("IoAllocateIrp failed\n");
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto exit;
                }
            } else {
                context->stripes[i].Irp = IoMakeAssociatedIrp(Irp, devices[i]->devobj->StackSize);
                
                if (!context->stripes[i].Irp) {
                    ERR("IoMakeAssociatedIrp failed\n");
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto exit;
                }
            }
            
            IrpSp = IoGetNextIrpStackLocation(context->stripes[i].Irp);
            IrpSp->MajorFunction = IRP_MJ_READ;
            
            if (devices[i]->devobj->Flags & DO_BUFFERED_IO) {
                FIXME("FIXME - buffered IO\n");
            } else if (devices[i]->devobj->Flags & DO_DIRECT_IO) {
                context->stripes[i].Irp->MdlAddress = IoAllocateMdl(context->stripes[i].buf, stripeend[i] - stripestart[i], FALSE, FALSE, NULL);
                if (!context->stripes[i].Irp->MdlAddress) {
                    ERR("IoAllocateMdl failed\n");
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto exit;
                }
                
                MmProbeAndLockPages(context->stripes[i].Irp->MdlAddress, KernelMode, IoWriteAccess);
            } else {
                context->stripes[i].Irp->UserBuffer = context->stripes[i].buf;
            }

            IrpSp->Parameters.Read.Length = stripeend[i] - stripestart[i];
            IrpSp->Parameters.Read.ByteOffset.QuadPart = stripestart[i] + cis[i].offset;
            
            context->stripes[i].Irp->UserIosb = &context->stripes[i].iosb;
            
            IoSetCompletionRoutine(context->stripes[i].Irp, read_data_completion, &context->stripes[i], TRUE, TRUE, TRUE);

            context->stripes[i].status = ReadDataStatus_Pending;
        }
    }
    
#ifdef DEBUG_STATS
    if (!is_tree)
        time1 = KeQueryPerformanceCounter(NULL);
#endif
    
    for (i = 0; i < ci->num_stripes; i++) {
        if (context->stripes[i].status != ReadDataStatus_MissingDevice) {
            IoCallDriver(devices[i]->devobj, context->stripes[i].Irp);
        }
    }

    KeWaitForSingleObject(&context->Event, Executive, KernelMode, FALSE, NULL);
   
#ifdef DEBUG_STATS
    if (!is_tree) {
        time2 = KeQueryPerformanceCounter(NULL);
        
        Vcb->stats.read_disk_time += time2.QuadPart - time1.QuadPart;
    }
#endif
    
    // check if any of the devices return a "user-induced" error
    
    for (i = 0; i < ci->num_stripes; i++) {
        if (context->stripes[i].status == ReadDataStatus_Error && IoIsErrorUserInduced(context->stripes[i].iosb.Status)) {
            if (Irp && context->stripes[i].iosb.Status == STATUS_VERIFY_REQUIRED) {
                PDEVICE_OBJECT dev;
                
                dev = IoGetDeviceToVerify(Irp->Tail.Overlay.Thread);
                IoSetDeviceToVerify(Irp->Tail.Overlay.Thread, NULL);
                
                if (!dev) {
                    dev = IoGetDeviceToVerify(PsGetCurrentThread());
                    IoSetDeviceToVerify(PsGetCurrentThread(), NULL);
                }
                
                dev = Vcb->Vpb ? Vcb->Vpb->RealDevice : NULL;
                
                if (dev)
                    IoVerifyVolume(dev, FALSE);
            }
//             IoSetHardErrorOrVerifyDevice(context->stripes[i].Irp, devices[i]->devobj);
            
            Status = context->stripes[i].iosb.Status;
            goto exit;
        }
    }
    
    if (type == BLOCK_FLAG_RAID0) {
        Status = read_data_raid0(Vcb, buf, addr, length, context, ci, stripestart, stripeend, startoffstripe);
        if (!NT_SUCCESS(Status)) {
            ERR("read_data_raid0 returned %08x\n", Status);
            goto exit;
        }
    } else if (type == BLOCK_FLAG_RAID10) {
        Status = read_data_raid10(Vcb, buf, addr, length, Irp, context, ci, devices, stripestart, stripeend, startoffstripe);
        if (!NT_SUCCESS(Status)) {
            ERR("read_data_raid10 returned %08x\n", Status);
            goto exit;
        }
    } else if (type == BLOCK_FLAG_DUPLICATE) {
        Status = read_data_dup(Vcb, buf, addr, length, Irp, context, ci, devices, stripestart, stripeend);
        if (!NT_SUCCESS(Status)) {
            ERR("read_data_dup returned %08x\n", Status);
            goto exit;
        }
    } else if (type == BLOCK_FLAG_RAID5) {
        Status = read_data_raid5(Vcb, buf, addr, length, Irp, context, ci, devices, stripestart, stripeend, offset, firststripesize, check_nocsum_parity);
        if (!NT_SUCCESS(Status)) {
            ERR("read_data_raid5 returned %08x\n", Status);
            goto exit;
        }
    } else if (type == BLOCK_FLAG_RAID6) {
        Status = read_data_raid6(Vcb, buf, addr, length, Irp, context, ci, devices, stripestart, stripeend, offset, firststripesize, check_nocsum_parity);
        if (!NT_SUCCESS(Status)) {
            ERR("read_data_raid6 returned %08x\n", Status);
            goto exit;
        }
    }

exit:
    if (stripestart) ExFreePool(stripestart);
    if (stripeend) ExFreePool(stripeend);

    for (i = 0; i < ci->num_stripes; i++) {
        if (context->stripes[i].Irp) {
            if (devices[i]->devobj->Flags & DO_DIRECT_IO) {
                MmUnlockPages(context->stripes[i].Irp->MdlAddress);
                IoFreeMdl(context->stripes[i].Irp->MdlAddress);
            }
            IoFreeIrp(context->stripes[i].Irp);
        }
        
        if (context->stripes[i].buf)
            ExFreePool(context->stripes[i].buf);
    }

    ExFreePool(context->stripes);
    ExFreePool(context);
    
    if (!Vcb->log_to_phys_loaded)
        ExFreePool(devices);
        
    return Status;
}

static NTSTATUS STDCALL read_stream(fcb* fcb, UINT8* data, UINT64 start, ULONG length, ULONG* pbr) {
    ULONG readlen;
    NTSTATUS Status;
    
    TRACE("(%p, %p, %llx, %llx, %p)\n", fcb, data, start, length, pbr);
    
    if (pbr) *pbr = 0;
    
    if (start >= fcb->adsdata.Length) {
        TRACE("tried to read beyond end of stream\n");
        return STATUS_END_OF_FILE;
    }
    
    if (length == 0) {
        WARN("tried to read zero bytes\n");
        return STATUS_SUCCESS;
    }
    
    if (start + length < fcb->adsdata.Length)
        readlen = length;
    else
        readlen = fcb->adsdata.Length - (ULONG)start;
    
    if (readlen > 0)
        RtlCopyMemory(data + start, fcb->adsdata.Buffer, readlen);
    
    if (pbr) *pbr = readlen;
    
    Status = STATUS_SUCCESS;
       
    return Status;
}

NTSTATUS STDCALL read_file(fcb* fcb, UINT8* data, UINT64 start, UINT64 length, ULONG* pbr, PIRP Irp, BOOL check_nocsum_parity) {
    NTSTATUS Status;
    EXTENT_DATA* ed;
    UINT64 bytes_read = 0;
    UINT64 last_end;
    LIST_ENTRY* le;
#ifdef DEBUG_STATS
    LARGE_INTEGER time1, time2;
#endif
    
    TRACE("(%p, %p, %llx, %llx, %p)\n", fcb, data, start, length, pbr);
    
    if (pbr)
        *pbr = 0;
    
    if (start >= fcb->inode_item.st_size) {
        WARN("Tried to read beyond end of file\n");
        Status = STATUS_END_OF_FILE;
        goto exit;        
    }
    
#ifdef DEBUG_STATS
    time1 = KeQueryPerformanceCounter(NULL);
#endif

    le = fcb->extents.Flink;

    last_end = start;

    while (le != &fcb->extents) {
        UINT64 len;
        extent* ext = CONTAINING_RECORD(le, extent, list_entry);
        EXTENT_DATA2* ed2;
        
        if (!ext->ignore) {
            ed = ext->data;
            
            ed2 = (ed->type == EXTENT_TYPE_REGULAR || ed->type == EXTENT_TYPE_PREALLOC) ? (EXTENT_DATA2*)ed->data : NULL;
            
            len = ed2 ? ed2->num_bytes : ed->decoded_size;
            
            if (ext->offset + len <= start) {
                last_end = ext->offset + len;
                goto nextitem;
            }
            
            if (ext->offset > last_end && ext->offset > start + bytes_read) {
                UINT32 read = min(length, ext->offset - max(start, last_end));
                
                RtlZeroMemory(data + bytes_read, read);
                bytes_read += read;
                length -= read;
            }
            
            if (length == 0 || ext->offset > start + bytes_read + length)
                break;
            
            if (ed->encryption != BTRFS_ENCRYPTION_NONE) {
                WARN("Encryption not supported\n");
                Status = STATUS_NOT_IMPLEMENTED;
                goto exit;
            }
            
            if (ed->encoding != BTRFS_ENCODING_NONE) {
                WARN("Other encodings not supported\n");
                Status = STATUS_NOT_IMPLEMENTED;
                goto exit;
            }
            
            switch (ed->type) {
                case EXTENT_TYPE_INLINE:
                {
                    UINT64 off = start + bytes_read - ext->offset;
                    UINT64 read = len - off;
                    
                    if (read > length) read = length;
                    
                    RtlCopyMemory(data + bytes_read, &ed->data[off], read);
                    
                    // FIXME - can we have compressed inline extents?
                    
                    bytes_read += read;
                    length -= read;
                    break;
                }
                
                case EXTENT_TYPE_REGULAR:
                {
                    UINT64 off = start + bytes_read - ext->offset;
                    UINT32 to_read, read;
                    UINT8* buf;
                    BOOL buf_free;
                    UINT32 bumpoff = 0;
                    UINT64 addr, lockaddr, locklen;
                    chunk* c;
                    
                    read = len - off;
                    if (read > length) read = length;
                    
                    if (ed->compression == BTRFS_COMPRESSION_NONE) {
                        addr = ed2->address + ed2->offset + off;
                        to_read = sector_align(read, fcb->Vcb->superblock.sector_size);
                        
                        if (addr % fcb->Vcb->superblock.sector_size > 0) {
                            bumpoff = addr % fcb->Vcb->superblock.sector_size;
                            addr -= bumpoff;
                            to_read = sector_align(read + bumpoff, fcb->Vcb->superblock.sector_size);
                        }
                    } else {
                        addr = ed2->address;
                        to_read = sector_align(ed2->size, fcb->Vcb->superblock.sector_size);
                    }
                    
                    if (ed->compression == BTRFS_COMPRESSION_NONE && start % fcb->Vcb->superblock.sector_size == 0 &&
                        length % fcb->Vcb->superblock.sector_size == 0) {
                        buf = data + bytes_read;
                        buf_free = FALSE;
                    } else {
                        buf = ExAllocatePoolWithTag(PagedPool, to_read, ALLOC_TAG);
                        buf_free = TRUE;
                        
                        if (!buf) {
                            ERR("out of memory\n");
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            goto exit;
                        }
                    }
                    
                    c = get_chunk_from_address(fcb->Vcb, addr);
                    
                    if (!c) {
                        ERR("get_chunk_from_address(%llx) failed\n", addr);
                        
                        if (buf_free)
                            ExFreePool(buf);
                        
                        goto exit;
                    }
                    
                    if (c->chunk_item->type & BLOCK_FLAG_RAID5 || c->chunk_item->type & BLOCK_FLAG_RAID6) {
                        get_raid56_lock_range(c, addr, to_read, &lockaddr, &locklen);
                        chunk_lock_range(fcb->Vcb, c, lockaddr, locklen);
                    }
                    
                    Status = read_data(fcb->Vcb, addr, to_read, ext->csum ? &ext->csum[off / fcb->Vcb->superblock.sector_size] : NULL, FALSE,
                                       buf, c, NULL, Irp, check_nocsum_parity);
                    if (!NT_SUCCESS(Status)) {
                        ERR("read_data returned %08x\n", Status);
                        
                        if (c->chunk_item->type & BLOCK_FLAG_RAID5 || c->chunk_item->type & BLOCK_FLAG_RAID6)
                            chunk_unlock_range(fcb->Vcb, c, lockaddr, locklen);
                        
                        if (buf_free)
                            ExFreePool(buf);
                        
                        goto exit;
                    }
                    
                    if (c->chunk_item->type & BLOCK_FLAG_RAID5 || c->chunk_item->type & BLOCK_FLAG_RAID6)
                        chunk_unlock_range(fcb->Vcb, c, lockaddr, locklen);
                    
                    if (ed->compression == BTRFS_COMPRESSION_NONE) {
                        if (buf_free)
                            RtlCopyMemory(data + bytes_read, buf + bumpoff, read);
                    } else {
                        UINT8* decomp = NULL;
                        
                        // FIXME - don't mess around with decomp if we're reading the whole extent
                        
                        decomp = ExAllocatePoolWithTag(PagedPool, ed->decoded_size, ALLOC_TAG);
                        if (!decomp) {
                            ERR("out of memory\n");
                            ExFreePool(buf);
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            goto exit;
                        }
                        
                        Status = decompress(ed->compression, buf, ed2->size, decomp, ed->decoded_size);
                        
                        if (!NT_SUCCESS(Status)) {
                            ERR("decompress returned %08x\n", Status);
                            ExFreePool(buf);
                            ExFreePool(decomp);
                            goto exit;
                        }
                        
                        RtlCopyMemory(data + bytes_read, decomp + ed2->offset + off, min(read, ed2->num_bytes - off));
                        
                        ExFreePool(decomp);
                    }
                    
                    if (buf_free)
                        ExFreePool(buf);
                    
                    bytes_read += read;
                    length -= read;
                    
                    break;
                }
            
                case EXTENT_TYPE_PREALLOC:
                {
                    UINT64 off = start + bytes_read - ext->offset;
                    UINT32 read = len - off;
                    
                    if (read > length) read = length;

                    RtlZeroMemory(data + bytes_read, read);

                    bytes_read += read;
                    length -= read;
                    
                    break;
                }
                    
                default:
                    WARN("Unsupported extent data type %u\n", ed->type);
                    Status = STATUS_NOT_IMPLEMENTED;
                    goto exit;
            }

            last_end = ext->offset + len;
            
            if (length == 0)
                break;
        }

nextitem:
        le = le->Flink;
    }
    
    if (length > 0 && start + bytes_read < fcb->inode_item.st_size) {
        UINT32 read = min(fcb->inode_item.st_size - start - bytes_read, length);
        
        RtlZeroMemory(data + bytes_read, read);
        
        bytes_read += read;
        length -= read;
    }
    
    Status = STATUS_SUCCESS;
    if (pbr)
        *pbr = bytes_read;
    
#ifdef DEBUG_STATS
    time2 = KeQueryPerformanceCounter(NULL);
    
    fcb->Vcb->stats.num_reads++;
    fcb->Vcb->stats.data_read += bytes_read;
    fcb->Vcb->stats.read_total_time += time2.QuadPart - time1.QuadPart;
#endif
    
exit:
    return Status;
}

NTSTATUS do_read(PIRP Irp, BOOL wait, ULONG* bytes_read) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    fcb* fcb = FileObject->FsContext;
    UINT8* data;
    ULONG length, addon = 0;
    UINT64 start = IrpSp->Parameters.Read.ByteOffset.QuadPart;
    length = IrpSp->Parameters.Read.Length;
    *bytes_read = 0;
    
    if (!fcb || !fcb->Vcb || !fcb->subvol)
        return STATUS_INTERNAL_ERROR;
    
    TRACE("file = %S (fcb = %p)\n", file_desc(FileObject), fcb);
    TRACE("offset = %llx, length = %x\n", start, length);
    TRACE("paging_io = %s, no cache = %s\n", Irp->Flags & IRP_PAGING_IO ? "TRUE" : "FALSE", Irp->Flags & IRP_NOCACHE ? "TRUE" : "FALSE");

    if (!fcb->ads && fcb->type == BTRFS_TYPE_DIRECTORY)
        return STATUS_INVALID_DEVICE_REQUEST;
    
    if (!(Irp->Flags & IRP_PAGING_IO) && !FsRtlCheckLockForReadAccess(&fcb->lock, Irp)) {
        WARN("tried to read locked region\n");
        return STATUS_FILE_LOCK_CONFLICT;
    }
    
    if (length == 0) {
        TRACE("tried to read zero bytes\n");
        return STATUS_SUCCESS;
    }
    
    if (start >= fcb->Header.FileSize.QuadPart) {
        TRACE("tried to read with offset after file end (%llx >= %llx)\n", start, fcb->Header.FileSize.QuadPart);
        return STATUS_END_OF_FILE;
    }
    
    TRACE("FileObject %p fcb %p FileSize = %llx st_size = %llx (%p)\n", FileObject, fcb, fcb->Header.FileSize.QuadPart, fcb->inode_item.st_size, &fcb->inode_item.st_size);
//     int3;
    
    if (Irp->Flags & IRP_NOCACHE || !(IrpSp->MinorFunction & IRP_MN_MDL)) {
        data = map_user_buffer(Irp);
        
        if (Irp->MdlAddress && !data) {
            ERR("MmGetSystemAddressForMdlSafe returned NULL\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        if (start >= fcb->Header.ValidDataLength.QuadPart) {
            length = min(length, min(start + length, fcb->Header.FileSize.QuadPart) - fcb->Header.ValidDataLength.QuadPart);
            RtlZeroMemory(data, length);
            Irp->IoStatus.Information = *bytes_read = length;
            return STATUS_SUCCESS;
        }
        
        if (length + start > fcb->Header.ValidDataLength.QuadPart) {
            addon = min(start + length, fcb->Header.FileSize.QuadPart) - fcb->Header.ValidDataLength.QuadPart;
            RtlZeroMemory(data + (fcb->Header.ValidDataLength.QuadPart - start), addon);
            length = fcb->Header.ValidDataLength.QuadPart - start;
        }
    }
        
    if (!(Irp->Flags & IRP_NOCACHE)) {
        NTSTATUS Status = STATUS_SUCCESS;
        
        _SEH2_TRY {
            if (!FileObject->PrivateCacheMap) {
                CC_FILE_SIZES ccfs;
                
                ccfs.AllocationSize = fcb->Header.AllocationSize;
                ccfs.FileSize = fcb->Header.FileSize;
                ccfs.ValidDataLength = fcb->Header.ValidDataLength;
                
                init_file_cache(FileObject, &ccfs);
            }
            
            if (IrpSp->MinorFunction & IRP_MN_MDL) {
                CcMdlRead(FileObject,&IrpSp->Parameters.Read.ByteOffset, length, &Irp->MdlAddress, &Irp->IoStatus);
            } else {
                if (CcCopyReadEx) {
                    TRACE("CcCopyReadEx(%p, %llx, %x, %u, %p, %p, %p, %p)\n", FileObject, IrpSp->Parameters.Read.ByteOffset.QuadPart,
                          length, wait, data, &Irp->IoStatus, Irp->Tail.Overlay.Thread);
                    TRACE("sizes = %llx, %llx, %llx\n", fcb->Header.AllocationSize, fcb->Header.FileSize, fcb->Header.ValidDataLength);
                    if (!CcCopyReadEx(FileObject, &IrpSp->Parameters.Read.ByteOffset, length, wait, data, &Irp->IoStatus, Irp->Tail.Overlay.Thread)) {
                        TRACE("CcCopyReadEx could not wait\n");
                        
                        IoMarkIrpPending(Irp);
                        return STATUS_PENDING;
                    }
                    TRACE("CcCopyReadEx finished\n");
                } else {
                    TRACE("CcCopyRead(%p, %llx, %x, %u, %p, %p)\n", FileObject, IrpSp->Parameters.Read.ByteOffset.QuadPart, length, wait, data, &Irp->IoStatus);
                    TRACE("sizes = %llx, %llx, %llx\n", fcb->Header.AllocationSize, fcb->Header.FileSize, fcb->Header.ValidDataLength);
                    if (!CcCopyRead(FileObject, &IrpSp->Parameters.Read.ByteOffset, length, wait, data, &Irp->IoStatus)) {
                        TRACE("CcCopyRead could not wait\n");
                        
                        IoMarkIrpPending(Irp);
                        return STATUS_PENDING;
                    }
                    TRACE("CcCopyRead finished\n");
                }
            }
        } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
            Status = _SEH2_GetExceptionCode();
        } _SEH2_END;
        
        if (NT_SUCCESS(Status)) {
            Status = Irp->IoStatus.Status;
            Irp->IoStatus.Information += addon;
            *bytes_read = Irp->IoStatus.Information;
        } else
            ERR("EXCEPTION - %08x\n", Status);
        
        return Status;
    } else {
        NTSTATUS Status;
        
        if (!wait) {
            IoMarkIrpPending(Irp);
            return STATUS_PENDING;
        }
        
        if (!(Irp->Flags & IRP_PAGING_IO) && FileObject->SectionObjectPointer->DataSectionObject) {
            IO_STATUS_BLOCK iosb;
            
            CcFlushCache(FileObject->SectionObjectPointer, &IrpSp->Parameters.Read.ByteOffset, length, &iosb);
            
            if (!NT_SUCCESS(iosb.Status)) {
                ERR("CcFlushCache returned %08x\n", iosb.Status);
                return iosb.Status;
            }
        }
        
        if (fcb->ads)
            Status = read_stream(fcb, data, start, length, bytes_read);
        else
            Status = read_file(fcb, data, start, length, bytes_read, Irp, TRUE);
        
        *bytes_read += addon;
        TRACE("read %u bytes\n", *bytes_read);
        
        Irp->IoStatus.Information = *bytes_read;
        
        if (diskacc && Status != STATUS_PENDING) {
            PETHREAD thread = NULL;
            
            if (Irp->Tail.Overlay.Thread && !IoIsSystemThread(Irp->Tail.Overlay.Thread))
                thread = Irp->Tail.Overlay.Thread;
            else if (!IoIsSystemThread(PsGetCurrentThread()))
                thread = PsGetCurrentThread();
            else if (IoIsSystemThread(PsGetCurrentThread()) && IoGetTopLevelIrp() == Irp)
                thread = PsGetCurrentThread();
            
            if (thread)
                PsUpdateDiskCounters(PsGetThreadProcess(thread), *bytes_read, 0, 1, 0, 0);
        }
        
        return Status;
    }
}

NTSTATUS STDCALL drv_read(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    device_extension* Vcb = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    ULONG bytes_read;
    NTSTATUS Status;
    BOOL top_level;
    fcb* fcb;
    ccb* ccb;
    BOOL fcb_lock = FALSE, wait;
    
    FsRtlEnterFileSystem();
    
    top_level = is_top_level(Irp);
    
    TRACE("read\n");
    
    if (Vcb && Vcb->type == VCB_TYPE_PARTITION0) {
        Status = part0_passthrough(DeviceObject, Irp);
        goto exit2;
    }
    
    Irp->IoStatus.Information = 0;
    
    if (IrpSp->MinorFunction & IRP_MN_COMPLETE) {
        CcMdlReadComplete(IrpSp->FileObject, Irp->MdlAddress);
        
        Irp->MdlAddress = NULL;
        Status = STATUS_SUCCESS;
        bytes_read = 0;
        
        goto exit;
    }
    
    fcb = FileObject->FsContext;
    
    if (!fcb) {
        ERR("fcb was NULL\n");
        Status = STATUS_INVALID_PARAMETER;
        goto exit;
    }
    
    ccb = FileObject->FsContext2;
    
    if (!ccb) {
        ERR("ccb was NULL\n");
        Status = STATUS_INVALID_PARAMETER;
        goto exit;
    }
    
    if (Irp->RequestorMode == UserMode && !(ccb->access & FILE_READ_DATA)) {
        WARN("insufficient privileges\n");
        Status = STATUS_ACCESS_DENIED;
        goto exit;
    }
    
    if (fcb == Vcb->volume_fcb) {
        TRACE("reading volume FCB\n");
        
        IoSkipCurrentIrpStackLocation(Irp);
    
        Status = IoCallDriver(Vcb->Vpb->RealDevice, Irp);
        
        goto exit2;
    }
    
    wait = IoIsOperationSynchronous(Irp);
    
    // Don't offload jobs when doing paging IO - otherwise this can lead to
    // deadlocks in CcCopyRead.
    if (Irp->Flags & IRP_PAGING_IO)
        wait = TRUE;
    
    if (!ExIsResourceAcquiredSharedLite(fcb->Header.Resource)) {
        if (!ExAcquireResourceSharedLite(fcb->Header.Resource, wait)) {
            Status = STATUS_PENDING;
            IoMarkIrpPending(Irp);
            goto exit;
        }
        
        fcb_lock = TRUE;
    }
    
    Status = do_read(Irp, wait, &bytes_read);
    
exit:
    if (fcb_lock)
        ExReleaseResourceLite(fcb->Header.Resource);

    Irp->IoStatus.Status = Status;
    
    if (FileObject->Flags & FO_SYNCHRONOUS_IO && !(Irp->Flags & IRP_PAGING_IO))
        FileObject->CurrentByteOffset.QuadPart = IrpSp->Parameters.Read.ByteOffset.QuadPart + (NT_SUCCESS(Status) ? bytes_read : 0);
    
    // fastfat doesn't do this, but the Wine ntdll file test seems to think we ought to
    if (Irp->UserIosb)
        *Irp->UserIosb = Irp->IoStatus;
    
    TRACE("Irp->IoStatus.Status = %08x\n", Irp->IoStatus.Status);
    TRACE("Irp->IoStatus.Information = %lu\n", Irp->IoStatus.Information);
    TRACE("returning %08x\n", Status);
    
    if (Status != STATUS_PENDING)
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    else {
        if (!add_thread_job(Vcb, Irp))
            do_read_job(Irp);
    }
    
exit2:
    if (top_level) 
        IoSetTopLevelIrp(NULL);
    
    FsRtlExitFileSystem();
    
    return Status;
}
