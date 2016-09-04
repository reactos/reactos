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

#define MAX_CSUM_SIZE (4096 - sizeof(tree_header) - sizeof(leaf_node))

// #define DEBUG_WRITE_LOOPS

// BOOL did_split;
BOOL chunk_test = FALSE;

typedef struct {
    KEVENT Event;
    IO_STATUS_BLOCK iosb;
} write_context;

typedef struct {
    EXTENT_ITEM ei;
    UINT8 type;
    EXTENT_DATA_REF edr;
} EXTENT_ITEM_DATA_REF;

typedef struct {
    EXTENT_ITEM_TREE eit;
    UINT8 type;
    TREE_BLOCK_REF tbr;
} EXTENT_ITEM_TREE2;

typedef struct {
    EXTENT_ITEM ei;
    UINT8 type;
    TREE_BLOCK_REF tbr;
} EXTENT_ITEM_SKINNY_METADATA;

// static BOOL extent_item_is_shared(EXTENT_ITEM* ei, ULONG len);
static NTSTATUS STDCALL write_data_completion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID conptr);
static void update_checksum_tree(device_extension* Vcb, PIRP Irp, LIST_ENTRY* rollback);
static void remove_fcb_extent(fcb* fcb, extent* ext, LIST_ENTRY* rollback);

static NTSTATUS STDCALL write_completion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID conptr) {
    write_context* context = conptr;
    
    context->iosb = Irp->IoStatus;
    KeSetEvent(&context->Event, 0, FALSE);
    
//     return STATUS_SUCCESS;
    return STATUS_MORE_PROCESSING_REQUIRED;
}

static NTSTATUS STDCALL write_data_phys(PDEVICE_OBJECT device, UINT64 address, void* data, UINT32 length) {
    NTSTATUS Status;
    LARGE_INTEGER offset;
    PIRP Irp;
    PIO_STACK_LOCATION IrpSp;
    write_context* context = NULL;
    
    TRACE("(%p, %llx, %p, %x)\n", device, address, data, length);
    
    context = ExAllocatePoolWithTag(NonPagedPool, sizeof(write_context), ALLOC_TAG);
    if (!context) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlZeroMemory(context, sizeof(write_context));
    
    KeInitializeEvent(&context->Event, NotificationEvent, FALSE);
    
    offset.QuadPart = address;
    
//     Irp = IoBuildSynchronousFsdRequest(IRP_MJ_WRITE, Vcb->device, data, length, &offset, NULL, &context->iosb);
    
    Irp = IoAllocateIrp(device->StackSize, FALSE);
    
    if (!Irp) {
        ERR("IoAllocateIrp failed\n");
        Status = STATUS_INTERNAL_ERROR;
        goto exit2;
    }
    
    IrpSp = IoGetNextIrpStackLocation(Irp);
    IrpSp->MajorFunction = IRP_MJ_WRITE;
    
    if (device->Flags & DO_BUFFERED_IO) {
        Irp->AssociatedIrp.SystemBuffer = data;

        Irp->Flags = IRP_BUFFERED_IO;
    } else if (device->Flags & DO_DIRECT_IO) {
        Irp->MdlAddress = IoAllocateMdl(data, length, FALSE, FALSE, NULL);
        if (!Irp->MdlAddress) {
            DbgPrint("IoAllocateMdl failed\n");
            goto exit;
        }
        
        MmProbeAndLockPages(Irp->MdlAddress, KernelMode, IoWriteAccess);
    } else {
        Irp->UserBuffer = data;
    }

    IrpSp->Parameters.Write.Length = length;
    IrpSp->Parameters.Write.ByteOffset = offset;
    
    Irp->UserIosb = &context->iosb;

    Irp->UserEvent = &context->Event;

    IoSetCompletionRoutine(Irp, write_completion, context, TRUE, TRUE, TRUE);

    // FIXME - support multiple devices
    Status = IoCallDriver(device, Irp);
    
    if (Status == STATUS_PENDING) {
        KeWaitForSingleObject(&context->Event, Executive, KernelMode, FALSE, NULL);
        Status = context->iosb.Status;
    }
    
    if (!NT_SUCCESS(Status)) {
        ERR("IoCallDriver returned %08x\n", Status);
    }
    
    if (device->Flags & DO_DIRECT_IO) {
        MmUnlockPages(Irp->MdlAddress);
        IoFreeMdl(Irp->MdlAddress);
    }
    
exit:
    IoFreeIrp(Irp);
    
exit2:
    if (context)
        ExFreePool(context);
    
    return Status;
}

static NTSTATUS STDCALL write_superblock(device_extension* Vcb, device* device) {
    NTSTATUS Status;
    unsigned int i = 0;
    UINT32 crc32;

#ifdef __REACTOS__
    Status = STATUS_INTERNAL_ERROR;
#endif
    
    RtlCopyMemory(&Vcb->superblock.dev_item, &device->devitem, sizeof(DEV_ITEM));
    
    // FIXME - only write one superblock if on SSD (?)
    while (superblock_addrs[i] > 0 && device->length >= superblock_addrs[i] + sizeof(superblock)) {
        TRACE("writing superblock %u\n", i);
        
        Vcb->superblock.sb_phys_addr = superblock_addrs[i];
        
        crc32 = calc_crc32c(0xffffffff, (UINT8*)&Vcb->superblock.uuid, (ULONG)sizeof(superblock) - sizeof(Vcb->superblock.checksum));
        crc32 = ~crc32;
        TRACE("crc32 is %08x\n", crc32);
        RtlCopyMemory(&Vcb->superblock.checksum, &crc32, sizeof(UINT32));
        
        Status = write_data_phys(device->devobj, superblock_addrs[i], &Vcb->superblock, sizeof(superblock));
        
        if (!NT_SUCCESS(Status))
            break;
        
        i++;
    }
    
    if (i == 0) {
        ERR("no superblocks written!\n");
    }

    return Status;
}

static BOOL find_address_in_chunk(device_extension* Vcb, chunk* c, UINT64 length, UINT64* address) {
    LIST_ENTRY* le;
    space* s;
    
    TRACE("(%p, %llx, %llx, %p)\n", Vcb, c->offset, length, address);
    
    if (IsListEmpty(&c->space_size))
        return FALSE;
    
    le = c->space_size.Flink;
    while (le != &c->space_size) {
        s = CONTAINING_RECORD(le, space, list_entry_size);
        
        if (s->size == length) {
            *address = s->address;
            return TRUE;
        } else if (s->size < length) {
            if (le == c->space_size.Flink)
                return FALSE;
            
            s = CONTAINING_RECORD(le->Blink, space, list_entry_size);
            
            *address = s->address;
            return TRUE;
        }
        
        le = le->Flink;
    }
    
    s = CONTAINING_RECORD(c->space_size.Blink, space, list_entry_size);
    
    if (s->size > length) {
        *address = s->address;
        return TRUE;
    }
    
    return FALSE;
}

chunk* get_chunk_from_address(device_extension* Vcb, UINT64 address) {
    LIST_ENTRY* le2;
    chunk* c;
    
    ExAcquireResourceSharedLite(&Vcb->chunk_lock, TRUE);
    
    le2 = Vcb->chunks.Flink;
    while (le2 != &Vcb->chunks) {
        c = CONTAINING_RECORD(le2, chunk, list_entry);
        
//         TRACE("chunk: %llx, %llx\n", c->offset, c->chunk_item->size);
        
        if (address >= c->offset && address < c->offset + c->chunk_item->size) {
            ExReleaseResourceLite(&Vcb->chunk_lock);
            return c;
        }
         
        le2 = le2->Flink;
    }
    
    ExReleaseResourceLite(&Vcb->chunk_lock);
    
    return NULL;
}

typedef struct {
    space* dh;
    device* device;
} stripe;

static UINT64 find_new_chunk_address(device_extension* Vcb, UINT64 size) {
    UINT64 lastaddr;
    LIST_ENTRY* le;
    
    lastaddr = 0;
    
    le = Vcb->chunks.Flink;
    while (le != &Vcb->chunks) {
        chunk* c = CONTAINING_RECORD(le, chunk, list_entry);
        
        if (c->offset >= lastaddr + size)
            return lastaddr;
        
        lastaddr = c->offset + c->chunk_item->size;
        
        le = le->Flink;
    }
    
    return lastaddr;
}

static NTSTATUS update_dev_item(device_extension* Vcb, device* device, PIRP Irp, LIST_ENTRY* rollback) {
    KEY searchkey;
    traverse_ptr tp;
    DEV_ITEM* di;
    NTSTATUS Status;
    
    searchkey.obj_id = 1;
    searchkey.obj_type = TYPE_DEV_ITEM;
    searchkey.offset = device->devitem.dev_id;
    
    Status = find_item(Vcb, Vcb->chunk_root, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    if (keycmp(&tp.item->key, &searchkey)) {
        ERR("error - could not find DEV_ITEM for device %llx\n", device->devitem.dev_id);
        return STATUS_INTERNAL_ERROR;
    }
    
    delete_tree_item(Vcb, &tp, rollback);
    
    di = ExAllocatePoolWithTag(PagedPool, sizeof(DEV_ITEM), ALLOC_TAG);
    if (!di) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlCopyMemory(di, &device->devitem, sizeof(DEV_ITEM));
    
    if (!insert_tree_item(Vcb, Vcb->chunk_root, 1, TYPE_DEV_ITEM, device->devitem.dev_id, di, sizeof(DEV_ITEM), NULL, Irp, rollback)) {
        ERR("insert_tree_item failed\n");
        return STATUS_INTERNAL_ERROR;
    }
    
    return STATUS_SUCCESS;
}

static void regen_bootstrap(device_extension* Vcb) {
    sys_chunk* sc2;
    USHORT i = 0;
    LIST_ENTRY* le;
    
    i = 0;
    le = Vcb->sys_chunks.Flink;
    while (le != &Vcb->sys_chunks) {
        sc2 = CONTAINING_RECORD(le, sys_chunk, list_entry);
        
        TRACE("%llx,%x,%llx\n", sc2->key.obj_id, sc2->key.obj_type, sc2->key.offset);
        
        RtlCopyMemory(&Vcb->superblock.sys_chunk_array[i], &sc2->key, sizeof(KEY));
        i += sizeof(KEY);
        
        RtlCopyMemory(&Vcb->superblock.sys_chunk_array[i], sc2->data, sc2->size);
        i += sc2->size;
        
        le = le->Flink;
    }
}

static NTSTATUS add_to_bootstrap(device_extension* Vcb, UINT64 obj_id, UINT8 obj_type, UINT64 offset, void* data, ULONG size) {
    sys_chunk *sc, *sc2;
    LIST_ENTRY* le;
    
    if (Vcb->superblock.n + sizeof(KEY) + size > SYS_CHUNK_ARRAY_SIZE) {
        ERR("error - bootstrap is full\n");
        return STATUS_INTERNAL_ERROR;
    }
    
    sc = ExAllocatePoolWithTag(PagedPool, sizeof(sys_chunk), ALLOC_TAG);
    if (!sc) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    sc->key.obj_id = obj_id;
    sc->key.obj_type = obj_type;
    sc->key.offset = offset;
    sc->size = size;
    sc->data = ExAllocatePoolWithTag(PagedPool, sc->size, ALLOC_TAG);
    if (!sc->data) {
        ERR("out of memory\n");
        ExFreePool(sc);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlCopyMemory(sc->data, data, sc->size);
    
    le = Vcb->sys_chunks.Flink;
    while (le != &Vcb->sys_chunks) {
        sc2 = CONTAINING_RECORD(le, sys_chunk, list_entry);
        
        if (keycmp(&sc2->key, &sc->key) == 1)
            break;
        
        le = le->Flink;
    }
    InsertTailList(le, &sc->list_entry);
    
    Vcb->superblock.n += sizeof(KEY) + size;
    
    regen_bootstrap(Vcb);
    
    return STATUS_SUCCESS;
}

static BOOL find_new_dup_stripes(device_extension* Vcb, stripe* stripes, UINT64 max_stripe_size) {
    UINT64 j, devnum, devusage = 0xffffffffffffffff;
    space *devdh1 = NULL, *devdh2 = NULL;
    
    for (j = 0; j < Vcb->superblock.num_devices; j++) {
        UINT64 usage;
        
        usage = (Vcb->devices[j].devitem.bytes_used * 4096) / Vcb->devices[j].devitem.num_bytes;
        
        // favour devices which have been used the least
        if (usage < devusage) {
            if (!IsListEmpty(&Vcb->devices[j].space)) {
                LIST_ENTRY* le;
                space *dh1 = NULL, *dh2 = NULL;
                
                le = Vcb->devices[j].space.Flink;
                while (le != &Vcb->devices[j].space) {
                    space* dh = CONTAINING_RECORD(le, space, list_entry);
                    
                    if (dh->size >= max_stripe_size && (!dh1 || dh->size < dh1->size)) {
                        dh2 = dh1;
                        dh1 = dh;
                    }

                    le = le->Flink;
                }
                
                if (dh1 && (dh2 || dh1->size >= 2 * max_stripe_size)) {
                    devnum = j;
                    devusage = usage;
                    devdh1 = dh1;
                    devdh2 = dh2 ? dh2 : dh1;
                }
            }
        }
    }
    
    if (!devdh1)
        return FALSE;
    
    stripes[0].device = &Vcb->devices[devnum];
    stripes[0].dh = devdh1;
    stripes[1].device = stripes[0].device;
    stripes[1].dh = devdh2;
    
    return TRUE;
}

static BOOL find_new_stripe(device_extension* Vcb, stripe* stripes, UINT16 i, UINT64 max_stripe_size, UINT16 type) {
    UINT64 j, k, devnum = 0xffffffffffffffff, devusage = 0xffffffffffffffff;
    space* devdh = NULL;
    
    for (j = 0; j < Vcb->superblock.num_devices; j++) {
        UINT64 usage;
        BOOL skip = FALSE;
        
        // skip this device if it already has a stripe
        if (i > 0) {
            for (k = 0; k < i; k++) {
                if (stripes[k].device == &Vcb->devices[j]) {
                    skip = TRUE;
                    break;
                }
            }
        }
        
        if (!skip) {
            usage = (Vcb->devices[j].devitem.bytes_used * 4096) / Vcb->devices[j].devitem.num_bytes;
            
            // favour devices which have been used the least
            if (usage < devusage) {
                if (!IsListEmpty(&Vcb->devices[j].space)) {
                    LIST_ENTRY* le;
                    
                    le = Vcb->devices[j].space.Flink;
                    while (le != &Vcb->devices[j].space) {
                        space* dh = CONTAINING_RECORD(le, space, list_entry);
                        
                        if ((devnum != j && dh->size >= max_stripe_size) ||
                            (devnum == j && dh->size >= max_stripe_size && dh->size < devdh->size)
                        ) {
                            devdh = dh;
                            devnum = j;
                            devusage = usage;
                        }

                        le = le->Flink;
                    }
                }
            }
        }
    }
    
    if (!devdh)
        return FALSE;
    
    stripes[i].dh = devdh;
    stripes[i].device = &Vcb->devices[devnum];

    return TRUE;
}

chunk* alloc_chunk(device_extension* Vcb, UINT64 flags) {
    UINT64 max_stripe_size, max_chunk_size, stripe_size, stripe_length, factor;
    UINT64 total_size = 0, i, logaddr;
    UINT16 type, num_stripes, sub_stripes, max_stripes, min_stripes;
    stripe* stripes = NULL;
    ULONG cisize;
    CHUNK_ITEM_STRIPE* cis;
    chunk* c = NULL;
    space* s = NULL;
    BOOL success = FALSE;
    
    ExAcquireResourceExclusiveLite(&Vcb->chunk_lock, TRUE);
    
    for (i = 0; i < Vcb->superblock.num_devices; i++) {
        total_size += Vcb->devices[i].devitem.num_bytes;
    }
    TRACE("total_size = %llx\n", total_size);
    
    // We purposely check for DATA first - mixed blocks have the same size
    // as DATA ones.
    if (flags & BLOCK_FLAG_DATA) {
        max_stripe_size = 0x40000000; // 1 GB
        max_chunk_size = 10 * max_stripe_size;
    } else if (flags & BLOCK_FLAG_METADATA) {
        if (total_size > 0xC80000000) // 50 GB
            max_stripe_size = 0x40000000; // 1 GB
        else
            max_stripe_size = 0x10000000; // 256 MB
        
        max_chunk_size = max_stripe_size;
    } else if (flags & BLOCK_FLAG_SYSTEM) {
        max_stripe_size = 0x2000000; // 32 MB
        max_chunk_size = 2 * max_stripe_size;
    }
    
    max_chunk_size = min(max_chunk_size, total_size / 10); // cap at 10%
    
    TRACE("would allocate a new chunk of %llx bytes and stripe %llx\n", max_chunk_size, max_stripe_size);
    
    if (flags & BLOCK_FLAG_DUPLICATE) {
        min_stripes = 2;
        max_stripes = 2;
        sub_stripes = 0;
        type = BLOCK_FLAG_DUPLICATE;
    } else if (flags & BLOCK_FLAG_RAID0) {
        min_stripes = 2;
        max_stripes = Vcb->superblock.num_devices;
        sub_stripes = 0;
        type = BLOCK_FLAG_RAID0;
    } else if (flags & BLOCK_FLAG_RAID1) {
        min_stripes = 2;
        max_stripes = 2;
        sub_stripes = 1;
        type = BLOCK_FLAG_RAID1;
    } else if (flags & BLOCK_FLAG_RAID10) {
        min_stripes = 4;
        max_stripes = Vcb->superblock.num_devices;
        sub_stripes = 2;
        type = BLOCK_FLAG_RAID10;
    } else if (flags & BLOCK_FLAG_RAID5) {
        FIXME("RAID5 not yet supported\n");
        goto end;
    } else if (flags & BLOCK_FLAG_RAID6) {
        FIXME("RAID6 not yet supported\n");
        goto end;
    } else { // SINGLE
        min_stripes = 1;
        max_stripes = 1;
        sub_stripes = 1;
        type = 0;
    }
    
    stripes = ExAllocatePoolWithTag(PagedPool, sizeof(stripe) * max_stripes, ALLOC_TAG);
    if (!stripes) {
        ERR("out of memory\n");
        goto end;
    }
    
    num_stripes = 0;
    
    if (type == BLOCK_FLAG_DUPLICATE) {
        if (!find_new_dup_stripes(Vcb, stripes, max_stripe_size))
            goto end;
        else
            num_stripes = max_stripes;
    } else {
        for (i = 0; i < max_stripes; i++) {
            if (!find_new_stripe(Vcb, stripes, i, max_stripe_size, type))
                break;
            else
                num_stripes++;
        }
    }
    
    // for RAID10, round down to an even number of stripes
    if (type == BLOCK_FLAG_RAID10 && (num_stripes % sub_stripes) != 0) {
        num_stripes -= num_stripes % sub_stripes;
    }
    
    if (num_stripes < min_stripes) {
        WARN("found %u stripes, needed at least %u\n", num_stripes, min_stripes);
        goto end;
    }
    
    c = ExAllocatePoolWithTag(NonPagedPool, sizeof(chunk), ALLOC_TAG);
    if (!c) {
        ERR("out of memory\n");
        goto end;
    }
    
    cisize = sizeof(CHUNK_ITEM) + (num_stripes * sizeof(CHUNK_ITEM_STRIPE));
    c->chunk_item = ExAllocatePoolWithTag(NonPagedPool, cisize, ALLOC_TAG);
    if (!c->chunk_item) {
        ERR("out of memory\n");
        goto end;
    }
    
    stripe_length = 0x10000; // FIXME? BTRFS_STRIPE_LEN in kernel
    
    stripe_size = max_stripe_size;
    for (i = 0; i < num_stripes; i++) {
        if (stripes[i].dh->size < stripe_size)
            stripe_size = stripes[i].dh->size;
    }
    
    if (type == 0 || type == BLOCK_FLAG_DUPLICATE || type == BLOCK_FLAG_RAID1)
        factor = 1;
    else if (type == BLOCK_FLAG_RAID0)
        factor = num_stripes;
    else if (type == BLOCK_FLAG_RAID10)
        factor = num_stripes / sub_stripes;
    
    if (stripe_size * factor > max_chunk_size)
        stripe_size = max_chunk_size / factor;
    
    if (stripe_size % stripe_length > 0)
        stripe_size -= stripe_size % stripe_length;
    
    if (stripe_size == 0)
        goto end;
    
    c->chunk_item->size = stripe_size * factor;
    c->chunk_item->root_id = Vcb->extent_root->id;
    c->chunk_item->stripe_length = stripe_length;
    c->chunk_item->type = flags;
    c->chunk_item->opt_io_alignment = c->chunk_item->stripe_length;
    c->chunk_item->opt_io_width = c->chunk_item->stripe_length;
    c->chunk_item->sector_size = stripes[0].device->devitem.minimal_io_size;
    c->chunk_item->num_stripes = num_stripes;
    c->chunk_item->sub_stripes = sub_stripes;
    
    c->devices = ExAllocatePoolWithTag(NonPagedPool, sizeof(device*) * num_stripes, ALLOC_TAG);
    if (!c->devices) {
        ERR("out of memory\n");
        goto end;
    }

    cis = (CHUNK_ITEM_STRIPE*)&c->chunk_item[1];
    for (i = 0; i < num_stripes; i++) {
        cis[i].dev_id = stripes[i].device->devitem.dev_id;
        
        if (type == BLOCK_FLAG_DUPLICATE && i == 1 && stripes[i].dh == stripes[0].dh)
            cis[i].offset = stripes[0].dh->address + stripe_size;
        else
            cis[i].offset = stripes[i].dh->address;
        
        cis[i].dev_uuid = stripes[i].device->devitem.device_uuid;
        
        c->devices[i] = stripes[i].device;
    }
    
    logaddr = find_new_chunk_address(Vcb, c->chunk_item->size);
    
    Vcb->superblock.chunk_root_generation = Vcb->superblock.generation;
    
    c->size = cisize;
    c->offset = logaddr;
    c->used = c->oldused = 0;
    c->cache = NULL;
    InitializeListHead(&c->space);
    InitializeListHead(&c->space_size);
    InitializeListHead(&c->deleting);
    InitializeListHead(&c->changed_extents);
    
    ExInitializeResourceLite(&c->lock);
    ExInitializeResourceLite(&c->changed_extents_lock);
    
    s = ExAllocatePoolWithTag(NonPagedPool, sizeof(space), ALLOC_TAG);
    if (!s) {
        ERR("out of memory\n");
        goto end;
    }
    
    s->address = c->offset;
    s->size = c->chunk_item->size;
    InsertTailList(&c->space, &s->list_entry);
    InsertTailList(&c->space_size, &s->list_entry_size);
    
    protect_superblocks(Vcb, c);
    
    for (i = 0; i < num_stripes; i++) {
        stripes[i].device->devitem.bytes_used += stripe_size;
        
        space_list_subtract2(&stripes[i].device->space, NULL, cis[i].offset, stripe_size, NULL);
    }
    
    success = TRUE;
    
end:
    if (stripes)
        ExFreePool(stripes);
    
    if (!success) {
        if (c && c->chunk_item) ExFreePool(c->chunk_item);
        if (c) ExFreePool(c);
        if (s) ExFreePool(s);
    } else {
        LIST_ENTRY* le;
        BOOL done = FALSE;
        
        le = Vcb->chunks.Flink;
        while (le != &Vcb->chunks) {
            chunk* c2 = CONTAINING_RECORD(le, chunk, list_entry);
            
            if (c2->offset > c->offset) {
                InsertHeadList(le->Blink, &c->list_entry);
                done = TRUE;
                break;
            }
            
            le = le->Flink;
        }
        
        if (!done)
            InsertTailList(&Vcb->chunks, &c->list_entry);
        
        c->created = TRUE;
        InsertTailList(&Vcb->chunks_changed, &c->list_entry_changed);
    }
    
    ExReleaseResourceLite(&Vcb->chunk_lock);

    return success ? c : NULL;
}

NTSTATUS STDCALL write_data(device_extension* Vcb, UINT64 address, void* data, BOOL need_free, UINT32 length, write_data_context* wtc, PIRP Irp, chunk* c) {
    NTSTATUS Status;
    UINT32 i;
    CHUNK_ITEM_STRIPE* cis;
    write_data_stripe* stripe;
    UINT64 *stripestart = NULL, *stripeend = NULL;
    UINT8** stripedata = NULL;
    BOOL need_free2;
    
    TRACE("(%p, %llx, %p, %x)\n", Vcb, address, data, length);
    
    if (!c) {
        c = get_chunk_from_address(Vcb, address);
        if (!c) {
            ERR("could not get chunk for address %llx\n", address);
            return STATUS_INTERNAL_ERROR;
        }
    }
    
    if (c->chunk_item->type & BLOCK_FLAG_RAID5) {
        FIXME("RAID5 not yet supported\n");
        return STATUS_NOT_IMPLEMENTED;
    } else if (c->chunk_item->type & BLOCK_FLAG_RAID6) {
        FIXME("RAID6 not yet supported\n");
        return STATUS_NOT_IMPLEMENTED;
    }
    
    stripestart = ExAllocatePoolWithTag(PagedPool, sizeof(UINT64) * c->chunk_item->num_stripes, ALLOC_TAG);
    if (!stripestart) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    stripeend = ExAllocatePoolWithTag(PagedPool, sizeof(UINT64) * c->chunk_item->num_stripes, ALLOC_TAG);
    if (!stripeend) {
        ERR("out of memory\n");
        ExFreePool(stripestart);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    stripedata = ExAllocatePoolWithTag(PagedPool, sizeof(UINT8*) * c->chunk_item->num_stripes, ALLOC_TAG);
    if (!stripedata) {
        ERR("out of memory\n");
        ExFreePool(stripeend);
        ExFreePool(stripestart);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(stripedata, sizeof(UINT8*) * c->chunk_item->num_stripes);
    
    cis = (CHUNK_ITEM_STRIPE*)&c->chunk_item[1];
    
    if (c->chunk_item->type & BLOCK_FLAG_RAID0) {
        UINT64 startoff, endoff;
        UINT16 startoffstripe, endoffstripe, stripenum;
        UINT64 pos, *stripeoff;
        
        stripeoff = ExAllocatePoolWithTag(PagedPool, sizeof(UINT64) * c->chunk_item->num_stripes, ALLOC_TAG);
        if (!stripeoff) {
            ERR("out of memory\n");
            ExFreePool(stripedata);
            ExFreePool(stripeend);
            ExFreePool(stripestart);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        get_raid0_offset(address - c->offset, c->chunk_item->stripe_length, c->chunk_item->num_stripes, &startoff, &startoffstripe);
        get_raid0_offset(address + length - c->offset - 1, c->chunk_item->stripe_length, c->chunk_item->num_stripes, &endoff, &endoffstripe);
        
        for (i = 0; i < c->chunk_item->num_stripes; i++) {
            if (startoffstripe > i) {
                stripestart[i] = startoff - (startoff % c->chunk_item->stripe_length) + c->chunk_item->stripe_length;
            } else if (startoffstripe == i) {
                stripestart[i] = startoff;
            } else {
                stripestart[i] = startoff - (startoff % c->chunk_item->stripe_length);
            }
            
            if (endoffstripe > i) {
                stripeend[i] = endoff - (endoff % c->chunk_item->stripe_length) + c->chunk_item->stripe_length;
            } else if (endoffstripe == i) {
                stripeend[i] = endoff + 1;
            } else {
                stripeend[i] = endoff - (endoff % c->chunk_item->stripe_length);
            }
            
            if (stripestart[i] != stripeend[i]) {
                stripedata[i] = ExAllocatePoolWithTag(NonPagedPool, stripeend[i] - stripestart[i], ALLOC_TAG);
                
                if (!stripedata[i]) {
                    ERR("out of memory\n");
                    ExFreePool(stripeoff);
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto end;
                }
            }
        }
        
        pos = 0;
        RtlZeroMemory(stripeoff, sizeof(UINT64) * c->chunk_item->num_stripes);
        
        stripenum = startoffstripe;
        while (pos < length) {
            if (pos == 0) {
                UINT32 writelen = min(stripeend[stripenum] - stripestart[stripenum],
                                      c->chunk_item->stripe_length - (stripestart[stripenum] % c->chunk_item->stripe_length));
                
                RtlCopyMemory(stripedata[stripenum], data, writelen);
                stripeoff[stripenum] += writelen;
                pos += writelen;
            } else if (length - pos < c->chunk_item->stripe_length) {
                RtlCopyMemory(stripedata[stripenum] + stripeoff[stripenum], (UINT8*)data + pos, length - pos);
                break;
            } else {
                RtlCopyMemory(stripedata[stripenum] + stripeoff[stripenum], (UINT8*)data + pos, c->chunk_item->stripe_length);
                stripeoff[stripenum] += c->chunk_item->stripe_length;
                pos += c->chunk_item->stripe_length;
            }
            
            stripenum = (stripenum + 1) % c->chunk_item->num_stripes;
        }

        ExFreePool(stripeoff);
        
        if (need_free)
            ExFreePool(data);

        need_free2 = TRUE;
    } else if (c->chunk_item->type & BLOCK_FLAG_RAID10) {
        UINT64 startoff, endoff;
        UINT16 startoffstripe, endoffstripe, stripenum;
        UINT64 pos, *stripeoff;
        
        stripeoff = ExAllocatePoolWithTag(PagedPool, sizeof(UINT64) * c->chunk_item->num_stripes / c->chunk_item->sub_stripes, ALLOC_TAG);
        if (!stripeoff) {
            ERR("out of memory\n");
            ExFreePool(stripedata);
            ExFreePool(stripeend);
            ExFreePool(stripestart);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        get_raid0_offset(address - c->offset, c->chunk_item->stripe_length, c->chunk_item->num_stripes / c->chunk_item->sub_stripes, &startoff, &startoffstripe);
        get_raid0_offset(address + length - c->offset - 1, c->chunk_item->stripe_length, c->chunk_item->num_stripes / c->chunk_item->sub_stripes, &endoff, &endoffstripe);
        
        startoffstripe *= c->chunk_item->sub_stripes;
        endoffstripe *= c->chunk_item->sub_stripes;
        
        for (i = 0; i < c->chunk_item->num_stripes; i += c->chunk_item->sub_stripes) {
            UINT16 j;
            
            if (startoffstripe > i) {
                stripestart[i] = startoff - (startoff % c->chunk_item->stripe_length) + c->chunk_item->stripe_length;
            } else if (startoffstripe == i) {
                stripestart[i] = startoff;
            } else {
                stripestart[i] = startoff - (startoff % c->chunk_item->stripe_length);
            }
            
            if (endoffstripe > i) {
                stripeend[i] = endoff - (endoff % c->chunk_item->stripe_length) + c->chunk_item->stripe_length;
            } else if (endoffstripe == i) {
                stripeend[i] = endoff + 1;
            } else {
                stripeend[i] = endoff - (endoff % c->chunk_item->stripe_length);
            }
            
            if (stripestart[i] != stripeend[i]) {
                stripedata[i] = ExAllocatePoolWithTag(NonPagedPool, stripeend[i] - stripestart[i], ALLOC_TAG);
                
                if (!stripedata[i]) {
                    ERR("out of memory\n");
                    ExFreePool(stripeoff);
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto end;
                }
            }
            
            for (j = 1; j < c->chunk_item->sub_stripes; j++) {
                stripestart[i+j] = stripestart[i];
                stripeend[i+j] = stripeend[i];
                stripedata[i+j] = stripedata[i];
            }
        }
        
        pos = 0;
        RtlZeroMemory(stripeoff, sizeof(UINT64) * c->chunk_item->num_stripes / c->chunk_item->sub_stripes);
        
        stripenum = startoffstripe / c->chunk_item->sub_stripes;
        while (pos < length) {
            if (pos == 0) {
                UINT32 writelen = min(stripeend[stripenum * c->chunk_item->sub_stripes] - stripestart[stripenum * c->chunk_item->sub_stripes],
                                      c->chunk_item->stripe_length - (stripestart[stripenum * c->chunk_item->sub_stripes] % c->chunk_item->stripe_length));
                
                RtlCopyMemory(stripedata[stripenum * c->chunk_item->sub_stripes], data, writelen);
                stripeoff[stripenum] += writelen;
                pos += writelen;
            } else if (length - pos < c->chunk_item->stripe_length) {
                RtlCopyMemory(stripedata[stripenum * c->chunk_item->sub_stripes] + stripeoff[stripenum], (UINT8*)data + pos, length - pos);
                break;
            } else {
                RtlCopyMemory(stripedata[stripenum * c->chunk_item->sub_stripes] + stripeoff[stripenum], (UINT8*)data + pos, c->chunk_item->stripe_length);
                stripeoff[stripenum] += c->chunk_item->stripe_length;
                pos += c->chunk_item->stripe_length;
            }
            
            stripenum = (stripenum + 1) % (c->chunk_item->num_stripes / c->chunk_item->sub_stripes);
        }

        ExFreePool(stripeoff);
        
        if (need_free)
            ExFreePool(data);

        need_free2 = TRUE;
    } else {
        for (i = 0; i < c->chunk_item->num_stripes; i++) {
            stripestart[i] = address - c->offset;
            stripeend[i] = stripestart[i] + length;
            stripedata[i] = data;
        }
        need_free2 = need_free;
    }

    for (i = 0; i < c->chunk_item->num_stripes; i++) {
        PIO_STACK_LOCATION IrpSp;
        
        // FIXME - handle missing devices
        
        stripe = ExAllocatePoolWithTag(NonPagedPool, sizeof(write_data_stripe), ALLOC_TAG);
        if (!stripe) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }
        
        if (stripestart[i] == stripeend[i]) {
            stripe->status = WriteDataStatus_Ignore;
            stripe->Irp = NULL;
            stripe->buf = NULL;
        } else {
            stripe->context = (struct _write_data_context*)wtc;
            stripe->buf = stripedata[i];
            stripe->need_free = need_free2;
            stripe->device = c->devices[i];
            RtlZeroMemory(&stripe->iosb, sizeof(IO_STATUS_BLOCK));
            stripe->status = WriteDataStatus_Pending;
            
            if (!Irp) {
                stripe->Irp = IoAllocateIrp(stripe->device->devobj->StackSize, FALSE);
            
                if (!stripe->Irp) {
                    ERR("IoAllocateIrp failed\n");
                    Status = STATUS_INTERNAL_ERROR;
                    goto end;
                }
            } else {
                stripe->Irp = IoMakeAssociatedIrp(Irp, stripe->device->devobj->StackSize);
                
                if (!stripe->Irp) {
                    ERR("IoMakeAssociatedIrp failed\n");
                    Status = STATUS_INTERNAL_ERROR;
                    goto end;
                }
            }
            
            IrpSp = IoGetNextIrpStackLocation(stripe->Irp);
            IrpSp->MajorFunction = IRP_MJ_WRITE;
            
            if (stripe->device->devobj->Flags & DO_BUFFERED_IO) {
                stripe->Irp->AssociatedIrp.SystemBuffer = stripedata[i];

                stripe->Irp->Flags = IRP_BUFFERED_IO;
            } else if (stripe->device->devobj->Flags & DO_DIRECT_IO) {
                stripe->Irp->MdlAddress = IoAllocateMdl(stripedata[i], stripeend[i] - stripestart[i], FALSE, FALSE, NULL);
                if (!stripe->Irp->MdlAddress) {
                    ERR("IoAllocateMdl failed\n");
                    Status = STATUS_INTERNAL_ERROR;
                    goto end;
                }
                
                MmProbeAndLockPages(stripe->Irp->MdlAddress, KernelMode, IoWriteAccess);
            } else {
                stripe->Irp->UserBuffer = stripedata[i];
            }

            IrpSp->Parameters.Write.Length = stripeend[i] - stripestart[i];
            IrpSp->Parameters.Write.ByteOffset.QuadPart = stripestart[i] + cis[i].offset;
            
            stripe->Irp->UserIosb = &stripe->iosb;
            wtc->stripes_left++;

            IoSetCompletionRoutine(stripe->Irp, write_data_completion, stripe, TRUE, TRUE, TRUE);
        }

        InsertTailList(&wtc->stripes, &stripe->list_entry);
    }
    
    Status = STATUS_SUCCESS;
    
end:

    if (stripestart) ExFreePool(stripestart);
    if (stripeend) ExFreePool(stripeend);
    if (stripedata) ExFreePool(stripedata);
    
    if (!NT_SUCCESS(Status)) {
        free_write_data_stripes(wtc);
        ExFreePool(wtc);
    }
    
    return Status;
}

NTSTATUS STDCALL write_data_complete(device_extension* Vcb, UINT64 address, void* data, UINT32 length, PIRP Irp, chunk* c) {
    write_data_context* wtc;
    NTSTATUS Status;
    
    wtc = ExAllocatePoolWithTag(NonPagedPool, sizeof(write_data_context), ALLOC_TAG);
    if (!wtc) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KeInitializeEvent(&wtc->Event, NotificationEvent, FALSE);
    InitializeListHead(&wtc->stripes);
    wtc->tree = FALSE;
    wtc->stripes_left = 0;
    
    Status = write_data(Vcb, address, data, FALSE, length, wtc, Irp, c);
    if (!NT_SUCCESS(Status)) {
        ERR("write_data returned %08x\n", Status);
        free_write_data_stripes(wtc);
        ExFreePool(wtc);
        return Status;
    }
    
    if (wtc->stripes.Flink != &wtc->stripes) {
        // launch writes and wait
        LIST_ENTRY* le = wtc->stripes.Flink;
        while (le != &wtc->stripes) {
            write_data_stripe* stripe = CONTAINING_RECORD(le, write_data_stripe, list_entry);
            
            if (stripe->status != WriteDataStatus_Ignore)
                IoCallDriver(stripe->device->devobj, stripe->Irp);
            
            le = le->Flink;
        }
        
        KeWaitForSingleObject(&wtc->Event, Executive, KernelMode, FALSE, NULL);
        
        le = wtc->stripes.Flink;
        while (le != &wtc->stripes) {
            write_data_stripe* stripe = CONTAINING_RECORD(le, write_data_stripe, list_entry);
            
            if (stripe->status != WriteDataStatus_Ignore && !NT_SUCCESS(stripe->iosb.Status)) {
                Status = stripe->iosb.Status;
                break;
            }
            
            le = le->Flink;
        }
        
        free_write_data_stripes(wtc);
    }

    ExFreePool(wtc);

    return STATUS_SUCCESS;
}

static void clean_space_cache_chunk(device_extension* Vcb, chunk* c) {
    // FIXME - loop through c->deleting and do TRIM if device supports it
    // FIXME - also find way of doing TRIM of dropped chunks
    
    while (!IsListEmpty(&c->deleting)) {
        space* s = CONTAINING_RECORD(c->deleting.Flink, space, list_entry);
        
        RemoveEntryList(&s->list_entry);
        ExFreePool(s);
    }
}

static void clean_space_cache(device_extension* Vcb) {
    chunk* c;
    
    TRACE("(%p)\n", Vcb);
    
    while (!IsListEmpty(&Vcb->chunks_changed)) {
        c = CONTAINING_RECORD(Vcb->chunks_changed.Flink, chunk, list_entry_changed);
        
        ExAcquireResourceExclusiveLite(&c->lock, TRUE);
        
        clean_space_cache_chunk(Vcb, c);
        RemoveEntryList(&c->list_entry_changed);
        c->list_entry_changed.Flink = NULL;
        
        ExReleaseResourceLite(&c->lock);
    }
}

static BOOL trees_consistent(device_extension* Vcb, LIST_ENTRY* rollback) {
    ULONG maxsize = Vcb->superblock.node_size - sizeof(tree_header);
    LIST_ENTRY* le;
    
    le = Vcb->trees.Flink;
    while (le != &Vcb->trees) {
        tree* t = CONTAINING_RECORD(le, tree, list_entry);
        
        if (t->write) {
            if (t->header.num_items == 0 && t->parent) {
#ifdef DEBUG_WRITE_LOOPS
                ERR("empty tree found, looping again\n");
#endif
                return FALSE;
            }
            
            if (t->size > maxsize) {
#ifdef DEBUG_WRITE_LOOPS
                ERR("overlarge tree found (%u > %u), looping again\n", t->size, maxsize);
#endif
                return FALSE;
            }
            
            if (!t->has_new_address) {
#ifdef DEBUG_WRITE_LOOPS
                ERR("tree found without new address, looping again\n");
#endif
                return FALSE;
            }
        }
        
        le = le->Flink;
    }
    
    return TRUE;
}

static NTSTATUS add_parents(device_extension* Vcb, LIST_ENTRY* rollback) {
    UINT8 level;
    LIST_ENTRY* le;
    
    for (level = 0; level <= 255; level++) {
        BOOL nothing_found = TRUE;
        
        TRACE("level = %u\n", level);
        
        le = Vcb->trees.Flink;
        while (le != &Vcb->trees) {
            tree* t = CONTAINING_RECORD(le, tree, list_entry);
            
            if (t->write && t->header.level == level) {
                TRACE("tree %p: root = %llx, level = %x, parent = %p\n", t, t->header.tree_id, t->header.level, t->parent);
                
                nothing_found = FALSE;
                
                if (t->parent) {
                    if (!t->parent->write)
                        TRACE("adding tree %p (level %x)\n", t->parent, t->header.level);
                        
                    t->parent->write = TRUE;
                }
            }
            
            le = le->Flink;
        }
        
        if (nothing_found)
            break;
    }

    return STATUS_SUCCESS;
}

static void add_parents_to_cache(device_extension* Vcb, tree* t) {
    while (t->parent) {
        t = t->parent;
        t->write = TRUE;
    }
}

static BOOL insert_tree_extent_skinny(device_extension* Vcb, UINT8 level, UINT64 root_id, chunk* c, UINT64 address, PIRP Irp, LIST_ENTRY* rollback) {
    EXTENT_ITEM_SKINNY_METADATA* eism;
    traverse_ptr insert_tp;
    
    eism = ExAllocatePoolWithTag(PagedPool, sizeof(EXTENT_ITEM_SKINNY_METADATA), ALLOC_TAG);
    if (!eism) {
        ERR("out of memory\n");
        return FALSE;
    }
    
    eism->ei.refcount = 1;
    eism->ei.generation = Vcb->superblock.generation;
    eism->ei.flags = EXTENT_ITEM_TREE_BLOCK;
    eism->type = TYPE_TREE_BLOCK_REF;
    eism->tbr.offset = root_id;
    
    if (!insert_tree_item(Vcb, Vcb->extent_root, address, TYPE_METADATA_ITEM, level, eism, sizeof(EXTENT_ITEM_SKINNY_METADATA), &insert_tp, Irp, rollback)) {
        ERR("insert_tree_item failed\n");
        ExFreePool(eism);
        return FALSE;
    }
    
    ExAcquireResourceExclusiveLite(&c->lock, TRUE);
    
    space_list_subtract(Vcb, c, FALSE, address, Vcb->superblock.node_size, rollback);

    ExReleaseResourceLite(&c->lock);
    
    add_parents_to_cache(Vcb, insert_tp.tree);
    
    return TRUE;
}

static BOOL insert_tree_extent(device_extension* Vcb, UINT8 level, UINT64 root_id, chunk* c, UINT64* new_address, PIRP Irp, LIST_ENTRY* rollback) {
    UINT64 address;
    EXTENT_ITEM_TREE2* eit2;
    traverse_ptr insert_tp;
    
    TRACE("(%p, %x, %llx, %p, %p, %p, %p)\n", Vcb, level, root_id, c, new_address, rollback);
    
    if (!find_address_in_chunk(Vcb, c, Vcb->superblock.node_size, &address))
        return FALSE;
    
    if (Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA) {
        BOOL b = insert_tree_extent_skinny(Vcb, level, root_id, c, address, Irp, rollback);
        
        if (b)
            *new_address = address;
        
        return b;
    }
    
    eit2 = ExAllocatePoolWithTag(PagedPool, sizeof(EXTENT_ITEM_TREE2), ALLOC_TAG);
    if (!eit2) {
        ERR("out of memory\n");
        return FALSE;
    }

    eit2->eit.extent_item.refcount = 1;
    eit2->eit.extent_item.generation = Vcb->superblock.generation;
    eit2->eit.extent_item.flags = EXTENT_ITEM_TREE_BLOCK;
//     eit2->eit.firstitem = wt->firstitem;
    eit2->eit.level = level;
    eit2->type = TYPE_TREE_BLOCK_REF;
    eit2->tbr.offset = root_id;
    
// #ifdef DEBUG_PARANOID
//     if (wt->firstitem.obj_type == 0xcc) { // TESTING
//         ERR("error - firstitem not set (wt = %p, tree = %p, address = %x)\n", wt, wt->tree, (UINT32)address);
//         ERR("num_items = %u, level = %u, root = %x, delete = %u\n", wt->tree->header.num_items, wt->tree->header.level, (UINT32)wt->tree->root->id, wt->delete);
//         int3;
//     }
// #endif
    
    if (!insert_tree_item(Vcb, Vcb->extent_root, address, TYPE_EXTENT_ITEM, Vcb->superblock.node_size, eit2, sizeof(EXTENT_ITEM_TREE2), &insert_tp, Irp, rollback)) {
        ERR("insert_tree_item failed\n");
        ExFreePool(eit2);
        return FALSE;
    }
    
    ExAcquireResourceExclusiveLite(&c->lock, TRUE);
    
    space_list_subtract(Vcb, c, FALSE, address, Vcb->superblock.node_size, rollback);
    
    ExReleaseResourceLite(&c->lock);

    add_parents_to_cache(Vcb, insert_tp.tree);
    
    *new_address = address;
    
    return TRUE;
}

NTSTATUS get_tree_new_address(device_extension* Vcb, tree* t, PIRP Irp, LIST_ENTRY* rollback) {
    chunk *origchunk = NULL, *c;
    LIST_ENTRY* le;
    UINT64 flags = t->flags, addr;
    
    if (flags == 0) {
        if (t->root->id == BTRFS_ROOT_CHUNK)
            flags = BLOCK_FLAG_SYSTEM | BLOCK_FLAG_DUPLICATE;
        else if (Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_MIXED_GROUPS)
            flags = BLOCK_FLAG_DATA | BLOCK_FLAG_METADATA;
        else
            flags = BLOCK_FLAG_METADATA | BLOCK_FLAG_DUPLICATE;
    }
    
//     TRACE("flags = %x\n", (UINT32)wt->flags);
    
//     if (!chunk_test) { // TESTING
//         if ((c = alloc_chunk(Vcb, flags))) {
//             if ((c->chunk_item->size - c->used) >= Vcb->superblock.node_size) {
//                 if (insert_tree_extent(Vcb, t, c)) {
//                     chunk_test = TRUE;
//                     return STATUS_SUCCESS;
//                 }
//             }
//         }
//     }
    
    if (t->has_address) {
        origchunk = get_chunk_from_address(Vcb, t->header.address);
        
        if (insert_tree_extent(Vcb, t->header.level, t->header.tree_id, origchunk, &addr, Irp, rollback)) {
            t->new_address = addr;
            t->has_new_address = TRUE;
            return STATUS_SUCCESS;
        }
    }
    
    ExAcquireResourceExclusiveLite(&Vcb->chunk_lock, TRUE);
    
    le = Vcb->chunks.Flink;
    while (le != &Vcb->chunks) {
        c = CONTAINING_RECORD(le, chunk, list_entry);
        
        ExAcquireResourceExclusiveLite(&c->lock, TRUE);
        
        if (c != origchunk && c->chunk_item->type == flags && (c->chunk_item->size - c->used) >= Vcb->superblock.node_size) {
            if (insert_tree_extent(Vcb, t->header.level, t->header.tree_id, c, &addr, Irp, rollback)) {
                ExReleaseResourceLite(&c->lock);
                ExReleaseResourceLite(&Vcb->chunk_lock);
                t->new_address = addr;
                t->has_new_address = TRUE;
                return STATUS_SUCCESS;
            }
        }
        
        ExReleaseResourceLite(&c->lock);

        le = le->Flink;
    }
    
    // allocate new chunk if necessary
    if ((c = alloc_chunk(Vcb, flags))) {
        ExAcquireResourceExclusiveLite(&c->lock, TRUE);
        
        if ((c->chunk_item->size - c->used) >= Vcb->superblock.node_size) {
            if (insert_tree_extent(Vcb, t->header.level, t->header.tree_id, c, &addr, Irp, rollback)) {
                ExReleaseResourceLite(&c->lock);
                ExReleaseResourceLite(&Vcb->chunk_lock);
                t->new_address = addr;
                t->has_new_address = TRUE;
                return STATUS_SUCCESS;
            }
        }
        
        ExReleaseResourceLite(&c->lock);
    }
    
    ExReleaseResourceLite(&Vcb->chunk_lock);
    
    ERR("couldn't find any metadata chunks with %x bytes free\n", Vcb->superblock.node_size);

    return STATUS_DISK_FULL;
}

static BOOL reduce_tree_extent_skinny(device_extension* Vcb, UINT64 address, tree* t, PIRP Irp, LIST_ENTRY* rollback) {
    KEY searchkey;
    traverse_ptr tp;
    chunk* c;
    NTSTATUS Status;
    
    searchkey.obj_id = address;
    searchkey.obj_type = TYPE_METADATA_ITEM;
    searchkey.offset = 0xffffffffffffffff;
    
    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return FALSE;
    }
    
    if (tp.item->key.obj_id != searchkey.obj_id || tp.item->key.obj_type != searchkey.obj_type) {
        TRACE("could not find %llx,%x,%llx in extent_root\n", searchkey.obj_id, searchkey.obj_type, searchkey.offset);
        return FALSE;
    }
    
    if (tp.item->size < sizeof(EXTENT_ITEM_SKINNY_METADATA)) {
        ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_ITEM_SKINNY_METADATA));
        return FALSE;
    }
    
    delete_tree_item(Vcb, &tp, rollback);

    c = get_chunk_from_address(Vcb, address);
    
    if (c) {
        ExAcquireResourceExclusiveLite(&c->lock, TRUE);
        
        decrease_chunk_usage(c, Vcb->superblock.node_size);
        
        space_list_add(Vcb, c, TRUE, address, Vcb->superblock.node_size, rollback);
        
        ExReleaseResourceLite(&c->lock);
    } else
        ERR("could not find chunk for address %llx\n", address);
    
    return TRUE;
}

// TESTING
// static void check_tree_num_items(tree* t) {
//     LIST_ENTRY* le2;
//     UINT32 ni;
//     
//     le2 = t->itemlist.Flink;
//     ni = 0;
//     while (le2 != &t->itemlist) {
//         tree_data* td = CONTAINING_RECORD(le2, tree_data, list_entry);
//         if (!td->ignore)
//             ni++;
//         le2 = le2->Flink;
//     }
//     
//     if (t->header.num_items != ni) {
//         ERR("tree %p not okay: num_items was %x, expecting %x\n", t, ni, t->header.num_items);
//         int3;
//     } else {
//         ERR("tree %p okay\n", t);
//     }
// }
// 
// static void check_trees_num_items(LIST_ENTRY* tc) {
//     LIST_ENTRY* le = tc->Flink;
//     while (le != tc) {
//         tree_cache* tc2 = CONTAINING_RECORD(le, tree_cache, list_entry);
//         
//         check_tree_num_items(tc2->tree);
//         
//         le = le->Flink;
//     }    
// }

static void convert_old_tree_extent(device_extension* Vcb, tree_data* td, tree* t, PIRP Irp, LIST_ENTRY* rollback) {
    KEY searchkey;
    traverse_ptr tp, tp2, insert_tp;
    EXTENT_REF_V0* erv0;
    NTSTATUS Status;
    
    TRACE("(%p, %p, %p)\n", Vcb, td, t);
    
    searchkey.obj_id = td->treeholder.address;
    searchkey.obj_type = TYPE_EXTENT_REF_V0;
    searchkey.offset = 0xffffffffffffffff;
    
    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return;
    }
    
    if (tp.item->key.obj_id != searchkey.obj_id || tp.item->key.obj_type != searchkey.obj_type) {
        TRACE("could not find EXTENT_REF_V0 for %llx\n", searchkey.obj_id);
        return;
    }
    
    searchkey.obj_id = td->treeholder.address;
    searchkey.obj_type = TYPE_EXTENT_ITEM;
    searchkey.offset = Vcb->superblock.node_size;
    
    Status = find_item(Vcb, Vcb->extent_root, &tp2, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return;
    }
    
    if (keycmp(&searchkey, &tp2.item->key)) {
        ERR("could not find %llx,%x,%llx\n", searchkey.obj_id, searchkey.obj_type, searchkey.offset);
        return;
    }
    
    if (tp.item->size < sizeof(EXTENT_REF_V0)) {
        ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_REF_V0));
        return;
    }
    
    erv0 = (EXTENT_REF_V0*)tp.item->data;
    
    delete_tree_item(Vcb, &tp, rollback);
    delete_tree_item(Vcb, &tp2, rollback);
    
    if (Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA) {
        EXTENT_ITEM_SKINNY_METADATA* eism = ExAllocatePoolWithTag(PagedPool, sizeof(EXTENT_ITEM_SKINNY_METADATA), ALLOC_TAG);
        
        if (!eism) {
            ERR("out of memory\n");
            return;
        }
        
        eism->ei.refcount = 1;
        eism->ei.generation = erv0->gen;
        eism->ei.flags = EXTENT_ITEM_TREE_BLOCK;
        eism->type = TYPE_TREE_BLOCK_REF;
        eism->tbr.offset = t->header.tree_id;
        
        if (!insert_tree_item(Vcb, Vcb->extent_root, td->treeholder.address, TYPE_METADATA_ITEM, t->header.level -1, eism, sizeof(EXTENT_ITEM_SKINNY_METADATA), &insert_tp, Irp, rollback)) {
            ERR("insert_tree_item failed\n");
            return;
        }
    } else {
        EXTENT_ITEM_TREE2* eit2 = ExAllocatePoolWithTag(PagedPool, sizeof(EXTENT_ITEM_TREE2), ALLOC_TAG);
        
        if (!eit2) {
            ERR("out of memory\n");
            return;
        }
        
        eit2->eit.extent_item.refcount = 1;
        eit2->eit.extent_item.generation = erv0->gen;
        eit2->eit.extent_item.flags = EXTENT_ITEM_TREE_BLOCK;
        eit2->eit.firstitem = td->key;
        eit2->eit.level = t->header.level - 1;
        eit2->type = TYPE_TREE_BLOCK_REF;
        eit2->tbr.offset = t->header.tree_id;

        if (!insert_tree_item(Vcb, Vcb->extent_root, td->treeholder.address, TYPE_EXTENT_ITEM, Vcb->superblock.node_size, eit2, sizeof(EXTENT_ITEM_TREE2), &insert_tp, Irp, rollback)) {
            ERR("insert_tree_item failed\n");
            return;
        }
    }
    
    add_parents_to_cache(Vcb, insert_tp.tree);
    add_parents_to_cache(Vcb, tp.tree);
    add_parents_to_cache(Vcb, tp2.tree);
}

static NTSTATUS reduce_tree_extent(device_extension* Vcb, UINT64 address, tree* t, PIRP Irp, LIST_ENTRY* rollback) {
    KEY searchkey;
    traverse_ptr tp;
    EXTENT_ITEM* ei;
    EXTENT_ITEM_V0* eiv0;
    chunk* c;
    NTSTATUS Status;
    
    // FIXME - deal with refcounts > 1
    
    TRACE("(%p, %llx, %p)\n", Vcb, address, t);
    
    if (Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA) {
        if (reduce_tree_extent_skinny(Vcb, address, t, Irp, rollback)) {
            return STATUS_SUCCESS;
        }
    }
    
    searchkey.obj_id = address;
    searchkey.obj_type = TYPE_EXTENT_ITEM;
    searchkey.offset = Vcb->superblock.node_size;
    
    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    if (keycmp(&tp.item->key, &searchkey)) {
        ERR("could not find %llx,%x,%llx in extent_root\n", searchkey.obj_id, searchkey.obj_type, searchkey.offset);
        int3;
        return STATUS_INTERNAL_ERROR;
    }
    
    if (tp.item->size == sizeof(EXTENT_ITEM_V0)) {
        eiv0 = (EXTENT_ITEM_V0*)tp.item->data;
        
        if (eiv0->refcount > 1) {
            FIXME("FIXME - cannot deal with refcounts larger than 1 at present (eiv0->refcount == %llx)\n", eiv0->refcount);
            return STATUS_INTERNAL_ERROR;
        }
    } else {
        if (tp.item->size < sizeof(EXTENT_ITEM)) {
            ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_ITEM));
            return STATUS_INTERNAL_ERROR;
        }
        
        ei = (EXTENT_ITEM*)tp.item->data;
        
        if (ei->refcount > 1) {
            FIXME("FIXME - cannot deal with refcounts larger than 1 at present (ei->refcount == %llx)\n", ei->refcount);
            return STATUS_INTERNAL_ERROR;
        }
    }
    
    delete_tree_item(Vcb, &tp, rollback);
    
    // if EXTENT_ITEM_V0, delete corresponding B4 item
    if (tp.item->size == sizeof(EXTENT_ITEM_V0)) {
        traverse_ptr tp2;
        
        searchkey.obj_id = address;
        searchkey.obj_type = TYPE_EXTENT_REF_V0;
        searchkey.offset = 0xffffffffffffffff;
        
        Status = find_item(Vcb, Vcb->extent_root, &tp2, &searchkey, FALSE, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            return Status;
        }
        
        if (tp2.item->key.obj_id == searchkey.obj_id && tp2.item->key.obj_type == searchkey.obj_type) {
            delete_tree_item(Vcb, &tp2, rollback);
        }
    }
     
    if (t && !(t->header.flags & HEADER_FLAG_MIXED_BACKREF)) {
        LIST_ENTRY* le;
        
        // when writing old internal trees, convert related extents
        
        le = t->itemlist.Flink;
        while (le != &t->itemlist) {
            tree_data* td = CONTAINING_RECORD(le, tree_data, list_entry);
            
//             ERR("%llx,%x,%llx\n", td->key.obj_id, td->key.obj_type, td->key.offset);
            
            if (!td->ignore && !td->inserted) {
                if (t->header.level > 0) {
                    convert_old_tree_extent(Vcb, td, t, Irp, rollback);
                } else if (td->key.obj_type == TYPE_EXTENT_DATA && td->size >= sizeof(EXTENT_DATA)) {
                    EXTENT_DATA* ed = (EXTENT_DATA*)td->data;
                    
                    if ((ed->type == EXTENT_TYPE_REGULAR || ed->type == EXTENT_TYPE_PREALLOC) && td->size >= sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2)) {
                        EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ed->data;
                        
                        if (ed2->address != 0) {
                            TRACE("trying to convert old data extent %llx,%llx\n", ed2->address, ed2->size);
                            convert_old_data_extent(Vcb, ed2->address, ed2->size, Irp, rollback);
                        }
                    }
                }
            }

            le = le->Flink;
        }
    }

    c = get_chunk_from_address(Vcb, address);
    
    if (c) {
        ExAcquireResourceExclusiveLite(&c->lock, TRUE);
        
        decrease_chunk_usage(c, tp.item->key.offset);
        
        space_list_add(Vcb, c, TRUE, address, tp.item->key.offset, rollback);
        
        ExReleaseResourceLite(&c->lock);
    } else
        ERR("could not find chunk for address %llx\n", address);
    
    return STATUS_SUCCESS;
}

static NTSTATUS allocate_tree_extents(device_extension* Vcb, PIRP Irp, LIST_ENTRY* rollback) {
    LIST_ENTRY* le;
    NTSTATUS Status;
    
    TRACE("(%p)\n", Vcb);
    
    le = Vcb->trees.Flink;
    while (le != &Vcb->trees) {
        tree* t = CONTAINING_RECORD(le, tree, list_entry);
        
        if (t->write && !t->has_new_address) {
            chunk* c;
            
            Status = get_tree_new_address(Vcb, t, Irp, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("get_tree_new_address returned %08x\n", Status);
                return Status;
            }
            
            TRACE("allocated extent %llx\n", t->new_address);
            
            if (t->has_address) {
                Status = reduce_tree_extent(Vcb, t->header.address, t, Irp, rollback);
                
                if (!NT_SUCCESS(Status)) {
                    ERR("reduce_tree_extent returned %08x\n", Status);
                    return Status;
                }
            }

            c = get_chunk_from_address(Vcb, t->new_address);
            
            if (c) {
                increase_chunk_usage(c, Vcb->superblock.node_size);
            } else {
                ERR("could not find chunk for address %llx\n", t->new_address);
                return STATUS_INTERNAL_ERROR;
            }
        }
        
        le = le->Flink;
    }
    
    return STATUS_SUCCESS;
}

static NTSTATUS update_root_root(device_extension* Vcb, PIRP Irp, LIST_ENTRY* rollback) {
    LIST_ENTRY* le;
    NTSTATUS Status;
    
    TRACE("(%p)\n", Vcb);
    
    le = Vcb->trees.Flink;
    while (le != &Vcb->trees) {
        tree* t = CONTAINING_RECORD(le, tree, list_entry);
        
        if (t->write && !t->parent) {
            if (t->root != Vcb->root_root && t->root != Vcb->chunk_root) {
                KEY searchkey;
                traverse_ptr tp;
                
                searchkey.obj_id = t->root->id;
                searchkey.obj_type = TYPE_ROOT_ITEM;
                searchkey.offset = 0xffffffffffffffff;
                
                Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE, Irp);
                if (!NT_SUCCESS(Status)) {
                    ERR("error - find_item returned %08x\n", Status);
                    return Status;
                }
                
                if (tp.item->key.obj_id != searchkey.obj_id || tp.item->key.obj_type != searchkey.obj_type) {
                    ERR("could not find ROOT_ITEM for tree %llx\n", searchkey.obj_id);
                    int3;
                    return STATUS_INTERNAL_ERROR;
                }
                
                TRACE("updating the address for root %llx to %llx\n", searchkey.obj_id, t->new_address);
                
                t->root->root_item.block_number = t->new_address;
                t->root->root_item.root_level = t->header.level;
                t->root->root_item.generation = Vcb->superblock.generation;
                t->root->root_item.generation2 = Vcb->superblock.generation;
                
                if (tp.item->size < sizeof(ROOT_ITEM)) { // if not full length, delete and create new entry
                    ROOT_ITEM* ri = ExAllocatePoolWithTag(PagedPool, sizeof(ROOT_ITEM), ALLOC_TAG);
                    
                    if (!ri) {
                        ERR("out of memory\n");
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }
                    
                    RtlCopyMemory(ri, &t->root->root_item, sizeof(ROOT_ITEM));
                    
                    delete_tree_item(Vcb, &tp, rollback);
                    
                    if (!insert_tree_item(Vcb, Vcb->root_root, searchkey.obj_id, searchkey.obj_type, 0, ri, sizeof(ROOT_ITEM), NULL, Irp, rollback)) {
                        ERR("insert_tree_item failed\n");
                        return STATUS_INTERNAL_ERROR;
                    }
                } else
                    RtlCopyMemory(tp.item->data, &t->root->root_item, sizeof(ROOT_ITEM));
            }
            
            t->root->treeholder.address = t->new_address;
        }
        
        le = le->Flink;
    }
    
    Status = update_chunk_caches(Vcb, Irp, rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("update_chunk_caches returned %08x\n", Status);
        return Status;
    }
    
    return STATUS_SUCCESS;
}

static NTSTATUS STDCALL write_data_completion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID conptr) {
    write_data_stripe* stripe = conptr;
    write_data_context* context = (write_data_context*)stripe->context;
    LIST_ENTRY* le;
    
    // FIXME - we need a lock here
    
    if (stripe->status == WriteDataStatus_Cancelling) {
        stripe->status = WriteDataStatus_Cancelled;
        goto end;
    }
    
    stripe->iosb = Irp->IoStatus;
    
    if (NT_SUCCESS(Irp->IoStatus.Status)) {
        stripe->status = WriteDataStatus_Success;
    } else {
        le = context->stripes.Flink;
        
        stripe->status = WriteDataStatus_Error;
        
        while (le != &context->stripes) {
            write_data_stripe* s2 = CONTAINING_RECORD(le, write_data_stripe, list_entry);
            
            if (s2->status == WriteDataStatus_Pending) {
                s2->status = WriteDataStatus_Cancelling;
                IoCancelIrp(s2->Irp);
            }
            
            le = le->Flink;
        }
    }
    
end:
    if (InterlockedDecrement(&context->stripes_left) == 0)
        KeSetEvent(&context->Event, 0, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

void free_write_data_stripes(write_data_context* wtc) {
    LIST_ENTRY *le, *le2, *nextle;
    
    le = wtc->stripes.Flink;
    while (le != &wtc->stripes) {
        write_data_stripe* stripe = CONTAINING_RECORD(le, write_data_stripe, list_entry);
        
        if (stripe->Irp) {
            if (stripe->device->devobj->Flags & DO_DIRECT_IO) {
                MmUnlockPages(stripe->Irp->MdlAddress);
                IoFreeMdl(stripe->Irp->MdlAddress);
            }
        }
        
        le = le->Flink;
    }
    
    le = wtc->stripes.Flink;
    while (le != &wtc->stripes) {
        write_data_stripe* stripe = CONTAINING_RECORD(le, write_data_stripe, list_entry);
        
        nextle = le->Flink;

        if (stripe->buf && stripe->need_free) {
            ExFreePool(stripe->buf);
            
            le2 = le->Flink;
            while (le2 != &wtc->stripes) {
                write_data_stripe* s2 = CONTAINING_RECORD(le2, write_data_stripe, list_entry);
                
                if (s2->buf == stripe->buf)
                    s2->buf = NULL;
                
                le2 = le2->Flink;
            }
            
        }
        
        ExFreePool(stripe);
        
        le = nextle;
    }
}

static NTSTATUS write_trees(device_extension* Vcb, PIRP Irp) {
    UINT8 level;
    UINT8 *data, *body;
    UINT32 crc32;
    NTSTATUS Status;
    LIST_ENTRY* le;
    write_data_context* wtc;
    
    TRACE("(%p)\n", Vcb);
    
    for (level = 0; level <= 255; level++) {
        BOOL nothing_found = TRUE;
        
        TRACE("level = %u\n", level);
        
        le = Vcb->trees.Flink;
        while (le != &Vcb->trees) {
            tree* t = CONTAINING_RECORD(le, tree, list_entry);
            
            if (t->write && t->header.level == level) {
                KEY firstitem, searchkey;
                LIST_ENTRY* le2;
                traverse_ptr tp;
                EXTENT_ITEM_TREE* eit;
                
                if (!t->has_new_address) {
                    ERR("error - tried to write tree with no new address\n");
                    int3;
                }
                
                le2 = t->itemlist.Flink;
                while (le2 != &t->itemlist) {
                    tree_data* td = CONTAINING_RECORD(le2, tree_data, list_entry);
                    if (!td->ignore) {
                        firstitem = td->key;
                        break;
                    }
                    le2 = le2->Flink;
                }
                
                if (t->parent) {
                    t->paritem->key = firstitem;
                    t->paritem->treeholder.address = t->new_address;
                    t->paritem->treeholder.generation = Vcb->superblock.generation;
                }
                
                if (!(Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA)) {
                    searchkey.obj_id = t->new_address;
                    searchkey.obj_type = TYPE_EXTENT_ITEM;
                    searchkey.offset = Vcb->superblock.node_size;
                    
                    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE, Irp);
                    if (!NT_SUCCESS(Status)) {
                        ERR("error - find_item returned %08x\n", Status);
                        return Status;
                    }
                    
                    if (keycmp(&searchkey, &tp.item->key)) {
//                         traverse_ptr next_tp;
//                         BOOL b;
//                         tree_data* paritem;
                        
                        ERR("could not find %llx,%x,%llx in extent_root (found %llx,%x,%llx instead)\n", searchkey.obj_id, searchkey.obj_type, searchkey.offset, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
                        
//                         searchkey.obj_id = 0;
//                         searchkey.obj_type = 0;
//                         searchkey.offset = 0;
//                         
//                         find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE);
//                         
//                         paritem = NULL;
//                         do {
//                             if (tp.tree->paritem != paritem) {
//                                 paritem = tp.tree->paritem;
//                                 ERR("paritem: %llx,%x,%llx\n", paritem->key.obj_id, paritem->key.obj_type, paritem->key.offset);
//                             }
//                             
//                             ERR("%llx,%x,%llx\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
//                             
//                             b = find_next_item(Vcb, &tp, &next_tp, NULL, FALSE);
//                             if (b) {
//                                 free_traverse_ptr(&tp);
//                                 tp = next_tp;
//                             }
//                         } while (b);
//                         
//                         free_traverse_ptr(&tp);
                        
                        return STATUS_INTERNAL_ERROR;
                    }
                    
                    if (tp.item->size < sizeof(EXTENT_ITEM_TREE)) {
                        ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_ITEM_TREE));
                        return STATUS_INTERNAL_ERROR;
                    }
                    
                    eit = (EXTENT_ITEM_TREE*)tp.item->data;
                    eit->firstitem = firstitem;
                }
                
                nothing_found = FALSE;
            }
            
            le = le->Flink;
        }
        
        if (nothing_found)
            break;
    }
    
    TRACE("allocated tree extents\n");
    
    wtc = ExAllocatePoolWithTag(NonPagedPool, sizeof(write_data_context), ALLOC_TAG);
    if (!wtc) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    KeInitializeEvent(&wtc->Event, NotificationEvent, FALSE);
    InitializeListHead(&wtc->stripes);
    wtc->tree = TRUE;
    wtc->stripes_left = 0;
    
    le = Vcb->trees.Flink;
    while (le != &Vcb->trees) {
        tree* t = CONTAINING_RECORD(le, tree, list_entry);
#ifdef DEBUG_PARANOID
        UINT32 num_items = 0, size = 0;
        LIST_ENTRY* le2;
        BOOL crash = FALSE;
#endif

        if (t->write) {
#ifdef DEBUG_PARANOID
            le2 = t->itemlist.Flink;
            while (le2 != &t->itemlist) {
                tree_data* td = CONTAINING_RECORD(le2, tree_data, list_entry);
                if (!td->ignore) {
                    num_items++;
                    
                    if (t->header.level == 0)
                        size += td->size;
                }
                le2 = le2->Flink;
            }
            
            if (t->header.level == 0)
                size += num_items * sizeof(leaf_node);
            else
                size += num_items * sizeof(internal_node);
            
            if (num_items != t->header.num_items) {
                ERR("tree %llx, level %x: num_items was %x, expected %x\n", t->root->id, t->header.level, num_items, t->header.num_items);
                crash = TRUE;
            }
            
            if (size != t->size) {
                ERR("tree %llx, level %x: size was %x, expected %x\n", t->root->id, t->header.level, size, t->size);
                crash = TRUE;
            }
            
            if (t->header.num_items == 0 && t->parent) {
                ERR("tree %llx, level %x: tried to write empty tree with parent\n", t->root->id, t->header.level);
                crash = TRUE;
            }
            
            if (t->size > Vcb->superblock.node_size - sizeof(tree_header)) {
                ERR("tree %llx, level %x: tried to write overlarge tree (%x > %x)\n", t->root->id, t->header.level, t->size, Vcb->superblock.node_size - sizeof(tree_header));
                crash = TRUE;
            }
            
            if (crash) {
                ERR("tree %p\n", t);
                le2 = t->itemlist.Flink;
                while (le2 != &t->itemlist) {
                    tree_data* td = CONTAINING_RECORD(le2, tree_data, list_entry);
                    if (!td->ignore) {
                        ERR("%llx,%x,%llx inserted=%u\n", td->key.obj_id, td->key.obj_type, td->key.offset, td->inserted);
                    }
                    le2 = le2->Flink;
                }
                int3;
            }
#endif
            t->header.address = t->new_address;
            t->header.generation = Vcb->superblock.generation;
            t->header.flags |= HEADER_FLAG_MIXED_BACKREF;
            t->has_address = TRUE;
            
            data = ExAllocatePoolWithTag(NonPagedPool, Vcb->superblock.node_size, ALLOC_TAG);
            if (!data) {
                ERR("out of memory\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto end;
            }
            
            body = data + sizeof(tree_header);
            
            RtlCopyMemory(data, &t->header, sizeof(tree_header));
            RtlZeroMemory(body, Vcb->superblock.node_size - sizeof(tree_header));
            
            if (t->header.level == 0) {
                leaf_node* itemptr = (leaf_node*)body;
                int i = 0;
                LIST_ENTRY* le2;
                UINT8* dataptr = data + Vcb->superblock.node_size;
                
                le2 = t->itemlist.Flink;
                while (le2 != &t->itemlist) {
                    tree_data* td = CONTAINING_RECORD(le2, tree_data, list_entry);
                    if (!td->ignore) {
                        dataptr = dataptr - td->size;
                        
                        itemptr[i].key = td->key;
                        itemptr[i].offset = (UINT8*)dataptr - (UINT8*)body;
                        itemptr[i].size = td->size;
                        i++;
                        
                        if (td->size > 0)
                            RtlCopyMemory(dataptr, td->data, td->size);
                    }
                    
                    le2 = le2->Flink;
                }
            } else {
                internal_node* itemptr = (internal_node*)body;
                int i = 0;
                LIST_ENTRY* le2;
                
                le2 = t->itemlist.Flink;
                while (le2 != &t->itemlist) {
                    tree_data* td = CONTAINING_RECORD(le2, tree_data, list_entry);
                    if (!td->ignore) {
                        itemptr[i].key = td->key;
                        itemptr[i].address = td->treeholder.address;
                        itemptr[i].generation = td->treeholder.generation;
                        i++;
                    }
                    
                    le2 = le2->Flink;
                }
            }
            
            crc32 = calc_crc32c(0xffffffff, (UINT8*)&((tree_header*)data)->fs_uuid, Vcb->superblock.node_size - sizeof(((tree_header*)data)->csum));
            crc32 = ~crc32;
            *((UINT32*)data) = crc32;
            TRACE("setting crc32 to %08x\n", crc32);
            
            Status = write_data(Vcb, t->new_address, data, TRUE, Vcb->superblock.node_size, wtc, NULL, NULL);
            if (!NT_SUCCESS(Status)) {
                ERR("write_data returned %08x\n", Status);
                goto end;
            }
        }

        le = le->Flink;
    }
    
    Status = STATUS_SUCCESS;
    
    if (wtc->stripes.Flink != &wtc->stripes) {
        // launch writes and wait
        le = wtc->stripes.Flink;
        while (le != &wtc->stripes) {
            write_data_stripe* stripe = CONTAINING_RECORD(le, write_data_stripe, list_entry);
            
            if (stripe->status != WriteDataStatus_Ignore)
                IoCallDriver(stripe->device->devobj, stripe->Irp);
            
            le = le->Flink;
        }
        
        KeWaitForSingleObject(&wtc->Event, Executive, KernelMode, FALSE, NULL);
        
        le = wtc->stripes.Flink;
        while (le != &wtc->stripes) {
            write_data_stripe* stripe = CONTAINING_RECORD(le, write_data_stripe, list_entry);
            
            if (stripe->status != WriteDataStatus_Ignore && !NT_SUCCESS(stripe->iosb.Status)) {
                Status = stripe->iosb.Status;
                break;
            }
            
            le = le->Flink;
        }
        
        free_write_data_stripes(wtc);
    }
    
end:
    ExFreePool(wtc);
    
    return Status;
}

static void update_backup_superblock(device_extension* Vcb, superblock_backup* sb, PIRP Irp) {
    KEY searchkey;
    traverse_ptr tp;
    
    RtlZeroMemory(sb, sizeof(superblock_backup));
    
    sb->root_tree_addr = Vcb->superblock.root_tree_addr;
    sb->root_tree_generation = Vcb->superblock.generation;
    sb->root_level = Vcb->superblock.root_level;

    sb->chunk_tree_addr = Vcb->superblock.chunk_tree_addr;
    sb->chunk_tree_generation = Vcb->superblock.chunk_root_generation;
    sb->chunk_root_level = Vcb->superblock.chunk_root_level;

    searchkey.obj_id = BTRFS_ROOT_EXTENT;
    searchkey.obj_type = TYPE_ROOT_ITEM;
    searchkey.offset = 0xffffffffffffffff;
    
    if (NT_SUCCESS(find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE, Irp))) {
        if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == searchkey.obj_type && tp.item->size >= sizeof(ROOT_ITEM)) {
            ROOT_ITEM* ri = (ROOT_ITEM*)tp.item->data;
            
            sb->extent_tree_addr = ri->block_number;
            sb->extent_tree_generation = ri->generation;
            sb->extent_root_level = ri->root_level;
        }
    }

    searchkey.obj_id = BTRFS_ROOT_FSTREE;
    
    if (NT_SUCCESS(find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE, Irp))) {
        if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == searchkey.obj_type && tp.item->size >= sizeof(ROOT_ITEM)) {
            ROOT_ITEM* ri = (ROOT_ITEM*)tp.item->data;
            
            sb->fs_tree_addr = ri->block_number;
            sb->fs_tree_generation = ri->generation;
            sb->fs_root_level = ri->root_level;
        }
    }
    
    searchkey.obj_id = BTRFS_ROOT_DEVTREE;
    
    if (NT_SUCCESS(find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE, Irp))) {
        if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == searchkey.obj_type && tp.item->size >= sizeof(ROOT_ITEM)) {
            ROOT_ITEM* ri = (ROOT_ITEM*)tp.item->data;
            
            sb->dev_root_addr = ri->block_number;
            sb->dev_root_generation = ri->generation;
            sb->dev_root_level = ri->root_level;
        }
    }

    searchkey.obj_id = BTRFS_ROOT_CHECKSUM;
    
    if (NT_SUCCESS(find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE, Irp))) {
        if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == searchkey.obj_type && tp.item->size >= sizeof(ROOT_ITEM)) {
            ROOT_ITEM* ri = (ROOT_ITEM*)tp.item->data;
            
            sb->csum_root_addr = ri->block_number;
            sb->csum_root_generation = ri->generation;
            sb->csum_root_level = ri->root_level;
        }
    }

    sb->total_bytes = Vcb->superblock.total_bytes;
    sb->bytes_used = Vcb->superblock.bytes_used;
    sb->num_devices = Vcb->superblock.num_devices;
}

static NTSTATUS write_superblocks(device_extension* Vcb, PIRP Irp) {
    UINT64 i;
    NTSTATUS Status;
    LIST_ENTRY* le;
    
    TRACE("(%p)\n", Vcb);
    
    le = Vcb->trees.Flink;
    while (le != &Vcb->trees) {
        tree* t = CONTAINING_RECORD(le, tree, list_entry);
        
        if (t->write && !t->parent) {
            if (t->root == Vcb->root_root) {
                Vcb->superblock.root_tree_addr = t->new_address;
                Vcb->superblock.root_level = t->header.level;
            } else if (t->root == Vcb->chunk_root) {
                Vcb->superblock.chunk_tree_addr = t->new_address;
                Vcb->superblock.chunk_root_generation = t->header.generation;
                Vcb->superblock.chunk_root_level = t->header.level;
            }
        }
        
        le = le->Flink;
    }
    
    for (i = 0; i < BTRFS_NUM_BACKUP_ROOTS - 1; i++) {
        RtlCopyMemory(&Vcb->superblock.backup[i], &Vcb->superblock.backup[i+1], sizeof(superblock_backup));
    }
    
    update_backup_superblock(Vcb, &Vcb->superblock.backup[BTRFS_NUM_BACKUP_ROOTS - 1], Irp);
    
    for (i = 0; i < Vcb->superblock.num_devices; i++) {
        if (Vcb->devices[i].devobj) {
            Status = write_superblock(Vcb, &Vcb->devices[i]);
            if (!NT_SUCCESS(Status)) {
                ERR("write_superblock returned %08x\n", Status);
                return Status;
            }
        }
    }
    
    return STATUS_SUCCESS;
}

static NTSTATUS flush_changed_extent(device_extension* Vcb, chunk* c, changed_extent* ce, PIRP Irp, LIST_ENTRY* rollback) {
    LIST_ENTRY *le, *le2;
    NTSTATUS Status;
    UINT64 old_size;
    
    le = ce->refs.Flink;
    while (le != &ce->refs) {
        changed_extent_ref* cer = CONTAINING_RECORD(le, changed_extent_ref, list_entry);
        LIST_ENTRY* le3 = le->Flink;
        UINT64 old_count = 0;
        
        le2 = ce->old_refs.Flink;
        while (le2 != &ce->old_refs) {
            changed_extent_ref* cer2 = CONTAINING_RECORD(le2, changed_extent_ref, list_entry);
            
            if (cer2->edr.root == cer->edr.root && cer2->edr.objid == cer->edr.objid && cer2->edr.offset == cer->edr.offset) {
                old_count = cer2->edr.count;
                
                RemoveEntryList(&cer2->list_entry);
                ExFreePool(cer2);
                break;
            }
            
            le2 = le2->Flink;
        }
        
        old_size = ce->old_count > 0 ? ce->old_size : ce->size;
        
        if (cer->edr.count > old_count) {
            Status = increase_extent_refcount_data(Vcb, ce->address, old_size, cer->edr.root, cer->edr.objid, cer->edr.offset, cer->edr.count - old_count, Irp, rollback);
                        
            if (!NT_SUCCESS(Status)) {
                ERR("increase_extent_refcount_data returned %08x\n", Status);
                return Status;
            }
        } else if (cer->edr.count < old_count) {
            Status = decrease_extent_refcount_data(Vcb, ce->address, old_size, cer->edr.root, cer->edr.objid, cer->edr.offset,
                                                   old_count - cer->edr.count, Irp, rollback);
            
            if (!NT_SUCCESS(Status)) {
                ERR("decrease_extent_refcount_data returned %08x\n", Status);
                return Status;
            }
        }
        
        if (ce->size != ce->old_size && ce->old_count > 0) {
            KEY searchkey;
            traverse_ptr tp;
            void* data;
            
            searchkey.obj_id = ce->address;
            searchkey.obj_type = TYPE_EXTENT_ITEM;
            searchkey.offset = ce->old_size;
            
            Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE, Irp);
            if (!NT_SUCCESS(Status)) {
                ERR("error - find_item returned %08x\n", Status);
                return Status;
            }
            
            if (keycmp(&searchkey, &tp.item->key)) {
                ERR("could not find (%llx,%x,%llx) in extent tree\n", searchkey.obj_id, searchkey.obj_type, searchkey.offset);
                return STATUS_INTERNAL_ERROR;
            }
            
            if (tp.item->size > 0) {
                data = ExAllocatePoolWithTag(PagedPool, tp.item->size, ALLOC_TAG);
                
                if (!data) {
                    ERR("out of memory\n");
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                
                RtlCopyMemory(data, tp.item->data, tp.item->size);
            } else
                data = NULL;
            
            if (!insert_tree_item(Vcb, Vcb->extent_root, ce->address, TYPE_EXTENT_ITEM, ce->size, data, tp.item->size, NULL, Irp, rollback)) {
                ERR("insert_tree_item failed\n");
                return STATUS_INTERNAL_ERROR;
            }
            
            delete_tree_item(Vcb, &tp, rollback);
        }
       
        RemoveEntryList(&cer->list_entry);
        ExFreePool(cer);
        
        le = le3;
    }
    
#ifdef DEBUG_PARANOID
    if (!IsListEmpty(&ce->old_refs))
        WARN("old_refs not empty\n");
#endif
    
    if (ce->count == 0) {
        if (!ce->no_csum) {
            LIST_ENTRY changed_sector_list;
            
            changed_sector* sc = ExAllocatePoolWithTag(PagedPool, sizeof(changed_sector), ALLOC_TAG);
            if (!sc) {
                ERR("out of memory\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            
            sc->ol.key = ce->address;
            sc->checksums = NULL;
            sc->length = ce->size / Vcb->superblock.sector_size;

            sc->deleted = TRUE;
            
            InitializeListHead(&changed_sector_list);
            insert_into_ordered_list(&changed_sector_list, &sc->ol);
            
            ExAcquireResourceExclusiveLite(&Vcb->checksum_lock, TRUE);
            commit_checksum_changes(Vcb, &changed_sector_list);
            ExReleaseResourceLite(&Vcb->checksum_lock);
        }
        
        decrease_chunk_usage(c, ce->size);
        
        space_list_add(Vcb, c, TRUE, ce->address, ce->size, rollback);
    }

    RemoveEntryList(&ce->list_entry);
    ExFreePool(ce);
    
    return STATUS_SUCCESS;
}

static NTSTATUS update_chunk_usage(device_extension* Vcb, PIRP Irp, LIST_ENTRY* rollback) {
    LIST_ENTRY *le = Vcb->chunks.Flink, *le2;
    chunk* c;
    KEY searchkey;
    traverse_ptr tp;
    BLOCK_GROUP_ITEM* bgi;
    NTSTATUS Status;
    BOOL flushed_extents = FALSE;
    
    TRACE("(%p)\n", Vcb);
    
    ExAcquireResourceSharedLite(&Vcb->chunk_lock, TRUE);
    
    while (le != &Vcb->chunks) {
        c = CONTAINING_RECORD(le, chunk, list_entry);
        
        ExAcquireResourceExclusiveLite(&c->lock, TRUE);
        
        le2 = c->changed_extents.Flink;
        while (le2 != &c->changed_extents) {
            LIST_ENTRY* le3 = le2->Flink;
            changed_extent* ce = CONTAINING_RECORD(le2, changed_extent, list_entry);
            
            Status = flush_changed_extent(Vcb, c, ce, Irp, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("flush_changed_extent returned %08x\n", Status);
                ExReleaseResourceLite(&c->lock);
                goto end;
            }
            
            flushed_extents = TRUE;
            
            le2 = le3;
        }
        
        if (c->used != c->oldused) {
            searchkey.obj_id = c->offset;
            searchkey.obj_type = TYPE_BLOCK_GROUP_ITEM;
            searchkey.offset = c->chunk_item->size;
            
            Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE, Irp);
            if (!NT_SUCCESS(Status)) {
                ERR("error - find_item returned %08x\n", Status);
                ExReleaseResourceLite(&c->lock);
                goto end;
            }
            
            if (keycmp(&searchkey, &tp.item->key)) {
                ERR("could not find (%llx,%x,%llx) in extent_root\n", searchkey.obj_id, searchkey.obj_type, searchkey.offset);
                int3;
                Status = STATUS_INTERNAL_ERROR;
                ExReleaseResourceLite(&c->lock);
                goto end;
            }
            
            if (tp.item->size < sizeof(BLOCK_GROUP_ITEM)) {
                ERR("(%llx,%x,%llx) was %u bytes, expected %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(BLOCK_GROUP_ITEM));
                Status = STATUS_INTERNAL_ERROR;
                ExReleaseResourceLite(&c->lock);
                goto end;
            }
            
            bgi = ExAllocatePoolWithTag(PagedPool, tp.item->size, ALLOC_TAG);
            if (!bgi) {
                ERR("out of memory\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                ExReleaseResourceLite(&c->lock);
                goto end;
            }
    
            RtlCopyMemory(bgi, tp.item->data, tp.item->size);
            bgi->used = c->used;
            
            TRACE("adjusting usage of chunk %llx to %llx\n", c->offset, c->used);
            
            delete_tree_item(Vcb, &tp, rollback);
            
            if (!insert_tree_item(Vcb, Vcb->extent_root, searchkey.obj_id, searchkey.obj_type, searchkey.offset, bgi, tp.item->size, NULL, Irp, rollback)) {
                ERR("insert_tree_item failed\n");
                ExFreePool(bgi);
                Status = STATUS_INTERNAL_ERROR;
                ExReleaseResourceLite(&c->lock);
                goto end;
            }
            
            TRACE("bytes_used = %llx\n", Vcb->superblock.bytes_used);
            TRACE("chunk_item type = %llx\n", c->chunk_item->type);
            
            if (c->chunk_item->type & BLOCK_FLAG_RAID0) {
                Vcb->superblock.bytes_used += c->used - c->oldused;
            } else if (c->chunk_item->type & BLOCK_FLAG_RAID1 || c->chunk_item->type & BLOCK_FLAG_DUPLICATE || c->chunk_item->type & BLOCK_FLAG_RAID10) {
                Vcb->superblock.bytes_used += 2 * (c->used - c->oldused);
            } else if (c->chunk_item->type & BLOCK_FLAG_RAID5) {
                FIXME("RAID5 not yet supported\n");
                ExFreePool(bgi);
                Status = STATUS_INTERNAL_ERROR;
                ExReleaseResourceLite(&c->lock);
                goto end;
            } else if (c->chunk_item->type & BLOCK_FLAG_RAID6) {
                FIXME("RAID6 not yet supported\n");
                ExFreePool(bgi);
                Status = STATUS_INTERNAL_ERROR;
                ExReleaseResourceLite(&c->lock);
                goto end;
            } else { // SINGLE
                Vcb->superblock.bytes_used += c->used - c->oldused;
            }
            
            TRACE("bytes_used = %llx\n", Vcb->superblock.bytes_used);
            
            c->oldused = c->used;
        }
        
        ExReleaseResourceLite(&c->lock);
        
        le = le->Flink;
    }
    
    if (flushed_extents) {
        ExAcquireResourceExclusiveLite(&Vcb->checksum_lock, TRUE);
        if (!IsListEmpty(&Vcb->sector_checksums)) {
            update_checksum_tree(Vcb, Irp, rollback);
        }
        ExReleaseResourceLite(&Vcb->checksum_lock);
    }
    
    Status = STATUS_SUCCESS;
    
end:
    ExReleaseResourceLite(&Vcb->chunk_lock);
    
    return Status;
}

static void get_first_item(tree* t, KEY* key) {
    LIST_ENTRY* le;
    
    le = t->itemlist.Flink;
    while (le != &t->itemlist) {
        tree_data* td = CONTAINING_RECORD(le, tree_data, list_entry);

        *key = td->key;
        return;
    }
}

static NTSTATUS STDCALL split_tree_at(device_extension* Vcb, tree* t, tree_data* newfirstitem, UINT32 numitems, UINT32 size) {
    tree *nt, *pt;
    tree_data* td;
    tree_data* oldlastitem;
//     write_tree* wt2;
// //     tree_data *firsttd, *lasttd;
// //     LIST_ENTRY* le;
// #ifdef DEBUG_PARANOID
//     KEY lastkey1, lastkey2;
//     traverse_ptr tp, next_tp;
//     ULONG numitems1, numitems2;
// #endif
    
    TRACE("splitting tree in %llx at (%llx,%x,%llx)\n", t->root->id, newfirstitem->key.obj_id, newfirstitem->key.obj_type, newfirstitem->key.offset);
    
// #ifdef DEBUG_PARANOID
//     lastkey1.obj_id = 0xffffffffffffffff;
//     lastkey1.obj_type = 0xff;
//     lastkey1.offset = 0xffffffffffffffff;
//     
//     if (!find_item(Vcb, t->root, &tp, &lastkey1, NULL, FALSE))
//         ERR("error - find_item failed\n");
//     else {
//         lastkey1 = tp.item->key;
//         numitems1 = 0;
//         while (find_prev_item(Vcb, &tp, &next_tp, NULL, FALSE)) {
//             free_traverse_ptr(&tp);
//             tp = next_tp;
//             numitems1++;
//         }
//         free_traverse_ptr(&tp);
//     }
// #endif
    
    nt = ExAllocatePoolWithTag(PagedPool, sizeof(tree), ALLOC_TAG);
    if (!nt) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlCopyMemory(&nt->header, &t->header, sizeof(tree_header));
    nt->header.address = 0;
    nt->header.generation = Vcb->superblock.generation;
    nt->header.num_items = t->header.num_items - numitems;
    nt->header.flags = HEADER_FLAG_MIXED_BACKREF;
    
    nt->has_address = FALSE;
    nt->Vcb = Vcb;
    nt->parent = t->parent;
    nt->root = t->root;
//     nt->nonpaged = ExAllocatePoolWithTag(NonPagedPool, sizeof(tree_nonpaged), ALLOC_TAG);
    nt->new_address = 0;
    nt->has_new_address = FALSE;
    nt->flags = t->flags;
    InitializeListHead(&nt->itemlist);
    
//     ExInitializeResourceLite(&nt->nonpaged->load_tree_lock);
    
    oldlastitem = CONTAINING_RECORD(newfirstitem->list_entry.Blink, tree_data, list_entry);

// //     firsttd = CONTAINING_RECORD(wt->tree->itemlist.Flink, tree_data, list_entry);
// //     lasttd = CONTAINING_RECORD(wt->tree->itemlist.Blink, tree_data, list_entry);
// //     
// //     TRACE("old tree in %x was from (%x,%x,%x) to (%x,%x,%x)\n",
// //                   (UINT32)wt->tree->root->id, (UINT32)firsttd->key.obj_id, firsttd->key.obj_type, (UINT32)firsttd->key.offset,
// //                   (UINT32)lasttd->key.obj_id, lasttd->key.obj_type, (UINT32)lasttd->key.offset);
// //     
// //     le = wt->tree->itemlist.Flink;
// //     while (le != &wt->tree->itemlist) {
// //         td = CONTAINING_RECORD(le, tree_data, list_entry);
// //         TRACE("old tree item was (%x,%x,%x)\n", (UINT32)td->key.obj_id, td->key.obj_type, (UINT32)td->key.offset);
// //         le = le->Flink;
// //     }
    
    nt->itemlist.Flink = &newfirstitem->list_entry;
    nt->itemlist.Blink = t->itemlist.Blink;
    nt->itemlist.Flink->Blink = &nt->itemlist;
    nt->itemlist.Blink->Flink = &nt->itemlist;
    
    t->itemlist.Blink = &oldlastitem->list_entry;
    t->itemlist.Blink->Flink = &t->itemlist;
    
// //     le = wt->tree->itemlist.Flink;
// //     while (le != &wt->tree->itemlist) {
// //         td = CONTAINING_RECORD(le, tree_data, list_entry);
// //         TRACE("old tree item now (%x,%x,%x)\n", (UINT32)td->key.obj_id, td->key.obj_type, (UINT32)td->key.offset);
// //         le = le->Flink;
// //     }
// //     
// //     firsttd = CONTAINING_RECORD(wt->tree->itemlist.Flink, tree_data, list_entry);
// //     lasttd = CONTAINING_RECORD(wt->tree->itemlist.Blink, tree_data, list_entry);
// //     
// //     TRACE("old tree in %x is now from (%x,%x,%x) to (%x,%x,%x)\n",
// //                   (UINT32)wt->tree->root->id, (UINT32)firsttd->key.obj_id, firsttd->key.obj_type, (UINT32)firsttd->key.offset,
// //                   (UINT32)lasttd->key.obj_id, lasttd->key.obj_type, (UINT32)lasttd->key.offset);
    
    nt->size = t->size - size;
    t->size = size;
    t->header.num_items = numitems;
    nt->write = TRUE;
    
    InterlockedIncrement(&Vcb->open_trees);
    InsertTailList(&Vcb->trees, &nt->list_entry);
    
// //     // TESTING
// //     td = wt->tree->items;
// //     while (td) {
// //         if (!td->ignore) {
// //             TRACE("old tree item: (%x,%x,%x)\n", (UINT32)td->key.obj_id, td->key.obj_type, (UINT32)td->key.offset);
// //         }
// //         td = td->next;
// //     }
    
// //     oldlastitem->next = NULL;
// //     wt->tree->lastitem = oldlastitem;
    
// //     TRACE("last item is now (%x,%x,%x)\n", (UINT32)oldlastitem->key.obj_id, oldlastitem->key.obj_type, (UINT32)oldlastitem->key.offset);
    
    if (nt->header.level > 0) {
        LIST_ENTRY* le = nt->itemlist.Flink;
        
        while (le != &nt->itemlist) {
            tree_data* td2 = CONTAINING_RECORD(le, tree_data, list_entry);
            
            if (td2->treeholder.tree)
                td2->treeholder.tree->parent = nt;
            
            le = le->Flink;
        }
    }
    
    if (nt->parent) {
        td = ExAllocatePoolWithTag(PagedPool, sizeof(tree_data), ALLOC_TAG);
        if (!td) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    
        td->key = newfirstitem->key;
        
        InsertHeadList(&t->paritem->list_entry, &td->list_entry);
        
        td->ignore = FALSE;
        td->inserted = TRUE;
        td->treeholder.tree = nt;
//         td->treeholder.nonpaged->status = tree_holder_loaded;
        nt->paritem = td;
        
        nt->parent->header.num_items++;
        nt->parent->size += sizeof(internal_node);

        goto end;
    }
    
    TRACE("adding new tree parent\n");
    
    if (nt->header.level == 255) {
        ERR("cannot add parent to tree at level 255\n");
        return STATUS_INTERNAL_ERROR;
    }
    
    pt = ExAllocatePoolWithTag(PagedPool, sizeof(tree), ALLOC_TAG);
    if (!pt) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlCopyMemory(&pt->header, &nt->header, sizeof(tree_header));
    pt->header.address = 0;
    pt->header.num_items = 2;
    pt->header.level = nt->header.level + 1;
    pt->header.flags = HEADER_FLAG_MIXED_BACKREF;
    
    pt->has_address = FALSE;
    pt->Vcb = Vcb;
    pt->parent = NULL;
    pt->paritem = NULL;
    pt->root = t->root;
    pt->new_address = 0;
    pt->has_new_address = FALSE;
//     pt->nonpaged = ExAllocatePoolWithTag(NonPagedPool, sizeof(tree_nonpaged), ALLOC_TAG);
    pt->size = pt->header.num_items * sizeof(internal_node);
    pt->flags = t->flags;
    InitializeListHead(&pt->itemlist);
    
//     ExInitializeResourceLite(&pt->nonpaged->load_tree_lock);
    
    InterlockedIncrement(&Vcb->open_trees);
    InsertTailList(&Vcb->trees, &pt->list_entry);
    
    td = ExAllocatePoolWithTag(PagedPool, sizeof(tree_data), ALLOC_TAG);
    if (!td) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    get_first_item(t, &td->key);
    td->ignore = FALSE;
    td->inserted = FALSE;
    td->treeholder.address = 0;
    td->treeholder.generation = Vcb->superblock.generation;
    td->treeholder.tree = t;
//     td->treeholder.nonpaged->status = tree_holder_loaded;
    InsertTailList(&pt->itemlist, &td->list_entry);
    t->paritem = td;
    
    td = ExAllocatePoolWithTag(PagedPool, sizeof(tree_data), ALLOC_TAG);
    if (!td) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    td->key = newfirstitem->key;
    td->ignore = FALSE;
    td->inserted = FALSE;
    td->treeholder.address = 0;
    td->treeholder.generation = Vcb->superblock.generation;
    td->treeholder.tree = nt;
//     td->treeholder.nonpaged->status = tree_holder_loaded;
    InsertTailList(&pt->itemlist, &td->list_entry);
    nt->paritem = td;
    
    pt->write = TRUE;

    t->root->treeholder.tree = pt;
    
    t->parent = pt;
    nt->parent = pt;
    
end:
    t->root->root_item.bytes_used += Vcb->superblock.node_size;

// #ifdef DEBUG_PARANOID
//     lastkey2.obj_id = 0xffffffffffffffff;
//     lastkey2.obj_type = 0xff;
//     lastkey2.offset = 0xffffffffffffffff;
//     
//     if (!find_item(Vcb, wt->tree->root, &tp, &lastkey2, NULL, FALSE))
//         ERR("error - find_item failed\n");
//     else {    
//         lastkey2 = tp.item->key;
//         
//         numitems2 = 0;
//         while (find_prev_item(Vcb, &tp, &next_tp, NULL, FALSE)) {
//             free_traverse_ptr(&tp);
//             tp = next_tp;
//             numitems2++;
//         }
//         free_traverse_ptr(&tp);
//     }
//     
//     ERR("lastkey1 = %llx,%x,%llx\n", lastkey1.obj_id, lastkey1.obj_type, lastkey1.offset);
//     ERR("lastkey2 = %llx,%x,%llx\n", lastkey2.obj_id, lastkey2.obj_type, lastkey2.offset);
//     ERR("numitems1 = %u\n", numitems1);
//     ERR("numitems2 = %u\n", numitems2);
// #endif
    
    return STATUS_SUCCESS;
}

static NTSTATUS STDCALL split_tree(device_extension* Vcb, tree* t) {
    LIST_ENTRY* le;
    UINT32 size, ds, numitems;
    
    size = 0;
    numitems = 0;
    
    // FIXME - naïve implementation: maximizes number of filled trees
    
    le = t->itemlist.Flink;
    while (le != &t->itemlist) {
        tree_data* td = CONTAINING_RECORD(le, tree_data, list_entry);
        
        if (!td->ignore) {
            if (t->header.level == 0)
                ds = sizeof(leaf_node) + td->size;
            else
                ds = sizeof(internal_node);
            
            // FIXME - move back if previous item was deleted item with same key
            if (size + ds > Vcb->superblock.node_size - sizeof(tree_header))
                return split_tree_at(Vcb, t, td, numitems, size);

            size += ds;
            numitems++;
        }
        
        le = le->Flink;
    }
    
    return STATUS_SUCCESS;
}

static NTSTATUS try_tree_amalgamate(device_extension* Vcb, tree* t, PIRP Irp, LIST_ENTRY* rollback) {
    LIST_ENTRY* le;
    tree_data* nextparitem = NULL;
    NTSTATUS Status;
    tree *next_tree, *par;
    BOOL loaded;
    
    TRACE("trying to amalgamate tree in root %llx, level %x (size %u)\n", t->root->id, t->header.level, t->size);
    
    // FIXME - doesn't capture everything, as it doesn't ascend
    // FIXME - write proper function and put it in treefuncs.c
    le = t->paritem->list_entry.Flink;
    while (le != &t->parent->itemlist) {
        tree_data* td = CONTAINING_RECORD(le, tree_data, list_entry);
        
        if (!td->ignore) {
            nextparitem = td;
            break;
        }
        
        le = le->Flink;
    }
    
    if (!nextparitem)
        return STATUS_SUCCESS;
    
    // FIXME - loop, and capture more than one tree if we can
    
    TRACE("nextparitem: key = %llx,%x,%llx\n", nextparitem->key.obj_id, nextparitem->key.obj_type, nextparitem->key.offset);
//     nextparitem = t->paritem;
    
//     ExAcquireResourceExclusiveLite(&t->parent->nonpaged->load_tree_lock, TRUE);
    
    Status = do_load_tree(Vcb, &nextparitem->treeholder, t->root, t->parent, nextparitem, &loaded, NULL);
    if (!NT_SUCCESS(Status)) {
        ERR("do_load_tree returned %08x\n", Status);
        return Status;
    }
    
//     ExReleaseResourceLite(&t->parent->nonpaged->load_tree_lock);
    
    next_tree = nextparitem->treeholder.tree;
    
    if (t->size + next_tree->size <= Vcb->superblock.node_size - sizeof(tree_header)) {
        // merge two trees into one
        
        t->header.num_items += next_tree->header.num_items;
        t->size += next_tree->size;
        
        if (next_tree->header.level > 0) {
            le = next_tree->itemlist.Flink;
            
            while (le != &next_tree->itemlist) {
                tree_data* td2 = CONTAINING_RECORD(le, tree_data, list_entry);
                
                if (td2->treeholder.tree)
                    td2->treeholder.tree->parent = t;
                
                le = le->Flink;
            }
        }
        
        t->itemlist.Blink->Flink = next_tree->itemlist.Flink;
        t->itemlist.Blink->Flink->Blink = t->itemlist.Blink;
        t->itemlist.Blink = next_tree->itemlist.Blink;
        t->itemlist.Blink->Flink = &t->itemlist;
        
//         // TESTING
//         le = t->itemlist.Flink;
//         while (le != &t->itemlist) {
//             tree_data* td = CONTAINING_RECORD(le, tree_data, list_entry);
//             if (!td->ignore) {
//                 ERR("key: %llx,%x,%llx\n", td->key.obj_id, td->key.obj_type, td->key.offset);
//             }
//             le = le->Flink;
//         }
        
        next_tree->itemlist.Flink = next_tree->itemlist.Blink = &next_tree->itemlist;
        
        next_tree->header.num_items = 0;
        next_tree->size = 0;
        
        if (next_tree->has_new_address) { // delete associated EXTENT_ITEM
            Status = reduce_tree_extent(Vcb, next_tree->new_address, next_tree, Irp, rollback);
            
            if (!NT_SUCCESS(Status)) {
                ERR("reduce_tree_extent returned %08x\n", Status);
                return Status;
            }
        } else if (next_tree->has_address) {
            Status = reduce_tree_extent(Vcb, next_tree->header.address, next_tree, Irp, rollback);
            
            if (!NT_SUCCESS(Status)) {
                ERR("reduce_tree_extent returned %08x\n", Status);
                return Status;
            }
        }
        
        if (!nextparitem->ignore) {
            nextparitem->ignore = TRUE;
            next_tree->parent->header.num_items--;
            next_tree->parent->size -= sizeof(internal_node);
        }
        
        par = next_tree->parent;
        while (par) {
            par->write = TRUE;
            par = par->parent;
        }
        
        RemoveEntryList(&nextparitem->list_entry);
        ExFreePool(next_tree->paritem);
        next_tree->paritem = NULL;
        
        next_tree->root->root_item.bytes_used -= Vcb->superblock.node_size;
        
        free_tree(next_tree);
    } else {
        // rebalance by moving items from second tree into first
        ULONG avg_size = (t->size + next_tree->size) / 2;
        KEY firstitem = {0, 0, 0};
        
        TRACE("attempting rebalance\n");
        
        le = next_tree->itemlist.Flink;
        while (le != &next_tree->itemlist && t->size < avg_size && next_tree->header.num_items > 1) {
            tree_data* td = CONTAINING_RECORD(le, tree_data, list_entry);
            ULONG size;
            
            if (!td->ignore) {
                if (next_tree->header.level == 0)
                    size = sizeof(leaf_node) + td->size;
                else
                    size = sizeof(internal_node);
            } else
                size = 0;
            
            if (t->size + size < Vcb->superblock.node_size - sizeof(tree_header)) {
                RemoveEntryList(&td->list_entry);
                InsertTailList(&t->itemlist, &td->list_entry);
                
                if (next_tree->header.level > 0 && td->treeholder.tree)
                    td->treeholder.tree->parent = t;
                
                if (!td->ignore) {
                    next_tree->size -= size;
                    t->size += size;
                    next_tree->header.num_items--;
                    t->header.num_items++;
                }
            } else
                break;
            
            le = next_tree->itemlist.Flink;
        }
        
        le = next_tree->itemlist.Flink;
        while (le != &next_tree->itemlist) {
            tree_data* td = CONTAINING_RECORD(le, tree_data, list_entry);
            
            if (!td->ignore) {
                firstitem = td->key;
                break;
            }
            
            le = le->Flink;
        }
        
//         ERR("firstitem = %llx,%x,%llx\n", firstitem.obj_id, firstitem.obj_type, firstitem.offset);
        
        // FIXME - once ascension is working, make this work with parent's parent, etc.
        if (next_tree->paritem)
            next_tree->paritem->key = firstitem;
        
        par = next_tree;
        while (par) {
            par->write = TRUE;
            par = par->parent;
        }
    }
    
    return STATUS_SUCCESS;
}

static NTSTATUS update_extent_level(device_extension* Vcb, UINT64 address, tree* t, UINT8 level, PIRP Irp, LIST_ENTRY* rollback) {
    KEY searchkey;
    traverse_ptr tp;
    NTSTATUS Status;
    
    if (Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA) {
        searchkey.obj_id = address;
        searchkey.obj_type = TYPE_METADATA_ITEM;
        searchkey.offset = t->header.level;
        
        Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            return Status;
        }
        
        if (!keycmp(&tp.item->key, &searchkey)) {
            EXTENT_ITEM_SKINNY_METADATA* eism;
            
            if (tp.item->size > 0) {
                eism = ExAllocatePoolWithTag(PagedPool, tp.item->size, ALLOC_TAG);
                
                if (!eism) {
                    ERR("out of memory\n");
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                
                RtlCopyMemory(eism, tp.item->data, tp.item->size);
            } else
                eism = NULL;
            
            delete_tree_item(Vcb, &tp, rollback);
            
            if (!insert_tree_item(Vcb, Vcb->extent_root, address, TYPE_METADATA_ITEM, level, eism, tp.item->size, NULL, Irp, rollback)) {
                ERR("insert_tree_item failed\n");
                ExFreePool(eism);
                return STATUS_INTERNAL_ERROR;
            }
            
            return STATUS_SUCCESS;
        }
    }
    
    searchkey.obj_id = address;
    searchkey.obj_type = TYPE_EXTENT_ITEM;
    searchkey.offset = 0xffffffffffffffff;
    
    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == searchkey.obj_type) {
        EXTENT_ITEM_TREE* eit;
        
        if (tp.item->size < sizeof(EXTENT_ITEM_TREE)) {
            ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_ITEM_TREE));
            return STATUS_INTERNAL_ERROR;
        }
        
        eit = ExAllocatePoolWithTag(PagedPool, tp.item->size, ALLOC_TAG);
                
        if (!eit) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        RtlCopyMemory(eit, tp.item->data, tp.item->size);
        
        delete_tree_item(Vcb, &tp, rollback);
        
        eit->level = level;
        
        if (!insert_tree_item(Vcb, Vcb->extent_root, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, eit, tp.item->size, NULL, Irp, rollback)) {
            ERR("insert_tree_item failed\n");
            ExFreePool(eit);
            return STATUS_INTERNAL_ERROR;
        }
    
        return STATUS_SUCCESS;
    }
    
    ERR("could not find EXTENT_ITEM for address %llx\n", address);
    
    return STATUS_INTERNAL_ERROR;
}

static NTSTATUS STDCALL do_splits(device_extension* Vcb, PIRP Irp, LIST_ENTRY* rollback) {
//     LIST_ENTRY *le, *le2;
//     write_tree* wt;
//     tree_data* td;
    UINT8 level, max_level;
    UINT32 min_size;
    BOOL empty, done_deletions = FALSE;
    NTSTATUS Status;
    tree* t;
    
    TRACE("(%p)\n", Vcb);
    
    max_level = 0;
    
    for (level = 0; level <= 255; level++) {
        LIST_ENTRY *le, *nextle;
        
        empty = TRUE;
        
        TRACE("doing level %u\n", level);
        
        le = Vcb->trees.Flink;
    
        while (le != &Vcb->trees) {
            t = CONTAINING_RECORD(le, tree, list_entry);
            
            nextle = le->Flink;
            
            if (t->write && t->header.level == level) {
                empty = FALSE;
                
                if (t->header.num_items == 0) {
                    if (t->parent) {
                        LIST_ENTRY* le2;
                        KEY firstitem = {0xcccccccccccccccc,0xcc,0xcccccccccccccccc};
#ifdef __REACTOS__
                        (void)firstitem;
#endif
                        
                        done_deletions = TRUE;
            
                        le2 = t->itemlist.Flink;
                        while (le2 != &t->itemlist) {
                            tree_data* td = CONTAINING_RECORD(le2, tree_data, list_entry);
                            firstitem = td->key;
                            break;
                        }
                        
                        TRACE("deleting tree in root %llx (first item was %llx,%x,%llx)\n",
                              t->root->id, firstitem.obj_id, firstitem.obj_type, firstitem.offset);
                        
                        t->root->root_item.bytes_used -= Vcb->superblock.node_size;
                        
                        if (t->has_new_address) { // delete associated EXTENT_ITEM
                            Status = reduce_tree_extent(Vcb, t->new_address, t, Irp, rollback);
                            
                            if (!NT_SUCCESS(Status)) {
                                ERR("reduce_tree_extent returned %08x\n", Status);
                                return Status;
                            }
                            
                            t->has_new_address = FALSE;
                        } else if (t->has_address) {
                            Status = reduce_tree_extent(Vcb,t->header.address, t, Irp, rollback);
                            
                            if (!NT_SUCCESS(Status)) {
                                ERR("reduce_tree_extent returned %08x\n", Status);
                                return Status;
                            }
                            
                            t->has_address = FALSE;
                        }
                        
                        if (!t->paritem->ignore) {
                            t->paritem->ignore = TRUE;
                            t->parent->header.num_items--;
                            t->parent->size -= sizeof(internal_node);
                        }
                        
                        RemoveEntryList(&t->paritem->list_entry);
                        ExFreePool(t->paritem);
                        t->paritem = NULL;
                        
                        free_tree(t);
                    } else if (t->header.level != 0) {
                        if (t->has_new_address) {
                            Status = update_extent_level(Vcb, t->new_address, t, 0, Irp, rollback);
                            
                            if (!NT_SUCCESS(Status)) {
                                ERR("update_extent_level returned %08x\n", Status);
                                return Status;
                            }
                        }
                        
                        t->header.level = 0;
                    }
                } else if (t->size > Vcb->superblock.node_size - sizeof(tree_header)) {
                    TRACE("splitting overlarge tree (%x > %x)\n", t->size, Vcb->superblock.node_size - sizeof(tree_header));
                    Status = split_tree(Vcb, t);

                    if (!NT_SUCCESS(Status)) {
                        ERR("split_tree returned %08x\n", Status);
                        return Status;
                    }
                }
            }
            
            le = nextle;
        }
        
        if (!empty) {
            max_level = level;
        } else {
            TRACE("nothing found for level %u\n", level);
            break;
        }
    }
    
    min_size = (Vcb->superblock.node_size - sizeof(tree_header)) / 2;
    
    for (level = 0; level <= max_level; level++) {
        LIST_ENTRY* le;
        
        le = Vcb->trees.Flink;
    
        while (le != &Vcb->trees) {
            t = CONTAINING_RECORD(le, tree, list_entry);
            
            if (t->write && t->header.level == level && t->header.num_items > 0 && t->parent && t->size < min_size) {
                Status = try_tree_amalgamate(Vcb, t, Irp, rollback);
                if (!NT_SUCCESS(Status)) {
                    ERR("try_tree_amalgamate returned %08x\n", Status);
                    return Status;
                }
            }
            
            le = le->Flink;
        }
    }
    
    // simplify trees if top tree only has one entry
    
    if (done_deletions) {
        for (level = max_level; level > 0; level--) {
            LIST_ENTRY *le, *nextle;
            
            le = Vcb->trees.Flink;
            while (le != &Vcb->trees) {
                nextle = le->Flink;
                t = CONTAINING_RECORD(le, tree, list_entry);
                
                if (t->write && t->header.level == level) {
                    if (!t->parent && t->header.num_items == 1) {
                        LIST_ENTRY* le2 = t->itemlist.Flink;
                        tree_data* td;
                        tree* child_tree = NULL;

                        while (le2 != &t->itemlist) {
                            td = CONTAINING_RECORD(le2, tree_data, list_entry);
                            if (!td->ignore)
                                break;
                            le2 = le2->Flink;
                        }
                        
                        TRACE("deleting top-level tree in root %llx with one item\n", t->root->id);
                        
                        if (t->has_new_address) { // delete associated EXTENT_ITEM
                            Status = reduce_tree_extent(Vcb, t->new_address, t, Irp, rollback);
                            
                            if (!NT_SUCCESS(Status)) {
                                ERR("reduce_tree_extent returned %08x\n", Status);
                                return Status;
                            }
                            
                            t->has_new_address = FALSE;
                        } else if (t->has_address) {
                            Status = reduce_tree_extent(Vcb,t->header.address, t, Irp, rollback);
                            
                            if (!NT_SUCCESS(Status)) {
                                ERR("reduce_tree_extent returned %08x\n", Status);
                                return Status;
                            }
                            
                            t->has_address = FALSE;
                        }
                        
                        if (!td->treeholder.tree) { // load first item if not already loaded
                            KEY searchkey = {0,0,0};
                            traverse_ptr tp;
                            
                            Status = find_item(Vcb, t->root, &tp, &searchkey, FALSE, Irp);
                            if (!NT_SUCCESS(Status)) {
                                ERR("error - find_item returned %08x\n", Status);
                                return Status;
                            }
                        }
                        
                        child_tree = td->treeholder.tree;
                        
                        if (child_tree) {
                            child_tree->parent = NULL;
                            child_tree->paritem = NULL;
                        }
                        
                        t->root->root_item.bytes_used -= Vcb->superblock.node_size;

                        free_tree(t);
                        
                        if (child_tree)
                            child_tree->root->treeholder.tree = child_tree;
                    }
                }
                
                le = nextle;
            }
        }
    }
    
    return STATUS_SUCCESS;
}

static NTSTATUS remove_root_extents(device_extension* Vcb, root* r, tree_holder* th, UINT8 level, PIRP Irp, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    
    if (level > 0) {
        if (!th->tree) {
            Status = load_tree(Vcb, th->address, r, &th->tree, NULL, NULL);
            
            if (!NT_SUCCESS(Status)) {
                ERR("load_tree(%llx) returned %08x\n", th->address, Status);
                return Status;
            }
        }
        
        if (th->tree->header.level > 0) {
            LIST_ENTRY* le = th->tree->itemlist.Flink;
            
            while (le != &th->tree->itemlist) {
                tree_data* td = CONTAINING_RECORD(le, tree_data, list_entry);
                
                if (!td->ignore) {
                    Status = remove_root_extents(Vcb, r, &td->treeholder, th->tree->header.level - 1, Irp, rollback);
                    
                    if (!NT_SUCCESS(Status)) {
                        ERR("remove_root_extents returned %08x\n", Status);
                        return Status;
                    }
                }
                
                le = le->Flink;
            }
        }
    }
    
    if (!th->tree || th->tree->has_address) {
        Status = reduce_tree_extent(Vcb, th->address, NULL, Irp, rollback);
        
        if (!NT_SUCCESS(Status)) {
            ERR("reduce_tree_extent(%llx) returned %08x\n", th->address, Status);
            return Status;
        }
    }
    
    return STATUS_SUCCESS;
}

static NTSTATUS drop_root(device_extension* Vcb, root* r, PIRP Irp, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    KEY searchkey;
    traverse_ptr tp;
    
    Status = remove_root_extents(Vcb, r, &r->treeholder, r->root_item.root_level, Irp, rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("remove_root_extents returned %08x\n", Status);
        return Status;
    }
    
    // remove entry in uuid root (tree 9)
    if (Vcb->uuid_root) {
        RtlCopyMemory(&searchkey.obj_id, &r->root_item.uuid.uuid[0], sizeof(UINT64));
        searchkey.obj_type = TYPE_SUBVOL_UUID;
        RtlCopyMemory(&searchkey.offset, &r->root_item.uuid.uuid[sizeof(UINT64)], sizeof(UINT64));
        
        if (searchkey.obj_id != 0 || searchkey.offset != 0) {
            Status = find_item(Vcb, Vcb->uuid_root, &tp, &searchkey, FALSE, Irp);
            if (!NT_SUCCESS(Status)) {
                WARN("find_item returned %08x\n", Status);
            } else {
                if (!keycmp(&tp.item->key, &searchkey))
                    delete_tree_item(Vcb, &tp, rollback);
                else
                    WARN("could not find (%llx,%x,%llx) in uuid tree\n", searchkey.obj_id, searchkey.obj_type, searchkey.offset);
            }
        }
    }
    
    // delete ROOT_ITEM
    
    searchkey.obj_id = r->id;
    searchkey.obj_type = TYPE_ROOT_ITEM;
    searchkey.offset = 0xffffffffffffffff;
    
    Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("find_item returned %08x\n", Status);
        return Status;
    }
    
    if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == searchkey.obj_type)
        delete_tree_item(Vcb, &tp, rollback);
    else
        WARN("could not find (%llx,%x,%llx) in root_root\n", searchkey.obj_id, searchkey.obj_type, searchkey.offset);
    
    // delete items in tree cache
    
    free_trees_root(Vcb, r);
    
    return STATUS_SUCCESS;
}

static NTSTATUS drop_roots(device_extension* Vcb, PIRP Irp, LIST_ENTRY* rollback) {
    LIST_ENTRY *le = Vcb->drop_roots.Flink, *le2;
    NTSTATUS Status;
    
    while (le != &Vcb->drop_roots) {
        root* r = CONTAINING_RECORD(le, root, list_entry);
        
        le2 = le->Flink;
        
        Status = drop_root(Vcb, r, Irp, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("drop_root(%llx) returned %08x\n", r->id, Status);
            return Status;
        }
        
        le = le2;
    }
    
    return STATUS_SUCCESS;
}

static NTSTATUS create_chunk(device_extension* Vcb, chunk* c, PIRP Irp, LIST_ENTRY* rollback) {
    CHUNK_ITEM* ci;
    CHUNK_ITEM_STRIPE* cis;
    BLOCK_GROUP_ITEM* bgi;
    UINT16 i, factor;
    NTSTATUS Status;
    
    ci = ExAllocatePoolWithTag(PagedPool, c->size, ALLOC_TAG);
    if (!ci) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlCopyMemory(ci, c->chunk_item, c->size);
    
    if (!insert_tree_item(Vcb, Vcb->chunk_root, 0x100, TYPE_CHUNK_ITEM, c->offset, ci, c->size, NULL, Irp, rollback)) {
        ERR("insert_tree_item failed\n");
        ExFreePool(ci);
        return STATUS_INTERNAL_ERROR;
    }

    if (c->chunk_item->type & BLOCK_FLAG_SYSTEM) {
        Status = add_to_bootstrap(Vcb, 0x100, TYPE_CHUNK_ITEM, c->offset, ci, c->size);
        if (!NT_SUCCESS(Status)) {
            ERR("add_to_bootstrap returned %08x\n", Status);
            return Status;
        }
    }

    // add BLOCK_GROUP_ITEM to tree 2
    
    bgi = ExAllocatePoolWithTag(PagedPool, sizeof(BLOCK_GROUP_ITEM), ALLOC_TAG);
    if (!bgi) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    bgi->used = c->used;
    bgi->chunk_tree = 0x100;
    bgi->flags = c->chunk_item->type;
    
    if (!insert_tree_item(Vcb, Vcb->extent_root, c->offset, TYPE_BLOCK_GROUP_ITEM, c->chunk_item->size, bgi, sizeof(BLOCK_GROUP_ITEM), NULL, Irp, rollback)) {
        ERR("insert_tree_item failed\n");
        ExFreePool(bgi);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    if (c->chunk_item->type & BLOCK_FLAG_RAID0)
        factor = c->chunk_item->num_stripes;
    else if (c->chunk_item->type & BLOCK_FLAG_RAID10)
        factor = c->chunk_item->num_stripes / c->chunk_item->sub_stripes;
    else // SINGLE, DUPLICATE, RAID1
        factor = 1;

    // add DEV_EXTENTs to tree 4
    
    cis = (CHUNK_ITEM_STRIPE*)&c->chunk_item[1];
    
    for (i = 0; i < c->chunk_item->num_stripes; i++) {
        DEV_EXTENT* de;
        
        de = ExAllocatePoolWithTag(PagedPool, sizeof(DEV_EXTENT), ALLOC_TAG);
        if (!de) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        de->chunktree = Vcb->chunk_root->id;
        de->objid = 0x100;
        de->address = c->offset;
        de->length = c->chunk_item->size / factor;
        de->chunktree_uuid = Vcb->chunk_root->treeholder.tree->header.chunk_tree_uuid;

        if (!insert_tree_item(Vcb, Vcb->dev_root, c->devices[i]->devitem.dev_id, TYPE_DEV_EXTENT, cis[i].offset, de, sizeof(DEV_EXTENT), NULL, Irp, rollback)) {
            ERR("insert_tree_item failed\n");
            ExFreePool(de);
            return STATUS_INTERNAL_ERROR;
        }
        
        // FIXME - no point in calling this twice for the same device
        Status = update_dev_item(Vcb, c->devices[i], Irp, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("update_dev_item returned %08x\n", Status);
            return Status;
        }
    }
    
    c->created = FALSE;
    
    return STATUS_SUCCESS;
}

static void remove_from_bootstrap(device_extension* Vcb, UINT64 obj_id, UINT8 obj_type, UINT64 offset) {
    sys_chunk* sc2;
    LIST_ENTRY* le;

    le = Vcb->sys_chunks.Flink;
    while (le != &Vcb->sys_chunks) {
        sc2 = CONTAINING_RECORD(le, sys_chunk, list_entry);
        
        if (sc2->key.obj_id == obj_id && sc2->key.obj_type == obj_type && sc2->key.offset == offset) {
            RemoveEntryList(&sc2->list_entry);
            
            Vcb->superblock.n -= sizeof(KEY) + sc2->size;
            
            ExFreePool(sc2->data);
            ExFreePool(sc2);
            regen_bootstrap(Vcb);
            return;
        }
        
        le = le->Flink;
    }
}

static NTSTATUS drop_chunk(device_extension* Vcb, chunk* c, PIRP Irp, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    KEY searchkey;
    traverse_ptr tp;
    UINT64 i, factor;
    CHUNK_ITEM_STRIPE* cis;
    
    TRACE("dropping chunk %llx\n", c->offset);
    
    // remove free space cache
    if (c->cache) {
        c->cache->deleted = TRUE;
        
        flush_fcb(c->cache, TRUE, Irp, rollback);
        
        free_fcb(c->cache);
        
        searchkey.obj_id = FREE_SPACE_CACHE_ID;
        searchkey.obj_type = 0;
        searchkey.offset = c->offset;
        
        Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            return Status;
        }

        if (!keycmp(&tp.item->key, &searchkey)) {
            delete_tree_item(Vcb, &tp, rollback);
        }
    }
    
    if (c->chunk_item->type & BLOCK_FLAG_RAID0)
        factor = c->chunk_item->num_stripes;
    else if (c->chunk_item->type & BLOCK_FLAG_RAID10)
        factor = c->chunk_item->num_stripes / c->chunk_item->sub_stripes;
    else // SINGLE, DUPLICATE, RAID1
        factor = 1;
    
    cis = (CHUNK_ITEM_STRIPE*)&c->chunk_item[1];
    for (i = 0; i < c->chunk_item->num_stripes; i++) {
        if (!c->created) {
            // remove DEV_EXTENTs from tree 4
            searchkey.obj_id = cis[i].dev_id;
            searchkey.obj_type = TYPE_DEV_EXTENT;
            searchkey.offset = cis[i].offset;
            
            Status = find_item(Vcb, Vcb->dev_root, &tp, &searchkey, FALSE, Irp);
            if (!NT_SUCCESS(Status)) {
                ERR("error - find_item returned %08x\n", Status);
                return Status;
            }
            
            if (!keycmp(&tp.item->key, &searchkey)) {
                delete_tree_item(Vcb, &tp, rollback);
                
                if (tp.item->size >= sizeof(DEV_EXTENT)) {
                    DEV_EXTENT* de = (DEV_EXTENT*)tp.item->data;
                    
                    c->devices[i]->devitem.bytes_used -= de->length;
                    
                    space_list_add2(&c->devices[i]->space, NULL, cis[i].offset, de->length, rollback);
                }
            } else
                WARN("could not find (%llx,%x,%llx) in dev tree\n", searchkey.obj_id, searchkey.obj_type, searchkey.offset);
        } else {
            UINT64 len = c->chunk_item->size / factor;
            
            c->devices[i]->devitem.bytes_used -= len;
            space_list_add2(&c->devices[i]->space, NULL, cis[i].offset, len, rollback);
        }
    }
    
    // modify DEV_ITEMs in chunk tree
    for (i = 0; i < c->chunk_item->num_stripes; i++) {
        if (c->devices[i]) {
            UINT64 j;
            DEV_ITEM* di;
            
            searchkey.obj_id = 1;
            searchkey.obj_type = TYPE_DEV_ITEM;
            searchkey.offset = c->devices[i]->devitem.dev_id;
            
            Status = find_item(Vcb, Vcb->chunk_root, &tp, &searchkey, FALSE, Irp);
            if (!NT_SUCCESS(Status)) {
                ERR("error - find_item returned %08x\n", Status);
                return Status;
            }
            
            if (keycmp(&tp.item->key, &searchkey)) {
                ERR("error - could not find DEV_ITEM for device %llx\n", searchkey.offset);
                return STATUS_INTERNAL_ERROR;
            }
            
            delete_tree_item(Vcb, &tp, rollback);
            
            di = ExAllocatePoolWithTag(PagedPool, sizeof(DEV_ITEM), ALLOC_TAG);
            if (!di) {
                ERR("out of memory\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            
            RtlCopyMemory(di, &c->devices[i]->devitem, sizeof(DEV_ITEM));
            
            if (!insert_tree_item(Vcb, Vcb->chunk_root, 1, TYPE_DEV_ITEM, c->devices[i]->devitem.dev_id, di, sizeof(DEV_ITEM), NULL, Irp, rollback)) {
                ERR("insert_tree_item failed\n");
                return STATUS_INTERNAL_ERROR;
            }
            
            for (j = i + 1; j < c->chunk_item->num_stripes; j++) {
                if (c->devices[j] == c->devices[i])
                    c->devices[j] = NULL;
            }
        }
    }
    
    if (!c->created) {
        // remove CHUNK_ITEM from chunk tree
        searchkey.obj_id = 0x100;
        searchkey.obj_type = TYPE_CHUNK_ITEM;
        searchkey.offset = c->offset;
        
        Status = find_item(Vcb, Vcb->chunk_root, &tp, &searchkey, FALSE, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            return Status;
        }
        
        if (!keycmp(&tp.item->key, &searchkey))
            delete_tree_item(Vcb, &tp, rollback);
        else
            WARN("could not find CHUNK_ITEM for chunk %llx\n", c->offset);
        
        // remove BLOCK_GROUP_ITEM from extent tree
        searchkey.obj_id = c->offset;
        searchkey.obj_type = TYPE_BLOCK_GROUP_ITEM;
        searchkey.offset = 0xffffffffffffffff;
        
        Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            return Status;
        }
        
        if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == searchkey.obj_type)
            delete_tree_item(Vcb, &tp, rollback);
        else
            WARN("could not find BLOCK_GROUP_ITEM for chunk %llx\n", c->offset);
    }
    
    if (c->chunk_item->type & BLOCK_FLAG_SYSTEM)
        remove_from_bootstrap(Vcb, 0x100, TYPE_CHUNK_ITEM, c->offset);
    
    RemoveEntryList(&c->list_entry);
    
    if (c->list_entry_changed.Flink)
        RemoveEntryList(&c->list_entry_changed);
    
    ExFreePool(c->chunk_item);
    ExFreePool(c->devices);
    
    while (!IsListEmpty(&c->space)) {
        space* s = CONTAINING_RECORD(c->space.Flink, space, list_entry);
        
        RemoveEntryList(&s->list_entry);
        ExFreePool(s);
    }
    
    while (!IsListEmpty(&c->deleting)) {
        space* s = CONTAINING_RECORD(c->deleting.Flink, space, list_entry);
        
        RemoveEntryList(&s->list_entry);
        ExFreePool(s);
    }
    
    ExDeleteResourceLite(&c->lock);
    ExDeleteResourceLite(&c->changed_extents_lock);

    ExFreePool(c);
    
    return STATUS_SUCCESS;
}

static NTSTATUS update_chunks(device_extension* Vcb, PIRP Irp, LIST_ENTRY* rollback) {
    LIST_ENTRY *le = Vcb->chunks_changed.Flink, *le2;
    NTSTATUS Status;
    UINT64 used_minus_cache;
    
    ExAcquireResourceExclusiveLite(&Vcb->chunk_lock, TRUE);
    
    // FIXME - do tree chunks before data chunks
    
    while (le != &Vcb->chunks_changed) {
        chunk* c = CONTAINING_RECORD(le, chunk, list_entry_changed);
        
        le2 = le->Flink;
        
        ExAcquireResourceExclusiveLite(&c->lock, TRUE);
        
        used_minus_cache = c->used;
        
        // subtract self-hosted cache
        if (used_minus_cache > 0 && c->chunk_item->type & BLOCK_FLAG_DATA && c->cache && c->cache->inode_item.st_size == c->used) {
            LIST_ENTRY* le3;
            
            le3 = c->cache->extents.Flink;
            while (le3 != &c->cache->extents) {
                extent* ext = CONTAINING_RECORD(le3, extent, list_entry);
                EXTENT_DATA* ed = ext->data;
                
                if (!ext->ignore) {
                    if (ext->datalen < sizeof(EXTENT_DATA)) {
                        ERR("extent %llx was %u bytes, expected at least %u\n", ext->offset, ext->datalen, sizeof(EXTENT_DATA));
                        break;
                    }
                    
                    if (ed->type == EXTENT_TYPE_REGULAR || ed->type == EXTENT_TYPE_PREALLOC) {
                        EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ed->data;
                        
                        if (ext->datalen < sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2)) {
                            ERR("extent %llx was %u bytes, expected at least %u\n", ext->offset, ext->datalen,
                                sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2));
                            break;
                        }
                        
                        if (ed2->size != 0 && ed2->address >= c->offset && ed2->address + ed2->size <= c->offset + c->chunk_item->size)
                            used_minus_cache -= ed2->size;
                    }
                }
                
                le3 = le3->Flink;
            }
        }
        
        if (used_minus_cache == 0) {
            Status = drop_chunk(Vcb, c, Irp, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("drop_chunk returned %08x\n", Status);
                ExReleaseResourceLite(&c->lock);
                ExReleaseResourceLite(&Vcb->chunk_lock);
                return Status;
            }
        } else if (c->created) {
            Status = create_chunk(Vcb, c, Irp, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("create_chunk returned %08x\n", Status);
                ExReleaseResourceLite(&c->lock);
                ExReleaseResourceLite(&Vcb->chunk_lock);
                return Status;
            }
        }
        
        if (used_minus_cache > 0)
            ExReleaseResourceLite(&c->lock);

        le = le2;
    }
    
    ExReleaseResourceLite(&Vcb->chunk_lock);
    
    return STATUS_SUCCESS;
}

static NTSTATUS STDCALL set_xattr(device_extension* Vcb, root* subvol, UINT64 inode, char* name, UINT32 crc32, UINT8* data, UINT16 datalen, PIRP Irp, LIST_ENTRY* rollback) {
    KEY searchkey;
    traverse_ptr tp;
    ULONG xasize, maxlen;
    DIR_ITEM* xa;
    NTSTATUS Status;
    
    TRACE("(%p, %llx, %llx, %s, %08x, %p, %u)\n", Vcb, subvol->id, inode, name, crc32, data, datalen);
    
    searchkey.obj_id = inode;
    searchkey.obj_type = TYPE_XATTR_ITEM;
    searchkey.offset = crc32;
    
    Status = find_item(Vcb, subvol, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    xasize = sizeof(DIR_ITEM) - 1 + (ULONG)strlen(name) + datalen;
    maxlen = Vcb->superblock.node_size - sizeof(tree_header) - sizeof(leaf_node);
    
    if (!keycmp(&tp.item->key, &searchkey)) { // key exists
        UINT8* newdata;
        ULONG size = tp.item->size;
        
        xa = (DIR_ITEM*)tp.item->data;
        
        if (tp.item->size < sizeof(DIR_ITEM)) {
            ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(DIR_ITEM));
        } else {
            while (TRUE) {
                ULONG oldxasize;
                
                if (size < sizeof(DIR_ITEM) || size < sizeof(DIR_ITEM) - 1 + xa->m + xa->n) {
                    ERR("(%llx,%x,%llx) was truncated\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
                    break;
                }
                
                oldxasize = sizeof(DIR_ITEM) - 1 + xa->m + xa->n;
                
                if (xa->n == strlen(name) && RtlCompareMemory(name, xa->name, xa->n) == xa->n) {
                    UINT64 pos;
                    
                    // replace
                    
                    if (tp.item->size + xasize - oldxasize > maxlen) {
                        ERR("DIR_ITEM would be over maximum size (%u + %u - %u > %u)\n", tp.item->size, xasize, oldxasize, maxlen);
                        return STATUS_INTERNAL_ERROR;
                    }
                    
                    newdata = ExAllocatePoolWithTag(PagedPool, tp.item->size + xasize - oldxasize, ALLOC_TAG);
                    if (!newdata) {
                        ERR("out of memory\n");
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }
                    
                    pos = (UINT8*)xa - tp.item->data;
                    if (pos + oldxasize < tp.item->size) { // copy after changed xattr
                        RtlCopyMemory(newdata + pos + xasize, tp.item->data + pos + oldxasize, tp.item->size - pos - oldxasize);
                    }
                    
                    if (pos > 0) { // copy before changed xattr
                        RtlCopyMemory(newdata, tp.item->data, pos);
                        xa = (DIR_ITEM*)(newdata + pos);
                    } else
                        xa = (DIR_ITEM*)newdata;
                    
                    xa->key.obj_id = 0;
                    xa->key.obj_type = 0;
                    xa->key.offset = 0;
                    xa->transid = Vcb->superblock.generation;
                    xa->m = datalen;
                    xa->n = (UINT16)strlen(name);
                    xa->type = BTRFS_TYPE_EA;
                    RtlCopyMemory(xa->name, name, strlen(name));
                    RtlCopyMemory(xa->name + strlen(name), data, datalen);
                    
                    delete_tree_item(Vcb, &tp, rollback);
                    insert_tree_item(Vcb, subvol, inode, TYPE_XATTR_ITEM, crc32, newdata, tp.item->size + xasize - oldxasize, NULL, Irp, rollback);
                    
                    break;
                }
                
                if ((UINT8*)xa - (UINT8*)tp.item->data + oldxasize >= size) {
                    // not found, add to end of data
                    
                    if (tp.item->size + xasize > maxlen) {
                        ERR("DIR_ITEM would be over maximum size (%u + %u > %u)\n", tp.item->size, xasize, maxlen);
                        return STATUS_INTERNAL_ERROR;
                    }
                    
                    newdata = ExAllocatePoolWithTag(PagedPool, tp.item->size + xasize, ALLOC_TAG);
                    if (!newdata) {
                        ERR("out of memory\n");
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }
                    
                    RtlCopyMemory(newdata, tp.item->data, tp.item->size);
                    
                    xa = (DIR_ITEM*)((UINT8*)newdata + tp.item->size);
                    xa->key.obj_id = 0;
                    xa->key.obj_type = 0;
                    xa->key.offset = 0;
                    xa->transid = Vcb->superblock.generation;
                    xa->m = datalen;
                    xa->n = (UINT16)strlen(name);
                    xa->type = BTRFS_TYPE_EA;
                    RtlCopyMemory(xa->name, name, strlen(name));
                    RtlCopyMemory(xa->name + strlen(name), data, datalen);
                    
                    delete_tree_item(Vcb, &tp, rollback);
                    insert_tree_item(Vcb, subvol, inode, TYPE_XATTR_ITEM, crc32, newdata, tp.item->size + xasize, NULL, Irp, rollback);
                    
                    break;
                } else {
                    xa = (DIR_ITEM*)&xa->name[xa->m + xa->n];
                    size -= oldxasize;
                }
            }
        }
    } else {
        if (xasize > maxlen) {
            ERR("DIR_ITEM would be over maximum size (%u > %u)\n", xasize, maxlen);
            return STATUS_INTERNAL_ERROR;
        }
        
        xa = ExAllocatePoolWithTag(PagedPool, xasize, ALLOC_TAG);
        if (!xa) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        xa->key.obj_id = 0;
        xa->key.obj_type = 0;
        xa->key.offset = 0;
        xa->transid = Vcb->superblock.generation;
        xa->m = datalen;
        xa->n = (UINT16)strlen(name);
        xa->type = BTRFS_TYPE_EA;
        RtlCopyMemory(xa->name, name, strlen(name));
        RtlCopyMemory(xa->name + strlen(name), data, datalen);
        
        insert_tree_item(Vcb, subvol, inode, TYPE_XATTR_ITEM, crc32, xa, xasize, NULL, Irp, rollback);
    }
    
    return STATUS_SUCCESS;
}

static BOOL STDCALL delete_xattr(device_extension* Vcb, root* subvol, UINT64 inode, char* name, UINT32 crc32, PIRP Irp, LIST_ENTRY* rollback) {
    KEY searchkey;
    traverse_ptr tp;
    DIR_ITEM* xa;
    NTSTATUS Status;
    
    TRACE("(%p, %llx, %llx, %s, %08x)\n", Vcb, subvol->id, inode, name, crc32);
    
    searchkey.obj_id = inode;
    searchkey.obj_type = TYPE_XATTR_ITEM;
    searchkey.offset = crc32;
    
    Status = find_item(Vcb, subvol, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return FALSE;
    }
    
    if (!keycmp(&tp.item->key, &searchkey)) { // key exists
        ULONG size = tp.item->size;
        
        if (tp.item->size < sizeof(DIR_ITEM)) {
            ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(DIR_ITEM));
            
            return FALSE;
        } else {
            xa = (DIR_ITEM*)tp.item->data;
            
            while (TRUE) {
                ULONG oldxasize;
                
                if (size < sizeof(DIR_ITEM) || size < sizeof(DIR_ITEM) - 1 + xa->m + xa->n) {
                    ERR("(%llx,%x,%llx) was truncated\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
                        
                    return FALSE;
                }
                
                oldxasize = sizeof(DIR_ITEM) - 1 + xa->m + xa->n;
                
                if (xa->n == strlen(name) && RtlCompareMemory(name, xa->name, xa->n) == xa->n) {
                    ULONG newsize;
                    UINT8 *newdata, *dioff;
                    
                    newsize = tp.item->size - (sizeof(DIR_ITEM) - 1 + xa->n + xa->m);
                    
                    delete_tree_item(Vcb, &tp, rollback);
                    
                    if (newsize == 0) {
                        TRACE("xattr %s deleted\n", name);
                        
                        return TRUE;
                    }

                    // FIXME - deleting collisions almost certainly works, but we should test it properly anyway
                    newdata = ExAllocatePoolWithTag(PagedPool, newsize, ALLOC_TAG);
                    if (!newdata) {
                        ERR("out of memory\n");
                        return FALSE;
                    }

                    if ((UINT8*)xa > tp.item->data) {
                        RtlCopyMemory(newdata, tp.item->data, (UINT8*)xa - tp.item->data);
                        dioff = newdata + ((UINT8*)xa - tp.item->data);
                    } else {
                        dioff = newdata;
                    }
                    
                    if ((UINT8*)&xa->name[xa->n+xa->m] - tp.item->data < tp.item->size)
                        RtlCopyMemory(dioff, &xa->name[xa->n+xa->m], tp.item->size - ((UINT8*)&xa->name[xa->n+xa->m] - tp.item->data));
                    
                    insert_tree_item(Vcb, subvol, inode, TYPE_XATTR_ITEM, crc32, newdata, newsize, NULL, Irp, rollback);
                    
                        
                    return TRUE;
                }
                
                if (xa->m + xa->n >= size) { // FIXME - test this works
                    WARN("xattr %s not found\n", name);

                    return FALSE;
                } else {
                    xa = (DIR_ITEM*)&xa->name[xa->m + xa->n];
                    size -= oldxasize;
                }
            }
        }
    } else {
        WARN("xattr %s not found\n", name);
        
        return FALSE;
    }
}

static NTSTATUS insert_sparse_extent(fcb* fcb, UINT64 start, UINT64 length, PIRP Irp, LIST_ENTRY* rollback) {
    EXTENT_DATA* ed;
    EXTENT_DATA2* ed2;
    
    TRACE("((%llx, %llx), %llx, %llx)\n", fcb->subvol->id, fcb->inode, start, length);
    
    ed = ExAllocatePoolWithTag(PagedPool, sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2), ALLOC_TAG);
    if (!ed) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    ed->generation = fcb->Vcb->superblock.generation;
    ed->decoded_size = length;
    ed->compression = BTRFS_COMPRESSION_NONE;
    ed->encryption = BTRFS_ENCRYPTION_NONE;
    ed->encoding = BTRFS_ENCODING_NONE;
    ed->type = EXTENT_TYPE_REGULAR;
    
    ed2 = (EXTENT_DATA2*)ed->data;
    ed2->address = 0;
    ed2->size = 0;
    ed2->offset = 0;
    ed2->num_bytes = length;
    
    if (!insert_tree_item(fcb->Vcb, fcb->subvol, fcb->inode, TYPE_EXTENT_DATA, start, ed, sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2), NULL, Irp, rollback)) {
        ERR("insert_tree_item failed\n");
        return STATUS_INTERNAL_ERROR;
    }

    return STATUS_SUCCESS;
}

void flush_fcb(fcb* fcb, BOOL cache, PIRP Irp, LIST_ENTRY* rollback) {
    traverse_ptr tp;
    KEY searchkey;
    NTSTATUS Status;
    INODE_ITEM* ii;
    UINT64 ii_offset;
#ifdef DEBUG_PARANOID
    UINT64 old_size = 0;
    BOOL extents_changed;
#endif
    
//     ExAcquireResourceExclusiveLite(fcb->Header.Resource, TRUE);
    
    while (!IsListEmpty(&fcb->index_list)) {
        LIST_ENTRY* le = RemoveHeadList(&fcb->index_list);
        index_entry* ie = CONTAINING_RECORD(le, index_entry, list_entry);

        if (ie->utf8.Buffer) ExFreePool(ie->utf8.Buffer);
        if (ie->filepart_uc.Buffer) ExFreePool(ie->filepart_uc.Buffer);
        ExFreePool(ie);
    }
    
    fcb->index_loaded = FALSE;
    
    if (fcb->ads) {
        if (fcb->deleted)
            delete_xattr(fcb->Vcb, fcb->subvol, fcb->inode, fcb->adsxattr.Buffer, fcb->adshash, Irp, rollback);
        else {
            Status = set_xattr(fcb->Vcb, fcb->subvol, fcb->inode, fcb->adsxattr.Buffer, fcb->adshash, (UINT8*)fcb->adsdata.Buffer, fcb->adsdata.Length, Irp, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("set_xattr returned %08x\n", Status);
                goto end;
            }
        }
        goto end;
    }
    
#ifdef DEBUG_PARANOID
    extents_changed = fcb->extents_changed;
#endif
    
    if (fcb->extents_changed) {
        BOOL b;
        traverse_ptr next_tp;
        LIST_ENTRY* le;
        BOOL prealloc = FALSE, extents_inline = FALSE;
        UINT64 last_end;
        
        // delete ignored extent items
        le = fcb->extents.Flink;
        while (le != &fcb->extents) {
            LIST_ENTRY* le2 = le->Flink;
            extent* ext = CONTAINING_RECORD(le, extent, list_entry);
            
            if (ext->ignore) {
                RemoveEntryList(&ext->list_entry);
                ExFreePool(ext->data);
                ExFreePool(ext);
            }
            
            le = le2;
        }
        
        le = fcb->extents.Flink;
        while (le != &fcb->extents) {
            LIST_ENTRY* le2 = le->Flink;
            extent* ext = CONTAINING_RECORD(le, extent, list_entry);
            
            if ((ext->data->type == EXTENT_TYPE_REGULAR || ext->data->type == EXTENT_TYPE_PREALLOC) && le->Flink != &fcb->extents) {
                extent* nextext = CONTAINING_RECORD(le->Flink, extent, list_entry);
                    
                if (ext->data->type == nextext->data->type) {
                    EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ext->data->data;
                    EXTENT_DATA2* ned2 = (EXTENT_DATA2*)nextext->data->data;
                    
                    if (ed2->size != 0 && ed2->address == ned2->address && ed2->size == ned2->size &&
                        nextext->offset == ext->offset + ed2->num_bytes && ned2->offset == ed2->offset + ed2->num_bytes) {
                        chunk* c;
                    
                        ext->data->generation = fcb->Vcb->superblock.generation;
                        ed2->num_bytes += ned2->num_bytes;
                    
                        RemoveEntryList(&nextext->list_entry);
                        ExFreePool(nextext->data);
                        ExFreePool(nextext);
                    
                        c = get_chunk_from_address(fcb->Vcb, ed2->address);
                            
                        if (!c) {
                            ERR("get_chunk_from_address(%llx) failed\n", ed2->address);
                        } else {
                            Status = update_changed_extent_ref(fcb->Vcb, c, ed2->address, ed2->size, fcb->subvol->id, fcb->inode, ext->offset - ed2->offset, -1,
                                                               fcb->inode_item.flags & BTRFS_INODE_NODATASUM, ed2->size, Irp);
                            if (!NT_SUCCESS(Status)) {
                                ERR("update_changed_extent_ref returned %08x\n", Status);
                                goto end;
                            }
                        }
                    
                        le2 = le;
                    }
                }
            }
            
            le = le2;
        }
        
        // delete existing EXTENT_DATA items
        
        searchkey.obj_id = fcb->inode;
        searchkey.obj_type = TYPE_EXTENT_DATA;
        searchkey.offset = 0;
        
        Status = find_item(fcb->Vcb, fcb->subvol, &tp, &searchkey, FALSE, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            goto end;
        }
        
        do {
            if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == searchkey.obj_type)
                delete_tree_item(fcb->Vcb, &tp, rollback);
            
            b = find_next_item(fcb->Vcb, &tp, &next_tp, FALSE, Irp);
            
            if (b) {
                tp = next_tp;
                
                if (tp.item->key.obj_id > searchkey.obj_id || (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type > searchkey.obj_type))
                    break;
            }
        } while (b);
        
        if (!fcb->deleted) {
            // add new EXTENT_DATAs
            
            last_end = 0;
            
            le = fcb->extents.Flink;
            while (le != &fcb->extents) {
                extent* ext = CONTAINING_RECORD(le, extent, list_entry);
                EXTENT_DATA* ed;
                
                if (!(fcb->Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_NO_HOLES) && ext->offset > last_end) {
                    Status = insert_sparse_extent(fcb, last_end, ext->offset - last_end, Irp, rollback);
                    if (!NT_SUCCESS(Status)) {
                        ERR("insert_sparse_extent returned %08x\n", Status);
                        goto end;
                    }
                }
                    
                ed = ExAllocatePoolWithTag(PagedPool, ext->datalen, ALLOC_TAG);
                if (!ed) {
                    ERR("out of memory\n");
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto end;
                }
                
                RtlCopyMemory(ed, ext->data, ext->datalen);
                
                if (!insert_tree_item(fcb->Vcb, fcb->subvol, fcb->inode, TYPE_EXTENT_DATA, ext->offset, ed, ext->datalen, NULL, Irp, rollback)) {
                    ERR("insert_tree_item failed\n");
                    goto end;
                }
                
                if (ext->datalen >= sizeof(EXTENT_DATA) && ed->type == EXTENT_TYPE_PREALLOC)
                    prealloc = TRUE;
                
                if (ext->datalen >= sizeof(EXTENT_DATA) && ed->type == EXTENT_TYPE_INLINE)
                    extents_inline = TRUE;
                
                if (!(fcb->Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_NO_HOLES)) {
                    if (ed->type == EXTENT_TYPE_INLINE)
                        last_end = ext->offset + ed->decoded_size;
                    else {
                        EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ed->data;
                        
                        last_end = ext->offset + ed2->num_bytes;
                    }
                }
                
                le = le->Flink;
            }
            
            if (!(fcb->Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_NO_HOLES) && !extents_inline &&
                sector_align(fcb->inode_item.st_size, fcb->Vcb->superblock.sector_size) > last_end) {
                Status = insert_sparse_extent(fcb, last_end, sector_align(fcb->inode_item.st_size, fcb->Vcb->superblock.sector_size) - last_end, Irp, rollback);
                if (!NT_SUCCESS(Status)) {
                    ERR("insert_sparse_extent returned %08x\n", Status);
                    goto end;
                }
            }
            
            // update prealloc flag in INODE_ITEM
            
            if (!prealloc)
                fcb->inode_item.flags &= ~BTRFS_INODE_PREALLOC;
            else
                fcb->inode_item.flags |= BTRFS_INODE_PREALLOC;
        }
        
        fcb->extents_changed = FALSE;
    }
    
    if (!fcb->created || cache) {
        searchkey.obj_id = fcb->inode;
        searchkey.obj_type = TYPE_INODE_ITEM;
        searchkey.offset = 0xffffffffffffffff;
        
        Status = find_item(fcb->Vcb, fcb->subvol, &tp, &searchkey, FALSE, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            goto end;
        }
        
        if (tp.item->key.obj_id != searchkey.obj_id || tp.item->key.obj_type != searchkey.obj_type) {
            if (cache) {
                ii = ExAllocatePoolWithTag(PagedPool, sizeof(INODE_ITEM), ALLOC_TAG);
                if (!ii) {
                    ERR("out of memory\n");
                    goto end;
                }
                
                RtlCopyMemory(ii, &fcb->inode_item, sizeof(INODE_ITEM));
                
                if (!insert_tree_item(fcb->Vcb, fcb->subvol, fcb->inode, TYPE_INODE_ITEM, 0, ii, sizeof(INODE_ITEM), NULL, Irp, rollback)) {
                    ERR("insert_tree_item failed\n");
                    goto end;
                }
                
                ii_offset = 0;
            } else {
                ERR("could not find INODE_ITEM for inode %llx in subvol %llx\n", fcb->inode, fcb->subvol->id);
                goto end;
            }
        } else {
#ifdef DEBUG_PARANOID
            INODE_ITEM* ii2 = (INODE_ITEM*)tp.item->data;
            
            old_size = ii2->st_size;
#endif
            
            ii_offset = tp.item->key.offset;
        }
        
        if (!cache)
            delete_tree_item(fcb->Vcb, &tp, rollback);
        else {
            searchkey.obj_id = fcb->inode;
            searchkey.obj_type = TYPE_INODE_ITEM;
            searchkey.offset = ii_offset;
            
            Status = find_item(fcb->Vcb, fcb->subvol, &tp, &searchkey, FALSE, Irp);
            if (!NT_SUCCESS(Status)) {
                ERR("error - find_item returned %08x\n", Status);
                goto end;
            }
            
            if (keycmp(&tp.item->key, &searchkey)) {
                ERR("could not find INODE_ITEM for inode %llx in subvol %llx\n", fcb->inode, fcb->subvol->id);
                goto end;
            } else
                RtlCopyMemory(tp.item->data, &fcb->inode_item, min(tp.item->size, sizeof(INODE_ITEM)));
        }
    } else
        ii_offset = 0;
    
#ifdef DEBUG_PARANOID
    if (!extents_changed && fcb->type != BTRFS_TYPE_DIRECTORY && old_size != fcb->inode_item.st_size) {
        ERR("error - size has changed but extents not marked as changed\n");
        int3;
    }
#endif
    
    fcb->created = FALSE;
        
    if (fcb->deleted) {
        traverse_ptr tp2;
        
        // delete XATTR_ITEMs
        
        searchkey.obj_id = fcb->inode;
        searchkey.obj_type = TYPE_XATTR_ITEM;
        searchkey.offset = 0;
        
        Status = find_item(fcb->Vcb, fcb->subvol, &tp, &searchkey, FALSE, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            goto end;
        }
    
        while (find_next_item(fcb->Vcb, &tp, &tp2, FALSE, Irp)) {
            tp = tp2;
            
            if (tp.item->key.obj_id == fcb->inode) {
                // FIXME - do metadata thing here too?
                if (tp.item->key.obj_type == TYPE_XATTR_ITEM) {
                    delete_tree_item(fcb->Vcb, &tp, rollback);
                    TRACE("deleting (%llx,%x,%llx)\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
                }
            } else
                break;
        }
        
        goto end;
    }
    
    if (!cache) {
        ii = ExAllocatePoolWithTag(PagedPool, sizeof(INODE_ITEM), ALLOC_TAG);
        if (!ii) {
            ERR("out of memory\n");
            goto end;
        }
        
        RtlCopyMemory(ii, &fcb->inode_item, sizeof(INODE_ITEM));
        
        if (!insert_tree_item(fcb->Vcb, fcb->subvol, fcb->inode, TYPE_INODE_ITEM, ii_offset, ii, sizeof(INODE_ITEM), NULL, Irp, rollback)) {
            ERR("insert_tree_item failed\n");
            goto end;
        }
    }
    
    if (fcb->sd_dirty) {
        Status = set_xattr(fcb->Vcb, fcb->subvol, fcb->inode, EA_NTACL, EA_NTACL_HASH, (UINT8*)fcb->sd, RtlLengthSecurityDescriptor(fcb->sd), Irp, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("set_xattr returned %08x\n", Status);
        }
        
        fcb->sd_dirty = FALSE;
    }
    
    if (fcb->atts_changed) {
        if (!fcb->atts_deleted) {
            char val[64];
            
            TRACE("inserting new DOSATTRIB xattr\n");
            sprintf(val, "0x%lx", fcb->atts);
        
            Status = set_xattr(fcb->Vcb, fcb->subvol, fcb->inode, EA_DOSATTRIB, EA_DOSATTRIB_HASH, (UINT8*)val, strlen(val), Irp, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("set_xattr returned %08x\n", Status);
                goto end;
            }
        } else
            delete_xattr(fcb->Vcb, fcb->subvol, fcb->inode, EA_DOSATTRIB, EA_DOSATTRIB_HASH, Irp, rollback);
        
        fcb->atts_changed = FALSE;
        fcb->atts_deleted = FALSE;
    }
    
    if (fcb->reparse_xattr_changed) {
        if (fcb->reparse_xattr.Buffer && fcb->reparse_xattr.Length > 0) {
            Status = set_xattr(fcb->Vcb, fcb->subvol, fcb->inode, EA_REPARSE, EA_REPARSE_HASH, (UINT8*)fcb->reparse_xattr.Buffer, fcb->reparse_xattr.Length, Irp, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("set_xattr returned %08x\n", Status);
                goto end;
            }
        } else
            delete_xattr(fcb->Vcb, fcb->subvol, fcb->inode, EA_REPARSE, EA_REPARSE_HASH, Irp, rollback);
        
        fcb->reparse_xattr_changed = FALSE;
    }
    
end:
    fcb->dirty = FALSE;
    
//     ExReleaseResourceLite(fcb->Header.Resource);
    return;
}

static NTSTATUS delete_root_ref(device_extension* Vcb, UINT64 subvolid, UINT64 parsubvolid, UINT64 parinode, PANSI_STRING utf8, PIRP Irp, LIST_ENTRY* rollback) {
    KEY searchkey;
    traverse_ptr tp;
    NTSTATUS Status;
    
    searchkey.obj_id = parsubvolid;
    searchkey.obj_type = TYPE_ROOT_REF;
    searchkey.offset = subvolid;
    
    Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    if (!keycmp(&searchkey, &tp.item->key)) {
        if (tp.item->size < sizeof(ROOT_REF)) {
            ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(ROOT_REF));
            return STATUS_INTERNAL_ERROR;
        } else {
            ROOT_REF* rr;
            ULONG len;
            
            rr = (ROOT_REF*)tp.item->data;
            len = tp.item->size;
            
            do {
                ULONG itemlen;
                
                if (len < sizeof(ROOT_REF) || len < sizeof(ROOT_REF) - 1 + rr->n) {
                    ERR("(%llx,%x,%llx) was truncated\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
                    break;
                }
                
                itemlen = sizeof(ROOT_REF) - sizeof(char) + rr->n;
                
                if (rr->dir == parinode && rr->n == utf8->Length && RtlCompareMemory(rr->name, utf8->Buffer, rr->n) == rr->n) {
                    ULONG newlen = tp.item->size - itemlen;
                    
                    delete_tree_item(Vcb, &tp, rollback);
                    
                    if (newlen == 0) {
                        TRACE("deleting (%llx,%x,%llx)\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
                    } else {
                        UINT8 *newrr = ExAllocatePoolWithTag(PagedPool, newlen, ALLOC_TAG), *rroff;
                        
                        if (!newrr) {
                            ERR("out of memory\n");
                            return STATUS_INSUFFICIENT_RESOURCES;
                        }
                        
                        TRACE("modifying (%llx,%x,%llx)\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);

                        if ((UINT8*)rr > tp.item->data) {
                            RtlCopyMemory(newrr, tp.item->data, (UINT8*)rr - tp.item->data);
                            rroff = newrr + ((UINT8*)rr - tp.item->data);
                        } else {
                            rroff = newrr;
                        }
                        
                        if ((UINT8*)&rr->name[rr->n] - tp.item->data < tp.item->size)
                            RtlCopyMemory(rroff, &rr->name[rr->n], tp.item->size - ((UINT8*)&rr->name[rr->n] - tp.item->data));
                        
                        insert_tree_item(Vcb, Vcb->root_root, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, newrr, newlen, NULL, Irp, rollback);
                    }
                    
                    break;
                }
                
                if (len > itemlen) {
                    len -= itemlen;
                    rr = (ROOT_REF*)&rr->name[rr->n];
                } else
                    break;
            } while (len > 0);
        }
    } else {
        WARN("could not find ROOT_REF entry for subvol %llx in %llx\n", searchkey.offset, searchkey.obj_id);
        return STATUS_NOT_FOUND;
    }
    
    return STATUS_SUCCESS;
}

static NTSTATUS add_root_ref(device_extension* Vcb, UINT64 subvolid, UINT64 parsubvolid, ROOT_REF* rr, PIRP Irp, LIST_ENTRY* rollback) {
    KEY searchkey;
    traverse_ptr tp;
    NTSTATUS Status;
    
    searchkey.obj_id = parsubvolid;
    searchkey.obj_type = TYPE_ROOT_REF;
    searchkey.offset = subvolid;
    
    Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    if (!keycmp(&searchkey, &tp.item->key)) {
        ULONG rrsize = tp.item->size + sizeof(ROOT_REF) - 1 + rr->n;
        UINT8* rr2;
        
        rr2 = ExAllocatePoolWithTag(PagedPool, rrsize, ALLOC_TAG);
        if (!rr2) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        if (tp.item->size > 0)
            RtlCopyMemory(rr2, tp.item->data, tp.item->size);
        
        RtlCopyMemory(rr2 + tp.item->size, rr, sizeof(ROOT_REF) - 1 + rr->n);
        ExFreePool(rr);
        
        delete_tree_item(Vcb, &tp, rollback);
        
        if (!insert_tree_item(Vcb, Vcb->root_root, searchkey.obj_id, searchkey.obj_type, searchkey.offset, rr2, rrsize, NULL, Irp, rollback)) {
            ERR("error - failed to insert item\n");
            ExFreePool(rr2);
            return STATUS_INTERNAL_ERROR;
        }
    } else {
        if (!insert_tree_item(Vcb, Vcb->root_root, searchkey.obj_id, searchkey.obj_type, searchkey.offset, rr, sizeof(ROOT_REF) - 1 + rr->n, NULL, Irp, rollback)) {
            ERR("error - failed to insert item\n");
            ExFreePool(rr);
            return STATUS_INTERNAL_ERROR;
        }
    }
    
    return STATUS_SUCCESS;
}

static NTSTATUS STDCALL update_root_backref(device_extension* Vcb, UINT64 subvolid, UINT64 parsubvolid, PIRP Irp, LIST_ENTRY* rollback) {
    KEY searchkey;
    traverse_ptr tp;
    UINT8* data;
    ULONG datalen;
    NTSTATUS Status;
    
    searchkey.obj_id = parsubvolid;
    searchkey.obj_type = TYPE_ROOT_REF;
    searchkey.offset = subvolid;
    
    Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    if (!keycmp(&tp.item->key, &searchkey) && tp.item->size > 0) {
        datalen = tp.item->size;
        
        data = ExAllocatePoolWithTag(PagedPool, datalen, ALLOC_TAG);
        if (!data) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        RtlCopyMemory(data, tp.item->data, datalen);
    } else {
        datalen = 0;
    }
    
    searchkey.obj_id = subvolid;
    searchkey.obj_type = TYPE_ROOT_BACKREF;
    searchkey.offset = parsubvolid;
    
    Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        
        if (datalen > 0)
            ExFreePool(data);
        
        return Status;
    }
    
    if (!keycmp(&tp.item->key, &searchkey))
        delete_tree_item(Vcb, &tp, rollback);
    
    if (datalen > 0) {
        if (!insert_tree_item(Vcb, Vcb->root_root, subvolid, TYPE_ROOT_BACKREF, parsubvolid, data, datalen, NULL, Irp, rollback)) {
            ERR("error - failed to insert item\n");
            ExFreePool(data);
            return STATUS_INTERNAL_ERROR;
        }
    }
    
    return STATUS_SUCCESS;
}

static NTSTATUS flush_fileref(file_ref* fileref, PIRP Irp, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    
    // if fileref created and then immediately deleted, do nothing
    if (fileref->created && fileref->deleted) {
        fileref->dirty = FALSE;
        return STATUS_SUCCESS;
    }
    
    if (fileref->fcb->ads) {
        fileref->dirty = FALSE;
        return STATUS_SUCCESS;
    }
    
    if (fileref->created) {
        ULONG disize;
        DIR_ITEM *di, *di2;
        UINT32 crc32;
        
        crc32 = calc_crc32c(0xfffffffe, (UINT8*)fileref->utf8.Buffer, fileref->utf8.Length);
        
        disize = sizeof(DIR_ITEM) - 1 + fileref->utf8.Length;
        di = ExAllocatePoolWithTag(PagedPool, disize, ALLOC_TAG);
        if (!di) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        if (fileref->parent->fcb->subvol == fileref->fcb->subvol) {
            di->key.obj_id = fileref->fcb->inode;
            di->key.obj_type = TYPE_INODE_ITEM;
            di->key.offset = 0;
        } else { // subvolume
            di->key.obj_id = fileref->fcb->subvol->id;
            di->key.obj_type = TYPE_ROOT_ITEM;
            di->key.offset = 0xffffffffffffffff;
        }

        di->transid = fileref->fcb->Vcb->superblock.generation;
        di->m = 0;
        di->n = (UINT16)fileref->utf8.Length;
        di->type = fileref->fcb->type;
        RtlCopyMemory(di->name, fileref->utf8.Buffer, fileref->utf8.Length);
        
        di2 = ExAllocatePoolWithTag(PagedPool, disize, ALLOC_TAG);
        if (!di2) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        RtlCopyMemory(di2, di, disize);
              
        if (!insert_tree_item(fileref->fcb->Vcb, fileref->parent->fcb->subvol, fileref->parent->fcb->inode, TYPE_DIR_INDEX, fileref->index, di, disize, NULL, Irp, rollback)) {
            ERR("insert_tree_item failed\n");
            Status = STATUS_INTERNAL_ERROR;
            return Status;
        }
        
        Status = add_dir_item(fileref->fcb->Vcb, fileref->parent->fcb->subvol, fileref->parent->fcb->inode, crc32, di2, disize, Irp, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("add_dir_item returned %08x\n", Status);
            return Status;
        }
        
        if (fileref->parent->fcb->subvol == fileref->fcb->subvol) {
            Status = add_inode_ref(fileref->fcb->Vcb, fileref->parent->fcb->subvol, fileref->fcb->inode, fileref->parent->fcb->inode, fileref->index, &fileref->utf8, Irp, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("add_inode_ref returned %08x\n", Status);
                return Status;
            }
        } else {
            ULONG rrlen;
            ROOT_REF* rr;

            rrlen = sizeof(ROOT_REF) - 1 + fileref->utf8.Length;
                
            rr = ExAllocatePoolWithTag(PagedPool, rrlen, ALLOC_TAG);
            if (!rr) {
                ERR("out of memory\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            
            rr->dir = fileref->parent->fcb->inode;
            rr->index = fileref->index;
            rr->n = fileref->utf8.Length;
            RtlCopyMemory(rr->name, fileref->utf8.Buffer, fileref->utf8.Length);
            
            Status = add_root_ref(fileref->fcb->Vcb, fileref->fcb->subvol->id, fileref->parent->fcb->subvol->id, rr, Irp, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("add_root_ref returned %08x\n", Status);
                return Status;
            }
            
            Status = update_root_backref(fileref->fcb->Vcb, fileref->fcb->subvol->id, fileref->parent->fcb->subvol->id, Irp, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("update_root_backref returned %08x\n", Status);
                return Status;
            }
        }
        
        fileref->created = FALSE;
    } else if (fileref->deleted) {
        UINT32 crc32;
        KEY searchkey;
        traverse_ptr tp;
        ANSI_STRING* name;
        
        if (fileref->oldutf8.Buffer)
            name = &fileref->oldutf8;
        else
            name = &fileref->utf8;

        crc32 = calc_crc32c(0xfffffffe, (UINT8*)name->Buffer, name->Length);

        TRACE("deleting %.*S\n", file_desc_fileref(fileref));
        
        // delete DIR_ITEM (0x54)
        
        Status = delete_dir_item(fileref->fcb->Vcb, fileref->parent->fcb->subvol, fileref->parent->fcb->inode, crc32, name, Irp, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("delete_dir_item returned %08x\n", Status);
            return Status;
        }
        
        if (fileref->parent->fcb->subvol == fileref->fcb->subvol) {
            // delete INODE_REF (0xc)
            
            Status = delete_inode_ref(fileref->fcb->Vcb, fileref->parent->fcb->subvol, fileref->fcb->inode, fileref->parent->fcb->inode, name, Irp, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("delete_inode_ref returned %08x\n", Status);
                return Status;
            }
        } else { // subvolume
            Status = delete_root_ref(fileref->fcb->Vcb, fileref->fcb->subvol->id, fileref->parent->fcb->subvol->id, fileref->parent->fcb->inode, name, Irp, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("delete_root_ref returned %08x\n", Status);
            }
            
            Status = update_root_backref(fileref->fcb->Vcb, fileref->fcb->subvol->id, fileref->parent->fcb->subvol->id, Irp, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("update_root_backref returned %08x\n", Status);
                return Status;
            }
        }
        
        // delete DIR_INDEX (0x60)
        
        searchkey.obj_id = fileref->parent->fcb->inode;
        searchkey.obj_type = TYPE_DIR_INDEX;
        searchkey.offset = fileref->index;

        Status = find_item(fileref->fcb->Vcb, fileref->parent->fcb->subvol, &tp, &searchkey, FALSE, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            Status = STATUS_INTERNAL_ERROR;
            return Status;
        }
        
        if (!keycmp(&searchkey, &tp.item->key)) {
            delete_tree_item(fileref->fcb->Vcb, &tp, rollback);
            TRACE("deleting (%llx,%x,%llx)\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
        }
        
        if (fileref->oldutf8.Buffer) {
            ExFreePool(fileref->oldutf8.Buffer);
            fileref->oldutf8.Buffer = NULL;
        }
    } else { // rename or change type
        PANSI_STRING oldutf8 = fileref->oldutf8.Buffer ? &fileref->oldutf8 : &fileref->utf8;
        UINT32 crc32, oldcrc32;
        ULONG disize;
        DIR_ITEM *di, *di2;
        KEY searchkey;
        traverse_ptr tp;
        
        crc32 = calc_crc32c(0xfffffffe, (UINT8*)fileref->utf8.Buffer, fileref->utf8.Length);
        
        if (!fileref->oldutf8.Buffer)
            oldcrc32 = crc32;
        else
            oldcrc32 = calc_crc32c(0xfffffffe, (UINT8*)fileref->oldutf8.Buffer, fileref->oldutf8.Length);

        // delete DIR_ITEM (0x54)
        
        Status = delete_dir_item(fileref->fcb->Vcb, fileref->parent->fcb->subvol, fileref->parent->fcb->inode, oldcrc32, oldutf8, Irp, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("delete_dir_item returned %08x\n", Status);
            return Status;
        }
        
        // add DIR_ITEM (0x54)
        
        disize = sizeof(DIR_ITEM) - 1 + fileref->utf8.Length;
        di = ExAllocatePoolWithTag(PagedPool, disize, ALLOC_TAG);
        if (!di) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        di2 = ExAllocatePoolWithTag(PagedPool, disize, ALLOC_TAG);
        if (!di2) {
            ERR("out of memory\n");
            ExFreePool(di);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        if (fileref->parent->fcb->subvol == fileref->fcb->subvol) {
            di->key.obj_id = fileref->fcb->inode;
            di->key.obj_type = TYPE_INODE_ITEM;
            di->key.offset = 0;
        } else { // subvolume
            di->key.obj_id = fileref->fcb->subvol->id;
            di->key.obj_type = TYPE_ROOT_ITEM;
            di->key.offset = 0xffffffffffffffff;
        }
        
        di->transid = fileref->fcb->Vcb->superblock.generation;
        di->m = 0;
        di->n = (UINT16)fileref->utf8.Length;
        di->type = fileref->fcb->type;
        RtlCopyMemory(di->name, fileref->utf8.Buffer, fileref->utf8.Length);
        
        RtlCopyMemory(di2, di, disize);
        
        Status = add_dir_item(fileref->fcb->Vcb, fileref->parent->fcb->subvol, fileref->parent->fcb->inode, crc32, di, disize, Irp, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("add_dir_item returned %08x\n", Status);
            return Status;
        }
        
        if (fileref->parent->fcb->subvol == fileref->fcb->subvol) {
            // delete INODE_REF (0xc)
            
            Status = delete_inode_ref(fileref->fcb->Vcb, fileref->parent->fcb->subvol, fileref->fcb->inode, fileref->parent->fcb->inode, oldutf8, Irp, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("delete_inode_ref returned %08x\n", Status);
                return Status;
            }
            
            // add INODE_REF (0xc)
            
            Status = add_inode_ref(fileref->fcb->Vcb, fileref->parent->fcb->subvol, fileref->fcb->inode, fileref->parent->fcb->inode, fileref->index, &fileref->utf8, Irp, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("add_inode_ref returned %08x\n", Status);
                return Status;
            }
        } else { // subvolume
            ULONG rrlen;
            ROOT_REF* rr;
            
            // FIXME - make sure this works with duff subvols within snapshots
            
            Status = delete_root_ref(fileref->fcb->Vcb, fileref->fcb->subvol->id, fileref->parent->fcb->subvol->id, fileref->parent->fcb->inode, oldutf8, Irp, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("delete_root_ref returned %08x\n", Status);
            }
            
            rrlen = sizeof(ROOT_REF) - 1 + fileref->utf8.Length;
            
            rr = ExAllocatePoolWithTag(PagedPool, rrlen, ALLOC_TAG);
            if (!rr) {
                ERR("out of memory\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            
            rr->dir = fileref->parent->fcb->inode;
            rr->index = fileref->index;
            rr->n = fileref->utf8.Length;
            RtlCopyMemory(rr->name, fileref->utf8.Buffer, fileref->utf8.Length);
            
            Status = add_root_ref(fileref->fcb->Vcb, fileref->fcb->subvol->id, fileref->parent->fcb->subvol->id, rr, Irp, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("add_root_ref returned %08x\n", Status);
                return Status;
            }
            
            Status = update_root_backref(fileref->fcb->Vcb, fileref->fcb->subvol->id, fileref->parent->fcb->subvol->id, Irp, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("update_root_backref returned %08x\n", Status);
                return Status;
            }
        }
        
        // delete DIR_INDEX (0x60)
        
        searchkey.obj_id = fileref->parent->fcb->inode;
        searchkey.obj_type = TYPE_DIR_INDEX;
        searchkey.offset = fileref->index;
        
        Status = find_item(fileref->fcb->Vcb, fileref->parent->fcb->subvol, &tp, &searchkey, FALSE, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            Status = STATUS_INTERNAL_ERROR;
            return Status;
        }
        
        if (!keycmp(&searchkey, &tp.item->key)) {
            delete_tree_item(fileref->fcb->Vcb, &tp, rollback);
            TRACE("deleting (%llx,%x,%llx)\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
        } else
            WARN("could not find (%llx,%x,%llx) in subvol %llx\n", searchkey.obj_id, searchkey.obj_type, searchkey.offset, fileref->fcb->subvol->id);
        
        // add DIR_INDEX (0x60)
        
        if (!insert_tree_item(fileref->fcb->Vcb, fileref->parent->fcb->subvol, fileref->parent->fcb->inode, TYPE_DIR_INDEX, fileref->index, di2, disize, NULL, Irp, rollback)) {
            ERR("insert_tree_item failed\n");
            Status = STATUS_INTERNAL_ERROR;
            return Status;
        }

        if (fileref->oldutf8.Buffer) {
            ExFreePool(fileref->oldutf8.Buffer);
            fileref->oldutf8.Buffer = NULL;
        }
    }

    fileref->dirty = FALSE;
    
    return STATUS_SUCCESS;
}

static void convert_shared_data_refs(device_extension* Vcb, PIRP Irp, LIST_ENTRY* rollback) {
    LIST_ENTRY* le;
    NTSTATUS Status;
    
    le = Vcb->trees.Flink;
    while (le != &Vcb->trees) {
        tree* t = CONTAINING_RECORD(le, tree, list_entry);
        
        if (t->write && t->header.level == 0 &&
            (t->header.flags & HEADER_FLAG_SHARED_BACKREF || !(t->header.flags & HEADER_FLAG_MIXED_BACKREF))) {
            LIST_ENTRY* le2;
            BOOL old = !(t->header.flags & HEADER_FLAG_MIXED_BACKREF);
            
            le2 = Vcb->shared_extents.Flink;
            while (le2 != &Vcb->shared_extents) {
                shared_data* sd = CONTAINING_RECORD(le2, shared_data, list_entry);
                
                if (sd->address == t->header.address) {
                    LIST_ENTRY* le3 = sd->entries.Flink;
                    while (le3 != &sd->entries) {
                        shared_data_entry* sde = CONTAINING_RECORD(le3, shared_data_entry, list_entry);
                        
                        TRACE("tree %llx; root %llx, objid %llx, offset %llx, count %x\n",
                              t->header.address, sde->edr.root, sde->edr.objid, sde->edr.offset, sde->edr.count);
                        
                        Status = increase_extent_refcount_data(Vcb, sde->address, sde->size, sde->edr.root, sde->edr.objid, sde->edr.offset, sde->edr.count, Irp, rollback);
                        
                        if (!NT_SUCCESS(Status))
                            WARN("increase_extent_refcount_data returned %08x\n", Status);
                        
                        if (old) {
                            Status = decrease_extent_refcount_old(Vcb, sde->address, sde->size, sd->address, Irp, rollback);
                            
                            if (!NT_SUCCESS(Status))
                                WARN("decrease_extent_refcount_old returned %08x\n", Status);
                        } else {
                            Status = decrease_extent_refcount_shared_data(Vcb, sde->address, sde->size, sd->address, sd->parent, Irp, rollback);
                            
                            if (!NT_SUCCESS(Status))
                                WARN("decrease_extent_refcount_shared_data returned %08x\n", Status);
                        }
                        
                        le3 = le3->Flink;
                    }
                    break;
                }
                
                le2 = le2->Flink;
            }
            
            t->header.flags &= ~HEADER_FLAG_SHARED_BACKREF;
            t->header.flags |= HEADER_FLAG_MIXED_BACKREF;
        }
        
        le = le->Flink;
    }
}

static NTSTATUS add_root_item_to_cache(device_extension* Vcb, UINT64 root, PIRP Irp, LIST_ENTRY* rollback) {
    KEY searchkey;
    traverse_ptr tp;
    NTSTATUS Status;
    
    searchkey.obj_id = root;
    searchkey.obj_type = TYPE_ROOT_ITEM;
    searchkey.offset = 0xffffffffffffffff;
    
    Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    if (tp.item->key.obj_id != searchkey.obj_id || tp.item->key.obj_type != searchkey.obj_type) {
        ERR("could not find ROOT_ITEM for tree %llx\n", searchkey.obj_id);
        int3;
        return STATUS_INTERNAL_ERROR;
    }
    
    if (tp.item->size < sizeof(ROOT_ITEM)) { // if not full length, create new entry with new bits zeroed
        ROOT_ITEM* ri = ExAllocatePoolWithTag(PagedPool, sizeof(ROOT_ITEM), ALLOC_TAG);
        if (!ri) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        if (tp.item->size > 0)
            RtlCopyMemory(ri, tp.item->data, tp.item->size);
        
        RtlZeroMemory(((UINT8*)ri) + tp.item->size, sizeof(ROOT_ITEM) - tp.item->size);
        
        delete_tree_item(Vcb, &tp, rollback);
        
        if (!insert_tree_item(Vcb, Vcb->root_root, searchkey.obj_id, searchkey.obj_type, tp.item->key.offset, ri, sizeof(ROOT_ITEM), NULL, Irp, rollback)) {
            ERR("insert_tree_item failed\n");
            return STATUS_INTERNAL_ERROR;
        }
    } else {
        tp.tree->write = TRUE;
    }
    
    return STATUS_SUCCESS;
}

static NTSTATUS add_root_items_to_cache(device_extension* Vcb, PIRP Irp, LIST_ENTRY* rollback) {
    LIST_ENTRY* le;
    NTSTATUS Status;
    
    le = Vcb->trees.Flink;
    while (le != &Vcb->trees) {
        tree* t = CONTAINING_RECORD(le, tree, list_entry);
        
        if (t->write && t->root != Vcb->chunk_root && t->root != Vcb->root_root) {
            Status = add_root_item_to_cache(Vcb, t->root->id, Irp, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("add_root_item_to_cache returned %08x\n", Status);
                return Status;
            }
        }
        
        le = le->Flink;
    }
    
    // make sure we always update the extent tree
    Status = add_root_item_to_cache(Vcb, BTRFS_ROOT_EXTENT, Irp, rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("add_root_item_to_cache returned %08x\n", Status);
        return Status;
    }
    
    return STATUS_SUCCESS;
}

NTSTATUS STDCALL do_write(device_extension* Vcb, PIRP Irp, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    LIST_ENTRY* le;
    BOOL cache_changed = FALSE;
    
#ifdef DEBUG_WRITE_LOOPS
    UINT loops = 0;
#endif
    
    TRACE("(%p)\n", Vcb);
    
    while (!IsListEmpty(&Vcb->dirty_filerefs)) {
        dirty_fileref* dirt;
        
        le = RemoveHeadList(&Vcb->dirty_filerefs);
        
        dirt = CONTAINING_RECORD(le, dirty_fileref, list_entry);
        
        flush_fileref(dirt->fileref, Irp, rollback);
        free_fileref(dirt->fileref);
        ExFreePool(dirt);
    }
    
    // We process deleted streams first, so we don't run over our xattr
    // limit unless we absolutely have to.
    
    le = Vcb->dirty_fcbs.Flink;
    while (le != &Vcb->dirty_fcbs) {
        dirty_fcb* dirt;
        LIST_ENTRY* le2 = le->Flink;
        
        dirt = CONTAINING_RECORD(le, dirty_fcb, list_entry);
        
        if (dirt->fcb->deleted && dirt->fcb->ads) {
            RemoveEntryList(le);
            
            flush_fcb(dirt->fcb, FALSE, Irp, rollback);
            free_fcb(dirt->fcb);
            ExFreePool(dirt);
        }
        
        le = le2;
    }
    
    le = Vcb->dirty_fcbs.Flink;
    while (le != &Vcb->dirty_fcbs) {
        dirty_fcb* dirt;
        LIST_ENTRY* le2 = le->Flink;
        
        dirt = CONTAINING_RECORD(le, dirty_fcb, list_entry);
        
        if (dirt->fcb->subvol != Vcb->root_root || dirt->fcb->deleted) {
            RemoveEntryList(le);
            
            flush_fcb(dirt->fcb, FALSE, Irp, rollback);
            free_fcb(dirt->fcb);
            ExFreePool(dirt);
        }
        
        le = le2;
    }
    
    convert_shared_data_refs(Vcb, Irp, rollback);
    
    ExAcquireResourceExclusiveLite(&Vcb->checksum_lock, TRUE);
    if (!IsListEmpty(&Vcb->sector_checksums)) {
        update_checksum_tree(Vcb, Irp, rollback);
    }
    ExReleaseResourceLite(&Vcb->checksum_lock);
    
    if (!IsListEmpty(&Vcb->drop_roots)) {
        Status = drop_roots(Vcb, Irp, rollback);
        
        if (!NT_SUCCESS(Status)) {
            ERR("drop_roots returned %08x\n", Status);
            return Status;
        }
    }
    
    if (!IsListEmpty(&Vcb->chunks_changed)) {
        Status = update_chunks(Vcb, Irp, rollback);
        
        if (!NT_SUCCESS(Status)) {
            ERR("update_chunks returned %08x\n", Status);
            return Status;
        }
    }
    
    // If only changing superblock, e.g. changing label, we still need to rewrite
    // the root tree so the generations match, otherwise you won't be able to mount on Linux.
    if (!Vcb->root_root->treeholder.tree || !Vcb->root_root->treeholder.tree->write) {
        KEY searchkey;
        
        traverse_ptr tp;
        
        searchkey.obj_id = 0;
        searchkey.obj_type = 0;
        searchkey.offset = 0;
        
        Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            return Status;
        }
        
        Vcb->root_root->treeholder.tree->write = TRUE;
    }
    
    Status = add_root_items_to_cache(Vcb, Irp, rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("add_root_items_to_cache returned %08x\n", Status);
        return Status;
    }
    
    do {
        Status = add_parents(Vcb, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("add_parents returned %08x\n", Status);
            goto end;
        }
        
        Status = do_splits(Vcb, Irp, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("do_splits returned %08x\n", Status);
            goto end;
        }
        
        Status = allocate_tree_extents(Vcb, Irp, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("add_parents returned %08x\n", Status);
            goto end;
        }
        
        Status = update_chunk_usage(Vcb, Irp, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("update_chunk_usage returned %08x\n", Status);
            goto end;
        }
        
        Status = allocate_cache(Vcb, &cache_changed, Irp, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("allocate_cache returned %08x\n", Status);
            goto end;
        }

#ifdef DEBUG_WRITE_LOOPS
        loops++;
        
        if (cache_changed)
            ERR("cache has changed, looping again\n");
#endif        
    } while (cache_changed || !trees_consistent(Vcb, rollback));
    
#ifdef DEBUG_WRITE_LOOPS
    ERR("%u loops\n", loops);
#endif
    
    TRACE("trees consistent\n");
    
    Status = update_root_root(Vcb, Irp, rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("update_root_root returned %08x\n", Status);
        goto end;
    }
    
    Status = write_trees(Vcb, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("write_trees returned %08x\n", Status);
        goto end;
    }
    
    Vcb->superblock.cache_generation = Vcb->superblock.generation;
    
    Status = write_superblocks(Vcb, Irp);
    if (!NT_SUCCESS(Status)) {
        ERR("write_superblocks returned %08x\n", Status);
        goto end;
    }
    
    clean_space_cache(Vcb);
    
    Vcb->superblock.generation++;
    
    Status = STATUS_SUCCESS;
    
    le = Vcb->trees.Flink;
    while (le != &Vcb->trees) {
        tree* t = CONTAINING_RECORD(le, tree, list_entry);
        
#ifdef DEBUG_PARANOID
        KEY searchkey;
        traverse_ptr tp;
        
        searchkey.obj_id = t->header.address;
        searchkey.obj_type = TYPE_METADATA_ITEM;
        searchkey.offset = 0xffffffffffffffff;
        
        Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            int3;
        }
        
        if (tp.item->key.obj_id != searchkey.obj_id || tp.item->key.obj_type != searchkey.obj_type) {
            searchkey.obj_id = t->header.address;
            searchkey.obj_type = TYPE_EXTENT_ITEM;
            searchkey.offset = 0xffffffffffffffff;
            
            Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE, Irp);
            if (!NT_SUCCESS(Status)) {
                ERR("error - find_item returned %08x\n", Status);
                int3;
            }
            
            if (tp.item->key.obj_id != searchkey.obj_id || tp.item->key.obj_type != searchkey.obj_type) {
                ERR("error - could not find entry in extent tree for tree at %llx\n", t->header.address);
                int3;
            }
        }
#endif
        
        t->write = FALSE;
        
        le = le->Flink;
    }
    
    Vcb->need_write = FALSE;
    
    while (!IsListEmpty(&Vcb->drop_roots)) {
        LIST_ENTRY* le = RemoveHeadList(&Vcb->drop_roots);
        root* r = CONTAINING_RECORD(le, root, list_entry);

        ExDeleteResourceLite(&r->nonpaged->load_tree_lock);
        ExFreePool(r->nonpaged);
        ExFreePool(r);
    }
    
end:
    TRACE("do_write returning %08x\n", Status);
    
    return Status;
}

static __inline BOOL entry_in_ordered_list(LIST_ENTRY* list, UINT64 value) {
    LIST_ENTRY* le = list->Flink;
    ordered_list* ol;
    
    while (le != list) {
        ol = (ordered_list*)le;
        
        if (ol->key > value)
            return FALSE;
        else if (ol->key == value)
            return TRUE;
        
        le = le->Flink;
    }
    
    return FALSE;
}

static changed_extent* get_changed_extent_item(chunk* c, UINT64 address, UINT64 size, BOOL no_csum) {
    LIST_ENTRY* le;
    changed_extent* ce;
    
    le = c->changed_extents.Flink;
    while (le != &c->changed_extents) {
        ce = CONTAINING_RECORD(le, changed_extent, list_entry);
        
        if (ce->address == address && ce->size == size)
            return ce;
        
        le = le->Flink;
    }
    
    ce = ExAllocatePoolWithTag(PagedPool, sizeof(changed_extent), ALLOC_TAG);
    if (!ce) {
        ERR("out of memory\n");
        return NULL;
    }
    
    ce->address = address;
    ce->size = size;
    ce->old_size = size;
    ce->count = 0;
    ce->old_count = 0;
    ce->no_csum = no_csum;
    InitializeListHead(&ce->refs);
    InitializeListHead(&ce->old_refs);
    
    InsertTailList(&c->changed_extents, &ce->list_entry);
    
    return ce;
}

NTSTATUS update_changed_extent_ref(device_extension* Vcb, chunk* c, UINT64 address, UINT64 size, UINT64 root, UINT64 objid, UINT64 offset, signed long long count,
                                   BOOL no_csum, UINT64 new_size, PIRP Irp) {
    LIST_ENTRY* le;
    changed_extent* ce;
    changed_extent_ref* cer;
    NTSTATUS Status;
    KEY searchkey;
    traverse_ptr tp;
    UINT64 old_count;
    
    ExAcquireResourceExclusiveLite(&c->changed_extents_lock, TRUE);
    
    ce = get_changed_extent_item(c, address, size, no_csum);
    
    if (!ce) {
        ERR("get_changed_extent_item failed\n");
        Status = STATUS_INTERNAL_ERROR;
        goto end;
    }
    
    if (IsListEmpty(&ce->refs) && IsListEmpty(&ce->old_refs)) { // new entry
        searchkey.obj_id = address;
        searchkey.obj_type = TYPE_EXTENT_ITEM;
        searchkey.offset = 0xffffffffffffffff;
        
        Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            goto end;
        }
        
        if (tp.item->key.obj_id != searchkey.obj_id || tp.item->key.obj_type != searchkey.obj_type) {
            ERR("could not find address %llx in extent tree\n", address);
            Status = STATUS_INTERNAL_ERROR;
            goto end;
        }
        
        if (tp.item->key.offset != size) {
            ERR("extent %llx had size %llx, not %llx as expected\n", address, tp.item->key.offset, size);
            Status = STATUS_INTERNAL_ERROR;
            goto end;
        }
        
        if (tp.item->size == sizeof(EXTENT_ITEM_V0)) {
            EXTENT_ITEM_V0* eiv0 = (EXTENT_ITEM_V0*)tp.item->data;
            
            ce->count = ce->old_count = eiv0->refcount;
        } else if (tp.item->size >= sizeof(EXTENT_ITEM)) {
            EXTENT_ITEM* ei = (EXTENT_ITEM*)tp.item->data;
            
            ce->count = ce->old_count = ei->refcount;
        } else {
            ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_ITEM));
            Status = STATUS_INTERNAL_ERROR;
            goto end;
        }
    }
    
    ce->size = new_size;
    
    le = ce->refs.Flink;
    while (le != &ce->refs) {
        cer = CONTAINING_RECORD(le, changed_extent_ref, list_entry);
        
        if (cer->edr.root == root && cer->edr.objid == objid && cer->edr.offset == offset) {
            ce->count += count;
            cer->edr.count += count;
            Status = STATUS_SUCCESS;
            goto end;
        }
        
        le = le->Flink;
    }
    
    old_count = find_extent_data_refcount(Vcb, address, size, root, objid, offset, Irp);
    
    if (old_count > 0) {
        cer = ExAllocatePoolWithTag(PagedPool, sizeof(changed_extent_ref), ALLOC_TAG);
    
        if (!cer) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }
        
        cer->edr.root = root;
        cer->edr.objid = objid;
        cer->edr.offset = offset;
        cer->edr.count = old_count;
        
        InsertTailList(&ce->old_refs, &cer->list_entry);
    }
    
    cer = ExAllocatePoolWithTag(PagedPool, sizeof(changed_extent_ref), ALLOC_TAG);
    
    if (!cer) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }
    
    cer->edr.root = root;
    cer->edr.objid = objid;
    cer->edr.offset = offset;
    cer->edr.count = old_count + count;
    
    InsertTailList(&ce->refs, &cer->list_entry);
    
    ce->count += count;
    
    Status = STATUS_SUCCESS;
    
end:
    ExReleaseResourceLite(&c->changed_extents_lock);
    
    return Status;
}

NTSTATUS excise_extents(device_extension* Vcb, fcb* fcb, UINT64 start_data, UINT64 end_data, PIRP Irp, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    LIST_ENTRY* le;
    
    le = fcb->extents.Flink;

    while (le != &fcb->extents) {
        LIST_ENTRY* le2 = le->Flink;
        extent* ext = CONTAINING_RECORD(le, extent, list_entry);
        EXTENT_DATA* ed = ext->data;
        EXTENT_DATA2* ed2;
        UINT64 len;
        
        if (!ext->ignore) {
            if (ext->datalen < sizeof(EXTENT_DATA)) {
                ERR("extent at %llx was %u bytes, expected at least %u\n", ext->offset, ext->datalen, sizeof(EXTENT_DATA));
                Status = STATUS_INTERNAL_ERROR;
                goto end;
            }
            
            if (ed->type == EXTENT_TYPE_REGULAR || ed->type == EXTENT_TYPE_PREALLOC) { 
                if (ext->datalen < sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2)) {
                    ERR("extent at %llx was %u bytes, expected at least %u\n", ext->offset, ext->datalen, sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2));
                    Status = STATUS_INTERNAL_ERROR;
                    goto end;
                }
                
                ed2 = (EXTENT_DATA2*)ed->data;
            }
            
            len = ed->type == EXTENT_TYPE_INLINE ? ed->decoded_size : ed2->num_bytes;
            
            if (ext->offset < end_data && ext->offset + len > start_data) {
                if (ed->type == EXTENT_TYPE_INLINE) {
                    if (start_data <= ext->offset && end_data >= ext->offset + len) { // remove all
                        remove_fcb_extent(fcb, ext, rollback);
                        
                        fcb->inode_item.st_blocks -= len;
                    } else if (start_data <= ext->offset && end_data < ext->offset + len) { // remove beginning
                        EXTENT_DATA* ned;
                        UINT64 size;
                        extent* newext;
                        
                        size = len - (end_data - ext->offset);
                        
                        ned = ExAllocatePoolWithTag(PagedPool, sizeof(EXTENT_DATA) - 1 + size, ALLOC_TAG);
                        if (!ned) {
                            ERR("out of memory\n");
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            goto end;
                        }
                        
                        newext = ExAllocatePoolWithTag(PagedPool, sizeof(extent), ALLOC_TAG);
                        if (!newext) {
                            ERR("out of memory\n");
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            ExFreePool(ned);
                            goto end;
                        }
                        
                        ned->generation = Vcb->superblock.generation;
                        ned->decoded_size = size;
                        ned->compression = ed->compression;
                        ned->encryption = ed->encryption;
                        ned->encoding = ed->encoding;
                        ned->type = ed->type;
                        
                        RtlCopyMemory(&ned->data[0], &ed->data[end_data - ext->offset], size);
                        
                        newext->offset = end_data;
                        newext->data = ned;
                        newext->datalen = sizeof(EXTENT_DATA) - 1 + size;
                        newext->unique = ext->unique;
                        newext->ignore = FALSE;
                        InsertHeadList(&ext->list_entry, &newext->list_entry);
                        
                        remove_fcb_extent(fcb, ext, rollback);
                        
                        fcb->inode_item.st_blocks -= end_data - ext->offset;
                    } else if (start_data > ext->offset && end_data >= ext->offset + len) { // remove end
                        EXTENT_DATA* ned;
                        UINT64 size;
                        extent* newext;
                        
                        size = start_data - ext->offset;
                        
                        ned = ExAllocatePoolWithTag(PagedPool, sizeof(EXTENT_DATA) - 1 + size, ALLOC_TAG);
                        if (!ned) {
                            ERR("out of memory\n");
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            goto end;
                        }
                        
                        newext = ExAllocatePoolWithTag(PagedPool, sizeof(extent), ALLOC_TAG);
                        if (!newext) {
                            ERR("out of memory\n");
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            ExFreePool(ned);
                            goto end;
                        }
                        
                        ned->generation = Vcb->superblock.generation;
                        ned->decoded_size = size;
                        ned->compression = ed->compression;
                        ned->encryption = ed->encryption;
                        ned->encoding = ed->encoding;
                        ned->type = ed->type;
                        
                        RtlCopyMemory(&ned->data[0], &ed->data[0], size);
                        
                        newext->offset = ext->offset;
                        newext->data = ned;
                        newext->datalen = sizeof(EXTENT_DATA) - 1 + size;
                        newext->unique = ext->unique;
                        newext->ignore = FALSE;
                        InsertHeadList(&ext->list_entry, &newext->list_entry);
                        
                        remove_fcb_extent(fcb, ext, rollback);
                        
                        fcb->inode_item.st_blocks -= ext->offset + len - start_data;
                    } else if (start_data > ext->offset && end_data < ext->offset + len) { // remove middle
                        EXTENT_DATA *ned1, *ned2;
                        UINT64 size;
                        extent *newext1, *newext2;
                        
                        size = start_data - ext->offset;
                        
                        ned1 = ExAllocatePoolWithTag(PagedPool, sizeof(EXTENT_DATA) - 1 + size, ALLOC_TAG);
                        if (!ned1) {
                            ERR("out of memory\n");
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            goto end;
                        }
                        
                        newext1 = ExAllocatePoolWithTag(PagedPool, sizeof(extent), ALLOC_TAG);
                        if (!newext1) {
                            ERR("out of memory\n");
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            ExFreePool(ned1);
                            goto end;
                        }
                        
                        ned1->generation = Vcb->superblock.generation;
                        ned1->decoded_size = size;
                        ned1->compression = ed->compression;
                        ned1->encryption = ed->encryption;
                        ned1->encoding = ed->encoding;
                        ned1->type = ed->type;
                        
                        RtlCopyMemory(&ned1->data[0], &ed->data[0], size);

                        newext1->offset = ext->offset;
                        newext1->data = ned1;
                        newext1->datalen = sizeof(EXTENT_DATA) - 1 + size;
                        newext1->unique = FALSE;
                        newext1->ignore = FALSE;
                        
                        size = ext->offset + len - end_data;
                        
                        ned2 = ExAllocatePoolWithTag(PagedPool, sizeof(EXTENT_DATA) - 1 + size, ALLOC_TAG);
                        if (!ned2) {
                            ERR("out of memory\n");
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            ExFreePool(ned1);
                            ExFreePool(newext1);
                            goto end;
                        }
                        
                        newext2 = ExAllocatePoolWithTag(PagedPool, sizeof(extent), ALLOC_TAG);
                        if (!newext2) {
                            ERR("out of memory\n");
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            ExFreePool(ned1);
                            ExFreePool(newext1);
                            ExFreePool(ned2);
                            goto end;
                        }
                        
                        ned2->generation = Vcb->superblock.generation;
                        ned2->decoded_size = size;
                        ned2->compression = ed->compression;
                        ned2->encryption = ed->encryption;
                        ned2->encoding = ed->encoding;
                        ned2->type = ed->type;
                        
                        RtlCopyMemory(&ned2->data[0], &ed->data[end_data - ext->offset], size);
                        
                        newext2->offset = end_data;
                        newext2->data = ned2;
                        newext2->datalen = sizeof(EXTENT_DATA) - 1 + size;
                        newext2->unique = FALSE;
                        newext2->ignore = FALSE;
                        
                        InsertHeadList(&ext->list_entry, &newext1->list_entry);
                        InsertHeadList(&newext1->list_entry, &newext2->list_entry);
                        
                        remove_fcb_extent(fcb, ext, rollback);
                        
                        fcb->inode_item.st_blocks -= end_data - start_data;
                    }
                } else if (ed->type == EXTENT_TYPE_REGULAR || ed->type == EXTENT_TYPE_PREALLOC) {
                    if (start_data <= ext->offset && end_data >= ext->offset + len) { // remove all
                        if (ed2->address != 0) {
                            chunk* c;
                            
                            fcb->inode_item.st_blocks -= len;
                            
                            c = get_chunk_from_address(Vcb, ed2->address);
                            
                            if (!c) {
                                ERR("get_chunk_from_address(%llx) failed\n", ed2->address);
                            } else {
                                Status = update_changed_extent_ref(Vcb, c, ed2->address, ed2->size, fcb->subvol->id, fcb->inode, ext->offset - ed2->offset, -1,
                                                                   fcb->inode_item.flags & BTRFS_INODE_NODATASUM, ed2->size, Irp);
                                if (!NT_SUCCESS(Status)) {
                                    ERR("update_changed_extent_ref returned %08x\n", Status);
                                    goto end;
                                }
                            }
                        }
                        
                        remove_fcb_extent(fcb, ext, rollback);
                    } else if (start_data <= ext->offset && end_data < ext->offset + len) { // remove beginning
                        EXTENT_DATA* ned;
                        EXTENT_DATA2* ned2;
                        extent* newext;
                        
                        if (ed2->address != 0)
                            fcb->inode_item.st_blocks -= end_data - ext->offset;
                        
                        ned = ExAllocatePoolWithTag(PagedPool, sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2), ALLOC_TAG);
                        if (!ned) {
                            ERR("out of memory\n");
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            goto end;
                        }
                        
                        newext = ExAllocatePoolWithTag(PagedPool, sizeof(extent), ALLOC_TAG);
                        if (!newext) {
                            ERR("out of memory\n");
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            ExFreePool(ned);
                            goto end;
                        }
                        
                        ned2 = (EXTENT_DATA2*)&ned->data[0];
                        
                        ned->generation = Vcb->superblock.generation;
                        ned->decoded_size = ed->decoded_size;
                        ned->compression = ed->compression;
                        ned->encryption = ed->encryption;
                        ned->encoding = ed->encoding;
                        ned->type = ed->type;
                        ned2->address = ed2->address;
                        ned2->size = ed2->size;
                        ned2->offset = ed2->address == 0 ? 0 : (ed2->offset + (end_data - ext->offset));
                        ned2->num_bytes = ed2->num_bytes - (end_data - ext->offset);

                        newext->offset = end_data;
                        newext->data = ned;
                        newext->datalen = sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2);
                        newext->unique = ext->unique;
                        newext->ignore = FALSE;
                        InsertHeadList(&ext->list_entry, &newext->list_entry);
                        
                        remove_fcb_extent(fcb, ext, rollback);
                    } else if (start_data > ext->offset && end_data >= ext->offset + len) { // remove end
                        EXTENT_DATA* ned;
                        EXTENT_DATA2* ned2;
                        extent* newext;
                        
                        if (ed2->address != 0)
                            fcb->inode_item.st_blocks -= ext->offset + len - start_data;
                        
                        ned = ExAllocatePoolWithTag(PagedPool, sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2), ALLOC_TAG);
                        if (!ned) {
                            ERR("out of memory\n");
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            goto end;
                        }
                        
                        newext = ExAllocatePoolWithTag(PagedPool, sizeof(extent), ALLOC_TAG);
                        if (!newext) {
                            ERR("out of memory\n");
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            ExFreePool(ned);
                            goto end;
                        }
                        
                        ned2 = (EXTENT_DATA2*)&ned->data[0];
                        
                        ned->generation = Vcb->superblock.generation;
                        ned->decoded_size = ed->decoded_size;
                        ned->compression = ed->compression;
                        ned->encryption = ed->encryption;
                        ned->encoding = ed->encoding;
                        ned->type = ed->type;
                        ned2->address = ed2->address;
                        ned2->size = ed2->size;
                        ned2->offset = ed2->address == 0 ? 0 : ed2->offset;
                        ned2->num_bytes = start_data - ext->offset;

                        newext->offset = ext->offset;
                        newext->data = ned;
                        newext->datalen = sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2);
                        newext->unique = ext->unique;
                        newext->ignore = FALSE;
                        InsertHeadList(&ext->list_entry, &newext->list_entry);
                        
                        remove_fcb_extent(fcb, ext, rollback);
                    } else if (start_data > ext->offset && end_data < ext->offset + len) { // remove middle
                        EXTENT_DATA *neda, *nedb;
                        EXTENT_DATA2 *neda2, *nedb2;
                        extent *newext1, *newext2;
                        
                        if (ed2->address != 0) {
                            chunk* c;
                            
                            fcb->inode_item.st_blocks -= end_data - start_data;
                            
                            c = get_chunk_from_address(Vcb, ed2->address);
                            
                            if (!c) {
                                ERR("get_chunk_from_address(%llx) failed\n", ed2->address);
                            } else {
                                Status = update_changed_extent_ref(Vcb, c, ed2->address, ed2->size, fcb->subvol->id, fcb->inode, ext->offset - ed2->offset, 1,
                                                                   fcb->inode_item.flags & BTRFS_INODE_NODATASUM, ed2->size, Irp);
                                if (!NT_SUCCESS(Status)) {
                                    ERR("update_changed_extent_ref returned %08x\n", Status);
                                    goto end;
                                }
                            }
                        }
                        
                        neda = ExAllocatePoolWithTag(PagedPool, sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2), ALLOC_TAG);
                        if (!neda) {
                            ERR("out of memory\n");
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            goto end;
                        }
                        
                        newext1 = ExAllocatePoolWithTag(PagedPool, sizeof(extent), ALLOC_TAG);
                        if (!newext1) {
                            ERR("out of memory\n");
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            ExFreePool(neda);
                            goto end;
                        }
                        
                        nedb = ExAllocatePoolWithTag(PagedPool, sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2), ALLOC_TAG);
                        if (!nedb) {
                            ERR("out of memory\n");
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            ExFreePool(neda);
                            ExFreePool(newext1);
                            goto end;
                        }
                        
                        newext2 = ExAllocatePoolWithTag(PagedPool, sizeof(extent), ALLOC_TAG);
                        if (!newext1) {
                            ERR("out of memory\n");
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            ExFreePool(neda);
                            ExFreePool(newext1);
                            ExFreePool(nedb);
                            goto end;
                        }
                        
                        neda2 = (EXTENT_DATA2*)&neda->data[0];
                        
                        neda->generation = Vcb->superblock.generation;
                        neda->decoded_size = ed->decoded_size;
                        neda->compression = ed->compression;
                        neda->encryption = ed->encryption;
                        neda->encoding = ed->encoding;
                        neda->type = ed->type;
                        neda2->address = ed2->address;
                        neda2->size = ed2->size;
                        neda2->offset = ed2->address == 0 ? 0 : ed2->offset;
                        neda2->num_bytes = start_data - ext->offset;

                        nedb2 = (EXTENT_DATA2*)&nedb->data[0];
                        
                        nedb->generation = Vcb->superblock.generation;
                        nedb->decoded_size = ed->decoded_size;
                        nedb->compression = ed->compression;
                        nedb->encryption = ed->encryption;
                        nedb->encoding = ed->encoding;
                        nedb->type = ed->type;
                        nedb2->address = ed2->address;
                        nedb2->size = ed2->size;
                        nedb2->offset = ed2->address == 0 ? 0 : (ed2->offset + (end_data - ext->offset));
                        nedb2->num_bytes = ext->offset + len - end_data;
                        
                        newext1->offset = ext->offset;
                        newext1->data = neda;
                        newext1->datalen = sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2);
                        newext1->unique = FALSE;
                        newext1->ignore = FALSE;
                        
                        newext2->offset = end_data;
                        newext2->data = nedb;
                        newext2->datalen = sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2);
                        newext2->unique = FALSE;
                        newext2->ignore = FALSE;
                        
                        InsertHeadList(&ext->list_entry, &newext1->list_entry);
                        InsertHeadList(&newext1->list_entry, &newext2->list_entry);
                        
                        remove_fcb_extent(fcb, ext, rollback);
                    }
                }
            }
        }

        le = le2;
    }
    
    // FIXME - do bitmap analysis of changed extents, and free what we can
    
    Status = STATUS_SUCCESS;

end:
    fcb->extents_changed = TRUE;
    mark_fcb_dirty(fcb);
    
    return Status;
}

static NTSTATUS do_write_data(device_extension* Vcb, UINT64 address, void* data, UINT64 length, LIST_ENTRY* changed_sector_list, PIRP Irp) {
    NTSTATUS Status;
    changed_sector* sc;
    int i;
    
    Status = write_data_complete(Vcb, address, data, length, Irp, NULL);
    if (!NT_SUCCESS(Status)) {
        ERR("write_data returned %08x\n", Status);
        return Status;
    }
    
    if (changed_sector_list) {
        sc = ExAllocatePoolWithTag(PagedPool, sizeof(changed_sector), ALLOC_TAG);
        if (!sc) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        sc->ol.key = address;
        sc->length = length / Vcb->superblock.sector_size;
        sc->deleted = FALSE;
        
        sc->checksums = ExAllocatePoolWithTag(PagedPool, sizeof(UINT32) * sc->length, ALLOC_TAG);
        if (!sc->checksums) {
            ERR("out of memory\n");
            ExFreePool(sc);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        for (i = 0; i < sc->length; i++) {
            sc->checksums[i] = ~calc_crc32c(0xffffffff, (UINT8*)data + (i * Vcb->superblock.sector_size), Vcb->superblock.sector_size);
        }

        insert_into_ordered_list(changed_sector_list, &sc->ol);
    }
    
    return STATUS_SUCCESS;
}

static void add_insert_extent_rollback(LIST_ENTRY* rollback, fcb* fcb, extent* ext) {
    rollback_extent* re;
    
    re = ExAllocatePoolWithTag(NonPagedPool, sizeof(rollback_extent), ALLOC_TAG);
    if (!re) {
        ERR("out of memory\n");
        return;
    }
    
    re->fcb = fcb;
    re->ext = ext;
    
    add_rollback(rollback, ROLLBACK_INSERT_EXTENT, re);
}

static BOOL add_extent_to_fcb(fcb* fcb, UINT64 offset, EXTENT_DATA* ed, ULONG edsize, BOOL unique, LIST_ENTRY* rollback) {
    extent* ext;
    LIST_ENTRY* le;
    
    ext = ExAllocatePoolWithTag(PagedPool, sizeof(extent), ALLOC_TAG);
    if (!ext) {
        ERR("out of memory\n");
        return FALSE;
    }
    
    ext->offset = offset;
    ext->data = ed;
    ext->datalen = edsize;
    ext->unique = unique;
    ext->ignore = FALSE;
    
    le = fcb->extents.Flink;
    while (le != &fcb->extents) {
        extent* oldext = CONTAINING_RECORD(le, extent, list_entry);
        
        if (!oldext->ignore) {
            if (oldext->offset > offset) {
                InsertHeadList(le->Blink, &ext->list_entry);
                goto end;
            }
        }
        
        le = le->Flink;
    }
    
    InsertTailList(&fcb->extents, &ext->list_entry);
    
end:
    add_insert_extent_rollback(rollback, fcb, ext);

    return TRUE;
}

static void remove_fcb_extent(fcb* fcb, extent* ext, LIST_ENTRY* rollback) {
    if (!ext->ignore) {
        rollback_extent* re;
        
        ext->ignore = TRUE;
        
        re = ExAllocatePoolWithTag(NonPagedPool, sizeof(rollback_extent), ALLOC_TAG);
        if (!re) {
            ERR("out of memory\n");
            return;
        }
        
        re->fcb = fcb;
        re->ext = ext;
        
        add_rollback(rollback, ROLLBACK_DELETE_EXTENT, re);
    }
}

static void add_changed_extent_ref(chunk* c, UINT64 address, UINT64 size, UINT64 root, UINT64 objid, UINT64 offset, UINT32 count, BOOL no_csum) {
    changed_extent* ce;
    changed_extent_ref* cer;
    LIST_ENTRY* le;
    
    ce = get_changed_extent_item(c, address, size, no_csum);
    
    if (!ce) {
        ERR("get_changed_extent_item failed\n");
        return;
    }
    
    le = ce->refs.Flink;
    while (le != &ce->refs) {
        cer = CONTAINING_RECORD(le, changed_extent_ref, list_entry);
        
        if (cer->edr.root == root && cer->edr.objid == objid && cer->edr.offset == offset) {
            ce->count += count;
            cer->edr.count += count;
            return;
        }
        
        le = le->Flink;
    }
    
    cer = ExAllocatePoolWithTag(PagedPool, sizeof(changed_extent_ref), ALLOC_TAG);
    
    if (!cer) {
        ERR("out of memory\n");
        return;
    }
    
    cer->edr.root = root;
    cer->edr.objid = objid;
    cer->edr.offset = offset;
    cer->edr.count = count;
    
    InsertTailList(&ce->refs, &cer->list_entry);
    
    ce->count += count;
}

BOOL insert_extent_chunk(device_extension* Vcb, fcb* fcb, chunk* c, UINT64 start_data, UINT64 length, BOOL prealloc, void* data,
                         LIST_ENTRY* changed_sector_list, PIRP Irp, LIST_ENTRY* rollback, UINT8 compression, UINT64 decoded_size) {
    UINT64 address;
    NTSTATUS Status;
    EXTENT_DATA* ed;
    EXTENT_DATA2* ed2;
    ULONG edsize = sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2);
// #ifdef DEBUG_PARANOID
//     traverse_ptr tp;
//     KEY searchkey;
// #endif
    
    TRACE("(%p, (%llx, %llx), %llx, %llx, %llx, %u, %p, %p, %p)\n", Vcb, fcb->subvol->id, fcb->inode, c->offset, start_data, length, prealloc, data, changed_sector_list, rollback);
    
    if (!find_address_in_chunk(Vcb, c, length, &address))
        return FALSE;
    
// #ifdef DEBUG_PARANOID
//     searchkey.obj_id = address;
//     searchkey.obj_type = TYPE_EXTENT_ITEM;
//     searchkey.offset = 0xffffffffffffffff;
//     
//     Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE);
//     if (!NT_SUCCESS(Status)) {
//         ERR("error - find_item returned %08x\n", Status);
//     } else if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == searchkey.obj_type) {
//         ERR("address %llx already allocated\n", address);
//         int3;
//     }
// #endif
    
    if (data) {
        Status = do_write_data(Vcb, address, data, length, changed_sector_list, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("do_write_data returned %08x\n", Status);
            return FALSE;
        }
    }
    
    // add extent data to inode
    ed = ExAllocatePoolWithTag(PagedPool, edsize, ALLOC_TAG);
    if (!ed) {
        ERR("out of memory\n");
        return FALSE;
    }
    
    ed->generation = Vcb->superblock.generation;
    ed->decoded_size = decoded_size;
    ed->compression = compression;
    ed->encryption = BTRFS_ENCRYPTION_NONE;
    ed->encoding = BTRFS_ENCODING_NONE;
    ed->type = prealloc ? EXTENT_TYPE_PREALLOC : EXTENT_TYPE_REGULAR;
    
    ed2 = (EXTENT_DATA2*)ed->data;
    ed2->address = address;
    ed2->size = length;
    ed2->offset = 0;
    ed2->num_bytes = decoded_size;
    
    if (!add_extent_to_fcb(fcb, start_data, ed, edsize, TRUE, rollback)) {
        ERR("add_extent_to_fcb failed\n");
        ExFreePool(ed);
        return FALSE;
    }
    
    increase_chunk_usage(c, length);
    space_list_subtract(Vcb, c, FALSE, address, length, rollback);
    
    fcb->inode_item.st_blocks += decoded_size;
    
    fcb->extents_changed = TRUE;
    mark_fcb_dirty(fcb);
    
    ExAcquireResourceExclusiveLite(&c->changed_extents_lock, TRUE);
    
    add_changed_extent_ref(c, address, length, fcb->subvol->id, fcb->inode, start_data, 1, fcb->inode_item.flags & BTRFS_INODE_NODATASUM);
    
    ExReleaseResourceLite(&c->changed_extents_lock);

    return TRUE;
}

static BOOL extend_data(device_extension* Vcb, fcb* fcb, UINT64 start_data, UINT64 length, void* data,
                        LIST_ENTRY* changed_sector_list, extent* ext, chunk* c, PIRP Irp, LIST_ENTRY* rollback) {
    EXTENT_DATA* ed;
    EXTENT_DATA2 *ed2, *ed2orig;
    extent* newext;
    UINT64 addr, origsize;
    NTSTATUS Status;
    LIST_ENTRY* le;
    
    TRACE("(%p, (%llx, %llx), %llx, %llx, %p, %p, %p, %p)\n", Vcb, fcb->subvol->id, fcb->inode, start_data,
                                                              length, data, changed_sector_list, ext, c, rollback);
    
    ed2orig = (EXTENT_DATA2*)ext->data->data;
    
    origsize = ed2orig->size;
    addr = ed2orig->address + ed2orig->size;
    
    Status = write_data_complete(Vcb, addr, data, length, Irp, c);
    if (!NT_SUCCESS(Status)) {
        ERR("write_data returned %08x\n", Status);
        return FALSE;
    }
    
    le = fcb->extents.Flink;
    while (le != &fcb->extents) {
        extent* ext2 = CONTAINING_RECORD(le, extent, list_entry);
            
        if (!ext2->ignore && (ext2->data->type == EXTENT_TYPE_REGULAR || ext2->data->type == EXTENT_TYPE_PREALLOC)) {
            EXTENT_DATA2* ed2b = (EXTENT_DATA2*)ext2->data->data;
            
            if (ed2b->address == ed2orig->address) {
                ed2b->size = origsize + length;
                ext2->data->decoded_size = origsize + length;
            }
        }
                
        le = le->Flink;
    }
    
    ed = ExAllocatePoolWithTag(PagedPool, ext->datalen, ALLOC_TAG);
    if (!ed) {
        ERR("out of memory\n");
        return FALSE;
    }
    
    newext = ExAllocatePoolWithTag(PagedPool, sizeof(extent), ALLOC_TAG);
    if (!newext) {
        ERR("out of memory\n");
        ExFreePool(ed);
        return FALSE;
    }
    
    RtlCopyMemory(ed, ext->data, ext->datalen);

    ed2 = (EXTENT_DATA2*)ed->data;
    ed2->offset = ed2orig->offset + ed2orig->num_bytes;
    ed2->num_bytes = length;
    
    RtlCopyMemory(newext, ext, sizeof(extent));
    newext->offset = ext->offset + ed2orig->num_bytes;
    newext->data = ed;
    
    InsertHeadList(&ext->list_entry, &newext->list_entry);
    
    add_insert_extent_rollback(rollback, fcb, newext);
    
    Status = update_changed_extent_ref(Vcb, c, ed2orig->address, origsize, fcb->subvol->id, fcb->inode, newext->offset - ed2->offset,
                                       1, fcb->inode_item.flags & BTRFS_INODE_NODATASUM, ed2->size, Irp);

    if (!NT_SUCCESS(Status)) {
        ERR("update_changed_extent_ref returned %08x\n", Status);
        return FALSE;
    }
    
    if (changed_sector_list) {
        int i;
        changed_sector* sc = ExAllocatePoolWithTag(PagedPool, sizeof(changed_sector), ALLOC_TAG);
        if (!sc) {
            ERR("out of memory\n");
            return FALSE;
        }
        
        sc->ol.key = addr;
        sc->length = length / Vcb->superblock.sector_size;
        sc->deleted = FALSE;
        
        sc->checksums = ExAllocatePoolWithTag(PagedPool, sizeof(UINT32) * sc->length, ALLOC_TAG);
        if (!sc->checksums) {
            ERR("out of memory\n");
            ExFreePool(sc);
            return FALSE;
        }
        
        for (i = 0; i < sc->length; i++) {
            sc->checksums[i] = ~calc_crc32c(0xffffffff, (UINT8*)data + (i * Vcb->superblock.sector_size), Vcb->superblock.sector_size);
        }
        insert_into_ordered_list(changed_sector_list, &sc->ol);
    }
    
    increase_chunk_usage(c, length);
      
    space_list_subtract(Vcb, c, FALSE, addr, length, NULL); // no rollback as we don't reverse extending the extent
     
    fcb->inode_item.st_blocks += length;
    
    return TRUE;
}

static BOOL try_extend_data(device_extension* Vcb, fcb* fcb, UINT64 start_data, UINT64 length, void* data,
                            LIST_ENTRY* changed_sector_list, PIRP Irp, UINT64* written, LIST_ENTRY* rollback) {
    BOOL success = FALSE;
    EXTENT_DATA* ed;
    EXTENT_DATA2* ed2;
    chunk* c;
    LIST_ENTRY* le;
    space* s;
    extent* ext = NULL;
    
    le = fcb->extents.Flink;
    
    while (le != &fcb->extents) {
        extent* nextext = CONTAINING_RECORD(le, extent, list_entry);
        
        if (!nextext->ignore) {
            if (nextext->offset == start_data) {
                ext = nextext;
                break;
            } else if (nextext->offset > start_data)
                break;
            
            ext = nextext;
        }
        
        le = le->Flink;
    }
    
    if (!ext)
        return FALSE;

    if (!ext->unique) {
        TRACE("extent was not unique\n");
        return FALSE;
    }
    
    ed = ext->data;
    
    if (ext->datalen < sizeof(EXTENT_DATA)) {
        ERR("extent %llx was %u bytes, expected at least %u\n", ext->offset, ext->datalen, sizeof(EXTENT_DATA));
        return FALSE;
    }
    
    if (ed->type != EXTENT_TYPE_REGULAR) {
        TRACE("not extending extent which is not EXTENT_TYPE_REGULAR\n");
        return FALSE;
    }
    
    ed2 = (EXTENT_DATA2*)ed->data;
    
    if (ext->datalen < sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2)) {
        ERR("extent %llx was %u bytes, expected at least %u\n", ext->offset, ext->datalen, sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2));
        return FALSE;
    }

    if (ext->offset + ed2->num_bytes != start_data) {
        TRACE("last EXTENT_DATA does not run up to start_data (%llx + %llx != %llx)\n", ext->offset, ed2->num_bytes, start_data);
        return FALSE;
    }
    
    if (ed->compression != BTRFS_COMPRESSION_NONE) {
        TRACE("not extending a compressed extent\n");
        return FALSE;
    }
    
    if (ed->encryption != BTRFS_ENCRYPTION_NONE) {
        WARN("encryption not supported\n");
        return FALSE;
    }
    
    if (ed->encoding != BTRFS_ENCODING_NONE) {
        WARN("other encodings not supported\n");
        return FALSE;
    }
    
    if (ed2->size - ed2->offset != ed2->num_bytes) {
        TRACE("last EXTENT_DATA does not run all the way to the end of the extent\n");
        return FALSE;
    }
    
    if (ed2->size >= MAX_EXTENT_SIZE) {
        TRACE("extent size was too large to extend (%llx >= %llx)\n", ed2->size, (UINT64)MAX_EXTENT_SIZE);
        return FALSE;
    }
    
    c = get_chunk_from_address(Vcb, ed2->address);
    
    ExAcquireResourceExclusiveLite(&c->lock, TRUE);
    
    le = c->space.Flink;
    while (le != &c->space) {
        s = CONTAINING_RECORD(le, space, list_entry);
        
        if (s->address == ed2->address + ed2->size) {
            UINT64 newlen = min(min(s->size, length), MAX_EXTENT_SIZE - ed2->size);
            
            success = extend_data(Vcb, fcb, start_data, newlen, data, changed_sector_list, ext, c, Irp, rollback);
            
            if (success)
                *written += newlen;
            
            break;
        } else if (s->address > ed2->address + ed2->size)
            break;
        
        le = le->Flink;
    }
    
    ExReleaseResourceLite(&c->lock);
    
    return success;
}

static NTSTATUS insert_prealloc_extent(fcb* fcb, UINT64 start, UINT64 length, LIST_ENTRY* rollback) {
    LIST_ENTRY* le;
    chunk* c;
#ifdef __REACTOS__
    UINT64 flags;
#else
    UINT64 flags, origlength = length;
#endif
    NTSTATUS Status;
    BOOL page_file = fcb->Header.Flags2 & FSRTL_FLAG2_IS_PAGING_FILE;
    
    flags = fcb->Vcb->data_flags;
    
    // FIXME - try and maximize contiguous ranges first. If we can't do that,
    // allocate all the free space we find until it's enough.
    
    do {
        UINT64 extlen = min(MAX_EXTENT_SIZE, length);
        
        ExAcquireResourceSharedLite(&fcb->Vcb->chunk_lock, TRUE);
        
        le = fcb->Vcb->chunks.Flink;
        while (le != &fcb->Vcb->chunks) {
            c = CONTAINING_RECORD(le, chunk, list_entry);
            
            ExAcquireResourceExclusiveLite(&c->lock, TRUE);
            
            if (c->chunk_item->type == flags && (c->chunk_item->size - c->used) >= extlen) {
                if (insert_extent_chunk(fcb->Vcb, fcb, c, start, extlen, !page_file, NULL, NULL, NULL, rollback, BTRFS_COMPRESSION_NONE, extlen)) {
                    ExReleaseResourceLite(&c->lock);
                    ExReleaseResourceLite(&fcb->Vcb->chunk_lock);
                    goto cont;
                }
            }
            
            ExReleaseResourceLite(&c->lock);

            le = le->Flink;
        }
        
        ExReleaseResourceLite(&fcb->Vcb->chunk_lock);
        
        ExAcquireResourceExclusiveLite(&fcb->Vcb->chunk_lock, TRUE);
        
        if ((c = alloc_chunk(fcb->Vcb, flags))) {
            ExReleaseResourceLite(&fcb->Vcb->chunk_lock);
            
            ExAcquireResourceExclusiveLite(&c->lock, TRUE);
            
            if (c->chunk_item->type == flags && (c->chunk_item->size - c->used) >= extlen) {
                if (insert_extent_chunk(fcb->Vcb, fcb, c, start, extlen, !page_file, NULL, NULL, NULL, rollback, BTRFS_COMPRESSION_NONE, extlen)) {
                    ExReleaseResourceLite(&c->lock);
                    goto cont;
                }
            }
            
            ExReleaseResourceLite(&c->lock);
        } else
            ExReleaseResourceLite(&fcb->Vcb->chunk_lock);
        
        WARN("couldn't find any data chunks with %llx bytes free\n", origlength);
        Status = STATUS_DISK_FULL;
        goto end;
        
cont:
        length -= extlen;
        start += extlen;
    } while (length > 0);
    
    Status = STATUS_SUCCESS;
    
end:
    return Status;
}

// static void print_tree(tree* t) {
//     LIST_ENTRY* le = t->itemlist.Flink;
//     while (le != &t->itemlist) {
//         tree_data* td = CONTAINING_RECORD(le, tree_data, list_entry);
//         ERR("%llx,%x,%llx (ignore = %s)\n", td->key.obj_id, td->key.obj_type, td->key.offset, td->ignore ? "TRUE" : "FALSE");
//         le = le->Flink;
//     }
// }

NTSTATUS insert_extent(device_extension* Vcb, fcb* fcb, UINT64 start_data, UINT64 length, void* data, LIST_ENTRY* changed_sector_list, PIRP Irp, LIST_ENTRY* rollback) {
    LIST_ENTRY* le;
    chunk* c;
    UINT64 flags, orig_length = length, written = 0;
    
    TRACE("(%p, (%llx, %llx), %llx, %llx, %p, %p)\n", Vcb, fcb->subvol->id, fcb->inode, start_data, length, data, changed_sector_list);
    
    // FIXME - split data up if not enough space for just one extent
    
    if (start_data > 0) {
        try_extend_data(Vcb, fcb, start_data, length, data, changed_sector_list, Irp, &written, rollback);
        
        if (written == length)
            return STATUS_SUCCESS;
        else if (written > 0) {
            start_data += written;
            length -= written;
            data = &((UINT8*)data)[written];
        }
    }
    
    flags = Vcb->data_flags;
    
    while (written < orig_length) {
        UINT64 newlen = min(length, MAX_EXTENT_SIZE);
        BOOL done = FALSE;
        
        // Rather than necessarily writing the whole extent at once, we deal with it in blocks of 128 MB.
        // First, see if we can write the extent part to an existing chunk.
        
        ExAcquireResourceSharedLite(&Vcb->chunk_lock, TRUE);
        
        le = Vcb->chunks.Flink;
        while (le != &Vcb->chunks) {
            c = CONTAINING_RECORD(le, chunk, list_entry);
            
            ExAcquireResourceExclusiveLite(&c->lock, TRUE);
            
            if (c->chunk_item->type == flags && (c->chunk_item->size - c->used) >= newlen) {
                if (insert_extent_chunk(Vcb, fcb, c, start_data, newlen, FALSE, data, changed_sector_list, Irp, rollback, BTRFS_COMPRESSION_NONE, newlen)) {
                    written += newlen;
                    
                    if (written == orig_length) {
                        ExReleaseResourceLite(&c->lock);
                        ExReleaseResourceLite(&Vcb->chunk_lock);
                        return STATUS_SUCCESS;
                    } else {
                        done = TRUE;
                        start_data += newlen;
                        length -= newlen;
                        data = &((UINT8*)data)[newlen];
                        break;
                    }
                }
            }
            
            ExReleaseResourceLite(&c->lock);

            le = le->Flink;
        }
        
        ExReleaseResourceLite(&fcb->Vcb->chunk_lock);
        
        if (done) continue;
        
        // Otherwise, see if we can put it in a new chunk.
        
        ExAcquireResourceExclusiveLite(&fcb->Vcb->chunk_lock, TRUE);
        
        if ((c = alloc_chunk(Vcb, flags))) {
            ExReleaseResourceLite(&Vcb->chunk_lock);
            
            ExAcquireResourceExclusiveLite(&c->lock, TRUE);
            
            if (c->chunk_item->type == flags && (c->chunk_item->size - c->used) >= newlen) {
                if (insert_extent_chunk(Vcb, fcb, c, start_data, newlen, FALSE, data, changed_sector_list, Irp, rollback, BTRFS_COMPRESSION_NONE, newlen)) {
                    written += newlen;
                    
                    if (written == orig_length) {
                        ExReleaseResourceLite(&c->lock);
                        return STATUS_SUCCESS;
                    } else {
                        done = TRUE;
                        start_data += newlen;
                        length -= newlen;
                        data = &((UINT8*)data)[newlen];
                    }
                }
            }
            
            ExReleaseResourceLite(&c->lock);
        } else
            ExReleaseResourceLite(&Vcb->chunk_lock);
        
        if (!done) {
            FIXME("FIXME - not enough room to write whole extent part, try to write bits and pieces\n"); // FIXME
            break;
        }
    }
    
    WARN("couldn't find any data chunks with %llx bytes free\n", length);

    return STATUS_DISK_FULL;
}

static void update_checksum_tree(device_extension* Vcb, PIRP Irp, LIST_ENTRY* rollback) {
    LIST_ENTRY* le = Vcb->sector_checksums.Flink;
    changed_sector* cs;
    traverse_ptr tp, next_tp;
    KEY searchkey;
    UINT32* data;
    NTSTATUS Status;
    
    if (!Vcb->checksum_root) {
        ERR("no checksum root\n");
        goto exit;
    }
    
    while (le != &Vcb->sector_checksums) {
        UINT64 startaddr, endaddr;
        ULONG len;
        UINT32* checksums;
        RTL_BITMAP bmp;
        ULONG* bmparr;
        ULONG runlength, index;
        
        cs = (changed_sector*)le;
        
        searchkey.obj_id = EXTENT_CSUM_ID;
        searchkey.obj_type = TYPE_EXTENT_CSUM;
        searchkey.offset = cs->ol.key;
        
        // FIXME - create checksum_root if it doesn't exist at all
        
        Status = find_item(Vcb, Vcb->checksum_root, &tp, &searchkey, FALSE, Irp);
        if (!NT_SUCCESS(Status)) { // tree is completely empty
            // FIXME - do proper check here that tree is empty
            if (!cs->deleted) {
                checksums = ExAllocatePoolWithTag(PagedPool, sizeof(UINT32) * cs->length, ALLOC_TAG);
                if (!checksums) {
                    ERR("out of memory\n");
                    goto exit;
                }
                
                RtlCopyMemory(checksums, cs->checksums, sizeof(UINT32) * cs->length);
                
                if (!insert_tree_item(Vcb, Vcb->checksum_root, EXTENT_CSUM_ID, TYPE_EXTENT_CSUM, cs->ol.key, checksums, sizeof(UINT32) * cs->length, NULL, Irp, rollback)) {
                    ERR("insert_tree_item failed\n");
                    ExFreePool(checksums);
                    goto exit;
                }
            }
        } else {
            UINT32 tplen;
            
            // FIXME - check entry is TYPE_EXTENT_CSUM?
            
            if (tp.item->key.offset < cs->ol.key && tp.item->key.offset + (tp.item->size * Vcb->superblock.sector_size / sizeof(UINT32)) >= cs->ol.key)
                startaddr = tp.item->key.offset;
            else
                startaddr = cs->ol.key;
            
            searchkey.obj_id = EXTENT_CSUM_ID;
            searchkey.obj_type = TYPE_EXTENT_CSUM;
            searchkey.offset = cs->ol.key + (cs->length * Vcb->superblock.sector_size);
            
            Status = find_item(Vcb, Vcb->checksum_root, &tp, &searchkey, FALSE, Irp);
            if (!NT_SUCCESS(Status)) {
                ERR("error - find_item returned %08x\n", Status);
                goto exit;
            }
            
            tplen = tp.item->size / sizeof(UINT32);
            
            if (tp.item->key.offset + (tplen * Vcb->superblock.sector_size) >= cs->ol.key + (cs->length * Vcb->superblock.sector_size))
                endaddr = tp.item->key.offset + (tplen * Vcb->superblock.sector_size);
            else
                endaddr = cs->ol.key + (cs->length * Vcb->superblock.sector_size);
            
            TRACE("cs starts at %llx (%x sectors)\n", cs->ol.key, cs->length);
            TRACE("startaddr = %llx\n", startaddr);
            TRACE("endaddr = %llx\n", endaddr);
            
            len = (endaddr - startaddr) / Vcb->superblock.sector_size;
            
            checksums = ExAllocatePoolWithTag(PagedPool, sizeof(UINT32) * len, ALLOC_TAG);
            if (!checksums) {
                ERR("out of memory\n");
                goto exit;
            }
            
            bmparr = ExAllocatePoolWithTag(PagedPool, sizeof(ULONG) * ((len/8)+1), ALLOC_TAG);
            if (!bmparr) {
                ERR("out of memory\n");
                ExFreePool(checksums);
                goto exit;
            }
                
            RtlInitializeBitMap(&bmp, bmparr, len);
            RtlSetAllBits(&bmp);
            
            searchkey.obj_id = EXTENT_CSUM_ID;
            searchkey.obj_type = TYPE_EXTENT_CSUM;
            searchkey.offset = cs->ol.key;
            
            Status = find_item(Vcb, Vcb->checksum_root, &tp, &searchkey, FALSE, Irp);
            if (!NT_SUCCESS(Status)) {
                ERR("error - find_item returned %08x\n", Status);
                goto exit;
            }
            
            // set bit = free space, cleared bit = allocated sector
            
    //         ERR("start loop\n");
            while (tp.item->key.offset < endaddr) {
    //             ERR("%llx,%x,%llx\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
                if (tp.item->key.offset >= startaddr) {
                    if (tp.item->size > 0) {
                        RtlCopyMemory(&checksums[(tp.item->key.offset - startaddr) / Vcb->superblock.sector_size], tp.item->data, tp.item->size);
                        RtlClearBits(&bmp, (tp.item->key.offset - startaddr) / Vcb->superblock.sector_size, tp.item->size / sizeof(UINT32));
                    }
                    
                    delete_tree_item(Vcb, &tp, rollback);
                }
                
                if (find_next_item(Vcb, &tp, &next_tp, FALSE, Irp)) {
                    tp = next_tp;
                } else
                    break;
            }
    //         ERR("end loop\n");
            
            if (cs->deleted) {
                RtlSetBits(&bmp, (cs->ol.key - startaddr) / Vcb->superblock.sector_size, cs->length);
            } else {
                RtlCopyMemory(&checksums[(cs->ol.key - startaddr) / Vcb->superblock.sector_size], cs->checksums, cs->length * sizeof(UINT32));
                RtlClearBits(&bmp, (cs->ol.key - startaddr) / Vcb->superblock.sector_size, cs->length);
            }
            
            runlength = RtlFindFirstRunClear(&bmp, &index);
            
            while (runlength != 0) {
                do {
                    ULONG rl;
                    
                    if (runlength * sizeof(UINT32) > MAX_CSUM_SIZE)
                        rl = MAX_CSUM_SIZE / sizeof(UINT32);
                    else
                        rl = runlength;
                    
                    data = ExAllocatePoolWithTag(PagedPool, sizeof(UINT32) * rl, ALLOC_TAG);
                    if (!data) {
                        ERR("out of memory\n");
                        ExFreePool(bmparr);
                        ExFreePool(checksums);
                        goto exit;
                    }
                    
                    RtlCopyMemory(data, &checksums[index], sizeof(UINT32) * rl);
                    
                    if (!insert_tree_item(Vcb, Vcb->checksum_root, EXTENT_CSUM_ID, TYPE_EXTENT_CSUM, startaddr + (index * Vcb->superblock.sector_size), data, sizeof(UINT32) * rl, NULL, Irp, rollback)) {
                        ERR("insert_tree_item failed\n");
                        ExFreePool(data);
                        ExFreePool(bmparr);
                        ExFreePool(checksums);
                        goto exit;
                    }
                    
                    runlength -= rl;
                    index += rl;
                } while (runlength > 0);
                
                runlength = RtlFindNextForwardRunClear(&bmp, index, &index);
            }
            
            ExFreePool(bmparr);
            ExFreePool(checksums);
        }
        
        le = le->Flink;
    }
    
exit:
    while (!IsListEmpty(&Vcb->sector_checksums)) {
        le = RemoveHeadList(&Vcb->sector_checksums);
        cs = (changed_sector*)le;
        
        if (cs->checksums)
            ExFreePool(cs->checksums);
        
        ExFreePool(cs);
    }
}

void commit_checksum_changes(device_extension* Vcb, LIST_ENTRY* changed_sector_list) {
    while (!IsListEmpty(changed_sector_list)) {
        LIST_ENTRY* le = RemoveHeadList(changed_sector_list);
        InsertTailList(&Vcb->sector_checksums, le);
    }
}

NTSTATUS truncate_file(fcb* fcb, UINT64 end, PIRP Irp, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    
    // FIXME - convert into inline extent if short enough
    
    Status = excise_extents(fcb->Vcb, fcb, sector_align(end, fcb->Vcb->superblock.sector_size),
                            sector_align(fcb->inode_item.st_size, fcb->Vcb->superblock.sector_size), Irp, rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("error - excise_extents failed\n");
        return Status;
    }
    
    fcb->inode_item.st_size = end;
    TRACE("setting st_size to %llx\n", end);

    fcb->Header.AllocationSize.QuadPart = sector_align(fcb->inode_item.st_size, fcb->Vcb->superblock.sector_size);
    fcb->Header.FileSize.QuadPart = fcb->inode_item.st_size;
    fcb->Header.ValidDataLength.QuadPart = fcb->inode_item.st_size;
    // FIXME - inform cache manager of this
    
    TRACE("fcb %p FileSize = %llx\n", fcb, fcb->Header.FileSize.QuadPart);
    
    return STATUS_SUCCESS;
}

NTSTATUS extend_file(fcb* fcb, file_ref* fileref, UINT64 end, BOOL prealloc, PIRP Irp, LIST_ENTRY* rollback) {
    UINT64 oldalloc, newalloc;
    BOOL cur_inline;
    NTSTATUS Status;
    
    TRACE("(%p, %p, %x, %u)\n", fcb, fileref, end, prealloc);

    if (fcb->ads)
        return stream_set_end_of_file_information(fcb->Vcb, end, fcb, fileref, NULL, FALSE, rollback);
    else {
        extent* ext = NULL;
        LIST_ENTRY* le;
        
        le = fcb->extents.Blink;
        while (le != &fcb->extents) {
            extent* ext2 = CONTAINING_RECORD(le, extent, list_entry);
            
            if (!ext2->ignore) {
                ext = ext2;
                break;
            }
            
            le = le->Blink;
        }
        
        oldalloc = 0;
        if (ext) {
            EXTENT_DATA* ed = ext->data;
            EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ed->data;
            
            if (ext->datalen < sizeof(EXTENT_DATA)) {
                ERR("extent %llx was %u bytes, expected at least %u\n", ext->offset, ext->datalen, sizeof(EXTENT_DATA));
                return STATUS_INTERNAL_ERROR;
            }
            
            oldalloc = ext->offset + (ed->type == EXTENT_TYPE_INLINE ? ed->decoded_size : ed2->num_bytes);
            cur_inline = ed->type == EXTENT_TYPE_INLINE;
        
            if (cur_inline && end > fcb->Vcb->options.max_inline) {
                LIST_ENTRY changed_sector_list;
                BOOL nocsum = fcb->inode_item.flags & BTRFS_INODE_NODATASUM;
                UINT64 origlength, length;
                UINT8* data;
                UINT64 offset = ext->offset;
                
                TRACE("giving inline file proper extents\n");
                
                origlength = ed->decoded_size;
                
                cur_inline = FALSE;
                
                if (!nocsum)
                    InitializeListHead(&changed_sector_list);
                
                length = sector_align(origlength, fcb->Vcb->superblock.sector_size);
                
                data = ExAllocatePoolWithTag(PagedPool, length, ALLOC_TAG);
                if (!data) {
                    ERR("could not allocate %llx bytes for data\n", length);
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                
                if (length > origlength)
                    RtlZeroMemory(data + origlength, length - origlength);
                
                RtlCopyMemory(data, ed->data, origlength);
                
                fcb->inode_item.st_blocks -= origlength;
                
                remove_fcb_extent(fcb, ext, rollback);
                
                if (write_fcb_compressed(fcb)) {
                    Status = write_compressed(fcb, offset, offset + length, data, nocsum ? NULL : &changed_sector_list, Irp, rollback);
                    if (!NT_SUCCESS(Status)) {
                        ERR("write_compressed returned %08x\n", Status);
                        ExFreePool(data);
                        return Status;
                    }
                } else {
                    Status = insert_extent(fcb->Vcb, fcb, offset, length, data, nocsum ? NULL : &changed_sector_list, Irp, rollback);
                    if (!NT_SUCCESS(Status)) {
                        ERR("insert_extent returned %08x\n", Status);
                        ExFreePool(data);
                        return Status;
                    }
                }
                
                oldalloc = ext->offset + length;
                
                ExFreePool(data);
                
                if (!nocsum) {
                    ExAcquireResourceExclusiveLite(&fcb->Vcb->checksum_lock, TRUE);
                    commit_checksum_changes(fcb->Vcb, &changed_sector_list);
                    ExReleaseResourceLite(&fcb->Vcb->checksum_lock);
                }
            }
            
            if (cur_inline) {
                ULONG edsize;
                
                if (end > oldalloc) {
                    edsize = sizeof(EXTENT_DATA) - 1 + end - ext->offset;
                    ed = ExAllocatePoolWithTag(PagedPool, edsize, ALLOC_TAG);
                    
                    if (!ed) {
                        ERR("out of memory\n");
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }
                    
                    RtlZeroMemory(ed, edsize);
                    RtlCopyMemory(ed, ext->data, ext->datalen);
                    
                    ed->decoded_size = end - ext->offset;
                    
                    remove_fcb_extent(fcb, ext, rollback);
                    
                    if (!add_extent_to_fcb(fcb, ext->offset, ed, edsize, ext->unique, rollback)) {
                        ERR("add_extent_to_fcb failed\n");
                        ExFreePool(ed);
                        return STATUS_INTERNAL_ERROR;
                    }
                    
                    fcb->extents_changed = TRUE;
                    mark_fcb_dirty(fcb);
                }
                
                TRACE("extending inline file (oldalloc = %llx, end = %llx)\n", oldalloc, end);
                
                fcb->inode_item.st_size = end;
                TRACE("setting st_size to %llx\n", end);
                
                fcb->inode_item.st_blocks = end;

                fcb->Header.AllocationSize.QuadPart = fcb->Header.FileSize.QuadPart = fcb->Header.ValidDataLength.QuadPart = end;
            } else {
                newalloc = sector_align(end, fcb->Vcb->superblock.sector_size);
            
                if (newalloc > oldalloc) {
                    if (prealloc) {
                        // FIXME - try and extend previous extent first
                        
                        Status = insert_prealloc_extent(fcb, oldalloc, newalloc - oldalloc, rollback);
                    
                        if (!NT_SUCCESS(Status)) {
                            ERR("insert_prealloc_extent returned %08x\n", Status);
                            return Status;
                        }
                    }
                    
                    fcb->extents_changed = TRUE;
                    mark_fcb_dirty(fcb);
                }
                
                fcb->inode_item.st_size = end;
                TRACE("setting st_size to %llx\n", end);
                
                TRACE("newalloc = %llx\n", newalloc);
                
                fcb->Header.AllocationSize.QuadPart = newalloc;
                fcb->Header.FileSize.QuadPart = fcb->Header.ValidDataLength.QuadPart = end;
            }
        } else {
            if (end > fcb->Vcb->options.max_inline) {
                newalloc = sector_align(end, fcb->Vcb->superblock.sector_size);
            
                if (prealloc) {
                    Status = insert_prealloc_extent(fcb, 0, newalloc, rollback);
                    
                    if (!NT_SUCCESS(Status)) {
                        ERR("insert_prealloc_extent returned %08x\n", Status);
                        return Status;
                    }
                }
                
                fcb->extents_changed = TRUE;
                mark_fcb_dirty(fcb);
                
                fcb->inode_item.st_size = end;
                TRACE("setting st_size to %llx\n", end);
                
                TRACE("newalloc = %llx\n", newalloc);
                
                fcb->Header.AllocationSize.QuadPart = newalloc;
                fcb->Header.FileSize.QuadPart = fcb->Header.ValidDataLength.QuadPart = end;
            } else {
                EXTENT_DATA* ed;
                ULONG edsize;
                
                edsize = sizeof(EXTENT_DATA) - 1 + end;
                ed = ExAllocatePoolWithTag(PagedPool, edsize, ALLOC_TAG);
                
                if (!ed) {
                    ERR("out of memory\n");
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                
                ed->generation = fcb->Vcb->superblock.generation;
                ed->decoded_size = end;
                ed->compression = BTRFS_COMPRESSION_NONE;
                ed->encryption = BTRFS_ENCRYPTION_NONE;
                ed->encoding = BTRFS_ENCODING_NONE;
                ed->type = EXTENT_TYPE_INLINE;
                
                RtlZeroMemory(ed->data, end);
                
                if (!add_extent_to_fcb(fcb, 0, ed, edsize, FALSE, rollback)) {
                    ERR("add_extent_to_fcb failed\n");
                    ExFreePool(ed);
                    return STATUS_INTERNAL_ERROR;
                }
                
                fcb->extents_changed = TRUE;
                mark_fcb_dirty(fcb);
                
                fcb->inode_item.st_size = end;
                TRACE("setting st_size to %llx\n", end);
                
                fcb->inode_item.st_blocks = end;

                fcb->Header.AllocationSize.QuadPart = fcb->Header.FileSize.QuadPart = fcb->Header.ValidDataLength.QuadPart = end;
            }
        }
    }
    
    return STATUS_SUCCESS;
}

// #ifdef DEBUG_PARANOID
// static void print_loaded_trees(tree* t, int spaces) {
//     char pref[10];
//     int i;
//     LIST_ENTRY* le;
//     
//     for (i = 0; i < spaces; i++) {
//         pref[i] = ' ';
//     }
//     pref[spaces] = 0;
//     
//     if (!t) {
//         ERR("%s(not loaded)\n", pref);
//         return;
//     }
//     
//     le = t->itemlist.Flink;
//     while (le != &t->itemlist) {
//         tree_data* td = CONTAINING_RECORD(le, tree_data, list_entry);
//         
//         ERR("%s%llx,%x,%llx ignore=%s\n", pref, td->key.obj_id, td->key.obj_type, td->key.offset, td->ignore ? "TRUE" : "FALSE");
//         
//         if (t->header.level > 0) {
//             print_loaded_trees(td->treeholder.tree, spaces+1);
//         }
//         
//         le = le->Flink;
//     }
// }

// static void check_extents_consistent(device_extension* Vcb, fcb* fcb) {
//     KEY searchkey;
//     traverse_ptr tp, next_tp;
//     UINT64 length, oldlength, lastoff, alloc;
//     NTSTATUS Status;
//     EXTENT_DATA* ed;
//     EXTENT_DATA2* ed2;
//     
//     if (fcb->ads || fcb->inode_item.st_size == 0 || fcb->deleted)
//         return;
//     
//     TRACE("inode = %llx, subvol = %llx\n", fcb->inode, fcb->subvol->id);
//     
//     searchkey.obj_id = fcb->inode;
//     searchkey.obj_type = TYPE_EXTENT_DATA;
//     searchkey.offset = 0;
//     
//     Status = find_item(Vcb, fcb->subvol, &tp, &searchkey, FALSE);
//     if (!NT_SUCCESS(Status)) {
//         ERR("error - find_item returned %08x\n", Status);
//         goto failure;
//     }
//     
//     if (keycmp(&searchkey, &tp.item->key)) {
//         ERR("could not find EXTENT_DATA at offset 0\n");
//         goto failure;
//     }
//     
//     if (tp.item->size < sizeof(EXTENT_DATA)) {
//         ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_DATA));
//         goto failure;
//     }
//     
//     ed = (EXTENT_DATA*)tp.item->data;
//     ed2 = (EXTENT_DATA2*)&ed->data[0];
//     
//     length = oldlength = ed->type == EXTENT_TYPE_INLINE ? ed->decoded_size : ed2->num_bytes;
//     lastoff = tp.item->key.offset;
//     
//     TRACE("(%llx,%x,%llx) length = %llx\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, length);
//     
//     alloc = 0;
//     if (ed->type != EXTENT_TYPE_REGULAR || ed2->address != 0) {
//         alloc += length;
//     }
//     
//     while (find_next_item(Vcb, &tp, &next_tp, FALSE)) {
//         if (next_tp.item->key.obj_id != searchkey.obj_id || next_tp.item->key.obj_type != searchkey.obj_type)
//             break;
//         
//         tp = next_tp;
//         
//         if (tp.item->size < sizeof(EXTENT_DATA)) {
//             ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_DATA));
//             goto failure;
//         }
//         
//         ed = (EXTENT_DATA*)tp.item->data;
//         ed2 = (EXTENT_DATA2*)&ed->data[0];
//     
//         length = ed->type == EXTENT_TYPE_INLINE ? ed->decoded_size : ed2->num_bytes;
//     
//         TRACE("(%llx,%x,%llx) length = %llx\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, length);
//         
//         if (tp.item->key.offset != lastoff + oldlength) {
//             ERR("EXTENT_DATA in %llx,%llx was at %llx, expected %llx\n", fcb->subvol->id, fcb->inode, tp.item->key.offset, lastoff + oldlength);
//             goto failure;
//         }
//         
//         if (ed->type != EXTENT_TYPE_REGULAR || ed2->address != 0) {
//             alloc += length;
//         }
//         
//         oldlength = length;
//         lastoff = tp.item->key.offset;
//     }
//     
//     if (alloc != fcb->inode_item.st_blocks) {
//         ERR("allocation size was %llx, expected %llx\n", alloc, fcb->inode_item.st_blocks);
//         goto failure;
//     }
//     
// //     if (fcb->inode_item.st_blocks != lastoff + oldlength) {
// //         ERR("extents finished at %x, expected %x\n", (UINT32)(lastoff + oldlength), (UINT32)fcb->inode_item.st_blocks);
// //         goto failure;
// //     }
//     
//     return;
//     
// failure:
//     if (fcb->subvol->treeholder.tree)
//         print_loaded_trees(fcb->subvol->treeholder.tree, 0);
// 
//     int3;
// }

// static void check_extent_tree_consistent(device_extension* Vcb) {
//     KEY searchkey;
//     traverse_ptr tp, next_tp;
//     UINT64 lastaddr;
//     BOOL b, inconsistency;
//     
//     searchkey.obj_id = 0;
//     searchkey.obj_type = 0;
//     searchkey.offset = 0;
//     
//     if (!find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE)) {
//         ERR("error - could not find any entries in extent_root\n");
//         int3;
//     }
//     
//     lastaddr = 0;
//     inconsistency = FALSE;
//     
//     do {
//         if (tp.item->key.obj_type == TYPE_EXTENT_ITEM) {
// //             ERR("%x,%x,%x\n", (UINT32)tp.item->key.obj_id, tp.item->key.obj_type, (UINT32)tp.item->key.offset);
//             
//             if (tp.item->key.obj_id < lastaddr) {
// //                 ERR("inconsistency!\n");
// //                 int3;
//                 inconsistency = TRUE;
//             }
//             
//             lastaddr = tp.item->key.obj_id + tp.item->key.offset;
//         }
//         
//         b = find_next_item(Vcb, &tp, &next_tp, NULL, FALSE);
//         if (b) {
//             free_traverse_ptr(&tp);
//             tp = next_tp;
//         }
//     } while (b);
//     
//     free_traverse_ptr(&tp);
//     
//     if (!inconsistency)
//         return;
//     
//     ERR("Inconsistency detected:\n");
//     
//     if (!find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE)) {
//         ERR("error - could not find any entries in extent_root\n");
//         int3;
//     }
//     
//     do {
//         if (tp.item->key.obj_type == TYPE_EXTENT_ITEM) {
//             ERR("%x,%x,%x\n", (UINT32)tp.item->key.obj_id, tp.item->key.obj_type, (UINT32)tp.item->key.offset);
//             
//             if (tp.item->key.obj_id < lastaddr) {
//                 ERR("inconsistency!\n");
//             }
//             
//             lastaddr = tp.item->key.obj_id + tp.item->key.offset;
//         }
//         
//         b = find_next_item(Vcb, &tp, &next_tp, NULL, FALSE);
//         if (b) {
//             free_traverse_ptr(&tp);
//             tp = next_tp;
//         }
//     } while (b);
//     
//     free_traverse_ptr(&tp);
//     
//     int3;
// }
// #endif

static NTSTATUS do_write_file_prealloc(fcb* fcb, extent* ext, UINT64 start_data, UINT64 end_data, void* data, UINT64* written,
                                       LIST_ENTRY* changed_sector_list, PIRP Irp, LIST_ENTRY* rollback) {
    EXTENT_DATA* ed = ext->data;
    EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ed->data;
    NTSTATUS Status;
    chunk* c;
    
    if (start_data <= ext->offset && end_data >= ext->offset + ed2->num_bytes) { // replace all
        EXTENT_DATA* ned;
        extent* newext;
        
        ned = ExAllocatePoolWithTag(PagedPool, ext->datalen, ALLOC_TAG);
        if (!ned) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        newext = ExAllocatePoolWithTag(PagedPool, sizeof(extent), ALLOC_TAG);
        if (!newext) {
            ERR("out of memory\n");
            ExFreePool(ned);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        RtlCopyMemory(ned, ext->data, ext->datalen);
        
        ned->type = EXTENT_TYPE_REGULAR;
        
        Status = do_write_data(fcb->Vcb, ed2->address + ed2->offset, (UINT8*)data + ext->offset - start_data, ed2->num_bytes, changed_sector_list, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("do_write_data returned %08x\n", Status);
            return Status;
        }
        
        *written = ed2->num_bytes;
        
        newext->offset = ext->offset;
        newext->data = ned;
        newext->datalen = ext->datalen;
        newext->unique = ext->unique;
        newext->ignore = FALSE;
        InsertHeadList(&ext->list_entry, &newext->list_entry);

        add_insert_extent_rollback(rollback, fcb, newext);
        
        remove_fcb_extent(fcb, ext, rollback);
    } else if (start_data <= ext->offset && end_data < ext->offset + ed2->num_bytes) { // replace beginning
        EXTENT_DATA *ned, *nedb;
        EXTENT_DATA2* ned2;
        extent *newext1, *newext2;
        
        ned = ExAllocatePoolWithTag(PagedPool, ext->datalen, ALLOC_TAG);
        if (!ned) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        nedb = ExAllocatePoolWithTag(PagedPool, ext->datalen, ALLOC_TAG);
        if (!nedb) {
            ERR("out of memory\n");
            ExFreePool(ned);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        newext1 = ExAllocatePoolWithTag(PagedPool, sizeof(extent), ALLOC_TAG);
        if (!newext1) {
            ERR("out of memory\n");
            ExFreePool(ned);
            ExFreePool(nedb);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        newext2 = ExAllocatePoolWithTag(PagedPool, sizeof(extent), ALLOC_TAG);
        if (!newext2) {
            ERR("out of memory\n");
            ExFreePool(ned);
            ExFreePool(nedb);
            ExFreePool(newext1);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        RtlCopyMemory(ned, ext->data, ext->datalen);
        ned->type = EXTENT_TYPE_REGULAR;
        ned2 = (EXTENT_DATA2*)ned->data;
        ned2->num_bytes = end_data - ext->offset;
        
        RtlCopyMemory(nedb, ext->data, ext->datalen);
        ned2 = (EXTENT_DATA2*)nedb->data;
        ned2->offset += end_data - ext->offset;
        ned2->num_bytes -= end_data - ext->offset;
        
        Status = do_write_data(fcb->Vcb, ed2->address + ed2->offset, (UINT8*)data + ext->offset - start_data, end_data - ext->offset, changed_sector_list, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("do_write_data returned %08x\n", Status);
            return Status;
        }
        
        *written = end_data - ext->offset;
        
        newext1->offset = ext->offset;
        newext1->data = ned;
        newext1->datalen = ext->datalen;
        newext1->unique = FALSE;
        newext1->ignore = FALSE;
        InsertHeadList(&ext->list_entry, &newext1->list_entry);
        
        add_insert_extent_rollback(rollback, fcb, newext1);
        
        newext2->offset = end_data;
        newext2->data = nedb;
        newext2->datalen = ext->datalen;
        newext2->unique = FALSE;
        newext2->ignore = FALSE;
        InsertHeadList(&newext1->list_entry, &newext2->list_entry);
        
        add_insert_extent_rollback(rollback, fcb, newext2);
        
        c = get_chunk_from_address(fcb->Vcb, ed2->address);
        
        if (!c)
            ERR("get_chunk_from_address(%llx) failed\n", ed2->address);
        else {
            Status = update_changed_extent_ref(fcb->Vcb, c, ed2->address, ed2->size, fcb->subvol->id, fcb->inode, ext->offset - ed2->offset, 1,
                                                fcb->inode_item.flags & BTRFS_INODE_NODATASUM, ed2->size, Irp);
            
            if (!NT_SUCCESS(Status)) {
                ERR("update_changed_extent_ref returned %08x\n", Status);
                return Status;
            }
        }

        remove_fcb_extent(fcb, ext, rollback);
    } else if (start_data > ext->offset && end_data >= ext->offset + ed2->num_bytes) { // replace end
        EXTENT_DATA *ned, *nedb;
        EXTENT_DATA2* ned2;
        extent *newext1, *newext2;
        
        ned = ExAllocatePoolWithTag(PagedPool, ext->datalen, ALLOC_TAG);
        if (!ned) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        nedb = ExAllocatePoolWithTag(PagedPool, ext->datalen, ALLOC_TAG);
        if (!nedb) {
            ERR("out of memory\n");
            ExFreePool(ned);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        newext1 = ExAllocatePoolWithTag(PagedPool, sizeof(extent), ALLOC_TAG);
        if (!newext1) {
            ERR("out of memory\n");
            ExFreePool(ned);
            ExFreePool(nedb);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        newext2 = ExAllocatePoolWithTag(PagedPool, sizeof(extent), ALLOC_TAG);
        if (!newext2) {
            ERR("out of memory\n");
            ExFreePool(ned);
            ExFreePool(nedb);
            ExFreePool(newext1);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        RtlCopyMemory(ned, ext->data, ext->datalen);
        
        ned2 = (EXTENT_DATA2*)ned->data;
        ned2->num_bytes = start_data - ext->offset;
        
        RtlCopyMemory(nedb, ext->data, ext->datalen);
        
        nedb->type = EXTENT_TYPE_REGULAR;
        ned2 = (EXTENT_DATA2*)nedb->data;
        ned2->offset += start_data - ext->offset;
        ned2->num_bytes = ext->offset + ed2->num_bytes - start_data;
        
        Status = do_write_data(fcb->Vcb, ed2->address + ned2->offset, data, ned2->num_bytes, changed_sector_list, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("do_write_data returned %08x\n", Status);
            return Status;
        }
        
        *written = ned2->num_bytes;
        
        newext1->offset = ext->offset;
        newext1->data = ned;
        newext1->datalen = ext->datalen;
        newext1->unique = FALSE;
        newext1->ignore = FALSE;
        InsertHeadList(&ext->list_entry, &newext1->list_entry);
        
        add_insert_extent_rollback(rollback, fcb, newext1);
        
        newext2->offset = start_data;
        newext2->data = nedb;
        newext2->datalen = ext->datalen;
        newext2->unique = FALSE;
        newext2->ignore = FALSE;
        InsertHeadList(&newext1->list_entry, &newext2->list_entry);
        
        add_insert_extent_rollback(rollback, fcb, newext2);
        
        c = get_chunk_from_address(fcb->Vcb, ed2->address);
        
        if (!c)
            ERR("get_chunk_from_address(%llx) failed\n", ed2->address);
        else {
            Status = update_changed_extent_ref(fcb->Vcb, c, ed2->address, ed2->size, fcb->subvol->id, fcb->inode, ext->offset - ed2->offset, 1,
                                               fcb->inode_item.flags & BTRFS_INODE_NODATASUM, ed2->size, Irp);
            
            if (!NT_SUCCESS(Status)) {
                ERR("update_changed_extent_ref returned %08x\n", Status);
                return Status;
            }
        }

        remove_fcb_extent(fcb, ext, rollback);
    } else if (start_data > ext->offset && end_data < ext->offset + ed2->num_bytes) { // replace middle
        EXTENT_DATA *ned, *nedb, *nedc;
        EXTENT_DATA2* ned2;
        extent *newext1, *newext2, *newext3;
        
        ned = ExAllocatePoolWithTag(PagedPool, ext->datalen, ALLOC_TAG);
        if (!ned) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        nedb = ExAllocatePoolWithTag(PagedPool, ext->datalen, ALLOC_TAG);
        if (!nedb) {
            ERR("out of memory\n");
            ExFreePool(ned);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        nedc = ExAllocatePoolWithTag(PagedPool, ext->datalen, ALLOC_TAG);
        if (!nedb) {
            ERR("out of memory\n");
            ExFreePool(ned);
            ExFreePool(nedb);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        newext1 = ExAllocatePoolWithTag(PagedPool, sizeof(extent), ALLOC_TAG);
        if (!newext1) {
            ERR("out of memory\n");
            ExFreePool(ned);
            ExFreePool(nedb);
            ExFreePool(nedc);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        newext2 = ExAllocatePoolWithTag(PagedPool, sizeof(extent), ALLOC_TAG);
        if (!newext2) {
            ERR("out of memory\n");
            ExFreePool(ned);
            ExFreePool(nedb);
            ExFreePool(nedc);
            ExFreePool(newext1);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        newext3 = ExAllocatePoolWithTag(PagedPool, sizeof(extent), ALLOC_TAG);
        if (!newext2) {
            ERR("out of memory\n");
            ExFreePool(ned);
            ExFreePool(nedb);
            ExFreePool(nedc);
            ExFreePool(newext1);
            ExFreePool(newext2);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        RtlCopyMemory(ned, ext->data, ext->datalen);
        RtlCopyMemory(nedb, ext->data, ext->datalen);
        RtlCopyMemory(nedc, ext->data, ext->datalen);
        
        ned2 = (EXTENT_DATA2*)ned->data;
        ned2->num_bytes = start_data - ext->offset;
        
        nedb->type = EXTENT_TYPE_REGULAR;
        ned2 = (EXTENT_DATA2*)nedb->data;
        ned2->offset += start_data - ext->offset;
        ned2->num_bytes = end_data - start_data;
        
        ned2 = (EXTENT_DATA2*)nedc->data;
        ned2->offset += end_data - ext->offset;
        ned2->num_bytes -= end_data - ext->offset;
        
        ned2 = (EXTENT_DATA2*)nedb->data;
        Status = do_write_data(fcb->Vcb, ed2->address + ned2->offset, data, end_data - start_data, changed_sector_list, Irp);
        if (!NT_SUCCESS(Status)) {
            ERR("do_write_data returned %08x\n", Status);
            return Status;
        }
        
        *written = end_data - start_data;
        
        newext1->offset = ext->offset;
        newext1->data = ned;
        newext1->datalen = ext->datalen;
        newext1->unique = FALSE;
        newext1->ignore = FALSE;
        InsertHeadList(&ext->list_entry, &newext1->list_entry);
        
        add_insert_extent_rollback(rollback, fcb, newext1);
        
        newext2->offset = start_data;
        newext2->data = nedb;
        newext2->datalen = ext->datalen;
        newext2->unique = FALSE;
        newext2->ignore = FALSE;
        InsertHeadList(&newext1->list_entry, &newext2->list_entry);
        
        add_insert_extent_rollback(rollback, fcb, newext2);
        
        newext3->offset = end_data;
        newext3->data = nedc;
        newext3->datalen = ext->datalen;
        newext3->unique = FALSE;
        newext3->ignore = FALSE;
        InsertHeadList(&newext2->list_entry, &newext3->list_entry);
        
        add_insert_extent_rollback(rollback, fcb, newext3);
        
        c = get_chunk_from_address(fcb->Vcb, ed2->address);
        
        if (!c)
            ERR("get_chunk_from_address(%llx) failed\n", ed2->address);
        else {
            Status = update_changed_extent_ref(fcb->Vcb, c, ed2->address, ed2->size, fcb->subvol->id, fcb->inode, ext->offset - ed2->offset, 2,
                                               fcb->inode_item.flags & BTRFS_INODE_NODATASUM, ed2->size, Irp);
            
            if (!NT_SUCCESS(Status)) {
                ERR("update_changed_extent_ref returned %08x\n", Status);
                return Status;
            }
        }

        remove_fcb_extent(fcb, ext, rollback);
    }
    
    return STATUS_SUCCESS;
}

NTSTATUS do_write_file(fcb* fcb, UINT64 start, UINT64 end_data, void* data, LIST_ENTRY* changed_sector_list, PIRP Irp, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    LIST_ENTRY *le, *le2;
    UINT64 written = 0, length = end_data - start;
    UINT64 last_cow_start;
#ifdef DEBUG_PARANOID
    UINT64 last_off;
#endif
    
    last_cow_start = 0;
    
    le = fcb->extents.Flink;
    while (le != &fcb->extents) {
        extent* ext = CONTAINING_RECORD(le, extent, list_entry);
        
        le2 = le->Flink;
        
        if (!ext->ignore) {
            EXTENT_DATA* ed = ext->data;
            EXTENT_DATA2* ed2 = ed->type == EXTENT_TYPE_INLINE ? NULL : (EXTENT_DATA2*)ed->data;
            UINT64 len;
            BOOL nocow;
            
            len = ed->type == EXTENT_TYPE_INLINE ? ed->decoded_size : ed2->num_bytes;
            
            if (ext->offset + len <= start)
                goto nextitem;
            
            if (ext->offset > start + written + length)
                break;
            
            nocow = (ext->unique && fcb->inode_item.flags & BTRFS_INODE_NODATACOW) || ed->type == EXTENT_TYPE_PREALLOC;
           
            if (nocow) {
                if (max(last_cow_start, start + written) < ext->offset) {
                    UINT64 start_write = max(last_cow_start, start + written);
                    
                    Status = excise_extents(fcb->Vcb, fcb, start_write, ext->offset, Irp, rollback);
                    if (!NT_SUCCESS(Status)) {
                        ERR("excise_extents returned %08x\n", Status);
                        return Status;
                    }
                    
                    Status = insert_extent(fcb->Vcb, fcb, start_write, ext->offset - start_write, data, changed_sector_list, Irp, rollback);
                    if (!NT_SUCCESS(Status)) {
                        ERR("insert_extent returned %08x\n", Status);
                        return Status;
                    }
                    
                    written += ext->offset - start_write;
                    length -= ext->offset - start_write;
                    
                    if (length == 0)
                        break;
                }
                
                if (ed->type == EXTENT_TYPE_REGULAR) {
                    UINT64 writeaddr = ed2->address + ed2->offset + start + written - ext->offset;
                    UINT64 write_len = min(len, length);
                                    
                    TRACE("doing non-COW write to %llx\n", writeaddr);
                    
                    Status = write_data_complete(fcb->Vcb, writeaddr, (UINT8*)data + written, write_len, Irp, NULL);
                    if (!NT_SUCCESS(Status)) {
                        ERR("write_data_complete returned %08x\n", Status);
                        return Status;
                    }
                    
                    if (changed_sector_list) {
                        unsigned int i;
                        changed_sector* sc;
                        
                        sc = ExAllocatePoolWithTag(PagedPool, sizeof(changed_sector), ALLOC_TAG);
                        if (!sc) {
                            ERR("out of memory\n");
                            return STATUS_INSUFFICIENT_RESOURCES;
                        }
                        
                        sc->ol.key = writeaddr;
                        sc->length = write_len / fcb->Vcb->superblock.sector_size;
                        sc->deleted = FALSE;
                        
                        sc->checksums = ExAllocatePoolWithTag(PagedPool, sizeof(UINT32) * sc->length, ALLOC_TAG);
                        if (!sc->checksums) {
                            ERR("out of memory\n");
                            ExFreePool(sc);
                            return STATUS_INSUFFICIENT_RESOURCES;
                        }
                        
                        for (i = 0; i < sc->length; i++) {
                            sc->checksums[i] = ~calc_crc32c(0xffffffff, (UINT8*)data + written + (i * fcb->Vcb->superblock.sector_size), fcb->Vcb->superblock.sector_size);
                        }
    
                        insert_into_ordered_list(changed_sector_list, &sc->ol);
                    }
                    
                    written += write_len;
                    length -= write_len;
                    
                    if (length == 0)
                        break;
                } else if (ed->type == EXTENT_TYPE_PREALLOC) {
                    UINT64 write_len;
                    
                    Status = do_write_file_prealloc(fcb, ext, start + written, end_data, (UINT8*)data + written, &write_len,
                                                    changed_sector_list, Irp, rollback);
                    if (!NT_SUCCESS(Status)) {
                        ERR("do_write_file_prealloc returned %08x\n", Status);
                        return Status;
                    }
                    
                    written += write_len;
                    length -= write_len;
                    
                    if (length == 0)
                        break;
                }
                
                last_cow_start = ext->offset + len;
            }
        }
        
nextitem:
        le = le2;
    }
    
    if (length > 0) {
        UINT64 start_write = max(last_cow_start, start + written);
        
        Status = excise_extents(fcb->Vcb, fcb, start_write, end_data, Irp, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("excise_extents returned %08x\n", Status);
            return Status;
        }
        
        Status = insert_extent(fcb->Vcb, fcb, start_write, end_data - start_write, data, changed_sector_list, Irp, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("insert_extent returned %08x\n", Status);
            return Status;
        }
    }
    
    // FIXME - make extending work again (here?)
    // FIXME - make maximum extent size 128 MB again (here?)
    
#ifdef DEBUG_PARANOID
    last_off = 0xffffffffffffffff;
    
    le = fcb->extents.Flink;
    while (le != &fcb->extents) {
        extent* ext = CONTAINING_RECORD(le, extent, list_entry);
        
        if (!ext->ignore) {
            if (ext->offset == last_off) {
                ERR("offset %llx duplicated\n", ext->offset);
                int3;
            } else if (ext->offset < last_off && last_off != 0xffffffffffffffff) {
                ERR("offsets out of order\n");
                int3;
            }
            
            last_off = ext->offset;
        }
        
        le = le->Flink;
    }
#endif
    
    fcb->extents_changed = TRUE;
    mark_fcb_dirty(fcb);
    
    return STATUS_SUCCESS;
}

NTSTATUS write_compressed(fcb* fcb, UINT64 start_data, UINT64 end_data, void* data, LIST_ENTRY* changed_sector_list, PIRP Irp, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    UINT64 i;
    
    for (i = 0; i < sector_align(end_data - start_data, COMPRESSED_EXTENT_SIZE) / COMPRESSED_EXTENT_SIZE; i++) {
        UINT64 s2, e2;
        BOOL compressed;
        
        s2 = start_data + (i * COMPRESSED_EXTENT_SIZE);
        e2 = min(s2 + COMPRESSED_EXTENT_SIZE, end_data);
        
        Status = write_compressed_bit(fcb, s2, e2, (UINT8*)data + (i * COMPRESSED_EXTENT_SIZE), &compressed, changed_sector_list, Irp, rollback);
        
        if (!NT_SUCCESS(Status)) {
            ERR("write_compressed_bit returned %08x\n", Status);
            return Status;
        }
        
        // If the first 128 KB of a file is incompressible, we set the nocompress flag so we don't
        // bother with the rest of it.
        if (s2 == 0 && e2 == COMPRESSED_EXTENT_SIZE && !compressed && !fcb->Vcb->options.compress_force) {
            fcb->inode_item.flags |= BTRFS_INODE_NOCOMPRESS;
            mark_fcb_dirty(fcb);
            
            // write subsequent data non-compressed
            if (e2 < end_data) {
                Status = do_write_file(fcb, e2, end_data, (UINT8*)data + e2, changed_sector_list, Irp, rollback);
                
                if (!NT_SUCCESS(Status)) {
                    ERR("do_write_file returned %08x\n", Status);
                    return Status;
                }
            }
            
            return STATUS_SUCCESS;
        }
    }
    
    return STATUS_SUCCESS;
}

NTSTATUS write_file2(device_extension* Vcb, PIRP Irp, LARGE_INTEGER offset, void* buf, ULONG* length, BOOL paging_io, BOOL no_cache,
                     BOOL wait, BOOL deferred_write, LIST_ENTRY* rollback) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    EXTENT_DATA* ed2;
    UINT64 newlength, start_data, end_data;
    UINT32 bufhead;
    BOOL make_inline;
    UINT8* data;
    LIST_ENTRY changed_sector_list;
    INODE_ITEM* origii;
    BOOL changed_length = FALSE, nocsum/*, lazy_writer = FALSE, write_eof = FALSE*/;
    NTSTATUS Status;
    LARGE_INTEGER time;
    BTRFS_TIME now;
    fcb* fcb;
    ccb* ccb;
    file_ref* fileref;
    BOOL paging_lock = FALSE, fcb_lock = FALSE, tree_lock = FALSE, pagefile;
    ULONG filter = 0;
    
    TRACE("(%p, %p, %llx, %p, %x, %u, %u)\n", Vcb, FileObject, offset.QuadPart, buf, *length, paging_io, no_cache);
    
    if (*length == 0) {
        WARN("returning success for zero-length write\n");
        return STATUS_SUCCESS;
    }
    
    if (!FileObject) {
        ERR("error - FileObject was NULL\n");
        return STATUS_ACCESS_DENIED;
    }
    
    fcb = FileObject->FsContext;
    ccb = FileObject->FsContext2;
    fileref = ccb ? ccb->fileref : NULL;
    
    if (fcb->type != BTRFS_TYPE_FILE && fcb->type != BTRFS_TYPE_SYMLINK) {
        WARN("tried to write to something other than a file or symlink (inode %llx, type %u, %p, %p)\n", fcb->inode, fcb->type, &fcb->type, fcb);
        return STATUS_INVALID_DEVICE_REQUEST;
    }
    
    if (offset.LowPart == FILE_WRITE_TO_END_OF_FILE && offset.HighPart == -1) {
        offset = fcb->Header.FileSize;
//         write_eof = TRUE;
    }
    
    TRACE("fcb->Header.Flags = %x\n", fcb->Header.Flags);
    
    if (!no_cache && !CcCanIWrite(FileObject, *length, wait, deferred_write))
        return STATUS_PENDING;
    
    if (!wait && no_cache)
        return STATUS_PENDING;
    
    if (no_cache && !paging_io && FileObject->SectionObjectPointer->DataSectionObject) {
        IO_STATUS_BLOCK iosb;
        
        ExAcquireResourceExclusiveLite(fcb->Header.PagingIoResource, TRUE);

        CcFlushCache(FileObject->SectionObjectPointer, &offset, *length, &iosb);

        if (!NT_SUCCESS(iosb.Status)) {
            ExReleaseResourceLite(fcb->Header.PagingIoResource);
            ERR("CcFlushCache returned %08x\n", iosb.Status);
            return iosb.Status;
        }
        
        paging_lock = TRUE;

        CcPurgeCacheSection(FileObject->SectionObjectPointer, &offset, *length, FALSE);
    }
    
    if (paging_io) {
        if (!ExAcquireResourceSharedLite(fcb->Header.PagingIoResource, wait)) {
            Status = STATUS_PENDING;
            goto end;
        } else
            paging_lock = TRUE;
    }
    
    pagefile = fcb->Header.Flags2 & FSRTL_FLAG2_IS_PAGING_FILE && paging_io;
    
    if (!pagefile && !ExIsResourceAcquiredExclusiveLite(&Vcb->tree_lock)) {
        if (!ExAcquireResourceSharedLite(&Vcb->tree_lock, wait)) {
            Status = STATUS_PENDING;
            goto end;
        } else
            tree_lock = TRUE;
    }
        
    if (no_cache && !ExIsResourceAcquiredExclusiveLite(fcb->Header.Resource)) {
        if (!ExAcquireResourceExclusiveLite(fcb->Header.Resource, wait)) {
            Status = STATUS_PENDING;
            goto end;
        } else
            fcb_lock = TRUE;
    }
    
    nocsum = fcb->ads ? TRUE : fcb->inode_item.flags & BTRFS_INODE_NODATASUM;
    
    newlength = fcb->ads ? fcb->adsdata.Length : fcb->inode_item.st_size;
    
    if (fcb->deleted)
        newlength = 0;
    
    TRACE("newlength = %llx\n", newlength);
    
//     if (KeGetCurrentThread() == fcb->lazy_writer_thread) {
//         ERR("lazy writer on the TV\n");
//         lazy_writer = TRUE;
//     }
    
    if (offset.QuadPart + *length > newlength) {
        if (paging_io) {
            if (offset.QuadPart >= newlength) {
                TRACE("paging IO tried to write beyond end of file (file size = %llx, offset = %llx, length = %x)\n", newlength, offset.QuadPart, *length);
                TRACE("filename %S\n", file_desc(FileObject));
                TRACE("FileObject: AllocationSize = %llx, FileSize = %llx, ValidDataLength = %llx\n",
                    fcb->Header.AllocationSize.QuadPart, fcb->Header.FileSize.QuadPart, fcb->Header.ValidDataLength.QuadPart);
                Status = STATUS_SUCCESS;
                goto end;
            }
            
            *length = newlength - offset.QuadPart;
        } else {
            newlength = offset.QuadPart + *length;
            changed_length = TRUE;
            
            TRACE("extending length to %llx\n", newlength);
        }
    }
    
    make_inline = fcb->ads ? FALSE : newlength <= fcb->Vcb->options.max_inline;
    
    if (changed_length) {
        if (newlength > fcb->Header.AllocationSize.QuadPart) {
            if (!tree_lock) {
                // We need to acquire the tree lock if we don't have it already - 
                // we can't give an inline file proper extents at the same as we're
                // doing a flush.
                if (!ExAcquireResourceSharedLite(&Vcb->tree_lock, wait)) {
                    Status = STATUS_PENDING;
                    goto end;
                } else
                    tree_lock = TRUE;
            }
            
            Status = extend_file(fcb, fileref, newlength, FALSE, Irp, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("extend_file returned %08x\n", Status);
                goto end;
            }
        } else if (!fcb->ads)
            fcb->inode_item.st_size = newlength;
        
        fcb->Header.FileSize.QuadPart = newlength;
        fcb->Header.ValidDataLength.QuadPart = newlength;
        
        TRACE("AllocationSize = %llx\n", fcb->Header.AllocationSize.QuadPart);
        TRACE("FileSize = %llx\n", fcb->Header.FileSize.QuadPart);
        TRACE("ValidDataLength = %llx\n", fcb->Header.ValidDataLength.QuadPart);
    }
    
    if (!no_cache) {
        if (!FileObject->PrivateCacheMap || changed_length) {
            CC_FILE_SIZES ccfs;
            
            ccfs.AllocationSize = fcb->Header.AllocationSize;
            ccfs.FileSize = fcb->Header.FileSize;
            ccfs.ValidDataLength = fcb->Header.ValidDataLength;
            
            if (!FileObject->PrivateCacheMap) {
                TRACE("calling CcInitializeCacheMap...\n");
                CcInitializeCacheMap(FileObject, &ccfs, FALSE, cache_callbacks, FileObject);
                
                CcSetReadAheadGranularity(FileObject, READ_AHEAD_GRANULARITY);
            }
            
            CcSetFileSizes(FileObject, &ccfs);
        }
        
        if (IrpSp->MinorFunction & IRP_MN_MDL) {
            CcPrepareMdlWrite(FileObject, &offset, *length, &Irp->MdlAddress, &Irp->IoStatus);

            Status = Irp->IoStatus.Status;
            goto end;
        } else {
            TRACE("CcCopyWrite(%p, %llx, %x, %u, %p)\n", FileObject, offset.QuadPart, *length, wait, buf);
            if (!CcCopyWrite(FileObject, &offset, *length, wait, buf)) {
                Status = STATUS_PENDING;
                goto end;
            }
            TRACE("CcCopyWrite finished\n");
        }
        
        Status = STATUS_SUCCESS;
        goto end;
    }
    
    if (fcb->ads) {
        if (changed_length) {
            char* data2;
            
            if (newlength > fcb->adsmaxlen) {
                ERR("error - xattr too long (%llu > %u)\n", newlength, fcb->adsmaxlen);
                Status = STATUS_DISK_FULL;
                goto end;
            }

            data2 = ExAllocatePoolWithTag(PagedPool, newlength, ALLOC_TAG);
            if (!data2) {
                ERR("out of memory\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto end;
            }
            
            if (fcb->adsdata.Buffer) {
                RtlCopyMemory(data2, fcb->adsdata.Buffer, fcb->adsdata.Length);
                ExFreePool(fcb->adsdata.Buffer);
            }
            
            if (newlength > fcb->adsdata.Length)
                RtlZeroMemory(&data2[fcb->adsdata.Length], newlength - fcb->adsdata.Length);
            
            
            fcb->adsdata.Buffer = data2;
            fcb->adsdata.Length = fcb->adsdata.MaximumLength = newlength;
            
            fcb->Header.AllocationSize.QuadPart = newlength;
            fcb->Header.FileSize.QuadPart = newlength;
            fcb->Header.ValidDataLength.QuadPart = newlength;
        }
        
        if (*length > 0)
            RtlCopyMemory(&fcb->adsdata.Buffer[offset.QuadPart], buf, *length);
        
        fcb->Header.ValidDataLength.QuadPart = newlength;
        
        mark_fcb_dirty(fcb);
        
        if (fileref)
            mark_fileref_dirty(fileref);
    } else {
        BOOL compress = write_fcb_compressed(fcb);
        
        if (make_inline) {
            start_data = 0;
            end_data = sector_align(newlength, fcb->Vcb->superblock.sector_size);
            bufhead = sizeof(EXTENT_DATA) - 1;
        } else if (compress) {
            start_data = offset.QuadPart & ~(UINT64)(COMPRESSED_EXTENT_SIZE - 1);
            end_data = min(sector_align(offset.QuadPart + *length, COMPRESSED_EXTENT_SIZE),
                           sector_align(newlength, fcb->Vcb->superblock.sector_size));
            bufhead = 0;
        } else {
            start_data = offset.QuadPart & ~(UINT64)(fcb->Vcb->superblock.sector_size - 1);
            end_data = sector_align(offset.QuadPart + *length, fcb->Vcb->superblock.sector_size);
            bufhead = 0;
        }
            
        fcb->Header.ValidDataLength.QuadPart = newlength;
        TRACE("fcb %p FileSize = %llx\n", fcb, fcb->Header.FileSize.QuadPart);
    
        data = ExAllocatePoolWithTag(PagedPool, end_data - start_data + bufhead, ALLOC_TAG);
        if (!data) {
            ERR("out of memory\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }
        
        RtlZeroMemory(data + bufhead, end_data - start_data);
        
        TRACE("start_data = %llx\n", start_data);
        TRACE("end_data = %llx\n", end_data);
        
        if (offset.QuadPart > start_data || offset.QuadPart + *length < end_data) {
            if (changed_length) {
                if (fcb->inode_item.st_size > start_data) 
                    Status = read_file(fcb, data + bufhead, start_data, fcb->inode_item.st_size - start_data, NULL, Irp);
                else
                    Status = STATUS_SUCCESS;
            } else
                Status = read_file(fcb, data + bufhead, start_data, end_data - start_data, NULL, Irp);
            
            if (!NT_SUCCESS(Status)) {
                ERR("read_file returned %08x\n", Status);
                ExFreePool(data);
                goto end;
            }
        }
        
        RtlCopyMemory(data + bufhead + offset.QuadPart - start_data, buf, *length);
        
        if (!nocsum)
            InitializeListHead(&changed_sector_list);

        if (make_inline) {
            Status = excise_extents(fcb->Vcb, fcb, start_data, end_data, Irp, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("error - excise_extents returned %08x\n", Status);
                ExFreePool(data);
                goto end;
            }
            
            ed2 = (EXTENT_DATA*)data;
            ed2->generation = fcb->Vcb->superblock.generation;
            ed2->decoded_size = newlength;
            ed2->compression = BTRFS_COMPRESSION_NONE;
            ed2->encryption = BTRFS_ENCRYPTION_NONE;
            ed2->encoding = BTRFS_ENCODING_NONE;
            ed2->type = EXTENT_TYPE_INLINE;
            
            if (!add_extent_to_fcb(fcb, 0, ed2, sizeof(EXTENT_DATA) - 1 + newlength, FALSE, rollback)) {
                ERR("add_extent_to_fcb failed\n");
                ExFreePool(data);
                Status = STATUS_INTERNAL_ERROR;
                goto end;
            }
            
            fcb->inode_item.st_blocks += newlength;
        } else if (compress) {
            Status = write_compressed(fcb, start_data, end_data, data, nocsum ? NULL : &changed_sector_list, Irp, rollback);
            
            if (!NT_SUCCESS(Status)) {
                ERR("write_compressed returned %08x\n", Status);
                ExFreePool(data);
                goto end;
            }
            
            ExFreePool(data);
        } else {
            Status = do_write_file(fcb, start_data, end_data, data, nocsum ? NULL : &changed_sector_list, Irp, rollback);
            
            if (!NT_SUCCESS(Status)) {
                ERR("do_write_file returned %08x\n", Status);
                ExFreePool(data);
                goto end;
            }
            
            ExFreePool(data);
        }
    }
    
    if (!pagefile) {
        KeQuerySystemTime(&time);
        win_time_to_unix(time, &now);
    
//         ERR("no_cache = %s, FileObject->PrivateCacheMap = %p\n", no_cache ? "TRUE" : "FALSE", FileObject->PrivateCacheMap);
//         
//         if (!no_cache) {
//             if (!FileObject->PrivateCacheMap) {
//                 CC_FILE_SIZES ccfs;
//                 
//                 ccfs.AllocationSize = fcb->Header.AllocationSize;
//                 ccfs.FileSize = fcb->Header.FileSize;
//                 ccfs.ValidDataLength = fcb->Header.ValidDataLength;
//                 
//                 TRACE("calling CcInitializeCacheMap...\n");
//                 CcInitializeCacheMap(FileObject, &ccfs, FALSE, cache_callbacks, fcb);
//                 
//                 changed_length = FALSE;
//             }
//         }
        
        if (fcb->ads) {
            if (fileref && fileref->parent)
                origii = &fileref->parent->fcb->inode_item;
            else {
                ERR("no parent fcb found for stream\n");
                Status = STATUS_INTERNAL_ERROR;
                goto end;
            }
        } else
            origii = &fcb->inode_item;
        
        origii->transid = Vcb->superblock.generation;
        origii->sequence++;
        origii->st_ctime = now;
        
        if (!fcb->ads) {
            if (changed_length) {
                TRACE("setting st_size to %llx\n", newlength);
                origii->st_size = newlength;
                filter |= FILE_NOTIFY_CHANGE_SIZE;
            }
            
            origii->st_mtime = now;
            filter |= FILE_NOTIFY_CHANGE_LAST_WRITE;
        }
        
        mark_fcb_dirty(fcb->ads ? fileref->parent->fcb : fcb);
    }
    
    if (!nocsum) {
        ExAcquireResourceExclusiveLite(&Vcb->checksum_lock, TRUE);
        commit_checksum_changes(Vcb, &changed_sector_list);
        ExReleaseResourceLite(&Vcb->checksum_lock);
    }
    
    if (changed_length) {
        CC_FILE_SIZES ccfs;
            
        ccfs.AllocationSize = fcb->Header.AllocationSize;
        ccfs.FileSize = fcb->Header.FileSize;
        ccfs.ValidDataLength = fcb->Header.ValidDataLength;

        CcSetFileSizes(FileObject, &ccfs);
    }
    
    // FIXME - make sure this still called if STATUS_PENDING and async
//     if (!no_cache) {
//         if (!CcCopyWrite(FileObject, &offset, *length, TRUE, buf)) {
//             ERR("CcCopyWrite failed.\n");
//         }
//     }
    
    fcb->subvol->root_item.ctransid = Vcb->superblock.generation;
    fcb->subvol->root_item.ctime = now;
    
    Status = STATUS_SUCCESS;
    
    if (filter != 0)
        send_notification_fcb(fcb->ads ? fileref->parent : fileref, filter, FILE_ACTION_MODIFIED);
    
end:
    if (NT_SUCCESS(Status) && FileObject->Flags & FO_SYNCHRONOUS_IO && !paging_io) {
        TRACE("CurrentByteOffset was: %llx\n", FileObject->CurrentByteOffset.QuadPart);
        FileObject->CurrentByteOffset.QuadPart = offset.QuadPart + (NT_SUCCESS(Status) ? *length : 0);
        TRACE("CurrentByteOffset now: %llx\n", FileObject->CurrentByteOffset.QuadPart);
    }
    
    if (fcb_lock)
        ExReleaseResourceLite(fcb->Header.Resource);
    
    if (tree_lock)
        ExReleaseResourceLite(&Vcb->tree_lock);
    
    if (paging_lock)
        ExReleaseResourceLite(fcb->Header.PagingIoResource);

    return Status;
}

NTSTATUS write_file(device_extension* Vcb, PIRP Irp, BOOL wait, BOOL deferred_write) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    void* buf;
    NTSTATUS Status;
    LARGE_INTEGER offset = IrpSp->Parameters.Write.ByteOffset;
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    fcb* fcb = FileObject ? FileObject->FsContext : NULL;
//     BOOL locked = FALSE;
//     LARGE_INTEGER freq, time1, time2;
    LIST_ENTRY rollback;
    
    InitializeListHead(&rollback);
    
//     time1 = KeQueryPerformanceCounter(&freq);
    
    TRACE("write\n");
    
    Irp->IoStatus.Information = 0;
    
    TRACE("offset = %llx\n", offset.QuadPart);
    TRACE("length = %x\n", IrpSp->Parameters.Write.Length);
    
    if (!Irp->AssociatedIrp.SystemBuffer) {
        buf = map_user_buffer(Irp);
        
        if (Irp->MdlAddress && !buf) {
            ERR("MmGetSystemAddressForMdlSafe returned NULL\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto exit;
        }   
    } else
        buf = Irp->AssociatedIrp.SystemBuffer;
    
    TRACE("buf = %p\n", buf);
    
//     if (Irp->Flags & IRP_NOCACHE) {
//         if (!ExAcquireResourceSharedLite(&Vcb->tree_lock, wait)) {
//             Status = STATUS_PENDING;
//             goto exit;
//         }
//         locked = TRUE;
//     }
    
    if (fcb && !(Irp->Flags & IRP_PAGING_IO) && !FsRtlCheckLockForWriteAccess(&fcb->lock, Irp)) {
        WARN("tried to write to locked region\n");
        Status = STATUS_FILE_LOCK_CONFLICT;
        goto exit;
    }
    
//     ERR("Irp->Flags = %x\n", Irp->Flags);
    Status = write_file2(Vcb, Irp, offset, buf, &IrpSp->Parameters.Write.Length, Irp->Flags & IRP_PAGING_IO, Irp->Flags & IRP_NOCACHE,
                         wait, deferred_write, &rollback);
    
    if (Status == STATUS_PENDING)
        goto exit;
    else if (!NT_SUCCESS(Status)) {
        ERR("write_file2 returned %08x\n", Status);
        goto exit;
    }
    
//     if (locked)
//         Status = consider_write(Vcb);

    if (NT_SUCCESS(Status)) {
        Irp->IoStatus.Information = IrpSp->Parameters.Write.Length;
    
#ifdef DEBUG_PARANOID
//         if (locked)
//             check_extents_consistent(Vcb, FileObject->FsContext); // TESTING
    
//         check_extent_tree_consistent(Vcb);
#endif
    }
    
exit:
//     if (locked) {
        if (NT_SUCCESS(Status))
            clear_rollback(&rollback);
        else
            do_rollback(Vcb, &rollback);
//         
//         ExReleaseResourceLite(&Vcb->tree_lock);
//     }
    
//     time2 = KeQueryPerformanceCounter(NULL);
    
//     ERR("time = %u (freq = %u)\n", (UINT32)(time2.QuadPart - time1.QuadPart), (UINT32)freq.QuadPart);
    
    return Status;
}

NTSTATUS STDCALL drv_write(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp) {
    NTSTATUS Status;
    BOOL top_level;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    device_extension* Vcb = DeviceObject->DeviceExtension;
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    fcb* fcb = FileObject ? FileObject->FsContext : NULL;
    ccb* ccb = FileObject ? FileObject->FsContext2 : NULL;

    FsRtlEnterFileSystem();

    top_level = is_top_level(Irp);
    
    if (Vcb && Vcb->type == VCB_TYPE_PARTITION0) {
        Status = part0_passthrough(DeviceObject, Irp);
        goto exit;
    }
    
    if (!fcb) {
        ERR("fcb was NULL\n");
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }
    
    if (!ccb) {
        ERR("ccb was NULL\n");
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }
    
    if (fcb->subvol->root_item.flags & BTRFS_SUBVOL_READONLY) {
        Status = STATUS_ACCESS_DENIED;
        goto end;
    }
    
    if (Vcb->readonly) {
        Status = STATUS_MEDIA_WRITE_PROTECTED;
        goto end;
    }
    
    if (Irp->RequestorMode == UserMode && !(ccb->access & (FILE_WRITE_DATA | FILE_APPEND_DATA))) {
        WARN("insufficient permissions\n");
        Status = STATUS_ACCESS_DENIED;
        goto end;
    }
    
//     ERR("recursive = %s\n", Irp != IoGetTopLevelIrp() ? "TRUE" : "FALSE");
    
    _SEH2_TRY {
        if (IrpSp->MinorFunction & IRP_MN_COMPLETE) {
            CcMdlWriteComplete(IrpSp->FileObject, &IrpSp->Parameters.Write.ByteOffset, Irp->MdlAddress);
            
            Irp->MdlAddress = NULL;
            Status = STATUS_SUCCESS;
        } else {
            Status = write_file(Vcb, Irp, IoIsOperationSynchronous(Irp), FALSE);
        }
    } _SEH2_EXCEPT (EXCEPTION_EXECUTE_HANDLER) {
        Status = _SEH2_GetExceptionCode();
    } _SEH2_END;
    
end:
    Irp->IoStatus.Status = Status;

    TRACE("wrote %u bytes\n", Irp->IoStatus.Information);
    
    if (Status != STATUS_PENDING)
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    else {
        IoMarkIrpPending(Irp);
        
        if (!add_thread_job(Vcb, Irp))
            do_write_job(Vcb, Irp);
    }
    
exit:
    if (top_level) 
        IoSetTopLevelIrp(NULL);
    
    FsRtlExitFileSystem();
    
    TRACE("returning %08x\n", Status);

    return Status;
}
