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

typedef struct {
    CHUNK_ITEM ci;
    CHUNK_ITEM_STRIPE stripes[1];
} CHUNK_ITEM2;

static BOOL extent_item_is_shared(EXTENT_ITEM* ei, ULONG len);
static BOOL is_file_prealloc(fcb* fcb, UINT64 start_data, UINT64 end_data);

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
    
    // FIXME - work with RAID
    
    // FIXME - only write one superblock if on SSD (?)
    while (superblock_addrs[i] > 0 && Vcb->length >= superblock_addrs[i] + sizeof(superblock)) {
        TRACE("writing superblock %u\n", i);
        
        Vcb->superblock.sb_phys_addr = superblock_addrs[i];
        RtlCopyMemory(&Vcb->superblock.dev_item, &device->devitem, sizeof(DEV_ITEM));
        
        crc32 = calc_crc32c(0xffffffff, (UINT8*)&Vcb->superblock.uuid, (ULONG)sizeof(superblock) - sizeof(Vcb->superblock.checksum));
        crc32 = ~crc32;
        TRACE("crc32 is %08x\n", crc32);
        RtlCopyMemory(&Vcb->superblock.checksum, &crc32, sizeof(UINT32));
        
        Status = write_data_phys(device->devobj, superblock_addrs[i], &Vcb->superblock, sizeof(superblock));
        
        if (!NT_SUCCESS(Status))
            break;
        
        i++;
    }

    return Status;
}

static BOOL find_address_in_chunk(device_extension* Vcb, chunk* c, UINT64 length, UINT64* address) {
    LIST_ENTRY* le;
    space *s, *bestfit = NULL;
    
    TRACE("(%p, %llx, %llx, %p)\n", Vcb, c->offset, length, address);
    
    le = c->space.Flink;
    while (le != &c->space) {
        s = CONTAINING_RECORD(le, space, list_entry);
        
        if (s->type == SPACE_TYPE_FREE) {
            if (s->size == length) {
                *address = s->offset;
                TRACE("returning exact fit at %llx\n", s->offset);
                return TRUE;
            } else if (s->size > length && (!bestfit || bestfit->size > s->size)) {
                bestfit = s;
            }
        }
        
        le = le->Flink;
    }
    
    if (bestfit) {
        TRACE("returning best fit at %llx\n", bestfit->offset);
        *address = bestfit->offset;
        return TRUE;
    }
    
    return FALSE;
}

void add_to_space_list(chunk* c, UINT64 offset, UINT64 size, UINT8 type) {
    LIST_ENTRY *le = c->space.Flink, *nextle, *insbef;
    space *s, *s2, *s3;
#ifdef DEBUG_PARANOID
    UINT64 lastaddr;
#endif
    
    TRACE("(%p, %llx, %llx, %x)\n", c, offset, size, type);
    
#ifdef DEBUG_PARANOID
    // TESTING
    le = c->space.Flink;
    while (le != &c->space) {
        s = CONTAINING_RECORD(le, space, list_entry);
        
        TRACE("%llx,%llx,%x\n", s->offset, s->size, s->type);
        
        le = le->Flink;
    }
#endif
    
    c->space_changed = TRUE;
    
    le = c->space.Flink;
    insbef = &c->space;
    while (le != &c->space) {
        s = CONTAINING_RECORD(le, space, list_entry);
        nextle = le->Flink;
        
        if (s->offset >= offset + size) {
            insbef = le;
            break;
        }
        
        if (s->offset >= offset && s->offset + s->size <= offset + size) { // delete entirely
#ifndef __REACTOS__
            RemoveEntryList(&s->list_entry);
#endif
            
            if (s->offset + s->size == offset + size) {
                insbef = s->list_entry.Flink;
                RemoveEntryList(&s->list_entry);
                ExFreePool(s);
                break;
            }
            
            RemoveEntryList(&s->list_entry);
            ExFreePool(s);
        } else if (s->offset < offset && s->offset + s->size > offset + size) { // split in two
            s3 = ExAllocatePoolWithTag(PagedPool, sizeof(space), ALLOC_TAG);
            if (!s3) {
                ERR("out of memory\n");
                return;
            }
            
            s3->offset = offset + size;
            s3->size = s->size - size - offset + s->offset;
            s3->type = s->type;
            InsertHeadList(&s->list_entry, &s3->list_entry);
            insbef = &s3->list_entry;
            
            s->size = offset - s->offset;
            break;
        } else if (s->offset + s->size > offset && s->offset + s->size <= offset + size) { // truncate before
            s->size = offset - s->offset;
        } else if (s->offset < offset + size && s->offset + s->size > offset + size) { // truncate after
            s->size -= offset + size - s->offset;
            s->offset = offset + size;
            
            insbef = le;
            break;
        }
        
        le = nextle;
    }
    
    s2 = ExAllocatePoolWithTag(PagedPool, sizeof(space), ALLOC_TAG);
    if (!s2) {
        ERR("out of memory\n");
        return;
    }
    
    s2->offset = offset;
    s2->size = size;
    s2->type = type;
    InsertTailList(insbef, &s2->list_entry);
    
    // merge entries if same type
   
    if (s2->list_entry.Blink != &c->space) {
        s = CONTAINING_RECORD(s2->list_entry.Blink, space, list_entry);
        
        if (s->type == type) {
            s->size += s2->size;
            
            RemoveEntryList(&s2->list_entry);
            ExFreePool(s2);
            
            s2 = s;
        }
    }
    
    if (s2->list_entry.Flink != &c->space) {
        s = CONTAINING_RECORD(s2->list_entry.Flink, space, list_entry);
        
        if (s->type == type) {
            s2->size += s->size;

            RemoveEntryList(&s->list_entry);
            ExFreePool(s);
        }
    }
    
    le = c->space.Flink;
    while (le != &c->space) {
        s = CONTAINING_RECORD(le, space, list_entry);
        
        TRACE("%llx,%llx,%x\n", s->offset, s->size, s->type);
        
        le = le->Flink;
    }
    
#ifdef DEBUG_PARANOID
    // TESTING
    lastaddr = c->offset;
    le = c->space.Flink;
    while (le != &c->space) {
        s = CONTAINING_RECORD(le, space, list_entry);
        
        if (s->offset != lastaddr) {
            ERR("inconsistency detected!\n");
            int3;
        }
        
        lastaddr = s->offset + s->size;
        
        le = le->Flink;
    }
    
    if (lastaddr != c->offset + c->chunk_item->size) {
        ERR("inconsistency detected - space doesn't run all the way to end of chunk\n");
        int3;
    }
#endif
}

chunk* get_chunk_from_address(device_extension* Vcb, UINT64 address) {
    LIST_ENTRY* le2;
    chunk* c;
    
    le2 = Vcb->chunks.Flink;
    while (le2 != &Vcb->chunks) {
        c = CONTAINING_RECORD(le2, chunk, list_entry);
        
//         TRACE("chunk: %llx, %llx\n", c->offset, c->chunk_item->size);
        
        if (address >= c->offset && address < c->offset + c->chunk_item->size)
            return c;
         
        le2 = le2->Flink;
    }
    
    return NULL;
}

typedef struct {
    disk_hole* dh;
    device* device;
} stripe;

static void add_provisional_disk_hole(device_extension* Vcb, stripe* s, UINT64 max_stripe_size) {
//     LIST_ENTRY* le = s->device->disk_holes.Flink;
//     disk_hole* dh;

//     ERR("old holes:\n");
//     while (le != &s->device->disk_holes) {
//         dh = CONTAINING_RECORD(le, disk_hole, listentry);
//         
//         ERR("address %llx, size %llx, provisional %u\n", dh->address, dh->size, dh->provisional);
//         
//         le = le->Flink;
//     }
    
    if (s->dh->size <= max_stripe_size) {
        s->dh->provisional = TRUE;
    } else {
        disk_hole* newdh = ExAllocatePoolWithTag(PagedPool, sizeof(disk_hole), ALLOC_TAG);
        if (!newdh) {
            ERR("out of memory\n");
            return;
        }
        
        newdh->address = s->dh->address + max_stripe_size;
        newdh->size = s->dh->size - max_stripe_size;
        newdh->provisional = FALSE;
        InsertTailList(&s->device->disk_holes, &newdh->listentry);
        
        s->dh->size = max_stripe_size;
        s->dh->provisional = TRUE;
    }
    
//     ERR("new holes:\n");
//     le = s->device->disk_holes.Flink;
//     while (le != &s->device->disk_holes) {
//         dh = CONTAINING_RECORD(le, disk_hole, listentry);
//         
//         ERR("address %llx, size %llx, provisional %u\n", dh->address, dh->size, dh->provisional);
//         
//         le = le->Flink;
//     }
}

static UINT64 find_new_chunk_address(device_extension* Vcb, UINT64 size) {
    KEY searchkey;
    traverse_ptr tp, next_tp;
    BOOL b;
    UINT64 lastaddr;
    NTSTATUS Status;
    
    searchkey.obj_id = 0x100;
    searchkey.obj_type = TYPE_CHUNK_ITEM;
    searchkey.offset = 0;
    
    Status = find_item(Vcb, Vcb->chunk_root, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return 0xffffffffffffffff;
    }
    
    lastaddr = 0;
    
    do {
        if (tp.item->key.obj_type == TYPE_CHUNK_ITEM) {
            if (tp.item->size < sizeof(CHUNK_ITEM)) {
                ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(CHUNK_ITEM));
            } else {
                CHUNK_ITEM* ci = (CHUNK_ITEM*)tp.item->data;
                
                if (tp.item->key.offset >= lastaddr + size)
                    return lastaddr;
                
                lastaddr = tp.item->key.offset + ci->size;
            }
        }
        
        b = find_next_item(Vcb, &tp, &next_tp, FALSE);
        if (b) {
            tp = next_tp;
            
            if (tp.item->key.obj_id > searchkey.obj_id || tp.item->key.obj_type > searchkey.obj_type)
                break;
        }
    } while (b);
    
    return lastaddr;
}

static BOOL increase_dev_item_used(device_extension* Vcb, device* device, UINT64 size, LIST_ENTRY* rollback) {
    KEY searchkey;
    traverse_ptr tp;
    DEV_ITEM* di;
    NTSTATUS Status;
    
    searchkey.obj_id = 1;
    searchkey.obj_type = TYPE_DEV_ITEM;
    searchkey.offset = device->devitem.dev_id;
    
    Status = find_item(Vcb, Vcb->chunk_root, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return FALSE;
    }
    
    if (keycmp(&tp.item->key, &searchkey)) {
        ERR("error - could not find DEV_ITEM for device %llx\n", device->devitem.dev_id);
        return FALSE;
    }
    
    delete_tree_item(Vcb, &tp, rollback);
    
    device->devitem.bytes_used += size;
    
    di = ExAllocatePoolWithTag(PagedPool, sizeof(DEV_ITEM), ALLOC_TAG);
    if (!di) {
        ERR("out of memory\n");
        return FALSE;
    }
    
    RtlCopyMemory(di, &device->devitem, sizeof(DEV_ITEM));
    
    if (!insert_tree_item(Vcb, Vcb->chunk_root, 1, TYPE_DEV_ITEM, device->devitem.dev_id, di, sizeof(DEV_ITEM), NULL, rollback)) {
        ERR("insert_tree_item failed\n");
        return FALSE;
    }
    
    return TRUE;
}

static void reset_disk_holes(device* device, BOOL commit) {
    LIST_ENTRY* le = device->disk_holes.Flink;
    disk_hole* dh;

//     ERR("old holes:\n");
//     while (le != &device->disk_holes) {
//         dh = CONTAINING_RECORD(le, disk_hole, listentry);
//         
//         ERR("address %llx, size %llx, provisional %u\n", dh->address, dh->size, dh->provisional);
//         
//         le = le->Flink;
//     }
    
    le = device->disk_holes.Flink;
    while (le != &device->disk_holes) {
        LIST_ENTRY* le2 = le->Flink;
        
        dh = CONTAINING_RECORD(le, disk_hole, listentry);
        
        if (dh->provisional) {
            if (commit) {
                RemoveEntryList(le);
                ExFreePool(dh);
            } else {
                dh->provisional = FALSE;
            }
        }
        
        le = le2;
    }
    
    if (!commit) {
        le = device->disk_holes.Flink;
        while (le != &device->disk_holes) {
            LIST_ENTRY* le2 = le->Flink;
            
            dh = CONTAINING_RECORD(le, disk_hole, listentry);
            
            while (le2 != &device->disk_holes) {
                disk_hole* dh2 = CONTAINING_RECORD(le2, disk_hole, listentry);
                
                if (dh2->address == dh->address + dh->size) {
                    LIST_ENTRY* le3 = le2->Flink;
                    dh->size += dh2->size;
                    
                    RemoveEntryList(le2);
                    ExFreePool(dh2);
                    
                    le2 = le3;
                } else
                    break;
            }
            
            le = le->Flink;
        }
    }
        
//     ERR("new holes:\n");
//     le = device->disk_holes.Flink;
//     while (le != &device->disk_holes) {
//         dh = CONTAINING_RECORD(le, disk_hole, listentry);
//         
//         ERR("address %llx, size %llx, provisional %u\n", dh->address, dh->size, dh->provisional);
//         
//         le = le->Flink;
//     }
}

static NTSTATUS add_to_bootstrap(device_extension* Vcb, UINT64 obj_id, UINT8 obj_type, UINT64 offset, void* data, ULONG size) {
    sys_chunk *sc, *sc2;
    LIST_ENTRY* le;
    USHORT i;
    
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
    
    return STATUS_SUCCESS;
}

chunk* alloc_chunk(device_extension* Vcb, UINT64 flags, LIST_ENTRY* rollback) {
    UINT64 max_stripe_size, max_chunk_size, stripe_size;
    UINT64 total_size = 0, i, j, logaddr;
    int num_stripes;
    disk_hole* dh;
    stripe* stripes;
    ULONG cisize;
    CHUNK_ITEM* ci;
    CHUNK_ITEM_STRIPE* cis;
    chunk* c = NULL;
    space* s = NULL;
    BOOL success = FALSE;
    BLOCK_GROUP_ITEM* bgi;
    
    for (i = 0; i < Vcb->superblock.num_devices; i++) {
        total_size += Vcb->devices[i].devitem.num_bytes;
    }
    TRACE("total_size = %llx\n", total_size);
    
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
    
    // FIXME - make sure whole number of sectors?
    max_chunk_size = min(max_chunk_size, total_size / 10); // cap at 10%
    
    TRACE("would allocate a new chunk of %llx bytes and stripe %llx\n", max_chunk_size, max_stripe_size);
    
    if (flags & BLOCK_FLAG_DUPLICATE) {
        num_stripes = 2;
    } else if (flags & BLOCK_FLAG_RAID0) {
        FIXME("RAID0 not yet supported\n");
        return NULL;
    } else if (flags & BLOCK_FLAG_RAID1) {
        FIXME("RAID1 not yet supported\n");
        return NULL;
    } else if (flags & BLOCK_FLAG_RAID10) {
        FIXME("RAID10 not yet supported\n");
        return NULL;
    } else if (flags & BLOCK_FLAG_RAID5) {
        FIXME("RAID5 not yet supported\n");
        return NULL;
    } else if (flags & BLOCK_FLAG_RAID6) {
        FIXME("RAID6 not yet supported\n");
        return NULL;
    } else { // SINGLE
        num_stripes = 1;
    }
    
    stripes = ExAllocatePoolWithTag(PagedPool, sizeof(stripe) * num_stripes, ALLOC_TAG);
    if (!stripes) {
        ERR("out of memory\n");
        return NULL;
    }
    
    for (i = 0; i < num_stripes; i++) {
        stripes[i].dh = NULL;
        
        for (j = 0; j < Vcb->superblock.num_devices; j++) {
            LIST_ENTRY* le = Vcb->devices[j].disk_holes.Flink;

            while (le != &Vcb->devices[j].disk_holes) {
                dh = CONTAINING_RECORD(le, disk_hole, listentry);
                
                if (!dh->provisional) {
                    if (!stripes[i].dh || dh->size > stripes[i].dh->size) {
                        stripes[i].dh = dh;
                        stripes[i].device = &Vcb->devices[j];
                        
                        if (stripes[i].dh->size >= max_stripe_size)
                            break;
                    }
                }

                le = le->Flink;
            }
            
            if (stripes[i].dh && stripes[i].dh->size >= max_stripe_size)
                break;
        }
        
        if (stripes[i].dh) {
            TRACE("good DH: device %llx, address %llx, size %llx\n", stripes[i].device->devitem.dev_id, stripes[i].dh->address, stripes[i].dh->size);
        } else {
            TRACE("good DH not found\n");
            goto end;
        }
        
        add_provisional_disk_hole(Vcb, &stripes[i], max_stripe_size);
    }
    
    stripe_size = min(stripes[0].dh->size, max_stripe_size);
    for (i = 1; i < num_stripes; i++) {
        stripe_size = min(stripe_size, stripes[1].dh->size);
    }
    // FIXME - make sure stripe_size aligned properly
    // FIXME - obey max_chunk_size
    
    c = ExAllocatePoolWithTag(PagedPool, sizeof(chunk), ALLOC_TAG);
    if (!c) {
        ERR("out of memory\n");
        goto end;
    }
    
    // add CHUNK_ITEM to tree 3
    
    cisize = sizeof(CHUNK_ITEM) + (num_stripes * sizeof(CHUNK_ITEM_STRIPE));
    ci = ExAllocatePoolWithTag(PagedPool, cisize, ALLOC_TAG);
    if (!ci) {
        ERR("out of memory\n");
        goto end;
    }
    
    ci->size = stripe_size; // FIXME for RAID
    ci->root_id = Vcb->extent_root->id;
    ci->stripe_length = 0x10000; // FIXME? BTRFS_STRIPE_LEN in kernel
    ci->type = flags;
    ci->opt_io_alignment = ci->stripe_length;
    ci->opt_io_width = ci->stripe_length;
    ci->sector_size = stripes[0].device->devitem.minimal_io_size;
    ci->num_stripes = num_stripes;
    ci->sub_stripes = 1;
    
    c->devices = ExAllocatePoolWithTag(PagedPool, sizeof(device*) * num_stripes, ALLOC_TAG);
    if (!c->devices) {
        ERR("out of memory\n");
        ExFreePool(ci);
        goto end;
    }

    for (i = 0; i < num_stripes; i++) {
        if (i == 0)
            cis = (CHUNK_ITEM_STRIPE*)&ci[1];
        else
            cis = &cis[1];
        
        cis->dev_id = stripes[i].device->devitem.dev_id;
        cis->offset = stripes[i].dh->address;
        cis->dev_uuid = stripes[i].device->devitem.device_uuid;
        
        c->devices[i] = stripes[i].device;
    }
    
    logaddr = find_new_chunk_address(Vcb, ci->size);
    if (logaddr == 0xffffffffffffffff) {
        ERR("find_new_chunk_address failed\n");
        ExFreePool(ci);
        goto end;
    }
    
    if (!insert_tree_item(Vcb, Vcb->chunk_root, 0x100, TYPE_CHUNK_ITEM, logaddr, ci, cisize, NULL, rollback)) {
        ERR("insert_tree_item failed\n");
        ExFreePool(ci);
        goto end;
    }
    
    if (flags & BLOCK_FLAG_SYSTEM) {
        NTSTATUS Status = add_to_bootstrap(Vcb, 0x100, TYPE_CHUNK_ITEM, logaddr, ci, cisize);
        if (!NT_SUCCESS(Status)) {
            ERR("add_to_bootstrap returned %08x\n", Status);
            goto end;
        }
    }

    Vcb->superblock.chunk_root_generation = Vcb->superblock.generation;
    
    c->chunk_item = ExAllocatePoolWithTag(PagedPool, cisize, ALLOC_TAG);
    if (!c->chunk_item) {
        ERR("out of memory\n");
        goto end;
    }
    
    RtlCopyMemory(c->chunk_item, ci, cisize);
    c->size = cisize;
    c->offset = logaddr;
    c->used = c->oldused = 0;
    c->space_changed = FALSE;
    c->cache_size = 0;
    c->cache_inode = 0;
    InitializeListHead(&c->space);
    
    s = ExAllocatePoolWithTag(PagedPool, sizeof(space), ALLOC_TAG);
    if (!s) {
        ERR("out of memory\n");
        goto end;
    }
    
    s->offset = c->offset;
    s->size = c->chunk_item->size;
    s->type = SPACE_TYPE_FREE;
    InsertTailList(&c->space, &s->list_entry);
    
    protect_superblocks(Vcb, c);
    
    // add BLOCK_GROUP_ITEM to tree 2
    
    bgi = ExAllocatePoolWithTag(PagedPool, sizeof(BLOCK_GROUP_ITEM), ALLOC_TAG);
    if (!bgi) {
        ERR("out of memory\n");
        goto end;
    }
        
    bgi->used = 0;
    bgi->chunk_tree = 0x100;
    bgi->flags = flags;
    
    if (!insert_tree_item(Vcb, Vcb->extent_root, logaddr, TYPE_BLOCK_GROUP_ITEM, ci->size, bgi, sizeof(BLOCK_GROUP_ITEM), NULL, rollback)) {
        ERR("insert_tree_item failed\n");
        ExFreePool(bgi);
        goto end;
    }
    
    // add DEV_EXTENTs to tree 4
    
    for (i = 0; i < num_stripes; i++) {
        DEV_EXTENT* de;
        
        de = ExAllocatePoolWithTag(PagedPool, sizeof(DEV_EXTENT), ALLOC_TAG);
        if (!de) {
            ERR("out of memory\n");
            goto end;
        }
        
        de->chunktree = Vcb->chunk_root->id;
        de->objid = 0x100;
        de->address = logaddr;
        de->length = ci->size;
        de->chunktree_uuid = Vcb->chunk_root->treeholder.tree->header.chunk_tree_uuid;
        
        if (!insert_tree_item(Vcb, Vcb->dev_root, stripes[i].device->devitem.dev_id, TYPE_DEV_EXTENT, stripes[i].dh->address, de, sizeof(DEV_EXTENT), NULL, rollback)) {
            ERR("insert_tree_item failed\n");
            ExFreePool(de);
            goto end;
        }
        
        if (!increase_dev_item_used(Vcb, stripes[i].device, ci->size, rollback)) {
            ERR("increase_dev_item_used failed\n");
            goto end;
        }
    }
    
    for (i = 0; i < num_stripes; i++) {
        BOOL b = FALSE;
        for (j = 0; j < i; j++) {
            if (stripes[j].device == stripes[i].device)
                b = TRUE;
        }
        
        if (!b)
            reset_disk_holes(stripes[i].device, TRUE);
    }
    
    success = TRUE;
    
end:
    ExFreePool(stripes);
    
    if (!success) {
        for (i = 0; i < num_stripes; i++) {
            BOOL b = FALSE;
            for (j = 0; j < i; j++) {
                if (stripes[j].device == stripes[i].device)
                    b = TRUE;
            }
            
            if (!b)
                reset_disk_holes(stripes[i].device, FALSE);
        }
        
        if (c) ExFreePool(c);
        if (s) ExFreePool(s);
    } else
        InsertTailList(&Vcb->chunks, &c->list_entry);

    return success ? c : NULL;
}

NTSTATUS STDCALL write_data(device_extension* Vcb, UINT64 address, void* data, UINT32 length) {
    KEY searchkey;
    traverse_ptr tp;
    CHUNK_ITEM2* ci;
    NTSTATUS Status;
    UINT32 i;
    
    TRACE("(%p, %llx, %p, %x)\n", Vcb, address, data, length);
    
    // FIXME - use version cached in Vcb
    
    searchkey.obj_id = 0x100; // fixed?
    searchkey.obj_type = TYPE_CHUNK_ITEM;
    searchkey.offset = address;
    
    Status = find_item(Vcb, Vcb->chunk_root, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    if (tp.item->key.obj_id != searchkey.obj_id || tp.item->key.obj_type != searchkey.obj_type) {
        ERR("error - unexpected item in chunk tree\n");
        Status = STATUS_INTERNAL_ERROR;
        goto end;
    }
    
    if (tp.item->size < sizeof(CHUNK_ITEM2)) {
        ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(CHUNK_ITEM2));
        Status = STATUS_INTERNAL_ERROR;
        goto end;
    }
    
    ci = (CHUNK_ITEM2*)tp.item->data;
    
    if (tp.item->key.offset > address || tp.item->key.offset + ci->ci.size < address) {
        ERR("error - address %llx was out of chunk bounds\n", address);
        Status = STATUS_INTERNAL_ERROR;
        goto end;
    }
    
    // FIXME - only do this for chunks marked DUPLICATE?
    // FIXME - for multiple writes, if PENDING do waits at the end
    // FIXME - work with RAID
    for (i = 0; i < ci->ci.num_stripes; i++) {
        Status = write_data_phys(Vcb->devices[0].devobj, address - tp.item->key.offset + ci->stripes[i].offset, data, length);
        if (!NT_SUCCESS(Status)) {
            ERR("error - write_data_phys failed\n");
            goto end;
        }
    }
    
end:
    
    return Status;
}

static void clean_space_cache_chunk(device_extension* Vcb, chunk* c) {
    LIST_ENTRY *le, *nextle;
    space *s, *s2;
    
//     // TESTING
//     le = c->space.Flink;
//     while (le != &c->space) {
//         s = CONTAINING_RECORD(le, space, list_entry);
//         
//         TRACE("%x,%x,%x\n", (UINT32)s->offset, (UINT32)s->size, s->type);
//         
//         le = le->Flink;
//     }
    
    le = c->space.Flink;
    while (le != &c->space) {
        s = CONTAINING_RECORD(le, space, list_entry);
        nextle = le->Flink;
        
        if (s->type == SPACE_TYPE_DELETING)
            s->type = SPACE_TYPE_FREE;
        else if (s->type == SPACE_TYPE_WRITING)
            s->type = SPACE_TYPE_USED;
        
        if (le->Blink != &c->space) {
            s2 = CONTAINING_RECORD(le->Blink, space, list_entry);
            
            if (s2->type == s->type) { // do merge
                s2->size += s->size;
                
                RemoveEntryList(&s->list_entry);
                ExFreePool(s);
            }
        }

        le = nextle;
    }
    
//     le = c->space.Flink;
//     while (le != &c->space) {
//         s = CONTAINING_RECORD(le, space, list_entry);
//         
//         TRACE("%x,%x,%x\n", (UINT32)s->offset, (UINT32)s->size, s->type);
//         
//         le = le->Flink;
//     }
}

static void clean_space_cache(device_extension* Vcb) {
    LIST_ENTRY* le;
    chunk* c;
    
    TRACE("(%p)\n", Vcb);
    
    le = Vcb->chunks.Flink;
    while (le != &Vcb->chunks) {
        c = CONTAINING_RECORD(le, chunk, list_entry);
        
        if (c->space_changed) {
            clean_space_cache_chunk(Vcb, c);
            c->space_changed = FALSE;
        }
        
        le = le->Flink;
    }
}

static BOOL trees_consistent(device_extension* Vcb, LIST_ENTRY* rollback) {
    ULONG maxsize = Vcb->superblock.node_size - sizeof(tree_header);
    LIST_ENTRY* le;
    
    le = Vcb->trees.Flink;
    while (le != &Vcb->trees) {
        tree* t = CONTAINING_RECORD(le, tree, list_entry);
        
        if (t->write) {
            if (t->header.num_items == 0 && t->parent)
                return FALSE;
            
            if (t->size > maxsize)
                return FALSE;
            
            if (!t->has_new_address)
                return FALSE;
        }
        
        le = le->Flink;
    }
    
    return TRUE;
}

static NTSTATUS add_parents(device_extension* Vcb, LIST_ENTRY* rollback) {
    LIST_ENTRY* le;
    NTSTATUS Status;
    
    le = Vcb->trees.Flink;
    while (le != &Vcb->trees) {
        tree* t = CONTAINING_RECORD(le, tree, list_entry);
        
        if (t->write) {
            if (t->parent) {
                if (!t->parent->write) {
                    t->parent->write = TRUE;
                    Vcb->write_trees++;
                }
            } else if (t->root != Vcb->chunk_root && t->root != Vcb->root_root) {
                KEY searchkey;
                traverse_ptr tp;
                
                searchkey.obj_id = t->root->id;
                searchkey.obj_type = TYPE_ROOT_ITEM;
                searchkey.offset = 0xffffffffffffffff;
                
                Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE);
                if (!NT_SUCCESS(Status)) {
                    ERR("error - find_item returned %08x\n", Status);
                    return Status;
                }
                
                if (tp.item->key.obj_id != searchkey.obj_id || tp.item->key.obj_type != searchkey.obj_type) {
                    ERR("could not find ROOT_ITEM for tree %llx\n", searchkey.obj_id);
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
                    
                    if (!insert_tree_item(Vcb, Vcb->root_root, searchkey.obj_id, searchkey.obj_type, tp.item->key.offset, ri, sizeof(ROOT_ITEM), NULL, rollback)) {
                        ERR("insert_tree_item failed\n");
                        return STATUS_INTERNAL_ERROR;
                    }
                } else {
                    if (!tp.tree->write) {
                        tp.tree->write = TRUE;
                        Vcb->write_trees++;
                    }
                }
            }
        }
        
        le = le->Flink;
    }

    return STATUS_SUCCESS;
}

static void add_parents_to_cache(device_extension* Vcb, tree* t) {
    KEY searchkey;
    traverse_ptr tp;
    NTSTATUS Status;
    
    while (t->parent) {
        t = t->parent;
        
        if (!t->write) {
            t->write = TRUE;
            Vcb->write_trees++;
        }
    }
    
    if (t->root == Vcb->root_root || t->root == Vcb->chunk_root)
        return;
    
    searchkey.obj_id = t->root->id;
    searchkey.obj_type = TYPE_ROOT_ITEM;
    searchkey.offset = 0xffffffffffffffff;
    
    Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return;
    }
    
    if (tp.item->key.obj_id != searchkey.obj_id || tp.item->key.obj_type != searchkey.obj_type) {
        ERR("could not find ROOT_ITEM for tree %llx\n", searchkey.obj_id);
        return;
    }
    
    if (!tp.tree->write) {
        tp.tree->write = TRUE;
        Vcb->write_trees++;
    }
}

static BOOL insert_tree_extent_skinny(device_extension* Vcb, UINT8 level, UINT64 root_id, chunk* c, UINT64 address, LIST_ENTRY* rollback) {
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
    
    if (!insert_tree_item(Vcb, Vcb->extent_root, address, TYPE_METADATA_ITEM, level, eism, sizeof(EXTENT_ITEM_SKINNY_METADATA), &insert_tp, rollback)) {
        ERR("insert_tree_item failed\n");
        ExFreePool(eism);
        return FALSE;
    }
    
    add_to_space_list(c, address, Vcb->superblock.node_size, SPACE_TYPE_WRITING);

    add_parents_to_cache(Vcb, insert_tp.tree);
    
    return TRUE;
}

static BOOL insert_tree_extent(device_extension* Vcb, UINT8 level, UINT64 root_id, chunk* c, UINT64* new_address, LIST_ENTRY* rollback) {
    UINT64 address;
    EXTENT_ITEM_TREE2* eit2;
    traverse_ptr insert_tp;
    
    TRACE("(%p, %x, %llx, %p, %p, %p, %p)\n", Vcb, level, root_id, c, new_address, rollback);
    
    if (!find_address_in_chunk(Vcb, c, Vcb->superblock.node_size, &address))
        return FALSE;
    
    if (Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA) {
        BOOL b = insert_tree_extent_skinny(Vcb, level, root_id, c, address, rollback);
        
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
    
    if (!insert_tree_item(Vcb, Vcb->extent_root, address, TYPE_EXTENT_ITEM, Vcb->superblock.node_size, eit2, sizeof(EXTENT_ITEM_TREE2), &insert_tp, rollback)) {
        ERR("insert_tree_item failed\n");
        ExFreePool(eit2);
        return FALSE;
    }
    
    add_to_space_list(c, address, Vcb->superblock.node_size, SPACE_TYPE_WRITING);

    add_parents_to_cache(Vcb, insert_tp.tree);
    
    *new_address = address;
    
    return TRUE;
}

NTSTATUS get_tree_new_address(device_extension* Vcb, tree* t, LIST_ENTRY* rollback) {
    chunk *origchunk = NULL, *c;
    LIST_ENTRY* le;
    UINT64 flags = t->flags, addr;
    
    if (flags == 0)
        flags = (t->root->id == BTRFS_ROOT_CHUNK ? BLOCK_FLAG_SYSTEM : BLOCK_FLAG_METADATA) | BLOCK_FLAG_DUPLICATE;
    
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
        
        if (insert_tree_extent(Vcb, t->header.level, t->header.tree_id, origchunk, &addr, rollback)) {
            t->new_address = addr;
            t->has_new_address = TRUE;
            return STATUS_SUCCESS;
        }
    }
    
    le = Vcb->chunks.Flink;
    while (le != &Vcb->chunks) {
        c = CONTAINING_RECORD(le, chunk, list_entry);
        
        if (c != origchunk && c->chunk_item->type == flags && (c->chunk_item->size - c->used) >= Vcb->superblock.node_size) {
            if (insert_tree_extent(Vcb, t->header.level, t->header.tree_id, c, &addr, rollback)) {
                t->new_address = addr;
                t->has_new_address = TRUE;
                return STATUS_SUCCESS;
            }
        }

        le = le->Flink;
    }
    
    // allocate new chunk if necessary
    if ((c = alloc_chunk(Vcb, flags, rollback))) {
        if ((c->chunk_item->size - c->used) >= Vcb->superblock.node_size) {
            if (insert_tree_extent(Vcb, t->header.level, t->header.tree_id, c, &addr, rollback)) {
                t->new_address = addr;
                t->has_new_address = TRUE;
                return STATUS_SUCCESS;
            }
        }
    }
    
    ERR("couldn't find any metadata chunks with %x bytes free\n", Vcb->superblock.node_size);

    return STATUS_DISK_FULL;
}

static BOOL reduce_tree_extent_skinny(device_extension* Vcb, UINT64 address, tree* t, LIST_ENTRY* rollback) {
    KEY searchkey;
    traverse_ptr tp;
    chunk* c;
    EXTENT_ITEM_SKINNY_METADATA* eism;
    NTSTATUS Status;
    
    searchkey.obj_id = address;
    searchkey.obj_type = TYPE_METADATA_ITEM;
    searchkey.offset = 0xffffffffffffffff;
    
    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE);
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
    
    eism = (EXTENT_ITEM_SKINNY_METADATA*)tp.item->data;
    if (t && t->header.level == 0 && eism->ei.flags & EXTENT_ITEM_SHARED_BACKREFS && eism->type == TYPE_TREE_BLOCK_REF) {
        // convert shared data extents
        
        LIST_ENTRY* le = t->itemlist.Flink;
        while (le != &t->itemlist) {
            tree_data* td = CONTAINING_RECORD(le, tree_data, list_entry);
            
            TRACE("%llx,%x,%llx\n", td->key.obj_id, td->key.obj_type, td->key.offset);
            
            if (!td->ignore && !td->inserted) {
                if (td->key.obj_type == TYPE_EXTENT_DATA) {
                    EXTENT_DATA* ed = (EXTENT_DATA*)td->data;
                    
                    if (ed->type == EXTENT_TYPE_REGULAR || ed->type == EXTENT_TYPE_PREALLOC) {
                        EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ed->data;
                        
                        if (ed2->address != 0) {
                            TRACE("trying to convert shared data extent %llx,%llx\n", ed2->address, ed2->size);
                            convert_shared_data_extent(Vcb, ed2->address, ed2->size, rollback);
                        }
                    }
                }
            }

            le = le->Flink;
        }
        
        t->header.flags &= ~HEADER_FLAG_SHARED_BACKREF;
    }

    c = get_chunk_from_address(Vcb, address);
    
    if (c) {
        decrease_chunk_usage(c, Vcb->superblock.node_size);
        
        add_to_space_list(c, address, Vcb->superblock.node_size, SPACE_TYPE_DELETING);
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

static void convert_old_tree_extent(device_extension* Vcb, tree_data* td, tree* t, LIST_ENTRY* rollback) {
    KEY searchkey;
    traverse_ptr tp, tp2, insert_tp;
    EXTENT_REF_V0* erv0;
    NTSTATUS Status;
    
    TRACE("(%p, %p, %p)\n", Vcb, td, t);
    
    searchkey.obj_id = td->treeholder.address;
    searchkey.obj_type = TYPE_EXTENT_REF_V0;
    searchkey.offset = 0xffffffffffffffff;
    
    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE);
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
    
    Status = find_item(Vcb, Vcb->extent_root, &tp2, &searchkey, FALSE);
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
        
        if (!insert_tree_item(Vcb, Vcb->extent_root, td->treeholder.address, TYPE_METADATA_ITEM, t->header.level -1, eism, sizeof(EXTENT_ITEM_SKINNY_METADATA), &insert_tp, rollback)) {
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

        if (!insert_tree_item(Vcb, Vcb->extent_root, td->treeholder.address, TYPE_EXTENT_ITEM, Vcb->superblock.node_size, eit2, sizeof(EXTENT_ITEM_TREE2), &insert_tp, rollback)) {
            ERR("insert_tree_item failed\n");
            return;
        }
    }
    
    add_parents_to_cache(Vcb, insert_tp.tree);
    add_parents_to_cache(Vcb, tp.tree);
    add_parents_to_cache(Vcb, tp2.tree);
}

static NTSTATUS reduce_tree_extent(device_extension* Vcb, UINT64 address, tree* t, LIST_ENTRY* rollback) {
    KEY searchkey;
    traverse_ptr tp;
    EXTENT_ITEM* ei;
    EXTENT_ITEM_V0* eiv0;
    chunk* c;
    NTSTATUS Status;
    
    // FIXME - deal with refcounts > 1
    
    TRACE("(%p, %llx, %p)\n", Vcb, address, t);
    
    if (Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA) {
        if (reduce_tree_extent_skinny(Vcb, address, t, rollback)) {
            return STATUS_SUCCESS;
        }
    }
    
    searchkey.obj_id = address;
    searchkey.obj_type = TYPE_EXTENT_ITEM;
    searchkey.offset = Vcb->superblock.node_size;
    
    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE);
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
        
        if (t && t->header.level == 0 && ei->flags & EXTENT_ITEM_SHARED_BACKREFS) {
            // convert shared data extents
            
            LIST_ENTRY* le = t->itemlist.Flink;
            while (le != &t->itemlist) {
                tree_data* td = CONTAINING_RECORD(le, tree_data, list_entry);
                
                TRACE("%llx,%x,%llx\n", td->key.obj_id, td->key.obj_type, td->key.offset);
                
                if (!td->ignore && !td->inserted) {
                    if (td->key.obj_type == TYPE_EXTENT_DATA) {
                        EXTENT_DATA* ed = (EXTENT_DATA*)td->data;
                        
                        if (ed->type == EXTENT_TYPE_REGULAR || ed->type == EXTENT_TYPE_PREALLOC) {
                            EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ed->data;
                            
                            if (ed2->address != 0) {
                                TRACE("trying to convert shared data extent %llx,%llx\n", ed2->address, ed2->size);
                                convert_shared_data_extent(Vcb, ed2->address, ed2->size, rollback);
                            }
                        }
                    }
                }
    
                le = le->Flink;
            }
            
            t->header.flags &= ~HEADER_FLAG_SHARED_BACKREF;
        }
    }
    
    delete_tree_item(Vcb, &tp, rollback);
    
    // if EXTENT_ITEM_V0, delete corresponding B4 item
    if (tp.item->size == sizeof(EXTENT_ITEM_V0)) {
        traverse_ptr tp2;
        
        searchkey.obj_id = address;
        searchkey.obj_type = TYPE_EXTENT_REF_V0;
        searchkey.offset = 0xffffffffffffffff;
        
        Status = find_item(Vcb, Vcb->extent_root, &tp2, &searchkey, FALSE);
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
                    convert_old_tree_extent(Vcb, td, t, rollback);
                } else if (td->key.obj_type == TYPE_EXTENT_DATA && td->size >= sizeof(EXTENT_DATA)) {
                    EXTENT_DATA* ed = (EXTENT_DATA*)td->data;
                    
                    if ((ed->type == EXTENT_TYPE_REGULAR || ed->type == EXTENT_TYPE_PREALLOC) && td->size >= sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2)) {
                        EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ed->data;
                        
                        if (ed2->address != 0) {
                            TRACE("trying to convert old data extent %llx,%llx\n", ed2->address, ed2->size);
                            convert_old_data_extent(Vcb, ed2->address, ed2->size, rollback);
                        }
                    }
                }
            }

            le = le->Flink;
        }
    }

    c = get_chunk_from_address(Vcb, address);
    
    if (c) {
        decrease_chunk_usage(c, tp.item->key.offset);
        
        add_to_space_list(c, address, tp.item->key.offset, SPACE_TYPE_DELETING);
    } else
        ERR("could not find chunk for address %llx\n", address);
    
    return STATUS_SUCCESS;
}

static NTSTATUS allocate_tree_extents(device_extension* Vcb, LIST_ENTRY* rollback) {
    LIST_ENTRY* le;
    NTSTATUS Status;
    
    TRACE("(%p)\n", Vcb);
    
    le = Vcb->trees.Flink;
    while (le != &Vcb->trees) {
        tree* t = CONTAINING_RECORD(le, tree, list_entry);
        
        if (t->write && !t->has_new_address) {
            chunk* c;
            
            Status = get_tree_new_address(Vcb, t, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("get_tree_new_address returned %08x\n", Status);
                return Status;
            }
            
            TRACE("allocated extent %llx\n", t->new_address);
            
            if (t->has_address) {
                Status = reduce_tree_extent(Vcb, t->header.address, t, rollback);
                
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

static NTSTATUS update_root_root(device_extension* Vcb, LIST_ENTRY* rollback) {
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
                
                Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE);
                if (!NT_SUCCESS(Status)) {
                    ERR("error - find_item returned %08x\n", Status);
                    return Status;
                }
                
                if (tp.item->key.obj_id != searchkey.obj_id || tp.item->key.obj_type != searchkey.obj_type) {
                    ERR("could not find ROOT_ITEM for tree %llx\n", searchkey.obj_id);
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
                    
                    if (!insert_tree_item(Vcb, Vcb->root_root, searchkey.obj_id, searchkey.obj_type, 0, ri, sizeof(ROOT_ITEM), NULL, rollback)) {
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
    
    Status = update_chunk_caches(Vcb, rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("update_chunk_caches returned %08x\n", Status);
        return Status;
    }
    
    return STATUS_SUCCESS;
}

static NTSTATUS STDCALL write_tree_completion(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID conptr) {
    write_tree_stripe* stripe = conptr;
    write_tree_context* context = (write_tree_context*)stripe->context;
    LIST_ENTRY* le;
    BOOL complete;
    
    if (stripe->status == WriteTreeStatus_Cancelling) {
        stripe->status = WriteTreeStatus_Cancelled;
        goto end;
    }
    
    stripe->iosb = Irp->IoStatus;
    
    if (NT_SUCCESS(Irp->IoStatus.Status)) {
        stripe->status = WriteTreeStatus_Success;
    } else {
        le = context->stripes.Flink;
        
        stripe->status = WriteTreeStatus_Error;
        
        while (le != &context->stripes) {
            write_tree_stripe* s2 = CONTAINING_RECORD(le, write_tree_stripe, list_entry);
            
            if (s2->status == WriteTreeStatus_Pending) {
                s2->status = WriteTreeStatus_Cancelling;
                IoCancelIrp(s2->Irp);
            }
            
            le = le->Flink;
        }
    }
    
end:
    le = context->stripes.Flink;
    complete = TRUE;
        
    while (le != &context->stripes) {
        write_tree_stripe* s2 = CONTAINING_RECORD(le, write_tree_stripe, list_entry);
        
        if (s2->status == WriteTreeStatus_Pending || s2->status == WriteTreeStatus_Cancelling) {
            complete = FALSE;
            break;
        }
        
        le = le->Flink;
    }
    
    if (complete)
        KeSetEvent(&context->Event, 0, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS write_tree(device_extension* Vcb, UINT64 addr, UINT8* data, write_tree_context* wtc) {
    chunk* c;
    CHUNK_ITEM_STRIPE* cis;
    write_tree_stripe* stripe;
    UINT64 i;
    
    c = get_chunk_from_address(Vcb, addr);
    
    if (!c) {
        ERR("get_chunk_from_address failed\n");
        return STATUS_INTERNAL_ERROR;
    }
    
    cis = (CHUNK_ITEM_STRIPE*)&c->chunk_item[1];
    
    // FIXME - make this work with RAID
    
    for (i = 0; i < c->chunk_item->num_stripes; i++) {
        PIO_STACK_LOCATION IrpSp;
        
        // FIXME - handle missing devices
        
        stripe = ExAllocatePoolWithTag(NonPagedPool, sizeof(write_tree_stripe), ALLOC_TAG);
        if (!stripe) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        stripe->context = (struct write_tree_context*)wtc;
        stripe->buf = data;
        stripe->device = c->devices[i];
        RtlZeroMemory(&stripe->iosb, sizeof(IO_STATUS_BLOCK));
        stripe->status = WriteTreeStatus_Pending;
        
        stripe->Irp = IoAllocateIrp(stripe->device->devobj->StackSize, FALSE);
    
        if (!stripe->Irp) {
            ERR("IoAllocateIrp failed\n");
            return STATUS_INTERNAL_ERROR;
        }
        
        IrpSp = IoGetNextIrpStackLocation(stripe->Irp);
        IrpSp->MajorFunction = IRP_MJ_WRITE;
        
        if (stripe->device->devobj->Flags & DO_BUFFERED_IO) {
            stripe->Irp->AssociatedIrp.SystemBuffer = data;

            stripe->Irp->Flags = IRP_BUFFERED_IO;
        } else if (stripe->device->devobj->Flags & DO_DIRECT_IO) {
            stripe->Irp->MdlAddress = IoAllocateMdl(data, Vcb->superblock.node_size, FALSE, FALSE, NULL);
            if (!stripe->Irp->MdlAddress) {
                ERR("IoAllocateMdl failed\n");
                return STATUS_INTERNAL_ERROR;
            }
            
            MmProbeAndLockPages(stripe->Irp->MdlAddress, KernelMode, IoWriteAccess);
        } else {
            stripe->Irp->UserBuffer = data;
        }

        IrpSp->Parameters.Write.Length = Vcb->superblock.node_size;
        IrpSp->Parameters.Write.ByteOffset.QuadPart = addr - c->offset + cis[i].offset;
        
        stripe->Irp->UserIosb = &stripe->iosb;

        IoSetCompletionRoutine(stripe->Irp, write_tree_completion, stripe, TRUE, TRUE, TRUE);

        InsertTailList(&wtc->stripes, &stripe->list_entry);
    }
    
    return STATUS_SUCCESS;
}

void free_write_tree_stripes(write_tree_context* wtc) {
    LIST_ENTRY *le, *le2, *nextle;
    
    le = wtc->stripes.Flink;
    while (le != &wtc->stripes) {
        write_tree_stripe* stripe = CONTAINING_RECORD(le, write_tree_stripe, list_entry);
        
        if (stripe->device->devobj->Flags & DO_DIRECT_IO) {
            MmUnlockPages(stripe->Irp->MdlAddress);
            IoFreeMdl(stripe->Irp->MdlAddress);
        }
        
        le = le->Flink;
    }
    
    le = wtc->stripes.Flink;
    while (le != &wtc->stripes) {
        write_tree_stripe* stripe = CONTAINING_RECORD(le, write_tree_stripe, list_entry);
        
        nextle = le->Flink;

        if (stripe->buf) {
            ExFreePool(stripe->buf);
            
            le2 = le->Flink;
            while (le2 != &wtc->stripes) {
                write_tree_stripe* s2 = CONTAINING_RECORD(le2, write_tree_stripe, list_entry);
                
                if (s2->buf == stripe->buf)
                    s2->buf = NULL;
                
                le2 = le2->Flink;
            }
            
        }
        
        ExFreePool(stripe);
        
        le = nextle;
    }
}

static NTSTATUS write_trees(device_extension* Vcb) {
    UINT8 level;
    UINT8 *data, *body;
    UINT32 crc32;
    NTSTATUS Status;
    LIST_ENTRY* le;
    write_tree_context* wtc;
    
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
                    
                    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE);
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
    
    wtc = ExAllocatePoolWithTag(NonPagedPool, sizeof(write_tree_context), ALLOC_TAG);
    if (!wtc) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    KeInitializeEvent(&wtc->Event, NotificationEvent, FALSE);
    InitializeListHead(&wtc->stripes);
    
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
            
            Status = write_tree(Vcb, t->new_address, data, wtc);
            if (!NT_SUCCESS(Status)) {
                ERR("write_tree returned %08x\n", Status);
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
            write_tree_stripe* stripe = CONTAINING_RECORD(le, write_tree_stripe, list_entry);
            
            IoCallDriver(stripe->device->devobj, stripe->Irp);
            
            le = le->Flink;
        }
        
        KeWaitForSingleObject(&wtc->Event, Executive, KernelMode, FALSE, NULL);
        
        le = wtc->stripes.Flink;
        while (le != &wtc->stripes) {
            write_tree_stripe* stripe = CONTAINING_RECORD(le, write_tree_stripe, list_entry);
            
            if (!NT_SUCCESS(stripe->iosb.Status)) {
                Status = stripe->iosb.Status;
                break;
            }
            
            le = le->Flink;
        }
        
        free_write_tree_stripes(wtc);
    }
    
end:
    ExFreePool(wtc);
    
    return Status;
}

static void update_backup_superblock(device_extension* Vcb, superblock_backup* sb) {
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
    
    if (NT_SUCCESS(find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE))) {
        if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == searchkey.obj_type && tp.item->size >= sizeof(ROOT_ITEM)) {
            ROOT_ITEM* ri = (ROOT_ITEM*)tp.item->data;
            
            sb->extent_tree_addr = ri->block_number;
            sb->extent_tree_generation = ri->generation;
            sb->extent_root_level = ri->root_level;
        }
    }

    searchkey.obj_id = BTRFS_ROOT_FSTREE;
    
    if (NT_SUCCESS(find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE))) {
        if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == searchkey.obj_type && tp.item->size >= sizeof(ROOT_ITEM)) {
            ROOT_ITEM* ri = (ROOT_ITEM*)tp.item->data;
            
            sb->fs_tree_addr = ri->block_number;
            sb->fs_tree_generation = ri->generation;
            sb->fs_root_level = ri->root_level;
        }
    }
    
    searchkey.obj_id = BTRFS_ROOT_DEVTREE;
    
    if (NT_SUCCESS(find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE))) {
        if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == searchkey.obj_type && tp.item->size >= sizeof(ROOT_ITEM)) {
            ROOT_ITEM* ri = (ROOT_ITEM*)tp.item->data;
            
            sb->dev_root_addr = ri->block_number;
            sb->dev_root_generation = ri->generation;
            sb->dev_root_level = ri->root_level;
        }
    }

    searchkey.obj_id = BTRFS_ROOT_CHECKSUM;
    
    if (NT_SUCCESS(find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE))) {
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

static NTSTATUS write_superblocks(device_extension* Vcb) {
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
    
    update_backup_superblock(Vcb, &Vcb->superblock.backup[BTRFS_NUM_BACKUP_ROOTS - 1]);
    
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

static NTSTATUS update_chunk_usage(device_extension* Vcb, LIST_ENTRY* rollback) {
    LIST_ENTRY* le = Vcb->chunks.Flink;
    chunk* c;
    KEY searchkey;
    traverse_ptr tp;
    BLOCK_GROUP_ITEM* bgi;
    NTSTATUS Status;
    
    TRACE("(%p)\n", Vcb);
    
    while (le != &Vcb->chunks) {
        c = CONTAINING_RECORD(le, chunk, list_entry);
        
        if (c->used != c->oldused) {
            searchkey.obj_id = c->offset;
            searchkey.obj_type = TYPE_BLOCK_GROUP_ITEM;
            searchkey.offset = c->chunk_item->size;
            
            Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE);
            if (!NT_SUCCESS(Status)) {
                ERR("error - find_item returned %08x\n", Status);
                return Status;
            }
            
            if (keycmp(&searchkey, &tp.item->key)) {
                ERR("could not find (%llx,%x,%llx) in extent_root\n", searchkey.obj_id, searchkey.obj_type, searchkey.offset);
                int3;
                return STATUS_INTERNAL_ERROR;
            }
            
            if (tp.item->size < sizeof(BLOCK_GROUP_ITEM)) {
                ERR("(%llx,%x,%llx) was %u bytes, expected %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(BLOCK_GROUP_ITEM));
                return STATUS_INTERNAL_ERROR;
            }
            
            bgi = ExAllocatePoolWithTag(PagedPool, tp.item->size, ALLOC_TAG);
            if (!bgi) {
                ERR("out of memory\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }
    
            RtlCopyMemory(bgi, tp.item->data, tp.item->size);
            bgi->used = c->used;
            
            TRACE("adjusting usage of chunk %llx to %llx\n", c->offset, c->used);
            
            delete_tree_item(Vcb, &tp, rollback);
            
            if (!insert_tree_item(Vcb, Vcb->extent_root, searchkey.obj_id, searchkey.obj_type, searchkey.offset, bgi, tp.item->size, NULL, rollback)) {
                ERR("insert_tree_item failed\n");
                ExFreePool(bgi);
                return STATUS_INTERNAL_ERROR;
            }
            
            TRACE("bytes_used = %llx\n", Vcb->superblock.bytes_used);
            TRACE("chunk_item type = %llx\n", c->chunk_item->type);
            
            if (c->chunk_item->type & BLOCK_FLAG_RAID0) {
                FIXME("RAID0 not yet supported\n");
                ExFreePool(bgi);
                return STATUS_INTERNAL_ERROR;
            } else if (c->chunk_item->type & BLOCK_FLAG_RAID1) {
                FIXME("RAID1 not yet supported\n");
                ExFreePool(bgi);
                return STATUS_INTERNAL_ERROR;
            } else if (c->chunk_item->type & BLOCK_FLAG_DUPLICATE) {
                Vcb->superblock.bytes_used = Vcb->superblock.bytes_used + (2 * (c->used - c->oldused));
            } else if (c->chunk_item->type & BLOCK_FLAG_RAID10) {
                FIXME("RAID10 not yet supported\n");
                ExFreePool(bgi);
                return STATUS_INTERNAL_ERROR;
            } else if (c->chunk_item->type & BLOCK_FLAG_RAID5) {
                FIXME("RAID5 not yet supported\n");
                ExFreePool(bgi);
                return STATUS_INTERNAL_ERROR;
            } else if (c->chunk_item->type & BLOCK_FLAG_RAID6) {
                FIXME("RAID6 not yet supported\n");
                ExFreePool(bgi);
                return STATUS_INTERNAL_ERROR;
            } else { // SINGLE
                Vcb->superblock.bytes_used = Vcb->superblock.bytes_used + c->used - c->oldused;
            }
            
            TRACE("bytes_used = %llx\n", Vcb->superblock.bytes_used);
            
            c->oldused = c->used;
        }
        
        le = le->Flink;
    }
    
    return STATUS_SUCCESS;
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
    Vcb->write_trees++;
    
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
        init_tree_holder(&td->treeholder);
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
    init_tree_holder(&td->treeholder);
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
    init_tree_holder(&td->treeholder);
//     td->treeholder.nonpaged->status = tree_holder_loaded;
    InsertTailList(&pt->itemlist, &td->list_entry);
    nt->paritem = td;
    
    pt->write = TRUE;
    Vcb->write_trees++;

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

static NTSTATUS try_tree_amalgamate(device_extension* Vcb, tree* t, LIST_ENTRY* rollback) {
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
    
    Status = do_load_tree(Vcb, &nextparitem->treeholder, t->root, t->parent, nextparitem, &loaded);
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
            Status = reduce_tree_extent(Vcb, next_tree->new_address, next_tree, rollback);
            
            if (!NT_SUCCESS(Status)) {
                ERR("reduce_tree_extent returned %08x\n", Status);
                return Status;
            }
        } else if (next_tree->has_address) {
            Status = reduce_tree_extent(Vcb, next_tree->header.address, next_tree, rollback);
            
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
            if (!par->write) {
                par->write = TRUE;
                Vcb->write_trees++;
            }
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
            if (!par->write) {
                par->write = TRUE;
                Vcb->write_trees++;
            }
            par = par->parent;
        }
    }
    
    return STATUS_SUCCESS;
}

static NTSTATUS update_extent_level(device_extension* Vcb, UINT64 address, tree* t, UINT8 level, LIST_ENTRY* rollback) {
    KEY searchkey;
    traverse_ptr tp;
    NTSTATUS Status;
    
    if (Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA) {
        searchkey.obj_id = address;
        searchkey.obj_type = TYPE_METADATA_ITEM;
        searchkey.offset = t->header.level;
        
        Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE);
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
            
            if (!insert_tree_item(Vcb, Vcb->extent_root, address, TYPE_METADATA_ITEM, level, eism, tp.item->size, NULL, rollback)) {
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
    
    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE);
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
        
        if (!insert_tree_item(Vcb, Vcb->extent_root, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, eit, tp.item->size, NULL, rollback)) {
            ERR("insert_tree_item failed\n");
            ExFreePool(eit);
            return STATUS_INTERNAL_ERROR;
        }
    
        return STATUS_SUCCESS;
    }
    
    ERR("could not find EXTENT_ITEM for address %llx\n", address);
    
    return STATUS_INTERNAL_ERROR;
}

static NTSTATUS STDCALL do_splits(device_extension* Vcb, LIST_ENTRY* rollback) {
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
                            Status = reduce_tree_extent(Vcb, t->new_address, t, rollback);
                            
                            if (!NT_SUCCESS(Status)) {
                                ERR("reduce_tree_extent returned %08x\n", Status);
                                return Status;
                            }
                        } else if (t->has_address) {
                            Status = reduce_tree_extent(Vcb,t->header.address, t, rollback);
                            
                            if (!NT_SUCCESS(Status)) {
                                ERR("reduce_tree_extent returned %08x\n", Status);
                                return Status;
                            }
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
                            Status = update_extent_level(Vcb, t->new_address, t, 0, rollback);
                            
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
                Status = try_tree_amalgamate(Vcb, t, rollback);
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
                            Status = reduce_tree_extent(Vcb, t->new_address, t, rollback);
                            
                            if (!NT_SUCCESS(Status)) {
                                ERR("reduce_tree_extent returned %08x\n", Status);
                                return Status;
                            }
                        } else if (t->has_address) {
                            Status = reduce_tree_extent(Vcb,t->header.address, t, rollback);
                            
                            if (!NT_SUCCESS(Status)) {
                                ERR("reduce_tree_extent returned %08x\n", Status);
                                return Status;
                            }
                        }
                        
                        if (!td->treeholder.tree) { // load first item if not already loaded
                            KEY searchkey = {0,0,0};
                            traverse_ptr tp;
                            
                            Status = find_item(Vcb, t->root, &tp, &searchkey, FALSE);
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

static NTSTATUS remove_root_extents(device_extension* Vcb, root* r, tree_holder* th, UINT8 level, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    
    if (level > 0) {
        if (!th->tree) {
            Status = load_tree(Vcb, th->address, r, &th->tree);
            
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
                    Status = remove_root_extents(Vcb, r, &td->treeholder, th->tree->header.level - 1, rollback);
                    
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
        Status = reduce_tree_extent(Vcb, th->address, NULL, rollback);
        
        if (!NT_SUCCESS(Status)) {
            ERR("reduce_tree_extent(%llx) returned %08x\n", th->address, Status);
            return Status;
        }
    }
    
    return STATUS_SUCCESS;
}

static NTSTATUS drop_root(device_extension* Vcb, root* r, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    KEY searchkey;
    traverse_ptr tp;
    
    Status = remove_root_extents(Vcb, r, &r->treeholder, r->root_item.root_level, rollback);
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
            Status = find_item(Vcb, Vcb->uuid_root, &tp, &searchkey, FALSE);
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
    
    Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE);
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

static NTSTATUS drop_roots(device_extension* Vcb, LIST_ENTRY* rollback) {
    LIST_ENTRY *le = Vcb->drop_roots.Flink, *le2;
    NTSTATUS Status;
    
    while (le != &Vcb->drop_roots) {
        root* r = CONTAINING_RECORD(le, root, list_entry);
        
        le2 = le->Flink;
        
        Status = drop_root(Vcb, r, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("drop_root(%llx) returned %08x\n", r->id, Status);
            return Status;
        }
        
        le = le2;
    }
    
    return STATUS_SUCCESS;
}

NTSTATUS STDCALL do_write(device_extension* Vcb, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    LIST_ENTRY* le;
    BOOL cache_changed;
    
    TRACE("(%p)\n", Vcb);
    
    if (!IsListEmpty(&Vcb->drop_roots)) {
        Status = drop_roots(Vcb, rollback);
        
        if (!NT_SUCCESS(Status)) {
            ERR("drop_roots returned %08x\n", Status);
            return Status;
        }
    }
    
    // If only changing superblock, e.g. changing label, we still need to rewrite
    // the root tree so the generations match, otherwise you won't be able to mount on Linux.
    if (Vcb->write_trees == 0) {
        KEY searchkey;
        traverse_ptr tp;
        
        searchkey.obj_id = 0;
        searchkey.obj_type = 0;
        searchkey.offset = 0;
        
        Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            return Status;
        }
        
        if (!Vcb->root_root->treeholder.tree->write) {
            Vcb->root_root->treeholder.tree->write = TRUE;
            Vcb->write_trees++;
        }
    }
    
    do {
        Status = add_parents(Vcb, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("add_parents returned %08x\n", Status);
            goto end;
        }
        
        Status = do_splits(Vcb, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("do_splits returned %08x\n", Status);
            goto end;
        }
        
        Status = allocate_tree_extents(Vcb, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("add_parents returned %08x\n", Status);
            goto end;
        }
        
        Status = update_chunk_usage(Vcb, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("update_chunk_usage returned %08x\n", Status);
            goto end;
        }
        
        Status = allocate_cache(Vcb, &cache_changed, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("allocate_cache returned %08x\n", Status);
            goto end;
        }
    } while (cache_changed || !trees_consistent(Vcb, rollback));
    
    TRACE("trees consistent\n");
    
    Status = update_root_root(Vcb, rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("update_root_root returned %08x\n", Status);
        goto end;
    }
    
    Status = write_trees(Vcb);
    if (!NT_SUCCESS(Status)) {
        ERR("write_trees returned %08x\n", Status);
        goto end;
    }
    
    Vcb->superblock.cache_generation = Vcb->superblock.generation;
    
    Status = write_superblocks(Vcb);
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
        
        t->write = FALSE;
        
        le = le->Flink;
    }
    
    Vcb->write_trees = 0;
    
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

NTSTATUS consider_write(device_extension* Vcb) {
    // FIXME - call do_write if Vcb->write_trees high
#if 0
    LIST_ENTRY rollback;
    NTSTATUS Status = STATUS_SUCCESS;
    
    InitializeListHead(&rollback);
    
    if (Vcb->write_trees > 0)
        Status = do_write(Vcb, &rollback);
    
    free_tree_cache(&Vcb->tree_cache);
    
    if (!NT_SUCCESS(Status))
        do_rollback(Vcb, &rollback);
    else
        clear_rollback(&rollback);
    
    return Status;
#else
    return STATUS_SUCCESS;
#endif
}

// NTSTATUS STDCALL add_extent_ref(device_extension* Vcb, UINT64 address, UINT64 size, root* subvol, UINT64 inode, UINT64 offset, LIST_ENTRY* rollback) {
//     KEY searchkey;
//     traverse_ptr tp;
//     EXTENT_ITEM* ei;
//     UINT8 *siptr, *type;
//     ULONG len;
//     UINT64 hash;
//     EXTENT_DATA_REF* edr;
//     NTSTATUS Status;
//     
//     TRACE("(%p, %llx, %llx, %llx, %llx, %llx)\n", Vcb, address, size, subvol->id, inode, offset);
//     
//     searchkey.obj_id = address;
//     searchkey.obj_type = TYPE_EXTENT_ITEM;
//     searchkey.offset = size;
//     
//     Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE);
//     if (!NT_SUCCESS(Status)) {
//         ERR("error - find_item returned %08x\n", Status);
//         return Status;
//     }
//     
//     if (keycmp(&tp.item->key, &searchkey)) {
//         // create new entry
//         
//         len = sizeof(EXTENT_ITEM) + sizeof(UINT8) + sizeof(EXTENT_DATA_REF);
//         
//         ei = ExAllocatePoolWithTag(PagedPool, len, ALLOC_TAG);
//         if (!ei) {
//             ERR("out of memory\n");
//             return STATUS_INSUFFICIENT_RESOURCES;
//         }
//         
//         ei->refcount = 1;
//         ei->generation = Vcb->superblock.generation;
//         ei->flags = EXTENT_ITEM_DATA;
//         
//         type = (UINT8*)&ei[1];
//         *type = TYPE_EXTENT_DATA_REF;
//         
//         edr = (EXTENT_DATA_REF*)&type[1];
//         edr->root = subvol->id;
//         edr->objid = inode;
//         edr->offset = offset;
//         edr->count = 1;
//         
//         if (!insert_tree_item(Vcb, Vcb->extent_root, searchkey.obj_id, searchkey.obj_type, searchkey.offset, ei, len, NULL, rollback)) {
//             ERR("error - failed to insert item\n");
//             return STATUS_INTERNAL_ERROR;
//         }
//         
//         // FIXME - update free space in superblock and CHUNK_ITEM
//         
//         return STATUS_SUCCESS;
//     }
//     
//     if (tp.item->size == sizeof(EXTENT_ITEM_V0)) { // old extent ref, convert
//         NTSTATUS Status = convert_old_data_extent(Vcb, address, size, rollback);
//         if (!NT_SUCCESS(Status)) {
//             ERR("convert_old_data_extent returned %08x\n", Status);
//             return Status;
//         }
//         
//         Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE);
//         if (!NT_SUCCESS(Status)) {
//             ERR("error - find_item returned %08x\n", Status);
//             return Status;
//         }
//         
//         if (keycmp(&tp.item->key, &searchkey)) {
//             WARN("extent item not found for address %llx, size %llx\n", address, size);
//             return STATUS_SUCCESS;
//         }
//     }
//     
//     ei = (EXTENT_ITEM*)tp.item->data;
//     
//     if (tp.item->size < sizeof(EXTENT_ITEM)) {
//         ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_ITEM));
//         return STATUS_INTERNAL_ERROR;
//     }
//     
//     if (extent_item_is_shared(ei, tp.item->size - sizeof(EXTENT_ITEM))) {
//         NTSTATUS Status = convert_shared_data_extent(Vcb, address, size, rollback);
//         if (!NT_SUCCESS(Status)) {
//             ERR("convert_shared_data_extent returned %08x\n", Status);
//             return Status;
//         }
//         
//         Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE);
//         if (!NT_SUCCESS(Status)) {
//             ERR("error - find_item returned %08x\n", Status);
//             return Status;
//         }
//         
//         if (keycmp(&tp.item->key, &searchkey)) {
//             WARN("extent item not found for address %llx, size %llx\n", address, size);
//             return STATUS_SUCCESS;
//         }
//         
//         ei = (EXTENT_ITEM*)tp.item->data;
//     }
//     
//     if (ei->flags != EXTENT_ITEM_DATA) {
//         ERR("error - flag was not EXTENT_ITEM_DATA\n");
//         return STATUS_INTERNAL_ERROR;
//     }
//     
//     hash = get_extent_data_ref_hash(subvol->id, inode, offset);
//     
//     len = tp.item->size - sizeof(EXTENT_ITEM);
//     siptr = (UINT8*)&ei[1];
//     
//     do {
//         if (*siptr == TYPE_EXTENT_DATA_REF) {
//             UINT64 sihash;
//             
//             edr = (EXTENT_DATA_REF*)&siptr[1];
//             
//             // already exists - increase refcount
//             if (edr->root == subvol->id && edr->objid == inode && edr->offset == offset) {
//                 ei = ExAllocatePoolWithTag(PagedPool, tp.item->size, ALLOC_TAG);
//                 
//                 if (!ei) {
//                     ERR("out of memory\n");
//                     return STATUS_INSUFFICIENT_RESOURCES;
//                 }
//                 
//                 RtlCopyMemory(ei, tp.item->data, tp.item->size);
//                 
//                 edr = (EXTENT_DATA_REF*)((UINT8*)ei + ((UINT8*)edr - tp.item->data));
//                 edr->count++;
//                 ei->refcount++;
//                 
//                 delete_tree_item(Vcb, &tp, rollback);
//                 
//                 if (!insert_tree_item(Vcb, Vcb->extent_root, searchkey.obj_id, searchkey.obj_type, searchkey.offset, ei, tp.item->size, NULL, rollback)) {
//                     ERR("error - failed to insert item\n");
//                     ExFreePool(ei);
//                     return STATUS_INTERNAL_ERROR;
//                 }
//                 
//                 return STATUS_SUCCESS;
//             }
//             
//             sihash = get_extent_data_ref_hash(edr->root, edr->objid, edr->offset);
//             
//             if (sihash >= hash)
//                 break;
//             
//             siptr += sizeof(UINT8) + sizeof(EXTENT_DATA_REF);
//             
//             if (len > sizeof(EXTENT_DATA_REF) + sizeof(UINT8)) {
//                 len -= sizeof(EXTENT_DATA_REF) + sizeof(UINT8);
//             } else
//                 break;
//         // FIXME - TYPE_TREE_BLOCK_REF    0xB0
//         } else {
//             ERR("unrecognized extent subitem %x\n", *siptr);
//             return STATUS_INTERNAL_ERROR;
//         }
//     } while (len > 0);
//     
//     len = tp.item->size + sizeof(UINT8) + sizeof(EXTENT_DATA_REF); // FIXME - die if too big
//     ei = ExAllocatePoolWithTag(PagedPool, len, ALLOC_TAG);
//     if (!ei) {
//         ERR("out of memory\n");
//         return STATUS_INSUFFICIENT_RESOURCES;
//     }
//     
//     RtlCopyMemory(ei, tp.item->data, siptr - tp.item->data);
//     ei->refcount++;
//     
//     type = (UINT8*)ei + (siptr - tp.item->data);
//     *type = TYPE_EXTENT_DATA_REF;
//     
//     edr = (EXTENT_DATA_REF*)&type[1];
//     edr->root = subvol->id;
//     edr->objid = inode;
//     edr->offset = offset;
//     edr->count = 1;
//     
//     if (siptr < tp.item->data + tp.item->size)
//         RtlCopyMemory(&edr[1], siptr, tp.item->data + tp.item->size - siptr);
//     
//     delete_tree_item(Vcb, &tp, rollback);
//     
//     if (!insert_tree_item(Vcb, Vcb->extent_root, searchkey.obj_id, searchkey.obj_type, searchkey.offset, ei, len, NULL, rollback)) {
//         ERR("error - failed to insert item\n");
//         ExFreePool(ei);
//         return STATUS_INTERNAL_ERROR;
//     }
//     
//     return STATUS_SUCCESS;
// }

static BOOL extent_item_is_shared(EXTENT_ITEM* ei, ULONG len) {
    UINT8* siptr = (UINT8*)&ei[1];
    
    do {
        if (*siptr == TYPE_TREE_BLOCK_REF) {
            siptr += sizeof(TREE_BLOCK_REF) + 1;
            len -= sizeof(TREE_BLOCK_REF) + 1;
        } else if (*siptr == TYPE_EXTENT_DATA_REF) {
            siptr += sizeof(EXTENT_DATA_REF) + 1;
            len -= sizeof(EXTENT_DATA_REF) + 1;
        } else if (*siptr == TYPE_SHARED_BLOCK_REF) {
            return TRUE;
        } else if (*siptr == TYPE_SHARED_DATA_REF) {
            return TRUE;
        } else {
            ERR("unrecognized extent subitem %x\n", *siptr);
            return FALSE;
        }
    } while (len > 0);
    
    return FALSE;
}

// static NTSTATUS STDCALL remove_extent_ref(device_extension* Vcb, UINT64 address, UINT64 size, root* subvol, UINT64 inode, UINT64 offset, LIST_ENTRY* changed_sector_list, LIST_ENTRY* rollback) {
//     KEY searchkey;
//     traverse_ptr tp;
//     EXTENT_ITEM* ei;
//     UINT8* siptr;
//     ULONG len;
//     EXTENT_DATA_REF* edr;
//     BOOL found;
//     NTSTATUS Status;
//     
//     TRACE("(%p, %llx, %llx, %llx, %llx, %llx)\n", Vcb, address, size, subvol->id, inode, offset);
//     
//     searchkey.obj_id = address;
//     searchkey.obj_type = TYPE_EXTENT_ITEM;
//     searchkey.offset = size;
//     
//     Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE);
//     if (!NT_SUCCESS(Status)) {
//         ERR("error - find_item returned %08x\n", Status);
//         return Status;
//     }
//     
//     if (keycmp(&tp.item->key, &searchkey)) {
//         WARN("extent item not found for address %llx, size %llx\n", address, size);
//         return STATUS_SUCCESS;
//     }
//     
//     if (tp.item->size == sizeof(EXTENT_ITEM_V0)) { // old extent ref, convert
//         NTSTATUS Status = convert_old_data_extent(Vcb, address, size, rollback);
//         if (!NT_SUCCESS(Status)) {
//             ERR("convert_old_data_extent returned %08x\n", Status);
//             return Status;
//         }
//         
//         Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE);
//         if (!NT_SUCCESS(Status)) {
//             ERR("error - find_item returned %08x\n", Status);
//             return Status;
//         }
//         
//         if (keycmp(&tp.item->key, &searchkey)) {
//             WARN("extent item not found for address %llx, size %llx\n", address, size);
//             return STATUS_SUCCESS;
//         }
//     }
//     
//     ei = (EXTENT_ITEM*)tp.item->data;
//     
//     if (tp.item->size < sizeof(EXTENT_ITEM)) {
//         ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_ITEM));
//         return STATUS_INTERNAL_ERROR;
//     }
//     
//     if (!(ei->flags & EXTENT_ITEM_DATA)) {
//         ERR("error - EXTENT_ITEM_DATA flag not set\n");
//         return STATUS_INTERNAL_ERROR;
//     }
//     
//     if (extent_item_is_shared(ei, tp.item->size - sizeof(EXTENT_ITEM))) {
//         NTSTATUS Status = convert_shared_data_extent(Vcb, address, size, rollback);
//         if (!NT_SUCCESS(Status)) {
//             ERR("convert_shared_data_extent returned %08x\n", Status);
//             return Status;
//         }
//         
//         Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE);
//         if (!NT_SUCCESS(Status)) {
//             ERR("error - find_item returned %08x\n", Status);
//             return Status;
//         }
//         
//         if (keycmp(&tp.item->key, &searchkey)) {
//             WARN("extent item not found for address %llx, size %llx\n", address, size);
//             return STATUS_SUCCESS;
//         }
//         
//         ei = (EXTENT_ITEM*)tp.item->data;
//     }
//     
//     len = tp.item->size - sizeof(EXTENT_ITEM);
//     siptr = (UINT8*)&ei[1];
//     found = FALSE;
//     
//     do {
//         if (*siptr == TYPE_EXTENT_DATA_REF) {
//             edr = (EXTENT_DATA_REF*)&siptr[1];
//             
//             if (edr->root == subvol->id && edr->objid == inode && edr->offset == offset) {
//                 if (edr->count > 1) {
//                     ei = ExAllocatePoolWithTag(PagedPool, tp.item->size, ALLOC_TAG);
//                 
//                     if (!ei) {
//                         ERR("out of memory\n");
//                         return STATUS_INSUFFICIENT_RESOURCES;
//                     }
//                     
//                     RtlCopyMemory(ei, tp.item->data, tp.item->size);
//                     
//                     edr = (EXTENT_DATA_REF*)((UINT8*)ei + ((UINT8*)edr - tp.item->data));
//                     edr->count--;
//                     ei->refcount--;
//                     
//                     delete_tree_item(Vcb, &tp, rollback);
//                     
//                     if (!insert_tree_item(Vcb, Vcb->extent_root, searchkey.obj_id, searchkey.obj_type, searchkey.offset, ei, tp.item->size, NULL, rollback)) {
//                         ERR("error - failed to insert item\n");
//                         ExFreePool(ei);
//                         return STATUS_INTERNAL_ERROR;
//                     }
//                     
//                     return STATUS_SUCCESS;
//                 }
//                 
//                 found = TRUE;
//                 break;
//             }
// 
//             siptr += sizeof(UINT8) + sizeof(EXTENT_DATA_REF);
//             
//             if (len > sizeof(EXTENT_DATA_REF) + sizeof(UINT8)) {
//                 len -= sizeof(EXTENT_DATA_REF) + sizeof(UINT8);
//             } else
//                 break;
// //         // FIXME - TYPE_TREE_BLOCK_REF    0xB0
//         } else {
//             ERR("unrecognized extent subitem %x\n", *siptr);
//             return STATUS_INTERNAL_ERROR;
//         }
//     } while (len > 0);
//     
//     if (!found) {
//         WARN("could not find extent data ref\n");
//         return STATUS_SUCCESS;
//     }
//     
//     // FIXME - decrease subitem refcount if there already?
//     
//     len = tp.item->size - sizeof(UINT8) - sizeof(EXTENT_DATA_REF);
//     
//     delete_tree_item(Vcb, &tp, rollback);
//     
//     if (len == sizeof(EXTENT_ITEM)) { // extent no longer needed
//         chunk* c;
//         LIST_ENTRY* le2;
//         
//         if (changed_sector_list) {
//             changed_sector* sc = ExAllocatePoolWithTag(PagedPool, sizeof(changed_sector), ALLOC_TAG);
//             if (!sc) {
//                 ERR("out of memory\n");
//                 return STATUS_INSUFFICIENT_RESOURCES;
//             }
//             
//             sc->ol.key = address;
//             sc->checksums = NULL;
//             sc->length = size / Vcb->superblock.sector_size;
// 
//             sc->deleted = TRUE;
//             
//             insert_into_ordered_list(changed_sector_list, &sc->ol);
//         }
//         
//         c = NULL;
//         le2 = Vcb->chunks.Flink;
//         while (le2 != &Vcb->chunks) {
//             c = CONTAINING_RECORD(le2, chunk, list_entry);
//             
//             TRACE("chunk: %llx, %llx\n", c->offset, c->chunk_item->size);
//             
//             if (address >= c->offset && address + size < c->offset + c->chunk_item->size)
//                 break;
//             
//             le2 = le2->Flink;
//         }
//         if (le2 == &Vcb->chunks) c = NULL;
//         
//         if (c) {
//             decrease_chunk_usage(c, size);
//             
//             add_to_space_list(c, address, size, SPACE_TYPE_DELETING);
//         }
// 
//         return STATUS_SUCCESS;
//     }
//     
//     ei = ExAllocatePoolWithTag(PagedPool, len, ALLOC_TAG);
//     if (!ei) {
//         ERR("out of memory\n");
//         return STATUS_INSUFFICIENT_RESOURCES;
//     }
//             
//     RtlCopyMemory(ei, tp.item->data, siptr - tp.item->data);
//     ei->refcount--;
//     ei->generation = Vcb->superblock.generation;
//     
//     if (tp.item->data + len != siptr)
//         RtlCopyMemory((UINT8*)ei + (siptr - tp.item->data), siptr + sizeof(UINT8) + sizeof(EXTENT_DATA_REF), tp.item->size - (siptr - tp.item->data) - sizeof(UINT8) - sizeof(EXTENT_DATA_REF));
//     
//     if (!insert_tree_item(Vcb, Vcb->extent_root, searchkey.obj_id, searchkey.obj_type, searchkey.offset, ei, len, NULL, rollback)) {
//         ERR("error - failed to insert item\n");
//         ExFreePool(ei);
//         return STATUS_INTERNAL_ERROR;
//     }
//     
//     return STATUS_SUCCESS;
// }

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

static BOOL is_file_prealloc_inode(device_extension* Vcb, root* subvol, UINT64 inode, UINT64 start_data, UINT64 end_data) {
    NTSTATUS Status;
    KEY searchkey;
    traverse_ptr tp, next_tp;
    BOOL b;
    
    searchkey.obj_id = inode;
    searchkey.obj_type = TYPE_EXTENT_DATA;
    searchkey.offset = start_data;
    
    Status = find_item(Vcb, subvol, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return FALSE;
    }
    
    if (tp.item->key.obj_id != inode || tp.item->key.obj_type != TYPE_EXTENT_DATA)
        return FALSE;
    
    do {
        EXTENT_DATA* ed = (EXTENT_DATA*)tp.item->data;
        EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ed->data;
        UINT64 len;
        
        if (tp.item->size < sizeof(EXTENT_DATA)) {
            ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_DATA));
            return FALSE;
        }
        
        if ((ed->type == EXTENT_TYPE_REGULAR || ed->type == EXTENT_TYPE_PREALLOC) && tp.item->size < sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2)) {
            ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2));
            return FALSE;
        }
        
        b = find_next_item(Vcb, &tp, &next_tp, FALSE);
        
        len = ed->type == EXTENT_TYPE_INLINE ? ed->decoded_size : ed2->num_bytes;
        
        if (tp.item->key.offset < end_data && tp.item->key.offset + len >= start_data && ed->type == EXTENT_TYPE_PREALLOC)
            return TRUE;
        
        if (b) {
            tp = next_tp;
            
            if (tp.item->key.obj_id > inode || tp.item->key.obj_type > TYPE_EXTENT_DATA || tp.item->key.offset >= end_data)
                break;
        }
    } while (b);
    
    return FALSE;
}

NTSTATUS excise_extents_inode(device_extension* Vcb, root* subvol, UINT64 inode, INODE_ITEM* ii, UINT64 start_data, UINT64 end_data, LIST_ENTRY* changed_sector_list, LIST_ENTRY* rollback) {
    KEY searchkey;
    traverse_ptr tp, next_tp;
    NTSTATUS Status;
    BOOL b, deleted_prealloc = FALSE;
    
    searchkey.obj_id = inode;
    searchkey.obj_type = TYPE_EXTENT_DATA;
    searchkey.offset = start_data;
    
    Status = find_item(Vcb, subvol, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }

    do {
        EXTENT_DATA* ed = (EXTENT_DATA*)tp.item->data;
        EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ed->data;
        UINT64 len;
        
        if (tp.item->size < sizeof(EXTENT_DATA)) {
            ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_DATA));
            Status = STATUS_INTERNAL_ERROR;
            goto end;
        }
        
        b = find_next_item(Vcb, &tp, &next_tp, FALSE);
        
        if (tp.item->key.obj_id != searchkey.obj_id || tp.item->key.obj_type != searchkey.obj_type)
            goto cont;
        
        if ((ed->type == EXTENT_TYPE_REGULAR || ed->type == EXTENT_TYPE_PREALLOC) && tp.item->size < sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2)) {
            ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2));
            Status = STATUS_INTERNAL_ERROR;
            goto end;
        }
        
        len = ed->type == EXTENT_TYPE_INLINE ? ed->decoded_size : ed2->num_bytes;
        
        if (tp.item->key.offset < end_data && tp.item->key.offset + len >= start_data) {
            if (ed->compression != BTRFS_COMPRESSION_NONE) {
                FIXME("FIXME - compression not supported at present\n");
                Status = STATUS_NOT_SUPPORTED;
                goto end;
            }
            
            if (ed->encryption != BTRFS_ENCRYPTION_NONE) {
                WARN("root %llx, inode %llx, extent %llx: encryption not supported (type %x)\n", subvol->id, inode, tp.item->key.offset, ed->encryption);
                Status = STATUS_NOT_SUPPORTED;
                goto end;
            }
            
            if (ed->encoding != BTRFS_ENCODING_NONE) {
                WARN("other encodings not supported\n");
                Status = STATUS_NOT_SUPPORTED;
                goto end;
            }
            
            if (ed->type == EXTENT_TYPE_INLINE) {
                if (start_data <= tp.item->key.offset && end_data >= tp.item->key.offset + len) { // remove all
                    delete_tree_item(Vcb, &tp, rollback);
                    
                    if (ii)
                        ii->st_blocks -= len;
                } else if (start_data <= tp.item->key.offset && end_data < tp.item->key.offset + len) { // remove beginning
                    EXTENT_DATA* ned;
                    UINT64 size;
                    
                    delete_tree_item(Vcb, &tp, rollback);
                    
                    size = len - (end_data - tp.item->key.offset);
                    
                    ned = ExAllocatePoolWithTag(PagedPool, sizeof(EXTENT_DATA) - 1 + size, ALLOC_TAG);
                    if (!ned) {
                        ERR("out of memory\n");
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        goto end;
                    }
                    
                    ned->generation = Vcb->superblock.generation;
                    ned->decoded_size = size;
                    ned->compression = ed->compression;
                    ned->encryption = ed->encryption;
                    ned->encoding = ed->encoding;
                    ned->type = ed->type;
                    
                    RtlCopyMemory(&ned->data[0], &ed->data[end_data - tp.item->key.offset], size);
                    
                    if (!insert_tree_item(Vcb, subvol, inode, TYPE_EXTENT_DATA, end_data, ned, sizeof(EXTENT_DATA) - 1 + size, NULL, rollback)) {
                        ERR("insert_tree_item failed\n");
                        ExFreePool(ned);
                        Status = STATUS_INTERNAL_ERROR;
                        goto end;
                    }
                    
                    if (ii)
                        ii->st_blocks -= end_data - tp.item->key.offset;
                } else if (start_data > tp.item->key.offset && end_data >= tp.item->key.offset + len) { // remove end
                    EXTENT_DATA* ned;
                    UINT64 size;
                    
                    delete_tree_item(Vcb, &tp, rollback);
                    
                    size = start_data - tp.item->key.offset;
                    
                    ned = ExAllocatePoolWithTag(PagedPool, sizeof(EXTENT_DATA) - 1 + size, ALLOC_TAG);
                    if (!ned) {
                        ERR("out of memory\n");
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        goto end;
                    }
                    
                    ned->generation = Vcb->superblock.generation;
                    ned->decoded_size = size;
                    ned->compression = ed->compression;
                    ned->encryption = ed->encryption;
                    ned->encoding = ed->encoding;
                    ned->type = ed->type;
                    
                    RtlCopyMemory(&ned->data[0], &ed->data[0], size);
                    
                    if (!insert_tree_item(Vcb, subvol, inode, TYPE_EXTENT_DATA, tp.item->key.offset, ned, sizeof(EXTENT_DATA) - 1 + size, NULL, rollback)) {
                        ERR("insert_tree_item failed\n");
                        ExFreePool(ned);
                        Status = STATUS_INTERNAL_ERROR;
                        goto end;
                    }
                    
                    if (ii)
                        ii->st_blocks -= tp.item->key.offset + len - start_data;
                } else if (start_data > tp.item->key.offset && end_data < tp.item->key.offset + len) { // remove middle
                    EXTENT_DATA* ned;
                    UINT64 size;
                    
                    delete_tree_item(Vcb, &tp, rollback);
                    
                    size = start_data - tp.item->key.offset;
                    
                    ned = ExAllocatePoolWithTag(PagedPool, sizeof(EXTENT_DATA) - 1 + size, ALLOC_TAG);
                    if (!ned) {
                        ERR("out of memory\n");
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        goto end;
                    }
                    
                    ned->generation = Vcb->superblock.generation;
                    ned->decoded_size = size;
                    ned->compression = ed->compression;
                    ned->encryption = ed->encryption;
                    ned->encoding = ed->encoding;
                    ned->type = ed->type;
                    
                    RtlCopyMemory(&ned->data[0], &ed->data[0], size);
                    
                    if (!insert_tree_item(Vcb, subvol, inode, TYPE_EXTENT_DATA, tp.item->key.offset, ned, sizeof(EXTENT_DATA) - 1 + size, NULL, rollback)) {
                        ERR("insert_tree_item failed\n");
                        ExFreePool(ned);
                        Status = STATUS_INTERNAL_ERROR;
                        goto end;
                    }
                    
                    size = tp.item->key.offset + len - end_data;
                    
                    ned = ExAllocatePoolWithTag(PagedPool, sizeof(EXTENT_DATA) - 1 + size, ALLOC_TAG);
                    if (!ned) {
                        ERR("out of memory\n");
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        goto end;
                    }
                    
                    ned->generation = Vcb->superblock.generation;
                    ned->decoded_size = size;
                    ned->compression = ed->compression;
                    ned->encryption = ed->encryption;
                    ned->encoding = ed->encoding;
                    ned->type = ed->type;
                    
                    RtlCopyMemory(&ned->data[0], &ed->data[end_data - tp.item->key.offset], size);
                    
                    if (!insert_tree_item(Vcb, subvol, inode, TYPE_EXTENT_DATA, end_data, ned, sizeof(EXTENT_DATA) - 1 + size, NULL, rollback)) {
                        ERR("insert_tree_item failed\n");
                        ExFreePool(ned);
                        Status = STATUS_INTERNAL_ERROR;
                        goto end;
                    }
                    
                    if (ii)
                        ii->st_blocks -= end_data - start_data;
                }
            } else if (ed->type == EXTENT_TYPE_REGULAR || ed->type == EXTENT_TYPE_PREALLOC) {
                if (start_data <= tp.item->key.offset && end_data >= tp.item->key.offset + len) { // remove all
                    if (ed2->address != 0) {
                        Status = decrease_extent_refcount_data(Vcb, ed2->address, ed2->size, subvol, inode, tp.item->key.offset - ed2->offset, 1, changed_sector_list, rollback);
                        if (!NT_SUCCESS(Status)) {
                            ERR("decrease_extent_refcount_data returned %08x\n", Status);
                            goto end;
                        }
                        
                        if (ii)
                            ii->st_blocks -= len;
                    }
                    
                    if (ed->type == EXTENT_TYPE_PREALLOC)
                        deleted_prealloc = TRUE;
                    
                    delete_tree_item(Vcb, &tp, rollback);
                } else if (start_data <= tp.item->key.offset && end_data < tp.item->key.offset + len) { // remove beginning
                    EXTENT_DATA* ned;
                    EXTENT_DATA2* ned2;
                    
                    if (ed2->address != 0 && ii)
                        ii->st_blocks -= end_data - tp.item->key.offset;
                    
                    delete_tree_item(Vcb, &tp, rollback);
                    
                    ned = ExAllocatePoolWithTag(PagedPool, sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2), ALLOC_TAG);
                    if (!ned) {
                        ERR("out of memory\n");
                        Status = STATUS_INSUFFICIENT_RESOURCES;
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
                    ned2->offset = ed2->address == 0 ? 0 : (ed2->offset + (end_data - tp.item->key.offset));
                    ned2->num_bytes = ed2->num_bytes - (end_data - tp.item->key.offset);
                    
                    if (!insert_tree_item(Vcb, subvol, inode, TYPE_EXTENT_DATA, end_data, ned, sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2), NULL, rollback)) {
                        ERR("insert_tree_item failed\n");
                        ExFreePool(ned);
                        Status = STATUS_INTERNAL_ERROR;
                        goto end;
                    }
                } else if (start_data > tp.item->key.offset && end_data >= tp.item->key.offset + len) { // remove end
                    EXTENT_DATA* ned;
                    EXTENT_DATA2* ned2;
                    
                    if (ed2->address != 0 && ii)
                        ii->st_blocks -= tp.item->key.offset + len - start_data;
                    
                    delete_tree_item(Vcb, &tp, rollback);
                    
                    ned = ExAllocatePoolWithTag(PagedPool, sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2), ALLOC_TAG);
                    if (!ned) {
                        ERR("out of memory\n");
                        Status = STATUS_INSUFFICIENT_RESOURCES;
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
                    ned2->num_bytes = start_data - tp.item->key.offset;
                    
                    if (!insert_tree_item(Vcb, subvol, inode, TYPE_EXTENT_DATA, tp.item->key.offset, ned, sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2), NULL, rollback)) {
                        ERR("insert_tree_item failed\n");
                        ExFreePool(ned);
                        Status = STATUS_INTERNAL_ERROR;
                        goto end;
                    }
                } else if (start_data > tp.item->key.offset && end_data < tp.item->key.offset + len) { // remove middle
                    EXTENT_DATA* ned;
                    EXTENT_DATA2* ned2;
                    
                    if (ed2->address != 0 && ii)
                        ii->st_blocks -= end_data - start_data;
                    
                    delete_tree_item(Vcb, &tp, rollback);
                    
                    ned = ExAllocatePoolWithTag(PagedPool, sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2), ALLOC_TAG);
                    if (!ned) {
                        ERR("out of memory\n");
                        Status = STATUS_INSUFFICIENT_RESOURCES;
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
                    ned2->num_bytes = start_data - tp.item->key.offset;
                    
                    if (!insert_tree_item(Vcb, subvol, inode, TYPE_EXTENT_DATA, tp.item->key.offset, ned, sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2), NULL, rollback)) {
                        ERR("insert_tree_item failed\n");
                        ExFreePool(ned);
                        Status = STATUS_INTERNAL_ERROR;
                        goto end;
                    }
                    
                    ned = ExAllocatePoolWithTag(PagedPool, sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2), ALLOC_TAG);
                    if (!ned) {
                        ERR("out of memory\n");
                        Status = STATUS_INSUFFICIENT_RESOURCES;
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
                    ned2->offset = ed2->address == 0 ? 0 : (ed2->offset + (end_data - tp.item->key.offset));
                    ned2->num_bytes = tp.item->key.offset + len - end_data;
                    
                    if (!insert_tree_item(Vcb, subvol, inode, TYPE_EXTENT_DATA, end_data, ned, sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2), NULL, rollback)) {
                        ERR("insert_tree_item failed\n");
                        ExFreePool(ned);
                        Status = STATUS_INTERNAL_ERROR;
                        goto end;
                    }
                    
                    if (ed2->address != 0) {
                        Status = increase_extent_refcount_data(Vcb, ed2->address, ed2->size, subvol, inode, tp.item->key.offset - ed2->offset, 1, rollback);
                        
                        if (!NT_SUCCESS(Status)) {
                            ERR("increase_extent_refcount_data returned %08x\n", Status);
                            goto end;
                        }
                    }
                }
            }
        }

cont:
        if (b) {
            tp = next_tp;
            
            if (tp.item->key.obj_id > inode || tp.item->key.obj_type > TYPE_EXTENT_DATA || tp.item->key.offset >= end_data)
                break;
        }
    } while (b);
    
    // FIXME - do bitmap analysis of changed extents, and free what we can
    
    if (ii && deleted_prealloc && !is_file_prealloc_inode(Vcb, subvol, inode, 0, sector_align(ii->st_size, Vcb->superblock.sector_size)))
        ii->flags &= ~BTRFS_INODE_PREALLOC;
    
end:
    
    return Status;
}

NTSTATUS excise_extents(device_extension* Vcb, fcb* fcb, UINT64 start_data, UINT64 end_data, LIST_ENTRY* changed_sector_list, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    
    TRACE("(%p, (%llx, %llx), %llx, %llx, %p)\n", Vcb, fcb->subvol->id, fcb->inode, start_data, end_data, changed_sector_list);
    
    Status = excise_extents_inode(Vcb, fcb->subvol, fcb->inode, &fcb->inode_item, start_data, end_data, changed_sector_list, rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("excise_extents_inode returned %08x\n");
        return Status;
    }
    
    return STATUS_SUCCESS;
}

static NTSTATUS do_write_data(device_extension* Vcb, UINT64 address, void* data, UINT64 length, LIST_ENTRY* changed_sector_list) {
    NTSTATUS Status;
    changed_sector* sc;
    int i;
    
    Status = write_data(Vcb, address, data, length);
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

BOOL insert_extent_chunk_inode(device_extension* Vcb, root* subvol, UINT64 inode, INODE_ITEM* inode_item, chunk* c, UINT64 start_data,
                               UINT64 length, BOOL prealloc, void* data, LIST_ENTRY* changed_sector_list, LIST_ENTRY* rollback) {
    UINT64 address;
    NTSTATUS Status;
    EXTENT_DATA* ed;
    EXTENT_DATA2* ed2;
    ULONG edsize = sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2);
    
    TRACE("(%p, (%llx, %llx), %llx, %llx, %llx, %p, %p)\n", Vcb, subvol->id, inode, c->offset, start_data, length, data, changed_sector_list);
    
    if (!find_address_in_chunk(Vcb, c, length, &address))
        return FALSE;
    
    Status = increase_extent_refcount_data(Vcb, address, length, subvol, inode, start_data, 1, rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("increase_extent_refcount_data returned %08x\n", Status);
        return FALSE;
    }
    
    if (data) {
        Status = do_write_data(Vcb, address, data, length, changed_sector_list);
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
    ed->decoded_size = length;
    ed->compression = BTRFS_COMPRESSION_NONE;
    ed->encryption = BTRFS_ENCRYPTION_NONE;
    ed->encoding = BTRFS_ENCODING_NONE;
    ed->type = prealloc ? EXTENT_TYPE_PREALLOC : EXTENT_TYPE_REGULAR;
    
    ed2 = (EXTENT_DATA2*)ed->data;
    ed2->address = address;
    ed2->size = length;
    ed2->offset = 0;
    ed2->num_bytes = length;
    
    if (!insert_tree_item(Vcb, subvol, inode, TYPE_EXTENT_DATA, start_data, ed, edsize, NULL, rollback)) {
        ERR("insert_tree_item failed\n");
        ExFreePool(ed);
        return FALSE;
    }
    
    increase_chunk_usage(c, length);
    add_to_space_list(c, address, length, SPACE_TYPE_WRITING);
    
    if (inode_item) {
        inode_item->st_blocks += length;
        
        if (prealloc)
            inode_item->flags |= BTRFS_INODE_PREALLOC;
    }
    
    return TRUE;
}

static BOOL insert_extent_chunk(device_extension* Vcb, fcb* fcb, chunk* c, UINT64 start_data, UINT64 length, BOOL prealloc, void* data, LIST_ENTRY* changed_sector_list, LIST_ENTRY* rollback) {
    return insert_extent_chunk_inode(Vcb, fcb->subvol, fcb->inode, &fcb->inode_item, c, start_data, length, prealloc, data, changed_sector_list, rollback);
}

static BOOL extend_data(device_extension* Vcb, fcb* fcb, UINT64 start_data, UINT64 length, void* data,
                        LIST_ENTRY* changed_sector_list, traverse_ptr* edtp, traverse_ptr* eitp, LIST_ENTRY* rollback) {
    EXTENT_DATA* ed;
    EXTENT_DATA2* ed2;
    EXTENT_ITEM* ei;
    NTSTATUS Status;
    changed_sector* sc;
    chunk* c;
    int i;
    
    TRACE("(%p, (%llx, %llx), %llx, %llx, %p, %p, %p, %p)\n", Vcb, fcb->subvol->id, fcb->inode, start_data,
                                                                  length, data, changed_sector_list, edtp, eitp);
    
    ed = ExAllocatePoolWithTag(PagedPool, edtp->item->size, ALLOC_TAG);
    if (!ed) {
        ERR("out of memory\n");
        return FALSE;
    }
    
    RtlCopyMemory(ed, edtp->item->data, edtp->item->size);
    
    ed->decoded_size += length;
    ed2 = (EXTENT_DATA2*)ed->data;
    ed2->size += length;
    ed2->num_bytes += length;
    
    delete_tree_item(Vcb, edtp, rollback);
    
    if (!insert_tree_item(Vcb, fcb->subvol, edtp->item->key.obj_id, edtp->item->key.obj_type, edtp->item->key.offset, ed, edtp->item->size, NULL, rollback)) {
        TRACE("insert_tree_item failed\n");

        ExFreePool(ed);
        return FALSE;
    }
    
    ei = ExAllocatePoolWithTag(PagedPool, eitp->item->size, ALLOC_TAG);
    if (!ei) {
        ERR("out of memory\n");
        ExFreePool(ed);
        return FALSE;
    }
    
    RtlCopyMemory(ei, eitp->item->data, eitp->item->size);
    
    if (!insert_tree_item(Vcb, Vcb->extent_root, eitp->item->key.obj_id, eitp->item->key.obj_type, eitp->item->key.offset + length, ei, eitp->item->size, NULL, rollback)) {
        ERR("insert_tree_item failed\n");

        ExFreePool(ei);
        return FALSE;
    }
    
    delete_tree_item(Vcb, eitp, rollback);
    
    Status = write_data(Vcb, eitp->item->key.obj_id + eitp->item->key.offset, data, length);
    if (!NT_SUCCESS(Status)) {
        ERR("write_data returned %08x\n", Status);
        return FALSE;
    }
    
    if (changed_sector_list) {
        sc = ExAllocatePoolWithTag(PagedPool, sizeof(changed_sector), ALLOC_TAG);
        if (!sc) {
            ERR("out of memory\n");
            return FALSE;
        }
        
        sc->ol.key = eitp->item->key.obj_id + eitp->item->key.offset;
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
    
    c = get_chunk_from_address(Vcb, eitp->item->key.obj_id);
    
    if (c) {
        increase_chunk_usage(c, length);
        
        add_to_space_list(c, eitp->item->key.obj_id + eitp->item->key.offset, length, SPACE_TYPE_WRITING);
    }
    
    fcb->inode_item.st_blocks += length;
    
    return TRUE;
}

static BOOL try_extend_data(device_extension* Vcb, fcb* fcb, UINT64 start_data, UINT64 length, void* data,
                            LIST_ENTRY* changed_sector_list, LIST_ENTRY* rollback) {
    KEY searchkey;
    traverse_ptr tp, tp2;
    BOOL success = FALSE;
    EXTENT_DATA* ed;
    EXTENT_DATA2* ed2;
    EXTENT_ITEM* ei;
    chunk* c;
    LIST_ENTRY* le;
    space* s;
    NTSTATUS Status;
    
    searchkey.obj_id = fcb->inode;
    searchkey.obj_type = TYPE_EXTENT_DATA;
    searchkey.offset = start_data;
    
    Status = find_item(Vcb, fcb->subvol, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return FALSE;
    }
    
    if (tp.item->key.obj_id != fcb->inode || tp.item->key.obj_type != TYPE_EXTENT_DATA || tp.item->key.offset >= start_data) {
        WARN("previous EXTENT_DATA not found\n");
        goto end;
    }
    
    ed = (EXTENT_DATA*)tp.item->data;
    
    if (tp.item->size < sizeof(EXTENT_DATA)) {
        ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_DATA));
        goto end;
    }
    
    if (ed->type != EXTENT_TYPE_REGULAR) {
        TRACE("not extending extent which is not EXTENT_TYPE_REGULAR\n");
        goto end;
    }
    
    ed2 = (EXTENT_DATA2*)ed->data;
    
    if (tp.item->size < sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2)) {
        ERR("(%llx,%x,%llx) was %u bytes, expected %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2));
        goto end;
    }

    if (tp.item->key.offset + ed2->num_bytes != start_data) {
        TRACE("last EXTENT_DATA does not run up to start_data (%llx + %llx != %llx)\n", tp.item->key.offset, ed2->num_bytes, start_data);
        goto end;
    }
    
    if (ed->compression != BTRFS_COMPRESSION_NONE) {
        FIXME("FIXME: compression not yet supported\n");
        goto end;
    }
    
    if (ed->encryption != BTRFS_ENCRYPTION_NONE) {
        WARN("encryption not supported\n");
        goto end;
    }
    
    if (ed->encoding != BTRFS_ENCODING_NONE) {
        WARN("other encodings not supported\n");
        goto end;
    }
    
    if (ed2->size - ed2->offset != ed2->num_bytes) {
        TRACE("last EXTENT_DATA does not run all the way to the end of the extent\n");
        goto end;
    }
    
    searchkey.obj_id = ed2->address;
    searchkey.obj_type = TYPE_EXTENT_ITEM;
    searchkey.offset = ed2->size;
    
    Status = find_item(Vcb, Vcb->extent_root, &tp2, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        goto end;
    }
    
    if (keycmp(&tp2.item->key, &searchkey)) {
        ERR("error - extent %llx,%llx not found in tree\n", ed2->address, ed2->size);
        int3; // TESTING
        goto end;
    }
    
    if (tp2.item->size == sizeof(EXTENT_ITEM_V0)) { // old extent ref, convert
        NTSTATUS Status = convert_old_data_extent(Vcb, ed2->address, ed2->size, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("convert_old_data_extent returned %08x\n", Status);
            goto end;
        }
        
        Status = find_item(Vcb, Vcb->extent_root, &tp2, &searchkey, FALSE);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            goto end;
        }
        
        if (keycmp(&tp2.item->key, &searchkey)) {
            WARN("extent item not found for address %llx, size %llx\n", ed2->address, ed2->size);
            goto end;
        }
    }
    
    ei = (EXTENT_ITEM*)tp2.item->data;
    
    if (tp.item->size < sizeof(EXTENT_ITEM)) {
        ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_ITEM));
        goto end;
    }
    
    // FIXME - test this
    if (extent_item_is_shared(ei, tp2.item->size - sizeof(EXTENT_ITEM))) {
        NTSTATUS Status = convert_shared_data_extent(Vcb, ed2->address, ed2->size, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("convert_shared_data_extent returned %08x\n", Status);
            goto end;
        }
        
        Status = find_item(Vcb, Vcb->extent_root, &tp2, &searchkey, FALSE);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            goto end;
        }
        
        if (keycmp(&tp2.item->key, &searchkey)) {
            WARN("extent item not found for address %llx, size %llx\n", ed2->address, ed2->size);
            goto end;
        }
        
        ei = (EXTENT_ITEM*)tp2.item->data;
        
        if (tp.item->size < sizeof(EXTENT_ITEM)) {
            ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_ITEM));
            goto end;
        }
    }
    
    if (ei->refcount != 1) {
        TRACE("extent refcount was not 1\n");
        goto end;
    }
    
    if (ei->flags != EXTENT_ITEM_DATA) {
        ERR("error - extent was not a data extent\n");
        goto end;
    }
    
    c = get_chunk_from_address(Vcb, ed2->address);
    
    le = c->space.Flink;
    while (le != &c->space) {
        s = CONTAINING_RECORD(le, space, list_entry);
        
        if (s->offset == ed2->address + ed2->size) {
            if (s->type == SPACE_TYPE_FREE && s->size >= length) {
                success = extend_data(Vcb, fcb, start_data, length, data, changed_sector_list, &tp, &tp2, rollback);
            }
            break;
        } else if (s->offset > ed2->address + ed2->size)
            break;
        
        le = le->Flink;
    }
    
end:
        
    return success;
}

static NTSTATUS insert_prealloc_extent(fcb* fcb, UINT64 start, UINT64 length, LIST_ENTRY* rollback) {
    LIST_ENTRY* le = fcb->Vcb->chunks.Flink;
    chunk* c;
    UINT64 flags;
    
    // FIXME - how do we know which RAID level to put this to?
    flags = BLOCK_FLAG_DATA; // SINGLE
    
    // FIXME - if length is more than max chunk size, loop through and
    // create the new chunks first
    
    while (le != &fcb->Vcb->chunks) {
        c = CONTAINING_RECORD(le, chunk, list_entry);
        
        if (c->chunk_item->type == flags && (c->chunk_item->size - c->used) >= length) {
            if (insert_extent_chunk(fcb->Vcb, fcb, c, start, length, TRUE, NULL, NULL, rollback))
                return STATUS_SUCCESS;
        }

        le = le->Flink;
    }
    
    if ((c = alloc_chunk(fcb->Vcb, flags, rollback))) {
        if (c->chunk_item->type == flags && (c->chunk_item->size - c->used) >= length) {
            if (insert_extent_chunk(fcb->Vcb, fcb, c, start, length, TRUE, NULL, NULL, rollback))
                return STATUS_SUCCESS;
        }
    }
    
    // FIXME - rebalance chunks if free space elsewhere?
    WARN("couldn't find any data chunks with %llx bytes free\n", length);

    return STATUS_DISK_FULL;
}

NTSTATUS insert_sparse_extent(device_extension* Vcb, root* r, UINT64 inode, UINT64 start, UINT64 length, LIST_ENTRY* rollback) {
    EXTENT_DATA* ed;
    EXTENT_DATA2* ed2;
    
    TRACE("(%p, %llx, %llx, %llx, %llx)\n", Vcb, r->id, inode, start, length);
    
    ed = ExAllocatePoolWithTag(PagedPool, sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2), ALLOC_TAG);
    if (!ed) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    ed->generation = Vcb->superblock.generation;
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
    
    if (!insert_tree_item(Vcb, r, inode, TYPE_EXTENT_DATA, start, ed, sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2), NULL, rollback)) {
        ERR("insert_tree_item failed\n");
        ExFreePool(ed);
        return STATUS_INTERNAL_ERROR;
    }
    
    return STATUS_SUCCESS;
}

// static void print_tree(tree* t) {
//     LIST_ENTRY* le = t->itemlist.Flink;
//     while (le != &t->itemlist) {
//         tree_data* td = CONTAINING_RECORD(le, tree_data, list_entry);
//         ERR("%llx,%x,%llx (ignore = %s)\n", td->key.obj_id, td->key.obj_type, td->key.offset, td->ignore ? "TRUE" : "FALSE");
//         le = le->Flink;
//     }
// }

static NTSTATUS insert_extent(device_extension* Vcb, fcb* fcb, UINT64 start_data, UINT64 length, void* data, LIST_ENTRY* changed_sector_list, LIST_ENTRY* rollback) {
    LIST_ENTRY* le = Vcb->chunks.Flink;
    chunk* c;
    KEY searchkey;
    UINT64 flags;
    
    TRACE("(%p, (%llx, %llx), %llx, %llx, %p, %p)\n", Vcb, fcb->subvol->id, fcb->inode, start_data, length, data, changed_sector_list);
    
    // FIXME - split data up if not enough space for just one extent
    
    if (start_data > 0 && try_extend_data(Vcb, fcb, start_data, length, data, changed_sector_list, rollback))
        return STATUS_SUCCESS;
    
    // if there is a gap before start_data, plug it with a sparse extent
    if (start_data > 0) {
        traverse_ptr tp;
        NTSTATUS Status;
        EXTENT_DATA* ed;
        UINT64 len;
        
        searchkey.obj_id = fcb->inode;
        searchkey.obj_type = TYPE_EXTENT_DATA;
        searchkey.offset = start_data;
        
        Status = find_item(Vcb, fcb->subvol, &tp, &searchkey, FALSE);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            return Status;
        }
        
//         if (tp.item->key.obj_id != fcb->inode || tp.item->key.obj_type != TYPE_EXTENT_DATA || tp.item->key.offset >= start_data) {
//             traverse_ptr next_tp;
//             
//             ERR("error - did not find EXTENT_DATA expected - looking for %llx,%x,%llx, found %llx,%x,%llx\n",
//                 searchkey.obj_id, searchkey.obj_type, searchkey.offset, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
//             print_tree(tp.tree);
//             
//             if (find_next_item(Vcb, &tp, &next_tp, FALSE)) {
//                 ERR("---\n");
//                 ERR("key = %llx,%x,%llx\n", next_tp.tree->paritem->key.obj_id, next_tp.tree->paritem->key.obj_type, next_tp.tree->paritem->key.offset);
//                 print_tree(next_tp.tree);
//                 
//                 free_traverse_ptr(&next_tp);
//             } else
//                 ERR("next item not found\n");
//             
//             int3;
//             free_traverse_ptr(&tp);
//             return STATUS_INTERNAL_ERROR;
//         }

        if (tp.item->key.obj_type == TYPE_EXTENT_DATA && tp.item->size >= sizeof(EXTENT_DATA)) {
            EXTENT_DATA2* ed2;
            
            ed = (EXTENT_DATA*)tp.item->data;
            ed2 = (EXTENT_DATA2*)ed->data;
            
            len = ed->type == EXTENT_TYPE_INLINE ? ed->decoded_size : ed2->num_bytes;
        } else
            ed = NULL;
        
        if (tp.item->key.obj_id != fcb->inode || tp.item->key.obj_type != TYPE_EXTENT_DATA || !ed || tp.item->key.offset + len < start_data) {
            if (tp.item->key.obj_id != fcb->inode || tp.item->key.obj_type != TYPE_EXTENT_DATA)
                Status = insert_sparse_extent(Vcb, fcb->subvol, fcb->inode, 0, start_data, rollback);
            else if (!ed)
                ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_DATA));
            else {
                Status = insert_sparse_extent(Vcb, fcb->subvol, fcb->inode, tp.item->key.offset + len,
                                              start_data - tp.item->key.offset - len, rollback);
            }
            if (!NT_SUCCESS(Status)) {
                ERR("insert_sparse_extent returned %08x\n", Status);
                return Status;
            }
        }
    }
    
    // FIXME - how do we know which RAID level to put this to?
    flags = BLOCK_FLAG_DATA; // SINGLE
    
//     if (!chunk_test) { // TESTING
//         if ((c = alloc_chunk(Vcb, flags, NULL))) {
//             ERR("chunk_item->type = %llx\n", c->chunk_item->type);
//             ERR("size = %llx\n", c->chunk_item->size);
//             ERR("used = %llx\n", c->used);
//             
//             if (c->chunk_item->type == flags && (c->chunk_item->size - c->used) >= length) {
//                 if (insert_extent_chunk(Vcb, fcb, c, start_data, length, data, changed_sector_list)) {
// //                     chunk_test = TRUE;
//                     ERR("SUCCESS\n");
//                     return STATUS_SUCCESS;
//                 } else
//                     ERR(":-(\n");
//             } else
//                 ERR("???\n");
//         }
//     }
    
    while (le != &Vcb->chunks) {
        c = CONTAINING_RECORD(le, chunk, list_entry);
        
        if (c->chunk_item->type == flags && (c->chunk_item->size - c->used) >= length) {
            if (insert_extent_chunk(Vcb, fcb, c, start_data, length, FALSE, data, changed_sector_list, rollback))
                return STATUS_SUCCESS;
        }

        le = le->Flink;
    }
    
    if ((c = alloc_chunk(Vcb, flags, rollback))) {
        if (c->chunk_item->type == flags && (c->chunk_item->size - c->used) >= length) {
            if (insert_extent_chunk(Vcb, fcb, c, start_data, length, FALSE, data, changed_sector_list, rollback))
                return STATUS_SUCCESS;
        }
    }
    
    // FIXME - rebalance chunks if free space elsewhere?
    WARN("couldn't find any data chunks with %llx bytes free\n", length);

    return STATUS_DISK_FULL;
}

void update_checksum_tree(device_extension* Vcb, LIST_ENTRY* changed_sector_list, LIST_ENTRY* rollback) {
    LIST_ENTRY* le = changed_sector_list->Flink;
    changed_sector* cs;
    traverse_ptr tp, next_tp;
    KEY searchkey;
    UINT32* data;
    NTSTATUS Status;
    
    if (!Vcb->checksum_root) {
        ERR("no checksum root\n");
        goto exit;
    }
    
    while (le != changed_sector_list) {
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
        
        Status = find_item(Vcb, Vcb->checksum_root, &tp, &searchkey, FALSE);
        if (!NT_SUCCESS(Status)) { // tree is completely empty
            // FIXME - do proper check here that tree is empty
            if (!cs->deleted) {
                checksums = ExAllocatePoolWithTag(PagedPool, sizeof(UINT32) * cs->length, ALLOC_TAG);
                if (!checksums) {
                    ERR("out of memory\n");
                    goto exit;
                }
                
                RtlCopyMemory(checksums, cs->checksums, sizeof(UINT32) * cs->length);
                
                if (!insert_tree_item(Vcb, Vcb->checksum_root, EXTENT_CSUM_ID, TYPE_EXTENT_CSUM, cs->ol.key, checksums, sizeof(UINT32) * cs->length, NULL, rollback)) {
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
            
            Status = find_item(Vcb, Vcb->checksum_root, &tp, &searchkey, FALSE);
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
            
            Status = find_item(Vcb, Vcb->checksum_root, &tp, &searchkey, FALSE);
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
                
                if (find_next_item(Vcb, &tp, &next_tp, FALSE)) {
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
                    
                    if (!insert_tree_item(Vcb, Vcb->checksum_root, EXTENT_CSUM_ID, TYPE_EXTENT_CSUM, startaddr + (index * Vcb->superblock.sector_size), data, sizeof(UINT32) * rl, NULL, rollback)) {
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
    while (!IsListEmpty(changed_sector_list)) {
        le = RemoveHeadList(changed_sector_list);
        cs = (changed_sector*)le;
        
        if (cs->checksums)
            ExFreePool(cs->checksums);
        
        ExFreePool(cs);
    }
}

NTSTATUS truncate_file(fcb* fcb, UINT64 end, LIST_ENTRY* rollback) {
    LIST_ENTRY changed_sector_list;
    NTSTATUS Status;
    BOOL nocsum = fcb->inode_item.flags & BTRFS_INODE_NODATASUM;
    
    if (!nocsum)
        InitializeListHead(&changed_sector_list);
    
    // FIXME - convert into inline extent if short enough
    
    Status = excise_extents(fcb->Vcb, fcb, sector_align(end, fcb->Vcb->superblock.sector_size),
                            sector_align(fcb->inode_item.st_size, fcb->Vcb->superblock.sector_size), nocsum ? NULL : &changed_sector_list, rollback);
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
    
    if (!nocsum)
        update_checksum_tree(fcb->Vcb, &changed_sector_list, rollback);
    
    return STATUS_SUCCESS;
}

NTSTATUS extend_file(fcb* fcb, file_ref* fileref, UINT64 end, BOOL prealloc, LIST_ENTRY* rollback) {
    UINT64 oldalloc, newalloc;
    KEY searchkey;
    traverse_ptr tp;
    BOOL cur_inline;
    NTSTATUS Status;
    
    TRACE("(%p, %x, %p)\n", fcb, end, rollback);

    if (fcb->ads)
        return stream_set_end_of_file_information(fcb->Vcb, end, fcb, fileref, NULL, FALSE, rollback) ;
    else {
        searchkey.obj_id = fcb->inode;
        searchkey.obj_type = TYPE_EXTENT_DATA;
        searchkey.offset = 0xffffffffffffffff;
        
        Status = find_item(fcb->Vcb, fcb->subvol, &tp, &searchkey, FALSE);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            return Status;
        }
        
        oldalloc = 0;
        if (tp.item->key.obj_id == fcb->inode && tp.item->key.obj_type == TYPE_EXTENT_DATA) {
            EXTENT_DATA* ed = (EXTENT_DATA*)tp.item->data;
            EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ed->data;
            
            if (tp.item->size < sizeof(EXTENT_DATA)) {
                ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_DATA));
                return STATUS_INTERNAL_ERROR;
            }
            
            oldalloc = tp.item->key.offset + (ed->type == EXTENT_TYPE_INLINE ? ed->decoded_size : ed2->num_bytes);
            cur_inline = ed->type == EXTENT_TYPE_INLINE;
        
            if (cur_inline && end > fcb->Vcb->max_inline) {
                LIST_ENTRY changed_sector_list;
                BOOL nocsum = fcb->inode_item.flags & BTRFS_INODE_NODATASUM;
                UINT64 origlength, length;
                UINT8* data;
                
                TRACE("giving inline file proper extents\n");
                
                origlength = ed->decoded_size;
                
                cur_inline = FALSE;
                
                if (!nocsum)
                    InitializeListHead(&changed_sector_list);
                
                delete_tree_item(fcb->Vcb, &tp, rollback);
                
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
                
                Status = insert_extent(fcb->Vcb, fcb, tp.item->key.offset, length, data, nocsum ? NULL : &changed_sector_list, rollback);
                if (!NT_SUCCESS(Status)) {
                    ERR("insert_extent returned %08x\n", Status);
                    ExFreePool(data);
                    return Status;
                }
                
                oldalloc = tp.item->key.offset + length;
                
                ExFreePool(data);
                
                if (!nocsum)
                    update_checksum_tree(fcb->Vcb, &changed_sector_list, rollback);
            }
            
            if (cur_inline) {
                ULONG edsize;
                
                if (end > oldalloc) {
                    edsize = sizeof(EXTENT_DATA) - 1 + end - tp.item->key.offset;
                    ed = ExAllocatePoolWithTag(PagedPool, edsize, ALLOC_TAG);
                    
                    if (!ed) {
                        ERR("out of memory\n");
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }
                    
                    RtlZeroMemory(ed, edsize);
                    RtlCopyMemory(ed, tp.item->data, tp.item->size);
                    
                    ed->decoded_size = end - tp.item->key.offset;
                    
                    delete_tree_item(fcb->Vcb, &tp, rollback);
                    
                    if (!insert_tree_item(fcb->Vcb, fcb->subvol, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, ed, edsize, NULL, rollback)) {
                        ERR("error - failed to insert item\n");
                        ExFreePool(ed);
                        return STATUS_INTERNAL_ERROR;
                    }
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
                    } else {
                        Status = insert_sparse_extent(fcb->Vcb, fcb->subvol, fcb->inode, oldalloc, newalloc - oldalloc, rollback);
                        
                        if (!NT_SUCCESS(Status)) {
                            ERR("insert_sparse_extent returned %08x\n", Status);
                            return Status;
                        }
                    }
                }
                
                fcb->inode_item.st_size = end;
                TRACE("setting st_size to %llx\n", end);
                
                TRACE("newalloc = %llx\n", newalloc);
                
                fcb->Header.AllocationSize.QuadPart = newalloc;
                fcb->Header.FileSize.QuadPart = fcb->Header.ValidDataLength.QuadPart = end;
            }
        } else {
            if (end > fcb->Vcb->max_inline) {
                newalloc = sector_align(end, fcb->Vcb->superblock.sector_size);
            
                if (prealloc) {
                    Status = insert_prealloc_extent(fcb, 0, newalloc, rollback);
                    
                    if (!NT_SUCCESS(Status)) {
                        ERR("insert_prealloc_extent returned %08x\n", Status);
                        return Status;
                    }
                } else {
                    Status = insert_sparse_extent(fcb->Vcb, fcb->subvol, fcb->inode, 0, newalloc, rollback);
                    
                    if (!NT_SUCCESS(Status)) {
                        ERR("insert_sparse_extent returned %08x\n", Status);
                        return Status;
                    }
                }
                
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

                if (!insert_tree_item(fcb->Vcb, fcb->subvol, fcb->inode, TYPE_EXTENT_DATA, 0, ed, edsize, NULL, rollback)) {
                    ERR("error - failed to insert item\n");
                    ExFreePool(ed);
                    return STATUS_INTERNAL_ERROR;
                }
                
                fcb->inode_item.st_size = end;
                TRACE("setting st_size to %llx\n", end);
                
                fcb->inode_item.st_blocks = end;

                fcb->Header.AllocationSize.QuadPart = fcb->Header.FileSize.QuadPart = fcb->Header.ValidDataLength.QuadPart = end;
            }
        }
    }
    
    return STATUS_SUCCESS;
}

static UINT64 get_extent_item_refcount(device_extension* Vcb, UINT64 address) {
    KEY searchkey;
    traverse_ptr tp;
    EXTENT_ITEM* ei;
    UINT64 rc;
    NTSTATUS Status;
    
    searchkey.obj_id = address;
    searchkey.obj_type = TYPE_EXTENT_ITEM;
    searchkey.offset = 0xffffffffffffffff;
    
    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return 0;
    }
    
    if (tp.item->key.obj_id != searchkey.obj_id || tp.item->key.obj_type != searchkey.obj_type) {
        ERR("error - could not find EXTENT_ITEM for %llx\n", address);
        return 0;
    }
    
    if (tp.item->size < sizeof(EXTENT_ITEM)) {
        ERR("(%llx,%x,%llx) was %u bytes, expected %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_ITEM));
        return 0;
    }
    
    ei = (EXTENT_ITEM*)tp.item->data;
    rc = ei->refcount;
    
    return rc;
}

static BOOL is_file_prealloc(fcb* fcb, UINT64 start_data, UINT64 end_data) {
    return is_file_prealloc_inode(fcb->Vcb, fcb->subvol, fcb->inode, start_data, end_data);
}

static NTSTATUS do_cow_write(device_extension* Vcb, fcb* fcb, UINT64 start_data, UINT64 end_data, void* data, LIST_ENTRY* changed_sector_list, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    
    Status = excise_extents(fcb->Vcb, fcb, start_data, end_data, changed_sector_list, rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("error - excise_extents returned %08x\n", Status);
        goto end;
    }
    
    Status = insert_extent(fcb->Vcb, fcb, start_data, end_data - start_data, data, changed_sector_list, rollback);
    
    if (!NT_SUCCESS(Status)) {
        ERR("error - insert_extent returned %08x\n", Status);
        goto end;
    }
    
    Status = STATUS_SUCCESS;
    
end:
    return Status;
}

static NTSTATUS merge_data_extents(device_extension* Vcb, fcb* fcb, UINT64 start_data, UINT64 end_data, LIST_ENTRY* rollback) {
    KEY searchkey;
    traverse_ptr tp, next_tp;
    NTSTATUS Status;
    BOOL b;
    EXTENT_DATA* ed;
    
    searchkey.obj_id = fcb->inode;
    searchkey.obj_type = TYPE_EXTENT_DATA;
    searchkey.offset = start_data;
    
    Status = find_item(Vcb, fcb->subvol, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    if (tp.item->key.obj_id != fcb->inode || tp.item->key.obj_type != TYPE_EXTENT_DATA) {
        ERR("error - EXTENT_DATA not found\n");
        return STATUS_INTERNAL_ERROR;
    }
    
    if (tp.item->key.offset > 0) {
        traverse_ptr tp2, prev_tp;
        
        tp2 = tp;
        do {
            b = find_prev_item(Vcb, &tp2, &prev_tp, FALSE);
            
            if (b) {
                if (!prev_tp.item->ignore)
                    break;
                
                tp2 = prev_tp;
            }
        } while (b);
        
        if (b) {
            if (prev_tp.item->key.obj_id == fcb->inode && prev_tp.item->key.obj_type == TYPE_EXTENT_DATA)
                tp = prev_tp;
        }
    }
    
    ed = (EXTENT_DATA*)tp.item->data;
    if ((ed->type == EXTENT_TYPE_REGULAR || ed->type == EXTENT_TYPE_PREALLOC) && tp.item->size < sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2)) {
        ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2));
        return STATUS_INTERNAL_ERROR;
    }
    
    do {
        b = find_next_item(Vcb, &tp, &next_tp, FALSE);
        
        if (b) {
            EXTENT_DATA* ned;
            
            if (next_tp.item->key.obj_id != fcb->inode || next_tp.item->key.obj_type != TYPE_EXTENT_DATA)
                return STATUS_SUCCESS;
            
            if (next_tp.item->size < sizeof(EXTENT_DATA)) {
                ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", next_tp.item->key.obj_id, next_tp.item->key.obj_type, next_tp.item->key.offset, next_tp.item->size, sizeof(EXTENT_DATA));
                return STATUS_INTERNAL_ERROR;
            }
            
            ned = (EXTENT_DATA*)next_tp.item->data;
            if ((ned->type == EXTENT_TYPE_REGULAR || ned->type == EXTENT_TYPE_PREALLOC) && next_tp.item->size < sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2)) {
                ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", next_tp.item->key.obj_id, next_tp.item->key.obj_type, next_tp.item->key.offset, next_tp.item->size, sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2));
                return STATUS_INTERNAL_ERROR;
            }
            
            if (ed->type == ned->type && (ed->type == EXTENT_TYPE_REGULAR || ed->type == EXTENT_TYPE_PREALLOC)) {
                EXTENT_DATA2 *ed2, *ned2;
                
                ed2 = (EXTENT_DATA2*)ed->data;
                ned2 = (EXTENT_DATA2*)ned->data;
                
                if (next_tp.item->key.offset == tp.item->key.offset + ed2->num_bytes && ed2->address == ned2->address && ed2->size == ned2->size && ned2->offset == ed2->offset + ed2->num_bytes) {
                    EXTENT_DATA* buf = ExAllocatePoolWithTag(PagedPool, tp.item->size, ALLOC_TAG);
                    EXTENT_DATA2* buf2;
                    traverse_ptr tp2;
                    
                    if (!buf) {
                        ERR("out of memory\n");
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }
                    
                    RtlCopyMemory(buf, tp.item->data, tp.item->size);
                    buf->generation = Vcb->superblock.generation;
                    
                    buf2 = (EXTENT_DATA2*)buf->data;
                    buf2->num_bytes += ned2->num_bytes;
                    
                    delete_tree_item(Vcb, &tp, rollback);
                    delete_tree_item(Vcb, &next_tp, rollback);
                    
                    if (!insert_tree_item(Vcb, fcb->subvol, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, buf, tp.item->size, &tp2, rollback)) {
                        ERR("insert_tree_item failed\n");
                        ExFreePool(buf);
                        return STATUS_INTERNAL_ERROR;
                    }
                    
                    Status = decrease_extent_refcount_data(Vcb, ed2->address, ed2->size, fcb->subvol, fcb->inode, tp.item->key.offset - buf2->offset, 1, NULL, rollback);
                    if (!NT_SUCCESS(Status)) {
                        ERR("decrease_extent_refcount_data returned %08x\n", Status);
                        return Status;
                    }
                        
                    tp = tp2;
                    
                    continue;
                }
            }

            tp = next_tp;
            ed = ned;
        }
    } while (b);
    
    return STATUS_SUCCESS;
}

static NTSTATUS do_prealloc_write(device_extension* Vcb, fcb* fcb, UINT64 start_data, UINT64 end_data, void* data, LIST_ENTRY* changed_sector_list, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    KEY searchkey;
    traverse_ptr tp, next_tp;
    BOOL b, deleted_prealloc = FALSE;
    UINT64 last_written = start_data;
    
    searchkey.obj_id = fcb->inode;
    searchkey.obj_type = TYPE_EXTENT_DATA;
    searchkey.offset = start_data;
    
    Status = find_item(fcb->Vcb, fcb->subvol, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    if (tp.item->key.obj_id != fcb->inode || tp.item->key.obj_type != TYPE_EXTENT_DATA)
        return do_cow_write(Vcb, fcb, start_data, end_data, data, changed_sector_list, rollback);
    
    do {
        EXTENT_DATA* ed = (EXTENT_DATA*)tp.item->data;
        EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ed->data;
        
        if (tp.item->size < sizeof(EXTENT_DATA)) {
            ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_DATA));
            return STATUS_INTERNAL_ERROR;
        }
        
        if ((ed->type == EXTENT_TYPE_REGULAR || ed->type == EXTENT_TYPE_PREALLOC) && tp.item->size < sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2)) {
            ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2));
            return STATUS_INTERNAL_ERROR;
        }
        
        b = find_next_item(fcb->Vcb, &tp, &next_tp, FALSE);
        
        if (ed->type == EXTENT_TYPE_PREALLOC) {
            if (tp.item->key.offset > last_written) {
                Status = do_cow_write(Vcb, fcb, last_written, tp.item->key.offset, (UINT8*)data + last_written - start_data, changed_sector_list, rollback);
                
                if (!NT_SUCCESS(Status)) {
                    ERR("do_cow_write returned %08x\n", Status);
                    
                    return Status;
                }
                
                last_written = tp.item->key.offset;
            }
            
            if (start_data <= tp.item->key.offset && end_data >= tp.item->key.offset + ed2->num_bytes) { // replace all
                EXTENT_DATA* ned = ExAllocatePoolWithTag(PagedPool, tp.item->size, ALLOC_TAG);
                
                if (!ned) {
                    ERR("out of memory\n");
                    
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                
                RtlCopyMemory(ned, tp.item->data, tp.item->size);
                
                ned->type = EXTENT_TYPE_REGULAR;
                
                delete_tree_item(Vcb, &tp, rollback);
                
                if (!insert_tree_item(Vcb, fcb->subvol, fcb->inode, TYPE_EXTENT_DATA, tp.item->key.offset, ned, tp.item->size, NULL, rollback)) {
                    ERR("insert_tree_item failed\n");
                    
                    return STATUS_INTERNAL_ERROR;
                }
                
                Status = do_write_data(Vcb, ed2->address + ed2->offset, (UINT8*)data + tp.item->key.offset - start_data, ed2->num_bytes, changed_sector_list);
                if (!NT_SUCCESS(Status)) {
                    ERR("do_write_data returned %08x\n", Status);
                    
                    return Status;
                }
                
                deleted_prealloc = TRUE;
                
                last_written = tp.item->key.offset + ed2->num_bytes;
            } else if (start_data <= tp.item->key.offset && end_data < tp.item->key.offset + ed2->num_bytes) { // replace beginning
                EXTENT_DATA *ned, *nedb;
                EXTENT_DATA2* ned2;
                
                ned = ExAllocatePoolWithTag(PagedPool, tp.item->size, ALLOC_TAG);
                
                if (!ned) {
                    ERR("out of memory\n");
                    
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                
                nedb = ExAllocatePoolWithTag(PagedPool, tp.item->size, ALLOC_TAG);
                
                if (!nedb) {
                    ERR("out of memory\n");
                    ExFreePool(ned);
                    
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                
                delete_tree_item(Vcb, &tp, rollback);
                
                RtlCopyMemory(ned, tp.item->data, tp.item->size);
                
                ned->type = EXTENT_TYPE_REGULAR;
                ned2 = (EXTENT_DATA2*)ned->data;
                ned2->num_bytes = end_data - tp.item->key.offset;
                
                if (!insert_tree_item(Vcb, fcb->subvol, fcb->inode, TYPE_EXTENT_DATA, tp.item->key.offset, ned, tp.item->size, NULL, rollback)) {
                    ERR("insert_tree_item failed\n");
                    ExFreePool(ned);
                    ExFreePool(nedb);
                    
                    return STATUS_INTERNAL_ERROR;
                }
                
                RtlCopyMemory(nedb, tp.item->data, tp.item->size);
                ned2 = (EXTENT_DATA2*)nedb->data;
                ned2->offset += end_data - tp.item->key.offset;
                ned2->num_bytes -= end_data - tp.item->key.offset;
                
                if (!insert_tree_item(Vcb, fcb->subvol, fcb->inode, TYPE_EXTENT_DATA, end_data, nedb, tp.item->size, NULL, rollback)) {
                    ERR("insert_tree_item failed\n");
                    ExFreePool(nedb);
                    
                    return STATUS_INTERNAL_ERROR;
                }
                
                Status = do_write_data(Vcb, ed2->address + ed2->offset, (UINT8*)data + tp.item->key.offset - start_data, end_data - tp.item->key.offset, changed_sector_list);
                if (!NT_SUCCESS(Status)) {
                    ERR("do_write_data returned %08x\n", Status);
                    
                    return Status;
                }
                
                Status = increase_extent_refcount_data(Vcb, ned2->address, ned2->size, fcb->subvol, fcb->inode, tp.item->key.offset - ed2->offset, 1, rollback);
                if (!NT_SUCCESS(Status)) {
                    ERR("increase_extent_refcount_data returned %08x\n", Status);
                    return Status;
                }
                
                last_written = end_data;
            } else if (start_data > tp.item->key.offset && end_data >= tp.item->key.offset + ed2->num_bytes) { // replace end
                EXTENT_DATA *ned, *nedb;
                EXTENT_DATA2* ned2;
                
                // FIXME - test this
                
                ned = ExAllocatePoolWithTag(PagedPool, tp.item->size, ALLOC_TAG);
                
                if (!ned) {
                    ERR("out of memory\n");
                    
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                
                nedb = ExAllocatePoolWithTag(PagedPool, tp.item->size, ALLOC_TAG);
                
                if (!nedb) {
                    ERR("out of memory\n");
                    ExFreePool(ned);
                    
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                
                delete_tree_item(Vcb, &tp, rollback);
                
                RtlCopyMemory(ned, tp.item->data, tp.item->size);
                
                ned2 = (EXTENT_DATA2*)ned->data;
                ned2->num_bytes = start_data - tp.item->key.offset;
                
                if (!insert_tree_item(Vcb, fcb->subvol, fcb->inode, TYPE_EXTENT_DATA, tp.item->key.offset, ned, tp.item->size, NULL, rollback)) {
                    ERR("insert_tree_item failed\n");
                    ExFreePool(ned);
                    ExFreePool(nedb);
                    
                    return STATUS_INTERNAL_ERROR;
                }
                
                RtlCopyMemory(nedb, tp.item->data, tp.item->size);
                
                nedb->type = EXTENT_TYPE_REGULAR;
                ned2 = (EXTENT_DATA2*)nedb->data;
                ned2->offset += start_data - tp.item->key.offset;
                ned2->num_bytes = tp.item->key.offset + ed2->num_bytes - start_data;
                
                if (!insert_tree_item(Vcb, fcb->subvol, fcb->inode, TYPE_EXTENT_DATA, start_data, nedb, tp.item->size, NULL, rollback)) {
                    ERR("insert_tree_item failed\n");
                    ExFreePool(nedb);
                    
                    return STATUS_INTERNAL_ERROR;
                }
                
                Status = do_write_data(Vcb, ed2->address + ned2->offset, data, ned2->num_bytes, changed_sector_list);
                if (!NT_SUCCESS(Status)) {
                    ERR("do_write_data returned %08x\n", Status);
                    
                    return Status;
                }
                
                Status = increase_extent_refcount_data(Vcb, ned2->address, ned2->size, fcb->subvol, fcb->inode, tp.item->key.offset - ed2->offset, 1, rollback);
                if (!NT_SUCCESS(Status)) {
                    ERR("increase_extent_refcount_data returned %08x\n", Status);
                    
                    return Status;
                }
                
                last_written = start_data + ned2->num_bytes;
            } else if (start_data > tp.item->key.offset && end_data < tp.item->key.offset + ed2->num_bytes) { // replace middle
                EXTENT_DATA *ned, *nedb, *nedc;
                EXTENT_DATA2* ned2;
                
                // FIXME - test this
                
                ned = ExAllocatePoolWithTag(PagedPool, tp.item->size, ALLOC_TAG);
                
                if (!ned) {
                    ERR("out of memory\n");
                    
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                
                nedb = ExAllocatePoolWithTag(PagedPool, tp.item->size, ALLOC_TAG);
                
                if (!nedb) {
                    ERR("out of memory\n");
                    ExFreePool(ned);
                    
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                
                nedc = ExAllocatePoolWithTag(PagedPool, tp.item->size, ALLOC_TAG);
                
                if (!nedb) {
                    ERR("out of memory\n");
                    ExFreePool(nedb);
                    ExFreePool(ned);
                    
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                
                delete_tree_item(Vcb, &tp, rollback);
                
                RtlCopyMemory(ned, tp.item->data, tp.item->size);
                RtlCopyMemory(nedb, tp.item->data, tp.item->size);
                RtlCopyMemory(nedc, tp.item->data, tp.item->size);
                
                ned2 = (EXTENT_DATA2*)ned->data;
                ned2->num_bytes = start_data - tp.item->key.offset;
                
                if (!insert_tree_item(Vcb, fcb->subvol, fcb->inode, TYPE_EXTENT_DATA, tp.item->key.offset, ned, tp.item->size, NULL, rollback)) {
                    ERR("insert_tree_item failed\n");
                    ExFreePool(ned);
                    ExFreePool(nedb);
                    ExFreePool(nedc);
                    
                    return STATUS_INTERNAL_ERROR;
                }
                
                nedb->type = EXTENT_TYPE_REGULAR;
                ned2 = (EXTENT_DATA2*)nedb->data;
                ned2->offset += start_data - tp.item->key.offset;
                ned2->num_bytes = end_data - start_data;
                
                if (!insert_tree_item(Vcb, fcb->subvol, fcb->inode, TYPE_EXTENT_DATA, start_data, nedb, tp.item->size, NULL, rollback)) {
                    ERR("insert_tree_item failed\n");
                    ExFreePool(nedb);
                    ExFreePool(nedc);
                    
                    return STATUS_INTERNAL_ERROR;
                }
                
                ned2 = (EXTENT_DATA2*)nedc->data;
                ned2->offset += end_data - tp.item->key.offset;
                ned2->num_bytes -= end_data - tp.item->key.offset;
                
                if (!insert_tree_item(Vcb, fcb->subvol, fcb->inode, TYPE_EXTENT_DATA, end_data, nedc, tp.item->size, NULL, rollback)) {
                    ERR("insert_tree_item failed\n");
                    ExFreePool(nedc);
                    
                    return STATUS_INTERNAL_ERROR;
                }
                
                ned2 = (EXTENT_DATA2*)nedb->data;
                Status = do_write_data(Vcb, ed2->address + ned2->offset, data, end_data - start_data, changed_sector_list);
                if (!NT_SUCCESS(Status)) {
                    ERR("do_write_data returned %08x\n", Status);
                    
                    return Status;
                }
                
                Status = increase_extent_refcount_data(Vcb, ed2->address, ed2->size, fcb->subvol, fcb->inode, tp.item->key.offset - ed2->offset, 2, rollback);
                if (!NT_SUCCESS(Status)) {
                    ERR("increase_extent_refcount_data returned %08x\n", Status);
                    return Status;
                }
                
                last_written = end_data;
            }
        }
        
        if (b) {
            tp = next_tp;
            
            if (tp.item->key.obj_id > fcb->inode || tp.item->key.obj_type > TYPE_EXTENT_DATA || tp.item->key.offset >= end_data)
                break;
        }
    } while (b);
    
    if (last_written < end_data) {
        Status = do_cow_write(Vcb, fcb, last_written, end_data, (UINT8*)data + last_written - start_data, changed_sector_list, rollback);
                
        if (!NT_SUCCESS(Status)) {
            ERR("do_cow_write returned %08x\n", Status);
            return Status;
        }
    }
    
    Status = merge_data_extents(Vcb, fcb, start_data, end_data, rollback);
    if (!NT_SUCCESS(Status)) {
        ERR("merge_data_extents returned %08x\n", Status);
        return Status;
    }
    
    if (deleted_prealloc && !is_file_prealloc(fcb, 0, sector_align(fcb->inode_item.st_size, Vcb->superblock.sector_size)))
        fcb->inode_item.flags &= ~BTRFS_INODE_PREALLOC;
    
    return STATUS_SUCCESS;
}

static NTSTATUS do_nocow_write(device_extension* Vcb, fcb* fcb, UINT64 start_data, UINT64 length, void* data, LIST_ENTRY* changed_sector_list, LIST_ENTRY* rollback) {
    KEY searchkey;
    traverse_ptr tp, next_tp;
    NTSTATUS Status;
    EXTENT_DATA* ed;
    BOOL b, do_cow;
    EXTENT_DATA2* eds;
    UINT64 size, new_start, new_end, last_write = 0;
    
    TRACE("(%p, (%llx, %llx), %llx, %llx, %p, %p)\n", Vcb, fcb->subvol->id, fcb->inode, start_data, length, data, changed_sector_list);
    
    searchkey.obj_id = fcb->inode;
    searchkey.obj_type = TYPE_EXTENT_DATA;
    searchkey.offset = start_data;
    
    Status = find_item(Vcb, fcb->subvol, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    if (tp.item->key.obj_id != fcb->inode || tp.item->key.obj_type != TYPE_EXTENT_DATA || tp.item->key.offset > start_data) {
        ERR("previous EXTENT_DATA not found (found %llx,%x,%llx)\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset);
        Status = STATUS_INTERNAL_ERROR;
        goto end;
    }
    
    do {
        if (tp.item->size < sizeof(EXTENT_DATA)) {
            ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_DATA));
            Status = STATUS_INTERNAL_ERROR;
            goto end;
        }
        
        ed = (EXTENT_DATA*)tp.item->data;
        
        if ((ed->type == EXTENT_TYPE_REGULAR || ed->type == EXTENT_TYPE_PREALLOC) && tp.item->size < sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2)) {
            ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2));
            Status = STATUS_INTERNAL_ERROR;
            goto end;
        }
        
        eds = (EXTENT_DATA2*)&ed->data[0];
        
        b = find_next_item(Vcb, &tp, &next_tp, TRUE);
        
        switch (ed->type) {
            case EXTENT_TYPE_REGULAR:
            {
                UINT64 rc = get_extent_item_refcount(Vcb, eds->address);
                
                if (rc == 0) {
                    ERR("get_extent_item_refcount failed\n");
                    Status = STATUS_INTERNAL_ERROR;
                    goto end;
                }
                
                do_cow = rc > 1;
                break;
            }
                
            case EXTENT_TYPE_INLINE:
                do_cow = TRUE;
                break;
                
            case EXTENT_TYPE_PREALLOC:
                FIXME("FIXME - handle prealloc extents\n"); // FIXME
                Status = STATUS_NOT_SUPPORTED;
                goto end;
                
            default:
                ERR("error - unknown extent type %x\n", ed->type);
                Status = STATUS_NOT_SUPPORTED;
                goto end;
        }
        
        if (ed->compression != BTRFS_COMPRESSION_NONE) {
            FIXME("FIXME: compression not yet supported\n");
            Status = STATUS_NOT_SUPPORTED;
            goto end;
        }
        
        if (ed->encryption != BTRFS_ENCRYPTION_NONE) {
            WARN("encryption not supported\n");
            Status = STATUS_INTERNAL_ERROR;
            goto end;
        }
        
        if (ed->encoding != BTRFS_ENCODING_NONE) {
            WARN("other encodings not supported\n");
            Status = STATUS_INTERNAL_ERROR;
            goto end;
        }
        
        size = ed->type == EXTENT_TYPE_INLINE ? ed->decoded_size : eds->num_bytes;
        
        TRACE("extent: start = %llx, length = %llx\n", tp.item->key.offset, size);
        
        new_start = tp.item->key.offset < start_data ? start_data : tp.item->key.offset;
        new_end = tp.item->key.offset + size > start_data + length ? (start_data + length) : (tp.item->key.offset + size);
        
        TRACE("new_start = %llx\n", new_start);
        TRACE("new_end = %llx\n", new_end);
        
        if (do_cow) {
            TRACE("doing COW write\n");
            
            Status = excise_extents(Vcb, fcb, new_start, new_start + new_end, changed_sector_list, rollback);
            
            if (!NT_SUCCESS(Status)) {
                ERR("error - excise_extents returned %08x\n", Status);
                goto end;
            }
            
            Status = insert_extent(Vcb, fcb, new_start, new_end - new_start, (UINT8*)data + new_start - start_data, changed_sector_list, rollback);
            
            if (!NT_SUCCESS(Status)) {
                ERR("error - insert_extent returned %08x\n", Status);
                goto end;
            }
        } else {
            UINT64 writeaddr;
            
            writeaddr = eds->address + eds->offset + new_start - tp.item->key.offset;
            TRACE("doing non-COW write to %llx\n", writeaddr);
            
            Status = write_data(Vcb, writeaddr, (UINT8*)data + new_start - start_data, new_end - new_start);
            
            if (!NT_SUCCESS(Status)) {
                ERR("error - write_data returned %08x\n", Status);
                goto end;
            }
            
            if (changed_sector_list) {
                unsigned int i;
                changed_sector* sc;
                
                sc = ExAllocatePoolWithTag(PagedPool, sizeof(changed_sector), ALLOC_TAG);
                if (!sc) {
                    ERR("out of memory\n");
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto end;
                }
                
                sc->ol.key = writeaddr;
                sc->length = (new_end - new_start) / Vcb->superblock.sector_size;
                sc->deleted = FALSE;
                
                sc->checksums = ExAllocatePoolWithTag(PagedPool, sizeof(UINT32) * sc->length, ALLOC_TAG);
                if (!sc->checksums) {
                    ERR("out of memory\n");
                    ExFreePool(sc);
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto end;
                }
                
                for (i = 0; i < sc->length; i++) {
                    sc->checksums[i] = ~calc_crc32c(0xffffffff, (UINT8*)data + new_start - start_data + (i * Vcb->superblock.sector_size), Vcb->superblock.sector_size);
                }

                insert_into_ordered_list(changed_sector_list, &sc->ol);
            }
        }
        
        last_write = new_end;
        
        if (b) {
            tp = next_tp;
            
            if (tp.item->key.obj_id != fcb->inode || tp.item->key.obj_type != TYPE_EXTENT_DATA || tp.item->key.offset >= start_data + length)
                b = FALSE;
        }
    } while (b);
    
    if (last_write < start_data + length) {
        new_start = last_write;
        new_end = start_data + length;
        
        TRACE("new_start = %llx\n", new_start);
        TRACE("new_end = %llx\n", new_end);
        
        Status = insert_extent(Vcb, fcb, new_start, new_end - new_start, (UINT8*)data + new_start - start_data, changed_sector_list, rollback);
            
        if (!NT_SUCCESS(Status)) {
            ERR("error - insert_extent returned %08x\n", Status);
            goto end;
        }
    }

    Status = STATUS_SUCCESS;
    
end:
    
    return Status;
}

#ifdef DEBUG_PARANOID
static void print_loaded_trees(tree* t, int spaces) {
    char pref[10];
    int i;
    LIST_ENTRY* le;
    
    for (i = 0; i < spaces; i++) {
        pref[i] = ' ';
    }
    pref[spaces] = 0;
    
    if (!t) {
        ERR("%s(not loaded)\n", pref);
        return;
    }
    
    le = t->itemlist.Flink;
    while (le != &t->itemlist) {
        tree_data* td = CONTAINING_RECORD(le, tree_data, list_entry);
        
        ERR("%s%llx,%x,%llx ignore=%s\n", pref, td->key.obj_id, td->key.obj_type, td->key.offset, td->ignore ? "TRUE" : "FALSE");
        
        if (t->header.level > 0) {
            print_loaded_trees(td->treeholder.tree, spaces+1);
        }
        
        le = le->Flink;
    }
}

static void check_extents_consistent(device_extension* Vcb, fcb* fcb) {
    KEY searchkey;
    traverse_ptr tp, next_tp;
    UINT64 length, oldlength, lastoff, alloc;
    NTSTATUS Status;
    EXTENT_DATA* ed;
    EXTENT_DATA2* ed2;
    
    if (fcb->ads || fcb->inode_item.st_size == 0 || fcb->deleted)
        return;
    
    TRACE("inode = %llx, subvol = %llx\n", fcb->inode, fcb->subvol->id);
    
    searchkey.obj_id = fcb->inode;
    searchkey.obj_type = TYPE_EXTENT_DATA;
    searchkey.offset = 0;
    
    Status = find_item(Vcb, fcb->subvol, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        goto failure;
    }
    
    if (keycmp(&searchkey, &tp.item->key)) {
        ERR("could not find EXTENT_DATA at offset 0\n");
        goto failure;
    }
    
    if (tp.item->size < sizeof(EXTENT_DATA)) {
        ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_DATA));
        goto failure;
    }
    
    ed = (EXTENT_DATA*)tp.item->data;
    ed2 = (EXTENT_DATA2*)&ed->data[0];
    
    length = oldlength = ed->type == EXTENT_TYPE_INLINE ? ed->decoded_size : ed2->num_bytes;
    lastoff = tp.item->key.offset;
    
    TRACE("(%llx,%x,%llx) length = %llx\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, length);
    
    alloc = 0;
    if (ed->type != EXTENT_TYPE_REGULAR || ed2->address != 0) {
        alloc += length;
    }
    
    while (find_next_item(Vcb, &tp, &next_tp, FALSE)) {
        if (next_tp.item->key.obj_id != searchkey.obj_id || next_tp.item->key.obj_type != searchkey.obj_type)
            break;
        
        tp = next_tp;
        
        if (tp.item->size < sizeof(EXTENT_DATA)) {
            ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_DATA));
            goto failure;
        }
        
        ed = (EXTENT_DATA*)tp.item->data;
        ed2 = (EXTENT_DATA2*)&ed->data[0];
    
        length = ed->type == EXTENT_TYPE_INLINE ? ed->decoded_size : ed2->num_bytes;
    
        TRACE("(%llx,%x,%llx) length = %llx\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, length);
        
        if (tp.item->key.offset != lastoff + oldlength) {
            ERR("EXTENT_DATA in %llx,%llx was at %llx, expected %llx\n", fcb->subvol->id, fcb->inode, tp.item->key.offset, lastoff + oldlength);
            goto failure;
        }
        
        if (ed->type != EXTENT_TYPE_REGULAR || ed2->address != 0) {
            alloc += length;
        }
        
        oldlength = length;
        lastoff = tp.item->key.offset;
    }
    
    if (alloc != fcb->inode_item.st_blocks) {
        ERR("allocation size was %llx, expected %llx\n", alloc, fcb->inode_item.st_blocks);
        goto failure;
    }
    
//     if (fcb->inode_item.st_blocks != lastoff + oldlength) {
//         ERR("extents finished at %x, expected %x\n", (UINT32)(lastoff + oldlength), (UINT32)fcb->inode_item.st_blocks);
//         goto failure;
//     }
    
    return;
    
failure:
    if (fcb->subvol->treeholder.tree)
        print_loaded_trees(fcb->subvol->treeholder.tree, 0);

    int3;
}

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
#endif

NTSTATUS write_file2(device_extension* Vcb, PIRP Irp, LARGE_INTEGER offset, void* buf, ULONG* length, BOOL paging_io, BOOL no_cache, LIST_ENTRY* rollback) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    KEY searchkey;
    traverse_ptr tp;
    EXTENT_DATA* ed2;
    UINT64 newlength, start_data, end_data;
    UINT32 bufhead;
    BOOL make_inline;
    UINT8* data;
    LIST_ENTRY changed_sector_list;
    INODE_ITEM *ii, *origii;
    BOOL changed_length = FALSE, nocsum, nocow/*, lazy_writer = FALSE, write_eof = FALSE*/;
    NTSTATUS Status;
    LARGE_INTEGER time;
    BTRFS_TIME now;
    fcb* fcb;
    ccb* ccb;
    file_ref* fileref;
    BOOL paging_lock = FALSE;
    
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
        ExAcquireResourceSharedLite(fcb->Header.PagingIoResource, TRUE);
        paging_lock = TRUE;
    }
    
    nocsum = fcb->ads ? TRUE : fcb->inode_item.flags & BTRFS_INODE_NODATASUM;
    nocow = fcb->ads ? TRUE : fcb->inode_item.flags & BTRFS_INODE_NODATACOW;
    
    newlength = fcb->ads ? fcb->adssize : fcb->inode_item.st_size;
    
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
    
    make_inline = fcb->ads ? FALSE : newlength <= fcb->Vcb->max_inline;
    
    if (changed_length) {
        if (newlength > fcb->Header.AllocationSize.QuadPart) {
            Status = extend_file(fcb, fileref, newlength, FALSE, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("extend_file returned %08x\n", Status);
                goto end;
            }
        } else if (fcb->ads)
            fcb->adssize = newlength;
        else
            fcb->inode_item.st_size = newlength;
        
        fcb->Header.FileSize.QuadPart = newlength;
        fcb->Header.ValidDataLength.QuadPart = newlength;
        
        TRACE("AllocationSize = %llx\n", fcb->Header.AllocationSize.QuadPart);
        TRACE("FileSize = %llx\n", fcb->Header.FileSize.QuadPart);
        TRACE("ValidDataLength = %llx\n", fcb->Header.ValidDataLength.QuadPart);
    }
    
    if (!no_cache) {
        BOOL wait;
        
        if (!FileObject->PrivateCacheMap || changed_length) {
            CC_FILE_SIZES ccfs;
            
            ccfs.AllocationSize = fcb->Header.AllocationSize;
            ccfs.FileSize = fcb->Header.FileSize;
            ccfs.ValidDataLength = fcb->Header.ValidDataLength;
            
            if (!FileObject->PrivateCacheMap) {
                TRACE("calling CcInitializeCacheMap...\n");
                CcInitializeCacheMap(FileObject, &ccfs, FALSE, cache_callbacks, FileObject);
                
                CcSetReadAheadGranularity(FileObject, READ_AHEAD_GRANULARITY);
            } else {
                CcSetFileSizes(FileObject, &ccfs);
            }
        }
        
        // FIXME - uncomment this when async is working
//             wait = IoIsOperationSynchronous(Irp) ? TRUE : FALSE;
        wait = TRUE;
        
        if (IrpSp->MinorFunction & IRP_MN_MDL) {
            CcPrepareMdlWrite(FileObject, &offset, *length, &Irp->MdlAddress, &Irp->IoStatus);

            Status = Irp->IoStatus.Status;
            goto end;
        } else {
            TRACE("CcCopyWrite(%p, %llx, %x, %u, %p)\n", FileObject, offset.QuadPart, *length, wait, buf);
            if (!CcCopyWrite(FileObject, &offset, *length, wait, buf)) {
                TRACE("CcCopyWrite failed.\n");
                
                IoMarkIrpPending(Irp);
                Status = STATUS_PENDING;
                goto end;
            }
            TRACE("CcCopyWrite finished\n");
        }
        
        Status = STATUS_SUCCESS;
        goto end;
    }
    
    if (fcb->ads) {
        UINT16 datalen;
        UINT8* data2;
        UINT32 maxlen;
        
        if (!get_xattr(fcb->Vcb, fcb->subvol, fcb->inode, fcb->adsxattr.Buffer, fcb->adshash, &data, &datalen)) {
            ERR("get_xattr failed\n");
            Status = STATUS_INTERNAL_ERROR;
            goto end;
        }
        
        if (changed_length) {
            // find maximum length of xattr
            maxlen = Vcb->superblock.node_size - sizeof(tree_header) - sizeof(leaf_node);
            
            searchkey.obj_id = fcb->inode;
            searchkey.obj_type = TYPE_XATTR_ITEM;
            searchkey.offset = fcb->adshash;

            Status = find_item(fcb->Vcb, fcb->subvol, &tp, &searchkey, FALSE);
            if (!NT_SUCCESS(Status)) {
                ERR("error - find_item returned %08x\n", Status);
                goto end;
            }
            
            if (keycmp(&tp.item->key, &searchkey)) {
                ERR("error - could not find key for xattr\n");
                Status = STATUS_INTERNAL_ERROR;
                goto end;
            }
            
            if (tp.item->size < datalen) {
                ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, datalen);
                Status = STATUS_INTERNAL_ERROR;
                goto end;
            }
            
            maxlen -= tp.item->size - datalen; // subtract XATTR_ITEM overhead
            
            if (newlength > maxlen) {
                ERR("error - xattr too long (%llu > %u)\n", newlength, maxlen);
                Status = STATUS_DISK_FULL;
                goto end;
            }
            
            fcb->adssize = newlength;

            data2 = ExAllocatePoolWithTag(PagedPool, newlength, ALLOC_TAG);
            if (!data2) {
                ERR("out of memory\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto end;
            }
            
            RtlCopyMemory(data2, data, datalen);
            
            if (offset.QuadPart > datalen)
                RtlZeroMemory(&data2[datalen], offset.QuadPart - datalen);
        } else
            data2 = data;
        
        if (*length > 0)
            RtlCopyMemory(&data2[offset.QuadPart], buf, *length);
        
        Status = set_xattr(fcb->Vcb, fcb->subvol, fcb->inode, fcb->adsxattr.Buffer, fcb->adshash, data2, newlength, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("set_xattr returned %08x\n", Status);
            goto end;
        }
        
        if (data) ExFreePool(data);
        if (data2 != data) ExFreePool(data2);
        
        fcb->Header.ValidDataLength.QuadPart = newlength;
    } else {
        if (make_inline) {
            start_data = 0;
            end_data = sector_align(newlength, fcb->Vcb->superblock.sector_size);
            bufhead = sizeof(EXTENT_DATA) - 1;
        } else {
            start_data = offset.QuadPart & ~(fcb->Vcb->superblock.sector_size - 1);
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
                    Status = read_file(Vcb, fcb->subvol, fcb->inode, data + bufhead, start_data, fcb->inode_item.st_size - start_data, NULL);
                else
                    Status = STATUS_SUCCESS;
            } else
                Status = read_file(Vcb, fcb->subvol, fcb->inode, data + bufhead, start_data, end_data - start_data, NULL);
            
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
            Status = excise_extents(fcb->Vcb, fcb, start_data, end_data, nocsum ? NULL : &changed_sector_list, rollback);
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
            
            insert_tree_item(Vcb, fcb->subvol, fcb->inode, TYPE_EXTENT_DATA, 0, ed2, sizeof(EXTENT_DATA) - 1 + newlength, NULL, rollback);
            
            fcb->inode_item.st_blocks += newlength;
        } else if (!nocow) {
            if (is_file_prealloc(fcb, start_data, end_data)) {
                Status = do_prealloc_write(fcb->Vcb, fcb, start_data, end_data, data, nocsum ? NULL : &changed_sector_list, rollback);
                
                if (!NT_SUCCESS(Status)) {
                    ERR("error - do_prealloc_write returned %08x\n", Status);
                    ExFreePool(data);
                    goto end;
                }
            } else {
                Status = do_cow_write(fcb->Vcb, fcb, start_data, end_data, data, nocsum ? NULL : &changed_sector_list, rollback);
                
                if (!NT_SUCCESS(Status)) {
                    ERR("error - do_cow_write returned %08x\n", Status);
                    ExFreePool(data);
                    goto end;
                }
            }
            
            ExFreePool(data);
        } else {
            Status = do_nocow_write(fcb->Vcb, fcb, start_data, end_data - start_data, data, nocsum ? NULL : &changed_sector_list, rollback);
            
            if (!NT_SUCCESS(Status)) {
                ERR("error - do_nocow_write returned %08x\n", Status);
                ExFreePool(data);
                goto end;
            }
            
            ExFreePool(data);
        }
    }
    
    KeQuerySystemTime(&time);
    win_time_to_unix(time, &now);
    
//     ERR("no_cache = %s, FileObject->PrivateCacheMap = %p\n", no_cache ? "TRUE" : "FALSE", FileObject->PrivateCacheMap);
    
//     if (!no_cache) {
//         if (!FileObject->PrivateCacheMap) {
//             CC_FILE_SIZES ccfs;
//             
//             ccfs.AllocationSize = fcb->Header.AllocationSize;
//             ccfs.FileSize = fcb->Header.FileSize;
//             ccfs.ValidDataLength = fcb->Header.ValidDataLength;
//             
//             TRACE("calling CcInitializeCacheMap...\n");
//             CcInitializeCacheMap(FileObject, &ccfs, FALSE, cache_callbacks, fcb);
//             
//             changed_length = FALSE;
//         }
//     }
    
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
        TRACE("setting st_size to %llx\n", newlength);
        origii->st_size = newlength;
        origii->st_mtime = now;
    }
    
    searchkey.obj_id = fcb->inode;
    searchkey.obj_type = TYPE_INODE_ITEM;
    searchkey.offset = 0;
    
    Status = find_item(fcb->Vcb, fcb->subvol, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        goto end;
    }
    
    if (!keycmp(&tp.item->key, &searchkey))
        delete_tree_item(Vcb, &tp, rollback);
    else
        WARN("couldn't find existing INODE_ITEM\n");

    ii = ExAllocatePoolWithTag(PagedPool, sizeof(INODE_ITEM), ALLOC_TAG);
    if (!ii) {
        ERR("out of memory\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }
    
    RtlCopyMemory(ii, origii, sizeof(INODE_ITEM));
    insert_tree_item(Vcb, fcb->subvol, fcb->inode, TYPE_INODE_ITEM, 0, ii, sizeof(INODE_ITEM), NULL, rollback);
    
    // FIXME - update inode_item of open FCBs pointing to the same inode (i.e. hardlinked files)
    
    if (!nocsum)
        update_checksum_tree(Vcb, &changed_sector_list, rollback);
    
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
    
end:
    if (FileObject->Flags & FO_SYNCHRONOUS_IO && !paging_io) {
        TRACE("CurrentByteOffset was: %llx\n", FileObject->CurrentByteOffset.QuadPart);
        FileObject->CurrentByteOffset.QuadPart = offset.QuadPart + (NT_SUCCESS(Status) ? *length : 0);
        TRACE("CurrentByteOffset now: %llx\n", FileObject->CurrentByteOffset.QuadPart);
    }
    
    if (paging_lock)
        ExReleaseResourceLite(fcb->Header.PagingIoResource);

    return Status;
}

NTSTATUS write_file(PDEVICE_OBJECT DeviceObject, PIRP Irp) {
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    device_extension* Vcb = DeviceObject->DeviceExtension;
    void* buf;
    NTSTATUS Status;
    LARGE_INTEGER offset = IrpSp->Parameters.Write.ByteOffset;
    PFILE_OBJECT FileObject = IrpSp->FileObject;
    fcb* fcb = FileObject ? FileObject->FsContext : NULL;
    BOOL locked = FALSE;
//     LARGE_INTEGER freq, time1, time2;
    LIST_ENTRY rollback;
    
    InitializeListHead(&rollback);
    
    if (Vcb->readonly)
        return STATUS_MEDIA_WRITE_PROTECTED;
    
    if (fcb && fcb->subvol->root_item.flags & BTRFS_SUBVOL_READONLY)
        return STATUS_ACCESS_DENIED;
    
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
    
    if (Irp->Flags & IRP_NOCACHE) {
        acquire_tree_lock(Vcb, TRUE);
        locked = TRUE;
    }
    
    if (fcb && !(Irp->Flags & IRP_PAGING_IO) && !FsRtlCheckLockForWriteAccess(&fcb->lock, Irp)) {
        WARN("tried to write to locked region\n");
        Status = STATUS_FILE_LOCK_CONFLICT;
        goto exit;
    }
    
//     ERR("Irp->Flags = %x\n", Irp->Flags);
    Status = write_file2(Vcb, Irp, offset, buf, &IrpSp->Parameters.Write.Length, Irp->Flags & IRP_PAGING_IO, Irp->Flags & IRP_NOCACHE, &rollback);
    if (!NT_SUCCESS(Status)) {
        if (Status != STATUS_PENDING)
            ERR("write_file2 returned %08x\n", Status);
        goto exit;
    }
    
    if (locked)
        Status = consider_write(Vcb);

    if (NT_SUCCESS(Status)) {
        Irp->IoStatus.Information = IrpSp->Parameters.Write.Length;
    
#ifdef DEBUG_PARANOID
        if (locked)
            check_extents_consistent(Vcb, FileObject->FsContext); // TESTING
    
//         check_extent_tree_consistent(Vcb);
#endif
    }
    
exit:
    if (locked) {
        if (NT_SUCCESS(Status))
            clear_rollback(&rollback);
        else
            do_rollback(Vcb, &rollback);
        
        release_tree_lock(Vcb, TRUE);
    }
    
//     time2 = KeQueryPerformanceCounter(NULL);
    
//     ERR("time = %u (freq = %u)\n", (UINT32)(time2.QuadPart - time1.QuadPart), (UINT32)freq.QuadPart);
    
    return Status;
}
