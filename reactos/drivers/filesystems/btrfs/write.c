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

typedef struct {
    LIST_ENTRY list_entry;
    UINT64 key;
} ordered_list;

typedef struct {
    ordered_list ol;
    ULONG length;
    UINT32* checksums;
    BOOL deleted;
} changed_sector;

static NTSTATUS convert_old_data_extent(device_extension* Vcb, UINT64 address, UINT64 size, LIST_ENTRY* rollback);
static BOOL extent_item_is_shared(EXTENT_ITEM* ei, ULONG len);
static NTSTATUS convert_shared_data_extent(device_extension* Vcb, UINT64 address, UINT64 size, LIST_ENTRY* rollback);

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
            RemoveEntryList(&s->list_entry);
            
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
            s->size -= s->offset - offset + size;
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
                
                if (tp.item->key.offset >= lastaddr + size) {
                    free_traverse_ptr(&tp);
                    return lastaddr;
                }
                
                lastaddr = tp.item->key.offset + ci->size;
            }
        }
        
        b = find_next_item(Vcb, &tp, &next_tp, FALSE);
        if (b) {
            free_traverse_ptr(&tp);
            tp = next_tp;
            
            if (tp.item->key.obj_id > searchkey.obj_id || tp.item->key.obj_type > searchkey.obj_type)
                break;
        }
    } while (b);
    
    free_traverse_ptr(&tp);
    
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
        free_traverse_ptr(&tp);
        return FALSE;
    }
    
    delete_tree_item(Vcb, &tp, rollback);
    
    free_traverse_ptr(&tp);
    
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

static chunk* alloc_chunk(device_extension* Vcb, UINT64 flags, LIST_ENTRY* rollback) {
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

static void decrease_chunk_usage(chunk* c, UINT64 delta) {
    c->used -= delta;
    
    TRACE("decreasing size of chunk %llx by %llx\n", c->offset, delta);
}

static void increase_chunk_usage(chunk* c, UINT64 delta) {
    c->used += delta;
    
    TRACE("increasing size of chunk %llx by %llx\n", c->offset, delta);
}

static NTSTATUS STDCALL write_data(device_extension* Vcb, UINT64 address, void* data, UINT32 length) {
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
    free_traverse_ptr(&tp);
    
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

static BOOL trees_consistent(device_extension* Vcb) {
    ULONG maxsize = Vcb->superblock.node_size - sizeof(tree_header);
    LIST_ENTRY* le;
    
    le = Vcb->tree_cache.Flink;
    while (le != &Vcb->tree_cache) {
        tree_cache* tc2 = CONTAINING_RECORD(le, tree_cache, list_entry);
        
        if (tc2->write) {
            if (tc2->tree->header.num_items == 0)
                return FALSE;
            
            if (tc2->tree->size > maxsize)
                return FALSE;
            
            if (tc2->tree->new_address == 0)
                return FALSE;
        }
        
        le = le->Flink;
    }
    
    return TRUE;
}

static NTSTATUS add_parents(device_extension* Vcb, LIST_ENTRY* rollback) {
    LIST_ENTRY* le;
    NTSTATUS Status;
    
    le = Vcb->tree_cache.Flink;
    while (le != &Vcb->tree_cache) {
        tree_cache* tc2 = CONTAINING_RECORD(le, tree_cache, list_entry);
        
        if (tc2->write) {
            if (tc2->tree->parent)
                add_to_tree_cache(Vcb, tc2->tree->parent, TRUE);
            else if (tc2->tree->root != Vcb->chunk_root && tc2->tree->root != Vcb->root_root) {
                KEY searchkey;
                traverse_ptr tp;
                
                searchkey.obj_id = tc2->tree->root->id;
                searchkey.obj_type = TYPE_ROOT_ITEM;
                searchkey.offset = 0xffffffffffffffff;
                
                Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE);
                if (!NT_SUCCESS(Status)) {
                    ERR("error - find_item returned %08x\n", Status);
                    return Status;
                }
                
                if (tp.item->key.obj_id != searchkey.obj_id || tp.item->key.obj_type != searchkey.obj_type) {
                    ERR("could not find ROOT_ITEM for tree %llx\n", searchkey.obj_id);
                    free_traverse_ptr(&tp);
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
                    
                    if (!insert_tree_item(Vcb, Vcb->root_root, searchkey.obj_id, searchkey.obj_type, 0, ri, sizeof(ROOT_ITEM), NULL, rollback)) {
                        ERR("insert_tree_item failed\n");
                        return STATUS_INTERNAL_ERROR;
                    }
                } else {
                    add_to_tree_cache(Vcb, tp.tree, TRUE);
                }
                
                free_traverse_ptr(&tp);
            }
        }
        
        le = le->Flink;
    }

    return STATUS_SUCCESS;
}

void print_trees(LIST_ENTRY* tc) {
    LIST_ENTRY *le, *le2;
    
    le = tc->Flink;
    while (le != tc) {
        KEY firstitem = {0xcccccccccccccccc,0xcc,0xcccccccccccccccc};
        tree_cache* tc2 = CONTAINING_RECORD(le, tree_cache, list_entry);
        UINT32 num_items = 0;
        
        le2 = tc2->tree->itemlist.Flink;
        while (le2 != &tc2->tree->itemlist) {
            tree_data* td = CONTAINING_RECORD(le2, tree_data, list_entry);
            if (!td->ignore) {
                firstitem = td->key;
                num_items++;
            }
            le2 = le2->Flink;
        }
        
        ERR("tree: root %llx, first key %llx,%x,%llx, level %x, num_items %x / %x\n",
            tc2->tree->header.tree_id, firstitem.obj_id, firstitem.obj_type, firstitem.offset, tc2->tree->header.level, num_items, tc2->tree->header.num_items);
        
        le = le->Flink;
    }
}

static void add_parents_to_cache(device_extension* Vcb, tree* t) {
    KEY searchkey;
    traverse_ptr tp;
    NTSTATUS Status;
    
    while (t->parent) {
        t = t->parent;
        
        add_to_tree_cache(Vcb, t, TRUE);
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
        free_traverse_ptr(&tp);
        return;
    }
    
    add_to_tree_cache(Vcb, tp.tree, TRUE);
    
    free_traverse_ptr(&tp);
}

static BOOL insert_tree_extent_skinny(device_extension* Vcb, tree* t, chunk* c, UINT64 address, LIST_ENTRY* rollback) {
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
    eism->tbr.offset = t->header.tree_id;
    
    if (!insert_tree_item(Vcb, Vcb->extent_root, address, TYPE_METADATA_ITEM, t->header.level, eism, sizeof(EXTENT_ITEM_SKINNY_METADATA), &insert_tp, rollback)) {
        ERR("insert_tree_item failed\n");
        ExFreePool(eism);
        return FALSE;
    }
    
    add_to_space_list(c, address, Vcb->superblock.node_size, SPACE_TYPE_WRITING);

//     add_to_tree_cache(tc, insert_tp.tree, TRUE);
    add_parents_to_cache(Vcb, insert_tp.tree);
    
    free_traverse_ptr(&insert_tp);
    
    t->new_address = address;
    
    return TRUE;
}

static BOOL insert_tree_extent(device_extension* Vcb, tree* t, chunk* c, LIST_ENTRY* rollback) {
    UINT64 address;
    EXTENT_ITEM_TREE2* eit2;
    traverse_ptr insert_tp;
    
    TRACE("(%p, %p, %p, %p)\n", Vcb, t, c);
    
    if (!find_address_in_chunk(Vcb, c, Vcb->superblock.node_size, &address))
        return FALSE;
    
    if (Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA)
        return insert_tree_extent_skinny(Vcb, t, c, address, rollback);
    
    eit2 = ExAllocatePoolWithTag(PagedPool, sizeof(EXTENT_ITEM_TREE2), ALLOC_TAG);
    if (!eit2) {
        ERR("out of memory\n");
        return FALSE;
    }

    eit2->eit.extent_item.refcount = 1;
    eit2->eit.extent_item.generation = Vcb->superblock.generation;
    eit2->eit.extent_item.flags = EXTENT_ITEM_TREE_BLOCK;
//     eit2->eit.firstitem = wt->firstitem;
    eit2->eit.level = t->header.level;
    eit2->type = TYPE_TREE_BLOCK_REF;
    eit2->tbr.offset = t->header.tree_id;
    
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

//     add_to_tree_cache(tc, insert_tp.tree, TRUE);
    add_parents_to_cache(Vcb, insert_tp.tree);
    
    free_traverse_ptr(&insert_tp);
    
    t->new_address = address;
    
    return TRUE;
}

static NTSTATUS get_tree_new_address(device_extension* Vcb, tree* t, LIST_ENTRY* rollback) {
    chunk *origchunk = NULL, *c;
    LIST_ENTRY* le;
    UINT64 flags = t->flags;
    
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
    
    if (t->header.address != 0) {
        origchunk = get_chunk_from_address(Vcb, t->header.address);
        
        if (insert_tree_extent(Vcb, t, origchunk, rollback))
            return STATUS_SUCCESS;
    }
    
    le = Vcb->chunks.Flink;
    while (le != &Vcb->chunks) {
        c = CONTAINING_RECORD(le, chunk, list_entry);
        
        // FIXME - make sure to avoid superblocks
        
        if (c != origchunk && c->chunk_item->type == flags && (c->chunk_item->size - c->used) >= Vcb->superblock.node_size) {
            if (insert_tree_extent(Vcb, t, c, rollback))
                return STATUS_SUCCESS;
        }

        le = le->Flink;
    }
    
    // allocate new chunk if necessary
    if ((c = alloc_chunk(Vcb, flags, rollback))) {
        if ((c->chunk_item->size - c->used) >= Vcb->superblock.node_size) {
            if (insert_tree_extent(Vcb, t, c, rollback))
                return STATUS_SUCCESS;
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
    searchkey.offset = t->header.level;
    
    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return FALSE;
    }
    
    if (keycmp(&tp.item->key, &searchkey)) {
        TRACE("could not find %llx,%x,%llx in extent_root\n", searchkey.obj_id, searchkey.obj_type, searchkey.offset);
        free_traverse_ptr(&tp);
        return FALSE;
    }
    
    if (tp.item->size < sizeof(EXTENT_ITEM_SKINNY_METADATA)) {
        ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_ITEM_SKINNY_METADATA));
        free_traverse_ptr(&tp);
        return FALSE;
    }
    
    delete_tree_item(Vcb, &tp, rollback);
    
    eism = (EXTENT_ITEM_SKINNY_METADATA*)tp.item->data;
    if (t->header.level == 0 && eism->ei.flags & EXTENT_ITEM_SHARED_BACKREFS && eism->type == TYPE_TREE_BLOCK_REF) {
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
    
    free_traverse_ptr(&tp);
    
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
        free_traverse_ptr(&tp);
        return;
    }
    
    searchkey.obj_id = td->treeholder.address;
    searchkey.obj_type = TYPE_EXTENT_ITEM;
    searchkey.offset = Vcb->superblock.node_size;
    
    Status = find_item(Vcb, Vcb->extent_root, &tp2, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        free_traverse_ptr(&tp);
        return;
    }
    
    if (keycmp(&searchkey, &tp2.item->key)) {
        ERR("could not find %llx,%x,%llx\n", searchkey.obj_id, searchkey.obj_type, searchkey.offset);
        free_traverse_ptr(&tp2);
        free_traverse_ptr(&tp);
        return;
    }
    
    if (tp.item->size < sizeof(EXTENT_REF_V0)) {
        ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_REF_V0));
        free_traverse_ptr(&tp2);
        free_traverse_ptr(&tp);
        return;
    }
    
    erv0 = (EXTENT_REF_V0*)tp.item->data;
    
    delete_tree_item(Vcb, &tp, rollback);
    delete_tree_item(Vcb, &tp2, rollback);
    
    if (Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA) {
        EXTENT_ITEM_SKINNY_METADATA* eism = ExAllocatePoolWithTag(PagedPool, sizeof(EXTENT_ITEM_SKINNY_METADATA), ALLOC_TAG);
        
        if (!eism) {
            ERR("out of memory\n");
            free_traverse_ptr(&tp2);
            free_traverse_ptr(&tp);
            return;
        }
        
        eism->ei.refcount = 1;
        eism->ei.generation = erv0->gen;
        eism->ei.flags = EXTENT_ITEM_TREE_BLOCK;
        eism->type = TYPE_TREE_BLOCK_REF;
        eism->tbr.offset = t->header.tree_id;
        
        if (!insert_tree_item(Vcb, Vcb->extent_root, td->treeholder.address, TYPE_METADATA_ITEM, t->header.level -1, eism, sizeof(EXTENT_ITEM_SKINNY_METADATA), &insert_tp, rollback)) {
            ERR("insert_tree_item failed\n");
            free_traverse_ptr(&tp2);
            free_traverse_ptr(&tp);
            return;
        }
    } else {
        EXTENT_ITEM_TREE2* eit2 = ExAllocatePoolWithTag(PagedPool, sizeof(EXTENT_ITEM_TREE2), ALLOC_TAG);
        
        if (!eit2) {
            ERR("out of memory\n");
            free_traverse_ptr(&tp2);
            free_traverse_ptr(&tp);
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
            free_traverse_ptr(&tp2);
            free_traverse_ptr(&tp);
            return;
        }
    }
    
//     add_to_tree_cache(tc, insert_tp.tree, TRUE);
    add_parents_to_cache(Vcb, insert_tp.tree);
    add_parents_to_cache(Vcb, tp.tree);
    add_parents_to_cache(Vcb, tp2.tree);
    
    free_traverse_ptr(&insert_tp);
    
    free_traverse_ptr(&tp2);
    free_traverse_ptr(&tp);
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
        free_traverse_ptr(&tp);
        return STATUS_INTERNAL_ERROR;
    }
    
    if (tp.item->size == sizeof(EXTENT_ITEM_V0)) {
        eiv0 = (EXTENT_ITEM_V0*)tp.item->data;
        
        if (eiv0->refcount > 1) {
            FIXME("FIXME - cannot deal with refcounts larger than 1 at present (eiv0->refcount == %llx)\n", eiv0->refcount);
            free_traverse_ptr(&tp);
            return STATUS_INTERNAL_ERROR;
        }
    } else {
        if (tp.item->size < sizeof(EXTENT_ITEM)) {
            ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_ITEM));
            free_traverse_ptr(&tp);
            return STATUS_INTERNAL_ERROR;
        }
        
        ei = (EXTENT_ITEM*)tp.item->data;
        
        if (ei->refcount > 1) {
            FIXME("FIXME - cannot deal with refcounts larger than 1 at present (ei->refcount == %llx)\n", ei->refcount);
            free_traverse_ptr(&tp);
            return STATUS_INTERNAL_ERROR;
        }
        
        if (t->header.level == 0 && ei->flags & EXTENT_ITEM_SHARED_BACKREFS) {
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
            free_traverse_ptr(&tp);
            return Status;
        }
        
        if (tp2.item->key.obj_id == searchkey.obj_id && tp2.item->key.obj_type == searchkey.obj_type) {
            delete_tree_item(Vcb, &tp2, rollback);
        }
        free_traverse_ptr(&tp2);
    }
     
    if (!(t->header.flags & HEADER_FLAG_MIXED_BACKREF)) {
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
    
    free_traverse_ptr(&tp);
    
    return STATUS_SUCCESS;
}

static NTSTATUS allocate_tree_extents(device_extension* Vcb, LIST_ENTRY* rollback) {
    LIST_ENTRY* le;
    NTSTATUS Status;
    
    TRACE("(%p)\n", Vcb);
    
    le = Vcb->tree_cache.Flink;
    while (le != &Vcb->tree_cache) {
        tree_cache* tc2 = CONTAINING_RECORD(le, tree_cache, list_entry);
        
        if (tc2->write && tc2->tree->new_address == 0) {
            chunk* c;
            
            Status = get_tree_new_address(Vcb, tc2->tree, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("get_tree_new_address returned %08x\n", Status);
                return Status;
            }
            
            TRACE("allocated extent %llx\n", tc2->tree->new_address);
            
            if (tc2->tree->header.address != 0) {
                Status = reduce_tree_extent(Vcb, tc2->tree->header.address, tc2->tree, rollback);
                
                if (!NT_SUCCESS(Status)) {
                    ERR("reduce_tree_extent returned %08x\n", Status);
                    return Status;
                }
            }

            c = get_chunk_from_address(Vcb, tc2->tree->new_address);
            
            if (c) {
                increase_chunk_usage(c, Vcb->superblock.node_size);
            } else {
                ERR("could not find chunk for address %llx\n", tc2->tree->new_address);
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
    
    le = Vcb->tree_cache.Flink;
    while (le != &Vcb->tree_cache) {
        tree_cache* tc2 = CONTAINING_RECORD(le, tree_cache, list_entry);
        
        if (tc2->write && !tc2->tree->parent) {
            if (tc2->tree->root != Vcb->root_root && tc2->tree->root != Vcb->chunk_root) {
                KEY searchkey;
                traverse_ptr tp;
                
                searchkey.obj_id = tc2->tree->root->id;
                searchkey.obj_type = TYPE_ROOT_ITEM;
                searchkey.offset = 0xffffffffffffffff;
                
                Status = find_item(Vcb, Vcb->root_root, &tp, &searchkey, FALSE);
                if (!NT_SUCCESS(Status)) {
                    ERR("error - find_item returned %08x\n", Status);
                    return Status;
                }
                
                if (tp.item->key.obj_id != searchkey.obj_id || tp.item->key.obj_type != searchkey.obj_type) {
                    ERR("could not find ROOT_ITEM for tree %llx\n", searchkey.obj_id);
                    free_traverse_ptr(&tp);
                    return STATUS_INTERNAL_ERROR;
                }
                
                TRACE("updating the address for root %llx to %llx\n", searchkey.obj_id, tc2->tree->new_address);
                
                tc2->tree->root->root_item.block_number = tc2->tree->new_address;
                tc2->tree->root->root_item.root_level = tc2->tree->header.level;
                tc2->tree->root->root_item.generation = Vcb->superblock.generation;
                tc2->tree->root->root_item.generation2 = Vcb->superblock.generation;
                
                if (tp.item->size < sizeof(ROOT_ITEM)) { // if not full length, delete and create new entry
                    ROOT_ITEM* ri = ExAllocatePoolWithTag(PagedPool, sizeof(ROOT_ITEM), ALLOC_TAG);
                    
                    if (!ri) {
                        ERR("out of memory\n");
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }
                    
                    RtlCopyMemory(ri, &tc2->tree->root->root_item, sizeof(ROOT_ITEM));
                    
                    delete_tree_item(Vcb, &tp, rollback);
                    
                    if (!insert_tree_item(Vcb, Vcb->root_root, searchkey.obj_id, searchkey.obj_type, 0, ri, sizeof(ROOT_ITEM), NULL, rollback)) {
                        ERR("insert_tree_item failed\n");
                        return STATUS_INTERNAL_ERROR;
                    }
                } else
                    RtlCopyMemory(tp.item->data, &tc2->tree->root->root_item, sizeof(ROOT_ITEM));
                
                free_traverse_ptr(&tp);
            }
            
            tc2->tree->root->treeholder.address = tc2->tree->new_address;
        }
        
        le = le->Flink;
    }
    
    return STATUS_SUCCESS;
}

enum write_tree_status {
    WriteTreeStatus_Pending,
    WriteTreeStatus_Success,
    WriteTreeStatus_Error,
    WriteTreeStatus_Cancelling,
    WriteTreeStatus_Cancelled
};

struct write_tree_context;

typedef struct {
    struct write_tree_context* context;
    UINT8* buf;
    device* device;
    PIRP Irp;
    IO_STATUS_BLOCK iosb;
    enum write_tree_status status;
    LIST_ENTRY list_entry;
} write_tree_stripe;

typedef struct {
    KEVENT Event;
    LIST_ENTRY stripes;
} write_tree_context;

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

static NTSTATUS write_tree(device_extension* Vcb, UINT64 addr, UINT8* data, write_tree_context* wtc) {
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

static void free_stripes(write_tree_context* wtc) {
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
        
        le = Vcb->tree_cache.Flink;
        while (le != &Vcb->tree_cache) {
            tree_cache* tc2 = CONTAINING_RECORD(le, tree_cache, list_entry);
            
            if (tc2->write && tc2->tree->header.level == level) {
                KEY firstitem, searchkey;
                LIST_ENTRY* le2;
                traverse_ptr tp;
                EXTENT_ITEM_TREE* eit;
                
                if (tc2->tree->new_address == 0) {
                    ERR("error - tried to write tree with no new address\n");
                    int3;
                }
                
                le2 = tc2->tree->itemlist.Flink;
                while (le2 != &tc2->tree->itemlist) {
                    tree_data* td = CONTAINING_RECORD(le2, tree_data, list_entry);
                    if (!td->ignore) {
                        firstitem = td->key;
                        break;
                    }
                    le2 = le2->Flink;
                }
                
                if (tc2->tree->parent) {
                    tc2->tree->paritem->key = firstitem;
                    tc2->tree->paritem->treeholder.address = tc2->tree->new_address;
                    tc2->tree->paritem->treeholder.generation = Vcb->superblock.generation;
                }
                
                if (!(Vcb->superblock.incompat_flags & BTRFS_INCOMPAT_FLAGS_SKINNY_METADATA)) {
                    searchkey.obj_id = tc2->tree->new_address;
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
                        free_traverse_ptr(&tp);
                        
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
                        free_traverse_ptr(&tp);
                        return STATUS_INTERNAL_ERROR;
                    }
                    
                    eit = (EXTENT_ITEM_TREE*)tp.item->data;
                    eit->firstitem = firstitem;
                    
                    free_traverse_ptr(&tp);
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
    
    le = Vcb->tree_cache.Flink;
    while (le != &Vcb->tree_cache) {
        tree_cache* tc2 = CONTAINING_RECORD(le, tree_cache, list_entry);
#ifdef DEBUG_PARANOID
        UINT32 num_items = 0, size = 0;
        LIST_ENTRY* le2;
        BOOL crash = FALSE;
#endif

        if (tc2->write) {
#ifdef DEBUG_PARANOID
            le2 = tc2->tree->itemlist.Flink;
            while (le2 != &tc2->tree->itemlist) {
                tree_data* td = CONTAINING_RECORD(le2, tree_data, list_entry);
                if (!td->ignore) {
                    num_items++;
                    
                    if (tc2->tree->header.level == 0)
                        size += td->size;
                }
                le2 = le2->Flink;
            }
            
            if (tc2->tree->header.level == 0)
                size += num_items * sizeof(leaf_node);
            else
                size += num_items * sizeof(internal_node);
            
            if (num_items != tc2->tree->header.num_items) {
                ERR("tree %llx, level %x: num_items was %x, expected %x\n", tc2->tree->root->id, tc2->tree->header.level, num_items, tc2->tree->header.num_items);
                crash = TRUE;
            }
            
            if (size != tc2->tree->size) {
                ERR("tree %llx, level %x: size was %x, expected %x\n", tc2->tree->root->id, tc2->tree->header.level, size, tc2->tree->size);
                crash = TRUE;
            }
            
            if (tc2->tree->new_address == 0) {
                ERR("tree %llx, level %x: tried to write tree to address 0\n", tc2->tree->root->id, tc2->tree->header.level);
                crash = TRUE;
            }
            
            if (tc2->tree->header.num_items == 0) {
                ERR("tree %llx, level %x: tried to write empty tree\n", tc2->tree->root->id, tc2->tree->header.level);
                crash = TRUE;
            }
            
            if (tc2->tree->size > Vcb->superblock.node_size - sizeof(tree_header)) {
                ERR("tree %llx, level %x: tried to write overlarge tree (%x > %x)\n", tc2->tree->root->id, tc2->tree->header.level, tc2->tree->size, Vcb->superblock.node_size - sizeof(tree_header));
                crash = TRUE;
            }
            
            if (crash) {
                ERR("tree %p\n", tc2->tree);
                le2 = tc2->tree->itemlist.Flink;
                while (le2 != &tc2->tree->itemlist) {
                    tree_data* td = CONTAINING_RECORD(le2, tree_data, list_entry);
                    if (!td->ignore) {
                        ERR("%llx,%x,%llx inserted=%u\n", td->key.obj_id, td->key.obj_type, td->key.offset, td->inserted);
                    }
                    le2 = le2->Flink;
                }
                int3;
            }
#endif
            tc2->tree->header.address = tc2->tree->new_address;
            tc2->tree->header.generation = Vcb->superblock.generation;
            tc2->tree->header.flags |= HEADER_FLAG_MIXED_BACKREF;
            
            data = ExAllocatePoolWithTag(NonPagedPool, Vcb->superblock.node_size, ALLOC_TAG);
            if (!data) {
                ERR("out of memory\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto end;
            }
            
            body = data + sizeof(tree_header);
            
            RtlCopyMemory(data, &tc2->tree->header, sizeof(tree_header));
            RtlZeroMemory(body, Vcb->superblock.node_size - sizeof(tree_header));
            
            if (tc2->tree->header.level == 0) {
                leaf_node* itemptr = (leaf_node*)body;
                int i = 0;
                LIST_ENTRY* le2;
                UINT8* dataptr = data + Vcb->superblock.node_size;
                
                le2 = tc2->tree->itemlist.Flink;
                while (le2 != &tc2->tree->itemlist) {
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
                
                le2 = tc2->tree->itemlist.Flink;
                while (le2 != &tc2->tree->itemlist) {
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
            
            Status = write_tree(Vcb, tc2->tree->new_address, data, wtc);
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
        
        free_stripes(wtc);
    }
    
end:
    ExFreePool(wtc);
    
    return Status;
}

static NTSTATUS write_superblocks(device_extension* Vcb) {
    UINT64 i;
    NTSTATUS Status;
    LIST_ENTRY* le;
    
    TRACE("(%p)\n", Vcb);
    
    le = Vcb->tree_cache.Flink;
    while (le != &Vcb->tree_cache) {
        tree_cache* tc2 = CONTAINING_RECORD(le, tree_cache, list_entry);
        
        if (tc2->write && !tc2->tree->parent) {
            if (tc2->tree->root == Vcb->root_root) {
                Vcb->superblock.root_tree_addr = tc2->tree->new_address;
                Vcb->superblock.root_level = tc2->tree->header.level;
            } else if (tc2->tree->root == Vcb->chunk_root) {
                Vcb->superblock.chunk_tree_addr = tc2->tree->new_address;
                Vcb->superblock.chunk_root_generation = tc2->tree->header.generation;
                Vcb->superblock.chunk_root_level = tc2->tree->header.level;
            }
        }
        
        le = le->Flink;
    }
    
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
                free_traverse_ptr(&tp);
                return STATUS_INTERNAL_ERROR;
            }
            
            if (tp.item->size < sizeof(BLOCK_GROUP_ITEM)) {
                ERR("(%llx,%x,%llx) was %u bytes, expected %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(BLOCK_GROUP_ITEM));
                free_traverse_ptr(&tp);
                return STATUS_INTERNAL_ERROR;
            }
            
            bgi = ExAllocatePoolWithTag(PagedPool, tp.item->size, ALLOC_TAG);
            if (!bgi) {
                ERR("out of memory\n");
                free_traverse_ptr(&tp);
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
                free_traverse_ptr(&tp);
                return STATUS_INTERNAL_ERROR;
            } else if (c->chunk_item->type & BLOCK_FLAG_RAID1) {
                FIXME("RAID1 not yet supported\n");
                ExFreePool(bgi);
                free_traverse_ptr(&tp);
                return STATUS_INTERNAL_ERROR;
            } else if (c->chunk_item->type & BLOCK_FLAG_DUPLICATE) {
                Vcb->superblock.bytes_used = Vcb->superblock.bytes_used + (2 * (c->used - c->oldused));
            } else if (c->chunk_item->type & BLOCK_FLAG_RAID10) {
                FIXME("RAID10 not yet supported\n");
                ExFreePool(bgi);
                free_traverse_ptr(&tp);
                return STATUS_INTERNAL_ERROR;
            } else if (c->chunk_item->type & BLOCK_FLAG_RAID5) {
                FIXME("RAID5 not yet supported\n");
                ExFreePool(bgi);
                free_traverse_ptr(&tp);
                return STATUS_INTERNAL_ERROR;
            } else if (c->chunk_item->type & BLOCK_FLAG_RAID6) {
                FIXME("RAID6 not yet supported\n");
                ExFreePool(bgi);
                free_traverse_ptr(&tp);
                return STATUS_INTERNAL_ERROR;
            } else { // SINGLE
                Vcb->superblock.bytes_used = Vcb->superblock.bytes_used + c->used - c->oldused;
            }
            
            TRACE("bytes_used = %llx\n", Vcb->superblock.bytes_used);

            free_traverse_ptr(&tp);
            
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
    
    nt->refcount = 0;
    nt->Vcb = Vcb;
    nt->parent = t->parent;
    nt->root = t->root;
//     nt->nonpaged = ExAllocatePoolWithTag(NonPagedPool, sizeof(tree_nonpaged), ALLOC_TAG);
    nt->new_address = 0;
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
    add_to_tree_cache(Vcb, nt, TRUE);
    
    InterlockedIncrement(&Vcb->open_trees);
#ifdef DEBUG_TREE_REFCOUNTS
    TRACE("created new split tree %p\n", nt);
#endif
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
    
    if (nt->parent) {
        increase_tree_rc(nt->parent);
        
        td = ExAllocatePoolWithTag(PagedPool, sizeof(tree_data), ALLOC_TAG);
        if (!td) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    
        td->key = newfirstitem->key;
        
        InsertAfter(&t->itemlist, &td->list_entry, &t->paritem->list_entry);
        
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
    
    pt->refcount = 2;
    pt->Vcb = Vcb;
    pt->parent = NULL;
    pt->paritem = NULL;
    pt->root = t->root;
    pt->new_address = 0;
//     pt->nonpaged = ExAllocatePoolWithTag(NonPagedPool, sizeof(tree_nonpaged), ALLOC_TAG);
    pt->size = pt->header.num_items * sizeof(internal_node);
    pt->flags = t->flags;
    InitializeListHead(&pt->itemlist);
    
//     ExInitializeResourceLite(&pt->nonpaged->load_tree_lock);
    
    InterlockedIncrement(&Vcb->open_trees);
#ifdef DEBUG_TREE_REFCOUNTS
    TRACE("created new parent tree %p\n", pt);
#endif
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
    
    add_to_tree_cache(Vcb, pt, TRUE);

    t->root->treeholder.tree = pt;
    
    t->parent = pt;
    nt->parent = pt;
    
end:

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
    
    if (loaded)
        increase_tree_rc(t->parent);
        
//     ExReleaseResourceLite(&t->parent->nonpaged->load_tree_lock);
    
    next_tree = nextparitem->treeholder.tree;
    
    if (t->size + next_tree->size <= Vcb->superblock.node_size - sizeof(tree_header)) {
        // merge two trees into one
        
        t->header.num_items += next_tree->header.num_items;
        t->size += next_tree->size;
        
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
        
        if (next_tree->new_address != 0) { // delete associated EXTENT_ITEM
            Status = reduce_tree_extent(Vcb, next_tree->new_address, next_tree, rollback);
            
            if (!NT_SUCCESS(Status)) {
                ERR("reduce_tree_extent returned %08x\n", Status);
                free_tree(next_tree);
                return Status;
            }
        } else if (next_tree->header.address != 0) {
            Status = reduce_tree_extent(Vcb, next_tree->header.address, next_tree, rollback);
            
            if (!NT_SUCCESS(Status)) {
                ERR("reduce_tree_extent returned %08x\n", Status);
                free_tree(next_tree);
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
            add_to_tree_cache(Vcb, par, TRUE);
            par = par->parent;
        }
        
        RemoveEntryList(&nextparitem->list_entry);
        ExFreePool(next_tree->paritem);
        next_tree->paritem = NULL;
        
        free_tree(next_tree);
        
        // remove next_tree from tree cache
        le = Vcb->tree_cache.Flink;
        while (le != &Vcb->tree_cache) {
            tree_cache* tc2 = CONTAINING_RECORD(le, tree_cache, list_entry);
            
            if (tc2->tree == next_tree) {
                free_tree(next_tree);
                RemoveEntryList(le);
                ExFreePool(tc2);
                break;
            }
            
            le = le->Flink;
        }
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
            add_to_tree_cache(Vcb, par, TRUE);
            par = par->parent;
        }
        
        free_tree(next_tree);
    }
    
    return STATUS_SUCCESS;
}

static NTSTATUS STDCALL do_splits(device_extension* Vcb, LIST_ENTRY* rollback) {
//     LIST_ENTRY *le, *le2;
//     write_tree* wt;
//     tree_data* td;
    UINT8 level, max_level;
    UINT32 min_size;
    BOOL empty, done_deletions = FALSE;
    NTSTATUS Status;
    tree_cache* tc2;
    
    TRACE("(%p)\n", Vcb);
    
    max_level = 0;
    
    for (level = 0; level <= 255; level++) {
        LIST_ENTRY *le, *nextle;
        
        empty = TRUE;
        
        TRACE("doing level %u\n", level);
        
        le = Vcb->tree_cache.Flink;
    
        while (le != &Vcb->tree_cache) {
            tc2 = CONTAINING_RECORD(le, tree_cache, list_entry);
            
            nextle = le->Flink;
            
            if (tc2->write && tc2->tree->header.level == level) {
                empty = FALSE;
                
                if (tc2->tree->header.num_items == 0) {
                    LIST_ENTRY* le2;
                    KEY firstitem = {0xcccccccccccccccc,0xcc,0xcccccccccccccccc};
                    
                    done_deletions = TRUE;
        
                    le2 = tc2->tree->itemlist.Flink;
                    while (le2 != &tc2->tree->itemlist) {
                        tree_data* td = CONTAINING_RECORD(le2, tree_data, list_entry);
                        firstitem = td->key;
                        break;
                    }
                    
                    ERR("deleting tree in root %llx (first item was %llx,%x,%llx)\n",
                        tc2->tree->root->id, firstitem.obj_id, firstitem.obj_type, firstitem.offset);
                    
                    if (tc2->tree->new_address != 0) { // delete associated EXTENT_ITEM
                        Status = reduce_tree_extent(Vcb, tc2->tree->new_address, tc2->tree, rollback);
                        
                        if (!NT_SUCCESS(Status)) {
                            ERR("reduce_tree_extent returned %08x\n", Status);
                            return Status;
                        }
                    } else if (tc2->tree->header.address != 0) {
                        Status = reduce_tree_extent(Vcb,tc2->tree->header.address, tc2->tree, rollback);
                        
                        if (!NT_SUCCESS(Status)) {
                            ERR("reduce_tree_extent returned %08x\n", Status);
                            return Status;
                        }
                    }
                    
                    if (tc2->tree->parent) {
                        if (!tc2->tree->paritem->ignore) {
                            tc2->tree->paritem->ignore = TRUE;
                            tc2->tree->parent->header.num_items--;
                            tc2->tree->parent->size -= sizeof(internal_node);
                        }
                        
                        RemoveEntryList(&tc2->tree->paritem->list_entry);
                        ExFreePool(tc2->tree->paritem);
                        tc2->tree->paritem = NULL;
                        
                        free_tree(tc2->tree);
                        
                        RemoveEntryList(le);
                        ExFreePool(tc2);
                    } else {
                        FIXME("trying to delete top root, not sure what to do here\n"); // FIXME
                        return STATUS_INTERNAL_ERROR;
                    }
                } else {
                    if (tc2->tree->size > Vcb->superblock.node_size - sizeof(tree_header)) {
                        TRACE("splitting overlarge tree (%x > %x)\n", tc2->tree->size, Vcb->superblock.node_size - sizeof(tree_header));
                        Status = split_tree(Vcb, tc2->tree);

                        if (!NT_SUCCESS(Status)) {
                            ERR("split_tree returned %08x\n", Status);
                            return Status;
                        }
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
        
        le = Vcb->tree_cache.Flink;
    
        while (le != &Vcb->tree_cache) {
            tc2 = CONTAINING_RECORD(le, tree_cache, list_entry);
            
            if (tc2->write && tc2->tree->header.level == level && tc2->tree->header.num_items > 0 && tc2->tree->parent && tc2->tree->size < min_size) {
                Status = try_tree_amalgamate(Vcb, tc2->tree, rollback);
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
            
            le = Vcb->tree_cache.Flink;
            while (le != &Vcb->tree_cache) {
                nextle = le->Flink;
                tc2 = CONTAINING_RECORD(le, tree_cache, list_entry);
                
                if (tc2->write && tc2->tree->header.level == level) {
                    if (!tc2->tree->parent && tc2->tree->header.num_items == 1) {
                        LIST_ENTRY* le2 = tc2->tree->itemlist.Flink;
                        tree_data* td;
                        tree* child_tree = NULL;

                        while (le2 != &tc2->tree->itemlist) {
                            td = CONTAINING_RECORD(le2, tree_data, list_entry);
                            if (!td->ignore)
                                break;
                            le2 = le2->Flink;
                        }
                        
                        ERR("deleting top-level tree in root %llx with one item\n", tc2->tree->root->id);
                        
                        if (tc2->tree->new_address != 0) { // delete associated EXTENT_ITEM
                            Status = reduce_tree_extent(Vcb, tc2->tree->new_address, tc2->tree, rollback);
                            
                            if (!NT_SUCCESS(Status)) {
                                ERR("reduce_tree_extent returned %08x\n", Status);
                                return Status;
                            }
                        } else if (tc2->tree->header.address != 0) {
                            Status = reduce_tree_extent(Vcb,tc2->tree->header.address, tc2->tree, rollback);
                            
                            if (!NT_SUCCESS(Status)) {
                                ERR("reduce_tree_extent returned %08x\n", Status);
                                return Status;
                            }
                        }
                        
                        if (!td->treeholder.tree) { // load first item if not already loaded
                            KEY searchkey = {0,0,0};
                            traverse_ptr tp;
                            
                            Status = find_item(Vcb, tc2->tree->root, &tp, &searchkey, FALSE);
                            if (!NT_SUCCESS(Status)) {
                                ERR("error - find_item returned %08x\n", Status);
                                return Status;
                            }
                            
                            free_traverse_ptr(&tp);
                        }
                        
                        child_tree = td->treeholder.tree;
                        
                        if (child_tree) {
                            child_tree->parent = NULL;
                            free_tree(tc2->tree);
                        }

                        free_tree(tc2->tree);
                        
                        if (child_tree)
                            child_tree->root->treeholder.tree = child_tree;
                        
                        RemoveEntryList(le);
                        ExFreePool(tc2);
                    }
                }
                
                le = nextle;
            }
        }
    }
    
    return STATUS_SUCCESS;
}

NTSTATUS STDCALL do_write(device_extension* Vcb, LIST_ENTRY* rollback) {
    NTSTATUS Status;
    LIST_ENTRY* le;
    
    TRACE("(%p)\n", Vcb);
    
    // If only changing superblock, e.g. changing label, we still need to rewrite
    // the root tree so the generations mach. Otherwise you won't be able to mount on Linux.
    if (Vcb->write_trees > 0) {
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
        
        add_to_tree_cache(Vcb, Vcb->root_root->treeholder.tree, TRUE);
        
        free_traverse_ptr(&tp);
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
    } while (!trees_consistent(Vcb));
    
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
    
    Status = write_superblocks(Vcb);
    if (!NT_SUCCESS(Status)) {
        ERR("write_superblocks returned %08x\n", Status);
        goto end;
    }
    
    clean_space_cache(Vcb);
    
    Vcb->superblock.generation++;
    
//     print_trees(tc); // TESTING
    
    Status = STATUS_SUCCESS;
    
    le = Vcb->tree_cache.Flink;
    while (le != &Vcb->tree_cache) {
        tree_cache* tc2 = CONTAINING_RECORD(le, tree_cache, list_entry);
        
        tc2->write = FALSE;
        
        le = le->Flink;
    }
    
    Vcb->write_trees = 0;
    
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

static __inline void insert_into_ordered_list(LIST_ENTRY* list, ordered_list* ins) {
    LIST_ENTRY* le = list->Flink;
    ordered_list* ol;
    
    while (le != list) {
        ol = (ordered_list*)le;
        
        if (ol->key > ins->key) {
            le->Blink->Flink = &ins->list_entry;
            ins->list_entry.Blink = le->Blink;
            le->Blink = &ins->list_entry;
            ins->list_entry.Flink = le;
            return;
        }
        
        le = le->Flink;
    }
    
    InsertTailList(list, &ins->list_entry);
}

static UINT64 get_extent_data_ref_hash(UINT64 root, UINT64 objid, UINT64 offset) {
    UINT32 high_crc = 0xffffffff, low_crc = 0xffffffff;
    
    // FIXME - can we test this?

    // FIXME - make sure numbers here are little-endian
    high_crc = calc_crc32c(high_crc, (UINT8*)&root, sizeof(UINT64));
    low_crc = calc_crc32c(low_crc, (UINT8*)&objid, sizeof(UINT64));
    low_crc = calc_crc32c(low_crc, (UINT8*)&offset, sizeof(UINT64));
    
    return ((UINT64)high_crc << 31) ^ (UINT64)low_crc;
}

NTSTATUS STDCALL add_extent_ref(device_extension* Vcb, UINT64 address, UINT64 size, root* subvol, UINT64 inode, UINT64 offset, LIST_ENTRY* rollback) {
    KEY searchkey;
    traverse_ptr tp;
    EXTENT_ITEM* ei;
    UINT8 *siptr, *type;
    ULONG len;
    UINT64 hash;
    EXTENT_DATA_REF* edr;
    NTSTATUS Status;
    
    TRACE("(%p, %llx, %llx, %llx, %llx, %llx)\n", Vcb, address, size, subvol->id, inode, offset);
    
    searchkey.obj_id = address;
    searchkey.obj_type = TYPE_EXTENT_ITEM;
    searchkey.offset = size;
    
    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    if (keycmp(&tp.item->key, &searchkey)) {
        // create new entry
        
        len = sizeof(EXTENT_ITEM) + sizeof(UINT8) + sizeof(EXTENT_DATA_REF);
        free_traverse_ptr(&tp);
        
        ei = ExAllocatePoolWithTag(PagedPool, len, ALLOC_TAG);
        if (!ei) {
            ERR("out of memory\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        
        ei->refcount = 1;
        ei->generation = Vcb->superblock.generation;
        ei->flags = EXTENT_ITEM_DATA;
        
        type = (UINT8*)&ei[1];
        *type = TYPE_EXTENT_DATA_REF;
        
        edr = (EXTENT_DATA_REF*)&type[1];
        edr->root = subvol->id;
        edr->objid = inode;
        edr->offset = offset;
        edr->count = 1;
        
        if (!insert_tree_item(Vcb, Vcb->extent_root, searchkey.obj_id, searchkey.obj_type, searchkey.offset, ei, len, NULL, rollback)) {
            ERR("error - failed to insert item\n");
            return STATUS_INTERNAL_ERROR;
        }
        
        // FIXME - update free space in superblock and CHUNK_ITEM
        
        return STATUS_SUCCESS;
    }
    
    if (tp.item->size == sizeof(EXTENT_ITEM_V0)) { // old extent ref, convert
        NTSTATUS Status = convert_old_data_extent(Vcb, address, size, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("convert_old_data_extent returned %08x\n", Status);
            free_traverse_ptr(&tp);
            return Status;
        }
        
        free_traverse_ptr(&tp);
        
        Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            return Status;
        }
        
        if (keycmp(&tp.item->key, &searchkey)) {
            WARN("extent item not found for address %llx, size %llx\n", address, size);
            free_traverse_ptr(&tp);
            return STATUS_SUCCESS;
        }
    }
    
    ei = (EXTENT_ITEM*)tp.item->data;
    
    if (tp.item->size < sizeof(EXTENT_ITEM)) {
        ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_ITEM));
        free_traverse_ptr(&tp);
        return STATUS_INTERNAL_ERROR;
    }
    
    if (extent_item_is_shared(ei, tp.item->size - sizeof(EXTENT_ITEM))) {
        NTSTATUS Status = convert_shared_data_extent(Vcb, address, size, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("convert_shared_data_extent returned %08x\n", Status);
            free_traverse_ptr(&tp);
            return Status;
        }
        
        free_traverse_ptr(&tp);
        
        Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            return Status;
        }
        
        if (keycmp(&tp.item->key, &searchkey)) {
            WARN("extent item not found for address %llx, size %llx\n", address, size);
            free_traverse_ptr(&tp);
            return STATUS_SUCCESS;
        }
        
        ei = (EXTENT_ITEM*)tp.item->data;
    }
    
    if (ei->flags != EXTENT_ITEM_DATA) {
        ERR("error - flag was not EXTENT_ITEM_DATA\n");
        free_traverse_ptr(&tp);
        return STATUS_INTERNAL_ERROR;
    }
    
    // FIXME - is ei->refcount definitely the number of items, or is it the sum of the subitem refcounts?

    hash = get_extent_data_ref_hash(subvol->id, inode, offset);
    
    len = tp.item->size - sizeof(EXTENT_ITEM);
    siptr = (UINT8*)&ei[1];
    
    // FIXME - increase subitem refcount if there already?
    do {
        if (*siptr == TYPE_EXTENT_DATA_REF) {
            UINT64 sihash;
            
            edr = (EXTENT_DATA_REF*)&siptr[1];
            sihash = get_extent_data_ref_hash(edr->root, edr->objid, edr->offset);
            
            if (sihash >= hash)
                break;
            
            siptr += sizeof(UINT8) + sizeof(EXTENT_DATA_REF);
            
            if (len > sizeof(EXTENT_DATA_REF) + sizeof(UINT8)) {
                len -= sizeof(EXTENT_DATA_REF) + sizeof(UINT8);
            } else
                break;
        // FIXME - TYPE_TREE_BLOCK_REF    0xB0
        } else {
            ERR("unrecognized extent subitem %x\n", *siptr);
            free_traverse_ptr(&tp);
            return STATUS_INTERNAL_ERROR;
        }
    } while (len > 0);
    
    len = tp.item->size + sizeof(UINT8) + sizeof(EXTENT_DATA_REF); // FIXME - die if too big
    ei = ExAllocatePoolWithTag(PagedPool, len, ALLOC_TAG);
    if (!ei) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlCopyMemory(ei, tp.item->data, siptr - tp.item->data);
    ei->refcount++;
    
    type = (UINT8*)ei + (siptr - tp.item->data);
    *type = TYPE_EXTENT_DATA_REF;
    
    edr = (EXTENT_DATA_REF*)&type[1];
    edr->root = subvol->id;
    edr->objid = inode;
    edr->offset = offset;
    edr->count = 1;
    
    if (siptr < tp.item->data + tp.item->size)
        RtlCopyMemory(&edr[1], siptr, tp.item->data + tp.item->size - siptr);
    
    delete_tree_item(Vcb, &tp, rollback);
    
    free_traverse_ptr(&tp);
    
    if (!insert_tree_item(Vcb, Vcb->extent_root, searchkey.obj_id, searchkey.obj_type, searchkey.offset, ei, len, NULL, rollback)) {
        ERR("error - failed to insert item\n");
        ExFreePool(ei);
        return STATUS_INTERNAL_ERROR;
    }
    
    return STATUS_SUCCESS;
}

typedef struct {
    EXTENT_DATA_REF edr;
    LIST_ENTRY list_entry;
} data_ref;

static void add_data_ref(LIST_ENTRY* data_refs, UINT64 root, UINT64 objid, UINT64 offset) {
    data_ref* dr = ExAllocatePoolWithTag(PagedPool, sizeof(data_ref), ALLOC_TAG);
    
    if (!dr) {
        ERR("out of memory\n");
        return;
    }
    
    // FIXME - increase count if entry there already
    // FIXME - put in order?
    
    dr->edr.root = root;
    dr->edr.objid = objid;
    dr->edr.offset = offset;
    dr->edr.count = 1;
    
    InsertTailList(data_refs, &dr->list_entry);
}

static void free_data_refs(LIST_ENTRY* data_refs) {
    while (!IsListEmpty(data_refs)) {
        LIST_ENTRY* le = RemoveHeadList(data_refs);
        data_ref* dr = CONTAINING_RECORD(le, data_ref, list_entry);
        
        ExFreePool(dr);
    }
}

static NTSTATUS convert_old_data_extent(device_extension* Vcb, UINT64 address, UINT64 size, LIST_ENTRY* rollback) {
    KEY searchkey;
    traverse_ptr tp, next_tp;
    BOOL b;
    LIST_ENTRY data_refs;
    LIST_ENTRY* le;
    UINT64 refcount;
    EXTENT_ITEM* ei;
    ULONG eisize;
    UINT8* type;
    NTSTATUS Status;
    
    searchkey.obj_id = address;
    searchkey.obj_type = TYPE_EXTENT_ITEM;
    searchkey.offset = size;
    
    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    if (keycmp(&tp.item->key, &searchkey)) {
        WARN("extent item not found for address %llx, size %llx\n", address, size);
        free_traverse_ptr(&tp);
        return STATUS_SUCCESS;
    }
    
    if (tp.item->size != sizeof(EXTENT_ITEM_V0)) {
        TRACE("extent does not appear to be old - returning STATUS_SUCCESS\n");
        free_traverse_ptr(&tp);
        return STATUS_SUCCESS;
    }
    
    delete_tree_item(Vcb, &tp, rollback);
    
    free_traverse_ptr(&tp);
    
    searchkey.obj_id = address;
    searchkey.obj_type = TYPE_EXTENT_REF_V0;
    searchkey.offset = 0;
    
    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    InitializeListHead(&data_refs);
    
    do {
        b = find_next_item(Vcb, &tp, &next_tp, FALSE);
        
        if (tp.item->key.obj_id == searchkey.obj_id && tp.item->key.obj_type == searchkey.obj_type) {
            tree* t;
            
            // normally we'd need to acquire load_tree_lock here, but we're protected by the write tree lock
    
            Status = load_tree(Vcb, tp.item->key.offset, NULL, &t);
            
            if (!NT_SUCCESS(Status)) {
                ERR("load tree for address %llx returned %08x\n", tp.item->key.offset, Status);
                free_traverse_ptr(&tp);
                free_data_refs(&data_refs);
                return Status;
            }
            
            if (t->header.level == 0) {
                le = t->itemlist.Flink;
                while (le != &t->itemlist) {
                    tree_data* td = CONTAINING_RECORD(le, tree_data, list_entry);
                    
                    if (!td->ignore && td->key.obj_type == TYPE_EXTENT_DATA) {
                        EXTENT_DATA* ed = (EXTENT_DATA*)td->data;
                        
                        if (ed->type == EXTENT_TYPE_REGULAR || ed->type == EXTENT_TYPE_PREALLOC) {
                            EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ed->data;
                            
                            if (ed2->address == address)
                                add_data_ref(&data_refs, t->header.tree_id, td->key.obj_id, td->key.offset);
                        }
                    }
                    
                    le = le->Flink;
                }
            }
            
            free_tree(t);
            
            delete_tree_item(Vcb, &tp, rollback);
        }
        
        if (b) {
            free_traverse_ptr(&tp);
            tp = next_tp;
            
            if (tp.item->key.obj_id > searchkey.obj_id || tp.item->key.obj_type > searchkey.obj_type)
                break;
        }
    } while (b);
    
    free_traverse_ptr(&tp);
    
    if (IsListEmpty(&data_refs)) {
        WARN("no data refs found\n");
        return STATUS_SUCCESS;
    }
    
    // create new entry
    
    refcount = 0;
    
    le = data_refs.Flink;
    while (le != &data_refs) {
        refcount++;
        le = le->Flink;
    }
    
    eisize = sizeof(EXTENT_ITEM) + ((sizeof(char) + sizeof(EXTENT_DATA_REF)) * refcount);
    ei = ExAllocatePoolWithTag(PagedPool, eisize, ALLOC_TAG);
    if (!ei) {
        ERR("out of memory\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    ei->refcount = refcount;
    ei->generation = Vcb->superblock.generation;
    ei->flags = EXTENT_ITEM_DATA;
    
    type = (UINT8*)&ei[1];
    
    le = data_refs.Flink;
    while (le != &data_refs) {
        data_ref* dr = CONTAINING_RECORD(le, data_ref, list_entry);
        
        type[0] = TYPE_EXTENT_DATA_REF;
        RtlCopyMemory(&type[1], &dr->edr, sizeof(EXTENT_DATA_REF));
        
        type = &type[1 + sizeof(EXTENT_DATA_REF)];
        
        le = le->Flink;
    }
    
    if (!insert_tree_item(Vcb, Vcb->extent_root, address, TYPE_EXTENT_ITEM, size, ei, eisize, NULL, rollback)) {
        ERR("error - failed to insert item\n");
        ExFreePool(ei);
        return STATUS_INTERNAL_ERROR;
    }
    
    free_data_refs(&data_refs);
    
    return STATUS_SUCCESS;
}

typedef struct {
    UINT8 type;
    void* data;
    BOOL allocated;
    LIST_ENTRY list_entry;
} extent_ref;

static void free_extent_refs(LIST_ENTRY* extent_refs) {
    while (!IsListEmpty(extent_refs)) {
        LIST_ENTRY* le = RemoveHeadList(extent_refs);
        extent_ref* er = CONTAINING_RECORD(le, extent_ref, list_entry);
        
        if (er->allocated)
            ExFreePool(er->data);
        
        ExFreePool(er);
    }
}

static NTSTATUS convert_shared_data_extent(device_extension* Vcb, UINT64 address, UINT64 size, LIST_ENTRY* rollback) {
    KEY searchkey;
    traverse_ptr tp;
    LIST_ENTRY extent_refs;
    LIST_ENTRY *le, *next_le;
    EXTENT_ITEM *ei, *newei;
    UINT8* siptr;
    ULONG len;
    UINT64 count;
    NTSTATUS Status;
    
    searchkey.obj_id = address;
    searchkey.obj_type = TYPE_EXTENT_ITEM;
    searchkey.offset = size;
    
    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    if (keycmp(&tp.item->key, &searchkey)) {
        WARN("extent item not found for address %llx, size %llx\n", address, size);
        free_traverse_ptr(&tp);
        return STATUS_SUCCESS;
    }
    
    if (tp.item->size < sizeof(EXTENT_ITEM)) {
        ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_ITEM));
        free_traverse_ptr(&tp);
        return STATUS_INTERNAL_ERROR;
    }
    
    ei = (EXTENT_ITEM*)tp.item->data;
    len = tp.item->size - sizeof(EXTENT_ITEM);
    
    InitializeListHead(&extent_refs);
    
    siptr = (UINT8*)&ei[1];
    
    do {
        extent_ref* er = ExAllocatePoolWithTag(PagedPool, sizeof(extent_ref), ALLOC_TAG);
        if (!er) {
            ERR("out of memory\n");
            free_traverse_ptr(&tp);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        er->type = *siptr;
        er->data = siptr+1;
        er->allocated = FALSE;
        
        InsertTailList(&extent_refs, &er->list_entry);
        
        if (*siptr == TYPE_TREE_BLOCK_REF) {
            siptr += sizeof(TREE_BLOCK_REF);
            len -= sizeof(TREE_BLOCK_REF) + 1;
        } else if (*siptr == TYPE_EXTENT_DATA_REF) {
            siptr += sizeof(EXTENT_DATA_REF);
            len -= sizeof(EXTENT_DATA_REF) + 1;
        } else if (*siptr == TYPE_SHARED_BLOCK_REF) {
            siptr += sizeof(SHARED_BLOCK_REF);
            len -= sizeof(SHARED_BLOCK_REF) + 1;
        } else if (*siptr == TYPE_SHARED_DATA_REF) {
            siptr += sizeof(SHARED_DATA_REF);
            len -= sizeof(SHARED_DATA_REF) + 1;
        } else {
            ERR("unrecognized extent subitem %x\n", *siptr);
            free_traverse_ptr(&tp);
            free_extent_refs(&extent_refs);
            return STATUS_INTERNAL_ERROR;
        }
    } while (len > 0);
    
    le = extent_refs.Flink;
    while (le != &extent_refs) {
        extent_ref* er = CONTAINING_RECORD(le, extent_ref, list_entry);
        next_le = le->Flink;
        
        if (er->type == TYPE_SHARED_DATA_REF) {
            // normally we'd need to acquire load_tree_lock here, but we're protected by the write tree lock
            SHARED_DATA_REF* sdr = er->data;
            tree* t;
            
            Status = load_tree(Vcb, sdr->offset, NULL, &t);
            if (!NT_SUCCESS(Status)) {
                ERR("load_tree for address %llx returned %08x\n", sdr->offset, Status);
                free_traverse_ptr(&tp);
                free_data_refs(&extent_refs);
                return Status;
            }
            
            if (t->header.level == 0) {
                LIST_ENTRY* le2 = t->itemlist.Flink;
                while (le2 != &t->itemlist) {
                    tree_data* td = CONTAINING_RECORD(le2, tree_data, list_entry);
                    
                    if (!td->ignore && td->key.obj_type == TYPE_EXTENT_DATA) {
                        EXTENT_DATA* ed = (EXTENT_DATA*)td->data;
                        
                        if (ed->type == EXTENT_TYPE_REGULAR || ed->type == EXTENT_TYPE_PREALLOC) {
                            EXTENT_DATA2* ed2 = (EXTENT_DATA2*)ed->data;
                            
                            if (ed2->address == address) {
                                extent_ref* er2;
                                EXTENT_DATA_REF* edr;
                                
                                er2 = ExAllocatePoolWithTag(PagedPool, sizeof(extent_ref), ALLOC_TAG);
                                if (!er2) {
                                    ERR("out of memory\n");
                                    free_traverse_ptr(&tp);
                                    return STATUS_INSUFFICIENT_RESOURCES;
                                }
                                
                                edr = ExAllocatePoolWithTag(PagedPool, sizeof(EXTENT_DATA_REF), ALLOC_TAG);
                                if (!edr) {
                                    ERR("out of memory\n");
                                    free_traverse_ptr(&tp);
                                    ExFreePool(er2);
                                    return STATUS_INSUFFICIENT_RESOURCES;
                                }
                                
                                edr->root = t->header.tree_id;
                                edr->objid = td->key.obj_id;
                                edr->offset = td->key.offset;
                                edr->count = 1;
                                
                                er2->type = TYPE_EXTENT_DATA_REF;
                                er2->data = edr;
                                er2->allocated = TRUE;
                                
                                InsertTailList(&extent_refs, &er2->list_entry); // FIXME - list should be in order
                            }
                        }
                    }
                    
                    le2 = le2->Flink;
                }
            }
            
            free_tree(t);

            RemoveEntryList(&er->list_entry);
            
            if (er->allocated)
                ExFreePool(er->data);
            
            ExFreePool(er);
        }
        // FIXME - also do for SHARED_BLOCK_REF?

        le = next_le;
    }
    
    if (IsListEmpty(&extent_refs)) {
        WARN("no extent refs found\n");
        delete_tree_item(Vcb, &tp, rollback);
        free_traverse_ptr(&tp);
        return STATUS_SUCCESS;
    }
    
    len = 0;
    count = 0;
    le = extent_refs.Flink;
    while (le != &extent_refs) {
        extent_ref* er = CONTAINING_RECORD(le, extent_ref, list_entry);
        
        len++;
        if (er->type == TYPE_TREE_BLOCK_REF) {
            len += sizeof(TREE_BLOCK_REF);
        } else if (er->type == TYPE_EXTENT_DATA_REF) {
            len += sizeof(EXTENT_DATA_REF);
        } else {
            ERR("unexpected extent subitem %x\n", er->type);
        }
        
        count++;
        
        le = le->Flink;
    }
    
    newei = ExAllocatePoolWithTag(PagedPool, sizeof(EXTENT_ITEM) + len, ALLOC_TAG);
    if (!newei) {
        ERR("out of memory\n");
        free_traverse_ptr(&tp);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    RtlCopyMemory(newei, ei, sizeof(EXTENT_ITEM));
    newei->refcount = count;
    
    siptr = (UINT8*)&newei[1];
    le = extent_refs.Flink;
    while (le != &extent_refs) {
        extent_ref* er = CONTAINING_RECORD(le, extent_ref, list_entry);
        
        *siptr = er->type;
        siptr++;
        
        if (er->type == TYPE_TREE_BLOCK_REF) {
            RtlCopyMemory(siptr, er->data, sizeof(TREE_BLOCK_REF));
        } else if (er->type == TYPE_EXTENT_DATA_REF) {
            RtlCopyMemory(siptr, er->data, sizeof(EXTENT_DATA_REF));
        } else {
            ERR("unexpected extent subitem %x\n", er->type);
        }
        
        le = le->Flink;
    }
    
    delete_tree_item(Vcb, &tp, rollback);
    free_traverse_ptr(&tp);
    
    if (!insert_tree_item(Vcb, Vcb->extent_root, address, TYPE_EXTENT_ITEM, size, newei, sizeof(EXTENT_ITEM) + len, NULL, rollback)) {
        ERR("error - failed to insert item\n");
        ExFreePool(newei);
        free_extent_refs(&extent_refs);
        return STATUS_INTERNAL_ERROR;
    }
    
    free_extent_refs(&extent_refs);
    
    return STATUS_SUCCESS;
}

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

NTSTATUS STDCALL remove_extent_ref(device_extension* Vcb, UINT64 address, UINT64 size, root* subvol, UINT64 inode, UINT64 offset, LIST_ENTRY* changed_sector_list, LIST_ENTRY* rollback) {
    KEY searchkey;
    traverse_ptr tp;
    EXTENT_ITEM* ei;
    UINT8* siptr;
    ULONG len;
    EXTENT_DATA_REF* edr;
    BOOL found;
    NTSTATUS Status;
    
    TRACE("(%p, %llx, %llx, %llx, %llx, %llx)\n", Vcb, address, size, subvol->id, inode, offset);
    
    searchkey.obj_id = address;
    searchkey.obj_type = TYPE_EXTENT_ITEM;
    searchkey.offset = size;
    
    Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }
    
    if (keycmp(&tp.item->key, &searchkey)) {
        WARN("extent item not found for address %llx, size %llx\n", address, size);
        free_traverse_ptr(&tp);
        return STATUS_SUCCESS;
    }
    
    if (tp.item->size == sizeof(EXTENT_ITEM_V0)) { // old extent ref, convert
        NTSTATUS Status = convert_old_data_extent(Vcb, address, size, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("convert_old_data_extent returned %08x\n", Status);
            free_traverse_ptr(&tp);
            return Status;
        }
        
        free_traverse_ptr(&tp);
        
        Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            return Status;
        }
        
        if (keycmp(&tp.item->key, &searchkey)) {
            WARN("extent item not found for address %llx, size %llx\n", address, size);
            free_traverse_ptr(&tp);
            return STATUS_SUCCESS;
        }
    }
    
    ei = (EXTENT_ITEM*)tp.item->data;
    
    if (tp.item->size < sizeof(EXTENT_ITEM)) {
        ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_ITEM));
        free_traverse_ptr(&tp);
        return STATUS_INTERNAL_ERROR;
    }
    
    if (!(ei->flags & EXTENT_ITEM_DATA)) {
        ERR("error - EXTENT_ITEM_DATA flag not set\n");
        free_traverse_ptr(&tp);
        return STATUS_INTERNAL_ERROR;
    }
    
    // FIXME - is ei->refcount definitely the number of items, or is it the sum of the subitem refcounts?
    
    if (extent_item_is_shared(ei, tp.item->size - sizeof(EXTENT_ITEM))) {
        NTSTATUS Status = convert_shared_data_extent(Vcb, address, size, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("convert_shared_data_extent returned %08x\n", Status);
            free_traverse_ptr(&tp);
            return Status;
        }
        
        free_traverse_ptr(&tp);
        
        Status = find_item(Vcb, Vcb->extent_root, &tp, &searchkey, FALSE);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            return Status;
        }
        
        if (keycmp(&tp.item->key, &searchkey)) {
            WARN("extent item not found for address %llx, size %llx\n", address, size);
            free_traverse_ptr(&tp);
            return STATUS_SUCCESS;
        }
        
        ei = (EXTENT_ITEM*)tp.item->data;
    }
    
    len = tp.item->size - sizeof(EXTENT_ITEM);
    siptr = (UINT8*)&ei[1];
    found = FALSE;
    
    do {
        if (*siptr == TYPE_EXTENT_DATA_REF) {
            edr = (EXTENT_DATA_REF*)&siptr[1];
            
            if (edr->root == subvol->id && edr->objid == inode && edr->offset == offset) {
                found = TRUE;
                break;
            }

            siptr += sizeof(UINT8) + sizeof(EXTENT_DATA_REF);
            
            if (len > sizeof(EXTENT_DATA_REF) + sizeof(UINT8)) {
                len -= sizeof(EXTENT_DATA_REF) + sizeof(UINT8);
            } else
                break;
//         // FIXME - TYPE_TREE_BLOCK_REF    0xB0
        } else {
            ERR("unrecognized extent subitem %x\n", *siptr);
            free_traverse_ptr(&tp);
            return STATUS_INTERNAL_ERROR;
        }
    } while (len > 0);
    
    if (!found) {
        WARN("could not find extent data ref\n");
        free_traverse_ptr(&tp);
        return STATUS_SUCCESS;
    }
    
    // FIXME - decrease subitem refcount if there already?
    
    len = tp.item->size - sizeof(UINT8) - sizeof(EXTENT_DATA_REF);
    
    delete_tree_item(Vcb, &tp, rollback);
    
    if (len == sizeof(EXTENT_ITEM)) { // extent no longer needed
        chunk* c;
        LIST_ENTRY* le2;
        
        if (changed_sector_list) {
            changed_sector* sc = ExAllocatePoolWithTag(PagedPool, sizeof(changed_sector), ALLOC_TAG);
            if (!sc) {
                ERR("out of memory\n");
                free_traverse_ptr(&tp);
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            
            sc->ol.key = address;
            sc->checksums = NULL;
            sc->length = size / Vcb->superblock.sector_size;

            sc->deleted = TRUE;
            
            insert_into_ordered_list(changed_sector_list, &sc->ol);
        }
        
        c = NULL;
        le2 = Vcb->chunks.Flink;
        while (le2 != &Vcb->chunks) {
            c = CONTAINING_RECORD(le2, chunk, list_entry);
            
            TRACE("chunk: %llx, %llx\n", c->offset, c->chunk_item->size);
            
            if (address >= c->offset && address + size < c->offset + c->chunk_item->size)
                break;
            
            le2 = le2->Flink;
        }
        if (le2 == &Vcb->chunks) c = NULL;
        
        if (c) {
            decrease_chunk_usage(c, size);
            
            add_to_space_list(c, address, size, SPACE_TYPE_DELETING);
        }
        
        free_traverse_ptr(&tp);
        return STATUS_SUCCESS;
    }
    
    ei = ExAllocatePoolWithTag(PagedPool, len, ALLOC_TAG);
    if (!ei) {
        ERR("out of memory\n");
        free_traverse_ptr(&tp);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
            
    RtlCopyMemory(ei, tp.item->data, siptr - tp.item->data);
    ei->refcount--;
    ei->generation = Vcb->superblock.generation;
    
    if (tp.item->data + len != siptr)
        RtlCopyMemory((UINT8*)ei + (siptr - tp.item->data), siptr + sizeof(UINT8) + sizeof(EXTENT_DATA_REF), tp.item->size - (siptr - tp.item->data) - sizeof(UINT8) - sizeof(EXTENT_DATA_REF));
    
    free_traverse_ptr(&tp);
    
    if (!insert_tree_item(Vcb, Vcb->extent_root, searchkey.obj_id, searchkey.obj_type, searchkey.offset, ei, len, NULL, rollback)) {
        ERR("error - failed to insert item\n");
        ExFreePool(ei);
        return STATUS_INTERNAL_ERROR;
    }
    
    return STATUS_SUCCESS;
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

NTSTATUS excise_extents(device_extension* Vcb, fcb* fcb, UINT64 start_data, UINT64 end_data, LIST_ENTRY* changed_sector_list, LIST_ENTRY* rollback) {
    KEY searchkey;
    traverse_ptr tp, next_tp;
    NTSTATUS Status;
    BOOL b;
    
    TRACE("(%p, (%llx, %llx), %llx, %llx, %p)\n", Vcb, fcb->subvol->id, fcb->inode, start_data, end_data, changed_sector_list);
    
    searchkey.obj_id = fcb->inode;
    searchkey.obj_type = TYPE_EXTENT_DATA;
    searchkey.offset = start_data;
    
    Status = find_item(Vcb, fcb->subvol, &tp, &searchkey, FALSE);
    if (!NT_SUCCESS(Status)) {
        ERR("error - find_item returned %08x\n", Status);
        return Status;
    }

    do {
        EXTENT_DATA* ed = (EXTENT_DATA*)tp.item->data;
        
        if (tp.item->size < sizeof(EXTENT_DATA)) {
            ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_DATA));
            Status = STATUS_INTERNAL_ERROR;
            goto end;
        }
        
        if ((ed->type == EXTENT_TYPE_REGULAR || ed->type == EXTENT_TYPE_PREALLOC) && tp.item->size < sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2)) {
            ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2));
            Status = STATUS_INTERNAL_ERROR;
            goto end;
        }
        
        b = find_next_item(Vcb, &tp, &next_tp, FALSE);
        
        if (tp.item->key.offset < end_data && tp.item->key.offset + ed->decoded_size >= start_data) {
            if (ed->compression != BTRFS_COMPRESSION_NONE) {
                FIXME("FIXME - compression not supported at present\n");
                Status = STATUS_NOT_SUPPORTED;
                goto end;
            }
            
            if (ed->encryption != BTRFS_ENCRYPTION_NONE) {
                WARN("root %llx, inode %llx, extent %llx: encryption not supported (type %x)\n", fcb->subvol->id, fcb->inode, tp.item->key.offset, ed->encryption);
                Status = STATUS_NOT_SUPPORTED;
                goto end;
            }
            
            if (ed->encoding != BTRFS_ENCODING_NONE) {
                WARN("other encodings not supported\n");
                Status = STATUS_NOT_SUPPORTED;
                goto end;
            }
            
            // FIXME - is ed->decoded_size the size of the whole extent, or just this bit of it?
            
            if (ed->type == EXTENT_TYPE_INLINE) {
                if (start_data <= tp.item->key.offset && end_data >= tp.item->key.offset + ed->decoded_size) { // remove all
                    delete_tree_item(Vcb, &tp, rollback);
                    
                    fcb->inode_item.st_blocks -= ed->decoded_size;
                } else if (start_data <= tp.item->key.offset && end_data < tp.item->key.offset + ed->decoded_size) { // remove beginning
                    EXTENT_DATA* ned;
                    UINT64 size;
                    
                    delete_tree_item(Vcb, &tp, rollback);
                    
                    size = ed->decoded_size - (end_data - tp.item->key.offset);
                    
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
                    
                    if (!insert_tree_item(Vcb, fcb->subvol, fcb->inode, TYPE_EXTENT_DATA, end_data, ned, sizeof(EXTENT_DATA) - 1 + size, NULL, rollback)) {
                        ERR("insert_tree_item failed\n");
                        ExFreePool(ned);
                        Status = STATUS_INTERNAL_ERROR;
                        goto end;
                    }
                    
                    fcb->inode_item.st_blocks -= end_data - tp.item->key.offset;
                } else if (start_data > tp.item->key.offset && end_data >= tp.item->key.offset + ed->decoded_size) { // remove end
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
                    
                    if (!insert_tree_item(Vcb, fcb->subvol, fcb->inode, TYPE_EXTENT_DATA, tp.item->key.offset, ned, sizeof(EXTENT_DATA) - 1 + size, NULL, rollback)) {
                        ERR("insert_tree_item failed\n");
                        ExFreePool(ned);
                        Status = STATUS_INTERNAL_ERROR;
                        goto end;
                    }
                    
                    fcb->inode_item.st_blocks -= tp.item->key.offset + ed->decoded_size - start_data;
                } else if (start_data > tp.item->key.offset && end_data < tp.item->key.offset + ed->decoded_size) { // remove middle
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
                    
                    if (!insert_tree_item(Vcb, fcb->subvol, fcb->inode, TYPE_EXTENT_DATA, tp.item->key.offset, ned, sizeof(EXTENT_DATA) - 1 + size, NULL, rollback)) {
                        ERR("insert_tree_item failed\n");
                        ExFreePool(ned);
                        Status = STATUS_INTERNAL_ERROR;
                        goto end;
                    }
                    
                    size = tp.item->key.offset + ed->decoded_size - end_data;
                    
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
                    
                    if (!insert_tree_item(Vcb, fcb->subvol, fcb->inode, TYPE_EXTENT_DATA, end_data, ned, sizeof(EXTENT_DATA) - 1 + size, NULL, rollback)) {
                        ERR("insert_tree_item failed\n");
                        ExFreePool(ned);
                        Status = STATUS_INTERNAL_ERROR;
                        goto end;
                    }
                    
                    fcb->inode_item.st_blocks -= end_data - start_data;
                }
            } else if (ed->type == EXTENT_TYPE_REGULAR || ed->type == EXTENT_TYPE_PREALLOC) {
                EXTENT_DATA2* ed2 = (EXTENT_DATA2*)&ed->data[0];
                
                if (start_data <= tp.item->key.offset && end_data >= tp.item->key.offset + ed->decoded_size) { // remove all
                    if (ed2->address != 0) {
                        Status = remove_extent_ref(Vcb, ed2->address, ed2->size, fcb->subvol, fcb->inode, tp.item->key.offset, changed_sector_list, rollback);
                        if (!NT_SUCCESS(Status)) {
                            ERR("remove_extent_ref returned %08x\n", Status);
                            goto end;
                        }
                        
                        fcb->inode_item.st_blocks -= ed->decoded_size;
                    }
                    
                    delete_tree_item(Vcb, &tp, rollback);
                } else if (start_data <= tp.item->key.offset && end_data < tp.item->key.offset + ed->decoded_size) { // remove beginning
                    EXTENT_DATA* ned;
                    EXTENT_DATA2* ned2;
                    
                    if (ed2->address != 0) {
                        Status = add_extent_ref(Vcb, ed2->address, ed2->size, fcb->subvol, fcb->inode, end_data, rollback);
                        if (!NT_SUCCESS(Status)) {
                            ERR("add_extent_ref returned %08x\n", Status);
                            goto end;
                        }
                        
                        Status = remove_extent_ref(Vcb, ed2->address, ed2->size, fcb->subvol, fcb->inode, tp.item->key.offset, changed_sector_list, rollback);
                        if (!NT_SUCCESS(Status)) {
                            ERR("remove_extent_ref returned %08x\n", Status);
                            goto end;
                        }
                        
                        fcb->inode_item.st_blocks -= end_data - tp.item->key.offset;
                    }
                    
                    delete_tree_item(Vcb, &tp, rollback);
                    
                    ned = ExAllocatePoolWithTag(PagedPool, sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2), ALLOC_TAG);
                    if (!ned) {
                        ERR("out of memory\n");
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        goto end;
                    }
                    
                    ned2 = (EXTENT_DATA2*)&ned->data[0];
                    
                    ned->generation = Vcb->superblock.generation;
                    ned->decoded_size = ed->decoded_size - (end_data - tp.item->key.offset);
                    ned->compression = ed->compression;
                    ned->encryption = ed->encryption;
                    ned->encoding = ed->encoding;
                    ned->type = ed->type;
                    ned2->address = ed2->address;
                    ned2->size = ed2->size;
                    ned2->offset = ed2->address == 0 ? 0 : (ed2->offset + (end_data - tp.item->key.offset));
                    ned2->num_bytes = ed2->num_bytes - (end_data - tp.item->key.offset);
                    
                    if (!insert_tree_item(Vcb, fcb->subvol, fcb->inode, TYPE_EXTENT_DATA, end_data, ned, sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2), NULL, rollback)) {
                        ERR("insert_tree_item failed\n");
                        ExFreePool(ned);
                        Status = STATUS_INTERNAL_ERROR;
                        goto end;
                    }
                } else if (start_data > tp.item->key.offset && end_data >= tp.item->key.offset + ed->decoded_size) { // remove end
                    EXTENT_DATA* ned;
                    EXTENT_DATA2* ned2;
                    
                    if (ed2->address != 0)
                        fcb->inode_item.st_blocks -= tp.item->key.offset + ed->decoded_size - start_data;
                    
                    delete_tree_item(Vcb, &tp, rollback);
                    
                    ned = ExAllocatePoolWithTag(PagedPool, sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2), ALLOC_TAG);
                    if (!ned) {
                        ERR("out of memory\n");
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        goto end;
                    }
                    
                    ned2 = (EXTENT_DATA2*)&ned->data[0];
                    
                    ned->generation = Vcb->superblock.generation;
                    ned->decoded_size = start_data - tp.item->key.offset;
                    ned->compression = ed->compression;
                    ned->encryption = ed->encryption;
                    ned->encoding = ed->encoding;
                    ned->type = ed->type;
                    ned2->address = ed2->address;
                    ned2->size = ed2->size;
                    ned2->offset = ed2->address == 0 ? 0 : ed2->offset;
                    ned2->num_bytes = start_data - tp.item->key.offset;
                    
                    if (!insert_tree_item(Vcb, fcb->subvol, fcb->inode, TYPE_EXTENT_DATA, tp.item->key.offset, ned, sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2), NULL, rollback)) {
                        ERR("insert_tree_item failed\n");
                        ExFreePool(ned);
                        Status = STATUS_INTERNAL_ERROR;
                        goto end;
                    }
                } else if (start_data > tp.item->key.offset && end_data < tp.item->key.offset + ed->decoded_size) { // remove middle
                    EXTENT_DATA* ned;
                    EXTENT_DATA2* ned2;
                    
                    if (ed2->address != 0) {
                        Status = add_extent_ref(Vcb, ed2->address, ed2->size, fcb->subvol, fcb->inode, end_data, rollback);
                        if (!NT_SUCCESS(Status)) {
                            ERR("add_extent_ref returned %08x\n", Status);
                            goto end;
                        }
                        
                        fcb->inode_item.st_blocks -= end_data - start_data;
                    }
                    
                    delete_tree_item(Vcb, &tp, rollback);
                    
                    ned = ExAllocatePoolWithTag(PagedPool, sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2), ALLOC_TAG);
                    if (!ned) {
                        ERR("out of memory\n");
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        goto end;
                    }
                    
                    ned2 = (EXTENT_DATA2*)&ned->data[0];
                    
                    ned->generation = Vcb->superblock.generation;
                    ned->decoded_size = start_data - tp.item->key.offset;
                    ned->compression = ed->compression;
                    ned->encryption = ed->encryption;
                    ned->encoding = ed->encoding;
                    ned->type = ed->type;
                    ned2->address = ed2->address;
                    ned2->size = ed2->size;
                    ned2->offset = ed2->address == 0 ? 0 : ed2->offset;
                    ned2->num_bytes = start_data - tp.item->key.offset;
                    
                    if (!insert_tree_item(Vcb, fcb->subvol, fcb->inode, TYPE_EXTENT_DATA, tp.item->key.offset, ned, sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2), NULL, rollback)) {
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
                    ned->decoded_size = tp.item->key.offset + ed->decoded_size - end_data;
                    ned->compression = ed->compression;
                    ned->encryption = ed->encryption;
                    ned->encoding = ed->encoding;
                    ned->type = ed->type;
                    ned2->address = ed2->address;
                    ned2->size = ed2->size;
                    ned2->offset = ed2->address == 0 ? 0 : (ed2->offset + (end_data - tp.item->key.offset));
                    ned2->num_bytes = tp.item->key.offset + ed->decoded_size - end_data;
                    
                    if (!insert_tree_item(Vcb, fcb->subvol, fcb->inode, TYPE_EXTENT_DATA, end_data, ned, sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2), NULL, rollback)) {
                        ERR("insert_tree_item failed\n");
                        ExFreePool(ned);
                        Status = STATUS_INTERNAL_ERROR;
                        goto end;
                    }
                }
            }
        }

        if (b) {
            free_traverse_ptr(&tp);
            tp = next_tp;
            
            if (tp.item->key.obj_id > fcb->inode || tp.item->key.obj_type > TYPE_EXTENT_DATA || tp.item->key.offset >= end_data)
                break;
        }
    } while (b);
    
    // FIXME - do bitmap analysis of changed extents, and free what we can
    
    Status = STATUS_SUCCESS;
    
end:
    free_traverse_ptr(&tp);
    
    return Status;
}

static BOOL insert_extent_chunk(device_extension* Vcb, fcb* fcb, chunk* c, UINT64 start_data, UINT64 length, void* data, LIST_ENTRY* changed_sector_list, LIST_ENTRY* rollback) {
    UINT64 address;
    NTSTATUS Status;
    EXTENT_ITEM_DATA_REF* eidr;
    EXTENT_DATA* ed;
    EXTENT_DATA2* ed2;
    ULONG edsize = sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2);
    changed_sector* sc;
    traverse_ptr tp;
    int i;
    
    TRACE("(%p, (%llx, %llx), %llx, %llx, %llx, %p, %p)\n", Vcb, fcb->subvol->id, fcb->inode, c->offset, start_data, length, data, changed_sector_list);
    
    if (!find_address_in_chunk(Vcb, c, length, &address))
        return FALSE;
    
    eidr = ExAllocatePoolWithTag(PagedPool, sizeof(EXTENT_ITEM_DATA_REF), ALLOC_TAG);
    if (!eidr) {
        ERR("out of memory\n");
        return FALSE;
    }
    
    eidr->ei.refcount = 1;
    eidr->ei.generation = Vcb->superblock.generation;
    eidr->ei.flags = EXTENT_ITEM_DATA;
    eidr->type = TYPE_EXTENT_DATA_REF;
    eidr->edr.root = fcb->subvol->id;
    eidr->edr.objid = fcb->inode;
    eidr->edr.offset = start_data;
    eidr->edr.count = 1;
    
    if (!insert_tree_item(Vcb, Vcb->extent_root, address, TYPE_EXTENT_ITEM, length, eidr, sizeof(EXTENT_ITEM_DATA_REF), &tp, rollback)) {
        ERR("insert_tree_item failed\n");
        ExFreePool(eidr);
        return FALSE;
    }
    
    tp.tree->header.generation = eidr->ei.generation;
    
    free_traverse_ptr(&tp);
    
    Status = write_data(Vcb, address, data, length);
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
        
        sc->ol.key = address;
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
    ed->type = EXTENT_TYPE_REGULAR;
    
    ed2 = (EXTENT_DATA2*)ed->data;
    ed2->address = address;
    ed2->size = length;
    ed2->offset = 0;
    ed2->num_bytes = length;
    
    if (!insert_tree_item(Vcb, fcb->subvol, fcb->inode, TYPE_EXTENT_DATA, start_data, ed, edsize, NULL, rollback)) {
        ERR("insert_tree_item failed\n");
        ExFreePool(ed);
        return FALSE;
    }
    
    increase_chunk_usage(c, length);
    add_to_space_list(c, address, length, SPACE_TYPE_WRITING);
    
    fcb->inode_item.st_blocks += length;
    
    return TRUE;
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

    if (tp.item->key.offset + ed->decoded_size != start_data) {
        TRACE("last EXTENT_DATA does not run up to start_data (%llx + %llx != %llx)\n", tp.item->key.offset, ed->decoded_size, start_data);
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
    
    ed2 = (EXTENT_DATA2*)ed->data;
    
    if (tp.item->size < sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2)) {
        ERR("(%llx,%x,%llx) was %u bytes, expected %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_DATA) - 1 + sizeof(EXTENT_DATA2));
        goto end;
    }
    
    if (ed2->size - ed2->offset != ed->decoded_size) {
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
        goto end2;
    }
    
    if (tp2.item->size == sizeof(EXTENT_ITEM_V0)) { // old extent ref, convert
        NTSTATUS Status = convert_old_data_extent(Vcb, ed2->address, ed2->size, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("convert_old_data_extent returned %08x\n", Status);
            goto end2;
        }
        
        free_traverse_ptr(&tp2);
        
        Status = find_item(Vcb, Vcb->extent_root, &tp2, &searchkey, FALSE);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            goto end;
        }
        
        if (keycmp(&tp2.item->key, &searchkey)) {
            WARN("extent item not found for address %llx, size %llx\n", ed2->address, ed2->size);
            goto end2;
        }
    }
    
    ei = (EXTENT_ITEM*)tp2.item->data;
    
    if (tp.item->size < sizeof(EXTENT_ITEM)) {
        ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_ITEM));
        goto end2;
    }
    
    // FIXME - test this
    if (extent_item_is_shared(ei, tp2.item->size - sizeof(EXTENT_ITEM))) {
        NTSTATUS Status = convert_shared_data_extent(Vcb, ed2->address, ed2->size, rollback);
        if (!NT_SUCCESS(Status)) {
            ERR("convert_shared_data_extent returned %08x\n", Status);
            goto end2;
        }
        
        free_traverse_ptr(&tp2);
        
        Status = find_item(Vcb, Vcb->extent_root, &tp2, &searchkey, FALSE);
        if (!NT_SUCCESS(Status)) {
            ERR("error - find_item returned %08x\n", Status);
            goto end;
        }
        
        if (keycmp(&tp2.item->key, &searchkey)) {
            WARN("extent item not found for address %llx, size %llx\n", ed2->address, ed2->size);
            goto end2;
        }
        
        ei = (EXTENT_ITEM*)tp2.item->data;
        
        if (tp.item->size < sizeof(EXTENT_ITEM)) {
            ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_ITEM));
            goto end2;
        }
    }
    
    if (ei->refcount != 1) {
        TRACE("extent refcount was not 1\n");
        goto end2;
    }
    
    if (ei->flags != EXTENT_ITEM_DATA) {
        ERR("error - extent was not a data extent\n");
        goto end2;
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
    
end2:
    free_traverse_ptr(&tp2);
    
end:
    free_traverse_ptr(&tp);
        
    return success;
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

        if (tp.item->key.obj_type == TYPE_EXTENT_DATA && tp.item->size >= sizeof(EXTENT_DATA))
            ed = (EXTENT_DATA*)tp.item->data;
        else
            ed = NULL;
        
        if (tp.item->key.obj_id != fcb->inode || tp.item->key.obj_type != TYPE_EXTENT_DATA || !ed || tp.item->key.offset + ed->decoded_size < start_data) {
            if (tp.item->key.obj_id != fcb->inode || tp.item->key.obj_type != TYPE_EXTENT_DATA)
                Status = insert_sparse_extent(Vcb, fcb->subvol, fcb->inode, 0, start_data, rollback);
            else if (!ed)
                ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_DATA));
            else {
                Status = insert_sparse_extent(Vcb, fcb->subvol, fcb->inode, tp.item->key.offset + ed->decoded_size,
                                              start_data - tp.item->key.offset - ed->decoded_size, rollback);
            }
            if (!NT_SUCCESS(Status)) {
                ERR("insert_sparse_extent returned %08x\n", Status);
                free_traverse_ptr(&tp);
                return Status;
            }
        }
        
        free_traverse_ptr(&tp);
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
            if (insert_extent_chunk(Vcb, fcb, c, start_data, length, data, changed_sector_list, rollback))
                return STATUS_SUCCESS;
        }

        le = le->Flink;
    }
    
    if ((c = alloc_chunk(Vcb, flags, rollback))) {
        if (c->chunk_item->type == flags && (c->chunk_item->size - c->used) >= length) {
            if (insert_extent_chunk(Vcb, fcb, c, start_data, length, data, changed_sector_list, rollback))
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
            
            free_traverse_ptr(&tp);
            
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
            
            free_traverse_ptr(&tp);
            
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
                    free_traverse_ptr(&tp);
                    tp = next_tp;
                } else
                    break;
            }
    //         ERR("end loop\n");
            
            free_traverse_ptr(&tp);
            
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

NTSTATUS extend_file(fcb* fcb, UINT64 end, LIST_ENTRY* rollback) {
    UINT64 oldalloc, newalloc;
    KEY searchkey;
    traverse_ptr tp;
    BOOL cur_inline;
    NTSTATUS Status;
    
    TRACE("(%p, %x, %p)\n", fcb, end, rollback);

    if (fcb->ads) {
        FIXME("FIXME - support streams here\n"); // FIXME
        return STATUS_NOT_IMPLEMENTED;
    } else {
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
            if (tp.item->size < sizeof(EXTENT_DATA)) {
                ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_DATA));
                free_traverse_ptr(&tp);
                return STATUS_INTERNAL_ERROR;
            }
            
            oldalloc = tp.item->key.offset + ((EXTENT_DATA*)tp.item->data)->decoded_size;
            cur_inline = ((EXTENT_DATA*)tp.item->data)->type == EXTENT_TYPE_INLINE;
        
            if (cur_inline && end > fcb->Vcb->max_inline) {
                LIST_ENTRY changed_sector_list;
                BOOL nocsum = fcb->inode_item.flags & BTRFS_INODE_NODATASUM;
                UINT64 origlength, length;
                UINT8* data;
                
                TRACE("giving inline file proper extents\n");
                
                origlength = ((EXTENT_DATA*)tp.item->data)->decoded_size;
                
                cur_inline = FALSE;
                
                if (!nocsum)
                    InitializeListHead(&changed_sector_list);
                
                delete_tree_item(fcb->Vcb, &tp, rollback);
                
                length = sector_align(origlength, fcb->Vcb->superblock.sector_size);
                
                data = ExAllocatePoolWithTag(PagedPool, length, ALLOC_TAG);
                if (!data) {
                    ERR("could not allocate %llx bytes for data\n", length);
                    free_traverse_ptr(&tp);
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
                
                if (length > origlength)
                    RtlZeroMemory(data + origlength, length - origlength);
                
                RtlCopyMemory(data, ((EXTENT_DATA*)tp.item->data)->data, origlength);
                
                fcb->inode_item.st_blocks -= origlength;
                
                Status = insert_extent(fcb->Vcb, fcb, tp.item->key.offset, length, data, nocsum ? NULL : &changed_sector_list, rollback);
                if (!NT_SUCCESS(Status)) {
                    ERR("insert_extent returned %08x\n", Status);
                    free_traverse_ptr(&tp);
                    ExFreePool(data);
                    return Status;
                }
                
                oldalloc = tp.item->key.offset + length;
                
                ExFreePool(data);
                
                if (!nocsum)
                    update_checksum_tree(fcb->Vcb, &changed_sector_list, rollback);
            }
            
            if (cur_inline) {
                EXTENT_DATA* ed;
                ULONG edsize;
                
                if (end > oldalloc) {
                    edsize = sizeof(EXTENT_DATA) - 1 + end - tp.item->key.offset;
                    ed = ExAllocatePoolWithTag(PagedPool, edsize, ALLOC_TAG);
                    
                    if (!ed) {
                        ERR("out of memory\n");
                        free_traverse_ptr(&tp);
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }
                    
                    RtlZeroMemory(ed, edsize);
                    RtlCopyMemory(ed, tp.item->data, tp.item->size);
                    
                    ed->decoded_size = end - tp.item->key.offset;
                    
                    delete_tree_item(fcb->Vcb, &tp, rollback);
                    
                    if (!insert_tree_item(fcb->Vcb, fcb->subvol, tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, ed, edsize, NULL, rollback)) {
                        ERR("error - failed to insert item\n");
                        ExFreePool(ed);
                        free_traverse_ptr(&tp);
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
                    Status = insert_sparse_extent(fcb->Vcb, fcb->subvol, fcb->inode, oldalloc, newalloc - oldalloc, rollback);
                    
                    if (!NT_SUCCESS(Status)) {
                        ERR("insert_sparse_extent returned %08x\n", Status);
                        free_traverse_ptr(&tp);
                        return Status;
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
            
                Status = insert_sparse_extent(fcb->Vcb, fcb->subvol, fcb->inode, 0, newalloc, rollback);
                
                if (!NT_SUCCESS(Status)) {
                    ERR("insert_sparse_extent returned %08x\n", Status);
                    free_traverse_ptr(&tp);
                    return Status;
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
                    free_traverse_ptr(&tp);
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
                    free_traverse_ptr(&tp);
                    return STATUS_INTERNAL_ERROR;
                }
                
                fcb->inode_item.st_size = end;
                TRACE("setting st_size to %llx\n", end);
                
                fcb->inode_item.st_blocks = end;

                fcb->Header.AllocationSize.QuadPart = fcb->Header.FileSize.QuadPart = fcb->Header.ValidDataLength.QuadPart = end;
            }
        }
        
        free_traverse_ptr(&tp);
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
        free_traverse_ptr(&tp);
        return 0;
    }
    
    if (tp.item->size < sizeof(EXTENT_ITEM)) {
        ERR("(%llx,%x,%llx) was %u bytes, expected %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_ITEM));
        free_traverse_ptr(&tp);
        return 0;
    }
    
    ei = (EXTENT_ITEM*)tp.item->data;
    rc = ei->refcount;
    
    free_traverse_ptr(&tp);
    
    return rc;
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
            free_traverse_ptr(&tp);
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
    free_traverse_ptr(&tp);
    
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
        goto failure2;
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
    
    length = oldlength = ed->decoded_size;
    lastoff = tp.item->key.offset;
    
    TRACE("(%llx,%x,%llx) length = %llx\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, length);
    
    alloc = 0;
    if (ed->type != EXTENT_TYPE_REGULAR || ed2->address != 0) {
        alloc += length;
    }
    
    while (find_next_item(Vcb, &tp, &next_tp, FALSE)) {
        if (next_tp.item->key.obj_id != searchkey.obj_id || next_tp.item->key.obj_type != searchkey.obj_type) {
            free_traverse_ptr(&next_tp);
            break;
        }
        
        free_traverse_ptr(&tp);
        tp = next_tp;
        
        if (tp.item->size < sizeof(EXTENT_DATA)) {
            ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, sizeof(EXTENT_DATA));
            goto failure;
        }
        
        ed = (EXTENT_DATA*)tp.item->data;
        ed2 = (EXTENT_DATA2*)&ed->data[0];
    
        length = ed->decoded_size;
    
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
    
    free_traverse_ptr(&tp);
    
    return;
    
failure:
    free_traverse_ptr(&tp);
    
failure2:
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
    
    if (fcb->type != BTRFS_TYPE_FILE && fcb->type != BTRFS_TYPE_SYMLINK) {
        WARN("tried to write to something other than a file or symlink (inode %llx, type %u, %p, %p)\n", fcb->inode, fcb->type, &fcb->type, fcb);
        return STATUS_ACCESS_DENIED;
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
                TRACE("filename %.*S\n", fcb->full_filename.Length / sizeof(WCHAR), fcb->full_filename.Buffer);
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
            Status = extend_file(fcb, newlength, rollback);
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
        
        TRACE("CcCopyWrite(%p, %llx, %x, %u, %p)\n", FileObject, offset.QuadPart, *length, wait, buf);
        if (!CcCopyWrite(FileObject, &offset, *length, wait, buf)) {
            TRACE("CcCopyWrite failed.\n");
            
            IoMarkIrpPending(Irp);
            Status = STATUS_PENDING;
            goto end;
        }
        TRACE("CcCopyWrite finished\n");
        
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
                free_traverse_ptr(&tp);
                Status = STATUS_INTERNAL_ERROR;
                goto end;
            }
            
            if (tp.item->size < datalen) {
                ERR("(%llx,%x,%llx) was %u bytes, expected at least %u\n", tp.item->key.obj_id, tp.item->key.obj_type, tp.item->key.offset, tp.item->size, datalen);
                free_traverse_ptr(&tp);
                Status = STATUS_INTERNAL_ERROR;
                goto end;
            }
            
            maxlen -= tp.item->size - datalen; // subtract XATTR_ITEM overhead
            
            free_traverse_ptr(&tp);
            
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

        if (make_inline || !nocow) {
            Status = excise_extents(fcb->Vcb, fcb, start_data, end_data, nocsum ? NULL : &changed_sector_list, rollback);
            if (!NT_SUCCESS(Status)) {
                ERR("error - excise_extents returned %08x\n", Status);
                ExFreePool(data);
                goto end;
            }
            
            if (!make_inline) {
                Status = insert_extent(fcb->Vcb, fcb, start_data, end_data - start_data, data, nocsum ? NULL : &changed_sector_list, rollback);
                
                if (!NT_SUCCESS(Status)) {
                    ERR("error - insert_extent returned %08x\n", Status);
                    ExFreePool(data);
                    goto end;
                }
                
                ExFreePool(data);
            } else {
                ed2 = (EXTENT_DATA*)data;
                ed2->generation = fcb->Vcb->superblock.generation;
                ed2->decoded_size = newlength;
                ed2->compression = BTRFS_COMPRESSION_NONE;
                ed2->encryption = BTRFS_ENCRYPTION_NONE;
                ed2->encoding = BTRFS_ENCODING_NONE;
                ed2->type = EXTENT_TYPE_INLINE;
                
                insert_tree_item(Vcb, fcb->subvol, fcb->inode, TYPE_EXTENT_DATA, 0, ed2, sizeof(EXTENT_DATA) - 1 + newlength, NULL, rollback);
                
                fcb->inode_item.st_blocks += newlength;
            }
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
    
    if (fcb->ads)
        origii = &fcb->par->inode_item;
    else
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
        free_traverse_ptr(&tp);
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }
    
    RtlCopyMemory(ii, origii, sizeof(INODE_ITEM));
    insert_tree_item(Vcb, fcb->subvol, fcb->inode, TYPE_INODE_ITEM, 0, ii, sizeof(INODE_ITEM), NULL, rollback);
    
    free_traverse_ptr(&tp);
    
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
    
    switch (IrpSp->MinorFunction) {
        case IRP_MN_COMPLETE:
            FIXME("unsupported - IRP_MN_COMPLETE\n");
            break;

        case IRP_MN_COMPLETE_MDL:
            FIXME("unsupported - IRP_MN_COMPLETE_MDL\n");
            break;

        case IRP_MN_COMPLETE_MDL_DPC:
            FIXME("unsupported - IRP_MN_COMPLETE_MDL_DPC\n");
            break;

        case IRP_MN_COMPRESSED:
            FIXME("unsupported - IRP_MN_COMPRESSED\n");
            break;

        case IRP_MN_DPC:
            FIXME("unsupported - IRP_MN_DPC\n");
            break;

        case IRP_MN_MDL:
            FIXME("unsupported - IRP_MN_MDL\n");
            break;

        case IRP_MN_MDL_DPC:
            FIXME("unsupported - IRP_MN_MDL_DPC\n");
            break;

        case IRP_MN_NORMAL:
            TRACE("IRP_MN_NORMAL\n");
            break;

        default:
            WARN("unknown minor function %x\n", IrpSp->MinorFunction);
            break;
    }
    
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
    
    acquire_tree_lock(Vcb, TRUE);
    locked = TRUE;
    
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
    
    Status = consider_write(Vcb);

    if (NT_SUCCESS(Status)) {
        Irp->IoStatus.Information = IrpSp->Parameters.Write.Length;
    
#ifdef DEBUG_PARANOID
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
