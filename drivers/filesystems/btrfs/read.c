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
    PIRP Irp;
    IO_STATUS_BLOCK iosb;
    enum read_data_status status;
} read_data_stripe;

typedef struct {
    KEVENT Event;
    NTSTATUS Status;
    chunk* c;
    UINT32 buflen;
    UINT64 num_stripes;
    LONG stripes_left;
    UINT64 type;
    UINT32 sector_size;
    UINT16 firstoff, startoffstripe, sectors_per_stripe;
    UINT32* csum;
    BOOL tree;
    read_data_stripe* stripes;
} read_data_context;

static NTSTATUS STDCALL read_data_completion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID conptr) {
    read_data_stripe* stripe = conptr;
    read_data_context* context = (read_data_context*)stripe->context;
    UINT64 i;

    // FIXME - we definitely need a per-stripe lock here
    
    if (stripe->status == ReadDataStatus_Cancelling) {
        stripe->status = ReadDataStatus_Cancelled;
        goto end;
    }
    
    stripe->iosb = Irp->IoStatus;
    
    if (NT_SUCCESS(Irp->IoStatus.Status)) {
        if (context->type == BLOCK_FLAG_DUPLICATE) {
            if (context->tree) {
                tree_header* th = (tree_header*)stripe->buf;
                UINT32 crc32;
                
                crc32 = ~calc_crc32c(0xffffffff, (UINT8*)&th->fs_uuid, context->buflen - sizeof(th->csum));
                
                if (crc32 != *((UINT32*)th->csum))
                    stripe->status = ReadDataStatus_CRCError;
            } else if (context->csum) {
                for (i = 0; i < Irp->IoStatus.Information / context->sector_size; i++) {
                    UINT32 crc32 = ~calc_crc32c(0xffffffff, stripe->buf + (i * context->sector_size), context->sector_size);
                    
                    if (crc32 != context->csum[i]) {
                        stripe->status = ReadDataStatus_CRCError;
                        goto end;
                    }
                }
            }
            
            stripe->status = ReadDataStatus_Success;
                
            for (i = 0; i < context->num_stripes; i++) {
                if (context->stripes[i].status == ReadDataStatus_Pending) {
                    context->stripes[i].status = ReadDataStatus_Cancelling;
                    IoCancelIrp(context->stripes[i].Irp);
                }
            }
        } else if (context->type == BLOCK_FLAG_RAID0) {
            // no point checking the checksum here, as there's nothing we can do
            stripe->status = ReadDataStatus_Success;
        } else if (context->type == BLOCK_FLAG_RAID10) {
            if (context->csum) {
                UINT16 start, left;
                UINT32 j;
                
                if (context->startoffstripe == stripe->stripenum) {
                    start = 0;
                    left = context->sectors_per_stripe - context->firstoff;
                } else {
                    UINT16 ns;
                    
                    if (context->startoffstripe > stripe->stripenum) {
                        ns = stripe->stripenum + (context->num_stripes / 2) - context->startoffstripe;
                    } else {
                        ns = stripe->stripenum - context->startoffstripe;
                    }
                    
                    if (context->firstoff == 0)
                        start = context->sectors_per_stripe * ns;
                    else
                        start = (context->sectors_per_stripe - context->firstoff) + (context->sectors_per_stripe * (ns - 1));
                    
                    left = context->sectors_per_stripe;
                }
                
                j = start;
                for (i = 0; i < Irp->IoStatus.Information / context->sector_size; i++) {
                    UINT32 crc32 = ~calc_crc32c(0xffffffff, stripe->buf + (i * context->sector_size), context->sector_size);
                    
                    if (crc32 != context->csum[j]) {
                        int3;
                        stripe->status = ReadDataStatus_CRCError;
                        goto end;
                    }
                    
                    j++;
                    left--;
                    
                    if (left == 0) {
                        j += context->sectors_per_stripe;
                        left = context->sectors_per_stripe;
                    }
                }
            }
            
            stripe->status = ReadDataStatus_Success;
            
            for (i = 0; i < context->num_stripes; i++) {
                if (context->stripes[i].status == ReadDataStatus_Pending && context->stripes[i].stripenum == stripe->stripenum) {
                    context->stripes[i].status = ReadDataStatus_Cancelling;
                    IoCancelIrp(context->stripes[i].Irp);
                }
            }
        }
            
        goto end;
    } else {
        stripe->status = ReadDataStatus_Error;
    }
    
end:
    if (InterlockedDecrement(&context->stripes_left) == 0)
        KeSetEvent(&context->Event, 0, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS STDCALL read_data(device_extension* Vcb, UINT64 addr, UINT32 length, UINT32* csum, BOOL is_tree, UINT8* buf, chunk** pc, PIRP Irp) {
    CHUNK_ITEM* ci;
    CHUNK_ITEM_STRIPE* cis;
    read_data_context* context;
    UINT64 i, type, offset;
    NTSTATUS Status;
    device** devices;
    UINT64 *stripestart = NULL, *stripeend = NULL;
    UINT16 startoffstripe;
    
    Status = verify_vcb(Vcb, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("verify_vcb returned %08x\n", Status);
        return Status;
    }
    
    if (Vcb->log_to_phys_loaded) {
        chunk* c = get_chunk_from_address(Vcb, addr);
        
        if (!c) {
            ERR("get_chunk_from_address failed\n");
            return STATUS_INTERNAL_ERROR;
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
    } else if (ci->type & BLOCK_FLAG_RAID0) {
        type = BLOCK_FLAG_RAID0;
    } else if (ci->type & BLOCK_FLAG_RAID1) {
        type = BLOCK_FLAG_DUPLICATE;
    } else if (ci->type & BLOCK_FLAG_RAID10) {
        type = BLOCK_FLAG_RAID10;
    } else if (ci->type & BLOCK_FLAG_RAID5) {
        FIXME("RAID5 not yet supported\n");
        return STATUS_NOT_IMPLEMENTED;
    } else if (ci->type & BLOCK_FLAG_RAID6) {
        FIXME("RAID6 not yet supported\n");
        return STATUS_NOT_IMPLEMENTED;
    } else { // SINGLE
        type = BLOCK_FLAG_DUPLICATE;
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
    
    stripestart = ExAllocatePoolWithTag(PagedPool, sizeof(UINT64) * ci->num_stripes, ALLOC_TAG);
    if (!stripestart) {
        ERR("out of memory\n");
        ExFreePool(context);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    stripeend = ExAllocatePoolWithTag(PagedPool, sizeof(UINT64) * ci->num_stripes, ALLOC_TAG);
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
    } else if (type == BLOCK_FLAG_DUPLICATE) {
        for (i = 0; i < ci->num_stripes; i++) {
            stripestart[i] = addr - offset;
            stripeend[i] = stripestart[i] + length;
        }
    }
    
    // FIXME - for RAID, check beforehand whether there's enough devices to satisfy request
    
    for (i = 0; i < ci->num_stripes; i++) {
        PIO_STACK_LOCATION IrpSp;
        
        if (!devices[i] || stripestart[i] == stripeend[i]) {
            context->stripes[i].status = ReadDataStatus_MissingDevice;
            context->stripes[i].buf = NULL;
            context->stripes_left--;
        } else {
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
    
    for (i = 0; i < ci->num_stripes; i++) {
        if (context->stripes[i].status != ReadDataStatus_MissingDevice) {
            IoCallDriver(devices[i]->devobj, context->stripes[i].Irp);
        }
    }

    KeWaitForSingleObject(&context->Event, Executive, KernelMode, FALSE, NULL);
    
    // FIXME - if checksum error, write good data over bad
    
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
        UINT32 pos, *stripeoff;
        UINT8 stripe;
        
        for (i = 0; i < ci->num_stripes; i++) {
            if (context->stripes[i].status == ReadDataStatus_Error) {
                WARN("stripe %llu returned error %08x\n", i, context->stripes[i].iosb.Status); 
                Status = context->stripes[i].iosb.Status;
                goto exit;
            }
        }
        
        pos = 0;
        stripeoff = ExAllocatePoolWithTag(PagedPool, sizeof(UINT32) * ci->num_stripes, ALLOC_TAG);
        if (!stripeoff) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit;
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
        
        if (is_tree) { // shouldn't happen, as trees shouldn't cross stripe boundaries
            tree_header* th = (tree_header*)buf;
            UINT32 crc32 = ~calc_crc32c(0xffffffff, (UINT8*)&th->fs_uuid, Vcb->superblock.node_size - sizeof(th->csum));
            
            if (crc32 != *((UINT32*)th->csum)) {
                WARN("crc32 was %08x, expected %08x\n", crc32, *((UINT32*)th->csum));
                Status = STATUS_CRC_ERROR;
                goto exit;
            }
        } else if (csum) {
            for (i = 0; i < length / Vcb->superblock.sector_size; i++) {
                UINT32 crc32 = ~calc_crc32c(0xffffffff, buf + (i * Vcb->superblock.sector_size), Vcb->superblock.sector_size);
                
                if (crc32 != csum[i]) {
                    WARN("checksum error (%08x != %08x)\n", crc32, csum[i]);
                    Status = STATUS_CRC_ERROR;
                    goto exit;
                }
            }
        }
        
        Status = STATUS_SUCCESS;
    } else if (type == BLOCK_FLAG_RAID10) {
        UINT32 pos, *stripeoff;
        UINT8 stripe;
        read_data_stripe** stripes;
        
        stripes = ExAllocatePoolWithTag(NonPagedPool, sizeof(read_data_stripe*) * ci->num_stripes / ci->sub_stripes, ALLOC_TAG);
        if (!stripes) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit;
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
                    if (context->stripes[i+j].status == ReadDataStatus_CRCError) {
                        WARN("stripe %llu had a checksum error\n", i+j);
                        Status = STATUS_CRC_ERROR;
                        goto exit;
                    }
                }
                
                for (j = 0; j < ci->sub_stripes; j++) {
                    if (context->stripes[i+j].status == ReadDataStatus_Error) {
                        WARN("stripe %llu returned error %08x\n", i+j, context->stripes[i+j].iosb.Status);
                        Status = context->stripes[i].iosb.Status;
                        goto exit;
                    }
                }
            }
        }
        
        pos = 0;
        stripeoff = ExAllocatePoolWithTag(PagedPool, sizeof(UINT32) * ci->num_stripes / ci->sub_stripes, ALLOC_TAG);
        if (!stripeoff) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit;
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
        
        ExFreePool(stripes);
        ExFreePool(stripeoff);
        
        // FIXME - handle the case where one of the stripes doesn't read everything, i.e. Irp->IoStatus.Information is short
        
        if (is_tree) {
            tree_header* th = (tree_header*)buf;
            UINT32 crc32 = ~calc_crc32c(0xffffffff, (UINT8*)&th->fs_uuid, Vcb->superblock.node_size - sizeof(th->csum));
            
            if (crc32 != *((UINT32*)th->csum)) {
                WARN("crc32 was %08x, expected %08x\n", crc32, *((UINT32*)th->csum));
                Status = STATUS_CRC_ERROR;
                goto exit;
            }
        }
        
        Status = STATUS_SUCCESS;
    } else if (type == BLOCK_FLAG_DUPLICATE) {
        // check if any of the stripes succeeded
        
        for (i = 0; i < ci->num_stripes; i++) {
            if (context->stripes[i].status == ReadDataStatus_Success) {
                RtlCopyMemory(buf, context->stripes[i].buf, length);
                Status = STATUS_SUCCESS;
                goto exit;
            }
        }
        
        // if not, see if we got a checksum error
        
        for (i = 0; i < ci->num_stripes; i++) {
            if (context->stripes[i].status == ReadDataStatus_CRCError) {
#ifdef _DEBUG
                WARN("stripe %llu had a checksum error\n", i);
                
                if (context->tree) {
                    tree_header* th = (tree_header*)context->stripes[i].buf;
                    UINT32 crc32 = ~calc_crc32c(0xffffffff, (UINT8*)&th->fs_uuid, context->buflen - sizeof(th->csum));
                    
                    WARN("crc32 was %08x, expected %08x\n", crc32, *((UINT32*)th->csum));
                }
#endif
                
                Status = STATUS_CRC_ERROR;
                goto exit;
            }
        }
        
        // failing that, return the first error we encountered
        
        for (i = 0; i < ci->num_stripes; i++) {
            if (context->stripes[i].status == ReadDataStatus_Error) {
                Status = context->stripes[i].iosb.Status;
                goto exit;
            }
        }
        
        // if we somehow get here, return STATUS_INTERNAL_ERROR
        
        Status = STATUS_INTERNAL_ERROR;
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

static NTSTATUS load_csum_from_disk(device_extension* Vcb, UINT32* csum, UINT64 start, UINT64 length, PIRP Irp) {
    NTSTATUS Status;
    KEY searchkey;
    traverse_ptr tp, next_tp;
    UINT64 i, j;
    BOOL b;
    
    searchkey.obj_id = EXTENT_CSUM_ID;
    searchkey.obj_type = TYPE_EXTENT_CSUM;
    searchkey.offset = start;
    
    Status = find_item(Vcb, Vcb->checksum_root, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    i = 0;
    do {
        if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == searchkey.obj_type) {
            ULONG readlen;
            
            if (start < tp.item->key.offset)
                j = 0;
            else
                j = ((start - tp.item->key.offset) / Vcb->superblock.sector_size) + i;
            
            if (j * sizeof(UINT32) > tp.item->size || tp.item->key.offset > start + (i * Vcb->superblock.sector_size)) {
                ERR("checksum not found for %llx\n", start + (i * Vcb->superblock.sector_size));
                return STATUS_INTERNAL_ERROR;
            }
            
            readlen = min((tp.item->size / sizeof(UINT32)) - j, length - i);
            RtlCopyMemory(&csum[i], tp.item->data + (j * sizeof(UINT32)), readlen * sizeof(UINT32));
            i += readlen;
            
            if (i == length)
                break;
        }
        
        b = find_next_item(Vcb, &tp, &next_tp, FALSE, Irp);
        
        if (b)
            tp = next_tp;
    } while (b);
    
    if (i < length) {
        ERR("could not read checksums: offset %llx, length %llx sectors\n", start, length);
        return STATUS_INTERNAL_ERROR;
    }
    
    return STATUS_SUCCESS;
}

static NTSTATUS load_csum(device_extension* Vcb, UINT64 start, UINT64 length, UINT32** pcsum, PIRP Irp) {
    UINT32* csum = NULL;
    NTSTATUS Status;
    UINT64 end;
    RTL_BITMAP bmp;
    ULONG *bmpbuf = NULL, bmpbuflen, index, runlength;
    LIST_ENTRY* le;
    
    if (length == 0) {
        *pcsum = NULL;
        return STATUS_SUCCESS;
    }
    
    bmpbuflen = sector_align(length, sizeof(ULONG) * 8) / 8;
    bmpbuf = ExAllocatePoolWithTag(PagedPool, bmpbuflen, ALLOC_TAG);
    if (!bmpbuf) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }
    
    RtlInitializeBitMap(&bmp, bmpbuf, length);
    RtlClearAllBits(&bmp);
    
    csum = ExAllocatePoolWithTag(NonPagedPool, sizeof(UINT32) * length, ALLOC_TAG);
    if (!csum) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }
    
    ExAcquireResourceSharedLite(&Vcb->checksum_lock, TRUE);
    
    end = start + (length * Vcb->superblock.sector_size);
    
    le = Vcb->sector_checksums.Flink;
    while (le != &Vcb->sector_checksums) {
        changed_sector* cs = (changed_sector*)le;
        UINT64 cs_end = cs->ol.key + (cs->length * Vcb->superblock.sector_size);
        
        if (cs->ol.key <= start && cs_end >= end) { // outer
            if (cs->deleted) {
                RtlClearAllBits(&bmp);
            } else {
                RtlSetAllBits(&bmp);
                RtlCopyMemory(csum, &cs->checksums[(start - cs->ol.key) / Vcb->superblock.sector_size], sizeof(UINT32) * length);
            }
        } else if (cs->ol.key >= start && cs->ol.key <= end) { // right or inner
            if (cs->deleted) {
                RtlClearBits(&bmp, (cs->ol.key - start) / Vcb->superblock.sector_size, (min(end, cs_end) - cs->ol.key) / Vcb->superblock.sector_size);
            } else {
                RtlSetBits(&bmp, (cs->ol.key - start) / Vcb->superblock.sector_size, (min(end, cs_end) - cs->ol.key) / Vcb->superblock.sector_size);
                RtlCopyMemory(&csum[(cs->ol.key - start) / Vcb->superblock.sector_size], cs->checksums, (min(end, cs_end) - cs->ol.key) * sizeof(UINT32) / Vcb->superblock.sector_size);
            }
        } else if (cs_end >= start && cs_end <= end) { // left
            if (cs->deleted) {
                RtlClearBits(&bmp, 0, (cs_end - start) / Vcb->superblock.sector_size);
            } else {
                RtlSetBits(&bmp, 0, (cs_end - start) / Vcb->superblock.sector_size);
                RtlCopyMemory(csum, &cs->checksums[(start - cs->ol.key) / Vcb->superblock.sector_size], (cs_end - start) * sizeof(UINT32) / Vcb->superblock.sector_size);
            }
        }
        
        le = le->Flink;
    }
    
    ExReleaseResourceLite(&Vcb->checksum_lock);
    
    runlength = RtlFindFirstRunClear(&bmp, &index);
            
    while (runlength != 0) {
        Status = load_csum_from_disk(Vcb, &csum[index], start + (index * Vcb->superblock.sector_size), runlength, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("load_csum_from_disk returned %08x\n", Status);
            goto end;
        }
       
        runlength = RtlFindNextForwardRunClear(&bmp, index + runlength, &index);
    }
    
    Status = STATUS_SUCCESS;
    
end:
    if (bmpbuf)
        ExFreePool(bmpbuf);
    
    if (NT_SUCCESS(Status))
        *pcsum = csum;
    else if (csum)
        ExFreePool(csum);

    return Status;
}

NTSTATUS STDCALL read_file(fcb* fcb, UINT8* data, UINT64 start, UINT64 length, ULONG* pbr, PIRP Irp) {
    NTSTATUS Status;
    EXTENT_DATA* ed;
    UINT64 bytes_read = 0;
    UINT64 last_end;
    LIST_ENTRY* le;
    
    TRACE("(%p, %p, %llx, %llx, %p)\n", fcb, data, start, length, pbr);
    
    if (pbr)
        *pbr = 0;
    
    if (start >= fcb->inode_item.st_size) {
        WARN("Tried to read beyond end of file\n");
        Status = STATUS_END_OF_FILE;
        goto exit;        
    }

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
                    UINT32 *csum, bumpoff = 0;
                    UINT64 addr;
                    
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
                    
                    buf = ExAllocatePoolWithTag(PagedPool, to_read, ALLOC_TAG);
                    
                    if (!buf) {
                        ERR("out of memory\n");
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        goto exit;
                    }
                    
                    if (!(fcb->inode_item.flags & BTRFS_INODE_NODATASUM)) {
                        Status = load_csum(fcb->Vcb, addr, to_read / fcb->Vcb->superblock.sector_size, &csum, Irp);
                        
                        if (!NT_SUCCESS(Status)) {
                            ERR("load_csum returned %08x\n", Status);
                            ExFreePool(buf);
                            goto exit;
                        }
                    } else
                        csum = NULL;
                    
                    Status = read_data(fcb->Vcb, addr, to_read, csum, FALSE, buf, NULL, Irp);
                    if (!NT_SUCCESS(Status)) {
                        ERR("read_data returned %08x\n", Status);
                        ExFreePool(buf);
                        goto exit;
                    }
                    
                    if (ed->compression == BTRFS_COMPRESSION_NONE) {
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
                    
                    ExFreePool(buf);
                    
                    if (csum)
                        ExFreePool(csum);
                    
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

    if (fcb->type == BTRFS_TYPE_DIRECTORY)
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
                
                TRACE("calling CcInitializeCacheMap (%llx, %llx, %llx)\n",
                            ccfs.AllocationSize.QuadPart, ccfs.FileSize.QuadPart, ccfs.ValidDataLength.QuadPart);
                CcInitializeCacheMap(FileObject, &ccfs, FALSE, cache_callbacks, FileObject);

                CcSetReadAheadGranularity(FileObject, READ_AHEAD_GRANULARITY);
            }
            
            if (IrpSp->MinorFunction & IRP_MN_MDL) {
                CcMdlRead(FileObject,&IrpSp->Parameters.Read.ByteOffset, length, &Irp->MdlAddress, &Irp->IoStatus);
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
        
        ExAcquireResourceSharedLite(&fcb->Vcb->tree_lock, TRUE);
    
        if (fcb->ads)
            Status = read_stream(fcb, data, start, length, bytes_read);
        else
            Status = read_file(fcb, data, start, length, bytes_read, Irp);
        
        ExReleaseResourceLite(&fcb->Vcb->tree_lock);
        
        *bytes_read += addon;
        TRACE("read %u bytes\n", *bytes_read);
        
        Irp->IoStatus.Information = *bytes_read;
        
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
    BOOL tree_lock = FALSE, fcb_lock = FALSE, pagefile;
    
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
    
    if (fcb == Vcb->volume_fcb) {
        TRACE("not allowing read of volume FCB\n");
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
    
    pagefile = fcb->Header.Flags2 & FSRTL_FLAG2_IS_PAGING_FILE && Irp->Flags & IRP_PAGING_IO;
    
    if (Irp->Flags & IRP_NOCACHE) {
        if (!pagefile) {
            if (!ExAcquireResourceSharedLite(&Vcb->tree_lock, IoIsOperationSynchronous(Irp))) {
                Status = STATUS_PENDING;
                IoMarkIrpPending(Irp);
                goto exit;
            }
            
            tree_lock = TRUE;
        }
    
        if (!ExAcquireResourceSharedLite(fcb->Header.Resource, IoIsOperationSynchronous(Irp))) {
            Status = STATUS_PENDING;
            IoMarkIrpPending(Irp);
            goto exit;
        }
        
        fcb_lock = TRUE;
    }
    
    Status = do_read(Irp, IoIsOperationSynchronous(Irp), &bytes_read);
    
exit:
    if (fcb_lock)
        ExReleaseResourceLite(fcb->Header.Resource);
    
    if (tree_lock)
        ExReleaseResourceLite(&Vcb->tree_lock);

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
